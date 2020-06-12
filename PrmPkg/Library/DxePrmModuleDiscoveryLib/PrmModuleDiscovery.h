/** @file

  Definitions internally used for Platform Runtime Mechanism (PRM) module discovery.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PRM_MODULE_DISCOVERY_H_
#define PRM_MODULE_DISCOVERY_H_

#include <PrmModuleImageContext.h>

#define PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY_SIGNATURE SIGNATURE_32('P','R','M','E')

#pragma pack(push, 1)

typedef struct {
  UINTN                                 Signature;
  LIST_ENTRY                            Link;
  PRM_MODULE_IMAGE_CONTEXT              Context;
} PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY;

#pragma pack(pop)

#endif
