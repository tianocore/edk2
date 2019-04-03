/** @file
QuarkNcSocId module initialization module

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "CommonHeader.h"

#include "LegacyRegion.h"
#include "DxeQNCSmbus.h"

#include "QNCInit.h"

//
// Definitions
//
#define QNC_RESERVED_ITEM_IO         0
#define QNC_RESERVED_ITEM_MEMORYIO   1
#define DXE_DEVICE_DISABLED 0
#define DXE_DEVICE_ENABLED 1

typedef struct _QNC_SPACE_TABLE_ITEM {
   UINTN                   IoOrMemory;
   UINTN                   Type;
   EFI_PHYSICAL_ADDRESS    BaseAddress;
   UINT64                  Length;
   UINTN                   Alignment;
   BOOLEAN                 RuntimeOrNot;
} QNC_SPACE_TABLE_ITEM;

typedef struct {
  ACPI_CPU_DATA       AcpuCpuData;
  MTRR_SETTINGS       MtrrTable;
  IA32_DESCRIPTOR     GdtrProfile;
  IA32_DESCRIPTOR     IdtrProfile;
  CPU_REGISTER_TABLE  RegisterTable;
  CPU_REGISTER_TABLE  PreSmmInitRegisterTable;
} ACPI_CPU_DATA_EX;

//
// Spaces to be reserved in GCD
// Expand it to add more
//
const QNC_SPACE_TABLE_ITEM  mQNCReservedSpaceTable[] = {
  {
    QNC_RESERVED_ITEM_MEMORYIO,
    EfiGcdMemoryTypeMemoryMappedIo,
    FixedPcdGet64 (PcdIoApicBaseAddress),
    FixedPcdGet64 (PcdIoApicSize),
    0,
    FALSE
  },
  {
    QNC_RESERVED_ITEM_MEMORYIO,
    EfiGcdMemoryTypeMemoryMappedIo,
    FixedPcdGet64 (PcdHpetBaseAddress),
    FixedPcdGet64 (PcdHpetSize),
    0,
    FALSE
  }
};

//
// Global variable for ImageHandle of QNCInit driver
//
EFI_HANDLE gQNCInitImageHandle;
QNC_DEVICE_ENABLES    mQNCDeviceEnables;


VOID
QNCInitializeResource (
  VOID
  );

EFI_STATUS
InitializeQNCPolicy (
  VOID
  );

/**
  Allocate EfiACPIMemoryNVS below 4G memory address.

  This function allocates EfiACPIMemoryNVS below 4G memory address.

  @param Size   Size of memory to allocate.

  @return       Allocated address for output.

**/
VOID *
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

/**
  Prepare ACPI NVS memory below 4G memory for use of S3 resume.

  This function allocates ACPI NVS memory below 4G memory for use of S3 resume,
  and saves data into the memory region.

**/
VOID
SaveCpuS3Data (
  VOID
  )
{
  EFI_STATUS        Status;
  ACPI_CPU_DATA_EX  *AcpiCpuDataEx;
  ACPI_CPU_DATA     *AcpiCpuData;
  UINTN             GdtSize;
  UINTN             IdtSize;
  VOID              *Gdt;
  VOID              *Idt;

  //
  // Allocate ACPI NVS memory below 4G memory for use of S3 resume.
  //
  AcpiCpuDataEx = AllocateAcpiNvsMemoryBelow4G (sizeof (ACPI_CPU_DATA_EX));
  AcpiCpuData = &AcpiCpuDataEx->AcpuCpuData;

  //
  //
  //
  AcpiCpuData->NumberOfCpus              = 1;
  AcpiCpuData->StackSize                 = PcdGet32 (PcdCpuApStackSize);
  AcpiCpuData->ApMachineCheckHandlerBase = 0;
  AcpiCpuData->ApMachineCheckHandlerSize = 0;
  AcpiCpuData->GdtrProfile               = (EFI_PHYSICAL_ADDRESS) (UINTN) &AcpiCpuDataEx->GdtrProfile;
  AcpiCpuData->IdtrProfile               = (EFI_PHYSICAL_ADDRESS) (UINTN) &AcpiCpuDataEx->IdtrProfile;
  AcpiCpuData->MtrrTable                 = (EFI_PHYSICAL_ADDRESS) (UINTN) &AcpiCpuDataEx->MtrrTable;
  AcpiCpuData->RegisterTable             = (EFI_PHYSICAL_ADDRESS) (UINTN) &AcpiCpuDataEx->RegisterTable;
  AcpiCpuData->PreSmmInitRegisterTable   = (EFI_PHYSICAL_ADDRESS) (UINTN) &AcpiCpuDataEx->PreSmmInitRegisterTable;

  //
  // Allocate stack space for all CPUs
  //
  AcpiCpuData->StackAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) AllocateAcpiNvsMemoryBelow4G (AcpiCpuData->NumberOfCpus * AcpiCpuData->StackSize);

  //
  // Get MTRR settings from currently executing CPU
  //
  MtrrGetAllMtrrs (&AcpiCpuDataEx->MtrrTable);

  //
  // Get the BSP's data of GDT and IDT
  //
  AsmReadGdtr ((IA32_DESCRIPTOR *) &AcpiCpuDataEx->GdtrProfile);
  AsmReadIdtr ((IA32_DESCRIPTOR *) &AcpiCpuDataEx->IdtrProfile);

  //
  // Allocate GDT and IDT in ACPI NVS and copy in current GDT and IDT contents
  //
  GdtSize = AcpiCpuDataEx->GdtrProfile.Limit + 1;
  IdtSize = AcpiCpuDataEx->IdtrProfile.Limit + 1;
  Gdt = AllocateAcpiNvsMemoryBelow4G (GdtSize + IdtSize);
  Idt = (VOID *)((UINTN)Gdt + GdtSize);
  CopyMem (Gdt, (VOID *)AcpiCpuDataEx->GdtrProfile.Base, GdtSize);
  CopyMem (Idt, (VOID *)AcpiCpuDataEx->IdtrProfile.Base, IdtSize);
  AcpiCpuDataEx->GdtrProfile.Base = (UINTN)Gdt;
  AcpiCpuDataEx->IdtrProfile.Base = (UINTN)Idt;

  //
  // No RegisterTable entries
  //
  AcpiCpuDataEx->RegisterTable.TableLength = 0;

  //
  // No PreSmmInitRegisterTable entries
  //
  AcpiCpuDataEx->PreSmmInitRegisterTable.TableLength = 0;

  //
  // Set the base address of CPU S3 data to PcdCpuS3DataAddress
  //
  Status = PcdSet64S (PcdCpuS3DataAddress, (UINT64)(UINTN)AcpiCpuData);
  ASSERT_EFI_ERROR (Status);
}

/**
   The entry function for QNCInit driver.

   This function just call initialization function for PciHostBridge,
   LegacyRegion and QNCSmmAccess module.

   @param ImageHandle   The driver image handle for GmchInit driver
   @param SystemTable   The pointer to System Table

   @retval EFI_SUCCESS  Success to initialize every module for GMCH driver.
   @return EFI_STATUS   The status of initialization work.

**/
EFI_STATUS
EFIAPI
QNCInit (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;

  S3BootScriptSaveInformationAsciiString (
    "QNCInitDxeEntryBegin"
    );

  gQNCInitImageHandle = ImageHandle;

  mQNCDeviceEnables.Uint32 = PcdGet32 (PcdDeviceEnables);


  //
  // Initialize PCIE root ports
  //
  Status = QncInitRootPorts ();
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "QNC Root Port initialization is failed!\n"));
    return Status;
  }

  Status = LegacyRegionInit ();
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "QNC LegacyRegion initialization is failed!\n"));
    return Status;
  }


  Status = InitializeQNCPolicy ();
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "QNC Policy initialization is failed!\n"));
    return Status;
  }

  Status = InitializeQNCSmbus (ImageHandle,SystemTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "QNC Smbus driver is failed!\n"));
    return Status;
  }

  QNCInitializeResource ();

  SaveCpuS3Data ();

  S3BootScriptSaveInformationAsciiString (
    "QNCInitDxeEntryEnd"
    );

  return EFI_SUCCESS;
}


/**
  Reserve I/O or memory space in GCD

  @param  IoOrMemory    Switch of I/O or memory.
  @param  GcdType       Type of the space.
  @param  BaseAddress   Base address of the space.
  @param  Length        Length of the space.
  @param  Alignment     Align with 2^Alignment
  @param  RuntimeOrNot  For runtime usage or not
  @param  ImageHandle   Handle for the image of this driver.

  @retval EFI_SUCCESS   Reserve successful
**/
EFI_STATUS
QNCReserveSpaceInGcd(
  IN UINTN                 IoOrMemory,
  IN UINTN                 GcdType,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINTN                 Alignment,
  IN BOOLEAN               RuntimeOrNot,
  IN EFI_HANDLE            ImageHandle
  )
{
  EFI_STATUS               Status;

  if (IoOrMemory == QNC_RESERVED_ITEM_MEMORYIO) {
    Status = gDS->AddMemorySpace (
                    GcdType,
                    BaseAddress,
                    Length,
                    EFI_MEMORY_UC
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        EFI_D_ERROR,
        "Failed to add memory space :0x%x 0x%x\n",
        BaseAddress,
        Length
        ));
    }
    ASSERT_EFI_ERROR (Status);
    Status = gDS->AllocateMemorySpace (
                    EfiGcdAllocateAddress,
                    GcdType,
                    Alignment,
                    Length,
                    &BaseAddress,
                    ImageHandle,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
    if (RuntimeOrNot) {
      Status = gDS->SetMemorySpaceAttributes (
                      BaseAddress,
                      Length,
                      EFI_MEMORY_RUNTIME | EFI_MEMORY_UC
                     );
      ASSERT_EFI_ERROR (Status);
    }
  } else {
    Status = gDS->AddIoSpace (
                    GcdType,
                    BaseAddress,
                    Length
                    );
    ASSERT_EFI_ERROR (Status);
    Status = gDS->AllocateIoSpace (
                    EfiGcdAllocateAddress,
                    GcdType,
                    Alignment,
                    Length,
                    &BaseAddress,
                    ImageHandle,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }
  return Status;
}


/**
  Initialize the memory and io resource which belong to QNC.
  1) Report and allocate all BAR's memory to GCD.
  2) Report PCI memory and I/O space to GCD.
  3) Set memory attribute for <1M memory space.
**/
VOID
QNCInitializeResource (
  )
{
  EFI_PHYSICAL_ADDRESS            BaseAddress;
  EFI_STATUS                      Status;
  UINT64                          ExtraRegionLength;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR Descriptor;
  UINTN                           Index;

  // Report TSEG range
  // This range maybe has been reportted in PEI phase via Resource Hob.
  //
  QNCGetTSEGMemoryRange (&BaseAddress, &ExtraRegionLength);
  if (ExtraRegionLength != 0) {
    Status = gDS->GetMemorySpaceDescriptor (BaseAddress, &Descriptor);
    if (Status == EFI_NOT_FOUND) {
      Status = gDS->AddMemorySpace (
                      EfiGcdMemoryTypeReserved,
                      BaseAddress,
                      ExtraRegionLength,
                      EFI_MEMORY_UC
                      );
    }
  }

  //
  // < 1M resource setting. The memory ranges <1M has been added into GCD via
  // resource hob produced by PEI phase. Here will set memory attribute of these
  // ranges for DXE phase.
  //

  //
  // Dos Area (0 ~ 0x9FFFFh)
  //
  Status = gDS->GetMemorySpaceDescriptor (0, &Descriptor);
  DEBUG ((
    EFI_D_INFO,
    "DOS Area Memory: base = 0x%x, length = 0x%x, attribute = 0x%x\n",
    Descriptor.BaseAddress,
    Descriptor.Length,
    Descriptor.Attributes
    ));
  ASSERT_EFI_ERROR (Status);
  Status = gDS->SetMemorySpaceAttributes(
                  0,
                  0xA0000,
                  EFI_MEMORY_WB
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Default SMRAM UnCachable until SMBASE relocated.
  //
  Status = gDS->SetMemorySpaceAttributes(
                  0x30000,
                  0x10000,
                  EFI_MEMORY_UC
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Default SMM ABSEG area. (0xA0000 ~ 0xBFFFF)
  //
  Status = gDS->GetMemorySpaceDescriptor (0xA0000, &Descriptor);
  DEBUG ((
    EFI_D_INFO,
    "ABSEG Memory: base = 0x%x, length = 0x%x, attribute = 0x%x\n",
    Descriptor.BaseAddress,
    Descriptor.Length,
    Descriptor.Attributes
    ));
  ASSERT_EFI_ERROR (Status);
  Status = gDS->SetMemorySpaceAttributes(
                  0xA0000,
                  0x20000,
                  EFI_MEMORY_UC
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Expansion BIOS area.
  //
  Status = gDS->GetMemorySpaceDescriptor (0xC0000, &Descriptor);
  DEBUG ((
    EFI_D_INFO,
    "Memory base = 0x%x, length = 0x%x, attribute = 0x%x\n",
    Descriptor.BaseAddress,
    Descriptor.Length,
    Descriptor.Attributes
    ));
  ASSERT_EFI_ERROR (Status);
  Status = gDS->SetMemorySpaceAttributes(
                  0xC0000,
                  0x30000,
                  EFI_MEMORY_UC
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Report other IO resources from mQNCReservedSpaceTable in GCD
  //
  for (Index = 0; Index < sizeof (mQNCReservedSpaceTable) / sizeof (QNC_SPACE_TABLE_ITEM); Index++) {
    Status = QNCReserveSpaceInGcd (
               mQNCReservedSpaceTable[Index].IoOrMemory,
               mQNCReservedSpaceTable[Index].Type,
               mQNCReservedSpaceTable[Index].BaseAddress,
               mQNCReservedSpaceTable[Index].Length,
               mQNCReservedSpaceTable[Index].Alignment,
               mQNCReservedSpaceTable[Index].RuntimeOrNot,
               gQNCInitImageHandle
               );
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Report unused PCIe config space as reserved.
  //
  if (PcdGet64 (PcdPciExpressSize) < SIZE_256MB) {
    Status = QNCReserveSpaceInGcd (
               QNC_RESERVED_ITEM_MEMORYIO,
               EfiGcdMemoryTypeMemoryMappedIo,
               (PcdGet64(PcdPciExpressBaseAddress) + PcdGet64(PcdPciExpressSize)),
               (SIZE_256MB - PcdGet64(PcdPciExpressSize)),
               0,
               FALSE,
               gQNCInitImageHandle
               );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Use the platform PCD to initialize devices in the QNC

  @param  ImageHandle   Handle for the image of this driver.
  @retval EFI_SUCCESS   Initialize successful
**/
EFI_STATUS
InitializeQNCPolicy (
  )
{
  UINT32       PciD31F0RegBase;  // LPC

  PciD31F0RegBase = PciDeviceMmBase (PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, PCI_FUNCTION_NUMBER_QNC_LPC);

  //
  // Disable for smbus
  //
  if (mQNCDeviceEnables.Bits.Smbus == DXE_DEVICE_DISABLED) {
    S3MmioAnd32 (PciD31F0RegBase + R_QNC_LPC_SMBUS_BASE, (~B_QNC_LPC_SMBUS_BASE_EN));
  }

  return EFI_SUCCESS;
}
