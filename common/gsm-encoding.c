/*

  $Id: gsm-encoding.c,v 1.24 2002-07-25 09:53:35 pkot Exp $

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

  Copyright (C) 2001 Pawe� Kot <pkot@linuxnews.pl>

  Functions for encoding SMS, calendar and other things.

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "misc.h"
#include "gsm-common.h"
#include "gsm-encoding.h"

#define NUMBER_OF_7_BIT_ALPHABET_ELEMENTS 128

#define ESCAPE_CHAR 0x1b

static unsigned char GSM_DefaultAlphabet[NUMBER_OF_7_BIT_ALPHABET_ELEMENTS] = {

	/* ETSI GSM 03.38, version 6.0.1, section 6.2.1; Default alphabet */
	/* Characters in hex position 10, [12 to 1a] and 24 are not present on
	   latin1 charset, so we cannot reproduce on the screen, however they are
	   greek symbol not present even on my Nokia */

	'@',  0xa3, '$',  0xa5, 0xe8, 0xe9, 0xf9, 0xec,
	0xf2, 0xc7, '\n', 0xd8, 0xf8, '\r', 0xc5, 0xe5,
	'?',  '_',  '?',  '?',  '?',  '?',  '?',  '?',
	'?',  '?',  '?',  '?',  0xc6, 0xe6, 0xdf, 0xc9,
	' ',  '!',  '\"', '#',  0xa4,  '%',  '&',  '\'',
	'(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
	'0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
	'8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
	0xa1, 'A',  'B',  'C',  'D',  'E',  'F',  'G',
	'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
	'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
	'X',  'Y',  'Z',  0xc4, 0xd6, 0xd1, 0xdc, 0xa7,
	0xbf, 'a',  'b',  'c',  'd',  'e',  'f',  'g',
	'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
	'x',  'y',  'z',  0xe4, 0xf6, 0xf1, 0xfc, 0xe0
};

static unsigned char GSM_ReverseDefaultAlphabet[256];
static bool reversed = false;

static void SetupReverse()
{
	int i;

	if (reversed) return;
	memset(GSM_ReverseDefaultAlphabet, 0x3f, 256);
	for (i = NUMBER_OF_7_BIT_ALPHABET_ELEMENTS - 1; i >= 0; i--)
		GSM_ReverseDefaultAlphabet[ GSM_DefaultAlphabet[i] ] = i;
	GSM_ReverseDefaultAlphabet['?'] = 0x3f;
	reversed = true;
}

static bool IsEscapeChar(unsigned char value)
{
	return (value == ESCAPE_CHAR);
}

/*
 * In GSM specification there are 10 characters in the extension
 * of the default alphabet. Their values look a bit random, they are
 * only 10, and probably they will never change, so hardcoding them
 * here is rather safe.
 */

static bool IsDefaultAlphabetExtensionChar(unsigned char value)
{
	wchar_t retval;

	if (mbtowc(&retval, &value, 1) == -1) return false;
	return (value == 0x0c ||
		value == '^' ||
		value == '{' ||
		value == '}' ||
		value == '\\' ||
		value == '[' ||
		value == '~' ||
		value == ']' ||
		value == '|' ||
		retval == 0x20ac);
}

static unsigned char DecodeWithDefaultAlphabetExtension(unsigned char value)
{
	switch (value) {
	case 0x0a: return 0x0c; break; /* from feed */
	case 0x14: return '^';  break;
	case 0x28: return '{';  break;
	case 0x29: return '}';  break;
	case 0x2f: return '\\'; break;
	case 0x3c: return '[';  break;
	case 0x3d: return '~';  break;
	case 0x3e: return ']';  break;
	case 0x40: return '|';  break;
	case 0x65: return 0xa4; break; /* euro */
	default: return '?';    break; /* invalid character */
	}
}

static unsigned char EncodeWithDefaultAlphabetExtension(unsigned char value)
{
	switch (value) {
	case 0x0c: return 0x0a; break; /* from feed */
	case '^':  return 0x14; break;
	case '{':  return '{';  break;
	case '}':  return '}';  break;
	case '\\': return '\\'; break;
	case '[':  return '[';  break;
	case '~':  return '~';  break;
	case ']':  return ']';  break;
	case '|':  return '|';  break;
	case 0xa4: return 0x65; break; /* euro */
	default: return 0x00;   break; /* invalid character */
	}
}

bool IsDefaultAlphabetString(unsigned char *string)
{
	unsigned int i, len = strlen(string);

	SetupReverse();
	for (i = 0; i < len; i++)
		if (!IsDefaultAlphabetExtensionChar(string[i]) &&
		    GSM_ReverseDefaultAlphabet[string[i]] == 0x3f &&
		    string[i] != '?')
			return false;
	return true;
}

static unsigned char EncodeWithDefaultAlphabet(unsigned char value)
{
	SetupReverse();
	return GSM_ReverseDefaultAlphabet[value];
}

static wchar_t EncodeWithUnicodeAlphabet(unsigned char value)
{
	wchar_t retval;

	if (mbtowc(&retval, &value, 1) == -1) return '?';
	else return retval;
}

static unsigned char DecodeWithDefaultAlphabet(unsigned char value)
{
	if (value < NUMBER_OF_7_BIT_ALPHABET_ELEMENTS) {
		return GSM_DefaultAlphabet[value];
	} else {
		return '?';
	}
}

static unsigned char DecodeWithUnicodeAlphabet(wchar_t value)
{
	unsigned char retval;

	if (wctomb(&retval, value) == -1) return '?';
	else return retval;
}


#define ByteMask ((1 << Bits) - 1)

int Unpack7BitCharacters(unsigned int offset, unsigned int in_length, unsigned int out_length,
			 unsigned char *input, unsigned char *output)
{
	unsigned char *OUT_NUM = output; /* Current pointer to the output buffer */
	unsigned char *IN_NUM = input;  /* Current pointer to the input buffer */
	unsigned char Rest = 0x00;
	int Bits;

	Bits = offset ? offset : 7;

	while ((IN_NUM - input) < in_length) {

		*OUT_NUM = ((*IN_NUM & ByteMask) << (7 - Bits)) | Rest;
		Rest = *IN_NUM >> Bits;

		/* If we don't start from 0th bit, we shouldn't go to the
		   next char. Under *OUT we have now 0 and under Rest -
		   _first_ part of the char. */
		if ((IN_NUM != input) || (Bits == 7)) OUT_NUM++;
		IN_NUM++;

		if ((OUT_NUM - output) >= out_length) break;

		/* After reading 7 octets we have read 7 full characters but
		   we have 7 bits as well. This is the next character */
		if (Bits == 1) {
			*OUT_NUM = Rest;
			OUT_NUM++;
			Bits = 7;
			Rest = 0x00;
		} else {
			Bits--;
		}
	}

	return OUT_NUM - output;
}

int Pack7BitCharacters(unsigned int offset, unsigned char *input, unsigned char *output, unsigned int *in_len)
{

	unsigned char *OUT_NUM = output; /* Current pointer to the output buffer */
	unsigned char *IN_NUM = input;  /* Current pointer to the input buffer */
	int Bits;		     /* Number of bits directly copied to
					the output buffer */

	Bits = (7 + offset) % 8;

	/* If we don't begin with 0th bit, we will write only a part of the
	   first octet */
	if (offset) {
		*OUT_NUM = 0x00;
		OUT_NUM++;
	}

	while ((IN_NUM - input) < strlen(input)) {
		unsigned char Byte;
		bool double_char = false;

		if (IsDefaultAlphabetExtensionChar(*IN_NUM)) {
			Byte = ESCAPE_CHAR;
			double_char = true;
			goto skip;
next_char:
			Byte = EncodeWithDefaultAlphabetExtension(*IN_NUM);
			double_char = false;
			(*in_len)++;
		} else {
			Byte = EncodeWithDefaultAlphabet(*IN_NUM);
		}
skip:
		*OUT_NUM = Byte >> (7 - Bits);
		/* If we don't write at 0th bit of the octet, we should write
		   a second part of the previous octet */
		if (Bits != 7)
			*(OUT_NUM-1) |= (Byte & ((1 << (7-Bits)) - 1)) << (Bits+1);

		Bits--;

		if (Bits == -1) Bits = 7;
		else OUT_NUM++;

		if (double_char) goto next_char;

		IN_NUM++;
	}

	return (OUT_NUM - output);
}

void DecodeAscii(unsigned char* dest, const unsigned char* src, int len)
{
	int i, j;

	for (i = 0, j = 0; j < len; i++, j++) {
		if (IsEscapeChar(src[j]))
			dest[i] = DecodeWithDefaultAlphabetExtension(src[++j]);
		else
			dest[i] = DecodeWithDefaultAlphabet(src[j]);
	}
	dest[i] = 0;
	return;
}

unsigned int EncodeAscii(unsigned char* dest, const unsigned char* src, unsigned int len)
{
	int i, j;

	for (i = 0, j = 0; j < len; i++, j++) {
		if (IsDefaultAlphabetExtensionChar(src[j])) {
			dest[i++] = ESCAPE_CHAR;
			dest[i] = EncodeWithDefaultAlphabetExtension(src[j]);
		} else {
			dest[i] = EncodeWithDefaultAlphabet(src[j]);
		}
	}
	return i;
}

void DecodeHex(unsigned char* dest, const unsigned char* src, int len)
{
	int i;
	char buf[3];

	buf[2] = '\0';
	for (i = 0; i < (len / 2); i++) {
		buf[0] = *(src + i * 2); buf[1] = *(src + i * 2 + 1);
		dest[i] = DecodeWithDefaultAlphabet(strtol(buf, NULL, 16));
	}
	return;
}

void EncodeHex(unsigned char* dest, const unsigned char* src, int len)
{
	int i;

	for (i = 0; i < (len / 2); i++) {
		sprintf(dest + i * 2, "%x", EncodeWithDefaultAlphabet(src[i]));
	}
	return;
}

void DecodeUCS2(unsigned char* dest, const unsigned char* src, int len)
{
	int i;
	char buf[5];

	buf[4] = '\0';
	for (i = 0; i < (len / 4); i++) {
		buf[0] = *(src + i * 4); buf[1] = *(src + i * 4 + 1);
		buf[2] = *(src + i * 4 + 2); buf[3] = *(src + i * 4 + 3);
		dest[i] = DecodeWithUnicodeAlphabet(strtol(buf, NULL, 16));
	}
	return;
}

void EncodeUCS2(unsigned char* dest, const unsigned char* src, int len)
{
	int i;

	for (i = 0; i < (len / 4); i++) {
		sprintf(dest + i * 4, "%lx", EncodeWithUnicodeAlphabet(src[i]));
	}
	return;
}

void DecodeUnicode(unsigned char* dest, const unsigned char* src, int len)
{
	int i;
	wchar_t wc;

	for (i = 0; i < len; i++) {
		wc = src[(2 * i) + 1] | (src[2 * i] << 8);
		dest[i] = DecodeWithUnicodeAlphabet(wc);
	}
	dest[len] = 0;
	return;
}

void EncodeUnicode(unsigned char* dest, const unsigned char* src, int len)
{
	int i;
	wchar_t wc;

	for (i = 0; i < len; i++) {
		wc = EncodeWithUnicodeAlphabet(src[i]);
		dest[i*2] = (wc >> 8) & 0xff;
		dest[(i*2)+1] = wc & 0xff;
	}
	return;
}

/* Conversion bin -> hex and hex -> bin */
void hex2bin(unsigned char *dest, const unsigned char *src, unsigned int len)
{
	int i;

	if (!dest) return;

	for (i = 0; i < len; i++) {
		unsigned aux;

		if (src[2 * i] >= '0' && src[2 * i] <= '9') aux = src[2 * i] - '0';
		else if (src[2 * i] >= 'a' && src[2 * i] <= 'f') aux = src[2 * i] - 'a' + 10;
		else if (src[2 * i] >= 'A' && src[2 * i] <= 'F') aux = src[2 * i] - 'A' + 10;
		else {
			dest[0] = 0;
			return;
		}
		dest[i] = aux << 4;
		if (src[2 * i + 1] >= '0' && src[2 * i + 1] <= '9') aux = src[2 * i + 1] - '0';
		else if (src[2 * i + 1] >= 'a' && src[2 * i + 1] <= 'f') aux = src[2 * i + 1] - 'a' + 10;
		else if (src[2 * i + 1] >= 'A' && src[2 * i + 1] <= 'F') aux = src[2 * i + 1] - 'A' + 10;
		else {
			dest[0] = 0;
			return;
		}
		dest[i] |= aux;
	}
}

void bin2hex(unsigned char *dest, const unsigned char *src, unsigned int len)
{
	int i;

	if (!dest) return;

	for (i = 0; i < len; i++) {
		dest[2 * i] = (src[i] & 0xf0) >> 4;
		if (dest[2 * i] < 10) dest[2 * i] += '0';
		else dest[2 * i] += ('A' - 10);
		dest[2 * i + 1] = src[i] & 0x0f;
		if (dest[2 * i + 1] < 10) dest[2 * i + 1] += '0';
		else dest[2 * i + 1] += ('A' - 10);
	}
}

/* This function implements packing of numbers (SMS Center number and
   destination number) for SMS sending function. */
int SemiOctetPack(char *Number, unsigned char *Output, SMS_NumberType type)
{
	unsigned char *IN_NUM = Number;  /* Pointer to the input number */
	unsigned char *OUT_NUM = Output; /* Pointer to the output */
	int count = 0; /* This variable is used to notify us about count of already
			  packed numbers. */

	/* The first byte in the Semi-octet representation of the address field is
	   the Type-of-Address. This field is described in the official GSM
	   specification 03.40 version 6.1.0, section 9.1.2.5, page 33. We support
	   only international and unknown number. */

	*OUT_NUM++ = type;
	if (type == SMS_International) IN_NUM++; /* Skip '+' */
	if ((type == SMS_Unknown) && (*IN_NUM == '+')) IN_NUM++; /* Optional '+' in Unknown number type */

	/* The next field is the number. It is in semi-octet representation - see
	   GSM scpecification 03.40 version 6.1.0, section 9.1.2.3, page 31. */
	while (*IN_NUM) {
		if (count & 0x01) {
			*OUT_NUM = *OUT_NUM | ((*IN_NUM - '0') << 4);
			OUT_NUM++;
		}
		else
			*OUT_NUM = *IN_NUM - '0';
		count++; IN_NUM++;
	}

	/* We should also fill in the most significant bits of the last byte with
	   0x0f (1111 binary) if the number is represented with odd number of
	   digits. */
	if (count & 0x01) {
		*OUT_NUM = *OUT_NUM | 0xf0;
		OUT_NUM++;
	}

	return (2 * (OUT_NUM - Output - 1) - (count % 2));
}

char *GetBCDNumber(u8 *Number)
{
	static char Buffer[MAX_BCD_STRING_LENGTH] = "";
	int length = Number[0]; /* This is the length of BCD coded number */
	int count, Digit;

	if (length > MAX_BCD_STRING_LENGTH) length = MAX_BCD_STRING_LENGTH;
	memset(Buffer, 0, MAX_BCD_STRING_LENGTH);
	switch (Number[1]) {
	case SMS_Alphanumeric:
		Unpack7BitCharacters(0, length, length, Number + 2, Buffer);
		Buffer[length] = 0;
		break;
	case SMS_International:
		sprintf(Buffer, "+");
		if (length == MAX_BCD_STRING_LENGTH) length--; /* avoid overflow */
	case SMS_Unknown:
	case SMS_National:
	case SMS_Network:
	case SMS_Subscriber:
	case SMS_Abbreviated:
	default:
		for (count = 0; count < length - 1; count++) {
			Digit = Number[count+2] & 0x0f;
			if (Digit < 10) sprintf(Buffer, "%s%d", Buffer, Digit);
			Digit = Number[count+2] >> 4;
			if (Digit < 10) sprintf(Buffer, "%s%d", Buffer, Digit);
		}
		break;
	}
	return Buffer;
}
