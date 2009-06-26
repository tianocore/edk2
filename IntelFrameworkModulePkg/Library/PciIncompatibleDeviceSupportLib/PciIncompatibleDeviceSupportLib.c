/** @file
  The template of PCI incompatible device support libary.

Copyright (c) 2006 - 2009, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IncompatiblePciDeviceList.h"

//
// the incompatible PCI devices list template for ACPI resource
//
GLOBAL_REMOVE_IF_UNREFERENCED UINT64 gIncompatiblePciDeviceListForResource[] = {
  //
  // DEVICE_INF_TAG,
  // PCI_DEVICE_ID (VendorID, DeviceID, Revision, SubVendorId, SubDeviceId),
  // DEVICE_RES_TAG,
  // ResType,  GFlag , SFlag,   Granularity,  RangeMin,
  // RangeMax, Offset, AddrLen
  //

  //
  // Sample Device 1
  //
  //DEVICE_INF_TAG,
  //PCI_DEVICE_ID(0xXXXX, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE),
  //DEVICE_RES_TAG,
  //PCI_BAR_TYPE_IO,
  //PCI_ACPI_UNUSED,
  //PCI_ACPI_UNUSED,
  //PCI_ACPI_UNUSED,
  //PCI_ACPI_UNUSED,
  //PCI_BAR_EVEN_ALIGN,
  //PCI_BAR_ALL,
  //PCI_BAR_NOCHANGE,

  //
  // Sample Device 2
  //
  //DEVICE_INF_TAG,
  //PCI_DEVICE_ID(0xXXXX, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE),
  //DEVICE_RES_TAG,
  //PCI_BAR_TYPE_IO,
  //PCI_ACPI_UNUSED,
  //PCI_ACPI_UNUSED,
  //PCI_ACPI_UNUSED,
  //PCI_ACPI_UNUSED,
  //PCI_BAR_EVEN_ALIGN,
  //PCI_BAR_ALL,
  //PCI_BAR_NOCHANGE,

  //
  // The end of the list
  //
  LIST_END_TAG
};

//
// the incompatible PCI devices list template for the values of configuration registers
//
GLOBAL_REMOVE_IF_UNREFERENCED UINT64 gIncompatiblePciDeviceListForRegister[] = {
  //
  // DEVICE_INF_TAG,
  // PCI_DEVICE_ID (VendorID, DeviceID, Revision, SubVendorId, SubDeviceId),
  // PCI_RES_TAG,
  // PCI_ACCESS_TYPE, PCI_CONFIG_ADDRESS,
  // AND_VALUE, OR_VALUE

  //
  // Sample Device 1
  //
  //DEVICE_INF_TAG,
  //PCI_DEVICE_ID(0xXXXX, 0xXXXX, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE),
  //DEVICE_RES_TAG,
  //PCI_REGISTER_READ,
  //PCI_CAPBILITY_POINTER_OFFSET,
  //0xffffff00,
  //VALUE_NOCARE,

  //
  // Sample Device 2
  //
  //DEVICE_INF_TAG,
  //PCI_DEVICE_ID(0xXXXX, 0xXXXX, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE),
  //DEVICE_RES_TAG,
  //PCI_REGISTER_READ,
  //PCI_CAPBILITY_POINTER_OFFSET,
  //0xffffff00,
  //VALUE_NOCARE,

  //
  // The end of the list
  //
  LIST_END_TAG
};

//
// the incompatible PCI devices list template for the access width of configuration registers
//
GLOBAL_REMOVE_IF_UNREFERENCED UINT64 gDeviceListForAccessWidth[] = {
  //
  // DEVICE_INF_TAG,
  // PCI_DEVICE_ID (VendorID, DeviceID, Revision, SubVendorId, SubDeviceId),
  // DEVICE_RES_TAG,
  // PCI_ACCESS_TYPE, PCI_ACCESS_WIDTH,
  // START_ADDRESS, END_ADDRESS,
  // ACTUAL_PCI_ACCESS_WIDTH,
  //

  //
  // Sample Device
  //
  //DEVICE_INF_TAG,
  //PCI_DEVICE_ID(0xXXXX, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE),
  //DEVICE_RES_TAG,
  //PCI_REGISTER_READ,
  //EfiPciWidthUint8,
  //0,
  //0xFF,
  //EfiPciWidthUint32,
  //

  //
  // The end of the list
  //
  LIST_END_TAG
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_PCI_REGISTER_ACCESS_DATA mPciRegisterAccessData = {0, 0, 0}; 
GLOBAL_REMOVE_IF_UNREFERENCED EFI_PCI_REGISTER_VALUE_DATA  mPciRegisterValueData  = {0, 0};
 

/**
  Check whether two PCI devices matched.

  @param  PciDeviceInfo      A pointer to EFI_PCI_DEVICE_INFO.
  @param  Header             A pointer to EFI_PCI_DEVICE_INFO.

  @retval EFI_SUCCESS        Two PCI devices matched.
  @retval EFI_UNSUPPORTED    Two PCI devices don't match.

**/
EFI_STATUS
DeviceCheck (
  IN  EFI_PCI_DEVICE_INFO      *PciDeviceInfo,
  IN  EFI_PCI_DEVICE_INFO      *Header
  )
{
  //
  // See if the Header matches the parameters passed in
  //
  if (Header->VendorID != DEVICE_ID_NOCARE) {
    if (PciDeviceInfo->VendorID != Header->VendorID) {
      return EFI_UNSUPPORTED;
    }
  }

  if (Header->DeviceID != DEVICE_ID_NOCARE) {
    if (PciDeviceInfo->DeviceID != Header->DeviceID) {
      return EFI_UNSUPPORTED;
    }
  }

  if (Header->RevisionID != DEVICE_ID_NOCARE) {
    if (PciDeviceInfo->RevisionID != Header->RevisionID) {
      return EFI_UNSUPPORTED;
    }
  }

  if (Header->SubsystemVendorID != DEVICE_ID_NOCARE) {
    if (PciDeviceInfo->SubsystemVendorID != Header->SubsystemVendorID) {
      return EFI_UNSUPPORTED;
    }
  }

  if (Header->SubsystemID != DEVICE_ID_NOCARE) {
    if (PciDeviceInfo->SubsystemID != Header->SubsystemID) {
      return EFI_UNSUPPORTED;
    }
  }

  return EFI_SUCCESS;
}


/**
  Check the incompatible device list for ACPI resource update and return
  the configuration.

  This function searches the incompatible device list according to request
  information. If the PCI device belongs to the devices list, corresponding
  configuration informtion will be returned, in the meantime return EFI_SUCCESS.

  @param  PciDeviceInfo        A pointer to PCI device information.
  @param  Configuration        Returned information.

  @retval EFI_SUCCESS          If check incompatible device successfully.
  @retval EFI_ABORTED          No any resource type.
  @retval EFI_OUT_OF_RESOURCES No memory available.
  @retval EFI_UNSUPPORTED      Invalid Tag encounted.

**/
EFI_STATUS
EFIAPI
PciResourceUpdateCheck (
  IN  EFI_PCI_DEVICE_INFO           *PciDeviceInfo,
  OUT VOID                          *Configuration
  )
{
  UINT64                            Tag;
  UINT64                            *ListPtr;
  UINT64                            *TempListPtr;
  EFI_PCI_DEVICE_INFO               *Header;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *AcpiPtr;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *OldAcpiPtr;
  EFI_PCI_RESOUCE_DESCRIPTOR        *Dsc;
  EFI_ACPI_END_TAG_DESCRIPTOR       *PtrEnd;
  UINTN                             Index;

  ASSERT (PciDeviceInfo != NULL);

  //
  // Initialize the return value to NULL
  //
  * (VOID **) Configuration = NULL;

  ListPtr                   = gIncompatiblePciDeviceListForResource;
  while (*ListPtr != LIST_END_TAG) {

    Tag = *ListPtr;

    switch (Tag) {
    case DEVICE_INF_TAG:
      Header  = (EFI_PCI_DEVICE_INFO *) (ListPtr + 1);
      ListPtr = ListPtr + 1 + sizeof (EFI_PCI_DEVICE_INFO) / sizeof (UINT64);

      if (DeviceCheck (PciDeviceInfo, Header) != EFI_SUCCESS) {
        continue;
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
      // allocate a acpi resource node
      //
      if (Index == 0) {
        return EFI_ABORTED;
      }

      AcpiPtr = AllocateZeroPool (
                  sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) * Index + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR)
                  );
      if (AcpiPtr == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      OldAcpiPtr = AcpiPtr;

      //
      //   Fill the EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR structure
      //   according to the EFI_PCI_RESOUCE_DESCRIPTOR structure
      //
      for (; *ListPtr == DEVICE_RES_TAG;) {

        Dsc = (EFI_PCI_RESOUCE_DESCRIPTOR *) (ListPtr + 1);

        AcpiPtr->Desc = ACPI_ADDRESS_SPACE_DESCRIPTOR;
        AcpiPtr->Len = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);
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
      // put the checksum
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

/**
  Check the incompatible device list and return configuraton register mask values.

  This function searches the incompatible device list according to request
  information. If the PCI device belongs to the devices list, corresponding
  configuration informtion will be returned, in the meantime return EFI_SUCCESS.

  @param  PciDeviceInfo       A pointer to EFI_PCI_DEVICE_INFO.
  @param  AccessType          Access Type, READ or WRITE.
  @param  Offset              The address within the PCI configuration space.
  @param  Configuration       Returned information.

  @retval EFI_SUCCESS         If check incompatible device successfully.
  @retval EFI_UNSUPPORTED     Failed to check incompatibility device.

**/
EFI_STATUS
EFIAPI
PciRegisterUpdateCheck (
  IN  EFI_PCI_DEVICE_INFO           *PciDeviceInfo,
  IN  UINT64                        AccessType,
  IN  UINT64                        Offset,
  OUT VOID                          *Configuration
  )
{
  EFI_PCI_DEVICE_INFO               *Header;
  UINT64                            Tag;
  UINT64                            *ListPtr;
  EFI_PCI_REGISTER_VALUE_DATA       *RegisterPtr;
  EFI_PCI_REGISTER_VALUE_DATA       *Dsc;

  ASSERT (PciDeviceInfo != NULL);

  ListPtr                   = gIncompatiblePciDeviceListForRegister;

  //
  // Initialize the return value to NULL
  //
  * (VOID **) Configuration = NULL;

  while (*ListPtr != LIST_END_TAG) {

    Tag = *ListPtr;

    switch (Tag) {
    case DEVICE_INF_TAG:
      Header  = (EFI_PCI_DEVICE_INFO *) (ListPtr + 1);
      ListPtr = ListPtr + 1 + sizeof (EFI_PCI_DEVICE_INFO) / sizeof (UINT64);

      //
      // Check whether the PCI device matches the device in the incompatible devices list?
      // If not, ship next
      //
      if (DeviceCheck (PciDeviceInfo, Header) != EFI_SUCCESS) {
        continue;
      }

      //
      // Matched an item, check whether access matches?
      //
      for (; *ListPtr == DEVICE_RES_TAG;) {
        ListPtr ++;
        if (((EFI_PCI_REGISTER_VALUE_DESCRIPTOR *)ListPtr)->Offset == (Offset & 0xfc)) {
          if (((EFI_PCI_REGISTER_VALUE_DESCRIPTOR *)ListPtr)->AccessType == AccessType) {

            Dsc = (EFI_PCI_REGISTER_VALUE_DATA *) (ListPtr + 2);
 
            RegisterPtr = &mPciRegisterValueData;

            RegisterPtr->AndValue      = Dsc->AndValue;
            RegisterPtr->OrValue       = Dsc->OrValue;

            *(VOID **) Configuration   = RegisterPtr;

            return EFI_SUCCESS;
          }
        }
        ListPtr += sizeof (EFI_PCI_REGISTER_VALUE_DESCRIPTOR) / (sizeof (UINT64));
      }
      return EFI_UNSUPPORTED;

    case DEVICE_RES_TAG:
      //
      // Adjust the pointer to the next item
      //
      ListPtr = ListPtr + 1 + ((sizeof (EFI_PCI_REGISTER_VALUE_DESCRIPTOR)) / sizeof (UINT64));
      break;

    default:
      return EFI_UNSUPPORTED;
    }
  }

  return EFI_UNSUPPORTED;
}

/**
  Check the incompatible device list for access width incompatibility and
  return the configuration.

  This function searches the incompatible device list for access width
  incompatibility according to request information. If the PCI device
  belongs to the devices list, corresponding configuration informtion
  will be returned, in the meantime return EFI_SUCCESS.

  @param  PciDeviceInfo       A pointer to PCI device information.
  @param  AccessType          Access type, READ or WRITE.
  @param  Offset              The address within the PCI configuration space.
  @param  AccessWidth         Access width needs to check incompatibility.
  @param  Configuration       Returned information.

  @retval EFI_SUCCESS         If check incompatible device successfully.
  @retval EFI_UNSUPPORTED     Failed to check incompatibility device.

**/
EFI_STATUS
EFIAPI
PciRegisterAccessCheck (
  IN  EFI_PCI_DEVICE_INFO           *PciDeviceInfo,
  IN  UINT64                        AccessType,
  IN  UINT64                        Offset,
  IN  UINT64                        AccessWidth,
  OUT VOID                          *Configuration
  )
{
  EFI_PCI_DEVICE_INFO                *Header;
  UINT64                             Tag;
  UINT64                             *ListPtr;
  EFI_PCI_REGISTER_ACCESS_DATA       *RegisterPtr;
  EFI_PCI_REGISTER_ACCESS_DATA       *Dsc;

  ASSERT (PciDeviceInfo != NULL);

  ListPtr                   = gDeviceListForAccessWidth;

  //
  // Initialize the return value to NULL
  //
  * (VOID **) Configuration = NULL;

  while (*ListPtr != LIST_END_TAG) {

    Tag = *ListPtr;

    switch (Tag) {
    case DEVICE_INF_TAG:
      Header  = (EFI_PCI_DEVICE_INFO *) (ListPtr + 1);
      ListPtr = ListPtr + 1 + sizeof (EFI_PCI_DEVICE_INFO) / sizeof (UINT64);

      //
      // Check whether the PCI device matches the device in the incompatible devices list?
      // If not, ship next
      //
      if (DeviceCheck (PciDeviceInfo, Header) != EFI_SUCCESS) {
        continue;
      }

      //
      // Matched an item, check whether access matches?
      //
      for (; *ListPtr == DEVICE_RES_TAG;) {
        ListPtr ++;
        if (((EFI_PCI_REGISTER_ACCESS_DESCRIPTOR *) ListPtr)->AccessType == AccessType &&
            ((EFI_PCI_REGISTER_ACCESS_DESCRIPTOR *) ListPtr)->AccessWidth == AccessWidth ) {

          Dsc = (EFI_PCI_REGISTER_ACCESS_DATA *) (ListPtr + 2);

          if((Dsc->StartOffset <= Offset) && (Dsc->EndOffset > Offset)) {

            RegisterPtr = &mPciRegisterAccessData;

            RegisterPtr->StartOffset      = Dsc->StartOffset;
            RegisterPtr->EndOffset        = Dsc->EndOffset;
            RegisterPtr->Width            = Dsc->Width;

            *(VOID **) Configuration  = RegisterPtr;

            return EFI_SUCCESS;
          }
        }
        ListPtr += sizeof (EFI_PCI_REGISTER_ACCESS_DESCRIPTOR) / (sizeof (UINT64));
      }
      return EFI_UNSUPPORTED;

    case DEVICE_RES_TAG:
      //
      // Adjust the pointer to the next item
      //
      ListPtr = ListPtr + 1 + ((sizeof (EFI_PCI_REGISTER_ACCESS_DESCRIPTOR)) / sizeof (UINT64));
      break;

    default:
      return EFI_UNSUPPORTED;
    }
  }

  return EFI_UNSUPPORTED;
}

