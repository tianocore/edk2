/** @file
  Initialization and helper routines for the Virtio Filesystem device.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/VirtioLib.h>           // Virtio10WriteFeatures()

#include "VirtioFsDxe.h"

/**
  Read the Virtio Filesystem device configuration structure in full.

  @param[in] Virtio   The Virtio protocol underlying the VIRTIO_FS object.

  @param[out] Config  The fully populated VIRTIO_FS_CONFIG structure.

  @retval EFI_SUCCESS  Config has been filled in.

  @return              Error codes propagated from Virtio->ReadDevice(). The
                       contents of Config are indeterminate.
**/
STATIC
EFI_STATUS
VirtioFsReadConfig (
  IN  VIRTIO_DEVICE_PROTOCOL *Virtio,
  OUT VIRTIO_FS_CONFIG       *Config
  )
{
  UINTN      Idx;
  EFI_STATUS Status;

  for (Idx = 0; Idx < VIRTIO_FS_TAG_BYTES; Idx++) {
    Status = Virtio->ReadDevice (
                       Virtio,                                 // This
                       OFFSET_OF (VIRTIO_FS_CONFIG, Tag[Idx]), // FieldOffset
                       sizeof Config->Tag[Idx],                // FieldSize
                       sizeof Config->Tag[Idx],                // BufferSize
                       &Config->Tag[Idx]                       // Buffer
                       );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = Virtio->ReadDevice (
                     Virtio,                                     // This
                     OFFSET_OF (VIRTIO_FS_CONFIG, NumReqQueues), // FieldOffset
                     sizeof Config->NumReqQueues,                // FieldSize
                     sizeof Config->NumReqQueues,                // BufferSize
                     &Config->NumReqQueues                       // Buffer
                     );
  return Status;
}

/**
  Configure the Virtio Filesystem device underlying VirtioFs.

  @param[in,out] VirtioFs  The VIRTIO_FS object for which Virtio communication
                           should be set up. On input, the caller is
                           responsible for VirtioFs->Virtio having been
                           initialized. On output, synchronous Virtio
                           Filesystem commands (primitives) may be submitted to
                           the device.

  @retval EFI_SUCCESS      Virtio machinery has been set up.

  @retval EFI_UNSUPPORTED  The host-side configuration of the Virtio Filesystem
                           is not supported by this driver.

  @return                  Error codes from underlying functions.
**/
EFI_STATUS
VirtioFsInit (
  IN OUT VIRTIO_FS *VirtioFs
  )
{
  UINT8            NextDevStat;
  EFI_STATUS       Status;
  UINT64           Features;
  VIRTIO_FS_CONFIG Config;
  UINTN            Idx;
  UINT64           RingBaseShift;

  //
  // Execute virtio-v1.1-cs01-87fa6b5d8155, 3.1.1 Driver Requirements: Device
  // Initialization.
  //
  // 1. Reset the device.
  //
  NextDevStat = 0;
  Status = VirtioFs->Virtio->SetDeviceStatus (VirtioFs->Virtio, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // 2. Set the ACKNOWLEDGE status bit [...]
  //
  NextDevStat |= VSTAT_ACK;
  Status = VirtioFs->Virtio->SetDeviceStatus (VirtioFs->Virtio, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // 3. Set the DRIVER status bit [...]
  //
  NextDevStat |= VSTAT_DRIVER;
  Status = VirtioFs->Virtio->SetDeviceStatus (VirtioFs->Virtio, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // 4. Read device feature bits...
  //
  Status = VirtioFs->Virtio->GetDeviceFeatures (VirtioFs->Virtio, &Features);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  if ((Features & VIRTIO_F_VERSION_1) == 0) {
    Status = EFI_UNSUPPORTED;
    goto Failed;
  }
  //
  // No device-specific feature bits have been defined in file "virtio-fs.tex"
  // of the virtio spec at <https://github.com/oasis-tcs/virtio-spec.git>, as
  // of commit 87fa6b5d8155.
  //
  Features &= VIRTIO_F_VERSION_1 | VIRTIO_F_IOMMU_PLATFORM;

  //
  // ... and write the subset of feature bits understood by the [...] driver to
  // the device. [...]
  // 5. Set the FEATURES_OK status bit.
  // 6. Re-read device status to ensure the FEATURES_OK bit is still set [...]
  //
  Status = Virtio10WriteFeatures (VirtioFs->Virtio, Features, &NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // 7. Perform device-specific setup, including discovery of virtqueues for
  // the device, [...] reading [...] the device's virtio configuration space
  //
  Status = VirtioFsReadConfig (VirtioFs->Virtio, &Config);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // 7.a. Convert the filesystem label from UTF-8 to UCS-2. Only labels with
  // printable ASCII code points (U+0020 through U+007E) are supported.
  // NUL-terminate at either the terminator we find, or right after the
  // original label.
  //
  for (Idx = 0; Idx < VIRTIO_FS_TAG_BYTES && Config.Tag[Idx] != '\0'; Idx++) {
    if (Config.Tag[Idx] < 0x20 || Config.Tag[Idx] > 0x7E) {
      Status = EFI_UNSUPPORTED;
      goto Failed;
    }
    VirtioFs->Label[Idx] = Config.Tag[Idx];
  }
  VirtioFs->Label[Idx] = L'\0';

  //
  // 7.b. We need one queue for sending normal priority requests.
  //
  if (Config.NumReqQueues < 1) {
    Status = EFI_UNSUPPORTED;
    goto Failed;
  }

  //
  // 7.c. Fetch and remember the number of descriptors we can place on the
  // queue at once. We'll need two descriptors per request, as a minimum --
  // request header, response header.
  //
  Status = VirtioFs->Virtio->SetQueueSel (VirtioFs->Virtio,
                               VIRTIO_FS_REQUEST_QUEUE);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  Status = VirtioFs->Virtio->GetQueueNumMax (VirtioFs->Virtio,
                               &VirtioFs->QueueSize);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  if (VirtioFs->QueueSize < 2) {
    Status = EFI_UNSUPPORTED;
    goto Failed;
  }

  //
  // 7.d. [...] population of virtqueues [...]
  //
  Status = VirtioRingInit (VirtioFs->Virtio, VirtioFs->QueueSize,
             &VirtioFs->Ring);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  Status = VirtioRingMap (VirtioFs->Virtio, &VirtioFs->Ring, &RingBaseShift,
             &VirtioFs->RingMap);
  if (EFI_ERROR (Status)) {
    goto ReleaseQueue;
  }

  Status = VirtioFs->Virtio->SetQueueAddress (VirtioFs->Virtio,
                               &VirtioFs->Ring, RingBaseShift);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  //
  // 8. Set the DRIVER_OK status bit.
  //
  NextDevStat |= VSTAT_DRIVER_OK;
  Status = VirtioFs->Virtio->SetDeviceStatus (VirtioFs->Virtio, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  return EFI_SUCCESS;

UnmapQueue:
  VirtioFs->Virtio->UnmapSharedBuffer (VirtioFs->Virtio, VirtioFs->RingMap);

ReleaseQueue:
  VirtioRingUninit (VirtioFs->Virtio, &VirtioFs->Ring);

Failed:
  //
  // If any of these steps go irrecoverably wrong, the driver SHOULD set the
  // FAILED status bit to indicate that it has given up on the device (it can
  // reset the device later to restart if desired). [...]
  //
  // Virtio access failure here should not mask the original error.
  //
  NextDevStat |= VSTAT_FAILED;
  VirtioFs->Virtio->SetDeviceStatus (VirtioFs->Virtio, NextDevStat);

  return Status;
}

/**
  De-configure the Virtio Filesystem device underlying VirtioFs.

  @param[in] VirtioFs  The VIRTIO_FS object for which Virtio communication
                       should be torn down. On input, the caller is responsible
                       for having called VirtioFsInit(). On output, Virtio
                       Filesystem commands (primitives) must no longer be
                       submitted to the device.
**/
VOID
VirtioFsUninit (
  IN OUT VIRTIO_FS *VirtioFs
  )
{
  //
  // Resetting the Virtio device makes it release its resources and forget its
  // configuration.
  //
  VirtioFs->Virtio->SetDeviceStatus (VirtioFs->Virtio, 0);
  VirtioFs->Virtio->UnmapSharedBuffer (VirtioFs->Virtio, VirtioFs->RingMap);
  VirtioRingUninit (VirtioFs->Virtio, &VirtioFs->Ring);
}

/**
  ExitBootServices event notification function for a Virtio Filesystem object.

  This function resets the VIRTIO_FS.Virtio device, causing it to release all
  references to guest-side resources. The function may only be called after
  VirtioFsInit() returns successfully and before VirtioFsUninit() is called.

  @param[in] ExitBootEvent   The VIRTIO_FS.ExitBoot event that has been
                             signaled.

  @param[in] VirtioFsAsVoid  Pointer to the VIRTIO_FS object, passed in as
                             (VOID*).
**/
VOID
EFIAPI
VirtioFsExitBoot (
  IN EFI_EVENT ExitBootEvent,
  IN VOID      *VirtioFsAsVoid
  )
{
  VIRTIO_FS *VirtioFs;

  VirtioFs = VirtioFsAsVoid;
  DEBUG ((DEBUG_VERBOSE, "%a: VirtioFs=0x%p Label=\"%s\"\n", __FUNCTION__,
    VirtioFsAsVoid, VirtioFs->Label));
  VirtioFs->Virtio->SetDeviceStatus (VirtioFs->Virtio, 0);
}
