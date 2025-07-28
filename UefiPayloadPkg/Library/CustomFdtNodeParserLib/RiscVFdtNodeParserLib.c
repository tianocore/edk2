/** @file
 * Parser functionality specific to Risc-V
  Copyright (c) 2024, Rivos Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Pi/PiHob.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PrintLib.h>
#include <Library/FdtLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include "../UefiPayloadEntry/UefiPayloadEntry.h"
#include <UniversalPayload/Ramdisk.h>

/**
  It will Parse FDT -custom node based on information from bootloaders.
  @param[in]  FdtBase The starting memory address of FdtBase
  @param[in]  HobList The starting memory address of New Hob list.

**/
UINTN
EFIAPI
CustomFdtNodeParser (
  IN VOID  *FdtBase,
  IN VOID  *HobList
  )
{
  UINT64                     *Data64;
  INT32                      Depth;
  INT32                      Node;
  FDT_NODE_HEADER            *NodePtr;
  UINT64                     NumberOfBytes;
  CONST FDT_PROPERTY         *PropertyPtr;
  UNIVERSAL_PAYLOAD_RAMDISK  *Ramdisk;
  UINT64                     StartAddress;
  INT32                      SubNode;
  INT32                      TempLen;
  CONST CHAR8                *TempStr;

  // Look for a reserved memory range with compatible = "ramdisk"
  Depth = 0;
  for (Node = FdtNextNode (FdtBase, 0, &Depth); Node >= 0; Node = FdtNextNode (FdtBase, Node, &Depth)) {
    NodePtr = (FDT_NODE_HEADER *)((CONST CHAR8 *)FdtBase + Node + Fdt32ToCpu (((FDT_HEADER *)FdtBase)->OffsetDtStruct));
    if (AsciiStrnCmp (NodePtr->Name, "reserved-memory", AsciiStrLen ("reserved-memory")) != 0) {
      continue;
    }

    for (SubNode = FdtFirstSubnode (FdtBase, Node); SubNode >= 0; SubNode = FdtNextSubnode (FdtBase, SubNode)) {
      NodePtr       = (FDT_NODE_HEADER *)((CONST CHAR8 *)FdtBase + SubNode + Fdt32ToCpu (((FDT_HEADER *)FdtBase)->OffsetDtStruct));
      PropertyPtr   = FdtGetProperty (FdtBase, SubNode, "reg", &TempLen);
      Data64        = (UINT64 *)(PropertyPtr->Data);
      StartAddress  = Fdt64ToCpu (*Data64);
      NumberOfBytes = Fdt64ToCpu (*(Data64 + 1));
      if (AsciiStrnCmp (NodePtr->Name, "memory@", AsciiStrLen ("memory@")) != 0) {
        continue;
      }

      PropertyPtr = FdtGetProperty (FdtBase, SubNode, "compatible", &TempLen);
      TempStr     = (CHAR8 *)(PropertyPtr->Data);
      if (AsciiStrnCmp (TempStr, "ramdisk", AsciiStrLen ("ramdisk")) != 0) {
        continue;
      }

      DEBUG ((DEBUG_INFO, "Found ramdisk\n"));
      Ramdisk = BuildGuidHob (&gUniversalPayloadRamdiskGuid, sizeof (UNIVERSAL_PAYLOAD_RAMDISK));
      if (Ramdisk != NULL) {
        Ramdisk->Header.Revision = UNIVERSAL_PAYLOAD_RAMDISK_REVISION;
        Ramdisk->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_RAMDISK);
        Ramdisk->RamdiskBase     = (EFI_PHYSICAL_ADDRESS)(UINTN)(StartAddress);
        Ramdisk->RamdiskSize     = (UINTN)NumberOfBytes;
      }
    }
  }

  return EFI_SUCCESS;
}
