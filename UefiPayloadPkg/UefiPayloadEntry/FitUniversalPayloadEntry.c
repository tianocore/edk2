/** @file
  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "UefiPayloadEntry.h"
#include <Library/FdtLib.h>
#include <Guid/UniversalPayloadBase.h>
#include <Guid/MemoryTypeInformation.h>
#include <Library/FdtParserLib.h>
#include <Library/HobParserLib.h>

#define MEMORY_ATTRIBUTE_MASK  (EFI_RESOURCE_ATTRIBUTE_PRESENT             |        \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED         | \
                                       EFI_RESOURCE_ATTRIBUTE_TESTED              | \
                                       EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED      | \
                                       EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED     | \
                                       EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED | \
                                       EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTED | \
                                       EFI_RESOURCE_ATTRIBUTE_16_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_32_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_64_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_PERSISTENT          )

#define TESTED_MEMORY_ATTRIBUTES  (EFI_RESOURCE_ATTRIBUTE_PRESENT     |     \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED | \
                                       EFI_RESOURCE_ATTRIBUTE_TESTED      )

GLOBAL_REMOVE_IF_UNREFERENCED EFI_MEMORY_TYPE_INFORMATION  mDefaultMemoryTypeInformation[] = {
  { EfiACPIReclaimMemory,   FixedPcdGet32 (PcdMemoryTypeEfiACPIReclaimMemory)   },
  { EfiACPIMemoryNVS,       FixedPcdGet32 (PcdMemoryTypeEfiACPIMemoryNVS)       },
  { EfiReservedMemoryType,  FixedPcdGet32 (PcdMemoryTypeEfiReservedMemoryType)  },
  { EfiRuntimeServicesData, FixedPcdGet32 (PcdMemoryTypeEfiRuntimeServicesData) },
  { EfiRuntimeServicesCode, FixedPcdGet32 (PcdMemoryTypeEfiRuntimeServicesCode) },
  { EfiMaxMemoryType,       0                                                   }
};

extern VOID  *mHobList;

CHAR8  *mLineBuffer = NULL;

/**
  Print all HOBs info from the HOB list.
  @return The pointer to the HOB list.
**/
VOID
PrintHob (
  IN CONST VOID  *HobStart
  );

VOID
EFIAPI
ProcessLibraryConstructorList (
  VOID
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

  Fdt = NULL;

  GuidHob = GetFirstGuidHob (&gUniversalPayloadBaseGuid);
  if (GuidHob != NULL) {
    PayloadBase = (UNIVERSAL_PAYLOAD_BASE *)GET_GUID_HOB_DATA (GuidHob);
    Fdt         = (VOID *)(UINTN)PayloadBase->Entry;
    DEBUG ((DEBUG_INFO, "PayloadBase Entry = 0x%08x\n", PayloadBase->Entry));
  }

  if (Fdt == NULL) {
    return EFI_UNSUPPORTED;
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
    } else {
      BuildFvHob (((UINTN)PayloadBase->Entry + (UINTN)DataOffset), DataSize);
    }

    DEBUG ((
      DEBUG_INFO,
      "UPL Multiple fv[%a], Base=0x%08x, size=0x%08x\n",
      Fvname,
      ((UINTN)PayloadBase->Entry + (UINTN)DataOffset),
      DataSize
      ));
    Depth  = FdtNodeDepth (Fdt, FvNode);
    FvNode = FdtNextNode (Fdt, FvNode, &Depth);
    Fvname = FdtGetName (Fdt, FvNode, &TempLen);
  }

  return EFI_SUCCESS;
}

/**
 *
  Create new HOB for new HOB list

  @param[in]  BootloaderParameter  The HOB to be added into the HOB list.
**/
VOID
CreatNewHobForHoblist (
  IN UINTN  BootloaderParameter
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  UINTN                        MinimalNeededSize;
  EFI_PHYSICAL_ADDRESS         FreeMemoryBottom;
  EFI_PHYSICAL_ADDRESS         FreeMemoryTop;
  EFI_PHYSICAL_ADDRESS         MemoryBottom;
  EFI_PHYSICAL_ADDRESS         MemoryTop;
  EFI_HOB_RESOURCE_DESCRIPTOR  *PhitResourceHob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;
  EFI_HOB_HANDOFF_INFO_TABLE   *HobInfo;

  Hob.Raw           = (UINT8 *)BootloaderParameter;
  MinimalNeededSize = FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);

  ASSERT (Hob.Raw != NULL);
  ASSERT ((UINTN)Hob.HandoffInformationTable->EfiFreeMemoryTop == Hob.HandoffInformationTable->EfiFreeMemoryTop);
  ASSERT ((UINTN)Hob.HandoffInformationTable->EfiMemoryTop == Hob.HandoffInformationTable->EfiMemoryTop);
  ASSERT ((UINTN)Hob.HandoffInformationTable->EfiFreeMemoryBottom == Hob.HandoffInformationTable->EfiFreeMemoryBottom);
  ASSERT ((UINTN)Hob.HandoffInformationTable->EfiMemoryBottom == Hob.HandoffInformationTable->EfiMemoryBottom);

  //
  // Try to find Resource Descriptor HOB that contains Hob range EfiMemoryBottom..EfiMemoryTop
  //
  PhitResourceHob = FindResourceDescriptorByRange (Hob.Raw, Hob.HandoffInformationTable->EfiMemoryBottom, Hob.HandoffInformationTable->EfiMemoryTop);
  if (PhitResourceHob == NULL) {
    //
    // Boot loader's Phit Hob is not in an available Resource Descriptor, find another Resource Descriptor for new Phit Hob
    //
    ResourceHob = FindAnotherHighestBelow4GResourceDescriptor (Hob.Raw, MinimalNeededSize, NULL);
    if (ResourceHob == NULL) {
      return;
    }

    MemoryBottom     = ResourceHob->PhysicalStart + ResourceHob->ResourceLength - MinimalNeededSize;
    FreeMemoryBottom = MemoryBottom;
    FreeMemoryTop    = ResourceHob->PhysicalStart + ResourceHob->ResourceLength;
    MemoryTop        = FreeMemoryTop;
  } else if (PhitResourceHob->PhysicalStart + PhitResourceHob->ResourceLength - Hob.HandoffInformationTable->EfiMemoryTop >= MinimalNeededSize) {
    //
    // New availiable Memory range in new hob is right above memory top in old hob.
    //
    MemoryBottom     = Hob.HandoffInformationTable->EfiFreeMemoryTop;
    FreeMemoryBottom = Hob.HandoffInformationTable->EfiMemoryTop;
    FreeMemoryTop    = FreeMemoryBottom + MinimalNeededSize;
    MemoryTop        = FreeMemoryTop;
  } else if (Hob.HandoffInformationTable->EfiMemoryBottom - PhitResourceHob->PhysicalStart >= MinimalNeededSize) {
    //
    // New availiable Memory range in new hob is right below memory bottom in old hob.
    //
    MemoryBottom     = Hob.HandoffInformationTable->EfiMemoryBottom - MinimalNeededSize;
    FreeMemoryBottom = MemoryBottom;
    FreeMemoryTop    = Hob.HandoffInformationTable->EfiMemoryBottom;
    MemoryTop        = Hob.HandoffInformationTable->EfiMemoryTop;
  } else {
    //
    // In the Resource Descriptor HOB contains boot loader Hob, there is no enough free memory size for payload hob
    // Find another Resource Descriptor Hob
    //
    ResourceHob = FindAnotherHighestBelow4GResourceDescriptor (Hob.Raw, MinimalNeededSize, PhitResourceHob);
    if (ResourceHob == NULL) {
      return;
    }

    MemoryBottom     = ResourceHob->PhysicalStart + ResourceHob->ResourceLength - MinimalNeededSize;
    FreeMemoryBottom = MemoryBottom;
    FreeMemoryTop    = ResourceHob->PhysicalStart + ResourceHob->ResourceLength;
    MemoryTop        = FreeMemoryTop;
  }

  HobInfo           = HobConstructor ((VOID *)(UINTN)MemoryBottom, (VOID *)(UINTN)MemoryTop, (VOID *)(UINTN)FreeMemoryBottom, (VOID *)(UINTN)FreeMemoryTop);
  HobInfo->BootMode = Hob.HandoffInformationTable->BootMode;

  //
  // Since payload created new Hob, move all hobs except PHIT from boot loader hob list.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (IsHobNeed (Hob)) {
      // Add this hob to payload HOB
      AddNewHob (&Hob);
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  return;
}

/**
  It will build HOBs based on information from bootloaders.
  @param[in]  NewFdtBase     The pointer to New FdtBase.
  @param[out] DxeFv          The pointer to the DXE FV in memory.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required HOBs.
**/
EFI_STATUS
FitBuildHobs (
  IN  UINTN                       NewFdtBase,
  OUT EFI_FIRMWARE_VOLUME_HEADER  **DxeFv
  )
{
  UINT8                          *GuidHob;
  UINT32                         FdtSize;
  EFI_HOB_FIRMWARE_VOLUME        *FvHob;
  UNIVERSAL_PAYLOAD_ACPI_TABLE   *AcpiTable;
  ACPI_BOARD_INFO                *AcpiBoardInfo;
  UNIVERSAL_PAYLOAD_DEVICE_TREE  *Fdt;

  if (FixedPcdGetBool (PcdHandOffFdtEnable)) {
    //
    // Back up FDT in Reserved memory region
    //
    if (NewFdtBase != 0) {
      GuidHob = GetFirstGuidHob (&gUniversalPayloadDeviceTreeGuid);
      if (GuidHob != NULL) {
        Fdt =  (UNIVERSAL_PAYLOAD_DEVICE_TREE *)GET_GUID_HOB_DATA (GuidHob);
        if (Fdt != NULL) {
          DEBUG ((DEBUG_INFO, "Update FDT base to reserved memory\n"));
          FdtSize = PcdGet8 (PcdFDTPageSize) * EFI_PAGE_SIZE;
          CopyMem ((VOID *)NewFdtBase, (VOID *)(Fdt->DeviceTreeAddress), FdtSize);
          Fdt->DeviceTreeAddress = NewFdtBase;
        }
      }
    }
  }

  //
  // To create Memory Type Information HOB
  //
  GuidHob = GetFirstGuidHob (&gEfiMemoryTypeInformationGuid);
  if (GuidHob == NULL) {
    BuildGuidDataHob (
      &gEfiMemoryTypeInformationGuid,
      mDefaultMemoryTypeInformation,
      sizeof (mDefaultMemoryTypeInformation)
      );
  }

  //
  // Create guid hob for acpi board information
  //
  GuidHob = GetFirstGuidHob (&gUniversalPayloadAcpiTableGuid);
  if (GuidHob != NULL) {
    AcpiTable = (UNIVERSAL_PAYLOAD_ACPI_TABLE *)GET_GUID_HOB_DATA (GuidHob);
    GuidHob   = GetFirstGuidHob (&gUefiAcpiBoardInfoGuid);
    if (GuidHob == NULL) {
      AcpiBoardInfo = BuildHobFromAcpi ((UINT64)AcpiTable->Rsdp);
      ASSERT (AcpiBoardInfo != NULL);
    }
  }

  //
  // Create an empty FvHob for the DXE FV that contains DXE core.
  //
  BuildFvHob ((EFI_PHYSICAL_ADDRESS)0, 0);

  BuildFitLoadablesFvHob (DxeFv);
  //
  // Update DXE FV information to first fv hob in the hob list, which
  // is the empty FvHob created before.
  //
  FvHob              = GetFirstHob (EFI_HOB_TYPE_FV);
  FvHob->BaseAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)*DxeFv;
  FvHob->Length      = (*DxeFv)->FvLength;
  return EFI_SUCCESS;
}

/**
  Entry point to the C language phase of UEFI payload.
  @param[in]   BootloaderParameter  The starting address of FDT .
  @retval      It will not return if SUCCESS, and return error when passing bootloader parameter.
**/
EFI_STATUS
EFIAPI
FitUplEntryPoint (
  IN UINTN  BootloaderParameter
  )
{
  EFI_STATUS                  Status;
  PHYSICAL_ADDRESS            DxeCoreEntryPoint;
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_FIRMWARE_VOLUME_HEADER  *DxeFv;

 #if FixedPcdGetBool (PcdHandOffFdtEnable) == 1
  PHYSICAL_ADDRESS  HobListPtr;
  VOID              *FdtBase;
 #endif
  VOID  *FdtBaseResvd;

  if (FixedPcdGetBool (PcdHandOffFdtEnable)) {
    mHobList = (VOID *)NULL;
  } else {
    mHobList = (VOID *)BootloaderParameter;
  }

  DxeFv        = NULL;
  FdtBaseResvd = 0;
  // Call constructor for all libraries
  ProcessLibraryConstructorList ();

  DEBUG ((DEBUG_INFO, "Entering Universal Payload...\n"));
  DEBUG ((DEBUG_INFO, "sizeof(UINTN) = 0x%x\n", sizeof (UINTN)));
  DEBUG ((DEBUG_INFO, "BootloaderParameter = 0x%x\n", BootloaderParameter));

  DEBUG ((DEBUG_INFO, "Start init Hobs...\n"));
 #if FixedPcdGetBool (PcdHandOffFdtEnable) == 1
  HobListPtr = UplInitHob ((VOID *)BootloaderParameter);

  //
  // Found hob list node
  //
  if (HobListPtr != 0) {
    FdtBase = (VOID *)BootloaderParameter;
    if (FdtCheckHeader (FdtBase) == 0) {
      CustomFdtNodeParser ((VOID *)FdtBase, (VOID *)HobListPtr);
      FdtBaseResvd = PayloadAllocatePages (PcdGet8 (PcdFDTPageSize), EfiReservedMemoryType);
    }
  }

 #else
  CreatNewHobForHoblist (BootloaderParameter);
 #endif

  // Build HOB based on information from Bootloader
  Status = FitBuildHobs ((UINTN)FdtBaseResvd, &DxeFv);

  // Call constructor for all libraries again since hobs were built
  ProcessLibraryConstructorList ();

  DEBUG_CODE (
    //
    // Dump the Hobs from boot loader
    //
    PrintHob (mHobList);
    );

  FixUpPcdDatabase (DxeFv);
  Status = UniversalLoadDxeCore (DxeFv, &DxeCoreEntryPoint);
  ASSERT_EFI_ERROR (Status);

  Hob.HandoffInformationTable = (EFI_HOB_HANDOFF_INFO_TABLE *)GetFirstHob (EFI_HOB_TYPE_HANDOFF);
  HandOffToDxeCore (DxeCoreEntryPoint, Hob);

  // Should not get here
  CpuDeadLoop ();
  return EFI_SUCCESS;
}
