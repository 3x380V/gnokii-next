/*
  $Id: virtmodem.c,v 1.11 2002-02-13 21:16:37 pkot Exp $

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Jan�k ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides a virtual modem interface to the GSM phone by calling
  code in gsm-api.c, at-emulator.c and datapump.c. The code here provides
  the overall framework and coordinates switching between command mode
  (AT-emulator) and "online" mode where the data pump code translates data
  from/to the GSM handset and the modem data/fax stream.

*/

#include <config.h>

/* This is the right way to include stdlib with __USE_XOPEN defined */
#ifdef USE_UNIX98PTYS
#  define _XOPEN_SOURCE 500
#  include <features.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <grp.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include "misc.h"
#include "gsm-api.h"
#include "gsm-common.h"
#include "data/at-emulator.h"
#include "data/datapump.h"
#include "data/virtmodem.h"
#include "data/rlp-common.h"

/* Prototypes */
static int  VM_PtySetup(char *bindir);
static void VM_ThreadLoop(void);
static void VM_CharHandler(void);
static GSM_Error VM_GSMInitialise(char *model,
			   char *port,
			   char *initlength,
			   GSM_ConnectionType connection);

/* Global variables */

extern bool TerminateThread;
int ConnectCount;

/* Local variables */

int PtyRDFD;	/* File descriptor for reading and writing to/from */
int PtyWRFD;	/* pty interface - only different in debug mode. */

bool UseSTDIO;	/* Use STDIO for debugging purposes instead of pty */
bool CommandMode;

pthread_t Thread;
bool RequestTerminate;

/* If initialised in debug mode, stdin/out is used instead
   of ptys for interface. */
bool VM_Initialise(char *model,char *port, char *initlength, GSM_ConnectionType connection, char *bindir, bool debug_mode, bool GSMInit)
{
	int rtn;
	GSM_Statemachine sm;

	CommandMode = true;
	RequestTerminate = false;

	if (debug_mode == true) {
		UseSTDIO = true;
	} else {
		UseSTDIO = false;
	}

	if (GSMInit) {
		dprintf("Initialising GSM\n");
		if ((VM_GSMInitialise(model, port, initlength, connection) != GE_NONE)) {
			fprintf (stderr, _("VM_Initialise - VM_GSMInitialise failed!\n"));
			return (false);
		}
	}
	GSMInit = false;

	if (VM_PtySetup(bindir) < 0) {
		fprintf (stderr, _("VM_Initialise - VM_PtySetup failed!\n"));
		return (false);
	}

	if (ATEM_Initialise(PtyRDFD, PtyWRFD, &sm) != true) {
		fprintf (stderr, _("VM_Initialise - ATEM_Initialise failed!\n"));
		return (false);
	}

	if (DP_Initialise(PtyRDFD, PtyWRFD) != true) {
		fprintf (stderr, _("VM_Initialise - DP_Initialise failed!\n"));
		return (false);
	}

		/* Create and start thread, */
	rtn = pthread_create(&Thread, NULL, (void *) VM_ThreadLoop, (void *)NULL);

	if (rtn == EAGAIN || rtn == EINVAL) {
		return (false);
	}
	return (true);
}

static void VM_ThreadLoop(void)
{
	int res;
	struct pollfd ufds;

	/* Note we can't use signals here as they are already used
	   in the FBUS code.  This may warrant changing the FBUS
	   code around sometime to use select instead to free up
	   the SIGIO handler for mainline code. */

	ufds.fd = PtyRDFD;
	ufds.events = POLLIN;

	while (!RequestTerminate) {
		if (!CommandMode) {
			sleep(1);
		} else {  /* If we are in data mode, leave it to datapump to get the data */

			res = poll(&ufds, 1, 500);

			switch (res) {
			case 0: /* Timeout */
				break;

			case -1:
				perror("VM_ThreadLoop - select");
				exit (-1);

			default:
				if (ufds.revents == POLLIN) {
					VM_CharHandler();
				} else usleep(500); /* Probably the file has been closed */
				break;
			}
		}
	}
}

/* Application should call VM_Terminate to shut down
   the virtual modem thread */
void VM_Terminate(void)
{
	/* Request termination of thread */
	RequestTerminate = true;

	/* Now wait for thread to terminate. */
	pthread_join(Thread, NULL);

	if (!UseSTDIO) {
		close (PtyRDFD);
		close (PtyWRFD);
	}
}

/* The following two functions are based on the skeleton from
 * W. Richard Stevens' "UNIX Network Programming", Volume 1, Second Edition.
 */

static int gread(int fd, void *ptr, size_t nbytes, int *recvfd)
{
	struct msghdr msg;
	struct iovec iov[1];
	int n;

#ifdef HAVE_MSGHDR_MSG_CONTROL
	union {
		struct cmsghdr cm;
		char control[CMSG_SPACE(sizeof(int))];
	} control_un;
	struct cmsghdr *cmptr;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
#else
	int newfd;

	msg.msg_accrights = (caddr_t) &newfd;
	msg.msg_accrightslen = sizeof(int);
#endif /* HAVE_MSGHDR_MSG_CONTROL */

	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	iov[0].iov_base = ptr;
	iov[0].iov_len = nbytes;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	if ((n = recvmsg(fd, &msg, 0)) <= 0) return n;

#ifdef HAVE_MSGHDR_MSG_CONTROL
	if ((cmptr = CMSG_FIRSTHDR(&msg)) != NULL &&
	    cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {
		if (cmptr->cmsg_level != SOL_SOCKET) perror("control level != SOL_SOCKET");
		if (cmptr->cmsg_type != SCM_RIGHTS) perror("control type != SCM_RIGHTS");
		*recvfd = *((int *)CMSG_DATA(cmptr));
	} else {
		*recvfd = -1;
	}
#else
	if (msg.msg_accrightslen == sizeof(int)) *recvfd = newfd;
	else *recvfd = -1;
#endif /* HAVE_MSGHDR_MSG_CONTROL */
	return (n);
}

static int gopen(const char *command)
{
	int fd, sockfd[2], status;
	pid_t childpid;
	char c, argsockfd[10];

	if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd) < 0) return -1;

	switch (childpid = fork()) {
	case -1:
		/* Critical error */
		return -1;
	case 0:
		/* Child */
		close(sockfd[0]);
		snprintf(argsockfd, sizeof(argsockfd), "%d", sockfd[1]);
		execl(command, "mgnokiidev", argsockfd, NULL);
		perror("execl: ");
	default:
		/* Parent */
		break;
	}

	close(sockfd[1]);
	waitpid(childpid, &status, 0);
	if (WIFEXITED(status) == 0) perror("wait: ");
	if ((status = WEXITSTATUS(status)) == 0) {
		gread(sockfd[0], &c, 1, &fd);
	} else {
		errno = status;
		fd = -1;
	}
	close(sockfd[0]);
	return(fd);
}

/* Open pseudo tty interface and (in due course create a symlink
   to be /dev/gnokii etc. ) */
static int VM_PtySetup(char *bindir)
{
	char mgnokiidev[200];

	if (UseSTDIO) {
		PtyRDFD = STDIN_FILENO;
		PtyWRFD = STDOUT_FILENO;
		return (0);
	}

	if (bindir) {
		strncpy(mgnokiidev, bindir, 200);
		strcat(mgnokiidev, "/");
	}
	strncat(mgnokiidev, "mgnokiidev", 200 - strlen(bindir));

	PtyRDFD = gopen(mgnokiidev);

	if (PtyRDFD < 0) {
		fprintf (stderr, _("Couldn't open pty!\n"));
		return(-1);
	}
	PtyWRFD = PtyRDFD;

	return (0);
}

/* Handler called when characters received from serial port.
   calls state machine code to process it. */
static void VM_CharHandler(void)
{
	unsigned char buffer[255];
	int res;

	/* If we are in command mode, get the character, otherwise leave it */

	if (CommandMode && ATEM_Initialised) {
		res = read(PtyRDFD, buffer, 255);
		/* A returned value of -1 means something serious has gone wrong - so quit!! */
		/* Note that file closure etc. should have been dealt with in ThreadLoop */
		if (res < 0) {
			TerminateThread = true;
			return;
		}

		ATEM_HandleIncomingData(buffer, res);
	}
}

/* Initialise GSM interface, returning GSM_Error as appropriate  */
static GSM_Error VM_GSMInitialise(char *model, char *port, char *initlength, GSM_ConnectionType connection)
{
	GSM_Error error;
	static GSM_Statemachine sm;

	/* Initialise the code for the GSM interface. */
	error = GSM_Initialise(model, port, initlength, connection, RLP_DisplayF96Frame, &sm);

	if (error != GE_NONE)
		fprintf(stderr, _("GSM/FBUS init failed!\n"));

	return (error);
}
