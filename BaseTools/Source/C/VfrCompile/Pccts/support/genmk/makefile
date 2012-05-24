##
##  7-Apr-97
##  added support/genmk/makefile to pccts 1.33MR1 distribution kit
##		(support/genmk/makefile" omitted from 1.33 distribution kit)
##
SRC=genmk.c
OBJ=genmk.o
# Define PC if you use a PC OS (changes directory symbol and object file extension)
# see pccts/h/pcctscfg.h
CC=cc
COPT=-O
#CFLAGS=-I../../h -DPC
CFLAGS=$(COPT) -I../../h
BAG=../../bin/bag

genmk: $(OBJ) $(SRC) ../../h/pcctscfg.h
	$(CC) -o genmk $(OBJ)

clean:
	rm -rf core *.o

scrub:
	rm -rf genmk core *.o

shar:
	shar genmk.c makefile > genmk.shar

archive:
	$(BAG) genmk.c makefile > genmk.bag
