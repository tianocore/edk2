/** @file
  This library will Build the FDT (flat device tree) table information.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef BUILD_FDT_LIB_H_
#define BUILD_FDT_LIB_H_

/**
  It will build FDT for UPL consumed.
  @param[in] FdtBase         Address of the Fdt data.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required FDT.
**/
EFI_STATUS
BuildFdtForUPL (
  IN     VOID  *FdtBase
  );

#endif
