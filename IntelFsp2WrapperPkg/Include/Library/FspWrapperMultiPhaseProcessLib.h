/** @file
  Provide FSP wrapper MultiPhase handling functions.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FSP_WRAPPER_MULTI_PHASE_PROCESS_LIB_H__
#define __FSP_WRAPPER_MULTI_PHASE_PROCESS_LIB_H__

/**
  FSP Wrapper Variable Request Handler

  @param[in] FspHobListPtr        - Pointer to FSP HobList (valid after FSP-M completed)
  @param[in] ComponentIndex       - FSP Component which executing MultiPhase initialization.

  @retval EFI_UNSUPPORTED   FSP Wrapper cannot support the specific variable request
  @retval EFI_STATUS        Return FSP returned status

**/EFI_STATUS
EFIAPI
FspWrapperVariableRequestHandler (
  IN OUT VOID  **FspHobListPtr,
  IN UINT8     ComponentIndex
  );

/**
  FSP Wrapper MultiPhase Handler

  @param[in] FspHobListPtr        - Pointer to FSP HobList (valid after FSP-M completed)
  @param[in] ComponentIndex       - FSP Component which executing MultiPhase initialization.

  @retval EFI_STATUS        Always return EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
FspWrapperMultiPhaseHandler (
  IN OUT VOID  **FspHobListPtr,
  IN UINT8     ComponentIndex
  );

#endif
