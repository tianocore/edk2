/** @file
  Arm FF-A ns common library Header file

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  Copyright (c), Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ARM_FFA_RX_TX_BUFFER_INFO_H_
#define ARM_FFA_RX_TX_BUFFER_INFO_H_

#include <Uefi/UefiBaseType.h>

/**
 * Guid Hob Data for gArmFfaRxTxBufferInfoGuid Guid Hob.
 */
typedef struct ArmFfaRxTxBuffersInfo {
  /// Tx Buffer Address.
  EFI_PHYSICAL_ADDRESS    TxBufferAddr;

  /// Tx Buffer Size.
  UINT64                  TxBufferSize;

  /// Rx Buffer Address.
  EFI_PHYSICAL_ADDRESS    RxBufferAddr;

  /// Rx Buffer Size.
  UINT64                  RxBufferSize;

  /// Rx/Tx buffer should be remapped to permanent memory.
  BOOLEAN                 RemapRequired;

  /// Rx/Tx buffer offset from its allocation base.
  UINT64                  RemapOffset;
} ARM_FFA_RX_TX_BUFFER_INFO;

extern EFI_GUID  gArmFfaRxTxBufferInfoGuid;

#endif
