/*++

 Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
 This program and the accompanying materials                          
 are licensed and made available under the terms and conditions of the BSD License         
 which accompanies this distribution.  The full text of the license may be found at        
 http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


Module Name:

  GenAprioriFile.c  

Abstract:

  Given an input file containing a list of GUIDs (or Guided file names),
  convert the file to an Apriori file consumable by the dispatcher.

--*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "EfiCommon.h"
#include "ParseInf.h"
#include "CommonLib.h"  // for compare guid
#include "EfiUtilityMsgs.h"

#define MAX_LINE_LEN  200
#define MAX_PATH      200

//
// typedef unsigned int          STATUS;
// #define STATUS_SUCCESS        0
// #define STATUS_WARNING        1
// #define STATUS_ERROR          2
//
#define UTILITY_NAME    "GenAprioriFile"
#define UTILITY_VERSION "v1.0"
//
// Here's all our globals.
//
static struct {
  FILE    *BinFptr; // output dependencies to this file
  INT8    *AprioriFileName;
  INT8    *OutputFileName;
  BOOLEAN Intelligent;
  BOOLEAN Verbose;
  BOOLEAN NullTerminate;
} mGlobals;

static
STATUS
ProcessArgs (
  int   Argc,
  char  *Argv[]
  );

static
BOOLEAN
IsCommentLine (
  INT8    *Line
  );

static
void
Usage (
  VOID
  );

int
main (
  int   Argc,
  char  *Argv[]
  )
/*++

Routine Description:

  Call the routine to parse the command-line options, then process the
  Apriori list file and generate the GUID file.
  
Arguments:

  Standard C main() argc and argv.

Returns:

  0       if successful
  nonzero otherwise
  
--*/
// GC_TODO:    Argc - add argument and description to function comment
// GC_TODO:    ] - add argument and description to function comment
{
  STATUS    Status;
  FILE      *AprioriFptr;
  FILE      *BinFptr;
  INT8      Line[MAX_LINE_LEN];
  EFI_GUID  Guid;
  EFI_GUID  GuidIn;
  EFI_GUID  ZeroGuid;
  UINT32    LineCounter;
  //
  // Initialize the error printing routines
  //
  SetUtilityName (UTILITY_NAME);
  //
  // Clear our globals
  //
  memset ((char *) &mGlobals, 0, sizeof (mGlobals));
  memset ((char *) &ZeroGuid, 0, sizeof (ZeroGuid));
  AprioriFptr = NULL;
  BinFptr     = NULL;

  //
  // Process the command-line arguments
  //
  Status = ProcessArgs (Argc, Argv);
  if (Status != STATUS_SUCCESS) {
    return Status;
  }
  //
  // If arguments were ok, then open the Apriori file and process it.
  //
  if ((AprioriFptr = fopen (mGlobals.AprioriFileName, "r")) == NULL) {
    Error (NULL, 0, 0, mGlobals.AprioriFileName, "failed to open file for reading");
    goto FinishUp;
  }
  //
  // If -i intelligent option specified, then attempt to read and
  // existing output file and see if we'd be creating an identical file.
  //
  if (mGlobals.Intelligent) {
    if ((BinFptr = fopen (mGlobals.OutputFileName, "rb")) == NULL) {
      if (mGlobals.Verbose) {
        DebugMsg (NULL, 0, 0, "Creating new apriori file -- no existing file", NULL);
      }

      goto CreateFile;
    }
    //
    // Read lines from the input file until done. Convert each to a guid, then
    // read a guid from the input file and compare them.
    //
    while (fgets (Line, sizeof (Line), AprioriFptr) != NULL) {

      if (IsCommentLine (Line)) {
        continue;
      }
      //
      // Convert to a guid
      //
      if (StringToGuid (Line, &Guid) != EFI_SUCCESS) {
        if (mGlobals.Verbose) {
          DebugMsg (NULL, 0, 0, "failed to read GUID from input text file -- creating new file", NULL);
        }

        goto CreateFile;
      }
      //
      // Read guid from input file, then compare
      //
      if (fread (&GuidIn, sizeof (GuidIn), 1, BinFptr) != 1) {
        if (mGlobals.Verbose) {
          DebugMsg (NULL, 0, 0, "failed to read GUID from input binary file -- creating new file", NULL);
        }

        goto CreateFile;
      }

      if (CompareGuid (&Guid, &GuidIn) != 0) {
        if (mGlobals.Verbose) {
          DebugMsg (NULL, 0, 0, "GUID comparison failed -- creating new file", NULL);
        }

        goto CreateFile;
      }
    }
    //
    // May be one more NULL guid in the binary file
    //
    if (mGlobals.NullTerminate) {
      if (fread (&GuidIn, sizeof (GuidIn), 1, BinFptr) != 1) {
        if (mGlobals.Verbose) {
          DebugMsg (NULL, 0, 0, "failed to read NULL GUID from input binary file -- creating new file", NULL);
        }

        goto CreateFile;
      }

      if (CompareGuid (&GuidIn, &ZeroGuid) != 0) {
        if (mGlobals.Verbose) {
          DebugMsg (NULL, 0, 0, "NULL GUID comparison failed -- creating new file", NULL);
        }

        goto CreateFile;
      }
    }
    //
    // Make sure we're at the end of both files.
    //
    if ((fgets (Line, sizeof (Line), AprioriFptr) != NULL) || (fread (&GuidIn, 1, 1, BinFptr) != 0)) {
      if (mGlobals.Verbose) {
        DebugMsg (NULL, 0, 0, "file sizes different, -i test failed -- creating new file", NULL);
      }

      goto CreateFile;
    }

    if (mGlobals.Verbose) {
      DebugMsg (NULL, 0, 0, "existing file would be unchanged -- keeping existing apriori file", NULL);
    }

    goto FinishUp;
  }

CreateFile:
  //
  // Rewind the Apriori file in case -i was specified. Also
  // try to close the output file for the case where we prescanned
  // it (again, because of -i).
  //
  rewind (AprioriFptr);
  if (BinFptr != NULL) {
    fclose (BinFptr);
  }
  //
  // Open the output file
  //
  if ((BinFptr = fopen (mGlobals.OutputFileName, "wb")) == NULL) {
    Error (NULL, 0, 0, mGlobals.OutputFileName, "could not open input file");
    goto FinishUp;
  }
  //
  // Read lines until we're done
  //
  LineCounter = 0;
  while (fgets (Line, sizeof (Line), AprioriFptr) != NULL) {
    LineCounter++;
    if (IsCommentLine (Line)) {
      continue;
    }
    //
    // Convert to a GUID
    //
    if (StringToGuid (Line, &Guid) != EFI_SUCCESS) {
      Error (mGlobals.AprioriFileName, LineCounter, 0, "failed to convert GUID", NULL);
      goto FinishUp;
    }
    //
    // Write the guid to the output file
    //
    if (fwrite (&Guid, sizeof (Guid), 1, BinFptr) != 1) {
      Error (NULL, 0, 0, mGlobals.OutputFileName, "failed to write GUID to output file");
      goto FinishUp;
    }
  }
  //
  // Write a null guid out to terminate the list
  //
  if (mGlobals.NullTerminate) {
    memset ((void *) &Guid, 0, sizeof (Guid));
    if (fwrite (&Guid, sizeof (Guid), 1, BinFptr) != 1) {
      Error (NULL, 0, 0, mGlobals.OutputFileName, "failed to write NULL termination GUID to output file");
    }
  }

FinishUp:

  if (AprioriFptr != NULL) {
    fclose (AprioriFptr);
  }

  if (BinFptr != NULL) {
    fclose (BinFptr);
  }

  return GetUtilityStatus ();
}

static
BOOLEAN
IsCommentLine (
  INT8    *Line
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Line  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  for (; isspace (*Line) && *Line; Line++)
    ;

  //
  // Allow # or // comments
  //
  if ((*Line == '#') || ((*Line == '/') && (*(Line + 1) == '/')) || (*Line == '\n') || (*Line == 0)) {
    return TRUE;
  }

  return FALSE;
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
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Argc  - GC_TODO: add argument description
  ]     - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  //
  // Skip program name
  //
  Argc--;
  Argv++;

  //
  // Process until no more args
  //
  while (Argc) {
    //
    // -f AprioriFile
    //
    if (_stricmp (Argv[0], "-f") == 0) {
      //
      // check for one more arg
      //
      if (Argc > 1) {
        mGlobals.AprioriFileName = Argv[1];
      } else {
        Error (NULL, 0, 0, NULL, "missing filename with %s", Argv[0]);
        Usage ();
        return STATUS_ERROR;
      }

      Argc--;
      Argv++;
    } else if (_stricmp (Argv[0], "-i") == 0) {
      //
      // intelligent creation of output file. That is to say, if
      // there's already a file there, and it's the same as what
      // we'd create, then don't re-create. This is to support
      // incremental builds (that is to say, running nmake a second time
      // does nothing).
      //
      mGlobals.Intelligent = TRUE;
    } else if (_stricmp (Argv[0], "-v") == 0) {
      mGlobals.Verbose = TRUE;
    } else if (_stricmp (Argv[0], "-null") == 0) {
      mGlobals.NullTerminate = TRUE;
    } else if (_stricmp (Argv[0], "-o") == 0) {
      //
      // -o OutputFileName
      // check for one more arg
      //
      if (Argc > 1) {
        mGlobals.OutputFileName = Argv[1];
      } else {
        Error (NULL, 0, 0, NULL, "missing filename argument with %s", Argv[0]);
        Usage ();
        return STATUS_ERROR;
      }

      Argc--;
      Argv++;
    } else if ((_stricmp (Argv[0], "-h") == 0) || (strcmp (Argv[0], "-?") == 0)) {
      Usage ();
      return STATUS_ERROR;
    } else {
      Error (NULL, 0, 0, Argv[0], "unrecognized option");
      Usage ();
      return STATUS_ERROR;
    }

    Argc--;
    Argv++;
  }
  //
  // Had to specify the apriori input file and output file names
  //
  if (mGlobals.AprioriFileName == NULL) {
    Error (NULL, 0, 0, "must specify -f AprioriFile", NULL);
    Usage ();
    return STATUS_ERROR;
  }

  if (mGlobals.OutputFileName == NULL) {
    Error (NULL, 0, 0, "must specify -o OutputFile", NULL);
    Usage ();
    return STATUS_ERROR;
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
  int        Index;
  const char *Str[] = {
    UTILITY_NAME" "UTILITY_VERSION" - Intel Generate Apriori File Utility",
    "  Copyright (C), 2006 - 2008 Intel Corporation",
    
#if ( defined(UTILITY_BUILD) && defined(UTILITY_VENDOR) )
    "  Built from "UTILITY_BUILD", project of "UTILITY_VENDOR,
#endif
    "",
    "Usage:",
    "  "UTILITY_NAME" [OPTION]...",
    "Description:",
    "  Generate an Apriori file consumable by the DXE or PEI dispatcher.",
    "Options:",
    "  -h or -?         for this help information",
    "  -f AprioriFile   parse the GUID'ed files in AprioriFile (required)",
    "  -o OutputFile    write output to OutputFile (required)",
    "  -i               for intelligent re-creation of OutputFile",
    "  -null            to terminate the output file with a NULL GUID",
    "  -v               verbose option",
    NULL
  };
  for (Index = 0; Str[Index] != NULL; Index++) {
    fprintf (stdout, "%s\n", Str[Index]);
  }
}
