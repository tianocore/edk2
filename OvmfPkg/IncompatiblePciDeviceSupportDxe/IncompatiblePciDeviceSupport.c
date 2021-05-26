/** @file
  A simple DXE_DRIVER that causes the PCI Bus UEFI_DRIVER to allocate 64-bit
  MMIO BARs above 4 GB, regardless of option ROM availability (as long as a CSM
  is not present), conserving 32-bit MMIO aperture for 32-bit BARs.

  Copyright (C) 2016, Red Hat, Inc.
  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi10.h>
#include <IndustryStandard/Pci22.h>

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/IncompatiblePciDeviceSupport.h>
#include <Protocol/LegacyBios.h>

//
// The Legacy BIOS protocol has been located.
//
STATIC BOOLEAN mLegacyBiosInstalled;

//
// The protocol interface this driver produces.
//
STATIC EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL
                                                 mIncompatiblePciDeviceSupport;

//
// Configuration template for the CheckDevice() protocol member function.
//
// Refer to Table 20 "ACPI 2.0 & 3.0 QWORD Address Space Descriptor Usage" in
// the Platform Init 1.4a Spec, Volume 5.
//
// This structure is interpreted by the UpdatePciInfo() function in the edk2
// PCI Bus UEFI_DRIVER.
//
#pragma pack (1)
typedef struct {
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR AddressSpaceDesc;
  EFI_ACPI_END_TAG_DESCRIPTOR       EndDesc;
} MMIO64_PREFERENCE;
#pragma pack ()

STATIC CONST MMIO64_PREFERENCE mConfiguration = {
  //
  // AddressSpaceDesc
  //
  {
    ACPI_ADDRESS_SPACE_DESCRIPTOR,                 // Desc
    (UINT16)(                                      // Len
      sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) -
      OFFSET_OF (
        EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR,
        ResType
        )
      ),
    ACPI_ADDRESS_SPACE_TYPE_MEM,                   // ResType
    0,                                             // GenFlag
    0,                                             // SpecificFlag
    64,                                            // AddrSpaceGranularity:
                                                   //   aperture selection hint
                                                   //   for BAR allocation
    0,                                             // AddrRangeMin
    0,                                             // AddrRangeMax:
                                                   //   no special alignment
                                                   //   for affected BARs
    MAX_UINT64,                                    // AddrTranslationOffset:
                                                   //   hint covers all
                                                   //   eligible BARs
    0                                              // AddrLen:
                                                   //   use probed BAR size
  },
  //
  // EndDesc
  //
  {
    ACPI_END_TAG_DESCRIPTOR,                       // Desc
    0                                              // Checksum: to be ignored
  }
};

//
// The CheckDevice() member function has been called.
//
STATIC BOOLEAN mCheckDeviceCalled;


/**
  Notification callback for Legacy BIOS protocol installation.

  @param[in] Event    Event whose notification function is being invoked.

  @param[in] Context  The pointer to the notification function's context, which
                      is implementation-dependent.
**/
STATIC
VOID
EFIAPI
LegacyBiosInstalled (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  EFI_STATUS               Status;
  EFI_LEGACY_BIOS_PROTOCOL *LegacyBios;

  ASSERT (!mCheckDeviceCalled);

  Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid,
                  NULL /* Registration */, (VOID **)&LegacyBios);
  if (EFI_ERROR (Status)) {
    return;
  }

  mLegacyBiosInstalled = TRUE;

  //
  // Close the event and deregister this callback.
  //
  Status = gBS->CloseEvent (Event);
  ASSERT_EFI_ERROR (Status);
}


/**
  Returns a list of ACPI resource descriptors that detail the special resource
  configuration requirements for an incompatible PCI device.

  Prior to bus enumeration, the PCI bus driver will look for the presence of
  the EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL. Only one instance of this
  protocol can be present in the system. For each PCI device that the PCI bus
  driver discovers, the PCI bus driver calls this function with the device's
  vendor ID, device ID, revision ID, subsystem vendor ID, and subsystem device
  ID. If the VendorId, DeviceId, RevisionId, SubsystemVendorId, or
  SubsystemDeviceId value is set to (UINTN)-1, that field will be ignored. The
  ID values that are not (UINTN)-1 will be used to identify the current device.

  This function will only return EFI_SUCCESS. However, if the device is an
  incompatible PCI device, a list of ACPI resource descriptors will be returned
  in Configuration. Otherwise, NULL will be returned in Configuration instead.
  The PCI bus driver does not need to allocate memory for Configuration.
  However, it is the PCI bus driver's responsibility to free it. The PCI bus
  driver then can configure this device with the information that is derived
  from this list of resource nodes, rather than the result of BAR probing.

  Only the following two resource descriptor types from the ACPI Specification
  may be used to describe the incompatible PCI device resource requirements:
  - QWORD Address Space Descriptor (ACPI 2.0, section 6.4.3.5.1; also ACPI 3.0)
  - End Tag (ACPI 2.0, section 6.4.2.8; also ACPI 3.0)

  The QWORD Address Space Descriptor can describe memory, I/O, and bus number
  ranges for dynamic or fixed resources. The configuration of a PCI root bridge
  is described with one or more QWORD Address Space Descriptors, followed by an
  End Tag. See the ACPI Specification for details on the field values.

  @param[in]  This                Pointer to the
                                  EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL
                                  instance.

  @param[in]  VendorId            A unique ID to identify the manufacturer of
                                  the PCI device.  See the Conventional PCI
                                  Specification 3.0 for details.

  @param[in]  DeviceId            A unique ID to identify the particular PCI
                                  device. See the Conventional PCI
                                  Specification 3.0 for details.

  @param[in]  RevisionId          A PCI device-specific revision identifier.
                                  See the Conventional PCI Specification 3.0
                                  for details.

  @param[in]  SubsystemVendorId   Specifies the subsystem vendor ID. See the
                                  Conventional PCI Specification 3.0 for
                                  details.

  @param[in]  SubsystemDeviceId   Specifies the subsystem device ID. See the
                                  Conventional PCI Specification 3.0 for
                                  details.

  @param[out] Configuration       A list of ACPI resource descriptors that
                                  detail the configuration requirement.

  @retval EFI_SUCCESS   The function always returns EFI_SUCCESS.
**/
STATIC
EFI_STATUS
EFIAPI
CheckDevice (
  IN  EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL  *This,
  IN  UINTN                                         VendorId,
  IN  UINTN                                         DeviceId,
  IN  UINTN                                         RevisionId,
  IN  UINTN                                         SubsystemVendorId,
  IN  UINTN                                         SubsystemDeviceId,
  OUT VOID                                          **Configuration
  )
{
  mCheckDeviceCalled = TRUE;

  //
  // Unlike the general description of this protocol member suggests, there is
  // nothing incompatible about the PCI devices that we'll match here. We'll
  // match all PCI devices, and generate exactly one QWORD Address Space
  // Descriptor for each. That descriptor will instruct the PCI Bus UEFI_DRIVER
  // not to degrade 64-bit MMIO BARs for the device, even if a PCI option ROM
  // BAR is present on the device.
  //
  // The concern captured in the PCI Bus UEFI_DRIVER is that a legacy BIOS boot
  // (via a CSM) could dispatch a legacy option ROM on the device, which might
  // have trouble with MMIO BARs that have been allocated outside of the 32-bit
  // address space. But, if we don't support legacy option ROMs at all, then
  // this problem cannot arise.
  //
  if (mLegacyBiosInstalled) {
    //
    // Don't interfere with resource degradation.
    //
    *Configuration = NULL;
    return EFI_SUCCESS;
  }

  //
  // This member function is mis-specified actually: it is supposed to allocate
  // memory, but as specified, it could not return an error status. Thankfully,
  // the edk2 PCI Bus UEFI_DRIVER actually handles error codes; see the
  // UpdatePciInfo() function.
  //
  *Configuration = AllocateCopyPool (sizeof mConfiguration, &mConfiguration);
  if (*Configuration == NULL) {
    DEBUG ((DEBUG_WARN,
      "%a: 64-bit MMIO BARs may be degraded for PCI 0x%04x:0x%04x (rev %d)\n",
      __FUNCTION__, (UINT32)VendorId, (UINT32)DeviceId, (UINT8)RevisionId));
    return EFI_OUT_OF_RESOURCES;
  }
  return EFI_SUCCESS;
}


/**
  Entry point for this driver.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  Pointer to SystemTable.

  @retval EFI_SUCESS       Driver has loaded successfully.
  @retval EFI_UNSUPPORTED  PCI resource allocation has been disabled.
  @retval EFI_UNSUPPORTED  There is no 64-bit PCI MMIO aperture.
  @return                  Error codes from lower level functions.

**/
EFI_STATUS
EFIAPI
DriverInitialize (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;
  EFI_EVENT  Event;
  VOID       *Registration;

  //
  // If there is no 64-bit PCI MMIO aperture, then 64-bit MMIO BARs have to be
  // allocated under 4 GB unconditionally.
  //
  if (PcdGet64 (PcdPciMmio64Size) == 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Otherwise, create a protocol notify to see if a CSM is present. (With the
  // CSM absent, the PCI Bus driver won't have to worry about allocating 64-bit
  // MMIO BARs in the 32-bit MMIO aperture, for the sake of a legacy BIOS.)
  //
  // If the Legacy BIOS Protocol is present at the time of this driver starting
  // up, we can mark immediately that the PCI Bus driver should perform the
  // usual 64-bit MMIO BAR degradation.
  //
  // Otherwise, if the Legacy BIOS Protocol is absent at startup, it may be
  // installed later. However, if it doesn't show up until the first
  // EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL.CheckDevice() call from the
  // PCI Bus driver, then it never will:
  //
  // 1. The following drivers are dispatched in some unspecified order:
  //    - PCI Host Bridge DXE_DRIVER,
  //    - PCI Bus UEFI_DRIVER,
  //    - this DXE_DRIVER,
  //    - Legacy BIOS DXE_DRIVER.
  //
  // 2. The DXE_CORE enters BDS.
  //
  // 3. The platform BDS connects the PCI Root Bridge IO instances (produced by
  //    the PCI Host Bridge DXE_DRIVER).
  //
  // 4. The PCI Bus UEFI_DRIVER enumerates resources and calls into this
  //    DXE_DRIVER (CheckDevice()).
  //
  // 5. This driver remembers if EFI_LEGACY_BIOS_PROTOCOL has been installed
  //    sometime during step 1 (produced by the Legacy BIOS DXE_DRIVER).
  //
  // For breaking this order, the Legacy BIOS DXE_DRIVER would have to install
  // its protocol after the firmware enters BDS, which cannot happen.
  //
  Status = gBS->CreateEvent (EVT_NOTIFY_SIGNAL, TPL_CALLBACK,
                  LegacyBiosInstalled, NULL /* Context */, &Event);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->RegisterProtocolNotify (&gEfiLegacyBiosProtocolGuid, Event,
                  &Registration);
  if (EFI_ERROR (Status)) {
    goto CloseEvent;
  }

  Status = gBS->SignalEvent (Event);
  ASSERT_EFI_ERROR (Status);

  mIncompatiblePciDeviceSupport.CheckDevice = CheckDevice;
  Status = gBS->InstallMultipleProtocolInterfaces (&ImageHandle,
                  &gEfiIncompatiblePciDeviceSupportProtocolGuid,
                  &mIncompatiblePciDeviceSupport, NULL);
  if (EFI_ERROR (Status)) {
    goto CloseEvent;
  }

  return EFI_SUCCESS;

CloseEvent:
  if (!mLegacyBiosInstalled) {
    EFI_STATUS CloseStatus;

    CloseStatus = gBS->CloseEvent (Event);
    ASSERT_EFI_ERROR (CloseStatus);
  }

  return Status;
}
