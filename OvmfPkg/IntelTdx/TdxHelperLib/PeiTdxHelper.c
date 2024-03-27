/** @file
  TdxHelper Functions which are used in PEI phase

  Copyright (c) 2022 - 2023, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <PiPei.h>
#include <Library/DebugLib.h>
#include <IndustryStandard/UefiTcgPlatform.h>

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
  return EFI_UNSUPPORTED;
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
  return EFI_UNSUPPORTED;
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
  return InternalBuildGuidHobForTdxMeasurement ();
}

EFI_STATUS
HashAndExtendToRtmr (
  IN UINT32  RtmrIndex,
  IN VOID    *DataToHash,
  IN UINTN   DataToHashLen,
  OUT UINT8  *Digest,
  IN  UINTN  DigestLen
  );

EFI_STATUS
BuildTdxMeasurementGuidHob (
  UINT32  RtmrIndex,
  UINT32  EventType,
  UINT8   *EventData,
  UINT32  EventSize,
  UINT8   *HashValue,
  UINT32  HashSize
  );

/**
 * In Tdx guest, OVMF uses FW_CFG_SELECTOR(0x510) and FW_CFG_IO_DATA(0x511)
 * to get configuration infomation from QEMU. From the security perspective,
 * that should be measured before it is consumed.

  @retval EFI_SUCCESS  The measurement is successfully
  @retval Others       Other errors as indicated
**/
EFI_STATUS
EFIAPI
TdxHelperMeasureFwCfgData (
  IN VOID    *EventLog,
  IN UINT32  LogLen,
  IN VOID    *HashData,
  IN UINT64  HashDataLen
  )
{
  EFI_STATUS  Status;
  UINT8       Digest[SHA384_DIGEST_SIZE];

  if ((EventLog == NULL) || (HashData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!TdIsEnabled ()) {
    return EFI_UNSUPPORTED;
  }

  Status = HashAndExtendToRtmr (
             0,
             (UINT8 *)HashData,
             (UINTN)HashDataLen,
             Digest,
             SHA384_DIGEST_SIZE
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: HashAndExtendToRtmr failed with %r\n", __func__, Status));
    return Status;
  }

  Status = BuildTdxMeasurementGuidHob (
             0,
             EV_PLATFORM_CONFIG_FLAGS,
             EventLog,
             LogLen,
             Digest,
             SHA384_DIGEST_SIZE
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: BuildTdxMeasurementGuidHob failed with %r\n", __func__, Status));
  }

  return Status;
}
