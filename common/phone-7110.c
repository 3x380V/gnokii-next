/*

  $Id: phone-7110.c,v 1.3 2001-01-23 15:32:41 chris Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Jan�k ml.
  Copytight (C) 2000 Chris Kemp

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides functions specific to the 7110 series. 
  See README for more details on supported mobile phones.

  The various routines are called P7110_(whatever).

  $Log: phone-7110.c,v $
  Revision 1.3  2001-01-23 15:32:41  chris
  Pavel's 'break' and 'static' corrections.
  Work on logos for 7110.

  Revision 1.2  2001/01/17 02:54:54  chris
  More 7110 work.  Use with care! (eg it is not possible to delete phonebook entries)
  I can now edit my phonebook in xgnokii but it is 'work in progress'.

  Revision 1.1  2001/01/14 22:46:59  chris
  Preliminary 7110 support (dlr9 only) and the beginnings of a new structure


*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define __phone_7110_c  /* Turn on prototypes in phone-7110.h */
#include "misc.h"
#include "gsm-common.h"
#include "phone-generic.h"
#include "phone-7110.h"
#include "fbus-generic.h"


/* Some globals */
/* Note that these could be created in initialise */
/* which would enable multiphone support */

GSM_Link link;
GSM_IncomingFunctionType P7110_IncomingFunctions[] = {
  { 0x01, P7110_GenericCRHandler },
  { 0x03, P7110_GenericCRHandler },
  { 0x0a, P7110_GenericCRHandler },
  { 0x17, P7110_GenericCRHandler },
  { 0x1b, P7110_GenericCRHandler },
  { 0x7a, P7110_GenericCRHandler }
};
GSM_Phone phone = {
   6,  /* No of functions in array */
   P7110_IncomingFunctions
};


/* Mobile phone information */

GSM_Information P7110_Information = {
  "7110|6210", /* Supported models */
  7,                     /* Max RF Level */
  0,                     /* Min RF Level */
  GRF_Percentage,        /* RF level units */
  7,                     /* Max Battery Level */
  0,                     /* Min Battery Level */
  GBU_Percentage,        /* Battery level units */
  GDT_DateTime,          /* Have date/time support */
  GDT_TimeOnly,	         /* Alarm supports time only */
  1,                     /* Alarms available - FIXME */
  65, 96,                /* Startup logo size */
  21, 78,                /* Op logo size */
  14, 72                 /* Caller logo size */
};


/* Here we initialise model specific functions called by 'gnokii'. */
/* This too needs fixing .. perhaps pass the link a 'request' of certain */
/* type and the link then searches the phone functions.... */

GSM_Functions P7110_Functions = {
  P7110_Initialise,
  P7110_Terminate,
  P7110_ReadPhonebook, /* GetMemoryLocation */
  P7110_WritePhonebookLocation, /* WritePhonebookLocation */
  UNIMPLEMENTED, /* GetSpeedDial */
  UNIMPLEMENTED, /* SetSpeedDial */
  P7110_GetMemoryStatus, /* GetMemoryStatus */
  UNIMPLEMENTED, /* GetSMSStatus */
  UNIMPLEMENTED, /* GetSMSCentre */
  UNIMPLEMENTED, /* SetSMSCentre */
  UNIMPLEMENTED, /* GetSMSMessage */
  UNIMPLEMENTED, /* DeleteSMSMessage */
  UNIMPLEMENTED, /* SendSMSMessage */
  UNIMPLEMENTED, /* SaveSMSMessage */
  P7110_GetRFLevel, /* GetRFLevel */
  P7110_GetBatteryLevel, /* GetBatteryLevel */
  UNIMPLEMENTED, /* GetPowerSource */
  UNIMPLEMENTED, /* GetDisplayStatus */
  UNIMPLEMENTED, /* EnterSecurityCode */
  UNIMPLEMENTED, /* GetSecurityCodeStatus */
  P7110_GetIMEI,        /* GetIMEI */
  P7110_GetRevision,    /* GetRevision */
  P7110_GetModel,       /* GetModel */
  UNIMPLEMENTED, /* GetDateTime */
  UNIMPLEMENTED, /* SetDateTime */
  UNIMPLEMENTED, /* GetAlarm */
  UNIMPLEMENTED, /* SetAlarm */
  P7110_DialVoice, /* DialVoice */
  UNIMPLEMENTED, /* DialData */
  UNIMPLEMENTED, /* GetIncomingCallNr */
  UNIMPLEMENTED, /* GetNetworkInfo */
  UNIMPLEMENTED, /* GetCalendarNote */
  UNIMPLEMENTED, /* WriteCalendarNote */
  UNIMPLEMENTED, /* DeleteCalendarNote */
  UNIMPLEMENTED, /* NetMonitor */
  UNIMPLEMENTED, /* SendDTMF */
  P7110_GetBitmap, /* GetBitmap */
  P7110_SetBitmap, /* SetBitmap */
  UNIMPLEMENTED, /* SetRingtone */
  UNIMPLEMENTED, /* SendRingtone */
  UNIMPLEMENTED, /* Reset */ 
  UNIMPLEMENTED, /* GetProfile */
  UNIMPLEMENTED, /* SetProfile */
  P7110_SendRLPFrame,   /* SendRLPFrame */
  UNIMPLEMENTED, /* CancelCall */
  UNIMPLEMENTED, /* EnableDisplayOutput */
  UNIMPLEMENTED, /* DisableDisplayOutput */
  UNIMPLEMENTED, /* EnableCellBroadcast */
  UNIMPLEMENTED, /* DisableCellBroadcast */
  UNIMPLEMENTED  /* ReadCellBroadcast */
};

/* LinkOK is always true for now... */
bool P7110_LinkOK=true;

static void P7110_Terminate()
{
  
};


/* Simple unicode stuff from Marcin for now */
/*
Simple UNICODE decoding and encoding from/to iso-8859-2
Martin Kacer <M.Kacer@sh.cvut.cz>

Following table contains triplets:
first unicode byte, second unicode byte, iso-8859-2 character

If character is not found, first unicode byte is set to zero
and second one is the same as iso-8859-2 character.
*/
static const char unicode_table[][3] =
{
	/* C< D< E< N< R< S< T< Uo Z< */
	{0x01, 0x0C, 0xC8}, {0x01, 0x0E, 0xCF}, {0x01, 0x1A, 0xCC},
	{0x01, 0x47, 0xD2}, {0x01, 0x58, 0xD8}, {0x01, 0x60, 0xA9},
	{0x01, 0x64, 0xAB}, {0x01, 0x6E, 0xD9}, {0x01, 0x7D, 0xAE},
	/* c< d< e< n< r< s< t< uo z< */
	{0x01, 0x0D, 0xE8}, {0x01, 0x0F, 0xEF}, {0x01, 0x1B, 0xEC},
	{0x01, 0x48, 0xF2}, {0x01, 0x59, 0xF8}, {0x01, 0x61, 0xB9},
	{0x01, 0x65, 0xBB}, {0x01, 0x6F, 0xF9}, {0x01, 0x7E, 0xBE},
	/* A< A, C' D/ E, L< L' L/ */
	{0x01, 0x02, 0xC3}, {0x01, 0x04, 0xA1}, {0x01, 0x06, 0xC6},
	{0x01, 0x10, 0xD0}, {0x01, 0x18, 0xCA}, {0x01, 0x3D, 0xA5},
	{0x01, 0x39, 0xC5}, {0x01, 0x41, 0xA3},
	/* N' O" R' S' S, T, U" Z' Z. */
	{0x01, 0x43, 0xD1}, {0x01, 0x50, 0xD5}, {0x01, 0x54, 0xC0},
	{0x01, 0x5A, 0xA6}, {0x01, 0x5E, 0xAA}, {0x01, 0x62, 0xDE},
	{0x01, 0x70, 0xDB}, {0x01, 0x79, 0xAC}, {0x01, 0x7B, 0xAF},
	/* a< a, c' d/ e, l< l' l/ */
	{0x01, 0x03, 0xE3}, {0x01, 0x05, 0xB1}, {0x01, 0x07, 0xE6},
	{0x01, 0x11, 0xF0}, {0x01, 0x19, 0xEA}, {0x01, 0x3E, 0xB5},
	{0x01, 0x3A, 0xE5}, {0x01, 0x42, 0xB3},
	/* n' o" r' s' s, t, u" z' z. */
	{0x01, 0x44, 0xF1}, {0x01, 0x51, 0xF5}, {0x01, 0x55, 0xE0},
	{0x01, 0x5B, 0xB6}, {0x01, 0x5F, 0xBA}, {0x01, 0x63, 0xFE},
	{0x01, 0x71, 0xFB}, {0x01, 0x7A, 0xBC}, {0x01, 0x7C, 0xBF},
	{0x00, 0x00, 0x00}
};


/* Makes "normal" string from Unicode.
   Dest - where to save destination string,
   Src - where to get string from,
   Len - len of string AFTER converting */
static void UnicodeDecode (char* dest, const char* src, int len)
{
	int i, j;
	char ch;

	for ( i = 0;  i < len;  ++i )
	{
		ch = src[i*2+1];   /* default is to cut off the first byte */
		for ( j = 0;  unicode_table[j][2] != 0x00;  ++j )
			if ( src[i*2] == unicode_table[j][0] &&
			     src[i*2+1] == unicode_table[j][1] )
				ch = unicode_table[j][2];
		dest[i] = ch;
	}
	dest[len] = '\0';
}


/* Makes Unicode from "normal" string.
   Dest - where to save destination string,
   Src - where to get string from,
   Pixd - variable containing offset (index) of the first char inside Dest,
          it is updated on return (IN+OUT argument), must NOT be NULL */
static void UnicodeEncode (char* dest, int *pidx, const char* src)
{
	int j;
	char ch1, ch2;

	for ( ; *src != '\0';  ++src )
	{
		ch1 = 0x00;  ch2 = *src;
		for ( j = 0;  unicode_table[j][2] != 0x00;  ++j )
			if ( *src == unicode_table[j][2] )
			{
				ch1 = unicode_table[j][0];
				ch2 = unicode_table[j][1];
			}
		dest[(*pidx)++] = ch1;
		dest[(*pidx)++] = ch2;
	}
}



static bool P7110_SendRLPFrame(RLP_F96Frame *frame, bool out_dtx) 
{
  return false;
}


GSM_Error P7110_Initialise(char *port_device, char *initlength,
		 GSM_ConnectionType connection,
		 void (*rlp_callback)(RLP_F96Frame *frame))
{
  strncpy(link.PortDevice,port_device,20);
  link.InitLength=atoi(initlength);
  link.ConnectionType=connection;
  FBUS_Initialise(&link, &phone);

  /* Now do any phone specific init */

  return GE_NONE;
}


static GSM_Error P7110_GenericCRHandler(int messagetype, char *buffer, int length)
{
  return PGEN_CommandResponseReceive(&link, messagetype, buffer, length);
}


static GSM_Error P7110_GetIMEI(char *imei)
{
  unsigned char req[100] = {FBUS_FRAME_HEADER, 0x01};
  int len=4;

  if (PGEN_CommandResponse(&link,req,&len,0x1b,0x1b,100)==GE_NONE) {
    snprintf(imei,P7110_MAX_IMEI_LENGTH,"%s",req+4);
    return GE_NONE;
  }
  else return GE_NOTIMPLEMENTED;
}


static GSM_Error P7110_GetModel(char *model)
{
  unsigned char req[100] = {FBUS_FRAME_HEADER, 0x03, 0x01, 0x32};
  int len=6;

  if (PGEN_CommandResponse(&link,req,&len,0x1b,0x1b,100)==GE_NONE) {
    snprintf(model,P7110_MAX_MODEL_LENGTH,"%s",req+22);
    return GE_NONE;
  }
  else return GE_NOTIMPLEMENTED;
}


static GSM_Error P7110_DialVoice(char *Number)
{

#if 0  /* Doesn't work (yet) */
  unsigned char req[] = {FBUS_FRAME_HEADER, 0x01, 0x01, 0x01, 0x01, 0x05, 0x02 ,0x00,0x35,0x02, 0x35,0x35, 0x00};
  unsigned char req_end[]={0x05, 0x01, 0x01, 0x05, 0x81, 0x01, 0x00, 0x00, 0x01};
  int len=0, i;

  //req[4]=strlen(Number);

  //for(i=0; i < strlen(Number) ; i++)
  // req[5+i]=Number[i];

  //memcpy(req+5+strlen(Number), req_end, 10);

  //len=6+strlen(Number);

  len=sizeof(req);

  PGEN_DebugMessage(req,len);

  if (PGEN_CommandResponse(&link,req,&len,0x01,0x01,100)==GE_NONE) {
    PGEN_DebugMessage(req,len);
    return GE_NONE;
  }

  else return GE_NOTIMPLEMENTED;
#endif
  
  return GE_NOTIMPLEMENTED;
}


static GSM_Error GetCallerBitmap(GSM_Bitmap *bitmap)
{
  unsigned char req[500] = {FBUS_FRAME_HEADER, 0x07, 0x01, 0x01, 0x00, 0x01,
                         0x00, 0x10 , /* memory type */
                         0x00, 0x00,  /* location */
                         0x00, 0x00};
  int len=14;
  int blocks, i;
  unsigned char *blockstart;

  req[11]=bitmap->number+1;

  if (PGEN_CommandResponse(&link,req,&len,0x03,0x03,500)==GE_NONE) {

    if( req[6] == 0x0f ) // not found
      {
        if( req[10] == 0x34 ) // not found because inv. location
          return GE_INVALIDPHBOOKLOCATION;
        else return GE_NOTIMPLEMENTED;
      }

    bitmap->size=0;
    bitmap->height=0;
    bitmap->width=0;
    bitmap->text[0]=0;
    
    blocks     = req[17];        
    blockstart = req+18;
    
    for( i = 0; i < blocks; i++ ) {
      switch( blockstart[0] ) {
      case 0x07:                   /* Name */
	UnicodeDecode(bitmap->text,blockstart+6,blockstart[5]/2);
	break;
      case 0x0c:                   /* Ringtone */
	bitmap->ringtone=blockstart[5];
	break;
      case 0x1b:                   /* Bitmap */
	bitmap->width=blockstart[5];
	bitmap->height=blockstart[6];
	bitmap->size=(bitmap->width*bitmap->height)/8;
	memcpy(bitmap->bitmap,blockstart+10,bitmap->size);
	break;
      case 0x1c:                   /* Graphic on/off */
	break;
      case 0x1e:                   /* Number */
	break;
      default:
	fprintf(stdout, "Unknown caller logo block %02x\n\r",blockstart[0]);
	break;
      }
      blockstart+=blockstart[3];
    }
    return GE_NONE;
  }
  else return GE_NOTIMPLEMENTED;
}


inline unsigned char PackBlock(u8 id, u8 size, u8 no, u8 *buf, u8 *block)
{
  *(block++)=id;
  *(block++)=0;
  *(block++)=0;
  *(block++)=size+6;
  *(block++)=no;
  memcpy(block,buf,size);
  block+=size;
  *(block++)=0;

  return size+6;
}


static GSM_Error SetCallerBitmap(GSM_Bitmap *bitmap)
{
  unsigned char req[500] = {FBUS_FRAME_HEADER, 0x0b, 0x00, 0x01, 0x01, 0x00, 0x00, 0x0c,
                         0x00, 0x10,  /* memory type */
                         0x00, 0x00,  /* location */
                         0x00, 0x00, 0x00};
  char string[500];
  int block, i;
  unsigned int count=18;


  req[13]=bitmap->number+1;
  block=1;

  /* Name */
  i=0;
  UnicodeEncode(string+1, &i, bitmap->text);
  string[0]=i;
  count+=PackBlock(0x07,i+1,block++,string,req+count);
  
  /* Ringtone */
  string[0]=bitmap->ringtone;
  string[1]=0;
  count+=PackBlock(0x0c,2,block++,string,req+count);
  
  /* Number */
  string[0]=bitmap->number+1;
  string[1]=0;
  count+=PackBlock(0x1e,2,block++,string,req+count);

  /* Logo on/off - assume on for now */
  string[0]=1;
  string[1]=0;
  count+=PackBlock(0x1c,2,block++,string,req+count);

  /* Logo */
  string[0]=bitmap->width;
  string[1]=bitmap->height;
  string[2]=0;
  string[3]=0;
  string[4]=0x7e; /* Size */
  memcpy(string+5,bitmap->bitmap,bitmap->size);
  count+=PackBlock(0x1b,bitmap->size+5,block++,string,req+count);

  req[17]=block-1;

  if (PGEN_CommandResponse(&link,req,&count,0x03,0x03,100)!=GE_NONE)
    return GE_NOTIMPLEMENTED;

  return GE_NONE;
}


static GSM_Error GetStartupBitmap(GSM_Bitmap *bitmap)
{
  unsigned char req[1000] = {FBUS_FRAME_HEADER, 0xee, 0x15};
  int count=5;

  if (PGEN_CommandResponse(&link,req,&count,0x7a,0x7a,1000)!=GE_NONE)
    return GE_NOTIMPLEMENTED;

  //PGEN_DebugMessage(req,count);

  /* I'm sure there are blocks here but never mind! */

  bitmap->type=GSM_StartupLogo;
  bitmap->height=req[13];
  bitmap->width=req[17];
  bitmap->size=864; /* Can't see this coded anywhere */
  memcpy(bitmap->bitmap,req+22,bitmap->size);

  return GE_NONE;
}

static GSM_Error GetOperatorBitmap(GSM_Bitmap *bitmap)
{
  unsigned char req[500] = {FBUS_FRAME_HEADER, 0x70};
  int count=4, i;
  unsigned char *blockstart;

  if (PGEN_CommandResponse(&link,req,&count,0x0a,0x0a,500)!=GE_NONE)
    return GE_NOTIMPLEMENTED;

  PGEN_DebugMessage(req,count);
 
  blockstart=req+6;

  for (i=0;i<req[4];i++) {
    switch (blockstart[0]) {
    case 0x01:  /* Operator details */
      /* Network code is stored as 0xBA 0xXC 0xED ("ABC DE"). */
      bitmap->netcode[0]='0' + (blockstart[8] & 0x0f);
      bitmap->netcode[1]='0' + (blockstart[8] >> 4);
      bitmap->netcode[2]='0' + (blockstart[9] & 0x0f);
      bitmap->netcode[3]=' ';
      bitmap->netcode[4]='0' + (blockstart[10] & 0x0f);
      bitmap->netcode[5]='0' + (blockstart[10] >> 4);
      bitmap->netcode[6]=0;
      break;
    case 0x04: /* Logo */
      bitmap->type=GSM_OperatorLogo;
      bitmap->size=blockstart[5]; /* Probably + [4]<<8 */
      bitmap->height=blockstart[3];
      bitmap->width=blockstart[2];
      memcpy(bitmap->bitmap,blockstart+8,bitmap->size);
      break;
    default:
      dprintf("Unknown operator block %d\n",blockstart[0]);
      break;
    }
    blockstart+=blockstart[1];
  }
  return GE_NONE;
}

static GSM_Error SetStartupBitmap(GSM_Bitmap *bitmap)
{
  unsigned char req[1000] = {FBUS_FRAME_HEADER, 0xec, 0x15, 0x00, 0x00, 0x00, 0x04, 0xc0, 0x02, 0x00,
			     0x00,           /* Height */
			     0xc0, 0x03, 0x00,
			     0x00,           /* Width */
			     0xc0, 0x04, 0x03, 0x00 };
  int count=21;

  req[12]=bitmap->height;
  req[16]=bitmap->width;
  memcpy(req+count,bitmap->bitmap,bitmap->size);
  count+=bitmap->size;

  if (PGEN_CommandResponse(&link,req,&count,0x7a,0x7a,1000)!=GE_NONE)
    return GE_NOTIMPLEMENTED;

  PGEN_DebugMessage(req,count);

  return GE_NONE;
}

static GSM_Error SetOperatorBitmap(GSM_Bitmap *bitmap)
{
  unsigned char req[500] = { FBUS_FRAME_HEADER, 0xa3, 0x01,
			     0x00,              /* logo enabled */
			     0x00, 0xf0, 0x00,  /* network code (000 00) */
			     0x00 ,0x04,
			     0x08,              /* length of rest */
			     0x00, 0x00,        /* Bitmap width / height */
			     0x00,
			     0x00,              /* Bitmap size */
			     0x00, 0x00
  };    
  int count=18;

  if (strcmp(bitmap->netcode,"000 00")) {  /* set logo */
      req[5] = 0x01;      // Logo enabled
      req[6] = ((bitmap->netcode[1] & 0x0f)<<4) | (bitmap->netcode[0] & 0x0f);
      req[7] = 0xf0 | (bitmap->netcode[2] & 0x0f);
      req[8] = ((bitmap->netcode[5] & 0x0f)<<4) | (bitmap->netcode[4] & 0x0f);
      req[11] = 8+bitmap->size;
      req[12]=bitmap->width;
      req[13]=bitmap->height;
      req[15]=bitmap->size;
      memcpy(req+count,bitmap->bitmap,bitmap->size);
      count += bitmap->size;
  }

  if (PGEN_CommandResponse(&link,req,&count,0x0a,0x0a,500)!=GE_NONE)
    return GE_NOTIMPLEMENTED;

  if (req[3]!=0xa4) return GE_UNKNOWN;

  return GE_NONE;
}

static GSM_Error P7110_GetBitmap(GSM_Bitmap *bitmap)
{
  switch(bitmap->type) {
  case GSM_CallerLogo:
    return GetCallerBitmap(bitmap);
    break;
  case GSM_StartupLogo:
    return GetStartupBitmap(bitmap);
    break;
  case GSM_OperatorLogo:
    return GetOperatorBitmap(bitmap);
    break;
  default:
    break;
  }
  return GE_NOTIMPLEMENTED;
}


static GSM_Error P7110_SetBitmap(GSM_Bitmap *bitmap)
{
  switch(bitmap->type) {
  case GSM_CallerLogo:
    return SetCallerBitmap(bitmap);
    break;
  case GSM_StartupLogo:
    return SetStartupBitmap(bitmap);
    break;
  case GSM_OperatorLogo:
    return SetOperatorBitmap(bitmap);
    break;
  default:
    break;
  }
  return GE_NOTIMPLEMENTED;
}


static GSM_Error P7110_GetRevision(char *revision)
{
  unsigned char req[100] = {FBUS_FRAME_HEADER, 0x03, 0x01, 0x32};
  int len=6;

  if (PGEN_CommandResponse(&link,req,&len,0x1b,0x1b,100)==GE_NONE) {
    snprintf(revision,P7110_MAX_REVISION_LENGTH,"%s",req+7);
    return GE_NONE;
  }
  else return GE_NOTIMPLEMENTED;
}


static GSM_Error P7110_GetBatteryLevel(GSM_BatteryUnits *units, float *level)
{
  unsigned char req[100] = {FBUS_FRAME_HEADER, 0x02};
  int len=4;

  if (PGEN_CommandResponse(&link,req,&len,0x17,0x17,100)==GE_NONE) {
    *units=GBU_Percentage;
    *level=req[5];
    return GE_NONE;
  }
  else return GE_NOTIMPLEMENTED;
}


static GSM_Error P7110_GetRFLevel(GSM_RFUnits *units, float *level)
{
  unsigned char req[100] = {FBUS_FRAME_HEADER, 0x81};
  int len=4;

  if (PGEN_CommandResponse(&link,req,&len,0x0a,0x0a,100)==GE_NONE) {
    *units=GRF_Percentage;
    *level=req[4];
    return GE_NONE;
  }
  else return GE_NOTIMPLEMENTED;
}


static GSM_Error P7110_GetMemoryStatus(GSM_MemoryStatus *status)
{
  unsigned char req[100] = {FBUS_FRAME_HEADER, 0x03, 0x00, 0x00};
  int len=6;

  req[5] = GetMemoryType(status->MemoryType);

  if (PGEN_CommandResponse(&link,req,&len,0x03,0x03,100)==GE_NONE) {
    if (req[5]!=0xff) {
      status->Used=(req[16]<<8)+req[17];
      status->Free=((req[14]<<8)+req[15])-status->Used;
      dprintf("Memory status - location = %d\n\r",(req[8]<<8)+req[9]);
      return GE_NONE;
    } else return GE_NOTIMPLEMENTED;
  }
  else return GE_NOTIMPLEMENTED;
}



static int GetMemoryType(GSM_MemoryType memory_type)
{

  int result;

  switch (memory_type) {
     case GMT_MT:
	result = P7110_MEMORY_MT;
        break;
     case GMT_ME:
	result = P7110_MEMORY_ME;
        break;
     case GMT_SM:
	result = P7110_MEMORY_SM;
        break;
     case GMT_FD:
	result = P7110_MEMORY_FD;
        break;
     case GMT_ON:
	result = P7110_MEMORY_ON;
        break;
     case GMT_EN:
	result = P7110_MEMORY_EN;
        break;
     case GMT_DC:
	result = P7110_MEMORY_DC;
        break;
     case GMT_RC:
	result = P7110_MEMORY_RC;
        break;
     case GMT_MC:
	result = P7110_MEMORY_MC;
        break;
     default:
        result=P7110_MEMORY_XX;
   }
   return (result);
}


static GSM_Error P7110_WritePhonebookLocation(GSM_PhonebookEntry *entry)
{
  unsigned char req[500] = {FBUS_FRAME_HEADER, 0x0b, 0x00, 0x01, 0x01, 0x00, 0x00, 0x0c,
                         0x00, 0x00,  /* memory type */
                         0x00, 0x00,  /* location */
                         0x00, 0x00, 0x00};
  char string[500];
  int block, i, j, defaultn;
  unsigned int count=18;

  req[11] = GetMemoryType(entry->MemoryType);
  req[12] = (entry->Location>>8);
  req[13] = entry->Location & 0xff;

  block=1;

  if (strlen(entry->Name)>0) {

  /* Name */
  i=0;
  UnicodeEncode(string+1, &i, entry->Name);
  string[0]=i;
  count+=PackBlock(0x07,i+1,block++,string,req+count);
  
  /* Group */
  string[0]=entry->Group+1;
  string[1]=0;
  count+=PackBlock(0x1e,2,block++,string,req+count);
  
  /* Default Number */

  defaultn=999;
  for (i=0;i<entry->SubEntriesCount;i++)
    if (entry->SubEntries[i].EntryType==GSM_Number)
      if (strcmp(entry->Number,entry->SubEntries[i].data.Number)==0)
	defaultn=i;

  if (defaultn<i) {
    string[0]=entry->SubEntries[defaultn].NumberType;
    string[1]=0;
    string[2]=0;
    string[3]=0;
    i=0;
    UnicodeEncode(string+5, &i,entry->SubEntries[defaultn].data.Number);
    string[4]=i;
    count+=PackBlock(0x0b,i+5,block++,string,req+count);
  }

  /* Rest of the numbers */

  for (i=0;i<entry->SubEntriesCount;i++)
    if (entry->SubEntries[i].EntryType==GSM_Number)
      if (i!=defaultn) {
	string[0]=entry->SubEntries[i].NumberType;
	string[1]=0;
	string[2]=0;
	string[3]=0;
	j=0;
	UnicodeEncode(string+5, &j,entry->SubEntries[i].data.Number);
	string[4]=j;
	count+=PackBlock(0x0b,j+5,block++,string,req+count);
      } 

  req[17]=block-1;

  if (PGEN_CommandResponse(&link,req,&count,0x03,0x03,500)!=GE_NONE)
    return GE_NOTIMPLEMENTED;

  }

  return GE_NONE;
}



static GSM_Error P7110_ReadPhonebook(GSM_PhonebookEntry *entry)
{
  unsigned char req[2000] = {FBUS_FRAME_HEADER, 0x07, 0x01, 0x01, 0x00, 0x01,
                         0x00, 0x00 , /* memory type */ //02,05
                         0x00, 0x00,  /* location */
                         0x00, 0x00};
  int len=14;
  int blocks, blockcount, i;
  unsigned char *pBlock;

  req[9] = GetMemoryType(entry->MemoryType);
  req[10] = (entry->Location>>8);
  req[11] = entry->Location & 0xff;

  if (PGEN_CommandResponse(&link,req,&len,0x03,0x03,2000)==GE_NONE) {

    entry->Empty = true;
    entry->Group = 0;
    entry->Name[0] = '\0';
    entry->Number[0] = '\0';
    entry->SubEntriesCount = 0;
    
    entry->Date.Year = 0;
    entry->Date.Month = 0;
    entry->Date.Day = 0;
    entry->Date.Hour = 0;
    entry->Date.Minute = 0;
    entry->Date.Second = 0;
    
    if( req[6] == 0x0f ) // not found
      {
        if( req[10] == 0x34 ) // not found because inv. location
          return GE_INVALIDPHBOOKLOCATION;
        else return GE_NONE;
      }
    
    blocks     = req[17];
    blockcount = 0;
    
    entry->SubEntriesCount = blocks - 1;

      dprintf(_("Message: Phonebook entry received:\n"));
      dprintf(_(" Blocks: %d\n"),blocks);
        
    pBlock = req+18;
 
    for( i = 0; i < blocks; i++ )
      {
        GSM_SubPhonebookEntry* pEntry = &entry->SubEntries[blockcount];
	
        switch( pBlock[0] )
	  {
	  case P7110_ENTRYTYPE_NAME:
	    UnicodeDecode (entry->Name, pBlock+6, pBlock[5]/2);
	    entry->Empty = false;
	    dprintf(_("   Name: %s\n"), entry->Name);
	    break;
	  case P7110_ENTRYTYPE_NUMBER:
	    pEntry->EntryType   = pBlock[0];
	    pEntry->NumberType  = pBlock[5];
	    pEntry->BlockNumber = pBlock[4];
	    
	    UnicodeDecode (pEntry->data.Number, pBlock+10, pBlock[9]/2);

	    if( blockcount == 0 )
	      strcpy( entry->Number, pEntry->data.Number );
	    dprintf("   Type: %d (%02x)\n",
		    pEntry->NumberType,
		    pEntry->NumberType);
	    dprintf(" Number: %s\n",
		    pEntry->data.Number);
	    blockcount++;
	    break;
	  case P7110_ENTRYTYPE_DATE:
	    pEntry->EntryType        = pBlock[0];
	    pEntry->NumberType       = pBlock[5];
	    pEntry->BlockNumber      = pBlock[4];
	    pEntry->data.Date.Year   = (pBlock[6]<<8) + pBlock[7];
	    pEntry->data.Date.Month  = pBlock[8];
	    pEntry->data.Date.Day    = pBlock[9];
	    pEntry->data.Date.Hour   = pBlock[10];
	    pEntry->data.Date.Minute = pBlock[11];
	    pEntry->data.Date.Second = pBlock[12];
	    dprintf("   Date: %02u.%02u.%04u\n", pEntry->data.Date.Day,
		    pEntry->data.Date.Month, pEntry->data.Date.Year );
	    dprintf("   Time: %02u:%02u:%02u\n", pEntry->data.Date.Hour,
		    pEntry->data.Date.Minute, pEntry->data.Date.Second);
	    blockcount++;
	    break;
	  case P7110_ENTRYTYPE_NOTE:
	  case P7110_ENTRYTYPE_POSTAL:
	  case P7110_ENTRYTYPE_EMAIL:
	    pEntry->EntryType   = pBlock[0];
	    pEntry->NumberType  = 0;
	    pEntry->BlockNumber = pBlock[4];
	    
	    UnicodeDecode (pEntry->data.Number, pBlock+6, pBlock[5]/2);
	    
	    dprintf("   Type: %d (%02x)\n",
		    pEntry->EntryType,
		    pEntry->EntryType);
	    dprintf("   Text: %s\n",
	            pEntry->data.Number);
	    blockcount++;
	    break;
	  case P7110_ENTRYTYPE_GROUP:
	    entry->Group = pBlock[5]-1;
	    dprintf(_("   Group: %d\n"), entry->Group);
	    break;
	  default:
	    dprintf(_("Unknown Entry Code (%u) received.\n"), pBlock[0] );
	    break;
	  }

        pBlock+=pBlock[3];
      }
    
    entry->SubEntriesCount = blockcount;    
    
    return GE_NONE;
  }
  else return GE_NOTIMPLEMENTED;
}


