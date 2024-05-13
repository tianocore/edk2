/** @file

  SPI Host controller entry point for SMM

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/SpiHcPlatformLib.h>
#include <Protocol/SpiSmmHc.h>
#include <IndustryStandard/SpiNorFlashJedecSfdp.h>
#include "SpiHc.h"

EFI_HANDLE  mSpiHcHandle = 0;

/**
  Entry point of the SPI Host Controller driver. Installs the EFI_SPI_HC_PROTOCOL on mSpiHcHandle.
  Also installs the EFI_DEVICE_PATH_PROTOCOL corresponding to the SPI Host controller on the same
  mSpiHcHandle.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  Pointer to standard EFI system table.

  @retval EFI_SUCCESS       Succeed.
  @retval EFI_OUT_RESOURCES If the system has run out of memory
**/
EFI_STATUS
EFIAPI
SpiHcProtocolEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS           Status;
  EFI_SPI_HC_PROTOCOL  *HcProtocol;

  DEBUG ((DEBUG_VERBOSE, "%a - ENTRY\n", __func__));

  // Allocate the SPI Host Controller protocol
  HcProtocol = AllocateZeroPool (sizeof (EFI_SPI_HC_PROTOCOL));
  ASSERT (HcProtocol != NULL);
  if (HcProtocol == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Fill in the SPI Host Controller Protocol
  Status = GetPlatformSpiHcDetails (
             &HcProtocol->Attributes,
             &HcProtocol->FrameSizeSupportMask,
             &HcProtocol->MaximumTransferBytes
             );

  HcProtocol->ChipSelect  = ChipSelect;
  HcProtocol->Clock       = Clock;
  HcProtocol->Transaction = Transaction;

  Status = gMmst->MmInstallProtocolInterface (
                    &mSpiHcHandle,
                    &gEfiSpiSmmHcProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    HcProtocol
                    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_VERBOSE, "Error installing gEfiSpiSmmHcProtocolGuid\n"));
  }

  DEBUG ((DEBUG_VERBOSE, "%a - EXIT Status=%r\n", __func__, Status));

  return Status;
}
