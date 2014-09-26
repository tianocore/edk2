#   File:       dlgPPC.make
#   Target:     dlgPPC
#   Sources:    automata.c
#               dlg_a.c
#               dlg_p.c
#               err.c
#               main.c
#               output.c
#               relabel.c
#               support.c
#               ::support:set:set.c
#   Created:    Sunday, May 17, 1998 11:34:20 PM
#	Author:		Kenji Tanaka


MAKEFILE     = dlgPPC.make
¥MondoBuild¥ = {MAKEFILE}  # Make blank to avoid rebuilds when makefile is modified
Includes     = ¶
		-i "::h:" ¶
		-i "::support:set:"
Sym¥PPC      = 
ObjDir¥PPC   = ":Obj:"

PPCCOptions  = {Includes} {Sym¥PPC}  -w off -d MPW -d __STDC__=1 -d USER_ZZSYN

Objects¥PPC  = ¶
		"{ObjDir¥PPC}automata.c.x" ¶
		"{ObjDir¥PPC}dlg_a.c.x" ¶
		"{ObjDir¥PPC}dlg_p.c.x" ¶
		"{ObjDir¥PPC}err.c.x" ¶
		"{ObjDir¥PPC}main.c.x" ¶
		"{ObjDir¥PPC}output.c.x" ¶
		"{ObjDir¥PPC}relabel.c.x" ¶
		"{ObjDir¥PPC}support.c.x" ¶
		"{ObjDir¥PPC}set.c.x"


dlgPPC ÄÄ {¥MondoBuild¥} {Objects¥PPC}
	PPCLink ¶
		-o {Targ} {Sym¥PPC} ¶
		{Objects¥PPC} ¶
		-t 'MPST' ¶
		-c 'MPS ' ¶
		"{SharedLibraries}InterfaceLib" ¶
		"{SharedLibraries}StdCLib" ¶
		"{SharedLibraries}MathLib" ¶
		"{PPCLibraries}StdCRuntime.o" ¶
		"{PPCLibraries}PPCCRuntime.o" ¶
		"{PPCLibraries}PPCToolLibs.o"


"{ObjDir¥PPC}automata.c.x" Ä {¥MondoBuild¥} automata.c
	{PPCC} automata.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}dlg_a.c.x" Ä {¥MondoBuild¥} dlg_a.c
	{PPCC} dlg_a.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}dlg_p.c.x" Ä {¥MondoBuild¥} dlg_p.c
	{PPCC} dlg_p.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}err.c.x" Ä {¥MondoBuild¥} err.c
	{PPCC} err.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}main.c.x" Ä {¥MondoBuild¥} main.c
	{PPCC} main.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}output.c.x" Ä {¥MondoBuild¥} output.c
	{PPCC} output.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}relabel.c.x" Ä {¥MondoBuild¥} relabel.c
	{PPCC} relabel.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}support.c.x" Ä {¥MondoBuild¥} support.c
	{PPCC} support.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}set.c.x" Ä {¥MondoBuild¥} "::support:set:set.c"
	{PPCC} "::support:set:set.c" -o {Targ} {PPCCOptions}


dlgPPC ÄÄ dlg.r
	Rez dlg.r -o dlgPPC -a

Install  Ä dlgPPC
	Duplicate -y dlgPPC "{MPW}"Tools:dlg
