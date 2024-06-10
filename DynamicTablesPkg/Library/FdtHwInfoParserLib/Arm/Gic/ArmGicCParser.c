/** @file
  Arm Gic cpu parser.

  Copyright (c) 2021 - 2022, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/arm/cpus.yaml
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic.yaml
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic-v3.yaml
  - linux/Documentation/devicetree/bindings/arm/pmu.yaml
**/

#include <Library/ArmLib.h>
#include "FdtHwInfoParser.h"
#include "CmObjectDescUtility.h"
#include "Arm/Gic/ArmGicCParser.h"
#include "Arm/Gic/ArmGicDispatcher.h"

/** List of "compatible" property values for CPU nodes.

  Any other "compatible" value is not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  CpuCompatibleStr[] = {
  { "arm,arm-v7"     },
  { "arm,arm-v8"     },
  { "arm,armv8"      },
  { "arm,cortex-a15" },
  { "arm,cortex-a7"  },
  { "arm,cortex-a57" }
};

/** COMPATIBILITY_INFO structure for CPU nodes.
*/
STATIC CONST COMPATIBILITY_INFO  CpuCompatibleInfo = {
  ARRAY_SIZE (CpuCompatibleStr),
  CpuCompatibleStr
};

/** Pmu compatible strings.

  Any other "compatible" value is not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  PmuCompatibleStr[] = {
  { "arm,armv8-pmuv3" }
};

/** COMPATIBILITY_INFO structure for the PmuCompatibleStr.
*/
CONST COMPATIBILITY_INFO  PmuCompatibleInfo = {
  ARRAY_SIZE (PmuCompatibleStr),
  PmuCompatibleStr
};

/** Parse a "cpu" node.

  @param [in]  Fdt              Pointer to a Flattened Device Tree (Fdt).
  @param [in]  CpuNode          Offset of a cpu node.
  @param [in]  GicVersion       Version of the GIC.
  @param [in]  AddressCells     Number of address cells used for the reg
                                property.
  @param [out] GicCInfo         CM_ARM_GICC_INFO structure to populate.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
STATIC
EFI_STATUS
EFIAPI
CpuNodeParser (
  IN  CONST VOID              *Fdt,
  IN        INT32             CpuNode,
  IN        UINT32            GicVersion,
  IN        UINT32            AddressCells,
  OUT       CM_ARM_GICC_INFO  *GicCInfo
  )
{
  CONST UINT8  *Data;
  INT32        DataSize;
  UINT32       ProcUid;
  UINT64       MpIdr;
  UINT64       CheckAffMask;

  MpIdr        = 0;
  CheckAffMask = ARM_CORE_AFF0 | ARM_CORE_AFF1 | ARM_CORE_AFF2;

  if (GicCInfo == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Data = fdt_getprop (Fdt, CpuNode, "reg", &DataSize);
  if ((Data == NULL)                  ||
      ((DataSize != sizeof (UINT32))  &&
       (DataSize != sizeof (UINT64))))
  {
    ASSERT (0);
    return EFI_ABORTED;
  }

  /* If cpus node's #address-cells property is set to 2
     The first reg cell bits [7:0] must be set to
     bits [39:32] of MPIDR_EL1.
     The second reg cell bits [23:0] must be set to
     bits [23:0] of MPIDR_EL1.
   */
  if (AddressCells == 2) {
    MpIdr         = fdt64_to_cpu (*((UINT64 *)Data));
    CheckAffMask |= ARM_CORE_AFF3;
  } else {
    MpIdr = fdt32_to_cpu (*((UINT32 *)Data));
  }

  if ((MpIdr & ~CheckAffMask) != 0) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // To fit the Affinity [0-3] a 32bits value, place the Aff3 on bits
  // [31:24] instead of their original place ([39:32]).
  ProcUid = MpIdr | ((MpIdr & ARM_CORE_AFF3) >> 8);

  /* ACPI 6.3, s5.2.12.14 GIC CPU Interface (GICC) Structure:
     GIC 's CPU Interface Number. In GICv1/v2 implementations,
     this value matches the bit index of the associated processor
     in the GIC distributor's GICD_ITARGETSR register. For
     GICv3/4 implementations this field must be provided by the
     platform, if compatibility mode is supported. If it is not supported
     by the implementation, then this field must be zero.

     Note: We do not support compatibility mode for GicV3
  */
  if (GicVersion == 2) {
    GicCInfo->CPUInterfaceNumber = ProcUid;
  } else {
    GicCInfo->CPUInterfaceNumber = 0;
  }

  GicCInfo->AcpiProcessorUid = ProcUid;
  GicCInfo->Flags            = EFI_ACPI_6_3_GIC_ENABLED;
  GicCInfo->MPIDR            = MpIdr;

  return EFI_SUCCESS;
}

/** Parse a "cpus" node and its children "cpu" nodes.

  Create as many CM_ARM_GICC_INFO structures as "cpu" nodes.

  @param [in]  Fdt              Pointer to a Flattened Device Tree (Fdt).
  @param [in]  CpusNode         Offset of a cpus node.
  @param [in]  GicVersion       Version of the GIC.
  @param [out] NewGicCmObjDesc  If success, CM_OBJ_DESCRIPTOR containing
                                all the created CM_ARM_GICC_INFO.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
STATIC
EFI_STATUS
EFIAPI
CpusNodeParser (
  IN  CONST VOID               *Fdt,
  IN        INT32              CpusNode,
  IN        UINT32             GicVersion,
  OUT       CM_OBJ_DESCRIPTOR  **NewGicCmObjDesc
  )
{
  EFI_STATUS  Status;
  INT32       CpuNode;
  UINT32      CpuNodeCount;
  INT32       AddressCells;

  UINT32            Index;
  CM_ARM_GICC_INFO  *GicCInfoBuffer;
  UINT32            GicCInfoBufferSize;

  if (NewGicCmObjDesc == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

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

  // Allocate memory for CpuNodeCount CM_ARM_GICC_INFO structures.
  GicCInfoBufferSize = CpuNodeCount * sizeof (CM_ARM_GICC_INFO);
  GicCInfoBuffer     = AllocateZeroPool (GicCInfoBufferSize);
  if (GicCInfoBuffer == NULL) {
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
               Fdt,
               CpuNode,
               GicVersion,
               AddressCells,
               &GicCInfoBuffer[Index]
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      goto exit_handler;
    }
  } // for

  Status = CreateCmObjDesc (
             CREATE_CM_ARM_OBJECT_ID (EArmObjGicCInfo),
             CpuNodeCount,
             GicCInfoBuffer,
             GicCInfoBufferSize,
             NewGicCmObjDesc
             );
  ASSERT_EFI_ERROR (Status);

exit_handler:
  FreePool (GicCInfoBuffer);
  return Status;
}

/** Parse a Gic compatible interrupt-controller node,
    extracting GicC information generic to Gic v2 and v3.

  This function modifies a CM_OBJ_DESCRIPTOR object.
  The following CM_ARM_GICC_INFO fields are patched:
    - VGICMaintenanceInterrupt;
    - Flags;

  @param [in]       Fdt              Pointer to a Flattened Device Tree (Fdt).
  @param [in]       GicIntcNode      Offset of a Gic compatible
                                     interrupt-controller node.
  @param [in, out]  GicCCmObjDesc    The CM_ARM_GICC_INFO to patch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
GicCIntcNodeParser (
  IN      CONST VOID               *Fdt,
  IN            INT32              GicIntcNode,
  IN  OUT       CM_OBJ_DESCRIPTOR  *GicCCmObjDesc
  )
{
  EFI_STATUS        Status;
  INT32             IntCells;
  CM_ARM_GICC_INFO  *GicCInfo;

  CONST UINT8  *Data;
  INT32        DataSize;

  if (GicCCmObjDesc == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Get the number of cells used to encode an interrupt.
  Status = FdtGetInterruptCellsInfo (Fdt, GicIntcNode, &IntCells);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Get the GSIV maintenance interrupt.
  // According to the DT bindings, this could be the:
  // "Interrupt source of the parent interrupt controller on secondary GICs"
  // but it is assumed that only one Gic is available.
  Data = fdt_getprop (Fdt, GicIntcNode, "interrupts", &DataSize);
  if ((Data != NULL) && (DataSize == (IntCells * sizeof (UINT32)))) {
    GicCInfo                           = (CM_ARM_GICC_INFO *)GicCCmObjDesc->Data;
    GicCInfo->VGICMaintenanceInterrupt =
      FdtGetInterruptId ((CONST UINT32 *)Data);
    GicCInfo->Flags = DT_IRQ_IS_EDGE_TRIGGERED (
                        fdt32_to_cpu (((UINT32 *)Data)[IRQ_FLAGS_OFFSET])
                        ) ?
                      EFI_ACPI_6_3_VGIC_MAINTENANCE_INTERRUPT_MODE_FLAGS :
                      0;
    return Status;
  } else if (DataSize < 0) {
    // This property is optional and was not found. Just return.
    return Status;
  }

  // The property exists and its size doesn't match for one interrupt.
  ASSERT (0);
  return EFI_ABORTED;
}

/** Parse a Gic compatible interrupt-controller node,
    extracting GicCv2 information.

  This function modifies a CM_OBJ_DESCRIPTOR object.
  The following CM_ARM_GICC_INFO fields are patched:
    - PhysicalAddress;
    - GICH;
    - GICV;

  @param [in]       Fdt              Pointer to a Flattened Device Tree (Fdt).
  @param [in]       Gicv2IntcNode    Offset of a Gicv2 compatible
                                     interrupt-controller node.
  @param [in, out]  GicCCmObjDesc    The CM_ARM_GICC_INFO to patch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
GicCv2IntcNodeParser (
  IN      CONST VOID               *Fdt,
  IN            INT32              Gicv2IntcNode,
  IN  OUT       CM_OBJ_DESCRIPTOR  *GicCCmObjDesc
  )
{
  EFI_STATUS        Status;
  UINT32            Index;
  CM_ARM_GICC_INFO  *GicCInfo;
  INT32             AddressCells;
  INT32             SizeCells;

  CONST UINT8  *GicCValue;
  CONST UINT8  *GicVValue;
  CONST UINT8  *GicHValue;

  CONST UINT8  *Data;
  INT32        DataSize;
  UINT32       RegSize;
  UINT32       RegCount;

  if (GicCCmObjDesc == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  GicCInfo  = (CM_ARM_GICC_INFO *)GicCCmObjDesc->Data;
  GicVValue = NULL;
  GicHValue = NULL;

  // Get the #address-cells and #size-cells property values.
  Status = FdtGetParentAddressInfo (
             Fdt,
             Gicv2IntcNode,
             &AddressCells,
             &SizeCells
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Don't support more than 64 bits and less than 32 bits addresses.
  if ((AddressCells < 1)  ||
      (AddressCells > 2)  ||
      (SizeCells < 1)     ||
      (SizeCells > 2))
  {
    ASSERT (0);
    return EFI_ABORTED;
  }

  RegSize = (AddressCells + SizeCells) * sizeof (UINT32);

  Data = fdt_getprop (Fdt, Gicv2IntcNode, "reg", &DataSize);
  if ((Data == NULL)  ||
      (DataSize < 0)  ||
      ((DataSize % RegSize) != 0))
  {
    // If error or wrong size.
    ASSERT (0);
    return EFI_ABORTED;
  }

  RegCount = DataSize/RegSize;

  switch (RegCount) {
    case 4:
    {
      // GicV is at index 3 in the reg property. GicV is optional.
      GicVValue = Data + (sizeof (UINT32) *
                          GET_DT_REG_ADDRESS_OFFSET (3, AddressCells, SizeCells));
      // fall-through.
    }
    case 3:
    {
      // GicH is at index 2 in the reg property. GicH is optional.
      GicHValue = Data + (sizeof (UINT32) *
                          GET_DT_REG_ADDRESS_OFFSET (2, AddressCells, SizeCells));
      // fall-through.
    }
    case 2:
    {
      // GicC is at index 1 in the reg property. GicC is mandatory.
      GicCValue = Data + (sizeof (UINT32) *
                          GET_DT_REG_ADDRESS_OFFSET (1, AddressCells, SizeCells));
      break;
    }
    default:
    {
      // Not enough or too much information.
      ASSERT (0);
      return EFI_ABORTED;
    }
  }

  // Patch the relevant fields of the CM_ARM_GICC_INFO objects.
  for (Index = 0; Index < GicCCmObjDesc->Count; Index++) {
    if (AddressCells == 2) {
      GicCInfo[Index].PhysicalBaseAddress = fdt64_to_cpu (*(UINT64 *)GicCValue);
      GicCInfo[Index].GICH                = (GicHValue == NULL) ? 0 :
                                            fdt64_to_cpu (*(UINT64 *)GicHValue);
      GicCInfo[Index].GICV = (GicVValue == NULL) ? 0 :
                             fdt64_to_cpu (*(UINT64 *)GicVValue);
    } else {
      GicCInfo[Index].PhysicalBaseAddress = fdt32_to_cpu (*(UINT32 *)GicCValue);
      GicCInfo[Index].GICH                = (GicHValue == NULL) ? 0 :
                                            fdt32_to_cpu (*(UINT32 *)GicHValue);
      GicCInfo[Index].GICV = (GicVValue == NULL) ? 0 :
                             fdt32_to_cpu (*(UINT32 *)GicVValue);
    }
  } // for

  return EFI_SUCCESS;
}

/** Parse a Gic compatible interrupt-controller node,
    extracting GicCv3 information.

  This function modifies a CM_OBJ_DESCRIPTOR object.
  The following CM_ARM_GICC_INFO fields are patched:
    - PhysicalAddress;
    - GICH;
    - GICV;

  @param [in]       Fdt              Pointer to a Flattened Device Tree (Fdt).
  @param [in]       Gicv3IntcNode    Offset of a Gicv3 compatible
                                     interrupt-controller node.
  @param [in, out]  GicCCmObjDesc    The CM_ARM_GICC_INFO to patch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
GicCv3IntcNodeParser (
  IN      CONST VOID               *Fdt,
  IN            INT32              Gicv3IntcNode,
  IN  OUT       CM_OBJ_DESCRIPTOR  *GicCCmObjDesc
  )
{
  EFI_STATUS        Status;
  UINT32            Index;
  CM_ARM_GICC_INFO  *GicCInfo;
  INT32             AddressCells;
  INT32             SizeCells;
  UINT32            AdditionalRedistReg;

  CONST UINT8  *GicCValue;
  CONST UINT8  *GicVValue;
  CONST UINT8  *GicHValue;

  CONST UINT8  *Data;
  INT32        DataSize;
  UINT32       RegSize;
  UINT32       RegCount;

  if (GicCCmObjDesc == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  GicCInfo  = (CM_ARM_GICC_INFO *)GicCCmObjDesc->Data;
  GicCValue = NULL;
  GicVValue = NULL;
  GicHValue = NULL;

  // Get the #address-cells and #size-cells property values.
  Status = FdtGetParentAddressInfo (
             Fdt,
             Gicv3IntcNode,
             &AddressCells,
             &SizeCells
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Don't support more than 64 bits and less than 32 bits addresses.
  if ((AddressCells < 1)  ||
      (AddressCells > 2)  ||
      (SizeCells < 1)     ||
      (SizeCells > 2))
  {
    ASSERT (0);
    return EFI_ABORTED;
  }

  // The "#redistributor-regions" property is optional.
  Data = fdt_getprop (Fdt, Gicv3IntcNode, "#redistributor-regions", &DataSize);
  if ((Data != NULL) && (DataSize == sizeof (UINT32))) {
    ASSERT (fdt32_to_cpu (*(UINT32 *)Data) > 1);
    AdditionalRedistReg = fdt32_to_cpu (*(UINT32 *)Data) - 1;
  } else {
    AdditionalRedistReg = 0;
  }

  RegSize = (AddressCells + SizeCells) * sizeof (UINT32);

  /*
    Ref: linux/blob/master/Documentation/devicetree/bindings/
         interrupt-controller/arm%2Cgic-v3.yaml

    reg:
    description: |
      Specifies base physical address(s) and size of the GIC
      registers, in the following order:
      - GIC Distributor interface (GICD)
      - GIC Redistributors (GICR), one range per redistributor region
      - GIC CPU interface (GICC)
      - GIC Hypervisor interface (GICH)
      - GIC Virtual CPU interface (GICV)
      GICC, GICH and GICV are optional.
    minItems: 2
    maxItems: 4096
  */
  Data = fdt_getprop (Fdt, Gicv3IntcNode, "reg", &DataSize);
  if ((Data == NULL)  ||
      (DataSize < 0)  ||
      ((DataSize % RegSize) != 0))
  {
    // If error or wrong size.
    ASSERT (0);
    return EFI_ABORTED;
  }

  RegCount = (DataSize / RegSize) - AdditionalRedistReg;

  // The GicD and GicR info is mandatory.
  switch (RegCount) {
    case 5:
    {
      // GicV is at index 4 in the reg property. GicV is optional.
      GicVValue = Data + (sizeof (UINT32) *
                          GET_DT_REG_ADDRESS_OFFSET (
                            4 + AdditionalRedistReg,
                            AddressCells,
                            SizeCells
                            ));
      // fall-through.
    }
    case 4:
    {
      // GicH is at index 3 in the reg property. GicH is optional.
      GicHValue = Data + (sizeof (UINT32) *
                          GET_DT_REG_ADDRESS_OFFSET (
                            3 + AdditionalRedistReg,
                            AddressCells,
                            SizeCells
                            ));
      // fall-through.
    }
    case 3:
    {
      // GicC is at index 2 in the reg property. GicC is optional.
      // Even though GicC is optional, it is made mandatory in this parser.
      GicCValue = Data + (sizeof (UINT32) *
                          GET_DT_REG_ADDRESS_OFFSET (
                            2 + AdditionalRedistReg,
                            AddressCells,
                            SizeCells
                            ));
      // fall-through
    }
    case 2:
    {
      // GicR is discribed by the CM_ARM_GIC_REDIST_INFO object.
      // GicD is described by the CM_ARM_GICD_INFO object.
      break;
    }
    default:
    {
      // Not enough or too much information.
      ASSERT (0);
      return EFI_ABORTED;
    }
  }

  // Patch the relevant fields of the CM_ARM_GICC_INFO objects.
  if (AddressCells == 2) {
    for (Index = 0; Index < GicCCmObjDesc->Count; Index++) {
      // GicR is discribed by the CM_ARM_GIC_REDIST_INFO object.
      GicCInfo[Index].GICRBaseAddress     = 0;
      GicCInfo[Index].PhysicalBaseAddress = (GicCValue == NULL) ? 0 :
                                            fdt64_to_cpu (*(UINT64 *)GicCValue);
      GicCInfo[Index].GICH = (GicHValue == NULL) ? 0 :
                             fdt64_to_cpu (*(UINT64 *)GicHValue);
      GicCInfo[Index].GICV = (GicVValue == NULL) ? 0 :
                             fdt64_to_cpu (*(UINT64 *)GicVValue);
    }
  } else {
    for (Index = 0; Index < GicCCmObjDesc->Count; Index++) {
      // GicR is discribed by the CM_ARM_GIC_REDIST_INFO object.
      GicCInfo[Index].GICRBaseAddress     = 0;
      GicCInfo[Index].PhysicalBaseAddress = (GicCValue == NULL) ? 0 :
                                            fdt32_to_cpu (*(UINT32 *)GicCValue);
      GicCInfo[Index].GICH = (GicHValue == NULL) ? 0 :
                             fdt32_to_cpu (*(UINT32 *)GicHValue);
      GicCInfo[Index].GICV = (GicVValue == NULL) ? 0 :
                             fdt32_to_cpu (*(UINT32 *)GicVValue);
    }
  }

  return EFI_SUCCESS;
}

/** Parse a Pmu compatible node, extracting Pmu information.

  This function modifies a CM_OBJ_DESCRIPTOR object.
  The following CM_ARM_GICC_INFO fields are patched:
    - PerformanceInterruptGsiv;

  @param [in]       Fdt              Pointer to a Flattened Device Tree (Fdt).
  @param [in]       GicIntcNode      Offset of a Gic compatible
                                     interrupt-controller node.
  @param [in, out]  GicCCmObjDesc    The CM_ARM_GICC_INFO to patch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
GicCPmuNodeParser (
  IN      CONST VOID               *Fdt,
  IN            INT32              GicIntcNode,
  IN  OUT       CM_OBJ_DESCRIPTOR  *GicCCmObjDesc
  )
{
  EFI_STATUS        Status;
  INT32             IntCells;
  INT32             PmuNode;
  UINT32            PmuNodeCount;
  UINT32            PmuIrq;
  UINT32            Index;
  CM_ARM_GICC_INFO  *GicCInfo;
  CONST UINT8       *Data;
  INT32             DataSize;

  if (GicCCmObjDesc == NULL) {
    ASSERT (GicCCmObjDesc != NULL);
    return EFI_INVALID_PARAMETER;
  }

  GicCInfo = (CM_ARM_GICC_INFO *)GicCCmObjDesc->Data;
  PmuNode  = 0;

  // Count the number of pmu nodes.
  Status = FdtCountCompatNodeInBranch (
             Fdt,
             0,
             &PmuCompatibleInfo,
             &PmuNodeCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  if (PmuNodeCount == 0) {
    return EFI_NOT_FOUND;
  }

  Status = FdtGetNextCompatNodeInBranch (
             Fdt,
             0,
             &PmuCompatibleInfo,
             &PmuNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    if (Status == EFI_NOT_FOUND) {
      // Should have found the node.
      Status = EFI_ABORTED;
    }
  }

  // Get the number of cells used to encode an interrupt.
  Status = FdtGetInterruptCellsInfo (Fdt, GicIntcNode, &IntCells);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Data = fdt_getprop (Fdt, PmuNode, "interrupts", &DataSize);
  if ((Data == NULL) || (DataSize != (IntCells * sizeof (UINT32)))) {
    // If error or not 1 interrupt.
    ASSERT (Data != NULL);
    ASSERT (DataSize == (IntCells * sizeof (UINT32)));
    return EFI_ABORTED;
  }

  PmuIrq = FdtGetInterruptId ((CONST UINT32 *)Data);

  // Only supports PPI 23 for now.
  // According to BSA 1.0 s3.6 PPI assignments, PMU IRQ ID is 23. A non BSA
  // compliant system may assign a different IRQ for the PMU, however this
  // is not implemented for now.
  if (PmuIrq != BSA_PMU_IRQ) {
    ASSERT (PmuIrq == BSA_PMU_IRQ);
    return EFI_ABORTED;
  }

  for (Index = 0; Index < GicCCmObjDesc->Count; Index++) {
    GicCInfo[Index].PerformanceInterruptGsiv = PmuIrq;
  }

  return EFI_SUCCESS;
}

/** CM_ARM_GICC_INFO parser function.

  This parser expects FdtBranch to be the "\cpus" node node.
  At most one CmObj is created.
  The following structure is populated:
  typedef struct CmArmGicCInfo {
    UINT32  CPUInterfaceNumber;               // {Populated}
    UINT32  AcpiProcessorUid;                 // {Populated}
    UINT32  Flags;                            // {Populated}
    UINT32  ParkingProtocolVersion;           // {default = 0}
    UINT32  PerformanceInterruptGsiv;         // {Populated}
    UINT64  ParkedAddress;                    // {default = 0}
    UINT64  PhysicalBaseAddress;              // {Populated}
    UINT64  GICV;                             // {Populated}
    UINT64  GICH;                             // {Populated}
    UINT32  VGICMaintenanceInterrupt;         // {Populated}
    UINT64  GICRBaseAddress;                  // {default = 0}
    UINT64  MPIDR;                            // {Populated}
    UINT8   ProcessorPowerEfficiencyClass;    // {default = 0}
    UINT16  SpeOverflowInterrupt;             // {default = 0}
    UINT32  ProximityDomain;                  // {default = 0}
    UINT32  ClockDomain;                      // {default = 0}
    UINT32  AffinityFlags;                    // {default = 0}
  } CM_ARM_GICC_INFO;

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
ArmGicCInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  )
{
  EFI_STATUS         Status;
  INT32              IntcNode;
  UINT32             GicVersion;
  CM_OBJ_DESCRIPTOR  *NewCmObjDesc;
  VOID               *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Fdt          = FdtParserHandle->Fdt;
  NewCmObjDesc = NULL;

  // The FdtBranch points to the Cpus Node.
  // Get the interrupt-controller node associated to the "cpus" node.
  Status = FdtGetIntcParentNode (Fdt, FdtBranch, &IntcNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    if (Status == EFI_NOT_FOUND) {
      // Should have found the node.
      Status = EFI_ABORTED;
    }

    return Status;
  }

  Status = GetGicVersion (Fdt, IntcNode, &GicVersion);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Parse the "cpus" nodes and its children "cpu" nodes,
  // and create a CM_OBJ_DESCRIPTOR.
  Status = CpusNodeParser (Fdt, FdtBranch, GicVersion, &NewCmObjDesc);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Parse the interrupt-controller node according to the Gic version.
  switch (GicVersion) {
    case 2:
    {
      Status = GicCv2IntcNodeParser (Fdt, IntcNode, NewCmObjDesc);
      break;
    }
    case 3:
    {
      Status = GicCv3IntcNodeParser (Fdt, IntcNode, NewCmObjDesc);
      break;
    }
    default:
    {
      // Unsupported Gic version.
      ASSERT (0);
      Status = EFI_UNSUPPORTED;
    }
  }

  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  // Parse the Gic information common to Gic v2 and v3.
  Status = GicCIntcNodeParser (Fdt, IntcNode, NewCmObjDesc);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  // Parse the Pmu Interrupt.
  Status = GicCPmuNodeParser (Fdt, IntcNode, NewCmObjDesc);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    ASSERT_EFI_ERROR (Status);
    goto exit_handler;
  }

  // Add all the CmObjs to the Configuration Manager.
  Status = AddMultipleCmObj (FdtParserHandle, NewCmObjDesc, 0, NULL);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

exit_handler:
  FreeCmObjDesc (NewCmObjDesc);
  return Status;
}
