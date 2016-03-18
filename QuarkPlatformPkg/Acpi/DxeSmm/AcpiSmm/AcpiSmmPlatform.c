/** @file
ACPISMM Driver implementation file.

This is QNC Smm platform driver

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include <AcpiSmmPlatform.h>

#define PCILIB_TO_COMMON_ADDRESS(Address) \
        ((UINT64) ((((UINTN) ((Address>>20) & 0xff)) << 24) + (((UINTN) ((Address>>15) & 0x1f)) << 16) + (((UINTN) ((Address>>12) & 0x07)) << 8) + ((UINTN) (Address & 0xfff ))))

//
// Modular variables needed by this driver
//
EFI_ACPI_SMM_DEV                 mAcpiSmm;

UINT8  mPciCfgRegTable[] = {
  //
  // Logic to decode the table masks to arrive at the registers saved
  // Dword Registers are saved. For a given mask, the Base+offset register
  // will be saved as in the table below.
  // (example) To save register 0x24, 0x28 the mask at the Base 0x20 will be 0x06
  //     Base      0x00   0x20   0x40  0x60  0x80  0xA0  0xC0  0xE0
  // Mask  offset
  // 0x01   0x00
  // 0x02   0x04
  // 0x04   0x08
  // 0x08   0x0C
  // 0x10   0x10
  // 0x20   0x14
  // 0x40   0x18
  // 0x80   0x1C
  //

  //
  // Bus,   Dev,  Func,
  // 00-1F, 20-3F, 40-5F, 60-7F, 80-9F, A0-BF, C0-DF, E0-FF
  // Only Bus 0 device is supported now
  //

  //
  // Quark South Cluster devices
  //
  PCI_DEVICE   (0, 20, 0),
  PCI_REG_MASK (0x18, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),

  PCI_DEVICE   (0, 20, 1),
  PCI_REG_MASK (0x38, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),

  PCI_DEVICE   (0, 20, 2),
  PCI_REG_MASK (0x18, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),

  PCI_DEVICE   (0, 20, 3),
  PCI_REG_MASK (0x18, 0x98, 0x00, 0x01, 0x00, 0x00, 0x03, 0x00),

  PCI_DEVICE   (0, 20, 4),
  PCI_REG_MASK (0x18, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),

  PCI_DEVICE   (0, 20, 5),
  PCI_REG_MASK (0x38, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),

  PCI_DEVICE   (0, 20, 6),
  PCI_REG_MASK (0x18, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),

  PCI_DEVICE   (0, 20, 7),
  PCI_REG_MASK (0x18, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),

  PCI_DEVICE   (0, 21, 0),
  PCI_REG_MASK (0x18, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),

  PCI_DEVICE   (0, 21, 1),
  PCI_REG_MASK (0x18, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),

  PCI_DEVICE   (0, 21, 2),
  PCI_REG_MASK (0x38, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),

  //
  // Quark North Cluster devices
  //
  PCI_DEVICE   (0, 0, 0),
  PCI_REG_MASK (0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),

  PCI_DEVICE   (0, 23, 0),
  PCI_REG_MASK (0xC0, 0x8F, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00),

  PCI_DEVICE   (0, 23, 1),
  PCI_REG_MASK (0xC0, 0x8F, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00),

  PCI_DEVICE   (0, 31, 0),
  PCI_REG_MASK (0x00, 0x08, 0x4E, 0x03, 0x02, 0x00, 0x60, 0x10),

  PCI_DEVICE_END
};

EFI_PLATFORM_TYPE                         mPlatformType;

  // These registers have to set in byte order
const UINT8  QNCS3SaveExtReg[] = {
    QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HSMMC, // SMRAM settings

    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR0+QUARK_NC_MEMORY_MANAGER_IMRXL,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR0+QUARK_NC_MEMORY_MANAGER_IMRXH,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR0+QUARK_NC_MEMORY_MANAGER_IMRXRM,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR0+QUARK_NC_MEMORY_MANAGER_IMRXWM,

    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR1+QUARK_NC_MEMORY_MANAGER_IMRXL,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR1+QUARK_NC_MEMORY_MANAGER_IMRXH,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR1+QUARK_NC_MEMORY_MANAGER_IMRXRM,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR1+QUARK_NC_MEMORY_MANAGER_IMRXWM,

    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR2+QUARK_NC_MEMORY_MANAGER_IMRXL,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR2+QUARK_NC_MEMORY_MANAGER_IMRXH,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR2+QUARK_NC_MEMORY_MANAGER_IMRXRM,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR2+QUARK_NC_MEMORY_MANAGER_IMRXWM,

    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR3+QUARK_NC_MEMORY_MANAGER_IMRXL,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR3+QUARK_NC_MEMORY_MANAGER_IMRXH,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR3+QUARK_NC_MEMORY_MANAGER_IMRXRM,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR3+QUARK_NC_MEMORY_MANAGER_IMRXWM,

    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR4+QUARK_NC_MEMORY_MANAGER_IMRXL,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR4+QUARK_NC_MEMORY_MANAGER_IMRXH,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR4+QUARK_NC_MEMORY_MANAGER_IMRXRM,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR4+QUARK_NC_MEMORY_MANAGER_IMRXWM,

    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR5+QUARK_NC_MEMORY_MANAGER_IMRXL,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR5+QUARK_NC_MEMORY_MANAGER_IMRXH,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR5+QUARK_NC_MEMORY_MANAGER_IMRXRM,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR5+QUARK_NC_MEMORY_MANAGER_IMRXWM,

    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR6+QUARK_NC_MEMORY_MANAGER_IMRXL,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR6+QUARK_NC_MEMORY_MANAGER_IMRXH,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR6+QUARK_NC_MEMORY_MANAGER_IMRXRM,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR6+QUARK_NC_MEMORY_MANAGER_IMRXWM,

    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR7+QUARK_NC_MEMORY_MANAGER_IMRXL,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR7+QUARK_NC_MEMORY_MANAGER_IMRXH,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR7+QUARK_NC_MEMORY_MANAGER_IMRXRM,
    QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR7+QUARK_NC_MEMORY_MANAGER_IMRXWM,

    QUARK_NC_RMU_SB_PORT_ID, QUARK_NC_ECC_SCRUB_END_MEM_REG, // ECC Scrub settings
    QUARK_NC_RMU_SB_PORT_ID, QUARK_NC_ECC_SCRUB_START_MEM_REG,
    QUARK_NC_RMU_SB_PORT_ID, QUARK_NC_ECC_SCRUB_NEXT_READ_REG,
    QUARK_NC_RMU_SB_PORT_ID, QUARK_NC_ECC_SCRUB_CONFIG_REG,

    0xFF
    };

/**
  Allocate EfiACPIMemoryNVS below 4G memory address.

  This function allocates EfiACPIMemoryNVS below 4G memory address.

  @param Size   Size of memory to allocate.

  @return       Allocated address for output.

**/
VOID*
AllocateAcpiNvsMemoryBelow4G (
  IN UINTN  Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID*                 Buffer;

  Pages = EFI_SIZE_TO_PAGES (Size);
  Address = 0xffffffff;

  Status  = gBS->AllocatePages (
                   AllocateMaxAddress,
                   EfiACPIMemoryNVS,
                   Pages,
                   &Address
                   );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Buffer = (VOID *) (UINTN) Address;
  ZeroMem (Buffer, Size);

  return Buffer;
}

EFI_STATUS
EFIAPI
ReservedS3Memory (
  UINTN  SystemMemoryLength

  )
/*++

Routine Description:

  Reserved S3 memory for InstallS3Memory

Arguments:


Returns:

  EFI_OUT_OF_RESOURCES  -  Insufficient resources to complete function.
  EFI_SUCCESS           -  Function has completed successfully.

--*/
{

  VOID                                      *GuidHob;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK            *DescriptorBlock;
  VOID                                      *AcpiReservedBase;

  UINTN                                     TsegIndex;
  UINTN                                     TsegSize;
  UINTN                                     TsegBase;
  RESERVED_ACPI_S3_RANGE                    *AcpiS3Range;
  //
  // Get Hob list for SMRAM desc
  //
  GuidHob    = GetFirstGuidHob (&gEfiSmmPeiSmramMemoryReserveGuid);
  ASSERT (GuidHob);
  DescriptorBlock = GET_GUID_HOB_DATA (GuidHob);
  ASSERT (DescriptorBlock);

  //
  // Use the hob to get SMRAM capabilities
  //
  TsegIndex = DescriptorBlock->NumberOfSmmReservedRegions - 1;
  ASSERT (TsegIndex <= (MAX_SMRAM_RANGES - 1));
  TsegBase  = (UINTN)DescriptorBlock->Descriptor[TsegIndex].PhysicalStart;
  TsegSize  = (UINTN)DescriptorBlock->Descriptor[TsegIndex].PhysicalSize;

  DEBUG ((EFI_D_INFO, "SMM  Base: %08X\n", TsegBase));
  DEBUG ((EFI_D_INFO, "SMM  Size: %08X\n", TsegSize));

  //
  // Now find the location of the data structure that is used to store the address
  // of the S3 reserved memory.
  //
  AcpiS3Range = (RESERVED_ACPI_S3_RANGE*) (UINTN) (TsegBase + RESERVED_ACPI_S3_RANGE_OFFSET);

  //
  // Allocate reserved ACPI memory for S3 resume.  Pointer to this region is
  // stored in SMRAM in the first page of TSEG.
  //
  AcpiReservedBase = AllocateAcpiNvsMemoryBelow4G (PcdGet32 (PcdS3AcpiReservedMemorySize));
  if (AcpiReservedBase != NULL) {
    AcpiS3Range->AcpiReservedMemoryBase = (UINT32)(UINTN) AcpiReservedBase;
    AcpiS3Range->AcpiReservedMemorySize = PcdGet32 (PcdS3AcpiReservedMemorySize);
  }
  AcpiS3Range->SystemMemoryLength = (UINT32)SystemMemoryLength;

  DEBUG ((EFI_D_INFO, "S3 Memory  Base:    %08X\n", AcpiS3Range->AcpiReservedMemoryBase));
  DEBUG ((EFI_D_INFO, "S3 Memory  Size:    %08X\n", AcpiS3Range->AcpiReservedMemorySize));
  DEBUG ((EFI_D_INFO, "S3 SysMemoryLength: %08X\n", AcpiS3Range->SystemMemoryLength));

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
InitAcpiSmmPlatform (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:

  Initializes the SMM S3 Handler Driver.

Arguments:

  ImageHandle  -  The image handle of Sleep State Wake driver.
  SystemTable  -  The starndard EFI system table.

Returns:

  EFI_OUT_OF_RESOURCES  -  Insufficient resources to complete function.
  EFI_SUCCESS           -  Function has completed successfully.
  Other                 -  Error occured during execution.

--*/
{
  EFI_STATUS                      Status;
  EFI_GLOBAL_NVS_AREA_PROTOCOL    *AcpiNvsProtocol = NULL;
  UINTN                           MemoryLength;
  EFI_PEI_HOB_POINTERS            Hob;

  Status = gBS->LocateProtocol (
                  &gEfiGlobalNvsAreaProtocolGuid,
                  NULL,
                  (VOID **) &AcpiNvsProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  mAcpiSmm.BootScriptSaved  = 0;

  mPlatformType = (EFI_PLATFORM_TYPE)PcdGet16 (PcdPlatformType);

  //
  // Calculate the system memory length by memory hobs
  //
  MemoryLength  = 0x100000;
  Hob.Raw = GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);
  ASSERT (Hob.Raw != NULL);
  while ((Hob.Raw != NULL) && (!END_OF_HOB_LIST (Hob))) {
    if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
      //
      // Skip the memory region below 1MB
      //
      if (Hob.ResourceDescriptor->PhysicalStart >= 0x100000) {
        MemoryLength += (UINTN)Hob.ResourceDescriptor->ResourceLength;
      }
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw);
  }

  ReservedS3Memory(MemoryLength);

  //
  // Locate and Register to Parent driver
  //
  Status = RegisterToDispatchDriver ();
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

EFI_STATUS
RegisterToDispatchDriver (
  VOID
  )
/*++

Routine Description:

  Register to dispatch driver.

Arguments:

  None.

Returns:

  EFI_SUCCESS  -  Successfully init the device.
  Other        -  Error occured whening calling Dxe lib functions.

--*/
{
  UINTN                         Length;
  EFI_STATUS                    Status;
  EFI_SMM_SX_DISPATCH2_PROTOCOL  *SxDispatch;
  EFI_SMM_SW_DISPATCH2_PROTOCOL  *SwDispatch;
  EFI_SMM_SX_REGISTER_CONTEXT   *EntryDispatchContext;
  EFI_SMM_SX_REGISTER_CONTEXT   *EntryS1DispatchContext;
  EFI_SMM_SX_REGISTER_CONTEXT   *EntryS3DispatchContext;
  EFI_SMM_SX_REGISTER_CONTEXT   *EntryS4DispatchContext;
  EFI_SMM_SX_REGISTER_CONTEXT   *EntryS5DispatchContext;
  EFI_SMM_SW_REGISTER_CONTEXT   *SwContext;
  EFI_SMM_SW_REGISTER_CONTEXT   *AcpiDisableSwContext;
  EFI_SMM_SW_REGISTER_CONTEXT   *AcpiEnableSwContext;

  Status = gSmst->SmmLocateProtocol (
                  &gEfiSmmSxDispatch2ProtocolGuid,
                  NULL,
                  (VOID **) &SxDispatch
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gSmst->SmmLocateProtocol (
                  &gEfiSmmSwDispatch2ProtocolGuid,
                  NULL,
                  (VOID **) &SwDispatch
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Length = sizeof (EFI_SMM_SX_REGISTER_CONTEXT) * 4 + sizeof (EFI_SMM_SW_REGISTER_CONTEXT) * 2;
  Status = gSmst->SmmAllocatePool (
                      EfiRuntimeServicesData,
                      Length,
                      (VOID **) &EntryDispatchContext
                      );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SetMem (EntryDispatchContext, Length, 0);

  EntryS1DispatchContext  = EntryDispatchContext++;
  EntryS3DispatchContext  = EntryDispatchContext++;
  EntryS4DispatchContext  = EntryDispatchContext++;
  EntryS5DispatchContext  = EntryDispatchContext++;

  SwContext = (EFI_SMM_SW_REGISTER_CONTEXT *)EntryDispatchContext;
  AcpiDisableSwContext = SwContext++;
  AcpiEnableSwContext  = SwContext++;

  //
  // Register the enable handler
  //
  AcpiEnableSwContext->SwSmiInputValue = EFI_ACPI_ACPI_ENABLE;
  Status = SwDispatch->Register (
                        SwDispatch,
                        EnableAcpiCallback,
                        AcpiEnableSwContext,
                        &(mAcpiSmm.DisableAcpiHandle)
                        );

  //
  // Register the disable handler
  //
  AcpiDisableSwContext->SwSmiInputValue = EFI_ACPI_ACPI_DISABLE;
  Status = SwDispatch->Register (
                        SwDispatch,
                        DisableAcpiCallback,
                        AcpiDisableSwContext,
                        &(mAcpiSmm.EnableAcpiHandle)
                        );


  //
  // Register entry phase call back function for S1
  //
  EntryS1DispatchContext->Type  = SxS1;
  EntryS1DispatchContext->Phase = SxEntry;
  Status = SxDispatch->Register (
                        SxDispatch,
                        SxSleepEntryCallBack,
                        EntryS1DispatchContext,
                        &(mAcpiSmm.S1SleepEntryHandle)
                        );

  //
  // Register entry phase call back function
  //
  EntryS3DispatchContext->Type  = SxS3;
  EntryS3DispatchContext->Phase = SxEntry;
  Status = SxDispatch->Register (
                        SxDispatch,
                        SxSleepEntryCallBack,
                        EntryS3DispatchContext,
                        &(mAcpiSmm.S3SleepEntryHandle)
                        );

  //
  // Register entry phase call back function for S4
  //
  EntryS4DispatchContext->Type  = SxS4;
  EntryS4DispatchContext->Phase = SxEntry;
  Status = SxDispatch->Register (
                        SxDispatch,
                        SxSleepEntryCallBack,
                        EntryS4DispatchContext,
                        &(mAcpiSmm.S4SleepEntryHandle)
                        );

  //
  // Register callback for S5 in order to workaround the LAN shutdown issue
  //
  EntryS5DispatchContext->Type  = SxS5;
  EntryS5DispatchContext->Phase = SxEntry;
  Status = SxDispatch->Register (
                        SxDispatch,
                        SxSleepEntryCallBack,
                        EntryS5DispatchContext,
                        &(mAcpiSmm.S5SoftOffEntryHandle)
                        );

  return Status;
}


EFI_STATUS
RestoreQncS3SwCallback (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  CONST VOID                    *DispatchContext,
  IN  OUT VOID                      *CommBuffer,
  IN  OUT UINTN                     *CommBufferSize
  )
/*++

Routine Description:
  SMI handler to retore QncS3 code & context for S3 path
  This will be only triggered when BootScript got executed during resume

Arguments:
  DispatchHandle  - EFI Handle
  DispatchContext - Pointer to the EFI_SMM_SW_DISPATCH_CONTEXT

Returns:
  Nothing

--*/
{
  //
  // Restore to original address by default
  //
  RestoreLockBox(&gQncS3CodeInLockBoxGuid, NULL, NULL);
  RestoreLockBox(&gQncS3ContextInLockBoxGuid, NULL, NULL);
  return EFI_SUCCESS;
}

EFI_STATUS
DisableAcpiCallback (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  CONST VOID                    *DispatchContext,
  IN  OUT VOID                      *CommBuffer,
  IN  OUT UINTN                     *CommBufferSize
  )
/*++

Routine Description:
  SMI handler to disable ACPI mode

  Dispatched on reads from APM port with value 0xA1

  ACPI events are disabled and ACPI event status is cleared.
  SCI mode is then disabled.
   Clear all ACPI event status and disable all ACPI events
   Disable PM sources except power button
   Clear status bits
   Disable GPE0 sources
   Clear status bits
   Disable GPE1 sources
   Clear status bits
   Disable SCI

Arguments:
  DispatchHandle  - EFI Handle
  DispatchContext - Pointer to the EFI_SMM_SW_DISPATCH_CONTEXT

Returns:
  Nothing

--*/
{
  EFI_STATUS  Status;
  UINT16      Pm1Cnt;

  Status = GetAllQncPmBase (gSmst);
  ASSERT_EFI_ERROR (Status);
  Pm1Cnt = IoRead16 (mAcpiSmm.QncPmBase + R_QNC_PM1BLK_PM1C);

  //
  // Disable SCI
  //
  Pm1Cnt &= ~B_QNC_PM1BLK_PM1C_SCIEN;

  IoWrite16 (mAcpiSmm.QncPmBase + R_QNC_PM1BLK_PM1C, Pm1Cnt);

  return EFI_SUCCESS;
}

EFI_STATUS
EnableAcpiCallback (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  CONST VOID                    *DispatchContext,
  IN  OUT VOID                      *CommBuffer,
  IN  OUT UINTN                     *CommBufferSize
  )
/*++

Routine Description:
  SMI handler to enable ACPI mode

  Dispatched on reads from APM port with value 0xA0

  Disables the SW SMI Timer.
  ACPI events are disabled and ACPI event status is cleared.
  SCI mode is then enabled.

   Disable SW SMI Timer

   Clear all ACPI event status and disable all ACPI events
   Disable PM sources except power button
   Clear status bits

   Disable GPE0 sources
   Clear status bits

   Disable GPE1 sources
   Clear status bits

   Guarantee day-of-month alarm is invalid (ACPI 1.0 section 4.7.2.4)

   Enable SCI

Arguments:
  DispatchHandle  - EFI Handle
  DispatchContext - Pointer to the EFI_SMM_SW_DISPATCH_CONTEXT

Returns:
  Nothing

--*/
{
  EFI_STATUS  Status;
  UINT32      SmiEn;
  UINT16      Pm1Cnt;
  UINT8       Data8;

  Status  = GetAllQncPmBase (gSmst);
  ASSERT_EFI_ERROR (Status);

  SmiEn = IoRead32 (mAcpiSmm.QncGpe0Base + R_QNC_GPE0BLK_SMIE);

  //
  // Disable SW SMI Timer
  //
  SmiEn &= ~(B_QNC_GPE0BLK_SMIE_SWT);
  IoWrite32 (mAcpiSmm.QncGpe0Base + R_QNC_GPE0BLK_SMIE, SmiEn);

  //
  // Guarantee day-of-month alarm is invalid (ACPI 1.0 section 4.7.2.4)
  //
  Data8 = RTC_ADDRESS_REGISTER_D;
  IoWrite8 (R_IOPORT_CMOS_STANDARD_INDEX, Data8);
  Data8 = 0x0;
  IoWrite8 (R_IOPORT_CMOS_STANDARD_DATA, Data8);

  //
  // Enable SCI
  //
  Pm1Cnt = IoRead16 (mAcpiSmm.QncPmBase + R_QNC_PM1BLK_PM1C);
  Pm1Cnt |= B_QNC_PM1BLK_PM1C_SCIEN;
  IoWrite16 (mAcpiSmm.QncPmBase + R_QNC_PM1BLK_PM1C, Pm1Cnt);

  //
  // Do platform specific stuff for ACPI enable SMI
  //


  return EFI_SUCCESS;
}

EFI_STATUS
SxSleepEntryCallBack (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  CONST VOID                    *DispatchContext,
  IN  OUT VOID                      *CommBuffer,
  IN  OUT UINTN                     *CommBufferSize
  )
/*++

Routine Description:

  Callback function entry for Sx sleep state.

Arguments:

  DispatchHandle   -  The handle of this callback, obtained when registering.
  DispatchContext  -  The predefined context which contained sleep type and phase.

Returns:

  EFI_SUCCESS            -  Operation successfully performed.
  EFI_INVALID_PARAMETER  -  Invalid parameter passed in.

--*/
{
  EFI_STATUS  Status;
  UINT8       Data8;
  UINT16      Data16;
  UINT32      Data32;

  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, PcdGet32 (PcdProgressCodeS3SuspendStart));

  //
  // Reget QNC power mgmr regs base in case of OS changing it at runtime
  //
  Status  = GetAllQncPmBase (gSmst);

  //
  // Clear RTC Alarm (if set)
  //
  Data8 = RTC_ADDRESS_REGISTER_C;
  IoWrite8 (R_IOPORT_CMOS_STANDARD_INDEX, Data8);
  Data8 = IoRead8 (R_IOPORT_CMOS_STANDARD_DATA);

  //
  // Clear all ACPI status bits
  //
  Data32 = B_QNC_GPE0BLK_GPE0S_ALL;
  Status = gSmst->SmmIo.Io.Write( &gSmst->SmmIo, SMM_IO_UINT32, mAcpiSmm.QncGpe0Base + R_QNC_GPE0BLK_GPE0S, 1, &Data32 );
  Data16 = B_QNC_PM1BLK_PM1S_ALL;
  Status = gSmst->SmmIo.Io.Write( &gSmst->SmmIo, SMM_IO_UINT16, mAcpiSmm.QncPmBase + R_QNC_PM1BLK_PM1S, 1, &Data16 );

  //
  // Handling S1 - setting appropriate wake bits in GPE0_EN
  //
  if ((DispatchHandle == mAcpiSmm.S1SleepEntryHandle) && (((EFI_SMM_SX_REGISTER_CONTEXT *)DispatchContext)->Type == SxS1)) {
    //
    // Enable bit13 (EGPE), 14 (GPIO) ,17 (PCIE) in GPE0_EN
    //
    Status = gSmst->SmmIo.Io.Read( &gSmst->SmmIo, SMM_IO_UINT32, mAcpiSmm.QncGpe0Base + R_QNC_GPE0BLK_GPE0E, 1, &Data32 );
    Data32 |= (B_QNC_GPE0BLK_GPE0E_EGPE | B_QNC_GPE0BLK_GPE0E_GPIO | B_QNC_GPE0BLK_GPE0E_PCIE);
    Status = gSmst->SmmIo.Io.Write( &gSmst->SmmIo, SMM_IO_UINT32, mAcpiSmm.QncGpe0Base + R_QNC_GPE0BLK_GPE0E, 1, &Data32 );

    //
    // Enable bit10 (RTC) in PM1E
    //
    Status = gSmst->SmmIo.Io.Read( &gSmst->SmmIo, SMM_IO_UINT16, mAcpiSmm.QncPmBase + R_QNC_PM1BLK_PM1E, 1, &Data16 );
    Data16 |= B_QNC_PM1BLK_PM1E_RTC;
    Status = gSmst->SmmIo.Io.Write( &gSmst->SmmIo, SMM_IO_UINT16, mAcpiSmm.QncPmBase + R_QNC_PM1BLK_PM1E, 1, &Data16 );

    return EFI_SUCCESS;
  }

  //
  // Handling S4, S5 and WOL - setting appropriate wake bits in GPE0_EN
  //
  if (((DispatchHandle == mAcpiSmm.S4SleepEntryHandle) && (((EFI_SMM_SX_REGISTER_CONTEXT *)DispatchContext)->Type == SxS4)) ||
      ((DispatchHandle == mAcpiSmm.S5SoftOffEntryHandle) && (((EFI_SMM_SX_REGISTER_CONTEXT *)DispatchContext)->Type == SxS5))
     ) {
    //
    // Enable bit13 (EGPE), 14 (GPIO) ,17 (PCIE) in GPE0_EN
    // Enable the WOL bits in GPE0_EN reg here for PME
    //
    Status = gSmst->SmmIo.Io.Read( &gSmst->SmmIo, SMM_IO_UINT32, mAcpiSmm.QncGpe0Base + R_QNC_GPE0BLK_GPE0E, 1, &Data32 );
    Data32 |= (B_QNC_GPE0BLK_GPE0E_EGPE | B_QNC_GPE0BLK_GPE0E_GPIO | B_QNC_GPE0BLK_GPE0E_PCIE);
    Status = gSmst->SmmIo.Io.Write( &gSmst->SmmIo, SMM_IO_UINT32, mAcpiSmm.QncGpe0Base + R_QNC_GPE0BLK_GPE0E, 1, &Data32 );

    //
    // Enable bit10 (RTC) in PM1E
    //
    Status = gSmst->SmmIo.Io.Read( &gSmst->SmmIo, SMM_IO_UINT16, mAcpiSmm.QncPmBase + R_QNC_PM1BLK_PM1E, 1, &Data16 );
    Data16 |= B_QNC_PM1BLK_PM1E_RTC;
    Status = gSmst->SmmIo.Io.Write( &gSmst->SmmIo, SMM_IO_UINT16, mAcpiSmm.QncPmBase + R_QNC_PM1BLK_PM1E, 1, &Data16 );

  } else {

    if ((DispatchHandle != mAcpiSmm.S3SleepEntryHandle) || (((EFI_SMM_SX_REGISTER_CONTEXT *)DispatchContext)->Type != SxS3)) {
      return EFI_INVALID_PARAMETER;
    }

    Status  = SaveRuntimeScriptTable (gSmst);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Enable bit13 (EGPE), 14 (GPIO), 17 (PCIE) in GPE0_EN
    // Enable the WOL bits in GPE0_EN reg here for PME
    //
    Status = gSmst->SmmIo.Io.Read( &gSmst->SmmIo, SMM_IO_UINT32, mAcpiSmm.QncGpe0Base + R_QNC_GPE0BLK_GPE0E, 1, &Data32 );
    Data32 |= (B_QNC_GPE0BLK_GPE0E_EGPE | B_QNC_GPE0BLK_GPE0E_GPIO | B_QNC_GPE0BLK_GPE0E_PCIE);
    Status = gSmst->SmmIo.Io.Write( &gSmst->SmmIo, SMM_IO_UINT32, mAcpiSmm.QncGpe0Base + R_QNC_GPE0BLK_GPE0E, 1, &Data32 );

    //
    // Enable bit10 (RTC) in PM1E
    //
    Status = gSmst->SmmIo.Io.Read( &gSmst->SmmIo, SMM_IO_UINT16, mAcpiSmm.QncPmBase + R_QNC_PM1BLK_PM1E, 1, &Data16 );
    Data16 |= B_QNC_PM1BLK_PM1E_RTC;
    Status = gSmst->SmmIo.Io.Write( &gSmst->SmmIo, SMM_IO_UINT16, mAcpiSmm.QncPmBase + R_QNC_PM1BLK_PM1E, 1, &Data16 );
  }

  //
  // When entering a power-managed state like S3,
  // PERST# must be asserted in advance of power-off.
  //
  PlatformPERSTAssert (mPlatformType);

  return EFI_SUCCESS;
}

EFI_STATUS
GetAllQncPmBase (
  IN EFI_SMM_SYSTEM_TABLE2       *Smst
  )
/*++

Routine Description:

  Get QNC chipset LPC Power Management I/O Base at runtime.

Arguments:

  Smst  -  The standard SMM system table.

Returns:

  EFI_SUCCESS  -  Successfully init the device.
  Other        -  Error occured whening calling Dxe lib functions.

--*/
{
  mAcpiSmm.QncPmBase    = PciRead16 (PCI_LIB_ADDRESS(PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, 0, R_QNC_LPC_PM1BLK)) & B_QNC_LPC_PM1BLK_MASK;
  mAcpiSmm.QncGpe0Base  = PciRead16 (PCI_LIB_ADDRESS(PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, 0, R_QNC_LPC_GPE0BLK)) & B_QNC_LPC_GPE0BLK_MASK;

  //
  // Quark does not support Changing Primary SoC IOBARs from what was
  // setup in SEC/PEI UEFI stages.
  //
  ASSERT (mAcpiSmm.QncPmBase == (UINT32) PcdGet16 (PcdPm1blkIoBaseAddress));
  ASSERT (mAcpiSmm.QncGpe0Base == (UINT32) PcdGet16 (PcdGpe0blkIoBaseAddress));
  return EFI_SUCCESS;
}

EFI_STATUS
SaveRuntimeScriptTable (
  IN EFI_SMM_SYSTEM_TABLE2       *Smst
  )
{
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS  PciAddress;
  UINT32                Data32;
  UINT16                Data16;
  UINT8                 Mask;
  UINTN                 Index;
  UINTN                 Offset;
  UINT16                DeviceId;

  //
  // Check what Soc we are running on (read Host bridge DeviceId)
  //
  DeviceId = QncGetSocDeviceId();

  //
  // Save PCI-Host bridge settings (0, 0, 0). 0x90, 94 and 9c are changed by CSM
  // and vital to S3 resume. That's why we put save code here
  //
  Index = 0;
  while (mPciCfgRegTable[Index] != PCI_DEVICE_END) {

    PciAddress.Bus              = mPciCfgRegTable[Index++];
    PciAddress.Device           = mPciCfgRegTable[Index++];
    PciAddress.Function         = mPciCfgRegTable[Index++];
    PciAddress.Register         = 0;
    PciAddress.ExtendedRegister = 0;

    Data16 = PciRead16 (PCI_LIB_ADDRESS(PciAddress.Bus, PciAddress.Device, PciAddress.Function, PciAddress.Register));
    if (Data16 == 0xFFFF) {
      Index += 8;
      continue;
    }

    for (Offset = 0, Mask = 0x01; Offset < 256; Offset += 4, Mask <<= 1) {

      if (Mask == 0x00) {
        Mask = 0x01;
      }

      if (mPciCfgRegTable[Index + Offset / 32] & Mask) {

        PciAddress.Register = (UINT8) Offset;
        Data32 = PciRead32 (PCI_LIB_ADDRESS(PciAddress.Bus, PciAddress.Device, PciAddress.Function, PciAddress.Register));


        //
        // Save latest settings to runtime script table
        //
        S3BootScriptSavePciCfgWrite (
             S3BootScriptWidthUint32,
             PCILIB_TO_COMMON_ADDRESS (PCI_LIB_ADDRESS(PciAddress.Bus, PciAddress.Device, PciAddress.Function, PciAddress.Register)),
             1,
             &Data32
             );
      }
    }

    Index += 8;

  }

  //
  // Save message bus registers
  //
  Index = 0;
  while (QNCS3SaveExtReg[Index] != 0xFF) {
    Data32 = QNCPortRead (QNCS3SaveExtReg[Index], QNCS3SaveExtReg[Index + 1]);

    //
    // Save IMR settings with IMR protection disabled initially
    // HMBOUND and IMRs will be locked just before jumping to the OS waking vector
    //
    if (QNCS3SaveExtReg[Index] == QUARK_NC_MEMORY_MANAGER_SB_PORT_ID) {
      if ((QNCS3SaveExtReg[Index + 1] >= (QUARK_NC_MEMORY_MANAGER_IMR0+QUARK_NC_MEMORY_MANAGER_IMRXL)) && (QNCS3SaveExtReg[Index + 1] <= (QUARK_NC_MEMORY_MANAGER_IMR7+QUARK_NC_MEMORY_MANAGER_IMRXWM)) && ((QNCS3SaveExtReg[Index + 1] & 0x03) == QUARK_NC_MEMORY_MANAGER_IMRXL)) {
        Data32 &= ~IMR_LOCK;
        if (DeviceId == QUARK2_MC_DEVICE_ID) {
          Data32 &= ~IMR_EN;
        }
      }
      if ((QNCS3SaveExtReg[Index + 1] >= (QUARK_NC_MEMORY_MANAGER_IMR0+QUARK_NC_MEMORY_MANAGER_IMRXRM)) && (QNCS3SaveExtReg[Index + 1] <= (QUARK_NC_MEMORY_MANAGER_IMR7+QUARK_NC_MEMORY_MANAGER_IMRXWM)) && ((QNCS3SaveExtReg[Index + 1] & 0x03) >= QUARK_NC_MEMORY_MANAGER_IMRXRM)) {
        Data32 = (UINT32)IMRX_ALL_ACCESS;
      }
    }

    //
    // Save latest settings to runtime script table
    //
    S3BootScriptSavePciCfgWrite (
      S3BootScriptWidthUint32,
      PCILIB_TO_COMMON_ADDRESS (PCI_LIB_ADDRESS(0, 0, 0, QNC_ACCESS_PORT_MDR)),
      1,
      &Data32
     );

    Data32 = MESSAGE_WRITE_DW (QNCS3SaveExtReg[Index], QNCS3SaveExtReg[Index + 1]);

    S3BootScriptSavePciCfgWrite (
      S3BootScriptWidthUint32,
      PCILIB_TO_COMMON_ADDRESS (PCI_LIB_ADDRESS(0, 0, 0, QNC_ACCESS_PORT_MCR)),
      1,
      &Data32
     );
    Index += 2;
  }

  Index = 0;
  while (QNCS3SaveExtReg[Index] != 0xFF) {
    //
    // Save IMR settings with IMR protection enabled (above script was to handle restoring all settings first - now we want to enable)
    //
    if (QNCS3SaveExtReg[Index] == QUARK_NC_MEMORY_MANAGER_SB_PORT_ID) {
      if (DeviceId == QUARK2_MC_DEVICE_ID) {
        if ((QNCS3SaveExtReg[Index + 1] >= (QUARK_NC_MEMORY_MANAGER_IMR0+QUARK_NC_MEMORY_MANAGER_IMRXL)) && (QNCS3SaveExtReg[Index + 1] <= (QUARK_NC_MEMORY_MANAGER_IMR7+QUARK_NC_MEMORY_MANAGER_IMRXWM)) && ((QNCS3SaveExtReg[Index + 1] & 0x03) == QUARK_NC_MEMORY_MANAGER_IMRXL)) {
          Data32 = QNCPortRead (QNCS3SaveExtReg[Index], QNCS3SaveExtReg[Index + 1]);
          Data32 &= ~IMR_LOCK;

          //
          // Save latest settings to runtime script table
          //
          S3BootScriptSavePciCfgWrite (
            S3BootScriptWidthUint32,
            PCILIB_TO_COMMON_ADDRESS (PCI_LIB_ADDRESS(0, 0, 0, QNC_ACCESS_PORT_MDR)),
            1,
            &Data32
          );

          Data32 = MESSAGE_WRITE_DW (QNCS3SaveExtReg[Index], QNCS3SaveExtReg[Index + 1]);

          S3BootScriptSavePciCfgWrite (
            S3BootScriptWidthUint32,
            PCILIB_TO_COMMON_ADDRESS (PCI_LIB_ADDRESS(0, 0, 0, QNC_ACCESS_PORT_MCR)),
            1,
            &Data32
          );
        }
      } else {
        if ((QNCS3SaveExtReg[Index + 1] >= (QUARK_NC_MEMORY_MANAGER_IMR0+QUARK_NC_MEMORY_MANAGER_IMRXRM)) && (QNCS3SaveExtReg[Index + 1] <= (QUARK_NC_MEMORY_MANAGER_IMR7+QUARK_NC_MEMORY_MANAGER_IMRXWM)) && ((QNCS3SaveExtReg[Index + 1] & 0x03) >= QUARK_NC_MEMORY_MANAGER_IMRXRM)) {
          Data32 = QNCPortRead (QNCS3SaveExtReg[Index], QNCS3SaveExtReg[Index + 1]);

          //
          // Save latest settings to runtime script table
          //
          S3BootScriptSavePciCfgWrite (
            S3BootScriptWidthUint32,
            PCILIB_TO_COMMON_ADDRESS (PCI_LIB_ADDRESS(0, 0, 0, QNC_ACCESS_PORT_MDR)),
            1,
            &Data32
          );

          Data32 = MESSAGE_WRITE_DW (QNCS3SaveExtReg[Index], QNCS3SaveExtReg[Index + 1]);

          S3BootScriptSavePciCfgWrite (
            S3BootScriptWidthUint32,
            PCILIB_TO_COMMON_ADDRESS (PCI_LIB_ADDRESS(0, 0, 0, QNC_ACCESS_PORT_MCR)),
            1,
            &Data32
          );
        }
      }
    }
    Index += 2;
  }

  // Check if ECC scrub enabled and need re-enabling on resume
  // All scrub related configuration registers are saved on suspend
  // as part of QNCS3SaveExtReg configuration table script.
  // The code below extends the S3 resume script with scrub reactivation
  // message (if needed only)
  Data32 = QNCPortRead (QUARK_NC_RMU_SB_PORT_ID, QUARK_NC_ECC_SCRUB_CONFIG_REG);
  if( 0 != (Data32 & SCRUB_CFG_ACTIVE)) {

      Data32 = SCRUB_RESUME_MSG();

      S3BootScriptSavePciCfgWrite (
        S3BootScriptWidthUint32,
        PCILIB_TO_COMMON_ADDRESS (PCI_LIB_ADDRESS(0, 0, 0, QNC_ACCESS_PORT_MCR)),
        1,
        &Data32
       );
  }

  //
  // Save I/O ports to S3 script table
  //

  //
  // Important to trap Sx for SMM
  //
  Data32 = IoRead32 (mAcpiSmm.QncGpe0Base + R_QNC_GPE0BLK_SMIE);
  S3BootScriptSaveIoWrite(S3BootScriptWidthUint32, (mAcpiSmm.QncGpe0Base + R_QNC_GPE0BLK_SMIE), 1, &Data32);

  return EFI_SUCCESS;
}

