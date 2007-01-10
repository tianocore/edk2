/*++

Copyright (c)  1999-2006 Intel Corporation. All rights reserved
This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

    SecFixup.c

Abstract:

    This utility is part of build process for IA32 SEC FFS file.
    
    It fixup the reset vector data. The reset vector data binary file
    will be wrapped as a RAW section and be located immediately after
    the PE/TE section.

    The SEC EXE file can be either PE or TE file.
    
--*/

#include <stdio.h>

#include <Common/UefiBaseTypes.h>
#include <Common/EfiImage.h>
#include <Common/FirmwareVolumeImageFormat.h>

#include "EfiUtilityMsgs.c"
#include "SecFixup.h"

VOID
Version (
  VOID
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
  printf ("%s v%d.%d -Tiano IA32 SEC Fixup Utility.\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION);
  printf ("Copyright (c) 1999-2007 Intel Corporation. All rights reserved.\n");
}

VOID
Usage (
  VOID
  )
/*++

Routine Description:

  Displays the utility usage syntax to STDOUT

Arguments:

  None

Returns:

  None

--*/
{
  Version();
  
  printf ("\nUsage: %s SecExeFile ResetVectorDataFile OutputFile\n", UTILITY_NAME);
  printf ("  Where:\n");
  printf ("     SecExeFile           - Name of the IA32 SEC EXE file.\n");
  printf ("     ResetVectorDataFile  - Name of the reset vector data binary file.\n");
  printf ("     OutputFileName       - Name of the output file.\n");
}

STATUS
main (
  IN INTN   argc,
  IN CHAR8  **argv
  )
/*++

Routine Description:

  Main function.

Arguments:

  argc - Number of command line parameters.
  argv - Array of pointers to parameter strings.

Returns:
  STATUS_SUCCESS - Utility exits successfully.
  STATUS_ERROR   - Some error occurred during execution.

--*/
{
  FILE    *FpIn;

  FILE    *FpOut;
  UINT32  AddressOfEntryPoint;
  INT32   DestRel;
  STATUS  Status;
  UINT32  SecFileSize;

  SetUtilityName (UTILITY_NAME);

  if (argc == 1) {
    Usage();
    return STATUS_ERROR;
  }
    
  if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0) ||
      (strcmp(argv[1], "-?") == 0) || (strcmp(argv[1], "/?") == 0)) {
    Usage();
    return STATUS_ERROR;
  }
  
  if ((strcmp(argv[1], "-V") == 0) || (strcmp(argv[1], "--version") == 0)) {
    Version();
    return STATUS_ERROR;
  }
  
  //
  // Verify the correct number of arguments
  //
  if (argc != MAX_ARGS) {
    Error (NULL, 0, 0, "invalid number of input parameters specified", NULL);
    Usage ();
    return STATUS_ERROR;
  }
  //
  // Open the SEC exe file
  //
  if ((FpIn = fopen (argv[1], "rb")) == NULL) {
    Error (NULL, 0, 0, "Unable to open file", argv[1]);
    return STATUS_ERROR;
  }
  //
  // Get the entry point of the EXE file
  //
  Status = GetEntryPoint (FpIn, &AddressOfEntryPoint);
  if (Status != STATUS_SUCCESS) {
    fclose (FpIn);
    return STATUS_ERROR;
  }
  //
  // Get the SEC file size
  //
  fseek (FpIn, 0, SEEK_END);
  SecFileSize = ftell (FpIn);

  //
  // Close the SEC file
  //
  fclose (FpIn);

  //
  // Open the reset vector data file
  //
  if ((FpIn = fopen (argv[2], "rb")) == NULL) {
    Error (NULL, 0, 0, "Unable to open file", argv[2]);
    return STATUS_ERROR;
  }
  //
  // Open the output file
  //
  if ((FpOut = fopen (argv[3], "w+b")) == NULL) {
    Error (NULL, 0, 0, "Unable to open file", argv[3]);
    fclose (FpIn);
    return STATUS_ERROR;
  }
  //
  // Copy the input file to the output file
  //
  if (CopyFile (FpIn, FpOut) != STATUS_SUCCESS) {
    fclose (FpIn);
    fclose (FpOut);
    return STATUS_ERROR;
  }
  //
  // Close the reset vector data file
  //
  fclose (FpIn);

  //
  // Fix the destination relative in the jmp instruction
  // in the reset vector data structure
  //
  fseek (FpOut, -DEST_REL_OFFSET, SEEK_END);
  DestRel = AddressOfEntryPoint - (SecFileSize + sizeof (EFI_COMMON_SECTION_HEADER) + (UINT32) (ftell (FpOut)) + 2);
  if (DestRel <= -65536) {
    Error (NULL, 0, 0, "The SEC EXE file size is too big", NULL);
    fclose (FpOut);
    return STATUS_ERROR;
  }

  if (fwrite (&DestRel, sizeof (UINT16), 1, FpOut) != 1) {
    Error (NULL, 0, 0, "Failed to write to the output file", NULL);
    fclose (FpOut);
    return STATUS_ERROR;
  }
  //
  // Close the output file
  //
  fclose (FpOut);

  return STATUS_SUCCESS;
}

STATUS
GetEntryPoint (
  IN  FILE   *ExeFile,
  OUT UINT32 *EntryPoint
  )
/*++

Routine Description:

  Get the address of the entry point of a PE/TE file.

Arguments:

  PeFile     - File pointer to the specified PE/TE file.
  EntryPoint - Buffer for the address of the entry point to be returned.

Returns:
  STATUS_SUCCESS - Function completed successfully.
  STATUS_ERROR   - Error occured.

--*/
// GC_TODO:    ExeFile - add argument and description to function comment
{
  EFI_IMAGE_DOS_HEADER    DosHeader;
  EFI_IMAGE_NT_HEADERS32  NtHeader;
  EFI_TE_IMAGE_HEADER     TeHeader;

  //
  // Check if it is a TE file
  //
  fseek (ExeFile, 0, SEEK_SET);
  //
  // Attempt to read the TE header
  //
  if (fread (&TeHeader, sizeof (TeHeader), 1, ExeFile) == 1) {
    if (TeHeader.Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
      if (TeHeader.Machine != EFI_IMAGE_MACHINE_IA32) {
        Error (NULL, 0, 0, "The SEC file is PE but is not PE32 for IA32", NULL);
        return STATUS_ERROR;
      }

      *EntryPoint = TeHeader.AddressOfEntryPoint + sizeof (EFI_TE_IMAGE_HEADER) - TeHeader.StrippedSize;
      return STATUS_SUCCESS;
    }
  }
  //
  // Check if it is a PE file
  //
  fseek (ExeFile, 0, SEEK_SET);
  //
  // Attempt to read the DOS header
  //
  if (fread (&DosHeader, sizeof (DosHeader), 1, ExeFile) != 1) {
    goto InvalidFile;
  }
  //
  // Check the magic number
  //
  if (DosHeader.e_magic != EFI_IMAGE_DOS_SIGNATURE) {
    goto InvalidFile;
  }
  //
  // Position into the file and read the NT PE header
  //
  fseek (ExeFile, (long) DosHeader.e_lfanew, SEEK_SET);
  if (fread (&NtHeader, sizeof (NtHeader), 1, ExeFile) != 1) {
    goto InvalidFile;
  }
  //
  // Check the PE signature in the header
  //
  if (NtHeader.Signature != EFI_IMAGE_NT_SIGNATURE) {
    goto InvalidFile;
  }
  //
  // Make sure the PE file is PE32 for IA32
  //
  if (NtHeader.FileHeader.Machine != EFI_IMAGE_MACHINE_IA32 ||
      NtHeader.OptionalHeader.Magic != EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC
      ) {
    Error (NULL, 0, 0, "The SEC file is PE but is not PE32 for IA32", NULL);
    return STATUS_ERROR;
  }
  //
  // Get the entry point from the optional header
  //
  *EntryPoint = NtHeader.OptionalHeader.AddressOfEntryPoint;
  return STATUS_SUCCESS;

InvalidFile:
  Error (NULL, 0, 0, "The SEC file is neither PE nor TE file", NULL);
  return STATUS_ERROR;
}

STATUS
CopyFile (
  FILE    *FpIn,
  FILE    *FpOut
  )
/*++

Routine Description:

  Copy file.

Arguments:

  FpIn  - File pointer to the source file.
  FpOut - File pointer to the destination file.

Returns:
  STATUS_SUCCESS - Function completed successfully.
  STATUS_ERROR   - Error occured.

--*/
{
  INTN  FileSize;

  INTN  Offset;

  INTN  Length;
  UINT8 Buffer[BUF_SIZE];

  fseek (FpIn, 0, SEEK_END);
  FileSize = ftell (FpIn);

  fseek (FpIn, 0, SEEK_SET);
  fseek (FpOut, 0, SEEK_SET);

  Offset = 0;
  while (Offset < FileSize) {
    Length = sizeof (Buffer);
    if (FileSize - Offset < Length) {
      Length = FileSize - Offset;
    }

    if (fread (Buffer, Length, 1, FpIn) != 1 || fwrite (Buffer, Length, 1, FpOut) != 1) {
      Error (NULL, 0, 0, "Copy file error", NULL);
      return STATUS_ERROR;
    }

    Offset += Length;
  }

  return STATUS_SUCCESS;
}
