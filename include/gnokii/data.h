/*

  $Id: data.h,v 1.29 2002-05-17 00:22:10 pkot Exp $

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

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Jan�k ml.

*/

#ifndef __gsm_data_h
#define __gsm_data_h

#include "gsm-common.h"
#include "gsm-sms.h"
#include "gsm-error.h"
#include "data/rlp-common.h"

/* This is a generic holder for high level information - eg a GSM_Bitmap */
typedef struct {
	SMS_Folder *SMSFolder;
	SMS_FolderList *SMSFolderList;
	GSM_SMSMessage *RawSMS;
	GSM_API_SMS *SMS;
	GSM_PhonebookEntry *PhonebookEntry;
	GSM_SpeedDial *SpeedDial;
	GSM_MemoryStatus *MemoryStatus;
	SMS_MessagesList *MessagesList[MAX_SMS_MESSAGES][MAX_SMS_FOLDERS];
	SMS_Status *SMSStatus;
	SMS_FolderStats *FolderStats[MAX_SMS_FOLDERS];
	SMS_MessageCenter *MessageCenter;
	char *Imei;
	char *Revision;
	char *Model;
	char *Manufacturer;
	GSM_NetworkInfo *NetworkInfo;
	GSM_CalendarNotesList *CalendarNotesList;
	GSM_CalendarNote *CalendarNote;
	GSM_Bitmap *Bitmap;
	GSM_Ringtone *Ringtone;
	GSM_Profile *Profile;
	GSM_BatteryUnits *BatteryUnits;
	float *BatteryLevel;
	GSM_RFUnits *RFUnits;
	float *RFLevel;
	GSM_DisplayOutput *DisplayOutput;
	char *IncomingCallNr;
	GSM_PowerSource *PowerSource;
	GSM_DateTime *DateTime;
	GSM_RawData *RawData;
	GSM_CallDivert *CallDivert;
	GSM_Error (*OnSMS)(GSM_API_SMS *Message);
	int *DisplayStatus;
	void (*OnCellBroadcast)(GSM_CBMessage *Message);
	GSM_NetMonitor *NetMonitor;
	GSM_CallInfo *CallInfo;
	void (*CallNotification)(GSM_CallStatus CallStatus, GSM_CallInfo *CallInfo);
	RLP_F96Frame *RLP_Frame;
	bool RLP_OutDTX;
	void (*RLP_RX_Callback)(RLP_F96Frame *Frame);
	GSM_SecurityCode *SecurityCode;
	const char *DTMFString;
	unsigned char ResetType;
	GSM_KeyCode KeyCode;
	unsigned char Character;
} GSM_Data;

/* Global structures intended to be independant of phone etc */
/* Obviously currently rather Nokia biased but I think most things */
/* (eg at commands) can be enumerated */

/* A structure to hold information about the particular link */
/* The link comes 'under' the phone */
typedef struct {
	char PortDevice[GSM_MAX_DEVICE_NAME_LENGTH];   /* The port device */
	int InitLength;        /* Number of chars sent to sync the serial port */
	GSM_ConnectionType ConnectionType;   /* Connection type, serial, ir etc */

	/* A regularly called loop function */
	/* timeout can be used to make the function block or not */
	GSM_Error (*Loop)(struct timeval *timeout);

	/* A pointer to the function used to send out a message */
	/* This is used by the phone specific code to send a message over the link */
	GSM_Error (*SendMessage)(u16 messagesize, u8 messagetype, unsigned char *message);
} GSM_Link;

/* Small structure used in GSM_Phone */
/* Messagetype is passed to the function in case it is a 'generic' one */
typedef struct {
	u8 MessageType;
	GSM_Error (*Functions)(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
} GSM_IncomingFunctionType;

typedef enum {
	GOP_Init,
	GOP_Terminate,
	GOP_GetModel,
	GOP_GetRevision,
	GOP_GetImei,
	GOP_GetManufacturer,
	GOP_Identify,
	GOP_GetBitmap,
	GOP_SetBitmap,
	GOP_GetBatteryLevel,
	GOP_GetRFLevel,
	GOP_DisplayOutput,
	GOP_GetMemoryStatus,
	GOP_ReadPhonebook,
	GOP_WritePhonebook,
	GOP_GetPowersource,
	GOP_GetAlarm,
	GOP_GetSMSStatus,
	GOP_GetIncomingCallNr,
	GOP_GetNetworkInfo,
	GOP_GetSMS,
	GOP_GetSMSnoValidate,
	GOP_GetSMSFolders,
	GOP_GetSMSFolderStatus,
	GOP_GetIncomingSMS,
	GOP_FindUnreadSMS,
	GOP_GetNextSMS,
	GOP_DeleteSMS,
	GOP_SendSMS,
	GOP_GetSpeedDial,
	GOP_GetSMSCenter,
	GOP_SetSMSCenter,
	GOP_GetDateTime,
	GOP_GetCalendarNote,
	GOP_CallDivert,
	GOP_OnSMS,
	GOP_PollSMS,
	GOP_SetAlarm,
	GOP_SetDateTime,
	GOP_GetProfile,
	GOP_SetProfile,
	GOP_WriteCalendarNote,
	GOP_DeleteCalendarNote,
	GOP_SetSpeedDial,
	GOP_GetDisplayStatus,
	GOP_PollDisplay,
	GOP_SaveSMS,
	GOP_SetCellBroadcast,
	GOP_NetMonitor,
	GOP_MakeCall,
	GOP_AnswerCall,
	GOP_CancelCall,
	GOP_SetCallNotification,
	GOP_SendRLPFrame,
	GOP_SetRLPRXCallback,
	GOP_EnterSecurityCode,
	GOP_GetSecurityCodeStatus,
	GOP_ChangeSecurityCode,
	GOP_SendDTMF,
	GOP_Reset,
	GOP_GetRingtone,
	GOP_SetRingtone,
	GOP_GetRawRingtone,
	GOP_SetRawRingtone,
	GOP_PressPhoneKey,
	GOP_ReleasePhoneKey,
	GOP_EnterChar,
	GOP_Max,	/* don't append anything after this entry */
} GSM_Operation;

/* This structure contains the 'callups' needed by the statemachine */
/* to deal with messages from the phone and other information */

typedef struct _GSM_Statemachine GSM_Statemachine;

typedef struct {
	/* These make up a list of functions, one for each message type and NULL terminated */
	GSM_IncomingFunctionType *IncomingFunctions;
	GSM_Error (*DefaultFunction)(int messagetype, unsigned char *buffer, int length);
	GSM_Information Info;
	GSM_Error (*Functions)(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);
} GSM_Phone;


/* The states the statemachine can take */

typedef enum {
	Startup,            /* Not yet initialised */
	Initialised,        /* Ready! */
	MessageSent,        /* A command has been sent to the link(phone) */
	WaitingForResponse, /* We are waiting for a response from the link(phone) */
	ResponseReceived    /* A response has been received - waiting for the phone layer to collect it */
} GSM_State;

/* How many message types we can wait for at one */
#define SM_MAXWAITINGFOR 3

/* All properties of the state machine */

struct _GSM_Statemachine {
	GSM_State CurrentState;
	GSM_Link Link;
	GSM_Phone Phone;
	
	/* Store last message for resend purposes */
	u8 LastMsgType;
	u16 LastMsgSize;
	void *LastMsg;

	/* The responses we are waiting for */
	unsigned char NumWaitingFor;
	unsigned char NumReceived;
	unsigned char WaitingFor[SM_MAXWAITINGFOR];
	GSM_Error ResponseError[SM_MAXWAITINGFOR];
	/* Data structure to be filled in with the response */
	GSM_Data *Data[SM_MAXWAITINGFOR];
};

/* Undefined functions in fbus/mbus files */
extern GSM_Error Unimplemented(void);
#define UNIMPLEMENTED (void *) Unimplemented

API GSM_MemoryType StrToMemoryType (const char *s);

API void GSM_DataClear(GSM_Data *data);

#endif	/* __gsm_data_h */
