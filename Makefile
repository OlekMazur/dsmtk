CFLAGS	+= -g -std=c99 -pedantic -Werror -Wall -Wextra -Wmissing-declarations -Wdeclaration-after-statement -Wformat=2
LDFLAGS	+= -g

.PHONY:	all clean

all:	dsmtk

clean:
	rm -f dsmtk dsmtk.o

dsmtk:	dsmtk.o
	$(CC) $(CFLAGS) $^ -o $@

%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

dsmtk.o:	dsmtk.c
