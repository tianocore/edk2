ARCH ?= IA32
MAKEROOT ?= ..

APPNAME = LzmaCompress

SDK_C = Sdk/C

OBJECTS = \
  LzmaCompress.o \
  $(SDK_C)/Alloc.o \
  $(SDK_C)/LzFind.o \
  $(SDK_C)/LzmaDec.o \
  $(SDK_C)/LzmaEnc.o \
  $(SDK_C)/7zFile.o \
  $(SDK_C)/7zStream.o

include $(MAKEROOT)/Makefiles/app.makefile

