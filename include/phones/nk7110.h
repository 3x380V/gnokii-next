/*

  $Id: nk7110.h,v 1.6 2001-11-08 16:47:48 pkot Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Jan�k ml.
  Copytight (C) 2000 Chris Kemp

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides functions specific to the 7110 series. 
  See README for more details on supported mobile phones.

  The various routines are called P7110_(whatever).

  $Log: nk7110.h,v $
  Revision 1.6  2001-11-08 16:47:48  pkot
  Start fiddling with 7110 and SMS

  Revision 1.5  2001/06/27 23:52:52  pkot
  7110/6210 updates (Marian Jancar)

  Revision 1.4  2001/05/24 20:47:30  chris
  More updating of 7110 code and some of xgnokii_lowlevel changed over.

  Revision 1.3  2001/03/23 13:40:25  chris
  Pavel's patch and a few fixes.

  Revision 1.2  2001/03/21 23:36:08  chris
  Added the statemachine
  This will break gnokii --identify and --monitor except for 6210/7110

  Revision 1.1  2001/02/21 19:57:13  chris
  More fiddling with the directory layout

  Revision 1.1  2001/02/16 14:29:54  chris
  Restructure of common/.  Fixed a problem in fbus-phonet.c
  Lots of dprintfs for Marcin
  Any size xpm can now be loaded (eg for 7110 startup logos)
  nk7110 code detects 7110/6210 and alters startup logo size to suit
  Moved Marcin's extended phonebook code into gnokii.c

  Revision 1.4  2001/01/29 17:14:44  chris
  dprintf now in misc.h (and fiddling with 7110 code)

  Revision 1.3  2001/01/23 15:32:44  chris
  Pavel's 'break' and 'static' corrections.
  Work on logos for 7110.

  Revision 1.2  2001/01/17 02:54:56  chris
  More 7110 work.  Use with care! (eg it is not possible to delete phonebook entries)
  I can now edit my phonebook in xgnokii but it is 'work in progress'.

  Revision 1.1  2001/01/14 22:47:01  chris
  Preliminary 7110 support (dlr9 only) and the beginnings of a new structure


*/

#ifndef __phones_nk7110_h
#define __phones_nk7110_h

#include <gsm-common.h>
#include "gsm-statemachine.h"

extern bool P7110_LinkOK;

/* Message types */
#define P7110_MSG_COMMSTATUS	0x01	/* Communication status */
#define P7110_MSG_SMS		0x02	/* SMS handling */
#define P7110_MSG_PHONEBOOK	0x03	/* Phonebook functions */
#define P7110_MSG_NETSTATUS	0x0a	/* Network status */
#define P7110_MSG_CALENDAR	0x13	/* Calendar notes */
#define P7110_MSG_FOLDER	0x14	/* Folders handling */
#define P7110_MSG_BATTERY	0x17	/* Battery info */
#define P7110_MSG_CLOCK		0x19	/* Date & alarm */
#define P7110_MSG_IDENTITY	0x1b	/* Brief product info */
#define P7110_MSG_RINGTONE	0x1f	/* Ringtone handling */
#define P7110_MSG_SECURITY	0x40	/* Security */
#define P7110_MSG_STLOGO	0x7a	/* Startup logo */
#define P7110_MSG_VERREQ	0xd1	/* HW&SW version request */
#define P7110_MSG_VERRESP	0xd2	/* HW&SW version response */

/* SMS handling message subtypes (send) */
#define P7110_SUBSMS_SEND_SMS		0x01	/* Send SMS */
#define P7110_SUBSMS_SET_CELLBRD	0x20	/* Set cell broadcast */
#define P7110_SUBSMS_SET_SMSC		0x30	/* Set SMS center */
#define P7110_SUBSMS_GET_SMSC		0x33	/* Get SMS center */
/* SMS handling message subtypes (recv) */
#define P7110_SUBSMS_SMS_SENT		0x02	/* SMS sent */
#define P7110_SUBSMS_SEND_FAIL		0x03	/* SMS send failed */
#define P7110_SUBSMS_SMS_RCVD		0x10	/* SMS received */
#define P7110_SUBSMS_CELLBRD_OK		0x21	/* Set cell broadcast success*/
#define P7110_SUBSMS_CELLBRD_FAIL	0x22	/* Set cell broadcast failure */
#define P7110_SUBSMS_READ_CELLBRD	0x23	/* Read cell broadcast */
#define P7110_SUBSMS_SMSC_OK		0x31	/* Set SMS center success*/
#define P7110_SUBSMS_SMSC_FAIL		0x32	/* Set SMS center failure */
#define P7110_SUBSMS_SMSC_RCVD		0x34	/* SMS center received */
#define P7110_SUBSMS_SMSC_RCVFAIL	0x35	/* SMS center receive failure */

/* Clock handling message subtypes (send) */
#define P7110_SUBCLO_GET_DATE		0x62	/* Get date & time */
#define P7110_SUBCLO_GET_ALARM		0x6D	/* Get alarm */
/* Clock handling message subtypes (recv) */
#define P7110_SUBCLO_DATE_RCVD		0x63	/* Received date & time */
#define P7110_SUBCLO_ALARM_RCVD		0x6E	/* Received alarm */
/* Alarm on/off */
#define P7110_ALARM_ENABLED		0x02	/* Alarm enabled */
#define P7110_ALARM_DISABLED		0x01	/* Alarm disabled */

/* Calendar handling message subtypes (send) */
#define P7110_SUBCAL_ADD_MEETING	0x01	/* Add meeting note */
#define P7110_SUBCAL_ADD_CALL		0x03	/* Add call note */
#define P7110_SUBCAL_ADD_BIRTHDAY	0x05	/* Add birthday note */
#define P7110_SUBCAL_ADD_REMINDER	0x07	/* Add reminder note */
#define P7110_SUBCAL_DEL_NOTE		0x0b	/* Delete note */
#define P7110_SUBCAL_GET_NOTE		0x19	/* Get note */
#define P7110_SUBCAL_GET_FREEPOS	0x31	/* Get first free position */
#define P7110_SUBCAL_GET_INFO		0x3a	/* Calendar sumary */
/* Calendar handling message subtypes (recv) */
#define P7110_SUBCAL_ADD_MEETING_RESP	0x02	/* Add meeting note response */
#define P7110_SUBCAL_ADD_CALL_RESP	0x04	/* Add call note response */
#define P7110_SUBCAL_ADD_BIRTHDAY_RESP	0x06	/* Add birthday note response */
#define P7110_SUBCAL_ADD_REMINDER_RESP	0x08	/* Add reminder note response */
#define P7110_SUBCAL_DEL_NOTE_RESP	0x0c	/* Dletete note response */
#define P7110_SUBCAL_NOTE_RCVD		0x1a	/* Received note */
#define P7110_SUBCAL_FREEPOS_RCVD	0x32	/* Received first free position */
#define P7110_SUBCAL_INFO_RCVD		0x3b	/* Received calendar summary*/
/* Calendar note types */
#define P7110_NOTE_MEETING		0x01	/* Metting */
#define P7110_NOTE_CALL			0x02	/* Call */
#define P7110_NOTE_BIRTHDAY		0x04	/* Birthday */
#define P7110_NOTE_REMINDER		0x08	/* Reminder */

/* Phone Memory types */
#define P7110_MEMORY_DIALLED	0x01	/* Dialled numbers */
#define P7110_MEMORY_MISSED	0x02	/* Missed calls */
#define P7110_MEMORY_RECEIVED	0x03	/* Received calls */
#define P7110_MEMORY_PHONE	0x05	/* Telephone phonebook */
#define P7110_MEMORY_SIM	0x06	/* SIM phonebook */
#define P7110_MEMORY_SPEEDDIALS	0x0e	/* Speed dials */
#define P7110_MEMORY_GROUPS	0x10	/* Caller groups */

#define P7110_MEMORY_DC		0x01	/* ME dialled calls list */
#define P7110_MEMORY_MC		0x02	/* ME missed (unanswered received) calls list */
#define P7110_MEMORY_RC		0x03	/* ME received calls list */
#define P7110_MEMORY_FD		0x04	/* ?? SIM fixdialling-phonebook */
#define P7110_MEMORY_ME		0x05	/* ME (Mobile Equipment) phonebook */
#define P7110_MEMORY_SM		0x06	/* SIM phonebook */
#define P7110_MEMORY_ON		0x07	/* ?? SIM (or ME) own numbers list */
#define P7110_MEMORY_EN		0x08	/* ?? SIM (or ME) emergency number */
#define P7110_MEMORY_MT		0x09	/* ?? combined ME and SIM phonebook */
#define P7110_MEMORY_VOICE	0x0b	/* Voice Mailbox */

/* This is used when the memory type is unknown. */
#define P7110_MEMORY_XX 0xff

/* Entry Types for the enhanced phonebook */
#define P7110_ENTRYTYPE_POINTER		0x04	/* Pointer to other memory */
#define P7110_ENTRYTYPE_NAME		0x07	/* Name always the only one */
#define P7110_ENTRYTYPE_EMAIL		0x08	/* Email Adress (TEXT) */
#define P7110_ENTRYTYPE_POSTAL		0x09	/* Postal Address (Text) */
#define P7110_ENTRYTYPE_NOTE		0x0a	/* Note (Text) */
#define P7110_ENTRYTYPE_NUMBER		0x0b	/* Phonenumber */
#define P7110_ENTRYTYPE_RINGTONE	0x0c	/* Ringtone */
#define P7110_ENTRYTYPE_DATE		0x13	/* Date for a Called List */
#define P7110_ENTRYTYPE_LOGO		0x1b	/* Group logo */
#define P7110_ENTRYTYPE_LOGOSWITCH	0x1c	/* Group logo on/off */
#define P7110_ENTRYTYPE_GROUP		0x1e	/* Group number for phonebook entry */

#ifdef __phones_nk7110_c  /* Prototype functions for phone-7110.c only */

static GSM_Error P7110_Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_Initialise(GSM_Statemachine *state);
static GSM_Error P7110_GetModel(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetRevision(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetIMEI(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_Identify(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetBatteryLevel(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetRFLevel(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetMemoryStatus(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_SetBitmap(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetBitmap(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_WritePhonebookLocation(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_ReadPhonebook(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetNetworkInfo(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetSpeedDial(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetSMSCenter(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetClock(char req_type, GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetCalendarNote(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetSMS(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetSMSFolders(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error P7110_GetSMSFolderStatus(GSM_Data *data, GSM_Statemachine *state);

static GSM_Error P7110_Incoming0x1b(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P7110_IncomingPhonebook(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P7110_IncomingNetwork(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P7110_IncomingBattLevel(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P7110_IncomingStartup(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P7110_IncomingSMS(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P7110_IncomingFolder(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
static GSM_Error P7110_IncomingClock(int messagetype, unsigned char *message, int length, GSM_Data *data);
static GSM_Error P7110_IncomingCalendar(int messagetype, unsigned char *message, int length, GSM_Data *data);

static int GetMemoryType(GSM_MemoryType memory_type);

#if 0
static GSM_Error P7110_Initialise(char *port_device, char *initlength,
		 GSM_ConnectionType connection,
		 void (*rlp_callback)(RLP_F96Frame *frame));
static GSM_Error P7110_GenericCRHandler(int messagetype, unsigned char *buffer, int length);
static GSM_Error P7110_IncomingDefault(int messagetype, unsigned char *buffer, int length);
static GSM_Error P7110_GetIMEI(char *imei);
static GSM_Error P7110_GetRevision(char *revision);
static GSM_Error P7110_GetModel(char *model);
static GSM_Error P7110_ReadPhonebook(GSM_PhonebookEntry *entry);
static GSM_Error P7110_WritePhonebookLocation(GSM_PhonebookEntry *entry);
static GSM_Error P7110_GetMemoryStatus(GSM_MemoryStatus *status);
static GSM_Error P7110_GetBatteryLevel(GSM_BatteryUnits *units, float *level);
static GSM_Error P7110_GetRFLevel(GSM_RFUnits *units, float *level);
static GSM_Error P7110_GetBitmap(GSM_Bitmap *bitmap);
static GSM_Error P7110_SetBitmap(GSM_Bitmap *bitmap);
static GSM_Error P7110_DialVoice(char *Number);
static void P7110_Terminate();
static bool P7110_SendRLPFrame( RLP_F96Frame *frame, bool out_dtx );

#endif


#endif  /* #ifdef __phones_nk7110_c */

#endif  /* #ifndef __phones_nk7110_h */
