## @file
# GNU/Linux makefile for 'VfrCompile' module build.
#
# Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

MAKEROOT ?= ..

APPNAME = VfrCompile

LIBS = -lCommon

TOOL_INCLUDE = -I Pccts/h

#OBJECTS = VfrSyntax.o VfrServices.o DLGLexer.o EfiVfrParser.o ATokenBuffer.o DLexerBase.o AParser.o
OBJECTS = AParser.o DLexerBase.o ATokenBuffer.o EfiVfrParser.o VfrLexer.o VfrSyntax.o \
          VfrFormPkg.o VfrError.o VfrUtilityLib.o VfrCompiler.o
ifeq ($(CXX), llvm)
VFR_CPPFLAGS = -Wno-deprecated-register -DPCCTS_USE_NAMESPACE_STD $(BUILD_CPPFLAGS)
else
VFR_CPPFLAGS = -DPCCTS_USE_NAMESPACE_STD $(BUILD_CPPFLAGS)
endif
# keep BUILD_OPTFLAGS last
VFR_CXXFLAGS = $(BUILD_OPTFLAGS)

# keep EXTRA_LDFLAGS last
VFR_LFLAGS = $(EXTRA_LDFLAGS)

LINKER = $(BUILD_CXX)

EXTRA_CLEAN_OBJECTS = EfiVfrParser.cpp EfiVfrParser.h VfrParser.dlg VfrTokens.h VfrLexer.cpp VfrLexer.h VfrSyntax.cpp tokens.h

MAKEROOT ?= ../..

include $(MAKEROOT)/Makefiles/header.makefile

APPLICATION = $(MAKEROOT)/bin/$(APPNAME)

.PHONY:all
all: $(MAKEROOT)/bin $(APPLICATION)

$(APPLICATION): $(OBJECTS)
	$(LINKER) -o $(APPLICATION) $(VFR_LFLAGS) $(OBJECTS) -L$(MAKEROOT)/libs $(LIBS)

VfrCompiler.o: ../Include/Common/BuildVersion.h

include $(MAKEROOT)/Makefiles/footer.makefile

VfrSyntax.cpp EfiVfrParser.cpp EfiVfrParser.h VfrParser.dlg VfrTokens.h: Pccts/antlr/antlr VfrSyntax.g
	Pccts/antlr/antlr -CC -e3 -ck 3 -k 2 -fl VfrParser.dlg -ft VfrTokens.h -o . VfrSyntax.g

VfrLexer.cpp VfrLexer.h: Pccts/dlg/dlg VfrParser.dlg
	Pccts/dlg/dlg -C2 -i -CC -cl VfrLexer -o . VfrParser.dlg

Pccts/antlr/antlr:
	BIN_DIR='.' $(MAKE) -C Pccts/antlr

Pccts/dlg/dlg:
	BIN_DIR='.' $(MAKE) -C Pccts/dlg

ATokenBuffer.o: Pccts/h/ATokenBuffer.cpp
	$(BUILD_CXX) -c $(VFR_CPPFLAGS) $(INC) $(VFR_CXXFLAGS) $? -o $@

DLexerBase.o: Pccts/h/DLexerBase.cpp
	$(BUILD_CXX) -c $(VFR_CPPFLAGS) $(INC) $(VFR_CXXFLAGS) $? -o $@

AParser.o: Pccts/h/AParser.cpp
	$(BUILD_CXX) -c $(VFR_CPPFLAGS) $(INC) $(VFR_CXXFLAGS) $? -o $@

VfrSyntax.o: VfrSyntax.cpp
	$(BUILD_CXX) -c $(VFR_CPPFLAGS) $(INC) $(VFR_CXXFLAGS) $? -o $@

clean: localClean

localClean:
	BIN_DIR='.' $(MAKE) -C Pccts/antlr clean
	BIN_DIR='.' $(MAKE) -C Pccts/dlg clean
	rm -f $(EXTRA_CLEAN_OBJECTS)

