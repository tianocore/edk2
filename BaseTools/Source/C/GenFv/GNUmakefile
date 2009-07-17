ARCH ?= IA32
MAKEROOT ?= ..

APPNAME = GenFv

OBJECTS = GenFv.o GenFvInternalLib.o

include $(MAKEROOT)/Makefiles/app.makefile

LIBS = -lCommon
ifeq ($(CYGWIN), CYGWIN)
  LIBS += -L/lib/e2fsprogs -luuid
endif

ifeq ($(LINUX), Linux)
  LIBS += -luuid
endif

