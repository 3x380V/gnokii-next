/*

  $Id: gsm-filetypes.h,v 1.14 2002-03-28 21:37:48 pkot Exp $

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

  Functions to read and write common file types.

*/

#ifndef __gsm_filetypes_h
#define __gsm_filetypes_h

#include "gsm-error.h"
#include "gsm-common.h"
#include "gsm-bitmaps.h"
#include "gsm-ringtones.h"

int GSM_ReadVCalendarFile(char *FileName, GSM_CalendarNote *cnote, int number);

int GetvCalTime(GSM_DateTime *dt, char *time);
int FillCalendarNote(GSM_CalendarNote *note, char *type,
		     char *text, char *time, char *alarm);


/* Ringtone Files */

GSM_Error GSM_ReadRingtoneFile(char *FileName, GSM_Ringtone *ringtone);
GSM_Error GSM_SaveRingtoneFile(char *FileName, GSM_Ringtone *ringtone);

int GetScale (char *num);
int GetDuration (char *num);

/* Defines the character that separates fields in rtttl files. */
#define RTTTL_SEP ":"

GSM_Error saverttl(FILE *file, GSM_Ringtone *ringtone);
GSM_Error saveott(FILE *file, GSM_Ringtone *ringtone);

GSM_Error loadrttl(FILE *file, GSM_Ringtone *ringtone);
GSM_Error loadott(FILE *file, GSM_Ringtone *ringtone);


/* Bitmap Files */

GSM_Error GSM_ReadBitmapFile(char *FileName, GSM_Bitmap *bitmap, GSM_Information *info);
GSM_Error GSM_SaveBitmapFile(char *FileName, GSM_Bitmap *bitmap, GSM_Information *info);
int GSM_SaveTextFile(char *FileName, char *text, int mode);
GSM_Error GSM_ShowBitmapFile(char *FileName);

void savenol(FILE *file, GSM_Bitmap *bitmap, GSM_Information *info);
void savengg(FILE *file, GSM_Bitmap *bitmap, GSM_Information *info);
void savensl(FILE *file, GSM_Bitmap *bitmap, GSM_Information *info);
void savenlm(FILE *file, GSM_Bitmap *bitmap);
void saveota(FILE *file, GSM_Bitmap *bitmap);
void savebmp(FILE *file, GSM_Bitmap *bitmap);

#ifdef XPM
void savexpm(char *filename, GSM_Bitmap *bitmap);
#endif

GSM_Error loadngg(FILE *file, GSM_Bitmap *bitmap, GSM_Information *info);
GSM_Error loadnol(FILE *file, GSM_Bitmap *bitmap, GSM_Information *info);
GSM_Error loadnsl(FILE *file, GSM_Bitmap *bitmap);
GSM_Error loadnlm(FILE *file, GSM_Bitmap *bitmap);
GSM_Error loadota(FILE *file, GSM_Bitmap *bitmap, GSM_Information *info);
GSM_Error loadbmp(FILE *file, GSM_Bitmap *bitmap);

#ifdef XPM
GSM_Error loadxpm(char *filename, GSM_Bitmap *bitmap);
#endif

typedef enum {
	None = 0,
	NOL,
	NGG,
	NSL,
	NLM,
	BMP,
	OTA,
	XPMF,
	RTTL,
	OTT
} GSM_Filetypes;

#endif /* __gsm_filetypes_h */
