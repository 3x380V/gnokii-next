/*

  X G N O K I I

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Jan�k ml., Hugh Blemings
  & J�n Derfi��k <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Mon May 01 2000
  Modified by Jan Derfinak

*/

#ifndef XGNOKII_COMMON_H
#define XGNOKII_COMMON_H

#include <gtk/gtk.h>
#include <stdlib.h>  /* for size_t */

typedef struct {
  gchar *model;
  gchar *number;
} Model;

typedef struct {
  GtkWidget *dialog;
  GtkWidget *text;
} ErrorDialog;

typedef struct {
  GtkWidget *dialog;
  GtkWidget *text;
} InfoDialog;

typedef struct {
  GtkWidget *dialog;
  GtkWidget *text;
} YesNoDialog;

extern void CancelDialog (const GtkWidget *, const gpointer);
extern void CreateErrorDialog (ErrorDialog *, GtkWidget *);
extern void CreateInfoDialog (InfoDialog *, GtkWidget *);
extern void CreateYesNoDialog (YesNoDialog *, const GtkSignalFunc,
                               const GtkSignalFunc, GtkWidget *);
extern GtkWidget* NewPixmap (gchar **, GdkWindow *, GdkColor *);
extern void DeleteEvent (const GtkWidget *, const GdkEvent *, const gpointer );
extern gint LaunchProcess (const gchar *, const gchar *, const gint,
                           const gint, const gint);
extern void RemoveZombie (const gint);
extern void Help (const GtkWidget *, const gpointer);
extern gint strrncmp (const gchar * const, const gchar * const, size_t);
extern gchar *GetModel (const gchar *);
extern bool CallerGroupSupported (const gchar *);
extern bool NetmonitorSupported (const gchar *);
extern bool KeyboardSupported (const gchar *);
extern bool SMSSupported (const gchar *);
extern bool CalendarSupported (const gchar *);
extern bool DTMFSupported (const gchar *);
extern bool SpeedDialSupported (const gchar *);
extern void GUI_Refresh (void);

#endif
