/** @file
  TCG PPI services.

Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _CC_MEASUREMENT_PPI_H_
#define _CC_MEASUREMENT_PPI_H_

// #include <IndustryStandard/UefiTcgPlatform.h>
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
  Tpm measure and log data, and extend the measurement result into a specific PCR.

  @param[in]      This          Indicates the calling context
  @param[in]      Flags         Bitmap providing additional information
  @param[in]      HashData      If BIT0 of Flags is 0, it is physical address of the
                                start of the data buffer to be hashed, extended, and logged.
                                If BIT0 of Flags is 1, it is physical address of the
                                start of the pre-hash data buffter to be extended, and logged.
                                The pre-hash data format is TPML_DIGEST_VALUES.
  @param[in]      HashDataLen   The length, in bytes, of the buffer referenced by HashData.
  @param[in]      NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.
  @param[in]      NewEventData  Pointer to the new event data.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_UNSUPPORTED       TPM device not available.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.
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
  The EFI_CC_MEASUREMENT_PROTOCOL MapPcrToMrIndex function call provides callers
  the info on TPM PCR <-> CC MR mapping information.

  @param[in]  This               Indicates the calling context
  @param[in]  PcrIndex           TPM PCR index.
  @param[out] MrIndex            CC MR index.

  @retval EFI_SUCCESS            The MrIndex is returned.
  @retval EFI_INVALID_PARAMETER  The MrIndex is NULL.
  @retval EFI_UNSUPPORTED        The PcrIndex is invalid.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_CC_MAP_PCR_TO_MR_INDEX)(
  IN  EDKII_CC_PPI      *This,
  IN  TCG_PCRINDEX      PcrIndex,
  OUT EFI_CC_MR_INDEX   *MrIndex
  );

///
/// The EFI_TCG Protocol abstracts TCG activity.
///
struct _EDKII_CC_PPI {
  EDKII_CC_HASH_LOG_EXTEND_EVENT    HashLogExtendEvent;
  EDKII_CC_MAP_PCR_TO_MR_INDEX      MapPcrToMrIndex;
};

extern EFI_GUID  gEdkiiCcPpiGuid;

#endif
