/*

  $Id: vcard.c,v 1.12 2005-03-20 18:26:52 pkot Exp $
  
  G N O K I I

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

  Copyright (C) 2002 Pavel Machek <pavel@ucw.cz>

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "compat.h"
#include "gnokii.h"

API int gn_phonebook2vcard(FILE * f, gn_phonebook_entry *entry, char *location)
{
	int i;

	fprintf(f, "BEGIN:VCARD\n");
	fprintf(f, "VERSION:3.0\n");
	fprintf(f, "FN:%s\n", entry->name);
	fprintf(f, "TEL;VOICE:%s\n", entry->number);
	fprintf(f, "X_GSM_STORE_AT:%s\n", location);
	fprintf(f, "X_GSM_CALLERGROUP:%d\n", entry->caller_group);

	/* Add ext. pbk info if required */
	for (i = 0; i < entry->subentries_count; i++) {
		switch (entry->subentries[i].entry_type) {
		case GN_PHONEBOOK_ENTRY_Email:
			fprintf(f, "EMAIL;INTERNET:%s\n", entry->subentries[i].data.number);
			break;
		case GN_PHONEBOOK_ENTRY_Postal:
			fprintf(f, "ADR;HOME:%s\n", entry->subentries[i].data.number);
			break;
		case GN_PHONEBOOK_ENTRY_Note:
			fprintf(f, "NOTE:%s\n", entry->subentries[i].data.number);
			break;
		case GN_PHONEBOOK_ENTRY_Number:
			switch (entry->subentries[i].number_type) {
			case GN_PHONEBOOK_NUMBER_Home:
				fprintf(f, "TEL;HOME:%s\n", entry->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_NUMBER_Mobile:
				fprintf(f, "TEL;CELL:%s\n", entry->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_NUMBER_Fax:
				fprintf(f, "TEL;FAX:%s\n", entry->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_NUMBER_Work:
				fprintf(f, "TEL;WORK:%s\n", entry->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_NUMBER_General:
				fprintf(f, "TEL;PREF:%s\n", entry->subentries[i].data.number);
				break;
			default:
				fprintf(f, "TEL;X_UNKNOWN_%d: %s\n", entry->subentries[i].number_type, entry->subentries[i].data.number);
				break;
			}
			break;
		case GN_PHONEBOOK_ENTRY_URL:
			fprintf(f, "URL:%s\n", entry->subentries[i].data.number);
			break;
		default:
			fprintf(f, "X_GNOKII_%d: %s\n", entry->subentries[i].entry_type, entry->subentries[i].data.number);
			break;
		}
	}

	fprintf(f, "END:VCARD\n\n");
	return 0;
}

#define BEGINS(a) ( !strncmp(buf, a, strlen(a)) )
#define STORE3(a, b) if (BEGINS(a)) { strncpy(b, buf+strlen(a), line_len - strlen(a)); }
#define STORE2(a, b, c) if (BEGINS(a)) { c; strncpy(b, buf+strlen(a), line_len - strlen(a)); continue; }
#define STORE(a, b) STORE2(a, b, (void) 0)
#define STOREINT(a, b) if (BEGINS(a)) { b = atoi(buf+strlen(a)); continue; }

#define STORESUB(a, c) STORE2(a, entry->subentries[entry->subentries_count++].data.number, \
				entry->subentries[entry->subentries_count].entry_type = c);
#define STORENUM(a, c) STORE2(a, entry->subentries[entry->subentries_count++].data.number, \
				entry->subentries[entry->subentries_count].entry_type = GN_PHONEBOOK_ENTRY_Number; \
				entry->subentries[entry->subentries_count].number_type = c);

#undef ERROR
#define ERROR(a) fprintf(stderr, "%s\n", a)

/* We assume gn_phonebook_entry is ready for writing in, ie. no rubbish inside */
API int gn_vcard2phonebook(FILE *f, gn_phonebook_entry *entry)
{
	char buf[10240];
	char memloc[10];
	int line_len;

	memset(memloc, 0, 10);
	while (1) {
		if (!fgets(buf, 1024, f))
			return -1;
		if (BEGINS("BEGIN:VCARD"))
			break;
	}

	while (1) {
		char *cr, *lf;
		if (!fgets(buf, 1024, f)) {
			ERROR("Vcard began but not ended?");
			return -1;
		}
		/* There's either "\n" or "\r\n' sequence at the
		 * end of the line */
		cr = strchr(buf, '\r');
		lf = strchr(buf, '\n');
		line_len = cr ? cr - buf : (lf ? lf - buf : strlen(buf));

		STORE("FN:", entry->name);
		STORE("TEL;VOICE:", entry->number);

		STORESUB("URL:", GN_PHONEBOOK_ENTRY_URL);
		STORESUB("EMAIL;INTERNET:", GN_PHONEBOOK_ENTRY_Email);
		STORESUB("ADR;HOME:", GN_PHONEBOOK_ENTRY_Postal);
		STORESUB("NOTE:", GN_PHONEBOOK_ENTRY_Note);

		STORE3("X_GSM_STORE_AT:", memloc);
		/* if the field is present and correctly formed */
		if (memloc && (strlen(memloc) > 2)) {
			entry->location = atoi(memloc + 2);
			memloc[2] = 0;
			entry->memory_type = gn_str2memory_type(memloc);
			continue;
		}
		STOREINT("X_GSM_CALLERGROUP:", entry->caller_group);

		STORENUM("TEL;HOME:", GN_PHONEBOOK_NUMBER_Home);
		STORENUM("TEL;CELL:", GN_PHONEBOOK_NUMBER_Mobile);
		STORENUM("TEL;FAX:", GN_PHONEBOOK_NUMBER_Fax);
		STORENUM("TEL;WORK:", GN_PHONEBOOK_NUMBER_Work);
		STORENUM("TEL;PREF:", GN_PHONEBOOK_NUMBER_General);

		if (BEGINS("END:VCARD"))
			break;
	}
	return 0;
}
