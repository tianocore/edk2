/** @file
  Boot-time reservation GUID HOB.

  A UEFI-hosted launcher (ChainloadApp) allocates the payload FV, its
  own HOB list, the payload's initial stack and, on AArch64, a full
  translation-table hierarchy from the outer firmware as
  EfiReservedMemoryType so that they appear as isolated Reserved
  descriptors in the memory-map snapshot and cannot be selected as
  free RAM by the payload's HOB-memory search.  None of them, however,
  needs to survive past the OS's ExitBootServices() call: DxeCore
  loads every driver out of the FV into its own pages, the launcher's
  HOB list is dead once UefiPayloadEntry has rebuilt the HOB list, the
  launcher's stack is dead once HandOffToDxeCore() has switched to its
  own, and firmware translation tables are the same EfiBootServicesData
  in a normal ArmMmuLib-based boot.

  This HOB tells the payload which of the Reserved records in the SBL
  memory-map HOB are the launcher's own boot-time-only allocations.
  UefiPayloadEntry excludes them from FindFreeMemForHobCallback(),
  publishes each as EFI_RESOURCE_SYSTEM_MEMORY, and pins each with an
  EfiBootServicesData memory-allocation HOB so that the payload's DXE
  never allocates over them and the OS reclaims them after
  ExitBootServices().

  A launcher that does not emit this HOB (Slim Bootloader, coreboot)
  gets the pre-existing behaviour unchanged: the payload publishes the
  Reserved records as EFI_RESOURCE_MEMORY_RESERVED and the OS never
  touches them.

  Copyright (c) 2026, Amazon.com, Inc. or its affiliates. All Rights Reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

extern EFI_GUID  gLoaderBootTimeReservationGuid;

#pragma pack(1)
typedef struct {
  EFI_PHYSICAL_ADDRESS    Base;
  UINT64                  Size;
} LOADER_BOOT_TIME_RESERVATION_ENTRY;

typedef struct {
  UINT8                                 Revision;
  UINT8                                 Reserved0[3];
  UINT32                                Count;
  LOADER_BOOT_TIME_RESERVATION_ENTRY    Entry[0];
} LOADER_BOOT_TIME_RESERVATION;
#pragma pack()
