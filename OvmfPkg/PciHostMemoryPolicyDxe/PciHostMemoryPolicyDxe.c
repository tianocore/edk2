/** @file
  Publishes EDKII_PCI_HOST_MEMORY_POLICY_PROTOCOL so that the generic PCI bus
  driver can identify prefetchable BARs that map host-backed memory (DRAM or
  VRAM) and should therefore be left without a firmware-assigned UC cache
  attribute.

  Currently this only recognizes the VirtIO-GPU hostmem/blob resource
  aperture (any prefetchable BAR on PCI vendor/device ID 1AF4:1050). All
  device-specific knowledge is confined to this platform driver; the generic
  PCI bus driver has none.

  Copyright (c) 2026, Collabora Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Acpi10.h>
#include <IndustryStandard/Pci22.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Protocol/PciHostMemoryPolicy.h>

#define VIRTIO_GPU_PCI_VENDOR_ID  0x1AF4
#define VIRTIO_GPU_PCI_DEVICE_ID  0x1050

/**
  Determine whether a prefetchable PCI memory BAR maps host-backed memory
  that should be left without a firmware-assigned cache attribute.

  @param[in]  This       Instance of this protocol.
  @param[in]  PciIo      The EFI_PCI_IO_PROTOCOL instance for the PCI device
                         that owns the BAR.
  @param[in]  BarIndex   Index of the BAR being programmed.

  @retval TRUE   The BAR should be left without a firmware cache attribute.
  @retval FALSE  The BAR should receive the default UC policy.

**/
STATIC
BOOLEAN
EFIAPI
IsHostBackedPrefetchableBar (
  IN EDKII_PCI_HOST_MEMORY_POLICY_PROTOCOL  *This,
  IN EFI_PCI_IO_PROTOCOL                    *PciIo,
  IN UINT8                                  BarIndex
  )
{
  EFI_STATUS                         Status;
  UINT16                             VendorId;
  UINT16                             DeviceId;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Descriptor;
  BOOLEAN                            Prefetchable;

  Status = PciIo->Pci.Read (
                         PciIo,
                         EfiPciIoWidthUint16,
                         PCI_VENDOR_ID_OFFSET,
                         1,
                         &VendorId
                         );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  Status = PciIo->Pci.Read (
                         PciIo,
                         EfiPciIoWidthUint16,
                         PCI_DEVICE_ID_OFFSET,
                         1,
                         &DeviceId
                         );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if ((VendorId != VIRTIO_GPU_PCI_VENDOR_ID) || (DeviceId != VIRTIO_GPU_PCI_DEVICE_ID)) {
    return FALSE;
  }

  //
  // Only exempt BARs that are actually prefetchable; VirtIO-GPU also has
  // ordinary register BARs that must keep the default UC policy.
  //
  Descriptor = NULL;
  Status     = PciIo->GetBarAttributes (PciIo, BarIndex, NULL, (VOID **)&Descriptor);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  Prefetchable = (BOOLEAN)((Descriptor->SpecificFlag &
                             EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE) != 0);
  FreePool (Descriptor);

  return Prefetchable;
}

STATIC EDKII_PCI_HOST_MEMORY_POLICY_PROTOCOL  mPciHostMemoryPolicy = {
  IsHostBackedPrefetchableBar
};

/**
  Entry point for this driver. Installs
  EDKII_PCI_HOST_MEMORY_POLICY_PROTOCOL unconditionally, before PCI bus
  enumeration programs any BAR.

  @param[in]  ImageHandle  The firmware allocated handle for this driver
                           image.
  @param[in]  SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS  The protocol was installed successfully.

**/
EFI_STATUS
EFIAPI
PciHostMemoryPolicyDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEdkiiPciHostMemoryPolicyProtocolGuid,
                  &mPciHostMemoryPolicy,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}
