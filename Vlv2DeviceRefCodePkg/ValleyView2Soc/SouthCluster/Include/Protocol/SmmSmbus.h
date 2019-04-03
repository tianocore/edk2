/*++

Copyright (c)  2009  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent



Module Name:

  SmmSmbus.h

Abstract:

  SmmSmbus Protocol

--*/
#ifndef __EFI_SMM_SMBUS_PROTOCOL_H__
#define __EFI_SMM_SMBUS_PROTOCOL_H__

//
// GUID for the SmmSmbus Protocol
//
// EDK and EDKII have different GUID formats
//

#define EFI_SMM_SMBUS_PROTOCOL_GUID  \
  { \
    0x72e40094, 0x2ee1, 0x497a, 0x8f, 0x33, 0x4c, 0x93, 0x4a, 0x9e, 0x9c, 0xc \
  }

//
// Resuse the DXE definition, and use another GUID.
//
typedef EFI_SMBUS_HC_PROTOCOL  SMM_SMBUS_HC_PROTOCOL;

extern EFI_GUID  gEfiSmmSmbusProtocolGuid;

#endif
