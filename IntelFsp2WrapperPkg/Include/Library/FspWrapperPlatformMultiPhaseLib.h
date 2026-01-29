/** @file
  Provide FSP wrapper Platform MultiPhase handling functions.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FSP_WRAPPER_PLATFORM_MULTI_PHASE_LIB_H_
#define FSP_WRAPPER_PLATFORM_MULTI_PHASE_LIB_H_

/**
  FSP Wrapper Platform MultiPhase Handler

  @param[in] FspHobListPtr        - Pointer to FSP HobList (valid after FSP-M completed)
  @param[in] ComponentIndex       - FSP Component which executing MultiPhase initialization.
  @param[in] PhaseIndex           - Indicates current execution phase of FSP MultiPhase initialization.

  @retval EFI_STATUS        Always return EFI_SUCCESS

**/
VOID
EFIAPI
FspWrapperPlatformMultiPhaseHandler (
  IN OUT VOID  **FspHobListPtr,
  IN UINT8     ComponentIndex,
  IN UINT32    PhaseIndex
  );

#endif //FSP_WRAPPER_PLATFORM_MULTI_PHASE_LIB_H_
