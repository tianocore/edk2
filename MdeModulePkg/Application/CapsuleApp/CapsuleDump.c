/** @file
  Dump Capsule image information.

  Copyright (c) 2016 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CapsuleApp.h"

/**
  Validate if it is valid capsule header

  This function assumes the caller provided correct CapsuleHeader pointer
  and CapsuleSize.

  This function validates the fields in EFI_CAPSULE_HEADER.

  @param[in] CapsuleHeader  Points to a capsule header.
  @param[in] CapsuleSize    Size of the whole capsule image.

**/
BOOLEAN
IsValidCapsuleHeader (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader,
  IN UINT64              CapsuleSize
  );

/**
  Dump UX capsule information.

  @param[in] CapsuleHeader      The UX capsule header
**/
VOID
DumpUxCapsule (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  EFI_DISPLAY_CAPSULE  *DisplayCapsule;

  DisplayCapsule = (EFI_DISPLAY_CAPSULE *)CapsuleHeader;
  Print (L"[UxCapsule]\n");
  Print (L"CapsuleHeader:\n");
  Print (L"  CapsuleGuid      - %g\n", &DisplayCapsule->CapsuleHeader.CapsuleGuid);
  Print (L"  HeaderSize       - 0x%x\n", DisplayCapsule->CapsuleHeader.HeaderSize);
  Print (L"  Flags            - 0x%x\n", DisplayCapsule->CapsuleHeader.Flags);
  Print (L"  CapsuleImageSize - 0x%x\n", DisplayCapsule->CapsuleHeader.CapsuleImageSize);
  Print (L"ImagePayload:\n");
  Print (L"  Version          - 0x%x\n", DisplayCapsule->ImagePayload.Version);
  Print (L"  Checksum         - 0x%x\n", DisplayCapsule->ImagePayload.Checksum);
  Print (L"  ImageType        - 0x%x\n", DisplayCapsule->ImagePayload.ImageType);
  Print (L"  Mode             - 0x%x\n", DisplayCapsule->ImagePayload.Mode);
  Print (L"  OffsetX          - 0x%x\n", DisplayCapsule->ImagePayload.OffsetX);
  Print (L"  OffsetY          - 0x%x\n", DisplayCapsule->ImagePayload.OffsetY);
}

/**
  Dump a non-nested FMP capsule.

  @param[in]  CapsuleHeader  A pointer to CapsuleHeader
**/
VOID
DumpFmpCapsule (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER        *FmpCapsuleHeader;
  UINT64                                        *ItemOffsetList;
  UINTN                                         Index;
  UINTN                                         Count;
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER  *FmpImageHeader;

  Print (L"[FmpCapsule]\n");
  Print (L"CapsuleHeader:\n");
  Print (L"  CapsuleGuid      - %g\n", &CapsuleHeader->CapsuleGuid);
  Print (L"  HeaderSize       - 0x%x\n", CapsuleHeader->HeaderSize);
  Print (L"  Flags            - 0x%x\n", CapsuleHeader->Flags);
  Print (L"  CapsuleImageSize - 0x%x\n", CapsuleHeader->CapsuleImageSize);

  FmpCapsuleHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER *)((UINT8 *)CapsuleHeader + CapsuleHeader->HeaderSize);
  ItemOffsetList   = (UINT64 *)(FmpCapsuleHeader + 1);
  Print (L"FmpHeader:\n");
  Print (L"  Version             - 0x%x\n", FmpCapsuleHeader->Version);
  Print (L"  EmbeddedDriverCount - 0x%x\n", FmpCapsuleHeader->EmbeddedDriverCount);
  Print (L"  PayloadItemCount    - 0x%x\n", FmpCapsuleHeader->PayloadItemCount);
  Count = FmpCapsuleHeader->EmbeddedDriverCount + FmpCapsuleHeader->PayloadItemCount;
  for (Index = 0; Index < Count; Index++) {
    Print (L"  Offset[%d]           - 0x%x\n", Index, ItemOffsetList[Index]);
  }

  for (Index = FmpCapsuleHeader->EmbeddedDriverCount; Index < Count; Index++) {
    Print (L"FmpPayload[%d] ImageHeader:\n", Index);
    FmpImageHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER *)((UINT8 *)FmpCapsuleHeader + ItemOffsetList[Index]);
    Print (L"  Version                - 0x%x\n", FmpImageHeader->Version);
    Print (L"  UpdateImageTypeId      - %g\n", &FmpImageHeader->UpdateImageTypeId);
    Print (L"  UpdateImageIndex       - 0x%x\n", FmpImageHeader->UpdateImageIndex);
    Print (L"  UpdateImageSize        - 0x%x\n", FmpImageHeader->UpdateImageSize);
    Print (L"  UpdateVendorCodeSize   - 0x%x\n", FmpImageHeader->UpdateVendorCodeSize);
    if (FmpImageHeader->Version >= 2) {
      Print (L"  UpdateHardwareInstance - 0x%lx\n", FmpImageHeader->UpdateHardwareInstance);
      if (FmpImageHeader->Version >= EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER_INIT_VERSION) {
        Print (L"  ImageCapsuleSupport    - 0x%lx\n", FmpImageHeader->ImageCapsuleSupport);
      }
    }
  }
}

/**
  Return if there is a FMP header below capsule header.

  @param[in] CapsuleHeader A pointer to EFI_CAPSULE_HEADER

  @retval TRUE  There is a FMP header below capsule header.
  @retval FALSE There is not a FMP header below capsule header
**/
BOOLEAN
IsNestedFmpCapsule (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  EFI_STATUS                 Status;
  EFI_SYSTEM_RESOURCE_TABLE  *Esrt;
  EFI_SYSTEM_RESOURCE_ENTRY  *EsrtEntry;
  UINTN                      Index;
  BOOLEAN                    EsrtGuidFound;
  EFI_CAPSULE_HEADER         *NestedCapsuleHeader;
  UINTN                      NestedCapsuleSize;

  //
  // Check ESRT
  //
  EsrtGuidFound = FALSE;
  Status        = EfiGetSystemConfigurationTable (&gEfiSystemResourceTableGuid, (VOID **)&Esrt);
  if (!EFI_ERROR (Status)) {
    ASSERT (Esrt != NULL);
    EsrtEntry = (VOID *)(Esrt + 1);
    for (Index = 0; Index < Esrt->FwResourceCount; Index++, EsrtEntry++) {
      if (CompareGuid (&EsrtEntry->FwClass, &CapsuleHeader->CapsuleGuid)) {
        EsrtGuidFound = TRUE;
        break;
      }
    }
  }

  if (!EsrtGuidFound) {
    return FALSE;
  }

  //
  // Check nested capsule header
  // FMP GUID after ESRT one
  //
  NestedCapsuleHeader = (EFI_CAPSULE_HEADER *)((UINT8 *)CapsuleHeader + CapsuleHeader->HeaderSize);
  NestedCapsuleSize   = (UINTN)CapsuleHeader + CapsuleHeader->CapsuleImageSize- (UINTN)NestedCapsuleHeader;
  if (NestedCapsuleSize < sizeof (EFI_CAPSULE_HEADER)) {
    return FALSE;
  }

  if (!CompareGuid (&NestedCapsuleHeader->CapsuleGuid, &gEfiFmpCapsuleGuid)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Dump capsule information

  @param[in] CapsuleName  The name of the capsule image.

  @retval EFI_SUCCESS            The capsule information is dumped.
  @retval EFI_UNSUPPORTED        Input parameter is not valid.
**/
EFI_STATUS
DumpCapsule (
  IN CHAR16  *CapsuleName
  )
{
  VOID                *Buffer;
  UINTN               FileSize;
  EFI_CAPSULE_HEADER  *CapsuleHeader;
  EFI_STATUS          Status;

  Buffer = NULL;
  Status = ReadFileToBuffer (CapsuleName, &FileSize, &Buffer);
  if (EFI_ERROR (Status)) {
    Print (L"CapsuleApp: Capsule (%s) is not found.\n", CapsuleName);
    goto Done;
  }

  if (!IsValidCapsuleHeader (Buffer, FileSize)) {
    Print (L"CapsuleApp: Capsule image (%s) is not a valid capsule.\n", CapsuleName);
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  CapsuleHeader = Buffer;
  if (CompareGuid (&CapsuleHeader->CapsuleGuid, &gWindowsUxCapsuleGuid)) {
    DumpUxCapsule (CapsuleHeader);
    Status = EFI_SUCCESS;
    goto Done;
  }

  if (CompareGuid (&CapsuleHeader->CapsuleGuid, &gEfiFmpCapsuleGuid)) {
    DumpFmpCapsule (CapsuleHeader);
  }

  if (IsNestedFmpCapsule (CapsuleHeader)) {
    Print (L"[NestedCapsule]\n");
    Print (L"CapsuleHeader:\n");
    Print (L"  CapsuleGuid      - %g\n", &CapsuleHeader->CapsuleGuid);
    Print (L"  HeaderSize       - 0x%x\n", CapsuleHeader->HeaderSize);
    Print (L"  Flags            - 0x%x\n", CapsuleHeader->Flags);
    Print (L"  CapsuleImageSize - 0x%x\n", CapsuleHeader->CapsuleImageSize);
    DumpFmpCapsule ((EFI_CAPSULE_HEADER *)((UINTN)CapsuleHeader + CapsuleHeader->HeaderSize));
  }

Done:
  if (Buffer != NULL) {
    FreePool (Buffer);
  }

  return Status;
}

/**
  Dump capsule status variable.

  @retval EFI_SUCCESS            The capsule status variable is dumped.
  @retval EFI_UNSUPPORTED        Input parameter is not valid.
**/
EFI_STATUS
DumpCapsuleStatusVariable (
  VOID
  )
{
  EFI_STATUS                          Status;
  UINT32                              Index;
  CHAR16                              CapsuleVarName[20];
  CHAR16                              *TempVarName;
  EFI_CAPSULE_RESULT_VARIABLE_HEADER  *CapsuleResult;
  EFI_CAPSULE_RESULT_VARIABLE_FMP     *CapsuleResultFmp;
  UINTN                               CapsuleFileNameSize;
  CHAR16                              CapsuleIndexData[12];
  CHAR16                              *CapsuleIndex;
  CHAR16                              *CapsuleFileName;
  CHAR16                              *CapsuleTarget;

  Status = GetVariable2 (
             L"CapsuleMax",
             &gEfiCapsuleReportGuid,
             (VOID **)&CapsuleIndex,
             NULL
             );
  if (!EFI_ERROR (Status)) {
    ASSERT (CapsuleIndex != NULL);
    CopyMem (CapsuleIndexData, CapsuleIndex, 11 * sizeof (CHAR16));
    CapsuleIndexData[11] = 0;
    Print (L"CapsuleMax - %s\n", CapsuleIndexData);
    FreePool (CapsuleIndex);
  }

  Status = GetVariable2 (
             L"CapsuleLast",
             &gEfiCapsuleReportGuid,
             (VOID **)&CapsuleIndex,
             NULL
             );
  if (!EFI_ERROR (Status)) {
    ASSERT (CapsuleIndex != NULL);
    CopyMem (CapsuleIndexData, CapsuleIndex, 11 * sizeof (CHAR16));
    CapsuleIndexData[11] = 0;
    Print (L"CapsuleLast - %s\n", CapsuleIndexData);
    FreePool (CapsuleIndex);
  }

  StrCpyS (CapsuleVarName, sizeof (CapsuleVarName)/sizeof (CapsuleVarName[0]), L"Capsule");
  TempVarName = CapsuleVarName + StrLen (CapsuleVarName);
  Index       = 0;

  while (TRUE) {
    UnicodeSPrint (TempVarName, 5 * sizeof (CHAR16), L"%04x", Index);

    Status = GetVariable2 (
               CapsuleVarName,
               &gEfiCapsuleReportGuid,
               (VOID **)&CapsuleResult,
               NULL
               );
    if (Status == EFI_NOT_FOUND) {
      break;
    } else if (EFI_ERROR (Status)) {
      continue;
    }

    ASSERT (CapsuleResult != NULL);

    //
    // display capsule process status
    //
    if (CapsuleResult->VariableTotalSize >= sizeof (EFI_CAPSULE_RESULT_VARIABLE_HEADER)) {
      Print (L"CapsuleName: %s\n", CapsuleVarName);
      Print (L"  Capsule Guid: %g\n", &CapsuleResult->CapsuleGuid);
      Print (L"  Capsule ProcessedTime: %t\n", &CapsuleResult->CapsuleProcessed);
      Print (L"  Capsule Status: %r\n", CapsuleResult->CapsuleStatus);
    }

    if (CompareGuid (&CapsuleResult->CapsuleGuid, &gEfiFmpCapsuleGuid)) {
      if (CapsuleResult->VariableTotalSize >= sizeof (EFI_CAPSULE_RESULT_VARIABLE_HEADER) + sizeof (EFI_CAPSULE_RESULT_VARIABLE_FMP) + sizeof (CHAR16) * 2) {
        CapsuleResultFmp = (EFI_CAPSULE_RESULT_VARIABLE_FMP *)(CapsuleResult + 1);
        Print (L"  Capsule FMP Version: 0x%x\n", CapsuleResultFmp->Version);
        Print (L"  Capsule FMP PayloadIndex: 0x%x\n", CapsuleResultFmp->PayloadIndex);
        Print (L"  Capsule FMP UpdateImageIndex: 0x%x\n", CapsuleResultFmp->UpdateImageIndex);
        Print (L"  Capsule FMP UpdateImageTypeId: %g\n", &CapsuleResultFmp->UpdateImageTypeId);
        CapsuleFileName = (CHAR16 *)(CapsuleResultFmp + 1);
        Print (L"  Capsule FMP CapsuleFileName: \"%s\"\n", CapsuleFileName);
        CapsuleFileNameSize = StrSize (CapsuleFileName);
        CapsuleTarget       = (CHAR16 *)((UINTN)CapsuleFileName + CapsuleFileNameSize);
        Print (L"  Capsule FMP CapsuleTarget: \"%s\"\n", CapsuleTarget);
      }
    }

    FreePool (CapsuleResult);

    Index++;
    if (Index > 0xFFFF) {
      break;
    }
  }

  return EFI_SUCCESS;
}

CHAR8  *mFwTypeString[] = {
  "Unknown",
  "SystemFirmware",
  "DeviceFirmware",
  "UefiDriver",
};

CHAR8  *mLastAttemptStatusString[] = {
  "Success",
  "Error: Unsuccessful",
  "Error: Insufficient Resources",
  "Error: Incorrect Version",
  "Error: Invalid Format",
  "Error: Auth Error",
  "Error: Power Event AC",
  "Error: Power Event Battery",
  "Error: Unsatisfied Dependencies",
};

/**
  Convert FwType to a string.

  @param[in] FwType  FwType in ESRT

  @return a string for FwType.
**/
CHAR8 *
FwTypeToString (
  IN UINT32  FwType
  )
{
  if (FwType < sizeof (mFwTypeString) / sizeof (mFwTypeString[0])) {
    return mFwTypeString[FwType];
  } else {
    return "Invalid";
  }
}

/**
  Convert LastAttemptStatus to a string.

  @param[in] LastAttemptStatus  LastAttemptStatus in FMP or ESRT

  @return a string for LastAttemptStatus.
**/
CHAR8 *
LastAttemptStatusToString (
  IN UINT32  LastAttemptStatus
  )
{
  if (LastAttemptStatus < sizeof (mLastAttemptStatusString) / sizeof (mLastAttemptStatusString[0])) {
    return mLastAttemptStatusString[LastAttemptStatus];
  } else {
    return "Error: Unknown";
  }
}

/**
  Dump ESRT entry.

  @param[in] EsrtEntry  ESRT entry
**/
VOID
DumpEsrtEntry (
  IN EFI_SYSTEM_RESOURCE_ENTRY  *EsrtEntry
  )
{
  Print (L"  FwClass                  - %g\n", &EsrtEntry->FwClass);
  Print (L"  FwType                   - 0x%x (%a)\n", EsrtEntry->FwType, FwTypeToString (EsrtEntry->FwType));
  Print (L"  FwVersion                - 0x%x\n", EsrtEntry->FwVersion);
  Print (L"  LowestSupportedFwVersion - 0x%x\n", EsrtEntry->LowestSupportedFwVersion);
  Print (L"  CapsuleFlags             - 0x%x\n", EsrtEntry->CapsuleFlags);
  Print (L"  LastAttemptVersion       - 0x%x\n", EsrtEntry->LastAttemptVersion);
  Print (L"  LastAttemptStatus        - 0x%x (%a)\n", EsrtEntry->LastAttemptStatus, LastAttemptStatusToString (EsrtEntry->LastAttemptStatus));
}

/**
  Dump ESRT table.

  @param[in] Esrt  ESRT table
**/
VOID
DumpEsrt (
  IN EFI_SYSTEM_RESOURCE_TABLE  *Esrt
  )
{
  UINTN                      Index;
  EFI_SYSTEM_RESOURCE_ENTRY  *EsrtEntry;

  if (Esrt == NULL) {
    return;
  }

  Print (L"EFI_SYSTEM_RESOURCE_TABLE:\n");
  Print (L"FwResourceCount    - 0x%x\n", Esrt->FwResourceCount);
  Print (L"FwResourceCountMax - 0x%x\n", Esrt->FwResourceCountMax);
  Print (L"FwResourceVersion  - 0x%lx\n", Esrt->FwResourceVersion);

  EsrtEntry = (VOID *)(Esrt + 1);
  for (Index = 0; Index < Esrt->FwResourceCount; Index++) {
    Print (L"EFI_SYSTEM_RESOURCE_ENTRY (%d):\n", Index);
    DumpEsrtEntry (EsrtEntry);
    EsrtEntry++;
  }
}

/**
  Dump ESRT info.
**/
VOID
DumpEsrtData (
  VOID
  )
{
  EFI_STATUS                 Status;
  EFI_SYSTEM_RESOURCE_TABLE  *Esrt;

  Print (L"##############\n");
  Print (L"# ESRT TABLE #\n");
  Print (L"##############\n");

  Status = EfiGetSystemConfigurationTable (&gEfiSystemResourceTableGuid, (VOID **)&Esrt);
  if (EFI_ERROR (Status)) {
    Print (L"ESRT - %r\n", Status);
    return;
  }

  DumpEsrt (Esrt);
  Print (L"\n");
}

/**
  Dump capsule information from CapsuleHeader

  @param[in] CapsuleHeader       The CapsuleHeader of the capsule image.

  @retval EFI_SUCCESS            The capsule information is dumped.

**/
EFI_STATUS
DumpCapsuleFromBuffer (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  if (CompareGuid (&CapsuleHeader->CapsuleGuid, &gWindowsUxCapsuleGuid)) {
    DumpUxCapsule (CapsuleHeader);
    return EFI_SUCCESS;
  }

  if (CompareGuid (&CapsuleHeader->CapsuleGuid, &gEfiFmpCapsuleGuid)) {
    DumpFmpCapsule (CapsuleHeader);
  }

  if (IsNestedFmpCapsule (CapsuleHeader)) {
    Print (L"[NestedCapusule]\n");
    Print (L"CapsuleHeader:\n");
    Print (L"  CapsuleGuid      - %g\n", &CapsuleHeader->CapsuleGuid);
    Print (L"  HeaderSize       - 0x%x\n", CapsuleHeader->HeaderSize);
    Print (L"  Flags            - 0x%x\n", CapsuleHeader->Flags);
    Print (L"  CapsuleImageSize - 0x%x\n", CapsuleHeader->CapsuleImageSize);
    DumpFmpCapsule ((EFI_CAPSULE_HEADER *)((UINTN)CapsuleHeader + CapsuleHeader->HeaderSize));
  }

  return EFI_SUCCESS;
}

/**
  This routine is called to upper case given unicode string.

  @param[in]   Str              String to upper case

  @retval upper cased string after process

**/
STATIC
CHAR16 *
UpperCaseString (
  IN CHAR16  *Str
  )
{
  CHAR16  *Cptr;

  for (Cptr = Str; *Cptr != L'\0'; Cptr++) {
    if ((L'a' <= *Cptr) && (*Cptr <= L'z')) {
      *Cptr = *Cptr - L'a' + L'A';
    }
  }

  return Str;
}

/**
  This routine is used to return substring before period '.' or '\0'
  Caller should respsonsible of substr space allocation & free

  @param[in]   Str              String to check
  @param[out]  SubStr           First part of string before period or '\0'
  @param[out]  SubStrLen        Length of first part of string

**/
STATIC
VOID
GetSubStringBeforePeriod (
  IN  CHAR16  *Str,
  OUT CHAR16  *SubStr,
  OUT UINTN   *SubStrLen
  )
{
  UINTN  Index;

  for (Index = 0; Str[Index] != L'.' && Str[Index] != L'\0'; Index++) {
    SubStr[Index] = Str[Index];
  }

  SubStr[Index] = L'\0';
  *SubStrLen    = Index;
}

/**
  This routine pad the string in tail with input character.

  @param[in]   StrBuf            Str buffer to be padded, should be enough room for
  @param[in]   PadLen            Expected padding length
  @param[in]   Character         Character used to pad

**/
STATIC
VOID
PadStrInTail (
  IN CHAR16  *StrBuf,
  IN UINTN   PadLen,
  IN CHAR16  Character
  )
{
  UINTN  Index;

  for (Index = 0; StrBuf[Index] != L'\0'; Index++) {
  }

  while (PadLen != 0) {
    StrBuf[Index] = Character;
    Index++;
    PadLen--;
  }

  StrBuf[Index] = L'\0';
}

/**
  This routine find the offset of the last period '.' of string. if No period exists
  function FileNameExtension is set to L'\0'

  @param[in]   FileName           File name to split between last period
  @param[out]  FileNameFirst      First FileName before last period
  @param[out]  FileNameExtension  FileName after last period

**/
STATIC
VOID
SplitFileNameExtension (
  IN  CHAR16  *FileName,
  OUT CHAR16  *FileNameFirst,
  OUT CHAR16  *FileNameExtension
  )
{
  UINTN  Index;
  UINTN  StringLen;

  StringLen = StrLen (FileName);
  for (Index = StringLen; Index > 0 && FileName[Index] != L'.'; Index--) {
  }

  //
  // No period exists. No FileName Extension
  //
  if ((Index == 0) && (FileName[Index] != L'.')) {
    FileNameExtension[0] = L'\0';
    Index                = StringLen;
  } else {
    StrCpyS (FileNameExtension, MAX_FILE_NAME_LEN, &FileName[Index+1]);
  }

  //
  // Copy First file name
  //
  StrnCpyS (FileNameFirst, MAX_FILE_NAME_LEN, FileName, Index);
  FileNameFirst[Index] = L'\0';
}

/**
  The function is called by PerformQuickSort to sort file name in alphabet.

  @param[in] Left            The pointer to first buffer.
  @param[in] Right           The pointer to second buffer.

  @retval 0                  Buffer1 equal to Buffer2.
  @return <0                 Buffer1 is less than Buffer2.
  @return >0                 Buffer1 is greater than Buffer2.

**/
INTN
EFIAPI
CompareFileNameInAlphabet (
  IN VOID  *Left,
  IN VOID  *Right
  )
{
  EFI_FILE_INFO  *FileInfo1;
  EFI_FILE_INFO  *FileInfo2;
  CHAR16         FileName1[MAX_FILE_NAME_SIZE];
  CHAR16         FileExtension1[MAX_FILE_NAME_SIZE];
  CHAR16         FileName2[MAX_FILE_NAME_SIZE];
  CHAR16         FileExtension2[MAX_FILE_NAME_SIZE];
  CHAR16         TempSubStr1[MAX_FILE_NAME_SIZE];
  CHAR16         TempSubStr2[MAX_FILE_NAME_SIZE];
  UINTN          SubStrLen1;
  UINTN          SubStrLen2;
  INTN           SubStrCmpResult;

  FileInfo1 = (EFI_FILE_INFO *)(*(UINTN *)Left);
  FileInfo2 = (EFI_FILE_INFO *)(*(UINTN *)Right);

  SplitFileNameExtension (FileInfo1->FileName, FileName1, FileExtension1);
  SplitFileNameExtension (FileInfo2->FileName, FileName2, FileExtension2);

  UpperCaseString (FileName1);
  UpperCaseString (FileName2);

  GetSubStringBeforePeriod (FileName1, TempSubStr1, &SubStrLen1);
  GetSubStringBeforePeriod (FileName2, TempSubStr2, &SubStrLen2);

  if (SubStrLen1 > SubStrLen2) {
    //
    // Substr in NewFileName is longer.  Pad tail with SPACE
    //
    PadStrInTail (TempSubStr2, SubStrLen1 - SubStrLen2, L' ');
  } else if (SubStrLen1 < SubStrLen2) {
    //
    // Substr in ListedFileName is longer. Pad tail with SPACE
    //
    PadStrInTail (TempSubStr1, SubStrLen2 - SubStrLen1, L' ');
  }

  SubStrCmpResult = StrnCmp (TempSubStr1, TempSubStr2, MAX_FILE_NAME_LEN);
  if (SubStrCmpResult != 0) {
    return SubStrCmpResult;
  }

  UpperCaseString (FileExtension1);
  UpperCaseString (FileExtension2);

  return StrnCmp (FileExtension1, FileExtension2, MAX_FILE_NAME_LEN);
}

/**
  Dump capsule information from disk.

  @param[in] Fs                  The device path of disk.
  @param[in] DumpCapsuleInfo     The flag to indicate whether to dump the capsule inforomation.

  @retval EFI_SUCCESS            The capsule information is dumped.

**/
EFI_STATUS
DumpCapsuleFromDisk (
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Fs,
  IN BOOLEAN                          DumpCapsuleInfo
  )
{
  EFI_STATUS     Status;
  EFI_FILE       *Root;
  EFI_FILE       *DirHandle;
  EFI_FILE       *FileHandle;
  UINTN          Index;
  UINTN          FileSize;
  VOID           *FileBuffer;
  EFI_FILE_INFO  **FileInfoBuffer;
  EFI_FILE_INFO  *FileInfo;
  UINTN          FileCount;
  BOOLEAN        NoFile;

  DirHandle      = NULL;
  FileHandle     = NULL;
  Index          = 0;
  FileInfoBuffer = NULL;
  FileInfo       = NULL;
  FileCount      = 0;
  NoFile         = FALSE;

  Status = Fs->OpenVolume (Fs, &Root);
  if (EFI_ERROR (Status)) {
    Print (L"Cannot open volume. Status = %r\n", Status);
    goto Done;
  }

  Status = Root->Open (Root, &DirHandle, EFI_CAPSULE_FILE_DIRECTORY, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
  if (EFI_ERROR (Status)) {
    Print (L"Cannot open %s. Status = %r\n", EFI_CAPSULE_FILE_DIRECTORY, Status);
    goto Done;
  }

  //
  // Get file count first
  //
  Status = FileHandleFindFirstFile (DirHandle, &FileInfo);
  do {
    if (EFI_ERROR (Status) || (FileInfo == NULL)) {
      Print (L"Get File Info Fail. Status = %r\n", Status);
      goto Done;
    }

    if ((FileInfo->Attribute & (EFI_FILE_SYSTEM | EFI_FILE_ARCHIVE)) != 0) {
      FileCount++;
    }

    Status = FileHandleFindNextFile (DirHandle, FileInfo, &NoFile);
    if (EFI_ERROR (Status)) {
      Print (L"Get Next File Fail. Status = %r\n", Status);
      goto Done;
    }
  } while (!NoFile);

  if (FileCount == 0) {
    Print (L"Error: No capsule file found!\n");
    Status = EFI_NOT_FOUND;
    goto Done;
  }

  FileInfoBuffer = AllocateZeroPool (sizeof (FileInfo) * FileCount);
  if (FileInfoBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  NoFile = FALSE;

  //
  // Get all file info
  //
  Status = FileHandleFindFirstFile (DirHandle, &FileInfo);
  do {
    if (EFI_ERROR (Status) || (FileInfo == NULL)) {
      Print (L"Get File Info Fail. Status = %r\n", Status);
      goto Done;
    }

    if ((FileInfo->Attribute & (EFI_FILE_SYSTEM | EFI_FILE_ARCHIVE)) != 0) {
      FileInfoBuffer[Index++] = AllocateCopyPool ((UINTN)FileInfo->Size, FileInfo);
    }

    Status = FileHandleFindNextFile (DirHandle, FileInfo, &NoFile);
    if (EFI_ERROR (Status)) {
      Print (L"Get Next File Fail. Status = %r\n", Status);
      goto Done;
    }
  } while (!NoFile);

  //
  // Sort FileInfoBuffer by alphabet order
  //
  PerformQuickSort (
    FileInfoBuffer,
    FileCount,
    sizeof (FileInfo),
    (SORT_COMPARE)CompareFileNameInAlphabet
    );

  Print (L"The capsules will be performed by following order:\n");

  for (Index = 0; Index < FileCount; Index++) {
    Print (L"  %d.%s\n", Index + 1, FileInfoBuffer[Index]->FileName);
  }

  if (!DumpCapsuleInfo) {
    Status = EFI_SUCCESS;
    goto Done;
  }

  Print (L"The infomation of the capsules:\n");

  for (Index = 0; Index < FileCount; Index++) {
    FileHandle = NULL;
    Status     = DirHandle->Open (DirHandle, &FileHandle, FileInfoBuffer[Index]->FileName, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    Status = FileHandleGetSize (FileHandle, (UINT64 *)&FileSize);
    if (EFI_ERROR (Status)) {
      Print (L"Cannot read file %s. Status = %r\n", FileInfoBuffer[Index]->FileName, Status);
      FileHandleClose (FileHandle);
      goto Done;
    }

    FileBuffer = AllocatePool (FileSize);
    if (FileBuffer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    Status = FileHandleRead (FileHandle, &FileSize, FileBuffer);
    if (EFI_ERROR (Status)) {
      Print (L"Cannot read file %s. Status = %r\n", FileInfoBuffer[Index]->FileName, Status);
      FileHandleClose (FileHandle);
      FreePool (FileBuffer);
      goto Done;
    }

    Print (L"**************************\n");
    Print (L"  %d.%s:\n", Index + 1, FileInfoBuffer[Index]->FileName);
    Print (L"**************************\n");
    DumpCapsuleFromBuffer ((EFI_CAPSULE_HEADER *)FileBuffer);
    FileHandleClose (FileHandle);
    FreePool (FileBuffer);
  }

Done:
  if (FileInfoBuffer != NULL) {
    for (Index = 0; Index < FileCount; Index++) {
      if (FileInfoBuffer[Index] != NULL) {
        FreePool (FileInfoBuffer[Index]);
      }
    }

    FreePool (FileInfoBuffer);
  }

  return Status;
}

/**
  Dump capsule inforomation form Gather list.

  @param[in]  BlockDescriptors The block descriptors for the capsule images
  @param[in]  DumpCapsuleInfo  The flag to indicate whether to dump the capsule inforomation.

**/
VOID
DumpBlockDescriptors (
  IN EFI_CAPSULE_BLOCK_DESCRIPTOR  *BlockDescriptors,
  IN BOOLEAN                       DumpCapsuleInfo
  )
{
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *TempBlockPtr;

  TempBlockPtr = BlockDescriptors;

  while (TRUE) {
    if (TempBlockPtr->Length != 0) {
      if (DumpCapsuleInfo) {
        Print (L"******************************************************\n");
      }

      Print (L"Capsule data starts at 0x%08x with size 0x%08x\n", TempBlockPtr->Union.DataBlock, TempBlockPtr->Length);
      if (DumpCapsuleInfo) {
        Print (L"******************************************************\n");
        DumpCapsuleFromBuffer ((EFI_CAPSULE_HEADER *)(UINTN)TempBlockPtr->Union.DataBlock);
      }

      TempBlockPtr += 1;
    } else {
      if (TempBlockPtr->Union.ContinuationPointer == (UINTN)NULL) {
        break;
      } else {
        TempBlockPtr = (EFI_CAPSULE_BLOCK_DESCRIPTOR *)(UINTN)TempBlockPtr->Union.ContinuationPointer;
      }
    }
  }
}

/**
  Dump Provisioned Capsule.

  @param[in]  DumpCapsuleInfo  The flag to indicate whether to dump the capsule inforomation.

**/
VOID
DumpProvisionedCapsule (
  IN BOOLEAN  DumpCapsuleInfo
  )
{
  EFI_STATUS                       Status;
  CHAR16                           CapsuleVarName[30];
  CHAR16                           *TempVarName;
  UINTN                            Index;
  EFI_PHYSICAL_ADDRESS             *CapsuleDataPtr64;
  UINT16                           *BootNext;
  CHAR16                           BootOptionName[20];
  EFI_BOOT_MANAGER_LOAD_OPTION     BootNextOptionEntry;
  EFI_DEVICE_PATH_PROTOCOL         *DevicePath;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Fs;
  EFI_SHELL_PROTOCOL               *ShellProtocol;

  Index            = 0;
  CapsuleDataPtr64 = NULL;
  BootNext         = NULL;

  ShellProtocol = GetShellProtocol ();
  if (ShellProtocol == NULL) {
    Print (L"Get Shell Protocol Fail\n");
    return;
  }

  //
  // Dump capsule provisioned on Memory
  //
  Print (L"#########################\n");
  Print (L"### Capsule on Memory ###\n");
  Print (L"#########################\n");
  StrCpyS (CapsuleVarName, sizeof (CapsuleVarName)/sizeof (CHAR16), EFI_CAPSULE_VARIABLE_NAME);
  TempVarName = CapsuleVarName + StrLen (CapsuleVarName);
  while (TRUE) {
    if (Index > 0) {
      UnicodeValueToStringS (
        TempVarName,
        sizeof (CapsuleVarName) - ((UINTN)TempVarName - (UINTN)CapsuleVarName),
        0,
        Index,
        0
        );
    }

    Status = GetVariable2 (
               CapsuleVarName,
               &gEfiCapsuleVendorGuid,
               (VOID **)&CapsuleDataPtr64,
               NULL
               );
    if (EFI_ERROR (Status) || (CapsuleDataPtr64 == NULL)) {
      if (Index == 0) {
        Print (L"No data.\n");
      }

      break;
    }

    Index++;
    Print (L"Capsule Description at 0x%08x\n", *CapsuleDataPtr64);
    DumpBlockDescriptors ((EFI_CAPSULE_BLOCK_DESCRIPTOR *)(UINTN)*CapsuleDataPtr64, DumpCapsuleInfo);
  }

  //
  // Dump capsule provisioned on Disk
  //
  Print (L"#########################\n");
  Print (L"### Capsule on Disk #####\n");
  Print (L"#########################\n");
  Status = GetVariable2 (
             L"BootNext",
             &gEfiGlobalVariableGuid,
             (VOID **)&BootNext,
             NULL
             );
  if (EFI_ERROR (Status) || (BootNext == NULL)) {
    Print (L"Get BootNext Variable Fail. Status = %r\n", Status);
  } else {
    UnicodeSPrint (BootOptionName, sizeof (BootOptionName), L"Boot%04x", *BootNext);
    Status = EfiBootManagerVariableToLoadOption (BootOptionName, &BootNextOptionEntry);
    if (!EFI_ERROR (Status)) {
      //
      // Display description and device path
      //
      GetEfiSysPartitionFromBootOptionFilePath (BootNextOptionEntry.FilePath, &DevicePath, &Fs);
      if (!EFI_ERROR (Status)) {
        Print (L"Capsules are provisioned on BootOption: %s\n", BootNextOptionEntry.Description);
        Print (L"    %s %s\n", ShellProtocol->GetMapFromDevicePath (&DevicePath), ConvertDevicePathToText (DevicePath, TRUE, TRUE));
        DumpCapsuleFromDisk (Fs, DumpCapsuleInfo);
      }
    }
  }
}

/**
  Dump FMP information.

  @param[in] ImageInfoSize       The size of ImageInfo, in bytes.
  @param[in] ImageInfo           A pointer to EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[in] DescriptorVersion   The version of EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[in] DescriptorCount     The count of EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[in] DescriptorSize      The size of an individual EFI_FIRMWARE_IMAGE_DESCRIPTOR, in bytes.
  @param[in] PackageVersion      The version of package.
  @param[in] PackageVersionName  The version name of package.
**/
VOID
DumpFmpImageInfo (
  IN UINTN                          ImageInfoSize,
  IN EFI_FIRMWARE_IMAGE_DESCRIPTOR  *ImageInfo,
  IN UINT32                         DescriptorVersion,
  IN UINT8                          DescriptorCount,
  IN UINTN                          DescriptorSize,
  IN UINT32                         PackageVersion,
  IN CHAR16                         *PackageVersionName
  )
{
  EFI_FIRMWARE_IMAGE_DESCRIPTOR  *CurrentImageInfo;
  UINTN                          Index;
  UINTN                          Index2;

  Print (L"  DescriptorVersion  - 0x%x\n", DescriptorVersion);
  Print (L"  DescriptorCount    - 0x%x\n", DescriptorCount);
  Print (L"  DescriptorSize     - 0x%x\n", DescriptorSize);
  Print (L"  PackageVersion     - 0x%x\n", PackageVersion);
  Print (L"  PackageVersionName - \"%s\"\n", PackageVersionName);
  CurrentImageInfo = ImageInfo;
  for (Index = 0; Index < DescriptorCount; Index++) {
    Print (L"  ImageDescriptor (%d)\n", Index);
    Print (L"    ImageIndex                  - 0x%x\n", CurrentImageInfo->ImageIndex);
    Print (L"    ImageTypeId                 - %g\n", &CurrentImageInfo->ImageTypeId);
    Print (L"    ImageId                     - 0x%lx\n", CurrentImageInfo->ImageId);
    Print (L"    ImageIdName                 - \"%s\"\n", CurrentImageInfo->ImageIdName);
    Print (L"    Version                     - 0x%x\n", CurrentImageInfo->Version);
    Print (L"    VersionName                 - \"%s\"\n", CurrentImageInfo->VersionName);
    Print (L"    Size                        - 0x%x\n", CurrentImageInfo->Size);
    Print (L"    AttributesSupported         - 0x%lx\n", CurrentImageInfo->AttributesSupported);
    Print (L"      IMAGE_UPDATABLE           - 0x%lx\n", CurrentImageInfo->AttributesSupported & IMAGE_ATTRIBUTE_IMAGE_UPDATABLE);
    Print (L"      RESET_REQUIRED            - 0x%lx\n", CurrentImageInfo->AttributesSupported & IMAGE_ATTRIBUTE_RESET_REQUIRED);
    Print (L"      AUTHENTICATION_REQUIRED   - 0x%lx\n", CurrentImageInfo->AttributesSupported & IMAGE_ATTRIBUTE_AUTHENTICATION_REQUIRED);
    Print (L"      IN_USE                    - 0x%lx\n", CurrentImageInfo->AttributesSupported & IMAGE_ATTRIBUTE_IN_USE);
    Print (L"      UEFI_IMAGE                - 0x%lx\n", CurrentImageInfo->AttributesSupported & IMAGE_ATTRIBUTE_UEFI_IMAGE);
    Print (L"    AttributesSetting           - 0x%lx\n", CurrentImageInfo->AttributesSetting);
    Print (L"      IMAGE_UPDATABLE           - 0x%lx\n", CurrentImageInfo->AttributesSetting & IMAGE_ATTRIBUTE_IMAGE_UPDATABLE);
    Print (L"      RESET_REQUIRED            - 0x%lx\n", CurrentImageInfo->AttributesSetting & IMAGE_ATTRIBUTE_RESET_REQUIRED);
    Print (L"      AUTHENTICATION_REQUIRED   - 0x%lx\n", CurrentImageInfo->AttributesSetting & IMAGE_ATTRIBUTE_AUTHENTICATION_REQUIRED);
    Print (L"      IN_USE                    - 0x%lx\n", CurrentImageInfo->AttributesSetting & IMAGE_ATTRIBUTE_IN_USE);
    Print (L"      UEFI_IMAGE                - 0x%lx\n", CurrentImageInfo->AttributesSetting & IMAGE_ATTRIBUTE_UEFI_IMAGE);
    Print (L"    Compatibilities             - 0x%lx\n", CurrentImageInfo->Compatibilities);
    Print (L"      COMPATIB_CHECK_SUPPORTED  - 0x%lx\n", CurrentImageInfo->Compatibilities & IMAGE_COMPATIBILITY_CHECK_SUPPORTED);
    if (DescriptorVersion > 1) {
      Print (L"    LowestSupportedImageVersion - 0x%x\n", CurrentImageInfo->LowestSupportedImageVersion);
      if (DescriptorVersion > 2) {
        Print (L"    LastAttemptVersion          - 0x%x\n", CurrentImageInfo->LastAttemptVersion);
        Print (L"    LastAttemptStatus           - 0x%x (%a)\n", CurrentImageInfo->LastAttemptStatus, LastAttemptStatusToString (CurrentImageInfo->LastAttemptStatus));
        Print (L"    HardwareInstance            - 0x%lx\n", CurrentImageInfo->HardwareInstance);
        if (DescriptorVersion > 3) {
          Print (L"    Dependencies                - ");
          if (CurrentImageInfo->Dependencies == NULL) {
            Print (L"NULL\n");
          } else {
            Index2 = 0;
            do {
              Print (L"%02x ", CurrentImageInfo->Dependencies->Dependencies[Index2]);
            } while (CurrentImageInfo->Dependencies->Dependencies[Index2++] != EFI_FMP_DEP_END);

            Print (L"\n");
          }
        }
      }
    }

    //
    // Use DescriptorSize to move ImageInfo Pointer to stay compatible with different ImageInfo version
    //
    CurrentImageInfo = (EFI_FIRMWARE_IMAGE_DESCRIPTOR *)((UINT8 *)CurrentImageInfo + DescriptorSize);
  }
}

/**
  Dump FMP package information.

  @param[in] PackageVersion             The version of package.
  @param[in] PackageVersionName         The version name of package.
  @param[in] PackageVersionNameMaxLen   The maximum length of PackageVersionName.
  @param[in] AttributesSupported        Package attributes that are supported by this device.
  @param[in] AttributesSetting          Package attributes.
**/
VOID
DumpFmpPackageInfo (
  IN UINT32  PackageVersion,
  IN CHAR16  *PackageVersionName,
  IN UINT32  PackageVersionNameMaxLen,
  IN UINT64  AttributesSupported,
  IN UINT64  AttributesSetting
  )
{
  Print (L"  PackageVersion              - 0x%x\n", PackageVersion);
  Print (L"  PackageVersionName          - \"%s\"\n", PackageVersionName);
  Print (L"  PackageVersionNameMaxLen    - 0x%x\n", PackageVersionNameMaxLen);
  Print (L"  AttributesSupported         - 0x%lx\n", AttributesSupported);
  Print (L"    IMAGE_UPDATABLE           - 0x%lx\n", AttributesSupported & IMAGE_ATTRIBUTE_IMAGE_UPDATABLE);
  Print (L"    RESET_REQUIRED            - 0x%lx\n", AttributesSupported & IMAGE_ATTRIBUTE_RESET_REQUIRED);
  Print (L"    AUTHENTICATION_REQUIRED   - 0x%lx\n", AttributesSupported & IMAGE_ATTRIBUTE_AUTHENTICATION_REQUIRED);
  Print (L"  AttributesSetting           - 0x%lx\n", AttributesSetting);
  Print (L"    IMAGE_UPDATABLE           - 0x%lx\n", AttributesSetting & IMAGE_ATTRIBUTE_IMAGE_UPDATABLE);
  Print (L"    RESET_REQUIRED            - 0x%lx\n", AttributesSetting & IMAGE_ATTRIBUTE_RESET_REQUIRED);
  Print (L"    AUTHENTICATION_REQUIRED   - 0x%lx\n", AttributesSetting & IMAGE_ATTRIBUTE_AUTHENTICATION_REQUIRED);
}

/**
  Dump FMP protocol info.
**/
VOID
DumpFmpData (
  VOID
  )
{
  EFI_STATUS                        Status;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *Fmp;
  EFI_HANDLE                        *HandleBuffer;
  UINTN                             NumberOfHandles;
  UINTN                             Index;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR     *FmpImageInfoBuf;
  UINTN                             ImageInfoSize;
  UINT32                            FmpImageInfoDescriptorVer;
  UINT8                             FmpImageInfoCount;
  UINTN                             DescriptorSize;
  UINT32                            PackageVersion;
  CHAR16                            *PackageVersionName;
  UINT32                            PackageVersionNameMaxLen;
  UINT64                            AttributesSupported;
  UINT64                            AttributesSetting;

  Print (L"############\n");
  Print (L"# FMP DATA #\n");
  Print (L"############\n");
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareManagementProtocolGuid,
                  NULL,
                  &NumberOfHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    Print (L"FMP protocol - %r\n", EFI_NOT_FOUND);
    return;
  }

  for (Index = 0; Index < NumberOfHandles; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiFirmwareManagementProtocolGuid,
                    (VOID **)&Fmp
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    ImageInfoSize = 0;
    Status        = Fmp->GetImageInfo (
                           Fmp,
                           &ImageInfoSize,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL
                           );
    if (Status != EFI_BUFFER_TOO_SMALL) {
      continue;
    }

    FmpImageInfoBuf = NULL;
    FmpImageInfoBuf = AllocateZeroPool (ImageInfoSize);
    if (FmpImageInfoBuf == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto EXIT;
    }

    PackageVersionName = NULL;
    Status             = Fmp->GetImageInfo (
                                Fmp,
                                &ImageInfoSize,             // ImageInfoSize
                                FmpImageInfoBuf,            // ImageInfo
                                &FmpImageInfoDescriptorVer, // DescriptorVersion
                                &FmpImageInfoCount,         // DescriptorCount
                                &DescriptorSize,            // DescriptorSize
                                &PackageVersion,            // PackageVersion
                                &PackageVersionName         // PackageVersionName
                                );

    //
    // If FMP GetInformation interface failed, skip this resource
    //
    if (EFI_ERROR (Status)) {
      Print (L"FMP (%d) ImageInfo - %r\n", Index, Status);
      FreePool (FmpImageInfoBuf);
      continue;
    }

    Print (L"FMP (%d) ImageInfo:\n", Index);
    DumpFmpImageInfo (
      ImageInfoSize,               // ImageInfoSize
      FmpImageInfoBuf,             // ImageInfo
      FmpImageInfoDescriptorVer,   // DescriptorVersion
      FmpImageInfoCount,           // DescriptorCount
      DescriptorSize,              // DescriptorSize
      PackageVersion,              // PackageVersion
      PackageVersionName           // PackageVersionName
      );

    if (PackageVersionName != NULL) {
      FreePool (PackageVersionName);
    }

    FreePool (FmpImageInfoBuf);

    //
    // Get package info
    //
    PackageVersionName = NULL;
    Status             = Fmp->GetPackageInfo (
                                Fmp,
                                &PackageVersion,           // PackageVersion
                                &PackageVersionName,       // PackageVersionName
                                &PackageVersionNameMaxLen, // PackageVersionNameMaxLen
                                &AttributesSupported,      // AttributesSupported
                                &AttributesSetting         // AttributesSetting
                                );
    if (EFI_ERROR (Status)) {
      Print (L"FMP (%d) PackageInfo - %r\n", Index, Status);
    } else {
      Print (L"FMP (%d) ImageInfo:\n", Index);
      DumpFmpPackageInfo (
        PackageVersion,              // PackageVersion
        PackageVersionName,          // PackageVersionName
        PackageVersionNameMaxLen,    // PackageVersionNameMaxLen
        AttributesSupported,         // AttributesSupported
        AttributesSetting            // AttributesSetting
        );

      if (PackageVersionName != NULL) {
        FreePool (PackageVersionName);
      }
    }
  }

  Print (L"\n");

EXIT:
  FreePool (HandleBuffer);
}

/**
  Check if the ImageInfo includes the ImageTypeId.

  @param[in] ImageInfo           A pointer to EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[in] DescriptorCount     The count of EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[in] DescriptorSize      The size of an individual EFI_FIRMWARE_IMAGE_DESCRIPTOR, in bytes.
  @param[in] ImageTypeId         A unique GUID identifying the firmware image type.

  @return TRUE  This ImageInfo includes the ImageTypeId
  @return FALSE This ImageInfo does not include the ImageTypeId
**/
BOOLEAN
IsThisFmpImageInfo (
  IN EFI_FIRMWARE_IMAGE_DESCRIPTOR  *ImageInfo,
  IN UINT8                          DescriptorCount,
  IN UINTN                          DescriptorSize,
  IN EFI_GUID                       *ImageTypeId
  )
{
  EFI_FIRMWARE_IMAGE_DESCRIPTOR  *CurrentImageInfo;
  UINTN                          Index;

  CurrentImageInfo = ImageInfo;
  for (Index = 0; Index < DescriptorCount; Index++) {
    if (CompareGuid (&CurrentImageInfo->ImageTypeId, ImageTypeId)) {
      return TRUE;
    }

    CurrentImageInfo = (EFI_FIRMWARE_IMAGE_DESCRIPTOR *)((UINT8 *)CurrentImageInfo + DescriptorSize);
  }

  return FALSE;
}

/**
  return the FMP whoes ImageInfo includes the ImageTypeId.

  @param[in] ImageTypeId         A unique GUID identifying the firmware image type.

  @return The FMP whoes ImageInfo includes the ImageTypeId
**/
EFI_FIRMWARE_MANAGEMENT_PROTOCOL *
FindFmpFromImageTypeId (
  IN EFI_GUID  *ImageTypeId
  )
{
  EFI_STATUS                        Status;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *Fmp;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *TargetFmp;
  EFI_HANDLE                        *HandleBuffer;
  UINTN                             NumberOfHandles;
  UINTN                             Index;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR     *FmpImageInfoBuf;
  UINTN                             ImageInfoSize;
  UINT32                            FmpImageInfoDescriptorVer;
  UINT8                             FmpImageInfoCount;
  UINTN                             DescriptorSize;
  UINT32                            PackageVersion;
  CHAR16                            *PackageVersionName;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareManagementProtocolGuid,
                  NULL,
                  &NumberOfHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    Print (L"FMP protocol - %r\n", EFI_NOT_FOUND);
    return NULL;
  }

  TargetFmp = NULL;
  for (Index = 0; Index < NumberOfHandles; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiFirmwareManagementProtocolGuid,
                    (VOID **)&Fmp
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    ImageInfoSize = 0;
    Status        = Fmp->GetImageInfo (
                           Fmp,
                           &ImageInfoSize,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL
                           );
    if (Status != EFI_BUFFER_TOO_SMALL) {
      continue;
    }

    FmpImageInfoBuf = NULL;
    FmpImageInfoBuf = AllocateZeroPool (ImageInfoSize);
    if (FmpImageInfoBuf == NULL) {
      FreePool (HandleBuffer);
      Print (L"Out of resource\n");
      return NULL;
    }

    PackageVersionName = NULL;
    Status             = Fmp->GetImageInfo (
                                Fmp,
                                &ImageInfoSize,             // ImageInfoSize
                                FmpImageInfoBuf,            // ImageInfo
                                &FmpImageInfoDescriptorVer, // DescriptorVersion
                                &FmpImageInfoCount,         // DescriptorCount
                                &DescriptorSize,            // DescriptorSize
                                &PackageVersion,            // PackageVersion
                                &PackageVersionName         // PackageVersionName
                                );

    //
    // If FMP GetInformation interface failed, skip this resource
    //
    if (EFI_ERROR (Status)) {
      FreePool (FmpImageInfoBuf);
      continue;
    }

    if (PackageVersionName != NULL) {
      FreePool (PackageVersionName);
    }

    if (IsThisFmpImageInfo (FmpImageInfoBuf, FmpImageInfoCount, DescriptorSize, ImageTypeId)) {
      TargetFmp = Fmp;
    }

    FreePool (FmpImageInfoBuf);
    if (TargetFmp != NULL) {
      break;
    }
  }

  FreePool (HandleBuffer);
  return TargetFmp;
}

/**
  Dump FMP image data.

  @param[in]  ImageTypeId   The ImageTypeId of the FMP image.
                            It is used to identify the FMP protocol.
  @param[in]  ImageIndex    The ImageIndex of the FMP image.
                            It is the input parameter for FMP->GetImage().
  @param[in]  ImageName     The file name to hold the output FMP image.
**/
VOID
DumpFmpImage (
  IN EFI_GUID  *ImageTypeId,
  IN UINTN     ImageIndex,
  IN CHAR16    *ImageName
  )
{
  EFI_STATUS                        Status;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *Fmp;
  VOID                              *Image;
  UINTN                             ImageSize;

  Fmp = FindFmpFromImageTypeId (ImageTypeId);
  if (Fmp == NULL) {
    Print (L"No FMP include ImageTypeId %g\n", ImageTypeId);
    return;
  }

  if (ImageIndex > 0xFF) {
    Print (L"ImageIndex 0x%x too big\n", ImageIndex);
    return;
  }

  Image     = Fmp;
  ImageSize = 0;
  Status    = Fmp->GetImage (Fmp, (UINT8)ImageIndex, Image, &ImageSize);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    Print (L"Fmp->GetImage - %r\n", Status);
    return;
  }

  Image = AllocatePool (ImageSize);
  if (Image == NULL) {
    Print (L"Allocate FmpImage 0x%x - %r\n", ImageSize, EFI_OUT_OF_RESOURCES);
    return;
  }

  Status = Fmp->GetImage (Fmp, (UINT8)ImageIndex, Image, &ImageSize);
  if (EFI_ERROR (Status)) {
    Print (L"Fmp->GetImage - %r\n", Status);
    return;
  }

  Status = WriteFileFromBuffer (ImageName, ImageSize, Image);
  Print (L"CapsuleApp: Dump %g ImageIndex (0x%x) to %s %r\n", ImageTypeId, ImageIndex, ImageName, Status);

  FreePool (Image);

  return;
}
