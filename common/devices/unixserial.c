/*

  $Id: unixserial.c,v 1.26 2002-10-30 00:52:07 bozo Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  This file is part of gnokii.

  Gnokii is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Gnokii is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with gnokii; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Jan�k ml.

*/

/* [global] option "serial_write_usleep" default: */
#define SERIAL_WRITE_USLEEP_DEFAULT (-1)

#include "misc.h"
#include "cfgreader.h"

/* Do not compile this file under Win32 systems. */

#ifndef WIN32

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <termios.h>
#include "devices/unixserial.h"

#ifdef HAVE_SYS_IOCTL_COMPAT_H
#  include <sys/ioctl_compat.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif

#ifdef HAVE_SYS_MODEM_H
#  include <sys/modem.h>
#endif

#ifdef HAVE_SYS_FILIO_H
#  include <sys/filio.h>
#endif

/* If the target operating system does not have cfsetspeed, we can emulate
   it. */

#ifndef HAVE_CFSETSPEED
#  if defined(HAVE_CFSETISPEED) && defined(HAVE_CFSETOSPEED)
#    define cfsetspeed(t, speed) \
	    (cfsetispeed(t, speed) || cfsetospeed(t, speed))
#  else
static int cfsetspeed(struct termios *t, int speed)
{
#    ifdef HAVE_TERMIOS_CSPEED
	t->c_ispeed = speed;
	t->c_ospeed = speed;
#    else
	t->c_cflag |= speed;
#    endif			/* HAVE_TERMIOS_CSPEED */
	return 0;
}
#  endif			/* HAVE_CFSETISPEED && HAVE_CFSETOSPEED */
#endif				/* HAVE_CFSETSPEED */

#ifndef O_NONBLOCK
#  define O_NONBLOCK  0
#endif

/* Structure to backup the setting of the terminal. */
struct termios serial_termios;

/* Script handling: */

static void device_script_cfgfunc(const char *section, const char *key, const char *value)
{
#if !defined(__svr4__)
#  if defined(HAVE_SETENV)
	setenv(key, value, 1 /* overwrite */); /* errors ignored */
#  else
	char *str;
	asprintf(&str, "%s=%s", key, value);
	putenv(str);
#  endif
#endif
}

int device_script(int fd, const char *section)
{
	pid_t pid;
	const char *scriptname = gn_cfg_get(gn_cfg_info, "global", section);
	int status;

	if (!scriptname)
		return 0;

	errno = 0;
	switch ((pid = fork())) {
	case -1:
		fprintf(stderr, _("device_script(\"%s\"): fork() failure: %s!\n"), scriptname, strerror(errno));
		return -1;

	case 0: /* child */
		cfg_get_foreach(gn_cfg_info, section, device_script_cfgfunc);
		errno = 0;
		if (dup2(fd, 0) != 0 || dup2(fd, 1) != 1 || close(fd)) {
			fprintf(stderr, _("device_script(\"%s\"): file descriptor prepare: %s\n"), scriptname, strerror(errno));
			_exit(-1);
		}
		/* FIXME: close all open descriptors - how to track them?
		 */
		execl("/bin/sh", "sh", "-c", scriptname, NULL);
		fprintf(stderr, _("device_script(\"%s\"): execute script: %s\n"), scriptname, strerror(errno));
		_exit(-1);
		/* NOTREACHED */

	default:
		if (pid == waitpid(pid, &status, 0 /* options */) && WIFEXITED(status) && !WEXITSTATUS(status))
			return 0;
		fprintf(stderr, _("device_script(\"%s\"): child script failure: %s, exit code=%d\n"), scriptname,
			(WIFEXITED(status) ? _("normal exit") : _("abnormal exit")),
			(WIFEXITED(status) ? WEXITSTATUS(status) : -1));
		errno = EIO;
		return -1;

	}
	/* NOTREACHED */
}


int serial_close(int fd);


/* Open the serial port and store the settings. */
int serial_open(const char *file, int oflag)
{
	int fd;
	int retcode;

	fd = open(file, oflag);
	if (fd == -1) {
		perror("Gnokii serial_open: open");
		return -1;
	}

	retcode = tcgetattr(fd, &serial_termios);
	if (retcode == -1) {
		perror("Gnokii serial_open:tcgetattr");
		/* Don't call serial_close since serial_termios is not valid */
		close(fd);
		return -1;
	}

	return fd;
}

/* Close the serial port and restore old settings. */
int serial_close(int fd)
{
	/* handle config file disconnect_script:
	 */
	if (device_script(fd, "disconnect_script") == -1)
		fprintf(stderr, _("Gnokii serial_close: disconnect_script\n"));

	if (fd >= 0) {
#if 1 /* HACK */
		serial_termios.c_cflag |= HUPCL;	/* production == 1 */
#else
		serial_termios.c_cflag &= ~HUPCL;	/* debugging  == 0 */
#endif

		tcsetattr(fd, TCSANOW, &serial_termios);
	}

	return close(fd);
}

/* Open a device with standard options.
 * Use value (-1) for "with_hw_handshake" if its specification is required from the user.
 */
int serial_opendevice(const char *file, int with_odd_parity,
		      int with_async, int with_hw_handshake)
{
	int fd;
	int retcode, baudrate = 0;
	struct termios tp;
	char *s;

	/* handle config file handshake override: */
	s = gn_cfg_get(gn_cfg_info, "global", "handshake");

	if (s && (!strcasecmp(s, "software") || !strcasecmp(s, "rtscts")))
		with_hw_handshake = false;
	else if (s && (!strcasecmp(s, "hardware") || !strcasecmp(s, "xonxoff")))
		with_hw_handshake = true;
	else if (s)
		fprintf(stderr, _("Unrecognized [%s] option \"%s\", use \"%s\" or \"%s\" value, ignoring!"),
				 "global", "handshake", "software", "hardware");

	if (with_hw_handshake == -1) {
		fprintf(stderr, _("[%s] option \"%s\" not found, trying to use \"%s\" value!"),
				  "global", "handshake", "software");
		with_hw_handshake = false;
	}

	/* Open device */

	/* O_NONBLOCK MUST be used here as the CLOCAL may be currently off
	 * and if DCD is down the "open" syscall would be stuck wating for DCD.
	 */
	fd = serial_open(file, O_RDWR | O_NOCTTY | O_NONBLOCK);

	if (fd < 0) return fd;

	/* Initialise the port settings */
	memcpy(&tp, &serial_termios, sizeof(struct termios));

	/* Set port settings for canonical input processing */
	tp.c_cflag = B0 | CS8 | CLOCAL | CREAD | HUPCL;
	if (with_odd_parity) {
		tp.c_cflag |= (PARENB | PARODD);
		tp.c_iflag = 0;
	} else
		tp.c_iflag = IGNPAR;
#ifdef CRTSCTS
	if (with_hw_handshake)
		tp.c_cflag |= CRTSCTS;
	else
		tp.c_cflag &= ~CRTSCTS;
#endif

	tp.c_oflag = 0;
	tp.c_lflag = 0;
	tp.c_cc[VMIN] = 1;
	tp.c_cc[VTIME] = 0;

	retcode = tcflush(fd, TCIFLUSH);
	if (retcode == -1) {
		perror("Gnokii serial_opendevice: tcflush");
		serial_close(fd);
		return -1;
	}

	retcode = tcsetattr(fd, TCSANOW, &tp);
	if (retcode == -1) {
		perror("Gnokii serial_opendevice: tcsetattr");
		serial_close(fd);
		return -1;
	}

	/* Set speed */
	s = gn_cfg_get(gn_cfg_info, "global", "serial_baudrate"); /* baud rate string */

	if (s) baudrate = atoi(s);
	if (baudrate && serial_changespeed(fd, baudrate) != GN_ERR_NONE)
		baudrate = 0;
	if (!baudrate)
		serial_changespeed(fd, 19200 /* default value */ );

	/* We need to turn off O_NONBLOCK now (we have CLOCAL set so it is safe).
	 * When we run some device script it really doesn't expect NONBLOCK!
	 */

	retcode = fcntl(fd, F_SETFL, 0);
	if (retcode == -1) {
		perror("Gnokii serial_opendevice: fnctl(F_SETFL)");
		serial_close(fd);
		return -1;
	}

	/* handle config file connect_script:
	 */
	if (device_script(fd,"connect_script") == -1) {
		fprintf(stderr,"Gnokii serial_opendevice: connect_script\n");
		serial_close(fd);
		return -1;
	}

	/* Allow process/thread to receive SIGIO */

#if !(__unices__)
	retcode = fcntl(fd, F_SETOWN, getpid());
	if (retcode == -1) {
		perror("Gnokii serial_opendevice: fnctl(F_SETOWN)");
		serial_close(fd);
		return -1;
	}
#endif

	/* Make filedescriptor asynchronous. */

	/* We need to supply FNONBLOCK (or O_NONBLOCK) again as it would get reset
	 * by F_SETFL as a side-effect!
	 */
#ifdef FNONBLOCK
	retcode = fcntl(fd, F_SETFL, (with_async ? FASYNC : 0) | FNONBLOCK);
#else
#  ifdef FASYNC
	retcode = fcntl(fd, F_SETFL, (with_async ? FASYNC : 0) | O_NONBLOCK);
#  else
	retcode = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (retcode != -1)
		retcode = ioctl(fd, FIOASYNC, &with_async);
#  endif
#endif
	if (retcode == -1) {
		perror("Gnokii serial_opendevice: fnctl(F_SETFL)");
		serial_close(fd);
		return -1;
	}

	return fd;
}

/* Set the DTR and RTS bit of the serial device. */
void serial_setdtrrts(int fd, int dtr, int rts)
{
	unsigned int flags;

	flags = TIOCM_DTR;

	if (dtr)
		ioctl(fd, TIOCMBIS, &flags);
	else
		ioctl(fd, TIOCMBIC, &flags);

	flags = TIOCM_RTS;

	if (rts)
		ioctl(fd, TIOCMBIS, &flags);
	else
		ioctl(fd, TIOCMBIC, &flags);
}


int serial_select(int fd, struct timeval *timeout)
{
	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	return select(fd + 1, &readfds, NULL, NULL, timeout);
}


/* Change the speed of the serial device.
 * RETURNS: Success
 */
gn_error serial_changespeed(int fd, int speed)
{
	gn_error retcode = GN_ERR_NONE;
#ifndef SGTTY
	struct termios t;
#else
	struct sgttyb t;
#endif
	int new_speed = B9600;

	switch (speed) {
	case 9600:
		new_speed = B9600;
		break;
	case 19200:
		new_speed = B19200;
		break;
	case 38400:
		new_speed = B38400;
		break;
	case 57600:
		new_speed = B57600;
		break;
	case 115200:
		new_speed = B115200;
		break;
	default:
		fprintf(stderr, _("Serial port speed %d not supported!\n"), speed);
		return GN_ERR_NOTSUPPORTED;
	}

#ifndef SGTTY
	if (tcgetattr(fd, &t)) retcode = GN_ERR_INTERNALERROR;

	if (cfsetspeed(&t, new_speed) == -1) {
		dprintf("Serial port speed setting failed\n");
		retcode = GN_ERR_INTERNALERROR;
	}

	tcsetattr(fd, TCSADRAIN, &t);
#else
	if (ioctl(fd, TIOCGETP, &t)) retcode = GN_ERR_INTERNALERROR;

	t.sg_ispeed = new_speed;
	t.sg_ospeed = new_speed;

	if (ioctl(fd, TIOCSETN, &t)) retcode = GN_ERR_INTERNALERROR;
#endif
	return retcode;
}

/* Read from serial device. */
size_t serial_read(int fd, __ptr_t buf, size_t nbytes)
{
	return read(fd, buf, nbytes);
}

#if !defined(TIOCMGET) && defined(TIOCMODG)
#  define TIOCMGET TIOCMODG
#endif

static void check_dcd(int fd)
{
#ifdef TIOCMGET
	int mcs;

	if (ioctl(fd, TIOCMGET, &mcs) || !(mcs & TIOCM_CAR)) {
		fprintf(stderr, _("ERROR: Modem DCD is down and global/require_dcd parameter is set!\n"));
		exit(EXIT_FAILURE);		/* Hard quit of all threads */
	}
#else
	/* Impossible!! (eg. Coherent) */
#endif
}

/* Write to serial device. */
size_t serial_write(int fd, const __ptr_t buf, size_t n)
{
	size_t r = 0;
	ssize_t got;
	static long serial_write_usleep = LONG_MIN;
	static int require_dcd = -1;

	if (serial_write_usleep == LONG_MIN) {
		char *s = gn_cfg_get(gn_cfg_info, "global", "serial_write_usleep");

		serial_write_usleep = (!s ?
			SERIAL_WRITE_USLEEP_DEFAULT : atol(gn_cfg_get(gn_cfg_info, "global", "serial_write_usleep")));
	}

	if (require_dcd == -1) {
		require_dcd = (!!gn_cfg_get(gn_cfg_info, "global", "require_dcd"));
#ifndef TIOCMGET
		if (require_dcd)
			fprintf(stderr, _("WARNING: global/require_dcd argument was set but it is not supported on this system!\n"));
#endif
	}

	if (require_dcd)
		check_dcd(fd);

	if (serial_write_usleep < 0)
		return write(fd, buf, n);

	while (n > 0) {
		got = write(fd, buf, 1);
		if (got <= 0)
			return (!r ? -1 : r);
		buf++;
		n--;
		r++;
		if (serial_write_usleep)
			usleep(serial_write_usleep);
	}
	return r;
}

gn_error serial_nreceived(int fd, int *n)
{
	if (ioctl(fd, FIONREAD, n)) {
		dprintf("serial_nreceived: cannot get the received data size\n");
		return GN_ERR_INTERNALERROR;
	}

	return GN_ERR_NONE;
}

gn_error serial_flush(int fd)
{
	if (tcdrain(fd)) {
		dprintf("serial_flush: cannot flush serial device\n");
		return GN_ERR_INTERNALERROR;
	}

	return GN_ERR_NONE;
}

#endif /* WIN32 */
