/*

  $Id: atbus.h,v 1.10 2002-12-09 15:27:19 ladis Exp $

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

#ifndef _gnokii_atbus_h
#define _gnokii_atbus_h

#include "gsm-statemachine.h"

gn_error atbus_initialise(struct gn_statemachine *state, int mode);

/* Define some result/error codes internal to the AT command functions.
   Also define a code for an unterminated message. */
typedef enum {
	GN_AT_NONE,		/* NO or unknown result code */
	GN_AT_PROMPT,		/* SMS command waiting for input */
	GN_AT_OK,		/* Command succceded */
	GN_AT_ERROR,		/* Command failed */
	GN_AT_CMS,		/* SMS Command failed */
	GN_AT_CME,		/* Extended error code found */
} at_result;

#endif   /* #ifndef _gnokii_atbus_h */
