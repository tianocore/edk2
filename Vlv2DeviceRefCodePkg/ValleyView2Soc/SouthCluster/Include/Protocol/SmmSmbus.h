/*++

Copyright (c)  2009  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



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
