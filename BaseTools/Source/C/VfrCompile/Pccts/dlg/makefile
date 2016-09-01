#
# Makefile for DLG 1.33
# Terence Parr
# Purdue University, U of MN, Parr Research Corporation
# 1989-1994
#
# Ported to IBM C-Set/2 and Microsoft 6.0 by
# Ed Harfmann
# Micro Data Base Systems
# Lafayette, Indiana
#
SET=../support/set
PCCTS_H=../h

##
## Uncomment the appropriate section to build
##

#
#   OS/2 & DOS 16 bit using MSC 6.0
#
#CC=cl
#ANTLR=..\bin\antlr
#DLG=..\bin\dlg
#CFLAGS= -I. -I$(SET) -I$(PCCTS_H) /AL /Za /W3 -DPC -DUSER_ZZSYN
#OUT_OBJ = -Fo
#LIBS=/NOD:LLIBCE LLIBCEP
#OBJ_EXT = obj
#
#dlg.exe : dlg_p.obj dlg_a.obj main.obj err.obj set.obj support.obj \
#        output.obj relabel.obj automata.obj
#        link @<<
#$** /NOI
#$@ /STACK:16384
#
#$(LIBS: = +^
#)
#$(DEF_FILE) $(LFLAGS) ;
#<<
#        bind $@ c:\os2\doscalls.lib
#        copy *.exe ..\bin
#

#
#   Borland C++ for DOS
#
#CC=bcc
#ANTLR=..\bin\antlr
#DLG=..\bin\dlg
#CFLAGS= -I. -I$(SET) -I$(PCCTS_H) -ml -ff- -w- -DPC -DUSER_ZZSYN
#OUT_OBJ = -o
#LIBS= emu mathl cl
#OBJ_EXT = obj
#
#dlg.exe : dlg_p.obj dlg_a.obj main.obj err.obj set.obj support.obj \
#        output.obj relabel.obj automata.obj
#       tlink @&&|
#C0L $**
#$@ /Tde /c
#
#$(LIBS)
#$(DEF_FILE) $(LFLAGS) ;
#|
#        copy *.exe ..\bin
#

#
# C-Set/2 for OS/2
#
#CC=icc
#CFLAGS= -I. -I$(SET) -I$(PCCTS_H) /Sa /W3 /DUSER_ZZSYN
#OUT_OBJ = -Fo
#LIBS=
#ANTLR=..\bin\antlr
#DLG=..\bin\dlg
#OBJ_EXT=obj
#
#dlg.exe : dlg_p.obj dlg_a.obj main.obj err.obj set.obj support.obj \
#        output.obj relabel.obj automata.obj
#        link386 @<<
#$** /NOI
#$@ /STACK:32768
#
#$(LIBS: = +^
#)
#$(DEF_FILE) $(LFLAGS) ;
#<<
#        copy *.exe ..\bin
#

#
# Borland C++ for OS/2
#
#CC=bcc
#CFLAGS= -I. -I$(SET) -I$(PCCTS_H) -w- -DUSER_ZZSYN
#OUT_OBJ = -o
#LIBS= c2 os2
#
#ANTLR=..\bin\antlr
#DLG=..\bin\dlg
#OBJ_EXT = obj
#dlg.exe : dlg_p.obj dlg_a.obj main.obj err.obj set.obj support.obj \
#        output.obj relabel.obj automata.obj
#        tlink @&&|
#c02 $** -c
#dlg.exe
#
#C2 os2
#
#|
#        copy *.exe ..\bin
#

#
#   UNIX
#
BUILD_CC?=cc
COPT=-O
ANTLR=${BIN_DIR}/antlr
DLG=${BIN_DIR}/dlg
BUILD_CFLAGS= $(COPT) -I. -I$(SET) -I$(PCCTS_H) -DUSER_ZZSYN -DZZLEXBUFSIZE=65536
BUILD_CPPFLAGS=
OBJ_EXT=o
OUT_OBJ = -o
OBJ = dlg_p.o dlg_a.o main.o err.o set.o support.o output.o \
        relabel.o automata.o

$(BIN_DIR)/dlg : $(OBJ) $(SRC)
		$(BUILD_CC) $(BUILD_CFLAGS) -o $(BIN_DIR)/dlg $(OBJ)

SRC = dlg_p.c dlg_a.c main.c err.c $(SET)/set.c support.c output.c \
        relabel.c automata.c

#dlg_p.c parser.dlg err.c tokens.h : dlg_p.g
#	$(ANTLR) dlg_p.g

#dlg_a.c mode.h : parser.dlg
#	$(DLG) -C2 parser.dlg dlg_a.c

dlg_p.$(OBJ_EXT) : dlg_p.c dlg.h tokens.h mode.h
	$(BUILD_CC) $(BUILD_CFLAGS) -c dlg_p.c

dlg_a.$(OBJ_EXT) : dlg_a.c dlg.h tokens.h mode.h
	$(BUILD_CC) $(BUILD_CFLAGS) -c dlg_a.c

main.$(OBJ_EXT) : main.c dlg.h
	$(BUILD_CC) $(BUILD_CFLAGS) -c main.c

set.$(OBJ_EXT) : $(SET)/set.c
	$(BUILD_CC) -c $(BUILD_CFLAGS) $(SET)/set.c

%.o : %.c 
	$(BUILD_CC) -c $(BUILD_CFLAGS) $(BUILD_CPPFLAGS) $< -o $@

lint:
	lint *.c

#clean up all the intermediate files
clean:
	rm -f $(BIN_DIR)/dlg *.$(OBJ_EXT) core
