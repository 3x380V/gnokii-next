/*

  X G N O K I I

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Jan�k ml., Hugh Blemings
  & J�n Derfi��k <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Wed Apr 26 2000
  Modified by Jan Derfinak

*/

#ifndef XGNOKII_H
#define XGNOKII_H

#include <gtk/gtk.h>
#include "config.h"
#include "misc.h"
#include "gsm-common.h"

#define MAX_CALLER_GROUP_LENGTH	10
#define MAX_SMS_CENTER		10
#define MAX_BUSINESS_CARD_LENGTH	139

typedef struct {
  gchar *name;
  gchar *title;
  gchar *company;
  gchar *telephone;
  gchar *fax;
  gchar *email;
  gchar *address;
} UserInf;

typedef struct {
  gchar *initlength; /* Init length from .gnokiirc file */
  gchar *model;      /* Model from .gnokiirc file. */
  gchar *port;       /* Serial port from .gnokiirc file */
  gchar *connection; /* Connection type from .gnokiirc file */
  gchar *bindir;
  gchar *xgnokiidir;
  gchar *helpviewer; /* Program to showing help files */
  gchar *mailbox;    /* Mailbox, where we can save SMS's */
  gchar *maxSIMLen;  /* Max length of names on SIM card */
  gchar *maxPhoneLen;/* Max length of names in phone */
  gchar *locale;
  SMS_MessageCenter smsSetting[MAX_SMS_CENTER];
  UserInf user;
  gchar *callerGroups[6];
  gint   smsSets:4;
  bool   alarmSupported:1;
} XgnokiiConfig;

/* Hold main configuration data for xgnokii */
extern XgnokiiConfig xgnokiiConfig;

extern gint max_phonebook_name_length;
extern gint max_phonebook_number_length;
extern gint max_phonebook_sim_name_length;
extern gint max_phonebook_sim_number_length;
extern void GUI_InitCallerGroupsInf (void);
extern void GUI_InitSMSSettings (void);
extern void GUI_ShowAbout (void);

#endif /* XGNOKII_H */
