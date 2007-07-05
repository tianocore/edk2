SOURCE_FILES = $(DEBUG_DIR)\AutoGen.c                 \
               $(MODULE_DIR)\Nt32Pkg\Sec\WinNtThunk.c \
               $(MODULE_DIR)\Nt32Pkg\Sec\FwVol.c      \
               $(MODULE_DIR)\Nt32Pkg\Sec\SecMain.c
               
INC = -I$(WORKSPACE)\Nt32Pkg\Sec            \
      -I$(DEBUG_DIR)                        \
      -I$(WORKSPACE)\Nt32Pkg                \
      -I$(WORKSPACE)\Nt32Pkg\Include        \
      -I$(WORKSPACE)\MdePkg                 \
      -I$(WORKSPACE)\MdePkg\Include\Ia32    \
      -I$(WORKSPACE)\MdePkg\Include         \
      -I$(WORKSPACE)\MdeModulePkg\Include   \
      -I$(WORKSPACE)\IntelFrameworkPkg\Include

OBJECTS = $(OUTPUT_DIR)\.\AutoGen.obj       \
          $(OUTPUT_DIR)\.\WinNtThunk.obj    \
          $(OUTPUT_DIR)\.\FwVol.obj         \
          $(OUTPUT_DIR)\.\SecMain.obj
          
LIBS = $(LIB_DIR)\BaseDebugLibNull.lib      \
       $(LIB_DIR)\BasePrintLib.lib          \
       $(LIB_DIR)\BaseMemoryLib.lib         \
       $(LIB_DIR)\BaseLib.lib               \
       $(LIB_DIR)\BasePeCoffLib.lib 

MSVS_LINK_LIBPATHS =  /LIBPATH:"$(VCINSTALLDIR)\Lib" /LIBPATH:"$(VCINSTALLDIR)\PlatformSdk\Lib"
LINK_FLAGS_EXE = /NOLOGO /SUBSYSTEM:CONSOLE /NODEFAULTLIB /IGNORE:4086 /MAP /OPT:REF /DEBUG /MACHINE:I386 /LTCG \
                 Kernel32.lib MSVCRTD.lib Gdi32.lib User32.lib Winmm.lib
CC_FLAGS = /nologo /W4 /WX /Gy /c /D UNICODE /D EFI32 /Od /DSTRING_ARRAY_NAME=${BASE_NAME}Strings /FI$(DEST_DIR_DEBUG)/AutoGen.h /EHs-c- /GF /Gs8192 /Fp${DEST_DIR_OUTPUT}/AutoGen.h.gch /Yu$(DEST_DIR_DEBUG)/AutoGen.h /Zi /Gm
COMMON_DEPS = $(DEBUG_DIR)\AutoGen.h

all : $(BIN_DIR)\SecMain.exe

$(BIN_DIR)\SecMain.exe : $(OBJECTS)
  "$(DLINK)"                        \
    $(MSVS_LINK_LIBPATHS)           \
    $(LINK_FLAGS_EXE)               \
    $(OBJECTS)                      \
    $(LIBS)                         \
    /base:0x10000000                \
    /out:"$(BIN_DIR)\SecMain.exe"   \
    /pdb:"$(BIN_DIR)\SecMain.pdb"
  
#
# Individual Object Build Targets
#
$(OUTPUT_DIR)\.\AutoGen.obj : $(COMMON_DEPS)
$(OUTPUT_DIR)\.\AutoGen.obj : $(DEBUG_DIR)\AutoGen.c
	"$(CC)" $(CC_FLAGS) $(INC) /Fo$@ $(DEBUG_DIR)\AutoGen.c

$(OUTPUT_DIR)\.\WinNtThunk.obj : $(COMMON_DEPS)
$(OUTPUT_DIR)\.\WinNtThunk.obj : $(MODULE_DIR)\WinNtThunk.c
	"$(CC)" $(CC_FLAGS) $(INC) /Fo$@ $(MODULE_DIR)\WinNtThunk.c

$(OUTPUT_DIR)\.\FwVol.obj : $(COMMON_DEPS)
$(OUTPUT_DIR)\.\FwVol.obj : $(MODULE_DIR)\FwVol.c
	"$(CC)" $(CC_FLAGS) $(INC) /Fo$@ $(MODULE_DIR)\FwVol.c
	
$(OUTPUT_DIR)\.\SecMain.obj : $(COMMON_DEPS)
$(OUTPUT_DIR)\.\SecMain.obj : $(MODULE_DIR)\SecMain.c
	"$(CC)" $(CC_FLAGS) $(INC) /Fo$@ $(MODULE_DIR)\SecMain.c
	
clean:
  - @if exist $(DEBUG_DIR)\AutoGen.c del $(DEBUG_DIR)\AutoGen.c > NUL
  del $(DEBUG_DIR)\AutoGen.h > NUL
  del $(OUTPUT_DIR)\.\*.obj > NUL
  del $(MODULE_BUILD_DIR)\makefile > NUL
  del $(MODULE_BUILD_DIR)\*.idb > NUL
  del $(MODULE_BUILD_DIR)\*.pdb > NUL
  del $(BIN_DIR)\SecMain.exe > NUL
  del $(BIN_DIR)\SecMain.pdb > NUL
  
  