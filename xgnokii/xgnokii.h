/*

  $Id: xgnokii.h,v 1.22 2004-07-20 21:35:35 pkot Exp $

  X G N O K I I

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

#ifndef XGNOKII_H
#define XGNOKII_H

#include <gtk/gtk.h>
#include "config.h"
#include "misc.h"
#include "gnokii.h"

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
	gchar *initlength;	/* Init length from .gnokiirc file */
	gchar *model;		/* Model from .gnokiirc file. */
	gchar *port;		/* Serial port from .gnokiirc file */
	gn_connection_type connection;	/* Connection type from .gnokiirc file */
	gchar *bindir;
	gchar *xgnokiidir;
	gchar *helpviewer;	/* Program to showing help files */
	gchar *mailbox;		/* Mailbox, where we can save SMS's */
	gchar *maxSIMLen;	/* Max length of names on SIM card */
	gchar *maxPhoneLen;	/* Max length of names in phone */
	gchar *locale;		/* Locale for the app translations */
	gchar *help_locale;	/* Locale for HTML help */
	gn_sms_message_center smsSetting[MAX_SMS_CENTER];
	UserInf user;
	gchar *callerGroups[6];
	gint smsSets:4;
	bool alarmSupported:1;
} XgnokiiConfig;

/* Hold main configuration data for xgnokii */
extern XgnokiiConfig xgnokiiConfig;

extern gint lastfoldercount, foldercount;
extern char folders[GN_SMS_FOLDER_MAX_NUMBER][GN_SMS_MESSAGE_MAX_NUMBER];
extern gint max_phonebook_name_length;
extern gint max_phonebook_number_length;
extern gint max_phonebook_sim_name_length;
extern gint max_phonebook_sim_number_length;
extern void GUI_InitCallerGroupsInf(void);
extern void GUI_InitSMSSettings(void);
extern void GUI_ShowAbout(void);
extern void MainExit();

#endif				/* XGNOKII_H */
