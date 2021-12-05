/** @file
PPI to describe stored hash digest for FVs.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PEI_FIRMWARE_VOLUME_INFO_STORED_HASH_FV_H__
#define __PEI_FIRMWARE_VOLUME_INFO_STORED_HASH_FV_H__

#include <Ppi/FirmwareVolumeInfoPrehashedFV.h>

// {7F5E4E31-81B1-47E5-9E21-1E4B5BC2F61D}
#define EDKII_PEI_FIRMWARE_VOLUME_INFO_STORED_HASH_FV_PPI_GUID \
  {0x7f5e4e31, 0x81b1, 0x47e5, {0x9e, 0x21, 0x1e, 0x4b, 0x5b, 0xc2, 0xf6, 0x1d}}

//
// Hashed FV flags.
//
#define HASHED_FV_FLAG_REPORT_FV_INFO_PPI  0x0000000000000001
#define HASHED_FV_FLAG_REPORT_FV_HOB       0x0000000000000002
#define HASHED_FV_FLAG_VERIFIED_BOOT       0x0000000000000010
#define HASHED_FV_FLAG_MEASURED_BOOT       0x0000000000000020
#define HASHED_FV_FLAG_SKIP_ALL            0xFFFFFFFFFFFFFF00
#define HASHED_FV_FLAG_SKIP_BOOT_MODE(Mode)  LShiftU64 (0x100, (Mode))

//
// FV hash flags
//
#define FV_HASH_FLAG_BOOT_MODE(Mode)  LShiftU64 (0x100, (Mode))

typedef struct _EDKII_PEI_FIRMWARE_VOLUME_INFO_STORED_HASH_FV_PPI
                EDKII_PEI_FIRMWARE_VOLUME_INFO_STORED_HASH_FV_PPI;

typedef struct _HASHED_FV_INFO {
  UINT64    Base;
  UINT64    Length;
  UINT64    Flag;
} HASHED_FV_INFO;

typedef struct _FV_HASH_INFO {
  UINT64    HashFlag;
  UINT16    HashAlgoId;
  UINT16    HashSize;
  UINT8     Hash[64];
} FV_HASH_INFO;

//
// PPI used to convey FVs and hash information of a specific platform. Only one
// instance of this PPI is allowed in the platform.
//
struct _EDKII_PEI_FIRMWARE_VOLUME_INFO_STORED_HASH_FV_PPI {
  FV_HASH_INFO      HashInfo;
  UINTN             FvNumber;
  HASHED_FV_INFO    FvInfo[1];
};

extern EFI_GUID  gEdkiiPeiFirmwareVolumeInfoStoredHashFvPpiGuid;

#endif
