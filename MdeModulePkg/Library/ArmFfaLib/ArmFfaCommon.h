/** @file
  Arm FF-A ns common library Header file

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
     - FF-A - Firmware Framework for Arm A-profile
     - spmc - Secure Partition Manager Core
     - spmd - Secure Partition Manager Dispatcher

  @par Reference(s):
     - Arm Firmware Framework for Arm A-Profile [https://developer.arm.com/documentation/den0077/latest]

**/

#ifndef ARM_FFA_COMMON_LIB_H_
#define ARM_FFA_COMMON_LIB_H_

#include <Library/ArmFfaLib.h>

/**
  Convert FfArgs to EFI_STATUS.

  @param [in] FfaArgs            Ffa arguments

  @retval EFI_STATUS             return value correspond EFI_STATUS to FfaStatus

**/
EFI_STATUS
EFIAPI
FfaArgsToEfiStatus (
  IN ARM_FFA_ARGS  *FfaArgs
  );

/**
  Common ArmFfaLib init.

  @param [out] PartId            PartitionId
  @param [out] IsFfaSupported    FF-A supported flag

  @retval EFI_SUCCESS            Success
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval Others                 Error

**/
EFI_STATUS
EFIAPI
ArmFfaLibCommonInit (
  OUT UINT16   *PartId,
  OUT BOOLEAN  *IsFfaSupported
  );

/**
  Get first Rx/Tx Buffer allocation hob.
  If UseGuid is TRUE, BufferAddr and BufferSize parameters are ignored.

  @param[in]  BufferAddr       Buffer address
  @param[in]  BufferSize       Buffer Size
  @param[in]  UseGuid          Find MemoryAllocationHob using gArmFfaRxTxBufferInfoGuid.

  @retval     NULL             Not found
  @retval     Other            MemoryAllocationHob related to Rx/Tx buffer

**/
EFI_HOB_MEMORY_ALLOCATION *
EFIAPI
GetRxTxBufferAllocationHob (
  IN EFI_PHYSICAL_ADDRESS  BufferAddr,
  IN UINT64                BufferSize,
  IN BOOLEAN               UseGuid
  );

/**
  Get Rx/Tx buffer MinSizeAndAign and MaxSize

  @param[out] MinSizeAndAlign  Minimum size of Buffer.

  @retval EFI_SUCCESS
  @retval EFI_UNSUPPORTED          Wrong min size received from SPMC
  @retval EFI_INVALID_PARAMETER    Wrong buffer size
  @retval Others                   Failure of ArmFfaLibGetFeatures()

**/
EFI_STATUS
EFIAPI
GetRxTxBufferMinSizeAndAlign (
  OUT UINTN  *MinSizeAndAlign
  );

/**
  Determine if FF-A is supported

  @retval TRUE if FF-A is supported, FALSE otherwise.

**/
BOOLEAN
EFIAPI
ArmFfaLibIsFfaSupported (
  IN VOID
  );

/**
  Return partition or VM ID

  @param[out] PartId  The partition or VM ID

  @retval EFI_SUCCESS  Partition ID or VM ID returned
  @retval Others       Errors

**/
EFI_STATUS
EFIAPI
ArmFfaLibGetPartId (
  OUT UINT16  *PartId
  );

#endif
