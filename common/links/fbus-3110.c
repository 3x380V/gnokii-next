/*

  $Id: fbus-3110.c,v 1.12 2002-12-10 11:05:56 ladis Exp $

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

  This file provides an API for accessing functions via fbus.
  See README for more details on supported mobile phones.

  The various routines are called fb3110_(whatever).

*/

/* System header files */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Various header file */

#include "config.h"
#include "misc.h"
#include "gnokii-internal.h"
#include "device.h"

#include "links/fbus-3110.h"

static bool fb3110_serial_open(void);
static void fb3110_rx_state_machine(unsigned char rx_byte);
static gn_error fb3110_tx_frame_send(u8 message_length, u8 message_type, u8 sequence_byte, u8 *buffer);
static gn_error fb3110_message_send(u16 messagesize, u8 messagetype, unsigned char *message);
static void fb3110_tx_ack_send(u8 *message, int length);
static void fb3110_sequence_number_update(void);

/* FIXME - pass device_* the link stuff?? */
/* FIXME - win32 stuff! */


/* Some globals */

static gn_link *glink;
static struct gn_statemachine *statemachine;
static fb3110_link flink;	/* FBUS specific stuff, internal to this file */


/*--------------------------------------------*/

static bool fb3110_serial_open(void)
{
	/* Open device. */
	if (!device_open(glink->port_device, false, false, false, GN_CT_Serial)) {
		perror(_("Couldn't open FBUS device"));
		return false;
	}
	device_changespeed(115200);

	return true;
}


/* 
 * RX_State machine for receive handling.
 * Called once for each character received from the phone.
 */
static void fb3110_rx_state_machine(unsigned char rx_byte)
{
	fb3110_incoming_frame *i = &flink.i;
	int count;

	switch (i->state) {

	/* Phone is currently off.  Wait for 0x55 before
	   restarting */
	case FB3110_RX_Discarding:
		if (rx_byte != 0x55)
			break;

		/* Seen 0x55, restart at 0x04 */
		i->state = FB3110_RX_Sync;

		dprintf("restarting.\n");

		/* FALLTHROUGH */

	/* Messages from the phone start with an 0x04 during
	   "normal" operation, 0x03 when in data/fax mode.  We
	   use this to "synchronise" with the incoming data
	   stream. */
	case FB3110_RX_Sync:
		if (rx_byte == 0x04 || rx_byte == 0x03) {
			i->frame_type = rx_byte;
			i->checksum = rx_byte;
			i->state = FB3110_RX_GetLength;
		}
		break;

	/* Next byte is the length of the message including
	   the message type byte but not including the checksum. */
	case FB3110_RX_GetLength:
		i->frame_len = rx_byte;
		i->buffer_count = 0;
		i->checksum ^= rx_byte;
		i->state = FB3110_RX_GetMessage;
		break;

	/* Get each byte of the message.  We deliberately
	   get one too many bytes so we get the checksum
	   here as well. */
	case FB3110_RX_GetMessage:
		i->buffer[i->buffer_count] = rx_byte;
		i->buffer_count++;

		if (i->buffer_count > FB3110_FRAME_MAX_LENGTH) {
			dprintf("FBUS: Message buffer overun - resetting\n");
			i->state = FB3110_RX_Sync;
			break;
		}

		/* If this is the last byte, it's the checksum. */
		if (i->buffer_count > i->frame_len) {

			/* Is the checksum correct? */
			if (rx_byte == i->checksum) {

				if (i->frame_type == 0x03) {
					/* FIXME: modify Buffer[0] to code FAX frame types */
				}

				dprintf("--> %02x:%02x:", i->frame_type, i->frame+len);
				for (count = 0; count < i->buffer_count; count++)
					dprintf("%02hhx:", i->buffer[count]);
				dprintf("\n");
				/* Transfer message to state machine */
				sm_incoming_function(statemachine, i->buffer[0], i->buffer, i->frame_len);

				/* Send an ack */
				fb3110_tx_ack_send(i->buffer, i->frame_len);

			} else {
				/* Checksum didn't match so ignore. */
				dprintf("Bad checksum!\n");
			}
			i->state = FB3110_RX_Sync;
		}
		i->checksum ^= rx_byte;
		break;
	}
}


/* 
 * This is the main loop function which must be called regularly
 * timeout can be used to make it 'busy' or not 
 */
static gn_error fb3110_loop(struct timeval *timeout)
{
	unsigned char buffer[255];
	int count, res;

	res = device_select(timeout);
	if (res > 0) {
		res = device_read(buffer, 255);
		for (count = 0; count < res; count++)
			fb3110_rx_state_machine(buffer[count]);
	} else
		return GN_ERR_TIMEOUT;

	/* This traps errors from device_read */
	if (res > 0)
		return GN_ERR_NONE;
	else
		return GN_ERR_INTERNALERROR;
}



/* 
 * Prepares the message header and sends it, prepends the message start
 * byte (0x01) and other values according the value specified when called.
 * Calculates checksum and then sends the lot down the pipe... 
 */
static gn_error fb3110_tx_frame_send(u8 message_length, u8 message_type, u8 sequence_byte, u8 *buffer)
{

	u8 out_buffer[FB3110_TRANSMIT_MAX_LENGTH];
	int count, current = 0;
	unsigned char checksum;

	/* Check message isn't too long, once the necessary
	   header and trailer bytes are included. */
	if ((message_length + 5) > FB3110_TRANSMIT_MAX_LENGTH) {
		fprintf(stderr, _("fb3110_tx_frame_send - message too long!\n"));
		return GN_ERR_INTERNALERROR;
	}

	/* Now construct the message header. */
	out_buffer[current++] = FB3110_FRAME_ID;    /* Start of frame */
	out_buffer[current++] = message_length + 2; /* Length */
	out_buffer[current++] = message_type;       /* Type */
	out_buffer[current++] = sequence_byte;      /* Sequence number */

	/* Copy in data if any. */
	if (message_length != 0) {
		memcpy(out_buffer + current, buffer, message_length);
		current += message_length;
	}

	/* Now calculate checksum over entire message
	   and append to message. */
	checksum = 0;
	for (count = 0; count < current; count++)
		checksum ^= out_buffer[count];
	out_buffer[current++] = checksum;

	dprintf("<-- ");
	for (count = 0; count < current; count++)
		dprintf("%02hhx:", out_buffer[count]);
	dprintf("\n");

	/* Send it out... */
	if (device_write(out_buffer, current) != current)
		return GN_ERR_INTERNALERROR;

	return GN_ERR_NONE;
}


/* 
 * Main function to send an fbus message 
 */
static gn_error fb3110_message_send(u16 messagesize, u8 messagetype, unsigned char *message)
{
	u8 seqnum;

	fb3110_sequence_number_update();
	seqnum = flink.request_sequence_number;

	return fb3110_tx_frame_send(messagesize, messagetype, seqnum, message);
}


/* 
 * Sends the "standard" acknowledge message back to the phone in response to
 * a message it sent automatically or in response to a command sent to it.
 * The ack algorithm isn't 100% understood at this time. 
 */
static void fb3110_tx_ack_send(u8 *message, int length)
{
	u8 t = message[0];

	switch (t) {
	case 0x0a:
		/* We send 0x0a messages to make a call so don't ack. */
	case 0x0c:
		/* We send 0x0c message to answer to incoming call
		   so don't ack */
	case 0x0f:
		/* We send 0x0f message to hang up so don't ack */
	case 0x15:
		/* 0x15 messages are sent by the phone in response to the
		   init sequence sent so we don't acknowledge them! */
	case 0x20:
		/* We send 0x20 message to phone to send DTFM, so don't ack */
	case 0x23:
		/* We send 0x23 messages to phone as a header for outgoing SMS
		   messages.  So we don't acknowledge it. */
	case 0x24:
		/* We send 0x24 messages to phone as a header for storing SMS
		   messages in memory. So we don't acknowledge it. :) */
	case 0x25:
		/* We send 0x25 messages to phone to request an SMS message
		   be dumped.  Thus we don't acknowledge it. */
	case 0x26:
		/* We send 0x26 messages to phone to delete an SMS message
		   so it's not acknowledged. */
	case 0x3f:
		/* We send an 0x3f message to the phone to request a different
		   type of status dump - this one seemingly concerned with
		   SMS message center details.  Phone responds with an ack to
		   our 0x3f request then sends an 0x41 message that has the
		   actual data in it. */
	case 0x4a:
		/* 0x4a message is a response to our 0x4a request, assumed to
		   be a keepalive message of sorts.  No response required. */
	case 0x4c:
		/* We send 0x4c to request IMEI, Revision and Model info. */
		break;
	case 0x27:
		/* 0x27 messages are a little different in that both ends of
		   the link send them.  So, first we have to check if this
		   is an acknowledgement or a message to be acknowledged */
		if (length == 0x02) break;
	default:
		/* Standard acknowledge seems to be to return an empty message
		   with the sequence number set to equal the sequence number
		   sent minus 0x08. */
		if (fb3110_tx_frame_send(0, t, (message[1] & 0x1f) - 0x08, NULL))
			dprintf("Failed to acknowledge message type %02x.\n", t);
		else
			dprintf("Acknowledged message type %02x.\n", t);
	}
}


/* 
 * Initialise variables and start the link
 * newlink is actually part of state - but the link code should not
 * anything about state. State is only passed around to allow for
 * muliple state machines (one day...)
 */
gn_error fb3110_initialise(gn_link *newlink, struct gn_statemachine *state)
{
	unsigned char init_char = 0x55;
	unsigned char count;
	static int try = 0;

	try++;
	if (try > 2) return GN_ERR_FAILED;
	/* 'Copy in' the global structures */
	glink = newlink;
	statemachine = state;

	/* Fill in the link functions */
	glink->loop = &fb3110_loop;
	glink->send_message = &fb3110_message_send;

	/* Check for a valid init length */
	if (glink->init_length == 0)
		glink->init_length = 100;

	/* Start up the link */

	flink.request_sequence_number = 0x10;

	if (!fb3110_serial_open()) return GN_ERR_FAILED;

	/* Send init string to phone, this is a bunch of 0x55 characters.
	   Timing is empirical. I believe that we need/can do this for any
	   phone to get the UART synced */
	for (count = 0; count < glink->init_length; count++) {
		usleep(1000);
		device_write(&init_char, 1);
	}

	/* Init variables */
	flink.i.state = FB3110_RX_Sync;

	return GN_ERR_NONE;
}


/* Any command we originate must have a unique SequenceNumber. Observation to
 * date suggests that these values startx at 0x10 and cycle up to 0x17
 * before repeating again. Perhaps more accurately, the numbers cycle
 * 0,1,2,3..7 with bit 4 of the byte premanently set. 
 */
static void fb3110_sequence_number_update(void)
{
	flink.request_sequence_number++;

	if (flink.request_sequence_number > 0x17 || flink.request_sequence_number < 0x10)
		flink.request_sequence_number = 0x10;
}
