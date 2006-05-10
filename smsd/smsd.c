/*

  $Id: smsd.c,v 1.50 2006-05-10 12:33:30 ja Exp $

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

  Copyright (C) 1999 Pavel Jan�k ml., Hugh Blemings
  Copyright (C) 1999-2005 Jan Derfinak

*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <time.h>
#include <dlfcn.h>

#ifndef WIN32
# include <unistd.h>  /* for usleep */
# include <signal.h>
#else
# include <windows.h>
# include "../win32/winserial.h"
# define WRITEPHONE(a, b, c) WriteCommBlock(b, c)
# undef IN
# undef OUT
#endif

#include <glib.h>

#include "misc.h"

#include "gnokii.h"
#include "smsd.h"
#include "lowlevel.h"
#include "db.h"


/* Hold main configuration data for smsd */
SmsdConfig smsdConfig;

/* Global variables */
bool GTerminateThread;

/* Local variables */
static DBConfig connection;
void (*DB_Bye) (void) = NULL;;
gint (*DB_ConnectInbox) (const DBConfig) = NULL;
gint (*DB_ConnectOutbox) (const DBConfig) = NULL;
gint (*DB_InsertSMS) (const gn_sms * const, const gchar * const) = NULL;
void (*DB_Look) (const gchar * const) = NULL;

static pthread_t db_monitor_th;
pthread_mutex_t db_monitorMutex;
static volatile bool db_monitor;

/* Escapes ' and \ with \. */
/* Returned value needs to be free with g_free(). */
gchar *strEscape (const gchar *const s)
{
  GString *str = g_string_new (s);
  register gint i = 0;
  gchar *ret;
  
  while (str->str[i] != '\0')
  {
    if (str->str[i] == '\\' || str->str[i] == '\'')
      g_string_insert_c (str, i++, '\\');
    i++;
  }
  
  ret = str->str;
  g_string_free (str, FALSE);
  
  return (ret);
}


gint LoadDB (void)
{
  void *handle;
  GString *buf;
  gchar *error;
  
  buf = g_string_sized_new (64);
  
  g_string_sprintf (buf, "%s/lib%s.so", smsdConfig.libDir, smsdConfig.dbMod);

  gn_log_xdebug ("Trying to load module %s\n", buf->str);
    
  handle = dlopen (buf->str, RTLD_LAZY);
  if (!handle)
  {
    g_print ("dlopen error: %s!\n", dlerror());
    return (1);
  }
    
  DB_Bye = dlsym(handle, "DB_Bye");
  if ((error = dlerror ()) != NULL)
  {
    g_print (error);
    return (2);
  }

  DB_ConnectInbox = dlsym(handle, "DB_ConnectInbox");
  if ((error = dlerror ()) != NULL)
  {
    g_print (error);
    return (2);
  }

  DB_ConnectOutbox = dlsym(handle, "DB_ConnectOutbox");
  if ((error = dlerror ()) != NULL)
  {
    g_print (error);
    return (2);
  }
  
  DB_InsertSMS = dlsym(handle, "DB_InsertSMS");
  if ((error = dlerror ()) != NULL)
  {
    g_print (error);
    return (2);
  }
  
  DB_Look = dlsym(handle, "DB_Look");
  if ((error = dlerror ()) != NULL)
  {
    g_print (error);
    return (2);
  }
  
  return (0);
}


static void Version (void)
{
  g_print ("\nsmsd - version 1.4-cvs		(20041011)\nCopyright  Jan Derfinak  <ja@mail.upjs.sk>\n");
}


static void Usage (gchar *p)
{
  Version ();
  g_print (_("\nUsage:  %s [options]\n"
             "            -u, --user db_username OR action if -m file\n"
             "            -p, --password db_password\n"
             "            -d, --db db_name\n"
             "            -c, --host db_hostname OR spool directory if -m file\n"
             "            -s, --schema db_schema\n"
             "            -m, --module db_module (pq, mysql, file)\n"
             "            -l, --libdir path_to_db_module\n"
             "            -f, --logfile file\n"
             "            -t, --phone phone_number\n"
             "            -i, --interval polling_interval_for_incoming_sms's_in_seconds\n"
             "            -S, --maxsms number_of_sms's (only in dumb mode)\n"
             "            -v, --version\n"
             "            -h, --help\n"), p);
}


static void LogFile (gchar *str, ...)
{
  FILE *f;
  va_list ap;
  time_t cas;
  gchar buf[50];
  
  if (smsdConfig.logFile == NULL)
    return;

  if (strcmp (smsdConfig.logFile, "-") == 0)
    f = stdout;
  else if ((f = fopen (smsdConfig.logFile, "a")) == NULL)
  {
    g_print (_("WARNING: Cannot open file %s for appending.\n"), smsdConfig.logFile);
    return;
  }

  cas = time (NULL);
  strftime (buf, 50, "%e %b %Y %T", localtime (&cas));
  fprintf (f, "%s: ", buf);

  va_start (ap, str);
  vfprintf (f, str, ap);
  va_end (ap);
  if (f == stdout)
    fflush (f);
  else
    fclose (f);
  return;
}


static void ReadConfig (gint argc, gchar *argv[])
{
  connection.user = g_strdup ("");
  connection.password = g_strdup ("");
  connection.db = g_strdup ("sms");
  connection.host = g_strdup ("");
  connection.schema = g_strdup ("public");
  smsdConfig.dbMod = g_strdup ("pq");
  smsdConfig.libDir = g_strdup (MODULES_DIR);
  smsdConfig.logFile = NULL;
  smsdConfig.phone = g_strdup ("");
  smsdConfig.refreshInt = 1;     // Phone querying interval in seconds
  smsdConfig.maxSMS = 10;        // Smsd uses it if GetSMSStatus isn't implemented
  smsdConfig.smsSets = 0;
  smsdConfig.memoryType = GN_MT_XX;

  while (1)
  {
    gint optionIndex = 0;
    gint c;
    static struct option longOptions[] =
    {
      {"user", 1, 0, 'u'},
      {"password", 1, 0, 'p'},
      {"db", 1, 0, 'd'},
      {"host", 1, 0, 'c'},
      {"schema", 1, 0, 's'},
      {"module", 1, 0, 'm'},
      {"libdir", 1, 0, 'l'},
      {"logfile", 1, 0, 'f'},
      {"phone", 1, 0, 't'},
      {"version", 0, 0, 'v'},
      {"interval", 1, 0, 'i'},
      {"maxsms", 1, 0, 'S'},
      {"inbox", 1, 0, 'b'},
      {"help", 0, 0, 'h'},
      {0, 0, 0, 0}
    };
    
    c = getopt_long (argc, argv, "u:p:d:c:s:m:l:f:t:vi:S:b:h", longOptions, &optionIndex);
    if (c == EOF)
      break;
    switch (c)
    {
      case 'u':
        g_free (connection.user);
        connection.user = g_strdup (optarg);
        memset (optarg, 'x', strlen (optarg));
        break;
      
      case 'p':
        g_free (connection.password);
        connection.password = g_strdup (optarg);
        memset (optarg, 'x', strlen (optarg));
        break;
        
      case 'd':
        g_free (connection.db);
        connection.db = g_strdup (optarg);
        memset (optarg, 'x', strlen (optarg));
        break;
        
      case 'c':
        g_free (connection.host);
        connection.host = g_strdup (optarg);
        memset (optarg, 'x', strlen (optarg));
        break;
      
      case 's':
        g_free (connection.schema);
        connection.schema = g_strdup (optarg);
        memset (optarg, 'x', strlen (optarg));
        break;
        
      case 'm':
        g_free (smsdConfig.dbMod);
        smsdConfig.dbMod = g_strdup (optarg);
        break;
        
      case 'l':
        g_free (smsdConfig.libDir);
        smsdConfig.libDir = g_strdup (optarg);
        break;

      case 'f':
        if (smsdConfig.logFile)
          g_free (smsdConfig.logFile);
        smsdConfig.logFile = g_strdup (optarg);
        break;

      case 't':
        if (smsdConfig.phone)
          g_free (smsdConfig.phone);
        smsdConfig.phone = g_strdup (optarg);
        break;

      case 'i':
        smsdConfig.refreshInt = atoi (optarg);
        break;

      case 'S':
        smsdConfig.maxSMS = atoi (optarg);
        break;

      case 'b':
        smsdConfig.memoryType = gn_str2memory_type (optarg);
        break;

      case 'v':
        Version ();
        exit (0);
        break;

      case 'h':
      case '?':
        Usage (argv[0]);
        exit (1);
        break;

      default:
        g_print ("getopt returned 0%o\n", c);
    }
  }
  
  if ((argc - optind) != 0)
  {
    g_print (_("Wrong number of arguments\n"));
    Usage (argv[0]);
    exit (1);
  }

  if (LoadDB ())
  {
    g_print (_("Cannot load database module %s in directory %s!\n"),
              smsdConfig.dbMod, smsdConfig.libDir);
    exit (-2);
  }
}


static void *SendSMS (void *a)
{
  if ((*DB_ConnectOutbox) (connection))
  {
    pthread_exit (0);
    return (NULL);
  }

  while (1)
  {
    pthread_mutex_lock (&db_monitorMutex);
    if (!db_monitor)
    {
      pthread_mutex_unlock (&db_monitorMutex);
      pthread_exit (0);
      return (NULL);
    }
    pthread_mutex_unlock (&db_monitorMutex);

    (*DB_Look) (smsdConfig.phone);

    sleep (3);
  }
}


gint WriteSMS (gn_sms *sms)
{
  gn_error error;
  PhoneEvent *e = (PhoneEvent *) g_malloc (sizeof (PhoneEvent));
  D_SMSMessage *m = (D_SMSMessage *) g_malloc (sizeof (D_SMSMessage));

  m->sms = sms;
  e->event = Event_SendSMSMessage;
  e->data = m;
  InsertEvent (e);
  pthread_mutex_lock (&sendSMSMutex);
  pthread_cond_wait (&sendSMSCond, &sendSMSMutex);
  pthread_mutex_unlock (&sendSMSMutex);

  gn_log_xdebug ("Address: %s\nText: %s\n",
  sms->remote.number,
  sms->user_data[0].u.text);

  error = m->status;
  g_free (m);

  if (smsdConfig.logFile)
  {
    if (error)
      LogFile (_("Sending to %s unsuccessful. Error %d\n"), sms->remote.number, error);
    else
      LogFile (_("Sending to %s successful.\n"), sms->remote.number);
  }

  return (error);
}


static void ReadSMS (gpointer d, gpointer userData)
{
  gn_sms *data = (gn_sms *) d;
  PhoneEvent *e;
  gint error;
  
  if (data->type == GN_SMS_MT_Deliver || data->type == GN_SMS_MT_DeliveryReport)
  {
/*    if (data->type == GN_SMS_MT_DeliveryReport)
    {
      if (smsdConfig.smsSets & SMSD_READ_REPORTS)
        error = (*DB_InsertSMS) (data);
    }
    else */
    {  
      gn_log_xdebug ("%d. %s   ", data->number, data->remote.number);
      gn_log_xdebug ("%02d-%02d-%02d %02d:%02d:%02d+%02d %s\n", data->smsc_time.year,
                data->smsc_time.month, data->smsc_time.day, data->smsc_time.hour,
                data->smsc_time.minute, data->smsc_time.second, data->smsc_time.timezone,
                data->user_data[0].u.text);
      error = (*DB_InsertSMS) (data, smsdConfig.phone);
    }
    
    if (smsdConfig.logFile)
    {
      if (error)
        LogFile (_("Inserting sms from %s unsuccessful.\n"), data->remote.number);
      else
        LogFile (_("Inserting sms from %s successful.\n"), data->remote.number);
    }
    
    e = (PhoneEvent *) g_malloc (sizeof (PhoneEvent));
    e->event = Event_DeleteSMSMessage;
    e->data = data;
    InsertEvent (e);
  }
}


static void GetSMS (void)
{
  while (1)
  {
    pthread_mutex_lock (&smsMutex);
    pthread_cond_wait (&smsCond, &smsMutex);

    // Waiting for signal
    pthread_mutex_unlock (&smsMutex);
    // Signal arrived.
    
    pthread_mutex_lock (&smsMutex);
    phoneMonitor.sms.messages = g_slist_reverse(phoneMonitor.sms.messages);
    g_slist_foreach (phoneMonitor.sms.messages, ReadSMS, (gpointer) NULL);
    pthread_mutex_unlock (&smsMutex);
  }
}


static void MainExit (gint sig)
{
  PhoneEvent *e = (PhoneEvent *) g_malloc (sizeof (PhoneEvent));

  e->event = Event_Exit;
  e->data = NULL;
  InsertEvent (e);
  
  pthread_mutex_lock (&db_monitorMutex);
  db_monitor = FALSE;
  pthread_mutex_unlock (&db_monitorMutex);
  
  pthread_join (monitor_th, NULL);
  pthread_join (db_monitor_th, NULL);
  (*DB_Bye) ();
  exit (0);
}


static void Run (void)
{
  struct sigaction act;
  
  act.sa_handler = MainExit;
  sigemptyset (&(act.sa_mask));
  
  sigaction (SIGQUIT, &act, NULL);
  sigaction (SIGTERM, &act, NULL);
  sigaction (SIGINT, &act, NULL);

#if __unices__
  act.sa_handler = SIG_IGN;
  sigemptyset (&(act.sa_mask));
  sigaction (SIGALRM, &act, NULL);
#endif

  InitPhoneMonitor ();
  if ((*DB_ConnectInbox) (connection))
    exit (2);
  pthread_create (&monitor_th, NULL, Connect, smsdConfig.phone);
  db_monitor = TRUE;
  pthread_mutex_init (&db_monitorMutex, NULL);
  pthread_create (&db_monitor_th, NULL, SendSMS, NULL);
  GetSMS ();
}


int main (int argc, char *argv[])
{
#ifdef ENABLE_NLS
  textdomain ("gnokii");
  setlocale (LC_ALL, "");
#endif

  gn_elog_handler = NULL;
  ReadConfig (argc, argv);
  GTerminateThread = false;
  Run ();

  return(0);
}
