/** @file
*  High memory node enumeration DXE driver for ARM Virtual Machines
*
*  Copyright (c) 2015, Linaro Ltd. All rights reserved.
*
*  This program and the accompanying materials are licensed and made available
*  under the terms and conditions of the BSD License which accompanies this
*  distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR
*  IMPLIED.
*
**/

#include <Library/BaseLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <libfdt.h>
#include <Library/DxeServicesTableLib.h>

EFI_STATUS
EFIAPI
InitializeHighMemDxe (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  VOID             *Hob;
  VOID             *DeviceTreeBase;
  INT32            Node, Prev;
  EFI_STATUS       Status;
  CONST CHAR8      *Type;
  INT32            Len;
  CONST VOID       *RegProp;
  UINT64           CurBase;
  UINT64           CurSize;

  Hob = GetFirstGuidHob(&gFdtHobGuid);
  if (Hob == NULL || GET_GUID_HOB_DATA_SIZE (Hob) != sizeof (UINT64)) {
    return EFI_NOT_FOUND;
  }
  DeviceTreeBase = (VOID *)(UINTN)*(UINT64 *)GET_GUID_HOB_DATA (Hob);

  if (fdt_check_header (DeviceTreeBase) != 0) {
    DEBUG ((EFI_D_ERROR, "%a: No DTB found @ 0x%p\n", __FUNCTION__,
      DeviceTreeBase));
    return EFI_NOT_FOUND;
  }

  DEBUG ((EFI_D_INFO, "%a: DTB @ 0x%p\n", __FUNCTION__, DeviceTreeBase));

  //
  // Check for memory node and add the memory spaces expect the lowest one
  //
  for (Prev = 0;; Prev = Node) {
    Node = fdt_next_node (DeviceTreeBase, Prev, NULL);
    if (Node < 0) {
      break;
    }

    Type = fdt_getprop (DeviceTreeBase, Node, "device_type", &Len);
    if (Type && AsciiStrnCmp (Type, "memory", Len) == 0) {
      //
      // Get the 'reg' property of this node. For now, we will assume
      // two 8 byte quantities for base and size, respectively.
      //
      RegProp = fdt_getprop (DeviceTreeBase, Node, "reg", &Len);
      if (RegProp != NULL && Len == (2 * sizeof (UINT64))) {

        CurBase = fdt64_to_cpu (((UINT64 *)RegProp)[0]);
        CurSize = fdt64_to_cpu (((UINT64 *)RegProp)[1]);

        if (FixedPcdGet64 (PcdSystemMemoryBase) != CurBase) {
          Status = gDS->AddMemorySpace (
                          EfiGcdMemoryTypeSystemMemory,
                          CurBase, CurSize,
                          EFI_MEMORY_WB | EFI_MEMORY_WC |
                          EFI_MEMORY_WT | EFI_MEMORY_UC);

          if (EFI_ERROR (Status)) {
            DEBUG ((EFI_D_ERROR,
              "%a: Failed to add System RAM @ 0x%lx - 0x%lx (%r)\n",
              __FUNCTION__, CurBase, CurBase + CurSize - 1, Status));
            continue;
          }

          Status = gDS->SetMemorySpaceAttributes (
                          CurBase, CurSize,
                          EFI_MEMORY_WB);

          if (EFI_ERROR (Status)) {
            DEBUG ((EFI_D_ERROR,
              "%a: Failed to set System RAM @ 0x%lx - 0x%lx attribute (%r)\n",
              __FUNCTION__, CurBase, CurBase + CurSize - 1, Status));
          } else {
            DEBUG ((EFI_D_INFO, "%a: Add System RAM @ 0x%lx - 0x%lx\n",
              __FUNCTION__, CurBase, CurBase + CurSize - 1));
          }
        }
      }
    }
  }

  return EFI_SUCCESS;
}
