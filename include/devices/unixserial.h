/*

  $Id: unixserial.h,v 1.1 2001-02-21 19:57:12 chris Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Jan�k ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  $Log: unixserial.h,v $
  Revision 1.1  2001-02-21 19:57:12  chris
  More fiddling with the directory layout


*/

#ifndef __devices_unixserial_h
#define __devices_unixserial_h

#ifdef WIN32
  #include <stddef.h>
  /* FIXME: this should be solved in config.h in 0.4.0 */
  #define __const const
  typedef void * __ptr_t;
#else
  #include <unistd.h>
#endif	/* WIN32 */

#include "misc.h"

int serial_open(__const char *__file, int __oflag);
int serial_close(int __fd);

int serial_opendevice(__const char *__file, int __with_odd_parity, int __with_async);

void serial_setdtrrts(int __fd, int __dtr, int __rts);
void serial_changespeed(int __fd, int __speed);

size_t serial_read(int __fd, __ptr_t __buf, size_t __nbytes);
size_t serial_write(int __fd, __const __ptr_t __buf, size_t __n);

int serial_select(int fd, struct timeval *timeout);

#endif  /* __devices_unixserial_h */




