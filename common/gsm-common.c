/*

  $Id: gsm-common.c,v 1.8 2001-11-13 16:12:20 pkot Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Jan�k ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  $Log: gsm-common.c,v $
  Revision 1.8  2001-11-13 16:12:20  pkot
  Preparing libsms to get to work. 6210/7110 SMS and SMS Folder updates

  Revision 1.7  2001/11/08 16:34:19  pkot
  Updates to work with new libsms

  Revision 1.6  2001/09/14 12:15:28  pkot
  Cleanups from 0.3.3 (part1)

  Revision 1.5  2001/03/22 16:17:05  chris
  Tidy-ups and fixed gnokii/Makefile and gnokii/ChangeLog which I somehow corrupted.

  Revision 1.4  2001/03/21 23:36:04  chris
  Added the statemachine
  This will break gnokii --identify and --monitor except for 6210/7110

  Revision 1.3  2001/02/28 21:26:51  machek
  Added StrToMemoryType utility function

  Revision 1.2  2001/02/03 23:56:15  chris
  Start of work on irda support (now we just need fbus-irda.c!)
  Proper unicode support in 7110 code (from pkot)

  Revision 1.1  2001/01/12 14:28:39  pkot
  Forgot to add this file. ;-)


*/

#include <string.h>
#include "gsm-common.h"

GSM_Error Unimplemented(void)
{
	return GE_NOTIMPLEMENTED;
}

GSM_MemoryType StrToMemoryType(const char *s)
{
#define X(a) if (!strcmp(s, #a)) return GMT_##a;
	X(ME);
	X(SM);
	X(FD);
	X(ON);
	X(EN);
	X(DC);
	X(RC);
	X(MC);
	X(LD);
	X(MT);
	X(IN);
	X(OU);
	X(AR);
	X(TE);
	X(F1);
	X(F2);
	X(F3);
	X(F4);
	X(F5);
	X(F6);
	X(F7);
	X(F8);
	X(F9);
	X(F10);
	X(F11);
	X(F12);
	X(F13);
	X(F14);
	X(F15);
	X(F16);
	X(F17);
	X(F18);
	X(F19);
	X(F20);
	return GMT_XX;
#undef X
}

/* This very small function is just to make it */
/* easier to clear the data struct every time one is created */
inline void GSM_DataClear(GSM_Data *data)
{
	memset(data, 0, sizeof(GSM_Data));
}
