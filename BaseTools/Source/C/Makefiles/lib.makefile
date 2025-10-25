## @file
# Makefiles
#
# Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

BUILDROOT ?= $(MAKEROOT)
OBJDIR ?= .
OBJECTS := $(addprefix $(OBJDIR)/,$(OBJECTS))

include $(MAKEROOT)/Makefiles/header.makefile

LIBRARY = $(BUILDROOT)/libs/lib$(LIBNAME).a

all: $(BUILDROOT)/libs $(LIBRARY)

include $(MAKEROOT)/Makefiles/footer.makefile
