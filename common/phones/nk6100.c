/*

  $Id: nk6100.c,v 1.13 2002-01-10 10:53:16 pkot Exp $

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
	 4, true, true,
	-1,  7, -1, -1,  2, -1, -1, -1, 19, 18, -1, 16,
	20, true, true,
	32, -1,
	 1,  0,
	39, true
};

static const SMSMessage_Layout nk6100_submit = {
	true,
	 4, true, true,
	-1, 16, 16, 16,  2, 17, 18, -1, 20, 19, 16, 16,
	21, true, true,
	33, -1,
	-1, -1,
	40, true
};

static const SMSMessage_Layout nk6100_delivery_report = {
	true,
	 4, true, true,
	-1, -1, -1, -1,  2, -1, -1, -1, 18, 17, -1, 16,
	20, true, true,
	31, 38,
	 1,  0,
	18, true
};

static const SMSMessage_Layout nk6100_picture = {
	false,
	-1, true, true,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, true, true,
	-1, -1,
	-1, -1,
	-1, true
};

static SMSMessage_PhoneLayout nk6100_layout;

static unsigned char MagicBytes[4] = { 0x00, 0x00, 0x00, 0x00 };

/* static functions prototypes */
static GSM_Error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);
static GSM_Error Initialise(GSM_Statemachine *state);
static GSM_Error GetModelName(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetRevision(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetIMEI(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error Identify(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetBatteryLevel(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetRFLevel(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetMemoryStatus(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetSMSMessage(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetBitmap(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error ReadPhonebook(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error WritePhonebook(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetPowersource(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetSMSStatus(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetNetworkInfo(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error IncomingPhoneInfo(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error IncomingModelInfo(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error IncomingSMS(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error IncomingPhonebook(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error IncomingNetworkInfo(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error IncomingPhoneStatus(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error Incoming0x17(int messagetype, unsigned char *buffer, int length, GSM_Data *data);

static int GetMemoryType(GSM_MemoryType memory_type);

static GSM_IncomingFunctionType IncomingFunctions[] = {
	{ 0x04, IncomingPhoneStatus },
	{ 0x0a, IncomingNetworkInfo },
	{ 0x03, IncomingPhonebook },

	{ 0x64, IncomingPhoneInfo },
        { 0xd2, IncomingModelInfo },
	{ 0x14, IncomingSMS },
	{ 0x17, Incoming0x17 },
	{ 0, NULL}
};

GSM_Phone phone_nokia_6100 = {
	IncomingFunctions,
	PGEN_IncomingDefault,
        /* Mobile phone information */
	{
		"6110|6130|6150|6190|5110|5130|5190|3210|3310|3330|8210", /* Supported models */
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
	case GOP_GetBitmap:
		return GetBitmap(data, state);
	case GOP_GetBatteryLevel:
		return GetBatteryLevel(data, state);
	case GOP_GetRFLevel:
		return GetRFLevel(data, state);
	case GOP_GetMemoryStatus:
		return GetMemoryStatus(data, state);
	case GOP_ReadPhonebook:
		return ReadPhonebook(data, state);
	case GOP_WritePhonebook:
		return WritePhonebook(data, state);
	case GOP_GetPowersource:
		return GetPowersource(data, state);
	case GOP_GetSMSStatus:
		return GetSMSStatus(data, state);
	case GOP_GetNetworkInfo:
		return GetNetworkInfo(data, state);
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
	nk6100_layout.SendHeader = 0;
	nk6100_layout.ReadHeader = 4;
	nk6100_layout.Deliver = nk6100_deliver;
	nk6100_layout.Submit = nk6100_submit;
	nk6100_layout.DeliveryReport = nk6100_delivery_report;
	nk6100_layout.Picture = nk6100_picture;
	layout = nk6100_layout;

	switch (state->Link.ConnectionType) {
	case GCT_Serial:
	case GCT_Infrared:
		err = FBUS_Initialise(&(state->Link), state, 0);
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
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x03, 0x00};

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


static GSM_Error GetPhoneStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x01};

	dprintf("Getting phone status...\n");
	if (SM_SendMessage(state, 4, 0x04, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x04);
}

static GSM_Error GetRFLevel(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Getting rf level...\n");
	return GetPhoneStatus(data, state);
}

static GSM_Error GetBatteryLevel(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Getting battery level...\n");
	return GetPhoneStatus(data, state);
}

static GSM_Error GetPowersource(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Getting power source...\n");
	return GetPhoneStatus(data, state);
}

static GSM_Error IncomingPhoneStatus(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	switch (message[3]) {
	/* Phone status */
	case 0x02:
		dprintf("\tRFLevel: %d\n",message[5]);
		if (data->RFLevel) {
			*(data->RFUnits) = GRF_Arbitrary;
			*(data->RFLevel) = message[5];
		}
		dprintf("\tPowerSource: %d\n",message[7]);
		if (data->PowerSource) {
			*(data->PowerSource) = message[7];
		}
		dprintf("\tBatteryLevel: %d\n",message[8]);
		if (data->BatteryLevel) {
			*(data->BatteryUnits) = GBU_Arbitrary;
			*(data->BatteryLevel) = message[8];
		}
		break;
	default:
		dprintf("Unknown subtype of type 0x02 (%d)\n", message[3]);
		return GE_UNKNOWN;
	}

	return GE_NONE;
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


static GSM_Error GetMemoryStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07, 0x00};
      
	dprintf("Getting memory status...\n");
	req[4] = GetMemoryType(data->MemoryStatus->MemoryType);
	if (SM_SendMessage(state, 5, 0x03, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static GSM_Error ReadPhonebook(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x01, 0x00, 0x00, 0x00};
      
	dprintf("Reading phonebook location (%d/%d)\n", data->PhonebookEntry->MemoryType, data->PhonebookEntry->Location);
	req[4] = GetMemoryType(data->PhonebookEntry->MemoryType);
	req[5] = data->PhonebookEntry->Location;
	if (SM_SendMessage(state, 7, 0x03, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static GSM_Error WritePhonebook(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[FBUS_MAX_FRAME_LENGTH] = {FBUS_FRAME_HEADER, 0x04};
	GSM_PhonebookEntry *pe;
	unsigned char *pos;
	int namelen, numlen;
      
	pe = data->PhonebookEntry;
	pos = req+4;
	namelen = strlen(pe->Name);
	numlen = strlen(pe->Number);
	dprintf("Writing phonebook location (%d/%d): %s\n", pe->MemoryType, pe->Location, pe->Name);
	if (namelen > GSM_MAX_PHONEBOOK_NAME_LENGTH) {
		dprintf("name too long\n");
		return GE_PHBOOKNAMETOOLONG;
	}
	if (numlen > GSM_MAX_PHONEBOOK_NUMBER_LENGTH) {
		dprintf("number too long\n");
		return GE_PHBOOKNUMBERTOOLONG;
	}
	if (pe->SubEntriesCount > 1) {
		dprintf("61xx doesn't support subentries\n");
		return GE_UNKNOWN;
	}
	if ((pe->SubEntriesCount == 1) && ((pe->SubEntries[0].EntryType != GSM_Number)
		|| (pe->SubEntries[0].NumberType != GSM_General) || (pe->SubEntries[0].BlockNumber != 2)
		|| strcmp(pe->SubEntries[0].data.Number, pe->Number))) {
		dprintf("61xx doesn't support subentries\n");
		return GE_UNKNOWN;
	}
	*pos++ = GetMemoryType(pe->MemoryType);
	*pos++ = pe->Location;
	*pos++ = namelen;
	memcpy(pos,pe->Name,namelen);
	pos += namelen;
	*pos++ = numlen;
	memcpy(pos,pe->Number,numlen);
	pos += numlen;
	*pos++ = (pe->Group == 5) ? 0xff : pe->Group;
	if (SM_SendMessage(state, pos-req, 0x03, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static GSM_Error GetCallerGroupData(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x10, 0x00};
      
	req[4] = data->Bitmap->number;
	if (SM_SendMessage(state, 5, 0x03, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static GSM_Error GetBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Reading bitmap...\n");
	switch (data->Bitmap->type) {
	case GSM_CallerLogo:
		return GetCallerGroupData(data, state);
	default:
		return GE_NOTIMPLEMENTED;
	}
}

static GSM_Error IncomingPhonebook(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	GSM_PhonebookEntry *pe;
	GSM_Bitmap *bmp;
	unsigned char *pos;
	int n;

	switch (message[3]) {
	case 0x02:
		if (data->PhonebookEntry) {
			pe = data->PhonebookEntry;
			pos = message+5;
			pe->Empty = false;
			n = *pos++;
			snprintf(pe->Name, sizeof(pe->Name), "%*.*s", n, n, pos);
			pos += n;
			n = *pos++;
			snprintf(pe->Number, sizeof(pe->Number), "%*.*s", n, n, pos);
			pos += n;
			pe->Group = *pos++;
			pos++;
			pe->Date.Year = (pos[0] << 8) + pos[1];
			pos += 2;
			pe->Date.Month = *pos++;
			pe->Date.Day = *pos++;
			pe->Date.Hour = *pos++;
			pe->Date.Minute = *pos++;
			pe->Date.Second = *pos++;
			pe->SubEntriesCount = 0;
		}
		break;
	case 0x03:
		if ((message[4] == 0x7d) || (message[4] == 0x74)) {
			return GE_INVALIDPHBOOKLOCATION;
		}
		dprintf("Invalid GetMemoryLocation reply: 0x%02x\n",message[4]);
		return GE_UNKNOWN;
	case 0x05:
		break;
	case 0x06:
		switch (message[4]) {
		case 0x7d:
		case 0x90:
			return GE_PHBOOKNAMETOOLONG;
		case 0x74:
			return GE_INVALIDPHBOOKLOCATION;
		}
		printf("Invalid GetMemoryLocation reply: 0x%02x\n",message[4]);
		dprintf("Invalid GetMemoryLocation reply: 0x%02x\n",message[4]);
		return GE_UNKNOWN;
	case 0x08:
		dprintf("\tMemory location: %d\n",data->MemoryStatus->MemoryType);
		dprintf("\tUsed: %d\n",message[6]);
		dprintf("\tFree: %d\n",message[5]);
		if (data->MemoryStatus) {
			data->MemoryStatus->Used = message[6];
			data->MemoryStatus->Free = message[5];
			return GE_NONE;
		}
		break;
	case 0x09:
		switch (message[4]) {
		case 0x6f:
			return GE_TIMEOUT;
		case 0x7d:
			return GE_INTERNALERROR;
		case 0x8d:
			return GE_INVALIDSECURITYCODE;
		}
		dprintf("Invalid GetMemoryStatus reply: 0x%02x\n",message[4]);
		return GE_UNKNOWN;
	case 0x11:
		if (data->Bitmap) {
			bmp = data->Bitmap;
			pos = message+4;
			bmp->number = *pos++;
			n = *pos++;
			snprintf(bmp->text, sizeof(bmp->text), "%*.*s", n, n, pos);
			pos += n;
			bmp->ringtone = *pos++;
			pos++;
			bmp->size = (pos[0] << 8) + pos[1];
			pos += 2;
			pos++;
			bmp->width = *pos++;
			bmp->height = *pos++;
			pos++;
			n = bmp->height * bmp->width / 8;
			if (bmp->size > n) bmp->size = n;
			memcpy(bmp->bitmap,pos,bmp->size);
		}
		break;
	case 0x12:
		dprintf("Invalid GetCallerGroupData reply: 0x%02x\n",message[4]);
		return GE_UNKNOWN;
	default:
		dprintf("Unknown subtype of type 0x03 (%d)\n", message[3]);
		return GE_UNKNOWN;
	}

	return GE_NONE;
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
		snprintf(data->Imei, GSM_MAX_IMEI_LENGTH, "%s", message + 4);
		dprintf("Received imei %s\n", data->Imei);
	}
	if (data->Model) { 
		snprintf(data->Model, GSM_MAX_MODEL_LENGTH, "%s", message + 22);
		dprintf("Received model %s\n", data->Model);
	}
	if (data->Revision) { 
		snprintf(data->Revision, GSM_MAX_REVISION_LENGTH, "%s", message + 7);
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
	dprintf("%p %p %p\n", data, data->Model, data->Revision);
	if (data->Model) {
		snprintf(data->Model, GSM_MAX_MODEL_LENGTH, "%s", message + 21);
		data->Model[GSM_MAX_MODEL_LENGTH - 1] = 0;
	}
	if (data->Revision) {
		snprintf(data->Revision, GSM_MAX_REVISION_LENGTH, "SW%s", message + 5);
		data->Revision[GSM_MAX_REVISION_LENGTH - 1] = 0;
	}
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

static GSM_Error GetSMSStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x36, 0x64 };

	if (SM_SendMessage(state, 5, 0x14, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x14);
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
		for (i = 0; i < length; i++)
			dprintf("[%02x(%d)]", message[i], i);
		dprintf("\n");

		memset(data->SMSMessage, 0, sizeof(GSM_SMSMessage));

		/* Short Message status */
		data->SMSMessage->Status = message[4];
		dprintf("\tStatus: ");
		switch (data->SMSMessage->Status) {
		case SMS_Read:
			dprintf("READ\n");
			break;
		case SMS_Unread:
			dprintf("UNREAD\n");
			break;
		case SMS_Sent:
			dprintf("SENT\n");
			break;
		case SMS_Unsent:
			dprintf("UNSENT\n");
			break;
		default:
			dprintf("UNKNOWN\n");
			break;
		}

		/* Short Message status */
		if (!data->RawData) {
			data->RawData = (GSM_RawData *)malloc(sizeof(GSM_RawData));
		}

		/* Skip the frame header */
//		data->RawData->Data = message + nk6100_layout.ReadHeader;
		data->RawData->Length = length - nk6100_layout.ReadHeader;
		data->RawData->Data = malloc(data->RawData->Length);
		memcpy(data->RawData->Data, message + nk6100_layout.ReadHeader, data->RawData->Length);
		dprintf("Everything set. Length: %d\n", data->RawData->Length);

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


static GSM_Error GetNetworkInfo(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x70 };

	if (SM_SendMessage(state, 4, 0x0a, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x0a);
}

static GSM_Error IncomingNetworkInfo(int messagetype, unsigned char *message, int length, GSM_Data *data)
{
	switch (message[3]) {
	/* Network info */
	case 0x71:
		if (data->NetworkInfo) {
			data->NetworkInfo->CellID[0] = message[10];
			data->NetworkInfo->CellID[1] = message[11];
			data->NetworkInfo->LAC[0] = message[12];
			data->NetworkInfo->LAC[1] = message[13];
			data->NetworkInfo->NetworkCode[0] = '0' + (message[14] & 0x0f);
			data->NetworkInfo->NetworkCode[1] = '0' + (message[14] >> 4);
			data->NetworkInfo->NetworkCode[2] = '0' + (message[15] & 0x0f);
			data->NetworkInfo->NetworkCode[3] = ' ';
			data->NetworkInfo->NetworkCode[4] = '0' + (message[16] & 0x0f);
			data->NetworkInfo->NetworkCode[5] = '0' + (message[16] >> 4);
			data->NetworkInfo->NetworkCode[6] = 0;
		}
		break;
	default:
		dprintf("Unknown subtype of type 0x0a (%d)\n", message[3]);
		return GE_UNKNOWN;
	}

	return GE_NONE;
}
