/** @file
  A Flattened Device Tree parser that scans the platform devices to
  retrieve the base address and the range.

  Copyright (c) 2021 - 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef FDT_INFO_PARSER_H_
#define FDT_INFO_PARSER_H_

/** Function pointer called by the parser to add information.

  @param  [in]  Context   A pointer to the caller's context.
  @param  [in]  Desc      An optional NULL terminated string
                          describing the hardware resource.
  @param  [in]  Base      Base address.
  @param  [in]  Range     Address range.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Maximum platform device count exceeded.
**/
typedef
EFI_STATUS
(EFIAPI *HW_ADDRESS_INFO)(
  IN        VOID                  *Context,
  IN  CONST CHAR8                 *Desc,
  IN        UINT64                Base,
  IN        UINT64                Range
  );

/** A structure describing the instance of the FdtHwInfoParser.
*/
typedef struct FdtHwInfoParser {
  /// Pointer to the HwDataSource i.e. the
  /// Flattened Device Tree (Fdt).
  VOID               *Fdt;

  /// Pointer to the caller's context.
  VOID               *Context;

  /// Callback function to notify information
  /// about platform devices found in the FDT.
  HW_ADDRESS_INFO    HwAddressInfo;
} FDT_HW_INFO_PARSER;

/** A pointer type for FDT_HW_INFO_PARSER.
*/
typedef FDT_HW_INFO_PARSER *FDT_HW_INFO_PARSER_HANDLE;

#endif // FDT_INFO_PARSER_H_
