/** @file

  Common Platform Runtime Mechanism (PRM) definitions.

  Copyright (c) Microsoft Corporation
  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PRM_H_
#define PRM_H_

#include <Uefi.h>
#include <PrmContextBuffer.h>

#if defined (_MSC_VER)
#define PRM_EXPORT_API  __declspec(dllexport)
#elif defined (__GNUC__)
#define PRM_EXPORT_API  __attribute__ ((visibility ("default")))
#else
#define PRM_EXPORT_API
#endif

#define PRM_HANDLER_NAME_MAXIMUM_LENGTH  128

#define PRM_STRING_(x)  #x
#define PRM_STRING(x)   PRM_STRING_(x)

/**
  A Platform Runtime Mechanism (PRM) handler function.

  @param[in]  ParameterBuffer             A pointer to a buffer with arbitrary data that is allocated and populated
                                          by the PRM handler caller.
  @param[in]  ContextBuffer               A pointer to a buffer with arbitrary data that is allocated in the firmware
                                          boot environment.

  @retval EFI_STATUS              The PRM handler executed successfully.
  @retval Others                  An error occurred in the PRM handler.

**/
typedef
EFI_STATUS
(EFIAPI PRM_HANDLER)(
  IN VOID                 *ParameterBuffer  OPTIONAL,
  IN PRM_CONTEXT_BUFFER   *ContextBuffer  OPTIONAL
  );

#endif
