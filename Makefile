
#
#   Flags for Solaris (SparcWorks cc):
#
#CFLAGS=-fast -native -xO5 -Xc -D__EXTENSIONS__ -v

#
#   Flags for FreeBSD:
#
CFLAGS=-O5 -ansi -pedantic -Wall

###############################################################
###############################################################

all:	mp3asm

clean:
	rm -f *core *.o *.ln *~ localtypes.h gettypes mp3asm

gettypes:	gettypes.c
	$(CC) $(TYPEFLAGS) -o gettypes gettypes.c

localtypes.h:	gettypes
	./gettypes > localtypes.h

utils.o:	utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

frameinfo.o:	frameinfo.c frameinfo.h localtypes.h
	$(CC) $(CFLAGS) -c frameinfo.c

getlopt.o:	getlopt.c getlopt.h
	$(CC) $(CFLAGS) -c getlopt.c

mp3asm.o:	mp3asm.c frameinfo.h utils.h localtypes.h getlopt.h
	$(CC) $(CFLAGS) -c mp3asm.c

mp3asm:	mp3asm.o frameinfo.o utils.o getlopt.o
	$(CC) $(CFLAGS) -o mp3asm mp3asm.o frameinfo.o utils.o getlopt.o
	strip mp3asm

# EOF
