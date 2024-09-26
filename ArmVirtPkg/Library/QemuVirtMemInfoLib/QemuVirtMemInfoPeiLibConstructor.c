/** @file

  Copyright (c) 2014-2017, Linaro Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Pi/PiMultiPhase.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <libfdt.h>

RETURN_STATUS
EFIAPI
QemuVirtMemInfoPeiLibConstructor (
  VOID
  )
{
  VOID          *DeviceTreeBase;
  INT32         Node, Prev;
  UINT64        NewBase, CurBase;
  UINT64        NewSize, CurSize;
  CONST CHAR8   *Type;
  INT32         Len;
  CONST UINT64  *RegProp;
  VOID          *Hob;

  NewBase = 0;
  NewSize = 0;

  DeviceTreeBase = (VOID *)(UINTN)PcdGet64 (PcdDeviceTreeInitialBaseAddress);
  ASSERT (DeviceTreeBase != NULL);

  //
  // Make sure we have a valid device tree blob
  //
  ASSERT (fdt_check_header (DeviceTreeBase) == 0);

  //
  // Look for the lowest memory node
  //
  for (Prev = 0; ; Prev = Node) {
    Node = fdt_next_node (DeviceTreeBase, Prev, NULL);
    if (Node < 0) {
      break;
    }

    //
    // Check for memory node
    //
    Type = fdt_getprop (DeviceTreeBase, Node, "device_type", &Len);
    if (Type && (AsciiStrnCmp (Type, "memory", Len) == 0)) {
      //
      // Get the 'reg' property of this node. For now, we will assume
      // two 8 byte quantities for base and size, respectively.
      //
      RegProp = fdt_getprop (DeviceTreeBase, Node, "reg", &Len);
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

        if ((NewBase > CurBase) || (NewBase == 0)) {
          NewBase = CurBase;
          NewSize = CurSize;
        }
      } else {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to parse FDT memory node\n",
          __func__
          ));
      }
    }
  }

  //
  // Make sure the start of DRAM matches our expectation
  //
  ASSERT (FixedPcdGet64 (PcdSystemMemoryBase) == NewBase);

  Hob = BuildGuidDataHob (
          &gArmVirtSystemMemorySizeGuid,
          &NewSize,
          sizeof NewSize
          );
  ASSERT (Hob != NULL);

  //
  // We need to make sure that the machine we are running on has at least
  // 128 MB of memory configured, and is currently executing this binary from
  // NOR flash. This prevents a device tree image in DRAM from getting
  // clobbered when our caller installs permanent PEI RAM, before we have a
  // chance of marking its location as reserved or copy it to a freshly
  // allocated block in the permanent PEI RAM in the platform PEIM.
  //
  ASSERT (NewSize >= SIZE_128MB);
  ASSERT (
    (((UINT64)PcdGet64 (PcdFdBaseAddress) +
      (UINT64)PcdGet32 (PcdFdSize)) <= NewBase) ||
    ((UINT64)PcdGet64 (PcdFdBaseAddress) >= (NewBase + NewSize))
    );

  return RETURN_SUCCESS;
}
