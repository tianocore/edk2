/** @file
 Define the structure for the Universal Payload APCI table.

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
    - Universal Payload Specification 0.75 (https://universalpayload.github.io/documentation/)
**/

#pragma once

#include <Uefi.h>
#include <UniversalPayload/UniversalPayload.h>

#pragma pack(1)

typedef struct {
  UNIVERSAL_PAYLOAD_GENERIC_HEADER    Header;
  EFI_PHYSICAL_ADDRESS                Rsdp;
} UNIVERSAL_PAYLOAD_ACPI_TABLE;

#pragma pack()

#define UNIVERSAL_PAYLOAD_ACPI_TABLE_REVISION  1

extern GUID  gUniversalPayloadAcpiTableGuid;
