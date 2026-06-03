/** @file
  Arm CCA Boot Sync Attestation protocol verification interfaces.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include "Include/ArmCcaTokenParser.h"
#include "Include/BootSyncBsbMsg.h"
#include "Include/BootSyncBsbParser.h"
#include "Include/BootSyncDebug.h"
#include "Include/BootSyncSecureChannel.h"

/**
  Perform verification of the Attestation Report.

  @param[in]  SecChannel          Pointer to the secure channel.
  @param[in]  AttReq              Pointer to the Attestation request.
  @param[out] AttestationResult   The result of the attestation report
                                  verification.
                                  - ATTESTATION_RESULT_VERIFY_SUCCESS
                                  - ATTESTATION_RESULT_VERIFY_FAILURE
  @param[out] BootSyncCompleted   Was Boot Sync already completed?

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
BootSyncPerformVerification (
  IN SECURE_CHANNEL         *SecChannel,
  IN  BOOT_SYNC_BSB_HEADER  *AttReq,
  OUT UINT64                *AttestationResult,
  OUT BOOLEAN               *BootSyncCompleted
  )
{
  EFI_STATUS             Status;
  BOOT_SYNC_BSB_ELEMENT  *BsbElement;
  UINT8                  *AttestationReport;
  UINTN                  AttestationReportLen;
  UINT8                  *Data;
  UINTN                  Length;
  ATTESTATION_CLAIMS     Claims;
  BOOLEAN                BindingKeyVerificationCheck;
  BOOLEAN                RpvVerificationCheck;

  if ((AttReq == NULL) || (AttestationResult == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = BsbGetElement (
             AttReq,
             &gArmBootSyncAttReport,
             &BsbElement
             );
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  AttestationReport    = (UINT8 *)(BsbElement + 1);
  AttestationReportLen = BsbElement->Header.Length -
                         sizeof (BOOT_SYNC_BSB_ELEMENT);

  DBG_DUMP_RAW ("Attestation Report", AttestationReport, AttestationReportLen);

  BindingKeyVerificationCheck = FALSE;
  RpvVerificationCheck        = FALSE;

  Status = ParseAttestationToken (
             AttestationReport,
             AttestationReportLen,
             &Claims
             );

  if (EFI_ERROR (Status)) {
    goto ExitHandler;
  }

  DEBUG ((DEBUG_INFO, "Token Parsing Result = 0x%x\n", Status));

  Status = AttestationTokenClaimGetChallenge (&Claims, &Data, &Length);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to get Challenge Data, Status = %r.\n",
      Status
      ));
    goto ExitHandler;
  }

  DBG_DUMP_RAW ("Claims.challenge", Data, Length);
  // Check if the Binding Key is the challenge.
  if ((Length == BINDING_KEY_SIZE) &&
      (CompareMem (
         SecChannel->Kb,
         Data,
         BINDING_KEY_SIZE
         ) == 0)
      )
  {
    DEBUG ((DEBUG_ERROR, "Info: Binding Key Verified Successfully.\n"));
    BindingKeyVerificationCheck = TRUE;
  } else {
    DEBUG ((DEBUG_ERROR, "Error: Binding Key Verification Failed!\n"));
    goto ExitHandler;
  }

  Status = AttestationTokenClaimGetRpv (&Claims, &Data, &Length);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to get RPV, Status = %r.\n",
      Status
      ));
    goto ExitHandler;
  }

  DBG_DUMP_RAW ("Claims.RPV", Data, Length);

  if (Length != ARM_CCA_REALM_CFG_RPV_SIZE) {
    DEBUG ((DEBUG_ERROR, "Error: Invalid RPV size!\n"));
    goto ExitHandler;
  } else {
    CopyMem (
      SecChannel->Rpv,
      Data,
      ARM_CCA_REALM_CFG_RPV_SIZE
      );
    RpvVerificationCheck = TRUE;
  }

  Status = AttestationTokenClaimGetRim (&Claims, &Data, &Length);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to get RIM, Status = %r.\n",
      Status
      ));
    goto ExitHandler;
  }

  DBG_DUMP_RAW ("Claims.RIM", Data, Length);

  Status = AttestationTokenClaimGetRem (&Claims, 0, &Data, &Length);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to get REM0, Status = %r.\n",
      Status
      ));
    goto ExitHandler;
  }

  DBG_DUMP_RAW ("Claims.REM0", Data, Length);

  Status = AttestationTokenClaimGetRem (&Claims, 1, &Data, &Length);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to get REM1, Status = %r.\n",
      Status
      ));
    goto ExitHandler;
  }

  DBG_DUMP_RAW ("Claims.REM1", Data, Length);

  Status = AttestationTokenClaimGetRem (&Claims, 2, &Data, &Length);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to get REM2, Status = %r.\n",
      Status
      ));
    goto ExitHandler;
  }

  DBG_DUMP_RAW ("Claims.REM2", Data, Length);

  Status = AttestationTokenClaimGetRem (&Claims, 3, &Data, &Length);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to get REM3, Status = %r.\n",
      Status
      ));
    goto ExitHandler;
  }

  DBG_DUMP_RAW ("Claims.REM3", Data, Length);

  // Check that the firmware has not already requested the BIB Secrets.
  // If the BIB secrets were sent the firmware would have extended the
  // REM3. This check is important to avoid potential person-in-middle
  // attacks.
  if (!IsZeroBuffer (Data, Length)) {
    *BootSyncCompleted = TRUE;
    DEBUG ((
      DEBUG_WARN,
      "Warning: BIB Already Sent. Do not release secrets !!!\n"
      ));
  } else {
    *BootSyncCompleted = FALSE;
  }

ExitHandler:
  if (!EFI_ERROR (Status) &&
      BindingKeyVerificationCheck &&
      RpvVerificationCheck)
  {
    *AttestationResult = ATTESTATION_RESULT_VERIFY_SUCCESS;
  } else {
    *AttestationResult = ATTESTATION_RESULT_VERIFY_FAILURE;
  }

  return Status;
}
