/*

  $Id: winserial.h,v 1.3 2002-03-28 21:37:50 pkot Exp $

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

#ifndef __gnokii_winserial_h__
#define __gnokii_winserial_h__

typedef void (*sigcallback)(char);
typedef void (*keepalive)();

int OpenConnection(char *szPort, sigcallback fn, keepalive ka);
int CloseConnection();
int WriteCommBlock(LPSTR lpByte, DWORD dwBytesToWrite);

int ReadCommBlock(LPSTR lpszBlock, int nMaxLength);
void device_changespeed(int speed);
void device_setdtrrts(int dtr, int rts);

#endif
