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
¥MondoBuild¥ = {MAKEFILE}  # Make blank to avoid rebuilds when makefile is modified
Includes     = ¶
		-i "::h:" ¶
		-i "::support:set:"
Sym¥PPC      = 
ObjDir¥PPC   = :Obj:
PPCCOptions  = {Includes} {Sym¥PPC} -w off -d MPW -d __STDC__=1 -d USER_ZZSYN
Objects¥PPC  = ¶
		"{ObjDir¥PPC}set.c.x" ¶
		"{ObjDir¥PPC}antlr.c.x" ¶
		"{ObjDir¥PPC}bits.c.x" ¶
		"{ObjDir¥PPC}build.c.x" ¶
		"{ObjDir¥PPC}egman.c.x" ¶
		"{ObjDir¥PPC}err.c.x" ¶
		"{ObjDir¥PPC}fcache.c.x" ¶
		"{ObjDir¥PPC}fset2.c.x" ¶
		"{ObjDir¥PPC}fset.c.x" ¶
		"{ObjDir¥PPC}gen.c.x" ¶
		"{ObjDir¥PPC}globals.c.x" ¶
		"{ObjDir¥PPC}hash.c.x" ¶
		"{ObjDir¥PPC}lex.c.x" ¶
		"{ObjDir¥PPC}main.c.x" ¶
		"{ObjDir¥PPC}misc.c.x" ¶
		"{ObjDir¥PPC}mrhoist.c.x" ¶
		"{ObjDir¥PPC}pred.c.x" ¶
		"{ObjDir¥PPC}scan.c.x"
antlrPPC ÄÄ {¥MondoBuild¥} {Objects¥PPC}
	PPCLink ¶
		-o {Targ} {Sym¥PPC} ¶
		{Objects¥PPC} ¶
		-t 'MPST' ¶
		-c 'MPS ' ¶
		"{SharedLibraries}InterfaceLib" ¶
		"{SharedLibraries}StdCLib" ¶
		#"{SharedLibraries}MathLib" ¶
		"{PPCLibraries}StdCRuntime.o" ¶
		"{PPCLibraries}PPCCRuntime.o" ¶
		"{PPCLibraries}PPCToolLibs.o"
"{ObjDir¥PPC}set.c.x" Ä {¥MondoBuild¥} "::support:set:set.c"
	{PPCC} "::support:set:set.c" -o {Targ} {PPCCOptions}
"{ObjDir¥PPC}antlr.c.x" Ä {¥MondoBuild¥} antlr.c
	{PPCC} antlr.c -o {Targ} {PPCCOptions}
"{ObjDir¥PPC}bits.c.x" Ä {¥MondoBuild¥} bits.c
	{PPCC} bits.c -o {Targ} {PPCCOptions}
"{ObjDir¥PPC}build.c.x" Ä {¥MondoBuild¥} build.c
	{PPCC} build.c -o {Targ} {PPCCOptions}
"{ObjDir¥PPC}egman.c.x" Ä {¥MondoBuild¥} egman.c
	{PPCC} egman.c -o {Targ} {PPCCOptions}
"{ObjDir¥PPC}err.c.x" Ä {¥MondoBuild¥} err.c
	{PPCC} err.c -o {Targ} {PPCCOptions}
"{ObjDir¥PPC}fcache.c.x" Ä {¥MondoBuild¥} fcache.c
	{PPCC} fcache.c -o {Targ} {PPCCOptions}
"{ObjDir¥PPC}fset2.c.x" Ä {¥MondoBuild¥} fset2.c
	{PPCC} fset2.c -o {Targ} {PPCCOptions}
"{ObjDir¥PPC}fset.c.x" Ä {¥MondoBuild¥} fset.c
	{PPCC} fset.c -o {Targ} {PPCCOptions}
"{ObjDir¥PPC}gen.c.x" Ä {¥MondoBuild¥} gen.c
	{PPCC} gen.c -o {Targ} {PPCCOptions}
"{ObjDir¥PPC}globals.c.x" Ä {¥MondoBuild¥} globals.c
	{PPCC} globals.c -o {Targ} {PPCCOptions}
"{ObjDir¥PPC}hash.c.x" Ä {¥MondoBuild¥} hash.c
	{PPCC} hash.c -o {Targ} {PPCCOptions}
"{ObjDir¥PPC}lex.c.x" Ä {¥MondoBuild¥} lex.c
	{PPCC} lex.c -o {Targ} {PPCCOptions}
"{ObjDir¥PPC}main.c.x" Ä {¥MondoBuild¥} main.c
	{PPCC} main.c -o {Targ} {PPCCOptions}
"{ObjDir¥PPC}misc.c.x" Ä {¥MondoBuild¥} misc.c
	{PPCC} misc.c -o {Targ} {PPCCOptions}
"{ObjDir¥PPC}mrhoist.c.x" Ä {¥MondoBuild¥} mrhoist.c
	{PPCC} mrhoist.c -o {Targ} {PPCCOptions}
"{ObjDir¥PPC}pred.c.x" Ä {¥MondoBuild¥} pred.c
	{PPCC} pred.c -o {Targ} {PPCCOptions}
"{ObjDir¥PPC}scan.c.x" Ä {¥MondoBuild¥} scan.c
	{PPCC} scan.c -o {Targ} {PPCCOptions}

antlrPPC ÄÄ antlr.r
	Rez antlr.r -o antlrPPC -a
Install  Ä antlrPPC
	Duplicate -y antlrPPC "{MPW}"Tools:antlr
