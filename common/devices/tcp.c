/*
  
  $Id: tcp.c,v 1.1 2002-04-20 22:24:01 machek Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2002 Jan Kratochvil

  Released under the terms of the GNU GPL, see file COPYING for more details.

*/

#include "misc.h"
#include "cfgreader.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

#if unices
#  include <sys/file.h>
#endif

#include <termios.h>
#include "devices/tcp.h"
#include "devices/unixserial.h"

#ifdef HAVE_SYS_IOCTL_COMPAT_H
  #include <sys/ioctl_compat.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifndef O_NONBLOCK
  #define O_NONBLOCK  0
#endif

/* Open the serial port and store the settings. */

int tcp_open(const char *file) {

	int fd;
	int i;
	struct sockaddr_in addr;
	static bool atexit_registered=false;
	char *filedup,*portstr,*end;
	unsigned long portul;
	struct hostent *hostent;

	if (!atexit_registered) {
		memset(serial_close_all_openfds,-1,sizeof(serial_close_all_openfds));
#if 0	/* Disabled for now as atexit() functions are then called multiple times for pthreads! */
		signal(SIGINT,unixserial_interrupted);
#endif
		atexit(serial_close_all);
		atexit_registered=true;
	}

	fd = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
	if (fd == -1) {
		perror("Gnokii tcp_open: socket()");
		return (-1);
	}
	if (!(filedup=strdup(file))) {
	fail_close:
		close(fd);
		return (-1);
	}
	if (!(portstr=strchr(filedup,':'))) {
		fprintf(stderr,"Gnokii tcp_open: colon (':') not found in connect strings \"%s\"!\n",filedup);
	fail_free:
		free(filedup);
		goto fail_close;
	}
	*portstr++='\0';
	portul=strtoul(portstr,&end,0);
	if ((end && *end) || portul>=0x10000) {
		fprintf(stderr,"Gnokii tcp_open: Port string \"%s\" not valid for IPv4 connection!\n",portstr);
		goto fail_free;
	}
	if (!(hostent=gethostbyname(filedup))) {
		fprintf(stderr,"Gnokii tcp_open: Unknown host \"%s\"!\n",filedup);
		goto fail_free;
	}
	if (hostent->h_addrtype!=AF_INET || hostent->h_length!=sizeof(addr.sin_addr) || !hostent->h_addr_list[0]) {
		fprintf(stderr,"Gnokii tcp_open: Address resolve for host \"%s\" not compatible!\n",filedup);
		goto fail_free;
	}
	free(filedup);
  
	addr.sin_family=AF_INET;
	addr.sin_port=htons(portul);
	memcpy(&addr.sin_addr,hostent->h_addr_list[0],sizeof(addr.sin_addr));

	if (connect(fd,(struct sockaddr *)&addr,sizeof(addr))) {
		perror("Gnokii tcp_open: connect()");
		goto fail_close;
	}

	for (i=0;i<ARRAY_LEN(serial_close_all_openfds);i++)
		if (serial_close_all_openfds[i]==-1 || serial_close_all_openfds[i]==fd) {
			serial_close_all_openfds[i]=fd;
			break;
		}

	return fd;
}

int tcp_close(int fd) {
	int i;

	for (i=0;i<ARRAY_LEN(serial_close_all_openfds);i++)
		if (serial_close_all_openfds[i]==fd)
			serial_close_all_openfds[i]=-1;		/* fd closed */

	/* handle config file disconnect_script:
	 */
	if (-1 == device_script(fd,"disconnect_script"))
		fprintf(stderr,"Gnokii tcp_close: disconnect_script\n");

	return (close(fd));
}

/* Open a device with standard options.
 * Use value (-1) for "with_hw_handshake" if its specification is required from the user
 */
int tcp_opendevice(const char *file, int with_async) {

	int fd;
	int retcode;

	/* Open device */

	fd = tcp_open(file);

	if (fd < 0) 
		return fd;

	/* handle config file connect_script:
	 */
	if (-1 == device_script(fd,"connect_script")) {
		fprintf(stderr,"Gnokii tcp_opendevice: connect_script\n");
		tcp_close(fd);
		return(-1);
	}

	/* Allow process/thread to receive SIGIO */

#if !(unices)
	retcode = fcntl(fd, F_SETOWN, getpid());
	if (retcode == -1){
		perror("Gnokii tcp_opendevice: fnctl(F_SETOWN)");
		tcp_close(fd);
		return(-1);
	}
#endif

	/* Make filedescriptor asynchronous. */

	/* We need to supply FNONBLOCK (or O_NONBLOCK) again as it would get reset
	 * by F_SETFL as a side-effect!
	 */
	retcode=fcntl(fd, F_SETFL, (with_async ? FASYNC : 0) | FNONBLOCK);
	if (retcode == -1){
		perror("Gnokii tcp_opendevice: fnctl(F_SETFL)");
		tcp_close(fd);
		return(-1);
	}
  
	return fd;
}

int tcp_select(int fd, struct timeval *timeout) {

	return serial_select(fd, timeout);
}


/* Read from serial device. */

size_t tcp_read(int fd, __ptr_t buf, size_t nbytes) {

	return (read(fd, buf, nbytes));
}

/* Write to serial device. */

size_t tcp_write(int fd, const __ptr_t buf, size_t n) {

	return(write(fd, buf, n));
}
