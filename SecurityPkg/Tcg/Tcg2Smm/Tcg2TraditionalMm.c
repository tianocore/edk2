/** @file
  TCG2 SMM driver that updates TPM2 items in ACPI table and registers
  SMI2 callback functions for Tcg2 physical presence, ClearMemory, and
  sample for dTPM StartMethod.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable and ACPINvs data in SMM mode.
  This external input must be validated carefully to avoid security issue.

  PhysicalPresenceCallback() and MemoryClearCallback() will receive untrusted input and do some check.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Tcg2Smm.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SmmMemLib.h>

/**
  Notify the system that the SMM variable driver is ready.
**/
VOID
Tcg2NotifyMmReady (
  VOID
  )
{
  EFI_STATUS            Status;
  EFI_HANDLE            Handle;

  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gTcg2MmSwSmiRegisteredGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
}

/**
  This function is an abstraction layer for implementation specific Mm buffer validation routine.

  @param Buffer  The buffer start address to be checked.
  @param Length  The buffer length to be checked.

  @retval TRUE  This buffer is valid per processor architecture and not overlap with SMRAM.
  @retval FALSE This buffer is not valid per processor architecture or overlap with SMRAM.
**/
BOOLEAN
IsBufferOutsideMmValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  )
{
  return SmmIsBufferOutsideSmmValid (Buffer, Length);
}

/**
  The driver's entry point.

  It install callbacks for TPM physical presence and MemoryClear, and locate
  SMM variable to be used in the callback function.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval Others          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeTcgSmm (
  IN EFI_HANDLE                  ImageHandle,
  IN EFI_SYSTEM_TABLE            *SystemTable
  )
{
  return InitializeTcgCommon ();
}
