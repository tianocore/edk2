/** @file
  Arm Ffa library common code.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

   @par Glossary:
     - FF-A - Firmware Framework for Arm A-profile

   @par Reference(s):
     - Arm Firmware Framework for Arm A-Profile [https://developer.arm.com/documentation/den0077/latest]

**/
#include <PiMm.h>

#include <Library/ArmLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/ArmFfaLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#include <Guid/ArmFfaRxTxBufferInfo.h>

#include "ArmFfaCommon.h"
#include "ArmFfaRxTxMap.h"

/**
  Get mapped Rx/Tx buffers.

  @param [out]   TxBuffer         Address of TxBuffer
  @param [out]   TxBufferSize     Size of TxBuffer
  @param [out]   RxBuffer         Address of RxBuffer
  @param [out]   RxBufferSize     Size of RxBuffer

  @retval EFI_SUCCESS
  @retval Others             Error.

**/
EFI_STATUS
EFIAPI
ArmFfaLibGetRxTxBuffers (
  OUT VOID    **TxBuffer OPTIONAL,
  OUT UINT64  *TxBufferSize OPTIONAL,
  OUT VOID    **RxBuffer OPTIONAL,
  OUT UINT64  *RxBufferSize OPTIONAL
  )
{
  /*
   * StandaloneMm doesn't use Rx/Tx buffer.
   * So, return EFI_UNSUPPORTED.
   */
  return EFI_UNSUPPORTED;
}

/**
  Mapping Rx/Tx buffers.
  This function is only called in ArmFfaLibConstructor because
  Rx/Tx buffer is registered only once per partition.

  @retval EFI_SUCCESS
  @retval EFI_ALREADY_STARTED     Rx/Tx buffer already mapped.
  @retval EFI_OUT_OF_RESOURCE     Out of memory
  @retval EFI_INVALID_PARAMETER   Invalid alignment of Rx/Tx buffer
  @retval Others                  Error

**/
EFI_STATUS
EFIAPI
ArmFfaLibRxTxMap (
  IN VOID
  )
{
  /*
   * StandaloneMm doesn't use Rx/Tx buffer.
   * So, return EFI_UNSUPPORTED.
   */
  return EFI_UNSUPPORTED;
}

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
  )
{
  /*
   * StandaloneMm doesn't use Rx/Tx buffer.
   * So, return EFI_UNSUPPORTED.
   */
  return EFI_UNSUPPORTED;
}

/**
  Update Rx/TX buffer information.

  @param  BufferInfo            Rx/Tx buffer information.

**/
VOID
EFIAPI
UpdateRxTxBufferInfo (
  OUT ARM_FFA_RX_TX_BUFFER_INFO  *BufferInfo
  )
{
  /*
   * StandaloneMm doesn't use Rx/Tx buffer.
   */
  return;
}

/**
  Find Rx/TX buffer memory allocation hob.

  @param  UseGuid           Find MemoryAllocationHob using Guid.

  @retval MemoryAllocationHob
  @retval NULL                No memory allocation hob related to Rx/Tx buffer

**/
EFI_HOB_MEMORY_ALLOCATION *
EFIAPI
FindRxTxBufferAllocationHob (
  IN BOOLEAN  UseGuid
  )
{
  /*
   * StandaloneMm doesn't use Rx/Tx buffer.
   */
  return NULL;
}

/**
  Remap Rx/TX buffer with converted Rx/Tx Buffer address after
  using permanent memory.

  @param[out] BufferInfo    BufferInfo

  @retval EFI_SUCCESS       Success
  @retval EFI_NOT_FOUND     No memory allocation hob related to Rx/Tx buffer

**/
EFI_STATUS
EFIAPI
RemapFfaRxTxBuffer (
  OUT ARM_FFA_RX_TX_BUFFER_INFO  *BufferInfo
  )
{
  /*
   * StandaloneMm doesn't use Rx/Tx buffer.
   * So, return EFI_UNSUPPORTED.
   */
  return EFI_UNSUPPORTED;
}
