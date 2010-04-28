/*++
 
Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  TianoDevicePath.h

Abstract:

  Tiano Device Path definitions

--*/

#ifndef _TIANO_DEVICE_PATH_H
#define _TIANO_DEVICE_PATH_H

#include "EfiDevicePath.h"
#include "TianoSpecDevicePath.h"

#pragma pack(1)

typedef struct _USB_PORT_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH      PciRootBridgeDevicePath;
  PCI_DEVICE_PATH           PciBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} USB_PORT_DEVICE_PATH;

//
// IDE
//
typedef struct _IDE_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH      PciRootBridgeDevicePath;
  PCI_DEVICE_PATH           PciBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} IDE_DEVICE_PATH;

//
// RMC Connector
//
typedef struct _RMC_CONN_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH      PciRootBridgeDevicePath;
  PCI_DEVICE_PATH           PciBridgeDevicePath;
  PCI_DEVICE_PATH           PciBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} RMC_CONN_DEVICE_PATH;

//
// RIDE
//
typedef struct _RIDE_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH      PciRootBridgeDevicePath;
  PCI_DEVICE_PATH           PciBridgeDevicePath;
  PCI_DEVICE_PATH           PciBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} RIDE_DEVICE_PATH;

//
// Gigabit NIC
//
typedef struct _GB_NIC_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH      PciRootBridgeDevicePath;
  PCI_DEVICE_PATH           PciBridgeDevicePath;
  PCI_DEVICE_PATH           PciXBridgeDevicePath;
  PCI_DEVICE_PATH           PciXBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} GB_NIC_DEVICE_PATH;

//
// P/S2 Connector
//
typedef struct _PS2_CONN_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH      PciRootBridgeDevicePath;
  PCI_DEVICE_PATH           LpcBridgeDevicePath;
  ACPI_HID_DEVICE_PATH      LpcBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} PS2_CONN_DEVICE_PATH;

//
// Serial Port Connector
//
typedef struct _SERIAL_CONN_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH      PciRootBridgeDevicePath;
  PCI_DEVICE_PATH           LpcBridgeDevicePath;
  ACPI_HID_DEVICE_PATH      LpcBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} SERIAL_CONN_DEVICE_PATH;

//
// Parallel Port Connector
//
typedef struct _PARALLEL_CONN_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH      PciRootBridgeDevicePath;
  PCI_DEVICE_PATH           LpcBridgeDevicePath;
  ACPI_HID_DEVICE_PATH      LpcBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} PARALLEL_CONN_DEVICE_PATH;

//
// Floopy Connector
//
typedef struct _FLOOPY_CONN_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH      PciRootBridgeDevicePath;
  PCI_DEVICE_PATH           LpcBridgeDevicePath;
  ACPI_HID_DEVICE_PATH      LpcBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} FLOOPY_CONN_DEVICE_PATH;

typedef union _EFI_MISC_PORT_DEVICE_PATH {
  USB_PORT_DEVICE_PATH      UsbDevicePath;
  IDE_DEVICE_PATH           IdeDevicePath;
  RMC_CONN_DEVICE_PATH      RmcConnDevicePath;
  RIDE_DEVICE_PATH          RideDevicePath;
  GB_NIC_DEVICE_PATH        GbNicDevicePath;
  PS2_CONN_DEVICE_PATH      Ps2ConnDevicePath;
  SERIAL_CONN_DEVICE_PATH   SerialConnDevicePath;
  PARALLEL_CONN_DEVICE_PATH ParallelConnDevicePath;
  FLOOPY_CONN_DEVICE_PATH   FloppyConnDevicePath;
} EFI_MISC_PORT_DEVICE_PATH;

#pragma pack()

#endif
