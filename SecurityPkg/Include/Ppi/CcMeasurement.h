/** @file
  CC Measurement PPI services.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _CC_MEASUREMENT_PPI_H_
#define _CC_MEASUREMENT_PPI_H_

#include <Protocol/CcMeasurement.h>

typedef struct _EDKII_CC_PPI EDKII_CC_PPI;

//
// This bit is shall be set when HashData is the pre-hash digest.
//
#define EDKII_CC_PRE_HASH  0x0000000000000001

//
// This bit is shall be set when HashData is the pre-hash digest and log only.
//
#define EDKII_CC_PRE_HASH_LOG_ONLY  0x0000000000000002

/**
  Do a hash operation on a data buffer, extend a specific RTMR with the hash result,
  and build a GUIDed HOB recording the event which will be passed to the DXE phase and
  added into the Event Log.

  @param[in]      This          Indicates the calling context
  @param[in]      Flags         Bitmap providing additional information
  @param[in]      HashData      Physical address of the start of the data buffer to be hashed.
  @param[in]      HashDataLen   The length, in bytes, of the buffer referenced by HashData.
  @param[in]      NewEventHdr   Pointer to a CC_EVENT_HDR data structure.
  @param[in]      NewEventData  Pointer to the new event data.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval Others                Other error as indicated
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_CC_HASH_LOG_EXTEND_EVENT)(
  IN      EDKII_CC_PPI             *This,
  IN      UINT64                    Flags,
  IN      EFI_PHYSICAL_ADDRESS      HashData,
  IN      UINTN                     HashDataLen,
  IN      CC_EVENT_HDR              *NewEventHdr,
  IN      UINT8                     *NewEventData
  );

/**
  The EDKII_CC_MEASUREMENT_PPI MapPcrToMrIndex function call provides callers
  the info on TPM PCR <-> CC MR mapping information.

  @param[in]  This       Indicates the calling context
  @param[in]  PcrIndex   TPM PCR index.
  @param[out] MrIndex    CC MR index.

  @retval EFI_SUCCESS    The MrIndex is returned.
  @retval Others         Other error as indicated
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_CC_MAP_PCR_TO_MR_INDEX)(
  IN  EDKII_CC_PPI      *This,
  IN  TCG_PCRINDEX      PcrIndex,
  OUT EFI_CC_MR_INDEX   *MrIndex
  );

struct _EDKII_CC_PPI {
  EDKII_CC_HASH_LOG_EXTEND_EVENT    HashLogExtendEvent;
  EDKII_CC_MAP_PCR_TO_MR_INDEX      MapPcrToMrIndex;
};

extern EFI_GUID  gEdkiiCcPpiGuid;

#endif
