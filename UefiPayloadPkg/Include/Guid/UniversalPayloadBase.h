/** @file
  Universal Payload general definitions.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef UNIVERSAL_PAYLOAD_BASE_H_
#define UNIVERSAL_PAYLOAD_BASE_H_

extern GUID  gUniversalPayloadBaseGuid;

typedef struct {
  UNIVERSAL_PAYLOAD_GENERIC_HEADER    Header;
  EFI_PHYSICAL_ADDRESS                Entry;
} UNIVERSAL_PAYLOAD_BASE;

#define UNIVERSAL_PAYLOAD_BASE_REVISION  1

#define N_NON_RELOCATABLE         BIT31
#define P_PREFETCHABLE            BIT30
#define SS_CONFIGURATION_SPACE    0
#define SS_IO_SPACE               BIT24
#define SS_32BIT_MEMORY_SPACE     BIT25
#define SS_64BIT_MEMORY_SPACE     BIT24+BIT25
#define DWORDS_TO_NEXT_ADDR_TYPE  7

#endif // UNIVERSAL_PAYLOAD_BASE_H_
