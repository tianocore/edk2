/** @file

  IoMmuDxe driver installs EDKII_IOMMU_PROTOCOL to provide the support for DMA
  operations when SEV is enabled.

  Copyright (c) 2017, AMD Inc. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemEncryptSevLib.h>

#include "AmdSevIoMmu.h"

EFI_STATUS
EFIAPI
IoMmuDxeEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS    Status;
  EFI_HANDLE    Handle;

  //
  // When SEV is enabled, install IoMmu protocol otherwise install the
  // placeholder protocol so that other dependent module can run.
  //
  if (MemEncryptSevIsEnabled ()) {
    Status = AmdSevInstallIoMmuProtocol ();
  } else {
    Handle = NULL;

    Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gIoMmuAbsentProtocolGuid,
                  NULL, NULL);
  }

  return Status;
}
