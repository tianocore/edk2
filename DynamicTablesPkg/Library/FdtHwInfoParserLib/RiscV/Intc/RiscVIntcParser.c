/** @file
  RISC-V Interrupt Controllers parser.

  Copyright (c) 2024, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/riscv/cpus.yaml
**/

#include <IndustryStandard/Acpi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FdtLib.h>
#include "FdtHwInfoParser.h"
#include "CmObjectDescUtility.h"
#include "RiscV/Intc/RiscVIntcParser.h"

#define ACPI_BUILD_EXT_INTC_ID(PlicAplicId, CtxIdcId) \
                    ((PlicAplicId << 24) | (CtxIdcId))

/** List of "compatible" property values for CPU nodes.

  Any other "compatible" value is not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  CpuCompatibleStr[] = {
  { "riscv" }
};

/** COMPATIBILITY_INFO structure for CPU nodes.
*/
STATIC CONST COMPATIBILITY_INFO  CpuCompatibleInfo = {
  ARRAY_SIZE (CpuCompatibleStr),
  CpuCompatibleStr
};

/** List of "compatible" property values for IMSIC node.

  Any other "compatible" value is not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  ImsicCompatibleStr[] = {
  { "riscv,imsics" }
};

/** COMPATIBILITY_INFO structure for IMSIC node.
*/
STATIC CONST COMPATIBILITY_INFO  ImsicCompatibleInfo = {
  ARRAY_SIZE (ImsicCompatibleStr),
  ImsicCompatibleStr
};

/** List of "compatible" property values for APLIC node.

  Any other "compatible" value is not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  AplicCompatibleStr[] = {
  { "riscv,aplic" }
};

/** COMPATIBILITY_INFO structure for APLIC node.
*/
STATIC CONST COMPATIBILITY_INFO  AplicCompatibleInfo = {
  ARRAY_SIZE (AplicCompatibleStr),
  AplicCompatibleStr
};

/** List of "compatible" property values for PLIC node.

  Any other "compatible" value is not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  PlicCompatibleStr[] = {
  { "riscv,plic0" }
};

/** COMPATIBILITY_INFO structure for IMSIC node.
*/
STATIC CONST COMPATIBILITY_INFO  PlicCompatibleInfo = {
  ARRAY_SIZE (PlicCompatibleStr),
  PlicCompatibleStr
};

STATIC LIST_ENTRY  ExtIntcList;

/** Parse a "cpu" node.

  CPU node parser.

  @param [in]  FdtParserHandle    A handle to the parser instance.
  @param [in]  CpuNode            cpu node.
  @param [in]  AddressCells       AddressCells info.
  @param [out] RintcInfo          Pointer to RINTC Info structure.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
STATIC
EFI_STATUS
EFIAPI
CpuNodeParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      CpuNode,
  IN        UINT32                     AddressCells,
  OUT       CM_RISCV_RINTC_INFO        *RintcInfo
  )
{
  STATIC UINT32  ProcUid;
  CONST UINT8    *Data;
  EFI_STATUS     Status;
  UINT64         HartId;
  INT32          DataSize;
  INT32          IntcNode;
  VOID           *Fdt;

  if (RintcInfo == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Fdt  = FdtParserHandle->Fdt;
  Data = FdtGetProp (Fdt, CpuNode, "reg", &DataSize);
  if ((Data == NULL)                  ||
      ((DataSize != sizeof (UINT32))  &&
       (DataSize != sizeof (UINT64))))
  {
    ASSERT (0);
    return EFI_ABORTED;
  }

  IntcNode = CpuNode;
  Status   = FdtGetNextNamedNodeInBranch (Fdt, CpuNode, "interrupt-controller", &IntcNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    if (Status == EFI_NOT_FOUND) {
      // Should have found the node.
      Status = EFI_ABORTED;
    }
  }

  if (AddressCells == 2) {
    HartId = Fdt64ToCpu (*((UINT64 *)Data));
  } else {
    HartId = Fdt32ToCpu (*((UINT32 *)Data));
  }

  RintcInfo->Flags            = EFI_ACPI_6_6_RINTC_FLAG_ENABLE; // REVISIT - check status
  RintcInfo->HartId           = HartId;
  RintcInfo->Version          = 1;
  RintcInfo->AcpiProcessorUid = ProcUid++;
  RintcInfo->ExtIntcId        = 0;
  return EFI_SUCCESS;
}

/** CPU node parser

  CPU node parser.

  @param [in]  FdtParserHandle    A handle to the parser instance.
  @param [in]  CpusNode           cpus node.
  @param [out] NewRintcCmObjDesc   Pointer to array of RINTC CM objects.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
STATIC
EFI_STATUS
EFIAPI
CpusNodeParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      CpusNode,
  OUT       CM_OBJ_DESCRIPTOR          **NewRintcCmObjDesc
  )
{
  EFI_STATUS  Status;
  UINT32      CpuNodeCount;
  INT32       CpuNode;
  INT32       AddressCells;

  UINT32               Index;
  CM_RISCV_RINTC_INFO  *RintcInfoBuffer;
  UINT32               RintcInfoBufferSize;
  VOID                 *Fdt;

  if (NewRintcCmObjDesc == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Fdt          = FdtParserHandle->Fdt;
  AddressCells = FdtAddressCells (Fdt, CpusNode);
  if (AddressCells < 0) {
    ASSERT (0);
    return EFI_ABORTED;
  }

  // Count the number of "cpu" nodes under the "cpus" node.
  Status = FdtCountNamedNodeInBranch (Fdt, CpusNode, "cpu", &CpuNodeCount);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  if (CpuNodeCount == 0) {
    ASSERT (0);
    return EFI_NOT_FOUND;
  }

  RintcInfoBufferSize = CpuNodeCount * sizeof (CM_RISCV_RINTC_INFO);
  RintcInfoBuffer     = AllocateZeroPool (RintcInfoBufferSize);
  if (RintcInfoBuffer == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  CpuNode = CpusNode;
  for (Index = 0; Index < CpuNodeCount; Index++) {
    Status = FdtGetNextNamedNodeInBranch (Fdt, CpusNode, "cpu", &CpuNode);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      if (Status == EFI_NOT_FOUND) {
        // Should have found the node.
        Status = EFI_ABORTED;
      }

      goto exit_handler;
    }

    // Parse the "cpu" node.
    if (!FdtNodeIsCompatible (Fdt, CpuNode, &CpuCompatibleInfo)) {
      ASSERT (0);
      Status = EFI_UNSUPPORTED;
      goto exit_handler;
    }

    Status = CpuNodeParser (
               FdtParserHandle,
               CpuNode,
               AddressCells,
               &RintcInfoBuffer[Index]
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      goto exit_handler;
    }
  } // for

  Status = CreateCmObjDesc (
             CREATE_CM_RISCV_OBJECT_ID (ERiscVObjRintcInfo),
             CpuNodeCount,
             RintcInfoBuffer,
             RintcInfoBufferSize,
             NewRintcCmObjDesc
             );
  ASSERT_EFI_ERROR (Status);

exit_handler:
  FreePool (RintcInfoBuffer);
  return Status;
}

/** Find the RINTC structure for a phandle

  @param [in] NewRintcCmObjDesc  Pointer to array of RINTC CM objects.
  @param [in] Phandle            Phandle to search

**/
STATIC
CM_RISCV_RINTC_INFO *
RiscVFindRintc (
  IN CM_OBJ_DESCRIPTOR  *NewRintcCmObjDesc,
  UINTN                 HartId
  )
{
  CM_RISCV_RINTC_INFO  *RintcInfo;
  INT32                Idx;

  RintcInfo = (CM_RISCV_RINTC_INFO *)NewRintcCmObjDesc->Data;

  for (Idx = 0; Idx < NewRintcCmObjDesc->Count; Idx++) {
    if (RintcInfo[Idx].HartId == HartId) {
      return &RintcInfo[Idx];
    }
  }

  return NULL;
}

STATIC
EFI_STATUS
FdtCreateExtIntcList (
  INT32   ExtIntcNode,
  UINT32  GsiBase
  )
{
  RISCV_EXT_INTC_DATA  *Node;

  Node = AllocateZeroPool (sizeof (RISCV_EXT_INTC_DATA));
  if (Node == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  // Initialise the list head.
  InitializeListHead (&ExtIntcList);
  Node->Signature   = RISCV_EXT_INTC_DATA_SIGNATURE;
  Node->ExtIntcNode = ExtIntcNode;
  Node->GsiBase     = GsiBase;
  InsertTailList (&ExtIntcList, &Node->Link);
  return EFI_SUCCESS;
}

UINT32
FdtConvertToGsi (
  INT32   ExtIntcNode,
  UINT32  Irq
  )
{
  LIST_ENTRY           *Node;
  RISCV_EXT_INTC_DATA  *Data;

  Node = GetFirstNode (&ExtIntcList);
  while (!IsNull (&ExtIntcList, Node)) {
    Data = RISCV_EXT_INTC_INFO_FROM_THIS (Node);
    if (Data->ExtIntcNode == ExtIntcNode) {
      return Data->GsiBase + Irq;
    }

    Node = GetNextNode (&ExtIntcList, Node);
  }

  return Irq;
}

VOID
FdtFreeExtIntcList (
  )
{
  RISCV_EXT_INTC_DATA  *Data;
  LIST_ENTRY           *NextNode;
  LIST_ENTRY           *Node;

  Node = GetFirstNode (&ExtIntcList);
  while (!IsNull (&ExtIntcList, Node)) {
    Data     = RISCV_EXT_INTC_INFO_FROM_THIS (Node);
    NextNode = GetNextNode (&ExtIntcList, Node);
    RemoveEntryList (&Data->Link);
    FreePool (Data);

    Node = NextNode;
  }
}

/** Check if it is a PLIC


  @param [in]  Fdt             Pointer to device tree.
  @param [in]  AplicNode       Node with APLIC compatible property.

  @retval TRUE                 The AplicNode is S-mode APLIC
  @retval FALSE                The AplicNode is not S-mode APLIC
**/
STATIC
BOOLEAN
IsPlicNode (
  IN VOID   *Fdt,
  IN INT32  ExtIntcNode
  )
{
  if (FdtNodeIsCompatible (Fdt, ExtIntcNode, &PlicCompatibleInfo)) {
    return TRUE;
  }

  return FALSE;
}

/** Check if it is S-mode APLIC

  FDT will have entries for both M-mode and S-mode APLIC. We
  need only S-mode APLIC.

  @param [in]  Fdt             Pointer to device tree.
  @param [in]  AplicNode       Node with APLIC compatible property.

  @retval TRUE                 The AplicNode is S-mode APLIC
  @retval FALSE                The AplicNode is not S-mode APLIC
**/
STATIC
BOOLEAN
IsAplicNode (
  IN VOID   *Fdt,
  IN INT32  ExtIntcNode
  )
{
  INT32  *IrqProp;
  INT32  *MsiProp;
  INT32  Len, ImsicNode;

  if (!FdtNodeIsCompatible (Fdt, ExtIntcNode, &AplicCompatibleInfo)) {
    return FALSE;
  }

  IrqProp = (INT32 *)FdtGetProp (Fdt, ExtIntcNode, "interrupts-extended", &Len);
  if ((IrqProp > 0) && (Len >= 4) &&
      (Fdt32ToCpu (IrqProp[1]) == IRQ_S_EXT))
  {
    return TRUE;
  }

  MsiProp = (INT32 *)FdtGetProp (Fdt, ExtIntcNode, "msi-parent", &Len);
  if (MsiProp && (Len >= sizeof (INT32))) {
    ImsicNode = FdtNodeOffsetByPhandle (Fdt, Fdt32ToCpu (*MsiProp));
    if (ImsicNode < 0) {
      return FALSE;
    }

    IrqProp = (INT32 *)FdtGetProp (Fdt, ImsicNode, "interrupts-extended", &Len);
    if ((IrqProp > 0) && (Len >= 4) &&
        (Fdt32ToCpu (IrqProp[1]) == IRQ_S_EXT))
    {
      return TRUE;
    }
  }

  return FALSE;
}

STATIC
EFI_STATUS
RiscVUpdateRintc (
  IN CONST VOID                *Fdt,
  IN  OUT   CM_OBJ_DESCRIPTOR  *NewRintcCmObjDesc,
  IN INT32                     CpuNode,
  IN UINT32                    ExtIntcId
  )
{
  CM_RISCV_RINTC_INFO  *RintcInfo;
  CONST INT32          *Data;
  UINT32               AddressCells;
  INT32                DataSize;
  UINTN                HartId;
  INT32                CpusNode;

  CpusNode     = FdtParentOffset (Fdt, CpuNode);
  AddressCells = FdtAddressCells (Fdt, CpusNode);
  if (AddressCells < 0) {
    ASSERT (0);
    return EFI_ABORTED;
  }

  Data = FdtGetProp (Fdt, CpuNode, "reg", &DataSize);
  if ((Data == NULL)                  ||
      ((DataSize != sizeof (UINT32))  &&
       (DataSize != sizeof (UINT64))))
  {
    ASSERT (0);
    return EFI_ABORTED;
  }

  if (AddressCells == 2) {
    HartId = Fdt64ToCpu (*((UINT64 *)Data));
  } else {
    HartId = Fdt32ToCpu (*((UINT32 *)Data));
  }

  RintcInfo = RiscVFindRintc (NewRintcCmObjDesc, HartId);
  if (RintcInfo == NULL) {
    ASSERT (0);
    return EFI_NOT_FOUND;
  }

  /* Update RINTC EXT INTC ID */
  RintcInfo->ExtIntcId = ExtIntcId;

  return EFI_SUCCESS;
}

/**
  PLIC/APLIC parser and update RINTC

  @param [in]      FdtParserHandle    A handle to the parser instance.
  @param [in, out] NewRintcCmObjDesc  Pointer to array of RINTC CM objects.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
STATIC
EFI_STATUS
EFIAPI
PlicAplicInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN  OUT   CM_OBJ_DESCRIPTOR          *NewRintcCmObjDesc
  )
{
  PLIC_APLIC_COMMON_INFO  PlicAplicCommonInfo;
  CM_RISCV_PLIC_INFO      *PlicInfo;
  CM_RISCV_APLIC_INFO     *AplicInfo = NULL;
  CONST UINT32            *Prop;
  CONST UINT32            *IntExtProp;
  EFI_STATUS              Status;
  UINT32                  GsiBase;
  INT32                   Len, LocalCpuId;
  INT32                   Id, Idx, Idx1, Idx2, Phandle;
  INT32                   Prev, ExtIntcNode;
  INT32                   IntcNode;
  INT32                   CpuNode;
  INT32                   ExtIntcId;
  VOID                    *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Fdt     = FdtParserHandle->Fdt;
  GsiBase = 0;
  Id      = 0;

  for (Prev = 0; ; Prev = ExtIntcNode) {
    ExtIntcNode = FdtNextNode (Fdt, Prev, NULL);
    if (ExtIntcNode < 0) {
      return EFI_SUCCESS;
    }

    if (!(IsPlicNode (Fdt, ExtIntcNode) || IsAplicNode (Fdt, ExtIntcNode))) {
      continue;
    }

    FdtCreateExtIntcList (ExtIntcNode, GsiBase);

    PlicAplicCommonInfo.Version = 1;
    PlicAplicCommonInfo.Id      = Id;
    Prop                        = FdtGetProp (Fdt, ExtIntcNode, "riscv,num-sources", &Len);
    if (!Prop) {
      Prop = FdtGetProp (Fdt, ExtIntcNode, "riscv,ndev", &Len);
      if (!Prop) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }
    }

    PlicAplicCommonInfo.NumSources = Fdt32ToCpu (*(const UINT32 *)Prop);

    Prop = FdtGetProp (Fdt, ExtIntcNode, "reg", &Len);
    if ((Prop == NULL) || (Len % 4 > 0)) {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    PlicAplicCommonInfo.BaseAddress = Fdt64ToCpu (*(const UINT64 *)Prop);
    PlicAplicCommonInfo.Size        = Fdt64ToCpu (*((const UINT64 *)Prop + 1));
    PlicAplicCommonInfo.GsiBase     = GsiBase;
    GsiBase                        += PlicAplicCommonInfo.NumSources;

    if (FdtNodeIsCompatible (Fdt, ExtIntcNode, &PlicCompatibleInfo)) {
      IntExtProp = FdtGetProp (Fdt, ExtIntcNode, "interrupts-extended", &Len);
      if ((IntExtProp == NULL) || (Len < 4)) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      } else {
        PlicInfo = AllocateZeroPool (sizeof (CM_RISCV_PLIC_INFO));
        if (PlicInfo == NULL) {
          ASSERT (0);
          return EFI_OUT_OF_RESOURCES;
        }

        Len = Len / 4;
        for (Idx1 = 0, Idx2 = 0; Idx1 < Len; Idx1 += 2, Idx2++) {
          if (Fdt32ToCpu (IntExtProp[Idx1 + 1]) == IRQ_S_EXT) {
            Phandle    = Fdt32ToCpu (IntExtProp[Idx1]);
            IntcNode   = FdtNodeOffsetByPhandle (Fdt, Phandle);
            CpuNode    = FdtParentOffset (Fdt, IntcNode);
            LocalCpuId = Idx2 / 2;
            ExtIntcId  = ACPI_BUILD_EXT_INTC_ID (Id, 2 * LocalCpuId + 1);
            RiscVUpdateRintc (Fdt, NewRintcCmObjDesc, CpuNode, ExtIntcId);
          }
        }

        PlicAplicCommonInfo.HwId[0] = 'R';
        PlicAplicCommonInfo.HwId[1] = 'S';
        PlicAplicCommonInfo.HwId[2] = 'C';
        PlicAplicCommonInfo.HwId[3] = 'V';
        PlicAplicCommonInfo.HwId[4] = '0';
        PlicAplicCommonInfo.HwId[5] = '0';
        PlicAplicCommonInfo.HwId[6] = '0';
        PlicAplicCommonInfo.HwId[7] = '1';

        CopyMem (&PlicInfo->PlicAplicCommonInfo, &PlicAplicCommonInfo, sizeof (PLIC_APLIC_COMMON_INFO));
        // Add the CmObj to the Configuration Manager.
        Status = AddSingleCmObj (
                   FdtParserHandle,
                   CREATE_CM_RISCV_OBJECT_ID (ERiscVObjPlicInfo),
                   PlicInfo,
                   sizeof (CM_RISCV_PLIC_INFO),
                   NULL
                   );
        if (EFI_ERROR (Status)) {
          ASSERT (0);
          return Status;
        }
      }
    } else if (FdtNodeIsCompatible (Fdt, ExtIntcNode, &AplicCompatibleInfo)) {
      AplicInfo = AllocateZeroPool (sizeof (CM_RISCV_APLIC_INFO));
      if (AplicInfo == NULL) {
        ASSERT (0);
        return EFI_OUT_OF_RESOURCES;
      }

      IntExtProp = FdtGetProp (Fdt, ExtIntcNode, "interrupts-extended", &Len);
      if ((IntExtProp != 0) && ((Len / sizeof (UINT32)) % 2 == 0)) {
        Len                = Len / 4;
        AplicInfo->NumIdcs = Len / 2;
        for (Idx = 0; Idx < Len; Idx += 2) {
          Phandle   = Fdt32ToCpu (IntExtProp[Idx]);
          IntcNode  = FdtNodeOffsetByPhandle (Fdt, Phandle);
          CpuNode   = FdtParentOffset (Fdt, IntcNode);
          ExtIntcId = ACPI_BUILD_EXT_INTC_ID (Id, Idx / 2);
          RiscVUpdateRintc (Fdt, NewRintcCmObjDesc, CpuNode, ExtIntcId);
        }
      }

      PlicAplicCommonInfo.HwId[0] = 'R';
      PlicAplicCommonInfo.HwId[1] = 'S';
      PlicAplicCommonInfo.HwId[2] = 'C';
      PlicAplicCommonInfo.HwId[3] = 'V';
      PlicAplicCommonInfo.HwId[4] = '0';
      PlicAplicCommonInfo.HwId[5] = '0';
      PlicAplicCommonInfo.HwId[6] = '0';
      PlicAplicCommonInfo.HwId[7] = '2';

      CopyMem (&AplicInfo->PlicAplicCommonInfo, &PlicAplicCommonInfo, sizeof (PLIC_APLIC_COMMON_INFO));
      // Add the CmObj to the Configuration Manager.
      Status = AddSingleCmObj (
                 FdtParserHandle,
                 CREATE_CM_RISCV_OBJECT_ID (ERiscVObjAplicInfo),
                 AplicInfo,
                 sizeof (CM_RISCV_APLIC_INFO),
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        return Status;
      }
    }

    Id++;
  }

  return EFI_SUCCESS;
}

/**
  RINTC parser using IMSIC node

  Parse RINTC information using IMSIC.

  @param [in]      FdtParserHandle     A handle to the parser instance.
  @param [in, out] NewRintcCmObjDesc   Pointer to array of RINTC CM objects.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
STATIC
EFI_STATUS
EFIAPI
ImsicRintcInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN  OUT   CM_OBJ_DESCRIPTOR          *NewRintcCmObjDesc
  )
{
  CM_RISCV_RINTC_INFO  *RintcInfoBuffer;
  CM_RISCV_IMSIC_INFO  ImsicInfo;
  CONST UINT64         *Prop;
  CONST UINT32         *IntExtProp;
  CONST UINT64         *ImsicRegProp;
  EFI_STATUS           Status;
  UINT64               ImsicBaseAddr, ImsicBaseLen;
  UINT64               ImsicCpuBaseAddr, ImsicCpuBaseLen;
  UINTN                NumImsicBase;
  INT32                Len, NumPhandle, Phandle;
  INT32                Prev, ImsicNode;
  INT32                IntcNode;
  INT32                AddressCells;
  INTN                 Idx, Idx1, Idx2, Limit;
  VOID                 *Fdt;
  CONST INT32          *Data;
  INT32                CpusNode;
  INT32                CpuNode;
  UINTN                HartId;
  INT32                DataSize;

  if (NewRintcCmObjDesc == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;
  ZeroMem (&ImsicInfo, sizeof (CM_RISCV_IMSIC_INFO));

  for (Prev = 0; ; Prev = ImsicNode) {
    ImsicNode = FdtNextNode (Fdt, Prev, NULL);
    if (ImsicNode < 0) {
      return EFI_NOT_FOUND;
    }

    if (FdtNodeIsCompatible (Fdt, ImsicNode, &ImsicCompatibleInfo)) {
      IntExtProp = FdtGetProp (Fdt, ImsicNode, "interrupts-extended", &Len);
      if ((IntExtProp == 0) || ((Len / sizeof (UINT32)) % 2 != 0)) {
        /* interrupts-extended: <phandle>, <flag> */
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      /* There can be M-mode IMSIC in DT. Consider only S-mode */
      if (Fdt32ToCpu (IntExtProp[1]) == IRQ_S_EXT) {
        NumPhandle = (Len / sizeof (UINT32)) / 2;
        if (NumPhandle == 0) {
          ASSERT (0);
          return EFI_NOT_FOUND;
        }

        Prop = FdtGetProp (Fdt, ImsicNode, "riscv,num-ids", &Len);
        if (Prop == 0) {
          ASSERT (0);
          return EFI_INVALID_PARAMETER;
        }

        ImsicInfo.NumIds = Fdt32ToCpu (*(const UINT32 *)Prop);
        Prop             = FdtGetProp (Fdt, ImsicNode, "riscv,num-guest-ids", &Len);
        if (Prop == 0) {
          ImsicInfo.NumGuestIds = ImsicInfo.NumIds;
        } else {
          ImsicInfo.NumGuestIds = Fdt32ToCpu (*(const UINT32 *)Prop);
        }

        Prop = FdtGetProp (Fdt, ImsicNode, "riscv,guest-index-bits", &Len);
        if (Prop == 0) {
          ImsicInfo.GuestIndexBits = 0;
        } else {
          ImsicInfo.GuestIndexBits = Fdt32ToCpu (*(const UINT32 *)Prop);
        }

        Prop = FdtGetProp (Fdt, ImsicNode, "riscv,hart-index-bits", &Len);
        if (Prop == 0) {
          ImsicInfo.HartIndexBits = 0; // update default value later
        } else {
          ImsicInfo.HartIndexBits = Fdt32ToCpu (*(const UINT32 *)Prop);
        }

        Prop = FdtGetProp (Fdt, ImsicNode, "riscv,group-index-bits", &Len);
        if (Prop == 0) {
          ImsicInfo.GroupIndexBits = 0;
        } else {
          ImsicInfo.GroupIndexBits = Fdt32ToCpu (*(const UINT32 *)Prop);
        }

        Prop = FdtGetProp (Fdt, ImsicNode, "riscv,group-index-shift", &Len);
        if (Prop == 0) {
          ImsicInfo.GroupIndexShift = IMSIC_MMIO_PAGE_SHIFT * 2;
        } else {
          ImsicInfo.GroupIndexShift = Fdt32ToCpu (*(const UINT32 *)Prop);
        }

        ImsicInfo.Version = 1;
        ImsicInfo.Flags   = 0;
        Prop              = FdtGetProp (Fdt, ImsicNode, "reg", &Len);
        if ((Prop == 0) || (((Len / sizeof (UINT32)) % 4) != 0)) {
          // address-cells and size-cells are always 2
          DEBUG (
            (
             DEBUG_ERROR,
             "%a: Failed to parse imsic node: reg\n",
             __func__
            )
            );
          return EFI_INVALID_PARAMETER;
        }

        ImsicRegProp = Prop;
        NumImsicBase = (Len / sizeof (UINT32)) / 4;
        if (ImsicInfo.HartIndexBits == 0) {
          Len = NumPhandle;
          while (Len > 0) {
            ImsicInfo.HartIndexBits++;
            Len = Len >> 1;
          }
        }

        Idx2 = 0;
        for (Idx = 0; Idx < NumImsicBase; Idx++) {
          ImsicBaseAddr = Fdt64ToCpu (ImsicRegProp[Idx * 2]);
          ImsicBaseLen  = Fdt64ToCpu (ImsicRegProp[Idx * 2 + 1]);
          // Calculate the limit of number of cpu nodes this imsic can handle
          Limit = ImsicBaseLen /  IMSIC_MMIO_PAGE_SZ;
          for (Idx1 = 0; Idx1 < Limit && Idx2 < NumPhandle; Idx1++, Idx2++) {
            Phandle      = Fdt32ToCpu (IntExtProp[Idx2 * 2]);
            IntcNode     = FdtNodeOffsetByPhandle (Fdt, Phandle);
            CpuNode      = FdtParentOffset (Fdt, IntcNode);
            CpusNode     = FdtParentOffset (Fdt, CpuNode);
            AddressCells = FdtAddressCells (Fdt, CpusNode);
            if (AddressCells < 0) {
              ASSERT (0);
              return EFI_ABORTED;
            }

            Data = FdtGetProp (Fdt, CpuNode, "reg", &DataSize);
            if ((Data == NULL)                  ||
                ((DataSize != sizeof (UINT32))  &&
                 (DataSize != sizeof (UINT64))))
            {
              ASSERT (0);
              return EFI_ABORTED;
            }

            if (AddressCells == 2) {
              HartId = Fdt64ToCpu (*((UINT64 *)Data));
            } else {
              HartId = Fdt32ToCpu (*((UINT32 *)Data));
            }

            RintcInfoBuffer = RiscVFindRintc (NewRintcCmObjDesc, HartId);
            if (RintcInfoBuffer == NULL) {
              DEBUG ((DEBUG_ERROR, "%a: Failed to find RINTC node\n", __func__));
              return EFI_NOT_FOUND;
            }

            ImsicCpuBaseAddr                  = ImsicBaseAddr + Idx1 * IMSIC_MMIO_PAGE_SZ;
            ImsicCpuBaseLen                   = IMSIC_MMIO_PAGE_SZ;
            RintcInfoBuffer->ImsicBaseAddress = ImsicCpuBaseAddr;
            RintcInfoBuffer->ImsicSize        = ImsicCpuBaseLen;
          }
        }

        // Add the CmObj to the Configuration Manager.
        Status = AddSingleCmObj (
                   FdtParserHandle,
                   CREATE_CM_RISCV_OBJECT_ID (ERiscVObjImsicInfo),
                   &ImsicInfo,
                   sizeof (CM_RISCV_IMSIC_INFO),
                   NULL
                   );
        if (EFI_ERROR (Status)) {
          ASSERT (0);
        }

        return Status;
      }
    }
  }

  return Status;
}

/** CM_RISCV_RINTC_INFO and CM_ARCH_IMSIC_INFO parser function.

  A parser parses a Device Tree to populate a specific CmObj type. None,
  one or many CmObj can be created by the parser.
  The created CmObj are then handed to the parser's caller through the
  HW_INFO_ADD_OBJECT interface.
  This can also be a dispatcher. I.e. a function that not parsing a
  Device Tree but calling other parsers.

  @param [in]  FdtParserHandle A handle to the parser instance.
  @param [in]  FdtBranch       When searching for DT node name, restrict
                               the search to this Device Tree branch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
STATIC
EFI_STATUS
EFIAPI
RiscVIntcInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  )
{
  CM_OBJ_DESCRIPTOR  *NewCmObjDesc;
  EFI_STATUS         Status;
  VOID               *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Fdt          = FdtParserHandle->Fdt;
  NewCmObjDesc = NULL;

  // Parse the "cpus" nodes and its children "cpu" nodes,
  // and create a CM_OBJ_DESCRIPTOR.
  Status = CpusNodeParser (FdtParserHandle, FdtBranch, &NewCmObjDesc);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  /* Search for IMSIC presence and update RINTC structures if so */
  Status = ImsicRintcInfoParser (FdtParserHandle, NewCmObjDesc);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    ASSERT_EFI_ERROR (Status);
    goto exit_handler;
  }

  /* Search for APLIC/PLIC presence and update RINTC structures if so */
  Status = PlicAplicInfoParser (FdtParserHandle, NewCmObjDesc);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    ASSERT_EFI_ERROR (Status);
    goto exit_handler;
  }

  // Finally, add all the RINTC CmObjs to the Configuration Manager.
  Status = AddMultipleCmObj (FdtParserHandle, NewCmObjDesc, 0, NULL);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

exit_handler:
  FreeCmObjDesc (NewCmObjDesc);
  return Status;
}

/** MADT dispatcher.

  A parser parses a Device Tree to populate a specific CmObj type. None,
  one or many CmObj can be created by the parser.
  The created CmObj are then handed to the parser's caller through the
  HW_INFO_ADD_OBJECT interface.
  This can also be a dispatcher. I.e. a function that not parsing a
  Device Tree but calling other parsers.

  @param [in]  FdtParserHandle A handle to the parser instance.
  @param [in]  FdtBranch       When searching for DT node name, restrict
                               the search to this Device Tree branch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
EFI_STATUS
EFIAPI
RiscVIntcDispatcher (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  )
{
  EFI_STATUS  Status;
  INT32       CpusNode;
  VOID        *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;

  // The "cpus" node resides at the root of the DT. Fetch it.
  CpusNode = FdtPathOffset (Fdt, "/cpus");
  if (CpusNode < 0) {
    return EFI_NOT_FOUND;
  }

  Status = RiscVIntcInfoParser (FdtParserHandle, CpusNode);
  if (EFI_ERROR (Status)) {
    // EFI_NOT_FOUND is not tolerated at this point.
    ASSERT (0);
    return Status;
  }

  return EFI_SUCCESS;
}
