/** @file
*
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
*  Copyright (c) 2014, Linaro Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <PiPei.h>

#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <libfdt.h>

#include <Guid/EarlyPL011BaseAddress.h>
#include <Guid/FdtHob.h>

EFI_STATUS
EFIAPI
PlatformPeim (
  VOID
  )
{
  VOID               *Base;
  VOID               *NewBase;
  UINTN              FdtSize;
  UINTN              FdtPages;
  UINT64             *FdtHobData;
  UINT64             *UartHobData;
  INT32              Node, Prev;
  CONST CHAR8        *Compatible;
  CONST CHAR8        *CompItem;
  CONST CHAR8        *NodeStatus;
  INT32              Len;
  INT32              StatusLen;
  CONST UINT64       *RegProp;
  UINT64             UartBase;


  Base = (VOID*)(UINTN)PcdGet64 (PcdDeviceTreeInitialBaseAddress);
  ASSERT (Base != NULL);
  ASSERT (fdt_check_header (Base) == 0);

  FdtSize = fdt_totalsize (Base) + PcdGet32 (PcdDeviceTreeAllocationPadding);
  FdtPages = EFI_SIZE_TO_PAGES (FdtSize);
  NewBase = AllocatePages (FdtPages);
  ASSERT (NewBase != NULL);
  fdt_open_into (Base, NewBase, EFI_PAGES_TO_SIZE (FdtPages));

  FdtHobData = BuildGuidHob (&gFdtHobGuid, sizeof *FdtHobData);
  ASSERT (FdtHobData != NULL);
  *FdtHobData = (UINTN)NewBase;

  UartHobData = BuildGuidHob (&gEarlyPL011BaseAddressGuid, sizeof *UartHobData);
  ASSERT (UartHobData != NULL);
  *UartHobData = 0;

  //
  // Look for a UART node
  //
  for (Prev = 0;; Prev = Node) {
    Node = fdt_next_node (Base, Prev, NULL);
    if (Node < 0) {
      break;
    }

    //
    // Check for UART node
    //
    Compatible = fdt_getprop (Base, Node, "compatible", &Len);

    //
    // Iterate over the NULL-separated items in the compatible string
    //
    for (CompItem = Compatible; CompItem != NULL && CompItem < Compatible + Len;
      CompItem += 1 + AsciiStrLen (CompItem)) {

      if (AsciiStrCmp (CompItem, "arm,pl011") == 0) {
        NodeStatus = fdt_getprop (Base, Node, "status", &StatusLen);
        if (NodeStatus != NULL && AsciiStrCmp (NodeStatus, "okay") != 0) {
          continue;
        }

        RegProp = fdt_getprop (Base, Node, "reg", &Len);
        ASSERT (Len == 16);

        UartBase = fdt64_to_cpu (ReadUnaligned64 (RegProp));

        DEBUG ((EFI_D_INFO, "%a: PL011 UART @ 0x%lx\n", __FUNCTION__, UartBase));

        *UartHobData = UartBase;
        break;
      }
    }
  }

  BuildFvHob (PcdGet64 (PcdFvBaseAddress), PcdGet32 (PcdFvSize));

  return EFI_SUCCESS;
}
