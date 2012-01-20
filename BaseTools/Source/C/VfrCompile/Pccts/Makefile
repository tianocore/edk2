#
# Main makefile for PCCTS 1.33MR33	/* MRXXX */
#
pccts:
	pushd . & cd antlr & $(MAKE) /nologo /f AntlrMS.mak & popd
	pushd . & cd dlg & $(MAKE) /nologo /f DlgMS.mak & popd

clean:
	pushd . & cd antlr & $(MAKE) /nologo /f AntlrMS.mak clean & popd
	pushd . & cd dlg & $(MAKE) /nologo /f DlgMS.mak clean & popd

cleanall:
	pushd . & cd antlr & $(MAKE) /nologo /f AntlrMS.mak cleanall & popd
	pushd . & cd dlg & $(MAKE) /nologo /f DlgMS.mak cleanall & popd

