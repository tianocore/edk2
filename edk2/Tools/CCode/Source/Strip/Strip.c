/*++

Copyright (c)  2004-2006 Intel Corporation. All rights reserved
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  Strip.c

Abstract:

  Quick Exe2Bin equivalent.

--*/

#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>

#define UTILITY_NAME  "Strip"
#define UTILITY_MAJOR_VERSION 1
#define UTILITY_MINOR_VERSION 1


void 
Version(
  void
  )
/*++

Routine Description:

  Print out version information for Strip.

Arguments:

  None
  
Returns:

  None
  
--*/ 
{
  printf ("%s v%d.%d -EDK Utility to Convert EXE to BIN\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION);
  printf ("Copyright (c) 2005-2006 Intel Corporation. All rights reserved.\n");
}

void 
Usage(
  void
  )
/*++

Routine Description:

  Print out usage information for Strip.

Arguments:

  None
  
Returns:

  None
  
--*/ 
{
  Version();
  printf ("\nUsage: %s InputFile OutputFile\n", UTILITY_NAME);
}

int
main (
  int  argc,
  char *argv[]
  )
/*++

Routine Description:

  Converts executable files to binary files.

Arguments:

  argc   - Number of command line arguments
  argv[] - Array of pointers to the command line arguments

Returns:

  Zero     - Function completed successfully.
  Non-zero - Function exited with errors.

--*/
{
  FILE  *InFile;
  FILE  *OutFile;
  int   Index;
  int   FileSize;
  char  *Buffer;
  char  *Ptrx;
  
  if (argc == 1) {
    Usage();
    return -1;
  }
  
  if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0) ||
      (strcmp(argv[1], "-?") == 0) || (strcmp(argv[1], "/?") == 0)) {
    Usage();
    return 0;
  }
  
  if ((strcmp(argv[1], "-V") == 0) || (strcmp(argv[1], "--version") == 0)) {
    Version();
    return 0;
  }
  
  if (argc < 3) {
    Usage();
    return -1;
  }

  InFile  = fopen (argv[1], "rb");
  OutFile = fopen (argv[2], "wb");

  if (!InFile) {
    printf ("Unable to open input file, exit\n");
    return -1;
  }

  if (OutFile == NULL) {
    printf ("Unable to open output file, exit.\n");
    return -1;
  }

  fseek (InFile, 0, SEEK_END);
  FileSize = ftell (InFile);

  if (FileSize < 0x200) {
    printf ("%d is not a legal file size, exit\n", FileSize);
    return -1;
  }

  fseek (InFile, 0, SEEK_SET);

  Buffer = (char *) malloc (FileSize);
  if (Buffer == NULL) {
    printf ("Error: Out of resources.\n");
    return -1;
  }

  fread (Buffer, 1, FileSize, InFile);

  Ptrx  = Buffer + 0x200;

  Index = FileSize - 0x200;

  fwrite (Ptrx, Index, 1, OutFile);

  fclose (InFile);
  fclose (OutFile);
  free (Buffer);

  return 0;
}
