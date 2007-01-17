/*

Copyright (c)  1999-2006 Intel Corporation. All rights reserved
This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

// GC_TODO: fix comment to start with /*++
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

//
// Utility Name
//
#define UTILITY_NAME  "SplitFile"

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
  printf ("%s v%d.%d -Utility to break a file into two pieces at the request offset.\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION);
  printf ("Copyright (c) 1999-2007 Intel Corporation. All rights reserved.\n");
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
   SplitFile Filename Offset\n\
     where:\n\
       Filename: Input file to split\n\
       Offset: offset at which to split file\n\
   The output files will be named <Filename>1 and <Filename>2 with \n\
   <Filename> being given as the input file name.\n");
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
  FILE          *In;

  FILE          *Out1;

  FILE          *Out2;
  char          OutName1[512];
  char          OutName2[512];
  unsigned long Index;
  unsigned long splitpoint;
  char          CharC;

  if (argc == 1) {
    Usage();
    return -1;
  }
    
  if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0) ||
      (strcmp(argv[1], "-?") == 0) || (strcmp(argv[1], "/?") == 0)) {
    Usage();
    return -1;
  }
  
  if ((strcmp(argv[1], "-V") == 0) || (strcmp(argv[1], "--version") == 0)) {
    Version();
    return -1;
  }
  
  if (argc != 3) {
    Usage ();
    return -1;
  }

  In = fopen (argv[1], "rb");
  if (In == NULL) {
    printf ("Unable to open file \"%s\"\n", argv[1]);
    return -1;
  }

  strncpy (OutName1, argv[1], 510);
  strncpy (OutName2, argv[1], 510);
  strcat (OutName1, "1");
  strcat (OutName2, "2");

  Out1 = fopen (OutName1, "wb");
  if (Out1 == NULL) {
    printf ("Unable to open file \"%s\"\n", OutName1);
    return -1;
  }

  Out2 = fopen (OutName2, "wb");
  if (Out2 == NULL) {
    printf ("Unable to open file \"%s\"\n", OutName2);
    return -1;
  }

  splitpoint = atoi (argv[2]);

  for (Index = 0; Index < splitpoint; Index++) {
    CharC = (char) fgetc (In);
    if (feof (In)) {
      break;
    }

    fputc (CharC, Out1);
  }

  for (;;) {
    CharC = (char) fgetc (In);
    if (feof (In)) {
      break;
    }

    fputc (CharC, Out2);
  }

  fclose (In);
  fclose (Out1);
  fclose (Out2);

  return 0;
}
