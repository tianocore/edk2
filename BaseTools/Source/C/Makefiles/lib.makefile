## @file
# Makefiles
#
# Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

include $(MAKEROOT)/Makefiles/header.makefile

LIBRARY = $(MAKEROOT)/libs/lib$(LIBNAME).a

all: $(MAKEROOT)/libs $(LIBRARY)

include $(MAKEROOT)/Makefiles/footer.makefile
