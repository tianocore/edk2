/** @file
PCH SPI Runtime Driver implements the SPI Host Controller Compatibility Interface.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "PchSpi.h"

extern EFI_GUID gEfiEventVirtualAddressChangeGuid;

//
// Global variables
//
SPI_INSTANCE  *mSpiInstance;
CONST UINT32 mSpiRegister[] = {
  R_QNC_RCRB_SPIS,
  R_QNC_RCRB_SPIPREOP,
  R_QNC_RCRB_SPIOPMENU,
  R_QNC_RCRB_SPIOPMENU + 4
  };

//
// Function implementations
//
VOID
PchSpiVirtualddressChangeEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
/*++

Routine Description:

  Fixup internal data pointers so that the services can be called in virtual mode.

Arguments:

  Event     The event registered.
  Context   Event context. Not used in this event handler.

Returns:

  None.

--*/
{
  gRT->ConvertPointer (EFI_INTERNAL_POINTER, (VOID *) &(mSpiInstance->PchRootComplexBar));
  gRT->ConvertPointer (EFI_INTERNAL_POINTER, (VOID *) &(mSpiInstance->SpiProtocol.Init));
  gRT->ConvertPointer (EFI_INTERNAL_POINTER, (VOID *) &(mSpiInstance->SpiProtocol.Lock));
  gRT->ConvertPointer (EFI_INTERNAL_POINTER, (VOID *) &(mSpiInstance->SpiProtocol.Execute));
  gRT->ConvertPointer (EFI_INTERNAL_POINTER, (VOID *) &(mSpiInstance));
}

EFI_STATUS
EFIAPI
InstallPchSpi (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_SYSTEM_TABLE      *SystemTable
  )
/*++

Routine Description:

  Entry point for the SPI host controller driver.

Arguments:

  ImageHandle       Image handle of this driver.
  SystemTable       Global system service table.

Returns:

  EFI_SUCCESS           Initialization complete.
  EFI_UNSUPPORTED       The chipset is unsupported by this driver.
  EFI_OUT_OF_RESOURCES  Do not have enough resources to initialize the driver.
  EFI_DEVICE_ERROR      Device error, driver exits abnormally.

--*/
{
  EFI_STATUS                      Status;
  UINT64                          BaseAddress;
  UINT64                          Length;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR GcdMemorySpaceDescriptor;
  UINT64                          Attributes;
  EFI_EVENT                       Event;

  DEBUG ((DEBUG_INFO, "InstallPchSpi() Start\n"));

  //
  // Allocate Runtime memory for the SPI protocol instance.
  //
  mSpiInstance = AllocateRuntimeZeroPool (sizeof (SPI_INSTANCE));
  if (mSpiInstance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Initialize the SPI protocol instance
  //
  Status = SpiProtocolConstructor (mSpiInstance);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Install the EFI_SPI_PROTOCOL interface
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &(mSpiInstance->Handle),
                  &gEfiSpiProtocolGuid,
                  &(mSpiInstance->SpiProtocol),
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    FreePool (mSpiInstance);
    return EFI_DEVICE_ERROR;
  }
  //
  // Set RCBA space in GCD to be RUNTIME so that the range will be supported in
  // virtual address mode in EFI aware OS runtime.
  // It will assert if RCBA Memory Space is not allocated
  // The caller is responsible for the existence and allocation of the RCBA Memory Spaces
  //
  BaseAddress = (EFI_PHYSICAL_ADDRESS) (mSpiInstance->PchRootComplexBar);
  Length      = PcdGet64 (PcdRcbaMmioSize);

  Status      = gDS->GetMemorySpaceDescriptor (BaseAddress, &GcdMemorySpaceDescriptor);
  ASSERT_EFI_ERROR (Status);

  Attributes = GcdMemorySpaceDescriptor.Attributes | EFI_MEMORY_RUNTIME;

  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeMemoryMappedIo,
                  BaseAddress,
                  Length,
                  EFI_MEMORY_RUNTIME | EFI_MEMORY_UC
                  );
  ASSERT_EFI_ERROR(Status);

  Status = gDS->SetMemorySpaceAttributes (
                  BaseAddress,
                  Length,
                  Attributes
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  PchSpiVirtualddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "InstallPchSpi() End\n"));

  return EFI_SUCCESS;
}

VOID
EFIAPI
SpiPhaseInit (
  VOID
  )
/*++
Routine Description:

  This function is a a hook for Spi Dxe phase specific initialization

Arguments:

  None

Returns:

  None

--*/
{
  UINTN  Index;

  //
  // Disable SMM BIOS write protect if it's not a SMM protocol
  //
  MmioAnd8 (
    PciDeviceMmBase (PCI_BUS_NUMBER_QNC,
    PCI_DEVICE_NUMBER_QNC_LPC,
    PCI_FUNCTION_NUMBER_QNC_LPC) + R_QNC_LPC_BIOS_CNTL,
    (UINT8) (~B_QNC_LPC_BIOS_CNTL_SMM_BWP)
    );

    //
    // Save SPI Registers for S3 resume usage
    //
    for (Index = 0; Index < sizeof (mSpiRegister) / sizeof (UINT32); Index++) {
    S3BootScriptSaveMemWrite (
      S3BootScriptWidthUint32,
        (UINTN) (mSpiInstance->PchRootComplexBar + mSpiRegister[Index]),
        1,
        (VOID *) (UINTN) (mSpiInstance->PchRootComplexBar + mSpiRegister[Index])
        );
    }
}
