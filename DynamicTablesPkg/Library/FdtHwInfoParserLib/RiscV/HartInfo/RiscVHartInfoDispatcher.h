/** @file
  RISC-V Hart Info dispatcher.

  Copyright (c) 2024, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef RISCV_HART_INFO_DISPATCHER_H_
#define RISCV_HART_INFO_DISPATCHER_H_

#include <FdtHwInfoParserInclude.h>

#define RISCV_TIMER_CANNOT_WAKE_CPU  BIT0

/** RISC-V Hart Info dispatcher.

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
RiscVHartInfoDispatcher (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  );

#endif // RISCV_HART_INFO_DISPATCHER_H_
