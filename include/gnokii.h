/*

  $Id: gnokii.h,v 1.17 2002-01-21 12:31:24 machek Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Jan�k ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Header file for the various functions, definitions etc. used to implement
  the handset interface.  See gsm-api.c for more details.

*/

#ifndef __gsm_api_h
#define __gsm_api_h

#include "gsm-data.h"
#include "data/rlp-common.h"
#include "gsm-statemachine.h"

/* Define these as externs so that app code can pick them up. */

extern bool *GSM_LinkOK;
extern GSM_Information *GSM_Info;
extern GSM_Error (*GSM_F)(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);

/* Prototype for the functions actually provided by gsm-api.c. */

GSM_Error GSM_Initialise(char *model, char *device, char *initlength, GSM_ConnectionType connection, void (*rlp_handler)(RLP_F96Frame *frame), GSM_Statemachine *sm);

/* SMS Functions */
GSM_Error SendSMS(GSM_Data *data, GSM_Statemachine *state);
GSM_Error GetSMS(GSM_Data *data, GSM_Statemachine *state);

/* All the rest of the API functions are contained in the GSM_Function
   structure which ultimately points into the model specific code. */

#endif	/* __gsm_api_h */
