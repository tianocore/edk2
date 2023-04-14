/** @file
  Memory Detection for Virtual Machines.

  Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  MemDetect.c

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

//
// The Library classes this module consumes
//
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/ResourcePublicationLib.h>
#include <Library/BaseRiscVSbiLib.h>
#include <Register/RiscV64/RiscVEncoding.h>
#include <Library/PrePiLib.h>
#include <libfdt.h>
#include <Guid/FdtHob.h>

VOID
BuildMemoryTypeInformationHob (
  VOID
  );

/**
  Create memory range resource HOB using the memory base
  address and size.

  @param  MemoryBase     Memory range base address.
  @param  MemorySize     Memory range size.

**/
STATIC
VOID
AddMemoryBaseSizeHob (
  IN EFI_PHYSICAL_ADDRESS  MemoryBase,
  IN UINT64                MemorySize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    EFI_RESOURCE_ATTRIBUTE_PRESENT |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_TESTED,
    MemoryBase,
    MemorySize
    );
}

/**
  Create memory range resource HOB using memory base
  address and top address of the memory range.

  @param  MemoryBase     Memory range base address.
  @param  MemoryLimit    Memory range size.

**/
STATIC
VOID
AddMemoryRangeHob (
  IN EFI_PHYSICAL_ADDRESS  MemoryBase,
  IN EFI_PHYSICAL_ADDRESS  MemoryLimit
  )
{
  AddMemoryBaseSizeHob (MemoryBase, (UINT64)(MemoryLimit - MemoryBase));
}

/**
  Configure MMU
**/
STATIC
VOID
InitMmu (
  )
{
  //
  // Set supervisor translation mode to Bare mode
  //
  RiscVSetSupervisorAddressTranslationRegister ((UINT64)SATP_MODE_OFF << 60);
  DEBUG ((DEBUG_INFO, "%a: Set Supervisor address mode to bare-metal mode.\n", __func__));
}

/**
  Publish system RAM and reserve memory regions.

**/
STATIC
VOID
InitializeRamRegions (
  IN EFI_PHYSICAL_ADDRESS  SystemMemoryBase,
  IN UINT64                SystemMemorySize
  )
{
  AddMemoryRangeHob (
    SystemMemoryBase,
    SystemMemoryBase + SystemMemorySize
    );
}

/** Get the number of cells for a given property

  @param[in]  Fdt   Pointer to Device Tree (DTB)
  @param[in]  Node  Node
  @param[in]  Name  Name of the property

  @return           Number of cells.
**/
STATIC
INT32
GetNumCells (
  IN VOID         *Fdt,
  IN INT32        Node,
  IN CONST CHAR8  *Name
  )
{
  CONST INT32  *Prop;
  INT32        Len;
  UINT32       Val;

  Prop = fdt_getprop (Fdt, Node, Name, &Len);
  if (Prop == NULL) {
    return Len;
  }

  if (Len != sizeof (*Prop)) {
    return -FDT_ERR_BADNCELLS;
  }

  Val = fdt32_to_cpu (*Prop);
  if (Val > FDT_MAX_NCELLS) {
    return -FDT_ERR_BADNCELLS;
  }

  return (INT32)Val;
}

/** Mark reserved memory ranges in the EFI memory map

  The M-mode firmware ranges should not be used by the
  EDK2/OS. These ranges are passed via device tree using reserved
  memory nodes. Parse the DT and mark those ranges as of
  type EfiReservedMemoryType.

  NOTE: Device Tree spec section 3.5.4 says reserved memory regions
  without no-map property should be installed as EfiBootServicesData.
  As per UEFI spec, memory of type EfiBootServicesData can be used
  by the OS after ExitBootServices().
  This is not an issue for DT since OS can parse the DT also along
  with EFI memory map and avoid using these ranges. But with ACPI,
  there is no such mechanisms possible.
  Since EDK2 needs to support both DT and ACPI, we are deviating
  from the DT spec and marking all reserved memory ranges as
  EfiReservedMemoryType itself irrespective of no-map.

  @param FdtPointer Pointer to FDT

**/
STATIC
VOID
AddReservedMemoryMap (
  IN VOID  *FdtPointer
  )
{
  CONST INT32           *RegProp;
  INT32                 Node;
  INT32                 SubNode;
  INT32                 Len;
  EFI_PHYSICAL_ADDRESS  Addr;
  UINT64                Size;
  INTN                  NumRsv, i;
  INT32                 NumAddrCells, NumSizeCells;

  NumRsv = fdt_num_mem_rsv (FdtPointer);

  /* Look for an existing entry and add it to the efi mem map. */
  for (i = 0; i < NumRsv; i++) {
    if (fdt_get_mem_rsv (FdtPointer, i, &Addr, &Size) != 0) {
      continue;
    }

    BuildMemoryAllocationHob (
      Addr,
      Size,
      EfiReservedMemoryType
      );
  }

  /* process reserved-memory */
  Node = fdt_subnode_offset (FdtPointer, 0, "reserved-memory");
  if (Node >= 0) {
    NumAddrCells = GetNumCells (FdtPointer, Node, "#address-cells");
    if (NumAddrCells <= 0) {
      return;
    }

    NumSizeCells = GetNumCells (FdtPointer, Node, "#size-cells");
    if (NumSizeCells <= 0) {
      return;
    }

    fdt_for_each_subnode (SubNode, FdtPointer, Node) {
      RegProp = fdt_getprop (FdtPointer, SubNode, "reg", &Len);

      if ((RegProp != 0) && (Len == ((NumAddrCells + NumSizeCells) * sizeof (INT32)))) {
        Addr = fdt32_to_cpu (RegProp[0]);

        if (NumAddrCells > 1) {
          Addr = (Addr << 32) | fdt32_to_cpu (RegProp[1]);
        }

        RegProp += NumAddrCells;
        Size     = fdt32_to_cpu (RegProp[0]);

        if (NumSizeCells > 1) {
          Size = (Size << 32) | fdt32_to_cpu (RegProp[1]);
        }

        DEBUG ((
          DEBUG_INFO,
          "%a: Adding Reserved Memory Addr = 0x%llx, Size = 0x%llx\n",
          __func__,
          Addr,
          Size
          ));

        BuildMemoryAllocationHob (
          Addr,
          Size,
          EfiReservedMemoryType
          );
      }
    }
  }
}

/**
  Initialize memory hob based on the DTB information.

  @return EFI_SUCCESS     The memory hob added successfully.

**/
EFI_STATUS
MemoryPeimInitialization (
  VOID
  )
{
  EFI_RISCV_FIRMWARE_CONTEXT  *FirmwareContext;
  CONST UINT64                *RegProp;
  CONST CHAR8                 *Type;
  UINT64                      CurBase, CurSize;
  INT32                       Node, Prev;
  INT32                       Len;
  VOID                        *FdtPointer;

  FirmwareContext = NULL;
  GetFirmwareContextPointer (&FirmwareContext);

  if (FirmwareContext == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Firmware Context is NULL\n", __func__));
    return EFI_UNSUPPORTED;
  }

  FdtPointer = (VOID *)FirmwareContext->FlattenedDeviceTree;
  if (FdtPointer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid FDT pointer\n", __func__));
    return EFI_UNSUPPORTED;
  }

  // Look for the lowest memory node
  for (Prev = 0; ; Prev = Node) {
    Node = fdt_next_node (FdtPointer, Prev, NULL);
    if (Node < 0) {
      break;
    }

    // Check for memory node
    Type = fdt_getprop (FdtPointer, Node, "device_type", &Len);
    if (Type && (AsciiStrnCmp (Type, "memory", Len) == 0)) {
      // Get the 'reg' property of this node. For now, we will assume
      // two 8 byte quantities for base and size, respectively.
      RegProp = fdt_getprop (FdtPointer, Node, "reg", &Len);
      if ((RegProp != 0) && (Len == (2 * sizeof (UINT64)))) {
        CurBase = fdt64_to_cpu (ReadUnaligned64 (RegProp));
        CurSize = fdt64_to_cpu (ReadUnaligned64 (RegProp + 1));

        DEBUG ((
          DEBUG_INFO,
          "%a: System RAM @ 0x%lx - 0x%lx\n",
          __func__,
          CurBase,
          CurBase + CurSize - 1
          ));

        InitializeRamRegions (
          CurBase,
          CurSize
          );
      } else {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to parse FDT memory node\n",
          __func__
          ));
      }
    }
  }

  AddReservedMemoryMap (FdtPointer);

  InitMmu ();

  BuildMemoryTypeInformationHob ();

  return EFI_SUCCESS;
}
