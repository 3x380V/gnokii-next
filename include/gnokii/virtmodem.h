/*

  $Id: virtmodem.h,v 1.1 2001-02-21 19:57:11 chris Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Jan�k ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file for virtmodem code in virtmodem.c

  $Log: virtmodem.h,v $
  Revision 1.1  2001-02-21 19:57:11  chris
  More fiddling with the directory layout


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
int  VM_PtySetup(char *bindir);
void VM_ThreadLoop(void);
void VM_CharHandler(void);
int  VM_GetMasterPty(char **name);
void VM_Terminate(void);
GSM_Error VM_GSMInitialise(char *model,
			   char *port,
			   char *initlength,
			   GSM_ConnectionType connection);

#endif	/* __virtmodem_h */
