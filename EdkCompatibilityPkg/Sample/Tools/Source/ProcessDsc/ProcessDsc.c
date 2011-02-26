/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  ProcessDsc.c

Abstract:

  Main module for the ProcessDsc utility.

--*/

#include <windows.h>  // for GetShortPathName()
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <direct.h>   // for _mkdir()
#include <errno.h>
#include <stdlib.h>   // for getenv()
#include <shlwapi.h>  // for PathCanonicalize()
#include "DSCFile.h"
#include "MultiThread.h"
#include "FWVolume.h"
#include "Exceptions.h"
#include "Common.h"

#include "EfiUtilityMsgs.h"
#include "TianoBind.h"

#define UTILITY_NAME    "ProcessDsc"
#define UTILITY_VERSION "v1.0"

//
// Common symbol name definitions. For example, the user can reference
// $(BUILD_DIR) in their DSC file and we will expand it for them (usually).
// I've defined the equivalents here in case we want to change the name the
// user references, in which case we just change the string value here and
// our code still works.
//
#define BUILD_DIR                       "BUILD_DIR"
#define EFI_SOURCE                      "EFI_SOURCE"
#define DEST_DIR                        "DEST_DIR"
#define SOURCE_DIR                      "SOURCE_DIR"
#define LIB_DIR                         "LIB_DIR"
#define BIN_DIR                         "BIN_DIR"
#define OUT_DIR                         "OUT_DIR"
#define INF_FILENAME                    "INF_FILENAME"
#define SOURCE_RELATIVE_PATH            "SOURCE_RELATIVE_PATH"
#define SOURCE_BASE_NAME                "SOURCE_BASE_NAME"
#define SOURCE_FILE_NAME                "SOURCE_FILE_NAME"    // c:\FullPath\File.c
#define PROCESSOR                       "PROCESSOR"
#define FV                              "FV"
#define BASE_NAME                       "BASE_NAME"
#define GUID                            "GUID"
#define FILE_GUID                       "FILE_GUID"
#define COMPONENT_TYPE_FILE             "FILE"
#define BUILD_TYPE                      "BUILD_TYPE"
#define FFS_EXT                         "FFS_EXT"             // FV_EXT is deprecated -- extension of FFS file
#define MAKEFILE_NAME                   "MAKEFILE_NAME"       // name of component's output makefile
#define PLATFORM                        "PLATFORM"            // for more granularity
#define PACKAGE_FILENAME                "PACKAGE_FILENAME"
#define PACKAGE                         "PACKAGE"
#define PACKAGE_TAG                     "PACKAGE_TAG"         // alternate name to PACKAGE
#define SHORT_NAMES                     "SHORT_NAMES"         // for 8.3 names of symbols
#define APRIORI                         "APRIORI"             // to add to apriori list
#define OPTIONAL_COMPONENT              "OPTIONAL"            // define as non-zero for optional INF files
#define SOURCE_SELECT                   "SOURCE_SELECT"       // say SOURCE_SELECT=smm,common to select INF sources
#define NONFFS_FV                       "NONFFS_FV"           // for non-FFS FV such as working & spare block FV
#define SKIP_FV_NULL                    "SKIP_FV_NULL"        // define as nonzero to not build components with FV=NULL
#define SOURCE_COMPILE_TYPE             "SOURCE_COMPILE_TYPE" // to build a source using a custom build section in the DSC file
#define SOURCE_FILE_EXTENSION           "SOURCE_FILE_EXTENSION"
#define COMPILE_SELECT                  "COMPILE_SELECT"
#define SOURCE_OVERRIDE_PATH            "SOURCE_OVERRIDE_PATH"  // get source files from here first
#define MAKEFILE_OUT_SECTION_NAME       "makefile.out"
#define COMMON_SECTION_NAME             "common"                // shared files or functionality
#define NMAKE_SECTION_NAME              "nmake"
#define SOURCES_SECTION_NAME            "sources"
#define COMPONENTS_SECTION_NAME         "components"
#define INCLUDE_SECTION_NAME            "includes"
#define DEFINES_SECTION_NAME            "defines"
#define LIBRARIES_SECTION_NAME          "libraries"
#define LIBRARIES_PLATFORM_SECTION_NAME "libraries.platform"
#define MAKEFILE_SECTION_NAME           "makefile"
#define COMPONENT_TYPE                  "component_type"
#define PLATFORM_STR                    "\\platform\\"          // to determine EFI_SOURCE
#define MAKEFILE_OUT_NAME               "makefile.out"          // if not specified on command line
#define MODULE_MAKEFILE_NAME            "module.mak"            // record all module makefile targets
#define MODULE_NAME_FILE                "module.list"           // record all module names defined in the dsc file
#define GLOBAL_LINK_LIB_NAME            "CompilerStub"          // Lib added in link option, maybe removed in the future
#define MODULE_BASE_NAME_WIDTH          25                      // Width for module name output

//
// When a symbol is defined as "NULL", it gets saved in the symbol table as a 0-length
// string. Use this macro to detect if a symbol has been defined this way.
//
#define IS_NULL_SYMBOL_VALUE(var) ((var != NULL) && (strlen (var) == 0))

//
// Defines for file types
//
#define FILETYPE_UNKNOWN  0
#define FILETYPE_C        1
#define FILETYPE_ASM      2
#define FILETYPE_S        3
#define FILETYPE_VFR      4
#define FILETYPE_INC      5
#define FILETYPE_H        6
#define FILETYPE_I        7


typedef struct {
  INT8  *Extension;         // file extension
  INT8  *BuiltExtension;
  INT8  FileFlags;
  int   FileType;
} FILETYPE;

//
// Define masks for the FileFlags field
//
#define FILE_FLAG_INCLUDE 0x01
#define FILE_FLAG_SOURCE  0x02

//
// This table describes a from-to list of files. For
// example, when a ".c" is built, it results in a ".obj" file.
//
static const FILETYPE mFileTypes[] = {
  {
    ".c",
    ".obj",
    FILE_FLAG_SOURCE,
    FILETYPE_C
  },
  {
    ".asm",
    ".obj",
    FILE_FLAG_SOURCE,
    FILETYPE_ASM
  },
  {
    ".s",
    ".obj",
    FILE_FLAG_SOURCE,
    FILETYPE_S
  },
  {
    ".vfr",
    ".obj",
    FILE_FLAG_SOURCE,
    FILETYPE_VFR
  },  // actually *.vfr -> *.c -> *.obj
  {
    ".h",
    NULL,
    FILE_FLAG_INCLUDE,
    FILETYPE_H
  },
  {
    ".inc",
    NULL,
    FILE_FLAG_INCLUDE,
    FILETYPE_INC
  },
  {
    ".i",
    NULL,
    FILE_FLAG_INCLUDE,
    FILETYPE_I
  },
  {
    NULL,
    NULL,
    0,
    0
  }
};

//
// Structure to split up a file into its different parts.
//
typedef struct {
  INT8  Drive[3];
  INT8  *Path;
  INT8  *BaseName;
  INT8  *Extension;
  int   ExtensionCode;
} FILE_NAME_PARTS;

//
// Maximum length for any line in any file after symbol expansion
//
#define MAX_EXP_LINE_LEN  (MAX_LINE_LEN * 2)

//
// Linked list to keep track of all symbols
//
typedef struct _SYMBOL {
  struct _SYMBOL  *Next;
  int             Type; // local or global symbol
  INT8            *Name;
  INT8            *Value;
} SYMBOL;

//
// Module globals for multi-thread build
//
static BUILD_ITEM  **mCurrentBuildList;          // build list currently handling
static BUILD_ITEM  *mCurrentBuildItem;           // build item currently handling

//
// Define masks for the build targets
//
#define BUILD_TARGET_COMPONENTS 0x01
#define BUILD_TARGET_LIBRARIES  0x02
#define BUILD_TARGET_FVS        0x04
#define BUILD_TARGET_ALL        0xff


//
// This structure is used to save globals
//
struct {
  INT8                *DscFilename;
  SYMBOL              *Symbol;
  INT8                MakefileName[MAX_PATH]; // output makefile name
  INT8                XRefFileName[MAX_PATH];
  INT8                GuidDatabaseFileName[MAX_PATH];
  INT8                ModuleMakefileName[MAX_PATH];
  FILE                *MakefileFptr;
  FILE                *ModuleMakefileFptr;
  SYMBOL              *ModuleList;
  SYMBOL              *OutdirList;
  UINT32              Verbose;
  UINT32              ThreadNumber;
  UINT32              BuildTarget;
  BUILD_ITEM          *LibraryList;
  COMPONENTS_ITEM     *ComponentsList;
} gGlobals;

//
// This gets dumped to the head of makefile.out
//
static const INT8 *MakefileHeader[] = {
  "#/*++",
  "#",
  "#  DO NOT EDIT",
  "#  File auto-generated by build utility",
  "#",
  "#  Module Name:",
  "#",
  "#    makefile",
  "#",
  "#  Abstract:",
  "#",
  "#    Auto-generated makefile for building of EFI components/libraries",
  "#",
  "#--*/",
  "",
  NULL
};

//
// Function prototypes
//
static
int
ProcessOptions (
  int  Argc,
  INT8 *Argv[]
  );

static
void
Usage (
  VOID
  );

static
INT8              *
StripLine (
  INT8 *Line
  );

static
STATUS
ParseGuidDatabaseFile (
  INT8 *FileName
  );

#define DSC_SECTION_TYPE_COMPONENTS         0
#define DSC_SECTION_TYPE_LIBRARIES          1
#define DSC_SECTION_TYPE_PLATFORM_LIBRARIES 2

static
int
ProcessSectionComponents (
  DSC_FILE *DscFile,
  int      DscSectionType,
  int      Instance
  );
static
int
ProcessComponentFile (
  DSC_FILE  *DscFile,
  INT8      *Line,
  int       DscSectionType,
  int       Instance
  );
static
int
ProcessIncludeFiles (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr
  );
static

int
ProcessIncludeFilesSingle (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  INT8      *SectionName
  );

//
// Mode flags for processing source files
//
#define SOURCE_MODE_BUILD_COMMANDS  0x01
#define SOURCE_MODE_SOURCE_FILES    0x02

static
int
ProcessSourceFiles (
  DSC_FILE  *DSCFile,
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  UINT32    Mode
  );

static
int
ProcessSourceFilesSection (
  DSC_FILE  *DSCFile,
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  INT8      *SectionName,
  UINT32    Mode
  );

static
int
ProcessObjects (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr
  );

static
int
ProcessObjectsSingle (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  INT8      *SectionName
  );

static
int
ProcessLibs (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr
  );

static
int
ProcessLibsSingle (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  INT8      *SectionName
  );

static
int
ProcessIncludesSection (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr
  );

static
int
ProcessIncludesSectionSingle (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  INT8      *SectionName
  );

static
int
ProcessINFNMakeSection (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr
  );

static
int
ProcessINFDefinesSection (
  DSC_FILE  *ComponentFile
  );

static
int
ProcessINFDefinesSectionSingle (
  DSC_FILE  *ComponentFile,
  INT8      *SectionName
  );

static
int
ProcessSectionLibraries (
  DSC_FILE  *DscFile,
  long      Offset
  );

static
int
ProcessDSCDefinesSection (
  DSC_FILE *DscFile
  );

static
int
SetSymbolType (
  INT8  *SymbolName,
  INT8  Type
  );

static
int
RemoveLocalSymbols (
  VOID
  );

static
int
RemoveFileSymbols (
  VOID
  );

static
int
RemoveSymbol (
  INT8   *Name,
  INT8   SymbolType
  );

static
int
SetFileExtension (
  INT8 *FileName,
  INT8 *Extension
  );

static
int
GetSourceFileType (
  INT8 *FileName
  );

static
int
IsIncludeFile (
  INT8 *FileName
  );

static
int
WriteCompileCommands (
  DSC_FILE *DscFile,
  FILE     *MakeFptr,
  INT8     *FileName,
  INT8     *Processor
  );

static
int
WriteCommonMakefile (
  DSC_FILE *DscFile,
  FILE     *MakeFptr,
  INT8     *Processor
  );

static
int
WriteComponentTypeBuildCommands (
  DSC_FILE *DscFile,
  FILE     *MakeFptr,
  INT8     *SectionName
  );

static
void
StripTrailingSpaces (
  INT8 *Str
  );

static
void
FreeFileParts (
  FILE_NAME_PARTS *FP
  );

static
FILE_NAME_PARTS   *
GetFileParts (
  INT8 *FileName
  );

static
SYMBOL            *
FreeSymbols (
  SYMBOL *Syms
  );

static
int
GetEfiSource (
  VOID
  );

static
int
CreatePackageFile (
  DSC_FILE          *DSCFile
  );

static
INT8              *
BuiltFileExtension (
  INT8      *SourceFileName
  );

static
void
SmartFree (
  SMART_FILE  *SmartFile
  );

static 
int
AddModuleName (
  SYMBOL  **SymbolList,
  INT8    *ModuleName,
  INT8    *InfName
  );

static 
void
ReplaceSlash (
  INT8    *Path
  );

/*****************************************************************************/
int
main (
  int   Argc,
  INT8  *Argv[]
  )
/*++

Routine Description:
  
  Main utility entry point.

Arguments:

  Argc - Standard app entry point args.
  Argv - Standard app entry point args.

Returns:

  0  if successful
  non-zero otherwise

--*/
{
  int                   i;
  DSC_FILE              DSCFile;
  SECTION               *Sect;
  INT8                  Line[MAX_LINE_LEN];
  INT8                  ExpLine[MAX_LINE_LEN];
  INT8                  *BuildDir;
  INT8                  *EMsg;
  FILE                  *FpModule;
  SYMBOL                *TempSymbol;
  COMPONENTS_ITEM       *TempComponents;

  SetUtilityName (UTILITY_NAME);

  InitExceptions ();

  DSCFileInit (&DSCFile);
  //
  // Initialize the firmware volume data
  //
  CFVConstructor ();
  //
  // Exception handling for this block of code.
  //
  TryException ();
  //
  // Process command-line options.
  //
  if (ProcessOptions (Argc, Argv)) {
    EMsg = CatchException ();
    if (EMsg != NULL) {
      fprintf (stderr, "%s\n", EMsg);
    }

    return STATUS_ERROR;
  }
  //
  // Parse the GUID database file if specified
  //
  if (gGlobals.GuidDatabaseFileName[0] != 0) {
    ParseGuidDatabaseFile (gGlobals.GuidDatabaseFileName);
  }
  //
  // Set the output cross-reference file if applicable
  //
  if (gGlobals.XRefFileName[0]) {
    CFVSetXRefFileName (gGlobals.XRefFileName);
  }

  //
  // Now get the EFI_SOURCE directory which we use everywhere.
  //
  if (GetEfiSource ()) {
    return STATUS_ERROR;
  }
    
  //
  // Pre-process the DSC file to get section info.
  //
  if (DSCFileSetFile (&DSCFile, gGlobals.DscFilename) != 0) {
    goto ProcessingError;
  }

  //
  // Set output makefile name for single module build 
  //
  strcpy (gGlobals.ModuleMakefileName, MODULE_MAKEFILE_NAME);

  //
  // Try to open all final output makefiles
  //
  if ((gGlobals.MakefileFptr = fopen (gGlobals.MakefileName, "w")) == NULL) {
    Error (NULL, 0, 0, gGlobals.MakefileName, "failed to open output makefile for writing");
    goto ProcessingError;
  }
  if ((gGlobals.ModuleMakefileFptr = fopen (gGlobals.ModuleMakefileName, "w")) == NULL) {
    Error (NULL, 0, 0, gGlobals.ModuleMakefileName, "failed to open output makefile for writing");
    goto ProcessingError;
  }
    
  //
  // Write the header out to the makefiles
  //
  for (i = 0; MakefileHeader[i] != NULL; i++) {
    fprintf (gGlobals.MakefileFptr, "%s\n", MakefileHeader[i]);
    fprintf (gGlobals.ModuleMakefileFptr, "%s\n", MakefileHeader[i]);
  }

  //
  // Init global potint = NULL
  //
  gGlobals.ModuleList = NULL;
  gGlobals.OutdirList = NULL;
  
  //
  // Process the [defines] section in the DSC file to get any defines we need
  // elsewhere
  //
  ProcessDSCDefinesSection (&DSCFile);
  if (ExceptionThrown ()) {
    goto ProcessingError;
  }
  //
  // Write out the [makefile.out] section data to the output makefiles
  //
  Sect = DSCFileFindSection (&DSCFile, MAKEFILE_OUT_SECTION_NAME);
  if (Sect != NULL) {
    while (DSCFileGetLine (&DSCFile, Line, sizeof (Line)) != NULL) {
      ExpandSymbols (Line, ExpLine, sizeof (ExpLine), 0);
      //
      // Write the line to the output makefiles
      //
      fprintf (gGlobals.MakefileFptr, ExpLine);
      fprintf (gGlobals.ModuleMakefileFptr, ExpLine);
    }
  }
  
  //
  // Add a pseudo target for GLOBAL_LINK_LIB_NAME to avoid single module build 
  // failure when this lib is not used.
  //
  fprintf (gGlobals.ModuleMakefileFptr, "%sbuild ::\n\n", GLOBAL_LINK_LIB_NAME);

  fprintf (gGlobals.MakefileFptr, "libraries : \n");
  //
  // Process [libraries] section in the DSC file
  //
  Sect = DSCFileFindSection (&DSCFile, LIBRARIES_SECTION_NAME);
  if (Sect != NULL) {
    mCurrentBuildList = &gGlobals.LibraryList;
    ProcessSectionComponents (&DSCFile, DSC_SECTION_TYPE_LIBRARIES, 0);
  }

  if (ExceptionThrown ()) {
    goto ProcessingError;
  }
  //
  // Process [libraries.platform] section in the DSC file
  //
  Sect = DSCFileFindSection (&DSCFile, LIBRARIES_PLATFORM_SECTION_NAME);
  if (Sect != NULL) {
    mCurrentBuildList = &gGlobals.LibraryList;
    ProcessSectionComponents (&DSCFile, DSC_SECTION_TYPE_PLATFORM_LIBRARIES, 0);
  }

  fprintf (gGlobals.MakefileFptr, "\n");
  if (ExceptionThrown ()) {
    goto ProcessingError;
  }
  
  //
  // Process [components] section in the DSC file
  //
  Sect = DSCFileFindSection (&DSCFile, COMPONENTS_SECTION_NAME);
  if (Sect != NULL) {
    fprintf (gGlobals.MakefileFptr, "components_0 : \n");
    TempComponents    = AddComponentsItem (&gGlobals.ComponentsList);
    mCurrentBuildList = &TempComponents->BuildList;
    ProcessSectionComponents (&DSCFile, DSC_SECTION_TYPE_COMPONENTS, 0);
    fprintf (gGlobals.MakefileFptr, "\n");
  }

  if (ExceptionThrown ()) {
    goto ProcessingError;
  }
  //
  // Now cycle through all [components.1], [components.2], ....[components.n].
  // This is necessary to support building of firmware volumes that may contain
  // other encapsulated firmware volumes (ala capsules).
  //
  i = 1;
  while (1) {
    RemoveSymbol (FV, SYM_GLOBAL);
    sprintf (Line, "%s.%d", COMPONENTS_SECTION_NAME, i);
    Sect = DSCFileFindSection (&DSCFile, Line);
    if (Sect != NULL) {
      fprintf (gGlobals.MakefileFptr, "components_%d : \n", i);
      TempComponents    = AddComponentsItem (&gGlobals.ComponentsList);
      mCurrentBuildList = &TempComponents->BuildList;
      ProcessSectionComponents (&DSCFile, DSC_SECTION_TYPE_COMPONENTS, i);
      fprintf (gGlobals.MakefileFptr, "\n");
    } else {
      break;
    }

    if (ExceptionThrown ()) {
      goto ProcessingError;
    }

    i++;
  }

ProcessingError:
  EMsg = CatchException ();
  if (EMsg != NULL) {
    fprintf (stderr, "%s\n", EMsg);
    fprintf (stderr, "Processing aborted\n");
  }

  TryException ();
  //
  // Create the FV files if no fatal errors or errors
  //
  if (GetUtilityStatus () < STATUS_ERROR) {
    CFVWriteInfFiles (&DSCFile, gGlobals.MakefileFptr);
  }

  //
  // Write all module name into MODULE_NAME_FILE file.
  //
  if ((FpModule = fopen (MODULE_NAME_FILE, "w")) != NULL) {
    TempSymbol = gGlobals.ModuleList;
    while (TempSymbol != NULL) {
      fprintf (FpModule, " %-*s %s \n", MODULE_BASE_NAME_WIDTH, TempSymbol->Name, TempSymbol->Value);
      TempSymbol = TempSymbol->Next;
    }
    fclose (FpModule);
    FpModule = NULL;
  }

  //
  // Close the all the output makefiles
  //
  if (gGlobals.MakefileFptr != NULL) {
    fclose (gGlobals.MakefileFptr);
    gGlobals.MakefileFptr = NULL;
  }

  if (gGlobals.ModuleMakefileFptr != NULL) {
    fclose (gGlobals.ModuleMakefileFptr);
    gGlobals.ModuleMakefileFptr = NULL;
  }
  
  //
  // Start multi-thread build if ThreadNumber is specified and no error status
  //
  if ((gGlobals.ThreadNumber != 0) && (GetUtilityStatus () < STATUS_ERROR)) {
    BuildDir = GetSymbolValue (BUILD_DIR);
    if (gGlobals.BuildTarget & BUILD_TARGET_LIBRARIES) {
      if (StartMultiThreadBuild (&gGlobals.LibraryList, gGlobals.ThreadNumber, BuildDir) != 0) {
        Error (NULL, 0, 0, NULL, "Multi-thread build libraries failure");
        goto Cleanup;
      }
    }
    i = 0;
    TempComponents = gGlobals.ComponentsList;
    while (TempComponents != NULL) {
      if (gGlobals.BuildTarget & BUILD_TARGET_COMPONENTS) {
        if (StartMultiThreadBuild (&TempComponents->BuildList, gGlobals.ThreadNumber, BuildDir) != 0) {
          Error (NULL, 0, 0, NULL, "Multi-thread build components %d failure", i);
          goto Cleanup;
        }
      }
      if (gGlobals.BuildTarget & BUILD_TARGET_FVS) {
        sprintf (ExpLine, "nmake -nologo -f %s fvs_%d", gGlobals.MakefileName, i);
        _flushall ();
        if (system (ExpLine)) {
          Error (NULL, 0, 0, NULL, "Build FVs for components %d failure", i);
          goto Cleanup;
        }
      }
      i++;
      TempComponents = TempComponents->Next;
    }
  }

Cleanup:    
  //
  // Clean up
  //
  FreeBuildList (gGlobals.LibraryList);
  gGlobals.LibraryList = NULL;
  FreeComponentsList (gGlobals.ComponentsList);
  gGlobals.ComponentsList = NULL;
  FreeSymbols (gGlobals.ModuleList);
  gGlobals.ModuleList = NULL;
  FreeSymbols (gGlobals.OutdirList);
  gGlobals.OutdirList = NULL;
  FreeSymbols (gGlobals.Symbol);
  gGlobals.Symbol = NULL;
  CFVDestructor ();
  DSCFileDestroy (&DSCFile);

  EMsg = CatchException ();
  if (EMsg != NULL) {
    fprintf (stderr, "%s\n", EMsg);
    fprintf (stderr, "Processing aborted\n");
  }

  return GetUtilityStatus ();
}

static
int
ProcessSectionComponents (
  DSC_FILE  *DSCFile,
  int       DscSectionType,
  int       Instance
  )
/*++

Routine Description:
  
  Process the [components] or [libraries] section in the description file. We
  use this function for both since they're very similar. Here we just
  read each line from the section, and if it's valid, call a function to
  do the actual processing of the component description file.

Arguments:

  DSCFile        - structure containing section info on the description file
  DscSectionType - type of description section

Returns:

  0     if successful

--*/
{
  INT8  Line[MAX_LINE_LEN];
  INT8  Line2[MAX_EXP_LINE_LEN];
  INT8  *Cptr;

  //
  // Read lines while they're valid
  //
  while (DSCFileGetLine (DSCFile, Line, sizeof (Line)) != NULL) {
    //
    // Expand symbols on the line
    //
    if (ExpandSymbols (Line, Line2, sizeof (Line2), 0)) {
      return STATUS_ERROR;
    }
    //
    // Strip the line
    //
    Cptr = StripLine (Line2);
    if (*Cptr) {
      Message (2, "Processing component line: %s", Line2);
      if (ProcessComponentFile (DSCFile, Line2, DscSectionType, Instance) != 0) {
        return STATUS_ERROR;
      }
    }
  }

  return 0;
}

static
int
ProcessComponentFile (
  DSC_FILE  *DSCFile,
  INT8      *ArgLine,
  int       DscSectionType,
  int       Instance
  )
/*++

Routine Description:
  
  Given a line from the [components] or [libraries] section of the description
  file, process the line to extract the component's INF filename and 
  parameters. Then open the INF file and process it to create a corresponding
  makefile.

Arguments:

  DSCFile               The project DSC file info structure.
  Libs                  Indicates whether we're processing the [components]
                        section or the [libraries] section.
  ArgLine               The actual line from the DSC file. Looks something like
                        one of the following:

   dxe\drivers\vm\vm.dsc PROCESSOR=IA32 DEST_DIR=$(DEST_DIR)\xxx FV=FV1,FV2
   $(BUILD_DIR).\FvVariable.ffs COMPONENT_TYPE=FILE
   .\FvVariable.ffs COMPONENT_TYPE=FILE
   define  VAR1=value1 VAR2=value2

Returns:

  0 if successful

--*/
{
  FILE      *MakeFptr;
  FILE      *TempFptr;
  INT8      *Cptr;
  INT8      *name;
  INT8      *End;
  INT8      *TempCptr;
  INT8      FileName[MAX_PATH];
  INT8      ComponentFilePath[MAX_PATH];
  INT8      InLine[MAX_LINE_LEN];
  INT8      Line[MAX_LINE_LEN];
  INT8      *Processor;
  INT8      SymType;
  int       Len;
  int       ComponentCreated;
  int       ComponentFilePathAbsolute;
  int       DefineLine;
  DSC_FILE  ComponentFile;
  INT8      ComponentMakefileName[MAX_PATH];
  BOOLEAN   IsForFv;

  //
  // Now remove all local symbols
  //
  RemoveLocalSymbols ();
  //
  // Null out the file pointer in case we take an exception somewhere
  // and we need to close it only if we opened it.
  //
  MakeFptr                  = NULL;
  ComponentFilePathAbsolute = 0;
  ComponentCreated          = 0;
  //
  // Skip preceeding spaces on the line
  //
  while (isspace (*ArgLine) && (*ArgLine)) {
    ArgLine++;
  }
  //
  // Find the end of the component's filename and truncate the line at that
  // point. From here on out ArgLine is the name of the component filename.
  //
  Cptr = ArgLine;
  while (!isspace (*Cptr) && *Cptr) {
    Cptr++;
  }

  End = Cptr;
  if (*Cptr) {
    End++;
    *Cptr = 0;
  }
  //
  // Exception-handle processing of this component description file
  //
  TryException ();

  //
  // We also allow a component line format for defines of global symbols
  // instead of a component filename. In this case, the line looks like:
  // defines  x=abc y=yyy. Be nice and accept "define" and "defines" in a
  // case-insensitive manner. If it's defines, then make the symbols global.
  //
  if ((_stricmp (ArgLine, "define") == 0) || (_stricmp (ArgLine, "defines") == 0)) {
    SymType     = SYM_OVERWRITE | SYM_GLOBAL;
    DefineLine  = 1;
  } else {
    SymType     = SYM_OVERWRITE | SYM_LOCAL;
    DefineLine  = 0;
  }
  //
  // The rest of the component line from the DSC file should be defines
  //
  while (*End) {
    End = StripLine (End);
    if (*End) {
      //
      // If we're processing a "define abc=1 xyz=2" line, then set symbols
      // as globals per the SymType set above.
      //
      Len = AddSymbol (End, NULL, SymType);
      if (Len > 0) {
        End += Len;
      } else {
        Warning (NULL, 0, 0, ArgLine, "unrecognized option in description file");
        break;
      }
    }
  }

  //
  // If DEBUG_BREAK or EFI_BREAKPOINT is defined, then do a debug breakpoint.
  //
  if ((GetSymbolValue ("DEBUG_BREAK") != NULL) || (GetSymbolValue ("EFI_BREAKPOINT") != NULL)) {
    EFI_BREAKPOINT ();
  }
  
  //
  // If it's a define line, then we're done
  //
  if (DefineLine) {
    //
    // If there is NonFFS_FV, create the FVxxx.inf file
    // and include it in makefile.out. Remove the symbol
    // in order not to process it again next time
    //
    Cptr = GetSymbolValue (NONFFS_FV);
    if (Cptr != NULL) {
      NonFFSFVWriteInfFiles (DSCFile, Cptr);
      RemoveSymbol (NONFFS_FV, SYM_GLOBAL);
    }

    goto ComponentDone;
  }

  //
  // Expand symbols in the component description filename to expand the newly 
  // added local symbols
  //
  ExpandSymbols (ArgLine, Line, sizeof (Line), EXPANDMODE_NO_UNDEFS);
  
  //
  // If we have "c:\path\filename"
  //
  ReplaceSlash (Line);
  if (IsAbsolutePath (Line)) {
    ComponentFilePathAbsolute = 1;
  } else if (Line[0] == '.') {
    //
    // or if the path starts with ".", then it's build-dir relative.
    // Prepend $(BUILD_DIR) on the file name
    //
    sprintf (InLine, "%s\\%s", GetSymbolValue (BUILD_DIR), Line);
    strcpy (Line, InLine);
    ComponentFilePathAbsolute = 1;
  }
  
  //
  // Save the path from the component name for later. It may be relative or
  // absolute.
  //
  strcpy (ComponentFilePath, Line);
  Cptr = ComponentFilePath + strlen (ComponentFilePath) - 1;
  while ((*Cptr != '\\') && (Cptr != ComponentFilePath)) {
    Cptr--;
  }
  //
  // Terminate the path.
  //
  *Cptr = 0;
  
  //
  // Typically the given line is a component description filename. However we
  // also allow a FV filename (fvvariable.ffs COMPONENT_TYPE=FILE). If the
  // component type is "FILE", then add it to the FV list, create a package
  // file, and we're done.
  //
  Cptr = GetSymbolValue (COMPONENT_TYPE);
  if ((Cptr != NULL) && (strncmp (
                          Cptr,
                          COMPONENT_TYPE_FILE,
                          strlen (COMPONENT_TYPE_FILE)
                          ) == 0)) {
    if (ComponentFilePathAbsolute) {
      strcpy (InLine, Line);
    } else {
      sprintf (InLine, "%s\\%s", GetSymbolValue (EFI_SOURCE), Line);
    }
    CFVAddFVFile (
      InLine,
      Cptr,
      GetSymbolValue (FV),
      Instance,
      NULL,
      NULL,
      GetSymbolValue (APRIORI),
      NULL,
      NULL
      );
    goto ComponentDone;
  }

  //
  // Better have defined processor by this point.
  //
  Processor = GetSymbolValue (PROCESSOR);
  if (Processor == NULL) {
    Error (NULL, 0, 0, NULL, "PROCESSOR not defined for component %s", Line);
    return STATUS_ERROR;
  }

  //
  // The bin, out, and lib dirs are now = $(BUILD_DIR)/$(PROCESSOR). Set them.
  // Don't flag them as file paths (required for short 8.3 filenames) since
  // they're defined using the BUILD_DIR macro.
  //
  sprintf (InLine, "$(BUILD_DIR)\\%s", Processor);
  AddSymbol (BIN_DIR, InLine, SYM_LOCAL);
  AddSymbol (OUT_DIR, InLine, SYM_LOCAL);
  AddSymbol (LIB_DIR, InLine, SYM_LOCAL);
  //
  // See if it's been destined for an FV. It's possible to not be in an
  // FV if they just want to build it.
  //
  Cptr = GetSymbolValue (FV);
  if ((Cptr != NULL) && !IS_NULL_SYMBOL_VALUE (Cptr)) {
    IsForFv = TRUE;
  } else {
    IsForFv = FALSE;
  }
  //
  // As an optimization, if they've defined SKIP_FV_NULL as non-zero, and
  // the component is not destined for an FV, then skip it.
  // Since libraries are never intended for firmware volumes, we have to
  // build all of them.
  //
  if ((DscSectionType == DSC_SECTION_TYPE_COMPONENTS) && (IsForFv == FALSE)) {
    if ((GetSymbolValue (SKIP_FV_NULL) != NULL) && (atoi (GetSymbolValue (SKIP_FV_NULL)) != 0)) {
      Message (0, "%s not being built (FV=NULL)", FileName);
      goto ComponentDone;
    }
  }
  //
  // Prepend EFI_SOURCE to the component description file to get the
  // full path. Only do this if the path is not a full path already.
  //
  if (ComponentFilePathAbsolute == 0) {
    name = GetSymbolValue (EFI_SOURCE);
    sprintf (FileName, "%s\\%s", name, Line);
  } else {
    strcpy (FileName, Line);
  }
  //
  // Print a message, depending on verbose level.
  //
  if (DscSectionType == DSC_SECTION_TYPE_COMPONENTS) {
    Message (1, "Processing component         %s", FileName);
  } else {
    Message (1, "Processing library           %s", FileName);
  }
  //
  // Open the component's description file and get the sections. If we fail
  // to open it, see if they defined "OPTIONAL=1, in which case we'll just
  // ignore the component.
  //
  TempFptr = fopen (FileName, "r");
  if (TempFptr == NULL) {
    //
    // Better have defined OPTIONAL
    //
    if (GetSymbolValue (OPTIONAL_COMPONENT) != NULL) {
      if (atoi (GetSymbolValue (OPTIONAL_COMPONENT)) != 0) {
        Message (0, "Optional component '%s' not found", FileName);
        goto ComponentDone;
      }
    }

    ParserError (0, FileName, "failed to open component file");
    return STATUS_ERROR;
  } else {
    fclose (TempFptr);
  }

  DSCFileInit (&ComponentFile);
  ComponentCreated = 1;
  if (DSCFileSetFile (&ComponentFile, FileName)) {
    Error (NULL, 0, 0, NULL, "failed to preprocess component file '%s'", FileName);
    return STATUS_ERROR;
  }
  //
  // Add a symbol for the INF filename so users can create dependencies
  // in makefiles.
  //
  AddSymbol (INF_FILENAME, FileName, SYM_OVERWRITE | SYM_LOCAL | SYM_FILENAME);
  //
  // Process the [defines], [defines.$(PROCESSOR)], and [defines.$(PROCESSOR).$(PLATFORM)]
  // sections in the INF file
  //
  ProcessINFDefinesSection (&ComponentFile);
  //
  // Better have defined FILE_GUID if not a library
  //
  if ((GetSymbolValue (GUID) == NULL) &&
      (GetSymbolValue (FILE_GUID) == NULL) &&
      (DscSectionType == DSC_SECTION_TYPE_COMPONENTS)
      ) {
    Error (GetSymbolValue (INF_FILENAME), 1, 0, NULL, "missing FILE_GUID definition in component file");
    DSCFileDestroy (&ComponentFile);
    return STATUS_ERROR;
  }
  //
  // Better have defined base name
  //
  if (GetSymbolValue (BASE_NAME) == NULL) {
    Error (GetSymbolValue (INF_FILENAME), 1, 0, NULL, "missing BASE_NAME definition in INF file");
    DSCFileDestroy (&ComponentFile);
    return STATUS_ERROR;
  }
  //
  // Better have defined COMPONENT_TYPE, since it's used to find named sections.
  //
  if (GetSymbolValue (COMPONENT_TYPE) == NULL) {
    Error (GetSymbolValue (INF_FILENAME), 1, 0, NULL, "missing COMPONENT_TYPE definition in INF file");
    DSCFileDestroy (&ComponentFile);
    return STATUS_ERROR;
  }

  //
  // Create the source directory path from the component file's path. If the component
  // file's path is absolute, we may have problems here. Try to account for it though.
  //
  if (ComponentFilePathAbsolute == 0) {
    sprintf (
      FileName,
      "%s\\%s",
      GetSymbolValue (EFI_SOURCE),
      ComponentFilePath
      );
  } else {
    strcpy (FileName, ComponentFilePath);
  }
  AddSymbol (SOURCE_DIR, FileName, SYM_OVERWRITE | SYM_LOCAL | SYM_FILEPATH);

  //
  // Create the destination path. 
  // They may have defined DEST_DIR on the component INF line, so it's already
  // been defined, If that's the case, then don't set it to the path of this file.
  //
  TempCptr = GetSymbolValue (DEST_DIR);
  if (TempCptr == NULL) {
    if (ComponentFilePathAbsolute == 0) {
      //
      // The destination path is $(BUILD_DIR)\$(PROCESSOR)\component_path
      //
      sprintf (
        FileName,
        "%s\\%s\\%s",
        GetSymbolValue (BUILD_DIR),
        Processor,
        ComponentFilePath
        );
    } else {
      //
      // The destination path is $(BUILD_DIR)\$(PROCESSOR)\$(BASE_NAME)
      //
      sprintf (
        FileName,
        "%s\\%s\\%s",
        GetSymbolValue (BUILD_DIR),
        Processor,
        GetSymbolValue (BASE_NAME)
        );
    }
    AddSymbol (DEST_DIR, FileName, SYM_OVERWRITE | SYM_LOCAL | SYM_FILEPATH);
  } else {
    ReplaceSlash (TempCptr);
  }
  
  //
  // Create the output directory, then open the output component's makefile
  // we're going to create. Allow them to override the makefile name.
  //
  TempCptr = GetSymbolValue (MAKEFILE_NAME);
  if (TempCptr != NULL) {
    ExpandSymbols (TempCptr, ComponentMakefileName, sizeof (ComponentMakefileName), EXPANDMODE_NO_UNDEFS);
    TempCptr = ComponentMakefileName;
  } else {
    TempCptr = "makefile";
  }

  sprintf (FileName, "%s\\%s", GetSymbolValue (DEST_DIR), TempCptr);
  //
  // Save it now with path info
  //
  AddSymbol (MAKEFILE_NAME, FileName, SYM_OVERWRITE | SYM_LOCAL | SYM_FILENAME);

  if (MakeFilePath (FileName)) {
    return STATUS_ERROR;
  }

  if ((MakeFptr = fopen (FileName, "w")) == NULL) {
    Error (NULL, 0, 0, FileName, "could not create makefile");
    return STATUS_ERROR;
  }
  //
  // At this point we should have all the info we need to create a package
  // file if setup to do so. Libraries don't use package files, so
  // don't do this for libs.
  //
  if (DscSectionType == DSC_SECTION_TYPE_COMPONENTS) {
    CreatePackageFile (DSCFile);
  }

  //
  // Add a new build item to mCurrentBuildList
  //
  mCurrentBuildItem = AddBuildItem (mCurrentBuildList, GetSymbolValue (BASE_NAME), Processor, FileName);
  //
  // ProcessDsc allows duplicate base name libraries. Make sure the duplicate 
  // base name libraries will be built in the same order as listed in DSC file.
  //
  AddDependency (*mCurrentBuildList, mCurrentBuildItem, mCurrentBuildItem->BaseName, 1);

  //
  // Add Module name to the global module list
  //
  AddModuleName (&gGlobals.ModuleList, GetSymbolValue (BASE_NAME), GetSymbolValue (INF_FILENAME));
  //
  // Write an nmake line to makefile.out
  //
  fprintf (gGlobals.MakefileFptr, "  @cd %s\n", Processor);
  fprintf (gGlobals.MakefileFptr, "  $(MAKE) -f %s all\n", FileName);
  fprintf (gGlobals.MakefileFptr, "  @cd ..\n");

  //
  // Copy the common makefile section from the description file to
  // the component's makefile
  //
  WriteCommonMakefile (DSCFile, MakeFptr, Processor);
  //
  // Process the component's [nmake.common] and [nmake.$(PROCESSOR)] sections
  //
  ProcessINFNMakeSection (&ComponentFile, MakeFptr);
  //
  // Create the SOURCE_FILES macro that includes the names of all source
  // files in this component. This macro can then be used elsewhere to
  // process all the files making up the component. Required for scanning
  // files for string localization.
  // Also add source files to mCurrentBuildItem.
  //
  ProcessSourceFiles (DSCFile, &ComponentFile, MakeFptr, SOURCE_MODE_SOURCE_FILES);
  //
  // Create the include paths. Process [includes.common] and
  // [includes.$(PROCESSOR)] and [includes.$(PROCESSOR).$(PLATFORM)] sections.
  //
  ProcessIncludesSection (&ComponentFile, MakeFptr);
  //
  // Process all include source files to create a dependency list that can
  // be used in the makefile.
  //
  ProcessIncludeFiles (&ComponentFile, MakeFptr);
  //
  // Process the [sources.common], [sources.$(PROCESSOR)], and
  // [sources.$(PROCESSOR).$(PLATFORM)] files and emit their build commands
  //
  ProcessSourceFiles (DSCFile, &ComponentFile, MakeFptr, SOURCE_MODE_BUILD_COMMANDS);
  //
  // Process sources again to create an OBJECTS macro
  //
  ProcessObjects (&ComponentFile, MakeFptr);

  //
  // Add Single Module target : build and clean in top level makefile
  //
  fprintf (gGlobals.ModuleMakefileFptr, "%sbuild ::", GetSymbolValue (BASE_NAME));
  if (DscSectionType == DSC_SECTION_TYPE_COMPONENTS) {
    fprintf (gGlobals.ModuleMakefileFptr, " %sbuild", GLOBAL_LINK_LIB_NAME);
  }
  
  //
  // Process all the libraries to define "LIBS = x.lib y.lib..."
  // Be generous and append ".lib" if they forgot.
  // Make a macro definition: LIBS = $(LIBS) xlib.lib ylib.lib...
  // Add libs dependency for single module build: basenamebuild :: xlibbuild ylibbuild ...
  // Also add libs dependency to mCurrentBuildItem.
  //
  ProcessLibs (&ComponentFile, MakeFptr);

  fprintf (gGlobals.ModuleMakefileFptr, "\n");
  
  fprintf (gGlobals.ModuleMakefileFptr, "  @cd %s\n", Processor);
  fprintf (gGlobals.ModuleMakefileFptr, "  $(MAKE) -f %s all\n", FileName);
  fprintf (gGlobals.ModuleMakefileFptr, "  @cd ..\n\n");

  fprintf (gGlobals.ModuleMakefileFptr, "%sclean ::\n", GetSymbolValue (BASE_NAME));
  fprintf (gGlobals.ModuleMakefileFptr, "  $(MAKE) -f %s clean\n\n", FileName);
  
  //
  // Emit commands to create the component. These are simply copied from
  // the description file to the component's makefile. First look for
  // [build.$(PROCESSOR).$(BUILD_TYPE)]. If not found, then look for if
  // find a [build.$(PROCESSOR).$(COMPONENT_TYPE)] line.
  //
  Cptr = GetSymbolValue (BUILD_TYPE);
  if (Cptr != NULL) {
    sprintf (InLine, "build.%s.%s", Processor, Cptr);
    WriteComponentTypeBuildCommands (DSCFile, MakeFptr, InLine);
  } else {
    sprintf (InLine, "build.%s.%s", Processor, GetSymbolValue (COMPONENT_TYPE));
    WriteComponentTypeBuildCommands (DSCFile, MakeFptr, InLine);
  }
  //
  // Add it to the FV if not a library
  //
  if (DscSectionType == DSC_SECTION_TYPE_COMPONENTS) {
    //
    // Create the FV filename and add it to the FV.
    // By this point we know it's in FV.
    //
    Cptr = GetSymbolValue (FILE_GUID);
    if (Cptr == NULL) {
      Cptr = GetSymbolValue (GUID);
    }

    sprintf (InLine, "%s-%s", Cptr, GetSymbolValue (BASE_NAME));
    //
    // We've deprecated FV_EXT, which should be FFS_EXT, the extension
    // of the FFS file generated by GenFFSFile.
    //
    TempCptr = GetSymbolValue (FFS_EXT);
    if (TempCptr == NULL) {
      TempCptr = GetSymbolValue ("FV_EXT");
    }

    CFVAddFVFile (
      InLine,
      GetSymbolValue (COMPONENT_TYPE),
      GetSymbolValue (FV),
      Instance,
      TempCptr,
      Processor,
      GetSymbolValue (APRIORI),
      GetSymbolValue (BASE_NAME),
      Cptr
      );
  }
  //
  // Catch any failures and print the name of the component file
  // being processed to assist debugging.
  //
ComponentDone:

  Cptr = CatchException ();
  if (Cptr != NULL) {
    fprintf (stderr, "%s\n", Cptr);
    sprintf (InLine, "Processing of component %s failed", ArgLine);
    ThrowException (InLine);
  }

  if (MakeFptr != NULL) {
    fclose (MakeFptr);
  }

  if (ComponentCreated) {
    DSCFileDestroy (&ComponentFile);
  }

  return STATUS_SUCCESS;
}

static
int
CreatePackageFile (
  DSC_FILE          *DSCFile
  )
{
  INT8       *Package;
  SECTION    *TempSect;
  INT8       Str[MAX_LINE_LEN];
  INT8       StrExpanded[MAX_LINE_LEN];
  SMART_FILE *PkgFptr;
  int        Status;

  PkgFptr = NULL;

  //
  // First find out if PACKAGE_FILENAME or PACKAGE is defined. PACKAGE_FILENAME
  // is used to specify the exact package file to use. PACKAGE is used to
  // specify the package section name.
  //
  Package = GetSymbolValue (PACKAGE_FILENAME);
  if (Package != NULL) {
    //
    // Use existing file. We're done.
    //
    return STATUS_SUCCESS;
  }
  //
  // See if PACKAGE or PACKAGE_TAG is defined
  //
  Package = GetSymbolValue (PACKAGE);
  if (Package == NULL) {
    Package = GetSymbolValue (PACKAGE_TAG);
  }

  if (Package == NULL) {
    //
    // Not defined either. Assume they are not using the package functionality
    // of this utility. However define the PACKAGE_FILENAME macro to the
    // best-guess value.
    //
    sprintf (
      Str,
      "%s\\%s.pkg",
      GetSymbolValue (SOURCE_DIR),
      GetSymbolValue (BASE_NAME)
      );
 
    //
    // Expand symbols in the package filename
    //
    ExpandSymbols (Str, StrExpanded, sizeof (StrExpanded), EXPANDMODE_NO_UNDEFS);    
 
    AddSymbol (PACKAGE_FILENAME, StrExpanded, SYM_LOCAL | SYM_FILENAME);
    return STATUS_SUCCESS;
  }
  //
  // Save the position in the DSC file.
  // Find the [package.$(COMPONENT_TYPE).$(PACKAGE)] section in the DSC file
  //
  Status = STATUS_SUCCESS;
  DSCFileSavePosition (DSCFile);
  sprintf (Str, "%s.%s.%s", PACKAGE, GetSymbolValue (COMPONENT_TYPE), Package);
  TempSect = DSCFileFindSection (DSCFile, Str);
  if (TempSect != NULL) {
    //
    // So far so good. Create the name of the package file, then open it up
    // for writing. File name is c:\...\oem\platform\nt32\ia32\...\BaseName.pkg.
    //
    sprintf (
      Str,
      "%s\\%s.pkg",
      GetSymbolValue (DEST_DIR),
      GetSymbolValue (BASE_NAME)
      );
    
    //
    // Expand symbols in the package filename
    //
    ExpandSymbols (Str, StrExpanded, sizeof (StrExpanded), EXPANDMODE_NO_UNDEFS);    
    
    //
    // Try to open the file, then save the file name as the PACKAGE_FILENAME
    // symbol for use elsewhere.
    //
    if ((PkgFptr = SmartOpen (StrExpanded)) == NULL) {
      Error (NULL, 0, 0, Str, "could not open package file for writing");
      Status = STATUS_ERROR;
      goto Finish;
    }

    AddSymbol (PACKAGE_FILENAME, StrExpanded, SYM_LOCAL | SYM_FILENAME);
    //
    // Now read lines in from the DSC file and write them back out to the
    // package file (with string substitution).
    //
    while (DSCFileGetLine (DSCFile, Str, sizeof (Str)) != NULL) {
      //
      // Expand symbols, then write the line out to the package file
      //
      ExpandSymbols (Str, StrExpanded, sizeof (StrExpanded), EXPANDMODE_RECURSIVE);
      SmartWrite (PkgFptr, StrExpanded);
    }
  } else {
    Warning (
      NULL,
      0,
      0,
      NULL,
      "cannot locate package section [%s] in DSC file for %s",
      Str,
      GetSymbolValue (INF_FILENAME)
      );
    Status = STATUS_WARNING;
    goto Finish;
  }

  if (PkgFptr != NULL) {
    SmartClose (PkgFptr);
  }

Finish:
  //
  // Restore the position in the DSC file
  //
  DSCFileRestorePosition (DSCFile);

  return STATUS_SUCCESS;
}

static
int
ProcessINFDefinesSection (
  DSC_FILE   *ComponentFile
  )
/*++

Routine Description:

  Process the [defines.xxx] sections of the component description file. Process
  platform first, then processor. In this way, if a platform wants and override,
  that one gets parsed first, and later assignments do not overwrite the value.
  
Arguments:

  ComponentFile     - section info on the component file being processed

Returns:

 
--*/
{
  INT8  *Cptr;
  INT8  Str[MAX_LINE_LEN];

  //
  // Find a [defines.$(PROCESSOR).$(PLATFORM)] section and process it
  //
  Cptr = GetSymbolValue (PLATFORM);
  if (Cptr != NULL) {
    sprintf (
      Str,
      "%s.%s.%s",
      DEFINES_SECTION_NAME,
      GetSymbolValue (PROCESSOR),
      Cptr
      );
    ProcessINFDefinesSectionSingle (ComponentFile, Str);
  }
  //
  // Find a [defines.$(PROCESSOR)] section and process it
  //
  sprintf (Str, "%s.%s", DEFINES_SECTION_NAME, GetSymbolValue (PROCESSOR));
  ProcessINFDefinesSectionSingle (ComponentFile, Str);

  //
  // Find a [defines] section and process it
  //
  if (ProcessINFDefinesSectionSingle (ComponentFile, DEFINES_SECTION_NAME) != STATUS_SUCCESS) {
    Error (NULL, 0, 0, NULL, "missing [defines] section in component file %s", GetSymbolValue (INF_FILENAME));
    return STATUS_ERROR;
  }

  return STATUS_SUCCESS;
}

static
int
ProcessINFDefinesSectionSingle (
  DSC_FILE  *ComponentFile,
  INT8      *SectionName
  )
{
  INT8    *Cptr;
  INT8    Str[MAX_LINE_LEN];
  INT8    ExpandedLine[MAX_LINE_LEN];
  SECTION *TempSect;

  TempSect = DSCFileFindSection (ComponentFile, SectionName);
  if (TempSect != NULL) {
    while (DSCFileGetLine (ComponentFile, Str, sizeof (Str)) != NULL) {
      ExpandSymbols (Str, ExpandedLine, sizeof (ExpandedLine), 0);
      Cptr = StripLine (ExpandedLine);
      //
      // Don't process blank lines.
      //
      if (*Cptr) {
        //
        // Add without overwriting macros specified on the component line
        // in the description file
        //
        AddSymbol (Cptr, NULL, SYM_LOCAL);
      }
    }
  } else {
    return STATUS_WARNING;
  }

  return STATUS_SUCCESS;
}

static
int
ProcessINFNMakeSection (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr
  )
/*++

Routine Description:

  Process the [nmake.common] and [nmake.$(PROCESSOR)] sections of the component
  description file and write and copy them to the component's makefile.
  
Arguments:

  ComponentFile     - section info on the component file being processed
  MakeFptr          - file pointer to the component' makefile we're creating

Returns:

  Always STATUS_SUCCESS right now, since the sections are optional.
  
--*/
{
  INT8    *Cptr;
  INT8    Str[MAX_LINE_LEN];
  INT8    ExpandedLine[MAX_LINE_LEN];
  SECTION *TempSect;

  //
  // Copy the [nmake.common] and [nmake.$(PROCESSOR)] sections from the
  // component file directly to the output file.
  // The line will be stripped and don't print blank lines
  //
  sprintf (Str, "%s.%s", NMAKE_SECTION_NAME, COMMON_SECTION_NAME);
  TempSect = DSCFileFindSection (ComponentFile, Str);
  if (TempSect != NULL) {
    while (DSCFileGetLine (ComponentFile, Str, sizeof (Str)) != NULL) {
      ExpandSymbols (
        Str,
        ExpandedLine,
        sizeof (ExpandedLine),
        EXPANDMODE_NO_DESTDIR | EXPANDMODE_NO_SOURCEDIR
        );
      Cptr = StripLine (ExpandedLine);
      if (*Cptr) {
        fprintf (MakeFptr, "%s\n", Cptr);
      }
    }

    fprintf (MakeFptr, "\n");
  } else {
    Error (GetSymbolValue (INF_FILENAME), 1, 0, Str, "section not found in component INF file");
  }

  sprintf (Str, "%s.%s", NMAKE_SECTION_NAME, GetSymbolValue (PROCESSOR));
  TempSect = DSCFileFindSection (ComponentFile, Str);
  if (TempSect != NULL) {
    while (DSCFileGetLine (ComponentFile, Str, sizeof (Str)) != NULL) {
      ExpandSymbols (
        Str,
        ExpandedLine,
        sizeof (ExpandedLine),
        EXPANDMODE_NO_DESTDIR | EXPANDMODE_NO_SOURCEDIR
        );
      Cptr = StripLine (ExpandedLine);
      if (*Cptr) {
        fprintf (MakeFptr, "%s\n", Cptr);
      }
    }

    fprintf (MakeFptr, "\n");
  }
  //
  // Do the same for [nmake.$(PROCESSOR).$(PLATFORM)]
  //
  Cptr = GetSymbolValue (PLATFORM);
  if (Cptr != NULL) {
    sprintf (Str, "%s.%s.%s", NMAKE_SECTION_NAME, GetSymbolValue (PROCESSOR), Cptr);
    TempSect = DSCFileFindSection (ComponentFile, Str);
    if (TempSect != NULL) {
      while (DSCFileGetLine (ComponentFile, Str, sizeof (Str)) != NULL) {
        ExpandSymbols (
          Str,
          ExpandedLine,
          sizeof (ExpandedLine),
          EXPANDMODE_NO_DESTDIR | EXPANDMODE_NO_SOURCEDIR
          );
        Cptr = StripLine (ExpandedLine);
        if (*Cptr) {
          fprintf (MakeFptr, "%s\n", Cptr);
        }
      }

      fprintf (MakeFptr, "\n");
    }
  }

  return STATUS_SUCCESS;
}

static
int
ProcessIncludesSection (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr
  )
/*++

Routine Description:

  Process the [includes.common], [includes.processor], and 
  [includes.processor.platform] section of the component description file 
  and write the appropriate macros to the component's makefile.

  Process in reverse order to allow overrides on platform basis.
    
Arguments:

  ComponentFile     - section info on the component file being processed
  MakeFptr          - file pointer to the component' makefile we're creating

Returns:

  Always STATUS_SUCCESS right now, since the sections are optional.
  
--*/
{
  INT8  *Cptr;
  INT8  Str[MAX_LINE_LEN];
  INT8  *Processor;
  INT8  *OverridePath;

  //
  // Write a useful comment to the output makefile so the user knows where
  // the data came from.
  //
  fprintf (MakeFptr, "#\n# Tool-generated list of include paths that are created\n");
  fprintf (MakeFptr, "# from the list of include paths in the [includes.*] sections\n");
  fprintf (MakeFptr, "# of the component INF file.\n#\n");

  //
  // We use this a lot here, so get the value only once.
  //
  Processor = GetSymbolValue (PROCESSOR);
  //
  // If they're using an override source path, then add OverridePath and
  // OverridePath\$(PROCESSOR) to the list of include paths.
  //
  OverridePath = GetSymbolValue (SOURCE_OVERRIDE_PATH);
  if (OverridePath != NULL) {
    ReplaceSlash (OverridePath);
    fprintf (MakeFptr, "!IF EXIST(%s)\n", OverridePath);
    fprintf (MakeFptr, "INC = $(INC) -I %s\n", OverridePath);
    fprintf (MakeFptr, "!IF EXIST(%s\\%s)\n", OverridePath, Processor);
    fprintf (MakeFptr, "INC = $(INC) -I %s\\%s\n", OverridePath, Processor);
    fprintf (MakeFptr, "!ENDIF\n");
    fprintf (MakeFptr, "!ELSE\n");
    fprintf (MakeFptr, "!MESSAGE Warning: include dir %s does not exist\n", OverridePath);
    fprintf (MakeFptr, "!ENDIF\n");
  }
  //
  // Try for an [includes.$(PROCESSOR).$(PLATFORM)]
  //
  Cptr = GetSymbolValue (PLATFORM);
  if (Cptr != NULL) {
    sprintf (Str, "%s.%s.%s", INCLUDE_SECTION_NAME, Processor, Cptr);
    ProcessIncludesSectionSingle (ComponentFile, MakeFptr, Str);
  }
  //
  // Now the [includes.$(PROCESSOR)] section
  //
  sprintf (Str, "%s.%s", INCLUDE_SECTION_NAME, Processor);
  ProcessIncludesSectionSingle (ComponentFile, MakeFptr, Str);

  //
  // Now the [includes.common] section
  //
  sprintf (Str, "%s.%s", INCLUDE_SECTION_NAME, COMMON_SECTION_NAME);
  ProcessIncludesSectionSingle (ComponentFile, MakeFptr, Str);

  //
  // Done
  //
  fprintf (MakeFptr, "\n");
  return STATUS_SUCCESS;
}
//
// Process one of the [includes.xxx] sections to create a list of all
// the include paths.
//
static
int
ProcessIncludesSectionSingle (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  INT8      *SectionName
  )
{
  INT8    *Cptr;
  SECTION *TempSect;
  INT8    Str[MAX_LINE_LEN];
  INT8    ExpandedLine[MAX_LINE_LEN];
  INT8    *Processor;

  TempSect = DSCFileFindSection (ComponentFile, SectionName);
  if (TempSect != NULL) {
    //
    // Add processor subdirectory on every include path
    //
    Processor = GetSymbolValue (PROCESSOR);
    //
    // Copy lines directly
    //
    while (DSCFileGetLine (ComponentFile, Str, sizeof (Str)) != NULL) {
      ExpandSymbols (Str, ExpandedLine, sizeof (ExpandedLine), 0);
      Cptr = StripLine (ExpandedLine);
      //
      // Don't process blank lines
      //
      if (*Cptr) {
        ReplaceSlash (Cptr);
        //
        // Strip off trailing slash
        //
        if (Cptr[strlen (Cptr) - 1] == '\\') {
          Cptr[strlen (Cptr) - 1] = 0;
        }
        //
        // Special case of ".". Replace it with source path
        // and the rest of the line (for .\$(PROCESSOR))
        //
        if (*Cptr == '.') {
          //
          // Handle case of just a "."
          //
          if (Cptr[1] == 0) {
            fprintf (MakeFptr, "INC = $(INC) -I $(SOURCE_DIR)\n");
            fprintf (MakeFptr, "!IF EXIST($(SOURCE_DIR)\\%s)\n", Processor);
            fprintf (MakeFptr, "INC = $(INC) -I $(SOURCE_DIR)\\%s\n", Processor);
            fprintf (MakeFptr, "!ENDIF\n");
          } else {
            //
            // Handle case of ".\path\path\path" or "..\path\path\path"
            //
            fprintf (MakeFptr, "!IF EXIST($(SOURCE_DIR)\\%s)\n", Cptr);
            fprintf (MakeFptr, "INC = $(INC) -I $(SOURCE_DIR)\\%s\n", Cptr);
            fprintf (MakeFptr, "!IF EXIST($(SOURCE_DIR)\\%s\\%s)\n", Cptr, Processor);
            fprintf (MakeFptr, "INC = $(INC) -I $(SOURCE_DIR)\\%s\\%s\n", Cptr, Processor);
            fprintf (MakeFptr, "!ENDIF\n");
            fprintf (MakeFptr, "!ELSE\n");
            fprintf (MakeFptr, "!MESSAGE Warning: include dir $(SOURCE_DIR)\\%s does not exist\n", Cptr);
            fprintf (MakeFptr, "!ENDIF\n");
          }
        } else if ((Cptr[1] != ':') && isalpha (*Cptr)) {
          fprintf (MakeFptr, "!IF EXIST($(EFI_SOURCE)\\%s)\n", Cptr);
          fprintf (MakeFptr, "INC = $(INC) -I $(EFI_SOURCE)\\%s\n", Cptr);
          fprintf (MakeFptr, "!IF EXIST($(EFI_SOURCE)\\%s\\%s)\n", Cptr, Processor);
          fprintf (MakeFptr, "INC = $(INC) -I $(EFI_SOURCE)\\%s\\%s\n", Cptr, Processor);
          fprintf (MakeFptr, "!ENDIF\n");
          fprintf (MakeFptr, "!ELSE\n");
          fprintf (MakeFptr, "!MESSAGE Warning: include dir $(EFI_SOURCE)\\%s does not exist\n", Cptr);
          fprintf (MakeFptr, "!ENDIF\n");
        } else {
          //
          // The line is something like: $(EFI_SOURCE)\dxe\include. Add it to
          // the existing $(INC) definition. Add user includes before any
          // other existing paths.
          //
          fprintf (MakeFptr, "!IF EXIST(%s)\n", Cptr);
          fprintf (MakeFptr, "INC = $(INC) -I %s\n", Cptr);
          fprintf (MakeFptr, "!IF EXIST(%s\\%s)\n", Cptr, Processor);
          fprintf (MakeFptr, "INC = $(INC) -I %s\\%s\n", Cptr, Processor);
          fprintf (MakeFptr, "!ENDIF\n");
          fprintf (MakeFptr, "!ELSE\n");
          fprintf (MakeFptr, "!MESSAGE Warning: include dir %s does not exist\n", Cptr);
          fprintf (MakeFptr, "!ENDIF\n");
        }
      }
    }
  }

  return STATUS_SUCCESS;
}

static
int
ProcessSourceFiles (
  DSC_FILE  *DSCFile,
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  UINT32    Mode
  )
/*++

Routine Description:

  Process the [sources.common], [sources.$(PROCESSOR)], and 
  [sources.$(PROCESSOR).$(PLATFORM] sections of the component
  description file and write the appropriate build commands out to the 
  component's makefile. If $(SOURCE_SELECT) is defined, then it overrides
  the source selections. We use this functionality for SMM.
  
Arguments:

  ComponentFile     - section info on the component file being processed
  MakeFptr          - file pointer to the component' makefile we're creating
  DSCFile           - section info on the description file we're processing
  Mode              - to write build commands, or just create a list
                      of sources.

Returns:

  Always STATUS_SUCCESS right now, since the sections are optional.
  
--*/
{
  INT8  Str[MAX_LINE_LEN];
  INT8  *Processor;
  INT8  *Platform;
  INT8  *SourceSelect;
  INT8  *CStart;
  INT8  *CEnd;
  INT8  CSave;
  INT8  *CopySourceSelect;

  if (Mode & SOURCE_MODE_SOURCE_FILES) {
    //
    // Write a useful comment to the output makefile so the user knows where
    // the data came from.
    //
    fprintf (MakeFptr, "#\n# Tool-generated list of source files that are created\n");
    fprintf (MakeFptr, "# from the list of source files in the [sources.*] sections\n");
    fprintf (MakeFptr, "# of the component INF file.\n#\n");
  }
  
  //
  // We use this a lot here, so get the value only once.
  //
  Processor = GetSymbolValue (PROCESSOR);
  //
  // See if they defined SOURCE_SELECT=xxx,yyy in which case we'll
  // select each [sources.xxx] and [sources.yyy] files and process
  // them.
  //
  SourceSelect = GetSymbolValue (SOURCE_SELECT);

  if (SourceSelect != NULL) {
    //
    // Make a copy of the string and break it up (comma-separated) and
    // select each [sources.*] file from the INF.
    //
    CopySourceSelect = (INT8 *) malloc (strlen (SourceSelect) + 1);
    if (CopySourceSelect == NULL) {
      Error (NULL, 0, 0, NULL, "failed to allocate memory");
      return STATUS_ERROR;
    }

    strcpy (CopySourceSelect, SourceSelect);
    CStart  = CopySourceSelect;
    CEnd    = CStart;
    while (*CStart) {
      CEnd = CStart + 1;
      while (*CEnd && *CEnd != ',') {
        CEnd++;
      }

      CSave = *CEnd;
      *CEnd = 0;
      sprintf (Str, "%s.%s", SOURCES_SECTION_NAME, CStart);
      ProcessSourceFilesSection (DSCFile, ComponentFile, MakeFptr, Str, Mode);
      //
      // Restore the terminator and advance
      //
      *CEnd   = CSave;
      CStart  = CEnd;
      if (*CStart) {
        CStart++;
      }
    }

    free (CopySourceSelect);

  } else {
    //
    // Process all the [sources.common] source files to make them build
    //
    sprintf (Str, "%s.%s", SOURCES_SECTION_NAME, COMMON_SECTION_NAME);
    ProcessSourceFilesSection (DSCFile, ComponentFile, MakeFptr, Str, Mode);
    //
    // Now process the [sources.$(PROCESSOR)] files.
    //
    sprintf (Str, "sources.%s", Processor);
    ProcessSourceFilesSection (DSCFile, ComponentFile, MakeFptr, Str, Mode);
    //
    // Now process the [sources.$(PROCESSOR).$(PLATFORM)] files.
    //
    Platform = GetSymbolValue (PLATFORM);
    if (Platform != NULL) {
      sprintf (Str, "sources.%s.%s", Processor, Platform);
      ProcessSourceFilesSection (DSCFile, ComponentFile, MakeFptr, Str, Mode);
    }
  }

  fprintf (MakeFptr, "\n");
  return STATUS_SUCCESS;
}

/*++

Routine Description:
  Given a source file line from an INF file, parse it to see if there are
  any defines on it. If so, then add them to the symbol table.
  Also, terminate the line after the file name.
  
Arguments:
  SourceFileLine - a line from a [sources.?] section of the INF file. Likely
  something like:
  
  MySourceFile.c   BUILT_NAME=$(BUILD_DIR)\MySourceFile.obj

Returns:
  Nothing.
  
--*/
static
void
AddFileSymbols (
  INT8    *SourceFileLine
  )
{
  int Len;
  //
  // Skip spaces
  //
  for (; *SourceFileLine && isspace (*SourceFileLine); SourceFileLine++)
    ;
  for (; *SourceFileLine && !isspace (*SourceFileLine); SourceFileLine++)
    ;
  if (*SourceFileLine) {
    *SourceFileLine = 0;
    SourceFileLine++;
    //
    // AddSymbol() will parse it for us, and return the length. Keep calling
    // it until it reports an error or is done.
    //
    do {
      Len = AddSymbol (SourceFileLine, NULL, SYM_FILE);
      SourceFileLine += Len;
    } while (Len > 0);
  }
}
//
// Process a single section of source files in the component INF file
//
static
int
ProcessSourceFilesSection (
  DSC_FILE  *DSCFile,
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  INT8      *SectionName,
  UINT32    Mode
  )
{
  INT8    *Cptr;
  INT8    FileName[MAX_EXP_LINE_LEN];
  INT8    FilePath[MAX_PATH];
  INT8    TempFileName[MAX_PATH];
  SECTION *TempSect;
  INT8    Str[MAX_LINE_LEN];
  INT8    *Processor;
  INT8    *OverridePath;
  FILE    *FPtr;

  TempSect = DSCFileFindSection (ComponentFile, SectionName);
  if (TempSect != NULL) {
    Processor = GetSymbolValue (PROCESSOR);
    while (DSCFileGetLine (ComponentFile, Str, sizeof (Str)) != NULL) {
      Cptr = StripLine (Str);
      //
      // Don't process blank lines
      //
      if (*Cptr) {
        //
        // Expand symbols in the filename, then parse the line for symbol
        // definitions. AddFileSymbols() will null-terminate the line
        // after the file name. Save a copy for override purposes, in which
        // case we'll need to know the file name and path (in case it's in
        // a subdirectory).
        //
        ExpandSymbols (Cptr, FileName, sizeof (FileName), 0);
        AddFileSymbols (FileName);
        ReplaceSlash (FileName);
        //
        // Set the SOURCE_FILE_NAME symbol. What we have now is the name of
        // the file, relative to the location of the INF file. So prepend
        // $(SOURCE_DIR) to it first.
        //
        if (IsAbsolutePath (FileName)) {
          strcpy (TempFileName, FileName);
        } else {
          strcpy (TempFileName, "$(SOURCE_DIR)\\");
          strcat (TempFileName, FileName);
        }
        AddSymbol (SOURCE_FILE_NAME, TempFileName, SYM_FILE | SYM_OVERWRITE);
        //
        // Extract path information from the source file and set internal
        // variable SOURCE_RELATIVE_PATH. Only do this if the path
        // contains a backslash.
        //
        strcpy (FilePath, FileName);
        for (Cptr = FilePath + strlen (FilePath) - 1; (Cptr > FilePath) && (*Cptr != '\\'); Cptr--)
          ;
        if (*Cptr == '\\') {
          *(Cptr + 1) = 0;
          AddSymbol (SOURCE_RELATIVE_PATH, FilePath, SYM_FILE);
        }
        //
        // Define another internal symbol for the name of the file without
        // the path and extension.
        //
        for (Cptr = FileName + strlen (FileName) - 1; (Cptr > FileName) && (*Cptr != '\\'); Cptr--)
          ;
        if (*Cptr == '\\') {
          Cptr++;
        }

        strcpy (FilePath, Cptr);
        //
        // We now have a file name with no path information. Before we do anything else,
        // see if OVERRIDE_PATH is set, and if so, see if file $(OVERRIDE_PATH)FileName
        // exists. If it does, then recursive call this function to use the override file
        // instead of the one from the INF file.
        //
        if (IsAbsolutePath (FileName)) {
          OverridePath = NULL;
        } else {
          OverridePath = GetSymbolValue (SOURCE_OVERRIDE_PATH);
        }
        if (OverridePath != NULL) {
          ReplaceSlash (OverridePath);
          //
          // See if the file exists. If it does, reset the SOURCE_FILE_NAME symbol.
          //
          strcpy (TempFileName, OverridePath);
          strcat (TempFileName, "\\");
          strcat (TempFileName, FileName);
          if ((FPtr = fopen (TempFileName, "rb")) != NULL) {
            fclose (FPtr);
            AddSymbol (SOURCE_FILE_NAME, TempFileName, SYM_FILE | SYM_OVERWRITE);
            //
            // Print a message. This function is called to create build commands
            // for source files, and to create a macro of all source files. Therefore
            // do this check so we don't print the override message multiple times.
            //
            if (Mode & SOURCE_MODE_BUILD_COMMANDS) {
              fprintf (stdout, "Override: %s\n", TempFileName);
            }
          } else {
            //
            // Set override path to null to use as a flag below
            //
            OverridePath = NULL;
          }
        }

        //
        // Start at the end and work back
        //
        for (Cptr = FilePath + strlen (FilePath) - 1; (Cptr > FilePath) && (*Cptr != '\\') && (*Cptr != '.'); Cptr--)
          ;
        if (*Cptr == '.') {
          *Cptr = 0;
          AddSymbol (SOURCE_FILE_EXTENSION, Cptr + 1, SYM_FILE);
        }

        AddSymbol (SOURCE_BASE_NAME, FilePath, SYM_FILE);
        //
        // If we're just creating the SOURCE_FILES macro, then write the
        // file name out to the makefile.
        //
        if (Mode & SOURCE_MODE_SOURCE_FILES) {
          //
          // If we're processing an override file, then use the file name as-is
          //
          if (OverridePath != NULL) {
            //
            // SOURCE_FILES = $(SOURCE_FILES) c:\Path\ThisFile.c
            //
            fprintf (MakeFptr, "SOURCE_FILES = $(SOURCE_FILES) %s\n", TempFileName);
            //
            // Save the source absolute path
            //
            if (PathCanonicalize (FilePath, TempFileName)) {
              AddSourceFile (mCurrentBuildItem, FilePath);
            }
          } else if (IsAbsolutePath (FileName)) {
            //
            // For Absolute path, don't print $(SOURCE_FILE) directory.
            //
            fprintf (MakeFptr, "SOURCE_FILES = $(SOURCE_FILES) %s\n", FileName);
            //
            // Save the source absolute path
            //
            if (PathCanonicalize (FilePath, FileName)) {
              AddSourceFile (mCurrentBuildItem, FilePath);
            }
          } else {
            //
            // SOURCE_FILES = $(SOURCE_FILES) $(SOURCE_DIR)\ThisFile.c
            //
            fprintf (MakeFptr, "SOURCE_FILES = $(SOURCE_FILES) $(SOURCE_DIR)\\%s\n", FileName);
            //
            // Save the source absolute path
            //
            sprintf (Str, "%s\\%s", GetSymbolValue (SOURCE_DIR), FileName);
            if (PathCanonicalize (FilePath, Str)) {
              AddSourceFile (mCurrentBuildItem, FilePath);
            }
          }
        } else if (Mode & SOURCE_MODE_BUILD_COMMANDS) {
          //
          // Write the build commands for this file per the build commands
          // for this file type as defined in the description file.
          // Also create the directory for it in the build path.
          //
          WriteCompileCommands (DSCFile, MakeFptr, FileName, Processor);
          if (!IsAbsolutePath (FileName)) {
            sprintf (Str, "%s\\%s", GetSymbolValue (DEST_DIR), FileName);
            MakeFilePath (Str);
            //
            // Get all output directory for build output files.
            //
            Cptr = FileName + strlen (FileName) - 1;
            for (; (Cptr > FileName) && (*Cptr != '\\'); Cptr--);
            if (*Cptr == '\\') {
              *Cptr = '\0';
              AddModuleName (&gGlobals.OutdirList, FileName, NULL);
            }
          }
        }
        //
        // Remove file-level symbols
        //
        RemoveFileSymbols ();
      }
    }
  }

  return STATUS_SUCCESS;
}
//
// Process the INF [sources.*] sections and emit the OBJECTS = .....
// lines to the component's makefile.
//
static
int
ProcessObjects (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr
  )
{
  INT8  Str[MAX_LINE_LEN];
  INT8  *Processor;
  INT8  *Platform;
  INT8  *SourceSelect;
  INT8  *CStart;
  INT8  *CEnd;
  INT8  CSave;
  INT8  *CopySourceSelect;
  SYMBOL *TempSymbol;

  //
  // Write a useful comment to the output makefile so the user knows where
  // the data came from.
  //
  fprintf (MakeFptr, "#\n# Tool-generated list of object files that are created\n");
  fprintf (MakeFptr, "# from the list of source files in the [sources.*] sections\n");
  fprintf (MakeFptr, "# of the component INF file.\n#\n");
  //
  // We use this a lot here, so get the value only once.
  //
  Processor = GetSymbolValue (PROCESSOR);
  //
  // Now define the OBJECTS variable and assign it to be all the object files we're going
  // to create. Afterwards create a pseudo-target objects to let the user quickly just compile
  // the source files. This means we need to process all the common objects and
  // processor-specific objects again.
  //
  fprintf (MakeFptr, "OBJECTS = $(OBJECTS) ");
  //
  // See if they defined SOURCE_SELECT=xxx,yyy in which case well
  // select each [sources.xxx] and [sources.yyy] files and process
  // them.
  //
  SourceSelect = GetSymbolValue (SOURCE_SELECT);

  if (SourceSelect != NULL) {
    //
    // Make a copy of the string and break it up (comma-separated) and
    // select each [sources.*] file from the INF.
    //
    CopySourceSelect = (INT8 *) malloc (strlen (SourceSelect) + 1);
    if (CopySourceSelect == NULL) {
      Error (NULL, 0, 0, NULL, "failed to allocate memory");
      return STATUS_ERROR;
    }

    strcpy (CopySourceSelect, SourceSelect);
    CStart  = CopySourceSelect;
    CEnd    = CStart;
    while (*CStart) {
      CEnd = CStart + 1;
      while (*CEnd && *CEnd != ',') {
        CEnd++;
      }

      CSave = *CEnd;
      *CEnd = 0;
      sprintf (Str, "%s.%s", SOURCES_SECTION_NAME, CStart);
      ProcessObjectsSingle (ComponentFile, MakeFptr, Str);
      //
      // Restore the terminator and advance
      //
      *CEnd   = CSave;
      CStart  = CEnd;
      if (*CStart) {
        CStart++;
      }
    }

    free (CopySourceSelect);
  
  } else {
    //
    // Now process all the [sources.common] files and emit build commands for them
    //
    sprintf (Str, "%s.%s", SOURCES_SECTION_NAME, COMMON_SECTION_NAME);
    if (ProcessObjectsSingle (ComponentFile, MakeFptr, Str) != STATUS_SUCCESS) {
      Warning (GetSymbolValue (INF_FILENAME), 1, 0, NULL, "no [%s] section found in component description", Str);
    }
    //
    // Now process any processor-specific source files in [sources.$(PROCESSOR)]
    //
    sprintf (Str, "%s.%s", SOURCES_SECTION_NAME, Processor);
    ProcessObjectsSingle (ComponentFile, MakeFptr, Str);

    //
    // Now process any [sources.$(PROCESSOR).$(PLATFORM)] files
    //
    Platform = GetSymbolValue (PLATFORM);
    if (Platform != NULL) {
      sprintf (Str, "sources.%s.%s", Processor, Platform);
      ProcessObjectsSingle (ComponentFile, MakeFptr, Str);
    }
  }

  fprintf (MakeFptr, "\n\n");

  //
  // Write a useful comment to the output makefile so the user knows where
  // the data came from.
  //
  fprintf (MakeFptr, "#\n# Tool-generated list of dest output dirs that are created\n");
  fprintf (MakeFptr, "# from the list of source files in the [sources.*] sections\n");
  fprintf (MakeFptr, "# of the component INF file.\n#\n");
  //
  // Create output directory list 
  // for clean target to delete all build output files.
  //
  fprintf (MakeFptr, "DEST_OUTPUT_DIRS = $(%s) ", DEST_DIR);

  TempSymbol = gGlobals.OutdirList;
  while (TempSymbol != NULL) {
    fprintf (MakeFptr, "\\\n                   $(%s)\\%s   ", 
             DEST_DIR, TempSymbol->Name);
    TempSymbol = TempSymbol->Next;
  }
  fprintf (MakeFptr, "\n\n");
  
  //
  // clean up for the next module
  //
  FreeSymbols (gGlobals.OutdirList);
  gGlobals.OutdirList = NULL;

  return STATUS_SUCCESS;
}

static
INT8 *
BuiltFileExtension (
  INT8      *SourceFileName
  )
{
  int   i;
  INT8  *Cptr;
  //
  // Find the dot in the filename extension
  //
  for (Cptr = SourceFileName + strlen (SourceFileName) - 1;
       (Cptr > SourceFileName) && (*Cptr != '\\') && (*Cptr != '.');
       Cptr--
      ) {
    //
    // Do nothing
    //
  }

  if (*Cptr != '.') {
    return NULL;
  }
  //
  // Look through our list of known file types and return a pointer to
  // its built file extension.
  //
  for (i = 0; mFileTypes[i].Extension != NULL; i++) {
    if (_stricmp (Cptr, mFileTypes[i].Extension) == 0) {
      return mFileTypes[i].BuiltExtension;
    }
  }

  return NULL;
}

int
ProcessObjectsSingle (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  INT8      *SectionName
  )
{
  INT8    *Cptr;
  INT8    *Cptr2;
  INT8    Str[MAX_LINE_LEN];
  INT8    FileName[MAX_EXP_LINE_LEN];
  SECTION *TempSect;

  TempSect = DSCFileFindSection (ComponentFile, SectionName);
  if (TempSect != NULL) {
    while (DSCFileGetLine (ComponentFile, Str, sizeof (Str)) != NULL) {
      Cptr = StripLine (Str);
      //
      // Don't process blank lines
      //
      if (*Cptr) {
        //
        // Expand symbols then create the output filename. We'll do a lookup
        // on the source file's extension to determine what the extension of
        // the built version of the file is. For example, .c -> .obj.
        //
        if (!IsIncludeFile (Cptr)) {
          ExpandSymbols (Cptr, FileName, sizeof (FileName), 0);
          ReplaceSlash (FileName);
          Cptr2 = BuiltFileExtension (FileName);
          if (Cptr2 != NULL) {
            SetFileExtension (FileName, Cptr2);
            if (!IsAbsolutePath (FileName)) {
              fprintf (MakeFptr, "\\\n          $(%s)\\%s   ", DEST_DIR, FileName);
            } else {
              fprintf (MakeFptr, "\\\n          %s   ", FileName);
            }
          }
        }
      }
    }
  } else {
    return STATUS_WARNING;
  }

  return STATUS_SUCCESS;
}
//
// Process all [libraries.*] sections in the component INF file to create a
// macro to the component's output makefile: LIBS = Lib1 Lib2, ...
//
static
int
ProcessLibs (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr
  )
{
  INT8  Str[MAX_LINE_LEN];
  INT8  *Processor;
  INT8  *Platform;

  //
  // Print a useful comment to the component's makefile so the user knows
  // where the data came from.
  //
  fprintf (MakeFptr, "#\n# Tool-generated list of libraries that are generated\n");
  fprintf (MakeFptr, "# from the list of libraries listed in the [libraries.*] sections\n");
  fprintf (MakeFptr, "# of the component INF file.\n#\n");
  
  fprintf (MakeFptr, "LIBS = $(LIBS) ");
  
  Processor = GetSymbolValue (PROCESSOR);
  //
  // Process [libraries.common] files
  //
  sprintf (Str, "%s.%s", LIBRARIES_SECTION_NAME, COMMON_SECTION_NAME);
  ProcessLibsSingle (ComponentFile, MakeFptr, Str);
  //
  // Process the [libraries.$(PROCESSOR)] libraries to define "LIBS = x.lib y.lib..."
  //
  sprintf (Str, "%s.%s", LIBRARIES_SECTION_NAME, Processor);
  ProcessLibsSingle (ComponentFile, MakeFptr, Str);
  //
  // Now process any [libraries.$(PROCESSOR).$(PLATFORM)] files
  //
  Platform = GetSymbolValue (PLATFORM);
  if (Platform != NULL) {
    sprintf (Str, "%s.%s.%s", LIBRARIES_SECTION_NAME, Processor, Platform);
    ProcessLibsSingle (ComponentFile, MakeFptr, Str);
  }
  //
  // Process any [libraries.platform] files
  //
  ProcessLibsSingle (ComponentFile, MakeFptr, LIBRARIES_PLATFORM_SECTION_NAME);

  fprintf (MakeFptr, "\n\n");
  return STATUS_SUCCESS;
}

static
int
ProcessLibsSingle (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  INT8      *SectionName
  )
{
  INT8    *Cptr;
  INT8    Str[MAX_LINE_LEN];
  INT8    ExpandedLine[MAX_LINE_LEN];
  SECTION *TempSect;

  TempSect = DSCFileFindSection (ComponentFile, SectionName);
  if (TempSect != NULL) {
    while (DSCFileGetLine (ComponentFile, Str, sizeof (Str)) != NULL) {
      ExpandSymbols (Str, ExpandedLine, sizeof (ExpandedLine), 0);
      Cptr = StripLine (ExpandedLine);
      //
      // Don't process blank lines
      //
      if (*Cptr) {
        if (Cptr[strlen (Cptr) - 4] != '.') {
          fprintf (MakeFptr, "    \\\n       $(LIB_DIR)\\%s.lib", Cptr);
          //
          // Add lib dependency for single module build
          //
          fprintf (gGlobals.ModuleMakefileFptr, " %sbuild", Cptr);
        } else {
          fprintf (MakeFptr, "    \\\n       $(LIB_DIR)\\%s", Cptr);
          //
          // Add lib dependency for single module build
          //
          Cptr[strlen (Cptr) - 4] = 0;
          fprintf (gGlobals.ModuleMakefileFptr, " %sbuild", Cptr);
        }
        //
        // Add libs dependency for mCurrentBuildItem 
        //
        AddDependency (*mCurrentBuildList, mCurrentBuildItem, Cptr, 0);
      }
    }
  }

  return STATUS_SUCCESS;
}

static
int
ProcessIncludeFiles (
  DSC_FILE *ComponentFile,
  FILE     *MakeFptr
  )
{
  INT8  Str[MAX_LINE_LEN];
  INT8  *Processor;
  INT8  *Platform;
  INT8  *SourceSelect;
  INT8  *CStart;
  INT8  *CEnd;
  INT8  CSave;
  INT8  *CopySourceSelect;

  //
  // Print a useful comment to the output makefile so the user knows where
  // the info came from
  //
  //fprintf (MakeFptr, "#\n# Tool-generated include dependencies from any include files in the\n");
  //fprintf (MakeFptr, "# [sources.*] sections of the component INF file\n#\n");

  Processor = GetSymbolValue (PROCESSOR);
  
  //
  // See if they defined SOURCE_SELECT=xxx,yyy in which case we'll
  // select each [sources.xxx] and [sources.yyy] files and process
  // them.
  //
  SourceSelect = GetSymbolValue (SOURCE_SELECT);

  if (SourceSelect != NULL) {
    //
    // Make a copy of the string and break it up (comma-separated) and
    // select each [sources.*] file from the INF.
    //
    CopySourceSelect = (INT8 *) malloc (strlen (SourceSelect) + 1);
    if (CopySourceSelect == NULL) {
      Error (NULL, 0, 0, NULL, "failed to allocate memory");
      return STATUS_ERROR;
    }

    strcpy (CopySourceSelect, SourceSelect);
    CStart  = CopySourceSelect;
    CEnd    = CStart;
    while (*CStart) {
      CEnd = CStart + 1;
      while (*CEnd && *CEnd != ',') {
        CEnd++;
      }

      CSave = *CEnd;
      *CEnd = 0;
      sprintf (Str, "%s.%s", SOURCES_SECTION_NAME, CStart);
      ProcessIncludeFilesSingle (ComponentFile, MakeFptr, Str);
      //
      // Restore the terminator and advance
      //
      *CEnd   = CSave;
      CStart  = CEnd;
      if (*CStart) {
        CStart++;
      }
    }

    free (CopySourceSelect);

  } else {
    //
    // Find all the include files in the [sources.common] sections.
    //
    sprintf (Str, "%s.%s", SOURCES_SECTION_NAME, COMMON_SECTION_NAME);
    ProcessIncludeFilesSingle (ComponentFile, MakeFptr, Str);
    //
    // Now process the [sources.$(PROCESSOR)] files.
    //
    sprintf (Str, "%s.%s", SOURCES_SECTION_NAME, Processor);
    ProcessIncludeFilesSingle (ComponentFile, MakeFptr, Str);
    //
    // Now process the [sources.$(PROCESSOR).$(PLATFORM)] files.
    //
    Platform = GetSymbolValue (PLATFORM);
    if (Platform != NULL) {
      sprintf (Str, "sources.%s.%s", Processor, Platform);
      ProcessIncludeFilesSingle (ComponentFile, MakeFptr, Str);
    }
  }
  
  fprintf (MakeFptr, "\n");
  return STATUS_SUCCESS;
}

int
ProcessIncludeFilesSingle (
  DSC_FILE  *ComponentFile,
  FILE      *MakeFptr,
  INT8      *SectionName
  )
{
  INT8            *Cptr;
  INT8            FileName[MAX_EXP_LINE_LEN];
  INT8            TempFileName[MAX_PATH];
  SECTION         *TempSect;
  INT8            Str[MAX_LINE_LEN];
  INT8            *OverridePath;
  FILE            *FPtr;

  TempSect = DSCFileFindSection (ComponentFile, SectionName);
  if (TempSect != NULL) {
    //
    // See if the SOURCE_OVERRIDE_PATH has been set. If it has, and
    // they have an include file that is overridden, then add the path
    // to it to the list of include paths (prepend).
    //
    OverridePath = GetSymbolValue (SOURCE_OVERRIDE_PATH);
    while (DSCFileGetLine (ComponentFile, Str, sizeof (Str)) != NULL) {
      Cptr = StripLine (Str);
      //
      // Don't process blank lines
      //
      if (*Cptr) {
        //
        // Expand symbols in the filename, then get its parts
        //
        ExpandSymbols (Cptr, FileName, sizeof (FileName), 0);
        AddFileSymbols (FileName);
        ReplaceSlash (FileName);
        if (IsIncludeFile (FileName)) {
          if ((OverridePath != NULL) && (!IsAbsolutePath (FileName))) {
            ReplaceSlash (OverridePath);
            strcpy (TempFileName, OverridePath);
            strcat (TempFileName, "\\");
            strcat (TempFileName, FileName);
            if ((FPtr = fopen (TempFileName, "rb")) != NULL) {
              fclose (FPtr);
              //
              // Null-terminate the file name at the last backslash and add that
              // to the beginning of the list of include paths.
              //
              for (Cptr = TempFileName + strlen (TempFileName) - 1;
                   (Cptr >= TempFileName) && (*Cptr != '\\');
                   Cptr--
                  )
                ;
              if (Cptr >= TempFileName) {
                *Cptr = 0;
              }
              fprintf (MakeFptr, "!IF EXIST(%s)\n", TempFileName);
              fprintf (MakeFptr, "INC = -I %s $(INC)\n", TempFileName);
              fprintf (MakeFptr, "!ENDIF\n");
            }
          }
          //
          // If absolute path already, don't prepend source directory
          //
          // if (IsAbsolutePath (FileName)) {
          //   fprintf (MakeFptr, "INC_DEPS = $(INC_DEPS) %s\n", FileName);
          // } else {
          //   fprintf (MakeFptr, "INC_DEPS = $(INC_DEPS) $(SOURCE_DIR)\\%s\n", FileName);
          // }
        }

        RemoveFileSymbols ();
      }
    }
  }

  return STATUS_SUCCESS;
}

static
void
FreeFileParts (
  FILE_NAME_PARTS *FP
  )
{
  if (FP != NULL) {
    if (FP->Path != NULL) {
      free (FP->Path);
    }

    if (FP->BaseName != NULL) {
      free (FP->BaseName);
    }

    if (FP->Extension != NULL) {
      free (FP->Extension);
    }
  }
}

static
FILE_NAME_PARTS *
GetFileParts (
  INT8 *FileName
  )
{
  FILE_NAME_PARTS *FP;
  INT8            *Cptr;
  INT8            CopyFileName[MAX_PATH];
  INT8            *FileNamePtr;

  strcpy (CopyFileName, FileName);
  FP = (FILE_NAME_PARTS *) malloc (sizeof (FILE_NAME_PARTS));
  if (FP == NULL) {
    Error (NULL, 0, 0, NULL, "failed to allocate memory");
    return NULL;
  }

  memset ((INT8 *) FP, 0, sizeof (FILE_NAME_PARTS));
  //
  // Get extension code
  //
  FP->ExtensionCode = GetSourceFileType (CopyFileName);
  //
  // Get drive if there
  //
  FileNamePtr = CopyFileName;
  if (FileNamePtr[1] == ':') {
    FP->Drive[0]  = FileNamePtr[0];
    FP->Drive[1]  = ':';
    FileNamePtr += 2;
  }
  //
  // Start at the end and work back
  //
  for (Cptr = FileNamePtr + strlen (FileNamePtr) - 1; (Cptr > FileNamePtr) && (*Cptr != '.'); Cptr--)
    ;

  if (*Cptr == '.') {
    //
    // Don't copy the dot
    //
    FP->Extension = (char *) malloc (strlen (Cptr));
    strcpy (FP->Extension, Cptr + 1);
    *Cptr = 0;
    Cptr--;
    StripTrailingSpaces (FP->Extension);
  } else {
    //
    // Create empty string for extension
    //
    FP->Extension     = (char *) malloc (1);
    FP->Extension[0]  = 0;
  }
  //
  // Now back up and get the base name (include the preceding '\')
  //
  for (; (Cptr > FileNamePtr) && (*Cptr != '\\'); Cptr--)
    ;
  FP->BaseName = (char *) malloc (strlen (Cptr) + 1);
  strcpy (FP->BaseName, Cptr);
  *Cptr = 0;
  Cptr--;
  //
  // Rest is path
  //
  if (Cptr >= FileNamePtr) {
    Cptr      = FileNamePtr;
    FP->Path  = (char *) malloc (strlen (Cptr) + 1);
    strcpy (FP->Path, Cptr);
  } else {
    FP->Path    = (char *) malloc (1);
    FP->Path[0] = 0;
  }

  return FP;
}

/*****************************************************************************
******************************************************************************/
static
int
WriteCommonMakefile (
  DSC_FILE  *DSCFile,
  FILE      *MakeFptr,
  INT8      *Processor
  )
{
  INT8    InLine[MAX_LINE_LEN];
  INT8    OutLine[MAX_EXP_LINE_LEN];
  SECTION *Sect;
  INT8    *Sym;
  int     i;
  //
  // Don't mess up the original file pointer, since we're processing it at a higher
  // level.
  //
  DSCFileSavePosition (DSCFile);
  //
  // Write the header to the file
  //
  for (i = 0; MakefileHeader[i] != NULL; i++) {
    fprintf (MakeFptr, "%s\n", MakefileHeader[i]);
  }

  fprintf (MakeFptr, "#\n# Hard-coded defines output by the tool\n#\n");
  //
  // First write the basics to the component's makefile. These includes
  // EFI_SOURCE, BIN_DIR, OUT_DIR, LIB_DIR, SOURCE_DIR, DEST_DIR.
  //
  Sym = GetSymbolValue (EFI_SOURCE);
  fprintf (MakeFptr, "%s       = %s\n", EFI_SOURCE, Sym);
  Sym = GetSymbolValue (BUILD_DIR);
  fprintf (MakeFptr, "%s        = %s\n", BUILD_DIR, Sym);
  Sym = GetSymbolValue (BIN_DIR);
  fprintf (MakeFptr, "%s          = %s\n", BIN_DIR, Sym);
  Sym = GetSymbolValue (OUT_DIR);
  fprintf (MakeFptr, "%s          = %s\n", OUT_DIR, Sym);
  Sym = GetSymbolValue (LIB_DIR);
  fprintf (MakeFptr, "%s          = %s\n", LIB_DIR, Sym);
  Sym = GetSymbolValue (SOURCE_DIR);
  fprintf (MakeFptr, "%s       = %s\n", SOURCE_DIR, Sym);
  Sym = GetSymbolValue (DEST_DIR);
  fprintf (MakeFptr, "%s         = %s\n", DEST_DIR, Sym);
  fprintf (MakeFptr, "\n");
  //
  // If there was a [makefile.common] section in the description file,
  // copy it (after symbol expansion) to the output file.
  //
  sprintf (InLine, "%s.%s", MAKEFILE_SECTION_NAME, COMMON_SECTION_NAME);
  Sect = DSCFileFindSection (DSCFile, InLine);
  if (Sect != NULL) {
    //
    // fprintf (MakeFptr, "# From the [makefile.common] section of the DSC file\n");
    // Read lines, expand, then dump out
    //
    while (DSCFileGetLine (DSCFile, InLine, sizeof (InLine)) != NULL) {
      //
      // Replace symbols
      //
      ExpandSymbols (InLine, OutLine, sizeof (OutLine), EXPANDMODE_RECURSIVE);
      fprintf (MakeFptr, OutLine);
    }
  }
  //
  // If there was a [makefile.platform] section in the description file,
  // copy it (after symbol expansion) to the output file.
  //
  sprintf (InLine, "%s.%s", MAKEFILE_SECTION_NAME, "Platform");
  Sect = DSCFileFindSection (DSCFile, InLine);
  if (Sect != NULL) {
    //
    // Read lines, expand, then dump out
    //
    while (DSCFileGetLine (DSCFile, InLine, sizeof (InLine)) != NULL) {
      //
      // Replace symbols
      //
      ExpandSymbols (InLine, OutLine, sizeof (OutLine), EXPANDMODE_RECURSIVE);
      fprintf (MakeFptr, OutLine);
    }
  }
  //
  // Do the same for any [makefile.$(PROCESSOR)]
  //
  sprintf (InLine, "%s.%s", MAKEFILE_SECTION_NAME, Processor);
  Sect = DSCFileFindSection (DSCFile, InLine);
  if (Sect != NULL) {
    //
    // Read lines, expand, then dump out
    //
    while (DSCFileGetLine (DSCFile, InLine, sizeof (InLine)) != NULL) {
      ExpandSymbols (InLine, OutLine, sizeof (OutLine), EXPANDMODE_RECURSIVE);
      fprintf (MakeFptr, OutLine);
    }
  }
  //
  // Same thing for [makefile.$(PROCESSOR).$(PLATFORM)]
  //
  Sym = GetSymbolValue (PLATFORM);
  if (Sym != NULL) {
    sprintf (InLine, "%s.%s.%s", MAKEFILE_SECTION_NAME, Processor, Sym);
    Sect = DSCFileFindSection (DSCFile, InLine);
    if (Sect != NULL) {
      //
      // Read lines, expand, then dump out
      //
      while (DSCFileGetLine (DSCFile, InLine, sizeof (InLine)) != NULL) {
        ExpandSymbols (InLine, OutLine, sizeof (OutLine), EXPANDMODE_RECURSIVE);
        fprintf (MakeFptr, OutLine);
      }
    }
  }
  
  fprintf (MakeFptr, "\n");
  DSCFileRestorePosition (DSCFile);
  return 0;
}

static
int
WriteComponentTypeBuildCommands (
  DSC_FILE *DSCFile,
  FILE     *MakeFptr,
  INT8     *SectionName
  )
/*++

Routine Description:
  
   Given a section name such as [build.ia32.library], find the section in
   the description file and copy the build commands.

Arguments:

  DSCFile     - section information on the main description file
  MakeFptr    - file pointer to the makefile we're writing to
  SectionName - name of the section we're to copy out to the makefile.

Returns:

  Always successful, since the section may be optional.

--*/
{
  SECTION *Sect;
  INT8    InLine[MAX_LINE_LEN];
  INT8    OutLine[MAX_EXP_LINE_LEN];
  
  //
  // Don't mess up the original file pointer, since we're processing it at a higher
  // level.
  //
  DSCFileSavePosition (DSCFile);
  Sect = DSCFileFindSection (DSCFile, SectionName);
  if (Sect != NULL) {
    //
    // Read lines, expand, then dump out
    //
    while (DSCFileGetLine (DSCFile, InLine, sizeof (InLine)) != NULL) {
      ExpandSymbols (
        InLine, 
        OutLine, 
        sizeof(OutLine), 
        EXPANDMODE_NO_DESTDIR | EXPANDMODE_NO_SOURCEDIR
        );
      fprintf (MakeFptr, OutLine);
    }
  } else {
    Warning (
      NULL,
      0,
      0,
      GetSymbolValue (INF_FILENAME),
      "no [%s] build commands found in DSC file for component",
      SectionName
      );
  }

  DSCFileRestorePosition (DSCFile);
  return STATUS_SUCCESS;
}

/*****************************************************************************

******************************************************************************/
static
int
WriteCompileCommands (
  DSC_FILE  *DscFile,
  FILE      *MakeFptr,
  INT8      *FileName,
  INT8      *Processor
  )
{
  FILE_NAME_PARTS *File;
  SECTION         *Sect;
  INT8            BuildSectionName[40];
  INT8            InLine[MAX_LINE_LEN];
  INT8            OutLine[MAX_EXP_LINE_LEN];
  INT8            *SourceCompileType;
  char            *CPtr;
  char            *CPtr2;
  //
  // Determine the filename, then chop it up into its parts
  //
  File = GetFileParts (FileName);
  if (File != NULL) {
    //
    // Don't mess up the original file pointer, since we're processing it at a higher
    // level.
    //
    DSCFileSavePosition (DscFile);
    //
    // Option 1: SOURCE_COMPILE_TYPE=MyCompileSection
    //           Find a section of that name from which to get the compile
    //           commands for this source file. 
    //           Look for [compile.$(PROCESSOR).$(SOURCE_COMPILE_TYPE]
    // Option 2: COMPILE_SELECT=.c=MyCCompile,.asm=MyAsm
    //           Find a [compile.$(PROCESSOR).MyCompile] section from which to
    //           get the compile commands for this source file. 
    //           Look for [compile.$(PROCESSOR).MyCompile]
    // Option 3: Look for standard section types to compile the file by extension.
    //           Look for [compile.$(PROCESSOR).<extension>]
    //
    Sect = NULL;
    //
    // Option 1 - use SOURCE_COMPILE_TYPE variable
    //
    SourceCompileType = GetSymbolValue (SOURCE_COMPILE_TYPE);
    if (SourceCompileType != NULL) {
      sprintf (BuildSectionName, "compile.%s.%s", Processor, SourceCompileType);
      Sect = DSCFileFindSection (DscFile, BuildSectionName);
    }
    //
    // Option 2 - use COMPILE_SELECT variable
    //
    if (Sect == NULL) {
      SourceCompileType = GetSymbolValue (COMPILE_SELECT);
      if (SourceCompileType != NULL) {
        //
        // Parse the variable, which looks like COMPILE_SELECT=.c=MyCCompiler;.asm=MyAsm;
        // to find an entry with a matching file name extension. If you find one,
        // then use that name to find the section name.
        //
        CPtr = SourceCompileType;
        while (*CPtr && (Sect == NULL)) {
          //
          // See if we found a match with this source file name extension. File->Extension
          // does not include the dot, so skip the dot in the COMPILE_SELECT variable if there
          // is one.
          //
          if (*CPtr == '.') {
            CPtr++;
          }

          if (_strnicmp (CPtr, File->Extension, strlen (File->Extension)) == 0) {
            //
            // Found a file name extension match -- extract the name from the variable, for
            // example "MyCCompiler"
            //
            while (*CPtr && (*CPtr != '=')) {
              CPtr++;
            }

            if ((*CPtr != '=') || (CPtr[1] == 0)) {
              Error (NULL, 0, 0, SourceCompileType, "malformed COMPILE_SELECT variable");
              break;
            }

            CPtr++;
            sprintf (BuildSectionName, "compile.%s.", Processor);
            for (CPtr2 = BuildSectionName + strlen (BuildSectionName);
                 *CPtr && (*CPtr != ',') && (*CPtr != ';');
                 CPtr++
                ) {
              *CPtr2 = *CPtr;
              CPtr2++;
            }

            *CPtr2  = 0;
            Sect    = DSCFileFindSection (DscFile, BuildSectionName);
            if (Sect == NULL) {
              ParserError (
                0,
                BuildSectionName,
                "could not find section in DSC file - selected by COMPILE_SELECT variable"
                );
            }
          }

          //
          // Skip to next file name extension in the COMPILE_SELECT variable
          //
          while (*CPtr && (*CPtr != ';') && (*CPtr != ',')) {
            CPtr++;
          }

          if (*CPtr) {
            CPtr++;
          }
        }
      }
    }
    //
    // Option 3 - use "Compile.$(PROCESSOR).<Extension>" section
    //
    if (Sect == NULL) {
      sprintf (BuildSectionName, "compile.%s.%s", Processor, File->Extension);
      Sect = DSCFileFindSection (DscFile, BuildSectionName);
    }
    //
    // Should have found something by now unless it's an include (.h) file
    //
    if (Sect != NULL) {
      //
      // Temporarily add a FILE variable to the global symbol table. Omit the
      // extension.
      //
      sprintf (InLine, "%s%s%s", File->Drive, File->Path, File->BaseName);
      AddSymbol ("FILE", InLine, SYM_OVERWRITE | SYM_LOCAL | SYM_FILENAME);
      //
      // Read lines, expand (except SOURCE_DIR and DEST_DIR), then dump out
      //
      while (DSCFileGetLine (DscFile, InLine, sizeof (InLine)) != NULL) {
        ExpandSymbols (
          InLine,
          OutLine,
          sizeof (OutLine),
          EXPANDMODE_NO_DESTDIR | EXPANDMODE_NO_SOURCEDIR
          );
        fprintf (MakeFptr, OutLine);
      }
      fprintf (MakeFptr, "\n");
    } else {
      //
      // Be nice and ignore include files
      //
      if (!IsIncludeFile (FileName)) {
        Error (
          NULL,
          0,
          0,
          NULL,
          "no compile commands section [%s] found in DSC file for %s",
          BuildSectionName,
          FileName
          );
      }
    }

    DSCFileRestorePosition (DscFile);
    FreeFileParts (File);
  }

  return STATUS_SUCCESS;
}

/*****************************************************************************
******************************************************************************/
static
int
SetFileExtension (
  INT8 *FileName,
  INT8 *Extension
  )
{
  INT8  *Cptr;

  Cptr = FileName + strlen (FileName) - 1;
  while ((Cptr > FileName) && (*Cptr != '.')) {
    Cptr--;

  }
  //
  // Better be a dot
  //
  if (*Cptr != '.') {
    Message (2, "Missing filename extension: %s", FileName);
    return STATUS_WARNING;
  }

  Cptr++;
  if (*Extension == '.') {
    Extension++;
  }

  strcpy (Cptr, Extension);
  return STATUS_SUCCESS;
}

/*****************************************************************************
******************************************************************************/
int
MakeFilePath (
  INT8 *FileName
  )
{
  INT8  *Cptr;
  INT8  SavedChar;
  INT8  BuildDir[MAX_PATH];
  INT8  CopyFileName[MAX_PATH];

  //
  // Expand symbols in the filename
  //
  if (ExpandSymbols (FileName, CopyFileName, sizeof (CopyFileName), EXPANDMODE_NO_UNDEFS)) {
    Error (NULL, 0, 0, NULL, "undefined symbols in file path: %s", FileName);
    return STATUS_ERROR;
  }
  //
  // Copy it back
  //
  strcpy (FileName, CopyFileName);
  //
  // To avoid creating $(BUILD_DIR) path, see if this path is the same as
  // $(BUILD_DIR), and if it is, see if build dir exists and skip over that
  // portion if it does
  //
  Cptr = GetSymbolValue (BUILD_DIR);
  if (Cptr != NULL) {
    if (_strnicmp (Cptr, FileName, strlen (Cptr)) == 0) {
      //
      // BUILD_DIR path. See if it exists
      //
      strcpy (BuildDir, FileName);
      BuildDir[strlen (Cptr)] = 0;
      if ((_mkdir (BuildDir) != 0) && (errno != EEXIST)) {
        Cptr = FileName;
      } else {
        //
        // Already done. Shortcut. Skip to next path so that we don't create
        // the BUILD_DIR as well.
        //
        Cptr = FileName + strlen (Cptr);
        if (*Cptr == '\\') {
          Cptr++;
        }
      }
    } else {
      //
      // Not build dir
      //
      Cptr = FileName;
    }
  } else {
    Cptr = FileName;
  }
  //
  // Create directories until done. Skip over "c:\" in the path if it exists
  //
  if (*Cptr && (*(Cptr + 1) == ':') && (*(Cptr + 2) == '\\')) {
    Cptr += 3;
  }

  for (;;) {
    for (; *Cptr && (*Cptr != '\\'); Cptr++)
      ;
    if (*Cptr) {
      SavedChar = *Cptr;
      *Cptr     = 0;
      if ((_mkdir (FileName) != 0)) {
        //
        //        Error (NULL, 0, 0, FileName, "failed to create directory");
        //        return 1;
        //
      }

      *Cptr = SavedChar;
      Cptr++;
    } else {
      break;
    }
  }

  return STATUS_SUCCESS;
}

/*****************************************************************************
******************************************************************************/
int
ExpandSymbols (
  INT8  *SourceLine,
  INT8  *DestLine,
  int   LineLen,
  int   ExpandMode
  )
{
  static int  NestDepth = 0;
  INT8        *FromPtr;
  INT8        *ToPtr;
  INT8        *SaveStart;
  INT8        *Cptr;
  INT8        *value;
  int         Expanded;
  int         ExpandedCount;
  INT8        *LocalDestLine;
  STATUS      Status;
  int         LocalLineLen;

  NestDepth++;
  Status        = STATUS_SUCCESS;
  LocalDestLine = (INT8 *) malloc (LineLen);
  if (LocalDestLine == NULL) {
    Error (__FILE__, __LINE__, 0, "application error", "memory allocation failed");
    NestDepth = 0;
    return STATUS_ERROR;
  }

  FromPtr = SourceLine;
  ToPtr   = LocalDestLine;
  //
  // Walk the entire line, replacing $(SYMBOL_NAME).
  //
  LocalLineLen  = LineLen;
  ExpandedCount = 0;
  while (*FromPtr && (LocalLineLen > 0)) {
    if ((*FromPtr == '$') && (*(FromPtr + 1) == '(')) {
      //
      // Save the start in case it's undefined, in which case we copy it as-is.
      //
      SaveStart = FromPtr;
      Expanded  = 0;
      //
      // Symbol expansion time. Find the end (no spaces allowed)
      //
      FromPtr += 2;
      for (Cptr = FromPtr; *Cptr && (*Cptr != ')'); Cptr++)
        ;
      if (*Cptr) {
        //
        // Truncate the string at the closing parenthesis for ease-of-use.
        // Then copy the string directly to the destination line in case we don't find
        // a definition for it.
        //
        *Cptr = 0;
        strcpy (ToPtr, SaveStart);
        if ((_stricmp (SOURCE_DIR, FromPtr) == 0) && (ExpandMode & EXPANDMODE_NO_SOURCEDIR)) {
          //
          // excluded this expansion
          //
        } else if ((_stricmp (DEST_DIR, FromPtr) == 0) && (ExpandMode & EXPANDMODE_NO_DESTDIR)) {
          //
          // excluded this expansion
          //
        } else if ((value = GetSymbolValue (FromPtr)) != NULL) {
          strcpy (ToPtr, value);
          LocalLineLen -= strlen (value);
          ToPtr += strlen (value);
          Expanded = 1;
          ExpandedCount++;
        } else if (ExpandMode & EXPANDMODE_NO_UNDEFS) {
          Error (NULL, 0, 0, "undefined symbol", "$(%s)", FromPtr);
          Status = STATUS_ERROR;
          goto Done;
        }
        
        //
        // Restore closing parenthesis, and advance to next character
        //
        *Cptr   = ')';
        if (!Expanded) {
          FromPtr = SaveStart + 1;
          ToPtr++;
        } else {
          FromPtr = Cptr + 1;
        }
      } else {
        Error (NULL, 0, 0, SourceLine, "missing closing parenthesis on symbol");
        strcpy (ToPtr, FromPtr);
        Status = STATUS_WARNING;
        goto Done;
      }
    } else {
      *ToPtr = *FromPtr;
      FromPtr++;
      ToPtr++;
      LocalLineLen--;
    }
  }

  if (*FromPtr == 0) {
    *ToPtr = 0;
  }

  //
  // If we're in recursive mode, and we expanded at least one string successfully,
  // then make a recursive call to try again.
  //
  if ((ExpandedCount != 0) && (Status == STATUS_SUCCESS) && (ExpandMode & EXPANDMODE_RECURSIVE) && (NestDepth < 2)) {
    Status = ExpandSymbols (LocalDestLine, DestLine, LineLen, ExpandMode);
    free (LocalDestLine);
    NestDepth = 0;
    return Status;
  }

Done:
  if (Status != STATUS_ERROR) {
    strcpy (DestLine, LocalDestLine);
  }

  NestDepth = 0;
  free (LocalDestLine);
  return Status;
}

INT8 *
GetSymbolValue (
  INT8 *SymbolName
  )
/*++

Routine Description:
  
  Look up a symbol in our symbol table.

Arguments:

  SymbolName - The name of symbol.

Returns:

  Pointer to the value of the symbol if found
  NULL if the symbol is not found

--*/
{
  SYMBOL  *Symbol;

  //
  // Scan once for file-level symbols
  //
  Symbol = gGlobals.Symbol;
  while (Symbol) {
    if ((_stricmp (SymbolName, Symbol->Name) == 0) && (Symbol->Type & SYM_FILE)) {
      return Symbol->Value;
    }

    Symbol = Symbol->Next;
  }
  //
  // Scan once for local symbols
  //
  Symbol = gGlobals.Symbol;
  while (Symbol) {
    if ((_stricmp (SymbolName, Symbol->Name) == 0) && (Symbol->Type & SYM_LOCAL)) {
      return Symbol->Value;
    }

    Symbol = Symbol->Next;
  }
  //
  // No local value found. Scan for globals.
  //
  Symbol = gGlobals.Symbol;
  while (Symbol) {
    if ((_stricmp (SymbolName, Symbol->Name) == 0) && (Symbol->Type & SYM_GLOBAL)) {
      return Symbol->Value;
    }

    Symbol = Symbol->Next;
  }
  //
  // For backwards-compatibility, if it's "GUID", return FILE_GUID value
  //
  if (_stricmp (SymbolName, GUID) == 0) {
    return GetSymbolValue (FILE_GUID);
  }

  return NULL;
}

static
int
RemoveLocalSymbols (
  VOID
  )
/*++

Routine Description:
  
  Remove all local symbols from the symbol table. Local symbols are those
  that are defined typically by the component's INF file.

Arguments:

  None.

Returns:

  Right now, never fails.

--*/
{
  SYMBOL  *Sym;
  int     FoundOne;

  do {
    FoundOne  = 0;
    Sym       = gGlobals.Symbol;
    while (Sym) {
      if (Sym->Type & SYM_LOCAL) {
        //
        // Going to delete it out from under ourselves, so break and restart
        //
        FoundOne = 1;
        RemoveSymbol (Sym->Name, SYM_LOCAL);
        break;
      }

      Sym = Sym->Next;
    }
  } while (FoundOne);
  return STATUS_SUCCESS;
}

static
int
RemoveFileSymbols (
  VOID
  )
/*++

Routine Description:
  
  Remove all file-level symbols from the symbol table. File-level symbols are 
  those that are defined on a source file line in an INF file.

Arguments:

  None.

Returns:

  Right now, never fails.

--*/
{
  SYMBOL  *Sym;
  int     FoundOne;

  do {
    FoundOne  = 0;
    Sym       = gGlobals.Symbol;
    while (Sym) {
      if (Sym->Type & SYM_FILE) {
        //
        // Going to delete it out from under ourselves, so break and restart
        //
        FoundOne = 1;
        RemoveSymbol (Sym->Name, SYM_FILE);
        break;
      }

      Sym = Sym->Next;
    }
  } while (FoundOne);
  return STATUS_SUCCESS;
}

static
STATUS
ParseGuidDatabaseFile (
  INT8 *FileName
  )
/*++

Routine Description:
  This function parses a GUID-to-basename text file (perhaps output by
  the GuidChk utility) to define additional symbols. The format of the 
  file should be:

  7BB28B99-61BB-11D5-9A5D-0090273FC14D EFI_DEFAULT_BMP_LOGO_GUID gEfiDefaultBmpLogoGuid
  
  This function parses the line and defines global symbol:

    EFI_DEFAULT_BMP_LOGO_GUID=7BB28B99-61BB-11D5-9A5D-0090273FC14D 
  
  This symbol (rather than the actual GUID) can then be used in INF files to 
  fix duplicate GUIDs

Arguments:
  FileName  - the name of the file to parse.

Returns:
  STATUS_ERROR    - could not open FileName
  STATUS_SUCCESS  - we opened the file

--*/
{
  FILE  *Fptr;
  INT8  Line[100];
  INT8  Guid[100];
  INT8  DefineName[80];

  Fptr = fopen (FileName, "r");
  if (Fptr == NULL) {
    Error (NULL, 0, 0, FileName, "failed to open input GUID database input file");
    return STATUS_ERROR;
  }

  while (fgets (Line, sizeof (Line), Fptr) != NULL) {
    //
    // Get the GUID string, skip the defined name (EFI_XXX_GUID), and get the
    // variable name (gWhateverProtocolGuid)
    //
    if (sscanf (Line, "%s %s %*s", Guid, DefineName) == 2) {
      AddSymbol (DefineName, Guid, SYM_GLOBAL);
    }
  }

  fclose (Fptr);
  return STATUS_SUCCESS;
}

/*****************************************************************************

  Returns:
     0 if successful standard add
    length of the parsed string if passed in " name = value  "
    < 0 on error

******************************************************************************/
int
AddSymbol (
  INT8    *Name,
  INT8    *Value,
  int     Mode
  )
{
  SYMBOL  *Symbol;
  SYMBOL  *NewSymbol;
  int     Len;
  INT8    *Start;
  INT8    *Cptr;
  INT8    CSave1;
  INT8    *SaveCptr1;
  INT8    CSave2;
  INT8    *SaveCptr2;
  INT8    ShortName[MAX_PATH];

  Len           = 0;
  SaveCptr1     = NULL;
  CSave1        = 0;
  SaveCptr2     = NULL;
  CSave2        = 0;

  ShortName[0]  = 0;
  //
  // Mode better be local or global symbol
  //
  if ((Mode & (SYM_LOCAL | SYM_GLOBAL | SYM_FILE)) == 0) {
    Error (NULL, 0, 0, "APP ERROR", "adding symbol '%s' that is not local, global, nor file level", Name);
    return -1;
  }
  //
  // If value pointer is null, then they passed us a line something like:
  //    varname = value, or simply var =
  //
  if (Value == NULL) {
    Start = Name;
    while (*Name && isspace (*Name)) {
      Name++;

    }

    if (!*Name) {
      return -1;
    }
    //
    // Find the end of the name. Either space or a '='.
    //
    for (Value = Name; *Value && !isspace (*Value) && (*Value != '='); Value++)
      ;
    if (!*Value) {
      return -1;
    }
    //
    // Look for the '='
    //
    Cptr = Value;
    while (*Value && (*Value != '=')) {
      Value++;
    }

    if (!*Value) {
      return -1;
    }

    //
    // Now truncate the name
    //
    CSave1    = *Cptr;
    SaveCptr1 = Cptr;
    *Cptr     = 0;    

    //
    // Skip over the = and then any spaces
    //
    Value++;
    while (*Value && isspace (*Value)) {
      Value++;

    }
    //
    // Find end of string, checking for quoted string
    //
    if (*Value == '\"') {
      Value++;
      for (Cptr = Value; *Cptr && *Cptr != '\"'; Cptr++)
        ;
    } else {
      for (Cptr = Value; *Cptr && !isspace (*Cptr); Cptr++)
        ;
    }
    //
    // Null terminate the value string
    //
    if (*Cptr) {
      Len = (int) (Cptr - Start) + 1;
      CSave2    = *Cptr;
      SaveCptr2 = Cptr;
      *Cptr     = 0;
    } else {
      Len = (int) (Cptr - Start);
    }
  }

  //
  // If file name or file path, and we're shortening, then print it
  //
  if ((Mode & (SYM_FILEPATH | SYM_FILENAME)) && (GetSymbolValue (SHORT_NAMES) != NULL)) {
    if (GetShortPathName (Value, ShortName, sizeof (ShortName)) > 0) {
      //
      // fprintf (stdout, "String value '%s' shortened to '%s'\n",
      //    Value, ShortName);
      //
      Value = ShortName;
    } else {
      //
      // fprintf (stdout, "WARNING: Failed to get short name for %s\n", Value);
      //
    }
  }
  //
  // We now have a symbol name and a value. Look for an existing variable of
  // the same type (global or local) and overwrite it.
  //
  Symbol = gGlobals.Symbol;
  while (Symbol) {
    //
    // Check for symbol name match
    //
    if (_stricmp (Name, Symbol->Name) == 0) {
      //
      // See if this symbol is of the same type (global or local) as what
      // they're requesting
      //
      if ((Symbol->Type & (SYM_LOCAL | SYM_GLOBAL)) == (Mode & (SYM_LOCAL | SYM_GLOBAL))) {
        //
        // Did they say we could overwrite it?
        //
        if (Mode & SYM_OVERWRITE) {
          free (Symbol->Value);
          Symbol->Value = (INT8 *) malloc (strlen (Value) + 1);
          if (Symbol->Value == NULL) {
            Error (NULL, 0, 0, NULL, "failed to allocate memory");
            return -1;
          }

          strcpy (Symbol->Value, Value);
          //
          // If value == "NULL", then make it a 0-length string
          //
          if (_stricmp (Symbol->Value, "NULL") == 0) {
            Symbol->Value[0] = 0;
          }

          return Len;
        } else {
          return STATUS_ERROR;
        }
      }
    }

    Symbol = Symbol->Next;
  }
  //
  // Does not exist, create a new one
  //
  NewSymbol = (SYMBOL *) malloc (sizeof (SYMBOL));
  if (NewSymbol == NULL) {
    Error (NULL, 0, 0, NULL, "failed to allocate memory");
    return -1;
  }

  memset ((INT8 *) NewSymbol, 0, sizeof (SYMBOL));
  NewSymbol->Name   = (INT8 *) malloc (strlen (Name) + 1);
  NewSymbol->Value  = (INT8 *) malloc (strlen (Value) + 1);
  //
  // Simply use the mode bits as the type.
  //
  NewSymbol->Type = Mode;
  if ((NewSymbol->Name == NULL) || (NewSymbol->Value == NULL)) {
    Error (NULL, 0, 0, NULL, "failed to allocate memory");
    return -1;
  }

  strcpy (NewSymbol->Name, Name);
  strcpy (NewSymbol->Value, Value);
  //
  // Remove trailing spaces
  //
  Cptr = NewSymbol->Value + strlen (NewSymbol->Value) - 1;
  while (Cptr > NewSymbol->Value) {
    if (isspace (*Cptr)) {
      *Cptr = 0;
      Cptr--;
    } else {
      break;
    }
  }
  //
  // Add it to the head of the list.
  //
  NewSymbol->Next = gGlobals.Symbol;
  gGlobals.Symbol = NewSymbol;
  //
  // If value == "NULL", then make it a 0-length string
  //
  if (_stricmp (NewSymbol->Value, "NULL") == 0) {
    NewSymbol->Value[0] = 0;
  }
  //
  // Restore the terminator we inserted if they passed in var=value
  //
  if (SaveCptr1 != NULL) {
    *SaveCptr1 = CSave1;
  }
  if (SaveCptr2 != NULL) {
    *SaveCptr2 = CSave2;
  }

  return Len;
}

/*****************************************************************************
******************************************************************************/
static
int
RemoveSymbol (
  INT8 *Name,
  INT8 SymbolType
  )
{
  SYMBOL  *Symbol;
  SYMBOL  *PrevSymbol;

  PrevSymbol  = NULL;
  Symbol      = gGlobals.Symbol;
  while (Symbol) {
    if ((_stricmp (Name, Symbol->Name) == 0) && (Symbol->Type & SymbolType)) {
      if (Symbol->Value) {
        free (Symbol->Value);
      }

      free (Symbol->Name);
      if (PrevSymbol) {
        PrevSymbol->Next = Symbol->Next;
      } else {
        gGlobals.Symbol = Symbol->Next;
      }

      free (Symbol);
      return STATUS_SUCCESS;
    }

    PrevSymbol  = Symbol;
    Symbol      = Symbol->Next;
  }

  return STATUS_WARNING;
}

#if 0

/*****************************************************************************
******************************************************************************/
static
void
FreeSections (
  SECTION *Sect
  )
{
  SECTION *Next;

  while (Sect != NULL) {
    Next = Sect->Next;
    if (Sect->Name != NULL) {
      delete[] Sect->Name;
    }

    delete Sect;
    Sect = Next;
  }
}
#endif

/*****************************************************************************
******************************************************************************/
static
INT8 *
StripLine (
  INT8 *Line
  )
{
  INT8  *Cptr;
  int   Len;

  Cptr = Line;
  //
  // Look for '#' comments in first character of line
  //
  if (*Cptr == '#') {
    *Cptr = 0;
    return Cptr;
  }

  while (isspace (*Cptr)) {
    Cptr++;
  }
  //
  // Hack off newlines
  //
  Len = strlen (Cptr);
  if ((Len > 0) && (Cptr[Len - 1] == '\n')) {
    Cptr[Len - 1] = 0;
  }
  //
  // Hack off trailing spaces
  //
  StripTrailingSpaces (Cptr);
  return Cptr;
}

/*****************************************************************************
  FUNCTION:  ProcessOptions()
  
  DESCRIPTION: Process the command-line options.  
******************************************************************************/
static
int
ProcessOptions (
  int   Argc,
  INT8  *Argv[]
  )
/*++

Routine Description:
  
  Process the command line options to this utility.

Arguments:

  Argc   - Standard Argc.
  Argv[] - Standard Argv.

Returns:

--*/
{
  INT8  *Cptr;
  int   FreeCwd;

  //
  // Clear out the options
  //
  memset ((INT8 *) &gGlobals, 0, sizeof (gGlobals));

  Argc--;
  Argv++;

  if (Argc == 0) {
    Usage ();
    return STATUS_ERROR;
  }
  //
  // Now process the arguments
  //
  while (Argc > 0) {

    if ((Argv[0][0] == '-') || (Argv[0][0] == '/')) {
      switch (Argv[0][1]) {
      //
      // -? or -h help option
      //
      case '?':
      case 'h':
      case 'H':
        Usage ();
        return STATUS_ERROR;

      //
      // /d symbol=name
      //
      case 'd':
      case 'D':
        //
        // Skip to next arg
        //
        Argc--;
        Argv++;
        if (Argc == 0) {
          Argv--;
          Error (NULL, 0, 0, NULL, "missing symbol definition with %c%c", Argv[0][0], Argv[0][1]);
          return STATUS_ERROR;
        } else {
          if (AddSymbol (Argv[0], NULL, SYM_OVERWRITE | SYM_GLOBAL) <= 0) {
            Warning (NULL, 0, 0, Argv[0], "failed to add symbol: %s");
          }
        }
        break;

      //
      // output makefile name
      //
      case 'm':
      case 'M':
        //
        // Skip to next arg
        //
        Argc--;
        Argv++;
        if (Argc == 0) {
          Argv--;
          Error (NULL, 0, 0, Argv[0], "missing output makefile name with option");
          Usage ();
          return STATUS_ERROR;
        } else {
          strcpy (gGlobals.MakefileName, Argv[0]);
        }
        break;

      //
      // Print a cross-reference file containing guid/basename/processor
      //
      case 'x':
      case 'X':
        //
        // Skip to next arg
        //
        Argc--;
        Argv++;
        if (Argc == 0) {
          Argv--;
          Error (NULL, 0, 0, Argv[0], "missing cross-reference output filename with option");
          Usage ();
          return STATUS_ERROR;
        } else {
          strcpy (gGlobals.XRefFileName, Argv[0]);
        }
        break;

      //
      // GUID database file to preparse
      //
      case 'g':
      case 'G':
        //
        // Skip to next arg
        //
        Argc--;
        Argv++;
        if (Argc == 0) {
          Argv--;
          Error (NULL, 0, 0, Argv[0], "missing input GUID database filename with option");
          Usage ();
          return STATUS_ERROR;
        } else {
          strcpy (gGlobals.GuidDatabaseFileName, Argv[0]);
        }
        break;

      //
      // Enable multi-thread build and specify the thread number
      //
      case 'n':
      case 'N':
        //
        // Skip to next arg
        //
        Argc--;
        Argv++;
        if (Argc == 0) {
          Argv--;
          Error (NULL, 0, 0, Argv[0], "missing input thread number with option");
          Usage ();
          return STATUS_ERROR;
        } else {
          gGlobals.ThreadNumber = atoi (Argv[0]);
          if (gGlobals.ThreadNumber == 0) {
            Argv--;
            Error (NULL, 0, 0, Argv[0], "input thread number should not be %s", Argv[1]);
            return STATUS_ERROR;
          } else if (gGlobals.ThreadNumber > MAXIMUM_WAIT_OBJECTS) {
            Argv--;
            Error (NULL, 0, 0, Argv[0], "input thread number should not greater than %d", MAXIMUM_WAIT_OBJECTS);
            return STATUS_ERROR;
          }
        }
        break;

      //
      // Specify the multi-thread build target
      //
      case 't':
      case 'T':
        //
        // Skip to next arg
        //
        Argc--;
        Argv++;
        if (Argc == 0) {
          Argv--;
          Error (NULL, 0, 0, Argv[0], "missing input build target with option");
          Usage ();
          return STATUS_ERROR;
        } else if (_stricmp (Argv[0], "all") == 0) {
          gGlobals.BuildTarget |= BUILD_TARGET_ALL;
        } else if (_stricmp (Argv[0], "libraries") == 0) {
          gGlobals.BuildTarget |= BUILD_TARGET_LIBRARIES;
        } else if (_stricmp (Argv[0], "components") == 0) {
          gGlobals.BuildTarget |= BUILD_TARGET_COMPONENTS;
        } else {
          Argv--;
          Error (NULL, 0, 0, Argv[0], "input build target not supported");
          Usage ();
        }
        break;

      case 'v':
      case 'V':
        gGlobals.Verbose = 1;
        break;

      default:
        Error (NULL, 0, 0, Argv[0], "unrecognized option");
        return STATUS_ERROR;
      }
    } else {
      break;
    }

    Argc--;
    Argv++;
  }
  //
  // Must be at least one arg left
  //
  if (Argc > 0) {
    gGlobals.DscFilename = Argv[0];
  }

  if (gGlobals.DscFilename == NULL) {
    Error (NULL, 0, 0, NULL, "must specify DSC filename on command line");
    return STATUS_ERROR;
  }
  //
  // Make a global symbol for the DSC filename
  //
  AddSymbol (DSC_FILENAME, gGlobals.DscFilename, SYM_GLOBAL | SYM_FILENAME);
  //
  // If no output makefile specified, take the default
  //
  if (gGlobals.MakefileName[0] == 0) {
    strcpy (gGlobals.MakefileName, MAKEFILE_OUT_NAME);
  }
  //
  // Get the current working directory and use it for the build directory.
  // Only do this if they have not defined it on the command line. Do the
  // same for the bin dir, output dir, and library directory.
  //
  Cptr = GetSymbolValue (BUILD_DIR);
  if (Cptr == NULL) {
    Cptr    = _getcwd (NULL, 0);
    FreeCwd = 1;
    AddSymbol (BUILD_DIR, Cptr, SYM_OVERWRITE | SYM_GLOBAL | SYM_FILEPATH);
  } else {
    ReplaceSlash (Cptr);
    FreeCwd = 0;
  }

  if (FreeCwd) {
    free (Cptr);
  }
  
  //
  // Default build target is all
  //
  if (gGlobals.BuildTarget == 0) {
    gGlobals.BuildTarget = BUILD_TARGET_ALL;
  }
  
  return 0;
}

/*****************************************************************************
******************************************************************************/
static
SYMBOL *
FreeSymbols (
  SYMBOL *Syms
  )
{
  SYMBOL  *Next;
  while (Syms) {

    if (Syms->Name != NULL) {
      free (Syms->Name);
    }

    if (Syms->Value != NULL) {
      free (Syms->Value);
    }

    Next = Syms->Next;
    free (Syms);
    Syms = Next;
  }

  return Syms;
}

/*****************************************************************************
******************************************************************************/
static
int
GetSourceFileType (
  INT8 *FileName
  )
{
  INT8  *Cptr;
  int   len;
  int   i;

  len = strlen (FileName);
  if (len == 0) {
    return FILETYPE_UNKNOWN;

  }

  Cptr = FileName + len - 1;
  while ((*Cptr != '.') && (Cptr >= FileName)) {
    Cptr--;

  }

  if (*Cptr == '.') {

    for (i = 0; mFileTypes[i].Extension != NULL; i++) {
      len = strlen (mFileTypes[i].Extension);
      if (_strnicmp (mFileTypes[i].Extension, Cptr, len) == 0) {
        if ((*(Cptr + len) == 0) || isspace (*(Cptr + len))) {
          return mFileTypes[i].FileType;
        }
      }
    }
  }

  return FILETYPE_UNKNOWN;
}
//
// Determine if a given file is a standard include file. If we don't know,
// then assume it's not.
//
static
int
IsIncludeFile (
  INT8 *FileName
  )
{
  INT8  *Cptr;
  int   len;
  int   i;

  len = strlen (FileName);
  if (len == 0) {
    return 0;
  }

  Cptr = FileName + len - 1;
  while ((*Cptr != '.') && (Cptr >= FileName)) {
    Cptr--;
  }

  if (*Cptr == '.') {
    //
    // Now go through the list of filename extensions and try to find
    // a match for this file extension.
    //
    for (i = 0; mFileTypes[i].Extension != NULL; i++) {
      len = strlen (mFileTypes[i].Extension);
      if (_strnicmp (mFileTypes[i].Extension, Cptr, len) == 0) {
        //
        // Make sure that's all there is to the filename extension.
        //
        if ((*(Cptr + len) == 0) || isspace (*(Cptr + len))) {
          return mFileTypes[i].FileFlags & FILE_FLAG_INCLUDE;
        }
      }
    }
  }

  return 0;
}

/*****************************************************************************
******************************************************************************/
static
void
StripTrailingSpaces (
  INT8 *Str
  )
{
  INT8  *Cptr;
  Cptr = Str + strlen (Str) - 1;
  while (Cptr > Str) {
    if (isspace (*Cptr)) {
      *Cptr = 0;
      Cptr--;
    } else {
      break;
    }
  }
}

/*****************************************************************************
******************************************************************************/
static
int
GetEfiSource (
  VOID
  )
{
  INT8  *EfiSource;

  //
  // Don't set it if the user specified it on the command line.
  //
  EfiSource = GetSymbolValue (EFI_SOURCE);
  if ( EfiSource != NULL) {
    ReplaceSlash (EfiSource);
    if (EfiSource[strlen (EfiSource) - 1] == '\\') {
      EfiSource[strlen (EfiSource) - 1] = 0;
    }    
    return STATUS_SUCCESS;
  }

  //
  // Get the environmental variable setting of EFI_SOURCE. 
  //
  EfiSource = getenv (EFI_SOURCE);
  if (EfiSource != NULL) {
    ReplaceSlash (EfiSource);
    if (EfiSource[strlen (EfiSource) - 1] == '\\') {
      EfiSource[strlen (EfiSource) - 1] = 0;
    }
    AddSymbol (EFI_SOURCE, EfiSource, SYM_GLOBAL | SYM_FILEPATH); 
    return STATUS_SUCCESS;
  }

  Error (NULL, 0, 0, NULL, "could not determine EFI_SOURCE");
  return STATUS_ERROR;
}

void
Message (
  UINT32  PrintMask,
  INT8    *Fmt,
  ...
  )
{
  INT8    Line[MAX_LINE_LEN];
  va_list List;

  va_start (List, Fmt);
  vsprintf (Line, Fmt, List);
  if (PrintMask & gGlobals.Verbose) {
    fprintf (stdout, "%s\n", Line);
  }

  va_end (List);
}

static
void
Usage (
  VOID
  )
{
  int         Index;
  const char  *Str[] = {
    UTILITY_NAME" "UTILITY_VERSION" - Intel Process DSC File Utility",
    "  Copyright (C), 2004 - 2008 Intel Corporation",
    
#if ( defined(UTILITY_BUILD) && defined(UTILITY_VENDOR) )
    "  Built from "UTILITY_BUILD", project of "UTILITY_VENDOR,
#endif
    "",
    "Usage:",
    "  "UTILITY_NAME" [OPTION]... DSCFILE",
    "Options:",
    "  -d var=value        to define symbol 'var' to 'value'",
    "  -v                  for verbose mode",
    "  -g filename         to preparse GUID listing file",
    "  -x filename         to create a cross-reference file",
    "  -n threadnumber     to build with multi-thread",
    "  -t target           to build the specified target:",
    "                      all, libraries or components",
    NULL
  };
  for (Index = 0; Str[Index] != NULL; Index++) {
    fprintf (stdout, "%s\n", Str[Index]);
  }
}

/*++

Routine Description:
  
  Process the [defines] section in the DSC file.

Arguments:

  DscFile - pointer to the DSCFile class that contains the relevant info.

Returns:

  0 if not necessarily an absolute path
  1 otherwise

--*/
static
int
ProcessDSCDefinesSection (
  DSC_FILE *DscFile
  )
{
  INT8    Line[MAX_LINE_LEN];
  INT8    Line2[MAX_EXP_LINE_LEN];
  INT8    *Cptr;
  SECTION *Sect;

  //
  // Look for a [defines] section and process it
  //
  Sect = DSCFileFindSection (DscFile, DEFINES_SECTION_NAME);
  if (Sect == NULL) {
    return STATUS_ERROR;
  }
  //
  // Read lines while they're valid
  //
  while (DSCFileGetLine (DscFile, Line, sizeof (Line)) != NULL) {
    //
    // Expand symbols on the line
    //
    if (ExpandSymbols (Line, Line2, sizeof (Line2), 0)) {
      return STATUS_ERROR;
    }
    //
    // Strip the line
    //
    Cptr = StripLine (Line2);
    if (*Cptr) {
      //
      // Make the assignment
      //
      AddSymbol (Line2, NULL, SYM_OVERWRITE | SYM_GLOBAL);
    }
  }

  return STATUS_SUCCESS;
}

int
IsAbsolutePath (
  char    *FileName
  )
/*++

Routine Description:
  
  Determine if a given filename contains the full path information.

Arguments:

  FileName - the name of the file, with symbol expanded.

Returns:

  0 if not necessarily an absolute path
  1 otherwise

--*/
{
  //
  // If the first character is a-z, and the second character is a colon, then
  // it is an absolute path.
  //
  if (isalpha (FileName[0]) && (FileName[1] == ':')) {
    return 1;
  }

  return 0;
}

SMART_FILE *
SmartOpen (
  char        *FileName
  )
{
  SMART_FILE     *SmartFile;
  FILE           *Fptr;
  int            FileSize;
  
  SmartFile = malloc (sizeof (SMART_FILE));
  if (SmartFile == NULL) { 
    return NULL;
  }
  memset (SmartFile, 0, sizeof (SMART_FILE));
  
  SmartFile->FileName = malloc (strlen (FileName) + 1);
  if (SmartFile->FileName == NULL){
    SmartFree (SmartFile); 
    return NULL;
  }
  strcpy (SmartFile->FileName, FileName);

  if ((Fptr = fopen (FileName, "r")) != NULL) {
    fseek (Fptr, 0, SEEK_END);
    FileSize = ftell (Fptr);
    fseek (Fptr, 0, SEEK_SET);
    SmartFile->FileContent = malloc (FileSize + 1);
    if (SmartFile->FileContent != NULL) {
      memset (SmartFile->FileContent, 0, FileSize + 1);
      //
      // Usually FileLength < FileSize, because in text mode, carriage return-linefeed
      // combinations are translated into single linefeeds on input
      //       
      SmartFile->FileLength = fread (SmartFile->FileContent, sizeof(char), FileSize, Fptr);
    }
    fclose (Fptr);
  }
  
  //
  // No previous output file content, re-create the file
  //
  if (SmartFile->FileContent == NULL) {
    if ((SmartFile->FilePtr = fopen (FileName, "w")) == NULL) {
      SmartFree (SmartFile);
      return NULL;
    }    
  }
  
  return SmartFile;
}

int
SmartWrite (
  SMART_FILE  *SmartFile,
  char        *String
  )
{
  int  StrLen;
  
  if (SmartFile->FilePtr != NULL) {
    return fprintf (SmartFile->FilePtr, "%s", String);
  } else {
    StrLen = strlen (String);
    if ((StrLen > SmartFile->FileLength - SmartFile->FilePosition) || 
       (_strnicmp (&SmartFile->FileContent[SmartFile->FilePosition], String, StrLen) != 0)) {
      //
      // file changed, need to re-create.
      //
      if ((SmartFile->FilePtr = fopen (SmartFile->FileName, "w")) == NULL) {
        Error (NULL, 0, 0, SmartFile->FileName, "could not open file for writing when SmartWrite");
        return -1;
      } else {
        SmartFile->FileContent[SmartFile->FilePosition] = 0;
        fprintf (SmartFile->FilePtr, "%s%s", SmartFile->FileContent, String);
        return StrLen;
      }      
    } else {
      SmartFile->FilePosition += StrLen;
      return StrLen;
    }
  }
}

void
SmartClose (
  SMART_FILE  *SmartFile
  )
{
  if ((SmartFile->FilePtr == NULL) && (SmartFile->FilePosition < SmartFile->FileLength)) {
    //
    // The new file is smaller than before, re-create it.
    //
    if ((SmartFile->FilePtr = fopen (SmartFile->FileName, "w")) == NULL) {
      Error (NULL, 0, 0, SmartFile->FileName, "could not open file for writing when SmartClose");
    } else {
      SmartFile->FileContent[SmartFile->FilePosition] = 0;
      fprintf (SmartFile->FilePtr, "%s", SmartFile->FileContent);
    }
  }
  
  SmartFree(SmartFile);
}
  
static
void
SmartFree (
  SMART_FILE  *SmartFile
  )
{
  if (SmartFile == NULL) {
    return;
  }
  
  if (SmartFile->FileName != NULL ) {
    free (SmartFile->FileName);
  }
  
  if (SmartFile->FileContent != NULL ) {
    free (SmartFile->FileContent);
  }

  if (SmartFile->FilePtr != NULL ) {
    fclose (SmartFile->FilePtr);
  } 

  free (SmartFile);
  
  return;
}

static 
int
AddModuleName (
  SYMBOL  **SymbolList,
  INT8    *ModuleName,
  INT8    *InfName
  )
/*++

Routine Description:
  
  Add module name in the global module list. 
  For the same module names, it is only added once.

Arguments:
  SymbolList : add name into this list
  ModuleName : point to one module name char string.
  InfName    : point to this module inf file name with path.

Returns:

  0 : Successfully add input name into the global list.
  other value : allocate memory failed.

--*/
{
  SYMBOL *CurrentSymbol;
  SYMBOL *LastSymbol;
  
  //
  // Get the global module list.
  //
  CurrentSymbol = *SymbolList;
  LastSymbol    = *SymbolList;
  
  //
  // Search whether this module name has been added into the global list.
  //
  while (CurrentSymbol != NULL) {
    if (_stricmp (CurrentSymbol->Name, ModuleName) == 0) {
      if ((CurrentSymbol->Value == NULL) && (InfName == NULL)) {
        break;
      } else if ((CurrentSymbol->Value != NULL) && (InfName != NULL) && \
        (_stricmp (CurrentSymbol->Value, InfName) == 0)) {
        break;
      }
    }
    LastSymbol    = CurrentSymbol;
    CurrentSymbol = CurrentSymbol->Next;
  }
  
  //
  // Add new module name in list.
  //
  if (CurrentSymbol == NULL) {
    CurrentSymbol = (SYMBOL *) malloc (sizeof (SYMBOL));
    if (CurrentSymbol == NULL) {
      Error (NULL, 0, 0, NULL, "failed to allocate memory");
      return -1;
    }
    memset ((INT8 *) CurrentSymbol, 0, sizeof (SYMBOL));

    if (ModuleName != NULL) {
      CurrentSymbol->Name   = (INT8 *) malloc (strlen (ModuleName) + 1);
      strcpy (CurrentSymbol->Name, ModuleName);
    }

    if (InfName != NULL) {
      CurrentSymbol->Value  = (INT8 *) malloc (strlen (InfName) + 1);
      strcpy (CurrentSymbol->Value, InfName);
    }
    
    if (LastSymbol == NULL) {   
      *SymbolList      = CurrentSymbol;
    } else {
      LastSymbol->Next = CurrentSymbol;
    }
  }

  return 0;
}

 
static 
void
ReplaceSlash (
  INT8    *Path
  )
/*++

Routine Description:
  
  Replace '/' with '\\'

Returns:

--*/
{
  while (*Path) {
    if (*Path == '/') {
      *Path = '\\';
    }
    Path++;
  }
}
