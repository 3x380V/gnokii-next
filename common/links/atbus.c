/*

  $Id: atbus.c,v 1.19 2002-09-13 22:40:00 pkot Exp $

  G N O K I I

  A Linux/Unix toolset and driver for mobile phones.

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

  Copyright 2001 Manfred Jonsson <manfred.jonsson@gmx.de>

*/

/* System header files */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* Various header file */
#include "config.h"
#include "misc.h"
#include "gsm-common.h"
#include "gsm-ringtones.h"
#include "gsm-networks.h"
#include "device.h"
#include "gsm-statemachine.h"

#include "links/atbus.h"

static GSM_Error ATBUS_Loop(struct timeval *timeout);
static bool ATBUS_OpenSerial(int mode, char *device);
static void ATBUS_RX_StateMachine(unsigned char rx_char);

/* FIXME - pass device_* the link stuff?? */
/* FIXME - win32 stuff! */

static void at_printf(char *prefix, char *buf, int len);

/* FIXME - when sending an AT command while another one is still in */
/*         progress, the old command is aborted and the new ignored. */
/*         the result is _one_ error message from the phone. */

/* Some globals */
static GSM_Statemachine *statemachine;

/* The buffer for phone responses not only holds the data from
the phone but also a byte which holds the compiled status of the
response. it is placed at [0]. */
static char reply_buf[1024];
static int reply_buf_pos = 1;
static int binlength = 1;

static int xwrite(unsigned char *d, int len)
{
	int res;

	at_printf("write: ", d, len);

	while (len) {
		res = device_write(d, len);
		if (res == -1) {
			if (errno != EAGAIN) {
				perror("gnokii I/O error ");
				return -1;
			}
		} else {
			d += res;
			len -= res;
		}
	}
	return 0;
}


static GSM_Error AT_SendMessage(u16 message_length, u8 message_type, unsigned char *msg)
{
	usleep(10000);
	xwrite(msg, message_length);
	return GE_NONE;
}


/* RX_State machine for receive handling.  Called once for each character
   received from the phone. */
static void ATBUS_RX_StateMachine(unsigned char rx_char)
{
	reply_buf[reply_buf_pos++] = rx_char;
	reply_buf[reply_buf_pos] = '\0';

	if (reply_buf_pos >= binlength) {
		reply_buf[0] = GEAT_NONE;
		/* first check if <cr><lf> is found at end of reply_buf.
		 * attention: the needed length is greater 2 because we
		 * dont need to enter if no result/error will be found. */
		if ((reply_buf_pos > 4) && (!strncmp(reply_buf+reply_buf_pos-2, "\r\n", 2))) {
			/* no lenght check needed */
			if (!strncmp(reply_buf+reply_buf_pos-4, "OK\r\n", 4))
				reply_buf[0] = GEAT_OK;
			else if ((reply_buf_pos > 7) && (!strncmp(reply_buf+reply_buf_pos-7, "ERROR\r\n", 7)))
				reply_buf[0] = GEAT_ERROR;
		}
		/* check if SMS prompt is found */
		if ((reply_buf_pos > 4) && (!strncmp(reply_buf+reply_buf_pos-4, "\r\n> ", 4))) {
			reply_buf[0] = GEAT_PROMPT;
		}
		if (reply_buf[0] != GEAT_NONE) {


			at_printf("read : ", reply_buf + 1, reply_buf_pos - 1);

			SM_IncomingFunction(statemachine, statemachine->LastMsgType, reply_buf, reply_buf_pos - 1);
			reply_buf_pos = 1;
			binlength = 1;
			return;
		}
/* needed for binary date etc
   TODO: correct reading of variable length integers
		if (reply_buf_pos == 12) {
			if (!strncmp(reply_buf + 3, "ABC", 3) {
				binlength = atoi(reply_buf + 8);
			}
		}
*/
	}
}


static bool ATBUS_OpenSerial(int mode, char *device)
{
	int result;
	result = device_open(device, false, false, mode, GCT_Serial);
	if (!result) {
		perror(_("Couldn't open ATBUS device"));
		return (false);
	}
	if (mode) {
		/* make 7110 with dlr-3 happy. the nokia dlr-3 cable     */
		/* provides hardware handshake lines but is, at least at */
		/* initialization, slow. to be properly detected, state  */
		/* changes must be longer than 700 milli seconds. if the */
		/* timing is to fast all commands after dtr high will be */
		/* ignored by the phone until dtr is toggled again.      */
		/* to reset the phone and set a sane state, dtr must     */
		/* pulled low. when irda is turned on in the phone, dtr  */
		/* must pulled high to switch on the serial line of the  */
		/* phone (this will switch of irda). only set it high    */
		/* after serial line initialization (when it probably    */
		/* was low before) is not enough. so we do high, low and */
		/* high again, always with one second apply time. also   */
		/* wait one second before sending commands or init will  */
		/* fail. */
		device_setdtrrts(1, 1);
		sleep(1);
		device_setdtrrts(0, 1);
		sleep(1);
		device_setdtrrts(1, 1);
		sleep(1);
	} else {
		device_setdtrrts(1, 1);
	}
	return (true);
}

static GSM_Error ATBUS_Loop(struct timeval *timeout)
{
	unsigned char buffer[255];
	int count, res;

	res = device_select(timeout);
	if (res > 0) {
		res = device_read(buffer, 255);
		for (count = 0; count < res; count++)
			ATBUS_RX_StateMachine(buffer[count]);
	} else
		return GE_TIMEOUT;
	/* This traps errors from device_read */
	if (res > 0)
		return GE_NONE;
	else
		return GE_INTERNALERROR;
}


/* Initialise variables and start the link */
/* Fixme we allow serial and irda for connection to reduce */
/* bug reports. this is pretty silly for /dev/ttyS?. */
GSM_Error ATBUS_Initialise(GSM_Statemachine *state, int mode)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	/* 'Copy in' the global structures */
	statemachine = state;

	/* Fill in the link functions */
	state->Link.Loop = &ATBUS_Loop;
	state->Link.SendMessage = &AT_SendMessage;

#ifdef HAVE_IRDA
	if ((state->Link.ConnectionType == GCT_Serial) ||
	    (state->Link.ConnectionType == GCT_Irda)) {
#else
	if (state->Link.ConnectionType == GCT_Serial) {
#endif
		if (!ATBUS_OpenSerial(mode, state->Link.PortDevice))
			return GE_FAILED;
	} else {
		dprintf("Device not supported by ATBUS\n");
		return GE_FAILED;
	}

	return GE_NONE;
}

static void at_printf(char *prefix, char *buf, int len)
{
#ifdef DEBUG
	int in = 0, out = 0;
	char *pos = prefix;
	char debug_buf[1024];

	while (*pos)
		debug_buf[out++] = *pos++;
	debug_buf[out++] ='[';
	while ((in < len) && (out < 1016)) {
		if (buf[in] == '\n') {
			sprintf(debug_buf + out,"<lf>");
			in++;
			out += 4;
		} else if (buf[in] == '\r') {
			sprintf(debug_buf + out,"<cr>");
			in++;
			out += 4;
		} else if (buf[in] < 32) {
			debug_buf[out++] = '^';
			debug_buf[out++] = buf[in++] + 64;
		} else {
			debug_buf[out++] = buf[in++];
		}
	}
	debug_buf[out++] =']';
	debug_buf[out++] ='\n';
	debug_buf[out] ='\0';
	dprintf(debug_buf);
#endif
}
