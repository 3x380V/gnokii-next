/*

  $Id: nokia.h,v 1.14 2002-12-04 00:47:22 bozo Exp $

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
  Copyright (C) 2001 Manfred Jonsson

  This file provides useful functions for all phones
  See README for more details on supported mobile phones.

  The various routines are called PNOK_...

*/

#ifndef __phones_nokia_h
#define __phones_nokia_h

#include "gsm-error.h"

#define	PNOK_MSG_SMS 0x02

gn_error PNOK_GetManufacturer(char *manufacturer);
void PNOK_DecodeString(unsigned char *dest, size_t max, const unsigned char *src, size_t len);
size_t PNOK_EncodeString(unsigned char *dest, size_t max, const unsigned char *src);

/* Common functions for misc Nokia drivers */
/* Call divert: nk6100, nk7110 */
gn_error PNOK_CallDivert(GSM_Data *data, GSM_Statemachine *state);
gn_error PNOK_IncomingCallDivert(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
int PNOK_FBUS_EncodeSMS(GSM_Data *data, GSM_Statemachine *state, unsigned char *req);

/* Security functions: nk6100, nk7100 */
gn_error PNOK_enable_extended_cmds(GSM_Data *data, GSM_Statemachine *state, unsigned char type);
gn_error PNOK_make_call(GSM_Data *data, GSM_Statemachine *state);
gn_error PNOK_answer_call(GSM_Data *data, GSM_Statemachine *state);
gn_error PNOK_cancel_call(GSM_Data *data, GSM_Statemachine *state);
gn_error PNOK_netmonitor(GSM_Data *data, GSM_Statemachine *state);
gn_error PNOK_IncomingSecurity(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);

#endif
