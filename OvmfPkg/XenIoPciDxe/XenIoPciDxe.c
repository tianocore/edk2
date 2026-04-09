/** @file

  Driver for the virtual Xen PCI device

  Copyright (C) 2012, Red Hat, Inc.
  Copyright (c) 2012 - 2016, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2013, ARM Ltd.
  Copyright (C) 2015, Linaro Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/Pci.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/PciIo.h>
#include <Protocol/XenIo.h>

#define PCI_VENDOR_ID_XEN           0x5853
#define PCI_DEVICE_ID_XEN_PLATFORM  0x0001

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

  @retval EFI_UNSUPPORTED  The driver does not support the device being probed.

  @return                  Error codes from the OpenProtocol() boot service or
                           the PciIo protocol.

**/
STATIC
EFI_STATUS
EFIAPI
XenIoPciDeviceBindingSupported (
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
    if ((Pci.Hdr.VendorId == PCI_VENDOR_ID_XEN) &&
        (Pci.Hdr.DeviceId == PCI_DEVICE_ID_XEN_PLATFORM))
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

  After we've pronounced support for a specific device in
  DriverBindingSupported(), we start managing said device (passed in by the
  Driver Execution Environment) with the following service.

  See DriverBindingSupported() for specification references.

  @param[in]  This                The EFI_DRIVER_BINDING_PROTOCOL object
                                  incorporating this driver (independently of
                                  any device).

  @param[in] DeviceHandle         The supported device to drive.

  @param[in] RemainingDevicePath  Relevant only for bus drivers, ignored.


  @retval EFI_SUCCESS           The device was started.

  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.

  @return                       Error codes from the OpenProtocol() boot
                                service, the PciIo protocol or the
                                InstallProtocolInterface() boot service.

**/
STATIC
EFI_STATUS
EFIAPI
XenIoPciDeviceBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                         Status;
  XENIO_PROTOCOL                     *XenIo;
  EFI_PCI_IO_PROTOCOL                *PciIo;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *BarDesc;

  XenIo = (XENIO_PROTOCOL *)AllocateZeroPool (sizeof *XenIo);
  if (XenIo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->OpenProtocol (
                  DeviceHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  DeviceHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto FreeXenIo;
  }

  //
  // The BAR1 of this PCI device is used for shared memory and is supposed to
  // look like MMIO. The address space of the BAR1 will be used to map the
  // Grant Table.
  //
  Status = PciIo->GetBarAttributes (PciIo, PCI_BAR_IDX1, NULL, (VOID **)&BarDesc);
  ASSERT_EFI_ERROR (Status);
  ASSERT (BarDesc->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM);

  /* Get a Memory address for mapping the Grant Table. */
  DEBUG ((DEBUG_INFO, "XenIoPci: BAR at %LX\n", BarDesc->AddrRangeMin));
  XenIo->GrantTableAddress = BarDesc->AddrRangeMin;
  FreePool (BarDesc);

  Status = gBS->InstallProtocolInterface (
                  &DeviceHandle,
                  &gXenIoProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  XenIo
                  );

  if (!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  gBS->CloseProtocol (
         DeviceHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         DeviceHandle
         );

FreeXenIo:
  FreePool (XenIo);

  return Status;
}

/**

  Stop driving the XenIo PCI device

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
XenIoPciDeviceBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS      Status;
  XENIO_PROTOCOL  *XenIo;

  Status = gBS->OpenProtocol (
                  DeviceHandle,                  // candidate device
                  &gXenIoProtocolGuid,           // retrieve the XenIo iface
                  (VOID **)&XenIo,               // target pointer
                  This->DriverBindingHandle,     // requestor driver identity
                  DeviceHandle,                  // requesting lookup for dev.
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL // lookup only, no ref. added
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Handle Stop() requests for in-use driver instances gracefully.
  //
  Status = gBS->UninstallProtocolInterface (
                  DeviceHandle,
                  &gXenIoProtocolGuid,
                  XenIo
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->CloseProtocol (
                  DeviceHandle,
                  &gEfiPciIoProtocolGuid,
                  This->DriverBindingHandle,
                  DeviceHandle
                  );

  FreePool (XenIo);

  return Status;
}

//
// The static object that groups the Supported() (ie. probe), Start() and
// Stop() functions of the driver together. Refer to UEFI Spec 2.3.1 + Errata
// C, 10.1 EFI Driver Binding Protocol.
//
STATIC EFI_DRIVER_BINDING_PROTOCOL  gDriverBinding = {
  &XenIoPciDeviceBindingSupported,
  &XenIoPciDeviceBindingStart,
  &XenIoPciDeviceBindingStop,
  0x10, // Version, must be in [0x10 .. 0xFFFFFFEF] for IHV-developed drivers
  NULL, // ImageHandle, to be overwritten by
        // EfiLibInstallDriverBindingComponentName2() in XenIoPciDeviceEntryPoint()
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
  { "eng;en", L"XenIo PCI Driver" },
  { NULL,     NULL                }
};

STATIC
EFI_COMPONENT_NAME_PROTOCOL  gComponentName;

EFI_STATUS
EFIAPI
XenIoPciGetDriverName (
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
XenIoPciGetDeviceName (
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
  &XenIoPciGetDriverName,
  &XenIoPciGetDeviceName,
  "eng" // SupportedLanguages, ISO 639-2 language codes
};

STATIC
EFI_COMPONENT_NAME2_PROTOCOL  gComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)&XenIoPciGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME)&XenIoPciGetDeviceName,
  "en" // SupportedLanguages, RFC 4646 language codes
};

//
// Entry point of this driver.
//
EFI_STATUS
EFIAPI
XenIoPciDeviceEntryPoint (
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
