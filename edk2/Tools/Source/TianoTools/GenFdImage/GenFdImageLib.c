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
  
  GenFdImageLib.C

Abstract:

  This file contains the functions required to generate 
  the Firmware Device

--*/

//
// Coded to EFI 2.0 Coding Standards
//
//
// Include file in build
//
#include "GenFdImage.h"

//
// Global declarations
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
// Internal Functions
//
INTN
CompareItems (
  IN const VOID  *Arg1,
  IN const VOID  *Arg2
  )
/*++

Routine Description:

    This function is used by qsort to sort the Fv list based on FvBaseAddress  

Arguments:

    Arg1
    Arg2

Returns:

    None

--*/
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
  IN CHAR8  *Token
  )
/*++

Routine Description:

    This function builds the token list in an array which will be parsed later

Arguments:

    Token    String,

Returns:

    None

--*/
{
  strcpy (*TokenStr, Token);
  TokenStr++;
}

VOID
TrimLine (
  IN CHAR8  *Line
  )
/*++

Routine Description:

  This function cleans up the line by removing all whitespace and 
  comments.

Arguments:

  Line    String,

Returns:

    None

--*/
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

  i = 0;
  j = 0;
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
  IN FILE  *Fp
  )
/*++

Routine Description:

  This function calculates number of valid lines in a input file.
  
Arguments:

  Fp     Pointer to a file handle which has been opened.

Returns:

  None

--*/
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
  IN FILE  *Fp
  )
/*++
  
Routine Description:

  This function parses the input file and tokenizes the string
  
Arguments:

  Fp     Pointer to a file handle which has been opened.
  
Returns:

  None

--*/
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

Routine Description:

  This function intializes the relevant global variable 
  used to store the information retrieved from the INF file.
  
Arguments:

  None

Returns:

  EFI_STATUS

--*/
// GC_TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// GC_TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// GC_TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// GC_TODO:    EFI_SUCCESS - add return value to function comment
{
  UINTN Index;

  FdInfo = malloc (sizeof (FDINFO));

  if (FdInfo == NULL) {
    printf ("ERROR: allocating memory (struct FDINFO) in"" function InitializeComps.\n");
    return EFI_OUT_OF_RESOURCES;
  }

  OrgFdInfoPtr  = FdInfo;

  FvInfo        = malloc (sizeof (INTN) * NumFvFiles);

  if (FvInfo == NULL) {
    printf ("ERROR: allocating memory (INTN * NumFvFiles) in"" function InitializeComps.\n");
    return EFI_OUT_OF_RESOURCES;
  }

  OrgFvInfoPtr = FvInfo;

  for (Index = 0; Index < NumFvFiles; Index++) {
    *FvInfo = malloc (sizeof (FVINFO));

    if (*FvInfo == NULL) {
      printf ("ERROR: allocating memory (FVINFO) in"" function InitializeComps.\n");
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

Routine Description:

  This function intializes the relevant global variable 
  used to store the information retrieved from the INF file.

Arguments:

  None

Returns:

  None

--*/
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
        if (AsciiStringToUint64 (
              *TokenStr,
              FALSE,
              &StringValue
              ) != EFI_SUCCESS) {
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
  IN CHAR8  *FileName
  )
/*++
  
Routine Description:

  This function reads the input file, parses it and create a list of tokens
  which are parsed and used, to intialize the data related to the Firmware 
  Volume.
  
Arguments:

  FileName  FileName which needed to be read to parse data

Returns:

  EFI_STATUS
    
--*/
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
  IN CHAR8   *FileName,
  IN UINT64  StartAddress,
  IN UINT64  Size,
  IN UINT8   *Buffer
  )
/*++
  
Routine Description:

  This function reads the input file, parses it and creates a list of tokens
  which are parsed and used to intialize the data related to the Firmware 
  Volume.
  
Arguments:

  FileName        FileName which needed to be read to parse data
  StartAddress    This will set the file position.
  Size            Size in bytes needed to be written
  Buffer          Buffer needed to e written

Returns:

  EFI_STATUS
    
--*/
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
  IN UINT64  FvBaseAddress,
  IN CHAR8   *FvFileName,
  IN CHAR8   *FdFileName
  )
/*++
  
Routine Description:

  This function reads the input file, parses it and creates a list of tokens
  which are parsed and used to intialize the data related to the Firmware 
  Volume.
  
Arguments:

  FvBaseAddress   Base Address. This info is retrieved from INF file
  FvFileName      InputFileName
  FdFileName      Output File Name

Returns:

  EFI_STATUS
    
--*/
// GC_TODO:    EFI_ABORTED - add return value to function comment
// GC_TODO:    EFI_ABORTED - add return value to function comment
// GC_TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// GC_TODO:    EFI_ABORTED - add return value to function comment
// GC_TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// GC_TODO:    EFI_SUCCESS - add return value to function comment
{
  FILE          *Fp;

  UINT64        FileSize;
  UINT64        NumByteRead;
  UINT64        PadByteSize;
  UINT64        BaseAddress;

  UINTN         Index;

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
    printf ("\nERROR: Firmware Volume Base Address falls below Firmware ""Device Address.\n");
    return EFI_ABORTED;
  }
  //
  // Check if there are any holes between two Firmware Volumes. If any holes
  // exist, fill the hole with PadByted data.
  //
  if (FvBaseAddress > LastAddress) {
    PadByteSize = (FvBaseAddress - LastAddress);
    Buffer      = malloc ((UINTN) PadByteSize);
    if (Buffer == NULL) {
      printf ("ERROR: allocating (Buffer) memory in"" function BuildFirmwareDeviceBinaryFromFwVolumes.\n");
      return EFI_OUT_OF_RESOURCES;
    }

    for (Index = 0; Index < PadByteSize; Index++) {
      *Buffer = FdInfo->PadValue;
      Buffer++;
    }

    Buffer -= PadByteSize;
    Status = WriteFwBinary (
              FdFileName,
              StartAddress,
              (UINT64) PadByteSize,
              Buffer
              );

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

Routine Description:

  This function cleans up any allocated buffer
  
Arguments:

  None

Returns:
  
  None

--*/
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

EFI_STATUS
GenerateFdImage (
  IN UINT64  BaseAddress,
  IN UINT64  Size,
  IN UINT8   PadByte,
  IN CHAR8   *OutFile,
  IN CHAR8   **FileList
  )
/*++
  
Routine Description:

  This function reads the input file, parses it and creates a list of tokens
  which are parsed and used to intialize the data related to the Firmware 
  Volume.
  
Arguments:

  BaseAddress   Base Address for this Firmware Device
  Size,         Total Size of the Firmware Device
  PadByte       Pad byte data
  OutFile       Output File Name
  FileList      File List pointer to INF file names.

Returns:

  EFI_STATUS
    
--*/
// GC_TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// GC_TODO:    EFI_ABORTED - add return value to function comment
// GC_TODO:    EFI_ABORTED - add return value to function comment
// GC_TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS  Status;

  UINTN       Index;
  UINTN       PadSize;
  UINTN       FileSize;
  UINTN       NumByte;

  CHAR8       **InFile;

  FILE        *Fp;

  UINT8       *Buffer;

  //
  // If any previous Firmware Device existed,
  // make it to 0 bytes
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
  // Sort the Firmware Volume information. Firmware Volume with lower
  // base addresses will be processed first and higher base address one
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
      printf ("\nERROR: allocating PadSize memory in function GenerateFdImage.\n");
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
