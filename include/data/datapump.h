/*

  $Id: datapump.h,v 1.7 2002-03-28 21:37:48 pkot Exp $

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

  Header file for data pump code.

*/

#ifndef __data_datapump_h
#define __data_datapump_h

/* Prototypes */
bool	DP_Initialise(int read_fd, int write_fd);
void    DP_CallPassup(GSM_CallStatus CallStatus, GSM_CallInfo *CallInfo);

#endif	/* __data_datapump_h */
