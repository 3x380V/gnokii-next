/*

  $Id: virtmodem.h,v 1.4 2002-02-13 22:13:16 pkot Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Jan�k ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

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
