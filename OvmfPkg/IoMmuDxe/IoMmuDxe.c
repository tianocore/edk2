/** @file

  IoMmuDxe driver installs EDKII_IOMMU_PROTOCOL to provide the support for DMA
  operations when SEV is enabled.

  Copyright (c) 2017, AMD Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CcIoMmu.h"

EFI_STATUS
EFIAPI
IoMmuDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  //
  // When SEV or TDX is enabled, install IoMmu protocol otherwise install the
  // placeholder protocol so that other dependent module can run.
  //
  if (MemEncryptSevIsEnabled () || MemEncryptTdxIsEnabled ()) {
    Status = InstallIoMmuProtocol ();
  } else {
    Handle = NULL;

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Handle,
                    &gIoMmuAbsentProtocolGuid,
                    NULL,
                    NULL
                    );
  }

  return Status;
}
