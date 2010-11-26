/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:  

  GuidChk.c 
  
Abstract:

  Parse files in a directory and subdirectories to find all guid definitions.
  Then check them against each other to make sure there are no duplicates.
  
--*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "CommonUtils.h"
#include "FileSearch.h"
#include "UtilsMsgs.h"

#define MAX_LINE_LEN  1024 // we concatenate lines sometimes
// Define a structure that correlates filename extensions to an enumerated
// type.
//
#ifdef MAX_PATH
#undef MAX_PATH
#define MAX_PATH  1024
#endif

#define UTILITY_NAME    "GuidChk"
#define UTILITY_VERSION "v1.0"

typedef struct {
  INT8  *Extension;
  INT8  ExtensionCode;
} FILE_TYPE_TABLE_ENTRY;

#define FILE_EXTENSION_UNKNOWN  0
#define FILE_EXTENSION_C        1
#define FILE_EXTENSION_H        2
#define FILE_EXTENSION_IA32_ASM 3
#define FILE_EXTENSION_IA32_INC 4
#define FILE_EXTENSION_IA64_ASM 5
#define FILE_EXTENSION_IA64_INC 6
#define FILE_EXTENSION_PKG      7
#define FILE_EXTENSION_INF      8

FILE_TYPE_TABLE_ENTRY FileTypeTable[] = {
  ".c",
  FILE_EXTENSION_C,
  ".h",
  FILE_EXTENSION_H,
  ".inc",
  FILE_EXTENSION_IA32_INC,
  ".asm",
  FILE_EXTENSION_IA32_ASM,
  ".s",
  FILE_EXTENSION_IA64_ASM,
  ".pkg",
  FILE_EXTENSION_PKG,
  ".inf",
  FILE_EXTENSION_INF,
  ".i",
  FILE_EXTENSION_IA64_INC,
  NULL,
  0
};

typedef struct EFI_GUID {
  UINT32  Data1;
  UINT16  Data2;
  UINT16  Data3;
  UINT8   Data4[8];
} EFI_GUID;

typedef struct {
  INT8  Data[8];
  INT8  DataLen;
} EFI_SIGNATURE;

typedef struct _GUID_RECORD {
  struct _GUID_RECORD *Next;
  BOOLEAN             Reported;
  INT8                *FileName;
  INT8                *SymName;
  EFI_GUID            Guid;
} GUID_RECORD;

typedef struct _SIGNATURE_RECORD {
  struct _SIGNATURE_RECORD  *Next;
  BOOLEAN                   Reported;
  INT8                      *FileName;
  EFI_SIGNATURE             Signature;
} SIGNATURE_RECORD;

//
// Utility options
//
typedef struct {
  INT8        DatabaseOutputFileName[MAX_PATH]; // with -b option
  STRING_LIST *ExcludeDirs;                     // list of directory names not to process
  STRING_LIST *ExcludeSubDirs;                  // list of directory names to not process subdirectories (build)
  STRING_LIST *ExcludeFiles;                    // list of files to exclude (make.inf)
  STRING_LIST *ExcludeExtensions;               // list of filename extensions to exclude (.inf, .pkg)
  BOOLEAN     Verbose;
  BOOLEAN     PrintFound;
  BOOLEAN     CheckGuids;
  BOOLEAN     CheckSignatures;
  BOOLEAN     GuidXReference;
} OPTIONS;

static
STATUS
ProcessArgs (
  int     Argc,
  char    *Argv[]
  );

static
VOID
Usage (
  VOID
  );

static
STATUS
ProcessDirectory (
  INT8        *Path,
  INT8        *DirectoryName
  );

static
STATUS
ProcessFile (
  INT8                *DirectoryName,
  INT8                *FileName
  );

static
UINT32
GetFileExtension (
  INT8        *FileName
  );

static
UINT32
SkipWhiteSpace (
  INT8    *Str
  );

static
UINT32
ValidSymbolName (
  INT8    *Name
  );

static
STATUS
ProcessCFileGuids (
  INT8    *FileName
  );

static
STATUS
AddSignature (
  INT8      *FileName,
  INT8      *StrDef,
  UINT32    SigSize
  );

static
STATUS
ProcessCFileSigs (
  INT8    *FileName
  );

static
STATUS
ProcessINFFileGuids (
  INT8    *FileName
  );

static
STATUS
ProcessPkgFileGuids (
  INT8    *FileName
  );

static
STATUS
ProcessIA32FileGuids (
  INT8    *FileName
  );

static
STATUS
ProcessIA64FileGuids (
  INT8    *FileName
  );

static
BOOLEAN
IsIA64GuidLine (
  INT8      *Line,
  UINT32    *GuidHigh,
  UINT32    *GuidLow,
  BOOLEAN   *Low,
  INT8      *SymName
  );

static
STATUS
AddGuid11 (
  INT8      *FileName,
  UINT32    *Data,
  INT8      *SymName
  );

static
STATUS
AddPkgGuid (
  INT8      *FileName,
  UINT32    *Data,
  UINT64    *Data64
  );

static
STATUS
AddGuid16 (
  INT8      *FileName,
  UINT32    *Data
  );

static
STATUS
AddGuid64x2 (
  INT8      *FileName,
  UINT32    DataHH,                             // Upper 32-bits of upper 64 bits of guid
  UINT32    DataHL,                             // Lower 32-bits of upper 64 bits
  UINT32    DataLH,
  UINT32    DataLL,
  INT8      *SymName
  );

static
VOID
FreeGuids (
  VOID
  );

static
VOID
FreeSigs (
  VOID
  );

static
STATUS
CheckDuplicates (
  VOID
  );

//
// static
// VOID
// ReportGuid (
//  INT8        *FileName,
//  GUID_RECORD *FileRecord
//  );
//
static
VOID
FreeOptions (
  VOID
  );

static
BOOLEAN
CheckGuidData (
  UINT32    *GuidData,
  UINT32    DataCount
  );

static
VOID
ConcatenateLines (
  FILE        *Fptr, 
  INT8        *Line,
  UINT32      Len
  );
  
/**************************** GLOBALS ****************************************/
static GUID_RECORD      *gGuidList      = NULL;
static SIGNATURE_RECORD *gSignatureList = NULL;
static OPTIONS          gOptions;

/*****************************************************************************/
int
main (
  int     Argc,
  char    *Argv[]
  )
{
  INT8    *Cwd;
  STATUS  Status;

  SetUtilityName ("GuidChk");
  //
  // Get the current working directory and then process the command line
  // arguments.
  //
  Cwd     = _getcwd (NULL, 0);
  Status  = ProcessArgs (Argc, Argv);
  if (Status != STATUS_SUCCESS) {
    return Status;
  }

  if (gOptions.CheckGuids || gOptions.CheckSignatures) {
    Status = ProcessDirectory (Cwd, NULL);
    if (Status == STATUS_SUCCESS) {
      //
      // Check for duplicates
      //
      Status = CheckDuplicates ();
    }
  }

  if (gOptions.DatabaseOutputFileName[0] != 0) {
    CreateGuidList (gOptions.DatabaseOutputFileName);
  }
  //
  // Free up the memory
  //
  free (Cwd);
  FreeGuids ();
  FreeSigs ();
  FreeOptions ();
  return GetUtilityStatus ();
}

static
STATUS
ProcessArgs (
  int     Argc,
  char    *Argv[]
  )
{
  STRING_LIST *StrList;

  memset ((char *) &gOptions, 0, sizeof (gOptions));
  //
  // skip over program name
  //
  Argc--;
  Argv++;

  if (Argc == 0) {
    Usage ();
    return STATUS_ERROR;
  }

  while (Argc > 0) {
    //
    // Look for options
    //
    if ((Argv[0][0] == '-') || (Argv[0][0] == '/')) {
      switch (Argv[0][1]) {
      //
      // Help option
      //
      case 'h':
      case 'H':
      case '?':
        Usage ();
        return STATUS_ERROR;
        break;

      //
      // Check guids option
      //
      case 'g':
      case 'G':
        gOptions.CheckGuids = TRUE;
        break;

      //
      // Check signatures option
      //
      case 's':
      case 'S':
        gOptions.CheckSignatures = TRUE;
        break;

      //
      // Print guids found option
      //
      case 'p':
      case 'P':
        gOptions.PrintFound = TRUE;
        break;

      //
      // Exclude files option
      //
      case 'f':
      case 'F':
        //
        // Check for another arg
        //
        if (Argc < 2) {
          Error (NULL, 0, 0, Argv[0], "missing argument with option");
          Usage ();
          return STATUS_ERROR;
        }

        StrList = malloc (sizeof (STRING_LIST));
        if (StrList == NULL) {
          Error (NULL, 0, 0, "memory allocation failure", NULL);
          return STATUS_ERROR;
        }

        memset ((char *) StrList, 0, sizeof (STRING_LIST));
        StrList->Str          = Argv[1];
        StrList->Next         = gOptions.ExcludeFiles;
        gOptions.ExcludeFiles = StrList;
        Argc--;
        Argv++;
        break;

      //
      // Exclude directories option
      //
      case 'd':
      case 'D':
        //
        // Check for another arg
        //
        if (Argc < 2) {
          Error (NULL, 0, 0, Argv[0], "missing argument with option");
          Usage ();
          return STATUS_ERROR;
        }

        StrList = malloc (sizeof (STRING_LIST));
        if (StrList == NULL) {
          Error (NULL, 0, 0, "memory allocation failure", NULL);
          return STATUS_ERROR;
        }

        memset ((char *) StrList, 0, sizeof (STRING_LIST));
        StrList->Str          = Argv[1];
        StrList->Next         = gOptions.ExcludeDirs;
        gOptions.ExcludeDirs  = StrList;
        Argc--;
        Argv++;
        break;

      //
      // -u  exclude all subdirectories of a given directory option
      //
      case 'u':
      case 'U':
        //
        // Check for another arg
        //
        if (Argc < 2) {
          Error (NULL, 0, 0, Argv[0], "missing argument with option");
          Usage ();
          return STATUS_ERROR;
        }

        StrList = malloc (sizeof (STRING_LIST));
        if (StrList == NULL) {
          Error (NULL, 0, 0, "memory allocation failure", NULL);
          return STATUS_ERROR;
        }

        memset ((char *) StrList, 0, sizeof (STRING_LIST));
        StrList->Str            = Argv[1];
        StrList->Next           = gOptions.ExcludeSubDirs;
        gOptions.ExcludeSubDirs = StrList;
        Argc--;
        Argv++;
        break;

      //
      // -e  exclude by filename extension option
      //
      case 'e':
      case 'E':
        //
        // Check for another arg
        //
        if (Argc < 2) {
          Error (NULL, 0, 0, Argv[0], "missing argument with option");
          Usage ();
          return STATUS_ERROR;
        }

        StrList = malloc (sizeof (STRING_LIST));
        if (StrList == NULL) {
          Error (NULL, 0, 0, "memory allocation failure", NULL);
          return STATUS_ERROR;
        }

        memset ((char *) StrList, 0, sizeof (STRING_LIST));
        //
        // Let them put a * in front of the filename extension
        //
        StrList->Str = Argv[1];
        if (StrList->Str[0] == '*') {
          StrList->Str++;
        }

        StrList->Next               = gOptions.ExcludeExtensions;
        gOptions.ExcludeExtensions  = StrList;
        Argc--;
        Argv++;
        break;

      //
      // Print guid with matching symbol name for guid definitions found
      //
      case 'x':
      case 'X':
        gOptions.GuidXReference = TRUE;
        break;

      //
      // -b   Print the internal database list to a file
      //
      case 'b':
      case 'B':
        //
        // Check for one more arg
        //
        if (Argc < 2) {
          Error (NULL, 0, 0, Argv[0], "must specify file name with option");
          Usage ();
          return STATUS_ERROR;
        }

        strcpy (gOptions.DatabaseOutputFileName, Argv[1]);
        Argc--;
        Argv++;
        break;

      default:
        Error (NULL, 0, 0, Argv[0], "invalid option");
        Usage ();
        return STATUS_ERROR;
      }
    } else {
      break;
    }
    //
    // Next arg
    //
    Argc--;
    Argv++;
  }

  if (Argc > 0) {
    Error (NULL, 0, 0, Argv[0], "invalid argument");
    Usage ();
    return STATUS_ERROR;
  }
  //
  // Have to check signatures, GUIDs, or dump the GUID database.
  //
  if ((!gOptions.CheckGuids) && (!gOptions.CheckSignatures) && (gOptions.DatabaseOutputFileName[0] == 0)) {
    Error (NULL, 0, 0, "nothing to do", "must specify -g, -s, and/or -b");
    Usage ();
    return STATUS_ERROR;
  }

  return STATUS_SUCCESS;
}
//
// Print usage instructions
//
static
VOID
Usage (
  VOID
  )
{
  int         Index;
  const char  *Str[] = {
    UTILITY_NAME" "UTILITY_VERSION" - Intel GUID Check Utility",
    "  Copyright (C), 2004 - 2008 Intel Corporation",
    
#if ( defined(UTILITY_BUILD) && defined(UTILITY_VENDOR) )
    "  Built from "UTILITY_BUILD", project of "UTILITY_VENDOR,
#endif
    "",
    "Usage:",
    "  "UTILITY_NAME" [OPTION]...",
    "Description:",
    "  Scan files for duplicate GUID or signature definitions.",
    "Options:",
    "  -d dirname     exclude searching of a directory",
    "  -f filename    exclude searching of a file",
    "  -e extension   exclude searching of files by extension",
    "  -p             print all GUIDS found",
    "  -g             check for duplicate guids",
    "  -s             check for duplicate signatures",
    "  -x             print guid+defined symbol name",
    "  -b outfile     write internal GUID+basename list to outfile",
    "  -u dirname     exclude searching all subdirectories of a directory",
    "  -h -?          print this help text",
    "Example Usage:",
    "  GuidChk -g -u build -d fv -f make.inf -e .pkg",
    NULL
  };
  for (Index = 0; Str[Index] != NULL; Index++) {
    fprintf (stdout, "%s\n", Str[Index]);
  }
}
//
// Process an entire directory by name
//
static
STATUS
ProcessDirectory (
  INT8          *Path,
  INT8          *DirectoryName
  )
{
  FILE_SEARCH_DATA  FSData;
  char              *FileMask;
  BOOLEAN           Done;
  UINT32            Len;
  BOOLEAN           NoSubdirs;
  STRING_LIST       *SLPtr;

  //
  // Root directory may be null
  //
  if (DirectoryName != NULL) {
    //
    // printf ("Processing directory: %s\n", DirectoryName);
    //
  }
  //
  // Initialize our file searching
  //
  FileSearchInit (&FSData);

  //
  // Exclude some directories, files, and extensions
  //
  FileSearchExcludeDirs (&FSData, gOptions.ExcludeDirs);
  FileSearchExcludeExtensions (&FSData, gOptions.ExcludeExtensions);
  FileSearchExcludeFiles (&FSData, gOptions.ExcludeFiles);
  //
  // See if this directory is in the list of directories that they
  // don't want to process subdirectories of
  //
  NoSubdirs = FALSE;
  if (DirectoryName != NULL) {
    for (SLPtr = gOptions.ExcludeSubDirs; SLPtr != NULL; SLPtr = SLPtr->Next) {
      if (_stricmp (SLPtr->Str, DirectoryName) == 0) {
        //
        // printf ("not processing subdirectories of %s\n", DirectoryName);
        //
        NoSubdirs = TRUE;
        break;
      }
    }
  }
  //
  // Create a filemask of files to search for. We'll append "\*.*" on the
  // end, so allocate some extra bytes.
  //
  Len = strlen (Path) + 10;
  if (DirectoryName != NULL) {
    Len += strlen (DirectoryName);
  }

  FileMask = malloc (Len);
  if (FileMask == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    return STATUS_ERROR;
  }
  //
  // Now put it all together
  //
  strcpy (FileMask, Path);
  if ((DirectoryName != NULL) && (strlen (DirectoryName) > 0)) {
    strcat (FileMask, "\\");
    strcat (FileMask, DirectoryName);
  }

  strcat (FileMask, "\\*.*");

  //
  // Start file searching for files and directories
  //
  if (FileSearchStart (&FSData, FileMask, FILE_SEARCH_FILE | FILE_SEARCH_DIR) == STATUS_SUCCESS) {
    Done = FALSE;
  } else {
    Done = TRUE;
  }

  //
  // Now hack the "\*.*" off the end of the filemask so we can use it to pass
  // the full directory path on recursive calls to process directories.
  //
  FileMask[strlen (FileMask) - 4] = 0;

  //
  // Loop until no more files
  //
  while (!Done) {
    //
    // printf ("Found %s...", FSData.FileName);
    //
    if (FSData.FileFlags & FILE_SEARCH_DIR) {
      //
      // printf ("directory\n");
      //
      if (!NoSubdirs) {
        ProcessDirectory (FileMask, FSData.FileName);
      }
    } else if (FSData.FileFlags & FILE_SEARCH_FILE) {
      //
      // printf ("file\n");
      //
      ProcessFile (FileMask, FSData.FileName);
    } else {
      //
      // printf ("unknown\n");
      //
    }

    if (FileSearchFindNext (&FSData) != STATUS_SUCCESS) {
      Done = TRUE;
    }
  }
  //
  // Free up allocated memory
  //
  free (FileMask);

  //
  // Free up our file searching
  //
  FileSearchDestroy (&FSData);

  return STATUS_SUCCESS;
}
//
// Process a single file.
//
static
STATUS
ProcessFile (
  INT8                *DirectoryName,
  INT8                *FileName
  )
{
  STATUS  Status;
  UINT32  FileExtension;
  INT8    FullFileName[MAX_PATH];

  Status = STATUS_SUCCESS;

  sprintf (FullFileName, "%s\\%s", DirectoryName, FileName);
  //
  // printf ("Found file: %s\n", FullFileName);
  //
  FileExtension = GetFileExtension (FileName);

  //
  // Process these for GUID checks
  //
  if (gOptions.CheckGuids) {
    switch (FileExtension) {
    case FILE_EXTENSION_C:
    case FILE_EXTENSION_H:
      Status = ProcessCFileGuids (FullFileName);
      break;

    case FILE_EXTENSION_PKG:
      Status = ProcessPkgFileGuids (FullFileName);
      break;

    case FILE_EXTENSION_IA32_INC:
    case FILE_EXTENSION_IA32_ASM:
      Status = ProcessIA32FileGuids (FullFileName);
      break;

    case FILE_EXTENSION_INF:
      Status = ProcessINFFileGuids (FullFileName);
      break;

    case FILE_EXTENSION_IA64_INC:
    case FILE_EXTENSION_IA64_ASM:
      Status = ProcessIA64FileGuids (FullFileName);
      break;

    default:
      //
      // No errors anyway
      //
      Status = STATUS_SUCCESS;
      break;
    }
  }

  if (gOptions.CheckSignatures) {
    switch (FileExtension) {
    case FILE_EXTENSION_C:
    case FILE_EXTENSION_H:
      Status = ProcessCFileSigs (FullFileName);
      break;

    default:
      //
      // No errors anyway
      //
      Status = STATUS_SUCCESS;
      break;
    }
  }

  return Status;
}
//
// Return a code indicating the file name extension.
//
static
UINT32
GetFileExtension (
  INT8        *FileName
  )
{
  INT8  *Extension;
  int   Index;

  //
  // Look back for a filename extension
  //
  for (Extension = FileName + strlen (FileName) - 1; Extension >= FileName; Extension--) {
    if (*Extension == '.') {
      for (Index = 0; FileTypeTable[Index].Extension != NULL; Index++) {
        if (_stricmp (FileTypeTable[Index].Extension, Extension) == 0) {
          return FileTypeTable[Index].ExtensionCode;
        }
      }
    }
  }

  return FILE_TYPE_UNKNOWN;
}
//
// Process a .pkg file.
//
// Look for FFS_FILEGUID=35b898ca-b6a9-49ce-8c72-904735cc49b7
//
static
STATUS
ProcessPkgFileGuids (
  INT8    *FileName
  )
{
  FILE    *Fptr;
  INT8    Line[MAX_LINE_LEN * 2];
  INT8    *Cptr;
  INT8    *Cptr2;
  UINT32  GuidScan[11];
  UINT64  Guid64;

  if ((Fptr = fopen (FileName, "r")) == NULL) {
    Error (NULL, 0, 0, FileName, "could not open input file for reading");
    return STATUS_ERROR;
  }
  //
  // Read lines from the file until done
  //
  while (fgets (Line, sizeof (Line), Fptr) != NULL) {
    Cptr = Line;
    Cptr += SkipWhiteSpace (Line);
    if (strncmp (Cptr, "FFS_FILEGUID", 12) == 0) {
      Cptr += 12;
      Cptr += SkipWhiteSpace (Cptr);
      if (*Cptr == '=') {
        Cptr++;
        Cptr += SkipWhiteSpace (Cptr + 1);
        //
        // Blank out dashes on the line.
        //
        for (Cptr2 = Cptr; *Cptr2; Cptr2++) {
          if (*Cptr2 == '-') {
            *Cptr2 = ' ';
          }
        }

        if (sscanf (
              Cptr,
              "%X %X %X %X %I64X",
              &GuidScan[0],
              &GuidScan[1],
              &GuidScan[2],
              &GuidScan[3],
              &Guid64
              ) == 5) {
          AddPkgGuid (FileName, GuidScan, &Guid64);
        } else {
          DebugMsg (NULL, 0, 0, FileName, "GUID scan failed");
        }
      }
    }
  }

  fclose (Fptr);
  return STATUS_SUCCESS;
}
//
// Process an IA32 assembly file.
//
// Look for:
// FIND_FD_GUID_VAL equ  01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h
// PEI_GUID_FileNameGuid_Gmch815  equ  081818181h, 08181h, 08181h, 081h, 081h, 081h, 081h, 081h, 081h, 081h, 081h
//
static
STATUS
ProcessIA32FileGuids (
  INT8    *FileName
  )
{
  FILE    *Fptr;
  INT8    Line[MAX_LINE_LEN];
  INT8    *Cptr;
  INT8    CSave;
  INT8    *CSavePtr;
  UINT32  Len;
  UINT32  GuidData[16];
  UINT32  Index;

  if ((Fptr = fopen (FileName, "r")) == NULL) {
    Error (NULL, 0, 0, FileName, "could not open input file for reading");
    return STATUS_ERROR;
  }
  //
  // Read lines from the file until done
  //
  while (fgets (Line, sizeof (Line), Fptr) != NULL) {
    Cptr = Line;
    Cptr += SkipWhiteSpace (Line);
    //
    // Look for xxxGUIDyyy equ 01h, 02h, 03h, ...
    //
    Len = ValidSymbolName (Cptr);
    if (Len) {
      //
      // Terminate the line after the symbol name, then look for "guid" in
      // the name.
      //
      CSavePtr  = Cptr + Len;
      CSave     = *CSavePtr;
      *CSavePtr = 0;
      while (*Cptr) {
        if (_strnicmp (Cptr, "guid", 4) == 0) {
          break;
        }

        Cptr++;
      }
      //
      // If we found the string "guid", continue
      //
      if (*Cptr) {
        //
        // Restore the character on the line where we null-terminated the symbol
        //
        *CSavePtr = CSave;
        Cptr      = CSavePtr;
        Len       = SkipWhiteSpace (Cptr);
        //
        // Had to be some white space
        //
        if (Len) {
          Cptr += Len;
          //
          // now look for "equ"
          //
          if (_strnicmp (Cptr, "equ", 3) == 0) {
            Cptr += 3;
            Cptr += SkipWhiteSpace (Cptr);
            //
            // Now scan all the data
            //
            for (Index = 0; Index < 16; Index++) {
              if (sscanf (Cptr, "%X", &GuidData[Index]) != 1) {
                break;
              }
              //
              // Skip to next
              //
              while (isxdigit (*Cptr)) {
                Cptr++;
              }

              if ((*Cptr != 'h') && (*Cptr != 'H')) {
                break;
              } else {
                Cptr++;
                while (*Cptr && (isspace (*Cptr) || (*Cptr == ','))) {
                  Cptr++;
                }
              }
            }
            //
            // Now see which form we had
            //
            if (Index == 16) {
              AddGuid16 (FileName, GuidData);
            } else if (Index == 11) {
              AddGuid11 (FileName, GuidData, NULL);
            }
          }
        }
      }
    }
  }

  fclose (Fptr);
  return STATUS_SUCCESS;
}
//
// Found and parsed an IA32 assembly code guid. Save the 16 bytes off in the list
// of guids.
//
static
STATUS
AddGuid16 (
  INT8      *FileName,
  UINT32    *Data
  )
{
  GUID_RECORD *NewRec;
  int         Index;

  //
  // Sanity check the data
  //
  if (!CheckGuidData (Data, 16)) {
    return STATUS_ERROR;
  }
  //
  // Allocate memory for a new guid structure
  //
  NewRec = malloc (sizeof (GUID_RECORD));
  if (NewRec == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    return STATUS_ERROR;
  }

  memset ((char *) NewRec, 0, sizeof (GUID_RECORD));
  NewRec->FileName = malloc (strlen (FileName) + 1);
  if (NewRec->FileName == NULL) {
    free (NewRec);
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    return STATUS_ERROR;
  }

  strcpy (NewRec->FileName, FileName);
  NewRec->Guid.Data1  = (UINT32) (Data[0] | (Data[1] << 8) | (Data[2] << 16) | (Data[3] << 24));
  NewRec->Guid.Data2  = (UINT16) (Data[4] | (Data[5] << 8));
  NewRec->Guid.Data3  = (UINT16) (Data[6] | (Data[7] << 8));
  for (Index = 0; Index < 8; Index++) {
    NewRec->Guid.Data4[Index] = (UINT8) Data[Index + 8];
  }
  //
  // Add it to the list
  //
  NewRec->Next  = gGuidList;
  gGuidList     = NewRec;

  //
  // Report it
  // ReportGuid (FileName, NewRec);
  //
  return STATUS_SUCCESS;
}
//
// Add a GUID defined as GuidLow: 0x1122334455667788
//                       GuidHi:  0x99AABBCCDDEEFF00
//
// These are equivalent:
// { 0x11223344, 0x5566, 0x7788, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00 }
//    and:
// Low: 00FFEEDDCCBBAA99
// Hi:  7788556611223344
//
static
STATUS
AddGuid64x2 (
  INT8      *FileName,
  UINT32    DataHH, // Upper 32-bits of upper 64 bits of guid
  UINT32    DataHL, // Lower 32-bits of upper 64 bits
  UINT32    DataLH,
  UINT32    DataLL,
  INT8      *SymName
  )
{
  GUID_RECORD *NewRec;
  int         Index;

  //
  // Allocate memory for a new guid structure
  //
  NewRec = malloc (sizeof (GUID_RECORD));
  if (NewRec == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    return STATUS_ERROR;
  }

  memset ((char *) NewRec, 0, sizeof (GUID_RECORD));
  NewRec->FileName = malloc (strlen (FileName) + 1);
  if (NewRec->FileName == NULL) {
    free (NewRec);
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    return STATUS_ERROR;
  }

  strcpy (NewRec->FileName, FileName);
  NewRec->Guid.Data1  = DataHL;
  NewRec->Guid.Data2  = (UINT16) DataHH;
  NewRec->Guid.Data3  = (UINT16) (DataHH >> 16);
  for (Index = 0; Index < 4; Index++) {
    NewRec->Guid.Data4[Index] = (UINT8) DataLL;
    DataLL >>= 8;
  }

  for (Index = 0; Index < 4; Index++) {
    NewRec->Guid.Data4[Index + 4] = (UINT8) DataLH;
    DataLH >>= 8;
  }

  if (SymName != NULL) {
    NewRec->SymName = malloc (strlen (SymName) + 1);
    if (NewRec->SymName == NULL) {
      free (NewRec);
      Error (NULL, 0, 0, "memory allocation failure", NULL);
      return STATUS_ERROR;
    }
    strcpy (NewRec->SymName, SymName);
  }
    
  //
  // Add it to the list
  //
  NewRec->Next  = gGuidList;
  gGuidList     = NewRec;

  //
  // Report it
  // ReportGuid (FileName, NewRec);
  //
  return STATUS_SUCCESS;
}
//
// Process INF files. Look for:
// FILE_GUID            = 240612B6-A063-11d4-9A3A-0090273FC14D
//
static
STATUS
ProcessINFFileGuids (
  INT8    *FileName
  )
{
  FILE    *Fptr;
  INT8    Line[MAX_LINE_LEN * 2];
  INT8    *Cptr;
  INT8    *Cptr2;
  UINT32  GuidScan[11];
  UINT64  Guid64;

  if ((Fptr = fopen (FileName, "r")) == NULL) {
    Error (NULL, 0, 0, FileName, "could not open input file for reading");
    return STATUS_ERROR;
  }
  //
  // Read lines from the file until done
  //
  while (fgets (Line, sizeof (Line), Fptr) != NULL) {
    Cptr = Line;
    Cptr += SkipWhiteSpace (Line);
    if (strncmp (Cptr, "FILE_GUID", 9) == 0) {
      Cptr += 9;
      Cptr += SkipWhiteSpace (Cptr);
      if (*Cptr == '=') {
        Cptr++;
        Cptr += SkipWhiteSpace (Cptr + 1);
        //
        // Blank out dashes on the line.
        //
        for (Cptr2 = Cptr; *Cptr2; Cptr2++) {
          if (*Cptr2 == '-') {
            *Cptr2 = ' ';
          }
        }

        if (sscanf (
              Cptr,
              "%X %X %X %X %I64X",
              &GuidScan[0],
              &GuidScan[1],
              &GuidScan[2],
              &GuidScan[3],
              &Guid64
              ) == 5) {
          AddPkgGuid (FileName, GuidScan, &Guid64);
        } else {
          DebugMsg (NULL, 0, 0, FileName, "GUID scan failed");
        }
      }
    }
  }

  fclose (Fptr);
  return STATUS_SUCCESS;
}
//
// Parse ('g','m','a','p','a','b','c','d')
//
static
STATUS
AddSignature (
  INT8      *FileName,
  INT8      *StrDef,
  UINT32    SigSize
  )
{
  SIGNATURE_RECORD  *NewRec;
  INT8              *Cptr;
  UINT32            Index;
  BOOLEAN           Fail;

  //
  // Allocate memory for the new record
  //
  Fail    = FALSE;
  NewRec  = malloc (sizeof (SIGNATURE_RECORD));
  
  if (NewRec == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    return STATUS_ERROR;
  }
  memset ((char *) NewRec, 0, sizeof (SIGNATURE_RECORD));
  
  //
  // Allocate memory to save the file name
  //
  NewRec->FileName = malloc (strlen (FileName) + 1);
  if (NewRec->FileName == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    free (NewRec);
    return STATUS_ERROR;
  }
  //
  // Fill in the fields
  //
  strcpy (NewRec->FileName, FileName);
  NewRec->Signature.DataLen = (UINT8) SigSize;
  //
  // Skip to open parenthesis
  //
  Cptr = StrDef;
  Cptr += SkipWhiteSpace (Cptr);
  if (*Cptr != '(') {
    Fail = TRUE;
    goto Done;
  }

  Cptr++;
  //
  // Skip to first ' and start processing
  //
  while (*Cptr && (*Cptr != '\'')) {
    Cptr++;
  }

  for (Index = 0; Index < SigSize; Index++) {
    if (*Cptr == '\'') {
      Cptr++;
      NewRec->Signature.Data[Index] = (INT8) *Cptr;
      //
      // Skip to closing quote
      //
      Cptr++;
      if (*Cptr != '\'') {
        Fail = TRUE;
        break;
      }
      //
      // Skip over closing quote, go to next one
      //
      Cptr++;
      while (*Cptr && (*Cptr != '\'')) {
        Cptr++;
      }
    } else {
      Fail = TRUE;
      DebugMsg (NULL, 0, 0, FileName, "failed to parse signature");
      break;
    }
  }

Done:
  if (Fail) {
    free (NewRec->FileName);
    free (NewRec);
    return STATUS_ERROR;
  }

  NewRec->Next    = gSignatureList;
  gSignatureList  = NewRec;
  return STATUS_SUCCESS;
}
//
// Look for:
// #define POOL_HEAD_SIGNATURE         EFI_SIGNATURE_16('p','h')
// #define GCD_MEMORY_MAP_SIGNATURE    EFI_SIGNATURE_32('g','m','a','p')
// #define GCD_MEMORY_MAP_SIGNATURE    EFI_SIGNATURE_64('g','m','a','p','a','b','c','d')
//
static
STATUS
ProcessCFileSigs (
  INT8    *FileName
  )
{
  FILE    *Fptr;
  INT8    Line[MAX_LINE_LEN * 2];
  INT8    *Cptr;
  UINT32  Len;

  if ((Fptr = fopen (FileName, "r")) == NULL) {
    Error (NULL, 0, 0, FileName, "could not open input file for reading");
    return STATUS_ERROR;
  }
  //
  // Read lines from the file until done
  //
  while (fgets (Line, sizeof (Line), Fptr) != NULL) {
    Cptr = Line;
    Cptr += SkipWhiteSpace (Line);
    //
    // look for #define EFI_SIGNATURE_xx value
    //
    if (*Cptr == '#') {
      Cptr++;
      Cptr += SkipWhiteSpace (Cptr);
      //
      // Look for "define"
      //
      if (!strncmp (Cptr, "define", 6)) {
        Cptr += 6;
        //
        // Better be whitespace
        //
        Len = SkipWhiteSpace (Cptr);
        if (Len) {
          Cptr += Len;
          //
          // See if it's a valid symbol name
          //
          Len = ValidSymbolName (Cptr);
          if (Len) {
            //
            // It is a valid symbol name. See if there's line continuation,
            // and if so, read more lines.
            // Skip over the symbol name and look for the string "EFI_SIGNATURE_xx"
            //
            ConcatenateLines (Fptr, Line, sizeof(Line));

            Cptr += Len;
            Cptr += SkipWhiteSpace (Cptr);
            if (strncmp (Cptr, "EFI_SIGNATURE_16", 16) == 0) {
              AddSignature (FileName, Cptr + 16, 2);
            } else if (strncmp (Cptr, "EFI_SIGNATURE_32", 16) == 0) {
              AddSignature (FileName, Cptr + 16, 4);
            } else if (strncmp (Cptr, "EFI_SIGNATURE_64", 16) == 0) {
              AddSignature (FileName, Cptr + 16, 8);
            }
          }
        }
      }
    }
  }

  fclose (Fptr);
  return STATUS_SUCCESS;
}
//
// look for #define xxxGUIDyyy { 0x...}
// xxx EFI_GUID  GuidName = { 0x... };
//
static
STATUS
ProcessCFileGuids (
  INT8    *FileName
  )
{
  FILE    *Fptr;
  INT8    Line[MAX_LINE_LEN * 2];
  INT8    *Cptr;
  INT8    *CSavePtr;
  INT8    *TempCptr;
  INT8    *SymName;
  UINT32  Len;
  UINT32  LineLen;
  UINT32  GuidScan[11];

  if ((Fptr = fopen (FileName, "r")) == NULL) {
    Error (NULL, 0, 0, FileName, "could not open input file for reading");
    return STATUS_ERROR;
  }
  //
  // Read lines from the file until done
  //
  while (fgets (Line, sizeof (Line), Fptr) != NULL) {
    Cptr = Line;
    Cptr += SkipWhiteSpace (Line);
    //
    // look for #define xxxGUIDxxx value
    //
    if (*Cptr == '#') {
      Cptr++;
      Cptr += SkipWhiteSpace (Cptr);
      //
      // Look for "define"
      //
      if (!strncmp (Cptr, "define", 6)) {
DefineLine:
        Cptr += 6;
        //
        // Better be whitespace
        //
        Len = SkipWhiteSpace (Cptr);
        if (Len) {
          Cptr += Len;
          //
          // See if it's a valid symbol name
          //
          Len = ValidSymbolName (Cptr);
          if (Len) {
            //
            // It is a valid symbol name. See if there's line continuation,
            // and if so, read more lines.
            // Then truncate after the symbol name, look for the string "GUID",
            // and continue.
            //
            SymName = Cptr;
            ConcatenateLines (Fptr, Line, sizeof(Line));

            //
            // Now look for { 0x....... }
            //
            CSavePtr  = Cptr + Len;
            Cptr += Len;
            Cptr += SkipWhiteSpace (Cptr);
            if (*Cptr == '{') {
              Cptr++;
              //
              // Blank out 'L', 'l', '{', '}', ',' on the line.
              //
              for (TempCptr = Cptr; *TempCptr; TempCptr++) {
                if ((*TempCptr == 'L') || (*TempCptr == 'l') || (*TempCptr == '{') || 
                    (*TempCptr == '}') || (*TempCptr == ',')) {
                  *TempCptr       = ' ';
                }
              }

              if (sscanf (
                    Cptr,
                    "%X %X %X %X %X %X %X %X %X %X %X",
                    &GuidScan[0],
                    &GuidScan[1],
                    &GuidScan[2],
                    &GuidScan[3],
                    &GuidScan[4],
                    &GuidScan[5],
                    &GuidScan[6],
                    &GuidScan[7],
                    &GuidScan[8],
                    &GuidScan[9],
                    &GuidScan[10]
                    ) == 11) {
                *CSavePtr = '\0';
                AddGuid11 (FileName, GuidScan, SymName);
              }
            }
          }
        }
      }
    //
    // Else look for "static EFI_GUID xxxGUIDxxx = { 0x.... };
    //
    } else if ((CSavePtr = strstr (Line, "EFI_GUID")) != NULL) {
      //
      // Read more lines until met ';'
      //
      ConcatenateLines (Fptr, Line, sizeof(Line));
      while (strstr (Line, ";") == NULL) {
        LineLen = strlen (Line);
        Len = sizeof(Line) - LineLen;
        if (Len <= 1) {
          break;
        }
        if (Line[LineLen - 1] == '\n') {
          Cptr = Line + LineLen - 1;
          *Cptr = '\0';
          if (fgets (Cptr, Len, Fptr) == NULL){
            break;
          }
          ConcatenateLines (Fptr, Line, sizeof(Line));
        } else {
          Cptr = Line + LineLen;
          *Cptr = '\0';
          if (fgets (Cptr, Len, Fptr) == NULL) {
            break;
          }
          ConcatenateLines (Fptr, Line, sizeof(Line));
        }
        
        //
        // EFI_GUID may appear in comments wihout end of ';' which may cause
        // ignoring of new #define, so handle it here.
        //
        Cptr += SkipWhiteSpace (Cptr);
        if (*Cptr == '#') {
          Cptr++;
          Cptr += SkipWhiteSpace (Cptr);
          if (!strncmp (Cptr, "define", 6)) {
            goto DefineLine;
          }
        }
      }

      Cptr = CSavePtr + 8;
      Cptr += SkipWhiteSpace (Cptr);
      //
      // Should be variable name next
      //
      Len     = ValidSymbolName (Cptr);
      SymName = Cptr;
      Cptr += Len;
      CSavePtr = Cptr;
      Cptr += SkipWhiteSpace (Cptr);
      if (*Cptr == '=') {
        *CSavePtr = '\0';
        Cptr++;
        Cptr += SkipWhiteSpace (Cptr);
        //
        // Should be open-brace next to define guid
        //
        if (*Cptr == '{') {
          Cptr++;
          //
          // Blank out 'L', 'l', '{', '}', ',' on the line.
          //
          for (TempCptr = Cptr; *TempCptr; TempCptr++) {
            if ((*TempCptr == 'L') || (*TempCptr == 'l') || (*TempCptr == '{') || 
                (*TempCptr == '}') || (*TempCptr == ',')) {
              *TempCptr       = ' ';
            }
          }

          if (sscanf (
                Cptr,
                "%X %X %X %X %X %X %X %X %X %X %X",
                &GuidScan[0],
                &GuidScan[1],
                &GuidScan[2],
                &GuidScan[3],
                &GuidScan[4],
                &GuidScan[5],
                &GuidScan[6],
                &GuidScan[7],
                &GuidScan[8],
                &GuidScan[9],
                &GuidScan[10]
                ) == 11) {
            AddGuid11 (FileName, GuidScan, SymName);
          } 
        }
      }
    }
  }

  fclose (Fptr);
  return STATUS_SUCCESS;
}
//
// Process Intel Itanium(TM) GUID definitions. Look for:
// #define Cs870MemoryTestPEIMGuidL 0x9C2403386E1C8FAA
// #define Cs870MemoryTestPEIMGuidH 0xE89E95C6180342f0
// in either order.
// This function assumes no blank lines between definitions.
//
static
STATUS
ProcessIA64FileGuids (
  INT8    *FileName
  )
{
  FILE    *Fptr;
  INT8    Line[MAX_LINE_LEN];
  UINT32  Guid1H;
  UINT32  Guid1L;
  UINT32  Guid2H;
  UINT32  Guid2L;
  INT8    SymName1[MAX_LINE_LEN];
  INT8    SymName2[MAX_LINE_LEN];
  BOOLEAN Done;
  BOOLEAN LowFirst;
  BOOLEAN FoundLow;

  if ((Fptr = fopen (FileName, "r")) == NULL) {
    Error (NULL, 0, 0, FileName, "could not open input file for reading");
    return STATUS_ERROR;
  }

  Done = FALSE;
  if (fgets (Line, sizeof (Line), Fptr) == NULL) {
    Done = 1;
  }
  //
  // Read lines from the file until done. Since the guid definition takes
  // two lines, we read lines in different places to recover gracefully
  // from mismatches. For example, if you thought you found the first half,
  // but the next line had a symbol mismatch, then you have to process the
  // line again in case it's the start of a new definition.
  //
  while (!Done) {
    //
    // Check current line for GUID definition. Assume low define first.
    //
    if (IsIA64GuidLine (Line, &Guid1H, &Guid1L, &FoundLow, SymName1)) {
      //
      // Might have to swap guids later. Save off if we found the LOW first
      //
      if (FoundLow) {
        LowFirst = TRUE;
      } else {
        LowFirst = FALSE;
      }
      //
      // Read the next line and try for the rest of the guid definition
      //
      if (fgets (Line, sizeof (Line), Fptr) == NULL) {
        Done = 1;
      } else {
        if (IsIA64GuidLine (Line, &Guid2H, &Guid2L, &FoundLow, SymName2)) {
          //
          // Found another. If the symbol names match, then save it off.
          //
          if (strcmp (SymName1, SymName2) == 0) {
            //
            // Yea, found one. Save it off.
            //
            if (LowFirst) {
              AddGuid64x2 (FileName, Guid2H, Guid2L, Guid1H, Guid1L, SymName1);
            } else {
              AddGuid64x2 (FileName, Guid1H, Guid1L, Guid2H, Guid2L, SymName1);
            }
            //
            // Read the next line for processing
            //
            if (fgets (Line, sizeof (Line), Fptr) == NULL) {
              Done = 1;
            }
          } else {
            //
            // Don't get another line so that we reprocess this line in case it
            // contains the start of a new definition.
            // fprintf (stdout, "Symbol name mismatch: %s: %s != %s\n",
            //    FileName, SymName1, SymName2);
            //
          }
        } else {
          //
          // Second line was not a guid definition. Get the next line from the
          // file.
          //
          if (fgets (Line, sizeof (Line), Fptr) == NULL) {
            Done = 1;
          }
        }
      }
    } else {
      //
      // Not a guid define line. Next.
      //
      if (fgets (Line, sizeof (Line), Fptr) == NULL) {
        Done = 1;
      }
    }
  }

  fclose (Fptr);
  return STATUS_SUCCESS;
}
//
// Given a line from an Itanium-based assembly file, check the line for a guid
// defininition. One of either:
// #define Cs870MemoryTestPEIMGuidL 0x9C2403386E1C8FAA
// #define Cs870MemoryTestPEIMGuidH 0xE89E95C6180342f0
// Return the defined value as two 32-bit values, and whether it's a high
// or low guid.
//
static
BOOLEAN
IsIA64GuidLine (
  INT8      *Line,
  UINT32    *GuidHigh,
  UINT32    *GuidLow,
  BOOLEAN   *FoundLow,
  INT8      *SymName
  )
{
  INT8    *Cptr;
  INT8    CSave;
  INT8    *CSavePtr;
  INT8    *SymStart;
  UINT32  Len;

  Cptr = Line;
  Cptr += SkipWhiteSpace (Cptr);
  //
  // look for #define xxxGUID[L|H] 0xHexValue
  //
  if (*Cptr == '#') {
    Cptr++;
    Cptr += SkipWhiteSpace (Cptr);
    //
    // Look for "define"
    //
    if (!strncmp (Cptr, "define", 6)) {
      Cptr += 6;
      //
      // Better be whitespace
      //
      Len = SkipWhiteSpace (Cptr);
      if (Len) {
        Cptr += Len;
        //
        // See if it's a valid symbol name
        //
        Len = ValidSymbolName (Cptr);
        if (Len) {
          //
          // Save the start so we can copy it to their string if later checks are ok
          //
          SymStart = Cptr;
          //
          // It is a valid symbol name, look for the string GuidL or GuidH
          //
          CSavePtr  = Cptr + Len;
          CSave     = *CSavePtr;
          *CSavePtr = 0;
          while (*Cptr) {
            if (strncmp (Cptr, "GuidL", 5) == 0) {
              *FoundLow = 1;
              break;
            } else if (strncmp (Cptr, "GuidH", 5) == 0) {
              *FoundLow = 0;
              break;
            }

            Cptr++;
          }
          //
          // If we didn't run out of string, then we found the GUID string.
          // Restore the null character we inserted above and continue.
          // Now look for  0x.......
          //
          if (*Cptr) {
            //
            // Return symbol name less the "L" or "H"
            //
            strcpy (SymName, SymStart);
            SymName[strlen (SymName) - 1] = 0;
            Cptr                          = CSavePtr;
            *CSavePtr                     = CSave;
            Cptr += SkipWhiteSpace (Cptr);
            if ((*Cptr == '0') && (*(Cptr + 1) == 'x')) {
              //
              // skip over "0x"
              //
              Cptr += 2;
              //
              // 0x0123456789ABCDEF -- null terminate after 8 characters,
              // scan, replace the character and scan at that point.
              //
              CSave       = *(Cptr + 8);
              *(Cptr + 8) = 0;
              if (sscanf (Cptr, "%X", GuidHigh) == 1) {
                *(Cptr + 8) = CSave;
                if (sscanf (Cptr + 8, "%X", GuidLow) == 1) {
                  return TRUE;
                }
              }
            }
          }
        }
      }
    }
  }

  return FALSE;
}
//
// Look at the characters in the string and determine if it's a valid
// symbol name. Basically [a-zA-Z_][a-zA-Z_0-9]*
//
static
UINT32
ValidSymbolName (
  INT8    *Name
  )
{
  int Len;

  Len = 0;

  //
  // Test first character
  //
  if (((*Name >= 'a') && (*Name <= 'z')) || ((*Name >= 'A') && (*Name <= 'Z')) || (*Name == '_')) {
    Name++;
    Len = 1;
    while (*Name) {
      if (((*Name >= 'a') && (*Name <= 'z')) ||
          ((*Name >= 'A') && (*Name <= 'Z')) ||
          ((*Name >= '0') && (*Name <= '9')) ||
          (*Name == '_')
          ) {
        Name++;
        Len++;
      } else {
        break;
      }
    }
  }

  return Len;
}

static
UINT32
SkipWhiteSpace (
  INT8    *Str
  )
{
  UINT32  Len;
  Len = 0;
  while (isspace (*Str) && *Str) {
    Len++;
    Str++;
  }

  return Len;
}
//
// found FFS_FILEGUID=35b898ca-b6a9-49ce-8c72-904735cc49b7
//
static
STATUS
AddPkgGuid (
  INT8      *FileName,
  UINT32    *Data,
  UINT64    *Data64
  )
{
  GUID_RECORD *NewRec;
  int         Index;

  //
  // Sanity check the data
  //
  if ((Data[1] | Data[2] | Data[3]) & 0xFFFF0000) {
    Error (NULL, 0, 0, "out of range value for GUID data word(s) [1] - [3]", NULL);
    return STATUS_ERROR;
  }
  //
  // More checks for Data64?
  // Allocate memory for a new one guid structure
  //
  NewRec = malloc (sizeof (GUID_RECORD));
  if (NewRec == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    return STATUS_ERROR;
  }

  memset ((char *) NewRec, 0, sizeof (GUID_RECORD));
  NewRec->FileName = malloc (strlen (FileName) + 1);
  if (NewRec->FileName == NULL) {
    free (NewRec);
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    return STATUS_ERROR;
  }

  strcpy (NewRec->FileName, FileName);
  NewRec->Guid.Data1    = Data[0];
  NewRec->Guid.Data2    = (UINT16) Data[1];
  NewRec->Guid.Data3    = (UINT16) Data[2];
  NewRec->Guid.Data4[0] = (UINT8) (Data[3] >> 8);
  NewRec->Guid.Data4[1] = (UINT8) Data[3];
  for (Index = 2; Index < 8; Index++) {
    NewRec->Guid.Data4[Index] = ((UINT8*)Data64)[7-Index];
  }
  //
  // Add it to the list
  //
  NewRec->Next  = gGuidList;
  gGuidList     = NewRec;

  //
  // Report it
  // ReportGuid (FileName, NewRec);
  //
  return STATUS_SUCCESS;
}
//
// Add a guid consisting of 11 fields to our list of guids
//
static
STATUS
AddGuid11 (
  INT8      *FileName,
  UINT32    *Data,
  INT8      *SymName
  )
{
  GUID_RECORD *NewRec;
  int         Index;

  //
  // Sanity check the data
  //
  if (!CheckGuidData (Data, 11)) {
    return STATUS_ERROR;
  }
  //
  // Allocate memory for a new one guid structure
  //
  NewRec = malloc (sizeof (GUID_RECORD));
  if (NewRec == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    return STATUS_ERROR;
  }

  memset ((char *) NewRec, 0, sizeof (GUID_RECORD));
  NewRec->FileName = malloc (strlen (FileName) + 1);
  if (NewRec->FileName == NULL) {
    free (NewRec);
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    return STATUS_ERROR;
  }

  strcpy (NewRec->FileName, FileName);
  if (SymName != NULL) {
    NewRec->SymName = malloc (strlen (SymName) + 1);
    if (NewRec->SymName == NULL) {
      free (NewRec);
      Error (NULL, 0, 0, "memory allocation failure", NULL);
      return STATUS_ERROR;
    }
    strcpy (NewRec->SymName, SymName);
  }

  NewRec->Guid.Data1  = Data[0];
  NewRec->Guid.Data2  = (UINT16) Data[1];
  NewRec->Guid.Data3  = (UINT16) Data[2];
  for (Index = 0; Index < 8; Index++) {
    NewRec->Guid.Data4[Index] = (UINT8) Data[3 + Index];
  }
  //
  // Add it to the list
  //
  NewRec->Next  = gGuidList;
  gGuidList     = NewRec;

  //
  // Report it
  // ReportGuid (FileName, NewRec);
  //
  return STATUS_SUCCESS;
}
//
// For debug purposes, print each guid found
//
// static
// VOID
// ReportGuid (
//  INT8        *FileName,
// GUID_RECORD *NewGuid
//  )
// {
//  //fprintf (stdout, "%s: 0x%08X\n", FileName, NewGuid->Guid.Data1);
// }
//
// Free up memory we allocated to keep track of guids defined.
//
static
VOID
FreeGuids (
  VOID
  )
{
  GUID_RECORD *NextRec;
  while (gGuidList != NULL) {
    NextRec = gGuidList->Next;
    if (gGuidList->FileName != NULL) {
      free (gGuidList->FileName);
    }

    if (gGuidList->SymName != NULL) {
      free (gGuidList->SymName);
    }

    free (gGuidList);
    gGuidList = NextRec;
  }
}

static
VOID
FreeSigs (
  VOID
  )
{
  SIGNATURE_RECORD  *NextRec;
  while (gSignatureList != NULL) {
    NextRec = gSignatureList->Next;
    if (gSignatureList->FileName != NULL) {
      free (gSignatureList->FileName);
    }

    free (gSignatureList);
    gSignatureList = NextRec;
  }
}
//
// Scan through all guids defined and compare each for duplicates.
//
static
STATUS
CheckDuplicates (
  VOID
  )
{
  GUID_RECORD       *CurrentFile;

  GUID_RECORD       *TempFile;
  SIGNATURE_RECORD  *CurrentSig;
  SIGNATURE_RECORD  *TempSig;
  STATUS            Status;
  int               Index;
  int               DupCount;
  int               Len;
  BOOLEAN           Same;
  UINT32            GuidSum;
  INT8              *SymName;

  Status = STATUS_SUCCESS;

  //
  // If we're checking guids.....
  //
  if (gOptions.CheckGuids) {
    //
    // If -p option, print all guids found
    //
    if (gOptions.PrintFound) {
      CurrentFile = gGuidList;
      while (CurrentFile != NULL) {
        fprintf (
          stdout,
          "GUID:  0x%08X 0x%04X 0x%04X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X %s\n",
          (UINT32) CurrentFile->Guid.Data1,
          (UINT32) CurrentFile->Guid.Data2,
          (UINT32) CurrentFile->Guid.Data3,
          (UINT32) CurrentFile->Guid.Data4[0],
          (UINT32) CurrentFile->Guid.Data4[1],
          (UINT32) CurrentFile->Guid.Data4[2],
          (UINT32) CurrentFile->Guid.Data4[3],
          (UINT32) CurrentFile->Guid.Data4[4],
          (UINT32) CurrentFile->Guid.Data4[5],
          (UINT32) CurrentFile->Guid.Data4[6],
          (UINT32) CurrentFile->Guid.Data4[7],
          CurrentFile->FileName
          );
        CurrentFile = CurrentFile->Next;
      }
    }

    if (gOptions.GuidXReference) {
      CurrentFile = gGuidList;
      while (CurrentFile != NULL) {
        //
        // If no symbol name, print FileName
        //
        SymName = CurrentFile->SymName;
        if (SymName == NULL) {
          //
          // Assume file name will not be NULL and strlen > 0
          //
          SymName = CurrentFile->FileName + strlen (CurrentFile->FileName) - 1;
          while ((*SymName != '\\') && (SymName > CurrentFile->FileName)) SymName --;
          if (*SymName == '\\') SymName ++;
        }

        fprintf (
          stdout,
          "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X %s\n",
          (UINT32) CurrentFile->Guid.Data1,
          (UINT32) CurrentFile->Guid.Data2,
          (UINT32) CurrentFile->Guid.Data3,
          (UINT32) CurrentFile->Guid.Data4[0],
          (UINT32) CurrentFile->Guid.Data4[1],
          (UINT32) CurrentFile->Guid.Data4[2],
          (UINT32) CurrentFile->Guid.Data4[3],
          (UINT32) CurrentFile->Guid.Data4[4],
          (UINT32) CurrentFile->Guid.Data4[5],
          (UINT32) CurrentFile->Guid.Data4[6],
          (UINT32) CurrentFile->Guid.Data4[7],
          SymName
          );
        CurrentFile = CurrentFile->Next;
      }
    }
    //
    // Now go through all guids and report duplicates.
    //
    CurrentFile = gGuidList;
    while (CurrentFile != NULL) {
      DupCount  = 0;
      TempFile  = CurrentFile->Next;
      while (TempFile) {
        //
        // Compare the guids
        //
        if ((CurrentFile->Guid.Data1 == TempFile->Guid.Data1) &&
            (CurrentFile->Guid.Data2 == TempFile->Guid.Data2) &&
            (CurrentFile->Guid.Data3 == TempFile->Guid.Data3)
            ) {
          //
          // OR in all the guid bytes so we can ignore NULL-guid definitions.
          //
          GuidSum = CurrentFile->Guid.Data1 | CurrentFile->Guid.Data2 | CurrentFile->Guid.Data3;
          Same    = TRUE;
          for (Index = 0; Index < 8; Index++) {
            GuidSum |= CurrentFile->Guid.Data4[Index];
            if (CurrentFile->Guid.Data4[Index] != TempFile->Guid.Data4[Index]) {
              Same = FALSE;
              break;
            }
          }
          //
          // If they're the same, and the guid was non-zero, print a message.
          //
          if (Same && GuidSum) {
            if (DupCount == 0) {
              Error (NULL, 0, 0, "duplicate GUIDS found", NULL);
              fprintf (stdout, "   FILE1: %s\n", CurrentFile->FileName);
            }

            DupCount++;
            fprintf (stdout, "   FILE%d: %s\n", DupCount + 1, TempFile->FileName);
            //
            // Flag it as reported so we don't report it again if there's three or more
            //
            TempFile->Reported = TRUE;
          }
        }
        //
        // Next one
        //
        TempFile = TempFile->Next;
      }
      //
      // Print the guid if we found duplicates
      //
      if (DupCount) {
        fprintf (
          stdout,
          "   GUID:  0x%08X 0x%04X 0x%04X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
          (UINT32) CurrentFile->Guid.Data1,
          (UINT32) CurrentFile->Guid.Data2,
          (UINT32) CurrentFile->Guid.Data3,
          (UINT32) CurrentFile->Guid.Data4[0],
          (UINT32) CurrentFile->Guid.Data4[1],
          (UINT32) CurrentFile->Guid.Data4[2],
          (UINT32) CurrentFile->Guid.Data4[3],
          (UINT32) CurrentFile->Guid.Data4[4],
          (UINT32) CurrentFile->Guid.Data4[5],
          (UINT32) CurrentFile->Guid.Data4[6],
          (UINT32) CurrentFile->Guid.Data4[7]
          );
        //
        // return STATUS_ERROR;
        //
      }
      //
      // Find the next one that hasn't been reported
      //
      do {
        CurrentFile = CurrentFile->Next;
      } while ((CurrentFile != NULL) && (CurrentFile->Reported));
    }
  }

  if (gOptions.CheckSignatures) {
    //
    // Print ones found if specified
    //
    if (gOptions.PrintFound) {
      CurrentSig = gSignatureList;
      while (CurrentSig != NULL) {
        Len = CurrentSig->Signature.DataLen;
        for (Index = 0; Index < Len; Index++) {
          fprintf (stdout, "%c", CurrentSig->Signature.Data[Index]);
        }

        fprintf (stdout, "  %s\n", CurrentSig->FileName);
        CurrentSig = CurrentSig->Next;
      }
    }

    CurrentSig = gSignatureList;
    while (CurrentSig != NULL) {
      DupCount  = 0;
      TempSig   = CurrentSig->Next;
      Len       = CurrentSig->Signature.DataLen;
      while (TempSig) {
        //
        // Check for same length, then do string compare
        //
        if (Len == TempSig->Signature.DataLen) {
          if (strncmp (CurrentSig->Signature.Data, TempSig->Signature.Data, Len) == 0) {
            //
            // Print header message if first failure for this sig
            //
            if (DupCount == 0) {
              Error (NULL, 0, 0, "duplicate signatures found", NULL);
              fprintf (stdout, "   FILE1: %s\n", CurrentSig->FileName);
            }

            DupCount++;
            fprintf (stdout, "   FILE%d: %s\n", DupCount + 1, TempSig->FileName);
            TempSig->Reported = TRUE;
          }
        }

        TempSig = TempSig->Next;
      }

      if (DupCount) {
        fprintf (stdout, "   SIG:   ");
        for (Index = 0; Index < Len; Index++) {
          fprintf (stdout, "%c", CurrentSig->Signature.Data[Index]);
        }

        fprintf (stdout, "\n");
      }
      //
      // On to the next one that hasn't been reported
      //
      do {
        CurrentSig = CurrentSig->Next;
      } while ((CurrentSig != NULL) && (CurrentSig->Reported));
    }
  }

  return Status;
}

static
VOID
FreeOptions (
  VOID
  )
/*++

Routine Description:
  Free up any memory we allocated when processing command-line options.

Arguments:
  None.

Returns:
  NA

Notes:
  We don't free up the ->Str fields because we did not allocate them.
  Instead, we just set the pointer to point to the actual parameter
  from the command line.

--*/
{
  STRING_LIST *Ptr;
  while (gOptions.ExcludeDirs != NULL) {
    Ptr = gOptions.ExcludeDirs->Next;
    //
    // free (gOptions.ExcludeDirs->Str);
    //
    free (gOptions.ExcludeDirs);
    gOptions.ExcludeDirs = Ptr;
  }

  while (gOptions.ExcludeSubDirs != NULL) {
    Ptr = gOptions.ExcludeSubDirs->Next;
    //
    // free (gOptions.ExcludeSubDirs->Str);
    //
    free (gOptions.ExcludeSubDirs);
    gOptions.ExcludeSubDirs = Ptr;
  }

  while (gOptions.ExcludeExtensions != NULL) {
    Ptr = gOptions.ExcludeExtensions->Next;
    //
    // free (gOptions.ExcludeExtensions->Str);
    //
    free (gOptions.ExcludeExtensions);
    gOptions.ExcludeExtensions = Ptr;
  }

  while (gOptions.ExcludeFiles != NULL) {
    Ptr = gOptions.ExcludeFiles->Next;
    //
    // free (gOptions.ExcludeFiles->Str);
    //
    free (gOptions.ExcludeFiles);
    gOptions.ExcludeFiles = Ptr;
  }
}
//
// Given an array of 32-bit data, validate the data for the given number of
// guid data. For example, it might have been scanned as 16 bytes of data, or
// 11 fields of data.
//
static
BOOLEAN
CheckGuidData (
  UINT32    *Data,
  UINT32    DataCount
  )
{
  UINT32  Index;

  if (DataCount == 16) {
    for (Index = 0; Index < 16; Index++) {
      if (Data[Index] &~0xFF) {
        return FALSE;
      }
    }

    return TRUE;
  } else if (DataCount == 11) {
    //
    // Data[0] never out of range (32-bit)
    //
    if ((Data[1] | Data[2]) &~0xFFFF) {
      //
      // Error ("Out of range value for GUID data word(s) [1] and/or [2]");
      //
      return FALSE;
    }

    for (Index = 0; Index < 8; Index++) {
      if (Data[Index + 3] &~0xFF) {
        //
        // Error ("Out of range value for GUID data byte(s) [4] - [11]");
        //
        return FALSE;
      }
    }

    return TRUE;
  }

  return FALSE;
}

static
VOID
ConcatenateLines (
  FILE        *Fptr, 
  INT8        *Line,
  UINT32      Len
  )
{
  UINT32      LineLen;
  BOOLEAN     NeedCheck;
  
  NeedCheck = TRUE;
  while (NeedCheck) {
    LineLen = strlen (Line);
    if ((Line[LineLen - 1] == '\n') && (Line[LineLen - 2] == '\\')) {
      Line[LineLen - 2] = '\0';
      fgets (Line + LineLen - 2, Len - LineLen, Fptr);
    } else if (Line[LineLen - 1] == '\\') {
      Line[LineLen - 1] = '\0';
      fgets (Line + LineLen - 1, Len - LineLen, Fptr);
    } else {
      NeedCheck = FALSE;
    }
  }
}
