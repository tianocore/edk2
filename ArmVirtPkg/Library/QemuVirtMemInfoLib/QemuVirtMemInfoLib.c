/** @file

  Copyright (c) 2014-2017, Linaro Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Pi/PiMultiPhase.h>
#include <Library/ArmCcaLib.h>
#include <Library/ArmLib.h>
#include <Library/ArmMmuLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FdtLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PlatformDeviceInfoLib.h>

// Number of Virtual Memory Map Descriptors
#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS  (MAX_PLAT_DEVICE_COUNT + 6)

//
// The Platform device info lib only parses architecturally supported
// devices. Therefore, the virtio-mmio, fw-cfg and platform-bus devices
// need to be added separately. So define their base addresses and range.
//
#define MACH_VIRT_VIRTIO_MMIO_BASE  0x0A000000
#define MACH_VIRT_VIRTIO_MMIO_SIZE  0x00004000

#define MACH_VIRT_FWCFG_BASE  0x09020000
#define MACH_VIRT_FWCFG_SIZE  SIZE_4KB

#define MACH_VIRT_PLAT_BUS_BASE  0x0C000000
#define MACH_VIRT_PLAT_BUS_SIZE  0x02000000

/** A macro to trace the memory map.
**/
#define LOG_MEM_MAP(Txt)                                            \
          DEBUG ((                                              \
            DEBUG_ERROR,                                        \
            "%02d\t%-*a\t0x%08lx\t0x%08lx\t0x%08lx\t0x%08lx\n", \
            Idx,                                                \
            16,                                                 \
            Txt,                                                \
            VirtualMemoryTable[Idx].PhysicalBase,               \
            VirtualMemoryTable[Idx].VirtualBase,                \
            VirtualMemoryTable[Idx].Length,                     \
            VirtualMemoryTable[Idx].Attributes                  \
            ));

/**
  Default library constructor that obtains the memory size from a PCD.

  @return  Always returns RETURN_SUCCESS

**/
RETURN_STATUS
EFIAPI
QemuVirtMemInfoLibConstructor (
  VOID
  )
{
  UINT64  Size;
  VOID    *Hob;

  Size = PcdGet64 (PcdSystemMemorySize);
  Hob  = BuildGuidDataHob (&gArmVirtSystemMemorySizeGuid, &Size, sizeof Size);
  ASSERT (Hob != NULL);

  return RETURN_SUCCESS;
}

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
  VOID                          *MemorySizeHob;
  EFI_STATUS                    Status;
  UINT64                        IpaWidth;
  UINT64                        DevMapBit;
  UINTN                         Idx;
  VOID                          *DtbBase;
  UINTN                         Devices;
  UINT64                        BaseAddress;
  UINT64                        Length;
  BOOLEAN                       IsProtectedMmio;
  BOOLEAN                       IsRealmContext;
  PLATFORM_DEVICE_INFO          PlatInfo;

  ASSERT (VirtualMemoryMap != NULL);

  IpaWidth  = 0;
  DevMapBit = 0;
  Idx       = 0;

  MemorySizeHob = GetFirstGuidHob (&gArmVirtSystemMemorySizeGuid);
  ASSERT (MemorySizeHob != NULL);
  if (MemorySizeHob == NULL) {
    return;
  }

  VirtualMemoryTable = AllocatePool (
                         sizeof (ARM_MEMORY_REGION_DESCRIPTOR) *
                         MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS
                         );

  if (VirtualMemoryTable == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Error: Failed AllocatePool()\n", __func__));
    return;
  }

  DEBUG ((
    DEBUG_ERROR,
    "Idx\tRegion          \tPhysical Base\tVirtual Base\t"
    "Length          Attributes\n"
    ));

  // System DRAM
  VirtualMemoryTable[Idx].PhysicalBase = PcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[Idx].VirtualBase  = VirtualMemoryTable[0].PhysicalBase;
  VirtualMemoryTable[Idx].Length       = *(UINT64 *)GET_GUID_HOB_DATA (MemorySizeHob);
  VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
  LOG_MEM_MAP ("System DRAM");
  Idx++;

  IsRealmContext = IsRealm ();
  if (IsRealmContext) {
    Status = GetIpaWidth (&IpaWidth);
    if (Status == RETURN_SUCCESS) {
      DevMapBit = 1ULL << (IpaWidth - 1);
    } else {
      DEBUG ((DEBUG_ERROR, "could not get Realm IPA width\n"));
    }
  }

  // Map the FV region as normal executable memory
  VirtualMemoryTable[Idx].PhysicalBase = PcdGet64 (PcdFvBaseAddress);
  VirtualMemoryTable[Idx].VirtualBase  = VirtualMemoryTable[Idx].PhysicalBase;
  VirtualMemoryTable[Idx].Length       = FixedPcdGet32 (PcdFvSize);
  VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK_RO;
  LOG_MEM_MAP ("FV Region");
  Idx++;

  // virtio-mmio Region
  VirtualMemoryTable[Idx].PhysicalBase = MACH_VIRT_VIRTIO_MMIO_BASE | DevMapBit;
  VirtualMemoryTable[Idx].VirtualBase  = MACH_VIRT_VIRTIO_MMIO_BASE;
  VirtualMemoryTable[Idx].Length       = MACH_VIRT_VIRTIO_MMIO_SIZE;
  VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  LOG_MEM_MAP ("virtio-mmio Region");
  Idx++;

  // FW-CFG region
  VirtualMemoryTable[Idx].PhysicalBase = MACH_VIRT_FWCFG_BASE | DevMapBit;
  VirtualMemoryTable[Idx].VirtualBase  = MACH_VIRT_FWCFG_BASE;
  VirtualMemoryTable[Idx].Length       = MACH_VIRT_FWCFG_SIZE;
  VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  LOG_MEM_MAP ("fw-cfg Region");
  Idx++;

  // platform-bus region
  VirtualMemoryTable[Idx].PhysicalBase = MACH_VIRT_PLAT_BUS_BASE | DevMapBit;
  VirtualMemoryTable[Idx].VirtualBase  = MACH_VIRT_PLAT_BUS_BASE;
  VirtualMemoryTable[Idx].Length       = MACH_VIRT_PLAT_BUS_SIZE;
  VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  LOG_MEM_MAP ("platform-bus Region");
  Idx++;

  // Parse the FDT and populate the PLATFORM_DEVICE_INFO structure with
  // the MMIO base address and range for the devices present in the FDT.
  DtbBase = (VOID *)(UINTN)PcdGet64 (PcdDeviceTreeInitialBaseAddress);

  ZeroMem (&PlatInfo, sizeof (PLATFORM_DEVICE_INFO));
  Status = ParsePlatformDeviceFdt (DtbBase, &PlatInfo);
  if (EFI_ERROR (Status)) {
    return;
  }

  if (PlatInfo.MaxDevices >= (MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS - Idx)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Error: Max Devices reached. Available Slots %d, Required %d\n",
      __func__,
      (MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS - Idx),
      PlatInfo.MaxDevices
      ));
    ASSERT (0);
    return;
  }

  // Map peripheral devices
  for (Devices = 0; Devices < PlatInfo.MaxDevices; Devices++) {
    BaseAddress = PlatInfo.Dev[Devices].BaseAddress;

    // Some devices may only use few registers and would report the length
    // accordingly. Although this is correct, the device is expected to reserve
    // at least a page for MMIO page mapping. Therefore, align the address
    // range to the nearest page size.
    Length = ALIGN_VALUE (PlatInfo.Dev[Devices].Length, EFI_PAGE_SIZE);

    if (!IsRealmContext) {
      VirtualMemoryTable[Idx].PhysicalBase = BaseAddress;
    } else {
      Status = ArmCcaMemRangeIsProtectedMmio (
                 BaseAddress,
                 Length,
                 &IsProtectedMmio
                 );
      if (RETURN_ERROR (Status)) {
        return;
      }

      VirtualMemoryTable[Idx].PhysicalBase = IsProtectedMmio ? BaseAddress : (BaseAddress | DevMapBit);
    }

    VirtualMemoryTable[Idx].VirtualBase = BaseAddress;
    VirtualMemoryTable[Idx].Length      = Length;
    VirtualMemoryTable[Idx].Attributes  = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
    LOG_MEM_MAP (PlatInfo.Dev[Devices].Desc);
    Idx++;
  } // Map Peripheral devices

  // End of Table
  ZeroMem (&VirtualMemoryTable[Idx], sizeof (ARM_MEMORY_REGION_DESCRIPTOR));

  *VirtualMemoryMap = VirtualMemoryTable;
}
