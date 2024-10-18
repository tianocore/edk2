/** @file
  Arm Ffa library common code.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

   @par Glossary:
     - FF-A - Firmware Framework for Arm A-profile

   @par Reference(s):
     - Arm Firmware Framework for Arm A-Profile [https://developer.arm.com/documentation/den0077/latest]

**/
#include <Uefi.h>
#include <Pi/PiMultiPhase.h>

#include <Library/ArmLib.h>
#include <Library/ArmFfaLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <IndustryStandard/ArmFfaSvc.h>

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
  OUT VOID    **TxBuffer,
  OUT UINT64  *TxBufferSize,
  OUT VOID    **RxBuffer,
  OUT UINT64  *RxBufferSize
  )
{
  if ((PcdGet64 (PcdFfaTxBuffer) == 0x00) || (PcdGet64 (PcdFfaRxBuffer) == 0x00)) {
    return EFI_NOT_READY;
  }

  if (TxBuffer != NULL) {
    *TxBuffer = (VOID *)(UINTN)PcdGet64 (PcdFfaTxBuffer);
  }

  if (TxBufferSize != NULL) {
    *TxBufferSize = PcdGet64 (PcdFfaTxRxPageCount) * EFI_PAGE_SIZE;
  }

  if (RxBuffer != NULL) {
    *RxBuffer = (VOID *)(UINTN)PcdGet64 (PcdFfaRxBuffer);
  }

  if (RxBufferSize != NULL) {
    *RxBufferSize = PcdGet64 (PcdFfaTxRxPageCount) * EFI_PAGE_SIZE;
  }

  return EFI_SUCCESS;
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
  EFI_STATUS    Status;
  ARM_FFA_ARGS  FfaArgs;
  UINTN         MinSizeAndAlign;
  UINTN         Property2;
  VOID          *Buffers;
  VOID          *TxBuffer;
  VOID          *RxBuffer;
  UINT64        BufferSize;

  TxBuffer   = (VOID *)(UINTN)PcdGet64 (PcdFfaTxBuffer);
  RxBuffer   = (VOID *)(UINTN)PcdGet64 (PcdFfaRxBuffer);
  BufferSize = PcdGet64 (PcdFfaTxRxPageCount) * EFI_PAGE_SIZE;

  /*
   * If someone already mapped Rx/Tx Buffers, return EFI_ALREADY_STARTED.
   * return EFI_ALREADY_STARTED.
   */
  if ((TxBuffer != NULL) && (RxBuffer != NULL)) {
    return EFI_ALREADY_STARTED;
  }

  Status = ArmFfaLibFeatures (ARM_FID_FFA_RXTX_MAP, 0x00, &MinSizeAndAlign, &Property2);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get RX/TX buffer property... Status: %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  ZeroMem (&FfaArgs, sizeof (ARM_FFA_ARGS));

  switch (MinSizeAndAlign) {
    case ARM_FFA_BUFFER_MINSIZE_AND_ALIGN_4K:
      MinSizeAndAlign = SIZE_4KB;
      break;
    case ARM_FFA_BUFFER_MINSIZE_AND_ALIGN_16K:
      MinSizeAndAlign = SIZE_16KB;
      break;
    case ARM_FFA_BUFFER_MINSIZE_AND_ALIGN_64K:
      MinSizeAndAlign = SIZE_64KB;
      break;
    default:
      DEBUG ((DEBUG_ERROR, "%a: Invalid MinSizeAndAlign: 0x%x\n", __func__, MinSizeAndAlign));
      return EFI_UNSUPPORTED;
  }

  if (MinSizeAndAlign > (PcdGet64 (PcdFfaTxRxPageCount) * EFI_PAGE_SIZE)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Buffer is too small! MinSize: 0x%x, PageCount: %d\n",
      __func__,
      MinSizeAndAlign,
      PcdGet64 (PcdFfaTxRxPageCount)
      ));
    return EFI_INVALID_PARAMETER;
  }

  Buffers = AllocateAlignedPages ((PcdGet64 (PcdFfaTxRxPageCount) * 2), MinSizeAndAlign);
  if (Buffers == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorHandler;
  }

  TxBuffer = Buffers;
  RxBuffer = Buffers + BufferSize;

  FfaArgs.Arg0 = ARM_FID_FFA_RXTX_MAP;
  FfaArgs.Arg1 = (UINTN)TxBuffer;
  FfaArgs.Arg2 = (UINTN)RxBuffer;
  FfaArgs.Arg3 = PcdGet64 (PcdFfaTxRxPageCount) / EFI_SIZE_TO_PAGES (MinSizeAndAlign);

  ArmCallFfa (&FfaArgs);

  if (IsFfaError (&FfaArgs)) {
    Status = FfaStatusToEfiStatus (FfaArgs.Arg2);
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to map Rx/Tx buffer. Status: %r\n",
      __func__,
      Status
      ));
    goto ErrorHandler;
  }

  PcdSet64S (PcdFfaTxBuffer, (UINTN)TxBuffer);
  PcdSet64S (PcdFfaRxBuffer, (UINTN)RxBuffer);

  return EFI_SUCCESS;

ErrorHandler:
  if (Buffers != NULL) {
    FreeAlignedPages (Buffers, (PcdGet64 (PcdFfaTxRxPageCount) * 2));
    TxBuffer = NULL;
    RxBuffer = NULL;
  }

  return Status;
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
  ARM_FFA_ARGS  FfaArgs;
  VOID          *Buffers;

  ZeroMem (&FfaArgs, sizeof (ARM_FFA_ARGS));

  FfaArgs.Arg0 = ARM_FID_FFA_RXTX_UNMAP;
  FfaArgs.Arg1 = (gPartId << ARM_FFA_SOURCE_EP_SHIFT);

  ArmCallFfa (&FfaArgs);

  if (IsFfaError (&FfaArgs)) {
    return FfaStatusToEfiStatus (FfaArgs.Arg2);
  }

  /*
   * Rx/Tx Buffer are allocated with continuous pages.
   * and start address of these pages is set on PcdFfaTxBuffer.
   * See ArmFfaLibRxTxMap().
   */
  Buffers = (VOID *)(UINTN)PcdGet64 (PcdFfaTxBuffer);
  if (Buffers != NULL) {
    FreeAlignedPages (Buffers, (PcdGet64 (PcdFfaTxRxPageCount) * 2));
  }

  PcdSet64S (PcdFfaTxBuffer, 0x00);
  PcdSet64S (PcdFfaRxBuffer, 0x00);

  return EFI_SUCCESS;
}
