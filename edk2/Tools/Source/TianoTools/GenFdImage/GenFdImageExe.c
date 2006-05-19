/*++

Copyright (c)  1999 - 2002 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  GenFdImageExe.c

Abstract:

  This contains all code necessary to build the GenFdImage.exe utility.
  This utility relies heavily on the GenFdImage Lib.  Definitions for both     
  can be found in the GenFdImage Utility Specification, review draft.

--*/

//
// Coded to EFI 2.0 Coding Standards
//
//
// Include files
//
#include "GenFdImage.h"
#include "GenFdImageExe.h"

VOID
PrintUtilityInfo (
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
  printf (
    "%s, EFI 2.0 Firmware Device Image Generation Utility. ""Version %i.%i, %s.\n\n",
    UTILITY_NAME,
    UTILITY_MAJOR_VERSION,
    UTILITY_MINOR_VERSION,
    UTILITY_DATE
    );
}

VOID
PrintUsage (
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
  printf (
    "Usage: %s -B BaseAddress -S Size -F FillByte"" [-I FvInfFileName] -O OutputFileName \n",
    UTILITY_NAME
    );
  printf ("  Where:\n");
  printf ("\tBaseAddress is the starting address of the FD Image\n\n");
  printf ("\tSize is the size of the FD Image.\n\n");
  printf ("\tFillByte is the desired value of free space in the Image\n\n");
  printf ("\tFvInfFileName is the name of an FV Image description file.\n\n");
  printf ("\tOutputFileName is the desired output file name.\n\n");
}

EFI_STATUS
main (
  IN INTN   argc,
  IN CHAR8  **argv
  )
/*++

Routine Description:

  This utility uses GenFdImage.lib to build a Firmware Device Image
  which will include several Firmware Volume Images.

Arguments:

  Base Address      Base Address of the firmware volume..
  Size              Size of the Firmware Volume
  FillByte          The byte value which would be needed to pad
                    between various Firmware Volumes
  FvInfFile         Fv inf file
  OutputFileName    The name of output file which would be created

Returns:

  EFI_SUCCESS            No error conditions detected.
  EFI_INVALID_PARAMETER  One or more of the input parameters is invalid.
  EFI_OUT_OF_RESOURCES   A resource required by the utility was unavailable.  
                         Most commonly this will be memory allocation or 
                         file creation.
  EFI_LOAD_ERROR         GenFvImage.lib could not be loaded.
  EFI_ABORTED            Error executing the GenFvImage lib.

--*/
// GC_TODO:    argc - add argument and description to function comment
// GC_TODO:    argv - add argument and description to function comment
{
  UINTN       Index;
  UINTN       FvFilesCount;

  UINT8       i;

  UINT64      StartAddress;
  UINT64      Size;
  UINT64      FillByteVal;

  CHAR8       **FvInfFileList;
  CHAR8       **OrgFvInfFileList;
  CHAR8       OutputFileName[_MAX_PATH];

  EFI_STATUS  Status;

  INTN        arg_counter;

  //
  //  Echo for makefile debug
  //
  printf ("\n\n");
  for (arg_counter = 0; arg_counter < argc; arg_counter++) {
    printf ("%s ", argv[arg_counter]);
  }

  printf ("\n\n");

  //
  // Display utility information
  //
  PrintUtilityInfo ();

  //
  // Verify the correct number of arguments
  //
  if (argc < MIN_ARGS) {
    printf ("ERROR: missing 1 or more input arguments.\n\n");
    PrintUsage ();
    return EFI_INVALID_PARAMETER;
  }
  //
  // Initialize variables
  //
  StartAddress  = 0;
  Size          = 0;
  FillByteVal   = 0;
  FvFilesCount  = 0;

  for (i = 1; i < argc; i++) {
    if (stricmp (argv[i], "-I") == 0) {
      FvFilesCount++;
    }
  }

  FvInfFileList = malloc (sizeof (UINTN) * (FvFilesCount + 1));
  if (FvInfFileList == NULL) {
    printf ("ERROR: allocating memory for FvInfFileList in -main- function.\n");
    return EFI_OUT_OF_RESOURCES;
  }

  memset (FvInfFileList, 0, sizeof (UINTN) * (FvFilesCount + 1));

  OrgFvInfFileList = FvInfFileList;

  for (Index = 0; Index < FvFilesCount; Index++) {
    *FvInfFileList = malloc (_MAX_PATH);
    memset (*FvInfFileList, 0, _MAX_PATH);
    FvInfFileList++;
  }

  strcpy (OutputFileName, "");

  //
  // Parse the command line arguments
  //
  FvInfFileList = OrgFvInfFileList;

  for (i = 1; i < argc; i += 2) {
    //
    // Make sure argument pair begin with - or /
    //
    if (argv[i][0] != '-' && argv[i][0] != '/') {
      PrintUsage ();
      printf ("ERROR: Argument pair must begin with \"-\" or \"/\"\n");
      return EFI_INVALID_PARAMETER;
    }
    //
    // Make sure argument specifier is only one letter
    //
    if (argv[i][2] != 0) {
      PrintUsage ();
      printf ("ERROR: Unrecognized argument \"%s\".\n", argv[i]);
      return EFI_INVALID_PARAMETER;
    }
    //
    // Determine argument to read
    //
    switch (argv[i][1]) {

    case 'I':
    case 'i':
      if ((FvInfFileList != NULL) && (strlen (*FvInfFileList) == 0)) {
        strcpy (*FvInfFileList, argv[i + 1]);
        FvInfFileList++;
      } else {
        printf ("ERROR: FvInfFile Name is more than specifed.\n");
        return EFI_INVALID_PARAMETER;
      }
      break;

    case 'O':
    case 'o':
      if (strlen (OutputFileName) == 0) {
        strcpy (OutputFileName, argv[i + 1]);
      } else {
        PrintUsage ();
        printf ("ERROR: OutputFileName may only be specified once.\n");
        return EFI_INVALID_PARAMETER;
      }
      break;

    case 'B':
    case 'b':
      Status = AsciiStringToUint64 (argv[i + 1], FALSE, &StartAddress);
      if (Status != EFI_SUCCESS) {
        printf ("\nERROR: Bad FD Image start address specified");
        return EFI_INVALID_PARAMETER;
      }
      break;

    case 'S':
    case 's':
      Status = AsciiStringToUint64 (argv[i + 1], FALSE, &Size);
      if (Status != EFI_SUCCESS) {
        printf ("\nERROR: Bad FD Image size specified");
        return EFI_INVALID_PARAMETER;
      }
      break;

    case 'F':
    case 'f':
      Status = AsciiStringToUint64 (argv[i + 1], FALSE, &FillByteVal);
      if (Status != EFI_SUCCESS) {
        printf ("\nERROR: Not a recognized Fill Byte value");
        return EFI_INVALID_PARAMETER;
      }
      break;

    default:
      PrintUsage ();
      printf ("ERROR: Unrecognized argument \"%s\".\n", argv[i]);
      return EFI_INVALID_PARAMETER;
      break;
    }
  }
  //
  // Call the GenFdImage Lib
  //
  FvInfFileList = OrgFvInfFileList;

  Status = GenerateFdImage (
            StartAddress,
            Size,
            (UINT8) FillByteVal,
            OutputFileName,
            FvInfFileList
            );

  if (EFI_ERROR (Status)) {
    switch (Status) {

    case EFI_INVALID_PARAMETER:
      printf ("\nERROR: Invalid parameter passed to GenFdImage lib.\n");
      break;

    case EFI_ABORTED:
      printf ("\nERROR: Error detected while creating the file image.\n");
      break;

    case EFI_OUT_OF_RESOURCES:
      printf ("\nERROR: GenFdImage Lib could not allocate required resources.\n");
      break;

    case EFI_VOLUME_CORRUPTED:
      printf ("\nERROR: No base address was specified \n");
      break;

    case EFI_LOAD_ERROR:
      printf ("\nERROR: An error occurred loading one of the required support Lib.\n");
      break;

    default:
      printf ("\nERROR: GenFdImage lib returned unknown status %X.\n", Status);
      break;
    }

    return Status;
  }

  return EFI_SUCCESS;
}
