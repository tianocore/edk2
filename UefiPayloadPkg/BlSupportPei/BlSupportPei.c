/** @file
  This PEIM will parse bootloader information and report resource information into pei core.
  This file contains the main entrypoint of the PEIM.

Copyright (c) 2014 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "BlSupportPei.h"

#define LEGACY_8259_MASK_REGISTER_MASTER  0x21
#define LEGACY_8259_MASK_REGISTER_SLAVE   0xA1

EFI_MEMORY_TYPE_INFORMATION mDefaultMemoryTypeInformation[] = {
  { EfiACPIReclaimMemory,   FixedPcdGet32 (PcdMemoryTypeEfiACPIReclaimMemory) },
  { EfiACPIMemoryNVS,       FixedPcdGet32 (PcdMemoryTypeEfiACPIMemoryNVS) },
  { EfiReservedMemoryType,  FixedPcdGet32 (PcdMemoryTypeEfiReservedMemoryType) },
  { EfiRuntimeServicesData, FixedPcdGet32 (PcdMemoryTypeEfiRuntimeServicesData) },
  { EfiRuntimeServicesCode, FixedPcdGet32 (PcdMemoryTypeEfiRuntimeServicesCode) },
  { EfiMaxMemoryType,       0     }
};

EFI_PEI_PPI_DESCRIPTOR   mPpiBootMode[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiMasterBootModePpiGuid,
    NULL
  }
};

EFI_PEI_GRAPHICS_DEVICE_INFO_HOB mDefaultGraphicsDeviceInfo = {
  MAX_UINT16, MAX_UINT16, MAX_UINT16, MAX_UINT16, MAX_UINT8,  MAX_UINT8
};

/**
  Create memory mapped io resource hob.

  @param  MmioBase    Base address of the memory mapped io range
  @param  MmioSize    Length of the memory mapped io range

**/
VOID
BuildMemoryMappedIoRangeHob (
  EFI_PHYSICAL_ADDRESS        MmioBase,
  UINT64                      MmioSize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
    (EFI_RESOURCE_ATTRIBUTE_PRESENT    |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_TESTED),
    MmioBase,
    MmioSize
    );

  BuildMemoryAllocationHob (
    MmioBase,
    MmioSize,
    EfiMemoryMappedIO
    );
}

/**
  Check the integrity of firmware volume header

  @param[in]  FwVolHeader   A pointer to a firmware volume header

  @retval     TRUE          The firmware volume is consistent
  @retval     FALSE         The firmware volume has corrupted.

**/
STATIC
BOOLEAN
IsFvHeaderValid (
  IN EFI_FIRMWARE_VOLUME_HEADER    *FwVolHeader
  )
{
  UINT16 Checksum;

  // Skip nv storage fv
  if (CompareMem (&FwVolHeader->FileSystemGuid, &gEfiFirmwareFileSystem2Guid, sizeof(EFI_GUID)) != 0 ) {
    return FALSE;
  }

  if ( (FwVolHeader->Revision != EFI_FVH_REVISION)   ||
     (FwVolHeader->Signature != EFI_FVH_SIGNATURE) ||
     (FwVolHeader->FvLength == ((UINTN) -1))       ||
     ((FwVolHeader->HeaderLength & 0x01 ) !=0) )  {
    return FALSE;
  }

  Checksum = CalculateCheckSum16 ((UINT16 *) FwVolHeader, FwVolHeader->HeaderLength);
  if (Checksum != 0) {
    DEBUG (( DEBUG_ERROR,
              "ERROR - Invalid Firmware Volume Header Checksum, change 0x%04x to 0x%04x\r\n",
              FwVolHeader->Checksum,
              (UINT16)( Checksum + FwVolHeader->Checksum )));
    return TRUE; //FALSE; Need update UEFI build tool when patching entrypoin @start of fd.
  }

  return TRUE;
}

/**
  Install FvInfo PPI and create fv hobs for remained fvs

**/
VOID
PeiReportRemainedFvs (
  VOID
  )
{
  UINT8*  TempPtr;
  UINT8*  EndPtr;

  TempPtr = (UINT8* )(UINTN) PcdGet32 (PcdPayloadFdMemBase);
  EndPtr = (UINT8* )(UINTN) (PcdGet32 (PcdPayloadFdMemBase) + PcdGet32 (PcdPayloadFdMemSize));

  for (;TempPtr < EndPtr;) {
    if (IsFvHeaderValid ((EFI_FIRMWARE_VOLUME_HEADER* )TempPtr)) {
      if (TempPtr != (UINT8* )(UINTN) PcdGet32 (PcdPayloadFdMemBase))  {
        // Skip the PEI FV
        DEBUG((DEBUG_INFO, "Found one valid fv : 0x%lx.\n", TempPtr, ((EFI_FIRMWARE_VOLUME_HEADER* )TempPtr)->FvLength));

        PeiServicesInstallFvInfoPpi (
          NULL,
          (VOID *) (UINTN) TempPtr,
          (UINT32) (UINTN) ((EFI_FIRMWARE_VOLUME_HEADER* )TempPtr)->FvLength,
          NULL,
          NULL
          );
        BuildFvHob ((EFI_PHYSICAL_ADDRESS)(UINTN) TempPtr, ((EFI_FIRMWARE_VOLUME_HEADER* )TempPtr)->FvLength);
      }
    }
    TempPtr += ((EFI_FIRMWARE_VOLUME_HEADER* )TempPtr)->FvLength;
  }
}


/**
  Find the board related info from ACPI table

  @param  AcpiTableBase          ACPI table start address in memory
  @param  AcpiBoardInfo          Pointer to the acpi board info strucutre

  @retval RETURN_SUCCESS     Successfully find out all the required information.
  @retval RETURN_NOT_FOUND   Failed to find the required info.

**/
RETURN_STATUS
ParseAcpiInfo (
  IN   UINT64                                   AcpiTableBase,
  OUT  ACPI_BOARD_INFO                          *AcpiBoardInfo
  )
{
  EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp;
  EFI_ACPI_DESCRIPTION_HEADER                   *Rsdt;
  UINT32                                        *Entry32;
  UINTN                                         Entry32Num;
  EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE     *Fadt;
  EFI_ACPI_DESCRIPTION_HEADER                   *Xsdt;
  UINT64                                        *Entry64;
  UINTN                                         Entry64Num;
  UINTN                                         Idx;
  EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER *MmCfgHdr;
  EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE *MmCfgBase;

  Rsdp = (EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)(UINTN)AcpiTableBase;
  DEBUG ((DEBUG_INFO, "Rsdp at 0x%p\n", Rsdp));
  DEBUG ((DEBUG_INFO, "Rsdt at 0x%x, Xsdt at 0x%lx\n", Rsdp->RsdtAddress, Rsdp->XsdtAddress));

  //
  // Search Rsdt First
  //
  Fadt     = NULL;
  MmCfgHdr = NULL;
  Rsdt     = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)(Rsdp->RsdtAddress);
  if (Rsdt != NULL) {
    Entry32  = (UINT32 *)(Rsdt + 1);
    Entry32Num = (Rsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 2;
    for (Idx = 0; Idx < Entry32Num; Idx++) {
      if (*(UINT32 *)(UINTN)(Entry32[Idx]) == EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
        Fadt = (EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE *)(UINTN)(Entry32[Idx]);
        DEBUG ((DEBUG_INFO, "Found Fadt in Rsdt\n"));
      }

      if (*(UINT32 *)(UINTN)(Entry32[Idx]) == EFI_ACPI_5_0_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE) {
        MmCfgHdr = (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER *)(UINTN)(Entry32[Idx]);
        DEBUG ((DEBUG_INFO, "Found MM config address in Rsdt\n"));
      }

      if ((Fadt != NULL) && (MmCfgHdr != NULL)) {
        goto Done;
      }
    }
  }

  //
  // Search Xsdt Second
  //
  Xsdt     = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)(Rsdp->XsdtAddress);
  if (Xsdt != NULL) {
    Entry64  = (UINT64 *)(Xsdt + 1);
    Entry64Num = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 3;
    for (Idx = 0; Idx < Entry64Num; Idx++) {
      if (*(UINT32 *)(UINTN)(Entry64[Idx]) == EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
        Fadt = (EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE *)(UINTN)(Entry64[Idx]);
        DEBUG ((DEBUG_INFO, "Found Fadt in Xsdt\n"));
      }

      if (*(UINT32 *)(UINTN)(Entry64[Idx]) == EFI_ACPI_5_0_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE) {
        MmCfgHdr = (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER *)(UINTN)(Entry32[Idx]);
        DEBUG ((DEBUG_INFO, "Found MM config address in Xsdt\n"));
      }

      if ((Fadt != NULL) && (MmCfgHdr != NULL)) {
        goto Done;
      }
    }
  }

  if (Fadt == NULL) {
    return RETURN_NOT_FOUND;
  }

Done:

  AcpiBoardInfo->PmCtrlRegBase   = Fadt->Pm1aCntBlk;
  AcpiBoardInfo->PmTimerRegBase  = Fadt->PmTmrBlk;
  AcpiBoardInfo->ResetRegAddress = Fadt->ResetReg.Address;
  AcpiBoardInfo->ResetValue      = Fadt->ResetValue;
  AcpiBoardInfo->PmEvtBase       = Fadt->Pm1aEvtBlk;
  AcpiBoardInfo->PmGpeEnBase     = Fadt->Gpe0Blk + Fadt->Gpe0BlkLen / 2;

  if (MmCfgHdr != NULL) {
    MmCfgBase = (EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE *)((UINT8*) MmCfgHdr + sizeof (*MmCfgHdr));
    AcpiBoardInfo->PcieBaseAddress = MmCfgBase->BaseAddress;
  } else {
    AcpiBoardInfo->PcieBaseAddress = 0;
  }
  DEBUG ((DEBUG_INFO, "PmCtrl  Reg 0x%lx\n",  AcpiBoardInfo->PmCtrlRegBase));
  DEBUG ((DEBUG_INFO, "PmTimer Reg 0x%lx\n",  AcpiBoardInfo->PmTimerRegBase));
  DEBUG ((DEBUG_INFO, "Reset   Reg 0x%lx\n",  AcpiBoardInfo->ResetRegAddress));
  DEBUG ((DEBUG_INFO, "Reset   Value 0x%x\n", AcpiBoardInfo->ResetValue));
  DEBUG ((DEBUG_INFO, "PmEvt   Reg 0x%lx\n",  AcpiBoardInfo->PmEvtBase));
  DEBUG ((DEBUG_INFO, "PmGpeEn Reg 0x%lx\n",  AcpiBoardInfo->PmGpeEnBase));
  DEBUG ((DEBUG_INFO, "PcieBaseAddr 0x%lx\n", AcpiBoardInfo->PcieBaseAddress));

  //
  // Verify values for proper operation
  //
  ASSERT(Fadt->Pm1aCntBlk != 0);
  ASSERT(Fadt->PmTmrBlk != 0);
  ASSERT(Fadt->ResetReg.Address != 0);
  ASSERT(Fadt->Pm1aEvtBlk != 0);
  ASSERT(Fadt->Gpe0Blk != 0);

  DEBUG_CODE_BEGIN ();
    BOOLEAN    SciEnabled;

    //
    // Check the consistency of SCI enabling
    //

    //
    // Get SCI_EN value
    //
   if (Fadt->Pm1CntLen == 4) {
      SciEnabled = (IoRead32 (Fadt->Pm1aCntBlk) & BIT0)? TRUE : FALSE;
    } else {
      //
      // if (Pm1CntLen == 2), use 16 bit IO read;
      // if (Pm1CntLen != 2 && Pm1CntLen != 4), use 16 bit IO read as a fallback
      //
      SciEnabled = (IoRead16 (Fadt->Pm1aCntBlk) & BIT0)? TRUE : FALSE;
    }

    if (!(Fadt->Flags & EFI_ACPI_5_0_HW_REDUCED_ACPI) &&
        (Fadt->SmiCmd == 0) &&
       !SciEnabled) {
      //
      // The ACPI enabling status is inconsistent: SCI is not enabled but ACPI
      // table does not provide a means to enable it through FADT->SmiCmd
      //
      DEBUG ((DEBUG_ERROR, "ERROR: The ACPI enabling status is inconsistent: SCI is not"
        " enabled but the ACPI table does not provide a means to enable it through FADT->SmiCmd."
        " This may cause issues in OS.\n"));
    }
  DEBUG_CODE_END ();

  return RETURN_SUCCESS;
}

EFI_STATUS
MemInfoCallback (
  IN MEMROY_MAP_ENTRY             *MemoryMapEntry,
  IN VOID                         *Params
  )
{
  PAYLOAD_MEM_INFO        *MemInfo;
  UINTN                   Attribue;
  EFI_PHYSICAL_ADDRESS    Base;
  EFI_RESOURCE_TYPE       Type;
  UINT64                  Size;
  UINT32                  SystemLowMemTop;

  Attribue = EFI_RESOURCE_ATTRIBUTE_PRESENT |
             EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
             EFI_RESOURCE_ATTRIBUTE_TESTED |
             EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE;

  MemInfo = (PAYLOAD_MEM_INFO *)Params;
  Type    = (MemoryMapEntry->Type == 1) ? EFI_RESOURCE_SYSTEM_MEMORY : EFI_RESOURCE_MEMORY_RESERVED;
  Base    = MemoryMapEntry->Base;
  Size    = MemoryMapEntry->Size;

  if ((Base  < 0x100000) && ((Base + Size) > 0x100000)) {
    Size -= (0x100000 - Base);
    Base  = 0x100000;
  }

  if (Base >= 0x100000) {
    if (Type == EFI_RESOURCE_SYSTEM_MEMORY) {
      if (Base < 0x100000000ULL) {
        MemInfo->UsableLowMemTop = (UINT32)(Base + Size);
      } else {
        Attribue &= ~EFI_RESOURCE_ATTRIBUTE_TESTED;
      }
      BuildResourceDescriptorHob (
        EFI_RESOURCE_SYSTEM_MEMORY,
        Attribue,
        (EFI_PHYSICAL_ADDRESS)Base,
        Size
        );
    } else if (Type == EFI_RESOURCE_MEMORY_RESERVED) {
      BuildResourceDescriptorHob (
        EFI_RESOURCE_MEMORY_RESERVED,
        Attribue,
        (EFI_PHYSICAL_ADDRESS)Base,
        Size
        );
      if (Base < 0x100000000ULL) {
        SystemLowMemTop = ((UINT32)(Base + Size) + 0x0FFFFFFF) & 0xF0000000;
        if (SystemLowMemTop > MemInfo->SystemLowMemTop) {
          MemInfo->SystemLowMemTop = SystemLowMemTop;
        }
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  This is the entrypoint of PEIM

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
BlPeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES        **PeiServices
  )
{
  EFI_STATUS                       Status;
  UINT64                           LowMemorySize;
  UINT64                           PeiMemSize = SIZE_64MB;
  EFI_PHYSICAL_ADDRESS             PeiMemBase = 0;
  UINT32                           RegEax;
  UINT8                            PhysicalAddressBits;
  PAYLOAD_MEM_INFO                 PldMemInfo;
  SYSTEM_TABLE_INFO                SysTableInfo;
  SYSTEM_TABLE_INFO                *NewSysTableInfo;
  ACPI_BOARD_INFO                  AcpiBoardInfo;
  ACPI_BOARD_INFO                  *NewAcpiBoardInfo;
  EFI_PEI_GRAPHICS_INFO_HOB        GfxInfo;
  EFI_PEI_GRAPHICS_INFO_HOB        *NewGfxInfo;
  EFI_PEI_GRAPHICS_DEVICE_INFO_HOB GfxDeviceInfo;
  EFI_PEI_GRAPHICS_DEVICE_INFO_HOB *NewGfxDeviceInfo;


  //
  // Report lower 640KB of RAM. Attribute EFI_RESOURCE_ATTRIBUTE_TESTED
  // is intentionally omitted to prevent erasing of the coreboot header
  // record before it is processed by ParseMemoryInfo.
  //
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    (
    EFI_RESOURCE_ATTRIBUTE_PRESENT |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
    ),
    (EFI_PHYSICAL_ADDRESS)(0),
    (UINT64)(0xA0000)
    );

  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_RESERVED,
    (
    EFI_RESOURCE_ATTRIBUTE_PRESENT |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_TESTED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
    ),
    (EFI_PHYSICAL_ADDRESS)(0xA0000),
    (UINT64)(0x60000)
    );


  //
  // Parse memory info
  //
  ZeroMem (&PldMemInfo, sizeof(PldMemInfo));
  Status = ParseMemoryInfo (MemInfoCallback, &PldMemInfo);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // Install memory
  //
  LowMemorySize = PldMemInfo.UsableLowMemTop;
  PeiMemBase = (LowMemorySize - PeiMemSize) & (~(BASE_64KB - 1));
  DEBUG ((DEBUG_INFO, "Low memory 0x%lx\n", LowMemorySize));
  DEBUG ((DEBUG_INFO, "SystemLowMemTop 0x%x\n", PldMemInfo.SystemLowMemTop));
  DEBUG ((DEBUG_INFO, "PeiMemBase: 0x%lx.\n", PeiMemBase));
  DEBUG ((DEBUG_INFO, "PeiMemSize: 0x%lx.\n", PeiMemSize));
  Status = PeiServicesInstallPeiMemory (PeiMemBase, PeiMemSize);
  ASSERT_EFI_ERROR (Status);

  //
  // Set cache on the physical memory
  //
  MtrrSetMemoryAttribute (BASE_1MB, LowMemorySize - BASE_1MB, CacheWriteBack);
  MtrrSetMemoryAttribute (0, 0xA0000, CacheWriteBack);

  //
  // Create Memory Type Information HOB
  //
  BuildGuidDataHob (
    &gEfiMemoryTypeInformationGuid,
    mDefaultMemoryTypeInformation,
    sizeof(mDefaultMemoryTypeInformation)
    );

  //
  // Create Fv hob
  //
  PeiReportRemainedFvs ();

  BuildMemoryAllocationHob (
    PcdGet32 (PcdPayloadFdMemBase),
    PcdGet32 (PcdPayloadFdMemSize),
    EfiBootServicesData
    );

  //
  // Build CPU memory space and IO space hob
  //
  AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
  if (RegEax >= 0x80000008) {
    AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
    PhysicalAddressBits = (UINT8) RegEax;
  } else {
    PhysicalAddressBits  = 36;
  }

  //
  // Create a CPU hand-off information
  //
  BuildCpuHob (PhysicalAddressBits, 16);

  //
  // Report Local APIC range
  //
  BuildMemoryMappedIoRangeHob (0xFEC80000, SIZE_512KB);

  //
  // Boot mode
  //
  Status = PeiServicesSetBootMode (BOOT_WITH_FULL_CONFIGURATION);
  ASSERT_EFI_ERROR (Status);

  Status = PeiServicesInstallPpi (mPpiBootMode);
  ASSERT_EFI_ERROR (Status);

  //
  // Create guid hob for frame buffer information
  //
  Status = ParseGfxInfo (&GfxInfo);
  if (!EFI_ERROR (Status)) {
    NewGfxInfo = BuildGuidHob (&gEfiGraphicsInfoHobGuid, sizeof (GfxInfo));
    ASSERT (NewGfxInfo != NULL);
    CopyMem (NewGfxInfo, &GfxInfo, sizeof (GfxInfo));
    DEBUG ((DEBUG_INFO, "Created graphics info hob\n"));
  }


  Status = ParseGfxDeviceInfo (&GfxDeviceInfo);
  if (!EFI_ERROR (Status)) {
    NewGfxDeviceInfo = BuildGuidHob (&gEfiGraphicsDeviceInfoHobGuid, sizeof (GfxDeviceInfo));
    ASSERT (NewGfxDeviceInfo != NULL);
    CopyMem (NewGfxDeviceInfo, &GfxDeviceInfo, sizeof (GfxDeviceInfo));
    DEBUG ((DEBUG_INFO, "Created graphics device info hob\n"));
  }


  //
  // Create guid hob for system tables like acpi table and smbios table
  //
  Status = ParseSystemTable(&SysTableInfo);
  ASSERT_EFI_ERROR (Status);
  if (!EFI_ERROR (Status)) {
    NewSysTableInfo = BuildGuidHob (&gUefiSystemTableInfoGuid, sizeof (SYSTEM_TABLE_INFO));
    ASSERT (NewSysTableInfo != NULL);
    CopyMem (NewSysTableInfo, &SysTableInfo, sizeof (SYSTEM_TABLE_INFO));
    DEBUG ((DEBUG_INFO, "Detected Acpi Table at 0x%lx, length 0x%x\n", SysTableInfo.AcpiTableBase, SysTableInfo.AcpiTableSize));
    DEBUG ((DEBUG_INFO, "Detected Smbios Table at 0x%lx, length 0x%x\n", SysTableInfo.SmbiosTableBase, SysTableInfo.SmbiosTableSize));
  }

  //
  // Create guid hob for acpi board information
  //
  Status = ParseAcpiInfo (SysTableInfo.AcpiTableBase, &AcpiBoardInfo);
  ASSERT_EFI_ERROR (Status);
  if (!EFI_ERROR (Status)) {
    NewAcpiBoardInfo = BuildGuidHob (&gUefiAcpiBoardInfoGuid, sizeof (ACPI_BOARD_INFO));
    ASSERT (NewAcpiBoardInfo != NULL);
    CopyMem (NewAcpiBoardInfo, &AcpiBoardInfo, sizeof (ACPI_BOARD_INFO));
    DEBUG ((DEBUG_INFO, "Create acpi board info guid hob\n"));
  }

  //
  // Parse platform specific information.
  //
  Status = ParsePlatformInfo ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error when parsing platform info, Status = %r\n", Status));
    return Status;
  }

  //
  // Mask off all legacy 8259 interrupt sources
  //
  IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, 0xFF);
  IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE,  0xFF);

  return EFI_SUCCESS;
}

