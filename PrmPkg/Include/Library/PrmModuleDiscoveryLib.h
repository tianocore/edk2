/** @file

  The PRM Module Discovery library provides functionality to discover PRM modules installed by platform firmware.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PRM_MODULE_DISCOVERY_LIB_H_
#define PRM_MODULE_DISCOVERY_LIB_H_

#include <Base.h>
#include <PrmContextBuffer.h>
#include <PrmModuleImageContext.h>
#include <Uefi.h>

/**
  Gets the next PRM module discovered after the given PRM module.

  @param[in,out]  ModuleImageContext      A pointer to a pointer to a PRM module image context structure.

  @retval EFI_SUCCESS                     The next PRM module was found successfully.
  @retval EFI_INVALID_PARAMETER           The given ModuleImageContext structure is invalid or the pointer is NULL.
  @retval EFI_NOT_FOUND                   The next PRM module was not found.

**/
EFI_STATUS
EFIAPI
GetNextPrmModuleEntry (
  IN OUT  PRM_MODULE_IMAGE_CONTEXT  **ModuleImageContext
  );

/**
  Discovers all PRM Modules loaded during boot.

  Each PRM Module discovered is placed into a linked list so the list can br processsed in the future.

  @param[out]   ModuleCount               An optional pointer parameter that, if provided, is set to the number
                                          of PRM modules discovered.
  @param[out]   HandlerCount              An optional pointer parameter that, if provided, is set to the number
                                          of PRM handlers discovered.

  @retval EFI_SUCCESS                     All PRM Modules were discovered successfully.
  @retval EFI_INVALID_PARAMETER           An actual pointer parameter was passed as NULL.
  @retval EFI_NOT_FOUND                   The gEfiLoadedImageProtocolGuid protocol could not be found.
  @retval EFI_OUT_OF_RESOURCES            Insufficient memory resources to allocate the new PRM Context
                                          linked list nodes.
  @retval EFI_ALREADY_STARTED             The function was called previously and already discovered the PRM modules
                                          loaded on this boot.

**/
EFI_STATUS
EFIAPI
DiscoverPrmModules (
  OUT UINTN  *ModuleCount    OPTIONAL,
  OUT UINTN  *HandlerCount   OPTIONAL
  );

#endif
