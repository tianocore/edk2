/** @file
  NULL library class implementation to discover the virtio-mmio regions and map them.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MapMmioLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/FdtClient.h>

/** Entrypoint for VirtioMmioProbeLib.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           Flash device not found.
**/
EFI_STATUS
EFIAPI
VirtioMmioProbe (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS           Status;
  EFI_STATUS           FindNodeStatus;
  FDT_CLIENT_PROTOCOL  *FdtClient;
  INT32                Node;
  CONST UINT64         *Reg;
  UINT32               RegSize;
  UINT64               RegBase;
  UINT64               Range;

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  for (FindNodeStatus = FdtClient->FindCompatibleNode (
                                     FdtClient,
                                     "virtio,mmio",
                                     &Node
                                     );
       !EFI_ERROR (FindNodeStatus);
       FindNodeStatus = FdtClient->FindNextCompatibleNode (
                                     FdtClient,
                                     "virtio,mmio",
                                     Node,
                                     &Node
                                     ))
  {
    Status = FdtClient->GetNodeProperty (
                          FdtClient,
                          Node,
                          "reg",
                          (CONST VOID **)&Reg,
                          &RegSize
                          );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: GetNodeProperty () failed (Status == %r)\n",
        __func__,
        Status
        ));
      continue;
    }

    if (RegSize != 16) {
      ASSERT (RegSize == 16);
      continue;
    }

    RegBase = SwapBytes64 (ReadUnaligned64 ((VOID *)&Reg[0]));
    Range   = SwapBytes64 (ReadUnaligned64 ((VOID *)&Reg[1]));
    DEBUG ((
      DEBUG_INFO,
      "virtio-mmio : RegBase = 0x%lx - 0x%lx\n",
      RegBase,
      Range
      ));

    Status = MapMmioMemory (
               RegBase,
               Range,
               (EFI_MEMORY_UC | EFI_MEMORY_XP)
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "virtio-mmio : Failed to map memory region - RegBase = 0x%lx - 0x%lx\n",
        RegBase,
        Range
        ));
      return Status;
    }
  } // for

  return Status;
}
