/*

  $Id: gsm-api.c,v 1.54 2002-12-10 12:59:34 ladis Exp $

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

  Provides a generic API for accessing functions on the phone, wherever
  possible hiding the model specific details.

  The underlying code should run in it's own thread to allow communications to
  the phone to be run independantly of mailing code that calls these API
  functions.

  Unless otherwise noted, all functions herein block until they complete.  The
  functions themselves are defined in a structure in gsm-common.h.

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "misc.h"
#include "gsm-common.h"
#include "cfgreader.h"
#include "gsm-api.h"
#include "data/rlp-common.h"
#include "gsm-statemachine.h"
#include "phones/nk6510.h"
#include "phones/nk7110.h"
#include "phones/nk6100.h"
#include "phones/nk3110.h"
#include "phones/nk2110.h"


#if defined(WIN32) && defined(_USRDLL)
/*	GnokiiDll.cpp : Defines the entry point for the DLL application.
 *	
 *	We don't do anything special here (yet) but the code is needed
 *	in order to create a DLL.
 *
 */
BOOL APIENTRY DllMain(HANDLE hModule, 
		      DWORD  ul_reason_for_call, 
		      LPVOID lpReserved)
{
	/*	For now this is enough to satisfy the compiler
	 *	and linker. Currently no extra code is needed
	 *	for initializing or deinitializing of the DLL.
	 */
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#endif	/* defined(WIN32) && defined(_USRDLL) */

struct gn_statemachine gn_sm;
gn_error (*gn_gsm_f)(gn_operation op, gn_data *data, struct gn_statemachine *state);


/* Define pointer to the gn_phone structure used by external code to
   obtain information that varies from model to model. This structure is also
   defined in gsm-common.h */
API gn_phone *gn_gsm_info;

/* Initialise interface to the phone. Model number should be a string such as
   3810, 5110, 6110 etc. Device is the serial port to use e.g. /dev/ttyS0, the
   user must have write permission to the device. */
static gn_error register_driver(gn_driver *driver, char *model, char *setupmodel, struct gn_statemachine *sm)
{
	gn_data *data = NULL;
	gn_data *p_data;
	gn_error error = GN_ERR_UNKNOWNMODEL;

	if (setupmodel) {
		data = calloc(1, sizeof(gn_data));
		data->model = setupmodel;
		p_data = data;
	} else {
		p_data = NULL;
	}
	if (strstr(driver->phone.models, model) != NULL)
		error = driver->functions(GN_OP_Init, p_data, sm);

	if (data) free(data);
	return error;
}

#define REGISTER_DRIVER(x, y) { \
	extern gn_driver driver_##x; \
	if ((ret = register_driver(&driver_##x, model, y, sm)) != GN_ERR_UNKNOWNMODEL) \
		return ret; \
}

API gn_error gn_gsm_initialise(char *model, char *device, char *initlength,
			       gn_connection_type connection,
			       void (*rlp_handler)(gn_rlp_f96_frame *frame),
			       struct gn_statemachine *sm)
{
	gn_error ret;
	char *sms_timeout;

	sm->link.connection_type = connection;
	sm->link.init_length = atoi(initlength);
	sms_timeout = gn_cfg_get(gn_cfg_info, "sms", "timeout");
	if (!sms_timeout) sm->link.sms_timeout = 100;
	else sm->link.sms_timeout = atoi(sms_timeout) * 10;
	memset(&sm->link.port_device, 0, sizeof(sm->link.port_device));
	strncpy(sm->link.port_device, device, sizeof(sm->link.port_device) - 1);

	REGISTER_DRIVER(nokia_7110, NULL);
	REGISTER_DRIVER(nokia_6510, NULL);
	REGISTER_DRIVER(nokia_6100, NULL);
#if 0
	REGISTER_DRIVER(nokia_3110, NULL);
#ifndef WIN32
	REGISTER_DRIVER(nokia_2110, NULL);
	REGISTER_DRIVER(dancall_2711, NULL);
#endif
#endif
	REGISTER_DRIVER(fake, NULL);
	REGISTER_DRIVER(at, model);
	REGISTER_DRIVER(nokia_6160, NULL);

	return GN_ERR_UNKNOWNMODEL;
}
