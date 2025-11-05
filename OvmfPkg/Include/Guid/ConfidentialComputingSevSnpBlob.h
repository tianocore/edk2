/** @file
   UEFI Configuration Table for exposing the SEV-SNP launch blob.

   Copyright (c) 2021, Advanced Micro Devices Inc. All right reserved.

   SPDX-License-Identifier: BSD-2-Clause-Patent
 **/

#ifndef CONFIDENTIAL_COMPUTING_SEV_SNP_BLOB_H_
#define CONFIDENTIAL_COMPUTING_SEV_SNP_BLOB_H_

#include <Uefi/UefiBaseType.h>

#define CONFIDENTIAL_COMPUTING_SNP_BLOB_GUID            \
  { 0x067b1f5f,                                         \
    0xcf26,                                             \
    0x44c5,                                             \
    { 0x85, 0x54, 0x93, 0xd7, 0x77, 0x91, 0x2d, 0x42 }, \
  }

typedef PACKED struct {
  UINT32    Header;
  UINT16    Version;
  UINT16    Reserved;
  UINT64    SecretsPhysicalAddress;
  UINT32    SecretsSize;
  UINT32    Reserved1;
  UINT64    CpuidPhysicalAddress;
  UINT32    CpuidLSize;
  UINT32    Reserved2;
} CONFIDENTIAL_COMPUTING_SNP_BLOB_LOCATION;

extern EFI_GUID  gConfidentialComputingSevSnpBlobGuid;

#endif
