/** @file
  RISC-V Flattened device tree utility for Interrupts.

  Copyright (c) 2024, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Device tree Specification - Release v0.3
**/

#include <Library/FdtLib.h>
#include <FdtHwInfoParserInclude.h>
#include "FdtUtility.h"

/** Get the interrupt Id of an interrupt described in a fdt.

  @param [in]  Data   Pointer to the first cell of an "interrupts" property.

  @retval  The interrupt id.
**/
UINT32
EFIAPI
FdtGetInterruptId (
  UINT32 CONST  *Data
  )
{
  ASSERT (Data != NULL);

  return Fdt32ToCpu (Data[RISCV_IRQ_NUMBER_OFFSET]);
}

/** Get the ACPI interrupt flags of an interrupt described in a fdt.

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

  IrqFlags = Fdt32ToCpu (Data[RISCV_IRQ_FLAGS_OFFSET]);

  AcpiIrqFlags  = DT_IRQ_IS_EDGE_TRIGGERED (IrqFlags) ? BIT0 : 0;
  AcpiIrqFlags |= DT_IRQ_IS_ACTIVE_LOW (IrqFlags) ? BIT1 : 0;

  return AcpiIrqFlags;
}

/** Get the Address cell info of the INTC node

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

  @retval EFI_SUCCESS           The function always returns SUCCESS

**/
EFI_STATUS
EFIAPI
FdtGetIntcAddressCells (
  IN  CONST VOID *Fdt,
  IN        INT32 Node,
  OUT       INT32 *AddressCells, OPTIONAL
  OUT       INT32  *SizeCells      OPTIONAL
  )
{
  /* Address Cells is zero for RISC-V */
  if (AddressCells != NULL) {
    *AddressCells = 0;
  }

  if (SizeCells != NULL) {
    *SizeCells = 0;
  }

  return EFI_SUCCESS;
}
