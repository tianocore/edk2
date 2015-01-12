/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



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
