/** @file
  TdxHelper Functions which are used in SEC phase

  Copyright (c) 2022 - 2023, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/BaseMemoryLib.h>
#include <IndustryStandard/Tdx.h>
#include <IndustryStandard/IntelTdx.h>
#include <IndustryStandard/Tpm20.h>
#include <Library/TdxLib.h>
#include <Pi/PrePiHob.h>
#include <WorkArea.h>
#include <ConfidentialComputingGuestAttr.h>
#include <Library/TdxHelperLib.h>

/**
  Build the GuidHob for tdx measurements which were done in SEC phase.
  The measurement values are stored in WorkArea.

  @retval EFI_SUCCESS  The GuidHob is built successfully
  @retval Others       Other errors as indicated
**/
EFI_STATUS
InternalBuildGuidHobForTdxMeasurement (
  VOID
  );

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
  )
{
  return EFI_UNSUPPORTED;
}

//
// SHA512_CTX is defined in <openssl/sha.h> and its size is 216 bytes.
// It can be built successfully with GCC5 compiler but failed with VS2019.
// The error code showed in VS2019 is that "openssl/sha.h" cannot be found.
// To overcome this error SHA512_CTX_SIZE is defined.
//
#define SHA512_CTX_SIZ  216

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
STATIC
EFI_STATUS
HashAndExtendToRtmr (
  IN UINT32  RtmrIndex,
  IN VOID    *DataToHash,
  IN UINTN   DataToHashLen,
  OUT UINT8  *Digest,
  IN  UINTN  DigestLen
  )
{
  EFI_STATUS  Status;
  UINT8       Sha384Ctx[SHA512_CTX_SIZ];

  if ((DataToHash == NULL) || (DataToHashLen == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Digest == NULL) || (DigestLen != SHA384_DIGEST_SIZE)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Calculate the sha384 of the data
  //
  Sha384Init (Sha384Ctx);
  Sha384Update (Sha384Ctx, DataToHash, DataToHashLen);
  Sha384Final (Sha384Ctx, Digest);

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
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  EFI_STATUS            Status;
  UINT8                 Digest[SHA384_DIGEST_SIZE];
  OVMF_WORK_AREA        *WorkArea;
  VOID                  *TdHob;

  TdHob   = (VOID *)(UINTN)FixedPcdGet32 (PcdOvmfSecGhcbBase);
  Hob.Raw = (UINT8 *)TdHob;

  //
  // Walk thru the TdHob list until end of list.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  Status = HashAndExtendToRtmr (
             0,
             (UINT8 *)TdHob,
             (UINTN)((UINT8 *)Hob.Raw - (UINT8 *)TdHob),
             Digest,
             SHA384_DIGEST_SIZE
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // This function is called in SEC phase and at that moment the Hob service
  // is not available. So the TdHob measurement value is stored in workarea.
  //
  WorkArea = (OVMF_WORK_AREA *)FixedPcdGet32 (PcdOvmfWorkAreaBase);
  if (WorkArea == NULL) {
    return EFI_DEVICE_ERROR;
  }

  WorkArea->TdxWorkArea.SecTdxWorkArea.TdxMeasurementsData.MeasurementsBitmap |= TDX_MEASUREMENT_TDHOB_BITMASK;
  CopyMem (WorkArea->TdxWorkArea.SecTdxWorkArea.TdxMeasurementsData.TdHobHashValue, Digest, SHA384_DIGEST_SIZE);

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS      Status;
  UINT8           Digest[SHA384_DIGEST_SIZE];
  OVMF_WORK_AREA  *WorkArea;

  Status = HashAndExtendToRtmr (
             0,
             (UINT8 *)(UINTN)PcdGet32 (PcdOvmfFlashNvStorageVariableBase),
             (UINT64)PcdGet32 (PcdCfvRawDataSize),
             Digest,
             SHA384_DIGEST_SIZE
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // This function is called in SEC phase and at that moment the Hob service
  // is not available. So CfvImage measurement value is stored in workarea.
  //
  WorkArea = (OVMF_WORK_AREA *)FixedPcdGet32 (PcdOvmfWorkAreaBase);
  if (WorkArea == NULL) {
    return EFI_DEVICE_ERROR;
  }

  WorkArea->TdxWorkArea.SecTdxWorkArea.TdxMeasurementsData.MeasurementsBitmap |= TDX_MEASUREMENT_CFVIMG_BITMASK;
  CopyMem (WorkArea->TdxWorkArea.SecTdxWorkArea.TdxMeasurementsData.CfvImgHashValue, Digest, SHA384_DIGEST_SIZE);

  return EFI_SUCCESS;
}

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
  )
{
 #ifdef TDX_PEI_LESS_BOOT
  return InternalBuildGuidHobForTdxMeasurement ();
 #else
  return EFI_UNSUPPORTED;
 #endif
}
