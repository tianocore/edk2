## @file
# Windows makefile for 'VfrCompile' module build.
#
# Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
!INCLUDE ..\Makefiles\ms.common

CPPFLAGS = $(CPPFLAGS) /WX /D PCCTS_USE_NAMESPACE_STD
APPNAME = VfrCompile

LIBS = $(LIB_PATH)\Common.lib

OBJECTS = AParser.obj DLexerBase.obj ATokenBuffer.obj \
          EfiVfrParser.obj VfrLexer.obj VfrSyntax.obj \
          VfrFormPkg.obj VfrError.obj VfrUtilityLib.obj VfrCompiler.obj

INC = $(INC) -I $(BASE_TOOLS_PATH)\Source\C\VfrCompile\Pccts\h

!INCLUDE ..\Makefiles\ms.app

VfrSyntax.cpp EfiVfrParser.cpp EfiVfrParser.h VfrParser.dlg VfrTokens.h: VfrSyntax.g
	pushd . & cd Pccts & $(MAKE) & popd
	antlr -CC -e3 -ck 3 -k 2 -fl VfrParser.dlg -ft VfrTokens.h -o . VfrSyntax.g
#	pushd . & cd Pccts & $(MAKE) clean

VfrLexer.cpp VfrLexer.h: VfrParser.dlg
	dlg -C2 -i -CC -cl VfrLexer -o . VfrParser.dlg

ATokenBuffer.obj: Pccts\h\ATokenBuffer.cpp
	$(CXX) -c $(CPPFLAGS) $(INC) $? -Fo$@

DLexerBase.obj: Pccts\h\DLexerBase.cpp
	$(CXX) -c $(CPPFLAGS) $(INC) $? -Fo$@

AParser.obj: Pccts\h\AParser.cpp
	$(CXX) -c $(CPPFLAGS) $(INC) $? -Fo$@

EXTRA_CLEAN_OBJECTS = VfrParser.dlg EfiVfrParser.cpp EfiVfrParser.h \
                      VfrLexer.cpp VfrLexer.h \
                      VfrSyntax.cpp VfrTokens.h

clean: localClean
cleanall: localClean localCleanall

localClean:
	-DEL $(EXTRA_CLEAN_OBJECTS)

localCleanall:
	pushd . & cd Pccts & $(MAKE) cleanall & popd
