/*
 *
 * $Id: unixirda.c,v 1.16 2002-08-05 20:37:26 pkot Exp $
 *
 * G N O K I I
 *
 * A Linux/Unix toolset and driver for Nokia mobile phones.
 *
 * Copyright (C) 1999, 2000 Hugh Blemings & Pavel Jan�k ml.
 * Copyright (C) 2000-2001  Marcel Holtmann <marcel@holtmann.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "devices/unixirda.h"

#ifndef AF_IRDA
#define AF_IRDA 23
#endif

#define INFO_LEN		22
#define DISCOVERY_TIMEOUT	60.0
#define DISCOVERY_SLEEP		0.4

static char *phone[] = {
	"Nokia 3360",
	"Nokia 6210",
	"Nokia 6310",
	"Nokia 6310i",
	"Nokia 6510",
	"Nokia 7110",
	"Nokia 7190",
	"Nokia 7650",
	"Nokia 8210",
	"Nokia 8290",
	"Nokia 8310",
	"Nokia 9110",
	"Nokia 9210"
};

double d_time(void)
{
	double		time;
	struct timeval	tv;

	gettimeofday(&tv, NULL);

	time = tv.tv_sec + (((double)tv.tv_usec) / 1000000.0);

	return time;
}

double d_sleep(double s)
{
	double		time;
	struct timeval	tv1, tv2;

	gettimeofday(&tv1, NULL);
	usleep(s * 1000000);
	gettimeofday(&tv2, NULL);

	time = tv2.tv_sec - tv1.tv_sec + (((double)(tv2.tv_usec - tv1.tv_usec)) / 1000000.0);

	return time;
}

static int irda_discover_device(void)
{
	struct irda_device_list	*list;
	struct irda_device_info	*dev;
	unsigned char		*buf;
	int			s, len, i, j, daddr = -1, fd;
	double			t1, t2;
	int phones = sizeof(phone) / sizeof(*phone);

	fd = socket(AF_IRDA, SOCK_STREAM, 0);

	len = sizeof(*list) + sizeof(*dev) * 10;	/* 10 = max devices in discover */
	buf = malloc(len);
	list = (struct irda_device_list *)buf;
	dev = list->dev;

	t1 = d_time();

	do {
		s = len;
		memset(buf, 0, s);

		if (getsockopt(fd, SOL_IRLMP, IRLMP_ENUMDEVICES, buf, &s) == 0) {
			for (i = 0; (i < list->len) && (daddr == -1); i++) {
				for (j = 0; (j < phones) && (daddr == -1); j++) {
					if (strncmp(dev[i].info, phone[j], INFO_LEN) == 0) {
						daddr = dev[i].daddr;
						dprintf("%s\t%x\n", dev[i].info, dev[i].daddr);
					}
				}
				if (daddr == -1) {
					dprintf("unknown: %s\t%x\n", dev[i].info, dev[i].daddr);
				}
			}
		}

		if (daddr == -1) {
			d_sleep(DISCOVERY_SLEEP);
		}

		t2 = d_time();
	} while ((t2 - t1 < DISCOVERY_TIMEOUT) && (daddr == -1));

	free(buf);
	close(fd);

	return daddr;
}

int irda_open(void)
{
	struct sockaddr_irda	peer;
	int			fd = -1, daddr;

	daddr = irda_discover_device();			/* discover the devices */

	if (daddr != -1)  {
		fd = socket(AF_IRDA, SOCK_STREAM, 0);	/* Create socket */
		peer.sir_family = AF_IRDA;
		peer.sir_lsap_sel = LSAP_ANY;
		peer.sir_addr = daddr;
		strcpy(peer.sir_name, "Nokia:PhoNet");

		if (connect(fd, (struct sockaddr *)&peer, sizeof(peer))) {	/* Connect to service "Nokia:PhoNet" */
			perror("connect");
			close(fd);
			fd = -1;
/*		} else { FIXME: It does not work in most cases. Why? Or why it should work?
			recv(fd, NULL, 0, 0);		 call recv first to make select work correctly */
		}
	}

	return fd;
}

int irda_close(int fd)
{
	shutdown(fd, 0);
	return close(fd);
}

int irda_write(int fd, const __ptr_t bytes, int size)
{
	return (send(fd, bytes, size, 0));
}

int irda_read(int fd, __ptr_t bytes, int size)
{
	return (recv(fd, bytes, size, 0));
}

int irda_select(int fd, struct timeval *timeout)
{
	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	return (select(fd + 1, &readfds, NULL, NULL, timeout));
}
