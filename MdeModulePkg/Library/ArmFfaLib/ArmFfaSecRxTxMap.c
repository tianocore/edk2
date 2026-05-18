/** @file
  Arm Ffa library common code.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
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
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <IndustryStandard/ArmFfaSvc.h>

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
  EFI_HOB_MEMORY_ALLOCATION  *RxTxBufferAllocationHob;
  UINT64                     BufferSize;

  /*
   * Attempt to find the allocation HOB, if it exists then the
   * Rx/Tx buffers have already been mapped.
   */
  RxTxBufferAllocationHob = FindRxTxBufferAllocationHob (TRUE);
  if (RxTxBufferAllocationHob == NULL) {
    return EFI_NOT_READY;
  }

  BufferSize = EFI_PAGES_TO_SIZE (PcdGet64 (PcdFfaTxRxPageCount));

  if (TxBuffer != NULL) {
    *TxBuffer = (VOID *)RxTxBufferAllocationHob->AllocDescriptor.MemoryBaseAddress;
  }

  if (TxBufferSize != NULL) {
    *TxBufferSize = BufferSize;
  }

  if (RxBuffer != NULL) {
    *RxBuffer = (VOID *)RxTxBufferAllocationHob->AllocDescriptor.MemoryBaseAddress + BufferSize;
  }

  if (RxBufferSize != NULL) {
    *RxBufferSize = BufferSize;
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
  EFI_STATUS                 Status;
  ARM_FFA_ARGS               FfaArgs;
  UINTN                      Property1;
  UINTN                      Property2;
  UINTN                      MinSizeAndAlign;
  UINTN                      MaxSize;
  VOID                       *Buffers;
  VOID                       *TxBuffer;
  VOID                       *RxBuffer;
  ARM_FFA_RX_TX_BUFFER_INFO  *BufferInfo;
  EFI_HOB_MEMORY_ALLOCATION  *RxTxBufferAllocationHob;
  UINT16                     PartId;

  /*
   * Attempt to find the allocation HOB, if it exists then the
   * Rx/Tx buffers have already been mapped.
   */
  RxTxBufferAllocationHob = FindRxTxBufferAllocationHob (TRUE);
  if (RxTxBufferAllocationHob != NULL) {
    return EFI_ALREADY_STARTED;
  }

  Status = ArmFfaLibGetFeatures (
             ARM_FID_FFA_RXTX_MAP,
             FFA_RXTX_MAP_INPUT_PROPERTY_DEFAULT,
             &Property1,
             &Property2
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get Rx/Tx buffer property... Status: %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  ZeroMem (&FfaArgs, sizeof (ARM_FFA_ARGS));

  MinSizeAndAlign =
    ((Property1 >>
      ARM_FFA_BUFFER_MINSIZE_AND_ALIGN_SHIFT) &
     ARM_FFA_BUFFER_MINSIZE_AND_ALIGN_MASK);

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

  MaxSize =
    (((Property1 >>
       ARM_FFA_BUFFER_MAXSIZE_PAGE_COUNT_SHIFT) &
      ARM_FFA_BUFFER_MAXSIZE_PAGE_COUNT_MASK));

  MaxSize = ((MaxSize == 0) ? MAX_UINTN : (MaxSize * MinSizeAndAlign));

  if ((MinSizeAndAlign > (EFI_PAGES_TO_SIZE (PcdGet64 (PcdFfaTxRxPageCount)))) ||
      (MaxSize < (EFI_PAGES_TO_SIZE (PcdGet64 (PcdFfaTxRxPageCount)))))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Buffer is too small! MinSize: 0x%x, MaxSize: 0x%x, PageCount: %d\n",
      __func__,
      MinSizeAndAlign,
      MaxSize,
      PcdGet64 (PcdFfaTxRxPageCount)
      ));
    return EFI_INVALID_PARAMETER;
  }

  Buffers = AllocateAlignedPages ((PcdGet64 (PcdFfaTxRxPageCount) * 2), MinSizeAndAlign);
  if (Buffers == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TxBuffer = Buffers;
  RxBuffer = Buffers + (EFI_PAGES_TO_SIZE (PcdGet64 (PcdFfaTxRxPageCount)));

  FfaArgs.Arg0 = ARM_FID_FFA_RXTX_MAP;
  FfaArgs.Arg1 = (UINTN)TxBuffer;
  FfaArgs.Arg2 = (UINTN)RxBuffer;

  /*
   * PcdFfaTxRxPageCount sets with count of EFI_PAGE_SIZE granularity
   * But, PageCounts for Tx/Rx buffer should set with
   * count of Tx/Rx Buffer's MinSizeAndAlign. granularity.
   */
  FfaArgs.Arg3 = PcdGet64 (PcdFfaTxRxPageCount) / EFI_SIZE_TO_PAGES (MinSizeAndAlign);

  ArmCallFfa (&FfaArgs);

  Status = FfaArgsToEfiStatus (&FfaArgs);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to map Rx/Tx buffer. Status: %r\n",
      __func__,
      Status
      ));
    goto ErrorHandler;
  }

  BufferInfo = BuildGuidHob (&gArmFfaRxTxBufferInfoGuid, sizeof (ARM_FFA_RX_TX_BUFFER_INFO));
  if (BufferInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to create Rx/Tx Buffer Info Hob\n", __func__));
    goto Unmap;
  }

  RxTxBufferAllocationHob = GetRxTxBufferAllocationHob ((EFI_PHYSICAL_ADDRESS)((UINTN)TxBuffer), EFI_PAGES_TO_SIZE (PcdGet64 (PcdFfaTxRxPageCount)) * 2, FALSE);
  if (RxTxBufferAllocationHob == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to acquire RxTxBufferAllocationHob\n", __func__));
    goto Unmap;
  }

  /*
   * Set then Name with gArmFfaRxTxBufferInfoGuid, so that ArmFfaPeiLib or
   * ArmFfaDxeLib can find the Rx/Tx buffer allocation area.
   */
  CopyGuid (&RxTxBufferAllocationHob->AllocDescriptor.Name, &gArmFfaRxTxBufferInfoGuid);

  BufferInfo->TxBufferAddr  = (UINTN)RxTxBufferAllocationHob->AllocDescriptor.MemoryBaseAddress;
  BufferInfo->TxBufferSize  = EFI_PAGES_TO_SIZE (PcdGet64 (PcdFfaTxRxPageCount));
  BufferInfo->RxBufferAddr  = (UINTN)RxTxBufferAllocationHob->AllocDescriptor.MemoryBaseAddress + BufferInfo->TxBufferSize;
  BufferInfo->RxBufferSize  = EFI_PAGES_TO_SIZE (PcdGet64 (PcdFfaTxRxPageCount));
  BufferInfo->RemapOffset   = (UINTN)(BufferInfo->TxBufferAddr - RxTxBufferAllocationHob->AllocDescriptor.MemoryBaseAddress);
  BufferInfo->RemapRequired = TRUE;

  return EFI_SUCCESS;

Unmap:
  Status = ArmFfaLibPartitionIdGet (&PartId);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, Failed to acquire partition ID. Status: %r\n", __func__, Status));
    goto ErrorHandler;
  }

  ZeroMem (&FfaArgs, sizeof (ARM_FFA_ARGS));

  FfaArgs.Arg0 = ARM_FID_FFA_RXTX_UNMAP;
  FfaArgs.Arg1 = (PartId << ARM_FFA_SOURCE_EP_SHIFT);

  ArmCallFfa (&FfaArgs);

  Status = FfaArgsToEfiStatus (&FfaArgs);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, Failed to unmap Rx/Tx buffer. Status: %r\n", __func__, Status));
    goto ErrorHandler;
  }

  /*
   * If we successfully unmap, we only got here if we were
   * unable to build the HOB or find the allocation HOB. Due
   * to Status being overwritten we still need to indicate a
   * failure occurred.
   */
  Status = EFI_OUT_OF_RESOURCES;

ErrorHandler:
  FreePages (Buffers, (PcdGet64 (PcdFfaTxRxPageCount) * 2));

  return Status;
}

/**
  Unmap Rx/Tx buffer.
  This function is only called in Exit boot service because
  Rx/Tx buffer is registered only once per partition.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER  Already unregistered
  @retval EFI_UNSUPPORTED        Not supported

**/
EFI_STATUS
EFIAPI
ArmFfaLibRxTxUnmap (
  IN VOID
  )
{
  EFI_STATUS                 Status;
  ARM_FFA_ARGS               FfaArgs;
  VOID                       *Buffers;
  UINT16                     PartId;
  EFI_HOB_MEMORY_ALLOCATION  *RxTxBufferAllocationHob;

  /*
   * Rx/Tx buffers are allocated with continuous pages.
   * See ArmFfaLibRxTxMap(). If HOB is not found, the Rx/Tx
   * buffers were not successfully mapped.
   */
  RxTxBufferAllocationHob = FindRxTxBufferAllocationHob (TRUE);
  if (RxTxBufferAllocationHob == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = ArmFfaLibPartitionIdGet (&PartId);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, Failed to acquire partition ID. Status: %r\n", __func__, Status));
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&FfaArgs, sizeof (ARM_FFA_ARGS));

  FfaArgs.Arg0 = ARM_FID_FFA_RXTX_UNMAP;
  FfaArgs.Arg1 = (PartId << ARM_FFA_SOURCE_EP_SHIFT);

  ArmCallFfa (&FfaArgs);

  Status = FfaArgsToEfiStatus (&FfaArgs);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Buffers = (VOID *)RxTxBufferAllocationHob->AllocDescriptor.MemoryBaseAddress;
  if (Buffers != NULL) {
    FreePages (Buffers, (PcdGet64 (PcdFfaTxRxPageCount) * 2));
  }

  UnmapCallback ();

  return EFI_SUCCESS;
}

/**
  Update Rx/Tx buffer information.

  @param  BufferInfo  Rx/Tx buffer information.

**/
VOID
EFIAPI
UpdateRxTxBufferInfo (
  OUT ARM_FFA_RX_TX_BUFFER_INFO  *BufferInfo
  )
{
  EFI_HOB_MEMORY_ALLOCATION  *RxTxBufferAllocationHob;

  /*
   * Attempt to find the HOB containing the Rx/Tx buffer information.
   * If it is not found, the Rx/Tx buffers were not successfully mapped.
   */
  RxTxBufferAllocationHob = FindRxTxBufferAllocationHob (TRUE);

  if ((RxTxBufferAllocationHob != NULL) && (BufferInfo != NULL)) {
    BufferInfo->TxBufferAddr = (UINTN)RxTxBufferAllocationHob->AllocDescriptor.MemoryBaseAddress;
    BufferInfo->TxBufferSize = EFI_PAGES_TO_SIZE (PcdGet64 (PcdFfaTxRxPageCount));
    BufferInfo->RxBufferAddr = (UINTN)RxTxBufferAllocationHob->AllocDescriptor.MemoryBaseAddress + BufferInfo->TxBufferSize;
    BufferInfo->RxBufferSize = EFI_PAGES_TO_SIZE (PcdGet64 (PcdFfaTxRxPageCount));
  }
}

/**
  Find Rx/Tx buffer memory allocation hob.

  @param  UseGuid             Find MemoryAllocationHob using Guid.

  @retval MemoryAllocationHob
  @retval NULL                No memory allocation hob related to Rx/Tx buffer

**/
EFI_HOB_MEMORY_ALLOCATION *
EFIAPI
FindRxTxBufferAllocationHob (
  IN BOOLEAN  UseGuid
  )
{
  (void)UseGuid;

  /*
   * Mapping the Rx/Tx buffer should have generated the HOB, if not,
   * then the allocation HOB will not be found and NULL will be returned.
   * Note that BufferAddr and BufferSize are ignored when UseGuid is TRUE.
   */
  return GetRxTxBufferAllocationHob (0, 0, TRUE);
}

/**
  Remap Rx/Tx buffer with converted Rx/Tx Buffer address after
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
   * SEC binary shouldn't remap the Rx/Tx Buffer.
   */
  return EFI_UNSUPPORTED;
}
