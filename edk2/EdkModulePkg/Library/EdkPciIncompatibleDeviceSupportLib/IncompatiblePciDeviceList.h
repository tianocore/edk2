/** @file
  The incompatible PCI device list

Copyright (c) 2007 Intel Corporation. All rights reserved. <BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

**/

#ifndef _EFI_INCOMPATIBLE_PCI_DEVICE_LIST_H
#define _EFI_INCOMPATIBLE_PCI_DEVICE_LIST_H

#include <IndustryStandard/pci22.h>
#include <IndustryStandard/Acpi.h>


#define PCI_DEVICE_ID(VendorId, DeviceId, Revision, SubVendorId, SubDeviceId) \
    VendorId, DeviceId, Revision, SubVendorId, SubDeviceId

#define PCI_BAR_TYPE_IO   ACPI_ADDRESS_SPACE_TYPE_IO
#define PCI_BAR_TYPE_MEM  ACPI_ADDRESS_SPACE_TYPE_MEM

#define DEVICE_INF_TAG    0xFFF2
#define DEVICE_RES_TAG    0xFFF1
#define LIST_END_TAG      0x0000

//
// descriptor for access width of incompatible PCI device
//
typedef struct {
  UINT64                         AccessType;
  UINT64                         AccessWidth;
  EFI_PCI_REGISTER_ACCESS_DATA   PciRegisterAccessData;
} EFI_PCI_REGISTER_ACCESS_DESCRIPTOR;

//
// descriptor for register value of incompatible PCI device
//
typedef struct {
  UINT64                         AccessType;
  UINT64                         Offset;
  EFI_PCI_REGISTER_VALUE_DATA    PciRegisterValueData;
} EFI_PCI_REGISTER_VALUE_DESCRIPTOR;


//
// the incompatible PCI devices list for ACPI resource
//
GLOBAL_REMOVE_IF_UNREFERENCED UINT64 IncompatiblePciDeviceListForResource[] = {
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
  PCI_DEVICE_ID(0x9004, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE),
  DEVICE_RES_TAG,
  PCI_BAR_TYPE_IO,
  PCI_ACPI_UNUSED,
  PCI_ACPI_UNUSED,
  PCI_ACPI_UNUSED,
  PCI_ACPI_UNUSED,
  PCI_BAR_EVEN_ALIGN,
  PCI_BAR_ALL,
  PCI_BAR_NOCHANGE,
  //
  // Device Adaptec 9005
  //
  DEVICE_INF_TAG,
  PCI_DEVICE_ID(0x9005, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE),
  DEVICE_RES_TAG,
  PCI_BAR_TYPE_IO,
  PCI_ACPI_UNUSED,
  PCI_ACPI_UNUSED,
  PCI_ACPI_UNUSED,
  PCI_ACPI_UNUSED,
  PCI_BAR_EVEN_ALIGN,
  PCI_BAR_ALL,
  PCI_BAR_NOCHANGE,
  //
  // Device QLogic  1007
  //
  DEVICE_INF_TAG,
  PCI_DEVICE_ID(0x1077, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE),
  DEVICE_RES_TAG,
  PCI_BAR_TYPE_IO,
  PCI_ACPI_UNUSED,
  PCI_ACPI_UNUSED,
  PCI_ACPI_UNUSED,
  PCI_ACPI_UNUSED,
  PCI_BAR_EVEN_ALIGN,
  PCI_BAR_ALL,
  PCI_BAR_NOCHANGE,
  //
  // Device Agilent 103C
  //
  DEVICE_INF_TAG,
  PCI_DEVICE_ID(0x103C, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE),
  DEVICE_RES_TAG,
  PCI_BAR_TYPE_IO,
  PCI_ACPI_UNUSED,
  PCI_ACPI_UNUSED,
  PCI_ACPI_UNUSED,
  PCI_ACPI_UNUSED,
  PCI_BAR_EVEN_ALIGN,
  PCI_BAR_ALL,
  PCI_BAR_NOCHANGE,
  //
  // Device Agilent 15BC
  //
  DEVICE_INF_TAG,
  PCI_DEVICE_ID(0x15BC, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE),
  DEVICE_RES_TAG,
  PCI_BAR_TYPE_IO,
  PCI_ACPI_UNUSED,
  PCI_ACPI_UNUSED,
  PCI_ACPI_UNUSED,
  PCI_ACPI_UNUSED,
  PCI_BAR_EVEN_ALIGN,
  PCI_BAR_ALL,
  PCI_BAR_NOCHANGE,
  //
  // The end of the list
  //
  LIST_END_TAG
};

//
// the incompatible PCI devices list for the values of configuration registers
//
GLOBAL_REMOVE_IF_UNREFERENCED UINT64 IncompatiblePciDeviceListForRegister[] = {
  //
  // DEVICE_INF_TAG,
  // PCI_DEVICE_ID (VendorID, DeviceID, Revision, SubVendorId, SubDeviceId),
  // PCI_RES_TAG,
  // PCI_ACCESS_TYPE, PCI_CONFIG_ADDRESS,
  // AND_VALUE, OR_VALUE

  //
  // Device Lava 0x1407, DeviceId 0x0110
  //
  DEVICE_INF_TAG,
  PCI_DEVICE_ID(0x1407, 0x0110, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE),
  DEVICE_RES_TAG,
  PCI_REGISTER_READ,
  PCI_CAPBILITY_POINTER_OFFSET,
  0xffffff00,
  VALUE_NOCARE,

  //
  // Device Lava 0x1407, DeviceId 0x0111
  //
  DEVICE_INF_TAG,
  PCI_DEVICE_ID(0x1407, 0x0111, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE),
  DEVICE_RES_TAG,
  PCI_REGISTER_READ,
  PCI_CAPBILITY_POINTER_OFFSET,
  0xffffff00,
  VALUE_NOCARE,

  //
  // The end of the list
  //
  LIST_END_TAG
};

//
// the incompatible PCI devices list for the access width of configuration registers
//
GLOBAL_REMOVE_IF_UNREFERENCED UINT64 DeviceListForAccessWidth[] = {
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

#endif
