/*

  S M S D

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Jan�k ml., Hugh Blemings
  & J�n Derfi��k <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.


  $Id: smsd.h,v 1.3 2002-01-27 14:54:43 ja Exp $
  
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
extern gint WriteSMS (GSM_SMSMessage *);

#endif
