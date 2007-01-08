/*++

Copyright (c)  1999-2006 Intel Corporation. All rights reserved
This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  ModifyInf.c

Abstract:

  It is a simple tool to modify some fields in a FV inf file 
  and output a new FV inf file.  

--*/

#include "stdio.h"
#include "string.h"

#define UTILITY_NAME "ModifyInf"
#define UTILITY_MAJOR_VERSION 1
#define UTILITY_MINOR_VERSION 1

//
// Read a line into buffer including '\r\n'
//
int
ReadLine (
  char *LineBuffer,
  FILE *fp
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  LineBuffer  - GC_TODO: add argument description
  fp          - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  int   CharC;
  char  *Line;

  Line = LineBuffer;

  while ((CharC = fgetc (fp)) != EOF) {
    *Line++ = (char) CharC;
    if (CharC == 0x0a) {
      break;
    }
  }

  *Line = 0;

  if (CharC == EOF) {
    return 0;
  } else {
    return 1;
  }

}
//
// Write a line into output file
//
int
WriteLine (
  char *Line,
  FILE *fp
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Line  - GC_TODO: add argument description
  fp    - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  fwrite (Line, strlen (Line), 1, fp);
  return 0;
}
//
// Apply patterns to a line
// Currently there are 2 patterns to support
// '==' replace a field value with a new value
// '+=' append a string at the end of original line
// '-'  prevent the line from applying any patterns
//      it has the highest priority
//
int
ApplyPattern (
  char *Line,
  char *argv[],
  int  argc
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Line  - GC_TODO: add argument description
  ]     - GC_TODO: add argument description
  argc  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  static char Section[256];
  char        PatternBuffer[256];
  char        *Pattern;
  char        *Pattern1;
  char        *Pattern2;
  int         PatternNum;
  char        *Ptr;

  Pattern     = PatternBuffer;

  PatternNum  = argc;

  //
  // For section field
  // record current scope section into static buffer
  //
  Ptr = Line;
  if (*Ptr == '[') {
    while (*Ptr != ']') {
      if (!(*Ptr++)) {
        return -1;
      }
    }

    strcpy (Section, Line);
    Section[Ptr - Line + 1] = 0;
  }
  //
  // Apply each pattern on the line
  //
  while (PatternNum-- > 3) {

    strcpy (Pattern, argv[PatternNum]);

    //
    // For pattern '-'
    // keep it unmodified by other patterns
    //
    if (*Pattern == '-') {
      if (strstr (Line, Pattern + 1)) {
        return 0;
      } else {
        continue;
      }
    }
    //
    // For other patterns
    // get its section at first if it has
    //
    if (*Pattern == '[') {
      if (strncmp (Section, Pattern, strlen (Section))) {
        //
        // This pattern can't be appied for current section
        //
        continue;
      }
      //
      // Strip the section field
      //
      while (*Pattern != ']') {
        if (!(*Pattern++)) {
          return -1;
        }
      }

      Pattern++;
    }
    //
    // Apply patterns
    //
    Pattern1  = strstr (Pattern, "==");
    Pattern2  = strstr (Pattern, "+=");
    if (Pattern1) {
      //
      // For pattern '=='
      // replace the field value with a new string
      //
      if (!strncmp (Line, Pattern, Pattern1 - Pattern)) {
        Pattern1 += 2;
        Ptr = strstr (Line, "=");
        if (!Ptr) {
          return -1;
        }

        while (*(++Ptr) == ' ')
          ;
        *Ptr = 0;
        strcat (Line, Pattern1);
        strcat (Line, "\r\n");
      }
    } else if (Pattern2) {
      //
      // For pattern '+='
      // append a string at end of the original string
      //
      if (!strncmp (Line, Pattern, Pattern2 - Pattern)) {
        Pattern2 += 2;
        Ptr = Line;
        while (*Ptr != 0x0D && *Ptr != 0x0A) {
          Ptr++;
        }

        *Ptr = 0;
        strcat (Line, Pattern2);
        strcat (Line, "\r\n");
      }
    }
  }

  return 0;
}

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
  printf ("%s v%d.%d -EDK Modify fields in FV inf files.\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION);
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
  printf ("\nUsage: %s InputFile OutputFile Pattern_String [Pattern_String бн]\n\
   Where: \n\
     Pattern_String is of the format (note that the section name must be \n\
     enclosed within square brackets):\n\
         [section]FieldKey<op>Value [(FieldKey<op>Value) бн] \n\
     The operator, <op>, must be one of the following: \n\
         '==' replace a field value with a new value \n\
         '+=' append a string at the end of original line \n\
         '-'  prevent the line from applying any patterns \n\
     Example: \n\
         ModifyInf BuildRootFvFvMain.inf BuildRootFvFvMainEXP.inf \\ \n\
               [files]EFI_FILE_NAME+=.Org EFI_NUM_BLOCKS==0x20 \\ \n\
               [options]EFI_FILENAME==FcMainCompact.fv -DpsdSignature.dxe \n", UTILITY_NAME);
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
  char  LineBuffer[256];
  FILE  *fpin;
  FILE  *fpout;

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

  fpin = fopen (argv[1], "rb");
  if (!fpin) {
    printf ("Can't open input file!\r\n");
    return -1;
  }

  fpout = fopen (argv[2], "wb");
  if (!fpout) {
    fclose (fpin);
    printf ("Can't create output file!\r\n");
    return -1;
  }

  while (ReadLine (LineBuffer, fpin)) {
    ApplyPattern (LineBuffer, argv, argc);
    WriteLine (LineBuffer, fpout);
  }

  fclose (fpin);
  fclose (fpout);

  return 0;
}
