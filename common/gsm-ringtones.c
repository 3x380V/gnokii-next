/*

  $Id: gsm-ringtones.c,v 1.12 2002-06-10 21:16:36 pkot Exp $

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
  Copyright (C) 2002       Pavel Machek <pavel@ucw.cz>
  

  This file provides support for ringtones.

*/

#include "gsm-sms.h"
#include "gsm-common.h"
#include "gsm-ringtones.h"
#include "misc.h"

/* Beats-per-Minute Encoding */

int BeatsPerMinute[] = {
	25,
	28,
	31,
	35,
	40,
	45,
	50,
	56,
	63,
	70,
	80,
	90,
	100,
	112,
	125,
	140,
	160,
	180,
	200,
	225,
	250,
	285,
	320,
	355,
	400,
	450,
	500,
	565,
	635,
	715,
	800,
	900
};

int OctetAlign(unsigned char *Dest, int CurrentBit)
{
	int i=0;

	while((CurrentBit+i)%8) {
		ClearBit(Dest, CurrentBit+i);
		i++;
	}

	return CurrentBit+i;
}

int OctetAlignNumber(int CurrentBit)
{
	int i=0;

	while((CurrentBit+i)%8) {
		i++;
	}

	return CurrentBit+i;
}

int BitPack(unsigned char *Dest, int CurrentBit, unsigned char *Source, int Bits)
{
	int i;

	for (i=0; i<Bits; i++)
		if (GetBit(Source, i))
			SetBit(Dest, CurrentBit+i);
		else
			ClearBit(Dest, CurrentBit+i);

	return CurrentBit+Bits;
}

int GetTempo(int Beats)
{
	int i=0;

	while ( i < sizeof(BeatsPerMinute)/sizeof(BeatsPerMinute[0])) {

		if (Beats<=BeatsPerMinute[i])
			break;
		i++;
	}

	return (i << 3);
}

int BitPackByte(unsigned char *Dest, int CurrentBit, unsigned char Command, int Bits)
{
	unsigned char Byte[]={Command};

	return BitPack(Dest, CurrentBit, Byte, Bits);
}



/* This is messy but saves using the math library! */

int GSM_GetDuration(int number, unsigned char *spec)
{
	int duration=0;

	switch (number) {

	case 128*3/2:
		duration=Duration_Full; *spec=DottedNote; break;
	case 128*2/3:
		duration=Duration_Full; *spec=Length_2_3; break;
	case 128:
		duration=Duration_Full; *spec=NoSpecialDuration; break;
	case 64*9/4:
		duration=Duration_1_2; *spec=DoubleDottedNote; break;
	case 64*3/2:
		duration=Duration_1_2; *spec=DottedNote; break;
	case 64*2/3:
		duration=Duration_1_2; *spec=Length_2_3; break;
	case 64:
		duration=Duration_1_2; *spec=NoSpecialDuration; break;
	case 32*9/4:
		duration=Duration_1_4; *spec=DoubleDottedNote; break;
	case 32*3/2:
		duration=Duration_1_4; *spec=DottedNote; break;
	case 32*2/3:
		duration=Duration_1_4; *spec=Length_2_3; break;
	case 32:
		duration=Duration_1_4; *spec=NoSpecialDuration; break;
	case 16*9/4:
		duration=Duration_1_8; *spec=DoubleDottedNote; break;
	case 16*3/2:
		duration=Duration_1_8; *spec=DottedNote; break;
	case 16*2/3:
		duration=Duration_1_8; *spec=Length_2_3; break;
	case 16:
		duration=Duration_1_8; *spec=NoSpecialDuration; break;
	case 8*9/4:
		duration=Duration_1_16; *spec=DoubleDottedNote; break;
	case 8*3/2:
		duration=Duration_1_16; *spec=DottedNote; break;
	case 8*2/3:
		duration=Duration_1_16; *spec=Length_2_3; break;
	case 8:
		duration=Duration_1_16; *spec=NoSpecialDuration; break;
	case 4*9/4:
		duration=Duration_1_32; *spec=DoubleDottedNote; break;
	case 4*3/2:
		duration=Duration_1_32; *spec=DottedNote; break;
	case 4*2/3:
		duration=Duration_1_32; *spec=Length_2_3; break;
	case 4:
		duration=Duration_1_32; *spec=NoSpecialDuration; break;
	}

	return duration;
}


API int GSM_GetNote(int number)
{
	int note = 0;

	if (number != 255) {
		note = number % 14;
		switch (note) {
		case 0:
			note=Note_C; break;
		case 1:
			note=Note_Cis; break;
		case 2:
			note=Note_D; break;
		case 3:
			note=Note_Dis; break;
		case 4:
			note=Note_E; break;
		case 6:
			note=Note_F; break;
		case 7:
			note=Note_Fis; break;
		case 8:
			note=Note_G; break;
		case 9:
			note=Note_Gis; break;
		case 10:
			note=Note_A; break;
		case 11:
			note=Note_Ais; break;
		case 12:
			note=Note_H; break;
		}
	}
	else note = Note_Pause;

	return note;

}

int GSM_GetScale(int number)
{
	int scale=-1;

	if (number!=255) {
		scale=number/14;

		/* Ensure the scale is valid */
		scale%=4;

		scale=scale<<6;
	}
	return scale;
}


/* This function packs the ringtone from the structure, so it can be set
   or sent via sms to another phone.
   Function returns number of packed notes and changes maxlength to
   number of used chars in "package" */

API u8 GSM_PackRingtone(GSM_Ringtone *ringtone, unsigned char *package, int *maxlength)
{
	int StartBit=0;
	int i;
	unsigned char CommandLength = 0x02;
	unsigned char spec;
	int oldscale=10, newscale;
	int HowMany=0, HowLong=0, StartNote=0, EndNote=0;

	StartBit=BitPackByte(package, StartBit, CommandLength, 8);
	StartBit=BitPackByte(package, StartBit, RingingToneProgramming, 7);

	/* The page 3-23 of the specs says that <command-part> is always
	   octet-aligned. */
	StartBit=OctetAlign(package, StartBit);

	StartBit=BitPackByte(package, StartBit, Sound, 7);
	StartBit=BitPackByte(package, StartBit, BasicSongType, 3);

	/* Packing the name of the tune. */
	StartBit=BitPackByte(package, StartBit, strlen(ringtone->name)<<4, 4);
	StartBit=BitPack(package, StartBit, ringtone->name, 8*strlen(ringtone->name));

	/* Info about song pattern */
	StartBit=BitPackByte(package, StartBit, 0x01, 8); /* One song pattern */
	StartBit=BitPackByte(package, StartBit, PatternHeaderId, 3);
	StartBit=BitPackByte(package, StartBit, A_part, 2);
	StartBit=BitPackByte(package, StartBit, 0, 4); /* No loop value */

	/* Info, how long is contents for SMS */
	HowLong=30+8*strlen(ringtone->name)+17+8+8+13;

	/* Calculate the number of instructions in the tune.
	   Each Note contains Note and (sometimes) Scale.
	   Default Tempo and Style are instructions too. */
	HowMany=2; /* Default Tempo and Style */

	for(i=0; i<ringtone->NrNotes; i++) {

		/* PC Composer 2.0.010 doesn't like, when we start ringtone from pause:
		   it displays that the format is invalid and
		   hangs, when you move mouse over place, where pause is */
		if (GSM_GetNote(ringtone->notes[i].note)==Note_Pause && oldscale==10) {
			StartNote++;
		} else {

			/* we don't write Scale info before "Pause" note - it saves space */
			if (GSM_GetNote(ringtone->notes[i].note)!=Note_Pause &&
			    oldscale!=(newscale=GSM_GetScale(ringtone->notes[i].note))) {

				/* We calculate, if we have space to add next scale instruction */
				if (((HowLong+5)/8)<=(*maxlength-1)) {
					oldscale=newscale;
					HowMany++;
					HowLong+=5;
				} else {
					break;
				}
			}

			/* We calculate, if we have space to add next note instruction */
			if (((HowLong+12)/8)<=(*maxlength-1)) {
				HowMany++;
				EndNote++;
				HowLong+=12;
			} else {
				break;
			}
		}

		/* If we are sure, we pack it for SMS or setting to phone, not for OTT file */
		if (*maxlength<1000) {
			/* Pc Composer gives this as the phone limitation */
			if ((EndNote-StartNote)==GSM_MAX_RINGTONE_NOTES-1) break;
		}
	}

	StartBit=BitPackByte(package, StartBit, HowMany, 8);

	/* Style */
	StartBit=BitPackByte(package, StartBit, StyleInstructionId, 3);
	StartBit=BitPackByte(package, StartBit, ContinuousStyle, 2);

	/* Beats per minute/tempo of the tune */
	StartBit=BitPackByte(package, StartBit, TempoInstructionId, 3);
	StartBit=BitPackByte(package, StartBit, GetTempo(ringtone->tempo), 5);

	/* Default scale */
	oldscale=10;

	/* Notes packing */
	for(i=StartNote; i<(EndNote+StartNote); i++) {

		/* we don't write Scale info before "Pause" note - it saves place */
		if (GSM_GetNote(ringtone->notes[i].note)!=Note_Pause &&
		    oldscale!=(newscale=GSM_GetScale(ringtone->notes[i].note))) {
			oldscale=newscale;
			StartBit=BitPackByte(package, StartBit, ScaleInstructionId, 3);
			StartBit=BitPackByte(package, StartBit, GSM_GetScale(ringtone->notes[i].note), 2);
		}

		/* Note */
		StartBit=BitPackByte(package, StartBit, NoteInstructionId, 3);
		StartBit=BitPackByte(package, StartBit, GSM_GetNote(ringtone->notes[i].note), 4);
		StartBit=BitPackByte(package, StartBit, GSM_GetDuration(ringtone->notes[i].duration,&spec), 3);
		StartBit=BitPackByte(package, StartBit, spec, 2);
	}

	StartBit=OctetAlign(package, StartBit);

	StartBit=BitPackByte(package, StartBit, CommandEnd, 8);

	if (StartBit!=OctetAlignNumber(HowLong))
		dprintf("Error in PackRingtone - StartBit different to HowLong %d - %d)\n", StartBit,OctetAlignNumber(HowLong));

	*maxlength=StartBit/8;

	return(EndNote+StartNote);
}


int BitUnPack(unsigned char *Dest, int CurrentBit, unsigned char *Source, int Bits)
{
	int i;

	for (i=0; i<Bits; i++)
		if (GetBit(Dest, CurrentBit+i)) {
			SetBit(Source, i);
		} else {
			ClearBit(Source, i);
		}

	return CurrentBit+Bits;
}

int BitUnPackInt(unsigned char *Src, int CurrentBit, int *integer, int Bits)
{
	int l=0,z=128,i;

	for (i=0; i<Bits; i++) {
		if (GetBit(Src, CurrentBit+i)) l=l+z;
		z=z/2;
	}

	*integer=l;

	return CurrentBit+i;
}

int OctetUnAlign(int CurrentBit)
{
	int i=0;

	while((CurrentBit+i)%8) i++;

	return CurrentBit+i;
}


/* TODO: better checking, if contents of ringtone is OK */

API GSM_Error GSM_UnPackRingtone(GSM_Ringtone *ringtone, unsigned char *package, int maxlength)
{
	int StartBit = 0;
	int spec, duration, scale;
	int HowMany;
	int l, q, i;

	StartBit = BitUnPackInt(package, StartBit, &l, 8);
	if (l != 0x02) {
		dprintf("Not header\n");
		return GE_SUBFORMATNOTSUPPORTED;
	}

	StartBit = BitUnPackInt(package, StartBit, &l, 7);
	if (l != RingingToneProgramming) {
		dprintf("Not RingingToneProgramming\n");
		return GE_SUBFORMATNOTSUPPORTED;
	}

/* The page 3-23 of the specs says that <command-part> is always
   octet-aligned. */
	StartBit = OctetUnAlign(StartBit);

	StartBit = BitUnPackInt(package, StartBit, &l, 7);
	if (l != Sound) {
		dprintf("Not Sound\n");
		return GE_SUBFORMATNOTSUPPORTED;
	}

	StartBit = BitUnPackInt(package, StartBit, &l, 3);
	if (l != BasicSongType) {
		dprintf("Not BasicSongType\n");
		return GE_SUBFORMATNOTSUPPORTED;
	}

/* Getting length of the tune name */
	StartBit = BitUnPackInt(package, StartBit, &l, 4);
	l = l >> 4;

/* Unpacking the name of the tune. */
	StartBit = BitUnPack(package, StartBit, ringtone->name, 8*l);
	ringtone->name[l] = 0;

	StartBit = BitUnPackInt(package, StartBit, &l, 8);
	if (l != 1) return GE_SUBFORMATNOTSUPPORTED; //we support only one song pattern

	StartBit = BitUnPackInt(package, StartBit, &l, 3);
	if (l != PatternHeaderId) {
		dprintf("Not PatternHeaderId\n");
		return GE_SUBFORMATNOTSUPPORTED;
	}

	StartBit += 2; //Pattern ID - we ignore it

	StartBit = BitUnPackInt(package, StartBit, &l, 4);

	HowMany = 0;
	StartBit = BitUnPackInt(package, StartBit, &HowMany, 8);

	scale = 0;
	ringtone->NrNotes = 0;

	for (i = 0; i < HowMany; i++) {

		StartBit = BitUnPackInt(package, StartBit, &q, 3);
		switch (q) {
		case VolumeInstructionId:
			StartBit += 4;
			break;
		case StyleInstructionId:
			StartBit = BitUnPackInt(package,StartBit,&l,2);
			l = l >> 3;
			break;
		case TempoInstructionId:
			StartBit = BitUnPackInt(package, StartBit, &l, 5);
			l = l >> 3;
			ringtone->tempo = BeatsPerMinute[l];
			break;
		case ScaleInstructionId:
			StartBit = BitUnPackInt(package, StartBit, &scale, 2);
			scale = scale >> 6;
			break;
		case NoteInstructionId:
			StartBit = BitUnPackInt(package, StartBit, &l, 4);

			switch (l) {
			case Note_C   :ringtone->notes[ringtone->NrNotes].note = 0;   break;
			case Note_Cis :ringtone->notes[ringtone->NrNotes].note = 1;   break;
			case Note_D   :ringtone->notes[ringtone->NrNotes].note = 2;   break;
			case Note_Dis :ringtone->notes[ringtone->NrNotes].note = 3;   break;
			case Note_E   :ringtone->notes[ringtone->NrNotes].note = 4;   break;
			case Note_F   :ringtone->notes[ringtone->NrNotes].note = 6;   break;
			case Note_Fis :ringtone->notes[ringtone->NrNotes].note = 7;   break;
			case Note_G   :ringtone->notes[ringtone->NrNotes].note = 8;   break;
			case Note_Gis :ringtone->notes[ringtone->NrNotes].note = 9;   break;
			case Note_A   :ringtone->notes[ringtone->NrNotes].note = 10;  break;
			case Note_Ais :ringtone->notes[ringtone->NrNotes].note = 11;  break;
			case Note_H   :ringtone->notes[ringtone->NrNotes].note = 12;  break;
			default       :ringtone->notes[ringtone->NrNotes].note = 255; break; //Pause ?
			}

			if (ringtone->notes[ringtone->NrNotes].note != 255)
				ringtone->notes[ringtone->NrNotes].note = ringtone->notes[ringtone->NrNotes].note + scale*14;

			StartBit = BitUnPackInt(package, StartBit, &duration, 3);

			StartBit = BitUnPackInt(package, StartBit, &spec, 2);

			if (duration==Duration_Full && spec==DottedNote)
				ringtone->notes[ringtone->NrNotes].duration=128*3/2;
			if (duration==Duration_Full && spec==Length_2_3)
				ringtone->notes[ringtone->NrNotes].duration=128*2/3;
			if (duration==Duration_Full && spec==NoSpecialDuration)
				ringtone->notes[ringtone->NrNotes].duration=128;
			if (duration==Duration_1_2 && spec==DottedNote)
				ringtone->notes[ringtone->NrNotes].duration=64*3/2;
			if (duration==Duration_1_2 && spec==Length_2_3)
				ringtone->notes[ringtone->NrNotes].duration=64*2/3;
			if (duration==Duration_1_2 && spec==NoSpecialDuration)
				ringtone->notes[ringtone->NrNotes].duration=64;
			if (duration==Duration_1_4 && spec==DottedNote)
				ringtone->notes[ringtone->NrNotes].duration=32*3/2;
			if (duration==Duration_1_4 && spec==Length_2_3)
				ringtone->notes[ringtone->NrNotes].duration=32*2/3;
			if (duration==Duration_1_4 && spec==NoSpecialDuration)
				ringtone->notes[ringtone->NrNotes].duration=32;
			if (duration==Duration_1_8 && spec==DottedNote)
				ringtone->notes[ringtone->NrNotes].duration=16*3/2;
			if (duration==Duration_1_8 && spec==Length_2_3)
				ringtone->notes[ringtone->NrNotes].duration=16*2/3;
			if (duration==Duration_1_8 && spec==NoSpecialDuration)
				ringtone->notes[ringtone->NrNotes].duration=16;
			if (duration==Duration_1_16 && spec==DottedNote)
				ringtone->notes[ringtone->NrNotes].duration=8*3/2;
			if (duration==Duration_1_16 && spec==Length_2_3)
				ringtone->notes[ringtone->NrNotes].duration=8*2/3;
			if (duration==Duration_1_16 && spec==NoSpecialDuration)
				ringtone->notes[ringtone->NrNotes].duration=8;
			if (duration==Duration_1_32 && spec==DottedNote)
				ringtone->notes[ringtone->NrNotes].duration=4*3/2;
			if (duration==Duration_1_32 && spec==Length_2_3)
				ringtone->notes[ringtone->NrNotes].duration=4*2/3;
			if (duration==Duration_1_32 && spec==NoSpecialDuration)
				ringtone->notes[ringtone->NrNotes].duration=4;

			if (ringtone->NrNotes==MAX_RINGTONE_NOTES) break;

			ringtone->NrNotes++;
			break;
		default:
			dprintf("Unsupported block\n");
			return GE_SUBFORMATNOTSUPPORTED;
		}
	}

	return GE_NONE;
}


GSM_Error GSM_ReadRingtoneFromSMS(GSM_API_SMS *message, GSM_Ringtone *ringtone)
{
	if (message->UDH.UDH[0].Type == SMS_Ringtone) {
		return GSM_UnPackRingtone(ringtone, message->UserData[0].u.Text, message->UserData[0].Length);
	} else return GE_SUBFORMATNOTSUPPORTED;
}

int GSM_EncodeSMSRingtone(unsigned char *message, GSM_Ringtone *ringtone)
{
	int j = GSM_MAX_8BIT_SMS_LENGTH;
	GSM_PackRingtone(ringtone, message, &j);
	return j;
}

/* Returns message length */
int GSM_EncodeSMSiMelody(unsigned char *imelody, unsigned char *message)
{
	unsigned int current = 0;

	dprintf("EMS iMelody\n");
	message[current++] = strlen(imelody) + 3;
	message[current++] = 0x0c; 	/* iMelody code */
	message[current++] = strlen(imelody) + 1;
	message[current++] = 0;		      /* Position in text this melody is at */
	strcpy(message + current, imelody);

	return (current + strlen(imelody));
}
