GLIB= `glib-config --cflags --libs`
CFLAGS= -g -O -Wall

###################################################################
###################################################################

all: mp3asm

clean:
	 rm -f *core *.o *.ln *~ mp3asm

utils.o:	utils.c mp3asm.h utils.h
	$(CC) $(CFLAGS) -c utils.c $(GLIB)

parse.o:	parse.c mp3asm.h utils.h parse.h
	$(CC) $(CFLAGS) -c parse.c $(GLIB)
 	
frame.o:	frame.c mp3asm.h utils.h stream.h
	$(CC) $(CFLAGS) -c frame.c $(GLIB)

stream.o:	stream.c mp3asm.h stream.h utils.h 
	$(CC) $(CFLAGS) -c stream.c $(GLIB)

mp3asm.o:	mp3asm.c mp3asm.h parse.h frame.h
	$(CC) $(CFLAGS) -c mp3asm.c $(GLIB)

mp3asm:		utils.o parse.o frame.o stream.o mp3asm.o
	$(CC) $(CFLAGS) -o mp3asm mp3asm.o stream.o parse.o \
	 		utils.o frame.o
	
#strip mp3asm
