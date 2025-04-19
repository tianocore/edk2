/** @file
  GUID for OVMF memory encryption related configurations.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef OVMF_MEMORY_ENCRYPTION_CONFIG_H_
#define OVMF_MEMORY_ENCRYPTION_CONFIG_H_

// {4B94CC94-AE50-434E-89BE-2A4989370B37}
#define OVMF_MEMORY_ENCRYPTION_CONFIG_GUID \
{ 0x4b94cc94, 0xae50, 0x434e, { 0x89, 0xbe, 0x2a, 0x49, 0x89, 0x37, 0xb, 0x37 } }

typedef struct {
  UINT64    ConfidentialComputingGuestAttr;   // SEV/TDX guest attributes
  UINT64    PteMemoryEncryptionAddressOrMask; // SEV/TDX memory encryption address or mask
} OVMF_MEMORY_ENCRYPTION_CONFIG;

extern EFI_GUID  gOvmfMemoryEncryptionConfigGuid;

#endif
