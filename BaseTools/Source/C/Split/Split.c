/** @file

  Split a file into two pieces at the request offset.

Copyright (c) 1999 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

// GC_TODO: fix comment to start with /*++
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef __GNUC__
#include <unistd.h>
#else
#include <direct.h>
#endif
#include <ctype.h>
#include "ParseInf.h"
#include "CommonLib.h"
#include "EfiUtilityMsgs.h"
//
// Utility Name
//
#define UTILITY_NAME  "Split"

//
// Utility version information
//
#define UTILITY_MAJOR_VERSION 0
#define UTILITY_MINOR_VERSION 1

void
Version (
  void
  )
/*++

Routine Description:

  Displays the standard utility information to SDTOUT

Arguments:

  None

Returns:

  None

--*/
{
  printf ("%s v%d.%d %s -Utility to break a file into two pieces at the request offset.\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION, __BUILD_VERSION);
  printf ("Copyright (c) 1999-2010 Intel Corporation. All rights reserved.\n");
}

void
Usage (
  void
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:


Returns:

  GC_TODO: add return values

--*/
{
  Version();
  printf ("\nUsage: \n\
   Split\n\
     -f, --filename inputFile to split\n\
     -s, --split VALUE the number of bytes in the first file\n\
     [-p, --prefix OutputDir]\n\
     [-o, --firstfile Filename1]\n\
     [-t, --secondfile Filename2]\n\
     [-v, --verbose]\n\
     [--version]\n\
     [-q, --quiet disable all messages except fatal errors]\n\
     [-d, --debug[#]\n\
     [-h, --help]\n");
}

EFI_STATUS
GetSplitValue (
  IN CONST CHAR8* SplitValueString,
  OUT UINT64 *ReturnValue
)
{
  UINT64 len = strlen(SplitValueString);
  UINT64 base = 1;
  UINT64 index = 0;
  UINT64 number = 0;
  CHAR8 lastCHAR = 0;
  EFI_STATUS Status = EFI_SUCCESS;

  if (len == 0) {
    return EFI_ABORTED;
  }

  Status = AsciiStringToUint64 (SplitValueString, FALSE, ReturnValue);
  if (!EFI_ERROR (Status)) {
    return Status;
  }

  if (SplitValueString[0] == '0' && (SplitValueString[1] == 'x' || SplitValueString[1] == 'X')) {
    Status = AsciiStringToUint64 (SplitValueString, TRUE, ReturnValue);
    if (!EFI_ERROR (Status)) {
      return Status;
    }
  }

  lastCHAR = (CHAR8)toupper((int)SplitValueString[len - 1]);

  if (lastCHAR != 'K' && lastCHAR != 'M' && lastCHAR != 'G') {
    return STATUS_ERROR;
  }

  for (;index < len - 1; ++index) {
    if (!isdigit((int)SplitValueString[index])) {
      return EFI_ABORTED;
    }
  }

  number = atol (SplitValueString);
  if (lastCHAR == 'K')
    base = 1024;
  else if (lastCHAR == 'M')
    base = 1024*1024;
  else
    base = 1024*1024*1024;

  *ReturnValue = number*base;

  return EFI_SUCCESS;
}

EFI_STATUS
CountVerboseLevel (
  IN CONST CHAR8* VerboseLevelString,
  IN CONST UINT64 Length,
  OUT UINT64 *ReturnValue
)
{
  UINT64 i = 0;
  for (;i < Length; ++i) {
    if (VerboseLevelString[i] != 'v' && VerboseLevelString[i] != 'V') {
      return EFI_ABORTED;
    }
    ++(*ReturnValue);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
CreateDir (
  IN OUT CHAR8** FullFileName
)
{
  CHAR8* temp = *FullFileName;
  CHAR8* start = temp;
  UINT64 index = 0;

  for (;index < strlen(temp); ++index) {
    if (temp[index] == '\\' || temp[index] == '/') {
      temp[index] = 0;
      if (chdir(start)) {
        if (mkdir(start, S_IRWXU | S_IRWXG | S_IRWXO) != 0) {
          return EFI_ABORTED;
        }
        chdir(start);
    }
      start = temp + index + 1;
      temp[index] = '/';
  }
  }

  return EFI_SUCCESS;
}

int
main (
  int argc,
  char*argv[]
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  argc  - GC_TODO: add argument description
  ]     - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  EFI_STATUS    Status = EFI_SUCCESS;
  FILE          *In;
  CHAR8         *InputFileName = NULL;
  CHAR8         *OutputDir = NULL;
  CHAR8         *OutFileName1 = NULL;
  CHAR8         *OutFileName2 = NULL;
  UINT64        SplitValue = (UINT64) -1;
  FILE          *Out1;
  FILE          *Out2;
  CHAR8         *OutName1 = NULL;
  CHAR8         *OutName2 = NULL;
  CHAR8         *CurrentDir = NULL;
  UINT64        Index;
  CHAR8         CharC;
  UINT64        DebugLevel = 0;
  UINT64        VerboseLevel = 0;

  SetUtilityName(UTILITY_NAME);
  if (argc == 1) {
    Usage();
    return STATUS_ERROR;
  }

  argc --;
  argv ++;

  if ((stricmp (argv[0], "-h") == 0) || (stricmp (argv[0], "--help") == 0)) {
    Usage();
    return STATUS_SUCCESS;
  }

  if (stricmp (argv[0], "--version") == 0) {
    Version();
    return STATUS_SUCCESS;
  }

  while (argc > 0) {
    if ((stricmp (argv[0], "-p") == 0) || (stricmp (argv[0], "--prefix") == 0)) {
      OutputDir = argv[1];
      if (OutputDir == NULL) {
        Warning (NULL, 0, 0, "NO output directory specified.", NULL);
        return STATUS_ERROR;
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-f") == 0) || (stricmp (argv[0], "--filename") == 0)) {
      InputFileName = argv[1];
      if (InputFileName == NULL) {
        Error (NULL, 0, 0x1001, "NO Input file specified.", NULL);
        return STATUS_ERROR;
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-s") == 0) || (stricmp (argv[0], "--split") == 0)) {
      Status = GetSplitValue(argv[1], &SplitValue);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 0x1003, "Input split value is not one valid integer.", NULL);
        return STATUS_ERROR;
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-o") == 0) || (stricmp (argv[0], "--firstfile") == 0)) {
      OutFileName1 = argv[1];
      if (OutFileName1 == NULL) {
        Warning (NULL, 0, 0, NULL, "No output file1 specified.");
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-t") == 0) || (stricmp (argv[0], "--secondfile") == 0)) {
      OutFileName2 = argv[1];
      if (OutFileName2 == NULL) {
        Warning (NULL, 0, 0, NULL, "No output file2 specified.");
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-q") == 0) || (stricmp (argv[0], "--quiet") == 0)) {
      argc --;
      argv ++;
      continue;
    }

    if ((strlen(argv[0]) >= 2 && argv[0][0] == '-' && (argv[0][1] == 'v' || argv[0][1] == 'V')) || (stricmp (argv[0], "--verbose") == 0)) {
      VerboseLevel = 1;
      if (strlen(argv[0]) > 2) {
        Status = CountVerboseLevel (&argv[0][2], strlen(argv[0]) - 2, &VerboseLevel);
        if (EFI_ERROR (Status)) {
          Error (NULL, 0, 0x1003, NULL, "%s is invaild paramter!", argv[0]);
          return STATUS_ERROR;
        }
      }

      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-d") == 0) || (stricmp (argv[0], "--debug") == 0)) {
      Status = AsciiStringToUint64 (argv[1], FALSE, &DebugLevel);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 0x1003, "Input debug level is not one valid integrator.", NULL);
        return STATUS_ERROR;
      }
      argc -= 2;
      argv += 2;
      continue;
    }
    //
    // Don't recognize the paramter.
    //
    Error (NULL, 0, 0x1003, NULL, "%s is invaild paramter!", argv[0]);
    return STATUS_ERROR;
  }

  if (InputFileName == NULL) {
    Error (NULL, 0, 0x1001, "NO Input file specified.", NULL);
    return STATUS_ERROR;
  }

  In = fopen (InputFileName, "rb");
  if (In == NULL) {
    // ("Unable to open file \"%s\"\n", InputFileName);
    Error (InputFileName, 0, 1, "File open failure", NULL);
    return STATUS_ERROR;
  }

  if (OutFileName1 == NULL) {
    OutName1 = (CHAR8*)malloc(strlen(InputFileName) + 16);
    if (OutName1 == NULL) {
      Warning (NULL, 0, 0, NULL, "Memory Allocation Fail.");
      return STATUS_ERROR;
    }
    strcpy (OutName1, InputFileName);
    strcat (OutName1, "1");
    OutFileName1 = OutName1;

  }
  if (OutFileName2 == NULL) {
    OutName2 = (CHAR8*)malloc(strlen(InputFileName) + 16);
    if (OutName2 == NULL) {
      Warning (NULL, 0, 0, NULL, "Memory Allocation Fail.");
      return STATUS_ERROR;
    }
    strcpy (OutName2, InputFileName);
    strcat (OutName2, "2");
    OutFileName2 = OutName2;

  }

  if (OutputDir != NULL) {
    //OutputDirSpecified = TRUE;
    if (chdir(OutputDir) != 0) {
      Warning (NULL, 0, 0, NULL, "Change dir to OutputDir Fail.");
      return STATUS_ERROR;
    }
  }

  CurrentDir = (CHAR8*)getcwd((CHAR8*)0, 0);
  if (EFI_ERROR(CreateDir(&OutFileName1))) {
      Error (OutFileName1, 0, 5, "Create Dir for File1 Fail.", NULL);
      return STATUS_ERROR;
  }
  chdir(CurrentDir);

  if (EFI_ERROR(CreateDir(&OutFileName2))) {
      Error (OutFileName2, 0, 5, "Create Dir for File2 Fail.", NULL);
      return STATUS_ERROR;
  }
  chdir(CurrentDir);
  free(CurrentDir);

  Out1 = fopen (OutFileName1, "wb");
  if (Out1 == NULL) {
    // ("Unable to open file \"%s\"\n", OutFileName1);
    Error (OutFileName1, 0, 1, "File open failure", NULL);
    return STATUS_ERROR;
  }

  Out2 = fopen (OutFileName2, "wb");
  if (Out2 == NULL) {
    // ("Unable to open file \"%s\"\n", OutFileName2);
    Error (OutFileName2, 0, 1, "File open failure", NULL);
    return STATUS_ERROR;
  }

  for (Index = 0; Index < SplitValue; Index++) {
    CharC = (CHAR8) fgetc (In);
    if (feof (In)) {
      break;
    }

    fputc (CharC, Out1);
  }

  for (;;) {
    CharC = (CHAR8) fgetc (In);
    if (feof (In)) {
      break;
    }

    fputc (CharC, Out2);
  }

  if (OutName1 != NULL) {
    free(OutName1);
  }
  if (OutName2 != NULL) {
    free(OutName2);
  }
  fclose (In);
  fclose (Out1);
  fclose (Out2);

  return STATUS_SUCCESS;
}
