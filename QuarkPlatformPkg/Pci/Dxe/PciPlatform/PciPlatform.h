/** @file
This code supports a the private implementation
of the Legacy BIOS Platform protocol

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef PCI_PLATFORM_H_
#define PCI_PLATFORM_H_

#include <IndustryStandard/Pci.h>
#include <Library/PcdLib.h>
//
// Global variables for Option ROMs
//
#define NULL_ROM_FILE_GUID \
{ 0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}

#define ONBOARD_VIDEO_OPTION_ROM_FILE_GUID \
{ 0x8dfae5d4, 0xb50e, 0x4c10, {0x96, 0xe6, 0xf2, 0xc2, 0x66, 0xca, 0xcb, 0xb6 }}

#define IDE_RAID_OPTION_ROM_FILE_GUID \
{ 0x3392A8E1, 0x1881, 0x4398, {0x83, 0xa6, 0x53, 0xd3, 0x87, 0xdb, 0x20, 0x20 }}

#define TANX_UNDI_OPTION_ROM_FILE_GUID \
{ 0x84c24ab0, 0x124e, 0x4aed, {0x8e, 0xfe, 0xf9, 0x1b, 0xb9, 0x73, 0x69, 0xf4 }}

#define PXE_UNDI_OPTION_ROM_FILE_GUID \
{ 0xea34cd48, 0x5fdf, 0x46f0, {0xb5, 0xfa, 0xeb, 0xe0, 0x76, 0xa4, 0xf1, 0x2c }}


typedef struct {
  EFI_GUID  FileName;
  UINTN     Segment;
  UINTN     Bus;
  UINTN     Device;
  UINTN     Function;
  UINT16    VendorId;
  UINT16    DeviceId;
} PCI_OPTION_ROM_TABLE;


EFI_STATUS
PhaseNotify (
  IN EFI_PCI_PLATFORM_PROTOCOL                        *This,
  IN EFI_HANDLE                                       HostBridge,
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE    Phase,
  IN EFI_PCI_CHIPSET_EXECUTION_PHASE                  ChipsetPhase
  );


EFI_STATUS
PlatformPrepController (
  IN  EFI_PCI_PLATFORM_PROTOCOL                      *This,
  IN  EFI_HANDLE                                     HostBridge,
  IN  EFI_HANDLE                                     RootBridge,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS    PciAddress,
  IN  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE   Phase,
  IN  EFI_PCI_CHIPSET_EXECUTION_PHASE                ChipsetPhase
  );

EFI_STATUS
GetPlatformPolicy (
  IN  CONST EFI_PCI_PLATFORM_PROTOCOL            *This,
  OUT       EFI_PCI_PLATFORM_POLICY             *PciPolicy
  );

EFI_STATUS
GetPciRom (
  IN CONST EFI_PCI_PLATFORM_PROTOCOL            *This,
  IN       EFI_HANDLE                           PciHandle,
  OUT      VOID                               **RomImage,
  OUT      UINTN                              *RomSize
  );

#endif


