/** @file
  Implementation of the command set of USB Mass Storage Specification
  for Bootability, Revision 1.0.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UsbMass.h"

/**
  Execute REQUEST SENSE Command to retrieve sense data from device.

  @param  UsbMass                The device whose sense data is requested.

  @retval EFI_SUCCESS            The command is executed successfully.
  @retval EFI_DEVICE_ERROR       Failed to request sense.
  @retval EFI_NO_RESPONSE        The device media doesn't response this request.
  @retval EFI_INVALID_PARAMETER  The command has some invalid parameters.
  @retval EFI_WRITE_PROTECTED    The device is write protected.
  @retval EFI_MEDIA_CHANGED      The device media has been changed.

**/
EFI_STATUS
UsbBootRequestSense (
  IN USB_MASS_DEVICE  *UsbMass
  )
{
  USB_BOOT_REQUEST_SENSE_CMD   SenseCmd;
  USB_BOOT_REQUEST_SENSE_DATA  SenseData;
  EFI_BLOCK_IO_MEDIA           *Media;
  USB_MASS_TRANSPORT           *Transport;
  EFI_STATUS                   Status;
  UINT32                       CmdResult;

  Transport = UsbMass->Transport;

  //
  // Request the sense data from the device
  //
  ZeroMem (&SenseCmd, sizeof (USB_BOOT_REQUEST_SENSE_CMD));
  ZeroMem (&SenseData, sizeof (USB_BOOT_REQUEST_SENSE_DATA));

  SenseCmd.OpCode   = USB_BOOT_REQUEST_SENSE_OPCODE;
  SenseCmd.Lun      = (UINT8)(USB_BOOT_LUN (UsbMass->Lun));
  SenseCmd.AllocLen = (UINT8)sizeof (USB_BOOT_REQUEST_SENSE_DATA);

  Status = Transport->ExecCommand (
                        UsbMass->Context,
                        &SenseCmd,
                        sizeof (USB_BOOT_REQUEST_SENSE_CMD),
                        EfiUsbDataIn,
                        &SenseData,
                        sizeof (USB_BOOT_REQUEST_SENSE_DATA),
                        UsbMass->Lun,
                        USB_BOOT_GENERAL_CMD_TIMEOUT,
                        &CmdResult
                        );
  if (EFI_ERROR (Status) || (CmdResult != USB_MASS_CMD_SUCCESS)) {
    DEBUG ((DEBUG_ERROR, "UsbBootRequestSense: (%r) CmdResult=0x%x\n", Status, CmdResult));
    if (!EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
    }

    return Status;
  }

  //
  // If sense data is retrieved successfully, interpret the sense data
  // and update the media status if necessary.
  //
  Media = &UsbMass->BlockIoMedia;

  switch (USB_BOOT_SENSE_KEY (SenseData.SenseKey)) {
    case USB_BOOT_SENSE_NO_SENSE:
      if (SenseData.Asc == USB_BOOT_ASC_NO_ADDITIONAL_SENSE_INFORMATION) {
        //
        // It is not an error if a device does not have additional sense information
        //
        Status = EFI_SUCCESS;
      } else {
        Status = EFI_NO_RESPONSE;
      }

      break;

    case USB_BOOT_SENSE_RECOVERED:
      //
      // Suppose hardware can handle this case, and recover later by itself
      //
      Status = EFI_NOT_READY;
      break;

    case USB_BOOT_SENSE_NOT_READY:
      Status = EFI_DEVICE_ERROR;
      if (SenseData.Asc == USB_BOOT_ASC_NO_MEDIA) {
        Media->MediaPresent = FALSE;
        Status              = EFI_NO_MEDIA;
      } else if (SenseData.Asc == USB_BOOT_ASC_NOT_READY) {
        Status = EFI_NOT_READY;
      }

      break;

    case USB_BOOT_SENSE_ILLEGAL_REQUEST:
      Status = EFI_INVALID_PARAMETER;
      break;

    case USB_BOOT_SENSE_UNIT_ATTENTION:
      Status = EFI_DEVICE_ERROR;
      if (SenseData.Asc == USB_BOOT_ASC_MEDIA_CHANGE) {
        //
        // If MediaChange, reset ReadOnly and new MediaId
        //
        Status          = EFI_MEDIA_CHANGED;
        Media->ReadOnly = FALSE;
        Media->MediaId++;
      } else if (SenseData.Asc == USB_BOOT_ASC_NOT_READY) {
        Status = EFI_NOT_READY;
      } else if (SenseData.Asc == USB_BOOT_ASC_NO_MEDIA) {
        Status = EFI_NOT_READY;
      }

      break;

    case USB_BOOT_SENSE_DATA_PROTECT:
      Status          = EFI_WRITE_PROTECTED;
      Media->ReadOnly = TRUE;
      break;

    default:
      Status = EFI_DEVICE_ERROR;
      break;
  }

  DEBUG ((
    DEBUG_INFO,
    "UsbBootRequestSense: (%r) with error code (%x) sense key %x/%x/%x\n",
    Status,
    SenseData.ErrorCode,
    USB_BOOT_SENSE_KEY (SenseData.SenseKey),
    SenseData.Asc,
    SenseData.Ascq
    ));

  return Status;
}

/**
  Execute the USB mass storage bootability commands.

  This function executes the USB mass storage bootability commands.
  If execution failed, retrieve the error by REQUEST_SENSE, then
  update the device's status, such as ReadyOnly.

  @param  UsbMass                The device to issue commands to
  @param  Cmd                    The command to execute
  @param  CmdLen                 The length of the command
  @param  DataDir                The direction of data transfer
  @param  Data                   The buffer to hold the data
  @param  DataLen                The length of expected data
  @param  Timeout                The timeout used to transfer

  @retval EFI_SUCCESS            Command is executed successfully
  @retval Others                 Command execution failed.

**/
EFI_STATUS
UsbBootExecCmd (
  IN USB_MASS_DEVICE         *UsbMass,
  IN VOID                    *Cmd,
  IN UINT8                   CmdLen,
  IN EFI_USB_DATA_DIRECTION  DataDir,
  IN VOID                    *Data,
  IN UINT32                  DataLen,
  IN UINT32                  Timeout
  )
{
  USB_MASS_TRANSPORT  *Transport;
  EFI_STATUS          Status;
  UINT32              CmdResult;

  Transport = UsbMass->Transport;
  Status    = Transport->ExecCommand (
                           UsbMass->Context,
                           Cmd,
                           CmdLen,
                           DataDir,
                           Data,
                           DataLen,
                           UsbMass->Lun,
                           Timeout,
                           &CmdResult
                           );

  if (Status == EFI_TIMEOUT) {
    DEBUG ((DEBUG_ERROR, "UsbBootExecCmd: %r to Exec 0x%x Cmd\n", Status, *(UINT8 *)Cmd));
    return EFI_TIMEOUT;
  }

  //
  // If ExecCommand() returns no error and CmdResult is success,
  // then the command transfer is successful.
  //
  if ((CmdResult == USB_MASS_CMD_SUCCESS) && !EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  if (*((UINT8 *)Cmd) == USB_BOOT_INQUIRY_OPCODE) {
    //
    // Only take care the condition for INQUIRY command.
    // It would indicate the DEVICE may not ready for other command.
    // Return EFI_DEVICE_ERROR and the caller will do the retry.
    // Usb Mass Boot spec 3.1
    //
    if (!EFI_ERROR (Status) && (CmdResult == USB_MASS_CMD_PERSISTENT)) {
      Transport->Reset (UsbMass->Context, TRUE);
      return EFI_DEVICE_ERROR;
    }

    if (Status == EFI_DEVICE_ERROR) {
      Transport->Reset (UsbMass->Context, TRUE);
      return EFI_DEVICE_ERROR;
    }
  }

  //
  // If command execution failed, then retrieve error info via sense request.
  //
  DEBUG ((DEBUG_INFO, "UsbBootExecCmd: %r to Exec 0x%x Cmd (Result = %x)\n", Status, *(UINT8 *)Cmd, CmdResult));
  return UsbBootRequestSense (UsbMass);
}

/**
  Execute the USB mass storage bootability commands with retrial.

  This function executes USB mass storage bootability commands.
  If the device isn't ready, wait for it. If the device is ready
  and error occurs, retry the command again until it exceeds the
  limit of retrial times.

  @param  UsbMass                The device to issue commands to
  @param  Cmd                    The command to execute
  @param  CmdLen                 The length of the command
  @param  DataDir                The direction of data transfer
  @param  Data                   The buffer to hold the data
  @param  DataLen                The length of expected data
  @param  Timeout                The timeout used to transfer

  @retval EFI_SUCCESS            The command is executed successfully.
  @retval EFI_NO_MEDIA           The device media is removed.
  @retval Others                 Command execution failed after retrial.

**/
EFI_STATUS
UsbBootExecCmdWithRetry (
  IN USB_MASS_DEVICE         *UsbMass,
  IN VOID                    *Cmd,
  IN UINT8                   CmdLen,
  IN EFI_USB_DATA_DIRECTION  DataDir,
  IN VOID                    *Data,
  IN UINT32                  DataLen,
  IN UINT32                  Timeout
  )
{
  EFI_STATUS  Status;
  UINTN       Retry;
  EFI_EVENT   TimeoutEvt;

  Retry  = 0;
  Status = EFI_SUCCESS;
  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &TimeoutEvt
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->SetTimer (TimeoutEvt, TimerRelative, EFI_TIMER_PERIOD_SECONDS (60));
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  //
  // Execute the cmd and retry if it fails.
  //
  while (EFI_ERROR (gBS->CheckEvent (TimeoutEvt))) {
    Status = UsbBootExecCmd (
               UsbMass,
               Cmd,
               CmdLen,
               DataDir,
               Data,
               DataLen,
               Timeout
               );
    if ((Status == EFI_SUCCESS) || (Status == EFI_NO_MEDIA)) {
      break;
    }

    //
    // If the sense data shows the drive is not ready, we need execute the cmd again.
    // We limit the upper boundary to 60 seconds.
    //
    if (Status == EFI_NOT_READY) {
      continue;
    }

    //
    // If the status is other error, then just retry 5 times.
    //
    if (Retry++ >= USB_BOOT_COMMAND_RETRY) {
      break;
    }
  }

EXIT:
  if (TimeoutEvt != NULL) {
    gBS->CloseEvent (TimeoutEvt);
  }

  return Status;
}

/**
  Execute TEST UNIT READY command to check if the device is ready.

  @param  UsbMass                The device to test

  @retval EFI_SUCCESS            The device is ready.
  @retval Others                 Device not ready.

**/
EFI_STATUS
UsbBootIsUnitReady (
  IN USB_MASS_DEVICE  *UsbMass
  )
{
  USB_BOOT_TEST_UNIT_READY_CMD  TestCmd;

  ZeroMem (&TestCmd, sizeof (USB_BOOT_TEST_UNIT_READY_CMD));

  TestCmd.OpCode = USB_BOOT_TEST_UNIT_READY_OPCODE;
  TestCmd.Lun    = (UINT8)(USB_BOOT_LUN (UsbMass->Lun));

  return UsbBootExecCmdWithRetry (
           UsbMass,
           &TestCmd,
           (UINT8)sizeof (USB_BOOT_TEST_UNIT_READY_CMD),
           EfiUsbNoData,
           NULL,
           0,
           USB_BOOT_GENERAL_CMD_TIMEOUT
           );
}

/**
  Execute INQUIRY Command to request information regarding parameters of
  the device be sent to the host computer.

  @param  UsbMass                The device to inquire.

  @retval EFI_SUCCESS            INQUIRY Command is executed successfully.
  @retval Others                 INQUIRY Command is not executed successfully.

**/
EFI_STATUS
UsbBootInquiry (
  IN USB_MASS_DEVICE  *UsbMass
  )
{
  USB_BOOT_INQUIRY_CMD  InquiryCmd;
  EFI_BLOCK_IO_MEDIA    *Media;
  EFI_STATUS            Status;

  Media = &(UsbMass->BlockIoMedia);

  ZeroMem (&InquiryCmd, sizeof (USB_BOOT_INQUIRY_CMD));
  ZeroMem (&UsbMass->InquiryData, sizeof (USB_BOOT_INQUIRY_DATA));

  InquiryCmd.OpCode   = USB_BOOT_INQUIRY_OPCODE;
  InquiryCmd.Lun      = (UINT8)(USB_BOOT_LUN (UsbMass->Lun));
  InquiryCmd.AllocLen = (UINT8)sizeof (USB_BOOT_INQUIRY_DATA);

  Status = UsbBootExecCmdWithRetry (
             UsbMass,
             &InquiryCmd,
             (UINT8)sizeof (USB_BOOT_INQUIRY_CMD),
             EfiUsbDataIn,
             &UsbMass->InquiryData,
             sizeof (USB_BOOT_INQUIRY_DATA),
             USB_BOOT_GENERAL_CMD_TIMEOUT
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get information from PDT (Peripheral Device Type) field and Removable Medium Bit
  // from the inquiry data.
  //
  UsbMass->Pdt          = (UINT8)(USB_BOOT_PDT (UsbMass->InquiryData.Pdt));
  Media->RemovableMedia = (BOOLEAN)(USB_BOOT_REMOVABLE (UsbMass->InquiryData.Removable));
  //
  // Set block size to the default value of 512 Bytes, in case no media is present at first time.
  //
  Media->BlockSize = 0x0200;

  return Status;
}

/**
  Execute READ CAPACITY 16 bytes command to request information regarding
  the capacity of the installed medium of the device.

  This function executes READ CAPACITY 16 bytes command to get the capacity
  of the USB mass storage media, including the presence, block size,
  and last block number.

  @param  UsbMass                The device to retireve disk gemotric.

  @retval EFI_SUCCESS            The disk geometry is successfully retrieved.
  @retval EFI_NOT_READY          The returned block size is zero.
  @retval Other                  READ CAPACITY 16 bytes command execution failed.

**/
EFI_STATUS
UsbBootReadCapacity16 (
  IN USB_MASS_DEVICE  *UsbMass
  )
{
  UINT8                          CapacityCmd[16];
  EFI_SCSI_DISK_CAPACITY_DATA16  CapacityData;
  EFI_BLOCK_IO_MEDIA             *Media;
  EFI_STATUS                     Status;
  UINT32                         BlockSize;

  Media = &UsbMass->BlockIoMedia;

  Media->MediaPresent = FALSE;
  Media->LastBlock    = 0;
  Media->BlockSize    = 0;

  ZeroMem (CapacityCmd, sizeof (CapacityCmd));
  ZeroMem (&CapacityData, sizeof (CapacityData));

  CapacityCmd[0] = EFI_SCSI_OP_READ_CAPACITY16;
  CapacityCmd[1] = 0x10;
  //
  // Partial medium indicator, set the bytes 2 ~ 9 of the Cdb as ZERO.
  //
  ZeroMem ((CapacityCmd + 2), 8);

  CapacityCmd[13] = sizeof (CapacityData);

  Status = UsbBootExecCmdWithRetry (
             UsbMass,
             CapacityCmd,
             (UINT8)sizeof (CapacityCmd),
             EfiUsbDataIn,
             &CapacityData,
             sizeof (CapacityData),
             USB_BOOT_GENERAL_CMD_TIMEOUT
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the information on media presence, block size, and last block number
  // from READ CAPACITY data.
  //
  Media->MediaPresent = TRUE;
  Media->LastBlock    = SwapBytes64 (ReadUnaligned64 ((CONST UINT64 *)&(CapacityData.LastLba7)));

  BlockSize = SwapBytes32 (ReadUnaligned32 ((CONST UINT32 *)&(CapacityData.BlockSize3)));

  Media->LowestAlignedLba = (CapacityData.LowestAlignLogic2 << 8) |
                            CapacityData.LowestAlignLogic1;
  Media->LogicalBlocksPerPhysicalBlock = (1 << CapacityData.LogicPerPhysical);
  if (BlockSize == 0) {
    //
    //  Get sense data
    //
    return UsbBootRequestSense (UsbMass);
  } else {
    Media->BlockSize = BlockSize;
  }

  return Status;
}

/**
  Execute READ CAPACITY command to request information regarding
  the capacity of the installed medium of the device.

  This function executes READ CAPACITY command to get the capacity
  of the USB mass storage media, including the presence, block size,
  and last block number.

  @param  UsbMass                The device to retireve disk gemotric.

  @retval EFI_SUCCESS            The disk geometry is successfully retrieved.
  @retval EFI_NOT_READY          The returned block size is zero.
  @retval Other                  READ CAPACITY command execution failed.

**/
EFI_STATUS
UsbBootReadCapacity (
  IN USB_MASS_DEVICE  *UsbMass
  )
{
  USB_BOOT_READ_CAPACITY_CMD   CapacityCmd;
  USB_BOOT_READ_CAPACITY_DATA  CapacityData;
  EFI_BLOCK_IO_MEDIA           *Media;
  EFI_STATUS                   Status;
  UINT32                       BlockSize;

  Media = &UsbMass->BlockIoMedia;

  ZeroMem (&CapacityCmd, sizeof (USB_BOOT_READ_CAPACITY_CMD));
  ZeroMem (&CapacityData, sizeof (USB_BOOT_READ_CAPACITY_DATA));

  CapacityCmd.OpCode = USB_BOOT_READ_CAPACITY_OPCODE;
  CapacityCmd.Lun    = (UINT8)(USB_BOOT_LUN (UsbMass->Lun));

  Status = UsbBootExecCmdWithRetry (
             UsbMass,
             &CapacityCmd,
             (UINT8)sizeof (USB_BOOT_READ_CAPACITY_CMD),
             EfiUsbDataIn,
             &CapacityData,
             sizeof (USB_BOOT_READ_CAPACITY_DATA),
             USB_BOOT_GENERAL_CMD_TIMEOUT
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the information on media presence, block size, and last block number
  // from READ CAPACITY data.
  //
  Media->MediaPresent = TRUE;
  Media->LastBlock    = SwapBytes32 (ReadUnaligned32 ((CONST UINT32 *)CapacityData.LastLba));

  BlockSize = SwapBytes32 (ReadUnaligned32 ((CONST UINT32 *)CapacityData.BlockLen));
  if (BlockSize == 0) {
    //
    //  Get sense data
    //
    return UsbBootRequestSense (UsbMass);
  } else {
    Media->BlockSize = BlockSize;
  }

  if (Media->LastBlock == 0xFFFFFFFF) {
    Status = UsbBootReadCapacity16 (UsbMass);
    if (!EFI_ERROR (Status)) {
      UsbMass->Cdb16Byte = TRUE;
    }
  }

  return Status;
}

/**
  Retrieves SCSI mode sense information via MODE SENSE(6) command.

  @param  UsbMass                The device whose sense data is requested.

  @retval EFI_SUCCESS            SCSI mode sense information retrieved successfully.
  @retval Other                  Command execution failed.

**/
EFI_STATUS
UsbScsiModeSense (
  IN USB_MASS_DEVICE  *UsbMass
  )
{
  EFI_STATUS                        Status;
  USB_SCSI_MODE_SENSE6_CMD          ModeSenseCmd;
  USB_SCSI_MODE_SENSE6_PARA_HEADER  ModeParaHeader;
  EFI_BLOCK_IO_MEDIA                *Media;

  Media = &UsbMass->BlockIoMedia;

  ZeroMem (&ModeSenseCmd, sizeof (USB_SCSI_MODE_SENSE6_CMD));
  ZeroMem (&ModeParaHeader, sizeof (USB_SCSI_MODE_SENSE6_PARA_HEADER));

  //
  // MODE SENSE(6) command is defined in Section 8.2.10 of SCSI-2 Spec
  //
  ModeSenseCmd.OpCode      = USB_SCSI_MODE_SENSE6_OPCODE;
  ModeSenseCmd.Lun         = (UINT8)USB_BOOT_LUN (UsbMass->Lun);
  ModeSenseCmd.PageCode    = 0x3F;
  ModeSenseCmd.AllocateLen = (UINT8)sizeof (USB_SCSI_MODE_SENSE6_PARA_HEADER);

  Status = UsbBootExecCmdWithRetry (
             UsbMass,
             &ModeSenseCmd,
             (UINT8)sizeof (USB_SCSI_MODE_SENSE6_CMD),
             EfiUsbDataIn,
             &ModeParaHeader,
             sizeof (USB_SCSI_MODE_SENSE6_PARA_HEADER),
             USB_BOOT_GENERAL_CMD_TIMEOUT
             );

  //
  // Format of device-specific parameter byte of the mode parameter header is defined in
  // Section 8.2.10 of SCSI-2 Spec.
  // BIT7 of this byte is indicates whether the medium is write protected.
  //
  if (!EFI_ERROR (Status)) {
    Media->ReadOnly = (BOOLEAN)((ModeParaHeader.DevicePara & BIT7) != 0);
  }

  return Status;
}

/**
  Get the parameters for the USB mass storage media.

  This function get the parameters for the USB mass storage media,
  It is used both to initialize the media during the Start() phase
  of Driver Binding Protocol and to re-initialize it when the media is
  changed. Although the RemoveableMedia is unlikely to change,
  it is also included here.

  @param  UsbMass                The device to retrieve disk gemotric.

  @retval EFI_SUCCESS            The disk gemotric is successfully retrieved.
  @retval Other                  Failed to get the parameters.

**/
EFI_STATUS
UsbBootGetParams (
  IN USB_MASS_DEVICE  *UsbMass
  )
{
  EFI_BLOCK_IO_MEDIA  *Media;
  EFI_STATUS          Status;

  Media = &(UsbMass->BlockIoMedia);

  Status = UsbBootInquiry (UsbMass);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UsbBootGetParams: UsbBootInquiry (%r)\n", Status));
    return Status;
  }

  //
  // According to USB Mass Storage Specification for Bootability, only following
  // 4 Peripheral Device Types are in spec.
  //
  if ((UsbMass->Pdt != USB_PDT_DIRECT_ACCESS) &&
      (UsbMass->Pdt != USB_PDT_CDROM) &&
      (UsbMass->Pdt != USB_PDT_OPTICAL) &&
      (UsbMass->Pdt != USB_PDT_SIMPLE_DIRECT))
  {
    DEBUG ((DEBUG_ERROR, "UsbBootGetParams: Found an unsupported peripheral type[%d]\n", UsbMass->Pdt));
    return EFI_UNSUPPORTED;
  }

  //
  // Don't use the Removable bit in inquiry data to test whether the media
  // is removable because many flash disks wrongly set this bit.
  //
  if ((UsbMass->Pdt == USB_PDT_CDROM) || (UsbMass->Pdt == USB_PDT_OPTICAL)) {
    //
    // CD-Rom device and Non-CD optical device
    //
    UsbMass->OpticalStorage = TRUE;
    //
    // Default value 2048 Bytes, in case no media present at first time
    //
    Media->BlockSize = 0x0800;
  }

  Status = UsbBootDetectMedia (UsbMass);

  return Status;
}

/**
  Detect whether the removable media is present and whether it has changed.

  @param  UsbMass                The device to check.

  @retval EFI_SUCCESS            The media status is successfully checked.
  @retval Other                  Failed to detect media.

**/
EFI_STATUS
UsbBootDetectMedia (
  IN  USB_MASS_DEVICE  *UsbMass
  )
{
  EFI_BLOCK_IO_MEDIA  OldMedia;
  EFI_BLOCK_IO_MEDIA  *Media;
  UINT8               CmdSet;
  EFI_STATUS          Status;

  Media = &UsbMass->BlockIoMedia;

  CopyMem (&OldMedia, &(UsbMass->BlockIoMedia), sizeof (EFI_BLOCK_IO_MEDIA));

  CmdSet = ((EFI_USB_INTERFACE_DESCRIPTOR *)(UsbMass->Context))->InterfaceSubClass;

  Status = UsbBootIsUnitReady (UsbMass);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UsbBootDetectMedia: UsbBootIsUnitReady (%r)\n", Status));
  }

  //
  // Status could be:
  //   EFI_SUCCESS: all good.
  //   EFI_NO_MEDIA: media is not present.
  //   others: HW error.
  // For either EFI_NO_MEDIA, or HW error, skip to get WriteProtected and capacity information.
  //
  if (!EFI_ERROR (Status)) {
    if ((UsbMass->Pdt != USB_PDT_CDROM) && (CmdSet == USB_MASS_STORE_SCSI)) {
      //
      // MODE SENSE is required for the device with PDT of 0x00/0x07/0x0E,
      // according to Section 4 of USB Mass Storage Specification for Bootability.
      // MODE SENSE(10) is useless here, while MODE SENSE(6) defined in SCSI
      // could get the information of Write Protected.
      // Since not all device support this command, skip if fail.
      //
      UsbScsiModeSense (UsbMass);
    }

    Status = UsbBootReadCapacity (UsbMass);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "UsbBootDetectMedia: UsbBootReadCapacity (%r)\n", Status));
    }
  }

  if (EFI_ERROR (Status) && (Status != EFI_NO_MEDIA)) {
    //
    // For NoMedia, BlockIo is still needed.
    //
    return Status;
  }

  //
  // Simply reject device whose block size is unacceptable small (==0) or large (>64K).
  //
  if ((Media->BlockSize == 0) || (Media->BlockSize > USB_BOOT_MAX_CARRY_SIZE)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Detect whether it is necessary to reinstall the Block I/O Protocol.
  //
  // MediaId may change in RequestSense for MediaChanged
  // MediaPresent may change in RequestSense for NoMedia
  // MediaReadOnly may change in RequestSense for WriteProtected or MediaChanged
  // MediaPresent/BlockSize/LastBlock may change in ReadCapacity
  //
  if ((Media->MediaId != OldMedia.MediaId) ||
      (Media->MediaPresent != OldMedia.MediaPresent) ||
      (Media->ReadOnly != OldMedia.ReadOnly) ||
      (Media->BlockSize != OldMedia.BlockSize) ||
      (Media->LastBlock != OldMedia.LastBlock))
  {
    //
    // This function is called from:
    //   Block I/O Protocol APIs, which run at TPL_CALLBACK.
    //   DriverBindingStart(), which raises to TPL_CALLBACK.
    ASSERT (EfiGetCurrentTpl () == TPL_CALLBACK);

    //
    // When it is called from DriverBindingStart(), below reinstall fails.
    // So ignore the return status check.
    //
    gBS->ReinstallProtocolInterface (
           UsbMass->Controller,
           &gEfiBlockIoProtocolGuid,
           &UsbMass->BlockIo,
           &UsbMass->BlockIo
           );

    //
    // Reset MediaId after reinstalling Block I/O Protocol.
    //
    if (Media->MediaPresent != OldMedia.MediaPresent) {
      if (Media->MediaPresent) {
        Media->MediaId = 1;
      } else {
        Media->MediaId = 0;
      }
    }

    if ((Media->ReadOnly != OldMedia.ReadOnly) ||
        (Media->BlockSize != OldMedia.BlockSize) ||
        (Media->LastBlock != OldMedia.LastBlock))
    {
      Media->MediaId++;
    }

    Status = Media->MediaPresent ? EFI_MEDIA_CHANGED : EFI_NO_MEDIA;
  }

  return Status;
}

/**
  Read or write some blocks from the device.

  @param  UsbMass                The USB mass storage device to access
  @param  Write                  TRUE for write operation.
  @param  Lba                    The start block number
  @param  TotalBlock             Total block number to read or write
  @param  Buffer                 The buffer to read to or write from

  @retval EFI_SUCCESS            Data are read into the buffer or writen into the device.
  @retval Others                 Failed to read or write all the data

**/
EFI_STATUS
UsbBootReadWriteBlocks (
  IN  USB_MASS_DEVICE  *UsbMass,
  IN  BOOLEAN          Write,
  IN  UINT32           Lba,
  IN  UINTN            TotalBlock,
  IN OUT UINT8         *Buffer
  )
{
  USB_BOOT_READ_WRITE_10_CMD  Cmd;
  EFI_STATUS                  Status;
  UINT32                      Count;
  UINT32                      CountMax;
  UINT32                      BlockSize;
  UINT32                      ByteSize;
  UINT32                      Timeout;

  BlockSize = UsbMass->BlockIoMedia.BlockSize;
  CountMax  = USB_BOOT_MAX_CARRY_SIZE / BlockSize;
  Status    = EFI_SUCCESS;

  while (TotalBlock > 0) {
    //
    // Split the total blocks into smaller pieces to ease the pressure
    // on the device. We must split the total block because the READ10
    // command only has 16 bit transfer length (in the unit of block).
    //
    Count    = (UINT32)MIN (TotalBlock, CountMax);
    Count    = MIN (MAX_UINT16, Count);
    ByteSize = Count * BlockSize;

    //
    // USB command's upper limit timeout is 5s. [USB2.0-9.2.6.1]
    //
    Timeout = (UINT32)USB_BOOT_GENERAL_CMD_TIMEOUT;

    //
    // Fill in the command then execute
    //
    ZeroMem (&Cmd, sizeof (USB_BOOT_READ_WRITE_10_CMD));

    Cmd.OpCode = Write ? USB_BOOT_WRITE10_OPCODE : USB_BOOT_READ10_OPCODE;
    Cmd.Lun    = (UINT8)(USB_BOOT_LUN (UsbMass->Lun));
    WriteUnaligned32 ((UINT32 *)Cmd.Lba, SwapBytes32 (Lba));
    WriteUnaligned16 ((UINT16 *)Cmd.TransferLen, SwapBytes16 ((UINT16)Count));

    Status = UsbBootExecCmdWithRetry (
               UsbMass,
               &Cmd,
               (UINT8)sizeof (USB_BOOT_READ_WRITE_10_CMD),
               Write ? EfiUsbDataOut : EfiUsbDataIn,
               Buffer,
               ByteSize,
               Timeout
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    DEBUG ((
      DEBUG_BLKIO,
      "UsbBoot%sBlocks: LBA (0x%lx), Blk (0x%x)\n",
      Write ? L"Write" : L"Read",
      Lba,
      Count
      ));
    Lba        += Count;
    Buffer     += ByteSize;
    TotalBlock -= Count;
  }

  return Status;
}

/**
  Read or write some blocks from the device by SCSI 16 byte cmd.

  @param  UsbMass                The USB mass storage device to access
  @param  Write                  TRUE for write operation.
  @param  Lba                    The start block number
  @param  TotalBlock             Total block number to read or write
  @param  Buffer                 The buffer to read to or write from

  @retval EFI_SUCCESS            Data are read into the buffer or writen into the device.
  @retval Others                 Failed to read or write all the data
**/
EFI_STATUS
UsbBootReadWriteBlocks16 (
  IN  USB_MASS_DEVICE  *UsbMass,
  IN  BOOLEAN          Write,
  IN  UINT64           Lba,
  IN  UINTN            TotalBlock,
  IN OUT UINT8         *Buffer
  )
{
  UINT8       Cmd[16];
  EFI_STATUS  Status;
  UINT32      Count;
  UINT32      CountMax;
  UINT32      BlockSize;
  UINT32      ByteSize;
  UINT32      Timeout;

  BlockSize = UsbMass->BlockIoMedia.BlockSize;
  CountMax  = USB_BOOT_MAX_CARRY_SIZE / BlockSize;
  Status    = EFI_SUCCESS;

  while (TotalBlock > 0) {
    //
    // Split the total blocks into smaller pieces.
    //
    Count    = (UINT32)MIN (TotalBlock, CountMax);
    ByteSize = Count * BlockSize;

    //
    // USB command's upper limit timeout is 5s. [USB2.0-9.2.6.1]
    //
    Timeout = (UINT32)USB_BOOT_GENERAL_CMD_TIMEOUT;

    //
    // Fill in the command then execute
    //
    ZeroMem (Cmd, sizeof (Cmd));

    Cmd[0] = Write ? EFI_SCSI_OP_WRITE16 : EFI_SCSI_OP_READ16;
    Cmd[1] = (UINT8)((USB_BOOT_LUN (UsbMass->Lun) & 0xE0));
    WriteUnaligned64 ((UINT64 *)&Cmd[2], SwapBytes64 (Lba));
    WriteUnaligned32 ((UINT32 *)&Cmd[10], SwapBytes32 (Count));

    Status = UsbBootExecCmdWithRetry (
               UsbMass,
               Cmd,
               (UINT8)sizeof (Cmd),
               Write ? EfiUsbDataOut : EfiUsbDataIn,
               Buffer,
               ByteSize,
               Timeout
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    DEBUG ((
      DEBUG_BLKIO,
      "UsbBoot%sBlocks16: LBA (0x%lx), Blk (0x%x)\n",
      Write ? L"Write" : L"Read",
      Lba,
      Count
      ));
    Lba        += Count;
    Buffer     += ByteSize;
    TotalBlock -= Count;
  }

  return Status;
}

/**
  Use the USB clear feature control transfer to clear the endpoint stall condition.

  @param  UsbIo                  The USB I/O Protocol instance
  @param  EndpointAddr           The endpoint to clear stall for

  @retval EFI_SUCCESS            The endpoint stall condition is cleared.
  @retval Others                 Failed to clear the endpoint stall condition.

**/
EFI_STATUS
UsbClearEndpointStall (
  IN EFI_USB_IO_PROTOCOL  *UsbIo,
  IN UINT8                EndpointAddr
  )
{
  EFI_USB_DEVICE_REQUEST  Request;
  EFI_STATUS              Status;
  UINT32                  CmdResult;
  UINT32                  Timeout;

  Request.RequestType = 0x02;
  Request.Request     = USB_REQ_CLEAR_FEATURE;
  Request.Value       = USB_FEATURE_ENDPOINT_HALT;
  Request.Index       = EndpointAddr;
  Request.Length      = 0;
  Timeout             = USB_BOOT_GENERAL_CMD_TIMEOUT / USB_MASS_1_MILLISECOND;

  Status = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &Request,
                    EfiUsbNoData,
                    Timeout,
                    NULL,
                    0,
                    &CmdResult
                    );

  return Status;
}
