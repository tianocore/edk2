/** @file
  The library to print all the HOBs.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef HOB_PRINT_LIB_H_
#define HOB_PRINT_LIB_H_

/**
  HOB Print Handler to print HOB information.

  @param[in] HobStart        A pointer to the HOB of type EFI_HOB_TYPE_GUID_EXTENSION.
  @param[in] HobLength       The length in bytes of the HOB of type EFI_HOB_TYPE_GUID_EXTENSION.

  @retval EFI_SUCCESS        If it completed successfully.
  @retval EFI_UNSUPPORTED    If the HOB type is not supported.

**/
typedef
EFI_STATUS
(*HOB_PRINT_HANDLER)(
  IN  VOID    *Hob,
  IN  UINT16  HobLength
  );

/**
  Print all HOBs info from the HOB list.
  If the input PrintHandler is not NULL, the PrintHandler will be processed first.
  If PrintHandler returns EFI_SUCCESS, default HOB info print logic in PrintHobList
  will be skipped.

  @param[in] HobStart       A pointer to the HOB list.
  @param[in] PrintHandler   A custom handler to print HOB info.

**/
VOID
EFIAPI
PrintHobList (
  IN CONST VOID         *HobStart,
  IN HOB_PRINT_HANDLER  PrintHandler OPTIONAL
  );

#endif
