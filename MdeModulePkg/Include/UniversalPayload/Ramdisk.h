/** @file
 Define the structure for the Universal Payload Ramdisk.

Copyright (c) 2024, Rivos, Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
    - Universal Payload Specification 0.75 (https://universalpayload.github.io/documentation/)
**/

#ifndef UNIVERSAL_PAYLOAD_RAMDISK_H_
#define UNIVERSAL_PAYLOAD_RAMDISK_H_

#include <Uefi.h>
#include <UniversalPayload/UniversalPayload.h>

#pragma pack (1)

typedef struct {
  UNIVERSAL_PAYLOAD_GENERIC_HEADER    Header;
  EFI_PHYSICAL_ADDRESS                RamdiskBase;
  UINTN                               RamdiskSize;
} UNIVERSAL_PAYLOAD_RAMDISK;

#pragma pack()

#define UNIVERSAL_PAYLOAD_RAMDISK_REVISION  1

extern GUID  gUniversalPayloadRamdiskGuid;
#endif // UNIVERSAL_PAYLOAD_RAMDISK_H_
