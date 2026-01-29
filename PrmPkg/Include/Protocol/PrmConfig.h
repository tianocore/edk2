/** @file

  PRM Configuration protocol

  PRM Configuration protocol is used by PRM module configuration libraries to
  describe their resources so that a generic PRM Configuration DXE driver can prepare those
  resources for OS runtime.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PRM_CONFIG_H_
#define PRM_CONFIG_H_

#include <PrmContextBuffer.h>
#include <Uefi.h>

typedef struct _PRM_CONFIG_PROTOCOL PRM_CONFIG_PROTOCOL;

#define PRM_CONFIG_PROTOCOL_SIGNATURE  SIGNATURE_32('P','M','C','P')
#define PRM_CONFIG_PROTOCOL_VERSION    1

struct _PRM_CONFIG_PROTOCOL {
  PRM_MODULE_CONTEXT_BUFFERS    ModuleContextBuffers;
};

extern EFI_GUID  gPrmConfigProtocolGuid;

#endif
