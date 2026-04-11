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
MondoBuild = {MAKEFILE}  # Make blank to avoid rebuilds when makefile is modified
Includes     =
		-i "::h:"
		-i "::support:set:"
SymPPC      = 
ObjDirPPC   = $":Obj:"

PPCCOptions  = {Includes} {SymPPC}  -w off -d MPW -d __STDC__=1 -d USER_ZZSYN

ObjectsPPC  =
		"${ObjDirPPC}automata.c.x"
		"${ObjDirPPC}dlg_a.c.x"
		"${ObjDirPPC}dlg_p.c.x"
		"${ObjDirPPC}err.c.x"
		"${ObjDirPPC}main.c.x"
		"${ObjDirPPC}output.c.x"
		"${ObjDirPPC}relabel.c.x"
		"${ObjDirPPC}support.c.x"
		"${ObjDirPPC}set.c.x"


dlgPPC: ${MondoBuild} ${ObjectsPPC}
	PPCLink
		-o ${Targ} ${SymPPC}
		${ObjectsPPC}
		-t 'MPST'
		-c 'MPS '
		"{SharedLibraries}InterfaceLib"
		"{SharedLibraries}StdCLib"
		"{SharedLibraries}MathLib"
		"{PPCLibraries}StdCRuntime.o"
		"{PPCLibraries}PPCCRuntime.o"
		"{PPCLibraries}PPCToolLibs.o"


"${ObjDirPPC}automata.c.x":  ${MondoBuild} automata.c
	{PPCC} automata.c -o ${Targ} ${PPCCOptions}

"${ObjDirPPC}dlg_a.c.x":  ${MondoBuild} dlg_a.c
	{PPCC} dlg_a.c -o ${Targ} ${PPCCOptions}

"${ObjDirPPC}dlg_p.c.x":  ${MondoBuild} dlg_p.c
	{PPCC} dlg_p.c -o ${Targ} ${PPCCOptions}

"${ObjDirPPC}err.c.x":  ${MondoBuild} err.c
	{PPCC} err.c -o ${Targ} ${PPCCOptions}

"${ObjDirPPC}main.c.x":  ${MondoBuild} main.c
	{PPCC} main.c -o ${Targ} ${PPCCOptions}

"${ObjDirPPC}output.c.x":  ${MondoBuild} output.c
	{PPCC} output.c -o ${Targ} ${PPCCOptions}

"${ObjDirPPC}relabel.c.x":  ${MondoBuild} relabel.c
	{PPCC} relabel.c -o ${Targ} ${PPCCOptions}

"${ObjDirPPC}support.c.x":  ${MondoBuild} support.c
	{PPCC} support.c -o ${Targ} ${PPCCOptions}

"${ObjDirPPC}set.c.x":  ${MondoBuild} "::support:set:set.c"
	{PPCC} "::support:set:set.c" -o ${Targ} ${PPCCOptions}


dlgPPC: dlg.r
	Rez dlg.r -o dlgPPC -a

Install :  dlgPPC
	Duplicate -y dlgPPC "{MPW}"Tools:dlg
