/** @file
  Legacy BIOS Platform support

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are
  licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef LEGACY_BIOS_PLATFORM_H_
#define LEGACY_BIOS_PLATFORM_H_

#include <FrameworkDxe.h>

#include <Protocol/PciIo.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LegacyInterrupt.h>
#include <Protocol/LegacyRegion2.h>
#include <Protocol/LegacyBiosPlatform.h>
#include <Protocol/FirmwareVolume.h>
#include <Protocol/DiskInfo.h>

#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/PcdLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/DevicePathLib.h>

#include <IndustryStandard/Pci.h>

//
// PIRQ information constants.
//
#define MAX_IRQ_ROUTING_ENTRIES     6
#define MAX_IRQ_PRIORITY_ENTRIES    7

#define V_INTEL_VENDOR_ID         0x8086
#define V_PIIX4_IDE_DEVICE_ID     0x7010

//
// Type declarations
//
typedef struct {
  UINT8   SetupValue;
  UINT16  DeviceType;
  UINT8   Class;
  UINT8   SubClass;
} EFI_SETUP_BBS_MAP;

typedef struct {
  UINT8          Class;
  UINT8          SubClass;
} PCI_CLASS_RECORD;

typedef struct {
  EFI_LEGACY_PIRQ_TABLE_HEADER  PirqTable;
  EFI_LEGACY_IRQ_ROUTING_ENTRY  IrqRoutingEntry[MAX_IRQ_ROUTING_ENTRIES];
} EFI_LEGACY_PIRQ_TABLE;

typedef struct {
  EFI_HANDLE  Handle;
  UINT16      Vid;
  UINT16      Did;
  UINT16      SvId;
  UINT16      SysId;
} DEVICE_STRUCTURE;

typedef struct {
  EFI_GUID  FileName;
  UINTN     Valid;
} SYSTEM_ROM_TABLE;

typedef struct {
  UINT32                            Signature;
  EFI_HANDLE                        Handle;
  EFI_LEGACY_BIOS_PLATFORM_PROTOCOL LegacyBiosPlatform;
  EFI_HANDLE                        ImageHandle;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL   *PciRootBridgeIo;
} LEGACY_BIOS_PLATFORM_INSTANCE;

#define LEGACY_BIOS_PLATFORM_INSTANCE_SIGNATURE   SIGNATURE_32('P','B','I','O')

#define LEGACY_BIOS_PLATFORM_INSTANCE_FROM_THIS(this) \
  CR (this, \
      LEGACY_BIOS_PLATFORM_INSTANCE, \
      LegacyBiosPlatform, \
      LEGACY_BIOS_PLATFORM_INSTANCE_SIGNATURE \
      )

#endif

