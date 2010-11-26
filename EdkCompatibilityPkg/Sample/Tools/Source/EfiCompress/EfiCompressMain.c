/*++ 

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  EfiCompressMain.c
  
Abstract:

--*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include "TianoCommon.h"
#include "Compress.h"

#define UTILITY_VERSION "v1.0"
#define UTILITY_NAME    "EfiCompress"

typedef enum {
  EFI_COMPRESS   = 1,
  TIANO_COMPRESS = 2
} COMPRESS_TYPE;

typedef struct _COMPRESS_ACTION_LIST {
  struct _COMPRESS_ACTION_LIST   *NextAction;
  INT32                          CompressType;
  CHAR8                          *InFileName;
  CHAR8                          *OutFileName;
} COMPRESS_ACTION_LIST;


STATIC
BOOLEAN
ParseCommandLine (
  INT32                 argc,
  CHAR8                 *argv[],
  COMPRESS_ACTION_LIST  **ActionListHead
  )
/*++

Routine Description:

  Parse command line options

Arguments:
  
  argc    - number of arguments passed into the command line.
  argv[]  - files to compress and files to output compressed data to.
  Options - Point to COMMAND_LINE_OPTIONS, receiving command line options.

Returns:
  
  BOOLEAN: TRUE for a successful parse.
--*/
;

STATIC
VOID
Usage (
  CHAR8 *ExeName
  )
/*++

Routine Description:

  Print usage.

Arguments:
  
  ExeName  - Application's full path

--*/
;


STATIC
BOOLEAN
ProcessFile (
  CHAR8         *InFileName,
  CHAR8         *OutFileName,
  COMPRESS_TYPE CompressType
  )
/*++

Routine Description:
  
  Compress InFileName to OutFileName using algorithm specified by CompressType.

Arguments:
  
  InFileName    - Input file to compress
  OutFileName   - Output file compress to
  CompressType  - Compress algorithm, can be EFI_COMPRESS or TIANO_COMPRESS

Returns:
  
  BOOLEAN: TRUE for compress file successfully

--*/
;

int
main (
  INT32 argc,
  CHAR8 *argv[]
  )
/*++

Routine Description:

  Compresses the input files

Arguments:

  argc   - number of arguments passed into the command line.
  argv[] - files to compress and files to output compressed data to.

Returns:

  int: 0 for successful execution of the function.

--*/
{
  COMPRESS_ACTION_LIST *ActionList;
  COMPRESS_ACTION_LIST *NextAction;
  UINT32               ActionCount;
  UINT32               SuccessCount;

  ActionList            = NULL;
  ActionCount          = SuccessCount = 0;

  if (!ParseCommandLine (argc, argv, &ActionList)) {
    Usage (*argv);
    return 1;
  }

  while (ActionList != NULL) {
    ++ActionCount;
    if (ProcessFile (
          ActionList->InFileName, 
          ActionList->OutFileName, 
          ActionList->CompressType)
        ) {
      ++SuccessCount;
    }
    NextAction = ActionList;
    ActionList = ActionList->NextAction;
    free (NextAction);
  }

  fprintf (stdout, "\nCompressed %d files, %d succeed!\n", ActionCount, SuccessCount);
  if (SuccessCount < ActionCount) {
    return 1;
  }

  return 0;
}

STATIC
BOOLEAN
ParseCommandLine (
  INT32                 argc,
  CHAR8                 *argv[],
  COMPRESS_ACTION_LIST  **ActionListHead
  )
{
  COMPRESS_TYPE         CurrentType;

  COMPRESS_ACTION_LIST  **Action;
  
  Action           =    ActionListHead;
  CurrentType      =    EFI_COMPRESS;     // default compress algorithm

  // Skip Exe Name
  --argc;
  ++argv;

  while (argc > 0) {
    if (strcmp (*argv, "-h") == 0 || strcmp (*argv, "-?") == 0) {
      //
      // 1. Directly return, help message will be printed.
      //
      return FALSE;
    
    } else if (strncmp (*argv, "-t", 2) == 0) {
      //
      // 2. Specifying CompressType
      //
      if (_stricmp ((*argv)+2, "EFI") == 0) {
        CurrentType = EFI_COMPRESS;
      } else if (_stricmp ((*argv)+2, "Tiano") == 0) {
        CurrentType = TIANO_COMPRESS;
      } else {
        fprintf (stdout, "  ERROR: CompressType %s not supported!\n", (*argv)+2);
        return FALSE;
      }
    } else {
      //
      // 3. Current parameter is *FileName
      //
      if (*Action == NULL) { 
        //
        // need to create a new action item
        //
        *Action = (COMPRESS_ACTION_LIST*) malloc (sizeof **Action);
        if (*Action == NULL) {
          fprintf (stdout, "  ERROR: malloc failed!\n");
          return FALSE;
        }
        memset (*Action, 0, sizeof **Action);
        (*Action)->CompressType = CurrentType;
      }

      //
      // Assignment to InFileName and OutFileName in order
      // 
      if ((*Action)->InFileName == NULL) {
        (*Action)->InFileName  = *argv;
      } else {
        (*Action)->OutFileName = *argv;
        Action                 = &(*Action)->NextAction;
      }
    }

    --argc;
    ++argv;

  }
  
  if (*Action != NULL) {
    assert ((*Action)->InFileName != NULL);
    fprintf (stdout, "  ERROR: Compress OutFileName not specified with InFileName: %s!\n", (*Action)->InFileName);
    return FALSE;
  }

  if (*ActionListHead == NULL) {
    return FALSE;
  }
  return TRUE;
}

STATIC
BOOLEAN
ProcessFile (
  CHAR8         *InFileName,
  CHAR8         *OutFileName,
  COMPRESS_TYPE CompressType
  )
{
  EFI_STATUS          Status;
  FILE                *InFileP;
  FILE                *OutFileP;
  UINT32              SrcSize;
  UINT32              DstSize;
  UINT8               *SrcBuffer;
  UINT8               *DstBuffer;
  COMPRESS_FUNCTION   CompressFunc;

  SrcBuffer     =     DstBuffer = NULL;
  InFileP       =     OutFileP  = NULL;

  fprintf (stdout, "%s --> %s\n", InFileName, OutFileName);

  if ((OutFileP = fopen (OutFileName, "wb")) == NULL) {
    fprintf (stdout, "  ERROR: Can't open output file %s for write!\n", OutFileName);
    goto ErrorHandle;
  }

  if ((InFileP = fopen (InFileName, "rb")) == NULL) {
    fprintf (stdout, "  ERROR: Can't open input file %s for read!\n", InFileName);
    goto ErrorHandle;
  }
  
  //
  // Get the size of source file
  //
  fseek (InFileP, 0, SEEK_END);
  SrcSize = ftell (InFileP);
  rewind (InFileP);
  //
  // Read in the source data
  //
  if ((SrcBuffer = malloc (SrcSize)) == NULL) {
    fprintf (stdout, "  ERROR: Can't allocate memory!\n");
    goto ErrorHandle;
  }

  if (fread (SrcBuffer, 1, SrcSize, InFileP) != SrcSize) {
    fprintf (stdout, "  ERROR: Can't read from source!\n");
    goto ErrorHandle;
  }

  //
  // Choose the right compress algorithm
  //
  CompressFunc = (CompressType == EFI_COMPRESS) ? EfiCompress : TianoCompress;

  //
  // Get destination data size and do the compression
  //
  DstSize = 0;
  Status = CompressFunc (SrcBuffer, SrcSize, DstBuffer, &DstSize);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    fprintf (stdout, "  Error: Compress failed: %x!\n", Status);
    goto ErrorHandle;
  }
  if ((DstBuffer = malloc (DstSize)) == NULL) {
    fprintf (stdout, "  ERROR: Can't allocate memory!\n");
    goto ErrorHandle;
  }

  Status = CompressFunc (SrcBuffer, SrcSize, DstBuffer, &DstSize);
  if (EFI_ERROR (Status)) {
    fprintf (stdout, "  ERROR: Compress Error!\n");
    goto ErrorHandle;
  }

  fprintf (stdout, "  Orig Size = %ld\tComp Size = %ld\n", SrcSize, DstSize);

  if (DstBuffer == NULL) {
    fprintf (stdout, "  ERROR: No destination to write to!\n");
    goto ErrorHandle;
  }

  //
  // Write out the result
  //
  if (fwrite (DstBuffer, 1, DstSize, OutFileP) != DstSize) {
    fprintf (stdout, "  ERROR: Can't write to destination file!\n");
    goto ErrorHandle;
  }

  return TRUE;

ErrorHandle:
  if (SrcBuffer) {
    free (SrcBuffer);
  }

  if (DstBuffer) {
    free (DstBuffer);
  }

  if (InFileP) {
    fclose (InFileP);
  }

  if (OutFileP) {
    fclose (OutFileP);
  }
  return FALSE;
}

VOID
Usage (
  CHAR8 *ExeName
  )
{
  int         Index;
  const char  *Str[] = {
    UTILITY_NAME" "UTILITY_VERSION" - Intel EFI Compress Utility",
    "  Copyright (C), 2006 - 2008 Intel Corporation",
    
#if ( defined(UTILITY_BUILD) && defined(UTILITY_VENDOR) )
    "  Built from "UTILITY_BUILD", project of "UTILITY_VENDOR,
#endif
    "",
    "Usage:",
    "  "UTILITY_NAME" [OPTION] SOURCE DEST ...",
    "Description:",
    "  Compress a list of SOURCE(s) to accordingly DEST(s) using the specified",
    "  compress algorithm.",
    "Options:",
    "  -tCompressAlgo   Optional compress algorithm (EFI | Tiano), case insensitive.",
    "                   If ommitted, compress type specified ahead is used,",
    "                   default is EFI\n"
    "                   e.g.: EfiCompress a.in a.out -tTiano b.in b.out \\",
    "                                     c.in c.out -tEFI d.in d.out",
    "                   a.in and d.in are compressed using EFI compress algorithm",
    "                   b.in and c.in are compressed using Tiano compress algorithm",
    NULL
  };
  for (Index = 0; Str[Index] != NULL; Index++) {
    fprintf (stdout, "%s\n", Str[Index]);
  }
  
}
