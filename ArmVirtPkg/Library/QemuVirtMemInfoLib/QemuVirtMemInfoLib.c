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

// Number of Virtual Memory Map Descriptors
#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS  7

//
// mach-virt's core peripherals such as the UART, the GIC and the RTC are
// all mapped in the 'miscellaneous device I/O' region, which we just map
// in its entirety rather than device by device. Note that it does not
// cover any of the NOR flash banks or PCI resource windows.
//
#define MACH_VIRT_PERIPH_BASE  0x08000000
#define MACH_VIRT_PERIPH_SIZE  SIZE_128MB
//
// The remaining is mapped lazily, but we need to register the memory
// attributes now if we're a Realm.
#define MACH_VIRT_LOWIO_SIZE  (SIZE_1GB - MACH_VIRT_PERIPH_BASE)

#define MACH_VIRT_PCIE_MMIO_SIZE  SIZE_512GB

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
  Obtain the `reg` values of the node that matches the given compatible string.

  @param[in]    Compatible  The compatible string to look for
  @param[out]   Start       Returned address of the region
  @param[out]   Size        Returned size of the region
  @param[in]    Index       Index of the (Address, Size) pair in the reg
                            property

  @return RETURN_SUCCESS, or an error.
**/
STATIC EFI_STATUS
GetFdtNodeAddress (
  CONST CHAR8  *Compatible,
  UINT64       *Start,
  UINT64       *Size,
  UINT8        Index
  )
{
  CONST VOID  *DeviceTree;
  INT32       Node;

  DeviceTree = (VOID *)(UINTN)PcdGet64 (PcdDeviceTreeInitialBaseAddress);
  if (DeviceTree == NULL) {
    return RETURN_UNSUPPORTED;
  }

  if (FdtCheckHeader (DeviceTree) != 0) {
    return RETURN_INVALID_PARAMETER;
  }

  for (Node = FdtNextNode (DeviceTree, 0, NULL);
       Node > 0;
       Node = FdtNextNode (DeviceTree, Node, NULL))
  {
    CONST CHAR8  *CompatProp;
    CONST CHAR8  *CompatItem;
    INT32        PropSize;

    CompatProp = FdtGetProp (DeviceTree, Node, "compatible", &PropSize);
    if (CompatProp == NULL) {
      continue;
    }

    CompatItem = CompatProp;
    while ((CompatItem < CompatProp + PropSize) &&
           (AsciiStrCmp (CompatItem, Compatible) != 0))
    {
      CompatItem += AsciiStrLen (CompatItem) + 1;
    }

    if (CompatItem < CompatProp + PropSize) {
      CONST VOID  *RegProp;

      RegProp = FdtGetProp (DeviceTree, Node, "reg", &PropSize);
      if (RegProp == NULL) {
        return RETURN_NOT_FOUND;
      }

      // For now assume 2 address-cells and 2 size-cells
      if (PropSize < 16 * (Index + 1)) {
        return RETURN_BAD_BUFFER_SIZE;
      }

      *Start = Fdt64ToCpu (ReadUnaligned64 (RegProp + 16 * Index));
      *Size  = Fdt64ToCpu (ReadUnaligned64 (RegProp + 16 * Index + 8));
      return RETURN_SUCCESS;
    }
  }

  return RETURN_NOT_FOUND;
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
  UINT64                        RegionStart;
  UINT64                        RegionSize;

  ASSERT (VirtualMemoryMap != NULL);

  IpaWidth    = 0;
  DevMapBit   = 0;
  Idx         = 0;
  RegionStart = 0;
  RegionSize  = 0;

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

  // System DRAM
  VirtualMemoryTable[Idx].PhysicalBase = PcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[Idx].VirtualBase  = VirtualMemoryTable[0].PhysicalBase;
  VirtualMemoryTable[Idx].Length       = *(UINT64 *)GET_GUID_HOB_DATA (MemorySizeHob);
  VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
  Idx++;

  DEBUG ((
    DEBUG_INFO,
    "%a: Dumping System DRAM Memory Map:\n"
    "\tPhysicalBase: 0x%lX\n"
    "\tVirtualBase: 0x%lX\n"
    "\tLength: 0x%lX\n",
    __func__,
    VirtualMemoryTable[Idx].PhysicalBase,
    VirtualMemoryTable[Idx].VirtualBase,
    VirtualMemoryTable[Idx].Length
    ));

  if (IsRealm ()) {
    Status = GetIpaWidth (&IpaWidth);
    if (Status == RETURN_SUCCESS) {
      DevMapBit = 1ULL << (IpaWidth - 1);
    } else {
      DEBUG ((DEBUG_ERROR, "could not get Realm IPA width\n"));
    }
  }

  // Memory mapped peripherals (UART, RTC, GIC, virtio-mmio, etc)
  VirtualMemoryTable[Idx].PhysicalBase = MACH_VIRT_PERIPH_BASE | DevMapBit;
  VirtualMemoryTable[Idx].VirtualBase  = MACH_VIRT_PERIPH_BASE;
  VirtualMemoryTable[Idx].Length       = MACH_VIRT_LOWIO_SIZE;
  VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  Idx++;

  // Map the FV region as normal executable memory
  VirtualMemoryTable[Idx].PhysicalBase = PcdGet64 (PcdFvBaseAddress);
  VirtualMemoryTable[Idx].VirtualBase  = VirtualMemoryTable[2].PhysicalBase;
  VirtualMemoryTable[Idx].Length       = FixedPcdGet32 (PcdFvSize);
  VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK_RO;
  Idx++;

  /*
   * When the platform has a lot of vCPUs, a second redistributor region is
   * allocated in the high MMIO region, above RAM.
   */
  Status = GetFdtNodeAddress ("arm,gic-v3", &RegionStart, &RegionSize, 2);
  if (Status == RETURN_SUCCESS) {
    VirtualMemoryTable[Idx].PhysicalBase = RegionStart | DevMapBit;
    VirtualMemoryTable[Idx].VirtualBase  = RegionStart;
    VirtualMemoryTable[Idx].Length       = RegionSize;
    VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
    Idx++;
  }

  /*
   * The PCIe config space is mapped above RAM. Its location depends on RAM
   * size (including room for hotpluggable memory), with some alignment
   * constraints. Find the base addresses in the FDT.
   */
  Status = GetFdtNodeAddress ("pci-host-ecam-generic", &RegionStart, &RegionSize, 0);
  if (Status == RETURN_SUCCESS) {
    // High PCIe ECAM region
    VirtualMemoryTable[Idx].PhysicalBase = RegionStart | DevMapBit;
    VirtualMemoryTable[Idx].VirtualBase  = RegionStart;
    VirtualMemoryTable[Idx].Length       = RegionSize;
    VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
    Idx++;

    /* The MMIO region follows, aligned on its own size */
    RegionStart = ALIGN_VALUE (RegionStart + RegionSize, MACH_VIRT_PCIE_MMIO_SIZE);
    RegionSize  = MACH_VIRT_PCIE_MMIO_SIZE;

    // High PCIe MMIO region
    VirtualMemoryTable[Idx].PhysicalBase = RegionStart | DevMapBit;
    VirtualMemoryTable[Idx].VirtualBase  = RegionStart;
    VirtualMemoryTable[Idx].Length       = RegionSize;
    VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
    Idx++;
  } else {
    DEBUG ((DEBUG_WARN, "PCIe Node not found -- unable to map the PCIe MMIO regions\n"));
  }

  // End of Table
  ZeroMem (&VirtualMemoryTable[Idx], sizeof (ARM_MEMORY_REGION_DESCRIPTOR));

  *VirtualMemoryMap = VirtualMemoryTable;
}
