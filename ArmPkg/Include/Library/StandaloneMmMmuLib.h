/** @file

  Copyright (c) 2018, ARM Ltd. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

/**
  Set XN bit preserving other permission.

  @param [in]  BaseAddress     Base address for the memory region.
  @param [in]  Length          Length of the memory region.

  @retval EFI_SUCCESS             Request successfull.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_READY           Callee is busy or not in a state to handle
                                  this request.
  @retval EFI_UNSUPPORTED         This function is not implemented by the
                                  callee.
  @retval EFI_ABORTED             Message target ran into an unexpected error
                                  and has aborted.
  @retval EFI_ACCESS_DENIED       Access denied.
  @retval EFI_OUT_OF_RESOURCES    Out of memory to perform operation.

**/
EFI_STATUS
ArmSetMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

/**
  Clear XN bit preserving other permission.

  @param [in]  BaseAddress     Base address for the memory region.
  @param [in]  Length          Length of the memory region.

  @retval EFI_SUCCESS             Request successfull.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_READY           Callee is busy or not in a state to handle
                                  this request.
  @retval EFI_UNSUPPORTED         This function is not implemented by the
                                  callee.
  @retval EFI_ABORTED             Message target ran into an unexpected error
                                  and has aborted.
  @retval EFI_ACCESS_DENIED       Access denied.
  @retval EFI_OUT_OF_RESOURCES    Out of memory to perform operation.

**/
EFI_STATUS
ArmClearMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

/**
  Change memory permission as RO ignoring former permission.

  @param [in]  BaseAddress     Base address for the memory region.
  @param [in]  Length          Length of the memory region.

  @retval EFI_SUCCESS             Request successfull.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_READY           Callee is busy or not in a state to handle
                                  this request.
  @retval EFI_UNSUPPORTED         This function is not implemented by the
                                  callee.
  @retval EFI_ABORTED             Message target ran into an unexpected error
                                  and has aborted.
  @retval EFI_ACCESS_DENIED       Access denied.
  @retval EFI_OUT_OF_RESOURCES    Out of memory to perform operation.

**/
EFI_STATUS
ArmSetMemoryRegionReadOnlyPerm (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

/**
  Change memory permission as RW ignoring former permission.

  @param [in]  BaseAddress     Base address for the memory region.
  @param [in]  Length          Length of the memory region.

  @retval EFI_SUCCESS             Request successfull.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_READY           Callee is busy or not in a state to handle
                                  this request.
  @retval EFI_UNSUPPORTED         This function is not implemented by the
                                  callee.
  @retval EFI_ABORTED             Message target ran into an unexpected error
                                  and has aborted.
  @retval EFI_ACCESS_DENIED       Access denied.
  @retval EFI_OUT_OF_RESOURCES    Out of memory to perform operation.

**/
EFI_STATUS
ArmSetMemoryRegionReadWritePerm (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

/**
  Change memory permission as ROX ignoring former permission.

  @param [in]  BaseAddress     Base address for the memory region.
  @param [in]  Length          Length of the memory region.

  @retval EFI_SUCCESS             Request successfull.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_READY           Callee is busy or not in a state to handle
                                  this request.
  @retval EFI_UNSUPPORTED         This function is not implemented by the
                                  callee.
  @retval EFI_ABORTED             Message target ran into an unexpected error
                                  and has aborted.
  @retval EFI_ACCESS_DENIED       Access denied.
  @retval EFI_OUT_OF_RESOURCES    Out of memory to perform operation.

**/
EFI_STATUS
ArmSetMemoryRegionReadOnlyExecPerm (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );
