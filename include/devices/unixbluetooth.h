/*

  $Id: unixbluetooth.h,v 1.3 2003-02-18 22:02:46 pkot Exp $
 
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
  Copyright (C) 2002       Marcel Holtmann <marcel@holtmann.org>

*/

#ifndef _gnokii_unix_bluetooth_h
#define _gnokii_unix_bluetooth_h

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "misc.h"

int bluetooth_open(bdaddr_t *bdaddr, uint8_t channel, struct gn_statemachine *state);
int bluetooth_close(int fd, struct gn_statemachine *state);
int bluetooth_write(int fd, const __ptr_t bytes, int size, struct gn_statemachine *state);
int bluetooth_read(int fd, __ptr_t bytes, int size, struct gn_statemachine *state);
int bluetooth_select(int fd, struct timeval *timeout, struct gn_statemachine *state);

#endif /* _gnokii_unix_bluetooth_h */
