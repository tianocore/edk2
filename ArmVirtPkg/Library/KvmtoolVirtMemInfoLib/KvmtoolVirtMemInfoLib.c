/** @file
  Kvmtool virtual memory map library.

  Copyright (c) 2018 - 2024, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/ArmCcaLib.h>
#include <Library/ArmLib.h>
#include <Library/ArmMmuLib.h>
#include <Library/ArmPlatformDeviceInfoLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

// Number of Virtual Memory Map Descriptors
#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS  (MAX_PLAT_DEVICE_COUNT + 4)

// A platform device information structure used to
// collate the MMIO base address and range for the
// Platform devices.
STATIC PLATFROM_DEVICE_INFO  mPlatInfo;

/**
  Return the Virtual Memory Map of your platform

  This Virtual Memory Map is used by MemoryInitPei Module to initialize the MMU
  on your platform.

  @param[out]   VirtualMemoryMap    Array of ARM_MEMORY_REGION_DESCRIPTOR
                                    describing a Physical-to-Virtual Memory
                                    mapping. This array must be ended by a
                                    zero-filled entry. The allocated memory
                                    will not be freed.

**/
VOID
ArmVirtGetMemoryMap (
  OUT ARM_MEMORY_REGION_DESCRIPTOR  **VirtualMemoryMap
  )
{
  ARM_MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;
  UINTN                         Idx;
  EFI_PHYSICAL_ADDRESS          TopOfAddressSpace;
  VOID                          *DtbBase;
  UINTN                         Devices;
  EFI_STATUS                    Status;

  ASSERT (VirtualMemoryMap != NULL);

  TopOfAddressSpace = LShiftU64 (1ULL, ArmGetPhysicalAddressBits ());

  // Parse the FDT and populate the PLATFROM_DEVICE_INFO structure with
  // the MMIO base address and range for the devices present in the FDT.
  DtbBase = (VOID *)(UINTN)PcdGet64 (PcdDeviceTreeInitialBaseAddress);

  ZeroMem (&mPlatInfo, sizeof (PLATFROM_DEVICE_INFO));
  Status = ArmParsePlatformDeviceFdt (DtbBase, &mPlatInfo);
  if (EFI_ERROR (Status)) {
    return;
  }

  if (mPlatInfo.MaxDevices > MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS) {
    return;
  }

  VirtualMemoryTable = (ARM_MEMORY_REGION_DESCRIPTOR *)
                       AllocatePages (
                         EFI_SIZE_TO_PAGES (
                           sizeof (ARM_MEMORY_REGION_DESCRIPTOR) *
                           MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS
                           )
                         );
  if (VirtualMemoryTable == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Error: Failed to Allocate Pages\n",
      __func__
      ));
    return;
  }

  Idx = 0;
  // System DRAM
  VirtualMemoryTable[Idx].PhysicalBase = PcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[Idx].VirtualBase  = VirtualMemoryTable[Idx].PhysicalBase;
  VirtualMemoryTable[Idx].Length       = PcdGet64 (PcdSystemMemorySize);
  VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;

  // Map the FV region as normal executable memory
  VirtualMemoryTable[++Idx].PhysicalBase = PcdGet64 (PcdFvBaseAddress);
  VirtualMemoryTable[Idx].VirtualBase    = VirtualMemoryTable[Idx].PhysicalBase;
  VirtualMemoryTable[Idx].Length         = FixedPcdGet32 (PcdFvSize);
  VirtualMemoryTable[Idx].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;

  // Map peripheral devices
  for (Devices = 0; Devices < mPlatInfo.MaxDevices; Devices++) {
    VirtualMemoryTable[++Idx].PhysicalBase = mPlatInfo.Dev[Devices].BaseAddress;
    VirtualMemoryTable[Idx].VirtualBase    = mPlatInfo.Dev[Devices].BaseAddress;
    // Some devices may only use few registers and would report the length
    // accordingly. Although this is correct, the device is expected to reserve
    // at least a page for MMIO page mapping. Therefore, align the address
    // range to the nearest page size.
    VirtualMemoryTable[Idx].Length = ALIGN_VALUE (
                                       mPlatInfo.Dev[Devices].Length,
                                       EFI_PAGE_SIZE
                                       );
    VirtualMemoryTable[Idx].Attributes = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  }

  // End of Table
  VirtualMemoryTable[++Idx].PhysicalBase = 0;
  VirtualMemoryTable[Idx].VirtualBase    = 0;
  VirtualMemoryTable[Idx].Length         = 0;
  VirtualMemoryTable[Idx].Attributes     = (ARM_MEMORY_REGION_ATTRIBUTES)0;

  ASSERT ((Idx + 1) <= MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS);

  *VirtualMemoryMap = VirtualMemoryTable;
}

/**
  Configure the MMIO regions as shared with the VMM.

  Set the protection attribute for the MMIO regions as Unprotected IPA.

  @param[in]    IpaWidth  IPA width of the Realm.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
  @retval RETURN_UNSUPPORTED        The execution context is not in a Realm.
**/
RETURN_STATUS
EFIAPI
ArmCcaConfigureMmio (
  IN UINT64  IpaWidth
  )
{
  RETURN_STATUS  Status;

  if (!IsRealm ()) {
    return RETURN_UNSUPPORTED;
  }

  // Set the protection attribute for the Peripheral memory.
  // Peripheral space before DRAM
  Status = ArmCcaSetMemoryProtectAttribute (
             0,
             PcdGet64 (PcdSystemMemoryBase),
             IpaWidth,
             TRUE
             );
  ASSERT_RETURN_ERROR (Status);
  return Status;
}
