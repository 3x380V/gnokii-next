/*

  $Id: encoding.h,v 1.2 2001-11-08 16:34:20 pkot Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2001 Pawe� Kot <pkot@linuxnews.pl>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Include file for encoding functions.

  $Log: encoding.h,v $
  Revision 1.2  2001-11-08 16:34:20  pkot
  Updates to work with new libsms

  Revision 1.1  2001/10/24 22:37:25  pkot
  Moved encoding functions to a separate file


*/

#ifndef __gsm_encoding_h_
#define __gsm_encoding_h_

int Unpack7BitCharacters(int offset, int in_length, int out_length,
                           unsigned char *input, unsigned char *output);
int Pack7BitCharacters(int offset, unsigned char *input, unsigned char *output);

void DecodeUnicode (unsigned char* dest, const unsigned char* src, int len);
void EncodeUnicode (unsigned char* dest, const unsigned char* src, int len);

void DecodeAscii (unsigned char* dest, const unsigned char* src, int len);
void EncodeAscii (unsigned char* dest, const unsigned char* src, int len);

#endif
