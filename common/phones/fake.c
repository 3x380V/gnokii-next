/*

  $Id: fake.c,v 1.9 2002-06-10 21:16:36 pkot Exp $

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

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

  Copyright (C) 2002 Pavel Machek

  This file provides functions for some functionality testing (eg. SMS)

*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "misc.h"
#include "gsm-common.h"
#include "phones/generic.h"
#include "gsm-encoding.h"
#include "gsm-api.h"

/* Some globals */

static GSM_Error Pfake_Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);

static const SMSMessage_Layout at_deliver = {
	true,						/* Is the SMS type supported */
	 1, true, false,				/* SMSC */
	 2,  2, -1,  2, -1, -1,  4, -1, 13,  5,  2,
	 -1, -1, -1,					/* Validity */
	 3, true, false,				/* Remote Number */
	 6, -1,						/* Time */
	-1, -1,						/* Nonstandard fields */
	14, true					/* User Data */
};

static const SMSMessage_Layout at_submit = {
	true,						/* Is the SMS type supported */
	-1, true, false,				/* SMSC */
	-1,  1,  1,  1, -1,  2,  4, -1,  7,  5,  1,
	 6, -1, -1,					/* Validity */
	 3, true, false,				/* Remote Number */
	-1, -1,						/* Time */
	-1, -1,						/* Nonstandard fields */
	 8, true					/* User Data */
};

GSM_Phone phone_fake = {
	NULL,
	PGEN_IncomingDefault,
	/* Mobile phone information */
	{
		"fake",      /* Supported models */
		7,                     /* Max RF Level */
		0,                     /* Min RF Level */
		GRF_Percentage,        /* RF level units */
		7,                     /* Max Battery Level */
		0,                     /* Min Battery Level */
		GBU_Percentage,        /* Battery level units */
		GDT_DateTime,          /* Have date/time support */
		GDT_TimeOnly,	       /* Alarm supports time only */
		1,                     /* Alarms available - FIXME */
		60, 96,                /* Startup logo size - 7110 is fixed at init */
		21, 78,                /* Op logo size */
		14, 72                 /* Caller logo size */
	},
	Pfake_Functions
};


/* Initialise is the only function allowed to 'use' state */
static GSM_Error Pfake_Initialise(GSM_Statemachine *state)
{
	GSM_Data data;
	char model[10];

	dprintf("Initializing...\n");

	/* Copy in the phone info */
	memcpy(&(state->Phone), &phone_fake, sizeof(GSM_Phone));

	dprintf("Connecting\n");

	/* Now test the link and get the model */
	GSM_DataClear(&data);
	data.Model = model;

	return GE_NONE;
}

static GSM_Error AT_WriteSMS(GSM_Data *data, GSM_Statemachine *state, char* cmd)
{
	unsigned char req[10240];
	int length;

	EncodeByLayout(data, &at_submit, 0);
	if (!data->RawData) return GE_INTERNALERROR;

	length = data->RawData->Length;

	if (length < 0) return GE_SMSWRONGFORMAT;
	fprintf(stdout, "AT+%s=%d\n", cmd, length - data->RawData->Data[0] - 1);

	bin2hex(req, data->RawData->Data, data->RawData->Length);
	req[data->RawData->Length * 2] = 0x1a;
	req[data->RawData->Length * 2 + 1] = 0;
	fprintf(stdout, "%s\n", req);
	return GE_NONE;
}

static GSM_Error Pfake_Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state)
{
	switch (op) {
	case GOP_Init:
		return Pfake_Initialise(state);
	case GOP_Terminate:
		return GE_NONE;
	case GOP_SendSMS:
		return AT_WriteSMS(data, state, "???");
	case GOP_GetSMSCenter:
		return GE_NONE;
	default:
		return GE_NOTIMPLEMENTED;
	}
	return GE_INTERNALERROR;
}
