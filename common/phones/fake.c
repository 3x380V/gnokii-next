/*

  $Id: fake.c,v 1.2 2002-04-18 22:08:06 machek Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Jan�k ml.
  Copyright (C) 2000 Chris Kemp
  Copyright (C) 2001 Markus Plail, Pawe� Kot
  Copyright (C) 2002 Pavel Machek

  Released under the terms of the GNU GPL, see file COPYING for more details.

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

static SMSMessage_PhoneLayout fake_layout;

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
	 0, true, false,				/* SMSC */
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
	GSM_Error err;
	int try = 0, connected = 0;

	dprintf("Initializing...\n");

	/* Copy in the phone info */
	memcpy(&(state->Phone), &phone_fake, sizeof(GSM_Phone));

	/* SMS Layout */
	fake_layout.Type = 8; /* Locate the Type of the mesage field. */
	fake_layout.SendHeader = 6;
	fake_layout.ReadHeader = 4;
	fake_layout.Deliver = at_deliver;
	fake_layout.Submit =  at_submit;
	fake_layout.DeliveryReport = at_deliver;
	fake_layout.Picture = at_deliver;
	layout = fake_layout;

	dprintf("Connecting\n");

	/* Now test the link and get the model */
	GSM_DataClear(&data);
	data.Model = model;

	return GE_NONE;
}

static GSM_Error AT_WriteSMS(GSM_Data *data, GSM_Statemachine *state, char* cmd)
{
	unsigned char req[256];
	GSM_Error error;
	int length, i;

	if (!data->RawData) return GE_INTERNALERROR;

	printf("In PDU mode: ");
	dprintf("AT mode set\n");
	/* 6 is the frame header as above */
	length = data->RawData->Length;
	printf("(%d bytes)\n", length);
	for (i = 0; i < length; i++) {
		printf("%02x ", data->RawData->Data[i]);
	}
	printf("\n");
	return GE_NONE;
}

static GSM_Error Pfake_Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state)
{
	printf("Doing operation #%d\n", op);
	switch (op) {
	case GOP_Init:
		return Pfake_Initialise(state);
	case GOP_SendSMS:
		return AT_WriteSMS(data, state, "Writing ");
	default:
		return GE_NOTIMPLEMENTED;
	}
	return GE_INTERNALERROR;
}
