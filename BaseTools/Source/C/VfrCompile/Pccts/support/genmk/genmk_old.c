/*
 * genmk -- a program to make makefiles for PCCTS
 *
 * ANTLR 1.33MR10
 * Terence John Parr 1989 - 1998
 * Purdue University
 * U of MN
 */

#include <stdio.h>
#include "pcctscfg.h" /* be sensitive to what ANTLR/DLG call the files */

#ifdef VAXC
#define DIE		return 0;
#define DONE	return 1;
#else
#define DIE		return 1;
#define DONE	return 0;
#endif

#ifndef require
#define require(expr, err) {if ( !(expr) ) fatal(err);}
#endif

#define MAX_FILES	50
#define MAX_CLASSES	50

char *RENAME_OBJ_FLAG="-o",
     *RENAME_EXE_FLAG="-o";

char *dlg = "parser.dlg";
char *err = "err.c";
char *hdr = "stdpccts.h";
char *tok = "tokens.h";
char *mode = "mode.h";
char *scan = "scan";

char ATOKENBUFFER_O[100];
char APARSER_O[100];
char ASTBASE_O[100];
char PCCTSAST_O[100];
char LIST_O[100];
char DLEXERBASE_O[100];

/* Option flags */
static char *project="t", *files[MAX_FILES], *classes[MAX_CLASSES];
static int	num_files = 0;
static int	num_classes = 0;
static int	user_lexer = 0;
static char	*user_token_types = NULL;
static int	gen_CPP = 0;
static char *outdir=".";
static char *dlg_class = "DLGLexer";
static int	gen_trees = 0;
static int  gen_hoist = 0;
static char cfiles[1600]="";
static char *compilerCCC="CC";
static char *compilerCC="cc";
static char *pccts_path="/usr/local/pccts";

void help();
void mk();
void pfiles();
void pclasses();
void fatal();
void warn();

typedef struct _Opt {
			char *option;
			int  arg;
#ifdef __cplusplus
			void (*process)(...);
#else
			void (*process)();
#endif
			char *descr;
		} Opt;

#ifdef __STDC__
static void ProcessArgs(int, char **, Opt *);
#else
static void ProcessArgs();
#endif

static void
pProj( s, t )
char *s;
char *t;
{
	project = t;
}

static void
pUL( s )
char *s;
{
	user_lexer = 1;
}

static void
pCPP( s )
char *s;
{
	gen_CPP = 1;
}

static void
pUT( s, t )
char *s;
char *t;
{
	user_token_types = t;
}

static void
pTrees( s )
char *s;
{
	gen_trees = 1;
}

static void
pHoist( s )
char *s;
{
	gen_hoist = 1;
}

static void
#ifdef __STDC__
pFile( char *s )
#else
pFile( s )
char *s;
#endif
{
	if ( *s=='-' )
	{
		fprintf(stderr, "invalid option: '%s'; ignored...",s);
		return;
	}

	require(num_files<MAX_FILES, "exceeded max # of input files");
	files[num_files++] = s;
}

static void
#ifdef __STDC__
pClass( char *s, char *t )
#else
pClass( s, t )
char *s;
char *t;
#endif
{
	require(num_classes<MAX_CLASSES, "exceeded max # of grammar classes");
	classes[num_classes++] = t;
}

static void
#ifdef __STDC__
pDLGClass( char *s, char *t )
#else
pDLGClass( s, t )
char *s;
char *t;
#endif
{
	if ( !gen_CPP ) {
		fprintf(stderr, "-dlg-class makes no sense without C++ mode; ignored...");
	}
	else dlg_class = t;
}

static void
#ifdef __STDC__
pOdir( char *s, char *t )
#else
pOdir( s, t )
char *s;
char *t;
#endif
{
	outdir = t;
}

static void
#ifdef __STDC__
pHdr( char *s, char *t )
#else
pHdr( s, t )
char *s;
char *t;
#endif
{
	hdr = t;
}

static void
#ifdef __STDC__
pCFiles( char *s, char *t )
#else
pCFiles( s, t )
char *s;
char *t;
#endif
{
	strcat(strcat(cfiles," "), t);
}

static void
#ifdef __STDC__
pCompiler( char *s, char *t )
#else
pCompiler( s, t )
char *s;
char *t;
#endif
{
        compilerCCC = t;
	compilerCC = t;
}

static void
#ifdef __STDC__
ppccts_path( char *s, char *t )
#else
ppccts_path( s, t )
char *s;
char *t;
#endif
{
        pccts_path = t;
}

Opt options[] = {
    { "-CC", 0,	pCPP,			"Generate C++ output"},
    { "-class", 1,	pClass,		"Name of a grammar class defined in grammar (if C++)"},
    { "-dlg-class", 1,pDLGClass,"Name of DLG lexer class (default=DLGLexer) (if C++)"},
    { "-header", 1,pHdr,		"Name of ANTLR standard header info (default=no file)"},
    { "-o", 1,	pOdir,			"Directory where output files should go (default=\".\")"},
    { "-project", 1,	pProj,	"Name of executable to create (default=t)"},
    { "-token-types", 1, pUT,	"Token types are in this file (don't use tokens.h)"},
    { "-trees", 0, pTrees,		"Generate ASTs"},
    { "-user-lexer", 0,	pUL,	"Do not create a DLG-based scanner"},
    { "-mrhoist",0,pHoist,      "Maintenance release style hoisting"},
    { "-cfiles",1,pCFiles,      "Additional files in C or C++ to compile"},
    { "-pccts_path",1,ppccts_path,
              "Path for $PCCTS directory (default is /usr/local/pccts)"},
    { "-compiler",1,pCompiler,
              "Default compiler (default is CC/cc)"},
    { "*", 0,pFile, 	        "" },	/* anything else is a file */
	{ NULL, 0, NULL, NULL }
};

extern char *DIR();

int main(argc, argv)
int argc;
char **argv;
{
	if ( argc == 1 ) { help(); DIE; }
	ProcessArgs(argc-1, &(argv[1]), options);

	strcpy(ATOKENBUFFER_O, ATOKENBUFFER_C);
	ATOKENBUFFER_O[strlen(ATOKENBUFFER_C)-strlen(CPP_FILE_SUFFIX)] = '\0';
	strcat(ATOKENBUFFER_O, OBJ_FILE_SUFFIX);
	strcpy(APARSER_O, APARSER_C);
	APARSER_O[strlen(APARSER_O)-strlen(CPP_FILE_SUFFIX)] = '\0';
	strcat(APARSER_O, OBJ_FILE_SUFFIX);

	strcpy(ASTBASE_O, ASTBASE_C);
	ASTBASE_O[strlen(ASTBASE_C)-strlen(CPP_FILE_SUFFIX)] = '\0';
	strcat(ASTBASE_O, OBJ_FILE_SUFFIX);

	strcpy(PCCTSAST_O, PCCTSAST_C);
	PCCTSAST_O[strlen(PCCTSAST_C)-strlen(CPP_FILE_SUFFIX)] = '\0';
	strcat(PCCTSAST_O, OBJ_FILE_SUFFIX);

	strcpy(LIST_O, LIST_C);
	LIST_O[strlen(LIST_C)-strlen(CPP_FILE_SUFFIX)] = '\0';
	strcat(LIST_O, OBJ_FILE_SUFFIX);

	strcpy(DLEXERBASE_O, DLEXERBASE_C);
	DLEXERBASE_O[strlen(DLEXERBASE_C)-strlen(CPP_FILE_SUFFIX)] = '\0';
	strcat(DLEXERBASE_O, OBJ_FILE_SUFFIX);

	if ( num_files == 0 ) fatal("no grammar files specified; exiting...");
	if ( !gen_CPP && num_classes>0 ) {
		warn("can't define classes w/o C++ mode; turning on C++ mode...\n");
		gen_CPP=1;
	}
	if ( gen_CPP && num_classes==0 ) {
		fatal("must define classes >0 grammar classes in C++ mode\n");
	}

	mk(project, files, num_files, argc, argv);
	DONE;
}

void help()
{
	Opt *p = options;
	static char buf[1000+1];

	fprintf(stderr, "genmk [options] f1.g ... fn.g\n");
	while ( p->option!=NULL && *(p->option) != '*' )
	{
		buf[0]='\0';
		if ( p->arg ) sprintf(buf, "%s ___", p->option);
		else strcpy(buf, p->option);
		fprintf(stderr, "\t%-16s   %s\n", buf, p->descr);
		p++;
	}
}

void mk(project, files, n, argc, argv)
char *project;
char **files;
int n;
int argc;
char **argv;
{
	int i;

	printf("#\n");
	printf("# PCCTS makefile for: ");
	pfiles(files, n, NULL);
	printf("\n");
	printf("#\n");
	printf("# Created from:");
	for (i=0; i<argc; i++) printf(" %s", argv[i]);
	printf("\n");
	printf("#\n");
	printf("# PCCTS release 1.33MR21\n");
	printf("# Project: %s\n", project);
	if ( gen_CPP ) printf("# C++ output\n");
	else printf("# C output\n");
	if ( user_lexer ) printf("# User-defined scanner\n");
	else printf("# DLG scanner\n");
	if ( user_token_types!=NULL ) printf("# User-defined token types in '%s'\n", user_token_types);
	else printf("# ANTLR-defined token types\n");
	printf("#\n");
	printf(".SUFFIXES:\n.SUFFIXES:      .o .cpp .c .h .g .i .dlg\n"); 
	if ( user_token_types!=NULL ) {
		printf("# Make sure #tokdefs directive in ANTLR grammar lists this file:\n");
		printf("TOKENS = %s", user_token_types);
	}
	else printf("TOKENS = %stokens.h", DIR());
	printf("\n");
	printf("#\n");
	printf("# The following filenames must be consistent with ANTLR/DLG flags\n");
	printf("DLG_FILE = %s%s\n", DIR(), dlg);
	printf("ERR = %serr\n", DIR());
	if ( strcmp(hdr,"stdpccts.h")!=0 ) printf("HDR_FILE = %s%s\n", DIR(), hdr);
	else printf("HDR_FILE =\n");
	if ( !gen_CPP ) printf("MOD_FILE = %s%s\n", DIR(), mode);
	if ( !gen_CPP ) printf("SCAN = %s\n", scan);
	else printf("SCAN = %s%s\n", DIR(), dlg_class);

	printf("PCCTS = %s\n",pccts_path);
	printf("ANTLR_H = $(PCCTS)%sh\n", DirectorySymbol);
	printf("BIN = $(PCCTS)%sbin\n", DirectorySymbol);
	printf("ANTLR = $(BIN)%santlr\n", DirectorySymbol);
	printf("DLG = $(BIN)%sdlg\n", DirectorySymbol);
	printf("CFLAGS = -I. -I$(ANTLR_H)");
	if ( strcmp(outdir, ".")!=0 ) printf(" -I%s", outdir);
    printf(" $(COTHER)");
	printf("\n");
	printf("AFLAGS =");
	if ( strcmp(outdir,".")!=0 ) printf(" -o %s", outdir);
	if ( user_lexer ) printf(" -gx");
	if ( gen_CPP ) printf(" -CC");
	if ( strcmp(hdr,"stdpccts.h")!=0 ) printf(" -gh %s", hdr);
	if ( gen_trees ) printf(" -gt");
    if ( gen_hoist ) {
      printf(" -mrhoist on") ;
    } else {
      printf(" -mrhoist off");
    };
    printf(" $(AOTHER)");
	printf("\n");
	printf("DFLAGS = -C2 -i");
	if ( gen_CPP ) printf(" -CC");
	if ( strcmp(dlg_class,"DLGLexer")!=0 ) printf(" -cl %s", dlg_class);
	if ( strcmp(outdir,".")!=0 ) printf(" -o %s", outdir);
    printf(" $(DOTHER)");
	printf("\n");
	printf("GRM = ");
	pfiles(files, n, NULL);
	printf("\n");
	printf("MYFILES = %s\n",cfiles);
	printf("SRC = ");
	if ( gen_CPP ) pfiles(files, n, CPP_FILE_SUFFIX_NO_DOT);
	else pfiles(files, n, "c");
	if ( gen_CPP ) {
		printf(" \\\n     ");
		printf(" ");
		pclasses(classes, num_classes, CPP_FILE_SUFFIX_NO_DOT);
		printf(" \\\n      ");
		printf("$(ANTLR_H)%s%s", DirectorySymbol, APARSER_C);
		if ( !user_lexer ) printf(" $(ANTLR_H)%s%s", DirectorySymbol, DLEXERBASE_C);
		if ( gen_trees ) {
			printf(" \\\n      ");
			printf("$(ANTLR_H)%s%s", DirectorySymbol, ASTBASE_C);
			printf(" $(ANTLR_H)%s%s", DirectorySymbol, PCCTSAST_C);
/*			printf(" $(ANTLR_H)%s%s", DirectorySymbol, LIST_C); */
			printf(" \\\n      ");
		}
		printf(" $(ANTLR_H)%s%s", DirectorySymbol, ATOKENBUFFER_C);
	}
	if ( !user_lexer ) {
		if ( gen_CPP ) printf(" $(SCAN)%s", CPP_FILE_SUFFIX);
		else printf(" %s$(SCAN).c", DIR());
	}
	if ( !gen_CPP ) printf(" $(ERR).c");
	printf("\\\n	$(MYFILES)\n");
	printf("OBJ = ");
	pfiles(files, n, "o");
	if ( gen_CPP ) {
		printf(" \\\n     ");
		printf(" ");
		pclasses(classes, num_classes, "o");
		printf(" \\\n      ");
		printf(" %s%s", DIR(), APARSER_O);
		if ( !user_lexer ) {
			printf(" %s%s", DIR(), DLEXERBASE_O);
		}
		if ( gen_trees ) {
			printf(" \\\n      ");
			printf("%s%s", DIR(), ASTBASE_O);
			printf(" %s%s", DIR(), PCCTSAST_O);
/*			printf(" %s%s", DIR(), LIST_O); */
			printf(" \\\n      ");
		}
		printf(" %s%s", DIR(), ATOKENBUFFER_O);
	}
	if ( !user_lexer ) {
		if ( gen_CPP ) printf(" $(SCAN)%s", OBJ_FILE_SUFFIX);
		else printf(" %s$(SCAN)%s", DIR(), OBJ_FILE_SUFFIX);
	}
	if ( !gen_CPP ) printf(" $(ERR)%s", OBJ_FILE_SUFFIX);
	printf("\\\n	$(MYFILES:.cpp=.o)\n");

	printf("ANTLR_SPAWN = ");
	if ( gen_CPP ) pfiles(files, n, CPP_FILE_SUFFIX_NO_DOT);
	else pfiles(files, n, "c");
	if ( gen_CPP ) {
		printf(" ");
		pclasses(classes, num_classes, CPP_FILE_SUFFIX_NO_DOT);
		printf(" \\\n              ");
		pclasses(classes, num_classes, "h");
		if ( strcmp(hdr,"stdpccts.h")!=0 ) {
			printf(" \\\n              ");
			printf("$(HDR_FILE) stdpccts.h");
		}
	}
	if ( user_lexer ) {
		if ( !user_token_types ) printf(" $(TOKENS)");
	}
	else {
		printf(" $(DLG_FILE)");
		if ( !user_token_types ) printf(" $(TOKENS)");
	}
	if ( !gen_CPP ) printf(" $(ERR).c");
	printf("\n");

	if ( !user_lexer ) {
		if ( gen_CPP ) printf("DLG_SPAWN = $(SCAN)%s", CPP_FILE_SUFFIX);
		else printf("DLG_SPAWN = %s$(SCAN).c", DIR());
		if ( gen_CPP ) printf(" $(SCAN).h");
		if ( !gen_CPP ) printf(" $(MOD_FILE)");
		printf("\n");
	}

	if ( gen_CPP ) {
		printf("CCC = %s\n",compilerCCC);
	}
	else printf("CC = %s\n",compilerCC);

	/* set up dependencies */
	printf("\n%s : $(OBJ) $(SRC)\n", project);
	printf("	%s %s %s $(CFLAGS) $(OBJ)\n",
		   gen_CPP?"$(CCC)":"$(CC)",
		   RENAME_EXE_FLAG,
		   project);
	printf("\n");

	/* implicit rules */

	if(gen_CPP)
		printf("%%.o : %%.cpp\n\t$(CCC) -c $(CFLAGS) $<\n\n");

	printf("%%.o : %%.c\n\t%s -c $(CFLAGS) $<\n\n",
			gen_CPP?"$(CCC)":"$(CC)");

	/* how to compile parser files */

	for (i=0; i<num_files; i++)
	{
		pfiles(&files[i], 1, "o");
		if ( user_lexer ) {
			printf(" : $(TOKENS)");
		}
		else {
			if ( gen_CPP ) printf(" : $(TOKENS) $(SCAN).h");
			else printf(" : $(MOD_FILE) $(TOKENS)");
		}
		printf(" ");
		if ( gen_CPP ) pfiles(&files[i], 1, CPP_FILE_SUFFIX_NO_DOT);
		else pfiles(&files[i], 1, "c");
		if ( gen_CPP && strcmp(hdr,"stdpccts.h")!=0 ) printf(" $(HDR_FILE)");
		printf("\n");
		printf("	%s -c $(CFLAGS) %s ",
		       gen_CPP?"$(CCC)":"$(CC)",RENAME_OBJ_FLAG);
		pfiles(&files[i], 1, "o");
		printf(" ");
		if ( gen_CPP ) pfiles(&files[i], 1, CPP_FILE_SUFFIX_NO_DOT);
		else pfiles(&files[i], 1, "c");
		printf("\n\n");
	}

	/* how to compile err.c */
	if ( !gen_CPP ) {
		printf("$(ERR)%s : $(ERR).c", OBJ_FILE_SUFFIX);
		if ( !user_lexer ) printf(" $(TOKENS)");
		printf("\n");
		printf("	%s -c $(CFLAGS) %s $(ERR)%s $(ERR).c",
			   gen_CPP?"$(CCC)":"$(CC)",
			   RENAME_OBJ_FLAG,
			   OBJ_FILE_SUFFIX);
		printf("\n\n");
	}

	/* how to compile Class.c */
	for (i=0; i<num_classes; i++)
	{
		pclasses(&classes[i], 1, "o");
		if ( user_lexer ) {
			printf(" : $(TOKENS)");
		}
		else {
			printf(" : $(TOKENS) $(SCAN).h");
		}
		printf(" ");
		pclasses(&classes[i], 1, CPP_FILE_SUFFIX_NO_DOT);
		printf(" ");
		pclasses(&classes[i], 1, "h");
		if ( gen_CPP && strcmp(hdr,"stdpccts.h")!=0 ) printf(" $(HDR_FILE)");
		printf("\n");
		printf("	%s -c $(CFLAGS) %s ",
			   gen_CPP?"$(CCC)":"$(CC)",
			   RENAME_OBJ_FLAG);
		pclasses(&classes[i], 1, "o");
		printf(" ");
		pclasses(&classes[i], 1, CPP_FILE_SUFFIX_NO_DOT);
		printf("\n\n");
	}

	/* how to compile scan.c */
	if ( !user_lexer ) {
		if ( gen_CPP ) printf("$(SCAN)%s : $(SCAN)%s", OBJ_FILE_SUFFIX, CPP_FILE_SUFFIX);
		else printf("%s$(SCAN)%s : %s$(SCAN).c", DIR(), OBJ_FILE_SUFFIX, DIR());
		if ( !user_lexer ) printf(" $(TOKENS)");
		printf("\n");
		if ( gen_CPP ) printf("	$(CCC) -c $(CFLAGS) %s $(SCAN)%s $(SCAN)%s",
							  RENAME_OBJ_FLAG,
							  OBJ_FILE_SUFFIX,
							  CPP_FILE_SUFFIX);
		else printf("	$(CC) -c $(CFLAGS) %s %s$(SCAN)%s %s$(SCAN).c",
					RENAME_OBJ_FLAG,
					DIR(),
					OBJ_FILE_SUFFIX,
					DIR());
		printf("\n\n");
	}

	printf("$(ANTLR_SPAWN) : $(GRM)\n");
	printf("	$(ANTLR) $(AFLAGS) $(GRM)\n");

	if ( !user_lexer )
	{
		printf("\n");
		printf("$(DLG_SPAWN) : $(DLG_FILE)\n");
		if ( gen_CPP ) printf("	$(DLG) $(DFLAGS) $(DLG_FILE)\n");
		else printf("	$(DLG) $(DFLAGS) $(DLG_FILE) $(SCAN).c\n");
	}

	/* do the makes for ANTLR/DLG support */
	if ( gen_CPP ) {
		printf("\n");
		printf("%s%s : $(ANTLR_H)%s%s\n", DIR(), APARSER_O, DirectorySymbol, APARSER_C);
		printf("	%s -c $(CFLAGS) %s ",
			   gen_CPP?"$(CCC)":"$(CC)",
			   RENAME_OBJ_FLAG);
		printf("%s%s $(ANTLR_H)%s%s\n", DIR(), APARSER_O, DirectorySymbol, APARSER_C);
		printf("\n");
		printf("%s%s : $(ANTLR_H)%s%s\n", DIR(), ATOKENBUFFER_O, DirectorySymbol, ATOKENBUFFER_C);
		printf("	%s -c $(CFLAGS) %s ",
			   gen_CPP?"$(CCC)":"$(CC)",
			   RENAME_OBJ_FLAG);
		printf("%s%s $(ANTLR_H)%s%s\n", DIR(), ATOKENBUFFER_O, DirectorySymbol, ATOKENBUFFER_C);
		if ( !user_lexer ) {
			printf("\n");
			printf("%s%s : $(ANTLR_H)%s%s\n", DIR(), DLEXERBASE_O, DirectorySymbol, DLEXERBASE_C);
			printf("	%s -c $(CFLAGS) %s ",
				   gen_CPP?"$(CCC)":"$(CC)",
				   RENAME_OBJ_FLAG);
			printf("%s%s $(ANTLR_H)%s%s\n", DIR(), DLEXERBASE_O, DirectorySymbol, DLEXERBASE_C);
		}
		if ( gen_trees ) {
			printf("\n");
			printf("%s%s : $(ANTLR_H)%s%s\n", DIR(), ASTBASE_O, DirectorySymbol, ASTBASE_C);
			printf("	%s -c $(CFLAGS) %s ",
				   gen_CPP?"$(CCC)":"$(CC)",
				   RENAME_OBJ_FLAG);
			printf("%s%s $(ANTLR_H)%s%s\n", DIR(), ASTBASE_O, DirectorySymbol, ASTBASE_C);
			printf("\n");
			printf("%s%s : $(ANTLR_H)%s%s\n", DIR(), PCCTSAST_O, DirectorySymbol, PCCTSAST_C);
			printf("	%s -c $(CFLAGS) %s ",
				   gen_CPP?"$(CCC)":"$(CC)",
				   RENAME_OBJ_FLAG);
			printf("%s%s $(ANTLR_H)%s%s\n", DIR(), PCCTSAST_O, DirectorySymbol, PCCTSAST_C);
			printf("\n");
/*
			printf("%s%s : $(ANTLR_H)%s%s\n", DIR(), LIST_O, DirectorySymbol, LIST_C);
			printf("	%s -c $(CFLAGS) %s ",
			       gen_CPP?"$(CCC)":"$(CC)",RENAME_OBJ_FLAG);
			printf("%s%s $(ANTLR_H)%s%s\n", DIR(), LIST_O, DirectorySymbol, LIST_C);
*/
		}
	}

	/* clean and scrub targets */

	printf("\nclean:\n");
	printf("	rm -f *%s core %s", OBJ_FILE_SUFFIX, project);
	if ( strcmp(outdir, ".")!=0 ) printf(" %s*%s", DIR(), OBJ_FILE_SUFFIX);
	printf("\n");

	printf("\nscrub:\n");
	printf("	rm -f *%s core %s", OBJ_FILE_SUFFIX, project);
	if ( strcmp(outdir, ".")!=0 ) printf(" %s*%s", DIR(), OBJ_FILE_SUFFIX);
	printf(" $(ANTLR_SPAWN)");
	if ( !user_lexer ) printf(" $(DLG_SPAWN)");
	printf("\n");
}

void pfiles(files, n, suffix)
char **files;
int n;
char *suffix;
{
	int first=1;

	while ( n>0 )
	{
		char *p = &(*files)[strlen(*files)-1];
		if ( !first ) putchar(' ');
		first=0;
		while ( p > *files && *p != '.' ) --p;
		if ( p == *files )
		{
			fprintf(stderr,
					"genmk: filenames must be file.suffix format: %s\n",
					*files);
			exit(-1);
		}
		if ( suffix == NULL ) printf("%s", *files);
		else
		{
			*p = '\0';
			printf("%s", DIR());
			if ( strcmp(suffix, "o")==0 ) printf("%s%s", *files, OBJ_FILE_SUFFIX);
			else printf("%s.%s", *files, suffix);
			*p = '.';
		}
		files++;
		--n;
	}
}

void pclasses(classes, n, suffix)
char **classes;
int n;
char *suffix;
{
	int first=1;

	while ( n>0 )
	{
		if ( !first ) putchar(' ');
		first=0;
		if ( suffix == NULL ) printf("%s", *classes);
		else {
			printf("%s", DIR());
			if ( strcmp(suffix, "o")==0 ) printf("%s%s", *classes, OBJ_FILE_SUFFIX);
			else printf("%s.%s", *classes, suffix);
		}
		classes++;
		--n;
	}
}

static void
#ifdef __STDC__
ProcessArgs( int argc, char **argv, Opt *options )
#else
ProcessArgs( argc, argv, options )
int argc;
char **argv;
Opt *options;
#endif
{
	Opt *p;
	require(argv!=NULL, "ProcessArgs: command line NULL");

	while ( argc-- > 0 )
	{
		p = options;
		while ( p->option != NULL )
		{
			if ( strcmp(p->option, "*") == 0 ||
				 strcmp(p->option, *argv) == 0 )
			{
				if ( p->arg )
				{
					(*p->process)( *argv, *(argv+1) );
					argv++;
					argc--;
				}
				else
					(*p->process)( *argv );
				break;
			}
			p++;
		}
		argv++;
	}
}

void fatal( err_)
char *err_;
{
    fprintf(stderr, "genmk: %s\n", err_);
    exit(1);
}

void warn( err_)
char *err_;
{
    fprintf(stderr, "genmk: %s\n", err_);
}

char *DIR()
{
	static char buf[200+1];
	
	if ( strcmp(outdir,TopDirectory)==0 ) return "";
	sprintf(buf, "%s%s", outdir, DirectorySymbol);
	return buf;
}
