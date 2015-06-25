/** @file
  This PEIM will parse coreboot table in memory and report resource information into pei core.
  This file contains the main entrypoint of the PEIM.

Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include "CbSupportPei.h"

#define LEGACY_8259_MASK_REGISTER_MASTER  0x21
#define LEGACY_8259_MASK_REGISTER_SLAVE   0xA1

EFI_MEMORY_TYPE_INFORMATION mDefaultMemoryTypeInformation[] = {
  { EfiACPIReclaimMemory,   0x008 },
  { EfiACPIMemoryNVS,       0x004 },
  { EfiReservedMemoryType,  0x004 },
  { EfiRuntimeServicesData, 0x080 },
  { EfiRuntimeServicesCode, 0x080 },
  { EfiMaxMemoryType,       0     }
};

EFI_PEI_PPI_DESCRIPTOR   mPpiBootMode[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiMasterBootModePpiGuid,
    NULL
  }
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
    return FALSE;
  }

  return TRUE;
}

/**
  Install FvInfo PPI and create fv hobs for remained fvs

**/
VOID
CbPeiReportRemainedFvs (
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
        DEBUG((EFI_D_ERROR, "Found one valid fv : 0x%lx.\n", TempPtr, ((EFI_FIRMWARE_VOLUME_HEADER* )TempPtr)->FvLength));

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
  This is the entrypoint of PEIM

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
CbPeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS Status;
  UINT64 LowMemorySize, HighMemorySize;
  UINT64 PeiMemSize = SIZE_64MB;   // 64 MB
  EFI_PHYSICAL_ADDRESS PeiMemBase = 0;
  UINT32               RegEax;
  UINT8                PhysicalAddressBits;
  VOID*                pCbHeader;
  VOID*                pAcpiTable;
  UINT32               AcpiTableSize;
  VOID*                pSmbiosTable;
  UINT32               SmbiosTableSize;
  SYSTEM_TABLE_INFO*   pSystemTableInfo;
  FRAME_BUFFER_INFO    FbInfo;
  FRAME_BUFFER_INFO*   pFbInfo;
  ACPI_BOARD_INFO*     pAcpiBoardInfo;
  UINTN                PmCtrlRegBase, PmTimerRegBase, ResetRegAddress, ResetValue;
  UINTN                PmEvtBase;
  UINTN                PmGpeEnBase;

  LowMemorySize = 0;
  HighMemorySize = 0;

  Status = CbParseMemoryInfo (&LowMemorySize, &HighMemorySize);
  if (EFI_ERROR(Status))
    return Status;

  DEBUG((EFI_D_ERROR, "LowMemorySize: 0x%lx.\n", LowMemorySize));
  DEBUG((EFI_D_ERROR, "HighMemorySize: 0x%lx.\n", HighMemorySize));

  ASSERT (LowMemorySize > 0);

  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    (
    EFI_RESOURCE_ATTRIBUTE_PRESENT |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_TESTED |
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

   BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    (
       EFI_RESOURCE_ATTRIBUTE_PRESENT |
       EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
       EFI_RESOURCE_ATTRIBUTE_TESTED |
       EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
       EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
       EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
       EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
    ),
    (EFI_PHYSICAL_ADDRESS)(0x100000),
    (UINT64) (LowMemorySize - 0x100000)
    );

  if (HighMemorySize > 0) {
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
    (EFI_PHYSICAL_ADDRESS)(0x100000000ULL),
    HighMemorySize
    );
  }

  //
  // Should be 64k aligned
  //
  PeiMemBase = (LowMemorySize - PeiMemSize) & (~(BASE_64KB - 1));

  DEBUG((EFI_D_ERROR, "PeiMemBase: 0x%lx.\n", PeiMemBase));
  DEBUG((EFI_D_ERROR, "PeiMemSize: 0x%lx.\n", PeiMemSize));

  Status = PeiServicesInstallPeiMemory (
         PeiMemBase,
         PeiMemSize
         );
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
  CbPeiReportRemainedFvs ();

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
  // Set pcd to save the upper coreboot header in case the dxecore will
  // erase 0~4k memory
  //
  pCbHeader = NULL;
  if ((CbParseGetCbHeader (1, &pCbHeader) == RETURN_SUCCESS)
    && ((UINTN)pCbHeader > BASE_4KB)) {
    DEBUG((EFI_D_ERROR, "Actual Coreboot header: %p.\n", pCbHeader));
    PcdSet32 (PcdCbHeaderPointer, (UINT32)(UINTN)pCbHeader);
  }

  //
  // Create guid hob for system tables like acpi table and smbios table
  //
  pAcpiTable = NULL;
  AcpiTableSize = 0;
  pSmbiosTable = NULL;
  SmbiosTableSize = 0;
  Status = CbParseAcpiTable (&pAcpiTable, &AcpiTableSize);
  if (EFI_ERROR (Status)) {
    // ACPI table is oblidgible
    DEBUG ((EFI_D_ERROR, "Failed to find the required acpi table\n"));
    ASSERT (FALSE);
  }
  CbParseSmbiosTable (&pSmbiosTable, &SmbiosTableSize);

  pSystemTableInfo = NULL;
  pSystemTableInfo = BuildGuidHob (&gUefiSystemTableInfoGuid, sizeof (SYSTEM_TABLE_INFO));
  ASSERT (pSystemTableInfo != NULL);
  pSystemTableInfo->AcpiTableBase = (UINT64) (UINTN)pAcpiTable;
  pSystemTableInfo->AcpiTableSize = AcpiTableSize;
  pSystemTableInfo->SmbiosTableBase = (UINT64) (UINTN)pSmbiosTable;
  pSystemTableInfo->SmbiosTableSize = SmbiosTableSize;
  DEBUG ((EFI_D_ERROR, "Detected Acpi Table at 0x%lx, length 0x%x\n", pSystemTableInfo->AcpiTableBase, pSystemTableInfo->AcpiTableSize));
  DEBUG ((EFI_D_ERROR, "Detected Smbios Table at 0x%lx, length 0x%x\n", pSystemTableInfo->SmbiosTableBase, pSystemTableInfo->SmbiosTableSize));
  DEBUG ((EFI_D_ERROR, "Create system table info guid hob\n"));

  //
  // Create guid hob for acpi board information
  //
  Status = CbParseFadtInfo (&PmCtrlRegBase, &PmTimerRegBase, &ResetRegAddress, &ResetValue, &PmEvtBase, &PmGpeEnBase);
  ASSERT_EFI_ERROR (Status);
  pAcpiBoardInfo = NULL;
  pAcpiBoardInfo = BuildGuidHob (&gUefiAcpiBoardInfoGuid, sizeof (ACPI_BOARD_INFO));
  ASSERT (pAcpiBoardInfo != NULL);
  pAcpiBoardInfo->PmCtrlRegBase = (UINT64)PmCtrlRegBase;
  pAcpiBoardInfo->PmTimerRegBase = (UINT64)PmTimerRegBase;
  pAcpiBoardInfo->ResetRegAddress = (UINT64)ResetRegAddress;
  pAcpiBoardInfo->ResetValue = (UINT8)ResetValue;
  pAcpiBoardInfo->PmEvtBase = (UINT64)PmEvtBase;
  pAcpiBoardInfo->PmGpeEnBase = (UINT64)PmGpeEnBase;
  DEBUG ((EFI_D_ERROR, "Create acpi board info guid hob\n"));

  //
  // Create guid hob for frame buffer information
  //
  ZeroMem (&FbInfo, sizeof (FRAME_BUFFER_INFO));
  Status = CbParseFbInfo (&FbInfo);
  if (!EFI_ERROR (Status)) {
    pFbInfo = BuildGuidHob (&gUefiFrameBufferInfoGuid, sizeof (FRAME_BUFFER_INFO));
    ASSERT (pSystemTableInfo != NULL);
    CopyMem (pFbInfo, &FbInfo, sizeof (FRAME_BUFFER_INFO));
    DEBUG ((EFI_D_ERROR, "Create frame buffer info guid hob\n"));
  }

  //
  // Mask off all legacy 8259 interrupt sources
  //
  IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, 0xFF);
  IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE,  0xFF);

  return EFI_SUCCESS;
}

