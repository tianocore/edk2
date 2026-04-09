/*
	File:		antlrMPW.r
	Target:		antlr 133MR
	Created:    Monday, June 15, 1998 4:41:11 AM
	Author:		Kenji Tanaka (kentar@osa.att.ne.jp)
*/

#include "cmdo.r"

resource 'cmdo' (128, "Antlr") {
	{	/* array dialogs: 5 elements */
		/* [1] */
		295,
		"ANTLR -- Purdue Compiler Construction To"
		"ol Set (PCCTS) LL(k) parser generator.",
		{	/* array itemArray: 12 elements */
			/* [1] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{18, 23, 33, 223},
				"Read grammar from stdin",
				"-",
				"Read grammar from stdin."
			},
			/* [2] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{38, 23, 53, 310},
				"Send grammar.c/grammar.cpp to stdout",
				"-stdout",
				"Send grammar.c/grammar.cpp to stdout."
			},
			/* [3] */
			NotDependent {

			},
			MultiFiles {
				"Grammar File(s)É",
				"Choose the grammar specification files y"
				"ou wish to have ANTLR process.",
				{79, 22, 98, 152},
				"Grammar specification:",
				"",
				MultiInputFiles {
					{	/* array MultiTypesArray: 1 elements */
						/* [1] */
						text
					},
					".g",
					"Files ending in .g",
					"All text files"
				}
			},
			/* [4] */
			NotDependent {

			},
			Files {
				DirOnly,
				OptionalFile {
					{58, 168, 74, 298},
					{79, 169, 98, 299},
					"Output Directory",
					":",
					"-o",
					"",
					"Choose the directory where ANTLR will pu"
					"t its output.",
					dim,
					"Output DirectoryÉ",
					"",
					""
				},
				NoMore {

				}
			},
			/* [5] */
			NotDependent {

			},
			Redirection {
				StandardOutput,
				{126, 27}
			},
			/* [6] */
			NotDependent {

			},
			Redirection {
				DiagnosticOutput,
				{126, 178}
			},
			/* [7] */
			NotDependent {

			},
			TextBox {
				gray,
				{117, 20, 167, 300},
				"Redirection"
			},
			/* [8] */
			NotDependent {

			},
			NestedDialog {
				5,
				{20, 324, 40, 460},
				"Parse OptionsÉ",
				"Parse control options may be set with th"
				"is button."
			},
			/* [9] */
			NotDependent {

			},
			NestedDialog {
				2,
				{50, 324, 70, 460},
				"Generate OptionsÉ",
				"Various command line options may be set "
				"with this button."
			},
			/* [10] */
			NotDependent {

			},
			NestedDialog {
				3,
				{78, 324, 98, 460},
				"More OptionsÉ",
				"Antlr has ALOT of options. There are eve"
				"n more to be found with this button."
			},
			/* [11] */
			NotDependent {

			},
			NestedDialog {
				4,
				{106, 324, 126, 460},
				"Rename OptionsÉ",
				"Options for renaming output files may be"
				" set with this button."
			},
			/* [12] */
			NotDependent {

			},
			VersionDialog {
				VersionString {
					"1.33MR"
				},
				"PCCTS was written by Terence Parr, Russe"
				"ll Quong, Will Cohen, and Hank Dietz: 19"
				"89-1998. MPW port by Scott Haney.",
				noDialog
			}
		},
		/* [2] */
		295,
		"Use this dialog to specify command line "
		"Generate Options.",
		{	/* array itemArray: 15 elements */
			/* [1] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{18, 25, 33, 225},
				"Generate C++ code",
				"-CC",
				"Generate C++ output from both ANTLR and "
				"DLG."
			},
			/* [2] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{38, 25, 53, 225},
				"Generate ASTs",
				"-gt",
				"Generate code for Abstract-Syntax-Trees "
				"(ASTs)."
			},
			/* [3] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{58, 25, 73, 225},
				"Generate line info",
				"-gl",
				"If this option is checked, ANTLR will ge"
				"nerate line info about grammaractions, t"
				"hereby making debugging easier since com"
				"pile errors will point to the grammar fi"
				"le."
			},
			/* [4] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{78, 25, 93, 225},
				"Generate error classes",
				"-ge",
				"If this option is checked, ANTLR will ge"
				"nerate an error class foreach non-termin"
				"al."
			},
			/* [5] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{98, 25, 113, 225},
				"Don't generate Code",
				"-gc",
				"If this option is checked, ANTLR will ge"
				"nerate no code, i.e. it will only perfor"
				"m analysis on the grammar."
			},
			/* [6] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{118, 25, 133, 225},
				"Delay lookahead fetches",
				"-gk",
				"If this option is checked, ANTLR will ge"
				"nerate a parser that delays lookahead fe"
				"tches until needed."
			},
			/* [7] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{138, 25, 153, 225},
				"Use newAST(...)",
				"-newAST",
				"In C++ mode use \"newAST(...)\" rather tha"
				"n \"new AST(...)\""
			},
			/* [8] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{18, 235, 33, 435},
				"Support parse traces",
				"-gd",
				"If this option is checked, ANTLR inserts"
				" code in each parsing function to provid"
				"e for user-defined handling of a detaile"
				"d parse trace. The code consists of call"
				"s to zzTRACEIN and zzTRACEOUT."
			},
			/* [9] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{38, 235, 53, 435},
				"Generate cross-references",
				"-cr",
				"If this option is checked, ANTLR will ge"
				"nerate a cross reference for all rules. "
				"For each rule it will print a list of al"
				"l other rules that reference it."
			},
			/* [10] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{58, 235, 73, 435},
				"Don't create Lexer files",
				"-gx",
				"If this option is checked, ANTLR will no"
				"t generate DLG-related output files. Thi"
				"s option should be used if one wants a c"
				"ustom lexical analyzer or if one has mad"
				"e changes to the grammar not affecting t"
				"he lexical structure."
			},
			/* [11] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{78, 235, 93, 460},
				"Don't generate token expr sets",
				"-gs",
				"If this option is checked, ANTLR will no"
				"t generate sets for token expression set"
				"s; instead, it will generate a || separa"
				"ted sequence of LA(1)==token #. "
			},
			/* [12] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{98, 235, 113, 460},
				"Generate ANSI-compatible",
				"-ga",
				"Generate ANSI-compatible code (default=F"
				"ALSE)"
			},
			/* [13] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{118, 235, 133, 460},
				"Don't generate tokens.h",
				"-gxt",
				"Do not generate tokens.h (default=FALSE)"
			},
			/* [13] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{138, 235, 153, 460},
				"Provide \"(alpha)? beta\" info",
				"-alpha",
				"Provide additional information for \"(alpha)? beta\" error messages"
			},
			/* [14] */
			NotDependent {

			},
			RegularEntry {
				"Tabs(1 to 8):",
				{162, 23, 177, 117},
				{163, 125, 179, 196},
				"",
				keepCase,
				"-tab",
				"Width of tabs (1 to 8) for grammar.c/gra"
				"mmar.cpp files."
			},
			/* [15] */
			NotDependent {

			},
			RegularEntry {
				"Function Prefix:",
				{161, 236, 177, 342},
				{162, 345, 177, 454},
				"",
				keepCase,
				"-gp",
				"Prefix all generated rule functions with"
				" a string."
			}
		},
		/* [3] */
		295,
		"Use this dialog to specify still more co"
		"mmand line options.",
		{	/* array itemArray: 12 elements */
			/* [1] */
			NotDependent {

			},
			RadioButtons {
				{	/* array radioArray: 3 elements */
					/* [1] */
					{38, 25, 53, 85}, "None", "", Set, "When this option is selected, ANTLR will"
					" not print the grammar to stdout.",
					/* [2] */
					{38, 100, 53, 160}, "Yes", "-p", NotSet, "When this option is selected, ANTLR will"
					" print the grammar, stripped of all acti"
					"ons and comments, to stdout.",
					/* [3] */
					{38, 175, 53, 235}, "More", "-pa", NotSet, "When this option is selected, ANTLR will"
					" print the grammar, stripped of all acti"
					"ons and comments, to stdout. It will als"
					"o annotate the output with the first set"
					"s determined from grammar analysis."
				}
			},
			/* [2] */
			NotDependent {

			},
			TextBox {
				gray,
				{28, 15, 60, 250},
				"Grammar Printing"
			},
			/* [3] */
			NotDependent {

			},
			RadioButtons {
				{	/* array radioArray: 3 elements */
					/* [1] */
					{88, 25, 103, 85}, "Low", "", Set, "When this option is selected, ANTLR will"
					" show ambiguities/errors in low detail.",
					/* [2] */
					{88, 100, 103, 160}, "Medium", "-e2", NotSet, "When this option is selected, ANTLR will"
					" show ambiguities/errors in more detail.",
					/* [3] */
					{88, 175, 103, 235}, "High", "-e3", NotSet, "When this option is selected, ANTLR will"
					" show ambiguities/errors in excruciating"
					" detail."
				}
			},
			/* [4] */
			NotDependent {

			},
			TextBox {
				gray,
				{78, 15, 110, 250},
				"Error reporting"
			},
			/* [5] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{130, 22, 145, 222},
				"More warnings",
				"-w2",
				"If this option is checked, ANTLR will wa"
				"rn if semantic predicates and/or (É)? bl"
				"ocks are assumed to cover ambiguous alte"
				"rnatives."
			},
			/* [6] */
			NotDependent {

			},
			RegularEntry {
				"Report when tnode usage exceeds:",
				{162, 23, 180, 253},
				{162, 255, 178, 326},
				"",
				keepCase,
				"-treport",
				"Report when tnode usage exceeds value du"
				"ring ambiguity resolution."
			},
			/* [7] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{40, 292, 55, 431},
				"Predicate",
				"-info p",
				"With the antlr \"-info p\" switch the user"
				" will receive information about the pred"
				"icate suppression in the generated file."
			},
			/* [8] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{60, 292, 75, 430},
				"Tree Nodes",
				"-info t",
				"Using \"-info t\" gives information about "
				"the total number of tnodes created and t"
				"he peak number of tnodes."
			},
			/* [9] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{80, 292, 95, 425},
				"First/follow",
				"-info f",
				"first/follow set information."
			},
			/* [10] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{100, 292, 115, 425},
				"Monitor progress",
				"-info m",
				"prints name of each rule as it is starte"
				"d and flushes output at start of each rule."
			},
			/* [11] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{120, 292, 135, 416},
				"Orphan rules",
				"-info o",
				"If there is more than one rule which is "
				"not referenced by any other rule then al"
				"l such rules are listed."
			},
			/* [12] */
			NotDependent {

			},
			TextBox {
				gray,
				{28, 279, 147, 451},
				"Extra info"
			}
		},
		/* [4] */
		295,
		"Use this dialog to specify command line "
		"options relating to renaming output file"
		"s.",
		{	/* array itemArray: 7 elements */
			/* [1] */
			NotDependent {

			},
			RegularEntry {
				"Errors file name:",
				{35, 25, 50, 205},
				{35, 205, 51, 300},
				"err.c",
				keepCase,
				"-fe",
				"This entry specifies the name ANTLR uses"
				" for the errors file."
			},
			/* [2] */
			NotDependent {

			},
			RegularEntry {
				"Lexical output name:",
				{60, 25, 75, 205},
				{60, 205, 76, 300},
				"parser.dlg",
				keepCase,
				"-fl",
				"This entry specifies the name ANTLR uses"
				" for the lexical output file."
			},
			/* [3] */
			NotDependent {

			},
			RegularEntry {
				"Lexical modes name:",
				{85, 25, 100, 205},
				{85, 205, 101, 300},
				"mode.h",
				keepCase,
				"-fm",
				"This entry specifies the name ANTLR uses"
				" for the lexical mode definitions file."
			},
			/* [4] */
			NotDependent {

			},
			RegularEntry {
				"Remap file name:",
				{110, 25, 125, 205},
				{110, 205, 126, 300},
				"remap.h",
				keepCase,
				"-fr",
				"This entry specifies the name ANTLR uses"
				" for the file that remaps globally visib"
				"le symbols."
			},
			/* [5] */
			NotDependent {

			},
			RegularEntry {
				"Tokens file name:",
				{135, 25, 150, 205},
				{135, 205, 151, 300},
				"tokens.h",
				keepCase,
				"-ft",
				"This entry specifies the name ANTLR uses"
				" for the tokens file."
			},
			/* [6] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{160, 25, 175, 175},
				"Create std header",
				"-gh",
				"If this option is checked, ANTLR will cr"
				"eate a standard header file named, by de"
				"fault 'stdpccts.h'. This name can be alt"
				"ered using the entry right next door."
			},
			/* [7] */
			Or {
				{	/* array OrArray: 1 elements */
					/* [1] */
					6
				}
			},
			RegularEntry {
				"Std header file name:",
				{160, 175, 175, 355},
				{160, 355, 176, 450},
				"stdpccts.h",
				keepCase,
				"-fh",
				"This entry specifies the name ANTLR uses"
				" for the standard header file."
			}
		},
		/* [5] */
		295,
		"Use this dialog to specify parse options"
		".",
		{	/* array itemArray: 9 elements */
			/* [1] */
			NotDependent {

			},
			RegularEntry {
				"Lookahead:",
				{23, 27, 38, 152},
				{46, 29, 62, 154},
				"1",
				keepCase,
				"-k",
				"This entry specifies the number of token"
				"s of lookahead."
			},
			/* [2] */
			NotDependent {

			},
			RegularEntry {
				"Compr lookahead:",
				{22, 167, 37, 292},
				{46, 172, 62, 297},
				"",
				keepCase,
				"-ck",
				"This entry specifies the number of token"
				"s of lookahead when using compressed (li"
				"near approximation) lookahead. In genera"
				"l, the compressed lookahead is much deep"
				"er than the full lookahead."
			},
			/* [3] */
			NotDependent {

			},
			RegularEntry {
				"Max tree nodes:",
				{22, 312, 37, 437},
				{46, 315, 62, 445},
				"",
				keepCase,
				"-rl",
				"This entry specifies the maximum number "
				"of tokens of tree nodes used by the gram"
				"mar analysis."
			},
			/* [4] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{76, 25, 91, 350},
				"Maintenance Release style hoisting",
				"-mrhoist",
				"Turn on/off k=1 Maintenance Release styl"
				"e hoisting."
			},
			/* [5] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{96, 25, 111, 431},
				"EXPERIMENTAL Maintenance Release style h"
				"oisting",
				"-mrhoistk",
				"Turn on/off k>1 EXPERIMENTAL Maintenance"
				" Release style hoisting."
			},
			/* [6] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{116, 25, 131, 363},
				"Compute context for hoisted predicates",
				"-prc on",
				"Turn on/off computation of context for h"
				"oisted predicates."
			},
			/* [7] */
			NotDependent {

			},
			RegularEntry {
				"Ambiguity aid:",
				{140, 27, 155, 125},
				{141, 135, 155, 209},
				"",
				keepCase,
				"-aa",
				"Ambiguity aid for a rule (rule name or l"
				"ine number)."
			},
			/* [8] */
			NotDependent {

			},
			RegularEntry {
				"Limits exp growth:",
				{140, 236, 155, 361},
				{139, 372, 155, 452},
				"",
				keepCase,
				"-aad",
				"Limits exp growth of -aa listing - defau"
				"lt=1 (max=ck value)."
			},
			/* [9] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{164, 26, 179, 366},
				"Lookahead token may appear multiple time"
				"s",
				"-aam",
				"Lookahead token may appear multiple time"
				"s in -aa listing."
			}
		}
	}
};

