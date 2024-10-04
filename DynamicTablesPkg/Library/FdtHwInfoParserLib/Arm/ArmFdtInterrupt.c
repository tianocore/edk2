/** @file
  Flattened device tree utility.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Device tree Specification - Release v0.3
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm%2Cgic.yaml
  - linux//Documentation/devicetree/bindings/interrupt-controller/arm%2Cgic.yaml
**/

#include <FdtHwInfoParserInclude.h>
#include "FdtUtility.h"

/** Get the interrupt Id of an interrupt described in a fdt.

  Data must describe a GIC interrupt. A GIC interrupt is on at least
  3 UINT32 cells.
  This function DOES NOT SUPPORT extended SPI range and extended PPI range.

  @param [in]  Data   Pointer to the first cell of an "interrupts" property.

  @retval  The interrupt id.
**/
UINT32
EFIAPI
FdtGetInterruptId (
  UINT32 CONST  *Data
  )
{
  UINT32  IrqType;
  UINT32  IrqId;

  ASSERT (Data != NULL);

  IrqType = fdt32_to_cpu (Data[IRQ_TYPE_OFFSET]);
  IrqId   = fdt32_to_cpu (Data[IRQ_NUMBER_OFFSET]);

  switch (IrqType) {
    case DT_SPI_IRQ:
      IrqId += SPI_OFFSET;
      break;

    case DT_PPI_IRQ:
      IrqId += PPI_OFFSET;
      break;

    default:
      ASSERT (0);
      IrqId = 0;
  }

  return IrqId;
}

/** Get the ACPI interrupt flags of an interrupt described in a fdt.

  Data must describe a GIC interrupt. A GIC interrupt is on at least
  3 UINT32 cells.

  PPI interrupt cpu mask on bits [15:8] are ignored.

  @param [in]  Data   Pointer to the first cell of an "interrupts" property.

  @retval  The interrupt flags (for ACPI).
**/
UINT32
EFIAPI
FdtGetInterruptFlags (
  UINT32 CONST  *Data
  )
{
  UINT32  IrqFlags;
  UINT32  AcpiIrqFlags;

  ASSERT (Data != NULL);

  IrqFlags = fdt32_to_cpu (Data[IRQ_FLAGS_OFFSET]);

  AcpiIrqFlags  = DT_IRQ_IS_EDGE_TRIGGERED (IrqFlags) ? BIT0 : 0;
  AcpiIrqFlags |= DT_IRQ_IS_ACTIVE_LOW (IrqFlags) ? BIT1 : 0;

  return AcpiIrqFlags;
}

/** For relevant architectures, get the "#address-cells" and/or "#size-cells"
    property of the node.

  According to the Device Tree specification, s2.3.5 "#address-cells and
  #size-cells":
  "If missing, a client program should assume a default value of 2 for
  #address-cells, and a value of 1 for #size-cells."

  @param [in]  Fdt              Pointer to a Flattened Device Tree.
  @param [in]  Node             Offset of the node having to get the
                                "#address-cells" and "#size-cells"
                                properties from.
  @param [out] AddressCells     If success, number of address-cells.
                                If the property is not available,
                                default value is 2.
  @param [out] SizeCells        If success, number of size-cells.
                                If the property is not available,
                                default value is 1.

  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
FdtGetIntcAddressCells (
  IN  CONST VOID *Fdt,
  IN        INT32 Node,
  OUT       INT32 *AddressCells, OPTIONAL
  OUT       INT32     *SizeCells       OPTIONAL
  )
{
  return FdtGetAddressInfo (Fdt, Node, AddressCells, SizeCells);
}
