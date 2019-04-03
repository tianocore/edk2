/** @file
  This library is to support TCG PC Client Platform Physical Presence Interface Specification
  Family "2.0" part, >= 128 Vendor Specific PPI Operation.

  The Vendor Specific PPI operation may change TPM state, BIOS TPM management
  flags, and may need additional boot cycle.

  Caution: This function may receive untrusted input.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _TCG2_PP_VENDOR_LIB_H_
#define _TCG2_PP_VENDOR_LIB_H_

#include <IndustryStandard/Tpm20.h>
#include <Protocol/Tcg2Protocol.h>
#include <Library/Tcg2PhysicalPresenceLib.h>

/**
  Check and execute the requested physical presence command.

  This API should be invoked in BIOS boot phase to process pending request.

  Caution: This function may receive untrusted input.

  If OperationRequest < 128, then ASSERT().

  @param[in]      PlatformAuth     platform auth value. NULL means no platform auth change.
  @param[in]      OperationRequest TPM physical presence operation request.
  @param[in, out] ManagementFlags  BIOS TPM Management Flags.
  @param[out]     ResetRequired    If reset is required to vendor settings in effect.
                                   True, it indicates the reset is required.
                                   False, it indicates the reset is not required.

  @return TPM Operation Response to OS Environment.
**/
UINT32
EFIAPI
Tcg2PpVendorLibExecutePendingRequest (
  IN TPM2B_AUTH             *PlatformAuth,  OPTIONAL
  IN UINT32                 OperationRequest,
  IN OUT UINT32             *ManagementFlags,
  OUT BOOLEAN               *ResetRequired
  );

/**
  Check if there is a valid physical presence command request.

  This API should be invoked in BIOS boot phase to process pending request.

  Caution: This function may receive untrusted input.

  If OperationRequest < 128, then ASSERT().

  @param[in]      OperationRequest TPM physical presence operation request.
  @param[in]      ManagementFlags  BIOS TPM Management Flags.
  @param[out]     RequestConfirmed If the physical presence operation command required user confirm from UI.
                                   True, it indicates the command doesn't require user confirm.
                                   False, it indicates the command need user confirm from UI.

  @retval  TRUE        Physical Presence operation command is valid.
  @retval  FALSE       Physical Presence operation command is invalid.
**/
BOOLEAN
EFIAPI
Tcg2PpVendorLibHasValidRequest (
  IN UINT32                 OperationRequest,
  IN UINT32                 ManagementFlags,
  OUT BOOLEAN               *RequestConfirmed
  );

/**
  The callback for TPM vendor specific physical presence which is called for
  Submit TPM Operation Request to Pre-OS Environment and
  Submit TPM Operation Request to Pre-OS Environment 2.

  This API should be invoked in OS runtime phase to interface with ACPI method.

  Caution: This function may receive untrusted input.

  If OperationRequest < 128, then ASSERT().

  @param[in]      OperationRequest TPM physical presence operation request.
  @param[in]      ManagementFlags  BIOS TPM Management Flags.
  @param[in]      RequestParameter Extra parameter from the passed package.

  @return Return Code for Submit TPM Operation Request to Pre-OS Environment and
          Submit TPM Operation Request to Pre-OS Environment 2.
**/
UINT32
EFIAPI
Tcg2PpVendorLibSubmitRequestToPreOSFunction (
  IN UINT32                 OperationRequest,
  IN UINT32                 ManagementFlags,
  IN UINT32                 RequestParameter
  );

/**
  The callback for TPM vendor specific physical presence which is called for
  Get User Confirmation Status for Operation.

  This API should be invoked in OS runtime phase to interface with ACPI method.

  Caution: This function may receive untrusted input.

  If OperationRequest < 128, then ASSERT().

  @param[in]      OperationRequest TPM physical presence operation request.
  @param[in]      ManagementFlags  BIOS TPM Management Flags.

  @return Return Code for Get User Confirmation Status for Operation.
**/
UINT32
EFIAPI
Tcg2PpVendorLibGetUserConfirmationStatusFunction (
  IN UINT32                 OperationRequest,
  IN UINT32                 ManagementFlags
  );

#endif
