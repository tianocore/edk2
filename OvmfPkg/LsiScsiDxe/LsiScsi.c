/** @file

  This driver produces Extended SCSI Pass Thru Protocol instances for
  LSI 53C895A SCSI devices.

  Copyright (C) 2020, SUSE LLC.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/LsiScsi.h>
#include <IndustryStandard/Pci.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/PciIo.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/ScsiPassThruExt.h>
#include <Uefi/UefiSpec.h>

#include "LsiScsi.h"

STATIC
EFI_STATUS
Out8 (
  IN LSI_SCSI_DEV  *Dev,
  IN UINT32        Addr,
  IN UINT8         Data
  )
{
  return Dev->PciIo->Io.Write (
                          Dev->PciIo,
                          EfiPciIoWidthUint8,
                          PCI_BAR_IDX0,
                          Addr,
                          1,
                          &Data
                          );
}

STATIC
EFI_STATUS
Out32 (
  IN LSI_SCSI_DEV  *Dev,
  IN UINT32        Addr,
  IN UINT32        Data
  )
{
  return Dev->PciIo->Io.Write (
                          Dev->PciIo,
                          EfiPciIoWidthUint32,
                          PCI_BAR_IDX0,
                          Addr,
                          1,
                          &Data
                          );
}

STATIC
EFI_STATUS
In8 (
  IN  LSI_SCSI_DEV  *Dev,
  IN  UINT32        Addr,
  OUT UINT8         *Data
  )
{
  return Dev->PciIo->Io.Read (
                          Dev->PciIo,
                          EfiPciIoWidthUint8,
                          PCI_BAR_IDX0,
                          Addr,
                          1,
                          Data
                          );
}

STATIC
EFI_STATUS
In32 (
  IN  LSI_SCSI_DEV  *Dev,
  IN  UINT32        Addr,
  OUT UINT32        *Data
  )
{
  return Dev->PciIo->Io.Read (
                          Dev->PciIo,
                          EfiPciIoWidthUint32,
                          PCI_BAR_IDX0,
                          Addr,
                          1,
                          Data
                          );
}

STATIC
EFI_STATUS
LsiScsiReset (
  IN LSI_SCSI_DEV  *Dev
  )
{
  return Out8 (Dev, LSI_REG_ISTAT0, LSI_ISTAT0_SRST);
}

STATIC
EFI_STATUS
ReportHostAdapterOverrunError (
  OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *Packet
  )
{
  Packet->SenseDataLength   = 0;
  Packet->HostAdapterStatus =
    EFI_EXT_SCSI_STATUS_HOST_ADAPTER_DATA_OVERRUN_UNDERRUN;
  Packet->TargetStatus = EFI_EXT_SCSI_STATUS_TARGET_GOOD;
  return EFI_BAD_BUFFER_SIZE;
}

/**

  Check the request packet from the Extended SCSI Pass Thru Protocol. The
  request packet is modified, to be forwarded outwards by LsiScsiPassThru(),
  if invalid or unsupported parameters are detected.

  @param[in] Dev          The LSI 53C895A SCSI device the packet targets.

  @param[in] Target       The SCSI target controlled by the LSI 53C895A SCSI
                          device.

  @param[in] Lun          The Logical Unit Number under the SCSI target.

  @param[in out] Packet   The Extended SCSI Pass Thru Protocol packet.


  @retval EFI_SUCCESS  The Extended SCSI Pass Thru Protocol packet was valid.

  @return              Otherwise, invalid or unsupported parameters were
                       detected. Status codes are meant for direct forwarding
                       by the EFI_EXT_SCSI_PASS_THRU_PROTOCOL.PassThru()
                       implementation.

 **/
STATIC
EFI_STATUS
LsiScsiCheckRequest (
  IN LSI_SCSI_DEV                                    *Dev,
  IN UINT8                                           Target,
  IN UINT64                                          Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *Packet
  )
{
  if ((Target > Dev->MaxTarget) || (Lun > Dev->MaxLun) ||
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
      // Trying to send, but source pointer is NULL, or contradicting transfer
      // direction
      //
      ((Packet->OutTransferLength > 0) &&
       ((Packet->OutDataBuffer == NULL) ||
        (Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_READ)
       )
      )
      )
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_BIDIRECTIONAL) ||
      ((Packet->InTransferLength > 0) && (Packet->OutTransferLength > 0)) ||
      (Packet->CdbLength > sizeof Dev->Dma->Cdb))
  {
    return EFI_UNSUPPORTED;
  }

  if (Packet->InTransferLength > sizeof Dev->Dma->Data) {
    Packet->InTransferLength = sizeof Dev->Dma->Data;
    return ReportHostAdapterOverrunError (Packet);
  }

  if (Packet->OutTransferLength > sizeof Dev->Dma->Data) {
    Packet->OutTransferLength = sizeof Dev->Dma->Data;
    return ReportHostAdapterOverrunError (Packet);
  }

  return EFI_SUCCESS;
}

/**

  Interpret the request packet from the Extended SCSI Pass Thru Protocol and
  compose the script to submit the command and data to the controller.

  @param[in] Dev          The LSI 53C895A SCSI device the packet targets.

  @param[in] Target       The SCSI target controlled by the LSI 53C895A SCSI
                          device.

  @param[in] Lun          The Logical Unit Number under the SCSI target.

  @param[in out] Packet   The Extended SCSI Pass Thru Protocol packet.


  @retval EFI_SUCCESS  The Extended SCSI Pass Thru Protocol packet was valid.

  @return              Otherwise, invalid or unsupported parameters were
                       detected. Status codes are meant for direct forwarding
                       by the EFI_EXT_SCSI_PASS_THRU_PROTOCOL.PassThru()
                       implementation.

 **/
STATIC
EFI_STATUS
LsiScsiProcessRequest (
  IN LSI_SCSI_DEV                                    *Dev,
  IN UINT8                                           Target,
  IN UINT64                                          Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *Packet
  )
{
  EFI_STATUS  Status;
  UINT32      *Script;
  UINT8       *Cdb;
  UINT8       *MsgOut;
  UINT8       *MsgIn;
  UINT8       *ScsiStatus;
  UINT8       *Data;
  UINT8       DStat;
  UINT8       SIst0;
  UINT8       SIst1;
  UINT32      Csbc;
  UINT32      CsbcBase;
  UINT32      Transferred;

  Script     = Dev->Dma->Script;
  Cdb        = Dev->Dma->Cdb;
  Data       = Dev->Dma->Data;
  MsgIn      = Dev->Dma->MsgIn;
  MsgOut     = &Dev->Dma->MsgOut;
  ScsiStatus = &Dev->Dma->Status;

  *ScsiStatus = 0xFF;

  DStat = 0;
  SIst0 = 0;
  SIst1 = 0;

  SetMem (Cdb, sizeof Dev->Dma->Cdb, 0x00);
  CopyMem (Cdb, Packet->Cdb, Packet->CdbLength);

  //
  // Fetch the first Cumulative SCSI Byte Count (CSBC).
  //
  // CSBC is a cumulative counter of the actual number of bytes that have been
  // transferred across the SCSI bus during data phases, i.e. it will not
  // count bytes sent in command, status, message in and out phases.
  //
  Status = In32 (Dev, LSI_REG_CSBC, &CsbcBase);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  //
  // Clean up the DMA buffer for the script.
  //
  SetMem (Script, sizeof Dev->Dma->Script, 0x00);

  //
  // Compose the script to transfer data between the host and the device.
  //
  // References:
  //   * LSI53C895A PCI to Ultra2 SCSI Controller Version 2.2
  //     - Chapter 5 SCSI SCRIPT Instruction Set
  //   * SEABIOS lsi-scsi driver
  //
  // All instructions used here consist of 2 32bit words. The first word
  // contains the command to execute. The second word is loaded into the
  // DMA SCRIPTS Pointer Save (DSPS) register as either the DMA address
  // for data transmission or the address/offset for the jump command.
  // Some commands, such as the selection of the target, don't need to
  // transfer data through DMA or jump to another instruction, then DSPS
  // has to be zero.
  //
  // There are 3 major parts in this script. The first part (1~3) contains
  // the instructions to select target and LUN and send the SCSI command
  // from the request packet. The second part (4~7) is to handle the
  // potential disconnection and prepare for the data transmission. The
  // instructions in the third part (8~10) transmit the given data and
  // collect the result. Instruction 11 raises the interrupt and marks the
  // end of the script.
  //

  //
  // 1. Select target.
  //
  *Script++ = LSI_INS_TYPE_IO | LSI_INS_IO_OPC_SEL | (UINT32)Target << 16;
  *Script++ = 0x00000000;

  //
  // 2. Select LUN.
  //
  *MsgOut   = 0x80 | (UINT8)Lun;  // 0x80: Identify bit
  *Script++ = LSI_INS_TYPE_BLK | LSI_INS_BLK_SCSIP_MSG_OUT |
              (UINT32)sizeof Dev->Dma->MsgOut;
  *Script++ = LSI_SCSI_DMA_ADDR (Dev, MsgOut);

  //
  // 3. Send the SCSI Command.
  //
  *Script++ = LSI_INS_TYPE_BLK | LSI_INS_BLK_SCSIP_CMD |
              (UINT32)sizeof Dev->Dma->Cdb;
  *Script++ = LSI_SCSI_DMA_ADDR (Dev, Cdb);

  //
  // 4. Check whether the current SCSI phase is "Message In" or not
  //    and jump to 7 if it is.
  //    Note: LSI_INS_TC_RA stands for "Relative Address Mode", so the
  //          offset 0x18 in the second word means jumping forward
  //          3 (0x18/8) instructions.
  //
  *Script++ = LSI_INS_TYPE_TC | LSI_INS_TC_OPC_JMP |
              LSI_INS_TC_SCSIP_MSG_IN | LSI_INS_TC_RA |
              LSI_INS_TC_CP;
  *Script++ = 0x00000018;

  //
  // 5. Read "Message" from the initiator to trigger reselect.
  //
  *Script++ = LSI_INS_TYPE_BLK | LSI_INS_BLK_SCSIP_MSG_IN |
              (UINT32)sizeof Dev->Dma->MsgIn;
  *Script++ = LSI_SCSI_DMA_ADDR (Dev, MsgIn);

  //
  // 6. Wait reselect.
  //
  *Script++ = LSI_INS_TYPE_IO | LSI_INS_IO_OPC_WAIT_RESEL;
  *Script++ = 0x00000000;

  //
  // 7. Read "Message" from the initiator again
  //
  *Script++ = LSI_INS_TYPE_BLK | LSI_INS_BLK_SCSIP_MSG_IN |
              (UINT32)sizeof Dev->Dma->MsgIn;
  *Script++ = LSI_SCSI_DMA_ADDR (Dev, MsgIn);

  //
  // 8. Set the DMA command for the read/write operations.
  //    Note: Some requests, e.g. "TEST UNIT READY", do not come with
  //          allocated InDataBuffer or OutDataBuffer. We skip the DMA
  //          data command for those requests or this script would fail
  //          with LSI_SIST0_SGE due to the zero data length.
  //
  // LsiScsiCheckRequest() prevents both integer overflows in the command
  // opcodes, and buffer overflows.
  //
  if (Packet->InTransferLength > 0) {
    ASSERT (Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_READ);
    ASSERT (Packet->InTransferLength <= sizeof Dev->Dma->Data);
    *Script++ = LSI_INS_TYPE_BLK | LSI_INS_BLK_SCSIP_DAT_IN |
                Packet->InTransferLength;
    *Script++ = LSI_SCSI_DMA_ADDR (Dev, Data);
  } else if (Packet->OutTransferLength > 0) {
    ASSERT (Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_WRITE);
    ASSERT (Packet->OutTransferLength <= sizeof Dev->Dma->Data);
    CopyMem (Data, Packet->OutDataBuffer, Packet->OutTransferLength);
    *Script++ = LSI_INS_TYPE_BLK | LSI_INS_BLK_SCSIP_DAT_OUT |
                Packet->OutTransferLength;
    *Script++ = LSI_SCSI_DMA_ADDR (Dev, Data);
  }

  //
  // 9. Get the SCSI status.
  //
  *Script++ = LSI_INS_TYPE_BLK | LSI_INS_BLK_SCSIP_STAT |
              (UINT32)sizeof Dev->Dma->Status;
  *Script++ = LSI_SCSI_DMA_ADDR (Dev, Status);

  //
  // 10. Get the SCSI message.
  //
  *Script++ = LSI_INS_TYPE_BLK | LSI_INS_BLK_SCSIP_MSG_IN |
              (UINT32)sizeof Dev->Dma->MsgIn;
  *Script++ = LSI_SCSI_DMA_ADDR (Dev, MsgIn);

  //
  // 11. Raise the interrupt to end the script.
  //
  *Script++ = LSI_INS_TYPE_TC | LSI_INS_TC_OPC_INT |
              LSI_INS_TC_SCSIP_DAT_OUT | LSI_INS_TC_JMP;
  *Script++ = 0x00000000;

  //
  // Make sure the size of the script doesn't exceed the buffer.
  //
  ASSERT (Script <= Dev->Dma->Script + ARRAY_SIZE (Dev->Dma->Script));

  //
  // The controller starts to execute the script once the DMA Script
  // Pointer (DSP) register is set.
  //
  Status = Out32 (Dev, LSI_REG_DSP, LSI_SCSI_DMA_ADDR (Dev, Script));
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  //
  // Poll the device registers (DSTAT, SIST0, and SIST1) until the SIR
  // bit sets.
  //
  for ( ; ;) {
    Status = In8 (Dev, LSI_REG_DSTAT, &DStat);
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    Status = In8 (Dev, LSI_REG_SIST0, &SIst0);
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    Status = In8 (Dev, LSI_REG_SIST1, &SIst1);
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    if ((SIst0 != 0) || (SIst1 != 0)) {
      goto Error;
    }

    //
    // Check the SIR (SCRIPTS Interrupt Instruction Received) bit.
    //
    if (DStat & LSI_DSTAT_SIR) {
      break;
    }

    gBS->Stall (Dev->StallPerPollUsec);
  }

  //
  // Check if everything is good.
  //   SCSI Message Code 0x00: COMMAND COMPLETE
  //   SCSI Status  Code 0x00: Good
  //
  if ((MsgIn[0] != 0) || (*ScsiStatus != 0)) {
    goto Error;
  }

  //
  // Fetch CSBC again to calculate the transferred bytes and update
  // InTransferLength/OutTransferLength.
  //
  // Note: The number of transferred bytes is bounded by
  //       "sizeof Dev->Dma->Data", so it's safe to subtract CsbcBase
  //       from Csbc. If the CSBC register wraps around, the correct
  //       difference is ensured by the standard C modular arithmetic.
  //
  Status = In32 (Dev, LSI_REG_CSBC, &Csbc);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  Transferred = Csbc - CsbcBase;
  if (Packet->InTransferLength > 0) {
    if (Transferred <= Packet->InTransferLength) {
      Packet->InTransferLength = Transferred;
    } else {
      goto Error;
    }
  } else if (Packet->OutTransferLength > 0) {
    if (Transferred <= Packet->OutTransferLength) {
      Packet->OutTransferLength = Transferred;
    } else {
      goto Error;
    }
  }

  //
  // Copy Data to InDataBuffer if necessary.
  //
  if (Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_READ) {
    CopyMem (Packet->InDataBuffer, Data, Packet->InTransferLength);
  }

  //
  // Always set SenseDataLength to 0.
  // The instructions of LSI53C895A don't reply sense data. Instead, it
  // relies on the SCSI command, "REQUEST SENSE", to get sense data. We set
  // SenseDataLength to 0 to notify ScsiDiskDxe that there is no sense data
  // written even if this request is processed successfully, so that It will
  // issue "REQUEST SENSE" later to retrieve sense data.
  //
  Packet->SenseDataLength   = 0;
  Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_OK;
  Packet->TargetStatus      = EFI_EXT_SCSI_STATUS_TARGET_GOOD;

  return EFI_SUCCESS;

Error:
  DEBUG ((
    DEBUG_VERBOSE,
    "%a: dstat: %02X, sist0: %02X, sist1: %02X\n",
    __func__,
    DStat,
    SIst0,
    SIst1
    ));
  //
  // Update the request packet to reflect the status.
  //
  if (*ScsiStatus != 0xFF) {
    Packet->TargetStatus = *ScsiStatus;
  } else {
    Packet->TargetStatus = EFI_EXT_SCSI_STATUS_TARGET_TASK_ABORTED;
  }

  if (SIst0 & LSI_SIST0_PAR) {
    Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_PARITY_ERROR;
  } else if (SIst0 & LSI_SIST0_RST) {
    Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_BUS_RESET;
  } else if (SIst0 & LSI_SIST0_UDC) {
    //
    // The target device is disconnected unexpectedly. According to UEFI spec,
    // this is TIMEOUT_COMMAND.
    //
    Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_TIMEOUT_COMMAND;
  } else if (SIst0 & LSI_SIST0_SGE) {
    Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_DATA_OVERRUN_UNDERRUN;
  } else if (SIst1 & LSI_SIST1_HTH) {
    Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_TIMEOUT;
  } else if (SIst1 & LSI_SIST1_GEN) {
    Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_TIMEOUT;
  } else if (SIst1 & LSI_SIST1_STO) {
    Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_SELECTION_TIMEOUT;
  } else {
    Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_OTHER;
  }

  //
  // SenseData may be used to inspect the error. Since we don't set sense data,
  // SenseDataLength has to be 0.
  //
  Packet->SenseDataLength = 0;

  return EFI_DEVICE_ERROR;
}

//
// The next seven functions implement EFI_EXT_SCSI_PASS_THRU_PROTOCOL
// for the LSI 53C895A SCSI Controller. Refer to UEFI Spec 2.3.1 + Errata C,
// sections
// - 14.1 SCSI Driver Model Overview,
// - 14.7 Extended SCSI Pass Thru Protocol.
//

EFI_STATUS
EFIAPI
LsiScsiPassThru (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                 *This,
  IN UINT8                                           *Target,
  IN UINT64                                          Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *Packet,
  IN EFI_EVENT                                       Event     OPTIONAL
  )
{
  EFI_STATUS    Status;
  LSI_SCSI_DEV  *Dev;

  Dev    = LSI_SCSI_FROM_PASS_THRU (This);
  Status = LsiScsiCheckRequest (Dev, *Target, Lun, Packet);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return LsiScsiProcessRequest (Dev, *Target, Lun, Packet);
}

EFI_STATUS
EFIAPI
LsiScsiGetNextTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN OUT UINT8                        **TargetPointer,
  IN OUT UINT64                       *Lun
  )
{
  LSI_SCSI_DEV  *Dev;
  UINTN         Idx;
  UINT8         *Target;
  UINT16        LastTarget;

  //
  // the TargetPointer input parameter is unnecessarily a pointer-to-pointer
  //
  Target = *TargetPointer;

  //
  // Search for first non-0xFF byte. If not found, return first target & LUN.
  //
  for (Idx = 0; Idx < TARGET_MAX_BYTES && Target[Idx] == 0xFF; ++Idx) {
  }

  if (Idx == TARGET_MAX_BYTES) {
    SetMem (Target, TARGET_MAX_BYTES, 0x00);
    *Lun = 0;
    return EFI_SUCCESS;
  }

  CopyMem (&LastTarget, Target, sizeof LastTarget);

  //
  // increment (target, LUN) pair if valid on input
  //
  Dev = LSI_SCSI_FROM_PASS_THRU (This);
  if ((LastTarget > Dev->MaxTarget) || (*Lun > Dev->MaxLun)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*Lun < Dev->MaxLun) {
    ++*Lun;
    return EFI_SUCCESS;
  }

  if (LastTarget < Dev->MaxTarget) {
    *Lun = 0;
    ++LastTarget;
    CopyMem (Target, &LastTarget, sizeof LastTarget);
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
LsiScsiBuildDevicePath (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN UINT8                            *Target,
  IN UINT64                           Lun,
  IN OUT EFI_DEVICE_PATH_PROTOCOL     **DevicePath
  )
{
  UINT16            TargetValue;
  LSI_SCSI_DEV      *Dev;
  SCSI_DEVICE_PATH  *ScsiDevicePath;

  if (DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (&TargetValue, Target, sizeof TargetValue);
  Dev = LSI_SCSI_FROM_PASS_THRU (This);
  if ((TargetValue > Dev->MaxTarget) || (Lun > Dev->MaxLun) || (Lun > 0xFFFF)) {
    return EFI_NOT_FOUND;
  }

  ScsiDevicePath = AllocatePool (sizeof *ScsiDevicePath);
  if (ScsiDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ScsiDevicePath->Header.Type      = MESSAGING_DEVICE_PATH;
  ScsiDevicePath->Header.SubType   = MSG_SCSI_DP;
  ScsiDevicePath->Header.Length[0] = (UINT8)sizeof *ScsiDevicePath;
  ScsiDevicePath->Header.Length[1] = (UINT8)(sizeof *ScsiDevicePath >> 8);
  ScsiDevicePath->Pun              = TargetValue;
  ScsiDevicePath->Lun              = (UINT16)Lun;

  *DevicePath = &ScsiDevicePath->Header;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LsiScsiGetTargetLun (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN  EFI_DEVICE_PATH_PROTOCOL         *DevicePath,
  OUT UINT8                            **TargetPointer,
  OUT UINT64                           *Lun
  )
{
  SCSI_DEVICE_PATH  *ScsiDevicePath;
  LSI_SCSI_DEV      *Dev;
  UINT8             *Target;

  if ((DevicePath == NULL) || (TargetPointer == NULL) || (*TargetPointer == NULL) ||
      (Lun == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((DevicePath->Type    != MESSAGING_DEVICE_PATH) ||
      (DevicePath->SubType != MSG_SCSI_DP))
  {
    return EFI_UNSUPPORTED;
  }

  ScsiDevicePath = (SCSI_DEVICE_PATH *)DevicePath;
  Dev            = LSI_SCSI_FROM_PASS_THRU (This);
  if ((ScsiDevicePath->Pun > Dev->MaxTarget) ||
      (ScsiDevicePath->Lun > Dev->MaxLun))
  {
    return EFI_NOT_FOUND;
  }

  Target = *TargetPointer;
  ZeroMem (Target, TARGET_MAX_BYTES);
  CopyMem (Target, &ScsiDevicePath->Pun, sizeof ScsiDevicePath->Pun);
  *Lun = ScsiDevicePath->Lun;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LsiScsiResetChannel (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
LsiScsiResetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN UINT8                            *Target,
  IN UINT64                           Lun
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
LsiScsiGetNextTarget (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN OUT UINT8                        **TargetPointer
  )
{
  LSI_SCSI_DEV  *Dev;
  UINTN         Idx;
  UINT8         *Target;
  UINT16        LastTarget;

  //
  // the TargetPointer input parameter is unnecessarily a pointer-to-pointer
  //
  Target = *TargetPointer;

  //
  // Search for first non-0xFF byte. If not found, return first target.
  //
  for (Idx = 0; Idx < TARGET_MAX_BYTES && Target[Idx] == 0xFF; ++Idx) {
  }

  if (Idx == TARGET_MAX_BYTES) {
    SetMem (Target, TARGET_MAX_BYTES, 0x00);
    return EFI_SUCCESS;
  }

  CopyMem (&LastTarget, Target, sizeof LastTarget);

  //
  // increment target if valid on input
  //
  Dev = LSI_SCSI_FROM_PASS_THRU (This);
  if (LastTarget > Dev->MaxTarget) {
    return EFI_INVALID_PARAMETER;
  }

  if (LastTarget < Dev->MaxTarget) {
    ++LastTarget;
    CopyMem (Target, &LastTarget, sizeof LastTarget);
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

STATIC
VOID
EFIAPI
LsiScsiExitBoot (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  LSI_SCSI_DEV  *Dev;

  Dev = Context;
  DEBUG ((DEBUG_VERBOSE, "%a: Context=0x%p\n", __func__, Context));
  LsiScsiReset (Dev);
}

//
// Probe, start and stop functions of this driver, called by the DXE core for
// specific devices.
//
// The following specifications document these interfaces:
// - Driver Writer's Guide for UEFI 2.3.1 v1.01, 9 Driver Binding Protocol
// - UEFI Spec 2.3.1 + Errata C, 10.1 EFI Driver Binding Protocol
//

EFI_STATUS
EFIAPI
LsiScsiControllerSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  PCI_TYPE00           Pci;

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

  if ((Pci.Hdr.VendorId == LSI_LOGIC_PCI_VENDOR_ID) &&
      (Pci.Hdr.DeviceId == LSI_53C895A_PCI_DEVICE_ID))
  {
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_UNSUPPORTED;
  }

Done:
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );
  return Status;
}

EFI_STATUS
EFIAPI
LsiScsiControllerStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS    Status;
  LSI_SCSI_DEV  *Dev;
  UINTN         Pages;
  UINTN         BytesMapped;

  Dev = AllocateZeroPool (sizeof (*Dev));
  if (Dev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Dev->Signature = LSI_SCSI_DEV_SIGNATURE;

  STATIC_ASSERT (
    FixedPcdGet8 (PcdLsiScsiMaxTargetLimit) < 8,
    "LSI 53C895A supports targets [0..7]"
    );
  STATIC_ASSERT (
    FixedPcdGet8 (PcdLsiScsiMaxLunLimit) < 128,
    "LSI 53C895A supports LUNs [0..127]"
    );
  Dev->MaxTarget        = PcdGet8 (PcdLsiScsiMaxTargetLimit);
  Dev->MaxLun           = PcdGet8 (PcdLsiScsiMaxLunLimit);
  Dev->StallPerPollUsec = PcdGet32 (PcdLsiScsiStallPerPollUsec);

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&Dev->PciIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto FreePool;
  }

  Status = Dev->PciIo->Attributes (
                         Dev->PciIo,
                         EfiPciIoAttributeOperationGet,
                         0,
                         &Dev->OrigPciAttrs
                         );
  if (EFI_ERROR (Status)) {
    goto CloseProtocol;
  }

  //
  // Enable I/O Space & Bus-Mastering
  //
  Status = Dev->PciIo->Attributes (
                         Dev->PciIo,
                         EfiPciIoAttributeOperationEnable,
                         (EFI_PCI_IO_ATTRIBUTE_IO |
                          EFI_PCI_IO_ATTRIBUTE_BUS_MASTER),
                         NULL
                         );
  if (EFI_ERROR (Status)) {
    goto CloseProtocol;
  }

  //
  // Create buffers for data transfer
  //
  Pages  = EFI_SIZE_TO_PAGES (sizeof (*Dev->Dma));
  Status = Dev->PciIo->AllocateBuffer (
                         Dev->PciIo,
                         AllocateAnyPages,
                         EfiBootServicesData,
                         Pages,
                         (VOID **)&Dev->Dma,
                         EFI_PCI_ATTRIBUTE_MEMORY_CACHED
                         );
  if (EFI_ERROR (Status)) {
    goto RestoreAttributes;
  }

  BytesMapped = EFI_PAGES_TO_SIZE (Pages);
  Status      = Dev->PciIo->Map (
                              Dev->PciIo,
                              EfiPciIoOperationBusMasterCommonBuffer,
                              Dev->Dma,
                              &BytesMapped,
                              &Dev->DmaPhysical,
                              &Dev->DmaMapping
                              );
  if (EFI_ERROR (Status)) {
    goto FreeBuffer;
  }

  if (BytesMapped != EFI_PAGES_TO_SIZE (Pages)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Unmap;
  }

  Status = LsiScsiReset (Dev);
  if (EFI_ERROR (Status)) {
    goto Unmap;
  }

  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_CALLBACK,
                  &LsiScsiExitBoot,
                  Dev,
                  &Dev->ExitBoot
                  );
  if (EFI_ERROR (Status)) {
    goto UninitDev;
  }

  //
  // Host adapter channel, doesn't exist
  //
  Dev->PassThruMode.AdapterId  = MAX_UINT32;
  Dev->PassThruMode.Attributes =
    EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_PHYSICAL |
    EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_LOGICAL;

  Dev->PassThru.Mode             = &Dev->PassThruMode;
  Dev->PassThru.PassThru         = &LsiScsiPassThru;
  Dev->PassThru.GetNextTargetLun = &LsiScsiGetNextTargetLun;
  Dev->PassThru.BuildDevicePath  = &LsiScsiBuildDevicePath;
  Dev->PassThru.GetTargetLun     = &LsiScsiGetTargetLun;
  Dev->PassThru.ResetChannel     = &LsiScsiResetChannel;
  Dev->PassThru.ResetTargetLun   = &LsiScsiResetTargetLun;
  Dev->PassThru.GetNextTarget    = &LsiScsiGetNextTarget;

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
  LsiScsiReset (Dev);

Unmap:
  Dev->PciIo->Unmap (
                Dev->PciIo,
                Dev->DmaMapping
                );

FreeBuffer:
  Dev->PciIo->FreeBuffer (
                Dev->PciIo,
                Pages,
                Dev->Dma
                );

RestoreAttributes:
  Dev->PciIo->Attributes (
                Dev->PciIo,
                EfiPciIoAttributeOperationSet,
                Dev->OrigPciAttrs,
                NULL
                );

CloseProtocol:
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

FreePool:
  FreePool (Dev);

  return Status;
}

EFI_STATUS
EFIAPI
LsiScsiControllerStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                       Status;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *PassThru;
  LSI_SCSI_DEV                     *Dev;

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

  Dev = LSI_SCSI_FROM_PASS_THRU (PassThru);

  Status = gBS->UninstallProtocolInterface (
                  ControllerHandle,
                  &gEfiExtScsiPassThruProtocolGuid,
                  &Dev->PassThru
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseEvent (Dev->ExitBoot);

  LsiScsiReset (Dev);

  Dev->PciIo->Unmap (
                Dev->PciIo,
                Dev->DmaMapping
                );

  Dev->PciIo->FreeBuffer (
                Dev->PciIo,
                EFI_SIZE_TO_PAGES (sizeof (*Dev->Dma)),
                Dev->Dma
                );

  Dev->PciIo->Attributes (
                Dev->PciIo,
                EfiPciIoAttributeOperationSet,
                Dev->OrigPciAttrs,
                NULL
                );

  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  FreePool (Dev);

  return Status;
}

//
// The static object that groups the Supported() (ie. probe), Start() and
// Stop() functions of the driver together. Refer to UEFI Spec 2.3.1 + Errata
// C, 10.1 EFI Driver Binding Protocol.
//
STATIC
EFI_DRIVER_BINDING_PROTOCOL  gDriverBinding = {
  &LsiScsiControllerSupported,
  &LsiScsiControllerStart,
  &LsiScsiControllerStop,
  0x10, // Version, must be in [0x10 .. 0xFFFFFFEF] for IHV-developed drivers
  NULL, // ImageHandle, to be overwritten by
        // EfiLibInstallDriverBindingComponentName2() in LsiScsiEntryPoint()
  NULL  // DriverBindingHandle, ditto
};

//
// The purpose of the following scaffolding (EFI_COMPONENT_NAME_PROTOCOL and
// EFI_COMPONENT_NAME2_PROTOCOL implementation) is to format the driver's name
// in English, for display on standard console devices. This is recommended for
// UEFI drivers that follow the UEFI Driver Model. Refer to the Driver Writer's
// Guide for UEFI 2.3.1 v1.01, 11 UEFI Driver and Controller Names.
//
// Device type names ("LSI 53C895A SCSI Controller") are not formatted because
// the driver supports only that device type. Therefore the driver name
// suffices for unambiguous identification.
//

STATIC
EFI_UNICODE_STRING_TABLE  mDriverNameTable[] = {
  { "eng;en", L"LSI 53C895A SCSI Controller Driver" },
  { NULL,     NULL                                  }
};

STATIC
EFI_COMPONENT_NAME_PROTOCOL  gComponentName;

EFI_STATUS
EFIAPI
LsiScsiGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mDriverNameTable,
           DriverName,
           (BOOLEAN)(This == &gComponentName) // Iso639Language
           );
}

EFI_STATUS
EFIAPI
LsiScsiGetDeviceName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   DeviceHandle,
  IN  EFI_HANDLE                   ChildHandle,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_COMPONENT_NAME_PROTOCOL  gComponentName = {
  &LsiScsiGetDriverName,
  &LsiScsiGetDeviceName,
  "eng" // SupportedLanguages, ISO 639-2 language codes
};

STATIC
EFI_COMPONENT_NAME2_PROTOCOL  gComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)&LsiScsiGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME)&LsiScsiGetDeviceName,
  "en" // SupportedLanguages, RFC 4646 language codes
};

//
// Entry point of this driver
//
EFI_STATUS
EFIAPI
LsiScsiEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gDriverBinding,
           ImageHandle, // The handle to install onto
           &gComponentName,
           &gComponentName2
           );
}
