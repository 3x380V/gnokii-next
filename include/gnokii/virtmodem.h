/*

  $Id: virtmodem.h,v 1.5 2002-03-28 21:37:48 pkot Exp $

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

  Header file for virtmodem code in virtmodem.c

*/

#ifndef __data_virtmodem_h
#define __data_virtmodem_h

/* Prototypes */

bool VM_Initialise(char *model,
		   char *port,
		   char *initlength,
		   GSM_ConnectionType connection,
		   char *bindir,
		   bool debug_mode,
		   bool GSM_Init);
void VM_Terminate(void);

#endif	/* __virtmodem_h */
