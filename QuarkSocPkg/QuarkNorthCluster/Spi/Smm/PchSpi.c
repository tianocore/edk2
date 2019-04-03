/** @file

PCH SPI SMM Driver implements the SPI Host Controller Compatibility Interface.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/
#include "PchSpi.h"

SPI_INSTANCE          *mSpiInstance;

CONST UINT32  mSpiRegister[] = {
    R_QNC_RCRB_SPIS,
    R_QNC_RCRB_SPIPREOP,
    R_QNC_RCRB_SPIOPMENU,
    R_QNC_RCRB_SPIOPMENU + 4
  };

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
  EFI_STATUS    Status;

  //
  // Allocate pool for SPI protocol instance
  //
  Status = gSmst->SmmAllocatePool (
                    EfiRuntimeServicesData, // MemoryType don't care
                    sizeof (SPI_INSTANCE),
                    (VOID **) &mSpiInstance
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  if (mSpiInstance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  ZeroMem ((VOID *) mSpiInstance, sizeof (SPI_INSTANCE));
  //
  // Initialize the SPI protocol instance
  //
  Status = SpiProtocolConstructor (mSpiInstance);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install the SMM EFI_SPI_PROTOCOL interface
  //
  Status = gSmst->SmmInstallProtocolInterface (
            &(mSpiInstance->Handle),
            &gEfiSmmSpiProtocolGuid,
            EFI_NATIVE_INTERFACE,
            &(mSpiInstance->SpiProtocol)
            );
  if (EFI_ERROR (Status)) {
    gSmst->SmmFreePool (mSpiInstance);
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

VOID
EFIAPI
SpiPhaseInit (
  VOID
  )
/*++
Routine Description:

  This function is a a hook for Spi Smm phase specific initialization

Arguments:

  None

Returns:

  None

--*/
{
  UINTN  Index;

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
