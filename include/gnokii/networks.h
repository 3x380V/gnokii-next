/*

  $Id: networks.h,v 1.5 2001-06-28 00:28:46 pkot Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Jan�k ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Header file for GSM networks.

  $Log: networks.h,v $
  Revision 1.5  2001-06-28 00:28:46  pkot
  Small docs updates (Pawel Kot)


*/

#ifndef __gsm_networks_h
#define __gsm_networks_h

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
char *GSM_GetNetworkName(char *NetworkCode);
char *GSM_GetNetworkCode(char *NetworkName);

char *GSM_GetCountryName(char *CountryCode);
char *GSM_GetCountryCode(char *CountryName);

#endif	/* __gsm_networks_h */
