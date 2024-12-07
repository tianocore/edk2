/** @file
  TdxHelperLib header file

  Copyright (c) 2021 - 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TDX_HELPER_LIB_H
#define TDX_HELPER_LIB_H

#include <PiPei.h>

#define   CC_MR_INDEX_0_MRTD   0
#define   CC_MR_INDEX_1_RTMR0  1
#define   CC_MR_INDEX_2_RTMR1  2
#define   CC_MR_INDEX_3_RTMR2  3
#define   CC_MR_INDEX_INVALID  4

/**
  In Tdx guest, some information need to be passed from host VMM to guest
  firmware. For example, the memory resource, etc. These information are
  prepared by host VMM and put in TdHob which is described in TdxMetadata.
  TDVF processes the TdHob to accept memories.

  @retval   EFI_SUCCESS   Successfully process the TdHob
  @retval   Others        Other error as indicated
**/
EFI_STATUS
EFIAPI
TdxHelperProcessTdHob (
  VOID
  );

/**
  In Tdx guest, TdHob is passed from host VMM to guest firmware and it contains
  the information of the memory resource. From the security perspective before
  it is consumed, it should be measured and extended.
 *
 * @retval EFI_SUCCESS Successfully measure the TdHob
 * @retval Others      Other error as indicated
 */
EFI_STATUS
EFIAPI
TdxHelperMeasureTdHob (
  VOID
  );

/**
 * In Tdx guest, Configuration FV (CFV) is treated as external input because it
 * may contain the data provided by VMM. From the sucurity perspective Cfv image
 * should be measured before it is consumed.
 *
 * @retval EFI_SUCCESS Successfully measure the CFV image
 * @retval Others      Other error as indicated
 */
EFI_STATUS
EFIAPI
TdxHelperMeasureCfvImage (
  VOID
  );

/**
  Build the GuidHob for tdx measurements which were done in SEC phase.
  The measurement values are stored in WorkArea.

  @retval EFI_SUCCESS  The GuidHob is built successfully
  @retval Others       Other errors as indicated
**/
EFI_STATUS
EFIAPI
TdxHelperBuildGuidHobForTdxMeasurement (
  VOID
  );

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
TdxHelperMapPcrToMrIndex (
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
TdxHelperBuildTdxMeasurementGuidHob (
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
TdxHelperHashAndExtendToRtmr (
  IN UINT32  RtmrIndex,
  IN VOID    *DataToHash,
  IN UINTN   DataToHashLen,
  OUT UINT8  *Digest,
  IN  UINTN  DigestLen
  );

#endif
