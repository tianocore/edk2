/** @file
  Deal with devices that just exist in memory space.

  To follow the EFI driver model you need a root handle to start with. An
  EFI driver will have a driver binding protocol (Supported, Start, Stop)
  that is used to layer on top of a handle via a gBS->ConnectController.
  The first handle has to just be in the system to make that work. For
  PCI it is a PCI Root Bridge IO protocol that provides the root.

  On an embedded system with MMIO device we need a handle to just
  show up. That handle will have this protocol and a device path
  protocol on it.

  For an ethernet device the device path must contain a MAC address device path
  node.

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EMBEDDED_DEVICE_PROTOCOL_H__
#define __EMBEDDED_DEVICE_PROTOCOL_H__


//
// Protocol GUID
//
// BF4B9D10-13EC-43dd-8880-E90B718F27DE

#define EMBEDDED_DEVICE_PROTOCOL_GUID \
  { 0xbf4b9d10, 0x13ec, 0x43dd, { 0x88, 0x80, 0xe9, 0xb, 0x71, 0x8f, 0x27, 0xde } }



typedef struct {
  UINT16          VendorId;
  UINT16          DeviceId;
  UINT16          RevisionId;
  UINT16          SubsystemId;
  UINT16          SubsystemVendorId;
  UINT8           ClassCode[3];
  UINT8           HeaderSize;
  UINTN           BaseAddress;
} EMBEDDED_DEVICE_PROTOCOL;

extern EFI_GUID gEmbeddedDeviceGuid;

#endif


