/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

**/

#ifndef __MMIO_DEVICE_H__
#define __MMIO_DEVICE_H__

//
// Protocol to define for the MMIO device
//
typedef struct {
  //
  // Address of a GUID
  //
  EFI_GUID *Guid;

  //
  // Context for the protocol
  //
  VOID *Context;
} EFI_MMIO_DEVICE_PROTOCOL_ITEM;


typedef struct _EFI_MMIO_DEVICE_PROTOCOL  EFI_MMIO_DEVICE_PROTOCOL;

//
//  The MMIO device protocol defines a memory mapped I/O device
//  for use by the system.
//
struct _EFI_MMIO_DEVICE_PROTOCOL {
  //
  // Pointer to an ACPI_EXTENDED_HID_DEVICE_PATH structure
  // containing HID/HidStr and CID/CidStr values.
  //
  // See the note below associated with the UnitIdentification
  // field.
  //
  CONST ACPI_EXTENDED_HID_DEVICE_PATH *AcpiPath;

  //
  // Allow the use of a shared template for the AcpiPath.
  //
  // If this value is non-zero UID value then the AcpiPath must
  // be a template which contains only the HID/HidStr and CID/CidStr
  // values.  The UID/UidStr values in the AcpiPath must be zero!
  //
  // If this value is zero then the AcpiPath is not shared and
  // must contain either a non-zero UID value or a UidStr value.
  //
  UINT32 UnitIdentification;

  //
  // Hardware revision - ACPI _HRV value
  //
  UINT32 HardwareRevision;

  //
  // Pointer to a data structure containing the controller
  // resources and configuration.  At a minimum this points
  // to an EFI_PHYSICAL_ADDRESS for the base address of the
  // MMIO device.
  //
  CONST VOID *DriverResources;

  //
  // Number of protocols in the array
  //
  UINTN ProtocolCount;

  //
  // List of protocols to define
  //
  CONST EFI_MMIO_DEVICE_PROTOCOL_ITEM *ProtocolArray;
};

extern EFI_GUID gEfiMmioDeviceProtocolGuid;

#endif  //  __MMIO_DEVICE_H__
