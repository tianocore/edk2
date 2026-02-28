/** @file NonDiscoverableDeviceUniqueId.h
  With this new protocol, a platform can register a UniqueId to be used to identify the PciIo location.
  For example, if a device registers with the NonDiscoverableDeviceRegistrationLib,
  they can also publish this new protocol on the same EFI_HANDLE as RegisterNonDiscoverableMmioDevice.
  NonDiscoverablePciDeviceDxe will consume this UniqueId if found and the device's UniqueId to this value.
  If not found, it will back up to the prior method of assigning the UniqueId via a static counter.
  The platform must ensure all UniqueIds assigned are unique across the firmware.

  Platforms may need to define their own UniqueIds through this method because
  the current UniqueId that gets used in NonDiscoverablePciDeviceDxe is a static counter,
  which can lead to non-deterministic value's being returned by PciIo->GetLocation().
  In order to have a deterministic UniqueId returned by PciIo->GetLocation(), this protocol
  can be used to specify a specific UniqueId for a given Handle.
  For example, if a platform needs to Locate a specific NonDiscoverable PciIo protocol,
  they could use the deterministic UniqueId assigned through this protocol to find the expected PciIo instance
  because a platform can match against the GetLocation value returned by using the assigned UniqueId.
  This protocol allows us to have a 1-1 defined mapping between a platform defined UniqueId and the NonDiscoverable PciIo protocol instance.

  The unique ID must be in the range of:
    MAX_NON_DISCOVERABLE_PCI_DEVICE_ID/2 <= UniqueId < MAX_NON_DISCOVERABLE_PCI_DEVICE_ID
    MAX_NON_DISCOVERABLE_PCI_DEVICE_ID == 32 * 256

  Usage example:
    EFI_HANDLE Handle = NULL;
    UINTN      Size; // NonDiscoverableDevice Size
    VOID       *Base; // NonDiscoverableDevice Base

    Status = RegisterNonDiscoverableMmioDevice (
                NonDiscoverableDeviceTypeAhci,
                NonDiscoverableDeviceDmaTypeCoherent,
                NULL,
                &Handle,
                1,
                Base,
                Size
              );

    NON_DISCOVERABLE_DEVICE_UNIQUE_ID_PROTOCOL * UniqueIdProtocol = (NON_DISCOVERABLE_DEVICE_UNIQUE_ID_PROTOCOL*) AllocateZeroPool (sizeof (NON_DISCOVERABLE_DEVICE_UNIQUE_ID_PROTOCOL));
    if (UniqueIdProtocol == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    UniqueIdProtocol->Revision = NON_DISCOVERABLE_DEVICE_UNIQUE_ID_PROTOCOL_REVISION;
    UniqueIdProtocol->UniqueId = 0x1000; // MAX_NON_DISCOVERABLE_PCI_DEVICE_ID/2 <= UniqueId < MAX_NON_DISCOVERABLE_PCI_DEVICE_ID

    Status = gBS->InstallMultipleProtocolInterfaces (&Handle, &gEdkiiNonDiscoverableDeviceUniqueIdProtocolGuid, UniqueIdProtocol, NULL);

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef NON_DISCOVERABLE_DEVICE_UNIQUE_ID_PROTOCOL_H
#define NON_DISCOVERABLE_DEVICE_UNIQUE_ID_PROTOCOL_H

#include <Uefi/UefiBaseType.h>

#define EDKII_NON_DISCOVERABLE_DEVICE_UNIQUE_ID_PROTOCOL_GUID \
  { 0xd60d0e74, 0x5c4e, 0x4d45, { 0x9f, 0xe0, 0x2a, 0x3e, 0x4f, 0x9d, 0x55, 0xc1 } }

#define NON_DISCOVERABLE_DEVICE_UNIQUE_ID_PROTOCOL_REVISION  1

// We will use the upper half of the ID space for this UniqueId protocol assignment.
// The lower half can be used as a backup by NonDiscoverablePciDeviceDxe when the protocol is not available.
// The unique ID must be in the range of: MAX_NON_DISCOVERABLE_PCI_DEVICE_ID/2 <= UniqueId < MAX_NON_DISCOVERABLE_PCI_DEVICE_ID
#define MAX_NON_DISCOVERABLE_PCI_DEVICE_ID  (32 * 256)

typedef struct _NON_DISCOVERABLE_DEVICE_UNIQUE_ID_PROTOCOL {
  UINTN    Revision;
  UINTN    UniqueId;
} NON_DISCOVERABLE_DEVICE_UNIQUE_ID_PROTOCOL;

extern EFI_GUID  gEdkiiNonDiscoverableDeviceUniqueIdProtocolGuid;

#endif // NON_DISCOVERABLE_DEVICE_UNIQUE_ID_PROTOCOL_H
