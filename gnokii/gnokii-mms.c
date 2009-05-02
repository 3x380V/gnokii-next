/*

  $Id: gnokii-mms.c,v 1.5 2009-05-02 16:26:47 dforsi Exp $

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

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

  Copyright (C) 2009 Daniele Forsi

  Mainline code for gnokii utility. MMS handling code.
  Derived from gnokii-sms.c

*/

#include "config.h"
#include "misc.h"
#include "compat.h"

#include <stdio.h>
#include <getopt.h>
#include <errno.h>

#include "gnokii-app.h"
#include "gnokii.h"

/* Outputs summary of all MMS gnokii commands */
void mms_usage(FILE *f)
{
	fprintf(f, _("MMS options:\n"
		     "          --getmms memory_type start [end] [{--mime|--pdu|--raw} file]\n"
		     "                 [-d|--delete]\n"
		     "          --deletemms memory_type start [end]\n"
		));
}

/* Displays usage of --getmms command */
int getmms_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --getmms memory start [end]  retrieves messages from memory type (SM, ME,\n"
			"                                    IN, OU, ...) starting from\n"
			"                                    location start and ending with end;\n"
			"                                    if end option is omitted, just one entry\n"
			"                                    is read;\n"
			"                                    start must be a number, end may be a number\n"
			"                                    or 'end' string;\n"
			"                                    if 'end' is used entries are being read\n"
			"                                    until empty location\n"
			"        --mime filename             convert MMS in MIME format and save it to\n"
			"                                    the given file;\n"
			"        --pdu filename              extract MMS from raw format and save it to\n"
			"                                    the given file;\n"
			"        --raw filename              get MMS from phone and save it unchanged to\n"
			"                                    the given file;\n"
			"        --delete\n"
			"        -d                          delete MMS after reading\n"
			"        --overwrite\n"
			"        -o                          overwrite destination file without asking\n"
			"\n"
		));
	return exitval;
}

gn_error fprint_mms(FILE *file, gn_mms *message)
{
	fprintf(file, _("%d. %s (%s)\n"), message->number, _("MMS"), gn_sms_message_status2str(message->status));
	fprintf(file, "%s", message->buffer);

	return GN_ERR_NONE;
}

/* Get MMS messages. */
gn_error getmms(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	int i, del = 0;
	gn_sms_folder folder;
	gn_sms_folder_list folderlist;
	gn_mms *message;
	char *memory_type_string;
	int start_message, end_message, count, mode = 1;
	char filename[64];
	gn_error error = GN_ERR_NONE;
	gn_mms_format output_format_type = GN_MMS_FORMAT_TEXT;

	struct option options[] = {
		{ "mime",       required_argument, NULL, GN_MMS_FORMAT_MIME},
		{ "pdu",        required_argument, NULL, GN_MMS_FORMAT_PDU},
		{ "raw",        required_argument, NULL, GN_MMS_FORMAT_RAW},
		{ "delete",     no_argument,       NULL, 'd' },
		{ "overwrite",  no_argument,       NULL, 'o' },
		{ NULL,         0,                 NULL, 0 }
	};

	/* Handle command line args that set type, start and end locations. */
	memory_type_string = optarg;
	if (gn_str2memory_type(memory_type_string) == GN_MT_XX) {
		fprintf(stderr, _("Unknown memory type %s (use ME, SM, IN, OU, ...)!\n"), optarg);
		return GN_ERR_INVALIDMEMORYTYPE;
	}

	start_message = gnokii_atoi(argv[optind]);
	if (errno || start_message < 0)
		return getmms_usage(stderr, -1);
	end_message = parse_end_value_option(argc, argv, optind + 1, start_message);
	if (errno || end_message < 0)
		return getmms_usage(stderr, -1);

	*filename = '\0';
	/* parse all options (beginning with '-') */
	while ((i = getopt_long(argc, argv, "1:2:3:do", options, NULL)) != -1) {
		switch (i) {
		case 'd':
			del = 1;
			break;
		/* force mode -- don't ask to overwrite */
		case 'o':
			mode = 0;
			break;
		case GN_MMS_FORMAT_MIME:
			/* FALL THROUGH */
		case GN_MMS_FORMAT_PDU:
			/* FALL THROUGH  */
		case GN_MMS_FORMAT_RAW:
			/* output formats are mutually exclusive */
			if (output_format_type != GN_MMS_FORMAT_TEXT) {
				return getmms_usage(stderr, -1);
			}
			output_format_type = i;
			if (!optarg) {
				return getsms_usage(stderr, -1);
			}
			snprintf(filename, sizeof(filename), "%s", optarg);
			fprintf(stderr, _("Saving into %s\n"), filename);
			break;
		default:
			return getmms_usage(stderr, -1);
		}
	}
	if (argc - optind > 3) {
		/* There are too many arguments that don't start with '-' */
		return getmms_usage(stderr, -1);
	}

	error = gn_mms_alloc(&message);
	if (error != GN_ERR_NONE)
		return error;

	folder.folder_id = 0;
	data->sms_folder = &folder;
	data->sms_folder_list = &folderlist;
	/* Now retrieve the requested entries. */
	for (count = start_message; count <= end_message; count++) {

		memset(message, 0, sizeof(gn_mms));
		message->memory_type = gn_str2memory_type(memory_type_string);
		message->number = count;
		message->buffer_format = output_format_type;
		data->mms = message;

		error = gn_mms_get(data, state);
		switch (error) {
		case GN_ERR_NONE:
			if (*filename) {
				/* writebuffer() will set mode to "append" */
				mode = writebuffer(filename, data->mms->buffer, data->mms->buffer_length, mode);
			} else {
				error = fprint_mms(stdout, message);
				fprintf(stdout, "\n");
			}
			if (del && mode != -1) {
				if (GN_ERR_NONE != gn_mms_delete(data, state))
					fprintf(stderr, _("(delete failed)\n"));
				else
					fprintf(stderr, _("(message deleted)\n"));
			}
			if (data->mms->buffer)
				free(data->mms->buffer);
			break;
		default:
			if ((error == GN_ERR_INVALIDLOCATION) && (end_message == INT_MAX) && (count > start_message))
				return GN_ERR_NONE;
			fprintf(stderr, _("GetMMS %s %d failed! (%s)\n"), memory_type_string, count, gn_error_print(error));
			if (error == GN_ERR_INVALIDMEMORYTYPE)
				fprintf(stderr, _("See the gnokii manual page for the supported memory types with the phone\nyou use.\n"));
			if (error != GN_ERR_EMPTYLOCATION)
				return error;
			break;
		}
		if (mode == -1)
			break;
	}
	gn_mms_free(message);

	return error;
}

/* Displays usage of --deletemms command */
int deletemms_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --deletemms memory start [end]  deletes messages from memory type\n"
			"                                       (SM, ME, IN, OU, ...) starting\n"
			"                                       from location start and ending with end;\n"
			"                                       if end option is omitted, just one entry\n"
			"                                       is removed;\n"
			"                                       if 'end' is used entries are being removed\n"
			"                                       until empty location\n"
			"\n"
		));
	return exitval;
}

/* Delete MMS messages. */
gn_error deletemms(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_mms message;
	char *memory_type_string;
	int start_message, end_message, count;
	gn_error error = GN_ERR_NONE;

	/* Handle command line args that set type, start and end locations. */
	memory_type_string = optarg;
	message.memory_type = gn_str2memory_type(memory_type_string);
	if (message.memory_type == GN_MT_XX) {
		fprintf(stderr, _("Unknown memory type %s (use ME, SM, ...)!\n"), optarg);
		return GN_ERR_INVALIDMEMORYTYPE;
	}

	start_message = gnokii_atoi(argv[optind]);
	if (errno || start_message < 0)
		return deletemms_usage(stderr, -1);
	end_message = parse_end_value_option(argc, argv, optind + 1, start_message);
	if (errno || end_message < 0)
		return deletemms_usage(stderr, -1);

	/* Now delete the requested entries. */
	for (count = start_message; count <= end_message; count++) {
		message.number = count;
		data->mms = &message;
		error = gn_mms_delete(data, state);

		if (error == GN_ERR_NONE)
			fprintf(stderr, _("Deleted MMS %s %d\n"), memory_type_string, count);
		else {
			if ((error == GN_ERR_INVALIDLOCATION) && (end_message == INT_MAX) && (count > start_message))
				return GN_ERR_NONE;
			fprintf(stderr, _("DeleteMMS %s %d failed!(%s)\n\n"), memory_type_string, count, gn_error_print(error));
		}
	}

	/* FIXME: We return the value of the last read.
	 * What should we return?
	 */
	return error;
}


