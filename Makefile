all: fatfs

CC=gcc
CFLAGS=-g

FATFSOBJS=fatfs.o fat12.o

FATFS_CLIBS=

fatfs: $(FATFSOBJS)
		$(CC) -o fatfs $(FATFSOBJS) $(FATFS_CLIBS) 

clean:
		rm -f *.o
		rm -f fatfs
		rm -f out.txt

run:
		./fatfs fat_volume.dat fat12
