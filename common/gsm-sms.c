/*

  $Id: gsm-sms.c,v 1.127 2003-01-19 22:26:47 pkot Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

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

  Copyright (C) 2001-2002 Pawe� Kot <pkot@linuxnews.pl>
  Copyright (C) 2001-2002 Pavel Machek <pavel@ucw.cz>

  Library for parsing and creating Short Messages (SMS).

*/

#include <stdlib.h>
#include <string.h>

#include "gsm-data.h"
#include "gsm-encoding.h"
#include "gsm-statemachine.h"

#include "gsm-ringtones.h"
#include "gsm-bitmaps.h"

#include "sms-nokia.h"

#include "gnokii-internal.h"
#include "gsm-api.h"

#define ERROR() do { if (error != GN_ERR_NONE) return error; } while (0)

struct sms_udh_data {
	unsigned int length;
	char *header;
};

#define MAX_SMS_PART 128

/* These are positions in the following (headers[]) table. */
#define SMS_UDH_CONCATENATED_MESSAGES  1
#define SMS_UDH_RINGTONES              2
#define SMS_UDH_OPERATOR_LOGOS         3
#define SMS_UDH_CALLER_LOGOS           4
#define SMS_UDH_MULTIPART_MESSAGES     5
#define SMS_UDH_WAP_VCARD              6
#define SMS_UDH_WAP_CALENDAR           7
#define SMS_UDH_WAP_VCARDSECURE        8
#define SMS_UDH_WAP_VCALENDARSECURE    9
#define SMS_UDH_VOICE_MESSAGES        10
#define SMS_UDH_FAX_MESSAGES          11
#define SMS_UDH_EMAIL_MESSAGES        12
#define SMS_UDH_WAP_PUSH              13

/* User data headers */
static struct sms_udh_data headers[] = {
	{ 0x00, "" },
	{ 0x05, "\x00\x03\x01\x00\x00" },     /* Concatenated messages */
	{ 0x06, "\x05\x04\x15\x81\x00\x00" }, /* Ringtones */
	{ 0x06, "\x05\x04\x15\x82\x00\x00" }, /* Operator logos */
	{ 0x06, "\x05\x04\x15\x83\x00\x00" }, /* Caller logos */
	{ 0x06, "\x05\x04\x15\x8a\x00\x00" }, /* Multipart Message */
	{ 0x06, "\x05\x04\x23\xf4\x00\x00" }, /* WAP vCard */
	{ 0x06, "\x05\x04\x23\xf5\x00\x00" }, /* WAP vCalendar */
	{ 0x06, "\x05\x04\x23\xf6\x00\x00" }, /* WAP vCardSecure */
	{ 0x06, "\x05\x04\x23\xf7\x00\x00" }, /* WAP vCalendarSecure */
	{ 0x04, "\x01\x02\x00\x00" },         /* Voice Messages */
	{ 0x04, "\x01\x02\x01\x00" },         /* Fax Messages */
	{ 0x04, "\x01\x02\x02\x00" },         /* Email Messages */
	{ 0x06, "\x05\x04\x0b\x84\x23\xf0" }, /* WAP PUSH */
	{ 0x00, "" },
};


/**
 * sms_default - fills in SMS structure with the default values
 * @sms: pointer to a structure we need to fill in
 *
 * Default settings:
 *  - no delivery report
 *  - no Class Message
 *  - no compression
 *  - 7 bit data
 *  - message validity for 3 days
 *  - unsent status
 */
static void sms_default(gn_sms *sms)
{
	memset(sms, 0, sizeof(gn_sms));

	sms->type = GN_SMS_MT_Deliver;
	sms->delivery_report = false;
	sms->status = GN_SMS_Unsent;
	sms->validity = 4320; /* 4320 minutes == 72 hours */
	sms->dcs.type = GN_SMS_DCS_GeneralDataCoding;
	sms->dcs.u.general.compressed = false;
	sms->dcs.u.general.alphabet = GN_SMS_DCS_DefaultAlphabet;
	sms->dcs.u.general.class = 0;
}

API void gn_sms_default_submit(gn_sms *sms)
{
	sms_default(sms);
	sms->type = GN_SMS_MT_Submit;
	sms->memory_type = GN_MT_SM;
}

API void gn_sms_default_deliver(gn_sms *sms)
{
	sms_default(sms);
	sms->type = GN_SMS_MT_Deliver;
	sms->memory_type = GN_MT_ME;
}


/***
 *** Util functions
 ***/

/**
 * sms_timestamp_print - formats date and time in the human readable manner
 * @number: binary coded date and time
 *
 * The function converts binary date time into a easily readable form.
 * It is Y2K compliant.
 */
static char *sms_timestamp_print(u8 *number)
{
#ifdef DEBUG
#define LOCAL_DATETIME_MAX_LENGTH 23
	static unsigned char buffer[LOCAL_DATETIME_MAX_LENGTH] = "";

	if (!number) return NULL;

	memset(buffer, 0, LOCAL_DATETIME_MAX_LENGTH);

	/* Ugly hack, but according to the GSM specs, the year is stored
         * as the 2 digit number. */
	if (number[0] < 70) sprintf(buffer, "20");
	else sprintf(buffer, "19");

	sprintf(buffer, "%s%d%d-", buffer, number[0] & 0x0f, number[0] >> 4);
	sprintf(buffer, "%s%d%d-", buffer, number[1] & 0x0f, number[1] >> 4);
	sprintf(buffer, "%s%d%d ", buffer, number[2] & 0x0f, number[2] >> 4);
	sprintf(buffer, "%s%d%d:", buffer, number[3] & 0x0f, number[3] >> 4);
	sprintf(buffer, "%s%d%d:", buffer, number[4] & 0x0f, number[4] >> 4);
	sprintf(buffer, "%s%d%d",  buffer, number[5] & 0x0f, number[5] >> 4);

	/* The GSM spec is not clear what is the sign of the timezone when the
	 * 6th bit is set. Some SMSCs are compatible with our interpretation,
	 * some are not. If your operator does use incompatible SMSC and wrong
	 * sign disturbs you, change the sign here.
	 */
	if (number[6] & 0x08) sprintf(buffer, "%s-", buffer);
	else sprintf(buffer, "%s+", buffer);
	/* The timezone is given in quarters. The base is GMT. */
	sprintf(buffer, "%s%02d00", buffer, (10 * (number[6] & 0x07) + (number[6] >> 4)) / 4);

	return buffer;
#undef LOCAL_DATETIME_MAX_LENGTH
#else
	return NULL;
#endif /* DEBUG */
}

/**
 * sms_timestamp_unpack - converts binary datetime to the gnokii's datetime struct
 * @number: binary coded date and time
 * @dt: datetime structure to be filled
 *
 * The function fills int gnokii datetime structure basing on the given datetime
 * in the binary format. It is Y2K compliant.
 */
static gn_timestamp *sms_timestamp_unpack(u8 *number, gn_timestamp *dt)
{
	if (!dt) return NULL;
	memset(dt, 0, sizeof(gn_timestamp));
	if (!number) return dt;

	dt->year     =  10 * (number[0] & 0x0f) + (number[0] >> 4);

	/* Ugly hack, but according to the GSM specs, the year is stored
         * as the 2 digit number. */
	if (dt->year < 70) dt->year += 2000;
	else dt->year += 1900;

	dt->month    =  10 * (number[1] & 0x0f) + (number[1] >> 4);
	dt->day      =  10 * (number[2] & 0x0f) + (number[2] >> 4);
	dt->hour     =  10 * (number[3] & 0x0f) + (number[3] >> 4);
	dt->minute   =  10 * (number[4] & 0x0f) + (number[4] >> 4);
	dt->second   =  10 * (number[5] & 0x0f) + (number[5] >> 4);

	/* The timezone is given in quarters. The base is GMT */
	dt->timezone = (10 * (number[6] & 0x07) + (number[6] >> 4)) / 4;
	/* The GSM spec is not clear what is the sign of the timezone when the
	 * 6th bit is set. Some SMSCs are compatible with our interpretation,
	 * some are not. If your operator does use incompatible SMSC and wrong
	 * sign disturbs you, change the sign here.
	 */
	if (number[6] & 0x08) dt->timezone = -dt->timezone;

	return dt;
}

/**
 * sms_timestamp_pack - converts gnokii's datetime struct to the binary datetime
 * @number: binary variable to be filled
 * @dt: datetime structure to be read
 *
 */
static u8 *sms_timestamp_pack(gn_timestamp *dt, u8 *number)
{
	if (!number) return NULL;
	memset(number, 0, sizeof(unsigned char));
	if (!dt) return number;

	/* Ugly hack, but according to the GSM specs, the year is stored
         * as the 2 digit number. */
	if (dt->year < 2000) dt->year -= 1900;
	else dt->year -= 2000;

	number[0]    = (dt->year   / 10) | ((dt->year   % 10) << 4);
	number[1]    = (dt->month  / 10) | ((dt->month  % 10) << 4);
	number[2]    = (dt->day    / 10) | ((dt->day    % 10) << 4);
	number[3]    = (dt->hour   / 10) | ((dt->hour   % 10) << 4);
	number[4]    = (dt->minute / 10) | ((dt->minute % 10) << 4);
	number[5]    = (dt->second / 10) | ((dt->second % 10) << 4);

	/* The timezone is given in quarters. The base is GMT */
	number[6]    = (dt->timezone / 10) | ((dt->second % 10) << 4) * 4;
	/* The GSM spec is not clear what is the sign of the timezone when the
	 * 6th bit is set. Some SMSCs are compatible with our interpretation,
	 * some are not. If your operator does use incompatible SMSC and wrong
	 * sign disturbs you, change the sign here.
	 */
	if (dt->timezone < 0) number[6] |= 0x08;

	return number;
}

/***
 *** DECODING SMS
 ***/

/**
 * sms_status - decodes the error code contained in the delivery report.
 * @status: error code to decode
 * @sms: SMS struct to store the result
 *
 * This function checks for the error (or success) code contained in the
 * received SMS - delivery report and fills in the SMS structure with
 * the correct message according to the GSM specification.
 * This function only applies to the delivery reports. Delivery reports
 * contain only one part, so it is assumed all other part except 0th are
 * NULL.
 */
static gn_error sms_status(unsigned char status, gn_sms *sms)
{
	sms->user_data[0].type = GN_SMS_DATA_Text;
	sms->user_data[1].type = GN_SMS_DATA_None;
	if (status < 0x03) {
		strcpy(sms->user_data[0].u.text, _("Delivered"));
		switch (status) {
		case 0x00:
			dprintf("SM received by the SME");
			break;
		case 0x01:
			dprintf("SM forwarded by the SC to the SME but the SC is unable to confirm delivery");
			break;
		case 0x02:
			dprintf("SM replaced by the SC");
			break;
		}
	} else if (status & 0x40) {
		strcpy(sms->user_data[0].u.text, _("Failed"));
		/* more detailed reason only for debug */
		if (status & 0x20) {
			dprintf("Temporary error, SC is not making any more transfer attempts\n");

			switch (status) {
			case 0x60:
				dprintf("Congestion");
				break;
			case 0x61:
				dprintf("SME busy");
				break;
			case 0x62:
				dprintf("No response from SME");
				break;
			case 0x63:
				dprintf("Service rejected");
				break;
			case 0x64:
				dprintf("Quality of service not aviable");
				break;
			case 0x65:
				dprintf("Error in SME");
				break;
			default:
				dprintf("Reserved/Specific to SC: %x", status);
				break;
			}
		} else {
			dprintf("Permanent error, SC is not making any more transfer attempts\n");
			switch (status) {
			case 0x40:
				dprintf("Remote procedure error");
				break;
			case 0x41:
				dprintf("Incompatibile destination");
				break;
			case 0x42:
				dprintf("Connection rejected by SME");
				break;
			case 0x43:
				dprintf("Not obtainable");
				break;
			case 0x44:
				dprintf("Quality of service not aviable");
				break;
			case 0x45:
				dprintf("No internetworking available");
				break;
			case 0x46:
				dprintf("SM Validity Period Expired");
				break;
			case 0x47:
				dprintf("SM deleted by originating SME");
				break;
			case 0x48:
				dprintf("SM Deleted by SC Administration");
				break;
			case 0x49:
				dprintf("SM does not exist");
				break;
			default:
				dprintf("Reserved/Specific to SC: %x", status);
				break;
			}
		}
	} else if (status & 0x20) {
		strcpy(sms->user_data[0].u.text, _("Pending"));
		/* more detailed reason only for debug */
		dprintf("Temporary error, SC still trying to transfer SM\n");
		switch (status) {
		case 0x20:
			dprintf("Congestion");
			break;
		case 0x21:
			dprintf("SME busy");
			break;
		case 0x22:
			dprintf("No response from SME");
			break;
		case 0x23:
			dprintf("Service rejected");
			break;
		case 0x24:
			dprintf("Quality of service not aviable");
			break;
		case 0x25:
			dprintf("Error in SME");
			break;
		default:
			dprintf("Reserved/Specific to SC: %x", status);
			break;
		}
	} else {
		strcpy(sms->user_data[0].u.text, _("Unknown"));

		/* more detailed reason only for debug */
		dprintf("Reserved/Specific to SC: %x", status);
	}
	dprintf("\n");
	sms->user_data[0].length = strlen(sms->user_data[0].u.text);
	return GN_ERR_NONE;
}

/**
 * sms_data_decode - decodes encoded text from the SMS.
 * @message: encoded text
 * @output: room for the encoded text
 * @length: decoded text length
 * @size: encoded text length
 * @udhlen: udh length 
 * @dcs: data coding scheme
 *
 * This function decodes either 7bit or 8bit os Unicode text to the
 * readable text format according to the locale set.
 */
static gn_error sms_data_decode(unsigned char *message, unsigned char *output, unsigned int length,
				 unsigned int size, unsigned int udhlen, gn_sms_dcs dcs)
{
	/* Unicode */
	if ((dcs.type & 0x08) == 0x08) {
		dprintf("Unicode message\n");
		char_unicode_decode(output, message, length);
	} else {
		/* 8bit SMS */
		if ((dcs.type & 0xf4) == 0xf4) {
			dprintf("8bit message\n");
			memcpy(output, message + udhlen, length);
		/* 7bit SMS */
		} else {
			dprintf("Default Alphabet\n");
			length = length - (udhlen * 8 + ((7-(udhlen%7))%7)) / 7;
			char_7bit_unpack((7-udhlen)%7, size, length, message, output);
			char_ascii_decode(output, output, length);
		}
	}
	dprintf("%s\n", output);
	return GN_ERR_NONE;
}

/**
 * sms_udh_decode - interprete the User Data Header contents
 * @message: received UDH
 * @sms: SMS structure to fill in
 *
 * At this stage we already need to know thet SMS has UDH.
 * This function decodes UDH as described in:
 *   - GSM 03.40 version 6.1.0 Release 1997, section 9.2.3.24
 *   - Smart Messaging Specification, Revision 1.0.0, September 15, 1997
 */
static gn_error sms_udh_decode(unsigned char *message, gn_sms_udh *udh)
{
	unsigned char length, pos, nr;

	udh->length = length = message[0];
	pos = 1;
	nr = 0;
	while (length > 1) {
		unsigned char udh_length;

		udh_length = message[pos+1];
		switch (message[pos]) {
		case 0x00: /* Concatenated short messages */
			dprintf("Concatenated messages\n");
			udh->udh[nr].type = GN_SMS_UDH_ConcatenatedMessages;
			udh->udh[nr].u.concatenated_short_message.reference_number = message[pos + 2];
			udh->udh[nr].u.concatenated_short_message.maximum_number   = message[pos + 3];
			udh->udh[nr].u.concatenated_short_message.current_number   = message[pos + 4];
			break;
		case 0x01: /* Special SMS Message Indication */
			switch (message[pos + 2] & 0x03) {
			case 0x00:
				dprintf("Voice Message\n");
				udh->udh[nr].type = GN_SMS_UDH_VoiceMessage;
				break;
			case 0x01:
				dprintf("Fax Message\n");
				udh->udh[nr].type = GN_SMS_UDH_FaxMessage;
				break;
			case 0x02:
				dprintf("Email Message\n");
				udh->udh[nr].type = GN_SMS_UDH_EmailMessage;
				break;
			default:
				dprintf("Unknown\n");
				udh->udh[nr].type = GN_SMS_UDH_Unknown;
				break;
			}
			udh->udh[nr].u.special_sms_message_indication.store = (message[pos + 2] & 0x80) >> 7;
			udh->udh[nr].u.special_sms_message_indication.message_count = message[pos + 3];
			break;
		case 0x05: /* Application port addression scheme, 16 bit address */
			switch (((0x00ff & message[pos + 2]) << 8) | (0x00ff & message[pos + 3])) {
			case 0x1581:
				dprintf("Ringtone\n");
				udh->udh[nr].type = GN_SMS_UDH_Ringtone;
				break;
			case 0x1582:
				dprintf("Operator Logo\n");
				udh->udh[nr].type = GN_SMS_UDH_OpLogo;
				break;
			case 0x1583:
				dprintf("Caller Icon\n");
				udh->udh[nr].type = GN_SMS_UDH_CallerIDLogo;
				break;
			case 0x158a:
				dprintf("Multipart Message\n");
				udh->udh[nr].type = GN_SMS_UDH_MultipartMessage;
				break;
			case 0x23f4:
				dprintf("WAP vCard\n");
				udh->udh[nr].type = GN_SMS_UDH_WAPvCard;
				break;
			case 0x23f5:
				dprintf("WAP vCalendar\n");
				udh->udh[nr].type = GN_SMS_UDH_WAPvCalendar;
				break;
			case 0x23f6:
				dprintf("WAP vCardSecure\n");
				udh->udh[nr].type = GN_SMS_UDH_WAPvCardSecure;
				break;
			case 0x23f7:
				dprintf("WAP vCalendarSecure\n");
				udh->udh[nr].type = GN_SMS_UDH_WAPvCalendarSecure;
				break;
			default:
				dprintf("Unknown\n");
				udh->udh[nr].type = GN_SMS_UDH_Unknown;
				break;
			}
			break;
		case 0x04: /* Application port addression scheme, 8 bit address */
		case 0x06: /* SMSC Control Parameters */
		case 0x07: /* UDH Source Indicator */
		default:
			udh->udh[nr].type = GN_SMS_UDH_Unknown;
			dprintf("Not supported UDH\n");
			break;
		}
		length -= (udh_length + 2);
		pos += (udh_length + 2);
		nr++;
	}
	udh->number = nr;

	return GN_ERR_NONE;
}

/**
 * sms_header_decode - Doecodes PDU SMS header
 * @rawsms:
 * @sms:
 * @udh:
 *
 * This function parses received SMS header information and stores
 * them in hihger level SMS struct. It also checks for the UDH and when
 * it's found calls the function to extract the UDH.
 */
static gn_error sms_header_decode(gn_sms_raw *rawsms, gn_sms *sms, gn_sms_udh *udh)
{
	switch (sms->type = rawsms->type) {
	case GN_SMS_MT_Deliver:
		dprintf("Mobile Terminated message:\n");
		break;
	case GN_SMS_MT_DeliveryReport:
		dprintf("Delivery Report:\n");
		break;
	case GN_SMS_MT_Submit:
		dprintf("Mobile Originated (stored) message:\n");
		break;
	case GN_SMS_MT_SubmitSent:
		dprintf("Mobile Originated (sent) message:\n");
		break;
	case GN_SMS_MT_Picture:
		dprintf("Picture Message:\n");
		break;
	case GN_SMS_MT_PictureTemplate:
		dprintf("Picture Template:\n");
		break;
	case GN_SMS_MT_TextTemplate:
		dprintf("Text Template:\n");
		break;
	default:
		dprintf("Not supported message type: %d\n", sms->type);
		return GN_ERR_NOTSUPPORTED;
	}

	/* Sending date */
	sms_timestamp_unpack(rawsms->smsc_time, &(sms->smsc_time));
	dprintf("\tDate: %s\n", sms_timestamp_print(rawsms->smsc_time));

	/* Remote number */
	rawsms->remote_number[0] = (rawsms->remote_number[0] + 1) / 2 + 1;
	snprintf(sms->remote.number, sizeof(sms->remote.number), "%s", char_bcd_number_get(rawsms->remote_number));
	dprintf("\tRemote number (recipient or sender): %s\n", sms->remote.number);

	/* Short Message Center */
	snprintf(sms->smsc.number, sizeof(sms->smsc.number), "%s", char_bcd_number_get(rawsms->message_center));
	dprintf("\tSMS center number: %s\n", sms->smsc.number);

	/* Delivery time */
	if (sms->type == GN_SMS_MT_DeliveryReport) {
		sms_timestamp_unpack(rawsms->time, &(sms->time));
		dprintf("\tDelivery date: %s\n", sms_timestamp_print(rawsms->time));
	}

	/* Data Coding Scheme */
	sms->dcs.type = rawsms->dcs;

	/* User Data Header */
	if (rawsms->udh_indicator & 0x40) { /* UDH header available */
		dprintf("UDH found\n");
		sms_udh_decode(rawsms->user_data, udh);
	}

	return GN_ERR_NONE;
}

/**
 * sms_pdu_decode - This function decodes the PDU SMS
 * @rawsms - SMS read by the phone driver
 * @sms - place to store the decoded message
 *
 * This function decodes SMS as described in GSM 03.40 version 6.1.0
 * Release 1997, section 9
 */
static gn_error sms_pdu_decode(gn_sms_raw *rawsms, gn_sms *sms)
{
	unsigned int size = 0;
	gn_error error;

	error = sms_header_decode(rawsms, sms, &sms->udh);
	ERROR();
	switch (sms->type) {
	case GN_SMS_MT_DeliveryReport:
		sms_status(rawsms->report_status, sms);
		break;
	case GN_SMS_MT_PictureTemplate:
	case GN_SMS_MT_Picture:
		/* This is incredible. Nokia violates it's own format in 6210 */
		/* Indicate that it is Multipart Message. Remove it if not needed */
		/* [I believe Nokia said in their manuals that any order is permitted --pavel] */
		sms->udh.number = 1;
		sms->udh.udh[0].type = GN_SMS_UDH_MultipartMessage;
		if ((rawsms->user_data[0] == 0x48) && (rawsms->user_data[1] == 0x1c)) {

			dprintf("First picture then text!\n");

			/* First part is a Picture */
			sms->user_data[0].type = GN_SMS_DATA_Bitmap;
			gn_bmp_sms_read(GN_BMP_PictureMessage, rawsms->user_data,
					NULL, &sms->user_data[0].u.bitmap);
			gn_bmp_print(&sms->user_data[0].u.bitmap, stderr);

			size = rawsms->user_data_length - 4 - sms->user_data[0].u.bitmap.size;
			/* Second part is a text */
			sms->user_data[1].type = GN_SMS_DATA_NokiaText;
			sms_data_decode(rawsms->user_data + 5 + sms->user_data[0].u.bitmap.size,
					(unsigned char *)&(sms->user_data[1].u.text),
					rawsms->length - sms->user_data[0].u.bitmap.size - 4,
					size, 0, sms->dcs);
		} else {

			dprintf("First text then picture!\n");

			/* First part is a text */
			sms->user_data[1].type = GN_SMS_DATA_NokiaText;
			sms_data_decode(rawsms->user_data + 3,
					(unsigned char *)&(sms->user_data[1].u.text),
					rawsms->user_data[1], rawsms->user_data[0], 0, sms->dcs);

			/* Second part is a Picture */
			sms->user_data[0].type = GN_SMS_DATA_Bitmap;
			gn_bmp_sms_read(GN_BMP_PictureMessage,
					rawsms->user_data + rawsms->user_data[0] + 7,
					NULL, &sms->user_data[0].u.bitmap);
			gn_bmp_print(&sms->user_data[0].u.bitmap, stderr);
		}
		break;
	/* Plain text message */
	default:
		size = rawsms->length - sms->udh.length;
		sms_data_decode(rawsms->user_data + sms->udh.length,        /* Skip the UDH */
				(unsigned char *)&sms->user_data[0].u.text, /* With a plain text message we have only 1 part */
				rawsms->length,                            /* Length of the decoded text */
				size,                                      /* Length of the encoded text (in full octets) without UDH */
				sms->udh.length,                          /* To skip the certain number of bits when unpacking 7bit message */
				sms->dcs);
		break;
	}

	return GN_ERR_NONE;
}

/**
 * gn_sms_parse - High-level function for the SMS parsing
 * @data: GSM data from the phone driver
 *
 * This function parses the SMS message from the lowlevel raw_sms to
 * the highlevel SMS. In data->raw_sms there's SMS read by the phone
 * driver, data->sms is the place for the parsed SMS.
 */
API gn_error gn_sms_parse(gn_data *data)
{
	if (!data->raw_sms || !data->sms) return GN_ERR_INTERNALERROR;
	/* Let's assume at the moment that all messages are PDU coded */
	return sms_pdu_decode(data->raw_sms, data->sms);
}

/**
 * gn_sms_request - High-level function for the expicit SMS reading from the phone
 * @data: GSM data for the phone driver
 * @state: current statemachine state
 *
 * This is the function for explicit requesting the SMS from the
 * phone driver. Not that raw_sms field in the gn_data structure must
 * be initialized
 */
API gn_error gn_sms_request(gn_data *data, struct gn_statemachine *state)
{
	if (!data->raw_sms) return GN_ERR_INTERNALERROR;
	return gn_sm_functions(GN_OP_GetSMS, data, state);
}

/**
 * gn_sms_get- High-level function for reading SMS
 * @data: GSM data for the phone driver
 * @state: current statemachine state
 *
 * This function is the frotnend for reading SMS. Note that SMS field
 * in the gn_data structure must be initialized.
 */
API gn_error gn_sms_get(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;
	gn_sms_raw rawsms;

	if (!data->sms) return GN_ERR_INTERNALERROR;
	memset(&rawsms, 0, sizeof(gn_sms_raw));
	rawsms.number = data->sms->number;
	rawsms.memory_type = data->sms->memory_type;
	data->raw_sms = &rawsms;
	error = gn_sms_request(data, state);
	ERROR();
	data->sms->status = rawsms.status;
	return gn_sms_parse(data);
}

/**
 * gn_sms_delete - High-level function for delting SMS
 * @data: GSM data for the phone driver
 * @state: current statemachine state
 *
 * This function is the frontend for deleting SMS. Note that SMS field
 * in the gn_data structure must be initialized.
 */
API gn_error gn_sms_delete(gn_data *data, struct gn_statemachine *state)
{
	gn_sms_raw rawsms;

	if (!data->sms) return GN_ERR_INTERNALERROR;
	memset(&rawsms, 0, sizeof(gn_sms_raw));
	rawsms.number = data->sms->number;
	rawsms.memory_type = data->sms->memory_type;
	data->raw_sms = &rawsms;
	return gn_sm_functions(GN_OP_DeleteSMS, data, state);
}

/***
 *** OTHER FUNCTIONS
 ***/

static gn_error sms_request_no_validate(gn_data *data, struct gn_statemachine *state)
{
	return gn_sm_functions(GN_OP_GetSMSnoValidate, data, state);
}

API gn_error gn_sms_get_no_validate(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;
	gn_sms_raw rawsms;

	if (!data->sms) return GN_ERR_INTERNALERROR;
	memset(&rawsms, 0, sizeof(gn_sms_raw));
	rawsms.number = data->sms->number;
	rawsms.memory_type = data->sms->memory_type;
	data->raw_sms = &rawsms;
	error = sms_request_no_validate(data, state);
	ERROR();
	data->sms->status = rawsms.status;
	return gn_sms_parse(data);
}

API gn_error gn_sms_delete_no_validate(gn_data *data, struct gn_statemachine *state)
{
	gn_sms_raw rawsms;

	if (!data->sms) return GN_ERR_INTERNALERROR;
	memset(&rawsms, 0, sizeof(gn_sms_raw));
	rawsms.number = data->sms->number;
	rawsms.memory_type = data->sms->memory_type;
	data->raw_sms = &rawsms;
	return gn_sm_functions(GN_OP_DeleteSMSnoValidate, data, state);
}

static gn_error sms_free_deleted(gn_data *data, int folder)
{
	int i, j;

	if (!data->sms_status) return GN_ERR_INTERNALERROR;

	for (i = 0; i < data->folder_stats[folder]->used; i++) {		/* for all previously found locations */
		if (data->message_list[i][folder]->status == GN_SMS_FLD_ToBeRemoved) {	/* previously deleted and read message */
			dprintf("Found deleted message, which will now be freed! %i , %i\n",
					i, data->message_list[i][folder]->location);
			for (j = i; j < data->folder_stats[folder]->used; j++) {
				memcpy(data->message_list[j][folder], data->message_list[j + 1][folder],
						sizeof(gn_sms_message_list));
			}
			data->folder_stats[folder]->used--;
			i--;
		}
	}
	return GN_ERR_NONE;
}

static gn_error sms_get_read(gn_data *data)
{
	int i, j, found;

	if (!data->message_list || !data->folder_stats || !data->sms_folder) return GN_ERR_INTERNALERROR;

	for (i = 0; i < data->sms_folder->number; i++) {		/* cycle through all messages in phone */
		found = 0;
		for (j = 0; j < data->folder_stats[data->sms_folder->folder_id]->used; j++) {		/* and compare them to those alread in list */
			if (data->sms_folder->locations[i] == data->message_list[j][data->sms_folder->folder_id]->location) found = 1;
		}
		if (data->folder_stats[data->sms_folder->folder_id]->used >= GN_SMS_MESSAGE_MAX_NUMBER) {
			dprintf("Max messages number in folder exceeded (%d)\n", GN_SMS_MESSAGE_MAX_NUMBER);
			return GN_ERR_MEMORYFULL;
		}
		if (!found) {
			dprintf("Found new (read) message. Will store it at #%i!\n", data->folder_stats[data->sms_folder->folder_id]->used);
			dprintf("%i\n", data->sms_folder->locations[i]);
			data->message_list[data->folder_stats[data->sms_folder->folder_id]->used][data->sms_folder->folder_id]->location = 
				data->sms_folder->locations[i];
			data->message_list[data->folder_stats[data->sms_folder->folder_id]->used][data->sms_folder->folder_id]->status = GN_SMS_FLD_New;
			data->folder_stats[data->sms_folder->folder_id]->used++;
			data->folder_stats[data->sms_folder->folder_id]->changed++;
			data->sms_status->changed++;
		}
	}
	return GN_ERR_NONE;
}

static gn_error sms_get_deleted(gn_data *data)
{
	int i, j, found = 0;

	for (i = 0; i < data->folder_stats[data->sms_folder->folder_id]->used; i++) {		/* for all previously found locations in folder */
		found = 0;

		for (j = 0; j < data->sms_folder->number; j++) {	/* see if there is a corresponding message in phone */
			if (data->message_list[i][data->sms_folder->folder_id]->location == data->sms_folder->locations[j]) found = 1;
		}
		if ((found == 0) && (data->message_list[i][data->sms_folder->folder_id]->status == GN_SMS_FLD_Old)) {	/* we have found a deleted message */
			dprintf("found a deleted message!!!! i: %i, loc: %i, MT: %i \n",
					i, data->message_list[i][data->sms_folder->folder_id]->location, data->sms_folder->folder_id);

			data->message_list[i][data->sms_folder->folder_id]->status = GN_SMS_FLD_Deleted;
			data->sms_status->changed++;
			data->folder_stats[data->sms_folder->folder_id]->changed++;
		}
	}
	return GN_ERR_NONE;
}

static gn_error sms_verify_status(gn_data *data)
{
	int i, j, found = 0;

	for (i = 0; i < data->folder_stats[data->sms_folder->folder_id]->used; i++) {		/* Cycle through all messages we know of */
		found = 0;
		if ((data->message_list[i][data->sms_folder->folder_id]->status == GN_SMS_FLD_NotRead) ||	/* if it is a unread one, i.e. not in folderstatus */
				(data->message_list[i][data->sms_folder->folder_id]->status == GN_SMS_FLD_NotReadHandled)) {
			for (j = 0; j < data->sms_folder->number; j++) {
				if (data->message_list[i][data->sms_folder->folder_id]->location == data->sms_folder->locations[j]) {
					/* We have a found a formerly unread message which has been read in the meantime */
					dprintf("Found a formerly unread message which has been read in the meantime: loc: %i\n",
							data->message_list[i][data->sms_folder->folder_id]->location);
					data->message_list[i][data->sms_folder->folder_id]->status = GN_SMS_FLD_Changed;
					data->sms_status->changed++;
					data->folder_stats[data->sms_folder->folder_id]->changed++;
				}
			}
		}
	}
	return GN_ERR_NONE;
}


API gn_error gn_sms_get_folder_changes(gn_data *data, struct gn_statemachine *state, int has_folders)
{
	gn_error error;
	gn_sms_folder  sms_folder;
	gn_sms_folder_list sms_folder_list;
	int i, previous_unread, previous_total;

	previous_total = data->sms_status->number;
	previous_unread = data->sms_status->unread;
	dprintf("GetFolderChanges: Old status: %d %d\n", data->sms_status->number, data->sms_status->unread);

	error = gn_sm_functions(GN_OP_GetSMSStatus, data, state);	/* Check overall SMS Status */
	ERROR();
	dprintf("GetFolderChanges: Status: %d %d\n", data->sms_status->number, data->sms_status->unread);

	if (!has_folders) {
		if ((previous_total == data->sms_status->number) && (previous_unread == data->sms_status->unread))
			data->sms_status->changed = 0;
		else
			data->sms_status->changed = 1;
		return GN_ERR_NONE;
	}

	data->sms_folder_list = &sms_folder_list;
	error = gn_sm_functions(GN_OP_GetSMSFolders, data, state);
	ERROR();

	data->sms_status->folders_count = data->sms_folder_list->number;

	for (i = 0; i < data->sms_status->folders_count; i++) {
		dprintf("GetFolderChanges: Freeing deleted messages for folder #%i\n", i);
		error = sms_free_deleted(data, i);
		ERROR();

		data->sms_folder = &sms_folder;
		data->sms_folder->folder_id = (gn_memory_type) i + 12;
		dprintf("GetFolderChanges: Getting folder status for folder #%i\n", i);
		error = gn_sm_functions(GN_OP_GetSMSFolderStatus, data, state);
		ERROR();

		data->sms_folder->folder_id = i;	/* so we don't need to do a modulo 8 each time */
			
		dprintf("GetFolderChanges: Reading read messages (%i) for folder #%i\n", data->sms_folder->number, i);
		error = sms_get_read(data);
		ERROR();

		dprintf("GetFolderChanges: Getting deleted messages for folder #%i\n", i);
		error = sms_get_deleted(data);
		ERROR();

		dprintf("GetFolderChanges: Verifying messages for folder #%i\n", i);
		error = sms_verify_status(data);
		ERROR();
	}
	return GN_ERR_NONE;
}

/***
 *** ENCODING SMS
 ***/


/**
 * sms_udh_encode - encodes User Data Header
 * @sms: SMS structure with the data source
 * @type:
 *
 * returns pointer to data it added;
 *
 * This function encodes the UserDataHeader as described in:
 *  o GSM 03.40 version 6.1.0 Release 1997, section 9.2.3.24
 *  o Smart Messaging Specification, Revision 1.0.0, September 15, 1997
 *  o Smart Messaging Specification, Revision 3.0.0
 */
static char *sms_udh_encode(gn_sms_raw *rawsms, int type)
{
	unsigned char pos;
	char *udh = rawsms->user_data;
	char *res = NULL;

	pos = udh[0];

	switch (type) {
	case GN_SMS_UDH_None:
		break;
	case GN_SMS_UDH_VoiceMessage:
	case GN_SMS_UDH_FaxMessage:
	case GN_SMS_UDH_EmailMessage:
		return NULL;
#if 0
		udh[pos+4] = udhi.u.SpecialSMSMessageIndication.MessageCount;
		if (udhi.u.SpecialSMSMessageIndication.Store) udh[pos+3] |= 0x80;
#endif
	case GN_SMS_UDH_ConcatenatedMessages:
		dprintf("Adding ConcatMsg header\n");
	case GN_SMS_UDH_OpLogo:
	case GN_SMS_UDH_CallerIDLogo:
	case GN_SMS_UDH_Ringtone:
	case GN_SMS_UDH_MultipartMessage:
	case GN_SMS_UDH_WAPPush:
		udh[0] += headers[type].length;
		res = udh+pos+1;
		memcpy(res, headers[type].header, headers[type].length);
		rawsms->user_data_length += headers[type].length;
		rawsms->length += headers[type].length;
		break;
	default:
		dprintf("Not supported User Data Header type\n");
		break;
	}
	if (!rawsms->udh_indicator) {
		rawsms->udh_indicator = 1;
		rawsms->length++;		/* Length takes one byte, too */
		rawsms->user_data_length++;
	}
	return res;
}

/**
 * sms_concat_header_encode - Adds concatenated messages header
 * @rawsms: processed SMS
 * @this: current part number
 * @total: total parts number
 *
 * This function adds sequent part of the concatenated messages header. Note
 * that this header should be the first of all headers.
 */
static gn_error sms_concat_header_encode(gn_sms_raw *rawsms, int this, int total)
{
	char *header = sms_udh_encode(rawsms, GN_SMS_UDH_ConcatenatedMessages);
	if (!header) return GN_ERR_NOTSUPPORTED;
	header[2] = 0xce;		/* Message serial number. Is 0xce value somehow special? -- pkot */
	header[3] = total;
	header[4] = this;
	return GN_ERR_NONE;
}

/**
 * sms_data_encode - encodes the date from the SMS structure to the phone frame
 *
 * @SMS: SMS structure to be encoded
 * @dcs: Data Coding Scheme field in the frame to be set
 * @message: phone frame to be filled in
 * @multipart: do we send one message or more?
 * @clen: coded data length
 *
 * This function does the phone frame encoding basing on the given SMS
 * structure. This function is capable to create only one frame at a time.
 */
static gn_error sms_data_encode(gn_sms *sms, gn_sms_raw *rawsms)
{
	gn_sms_dcs_alphabet_type al = GN_SMS_DCS_DefaultAlphabet;
	unsigned int i, size = 0;
	gn_error error;

	/* Additional Headers */
	switch (sms->dcs.type) {
	case GN_SMS_DCS_GeneralDataCoding:
		switch (sms->dcs.u.general.class) {
		case 0: break;
		case 1: rawsms->dcs |= 0xf0; break; /* Class 0 */
		case 2: rawsms->dcs |= 0xf1; break; /* Class 1 */
		case 3: rawsms->dcs |= 0xf2; break; /* Class 2 */
		case 4: rawsms->dcs |= 0xf3; break; /* Class 3 */
		default: dprintf("What ninja-mutant class is this?\n"); break; 
		}
		if (sms->dcs.u.general.compressed) {
			/* Compression not supported yet */
			/* dcs[0] |= 0x20; */
		}
		al = sms->dcs.u.general.alphabet;
		break;
	case GN_SMS_DCS_MessageWaiting:
		al = sms->dcs.u.message_waiting.alphabet;
		if (sms->dcs.u.message_waiting.discard) rawsms->dcs |= 0xc0;
		else if (sms->dcs.u.message_waiting.alphabet == GN_SMS_DCS_UCS2) rawsms->dcs |= 0xe0;
		else rawsms->dcs |= 0xd0;

		if (sms->dcs.u.message_waiting.active) rawsms->dcs |= 0x08;
		rawsms->dcs |= (sms->dcs.u.message_waiting.type & 0x03);

		break;
	default:
		return GN_ERR_WRONGDATAFORMAT;
	}

	rawsms->length = rawsms->user_data_length = 0;

	for (i = 0; i < GN_SMS_PART_MAX_NUMBER; i++) {
		switch (sms->user_data[i].type) {
		case GN_SMS_DATA_Bitmap:
			switch (sms->user_data[0].u.bitmap.type) {
			case GN_BMP_PictureMessage:
				size = sms_nokia_bitmap_encode(&(sms->user_data[i].u.bitmap),
							       rawsms->user_data + rawsms->user_data_length,
							       (i == 0));
				break;
			case GN_BMP_OperatorLogo:
				if (!sms_udh_encode(rawsms, GN_SMS_UDH_OpLogo)) return GN_ERR_NOTSUPPORTED;
			default:
				size = gn_bmp_sms_encode(&(sms->user_data[i].u.bitmap),
							 rawsms->user_data + rawsms->user_data_length);
				break;
			}
			rawsms->length += size;
			rawsms->user_data_length += size;
			rawsms->dcs = 0xf5;
			rawsms->udh_indicator = 1;
			break;

		case GN_SMS_DATA_Animation: {
			int j;
			for (j = 0; j < 4; j++) {
				size = gn_bmp_sms_encode(&(sms->user_data[i].u.animation[j]), rawsms->user_data + rawsms->user_data_length);
				rawsms->length += size;
				rawsms->user_data_length += size;
			}
			rawsms->dcs = 0xf5;
			rawsms->udh_indicator = 1;
			break;
		}

		case GN_SMS_DATA_Text: {
			unsigned int length, offset = rawsms->user_data_length;

			length = strlen(sms->user_data[i].u.text);
			switch (al) {
			case GN_SMS_DCS_DefaultAlphabet:
#define UDH_Length 0
				size = char_7bit_pack((7 - (UDH_Length % 7)) % 7,
						      sms->user_data[i].u.text,
						      rawsms->user_data + offset,
						      &length);
				rawsms->length = length;
				rawsms->user_data_length = size + offset;
				break;
			case GN_SMS_DCS_8bit:
				rawsms->dcs |= 0xf4;
				memcpy(rawsms->user_data + offset, sms->user_data[i].u.text, sms->user_data[i].u.text[0]);
				rawsms->user_data_length = rawsms->length = length + offset;
				break;
			case GN_SMS_DCS_UCS2:
				rawsms->dcs |= 0x08;
				char_unicode_encode(rawsms->user_data + offset, sms->user_data[i].u.text, length);
				length *= 2;
				rawsms->user_data_length = rawsms->length = length + offset;
				break;
			default:
				return GN_ERR_WRONGDATAFORMAT;
			}
			break;
		}

		case GN_SMS_DATA_NokiaText:
			size = sms_nokia_text_encode(sms->user_data[i].u.text,
						     rawsms->user_data + rawsms->user_data_length,
						     (i == 0));
			rawsms->length += size;
			rawsms->user_data_length += size;
			break;

		case GN_SMS_DATA_iMelody:
			size = imelody_sms_encode(sms->user_data[i].u.text, rawsms->user_data + rawsms->user_data_length);
			dprintf("Imelody, size %d\n", size);
			rawsms->length += size;
			rawsms->user_data_length += size;
			rawsms->dcs = 0xf5;
			rawsms->udh_indicator = 1;
			break;

		case GN_SMS_DATA_Multi:
			size = sms->user_data[0].length;
			if (!sms_udh_encode(rawsms, GN_SMS_UDH_MultipartMessage)) return GN_ERR_NOTSUPPORTED;
			error = sms_concat_header_encode(rawsms, sms->user_data[i].u.multi.this, sms->user_data[i].u.multi.total);
			ERROR();

			memcpy(rawsms->user_data + rawsms->user_data_length, sms->user_data[i].u.multi.binary, MAX_SMS_PART);
			rawsms->length += size;
			rawsms->user_data_length += size;
			rawsms->dcs = 0xf5;
			break;

		case GN_SMS_DATA_Ringtone:
			if (!sms_udh_encode(rawsms, GN_SMS_UDH_Ringtone)) return GN_ERR_NOTSUPPORTED;
			size = ringtone_sms_encode(rawsms->user_data + rawsms->length, &sms->user_data[i].u.ringtone);
			rawsms->length += size;
			rawsms->user_data_length += size;
			rawsms->dcs = 0xf5;
			break;

		case GN_SMS_DATA_WAPPush:
			if (!sms_udh_encode(rawsms, GN_SMS_UDH_WAPPush)) return GN_ERR_NOTSUPPORTED;
			size = sms->user_data[i].length;
			memcpy(rawsms->user_data + rawsms->user_data_length, sms->user_data[i].u.text, size );
			rawsms->length += size;
			rawsms->user_data_length += size;
			rawsms->dcs = 0xf5;
			break;

		case GN_SMS_DATA_Concat:
			dprintf("Encoding concat header\n");
			al = GN_SMS_DCS_8bit;
			rawsms->dcs = 0xf5;
			sms_concat_header_encode(rawsms, sms->user_data[i].u.concat.this, sms->user_data[i].u.concat.total);
			break;

		case GN_SMS_DATA_None:
			return GN_ERR_NONE;

		default:
			dprintf("What kind of ninja-mutant user_data is this?\n");
			break;
		}
	}
	return GN_ERR_NONE;
}

gn_error sms_prepare(gn_sms *sms, gn_sms_raw *rawsms)
{
	switch (rawsms->type = sms->type) {
	case GN_SMS_MT_Submit:
	case GN_SMS_MT_Deliver:
	case GN_SMS_MT_Picture:
		break;
	case GN_SMS_MT_DeliveryReport:
	default:
		dprintf("Not supported message type: %d\n", rawsms->type);
		return GN_ERR_NOTSUPPORTED;
	}
	/* Encoding the header */
	rawsms->report = sms->delivery_report;
	rawsms->remote_number[0] = char_semi_octet_pack(sms->remote.number, rawsms->remote_number + 1, sms->remote.type);
	rawsms->validity_indicator = GN_SMS_VP_RelativeFormat;
	rawsms->validity[0] = 0xa9;

	sms_data_encode(sms, rawsms);

	return GN_ERR_NONE;
}

static void sms_dump_raw(gn_sms_raw *rawsms)
{
	char buf[10240];

	memset(buf, 0, 10240);

	dprintf("dcs: 0x%x\n", rawsms->dcs);
	dprintf("Length: 0x%x\n", rawsms->length);
	dprintf("user_data_length: 0x%x\n", rawsms->user_data_length);
	dprintf("ValidityIndicator: %d\n", rawsms->validity_indicator);
	bin2hex(buf, rawsms->user_data, rawsms->user_data_length);
	dprintf("user_data: %s\n", buf);
}

static gn_error sms_send_long(gn_data *data, struct gn_statemachine *state);

/**
 * gn_sms_send - The main function for the SMS sending
 * @data: GSM data for the phone driver
 * @state: current statemachine state
 *
 * The high level function to send SMS. You need to fill in data->sms
 * (gn_sms) in the higher level. This is converted to raw_sms here,
 * and then phone driver takes the fields it needs and sends it in the
 * phone specific way to the phone.
 */
API gn_error gn_sms_send(gn_data *data, struct gn_statemachine *state)
{
	gn_error error = GN_ERR_NONE;

	if (!data->sms) return GN_ERR_INTERNALERROR;

	data->raw_sms = malloc(sizeof(*data->raw_sms));
	memset(data->raw_sms, 0, sizeof(*data->raw_sms));

	data->raw_sms->status = GN_SMS_Sent;

	data->raw_sms->message_center[0] = char_semi_octet_pack(data->sms->smsc.number, data->raw_sms->message_center + 1, data->sms->smsc.type);
	if (data->raw_sms->message_center[0] % 2) data->raw_sms->message_center[0]++;
	data->raw_sms->message_center[0] = data->raw_sms->message_center[0] / 2 + 1;

	error = sms_prepare(data->sms, data->raw_sms);
	ERROR();

	if (data->raw_sms->length > GN_SMS_MAX_LENGTH) {
		dprintf("SMS is too long? %d\n", data->raw_sms->length);
		error = sms_send_long(data, state);
		goto cleanup;
	}

	error = gn_sm_functions(GN_OP_SendSMS, data, state);
 cleanup:
	free(data->raw_sms);
	data->raw_sms = NULL;
	return error;
}

static gn_error sms_send_long(gn_data *data, struct gn_statemachine *state)
{
	int i, count;
	gn_sms_raw LongSMS, *rawsms = &LongSMS;	/* We need local copy for our dirty tricks */
	gn_sms sms;
	gn_error error = GN_ERR_NONE;

	LongSMS = *data->raw_sms;
	sms = *data->sms;

	count = (rawsms->user_data_length + MAX_SMS_PART - 1) / MAX_SMS_PART;
	dprintf("Will need %d sms-es\n", count);
	for (i = 0; i < count; i++) {
		dprintf("Sending sms #%d\n", i);
		sms.user_data[0].type = GN_SMS_DATA_Multi;
		sms.user_data[0].length = MAX_SMS_PART;
		if (i + 1 == count)
			sms.user_data[0].length = rawsms->user_data_length % MAX_SMS_PART;
		memcpy(sms.user_data[0].u.multi.binary, rawsms->user_data + i*MAX_SMS_PART, MAX_SMS_PART);
		sms.user_data[0].u.multi.this = i + 1;
		sms.user_data[0].u.multi.total = count;
		sms.user_data[1].type = GN_SMS_DATA_None;
		data->sms = &sms;
		error = gn_sms_send(data, state);
		ERROR();
	}
	return GN_ERR_NONE;
}

API gn_error gn_sms_save(gn_data *data, struct gn_statemachine *state)
{
	gn_error error = GN_ERR_NONE;
	gn_sms_raw rawsms;

	data->raw_sms = &rawsms;
	memset(&rawsms, 0, sizeof(rawsms));

	data->raw_sms->number = data->sms->number;
	data->raw_sms->status = data->sms->status;
	data->raw_sms->memory_type = data->sms->memory_type;

	sms_timestamp_pack(&data->sms->smsc_time, data->raw_sms->smsc_time);
	dprintf("\tDate: %s\n", sms_timestamp_print(data->raw_sms->smsc_time));

	if (data->sms->smsc.number[0] != '\0') {
		data->raw_sms->message_center[0] = 
			char_semi_octet_pack(data->sms->smsc.number, data->raw_sms->message_center + 1, data->sms->smsc.type);
		if (data->raw_sms->message_center[0] % 2) data->raw_sms->message_center[0]++;
		data->raw_sms->message_center[0] = data->raw_sms->message_center[0] / 2 + 1;
	}
	error = sms_prepare(data->sms, data->raw_sms);
	ERROR();

	if (data->raw_sms->length > GN_SMS_MAX_LENGTH) {
		dprintf("SMS is too long? %d\n", data->raw_sms->length);
		goto cleanup;
	}

	error = gn_sm_functions(GN_OP_SaveSMS, data, state);
cleanup:
	data->raw_sms = NULL;
	return error;
}
