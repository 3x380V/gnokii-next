/*

  $Id: encoding.h,v 1.4 2002-01-15 12:01:02 pkot Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2001 Pawe� Kot <pkot@linuxnews.pl>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Include file for encoding functions.

  $Log: encoding.h,v $
  Revision 1.4  2002-01-15 12:01:02  pkot
  SMS sending for AT mode. It does nt work yet, but putting it into CVS gives
  more opportunity for the code to get tested ;-)
  Fixes to the commit I made tonight that broke compile. Commits at 2 am are
  not A Good Thing (tm).
  Moved bin2hex and hex2bin functions to gsm-encoding.c.
  Cleanups.

  Revision 1.3  2002/01/11 11:11:46  pkot
  Character set setting in AT mode (Manfred Jonsson)

  Revision 1.2  2001/11/08 16:34:20  pkot
  Updates to work with new libsms

  Revision 1.1  2001/10/24 22:37:25  pkot
  Moved encoding functions to a separate file


*/

#ifndef __gsm_encoding_h_
#define __gsm_encoding_h_

extern void hex2bin(unsigned char *dest, const unsigned char *src, unsigned int len);
extern void bin2hex(unsigned char *dest, const unsigned char *src, unsigned int len);

int Unpack7BitCharacters(int offset, int in_length, int out_length,
                           unsigned char *input, unsigned char *output);
int Pack7BitCharacters(int offset, unsigned char *input, unsigned char *output);

void DecodeUnicode (unsigned char* dest, const unsigned char* src, int len);
void EncodeUnicode (unsigned char* dest, const unsigned char* src, int len);

void DecodeAscii (unsigned char* dest, const unsigned char* src, int len);
void EncodeAscii (unsigned char* dest, const unsigned char* src, int len);

void DecodeHex (unsigned char* dest, const unsigned char* src, int len);
void EncodeHex (unsigned char* dest, const unsigned char* src, int len);

void DecodeUCS2 (unsigned char* dest, const unsigned char* src, int len);
void EncodeUCS2 (unsigned char* dest, const unsigned char* src, int len);

#endif
