/*

  $Id: generic.c,v 1.14 2002-09-28 23:51:37 pkot Exp $

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

  This file provides useful functions for all phones
  See README for more details on supported mobile phones.

  The various routines are called PGEN_(whatever).

*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "misc.h"
#include "gsm-common.h"
#include "gsm-statemachine.h"
#include "phones/generic.h"
#include "links/utils.h"

/* If we do not support a message type, print out some debugging info */
gn_error PGEN_IncomingDefault(int messagetype, unsigned char *buffer, int length, GSM_Statemachine *state)
{
	dprintf("Unknown Message received [type (%02x) length (%d): \n", messagetype, length);
	SM_DumpMessage(messagetype, buffer, length);

	return GN_ERR_NONE;
}

gn_error PGEN_Terminate(GSM_Data *data, GSM_Statemachine *state)
{
	return LINK_Terminate(state);
}

