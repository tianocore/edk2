#   Target:     antlrPPC
#   Sources:    ::support:set:set.c
#               antlr.c
#               bits.c
#               build.c
#               egman.c
#               err.c
#               fcache.c
#               fset2.c
#               fset.c
#               gen.c
#               globals.c
#               hash.c
#               lex.c
#               main.c
#               misc.c
#               mrhoist.c
#               pred.c
#               scan.c
#   Created:    Sunday, May 17, 1998 10:24:53 PM
#	Author:		Kenji Tanaka
MAKEFILE     = antlrPPC.make
MondoBuild = {MAKEFILE}  # Make blank to avoid rebuilds when makefile is modified
Includes     =
		-i "::h:"
		-i "::support:set:"
SymPPC      = 
ObjDirPPC   = :Obj:
PPCCOptions  = {Includes} {SymPPC} -w off -d MPW -d __STDC__=1 -d USER_ZZSYN
ObjectsPPC  =
		"{ObjDirPPC}set.c.x"
		"{ObjDirPPC}antlr.c.x"
		"{ObjDirPPC}bits.c.x"
		"{ObjDirPPC}build.c.x"
		"{ObjDirPPC}egman.c.x"
		"{ObjDirPPC}err.c.x"
		"{ObjDirPPC}fcache.c.x"
		"{ObjDirPPC}fset2.c.x"
		"{ObjDirPPC}fset.c.x"
		"{ObjDirPPC}gen.c.x"
		"{ObjDirPPC}globals.c.x"
		"{ObjDirPPC}hash.c.x"
		"{ObjDirPPC}lex.c.x"
		"{ObjDirPPC}main.c.x"
		"{ObjDirPPC}misc.c.x"
		"{ObjDirPPC}mrhoist.c.x"
		"{ObjDirPPC}pred.c.x"
		"{ObjDirPPC}scan.c.x"
antlrPPC: ${MondoBuild} {ObjectsPPC}
	PPCLink
		-o {Targ} {SymPPC}
		{ObjectsPPC}
		-t 'MPST'
		-c 'MPS '
		"{SharedLibraries}InterfaceLib"
		"{SharedLibraries}StdCLib"
		#"{SharedLibraries}MathLib"
		"{PPCLibraries}StdCRuntime.o"
		"{PPCLibraries}PPCCRuntime.o"
		"{PPCLibraries}PPCToolLibs.o"
"{ObjDirPPC}set.c.x" ${MondoBuild} "::support:set:set.c"
	{PPCC} "::support:set:set.c" -o {Targ} {PPCCOptions}
"{ObjDirPPC}antlr.c.x" ${MondoBuild} antlr.c
	{PPCC} antlr.c -o {Targ} {PPCCOptions}
"{ObjDirPPC}bits.c.x" ${MondoBuild} bits.c
	{PPCC} bits.c -o {Targ} {PPCCOptions}
"{ObjDirPPC}build.c.x" ${MondoBuild} build.c
	{PPCC} build.c -o {Targ} {PPCCOptions}
"{ObjDirPPC}egman.c.x" ${MondoBuild} egman.c
	{PPCC} egman.c -o {Targ} {PPCCOptions}
"{ObjDirPPC}err.c.x" ${MondoBuild} err.c
	{PPCC} err.c -o {Targ} {PPCCOptions}
"{ObjDirPPC}fcache.c.x" ${MondoBuild} fcache.c
	{PPCC} fcache.c -o {Targ} {PPCCOptions}
"{ObjDirPPC}fset2.c.x" ${MondoBuild} fset2.c
	{PPCC} fset2.c -o {Targ} {PPCCOptions}
"{ObjDirPPC}fset.c.x" ${MondoBuild} fset.c
	{PPCC} fset.c -o {Targ} {PPCCOptions}
"{ObjDirPPC}gen.c.x" ${MondoBuild} gen.c
	{PPCC} gen.c -o {Targ} {PPCCOptions}
"{ObjDirPPC}globals.c.x" ${MondoBuild} globals.c
	{PPCC} globals.c -o {Targ} {PPCCOptions}
"{ObjDirPPC}hash.c.x" ${MondoBuild} hash.c
	{PPCC} hash.c -o {Targ} {PPCCOptions}
"{ObjDirPPC}lex.c.x" ${MondoBuild} lex.c
	{PPCC} lex.c -o {Targ} {PPCCOptions}
"{ObjDirPPC}main.c.x" ${MondoBuild} main.c
	{PPCC} main.c -o {Targ} {PPCCOptions}
"{ObjDirPPC}misc.c.x" ${MondoBuild} misc.c
	{PPCC} misc.c -o {Targ} {PPCCOptions}
"{ObjDirPPC}mrhoist.c.x" ${MondoBuild} mrhoist.c
	{PPCC} mrhoist.c -o {Targ} {PPCCOptions}
"{ObjDirPPC}pred.c.x" ${MondoBuild} pred.c
	{PPCC} pred.c -o {Targ} {PPCCOptions}
"{ObjDirPPC}scan.c.x" ${MondoBuild} scan.c
	{PPCC} scan.c -o {Targ} {PPCCOptions}

antlrPPC: antlr.r
	Rez antlr.r -o antlrPPC -a
Install: antlrPPC
	Duplicate -y antlrPPC "{MPW}"Tools:antlr
