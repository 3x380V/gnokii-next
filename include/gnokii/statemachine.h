/*

  $Id: statemachine.h,v 1.3 2001-12-28 16:00:31 pkot Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Jan�k ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Header file for the statemachine.

*/

#ifndef __gsm_statemachine_h
#define __gsm_statemachine_h

#include "gsm-common.h"

GSM_Error SM_Initialise(GSM_Statemachine *state);
GSM_State SM_Loop(GSM_Statemachine *state, int timeout);
GSM_Error SM_SendMessage(GSM_Statemachine *state, u16 messagesize, u8 messagetype, void *message);
GSM_Error SM_WaitFor(GSM_Statemachine *state, GSM_Data *data, unsigned char messagetype);
void SM_IncomingFunction(GSM_Statemachine *state, u8 messagetype, void *message, u16 messagesize);
void SM_Reset(GSM_Statemachine *state);
GSM_Error SM_GetError(GSM_Statemachine *state, unsigned char messagetype);
GSM_Error SM_Block(GSM_Statemachine *state, GSM_Data *data, int waitfor);
GSM_Error SM_BlockNoRetry(GSM_Statemachine *state, GSM_Data *data, int waitfor);
GSM_Error SM_Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *sm);

#endif	/* __gsm_statemachine_h */
