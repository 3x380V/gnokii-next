/*

  $Id: gnokii-app.h,v 1.36 2003-09-07 22:01:58 bozo Exp $

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

  Header file for test utility.

*/

/* Nokia ringtones codes. */

char *RingingTones[] = {
/*  0 */ "Unknown",
/*  1 */ "Unknown",                 /* FIXME: probably not set. */
/*  2 */ "Unknown",
/*  3 */ "Unknown",
/*  4 */ "Unknown",
/*  5 */ "Unknown",
/*  6 */ "Unknown",
/*  7 */ "Unknown",
/*  8 */ "Unknown",
/*  9 */ "Unknown",
/* 10 */ "Unknown",                 /* FIXME: probably pre set. */
/* 11 */ "Unknown",
/* 12 */ "Unknown",
/* 13 */ "Unknown",
/* 14 */ "Unknown",
/* 15 */ "Unknown",
/* 16 */ "Unknown",
/* 17 */ "Uploaded",
/* 18 */ "Ring ring",
/* 19 */ "Low",
/* 20 */ "Fly",
/* 21 */ "Mosquito",
/* 22 */ "Bee",
/* 23 */ "Intro",
/* 24 */ "Etude",
/* 25 */ "Hunt",
/* 26 */ "Going up",
/* 27 */ "City Bird",
/* 28 */ "Unknown",
/* 29 */ "Unknown",
/* 30 */ "Chase",
/* 31 */ "Unknown",
/* 32 */ "Scifi",
/* 33 */ "Unknown",
/* 34 */ "Kick",
/* 35 */ "Do-mi-so",
/* 36 */ "Robo N1X",
/* 37 */ "Dizzy",
/* 38 */ "Unknown",
/* 39 */ "Playground",
/* 40 */ "Unknown",
/* 41 */ "Unknown",
/* 42 */ "Unknown",
/* 43 */ "That's it!",
/* 44 */ "Unknown",
/* 45 */ "Unknown",
/* 46 */ "Unknown",
/* 47 */ "Grande valse",   /* FIXME: Knock knock (Knock again). */
/* 48 */ "Helan",          /* FIXME: Grand valse on 5110. */
/* 49 */ "Fuga",           /* FIXME: Helan on 5110. */
/* 50 */ "Menuet",         /* FIXME: Fuga on 5110. */
/* 51 */ "Ode to Joy",
/* 52 */ "Elise",
/* 53 */ "Mozart 40",
/* 54 */ "Piano Concerto", /* FIXME: Mozart 40 on 5110. */
/* 55 */ "William Tell",
/* 56 */ "Badinerie",      /* FIXME: William Tell on 5110. */
/* 57 */ "Polka",          /* FIXME: Badinerie on 5110. */
/* 58 */ "Attraction",     /* FIXME: Polka on 5110. */
/* 59 */ "Unknown",        /* FIXME: Attraction on 5110. */
/* 60 */ "Polite",         /* FIXME: Down on 5110. */
/* 61 */ "Persuasion",
/* 62 */ "Unknown",        /* FIXME: Persuasion on 5110. */
/* 63 */ "Unknown",
/* 64 */ "Unknown",
/* 65 */ "Unknown",
/* 66 */ "Unknown",
/* 67 */ "Tick tick",
/* 68 */ "Samba",
/* 69 */ "Unknown",        /* FIXME: Samba on 5110. */
/* 70 */ "Orient",
/* 71 */ "Charleston",     /* FIXME: Orient on 5110. */
/* 72 */ "Unknown",        /* FIXME: Charleston on 5110. */
/* 73 */ "Jumping",        /* FIXME: Songette on 5110. */
/* 74 */ "Unknown",        /* FIXME: Jumping on 5110. */
/* 75 */ "Unknown",        /* FIXME: Lamb (Marry) on 5110. */
/* 76 */ "Unknown",
/* 77 */ "Unknown",
/* 78 */ "Unknown",
/* 79 */ "Unknown",
/* 80 */ "Unknown"         /* FIXME: Tango (Tangoed) on 5110. */
};
