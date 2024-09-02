/** @file

  Parts of the SMM/MM implementation that are specific to standalone MM

Copyright (c) 2011 - 2024, Intel Corporation. All rights reserved. <BR>
Copyright (c) 2018, Linaro, Ltd. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/MmServicesTableLib.h>
#include <Library/StandaloneMmMemLib.h>
#include "Variable.h"

/**
  This function checks if the Primary Buffer (CommBuffer) is valid.

  @param Buffer The buffer start address to be checked.
  @param Length The buffer length to be checked.

  @retval TRUE  This buffer is valid.
  @retval FALSE This buffer is not valid.
**/
BOOLEAN
VariableSmmIsPrimaryBufferValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  )
{
  return TRUE;
}

/**
  This function checks if the buffer is valid per processor architecture and
  does not overlap with SMRAM.

  @param Buffer The buffer start address to be checked.
  @param Length The buffer length to be checked.

  @retval TRUE  This buffer is valid per processor architecture and does not
                overlap with SMRAM.
  @retval FALSE This buffer is not valid per processor architecture or overlaps
                with SMRAM.
**/
BOOLEAN
VariableSmmIsNonPrimaryBufferValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  )
{
  return MmIsBufferOutsideMmValid (Buffer, Length);
}

/**
  Notify the system that the SMM variable driver is ready.
**/
VOID
VariableNotifySmmReady (
  VOID
  )
{
}

/**
  Notify the system that the SMM variable write driver is ready.
**/
VOID
VariableNotifySmmWriteReady (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  Handle = NULL;
  Status = gMmst->MmInstallProtocolInterface (
                    &Handle,
                    &gSmmVariableWriteGuid,
                    EFI_NATIVE_INTERFACE,
                    NULL
                    );
  ASSERT_EFI_ERROR (Status);
}

/**
  Variable service MM driver entry point.

  @param[in] ImageHandle    A handle for the image that is initializing this
                            driver
  @param[in] MmSystemTable  A pointer to the MM system table

  @retval EFI_SUCCESS       Variable service successfully initialized.
**/
EFI_STATUS
EFIAPI
VariableServiceInitialize (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  return MmVariableServiceInitialize ();
}

/**
  Whether the MOR variable is legitimate or not.

  @retval TRUE  MOR Variable is legitimate.
  @retval FALSE MOR Variable in not legitimate.
**/
BOOLEAN
VariableIsMorVariableLegitimate (
  VOID
  )
{
  return TRUE;
}
