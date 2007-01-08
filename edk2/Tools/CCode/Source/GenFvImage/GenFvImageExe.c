/*++

Copyright (c) 2004-2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GenFvImageExe.c

Abstract:

  This contains all code necessary to build the GenFvImage.exe utility.       
  This utility relies heavily on the GenFvImage Lib.  Definitions for both
  can be found in the Tiano Firmware Volume Generation Utility 
  Specification, review draft.

--*/

//
// File included in build
//
#include "GenFvImageExe.h"
#include "CommonLib.h"
#include "EfiUtilityMsgs.h"

static
void 
Version(
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
  printf ("%s v%d.%d -Tiano Firmware Volume Generation Utility.\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION);
  printf ("Copyright (c) 1999-2007 Intel Corporation. All rights reserved.\n");
}
 

static
void 
Usage(
  void
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
  
  printf ("\nUsage: %s -I FvInfFileName\n", UTILITY_NAME);
  printf ("  Where:\n");
  printf ("    FvInfFileName is the name of the image description file.\n\n");
}

int
main (
  IN INTN   argc,
  IN CHAR8  **argv
  )
/*++

Routine Description:

  This utility uses GenFvImage.Lib to build a firmware volume image.

Arguments:

  FvInfFileName      The name of an FV image description file.

  Arguments come in pair in any order.
    -I FvInfFileName 

Returns:

  EFI_SUCCESS            No error conditions detected.
  EFI_INVALID_PARAMETER  One or more of the input parameters is invalid.
  EFI_OUT_OF_RESOURCES   A resource required by the utility was unavailable.  
                         Most commonly this will be memory allocation 
                         or file creation.
  EFI_LOAD_ERROR         GenFvImage.lib could not be loaded.
  EFI_ABORTED            Error executing the GenFvImage lib.

--*/
{
  EFI_STATUS  Status;
  CHAR8       InfFileName[_MAX_PATH];
  CHAR8       *InfFileImage;
  UINTN       InfFileSize;
  UINT8       *FvImage;
  UINTN       FvImageSize;
  UINT8       Index;
  CHAR8       FvFileNameBuffer[_MAX_PATH];
  CHAR8       *FvFileName;
  FILE        *FvFile;
  FILE        *SymFile;
  CHAR8       SymFileNameBuffer[_MAX_PATH];
  CHAR8       *SymFileName;
  UINT8       *SymImage;
  UINTN       SymImageSize = 0;
  CHAR8       *CurrentSymString;

  FvFileName  = FvFileNameBuffer;
  SymFileName = SymFileNameBuffer;

  SetUtilityName (UTILITY_NAME);
  
  if (argc == 1) {
    Usage ();
    return STATUS_ERROR;
  }
  
  if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0) ||
      (strcmp(argv[1], "-?") == 0) || (strcmp(argv[1], "/?") == 0)) {
    Usage();
    return GetUtilityStatus ();
  }
  
  if ((strcmp(argv[1], "-V") == 0) || (strcmp(argv[1], "--version") == 0)) {
    Version();
    return GetUtilityStatus ();
  }
  
  //
  // Verify the correct number of arguments
  //
  if (argc != MAX_ARGS) {
    Error (NULL, 0, 0, "invalid number of input parameters specified", NULL);
    Usage ();
    return GetUtilityStatus ();
  }
  //
  // Initialize variables
  //
  strcpy (InfFileName, "");

  //
  // Parse the command line arguments
  //
  for (Index = 1; Index < MAX_ARGS; Index += 2) {
    //
    // Make sure argument pair begin with - or /
    //
    if (argv[Index][0] != '-' && argv[Index][0] != '/') {
      Error (NULL, 0, 0, argv[Index], "argument pair must begin with \"-\" or \"/\"");
      Usage ();
      return GetUtilityStatus ();
    }
    //
    // Make sure argument specifier is only one letter
    //
    if (argv[Index][2] != 0) {
      Error (NULL, 0, 0, argv[Index], "unrecognized argument");
      Usage ();
      return GetUtilityStatus ();
    }
    //
    // Determine argument to read
    //
    switch (argv[Index][1]) {

    case 'I':
    case 'i':
      if (strlen (InfFileName) == 0) {
        strcpy (InfFileName, argv[Index + 1]);
      } else {
        Error (NULL, 0, 0, argv[Index + 1], "FvInfFileName may only be specified once");
        Usage ();
        return GetUtilityStatus ();
      }
      break;

    default:
      Error (NULL, 0, 0, argv[Index], "unrecognized argument");
      Usage ();
      return GetUtilityStatus ();
      break;
    }
  }
  //
  // Read the INF file image
  //
  Status = GetFileImage (InfFileName, &InfFileImage, &InfFileSize);
  if (EFI_ERROR (Status)) {
    return STATUS_ERROR;
  }
  //
  // Call the GenFvImage lib
  //
  Status = GenerateFvImage (
            InfFileImage,
            InfFileSize,
            &FvImage,
            &FvImageSize,
            &FvFileName,
            &SymImage,
            &SymImageSize,
            &SymFileName
            );

  //
  // free InfFileImage memory
  //
  free (InfFileImage);

  if (EFI_ERROR (Status)) {
    switch (Status) {

    case EFI_INVALID_PARAMETER:
      Error (NULL, 0, 0, "invalid parameter passed to GenFvImage Lib", NULL);
      return GetUtilityStatus ();
      break;

    case EFI_ABORTED:
      Error (NULL, 0, 0, "error detected while creating the file image", NULL);
      return GetUtilityStatus ();
      break;

    case EFI_OUT_OF_RESOURCES:
      Error (NULL, 0, 0, "GenFvImage Lib could not allocate required resources", NULL);
      return GetUtilityStatus ();
      break;

    case EFI_VOLUME_CORRUPTED:
      Error (NULL, 0, 0, "no base address was specified, but the FV.INF included a PEI or BSF file", NULL);
      return GetUtilityStatus ();
      break;

    case EFI_LOAD_ERROR:
      Error (NULL, 0, 0, "could not load FV image generation library", NULL);
      return GetUtilityStatus ();
      break;

    default:
      Error (NULL, 0, 0, "GenFvImage Lib returned unknown status", "status returned = 0x%X", Status);
      return GetUtilityStatus ();
      break;
    }
  }
  //
  // Write file
  //
  FvFile = fopen (FvFileName, "wb");
  if (FvFile == NULL) {
    Error (NULL, 0, 0, FvFileName, "could not open output file");
    free (FvImage);
    free (SymImage);
    return GetUtilityStatus ();
  }

  if (fwrite (FvImage, 1, FvImageSize, FvFile) != FvImageSize) {
    Error (NULL, 0, 0, FvFileName, "failed to write to output file");
    free (FvImage);
    free (SymImage);
    fclose (FvFile);
    return GetUtilityStatus ();
  }

  fclose (FvFile);
  free (FvImage);

  //
  // Write symbol file
  //
  if (strcmp (SymFileName, "")) {
    SymFile = fopen (SymFileName, "wt");
    if (SymFile == NULL) {
      Error (NULL, 0, 0, SymFileName, "could not open output symbol file");
      free (SymImage);
      return GetUtilityStatus ();
    }

    fprintf (SymFile, "TEXTSYM format | V1.0\n");

    CurrentSymString = SymImage;
    while (((UINTN) CurrentSymString - (UINTN) SymImage) < SymImageSize) {
      fprintf (SymFile, "%s", CurrentSymString);
      CurrentSymString = (CHAR8 *) (((UINTN) CurrentSymString) + strlen (CurrentSymString) + 1);
    }

    fclose (SymFile);
  }

  free (SymImage);

  return GetUtilityStatus ();
}
