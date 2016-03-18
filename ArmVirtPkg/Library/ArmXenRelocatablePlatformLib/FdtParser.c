/*
 * Copyright (c) 2015, Linaro Ltd. All rights reserved.
 *
 * This program and the accompanying materials
 * are licensed and made available under the terms and conditions of the BSD License
 * which accompanies this distribution.  The full text of the license may be found at
 * http://opensource.org/licenses/bsd-license.php
 *
 * THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 * WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 */

#include <Uefi.h>
#include <Include/libfdt.h>

BOOLEAN
FindMemnode (
  IN  VOID    *DeviceTreeBlob,
  OUT UINT64  *SystemMemoryBase,
  OUT UINT64  *SystemMemorySize
  )
{
  INT32         MemoryNode;
  INT32         AddressCells;
  INT32         SizeCells;
  INT32         Length;
  CONST INT32   *Prop;

  if (fdt_check_header (DeviceTreeBlob) != 0) {
    return FALSE;
  }

  //
  // Look for a node called "memory" at the lowest level of the tree
  //
  MemoryNode = fdt_path_offset (DeviceTreeBlob, "/memory");
  if (MemoryNode <= 0) {
    return FALSE;
  }

  //
  // Retrieve the #address-cells and #size-cells properties
  // from the root node, or use the default if not provided.
  //
  AddressCells = 1;
  SizeCells = 1;

  Prop = fdt_getprop (DeviceTreeBlob, 0, "#address-cells", &Length);
  if (Length == 4) {
    AddressCells = fdt32_to_cpu (*Prop);
  }

  Prop = fdt_getprop (DeviceTreeBlob, 0, "#size-cells", &Length);
  if (Length == 4) {
    SizeCells = fdt32_to_cpu (*Prop);
  }

  //
  // Now find the 'reg' property of the /memory node, and read the first
  // range listed.
  //
  Prop = fdt_getprop (DeviceTreeBlob, MemoryNode, "reg", &Length);

  if (Length < (AddressCells + SizeCells) * sizeof (INT32)) {
    return FALSE;
  }

  if (AddressCells == 1) {
    *SystemMemoryBase = fdt32_to_cpu (*Prop);
  } else {
    *SystemMemoryBase = fdt64_to_cpu (*(UINT64 *)Prop);
  }
  Prop += AddressCells;

  if (SizeCells == 1) {
    *SystemMemorySize = fdt32_to_cpu (*Prop);
  } else {
    *SystemMemorySize = fdt64_to_cpu (*(UINT64 *)Prop);
  }

  return TRUE;
}

VOID
CopyFdt (
  IN    VOID    *FdtDest,
  IN    VOID    *FdtSource
  )
{
  CopyMem (FdtDest, FdtSource, fdt_totalsize (FdtSource));
}
