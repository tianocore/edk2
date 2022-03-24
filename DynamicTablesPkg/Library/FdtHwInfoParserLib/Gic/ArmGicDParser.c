/** @file
  Arm Gic Distributor Parser.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic.yaml
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic-v3.yaml
**/

#include "CmObjectDescUtility.h"
#include "FdtHwInfoParser.h"
#include "Gic/ArmGicDispatcher.h"
#include "Gic/ArmGicDParser.h"

/** Parse a Gic compatible interrupt-controller node,
    extracting GicD information.

  This parser is valid for Gic v2 and v3.

  @param [in]  Fdt              Pointer to a Flattened Device Tree (Fdt).
  @param [in]  GicIntcNode      Offset of a Gic compatible
                                interrupt-controller node.
  @param [in]  GicDInfo         The CM_ARM_GICD_INFO to populate.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
GicDIntcNodeParser (
  IN  CONST VOID        *Fdt,
  IN  INT32             GicIntcNode,
  IN  CM_ARM_GICD_INFO  *GicDInfo
  )
{
  EFI_STATUS   Status;
  INT32        AddressCells;
  CONST UINT8  *Data;
  INT32        DataSize;

  if ((Fdt == NULL) ||
      (GicDInfo == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = FdtGetParentAddressInfo (Fdt, GicIntcNode, &AddressCells, NULL);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Don't support more than 64 bits and less than 32 bits addresses.
  if ((AddressCells < 1)  ||
      (AddressCells > 2))
  {
    ASSERT (0);
    return EFI_ABORTED;
  }

  Data = fdt_getprop (Fdt, GicIntcNode, "reg", &DataSize);
  if ((Data == NULL) || (DataSize < (INT32)(AddressCells * sizeof (UINT32)))) {
    // If error or not enough space.
    ASSERT (0);
    return EFI_ABORTED;
  }

  if (AddressCells == 2) {
    GicDInfo->PhysicalBaseAddress = fdt64_to_cpu (*(UINT64 *)Data);
  } else {
    GicDInfo->PhysicalBaseAddress = fdt32_to_cpu (*(UINT32 *)Data);
  }

  return Status;
}

/** CM_ARM_GICD_INFO parser function.

  This parser expects FdtBranch to be a Gic interrupt-controller node.
  At most one CmObj is created.
  The following structure is populated:
  typedef struct CmArmGicDInfo {
    UINT64  PhysicalBaseAddress;              // {Populated}
    UINT32  SystemVectorBase;
    UINT8   GicVersion;                       // {Populated}
  } CM_ARM_GICD_INFO;

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
ArmGicDInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  )
{
  EFI_STATUS        Status;
  UINT32            GicVersion;
  CM_ARM_GICD_INFO  GicDInfo;
  VOID              *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;

  if (!FdtNodeHasProperty (Fdt, FdtBranch, "interrupt-controller")) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Get the Gic version of the interrupt-controller.
  Status = GetGicVersion (Fdt, FdtBranch, &GicVersion);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  ZeroMem (&GicDInfo, sizeof (GicDInfo));
  GicDInfo.GicVersion = GicVersion;

  // Parse the interrupt-controller depending on its Gic version.
  switch (GicVersion) {
    case 2:
    case 3:
    {
      // Set the Gic version, then parse the GicD information.
      Status = GicDIntcNodeParser (Fdt, FdtBranch, &GicDInfo);
      break;
    }
    default:
    {
      // Unsupported Gic version.
      ASSERT (0);
      return EFI_UNSUPPORTED;
    }
  }

  // Add the CmObj to the Configuration Manager.
  Status = AddSingleCmObj (
             FdtParserHandle,
             CREATE_CM_ARM_OBJECT_ID (EArmObjGicDInfo),
             &GicDInfo,
             sizeof (CM_ARM_GICD_INFO),
             NULL
             );
  ASSERT_EFI_ERROR (Status);
  return Status;
}
