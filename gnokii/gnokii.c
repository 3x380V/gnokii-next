/*

  $Id: gnokii.c,v 1.305 2002-10-15 09:34:27 bozo Exp $

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

  Copyright (C) 1999-2000  Hugh Blemings & Pavel Jan�k ml.
  Copyright (C) 2001-2002  Pavel Machek
  Copyright (C) 2001-2002  Pawe� Kot
  Copyright (C) 2002       BORBELY Zoltan

  Mainline code for gnokii utility.  Handles command line parsing and
  reading/writing phonebook entries and other stuff.

  WARNING: this code is the test tool. Well, our test tool is now
  really powerful and useful :-)

*/

#include "misc.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#if __unices__
#  include <strings.h>	/* for memset */
#endif
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>


#ifdef WIN32

#  include <windows.h>
#  include <process.h>
#  include "getopt.h"

#else

#  include <unistd.h>
#  include <termios.h>
#  include <fcntl.h>
#  include <getopt.h>

#endif


#ifdef ENABLE_NLS
#  include <locale.h>
#endif

#include "gnokii.h"

#include "gsm-sms.h"
#include "gsm-common.h"
#include "gsm-api.h"
#include "gsm-networks.h"
#include "cfgreader.h"
#include "gsm-filetypes.h"
#include "gsm-bitmaps.h"
#include "gsm-ringtones.h"
#include "gsm-statemachine.h"
#include "gsm-call.h"

#define MAX_INPUT_LINE_LEN 512

struct gnokii_arg_len {
	int gal_opt;
	int gal_min;
	int gal_max;
	int gal_flags;
};

/* This is used for checking correct argument count. If it is used then if
   the user specifies some argument, their count should be equivalent to the
   count the programmer expects. */

#define GAL_XOR 0x01

typedef enum {
	OPT_HELP,
	OPT_VERSION,
	OPT_MONITOR,
	OPT_ENTERSECURITYCODE,
	OPT_GETSECURITYCODESTATUS,
	OPT_CHANGESECURITYCODE,
	OPT_SETDATETIME,
	OPT_GETDATETIME,
	OPT_SETALARM,
	OPT_GETALARM,
	OPT_DIALVOICE,
	OPT_ANSWERCALL,
	OPT_HANGUP,
	OPT_GETTODO,
	OPT_WRITETODO,
	OPT_DELETEALLTODOS,
	OPT_GETCALENDARNOTE,
	OPT_WRITECALENDARNOTE,
	OPT_DELCALENDARNOTE,
	OPT_GETDISPLAYSTATUS,
	OPT_GETPHONEBOOK,
	OPT_WRITEPHONEBOOK,
	OPT_GETSPEEDDIAL,
	OPT_SETSPEEDDIAL,
	OPT_GETSMS,
	OPT_DELETESMS,
	OPT_SENDSMS,
	OPT_SAVESMS,
	OPT_SENDLOGO,
	OPT_SENDRINGTONE,
	OPT_GETSMSC,
	OPT_SETSMSC,
	OPT_GETWELCOMENOTE,
	OPT_SETWELCOMENOTE,
	OPT_PMON,
	OPT_NETMONITOR,
	OPT_IDENTIFY,
	OPT_SENDDTMF,
	OPT_RESET,
	OPT_SETLOGO,
	OPT_GETLOGO,
	OPT_VIEWLOGO,
	OPT_GETRINGTONE,
	OPT_SETRINGTONE,
	OPT_GETPROFILE,
	OPT_SETPROFILE,
	OPT_DISPLAYOUTPUT,
	OPT_KEYPRESS,
	OPT_ENTERCHAR,
	OPT_DIVERT,
	OPT_SMSREADER,
	OPT_FOOGLE,
	OPT_GETSECURITYCODE,
	OPT_GETWAPBOOKMARK,
	OPT_WRITEWAPBOOKMARK,
	OPT_GETWAPSETTING,
	OPT_WRITEWAPSETTING,
	OPT_ACTIVATEWAPSETTING,
	OPT_DELETEWAPBOOKMARK,
	OPT_DELETESMSFOLDER,
	OPT_CREATESMSFOLDER,
	OPT_LISTNETWORKS,
} opt_index;

static char *model;      /* Model from .gnokiirc file. */
static char *Port;       /* Serial port from .gnokiirc file */
static char *Initlength; /* Init length from .gnokiirc file */
static char *Connection; /* Connection type from .gnokiirc file */
static char *BinDir;     /* Binaries directory from .gnokiirc file - not used here yet */
static FILE *logfile = NULL;
static char *lockfile = NULL;

/* Local variables */
static char *GetProfileCallAlertString(int code)
{
	switch (code) {
	case PROFILE_CALLALERT_RINGING:		return "Ringing";
	case PROFILE_CALLALERT_ASCENDING:	return "Ascending";
	case PROFILE_CALLALERT_RINGONCE:	return "Ring once";
	case PROFILE_CALLALERT_BEEPONCE:	return "Beep once";
	case PROFILE_CALLALERT_CALLERGROUPS:	return "Caller groups";
	case PROFILE_CALLALERT_OFF:		return "Off";
	default:				return "Unknown";
	}
}

static char *GetProfileVolumeString(int code)
{
	switch (code) {
	case PROFILE_VOLUME_LEVEL1:		return "Level 1";
	case PROFILE_VOLUME_LEVEL2:		return "Level 2";
	case PROFILE_VOLUME_LEVEL3:		return "Level 3";
	case PROFILE_VOLUME_LEVEL4:		return "Level 4";
	case PROFILE_VOLUME_LEVEL5:		return "Level 5";
	default:				return "Unknown";
	}
}

static char *GetProfileKeypadToneString(int code)
{
	switch (code) {
	case PROFILE_KEYPAD_OFF:		return "Off";
	case PROFILE_KEYPAD_LEVEL1:		return "Level 1";
	case PROFILE_KEYPAD_LEVEL2:		return "Level 2";
	case PROFILE_KEYPAD_LEVEL3:		return "Level 3";
	default:				return "Unknown";
	}
}

static char *GetProfileMessageToneString(int code)
{
	switch (code) {
	case PROFILE_MESSAGE_NOTONE:		return "No tone";
	case PROFILE_MESSAGE_STANDARD:		return "Standard";
	case PROFILE_MESSAGE_SPECIAL:		return "Special";
	case PROFILE_MESSAGE_BEEPONCE:		return "Beep once";
	case PROFILE_MESSAGE_ASCENDING:		return "Ascending";
	default:				return "Unknown";
	}
}

static char *GetProfileWarningToneString(int code)
{
	switch (code) {
	case PROFILE_WARNING_OFF:		return "Off";
	case PROFILE_WARNING_ON:		return "On";
	default:				return "Unknown";
	}
}

static char *GetProfileVibrationString(int code)
{
	switch (code) {
	case PROFILE_VIBRATION_OFF:		return "Off";
	case PROFILE_VIBRATION_ON:		return "On";
	default:				return "Unknown";
	}
}

static void short_version(void)
{
	fprintf(stderr, _("GNOKII Version %s\n"), VERSION);
}

/* This function shows the copyright and some informations usefull for
   debugging. */
static void version(void)
{
	fprintf(stderr, _("Copyright (C) Hugh Blemings <hugh@blemings.org>, 1999, 2000\n"
			  "Copyright (C) Pavel Jan�k ml. <Pavel.Janik@suse.cz>, 1999, 2000\n"
			  "Copyright (C) Pavel Machek <pavel@ucw.cz>, 2001\n"
			  "Copyright (C) Pawe� Kot <pkot@linuxnews.pl>, 2001-2002\n"
			  "Copyright (C) BORBELY Zoltan <bozo@andrews.hu>, 2002\n"
			  "gnokii is free software, covered by the GNU General Public License, and you are\n"
			  "welcome to change it and/or distribute copies of it under certain conditions.\n"
			  "There is absolutely no warranty for gnokii.  See GPL for details.\n"
			  "Built %s %s for %s on %s \n"), __TIME__, __DATE__, model, Port);
}

/* The function usage is only informative - it prints this program's usage and
   command-line options. */
static int usage(FILE *f)
{
	fprintf(f, _("   usage: gnokii [--help|--monitor|--version]\n"
		     "          gnokii --getphonebook memory_type start_number [end_number|end]\n"
		     "                 [-r|--raw]\n"
		     "          gnokii --writephonebook [-iv]\n"
		     "          gnokii --getwapbookmark number\n"
		     "          gnokii --writewapbookmark name URL\n"
		     "          gnokii --deletewapbookmark number\n"
		     "          gnokii --getwapsetting number [-r]\n"
		     "          gnokii --writewapsetting\n"
		     "          gnokii --activatewapsetting number\n"
		     "          gnokii --getspeeddial number\n"
		     "          gnokii --setspeeddial number memory_type location\n"
		     "          gnokii --createsmsfolder name\n"
		     "          gnokii --deletesmsfolder number\n"
		     "          gnokii --getsms memory_type start [end] [-f file] [-F file] [-d]\n"
		     "          gnokii --deletesms memory_type start [end]\n"
		     "          gnokii --sendsms destination [--smsc message_center_number |\n"
		     "                 --smscno message_center_index] [-r] [-C n] [-v n]\n"
		     "                 [--long n] [-i]\n"
		     "          gnokii --savesms [--sender from] [--smsc message_center_number |\n"
		     "                 --smscno message_center_index] [--folder folder_id]\n"
		     "                 [--location number] [--sent | --read] [--deliver] \n"
		     "          gnokii --smsreader\n"
		     "          gnokii --getsmsc [start_number [end_number]] [-r|--raw]\n"
		     "          gnokii --setsmsc\n"
		     "          gnokii --setdatetime [YYYY [MM [DD [HH [MM]]]]]\n"
		     "          gnokii --getdatetime\n"
		     "          gnokii --setalarm [HH MM]\n"
		     "          gnokii --getalarm\n"
		     "          gnokii --dialvoice number\n"
		     "          gnokii --answercall callid\n"
		     "          gnokii --hangup callid\n"
		     "          gnokii --gettodo start [end] [-v]\n"
		     "          gnokii --writetodo vcardfile number\n"
		     "          gnokii --deletealltodos\n"
		     "          gnokii --getcalendarnote start [end] [-v]\n"
		     "          gnokii --writecalendarnote vcardfile number\n"
		     "          gnokii --deletecalendarnote start [end]\n"
		     "          gnokii --getdisplaystatus\n"
		     "          gnokii --netmonitor {reset|off|field|devel|next|nr}\n"
		     "          gnokii --identify\n"
		     "          gnokii --senddtmf string\n"
		     "          gnokii --sendlogo {caller|op|picture} destination logofile [network code]\n"
		     "          gnokii --sendringtone rtttlfile destination\n"
		     "          gnokii --setlogo op [logofile] [network code]\n"
		     "          gnokii --setlogo startup [logofile]\n"
		     "          gnokii --setlogo caller [logofile] [caller group number] [group name]\n"
		     "          gnokii --setlogo {dealer|text} [text]\n"
		     "          gnokii --getlogo op [logofile] [network code]\n"
		     "          gnokii --getlogo startup [logofile] [network code]\n"
		     "          gnokii --getlogo caller [logofile][caller group number][network code]\n"
		     "          gnokii --getlogo {dealer|text}\n"
		     "          gnokii --viewlogo logofile\n"
		     "          gnokii --getringtone rtttlfile [location] [-r|--raw]\n"
		     "          gnokii --setringtone rtttlfile [location] [-r|--raw] [--name name]\n"
		     "          gnokii --reset [soft|hard]\n"
		     "          gnokii --getprofile [start_number [end_number]] [-r|--raw]\n"
		     "          gnokii --setprofile\n"
		     "          gnokii --displayoutput\n"
		     "          gnokii --keysequence\n"
		     "          gnokii --enterchar\n"
		     "          gnokii --divert {--op|-o} {register|enable|query|disable|erasure}\n"
		     "                 {--type|-t} {all|busy|noans|outofreach|notavail}\n"
		     "                 {--call|-c} {all|voice|fax|data}\n"
		     "                 [{--timeout|-m} time_in_seconds]\n"
		     "                 [{--number|-n} number]\n"
		     "          gnokii --listnetworks\n"
		     "          gnokii --getsecuritycode\n"
		));
#ifdef SECURITY
	fprintf(f, _("          gnokii --entersecuritycode PIN|PIN2|PUK|PUK2\n"
		     "          gnokii --getsecuritycodestatus\n"
		     "          gnokii --changesecuritycode PIN|PIN2|PUK|PUK2\n"
		));
#endif
	exit(-1);
}

/* businit is the generic function which waits for the FBUS link. The limit
   is 10 seconds. After 10 seconds we quit. */

static GSM_Statemachine State;
static GSM_Data data;
static GSM_CBMessage CBQueue[16];
static int cb_ridx = 0;
static int cb_widx = 0;


static void busterminate(void)
{
	SM_Functions(GOP_Terminate, NULL, &State);
	if (lockfile) unlock_device(lockfile);
}

static void businit(void (*rlp_handler)(RLP_F96Frame *frame))
{
	gn_error error;
	GSM_ConnectionType connection = GCT_Serial;
	char *aux;
	static bool atexit_registered = false;

	GSM_DataClear(&data);

	if (!strcasecmp(Connection, "dau9p"))    connection = GCT_DAU9P; /* Use only with 6210/7110 for faster connection with such cable */
	if (!strcasecmp(Connection, "dlr3p"))    connection = GCT_DLR3P;
	if (!strcasecmp(Connection, "infrared")) connection = GCT_Infrared;
#ifdef HAVE_IRDA
	if (!strcasecmp(Connection, "irda"))     connection = GCT_Irda;
#endif
#ifndef WIN32
	if (!strcasecmp(Connection, "tcp"))      connection = GCT_TCP;
#endif

	/* register cleanup function */
	if (!atexit_registered) {
		atexit_registered = true;
		atexit(busterminate);
	}
	/* signal(SIGINT, bussignal); */

	aux = gn_cfg_get(gn_cfg_info, "global", "use_locking");
	/* Defaults to 'no' */
	if (aux && !strcmp(aux, "yes")) {
		lockfile = lock_device(Port);
		if (lockfile == NULL) {
			fprintf(stderr, _("Lock file error. Exiting\n"));
			exit(1);
		}
	}

	/* Initialise the code for the GSM interface. */
	error = gn_gsm_initialise(model, Port, Initlength, connection, rlp_handler, &State);
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Telephone interface init failed: %s Quitting.\n"), gn_error_print(error));
		exit(2);
	}
}

/* This function checks that the argument count for a given options is withing
   an allowed range. */
static int checkargs(int opt, struct gnokii_arg_len gals[], int argc)
{
	int i;

	/* Walk through the whole array with options requiring arguments. */
	for (i = 0; !(gals[i].gal_min == 0 && gals[i].gal_max == 0); i++) {

		/* Current option. */
		if (gals[i].gal_opt == opt) {

			/* Argument count checking. */
			if (gals[i].gal_flags == GAL_XOR) {
				if (gals[i].gal_min == argc || gals[i].gal_max == argc)
					return 0;
			} else {
				if (gals[i].gal_min <= argc && gals[i].gal_max >= argc)
					return 0;
			}

			return 1;
		}
	}

	/* We do not have options without arguments in the array, so check them. */
	if (argc == 0) return 0;
	else return 1;
}

static void sendsms_usage()
{
	fprintf(stderr, _(" usage: gnokii --sendsms destination\n"
	                  "               [--smsc message_center_number | --smscno message_center_index]\n"
	                  "               [-r] [-C n] [-v n] [--long n] [--animation file;file;file;file]\n"
	                  "               [--concat this;total;serial]\n"
			  " Give the text of the message to the standard input.\n"
			  "   destination - phone number where to send SMS\n"
			  "   --smsc      - phone number of the SMSC\n"
			  "   --smscno    - index of the SMSC set stored in the phone\n"
			  "   -r          - require delivery report\n"
			  "   -C n        - message Class; values [0, 3]\n"
			  "   -v n        - set validity period to n minutes\n"
			  "   --long n    - read n bytes from the input; default is 160\n"
			  "\n"
		));
	exit(1);
}

static gn_error readtext(SMS_UserData *udata, int input_len)
{
	char message_buffer[255 * GSM_MAX_SMS_LENGTH];
	int chars_read;

	fprintf(stderr, _("Please enter SMS text. End your input with <cr><control-D>:"));

	/* Get message text from stdin. */
	chars_read = fread(message_buffer, 1, sizeof(message_buffer), stdin);

	if (chars_read == 0) {
		fprintf(stderr, _("Couldn't read from stdin!\n"));
		return GN_ERR_INTERNALERROR;
	} else if (chars_read > input_len || chars_read > sizeof(udata->u.Text) - 1) {
		fprintf(stderr, _("Input too long! (%d, maximum is %d)\n"), chars_read, input_len);
		return GN_ERR_INTERNALERROR;
	}

	/* Null terminate. */
	message_buffer[chars_read] = 0x00;
	if (udata->Type != SMS_iMelodyText && chars_read > 0 && message_buffer[chars_read - 1] == '\n') 
		message_buffer[--chars_read] = 0x00;
	strncpy(udata->u.Text, message_buffer, chars_read);
	udata->u.Text[chars_read] = 0;
	udata->Length = chars_read;

	return GN_ERR_NONE;
}

static gn_error loadbitmap(gn_bmp *bitmap, char *s, int type)
{
	gn_error error;
	bitmap->type = type;
	error = gn_bmp_null(bitmap, &State.Phone.Info);	
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Could not null bitmap: %s\n"), gn_error_print(error));
		return error;
	}
	error = GSM_ReadBitmapFile(s, bitmap, &State.Phone.Info);
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Could not load bitmap from %s: %s\n"), s, gn_error_print(error));
		return error;
	}
	return GN_ERR_NONE;
}

/* Send  SMS messages. */
static int sendsms(int argc, char *argv[])
{
	GSM_API_SMS sms;
	gn_error error;
	/* The maximum length of an uncompressed concatenated short message is
	   255 * 153 = 39015 default alphabet characters */
	int input_len, i, curpos = 0;

	struct option options[] = {
		{ "smsc",    required_argument, NULL, '1'},
		{ "smscno",  required_argument, NULL, '2'},
		{ "long",    required_argument, NULL, '3'},
		{ "picture", required_argument, NULL, '4'},
		{ "8bit",    0,                 NULL, '8'},
		{ "imelody", 0,                 NULL, 'i'},
		{ "animation",required_argument,NULL, 'a'},
		{ "concat",  required_argument, NULL, 'o'},
		{ NULL,      0,                 NULL, 0}
	};

	input_len = GSM_MAX_SMS_LENGTH;

	/* The memory is zeroed here */
	gn_sms_default_submit(&sms);

	memset(&sms.Remote.Number, 0, sizeof(sms.Remote.Number));
	strncpy(sms.Remote.Number, argv[0], sizeof(sms.Remote.Number) - 1);
	if (sms.Remote.Number[0] == '+') 
		sms.Remote.Type = SMS_International;
	else 
		sms.Remote.Type = SMS_Unknown;

	optarg = NULL;
	optind = 0;

	while ((i = getopt_long(argc, argv, "r8co:C:v:ip:", options, NULL)) != -1) {
		switch (i) {       /* -c for compression. not yet implemented. */
		case '1': /* SMSC number */
			strncpy(sms.SMSC.Number, optarg, sizeof(sms.SMSC.Number) - 1);
			if (sms.SMSC.Number[0] == '+') 
				sms.SMSC.Type = SMS_International;
			else
				sms.SMSC.Type = SMS_Unknown;
			break;

		case '2': /* SMSC number index in phone memory */
			data.MessageCenter = calloc(1, sizeof(SMS_MessageCenter));
			data.MessageCenter->No = atoi(optarg);
			if (data.MessageCenter->No < 1 || data.MessageCenter->No > 5) {
				free(data.MessageCenter);
				sendsms_usage();
			}
			if (SM_Functions(GOP_GetSMSCenter, &data, &State) == GN_ERR_NONE) {
				strcpy(sms.SMSC.Number, data.MessageCenter->SMSC.Number);
				sms.SMSC.Type = data.MessageCenter->SMSC.Type;
			}
			free(data.MessageCenter);
			break;

		case '3': /* we send long message */
			input_len = atoi(optarg);
			if (input_len > 255 * GSM_MAX_SMS_LENGTH) {
				fprintf(stderr, _("Input too long!\n"));
				return -1;
			}
			break;

		case 'p':
		case '4': /* we send multipart message - picture message; FIXME: This seems not yet implemented */
			sms.UDH.Number = 1;
			break;

		case 'a': /* Animation */ {
			char buf[10240];
			char *s = buf, *t;
			strcpy(buf, optarg);
			sms.UserData[curpos].Type = SMS_AnimationData;
			for (i = 0; i < 4; i++) {
				t = strchr(s, ';');
				if (t)
					*t++ = 0;
				loadbitmap(&sms.UserData[curpos].u.Animation[i], s, i ? GN_BMP_EMSAnimation2 : GN_BMP_EMSAnimation);
				s = t;
			}
			sms.UserData[++curpos].Type = SMS_NoData;
			curpos = -1;
			break;
		}

		case 'o': /* Concat header */ {
			dprintf("Adding concat header\n");
			sms.UserData[curpos].Type = SMS_Concat;
			if (3 != sscanf(optarg, "%d;%d;%d", &sms.UserData[curpos].u.Concat.this, 
					&sms.UserData[curpos].u.Concat.total, 
					&sms.UserData[curpos].u.Concat.serial)) {
				fprintf(stderr, _("Incorrect --concat option\n"));
				break;
			}
			curpos++;
			break;
		}

		case 'r': /* request for delivery report */
			sms.DeliveryReport = true;
			break;

		case 'C': /* class Message */
			switch (*optarg) {
			case '0': sms.DCS.u.General.Class = 1; break;
			case '1': sms.DCS.u.General.Class = 2; break;
			case '2': sms.DCS.u.General.Class = 3; break;
			case '3': sms.DCS.u.General.Class = 4; break;
			default: sendsms_usage();
			}
			break;

		case 'v':
			sms.Validity = atoi(optarg);
			break;

		case '8':
			sms.DCS.u.General.Alphabet = SMS_8bit;
			input_len = GSM_MAX_8BIT_SMS_LENGTH;
			break;

		case 'i':
			sms.UserData[0].Type = SMS_iMelodyText;
			sms.UserData[1].Type = SMS_NoData;
			error = readtext(&sms.UserData[0], input_len);
			if (error != GN_ERR_NONE) return -1;
			if (sms.UserData[0].Length < 1) {
				fprintf(stderr, _("Empty message. Quitting.\n"));
				return -1;
			}
			curpos = -1;
			break;
		default:
			sendsms_usage();
		}
	}

	if (!sms.SMSC.Number[0]) {
		data.MessageCenter = calloc(1, sizeof(SMS_MessageCenter));
		data.MessageCenter->No = 1;
		if (SM_Functions(GOP_GetSMSCenter, &data, &State) == GN_ERR_NONE) {
			strcpy(sms.SMSC.Number, data.MessageCenter->SMSC.Number);
			sms.SMSC.Type = data.MessageCenter->SMSC.Type;
		}
		free(data.MessageCenter);
	}

	if (!sms.SMSC.Type) sms.SMSC.Type = SMS_Unknown;

	if (curpos != -1) {
		error = readtext(&sms.UserData[curpos], input_len);
		if (error != GN_ERR_NONE) return -1;
		if (sms.UserData[curpos].Length < 1) {
			fprintf(stderr, _("Empty message. Quitting.\n"));
			return -1;
		}
		sms.UserData[curpos].Type = SMS_PlainText;
		if (!gn_char_def_alphabet(sms.UserData[curpos].u.Text))
			sms.DCS.u.General.Alphabet = SMS_UCS2;
		sms.UserData[++curpos].Type = SMS_NoData;
	}

	data.SMS = &sms;

	/* Send the message. */
	error = gn_sms_send(&data, &State);

	if (error == GN_ERR_NONE) {
		fprintf(stderr, _("Send succeeded!\n"));
	} else {
		fprintf(stderr, _("SMS Send failed (%s)\n"), gn_error_print(error));
	}

	return 0;
}

static int savesms(int argc, char *argv[])
{
	GSM_API_SMS sms;
	gn_error error = GN_ERR_INTERNALERROR;
	/* The maximum length of an uncompressed concatenated short message is
	   255 * 153 = 39015 default alphabet characters */
	char message_buffer[255 * GSM_MAX_SMS_LENGTH];
	int input_len, chars_read;
	int i;
#if 0
	int confirm = -1;
	int interactive = 0;
	char ans[8];
#endif
	struct option options[] = {
		{ "smsc",     required_argument, NULL, '0'},
		{ "smscno",   required_argument, NULL, '1'},
		{ "sender",   required_argument, NULL, '2'},
		{ "location", required_argument, NULL, '3'},
		{ "read",     0,                 NULL, 'r'},
		{ "sent",     0,                 NULL, 's'},
		{ "folder",   required_argument, NULL, 'f'},
		{ "deliver",  0                , NULL, 'd'},
		{ NULL,       0,                 NULL, 0}
	};

	unsigned char memory_type[20];
	struct tm		*now;
	time_t			nowh;

	nowh = time(NULL);
	now  = localtime(&nowh);

	gn_sms_default_submit(&sms);

	sms.SMSCTime.Year	= now->tm_year;
	sms.SMSCTime.Month	= now->tm_mon+1;
	sms.SMSCTime.Day	= now->tm_mday;
	sms.SMSCTime.Hour	= now->tm_hour;
	sms.SMSCTime.Minute	= now->tm_min;
	sms.SMSCTime.Second	= now->tm_sec;
	sms.SMSCTime.Timezone	= 0;

	sms.SMSCTime.Year += 1900;

	/* the nokia 7110 will choke if no number is present when we */
	/* try to store a SMS on the phone. maybe others do too */
	/* TODO should this be handled here? report an error instead */
	/* of setting an default? */
	strcpy(sms.Remote.Number, "0");
	sms.Remote.Type = SMS_International;
	sms.Number = 0;
	input_len = GSM_MAX_SMS_LENGTH;

	optarg = NULL;
	optind = 0;
	argv--;
	argc++;

	/* Option parsing */
	while ((i = getopt_long(argc, argv, "0:1:2:3:rsf:id", options, NULL)) != -1) {
		switch (i) {
		case '0': /* SMSC number */
			snprintf(sms.SMSC.Number, sizeof(sms.SMSC.Number) - 1, "%s", optarg);
			if (sms.SMSC.Number[0] == '+') 
				sms.SMSC.Type = SMS_International;
			else
				sms.SMSC.Type = SMS_Unknown;
			break;
		case '1': /* SMSC number index in phone memory */
			data.MessageCenter = calloc(1, sizeof(SMS_MessageCenter));
			data.MessageCenter->No = atoi(optarg);
			if (data.MessageCenter->No < 1 || data.MessageCenter->No > 5) {
				free(data.MessageCenter);
				sendsms_usage();
			}
			if (SM_Functions(GOP_GetSMSCenter, &data, &State) == GN_ERR_NONE) {
				strcpy(sms.SMSC.Number, data.MessageCenter->SMSC.Number);
				sms.SMSC.Type = data.MessageCenter->SMSC.Type;
			}
			free(data.MessageCenter);
			break;
		case '2': /* sender number */
			if (*optarg == '+')
				sms.Remote.Type = SMS_International;
			else
				sms.Remote.Type = SMS_Unknown;
			snprintf(sms.Remote.Number, MAX_NUMBER_LEN, "%s", optarg);
			break;
		case '3': /* location to write to */
			sms.Number = atoi(optarg);
			break;
		case 's': /* mark the message as sent */
		case 'r': /* mark the message as read */
			sms.Status = SMS_Sent;
			break;
#if 0
		case 'i': /* Ask before overwriting */
			interactive = 1;
			break;
#endif
		case 'f': /* Specify the folder where to save the message */
			snprintf(memory_type, 19, "%s", optarg);
			if (StrToMemoryType(memory_type) == GMT_XX) {
				fprintf(stderr, _("Unknown memory type %s (use ME, SM, ...)!\n"), optarg);
				return -1;
			}
			break;
		case 'd': /* type Deliver */
			sms.Type = SMS_Deliver;
			break;
		default:
			usage(stderr);
			return -1;
		}
	}
#if 0
	if (interactive) {
		GSM_API_SMS aux;

		aux.Number = sms.Number;
		data.SMS = &aux;
		error = SM_Functions(GOP_GetSMSnoValidate, &data, &State);
		switch (error) {
		case GN_ERR_NONE:
			fprintf(stderr, _("Message at specified location exists. "));
			while (confirm < 0) {
				fprintf(stderr, _("Overwrite? (yes/no) "));
				GetLine(stdin, ans, 7);
				if (!strcmp(ans, _("yes"))) confirm = 1;
				else if (!strcmp(ans, _("no"))) confirm = 0;
			}
			if (!confirm) {
				return 0;
			}
			else break;
		case GN_ERR_EMPTYLOCATION: /* OK. We can save now. */
			break;
		default:
			fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
			return -1;
		}
	}
#endif
	if ((!sms.SMSC.Number[0]) && (sms.Type == SMS_Deliver)) {
		data.MessageCenter = calloc(1, sizeof(SMS_MessageCenter));
		data.MessageCenter->No = 1;
		if (SM_Functions(GOP_GetSMSCenter, &data, &State) == GN_ERR_NONE) {
			snprintf(sms.SMSC.Number, MAX_NUMBER_LEN, "%s", data.MessageCenter->SMSC.Number);
			dprintf("SMSC number: %s\n", sms.SMSC.Number);
			sms.SMSC.Type = data.MessageCenter->SMSC.Type;
		}
		free(data.MessageCenter);
	}

	if (!sms.SMSC.Type) sms.SMSC.Type = SMS_Unknown;

	fprintf(stderr, _("Please enter SMS text. End your input with <cr><control-D>:"));

	chars_read = fread(message_buffer, 1, sizeof(message_buffer), stdin);

	fprintf(stderr, _("storing sms"));

	if (chars_read == 0) {
		fprintf(stderr, _("Couldn't read from stdin!\n"));
		return -1;
	} else if (chars_read > input_len || chars_read > sizeof(sms.UserData[0].u.Text) - 1) {
		fprintf(stderr, _("Input too long!\n"));
		return -1;
	}

	if (chars_read > 0 && message_buffer[chars_read - 1] == '\n') message_buffer[--chars_read] = 0x00;
	if (chars_read < 1) {
		fprintf(stderr, _("Empty message. Quitting"));
		return -1;
	}
	if (memory_type[0] != '\0')
		sms.MemoryType = StrToMemoryType(memory_type);

	sms.UserData[0].Type = SMS_PlainText;
	strncpy(sms.UserData[0].u.Text, message_buffer, chars_read);
	sms.UserData[0].u.Text[chars_read] = 0;
	sms.UserData[1].Type = SMS_NoData;
	if (!gn_char_def_alphabet(sms.UserData[0].u.Text))
		sms.DCS.u.General.Alphabet = SMS_UCS2;

	data.SMS = &sms;
	error = gn_sms_save(&data, &State);

	if (error == GN_ERR_NONE)
		fprintf(stderr, _("Saved to %d!\n"), sms.Number);
	else
		fprintf(stderr, _("Saving failed (%s)\n"), gn_error_print(error));

	return error;
}

/* Get SMSC number */
static int getsmsc(int argc, char *argv[])
{
	SMS_MessageCenter MessageCenter;
	GSM_Data data;
	gn_error error;
	int start, stop, i;
	bool raw = false;
	struct option options[] = {
		{ "raw",    no_argument, NULL, 'r'},
		{ NULL,     0,           NULL, 0}
	};

	optarg = NULL;
	optind = 0;
	argv++;
	argc--;

	while ((i = getopt_long(argc, argv, "r", options, NULL)) != -1) {
		switch (i) {
		case 'r':
			raw = true;
			break;
		default:
			usage(stderr); /* FIXME */
			return -1;
		}
	}

	if (argc > optind) {
		start = atoi(argv[optind]);
		stop = (argc > optind+1) ? atoi(argv[optind+1]) : start;

		if (start > stop) {
			fprintf(stderr, _("Starting SMS center number is greater than stop\n"));
			return -1;
		}

		if (start < 1) {
			fprintf(stderr, _("SMS center number must be greater than 1\n"));
			return -1;
		}
	} else {
		start = 1;
		stop = 5;	/* FIXME: determine it */
	}

	GSM_DataClear(&data);
	data.MessageCenter = &MessageCenter;

	for (i = start; i <= stop; i++) {
		memset(&MessageCenter, 0, sizeof(MessageCenter));
		MessageCenter.No = i;

		error = SM_Functions(GOP_GetSMSCenter, &data, &State);
		switch (error) {
		case GN_ERR_NONE:
			break;
		default:
			fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
			return error;
		}

		if (raw) {
			fprintf(stdout, "%d;%s;%d;%d;%d;%d;%s;%d;%s\n",
				MessageCenter.No, MessageCenter.Name,
				MessageCenter.DefaultName, MessageCenter.Format,
				MessageCenter.Validity,
				MessageCenter.SMSC.Type, MessageCenter.SMSC.Number,
				MessageCenter.Recipient.Type, MessageCenter.Recipient.Number);
		} else {
			if (MessageCenter.DefaultName == -1)
				fprintf(stdout, _("No. %d: \"%s\" (defined name)\n"), MessageCenter.No, MessageCenter.Name);
			else
				fprintf(stdout, _("No. %d: \"%s\" (default name)\n"), MessageCenter.No, MessageCenter.Name);
			fprintf(stdout, _("SMS center number is %s\n"), MessageCenter.SMSC.Number);
			fprintf(stdout, _("Default recipient number is %s\n"), MessageCenter.Recipient.Number);
			fprintf(stdout, _("Messages sent as "));

			switch (MessageCenter.Format) {
			case SMS_FText:
				fprintf(stdout, _("Text"));
				break;
			case SMS_FVoice:
				fprintf(stdout, _("VoiceMail"));
				break;
			case SMS_FFax:
				fprintf(stdout, _("Fax"));
				break;
			case SMS_FEmail:
			case SMS_FUCI:
				fprintf(stdout, _("Email"));
				break;
			case SMS_FERMES:
				fprintf(stdout, _("ERMES"));
				break;
			case SMS_FX400:
				fprintf(stdout, _("X.400"));
				break;
			default:
				fprintf(stdout, _("Unknown"));
				break;
			}

			fprintf(stdout, "\n");
			fprintf(stdout, _("Message validity is "));

			switch (MessageCenter.Validity) {
			case SMS_V1H:
				fprintf(stdout, _("1 hour"));
				break;
			case SMS_V6H:
				fprintf(stdout, _("6 hours"));
				break;
			case SMS_V24H:
				fprintf(stdout, _("24 hours"));
				break;
			case SMS_V72H:
				fprintf(stdout, _("72 hours"));
				break;
			case SMS_V1W:
				fprintf(stdout, _("1 week"));
				break;
			case SMS_VMax:
				fprintf(stdout, _("Maximum time"));
				break;
			default:
				fprintf(stdout, _("Unknown"));
				break;
			}

			fprintf(stdout, "\n");
		}
	}

	return GN_ERR_NONE;
}

/* Set SMSC number */
static int setsmsc()
{
	SMS_MessageCenter MessageCenter;
	GSM_Data data;
	gn_error error;
	char line[256], ch;
	int n;

	GSM_DataClear(&data);
	data.MessageCenter = &MessageCenter;

	while (fgets(line, sizeof(line), stdin)) {
		n = strlen(line);
		if (n > 0 && line[n-1] == '\n') {
			line[--n] = 0;
		}

		memset(&MessageCenter, 0, sizeof(MessageCenter));
		n = sscanf(line, "%d;%19[^;];%d;%d;%d;%d;%39[^;];%d;%39[^;\n]%c",
			   &MessageCenter.No, MessageCenter.Name,
			   &MessageCenter.DefaultName,
			   (int *)&MessageCenter.Format, (int *)&MessageCenter.Validity,
			   (int *)&MessageCenter.SMSC.Type, MessageCenter.SMSC.Number,
			   (int *)&MessageCenter.Recipient.Type, MessageCenter.Recipient.Number,
			   &ch);
		switch (n) {
		case 6:
			MessageCenter.SMSC.Number[0] = 0;
		case 7:
			MessageCenter.Recipient.Number[0] = 0;
		case 8:
			break;
		default:
			fprintf(stderr, _("Input line format isn't valid\n"));
			return GN_ERR_UNKNOWN;
		}

		error = SM_Functions(GOP_SetSMSCenter, &data, &State);
		if (error != GN_ERR_NONE) {
			fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
			return error;
		}
	}

	return GN_ERR_NONE;
}

/* Creating SMS folder. */
static int createsmsfolder(char *Name)
{
	SMS_Folder	folder;
	GSM_Data	data;
	gn_error	error;

	GSM_DataClear(&data);

	snprintf(folder.Name, MAX_SMS_FOLDER_NAME_LENGTH, "%s", Name);
	data.SMSFolder = &folder;

	error = SM_Functions(GOP_CreateSMSFolder, &data, &State);
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		return error;
	} else 
		fprintf(stderr, _("Folder with name: %s successfully created!\n"), folder.Name);
	return GN_ERR_NONE;
}

/* Creating SMS folder. */
static int deletesmsfolder(char *Number)
{
	SMS_Folder	folder;
	GSM_Data	data;
	gn_error	error;

	GSM_DataClear(&data);

	folder.FolderID = atoi(Number);
	if (folder.Number > 0 && folder.Number < MAX_SMS_FOLDERS + 1)
		data.SMSFolder = &folder;
	else
		fprintf(stderr, _("Error: Number must be between 1 and %i!\n"), MAX_SMS_FOLDERS);

	error = SM_Functions(GOP_DeleteSMSFolder, &data, &State);
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		return error;
	} else 
		fprintf(stderr, _("Number %i of 'My Folders' successfully deleted!\n"), folder.Number);
	return GN_ERR_NONE;
}

/* Get SMS messages. */
static int getsms(int argc, char *argv[])
{
	int del = 0;
	SMS_Folder folder;
	SMS_FolderList folderlist;
	GSM_API_SMS message;
	char *memory_type_string;
	int start_message, end_message, count, mode = 1;
	char filename[64];
	gn_error error;
	gn_bmp bitmap;
	GSM_Information *info = &State.Phone.Info;
	char ans[5];
	struct stat buf;

	/* Handle command line args that set type, start and end locations. */
	memory_type_string = argv[2];
	if (StrToMemoryType(memory_type_string) == GMT_XX) {
		fprintf(stderr, _("Unknown memory type %s (use ME, SM, ...)!\n"), argv[2]);
		return -1;
	}

	memset(&filename, 0, sizeof(filename));

	start_message = end_message = atoi(argv[3]);
	if (argc > 4) {
		int i;

		/* [end] can be only argv[4] */
		if (argv[4][0] != '-') {
			end_message = atoi(argv[4]);
		}

		/* parse all options (beginning with '-' */
		while ((i = getopt(argc, argv, "f:F:d")) != -1) {
			switch (i) {
			case 'd':
				del = 1;
				break;
			case 'F':
				mode = 0;
			case 'f':
				if (optarg) {
					fprintf(stderr, _("Saving into %s\n"), optarg);
					memset(&filename, 0, sizeof(filename));
					strncpy(filename, optarg, sizeof(filename) - 1);
					if (strlen(optarg) > sizeof(filename) - 1)
						fprintf(stderr, _("Filename too long - will be truncated to 63 characters.\n"));
				} else usage(stderr);
				break;
			default:
				usage(stderr);
			}
		}
	}
	folder.FolderID = 0;
	data.SMSFolder = &folder;
	data.SMSFolderList = &folderlist;
	/* Now retrieve the requested entries. */
	for (count = start_message; count <= end_message; count++) {
		bool done = false;
		int offset = 0;

		memset(&message, 0, sizeof(GSM_API_SMS));
		message.MemoryType = StrToMemoryType(memory_type_string);
		message.Number = count;
		data.SMS = &message;

		error = gn_sms_get(&data, &State);
		switch (error) {
		case GN_ERR_NONE:
			switch (message.Type) {
			case SMS_Submit:
				fprintf(stdout, _("%d. MO Message "), message.Number);
				switch (message.Status) {
				case SMS_Read:
					fprintf(stdout, _("(read)\n"));
					break;
				case SMS_Unread:
					fprintf(stdout, _("(unread)\n"));
					break;
				case SMS_Sent:
					fprintf(stdout, _("(sent)\n"));
					break;
				case SMS_Unsent:
					fprintf(stdout, _("(unsent)\n"));
					break;
				default:
					fprintf(stdout, _("(read)\n"));
					break;
				}
				fprintf(stdout, _("Text: %s\n\n"), message.UserData[0].u.Text);
				break;
			case SMS_Delivery_Report:
				fprintf(stdout, _("%d. Delivery Report "), message.Number);
				if (message.Status)
					fprintf(stdout, _("(read)\n"));
				else
					fprintf(stdout, _("(not read)\n"));
				fprintf(stdout, _("Sending date/time: %02d/%02d/%04d %02d:%02d:%02d "), \
					message.SMSCTime.Day, message.SMSCTime.Month, message.SMSCTime.Year, \
					message.SMSCTime.Hour, message.SMSCTime.Minute, message.SMSCTime.Second);
				if (message.SMSCTime.Timezone) {
					if (message.SMSCTime.Timezone > 0)
						fprintf(stdout,_("+%02d00"), message.SMSCTime.Timezone);
					else
						fprintf(stdout,_("%02d00"), message.SMSCTime.Timezone);
				}
				fprintf(stdout, "\n");
				fprintf(stdout, _("Response date/time: %02d/%02d/%04d %02d:%02d:%02d "), \
					message.Time.Day, message.Time.Month, message.Time.Year, \
					message.Time.Hour, message.Time.Minute, message.Time.Second);
				if (message.SMSCTime.Timezone) {
					if (message.Time.Timezone > 0)
						fprintf(stdout,_("+%02d00"), message.Time.Timezone);
					else
						fprintf(stdout,_("%02d00"), message.Time.Timezone);
				}
				fprintf(stdout, "\n");
				fprintf(stdout, _("Receiver: %s Msg Center: %s\n"), message.Remote.Number, message.SMSC.Number);
				fprintf(stdout, _("Text: %s\n\n"), message.UserData[0].u.Text);
				break;
			case SMS_Picture:
				fprintf(stdout, _("Picture Message\n"));
				fprintf(stdout, _("Date/time: %02d/%02d/%04d %02d:%02d:%02d "), \
					message.SMSCTime.Day, message.SMSCTime.Month, message.SMSCTime.Year, \
					message.SMSCTime.Hour, message.SMSCTime.Minute, message.SMSCTime.Second);
				if (message.SMSCTime.Timezone) {
					if (message.SMSCTime.Timezone > 0)
						fprintf(stdout,_("+%02d00"), message.SMSCTime.Timezone);
					else
						fprintf(stdout,_("%02d00"), message.SMSCTime.Timezone);
				}
				fprintf(stdout, "\n");
				fprintf(stdout, _("Sender: %s Msg Center: %s\n"), message.Remote.Number, message.SMSC.Number);
				fprintf(stdout, _("Bitmap:\n"));
				gn_bmp_print(&message.UserData[0].u.Bitmap, stdout);
				fprintf(stdout, _("Text:\n"));
				fprintf(stdout, "%s\n", message.UserData[1].u.Text);
				break;
			default:
				fprintf(stdout, _("%d. Inbox Message "), message.Number);
				switch (message.Status) {
				case SMS_Read:
					fprintf(stdout, _("(read)\n"));
					break;
				case SMS_Unread:
					fprintf(stdout, _("(unread)\n"));
					break;
				case SMS_Sent:
					fprintf(stdout, _("(sent)\n"));
					break;
				case SMS_Unsent:
					fprintf(stdout, _("(unsent)\n"));
					break;
				default:
					fprintf(stdout, _("(read)\n"));
					break;
				}
				fprintf(stdout, _("Date/time: %02d/%02d/%04d %02d:%02d:%02d "), \
					message.SMSCTime.Day, message.SMSCTime.Month, message.SMSCTime.Year, \
					message.SMSCTime.Hour, message.SMSCTime.Minute, message.SMSCTime.Second);
				if (message.SMSCTime.Timezone) {
					if (message.SMSCTime.Timezone > 0)
						fprintf(stdout,_("+%02d00"), message.SMSCTime.Timezone);
					else
						fprintf(stdout,_("%02d00"), message.SMSCTime.Timezone);
				}
				fprintf(stdout, "\n");
				fprintf(stdout, _("Sender: %s Msg Center: %s\n"), message.Remote.Number, message.SMSC.Number);
				/* No UDH */
				if (!message.UDH.Number) message.UDH.UDH[0].Type = SMS_NoUDH;
				switch (message.UDH.UDH[0].Type) {
				case SMS_NoUDH:
					fprintf(stdout, _("Text:\n"));
					break;
				case SMS_OpLogo:
					fprintf(stdout, _("GSM operator logo for %s (%s) network.\n"), bitmap.netcode, GSM_GetNetworkName(bitmap.netcode));
					if (!strcmp(message.Remote.Number, "+998000005") && !strcmp(message.SMSC.Number, "+886935074443")) fprintf(stdout, _("Saved by Logo Express\n"));
					if (!strcmp(message.Remote.Number, "+998000002") || !strcmp(message.Remote.Number, "+998000003")) fprintf(stdout, _("Saved by Operator Logo Uploader by Thomas Kessler\n"));
					offset = 3;
				case SMS_CallerIDLogo:
					fprintf(stdout, _("Logo:\n"));
					/* put bitmap into bitmap structure */
					gn_bmp_read_sms(GN_BMP_OperatorLogo, message.UserData[0].u.Text + 2 + offset, message.UserData[0].u.Text, &bitmap);
					gn_bmp_print(&bitmap, stdout);
					if (*filename) {
						error = GN_ERR_NONE;
						if ((stat(filename, &buf) == 0)) {
							fprintf(stdout, _("File %s exists.\n"), filename);
							fprintf(stdout, _("Overwrite? (yes/no) "));
							GetLine(stdin, ans, 4);
							if (!strcmp(ans, _("yes"))) {
								error = GSM_SaveBitmapFile(filename, &bitmap, info);
							}
						} else error = GSM_SaveBitmapFile(filename, &bitmap, info);
						if (error != GN_ERR_NONE) fprintf(stderr, _("Couldn't save logofile %s!\n"), filename);
					}
					done = true;
					break;
				case SMS_Ringtone:
					fprintf(stdout, _("Ringtone\n"));
					done = true;
					break;
				case SMS_ConcatenatedMessages:
					fprintf(stdout, _("Linked (%d/%d):\n"),
						message.UDH.UDH[0].u.ConcatenatedShortMessage.CurrentNumber,
						message.UDH.UDH[0].u.ConcatenatedShortMessage.MaximumNumber);
					break;
				case SMS_WAPvCard:
					fprintf(stdout, _("WAP vCard:\n"));
					break;
				case SMS_WAPvCalendar:
					fprintf(stdout, _("WAP vCalendar:\n"));
					break;
				case SMS_WAPvCardSecure:
					fprintf(stdout, _("WAP vCardSecure:\n"));
					break;
				case SMS_WAPvCalendarSecure:
					fprintf(stdout, _("WAP vCalendarSecure:\n"));
					break;
				default:
					fprintf(stderr, _("Unknown\n"));
					break;
				}
				if (done) break;
				fprintf(stdout, "%s\n", message.UserData[0].u.Text);
				if ((mode != -1) && *filename) {
					char buf[1024];
					sprintf(buf, "%s%d", filename, count);
					mode = GSM_SaveTextFile(buf, message.UserData[0].u.Text, mode);
				}
				break;
			}
			if (del) {
				data.SMS = &message;
				if (GN_ERR_NONE != gn_sms_delete(&data, &State))
					fprintf(stderr, _("(delete failed)\n"));
				else
					fprintf(stderr, _("(message deleted)\n"));
			}
			break;
		default:
			fprintf(stderr, _("GetSMS %s %d failed! (%s)\n"), memory_type_string, count, gn_error_print(error));
			if (error == GN_ERR_INVALIDMEMORYTYPE)
				fprintf(stderr, _("See the gnokii manual page for the supported memory types with the phone\nyou use\n"));
			break;
		}
	}

	return 0;
}

/* Delete SMS messages. */
static int deletesms(int argc, char *argv[])
{
	GSM_API_SMS message;
	SMS_Folder folder;
	SMS_FolderList folderlist;
	char *memory_type_string;
	int start_message, end_message, count;
	gn_error error;

	/* Handle command line args that set type, start and end locations. */
	memory_type_string = argv[0];
	message.MemoryType = StrToMemoryType(memory_type_string);
	if (message.MemoryType == GMT_XX) {
		fprintf(stderr, _("Unknown memory type %s (use ME, SM, ...)!\n"), argv[0]);
		return -1;
	}

	start_message = end_message = atoi(argv[1]);
	if (argc > 2) end_message = atoi(argv[2]);

	/* Now delete the requested entries. */
	for (count = start_message; count <= end_message; count++) {
		message.Number = count;
		data.SMS = &message;
		data.SMSFolder = &folder;
		data.SMSFolderList = &folderlist;
		error = gn_sms_delete(&data, &State);

		if (error == GN_ERR_NONE)
			fprintf(stderr, _("Deleted SMS %s %d\n"), memory_type_string, count);
		else {
			fprintf(stderr, _("DeleteSMS %s %d failed!(%s)\n\n"), memory_type_string, count, gn_error_print(error));
			return error;
		}
	}

	return 0;
}

static volatile bool bshutdown = false;

/* SIGINT signal handler. */
static void interrupted(int sig)
{
	signal(sig, SIG_IGN);
	bshutdown = true;
}

#ifdef SECURITY

int get_password(const char *prompt, char *pass, int length)
{
#ifdef WIN32
	fprintf(stdout, "%s", prompt);
	fgets(pass, length, stdin);
#else
	/* FIXME: manual says: Do not use it */
	strncpy(pass, getpass(prompt), length - 1);
	pass[length - 1] = 0;
#endif

	return 0;
}

/* In this mode we get the code from the keyboard and send it to the mobile
   phone. */
static int entersecuritycode(char *type)
{
	gn_error error;
	GSM_SecurityCode SecurityCode;

	if (!strcmp(type, "PIN"))
		SecurityCode.Type = GSCT_Pin;
	else if (!strcmp(type, "PUK"))
		SecurityCode.Type = GSCT_Puk;
	else if (!strcmp(type, "PIN2"))
		SecurityCode.Type = GSCT_Pin2;
	else if (!strcmp(type, "PUK2"))
		SecurityCode.Type = GSCT_Puk2;
	/* FIXME: Entering of SecurityCode does not work :-(
	else if (!strcmp(type, "SecurityCode"))
		SecurityCode.Type = GSCT_SecurityCode;
	*/
	else
		usage(stderr);

	memset(&SecurityCode.Code, 0, sizeof(SecurityCode.Code));
	get_password(_("Enter your code: "), SecurityCode.Code, sizeof(SecurityCode.Code));

	GSM_DataClear(&data);
	data.SecurityCode = &SecurityCode;

	error = SM_Functions(GOP_EnterSecurityCode, &data, &State);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stderr, _("Code ok.\n"));
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}
	return error;
}

static int getsecuritycodestatus(void)
{
	GSM_SecurityCode SecurityCode;

	GSM_DataClear(&data);
	data.SecurityCode = &SecurityCode;

	if (SM_Functions(GOP_GetSecurityCodeStatus, &data, &State) == GN_ERR_NONE) {

		fprintf(stdout, _("Security code status: "));

		switch(SecurityCode.Type) {
		case GSCT_SecurityCode:
			fprintf(stdout, _("waiting for Security Code.\n"));
			break;
		case GSCT_Pin:
			fprintf(stdout, _("waiting for PIN.\n"));
			break;
		case GSCT_Pin2:
			fprintf(stdout, _("waiting for PIN2.\n"));
			break;
		case GSCT_Puk:
			fprintf(stdout, _("waiting for PUK.\n"));
			break;
		case GSCT_Puk2:
			fprintf(stdout, _("waiting for PUK2.\n"));
			break;
		case GSCT_None:
			fprintf(stdout, _("nothing to enter.\n"));
			break;
		default:
			fprintf(stdout, _("Unknown!\n"));
			break;
		}
	}

	return 0;
}

static int changesecuritycode(char *type)
{
	gn_error error;
	GSM_SecurityCode SecurityCode;
	char newcode2[10];

	memset(&SecurityCode, 0, sizeof(SecurityCode));

	if (!strcmp(type, "PIN"))
		SecurityCode.Type = GSCT_Pin;
	else if (!strcmp(type, "PUK"))
		SecurityCode.Type = GSCT_Puk;
	else if (!strcmp(type, "PIN2"))
		SecurityCode.Type = GSCT_Pin2;
	else if (!strcmp(type, "PUK2"))
		SecurityCode.Type = GSCT_Puk2;
	/* FIXME: Entering of SecurityCode does not work :-(
	else if (!strcmp(type, "SecurityCode"))
		SecurityCode.Type = GSCT_SecurityCode;
	*/
	else
		usage(stderr);

	get_password(_("Enter your code: "), SecurityCode.Code, sizeof(SecurityCode.Code));
	get_password(_("Enter new code: "), SecurityCode.NewCode, sizeof(SecurityCode.NewCode));
	get_password(_("Retype new code: "), newcode2, sizeof(newcode2));
	if (strcmp(SecurityCode.NewCode, newcode2)) {
		fprintf(stdout, _("Error: new code differ\n"));
		return 1;
	}

	GSM_DataClear(&data);
	data.SecurityCode = &SecurityCode;

	error = SM_Functions(GOP_ChangeSecurityCode, &data, &State);
	switch (error) {
	case GN_ERR_NONE:
		fprintf(stderr, _("Code changed.\n"));
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

#endif

static void callnotifier(GSM_CallStatus call_status, GSM_CallInfo *call_info, GSM_Statemachine *state)
{
	switch (call_status) {
	case GSM_CS_IncomingCall:
		fprintf(stdout, _("INCOMING CALL: ID: %d, Number: %s, Name: \"%s\"\n"), call_info->CallID, call_info->Number, call_info->Name);
		break;
	case GSM_CS_LocalHangup:
		fprintf(stdout, _("CALL %d TERMINATED (LOCAL)\n"), call_info->CallID);
		break;
	case GSM_CS_RemoteHangup:
		fprintf(stdout, _("CALL %d TERMINATED (REMOTE)\n"), call_info->CallID);
		break;
	case GSM_CS_Established:
		fprintf(stdout, _("CALL %d ACCEPTED BY THE REMOTE SIDE\n"), call_info->CallID);
		break;
	case GSM_CS_CallHeld:
		fprintf(stdout, _("CALL %d PLACED ON HOLD\n"), call_info->CallID);
		break;
	case GSM_CS_CallResumed:
		fprintf(stdout, _("CALL %d RETRIEVED FROM HOLD\n"), call_info->CallID);
		break;
	default:
		break;
	}

	gn_call_notifier(call_status, call_info, state);
}

/* Voice dialing mode. */
static int dialvoice(char *number)
{
    	GSM_CallInfo call_info;
	gn_error error;
	int call_id;

	memset(&call_info, 0, sizeof(call_info));
	snprintf(call_info.Number, sizeof(call_info.Number), "%s", number);
	call_info.Type = GSM_CT_VoiceCall;
	call_info.SendNumber = GSM_CSN_Default;

	GSM_DataClear(&data);
	data.CallInfo = &call_info;

	if ((error = gn_call_dial(&call_id, &data, &State)) != GN_ERR_NONE) {
		fprintf(stderr, _("Dialing failed: %s\n"), gn_error_print(error));
	    	return error;
	}

	fprintf(stdout, _("Dialled call, id: %d (lowlevel id: %d)\n"), call_id, call_info.CallID);

	return 0;
}

/* Answering incoming call */
static int answercall(char *CallID)
{
    	GSM_CallInfo CallInfo;

	memset(&CallInfo, 0, sizeof(CallInfo));
	CallInfo.CallID = atoi(CallID);

	GSM_DataClear(&data);
	data.CallInfo = &CallInfo;

	return SM_Functions(GOP_AnswerCall, &data, &State);
}

/* Hangup the call */
static int hangup(char *CallID)
{
    	GSM_CallInfo CallInfo;

	memset(&CallInfo, 0, sizeof(CallInfo));
	CallInfo.CallID = atoi(CallID);

	GSM_DataClear(&data);
	data.CallInfo = &CallInfo;

	return SM_Functions(GOP_CancelCall, &data, &State);
}


static gn_bmp_types set_bitmap_type(char *s)
{
	if (!strcmp(s, "text"))         return GN_BMP_WelcomeNoteText;
	if (!strcmp(s, "dealer"))       return GN_BMP_DealerNoteText;
	if (!strcmp(s, "op"))           return GN_BMP_OperatorLogo;
	if (!strcmp(s, "startup"))      return GN_BMP_StartupLogo;
	if (!strcmp(s, "caller"))       return GN_BMP_CallerLogo;
	if (!strcmp(s, "picture"))      return GN_BMP_PictureMessage;
	if (!strcmp(s, "emspicture"))   return GN_BMP_EMSPicture;
	if (!strcmp(s, "emsanimation")) return GN_BMP_EMSAnimation;
	return GN_BMP_None;
}

/* FIXME: Integrate with sendsms */
/* The following function allows to send logos using SMS */
static int sendlogo(int argc, char *argv[])
{
	GSM_API_SMS sms;
	gn_error error;
	int type;

	gn_sms_default_submit(&sms);

	/* The first argument is the type of the logo. */
	switch (type = set_bitmap_type(argv[0])) {
	case GN_BMP_OperatorLogo:   fprintf(stderr, _("Sending operator logo.\n")); break;
	case GN_BMP_CallerLogo:     fprintf(stderr, _("Sending caller line identification logo.\n")); break;
	case GN_BMP_PictureMessage: fprintf(stderr,_("Sending Multipart Message: Picture Message.\n")); break;
	case GN_BMP_EMSPicture:     fprintf(stderr, _("Sending EMS-compliant Picture Message.\n")); break;
	case GN_BMP_EMSAnimation:   fprintf(stderr, _("Sending EMS-compliant Animation.\n")); break;
	default: 	            fprintf(stderr, _("You should specify what kind of logo to send!\n")); return -1;
	}

	sms.UserData[0].Type = SMS_BitmapData;

	/* The second argument is the destination, ie the phone number of recipient. */
	memset(&sms.Remote.Number, 0, sizeof(sms.Remote.Number));
	strncpy(sms.Remote.Number, argv[1], sizeof(sms.Remote.Number) - 1);
	if (sms.Remote.Number[0] == '+') sms.Remote.Type = SMS_International;
	else sms.Remote.Type = SMS_Unknown;

	if (loadbitmap(&sms.UserData[0].u.Bitmap, argv[2], type) != GN_ERR_NONE)
		return -1;

	/* If we are sending op logo we can rewrite network code. */
	if ((sms.UserData[0].u.Bitmap.type == GN_BMP_OperatorLogo) && (argc > 3)) {
		/*
		 * The fourth argument, if present, is the Network code of the operator.
		 * Network code is in this format: "xxx yy".
		 */
		memset(sms.UserData[0].u.Bitmap.netcode, 0, sizeof(sms.UserData[0].u.Bitmap.netcode));
		strncpy(sms.UserData[0].u.Bitmap.netcode, argv[3], sizeof(sms.UserData[0].u.Bitmap.netcode) - 1);
		dprintf("Operator code: %s\n", argv[3]);
	}

	sms.UserData[1].Type = SMS_NoData;
	if (sms.UserData[0].u.Bitmap.type == GN_BMP_PictureMessage) {
		sms.UserData[1].Type = SMS_NokiaText;
		readtext(&sms.UserData[1], 120);
		sms.UserData[2].Type = SMS_NoData;
	}

	data.MessageCenter = calloc(1, sizeof(SMS_MessageCenter));
	data.MessageCenter->No = 1;
	if (SM_Functions(GOP_GetSMSCenter, &data, &State) == GN_ERR_NONE) {
		strcpy(sms.SMSC.Number, data.MessageCenter->SMSC.Number);
		sms.SMSC.Type = data.MessageCenter->SMSC.Type;
	}
	free(data.MessageCenter);

	if (!sms.SMSC.Type) sms.SMSC.Type = SMS_Unknown;

	/* Send the message. */
	data.SMS = &sms;
	error = gn_sms_send(&data, &State);

	if (error == GN_ERR_NONE) fprintf(stderr, _("Send succeeded!\n"));
	else fprintf(stderr, _("SMS Send failed (%s)\n"), gn_error_print(error));

	return error;
}

/* Getting logos. */
static gn_error SaveBitmapFileDialog(char *FileName, gn_bmp *bitmap, GSM_Information *info)
{
	int confirm;
	char ans[4];
	struct stat buf;
	gn_error error;

	/* Ask before overwriting */
	while (stat(FileName, &buf) == 0) {
		confirm = 0;
		while (!confirm) {
			fprintf(stderr, _("Saving logo. File \"%s\" exists. (O)verwrite, create (n)ew or (s)kip ? "), FileName);
			GetLine(stdin, ans, 4);
			if (!strcmp(ans, _("O")) || !strcmp(ans, _("o"))) confirm = 1;
			if (!strcmp(ans, _("N")) || !strcmp(ans, _("n"))) confirm = 2;
			if (!strcmp(ans, _("S")) || !strcmp(ans, _("s"))) return GN_ERR_USERCANCELED;
		}
		if (confirm == 1) break;
		if (confirm == 2) {
			fprintf(stderr, _("Enter name of new file: "));
			GetLine(stdin, FileName, 50);
			if (!FileName || (*FileName == 0)) return GN_ERR_USERCANCELED;
		}
	}

	error = GSM_SaveBitmapFile(FileName, bitmap, info);

	switch (error) {
	case GN_ERR_FAILED:
		fprintf(stderr, _("Failed to write file \"%s\"\n"), FileName);
		break;
	default:
		break;
	}

	return error;
}

static int getlogo(int argc, char *argv[])
{
	gn_bmp bitmap;
	gn_error error;
	GSM_Information *info = &State.Phone.Info;

	memset(&bitmap, 0, sizeof(gn_bmp));
	bitmap.type = set_bitmap_type(argv[0]);

	/* There is caller group number missing in argument list. */
	if ((bitmap.type == GN_BMP_CallerLogo) && (argc == 3)) {
		bitmap.number = (argv[2][0] < '0') ? 0 : argv[2][0] - '0';
		if (bitmap.number > 9) bitmap.number = 0;
	}

	if (bitmap.type != GN_BMP_None) {
		data.Bitmap = &bitmap;
		error = SM_Functions(GOP_GetBitmap, &data, &State);

		gn_bmp_print(&bitmap, stderr);
		switch (error) {
		case GN_ERR_NONE:
			switch (bitmap.type) {
			case GN_BMP_DealerNoteText:
				fprintf(stdout, _("Dealer welcome note "));
				if (bitmap.text[0]) fprintf(stdout, _("currently set to \"%s\"\n"), bitmap.text);
				else fprintf(stdout, _("currently empty\n"));
				break;
			case GN_BMP_WelcomeNoteText:
				fprintf(stdout, _("Welcome note "));
				if (bitmap.text[0]) fprintf(stdout, _("currently set to \"%s\"\n"), bitmap.text);
				else fprintf(stdout, _("currently empty\n"));
				break;
			case GN_BMP_OperatorLogo:
			case GN_BMP_NewOperatorLogo:
				if (!bitmap.width) goto empty_bitmap;
				fprintf(stdout, _("Operator logo for %s (%s) network got succesfully\n"),
					bitmap.netcode, GSM_GetNetworkName(bitmap.netcode));
				if (argc == 3) {
					strncpy(bitmap.netcode, argv[2], sizeof(bitmap.netcode) - 1);
					if (!strcmp(GSM_GetNetworkName(bitmap.netcode), "unknown")) {
						fprintf(stderr, _("Sorry, gnokii doesn't know %s network !\n"), bitmap.netcode);
						return -1;
					}
				}
				break;
			case GN_BMP_StartupLogo:
				if (!bitmap.width) goto empty_bitmap;
				fprintf(stdout, _("Startup logo got successfully\n"));
				if (argc == 3) {
					strncpy(bitmap.netcode, argv[2], sizeof(bitmap.netcode) - 1);
					if (!strcmp(GSM_GetNetworkName(bitmap.netcode), "unknown")) {
						fprintf(stderr, _("Sorry, gnokii doesn't know %s network !\n"), bitmap.netcode);
						return -1;
					}
				}
				break;
			case GN_BMP_CallerLogo:
				if (!bitmap.width) goto empty_bitmap;
				fprintf(stdout, _("Caller logo got successfully\n"));
				if (argc == 4) {
					strncpy(bitmap.netcode, argv[3], sizeof(bitmap.netcode) - 1);
					if (!strcmp(GSM_GetNetworkName(bitmap.netcode), "unknown")) {
						fprintf(stderr, _("Sorry, gnokii doesn't know %s network !\n"), bitmap.netcode);
						return -1;
					}
				}
				break;
			default:
				fprintf(stderr, _("Unknown bitmap type.\n"));
				break;
			empty_bitmap:
				fprintf(stderr, _("Your phone doesn't have logo uploaded !\n"));
				return -1;
			}
			if ((argc > 1) && (SaveBitmapFileDialog(argv[1], &bitmap, info) != GN_ERR_NONE)) return -1;
			break;
		default:
			fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
			return -1;
		}
	} else {
		fprintf(stderr, _("What kind of logo do you want to get ?\n"));
		return -1;
	}

	return 0;
}


/* Sending logos. */
gn_error ReadBitmapFileDialog(char *FileName, gn_bmp *bitmap, GSM_Information *info)
{
	gn_error error;

	error = GSM_ReadBitmapFile(FileName, bitmap, info);
	switch (error) {
	case GN_ERR_NONE:
		break;
	default:
		fprintf(stderr, _("Error while reading file \"%s\": %s\n"), FileName, gn_error_print(error));
		break;
	}
	return error;
}


static int setlogo(int argc, char *argv[])
{
	gn_bmp bitmap, oldbit;
	GSM_NetworkInfo NetworkInfo;
	gn_error error = GN_ERR_NOTSUPPORTED;
	GSM_Information *info = &State.Phone.Info;
	GSM_Data data;
	bool ok = true;
	int i;

	GSM_DataClear(&data);
	data.Bitmap = &bitmap;
	data.NetworkInfo = &NetworkInfo;

	memset(&bitmap.text, 0, sizeof(bitmap.text));

	bitmap.type = set_bitmap_type(argv[0]);
	switch (bitmap.type) {
	case GN_BMP_WelcomeNoteText:
	case GN_BMP_DealerNoteText:
		if (argc > 1) strncpy(bitmap.text, argv[1], sizeof(bitmap.text) - 1);
		break;
	case GN_BMP_OperatorLogo:
		error = (argc < 2) ? gn_bmp_null(&bitmap, info) : ReadBitmapFileDialog(argv[1], &bitmap, info);
		if (error != GN_ERR_NONE) return -1;
			
		memset(&bitmap.netcode, 0, sizeof(bitmap.netcode));
		if (argc < 3)
			if (SM_Functions(GOP_GetNetworkInfo, &data, &State) == GN_ERR_NONE) 
				strncpy(bitmap.netcode, NetworkInfo.NetworkCode, sizeof(bitmap.netcode) - 1);

		if (!strncmp(State.Phone.Info.Models, "6510", 4))
			gn_bmp_resize(&bitmap, GN_BMP_NewOperatorLogo, info);
		else
			gn_bmp_resize(&bitmap, GN_BMP_OperatorLogo, info);

		if (argc == 3) {
			strncpy(bitmap.netcode, argv[2], sizeof(bitmap.netcode) - 1);
			if (!strcmp(GSM_GetNetworkName(bitmap.netcode), "unknown")) {
				fprintf(stderr, _("Sorry, gnokii doesn't know %s network !\n"), bitmap.netcode);
				return -1;
			}
		}
		break;
	case GN_BMP_StartupLogo:
		if ((argc > 1) && (ReadBitmapFileDialog(argv[1], &bitmap, info) != GN_ERR_NONE)) return -1;
		gn_bmp_resize(&bitmap, GN_BMP_StartupLogo, info);
		break;
	case GN_BMP_CallerLogo:
		if ((argc > 1) && (ReadBitmapFileDialog(argv[1], &bitmap, info) != GN_ERR_NONE)) return -1;
		gn_bmp_resize(&bitmap, GN_BMP_CallerLogo, info);
		if (argc > 2) {
			bitmap.number = argv[2][0] - '0';
			dprintf("%i \n", bitmap.number);
			if ((bitmap.number < 0) || (bitmap.number > 9)) bitmap.number = 0;
			oldbit.type = GN_BMP_CallerLogo;
			oldbit.number = bitmap.number;
			data.Bitmap = &oldbit;
			if (SM_Functions(GOP_GetBitmap, &data, &State) == GN_ERR_NONE) {
				/* We have to get the old name and ringtone!! */
				bitmap.ringtone = oldbit.ringtone;
				strncpy(bitmap.text, oldbit.text, sizeof(bitmap.text) - 1);
			}
			if (argc > 3) strncpy(bitmap.text, argv[3], sizeof(bitmap.text) - 1);
		}
		break;
	default:
		fprintf(stderr, _("What kind of logo do you want to set ?\n"));
		return -1;
	}

	data.Bitmap = &bitmap;
	error = SM_Functions(GOP_SetBitmap, &data, &State);

	switch (error) {
	case GN_ERR_NONE:
		oldbit.type = bitmap.type;
		oldbit.number = bitmap.number;
		data.Bitmap = &oldbit;
		if (SM_Functions(GOP_GetBitmap, &data, &State) == GN_ERR_NONE) {
			switch (bitmap.type) {
			case GN_BMP_WelcomeNoteText:
			case GN_BMP_DealerNoteText:
				if (strcmp(bitmap.text, oldbit.text)) {
					fprintf(stderr, _("Error setting"));
					if (bitmap.type == GN_BMP_DealerNoteText) fprintf(stderr, _(" dealer"));
					fprintf(stderr, _(" welcome note - "));

					/* I know, it looks horrible, but...
					 * I set it to the short string - if it won't be set
					 * it means, PIN is required. If it will be correct, previous
					 * (user) text was too long.
					 *
					 * Without it, I could have such thing:
					 * user set text to very short string (for example, "Marcin")
					 * then enable phone without PIN and try to set it to the very long (too long for phone)
					 * string (which start with "Marcin"). If we compare them as only length different, we could think,
					 * that phone accepts strings 6 chars length only (length of "Marcin")
					 * When we make it correct, we don't have this mistake
					 */

					strcpy(oldbit.text, "!");
					data.Bitmap = &oldbit;
					SM_Functions(GOP_SetBitmap, &data, &State);
					SM_Functions(GOP_GetBitmap, &data, &State);
					if (oldbit.text[0] != '!') {
						fprintf(stderr, _("SIM card and PIN is required\n"));
					} else {
						data.Bitmap = &bitmap;
						SM_Functions(GOP_SetBitmap, &data, &State);
						data.Bitmap = &oldbit;
						SM_Functions(GOP_GetBitmap, &data, &State);
						fprintf(stderr, _("too long, truncated to \"%s\" (length %i)\n"),oldbit.text,strlen(oldbit.text));
					}
					ok = false;
				}
				break;
			case GN_BMP_StartupLogo:
				for (i = 0; i < oldbit.size; i++) {
					if (oldbit.bitmap[i] != bitmap.bitmap[i]) {
						fprintf(stderr, _("Error setting startup logo - SIM card and PIN is required\n"));
						fprintf(stderr, _("i: %i, %2x %2x\n"), i, oldbit.bitmap[i], bitmap.bitmap[i]);
						ok = false;
						break;
					}
				}
				break;
			default:
				break;

			}
		}
		if (ok) fprintf(stderr, _("Done.\n"));
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return 0;
}

static int viewlogo(char *filename)
{
	gn_error error;
	error = GSM_ShowBitmapFile(filename);
	if (error != GN_ERR_NONE)
		fprintf(stderr, _("Could not load bitmap: %s\n"), gn_error_print(error));
	return error;
}

/* ToDo notes receiving. */
static int gettodo(int argc, char *argv[])
{
	GSM_ToDoList	ToDoList;
	GSM_ToDo	ToDo;
	GSM_Data	data;
	gn_error	error = GN_ERR_NONE;
	bool		vCal = false;
	int		i, first_location, last_location;

	struct option options[] = {
		{ "vCard",   optional_argument, NULL, '1'},
		{ NULL,      0,                 NULL, 0}
	};

	optarg = NULL;
	optind = 0;

	first_location = last_location = atoi(argv[0]);
	if ((argc > 1) && (argv[1][0] != '-')) {
		last_location = atoi(argv[1]);
	}

	while ((i = getopt_long(argc, argv, "v", options, NULL)) != -1) {
		switch (i) {
		case 'v':
			vCal = true;
			break;
		default:
			usage(stderr); /* Would be better to have an calendar_usage() here. */
			return -1;
		}
	}

	for (i = first_location; i <= last_location; i++) {
		ToDo.Location = i;

		GSM_DataClear(&data);
		data.ToDo = &ToDo;
		data.ToDoList = &ToDoList;

		error = SM_Functions(GOP_GetToDo, &data, &State);
		switch (error) {
		case GN_ERR_NONE:
			if (vCal) {
				fprintf(stdout, "BEGIN:VCALENDAR\r\n");
				fprintf(stdout, "VERSION:1.0\r\n");
				fprintf(stdout, "BEGIN:VTODO\r\n");
				fprintf(stdout, "PRIORITY:%i\r\n", ToDo.Priority);
				fprintf(stdout, "SUMMARY:%s\r\n", ToDo.Text);
				fprintf(stdout, "END:VTODO\r\n");
				fprintf(stdout, "END:VCALENDAR\r\n");
			} else {
				fprintf(stdout, _("Todo: %s\n"), ToDo.Text);
				fprintf(stdout, _("Priority: "));
				switch (ToDo.Priority) {
				case GTD_LOW:
					fprintf(stdout, _("low\n"));
					break;
				case GTD_MEDIUM:
					fprintf(stdout, _("medium\n"));
					break;
				case GTD_HIGH:
					fprintf(stdout, _("high\n"));
					break;
				default:
					fprintf(stdout, _("unknown\n"));
					break;
				}
			}
			break;
		default:
			fprintf(stderr, _("The ToDo note could not be read: %s\n"), gn_error_print(error));
			break;
		}
	}
	return error;
}

/* ToDo notes writing */
static int writetodo(char *argv[])
{
	GSM_ToDo ToDo;
	GSM_Data data;
	gn_error error;

	GSM_DataClear(&data);
	data.ToDo = &ToDo;

#ifndef WIN32
	error = GSM_ReadVCalendarFileTodo(argv[0], &ToDo, atoi(argv[1]));
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Failed to load vCalendar file: %s\n"), gn_error_print(error));
		return -1;
	}
#endif

	error = SM_Functions(GOP_WriteToDo, &data, &State);
	if (error == GN_ERR_NONE)
		fprintf(stderr, _("Succesfully written!\n"));
	else {
		fprintf(stderr, _("Failed to write calendar note: %s\n"), gn_error_print(error));
		return -1;
	}
	return 0;
}

/* Deleting all ToDo notes */
static int deletealltodos()
{
	GSM_Data data;
	gn_error error;

	GSM_DataClear(&data);

	error = SM_Functions(GOP_DeleteAllToDos, &data, &State);
	if (error == GN_ERR_NONE)
		fprintf(stderr, _("Succesfully deleted all ToDo notes!\n"));
	else {
		fprintf(stderr, _("Failed to write calendar note: %s\n"), gn_error_print(error));
		return -1;
	}
	return 0;
}

/* Calendar notes receiving. */
static int getcalendarnote(int argc, char *argv[])
{
	GSM_CalendarNotesList	CalendarNotesList;
	GSM_CalendarNote	CalendarNote;
	GSM_Data		data;
	gn_error		error = GN_ERR_NONE;
	int			i, first_location, last_location;
	bool			vCal = false;

	struct option options[] = {
		{ "vCard",   optional_argument, NULL, '1'},
		{ NULL,      0,                 NULL, 0}
	};

	optarg = NULL;
	optind = 0;

	first_location = last_location = atoi(argv[0]);
	if ((argc > 1) && (argv[1][0] != '-')) {
		last_location = atoi(argv[1]);
	}

	while ((i = getopt_long(argc, argv, "v", options, NULL)) != -1) {
		switch (i) {
		case 'v':
			vCal = true;
			break;
		default:
			usage(stderr); /* Would be better to have an calendar_usage() here. */
			return -1;
		}
	}

	for (i = first_location; i <= last_location; i++) {
		CalendarNote.Location = i;

		GSM_DataClear(&data);
		data.CalendarNote = &CalendarNote;
		data.CalendarNotesList = &CalendarNotesList;

		error = SM_Functions(GOP_GetCalendarNote, &data, &State);
		switch (error) {
		case GN_ERR_NONE:
			if (vCal) {
				fprintf(stdout, "BEGIN:VCALENDAR\r\n");
				fprintf(stdout, "VERSION:1.0\r\n");
				fprintf(stdout, "BEGIN:VEVENT\r\n");
				fprintf(stdout, "CATEGORIES:");
				switch (CalendarNote.Type) {
				case GCN_REMINDER:
					fprintf(stdout, "MISCELLANEOUS\r\n");
					break;
				case GCN_CALL:
					fprintf(stdout, "PHONE CALL\r\n");
					fprintf(stdout, "SUMMARY:%s\r\n", CalendarNote.Phone);
					fprintf(stdout, "DESCRIPTION:%s\r\n", CalendarNote.Text);
					break;
				case GCN_MEETING:
					fprintf(stdout, "MEETING\r\n");
					break;
				case GCN_BIRTHDAY:
					fprintf(stdout, "SPECIAL OCCASION\r\n");
					break;
				default:
					fprintf(stdout, "UNKNOWN\r\n");
					break;
				}
				if (CalendarNote.Type != GCN_CALL) fprintf(stdout, "SUMMARY:%s\r\n",CalendarNote.Text);
				fprintf(stdout, "DTSTART:%04d%02d%02dT%02d%02d%02d\r\n", CalendarNote.Time.Year,
					CalendarNote.Time.Month, CalendarNote.Time.Day, CalendarNote.Time.Hour,
					CalendarNote.Time.Minute, CalendarNote.Time.Second);
				if (CalendarNote.Alarm.Year != 0) {
					fprintf(stdout, "DALARM:%04d%02d%02dT%02d%02d%02d\r\n", CalendarNote.Alarm.Year,
						CalendarNote.Alarm.Month, CalendarNote.Alarm.Day, CalendarNote.Alarm.Hour,
						CalendarNote.Alarm.Minute, CalendarNote.Alarm.Second);
				}
				fprintf(stdout, "END:VEVENT\r\n");
				fprintf(stdout, "END:VCALENDAR\r\n");

			} else {  /* not vCal */
				fprintf(stdout, _("   Type of the note: "));

				switch (CalendarNote.Type) {
				case GCN_REMINDER:
					fprintf(stdout, _("Reminder\n"));
					break;
				case GCN_CALL:
					fprintf(stdout, _("Call\n"));
					break;
				case GCN_MEETING:
					fprintf(stdout, _("Meeting\n"));
					break;
				case GCN_BIRTHDAY:
					fprintf(stdout, _("Birthday\n"));
					break;
				default:
					fprintf(stdout, _("Unknown\n"));
					break;
				}

				fprintf(stdout, _("   Date: %d-%02d-%02d\n"), CalendarNote.Time.Year,
					CalendarNote.Time.Month,
					CalendarNote.Time.Day);

				fprintf(stdout, _("   Time: %02d:%02d:%02d\n"), CalendarNote.Time.Hour,
					CalendarNote.Time.Minute,
					CalendarNote.Time.Second);

				if (CalendarNote.Alarm.AlarmEnabled == 1) {
					fprintf(stdout, _("   Alarm date: %d-%02d-%02d\n"), CalendarNote.Alarm.Year,
						CalendarNote.Alarm.Month,
						CalendarNote.Alarm.Day);

					fprintf(stdout, _("   Alarm time: %02d:%02d:%02d\n"), CalendarNote.Alarm.Hour,
						CalendarNote.Alarm.Minute,
						CalendarNote.Alarm.Second);
				}

				fprintf(stdout, _("   Text: %s\n"), CalendarNote.Text);

				if (CalendarNote.Type == GCN_CALL)
					fprintf(stdout, _("   Phone: %s\n"), CalendarNote.Phone);
			}
			break;
		default:
			fprintf(stderr, _("The calendar note can not be read: %s\n"), gn_error_print(error));
			break;
		}
	}

	return error;
}

/* Writing calendar notes. */
static int writecalendarnote(char *argv[])
{
	GSM_CalendarNote CalendarNote;
	GSM_Data data;
	gn_error error;

	GSM_DataClear(&data);
	data.CalendarNote = &CalendarNote;

#ifndef WIN32
	error = GSM_ReadVCalendarFileEvent(argv[0], &CalendarNote, atoi(argv[1]));
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Failed to load vCalendar file: %s\n"), gn_error_print(error));
		return -1;
	}
#endif

	error = SM_Functions(GOP_WriteCalendarNote, &data, &State);
	if (error == GN_ERR_NONE)
		fprintf(stderr, _("Succesfully written!\n"));
	else {
		fprintf(stderr, _("Failed to write calendar note: %s\n"), gn_error_print(error));
		return -1;
	}
	return 0;
}

/* Calendar note deleting. */
static int deletecalendarnote(int argc, char *argv[])
{
	GSM_CalendarNote CalendarNote;
	int i, first_location, last_location;
	GSM_Data data;
	gn_error error = GN_ERR_NONE;

	GSM_DataClear(&data);
	data.CalendarNote = &CalendarNote;

	first_location = last_location = atoi(argv[0]);
	if (argc > 1) last_location = atoi(argv[1]);

	for (i = first_location; i <= last_location; i++) {

		CalendarNote.Location = i;

		error = SM_Functions(GOP_DeleteCalendarNote, &data, &State);
		if (error == GN_ERR_NONE) {
			fprintf(stderr, _("Calendar note deleted.\n"));
		} else {
			fprintf(stderr, _("The calendar note cannot be deleted: %s\n"), gn_error_print(error));
		}

	}

	return error;
}

/* Setting the date and time. */
static int setdatetime(int argc, char *argv[])
{
	struct tm *now;
	time_t nowh;
	GSM_DateTime Date;
	gn_error error;

	nowh = time(NULL);
	now = localtime(&nowh);

	Date.Year = now->tm_year;
	Date.Month = now->tm_mon+1;
	Date.Day = now->tm_mday;
	Date.Hour = now->tm_hour;
	Date.Minute = now->tm_min;
	Date.Second = now->tm_sec;

	if (argc > 0) Date.Year = atoi(argv[0]);
	if (argc > 1) Date.Month = atoi(argv[1]);
	if (argc > 2) Date.Day = atoi(argv[2]);
	if (argc > 3) Date.Hour = atoi (argv[3]);
	if (argc > 4) Date.Minute = atoi(argv[4]);

	if (Date.Year < 1900) {

		/* Well, this thing is copyrighted in U.S. This technique is known as
		   Windowing and you can read something about it in LinuxWeekly News:
		   http://lwn.net/1999/features/Windowing.phtml. This thing is beeing
		   written in Czech republic and Poland where algorithms are not allowed
		   to be patented. */

		if (Date.Year > 90)
			Date.Year = Date.Year + 1900;
		else
			Date.Year = Date.Year + 2000;
	}

	GSM_DataClear(&data);
	data.DateTime = &Date;
	error = SM_Functions(GOP_SetDateTime, &data, &State);

	return error;
}

/* In this mode we receive the date and time from mobile phone. */
static int getdatetime(void)
{
	GSM_Data	data;
	GSM_DateTime	date_time;
	gn_error	error;

	GSM_DataClear(&data);
	data.DateTime = &date_time;

	error = SM_Functions(GOP_GetDateTime, &data, &State);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stdout, _("Date: %4d/%02d/%02d\n"), date_time.Year, date_time.Month, date_time.Day);
		fprintf(stdout, _("Time: %02d:%02d:%02d\n"), date_time.Hour, date_time.Minute, date_time.Second);
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* Setting the alarm. */
static int setalarm(int argc, char *argv[])
{
	GSM_DateTime Date;
	gn_error error;

	if (argc == 2) {
		Date.Hour = atoi(argv[0]);
		Date.Minute = atoi(argv[1]);
		Date.Second = 0;
		Date.AlarmEnabled = true;
	} else {
		Date.Hour = 0;
		Date.Minute = 0;
		Date.Second = 0;
		Date.AlarmEnabled = false;
	}

	GSM_DataClear(&data);
	data.DateTime = &Date;

	error = SM_Functions(GOP_SetAlarm, &data, &State);

	return 0;
}

/* Getting the alarm. */
static int getalarm(void)
{
	gn_error	error;
	GSM_Data	data;
	GSM_DateTime	date_time;

	GSM_DataClear(&data);
	data.DateTime = &date_time;

	error = SM_Functions(GOP_GetAlarm, &data, &State);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stdout, _("Alarm: %s\n"), (date_time.AlarmEnabled==0)?"off":"on");
		fprintf(stdout, _("Time: %02d:%02d\n"), date_time.Hour, date_time.Minute);
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

static void StoreCBMessage(GSM_CBMessage *Message )
{
	if (CBQueue[cb_widx].New) {
		/* queue is full */
		return;
	}

	CBQueue[cb_widx] = *Message;
	cb_widx = (cb_widx + 1) % (sizeof(CBQueue) / sizeof(GSM_CBMessage));
}

static gn_error ReadCBMessage(GSM_CBMessage *Message)
{
	if (!CBQueue[cb_ridx].New)
		return GN_ERR_NONEWCBRECEIVED;
	
	*Message = CBQueue[cb_ridx];
	CBQueue[cb_ridx].New = false;
	cb_ridx = (cb_ridx + 1) % (sizeof(CBQueue) / sizeof(GSM_CBMessage));

	return GN_ERR_NONE;
}

static void DisplayCall(int call_id)
{
	gn_call *call;
	struct timeval now, delta;
	char *s;

	if ((call = gn_call_get_active(call_id)) == NULL)
	{
		fprintf(stdout, _("CALL%d: IDLE\n"), call_id);
		return;
	}

	gettimeofday(&now, NULL);
	switch (call->Status) {
	case GN_CALL_Ringing:
		s = "RINGING";
		timersub(&now, &call->StartTime, &delta);
		break;
	case GN_CALL_Dialing:
		s = "DIALING";
		timersub(&now, &call->StartTime, &delta);
		break;
	case GN_CALL_Established:
		s = "ESTABLISHED";
		timersub(&now, &call->AnswerTime, &delta);
		break;
	case GN_CALL_Held:
		s = "ON HOLD";
		timersub(&now, &call->AnswerTime, &delta);
		break;
	default:
		s = "UNKNOWN STATE";
		memset(&delta, 0, sizeof(delta));
		break;
	}

	fprintf(stderr, _("CALL%d: %s %s(%s) (duration: %d sec)\n"), call_id, s,
		call->RemoteNumber, call->RemoteName,
		(int)delta.tv_sec);
}

/* In monitor mode we don't do much, we just initialise the fbus code.
   Note that the fbus code no longer has an internal monitor mode switch,
   instead compile with DEBUG enabled to get all the gumpf. */
static int monitormode(void)
{
	float rflevel = -1, batterylevel = -1;
	GSM_PowerSource powersource = -1;
	GSM_RFUnits rf_units = GRF_Arbitrary;
	GSM_BatteryUnits batt_units = GBU_Arbitrary;
	GSM_Data data;
	int i;

	GSM_NetworkInfo NetworkInfo;
	GSM_CBMessage CBMessage;
	GSM_MemoryStatus SIMMemoryStatus   = {GMT_SM, 0, 0};
	GSM_MemoryStatus PhoneMemoryStatus = {GMT_ME, 0, 0};
	GSM_MemoryStatus DC_MemoryStatus   = {GMT_DC, 0, 0};
	GSM_MemoryStatus EN_MemoryStatus   = {GMT_EN, 0, 0};
	GSM_MemoryStatus FD_MemoryStatus   = {GMT_FD, 0, 0};
	GSM_MemoryStatus LD_MemoryStatus   = {GMT_LD, 0, 0};
	GSM_MemoryStatus MC_MemoryStatus   = {GMT_MC, 0, 0};
	GSM_MemoryStatus ON_MemoryStatus   = {GMT_ON, 0, 0};
	GSM_MemoryStatus RC_MemoryStatus   = {GMT_RC, 0, 0};

	SMS_Status SMSStatus = {0, 0, 0, 0};
	/*
	char Number[20];
	*/
	GSM_DataClear(&data);

	/* We do not want to monitor serial line forever - press Ctrl+C to stop the
	   monitoring mode. */
	signal(SIGINT, interrupted);

	fprintf(stderr, _("Entering monitor mode...\n"));

	/* Loop here indefinitely - allows you to see messages from GSM code in
	   response to unknown messages etc. The loops ends after pressing the
	   Ctrl+C. */
	data.RFUnits = &rf_units;
	data.RFLevel = &rflevel;
	data.BatteryUnits = &batt_units;
	data.BatteryLevel = &batterylevel;
	data.PowerSource = &powersource;
	data.SMSStatus = &SMSStatus;
	data.NetworkInfo = &NetworkInfo;
	data.OnCellBroadcast = StoreCBMessage;
	data.CallNotification = callnotifier;

	SM_Functions(GOP_SetCallNotification, &data, &State);

	memset(CBQueue, 0, sizeof(CBQueue));
	cb_ridx = 0;
	cb_widx = 0;
	SM_Functions(GOP_SetCellBroadcast, &data, &State);

	while (!bshutdown) {
		if (SM_Functions(GOP_GetRFLevel, &data, &State) == GN_ERR_NONE)
			fprintf(stdout, _("RFLevel: %d\n"), (int)rflevel);

		if (SM_Functions(GOP_GetBatteryLevel, &data, &State) == GN_ERR_NONE)
			fprintf(stdout, _("Battery: %d\n"), (int)batterylevel);

		if (SM_Functions(GOP_GetPowersource, &data, &State) == GN_ERR_NONE)
			fprintf(stdout, _("Power Source: %s\n"), (powersource == GPS_ACDC) ? _("AC/DC") : _("battery"));

		data.MemoryStatus = &SIMMemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus, &data, &State) == GN_ERR_NONE)
			fprintf(stdout, _("SIM: Used %d, Free %d\n"), SIMMemoryStatus.Used, SIMMemoryStatus.Free);

		data.MemoryStatus = &PhoneMemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus, &data, &State) == GN_ERR_NONE)
			fprintf(stdout, _("Phone: Used %d, Free %d\n"), PhoneMemoryStatus.Used, PhoneMemoryStatus.Free);

		data.MemoryStatus = &DC_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus, &data, &State) == GN_ERR_NONE)
			fprintf(stdout, _("DC: Used %d, Free %d\n"), DC_MemoryStatus.Used, DC_MemoryStatus.Free);

		data.MemoryStatus = &EN_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus, &data, &State) == GN_ERR_NONE)
			fprintf(stdout, _("EN: Used %d, Free %d\n"), EN_MemoryStatus.Used, EN_MemoryStatus.Free);

		data.MemoryStatus = &FD_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus, &data, &State) == GN_ERR_NONE)
			fprintf(stdout, _("FD: Used %d, Free %d\n"), FD_MemoryStatus.Used, FD_MemoryStatus.Free);

		data.MemoryStatus = &LD_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus, &data, &State) == GN_ERR_NONE)
			fprintf(stdout, _("LD: Used %d, Free %d\n"), LD_MemoryStatus.Used, LD_MemoryStatus.Free);

		data.MemoryStatus = &MC_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus, &data, &State) == GN_ERR_NONE)
			fprintf(stdout, _("MC: Used %d, Free %d\n"), MC_MemoryStatus.Used, MC_MemoryStatus.Free);

		data.MemoryStatus = &ON_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus, &data, &State) == GN_ERR_NONE)
			fprintf(stdout, _("ON: Used %d, Free %d\n"), ON_MemoryStatus.Used, ON_MemoryStatus.Free);

		data.MemoryStatus = &RC_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus, &data, &State) == GN_ERR_NONE)
			fprintf(stdout, _("RC: Used %d, Free %d\n"), RC_MemoryStatus.Used, RC_MemoryStatus.Free);

		if (SM_Functions(GOP_GetSMSStatus, &data, &State) == GN_ERR_NONE)
			fprintf(stdout, _("SMS Messages: Unread %d, Number %d\n"), SMSStatus.Unread, SMSStatus.Number);

		if (SM_Functions(GOP_GetNetworkInfo, &data, &State) == GN_ERR_NONE)
			fprintf(stdout, _("Network: %s (%s), LAC: %02x%02x, CellID: %02x%02x\n"), GSM_GetNetworkName (NetworkInfo.NetworkCode), GSM_GetCountryName(NetworkInfo.NetworkCode), NetworkInfo.LAC[0], NetworkInfo.LAC[1], NetworkInfo.CellID[0], NetworkInfo.CellID[1]);

		for (i = 0; i < GN_CALL_MAX_PARALLEL; i++)
			DisplayCall(i);

		if (ReadCBMessage(&CBMessage) == GN_ERR_NONE)
			fprintf(stdout, _("Cell broadcast received on channel %d: %s\n"), CBMessage.Channel, CBMessage.Message);

		sleep(1);
	}

	data.OnCellBroadcast = NULL;
	SM_Functions(GOP_SetCellBroadcast, &data, &State);

	fprintf(stderr, _("Leaving monitor mode...\n"));

	return 0;
}


static void PrintDisplayStatus(int Status)
{
	fprintf(stdout, _("Call in progress: %-3s\n"),
		Status & (1 << DS_Call_In_Progress) ? _("on") : _("off"));
	fprintf(stdout, _("Unknown: %-3s\n"),
		Status & (1 << DS_Unknown)          ? _("on") : _("off"));
	fprintf(stdout, _("Unread SMS: %-3s\n"),
		Status & (1 << DS_Unread_SMS)       ? _("on") : _("off"));
	fprintf(stdout, _("Voice call: %-3s\n"),
		Status & (1 << DS_Voice_Call)       ? _("on") : _("off"));
	fprintf(stdout, _("Fax call active: %-3s\n"),
		Status & (1 << DS_Fax_Call)         ? _("on") : _("off"));
	fprintf(stdout, _("Data call active: %-3s\n"),
		Status & (1 << DS_Data_Call)        ? _("on") : _("off"));
	fprintf(stdout, _("Keyboard lock: %-3s\n"),
		Status & (1 << DS_Keyboard_Lock)    ? _("on") : _("off"));
	fprintf(stdout, _("SMS storage full: %-3s\n"),
		Status & (1 << DS_SMS_Storage_Full) ? _("on") : _("off"));
}

#define ESC "\e"

static void NewOutputFn(GSM_DrawMessage *DrawMessage)
{
	int x, y, n;
	static int status;
	static unsigned char screen[DRAW_MAX_SCREEN_HEIGHT][DRAW_MAX_SCREEN_WIDTH];
	static bool init = false;

	if (!init) {
		for (y = 0; y < DRAW_MAX_SCREEN_HEIGHT; y++)
			for (x = 0; x < DRAW_MAX_SCREEN_WIDTH; x++)
				screen[y][x] = ' ';
		status = 0;
		init = true;
	}

	fprintf(stdout, ESC "[1;1H");

	switch (DrawMessage->Command) {
	case GSM_Draw_ClearScreen:
		for (y = 0; y < DRAW_MAX_SCREEN_HEIGHT; y++)
			for (x = 0; x < DRAW_MAX_SCREEN_WIDTH; x++)
				screen[y][x] = ' ';
		break;

	case GSM_Draw_DisplayText:
		x = DrawMessage->Data.DisplayText.x*DRAW_MAX_SCREEN_WIDTH / 84;
		y = DrawMessage->Data.DisplayText.y*DRAW_MAX_SCREEN_HEIGHT / 48;
		n = strlen(DrawMessage->Data.DisplayText.text);
		if (n > DRAW_MAX_SCREEN_WIDTH)
			return;
		if (x + n > DRAW_MAX_SCREEN_WIDTH)
			x = DRAW_MAX_SCREEN_WIDTH - n;
		if (y > DRAW_MAX_SCREEN_HEIGHT)
			y = DRAW_MAX_SCREEN_HEIGHT - 1;
		memcpy(&screen[y][x], DrawMessage->Data.DisplayText.text, n);
		break;

	case GSM_Draw_DisplayStatus:
		status = DrawMessage->Data.DisplayStatus;
		break;

	default:
		return;
	}

	for (y = 0; y < DRAW_MAX_SCREEN_HEIGHT; y++) {
		for (x = 0; x < DRAW_MAX_SCREEN_WIDTH; x++)
			fprintf(stdout, "%c", screen[y][x]);
		fprintf(stdout, "\n");
	}

	fprintf(stdout, "\n");
	PrintDisplayStatus(status);
}

static void console_raw(void)
{
#ifndef WIN32
	struct termios it;

	tcgetattr(fileno(stdin), &it);
	it.c_lflag &= ~(ICANON);
	//it.c_iflag &= ~(INPCK|ISTRIP|IXON);
	it.c_cc[VMIN] = 1;
	it.c_cc[VTIME] = 0;

	tcsetattr(fileno(stdin), TCSANOW, &it);
#endif
}

static int displayoutput(void)
{
	GSM_Data data;
	gn_error error;
	GSM_DisplayOutput output;

	GSM_DataClear(&data);
	memset(&output, 0, sizeof(output));
	output.OutputFn = NewOutputFn;
	data.DisplayOutput = &output;

	error = SM_Functions(GOP_DisplayOutput, &data, &State);
	console_raw();
#ifndef WIN32
	fcntl(fileno(stdin), F_SETFL, O_NONBLOCK);
#endif

	switch (error) {
	case GN_ERR_NONE:
		/* We do not want to see texts forever - press Ctrl+C to stop. */
		signal(SIGINT, interrupted);

		fprintf (stderr, _("Entered display monitoring mode...\n"));
		fprintf (stderr, ESC "c" );

		/* Loop here indefinitely - allows you to read texts from phone's
		   display. The loops ends after pressing the Ctrl+C. */
		while (!bshutdown) {
			char buf[105];
			memset(&buf[0], 0, 102);
			while (read(0, buf, 100) > 0) {
				/*
				fprintf(stderr, _("handling keys (%d).\n"), strlen(buf));
				if (GSM && GSM->HandleString && GSM->HandleString(buf) != GN_ERR_NONE)
					fprintf(stdout, _("Key press simulation failed.\n"));
				*/
				memset(buf, 0, 102);
			}
			SM_Loop(&State, 1);
			SM_Functions(GOP_PollDisplay, &data, &State);
		}
		fprintf (stderr, _("Shutting down\n"));

		fprintf (stderr, _("Leaving display monitor mode...\n"));

		output.OutputFn = NULL;
		error = SM_Functions(GOP_DisplayOutput, &data, &State);
		if (error != GN_ERR_NONE)
			fprintf (stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	default:
		fprintf (stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return 0;
}

/* Reads profile from phone and displays its' settings */
static int getprofile(int argc, char *argv[])
{
	int max_profiles;
	int start, stop, i;
	GSM_Profile p;
	gn_error error = GN_ERR_NOTSUPPORTED;
	GSM_Data data;

	/* Hopefully is 64 larger as FB38_MAX* / FB61_MAX* */
	char model[GSM_MAX_MODEL_LENGTH];
	bool raw = false;
	struct option options[] = {
		{ "raw",    no_argument, NULL, 'r'},
		{ NULL,     0,           NULL, 0}
	};

	optarg = NULL;
	optind = 0;
	argv++;
	argc--;

	while ((i = getopt_long(argc, argv, "r", options, NULL)) != -1) {
		switch (i) {
		case 'r':
			raw = true;
			break;
		default:
			usage(stderr); /* FIXME */
			return -1;
		}
	}

	GSM_DataClear(&data);
	data.Model = model;
	while (SM_Functions(GOP_GetModel, &data, &State) != GN_ERR_NONE)
		sleep(1);

	p.Number = 0;
	GSM_DataClear(&data);
	data.Profile = &p;
	error = SM_Functions(GOP_GetProfile, &data, &State);

	switch (error) {
	case GN_ERR_NONE:
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		return -1;
	}

	max_profiles = 7; /* This is correct for 6110 (at least my). How do we get
			     the number of profiles? */

	/* For N5110 */
	/* FIXME: It should be set to 3 for N5130 and 3210 too */
	if (!strcmp(model, "NSE-1"))
		max_profiles = 3;

	if (argc > optind) {
		start = atoi(argv[optind]);
		stop = (argc > optind + 1) ? atoi(argv[optind + 1]) : start;

		if (start > stop) {
			fprintf(stderr, _("Starting profile number is greater than stop\n"));
			return -1;
		}

		if (start < 0) {
			fprintf(stderr, _("Profile number must be value from 0 to %d!\n"), max_profiles-1);
			return -1;
		}

		if (stop >= max_profiles) {
			fprintf(stderr, _("This phone supports only %d profiles!\n"), max_profiles);
			return -1;
		}
	} else {
		start = 0;
		stop = max_profiles - 1;
	}

	GSM_DataClear(&data);
	data.Profile = &p;

	for (i = start; i <= stop; i++) {
		p.Number = i;

		if (p.Number != 0) {
			error = SM_Functions(GOP_GetProfile, &data, &State);
			if (error != GN_ERR_NONE) {
				fprintf(stderr, _("Cannot get profile %d\n"), i);
				return -1;
			}
		}

		if (raw) {
			fprintf(stdout, "%d;%s;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d\n",
				p.Number, p.Name, p.DefaultName, p.KeypadTone,
				p.Lights, p.CallAlert, p.Ringtone, p.Volume,
				p.MessageTone, p.Vibration, p.WarningTone,
				p.CallerGroups, p.AutomaticAnswer);
		} else {
			fprintf(stdout, "%d. \"%s\"\n", p.Number, p.Name);
			if (p.DefaultName == -1) fprintf(stdout, _(" (name defined)\n"));
			fprintf(stdout, _("Incoming call alert: %s\n"), GetProfileCallAlertString(p.CallAlert));
			/* For different phones different ringtones names */
			if (!strcmp(model, "NSE-3"))
				fprintf(stdout, _("Ringing tone: %s (%d)\n"), RingingTones[p.Ringtone], p.Ringtone);
			else
				fprintf(stdout, _("Ringtone number: %d\n"), p.Ringtone);
			fprintf(stdout, _("Ringing volume: %s\n"), GetProfileVolumeString(p.Volume));
			fprintf(stdout, _("Message alert tone: %s\n"), GetProfileMessageToneString(p.MessageTone));
			fprintf(stdout, _("Keypad tones: %s\n"), GetProfileKeypadToneString(p.KeypadTone));
			fprintf(stdout, _("Warning and game tones: %s\n"), GetProfileWarningToneString(p.WarningTone));

			/* FIXME: Light settings is only used for Car */
			if (p.Number == (max_profiles - 2)) fprintf(stdout, _("Lights: %s\n"), p.Lights ? _("On") : _("Automatic"));
			fprintf(stdout, _("Vibration: %s\n"), GetProfileVibrationString(p.Vibration));

			/* FIXME: it will be nice to add here reading caller group name. */
			if (max_profiles != 3) fprintf(stdout, _("Caller groups: 0x%02x\n"), p.CallerGroups);

			/* FIXME: Automatic answer is only used for Car and Headset. */
			if (p.Number >= (max_profiles - 2)) fprintf(stdout, _("Automatic answer: %s\n"), p.AutomaticAnswer ? _("On") : _("Off"));
			fprintf(stdout, "\n");
		}
	}

	return 0;
}

/* Writes profiles to phone */
static int setprofile()
{
	int n;
	GSM_Profile p;
	gn_error error = GN_ERR_NONE;
	GSM_Data data;
	char line[256], ch;

	GSM_DataClear(&data);
	data.Profile = &p;

	while (fgets(line, sizeof(line), stdin)) {
		n = strlen(line);
		if (n > 0 && line[n-1] == '\n') {
			line[--n] = 0;
		}

		n = sscanf(line, "%d;%39[^;];%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d%c",
			    &p.Number, p.Name, &p.DefaultName, &p.KeypadTone,
			    &p.Lights, &p.CallAlert, &p.Ringtone, &p.Volume,
			    &p.MessageTone, &p.Vibration, &p.WarningTone,
			    &p.CallerGroups, &p.AutomaticAnswer, &ch);
		if (n != 13) {
			fprintf(stderr, _("Input line format isn't valid\n"));
			return GN_ERR_UNKNOWN;
		}

		error = SM_Functions(GOP_SetProfile, &data, &State);
		if (error != GN_ERR_NONE) {
			fprintf(stderr, _("Cannot set profile: %s\n"), gn_error_print(error));
			return error;
		}
	}

	return error;
}

/* Get requested range of memory storage entries and output to stdout in
   easy-to-parse format */
static int getphonebook(int argc, char *argv[])
{
	GSM_PhonebookEntry entry;
	int count, start_entry, end_entry = 0;
	gn_error error;
	char *memory_type_string;
	bool all = false, raw = false, vcard = false;

	/* Handle command line args that set type, start and end locations. */
	memory_type_string = argv[0];
	entry.MemoryType = StrToMemoryType(memory_type_string);
	if (entry.MemoryType == GMT_XX) {
		fprintf(stderr, _("Unknown memory type %s (use ME, SM, ...)!\n"), argv[0]);
		return -1;
	}

	start_entry = end_entry = atoi(argv[1]);
	switch (argc) {
	case 2: /* Nothing to do */
		break;
	case 4:
		if (!strcmp(argv[3], "-r") || !strcmp(argv[3], "--raw")) raw = true;
		else 
			if (!strcmp(argv[3], "-v") || !strcmp(argv[3], "--vcard")) vcard = true;
			else usage(stderr);
	case 3:
		if (!strcmp(argv[2], "end")) all = true;
		else if (!strcmp(argv[2], "-r") || !strcmp(argv[2], "--raw")) raw = true;
		else end_entry = atoi(argv[2]);
		break;
	default:
		usage(stderr);
		break;
	}

	/* Now retrieve the requested entries. */
	count = start_entry;
	while (all || count <= end_entry) {
		entry.Location = count;

		data.PhonebookEntry = &entry;
		error = SM_Functions(GOP_ReadPhonebook, &data, &State);

		switch (error) {
			int i;
		case GN_ERR_NONE:
			if (raw) {
				fprintf(stdout, "%s;%s;%s;%d;%d", entry.Name, entry.Number, memory_type_string, entry.Location, entry.Group);
				for (i = 0; i < entry.SubEntriesCount; i++) {
					fprintf(stdout, ";%d;%d;%d;%s", entry.SubEntries[i].EntryType, entry.SubEntries[i].NumberType,
						entry.SubEntries[i].BlockNumber, entry.SubEntries[i].data.Number);
				}
				fprintf(stdout, "\n");
				if (entry.MemoryType == GMT_MC || entry.MemoryType == GMT_DC || entry.MemoryType == GMT_RC)
					fprintf(stdout, "%02u.%02u.%04u %02u:%02u:%02u\n", entry.Date.Day, entry.Date.Month, entry.Date.Year, entry.Date.Hour, entry.Date.Minute, entry.Date.Second);
			} else if (vcard) {
				char buf[10240];
				sprintf(buf, "X_GSM_STORE_AT:%s%d\n", memory_type_string, entry.Location);
				phonebook2vcard(stdout, &entry, buf);
			} else {
				fprintf(stdout, _("%d. Name: %s\nNumber: %s\nGroup id: %d\n"), entry.Location, entry.Name, entry.Number, entry.Location);
				for (i = 0; i < entry.SubEntriesCount; i++) {
					switch (entry.SubEntries[i].EntryType) {
					case 0x08:
						fprintf(stdout, _("Email address: "));
						break;
					case 0x09:
						fprintf(stdout, _("Address: "));
						break;
					case 0x0a:
						fprintf(stdout, _("Notes: "));
						break;
					case 0x0b:
						switch (entry.SubEntries[i].NumberType) {
						case 0x02:
							fprintf(stdout, _("Home number: "));
							break;
						case 0x03:
							fprintf(stdout, _("Cellular number: "));
							break;
						case 0x04:
							fprintf(stdout, _("Fax number: "));
							break;
						case 0x06:
							fprintf(stdout, _("Business number: "));
							break;
						case 0x0a:
							fprintf(stdout, _("Preferred number: "));
							break;
						default:
							fprintf(stdout, _("Unknown (%d): "), entry.SubEntries[i].NumberType);
							break;
						}
						break;
					case 0x2c:
						fprintf(stdout, _("WWW address: "));
						break;
					default:
						fprintf(stdout, _("Unknown (%d): "), entry.SubEntries[i].EntryType);
						break;
					}
					fprintf(stdout, "%s\n", entry.SubEntries[i].data.Number);
				}
				if (entry.MemoryType == GMT_MC || entry.MemoryType == GMT_DC || entry.MemoryType == GMT_RC)
					fprintf(stdout, _("Date: %02u.%02u.%04u %02u:%02u:%02u\n"), entry.Date.Day, entry.Date.Month, entry.Date.Year, entry.Date.Hour, entry.Date.Minute, entry.Date.Second);
			}
			break;
		case GN_ERR_EMPTYLOCATION:
			fprintf(stderr, _("Empty memory location. Skipping.\n"));
			break;
		case GN_ERR_INVALIDLOCATION:
			fprintf(stderr, _("Error reading from the location %d in memory %s\n"), count, memory_type_string);
			if (all) {
				/* Ensure that we quit the loop */
				all = false;
				count = 0;
				end_entry = -1;
			}
			break;
		default:
			fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
			return -1;
		}
		count++;
	}
	return 0;
}

static char * decodephonebook(GSM_PhonebookEntry *entry, char *OLine)
{
	char *Line = OLine;
	char *ptr;
	char *memory_type_string;
	char BackLine[MAX_INPUT_LINE_LEN];
	int subentry;

	strcpy(BackLine, Line);
	memset(entry, 0, sizeof(GSM_PhonebookEntry));

	ptr = strsep(&Line, ";");
	if (ptr) strncpy(entry->Name, ptr, sizeof(entry->Name) - 1);

	ptr = strsep(&Line, ";");
	if (ptr) strncpy(entry->Number, ptr, sizeof(entry->Number) - 1);

	ptr = strsep(&Line, ";");

	if (!ptr) {
		fprintf(stderr, _("Format problem on line [%s]\n"), BackLine);
		Line = OLine;
		return NULL;
	}

	if (!strncmp(ptr, "ME", 2)) {
		memory_type_string = "int";
		entry->MemoryType = GMT_ME;
	} else {
		if (!strncmp(ptr, "SM", 2)) {
			memory_type_string = "sim";
			entry->MemoryType = GMT_SM;
		} else {
			fprintf(stderr, _("Format problem on line [%s]\n"), BackLine);
			return NULL;
		}
	}

	ptr = strsep(&Line, ";");
	if (ptr) entry->Location = atoi(ptr);
	else entry->Location = 0;

	ptr = strsep(&Line, ";");
	if (ptr) entry->Group = atoi(ptr);
	else entry->Group = 0;

	if (!ptr) {
		fprintf(stderr, _("Format problem on line [%s]\n"), BackLine);
		return NULL;
	}

	for (subentry = 0; ; subentry++) {
		ptr = strsep(&Line, ";");

		if (ptr && *ptr != 0)
			entry->SubEntries[subentry].EntryType = atoi(ptr);
		else
			break;

		ptr = strsep(&Line, ";");
		if (ptr) entry->SubEntries[subentry].NumberType = atoi(ptr);

		/* Phone Numbers need to have a number type. */
		if (!ptr && entry->SubEntries[subentry].EntryType == GSM_Number) {
			fprintf(stderr, _("Missing phone number type on line %d"
					  " entry [%s]\n"), subentry, BackLine);
			subentry--;
			break;
		}

		ptr = strsep(&Line, ";");
		if (ptr) entry->SubEntries[subentry].BlockNumber = atoi(ptr);

		ptr = strsep(&Line, ";");

		/* 0x13 Date Type; it is only for Dailed Numbers, etc.
		   we don't store to this memories so it's an error to use it. */
		if (!ptr || entry->SubEntries[subentry].EntryType == GSM_Date) {
			fprintf(stderr, _("There is no phone number on line [%s] entry %d\n"),
				BackLine, subentry);
			subentry--;
			break;
		} else
			strncpy(entry->SubEntries[subentry].data.Number, ptr, sizeof(entry->SubEntries[subentry].data.Number) - 1);
	}

	entry->SubEntriesCount = subentry;

	/* This is to send other exports (like from 6110) to 7110 */
	if (!entry->SubEntriesCount) {
		entry->SubEntriesCount = 1;
		entry->SubEntries[subentry].EntryType   = GSM_Number;
		entry->SubEntries[subentry].NumberType  = GSM_General;
		entry->SubEntries[subentry].BlockNumber = 2;
		strcpy(entry->SubEntries[subentry].data.Number, entry->Number);
	}
	return memory_type_string;
}


/* Read data from stdin, parse and write to phone.  The parsing is relatively
   crude and doesn't allow for much variation from the stipulated format. */
/* FIXME: I guess there's *very* similar code in xgnokii */
static int writephonebook(int argc, char *args[])
{
	GSM_PhonebookEntry entry;
	gn_error error = GN_ERR_NOTSUPPORTED;
	char *memory_type_string;
	char *Line, OLine[MAX_INPUT_LINE_LEN];
	int vcard = 0;

	/* Check argument */
	if (argc && (strcmp("-i", args[0])) && (strcmp("-v", args[0])))
		usage(stderr);

	if (!strcmp("-v", args[0]))
		vcard = 1;

	Line = OLine;

	/* Go through data from stdin. */
	while (1) {
		if (!vcard) {
			if (!GetLine(stdin, Line, MAX_INPUT_LINE_LEN))
				break;
			if (decodephonebook(&entry, OLine))
				continue;
		} else {
			if (vcard2phonebook(stdin, &entry))
				break;
		}

		if (argc) {
			GSM_PhonebookEntry aux;

			aux.Location = entry.Location;
	      		data.PhonebookEntry = &aux;
			error = SM_Functions(GOP_ReadPhonebook, &data, &State);

			if (error == GN_ERR_NONE) {
				if (!aux.Empty) {
					int confirm = -1;
					char ans[8];

					fprintf(stdout, _("Location busy. "));
					while (confirm < 0) {
						fprintf(stdout, _("Overwrite? (yes/no) "));
						GetLine(stdin, ans, 7);
						if (!strcmp(ans, _("yes"))) confirm = 1;
						else if (!strcmp(ans, _("no"))) confirm = 0;
					}
					if (!confirm) return -1;
				}
			} else {
				fprintf(stderr, _("Error (%s)\n"), gn_error_print(error));
				return 0;
			}
		}

		/* Do write and report success/failure. */
		data.PhonebookEntry = &entry;
		error = SM_Functions(GOP_WritePhonebook, &data, &State);

		if (error == GN_ERR_NONE)
			fprintf (stderr, 
				 _("Write Succeeded: memory type: %s, loc: %d, name: %s, number: %s\n"), 
				 memory_type_string, entry.Location, entry.Name, entry.Number);
		else
			fprintf (stderr, _("Write FAILED (%s): memory type: %s, loc: %d, name: %s, number: %s\n"), 
				 gn_error_print(error), memory_type_string, entry.Location, entry.Name, entry.Number);
	}
	return 0;
}

/* Getting WAP bookmarks. */
static int getwapbookmark(char *Number)
{
	GSM_WAPBookmark	WAPBookmark;
	GSM_Data	data;
	gn_error	error;

	WAPBookmark.Location = atoi(Number);

	GSM_DataClear(&data);
	data.WAPBookmark = &WAPBookmark;

	error = SM_Functions(GOP_GetWAPBookmark, &data, &State);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stdout, _("WAP bookmark nr. %d:\n"), WAPBookmark.Location);
		fprintf(stdout, _("Name: %s\n"), WAPBookmark.Name);
		fprintf(stdout, _("URL: %s\n"), WAPBookmark.URL);

		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* Writing WAP bookmarks. */
static int writewapbookmark(int nargc, char *nargv[])
{
	GSM_WAPBookmark	WAPBookmark;
	GSM_Data	data;
	gn_error	error;


	GSM_DataClear(&data);
	data.WAPBookmark = &WAPBookmark;

	if (nargc != 2) usage(stderr);

	snprintf(&WAPBookmark.Name[0], MAX_WAP_NAME_LENGTH, nargv[0]);
	snprintf(&WAPBookmark.URL[0], MAX_WAP_URL_LENGTH, nargv[1]);

	error = SM_Functions(GOP_WriteWAPBookmark, &data, &State);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stderr, _("WAP bookmark nr. %d succesfully written!\n"), WAPBookmark.Location);
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* Deleting WAP bookmarks. */
static int deletewapbookmark(char *Number)
{
	GSM_WAPBookmark	WAPBookmark;
	GSM_Data	data;
	gn_error	error;

	WAPBookmark.Location = atoi(Number);

	GSM_DataClear(&data);
	data.WAPBookmark = &WAPBookmark;

	error = SM_Functions(GOP_DeleteWAPBookmark, &data, &State);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stderr, _("WAP bookmark nr. %d deleted!\n"), WAPBookmark.Location);
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* Getting WAP settings. */
static int getwapsetting(int argc, char *argv[])
{
	GSM_WAPSetting	WAPSetting;
	GSM_Data	data;
	gn_error	error;
	bool		raw = false;

	WAPSetting.Location = atoi(argv[0]);
	switch (argc) {
	case 1:
		break;
	case 2: 
		if (!strcmp(argv[1], "-r") || !strcmp(argv[1], "--raw")) 
			raw = true;
		else 
			usage(stderr);
		break;
	}

	GSM_DataClear(&data);
	data.WAPSetting = &WAPSetting;

	error = SM_Functions(GOP_GetWAPSetting, &data, &State);

	switch (error) {
	case GN_ERR_NONE:
		if (raw) {
			fprintf(stdout, ("%i;%s;%s;%i;%i;%i;%i;%i;%i;%i;%s;%s;%s;%s;%i;%i;%i;%s;%s;%s;%s;%s;%s;\n"), 
				WAPSetting.Location, WAPSetting.Name, WAPSetting.Home, WAPSetting.Session, 
				WAPSetting.Security, WAPSetting.Bearer, WAPSetting.GSMdataAuthentication, 
				WAPSetting.CallType, WAPSetting.CallSpeed, WAPSetting.GSMdataLogin, WAPSetting.GSMdataIP, 
				WAPSetting.Number, WAPSetting.GSMdataUsername, WAPSetting.GSMdataPassword, 
				WAPSetting.GPRSConnection, WAPSetting.GPRSAuthentication, WAPSetting.GPRSLogin, 
				WAPSetting.AccessPoint, WAPSetting.GPRSIP,  WAPSetting.GPRSUsername, WAPSetting.GPRSPassword,
				WAPSetting.SMSServiceNumber, WAPSetting.SMSServerNumber);
		} else {
			fprintf(stdout, _("WAP bookmark nr. %d:\n"), WAPSetting.Location);
			fprintf(stdout, _("Name: %s\n"), WAPSetting.Name);
			fprintf(stdout, _("Home: %s\n"), WAPSetting.Home);
			fprintf(stdout, _("Session mode: "));
			switch (WAPSetting.Session) {
			case GWP_TEMPORARY: 
				fprintf(stdout, _("temporary\n"));
				break;
			case GWP_PERMANENT: 
				fprintf(stdout, _("permanent\n"));
				break;
			default: 
				fprintf(stdout, _("unknown\n"));
				break;
			}
			fprintf(stdout, _("Connection security: "));
			if (WAPSetting.Security)
				fprintf(stdout, _("yes\n"));
			else
				fprintf(stdout, _("no\n"));
			fprintf(stdout, _("Data bearer: "));
			switch (WAPSetting.Bearer) {
			case GWP_GSMDATA: 
				fprintf(stdout, _("GSM data\n"));
				break;
			case GWP_GPRS: 
				fprintf(stdout, _("GPRS\n"));
				break;
			case GWP_SMS: 
				fprintf(stdout, _("SMS\n"));
				break;
			case GWP_USSD: 
				fprintf(stdout, _("USSD\n"));
				break;
			default: 
				fprintf(stdout, _("unknown\n"));
				break;
			}
			fprintf(stdout, _("GSM data\n"));
			fprintf(stdout, _("   Authentication type: "));
			switch (WAPSetting.GSMdataAuthentication) {
			case GWP_NORMAL: 
				fprintf(stdout, _("normal\n"));
				break;
			case GWP_SECURE: 
				fprintf(stdout, _("secure\n"));
				break;
			default: 
				fprintf(stdout, _("unknown\n"));
				break;
			}
			fprintf(stdout, _("   Data call type: "));
			switch (WAPSetting.CallType) {
			case GWP_ANALOGUE: 
				fprintf(stdout, _("analogue\n"));
				break;
			case GWP_ISDN: 
				fprintf(stdout, _("ISDN\n"));
				break;
			default: 
				fprintf(stdout, _("unknown\n"));
				break;
			}
			fprintf(stdout, _("   Data call speed: "));
			switch (WAPSetting.CallSpeed) {
			case GWP_AUTOMATIC: 
				fprintf(stdout, _("automatic\n"));
				break;
			case GWP_9600: 
				fprintf(stdout, _("9600\n"));
				break;
			case GWP_14400: 
				fprintf(stdout, _("14400\n"));
				break;	
			default: 
				fprintf(stdout, _("unknown\n"));
				break;
			}
			fprintf(stdout, _("   Login type: "));
			switch (WAPSetting.GSMdataLogin) {
			case GWP_MANUAL: 
				fprintf(stdout, _("manual\n"));
				break;
			case GWP_AUTOLOG: 
				fprintf(stdout, _("automatic\n"));
				break;
			default: 
				fprintf(stdout, _("unknown\n"));
				break;
			}
			fprintf(stdout, _("   IP: %s\n"), WAPSetting.GSMdataIP);
			fprintf(stdout, _("   Number: %s\n"), WAPSetting.Number);
			fprintf(stdout, _("   Username: %s\n"), WAPSetting.GSMdataUsername);
			fprintf(stdout, _("   Password: %s\n"), WAPSetting.GSMdataPassword);
			fprintf(stdout, _("GPRS\n"));
			fprintf(stdout, _("   connection: "));
			switch (WAPSetting.GPRSConnection) {
			case GWP_NEEDED: 
				fprintf(stdout, _("when needed\n"));
				break;
			case GWP_ALWAYS: 
				fprintf(stdout, _("always\n"));
				break;
			default: 
				fprintf(stdout, _("unknown\n"));
				break;
			}
			fprintf(stdout, _("   Authentication type: "));
			switch (WAPSetting.GPRSAuthentication) {
			case GWP_NORMAL: 
				fprintf(stdout, _("normal\n"));
				break;
			case GWP_SECURE: 
				fprintf(stdout, _("secure\n"));
				break;
			default: 
				fprintf(stdout, _("unknown\n"));
				break;
			}
			fprintf(stdout, _("   Login type: "));
			switch (WAPSetting.GPRSLogin) {
			case GWP_MANUAL: 
				fprintf(stdout, _("manual\n"));
				break;
			case GWP_AUTOLOG: 
				fprintf(stdout, _("automatic\n"));
				break;
			default: 
				fprintf(stdout, _("unknown\n"));
				break;
			}
			fprintf(stdout, _("   Access point: %s\n"), WAPSetting.AccessPoint);
			fprintf(stdout, _("   IP: %s\n"), WAPSetting.GPRSIP);
			fprintf(stdout, _("   Username: %s\n"), WAPSetting.GPRSUsername);
			fprintf(stdout, _("   Password: %s\n"), WAPSetting.GPRSPassword);
			fprintf(stdout, _("SMS\n"));
			fprintf(stdout, _("   Service number: %s\n"), WAPSetting.SMSServiceNumber);
			fprintf(stdout, _("   Server number: %s\n"), WAPSetting.SMSServerNumber);
		}
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* Writes WAP settings to phone */
static int writewapsetting()
{
	int n;
	GSM_WAPSetting WAPSetting;
	gn_error error = GN_ERR_NONE;
	GSM_Data data;
	char line[1000];

	GSM_DataClear(&data);
	data.WAPSetting = &WAPSetting;

	while (fgets(line, sizeof(line), stdin)) {
		n = strlen(line);
		if (n > 0 && line[n-1] == '\n') {
			line[--n] = 0;
		}
		
		n = sscanf(line, "%d;%50[^;];%256[^;];%d;%d;%d;%d;%d;%d;%d;%50[^;];%50[^;];%32[^;];%20[^;];%d;%d;%d;%100[^;];%20[^;];%32[^;];%20[^;];%20[^;];%20[^;];", 
		/*
		n = sscanf(line, "%d;%s;%s;%d;%d;%d;%d;%d;%d;%d;%s;%s;%s;%s;%d;%d;%d;%s;%s;%s;%s;%s;%s;", 
		*/
			   &WAPSetting.Location, WAPSetting.Name, WAPSetting.Home, (int*)&WAPSetting.Session, 
			   (int*)&WAPSetting.Security, (int*)&WAPSetting.Bearer, (int*)&WAPSetting.GSMdataAuthentication, 
			   (int*)&WAPSetting.CallType, (int*)&WAPSetting.CallSpeed, (int*)&WAPSetting.GSMdataLogin, WAPSetting.GSMdataIP, 
			   WAPSetting.Number, WAPSetting.GSMdataUsername, WAPSetting.GSMdataPassword, 
			   (int*)&WAPSetting.GPRSConnection, (int*)&WAPSetting.GPRSAuthentication, (int*)&WAPSetting.GPRSLogin, 
			   WAPSetting.AccessPoint, WAPSetting.GPRSIP,  WAPSetting.GPRSUsername, WAPSetting.GPRSPassword,
			   WAPSetting.SMSServiceNumber, WAPSetting.SMSServerNumber);

		if (n != 23) {
			fprintf(stderr, _("Input line format isn't valid\n"));
			dprintf("n: %i\n", n);
			return GN_ERR_UNKNOWN;
		}

		error = SM_Functions(GOP_WriteWAPSetting, &data, &State);
		if (error != GN_ERR_NONE) 
			fprintf(stderr, _("Cannot write WAP setting: %s\n"), gn_error_print(error));
		return error;
	}
	return error;
}

/* Deleting WAP bookmarks. */
static int activatewapsetting(char *Number)
{
	GSM_WAPSetting	WAPSetting;
	GSM_Data	data;
	gn_error	error;

	WAPSetting.Location = atoi(Number);

	GSM_DataClear(&data);
	data.WAPSetting = &WAPSetting;

	error = SM_Functions(GOP_ActivateWAPSetting, &data, &State);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stderr, _("WAP setting nr. %d activated!\n"), WAPSetting.Location);
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* Getting speed dials. */
static int getspeeddial(char *Number)
{
	GSM_SpeedDial	SpeedDial;
	GSM_Data	data;
	gn_error	error;

	SpeedDial.Number = atoi(Number);

	GSM_DataClear(&data);
	data.SpeedDial = &SpeedDial;

	error = SM_Functions(GOP_GetSpeedDial, &data, &State);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stderr, _("SpeedDial nr. %d: %d:%d\n"), SpeedDial.Number, SpeedDial.MemoryType, SpeedDial.Location);
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* Setting speed dials. */
static int setspeeddial(char *argv[])
{
	GSM_SpeedDial entry;
	GSM_Data data;
	gn_error error;
	char *memory_type_string;

	GSM_DataClear(&data);
	data.SpeedDial = &entry;

	/* Handle command line args that set type, start and end locations. */

	if (strcmp(argv[1], "ME") == 0) {
		entry.MemoryType = GMT_ME;
		memory_type_string = "ME";
	} else if (strcmp(argv[1], "SM") == 0) {
		entry.MemoryType = GMT_SM;
		memory_type_string = "SM";
	} else {
		fprintf(stderr, _("Unknown memory type %s!\n"), argv[1]);
		return -1;
	}

	entry.Number = atoi(argv[0]);
	entry.Location = atoi(argv[2]);

	if ((error = SM_Functions(GOP_SetSpeedDial, &data, &State)) == GN_ERR_NONE )
		fprintf(stderr, _("Succesfully written!\n"));

	return error;
}

/* Getting the status of the display. */
static int getdisplaystatus(void)
{
	gn_error error = GN_ERR_INTERNALERROR;
	int Status;
	GSM_Data data;

	GSM_DataClear(&data);
	data.DisplayStatus = &Status;

	error = SM_Functions(GOP_GetDisplayStatus, &data, &State);
	if (error == GN_ERR_NONE) PrintDisplayStatus(Status);

	return error;
}

static int netmonitor(char *Mode)
{
	unsigned char mode = atoi(Mode);
	GSM_NetMonitor nm;

	if (!strcmp(Mode, "reset"))
		mode = 0xf0;
	else if (!strcmp(Mode, "off"))
		mode = 0xf1;
	else if (!strcmp(Mode, "field"))
		mode = 0xf2;
	else if (!strcmp(Mode, "devel"))
		mode = 0xf3;
	else if (!strcmp(Mode, "next"))
		mode = 0x00;

	nm.Field = mode;
	memset(&nm.Screen, 0, 50);
	data.NetMonitor = &nm;

	SM_Functions(GOP_NetMonitor, &data, &State);

	if (nm.Screen) fprintf(stdout, "%s\n", nm.Screen);

	return 0;
}

static int identify(void)
{
	/* Hopefully 64 is enough */
	char imei[64], model[64], rev[64], manufacturer[64];

	manufacturer[0] = model[0] = rev[0] = imei[0] = 0;

	data.Manufacturer = manufacturer;
	data.Model = model;
	data.Revision = rev;
	data.Imei = imei;

	/* Retrying is bad idea: what if function is simply not implemented?
	   Anyway let's wait 2 seconds for the right packet from the phone. */
	sleep(2);

	strcpy(imei, _("(unknown)"));
	strcpy(manufacturer, _("(unknown)"));
	strcpy(model, _("(unknown)"));
	strcpy(rev, _("(unknown)"));

	SM_Functions(GOP_Identify, &data, &State);

	fprintf(stdout, _("IMEI         : %s\n"), imei);
	fprintf(stdout, _("Manufacturer : %s\n"), manufacturer);
	fprintf(stdout, _("Model        : %s\n"), model);
	fprintf(stdout, _("Revision     : %s\n"), rev);

	return 0;
}

static int senddtmf(char *String)
{
	GSM_DataClear(&data);
	data.DTMFString = String;

	return SM_Functions(GOP_SendDTMF, &data, &State);
}

/* Resets the phone */
static int reset(char *type)
{
	GSM_DataClear(&data);
	data.ResetType = 0x03;

	if (type) {
		if (!strcmp(type, "soft"))
			data.ResetType = 0x03;
		else
			if (!strcmp(type, "hard")) {
				data.ResetType = 0x04;
			} else {
				fprintf(stderr, _("What kind of reset do you want??\n"));
				return -1;
			}
	}

	return SM_Functions(GOP_Reset, &data, &State);
}

/* pmon allows fbus code to run in a passive state - it doesn't worry about
   whether comms are established with the phone.  A debugging/development
   tool. */
static int pmon(void)
{
	gn_error error;
	GSM_ConnectionType connection = GCT_Serial;

	/* Initialise the code for the GSM interface. */
	error = gn_gsm_initialise(model, Port, Initlength, connection, NULL, &State);

	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("GSM/FBUS init failed! (Unknown model?). Quitting.\n"));
		return -1;
	}

	while (1) {
		usleep(50000);
	}

	return 0;
}

static int sendringtone(int argc, char *argv[])
{
	GSM_API_SMS sms;
	gn_error error = GN_ERR_NOTSUPPORTED;

	gn_sms_default_submit(&sms);
	sms.UserData[0].Type = SMS_RingtoneData;
	sms.UserData[1].Type = SMS_NoData;

	if (GSM_ReadRingtoneFile(argv[0], &sms.UserData[0].u.Ringtone)) {
		fprintf(stderr, _("Failed to load ringtone.\n"));
		return -1;
	}

	/* The second argument is the destination, ie the phone number of recipient. */
	memset(&sms.Remote.Number, 0, sizeof(sms.Remote.Number));
	strncpy(sms.Remote.Number, argv[1], sizeof(sms.Remote.Number) - 1);

	/* Send the message. */
	data.SMS = &sms;
	error = gn_sms_send(&data, &State);

	if (error == GN_ERR_NONE)
		fprintf(stderr, _("Send succeeded!\n"));
	else
		fprintf(stderr, _("SMS Send failed (%s)\n"), gn_error_print(error));

	return 0;
}

static int getringtone(int argc, char *argv[])
{
	GSM_Ringtone ringtone;
	GSM_RawData rawdata;
	gn_error error;
	unsigned char buff[512];
	int i;

	bool raw = false;
	struct option options[] = {
		{ "raw",    no_argument, NULL, 'r'},
		{ NULL,     0,           NULL, 0}
	};

	optarg = NULL;
	optind = 0;
	argv++;
	argc--;

	while ((i = getopt_long(argc, argv, "r", options, NULL)) != -1) {
		switch (i) {
		case 'r':
			raw = true;
			break;
		default:
			usage(stderr); /* FIXME */
			return -1;
		}
	}

	memset(&ringtone, 0, sizeof(ringtone));
	rawdata.Data = buff;
	rawdata.Length = sizeof(buff);
	GSM_DataClear(&data);
	data.Ringtone = &ringtone;
	data.RawData = &rawdata;

	if (argc <= optind) {
		usage(stderr);
		return -1;
	}

	ringtone.Location = (argc > optind + 1) ? atoi(argv[optind + 1]) : 0;

	if (raw)
		error = SM_Functions(GOP_GetRawRingtone, &data, &State);
	else
		error = SM_Functions(GOP_GetRingtone, &data, &State);
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Getting ringtone %d failed: %s\n"), ringtone.Location, gn_error_print(error));
		return error;
	}
	fprintf(stderr, _("Getting ringtone %d (\"%s\") succeeded!\n"), ringtone.Location, ringtone.name);

	if (raw) {
		FILE *f;

		if ((f = fopen(argv[optind], "wb")) == NULL) {
			fprintf(stderr, _("Failed to save ringtone.\n"));
			return -1;
		}
		fwrite(rawdata.Data, 1, rawdata.Length, f);
		fclose(f);
	} else {
		if (GSM_SaveRingtoneFile(argv[optind], &ringtone) != GN_ERR_NONE) {
			fprintf(stderr, _("Failed to save ringtone: %s\n"), gn_error_print(error));
			return -1;
		}
	}

	return 0;
}

static int setringtone(int argc, char *argv[])
{
	GSM_Ringtone ringtone;
	GSM_RawData rawdata;
	gn_error error;
	unsigned char buff[512];
	int i;

	bool raw = false;
	char name[16] = "";
	struct option options[] = {
		{ "raw",    no_argument,       NULL, 'r'},
		{ "name",   required_argument, NULL, 'n'},
		{ NULL,     0,                 NULL, 0}
	};

	optarg = NULL;
	optind = 0;
	argv++;
	argc--;

	while ((i = getopt_long(argc, argv, "r", options, NULL)) != -1) {
		switch (i) {
		case 'r':
			raw = true;
			break;
		case 'n':
			snprintf(name, sizeof(name), "%s", optarg);
			break;
		default:
			usage(stderr); /* FIXME */
			return -1;
		}
	}

	memset(&ringtone, 0, sizeof(ringtone));
	rawdata.Data = buff;
	rawdata.Length = sizeof(buff);
	GSM_DataClear(&data);
	data.Ringtone = &ringtone;
	data.RawData = &rawdata;

	if (argc <= optind) {
		usage(stderr);
		return -1;
	}

	ringtone.Location = (argc > optind + 1) ? atoi(argv[optind + 1]) : 0;

	if (raw) {
		FILE *f;

		if ((f = fopen(argv[optind], "rb")) == NULL) {
			fprintf(stderr, _("Failed to load ringtone.\n"));
			return -1;
		}
		rawdata.Length = fread(rawdata.Data, 1, rawdata.Length, f);
		fclose(f);
		if (*name)
			snprintf(ringtone.name, sizeof(ringtone.name), "%s", name);
		else
			snprintf(ringtone.name, sizeof(ringtone.name), "GNOKII");
		error = SM_Functions(GOP_SetRawRingtone, &data, &State);
	} else {
		if (GSM_ReadRingtoneFile(argv[optind], &ringtone)) {
			fprintf(stderr, _("Failed to load ringtone.\n"));
			return -1;
		}
		if (*name) snprintf(ringtone.name, sizeof(ringtone.name), "%s", name);
		error = SM_Functions(GOP_SetRingtone, &data, &State);
	}

	if (error == GN_ERR_NONE)
		fprintf(stderr, _("Send succeeded!\n"));
	else
		fprintf(stderr, _("Send failed: %s\n"), gn_error_print(error));

	return 0;
}

static void presskey(void)
{
	gn_error error;
	error = SM_Functions(GOP_PressPhoneKey, &data, &State);
	if (error == GN_ERR_NONE)
		error = SM_Functions(GOP_ReleasePhoneKey, &data, &State);
	fprintf(stderr, _("Failed to press key: %s\n"), gn_error_print(error));
}

static int presskeysequence(void)
{
	unsigned char *syms = "0123456789#*PGR+-UDMN";
	GSM_KeyCode keys[] = {GSM_KEY_0, GSM_KEY_1, GSM_KEY_2, GSM_KEY_3,
			      GSM_KEY_4, GSM_KEY_5, GSM_KEY_6, GSM_KEY_7,
			      GSM_KEY_8, GSM_KEY_9, GSM_KEY_HASH,
			      GSM_KEY_ASTERISK, GSM_KEY_POWER, GSM_KEY_GREEN,
			      GSM_KEY_RED, GSM_KEY_INCREASEVOLUME,
			      GSM_KEY_DECREASEVOLUME, GSM_KEY_UP, GSM_KEY_DOWN,
			      GSM_KEY_MENU, GSM_KEY_NAMES};
	unsigned char ch, *pos;

	GSM_DataClear(&data);
	console_raw();

	while (read(0, &ch, 1) > 0) {
		if ((pos = strchr(syms, toupper(ch))) != NULL)
			data.KeyCode = keys[pos - syms];
		else
			continue;
		presskey();
	}

	return 0;
}

static int enterchar(void)
{
	unsigned char ch;
	gn_error error;

	GSM_DataClear(&data);
	console_raw();

	while (read(0, &ch, 1) > 0) {
		switch (ch) {
		case '\r':
			break;
		case '\n':
			data.KeyCode = GSM_KEY_MENU;
			presskey();
			break;
		case '\e':
			data.KeyCode = GSM_KEY_NAMES;
			presskey();
			break;
		default:
			data.Character = ch;
			error = SM_Functions(GOP_EnterChar, &data, &State);
			if (error != GN_ERR_NONE)
				fprintf(stderr, _("Error entering char: %s\n"), gn_error_print(error));
			break;
		}
	}

	return 0;
}

/* Options for --divert:
   --op, -o [register|enable|query|disable|erasure]  REQ
   --type, -t [all|busy|noans|outofreach|notavail]   REQ
   --call, -c [all|voice|fax|data]                   REQ
   --timeout, m time_in_seconds                      OPT
   --number, -n number                               OPT
 */
static int divert(int argc, char **argv)
{
	int opt;
	GSM_CallDivert cd;
	GSM_Data data;
	gn_error error;
	struct option options[] = {
		{ "op",      required_argument, NULL, 'o'},
		{ "type",    required_argument, NULL, 't'},
		{ "call",    required_argument, NULL, 'c'},
		{ "number",  required_argument, NULL, 'n'},
		{ "timeout", required_argument, NULL, 'm'},
		{ NULL,      0,                 NULL, 0}
	};

	optarg = NULL;
	optind = 0;

	/* Skip --divert */
	argc--; argv++;

	memset(&cd, 0, sizeof(GSM_CallDivert));

	while ((opt = getopt_long(argc, argv, "o:t:n:c:m:", options, NULL)) != -1) {
		switch (opt) {
		case 'o':
			if (!strcmp("register", optarg)) {
				cd.Operation = GSM_CDV_Register;
			} else if (!strcmp("enable", optarg)) {
				cd.Operation = GSM_CDV_Enable;
			} else if (!strcmp("disable", optarg)) {
				cd.Operation = GSM_CDV_Disable;
			} else if (!strcmp("erasure", optarg)) {
				cd.Operation = GSM_CDV_Erasure;
			} else if (!strcmp("query", optarg)) {
				cd.Operation = GSM_CDV_Query;
			} else {
				usage(stderr);
				return -1;
			}
			break;
		case 't':
			if (!strcmp("all", optarg)) {
				cd.DType = GSM_CDV_AllTypes;
			} else if (!strcmp("busy", optarg)) {
				cd.DType = GSM_CDV_Busy;
			} else if (!strcmp("noans", optarg)) {
				cd.DType = GSM_CDV_NoAnswer;
			} else if (!strcmp("outofreach", optarg)) {
				cd.DType = GSM_CDV_OutOfReach;
			} else if (!strcmp("notavail", optarg)) {
				cd.DType = GSM_CDV_NotAvailable;
			} else {
				usage(stderr);
				return -1;
			}
			break;
		case 'c':
			if (!strcmp("all", optarg)) {
				cd.CType = GSM_CDV_AllCalls;
			} else if (!strcmp("voice", optarg)) {
				cd.CType = GSM_CDV_VoiceCalls;
			} else if (!strcmp("fax", optarg)) {
				cd.CType = GSM_CDV_FaxCalls;
			} else if (!strcmp("data", optarg)) {
				cd.CType = GSM_CDV_DataCalls;
			} else {
				usage(stderr);
				return -1;
			}
			break;
		case 'm':
			cd.Timeout = atoi(optarg);
			break;
		case 'n':
			strncpy(cd.Number.Number, optarg, sizeof(cd.Number.Number) - 1);
			if (cd.Number.Number[0] == '+') cd.Number.Type = SMS_International;
			else cd.Number.Type = SMS_Unknown;
			break;
		default:
			usage(stderr);
			return -1;
		}
	}
	data.CallDivert = &cd;
	error = SM_Functions(GOP_CallDivert, &data, &State);

	if (error == GN_ERR_NONE) {
		fprintf(stdout, _("Divert type: "));
		switch (cd.DType) {
		case GSM_CDV_AllTypes: fprintf(stdout, _("all\n")); break;
		case GSM_CDV_Busy: fprintf(stdout, _("busy\n")); break;
		case GSM_CDV_NoAnswer: fprintf(stdout, _("noans\n")); break;
		case GSM_CDV_OutOfReach: fprintf(stdout, _("outofreach\n")); break;
		case GSM_CDV_NotAvailable: fprintf(stdout, _("notavail\n")); break;
		default: fprintf(stdout, _("unknown(0x%02x)\n"), cd.DType);
		}
		fprintf(stdout, _("Call type: "));
		switch (cd.CType) {
		case GSM_CDV_AllCalls: fprintf(stdout, _("all\n")); break;
		case GSM_CDV_VoiceCalls: fprintf(stdout, _("voice\n")); break;
		case GSM_CDV_FaxCalls: fprintf(stdout, _("fax\n")); break;
		case GSM_CDV_DataCalls: fprintf(stdout, _("data\n")); break;
		default: fprintf(stdout, _("unknown(0x%02x)\n"), cd.CType);
		}
		if (cd.Number.Number[0]) {
			fprintf(stdout, _("Number: %s\n"), cd.Number.Number);
			fprintf(stdout, _("Timeout: %d\n"), cd.Timeout);
		} else
			fprintf(stdout, _("Divert isn't active.\n"));
	} else {
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
	}
	return 0;
}

static gn_error smsslave(GSM_API_SMS *message)
{
	FILE *output;
	char *s = message->UserData[0].u.Text;
	char buf[10240];
	int i = message->Number;
	int i1, i2, msgno, msgpart;
	static int unknown = 0;
	char c, number[MAX_BCD_STRING_LENGTH];
	char *p = message->Remote.Number;

	if (p[0] == '+') {
		p++;
	}
	strcpy(number, p);
	fprintf(stderr, _("SMS received from number: %s\n"), number);

	/* From Pavel Machek's email to the gnokii-ml (msgid: <20020414212455.GB9528@elf.ucw.cz>):
	 *  It uses fixed format of provider in CR. If your message matches
	 *  WWW1/1:1234-5678 where 1234 is msgno and 5678 is msgpart, it should
	 *  work.
	 */
	while (*s == 'W')
		s++;
	fprintf(stderr, _("Got message %d: %s\n"), i, s);
	if ((sscanf(s, "%d/%d:%d-%c-", &i1, &i2, &msgno, &c) == 4) && (c == 'X'))
		sprintf(buf, "/tmp/sms/mail_%d_", msgno);
	else if (sscanf(s, "%d/%d:%d-%d-", &i1, &i2, &msgno, &msgpart) == 4)
		sprintf(buf, "/tmp/sms/mail_%d_%03d", msgno, msgpart);
	else	sprintf(buf, "/tmp/sms/sms_%s_%d_%d", number, getpid(), unknown++);
	if ((output = fopen(buf, "r")) != NULL) {
		fprintf(stderr, _("### Exists?!\n"));
		return GN_ERR_FAILED;
	}
	output = fopen(buf, "w+");

	/* Skip formatting chars */
	if (!strstr(buf, "mail"))
		fprintf(output, "%s", message->UserData[0].u.Text);
	else {
		s = message->UserData[0].u.Text;
		while (!(*s == '-'))
			s++;
		s++;
		while (!(*s == '-'))
			s++;
		s++;
		fprintf(output, "%s", s);
	}
	fclose(output);
	return GN_ERR_NONE;
}

static int smsreader(void)
{
	GSM_Data data;
	gn_error error;

	data.OnSMS = smsslave;
	error = SM_Functions(GOP_OnSMS, &data, &State);
	if (error == GN_ERR_NONE) {
		/* We do not want to see texts forever - press Ctrl+C to stop. */
		signal(SIGINT, interrupted);
		fprintf(stderr, _("Entered sms reader mode...\n"));

		while (!bshutdown) {
			SM_Loop(&State, 1);
			/* Some phones may not be able to notify us, thus we give
			   lowlevel chance to poll them */
			error = SM_Functions(GOP_PollSMS, &data, &State);
		}
		fprintf(stderr, _("Shutting down\n"));

		fprintf(stderr, _("Exiting sms reader mode...\n"));
		data.OnSMS = NULL;

		error = SM_Functions(GOP_OnSMS, &data, &State);
		if (error != GN_ERR_NONE)
			fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
	} else
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));

	return 0;
}

static int getsecuritycode()
{
	GSM_Data data;
	gn_error error;
	GSM_SecurityCode sc;
	
	memset(&sc.Code, 0, 10 * sizeof(char));
	data.SecurityCode = &sc;
	fprintf(stderr, _("Getting security code... \n"));
	error = SM_Functions(GOP_GetSecurityCode, &data, &State);
	fprintf(stdout, _("Security code is: %s\n"), &sc.Code[0]);
	return error;
}

static void list_gsm_networks(void)
{
	extern GSM_Network GSM_Networks[];
	int i;

	printf("Network  Name\n");
	printf("-----------------------------------------\n");
	for (i = 0; strcmp(GSM_Networks[i].Name, "unknown"); i++)
		printf("%-7s  %s\n", GSM_Networks[i].Code, GSM_Networks[i].Name);
}

/* This is a "convenience" function to allow quick test of new API stuff which
   doesn't warrant a "proper" command line function. */
#ifndef WIN32
static int foogle(char *argv[])
{
	/* Fill in what you would like to test here... */
	return 0;
}
#endif

static void  gnokii_error_logger(const char *fmt, va_list ap)
{
	if (logfile) {
		vfprintf(logfile, fmt, ap);
		fflush(logfile);
	}
}

static int install_log_handler(void)
{
	char logname[256];
	char *home;
#ifdef WIN32
	char *file = "gnokii-errors";
#else
	char *file = ".gnokii-errors";
#endif

	if ((home = getenv("HOME")) == NULL) {
#ifdef WIN32
		home = ".";
#else
		fprintf(stderr, _("HOME variable missing\n"));
		return -1;
#endif
	}

	snprintf(logname, sizeof(logname), "%s/%s", home, file);

	if ((logfile = fopen(logname, "a")) == NULL) {
		perror("fopen");
		return -1;
	}

	GSM_ELogHandler = gnokii_error_logger;

	return 0;
}

/* Main function - handles command line arguments, passes them to separate
   functions accordingly. */
int main(int argc, char *argv[])
{
	int c, i, rc = -1;
	int nargc = argc - 2;
	char **nargv;

	/* Every option should be in this array. */
	static struct option long_options[] = {
		/* FIXME: these comments are nice, but they would be more usefull as docs for the user */
		/* Display usage. */
		{ "help",               no_argument,       NULL, OPT_HELP },

		/* Display version and build information. */
		{ "version",            no_argument,       NULL, OPT_VERSION },

		/* Monitor mode */
		{ "monitor",            no_argument,       NULL, OPT_MONITOR },

#ifdef SECURITY

		/* Enter Security Code mode */
		{ "entersecuritycode",  required_argument, NULL, OPT_ENTERSECURITYCODE },

		/* Get Security Code status */
		{ "getsecuritycodestatus",  no_argument,   NULL, OPT_GETSECURITYCODESTATUS },

		/* Change Security Code */
		{ "changesecuritycode", required_argument, NULL, OPT_CHANGESECURITYCODE },

#endif

		/* Set date and time */
		{ "setdatetime",        optional_argument, NULL, OPT_SETDATETIME },

		/* Get date and time mode */
		{ "getdatetime",        no_argument,       NULL, OPT_GETDATETIME },

		/* Set alarm */
		{ "setalarm",           optional_argument, NULL, OPT_SETALARM },

		/* Get alarm */
		{ "getalarm",           no_argument,       NULL, OPT_GETALARM },

		/* Voice call mode */
		{ "dialvoice",          required_argument, NULL, OPT_DIALVOICE },

		/* Answer the incoming call */
		{ "answercall",         required_argument, NULL, OPT_ANSWERCALL },

		/* Hangup call */
		{ "hangup",             required_argument, NULL, OPT_HANGUP },

		/* Get ToDo note mode */
		{ "gettodo",		required_argument, NULL, OPT_GETTODO },

		/* Write ToDo note mode */
		{ "writetodo",          required_argument, NULL, OPT_WRITETODO },

		/* Delete all ToDo notes mode */
		{ "deletealltodos",     no_argument,       NULL, OPT_DELETEALLTODOS },

		/* Get calendar note mode */
		{ "getcalendarnote",    required_argument, NULL, OPT_GETCALENDARNOTE },

		/* Write calendar note mode */
		{ "writecalendarnote",  required_argument, NULL, OPT_WRITECALENDARNOTE },

		/* Delete calendar note mode */
		{ "deletecalendarnote", required_argument, NULL, OPT_DELCALENDARNOTE },

		/* Get display status mode */
		{ "getdisplaystatus",   no_argument,       NULL, OPT_GETDISPLAYSTATUS },

		/* Get memory mode */
		{ "getphonebook",       required_argument, NULL, OPT_GETPHONEBOOK },

		/* Write phonebook (memory) mode */
		{ "writephonebook",     optional_argument, NULL, OPT_WRITEPHONEBOOK },

		/* Get speed dial mode */
		{ "getspeeddial",       required_argument, NULL, OPT_GETSPEEDDIAL },

		/* Set speed dial mode */
		{ "setspeeddial",       required_argument, NULL, OPT_SETSPEEDDIAL },

		/* Create SMS folder mode */
		{ "createsmsfolder",    required_argument, NULL, OPT_CREATESMSFOLDER },

		/* Delete SMS folder mode */
		{ "deletesmsfolder",    required_argument, NULL, OPT_DELETESMSFOLDER },

		/* Get SMS message mode */
		{ "getsms",             required_argument, NULL, OPT_GETSMS },

		/* Delete SMS message mode */
		{ "deletesms",          required_argument, NULL, OPT_DELETESMS },

		/* Send SMS message mode */
		{ "sendsms",            required_argument, NULL, OPT_SENDSMS },

		/* Ssve SMS message mode */
		{ "savesms",            optional_argument, NULL, OPT_SAVESMS },

		/* Send logo as SMS message mode */
		{ "sendlogo",           required_argument, NULL, OPT_SENDLOGO },

		/* Send ringtone as SMS message */
		{ "sendringtone",       required_argument, NULL, OPT_SENDRINGTONE },

		/* Get ringtone */
		{ "getringtone",        required_argument, NULL, OPT_GETRINGTONE },

		/* Set ringtone */
		{ "setringtone",        required_argument, NULL, OPT_SETRINGTONE },

		/* Get SMS center number mode */
		{ "getsmsc",            optional_argument, NULL, OPT_GETSMSC },

		/* Set SMS center number mode */
		{ "setsmsc",            no_argument,       NULL, OPT_SETSMSC },

		/* For development purposes: run in passive monitoring mode */
		{ "pmon",               no_argument,       NULL, OPT_PMON },

		/* NetMonitor mode */
		{ "netmonitor",         required_argument, NULL, OPT_NETMONITOR },

		/* Identify */
		{ "identify",           no_argument,       NULL, OPT_IDENTIFY },

		/* Send DTMF sequence */
		{ "senddtmf",           required_argument, NULL, OPT_SENDDTMF },

		/* Resets the phone */
		{ "reset",              optional_argument, NULL, OPT_RESET },

		/* Set logo */
		{ "setlogo",            optional_argument, NULL, OPT_SETLOGO },

		/* Get logo */
		{ "getlogo",            required_argument, NULL, OPT_GETLOGO },

		/* View logo */
		{ "viewlogo",           required_argument, NULL, OPT_VIEWLOGO },

		/* Show profile */
		{ "getprofile",         optional_argument, NULL, OPT_GETPROFILE },

		/* Set profile */
		{ "setprofile",         no_argument,       NULL, OPT_SETPROFILE },

		/* Show texts from phone's display */
		{ "displayoutput",      no_argument,       NULL, OPT_DISPLAYOUTPUT },

		/* Simulate pressing the keys */
		{ "keysequence",        no_argument,       NULL, OPT_KEYPRESS },

		/* Simulate pressing the keys */
		{ "enterchar",          no_argument,       NULL, OPT_ENTERCHAR },

		/* Divert calls */
		{ "divert",		required_argument, NULL, OPT_DIVERT },

		/* SMS reader */
		{ "smsreader",          no_argument,       NULL, OPT_SMSREADER },

		/* For development purposes: insert you function calls here */
		{ "foogle",             no_argument,       NULL, OPT_FOOGLE },

		/* Get Security Code */
		{ "getsecuritycode",    no_argument,   	   NULL, OPT_GETSECURITYCODE },

		/* Get WAP bookmark */
		{ "getwapbookmark",     required_argument, NULL, OPT_GETWAPBOOKMARK },

		/* Write WAP bookmark */
		{ "writewapbookmark",   required_argument, NULL, OPT_WRITEWAPBOOKMARK },

		/* Delete WAP bookmark */
		{ "deletewapbookmark",  required_argument, NULL, OPT_DELETEWAPBOOKMARK },

		/* Get WAP setting */
		{ "getwapsetting",      required_argument, NULL, OPT_GETWAPSETTING },

		/* Write WAP setting */
		{ "writewapsetting",    no_argument, 	   NULL, OPT_WRITEWAPSETTING },

		/* Activate WAP setting */
		{ "activatewapsetting", required_argument, NULL, OPT_ACTIVATEWAPSETTING },

		/* List GSM networks */
		{ "listnetworks",       no_argument,       NULL, OPT_LISTNETWORKS },

		{ 0, 0, 0, 0},
	};

	/* Every command which requires arguments should have an appropriate entry
	   in this array. */
	static struct gnokii_arg_len gals[] = {
#ifdef SECURITY
		{ OPT_ENTERSECURITYCODE, 1, 1, 0 },
		{ OPT_CHANGESECURITYCODE,1, 1, 0 },
#endif
		{ OPT_SETDATETIME,       0, 5, 0 },
		{ OPT_SETALARM,          0, 2, 0 },
		{ OPT_DIALVOICE,         1, 1, 0 },
		{ OPT_ANSWERCALL,        1, 1, 0 },
		{ OPT_HANGUP,            1, 1, 0 },
		{ OPT_GETTODO,           1, 3, 0 },
		{ OPT_WRITETODO,         2, 2, 0 },
		{ OPT_GETCALENDARNOTE,   1, 3, 0 },
		{ OPT_WRITECALENDARNOTE, 2, 2, 0 },
		{ OPT_DELCALENDARNOTE,   1, 2, 0 },
		{ OPT_GETPHONEBOOK,      2, 4, 0 },
		{ OPT_GETSPEEDDIAL,      1, 1, 0 },
		{ OPT_SETSPEEDDIAL,      3, 3, 0 },
		{ OPT_CREATESMSFOLDER,      1, 1, 0 },
		{ OPT_DELETESMSFOLDER,      1, 1, 0 },
		{ OPT_GETSMS,            2, 5, 0 },
		{ OPT_DELETESMS,         2, 3, 0 },
		{ OPT_SENDSMS,           1, 10, 0 },
		{ OPT_SAVESMS,           0, 10, 0 },
		{ OPT_SENDLOGO,          3, 4, GAL_XOR },
		{ OPT_SENDRINGTONE,      2, 2, 0 },
		{ OPT_GETSMSC,           0, 3, 0 },
		{ OPT_GETWELCOMENOTE,    1, 1, 0 },
		{ OPT_SETWELCOMENOTE,    1, 1, 0 },
		{ OPT_NETMONITOR,        1, 1, 0 },
		{ OPT_SENDDTMF,          1, 1, 0 },
		{ OPT_SETLOGO,           1, 4, 0 },
		{ OPT_GETLOGO,           1, 4, 0 },
		{ OPT_VIEWLOGO,          1, 1, 0 },
		{ OPT_GETRINGTONE,       1, 3, 0 },
		{ OPT_SETRINGTONE,       1, 5, 0 },
		{ OPT_RESET,             0, 1, 0 },
		{ OPT_GETPROFILE,        0, 3, 0 },
		{ OPT_WRITEPHONEBOOK,    0, 1, 0 },
		{ OPT_DIVERT,            6, 10, 0 },
		{ OPT_GETWAPBOOKMARK,    1, 1, 0 },
		{ OPT_WRITEWAPBOOKMARK,  2, 2, 0 },
		{ OPT_DELETEWAPBOOKMARK, 1, 1, 0 },
		{ OPT_GETWAPSETTING,     1, 2, 0 },
		{ OPT_ACTIVATEWAPSETTING,1, 1, 0 },

		{ 0, 0, 0, 0 },
	};

	if (install_log_handler()) {
		fprintf(stderr, _("WARNING: cannot open logfile, logs will be directed to stderr\n"));
	}

	opterr = 0;

	/* For GNU gettext */
#ifdef ENABLE_NLS
	textdomain("gnokii");
	setlocale(LC_ALL, "");
#endif

	/* Introduce yourself */
	short_version();

	/* Read config file */
	if (gn_cfg_readconfig(&model, &Port, &Initlength, &Connection, &BinDir) < 0) {
		exit(1);
	}

	/* Handle command line arguments. */
	c = getopt_long(argc, argv, "", long_options, NULL);
	if (c == -1) 		/* No argument given - we should display usage. */
		usage(stderr);

	switch(c) {
	/* First, error conditions */
	case '?':
	case ':':
		fprintf(stderr, _("Use '%s --help' for usage information.\n"), argv[0]);
		exit(0);
	/* Then, options with no arguments */
	case OPT_HELP:
		usage(stdout);
	case OPT_VERSION:
		version();
		exit(0);
	}

	/* We have to build an array of the arguments which will be passed to the
	   functions.  Please note that every text after the --command will be
	   passed as arguments.  A syntax like gnokii --cmd1 args --cmd2 args will
	   not work as expected; instead args --cmd2 args is passed as a
	   parameter. */
	if ((nargv = malloc(sizeof(char *) * argc)) != NULL) {
		for (i = 2; i < argc; i++)
			nargv[i-2] = argv[i];

		if (checkargs(c, gals, nargc)) {
			free(nargv); /* Wrong number of arguments - we should display usage. */
			usage(stderr);
		}

#ifdef __svr4__
		/* have to ignore SIGALARM */
		sigignore(SIGALRM);
#endif

		/* Initialise the code for the GSM interface. */
		if (c != OPT_VIEWLOGO && c != OPT_FOOGLE && c != OPT_LISTNETWORKS) businit(NULL);

		switch(c) {
		case OPT_MONITOR:
			rc = monitormode();
			break;
#ifdef SECURITY
		case OPT_ENTERSECURITYCODE:
			rc = entersecuritycode(optarg);
			break;
		case OPT_GETSECURITYCODESTATUS:
			rc = getsecuritycodestatus();
			break;
		case OPT_CHANGESECURITYCODE:
			rc = changesecuritycode(optarg);
			break;
#endif
		case OPT_GETSECURITYCODE:
			rc = getsecuritycode();
			break;
		case OPT_GETDATETIME:
			rc = getdatetime();
			break;
		case OPT_GETALARM:
			rc = getalarm();
			break;
		case OPT_GETDISPLAYSTATUS:
			rc = getdisplaystatus();
			break;
		case OPT_PMON:
			rc = pmon();
			break;
		case OPT_WRITEPHONEBOOK:
			rc = writephonebook(nargc, nargv);
			break;
		/* Now, options with arguments */
		case OPT_SETDATETIME:
			rc = setdatetime(nargc, nargv);
			break;
		case OPT_SETALARM:
			rc = setalarm(nargc, nargv);
			break;
		case OPT_DIALVOICE:
			rc = dialvoice(optarg);
			break;
		case OPT_ANSWERCALL:
			rc = answercall(optarg);
			break;
		case OPT_HANGUP:
			rc = hangup(optarg);
			break;
		case OPT_GETTODO:
			rc = gettodo(nargc, nargv);
			break;
		case OPT_WRITETODO:
			rc = writetodo(nargv);
			break;
		case OPT_DELETEALLTODOS:
			rc = deletealltodos();
			break;
		case OPT_GETCALENDARNOTE:
			rc = getcalendarnote(nargc, nargv);
			break;
		case OPT_DELCALENDARNOTE:
			rc = deletecalendarnote(nargc, nargv);
			break;
		case OPT_WRITECALENDARNOTE:
			rc = writecalendarnote(nargv);
			break;
		case OPT_GETPHONEBOOK:
			rc = getphonebook(nargc, nargv);
			break;
		case OPT_GETSPEEDDIAL:
			rc = getspeeddial(optarg);
			break;
		case OPT_SETSPEEDDIAL:
			rc = setspeeddial(nargv);
			break;
		case OPT_CREATESMSFOLDER:
			rc = createsmsfolder(optarg);
			break;
		case OPT_DELETESMSFOLDER:
			rc = deletesmsfolder(optarg);
			break;
		case OPT_GETSMS:
			rc = getsms(argc, argv);
			break;
		case OPT_DELETESMS:
			rc = deletesms(nargc, nargv);
			break;
		case OPT_SENDSMS:
			rc = sendsms(nargc, nargv);
			break;
		case OPT_SAVESMS:
			rc = savesms(nargc, nargv);
			break;
		case OPT_SENDLOGO:
			rc = sendlogo(nargc, nargv);
			break;
		case OPT_GETSMSC:
			rc = getsmsc(argc, argv);
			break;
		case OPT_SETSMSC:
			rc = setsmsc();
			break;
		case OPT_NETMONITOR:
			rc = netmonitor(optarg);
			break;
		case OPT_IDENTIFY:
			rc = identify();
			break;
		case OPT_SETLOGO:
			rc = setlogo(nargc, nargv);
			break;
		case OPT_GETLOGO:
			rc = getlogo(nargc, nargv);
			break;
		case OPT_VIEWLOGO:
			rc = viewlogo(optarg);
			break;
		case OPT_GETRINGTONE:
			rc = getringtone(argc, argv);
			break;
		case OPT_SETRINGTONE:
			rc = setringtone(argc, argv);
			break;
		case OPT_SENDRINGTONE:
			rc = sendringtone(nargc, nargv);
			break;
		case OPT_GETPROFILE:
			rc = getprofile(argc, argv);
			break;
		case OPT_SETPROFILE:
			rc = setprofile();
			break;
		case OPT_DISPLAYOUTPUT:
			rc = displayoutput();
			break;
		case OPT_KEYPRESS:
			rc = presskeysequence();
			break;
		case OPT_ENTERCHAR:
			rc = enterchar();
			break;
		case OPT_SENDDTMF:
			rc = senddtmf(optarg);
			break;
		case OPT_RESET:
			rc = reset(optarg);
			break;
		case OPT_DIVERT:
			rc = divert(argc, argv);
			break;
		case OPT_SMSREADER:
			rc = smsreader();
			break;
		case OPT_GETWAPBOOKMARK:
			rc = getwapbookmark(optarg);
			break;
		case OPT_WRITEWAPBOOKMARK:
			rc = writewapbookmark(nargc, nargv);
			break;
		case OPT_DELETEWAPBOOKMARK:
			rc = deletewapbookmark(optarg);
			break;
		case OPT_GETWAPSETTING:
			rc = getwapsetting(nargc, nargv);
			break;
		case OPT_WRITEWAPSETTING:
			rc = writewapsetting();
			break;
		case OPT_ACTIVATEWAPSETTING:
			rc = activatewapsetting(optarg);
			break;
		case OPT_LISTNETWORKS:
			list_gsm_networks();
			break;
#ifndef WIN32
		case OPT_FOOGLE:
			rc = foogle(nargv);
			break;
#endif
		default:
			fprintf(stderr, _("Unknown option: %d\n"), c);
			break;

		}
		exit(rc);
	}

	fprintf(stderr, _("Wrong number of arguments\n"));
	exit(1);
}
