/*

  $Id: gsm-common.c,v 1.1 2001-01-12 14:28:39 pkot Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Jan�k ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  $Log: gsm-common.c,v $
  Revision 1.1  2001-01-12 14:28:39  pkot
  Forgot to add this file. ;-)


*/

#include <gsm-common.h>

GSM_Error Unimplemented(void)
{
	return GE_NOTIMPLEMENTED;
}
