/* -*- linux-c -*-

  $Id: cbus.h,v 1.5 2002-03-26 01:10:30 pkot Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2001 Pavel Machek <pavel@ucw.cz>
  Copyright (C) 2001 Michl Ladislav <xmichl03@stud.fee.vutbr.cz>

  Released under the terms of the GNU GPL, see file COPYING for more details.

 */

#ifndef __cbus_h
#define __cbus_h

#define CBUS_MAX_FRAME_LENGTH 256
#define CBUS_MAX_TRANSMIT_LENGTH 256
#define CBUS_MAX_MSG_LENGTH 256

enum CBUS_RX_States {
	CBUS_RX_Header,
	CBUS_RX_FrameType1,
	CBUS_RX_FrameType2,
	CBUS_RX_GetLengthLB,
	CBUS_RX_GetLengthHB,
	CBUS_RX_GetMessage,
	CBUS_RX_GetCSum
};

typedef struct{
	int checksum;
	int BufferCount;
	enum CBUS_RX_States state;
	int FrameHeader1;
	int FrameHeader2;
	int FrameType1;
	int FrameType2;
	int MessageLength;
	unsigned char buffer[CBUS_MAX_FRAME_LENGTH];
	u8 prev_rx_byte;
} CBUS_IncomingFrame;

typedef struct {
	int message_length;
	unsigned char buffer[CBUS_MAX_MSG_LENGTH];
} CBUS_OutgoingMessage;

typedef struct{
	CBUS_IncomingFrame i;
} CBUS_Link;

GSM_Error CBUS_Initialise(GSM_Statemachine *state);

void sendat(unsigned char *msg);

#endif   /* #ifndef __cbus_h */
