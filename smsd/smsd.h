/*

  $Id: smsd.h,v 1.8 2002-12-27 17:03:20 bozo Exp $

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
#include "gsm-api.h"


typedef enum {
  SMSD_READ_REPORTS = 1
} SMSDSettings;

typedef struct {
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
extern gint WriteSMS (gn_sms *);

#endif
