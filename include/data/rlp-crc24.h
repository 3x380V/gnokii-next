/*

  $Id: rlp-crc24.h,v 1.3 2002-01-01 22:54:33 pkot Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Jan�k ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file:  rlp-crc24.h   Version 0.3.1

  Header file for CRC24 (aka FCS) implementation in RLP.

*/

#ifndef __data_rlp_crc24_h
#define __data_rlp_crc24_h

#ifndef __misc_h
#  include    "misc.h"
#endif

/* Prototypes for functions */

void RLP_CalculateCRC24Checksum(u8 *data, int length, u8 *crc);
bool RLP_CheckCRC24FCS(u8 *data, int length);

#endif
