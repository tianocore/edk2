/** @file
 Define the structure for the Universal Payload SmBios.

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
    - Universal Payload Specification 0.75 (https://universalpayload.github.io/documentation/)
**/

#ifndef UNIVERSAL_PAYLOAD_SMBIOS_TABLE_H_
#define UNIVERSAL_PAYLOAD_SMBIOS_TABLE_H_

#include <Uefi.h>
#include <UniversalPayload/UniversalPayload.h>

#pragma pack (1)

typedef struct {
  UNIVERSAL_PAYLOAD_GENERIC_HEADER   Header;
  EFI_PHYSICAL_ADDRESS               SmBiosEntryPoint;
} UNIVERSAL_PAYLOAD_SMBIOS_TABLE;

#pragma pack()

#define UNIVERSAL_PAYLOAD_SMBIOS_TABLE_REVISION 1

extern GUID gUniversalPayloadSmbios3TableGuid;
extern GUID gUniversalPayloadSmbiosTableGuid;
#endif // UNIVERSAL_PAYLOAD_SMBIOS_TABLE_H_
