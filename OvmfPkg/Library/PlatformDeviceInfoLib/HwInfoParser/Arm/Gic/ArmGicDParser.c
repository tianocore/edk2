/** @file
  Arm Gic Distributor Parser.

  Copyright (c) 2021 - 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic.yaml
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic-v3.yaml
**/

#include "FdtUtility.h"
#include "FdtInfoParser.h"
#include "ArmGicDispatcher.h"
#include "ArmGicDParser.h"

/** Parse a Gic compatible interrupt-controller node,
    extracting GicD information.

  This parser is valid for Gic v2 and v3.

  @param [in]  FdtParserHandle  A handle to the parser instance.
  @param [in]  GicIntcNode      Offset of a Gic compatible
                                interrupt-controller node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
GicDIntcNodeParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN  INT32                            GicIntcNode
  )
{
  EFI_STATUS   Status;
  INT32        AddressCells;
  INT32        SizeCells;
  CONST UINT8  *Data;
  INT32        DataSize;
  UINT32       RegSize;
  UINT64       PhysicalBaseAddress;
  UINT64       PhysicalBaseAddressRange;
  VOID         *Fdt;
  CONST UINT8  *GicDValue;
  CONST UINT8  *GicDRange;

  if (FdtParserHandle == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;

  Status = FdtGetParentAddressInfo (
             Fdt,
             GicIntcNode,
             &AddressCells,
             &SizeCells
             );
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  // Don't support more than 64 bits and less than 32 bits addresses.
  if ((AddressCells < 1) ||
      (AddressCells > 2) ||
      (SizeCells < 1) ||
      (SizeCells > 2))
  {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  RegSize = (AddressCells + SizeCells) * sizeof (UINT32);

  Data = fdt_getprop (Fdt, GicIntcNode, "reg", &DataSize);
  if ((Data == NULL) ||
      (DataSize < 0) ||
      ((DataSize % RegSize) != 0))
  {
    // If error or wrong size.
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  GicDValue = Data + (sizeof (UINT32) *
                      GET_DT_REG_ADDRESS_OFFSET (0, AddressCells, SizeCells));
  GicDRange = Data + sizeof (UINT32) * AddressCells;

  if (AddressCells == 2) {
    PhysicalBaseAddress      = FdtReadUnaligned64 ((UINT64 *)GicDValue);
    PhysicalBaseAddressRange = FdtReadUnaligned64 ((UINT64 *)GicDRange);
  } else {
    PhysicalBaseAddress      = FdtReadUnaligned32 ((UINT32 *)GicDValue);
    PhysicalBaseAddressRange = FdtReadUnaligned32 ((UINT32 *)GicDRange);
  }

  if (FdtParserHandle->HwAddressInfo != NULL) {
    Status = FdtParserHandle->HwAddressInfo (
                                FdtParserHandle->Context,
                                "GICD",
                                PhysicalBaseAddress,
                                PhysicalBaseAddressRange
                                );
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

/** GICD information parser function.

  @param [in]  FdtParserHandle  A handle to the parser instance.
  @param [in]  FdtBranch        When searching for DT node name, restrict
                                the search to this Device Tree branch.
  @param [in]  GicVersion       The version of the GIC.

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
  IN        INT32                      FdtBranch,
  IN  CONST UINT32                     GicVersion
  )
{
  EFI_STATUS  Status;
  VOID        *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;

  if (!FdtNodeHasProperty (Fdt, FdtBranch, "interrupt-controller")) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  // Parse the interrupt-controller depending on its Gic version.
  switch (GicVersion) {
    case 2:
    case 3:
    {
      // Set the Gic version, then parse the GicD information.
      Status = GicDIntcNodeParser (FdtParserHandle, FdtBranch);
      break;
    }
    default:
    {
      // Unsupported Gic version.
      ASSERT (FALSE);
      return EFI_UNSUPPORTED;
    }
  }

  return Status;
}
