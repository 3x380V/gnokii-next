/*

  $Id: gsm-statemachine.c,v 1.34 2002-12-09 00:24:16 pkot Exp $

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

#include "misc.h"
#include "gsm-common.h"
#include "gsm-statemachine.h"

#include "gnokii-internal.h"
#include "gnokii-api.h"

gn_error SM_Initialise(struct gn_statemachine *state)
{
	state->current_state = GN_SM_Initialised;
	state->waiting_for_number = 0;
	state->received_number = 0;

	return GN_ERR_NONE;
}

gn_error SM_SendMessage(struct gn_statemachine *state, u16 messagesize, u8 messagetype, void *message)
{
	if (state->current_state != GN_SM_Startup) {
#ifdef	DEBUG
	dump("Message sent: ");
	sm_message_dump(messagetype, message, messagesize);
#endif
		state->last_msg_size = messagesize;
		state->last_msg_type = messagetype;
		state->last_msg = message;
		state->current_state = GN_SM_MessageSent;

		/* FIXME - clear KeepAlive timer */
		return state->link.send_message(messagesize, messagetype, message);
	}
	else return GN_ERR_NOTREADY;
}

API gn_state SM_Loop(struct gn_statemachine *state, int timeout)
{
	struct timeval loop_timeout;
	int i;

	if (!state->link.loop) {
		dprintf("No Loop function. Aborting.\n");
		abort();
	}
	for (i = 0; i < timeout; i++) {
		/*
		 * Some select() implementation (e.g. Linux) will modify the
		 * timeval structure - bozo
		 */
		loop_timeout.tv_sec = 0;
		loop_timeout.tv_usec = 100000;

		state->link.loop(&loop_timeout);
	}

	/* FIXME - add calling a KeepAlive function here */
	return state->current_state;
}

void SM_Reset(struct gn_statemachine *state)
{
	/* Don't reset to initialised if we aren't! */
	if (state->current_state != GN_SM_Startup) {
		state->current_state = GN_SM_Initialised;
		state->waiting_for_number = 0;
		state->received_number = 0;
	}
}

void SM_IncomingFunction(struct gn_statemachine *state, u8 messagetype, void *message, u16 messagesize)
{
	int c;
	int temp = 1;
	gn_data *data, *edata;
	gn_error res = GN_ERR_INTERNALERROR;
	int waitingfor = -1;

#ifdef	DEBUG
	dump("Message received: ");
	sm_message_dump(messagetype, message, messagesize);
#endif
	edata = calloc(1, sizeof(gn_data));
	data = edata;

	/* See if we need to pass the function the data struct */
	if (state->current_state == GN_SM_WaitingForResponse)
		for (c = 0; c < state->waiting_for_number; c++)
			if (state->waiting_for[c] == messagetype) {
				/* FIXME What's wrong with that? 
				data = state->data[c];
				*/
				waitingfor = c;
			}

	/* Pass up the message to the correct phone function, with data if necessary */
	c = 0;
	while (state->driver.incoming_functions[c].functions) {
		if (state->driver.incoming_functions[c].message_type == messagetype) {
			dprintf("Received message type %02x\n", messagetype);
			res = state->driver.incoming_functions[c].functions(messagetype, message, messagesize, data, state);
			temp = 0;
			break;
		}
		c++;
	}
	if (res == GN_ERR_UNSOLICITED) {
		dprintf("Unsolicited frame, skipping...\n");
		free(edata);
		return;
	} else if (res == GN_ERR_UNHANDLEDFRAME)
		sm_unhandled_frame_dump(state, messagetype, message, messagesize);
	if (temp != 0) {
		dprintf("Unknown Frame Type %02x\n", messagetype);
		state->driver.default_function(messagetype, message, messagesize, state);
		free(edata);
		return;
	}

	if (state->current_state == GN_SM_WaitingForResponse) {
		/* Check if we were waiting for a response and we received it */
		if (waitingfor != -1) {
			state->ResponseError[waitingfor] = res;
			state->received_number++;
		}

		/* Check if all waitingfors have been received */
		if (state->received_number == state->waiting_for_number) {
			state->current_state = GN_SM_ResponseReceived;
		}
	}
	free(edata);
}

/* This returns the error recorded from the phone function and indicates collection */
gn_error SM_GetError(struct gn_statemachine *state, unsigned char messagetype)
{
	int c, d;
	gn_error error = GN_ERR_NOTREADY;

	if (state->current_state == GN_SM_ResponseReceived) {
		for (c = 0; c < state->received_number; c++)
			if (state->waiting_for[c] == messagetype) {
				error = state->ResponseError[c];
				for (d = c + 1 ; d < state->received_number; d++) {
					state->ResponseError[d-1] = state->ResponseError[d];
					state->waiting_for[d-1] = state->waiting_for[d];
					state->data[d-1] = state->data[d];
				}
				state->received_number--;
				state->waiting_for_number--;
				c--; /* For neatness continue in the correct place */
			}
		if (state->received_number == 0) {
			state->waiting_for_number = 0;
			state->current_state = GN_SM_Initialised;
		}
	}

	return error;
}



/* Indicate that the phone code is waiting for an response */
/* This does not actually wait! */
gn_error SM_WaitFor(struct gn_statemachine *state, gn_data *data, unsigned char messagetype)
{
	/* If we've received a response, we have to call SM_GetError first */
	if ((state->current_state == GN_SM_Startup) || (state->current_state == GN_SM_ResponseReceived))
		return GN_ERR_NOTREADY;

	if (state->waiting_for_number == GN_SM_WAITINGFOR_MAX_NUMBER) return GN_ERR_NOTREADY;
	state->waiting_for[state->waiting_for_number] = messagetype;
	/* FIXME What's wrong with that? 
	state->data[state->waiting_for_number] = data;
	*/
	state->ResponseError[state->waiting_for_number] = GN_ERR_BUSY;
	state->waiting_for_number++;
	state->current_state = GN_SM_WaitingForResponse;

	return GN_ERR_NONE;
}


/* This function is for convinience only
   It is called after SM_SendMessage and blocks until a response is received

   t is in tenths of second
*/
static gn_error __SM_BlockTimeout(struct gn_statemachine *state, gn_data *data, int waitfor, int t, int noretry)
{
	int retry, timeout;
	gn_state s;
	gn_error err;

	for (retry = 0; retry < 3; retry++) {
		timeout = t;
		err = SM_WaitFor(state, data, waitfor);
		if (err != GN_ERR_NONE) return err;

		do {            /* ~3secs timeout */
			s = SM_Loop(state, 1);  /* Timeout=100ms */
			timeout--;
		} while ((timeout > 0) && (s == GN_SM_WaitingForResponse));

		if (s == GN_SM_ResponseReceived) return SM_GetError(state, waitfor);

		dprintf("SM_Block Retry - %d\n", retry);
		SM_Reset(state);
		if ((retry < 2) && (!noretry)) 
			SM_SendMessage(state, state->last_msg_size, state->last_msg_type, state->last_msg);
	}

	return GN_ERR_TIMEOUT;
}

gn_error SM_BlockTimeout(struct gn_statemachine *state, gn_data *data, int waitfor, int t)
{
	return __SM_BlockTimeout(state, data, waitfor, t, 0);
}

gn_error SM_Block(struct gn_statemachine *state, gn_data *data, int waitfor)
{
	return __SM_BlockTimeout(state, data, waitfor, 30, 0);
}

/* This function is equal to SM_Block except it does not retry the message */
gn_error SM_BlockNoRetryTimeout(struct gn_statemachine *state, gn_data *data, int waitfor, int t)
{
	return __SM_BlockTimeout(state, data, waitfor, t, 1);
}

gn_error SM_BlockNoRetry(struct gn_statemachine *state, gn_data *data, int waitfor)
{
	return __SM_BlockTimeout(state, data, waitfor, 100, 1);
}

/* Just to do things neatly */
API gn_error SM_Functions(gn_operation op, gn_data *data, struct gn_statemachine *sm)
{
	if (!sm->driver.functions) {
		dprintf("Sorry, phone has not yet been converted to new style. Phone.Functions == NULL!\n");
		return GN_ERR_INTERNALERROR;
	}
	return sm->driver.functions(op, data, sm);
}

/* Dumps a message */
void sm_message_dump(int messagetype, unsigned char *message, int messagesize)
{
	int i;
	char buf[17];

	buf[16] = 0;

	dump("0x%02x / 0x%04x", messagetype, messagesize);

	for (i = 0; i < messagesize; i++) {
		if (i % 16 == 0) {
			if (i != 0) dump("| %s", buf);
			dump("\n");
			memset(buf, ' ', 16);
		}
		dump("%02x ", message[i]);
		if (isprint(message[i])) buf[i % 16] = message[i];
	}

	if (i % 16) dump("%*s| %s", 3 * (16 - i % 16), "", buf);
	dump("\n");
}

/* Prints a warning message about unhandled frames */
void sm_unhandled_frame_dump(struct gn_statemachine *state, int messagetype, unsigned char *message, int messagesize)
{
	dump(_("UNHANDLED FRAME RECEIVED\nrequest: "));
	sm_message_dump(state->last_msg_type, state->last_msg, state->last_msg_size);

	dump(_("reply: "));
	sm_message_dump(messagetype, message, messagesize);

	dump(_("Please read Docs/Reporting-HOWTO and send a bug report!\n"));
}
