/*
 *
 * $Id: tekram.c,v 1.7 2002-12-27 00:11:39 bozo Exp $
 *
 * G N O K I I
 *
 * A Linux/Unix toolset and driver for Nokia mobile phones.
 *
 * Copyright (C) 1999, 2000 Hugh Blemings & Pavel Jan�k ml.
 * Copyright (C) 2000-2001  Marcel Holtmann <marcel@holtmann.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <string.h>

#include "misc.h"
#include "gsm-api.h"

#ifndef WIN32
#  include "devices/unixserial.h"
#else
#  include "winserial.h"
#endif

#include "devices/tekram.h"

int tekram_open(const char *file, struct gn_statemachine *state)
{
	return serial_open(file, O_RDWR | O_NOCTTY | O_NONBLOCK);
}

void tekram_close(int fd, struct gn_statemachine *state)
{
	serial_setdtrrts(fd, 0, 0, state);
	serial_close(fd, state);
}

void tekram_reset(int fd, struct gn_statemachine *state)
{
	serial_setdtrrts(fd, 0, 0, state);
	usleep(50000);
	serial_setdtrrts(fd, 1, 0, state);
	usleep(1000);
	serial_setdtrrts(fd, 1, 1, state);
	usleep(50);
	serial_changespeed(fd, 9600, state);
}

void tekram_changespeed(int fd, int speed, struct gn_statemachine *state)
{
	unsigned char speedbyte;
	switch (speed) {
	default:
	case 9600:	speedbyte = TEKRAM_PW | TEKRAM_B9600;   break;
	case 19200:	speedbyte = TEKRAM_PW | TEKRAM_B19200;  break;
	case 38400:	speedbyte = TEKRAM_PW | TEKRAM_B38400;  break;
	case 57600:	speedbyte = TEKRAM_PW | TEKRAM_B57600;  break;
	case 115200:	speedbyte = TEKRAM_PW | TEKRAM_B115200; break;
	}
	tekram_reset(fd, state);
	serial_setdtrrts(fd, 1, 0, state);
	usleep(7);
	serial_write(fd, &speedbyte, 1, state);
	usleep(100000);
	serial_setdtrrts(fd, 1, 1, state);
	serial_changespeed(fd, speed, state);
}

size_t tekram_read(int fd, __ptr_t buf, size_t nbytes, struct gn_statemachine *state)
{
	return serial_read(fd, buf, nbytes, state);
}

size_t tekram_write(int fd, const __ptr_t buf, size_t n, struct gn_statemachine *state)
{
	return serial_write(fd, buf, n, state);
}

int tekram_select(int fd, struct timeval *timeout, struct gn_statemachine *state)
{
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);
	return select(fd + 1, &readfds, NULL, NULL, timeout);
}
