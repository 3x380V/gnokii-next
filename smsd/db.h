/*

  $Id: db.h,v 1.9 2003-02-26 00:15:49 pkot Exp $

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

#ifndef DB_H
#define DB_H

#include <glib.h>
#include "smsd.h"
#include "gnokii.h"

extern void (*DB_Bye) (void);
extern gint (*DB_ConnectInbox) (const DBConfig);
extern gint (*DB_ConnectOutbox) (const DBConfig);
extern gint (*DB_InsertSMS) (const gn_sms * const);
extern void (*DB_Look) (void);

#endif
