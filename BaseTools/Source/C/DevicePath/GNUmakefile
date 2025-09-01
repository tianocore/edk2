## @file
# GNU/Linux makefile for 'DevicePath' module build.
#
# Copyright (c) 2017 - 2025, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
ARCH ?= IA32
MAKEROOT ?= ..

APPNAME = DevicePath

OBJECTS = DevicePath.o UefiDevicePathLib.o DevicePathFromText.o  DevicePathUtilities.o

include $(MAKEROOT)/Makefiles/app.makefile

ifeq (Windows, $(findstring Windows,$(MAKE_HOST)))
GCCVERSION := $(shell for /f "tokens=1 delims=." %%a in ("$(shell $(CC) -dumpversion)") do echo %%a)
else
GCCVERSION := $(shell $(CC) -dumpversion | awk -F'.' '{print $$1}')
endif
CLANG := $(findstring clang,$(shell $(CC) --version))

ifneq ("$(GCCVERSION)", "5")
ifeq ($(CLANG),)
ifneq ($(DARWIN),Darwin)
# gcc 12 trips over device path handling
CFLAGS += -Wno-error=stringop-overflow
endif
endif
endif

LIBS = -lCommon
ifeq ($(CYGWIN), CYGWIN)
  LIBS += -L/lib/e2fsprogs -luuid
endif

ifeq ($(LINUX), Linux)
ifndef CROSS_LIB_UUID
  LIBS += -luuid
else
  LIBS += -L$(CROSS_LIB_UUID)
endif
endif

