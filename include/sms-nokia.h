/*

  $Id: sms-nokia.h,v 1.2 2002-08-18 22:29:58 pkot Exp $

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

  Copyright (C) 2002 Pawe� Kot <pkot@linuxnews.pl>

  Various function prototypes used by Nokia in their SMS extension protocols.

*/

#ifndef _gnokii_sms_nokia_h
#define _gnokii_sms_nokia_h

#include "gsm-bitmaps.h"

/* There are type values for the different parts of the Nokia Multipart
 * Messages */
#define SMS_MULTIPART_DEFAULT      0
#define SMS_MULTIPART_UNICODE      1
#define SMS_MULTIPART_BITMAP       2
#define SMS_MULTIPART_RINGTONE     3
#define SMS_MULTIPART_PROFILENAME  4 /* This is sick. Isn't it? */
#define SMS_MULTIPART_RESERVED     5 /* I am really afraid what else can appear here... */
#define SMS_MULTIPART_SCREENSAVER  6

int PackSmartMessagePart(unsigned char *msg, unsigned int size,
			 unsigned int type, bool first);
int EncodeNokiaText(unsigned char *text, unsigned char *message, bool first);
int GSM_EncodeNokiaBitmap(gn_bmp *bitmap, unsigned char *message, bool first);

#endif /* _gnokii_sms_nokia_h */
