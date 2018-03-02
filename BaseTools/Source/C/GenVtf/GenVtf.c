/** @file
This file contains functions required to generate a boot strap file (BSF) also 
known as the Volume Top File (VTF)

Copyright (c) 1999 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

//
//
//
#include <FvLib.h>
#include <Common/UefiBaseTypes.h>
#include "GenVtf.h"
#include <Guid/PiFirmwareFileSystem.h>
#include "CommonLib.h"
#include "EfiUtilityMsgs.h"

//
// Global variables
//
UINTN SectionOptionFlag = 0;
UINTN SectionCompFlag = 0;

UINT64        DebugLevel;
BOOLEAN       DebugMode;

BOOLEAN     QuietMode = FALSE;

BOOLEAN     VTF_OUTPUT = FALSE;
CHAR8       *OutFileName1;
CHAR8       *OutFileName2;
CHAR8       *SymFileName;

CHAR8           **TokenStr;
CHAR8           **OrgStrTokPtr;

PARSED_VTF_INFO *FileListPtr;
PARSED_VTF_INFO *FileListHeadPtr;

VOID            *Vtf1Buffer;
VOID            *Vtf1EndBuffer;
VOID            *Vtf2Buffer;
VOID            *Vtf2EndBuffer;

UINTN           ValidLineNum        = 0;
UINTN           ValidFFDFileListNum = 0;

//
// Section Description and their number of occurences in *.INF file
//
UINTN           NumFvFiles        = 0;
UINTN           SectionOptionNum  = 0;

//
// Global flag which will check for VTF Present, if yes then will be used
// to decide about adding FFS header to pad data
//
BOOLEAN         VTFPresent = FALSE;
BOOLEAN         SecondVTF = FALSE;

//
// Address related information
//
UINT64          Fv1BaseAddress        = 0;
UINT64          Fv2BaseAddress        = 0;
UINT64          Fv1EndAddress         = 0;
UINT64          Fv2EndAddress         = 0;
UINT32          Vtf1TotalSize         = SIZE_TO_OFFSET_PAL_A_END;
UINT64          Vtf1LastStartAddress  = 0;
UINT32          Vtf2TotalSize         = 0;
UINT64          Vtf2LastStartAddress  = 0;

UINT32          BufferToTop           = 0;

//
// IA32 Reset Vector Bin name
//
CHAR8           IA32BinFile[FILE_NAME_SIZE];

//
// Function Implementations
//
EFI_STATUS
ConvertVersionInfo (
  IN      CHAR8     *Str,
  IN OUT  UINT8     *MajorVer,
  IN OUT  UINT8     *MinorVer
  )
/*++
Routine Description:

  This function split version to major version and minor version

Arguments:

  Str      - String representing in form XX.XX
  MajorVer - The major version
  MinorVer - The minor version

Returns:

  EFI_SUCCESS  - The function completed successfully.

--*/
{
  CHAR8  TemStr[5] = "0000";
  int    Major;
  int    Minor;
  UINTN Length;

  Major = 0;
  Minor = 0;

  if (strstr (Str, ".") != NULL) {
    sscanf (
      Str,
      "%02x.%02x",
      &Major,
      &Minor
      );
  } else {
    Length = strlen(Str);
    if (Length < 4) {
      memcpy (TemStr + 4 - Length, Str, Length);
    } else {
      memcpy (TemStr, Str + Length - 4, 4);
    }
  
    sscanf (
      TemStr,
      "%02x%02x",
      &Major,
      &Minor
      );
  }

  *MajorVer = (UINT8) Major;
  *MinorVer = (UINT8) Minor;
  return EFI_SUCCESS;
}

VOID
TrimLine (
  IN  CHAR8 *Line
  )
/*++
Routine Description:

  This function cleans up the line by removing all whitespace and
  comments

Arguments:

  Line   - The pointer of the string

Returns:

  None

--*/
{
  CHAR8 TmpLine[FILE_NAME_SIZE];
  CHAR8 Char;
  CHAR8 *Ptr0;
  UINTN Index;
  UINTN Index2;

  //
  // Change '#' to '//' for Comment style
  //
  if (((Ptr0 = strchr (Line, '#')) != NULL) || ((Ptr0 = strstr (Line, "//")) != NULL)) {
    Line[Ptr0 - Line] = 0;
  }

  //
  // Initialize counters
  //
  Index   = 0;
  Index2  = 0;

  while ((Char = Line[Index]) != 0) {
    if ((Char != ' ') && (Char != '\t') && (Char != '\n') && (Char != '\r')) {
      TmpLine[Index2++] = Char;
    }
    Index++;
  }

  TmpLine[Index2] = 0;
  strcpy (Line, TmpLine);
}

VOID
ValidLineCount (
  IN  FILE *Fp
  )
/*++

Routine Description:

  This function calculated number of valid lines in a input file.

Arguments:

  Fp    - Pointer to a file handle which has been opened.

Returns:

  None

--*/
{
  CHAR8 Buff[FILE_NAME_SIZE];
  while (fgets(Buff, sizeof (Buff), Fp)) {
    TrimLine (Buff);
    if (Buff[0] == 0) {
      continue;
    }
    ValidLineNum++;
  }
}

EFI_STATUS
ParseInputFile (
  IN  FILE *Fp
  )
/*++

Routine Description:

  This function parses the input file and tokenize the string

Arguments:

  Fp    - Pointer to a file handle which has been opened.

Returns:

  None

--*/
{
  CHAR8 *Token;
  CHAR8 Buff[FILE_NAME_SIZE + 1];
  CHAR8 Delimit[] = "=";

  Buff [FILE_NAME_SIZE] = '\0';
  Token = NULL;

  while (fgets (Buff, FILE_NAME_SIZE, Fp) != NULL) {
    TrimLine (Buff);
    if (Buff[0] == 0) {
      continue;
    }
    Token = strtok (Buff, Delimit);
    while (Token != NULL) {
      strcpy (*TokenStr, Token);
      TokenStr ++;
      Token = strtok (NULL, Delimit);
    }
  }
  return EFI_SUCCESS;
}

EFI_STATUS
InitializeComps (
  VOID
  )
/*++

Routine Description:

  This function initializes the relevant global variable which is being
  used to store the information retrieved from INF file.  This also initializes
  the VTF symbol file.

Arguments:

  None

Returns:

  EFI_SUCCESS            - The function completed successfully
  EFI_OUT_OF_RESOURCES   - Malloc failed.

--*/
{

  FileListPtr = malloc (sizeof (PARSED_VTF_INFO));

  if (FileListPtr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  FileListHeadPtr = FileListPtr;
  memset (FileListPtr, 0, sizeof (PARSED_VTF_INFO));
  FileListPtr->NextVtfInfo = NULL;

  remove (SymFileName);
  return EFI_SUCCESS;
}

VOID
ParseAndUpdateComponents (
  IN  PARSED_VTF_INFO   *VtfInfo
  )
/*++

Routine Description:

  This function initializes the relevant global variable which is being
  used to store the information retrieved from INF file.

Arguments:

  VtfInfo  - A pointer to the VTF Info Structure


Returns:

  None

--*/
{
  UINT64  StringValue;

  while (*TokenStr != NULL && (strnicmp (*TokenStr, "COMP_NAME", 9) != 0)) {

    if (strnicmp (*TokenStr, "COMP_LOC", 8) == 0) {
      TokenStr++;
      if (strnicmp (*TokenStr, "F", 1) == 0) {
        VtfInfo->LocationType = FIRST_VTF;
      } else if (strnicmp (*TokenStr, "S", 1) == 0) {
        VtfInfo->LocationType = SECOND_VTF;
      } else {
        VtfInfo->LocationType = NONE;
      }
    } else if (strnicmp (*TokenStr, "COMP_TYPE", 9) == 0) {
      TokenStr++;
      if (AsciiStringToUint64 (*TokenStr, FALSE, &StringValue) != EFI_SUCCESS) {
        Error (NULL, 0, 5001, "Cannot get: \"0x%s\".", *TokenStr);
        return ;
      }

      VtfInfo->CompType = (UINT8) StringValue;
    } else if (strnicmp (*TokenStr, "COMP_VER", 8) == 0) {
      TokenStr++;
      if (strnicmp (*TokenStr, "-", 1) == 0) {
        VtfInfo->VersionPresent = FALSE;
        VtfInfo->MajorVer       = 0;
        VtfInfo->MinorVer       = 0;
      } else {
        VtfInfo->VersionPresent = TRUE;
        ConvertVersionInfo (*TokenStr, &VtfInfo->MajorVer, &VtfInfo->MinorVer);
      }
    } else if (strnicmp (*TokenStr, "COMP_BIN", 8) == 0) {
      TokenStr++;
      if (strlen (*TokenStr) >= FILE_NAME_SIZE) {
        Error (NULL, 0, 3000, "Invalid", "The 'COMP_BIN' name is too long.");
        return ;
      }
      strncpy (VtfInfo->CompBinName, *TokenStr, FILE_NAME_SIZE - 1);
      VtfInfo->CompBinName[FILE_NAME_SIZE - 1] = 0;
    } else if (strnicmp (*TokenStr, "COMP_SYM", 8) == 0) {
      TokenStr++;
      if (strlen (*TokenStr) >= FILE_NAME_SIZE) {
        Error (NULL, 0, 3000, "Invalid", "The 'COMP_SYM' name is too long.");
        return ;
      }
      strncpy (VtfInfo->CompSymName, *TokenStr, FILE_NAME_SIZE - 1);
      VtfInfo->CompSymName[FILE_NAME_SIZE - 1] = 0;
    } else if (strnicmp (*TokenStr, "COMP_SIZE", 9) == 0) {
      TokenStr++;
      if (strnicmp (*TokenStr, "-", 1) == 0) {
        VtfInfo->PreferredSize  = FALSE;
        VtfInfo->CompSize       = 0;
      } else {
        VtfInfo->PreferredSize = TRUE;
        if (AsciiStringToUint64 (*TokenStr, FALSE, &StringValue) != EFI_SUCCESS) {
          Error (NULL, 0, 5001, "Parse error", "Cannot get: %s.", TokenStr);
          return ;
        }

        VtfInfo->CompSize = (UINTN) StringValue;
      }

    } else if (strnicmp (*TokenStr, "COMP_CS", 7) == 0) {
      TokenStr++;
      if (strnicmp (*TokenStr, "1", 1) == 0) {
        VtfInfo->CheckSumRequired = 1;
      } else if (strnicmp (*TokenStr, "0", 1) == 0) {
        VtfInfo->CheckSumRequired = 0;
      } else {
        Error (NULL, 0, 3000, "Invaild", "Bad value in INF file required field: Checksum, the value must be '0' or '1'.");
      }
    }

    TokenStr++;
    if (*TokenStr == NULL) {
      break;
    }
  }
}

VOID
InitializeInFileInfo (
  VOID
  )
/*++

Routine Description:

  This function intializes the relevant global variable which is being
  used to store the information retrieved from INF file.

Arguments:

  NONE

Returns:

  NONE

--*/
{

  SectionOptionFlag = 0;
  SectionCompFlag   = 0;
  TokenStr          = OrgStrTokPtr;

  while (*TokenStr != NULL) {
    if (strnicmp (*TokenStr, "[OPTIONS]", 9) == 0) {
      SectionOptionFlag = 1;
      SectionCompFlag   = 0;
    }

    if (strnicmp (*TokenStr, "[COMPONENTS]", 12) == 0) {
      if (FileListPtr == NULL) {
        FileListPtr = FileListHeadPtr;
      }

      SectionCompFlag   = 1;
      SectionOptionFlag = 0;
      TokenStr++;
    }

    if (SectionOptionFlag) {
      if (stricmp (*TokenStr, "IA32_RST_BIN") == 0) {
        TokenStr++;
        if (strlen (*TokenStr) >= FILE_NAME_SIZE) {
          Error (NULL, 0, 3000, "Invalid", "The 'IA32_RST_BIN' name is too long.");
          break;
        }
        strncpy (IA32BinFile, *TokenStr, FILE_NAME_SIZE - 1);
        IA32BinFile[FILE_NAME_SIZE - 1] = 0;
      }
    }

    if (SectionCompFlag) {
      if (stricmp (*TokenStr, "COMP_NAME") == 0) {
        TokenStr++;
        if (strlen (*TokenStr) >= COMPONENT_NAME_SIZE) {
          Error (NULL, 0, 3000, "Invalid", "The 'COMP_NAME' name is too long.");
          break;
        }
        strncpy (FileListPtr->CompName, *TokenStr, COMPONENT_NAME_SIZE - 1);
        FileListPtr->CompName[COMPONENT_NAME_SIZE - 1] = 0;
        TokenStr++;
        ParseAndUpdateComponents (FileListPtr);
      }

      if (*TokenStr != NULL) {
        FileListPtr->NextVtfInfo  = malloc (sizeof (PARSED_VTF_INFO));
        if (FileListPtr->NextVtfInfo == NULL) {
          Error (NULL, 0, 4003, "Resource", "Out of memory resources.", NULL);
          break;
        }
        FileListPtr = FileListPtr->NextVtfInfo;
        memset (FileListPtr, 0, sizeof (PARSED_VTF_INFO));
        FileListPtr->NextVtfInfo = NULL;
        continue;
      } else {
        break;
      }
    }

    TokenStr++;
  }
}

EFI_STATUS
GetVtfRelatedInfoFromInfFile (
  IN FILE *FilePointer
  )
/*++

Routine Description:

  This function reads the input file, parse it and create a list of tokens
  which is parsed and used, to intialize the data related to VTF

Arguments:

  FileName  - FileName which needed to be read to parse data

Returns:

  EFI_ABORTED           - Error in opening file
  EFI_INVALID_PARAMETER - File doesn't contain any valid information
  EFI_OUT_OF_RESOURCES  - Malloc Failed
  EFI_SUCCESS           - The function completed successfully

--*/
{
  FILE        *Fp;
  UINTN       Index;
  UINTN       Index1;
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;
  Fp = FilePointer;
  if (Fp == NULL) {
    Error (NULL, 0, 2000, "Invalid parameter", "BSF INF file is invalid!");
    return EFI_ABORTED;
  }

  ValidLineCount (Fp);

  if (ValidLineNum == 0) {
    Error (NULL, 0, 2000, "Invalid parameter", "File does not contain any valid information!");
    return EFI_INVALID_PARAMETER;
  }

  TokenStr = (CHAR8 **) malloc (sizeof (UINTN) * (2 * ValidLineNum + 1));

  if (TokenStr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  memset (TokenStr, 0, (sizeof (UINTN) * (2 * ValidLineNum + 1)));
  OrgStrTokPtr = TokenStr;

  for (Index = 0; Index < (2 * ValidLineNum); Index++) {
    *TokenStr = (CHAR8*)malloc (sizeof (CHAR8) * FILE_NAME_SIZE);

    if (*TokenStr == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ParseFileError;
    }

    memset (*TokenStr, 0, FILE_NAME_SIZE);
    TokenStr++;
  }

  TokenStr  = OrgStrTokPtr;
  fseek (Fp, 0L, SEEK_SET);

  Status = InitializeComps ();

  if (Status != EFI_SUCCESS) {
    goto ParseFileError;
  }

  Status = ParseInputFile (Fp);
  if (Status != EFI_SUCCESS) {
    goto ParseFileError;
  }

  InitializeInFileInfo ();

ParseFileError:

  for (Index1 = 0; Index1 < Index; Index1 ++) {
    free (OrgStrTokPtr[Index1]);
  }

  free (OrgStrTokPtr);

  return Status;
}

VOID
GetRelativeAddressInVtfBuffer (
  IN      UINT64     Address,
  IN OUT  UINTN      *RelativeAddress,
  IN      LOC_TYPE   LocType
  )
/*++

Routine Description:

  This function checks for the address alignmnet for specified data boundary. In
  case the address is not aligned, it returns FALSE and the amount of data in
  terms of byte needed to adjust to get the boundary alignmnet. If data is
  aligned, TRUE will be returned.

Arguments:

  Address             - The address of the flash map space
  RelativeAddress     - The relative address of the Buffer
  LocType             - The type of the VTF


Returns:


--*/
{
  UINT64  TempAddress;
  UINT8   *LocalBuff;

  if (LocType == FIRST_VTF) {
    LocalBuff         = (UINT8 *) Vtf1EndBuffer;
    TempAddress       = Fv1EndAddress - Address;
    *RelativeAddress  = (UINTN) LocalBuff - (UINTN) TempAddress;
  } else {
    LocalBuff         = (UINT8 *) Vtf2EndBuffer;
    TempAddress       = Fv2EndAddress - Address;
    *RelativeAddress  = (UINTN) LocalBuff - (UINTN) TempAddress;
  }
}

EFI_STATUS
GetComponentVersionInfo (
  IN  OUT PARSED_VTF_INFO   *VtfInfo,
  IN      UINT8             *Buffer
  )
/*++
Routine Description:

  This function will extract the version information from File

Arguments:

  VtfInfo  - A Pointer to the VTF Info Structure
  Buffer   - A Pointer to type UINT8

Returns:

   EFI_SUCCESS           - The function completed successfully
   EFI_INVALID_PARAMETER - The parameter is invalid

--*/
{
  UINT16      VersionInfo;
  EFI_STATUS  Status;

  switch (VtfInfo->CompType) {

  case COMP_TYPE_FIT_PAL_A:
  case COMP_TYPE_FIT_PAL_B:
    memcpy (&VersionInfo, (Buffer + 8), sizeof (UINT16));
    VtfInfo->MajorVer = (UINT8) ((VersionInfo & 0xFF00) >> 8);
    VtfInfo->MinorVer = (UINT8) (VersionInfo & 0x00FF);
    Status            = EFI_SUCCESS;
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
    break;
  }

  return Status;
}

BOOLEAN
CheckAddressAlignment (
  IN      UINT64  Address,
  IN      UINT64  AlignmentData,
  IN OUT  UINT64  *AlignAdjustByte
  )
/*++

Routine Description:

  This function checks for the address alignmnet for specified data boundary. In
  case the address is not aligned, it returns FALSE and the amount of data in
  terms of byte needed to adjust to get the boundary alignmnet. If data is
  aligned, TRUE will be returned.

Arguments:

  Address              - Pointer to buffer containing byte data of component.
  AlignmentData        - DataSize for which address needed to be aligned
  AlignAdjustByte      - Number of bytes needed to adjust alignment.

Returns:

  TRUE                 - Address is aligned to specific data size boundary
  FALSE                - Address in not aligned to specified data size boundary
                       - Add/Subtract AlignAdjustByte to aling the address.

--*/
{
  //
  // Check if the assigned address is on address boundary. If not, it will
  // return the remaining byte required to adjust the address for specified
  // address boundary
  //
  *AlignAdjustByte = (Address % AlignmentData);

  if (*AlignAdjustByte == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

EFI_STATUS
GetFitTableStartAddress (
  IN OUT  FIT_TABLE   **FitTable
  )
/*++

Routine Description:

  Get the FIT table start address in VTF Buffer

Arguments:

  FitTable    - Pointer to available fit table where new component can be added

Returns:

  EFI_SUCCESS - The function completed successfully

--*/
{
  UINT64  FitTableAdd;
  UINT64  FitTableAddOffset;
  UINTN   RelativeAddress;

  //
  // Read the Fit Table address from Itanium-based address map.
  //
  FitTableAddOffset = Fv1EndAddress - (SIZE_IA32_RESET_VECT + SIZE_SALE_ENTRY_POINT + SIZE_FIT_TABLE_ADD);

  //
  // Translate this Itanium-based address in terms of local buffer address which
  // contains the image for Boot Strapped File. The relative address will be
  // the address of fit table VTF buffer.
  //
  GetRelativeAddressInVtfBuffer (FitTableAddOffset, &RelativeAddress, FIRST_VTF);
  FitTableAdd = *(UINTN *) RelativeAddress;

  //
  // The FitTableAdd is the extracted Itanium based address pointing to FIT
  // table. The relative address will return its actual location in VTF
  // Buffer.
  //
  GetRelativeAddressInVtfBuffer (FitTableAdd, &RelativeAddress, FIRST_VTF);

  *FitTable = (FIT_TABLE *) RelativeAddress;

  return EFI_SUCCESS;
}

EFI_STATUS
GetNextAvailableFitPtr (
  IN  FIT_TABLE   **FitPtr
  )
/*++

Routine Description:

  Get the FIT table address and locate the free space in fit where we can add
  new component. In this process, this function locates the fit table using
  Fit pointer in Itanium-based address map (as per Intel?Itanium(TM) SAL spec)
  and locate the available location in FIT table to be used by new components.
  If there are any Fit table which areg not being used contains ComponentType
  field as 0x7F. If needed we can change this and spec this out.

Arguments:

  FitPtr    - Pointer to available fit table where new component can be added

Returns:

  EFI_SUCCESS  - The function completed successfully

--*/
{
  FIT_TABLE *TmpFitPtr;
  UINT64    FitTableAdd;
  UINT64    FitTableAddOffset;
  UINTN     Index;
  UINTN     NumFitComponents;
  UINTN     RelativeAddress;

  //
  // Read the Fit Table address from Itanium-based address map.
  //
  FitTableAddOffset = Fv1EndAddress - (SIZE_IA32_RESET_VECT + SIZE_SALE_ENTRY_POINT + SIZE_FIT_TABLE_ADD);

  //
  // Translate this Itanium-based address in terms of local buffer address which
  // contains the image for Boot Strapped File. The relative address will be
  // the address of fit table VTF buffer.
  //
  GetRelativeAddressInVtfBuffer (FitTableAddOffset, &RelativeAddress, FIRST_VTF);
  FitTableAdd = *(UINTN *) RelativeAddress;

  //
  // The FitTableAdd is the extracted Itanium based address pointing to FIT
  // table. The relative address will return its actual location in VTF
  // Buffer.
  //
  GetRelativeAddressInVtfBuffer (FitTableAdd, &RelativeAddress, FIRST_VTF);

  TmpFitPtr         = (FIT_TABLE *) RelativeAddress;
  NumFitComponents  = TmpFitPtr->CompSize;
  *FitPtr           = NULL;

  for (Index = 0; Index < NumFitComponents; Index++) {
    if ((TmpFitPtr->CvAndType & FIT_TYPE_MASK) == COMP_TYPE_FIT_UNUSED) {
      *FitPtr = TmpFitPtr;
      break;
    }

    TmpFitPtr++;
  }

  return EFI_SUCCESS;
}

int
CompareItems (
  IN const VOID  *Arg1,
  IN const VOID  *Arg2
  )
/*++

Routine Description:

    This function is used by qsort to sort the FIT table based upon Component
    Type in their incresing order.

Arguments:

    Arg1  -   Pointer to Arg1
    Arg2  -   Pointer to Arg2

Returns:

    None

--*/
{
  if ((((FIT_TABLE *) Arg1)->CvAndType & FIT_TYPE_MASK) > (((FIT_TABLE *) Arg2)->CvAndType & FIT_TYPE_MASK)) {
    return 1;
  } else if ((((FIT_TABLE *) Arg1)->CvAndType & FIT_TYPE_MASK) < (((FIT_TABLE *) Arg2)->CvAndType & FIT_TYPE_MASK)) {
    return -1;
  } else {
    return 0;
  }
}

VOID
SortFitTable (
  IN  VOID
  )
/*++

Routine Description:

    This function is used by qsort to sort the FIT table based upon Component
    Type in their incresing order.

Arguments:

    VOID

Returns:

    None

--*/
{
  FIT_TABLE *FitTable;
  FIT_TABLE *TmpFitPtr;
  UINTN     NumFitComponents;
  UINTN     Index;

  GetFitTableStartAddress (&FitTable);
  TmpFitPtr         = FitTable;
  NumFitComponents  = 0;
  for (Index = 0; Index < FitTable->CompSize; Index++) {
    if ((TmpFitPtr->CvAndType & FIT_TYPE_MASK) != COMP_TYPE_FIT_UNUSED) {
      NumFitComponents += 1;
    }
    TmpFitPtr++;
  }
  qsort ((VOID *) FitTable, NumFitComponents, sizeof (FIT_TABLE), CompareItems);
}

VOID
UpdateFitEntryForFwVolume (
  IN  UINT64  Size
  )
/*++

Routine Description:

  This function updates the information about Firmware Volume  in FIT TABLE.
  This FIT table has to be immediately below the PAL_A Start and it contains
  component type and address information. Other information can't be
  created this time so we would need to fix it up..


Arguments:

  Size   - Firmware Volume Size

Returns:

  VOID

--*/
{
  FIT_TABLE *CompFitPtr;
  UINTN     RelativeAddress;

  //
  // FV Fit table will be located at PAL_A Startaddress - 16 byte location
  //
  Vtf1LastStartAddress -= 0x10;
  Vtf1TotalSize += 0x10;

  GetRelativeAddressInVtfBuffer (Vtf1LastStartAddress, &RelativeAddress, FIRST_VTF);

  CompFitPtr              = (FIT_TABLE *) RelativeAddress;
  CompFitPtr->CompAddress = Fv1BaseAddress;

  //
  // Since we don't have any information about its location in Firmware Volume,
  // initialize address to 0. This will be updated once Firmware Volume is
  // being build and its current address will be fixed in FIT table. Currently
  // we haven't implemented it so far and working on architectural clarafication
  //
  //
  // Firmware Volume Size in 16 byte block
  //
  CompFitPtr->CompSize = ((UINT32) Size) / 16;

  //
  // Since Firmware Volume does not exist by the time we create this FIT info
  // this should be fixedup from Firmware Volume creation tool. We haven't
  // worked out a method so far.
  //
  CompFitPtr->CompVersion = MAKE_VERSION (0, 0);

  //
  // Since we don't have any info about this file, we are making sure that
  // checksum is not needed.
  //
  CompFitPtr->CvAndType = CV_N_TYPE (0, COMP_TYPE_FIT_FV_BOOT);

  //
  // Since non VTF component will reside outside the VTF, we will not have its
  // binary image while creating VTF, hence we will not perform checksum at
  // this time. Once Firmware Volume is being created which will contain this
  // VTF, it will fix the FIT table for all the non VTF component and hence
  // checksum
  //
  CompFitPtr->CheckSum = 0;
}

EFI_STATUS
UpdateFitEntryForNonVTFComp (
  IN  PARSED_VTF_INFO   *VtfInfo
  )
/*++

Routine Description:

  This function updates the information about non VTF component in FIT TABLE.
  Since non VTF componets binaries are not part of VTF binary, we would still
  be required to update its location information in Firmware Volume, inside
  FIT table.

Arguments:

  VtfInfo    - Pointer to VTF Info Structure

Returns:

  EFI_ABORTED  - The function fails to update the component in FIT
  EFI_SUCCESS  - The function completed successfully

--*/
{
  FIT_TABLE *CompFitPtr;

  //
  // Scan the FIT table for available space
  //
  GetNextAvailableFitPtr (&CompFitPtr);
  if (CompFitPtr == NULL) {
    Error (NULL, 0, 5003, "Invalid", "Can't update this component in FIT");
    return EFI_ABORTED;
  }

  //
  // Since we don't have any information about its location in Firmware Volume,
  // initialize address to 0. This will be updated once Firmware Volume is
  // being build and its current address will be fixed in FIT table
  //
  CompFitPtr->CompAddress = 0;
  CompFitPtr->CompSize    = VtfInfo->CompSize;
  CompFitPtr->CompVersion = MAKE_VERSION (VtfInfo->MajorVer, VtfInfo->MinorVer);
  CompFitPtr->CvAndType   = CV_N_TYPE (VtfInfo->CheckSumRequired, VtfInfo->CompType);

  //
  // Since non VTF component will reside outside the VTF, we will not have its
  // binary image while creating VTF, hence we will not perform checksum at
  // this time. Once Firmware Volume is being created which will contain this
  // VTF, it will fix the FIT table for all the non VTF component and hence
  // checksum
  //
  CompFitPtr->CheckSum = 0;

  //
  // Fit Type is FV_BOOT which means Firmware Volume, we initialize this to base
  // address of Firmware Volume in which this VTF will be attached.
  //
  if ((CompFitPtr->CvAndType & 0x7F) == COMP_TYPE_FIT_FV_BOOT) {
    CompFitPtr->CompAddress = Fv1BaseAddress;
  }

  return EFI_SUCCESS;
}

//
// !!!WARNING
// This function is updating the SALE_ENTRY in Itanium address space as per SAL
// spec. SALE_ENTRY is being read from SYM file of PEICORE. Once the PEI
// CORE moves in Firmware Volume, we would need to modify this function to be
// used with a API which will detect PEICORE component while building Firmware
// Volume and update its entry in FIT table as well as in Itanium address space
// as per Intel?Itanium(TM) SAL address space
//
EFI_STATUS
UpdateEntryPoint (
  IN  PARSED_VTF_INFO   *VtfInfo,
  IN  UINT64            *CompStartAddress
  )
/*++

Routine Description:

  This function updated the architectural entry point in IPF, SALE_ENTRY.

Arguments:

  VtfInfo            - Pointer to VTF Info Structure
  CompStartAddress   - Pointer to Component Start Address

Returns:

  EFI_INVALID_PARAMETER  - The parameter is invalid
  EFI_SUCCESS            - The function completed successfully

--*/
{
  UINTN   RelativeAddress;
  UINT64  SalEntryAdd;
  FILE    *Fp;
  UINTN   Offset;

  CHAR8   Buff[FILE_NAME_SIZE];
  CHAR8   Buff1[10];
  CHAR8   Buff2[10];
  CHAR8   OffsetStr[30];
  CHAR8   Buff3[10];
  CHAR8   Buff4[10];
  CHAR8   Buff5[10];
  CHAR8   Token[50];
  CHAR8   FormatString[MAX_LINE_LEN];

  Fp = fopen (LongFilePath (VtfInfo->CompSymName), "rb");

  if (Fp == NULL) {
    Error (NULL, 0, 0001, "Error opening file", VtfInfo->CompSymName);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Generate the format string for fscanf
  //
  sprintf (
    FormatString,
    "%%%us %%%us %%%us %%%us %%%us %%%us %%%us",
    (unsigned) sizeof (Buff1) - 1,
    (unsigned) sizeof (Buff2) - 1,
    (unsigned) sizeof (OffsetStr) - 1,
    (unsigned) sizeof (Buff3) - 1,
    (unsigned) sizeof (Buff4) - 1,
    (unsigned) sizeof (Buff5) - 1,
    (unsigned) sizeof (Token) - 1
    );

  while (fgets (Buff, sizeof (Buff), Fp) != NULL) {
    fscanf (
      Fp,
      FormatString,
      Buff1,
      Buff2,
      OffsetStr,
      Buff3,
      Buff4,
      Buff5,
      Token
      );
    if (strnicmp (Token, "SALE_ENTRY", 10) == 0) {
      break;
    }
  }

  Offset = strtoul (OffsetStr, NULL, 16);

  *CompStartAddress += Offset;
  SalEntryAdd = Fv1EndAddress - (SIZE_IA32_RESET_VECT + SIZE_SALE_ENTRY_POINT);

  GetRelativeAddressInVtfBuffer (SalEntryAdd, &RelativeAddress, FIRST_VTF);

  memcpy ((VOID *) RelativeAddress, (VOID *) CompStartAddress, sizeof (UINT64));

  if (Fp != NULL) {
    fclose (Fp);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
CreateAndUpdateComponent (
  IN  PARSED_VTF_INFO   *VtfInfo
  )
/*++

Routine Description:

  This function reads the binary file for each components and update them
  in VTF Buffer as well as in FIT table. If the component is located in non
  VTF area, only the FIT table address will be updated

Arguments:

  VtfInfo    - Pointer to Parsed Info

Returns:

  EFI_SUCCESS      - The function completed successful
  EFI_ABORTED      - Aborted due to one of the many reasons like:
                      (a) Component Size greater than the specified size.
                      (b) Error opening files.
                      (c) Fail to get the FIT table address.

  EFI_INVALID_PARAMETER     Value returned from call to UpdateEntryPoint()
  EFI_OUT_OF_RESOURCES      Memory allocation failure.

--*/
{
  EFI_STATUS  Status;
  UINT64      CompStartAddress;
  UINT64      FileSize;
  UINT64      NumAdjustByte;
  UINT8       *Buffer;
  FILE        *Fp;
  FIT_TABLE   *CompFitPtr;
  BOOLEAN     Aligncheck;

  if (VtfInfo->LocationType == NONE) {
    UpdateFitEntryForNonVTFComp (VtfInfo);
    return EFI_SUCCESS;
  }

  Fp = fopen (LongFilePath (VtfInfo->CompBinName), "rb");

  if (Fp == NULL) {
    Error (NULL, 0, 0001, "Error opening file", VtfInfo->CompBinName);
    return EFI_ABORTED;
  }

  FileSize = _filelength (fileno (Fp));
  if ((VtfInfo->CompType == COMP_TYPE_FIT_PAL_B) || (VtfInfo->CompType == COMP_TYPE_FIT_PAL_A_SPECIFIC)) {

    //
    // BUGBUG: Satish to correct
    //
    FileSize -= SIZE_OF_PAL_HEADER;
  }

  if (VtfInfo->PreferredSize) {
    if (FileSize > VtfInfo->CompSize) {
      fclose (Fp);
      Error (NULL, 0, 2000, "Invalid parameter", "The component size is more than specified size.");
      return EFI_ABORTED;
    }

    FileSize = VtfInfo->CompSize;
  }

  Buffer = malloc ((UINTN) FileSize);
  if (Buffer == NULL) {
    fclose (Fp);
    return EFI_OUT_OF_RESOURCES;
  }
  memset (Buffer, 0, (UINTN) FileSize);

  if ((VtfInfo->CompType == COMP_TYPE_FIT_PAL_B) || (VtfInfo->CompType == COMP_TYPE_FIT_PAL_A_SPECIFIC)) {

    //
    // Read first 64 bytes of PAL header and use it to find version info
    //
    fread (Buffer, sizeof (UINT8), SIZE_OF_PAL_HEADER, Fp);

    //
    // PAL header contains the version info. Currently, we will use the header
    // to read version info and then discard.
    //
    if (!VtfInfo->VersionPresent) {
      GetComponentVersionInfo (VtfInfo, Buffer);
    }
  }

  fread (Buffer, sizeof (UINT8), (UINTN) FileSize, Fp);
  fclose (Fp);

  //
  // If it is non PAL_B component, pass the entire buffer to get the version
  // info and implement any specific case inside GetComponentVersionInfo.
  //
  if (VtfInfo->CompType != COMP_TYPE_FIT_PAL_B) {
    if (!VtfInfo->VersionPresent) {
      GetComponentVersionInfo (VtfInfo, Buffer);
    }
  }

  if (VtfInfo->LocationType == SECOND_VTF) {

    CompStartAddress = (Vtf2LastStartAddress - FileSize);
  } else {
    CompStartAddress = (Vtf1LastStartAddress - FileSize);
  }

  if (VtfInfo->CompType == COMP_TYPE_FIT_PAL_B) {
    Aligncheck = CheckAddressAlignment (CompStartAddress, 32 * 1024, &NumAdjustByte);
  } else {
    Aligncheck = CheckAddressAlignment (CompStartAddress, 8, &NumAdjustByte);
  }

  if (!Aligncheck) {
    CompStartAddress -= NumAdjustByte;
  }

  if (VtfInfo->LocationType == SECOND_VTF && SecondVTF == TRUE) {
    Vtf2LastStartAddress = CompStartAddress;
    Vtf2TotalSize += (UINT32) (FileSize + NumAdjustByte);
    Status = UpdateVtfBuffer (CompStartAddress, Buffer, FileSize, SECOND_VTF);
  } else if (VtfInfo->LocationType == FIRST_VTF) {
    Vtf1LastStartAddress = CompStartAddress;
    Vtf1TotalSize += (UINT32) (FileSize + NumAdjustByte);
    Status = UpdateVtfBuffer (CompStartAddress, Buffer, FileSize, FIRST_VTF);
  } else {
    free (Buffer);
    Error (NULL, 0, 2000,"Invalid Parameter", "There's component in second VTF so second BaseAddress and Size must be specified!");
    return EFI_INVALID_PARAMETER;
  }

  if (EFI_ERROR (Status)) {
    free (Buffer);
    return EFI_ABORTED;
  }

  GetNextAvailableFitPtr (&CompFitPtr);
  if (CompFitPtr == NULL) {
    free (Buffer);
    return EFI_ABORTED;
  }

  CompFitPtr->CompAddress = CompStartAddress | IPF_CACHE_BIT;
  if ((FileSize % 16) != 0) {
    free (Buffer);
    Error (NULL, 0, 2000, "Invalid parameter", "Binary FileSize must be a multiple of 16.");
    return EFI_INVALID_PARAMETER;
  }
  //assert ((FileSize % 16) == 0);
  CompFitPtr->CompSize    = (UINT32) (FileSize / 16);
  CompFitPtr->CompVersion = MAKE_VERSION (VtfInfo->MajorVer, VtfInfo->MinorVer);
  CompFitPtr->CvAndType   = CV_N_TYPE (VtfInfo->CheckSumRequired, VtfInfo->CompType);
  if (VtfInfo->CheckSumRequired) {
    CompFitPtr->CheckSum  = 0;
    CompFitPtr->CheckSum  = CalculateChecksum8 (Buffer, (UINTN) FileSize);
  }

  //
  // Free the buffer
  //
  if (Buffer) {
    free (Buffer);
  }

  //
  // Update the SYM file for this component based on it's start address.
  //
  Status = UpdateSymFile (CompStartAddress, SymFileName, VtfInfo->CompSymName, FileSize);
  if (EFI_ERROR (Status)) {

    //
    // At this time, SYM files are not required, so continue on error.
    //
  }

  // !!!!!!!!!!!!!!!!!!!!!
  // BUGBUG:
  // This part of the code is a temporary line since PEICORE is going to be inside
  // VTF till we work out how to determine the SALE_ENTRY through it. We will need
  // to clarify so many related questions
  // !!!!!!!!!!!!!!!!!!!!!!!

  if (VtfInfo->CompType == COMP_TYPE_FIT_PEICORE) {
    Status = UpdateEntryPoint (VtfInfo, &CompStartAddress);
  }

  return Status;
}

EFI_STATUS
CreateAndUpdatePAL_A (
  IN  PARSED_VTF_INFO   *VtfInfo
  )
/*++

Routine Description:

  This function reads the binary file for each components and update them
  in VTF Buffer as well as FIT table

Arguments:

  VtfInfo    - Pointer to Parsed Info

Returns:

  EFI_ABORTED           - Due to one of the following reasons:
                           (a)Error Opening File
                           (b)The PAL_A Size is more than specified size status
                              One of the values mentioned below returned from
                              call to UpdateSymFile
  EFI_SUCCESS           - The function completed successfully.
  EFI_INVALID_PARAMETER - One of the input parameters was invalid.
  EFI_ABORTED           - An error occurred.UpdateSymFile
  EFI_OUT_OF_RESOURCES  - Memory allocation failed.

--*/
{
  EFI_STATUS  Status;
  UINT64      PalStartAddress;
  UINT64      AbsAddress;
  UINTN       RelativeAddress;
  UINT64      FileSize;
  UINT8       *Buffer;
  FILE        *Fp;
  FIT_TABLE   *PalFitPtr;

  Fp = fopen (LongFilePath (VtfInfo->CompBinName), "rb");

  if (Fp == NULL) {
    Error (NULL, 0, 0001, "Error opening file", VtfInfo->CompBinName);
    return EFI_ABORTED;
  }

  FileSize = _filelength (fileno (Fp));
  if (FileSize < 64) {
    fclose (Fp);
    Error (NULL, 0, 2000, "Invalid parameter", "PAL_A bin header is 64 bytes, so the Bin size must be larger than 64 bytes!");
    return EFI_INVALID_PARAMETER;
  }
  FileSize -= SIZE_OF_PAL_HEADER;


  if (VtfInfo->PreferredSize) {
    if (FileSize > VtfInfo->CompSize) {
      fclose (Fp);
      Error (NULL, 0, 2000, "Invalid parameter", "The PAL_A Size is more than the specified size.");
      return EFI_ABORTED;
    }

    FileSize = VtfInfo->CompSize;
  }

  Buffer = malloc ((UINTN) FileSize);
  if (Buffer == NULL) {
    fclose (Fp);
    return EFI_OUT_OF_RESOURCES;
  }
  memset (Buffer, 0, (UINTN) FileSize);

  //
  // Read, Get version Info and discard the PAL header.
  //
  fread (Buffer, sizeof (UINT8), SIZE_OF_PAL_HEADER, Fp);

  //
  // Extract the version info from header of PAL_A. Once done, discrad this buffer
  //
  if (!VtfInfo->VersionPresent) {
    GetComponentVersionInfo (VtfInfo, Buffer);
  }

  //
  // Read PAL_A file in a buffer
  //
  fread (Buffer, sizeof (UINT8), (UINTN) FileSize, Fp);
  fclose (Fp);

  PalStartAddress       = Fv1EndAddress - (SIZE_TO_OFFSET_PAL_A_END + FileSize);
  Vtf1LastStartAddress  = PalStartAddress;
  Vtf1TotalSize += (UINT32) FileSize;
  Status      = UpdateVtfBuffer (PalStartAddress, Buffer, FileSize, FIRST_VTF);

  AbsAddress  = Fv1EndAddress - SIZE_TO_PAL_A_FIT;
  GetRelativeAddressInVtfBuffer (AbsAddress, &RelativeAddress, FIRST_VTF);
  PalFitPtr               = (FIT_TABLE *) RelativeAddress;
  PalFitPtr->CompAddress  = PalStartAddress | IPF_CACHE_BIT;
  //assert ((FileSize % 16) == 0);
  if ((FileSize % 16) != 0) {
    free (Buffer);
    Error (NULL, 0, 2000, "Invalid parameter", "Binary FileSize must be a multiple of 16.");
    return EFI_INVALID_PARAMETER;
  }

  PalFitPtr->CompSize     = (UINT32) (FileSize / 16);
  PalFitPtr->CompVersion  = MAKE_VERSION (VtfInfo->MajorVer, VtfInfo->MinorVer);
  PalFitPtr->CvAndType    = CV_N_TYPE (VtfInfo->CheckSumRequired, VtfInfo->CompType);
  if (VtfInfo->CheckSumRequired) {
    PalFitPtr->CheckSum = 0;
    PalFitPtr->CheckSum = CalculateChecksum8 (Buffer, (UINTN) FileSize);
  }

  if (Buffer) {
    free (Buffer);
  }

  //
  // Update the SYM file for this component based on it's start address.
  //
  Status = UpdateSymFile (PalStartAddress, SymFileName, VtfInfo->CompSymName, FileSize);
  if (EFI_ERROR (Status)) {

    //
    // At this time, SYM files are not required, so continue on error.
    //
  }

  return Status;
}

EFI_STATUS
CreateFitTableAndInitialize (
  IN  PARSED_VTF_INFO   *VtfInfo
  )
/*++

Routine Description:

  This function creates and intializes FIT table which would be used to
  add component info inside this

Arguments:

  VtfInfo    - Pointer to Parsed Info

Returns:

  EFI_ABORTED  - Aborted due to no size information
  EFI_SUCCESS  - The function completed successfully

--*/
{
  UINT64    PalFitTableAdd;
  UINT64    FitTableAdd;
  UINT64    FitTableAddressOffset;
  FIT_TABLE *PalFitPtr;
  FIT_TABLE *FitStartPtr;
  UINTN     NumFitComp;
  UINTN     RelativeAddress;
  UINTN     Index;

  if (!VtfInfo->PreferredSize) {
    Error (NULL, 0, 2000, "Invalid parameter", "FIT could not be allocated because there is no size information.");
    return EFI_ABORTED;
  }

  if ((VtfInfo->CompSize % 16) != 0) {
    Error (NULL, 0, 2000, "Invalid parameter", "Invalid FIT Table Size, it is not a multiple of 16 bytes. Please correct the size.");
  }

  PalFitTableAdd = Fv1EndAddress - SIZE_TO_PAL_A_FIT;
  GetRelativeAddressInVtfBuffer (PalFitTableAdd, &RelativeAddress, FIRST_VTF);
  PalFitPtr             = (FIT_TABLE *) RelativeAddress;
  PalFitTableAdd        = (PalFitPtr->CompAddress - VtfInfo->CompSize);

  FitTableAdd           = (PalFitPtr->CompAddress - 0x10) - VtfInfo->CompSize;
  FitTableAddressOffset = Fv1EndAddress - (SIZE_IA32_RESET_VECT + SIZE_SALE_ENTRY_POINT + SIZE_FIT_TABLE_ADD);
  GetRelativeAddressInVtfBuffer (FitTableAddressOffset, &RelativeAddress, FIRST_VTF);
  *(UINT64 *) RelativeAddress = FitTableAdd;

  GetRelativeAddressInVtfBuffer (FitTableAdd, &RelativeAddress, FIRST_VTF);

  //
  // Update Fit Table with FIT Signature and FIT info in first 16 bytes.
  //
  FitStartPtr = (FIT_TABLE *) RelativeAddress;

  strncpy ((CHAR8 *) &FitStartPtr->CompAddress, FIT_SIGNATURE, 8);  // "_FIT_   "
  assert (((VtfInfo->CompSize & 0x00FFFFFF) % 16) == 0);
  FitStartPtr->CompSize     = (VtfInfo->CompSize & 0x00FFFFFF) / 16;
  FitStartPtr->CompVersion  = MAKE_VERSION (VtfInfo->MajorVer, VtfInfo->MinorVer);

  //
  // BUGBUG: If a checksum is required, add code to checksum the FIT table.  Also
  // determine what to do for things like the FV component that aren't easily checksummed.
  // The checksum will be done once we are done with all the componet update in the FIT
  // table
  //
  FitStartPtr->CvAndType  = CV_N_TYPE (VtfInfo->CheckSumRequired, VtfInfo->CompType);

  NumFitComp              = FitStartPtr->CompSize;

  FitStartPtr++;

  //
  // Intialize remaining FIT table space to UNUSED fit component type
  // so that when we need to create a FIT entry for a component, we can
  // locate a free one and use it.
  //
  for (Index = 0; Index < (NumFitComp - 1); Index++) {
    FitStartPtr->CvAndType = 0x7F;  // Initialize all with UNUSED
    FitStartPtr++;
  }

  Vtf1TotalSize += VtfInfo->CompSize;
  Vtf1LastStartAddress -= VtfInfo->CompSize;

  return EFI_SUCCESS;
}

EFI_STATUS
WriteVtfBinary (
  IN CHAR8     *FileName,
  IN UINT32    VtfSize,
  IN LOC_TYPE  LocType
  )
/*++

Routine Description:

  Write Firmware Volume from memory to a file.

Arguments:

  FileName     - Output File Name which needed to be created/
  VtfSize      - FileSize
  LocType      - The type of the VTF

Returns:

  EFI_ABORTED - Returned due to one of the following resons:
                 (a) Error Opening File
                 (b) Failing to copy buffers
  EFI_SUCCESS - The fuction completes successfully

--*/
{
  FILE  *Fp;
  UINTN NumByte;
  VOID  *VtfBuffer;
  UINTN RelativeAddress;

  if (LocType == FIRST_VTF) {
    GetRelativeAddressInVtfBuffer (Vtf1LastStartAddress, &RelativeAddress, FIRST_VTF);
    VtfBuffer = (VOID *) RelativeAddress;
  } else {
    GetRelativeAddressInVtfBuffer (Vtf2LastStartAddress, &RelativeAddress, SECOND_VTF);
    VtfBuffer = (VOID *) RelativeAddress;
  }

  Fp = fopen (LongFilePath (FileName), "wb");
  if (Fp == NULL) {
    Error (NULL, 0, 0001, "Error opening file", FileName);
    return EFI_ABORTED;
  }

  NumByte = fwrite (VtfBuffer, sizeof (UINT8), (UINTN) VtfSize, Fp);

  if (Fp) {
    fclose (Fp);
  }

  if (NumByte != (sizeof (UINT8) * VtfSize)) {
    Error (NULL, 0, 0002, "Error writing file", FileName);
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
UpdateVtfBuffer (
  IN  UINT64   StartAddress,
  IN  UINT8    *Buffer,
  IN  UINT64   DataSize,
  IN LOC_TYPE  LocType
  )
/*++

Routine Description:

  Update the Firmware Volume Buffer with requested buffer data

Arguments:

  StartAddress   - StartAddress in buffer. This number will automatically
                  point to right address in buffer where data needed
                  to be updated.
  Buffer         - Buffer pointer from data will be copied to memory mapped buffer.
  DataSize       - Size of the data needed to be copied.
  LocType        - The type of the VTF: First or Second

Returns:

  EFI_ABORTED  - The input parameter is error
  EFI_SUCCESS  - The function completed successfully

--*/
{
  UINT8 *LocalBufferPtrToWrite;

  if (LocType == FIRST_VTF) {
    if ((StartAddress | IPF_CACHE_BIT) < (Vtf1LastStartAddress | IPF_CACHE_BIT)) {
      Error (NULL, 0, 2000, "Invalid parameter", "Start Address is less than the VTF start address.");
      return EFI_ABORTED;
    }

    LocalBufferPtrToWrite = (UINT8 *) Vtf1EndBuffer;

    LocalBufferPtrToWrite -= (Fv1EndAddress - StartAddress);

  } else {

    if ((StartAddress | IPF_CACHE_BIT) < (Vtf2LastStartAddress | IPF_CACHE_BIT)) {
      Error (NULL, 0, 2000, "Invalid parameter", "Error StartAddress");
      return EFI_ABORTED;
    }
    LocalBufferPtrToWrite = (UINT8 *) Vtf2EndBuffer;
    LocalBufferPtrToWrite -= (Fv2EndAddress - StartAddress);
  }

  memcpy (LocalBufferPtrToWrite, Buffer, (UINTN) DataSize);

  return EFI_SUCCESS;
}

EFI_STATUS
UpdateFfsHeader (
  IN UINT32         TotalVtfSize,
  IN LOC_TYPE       LocType
  )
/*++

Routine Description:

  Update the Firmware Volume Buffer with requested buffer data

Arguments:

  TotalVtfSize     - Size of the VTF
  Fileoffset       - The start of the file relative to the start of the FV.
  LocType          - The type of the VTF

Returns:

  EFI_SUCCESS            - The function completed successfully
  EFI_INVALID_PARAMETER  - The Ffs File Header Pointer is NULL

--*/
{
  EFI_FFS_FILE_HEADER *FileHeader;
  UINTN               RelativeAddress;
  EFI_GUID            EfiFirmwareVolumeTopFileGuid = EFI_FFS_VOLUME_TOP_FILE_GUID;

  //
  // Find the VTF file header location
  //
  if (LocType == FIRST_VTF) {
    GetRelativeAddressInVtfBuffer (Vtf1LastStartAddress, &RelativeAddress, FIRST_VTF);
    FileHeader = (EFI_FFS_FILE_HEADER *) RelativeAddress;
  } else {
    GetRelativeAddressInVtfBuffer (Vtf2LastStartAddress, &RelativeAddress, SECOND_VTF);
    FileHeader = (EFI_FFS_FILE_HEADER *) RelativeAddress;
  }

  if (FileHeader == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // write header
  //
  memset (FileHeader, 0, sizeof (EFI_FFS_FILE_HEADER));
  memcpy (&FileHeader->Name, &EfiFirmwareVolumeTopFileGuid, sizeof (EFI_GUID));
  FileHeader->Type        = EFI_FV_FILETYPE_RAW;
  FileHeader->Attributes  = FFS_ATTRIB_CHECKSUM;

  //
  // Now FileSize includes the EFI_FFS_FILE_HEADER
  //
  FileHeader->Size[0] = (UINT8) (TotalVtfSize & 0x000000FF);
  FileHeader->Size[1] = (UINT8) ((TotalVtfSize & 0x0000FF00) >> 8);
  FileHeader->Size[2] = (UINT8) ((TotalVtfSize & 0x00FF0000) >> 16);

  //
  // Fill in checksums and state, all three must be zero for the checksums.
  //
  FileHeader->IntegrityCheck.Checksum.Header  = 0;
  FileHeader->IntegrityCheck.Checksum.File    = 0;
  FileHeader->State                           = 0;
  FileHeader->IntegrityCheck.Checksum.Header  = CalculateChecksum8 ((UINT8 *) FileHeader, sizeof (EFI_FFS_FILE_HEADER));
  FileHeader->IntegrityCheck.Checksum.File    = CalculateChecksum8 ((UINT8 *) (FileHeader + 1), TotalVtfSize - sizeof (EFI_FFS_FILE_HEADER));
  FileHeader->State                           = EFI_FILE_HEADER_CONSTRUCTION | EFI_FILE_HEADER_VALID | EFI_FILE_DATA_VALID;

  return EFI_SUCCESS;
}

EFI_STATUS
ValidateAddressAndSize (
  IN  UINT64  BaseAddress,
  IN  UINT64  FwVolSize
  )
/*++

Routine Description:

  Update the Firmware Volume Buffer with requested buffer data

Arguments:

  BaseAddress    - Base address for the Fw Volume.

  FwVolSize      - Total Size of the FwVolume to which VTF will be attached..

Returns:

  EFI_SUCCESS     - The function completed successfully
  EFI_UNSUPPORTED - The input parameter is error

--*/
{
  if ((FwVolSize > 0x40) && ((BaseAddress + FwVolSize) % 8 == 0)) {
    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}

EFI_STATUS
UpdateIA32ResetVector (
  IN  CHAR8   *FileName,
  IN  UINT64  FirstFwVSize
  )
/*++

Routine Description:

  Update the 16 byte IA32 Reset vector to maintain the compatibility

Arguments:

  FileName     - Binary file name which contains the IA32 Reset vector info..
  FirstFwVSize - Total Size of the FwVolume to which VTF will be attached..

Returns:

  EFI_SUCCESS            - The function completed successfully
  EFI_ABORTED            - Invalid File Size
  EFI_INVALID_PARAMETER  - Bad File Name
  EFI_OUT_OF_RESOURCES   - Memory allocation failed.

--*/
{
  UINT8 *Buffer;
  UINT8 *LocalVtfBuffer;
  UINTN FileSize;
  FILE  *Fp;

  if (!strcmp (FileName, "")) {
    return EFI_INVALID_PARAMETER;
  }

  Fp = fopen (LongFilePath (FileName), "rb");

  if (Fp == NULL) {
    Error (NULL, 0, 0001, "Error opening file", FileName);
    return EFI_ABORTED;
  }

  FileSize = _filelength (fileno (Fp));

  if (FileSize > 16) {
    fclose (Fp);
    return EFI_ABORTED;
  }

  Buffer = malloc (FileSize);
  if (Buffer == NULL) {
    fclose (Fp);
    return EFI_OUT_OF_RESOURCES;
  }

  fread (Buffer, sizeof (UINT8), FileSize, Fp);

  LocalVtfBuffer  = (UINT8 *) Vtf1EndBuffer - SIZE_IA32_RESET_VECT;
  memcpy (LocalVtfBuffer, Buffer, FileSize);

  if (Buffer) {
    free (Buffer);
  }

  if (Fp != NULL) {
    fclose (Fp);
  }
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

  NONE

Returns:

  NONE

--*/
{
  PARSED_VTF_INFO *TempFileListPtr;

  if (Vtf1Buffer) {
    free (Vtf1Buffer);
  }

  if (Vtf2Buffer) {
    free (Vtf2Buffer);
  }

  //
  // Cleanup the buffer which was allocated to read the file names from FV.INF
  //
  FileListPtr = FileListHeadPtr;
  while (FileListPtr != NULL) {
    TempFileListPtr = FileListPtr->NextVtfInfo;
    free (FileListPtr);
    FileListPtr = TempFileListPtr;
  }
}

EFI_STATUS
ProcessAndCreateVtf (
  IN  UINT64  Size
  )
/*++

Routine Description:

  This function process the link list created during INF file parsing
  and create component in VTF and updates its info in FIT table

Arguments:

  Size   - Size of the Firmware Volume of which, this VTF belongs to.

Returns:

  EFI_UNSUPPORTED - Unknown FIT type
  EFI_SUCCESS     - The function completed successfully

--*/
{
  EFI_STATUS      Status;
  PARSED_VTF_INFO *ParsedInfoPtr;

  Status        = EFI_SUCCESS;

  ParsedInfoPtr = FileListHeadPtr;

  while (ParsedInfoPtr != NULL) {

    switch (ParsedInfoPtr->CompType) {
    //
    // COMP_TYPE_FIT_HEADER is a special case, hence handle it here
    //
    case COMP_TYPE_FIT_HEADER:
      //COMP_TYPE_FIT_HEADER          0x00
      Status = CreateFitTableAndInitialize (ParsedInfoPtr);
      break;

    //
    // COMP_TYPE_FIT_PAL_A is a special case, hence handle it here
    //
    case COMP_TYPE_FIT_PAL_A:
      //COMP_TYPE_FIT_PAL_A           0x0F
      Status = CreateAndUpdatePAL_A (ParsedInfoPtr);

      //
      // Based on VTF specification, once the PAL_A component has been written,
      // update the Firmware Volume info as FIT table. This will be utilized
      // to extract the Firmware Volume Start address where this VTF will be
      // of part.
      //
      if (Status == EFI_SUCCESS) {
        UpdateFitEntryForFwVolume (Size);
      }
      break;

    case COMP_TYPE_FIT_FV_BOOT:
      //COMP_TYPE_FIT_FV_BOOT         0x7E
      //
      // Since FIT entry for Firmware Volume has been created and it is
      // located at (PAL_A start - 16 byte). So we will not process any
      // Firmware Volume related entry from INF file
      //
      Status = EFI_SUCCESS;
      break;

    default:
      //
      // Any other component type should be handled here. This will create the
      // image in specified VTF and create appropriate entry about this
      // component in FIT Entry.
      //
      Status = CreateAndUpdateComponent (ParsedInfoPtr);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 0002, "Error updating component", ParsedInfoPtr->CompName);
        return EFI_ABORTED;
      } else {
      break;}
    }

    ParsedInfoPtr = ParsedInfoPtr->NextVtfInfo;
  }
  return Status;
}

EFI_STATUS
GenerateVtfImage (
  IN  UINT64  StartAddress1,
  IN  UINT64  Size1,
  IN  UINT64  StartAddress2,
  IN  UINT64  Size2,
  IN  FILE    *fp
  )
/*++

Routine Description:

  This is the main function which will be called from application.

Arguments:

  StartAddress1  - The start address of the first VTF
  Size1          - The size of the first VTF
  StartAddress2  - The start address of the second VTF
  Size2          - The size of the second VTF
  fp             - The pointer to BSF inf file

Returns:

  EFI_OUT_OF_RESOURCES - Can not allocate memory
  The return value can be any of the values
  returned by the calls to following functions:
      GetVtfRelatedInfoFromInfFile
      ProcessAndCreateVtf
      UpdateIA32ResetVector
      UpdateFfsHeader
      WriteVtfBinary

--*/
{
  EFI_STATUS  Status;
  FILE            *VtfFP;

  Status          = EFI_UNSUPPORTED;
  VtfFP = fp;

  if (StartAddress2 == 0) {
    SecondVTF = FALSE;
  } else {
    SecondVTF = TRUE;
  }

  Fv1BaseAddress        = StartAddress1;
  Fv1EndAddress         = Fv1BaseAddress + Size1;
  if (Fv1EndAddress != 0x100000000ULL || Size1 < 0x100000) {
    Error (NULL, 0, 2000, "Invalid parameter", "Error BaseAddress and Size parameters!");
    if (Size1 < 0x100000) {
      Error (NULL, 0, 2000, "Invalid parameter", "The FwVolumeSize must be larger than 1M!");
    } else if (SecondVTF != TRUE) {
      Error (NULL, 0, 2000, "Invalid parameter", "BaseAddress + FwVolumeSize must equal 0x100000000!");
    }
    Usage();
    return EFI_INVALID_PARAMETER;
  }

  //
  // The image buffer for the First VTF
  //
  Vtf1Buffer = malloc ((UINTN) Size1);
  if (Vtf1Buffer == NULL) {
    Error (NULL, 0, 4001, "Resource", "Not enough resources available to create memory mapped file for the Boot Strap File!");
    return EFI_OUT_OF_RESOURCES;
  }
  memset (Vtf1Buffer, 0x00, (UINTN) Size1);
  Vtf1EndBuffer         = (UINT8 *) Vtf1Buffer + Size1;
  Vtf1LastStartAddress  = Fv1EndAddress | IPF_CACHE_BIT;

  if (SecondVTF) {
    Fv2BaseAddress        = StartAddress2;
    Fv2EndAddress         = Fv2BaseAddress + Size2;
    if (Fv2EndAddress != StartAddress1) {
      Error (NULL, 0, 2000, "Invalid parameter", "Error BaseAddress and Size parameters!");
      if (SecondVTF == TRUE) {
        Error (NULL, 0, 2000, "Invalid parameter", "FirstBaseAddress + FirstFwVolumeSize must equal 0x100000000!");
        Error (NULL, 0, 2000, "Invalid parameter", "SecondBaseAddress + SecondFwVolumeSize must equal FirstBaseAddress!");
      }
      Usage();
      return EFI_INVALID_PARAMETER;
    }

    //
    // The image buffer for the second VTF
    //
    Vtf2Buffer = malloc ((UINTN) Size2);
    if (Vtf2Buffer == NULL) {
      Error (NULL, 0, 4001, "Resource", "Not enough resources available to create memory mapped file for the Boot Strap File!");
      return EFI_OUT_OF_RESOURCES;
    }
    memset (Vtf2Buffer, 0x00, (UINTN) Size2);
    Vtf2EndBuffer         = (UINT8 *) Vtf2Buffer + Size2;
    Vtf2LastStartAddress  = Fv2EndAddress | IPF_CACHE_BIT;
  }

  Status = GetVtfRelatedInfoFromInfFile (VtfFP);

  if (Status != EFI_SUCCESS) {
    Error (NULL, 0, 0003, "Error parsing file", "the input file.");
    CleanUpMemory ();
    return Status;
  }

  Status = ProcessAndCreateVtf (Size1);
  if (Status != EFI_SUCCESS) {
    CleanUpMemory ();
    return Status;
  }

  if (SectionOptionFlag) {
    Status = UpdateIA32ResetVector (IA32BinFile, Vtf1TotalSize);
    if (Status != EFI_SUCCESS) {
      CleanUpMemory ();
      return Status;
    }
  }

  //
  // Re arrange the FIT Table for Ascending order of their FIT Type..
  //
  SortFitTable ();

  //
  // All components have been updated in FIT table. Now perform the FIT table
  // checksum. The following function will check if Checksum is required,
  // if yes, then it will perform the checksum otherwise not.
  //
  CalculateFitTableChecksum ();

  //
  // Write the FFS header
  //
  Vtf1TotalSize += sizeof (EFI_FFS_FILE_HEADER);
  Vtf1LastStartAddress -= sizeof (EFI_FFS_FILE_HEADER);

  Status = UpdateFfsHeader (Vtf1TotalSize, FIRST_VTF);
  if (Status != EFI_SUCCESS) {
    CleanUpMemory ();
    return Status;
  }
  //
  // Update the VTF buffer into specified VTF binary file
  //
  Status  = WriteVtfBinary (OutFileName1, Vtf1TotalSize, FIRST_VTF);

  if (SecondVTF) {
    Vtf2TotalSize += sizeof (EFI_FFS_FILE_HEADER);
    Vtf2LastStartAddress -= sizeof (EFI_FFS_FILE_HEADER);
    Status = UpdateFfsHeader (Vtf2TotalSize, SECOND_VTF);
    if (Status != EFI_SUCCESS) {
      CleanUpMemory ();
      return Status;
    }

    //
    // Update the VTF buffer into specified VTF binary file
    //
    Status  = WriteVtfBinary (OutFileName2, Vtf2TotalSize, SECOND_VTF);
  }

  CleanUpMemory ();

  return Status;
}

EFI_STATUS
PeimFixupInFitTable (
  IN  UINT64  StartAddress
  )
/*++

Routine Description:

  This function is an entry point to fixup SAL-E entry point.

Arguments:

  StartAddress - StartAddress for PEIM.....

Returns:

  EFI_SUCCESS          - The function completed successfully
  EFI_ABORTED          - Error Opening File
  EFI_OUT_OF_RESOURCES - System out of resources for memory allocation.

--*/
{
  EFI_STATUS  Status;
  FILE        *Fp;
  UINT64      *StartAddressPtr;
  UINTN       FirstFwVSize;

  StartAddressPtr   = malloc (sizeof (UINT64));
  if (StartAddressPtr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  *StartAddressPtr = StartAddress;

  Fp = fopen (LongFilePath (OutFileName1), "rb");

  if (Fp == NULL) {
    Error (NULL, 0, 0001, "Error opening file", OutFileName1);
    if (StartAddressPtr) {
      free (StartAddressPtr);
    }
    return EFI_ABORTED;
  }

  FirstFwVSize = _filelength (fileno (Fp));
  fseek (Fp, (long) (FirstFwVSize - (UINTN) (SIZE_IA32_RESET_VECT + SIZE_SALE_ENTRY_POINT)), SEEK_SET);
  fwrite ((VOID *) StartAddressPtr, sizeof (UINT64), 1, Fp);

  if (Fp) {
    fclose (Fp);
  }

  if (StartAddressPtr) {
    free (StartAddressPtr);
  }

  Status = EFI_SUCCESS;
  return Status;
}

EFI_STATUS
UpdateSymFile (
  IN UINT64 BaseAddress,
  IN CHAR8  *DestFileName,
  IN CHAR8  *SourceFileName,
  IN UINT64 FileSize

  )
/*++

Routine Description:

  This function adds the SYM tokens in the source file to the destination file.
  The SYM tokens are updated to reflect the base address.

Arguments:

  BaseAddress    - The base address for the new SYM tokens.
  DestFileName   - The destination file.
  SourceFileName - The source file.
  FileSize       - Size of bin file.

Returns:

  EFI_SUCCESS             - The function completed successfully.
  EFI_INVALID_PARAMETER   - One of the input parameters was invalid.
  EFI_ABORTED             - An error occurred.

--*/
{
  FILE    *SourceFile;
  FILE    *DestFile;
  CHAR8   Buffer[MAX_LONG_FILE_PATH];
  CHAR8   Type[MAX_LONG_FILE_PATH];
  CHAR8   Address[MAX_LONG_FILE_PATH];
  CHAR8   Section[MAX_LONG_FILE_PATH];
  CHAR8   Token[MAX_LONG_FILE_PATH];
  CHAR8   BaseToken[MAX_LONG_FILE_PATH];
  CHAR8   FormatString[MAX_LINE_LEN];
  UINT64  TokenAddress;
  long    StartLocation;

  //
  // Verify input parameters.
  //
  if (BaseAddress == 0 || DestFileName == NULL || SourceFileName == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Open the source file
  //
  SourceFile = fopen (LongFilePath (SourceFileName), "r");
  if (SourceFile == NULL) {

    //
    // SYM files are not required.
    //
    return EFI_SUCCESS;
  }

  //
  // Use the file name minus extension as the base for tokens
  //
  if (strlen (SourceFileName) >= MAX_LONG_FILE_PATH) {
    fclose (SourceFile);
    Error (NULL, 0, 2000, "Invalid parameter", "The source file name is too long.");
    return EFI_ABORTED;
  }
  strncpy (BaseToken, SourceFileName, MAX_LONG_FILE_PATH - 1);
  BaseToken[MAX_LONG_FILE_PATH - 1] = 0;
  strtok (BaseToken, ". \t\n");
  if (strlen (BaseToken) + strlen ("__") >= MAX_LONG_FILE_PATH) {
    fclose (SourceFile);
    Error (NULL, 0, 2000, "Invalid parameter", "The source file name is too long.");
    return EFI_ABORTED;
  }
  strncat (BaseToken, "__", MAX_LONG_FILE_PATH - strlen (BaseToken) - 1);

  //
  // Open the destination file
  //
  DestFile = fopen (LongFilePath (DestFileName), "a+");
  if (DestFile == NULL) {
    fclose (SourceFile);
    Error (NULL, 0, 0001, "Error opening file", DestFileName);
    return EFI_ABORTED;
  }

  //
  // If this is the beginning of the output file, write the symbol format info.
  //
  if (fseek (DestFile, 0, SEEK_END) != 0) {
    fclose (SourceFile);
    fclose (DestFile);
    Error (NULL, 0, 2000, "Invalid parameter", "not at the beginning of the output file.");
    return EFI_ABORTED;
  }

  StartLocation = ftell (DestFile);

  if (StartLocation == 0) {
    fprintf (DestFile, "TEXTSYM format | V1.0\n");
  } else if (StartLocation == -1) {
    fclose (SourceFile);
    fclose (DestFile);
    Error (NULL, 0, 2000, "Invalid parameter", "StartLocation error");
    return EFI_ABORTED;
  }

  //
  // Read the first line
  //
  if (fgets (Buffer, MAX_LONG_FILE_PATH, SourceFile) == NULL) {
    Buffer[0] = 0;
  }

  //
  // Make sure it matches the expected sym format
  //
  if (strcmp (Buffer, "TEXTSYM format | V1.0\n")) {
    fclose (SourceFile);
    fclose (DestFile);
    Error (NULL, 0, 2000, "Invalid parameter", "The symbol file does not match the expected TEXTSYM format (V1.0.)");
    return EFI_ABORTED;
  }

  //
  // Generate the format string for fscanf
  //
  sprintf (
    FormatString,
    "%%%us | %%%us | %%%us | %%%us\n",
    (unsigned) sizeof (Type) - 1,
    (unsigned) sizeof (Address) - 1,
    (unsigned) sizeof (Section) - 1,
    (unsigned) sizeof (Token) - 1
    );

  //
  // Read in the file
  //
  while (feof (SourceFile) == 0) {

    //
    // Read a line
    //
    if (fscanf (SourceFile, FormatString, Type, Address, Section, Token) == 4) {

      //
      // Get the token address
      //
      AsciiStringToUint64 (Address, TRUE, &TokenAddress);
      if (TokenAddress > FileSize) {
        //
        // Symbol offset larger than FileSize. This Symbol can't be in Bin file. Don't print them.
        //
        break;
      }

      //
      // Add the base address, the size of the FFS file header and the size of the peim header.
      //
      TokenAddress += BaseAddress &~IPF_CACHE_BIT;

      fprintf (DestFile, "%s | %016llX | ", Type, (unsigned long long) TokenAddress);
      fprintf (DestFile, "%s | %s\n    %s\n", Section, Token, BaseToken); 
    }
  }

  fclose (SourceFile);
  fclose (DestFile);
  return EFI_SUCCESS;
}

EFI_STATUS
CalculateFitTableChecksum (
  VOID
  )
/*++

Routine Description:

  This function will perform byte checksum on the FIT table, if the the checksum required
  field is set to CheckSum required. If the checksum is not required then checksum byte
  will have value as 0;.

Arguments:

  NONE

Returns:

  Status       - Value returned by call to CalculateChecksum8 ()
  EFI_SUCCESS  - The function completed successfully

--*/
{
  FIT_TABLE *TmpFitPtr;
  UINT64    FitTableAdd;
  UINT64    FitTableAddOffset;
  UINTN     RelativeAddress;
  UINTN     Size;

  //
  // Read the Fit Table address from Itanium-based address map.
  //
  FitTableAddOffset = Fv1EndAddress - (SIZE_IA32_RESET_VECT + SIZE_SALE_ENTRY_POINT + SIZE_FIT_TABLE_ADD);

  //
  // Translate this Itanium-based address in terms of local buffer address which
  // contains the image for Boot Strapped File
  //
  GetRelativeAddressInVtfBuffer (FitTableAddOffset, &RelativeAddress, FIRST_VTF);
  FitTableAdd = *(UINTN *) RelativeAddress;

  GetRelativeAddressInVtfBuffer (FitTableAdd, &RelativeAddress, FIRST_VTF);

  TmpFitPtr = (FIT_TABLE *) RelativeAddress;

  Size      = TmpFitPtr->CompSize * 16;

  if ((TmpFitPtr->CvAndType & CHECKSUM_BIT_MASK) >> 7) {
    TmpFitPtr->CheckSum = 0;
    TmpFitPtr->CheckSum = CalculateChecksum8 ((UINT8 *) TmpFitPtr, Size);
  } else {
    TmpFitPtr->CheckSum = 0;
  }

  return EFI_SUCCESS;
}

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
  fprintf (stdout, "%s Version %d.%d %s \n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION, __BUILD_VERSION);
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
  //
  // Summary usage
  //
  fprintf (stdout, "Usage: %s [options] <-f input_file> <-r BaseAddress> <-s FwVolumeSize>\n\n", UTILITY_NAME);

  //
  // Copyright declaration
  //
  fprintf (stdout, "Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.\n\n");
  //
  // Details Option
  //
  fprintf (stdout, "Options:\n");
  fprintf (stdout, "  -f Input_file,   --filename Input_file\n\
                        Input_file is name of the BS Image INF file\n");
  fprintf (stdout, "  -r BaseAddress,  --baseaddr BaseAddress\n\
                        BaseAddress is the starting address of Firmware Volume\n\
                        where Boot Strapped Image will reside.\n");
  fprintf (stdout, "  -s FwVolumeSize, --size FwVolumeSize\n\
                        FwVolumeSize is the size of Firmware Volume.\n");
  fprintf (stdout, "  -o FileName,     --output FileName\n\
                        File will be created to store the ouput content.\n");
  fprintf (stdout, "  -v, --verbose         Turn on verbose output with informational messages.\n");
  fprintf (stdout, "  --version             Show program's version number and exit.\n");
  fprintf (stdout, "  -h, --help            Show this help message and exit.\n");
  fprintf (stdout, "  -q, --quiet           Disable all messages except FATAL ERRORS.\n");
  fprintf (stdout, "  -d, --debug [#, 0-9]  Enable debug messages at level #.\n");
}

int
main (
  IN  int  argc,
  IN  char  **argv
  )
/*++

Routine Description:

  This utility uses GenVtf.dll to build a Boot Strap File Image which will be
  part of firmware volume image.

Arguments:

  argc   - The count of the parameters
  argv   - The parameters


Returns:

  0   - No error conditions detected.
  1   - One or more of the input parameters is invalid.
  2   - A resource required by the utility was unavailable.
      - Most commonly this will be memory allocation or file creation.
  3   - GenFvImage.dll could not be loaded.
  4   - Error executing the GenFvImage dll.
  5   - Now this tool does not support the IA32 platform

--*/
{
  UINT8          Index;
  UINT64         StartAddress1;
  UINT64         StartAddress2;
  UINT64         FwVolSize1;
  UINT64         FwVolSize2;
  BOOLEAN       FirstRoundO;
  BOOLEAN       FirstRoundB;
  BOOLEAN       FirstRoundS;
  EFI_STATUS    Status;
  FILE          *VtfFP;
  CHAR8         *VtfFileName;

  SetUtilityName (UTILITY_NAME);

  //
  // Initialize variables
  //
  StartAddress1 = 0;
  StartAddress2 = 0;
  FwVolSize1    = 0;
  FwVolSize2    = 0;
  FirstRoundB   = TRUE;
  FirstRoundS   = TRUE;
  FirstRoundO   = TRUE;
  DebugMode     = FALSE;
  OutFileName1  = NULL;
  OutFileName2  = NULL;
  VtfFP = NULL;
  DebugLevel = 0;

  //
  // Verify the correct number of arguments
  //
  if (argc == 1) {
    Usage();
    return 0;
  }

  if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)) {
    Usage();
    return 0;
  }

  if ((strcmp(argv[1], "--version") == 0)) {
    Version();
    return 0;
  }

  //
  // Parse the command line arguments
  //
  for (Index = 1; Index < argc; Index += 2) {
    if ((stricmp (argv[Index], "-o") == 0) || (stricmp (argv[Index], "--output") == 0)) {
      if (argv[Index + 1] == NULL || argv[Index + 1][0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "Output file is missing for -o option");
        goto ERROR;
      }
      //
      // Get the output file name
      //
      VTF_OUTPUT = TRUE;
      if (FirstRoundO) {
        //
        // It's the first output file name
        //
        OutFileName1 = (CHAR8 *)argv[Index+1];
        FirstRoundO = FALSE;
      } else {
        //
        //It's the second output file name
        //
        OutFileName2 = (CHAR8 *)argv[Index+1];
      }
      continue;
    }

    if ((stricmp (argv[Index], "-f") == 0) || (stricmp (argv[Index], "--filename") == 0)) {
      if (argv[Index + 1] == NULL || argv[Index + 1][0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "BS Image INF file is missing for -f option");
        goto ERROR;
      }
      //
      // Get the input VTF file name
      //
      VtfFileName = argv[Index+1];
      if (VtfFP != NULL) {
        //
        // VTF file name has been given previously, override with the new value
        //
        fclose (VtfFP);
      }
      VtfFP = fopen (LongFilePath (VtfFileName), "rb");
      if (VtfFP == NULL) {
        Error (NULL, 0, 0001, "Error opening file", VtfFileName);
        goto ERROR;
      }
      continue;
    }
    
    if ((stricmp (argv[Index], "-r") == 0) || (stricmp (argv[Index], "--baseaddr") == 0)) {
      if (FirstRoundB) {
        Status      = AsciiStringToUint64 (argv[Index + 1], FALSE, &StartAddress1);
        FirstRoundB = FALSE;
      } else {
        Status = AsciiStringToUint64 (argv[Index + 1], FALSE, &StartAddress2);
      }
      if (Status != EFI_SUCCESS) {
        Error (NULL, 0, 2000, "Invalid option value", "%s is Bad FV start address.", argv[Index + 1]);
        goto ERROR;
      }  
      continue;
    }

    if ((stricmp (argv[Index], "-s") == 0) || (stricmp (argv[Index], "--size") == 0)) {
      if (FirstRoundS) {
        Status      = AsciiStringToUint64 (argv[Index + 1], FALSE, &FwVolSize1);
        FirstRoundS = FALSE;
      } else {
        Status = AsciiStringToUint64 (argv[Index + 1], FALSE, &FwVolSize2);
    	  SecondVTF = TRUE;
      }

      if (Status != EFI_SUCCESS) {
        Error (NULL, 0, 2000, "Invalid option value", "%s is Bad FV size.", argv[Index + 1]);
        goto ERROR;
      }
      continue;
    }

    if ((stricmp (argv[Index], "-v") == 0) || (stricmp (argv[Index], "--verbose") == 0)) {
	    VerboseMode = TRUE;
	    Index--;
      continue;
    }

    if ((stricmp (argv[Index], "-q") == 0) || (stricmp (argv[Index], "--quiet") == 0)) {
      QuietMode = TRUE;
      Index--;
      continue;
    }

    if ((stricmp (argv[Index], "-d") == 0) || (stricmp (argv[Index], "--debug") == 0)) {
      //
      // debug level specified
      //
      Status = AsciiStringToUint64(argv[Index + 1], FALSE, &DebugLevel);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[Index], argv[Index + 1]);
        goto ERROR;
      }
      if (DebugLevel > 9)  {
        Error (NULL, 0, 2000, "Invalid option value", "Unrecognized argument %s.", argv[Index + 1]);
        goto ERROR;
      }
      if((DebugLevel <= 9) &&(DebugLevel >= 5)) {
        DebugMode = TRUE;
      } else {
        DebugMode = FALSE;
      }
      continue;
    }

    Error (NULL, 0, 2000, "Invalid parameter", "Unrecognized argument %s.", argv[Index]);
    goto ERROR;
  }

  if (VtfFP == NULL) {
    Error (NULL, 0, 2000, "Invalid parameter", "No BS Image INF file is specified");
    goto ERROR;
  }

  if (FirstRoundB) {
    Error (NULL, 0, 2000, "Invalid parameter", "No FV base address is specified");
    goto ERROR;
  }

  if (FirstRoundS) {
    Error (NULL, 0, 2000, "Invalid parameter", "No FV Size is specified");
    goto ERROR;
  }
  //
  // All Parameters has been parsed, now set the message print level
  //
  if (QuietMode) {
    SetPrintLevel(40);
  } else if (VerboseMode) {
    SetPrintLevel(15);
  } else if (DebugMode) {
    SetPrintLevel(DebugLevel);
  }

  if (VerboseMode) {
    VerboseMsg("%s tool start.\n", UTILITY_NAME);
  }

  if (VTF_OUTPUT == FALSE) {
    if (SecondVTF == TRUE) {
      OutFileName1 = VTF_OUTPUT_FILE1;
      OutFileName2 = VTF_OUTPUT_FILE2;
	  } else {
      OutFileName1 = VTF_OUTPUT_FILE1;
    }
    SymFileName = VTF_SYM_FILE;
  } else {
    INTN OutFileNameLen;
    INTN NewIndex;

    assert (OutFileName1);
    OutFileNameLen = strlen(OutFileName1);

    for (NewIndex = OutFileNameLen; NewIndex > 0; --NewIndex) {
      if (OutFileName1[NewIndex] == '/' || OutFileName1[NewIndex] == '\\') {
        break;
      }
    }
    if (NewIndex == 0) {
      SymFileName = VTF_SYM_FILE;
    } else {
      INTN SymFileNameLen = NewIndex + 1 + strlen(VTF_SYM_FILE);
      SymFileName = malloc(SymFileNameLen + 1);
      if (SymFileName == NULL) {
        Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
        goto ERROR;
      }
      memcpy(SymFileName, OutFileName1, NewIndex + 1);
      memcpy(SymFileName + NewIndex + 1, VTF_SYM_FILE, strlen(VTF_SYM_FILE));
      SymFileName[SymFileNameLen] = '\0';
    }
    if (DebugMode) {
      DebugMsg(UTILITY_NAME, 0, DebugLevel, SymFileName, NULL);
    }
  }

  //
  // Call the GenVtfImage
  //
  if (DebugMode) {
    DebugMsg(UTILITY_NAME, 0, DebugLevel, "Start to generate the VTF image\n", NULL);
  }
  Status = GenerateVtfImage (StartAddress1, FwVolSize1, StartAddress2, FwVolSize2, VtfFP);

  if (EFI_ERROR (Status)) {
    switch (Status) {

    case EFI_INVALID_PARAMETER:
      Error (NULL, 0, 2000, "Invalid parameter", "Invalid parameter passed to GenVtf function.");
      break;

    case EFI_ABORTED:
      Error (NULL, 0, 3000, "Invalid", "Error detected while creating the file image.");
      break;

    case EFI_OUT_OF_RESOURCES:
      Error (NULL, 0, 4002, "Resource", "GenVtfImage function could not allocate required resources.");
      break;

    case EFI_VOLUME_CORRUPTED:
      Error (NULL, 0, 3000, "Invalid", "No base address was specified.");
      break;

    default:
      Error (NULL, 0, 3000, "Invalid", "GenVtfImage function returned unknown status %x.", (int) Status );
      break;
    }
  }
ERROR:
  if (VtfFP != NULL) {
    fclose (VtfFP);
  }

  if (DebugMode) {
    DebugMsg(UTILITY_NAME, 0, DebugLevel, "VTF image generated successful\n", NULL);
  }

  if (VerboseMode) {
    VerboseMsg("%s tool done with return code is 0x%x.\n", UTILITY_NAME, GetUtilityStatus ());
  }
  return GetUtilityStatus();
}
