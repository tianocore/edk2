/** @file
  RISC-V Interrupt Controllers parser.

  Copyright (c) 2024, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/riscv/cpus.yaml
**/

#include "FdtHwInfoParser.h"
#include "CmObjectDescUtility.h"
#include "RiscVAcpi.h"
#include "RiscV/Intc/RiscVIntcParser.h"
#include "RiscV/Intc/RiscVIntcDispatcher.h"

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
  CONST UINT32   *Prop;
  CONST UINT8    *Data;
  EFI_STATUS     Status;
  UINT64         HartId;
  INT32          DataSize, Len;
  INT32          IntcNode;
  VOID           *Fdt;

  if (RintcInfo == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Fdt  = FdtParserHandle->Fdt;
  Data = fdt_getprop (Fdt, CpuNode, "reg", &DataSize);
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

  Prop = fdt_getprop (Fdt, IntcNode, "phandle", &Len);
  if (Prop || (Len > 0)) {
    RintcInfo->IntcPhandle = fdt32_to_cpu (*((UINT32 *)Prop));
  }

  if (AddressCells == 2) {
    HartId = fdt64_to_cpu (*((UINT64 *)Data));
  } else {
    HartId = fdt32_to_cpu (*((UINT32 *)Data));
  }

  RintcInfo->Flags            = EFI_ACPI_6_6_RINTC_FLAG_ENABLE; // REVISIT - check status
  RintcInfo->HartId           = HartId;
  RintcInfo->Version          = 1;
  RintcInfo->AcpiProcessorUid = ProcUid++;
  RintcInfo->ExtIntCId        = 0;
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
  AddressCells = fdt_address_cells (Fdt, CpusNode);
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
  INT32                 Phandle
  )
{
  CM_RISCV_RINTC_INFO  *RintcInfo;
  INT32                Idx;

  RintcInfo = (CM_RISCV_RINTC_INFO *)NewRintcCmObjDesc->Data;

  for (Idx = 0; Idx < NewRintcCmObjDesc->Count; Idx++) {
    if (RintcInfo[Idx].IntcPhandle == Phandle) {
      return &RintcInfo[Idx];
    }
  }

  return NULL;
}

/**
  PLIC parser and update RINTC

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
PlicRintcInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN  OUT   CM_OBJ_DESCRIPTOR          *NewRintcCmObjDesc
  )
{
  CM_RISCV_RINTC_INFO  *RintcInfo;
  CM_RISCV_PLIC_INFO   PlicInfo;
  CONST UINT32         *Prop;
  CONST UINT32         *IntExtProp;
  CONST UINT32         *PhandleProp;
  EFI_STATUS           Status;
  UINT32               PlicGsiBase;
  INT32                Len, LocalCpuId;
  INT32                PlicId, Idx1, Idx2, Phandle;
  INT32                Prev, PlicNode;
  VOID                 *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Fdt         = FdtParserHandle->Fdt;
  PlicGsiBase = 0;
  PlicId      = 0;

  for (Prev = 0; ; Prev = PlicNode) {
    PlicNode = fdt_next_node (Fdt, Prev, NULL);
    if (PlicNode < 0) {
      return EFI_SUCCESS;
    }

    if (FdtNodeIsCompatible (Fdt, PlicNode, &PlicCompatibleInfo)) {
      IntExtProp = fdt_getprop (Fdt, PlicNode, "interrupts-extended", &Len);
      if ((IntExtProp == NULL) || (Len < 4)) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      } else {
        Len = Len / 4;
        for (Idx1 = 0, Idx2 = 0; Idx1 < Len; Idx1 += 2, Idx2++) {
          if (fdt32_to_cpu (IntExtProp[Idx1 + 1]) == IRQ_S_EXT) {
            Phandle   = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)IntExtProp + Idx1));
            RintcInfo = RiscVFindRintc (NewRintcCmObjDesc, Phandle);
            if (RintcInfo == NULL) {
              ASSERT (0);
              return EFI_INVALID_PARAMETER;
            }

            LocalCpuId = Idx2 / 2;
            /* Update RINTC EXT INTC ID */
            RintcInfo->ExtIntCId = ACPI_BUILD_EXT_INTC_ID (PlicId, 2 * LocalCpuId + 1);
          }
        }

        ZeroMem (&PlicInfo, sizeof (CM_RISCV_PLIC_INFO));
        Prop = fdt_getprop (Fdt, PlicNode, "reg", &Len);
        if ((Prop == NULL) || (Len % 4 > 0)) {
          ASSERT (0);
          return EFI_INVALID_PARAMETER;
        }

        PlicInfo.PlicAddress = fdt64_to_cpu (ReadUnaligned64 ((const UINT64 *)Prop));
        PlicInfo.PlicSize    = fdt64_to_cpu (ReadUnaligned64 ((const UINT64 *)Prop + 1));
        Prop                 = fdt_getprop (Fdt, PlicNode, "riscv,ndev", &Len);
        if (Prop == NULL) {
          ASSERT (0);
          return EFI_INVALID_PARAMETER;
        }

        PlicInfo.NumSources = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop));
        PhandleProp         = fdt_getprop (Fdt, PlicNode, "phandle", &Len);
        if (PhandleProp == NULL) {
          ASSERT (0);
          return EFI_INVALID_PARAMETER;
        }

        PlicInfo.Phandle = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)PhandleProp));
        PlicInfo.GsiBase = PlicGsiBase;
        PlicInfo.Version = 1;
        PlicInfo.PlicId  = PlicId++;
        PlicGsiBase     += PlicInfo.NumSources;

        // Add the CmObj to the Configuration Manager.
        Status = AddSingleCmObj (
                   FdtParserHandle,
                   CREATE_CM_RISCV_OBJECT_ID (ERiscVObjPlicInfo),
                   &PlicInfo,
                   sizeof (CM_RISCV_PLIC_INFO),
                   NULL
                   );
        if (EFI_ERROR (Status)) {
          ASSERT (0);
          return Status;
        }
      }
    }
  }

  return EFI_SUCCESS;
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
IsSmodeAplic (
  IN VOID   *Fdt,
  IN INT32  AplicNode
  )
{
  fdt32_t  *IrqProp;
  fdt32_t  *MsiProp;
  INT32    Len, ImsicNode;

  IrqProp = (fdt32_t *)fdt_getprop (Fdt, AplicNode, "interrupts-extended", &Len);
  if ((IrqProp > 0) && (Len >= 4) &&
      (fdt32_to_cpu (IrqProp[1]) == IRQ_S_EXT))
  {
    return TRUE;
  }

  MsiProp = (fdt32_t *)fdt_getprop (Fdt, AplicNode, "msi-parent", &Len);
  if (MsiProp && (Len >= sizeof (fdt32_t))) {
    ImsicNode = fdt_node_offset_by_phandle (Fdt, fdt32_to_cpu (*MsiProp));
    if (ImsicNode < 0) {
      return FALSE;
    }

    IrqProp = (fdt32_t *)fdt_getprop (Fdt, ImsicNode, "interrupts-extended", &Len);
    if ((IrqProp > 0) && (Len >= 4) &&
        (fdt32_to_cpu (IrqProp[1]) == IRQ_S_EXT))
    {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  APLIC parser and update RINTC

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
AplicRintcInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN  OUT   CM_OBJ_DESCRIPTOR          *NewRintcCmObjDesc
  )
{
  CM_RISCV_RINTC_INFO  *RintcInfo;
  CM_RISCV_APLIC_INFO  AplicInfo;
  CONST UINT32         *IntExtProp;
  CONST UINT32         *PhandleProp;
  CONST UINT64         *Prop;
  EFI_STATUS           Status;
  UINT32               AplicGsiBase;
  INT32                Len;
  INT32                AplicId, Idx, Phandle;
  INT32                Prev, AplicNode;
  VOID                 *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Fdt          = FdtParserHandle->Fdt;
  AplicGsiBase = 0;
  AplicId      = 0;

  for (Prev = 0; ; Prev = AplicNode) {
    AplicNode = fdt_next_node (Fdt, Prev, NULL);
    if (AplicNode < 0) {
      return EFI_SUCCESS;
    }

    if (FdtNodeIsCompatible (Fdt, AplicNode, &AplicCompatibleInfo) &&
        IsSmodeAplic (Fdt, AplicNode))
    {
      ZeroMem (&AplicInfo, sizeof (CM_RISCV_APLIC_INFO));
      IntExtProp = fdt_getprop (Fdt, AplicNode, "interrupts-extended", &Len);
      if ((IntExtProp != 0) && ((Len / sizeof (UINT32)) % 2 == 0)) {
        Len               = Len / 4;
        AplicInfo.NumIdcs = Len / 2;
        for (Idx = 0; Idx < Len; Idx += 2) {
          Phandle   = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)IntExtProp + Idx));
          RintcInfo = RiscVFindRintc (NewRintcCmObjDesc, Phandle);
          if (RintcInfo == NULL) {
            ASSERT (0);
            return EFI_NOT_FOUND;
          }

          /* Update RINTC EXT INTC ID */
          RintcInfo->ExtIntCId = ACPI_BUILD_EXT_INTC_ID (AplicId, Idx / 2);
        }
      }

      Prop = fdt_getprop (Fdt, AplicNode, "reg", &Len);
      if ((Prop == NULL) || (Len % 4 > 0)) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      AplicInfo.AplicAddress = fdt64_to_cpu (ReadUnaligned64 ((const UINT64 *)Prop));
      AplicInfo.AplicSize    = fdt64_to_cpu (ReadUnaligned64 ((const UINT64 *)Prop + 1));
      Prop                   = fdt_getprop (Fdt, AplicNode, "riscv,num-sources", &Len);
      if (!Prop) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      AplicInfo.NumSources = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop));
      PhandleProp          = fdt_getprop (Fdt, AplicNode, "phandle", &Len);
      if (!PhandleProp) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      AplicInfo.Phandle = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)PhandleProp));
      AplicInfo.GsiBase = AplicGsiBase;
      AplicInfo.Version = 1;
      AplicInfo.AplicId = AplicId++;
      AplicGsiBase     += AplicInfo.NumSources;

      // Add the CmObj to the Configuration Manager.
      Status = AddSingleCmObj (
                 FdtParserHandle,
                 CREATE_CM_RISCV_OBJECT_ID (ERiscVObjAplicInfo),
                 &AplicInfo,
                 sizeof (CM_RISCV_APLIC_INFO),
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        return Status;
      }
    }
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
  INTN                 Idx, Idx1, Idx2, Limit;
  VOID                 *Fdt;

  if (NewRintcCmObjDesc == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;
  ZeroMem (&ImsicInfo, sizeof (CM_RISCV_IMSIC_INFO));

  for (Prev = 0; ; Prev = ImsicNode) {
    ImsicNode = fdt_next_node (Fdt, Prev, NULL);
    if (ImsicNode < 0) {
      return EFI_NOT_FOUND;
    }

    if (FdtNodeIsCompatible (Fdt, ImsicNode, &ImsicCompatibleInfo)) {
      IntExtProp = fdt_getprop (Fdt, ImsicNode, "interrupts-extended", &Len);
      if ((IntExtProp == 0) || ((Len / sizeof (UINT32)) % 2 != 0)) {
        /* interrupts-extended: <phandle>, <flag> */
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      /* There can be M-mode IMSIC in DT. Consider only S-mode */
      if (fdt32_to_cpu (IntExtProp[1]) == IRQ_S_EXT) {
        NumPhandle = (Len / sizeof (UINT32)) / 2;
        if (NumPhandle == 0) {
          ASSERT (0);
          return EFI_NOT_FOUND;
        }

        Prop = fdt_getprop (Fdt, ImsicNode, "riscv,num-ids", &Len);
        if (Prop == 0) {
          ASSERT (0);
          return EFI_INVALID_PARAMETER;
        }

        ImsicInfo.NumIds = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop));
        Prop             = fdt_getprop (Fdt, ImsicNode, "riscv,num-guest-ids", &Len);
        if (Prop == 0) {
          ImsicInfo.NumGuestIds = ImsicInfo.NumIds;
        } else {
          ImsicInfo.NumGuestIds = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop));
        }

        Prop = fdt_getprop (Fdt, ImsicNode, "riscv,guest-index-bits", &Len);
        if (Prop == 0) {
          ImsicInfo.GuestIndexBits = 0;
        } else {
          ImsicInfo.GuestIndexBits = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop));
        }

        Prop = fdt_getprop (Fdt, ImsicNode, "riscv,hart-index-bits", &Len);
        if (Prop == 0) {
          ImsicInfo.HartIndexBits = 0; // update default value later
        } else {
          ImsicInfo.HartIndexBits = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop));
        }

        Prop = fdt_getprop (Fdt, ImsicNode, "riscv,group-index-bits", &Len);
        if (Prop == 0) {
          ImsicInfo.GroupIndexBits = 0;
        } else {
          ImsicInfo.GroupIndexBits = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop));
        }

        Prop = fdt_getprop (Fdt, ImsicNode, "riscv,group-index-shift", &Len);
        if (Prop == 0) {
          ImsicInfo.GroupIndexShift = IMSIC_MMIO_PAGE_SHIFT * 2;
        } else {
          ImsicInfo.GroupIndexShift = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop));
        }

        ImsicInfo.Version   = 1;
        ImsicInfo.Reserved1 = 0;
        ImsicInfo.Flags     = 0;
        Prop                = fdt_getprop (Fdt, ImsicNode, "reg", &Len);
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
          ImsicBaseAddr = fdt64_to_cpu (ReadUnaligned64 ((const UINT64 *)ImsicRegProp + Idx * 2));
          ImsicBaseLen  = fdt64_to_cpu (ReadUnaligned64 ((const UINT64 *)ImsicRegProp + Idx * 2 + 1));
          // Calculate the limit of number of cpu nodes this imsic can handle
          Limit = ImsicBaseLen /  IMSIC_MMIO_PAGE_SZ;
          for (Idx1 = 0; Idx1 < Limit && Idx2 < NumPhandle; Idx1++, Idx2++) {
            Phandle         = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)IntExtProp + Idx2 * 2));
            RintcInfoBuffer = RiscVFindRintc (NewRintcCmObjDesc, Phandle);
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

  /* Search for APLIC presence and update RINTC structures if so */
  Status = AplicRintcInfoParser (FdtParserHandle, NewCmObjDesc);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    ASSERT_EFI_ERROR (Status);
    goto exit_handler;
  }

  /* Search for PLIC presence and update RINTC structures if so */
  Status = PlicRintcInfoParser (FdtParserHandle, NewCmObjDesc);
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
