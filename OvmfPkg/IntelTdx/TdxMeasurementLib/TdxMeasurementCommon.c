/** @file
   TdxMeasurement Common Functions

Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Ppi/CcMeasurement.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/TdxLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/TdxMeasurementLib.h>

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
  )
{
  UINT32  MrIndex;

  if (PCRIndex > 15) {
    ASSERT (FALSE);
    return CC_MR_INDEX_INVALID;
  }

  MrIndex = 0;
  if (PCRIndex == 0) {
    MrIndex = CC_MR_INDEX_0_MRTD;
  } else if ((PCRIndex == 1) || (PCRIndex == 7)) {
    MrIndex = CC_MR_INDEX_1_RTMR0;
  } else if ((PCRIndex >= 2) && (PCRIndex <= 6)) {
    MrIndex = CC_MR_INDEX_2_RTMR1;
  } else if ((PCRIndex >= 8) && (PCRIndex <= 15)) {
    MrIndex = CC_MR_INDEX_3_RTMR2;
  }

  return MrIndex;
}

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
  )
{
  EFI_STATUS  Status;

  if ((DataToHash == NULL) || (DataToHashLen == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Digest == NULL) || (DigestLen != SHA384_DIGEST_SIZE)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Calculate the sha384 of the data
  //
  if (!Sha384HashAll (DataToHash, DataToHashLen, Digest)) {
    return EFI_ABORTED;
  }

  //
  // Extend to RTMR
  //
  Status = TdExtendRtmr (
             (UINT32 *)Digest,
             SHA384_DIGEST_SIZE,
             (UINT8)RtmrIndex
             );
  ASSERT (!EFI_ERROR (Status));
  return Status;
}
