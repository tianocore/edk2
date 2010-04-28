/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  Smbus2.h
    
Abstract:

  Smbus2 PPI as defined in PI 1.0

--*/

#ifndef _PEI_SMBUS2_PPI_H
#define _PEI_SMBUS2_PPI_H

#include "EfiSmbus.h"

#define PEI_SMBUS2_PPI_GUID \
  { \
    0x9ca93627, 0xb65b, 0x4324, {0xa2, 0x2, 0xc0, 0xb4, 0x61, 0x76, 0x45, 0x43} \
  }

EFI_FORWARD_DECLARATION (EFI_PEI_SMBUS2_PPI);

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SMBUS2_PPI_EXECUTE_OPERATION) (
  IN CONST EFI_PEI_SMBUS2_PPI       * This,
  IN      EFI_SMBUS_DEVICE_ADDRESS  SlaveAddress,
  IN      EFI_SMBUS_DEVICE_COMMAND  Command,
  IN      EFI_SMBUS_OPERATION       Operation,
  IN      BOOLEAN                   PecCheck,
  IN OUT  UINTN                     *Length,
  IN OUT  VOID                      *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SMBUS_NOTIFY2_FUNCTION) (
  IN  CONST EFI_PEI_SMBUS2_PPI          * SmbusPpi,
  IN      EFI_SMBUS_DEVICE_ADDRESS      SlaveAddress,
  IN      UINTN                         Data
  );

//
// If ArpAll is TRUE, SmbusUdid/SlaveAddress is Optional.
// If FALSE, ArpDevice will enum SmbusUdid and the address will be at SlaveAddress
//
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SMBUS2_PPI_ARP_DEVICE) (
  IN CONST EFI_PEI_SMBUS2_PPI       * This,
  IN      BOOLEAN                   ArpAll,
  IN      EFI_SMBUS_UDID            * SmbusUdid, OPTIONAL
  IN OUT  EFI_SMBUS_DEVICE_ADDRESS  * SlaveAddress OPTIONAL
  );


typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SMBUS2_PPI_GET_ARP_MAP) (
  IN CONST EFI_PEI_SMBUS2_PPI       * This,
  IN OUT  UINTN                     *Length,
  IN OUT  EFI_SMBUS_DEVICE_MAP      **SmbusDeviceMap
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SMBUS2_PPI_NOTIFY) (
  IN CONST EFI_PEI_SMBUS2_PPI       * This,
  IN      EFI_SMBUS_DEVICE_ADDRESS  SlaveAddress,
  IN      UINTN                     Data,
  IN      EFI_PEI_SMBUS_NOTIFY2_FUNCTION NotifyFunction
  );

struct _EFI_PEI_SMBUS2_PPI {
  EFI_PEI_SMBUS2_PPI_EXECUTE_OPERATION Execute;
  EFI_PEI_SMBUS2_PPI_ARP_DEVICE        ArpDevice;
  EFI_PEI_SMBUS2_PPI_GET_ARP_MAP       GetArpMap;
  EFI_PEI_SMBUS2_PPI_NOTIFY            Notify;
  EFI_GUID                             Identifier;
};

extern EFI_GUID gPeiSmbus2PpiGuid;

#endif
