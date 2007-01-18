/*++

Copyright (c)  1999-2006 Intel Corporation. All rights reserved
This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:
  
  EfiCompressMain.c

Abstract:

  The main function for the compression utility.
  
--*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

#include <Common/UefiBaseTypes.h>
#include "Compress.h"

#define UTILITY_NAME "EfiCompress"
#define UTILITY_MAJOR_VERSION 1
#define UTILITY_MINOR_VERSION 1

void 
Version(
  void
  )
/*++

Routine Description:

  Print out version information for EfiCompress.

Arguments:

  None
  
Returns:

  None
  
--*/ 
{
  printf ("%s v%d.%d -Efi File Compress Utility\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION);
  printf ("Copyright (c) 2005-2006 Intel Corporation. All rights reserved.\n");
}

void 
Usage(
  void
  )
/*++

Routine Description:

  Print out usage information for EfiCompress.

Arguments:

  None
  
Returns:

  None
  
--*/ 
{
  Version();
  printf ("\nUsage: %s Inputfile Outputfile\n", UTILITY_NAME);
}


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
  EFI_STATUS  Status;
  FILE        *infile;
  FILE        *outfile;
  UINT32      SrcSize;
  UINT32      DstSize;
  UINT8       *SrcBuffer;
  UINT8       *DstBuffer;
  UINT8       Buffer[8];

  //
  //  Added for makefile debug - KCE
  //
  INT32       arg_counter;
 
  SrcBuffer             = DstBuffer = NULL;
  infile                = outfile = NULL;

  if (argc == 1) {
    Usage();
    goto Done;
  }
  
  if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0) ||
      (strcmp(argv[1], "-?") == 0) || (strcmp(argv[1], "/?") == 0)) {
    Usage();
    goto Done;
  }
  
  if ((strcmp(argv[1], "-V") == 0) || (strcmp(argv[1], "--version") == 0)) {
    Version();
    goto Done;
  }
  
  if (argc != 3) {
    Usage();
    goto Done;
  }

  if ((outfile = fopen (argv[2], "wb")) == NULL) {
    printf ("Can't open output file\n");
    goto Done;
  }

  if ((infile = fopen (argv[1], "rb")) == NULL) {
    printf ("Can't open input file\n");
    goto Done;
  }
  //
  // Get the size of source file
  //
  SrcSize = 0;
  while (fread (Buffer, 1, 1, infile)) {
    SrcSize++;

  }
  //
  // Read in the source data
  //
  if ((SrcBuffer = malloc (SrcSize)) == NULL) {
    printf ("Can't allocate memory\n");
    goto Done;
  }

  rewind (infile);
  if (fread (SrcBuffer, 1, SrcSize, infile) != SrcSize) {
    printf ("Can't read from source\n");
    goto Done;
  }
  //
  // Get destination data size and do the compression
  //
  DstSize = 0;
  Status  = EfiCompress (SrcBuffer, SrcSize, DstBuffer, &DstSize);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    if ((DstBuffer = malloc (DstSize)) == NULL) {
      printf ("Can't allocate memory\n");
      goto Done;
    }

    Status = EfiCompress (SrcBuffer, SrcSize, DstBuffer, &DstSize);
  }

  if (EFI_ERROR (Status)) {
    printf ("Compress Error\n");
    goto Done;
  }

  printf ("\nOrig Size = %ld\n", SrcSize);
  printf ("Comp Size = %ld\n", DstSize);

  if (DstBuffer == NULL) {
    printf ("No destination to write to.\n");
    goto Done;
  }
  //
  // Write out the result
  //
  if (fwrite (DstBuffer, 1, DstSize, outfile) != DstSize) {
    printf ("Can't write to destination file\n");
  }

Done:
  if (SrcBuffer) {
    free (SrcBuffer);
  }

  if (DstBuffer) {
    free (DstBuffer);
  }

  if (infile) {
    fclose (infile);
  }

  if (outfile) {
    fclose (outfile);
  }

  return 0;
}
