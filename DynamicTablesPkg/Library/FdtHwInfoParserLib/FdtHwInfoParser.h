/** @file
  Flattened Device Tree parser definitions.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef FDT_HW_INFO_PARSER_H_
#define FDT_HW_INFO_PARSER_H_

#include <FdtHwInfoParserInclude.h>

#include <ConfigurationManagerObject.h>
#include <Library/HwInfoParserLib.h>

#include "FdtUtility.h"

/** A structure describing the instance of the FdtHwInfoParser.
*/
typedef struct FdtHwInfoParser {
  /// Pointer to the HwDataSource i.e. the
  /// Flattened Device Tree (Fdt).
  VOID                  *Fdt;

  /// Pointer to the caller's context.
  VOID                  *Context;

  /// Function pointer called by the
  /// parser when adding information.
  HW_INFO_ADD_OBJECT    HwInfoAdd;
} FDT_HW_INFO_PARSER;

/** A pointer type for FDT_HW_INFO_PARSER.
*/
typedef FDT_HW_INFO_PARSER *FDT_HW_INFO_PARSER_HANDLE;

/** Function pointer to a parser function.

  A parser parses a Device Tree to populate a specific CmObj type. None,
  one or many CmObj can be created by the parser.
  The created CmObj are then handed to the parser's caller through the
  HW_INFO_ADD_OBJECT interface.
  This can also be a dispatcher. I.e. a function that not parsing a
  Device Tree but calling other parsers.

  @param [in]  ParserHandle    Handle to the parser instance.
  @param [in]  FdtBranch       When searching for DT node name, restrict
                               the search to this Device Tree branch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
typedef
EFI_STATUS
(EFIAPI *FDT_HW_INFO_PARSER_FUNC)(
  IN  CONST FDT_HW_INFO_PARSER_HANDLE ParserHandle,
  IN        INT32                     FdtBranch
  );

/** Main dispatcher: sequentially call the parsers/dispatchers
    of the HwInfoParserTable.

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
ArchFdtHwInfoMainDispatcher (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  );

#endif // FDT_HW_INFO_PARSER_H_
