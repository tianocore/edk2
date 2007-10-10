/*++

Copyright (c)  2004-2006 Intel Corporation. All rights reserved
This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Microcode.c

Abstract:

  Utility for working with microcode patch files in the Intel 
  Platform Innovation Framework for EFI build environment.

--*/

#include <stdio.h>
#include <string.h> // for memset()
#include <ctype.h>
#include <stdlib.h> // for malloc()

#include "EfiUtilityMsgs.h"
#include "Microcode.h"

#define MAX_LINE_LEN  256

#define STATUS_IGNORE 0xA

//
// Structure definition for a microcode header
//
typedef struct {
  unsigned int  HeaderVersion;
  unsigned int  PatchId;
  unsigned int  Date;
  unsigned int  CpuId;
  unsigned int  Checksum;
  unsigned int  LoaderVersion;
  unsigned int  PlatformId;
  unsigned int  DataSize;   // if 0, then TotalSize = 2048, and TotalSize field is invalid
  unsigned int  TotalSize;  // number of bytes
  unsigned int  Reserved[3];
} MICROCODE_IMAGE_HEADER;

static
STATUS
MicrocodeReadData (
  FILE          *InFptr,
  unsigned int  *Data
  );

void
MicrocodeConstructor (
  void
  )
/*++

Routine Description:

  Constructor of module Microcode

Arguments:

  None

Returns:

  None

--*/
{
}

void
MicrocodeDestructor (
  void
  )
/*++

Routine Description:

  Destructor of module Microcode

Arguments:

  None

Returns:

  None

--*/
{
}

static
STATUS
MicrocodeReadData (
  FILE          *InFptr,
  unsigned int  *Data
  )
/*++

Routine Description:
  Read a 32-bit microcode data value from a text file and convert to raw binary form.

Arguments:
  InFptr    - file pointer to input text file
  Data      - pointer to where to return the data parsed

Returns:
  STATUS_SUCCESS    - no errors or warnings, Data contains valid information
  STATUS_ERROR      - errors were encountered

--*/
{
  char  Line[MAX_LINE_LEN];
  char  *cptr;
  unsigned int  ctr;

  Line[MAX_LINE_LEN - 1]  = 0;
  *Data                   = 0;
  if (fgets (Line, MAX_LINE_LEN, InFptr) == NULL) {
    return STATUS_ERROR;
  }
  //
  // If it was a binary file, then it may have overwritten our null terminator
  //
  if (Line[MAX_LINE_LEN - 1] != 0) {
    return STATUS_ERROR;
  }

  // Strip leading white-space characters (except carriage returns) from Line
  //
  if (isspace(Line[0]) && Line[0] != '\n') {
    while (isspace(Line[0])) {
       for (ctr = 0; ctr < strlen(Line); ctr++)
         if (Line[ctr] != '\n')
           Line[ctr] = Line[ctr + 1];
    }
  }

  //
  // Look for
  // dd 000000001h ; comment
  // dd XXXXXXXX
  // DD  XXXXXXXXX
  //  DD XXXXXXXXX
  //
  for (cptr = Line; *cptr && isspace(*cptr); cptr++) {
  }
  if ((tolower(cptr[0]) == 'd') && (tolower(cptr[1]) == 'd') && isspace (cptr[2])) {
    //
    // Skip blanks and look for a hex digit
    //
    cptr += 3;
    for (; *cptr && isspace(*cptr); cptr++) {
    }
    if (isxdigit (*cptr)) {
      if (sscanf (cptr, "%X", Data) != 1) {
        return STATUS_ERROR;
      }
    }
    return STATUS_SUCCESS;
  }
  if (strlen(Line) == 1) {
    return STATUS_IGNORE;
  }
  if (tolower(cptr[0]) == ';') {
    return STATUS_IGNORE;
  }
  return STATUS_ERROR;
}

STATUS
MicrocodeParseFile (
  char  *InFileName,
  char  *OutFileName
  )
/*++

Routine Description:
  Parse a microcode text file, and write the binary results to an output file.

Arguments:
  InFileName  - input text file to parse
  OutFileName - output file to write raw binary data from parsed input file

Returns:
  STATUS_SUCCESS    - no errors or warnings
  STATUS_ERROR      - errors were encountered

--*/
{
  FILE                    *InFptr;
  FILE                    *OutFptr;
  STATUS                  Status;
  MICROCODE_IMAGE_HEADER  *Header;
  unsigned int            Size;
  unsigned int            Size2;
  unsigned int            Data;
  unsigned int            Checksum;
  char                    *Buffer;
  char                    *Ptr;
  char                    *OrigPtr;
  unsigned int            TotalSize;

  Status  = STATUS_ERROR;
  InFptr  = NULL;
  OutFptr = NULL;
  Buffer  = NULL;
  //
  // Open the input text file
  //
  if ((InFptr = fopen (InFileName, "r")) == NULL) {
    Error (NULL, 0, 0, InFileName, "failed to open input microcode file for reading");
    return STATUS_ERROR;
  }
  //
  // Make two passes on the input file. The first pass is to determine how
  // much data is in the file so we can allocate a working buffer. Then
  // we'll allocate a buffer and re-read the file into the buffer for processing.
  //
  Size = 0;
  do {
    Status = MicrocodeReadData (InFptr, &Data);
    if (Status == STATUS_SUCCESS) {
      Size += sizeof (Data);
    }
    if (Status == STATUS_IGNORE) {
      Status = STATUS_SUCCESS;
    }
  } while (Status == STATUS_SUCCESS);
  //
  // Error if no data.
  //
  if (Size == 0) {
    Error (NULL, 0, 0, InFileName, "no parse-able data found in file");
    goto Done;
  }
  if (Size < sizeof (MICROCODE_IMAGE_HEADER)) {
    Error (NULL, 0, 0, InFileName, "amount of parse-able data is insufficient to contain a microcode header");
    goto Done;
  }
  //
  // Allocate a buffer for the data
  //
  Buffer = (char *) _malloc (Size);
  if (Buffer == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    goto Done;
  }
  //
  // Re-read the file, storing the data into our buffer
  //
  fseek (InFptr, 0, SEEK_SET);
  Ptr = Buffer;
  OrigPtr = Ptr;
  do {
    OrigPtr = Ptr;
    Status = MicrocodeReadData (InFptr, &Data);
    if (Status == STATUS_SUCCESS) {
      *(unsigned int *) Ptr = Data;
      Ptr += sizeof (Data);
    }
    if (Status == STATUS_IGNORE) {
      Ptr = OrigPtr;
      Status = STATUS_SUCCESS;
    }
  } while (Status == STATUS_SUCCESS);
  //
  // Can't do much checking on the header because, per the spec, the
  // DataSize field may be 0, which means DataSize = 2000 and TotalSize = 2K,
  // and the TotalSize field is invalid (actually missing). Thus we can't
  // even verify the Reserved fields are 0.
  //
  Header = (MICROCODE_IMAGE_HEADER *) Buffer;
  if (Header->DataSize == 0) {
    TotalSize = 2048;
  } else {
    TotalSize = Header->TotalSize;
  }
  if (TotalSize != Size) {
    Error (NULL, 0, 0, InFileName, "file contents do not contain expected TotalSize 0x%04X", TotalSize);
    goto Done;
  }
  //
  // Checksum the contents
  //
  Ptr       = Buffer;
  Checksum  = 0;
  Size2     = 0;
  while (Size2 < Size) {
    Checksum += *(unsigned int *) Ptr;
    Ptr += 4;
    Size2 += 4;
  }
  if (Checksum != 0) {
    Error (NULL, 0, 0, InFileName, "checksum failed on file contents");
    goto Done;
  }
  //
  // Open the output file and write the buffer contents
  //
  if ((OutFptr = fopen (OutFileName, "wb")) == NULL) {
    Error (NULL, 0, 0, OutFileName, "failed to open output microcode file for writing");
    goto Done;
  }
  if (fwrite (Buffer, Size, 1, OutFptr) != 1) {
    Error (NULL, 0, 0, OutFileName, "failed to write microcode data to output file");
    goto Done;
  }
  Status = STATUS_SUCCESS;
Done:
  if (Buffer != NULL) {
    free (Buffer);
  }
  if (InFptr != NULL) {
    fclose (InFptr);
  }
  if (OutFptr != NULL) {
    fclose (OutFptr);
    if (Status == STATUS_ERROR) {
      remove (OutFileName);
    }
  }
  return Status;
}
