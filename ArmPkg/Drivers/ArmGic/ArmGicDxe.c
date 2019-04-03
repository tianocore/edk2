/*++

Copyright (c) 2013-2014, ARM Ltd. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  ArmGicDxe.c

Abstract:

  Driver implementing the GIC interrupt controller protocol

--*/

#include <PiDxe.h>

#include "ArmGicDxe.h"

/**
  Initialize the state information for the CPU Architectural Protocol

  @param  ImageHandle   of the loaded driver
  @param  SystemTable   Pointer to the System Table

  @retval EFI_SUCCESS           Protocol registered
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Hardware problems
  @retval EFI_UNSUPPORTED       GIC version not supported

**/
EFI_STATUS
InterruptDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS            Status;
  ARM_GIC_ARCH_REVISION Revision;

  Revision = ArmGicGetSupportedArchRevision ();

  if (Revision == ARM_GIC_ARCH_REVISION_2) {
    Status = GicV2DxeInitialize (ImageHandle, SystemTable);
  } else if (Revision == ARM_GIC_ARCH_REVISION_3) {
    Status = GicV3DxeInitialize (ImageHandle, SystemTable);
  } else {
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}
