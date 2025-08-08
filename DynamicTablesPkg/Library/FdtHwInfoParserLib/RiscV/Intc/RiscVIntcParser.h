/** @file
  RISC-V RINTC and IMSIC parser.

  Copyright (c) 2024, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
**/

#ifndef RISCV_RINTC_PARSER_H_
#define RISCV_RINTC_PARSER_H_

#include <FdtHwInfoParserInclude.h>

#define IMSIC_MMIO_PAGE_SHIFT  12
#define IMSIC_MMIO_PAGE_SZ     (1 << IMSIC_MMIO_PAGE_SHIFT)
#define IRQ_S_EXT              9

/** RISC-V MADT dispatcher.


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
  );

typedef struct {
  UINT32        Signature;
  LIST_ENTRY    Link;
  INT32         ExtIntcNode;
  UINT32        GsiBase;
} RISCV_EXT_INTC_DATA;

//
// Unique signature for private data structure.
//
#define RISCV_EXT_INTC_DATA_SIGNATURE  SIGNATURE_32 ('R','S','I','C')

#define RISCV_EXT_INTC_INFO_FROM_THIS(a)  \
  CR (a,                                       \
      RISCV_EXT_INTC_DATA,                \
      Link,                                    \
      RISCV_EXT_INTC_DATA_SIGNATURE       \
      );

#endif // RISCV_RINTC_PARSER_H_
