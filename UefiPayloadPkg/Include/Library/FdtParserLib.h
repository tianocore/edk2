/** @file
  This library will parse the FDT (flat device tree) table information.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef FDT_PARSER_LIB_H_
#define FDT_PARSER_LIB_H_

/**
  It will parse FDT based on DTB from bootloaders.

  @param[in]  FdtBase               Address of the Fdt data.

  @retval EFI_SUCCESS               If it completed successfully.
  @retval Others                    If it failed to parse DTB.
**/
UINTN
ParseDtb (
  IN VOID  *FdtBase
  );

/**
  It will Parse FDT -node based on information from bootloaders.
  @param[in]  FdtBase   The starting memory address of FdtBase
  @retval HobList   The base address of Hoblist.

**/
UINT64
FdtNodeParser (
  IN VOID  *Fdt
  );

/**
  It will Parse FDT -custom node based on information from bootloaders.
  @param[in]  FdtBase The starting memory address of FdtBase
  @param[in]  HostList The starting memory address of New Hob list.

**/
UINT64
CustomFdtNodeParser (
  IN VOID  *Fdt,
  IN VOID  *HostList
  );

/**
  It will initialize HOBs for UPL.

  @param[in]  FdtBase        Address of the Fdt data.

  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to initialize HOBs.
**/
UINT64
UplInitHob (
  IN VOID  *FdtBase
  );

#endif
