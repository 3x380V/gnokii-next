/*

  $Id: nokia.h,v 1.1 2001-02-21 19:57:13 chris Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Jan�k ml.
  Copyright (C) 2001 Manfred Jonsson

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides useful functions for all phones  
  See README for more details on supported mobile phones.

  The various routines are called PNOK_...

  $Log: nokia.h,v $
  Revision 1.1  2001-02-21 19:57:13  chris
  More fiddling with the directory layout

  Revision 1.1  2001/02/01 15:19:41  pkot
  Fixed --identify and added Manfred's manufacturer patch


*/

#ifndef __phones_nokia_h
#define __phones_nokia_h


#define PNOK_MAX_MANUFACTURER_LENGTH 16 

GSM_Error PNOK_GetManufacturer(char *manufacturer);

#endif
