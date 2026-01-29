/** @file
  Contains function prototypes for Memory Profile Services in DxeCore.

  This header file borrows the DxeCore Memory Profile services as the primitive
  for memory profile.

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _DXE_CORE_MEMORY_PROFILE_SERVICES_H_
#define _DXE_CORE_MEMORY_PROFILE_SERVICES_H_

/**
  Update memory profile information.

  @param CallerAddress  Address of caller who call Allocate or Free.
  @param Action         This Allocate or Free action.
  @param MemoryType     Memory type.
                        EfiMaxMemoryType means the MemoryType is unknown.
  @param Size           Buffer size.
  @param Buffer         Buffer address.
  @param ActionString   String for memory profile action.
                        Only needed for user defined allocate action.

  @return EFI_SUCCESS           Memory profile is updated.
  @return EFI_UNSUPPORTED       Memory profile is unsupported,
                                or memory profile for the image is not required,
                                or memory profile for the memory type is not required.
  @return EFI_ACCESS_DENIED     It is during memory profile data getting.
  @return EFI_ABORTED           Memory profile recording is not enabled.
  @return EFI_OUT_OF_RESOURCES  No enough resource to update memory profile for allocate action.
  @return EFI_NOT_FOUND         No matched allocate info found for free action.

**/
EFI_STATUS
EFIAPI
CoreUpdateProfile (
  IN PHYSICAL_ADDRESS       CallerAddress,
  IN MEMORY_PROFILE_ACTION  Action,
  IN EFI_MEMORY_TYPE        MemoryType,
  IN UINTN                  Size,       // Valid for AllocatePages/FreePages/AllocatePool
  IN VOID                   *Buffer,
  IN CHAR8                  *ActionString OPTIONAL
  );

#endif
