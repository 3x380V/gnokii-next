/*

  $Id: lowlevel.c,v 1.51 2008-02-04 22:44:54 pkot Exp $

  S M S D

  A Linux/Unix tool for the mobile phones.

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

  Copyright (C) 1999      Pavel Jan�k ml., Hugh Blemings
  Copyright (C) 1999-2004 Jan Derfinak

*/

#include "config.h"
#include <pthread.h>
#include <string.h>
#include <glib.h>
#include "misc.h"
#include "gnokii.h"
#include "compat.h"
#include "smsd.h"
#include "lowlevel.h"

pthread_t monitor_th;
PhoneMonitor phoneMonitor;
pthread_mutex_t smsMutex;
pthread_cond_t  smsCond;
pthread_mutex_t sendSMSMutex;
pthread_cond_t  sendSMSCond;
static pthread_mutex_t eventsMutex;
static GSList *ScheduledEvents = NULL;
static struct gn_statemachine *sm;


inline void InsertEvent (PhoneEvent *event)
{
  gn_log_xdebug ("Inserting Event: %d\n", event->event);
  pthread_mutex_lock (&eventsMutex);
  ScheduledEvents = g_slist_prepend (ScheduledEvents, event);
  pthread_mutex_unlock (&eventsMutex);
}


inline static PhoneEvent *RemoveEvent (void)
{
  GSList *list;
  PhoneEvent *event = NULL;

  pthread_mutex_lock (&eventsMutex);
  list = g_slist_last (ScheduledEvents);
  if (list)
  {
    event = (PhoneEvent *) list->data;
    ScheduledEvents = g_slist_remove_link (ScheduledEvents, list);
    g_slist_free_1 (list);
  }
  pthread_mutex_unlock (&eventsMutex);

  return (event);
}


static gn_error InitModelInf (void)
{
  gn_data data;
  gn_error error;
  char *aux;
  char model[GN_MODEL_MAX_LENGTH], rev[GN_REVISION_MAX_LENGTH], 
	manufacturer[GN_MANUFACTURER_MAX_LENGTH];

  data.manufacturer = manufacturer;
  data.model = model;
  data.revision = rev;
                          
  error = gn_sm_functions (GN_OP_GetModel, &data, sm);
  if (error != GN_ERR_NONE)
  {
    return error;
  }
    
  g_free (phoneMonitor.phone.version);
  phoneMonitor.phone.version = g_strdup (model);

  g_free (phoneMonitor.phone.model);
  aux = (char *)gn_phone_model_get(model)->model;
  if (aux == NULL)
    phoneMonitor.phone.model = g_strdup (_("unknown"));
  else
    phoneMonitor.phone.model = g_strdup (aux);

  phoneMonitor.supported = gn_phone_model_get (model)->flags;

  g_free (phoneMonitor.phone.revision);
  phoneMonitor.phone.revision = g_strdup (rev);

  gn_log_xdebug ("Version: %s\n", phoneMonitor.phone.version);
  gn_log_xdebug ("Model: %s\n", phoneMonitor.phone.model);
  gn_log_xdebug ("Revision: %s\n", phoneMonitor.phone.revision);

  return GN_ERR_NONE;
}

static void busterminate (void)
{
  if (sm) {
    gn_lib_phone_close(sm);
    gn_lib_phoneprofile_free(&sm);
  }
  gn_lib_library_free();
}

static gn_error fbusinit (const char * const iname)
{
  gn_error error;

  error = gn_lib_phoneprofile_load(iname, &sm);
  if (error != GN_ERR_NONE)
  {
    g_print (_("Cannot load phone %s!\nDo you have proper section in gnokiirc?\n"), iname);
    g_print (_("%s\n"), gn_error_print(error));
    exit (-1);
  }

  /* register cleanup function */
  atexit(busterminate);

  /* Initialise the code for the GSM interface. */     
  error = gn_lib_phone_open(sm);
  gn_log_xdebug ("fbusinit: error %d\n", error);
  if (error != GN_ERR_NONE)
  {
    g_print (_("GSM/FBUS init failed! (Unknown model?). Quitting.\n"));
    g_print (_("%s\n"), gn_error_print(error));
    return (error);
  }

  /* Why GSM_Initialise returns before initialization is completed?
     We must wait cca 2 seconds. If smsd segfaults during initialiazation
     try to increase number of seconds.
  */
  sleep (2);

  return InitModelInf ();
}


void InitPhoneMonitor (void)
{
  phoneMonitor.phone.model = g_strdup (_("unknown"));
  phoneMonitor.phone.version = g_strdup (_("unknown"));
  phoneMonitor.phone.revision = g_strdup (_("unknown"));
  phoneMonitor.supported = 0;
//  phoneMonitor.working = FALSE;
//  phoneMonitor.sms.unRead = 0;
  phoneMonitor.sms.number = 0;
  phoneMonitor.sms.messages = NULL;
  pthread_mutex_init (&smsMutex, NULL);
  pthread_cond_init (&smsCond, NULL);
  pthread_mutex_init (&sendSMSMutex, NULL);
  pthread_cond_init (&sendSMSCond, NULL);
  pthread_mutex_init (&eventsMutex, NULL);
}


static inline void FreeElement (gpointer data, gpointer userData)
{
  g_free ((gn_sms *) data);
}


static inline void FreeArray (GSList **array)
{
  if (*array)
  {
    g_slist_foreach (*array, FreeElement, NULL);
    g_slist_free (*array);
    *array = NULL;
  }
}


static void RefreshSMS (const gint number)
{
  static gn_data data;
  gn_error error;
  gn_sms *msg;
  static gn_sms_folder folder;
  static gn_sms_folder_list folderlist;
  register gint i;
  

  if (number < 1)
  {
    gn_log_xdebug ("RefreshSMS is started with number of sms less than 1.\n\
This should not happen.\nSkiping.");
    return;
  }
  
  gn_log_xdebug ("RefreshSMS is running...\nNumber of messages: %d\n", number);

  FreeArray (&(phoneMonitor.sms.messages));
  phoneMonitor.sms.number = 0;

  gn_data_clear(&data);
  data.sms_folder_list = &folderlist;
  folder.folder_id = 0;
  data.sms_folder = &folder;

  if (smsdConfig.firstSMS < 0)
  {
    gn_log_xdebug ("Detecting first SMS position.\n");
    
    msg = g_malloc (sizeof (gn_sms));
    memset (msg, 0, sizeof (gn_sms));
    msg->memory_type = smsdConfig.memoryType;
    msg->number = 0;
    data.sms = msg;
    
    if ((error = gn_sms_get (&data, sm)) == GN_ERR_INVALIDLOCATION)
      smsdConfig.firstSMS = 1;
    else
      smsdConfig.firstSMS = 0;
      
    gn_log_xdebug ("First SMS position is %d\n", smsdConfig.firstSMS);
    
    g_free (msg);
  }
    
  i = smsdConfig.firstSMS;
  while (1)
  {
    msg = g_malloc (sizeof (gn_sms));
    memset (msg, 0, sizeof (gn_sms));
    msg->memory_type = smsdConfig.memoryType;
    msg->number = i++;
    data.sms = msg;
    
    if ((error = gn_sms_get (&data, sm)) == GN_ERR_NONE)
    {
      phoneMonitor.sms.messages = g_slist_append (phoneMonitor.sms.messages, msg);
      phoneMonitor.sms.number++;
      
      if (phoneMonitor.sms.number == number)
      {
        pthread_cond_signal (&smsCond);
        return;
      }
    }
    else if (error == GN_ERR_INVALIDLOCATION)   /* All positions are readed */
    {
      g_free (msg);
      pthread_cond_signal (&smsCond);
      break;
    }
    else
    {  
      g_free (msg);
      gn_log_xdebug ("gn_sms_get returned %d\n", error);
    }

    usleep (500000);
  }
}


static gint A_SendSMSMessage (gpointer data)
{
  D_SMSMessage *d = (D_SMSMessage *) data;
  gn_data *dt;

  if (!d)
    return (GN_ERR_UNKNOWN);
    
  pthread_mutex_lock (&sendSMSMutex);
  dt = calloc (1, sizeof (gn_data));
  if (!d->sms->smsc.number[0])
  {
    dt->message_center = calloc (1, sizeof (gn_sms_message_center));
    dt->message_center->id = 1;
    if (gn_sm_functions (GN_OP_GetSMSCenter, dt, sm) == GN_ERR_NONE)
    {
      strcpy (d->sms->smsc.number, dt->message_center->smsc.number);
      d->sms->smsc.type = dt->message_center->smsc.type;
    }
    free (dt->message_center);
  }
  
  if (!d->sms->smsc.type)
    d->sms->smsc.type = GN_GSM_NUMBER_Unknown;
    
  gn_data_clear (dt);
  dt->sms = d->sms;
  d->status = gn_sms_send (dt, sm);
  free (dt);
  pthread_cond_signal (&sendSMSCond);
  pthread_mutex_unlock (&sendSMSMutex);

  return (d->status);
}

/*
 * Argument passed here should be gn_sms structure.
 * Required fields for sms deletion are:
 *   - gn_sms.number
 *   - gn_sms.memory_type
 */
static gint A_DeleteSMSMessage (gpointer data)
{
  gn_data *dt;
  gn_error error = GN_ERR_UNKNOWN;
  gn_sms_folder SMSFolder;
  gn_sms_folder_list SMSFolderList;

  dt = calloc (1, sizeof (gn_data));
  dt->sms = (gn_sms *) data;
  SMSFolder.folder_id = 0;
  dt->sms_folder = &SMSFolder;
  dt->sms_folder_list = &SMSFolderList;
  if (dt->sms)
  {
    gn_log_xdebug("Deleting SMS %d\n", dt->sms->number);
    /* FIXME: error handling */
    error = gn_sms_delete (dt, sm);

    gn_log_xdebug("Error: %s\n", gn_error_print(error));
    pthread_mutex_lock (&smsMutex);
    FreeElement (data, NULL);
    phoneMonitor.sms.messages = g_slist_remove (phoneMonitor.sms.messages, data);
    phoneMonitor.sms.number--;
    pthread_mutex_unlock (&smsMutex);
  } else {
    gn_log_xdebug("Internal error: dt->sms == NULL\n");
  }

  free (dt);
  return (error);
}


static gint A_Exit (gpointer data)
{
  pthread_exit (0);
  return (0); /* just to be proper */
}


gint (*DoAction[])(gpointer) = {
  A_SendSMSMessage,
  A_DeleteSMSMessage,
  A_Exit
};


static void RealConnect (void *phone)
{
  gn_data *data;
  gn_sms_status SMSStatus = {0, 0, 0, 0};
  gn_sms_folder SMSFolder;
  PhoneEvent *event;
  gn_error error;

  data = calloc (1, sizeof (gn_data));
  
  gn_log_xdebug ("Initializing connection...\n");

  if (fbusinit ((gchar *)phone) != GN_ERR_NONE)
  {
    free (data);
    exit (1);
  }

  if (smsdConfig.memoryType == GN_MT_XX) {
    if (phoneMonitor.supported & PM_FOLDERS)
      smsdConfig.memoryType = GN_MT_IN;
    else
      smsdConfig.memoryType = GN_MT_SM;
  }

  gn_log_xdebug ("Phone connected. Starting monitoring...\n");

  while (1)
  {
//    phoneMonitor.working = FALSE;

    pthread_mutex_lock (&smsMutex);
    
    if (phoneMonitor.supported & PM_FOLDERS)
    {
      data->sms_folder = &SMSFolder;
      SMSFolder.folder_id = smsdConfig.memoryType;
      if ((error = gn_sm_functions (GN_OP_GetSMSFolderStatus, data, sm)) == GN_ERR_NONE)
      {
        gn_log_xdebug ("GN_OP_GetSMSFolderStatus returned  %d\n",
                       SMSFolder.number);
        gn_log_xdebug ("phoneMonitor.sms.number %d\n",
                       phoneMonitor.sms.number);
        if (phoneMonitor.sms.number != SMSFolder.number)
        {
//          phoneMonitor.working = TRUE;
          RefreshSMS (SMSFolder.number);
//          phoneMonitor.working = FALSE;
        }
//        phoneMonitor.sms.unRead = 0;
      }
    }
    else
    {
      gn_memory_status dummy;
      dummy.memory_type = smsdConfig.memoryType;

      data->sms_status = &SMSStatus;
      data->memory_status = &dummy;
      if ((error = gn_sm_functions (GN_OP_GetSMSStatus, data, sm)) == GN_ERR_NONE)
      {
        gn_log_xdebug ("GN_OP_GetSMSStatus returned (number, unread) %d, %d\n",
                       SMSStatus.number, SMSStatus.unread);
        gn_log_xdebug ("phoneMonitor %d\n", phoneMonitor.sms.number);
        if (/* phoneMonitor.sms.unRead != SMSStatus.unread || */
            phoneMonitor.sms.number != SMSStatus.number)
        {
//          phoneMonitor.working = TRUE;
          RefreshSMS (SMSStatus.number);
//          phoneMonitor.working = FALSE;
        }
//        phoneMonitor.sms.unRead = SMSStatus.unread;
      }
    }
    
    pthread_mutex_unlock (&smsMutex);
    
    if (error != GN_ERR_NONE)
    {
      if (error == GN_ERR_TIMEOUT)
      {
        g_print (_("Timeout in file %s, line %d, restarting connection.\n"),
                 __FILE__, __LINE__);
        break;
      }
      else
        g_print ("%s:%d error: %d, %s\n", __FILE__, __LINE__, error, gn_error_print(error));
    }

    while ((event = RemoveEvent ()) != NULL)
    {
      gn_log_xdebug ("Processing Event: %d\n", event->event);
//      phoneMonitor.working = TRUE;
      if (event->event <= Event_Exit)
        if ((error = DoAction[event->event] (event->data)) != GN_ERR_NONE)
          g_print (_("Event %d failed with return code %d!\n"), event->event, error);
      g_free (event);
    }
    
    sleep (smsdConfig.refreshInt);
  }

  free (data);
}


void *Connect (void *phone)
{
  while (1)
  {
    RealConnect (phone);
    busterminate ();
    sleep (1);
  }
} 
