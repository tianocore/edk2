/** @file

  Parts of the SMM/MM implementation that are specific to standalone MM

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved. <BR>
Copyright (c) 2018, Linaro, Ltd. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Variable.h"

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
VariableSmmIsBufferOutsideSmmValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  )
{
  return TRUE;
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
  IN EFI_HANDLE                           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE                  *MmSystemTable
  )
{
  return MmVariableServiceInitialize ();
}

/**
  Whether the TCG or TCG2 protocols are installed in the UEFI protocol database.
  This information is used by the MorLock code to infer whether an existing
  MOR variable is legitimate or not.

  @retval TRUE  Either the TCG or TCG2 protocol is installed in the UEFI
                protocol database
  @retval FALSE Neither the TCG nor the TCG2 protocol is installed in the UEFI
                protocol database
**/
BOOLEAN
VariableHaveTcgProtocols (
  VOID
  )
{
  return FALSE;
}
