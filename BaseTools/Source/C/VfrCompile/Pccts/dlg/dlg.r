/*
	File:		dlgMPW.r
	Target:		dlg 133MR
	Created:    Monday, June 15, 1998 4:44:11 AM
	Author:		Kenji Tanaka (kentar@osa.att.ne.jp)
*/

#include "cmdo.r"

resource 'cmdo' (128, "Dlg") {
	{	/* array dialogs: 1 elements */
		/* [1] */
		295,
		"DLG -- Purdue Compiler Construction Tool"
		" Set (PCCTS) lexical analyzer generator.",
		{	/* array itemArray: 18 elements */
			/* [1] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{35, 175, 50, 225},
				"On",
				"-CC",
				"When this control is checked, DLG genera"
				"tes a scanner using C++ classes rather t"
				"han C functions."
			},
			/* [2] */
			Or {
				{	/* array OrArray: 1 elements */
					/* [1] */
					1
				}
			},
			RegularEntry {
				"Lexer Class Name:",
				{35, 225, 50, 355},
				{35, 355, 51, 450},
				"DLGLexer",
				keepCase,
				"-cl",
				"This entry specifies the name DLG uses f"
				"or the C++ lexer class."
			},
			/* [3] */
			NotDependent {

			},
			TextBox {
				gray,
				{25, 165, 60, 460},
				"C++ Code Generation"
			},
			/* [4] */
			NotDependent {

			},
			Files {
				InputFile,
				RequiredFile {
					{37, 25, 56, 135},
					"Input File",
					"",
					"Choose the lexical description file for "
					"DLG to process."
				},
				Additional {
					"",
					"",
					"",
					"",
					{	/* array TypesArray: 1 elements */
						/* [1] */
						text
					}
				}
			},
			/* [5] */
			Or {
				{	/* array OrArray: 1 elements */
					/* [1] */
					-1
				}
			},
			Files {
				OutputFile,
				RequiredFile {
					{66, 25, 85, 135},
					"Output File",
					"",
					"Choose the name of the file that will ho"
					"ld the DLG-produced scanner."
				},
				NoMore {

				}
			},
			/* [6] */
			Or {
				{	/* array OrArray: 2 elements */
					/* [1] */
					1,
					/* [2] */
					5
				}
			},
			Dummy {

			},
			/* [7] */
			NotDependent {

			},
			Redirection {
				DiagnosticOutput,
				{90, 25}
			},
			/* [8] */
			NotDependent {

			},
			TextBox {
				gray,
				{25, 20, 132, 145},
				"Files"
			},
			/* [9] */
			NotDependent {

			},
			Files {
				DirOnly,
				OptionalFile {
					{68, 175, 84, 305},
					{88, 175, 107, 305},
					"Output Directory",
					":",
					"-o",
					"",
					"Choose the directory where DLG will put "
					"its output.",
					dim,
					"Output DirectoryI",
					"",
					""
				},
				NoMore {

				}
			},
			/* [10] */
			NotDependent {

			},
			RegularEntry {
				"Mode File Name:",
				{68, 315, 83, 450},
				{88, 315, 104, 450},
				"mode.h",
				keepCase,
				"-m",
				"This entry specifies the name DLG uses f"
				"or its lexical mode output file."
			},
			/* [11] */
			NotDependent {

			},
			RadioButtons {
				{	/* array radioArray: 3 elements */
					/* [1] */
					{134, 175, 149, 255}, "None", "", Set, "When this option is selected, DLG will n"
					"ot compress its tables.",
					/* [2] */
					{134, 265, 149, 345}, "Level 1", "-C1", NotSet, "When this option is selected, DLG will r"
					"emove all unused characters from the tra"
					"nsition-from table.",
					/* [3] */
					{134, 360, 149, 450}, "Level 2", "-C2", NotSet, "When this option is selected, DLG will p"
					"erform level 1 compression plus it will "
					"map equivalent characters into the same "
					"character classes."
				}
			},
			/* [12] */
			NotDependent {

			},
			TextBox {
				gray,
				{124, 165, 156, 460},
				"Table Compression"
			},
			/* [13] */
			NotDependent {

			},
			CheckOption {
				Set,
				{165, 20, 180, 145},
				"Case Sensitive",
				"-ci",
				"When this control is checked, the DLG au"
				"tomaton will treat upper and lower case "
				"characters identically."
			},
			/* [14] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{165, 150, 180, 300},
				"Interactive Scanner",
				"-i",
				"When this control is checked, DLG will g"
				"enerate as interactive a scanner as poss"
				"ible."
			},
			/* [15] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{165, 310, 180, 460},
				"Ambiguity Warnings",
				"-Wambiguity",
				"When this control is checked, DLG warns "
				"if more than one regular expression coul"
				"d match the same character sequence."
			},
			/* [16] */
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
			},
			/* [17] */
			And {
				{	/* array AndArray: 2 elements */
					/* [1] */
					4,
					/* [2] */
					6
				}
			},
			DoItButton {

			},
			/* [18] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{142, 20, 157, 148},
				"Generate ANSI C",
				"-ga",
				"When this control is checked, DLG genera"
				"tes ANSI C compatible code."
			}
		}
	}
};

