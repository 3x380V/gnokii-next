/*

  $Id: generic.h,v 1.9 2002-07-26 21:00:59 bozo Exp $

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

  The various routines are called PGEN_...

*/

#ifndef __phones_generic_h
#define __phones_generic_h

#include "gsm-error.h"
#include "gsm-statemachine.h"

/* Generic Functions */

GSM_Error PGEN_IncomingDefault(int messagetype, unsigned char *buffer, int length, GSM_Statemachine *state);
GSM_Error PGEN_Terminate(GSM_Data *data, GSM_Statemachine *state);


#endif








