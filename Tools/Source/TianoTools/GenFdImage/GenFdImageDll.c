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
          GenFdImageDll.C

  Abstarct:
          This file contains the relevant functions required to complete
          the API to generate Firmware Device
--*/

// GC_TODO: fix comment to add:  Abstract:
//
// This tells the compiler to export the DLL functions
//
#define GEN_FD_IMAGE_EXPORTS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <io.h>
#include <assert.h>
#include "UefiBaseTypes.h"
#include "Base.h"
#include "GenFdImage.h"
// #include "GenFvImage.h"
#include "ParseInf.h"

//
// Global declaration
//
UINTN         ValidLineNum  = 0;

UINTN         NumFvFiles    = 0;
static UINT64 LastAddress   = 0;

CHAR8         **TokenStr;
CHAR8         **OrgStrTokPtr;

FDINFO        *FdInfo;
FDINFO        *OrgFdInfoPtr;

FVINFO        **FvInfo;
FVINFO        **OrgFvInfoPtr;

//
// Global function declarations
//
EFI_STATUS
BuildFirmwareDeviceBinaryFromFwVolumes (
  IN UINT64   FvBaseAddress,
  IN CHAR8    *FvFileName,
  IN CHAR8    *FdFileName
  );

INTN
CompareItems (
  IN  const VOID *Arg1,
  IN  const VOID *Arg2
  )
/*++
Description:

    This function is used by qsort to sort the Fv list based on FvBaseAddress

Input:
    Arg1
    Arg2

Return:

    None
--*/
// GC_TODO: function comment is missing 'Routine Description:'
// GC_TODO: function comment is missing 'Arguments:'
// GC_TODO: function comment is missing 'Returns:'
// GC_TODO:    Arg1 - add argument and description to function comment
// GC_TODO:    Arg2 - add argument and description to function comment
{
  if ((*(FVINFO **) Arg1)->FvBaseAddress > (*(FVINFO **) Arg2)->FvBaseAddress) {
    return 1;
  } else if ((*(FVINFO **) Arg1)->FvBaseAddress < (*(FVINFO **) Arg2)->FvBaseAddress) {
    return -1;
  } else {
    return 0;
  }
}

VOID
BuildTokenList (
  IN  CHAR8 *Token
  )
/*++
Description:

    This function builds the token list in an array which will be parsed later

Input:
    Token    String,

Return:

    None
--*/
// GC_TODO: function comment is missing 'Routine Description:'
// GC_TODO: function comment is missing 'Arguments:'
// GC_TODO: function comment is missing 'Returns:'
// GC_TODO:    Token - add argument and description to function comment
{

  strcpy (*TokenStr, Token);
  TokenStr++;
}

VOID
TrimLine (
  IN  CHAR8 *Line
  )
/*++
Description:

  This function cleans up the line by removing all whitespace and 
  comments

Input:

  Line    String,

Return:
    None
--*/
// GC_TODO: function comment is missing 'Routine Description:'
// GC_TODO: function comment is missing 'Arguments:'
// GC_TODO: function comment is missing 'Returns:'
// GC_TODO:    Line - add argument and description to function comment
{
  CHAR8 TmpLine[FILE_NAME_SIZE];
  CHAR8 c;
  CHAR8 *Ptr0;
  UINTN i;
  UINTN j;

  //
  // Change '#' to '//' for Comment style
  //
  //  if((Ptr0=strchr(Line, '#')) != NULL) {
  //
  if ((Ptr0 = strstr (Line, "//")) != NULL) {
    Line[Ptr0 - Line] = 0;
  }

  i = j = 0;

  while ((c = Line[i]) != 0) {
    if ((c != ' ') && (c != '\t') && (c != '\n')) {
      TmpLine[j++] = c;
    }

    i++;
  }

  TmpLine[j] = 0;
  strcpy (Line, TmpLine);
}

VOID
ValidLineCount (
  IN  FILE *Fp
  )
/*++

Description:

  This function calculated number of valid lines in a input file.
  
Input:

  Fp     Pointer to a file handle which has been opened.

Return:

  None
--*/
// GC_TODO: function comment is missing 'Routine Description:'
// GC_TODO: function comment is missing 'Arguments:'
// GC_TODO: function comment is missing 'Returns:'
// GC_TODO:    Fp - add argument and description to function comment
{
  CHAR8 Buff[FILE_NAME_SIZE];

  while (fgets (Buff, sizeof (Buff), Fp)) {
    TrimLine (Buff);
    if (Buff[0] == 0) {
      continue;
    }

    ValidLineNum++;
  }
}

VOID
ParseInputFile (
  IN  FILE *Fp
  )
/*++
  
Description:

  This function parses the input file and tokenize the string
  
Input:

  Fp     Pointer to a file handle which has been opened.
  
Return:

  None
--*/
// GC_TODO: function comment is missing 'Routine Description:'
// GC_TODO: function comment is missing 'Arguments:'
// GC_TODO: function comment is missing 'Returns:'
// GC_TODO:    Fp - add argument and description to function comment
{
  CHAR8 *Token;
  CHAR8 Buff[FILE_NAME_SIZE];
  CHAR8 OrgLine[FILE_NAME_SIZE];
  CHAR8 Str[FILE_NAME_SIZE];
  CHAR8 Delimit[] = "=";

  while (fgets (Buff, sizeof (Buff), Fp) != NULL) {
    strcpy (OrgLine, Buff);
    TrimLine (Buff);

    if (Buff[0] == 0) {
      continue;
    }

    Token = strtok (Buff, Delimit);

    while (Token != NULL) {
      strcpy (Str, Token);
      BuildTokenList (Str);
      Token = strtok (NULL, Delimit);
    }
  }
}

EFI_STATUS
InitializeComps (
  VOID
  )
/*++

Description:

  This function intializes the relevant global variable which is being
  used to store the information retrieved from INF file.
  
Input:

  None

Return:

  EFI_STATUS
--*/
// GC_TODO: function comment is missing 'Routine Description:'
// GC_TODO: function comment is missing 'Arguments:'
// GC_TODO: function comment is missing 'Returns:'
// GC_TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// GC_TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// GC_TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// GC_TODO:    EFI_SUCCESS - add return value to function comment
{
  UINTN Index;

  FdInfo = malloc (sizeof (FDINFO));

  if (FdInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  OrgFdInfoPtr  = FdInfo;

  FvInfo        = malloc (sizeof (int) * NumFvFiles);

  if (FvInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  OrgFvInfoPtr = FvInfo;

  for (Index = 0; Index < NumFvFiles; Index++) {
    *FvInfo = malloc (sizeof (FVINFO));

    if (*FvInfo == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    memset (*FvInfo, 0, sizeof (FVINFO));
    FvInfo++;
  }

  FvInfo = OrgFvInfoPtr;

  return EFI_SUCCESS;
}

VOID
InitializeInFileInfo (
  VOID
  )
/*++

Description:

  This function intializes the relevant global variable which is being
  used to store the information retrieved from INF file.

Input:

  NONE

Return:

  NONE
--*/
// GC_TODO: function comment is missing 'Routine Description:'
// GC_TODO: function comment is missing 'Arguments:'
// GC_TODO: function comment is missing 'Returns:'
{
  UINTN   OptionFlag;
  UINT64  StringValue;

  OptionFlag  = 0;
  TokenStr    = OrgStrTokPtr;

  while (*TokenStr != NULL) {
    if (stricmp (*TokenStr, "[options]") == 0) {
      OptionFlag = 1;
    }

    if (OptionFlag) {
      if (stricmp (*TokenStr, "EFI_FV_BASE_ADDRESS") == 0) {
        *TokenStr++;
        if (AsciiStringToUint64 (*TokenStr, FALSE, &StringValue) != EFI_SUCCESS) {
          printf ("\nERROR: Cannot determine the FV base address.");
          return ;
        }
        (*FvInfo)->FvBaseAddress = StringValue;
      } else if (stricmp (*TokenStr, "EFI_FV_FILE_NAME") == 0) {
        *TokenStr++;
        strcpy ((*FvInfo)->FvFile, *TokenStr);
      }
    }

    TokenStr++;
  }
}

EFI_STATUS
GetFvRelatedInfoFromInfFile (
  IN  CHAR8 *FileName
  )
/*++
  
Description:

  This function reads the input file, parse it and create a list of tokens
  which is parsed and used, to intialize the data related to Firmware Volume.
  
Input:

  FileName  FileName which needed to be read to parse data

Return:

  EFI_STATUS
    
--*/
// GC_TODO: function comment is missing 'Routine Description:'
// GC_TODO: function comment is missing 'Arguments:'
// GC_TODO: function comment is missing 'Returns:'
// GC_TODO:    FileName - add argument and description to function comment
// GC_TODO:    EFI_ABORTED - add return value to function comment
// GC_TODO:    EFI_ABORTED - add return value to function comment
// GC_TODO:    EFI_SUCCESS - add return value to function comment
{
  FILE  *Fp;
  UINTN Index;

  Fp = fopen (FileName, "r");

  if (Fp == NULL) {
    printf ("Error in opening %s file\n", FileName);
    return EFI_ABORTED;
  }

  ValidLineCount (Fp);

  if (ValidLineNum == 0) {
    printf ("\nFile doesn't contain any valid informations");
    return EFI_ABORTED;
  }

  TokenStr = (CHAR8 **) malloc (sizeof (UINTN) * (2 * ValidLineNum));
  memset (TokenStr, 0, sizeof (UINTN) * (2 * ValidLineNum));
  OrgStrTokPtr = TokenStr;

  for (Index = 0; Index < (2 * ValidLineNum); Index++) {
    *TokenStr = (CHAR8 *) malloc (sizeof (CHAR8) * FILE_NAME_SIZE);
    memset (*TokenStr, 0, FILE_NAME_SIZE);
    TokenStr++;
  }

  *TokenStr = NULL;
  TokenStr  = OrgStrTokPtr;
  fseek (Fp, 0L, SEEK_SET);

  ParseInputFile (Fp);
  InitializeInFileInfo ();

  if (Fp) {
    fclose (Fp);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
WriteFwBinary (
  IN  CHAR8   *FileName,
  IN  UINT64  StartAddress,
  IN  UINT64  Size,
  IN  UINT8   *Buffer
  )
/*++
  
Description:

  This function reads the input file, parse it and create a list of tokens
  which is parsed and used, to intialize the data related to Firmware Volume.
  
Input:

  FileName        FileName which needed to be read to parse data
  StartAddress    This will set the file position.
  Size            Size in bytes needed to be written
  Buffer          Buffer needed to e written

Return:

  EFI_STATUS
    
--*/
// GC_TODO: function comment is missing 'Routine Description:'
// GC_TODO: function comment is missing 'Arguments:'
// GC_TODO: function comment is missing 'Returns:'
// GC_TODO:    FileName - add argument and description to function comment
// GC_TODO:    StartAddress - add argument and description to function comment
// GC_TODO:    Size - add argument and description to function comment
// GC_TODO:    Buffer - add argument and description to function comment
// GC_TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// GC_TODO:    EFI_ABORTED - add return value to function comment
// GC_TODO:    EFI_SUCCESS - add return value to function comment
{
  FILE  *Fp;
  UINTN NumByte;

  Fp = fopen (FileName, "a+b");

  if (Fp == NULL) {
    printf ("\nERROR:Error in opening file %s ", FileName);
    return EFI_INVALID_PARAMETER;
  }

  fseek (Fp, (UINTN) StartAddress, SEEK_SET);
  NumByte = fwrite ((VOID *) Buffer, sizeof (UINT8), (UINTN) Size, Fp);

  //
  // Check to ensure that buffer has been copied successfully
  //
  if (NumByte != Size) {
    printf ("\nERROR: Error in copying the buffer into file");
    return EFI_ABORTED;
  }

  if (Fp) {
    fclose (Fp);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
BuildFirmwareDeviceBinaryFromFwVolumes (
  IN UINT64   FvBaseAddress,
  IN CHAR8    *FvFileName,
  IN CHAR8    *FdFileName
  )
/*++
  
Description:

  This function reads the input file, parse it and create a list of tokens
  which is parsed and used, to intialize the data related to Firmware Volume.
  
Input:

  FvBaseAddress   Base Address. This info is retrieved from INF file
  FvFileName      InputFileName
  FdFileName      Output File Name

Return:

  EFI_STATUS
    
--*/
// GC_TODO: function comment is missing 'Routine Description:'
// GC_TODO: function comment is missing 'Arguments:'
// GC_TODO: function comment is missing 'Returns:'
// GC_TODO:    FvBaseAddress - add argument and description to function comment
// GC_TODO:    FvFileName - add argument and description to function comment
// GC_TODO:    FdFileName - add argument and description to function comment
// GC_TODO:    EFI_ABORTED - add return value to function comment
// GC_TODO:    EFI_ABORTED - add return value to function comment
// GC_TODO:    EFI_ABORTED - add return value to function comment
// GC_TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// GC_TODO:    EFI_SUCCESS - add return value to function comment
{
  FILE          *Fp;
  UINT64        FileSize;
  UINT64        NumByteRead;
  UINT64        PadByteSize;
  UINTN         Index;
  UINT64        BaseAddress;
  UINT8         *Buffer;
  EFI_STATUS    Status;
  static UINT64 StartAddress  = 0;

  Fp                          = fopen (FvFileName, "r+b");

  if (Fp == NULL) {
    printf ("\nERROR:Error in opening file %s", FvFileName);
    return EFI_ABORTED;
  }

  BaseAddress = FdInfo->FdBaseAddress;

  //
  // Check if Base Address of Firmware Volume falls below the Base Address
  // Firmware Device, if yes, then abort this process.
  //
  if (FvBaseAddress < BaseAddress) {
    printf ("\nERROR: Firmware Volume Base Address falls below Firmware Device Address.\n");
    return EFI_ABORTED;
  }
  //
  // Check if there are any hole between two Firmware Volumes. If any hole
  // exists, fill the hole with PadByte data.
  //
  if (FvBaseAddress > LastAddress) {
    PadByteSize = (FvBaseAddress - LastAddress);
    Buffer      = malloc ((UINTN) PadByteSize);

    for (Index = 0; Index < PadByteSize; Index++) {
      *Buffer = FdInfo->PadValue;
      Buffer++;
    }

    Buffer -= PadByteSize;
    Status = WriteFwBinary (FdFileName, StartAddress, (UINT64) PadByteSize, Buffer);

    if (Buffer) {
      free (Buffer);
    }

    if (Status != EFI_SUCCESS) {
      printf ("\nERROR: Error in writing the binary image to file");
      return Status;
    }

    StartAddress += PadByteSize;
    LastAddress += PadByteSize;
  }
  //
  // Proceed with next Firmware Volume updates
  //
  FileSize = _filelength (fileno (Fp));

  if ((FvBaseAddress + FileSize) > (FdInfo->FdBaseAddress + FdInfo->FdSize)) {
    printf (
      "\nERROR:Unable to update Firmware Device. File %s is larger than \
                                  available space.",
      FvFileName
      );
    if (Fp) {
      fclose (Fp);
    }

    return EFI_ABORTED;
  }

  Buffer = malloc ((UINTN) FileSize);

  if (Buffer == NULL) {
    printf ("Error in allocating buffer to read specific file\n");
    return EFI_OUT_OF_RESOURCES;
  }

  NumByteRead = fread ((VOID *) Buffer, sizeof (UINT8), (UINTN) FileSize, Fp);

  Status      = WriteFwBinary (FdFileName, StartAddress, FileSize, Buffer);

  if (Buffer) {
    free ((VOID *) Buffer);
  }

  if (Fp) {
    fclose (Fp);
  }

  if (Status != EFI_SUCCESS) {
    printf ("\nERROR: Error in writing the binary image to file");
    return Status;
  }

  StartAddress += NumByteRead;
  LastAddress += FileSize;

  return EFI_SUCCESS;
}

VOID
CleanUpMemory (
  VOID
  )
/*++

Description:

  This function cleans up any allocated buffer
  
Input:

  NONE

Return:
  
  NONE
--*/
// GC_TODO: function comment is missing 'Routine Description:'
// GC_TODO: function comment is missing 'Arguments:'
// GC_TODO: function comment is missing 'Returns:'
{
  UINTN Index;

  if (FdInfo) {
    free (FdInfo);
  }

  FvInfo = OrgFvInfoPtr;

  if (FvInfo) {
    for (Index = 0; Index < NumFvFiles; Index++) {
      if (*FvInfo) {
        free (*FvInfo);
      }

      FvInfo++;
    }

    FvInfo = OrgFvInfoPtr;
    free (FvInfo);
  }
}

GEN_FD_IMAGE_API
EFI_STATUS
GenerateFdImage (
  IN  UINT64  BaseAddress,
  IN  UINT64  Size,
  IN  UINT8   PadByte,
  IN  CHAR8   *OutFile,
  IN  CHAR8   **FileList
  )
/*++
  
Description:

  This function reads the input file, parse it and create a list of tokens
  which is parsed and used, to intialize the data related to Firmware Volume.
  
Input:

  BaseAddress   Base Address for this Firmware Device
  Size,         Total Size of the Firmware Device
  PadByte       Pad byte data
  OutFile       Output File Name
  FileList      File List pointer to INF file names.

Return:

  EFI_STATUS
    
--*/
// GC_TODO: function comment is missing 'Routine Description:'
// GC_TODO: function comment is missing 'Arguments:'
// GC_TODO: function comment is missing 'Returns:'
// GC_TODO:    BaseAddress - add argument and description to function comment
// GC_TODO:    Size - add argument and description to function comment
// GC_TODO:    PadByte - add argument and description to function comment
// GC_TODO:    OutFile - add argument and description to function comment
// GC_TODO:    FileList - add argument and description to function comment
// GC_TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// GC_TODO:    EFI_ABORTED - add return value to function comment
// GC_TODO:    EFI_ABORTED - add return value to function comment
// GC_TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS  Status;
  UINTN       Index;
  UINTN       PadSize;
  UINTN       FileSize;
  CHAR8       **InFile;
  FILE        *Fp;
  UINT8       *Buffer;
  UINTN       NumByte;

  //
  // Ensure, if there are any previous Firmware Device exists,
  // If yes, make it to 0 bytes
  //
  if ((Fp = fopen (OutFile, "w")) != NULL) {
    fclose (Fp);
  }

  InFile = FileList;

  while (*InFile != NULL) {
    NumFvFiles++;
    InFile++;
  }

  InitializeComps ();

  //
  // Restore the orginal pointers
  //
  FvInfo  = OrgFvInfoPtr;
  InFile  = FileList;

  while (*InFile != NULL) {
    strcpy ((*FvInfo)->FvInfoFile, *InFile);
    Status = GetFvRelatedInfoFromInfFile (*InFile);

    if (Status != EFI_SUCCESS) {
      printf ("\nERROR: Error occurred in processsing INF file");
      CleanUpMemory ();
      return Status;
    }

    InFile++;
    FvInfo++;
  }

  FdInfo->FdSize        = Size;
  FdInfo->FdBaseAddress = BaseAddress;
  FdInfo->PadValue      = PadByte;
  FvInfo                = OrgFvInfoPtr;
  strcpy (FdInfo->OutFileName, OutFile);

  for (Index = 0; Index < NumFvFiles; Index++) {
    Status = GenerateFvImage ((*FvInfo)->FvInfoFile);

    if (Status != EFI_SUCCESS) {
      CleanUpMemory ();
      return Status;
    }

    FvInfo++;
  }

  FvInfo = OrgFvInfoPtr;

  //
  // Sort the Firmware Volume informations. Firmware Volume with lower
  // base addresses will be processed first and hiher base address one
  // will be processed later.
  //
  qsort ((VOID *) FvInfo, NumFvFiles, sizeof (FVINFO *), CompareItems);

  LastAddress = (*FvInfo)->FvBaseAddress;

  for (Index = 0; Index < NumFvFiles; Index++) {
    Status = BuildFirmwareDeviceBinaryFromFwVolumes (
              (*FvInfo)->FvBaseAddress,
              (*FvInfo)->FvFile,
              FdInfo->OutFileName
              );
    if (Status != EFI_SUCCESS) {
      CleanUpMemory ();
      return Status;
    }

    FvInfo++;
  }
  //
  // Check if any space left after copying data from all Firmware Volumes
  // If yes, then fill those location with PadValue.
  //
  if ((FdInfo->FdBaseAddress + Size) > LastAddress) {

    PadSize = (UINTN) ((FdInfo->FdBaseAddress + FdInfo->FdSize) - LastAddress);
    Buffer  = malloc (PadSize);

    if (Buffer == NULL) {
      CleanUpMemory ();
      return EFI_OUT_OF_RESOURCES;
    }

    for (Index = 0; Index < PadSize; Index++) {
      *Buffer = FdInfo->PadValue;
      Buffer++;
    }

    Buffer -= PadSize;

    Fp = fopen (OutFile, "a+b");

    if (Fp == NULL) {
      printf ("\nERROR:Opening file %s", OutFile);
      CleanUpMemory ();
      return EFI_ABORTED;
    }

    FileSize = _filelength (fileno (Fp));
    fseek (Fp, FileSize, SEEK_SET);
    NumByte = fwrite (Buffer, sizeof (UINT8), PadSize, Fp);

    if (Buffer) {
      free (Buffer);
    }

    fclose (Fp);

    if (NumByte != (sizeof (UINT8) * PadSize)) {
      printf ("\nERROR: Copying data from buffer to File %s ", OutFile);
      CleanUpMemory ();
      return EFI_ABORTED;
    }
  }
  //
  // Clean up all the memory which has been allocated so far.
  //
  CleanUpMemory ();
  return EFI_SUCCESS;
}
