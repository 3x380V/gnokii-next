/*

  $Id: fbus.c,v 1.32 2002-12-10 11:05:56 ladis Exp $

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

  This file provides an API for accessing functions via fbus.
  See README for more details on supported mobile phones.

  The various routines are called FBUS_(whatever).

*/

/* System header files */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Various header file */

#include "config.h"
#include "misc.h"
#include "gsm-common.h"
#include "gsm-ringtones.h"
#include "gsm-networks.h"
#include "gsm-statemachine.h"
#include "device.h"
#include "links/fbus.h"

#include "gnokii-internal.h"

static void fbus_rx_statemachine(unsigned char rx_byte);
static gn_error fbus_send_message(u16 messagesize, u8 messagetype, unsigned char *message);
static int fbus_tx_send_ack(u8 message_type, u8 message_seq);

/* FIXME - pass device_* the link stuff?? */
/* FIXME - win32 stuff! */

/* Some globals */

static gn_link *glink;
static struct gn_statemachine *statemachine;
static fbus_link flink;		/* FBUS specific stuff, internal to this file */


/*--------------------------------------------*/

static bool fbus_serial_open(bool dlr3)
{
	if (dlr3) dlr3 = 1;
	/* Open device. */
	if (!device_open(glink->port_device, false, false, false, GN_CT_Serial)) {
		perror(_("Couldn't open FBUS device"));
		return false;
	}
	device_changespeed(115200);

	/* clearing the RTS bit and setting the DTR bit */
	device_setdtrrts((1-dlr3), 0);

	return true;
}

static bool at2fbus_serial_open()
{
	unsigned char init_char = 0x55;
	unsigned char end_init_char = 0xc1;
	int count, res;
	unsigned char buffer[255];
 
	/* Open device. */
	if (!device_open(glink->port_device, false, false, false, GN_CT_Serial)) {
		perror(_("Couldn't open FBUS device"));
		return false;
	}
 
	device_setdtrrts(0, 0);
	sleep(1);
	device_setdtrrts(1, 1);
	device_changespeed(19200);
	sleep(1);
	device_write("AT\r", 3);
	sleep(1);
	res = device_read(buffer, 255);
	device_write("AT&F\r", 5);
	usleep(100000);
	res = device_read(buffer, 255);
	device_write("AT*NOKIAFBUS\r", 13);
	usleep(100000);
	res = device_read(buffer, 255);
 
	device_changespeed(115200);
 
	for (count = 0; count < 32; count++) {
		device_write(&init_char, 1);
	}
	device_write(&end_init_char, 1);
	usleep(1000000);
 
	return true;
}

static bool fbus_ir_open(void)
{
	struct timeval timeout;
	unsigned char init_char = 0x55;
	unsigned char end_init_char = 0xc1;
	unsigned char connect_seq[] = { FBUS_FRAME_HEADER, 0x0d, 0x00, 0x00, 0x02 };
	unsigned int count, retry;

	if (!device_open(glink->port_device, false, false, false, GN_CT_Infrared)) {
		perror(_("Couldn't open FBUS device"));
		return false;
	}

	/* clearing the RTS bit and setting the DTR bit */
	device_setdtrrts(1, 0);

	for (retry = 0; retry < 5; retry++) {
		dprintf("IR init, retry=%d\n", retry);

		device_changespeed(9600);

		for (count = 0; count < 32; count++) {
			device_write(&init_char, 1);
		}
		device_write(&end_init_char, 1);
		usleep(100000);

		device_changespeed(115200);

		fbus_send_message(7, 0x02, connect_seq);

		/* Wait for 1 sec. */
		timeout.tv_sec	= 1;
		timeout.tv_usec	= 0;

		if (device_select(&timeout)) {
			dprintf("IR init succeeded\n");
			return true;
		}
	}

	return false;
}


/* RX_State machine for receive handling.  Called once for each character
   received from the phone. */

static void fbus_rx_statemachine(unsigned char rx_byte)
{
	struct timeval time_diff;
	fbus_incoming_frame *i = &flink.i;
	int frm_num, seq_num;
	fbus_incoming_message *m;

	/* XOR the byte with the current checksum */
	i->checksum[i->buffer_count & 1] ^= rx_byte;

	switch (i->state) {
		/* Messages from the phone start with an 0x1e (cable) or 0x1c (IR).
		   We use this to "synchronise" with the incoming data stream. However,
		   if we see something else, we assume we have lost sync and we require
		   a gap of at least 5ms before we start looking again. This is because
		   the data part of the frame could contain a byte which looks like the
		   sync byte */

	case FBUS_RX_Discarding:
		gettimeofday(&i->time_now, NULL);
		timersub(&i->time_now, &i->time_last, &time_diff);
		if (time_diff.tv_sec == 0 && time_diff.tv_usec < 5000) {
			i->time_last = i->time_now;	/* no gap seen, continue discarding */
			break;
		}

		/* else fall through to... */

	case FBUS_RX_Sync:
		if (glink->connection_type == GN_CT_Infrared) {
			if (rx_byte == FBUS_IR_FRAME_ID) {
				/* Initialize checksums. */
				i->checksum[0] = FBUS_IR_FRAME_ID;
				i->checksum[1] = 0;
				i->state = FBUS_RX_GetDestination;
			} else {
				/* Lost frame sync */
				i->state = FBUS_RX_Discarding;
				gettimeofday(&i->time_last, NULL);
			}

		} else {	/* glink->ConnectionType == GCT_Serial */
			if (rx_byte == FBUS_FRAME_ID) {
				/* Initialize checksums. */
				i->checksum[0] = FBUS_FRAME_ID;
				i->checksum[1] = 0;
				i->state = FBUS_RX_GetDestination;
			} else {
				/* Lost frame sync */
				i->state = FBUS_RX_Discarding;
				gettimeofday(&i->time_last, NULL);
			}
		}

		break;

	case FBUS_RX_GetDestination:

		i->message_destination = rx_byte;
		i->state = FBUS_RX_GetSource;

		/* When there is a checksum error and things get out of sync we have to manage to resync */
		/* If doing a data call at the time, finding a 0x1e etc is really quite likely in the data stream */
		/* Then all sorts of horrible things happen because the packet length etc is wrong... */
		/* Therefore we test here for a destination of 0x0c and return to the top if it is not */

		if (rx_byte != 0x0c) {
			i->state = FBUS_RX_Sync;
			dprintf("The fbus stream is out of sync - expected 0x0c, got %2x\n", rx_byte);
		}

		break;

	case FBUS_RX_GetSource:

		i->message_source = rx_byte;
		i->state = FBUS_RX_GetType;

		/* Source should be 0x00 */

		if (rx_byte != 0x00) {
			i->state = FBUS_RX_Sync;
			dprintf("The fbus stream is out of sync - expected 0x00, got %2x\n",rx_byte);
		}

		break;

	case FBUS_RX_GetType:

		i->message_type = rx_byte;
		i->state = FBUS_RX_GetLength1;

		break;

	case FBUS_RX_GetLength1:

		i->frame_length = rx_byte << 8;
		i->state = FBUS_RX_GetLength2;

		break;

	case FBUS_RX_GetLength2:

		i->frame_length = i->frame_length + rx_byte;
		i->state = FBUS_RX_GetMessage;
		i->buffer_count = 0;

		break;

	case FBUS_RX_GetMessage:

		if (i->buffer_count >= FBUS_FRAME_MAX_LENGTH) {
			dprintf("FBUS: Message buffer overun - resetting\n");
			i->state = FBUS_RX_Sync;
			break;
		}

		i->message_buffer[i->buffer_count] = rx_byte;
		i->buffer_count++;

		/* If this is the last byte, it's the checksum. */

		if (i->buffer_count == i->frame_length + (i->frame_length % 2) + 2) {
			/* Is the checksum correct? */
			if (i->checksum[0] == i->checksum[1]) {

				/* Deal with exceptions to the rules - acks and rlp.. */

				if (i->message_type == 0x7f) {
					dprintf("[Received Ack of type %02x, seq: %2x]\n",
						i->message_buffer[0], (unsigned char) i->message_buffer[1]);

				} else if (i->message_type == 0xf1) {
					sm_incoming_function(statemachine, i->message_type,
							     i->message_buffer, i->frame_length - 2);
				} else {	/* Normal message type */

					/* Add data to the relevant Message buffer */
					/* having checked the sequence number */

					m = &flink.messages[i->message_type];

					frm_num = i->message_buffer[i->frame_length - 2];
					seq_num = i->message_buffer[i->frame_length - 1];


					/* 0x40 in the sequence number indicates first frame of a message */

					if ((seq_num & 0x40) == 0x40) {
						/* Fiddle around and malloc some memory */
						m->message_length = 0;
						m->frames_to_go = frm_num;
						if (m->malloced != 0) {
							free(m->message_buffer);
							m->malloced = 0;
							m->message_buffer = NULL;
						}
						m->malloced = frm_num * m->message_length;
						m->message_buffer = (char *) malloc(m->malloced);

					} else if (m->frames_to_go != frm_num) {
						dprintf("Missed a frame in a multiframe message.\n");
						/* FIXME - we should make sure we don't ack the rest etc */
					}

					if (m->malloced < m->message_length + i->frame_length) {
						m->malloced = m->message_length + i->frame_length;
						m->message_buffer = (char *) realloc(m->message_buffer, m->malloced);
					}

					memcpy(m->message_buffer + m->message_length, i->message_buffer,
					       i->frame_length - 2);

					m->message_length += i->frame_length - 2;

					m->frames_to_go--;

					/* Send an ack (for all for now) */

					fbus_tx_send_ack(i->message_type, seq_num & 0x0f);

					/* Finally dispatch if ready */

					if (m->frames_to_go == 0) {
						sm_incoming_function(statemachine, i->message_type, m->message_buffer, m->message_length);
						free(m->message_buffer);
						m->message_buffer = NULL;
						m->malloced = 0;
					}
				}
			} else {
				dprintf("Bad checksum!\n");
			}
			i->state = FBUS_RX_Sync;
		}
		break;
	}
}


/* This is the main loop function which must be called regularly */
/* timeout can be used to make it 'busy' or not */

static gn_error fbus_loop(struct timeval *timeout)
{
	unsigned char buffer[255];
	int count, res;

	res = device_select(timeout);
	if (res > 0) {
		res = device_read(buffer, 255);
		for (count = 0; count < res; count++)
			fbus_rx_statemachine(buffer[count]);
	} else
		return GN_ERR_TIMEOUT;

	/* This traps errors from device_read */
	if (res > 0)
		return GN_ERR_NONE;
	else
		return GN_ERR_INTERNALERROR;
}



/* Prepares the message header and sends it, prepends the message start byte
	   (0x1e) and other values according the value specified when called.
	   Calculates checksum and then sends the lot down the pipe... */

int fbus_tx_send_frame(u8 message_length, u8 message_type, u8 * buffer)
{
	u8 out_buffer[FBUS_TRANSMIT_MAX_LENGTH + 5];
	int count, current = 0;
	unsigned char checksum;

	/* FIXME - we should check for the message length ... */

	/* Now construct the message header. */

	if (glink->connection_type == GN_CT_Infrared)
		out_buffer[current++] = FBUS_IR_FRAME_ID;	/* Start of the IR frame indicator */
	else			/* connection_type == GN_CT_Serial */
		out_buffer[current++] = FBUS_FRAME_ID;	/* Start of the frame indicator */

	out_buffer[current++] = FBUS_DEVICE_PHONE;	/* Destination */
	out_buffer[current++] = FBUS_DEVICE_PC;	/* Source */

	out_buffer[current++] = message_type;	/* Type */

	out_buffer[current++] = 0;	/* Length */

	out_buffer[current++] = message_length;	/* Length */

	/* Copy in data if any. */

	if (message_length != 0) {
		memcpy(out_buffer + current, buffer, message_length);
		current += message_length;
	}

	/* If the message length is odd we should add pad byte 0x00 */
	if (message_length % 2)
		out_buffer[current++] = 0x00;

	/* Now calculate checksums over entire message and append to message. */

	/* Odd bytes */

	checksum = 0;
	for (count = 0; count < current; count += 2)
		checksum ^= out_buffer[count];

	out_buffer[current++] = checksum;

	/* Even bytes */

	checksum = 0;
	for (count = 1; count < current; count += 2)
		checksum ^= out_buffer[count];

	out_buffer[current++] = checksum;

	/* Send it out... */

	if (device_write(out_buffer, current) != current)
		return false;

	return true;
}


/* Main function to send an fbus message */
/* Splits up the message into frames if necessary */

static gn_error fbus_send_message(u16 messagesize, u8 messagetype, unsigned char *message)
{
	u8 seqnum, frame_buffer[FBUS_CONTENT_MAX_LENGTH + 2];
	u8 nom, lml;		/* number of messages, last message len */
	int i;

	seqnum = 0x40 + flink.request_sequence_number;
	flink.request_sequence_number =
	    (flink.request_sequence_number + 1) & 0x07;

	if (messagesize > FBUS_CONTENT_MAX_LENGTH) {

		nom = (messagesize + FBUS_CONTENT_MAX_LENGTH - 1)
		    / FBUS_CONTENT_MAX_LENGTH;
		lml = messagesize - ((nom - 1) * FBUS_CONTENT_MAX_LENGTH);

		for (i = 0; i < nom - 1; i++) {

			memcpy(frame_buffer, message + (i * FBUS_CONTENT_MAX_LENGTH),
			       FBUS_CONTENT_MAX_LENGTH);
			frame_buffer[FBUS_CONTENT_MAX_LENGTH] = nom - i;
			frame_buffer[FBUS_CONTENT_MAX_LENGTH + 1] = seqnum;

			fbus_tx_send_frame(FBUS_CONTENT_MAX_LENGTH + 2,
					  messagetype, frame_buffer);

			seqnum = flink.request_sequence_number;
			flink.request_sequence_number = (flink.request_sequence_number + 1) & 0x07;
		}

		memcpy(frame_buffer, message + ((nom - 1) * FBUS_CONTENT_MAX_LENGTH), lml);
		frame_buffer[lml] = 0x01;
		frame_buffer[lml + 1] = seqnum;
		fbus_tx_send_frame(lml + 2, messagetype, frame_buffer);

	} else {

		memcpy(frame_buffer, message, messagesize);
		frame_buffer[messagesize] = 0x01;
		frame_buffer[messagesize + 1] = seqnum;
		fbus_tx_send_frame(messagesize + 2, messagetype,
				  frame_buffer);
	}
	return GN_ERR_NONE;
}


static int fbus_tx_send_ack(u8 message_type, u8 message_seq)
{
	unsigned char request[2];

	request[0] = message_type;
	request[1] = message_seq;

	dprintf("[Sending Ack of type %02x, seq: %x]\n",message_type, message_seq);

	return fbus_tx_send_frame(2, 0x7f, request);
}


/* Initialise variables and start the link */
/* newlink is actually part of state - but the link code should not anything about state */
/* state is only passed around to allow for muliple state machines (one day...) */

gn_error fbus_initialise(gn_link *newlink, struct gn_statemachine *state, int try)
{
	unsigned char init_char = 0x55;
	int count;
	bool err;

	/* 'Copy in' the global structures */
	glink = newlink;
	statemachine = state;

	/* Fill in the link functions */
	glink->loop = &fbus_loop;
	glink->send_message = &fbus_send_message;

	/* Check for a valid init length */
	if (glink->init_length == 0)
		glink->init_length = 250;

	/* Start up the link */
	flink.request_sequence_number = 0;

	switch (glink->connection_type) {
	case GN_CT_Infrared:
		if (!fbus_ir_open())
			return GN_ERR_FAILED;
		break;
	case GN_CT_Serial:
		switch (try) {
		case 0:
		case 1:
			err = fbus_serial_open(1 - try);
			break;
		case 2:
			err = at2fbus_serial_open();
			break;
		default:
			return GN_ERR_FAILED;
		}
		if (!err) return GN_ERR_FAILED;
		break;
	case GN_CT_DAU9P:
		if (!fbus_serial_open(0))
			return GN_ERR_FAILED;
		break;
	case GN_CT_DLR3P:
		switch (try) {
		case 0:
			err = at2fbus_serial_open();
			break;
		case 1:
			err = fbus_serial_open(1);
			break;
		default:
			return GN_ERR_FAILED;
		}
		if (!err) return GN_ERR_FAILED;
		break;
	default:
		return GN_ERR_FAILED;
	}

	/* Send init string to phone, this is a bunch of 0x55 characters. Timing is
	   empirical. */
	/* I believe that we need/can do this for any phone to get the UART synced */

	for (count = 0; count < glink->init_length; count++) {
		usleep(100);
		device_write(&init_char, 1);
	}

	/* Init variables */
	flink.i.state = FBUS_RX_Sync;
	flink.i.buffer_count = 0;
	for (count = 0; count < FBUS_MESSAGE_MAX_TYPES; count++) {
		flink.messages[count].malloced = 0;
		flink.messages[count].frames_to_go = 0;
		flink.messages[count].message_length = 0;
		flink.messages[count].message_buffer = NULL;
	}

	return GN_ERR_NONE;
}
