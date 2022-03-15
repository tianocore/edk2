/** @file

  Common definitions needed for Platform Runtime Mechanism (PRM) modules.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PRM_MODULE_H_
#define PRM_MODULE_H_

#include <Prm.h>
#include <PrmContextBuffer.h>
#include <PrmDataBuffer.h>
#include <PrmExportDescriptor.h>
#include <PrmMmio.h>

/**
  Macro that provides a condensed form of a PRM Handler.

  This macro can be used to define a PRM Handler that uses the standard PRM Handle
  signature. It is simply provided for convenience.

**/
#define PRM_HANDLER_EXPORT(Name)                                                  \
  STATIC_ASSERT (sizeof (PRM_STRING_(Name)) <= PRM_HANDLER_NAME_MAXIMUM_LENGTH, "The PRM handler exceeds the maximum allowed size of 128.");  \
                                                                                  \
  /**                                                                               \
    A Platform Runtime Mechanism (PRM) handler.                                     \
                                                                                    \
    @param[in]  ParameterBuffer     A pointer to the PRM handler parameter buffer   \
    @param[in]  ContextBUffer       A pointer to the PRM handler context buffer     \
                                                                                    \
    @retval EFI_STATUS              The PRM handler executed successfully.          \
    @retval Others                  An error occurred in the PRM handler.           \
                                                                                    \
  **/                                                                             \
  EFI_STATUS                                    \
  PRM_EXPORT_API                                \
  EFIAPI                                        \
  Name (                                        \
    IN VOID                 *ParameterBuffer,   \
    IN PRM_CONTEXT_BUFFER   *ContextBuffer      \
    )                                           \

#endif
