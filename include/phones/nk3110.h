/*

  $Id: nk3110.h,v 1.8 2003-02-26 00:15:49 pkot Exp $

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
  Copyright (C) 2001 Pawe� Kot <pkot@linuxnews.pl>

  This file provides functions specific to the 3110 series.
  See README for more details on supported mobile phones.

*/

#ifndef _gnokii_phones_nk3110_h
#define _gnokii_phones_nk3110_h

#include "gnokii/data.h"

/* Phone Memory Sizes */
#define P3110_MEMORY_SIZE_SM 20
#define P3110_MEMORY_SIZE_ME 0

/* Number of times to try resending SMS (empirical) */
#define P3110_SMS_SEND_RETRY_COUNT 4

typedef struct {
	bool sim_available;
	int user_data_count;
} nk3110_driver_instance;

#endif  /* #ifndef _gnokii_phones_nk3110_h */
