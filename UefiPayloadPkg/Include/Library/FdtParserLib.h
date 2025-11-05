/** @file
  This library will parse the FDT (flat device tree) table information.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef FDT_PARSER_LIB_H_
#define FDT_PARSER_LIB_H_

/**
  It will parse FDT based on DTB.

  @param[in]  FdtBase               Address of the Fdt data.

  @retval EFI_SUCCESS               If it completed successfully.
  @retval Others                    If it failed to parse DTB.
**/
UINTN
EFIAPI
ParseDtb (
  IN VOID  *FdtBase
  );

/**
  It will Parse FDT -node based on information.
  @param[in]  FdtBase   The starting memory address of FdtBase
  @retval HobList   The base address of Hoblist.

**/
UINT64
EFIAPI
FdtNodeParser (
  IN VOID  *FdtBase
  );

/**
  It will Parse FDT -custom node based on information.
  @param[in]  FdtBase The starting memory address of FdtBase
  @param[in]  HostList The starting memory address of New Hob list.

**/
UINTN
EFIAPI
CustomFdtNodeParser (
  IN VOID  *FdtBase,
  IN VOID  *HostList
  );

/**
  It will initialize HOBs for UPL.

  @param[in]  FdtBase        Address of the Fdt data.

  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to initialize HOBs.
**/
UINTN
EFIAPI
UplInitHob (
  IN VOID  *FdtBase
  );

#endif
