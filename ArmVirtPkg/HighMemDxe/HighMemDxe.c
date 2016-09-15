/** @file
*  High memory node enumeration DXE driver for ARM Virtual Machines
*
*  Copyright (c) 2015-2016, Linaro Ltd. All rights reserved.
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
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/FdtClient.h>

EFI_STATUS
EFIAPI
InitializeHighMemDxe (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  FDT_CLIENT_PROTOCOL   *FdtClient;
  EFI_STATUS            Status, FindNodeStatus;
  INT32                 Node;
  CONST UINT32          *Reg;
  UINT32                RegSize;
  UINTN                 AddressCells, SizeCells;
  UINT64                CurBase;
  UINT64                CurSize;

  Status = gBS->LocateProtocol (&gFdtClientProtocolGuid, NULL,
                  (VOID **)&FdtClient);
  ASSERT_EFI_ERROR (Status);

  //
  // Check for memory node and add the memory spaces except the lowest one
  //
  for (FindNodeStatus = FdtClient->FindMemoryNodeReg (FdtClient, &Node,
                                     (CONST VOID **) &Reg, &AddressCells,
                                     &SizeCells, &RegSize);
       !EFI_ERROR (FindNodeStatus);
       FindNodeStatus = FdtClient->FindNextMemoryNodeReg (FdtClient, Node,
                                     &Node, (CONST VOID **) &Reg, &AddressCells,
                                     &SizeCells, &RegSize)) {
    ASSERT (AddressCells <= 2);
    ASSERT (SizeCells <= 2);

    while (RegSize > 0) {
      CurBase = SwapBytes32 (*Reg++);
      if (AddressCells > 1) {
        CurBase = (CurBase << 32) | SwapBytes32 (*Reg++);
      }
      CurSize = SwapBytes32 (*Reg++);
      if (SizeCells > 1) {
        CurSize = (CurSize << 32) | SwapBytes32 (*Reg++);
      }
      RegSize -= (AddressCells + SizeCells) * sizeof (UINT32);

      if (PcdGet64 (PcdSystemMemoryBase) != CurBase) {
        Status = gDS->AddMemorySpace (EfiGcdMemoryTypeSystemMemory, CurBase,
                        CurSize, EFI_MEMORY_WB);

        if (EFI_ERROR (Status)) {
          DEBUG ((EFI_D_ERROR,
            "%a: Failed to add System RAM @ 0x%lx - 0x%lx (%r)\n",
            __FUNCTION__, CurBase, CurBase + CurSize - 1, Status));
          continue;
        }

        Status = gDS->SetMemorySpaceAttributes (CurBase, CurSize,
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

  return EFI_SUCCESS;
}
