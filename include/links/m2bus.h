/*

  $Id: m2bus.h,v 1.1 2002-05-29 02:09:02 bozo Exp $

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

  Copyright (C) 2000 Hugh Blemings & Pavel Jan�k ml.
  Copyright (C) 2000 Chris Kemp
  Copyright (C) 2002 BORBELY Zoltan

  This file provides an API for accessing functions via m2bus.
  See README for more details on supported mobile phones.

  The various routines are called M2BUS_(whatever).

*/

#ifndef __links_m2bus_h
#define __links_m2bus_h

#include <time.h>
#include "config.h"
#include "compat.h"

#ifdef WIN32
#  include <sys/types.h>
#endif

/* This byte is at the beginning of all GSM Frames sent over M2BUS to Nokia
   phones.  This may have to become a phone dependant parameter... */
#define M2BUS_FRAME_ID 0x1f

/* This byte is at the beginning of all GSM Frames sent over IR to Nokia phones. */
#define M2BUS_IR_FRAME_ID 0x14

/* Nokia mobile phone. */
#define M2BUS_DEVICE_PHONE 0x00

/* Our PC. */
#define M2BUS_DEVICE_PC 0x1d


enum M2BUS_RX_States {
    	M2BUS_RX_Sync,
	M2BUS_RX_Discarding,
	M2BUS_RX_GetDestination,
	M2BUS_RX_GetSource,
	M2BUS_RX_GetType,
	M2BUS_RX_GetLength1,
	M2BUS_RX_GetLength2,
	M2BUS_RX_GetMessage
};

typedef struct{
	enum M2BUS_RX_States state;
	int BufferCount;
	struct timeval time_now;
	struct timeval time_last;
	int MessageSource;
	int MessageDestination;
	int MessageType;
	int MessageLength;
	unsigned char Checksum;
	unsigned char *MessageBuffer;
} M2BUS_IncomingMessage;


typedef struct{
	M2BUS_IncomingMessage i;
	u8 RequestSequenceNumber;
} M2BUS_Link;


GSM_Error M2BUS_Initialise(GSM_Link *newlink, GSM_Statemachine *state);

#endif   /* #ifndef __links_m2bus_h */
