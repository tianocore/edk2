/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  HiiPack.c  

Abstract:

  Process HII export and pack files and create HII export files,
  dumps, or variable defaults packs.

--*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "Tiano.h"
#include "EfiUtilityMsgs.h"
#include "ParseInf.h"
#include "EfiInternalFormRepresentation.h"
#include "HiiPack.h"
#include "Hii.h"
#include "IfrParse.h"
#include "StringParse.h"

#define UTILITY_VERSION "v1.0"
#define UTILITY_NAME    "HiiPack"
#define MAX_PATH        260

//
// We may have to create an empty IFR formset to provide a GUID for an HII
// export pack. Create a structure definition to make it easier.
//
#pragma pack(1)

typedef struct {
  EFI_HII_IFR_PACK      PackHeader;
  EFI_IFR_FORM_SET      Formset;
  EFI_IFR_END_FORM_SET  EndFormset;
} EMPTY_FORMSET_PACK;

#pragma pack()
//
// We'll store lists of file names from the command line in
// a linked list of these
//
typedef struct _FILE_NAME_LIST {
  struct _FILE_NAME_LIST  *Next;
  UINT8                   FileName[MAX_PATH];
  int                     Tag;  // used for whatever
} FILE_NAME_LIST;

//
// When merging HII export packs, we save HII data table headers in a linked
// list of these.
//
typedef struct _DATA_TABLE_HEADER_LIST {
  struct _DATA_TABLE_HEADER_LIST  *Next;
  EFI_HII_DATA_TABLE              DataTableHeader;
} DATA_TABLE_HEADER_LIST;
//
// Create some defines for the different operation modes supported by this utility
//
#define MODE_CREATE_HII_EXPORT  1
#define MODE_MERGE_HII_EXPORTS  2
#define MODE_EMIT_DEFAULTS      3
#define MODE_DUMP_HII_EXPORT    4
//
// Here's all our globals.
//
static struct {
  FILE_NAME_LIST  *PackFileNames;           // Input HII pack file names
  FILE_NAME_LIST  *HiiExportFileNames;      // Input files when merging
  CHAR8           OutputFileName[MAX_PATH]; // Output dump file
  BOOLEAN         MfgFlag;                  // From -mfg command line arg
  BOOLEAN         NoEmptyVarPacks;          // From -noemptyvarpacks command line arg
  BOOLEAN         NoVarPacks;               // From -novarpacks command line arg
  EFI_GUID        Guid;                     // Guid specified on command line
  BOOLEAN         GuidSpecified;
  BOOLEAN         DumpStrings;              // In dump mode, dump string data
  int             Verbose;
  int             Mode;                     // Mode this utility is operating in
} mGlobals;

static
void
Usage (
  VOID
  );

static
STATUS
ProcessArgs (
  int   Argc,
  char  *Argv[]
  );

static
STATUS
DumpHiiExportFile (
  char    *HiiExportFileName,
  char    *OutputFileName
  );

static
void
DumpString (
  FILE    *OutFptr,
  int     StringIndex,
  CHAR16  *Str,
  int     Indent
  );

static
void
DumpStringPack (
  FILE                  *OutFptr,
  EFI_HII_STRING_PACK   *Pack,
  int                   BaseOffset,
  int                   Indent
  );

static
void
DumpVariablePacks (
  FILE                  *OutFptr,
  EFI_HII_VARIABLE_PACK *Pack,
  int                   NumPacks,
  int                   BaseOffset,
  int                   Indent
  );

static
void
TestDumpHiiPack (
  FILE    *OutFptr,
  char    *BufferStart,
  int     BufferSize
  );

static
void
DumpRawBytes (
  FILE                  *OutFptr,
  char                  *Buffer,
  int                   Count,
  int                   BaseOffset,
  int                   Indent
  );

static
void
DumpIfrPack (
  FILE                  *OutFptr,
  EFI_HII_IFR_PACK      *Pack,
  int                   BaseOffset,
  int                   Indent
  );

static
void
FreeGlobals (
  VOID
  );

static
STATUS
AddStringPack (
  EFI_HII_STRING_PACK   *PackHeader
  );

static
STATUS
ProcessHiiExportFile (
  char    *FileName,
  int     MfgDefaults
  );

static
STATUS
ProcessIfrFiles (
  FILE_NAME_LIST *FileName
  );

static
STATUS
EmitDefaults (
  FILE_NAME_LIST *HiiExportFiles,
  int            MfgDefaults,
  int            NoEmptyVarPacks
  );

static
STATUS
MergeHiiExports (
  FILE_NAME_LIST *HiiExportFiles,
  char           *OutputFileName,
  int            MfgDefaults,
  int            NoEmptyVarPacks
  );

void
GuidToString (
  EFI_GUID   *Guid,
  char       *Str
  );

static
CHAR16  *
AsciiToWchar (
  CHAR8 *Str
  );

static
STATUS
CreateHiiExport (
  char              *OutputFileName,
  EFI_GUID          *DummyFormsetGuid,
  FILE_NAME_LIST    *PackFiles,
  int               MfgDefaults
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

  Standard C main() argc and argv.

Returns:

  0       if successful
  nonzero otherwise
  
--*/
// GC_TODO:    Argc - add argument and description to function comment
// GC_TODO:    ] - add argument and description to function comment
{
  STATUS  Status;
  //
  // Set the utility name for error reporting purposes
  //
  SetUtilityName (UTILITY_NAME);
  //
  // Process the command-line arguments
  //
  Status = ProcessArgs (Argc, Argv);
  if (Status != STATUS_SUCCESS) {
    return Status;
  }
  //
  // Switch based on whether we're dumping, merging, etc.
  //
  if (mGlobals.Mode == MODE_DUMP_HII_EXPORT) {
    if (mGlobals.Verbose) {
      fprintf (stdout, "Dumping HII export file %s => %s\n", mGlobals.HiiExportFileNames, mGlobals.OutputFileName);
    }

    DumpHiiExportFile (mGlobals.HiiExportFileNames->FileName, mGlobals.OutputFileName);
  } else if (mGlobals.Mode == MODE_CREATE_HII_EXPORT) {
    CreateHiiExport (mGlobals.OutputFileName, &mGlobals.Guid, mGlobals.PackFileNames, mGlobals.MfgFlag);
  } else if (mGlobals.Mode == MODE_MERGE_HII_EXPORTS) {
    MergeHiiExports (mGlobals.HiiExportFileNames, mGlobals.OutputFileName, mGlobals.MfgFlag, mGlobals.NoEmptyVarPacks);
  } else if (mGlobals.Mode == MODE_EMIT_DEFAULTS) {
    EmitDefaults (mGlobals.HiiExportFileNames, mGlobals.MfgFlag, mGlobals.NoEmptyVarPacks);
  }
  //
  //
  FreeGlobals ();
  IfrParseEnd ();
  StringEnd ();
  return GetUtilityStatus ();
}

/******************************************************************************/
static
STATUS
MergeHiiExports (
  FILE_NAME_LIST *HiiExportFiles,
  char           *OutputFileName,
  int            MfgDefaults,
  int            NoEmptyVarPacks
  )
/*++

Routine Description:

  Given a linked list of input HII export pack files, read in the contents
  of each and create a single HII export pack that contains the contents
  of all the input files.
  
Arguments:

  HiiExportFiles    - pointer to linked list of input HII export pack file names
  OutputFileName    - name of output (merged) HII export file
  MfgDefaults       - non-zero to emit manufacturing defaults in output file
  NoEmptyVarPacks   - non-zero to not emit 0-length variable packs to the output file

Returns:

  STATUS_SUCCESS    - if successful
  STATUS_ERROR      - otherwise
  
--*/
{
  EFI_HII_HANDLE          HiiHandle;
  FILE                    *OutFptr;
  FILE                    *InFptr;
  STATUS                  Status;
  CHAR8                   *Buffer;
  int                     FileSize;
  int                     DataTableIndex;
  int                     Count;
  int                     NumDataTables;
  EFI_HII_EXPORT_TABLE    *HiiExportTableHeader;
  EFI_HII_EXPORT_TABLE    TempHiiExportTableHeader;
  EFI_HII_DATA_TABLE      *DataTableHeader;
  EFI_HII_STRING_PACK     *StringPack;
  EFI_HII_VARIABLE_PACK   *VarPack;
  EFI_HII_IFR_PACK        *IfrPack;
  EFI_GUID                HiiExportRevisionGuid = EFI_HII_PROTOCOL_GUID;
  EFI_GUID                PackageGuid;
  EFI_GUID                FormsetGuid;
  long                    DataTableHeaderOffset;
  DATA_TABLE_HEADER_LIST  *DataTableList;
  DATA_TABLE_HEADER_LIST  *LastDataTable;
  DATA_TABLE_HEADER_LIST  *TempDataTable;
  //
  // Init locals
  //
  HiiHandle     = FIRST_HII_PACK_HANDLE;
  Buffer        = NULL;
  InFptr        = NULL;
  OutFptr       = NULL;
  Status        = STATUS_ERROR;
  DataTableList = NULL;
  LastDataTable = NULL;
  //
  // Initialize our IFR parser and string routines
  //
  IfrParseInit ();
  StringInit ();
  //
  // Process each input HII export file
  //
  NumDataTables = 0;
  while (HiiExportFiles != NULL) {
    if (mGlobals.Verbose) {
      fprintf (stdout, "Processing file %s\n", HiiExportFiles->FileName);
    }
    //
    // Read in the entire file contents
    //
    if ((InFptr = fopen (HiiExportFiles->FileName, "rb")) == NULL) {
      Error (NULL, 0, 0, HiiExportFiles->FileName, "failed to open HII export file for reading");
      goto Done;
    }

    fseek (InFptr, 0, SEEK_END);
    FileSize = (int) ftell (InFptr);
    fseek (InFptr, 0, SEEK_SET);
    Buffer = (CHAR8 *) malloc (FileSize);
    if (Buffer == NULL) {
      Error (NULL, 0, 0, "memory allocation failure", NULL);
      goto Done;
    }

    if (fread (Buffer, FileSize, 1, InFptr) != 1) {
      Error (NULL, 0, 0, HiiExportFiles->FileName, "failed to read file contents");
      goto Done;
    }

    fclose (InFptr);
    InFptr                = NULL;
    HiiExportTableHeader  = (EFI_HII_EXPORT_TABLE *) Buffer;
    //
    // Walk all the data tables
    //
    DataTableHeader = (EFI_HII_DATA_TABLE *) (HiiExportTableHeader + 1);
    for (DataTableIndex = 0; DataTableIndex < (int) HiiExportTableHeader->NumberOfHiiDataTables; DataTableIndex++) {
      NumDataTables++;
      //
      // Make sure we're still pointing into our buffer
      //
      if (((char *) DataTableHeader < Buffer) || ((char *) DataTableHeader > Buffer + FileSize)) {
        Error (NULL, 0, 0, "bad data table size in input file", NULL);
        goto Done;
      }
      //
      // Save a copy of the data table header
      //
      TempDataTable = (DATA_TABLE_HEADER_LIST *) malloc (sizeof (DATA_TABLE_HEADER_LIST));
      if (TempDataTable == NULL) {
        Error (NULL, 0, 0, "memory allocation failure", NULL);
        goto Done;
      }

      memset ((void *) TempDataTable, 0, sizeof (DATA_TABLE_HEADER_LIST));
      memcpy (&TempDataTable->DataTableHeader, DataTableHeader, sizeof (EFI_HII_DATA_TABLE));
      if (DataTableList == NULL) {
        DataTableList = TempDataTable;
      } else {
        LastDataTable->Next = TempDataTable;
      }

      LastDataTable = TempDataTable;
      //
      // If there is an IFR pack, parse it
      //
      if (DataTableHeader->IfrDataOffset != 0) {
        if (IfrParsePack (
            HiiHandle,
            (EFI_HII_IFR_PACK *) ((char *) DataTableHeader + DataTableHeader->IfrDataOffset),
            &DataTableHeader->PackageGuid
            ) != STATUS_SUCCESS
            ) {
          goto Done;
        }
      }
      //
      // If there is string data, save it
      //
      if (DataTableHeader->StringDataOffset != 0) {
        Status = StringParsePack (
                  HiiHandle,
                  (EFI_HII_STRING_PACK *) ((char *) DataTableHeader + DataTableHeader->StringDataOffset),
                  NULL,
                  &DataTableHeader->PackageGuid
                  );
        if (Status != STATUS_SUCCESS) {
          goto Done;
        }
      }
      //
      // If there is device path data, process it
      //
      if (DataTableHeader->DevicePathOffset != 0) {
        Error (NULL, 0, 0, "application error", "%s contains unsupported device path data", HiiExportFiles->FileName);
        goto Done;
      }
      //
      // Next data pack
      //
      DataTableHeader = (EFI_HII_DATA_TABLE *) ((char *) DataTableHeader + DataTableHeader->DataTableSize);
      HiiHandle++;
    }

    free (Buffer);
    Buffer = NULL;
    //
    // Next input file
    //
    HiiExportFiles = HiiExportFiles->Next;
  }
  //
  // Now create defaults
  //
  if (IfrSetDefaults (MfgDefaults) != STATUS_SUCCESS) {
    goto Done;
  }
  //
  // Create and write the output HII export header
  //
  if ((OutFptr = fopen (OutputFileName, "wb")) == NULL) {
    Error (NULL, 0, 0, OutputFileName, "failed to open output file for writing");
    goto Done;
  }

  memset ((void *) &TempHiiExportTableHeader, 0, sizeof (EFI_HII_EXPORT_TABLE));
  TempHiiExportTableHeader.NumberOfHiiDataTables = HiiHandle - FIRST_HII_PACK_HANDLE;
  memcpy (&TempHiiExportTableHeader.Revision, &HiiExportRevisionGuid, sizeof (EFI_GUID));
  if (fwrite ((void *) &TempHiiExportTableHeader, sizeof (EFI_HII_EXPORT_TABLE), 1, OutFptr) != 1) {
    Error (NULL, 0, 0, OutputFileName, "failed to write HII export table header to output file");
    goto Done;
  }
  //
  // Now go back through all the handles and create new data packs for each, writing out
  // the contents as we go.
  //
  HiiHandle = FIRST_HII_PACK_HANDLE;
  for (TempDataTable = DataTableList; TempDataTable != NULL; TempDataTable = TempDataTable->Next) {
    //
    // Write a data table header to the output file. We'll rewind the file and
    // write an updated one when we're done with this data set
    //
    DataTableHeaderOffset                         = ftell (OutFptr);
    TempDataTable->DataTableHeader.HiiHandle      = HiiHandle;
    TempDataTable->DataTableHeader.DataTableSize  = sizeof (EFI_HII_DATA_TABLE);
    //
    // We may change the number of variable data when merging export files, so init to 0
    //
    TempDataTable->DataTableHeader.NumberOfVariableData = 0;
    if (fwrite ((void *) &TempDataTable->DataTableHeader, sizeof (EFI_HII_DATA_TABLE), 1, OutFptr) != 1) {
      Error (NULL, 0, 0, OutputFileName, "failed to write HII data table header to output file");
      goto Done;
    }
    //
    // Get the string pack if any
    //
    Status = StringGetPack (HiiHandle, &StringPack, &FileSize, &Count, &FormsetGuid, &PackageGuid);
    if (Status == STATUS_SUCCESS) {
      TempDataTable->DataTableHeader.StringDataOffset = TempDataTable->DataTableHeader.DataTableSize;
      TempDataTable->DataTableHeader.DataTableSize += FileSize;
      //
      // TempDataTable->DataTableHeader.NumberOfLanguages should be unchanged
      //
      if (fwrite ((void *) StringPack, FileSize, 1, OutFptr) != 1) {
        Error (NULL, 0, 0, "failed to write string pack to output file", NULL);
        goto Done;
      }
    }
    //
    // Get the IFR pack
    //
    Status = IfrGetIfrPack (HiiHandle, &IfrPack, &FormsetGuid);
    if (Status == STATUS_SUCCESS) {
      //
      // Write the IFR pack, followed by the variable packs
      //
      TempDataTable->DataTableHeader.IfrDataOffset = TempDataTable->DataTableHeader.DataTableSize;
      TempDataTable->DataTableHeader.DataTableSize += IfrPack->Header.Length;
      if (fwrite ((void *) IfrPack, IfrPack->Header.Length, 1, OutFptr) != 1) {
        Error (NULL, 0, 0, "failed to write IFR pack to output file", NULL);
        goto Done;
      }
      //
      // If this is just a formset stub, then don't write the variable packs
      //
      if (IfrPack->Header.Length != sizeof (EMPTY_FORMSET_PACK)) {
        //
        // Go through all the variable packs and see if they're referenced by this IFR
        //
        Count = 0;
        do {
          Status = IfrGetVarPack (Count, &VarPack);
          if (Status == STATUS_SUCCESS) {
            //
            // Check for variable data length of 0
            //
            if ((NoEmptyVarPacks == 0) ||
                ((VarPack->Header.Length - sizeof (EFI_HII_VARIABLE_PACK) - VarPack->VariableNameLength) != 0)
                ) {
              //
              // See if it's referenced by this IFR
              //
              if (IfrReferencesVarPack (HiiHandle, VarPack) == STATUS_SUCCESS) {
                if (TempDataTable->DataTableHeader.VariableDataOffset == 0) {
                  TempDataTable->DataTableHeader.VariableDataOffset = TempDataTable->DataTableHeader.DataTableSize;
                }

                TempDataTable->DataTableHeader.DataTableSize += VarPack->Header.Length;
                TempDataTable->DataTableHeader.NumberOfVariableData++;
                if (fwrite ((void *) VarPack, VarPack->Header.Length, 1, OutFptr) != 1) {
                  Error (NULL, 0, 0, "failed to write variable pack to output file", NULL);
                  goto Done;
                }

              }
            }
          }

          Count++;
        } while (Status == STATUS_SUCCESS);
      }

      Status = STATUS_SUCCESS;
    }
    //
    // Get the device path pack
    //
    //
    // Rewind the file and write the updated data table header.
    //
    fseek (OutFptr, DataTableHeaderOffset, SEEK_SET);
    if (fwrite ((void *) &TempDataTable->DataTableHeader, sizeof (EFI_HII_DATA_TABLE), 1, OutFptr) != 1) {
      Error (NULL, 0, 0, OutputFileName, "failed to write HII data table header to output file");
      goto Done;
    }

    fseek (OutFptr, 0, SEEK_END);
    HiiHandle++;
  }

  Status = STATUS_SUCCESS;
Done:
  IfrParseEnd ();
  StringEnd ();
  if (Buffer != NULL) {
    free (Buffer);
  }

  if (InFptr != NULL) {
    fclose (InFptr);
  }

  if (OutFptr != NULL) {
    fclose (OutFptr);
  }

  while (DataTableList != NULL) {
    TempDataTable = DataTableList->Next;
    free (DataTableList);
    DataTableList = TempDataTable;
  }

  return Status;
}

/******************************************************************************/
static
STATUS
CreateHiiExport (
  char              *OutputFileName,
  EFI_GUID          *DummyFormsetGuid,
  FILE_NAME_LIST    *PackFiles,
  int               MfgDefaults
  )
/*++

Routine Description:

  Given a linked list of HII pack file names, walk the list to
  process them and create a single HII export file.
  
Arguments:

  OutputFileName    - name of output HII export file to create
  DummyFormsetGuid  - IFR formsets contain a GUID which is used in many 
                      places while processing data tables. If we were not
                      given an IFR pack, then we'll create a stub IFR
                      pack using this GUID as the formset GUID.
  PackFiles         - linked list of HII pack files to process
  MfgDefaults       - when creating variable packs (via IFR pack processing),
                      use manufacturing defaults rather than standard defaults

Returns:

  STATUS_SUCCESS    - if successful
  STATUS_ERROR      - otherwise
  
--*/
{
  STATUS                      Status;
  EMPTY_FORMSET_PACK          EmptyFormset;
  EFI_HII_DATA_TABLE          DataTableHeader;
  EFI_HII_EXPORT_TABLE        ExportTableHeader;
  long                        DataTableHeaderOffset;
  long                        FileSize;
  FILE                        *OutFptr;
  FILE                        *InFptr;
  FILE_NAME_LIST              *TempFile;
  EFI_GUID                    HiiExportRevisionGuid = EFI_HII_PROTOCOL_GUID;
  EFI_GUID                    TempGuid;
  EFI_GUID                    PackageGuid;
  char                        *Buffer;
  EFI_HII_VARIABLE_PACK       *VarPack;
  EFI_HII_IFR_PACK            *IfrPack;
  EFI_HII_STRING_PACK_HEADER  *StringPack;
  EFI_HII_STRING_PACK_HEADER  TerminatorStringPack;
  int                         NumIfr;
  int                         NumStrings;
  int                         Index;
  int                         VarPackIndex;
  //
  // If no input HII pack files, then why are we here? Should have been caught when
  // args were processed though.
  //
  if (PackFiles == NULL) {
    Error (NULL, 0, 0, "no input pack files specified", NULL);
    return STATUS_ERROR;
  }

  InFptr  = NULL;
  Status  = STATUS_ERROR;
  Buffer  = NULL;
  //
  // Open the output file for writing
  //
  if ((OutFptr = fopen (OutputFileName, "wb")) == NULL) {
    Error (NULL, 0, 0, OutputFileName, "failed to open output file for writing");
    goto Done;
  }
  //
  // Figure out how many data tables we are going to need. We'll create one
  // data table if no more than one IFR, or we'll create one data table per IFR,
  // and then one for strings if multiple IFR
  //
  NumIfr      = 0;
  NumStrings  = 0;
  for (TempFile = PackFiles; TempFile != NULL; TempFile = TempFile->Next) {
    if (TempFile->Tag == EFI_HII_IFR) {
      NumIfr++;
    } else if (TempFile->Tag == EFI_HII_STRING) {
      NumStrings++;
    }
  }
  //
  // Three possibilities:
  //  1) No IFR, so create one data table that contains only strings and an empty formset
  //  2) Only 1 IFR, so create an export table with one data table that contains the IFR
  //     and all the strings
  //  3) Multiple IFR, so create a data table for each IFR and another data table with
  //     all the strings.
  //
  // Initialize the export table header and write it out
  //
  memset ((void *) &ExportTableHeader, 0, sizeof (EFI_HII_EXPORT_TABLE));
  if (NumIfr < 2) {
    ExportTableHeader.NumberOfHiiDataTables = 1;
  } else {
    //
    // One data table per IFR, plus one for strings (if any).
    //
    ExportTableHeader.NumberOfHiiDataTables = (UINT16) NumIfr;
    if (NumStrings != 0) {
      ExportTableHeader.NumberOfHiiDataTables++;
    }
  }
  //
  // Init the GUID in the HII export table header
  //
  memcpy (&ExportTableHeader.Revision, &HiiExportRevisionGuid, sizeof (EFI_GUID));
  if (fwrite ((void *) &ExportTableHeader, sizeof (EFI_HII_EXPORT_TABLE), 1, OutFptr) != 1) {
    Error (NULL, 0, 0, OutputFileName, "failed to write HII export table header to output file");
    goto Done;
  }
  //
  // *****************************************************************************************
  //
  //  CASE 1 - No IFR => one data table that contains only strings and an empty formset.
  //           No variable data.
  //
  //  CASE 2 - Only 1 IFR => create an export table with one data table that contains the IFR
  //           and all the strings plus variable data
  //
  //  CASE 3 - Multiple IFR => create a data table for each IFR and another data table with
  //           all the strings. Each IFR data table has variable data if applicable.
  //
  // *****************************************************************************************
  //
  // If the user did not give us an IFR file, then we'll have to create an empty formset
  // and emit it to the output file. In this case, we need a formset GUID on the command
  // line.
  //
  if ((NumIfr == 0) && (mGlobals.GuidSpecified == 0)) {
    //
    //    Warning (NULL, 0, 0, "using NULL GUID for empty formset", "specify -g GUID on the command line if desired");
    //
    memset ((void *) &PackageGuid, 0, sizeof (EFI_GUID));
  } else if (mGlobals.GuidSpecified) {
    //
    // Use it for the package GUID
    //
    memcpy (&PackageGuid, &mGlobals.Guid, sizeof (EFI_GUID));
  }
  //
  // Init the data table header.
  // Write out the blank data table header. Save the offset so we can
  // write an updated version at the end of processing.
  //
  memset ((void *) &DataTableHeader, 0, sizeof (EFI_HII_DATA_TABLE));
  DataTableHeaderOffset     = ftell (OutFptr);
  DataTableHeader.HiiHandle = FIRST_HII_PACK_HANDLE;
  if (mGlobals.Verbose) {
    fprintf (stdout, "writing data table (first time) to offset 0x%X\n", ftell (OutFptr));
  }

  if (fwrite ((void *) &DataTableHeader, sizeof (EFI_HII_DATA_TABLE), 1, OutFptr) != 1) {
    Error (NULL, 0, 0, "failed to write Data Table Header to output file", NULL);
    goto Done;
  }
  //
  // Set the data table size, then write out all the string packs
  //
  DataTableHeader.DataTableSize = sizeof (EFI_HII_DATA_TABLE);
  //
  // Write out the string files to a single data record
  //
  for (TempFile = PackFiles; TempFile != NULL; TempFile = TempFile->Next) {
    //
    // Continue to next file if it's not a string pack file
    //
    if (TempFile->Tag != EFI_HII_STRING) {
      continue;
    }
    //
    // Set the offset in the header if this is the first string pack
    //
    if (DataTableHeader.StringDataOffset == 0) {
      DataTableHeader.StringDataOffset = DataTableHeader.DataTableSize;
    }

    if ((InFptr = fopen (TempFile->FileName, "rb")) == NULL) {
      Error (NULL, 0, 0, TempFile->FileName, "failed to open input string pack file for reading");
      goto Done;
    }
    //
    // Get the file size, then read it into a buffer
    //
    fseek (InFptr, 0, SEEK_END);
    FileSize = ftell (InFptr);
    fseek (InFptr, 0, SEEK_SET);
    Buffer = (char *) malloc (FileSize);
    if (Buffer == NULL) {
      Error (NULL, 0, 0, TempFile->FileName, "memory allocation failure reading in file contents");
      goto Done;
    }

    if (fread (Buffer, FileSize, 1, InFptr) != 1) {
      Error (NULL, 0, 0, TempFile->FileName, "failed to read file contents");
      goto Done;
    }

    fclose (InFptr);
    InFptr = NULL;
    //
    // Verify that it's actually a string pack
    //
    StringPack = (EFI_HII_STRING_PACK_HEADER *) Buffer;
    while ((char *) StringPack < Buffer + FileSize) {
      if (StringPack->Header.Type != EFI_HII_STRING) {
        Error (NULL, 0, 0, TempFile->FileName, "file does not consist entirely of string packs");
        goto Done;
      }

      if (StringPack->Header.Length == 0) {
        break;
      }

      DataTableHeader.NumberOfLanguages++;
      DataTableHeader.DataTableSize += StringPack->Header.Length;
      //
      // Write the string pack to the output file
      //
      if (mGlobals.Verbose) {
        fprintf (stdout, "writing string pack to offset 0x%X\n", ftell (OutFptr));
      }

      if (fwrite (StringPack, StringPack->Header.Length, 1, OutFptr) != 1) {
        Error (NULL, 0, 0, TempFile->FileName, "failed to write string pack to output file");
        goto Done;
      }
      //
      // Sanity check that adding the length advances us (no wrap)
      //
      if ((char *) StringPack + StringPack->Header.Length <= (char *) StringPack) {
        Error (NULL, 0, 0, TempFile->FileName, "invalid pack size in file");
        goto Done;
      }

      StringPack = (EFI_HII_STRING_PACK_HEADER *) ((char *) StringPack + StringPack->Header.Length);
    }
    //
    // Free up buffer, go to next input string pack file
    //
    free (Buffer);
    Buffer = NULL;
  }
  //
  // Write a null-terminator string pack if we had any string packs at all
  //
  if (DataTableHeader.StringDataOffset != 0) {
    memset (&TerminatorStringPack, 0, sizeof (EFI_HII_STRING_PACK_HEADER));
    TerminatorStringPack.Header.Length  = 0;
    TerminatorStringPack.Header.Type    = EFI_HII_STRING;
    if (mGlobals.Verbose) {
      fprintf (stdout, "writing terminator string pack to offset 0x%X\n", ftell (OutFptr));
    }

    if (fwrite (&TerminatorStringPack, sizeof (EFI_HII_STRING_PACK_HEADER), 1, OutFptr) != 1) {
      Error (NULL, 0, 0, "failed to write string pack terminator to output file", NULL);
      goto Done;
    }

    DataTableHeader.DataTableSize += sizeof (EFI_HII_STRING_PACK_HEADER);
  }
  //
  // Parse all the IFR packs, then get the GUID from the first
  // one so we can use it for the package GUID if necessary.
  //
  memcpy (&PackageGuid, &mGlobals.Guid, sizeof (EFI_GUID));
  if (NumIfr != 0) {
    IfrParseInit ();
    if (ProcessIfrFiles (PackFiles) != STATUS_SUCCESS) {
      goto Done;
    }
    //
    // Set variable defaults in all variable packs
    //
    IfrSetDefaults (MfgDefaults);
    //
    // Get the GUID from the first IFR pack if the user did not specify a GUID on
    // the command line.
    //
    if (mGlobals.GuidSpecified == 0) {
      if (IfrGetIfrPack (FIRST_HII_PACK_HANDLE, &IfrPack, &PackageGuid) != STATUS_SUCCESS) {
        Error (NULL, 0, 0, "internal application error", "failed to retrieve IFR pack after parsing");
        goto Done;
      }
    }
  }
  //
  // Set the package GUID in the data table header.
  //
  memcpy (&DataTableHeader.PackageGuid, &PackageGuid, sizeof (EFI_GUID));
  //
  // If no IFR, then create and write the empty formset. Otherwise
  // parse the IFR and emit it and the variable data for it.
  //
  if (NumIfr == 0) {
    memset ((void *) &EmptyFormset, 0, sizeof (EMPTY_FORMSET_PACK));
    EmptyFormset.PackHeader.Header.Type   = EFI_HII_IFR;
    EmptyFormset.PackHeader.Header.Length = sizeof (EMPTY_FORMSET_PACK);
    //
    // Formset Opcode
    //
    EmptyFormset.Formset.Header.OpCode  = EFI_IFR_FORM_SET_OP;
    EmptyFormset.Formset.Header.Length  = (UINT8) sizeof (EFI_IFR_FORM_SET);
    memcpy (&EmptyFormset.Formset.Guid, &PackageGuid, sizeof (EFI_GUID));
    //
    // EndFormset Opcode
    //
    EmptyFormset.EndFormset.Header.OpCode = EFI_IFR_END_FORM_SET_OP;
    EmptyFormset.EndFormset.Header.Length = (UINT8) sizeof (EFI_IFR_END_FORM_SET);
    DataTableHeader.IfrDataOffset         = DataTableHeader.DataTableSize;
    if (mGlobals.Verbose) {
      fprintf (stdout, "writing stub IFR formset to to offset 0x%X\n", ftell (OutFptr));
    }

    if (fwrite (&EmptyFormset, sizeof (EMPTY_FORMSET_PACK), 1, OutFptr) != 1) {
      Error (NULL, 0, 0, OutputFileName, "failed to write formset stub to output file");
      goto Done;
    }

    DataTableHeader.DataTableSize += sizeof (EMPTY_FORMSET_PACK);
    //
    // Go back and re-write the data table header, reposition to the end, then return.
    //
    fseek (OutFptr, DataTableHeaderOffset, SEEK_SET);
    if (mGlobals.Verbose) {
      fprintf (stdout, "writing data table (second time) to offset 0x%X\n", ftell (OutFptr));
    }

    if (fwrite ((void *) &DataTableHeader, sizeof (EFI_HII_DATA_TABLE), 1, OutFptr) != 1) {
      Error (NULL, 0, 0, "failed to write Data Table Header to output file", NULL);
      goto Done;
    }

    fseek (OutFptr, 0, SEEK_END);
    if (mGlobals.Verbose) {
      fprintf (
        stdout,
        "final file offset=0x%X DataTableHeader.DataTableSize=0x%X\n",
        ftell (OutFptr),
        DataTableHeader.DataTableSize
        );
    }
  } else if (NumIfr == 1) {
    //
    // They gave us one input IFR file. We parsed it above, so get each one
    // and emit the IFR and each variable pack it references.
    // Update the data pack header for the IFR pack, then write the IFR pack data
    //
    DataTableHeader.IfrDataOffset = DataTableHeader.DataTableSize;
    if (IfrGetIfrPack (FIRST_HII_PACK_HANDLE, &IfrPack, &TempGuid) != STATUS_SUCCESS) {
      Error (NULL, 0, 0, "internal application error", "failed to retrieve IFR pack after parsing");
      goto Done;
    }

    if (mGlobals.Verbose) {
      fprintf (stdout, "writing IFR pack to 0x%X\n", ftell (OutFptr));
    }

    if (fwrite (IfrPack, IfrPack->Header.Length, 1, OutFptr) != 1) {
      Error (NULL, 0, 0, OutputFileName, "failed to write IFR pack to output file");
      goto Done;
    }

    DataTableHeader.DataTableSize += IfrPack->Header.Length;
    //
    // Now go through all the variable packs discovered during IFR processing
    // and write them to the output file
    //
    if (mGlobals.NoVarPacks == 0) {
      Index = 0;
      do {
        Status = IfrGetVarPack (Index, &VarPack);
        if (Status == STATUS_SUCCESS) {
          //
          // If this is the first variable pack, then update the "offset
          // to variable data" in the data table header
          //
          if (Index == 0) {
            DataTableHeader.VariableDataOffset = DataTableHeader.DataTableSize;
          }

          DataTableHeader.DataTableSize += VarPack->Header.Length;
          DataTableHeader.NumberOfVariableData++;
          if (fwrite ((void *) VarPack, VarPack->Header.Length, 1, OutFptr) != 1) {
            Error (NULL, 0, 0, OutputFileName, "failed to write variable pack to output file");
            goto Done;
          }

          Index++;
        }
      } while (Status == STATUS_SUCCESS);
    }
    //
    // Reposition in the output file and write the updated data table header
    //
    fseek (OutFptr, DataTableHeaderOffset, SEEK_SET);
    if (fwrite ((void *) &DataTableHeader, sizeof (EFI_HII_DATA_TABLE), 1, OutFptr) != 1) {
      Error (NULL, 0, 0, "failed to write Data Table Header to output file", NULL);
      goto Done;
    }

    fseek (OutFptr, 0, SEEK_END);
  } else {
    //
    // Multiple IFR input files. Close out the current data table (strings)
    // if applicable. Then retrieve each parsed IFR pack and create a data pack
    // that contains the IFR (one per data set) and the variable packs that
    // the given IFR form references.
    //
    if (NumStrings != 0) {
      fseek (OutFptr, DataTableHeaderOffset, SEEK_SET);
      if (fwrite ((void *) &DataTableHeader, sizeof (EFI_HII_DATA_TABLE), 1, OutFptr) != 1) {
        Error (NULL, 0, 0, "failed to write Data Table Header to output file", NULL);
        goto Done;
      }

      fseek (OutFptr, 0, SEEK_END);
    } else {
      //
      // No strings, so back up over the data table header we wrote because we assumed
      // at least one string pack.
      //
      fseek (OutFptr, DataTableHeaderOffset, SEEK_SET);
    }
    //
    // Now go through all the IFR packs and write them out, along with variable
    // data referenced by each. Note that multiple IFR forms can refer to the
    // same variables, so the same variable data could be duplicated in multiple
    // data packs.
    //
    Index = FIRST_HII_PACK_HANDLE;
    while (IfrGetIfrPack (Index, &IfrPack, &TempGuid) == STATUS_SUCCESS) {
      //
      // Initialize the data table header
      //
      memset (&DataTableHeader, 0, sizeof (EFI_HII_DATA_TABLE));
      memcpy (&DataTableHeader.PackageGuid, &PackageGuid, sizeof (EFI_GUID));
      //
      // If we didn't have strings, then the HiiHandle should be just Index,
      // rather than Index+1. But since the HiiHandle is not required to start
      // with 1, we'll let it be Index+1.
      //
      DataTableHeader.HiiHandle     = (EFI_HII_HANDLE) (Index + 1);
      DataTableHeader.DataTableSize = sizeof (EFI_HII_DATA_TABLE);
      //
      // Save the file offset of the data table header so we can write an updated
      // version later.
      //
      DataTableHeaderOffset = ftell (OutFptr);
      if (mGlobals.Verbose) {
        fprintf (stdout, "writing data table header to 0x%X\n", ftell (OutFptr));
      }

      if (fwrite ((void *) &DataTableHeader, sizeof (EFI_HII_DATA_TABLE), 1, OutFptr) != 1) {
        Error (NULL, 0, 0, "failed to write Data Table Header to output file", NULL);
        goto Done;
      }

      DataTableHeader.IfrDataOffset = DataTableHeader.DataTableSize;
      if (fwrite (IfrPack, IfrPack->Header.Length, 1, OutFptr) != 1) {
        Error (NULL, 0, 0, OutputFileName, "failed to write IFR pack to output file");
        goto Done;
      }

      DataTableHeader.DataTableSize += IfrPack->Header.Length;
      //
      // Go through all the variable packs and see if this IFR references each. If the
      // IFR does reference it, then add the variable pack to the output.
      //
      if (mGlobals.NoVarPacks == 0) {
        VarPackIndex = 0;
        while (IfrGetVarPack (VarPackIndex, &VarPack) == STATUS_SUCCESS) {
          //
          // See if the IFR references this variable pack
          //
          if (IfrReferencesVarPack (Index, VarPack) == STATUS_SUCCESS) {
            //
            // If this is the first variable pack, then set the offset in
            // the data table header.
            //
            if (DataTableHeader.VariableDataOffset == 0) {
              DataTableHeader.VariableDataOffset = DataTableHeader.DataTableSize;
            }
            //
            // Write the variable pack
            //
            if (fwrite (VarPack, VarPack->Header.Length, 1, OutFptr) != 1) {
              Error (NULL, 0, 0, OutputFileName, "failed to write variable pack to output file");
              goto Done;
            }

            DataTableHeader.NumberOfVariableData++;
            DataTableHeader.DataTableSize += VarPack->Header.Length;
          }

          VarPackIndex++;
        }
      }
      //
      // Write the updated data table header
      //
      fseek (OutFptr, DataTableHeaderOffset, SEEK_SET);
      if (fwrite ((void *) &DataTableHeader, sizeof (EFI_HII_DATA_TABLE), 1, OutFptr) != 1) {
        Error (NULL, 0, 0, "failed to write Data Table Header to output file", NULL);
        goto Done;
      }

      fseek (OutFptr, 0, SEEK_END);
      //
      // Next IFR pack
      //
      Index++;
    }
  }

  Status = STATUS_SUCCESS;
Done:
  IfrParseEnd ();
  StringEnd ();
  if (Buffer != NULL) {
    free (Buffer);
  }

  if (InFptr != NULL) {
    fclose (InFptr);
  }

  if (OutFptr != NULL) {
    fclose (OutFptr);
  }

  return Status;
}

/******************************************************************************/
static
STATUS
ProcessIfrFiles (
  FILE_NAME_LIST  *FileName
  )
/*++

Routine Description:

  Given a linked list of pack file names, read in each IFR pack file
  and process the contents.
  
Arguments:

  FileName    - pointer to linked list of input pack file names

Returns:

  STATUS_SUCCESS    - if successful
  STATUS_ERROR      - otherwise
  
--*/
{
  FILE                *InFptr;
  char                *Buffer;
  long                BufferSize;
  STATUS              Status;
  STATUS              IfrStatus;
  int                 Handle;
  EFI_GUID            FormsetGuid;
  EFI_HII_PACK_HEADER *PackHeader;
  //
  // Process each input IFR file
  //
  Status  = STATUS_ERROR;
  Handle  = 1;
  InFptr  = NULL;
  Buffer  = NULL;
  while (FileName != NULL) {
    //
    // Only process IFR pack files
    //
    if (FileName->Tag != EFI_HII_IFR) {
      FileName = FileName->Next;
      continue;
    }
    //
    // Open the input file, then read the contents
    //
    if ((InFptr = fopen (FileName->FileName, "rb")) == NULL) {
      Error (NULL, 0, 0, FileName->FileName, "failed to open input IFR file");
      goto Done;
    }

    fseek (InFptr, 0, SEEK_END);
    BufferSize = ftell (InFptr);
    fseek (InFptr, 0, SEEK_SET);
    Buffer = (char *) malloc (BufferSize);
    if (Buffer == NULL) {
      Error (NULL, 0, 0, "memory allocation failure", NULL);
      goto Done;
    }

    if (fread (Buffer, BufferSize, 1, InFptr) != 1) {
      Error (NULL, 0, 0, FileName->FileName, "failed to read file contents");
      goto Done;
    }

    fclose (InFptr);
    InFptr = NULL;
    //
    // Check the buffer contents -- better be an IFR pack
    //
    if (BufferSize < sizeof (EFI_HII_PACK_HEADER)) {
      Error (NULL, 0, 0, FileName->FileName, "file is not large enough to contain an IFR pack");
      goto Done;
    }

    PackHeader = (EFI_HII_PACK_HEADER *) Buffer;
    if (PackHeader->Type != EFI_HII_IFR) {
      Error (NULL, 0, 0, FileName->FileName, "file does not appear to be an IFR pack");
      goto Done;
    }
    //
    // Process the contents
    //
    memset ((void *) &FormsetGuid, 0, sizeof (EFI_GUID));
    IfrStatus = IfrParsePack (Handle, (EFI_HII_IFR_PACK *) PackHeader, &FormsetGuid);
    if (IfrStatus != STATUS_SUCCESS) {
      goto Done;
    }

    Handle++;
    free (Buffer);
    Buffer    = NULL;
    FileName  = FileName->Next;
  }

  Status = STATUS_SUCCESS;
Done:
  if (InFptr != NULL) {
    fclose (InFptr);
  }

  if (Buffer != NULL) {
    free (Buffer);
  }

  return Status;
}

static
STATUS
EmitDefaults (
  FILE_NAME_LIST  *HiiExportFiles,
  int             MfgDefaults,
  int             NoEmptyVarPacks
  )
/*++

Routine Description:

  Given a linked list of HII export files, read in each file,
  process the contents, and then emit variable packs.
  
Arguments:

  HiiExportFiles  - linked list of HII export files to process
  MfgDefaults     - emit manufacturing defaults
  NoEmptyVarPacks - don't emit variable packs if they are 0-length

Returns:

  STATUS_SUCCESS    - if successful
  STATUS_ERROR      - otherwise
  
--*/
{
  int                   HiiHandle;
  FILE                  *OutFptr;
  FILE                  *InFptr;
  EFI_HII_VARIABLE_PACK *VarPack;
  CHAR8                 OutFileName[MAX_PATH];
  CHAR8                 GuidString[100];
  STATUS                Status;
  CHAR8                 *Buffer;
  int                   FileSize;
  int                   DataTableIndex;
  EFI_HII_EXPORT_TABLE  *HiiExportTableHeader;
  EFI_HII_DATA_TABLE    *DataTableHeader;
  //
  // Init locals
  //
  HiiHandle = FIRST_HII_PACK_HANDLE;
  Buffer    = NULL;
  InFptr    = NULL;
  OutFptr   = NULL;
  Status    = STATUS_ERROR;
  //
  // Initialize our IFR parser
  //
  IfrParseInit ();
  //
  // Process each input HII export file
  //
  while (HiiExportFiles != NULL) {
    if (mGlobals.Verbose) {
      fprintf (stdout, "Processing file %s\n", HiiExportFiles->FileName);
    }
    //
    // Read in the entire file contents
    //
    if ((InFptr = fopen (HiiExportFiles->FileName, "rb")) == NULL) {
      Error (NULL, 0, 0, HiiExportFiles->FileName, "failed to open HII export file for reading");
      goto Done;
    }

    fseek (InFptr, 0, SEEK_END);
    FileSize = (int) ftell (InFptr);
    fseek (InFptr, 0, SEEK_SET);
    Buffer = (CHAR8 *) malloc (FileSize);
    if (Buffer == NULL) {
      Error (NULL, 0, 0, "memory allocation failure", NULL);
      goto Done;
    }

    if (fread (Buffer, FileSize, 1, InFptr) != 1) {
      Error (NULL, 0, 0, HiiExportFiles->FileName, "failed to read file contents");
      goto Done;
    }

    fclose (InFptr);
    InFptr                = NULL;
    HiiExportTableHeader  = (EFI_HII_EXPORT_TABLE *) Buffer;
    //
    // Walk all the data tables
    //
    DataTableHeader = (EFI_HII_DATA_TABLE *) (HiiExportTableHeader + 1);
    for (DataTableIndex = 0; DataTableIndex < (int) HiiExportTableHeader->NumberOfHiiDataTables; DataTableIndex++) {
      //
      // Make sure we're still pointing into our buffer
      //
      if (((char *) DataTableHeader < Buffer) || ((char *) DataTableHeader > Buffer + FileSize)) {
        Error (NULL, 0, 0, "bad data table size in input file", NULL);
        goto Done;
      }
      //
      // If there is an IFR pack, parse it
      //
      HiiHandle++;
      if (DataTableHeader->IfrDataOffset != 0) {
        if (IfrParsePack (
            HiiHandle,
            (EFI_HII_IFR_PACK *) ((char *) DataTableHeader + DataTableHeader->IfrDataOffset),
            &DataTableHeader->PackageGuid
            ) != STATUS_SUCCESS
            ) {
          goto Done;
        }
      }
      //
      // Next data pack
      //
      DataTableHeader = (EFI_HII_DATA_TABLE *) ((char *) DataTableHeader + DataTableHeader->DataTableSize);
    }

    free (Buffer);
    Buffer = NULL;
    //
    // Next input file
    //
    HiiExportFiles = HiiExportFiles->Next;
  }
  //
  // Now create defaults
  //
  if (IfrSetDefaults (MfgDefaults) != STATUS_SUCCESS) {
    goto Done;
  }
  //
  // Now retrieve each variable pack and write it out to a GUID-VarName.hpk file
  //
  HiiHandle = 0;
  do {
    Status = IfrGetVarPack (HiiHandle, &VarPack);
    if (Status == STATUS_SUCCESS) {
      //
      // Check for variable data length of 0
      //
      if ((NoEmptyVarPacks == 0) ||
          ((VarPack->Header.Length - sizeof (EFI_HII_VARIABLE_PACK) - VarPack->VariableNameLength) != 0)
          ) {
        //
        // Open the output file and write the variable pack
        //
        GuidToString (&VarPack->VariableGuid, GuidString);
        if (MfgDefaults) {
          sprintf (
            OutFileName,
            "%s-%S-MfgDefaults%s",
            GuidString,
            (CHAR16 *) (VarPack + 1),
            DEFAULT_HII_PACK_FILENAME_EXTENSION
            );
        } else {
          sprintf (
            OutFileName,
            "%s-%S-Defaults%s",
            GuidString,
            (CHAR16 *) (VarPack + 1),
            DEFAULT_HII_PACK_FILENAME_EXTENSION
            );
        }

        if (mGlobals.Verbose) {
          fprintf (
            stdout,
            "Creating %svariable defaults pack file %s\n",
            MfgDefaults ? "manufacturing " : "",
            OutFileName
            );
        }

        if ((OutFptr = fopen (OutFileName, "wb")) == NULL) {
          Error (NULL, 0, 0, OutFileName, "failed to open output file for writing", NULL);
          goto Done;
        }

        if (fwrite ((void *) VarPack, VarPack->Header.Length, 1, OutFptr) != 1) {
          Error (NULL, 0, 0, OutFileName, "failed to write defaults to output file");
          goto Done;
        }

        fclose (OutFptr);
        OutFptr = NULL;
      } else {
        //
        // Print a message that we skipped one if in verbose mode
        //
        if (mGlobals.Verbose) {
          GuidToString (&VarPack->VariableGuid, GuidString);
          if (MfgDefaults) {
            sprintf (
              OutFileName,
              "%s-%S-MfgDefaults%s",
              GuidString,
              (CHAR16 *) (VarPack + 1),
              DEFAULT_HII_PACK_FILENAME_EXTENSION
              );
          } else {
            sprintf (
              OutFileName,
              "%s-%S-Defaults%s",
              GuidString,
              (CHAR16 *) (VarPack + 1),
              DEFAULT_HII_PACK_FILENAME_EXTENSION
              );
          }

          fprintf (
            stdout,
            "Skipping 0-length %svariable defaults pack file %s\n",
            MfgDefaults ? "manufacturing " : "",
            OutFileName
            );
        }
      }
    }

    HiiHandle++;
  } while (Status == STATUS_SUCCESS);
  Status = STATUS_SUCCESS;
Done:
  IfrParseEnd ();
  if (Buffer != NULL) {
    free (Buffer);
  }

  if (InFptr != NULL) {
    fclose (InFptr);
  }

  if (OutFptr != NULL) {
    fclose (OutFptr);
  }

  return Status;
}

static
void
FreeGlobals (
  VOID
  )
/*++

Routine Description:

  Free up an memory we allocated so we can exit cleanly
  
Arguments:

Returns: NA

--*/
{
  FILE_NAME_LIST  *Next;
  //
  // Free up input pack file names
  //
  while (mGlobals.PackFileNames != NULL) {
    Next = mGlobals.PackFileNames->Next;
    free (mGlobals.PackFileNames);
    mGlobals.PackFileNames = Next;
  }
  //
  // Free up input HII export file names
  //
  while (mGlobals.HiiExportFileNames != NULL) {
    Next = mGlobals.HiiExportFileNames->Next;
    free (mGlobals.HiiExportFileNames);
    mGlobals.HiiExportFileNames = Next;
  }
}

static
STATUS
DumpHiiExportFile (
  char    *HiiExportFileName,
  char    *OutputFileName
  )
/*++

Routine Description:

  Dump the contents of an HII export file for debug purposes
  
Arguments:

  HiiExportFileName - name of input HII export file
  OutputFileName    - name of output file to dump contents

Returns: 
  STATUS_SUCCESS  - no problems
  STATUS_ERROR    - problems encountered processing the file

--*/
{
  FILE                *InFptr;

  FILE                *OutFptr;
  char                *Buffer;
  char                *BufferStart;
  char                *BufferEnd;
  int                 BufferSize;
  STATUS              Status;
  char                GuidString[100];
  int                 Counter;
  int                 NumberOfTables;
  EFI_HII_DATA_TABLE  *DataTableHeader;
  EFI_GUID              HiiExportRevisionGuid = EFI_HII_PROTOCOL_GUID;
  //
  // Init locals
  //
  InFptr      = NULL;
  OutFptr     = NULL;
  BufferStart = NULL;
  Status      = STATUS_ERROR;
  //
  // Open the input file
  //
  if ((InFptr = fopen (HiiExportFileName, "rb")) == NULL) {
    Error (NULL, 0, 0, HiiExportFileName, "failed to open input HII export file for reading");
    goto Done;
  }
  //
  // Open the output file
  //
  if ((OutFptr = fopen (OutputFileName, "w")) == NULL) {
    Error (NULL, 0, 0, OutputFileName, "failed to open output file for writing");
    goto Done;
  }
  //
  // Get the file size, then allocate a buffer and read in the file contents.
  //
  fseek (InFptr, 0, SEEK_END);
  BufferSize = (int) ftell (InFptr);
  fseek (InFptr, 0, SEEK_SET);
  BufferStart = (char *) malloc (BufferSize);
  if (BufferStart == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    goto Done;
  }

  if (fread (BufferStart, BufferSize, 1, InFptr) != 1) {
    Error (NULL, 0, 0, HiiExportFileName, "error reading file contents");
    goto Done;
  }

  fclose (InFptr);
  InFptr = NULL;
  //
  // Crude check of the input data -- check the size and GUID
  //
  if (BufferSize < sizeof (EFI_HII_EXPORT_TABLE)) {
    Error (NULL, 0, 0, HiiExportFileName, "files not large enough to contain an HII export table header");
    goto Done;
  }

  if (memcmp (&((EFI_HII_EXPORT_TABLE *) BufferStart)->Revision, &HiiExportRevisionGuid, sizeof (EFI_GUID)) != 0) {
    Error (NULL, 0, 0, HiiExportFileName, "invalid HII export revision GUID -- is this an HII export file?");
    //
    // See if it's a HII pack file
    //
    TestDumpHiiPack (OutFptr, BufferStart, BufferSize);
    goto Done;
  }
  //
  // Now walk the export data
  //
  Buffer    = BufferStart;
  BufferEnd = BufferStart + BufferSize;
  //
  // Dump the header
  //
  fprintf (OutFptr, "HII dump of file %s\n\n", HiiExportFileName);
  NumberOfTables = ((EFI_HII_EXPORT_TABLE *) Buffer)->NumberOfHiiDataTables;
  fprintf (OutFptr, "Number of data tables:  %d\n", NumberOfTables);
  GuidToString (&((EFI_HII_EXPORT_TABLE *) Buffer)->Revision, GuidString);
  fprintf (OutFptr, "HII export revision:    %s\n", GuidString);
  //
  // Now walk the data tables
  //
  Buffer += sizeof (EFI_HII_EXPORT_TABLE);
  for (Counter = 0; Counter < NumberOfTables; Counter++) {
    DataTableHeader = (EFI_HII_DATA_TABLE *) Buffer;
    fprintf (OutFptr, "----------------------------------------------------------\n");
    fprintf (OutFptr, "  DataTable at offset 0x%08X\n", (int) Buffer - (int) BufferStart);
    fprintf (OutFptr, "    HII Handle:                            0x%08X\n", DataTableHeader->HiiHandle);
    GuidToString (&DataTableHeader->PackageGuid, GuidString);
    fprintf (OutFptr, "    Package GUID:                          %s\n", GuidString);
    fprintf (OutFptr, "    Data table size:                       0x%08X\n", DataTableHeader->DataTableSize);
    fprintf (OutFptr, "    IFR data offset:                       0x%08X\n", DataTableHeader->IfrDataOffset);
    fprintf (OutFptr, "    String data offset:                    0x%08X\n", DataTableHeader->StringDataOffset);
    fprintf (OutFptr, "    Variable data offset:                  0x%08X\n", DataTableHeader->VariableDataOffset);
    fprintf (OutFptr, "    Device path offset:                    0x%08X\n", DataTableHeader->DevicePathOffset);
    fprintf (OutFptr, "    Number of variable data:               0x%08X\n", DataTableHeader->NumberOfVariableData);
    fprintf (OutFptr, "    Number of languages:                   0x%08X\n", DataTableHeader->NumberOfLanguages);
    //
    // Dump strings
    //
    if (DataTableHeader->StringDataOffset != 0) {
      DumpStringPack (
        OutFptr,
        (EFI_HII_STRING_PACK *) ((char *) DataTableHeader + DataTableHeader->StringDataOffset),
        DataTableHeader->StringDataOffset,
        6
        );
    }
    //
    // Dump IFR
    //
    if (DataTableHeader->IfrDataOffset != 0) {
      DumpIfrPack (
        OutFptr,
        (EFI_HII_IFR_PACK *) ((char *) DataTableHeader + DataTableHeader->IfrDataOffset),
        DataTableHeader->IfrDataOffset,
        6
        );
    }
    //
    // Dump variables
    //
    if (DataTableHeader->VariableDataOffset != 0) {
      DumpVariablePacks (
        OutFptr,
        (EFI_HII_VARIABLE_PACK *) ((char *) DataTableHeader + DataTableHeader->VariableDataOffset),
        DataTableHeader->NumberOfVariableData,
        DataTableHeader->VariableDataOffset,
        6
        );
    }
    //
    // Dump device path
    //
    //
    // Check position before advancing
    //
    if ((Buffer + DataTableHeader->DataTableSize < Buffer) || (Buffer + DataTableHeader->DataTableSize > BufferEnd)) {
      Error (NULL, 0, 0, HiiExportFileName, "bad data table size at offset 0x%X", (int) Buffer - (int) BufferStart);
      goto Done;
    }

    Buffer += DataTableHeader->DataTableSize;
  }

  Status = STATUS_SUCCESS;
Done:
  if (OutFptr != NULL) {
    fclose (OutFptr);
  }

  if (InFptr != NULL) {
    fclose (InFptr);
  }

  if (BufferStart != NULL) {
    free (BufferStart);
  }

  return Status;
}

static
void
DumpIfrPack (
  FILE                  *OutFptr,
  EFI_HII_IFR_PACK      *Pack,
  int                   BaseOffset,
  int                   Indent
  )
/*++

Routine Description:

  Dump the contents of an IFR pack for debug purposes
  
Arguments:

  OutFptr         - file pointer to which to dump the output
  Pack            - pointer to IFR pack to dump
  BaseOffset      - offset from which Pack starts in its parent data table
  Indent          - indent this many spaces when printing text to OutFptr

Returns: 
  NA

--*/
{
  EFI_IFR_FORM_SET  *IfrFormSet;
  char              GuidString[100];
  if (Pack->Header.Type != EFI_HII_IFR) {
    Error (NULL, 0, 0, "found non-IFR pack type at IFR data offset", NULL);
    return ;
  }

  fprintf (OutFptr, "%*cIFR pack at offset      0x%08X\n", Indent, ' ', BaseOffset);
  fprintf (OutFptr, "%*c  Length                0x%08X\n", Indent, ' ', Pack->Header.Length);
  //
  // Get the GUID from the formset
  //
  IfrFormSet = (EFI_IFR_FORM_SET *) (Pack + 1);
  GuidToString (&IfrFormSet->Guid, GuidString);
  fprintf (OutFptr, "%*c  Variable GUID         %s\n", Indent, ' ', GuidString);
  //
  // Print the IFR formset size, with a note indicating if it's a min (likely stub)
  // formset
  //
  if (Pack->Header.Length == sizeof (EMPTY_FORMSET_PACK)) {
    fprintf (
      OutFptr,
      "%*c  IFR formset size      0x%08X (empty formset)\n",
      Indent,
      ' ',
      Pack->Header.Length - sizeof (EFI_HII_IFR_PACK)
      );
  } else {
    fprintf (
      OutFptr,
      "%*c  IFR formset size      0x%08X\n",
      Indent,
      ' ',
      Pack->Header.Length - sizeof (EFI_HII_IFR_PACK)
      );
  }
  //
  // Dump raw bytes -- not much use
  //
}

static
void
DumpVariablePacks (
  FILE                  *OutFptr,
  EFI_HII_VARIABLE_PACK *Pack,
  int                   NumPacks,
  int                   BaseOffset,
  int                   Indent
  )
/*++

Routine Description:

  Dump the contents of an IFR pack for debug purposes
  
Arguments:

  OutFptr         - file pointer to which to dump the output
  Pack            - pointer to variable pack to dump
  NumPacks        - number of packs in Pack[] array
  BaseOffset      - offset from which Pack starts in its parent data table
  Indent          - indent this many spaces when printing text to OutFptr

Returns: 
  NA

--*/
{
  int   Count;

  int   Len;
  char  GuidString[100];

  for (Count = 0; Count < NumPacks; Count++) {
    if (Pack->Header.Type != EFI_HII_VARIABLE) {
      Error (NULL, 0, 0, "found non-variable pack type in variable pack array", NULL);
      return ;
    }

    fprintf (OutFptr, "%*cVariable pack at offset 0x%08X\n", Indent, ' ', BaseOffset);
    fprintf (OutFptr, "%*c  Length                0x%08X\n", Indent, ' ', Pack->Header.Length);
    GuidToString (&Pack->VariableGuid, GuidString);
    fprintf (OutFptr, "%*c  Variable GUID         %s\n", Indent, ' ', GuidString);
    fprintf (OutFptr, "%*c  Variable Name         %S\n", Indent, ' ', (CHAR16 *) (Pack + 1));
    Len = sizeof (EFI_HII_VARIABLE_PACK) + Pack->VariableNameLength;
    fprintf (OutFptr, "%*c  Variable Size         0x%08X\n", Indent, ' ', Pack->Header.Length - Len);
    //
    // Dump raw bytes
    //
    DumpRawBytes (OutFptr, (char *) Pack + Len, Pack->Header.Length - Len, Len, Indent + 2);
    BaseOffset += Pack->Header.Length;
    Pack = (EFI_HII_VARIABLE_PACK *) ((char *) Pack + Pack->Header.Length);
  }
}

static
void
DumpStringPack (
  FILE                  *OutFptr,
  EFI_HII_STRING_PACK   *Pack,
  int                   BaseOffset,
  int                   Indent
  )
/*++

Routine Description:

  Dump the contents of a string pack array for debug purposes
  
Arguments:

  OutFptr         - file pointer to which to dump the output
  Pack            - pointer to string pack array to dump
  BaseOffset      - offset from which Pack starts in its parent data table
  Indent          - indent this many spaces when printing text to OutFptr

Returns: 
  NA

--*/
{
  int     Count;
  int     *IndexPtr;
  CHAR16  *WCPtr;
  //
  // String pack array is terminated with a zero-length string pack
  //
  while (Pack->Header.Length > 0) {
    if (Pack->Header.Type != EFI_HII_STRING) {
      Error (NULL, 0, 0, "found non-string pack type in string pack array", NULL);
      return ;
    }

    fprintf (OutFptr, "%*cString pack at offset   0x%08X\n", Indent, ' ', BaseOffset);
    fprintf (OutFptr, "%*c  Length                0x%08X\n", Indent, ' ', Pack->Header.Length);
    fprintf (
      OutFptr,
      "%*c  Language              %S\n",
      Indent,
      ' ',
      (CHAR16 *) ((char *) Pack + Pack->LanguageNameString)
      );
    fprintf (
      OutFptr,
      "%*c  Printable Language    %S\n",
      Indent,
      ' ',
      (CHAR16 *) ((char *) Pack + Pack->PrintableLanguageName)
      );
    fprintf (OutFptr, "%*c  Number of strings     0x%08X\n", Indent, ' ', Pack->NumStringPointers);
    fprintf (OutFptr, "%*c  Attributes            0x%08X\n", Indent, ' ', Pack->Attributes);
    IndexPtr = (int *) (Pack + 1);
    //
    // Dump string data
    //
    if (mGlobals.DumpStrings) {
      for (Count = 0; Count < (int) Pack->NumStringPointers; Count++) {
        fprintf (OutFptr, "%*c    String 0x%04X: ", Indent, ' ', Count);
        //
        // Print raw hex bytes
        //
        for (WCPtr = (CHAR16 *) ((char *) Pack +*IndexPtr); *WCPtr != 0; WCPtr++) {
          fprintf (OutFptr, "%02X ", (unsigned int) *WCPtr);
        }

        fprintf (OutFptr, "00\n");
        IndexPtr++;
      }
    }

    BaseOffset += Pack->Header.Length;
    Pack = (EFI_HII_STRING_PACK *) ((char *) Pack + Pack->Header.Length);
  }
}

static
void
TestDumpHiiPack (
  FILE    *OutFptr,
  char    *Buffer,
  int     BufferSize
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  OutFptr     - GC_TODO: add argument description
  Buffer      - GC_TODO: add argument description
  BufferSize  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  EFI_HII_PACK_HEADER *PackHeader;

  PackHeader = (EFI_HII_PACK_HEADER *) Buffer;
  //
  // Check size match
  //
  if (PackHeader->Length != (unsigned int) BufferSize) {
    return ;
  }
  //
  // Check type
  //
  switch (PackHeader->Type) {
  case EFI_HII_STRING:
    fprintf (stdout, "Dumping as string pack\n");
    DumpStringPack (OutFptr, (EFI_HII_STRING_PACK *) Buffer, 0, 2);
    break;

  case EFI_HII_IFR:
    fprintf (stdout, "Dumping as IFR pack\n");
    DumpIfrPack (OutFptr, (EFI_HII_IFR_PACK *) Buffer, 0, 2);
    break;

  case EFI_HII_VARIABLE:
    fprintf (stdout, "Dumping as IFR pack\n");
    DumpVariablePacks (OutFptr, (EFI_HII_VARIABLE_PACK *) Buffer, 1, 0, 2);
    break;
  }
}

static
void
DumpRawBytes (
  FILE                  *OutFptr,
  char                  *Buffer,
  int                   Count,
  int                   BaseOffset,
  int                   Indent
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  OutFptr     - GC_TODO: add argument description
  Buffer      - GC_TODO: add argument description
  Count       - GC_TODO: add argument description
  BaseOffset  - GC_TODO: add argument description
  Indent      - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  int Counter;

  for (Counter = 0; Counter < Count; Counter++) {
    if ((Counter & 0xF) == 0) {
      if (Counter != 0) {
        fprintf (OutFptr, "\n%*c%08X ", Indent, ' ', Counter);
      } else {
        fprintf (OutFptr, "\n%*c%08X ", Indent, ' ', Counter);
      }
    }

    fprintf (OutFptr, "%02X ", (unsigned int) (unsigned char) *Buffer);
    Buffer++;
  }

  fprintf (OutFptr, "\n");
}

void
GuidToString (
  EFI_GUID   *Guid,
  char       *Str
  )
/*++

Routine Description:

  Given a pointer to a GUID, sprint the value into a string
  
Arguments:

  Guid   - pointer to input GUID
  Str    - pointer to outgoing printed GUID value

Returns:
  NA
  
--*/
{
  sprintf (
    Str,
    "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
    Guid->Data1,
    Guid->Data2,
    Guid->Data3,
    Guid->Data4[0],
    Guid->Data4[1],
    Guid->Data4[2],
    Guid->Data4[3],
    Guid->Data4[4],
    Guid->Data4[5],
    Guid->Data4[6],
    Guid->Data4[7]
    );
}

int
FindFilesCallback (
  char *FoundFileName
  )
/*++

Routine Description:

  Callback function used to get files matching a file mask. This
  function is called when the command-line arguments to this utility
  are parsed and the user specified "-s Path FileMask" to process
  all HII export files in Path and its subdirectories that match
  FileMask.
  
Arguments:

  FoundFileName - name of file found.

Returns:
  non-zero    - caller should halt processing
  zero        - no problems while processing FoundFileName
  
--*/
{
  FILE_NAME_LIST  *FileName;

  FILE_NAME_LIST  *TempFileName;

  FileName = (FILE_NAME_LIST *) malloc (sizeof (FILE_NAME_LIST));
  if (FileName == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    return STATUS_ERROR;
  }

  memset ((void *) FileName, 0, sizeof (FILE_NAME_LIST));
  strcpy (FileName->FileName, FoundFileName);
  if (mGlobals.HiiExportFileNames == NULL) {
    mGlobals.HiiExportFileNames = FileName;
  } else {
    //
    // Add to the end of the list
    //
    for (TempFileName = mGlobals.HiiExportFileNames; TempFileName->Next != NULL; TempFileName = TempFileName->Next)
      ;
    TempFileName->Next = FileName;
  }

  return 0;
}

static
STATUS
ProcessArgs (
  int   Argc,
  char  *Argv[]
  )
/*++

Routine Description:

  Process the command line arguments
  
Arguments:

  As per standard C main()

Returns:

  STATUS_SUCCESS    - if successful
  STATUS_ERROR      - otherwise
  
--*/
// GC_TODO:    Argc - add argument and description to function comment
// GC_TODO:    ] - add argument and description to function comment
{
  FILE_NAME_LIST      *FileName;

  FILE_NAME_LIST      *TempFileName;
  FILE                *InFptr;
  EFI_HII_PACK_HEADER PackHeader;

  memset ((void *) &mGlobals, 0, sizeof (mGlobals));
  //
  // Skip program name
  //
  Argc--;
  Argv++;

  if (Argc == 0) {
    Usage ();
    return STATUS_ERROR;
  }
  //
  // First arg must be one of create, merge, defaults, or dump
  //
  if (_stricmp (Argv[0], "create") == 0) {
    mGlobals.Mode = MODE_CREATE_HII_EXPORT;
  } else if (_stricmp (Argv[0], "merge") == 0) {
    mGlobals.Mode = MODE_MERGE_HII_EXPORTS;
  } else if (_stricmp (Argv[0], "defaults") == 0) {
    mGlobals.Mode = MODE_EMIT_DEFAULTS;
  } else if (_stricmp (Argv[0], "dump") == 0) {
    mGlobals.Mode = MODE_DUMP_HII_EXPORT;
  } else if (strcmp (Argv[0], "-?") == 0) {
    Usage ();
    return STATUS_ERROR;
  } else {
    Error (NULL, 0, 0, Argv[0], "unrecognized mode");
    return STATUS_ERROR;
  }

  Argv++;
  Argc--;
  //
  // Process until no more args.
  //
  while (Argc > 0) {
    if (_stricmp (Argv[0], "-o") == 0) {
      //
      // -o option to specify the output file
      //
      if ((Argc <= 1) || (Argv[1][0] == '-')) {
        Error (UTILITY_NAME, 0, 0, Argv[0], "missing output file name");
        return STATUS_ERROR;
      }

      if (mGlobals.OutputFileName[0] == 0) {
        mGlobals.OutputFileName[MAX_PATH - 1] = 0;
        strncpy (mGlobals.OutputFileName, Argv[1], MAX_PATH - 1);
      } else {
        Error (UTILITY_NAME, 0, 0, Argv[1], "-o option already specified with '%s'", mGlobals.OutputFileName);
        return STATUS_ERROR;
      }

      Argv++;
      Argc--;
    } else if (_stricmp (Argv[0], "-mfg") == 0) {
      mGlobals.MfgFlag = 1;
    } else if (_stricmp (Argv[0], "-g") == 0) {
      //
      // -g option to specify the guid
      //
      if ((Argc <= 1) || (Argv[1][0] == '-')) {
        Error (UTILITY_NAME, 0, 0, Argv[0], "missing GUID");
        return STATUS_ERROR;
      }

      StringToGuid (Argv[1], &mGlobals.Guid);
      mGlobals.GuidSpecified = 1;
      Argv++;
      Argc--;
    } else if (_stricmp (Argv[0], "-v") == 0) {
      mGlobals.Verbose = 1;
    } else if (_stricmp (Argv[0], "-p") == 0) {
      //
      // -p option to specify an input pack file. Only valid for 'create' mode
      //
      if (mGlobals.Mode != MODE_CREATE_HII_EXPORT) {
        Error (NULL, 0, 0, Argv[0], "option only valid in 'create' mode");
        return STATUS_ERROR;
      }

      if ((Argc <= 1) || (Argv[1][0] == '-')) {
        Error (UTILITY_NAME, 0, 0, Argv[0], "missing pack file name");
        return STATUS_ERROR;
      }
      //
      // Consume arguments until next -arg or end
      //
      do {
        Argv++;
        Argc--;
        //
        // Open the file, read the pack header, and figure out what type of
        // HII pack it is.
        //
        if ((InFptr = fopen (Argv[0], "rb")) == NULL) {
          Error (NULL, 0, 0, Argv[0], "failed to open input HII pack file for reading");
          return STATUS_ERROR;
        }

        if (fread (&PackHeader, sizeof (EFI_HII_PACK_HEADER), 1, InFptr) != 1) {
          Error (NULL, 0, 0, Argv[0], "failed to read pack header from input HII pack file");
          fclose (InFptr);
          return STATUS_ERROR;
        }

        fclose (InFptr);
        if ((PackHeader.Type != EFI_HII_STRING) &&
            (PackHeader.Type != EFI_HII_IFR) &&
            (PackHeader.Type != EFI_HII_VARIABLE)
            ) {
          Error (NULL, 0, 0, Argv[0], "unsupported HII pack type 0x%X", (unsigned int) PackHeader.Type);
          return STATUS_ERROR;
        }
        //
        // Add this file name to our list of pack files
        //
        FileName = (FILE_NAME_LIST *) malloc (sizeof (FILE_NAME_LIST));
        if (FileName == NULL) {
          Error (NULL, 0, 0, "memory allocation failure", NULL);
          return STATUS_ERROR;
        }

        memset ((void *) FileName, 0, sizeof (FILE_NAME_LIST));
        FileName->Tag = (int) PackHeader.Type;
        strcpy (FileName->FileName, Argv[0]);
        if (mGlobals.PackFileNames == NULL) {
          mGlobals.PackFileNames = FileName;
        } else {
          //
          // Add to the end of the list
          //
          for (TempFileName = mGlobals.PackFileNames; TempFileName->Next != NULL; TempFileName = TempFileName->Next)
            ;
          TempFileName->Next = FileName;
        }
      } while ((Argc > 1) && (Argv[1][0] != '-'));
    } else if (_stricmp (Argv[0], "-noemptyvarpacks") == 0) {
      mGlobals.NoEmptyVarPacks = 1;
    } else if (_stricmp (Argv[0], "-novarpacks") == 0) {
      mGlobals.NoVarPacks = 1;
    } else if (_stricmp (Argv[0], "-x") == 0) {
      //
      // -x option to specify an input HII export file name. Not valid for 'create' mode
      //
      if (mGlobals.Mode == MODE_CREATE_HII_EXPORT) {
        Error (NULL, 0, 0, Argv[0], "option is not valid in 'create' mode");
        return STATUS_ERROR;
      }

      if ((Argc <= 1) || (Argv[1][0] == '-')) {
        Error (UTILITY_NAME, 0, 0, Argv[0], "missing HII export input file name");
        return STATUS_ERROR;
      }
      //
      // Consume arguments until next -arg or end
      //
      do {
        Argv++;
        Argc--;
        //
        // Add this file name to our list of export files
        //
        FileName = (FILE_NAME_LIST *) malloc (sizeof (FILE_NAME_LIST));
        if (FileName == NULL) {
          Error (NULL, 0, 0, "memory allocation failure", NULL);
          return STATUS_ERROR;
        }

        memset ((void *) FileName, 0, sizeof (FILE_NAME_LIST));
        strcpy (FileName->FileName, Argv[0]);
        if (mGlobals.HiiExportFileNames == NULL) {
          mGlobals.HiiExportFileNames = FileName;
        } else {
          //
          // Add to the end of the list
          //
          for (TempFileName = mGlobals.HiiExportFileNames;
               TempFileName->Next != NULL;
               TempFileName = TempFileName->Next
              )
            ;
          TempFileName->Next = FileName;
        }
      } while ((Argc > 1) && (Argv[1][0] != '-'));
    } else if (_stricmp (Argv[0], "-dumpstrings") == 0) {
      mGlobals.DumpStrings = 1;
    } else if (_stricmp (Argv[0], "-s") == 0) {
      //
      // -s option to specify input HII export files using a path and file mask.
      // Only valid in merge mode
      //
      if (mGlobals.Mode != MODE_MERGE_HII_EXPORTS) {
        Error (NULL, 0, 0, Argv[0], "option only valid in 'merge' mode");
        return STATUS_ERROR;
      }

      if ((Argc <= 1) || (Argv[1][0] == '-')) {
        Error (UTILITY_NAME, 0, 0, Argv[0], "missing root directory name");
        return STATUS_ERROR;
      }

      if ((Argc <= 2) || (Argv[2][0] == '-')) {
        Error (UTILITY_NAME, 0, 0, Argv[0], "missing file mask");
        return STATUS_ERROR;
      }
      //
      // Call our function to process the directory and file mask. If
      // the directory does not start with c:\, then prepend cwd to it.
      //
      if (FindFiles (Argv[1], Argv[2], FindFilesCallback)) {
        Error (NULL, 0, 0, "failed to process matching files", "%s\\%s", Argv[1], Argv[2]);
        return STATUS_ERROR;
      }

      Argv += 2;
      Argc -= 2;
    } else if (_stricmp (Argv[0], "-p") == 0) {
      //
      // -p option to specify an input pack file. Only valid for 'create' mode
      //
      if (mGlobals.Mode != MODE_CREATE_HII_EXPORT) {
        Error (NULL, 0, 0, Argv[0], "option only valid in 'create' mode");
        return STATUS_ERROR;
      }

      if ((Argc <= 1) || (Argv[1][0] == '-')) {
        Error (UTILITY_NAME, 0, 0, Argv[0], "missing pack file name");
        return STATUS_ERROR;
      }
      //
      // Consume arguments until next -arg or end
      //
      do {
        Argv++;
        Argc--;
        //
        // Open the file, read the pack header, and figure out what type of
        // HII pack it is.
        //
        if ((InFptr = fopen (Argv[0], "rb")) == NULL) {
          Error (NULL, 0, 0, Argv[0], "failed to open input HII pack file for reading");
          return STATUS_ERROR;
        }

        if (fread (&PackHeader, sizeof (EFI_HII_PACK_HEADER), 1, InFptr) != 1) {
          Error (NULL, 0, 0, Argv[0], "failed to read pack header from input HII pack file");
          fclose (InFptr);
          return STATUS_ERROR;
        }

        fclose (InFptr);
        if ((PackHeader.Type != EFI_HII_STRING) &&
            (PackHeader.Type != EFI_HII_IFR) &&
            (PackHeader.Type != EFI_HII_VARIABLE)
            ) {
          Error (NULL, 0, 0, Argv[0], "unsupported HII pack type 0x%X", (unsigned int) PackHeader.Type);
          return STATUS_ERROR;
        }
        //
        // Add this file name to our list of pack files
        //
        FileName = (FILE_NAME_LIST *) malloc (sizeof (FILE_NAME_LIST));
        if (FileName == NULL) {
          Error (NULL, 0, 0, "memory allocation failure", NULL);
          return STATUS_ERROR;
        }

        memset ((void *) FileName, 0, sizeof (FILE_NAME_LIST));
        FileName->Tag = (int) PackHeader.Type;
        strcpy (FileName->FileName, Argv[0]);
        if (mGlobals.PackFileNames == NULL) {
          mGlobals.PackFileNames = FileName;
        } else {
          //
          // Add to the end of the list
          //
          for (TempFileName = mGlobals.PackFileNames; TempFileName->Next != NULL; TempFileName = TempFileName->Next)
            ;
          TempFileName->Next = FileName;
        }
      } while ((Argc > 1) && (Argv[1][0] != '-'));
    } else {
      Error (NULL, 0, 0, Argv[0], "unrecognized option");
      return STATUS_ERROR;
    }

    Argv++;
    Argc--;
  }
  //
  // All modes except 'defaults' requires an output file name
  //
  if (mGlobals.Mode != MODE_EMIT_DEFAULTS) {
    if (mGlobals.OutputFileName[0] == 0) {
      Error (NULL, 0, 0, "must specify '-o OutputFileName'", NULL);
      return STATUS_ERROR;
    }
    //
    // If merging, then you have to specify at least one HII export files.
    // We support specifying only one file in case you want to take an export file
    // and emit a copy with different (for example, manufacturing) defaults.
    //
    if (mGlobals.Mode == MODE_MERGE_HII_EXPORTS) {
      if (mGlobals.HiiExportFileNames == NULL) {
        Error (NULL, 0, 0, "must specify at least one HII export file in 'merge' mode", NULL);
        return STATUS_ERROR;
      }
    } else if (mGlobals.Mode == MODE_CREATE_HII_EXPORT) {
      //
      // Must have specified at least one HII pack file
      //
      if (mGlobals.PackFileNames == NULL) {
        Error (NULL, 0, 0, "must specify at least one input HII pack file in 'create' mode", NULL);
        return STATUS_ERROR;
      }
    }
  } else {
    //
    // Must have specified an input HII export file name
    //
    if (mGlobals.HiiExportFileNames == NULL) {
      Error (NULL, 0, 0, "must specify at least one '-x HiiExportFileName'", NULL);
      return STATUS_ERROR;
    }
  }

  return STATUS_SUCCESS;
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
  int          Index;
  const char   *Str[] = {
    UTILITY_NAME" "UTILITY_VERSION" - Create/Dump HII Database Files Utility",
    "  Copyright (C), 2004 - 2008 Intel Corporation",
#if ( defined(UTILITY_BUILD) && defined(UTILITY_VENDOR) )
    "  Built from "UTILITY_BUILD", project of "UTILITY_VENDOR,
#endif
    "",
    "Usage:",
    "  "UTILITY_NAME " [MODE] [OPTION]",
    "Modes:",
    "  create     create an HII export file from one or more HII pack files",
    "  merge      merge two or more HII export files into one HII export file",
    "  defaults   emit variable defaults from an input HII export file",
    "  dump       ASCII dump the contents of an HII export file",
    "Options for all modes:",
    "  -o FileName write output to FileName",
    "  -mfg        use manufacturing defaults from IFR rather than standard defaults",
    "  -g GUID     use GUID for a package GUID in the data tables where applicable",
    "  -v          verbose operation",
    "Options for 'create' mode:",
    "  -p PackFileName(s)  include contents of HII pack file PackFileName",
    "                      in the output file",
    "  -novarpacks         don't emit variable packs to the output file",
    "Options for 'merge' mode:",
    "  -x HiiExportFileName(s)  include contents of HII export file",
    "                           HiiExportFileName in the output file",
    "  -s Path FileMask         include all matching HII export files in Path",
    "                           and its subdirectories in the output file.",
    "                           If Path does not begin with the form C:\\, then",
    "                           it is assumed to be relative to the current working",
    "                           directory. FileMask may contain wildcard characters.",
    "Options for 'defaults' mode:",
    "  -x HiiExportFileName     emit defaults from all variables referenced",
    "                           in input file HiiExportFileName",
    "  -noemptyvarpacks         don't emit variable packs for 0-length variables",
    "Options for 'dump' mode:",
    "  -x HiiExportFileName     dump contents of input file HiiExportFileName",
    "  -dumpstrings             dump string data",
    NULL
  };
  for (Index = 0; Str[Index] != NULL; Index++) {
    fprintf (stdout, "%s\n", Str[Index]);
  }
}
