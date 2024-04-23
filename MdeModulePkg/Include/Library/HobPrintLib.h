/** @file
  The library to print all the HOBs.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef HOB_PRINT_LIB_H_
#define HOB_PRINT_LIB_H_

typedef
EFI_STATUS
(*GUID_HOB_PRINT) (
  IN  UINT8   *HobRaw,
  IN  UINT16  HobLength
  );

typedef struct {
  EFI_GUID          *Guid;
  GUID_HOB_PRINT    PrintHandler;
  CHAR8             *GuidName;
} GUID_HOB_PRINT_HANDLE;

/**
  Register Guid HOB print handlers.

  @param[in] Handlers           A pointer to the list of Guid Hob print handler
  @param[in] NumberOfHandlers   Numbers of Guid Hob print handler

  @return EFI_SUCCESS           The handlers were registered.
  @return EFI_OUT_OF_RESOURCES  There are not enough resources available to register the handlers.

**/
EFI_STATUS
EFIAPI
RegisterGuidHobPrintHandlers (
  IN GUID_HOB_PRINT_HANDLE  *Handlers,
  IN UINTN                  NumberOfHandlers
  );

/**
  Print all HOBs info from the HOB list.

  @param[in] HobStart A pointer to the HOB list
**/
VOID
EFIAPI
PrintHobs (
  IN CONST VOID  *HobStart
  );

#endif
