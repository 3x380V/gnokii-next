/* -*- linux-c -*-

  $Id: cbus.c,v 1.18 2002-09-28 23:51:37 pkot Exp $

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

  Copyright (C) 2001 Pavel Machek <pavel@ucw.cz>
  Copyright (C) 2001 Michl Ladislav <xmichl03@stud.fee.vutbr.cz>

 */

/* System header files */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* Various header file */
#include "config.h"
#ifndef DEBUG
#define DEBUG
#endif
#include "misc.h"
#include "gsm-data.h"
#include "gsm-ringtones.h"
#include "gsm-networks.h"
#include "device.h"

#include "links/cbus.h"

static gn_error CBUS_Loop(struct timeval *timeout);
static bool CBUS_OpenSerial();
static void CBUS_RX_StateMachine(unsigned char rx_byte);
static int CBUS_SendMessage(u16 messagesize, u8 messagetype, unsigned char *message);

/* FIXME - pass device_* the link stuff?? */
/* FIXME - win32 stuff! */


/* Some globals */

static GSM_Link *glink;
static GSM_Phone *gphone;
static CBUS_Link clink;	/* CBUS specific stuff, internal to this file */

static int init_okay = 0;
int seen_okay;
char reply_buf[10240];

/*--------------------------------------------------------------------------*/

static bool CBUS_OpenSerial()
{
	int result;
	result = device_open(glink->PortDevice, false, false, false, GCT_Serial);
	if (!result) {
		perror(_("Couldn't open CBUS device"));
		return (false);
	}
	device_changespeed(9600);
	device_setdtrrts(1, 0);
	return (true);
}

/* -------------------------------------------------------------------- */

static int xread(unsigned char *d, int len)
{
	int res;
	while (len) {
		res = device_read(d, len);
		if (res == -1) {
			if (errno != EAGAIN) {
				dprintf("I/O error : %m?!\n");
				return -1;
			} else device_select(NULL);
		} else {
			d += res;
			len -= res;
			dprintf("(%d)", len);
		}
	}
	return 0;
}

static int xwrite(unsigned char *d, int len)
{
	int res;
	while (len) {
		res = device_write(d, len);
		if (res == -1) {
			if (errno != EAGAIN) {
				dprintf("I/O error : %m?!\n");
				return -1;
			}
		} else {
			d += res;
			len -= res;
			dprintf("<%d>", len);
		}
	}
	return 0;
}

static void say(unsigned char *c, int len)
{
	unsigned char d[10240];

	xwrite(c, len);
	xread(d, len);
	if (memcmp(c, d, len)) {
		int i;
		dprintf("Did not get my own echo?: ");
		for (i = 0; i < len; i++)
			dprintf("%x ", d[i]);
		dprintf("\n");
	}
}

static int waitack(void)
{
	unsigned char c;
	dprintf("Waiting ack\n");
	if (xread(&c, 1) == 0) {
		dprintf("Got %x\n", c);
		if (c != 0xa5)
			dprintf("???\n");
		else return 1;
	} else dprintf("Timeout?\n");
	return 0;
}

static void sendpacket(unsigned char *msg, int len, unsigned short cmd)
{
	unsigned char p[10240], csum = 0;
	int pos;

	p[0] = 0x34;
	p[1] = 0x19;
	p[2] = cmd & 0xff;
	p[3] = 0x68;
	p[4] = len & 0xff;
	p[5] = len >> 8;
	memcpy(p+6, msg, len);

	pos = 6+len;
	{
		int i;
		for (i = 0; i < pos; i++) {
			csum = csum ^ p[i];
		}
	}
	p[pos] = csum;
	{
		int i;
		dprintf("Sending: ");
		for (i = 0; i <= pos; i++) {
			dprintf("%x ", p[i]);
		}
		dprintf("\n");
	}
	say(p, pos+1);
	waitack();
}

/* -------------------------------------------------------------------- */


static gn_error CommandAck(int messagetype, unsigned char *buffer, int length)
{
	dprintf("[ack]");
	return GN_ERR_NONE;
}

static gn_error PhoneReply(int messagetype, unsigned char *buffer, int length)
{
	if (!strncmp(buffer, "OK", 2)) {
		seen_okay = 1;
		dprintf("Phone okays\n");
	} else {
		strncpy(reply_buf, buffer, length);
		reply_buf[length+1] = '\0';
		dprintf("Phone says: %s\n", reply_buf);
	}
	{
		u8 buf[2] = { 0x3e, 0x68 };
		usleep(10000);
		CBUS_SendMessage(2, 0x3f, buf);
	}

	return GN_ERR_NONE;
}

void sendat(unsigned char *msg)
{
	usleep(10000);
	dprintf("AT command: %s\n", msg);
	CBUS_SendMessage(strlen(msg), 0x3c, msg);
	seen_okay = 0;
	while (!seen_okay)
		CBUS_Loop(NULL);
//	getpacket();	/* This should be phone's acknowledge of AT command */
}


/* -------------------------------------------------- */

/* RX_State machine for receive handling.  Called once for each character
   received from the phone. */

static void internal_dispatch(GSM_Link *glink, GSM_Phone *gphone, int type, u8 *buf, int len)
{
	switch(type) {
	case '=': CommandAck(type, buf, len);
		break;
	case '>': PhoneReply(type, buf, len);
		break;
	default: dprintf("Unknown Frame Type 68/ %02x\n", type);
		break;
	}
}

static void CBUS_RX_StateMachine(unsigned char rx_byte)
{
	CBUS_IncomingFrame *i = &clink.i;

	/* checksum is XOR of all bytes in the frame */
	if (i->state != CBUS_RX_GetCSum) i->checksum ^= rx_byte;

	switch (i->state) {

	case CBUS_RX_Header:
		switch (rx_byte) {
			case 0x38:
			case 0x34:
				if (i->prev_rx_byte == 0x19) {
					i->state = CBUS_RX_FrameType1;
				}
				break;
			case 0x19:
				if ((i->prev_rx_byte == 0x38) || (i->prev_rx_byte == 0x34)) {
					i->state = CBUS_RX_FrameType1;
				}
				break;
			default:
				break;
		}
		if (i->state != CBUS_RX_Header) {
			i->FrameHeader1 = i->prev_rx_byte;
			i->FrameHeader2 = rx_byte;
			i->BufferCount = 0;
			i->checksum = i->prev_rx_byte ^ rx_byte;
		}
		break;

	/* FIXME: Do you know exact meaning? just mail me. ladis. */
	case CBUS_RX_FrameType1:
		i->FrameType1 = rx_byte;
		i->state = CBUS_RX_FrameType2;
		break;

	/* FIXME: Do you know exact meaning? just mail me. ladis. */
	case CBUS_RX_FrameType2:
		i->FrameType2 = rx_byte;
		i->state = CBUS_RX_GetLengthLB;
		break;

	/* message length - low byte */
	case CBUS_RX_GetLengthLB:
		i->MessageLength = rx_byte;
		i->state = CBUS_RX_GetLengthHB;
		break;

	/* message length - high byte */
	case CBUS_RX_GetLengthHB:
		i->MessageLength = i->MessageLength | rx_byte << 8;
		/* there are also empty commands */
		if (i->MessageLength == 0)
			i->state = CBUS_RX_GetCSum;
		else
			i->state = CBUS_RX_GetMessage;
		break;

	/* get each byte of the message body */
	case CBUS_RX_GetMessage:
		i->buffer[i->BufferCount++] = rx_byte;
		/* avoid buffer overflow */
		if (i->BufferCount > CBUS_MAX_MSG_LENGTH) {
			dprintf("CBUS: Message buffer overun - resetting\n");
			i->state = CBUS_RX_Header;
			break;
		}

		if (i->BufferCount == i->MessageLength)
			i->state = CBUS_RX_GetCSum;
		break;

	/* get checksum */
	case CBUS_RX_GetCSum:
		/* compare against calculated checksum. */
		if (i->checksum == rx_byte) {
			u8 ack = 0xa5;

			xwrite(&ack, 1);
			xread(&ack, 1);
			if (ack != 0xa5)
				dprintf("ack lost, expect armagedon!\n");

			/* Got checksum, matches calculated one, so
			 * now pass to appropriate dispatch handler. */
			i->buffer[i->MessageLength + 1] = 0;
			/* FIXME: really call it :-) */

			switch(i->FrameType2) {
			case 0x68:
				internal_dispatch(glink, gphone, i->FrameType1, i->buffer, i->MessageLength);
				break;
			case 0x70:
				if (i->FrameType1 == 0x91) {
					init_okay = 1;
					dprintf("Registration acknowledged\n");
				} else
					dprintf("Unknown message\n");
			}
		} else {
			/* checksum doesn't match so ignore. */
			dprintf("CBUS: Checksum error; expected: %02x, got: %02x", i->checksum, rx_byte);
		}

		i->state = CBUS_RX_Header;
		break;

	default:
		break;
	}
	i->prev_rx_byte = rx_byte;
}

static int CBUS_SendMessage(u16 message_length, u8 message_type, unsigned char *buffer)
{
	sendpacket(buffer, message_length, message_type);
	return true;
}

static gn_error AT_SendMessage(u16 message_length, u8 message_type, unsigned char *buffer)
{
	sendat(buffer);
	return true;
}

/* This is the main loop function which must be called regularly */
/* timeout can be used to make it 'busy' or not */

/* ladis: this function ought to be the same for all phones... */

static gn_error CBUS_Loop(struct timeval *timeout)
{
	unsigned char buffer[255];
	int count, res;

	res = device_select(timeout);
	if (res > 0) {
		res = device_read(buffer, 255);
		for (count = 0; count < res; count++)
			CBUS_RX_StateMachine(buffer[count]);
	} else
		return GN_ERR_TIMEOUT;

	/* This traps errors from device_read */
	if (res > 0)
		return GN_ERR_NONE;
	else
		return GN_ERR_INTERNALERROR;
}


/* Initialise variables and start the link */

gn_error CBUS_Initialise(GSM_Statemachine *state)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	/* 'Copy in' the global structures */
	glink = &(state->Link);
	gphone = &(state->Phone);

	/* Fill in the link functions */
	glink->Loop = &CBUS_Loop;
	glink->SendMessage = &AT_SendMessage;

	if (glink->ConnectionType == GCT_Serial) {
		if (!CBUS_OpenSerial())
			return GN_ERR_FAILED;
	} else {
		dprintf("Device not supported by CBUS");
		return GN_ERR_FAILED;
	}

	dprintf("Saying hello...");
	{
		char init1[] = { 0, 0x38, 0x19, 0xa6, 0x70, 0x00, 0x00, 0xf7, 0xa5 };
		say(init1, 8);
		say(init1, 8);
	}
	usleep(10000);
	dprintf("second hello...");
	{
		char init1[] = { 0x38, 0x19, 0x90, 0x70, 0x01, 0x00, 0x1f, 0xdf };
		say(init1, 8);
		if (!waitack())
			return GN_ERR_BUSY;
	}
	while (!init_okay)
		CBUS_Loop(NULL);
	return GN_ERR_NONE;
}
