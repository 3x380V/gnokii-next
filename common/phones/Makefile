
#
# $Id: Makefile,v 1.7 2001-08-09 11:51:39 pkot Exp $
#
# Makefile for the GNOKII tool suite.
#
# Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.
#               2000 Karel Zak
#

#
# For this common directory is used "subsystem.o" .o files concept.
#   (the list of object files to be linked together (to COMMON.o),
#   and other dirs is used this _one_ file (instead of all OBJS)
#

TOPDIR=../..
include $(TOPDIR)/Makefile.global

CFLAGS += $(PTHREAD_CFLAGS)

OBJS =	generic.o \
	nokia.o \
	nk7110.o \
	nk2110.o \
	atgen.o
#	dc2711.o

all: PHONES.o

PHONES.o: $(OBJS)
	$(LD) PHONES.o $(OBJS)

clean:
	$(RM) $(OBJS) *~ depend PHONES.o

install:
	@echo

depend dep:
	$(CC) $(CFLAGS) -MM *.c >depend

ifeq (depend,$(wildcard depend))
include depend
endif


.PHONY: all install clean dep depend

