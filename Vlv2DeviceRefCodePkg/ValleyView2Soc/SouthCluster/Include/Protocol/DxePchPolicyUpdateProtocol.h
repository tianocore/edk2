/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent



Module Name:

  DxePchPolicyUpdateProtocol.h

Abstract:

  PCH policy update protocol. This protocol is consumed by the PchDxePolicyInit driver

--*/
#ifndef _DXE_PCH_POLICY_UPDATE_PROTOCOL_H_
#define _DXE_PCH_POLICY_UPDATE_PROTOCOL_H_

#include "PchRegs.h"


#ifdef ECP_FLAG
#define DXE_PCH_POLICY_UPDATE_PROTOCOL_GUID \
  { \
    0x1a819e49, 0xd8ee, 0x48cb, 0x9a, 0x9c, 0xa, 0xa0, 0xd2, 0x81, 0xa, 0x38 \
  }
#else
#define DXE_PCH_POLICY_UPDATE_PROTOCOL_GUID \
  { \
    0x1a819e49, 0xd8ee, 0x48cb, \
    { \
        0x9a, 0x9c, 0xa, 0xa0, 0xd2, 0x81, 0xa, 0x38 \
    } \
  }
#endif

extern EFI_GUID                                   gDxePchPolicyUpdateProtocolGuid;
#define DXE_PCH_POLICY_UPDATE_PROTOCOL_REVISION_1 1

//
// ------------ General PCH policy Update protocol definition ------------
//
struct _DXE_PCH_POLICY_UPDATE_PROTOCOL {
  UINT8                   Revision;
};

typedef struct _DXE_PCH_POLICY_UPDATE_PROTOCOL  DXE_PCH_POLICY_UPDATE_PROTOCOL;

#endif
