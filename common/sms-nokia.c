/*

  $Id: sms-nokia.c,v 1.6 2002-12-10 12:59:34 ladis Exp $

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

  Various function used by Nokia in their SMS extension protocols.

*/

#include <string.h>
#include "misc.h"
#include "sms-nokia.h"
#include "gsm-encoding.h"
#include "gsm-bitmaps.h"

/**
 * PackSmartMessagePart - Adds Smart Message header to the certain part of the message
 * @msg: place to store the header
 * @size: size of the part
 * @type: type of the part
 * @first: indicator of the first part
 *
 * This function adds the header as specified in the Nokia Smart Messaging
 * Specification v3.
 */
int sms_nokia_pack_smart_message_part(unsigned char *msg, unsigned int size,
				      unsigned int type, bool first)
{
	unsigned char current = 0;

	if (first) msg[current++] = 0x30; /* SM version. Here 3.0 */
	msg[current++] = type;
	msg[current++] = (size & 0xff00) >> 8; /* Length for picture part, hi */
	msg[current++] = size & 0x00ff;        /* length lo */
	return current;
}

/* Returns used length */
int sms_nokia_encode_text(unsigned char *text, unsigned char *message, bool first)
{
	int len, current = 0;
	/* FIXME: unicode length is not as simple as strlen */
	int type = GN_SMS_MULTIPART_DEFAULT;

	len = strlen(text);
	if (type == GN_SMS_MULTIPART_UNICODE) len *= 2;

	current = sms_nokia_pack_smart_message_part(message, len, type, first);

	if (type == GN_SMS_MULTIPART_UNICODE)
		char_encode_unicode(message + current, text, strlen(text));
	else
		memcpy(message + current, text, strlen(text));
	current += len;
	return current;
}

int sms_nokia_encode_bitmap(gn_bmp *bitmap, unsigned char *message, bool first)
{
	unsigned int current;

	/* FIXME: allow for the different sizes */
	current = sms_nokia_pack_smart_message_part(message, 256, GN_SMS_MULTIPART_BITMAP, first);
	return gn_bmp_encode_sms(bitmap, message + current) + current;
}
