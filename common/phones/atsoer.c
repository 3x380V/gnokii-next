/*

  $Id: atsoer.c,v 1.1 2004-07-01 22:17:03 pkot Exp $

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

  Copyright 2004 Hugo Hass <hugo@larve.net>
  Copyright 2004 Pawel Kot <pkot@bezsensu.pl>

  This file provides functions specific to at commands on ericsson
  phones. See README for more details on supported mobile phones.

*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "config.h"
#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "phones/generic.h"
#include "phones/atgen.h"
#include "phones/atsoer.h"
#include "links/atbus.h"

static gn_error se_at_memory_type_set(gn_memory_type mt, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	gn_data data;
	char req[32];
	gn_error ret = GN_ERR_NONE;

	if (mt != drvinst->memorytype) {
		int len;
		char memtype[10];

		len = at_encode(drvinst->charset, memtype, memorynames[mt], strlen(memorynames[mt]));
		sprintf(req, "AT+CPBS=\"%s\"\r", memtype);
		ret = sm_message_send(11 + len, GN_OP_Init, req, state);
		if (ret)
			return GN_ERR_NOTREADY;
		gn_data_clear(&data);
		ret = sm_block_no_retry(GN_OP_Init, &data, state);
		if (ret == GN_ERR_NONE)
			drvinst->memorytype = mt;

		gn_data_clear(&data);
		ret = state->driver.functions(GN_OP_AT_GetMemoryRange, &data, state);
	}
	return ret;
}


static gn_error ReplyReadPhonebook(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	at_line_buffer buf;
	char *pos, *endpos;
	gn_error error;

	if ((error = at_error_get(buffer, state)) != GN_ERR_NONE)
		return (error == GN_ERR_UNKNOWN) ? GN_ERR_INVALIDLOCATION : error;

	buf.line1 = buffer + 1;
	buf.length = length;
	splitlines(&buf);

	if (strncmp(buf.line1, "AT+CPBR", 7)) {
		return GN_ERR_UNKNOWN;
	}

	if (!strncmp(buf.line2, "OK", 2)) {
		/* Empty phonebook location found */
		if (data->phonebook_entry) {
			*(data->phonebook_entry->number) = '\0';
			*(data->phonebook_entry->name) = '\0';
			data->phonebook_entry->caller_group = 0;
			data->phonebook_entry->subentries_count = 0;
			data->phonebook_entry->empty = true;
		}
		return GN_ERR_NONE;
	}
	if (data->phonebook_entry) {
		data->phonebook_entry->caller_group = 0;
		data->phonebook_entry->subentries_count = 0;

		/* store number */
		pos = strchr(buf.line2, '\"');
		endpos = NULL;
		if (pos)
			endpos = strchr(++pos, '\"');
		if (endpos) {
			*endpos = '\0';
			at_decode(drvinst->charset, data->phonebook_entry->number, pos, strlen(pos));
		}

		/* store name */
		pos = NULL;
		if (endpos)
			pos = strchr(endpos+2, ',');
		endpos = NULL;
		if (pos) {
			pos = strip_quotes(pos+1);
			at_decode(drvinst->charset, data->phonebook_entry->name, pos, strlen(pos));
		}
	}
	return GN_ERR_NONE;
}

static gn_error AT_WritePhonebook(gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	int len, ofs;
	char req[256], *tmp;
	gn_error ret;
	
	ret = se_at_memory_type_set(data->phonebook_entry->memory_type, state);
	if (ret)
		return ret;
	if (data->phonebook_entry->empty) {
		return state->driver.functions(GN_OP_DeletePhonebook, data, state);
	} else {
		char pnumber[128];

		ret = state->driver.functions(GN_OP_AT_SetCharset, data, state);
		if (ret)
			return ret;
		at_encode(drvinst->charset, pnumber, data->phonebook_entry->number, strlen(data->phonebook_entry->number));
		ofs = sprintf(req, "AT+CPBW=%d,\"%s\",%s,\"",
			      data->phonebook_entry->location+drvinst->memoryoffset,
			      pnumber,
			      data->phonebook_entry->number[0] == '+' ? "145" : "129");
		len = strlen(data->phonebook_entry->name);
		tmp = req + ofs;
		len = at_encode(drvinst->charset, tmp, data->phonebook_entry->name, len);
		tmp[len++] = '"'; tmp[len++] = '\r';
		len += ofs;
	} 
	if (sm_message_send(len, GN_OP_WritePhonebook, req, state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_WritePhonebook, data, state);
}

void at_sonyericsson_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state)
{
	at_insert_recv_function(GN_OP_ReadPhonebook, ReplyReadPhonebook, state);
	at_insert_send_function(GN_OP_WritePhonebook, AT_WritePhonebook, state);
}
