# PCCTS directory

# You will need to set the LIB variable similar to this.
# LIB="C:/Program Files/Microsoft Visual Studio .NET 2003/Vc7/lib;c:/Microsoft Visual Studio .NET 2003/Vc7/PlatformSDK/Lib"

# PCCTS_HOME=<your PCCTS_HOME>
PCCTS_HOME=$(BASE_TOOLS_PATH)\Source\C\VfrCompile\Pccts
ANTLR_SRC=$(PCCTS_HOME)\antlr
PCCTS_H=$(PCCTS_HOME)\h

!IFNDEF HOST_ARCH
HOST_ARCH = IA32
!ENDIF

!IF "$(HOST_ARCH)"=="IA32"
SYS_BIN_PATH=$(EDK_TOOLS_PATH)\Bin\Win32
!ELSE
SYS_BIN_PATH=$(EDK_TOOLS_PATH)\Bin\Win64
!ENDIF

# Support directories
SET=$(PCCTS_HOME)\support\set


# Compiler stuff
CC = cl
CFLAGS = /nologo -I "." -I "$(PCCTS_H)" -I "$(SET)" -D "USER_ZZSYN" -D "PC" \
        -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /Z7 /W3 -D__USE_PROTOS /wd4700 \
         /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE

ANTLR_OBJS = antlr.obj scan.obj err.obj bits.obj build.obj fset2.obj \
            fset.obj gen.obj globals.obj hash.obj lex.obj main.obj \
            misc.obj pred.obj egman.obj mrhoist.obj fcache.obj

SUPPORT_OBJS = set.obj

# Dependencies

$(SYS_BIN_PATH)\antlr.exe: $(ANTLR_OBJS) $(SUPPORT_OBJS)
    $(CC) $(CFLAGS) -Feantlr.exe $(ANTLR_OBJS) $(SUPPORT_OBJS)
    -@if not exist $(SYS_BIN_PATH) mkdir $(SYS_BIN_PATH)
		copy antlr.exe $(SYS_BIN_PATH)


antlr.obj: $(ANTLR_SRC)\antlr.c \
	                $(PCCTS_H)\antlr.h \
                	$(PCCTS_H)\config.h \
	                $(PCCTS_H)\dlgdef.h \
					$(SET)\set.h \
					$(ANTLR_SRC)\generic.h \
					$(ANTLR_SRC)\hash.h \
					$(ANTLR_SRC)\mode.h \
					$(ANTLR_SRC)\proto.h \
					$(ANTLR_SRC)\syn.h \
					$(ANTLR_SRC)\tokens.h \

    $(CC) -c $(CFLAGS) $(ANTLR_SRC)\antlr.c

scan.obj: $(ANTLR_SRC)\scan.c \
	                $(PCCTS_H)\antlr.h \
					$(PCCTS_H)\config.h \
					$(PCCTS_H)\dlgauto.h \
					$(PCCTS_H)\dlgdef.h \
					$(SET)\set.h \
					$(ANTLR_SRC)\generic.h \
					$(ANTLR_SRC)\hash.h \
					$(ANTLR_SRC)\mode.h \
					$(ANTLR_SRC)\proto.h \
					$(ANTLR_SRC)\syn.h \
					$(ANTLR_SRC)\tokens.h \

    $(CC) -c $(CFLAGS) $(ANTLR_SRC)\scan.c

err.obj: $(ANTLR_SRC)\err.c \
					$(PCCTS_H)\antlr.h \
					$(PCCTS_H)\config.h \
					$(PCCTS_H)\dlgdef.h \
					$(PCCTS_H)\err.h \
					$(SET)\set.h \
					$(ANTLR_SRC)\generic.h \
					$(ANTLR_SRC)\hash.h \
					$(ANTLR_SRC)\proto.h \
					$(ANTLR_SRC)\syn.h \
					$(ANTLR_SRC)\tokens.h \

    $(CC) -c $(CFLAGS) $(ANTLR_SRC)\err.c

bits.obj: $(ANTLR_SRC)\bits.c \
					$(PCCTS_H)\config.h \
					$(PCCTS_H)\dlgdef.h \
					$(SET)\set.h \
					$(ANTLR_SRC)\generic.h \
					$(ANTLR_SRC)\hash.h \
					$(ANTLR_SRC)\proto.h \
					$(ANTLR_SRC)\syn.h \

    $(CC) -c $(CFLAGS) $(ANTLR_SRC)\bits.c

build.obj: $(ANTLR_SRC)\build.c \
					$(PCCTS_H)\config.h \
					$(PCCTS_H)\dlgdef.h \
					$(SET)\set.h \
					$(ANTLR_SRC)\generic.h \
					$(ANTLR_SRC)\hash.h \
					$(ANTLR_SRC)\proto.h \
					$(ANTLR_SRC)\syn.h \

    $(CC) -c $(CFLAGS) $(ANTLR_SRC)\build.c

fset2.obj: $(ANTLR_SRC)\fset2.c \
					$(PCCTS_H)\config.h \
					$(PCCTS_H)\dlgdef.h \
					$(SET)\set.h \
					$(ANTLR_SRC)\generic.h \
					$(ANTLR_SRC)\hash.h \
					$(ANTLR_SRC)\proto.h \
					$(ANTLR_SRC)\syn.h \

    $(CC) -c $(CFLAGS) $(ANTLR_SRC)\fset2.c

fset.obj: $(ANTLR_SRC)\fset.c \
					$(PCCTS_H)\config.h \
					$(PCCTS_H)\dlgdef.h \
					$(SET)\set.h \
					$(ANTLR_SRC)\generic.h \
					$(ANTLR_SRC)\hash.h \
					$(ANTLR_SRC)\proto.h \
					$(ANTLR_SRC)\syn.h \

    $(CC) -c $(CFLAGS) $(ANTLR_SRC)\fset.c

gen.obj: $(ANTLR_SRC)\gen.c \
					$(PCCTS_H)\config.h \
					$(PCCTS_H)\dlgdef.h \
					$(SET)\set.h \
					$(ANTLR_SRC)\generic.h \
					$(ANTLR_SRC)\hash.h \
					$(ANTLR_SRC)\proto.h \
					$(ANTLR_SRC)\syn.h \

    $(CC) -c $(CFLAGS) $(ANTLR_SRC)\gen.c

globals.obj: $(ANTLR_SRC)\globals.c \
					$(PCCTS_H)\config.h \
					$(SET)\set.h \
					$(ANTLR_SRC)\generic.h \
					$(ANTLR_SRC)\hash.h \
					$(ANTLR_SRC)\proto.h \
					$(ANTLR_SRC)\syn.h \

    $(CC) -c $(CFLAGS) $(ANTLR_SRC)\globals.c

hash.obj: $(ANTLR_SRC)\hash.c \
					$(PCCTS_H)\config.h \
					$(ANTLR_SRC)\hash.h \

    $(CC) -c $(CFLAGS) $(ANTLR_SRC)\hash.c

lex.obj: $(ANTLR_SRC)\lex.c \
					$(PCCTS_H)\config.h \
					$(SET)\set.h \
					$(ANTLR_SRC)\generic.h \
					$(ANTLR_SRC)\hash.h \
					$(ANTLR_SRC)\proto.h \
					$(ANTLR_SRC)\syn.h \

    $(CC) -c $(CFLAGS) $(ANTLR_SRC)\lex.c

main.obj: $(ANTLR_SRC)\main.c \
					$(PCCTS_H)\antlr.h \
					$(PCCTS_H)\config.h \
					$(PCCTS_H)\dlgdef.h \
					$(SET)\set.h \
					$(ANTLR_SRC)\generic.h \
					$(ANTLR_SRC)\hash.h \
					$(ANTLR_SRC)\mode.h \
					$(ANTLR_SRC)\proto.h \
					$(ANTLR_SRC)\stdpccts.h \
					$(ANTLR_SRC)\syn.h \
					$(ANTLR_SRC)\tokens.h \

    $(CC) -c $(CFLAGS) $(ANTLR_SRC)\main.c

misc.obj: $(ANTLR_SRC)\misc.c \
					$(PCCTS_H)\config.h \
					$(PCCTS_H)\dlgdef.h \
					$(SET)\set.h \
					$(ANTLR_SRC)\generic.h \
					$(ANTLR_SRC)\hash.h \
					$(ANTLR_SRC)\proto.h \
					$(ANTLR_SRC)\syn.h \

    $(CC) -c $(CFLAGS) $(ANTLR_SRC)\misc.c

pred.obj: $(ANTLR_SRC)\pred.c \
					$(PCCTS_H)\config.h \
					$(PCCTS_H)\dlgdef.h \
					$(SET)\set.h \
					$(ANTLR_SRC)\generic.h \
					$(ANTLR_SRC)\hash.h \
					$(ANTLR_SRC)\proto.h \
					$(ANTLR_SRC)\syn.h \

    $(CC) -c $(CFLAGS) $(ANTLR_SRC)\pred.c

egman.obj: $(ANTLR_SRC)\egman.c \
					$(PCCTS_H)\config.h \
					$(SET)\set.h \
					$(ANTLR_SRC)\generic.h \
					$(ANTLR_SRC)\hash.h \
					$(ANTLR_SRC)\proto.h \
					$(ANTLR_SRC)\syn.h \

    $(CC) -c $(CFLAGS) $(ANTLR_SRC)\egman.c

mrhoist.obj: $(ANTLR_SRC)\mrhoist.c \
					$(ANTLR_SRC)\generic.h \
					$(ANTLR_SRC)\hash.h \
					$(ANTLR_SRC)\proto.h \
					$(ANTLR_SRC)\syn.h \

    $(CC) -c $(CFLAGS) $(ANTLR_SRC)\mrhoist.c

fcache.obj: $(ANTLR_SRC)\fcache.c \
					$(ANTLR_SRC)\generic.h \
					$(ANTLR_SRC)\hash.h \
					$(ANTLR_SRC)\proto.h \
					$(ANTLR_SRC)\syn.h \

    $(CC) -c $(CFLAGS) $(ANTLR_SRC)\fcache.c

set.obj: $(SET)\set.c \
					$(PCCTS_H)\config.h \
					$(SET)\set.h \

    $(CC) -c $(CFLAGS) $(SET)\set.c

clean:
    -del *.obj
    -del *.ilk
    -del *.pdb

cleanall:
    -del *.obj
    -del *.ilk
    -del *.pdb
		-del *.exe
    -del $(EDK_TOOLS_PATH)\Bin\Win32\antlr.exe
