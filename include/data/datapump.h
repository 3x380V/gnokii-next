/*

  $Id: datapump.h,v 1.6 2002-02-21 01:12:05 pkot Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Jan�k ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file for data pump code.

*/

#ifndef __data_datapump_h
#define __data_datapump_h

/* Prototypes */
bool	DP_Initialise(int read_fd, int write_fd);
void    DP_CallPassup(GSM_CallStatus CallStatus, GSM_CallInfo *CallInfo);

#endif	/* __data_datapump_h */
