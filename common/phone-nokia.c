/*

  $Id: phone-nokia.c,v 1.1 2001-02-01 15:19:41 pkot Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Jan�k ml.
  Copyright (C) 2001 Manfred Jonsson

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides useful functions for all phones  
  See README for more details on supported mobile phones.

  The various routines are called PNOK_(whatever).

  $Log: phone-nokia.c,v $
  Revision 1.1  2001-02-01 15:19:41  pkot
  Fixed --identify and added Manfred's manufacturer patch


*/

#include <string.h>

#include "gsm-common.h"
#include "phone-nokia.h"


/* This function provides a way to detect the manufacturer of a phone */
/* because it is only used by the fbus/mbus protocol phones and only */
/* nokia is using those protocols, the result is clear. */
/* the error reporting is also very simple */
/* the strncpy and PNOK_MAX_MODEL_LENGTH is only here as a reminder */

GSM_Error PNOK_GetManufacturer(char *manufacturer)
{
	strncpy (manufacturer, "Nokia", PNOK_MAX_MANUFACTURER_LENGTH);
	return (GE_NONE);
}
