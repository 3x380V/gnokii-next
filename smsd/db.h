/*

  S M S D

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Jan�k ml., Hugh Blemings
  & J�n Derfi��k <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  $Id: db.h,v 1.4 2002-02-01 13:43:31 ja Exp $
  
*/

#ifndef DB_H
#define DB_H

#include <glib.h>
#include "smsd.h"
#include "gsm-sms.h"

extern void (*DB_Bye) (void);
extern gint (*DB_ConnectInbox) (const DBConfig);
extern gint (*DB_ConnectOutbox) (const DBConfig);
extern gint (*DB_InsertSMS) (const GSM_SMSMessage * const);
extern void (*DB_Look) (void);

#endif
