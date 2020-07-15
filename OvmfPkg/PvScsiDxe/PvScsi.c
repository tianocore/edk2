/** @file

  This driver produces Extended SCSI Pass Thru Protocol instances for
  pvscsi devices.

  Copyright (C) 2020, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/PvScsi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/PciIo.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Uefi/UefiSpec.h>

#include "PvScsi.h"

//
// Higher versions will be used before lower, 0x10-0xffffffef is the version
// range for IHV (Indie Hardware Vendors)
//
#define PVSCSI_BINDING_VERSION      0x10

//
// Ext SCSI Pass Thru utilities
//

/**
  Reads a 32-bit value into BAR0 using MMIO
**/
STATIC
EFI_STATUS
PvScsiMmioRead32 (
  IN CONST PVSCSI_DEV   *Dev,
  IN UINT64             Offset,
  OUT UINT32            *Value
  )
{
  return Dev->PciIo->Mem.Read (
                           Dev->PciIo,
                           EfiPciIoWidthUint32,
                           PCI_BAR_IDX0,
                           Offset,
                           1,   // Count
                           Value
                           );
}

/**
  Writes a 32-bit value into BAR0 using MMIO
**/
STATIC
EFI_STATUS
PvScsiMmioWrite32 (
  IN CONST PVSCSI_DEV   *Dev,
  IN UINT64             Offset,
  IN UINT32             Value
  )
{
  return Dev->PciIo->Mem.Write (
                           Dev->PciIo,
                           EfiPciIoWidthUint32,
                           PCI_BAR_IDX0,
                           Offset,
                           1,   // Count
                           &Value
                           );
}

/**
  Writes multiple words of data into BAR0 using MMIO
**/
STATIC
EFI_STATUS
PvScsiMmioWrite32Multiple (
  IN CONST PVSCSI_DEV   *Dev,
  IN UINT64             Offset,
  IN UINTN              Count,
  IN UINT32             *Words
  )
{
  return Dev->PciIo->Mem.Write (
                           Dev->PciIo,
                           EfiPciIoWidthFifoUint32,
                           PCI_BAR_IDX0,
                           Offset,
                           Count,
                           Words
                           );
}

/**
  Send a PVSCSI command to device.

  @param[in] Dev                    The pvscsi host device.
  @param[in] Cmd                    The command to send to device.
  @param[in] OPTIONAL DescWords     An optional command descriptor (If command
                                    have a descriptor). The descriptor is
                                    provided as an array of UINT32 words and
                                    is must be 32-bit aligned.
  @param[in] DescWordsCount         The number of words in command descriptor.
                                    Caller must specify here 0 if DescWords
                                    is not supplied (It is optional). In that
                                    case, DescWords is ignored.

  @return   Status codes returned by Dev->PciIo->Mem.Write().

**/
STATIC
EFI_STATUS
PvScsiWriteCmdDesc (
  IN CONST PVSCSI_DEV   *Dev,
  IN UINT32             Cmd,
  IN UINT32             *DescWords      OPTIONAL,
  IN UINTN              DescWordsCount
  )
{
  EFI_STATUS Status;

  if (DescWordsCount > PVSCSI_MAX_CMD_DATA_WORDS) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PvScsiMmioWrite32 (Dev, PvScsiRegOffsetCommand, Cmd);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (DescWordsCount > 0) {
    return PvScsiMmioWrite32Multiple (
             Dev,
             PvScsiRegOffsetCommandData,
             DescWordsCount,
             DescWords
             );
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
PvScsiResetAdapter (
  IN CONST PVSCSI_DEV   *Dev
  )
{
  return PvScsiWriteCmdDesc (Dev, PvScsiCmdAdapterReset, NULL, 0);
}

/**
  Returns if PVSCSI request ring is full
**/
STATIC
BOOLEAN
PvScsiIsReqRingFull (
  IN CONST PVSCSI_DEV   *Dev
  )
{
  PVSCSI_RINGS_STATE *RingsState;
  UINT32             ReqNumEntries;

  RingsState = Dev->RingDesc.RingState;
  ReqNumEntries = 1U << RingsState->ReqNumEntriesLog2;
  return (RingsState->ReqProdIdx - RingsState->CmpConsIdx) >= ReqNumEntries;
}

/**
  Returns pointer to current request descriptor to produce
**/
STATIC
PVSCSI_RING_REQ_DESC *
PvScsiGetCurrentRequest (
  IN CONST PVSCSI_DEV   *Dev
  )
{
  PVSCSI_RINGS_STATE *RingState;
  UINT32             ReqNumEntries;

  RingState = Dev->RingDesc.RingState;
  ReqNumEntries = 1U << RingState->ReqNumEntriesLog2;
  return Dev->RingDesc.RingReqs +
         (RingState->ReqProdIdx & (ReqNumEntries - 1));
}

/**
  Returns pointer to current completion descriptor to consume
**/
STATIC
PVSCSI_RING_CMP_DESC *
PvScsiGetCurrentResponse (
  IN CONST PVSCSI_DEV   *Dev
  )
{
  PVSCSI_RINGS_STATE *RingState;
  UINT32             CmpNumEntries;

  RingState = Dev->RingDesc.RingState;
  CmpNumEntries = 1U << RingState->CmpNumEntriesLog2;
  return Dev->RingDesc.RingCmps +
         (RingState->CmpConsIdx & (CmpNumEntries - 1));
}

/**
  Wait for device to signal completion of submitted requests
**/
STATIC
EFI_STATUS
PvScsiWaitForRequestCompletion (
  IN CONST PVSCSI_DEV   *Dev
  )
{
  EFI_STATUS Status;
  UINT32     IntrStatus;

  //
  // Note: We don't yet support Timeout according to
  // EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET.Timeout.
  //
  // This is consistent with some other Scsi PassThru drivers
  // such as VirtioScsi.
  //
  for (;;) {
    Status = PvScsiMmioRead32 (Dev, PvScsiRegOffsetIntrStatus, &IntrStatus);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // PVSCSI_INTR_CMPL_MASK is set if device completed submitted requests
    //
    if ((IntrStatus & PVSCSI_INTR_CMPL_MASK) != 0) {
      break;
    }

    gBS->Stall (Dev->WaitForCmpStallInUsecs);
  }

  //
  // Acknowledge PVSCSI_INTR_CMPL_MASK in device interrupt-status register
  //
  return PvScsiMmioWrite32 (
           Dev,
           PvScsiRegOffsetIntrStatus,
           PVSCSI_INTR_CMPL_MASK
           );
}

/**
  Create a fake host adapter error
**/
STATIC
EFI_STATUS
ReportHostAdapterError (
  OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET *Packet
  )
{
  Packet->InTransferLength = 0;
  Packet->OutTransferLength = 0;
  Packet->SenseDataLength = 0;
  Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_OTHER;
  Packet->TargetStatus = EFI_EXT_SCSI_STATUS_TARGET_GOOD;
  return EFI_DEVICE_ERROR;
}

/**
  Create a fake host adapter overrun error
**/
STATIC
EFI_STATUS
ReportHostAdapterOverrunError (
  OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET *Packet
  )
{
  Packet->SenseDataLength = 0;
  Packet->HostAdapterStatus =
            EFI_EXT_SCSI_STATUS_HOST_ADAPTER_DATA_OVERRUN_UNDERRUN;
  Packet->TargetStatus = EFI_EXT_SCSI_STATUS_TARGET_GOOD;
  return EFI_BAD_BUFFER_SIZE;
}

/**
  Populate a PVSCSI request descriptor from the Extended SCSI Pass Thru
  Protocol packet.
**/
STATIC
EFI_STATUS
PopulateRequest (
  IN CONST PVSCSI_DEV                               *Dev,
  IN UINT8                                          *Target,
  IN UINT64                                         Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET *Packet,
  OUT PVSCSI_RING_REQ_DESC                          *Request
  )
{
  UINT8 TargetValue;

  //
  // We only use first byte of target identifer
  //
  TargetValue = *Target;

  //
  // Check for unsupported requests
  //
  if (
      //
      // Bidirectional transfer was requested
      //
      (Packet->InTransferLength > 0 && Packet->OutTransferLength > 0) ||
      (Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_BIDIRECTIONAL) ||
      //
      // Command Descriptor Block bigger than this constant should be considered
      // out-of-band. We currently don't support these CDBs.
      //
      (Packet->CdbLength > PVSCSI_CDB_MAX_SIZE)
      ) {

    //
    // This error code doesn't require updates to the Packet output fields
    //
    return EFI_UNSUPPORTED;
  }

  //
  // Check for invalid parameters
  //
  if (
      //
      // Addressed invalid device
      //
      (TargetValue > Dev->MaxTarget) || (Lun > Dev->MaxLun) ||
      //
      // Invalid direction (there doesn't seem to be a macro for the "no data
      // transferred" "direction", eg. for TEST UNIT READY)
      //
      (Packet->DataDirection > EFI_EXT_SCSI_DATA_DIRECTION_BIDIRECTIONAL) ||
      //
      // Trying to receive, but destination pointer is NULL, or contradicting
      // transfer direction
      //
      ((Packet->InTransferLength > 0) &&
       ((Packet->InDataBuffer == NULL) ||
        (Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_WRITE)
        )
       ) ||
      //
      // Trying to send, but source pointer is NULL, or contradicting
      // transfer direction
      //
      ((Packet->OutTransferLength > 0) &&
       ((Packet->OutDataBuffer == NULL) ||
        (Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_READ)
        )
       )
      ) {

    //
    // This error code doesn't require updates to the Packet output fields
    //
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check for input/output buffer too large for DMA communication buffer
  //
  if (Packet->InTransferLength > sizeof (Dev->DmaBuf->Data)) {
    Packet->InTransferLength = sizeof (Dev->DmaBuf->Data);
    return ReportHostAdapterOverrunError (Packet);
  }
  if (Packet->OutTransferLength > sizeof (Dev->DmaBuf->Data)) {
    Packet->OutTransferLength = sizeof (Dev->DmaBuf->Data);
    return ReportHostAdapterOverrunError (Packet);
  }

  //
  // Encode PVSCSI request
  //
  ZeroMem (Request, sizeof (*Request));

  Request->Bus = 0;
  Request->Target = TargetValue;
  //
  // This cast is safe as PVSCSI_DEV.MaxLun is defined as UINT8
  //
  Request->Lun[1] = (UINT8)Lun;
  Request->SenseLen = Packet->SenseDataLength;
  //
  // DMA communication buffer SenseData overflow is not possible
  // due to Packet->SenseDataLength defined as UINT8
  //
  Request->SenseAddr = PVSCSI_DMA_BUF_DEV_ADDR (Dev, SenseData);
  Request->CdbLen = Packet->CdbLength;
  CopyMem (Request->Cdb, Packet->Cdb, Packet->CdbLength);
  Request->VcpuHint = 0;
  Request->Tag = PVSCSI_SIMPLE_QUEUE_TAG;
  if (Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_READ) {
    Request->Flags = PVSCSI_FLAG_CMD_DIR_TOHOST;
    Request->DataLen = Packet->InTransferLength;
  } else {
    Request->Flags = PVSCSI_FLAG_CMD_DIR_TODEVICE;
    Request->DataLen = Packet->OutTransferLength;
    CopyMem (
      Dev->DmaBuf->Data,
      Packet->OutDataBuffer,
      Packet->OutTransferLength
      );
  }
  Request->DataAddr = PVSCSI_DMA_BUF_DEV_ADDR (Dev, Data);

  return EFI_SUCCESS;
}

/**
  Handle the PVSCSI device response:
  - Copy returned data from DMA communication buffer.
  - Update fields in Extended SCSI Pass Thru Protocol packet as required.
  - Translate response code to EFI status code and host adapter status.
**/
STATIC
EFI_STATUS
HandleResponse (
  IN PVSCSI_DEV                                     *Dev,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET *Packet,
  IN CONST PVSCSI_RING_CMP_DESC                     *Response
  )
{
  //
  // Fix SenseDataLength to amount of data returned
  //
  if (Packet->SenseDataLength > Response->SenseLen) {
    Packet->SenseDataLength = (UINT8)Response->SenseLen;
  }
  //
  // Copy sense data from DMA communication buffer
  //
  CopyMem (
    Packet->SenseData,
    Dev->DmaBuf->SenseData,
    Packet->SenseDataLength
    );

  //
  // Copy device output from DMA communication buffer
  //
  if (Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_READ) {
    CopyMem (Packet->InDataBuffer, Dev->DmaBuf->Data, Packet->InTransferLength);
  }

  //
  // Report target status
  // (Strangely, PVSCSI interface defines Response->ScsiStatus as UINT16.
  // But it should de-facto always have a value that fits UINT8. To avoid
  // unexpected behavior, verify value is in UINT8 bounds before casting)
  //
  ASSERT (Response->ScsiStatus <= MAX_UINT8);
  Packet->TargetStatus = (UINT8)Response->ScsiStatus;

  //
  // Host adapter status and function return value depend on
  // device response's host status
  //
  switch (Response->HostStatus) {
    case PvScsiBtStatSuccess:
    case PvScsiBtStatLinkedCommandCompleted:
    case PvScsiBtStatLinkedCommandCompletedWithFlag:
      Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_OK;
      return EFI_SUCCESS;

    case PvScsiBtStatDataUnderrun:
      //
      // Report transferred amount in underrun
      //
      if (Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_READ) {
        Packet->InTransferLength = (UINT32)Response->DataLen;
      } else {
        Packet->OutTransferLength = (UINT32)Response->DataLen;
      }
      Packet->HostAdapterStatus =
                EFI_EXT_SCSI_STATUS_HOST_ADAPTER_DATA_OVERRUN_UNDERRUN;
      return EFI_SUCCESS;

    case PvScsiBtStatDatarun:
      Packet->HostAdapterStatus =
                EFI_EXT_SCSI_STATUS_HOST_ADAPTER_DATA_OVERRUN_UNDERRUN;
      return EFI_SUCCESS;

    case PvScsiBtStatSelTimeout:
      Packet->HostAdapterStatus =
                EFI_EXT_SCSI_STATUS_HOST_ADAPTER_SELECTION_TIMEOUT;
      return EFI_TIMEOUT;

    case PvScsiBtStatBusFree:
      Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_BUS_FREE;
      break;

    case PvScsiBtStatInvPhase:
      Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_PHASE_ERROR;
      break;

    case PvScsiBtStatSensFailed:
      Packet->HostAdapterStatus =
                EFI_EXT_SCSI_STATUS_HOST_ADAPTER_REQUEST_SENSE_FAILED;
      break;

    case PvScsiBtStatTagReject:
    case PvScsiBtStatBadMsg:
      Packet->HostAdapterStatus =
          EFI_EXT_SCSI_STATUS_HOST_ADAPTER_MESSAGE_REJECT;
      break;

    case PvScsiBtStatBusReset:
      Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_BUS_RESET;
      break;

    case PvScsiBtStatHaTimeout:
      Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_TIMEOUT;
      return EFI_TIMEOUT;

    case PvScsiBtStatScsiParity:
      Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_PARITY_ERROR;
      break;

    default:
      Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_OTHER;
      break;
  }

  return EFI_DEVICE_ERROR;
}

/**
  Check if Target argument to EXT_SCSI_PASS_THRU.GetNextTarget() and
  EXT_SCSI_PASS_THRU.GetNextTargetLun() is initialized
**/
STATIC
BOOLEAN
IsTargetInitialized (
  IN UINT8                                          *Target
  )
{
  UINTN Idx;

  for (Idx = 0; Idx < TARGET_MAX_BYTES; ++Idx) {
    if (Target[Idx] != 0xFF) {
      return TRUE;
    }
  }
  return FALSE;
}

//
// Ext SCSI Pass Thru
//

STATIC
EFI_STATUS
EFIAPI
PvScsiPassThru (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN UINT8                                          *Target,
  IN UINT64                                         Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET *Packet,
  IN EFI_EVENT                                      Event    OPTIONAL
  )
{
  PVSCSI_DEV            *Dev;
  EFI_STATUS            Status;
  PVSCSI_RING_REQ_DESC *Request;
  PVSCSI_RING_CMP_DESC *Response;

  Dev = PVSCSI_FROM_PASS_THRU (This);

  if (PvScsiIsReqRingFull (Dev)) {
    return EFI_NOT_READY;
  }

  Request = PvScsiGetCurrentRequest (Dev);

  Status = PopulateRequest (Dev, Target, Lun, Packet, Request);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Writes to Request must be globally visible before making request
  // available to device
  //
  MemoryFence ();
  Dev->RingDesc.RingState->ReqProdIdx++;

  Status = PvScsiMmioWrite32 (Dev, PvScsiRegOffsetKickRwIo, 0);
  if (EFI_ERROR (Status)) {
    //
    // If kicking the host fails, we must fake a host adapter error.
    // EFI_NOT_READY would save us the effort, but it would also suggest that
    // the caller retry.
    //
    return ReportHostAdapterError (Packet);
  }

  Status = PvScsiWaitForRequestCompletion (Dev);
  if (EFI_ERROR (Status)) {
    //
    // If waiting for request completion fails, we must fake a host adapter
    // error. EFI_NOT_READY would save us the effort, but it would also suggest
    // that the caller retry.
    //
    return ReportHostAdapterError (Packet);
  }

  Response = PvScsiGetCurrentResponse (Dev);
  Status = HandleResponse (Dev, Packet, Response);

  //
  // Reads from response must complete before releasing completion entry
  // to device
  //
  MemoryFence ();
  Dev->RingDesc.RingState->CmpConsIdx++;

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiGetNextTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN OUT UINT8                                      **Target,
  IN OUT UINT64                                     *Lun
  )
{
  UINT8      *TargetPtr;
  UINT8      LastTarget;
  PVSCSI_DEV *Dev;

  if (Target == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The Target input parameter is unnecessarily a pointer-to-pointer
  //
  TargetPtr = *Target;

  //
  // If target not initialized, return first target & LUN
  //
  if (!IsTargetInitialized (TargetPtr)) {
    ZeroMem (TargetPtr, TARGET_MAX_BYTES);
    *Lun = 0;
    return EFI_SUCCESS;
  }

  //
  // We only use first byte of target identifer
  //
  LastTarget = *TargetPtr;

  //
  // Increment (target, LUN) pair if valid on input
  //
  Dev = PVSCSI_FROM_PASS_THRU (This);
  if (LastTarget > Dev->MaxTarget || *Lun > Dev->MaxLun) {
    return EFI_INVALID_PARAMETER;
  }

  if (*Lun < Dev->MaxLun) {
    ++*Lun;
    return EFI_SUCCESS;
  }

  if (LastTarget < Dev->MaxTarget) {
    *Lun = 0;
    ++LastTarget;
    *TargetPtr = LastTarget;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiBuildDevicePath (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN UINT8                                          *Target,
  IN UINT64                                         Lun,
  IN OUT EFI_DEVICE_PATH_PROTOCOL                   **DevicePath
  )
{
  UINT8             TargetValue;
  PVSCSI_DEV        *Dev;
  SCSI_DEVICE_PATH  *ScsiDevicePath;

  if (DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // We only use first byte of target identifer
  //
  TargetValue = *Target;

  Dev = PVSCSI_FROM_PASS_THRU (This);
  if (TargetValue > Dev->MaxTarget || Lun > Dev->MaxLun) {
    return EFI_NOT_FOUND;
  }

  ScsiDevicePath = AllocatePool (sizeof (*ScsiDevicePath));
  if (ScsiDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ScsiDevicePath->Header.Type      = MESSAGING_DEVICE_PATH;
  ScsiDevicePath->Header.SubType   = MSG_SCSI_DP;
  ScsiDevicePath->Header.Length[0] = (UINT8)sizeof (*ScsiDevicePath);
  ScsiDevicePath->Header.Length[1] = (UINT8)(sizeof (*ScsiDevicePath) >> 8);
  ScsiDevicePath->Pun              = TargetValue;
  ScsiDevicePath->Lun              = (UINT16)Lun;

  *DevicePath = &ScsiDevicePath->Header;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiGetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN EFI_DEVICE_PATH_PROTOCOL                       *DevicePath,
  OUT UINT8                                         **Target,
  OUT UINT64                                        *Lun
  )
{
  SCSI_DEVICE_PATH *ScsiDevicePath;
  PVSCSI_DEV       *Dev;

  if (DevicePath == NULL || Target == NULL || *Target == NULL || Lun == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (DevicePath->Type    != MESSAGING_DEVICE_PATH ||
      DevicePath->SubType != MSG_SCSI_DP) {
    return EFI_UNSUPPORTED;
  }

  ScsiDevicePath = (SCSI_DEVICE_PATH *)DevicePath;
  Dev = PVSCSI_FROM_PASS_THRU (This);
  if (ScsiDevicePath->Pun > Dev->MaxTarget ||
      ScsiDevicePath->Lun > Dev->MaxLun) {
    return EFI_NOT_FOUND;
  }

  //
  // We only use first byte of target identifer
  //
  **Target = (UINT8)ScsiDevicePath->Pun;
  ZeroMem (*Target + 1, TARGET_MAX_BYTES - 1);
  *Lun = ScsiDevicePath->Lun;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiResetChannel (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiResetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN UINT8                                          *Target,
  IN UINT64                                         Lun
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiGetNextTarget (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN OUT UINT8                                      **Target
  )
{
  UINT8      *TargetPtr;
  UINT8      LastTarget;
  PVSCSI_DEV *Dev;

  if (Target == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The Target input parameter is unnecessarily a pointer-to-pointer
  //
  TargetPtr = *Target;

  //
  // If target not initialized, return first target
  //
  if (!IsTargetInitialized (TargetPtr)) {
    ZeroMem (TargetPtr, TARGET_MAX_BYTES);
    return EFI_SUCCESS;
  }

  //
  // We only use first byte of target identifer
  //
  LastTarget = *TargetPtr;

  //
  // Increment target if valid on input
  //
  Dev = PVSCSI_FROM_PASS_THRU (This);
  if (LastTarget > Dev->MaxTarget) {
    return EFI_INVALID_PARAMETER;
  }

  if (LastTarget < Dev->MaxTarget) {
    ++LastTarget;
    *TargetPtr = LastTarget;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
PvScsiSetPciAttributes (
  IN OUT PVSCSI_DEV *Dev
  )
{
  EFI_STATUS Status;

  //
  // Backup original PCI Attributes
  //
  Status = Dev->PciIo->Attributes (
                         Dev->PciIo,
                         EfiPciIoAttributeOperationGet,
                         0,
                         &Dev->OriginalPciAttributes
                         );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Enable MMIO-Space & Bus-Mastering
  //
  Status = Dev->PciIo->Attributes (
                         Dev->PciIo,
                         EfiPciIoAttributeOperationEnable,
                         (EFI_PCI_IO_ATTRIBUTE_MEMORY |
                          EFI_PCI_IO_ATTRIBUTE_BUS_MASTER),
                         NULL
                         );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Signal device supports 64-bit DMA addresses
  //
  Status = Dev->PciIo->Attributes (
                         Dev->PciIo,
                         EfiPciIoAttributeOperationEnable,
                         EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE,
                         NULL
                         );
  if (EFI_ERROR (Status)) {
    //
    // Warn user that device will only be using 32-bit DMA addresses.
    //
    // Note that this does not prevent the device/driver from working
    // and therefore we only warn and continue as usual.
    //
    DEBUG ((
      DEBUG_WARN,
      "%a: failed to enable 64-bit DMA addresses\n",
      __FUNCTION__
      ));
  }

  return EFI_SUCCESS;
}

STATIC
VOID
PvScsiRestorePciAttributes (
  IN PVSCSI_DEV *Dev
  )
{
  Dev->PciIo->Attributes (
                Dev->PciIo,
                EfiPciIoAttributeOperationSet,
                Dev->OriginalPciAttributes,
                NULL
                );
}

STATIC
EFI_STATUS
PvScsiAllocateSharedPages (
  IN PVSCSI_DEV                     *Dev,
  IN UINTN                          Pages,
  OUT VOID                          **HostAddress,
  OUT PVSCSI_DMA_DESC               *DmaDesc
  )
{
  EFI_STATUS Status;
  UINTN      NumberOfBytes;

  Status = Dev->PciIo->AllocateBuffer (
                         Dev->PciIo,
                         AllocateAnyPages,
                         EfiBootServicesData,
                         Pages,
                         HostAddress,
                         EFI_PCI_ATTRIBUTE_MEMORY_CACHED
                         );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  NumberOfBytes = EFI_PAGES_TO_SIZE (Pages);
  Status = Dev->PciIo->Map (
                         Dev->PciIo,
                         EfiPciIoOperationBusMasterCommonBuffer,
                         *HostAddress,
                         &NumberOfBytes,
                         &DmaDesc->DeviceAddress,
                         &DmaDesc->Mapping
                         );
  if (EFI_ERROR (Status)) {
    goto FreeBuffer;
  }

  if (NumberOfBytes != EFI_PAGES_TO_SIZE (Pages)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Unmap;
  }

  return EFI_SUCCESS;

Unmap:
  Dev->PciIo->Unmap (Dev->PciIo, DmaDesc->Mapping);

FreeBuffer:
  Dev->PciIo->FreeBuffer (Dev->PciIo, Pages, *HostAddress);

  return Status;
}

STATIC
VOID
PvScsiFreeSharedPages (
  IN PVSCSI_DEV                     *Dev,
  IN UINTN                          Pages,
  IN VOID                           *HostAddress,
  IN PVSCSI_DMA_DESC                *DmaDesc
  )
{
  Dev->PciIo->Unmap (Dev->PciIo, DmaDesc->Mapping);
  Dev->PciIo->FreeBuffer (Dev->PciIo, Pages, HostAddress);
}

STATIC
EFI_STATUS
PvScsiInitRings (
  IN OUT PVSCSI_DEV *Dev
  )
{
  EFI_STATUS Status;

  Status = PvScsiAllocateSharedPages (
             Dev,
             1,
             (VOID **)&Dev->RingDesc.RingState,
             &Dev->RingDesc.RingStateDmaDesc
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  ZeroMem (Dev->RingDesc.RingState, EFI_PAGE_SIZE);

  Status = PvScsiAllocateSharedPages (
             Dev,
             1,
             (VOID **)&Dev->RingDesc.RingReqs,
             &Dev->RingDesc.RingReqsDmaDesc
             );
  if (EFI_ERROR (Status)) {
    goto FreeRingState;
  }
  ZeroMem (Dev->RingDesc.RingReqs, EFI_PAGE_SIZE);

  Status = PvScsiAllocateSharedPages (
             Dev,
             1,
             (VOID **)&Dev->RingDesc.RingCmps,
             &Dev->RingDesc.RingCmpsDmaDesc
             );
  if (EFI_ERROR (Status)) {
    goto FreeRingReqs;
  }
  ZeroMem (Dev->RingDesc.RingCmps, EFI_PAGE_SIZE);

  return EFI_SUCCESS;

FreeRingReqs:
  PvScsiFreeSharedPages (
    Dev,
    1,
    Dev->RingDesc.RingReqs,
    &Dev->RingDesc.RingReqsDmaDesc
    );

FreeRingState:
  PvScsiFreeSharedPages (
    Dev,
    1,
    Dev->RingDesc.RingState,
    &Dev->RingDesc.RingStateDmaDesc
    );

  return Status;
}

STATIC
VOID
PvScsiFreeRings (
  IN OUT PVSCSI_DEV *Dev
  )
{
  PvScsiFreeSharedPages (
    Dev,
    1,
    Dev->RingDesc.RingCmps,
    &Dev->RingDesc.RingCmpsDmaDesc
    );

  PvScsiFreeSharedPages (
    Dev,
    1,
    Dev->RingDesc.RingReqs,
    &Dev->RingDesc.RingReqsDmaDesc
    );

  PvScsiFreeSharedPages (
    Dev,
    1,
    Dev->RingDesc.RingState,
    &Dev->RingDesc.RingStateDmaDesc
    );
}

STATIC
EFI_STATUS
PvScsiSetupRings (
  IN OUT PVSCSI_DEV *Dev
  )
{
  union {
    PVSCSI_CMD_DESC_SETUP_RINGS Cmd;
    UINT32                      Uint32;
  } AlignedCmd;
  PVSCSI_CMD_DESC_SETUP_RINGS *Cmd;

  Cmd = &AlignedCmd.Cmd;

  ZeroMem (Cmd, sizeof (*Cmd));
  Cmd->ReqRingNumPages = 1;
  Cmd->CmpRingNumPages = 1;
  Cmd->RingsStatePPN = RShiftU64 (
                         Dev->RingDesc.RingStateDmaDesc.DeviceAddress,
                         EFI_PAGE_SHIFT
                         );
  Cmd->ReqRingPPNs[0] = RShiftU64 (
                          Dev->RingDesc.RingReqsDmaDesc.DeviceAddress,
                          EFI_PAGE_SHIFT
                          );
  Cmd->CmpRingPPNs[0] = RShiftU64 (
                          Dev->RingDesc.RingCmpsDmaDesc.DeviceAddress,
                          EFI_PAGE_SHIFT
                          );

  STATIC_ASSERT (
    sizeof (*Cmd) % sizeof (UINT32) == 0,
    "Cmd must be multiple of 32-bit words"
    );
  return PvScsiWriteCmdDesc (
           Dev,
           PvScsiCmdSetupRings,
           (UINT32 *)Cmd,
           sizeof (*Cmd) / sizeof (UINT32)
           );
}

STATIC
EFI_STATUS
PvScsiInit (
  IN OUT PVSCSI_DEV *Dev
  )
{
  EFI_STATUS Status;

  //
  // Init configuration
  //
  Dev->MaxTarget = PcdGet8 (PcdPvScsiMaxTargetLimit);
  Dev->MaxLun = PcdGet8 (PcdPvScsiMaxLunLimit);
  Dev->WaitForCmpStallInUsecs = PcdGet32 (PcdPvScsiWaitForCmpStallInUsecs);

  //
  // Set PCI Attributes
  //
  Status = PvScsiSetPciAttributes (Dev);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Reset adapter
  //
  Status = PvScsiResetAdapter (Dev);
  if (EFI_ERROR (Status)) {
    goto RestorePciAttributes;
  }

  //
  // Init PVSCSI rings
  //
  Status = PvScsiInitRings (Dev);
  if (EFI_ERROR (Status)) {
    goto RestorePciAttributes;
  }

  //
  // Allocate DMA communication buffer
  //
  Status = PvScsiAllocateSharedPages (
             Dev,
             EFI_SIZE_TO_PAGES (sizeof (*Dev->DmaBuf)),
             (VOID **)&Dev->DmaBuf,
             &Dev->DmaBufDmaDesc
             );
  if (EFI_ERROR (Status)) {
    goto FreeRings;
  }

  //
  // Setup rings against device
  //
  Status = PvScsiSetupRings (Dev);
  if (EFI_ERROR (Status)) {
    goto FreeDmaCommBuffer;
  }

  //
  // Populate the exported interface's attributes
  //
  Dev->PassThru.Mode             = &Dev->PassThruMode;
  Dev->PassThru.PassThru         = &PvScsiPassThru;
  Dev->PassThru.GetNextTargetLun = &PvScsiGetNextTargetLun;
  Dev->PassThru.BuildDevicePath  = &PvScsiBuildDevicePath;
  Dev->PassThru.GetTargetLun     = &PvScsiGetTargetLun;
  Dev->PassThru.ResetChannel     = &PvScsiResetChannel;
  Dev->PassThru.ResetTargetLun   = &PvScsiResetTargetLun;
  Dev->PassThru.GetNextTarget    = &PvScsiGetNextTarget;

  //
  // AdapterId is a target for which no handle will be created during bus scan.
  // Prevent any conflict with real devices.
  //
  Dev->PassThruMode.AdapterId = MAX_UINT32;

  //
  // Set both physical and logical attributes for non-RAID SCSI channel
  //
  Dev->PassThruMode.Attributes = EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_PHYSICAL |
                                 EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_LOGICAL;

  //
  // No restriction on transfer buffer alignment
  //
  Dev->PassThruMode.IoAlign = 0;

  return EFI_SUCCESS;

FreeDmaCommBuffer:
  PvScsiFreeSharedPages (
    Dev,
    EFI_SIZE_TO_PAGES (sizeof (*Dev->DmaBuf)),
    Dev->DmaBuf,
    &Dev->DmaBufDmaDesc
    );

FreeRings:
  PvScsiFreeRings (Dev);

RestorePciAttributes:
  PvScsiRestorePciAttributes (Dev);

  return Status;
}

STATIC
VOID
PvScsiUninit (
  IN OUT PVSCSI_DEV *Dev
  )
{
  //
  // Reset device to:
  // - Make device stop processing all requests.
  // - Stop device usage of the rings.
  //
  // This is required to safely free the DMA communication buffer
  // and the rings.
  //
  PvScsiResetAdapter (Dev);

  //
  // Free DMA communication buffer
  //
  PvScsiFreeSharedPages (
    Dev,
    EFI_SIZE_TO_PAGES (sizeof (*Dev->DmaBuf)),
    Dev->DmaBuf,
    &Dev->DmaBufDmaDesc
    );

  PvScsiFreeRings (Dev);

  PvScsiRestorePciAttributes (Dev);
}

/**
  Event notification called by ExitBootServices()
**/
STATIC
VOID
EFIAPI
PvScsiExitBoot (
  IN  EFI_EVENT Event,
  IN  VOID      *Context
  )
{
  PVSCSI_DEV *Dev;

  Dev = Context;
  DEBUG ((DEBUG_VERBOSE, "%a: Context=0x%p\n", __FUNCTION__, Context));

  //
  // Reset the device to stop device usage of the rings.
  //
  // We allocated said rings in EfiBootServicesData type memory, and code
  // executing after ExitBootServices() is permitted to overwrite it.
  //
  PvScsiResetAdapter (Dev);
}

//
// Driver Binding
//

STATIC
EFI_STATUS
EFIAPI
PvScsiDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  PCI_TYPE00          Pci;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        0,
                        sizeof (Pci) / sizeof (UINT32),
                        &Pci
                        );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  if ((Pci.Hdr.VendorId != PCI_VENDOR_ID_VMWARE) ||
      (Pci.Hdr.DeviceId != PCI_DEVICE_ID_VMWARE_PVSCSI)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  Status = EFI_SUCCESS;

Done:
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath OPTIONAL
  )
{
  PVSCSI_DEV *Dev;
  EFI_STATUS Status;

  Dev = (PVSCSI_DEV *) AllocateZeroPool (sizeof (*Dev));
  if (Dev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&Dev->PciIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto FreePvScsi;
  }

  Status = PvScsiInit (Dev);
  if (EFI_ERROR (Status)) {
    goto ClosePciIo;
  }

  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_CALLBACK,
                  &PvScsiExitBoot,
                  Dev,
                  &Dev->ExitBoot
                  );
  if (EFI_ERROR (Status)) {
    goto UninitDev;
  }

  //
  // Setup complete, attempt to export the driver instance's PassThru interface
  //
  Dev->Signature = PVSCSI_SIG;
  Status = gBS->InstallProtocolInterface (
                  &ControllerHandle,
                  &gEfiExtScsiPassThruProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &Dev->PassThru
                  );
  if (EFI_ERROR (Status)) {
    goto CloseExitBoot;
  }

  return EFI_SUCCESS;

CloseExitBoot:
  gBS->CloseEvent (Dev->ExitBoot);

UninitDev:
  PvScsiUninit (Dev);

ClosePciIo:
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

FreePvScsi:
  FreePool (Dev);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  ControllerHandle,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
  )
{
  EFI_STATUS                      Status;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL *PassThru;
  PVSCSI_DEV                      *Dev;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiExtScsiPassThruProtocolGuid,
                  (VOID **)&PassThru,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL // Lookup only
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Dev = PVSCSI_FROM_PASS_THRU (PassThru);

  Status = gBS->UninstallProtocolInterface (
                  ControllerHandle,
                  &gEfiExtScsiPassThruProtocolGuid,
                  &Dev->PassThru
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseEvent (Dev->ExitBoot);

  PvScsiUninit (Dev);

  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  FreePool (Dev);

  return EFI_SUCCESS;
}

STATIC EFI_DRIVER_BINDING_PROTOCOL mPvScsiDriverBinding = {
  &PvScsiDriverBindingSupported,
  &PvScsiDriverBindingStart,
  &PvScsiDriverBindingStop,
  PVSCSI_BINDING_VERSION,
  NULL, // ImageHandle, filled by EfiLibInstallDriverBindingComponentName2()
  NULL  // DriverBindingHandle, filled as well
};

//
// Component Name
//

STATIC EFI_UNICODE_STRING_TABLE mDriverNameTable[] = {
  { "eng;en", L"PVSCSI Host Driver" },
  { NULL,     NULL                  }
};

STATIC EFI_COMPONENT_NAME_PROTOCOL mComponentName;

STATIC
EFI_STATUS
EFIAPI
PvScsiGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mDriverNameTable,
           DriverName,
           (BOOLEAN)(This == &mComponentName) // Iso639Language
           );
}

STATIC
EFI_STATUS
EFIAPI
PvScsiGetDeviceName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  EFI_HANDLE                  DeviceHandle,
  IN  EFI_HANDLE                  ChildHandle,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **ControllerName
  )
{
  return EFI_UNSUPPORTED;
}

STATIC EFI_COMPONENT_NAME_PROTOCOL mComponentName = {
  &PvScsiGetDriverName,
  &PvScsiGetDeviceName,
  "eng" // SupportedLanguages, ISO 639-2 language codes
};

STATIC EFI_COMPONENT_NAME2_PROTOCOL mComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)     &PvScsiGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME) &PvScsiGetDeviceName,
  "en" // SupportedLanguages, RFC 4646 language codes
};

//
// Entry Point
//

EFI_STATUS
EFIAPI
PvScsiEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &mPvScsiDriverBinding,
           ImageHandle,
           &mComponentName,
           &mComponentName2
           );
}
