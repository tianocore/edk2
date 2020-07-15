/** @file
  This module is one template module for Incompatible PCI Device Support protocol.
  It includes one incompatible pci devices list template.

  Incompatible PCI Device Support protocol allows the PCI bus driver to support
  resource allocation for some PCI devices that do not comply with the PCI Specification.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Protocol/IncompatiblePciDeviceSupport.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Acpi.h>

typedef struct {
  UINT64  VendorId;
  UINT64  DeviceId;
  UINT64  RevisionId;
  UINT64  SubsystemVendorId;
  UINT64  SubsystemDeviceId;
} EFI_PCI_DEVICE_HEADER_INFO;

typedef struct {
  UINT64  ResType;
  UINT64  GenFlag;
  UINT64  SpecificFlag;
  UINT64  AddrSpaceGranularity;
  UINT64  AddrRangeMin;
  UINT64  AddrRangeMax;
  UINT64  AddrTranslationOffset;
  UINT64  AddrLen;
} EFI_PCI_RESOUCE_DESCRIPTOR;

#define PCI_DEVICE_ID(VendorId, DeviceId, Revision, SubVendorId, SubDeviceId) \
    VendorId, DeviceId, Revision, SubVendorId, SubDeviceId

#define DEVICE_INF_TAG    0xFFF2
#define DEVICE_RES_TAG    0xFFF1
#define LIST_END_TAG      0x0000

#define EVEN_ALIGN        0xFFFFFFFFFFFFFFFEULL

/**
  Returns a list of ACPI resource descriptors that detail the special
  resource configuration requirements for an incompatible PCI device.

  @param  This                  Pointer to the EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL instance.
  @param  VendorId              A unique ID to identify the manufacturer of the PCI device.
  @param  DeviceId              A unique ID to identify the particular PCI device.
  @param  RevisionId            A PCI device-specific revision identifier.
  @param  SubsystemVendorId     Specifies the subsystem vendor ID.
  @param  SubsystemDeviceId     Specifies the subsystem device ID.
  @param  Configuration         A list of ACPI resource descriptors returned that detail
                                the configuration requirement.

  @retval EFI_SUCCESS           Successfully got ACPI resource for specified PCI device.
  @retval EFI_INVALID_PARAMETER Configuration is NULL.
  @retval EFI_OUT_OF_RESOURCES  No memory available.
  @retval EFI_UNSUPPORTED       The specified PCI device wasn't supported.

**/
EFI_STATUS
EFIAPI
PCheckDevice (
  IN  EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL  *This,
  IN  UINTN                                         VendorId,
  IN  UINTN                                         DeviceId,
  IN  UINTN                                         RevisionId,
  IN  UINTN                                         SubsystemVendorId,
  IN  UINTN                                         SubsystemDeviceId,
  OUT VOID                                          **Configuration
  );

//
// Handle onto which the Incompatible PCI Device List is installed
//
EFI_HANDLE                                    mIncompatiblePciDeviceSupportHandle = NULL;

//
// The Incompatible PCI Device Support Protocol instance produced by this driver
//
EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL  mIncompatiblePciDeviceSupport = {
  PCheckDevice
};

//
// The incompatible PCI devices list template
//
GLOBAL_REMOVE_IF_UNREFERENCED UINT64 mIncompatiblePciDeviceList[] = {
  //
  // DEVICE_INF_TAG,
  // PCI_DEVICE_ID (VendorID, DeviceID, Revision, SubVendorId, SubDeviceId),
  // DEVICE_RES_TAG,
  // ResType,  GFlag , SFlag,   Granularity,  RangeMin,
  // RangeMax, Offset, AddrLen
  //
  //
  // Device Adaptec 9004
  //
  DEVICE_INF_TAG,
  PCI_DEVICE_ID(0x9004, MAX_UINT64, MAX_UINT64, MAX_UINT64, MAX_UINT64),
  DEVICE_RES_TAG,
  ACPI_ADDRESS_SPACE_TYPE_IO,
  0,
  0,
  0,
  0,
  EVEN_ALIGN,
  MAX_UINT64,
  0,
  //
  // Device Adaptec 9005
  //
  DEVICE_INF_TAG,
  PCI_DEVICE_ID(0x9005, MAX_UINT64, MAX_UINT64, MAX_UINT64, MAX_UINT64),
  DEVICE_RES_TAG,
  ACPI_ADDRESS_SPACE_TYPE_IO,
  0,
  0,
  0,
  0,
  EVEN_ALIGN,
  MAX_UINT64,
  0,
  //
  // Device QLogic  1007
  //
  DEVICE_INF_TAG,
  PCI_DEVICE_ID(0x1077, MAX_UINT64, MAX_UINT64, MAX_UINT64, MAX_UINT64),
  DEVICE_RES_TAG,
  ACPI_ADDRESS_SPACE_TYPE_IO,
  0,
  0,
  0,
  0,
  EVEN_ALIGN,
  MAX_UINT64,
  0,
  //
  // Device Agilent 103C
  //
  DEVICE_INF_TAG,
  PCI_DEVICE_ID(0x103C, MAX_UINT64, MAX_UINT64, MAX_UINT64, MAX_UINT64),
  DEVICE_RES_TAG,
  ACPI_ADDRESS_SPACE_TYPE_IO,
  0,
  0,
  0,
  0,
  EVEN_ALIGN,
  MAX_UINT64,
  0,
  //
  // Device Agilent 15BC
  //
  DEVICE_INF_TAG,
  PCI_DEVICE_ID(0x15BC, MAX_UINT64, MAX_UINT64, MAX_UINT64, MAX_UINT64),
  DEVICE_RES_TAG,
  ACPI_ADDRESS_SPACE_TYPE_IO,
  0,
  0,
  0,
  0,
  EVEN_ALIGN,
  MAX_UINT64,
  0,
  //
  // The end of the list
  //
  LIST_END_TAG
};


/**
  Entry point of the incompatible pci device support code. Setup an incompatible device list template
  and install EFI Incompatible PCI Device Support protocol.

  @param ImageHandle             A handle for the image that is initializing this driver.
  @param SystemTable             A pointer to the EFI system table.

  @retval EFI_SUCCESS            Installed EFI Incompatible PCI Device Support Protocol successfully.
  @retval others                 Failed to install protocol.

**/
EFI_STATUS
EFIAPI
IncompatiblePciDeviceSupportEntryPoint (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS                         Status;

  //
  // Install EFI Incompatible PCI Device Support Protocol on a new handle
  //
  Status = gBS->InstallProtocolInterface (
                  &mIncompatiblePciDeviceSupportHandle,
                  &gEfiIncompatiblePciDeviceSupportProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mIncompatiblePciDeviceSupport
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Returns a list of ACPI resource descriptors that detail the special
  resource configuration requirements for an incompatible PCI device.

  @param  This                  Pointer to the EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL instance.
  @param  VendorId              A unique ID to identify the manufacturer of the PCI device.
  @param  DeviceId              A unique ID to identify the particular PCI device.
  @param  RevisionId            A PCI device-specific revision identifier.
  @param  SubsystemVendorId     Specifies the subsystem vendor ID.
  @param  SubsystemDeviceId     Specifies the subsystem device ID.
  @param  Configuration         A list of ACPI resource descriptors returned that detail
                                the configuration requirement.

  @retval EFI_SUCCESS           Successfully got ACPI resource for specified PCI device.
  @retval EFI_INVALID_PARAMETER Configuration is NULL.
  @retval EFI_OUT_OF_RESOURCES  No memory available.
  @retval EFI_UNSUPPORTED       The specified PCI device wasn't supported.

**/
EFI_STATUS
EFIAPI
PCheckDevice (
  IN  EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL  *This,
  IN  UINTN                                         VendorId,
  IN  UINTN                                         DeviceId,
  IN  UINTN                                         RevisionId,
  IN  UINTN                                         SubsystemVendorId,
  IN  UINTN                                         SubsystemDeviceId,
  OUT VOID                                          **Configuration
  )
{
  UINT64                            Tag;
  UINT64                            *ListPtr;
  UINT64                            *TempListPtr;
  EFI_PCI_DEVICE_HEADER_INFO        *Header;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *AcpiPtr;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *OldAcpiPtr;
  EFI_PCI_RESOUCE_DESCRIPTOR        *Dsc;
  EFI_ACPI_END_TAG_DESCRIPTOR       *PtrEnd;
  UINTN                             Index;

  //
  // Validate the parameters
  //
  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Initialize the return value to NULL
  //
  * (VOID **) Configuration = NULL;

  ListPtr                   = mIncompatiblePciDeviceList;
  while (*ListPtr != LIST_END_TAG) {

    Tag = *ListPtr;

    switch (Tag) {
    case DEVICE_INF_TAG:
      Header  = (EFI_PCI_DEVICE_HEADER_INFO *) (ListPtr + 1);
      ListPtr = ListPtr + 1 + sizeof (EFI_PCI_DEVICE_HEADER_INFO) / sizeof (UINT64);
      //
      // See if the Header matches the parameters passed in
      //
      if ((Header->VendorId != MAX_UINT64) && (VendorId != MAX_UINTN)) {
        if (Header->VendorId != VendorId) {
          continue;
        }
      }

      if ((Header->DeviceId != MAX_UINT64) && (DeviceId != MAX_UINTN)) {
        if (DeviceId != Header->DeviceId) {
          continue;
        }
      }

      if ((Header->RevisionId != MAX_UINT64) && (RevisionId != MAX_UINTN)) {
        if (RevisionId != Header->RevisionId) {
          continue;
        }
      }

      if ((Header->SubsystemVendorId != MAX_UINT64) && (SubsystemVendorId != MAX_UINTN)) {
        if (SubsystemVendorId != Header->SubsystemVendorId) {
          continue;
        }
      }

      if ((Header->SubsystemDeviceId != MAX_UINT64) && (SubsystemDeviceId != MAX_UINTN)) {
        if (SubsystemDeviceId != Header->SubsystemDeviceId) {
          continue;
        }
      }
      //
      // Matched an item, so construct the ACPI descriptor for the resource.
      //
      //
      // Count the resource items so that to allocate space
      //
      for (Index = 0, TempListPtr = ListPtr; *TempListPtr == DEVICE_RES_TAG; Index++) {
        TempListPtr = TempListPtr + 1 + ((sizeof (EFI_PCI_RESOUCE_DESCRIPTOR)) / sizeof (UINT64));
      }
      //
      // If there is at least one type of resource request,
      // allocate an acpi resource node
      //
      if (Index == 0) {
        return EFI_UNSUPPORTED;
      }

      AcpiPtr = AllocateZeroPool (sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) * Index + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR));
      if (AcpiPtr == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      OldAcpiPtr = AcpiPtr;
      //
      // Fill the EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR structure
      // according to the EFI_PCI_RESOUCE_DESCRIPTOR structure
      //
      for (; *ListPtr == DEVICE_RES_TAG;) {

        Dsc = (EFI_PCI_RESOUCE_DESCRIPTOR *) (ListPtr + 1);

        AcpiPtr->Desc = ACPI_ADDRESS_SPACE_DESCRIPTOR;
        AcpiPtr->Len = (UINT16) sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3;
        AcpiPtr->ResType = (UINT8) Dsc->ResType;
        AcpiPtr->GenFlag = (UINT8) Dsc->GenFlag;
        AcpiPtr->SpecificFlag = (UINT8) Dsc->SpecificFlag;
        AcpiPtr->AddrSpaceGranularity = Dsc->AddrSpaceGranularity;;
        AcpiPtr->AddrRangeMin = Dsc->AddrRangeMin;
        AcpiPtr->AddrRangeMax = Dsc->AddrRangeMax;
        AcpiPtr->AddrTranslationOffset = Dsc->AddrTranslationOffset;
        AcpiPtr->AddrLen = Dsc->AddrLen;

        ListPtr = ListPtr + 1 + ((sizeof (EFI_PCI_RESOUCE_DESCRIPTOR)) / sizeof (UINT64));
        AcpiPtr++;
      }
      //
      // Put the checksum
      //
      PtrEnd                    = (EFI_ACPI_END_TAG_DESCRIPTOR *) (AcpiPtr);
      PtrEnd->Desc              = ACPI_END_TAG_DESCRIPTOR;
      PtrEnd->Checksum          = 0;

      *(VOID **) Configuration  = OldAcpiPtr;

      return EFI_SUCCESS;

    case DEVICE_RES_TAG:
      //
      // Adjust the pointer to the next PCI resource descriptor item
      //
      ListPtr = ListPtr + 1 + ((sizeof (EFI_PCI_RESOUCE_DESCRIPTOR)) / sizeof (UINT64));
      break;

    default:
      return EFI_UNSUPPORTED;
    }
  }

  return EFI_UNSUPPORTED;
}

