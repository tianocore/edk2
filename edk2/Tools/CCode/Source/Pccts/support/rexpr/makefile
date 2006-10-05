BAG=../../bin/bag
SRC=test.c rexpr.c
OBJ=test.o rexpr.o
CFLAGS = -g

test: $(OBJ) $(SRC)
	cc -g -o texpr $(OBJ)

shar:
	shar makefile test.c rexpr.c rexpr.h > rexpr.shar

archive:
	$(BAG) makefile test.c rexpr.c rexpr.h > rexpr.bag

clean:
	rm -rf *.o core texpr

scrub:
	rm -rf *.o core texpr
