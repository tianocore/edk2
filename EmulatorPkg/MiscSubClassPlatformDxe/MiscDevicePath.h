/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  MiscDevicePath.h

Abstract:

  Misc class required EFI Device Path definitions (Ports, slots &
  onboard devices)

**/

#ifndef _MISC_DEVICE_PATH_H
#define _MISC_DEVICE_PATH_H


#pragma pack(1)
//
// USB
//

/* For reference:
#define USB1_1_STR  "ACPI(PNP0A03,0)/PCI(1D,0)."
#define USB1_2_STR  "ACPI(PNP0A03,0)/PCI(1D,1)."
#define USB1_3_STR  "ACPI(PNP0A03,0)/PCI(1D,2)."
#define USB2_1_STR  "ACPI(PNP0A03,0)/PCI(1D,7)."
*/

//
// #define acpi { 0x02, 0x01, 0x00, 0x0C, 0x0a0341d0, 0x00000000 }
// #define pci( device,function)  { 0x01, 0x01, 0x00, 0x06, device, function }
// #define end  { 0xFF, 0xFF, 0x00, 0x04 }
//
#define DP_ACPI \
  { \
      {ACPI_DEVICE_PATH, ACPI_DP, {(UINT8) (sizeof (ACPI_HID_DEVICE_PATH)), (UINT8) \
      ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8)}}, EISA_PNP_ID (0x0A03), 0 \
  }
#define DP_PCI(device, function) \
  { \
      {HARDWARE_DEVICE_PATH, HW_PCI_DP, {(UINT8) (sizeof (PCI_DEVICE_PATH)), (UINT8) \
       ((sizeof (PCI_DEVICE_PATH)) >> 8)}}, function, device \
  }
#define DP_END \
  { \
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, {END_DEVICE_PATH_LENGTH, 0} \
  }

#define DP_LPC(eisaid, function) \
  { \
    {ACPI_DEVICE_PATH, ACPI_DP, {(UINT8) (sizeof (ACPI_HID_DEVICE_PATH)), (UINT8) \
     ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8)}}, EISA_PNP_ID (eisaid), function \
  }

//
// Shanmu >> moved to TianoDevicePath.h
//

/*
typedef struct _USB_PORT_DEVICE_PATH
{
  ACPI_HID_DEVICE_PATH    PciRootBridgeDevicePath;
  PCI_DEVICE_PATH      PciBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} USB_PORT_DEVICE_PATH;


//IDE ??I am not sure. Should this be ATAPI_DEVICE_PATH
typedef struct _IDE_DEVICE_PATH
{
  ACPI_HID_DEVICE_PATH    PciRootBridgeDevicePath;
  PCI_DEVICE_PATH      PciBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} IDE_DEVICE_PATH;

//RMC Connector
typedef struct _RMC_CONN_DEVICE_PATH
{
  ACPI_HID_DEVICE_PATH    PciRootBridgeDevicePath;
  PCI_DEVICE_PATH      PciBridgeDevicePath;
  PCI_DEVICE_PATH      PciBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} RMC_CONN_DEVICE_PATH;

//static RMC_CONN_DEVICE_PATH mRmcConnDevicePath = { acpi, pci( 0x1E,0x00 ),pci( 0x0A,0x00 ), end };

//RIDE
typedef struct _RIDE_DEVICE_PATH
{
  ACPI_HID_DEVICE_PATH    PciRootBridgeDevicePath;
  PCI_DEVICE_PATH      PciBridgeDevicePath;
  PCI_DEVICE_PATH      PciBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} RIDE_DEVICE_PATH;

//static RIDE_DEVICE_PATH mRideDevicePath = { acpi, pci( 0x1E,0x00 ),pci( 0x02,0x00 ), end };

//Gigabit NIC
//typedef struct _GB_NIC_DEVICE_PATH
//{
//  ACPI_HID_DEVICE_PATH      PciRootBridgeDevicePath;
//  PCI_DEVICE_PATH            PciBridgeDevicePath;
//  PCI_DEVICE_PATH            PciXBridgeDevicePath;
//  PCI_DEVICE_PATH            PciXBusDevicePath;
//  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
//} GB_NIC_DEVICE_PATH;

//static GB_NIC_DEVICE_PATH mGbNicDevicePath = { acpi, pci( 0x03,0x00 ),pci( 0x1F,0x00 ),pci( 0x07,0x00 ), end };


//P/S2 Connector
typedef struct _PS2_CONN_DEVICE_PATH
{
  ACPI_HID_DEVICE_PATH    PciRootBridgeDevicePath;
  PCI_DEVICE_PATH      LpcBridgeDevicePath;
  ACPI_HID_DEVICE_PATH    LpcBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} PS2_CONN_DEVICE_PATH;

//static PS2_CONN_DEVICE_PATH mPs2KeyboardDevicePath   = { acpi, pci( 0x1F,0x00 ),lpc( 0x0303,0 ), end };
//static PS2_CONN_DEVICE_PATH mPs2MouseDevicePath      = { acpi, pci( 0x1F,0x00 ),lpc( 0x0303,1 ), end };

//Serial Port Connector
typedef struct _SERIAL_CONN_DEVICE_PATH
{
  ACPI_HID_DEVICE_PATH    PciRootBridgeDevicePath;
  PCI_DEVICE_PATH      LpcBridgeDevicePath;
  ACPI_HID_DEVICE_PATH    LpcBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} SERIAL_CONN_DEVICE_PATH;

//static SERIAL_CONN_DEVICE_PATH mCom1DevicePath   = { acpi, pci( 0x1F,0x00 ),lpc( 0x0501,0 ), end };
//static SERIAL_CONN_DEVICE_PATH mCom2DevicePath   = { acpi, pci( 0x1F,0x00 ),lpc( 0x0501,1 ), end };

//Parallel Port Connector
typedef struct _PARALLEL_CONN_DEVICE_PATH
{
  ACPI_HID_DEVICE_PATH    PciRootBridgeDevicePath;
  PCI_DEVICE_PATH      LpcBridgeDevicePath;
  ACPI_HID_DEVICE_PATH    LpcBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} PARALLEL_CONN_DEVICE_PATH;

//static PARALLEL_CONN_DEVICE_PATH mLpt1DevicePath   = { acpi, pci( 0x1F,0x00 ),lpc( 0x0401,0 ), end };

//Floopy Connector
typedef struct _FLOOPY_CONN_DEVICE_PATH
{
  ACPI_HID_DEVICE_PATH    PciRootBridgeDevicePath;
  PCI_DEVICE_PATH      LpcBridgeDevicePath;
  ACPI_HID_DEVICE_PATH    LpcBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} FLOOPY_CONN_DEVICE_PATH;

//static FLOOPY_CONN_DEVICE_PATH mFloopyADevicePath   = { acpi, pci( 0x1F,0x00 ),lpc( 0x0604,0 ), end };
//static FLOOPY_CONN_DEVICE_PATH mFloopyBDevicePath   = { acpi, pci( 0x1F,0x00 ),lpc( 0x0604,1 ), end };

*/

//
// End Shanmu
//
#pragma pack()

#endif
