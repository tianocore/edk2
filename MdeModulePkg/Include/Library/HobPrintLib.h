/** @file
  The library to print all the HOBs.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef HOB_PRINT_LIB_H_
#define HOB_PRINT_LIB_H_

typedef
EFI_STATUS
(*HOB_PRINT_HANDLER)(
  IN  VOID    *Hob,
  IN  UINT16  HobLength
  );

/**
  Print all HOBs info from the HOB list.

  @param[in] HobStart       A pointer to the HOB list.
  @param[in] PrintHandler   A custom handler to print HOB info.

**/
VOID
EFIAPI
PrintHobList (
  IN CONST VOID         *HobStart,
  IN HOB_PRINT_HANDLER  PrintHandler
  );

#endif
