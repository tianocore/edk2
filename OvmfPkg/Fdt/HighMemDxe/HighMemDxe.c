/** @file
*  High memory node enumeration DXE driver for ARM and RISC-V
*  Virtual Machines
*
*  Copyright (c) 2015-2016, Linaro Ltd. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/Cpu.h>
#include <Protocol/FdtClient.h>

EFI_STATUS
EFIAPI
InitializeHighMemDxe (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  FDT_CLIENT_PROTOCOL               *FdtClient;
  EFI_CPU_ARCH_PROTOCOL             *Cpu;
  EFI_STATUS                        Status, FindNodeStatus;
  INT32                             Node;
  CONST UINT32                      *Reg;
  UINT32                            RegSize;
  UINTN                             AddressCells, SizeCells;
  UINT64                            CurBase;
  UINT64                            CurSize;
  UINT64                            Attributes;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR   GcdDescriptor;

  Status = gBS->LocateProtocol (&gFdtClientProtocolGuid, NULL,
                  (VOID **)&FdtClient);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL,
                  (VOID **)&Cpu);
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

      Status = gDS->GetMemorySpaceDescriptor (CurBase, &GcdDescriptor);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN,
          "%a: Region 0x%lx - 0x%lx not found in the GCD memory space map\n",
          __FUNCTION__, CurBase, CurBase + CurSize - 1));
          continue;
      }
      if (GcdDescriptor.GcdMemoryType == EfiGcdMemoryTypeNonExistent) {
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
          DEBUG ((DEBUG_WARN,
            "%a: gDS->SetMemorySpaceAttributes() failed on region 0x%lx - 0x%lx (%r)\n",
            __FUNCTION__, CurBase, CurBase + CurSize - 1, Status));
        }

        //
        // Due to the ambiguous nature of the RO/XP GCD memory space attributes,
        // it is impossible to add a memory space with the XP attribute in a way
        // that does not result in the XP attribute being set on *all* UEFI
        // memory map entries that are carved from it, including code regions
        // that require executable permissions.
        //
        // So instead, we never set the RO/XP attributes in the GCD memory space
        // capabilities or attribute fields, and apply any protections directly
        // on the page table mappings by going through the cpu arch protocol.
        //
        Attributes = EFI_MEMORY_WB;
        if ((PcdGet64 (PcdDxeNxMemoryProtectionPolicy) &
             (1U << (UINT32)EfiConventionalMemory)) != 0) {
          Attributes |= EFI_MEMORY_XP;
        }

        Status = Cpu->SetMemoryAttributes (Cpu, CurBase, CurSize, Attributes);

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
