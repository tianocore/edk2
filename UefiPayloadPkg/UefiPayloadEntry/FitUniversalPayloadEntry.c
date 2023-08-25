/** @file
  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "UefiPayloadEntry.h"

#include <FitLib.h>
#include <Elf32.h>
#include <Elf64.h>
#include <Library/FdtLib.h>

extern VOID  *mHobList;

CHAR8  *mLineBuffer = NULL;
#if FixedPcdGet8(PcdFdtSupport) == 0 || FixedPcdGet8 (PcdFdtSupport) == 2
/**
  It will build HOBs based on information from bootloaders.

  @param[in]  BootloaderParameter   The starting memory address of bootloader parameter block.

  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required HOBs.
**/
EFI_STATUS
BuildHobs (
  IN  UINTN                       BootloaderParameter
  );
#endif

#if FixedPcdGet8(PcdFdtSupport) == 1 || FixedPcdGet8 (PcdFdtSupport) == 2
/**
  It will initialize HOBs for UPL.

  @param[in]  FdtBase        Address of the Fdt data.

  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to initialize HOBs.
**/
UINT64
UplInitHob (
  IN VOID                         *FdtBase
  );

/**
  It will parse FDT based on DTB from bootloaders.

  @param[in]  FdtBase               Address of the Fdt data.

  @retval EFI_SUCCESS               If it completed successfully.
  @retval Others                    If it failed to parse DTB.
**/
UINTN
ParseDtb (
  IN VOID                           *FdtBase
  );
#endif

/**
  Print all HOBs info from the HOB list.

  @return The pointer to the HOB list.
**/
VOID
PrintHob (
  IN CONST VOID  *HobStart
  );

/**
  Find the first substring.
  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.
**/
UINTN
EFIAPI
AsciiStrSpn (
  IN CHAR8  *String,
  IN CHAR8  *CharSet
  )
{
  UINTN  Count;
  CHAR8  *Str1;
  CHAR8  *Str2;

  Count = 0;

  for (Str1 = String; *Str1 != L'\0'; Str1++) {
    for (Str2 = CharSet; *Str2 != L'\0'; Str2++) {
      if (*Str1 == *Str2) {
        break;
      }
    }

    if (*Str2 == L'\0') {
      return Count;
    }

    Count++;
  }

  return Count;
}

/**
  Searches a string for the first occurrence of a character contained in a
  specified buffer.
  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.
**/
CHAR8 *
EFIAPI
AsciiStrBrk (
  IN CHAR8  *String,
  IN CHAR8  *CharSet
  )
{
  CHAR8  *Str1;
  CHAR8  *Str2;

  for (Str1 = String; *Str1 != L'\0'; Str1++) {
    for (Str2 = CharSet; *Str2 != L'\0'; Str2++) {
      if (*Str1 == *Str2) {
        return (CHAR8 *)Str1;
      }
    }
  }

  return NULL;
}

/**
  Find the next token after one or more specified characters.
  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.
**/
CHAR8 *
EFIAPI
AsciiStrTokenLine (
  IN CHAR8  *String OPTIONAL,
  IN CHAR8  *CharSet
  )
{
  CHAR8  *Begin;
  CHAR8  *End;

  Begin = (String == NULL) ? mLineBuffer : String;
  if (Begin == NULL) {
    return NULL;
  }

  Begin += AsciiStrSpn (Begin, CharSet);
  if (*Begin == L'\0') {
    mLineBuffer = NULL;
    return NULL;
  }

  End = AsciiStrBrk (Begin, CharSet);
  if ((End != NULL) && (*End != L'\0')) {
    *End = L'\0';
    End++;
  }

  mLineBuffer = End;
  return Begin;
}

/**
  It will build Fv HOBs based on information from bootloaders.
  @param[out] DxeFv          The pointer to the DXE FV in memory.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval EFI_NOT_FOUND      If it failed to find node in fit image.
  @retval Others             If it failed to build required HOBs.
**/
EFI_STATUS
BuildFitLoadablesFvHob (
  OUT EFI_FIRMWARE_VOLUME_HEADER  **DxeFv
  )
{
  EFI_STATUS              Status;
  VOID                    *Fdt;
  UINT8                   *GuidHob;
  UNIVERSAL_PAYLOAD_BASE  *PayloadBase;
  INT32                   ConfigNode;
  INT32                   Config1Node;
  INT32                   ImageNode;
  INT32                   FvNode;
  INT32                   Depth;
  CONST FDT_PROPERTY      *PropertyPtr;
  INT32                   TempLen;
  CONST CHAR8             *Fvname;
  UINT32                  DataOffset;
  UINT32                  DataSize;
  UINT32                  *Data32;
  GuidHob = GetFirstGuidHob (&gUniversalPayloadBaseGuid);
  if (GuidHob != NULL) {
    PayloadBase = (UNIVERSAL_PAYLOAD_BASE *)GET_GUID_HOB_DATA (GuidHob);
    Fdt         = (VOID *)(UINTN)PayloadBase->Entry;
    DEBUG ((DEBUG_INFO, "PayloadBase Entry = 0x%08x\n", PayloadBase->Entry));
  }

  Status = FdtCheckHeader (Fdt);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  ConfigNode = FdtSubnodeOffsetNameLen (Fdt, 0, "configurations", (INT32)AsciiStrLen ("configurations"));
  if (ConfigNode <= 0) {
    return EFI_NOT_FOUND;
  }

  Config1Node = FdtSubnodeOffsetNameLen (Fdt, ConfigNode, "conf-1", (INT32)AsciiStrLen ("conf-1"));
  if (Config1Node <= 0) {
    return EFI_NOT_FOUND;
  }
  ImageNode   = FdtSubnodeOffsetNameLen (Fdt, 0, "images", (INT32)AsciiStrLen ("images"));
  if (ImageNode <= 0) {
    return EFI_NOT_FOUND;
  }
  FvNode = FdtSubnodeOffsetNameLen (Fdt, ImageNode, "tianocore", (INT32)AsciiStrLen ("tianocore"));
  Depth  = FdtNodeDepth (Fdt, FvNode);
  FvNode = FdtNextNode (Fdt, FvNode, &Depth);
  Fvname = FdtGetName (Fdt, FvNode, &TempLen);
  while ((AsciiStrCmp((Fvname + AsciiStrLen(Fvname) - 2), "fv") == 0)) {
    if (FvNode <= 0) {
      return EFI_NOT_FOUND;
    }
    PropertyPtr = FdtGetProperty (Fdt, FvNode, "data-offset", &TempLen);
    Data32      = (UINT32 *)(PropertyPtr->Data);
    DataOffset  = SwapBytes32 (*Data32);

    PropertyPtr = FdtGetProperty (Fdt, FvNode, "data-size", &TempLen);
    Data32      = (UINT32 *)(PropertyPtr->Data);
    DataSize    = SwapBytes32 (*Data32);

    if (AsciiStrCmp (Fvname, "uefi-fv") == 0) {
      *DxeFv = (EFI_FIRMWARE_VOLUME_HEADER *)((UINTN)PayloadBase->Entry + (UINTN)DataOffset);
      ASSERT ((*DxeFv)->FvLength == DataSize);
    } else {
      BuildFvHob (((UINTN)PayloadBase->Entry + (UINTN)DataOffset), DataSize);
    }

    DEBUG ((
      DEBUG_INFO,
      "UPL Multiple fv[%a], Base=0x%08x, size=0x%08x\n",
      Fvname,
      ((UINTN)PayloadBase->Entry + (UINTN)DataOffset),
      DataSize,
      DataOffset
      ));
    Depth   = FdtNodeDepth (Fdt, FvNode);
    FvNode  = FdtNextNode (Fdt, FvNode, &Depth);
    Fvname  = FdtGetName (Fdt, FvNode, &TempLen);
  }
  return EFI_SUCCESS;
}
/**
  Retrieve DXE FV from Hob.

  @return .
**/
EFI_STATUS
GetDxeFv (
  OUT EFI_FIRMWARE_VOLUME_HEADER  **DxeFv
  )
{
  EFI_HOB_FIRMWARE_VOLUME       *FvHob;

  BuildFitLoadablesFvHob (DxeFv);

  //
  // Update DXE FV information to first fv hob in the hob list, which
  // is the empty FvHob created before.
  //
  FvHob              = GetFirstHob (EFI_HOB_TYPE_FV);
  FvHob->BaseAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) *DxeFv;
  FvHob->Length      = (*DxeFv)->FvLength;

  return EFI_SUCCESS;
}

/**
  Find extra FV sections from Fit image.

  @return .
**/
EFI_STATUS
FitFindExtraSection (
  IN  EFI_PHYSICAL_ADDRESS        ElfEntryPoint,
  OUT EFI_FIRMWARE_VOLUME_HEADER  **DxeFv
  )
{
  EFI_STATUS              Status;
  VOID                    *Fdt;
  EFI_HOB_FIRMWARE_VOLUME  *FvHob;
  UINT8                   *GuidHob;
  UNIVERSAL_PAYLOAD_BASE  *PayloadBase;
  INT32                   ConfigNode;
  INT32                   Config1Node;
  INT32                   ImageNode;
  INT32                   FvNode;
  INT32                   Depth;
  CONST FDT_PROPERTY      *PropertyPtr;
  INT32                   TempLen;
  CONST CHAR8             *Fvname;
  UINT32                  DataOffset;
  UINT32                  DataSize;
  UINT32                  *Data32;

  GuidHob = GetFirstGuidHob (&gUniversalPayloadBaseGuid);
  if (GuidHob != NULL) {
    PayloadBase = (UNIVERSAL_PAYLOAD_BASE *) GET_GUID_HOB_DATA (GuidHob);
    Fdt         = (VOID *)(UINTN)PayloadBase->Entry;
    DEBUG ((DEBUG_INFO, "PayloadBase Entry = 0x%08x\n", PayloadBase->Entry));
  }
  else {
    Status = GetDxeFv (DxeFv);
    FvHob              = GetFirstHob (EFI_HOB_TYPE_FV);
    FvHob->BaseAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) *DxeFv;
    FvHob->Length      = (*DxeFv)->FvLength;

    return Status;
  }
  Status = FdtCheckHeader (Fdt);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }
  ConfigNode = FdtSubnodeOffsetNameLen (Fdt, 0, "configurations", (INT32)AsciiStrLen ("configurations"));
  if (ConfigNode <= 0) {
    return EFI_NOT_FOUND;
  }

  Config1Node = FdtSubnodeOffsetNameLen (Fdt, ConfigNode, "conf-1", (INT32)AsciiStrLen ("conf-1"));
  if (Config1Node <= 0) {
    return EFI_NOT_FOUND;
  }

  ImageNode = FdtSubnodeOffsetNameLen (Fdt, 0, "images", (INT32)AsciiStrLen ("images"));
  if (ImageNode <= 0) {
    return EFI_NOT_FOUND;
  }

  FvNode = FdtSubnodeOffsetNameLen (Fdt, ImageNode, "tianocore", (INT32)AsciiStrLen ("tianocore"));
  Depth  = FdtNodeDepth (Fdt, FvNode);
  FvNode = FdtNextNode (Fdt, FvNode, &Depth);
  Fvname = FdtGetName (Fdt, FvNode, &TempLen);
  while ((AsciiStrCmp ((Fvname + AsciiStrLen (Fvname) - 2), "fv") == 0)) {
    if (FvNode <= 0) {
      return EFI_NOT_FOUND;
    }
    PropertyPtr = FdtGetProperty (Fdt, FvNode, "data-offset", &TempLen);
    Data32      = (UINT32 *)(PropertyPtr->Data);
    DataOffset  = SwapBytes32 (*Data32);

    PropertyPtr = FdtGetProperty (Fdt, FvNode, "data-size", &TempLen);
    Data32      = (UINT32 *)(PropertyPtr->Data);
    DataSize    = SwapBytes32 (*Data32);
    if (AsciiStrCmp (Fvname, "uefi-fv") == 0) {
      *DxeFv = (EFI_FIRMWARE_VOLUME_HEADER *)((UINTN)PayloadBase->Entry + (UINTN)DataOffset);
      ASSERT ((*DxeFv)->FvLength == DataSize);
      DEBUG ((DEBUG_INFO, "  - Found uefi_fv section\n"));
      FvHob              = GetFirstHob (EFI_HOB_TYPE_FV);
      FvHob->BaseAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) *DxeFv;
      FvHob->Length      = (*DxeFv)->FvLength;
    }
  }


  return Status;
}
/**
  Some bootloader may pass a pcd database, and UPL also contain a PCD database.
  Dxe PCD driver has the assumption that the two PCD database can be catenated and
  the local token number should be successive.
  This function will fix up the UPL PCD database to meet that assumption.
  @param[in]   DxeFv         The FV where to find the Universal PCD database.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval other              Failed to fix up.
**/
EFI_STATUS
FixUpPcdDatabase (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *DxeFv
  )
{
  EFI_STATUS           Status;
  EFI_FFS_FILE_HEADER  *FileHeader;
  VOID                 *PcdRawData;
  PEI_PCD_DATABASE     *PeiDatabase;
  PEI_PCD_DATABASE     *UplDatabase;
  EFI_HOB_GUID_TYPE    *GuidHob;
  DYNAMICEX_MAPPING    *ExMapTable;
  UINTN                Index;

  GuidHob = GetFirstGuidHob (&gPcdDataBaseHobGuid);
  if (GuidHob == NULL) {
    //
    // No fix-up is needed.
    //
    return EFI_SUCCESS;
  }

  PeiDatabase = (PEI_PCD_DATABASE *)GET_GUID_HOB_DATA (GuidHob);
  DEBUG ((DEBUG_INFO, "Find the Pei PCD data base, the total local token number is %d\n", PeiDatabase->LocalTokenCount));

  Status = FvFindFileByTypeGuid (DxeFv, EFI_FV_FILETYPE_DRIVER, PcdGetPtr (PcdPcdDriverFile), &FileHeader);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = FileFindSection (FileHeader, EFI_SECTION_RAW, &PcdRawData);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  UplDatabase = (PEI_PCD_DATABASE *)PcdRawData;
  ExMapTable  = (DYNAMICEX_MAPPING *)(UINTN)((UINTN)PcdRawData + UplDatabase->ExMapTableOffset);

  for (Index = 0; Index < UplDatabase->ExTokenCount; Index++) {
    ExMapTable[Index].TokenNumber += PeiDatabase->LocalTokenCount;
  }

  DEBUG ((DEBUG_INFO, "Fix up UPL PCD database successfully\n"));
  return EFI_SUCCESS;
}

/**
  Entry point to the C language phase of UEFI payload.
  @param[in]   BootloaderParameter    The starting address of bootloader parameter block.
  @retval      It will not return if SUCCESS, and return error when passing bootloader parameter.
**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN UINTN  BootloaderParameter
  )
{
  EFI_STATUS                  Status;
  PHYSICAL_ADDRESS            DxeCoreEntryPoint;
#if FixedPcdGet8 (PcdFdtSupport) == 1 || FixedPcdGet8 (PcdFdtSupport) == 2
  PHYSICAL_ADDRESS            HobListPtr;
#endif
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_FIRMWARE_VOLUME_HEADER  *DxeFv;

  DxeFv    = NULL;
  // Call constructor with default values for all libraries
  ProcessLibraryConstructorList ();

  // Initialize floating point operating environment to be compliant with UEFI spec.
  InitializeFloatingPointUnits ();

  DEBUG ((DEBUG_INFO, "Entering Universal Payload...\n"));
  DEBUG ((DEBUG_INFO, "sizeof(UINTN) = 0x%x\n", sizeof (UINTN)));
  DEBUG ((DEBUG_INFO, "BootloaderParameter = 0x%x\n", BootloaderParameter));

#if FixedPcdGet8 (PcdFdtSupport) == 0
  mHobList = (VOID *) BootloaderParameter;

  DEBUG ((DEBUG_INFO, "Start build HOB...\n"));

  // Build HOB based on information from Bootloader
  Status = BuildHobs (BootloaderParameter);
  ASSERT_EFI_ERROR (Status);
#endif

#if FixedPcdGet8 (PcdFdtSupport) == 1 || FixedPcdGet8 (PcdFdtSupport) == 2
  DEBUG ((DEBUG_INFO, "Start parsing FDT...\n"));
  HobListPtr = UplInitHob ((VOID *) BootloaderParameter);

  //
  // Found hob list node
  //
  if (HobListPtr != 0) {
    Status = BuildHobs (HobListPtr);
    ParseDtb ((VOID *) BootloaderParameter);
  }
#endif

  // Call constructor for all libraries again since hobs were built
  ProcessLibraryConstructorList ();

  FitFindExtraSection ((EFI_PHYSICAL_ADDRESS) (UINTN) _ModuleEntryPoint, &DxeFv);

  DEBUG_CODE (
    //
    // Dump the Hobs from boot loader
    //
    PrintHob (mHobList);
    );

  FixUpPcdDatabase (DxeFv);
  Status = UniversalLoadDxeCore (DxeFv, &DxeCoreEntryPoint);
  ASSERT_EFI_ERROR (Status);

  //
  // Mask off all legacy 8259 interrupt sources
  //
  IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, 0xFF);
  IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE, 0xFF);

  Hob.HandoffInformationTable = (EFI_HOB_HANDOFF_INFO_TABLE *) GetFirstHob (EFI_HOB_TYPE_HANDOFF);
  HandOffToDxeCore (DxeCoreEntryPoint, Hob);

  // Should not get here
  CpuDeadLoop ();
  return EFI_SUCCESS;
}
