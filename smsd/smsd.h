/*

  $Id: smsd.h,v 1.5 2002-06-25 09:59:26 ja Exp $

  S M S D

  A Linux/Unix GUI for Nokia mobile phones.

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

  Copyright (C) 1999 Pavel Jan�k ml., Hugh Blemings
  & J�n Derfi��k <ja@mail.upjs.sk>.

*/

#ifndef SMSD_H
#define SMSD_H

#include <glib.h>
#include "gsm-sms.h"


typedef enum {
  SMSD_READ_REPORTS = 1
} SMSDSettings;

typedef struct {
  gchar *initlength; /* Init length from .gnokiirc file */
  gchar *model;      /* Model from .gnokiirc file. */
  gchar *port;       /* Serial port from .gnokiirc file */
  gchar *connection; /* Connection type from .gnokiirc file */
  gchar *bindir;
  gchar *dbMod;
  gchar *libDir;
  gint   smsSets:4;
} SmsdConfig;

typedef struct {
  gchar *user;
  gchar *password;
  gchar *db;
  gchar *host;
} DBConfig;

extern gchar *strEscape (const gchar *const);
extern SmsdConfig smsdConfig;
extern gint WriteSMS (GSM_API_SMS *);

#endif
