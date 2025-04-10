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

#ifndef ARM_FFA_RX_TX_MAP_LIB_H_
#define ARM_FFA_RX_TX_MAP_LIB_H_

/**
 * Guid Hob Data for gArmFfaRxTxBufferInfoGuid Guid Hob.
 */
typedef struct ArmFfaRxTxBuffersInfo {
  /// Tx Buffer Address.
  VOID      *TxBufferAddr;

  /// Tx Buffer Size.
  UINT64    TxBufferSize;

  /// Rx Buffer Address.
  VOID      *RxBufferAddr;

  /// Rx Buffer Size.
  UINT64    RxBufferSize;
} ARM_FFA_RX_TX_BUFFER_INFO;

/**
  Mapping Rx/Tx buffers.
  This function is only called in ArmFfaLibConstructor because
  Rx/Tx buffer is registered only once per partition.

  @retval EFI_SUCCESS
  @retval EFI_ALREADY_STARTED     Rx/Tx buffer already mapped in PEI phase
  @retval EFI_OUT_OF_RESOURCE     Out of memory
  @retval EFI_INVALID_PARAMETER   Invalid alignment of Rx/Tx buffer
  @retval Others                  Error

**/
EFI_STATUS
EFIAPI
ArmFfaLibRxTxMap (
  IN VOID
  );

/**
  Unmap Rx/Tx buffer.
  This function is only called in Exit boot service because
  Rx/Tx buffer is registered only once per partition.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETERS               Already unregistered
  @retval EFI_UNSUPPORTED                      Not supported

**/
EFI_STATUS
EFIAPI
ArmFfaLibRxTxUnmap (
  IN VOID
  );

#endif
