/*

  S M S D

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  $Id: db.h,v 1.2 2002-01-14 22:14:36 ja Exp $
  
*/

#ifndef DB_H
#define DB_H

#include <glib.h>
#include "gsm-sms.h"

extern void DB_Bye (void);
extern gint DB_ConnectInbox (const gchar * const);
extern gint DB_ConnectOutbox (const gchar * const);
extern gint DB_InsertSMS (const GSM_SMSMessage * const);
extern void DB_Look (void);

#endif
