/** @file

  The PRM Buffer Context library provides a general abstraction for context buffer management.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PRM_CONTEXT_BUFFER_LIB_H_
#define PRM_CONTEXT_BUFFER_LIB_H_

#include <Base.h>
#include <PrmContextBuffer.h>
#include <Uefi.h>

typedef enum {
  ///
  /// Search by the PRM module GUID
  ///
  ByModuleGuid,
  ///
  /// Search by the PRM handler GUID
  ///
  ByHandlerGuid
} PRM_GUID_SEARCH_TYPE;

/**
  Finds a PRM context buffer for the given PRM handler GUID.

  Note: PRM_MODULE_CONTEXT_BUFFERS is at the PRM module level while PRM_CONTEXT_BUFFER is at the PRM handler level.

  @param[in]  HandlerGuid                 A pointer to the PRM handler GUID.
  @param[in]  ModuleContextBuffers        A pointer to the PRM context buffers structure for the PRM module.
  @param[out] PrmModuleContextBuffer      A pointer to a pointer that will be set to the PRM context buffer
                                          if successfully found.

  @retval EFI_SUCCESS                     The PRM context buffer was found.
  @retval EFI_INVALID_PARAMETER           A required parameter pointer is NULL.
  @retval EFI_NOT_FOUND                   The context buffer for the given PRM handler GUID could not be found.

**/
EFI_STATUS
FindContextBufferInModuleBuffers (
  IN  CONST EFI_GUID                    *HandlerGuid,
  IN  CONST PRM_MODULE_CONTEXT_BUFFERS  *ModuleContextBuffers,
  OUT CONST PRM_CONTEXT_BUFFER          **ContextBuffer
  );

/**
  Returns a PRM context buffers structure for the given PRM search type.

  This function allows a caller to get the context buffers structure for a PRM module with either the PRM module
  GUID or the GUID for a PRM handler in the module.

  Note: PRM_MODULE_CONTEXT_BUFFERS is at the PRM module level while PRM_CONTEXT_BUFFER is at the PRM handler level.

  @param[in]  GuidSearchType              The type of GUID passed in the Guid argument.
  @param[in]  Guid                        A pointer to the GUID of a PRM module or PRM handler. The actual GUID type
                                          will be interpreted based on the value passed in GuidSearchType.
  @param[out] PrmModuleContextBuffers     A pointer to a pointer that will be set to the PRM context buffers
                                          structure if successfully found.

  @retval EFI_SUCCESS                     The PRM context buffers structure was found.
  @retval EFI_INVALID_PARAMETER           A required parameter pointer is NULL.
  @retval EFI_NOT_FOUND                   The context buffers for the given GUID could not be found.

**/
EFI_STATUS
GetModuleContextBuffers (
  IN  PRM_GUID_SEARCH_TYPE              GuidSearchType,
  IN  CONST EFI_GUID                    *Guid,
  OUT CONST PRM_MODULE_CONTEXT_BUFFERS  **PrmModuleContextBuffers
  );

/**
  Returns a PRM context buffer for the given PRM handler.

  @param[in]  PrmHandlerGuid              A pointer to the GUID for the PRM handler.
  @param[in]  PrmModuleContextBuffers     A pointer to a PRM_MODULE_CONTEXT_BUFFERS structure. If this optional
                                          parameter is provided, the handler context buffer will be searched for in this
                                          buffer structure which saves time by not performing a global search for the
                                          module buffer structure.
  @param[out] PrmContextBuffer            A pointer to a pointer that will be set to the PRM context buffer
                                          if successfully found.

  @retval EFI_SUCCESS                     The PRM context buffer was found.
  @retval EFI_INVALID_PARAMETER           A required parameter pointer is NULL.
  @retval EFI_NOT_FOUND                   The context buffer for the PRM handler could not be found.

**/
EFI_STATUS
GetContextBuffer (
  IN  CONST EFI_GUID                    *PrmHandlerGuid,
  IN  CONST PRM_MODULE_CONTEXT_BUFFERS  *PrmModuleContextBuffers  OPTIONAL,
  OUT CONST PRM_CONTEXT_BUFFER          **PrmContextBuffer
  );

#endif
