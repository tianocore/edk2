/** @file

  Copyright (c) 2014-2017, Linaro Limited. All rights reserved.
  Copyright (c) 2026, Arm Limited. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Pi/PiMultiPhase.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/FdtSerialPortAddressLib.h>
#include <Library/FdtLib.h>

// Number of Virtual Memory Map Descriptors
#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS  (5)

/** A macro to trace the memory map.
**/
#define LOG_MEM_MAP(Txt)                                        \
          DEBUG ((                                              \
            DEBUG_INFO,                                         \
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
  Get the serial port base address to be used for the serial console output.

  @param[in]  DeviceTreeBase        Base address of the Device Tree.
  @param[out] UartBase              Base address of the Uart.

  @retval RETURN_INVALID_PARAMETER  Device Tree Base address is invalid.
  @retval RETURN_NOT_FOUND          No enabled console port has been found.
  @retval RETURN_SUCCESS            BaseAddress has been populated.
**/
STATIC
RETURN_STATUS
EFIAPI
GetUartBase (
  IN  VOID    *DeviceTreeBase,
  OUT UINT64  *UartBase
  )
{
  RETURN_STATUS     RetStatus;
  FDT_SERIAL_PORTS  Ports;

  if (DeviceTreeBase == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  RetStatus = FdtSerialGetPorts (DeviceTreeBase, "arm,pl011", &Ports);
  if (RETURN_ERROR (RetStatus)) {
    return RetStatus;
  }

  //
  // Default to the first port found, but (if there are multiple ports) allow
  // the "/chosen" node to override it. Note that if FdtSerialGetConsolePort()
  // fails, it does not modify UartBase.
  //
  *UartBase = Ports.BaseAddress[0];
  if (Ports.NumberOfPorts > 1) {
    FdtSerialGetConsolePort (DeviceTreeBase, UartBase);
  }

  return RETURN_SUCCESS;
}

/**
  Get the QEMU fw_cfg MMIO region from the device tree.

  @param[in]  DeviceTreeBase        Base address of the Device Tree.
  @param[out] FwCfgBase             Base address of the fw_cfg MMIO region.
  @param[out] FwCfgSize             Size of the fw_cfg MMIO region.

  @retval RETURN_INVALID_PARAMETER  Device Tree Base address is invalid.
  @retval RETURN_NOT_FOUND          No fw_cfg MMIO node has been found.
  @retval RETURN_SUCCESS            FwCfgBase and FwCfgSize have been populated.
**/
STATIC
RETURN_STATUS
EFIAPI
GetFwCfgMmioRegion (
  IN  VOID    *DeviceTreeBase,
  OUT UINT64  *FwCfgBase,
  OUT UINT64  *FwCfgSize
  )
{
  RETURN_STATUS  Status;
  INT32          Len;
  INT32          Node;
  INT32          Prev;
  CONST CHAR8    *Type;
  CONST UINT64   *Reg;

  if (DeviceTreeBase == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  Status = RETURN_NOT_FOUND;
  for (Prev = 0; ; Prev = Node) {
    Node = FdtNextNode (DeviceTreeBase, Prev, NULL);
    if (Node < 0) {
      break;
    }

    //
    // Check for memory node
    //
    Type = FdtGetProp (DeviceTreeBase, Node, "compatible", &Len);
    if ((Type != NULL) &&
        (AsciiStrnCmp (Type, "qemu,fw-cfg-mmio", Len) == 0))
    {
      //
      // Get the 'reg' property of this node. For now, we will assume
      // two 8 byte quantities for base and size, respectively.
      //
      Reg = FdtGetProp (DeviceTreeBase, Node, "reg", &Len);
      if ((Reg != 0) && (Len == (2 * sizeof (UINT64)))) {
        *FwCfgBase = SwapBytes64 (ReadUnaligned64 ((VOID *)&Reg[0]));
        *FwCfgSize = SwapBytes64 (ReadUnaligned64 ((VOID *)&Reg[1]));
        Status     = RETURN_SUCCESS;
        break;
      } else {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to parse FDT QemuCfg node\n",
          __func__
          ));
        break;
      }
    }
  } // for

  return Status;
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
  RETURN_STATUS                 RetStatus;
  ARM_MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;
  VOID                          *MemorySizeHob;
  UINTN                         Idx;
  VOID                          *DeviceTreeBase;
  UINT64                        MappingBase;
  UINT64                        MappingSize;
  UINT64                        UartBase;
  UINT64                        FwCfgBase;
  UINT64                        FwCfgSize;

  ASSERT (VirtualMemoryMap != NULL);

  Idx = 0;

  MemorySizeHob = GetFirstGuidHob (&gArmVirtSystemMemorySizeGuid);
  ASSERT (MemorySizeHob != NULL);
  if (MemorySizeHob == NULL) {
    return;
  }

  DeviceTreeBase = (VOID *)(UINTN)PcdGet64 (PcdDeviceTreeInitialBaseAddress);
  if (DeviceTreeBase == NULL) {
    ASSERT (0);
    return;
  }

  RetStatus = GetUartBase (DeviceTreeBase, &UartBase);
  if (RETURN_ERROR (RetStatus) || (UartBase == 0)) {
    ASSERT_RETURN_ERROR (RetStatus);
    return;
  }

  RetStatus = GetFwCfgMmioRegion (DeviceTreeBase, &FwCfgBase, &FwCfgSize);
  if (RETURN_ERROR (RetStatus) ||
      (FwCfgBase == 0) ||
      (FwCfgSize == 0))
  {
    ASSERT_RETURN_ERROR (RetStatus);
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
    DEBUG_INFO,
    "Idx\tRegion          \tPhysical Base\tVirtual Base\t"
    "Length          Attributes\n"
    ));

  // System DRAM
  VirtualMemoryTable[Idx].PhysicalBase = PcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[Idx].VirtualBase  = VirtualMemoryTable[Idx].PhysicalBase;
  VirtualMemoryTable[Idx].Length       = *(UINT64 *)GET_GUID_HOB_DATA (MemorySizeHob);
  VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
  LOG_MEM_MAP ("System DRAM");
  Idx++;

  // Map the FV region as normal executable memory
  VirtualMemoryTable[Idx].PhysicalBase = PcdGet64 (PcdFvBaseAddress);
  VirtualMemoryTable[Idx].VirtualBase  = VirtualMemoryTable[Idx].PhysicalBase;
  VirtualMemoryTable[Idx].Length       = FixedPcdGet32 (PcdFvSize);
  VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK_RO;
  LOG_MEM_MAP ("FV Region");
  Idx++;

  // Map the UART
  MappingBase = UartBase & ~(UINT64)EFI_PAGE_MASK;
  MappingSize = EFI_PAGES_TO_SIZE (
                  EFI_SIZE_TO_PAGES (UartBase - MappingBase + EFI_PAGE_SIZE)
                  );
  VirtualMemoryTable[Idx].PhysicalBase = MappingBase;
  VirtualMemoryTable[Idx].VirtualBase  = MappingBase;
  VirtualMemoryTable[Idx].Length       = MappingSize;
  VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  LOG_MEM_MAP ("Serial Port");
  Idx++;

  // Map the fw_cfg MMIO region
  MappingBase = FwCfgBase & ~(UINT64)EFI_PAGE_MASK;
  MappingSize = EFI_PAGES_TO_SIZE (
                  EFI_SIZE_TO_PAGES (FwCfgBase - MappingBase + FwCfgSize)
                  );
  VirtualMemoryTable[Idx].PhysicalBase = MappingBase;
  VirtualMemoryTable[Idx].VirtualBase  = MappingBase;
  VirtualMemoryTable[Idx].Length       = MappingSize;
  VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  LOG_MEM_MAP ("FwCfg");
  Idx++;

  // End of Table
  ZeroMem (&VirtualMemoryTable[Idx], sizeof (ARM_MEMORY_REGION_DESCRIPTOR));

  ASSERT ((Idx + 1) <= MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS);

  *VirtualMemoryMap = VirtualMemoryTable;
}
