/** @file
  Internal library header.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMM_CORE_PERFORMANCE_LIB_INTERNAL_H_
#define SMM_CORE_PERFORMANCE_LIB_INTERNAL_H_

/**
  This is the Event call back function is triggered in MM to notify the Library
  the system is entering runtime phase.

  @param[in] Protocol   Points to the protocol's unique identifier
  @param[in] Interface  Points to the interface instance
  @param[in] Handle     The handle on which the interface was installed

  @retval EFI_SUCCESS SmmAtRuntimeCallBack runs successfully
 **/
EFI_STATUS
EFIAPI
SmmPerformanceLibExitBootServicesCallback (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  );

#endif // SMM_CORE_PERFORMANCE_LIB_INTERNAL_H_
