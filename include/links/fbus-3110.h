/*

  $Id: fbus-3110.h,v 1.4 2002-03-26 01:10:30 pkot Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Jan�k ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides an API for accessing functions via fbus.
  See README for more details on supported mobile phones.

  The various routines are called FB3110_(whatever).

*/

#ifndef __links_fbus_3110_h
#define __links_fbus_3110_h

#include <time.h>
#include "gsm-statemachine.h"
#include "config.h"
#include "compat.h"

#ifdef WIN32
#  include <sys/types.h>
#endif

#define FB3110_MAX_FRAME_LENGTH 256
#define FB3110_MAX_MESSAGE_TYPES 128
#define FB3110_MAX_TRANSMIT_LENGTH 256
#define FB3110_MAX_CONTENT_LENGTH 120

/* This byte is at the beginning of all GSM Frames sent over FBUS to Nokia
   phones.  This may have to become a phone dependant parameter... */
#define FB3110_FRAME_ID 0x01


/* States for receive code. */

enum FB3110_RX_States {
	FB3110_RX_Sync,
	FB3110_RX_Discarding,
	FB3110_RX_GetLength,
	FB3110_RX_GetMessage
};


typedef struct{
	int Checksum;
	int BufferCount;
	enum FB3110_RX_States State;
	int FrameType;
	int FrameLength;
	char Buffer[FB3110_MAX_FRAME_LENGTH];
} FB3110_IncomingFrame;

typedef struct {
	u16 message_length;
	u8 message_type;
	u8 *buffer;
} FB3110_OutgoingMessage;


typedef struct{
	FB3110_IncomingFrame i;
	u8 RequestSequenceNumber;
} FB3110_Link;

GSM_Error FB3110_Initialise(GSM_Link *newlink, GSM_Statemachine *state);

#endif   /* #ifndef __links_fbus_3110_h */
