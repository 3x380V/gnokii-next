/*

  $Id: datapump.h,v 1.5 2002-02-21 00:56:34 pkot Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Jan�k ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file for data pump code.

*/

#ifndef __data_datapump_h
#define __data_datapump_h

/* Prototypes */

void    DP_CallFinished(void);
bool	DP_Initialise(int read_fd, int write_fd);
int     DP_CallBack(RLP_UserInds ind, u8 *buffer, int length);
void    DP_CallPassup(GSM_CallStatus CallStatus, GSM_CallInfo *CallInfo);
int	DP_SendRLPFrame(RLP_F96Frame *frame, bool out_dtx);

	/* All defines and prototypes from here down are specific to
	   the datapump code and so are #ifdef out if __datapump_c isn't
	   defined. */
#ifdef	__data_datapump_c

#endif	/* __data_datapump_c */

#endif	/* __data_datapump_h */
