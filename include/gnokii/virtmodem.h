/*

  $Id: virtmodem.h,v 1.8 2002-12-10 23:27:19 pkot Exp $

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

#ifndef _gnokii_data_virtmodem_h
#define _gnokii_data_virtmodem_h

struct vm_queue
{
	int n;
	int head;
	int tail;
	unsigned char buf[256];
};

extern struct vm_queue queue;


/* Prototypes */
bool vm_initialise(char *model,
		   char *port,
		   char *initlength,
		   const char *connection,
		   char *bindir,
		   bool debug_mode,
		   bool GSM_Init);
void vm_loop(void);
void vm_terminate(void);

#endif	/* _gnokii_data_virtmodem_h */
