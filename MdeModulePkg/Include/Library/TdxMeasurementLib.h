/** @file
  TdxMeasurementLib header file
  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef TDX_MEASUREMENT_LIB_H
#define TDX_MEASUREMENT_LIB_H

#include <PiPei.h>

#define   CC_MR_INDEX_0_MRTD   0
#define   CC_MR_INDEX_1_RTMR0  1
#define   CC_MR_INDEX_2_RTMR1  2
#define   CC_MR_INDEX_3_RTMR2  3
#define   CC_MR_INDEX_INVALID  4

/**
  According to UEFI Spec 2.10 Section 38.4.1:
    The following table shows the TPM PCR index mapping and CC event log measurement
  register index interpretation for Intel TDX, where MRTD means Trust Domain Measurement
   Register and RTMR means Runtime Measurement Register
    // TPM PCR Index | CC Measurement Register Index | TDX-measurement register
    //  ------------------------------------------------------------------------
    // 0             |   0                           |   MRTD
    // 1, 7          |   1                           |   RTMR[0]
    // 2~6           |   2                           |   RTMR[1]
    // 8~15          |   3                           |   RTMR[2]
  @param[in] PCRIndex Index of the TPM PCR
  @retval    UINT32               Index of the CC Event Log Measurement Register Index
  @retval    CC_MR_INDEX_INVALID  Invalid MR Index
**/
UINT32
EFIAPI
TdxMeasurementMapPcrToMrIndex (
  IN  UINT32  PCRIndex
  );

/**
 * Build GuidHob for Tdx CC measurement event.
 *
 * @param RtmrIndex     RTMR index
 * @param EventType     Event type
 * @param EventData     Event data
 * @param EventSize     Size of event data
 * @param HashValue     Hash value
 * @param HashSize      Size of hash
 *
 * @retval EFI_SUCCESS  Successfully build the GuidHobs
 * @retval Others       Other error as indicated
 */
EFI_STATUS
EFIAPI
TdxMeasurementBuildGuidHob (
  UINT32  RtmrIndex,
  UINT32  EventType,
  UINT8   *EventData,
  UINT32  EventSize,
  UINT8   *HashValue,
  UINT32  HashSize
  );

/**
 * Calculate the sha384 of input Data and extend it to RTMR register.
 *
 * @param RtmrIndex       Index of the RTMR register
 * @param DataToHash      Data to be hashed
 * @param DataToHashLen   Length of the data
 * @param Digest          Hash value of the input data
 * @param DigestLen       Length of the hash value
 *
 * @retval EFI_SUCCESS    Successfully hash and extend to RTMR
 * @retval Others         Other errors as indicated
 */
EFI_STATUS
EFIAPI
TdxMeasurementHashAndExtendToRtmr (
  IN UINT32  RtmrIndex,
  IN VOID    *DataToHash,
  IN UINTN   DataToHashLen,
  OUT UINT8  *Digest,
  IN  UINTN  DigestLen
  );

#endif
