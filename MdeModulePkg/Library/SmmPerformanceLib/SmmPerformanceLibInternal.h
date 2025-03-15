/** @file
  Internal library header.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMM_CORE_PERFORMANCE_LIB_INTERNAL_H_
#define SMM_CORE_PERFORMANCE_LIB_INTERNAL_H_

/**
  Registers a callback to perform library actions needed at exit boot services.

  @param[in] ExitBootServicesProtocolGuid  The protocol GUID to register the callback for.

  @retval EFI_SUCCESS The callback was registered successfully.
  @retval Others      An error occurred registering the callback.
 **/
EFI_STATUS
RegisterExitBootServicesCallback (
  IN  CONST EFI_GUID  *ExitBootServicesProtocolGuid
  );

/**
  Unregisters a callback to perform library actions needed at exit boot services.

  @param[in] ExitBootServicesProtocolGuid  The protocol GUID to unregister the callback for.

  @retval EFI_SUCCESS The callback was unregistered successfully.
  @retval Others      An error occurred unregistering the callback.
 **/
EFI_STATUS
UnregisterExitBootServicesCallback (
  IN  CONST EFI_GUID  *ExitBootServicesProtocolGuid
  );

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
