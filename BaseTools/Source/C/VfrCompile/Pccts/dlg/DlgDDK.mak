# PCCTS directory

# You will need to set the LIB variable similar to this.
# LIB="C:/Program Files/Microsoft Visual Studio .NET 2003/Vc7/lib;c:/Microsoft Visual Studio .NET 2003/Vc7/PlatformSDK/Lib"

# PCCTS_HOME=<your PCCTS_HOME>
PCCTS_HOME=$(WORKSPACE)\Tools\CCode\Source\Pccts
DLG_SRC=$(PCCTS_HOME)\dlg
PCCTS_H=$(PCCTS_HOME)\h


# Support directories
SET=$(PCCTS_HOME)\support\set


# Compiler stuff
CC = cl
CFLAGS = /nologo -I "." -I "$(PCCTS_H)" -I "$(SET)" -D "USER_ZZSYN" -D "PC" \
        -D "ZZLEXBUFSIZE=65536"  /D "LONGFILENAMES" /W3 /Zi

DLG_OBJS = dlg_p.obj dlg_a.obj main.obj err.obj support.obj \
           output.obj relabel.obj automata.obj

SUPPORT_OBJS = set.obj

# Dependencies

dlg.exe: $(DLG_OBJS) $(SUPPORT_OBJS)
    $(CC) $(CFLAGS) -o dlg.exe bufferoverflowu.lib $(DLG_OBJS) $(SUPPORT_OBJS)
    del *.obj
    del *.ilk
    del *.pdb
		move dlg.exe $(WORKSPACE)\Tools\bin\.

dlg_p.obj: $(DLG_SRC)\dlg_p.c \
					$(PCCTS_H)\antlr.h \
					$(PCCTS_H)\config.h \
					$(PCCTS_H)\dlgdef.h \
					$(SET)\set.h \
                	$(DLG_SRC)\dlg.h \
                	$(DLG_SRC)\mode.h \
                	$(DLG_SRC)\tokens.h \

    $(CC) -c $(CFLAGS) $(DLG_SRC)\dlg_p.c

dlg_a.obj: $(DLG_SRC)\dlg_a.c \
					$(PCCTS_H)\antlr.h \
					$(PCCTS_H)\config.h \
					$(PCCTS_H)\dlgauto.h \
					$(PCCTS_H)\dlgdef.h \
					$(SET)\set.h \
                	$(DLG_SRC)\dlg.h \
                	$(DLG_SRC)\mode.h \
                	$(DLG_SRC)\tokens.h \

    $(CC) -c $(CFLAGS) $(DLG_SRC)\dlg_a.c

main.obj: $(DLG_SRC)\main.c \
					$(PCCTS_H)\antlr.h \
					$(PCCTS_H)\config.h \
					$(PCCTS_H)\dlgdef.h \
					$(SET)\set.h \
                	$(DLG_SRC)\dlg.h \
                	$(DLG_SRC)\mode.h \
                	$(DLG_SRC)\stdpccts.h \
                	$(DLG_SRC)\tokens.h \

    $(CC) -c $(CFLAGS) $(DLG_SRC)\main.c

err.obj: $(DLG_SRC)\err.c \
					$(PCCTS_H)\antlr.h \
					$(PCCTS_H)\config.h \
					$(PCCTS_H)\dlgdef.h \
					$(PCCTS_H)\err.h \
					$(SET)\set.h \
                	$(DLG_SRC)\dlg.h \
                	$(DLG_SRC)\tokens.h \

    $(CC) -c $(CFLAGS) $(DLG_SRC)\err.c

support.obj: $(DLG_SRC)\support.c \
					$(PCCTS_H)\config.h \
					$(SET)\set.h \
                	$(DLG_SRC)\dlg.h \

    $(CC) -c $(CFLAGS) $(DLG_SRC)\support.c

output.obj: $(DLG_SRC)\output.c \
					$(PCCTS_H)\config.h \
					$(SET)\set.h \
                	$(DLG_SRC)\dlg.h \

    $(CC) -c $(CFLAGS) $(DLG_SRC)\output.c

relabel.obj: $(DLG_SRC)\relabel.c \
					$(PCCTS_H)\config.h \
					$(SET)\set.h \
                	$(DLG_SRC)\dlg.h \

    $(CC) -c $(CFLAGS) $(DLG_SRC)\relabel.c

automata.obj: $(DLG_SRC)\automata.c \
					$(PCCTS_H)\config.h \
					$(SET)\set.h \
                	$(DLG_SRC)\dlg.h \

    $(CC) -c $(CFLAGS) $(DLG_SRC)\automata.c


set.obj: $(SET)\set.c \
					$(PCCTS_H)\config.h \
					$(SET)\set.h \

    $(CC) -c $(CFLAGS) $(SET)\set.c

clean:	
    del *.obj

distclean:
    del *.obj
    del $(WORKSPACE)\Tools\bin\dlg.exe
