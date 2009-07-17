
ARCH ?= IA32
MAKEROOT ?= ..

APPNAME = VfrCompile

LIBS = -lCommon

TOOL_INCLUDE = -I Pccts/h

#OBJECTS = VfrSyntax.o VfrServices.o DLGLexer.o EfiVfrParser.o ATokenBuffer.o DLexerBase.o AParser.o
OBJECTS = AParser.o DLexerBase.o ATokenBuffer.o EfiVfrParser.o VfrLexer.o VfrSyntax.o \
          VfrFormPkg.o VfrError.o VfrUtilityLib.o VfrCompiler.o

VFR_CPPFLAGS = -DPCCTS_USE_NAMESPACE_STD $(CPPFLAGS)

LINKER = $(CXX)

EXTRA_CLEAN_OBJECTS = EfiVfrParser.cpp EfiVfrParser.h VfrParser.dlg VfrTokens.h VfrLexer.cpp VfrLexer.h VfrSyntax.cpp tokens.h

include $(MAKEROOT)/Makefiles/app.makefile

VfrSyntax.cpp EfiVfrParser.cpp EfiVfrParser.h VfrParser.dlg VfrTokens.h: Pccts/antlr/antlr VfrSyntax.g
	Pccts/antlr/antlr -CC -e3 -ck 3 -k 2 -fl VfrParser.dlg -ft VfrTokens.h -o . VfrSyntax.g

VfrLexer.cpp VfrLexer.h: Pccts/dlg/dlg VfrParser.dlg
	Pccts/dlg/dlg -C2 -i -CC -cl VfrLexer -o . VfrParser.dlg

Pccts/antlr/antlr:
	BIN_DIR='.' make -C Pccts/antlr

Pccts/dlg/dlg:
	BIN_DIR='.' make -C Pccts/dlg

ATokenBuffer.o: Pccts/h/ATokenBuffer.cpp
	$(CXX) -c $(VFR_CPPFLAGS) $(INC) $? -o $@

DLexerBase.o: Pccts/h/DLexerBase.cpp
	$(CXX) -c $(VFR_CPPFLAGS) $(INC) $? -o $@

AParser.o: Pccts/h/AParser.cpp
	$(CXX) -c $(VFR_CPPFLAGS) $(INC) $? -o $@

VfrSyntax.o: VfrSyntax.cpp
	$(CXX) -c $(VFR_CPPFLAGS) $(INC) $? -o $@
	
clean: localClean

localClean:
	BIN_DIR='.' make -C Pccts/antlr clean
	BIN_DIR='.' make -C Pccts/dlg clean
	rm -f $(EXTRA_CLEAN_OBJECTS)

