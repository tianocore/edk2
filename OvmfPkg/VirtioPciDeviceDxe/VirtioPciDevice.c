/** @file

  This driver produces Virtio Device Protocol instances for Virtio PCI devices.

  Copyright (C) 2012, Red Hat, Inc.
  Copyright (c) 2012 - 2016, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2013, ARM Ltd.
  Copyright (C) 2017, AMD Inc, All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Pci.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include "VirtioPciDevice.h"

STATIC VIRTIO_DEVICE_PROTOCOL  mDeviceProtocolTemplate = {
  0,                                    // Revision
  0,                                    // SubSystemDeviceId
  VirtioPciGetDeviceFeatures,           // GetDeviceFeatures
  VirtioPciSetGuestFeatures,            // SetGuestFeatures
  VirtioPciSetQueueAddress,             // SetQueueAddress
  VirtioPciSetQueueSel,                 // SetQueueSel
  VirtioPciSetQueueNotify,              // SetQueueNotify
  VirtioPciSetQueueAlignment,           // SetQueueAlignment
  VirtioPciSetPageSize,                 // SetPageSize
  VirtioPciGetQueueSize,                // GetQueueNumMax
  VirtioPciSetQueueSize,                // SetQueueNum
  VirtioPciGetDeviceStatus,             // GetDeviceStatus
  VirtioPciSetDeviceStatus,             // SetDeviceStatus
  VirtioPciDeviceWrite,                 // WriteDevice
  VirtioPciDeviceRead,                  // ReadDevice
  VirtioPciAllocateSharedPages,         // AllocateSharedPages
  VirtioPciFreeSharedPages,             // FreeSharedPages
  VirtioPciMapSharedBuffer,             // MapSharedBuffer
  VirtioPciUnmapSharedBuffer,           // UnmapSharedBuffer
};

/**

  Read a word from Region 0 of the device specified by PciIo.

  Region 0 must be an iomem region. This is an internal function for the PCI
  implementation of the protocol.

  @param[in] Dev          Virtio PCI device.

  @param[in] FieldOffset  Source offset.

  @param[in] FieldSize    Source field size, must be in { 1, 2, 4, 8 }.

  @param[in] BufferSize   Number of bytes available in the target buffer. Must
                          equal FieldSize.

  @param[out] Buffer      Target buffer.


  @return  Status code returned by PciIo->Io.Read().

**/
EFI_STATUS
EFIAPI
VirtioPciIoRead (
  IN  VIRTIO_PCI_DEVICE  *Dev,
  IN  UINTN              FieldOffset,
  IN  UINTN              FieldSize,
  IN  UINTN              BufferSize,
  OUT VOID               *Buffer
  )
{
  UINTN                      Count;
  EFI_PCI_IO_PROTOCOL_WIDTH  Width;
  EFI_PCI_IO_PROTOCOL        *PciIo;

  ASSERT (FieldSize == BufferSize);

  PciIo = Dev->PciIo;
  Count = 1;

  switch (FieldSize) {
    case 1:
      Width = EfiPciIoWidthUint8;
      break;

    case 2:
      Width = EfiPciIoWidthUint16;
      break;

    case 8:
      //
      // The 64bit PCI I/O is broken down into two 32bit reads to prevent
      // any alignment or width issues.
      // The UEFI spec says under EFI_PCI_IO_PROTOCOL.Io.Write():
      //
      // The I/O operations are carried out exactly as requested. The caller
      // is responsible for any alignment and I/O width issues which the
      // bus, device, platform, or type of I/O might require. For example on
      // some platforms, width requests of EfiPciIoWidthUint64 do not work.
      //
      Count = 2;

    //
    // fall through
    //
    case 4:
      Width = EfiPciIoWidthUint32;
      break;

    default:
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
  }

  return PciIo->Io.Read (
                     PciIo,
                     Width,
                     PCI_BAR_IDX0,
                     FieldOffset,
                     Count,
                     Buffer
                     );
}

/**

  Write a word into Region 0 of the device specified by PciIo.

  Region 0 must be an iomem region. This is an internal function for the PCI
  implementation of the protocol.

  @param[in] Dev          Virtio PCI device.

  @param[in] FieldOffset  Destination offset.

  @param[in] FieldSize    Destination field size, must be in { 1, 2, 4, 8 }.

  @param[in] Value        Little endian value to write, converted to UINT64.
                          The least significant FieldSize bytes will be used.


  @return  Status code returned by PciIo->Io.Write().

**/
EFI_STATUS
EFIAPI
VirtioPciIoWrite (
  IN  VIRTIO_PCI_DEVICE  *Dev,
  IN UINTN               FieldOffset,
  IN UINTN               FieldSize,
  IN UINT64              Value
  )
{
  UINTN                      Count;
  EFI_PCI_IO_PROTOCOL_WIDTH  Width;
  EFI_PCI_IO_PROTOCOL        *PciIo;

  PciIo = Dev->PciIo;
  Count = 1;

  switch (FieldSize) {
    case 1:
      Width = EfiPciIoWidthUint8;
      break;

    case 2:
      Width = EfiPciIoWidthUint16;
      break;

    case 8:
      //
      // The 64bit PCI I/O is broken down into two 32bit writes to prevent
      // any alignment or width issues.
      // The UEFI spec says under EFI_PCI_IO_PROTOCOL.Io.Write():
      //
      // The I/O operations are carried out exactly as requested. The caller
      // is responsible for any alignment and I/O width issues which the
      // bus, device, platform, or type of I/O might require. For example on
      // some platforms, width requests of EfiPciIoWidthUint64 do not work
      //
      Count = Count * 2;

    //
    // fall through
    //
    case 4:
      Width = EfiPciIoWidthUint32;
      break;

    default:
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
  }

  return PciIo->Io.Write (
                     PciIo,
                     Width,
                     PCI_BAR_IDX0,
                     FieldOffset,
                     Count,
                     &Value
                     );
}

/**

  Device probe function for this driver.

  The DXE core calls this function for any given device in order to see if the
  driver can drive the device.

  @param[in]  This                The EFI_DRIVER_BINDING_PROTOCOL object
                                  incorporating this driver (independently of
                                  any device).

  @param[in] DeviceHandle         The device to probe.

  @param[in] RemainingDevicePath  Relevant only for bus drivers, ignored.


  @retval EFI_SUCCESS      The driver supports the device being probed.

  @retval EFI_UNSUPPORTED  Based on virtio-pci discovery, we do not support
                           the device.

  @return                  Error codes from the OpenProtocol() boot service or
                           the PciIo protocol.

**/
STATIC
EFI_STATUS
EFIAPI
VirtioPciDeviceBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  PCI_TYPE00           Pci;

  //
  // Attempt to open the device with the PciIo set of interfaces. On success,
  // the protocol is "instantiated" for the PCI device. Covers duplicate open
  // attempts (EFI_ALREADY_STARTED).
  //
  Status = gBS->OpenProtocol (
                  DeviceHandle,               // candidate device
                  &gEfiPciIoProtocolGuid,     // for generic PCI access
                  (VOID **)&PciIo,            // handle to instantiate
                  This->DriverBindingHandle,  // requestor driver identity
                  DeviceHandle,               // ControllerHandle, according to
                                              // the UEFI Driver Model
                  EFI_OPEN_PROTOCOL_BY_DRIVER // get exclusive PciIo access to
                                              // the device; to be released
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Read entire PCI configuration header for more extensive check ahead.
  //
  Status = PciIo->Pci.Read (
                        PciIo,                        // (protocol, device)
                                                      // handle
                        EfiPciIoWidthUint32,          // access width & copy
                                                      // mode
                        0,                            // Offset
                        sizeof Pci / sizeof (UINT32), // Count
                        &Pci                          // target buffer
                        );

  if (Status == EFI_SUCCESS) {
    //
    // virtio-0.9.5, 2.1 PCI Discovery
    //
    if ((Pci.Hdr.VendorId == VIRTIO_VENDOR_ID) &&
        (Pci.Hdr.DeviceId >= 0x1000) &&
        (Pci.Hdr.DeviceId <= 0x103F) &&
        (Pci.Hdr.RevisionID == 0x00))
    {
      Status = EFI_SUCCESS;
    } else {
      Status = EFI_UNSUPPORTED;
    }
  }

  //
  // We needed PCI IO access only transitorily, to see whether we support the
  // device or not.
  //
  gBS->CloseProtocol (
         DeviceHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         DeviceHandle
         );

  return Status;
}

/**

  Initialize the VirtIo PCI Device

  @param[in, out] Dev      The driver instance to configure. The caller is
                           responsible for Device->PciIo's validity (ie. working IO
                           access to the underlying virtio-pci device).

  @retval EFI_SUCCESS      Setup complete.

  @retval EFI_UNSUPPORTED  The underlying IO device doesn't support the
                           provided address offset and read size.

  @return                  Error codes from PciIo->Pci.Read().

**/
STATIC
EFI_STATUS
EFIAPI
VirtioPciInit (
  IN OUT VIRTIO_PCI_DEVICE  *Device
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  PCI_TYPE00           Pci;

  ASSERT (Device != NULL);
  PciIo = Device->PciIo;
  ASSERT (PciIo != NULL);
  ASSERT (PciIo->Pci.Read != NULL);

  Status = PciIo->Pci.Read (
                        PciIo,                          // (protocol, device)
                                                        // handle
                        EfiPciIoWidthUint32,            // access width & copy
                                                        // mode
                        0,                              // Offset
                        sizeof (Pci) / sizeof (UINT32), // Count
                        &Pci                            // target buffer
                        );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Copy protocol template
  //
  CopyMem (
    &Device->VirtioDevice,
    &mDeviceProtocolTemplate,
    sizeof (VIRTIO_DEVICE_PROTOCOL)
    );

  //
  // Initialize the protocol interface attributes
  //
  Device->VirtioDevice.Revision          = VIRTIO_SPEC_REVISION (0, 9, 5);
  Device->VirtioDevice.SubSystemDeviceId = Pci.Device.SubsystemID;

  //
  // Note: We don't support the MSI-X capability.  If we did,
  //       the offset would become 24 after enabling MSI-X.
  //
  Device->DeviceSpecificConfigurationOffset =
    VIRTIO_DEVICE_SPECIFIC_CONFIGURATION_OFFSET_PCI;

  return EFI_SUCCESS;
}

/**

  Uninitialize the internals of a virtio-pci device that has been successfully
  set up with VirtioPciInit().

  @param[in, out]  Dev  The device to clean up.

**/
STATIC
VOID
EFIAPI
VirtioPciUninit (
  IN OUT VIRTIO_PCI_DEVICE  *Device
  )
{
  // Note: This function mirrors VirtioPciInit() that does not allocate any
  //       resources - there's nothing to free here.
}

/**

  After we've pronounced support for a specific device in
  DriverBindingSupported(), we start managing said device (passed in by the
  Driver Execution Environment) with the following service.

  See DriverBindingSupported() for specification references.

  @param[in]  This                The EFI_DRIVER_BINDING_PROTOCOL object
                                  incorporating this driver (independently of
                                  any device).

  @param[in] DeviceHandle         The supported device to drive.

  @param[in] RemainingDevicePath  Relevant only for bus drivers, ignored.


  @retval EFI_SUCCESS           Driver instance has been created and
                                initialized  for the virtio-pci device, it
                                is now accessible via VIRTIO_DEVICE_PROTOCOL.

  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.

  @return                       Error codes from the OpenProtocol() boot
                                service, the PciIo protocol, VirtioPciInit(),
                                or the InstallProtocolInterface() boot service.

**/
STATIC
EFI_STATUS
EFIAPI
VirtioPciDeviceBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  VIRTIO_PCI_DEVICE  *Device;
  EFI_STATUS         Status;

  Device = (VIRTIO_PCI_DEVICE *)AllocateZeroPool (sizeof *Device);
  if (Device == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->OpenProtocol (
                  DeviceHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&Device->PciIo,
                  This->DriverBindingHandle,
                  DeviceHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto FreeVirtioPci;
  }

  //
  // We must retain and ultimately restore the original PCI attributes of the
  // device. See Driver Writer's Guide for UEFI 2.3.1 v1.01, 18.3 PCI drivers /
  // 18.3.2 Start() and Stop().
  //
  // The third parameter ("Attributes", input) is ignored by the Get operation.
  // The fourth parameter ("Result", output) is ignored by the Enable and Set
  // operations.
  //
  // For virtio-pci we only need IO space access.
  //
  Status = Device->PciIo->Attributes (
                            Device->PciIo,
                            EfiPciIoAttributeOperationGet,
                            0,
                            &Device->OriginalPciAttributes
                            );
  if (EFI_ERROR (Status)) {
    goto ClosePciIo;
  }

  Status = Device->PciIo->Attributes (
                            Device->PciIo,
                            EfiPciIoAttributeOperationEnable,
                            (EFI_PCI_IO_ATTRIBUTE_IO |
                             EFI_PCI_IO_ATTRIBUTE_BUS_MASTER),
                            NULL
                            );
  if (EFI_ERROR (Status)) {
    goto ClosePciIo;
  }

  //
  // PCI IO access granted, configure protocol instance
  //

  Status = VirtioPciInit (Device);
  if (EFI_ERROR (Status)) {
    goto RestorePciAttributes;
  }

  //
  // Setup complete, attempt to export the driver instance's VirtioDevice
  // interface.
  //
  Device->Signature = VIRTIO_PCI_DEVICE_SIGNATURE;
  Status            = gBS->InstallProtocolInterface (
                             &DeviceHandle,
                             &gVirtioDeviceProtocolGuid,
                             EFI_NATIVE_INTERFACE,
                             &Device->VirtioDevice
                             );
  if (EFI_ERROR (Status)) {
    goto UninitDev;
  }

  return EFI_SUCCESS;

UninitDev:
  VirtioPciUninit (Device);

RestorePciAttributes:
  Device->PciIo->Attributes (
                   Device->PciIo,
                   EfiPciIoAttributeOperationSet,
                   Device->OriginalPciAttributes,
                   NULL
                   );

ClosePciIo:
  gBS->CloseProtocol (
         DeviceHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         DeviceHandle
         );

FreeVirtioPci:
  FreePool (Device);

  return Status;
}

/**

  Stop driving the Virtio PCI device

  @param[in] This               The EFI_DRIVER_BINDING_PROTOCOL object
                                incorporating this driver (independently of any
                                device).

  @param[in] DeviceHandle       Stop driving this device.

  @param[in] NumberOfChildren   Since this function belongs to a device driver
                                only (as opposed to a bus driver), the caller
                                environment sets NumberOfChildren to zero, and
                                we ignore it.

  @param[in] ChildHandleBuffer  Ignored (corresponding to NumberOfChildren).

  @retval EFI_SUCCESS           Driver instance has been stopped and the PCI
                                configuration attributes have been restored.

  @return                       Error codes from the OpenProtocol() or
                                CloseProtocol(), UninstallProtocolInterface()
                                boot services.

**/
STATIC
EFI_STATUS
EFIAPI
VirtioPciDeviceBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS              Status;
  VIRTIO_DEVICE_PROTOCOL  *VirtioDevice;
  VIRTIO_PCI_DEVICE       *Device;

  Status = gBS->OpenProtocol (
                  DeviceHandle,                  // candidate device
                  &gVirtioDeviceProtocolGuid,    // retrieve the VirtIo iface
                  (VOID **)&VirtioDevice,        // target pointer
                  This->DriverBindingHandle,     // requestor driver identity
                  DeviceHandle,                  // requesting lookup for dev.
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL // lookup only, no ref. added
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Device = VIRTIO_PCI_DEVICE_FROM_VIRTIO_DEVICE (VirtioDevice);

  //
  // Handle Stop() requests for in-use driver instances gracefully.
  //
  Status = gBS->UninstallProtocolInterface (
                  DeviceHandle,
                  &gVirtioDeviceProtocolGuid,
                  &Device->VirtioDevice
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  VirtioPciUninit (Device);

  Device->PciIo->Attributes (
                   Device->PciIo,
                   EfiPciIoAttributeOperationSet,
                   Device->OriginalPciAttributes,
                   NULL
                   );

  Status = gBS->CloseProtocol (
                  DeviceHandle,
                  &gEfiPciIoProtocolGuid,
                  This->DriverBindingHandle,
                  DeviceHandle
                  );

  FreePool (Device);

  return Status;
}

//
// The static object that groups the Supported() (ie. probe), Start() and
// Stop() functions of the driver together. Refer to UEFI Spec 2.3.1 + Errata
// C, 10.1 EFI Driver Binding Protocol.
//
STATIC EFI_DRIVER_BINDING_PROTOCOL  gDriverBinding = {
  &VirtioPciDeviceBindingSupported,
  &VirtioPciDeviceBindingStart,
  &VirtioPciDeviceBindingStop,
  0x10, // Version, must be in [0x10 .. 0xFFFFFFEF] for IHV-developed drivers
  NULL, // ImageHandle, to be overwritten by
        // EfiLibInstallDriverBindingComponentName2() in VirtioPciEntryPoint()
  NULL  // DriverBindingHandle, ditto
};

//
// The purpose of the following scaffolding (EFI_COMPONENT_NAME_PROTOCOL and
// EFI_COMPONENT_NAME2_PROTOCOL implementation) is to format the driver's name
// in English, for display on standard console devices. This is recommended for
// UEFI drivers that follow the UEFI Driver Model. Refer to the Driver Writer's
// Guide for UEFI 2.3.1 v1.01, 11 UEFI Driver and Controller Names.
//
STATIC
EFI_UNICODE_STRING_TABLE  mDriverNameTable[] = {
  { "eng;en", L"Virtio PCI Driver" },
  { NULL,     NULL                 }
};

STATIC
EFI_COMPONENT_NAME_PROTOCOL  gComponentName;

EFI_STATUS
EFIAPI
VirtioPciGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mDriverNameTable,
           DriverName,
           (BOOLEAN)(This == &gComponentName) // Iso639Language
           );
}

EFI_STATUS
EFIAPI
VirtioPciGetDeviceName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   DeviceHandle,
  IN  EFI_HANDLE                   ChildHandle,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_COMPONENT_NAME_PROTOCOL  gComponentName = {
  &VirtioPciGetDriverName,
  &VirtioPciGetDeviceName,
  "eng" // SupportedLanguages, ISO 639-2 language codes
};

STATIC
EFI_COMPONENT_NAME2_PROTOCOL  gComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)&VirtioPciGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME)&VirtioPciGetDeviceName,
  "en" // SupportedLanguages, RFC 4646 language codes
};

//
// Entry point of this driver.
//
EFI_STATUS
EFIAPI
VirtioPciDeviceEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gDriverBinding,
           ImageHandle,
           &gComponentName,
           &gComponentName2
           );
}
