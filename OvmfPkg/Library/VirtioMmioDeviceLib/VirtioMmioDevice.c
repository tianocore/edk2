/** @file

  This driver produces Virtio Device Protocol instances for Virtio Mmio devices.

  Copyright (C) 2013, ARM Ltd.
  Copyright (C) 2017, AMD Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "VirtioMmioDevice.h"

STATIC CONST VIRTIO_DEVICE_PROTOCOL  mMmioDeviceProtocolTemplate = {
  0,                                       // Revision
  0,                                       // SubSystemDeviceId
  VirtioMmioGetDeviceFeatures,             // GetDeviceFeatures
  VirtioMmioSetGuestFeatures,              // SetGuestFeatures
  VirtioMmioSetQueueAddress,               // SetQueueAddress
  VirtioMmioSetQueueSel,                   // SetQueueSel
  VirtioMmioSetQueueNotify,                // SetQueueNotify
  VirtioMmioSetQueueAlignment,             // SetQueueAlign
  VirtioMmioSetPageSize,                   // SetPageSize
  VirtioMmioGetQueueSize,                  // GetQueueNumMax
  VirtioMmioSetQueueSize,                  // SetQueueNum
  VirtioMmioGetDeviceStatus,               // GetDeviceStatus
  VirtioMmioSetDeviceStatus,               // SetDeviceStatus
  VirtioMmioDeviceWrite,                   // WriteDevice
  VirtioMmioDeviceRead,                    // ReadDevice
  VirtioMmioAllocateSharedPages,           // AllocateSharedPages
  VirtioMmioFreeSharedPages,               // FreeSharedPages
  VirtioMmioMapSharedBuffer,               // MapSharedBuffer
  VirtioMmioUnmapSharedBuffer              // UnmapSharedBuffer
};

/**

  Initialize the VirtIo MMIO Device

  @param[in] BaseAddress   Base Address of the VirtIo MMIO Device

  @param[in, out] Device   The driver instance to configure.

  @retval EFI_SUCCESS      Setup complete.

  @retval EFI_UNSUPPORTED  The driver is not a VirtIo MMIO device.

**/
STATIC
EFI_STATUS
EFIAPI
VirtioMmioInit (
  IN PHYSICAL_ADDRESS        BaseAddress,
  IN OUT VIRTIO_MMIO_DEVICE  *Device
  )
{
  UINT32  MagicValue;

  //
  // Initialize VirtIo Mmio Device
  //
  CopyMem (
    &Device->VirtioDevice,
    &mMmioDeviceProtocolTemplate,
    sizeof (VIRTIO_DEVICE_PROTOCOL)
    );
  Device->BaseAddress                    = BaseAddress;
  Device->VirtioDevice.SubSystemDeviceId =
    MmioRead32 (BaseAddress + VIRTIO_MMIO_OFFSET_DEVICE_ID);

  //
  // Double-check MMIO-specific values
  //
  MagicValue = VIRTIO_CFG_READ (Device, VIRTIO_MMIO_OFFSET_MAGIC);
  if (MagicValue != VIRTIO_MMIO_MAGIC) {
    return EFI_UNSUPPORTED;
  }

  Device->Version = VIRTIO_CFG_READ (Device, VIRTIO_MMIO_OFFSET_VERSION);
  switch (Device->Version) {
    case VIRTIO_MMIO_DEVICE_VERSION_0_95:
      DEBUG ((
        DEBUG_INFO,
        "%a virtio 0.9.5, id %d\n",
        __FUNCTION__,
        Device->VirtioDevice.SubSystemDeviceId
        ));
      Device->VirtioDevice.Revision = VIRTIO_SPEC_REVISION (0, 9, 5);
      break;
    case VIRTIO_MMIO_DEVICE_VERSION_1_00:
      DEBUG ((
        DEBUG_INFO,
        "%a virtio 1.0, id %d\n",
        __FUNCTION__,
        Device->VirtioDevice.SubSystemDeviceId
        ));
      Device->VirtioDevice.Revision = VIRTIO_SPEC_REVISION (1, 0, 0);
      break;
    default:
      return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**

  Uninitialize the internals of a virtio-mmio device that has been successfully
  set up with VirtioMmioInit().

  @param[in, out]  Device  The device to clean up.

**/
STATIC
VOID
EFIAPI
VirtioMmioUninit (
  IN VIRTIO_MMIO_DEVICE  *Device
  )
{
  //
  // Note: This function mirrors VirtioMmioInit() that does not allocate any
  //       resources - there's nothing to free here.
  //
}

EFI_STATUS
VirtioMmioInstallDevice (
  IN PHYSICAL_ADDRESS  BaseAddress,
  IN EFI_HANDLE        Handle
  )
{
  EFI_STATUS          Status;
  VIRTIO_MMIO_DEVICE  *VirtIo;

  if (!BaseAddress) {
    return EFI_INVALID_PARAMETER;
  }

  if (Handle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Allocate VIRTIO_MMIO_DEVICE
  //
  VirtIo = AllocateZeroPool (sizeof (VIRTIO_MMIO_DEVICE));
  if (VirtIo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  VirtIo->Signature = VIRTIO_MMIO_DEVICE_SIGNATURE;

  Status = VirtioMmioInit (BaseAddress, VirtIo);
  if (EFI_ERROR (Status)) {
    goto FreeVirtioMem;
  }

  //
  // Install VIRTIO_DEVICE_PROTOCOL to Handle
  //
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gVirtioDeviceProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &VirtIo->VirtioDevice
                  );
  if (EFI_ERROR (Status)) {
    goto UninitVirtio;
  }

  return EFI_SUCCESS;

UninitVirtio:
  VirtioMmioUninit (VirtIo);

FreeVirtioMem:
  FreePool (VirtIo);
  return Status;
}

EFI_STATUS
VirtioMmioUninstallDevice (
  IN EFI_HANDLE  DeviceHandle
  )
{
  VIRTIO_DEVICE_PROTOCOL  *VirtioDevice;
  VIRTIO_MMIO_DEVICE      *MmioDevice;
  EFI_STATUS              Status;

  Status = gBS->OpenProtocol (
                  DeviceHandle,                  // candidate device
                  &gVirtioDeviceProtocolGuid,    // retrieve the VirtIo iface
                  (VOID **)&VirtioDevice,        // target pointer
                  DeviceHandle,                  // requestor driver identity
                  DeviceHandle,                  // requesting lookup for dev.
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL // lookup only, no ref. added
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the MMIO device from the VirtIo Device instance
  //
  MmioDevice = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (VirtioDevice);

  //
  // Uninstall the protocol interface
  //
  Status = gBS->UninstallProtocolInterface (
                  DeviceHandle,
                  &gVirtioDeviceProtocolGuid,
                  &MmioDevice->VirtioDevice
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Uninitialize the VirtIo Device
  //
  VirtioMmioUninit (MmioDevice);
  FreePool (MmioDevice);

  return EFI_SUCCESS;
}
