# Makefile for dsmtk

CFLAGS=-s
VERSION=0.2
TARBALL=dsmtk-$(VERSION).tar.gz
DISTRO_FILES=COPYING README Makefile *.c dsmtk dsload

all:	dsmtk

tarball:	$(TARBALL)

$(TARBALL):	$(DISTRO_FILES)
	tar czf $@ $^

dsmtk:	dsmtk.o
	$(CC) $(CFLAGS) $^ -o $@

%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

dsmtk.o:	dsmtk.c
