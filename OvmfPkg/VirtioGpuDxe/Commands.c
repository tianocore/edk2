/** @file

  VirtIo GPU initialization, and commands (primitives) for the GPU device.

  Copyright (C) 2016, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <IndustryStandard/VirtioGpu.h>
#include <Library/VirtioLib.h>

#include "VirtioGpu.h"

/**
  Configure the VirtIo GPU device that underlies VgpuDev.

  @param[in,out] VgpuDev  The VGPU_DEV object to set up VirtIo messaging for.
                          On input, the caller is responsible for having
                          initialized VgpuDev->VirtIo. On output, VgpuDev->Ring
                          has been initialized, and synchronous VirtIo GPU
                          commands (primitives) can be submitted to the device.

  @retval EFI_SUCCESS      VirtIo GPU configuration successful.

  @retval EFI_UNSUPPORTED  The host-side configuration of the VirtIo GPU is not
                           supported by this driver.

  @retval                  Error codes from underlying functions.
**/
EFI_STATUS
VirtioGpuInit (
  IN OUT VGPU_DEV *VgpuDev
  )
{
  UINT8      NextDevStat;
  EFI_STATUS Status;
  UINT64     Features;
  UINT16     QueueSize;

  //
  // Execute virtio-v1.0-cs04, 3.1.1 Driver Requirements: Device
  // Initialization.
  //
  // 1. Reset the device.
  //
  NextDevStat = 0;
  Status = VgpuDev->VirtIo->SetDeviceStatus (VgpuDev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // 2. Set the ACKNOWLEDGE status bit [...]
  //
  NextDevStat |= VSTAT_ACK;
  Status = VgpuDev->VirtIo->SetDeviceStatus (VgpuDev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // 3. Set the DRIVER status bit [...]
  //
  NextDevStat |= VSTAT_DRIVER;
  Status = VgpuDev->VirtIo->SetDeviceStatus (VgpuDev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // 4. Read device feature bits...
  //
  Status = VgpuDev->VirtIo->GetDeviceFeatures (VgpuDev->VirtIo, &Features);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  if ((Features & VIRTIO_F_VERSION_1) == 0) {
    Status = EFI_UNSUPPORTED;
    goto Failed;
  }
  //
  // We only want the most basic 2D features.
  //
  Features &= VIRTIO_F_VERSION_1;

  //
  // ... and write the subset of feature bits understood by the [...] driver to
  // the device. [...]
  // 5. Set the FEATURES_OK status bit.
  // 6. Re-read device status to ensure the FEATURES_OK bit is still set [...]
  //
  Status = Virtio10WriteFeatures (VgpuDev->VirtIo, Features, &NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // 7. Perform device-specific setup, including discovery of virtqueues for
  // the device [...]
  //
  Status = VgpuDev->VirtIo->SetQueueSel (VgpuDev->VirtIo,
                              VIRTIO_GPU_CONTROL_QUEUE);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  Status = VgpuDev->VirtIo->GetQueueNumMax (VgpuDev->VirtIo, &QueueSize);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // We implement each VirtIo GPU command that we use with two descriptors:
  // request, response.
  //
  if (QueueSize < 2) {
    Status = EFI_UNSUPPORTED;
    goto Failed;
  }

  //
  // [...] population of virtqueues [...]
  //
  Status = VirtioRingInit (QueueSize, &VgpuDev->Ring);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  Status = VgpuDev->VirtIo->SetQueueAddress (VgpuDev->VirtIo, &VgpuDev->Ring);
  if (EFI_ERROR (Status)) {
    goto ReleaseQueue;
  }

  //
  // 8. Set the DRIVER_OK status bit.
  //
  NextDevStat |= VSTAT_DRIVER_OK;
  Status = VgpuDev->VirtIo->SetDeviceStatus (VgpuDev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto ReleaseQueue;
  }

  return EFI_SUCCESS;

ReleaseQueue:
  VirtioRingUninit (&VgpuDev->Ring);

Failed:
  //
  // If any of these steps go irrecoverably wrong, the driver SHOULD set the
  // FAILED status bit to indicate that it has given up on the device (it can
  // reset the device later to restart if desired). [...]
  //
  // VirtIo access failure here should not mask the original error.
  //
  NextDevStat |= VSTAT_FAILED;
  VgpuDev->VirtIo->SetDeviceStatus (VgpuDev->VirtIo, NextDevStat);

  return Status;
}

/**
  De-configure the VirtIo GPU device that underlies VgpuDev.

  @param[in,out] VgpuDev  The VGPU_DEV object to tear down VirtIo messaging
                          for. On input, the caller is responsible for having
                          called VirtioGpuInit(). On output, VgpuDev->Ring has
                          been uninitialized; VirtIo GPU commands (primitives)
                          can no longer be submitted to the device.
**/
VOID
VirtioGpuUninit (
  IN OUT VGPU_DEV *VgpuDev
  )
{
  //
  // Resetting the VirtIo device makes it release its resources and forget its
  // configuration.
  //
  VgpuDev->VirtIo->SetDeviceStatus (VgpuDev->VirtIo, 0);
  VirtioRingUninit (&VgpuDev->Ring);
}

/**
  EFI_EVENT_NOTIFY function for the VGPU_DEV.ExitBoot event. It resets the
  VirtIo device, causing it to release its resources and to forget its
  configuration.

  This function may only be called (that is, VGPU_DEV.ExitBoot may only be
  signaled) after VirtioGpuInit() returns and before VirtioGpuUninit() is
  called.

  @param[in] Event    Event whose notification function is being invoked.

  @param[in] Context  Pointer to the associated VGPU_DEV object.
**/
VOID
EFIAPI
VirtioGpuExitBoot (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  VGPU_DEV *VgpuDev;

  VgpuDev = Context;
  VgpuDev->VirtIo->SetDeviceStatus (VgpuDev->VirtIo, 0);
}
