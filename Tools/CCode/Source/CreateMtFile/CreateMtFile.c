/*++

Copyright (c)  1999-2006 Intel Corporation. All rights reserved
This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  CreateMtFile.c

Abstract:

  Simple utility to create a pad file containing fixed data.
  
--*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <Common/UefiBaseTypes.h>

#define UTILITY_NAME "CreateMtFile"
#define UTILITY_MAJOR_VERSION 1
#define UTILITY_MINOR_VERSION 1

typedef struct {
  INT8    *OutFileName;
  INT8    ByteValue;
  UINT32  FileSize;
} OPTIONS;

static
EFI_STATUS
ProcessArgs (
  IN INT32          Argc,
  IN INT8           *Argv[],
  IN OUT OPTIONS    *Options
  );

static
void
Usage (
  VOID
  );

static
void
Version (
  VOID
  );

int
main (
  IN INT32  Argc,
  IN INT8   *Argv[]
  )
/*++

Routine Description:
  
  Main entry point for this utility.

Arguments:

  Standard C entry point args Argc and Argv

Returns:

  EFI_SUCCESS if good to go

--*/
// GC_TODO:    ] - add argument and description to function comment
// GC_TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// GC_TODO:    EFI_DEVICE_ERROR - add return value to function comment
// GC_TODO:    EFI_DEVICE_ERROR - add return value to function comment
{
  FILE    *OutFptr;
  OPTIONS Options;

  //
  // Process the command-line arguments.
  //
  if (ProcessArgs (Argc, Argv, &Options) != EFI_SUCCESS) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Open the output file
  //
  if ((OutFptr = fopen (Options.OutFileName, "wb")) == NULL) {
      printf (" ERROR: Could not open output file '%s' for writing\n", Options.OutFileName);
    return EFI_DEVICE_ERROR;
  }
  //
  // Write the pad bytes. Do it the slow way (one at a time) for now.
  //
  while (Options.FileSize > 0) {
    if (fwrite (&Options.ByteValue, 1, 1, OutFptr) != 1) {
      fclose (OutFptr);
      printf (" ERROR: Failed to write to output file\n");
      return EFI_DEVICE_ERROR;
    }

    Options.FileSize--;
  }
  //
  // Close the file
  //
  fclose (OutFptr);
  return EFI_SUCCESS;
}

static
EFI_STATUS
ProcessArgs (
  IN INT32          Argc,
  IN INT8           *Argv[],
  IN OUT OPTIONS    *Options
  )
/*++

Routine Description:
  
  Process the command line arguments.

Arguments:

  Argc    - argument count as passed in to the entry point function
  Argv    - array of arguments as passed in to the entry point function
  Options - stucture of where to put the values of the parsed arguments

Returns:

  EFI_SUCCESS if everything looks good
  EFI_INVALID_PARAMETER otherwise

--*/
// GC_TODO:    ] - add argument and description to function comment
{
  UINT32  Multiplier;

  //
  // Clear the options
  //
  memset ((char *) Options, 0, sizeof (OPTIONS));

  //
  // Skip program name
  //
  Argv++;
  Argc--;
  
  if (Argc < 1) {
    Usage();
    return EFI_INVALID_PARAMETER;
  }
  
  if ((strcmp(Argv[0], "-h") == 0) || (strcmp(Argv[0], "--help") == 0) ||
      (strcmp(Argv[0], "-?") == 0) || (strcmp(Argv[0], "/?") == 0)) {
    Usage();
    return EFI_INVALID_PARAMETER;
  }
  
  if ((strcmp(Argv[0], "-V") == 0) || (strcmp(Argv[0], "--version") == 0)) {
    Version();
    return EFI_INVALID_PARAMETER;
  }
 
  if (Argc < 2) {
    Usage ();
    return EFI_INVALID_PARAMETER;
  }
  //
  // If first arg is dash-option, then print usage.
  //
  if (Argv[0][0] == '-') {
    Usage ();
    return EFI_INVALID_PARAMETER;
  }
  //
  // First arg is file name
  //
  Options->OutFileName = Argv[0];
  Argc--;
  Argv++;

  //
  // Second arg is file size. Allow 0x1000, 0x100K, 1024, 1K
  //
  Multiplier = 1;
  if ((Argv[0][strlen (Argv[0]) - 1] == 'k') || (Argv[0][strlen (Argv[0]) - 1] == 'K')) {
    Multiplier = 1024;
  }
  
  //
  // Check for negtive size
  //
  if (Argv[0][0] == '-') {
    printf("ERROR: File size should be non-negtive.\n");
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Look for 0x prefix on file size
  //
  if ((Argv[0][0] == '0') && ((Argv[0][1] == 'x') || (Argv[0][1] == 'X'))) {
    if (sscanf (Argv[0], "%x", &Options->FileSize) != 1) {
      printf ("ERROR: Invalid file size '%s'\n", Argv[0]);
      Usage ();
      return EFI_INVALID_PARAMETER;
    }
    //
    // Otherwise must be a decimal number
    //
  } else {
    if (sscanf (Argv[0], "%d", &Options->FileSize) != 1) {
      printf ("ERROR: Invalid file size '%s'\n", Argv[0]);
      Usage ();
      return EFI_INVALID_PARAMETER;
    }
  }
  
  Options->FileSize *= Multiplier;
  //
  // Assume byte value of 0xff
  //
  Options->ByteValue = (INT8) (UINT8) 0xFF;
  return EFI_SUCCESS;
}


static
void 
Version(
  void
  )
/*++

Routine Description:

  Print out version information for this utility.

Arguments:

  None
  
Returns:

  None
  
--*/ 
{
  printf ("%s v%d.%d -EDK utility to create a pad file containing fixed data\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION);
  printf ("Copyright (c) 1999-2006 Intel Corporation. All rights reserved.\n");
}

//
// Print utility usage info
//
static
void
Usage (
  VOID
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  None

Returns:

  GC_TODO: add return values

--*/
{ 
  Version();
  
  printf ("\nUsage: %s OutFileName FileSize \n\
   where: \n\
      OutFileName is the name of the output file to generate \n\
      FileSize is the size of the file to create \n\
   Examples: \n\
      %s OutFile.bin 32K \n\
      %s OutFile.bin 0x1000 \n",UTILITY_NAME, UTILITY_NAME, UTILITY_NAME);
} 

