/*

  $Id: nk6100.c,v 1.9 2001-11-29 18:01:03 pkot Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Jan�k ml.
  Copyright (C) 2001 Pawe� Kot <pkot@linuxnews.pl>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides functions specific to the 6100 series. 
  See README for more details on supported mobile phones.

*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define __phones_nk6100_c
#include "misc.h"
#include "gsm-common.h"
#include "phones/generic.h"
#include "phones/nk6100.h"
#include "links/fbus.h"
#include "links/fbus-phonet.h"
#include "phones/nokia.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

/* Some globals */

static const SMSMessage_Layout nk6100_deliver = {
	true,
	0, 11, 0, 0, 6, 0, 0, -1, 23, 22, 0, 20,
	8, true, 24, true, 36, -1,
	5, 4,
	43, true
};

static const SMSMessage_Layout nk6100_submit = {
	true,
	-1, 20, 20, 20, 6, 21, 22, -1, 24, 23, 20, 20,
	8, true, 25, true, 37, -1,
	-1, -1,
	44, true
};

static const SMSMessage_Layout nk6100_delivery_report = {
	true,
	0, 0, 0, 0, 6, 0, 0, 0, 22, 21, -1, 20,
	8, true, 24, true, 35, 42,
	5, 4,
	22, true
};

static const SMSMessage_Layout nk6100_picture = {
	false,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, true, 0, true, 0, 0,
	0, 0,
	0, true
};

static SMSMessage_PhoneLayout nk6100_layout;

static unsigned char MagicBytes[4] = { 0x00, 0x00, 0x00, 0x00 };

static GSM_IncomingFunctionType IncomingFunctions[] = {
	{ 0x64, IncomingPhoneInfo },
        { 0xd2, IncomingModelInfo },
	{ 0x14, IncomingSMS },
	{ 0x03, Incoming0x03 },
	{ 0x0a, Incoming0x0a },
	{ 0x17, Incoming0x17 },
	{ 0, NULL}
};

GSM_Phone phone_nokia_6100 = {
	IncomingFunctions,
	PGEN_IncomingDefault,
        /* Mobile phone information */
	{
		"6110|6130|6150|6190|5110|5130|5190|3210|3310|3330", /* Supported models */
		4,                     /* Max RF Level */
		0,                     /* Min RF Level */
		GRF_Arbitrary,         /* RF level units */
		4,                     /* Max Battery Level */
		0,                     /* Min Battery Level */
		GBU_Arbitrary,         /* Battery level units */
		GDT_DateTime,          /* Have date/time support */
		GDT_TimeOnly,	       /* Alarm supports time only */
		1,                     /* Alarms available - FIXME */
		48, 84,                /* Startup logo size - 7110 is fixed at init*/
		14, 72,                /* Op logo size */
		14, 72                 /* Caller logo size */
	},
	Functions
};

static GSM_Error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state)
{
	switch (op) {
	case GOP_Init:
		return Initialise(state);
	case GOP_GetModel:
		return GetModelName(data, state);
	case GOP_GetRevision:
		return GetRevision(data, state);
	case GOP_GetImei:
		return GetIMEI(data, state);
	case GOP_Identify:
		return Identify(data, state);
	case GOP_GetBatteryLevel:
		return GetBatteryLevel(data, state);
	case GOP_GetRFLevel:
		return GetRFLevel(data, state);
	case GOP_GetMemoryStatus:
		return GetMemoryStatus(data, state);
	case GOP_GetSMS:
		return GetSMSMessage(data, state);
	default:
		return GE_NOTIMPLEMENTED;
	}
}

/* static bool LinkOK = true; */

/* Initialise is the only function allowed to 'use' state */
static GSM_Error Initialise(GSM_Statemachine *state)
{
	GSM_Data data;
	char model[10];
	GSM_Error err;

	/* Copy in the phone info */
	memcpy(&(state->Phone), &phone_nokia_6100, sizeof(GSM_Phone));

	/* SMS Layout */
	nk6100_layout.Type = 7;
	nk6100_layout.Deliver = nk6100_deliver;
	nk6100_layout.Submit = nk6100_submit;
	nk6100_layout.DeliveryReport = nk6100_delivery_report;
	nk6100_layout.Picture = nk6100_picture;
	layout = nk6100_layout;

	switch (state->Link.ConnectionType) {
	case GCT_Serial:
		err = FBUS_Initialise(&(state->Link), state, 0);
		break;
	case GCT_Infrared:
		err = PHONET_Initialise(&(state->Link), state);
		break;
	default:
		return GE_NOTSUPPORTED;
	}

	if (err != GE_NONE) {
		dprintf("Error in link initialisation\n");
		return GE_NOTSUPPORTED;
	}
	
	SM_Initialise(state);

	/* Now test the link and get the model */
	GSM_DataClear(&data);
	data.Model = model;
	if (state->Phone.Functions(GOP_GetModel, &data, state) != GE_NONE) return GE_NOTSUPPORTED;
  	return GE_NONE;
}

static GSM_Error GetPhoneInfo(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x03, 0x00 };

	dprintf("Getting phone info...\n");
	if (SM_SendMessage(state, 5, 0xd1, req) != GE_NONE) return GE_NOTREADY;
	return (SM_Block(state, data, 0xd2));
}

static GSM_Error GetModelName(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Getting model...\n");
	return GetPhoneInfo(data, state);
}

static GSM_Error GetRevision(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Getting revision...\n");
	return GetPhoneInfo(data, state);
}

static GSM_Error GetIMEI(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x01};
  
	dprintf("Getting imei...\n");
	if (SM_SendMessage(state, 4, 0x1b, req)!=GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x1b);
}


static GSM_Error GetBatteryLevel(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x02};

	dprintf("Getting battery level...\n");
	if (SM_SendMessage(state, 4, 0x17, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x17);
}


static GSM_Error Incoming0x17(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	switch (message[3]) {
	case 0x03:
		if (data->BatteryLevel) { 
			*(data->BatteryUnits) = GBU_Percentage;
			*(data->BatteryLevel) = message[5];
			dprintf("Battery level %f\n",*(data->BatteryLevel));
		}
		return GE_NONE;
		break;
	default:
		dprintf("Unknown subtype of type 0x17 (%d)\n", message[3]);
		return GE_UNKNOWN;
		break;
	}
}

static GSM_Error GetRFLevel(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x81};

	dprintf("Getting rf level...\n");
	if (SM_SendMessage(state, 4, 0x0a, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x0a);
}

static GSM_Error Incoming0x0a(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	switch (message[3]) {
	case 0x82:
		if (data->RFLevel) { 
			*(data->RFUnits) = GRF_Percentage;
			*(data->RFLevel) = message[4];
			dprintf("RF level %f\n",*(data->RFLevel));
		}
		return GE_NONE;
		break;
	default:
		dprintf("Unknown subtype of type 0x0a (%d)\n", message[3]);
		return GE_UNKNOWN;
		break;
	}
}

static GSM_Error GetMemoryStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x03, 0x00, 0x00};
      
	dprintf("Getting memory status...\n");
	req[5] = GetMemoryType(data->MemoryStatus->MemoryType);
	if (SM_SendMessage(state, 6, 0x03, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static GSM_Error Incoming0x03(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	switch (message[3]) {
	case 0x04:
		if (data->MemoryStatus) {
			if (message[5] != 0xff) {
				data->MemoryStatus->Used = (message[16] << 8) + message[17];
				data->MemoryStatus->Free = ((message[14] << 8) + message[15]) - data->MemoryStatus->Used;
				dprintf("Memory status - location = %d\n", (message[8] << 8) + message[9]);
				return GE_NONE;
			} else {
				dprintf("Unknown error getting mem status\n");
				return GE_NOTIMPLEMENTED;
				
			}
		}
		return GE_NONE;
		break;
	default:
		dprintf("Unknown subtype of type 0x03 (%d)\n", message[3]);
		return GE_UNKNOWN;
		break;
	}
}

static GSM_Error Identify(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x10 };

	dprintf("Identifying...\n");
	PNOK_GetManufacturer(data->Manufacturer);
	if (SM_SendMessage(state, 4, 0x64, req) != GE_NONE) return GE_NOTREADY;
	SM_WaitFor(state, data, 0x64);
       	SM_Block(state, data, 0x64); /* waits for all requests */
	SM_GetError(state, 0x64);
	
	/* Check that we are back at state Initialised */
	if (SM_Loop(state, 0) != Initialised) return GE_UNKNOWN;
	return GE_NONE;
}

static GSM_Error IncomingPhoneInfo(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	if (data->Imei) { 
#if defined WIN32 || !defined HAVE_SNPRINTF
		sprintf(data->Imei, "%s", message + 4);
#else
		snprintf(data->Imei, GSM_MAX_IMEI_LENGTH, "%s", message + 4);
#endif
		dprintf("Received imei %s\n", data->Imei);
	}
	if (data->Model) { 
#if defined WIN32 || !defined HAVE_SNPRINTF
		sprintf(data->Model, "%s", message + 22);
#else
		snprintf(data->Model, GSM_MAX_MODEL_LENGTH, "%s", message + 22);
#endif
		dprintf("Received model %s\n", data->Model);
	}
	if (data->Revision) { 
#if defined WIN32 || !defined HAVE_SNPRINTF
		sprintf(data->Revision, "%s", message + 7);
#else
		snprintf(data->Revision, GSM_MAX_REVISION_LENGTH, "%s", message + 7);
#endif
		dprintf("Received revision %s\n", data->Revision);
	}

	dprintf("Message: Mobile phone identification received:\n");
	dprintf("\tIMEI: %s\n", data->Imei);
	dprintf("\tModel: %s\n", data->Model);
	dprintf("\tProduction Code: %s\n", message + 31);
	dprintf("\tHW: %s\n", message + 39);
	dprintf("\tFirmware: %s\n", message + 44);
	
	/* These bytes are probably the source of the "Accessory not connected"
	   messages on the phone when trying to emulate NCDS... I hope....
	   UPDATE: of course, now we have the authentication algorithm. */
	dprintf("\tMagic bytes: %02x %02x %02x %02x\n", message[50], message[51], message[52], message[53]);
	
	MagicBytes[0] = message[50];
	MagicBytes[1] = message[51];
	MagicBytes[2] = message[52];
	MagicBytes[3] = message[53];

	return GE_NONE;
}

static GSM_Error IncomingModelInfo(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
#if defined WIN32 || !defined HAVE_SNPRINTF
	if (data->Model) sprintf(data->Model, "%s", message + 21);
	if (data->Revision) sprintf(data->Revision, "SW%s", message + 5);
#else
	dprintf("%p %p %p\n", data, data->Model, data->Revision);
	if (data->Model) {
		snprintf(data->Model, GSM_MAX_MODEL_LENGTH, "%s", message + 21);
		data->Model[GSM_MAX_MODEL_LENGTH - 1] = 0;
	}
	if (data->Revision) {
		snprintf(data->Revision, GSM_MAX_REVISION_LENGTH, "SW%s", message + 5);
		data->Revision[GSM_MAX_REVISION_LENGTH - 1] = 0;
	}
#endif
	dprintf("Phone info:\n%s\n", message + 4);

	return GE_NONE;
}

static int GetMemoryType(GSM_MemoryType memory_type)
{
	int result;

	switch (memory_type) {
	case GMT_MT:
		result = P6100_MEMORY_MT;
		break;
	case GMT_ME:
		result = P6100_MEMORY_ME;
		break;
	case GMT_SM:
		result = P6100_MEMORY_SM;
		break;
	case GMT_FD:
		result = P6100_MEMORY_FD;
		break;
	case GMT_ON:
		result = P6100_MEMORY_ON;
		break;
	case GMT_EN:
		result = P6100_MEMORY_EN;
		break;
	case GMT_DC:
		result = P6100_MEMORY_DC;
		break;
	case GMT_RC:
		result = P6100_MEMORY_RC;
		break;
	case GMT_MC:
		result = P6100_MEMORY_MC;
		break;
	default:
		result = P6100_MEMORY_XX;
		break;
	}
	return (result);
}

static GSM_Error GetSMSMessage(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x07, 0x02 /* Unknown */, 0x00 /* Location */, 0x01, 0x64};

	req[5] = data->SMSMessage->Number;

	if (SM_SendMessage(state, 8, 0x02, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x14);
}

static GSM_Error IncomingSMS(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	int i;

	switch (message[3]) {
	/* save sms succeeded */
	case 0x05:
		dprintf("Message stored at %d\n", message[5]);
		break;
        /* save sms failed */
	case 0x06:
		dprintf("SMS saving failed:\n");
		switch (message[4]) {
		case 0x02:
			dprintf("\tAll locations busy.\n");
			return GE_MEMORYFULL;
		case 0x03:
			dprintf("\tInvalid location!\n");
			return GE_INVALIDSMSLOCATION;
		default:
			dprintf("\tUnknown reason.\n");
			return GE_UNKNOWN;
		}
        /* read sms */
        case 0x08:
                for (i = 0; i < length; i ++)
                        if (isprint(message[i]))
                                dprintf("[%02x%c]", message[i], message[i]);
                        else
                                dprintf("[%02x ]", message[i]);
                dprintf("\n");

		memset(data->SMSMessage, 0, sizeof(GSM_SMSMessage));

		/* Short Message status */
		DecodePDUSMS(message, data->SMSMessage, length);

                break;
	/* read sms failed */
	case 0x09:
		dprintf("SMS reading failed:\n");
		switch (message[4]) {
		case 0x02:
			dprintf("\tInvalid location!\n");
			return GE_INVALIDSMSLOCATION;
		case 0x07:
			dprintf("\tEmpty SMS location.\n");
			return GE_EMPTYSMSLOCATION;
		default:
			dprintf("\tUnknown reason.\n");
			return GE_UNKNOWN;
		}
	/* delete sms succeeded */
	case 0x0b:
		dprintf("Message: SMS deleted successfully.\n");
		break;
	/* sms status succeded */
	case 0x37:
		dprintf("Message: SMS Status Received\n");
		dprintf("\tThe number of messages: %d\n", message[10]);
		dprintf("\tUnread messages: %d\n", message[11]);
		data->SMSStatus->Unread = message[11];
		data->SMSStatus->Number = message[10];
		break;
	/* sms status failed */
	case 0x38:
		dprintf("Message: SMS Status error, probably not authorized by PIN\n");
		return GE_INTERNALERROR;
	/* unknown */
	default:
		dprintf("Unknown message.\n");
		return GE_UNKNOWN;
	}
	return GE_NONE;
}
