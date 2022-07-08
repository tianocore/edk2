/** @file
  Library that implements the Arm CCA Realm Service Interface calls.

  Copyright (c) 2022 - 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Rsi or RSI   - Realm Service Interface
    - IPA          - Intermediate Physical Address

  @par Reference(s):
   - Realm Management Monitor (RMM) Specification, version 1.0-rel0
     (https://developer.arm.com/documentation/den0137/)

**/
#include <Base.h>

#include <IndustryStandard/ArmStdSmc.h>
#include <Library/ArmCcaRsiLib.h>
#include <Library/ArmLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include "ArmCcaRsi.h"

/** The version of RSI specification implemented by this module.
*/
#define RSI_IMPL_VERSION  RMM_VERSION (1, 0);

/** A macro to test if a pointer or address is Realm Granule size aligned.
*/
#define IS_REALM_GRANULE_ALIGNED(Ptr) \
  ADDRESS_IS_ALIGNED (Ptr, ARM_CCA_REALM_GRANULE_SIZE)

/**
  Convert the RSI status code to EFI Status code.

  @param [in]   RsiCommandReturnCode  RSI status code.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
  @retval RETURN_ABORTED            The operation was aborted as the state
                                    of the Realm or REC does not match the
                                    state expected by the command.
  @retval RETURN_NOT_READY          The operation requested by the command
                                    is not complete.
 **/
STATIC
RETURN_STATUS
ArmCcaRsiCmdStatusToReturnStatus (
  IN  UINT64  RsiCommandReturnCode
  )
{
  switch (RsiCommandReturnCode) {
    case RSI_SUCCESS:
      return RETURN_SUCCESS;
    case RSI_ERROR_INPUT:
      return RETURN_INVALID_PARAMETER;
    case RSI_ERROR_STATE:
    case RSI_ERROR_UNKNOWN:
      return RETURN_ABORTED;
    case RSI_INCOMPLETE:
      return RETURN_NOT_READY;
    default:
      // Unknown error code.
      ASSERT (0);
      break;
  } // switch

  return RETURN_ABORTED;
}

/**
  Read the Realm Configuration.

  @param [out]  Config     Pointer to the address of the buffer to retrieve
                           the Realm configuration.

  Note: The buffer to retrieve the Realm configuration must be aligned to the
        Realm granule size.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
**/
RETURN_STATUS
EFIAPI
ArmCcaRsiGetRealmConfig (
  OUT ARM_CCA_REALM_CONFIG  *Config
  )
{
  ARM_SMC_ARGS  SmcCmd;

  if ((Config == NULL) || (!IS_REALM_GRANULE_ALIGNED (Config))) {
    return RETURN_INVALID_PARAMETER;
  }

  ZeroMem (&SmcCmd, sizeof (SmcCmd));
  SmcCmd.Arg0 = ARM_CCA_FID_RSI_REALM_CONFIG;
  SmcCmd.Arg1 = (UINTN)Config;

  ArmCallSmc (&SmcCmd);
  return ArmCcaRsiCmdStatusToReturnStatus (SmcCmd.Arg0);
}

/**
   Get the version of the RSI implementation.

  @param [out] UefiImpl     The version of the RSI specification
                            implemented by the UEFI firmware.
  @param [out] RmmImplLow   The low version of the RSI specification
                            implemented by the RMM.
  @param [out] RmmImplHigh  The high version of the RSI specification
                            implemented by the RMM.

  @retval RETURN_SUCCESS                Success.
  @retval RETURN_UNSUPPORTED            The execution context is not a Realm.
  @retval RETURN_INCOMPATIBLE_VERSION   The Firmware and RMM specification
                                        revisions are not compatible.
  @retval RETURN_INVALID_PARAMETER      A parameter is invalid.
**/
RETURN_STATUS
EFIAPI
ArmCcaRsiGetVersion (
  OUT UINT32 *CONST  UefiImpl,
  OUT UINT32 *CONST  RmmImplLow,
  OUT UINT32 *CONST  RmmImplHigh
  )
{
  RETURN_STATUS  Status;
  ARM_SMC_ARGS   SmcCmd;

  if ((UefiImpl == NULL) || (RmmImplLow == NULL) || (RmmImplHigh == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  ZeroMem (&SmcCmd, sizeof (SmcCmd));
  SmcCmd.Arg0 = ARM_CCA_FID_RSI_VERSION;
  SmcCmd.Arg1 = RSI_IMPL_VERSION;
  ArmCallSmc (&SmcCmd);
  if (SmcCmd.Arg0 == ARM_SMC_MM_RET_NOT_SUPPORTED) {
    // This FID is not implemented, which means
    // we are not running in a Realm, therefore
    // return the error code as unsupported.
    return RETURN_UNSUPPORTED;
  }

  *RmmImplLow  = (SmcCmd.Arg1 & RSI_VERSION_MASK);
  *RmmImplHigh = (SmcCmd.Arg2 & RSI_VERSION_MASK);
  *UefiImpl    = RSI_IMPL_VERSION;

  // The RSI_VERSION command does not have any failure
  // conditions see section B5.3.10.2 Failure conditions
  // Therefore the only defined return values are
  // RSI_SUCCESS and RSI_ERROR_INPUT.
  Status = ArmCcaRsiCmdStatusToReturnStatus (SmcCmd.Arg0);
  if (Status == RETURN_INVALID_PARAMETER) {
    // RSI_VERSION returns RSI_ERROR_INPUT when
    // the RMM does not support an interface revision
    // which is compatible with the requested revision.
    // Since RSI_ERROR_INPUT is mapped to RETURN_INVALID_PARAMETER
    // by ArmCcaRsiCmdStatusToReturnStatus(), return the status code as
    // RETURN_INCOMPATIBLE_VERSION.
    return RETURN_INCOMPATIBLE_VERSION;
  }

  // Add an assert in case RMM returns a different error code than expected.
  ASSERT (Status == RETURN_SUCCESS);
  return Status;
}
