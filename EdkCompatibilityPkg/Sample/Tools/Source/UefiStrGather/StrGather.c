/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  StrGather.c

Abstract:

  Parse a strings file and create or add to a string database file.

--*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <Tiano.h>
#include <EfiUtilityMsgs.h>
#include <EfiHii.h>
#include "StrGather.h"
#include "StringDB.h"

#define UTILITY_NAME     "StrGather"
#define UTILITY_VERSION  "v1.2"

typedef UINT16  WCHAR;

#define MAX_PATH                    1024
#define MAX_NEST_DEPTH              20  // just in case we get in an endless loop.
#define MAX_STRING_IDENTIFIER_NAME  128 // number of wchars
#define MAX_LINE_LEN                400
#define STRING_TOKEN                "STRING_TOKEN"
#define DEFAULT_BASE_NAME           "BaseName"
//
// Operational modes for this utility
//
#define MODE_UNKNOWN  0
#define MODE_PARSE    1
#define MODE_SCAN     2
#define MODE_DUMP     3

//
// This is how we invoke the C preprocessor on the source file
// to resolve #if, #else, etc.
//
#define PREPROCESSOR_COMMAND                "cl"
#define PREPROCESSOR_OPTIONS                "/nologo /EP /TC /DSTRGATHER"
#define PREPROCESS_TEMP_FILE_EXTENSION      ".ii"
#define PREPROCESS_OUTPUT_FILE_EXTENSION    ".iii"

//
// We keep a linked list of these for the source files we process
//
typedef struct _SOURCE_FILE {
  FILE                *Fptr;
  WCHAR               *FileBuffer;
  WCHAR               *FileBufferPtr;
  UINT32              FileSize;
  INT8                FileName[MAX_PATH];
  UINT32              LineNum;
  BOOLEAN             EndOfFile;
  BOOLEAN             SkipToHash;
  struct _SOURCE_FILE *Previous;
  struct _SOURCE_FILE *Next;
  WCHAR               ControlCharacter;
} SOURCE_FILE;

#define DEFAULT_CONTROL_CHARACTER UNICODE_SLASH

//
// Here's all our globals. We need a linked list of include paths, a linked
// list of source files, a linked list of subdirectories (appended to each
// include path when searching), and a couple other fields.
//
static struct {
  SOURCE_FILE                 SourceFiles;
  TEXT_STRING_LIST            *IncludePaths;                    // all include paths to search
  TEXT_STRING_LIST            *LastIncludePath;
  TEXT_STRING_LIST            *ScanFileName;
  TEXT_STRING_LIST            *LastScanFileName;
  TEXT_STRING_LIST            *SkipExt;                         // if -skipext .uni
  TEXT_STRING_LIST            *LastSkipExt;
  TEXT_STRING_LIST            *IndirectionFileName;
  TEXT_STRING_LIST            *LastIndirectionFileName;
  TEXT_STRING_LIST            *DatabaseFileName;
  TEXT_STRING_LIST            *LastDatabaseFileName;
  TEXT_STRING_LIST            *PreprocessFlags;
  TEXT_STRING_LIST            *LastPreprocessFlags;
  WCHAR_STRING_LIST           *Language;
  WCHAR_STRING_LIST           *LastLanguage;
  WCHAR_MATCHING_STRING_LIST  *IndirectionList;                 // from indirection file(s)
  WCHAR_MATCHING_STRING_LIST  *LastIndirectionList;
  BOOLEAN                     Verbose;                          // for more detailed output
  BOOLEAN                     VerboseDatabaseWrite;             // for more detailed output when writing database
  BOOLEAN                     VerboseDatabaseRead;              // for more detailed output when reading database
  BOOLEAN                     NewDatabase;                      // to start from scratch
  BOOLEAN                     IgnoreNotFound;                   // when scanning
  BOOLEAN                     VerboseScan;
  BOOLEAN                     UnquotedStrings;                  // -uqs option
  BOOLEAN                     Preprocess;                       // -ppflag option
  INT8                        PreprocessFileName[MAX_PATH];
  INT8                        OutputDatabaseFileName[MAX_PATH];
  INT8                        StringHFileName[MAX_PATH];
  INT8                        StringCFileName[MAX_PATH];        // output .C filename
  INT8                        DumpUFileName[MAX_PATH];          // output unicode dump file name
  INT8                        HiiExportPackFileName[MAX_PATH];  // HII export pack file name
  INT8                        BaseName[MAX_PATH];               // base filename of the strings file
  INT8                        OutputDependencyFileName[MAX_PATH];
  FILE                        *OutputDependencyFptr;
  UINT32                      Mode;
} mGlobals;

static
BOOLEAN
IsValidIdentifierChar (
  INT8      Char,
  BOOLEAN   FirstChar
  );

static
void
RewindFile (
  SOURCE_FILE *SourceFile
  );

static
BOOLEAN
SkipTo (
  SOURCE_FILE *SourceFile,
  WCHAR       WChar,
  BOOLEAN     StopAfterNewline
  );

static
UINT32
SkipWhiteSpace (
  SOURCE_FILE *SourceFile
  );

static
BOOLEAN
IsWhiteSpace (
  SOURCE_FILE *SourceFile
  );

static
BOOLEAN
EndOfFile (
  SOURCE_FILE *SourceFile
  );

static
void
PreprocessFile (
  SOURCE_FILE *SourceFile
  );

static
UINT32
GetStringIdentifierName (
  IN SOURCE_FILE  *SourceFile,
  IN OUT WCHAR    *StringIdentifierName,
  IN UINT32       StringIdentifierNameLen
  );

static
STATUS
GetLanguageIdentifierName (
  IN SOURCE_FILE  *SourceFile,
  IN OUT WCHAR    *LanguageIdentifierName,
  IN UINT32       LanguageIdentifierNameLen,
  IN BOOLEAN      Optional
  );

static
WCHAR *
GetPrintableLanguageName (
  IN SOURCE_FILE  *SourceFile
  );

static
STATUS
AddCommandLineLanguage (
  IN INT8          *Language
  );

static
WCHAR *
GetQuotedString (
  SOURCE_FILE *SourceFile,
  BOOLEAN     Optional
  );

static
STATUS
ProcessIncludeFile (
  SOURCE_FILE *SourceFile,
  SOURCE_FILE *ParentSourceFile
  );

static
STATUS
ParseFile (
  SOURCE_FILE *SourceFile
  );

static
FILE  *
FindFile (
  IN INT8     *FileName,
  OUT INT8    *FoundFileName,
  IN UINT32   FoundFileNameLen
  );

static
STATUS
ProcessArgs (
  int   Argc,
  char  *Argv[]
  );

static
STATUS
ProcessFile (
  SOURCE_FILE *SourceFile
  );

static
UINT32
wstrcmp (
  WCHAR *Buffer,
  WCHAR *Str
  );

static
WCHAR *
wstrcatenate (
  WCHAR *Dst,
  WCHAR *Src
  );

static
void
Usage (
  VOID
  );

static
void
FreeLists (
  VOID
  );

static
void
ProcessTokenString (
  SOURCE_FILE *SourceFile
  );

static
void
ProcessTokenInclude (
  SOURCE_FILE *SourceFile
  );

static
void
ProcessTokenScope (
  SOURCE_FILE *SourceFile
  );

static
void
ProcessTokenLanguage (
  SOURCE_FILE *SourceFile
  );

static
void
ProcessTokenLangDef (
  SOURCE_FILE *SourceFile
  );

static
VOID
ProcessTokenSecondaryLangDef (
  SOURCE_FILE *SourceFile
  );

static
STATUS
ScanFiles (
  TEXT_STRING_LIST *ScanFiles
  );

static
STATUS
ParseIndirectionFiles (
  TEXT_STRING_LIST    *Files
  );

int
main (
  int   Argc,
  char  *Argv[]
  )
/*++

Routine Description:

  Call the routine to parse the command-line options, then process the file.

Arguments:

  Argc - Standard C main() argc and argv.
  Argv - Standard C main() argc and argv.

Returns:

  0       if successful
  nonzero otherwise

--*/
{
  STATUS  Status;

  SetUtilityName (UTILITY_NAME);
  //
  // Process the command-line arguments
  //
  Status = ProcessArgs (Argc, Argv);
  if (Status != STATUS_SUCCESS) {
    return Status;
  }
  //
  // Initialize the database manager
  //
  StringDBConstructor ();
  //
  // We always try to read in an existing database file. It may not
  // exist, which is ok usually.
  //
  if (mGlobals.NewDatabase == 0) {
    //
    // Read all databases specified.
    //
    for (mGlobals.LastDatabaseFileName = mGlobals.DatabaseFileName;
         mGlobals.LastDatabaseFileName != NULL;
         mGlobals.LastDatabaseFileName = mGlobals.LastDatabaseFileName->Next
        ) {
      Status = StringDBReadDatabase (mGlobals.LastDatabaseFileName->Str, TRUE, mGlobals.VerboseDatabaseRead);
      if (Status != STATUS_SUCCESS) {
        return Status;
      }
    }
  }
  //
  // Read indirection file(s) if specified
  //
  if (ParseIndirectionFiles (mGlobals.IndirectionFileName) != STATUS_SUCCESS) {
    goto Finish;
  }
  //
  // If scanning source files, do that now
  //
  if (mGlobals.Mode == MODE_SCAN) {
    ScanFiles (mGlobals.ScanFileName);
  } else if (mGlobals.Mode == MODE_PARSE) {
    //
    // Parsing a unicode strings file
    //
    mGlobals.SourceFiles.ControlCharacter = DEFAULT_CONTROL_CHARACTER;
    if (mGlobals.OutputDependencyFileName[0] != 0) {
      if ((mGlobals.OutputDependencyFptr = fopen (mGlobals.OutputDependencyFileName, "w")) == NULL) {
        Error (NULL, 0, 0, mGlobals.OutputDependencyFileName, "failed to open output dependency file");
        goto Finish;
      }
    }
    Status = ProcessIncludeFile (&mGlobals.SourceFiles, NULL);
    if (mGlobals.OutputDependencyFptr != NULL) {
      fclose (mGlobals.OutputDependencyFptr);
    }
    if (Status != STATUS_SUCCESS) {
      goto Finish;
    }
  }
  //
  // Create the string defines header file if there have been no errors.
  //
  ParserSetPosition (NULL, 0);
  if ((mGlobals.StringHFileName[0] != 0) && (GetUtilityStatus () < STATUS_ERROR)) {
    Status = StringDBDumpStringDefines (mGlobals.StringHFileName, mGlobals.BaseName);
    if (Status != EFI_SUCCESS) {
      goto Finish;
    }
  }

  //
  // Dump the strings to a .c file if there have still been no errors.
  //
  if ((mGlobals.StringCFileName[0] != 0) && (GetUtilityStatus () < STATUS_ERROR)) {
    Status = StringDBDumpCStrings (
              mGlobals.BaseName,
              mGlobals.StringCFileName,
              mGlobals.Language
              );
    if (Status != EFI_SUCCESS) {
      goto Finish;
    }
  }

  //
  // Dump the database if requested
  //
  if ((mGlobals.DumpUFileName[0] != 0) && (GetUtilityStatus () < STATUS_ERROR)) {
    StringDBDumpDatabase (NULL, mGlobals.DumpUFileName, FALSE);
  }
  //
  // Dump the string data as HII binary string pack if requested
  //
  if ((mGlobals.HiiExportPackFileName[0] != 0) && (GetUtilityStatus () < STATUS_ERROR)) {
    StringDBCreateHiiExportPack (mGlobals.HiiExportPackFileName, mGlobals.Language);
  }
  //
  // Always update the database if no errors and not in dump mode. If they specified -od
  // for an output database file name, then use that name. Otherwise use the name of
  // the first database file specified with -db
  //
  if ((mGlobals.Mode != MODE_DUMP) && (GetUtilityStatus () < STATUS_ERROR)) {
    if (mGlobals.OutputDatabaseFileName[0]) {
      Status = StringDBWriteDatabase (mGlobals.OutputDatabaseFileName, mGlobals.VerboseDatabaseWrite);
    } else {
      Status = StringDBWriteDatabase (mGlobals.DatabaseFileName->Str, mGlobals.VerboseDatabaseWrite);
    }

    if (Status != EFI_SUCCESS) {
      goto Finish;
    }
  }

Finish:
  //
  // Free up memory
  //
  FreeLists ();
  StringDBDestructor ();
  return GetUtilityStatus ();
}

static
STATUS
ProcessIncludeFile (
  SOURCE_FILE *SourceFile,
  SOURCE_FILE *ParentSourceFile
  )
/*++

Routine Description:

  Given a source file, open the file and parse it

Arguments:

  SourceFile        - name of file to parse
  ParentSourceFile  - for error reporting purposes, the file that #included SourceFile.

Returns:

  Standard status.

--*/
{
  static UINT32 NestDepth = 0;
  INT8          FoundFileName[MAX_PATH];
  STATUS        Status;

  Status = STATUS_SUCCESS;
  NestDepth++;
  //
  // Print the file being processed. Indent so you can tell the include nesting
  // depth.
  //
  if (mGlobals.Verbose) {
    fprintf (stdout, "%*cProcessing file '%s'\n", NestDepth * 2, ' ', SourceFile->FileName);
  }

  //
  // Make sure we didn't exceed our maximum nesting depth
  //
  if (NestDepth > MAX_NEST_DEPTH) {
    Error (NULL, 0, 0, SourceFile->FileName, "max nesting depth (%d) exceeded", NestDepth);
    Status = STATUS_ERROR;
    goto Finish;
  }
  //
  // Try to open the file locally, and if that fails try along our include paths.
  //
  strcpy (FoundFileName, SourceFile->FileName);
  if ((SourceFile->Fptr = fopen (FoundFileName, "rb")) == NULL) {
    //
    // Try to find it among the paths if it has a parent (that is, it is included
    // by someone else).
    //
    if (ParentSourceFile == NULL) {
      Error (NULL, 0, 0, SourceFile->FileName, "file not found");
      Status = STATUS_ERROR;
      goto Finish;
    }

    SourceFile->Fptr = FindFile (SourceFile->FileName, FoundFileName, sizeof (FoundFileName));
    if (SourceFile->Fptr == NULL) {
      Error (ParentSourceFile->FileName, ParentSourceFile->LineNum, 0, SourceFile->FileName, "include file not found");
      Status = STATUS_ERROR;
      goto Finish;
    }
  }

  //
  // Output the dependency
  //
  if (mGlobals.OutputDependencyFptr != NULL) {
    fprintf (mGlobals.OutputDependencyFptr, "%s : %s\n", mGlobals.DatabaseFileName->Str, FoundFileName);
    //
    // Add pseudo target to avoid incremental build failure when the file is deleted
    //
    fprintf (mGlobals.OutputDependencyFptr, "%s : \n", FoundFileName);
  }

  //
  // Process the file found
  //
  ProcessFile (SourceFile);

Finish:
  NestDepth--;
  //
  // Close open files and return status
  //
  if (SourceFile->Fptr != NULL) {
    fclose (SourceFile->Fptr);
  }

  return Status;
}

static
STATUS
ProcessFile (
  SOURCE_FILE *SourceFile
  )
{
  //
  // Get the file size, and then read the entire thing into memory.
  // Allocate space for a terminator character.
  //
  fseek (SourceFile->Fptr, 0, SEEK_END);
  SourceFile->FileSize = ftell (SourceFile->Fptr);
  fseek (SourceFile->Fptr, 0, SEEK_SET);
  SourceFile->FileBuffer = (WCHAR *) malloc (SourceFile->FileSize + sizeof (WCHAR));
  if (SourceFile->FileBuffer == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    return STATUS_ERROR;
  }

  fread ((VOID *) SourceFile->FileBuffer, SourceFile->FileSize, 1, SourceFile->Fptr);
  SourceFile->FileBuffer[(SourceFile->FileSize / sizeof (WCHAR))] = UNICODE_NULL;
  //
  // Pre-process the file to replace comments with spaces
  //
  PreprocessFile (SourceFile);
  //
  // Parse the file
  //
  ParseFile (SourceFile);
  free (SourceFile->FileBuffer);
  return STATUS_SUCCESS;
}

static
STATUS
ParseFile (
  SOURCE_FILE *SourceFile
  )
{
  BOOLEAN InComment;
  UINT32  Len;

  //
  // First character of a unicode file is special. Make sure
  //
  if (SourceFile->FileBufferPtr[0] != UNICODE_FILE_START) {
    Error (SourceFile->FileName, 1, 0, SourceFile->FileName, "file does not appear to be a unicode file");
    return STATUS_ERROR;
  }

  SourceFile->FileBufferPtr++;
  InComment = FALSE;
  //
  // Print the first line if in verbose mode
  //
  if (mGlobals.Verbose) {
    printf ("%d: %S\n", SourceFile->LineNum, SourceFile->FileBufferPtr);
  }
  //
  // Since the syntax is relatively straightforward, just switch on the next char
  //
  while (!EndOfFile (SourceFile)) {
    //
    // Check for whitespace
    //
    if (SourceFile->FileBufferPtr[0] == UNICODE_SPACE) {
      SourceFile->FileBufferPtr++;
    } else if (SourceFile->FileBufferPtr[0] == UNICODE_TAB) {
      SourceFile->FileBufferPtr++;
    } else if (SourceFile->FileBufferPtr[0] == UNICODE_CR) {
      SourceFile->FileBufferPtr++;
    } else if (SourceFile->FileBufferPtr[0] == UNICODE_LF) {
      SourceFile->FileBufferPtr++;
      SourceFile->LineNum++;
      if (mGlobals.Verbose) {
        printf ("%d: %S\n", SourceFile->LineNum, SourceFile->FileBufferPtr);
      }

      InComment = FALSE;
    } else if (SourceFile->FileBufferPtr[0] == 0) {
      SourceFile->FileBufferPtr++;
    } else if (InComment) {
      SourceFile->FileBufferPtr++;
    } else if ((SourceFile->FileBufferPtr[0] == UNICODE_SLASH) && (SourceFile->FileBufferPtr[1] == UNICODE_SLASH)) {
      SourceFile->FileBufferPtr += 2;
      InComment = TRUE;
    } else if (SourceFile->SkipToHash && (SourceFile->FileBufferPtr[0] != SourceFile->ControlCharacter)) {
      SourceFile->FileBufferPtr++;
    } else {
      SourceFile->SkipToHash = FALSE;
      if ((SourceFile->FileBufferPtr[0] == SourceFile->ControlCharacter) &&
          ((Len = wstrcmp (SourceFile->FileBufferPtr + 1, L"include")) > 0)
          ) {
        SourceFile->FileBufferPtr += Len + 1;
        ProcessTokenInclude (SourceFile);
      } else if ((SourceFile->FileBufferPtr[0] == SourceFile->ControlCharacter) &&
               (Len = wstrcmp (SourceFile->FileBufferPtr + 1, L"scope")) > 0
              ) {
        SourceFile->FileBufferPtr += Len + 1;
        ProcessTokenScope (SourceFile);
      } else if ((SourceFile->FileBufferPtr[0] == SourceFile->ControlCharacter) &&
               (Len = wstrcmp (SourceFile->FileBufferPtr + 1, L"language")) > 0
              ) {
        SourceFile->FileBufferPtr += Len + 1;
        ProcessTokenLanguage (SourceFile);
      } else if ((SourceFile->FileBufferPtr[0] == SourceFile->ControlCharacter) &&
               (Len = wstrcmp (SourceFile->FileBufferPtr + 1, L"langdef")) > 0
              ) {
        SourceFile->FileBufferPtr += Len + 1;
        ProcessTokenLangDef (SourceFile);
      } else if ((SourceFile->FileBufferPtr[0] == SourceFile->ControlCharacter) &&
                 (Len = wstrcmp (SourceFile->FileBufferPtr + 1, L"secondarylang")) > 0
                ) {
        SourceFile->FileBufferPtr += Len + 1;
        ProcessTokenSecondaryLangDef (SourceFile);
      } else if ((SourceFile->FileBufferPtr[0] == SourceFile->ControlCharacter) &&
               (Len = wstrcmp (SourceFile->FileBufferPtr + 1, L"string")) > 0
              ) {
        SourceFile->FileBufferPtr += Len + 1;
        ProcessTokenString (SourceFile);
      } else if ((SourceFile->FileBufferPtr[0] == SourceFile->ControlCharacter) &&
               (Len = wstrcmp (SourceFile->FileBufferPtr + 1, L"EFI_BREAKPOINT()")) > 0
              ) {
        SourceFile->FileBufferPtr += Len;
        EFI_BREAKPOINT ();
      } else if ((SourceFile->FileBufferPtr[0] == SourceFile->ControlCharacter) &&
               (SourceFile->FileBufferPtr[1] == UNICODE_EQUAL_SIGN)
              ) {
        SourceFile->ControlCharacter = SourceFile->FileBufferPtr[2];
        SourceFile->FileBufferPtr += 3;
      } else {
        Error (SourceFile->FileName, SourceFile->LineNum, 0, "unrecognized token", "%S", SourceFile->FileBufferPtr);
        //
        // Treat rest of line as a comment.
        //
        InComment = TRUE;
      }
    }
  }

  return STATUS_SUCCESS;
}

static
void
PreprocessFile (
  SOURCE_FILE *SourceFile
  )
/*++

Routine Description:
  Preprocess a file to replace all carriage returns with NULLs so
  we can print lines from the file to the screen.

Arguments:
  SourceFile - structure that we use to keep track of an input file.

Returns:
  Nothing.

--*/
{
  BOOLEAN InComment;

  RewindFile (SourceFile);
  InComment = FALSE;
  while (!EndOfFile (SourceFile)) {
    //
    // If a line-feed, then no longer in a comment
    //
    if (SourceFile->FileBufferPtr[0] == UNICODE_LF) {
      SourceFile->FileBufferPtr++;
      SourceFile->LineNum++;
      InComment = 0;
    } else if (SourceFile->FileBufferPtr[0] == UNICODE_CR) {
      //
      // Replace all carriage returns with a NULL so we can print stuff
      //
      SourceFile->FileBufferPtr[0] = 0;
      SourceFile->FileBufferPtr++;
    } else if (InComment) {
      SourceFile->FileBufferPtr[0] = UNICODE_SPACE;
      SourceFile->FileBufferPtr++;
    } else if ((SourceFile->FileBufferPtr[0] == UNICODE_SLASH) && (SourceFile->FileBufferPtr[1] == UNICODE_SLASH)) {
      SourceFile->FileBufferPtr += 2;
      InComment = TRUE;
    } else {
      SourceFile->FileBufferPtr++;
    }
  }
  //
  // Could check for end-of-file and still in a comment, but
  // should not be necessary. So just restore the file pointers.
  //
  RewindFile (SourceFile);
}

static
WCHAR *
GetPrintableLanguageName (
  IN SOURCE_FILE  *SourceFile
  )
{
  WCHAR   *String;
  WCHAR   *Start;
  WCHAR   *Ptr;
  UINT32  Len;

  SkipWhiteSpace (SourceFile);
  if (SourceFile->FileBufferPtr[0] != UNICODE_DOUBLE_QUOTE) {
    Error (SourceFile->FileName, SourceFile->LineNum, 0, "expected quoted printable language name", "%S", SourceFile->FileBufferPtr);
    SourceFile->SkipToHash = TRUE;
    return NULL;
  }

  Len = 0;
  SourceFile->FileBufferPtr++;
  Start = Ptr = SourceFile->FileBufferPtr;
  while (!EndOfFile (SourceFile)) {
    if (SourceFile->FileBufferPtr[0] == UNICODE_CR) {
      Warning (SourceFile->FileName, SourceFile->LineNum, 0, "carriage return found in quoted string", "%S", Start);
      break;
    } else if (SourceFile->FileBufferPtr[0] == UNICODE_DOUBLE_QUOTE) {
      break;
    }

    SourceFile->FileBufferPtr++;
    Len++;
  }

  if (SourceFile->FileBufferPtr[0] != UNICODE_DOUBLE_QUOTE) {
    Warning (
      SourceFile->FileName,
      SourceFile->LineNum,
      0,
      "missing closing quote on printable language name string",
      "%S",
      Start
      );
  } else {
    SourceFile->FileBufferPtr++;
  }
  //
  // Now allocate memory for the string and save it off
  //
  String = (WCHAR *) malloc ((Len + 1) * sizeof (WCHAR));
  if (String == NULL) {
    Error (NULL, 0, 0, "memory allocation failed", NULL);
    return NULL;
  }
  //
  // Copy the string from the file buffer to the local copy.
  // We do no reformatting of it whatsoever at this point.
  //
  Ptr = String;
  while (Len > 0) {
    *Ptr = *Start;
    Start++;
    Ptr++;
    Len--;
  }

  *Ptr = 0;
  //
  // Now format the string to convert \wide and \narrow controls
  //
  StringDBFormatString (String);
  return String;
}

static struct {
  WCHAR *ISO639;
  WCHAR *RFC3066;
} LanguageConvertTable[] = {
  { L"eng", L"en-US" },
  { L"fra", L"fr-FR" },
  { L"spa", L"es-ES" },
  { NULL, NULL }
};

WCHAR *
GetLangCode (
  IN WCHAR                        *Lang
  )
{
  UINT32  Index;
  WCHAR   *LangCode;

  LangCode = NULL;

  //
  // The Lang is xx-XX format and return.
  //
  if (wcschr (Lang, L'-') != NULL) {
    LangCode = (WCHAR *) malloc ((wcslen (Lang) + 1) * sizeof(WCHAR));
    if (LangCode != NULL) {
      wcscpy (LangCode, Lang);
    }
    return LangCode;
  }

  //
  // Convert the language accoring to the table.
  //
  for (Index = 0; LanguageConvertTable[Index].ISO639 != NULL; Index++) {
    if (wcscmp(LanguageConvertTable[Index].ISO639, Lang) == 0) {
      LangCode = (WCHAR *) malloc ((wcslen (LanguageConvertTable[Index].RFC3066) + 1) * sizeof (WCHAR));
      if (LangCode != NULL) {
        wcscpy (LangCode, LanguageConvertTable[Index].RFC3066);
      }
      return LangCode;
    }
  }

  return NULL;
}

WCHAR *
GetLangCodeList (
  IN WCHAR                        *SecondaryLangList
  )
{
  WCHAR *CodeBeg, *CodeEnd;
  WCHAR *CodeRet;
  WCHAR *LangCodeList = NULL;
  WCHAR *TempLangCodeList = NULL;

  TempLangCodeList = (WCHAR *) malloc ((wcslen(SecondaryLangList) + 1) * sizeof(WCHAR));
  if (TempLangCodeList == NULL) {
    return NULL;
  }
  wcscpy (TempLangCodeList, SecondaryLangList);
  CodeBeg = TempLangCodeList;

  while (CodeBeg != NULL) {
    CodeEnd = wcschr (CodeBeg, L';');
    if (CodeEnd != NULL) {
      *CodeEnd = L'\0';
      CodeEnd++;
    }

    CodeRet = GetLangCode (CodeBeg);
    if (CodeRet != NULL) {
      if (LangCodeList != NULL) {
        LangCodeList = wstrcatenate (LangCodeList, L";");
      }
      LangCodeList = wstrcatenate (LangCodeList, CodeRet);
    }

    CodeBeg = CodeEnd;
    FREE (CodeRet);
  }

  free (TempLangCodeList);

  return LangCodeList;
}

static
WCHAR *
GetSecondaryLanguageList (
  IN SOURCE_FILE  *SourceFile
  )
{
  WCHAR   *SecondaryLangList = NULL;
  WCHAR   SecondaryLang[MAX_STRING_IDENTIFIER_NAME + 1];
  WCHAR   *LangCodeList;
  WCHAR   *Start;
  WCHAR   *Ptr;
  UINT32  Index;

  SkipWhiteSpace (SourceFile);

  if (SourceFile->FileBufferPtr[0] != UNICODE_OPEN_PAREN) {
    Error (SourceFile->FileName, SourceFile->LineNum, 0, "expected open bracket", "%S", SourceFile->FileBufferPtr);
    SourceFile->SkipToHash = TRUE;
    return NULL;
  }

  Index             = 0;
  SecondaryLang [0] = L'\0';
  SourceFile->FileBufferPtr++;
  Start = Ptr = SourceFile->FileBufferPtr;
  while (!EndOfFile (SourceFile)) {
    if (((SourceFile->FileBufferPtr[0] >= UNICODE_a) && (SourceFile->FileBufferPtr[0] <= UNICODE_z)) ||
		((SourceFile->FileBufferPtr[0] >= UNICODE_A) && (SourceFile->FileBufferPtr[0] <= UNICODE_Z)) ||
		(SourceFile->FileBufferPtr[0] == UNICODE_MINUS)) {
      if (Index > MAX_STRING_IDENTIFIER_NAME) {
        Error (SourceFile->FileName, SourceFile->LineNum, 0, "secondary language length is too lang", "%S", SourceFile->FileBufferPtr);
        goto Err;
      }
      SecondaryLang[Index] = SourceFile->FileBufferPtr[0];
      Index++;
    } else if (SourceFile->FileBufferPtr[0] == UNICODE_SPACE) {
      SecondaryLang[Index] = L'\0';

      if (SecondaryLang[0] != L'\0') {
        if (SecondaryLangList != NULL) {
          SecondaryLangList = wstrcatenate (SecondaryLangList, L";");
        }
        SecondaryLangList = wstrcatenate (SecondaryLangList, SecondaryLang);
        Index             = 0;
        SecondaryLang [0] = L'\0';
        SourceFile->FileBufferPtr++;
        continue;
      }
    } else if (SourceFile->FileBufferPtr[0] == UNICODE_CLOSE_PAREN) {
      if (SecondaryLangList != NULL) {
        SecondaryLangList = wstrcatenate (SecondaryLangList, L";");
      }
      SecondaryLangList = wstrcatenate (SecondaryLangList, SecondaryLang);
      break;
    } else {
      Error (SourceFile->FileName, SourceFile->LineNum, 0, "can not recognize the secondary language", "%S", SourceFile->FileBufferPtr);
      goto Err;
    }

    SourceFile->FileBufferPtr++;
  }

  if (SourceFile->FileBufferPtr[0] != UNICODE_CLOSE_PAREN) {
    Error (SourceFile->FileName, SourceFile->LineNum, 0, "missing closing bracket", "%S", Start);
  } else {
    SourceFile->FileBufferPtr++;
  }

  LangCodeList = GetLangCodeList (SecondaryLangList);
  FREE (SecondaryLangList);
  return LangCodeList;

Err:
  FREE(SecondaryLangList);
  return NULL;
}

static
WCHAR *
GetQuotedString (
  SOURCE_FILE *SourceFile,
  BOOLEAN     Optional
  )
{
  WCHAR   *String;
  WCHAR   *Start;
  WCHAR   *Ptr;
  UINT32  Len;
  BOOLEAN PreviousBackslash;

  if (SourceFile->FileBufferPtr[0] != UNICODE_DOUBLE_QUOTE) {
    if (!Optional) {
      Error (SourceFile->FileName, SourceFile->LineNum, 0, "expected quoted string", "%S", SourceFile->FileBufferPtr);
    }

    return NULL;
  }

  Len = 0;
  SourceFile->FileBufferPtr++;
  Start             = Ptr = SourceFile->FileBufferPtr;
  PreviousBackslash = FALSE;
  while (!EndOfFile (SourceFile)) {
    if ((SourceFile->FileBufferPtr[0] == UNICODE_DOUBLE_QUOTE) && (!PreviousBackslash)) {
      break;
    } else if (SourceFile->FileBufferPtr[0] == UNICODE_CR) {
      Warning (SourceFile->FileName, SourceFile->LineNum, 0, "carriage return found in quoted string", "%S", Start);
      PreviousBackslash = FALSE;
    } else if (SourceFile->FileBufferPtr[0] == UNICODE_BACKSLASH) {
      PreviousBackslash = TRUE;
    } else {
      PreviousBackslash = FALSE;
    }

    SourceFile->FileBufferPtr++;
    Len++;
  }

  if (SourceFile->FileBufferPtr[0] != UNICODE_DOUBLE_QUOTE) {
    Warning (SourceFile->FileName, SourceFile->LineNum, 0, "missing closing quote on string", "%S", Start);
  } else {
    SourceFile->FileBufferPtr++;
  }
  //
  // Now allocate memory for the string and save it off
  //
  String = (WCHAR *) malloc ((Len + 1) * sizeof (WCHAR));
  if (String == NULL) {
    Error (NULL, 0, 0, "memory allocation failed", NULL);
    return NULL;
  }
  //
  // Copy the string from the file buffer to the local copy.
  // We do no reformatting of it whatsoever at this point.
  //
  Ptr = String;
  while (Len > 0) {
    *Ptr = *Start;
    Start++;
    Ptr++;
    Len--;
  }

  *Ptr = 0;
  return String;
}
//
// Parse:
//    #string STR_ID_NAME
//
// All we can do is call the string database to add the string identifier. Unfortunately
// he'll have to keep track of the last identifier we added.
//
static
void
ProcessTokenString (
  SOURCE_FILE *SourceFile
  )
{
  WCHAR   StringIdentifier[MAX_STRING_IDENTIFIER_NAME + 1];
  UINT16  StringId;
  //
  // Extract the string identifier name and add it to the database.
  //
  if (GetStringIdentifierName (SourceFile, StringIdentifier, sizeof (StringIdentifier)) > 0) {
    StringId = STRING_ID_INVALID;
    StringDBAddStringIdentifier (StringIdentifier, &StringId, 0);
  } else {
    //
    // Error recovery -- skip to the next #
    //
    SourceFile->SkipToHash = TRUE;
  }
}

static
BOOLEAN
EndOfFile (
  SOURCE_FILE *SourceFile
  )
{
  //
  // The file buffer pointer will typically get updated before the End-of-file flag in the
  // source file structure, so check it first.
  //
  if (SourceFile->FileBufferPtr >= SourceFile->FileBuffer + SourceFile->FileSize / sizeof (WCHAR)) {
    SourceFile->EndOfFile = TRUE;
    return TRUE;
  }

  if (SourceFile->EndOfFile) {
    return TRUE;
  }

  return FALSE;
}

static
UINT32
GetStringIdentifierName (
  IN SOURCE_FILE  *SourceFile,
  IN OUT WCHAR    *StringIdentifierName,
  IN UINT32       StringIdentifierNameLen
  )
{
  UINT32  Len;
  WCHAR   *From;
  WCHAR   *Start;

  //
  // Skip whitespace
  //
  SkipWhiteSpace (SourceFile);
  if (SourceFile->EndOfFile) {
    Error (SourceFile->FileName, SourceFile->LineNum, 0, "end-of-file encountered", "expected string identifier");
    return 0;
  }
  //
  // Verify first character of name is [A-Za-z]
  //
  Len = 0;
  StringIdentifierNameLen /= 2;
  From  = SourceFile->FileBufferPtr;
  Start = SourceFile->FileBufferPtr;
  if (((SourceFile->FileBufferPtr[0] >= UNICODE_A) && (SourceFile->FileBufferPtr[0] <= UNICODE_Z)) ||
      ((SourceFile->FileBufferPtr[0] >= UNICODE_z) && (SourceFile->FileBufferPtr[0] <= UNICODE_z))
      ) {
    //
    // Do nothing
    //
  } else {
    Error (SourceFile->FileName, SourceFile->LineNum, 0, "invalid character in string identifier name", "%S", Start);
    return 0;
  }

  while (!EndOfFile (SourceFile)) {
    if (((SourceFile->FileBufferPtr[0] >= UNICODE_A) && (SourceFile->FileBufferPtr[0] <= UNICODE_Z)) ||
        ((SourceFile->FileBufferPtr[0] >= UNICODE_z) && (SourceFile->FileBufferPtr[0] <= UNICODE_z)) ||
        ((SourceFile->FileBufferPtr[0] >= UNICODE_0) && (SourceFile->FileBufferPtr[0] <= UNICODE_9)) ||
        (SourceFile->FileBufferPtr[0] == UNICODE_UNDERSCORE)
        ) {
      Len++;
      if (Len >= StringIdentifierNameLen) {
        Error (SourceFile->FileName, SourceFile->LineNum, 0, "string identifier name too long", "%S", Start);
        return 0;
      }

      *StringIdentifierName = SourceFile->FileBufferPtr[0];
      StringIdentifierName++;
      SourceFile->FileBufferPtr++;
    } else if (SkipWhiteSpace (SourceFile) == 0) {
      Error (SourceFile->FileName, SourceFile->LineNum, 0, "invalid string identifier name", "%S", Start);
      return 0;
    } else {
      break;
    }
  }
  //
  // Terminate the copy of the string.
  //
  *StringIdentifierName = 0;
  return Len;
}

static
STATUS
GetLanguageIdentifierName (
  IN SOURCE_FILE  *SourceFile,
  IN OUT WCHAR    *LanguageIdentifierName,
  IN UINT32       LanguageIdentifierNameLen,
  IN BOOLEAN      Optional
  )
{
  UINT32  Len;
  WCHAR   *Start;
  WCHAR   *LangCode;
  WCHAR   *LanguageIdentifier;

  LanguageIdentifier = LanguageIdentifierName;

  //
  // Skip whitespace
  //
  SkipWhiteSpace (SourceFile);
  if (SourceFile->EndOfFile) {
    if (!Optional) {
      Error (SourceFile->FileName, SourceFile->LineNum, 0, "end-of-file encountered", "expected language identifier");
      return STATUS_ERROR;
    }

    return STATUS_SUCCESS;
  }
  //
  // This function is called to optionally get a language identifier name in:
  //   #string STR_ID eng "the string"
  // If it's optional, and we find a double-quote, then return now.
  //
  if (Optional) {
    if (*SourceFile->FileBufferPtr == UNICODE_DOUBLE_QUOTE) {
      return STATUS_SUCCESS;
    }
  }

  LanguageIdentifierNameLen /= 2;
  //
  // Internal error if we weren't given at least 4 WCHAR's to work with.
  //
  if (LanguageIdentifierNameLen < LANGUAGE_IDENTIFIER_NAME_LEN + 1) {
    Error (
      SourceFile->FileName,
      SourceFile->LineNum,
      0,
      "app error -- language identifier name length is invalid",
      NULL
      );
  }

  Len   = 0;
  Start = SourceFile->FileBufferPtr;
  while (!EndOfFile (SourceFile)) {
    if (((SourceFile->FileBufferPtr[0] >= UNICODE_a) && (SourceFile->FileBufferPtr[0] <= UNICODE_z)) ||
        ((SourceFile->FileBufferPtr[0] >= UNICODE_A) && (SourceFile->FileBufferPtr[0] <= UNICODE_Z)) ||
        (SourceFile->FileBufferPtr[0] == UNICODE_MINUS)) {
      Len++;
      if (Len > LANGUAGE_IDENTIFIER_NAME_LEN) {
        Error (SourceFile->FileName, SourceFile->LineNum, 0, "language identifier name too long", "%S", Start);
        return STATUS_ERROR;
      }
      *LanguageIdentifierName = SourceFile->FileBufferPtr[0];
      SourceFile->FileBufferPtr++;
      LanguageIdentifierName++;
    } else if (!IsWhiteSpace (SourceFile)) {
      Error (SourceFile->FileName, SourceFile->LineNum, 0, "invalid language identifier name", "%S", Start);
      return STATUS_ERROR;
    } else {
      break;
    }
  }
  //
  // Terminate the copy of the string.
  //
  *LanguageIdentifierName = 0;
  LangCode = GetLangCode (LanguageIdentifier);
  if (LangCode != NULL) {
    wcscpy (LanguageIdentifier, LangCode);
    FREE (LangCode);
  }
  return STATUS_SUCCESS;
}

static
void
ProcessTokenInclude (
  SOURCE_FILE *SourceFile
  )
{
  INT8        IncludeFileName[MAX_PATH];
  INT8        *To;
  UINT32      Len;
  BOOLEAN     ReportedError;
  SOURCE_FILE IncludedSourceFile;

  ReportedError = FALSE;
  if (SkipWhiteSpace (SourceFile) == 0) {
    Warning (SourceFile->FileName, SourceFile->LineNum, 0, "expected whitespace following #include keyword", NULL);
  }
  //
  // Should be quoted file name
  //
  if (SourceFile->FileBufferPtr[0] != UNICODE_DOUBLE_QUOTE) {
    Error (SourceFile->FileName, SourceFile->LineNum, 0, "expected quoted include file name", NULL);
    goto FailDone;
  }

  SourceFile->FileBufferPtr++;
  //
  // Copy the filename as ascii to our local string
  //
  To  = IncludeFileName;
  Len = 0;
  while (!EndOfFile (SourceFile)) {
    if ((SourceFile->FileBufferPtr[0] == UNICODE_CR) || (SourceFile->FileBufferPtr[0] == UNICODE_LF)) {
      Error (SourceFile->FileName, SourceFile->LineNum, 0, "end-of-line found in quoted include file name", NULL);
      goto FailDone;
    }

    if (SourceFile->FileBufferPtr[0] == UNICODE_DOUBLE_QUOTE) {
      SourceFile->FileBufferPtr++;
      break;
    }
    //
    // If too long, then report the error once and process until the closing quote
    //
    Len++;
    if (!ReportedError && (Len >= sizeof (IncludeFileName))) {
      Error (SourceFile->FileName, SourceFile->LineNum, 0, "length of include file name exceeds limit", NULL);
      ReportedError = TRUE;
    }

    if (!ReportedError) {
      *To = UNICODE_TO_ASCII (SourceFile->FileBufferPtr[0]);
      To++;
    }

    SourceFile->FileBufferPtr++;
  }

  if (!ReportedError) {
    *To = 0;
    memset ((char *) &IncludedSourceFile, 0, sizeof (SOURCE_FILE));
    strcpy (IncludedSourceFile.FileName, IncludeFileName);
    IncludedSourceFile.ControlCharacter = DEFAULT_CONTROL_CHARACTER;
    ProcessIncludeFile (&IncludedSourceFile, SourceFile);
    //
    // printf ("including file '%s'\n", IncludeFileName);
    //
  }

  return ;
FailDone:
  //
  // Error recovery -- skip to next #
  //
  SourceFile->SkipToHash = TRUE;
}

static
void
ProcessTokenScope (
  SOURCE_FILE *SourceFile
  )
{
  WCHAR StringIdentifier[MAX_STRING_IDENTIFIER_NAME + 1];
  //
  // Extract the scope name
  //
  if (GetStringIdentifierName (SourceFile, StringIdentifier, sizeof (StringIdentifier)) > 0) {
    StringDBSetScope (StringIdentifier);
  }
}

//
// Parse:  #langdef eng "English"
//         #langdef chn "\wideChinese"
//
static
void
ProcessTokenLangDef (
  SOURCE_FILE *SourceFile
  )
{
  STATUS  Status;
  WCHAR   LanguageIdentifier[MAX_STRING_IDENTIFIER_NAME + 1];
  WCHAR   *PrintableName;

  Status = GetLanguageIdentifierName (SourceFile, LanguageIdentifier, sizeof (LanguageIdentifier), FALSE);
  if (Status != STATUS_SUCCESS) {
    return;
  }

  //
  // Extract the printable name
  //
  PrintableName = GetPrintableLanguageName (SourceFile);
  if (PrintableName != NULL) {
    ParserSetPosition (SourceFile->FileName, SourceFile->LineNum);
    StringDBAddLanguage (LanguageIdentifier, PrintableName, NULL);
    FREE (PrintableName);
    return ;
  }
  //
  // Error recovery -- skip to next #
  //
  SourceFile->SkipToHash = TRUE;
}

static
VOID
ProcessTokenSecondaryLangDef (
  SOURCE_FILE *SourceFile
  )
{
  STATUS        Status;
  LANGUAGE_LIST *Lang;
  WCHAR         LanguageIdentifier[MAX_STRING_IDENTIFIER_NAME + 1];
  WCHAR         *LangCode;
  WCHAR         *SecondaryLangList = NULL;

  Status = GetLanguageIdentifierName (SourceFile, LanguageIdentifier, sizeof (LanguageIdentifier), FALSE);
  if (Status != STATUS_SUCCESS) {
    return;
  }
  LangCode = GetLangCode(LanguageIdentifier);
  if (LangCode == NULL) {
    return ;
  }

  Lang = StringDBFindLanguageList (LanguageIdentifier);
  if (Lang == NULL) {
    return;
  }

  SecondaryLangList = GetSecondaryLanguageList (SourceFile);
  if (SecondaryLangList != NULL) {
    ParserSetPosition (SourceFile->FileName, SourceFile->LineNum);
    Status = StringDBAddSecondaryLanguage (LangCode, GetLangCodeList(SecondaryLangList));
    if (Status != STATUS_SUCCESS) {
      SourceFile->SkipToHash = TRUE;
    }
    FREE (LangCode);
    FREE (SecondaryLangList);
    return ;
  }
  FREE (LangCode);


  SourceFile->SkipToHash = TRUE;
}

static
BOOLEAN
ApparentQuotedString (
  SOURCE_FILE *SourceFile
  )
{
  WCHAR *Ptr;
  //
  // See if the first and last nonblank characters on the line are double quotes
  //
  for (Ptr = SourceFile->FileBufferPtr; *Ptr && (*Ptr == UNICODE_SPACE); Ptr++)
    ;
  if (*Ptr != UNICODE_DOUBLE_QUOTE) {
    return FALSE;
  }

  while (*Ptr) {
    Ptr++;
  }

  Ptr--;
  for (; *Ptr && (*Ptr == UNICODE_SPACE); Ptr--)
    ;
  if (*Ptr != UNICODE_DOUBLE_QUOTE) {
    return FALSE;
  }

  return TRUE;
}
//
// Parse:
//   #language eng "some string " "more string"
//
static
void
ProcessTokenLanguage (
  SOURCE_FILE *SourceFile
  )
{
  STATUS  Status;
  WCHAR   *String;
  WCHAR   *SecondString;
  WCHAR   *TempString;
  WCHAR   *From;
  WCHAR   *To;
  WCHAR   Language[LANGUAGE_IDENTIFIER_NAME_LEN + 1];
  UINT32  Len;
  BOOLEAN PreviousNewline;
  //
  // Get the language identifier
  //
  Language[0] = 0;
  Status = GetLanguageIdentifierName (SourceFile, Language, sizeof (Language), TRUE);
  if (Status != STATUS_SUCCESS) {
    return;
  }

  //
  // Extract the string value. It's either a quoted string that starts on the current line, or
  // an unquoted string that starts on the following line and continues until the next control
  // character in column 1.
  // Look ahead to find a quote or a newline
  //
  if (SkipTo (SourceFile, UNICODE_DOUBLE_QUOTE, TRUE)) {
    String = GetQuotedString (SourceFile, FALSE);
    if (String != NULL) {
      //
      // Set the position in the file of where we are parsing for error
      // reporting purposes. Then start looking ahead for additional
      // quoted strings, and concatenate them until we get a failure
      // back from the string parser.
      //
      Len = wcslen (String) + 1;
      ParserSetPosition (SourceFile->FileName, SourceFile->LineNum);
      do {
        SkipWhiteSpace (SourceFile);
        SecondString = GetQuotedString (SourceFile, TRUE);
        if (SecondString != NULL) {
          Len += wcslen (SecondString);
          TempString = (WCHAR *) malloc (Len * sizeof (WCHAR));
          if (TempString == NULL) {
            Error (NULL, 0, 0, "application error", "failed to allocate memory");
            return ;
          }

          wcscpy (TempString, String);
          wcscat (TempString, SecondString);
          free (String);
          free (SecondString);
          String = TempString;
        }
      } while (SecondString != NULL);
      StringDBAddString (Language, NULL, NULL, String, TRUE, 0);
      free (String);
    } else {
      //
      // Error was reported at lower level. Error recovery mode.
      //
      SourceFile->SkipToHash = TRUE;
    }
  } else {
    if (!mGlobals.UnquotedStrings) {
      //
      // They're using unquoted strings. If the next non-blank character is a double quote, and the
      // last non-blank character on the line is a double quote, then more than likely they're using
      // quotes, so they need to put the quoted string on the end of the previous line
      //
      if (ApparentQuotedString (SourceFile)) {
        Warning (
          SourceFile->FileName,
          SourceFile->LineNum,
          0,
          "unexpected quoted string on line",
          "specify -uqs option if necessary"
          );
      }
    }
    //
    // Found end-of-line (hopefully). Skip over it and start taking in characters
    // until we find a control character at the start of a line.
    //
    Len             = 0;
    From            = SourceFile->FileBufferPtr;
    PreviousNewline = FALSE;
    while (!EndOfFile (SourceFile)) {
      if (SourceFile->FileBufferPtr[0] == UNICODE_LF) {
        PreviousNewline = TRUE;
        SourceFile->LineNum++;
      } else {
        Len++;
        if (PreviousNewline && (SourceFile->FileBufferPtr[0] == SourceFile->ControlCharacter)) {
          break;
        }

        PreviousNewline = FALSE;
      }

      SourceFile->FileBufferPtr++;
    }

    if ((Len == 0) && EndOfFile (SourceFile)) {
      Error (SourceFile->FileName, SourceFile->LineNum, 0, "unexpected end of file", NULL);
      SourceFile->SkipToHash = TRUE;
      return ;
    }
    //
    // Now allocate a buffer, copy the characters, and add the string.
    //
    String = (WCHAR *) malloc ((Len + 1) * sizeof (WCHAR));
    if (String == NULL) {
      Error (NULL, 0, 0, "application error", "failed to allocate memory");
      return ;
    }

    To = String;
    while (From < SourceFile->FileBufferPtr) {
      switch (*From) {
      case UNICODE_LF:
      case 0:
        break;

      default:
        *To = *From;
        To++;
        break;
      }

      From++;
    }

    //
    // String[Len] = 0;
    //
    *To = 0;
    StringDBAddString (Language, NULL, NULL, String, TRUE, 0);
  }
}

static
BOOLEAN
IsWhiteSpace (
  SOURCE_FILE *SourceFile
  )
{
  switch (SourceFile->FileBufferPtr[0]) {
  case UNICODE_NULL:
  case UNICODE_CR:
  case UNICODE_SPACE:
  case UNICODE_TAB:
  case UNICODE_LF:
    return TRUE;

  default:
    return FALSE;
  }
}

static
UINT32
SkipWhiteSpace (
  SOURCE_FILE *SourceFile
  )
{
  UINT32  Count;

  Count = 0;
  while (!EndOfFile (SourceFile)) {
    Count++;
    switch (*SourceFile->FileBufferPtr) {
    case UNICODE_NULL:
    case UNICODE_CR:
    case UNICODE_SPACE:
    case UNICODE_TAB:
      SourceFile->FileBufferPtr++;
      break;

    case UNICODE_LF:
      SourceFile->FileBufferPtr++;
      SourceFile->LineNum++;
      if (mGlobals.Verbose) {
        printf ("%d: %S\n", SourceFile->LineNum, SourceFile->FileBufferPtr);
      }
      break;

    default:
      return Count - 1;
    }
  }
  //
  // Some tokens require trailing whitespace. If we're at the end of the
  // file, then we count that as well.
  //
  if ((Count == 0) && (EndOfFile (SourceFile))) {
    Count++;
  }

  return Count;
}

static
UINT32
wstrcmp (
  WCHAR *Buffer,
  WCHAR *Str
  )
{
  UINT32  Len;

  Len = 0;
  while (*Str == *Buffer) {
    Buffer++;
    Str++;
    Len++;
  }

  if (*Str) {
    return 0;
  }

  return Len;
}

static
WCHAR *
wstrcatenate (
  WCHAR *Dst,
  WCHAR *Src
  )
{
  UINT32 Len  = 0;
  WCHAR  *Bak = Dst;

  if (Src == NULL) {
    return Dst;
  }

  if (Dst != NULL) {
    Len = wcslen (Dst);
  }
  Len += wcslen (Src);
  Dst = (WCHAR *) malloc ((Len + 1) * 2);
  if (Dst == NULL) {
    return NULL;
  }

  Dst[0] = L'\0';
  if (Bak != NULL) {
    wcscpy (Dst, Bak);
    FREE (Bak);
  }
  wcscat (Dst, Src);
  return Dst;
}

//
// Given a filename, try to find it along the include paths.
//
static
FILE *
FindFile (
  IN INT8    *FileName,
  OUT INT8   *FoundFileName,
  IN UINT32  FoundFileNameLen
  )
{
  FILE              *Fptr;
  TEXT_STRING_LIST  *List;

  //
  // Traverse the list of paths and try to find the file
  //
  List = mGlobals.IncludePaths;
  while (List != NULL) {
    //
    // Put the path and filename together
    //
    if (strlen (List->Str) + strlen (FileName) + 1 > FoundFileNameLen) {
      Error (UTILITY_NAME, 0, 0, NULL, "internal error - cannot concatenate path+filename");
      return NULL;
    }
    //
    // Append the filename to this include path and try to open the file.
    //
    strcpy (FoundFileName, List->Str);
    strcat (FoundFileName, FileName);
    if ((Fptr = fopen (FoundFileName, "rb")) != NULL) {
      //
      // Return the file pointer
      //
      return Fptr;
    }

    List = List->Next;
  }
  //
  // Not found
  //
  FoundFileName[0] = 0;
  return NULL;
}
//
// Process the command-line arguments
//
static
STATUS
ProcessArgs (
  int   Argc,
  char  *Argv[]
  )
{
  TEXT_STRING_LIST  *NewList;
  char              *Cptr;
  char              *Cptr2;

  //
  // Clear our globals
  //
  memset ((char *) &mGlobals, 0, sizeof (mGlobals));
  strcpy (mGlobals.BaseName, DEFAULT_BASE_NAME);
  //
  // Skip program name
  //
  Argc--;
  Argv++;

  if (Argc == 0) {
    Usage ();
    return STATUS_ERROR;
  }

  mGlobals.Mode = MODE_UNKNOWN;
  //
  // Process until no more -args.
  //
  while ((Argc > 0) && (Argv[0][0] == '-')) {
    //
    // -parse option
    //
    if (_stricmp (Argv[0], "-parse") == 0) {
      if (mGlobals.Mode != MODE_UNKNOWN) {
        Error (NULL, 0, 0, "only one of -parse/-scan/-dump allowed", NULL);
        return STATUS_ERROR;
      }

      mGlobals.Mode = MODE_PARSE;
      //
      // -scan option
      //
    } else if (_stricmp (Argv[0], "-scan") == 0) {
      if (mGlobals.Mode != MODE_UNKNOWN) {
        Error (NULL, 0, 0, "only one of -parse/-scan/-dump allowed", NULL);
        return STATUS_ERROR;
      }

      mGlobals.Mode = MODE_SCAN;
      //
      // -vscan verbose scanning option
      //
    } else if (_stricmp (Argv[0], "-vscan") == 0) {
      mGlobals.VerboseScan = TRUE;
      //
      // -dump option
      //
    } else if (_stricmp (Argv[0], "-dump") == 0) {
      if (mGlobals.Mode != MODE_UNKNOWN) {
        Error (NULL, 0, 0, "only one of -parse/-scan/-dump allowed", NULL);
        return STATUS_ERROR;
      }

      mGlobals.Mode = MODE_DUMP;
    } else if (_stricmp (Argv[0], "-uqs") == 0) {
      mGlobals.UnquotedStrings = TRUE;
      //
      // -i path    add include search path when parsing
      //
    } else if (_stricmp (Argv[0], "-i") == 0) {
      //
      // check for one more arg
      //
      if ((Argc <= 1) || (Argv[1][0] == '-')) {
        Error (UTILITY_NAME, 0, 0, Argv[0], "missing include path");
        return STATUS_ERROR;
      }
      //
      // Allocate memory for a new list element, fill it in, and
      // add it to our list of include paths. Always make sure it
      // has a "\" on the end of it.
      //
      NewList = malloc (sizeof (TEXT_STRING_LIST));
      if (NewList == NULL) {
        Error (UTILITY_NAME, 0, 0, NULL, "memory allocation failure");
        return STATUS_ERROR;
      }

      memset ((char *) NewList, 0, sizeof (TEXT_STRING_LIST));
      NewList->Str = malloc (strlen (Argv[1]) + 2);
      if (NewList->Str == NULL) {
        free (NewList);
        Error (UTILITY_NAME, 0, 0, NULL, "memory allocation failure");
        return STATUS_ERROR;
      }

      strcpy (NewList->Str, Argv[1]);
      if (NewList->Str[strlen (NewList->Str) - 1] != '\\') {
        strcat (NewList->Str, "\\");
      }
      //
      // Add it to our linked list
      //
      if (mGlobals.IncludePaths == NULL) {
        mGlobals.IncludePaths = NewList;
      } else {
        mGlobals.LastIncludePath->Next = NewList;
      }

      mGlobals.LastIncludePath = NewList;
      Argc--;
      Argv++;
    } else if (_stricmp (Argv[0], "-if") == 0) {
      //
      // Indirection file -- check for one more arg
      //
      if ((Argc <= 1) || (Argv[1][0] == '-')) {
        Error (UTILITY_NAME, 0, 0, Argv[0], "missing indirection file name");
        return STATUS_ERROR;
      }
      //
      // Allocate memory for a new list element, fill it in, and
      // add it to our list of include paths. Always make sure it
      // has a "\" on the end of it.
      //
      NewList = malloc (sizeof (TEXT_STRING_LIST));
      if (NewList == NULL) {
        Error (UTILITY_NAME, 0, 0, NULL, "memory allocation failure");
        return STATUS_ERROR;
      }

      memset ((char *) NewList, 0, sizeof (TEXT_STRING_LIST));
      NewList->Str = malloc (strlen (Argv[1]) + 1);
      if (NewList->Str == NULL) {
        free (NewList);
        Error (UTILITY_NAME, 0, 0, NULL, "memory allocation failure");
        return STATUS_ERROR;
      }

      strcpy (NewList->Str, Argv[1]);
      //
      // Add it to our linked list
      //
      if (mGlobals.IndirectionFileName == NULL) {
        mGlobals.IndirectionFileName = NewList;
      } else {
        mGlobals.LastIndirectionFileName->Next = NewList;
      }

      mGlobals.LastIndirectionFileName = NewList;
      Argc--;
      Argv++;
    } else if (_stricmp (Argv[0], "-db") == 0) {
      //
      // -db option to specify a database file.
      // Check for one more arg (the database file name)
      //
      if ((Argc <= 1) || (Argv[1][0] == '-')) {
        Error (UTILITY_NAME, 0, 0, Argv[0], "missing database file name");
        return STATUS_ERROR;
      }

      NewList = malloc (sizeof (TEXT_STRING_LIST));
      if (NewList == NULL) {
        Error (UTILITY_NAME, 0, 0, NULL, "memory allocation failure");
        return STATUS_ERROR;
      }

      memset ((char *) NewList, 0, sizeof (TEXT_STRING_LIST));
      NewList->Str = malloc (strlen (Argv[1]) + 1);
      if (NewList->Str == NULL) {
        free (NewList);
        Error (UTILITY_NAME, 0, 0, NULL, "memory allocation failure");
        return STATUS_ERROR;
      }

      strcpy (NewList->Str, Argv[1]);
      //
      // Add it to our linked list
      //
      if (mGlobals.DatabaseFileName == NULL) {
        mGlobals.DatabaseFileName = NewList;
      } else {
        mGlobals.LastDatabaseFileName->Next = NewList;
      }

      mGlobals.LastDatabaseFileName = NewList;
      Argc--;
      Argv++;
    } else if (_stricmp (Argv[0], "-ou") == 0) {
      //
      // -ou option to specify an output unicode file to
      // which we can dump our database.
      //
      if ((Argc <= 1) || (Argv[1][0] == '-')) {
        Error (UTILITY_NAME, 0, 0, Argv[0], "missing database dump output file name");
        return STATUS_ERROR;
      }

      if (mGlobals.DumpUFileName[0] == 0) {
        strcpy (mGlobals.DumpUFileName, Argv[1]);
      } else {
        Error (UTILITY_NAME, 0, 0, Argv[1], "-ou option already specified with '%s'", mGlobals.DumpUFileName);
        return STATUS_ERROR;
      }

      Argc--;
      Argv++;
    } else if (_stricmp (Argv[0], "-hpk") == 0) {
      //
      // -hpk option to create an HII export pack of the input database file
      //
      if ((Argc <= 1) || (Argv[1][0] == '-')) {
        Error (UTILITY_NAME, 0, 0, Argv[0], "missing raw string data dump output file name");
        return STATUS_ERROR;
      }

      if (mGlobals.HiiExportPackFileName[0] == 0) {
        strcpy (mGlobals.HiiExportPackFileName, Argv[1]);
      } else {
        Error (UTILITY_NAME, 0, 0, Argv[1], "-or option already specified with '%s'", mGlobals.HiiExportPackFileName);
        return STATUS_ERROR;
      }

      Argc--;
      Argv++;
    } else if ((_stricmp (Argv[0], "-?") == 0) || (_stricmp (Argv[0], "-h") == 0)) {
      Usage ();
      return STATUS_ERROR;
    } else if (_stricmp (Argv[0], "-v") == 0) {
      mGlobals.Verbose = 1;
    } else if (_stricmp (Argv[0], "-vdbw") == 0) {
      mGlobals.VerboseDatabaseWrite = 1;
    } else if (_stricmp (Argv[0], "-vdbr") == 0) {
      mGlobals.VerboseDatabaseRead = 1;
    } else if (_stricmp (Argv[0], "-newdb") == 0) {
      mGlobals.NewDatabase = 1;
    } else if (_stricmp (Argv[0], "-ignorenotfound") == 0) {
      mGlobals.IgnoreNotFound = 1;
    } else if (_stricmp (Argv[0], "-oc") == 0) {
      //
      // check for one more arg
      //
      if ((Argc <= 1) || (Argv[1][0] == '-')) {
        Error (UTILITY_NAME, 0, 0, Argv[0], "missing output C filename");
        return STATUS_ERROR;
      }

      strcpy (mGlobals.StringCFileName, Argv[1]);
      Argc--;
      Argv++;
    } else if (_stricmp (Argv[0], "-bn") == 0) {
      //
      // check for one more arg
      //
      if ((Argc <= 1) || (Argv[1][0] == '-')) {
        Error (UTILITY_NAME, 0, 0, Argv[0], "missing base name");
        Usage ();
        return STATUS_ERROR;
      }

      strcpy (mGlobals.BaseName, Argv[1]);
      Argc--;
      Argv++;
    } else if (_stricmp (Argv[0], "-oh") == 0) {
      //
      // -oh to specify output .h defines file name
      //
      if ((Argc <= 1) || (Argv[1][0] == '-')) {
        Error (UTILITY_NAME, 0, 0, Argv[0], "missing output .h filename");
        return STATUS_ERROR;
      }

      strcpy (mGlobals.StringHFileName, Argv[1]);
      Argc--;
      Argv++;
    } else if (_stricmp (Argv[0], "-dep") == 0) {
      //
      // -dep to specify output dependency file name
      //
      if ((Argc <= 1) || (Argv[1][0] == '-')) {
        Error (UTILITY_NAME, 0, 0, Argv[0], "missing output dependency filename");
        return STATUS_ERROR;
      }

      strcpy (mGlobals.OutputDependencyFileName, Argv[1]);
      Argc--;
      Argv++;
    } else if (_stricmp (Argv[0], "-skipext") == 0) {
      //
      // -skipext to skip scanning of files with certain filename extensions
      //
      if ((Argc <= 1) || (Argv[1][0] == '-')) {
        Error (UTILITY_NAME, 0, 0, Argv[0], "missing filename extension");
        return STATUS_ERROR;
      }
      //
      // Allocate memory for a new list element, fill it in, and
      // add it to our list of excluded extensions. Always make sure it
      // has a "." as the first character.
      //
      NewList = malloc (sizeof (TEXT_STRING_LIST));
      if (NewList == NULL) {
        Error (UTILITY_NAME, 0, 0, NULL, "memory allocation failure");
        return STATUS_ERROR;
      }

      memset ((char *) NewList, 0, sizeof (TEXT_STRING_LIST));
      NewList->Str = malloc (strlen (Argv[1]) + 2);
      if (NewList->Str == NULL) {
        free (NewList);
        Error (UTILITY_NAME, 0, 0, NULL, "memory allocation failure");
        return STATUS_ERROR;
      }

      if (Argv[1][0] == '.') {
        strcpy (NewList->Str, Argv[1]);
      } else {
        NewList->Str[0] = '.';
        strcpy (NewList->Str + 1, Argv[1]);
      }
      //
      // Add it to our linked list
      //
      if (mGlobals.SkipExt == NULL) {
        mGlobals.SkipExt = NewList;
      } else {
        mGlobals.LastSkipExt->Next = NewList;
      }

      mGlobals.LastSkipExt = NewList;
      Argc--;
      Argv++;
    } else if (_stricmp (Argv[0], "-lang") == 0) {
      //
      // "-lang zh-Hans" or "-lang en-US" to only output certain languages
      //
      if ((Argc <= 1) || (Argv[1][0] == '-')) {
        Error (UTILITY_NAME, 0, 0, Argv[0], "missing language name");
        Usage ();
        return STATUS_ERROR;
      }

      if (AddCommandLineLanguage (Argv[1]) != STATUS_SUCCESS) {
        return STATUS_ERROR;
      }

      Argc--;
      Argv++;
    } else if (_stricmp (Argv[0], "-od") == 0) {
      //
      // Output database file name -- check for another arg
      //
      if ((Argc <= 1) || (Argv[1][0] == '-')) {
        Error (UTILITY_NAME, 0, 0, Argv[0], "missing output database file name");
        return STATUS_ERROR;
      }

      strcpy (mGlobals.OutputDatabaseFileName, Argv[1]);
      Argv++;
      Argc--;
    } else if (_stricmp (Argv[0], "-ppflag") == 0) {
      //
      // -ppflag "Preprocess flags" -- check for another arg
      //
      if ((Argc <= 1) || (Argv[1][0] == '-')) {
        Error (UTILITY_NAME, 0, 0, Argv[0], "missing preprocess flags");
        Usage ();
        return STATUS_ERROR;
      }

      //
      // Allocate memory for a new list element, fill it in, and
      // add it to our list of preprocess flag.
      //
      NewList = malloc (sizeof (TEXT_STRING_LIST));
      if (NewList == NULL) {
        Error (UTILITY_NAME, 0, 0, NULL, "memory allocation failure");
        return STATUS_ERROR;
      }

      memset ((char *) NewList, 0, sizeof (TEXT_STRING_LIST));
      NewList->Str = malloc (strlen (Argv[1]) * 2 + 1);
      if (NewList->Str == NULL) {
        free (NewList);
        Error (UTILITY_NAME, 0, 0, NULL, "memory allocation failure");
        return STATUS_ERROR;
      }

      //
      // Convert '"' to '\"' in preprocess flag
      //
      Cptr = Argv[1];
      Cptr2 = NewList->Str;
      if (*Cptr == '"') {
        *Cptr2++ = '\\';
        *Cptr2++ = '"';
        Cptr++;
      }
      while (*Cptr != '\0') {
        if ((*Cptr == '"') && (*(Cptr - 1) != '\\')) {
          *Cptr2++ = '\\';
        }
        *Cptr2++ = *Cptr++;
      }
      *Cptr2 = '\0';

      //
      // Add it to our linked list
      //
      if (mGlobals.PreprocessFlags == NULL) {
        mGlobals.PreprocessFlags = NewList;
      } else {
        mGlobals.LastPreprocessFlags->Next = NewList;
      }
      mGlobals.LastPreprocessFlags = NewList;

      mGlobals.Preprocess = TRUE;

      Argv++;
      Argc--;
    } else {
      //
      // Unrecognized arg
      //
      Error (UTILITY_NAME, 0, 0, Argv[0], "unrecognized option");
      Usage ();
      return STATUS_ERROR;
    }

    Argv++;
    Argc--;
  }
  //
  // Make sure they specified the mode parse/scan/dump
  //
  if (mGlobals.Mode == MODE_UNKNOWN) {
    Error (NULL, 0, 0, "must specify one of -parse/-scan/-dump", NULL);
    return STATUS_ERROR;
  }
  //
  // All modes require a database filename
  //
  if (mGlobals.DatabaseFileName == 0) {
    Error (NULL, 0, 0, "must specify a database filename using -db DbFileName", NULL);
    Usage ();
    return STATUS_ERROR;
  }
  //
  // If dumping the database file, then return immediately if all
  // parameters check out.
  //
  if (mGlobals.Mode == MODE_DUMP) {
    //
    // Not much use if they didn't specify -oh or -oc or -ou or -hpk
    //
    if ((mGlobals.DumpUFileName[0] == 0) &&
        (mGlobals.StringHFileName[0] == 0) &&
        (mGlobals.StringCFileName[0] == 0) &&
        (mGlobals.HiiExportPackFileName[0] == 0)
        ) {
      Error (NULL, 0, 0, "-dump without -oc/-oh/-ou/-hpk is a NOP", NULL);
      return STATUS_ERROR;
    }

    return STATUS_SUCCESS;
  }
  //
  // Had to specify source string file and output string defines header filename.
  //
  if (mGlobals.Mode == MODE_SCAN) {
    if (Argc < 1) {
      Error (UTILITY_NAME, 0, 0, NULL, "must specify at least one source file to scan with -scan");
      Usage ();
      return STATUS_ERROR;
    }
    //
    // If -ppflag is specified, -oh should also be specified for preprocess
    //
    if (mGlobals.Preprocess && (mGlobals.StringHFileName[0] == 0)) {
      Error (UTILITY_NAME, 0, 0, NULL, "must specify string defines file name to preprocess before scan");
      Usage ();
      return STATUS_ERROR;
    }
    //
    // Get the list of filenames
    //
    while (Argc > 0) {
      NewList = malloc (sizeof (TEXT_STRING_LIST));
      if (NewList == NULL) {
        Error (UTILITY_NAME, 0, 0, "memory allocation failure", NULL);
        return STATUS_ERROR;
      }

      memset (NewList, 0, sizeof (TEXT_STRING_LIST));
      NewList->Str = (UINT8 *) malloc (strlen (Argv[0]) + 1);
      if (NewList->Str == NULL) {
        Error (UTILITY_NAME, 0, 0, "memory allocation failure", NULL);
        return STATUS_ERROR;
      }

      strcpy (NewList->Str, Argv[0]);
      if (mGlobals.ScanFileName == NULL) {
        mGlobals.ScanFileName = NewList;
      } else {
        mGlobals.LastScanFileName->Next = NewList;
      }

      mGlobals.LastScanFileName = NewList;
      Argc--;
      Argv++;
    }
  } else {
    //
    // Parse mode -- must specify an input unicode file name
    //
    if (Argc < 1) {
      Error (UTILITY_NAME, 0, 0, NULL, "must specify input unicode string file name with -parse");
      Usage ();
      return STATUS_ERROR;
    }

    strcpy (mGlobals.SourceFiles.FileName, Argv[0]);
  }

  return STATUS_SUCCESS;
}
//
// Found "-lang zh-Hans;en-US" on the command line. Parse the
// language list and save the setting for later processing.
//
static
STATUS
AddCommandLineLanguage (
  IN INT8          *Language
  )
{
  char              Separator[] = ";";
  char              *Token;
  WCHAR_STRING_LIST *WNewList;

  //
  // Keep processing the input string until we find the end.
  //
  Token = strtok (Language, Separator);
  while (Token != NULL) {
    WNewList = MALLOC (sizeof (WCHAR_STRING_LIST));
    if (WNewList == NULL) {
      Error (UTILITY_NAME, 0, 0, NULL, "memory allocation failure");
      return STATUS_ERROR;
    }
    WNewList->Next = NULL;
    WNewList->Str  = MALLOC ((strlen (Token) + 1) * sizeof (WCHAR));
    if (WNewList->Str == NULL) {
      free (WNewList);
      Error (UTILITY_NAME, 0, 0, NULL, "memory allocation failure");
      return STATUS_ERROR;
    }
#ifdef USE_VC8
    swprintf (WNewList->Str, (strlen (Token) + 1) * sizeof (WCHAR), L"%S", Token);
#else
    swprintf (WNewList->Str, L"%S", Token);
#endif

    //
    // Add it to our linked list
    //
    if (mGlobals.Language == NULL) {
      mGlobals.Language = WNewList;
    } else {
      mGlobals.LastLanguage->Next = WNewList;
    }

    mGlobals.LastLanguage = WNewList;
    Token = strtok (NULL, Separator);
  }

  return STATUS_SUCCESS;
}
//
// The contents of the text file are expected to be (one per line)
//   STRING_IDENTIFIER_NAME   ScopeName
// For example:
//   STR_ID_MY_FAVORITE_STRING   IBM
//
static
STATUS
ParseIndirectionFiles (
  TEXT_STRING_LIST    *Files
  )
{
  FILE                        *Fptr;
  INT8                        Line[200];
  INT8                        *StringName;
  INT8                        *ScopeName;
  INT8                        *End;
  UINT32                      LineCount;
  WCHAR_MATCHING_STRING_LIST  *NewList;

  Line[sizeof (Line) - 1] = 0;
  Fptr                    = NULL;
  while (Files != NULL) {
    Fptr      = fopen (Files->Str, "r");
    LineCount = 0;
    if (Fptr == NULL) {
      Error (NULL, 0, 0, Files->Str, "failed to open input indirection file for reading");
      return STATUS_ERROR;
    }

    while (fgets (Line, sizeof (Line), Fptr) != NULL) {
      //
      // remove terminating newline for error printing purposes.
      //
      if (Line[strlen (Line) - 1] == '\n') {
        Line[strlen (Line) - 1] = 0;
      }

      LineCount++;
      if (Line[sizeof (Line) - 1] != 0) {
        Error (Files->Str, LineCount, 0, "line length exceeds maximum supported", NULL);
        goto Done;
      }

      StringName = Line;
      while (*StringName && (isspace (*StringName))) {
        StringName++;
      }

      if (*StringName) {
        if ((*StringName == '_') || isalpha (*StringName)) {
          End = StringName;
          while ((*End) && (*End == '_') || (isalnum (*End))) {
            End++;
          }

          if (isspace (*End)) {
            *End = 0;
            End++;
            while (isspace (*End)) {
              End++;
            }

            if (*End) {
              ScopeName = End;
              while (*End && !isspace (*End)) {
                End++;
              }

              *End = 0;
              //
              // Add the string name/scope pair
              //
              NewList = malloc (sizeof (WCHAR_MATCHING_STRING_LIST));
              if (NewList == NULL) {
                Error (NULL, 0, 0, "memory allocation error", NULL);
                goto Done;
              }

              memset (NewList, 0, sizeof (WCHAR_MATCHING_STRING_LIST));
              NewList->Str1 = (WCHAR *) malloc ((strlen (StringName) + 1) * sizeof (WCHAR));
              NewList->Str2 = (WCHAR *) malloc ((strlen (ScopeName) + 1) * sizeof (WCHAR));
              if ((NewList->Str1 == NULL) || (NewList->Str2 == NULL)) {
                Error (NULL, 0, 0, "memory allocation error", NULL);
                goto Done;
              }

#ifdef USE_VC8
              swprintf (NewList->Str1, (strlen (StringName) + 1) * sizeof (WCHAR), L"%S", StringName);
              swprintf (NewList->Str2, (strlen (ScopeName) + 1) * sizeof (WCHAR), L"%S", ScopeName);
#else
              swprintf (NewList->Str1, L"%S", StringName);
              swprintf (NewList->Str2, L"%S", ScopeName);
#endif
              if (mGlobals.IndirectionList == NULL) {
                mGlobals.IndirectionList = NewList;
              } else {
                mGlobals.LastIndirectionList->Next = NewList;
              }

              mGlobals.LastIndirectionList = NewList;
            } else {
              Error (Files->Str, LineCount, 0, StringName, "invalid line : expected 'StringIdentifier Scope'");
              goto Done;
            }
          } else {
            Error (Files->Str, LineCount, 0, StringName, "invalid line : expected 'StringIdentifier Scope'");
            goto Done;
          }
        } else {
          Error (Files->Str, LineCount, 0, StringName, "invalid string identifier");
          goto Done;
        }
      }
    }

    fclose (Fptr);
    Fptr  = NULL;
    Files = Files->Next;
  }

Done:
  if (Fptr != NULL) {
    fclose (Fptr);
    return STATUS_ERROR;
  }

  return STATUS_SUCCESS;
}

static
INTN
PreprocessSourceFile (
  UINT8 *SourceFileName
  )
{
  char              *Cptr;
  FILE              *InFptr;
  FILE              *OutFptr;
  UINT32            CmdLen;
  char              *PreProcessCmd;
  char              BaseName[MAX_PATH];
  char              TempFileName[MAX_PATH];
  char              SourceFileDir[MAX_PATH];
  char              Line[MAX_LINE_LEN];
  TEXT_STRING_LIST  *List;
  char              InsertLine[] = "#undef STRING_TOKEN\n";
  int               Status;

  //
  // Check whehter source file exist
  //
  InFptr = fopen (SourceFileName, "r");
  if (InFptr == NULL) {
    Error (NULL, 0, 0, SourceFileName, "failed to open input file for scanning");
    return STATUS_ERROR;
  }

  //
  // Get source file directory
  //
  strcpy (SourceFileDir, SourceFileName);
  Cptr = strrchr (SourceFileDir, '\\');
  if (Cptr != NULL) {
    *Cptr = '\0';
  }

  //
  // Generate preprocess output file name
  //
  strcpy (BaseName, mGlobals.OutputDatabaseFileName);
  Cptr = strrchr (BaseName, '\\');
  if (Cptr != NULL) {
    *++Cptr = '\0';
  }

  Cptr = strrchr (SourceFileName, '\\');
  if (Cptr != NULL) {
    Cptr++;
  }
  strcat (BaseName, Cptr);

  Cptr = strrchr (BaseName, '.');
  if (Cptr != NULL) {
    *Cptr = '\0';
  }

  strcpy (mGlobals.PreprocessFileName, BaseName);
  strcat (mGlobals.PreprocessFileName, PREPROCESS_OUTPUT_FILE_EXTENSION);

  strcpy (TempFileName, BaseName);
  strcat (TempFileName, PREPROCESS_TEMP_FILE_EXTENSION);

  //
  // Insert "#undef STRING_TOKEN" after each line of "#include ...", so as to
  // preserve the STRING_TOKEN() for scanning after preprocess
  //
  OutFptr = fopen (TempFileName, "w");
  if (OutFptr == NULL) {
    Error (NULL, 0, 0, TempFileName, "failed to open file for write");
    fclose (InFptr);
    return STATUS_ERROR;
  }
  while (fgets (Line, MAX_LINE_LEN, InFptr) != NULL) {
    fputs (Line, OutFptr);
    Cptr = Line;

    //
    // Skip leading blank space
    //
    while (*Cptr == ' ' || *Cptr == '\t') {
      Cptr++;
    }

    if (*Cptr == '#' && strncmp (Cptr + 1, "include", 7) == 0){
      fputs (InsertLine, OutFptr);
    }
  }
  fclose (InFptr);
  fclose (OutFptr);

  //
  // Prepare preprocess command
  //
  CmdLen = 1;
  CmdLen += strlen (PREPROCESSOR_COMMAND);
  CmdLen++;
  CmdLen += strlen (PREPROCESSOR_OPTIONS);
  CmdLen++;

  //
  // "-I SourceFileDir "
  //
  CmdLen += strlen (SourceFileDir);
  CmdLen += 4;

  List = mGlobals.PreprocessFlags;
  while (List != NULL) {
    CmdLen += strlen (List->Str);
    CmdLen++;

    List = List->Next;
  }

  CmdLen += strlen (TempFileName);
  CmdLen += 3;
  CmdLen += strlen (mGlobals.PreprocessFileName);

  PreProcessCmd = malloc (CmdLen);
  if (PreProcessCmd == NULL) {
    Error (NULL, 0, 0, UTILITY_NAME, "memory allocation fail (%d bytes)\n", CmdLen);
    return STATUS_ERROR;
  }

  strcpy (PreProcessCmd, PREPROCESSOR_COMMAND);
  strcat (PreProcessCmd, " ");
  strcat (PreProcessCmd, PREPROCESSOR_OPTIONS);
  strcat (PreProcessCmd, " ");


  strcat (PreProcessCmd, "-I ");
  strcat (PreProcessCmd, SourceFileDir);
  strcat (PreProcessCmd, " ");

  List = mGlobals.PreprocessFlags;
  while (List != NULL) {
    strcat (PreProcessCmd, List->Str);
    strcat (PreProcessCmd, " ");

    List = List->Next;
  }

  strcat (PreProcessCmd, TempFileName);
  strcat (PreProcessCmd, " > ");
  strcat (PreProcessCmd, mGlobals.PreprocessFileName);

  //
  // Preprocess the source file
  //
  Status = system (PreProcessCmd);
  if (Status != 0) {
    Error (NULL, 0, 0, PreProcessCmd, "failed to spawn C preprocessor on source file\n");
    free (PreProcessCmd);
    return STATUS_ERROR;
  }

  free (PreProcessCmd);
  return STATUS_SUCCESS;
}

static
STATUS
ScanFiles (
  TEXT_STRING_LIST *ScanFiles
  )
{
  char              Line[MAX_LINE_LEN];
  FILE              *Fptr;
  char              *FileName;
  UINT32            LineNum;
  char              *Cptr;
  char              *SavePtr;
  char              *TermPtr;
  char              *StringTokenPos;
  TEXT_STRING_LIST  *SList;
  BOOLEAN           SkipIt;
  BOOLEAN           FileExist;

  //
  // Put a null-terminator at the end of the line. If we read in
  // a line longer than we support, then we can catch it.
  //
  Line[MAX_LINE_LEN - 1] = 0;
  //
  // Process each file. If they gave us a skip extension list, then
  // skip it if the extension matches.
  //
  FileExist = FALSE;
  while (ScanFiles != NULL) {
    SkipIt = FALSE;
    for (SList = mGlobals.SkipExt; SList != NULL; SList = SList->Next) {
      if ((strlen (ScanFiles->Str) > strlen (SList->Str)) &&
          (strcmp (ScanFiles->Str + strlen (ScanFiles->Str) - strlen (SList->Str), SList->Str) == 0)
          ) {
        SkipIt = TRUE;
        //
        // printf ("Match: %s : %s\n", ScanFiles->Str, SList->Str);
        //
        break;
      }
    }

    if (!SkipIt) {
      if (mGlobals.VerboseScan) {
        printf ("Scanning %s\n", ScanFiles->Str);
      }

      if (mGlobals.Preprocess) {
        //
        // Create an empty string defines file for preprocessor
        //
        if (!FileExist) {
          Fptr = fopen (mGlobals.StringHFileName, "w");
          if (Fptr == NULL) {
            Error (NULL, 0, 0, mGlobals.StringHFileName, "failed to open file for write");
            return STATUS_ERROR;
          }

          fclose (Fptr);
          FileExist = TRUE;
        }

        //
        // Preprocess using C preprocessor
        //
        if (PreprocessSourceFile (ScanFiles->Str) != STATUS_SUCCESS) {
          return STATUS_ERROR;
        }

        FileName = mGlobals.PreprocessFileName;
      } else {
        FileName = ScanFiles->Str;
      }

      Fptr = fopen (FileName, "r");
      if (Fptr == NULL) {
        Error (NULL, 0, 0, FileName, "failed to open input file for scanning");
        return STATUS_ERROR;
      }

      LineNum = 0;
      while (fgets (Line, sizeof (Line), Fptr) != NULL) {
        LineNum++;
        if (Line[MAX_LINE_LEN - 1] != 0) {
          Error (ScanFiles->Str, LineNum, 0, "line length exceeds maximum supported by tool", NULL);
          fclose (Fptr);
          return STATUS_ERROR;
        }
        //
        // Remove the newline from the input line so we can print a warning message
        //
        if (Line[strlen (Line) - 1] == '\n') {
          Line[strlen (Line) - 1] = 0;
        }
        //
        // Terminate the line at // comments
        //
        Cptr = strstr (Line, "//");
        if (Cptr != NULL) {
          *Cptr = 0;
        }

        Cptr = Line;
        while ((Cptr = strstr (Cptr, STRING_TOKEN)) != NULL) {
          //
          // Found "STRING_TOKEN". Make sure we don't have NUM_STRING_TOKENS or
          // something like that. Then make sure it's followed by
          // an open parenthesis, a string identifier, and then a closing
          // parenthesis.
          //
          if (mGlobals.VerboseScan) {
            printf (" %d: %s", LineNum, Cptr);
          }

          if (((Cptr == Line) || (!IsValidIdentifierChar (*(Cptr - 1), FALSE))) &&
              (!IsValidIdentifierChar (*(Cptr + sizeof (STRING_TOKEN) - 1), FALSE))
              ) {
            StringTokenPos  = Cptr;
            SavePtr         = Cptr;
            Cptr += strlen (STRING_TOKEN);
            while (*Cptr && isspace (*Cptr) && (*Cptr != '(')) {
              Cptr++;
            }

            if (*Cptr != '(') {
              Warning (ScanFiles->Str, LineNum, 0, StringTokenPos, "expected "STRING_TOKEN "(identifier)");
            } else {
              //
              // Skip over the open-parenthesis and find the next non-blank character
              //
              Cptr++;
              while (isspace (*Cptr)) {
                Cptr++;
              }

              SavePtr = Cptr;
              if ((*Cptr == '_') || isalpha (*Cptr)) {
                while ((*Cptr == '_') || (isalnum (*Cptr))) {
                  Cptr++;
                }

                TermPtr = Cptr;
                while (*Cptr && isspace (*Cptr)) {
                  Cptr++;
                }

                if (*Cptr != ')') {
                  Warning (ScanFiles->Str, LineNum, 0, StringTokenPos, "expected "STRING_TOKEN "(identifier)");
                }

                if (*TermPtr) {
                  *TermPtr  = 0;
                  Cptr      = TermPtr + 1;
                } else {
                  Cptr = TermPtr;
                }
                //
                // Add the string identifier to the list of used strings
                //
                ParserSetPosition (ScanFiles->Str, LineNum);
                StringDBSetStringReferenced (SavePtr, mGlobals.IgnoreNotFound);
                if (mGlobals.VerboseScan) {
                  printf ("...referenced %s", SavePtr);
                }
              } else {
                Warning (ScanFiles->Str, LineNum, 0, StringTokenPos, "expected valid string identifier name");
              }
            }
          } else {
            //
            // Found it, but it's a substring of something else. Advance our pointer.
            //
            Cptr++;
          }

          if (mGlobals.VerboseScan) {
            printf ("\n");
          }
        }
      }

      fclose (Fptr);
    } else {
      //
      // Skipping this file type
      //
      if (mGlobals.VerboseScan) {
        printf ("Skip scanning of %s\n", ScanFiles->Str);
      }
    }

    ScanFiles = ScanFiles->Next;
  }

  //
  // Remove the empty string defines file
  //
  if (FileExist) {
    remove (mGlobals.StringHFileName);
  }

  return STATUS_SUCCESS;
}
//
// Free the global string lists we allocated memory for
//
static
void
FreeLists (
  VOID
  )
{
  TEXT_STRING_LIST  *Temp;
  WCHAR_STRING_LIST *WTemp;

  //
  // Traverse the include paths, freeing each
  //
  while (mGlobals.IncludePaths != NULL) {
    Temp = mGlobals.IncludePaths->Next;
    free (mGlobals.IncludePaths->Str);
    free (mGlobals.IncludePaths);
    mGlobals.IncludePaths = Temp;
  }
  //
  // If we did a scan, then free up our
  // list of files to scan.
  //
  while (mGlobals.ScanFileName != NULL) {
    Temp = mGlobals.ScanFileName->Next;
    free (mGlobals.ScanFileName->Str);
    free (mGlobals.ScanFileName);
    mGlobals.ScanFileName = Temp;
  }
  //
  // Free up preprocess flags list
  //
  while (mGlobals.PreprocessFlags != NULL) {
    Temp = mGlobals.PreprocessFlags->Next;
    free (mGlobals.PreprocessFlags->Str);
    free (mGlobals.PreprocessFlags);
    mGlobals.PreprocessFlags = Temp;
  }
  //
  // If they gave us a list of filename extensions to
  // skip on scan, then free them up.
  //
  while (mGlobals.SkipExt != NULL) {
    Temp = mGlobals.SkipExt->Next;
    free (mGlobals.SkipExt->Str);
    free (mGlobals.SkipExt);
    mGlobals.SkipExt = Temp;
  }
  //
  // Free up any languages specified
  //
  while (mGlobals.Language != NULL) {
    WTemp = mGlobals.Language->Next;
    free (mGlobals.Language->Str);
    free (mGlobals.Language);
    mGlobals.Language = WTemp;
  }
  //
  // Free up our indirection list
  //
  while (mGlobals.IndirectionList != NULL) {
    mGlobals.LastIndirectionList = mGlobals.IndirectionList->Next;
    free (mGlobals.IndirectionList->Str1);
    free (mGlobals.IndirectionList->Str2);
    free (mGlobals.IndirectionList);
    mGlobals.IndirectionList = mGlobals.LastIndirectionList;
  }

  while (mGlobals.IndirectionFileName != NULL) {
    mGlobals.LastIndirectionFileName = mGlobals.IndirectionFileName->Next;
    free (mGlobals.IndirectionFileName->Str);
    free (mGlobals.IndirectionFileName);
    mGlobals.IndirectionFileName = mGlobals.LastIndirectionFileName;
  }
}

static
BOOLEAN
IsValidIdentifierChar (
  INT8      Char,
  BOOLEAN   FirstChar
  )
{
  //
  // If it's the first character of an identifier, then
  // it must be one of [A-Za-z_].
  //
  if (FirstChar) {
    if (isalpha (Char) || (Char == '_')) {
      return TRUE;
    }
  } else {
    //
    // If it's not the first character, then it can
    // be one of [A-Za-z_0-9]
    //
    if (isalnum (Char) || (Char == '_')) {
      return TRUE;
    }
  }

  return FALSE;
}

static
void
RewindFile (
  SOURCE_FILE *SourceFile
  )
{
  SourceFile->LineNum       = 1;
  SourceFile->FileBufferPtr = SourceFile->FileBuffer;
  SourceFile->EndOfFile     = FALSE;
}

static
BOOLEAN
SkipTo (
  SOURCE_FILE *SourceFile,
  WCHAR       WChar,
  BOOLEAN     StopAfterNewline
  )
{
  while (!EndOfFile (SourceFile)) {
    //
    // Check for the character of interest
    //
    if (SourceFile->FileBufferPtr[0] == WChar) {
      return TRUE;
    } else {
      if (SourceFile->FileBufferPtr[0] == UNICODE_LF) {
        SourceFile->LineNum++;
        if (StopAfterNewline) {
          SourceFile->FileBufferPtr++;
          if (SourceFile->FileBufferPtr[0] == 0) {
            SourceFile->FileBufferPtr++;
          }

          return FALSE;
        }
      }

      SourceFile->FileBufferPtr++;
    }
  }

  return FALSE;
}

static
void
Usage (
  VOID
  )
/*++

Routine Description:

  Print usage information for this utility.

Arguments:

  None.

Returns:

  Nothing.

--*/
{
  int         Index;
  const char  *Str[] = {
    UTILITY_NAME" "UTILITY_VERSION" - Intel UEFI String Gather Utility",
    "  Copyright (C), 2004 - 2008 Intel Corporation",

#if ( defined(UTILITY_BUILD) && defined(UTILITY_VENDOR) )
    "  Built from "UTILITY_BUILD", project of "UTILITY_VENDOR,
#endif
    "",
    "Usage:",
    "  "UTILITY_NAME" -parse [OPTION] FILE",
    "  "UTILITY_NAME" -scan  [OPTION] FILE",
    "  "UTILITY_NAME" -dump  [OPTION]",
    "Description:",
    "  Process unicode strings file.",
    "Common options include:",
    "  -h or -?         for this help information",
    "  -db Database     required name of output/input database file",
    "  -bn BaseName     for use in the .h and .c output files",
    "                   Default = "DEFAULT_BASE_NAME,
    "  -v               for verbose output",
    "  -vdbw            for verbose output when writing database",
    "  -vdbr            for verbose output when reading database",
    "  -od FileName     to specify an output database file name",
    "Parse options include:",
    "  -i IncludePath   add IncludePath to list of search paths",
    "  -dep FileName    to specify an output dependency file name",
    "  -newdb           to not read in existing database file",
    "  -uqs             to indicate that unquoted strings are used",
    "  FileNames        name of one or more unicode files to parse",
    "Scan options include:",
    "  -scan            scan text file(s) for STRING_TOKEN() usage",
    "  -skipext .ext    to skip scan of files with .ext filename extension",
    "  -ppflag \"Flags\"  to specify the C preprocessor flags",
    "  -oh FileName     to specify string defines file name for preprocessor",
    "  -ignorenotfound  ignore if a given STRING_TOKEN(STR) is not ",
    "                   found in the database",
    "  FileNames        one or more files to scan",
    "Dump options include:",
    "  -oc FileName     write string data to FileName",
    "  -oh FileName     write string defines to FileName",
    "  -ou FileName     dump database to unicode file FileName",
    "  -lang Lang       only dump for the language 'Lang'",
    "                   use ';' to separate multiple languages",
    "  -if FileName     to specify an indirection file",
    "  -hpk FileName    to create an HII export pack of the strings",
    "",
    "The expected process is to parse a unicode string file to create an initial",
    "database of string identifier names and string definitions. Then text files",
    "should be scanned for STRING_TOKEN() usages, and the referenced",
    "strings will be tagged as used in the database. After all files have been",
    "scanned, then the database should be dumped to create the necessary output",
    "files.",
    "",
    NULL
  };
  for (Index = 0; Str[Index] != NULL; Index++) {
    fprintf (stdout, "%s\n", Str[Index]);
  }
}
