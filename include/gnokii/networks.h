/*

  $Id: networks.h,v 1.8 2002-04-30 18:56:12 pkot Exp $

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

  Header file for GSM networks.

*/

#ifndef __gsm_networks_h
#define __gsm_networks_h

#include "compat.h"

/* This type is used to hold information about various GSM networks. */

typedef struct {
	char *Code; /* GSM network code */
	char *Name; /* GSM network name */
} GSM_Network;

/* This type is used to hold information about various GSM countries. */

typedef struct {
	char *Code; /* GSM country code */
	char *Name; /* GSM country name */
} GSM_Country;

/* These functions are used to search the structure defined above.*/
API char *GSM_GetNetworkName(char *NetworkCode);
API char *GSM_GetNetworkCode(char *NetworkName);

API char *GSM_GetCountryName(char *CountryCode);
API char *GSM_GetCountryCode(char *CountryName);

#endif	/* __gsm_networks_h */
