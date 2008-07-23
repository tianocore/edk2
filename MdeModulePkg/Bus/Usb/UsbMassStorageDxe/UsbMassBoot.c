/** @file

  This file implement the command set of "USB Mass Storage Specification
  for Bootability".

Copyright (c) 2007 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UsbMassImpl.h"


/**
  Return the current TPL.

  @return Current TPL.

**/
EFI_TPL
UsbGetCurrentTpl (
  VOID
  )
{
  EFI_TPL                 Tpl;

  Tpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (Tpl);

  return Tpl;
}

/**
  Read an UINT32 from the buffer to avoid byte alignment problems, then
  convert that to the little endia. The USB mass storage bootability spec
  use big endia.

  @param  Buf                    The buffer contains the first byte of the UINT32
                                 in big endia.

  @return The UINT32 value read from the buffer in little endia.

**/
UINT32
UsbBootGetUint32 (
  IN UINT8                  *Buf
  )
{
  UINT32                    Value;

  CopyMem (&Value, Buf, sizeof (UINT32));
  return USB_BOOT_SWAP32 (Value);
}


/**
  Put an UINT32 in little endia to the buffer. The data is converted to
  big endia before writing.

  @param  Buf                    The buffer to write data to.
  @param  Data32                 The data to write.

  @return None.

**/
VOID
UsbBootPutUint32 (
  IN UINT8                  *Buf,
  IN UINT32                 Data32
  )
{
  Data32 = USB_BOOT_SWAP32 (Data32);
  CopyMem (Buf, &Data32, sizeof (UINT32));
}

/**
  Put an UINT16 in little endia to the buffer. The data is converted to
  big endia before writing.

  @param  Buf                    The buffer to write data to.
  @param  Data16                 The data to write.

  @return None.

**/
VOID
UsbBootPutUint16 (
  IN UINT8                   *Buf,
  IN UINT16                  Data16
  )
{
  Data16 = (UINT16) (USB_BOOT_SWAP16 (Data16));
  CopyMem (Buf, &Data16, sizeof (UINT16));
}

/**
  Request sense information via sending Request Sense
  Packet Command.

  @param  UsbMass                The device to be requested sense data.

  @retval EFI_SUCCESS            The command is excuted OK.
  @retval EFI_DEVICE_ERROR       Failed to request sense.
  @retval EFI_NO_RESPONSE        The device media doesn't response this request.
  @retval EFI_INVALID_PARAMETER  The command has some invalid parameters.
  @retval EFI_WRITE_PROTECTED    The device is write protected.
  @retval EFI_MEDIA_CHANGED      The device media has been changed.

**/
EFI_STATUS
UsbBootRequestSense (
  IN USB_MASS_DEVICE          *UsbMass
  )
{
  USB_BOOT_REQUEST_SENSE_CMD  SenseCmd;
  USB_BOOT_REQUEST_SENSE_DATA SenseData;
  EFI_BLOCK_IO_MEDIA          *Media;
  USB_MASS_TRANSPORT          *Transport;
  EFI_STATUS                  Status;
  UINT32                      CmdResult;

  Transport = UsbMass->Transport;

  //
  // Request the sense data from the device if command failed
  //
  ZeroMem (&SenseCmd, sizeof (USB_BOOT_REQUEST_SENSE_CMD));
  ZeroMem (&SenseData, sizeof (USB_BOOT_REQUEST_SENSE_DATA));

  SenseCmd.OpCode   = USB_BOOT_REQUEST_SENSE_OPCODE;
  SenseCmd.Lun      = (UINT8) (USB_BOOT_LUN (UsbMass->Lun));
  SenseCmd.AllocLen = sizeof (USB_BOOT_REQUEST_SENSE_DATA);

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
  if (EFI_ERROR (Status) || CmdResult != USB_MASS_CMD_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "UsbBootRequestSense: (%r) CmdResult=0x%x\n", Status, CmdResult));
    return Status;
  }

  //
  // Interpret the sense data and update the media status if necessary.
  //
  Media = &UsbMass->BlockIoMedia;

  switch (USB_BOOT_SENSE_KEY (SenseData.SenseKey)) {

  case USB_BOOT_SENSE_NO_SENSE:
    Status = EFI_NO_RESPONSE;
    break;

  case USB_BOOT_SENSE_RECOVERED:
    //
    // Suppose hardware can handle this case, and recover later by itself
    //
    Status = EFI_NOT_READY;
    break;

  case USB_BOOT_SENSE_NOT_READY:
    Status = EFI_DEVICE_ERROR;
    if (SenseData.ASC == USB_BOOT_ASC_NO_MEDIA) {
          Media->MediaPresent = FALSE;
          Status              = EFI_NO_MEDIA;
    } else if (SenseData.ASC == USB_BOOT_ASC_NOT_READY) {
      Status              = EFI_NOT_READY;
    }
    break;

  case USB_BOOT_SENSE_ILLEGAL_REQUEST:
    Status = EFI_INVALID_PARAMETER;
    break;

  case USB_BOOT_SENSE_UNIT_ATTENTION:
    Status = EFI_DEVICE_ERROR;
    if (SenseData.ASC == USB_BOOT_ASC_MEDIA_CHANGE) {
      //
      // If MediaChange, reset ReadOnly and new MediId
      //
      Status          = EFI_MEDIA_CHANGED;
      Media->ReadOnly = FALSE;
      Media->MediaId++;
    }
    break;

  case USB_BOOT_SNESE_DATA_PROTECT:
    Status          = EFI_WRITE_PROTECTED;
    Media->ReadOnly = TRUE;
    break;

  default:
    Status = EFI_DEVICE_ERROR;
    break;
  }

  DEBUG ((EFI_D_INFO, "UsbBootRequestSense: (%r) with sense key %x/%x/%x\n",
          Status,
          USB_BOOT_SENSE_KEY (SenseData.SenseKey),
          SenseData.ASC,
          SenseData.ASCQ
          ));

  return Status;
}


/**
  Execute the USB mass storage bootability commands. If execution
  failed, retrieve the error by REQUEST_SENSE then update the device's
  status, such as ReadyOnly.

  @param  UsbMass                The device to issue commands to
  @param  Cmd                    The command to execute
  @param  CmdLen                 The length of the command
  @param  DataDir                The direction of data transfer
  @param  Data                   The buffer to hold the data
  @param  DataLen                The length of expected data
  @param  Timeout                The timeout used to transfer

  @retval EFI_SUCCESS            The command is excuted OK
  @retval EFI_DEVICE_ERROR       Failed to request sense
  @retval EFI_INVALID_PARAMETER  The command has some invalid parameters
  @retval EFI_WRITE_PROTECTED    The device is write protected
  @retval EFI_MEDIA_CHANGED      The device media has been changed

**/
EFI_STATUS
UsbBootExecCmd (
  IN USB_MASS_DEVICE            *UsbMass,
  IN VOID                       *Cmd,
  IN UINT8                      CmdLen,
  IN EFI_USB_DATA_DIRECTION     DataDir,
  IN VOID                       *Data,
  IN UINT32                     DataLen,
  IN UINT32                     Timeout
  )
{
  USB_MASS_TRANSPORT          *Transport;
  EFI_STATUS                  Status;
  UINT32                      CmdResult;

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
  //
  // ExecCommand return success and get the right CmdResult means
  // the commnad transfer is OK.
  //
  if ((CmdResult == USB_MASS_CMD_SUCCESS) && !EFI_ERROR(Status)) {
    return EFI_SUCCESS;
  }

  DEBUG ((EFI_D_INFO, "UsbBootExecCmd: Fail to Exec 0x%x Cmd /w %r\n",
          *(UINT8 *)Cmd ,Status));

  return UsbBootRequestSense (UsbMass);
}


/**
  Execute the USB mass storage bootability commands. If execution
  failed, retrieve the error by REQUEST_SENSE then update the device's
  status, such as ReadyOnly.

  @param  UsbMass                The device to issue commands to
  @param  Cmd                    The command to execute
  @param  CmdLen                 The length of the command
  @param  DataDir                The direction of data transfer
  @param  Data                   The buffer to hold the data
  @param  DataLen                The length of expected data
  @param  Timeout                The timeout used to transfer

  @retval EFI_SUCCESS            The command is excuted OK
  @retval EFI_DEVICE_ERROR       Failed to request sense
  @retval EFI_INVALID_PARAMETER  The command has some invalid parameters
  @retval EFI_WRITE_PROTECTED    The device is write protected
  @retval EFI_MEDIA_CHANGED      The device media has been changed

**/
EFI_STATUS
UsbBootExecCmdWithRetry (
  IN USB_MASS_DEVICE          *UsbMass,
  IN VOID                     *Cmd,
  IN UINT8                    CmdLen,
  IN EFI_USB_DATA_DIRECTION   DataDir,
  IN VOID                     *Data,
  IN UINT32                   DataLen,
  IN UINT32                   Timeout
  )
{
  EFI_STATUS                  Status;
  INT16                       Index;

  //
  // If the device isn't ready, wait some time. If the device is ready,
  // retry the command again.
  //
  Status  = EFI_SUCCESS;

  for (Index = 0; Index < USB_BOOT_COMMAND_RETRY; Index++) {
    //
    // Execute the command with an increasingly larger timeout value.
    //
    Status = UsbBootExecCmd (
               UsbMass,
               Cmd,
               CmdLen,
               DataDir,
               Data,
               DataLen,
               Timeout
               );
    if (Status == EFI_SUCCESS ||
        Status == EFI_MEDIA_CHANGED) {
      break;
    }
    //
    // Need retry once more, so reset index
    //
    if (Status == EFI_NOT_READY) {
      Index = 0;
    }
  }

  return Status;
}


/**
  Use the TEST UNIT READY command to check whether it is ready.
  If it is ready, update the parameters.

  @param  UsbMass                The device to test

  @retval EFI_SUCCESS            The device is ready and parameters are updated.
  @retval Others                 Device not ready.

**/
EFI_STATUS
UsbBootIsUnitReady (
  IN USB_MASS_DEVICE            *UsbMass
  )
{
  USB_BOOT_TEST_UNIT_READY_CMD  TestCmd;

  ZeroMem (&TestCmd, sizeof (USB_BOOT_TEST_UNIT_READY_CMD));

  TestCmd.OpCode  = USB_BOOT_TEST_UNIT_READY_OPCODE;
  TestCmd.Lun     = (UINT8) (USB_BOOT_LUN (UsbMass->Lun));

  return UsbBootExecCmdWithRetry (
           UsbMass,
           &TestCmd,
           sizeof (USB_BOOT_TEST_UNIT_READY_CMD),
           EfiUsbNoData,
           NULL,
           0,
           USB_BOOT_GENERAL_CMD_TIMEOUT
           );
}


/**
  Inquiry Command requests that information regrarding parameters of
  the Device be sent to the Host.

  @param  UsbMass                The device to inquiry.

  @retval EFI_SUCCESS            The device is ready and parameters are updated.
  @retval Others                 Device not ready.

**/
EFI_STATUS
UsbBootInquiry (
  IN USB_MASS_DEVICE            *UsbMass
  )
{
  USB_BOOT_INQUIRY_CMD        InquiryCmd;
  USB_BOOT_INQUIRY_DATA       InquiryData;
  EFI_BLOCK_IO_MEDIA          *Media;
  EFI_STATUS                  Status;

  Media = &(UsbMass->BlockIoMedia);

  //
  // Use the Inquiry command to get the RemovableMedia setting.
  //
  ZeroMem (&InquiryCmd, sizeof (USB_BOOT_INQUIRY_CMD));
  ZeroMem (&InquiryData, sizeof (USB_BOOT_INQUIRY_DATA));

  InquiryCmd.OpCode   = USB_BOOT_INQUIRY_OPCODE;
  InquiryCmd.Lun      = (UINT8) (USB_BOOT_LUN (UsbMass->Lun));
  InquiryCmd.AllocLen = sizeof (InquiryData);

  Status = UsbBootExecCmdWithRetry (
             UsbMass,
             &InquiryCmd,
             sizeof (USB_BOOT_INQUIRY_CMD),
             EfiUsbDataIn,
             &InquiryData,
             sizeof (USB_BOOT_INQUIRY_DATA),
             USB_BOOT_GENERAL_CMD_TIMEOUT
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  UsbMass->Pdt          = (UINT8) (USB_BOOT_PDT (InquiryData.Pdt));
  Media->RemovableMedia = (BOOLEAN) (USB_BOOT_REMOVABLE (InquiryData.Removable));
  //
  // Default value 512 Bytes, in case no media present at first time
  //
  Media->BlockSize      = 0x0200;

  return Status;
}


/**
  Get the capacity of the USB mass storage media, including
  the presentation, block size, and last block number. This
  function is used to get the disk parameters at the start if
  it is a non-removable media or to detect the media if it is
  removable.

  @param  UsbMass                The device to retireve disk gemotric.

  @retval EFI_SUCCESS            The disk gemotric is successfully retrieved.
  @retval EFI_DEVICE_ERROR       Something is inconsistent with the disk gemotric.
  @retval Other                  Read capacity request fails.
 
**/
EFI_STATUS
UsbBootReadCapacity (
  IN USB_MASS_DEVICE          *UsbMass
  )
{
  USB_BOOT_READ_CAPACITY_CMD  CapacityCmd;
  USB_BOOT_READ_CAPACITY_DATA CapacityData;
  EFI_BLOCK_IO_MEDIA          *Media;
  EFI_STATUS                  Status;

  Media   = &UsbMass->BlockIoMedia;

  //
  // Use the READ CAPACITY command to get the block length and last blockno
  //
  ZeroMem (&CapacityCmd, sizeof (USB_BOOT_READ_CAPACITY_CMD));
  ZeroMem (&CapacityData, sizeof (USB_BOOT_READ_CAPACITY_DATA));

  CapacityCmd.OpCode = USB_BOOT_READ_CAPACITY_OPCODE;
  CapacityCmd.Lun    = (UINT8) (USB_BOOT_LUN (UsbMass->Lun));

  Status = UsbBootExecCmdWithRetry (
             UsbMass,
             &CapacityCmd,
             sizeof (USB_BOOT_READ_CAPACITY_CMD),
             EfiUsbDataIn,
             &CapacityData,
             sizeof (USB_BOOT_READ_CAPACITY_DATA),
             USB_BOOT_GENERAL_CMD_TIMEOUT
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Media->MediaPresent = TRUE;
  Media->LastBlock    = UsbBootGetUint32 (CapacityData.LastLba);
  Media->BlockSize    = UsbBootGetUint32 (CapacityData.BlockLen);

  if (Media->BlockSize == 0) {
    return EFI_NOT_READY;
  }

  DEBUG ((EFI_D_INFO, "UsbBootReadCapacity Success LBA=%ld BlockSize=%d\n",
          Media->LastBlock, Media->BlockSize));

  return EFI_SUCCESS;
}

/**
  Retrieves mode sense information via sending Mode Sense
  Packet Command.

  @param  UsbMass                The USB_FLOPPY_DEV instance.

  @retval EFI_SUCCESS            Success
  @retval Other                  Execute Request command fails.

**/
EFI_STATUS
UsbScsiModeSense (
  IN USB_MASS_DEVICE          *UsbMass
  )
{
  EFI_STATUS                       Status;
  USB_SCSI_MODE_SENSE6_CMD         ModeSenseCmd;
  USB_SCSI_MODE_SENSE6_PARA_HEADER ModeParaHeader;
  EFI_BLOCK_IO_MEDIA               *Media;

  Media   = &UsbMass->BlockIoMedia;

  ZeroMem (&ModeSenseCmd, sizeof (USB_SCSI_MODE_SENSE6_CMD));
  ZeroMem (&ModeParaHeader, sizeof (USB_SCSI_MODE_SENSE6_PARA_HEADER));

  //
  // ModeSense6 command is defined in [SCSI2Spec-Page151]
  //
  ModeSenseCmd.OpCode         = USB_SCSI_MODE_SENSE6_OPCODE;
  ModeSenseCmd.Lun            = (UINT8) USB_BOOT_LUN (UsbMass->Lun);
  ModeSenseCmd.PageCode       = 0x3F;
  ModeSenseCmd.AllocateLen    = (UINT8) sizeof (USB_SCSI_MODE_SENSE6_PARA_HEADER);

  Status = UsbBootExecCmdWithRetry (
             UsbMass,
             &ModeSenseCmd,
             sizeof (USB_SCSI_MODE_SENSE6_CMD),
             EfiUsbDataIn,
             &ModeParaHeader,
             sizeof (USB_SCSI_MODE_SENSE6_PARA_HEADER),
             USB_BOOT_GENERAL_CMD_TIMEOUT
             );

  //
  // ModeSense(6) is used to get the information of WriteProtected. While only some of
  // devices support this command, so have a try here.
  //
  if (!EFI_ERROR (Status)) {
    Media->ReadOnly = (BOOLEAN) (((ModeParaHeader.DevicePara & 0x80) != 0) ? TRUE : FALSE);
  }

  return Status;
}


/**
  Get the parameters for the USB mass storage media, including
  the RemovableMedia, block size, and last block number. This
  function is used both to initialize the media during the
  DriverBindingStart and to re-initialize it when the media is
  changed. Althought the RemoveableMedia is unlikely to change,
  I include it here.

  @param  UsbMass                The device to retireve disk gemotric.

  @retval EFI_SUCCESS            The disk gemotric is successfully retrieved.
  @retval Other                  Get the parameters failed.

**/
EFI_STATUS
UsbBootGetParams (
  IN USB_MASS_DEVICE          *UsbMass
  )
{
  EFI_BLOCK_IO_MEDIA          *Media;
  EFI_STATUS                  Status;
  UINT8                       CmdSet;

  Media  = &(UsbMass->BlockIoMedia);
  CmdSet = ((EFI_USB_INTERFACE_DESCRIPTOR *) (UsbMass->Context))->InterfaceSubClass;

  Status = UsbBootInquiry (UsbMass);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "UsbBootGetParams: UsbBootInquiry (%r)\n", Status));
    return Status;
  }

  //
  // Don't use the Removable bit in inquirydata to test whether the media
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
    Media->BlockSize        = 0x0800;
  }

  if ((UsbMass->Pdt != USB_PDT_CDROM) && (CmdSet == USB_MASS_STORE_SCSI)) {
    //
    // ModeSense is required for the device with PDT of 0x00/0x07/0x0E,
    // which is from [MassStorageBootabilitySpec-Page7].
    // ModeSense(10) is useless here, while ModeSense(6) defined in SCSI
    // could get the information of WriteProtected.
    // Since not all device support this command, so skip if fail.
    //
    UsbScsiModeSense (UsbMass);
  }

  return UsbBootReadCapacity (UsbMass);
}


/**
  Detect whether the removable media is present and whether it has changed.
  The Non-removable media doesn't need it.

  @param  UsbMass                The device to retireve disk gemotric.

  @retval EFI_SUCCESS            The disk gemotric is successfully retrieved.
  @retval Other                  Decect media fails.

**/
EFI_STATUS
UsbBootDetectMedia (
  IN  USB_MASS_DEVICE       *UsbMass
  )
{
  EFI_BLOCK_IO_MEDIA        OldMedia;
  EFI_BLOCK_IO_MEDIA        *Media;
  UINT8                     CmdSet;
  EFI_TPL                   OldTpl;
  EFI_STATUS                Status;

  Media    = &UsbMass->BlockIoMedia;

  CopyMem (
    &OldMedia,
    &(UsbMass->BlockIoMedia),
    sizeof (EFI_BLOCK_IO_MEDIA)
    );

  CmdSet = ((EFI_USB_INTERFACE_DESCRIPTOR *) (UsbMass->Context))->InterfaceSubClass;

  Status = UsbBootIsUnitReady (UsbMass);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "UsbBootDetectMedia: UsbBootIsUnitReady (%r)\n", Status));
    goto ON_ERROR;
  }

  if ((UsbMass->Pdt != USB_PDT_CDROM) && (CmdSet == USB_MASS_STORE_SCSI)) {
    //
    // ModeSense is required for the device with PDT of 0x00/0x07/0x0E,
    // which is from [MassStorageBootabilitySpec-Page7].
    // ModeSense(10) is useless here, while ModeSense(6) defined in SCSI
    // could get the information of WriteProtected.
    // Since not all device support this command, so skip if fail.
    //
    UsbScsiModeSense (UsbMass);
  }

  Status = UsbBootReadCapacity (UsbMass);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "UsbBootDetectMedia: UsbBootReadCapacity (%r)\n", Status));
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:
  //
  // Detect whether it is necessary to reinstall the BlockIO
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
      (Media->LastBlock != OldMedia.LastBlock)) {

    OldTpl = UsbGetCurrentTpl ();
    DEBUG ((EFI_D_ERROR, "UsbBootDetectMedia: TPL before reinstall BlockIoProtocol is %d\n", OldTpl));

    gBS->RestoreTPL (TPL_CALLBACK);

    gBS->ReinstallProtocolInterface (
           UsbMass->Controller,
           &gEfiBlockIoProtocolGuid,
           &UsbMass->BlockIo,
           &UsbMass->BlockIo
           );

    DEBUG ((EFI_D_ERROR, "UsbBootDetectMedia: TPL after reinstall is %d\n", UsbGetCurrentTpl()));
    ASSERT (UsbGetCurrentTpl () == TPL_CALLBACK);

    gBS->RaiseTPL (OldTpl);

    //
    // Update MediaId after reinstall BLOCK_IO_PROTOCOL
    //
    if (Media->MediaPresent != OldMedia.MediaPresent) {
      if (Media->MediaPresent == TRUE) {
        Media->MediaId = 1;
      } else {
        Media->MediaId = 0;
      }
    }

    if ((Media->ReadOnly != OldMedia.ReadOnly) ||
        (Media->BlockSize != OldMedia.BlockSize) ||
        (Media->LastBlock != OldMedia.LastBlock)) {
      Media->MediaId++;
    }
  }

  return Status;
}


/**
  Read some blocks from the device.

  @param  UsbMass                The USB mass storage device to read from
  @param  Lba                    The start block number
  @param  TotalBlock             Total block number to read
  @param  Buffer                 The buffer to read to

  @retval EFI_SUCCESS            Data are read into the buffer
  @retval Others                 Failed to read all the data

**/
EFI_STATUS
UsbBootReadBlocks (
  IN  USB_MASS_DEVICE       *UsbMass,
  IN  UINT32                Lba,
  IN  UINTN                 TotalBlock,
  OUT UINT8                 *Buffer
  )
{
  USB_BOOT_READ10_CMD       ReadCmd;
  EFI_STATUS                Status;
  UINT16                    Count;
  UINT32                    BlockSize;
  UINT32                    ByteSize;
  UINT32                    Timeout;

  BlockSize = UsbMass->BlockIoMedia.BlockSize;
  Status    = EFI_SUCCESS;

  while (TotalBlock > 0) {
    //
    // Split the total blocks into smaller pieces to ease the pressure
    // on the device. We must split the total block because the READ10
    // command only has 16 bit transfer length (in the unit of block).
    //
    Count     = (UINT16)((TotalBlock < USB_BOOT_IO_BLOCKS) ? TotalBlock : USB_BOOT_IO_BLOCKS);
    ByteSize  = (UINT32)Count * BlockSize;

    //
    // USB command's upper limit timeout is 5s. [USB2.0-9.2.6.1]
    //
    Timeout = (UINT32) USB_BOOT_GENERAL_CMD_TIMEOUT;

    //
    // Fill in the command then execute
    //
    ZeroMem (&ReadCmd, sizeof (USB_BOOT_READ10_CMD));

    ReadCmd.OpCode  = USB_BOOT_READ10_OPCODE;
    ReadCmd.Lun     = (UINT8) (USB_BOOT_LUN (UsbMass->Lun));
    UsbBootPutUint32 (ReadCmd.Lba, Lba);
    UsbBootPutUint16 (ReadCmd.TransferLen, Count);

    Status = UsbBootExecCmdWithRetry (
               UsbMass,
               &ReadCmd,
               sizeof (USB_BOOT_READ10_CMD),
               EfiUsbDataIn,
               Buffer,
               ByteSize,
               Timeout
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Lba        += Count;
    Buffer     += Count * BlockSize;
    TotalBlock -= Count;
  }

  return Status;
}


/**
  Write some blocks to the device.

  @param  UsbMass                The USB mass storage device to write to
  @param  Lba                    The start block number
  @param  TotalBlock             Total block number to write
  @param  Buffer                 The buffer to write to

  @retval EFI_SUCCESS            Data are written into the buffer
  @retval Others                 Failed to write all the data

**/
EFI_STATUS
UsbBootWriteBlocks (
  IN  USB_MASS_DEVICE         *UsbMass,
  IN  UINT32                  Lba,
  IN  UINTN                   TotalBlock,
  OUT UINT8                   *Buffer
  )
{
  USB_BOOT_WRITE10_CMD  WriteCmd;
  EFI_STATUS            Status;
  UINT16                Count;
  UINT32                BlockSize;
  UINT32                ByteSize;
  UINT32                Timeout;

  BlockSize = UsbMass->BlockIoMedia.BlockSize;
  Status    = EFI_SUCCESS;

  while (TotalBlock > 0) {
    //
    // Split the total blocks into smaller pieces to ease the pressure
    // on the device. We must split the total block because the WRITE10
    // command only has 16 bit transfer length (in the unit of block).
    //
    Count     = (UINT16)((TotalBlock < USB_BOOT_IO_BLOCKS) ? TotalBlock : USB_BOOT_IO_BLOCKS);
    ByteSize  = (UINT32)Count * BlockSize;

    //
    // USB command's upper limit timeout is 5s. [USB2.0-9.2.6.1]
    //
    Timeout = (UINT32) USB_BOOT_GENERAL_CMD_TIMEOUT;

    //
    // Fill in the write10 command block
    //
    ZeroMem (&WriteCmd, sizeof (USB_BOOT_WRITE10_CMD));

    WriteCmd.OpCode = USB_BOOT_WRITE10_OPCODE;
    WriteCmd.Lun    = (UINT8) (USB_BOOT_LUN (UsbMass->Lun));
    UsbBootPutUint32 (WriteCmd.Lba, Lba);
    UsbBootPutUint16 (WriteCmd.TransferLen, Count);

    Status = UsbBootExecCmdWithRetry (
               UsbMass,
               &WriteCmd,
               sizeof (USB_BOOT_WRITE10_CMD),
               EfiUsbDataOut,
               Buffer,
               ByteSize,
               Timeout
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Lba        += Count;
    Buffer     += Count * BlockSize;
    TotalBlock -= Count;
  }

  return Status;
}


/**
  Use the USB clear feature control transfer to clear the endpoint
  stall condition.

  @param  UsbIo                  The USB IO protocol to use
  @param  EndpointAddr           The endpoint to clear stall for

  @retval EFI_SUCCESS            The endpoint stall condtion is clear
  @retval Others                 Failed to clear the endpoint stall condtion

**/
EFI_STATUS
UsbClearEndpointStall (
  IN EFI_USB_IO_PROTOCOL    *UsbIo,
  IN UINT8                  EndpointAddr
  )
{
  EFI_USB_DEVICE_REQUEST    Request;
  EFI_STATUS                Status;
  UINT32                    CmdResult;
  UINT32                    Timeout;

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
