## @file
# GNU/Linux makefile for 'VfrCompile' module build.
#
# Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

MAKEROOT ?= ..

APPNAME = VfrCompile

LIBS = -lCommon

TOOL_INCLUDE = -I $(MAKEROOT)/$(APPNAME)/Pccts/h

#OBJECTS = VfrSyntax.o VfrServices.o DLGLexer.o EfiVfrParser.o ATokenBuffer.o DLexerBase.o AParser.o
OBJECTS = AParser.o DLexerBase.o ATokenBuffer.o EfiVfrParser.o VfrLexer.o VfrSyntax.o \
          VfrFormPkg.o VfrError.o VfrUtilityLib.o VfrCompiler.o
CLANG:=$(shell $(CC) --version | grep clang)
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

EXTRA_CLEAN_OBJECTS = EfiVfrParser.cpp EfiVfrParser.h VfrParser.dlg VfrTokens.h VfrLexer.cpp VfrLexer.h VfrSyntax.cpp tokens.h

MAKEROOT ?= ../..

include $(MAKEROOT)/Makefiles/header.makefile

APPLICATION = $(MAKEROOT)/bin/$(APPNAME)

override NINJA_CSRCS :=
override NINJA_CXXSRCS :=

ANTLR_OBJECTS = \
  $(NINJA_DIR)/Pccts/antlr/antlr.o \
  $(NINJA_DIR)/Pccts/antlr/scan.o \
  $(NINJA_DIR)/Pccts/antlr/err.o \
  $(NINJA_DIR)/Pccts/antlr/bits.o \
  $(NINJA_DIR)/Pccts/antlr/build.o \
  $(NINJA_DIR)/Pccts/antlr/fset2.o \
  $(NINJA_DIR)/Pccts/antlr/fset.o \
  $(NINJA_DIR)/Pccts/antlr/gen.o \
  $(NINJA_DIR)/Pccts/antlr/globals.o \
  $(NINJA_DIR)/Pccts/antlr/hash.o \
  $(NINJA_DIR)/Pccts/antlr/lex.o \
  $(NINJA_DIR)/Pccts/antlr/main.o \
  $(NINJA_DIR)/Pccts/antlr/misc.o \
  $(NINJA_DIR)/Pccts/antlr/pred.o \
  $(NINJA_DIR)/Pccts/antlr/egman.o \
  $(NINJA_DIR)/Pccts/antlr/mrhoist.o \
  $(NINJA_DIR)/Pccts/antlr/fcache.o

DLG_OBJECTS = \
  $(NINJA_DIR)/Pccts/dlg/dlg_p.o \
  $(NINJA_DIR)/Pccts/dlg/dlg_a.o \
  $(NINJA_DIR)/Pccts/dlg/main.o \
  $(NINJA_DIR)/Pccts/dlg/err.o \
  $(NINJA_DIR)/Pccts/dlg/support.o \
  $(NINJA_DIR)/Pccts/dlg/output.o \
  $(NINJA_DIR)/Pccts/dlg/relabel.o \
  $(NINJA_DIR)/Pccts/dlg/automata.o

ANTLR_BUILDS := $(foreach file,$(ANTLR_OBJECTS),"build $(file): cc_tools_$(NINJA_ID) $(subst .o,,$(file)).c")
DLG_BUILDS := $(foreach file,$(DLG_OBJECTS),"build $(file): cc_tools_$(NINJA_ID) $(subst .o,,$(file)).c")

.PHONY:all ninja
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
	$(CXX) -c $(VFR_CPPFLAGS) $(INC) $(VFR_CXXFLAGS) $? -o $@

DLexerBase.o: Pccts/h/DLexerBase.cpp
	$(CXX) -c $(VFR_CPPFLAGS) $(INC) $(VFR_CXXFLAGS) $? -o $@

AParser.o: Pccts/h/AParser.cpp
	$(CXX) -c $(VFR_CPPFLAGS) $(INC) $(VFR_CXXFLAGS) $? -o $@

VfrSyntax.o: VfrSyntax.cpp
	$(CXX) -c $(VFR_CPPFLAGS) $(INC) $(VFR_CXXFLAGS) $? -o $@


# The source code licence for Pccts is not clearly defined. I'm not sure
#   if editing its makefiles is allowed. The build is invoked without
#   automatically generating a ninja file.
ninja_antlr_tools:
	@echo "rule link_tools_$(NINJA_ID)" > build.ninja
	@echo "  pool = link_pool" >> build.ninja
	@echo "  command = $(LINKER) $(LDFLAGS) -o \044out \044in" >> build.ninja
	@echo "" >> build.ninja
	@echo "rule cc_tools_$(NINJA_ID)" >> build.ninja
	@echo "  deps = gcc" >> build.ninja
	@echo "  depfile = \044out.d" >> build.ninja
	@echo "  command = $(CC) -O -I$(NINJA_DIR)/Pccts/dlg -I$(NINJA_DIR)/Pccts/support/set -I$(NINJA_DIR)/Pccts/h -DUSER_ZZSYN -DZZLEXBUFSIZE=65536 -c \044in -o \044out" >> build.ninja
	@echo "" >> build.ninja
	@printf "%s\n" $(ANTLR_BUILDS) >> build.ninja
	@printf "%s\n" $(DLG_BUILDS) >> build.ninja
	@echo "build $(NINJA_DIR)/Pccts/support/set/set.o: cc_tools_$(NINJA_ID) $(NINJA_DIR)/Pccts/support/set/set.c" >> build.ninja
	@echo "build $(NINJA_DIR)/Pccts/antlr/antlr: link_tools_$(NINJA_ID) $(NINJA_DIR)/Pccts/support/set/set.o $(ANTLR_OBJECTS)" >> build.ninja
	@echo "build $(NINJA_DIR)/Pccts/dlg/dlg: link_tools_$(NINJA_ID) $(NINJA_DIR)/Pccts/support/set/set.o $(DLG_OBJECTS)" >> build.ninja

ninja_app: ninja_antlr_tools
	@echo "" >> build.ninja
	@echo "rule antlr_$(NINJA_ID)" >> build.ninja
	@echo "  depfile = \044out.d" >> build.ninja
	@echo "  command = $(NINJA_DIR)/Pccts/antlr/antlr -CC -e3 -ck 3 -k 2 -fl VfrParser.dlg -ft VfrTokens.h -o $(NINJA_DIR)/ \044in" >> build.ninja
	@echo "" >> build.ninja
	@echo "rule dlg_$(NINJA_ID)" >> build.ninja
	@echo "  depfile = \044out.d" >> build.ninja
	@echo "  command = $(NINJA_DIR)/Pccts/dlg/dlg -C2 -i -CC -cl VfrLexer -o $(NINJA_DIR)/ \044in" >> build.ninja
	@echo "" >> build.ninja
	@echo "rule cxx_vfr_$(NINJA_ID)" >> build.ninja
	@echo "  deps = gcc" >> build.ninja
	@echo "  depfile = \044out.d" >> build.ninja
	@echo "  command = $(CXX) -c $(VFR_CPPFLAGS) $(INC) $(VFR_CXXFLAGS) \044in -o \044out" >> build.ninja
	@echo "" >> build.ninja
	@echo "build $(NINJA_DIR)/AParser.o: cxx_vfr_$(NINJA_ID) $(NINJA_DIR)/Pccts/h/AParser.cpp" >> build.ninja
	@echo "build $(NINJA_DIR)/DLexerBase.o: cxx_vfr_$(NINJA_ID) $(NINJA_DIR)/Pccts/h/DLexerBase.cpp" >> build.ninja
	@echo "build $(NINJA_DIR)/ATokenBuffer.o: cxx_vfr_$(NINJA_ID) $(NINJA_DIR)/Pccts/h/ATokenBuffer.cpp" >> build.ninja
	@echo "build $(NINJA_DIR)/EfiVfrParser.cpp $(NINJA_DIR)/VfrParser.dlg: antlr_$(NINJA_ID) $(NINJA_DIR)/VfrSyntax.g || $(NINJA_DIR)/AParser.o $(NINJA_DIR)/DLexerBase.o $(NINJA_DIR)/ATokenBuffer.o $(NINJA_DIR)/Pccts/antlr/antlr" >> build.ninja
	@echo "build $(NINJA_DIR)/EfiVfrParser.o: cxx_vfr_$(NINJA_ID) $(NINJA_DIR)/EfiVfrParser.cpp || $(NINJA_DIR)/EfiVfrParser.cpp" >> build.ninja
	@echo "build $(NINJA_DIR)/VfrSyntax.cpp $(NINJA_DIR)/VfrLexer.cpp $(NINJA_DIR)/VfrFormPkg.cpp $(NINJA_DIR)/VfrError.cpp $(NINJA_DIR)/VfrUtilityLib.cpp $(NINJA_DIR)/VfrCompiler.cpp: dlg_$(NINJA_ID) $(NINJA_DIR)/VfrParser.dlg || $(NINJA_DIR)/EfiVfrParser.o $(NINJA_DIR)/Pccts/dlg/dlg" >> build.ninja
	@echo "build $(NINJA_DIR)/VfrLexer.o: cxx_vfr_$(NINJA_ID) $(NINJA_DIR)/VfrLexer.cpp || $(NINJA_DIR)/EfiVfrParser.o" >> build.ninja
	@echo "build $(NINJA_DIR)/VfrSyntax.o: cxx_vfr_$(NINJA_ID) $(NINJA_DIR)/VfrSyntax.cpp || $(NINJA_DIR)/EfiVfrParser.o" >> build.ninja
	@echo "build $(NINJA_DIR)/VfrFormPkg.o: cxx_vfr_$(NINJA_ID) $(NINJA_DIR)/VfrFormPkg.cpp || $(NINJA_DIR)/EfiVfrParser.o" >> build.ninja
	@echo "build $(NINJA_DIR)/VfrError.o: cxx_vfr_$(NINJA_ID) $(NINJA_DIR)/VfrError.cpp || $(NINJA_DIR)/EfiVfrParser.o" >> build.ninja
	@echo "build $(NINJA_DIR)/VfrUtilityLib.o: cxx_vfr_$(NINJA_ID) $(NINJA_DIR)/VfrUtilityLib.cpp || $(NINJA_DIR)/EfiVfrParser.o" >> build.ninja
	@echo "build $(NINJA_DIR)/VfrCompiler.o: cxx_vfr_$(NINJA_ID) $(NINJA_DIR)/VfrCompiler.cpp || $(NINJA_DIR)/EfiVfrParser.o" >> build.ninja
	@echo "" >> build.ninja
	@echo "rule link_$(NINJA_ID)" >> build.ninja
	@echo "  pool = link_pool" >> build.ninja
	@echo "  command = $(LINKER) -o \044out $(LDFLAGS) \044in -L$(MAKEROOT)/libs $(LIBS)" >> build.ninja
	@echo "" >> build.ninja
	@echo "build $(APPLICATION): link_$(NINJA_ID) $(NINJA_OBJECTS) || $(MAKEROOT)/libs/libCommon.a" >> build.ninja

clean: localClean

localClean:
	BIN_DIR='.' $(MAKE) -C Pccts/antlr clean
	BIN_DIR='.' $(MAKE) -C Pccts/dlg clean
	rm -f $(EXTRA_CLEAN_OBJECTS)

