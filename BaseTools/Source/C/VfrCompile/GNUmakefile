## @file
# GNU/Linux makefile for 'VfrCompile' module build.
#
# Copyright (c) 2008 - 2025, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

MAKEROOT ?= ..

APPNAME = VfrCompile
BUILDROOT ?= $(MAKEROOT)
OBJDIR ?= $(abspath .)

LIBS = -lCommon

TOOL_INCLUDE = -I Pccts/h

OBJECTS = AParser.o DLexerBase.o ATokenBuffer.o EfiVfrParser.o VfrLexer.o VfrSyntax.o \
          VfrFormPkg.o VfrError.o VfrUtilityLib.o VfrCompiler.o
OBJECTS := $(addprefix $(OBJDIR)/,$(OBJECTS))
CLANG := $(findstring clang,$(shell $(CC) --version))
ifneq ($(CLANG),)
VFR_CPPFLAGS = -Wno-deprecated-register -std=c++14 -DPCCTS_USE_NAMESPACE_STD $(CPPFLAGS)
else
VFR_CPPFLAGS = -DPCCTS_USE_NAMESPACE_STD $(CPPFLAGS)
endif
# keep BUILD_OPTFLAGS last
VFR_CXXFLAGS = $(BUILD_OPTFLAGS)

# keep EXTRA_LDFLAGS last
VFR_LFLAGS = $(EXTRA_LDFLAGS)

LINKER = $(CXX)

include $(MAKEROOT)/Makefiles/header.makefile

APPLICATION = $(BUILDROOT)/bin/$(APPNAME)

BIN_DIR=$(OBJDIR)
export BIN_DIR

.PHONY:all
all: $(BUILDROOT)/bin $(APPLICATION)

$(APPLICATION): $(OBJECTS)
	$(LINKER) -o $(APPLICATION) $(VFR_LFLAGS) $(OBJECTS) -L$(BUILDROOT)/libs $(LIBS)
ifeq (Windows, $(findstring Windows,$(OS)))
	$(CP) $(APPLICATION).exe $(BIN_PATH)
endif

VfrCompiler.o: ../Include/Common/BuildVersion.h

include $(MAKEROOT)/Makefiles/footer.makefile

GEN_SYNTAX_FILES := $(addprefix $(OBJDIR)/,VfrSyntax.cpp EfiVfrParser.cpp EfiVfrParser.h VfrParser.dlg VfrTokens.h)
$(GEN_SYNTAX_FILES): $(OBJDIR)/antlr VfrSyntax.g
	$(OBJDIR)/antlr -CC -e3 -ck 3 -k 2 -fl VfrParser.dlg -ft VfrTokens.h -o $(OBJDIR) VfrSyntax.g

GEN_LEXER_FILES := $(addprefix $(OBJDIR)/,VfrLexer.cpp VfrLexer.h)
$(GEN_LEXER_FILES): $(OBJDIR)/dlg $(OBJDIR)/VfrParser.dlg
	$(OBJDIR)/dlg -C2 -i -CC -cl VfrLexer -o $(OBJDIR) $(OBJDIR)/VfrParser.dlg

EXTRA_CLEAN_OBJECTS = $(GEN_SYNTAX_FILES) $(GEN_LEXER_FILES)

$(OBJDIR)/Pccts/antlr $(OBJDIR)/Pccts/dlg:
	$(MD) $@

$(OBJDIR)/antlr: | $(OBJDIR)/Pccts/antlr
	$(MAKE) OBJDIR=$(OBJDIR)/Pccts/antlr -C Pccts/antlr

$(OBJDIR)/dlg: | $(OBJDIR)/Pccts/dlg
	$(MAKE) OBJDIR=$(OBJDIR)/Pccts/dlg -C Pccts/dlg

$(OBJDIR)/ATokenBuffer.o: Pccts/h/ATokenBuffer.cpp
	$(CXX) -c $(VFR_CPPFLAGS) $(INC) $(VFR_CXXFLAGS) $? -o $@

$(OBJDIR)/DLexerBase.o: Pccts/h/DLexerBase.cpp
	$(CXX) -c $(VFR_CPPFLAGS) $(INC) $(VFR_CXXFLAGS) $? -o $@

$(OBJDIR)/AParser.o: Pccts/h/AParser.cpp
	$(CXX) -c $(VFR_CPPFLAGS) $(INC) $(VFR_CXXFLAGS) $? -o $@

$(OBJDIR)/VfrSyntax.o: $(OBJDIR)/VfrSyntax.cpp
	$(CXX) -c $(VFR_CPPFLAGS) $(INC) $(VFR_CXXFLAGS) $? -o $@

clean: localClean

localClean:
	$(MAKE) OBJDIR=$(OBJDIR)/Pccts/antlr -C Pccts/antlr clean
	$(MAKE) OBJDIR=$(OBJDIR)/Pccts/dlg -C Pccts/dlg clean
	$(RM) $(EXTRA_CLEAN_OBJECTS)
ifeq (Windows, $(findstring Windows,$(OS)))
	$(RM) $(BIN_PATH)/$(APPNAME).exe
endif
