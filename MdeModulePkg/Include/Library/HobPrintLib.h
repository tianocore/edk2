/** @file
  Hob Print Library

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef HOB_PRINT_LIB_H_
#define HOB_PRINT_LIB_H_

/**
  Print all HOBs info from the HOB list.
  @param[in] HobStart A pointer to the HOB list
  @return    The pointer to the HOB list.
**/
VOID
PrintHob (
  IN CONST VOID  *HobStart
  );

#endif
