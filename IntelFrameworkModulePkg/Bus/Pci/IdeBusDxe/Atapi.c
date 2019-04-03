/** @file
   This file contains all helper functions on the ATAPI command

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "IdeBus.h"

/**
  This function is used to get the current status of the media residing
  in the LS-120 drive or ZIP drive. The media status is returned in the
  Error Status.

  @param IdeDev   pointer pointing to IDE_BLK_IO_DEV data structure, used
                  to record all the information of the IDE device.

  @retval EFI_SUCCESS         The media status is achieved successfully and the media
                              can be read/written.
  @retval EFI_DEVICE_ERROR    Get Media Status Command is failed.
  @retval EFI_NO_MEDIA        There is no media in the drive.
  @retval EFI_WRITE_PROTECTED The media is writing protected.

  @note  This function must be called after the LS120EnableMediaStatus()
         with second parameter set to TRUE
         (means enable media status notification) is called.
**/
EFI_STATUS
LS120GetMediaStatus (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
{
  UINT8       DeviceSelect;
  UINT8       StatusValue;
  EFI_STATUS  EfiStatus;
  //
  // Poll Alternate Register for BSY clear within timeout.
  //
  EfiStatus = WaitForBSYClear2 (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (EfiStatus)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Select device via Device/Head Register.
  //
  DeviceSelect = (UINT8) ((IdeDev->Device) << 4 | 0xe0);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Head, DeviceSelect);

  //
  // Poll Alternate Register for DRDY set within timeout.
  // After device is selected, DRDY set indicates the device is ready to
  // accept command.
  //
  EfiStatus = DRDYReady2 (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (EfiStatus)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Get Media Status Command is sent
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Command, 0xDA);

  //
  // BSY bit will clear after command is complete.
  //
  EfiStatus = WaitForBSYClear2 (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (EfiStatus)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // the media status is returned by the command in the ERROR register
  //
  StatusValue = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Error);

  if ((StatusValue & BIT1) != 0) {
    return EFI_NO_MEDIA;
  }

  if ((StatusValue & BIT6) != 0) {
    return EFI_WRITE_PROTECTED;
  } else {
    return EFI_SUCCESS;
  }
}
/**
  This function is used to send Enable Media Status Notification Command
  or Disable Media Status Notification Command.

  @param IdeDev pointer pointing to IDE_BLK_IO_DEV data structure, used
                to record all the information of the IDE device.

  @param Enable a flag that indicates whether enable or disable media
                status notification.
  @retval EFI_SUCCESS      If command completes successfully.
  @retval EFI_DEVICE_ERROR If command failed.
**/
EFI_STATUS
LS120EnableMediaStatus (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  BOOLEAN         Enable
  )
{
  UINT8       DeviceSelect;
  EFI_STATUS  Status;

  //
  // Poll Alternate Register for BSY clear within timeout.
  //
  Status = WaitForBSYClear2 (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Select device via Device/Head Register.
  //
  DeviceSelect = (UINT8) ((IdeDev->Device) << 4 | 0xe0);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Head, DeviceSelect);

  //
  // Poll Alternate Register for DRDY set within timeout.
  // After device is selected, DRDY set indicates the device is ready to
  // accept command.
  //
  Status = DRDYReady2 (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  if (Enable) {
    //
    // 0x95: Enable media status notification
    //
    IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Feature, 0x95);
  } else {
    //
    // 0x31: Disable media status notification
    //
    IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Feature, 0x31);
  }
  //
  // Set Feature Command is sent
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Command, 0xEF);

  //
  // BSY bit will clear after command is complete.
  //
  Status = WaitForBSYClear (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}
/**
  This function reads the pending data in the device.

  @param IdeDev   Indicates the calling context.

  @retval EFI_SUCCESS   Successfully read.
  @retval EFI_NOT_READY The BSY is set avoiding reading.

**/
EFI_STATUS
AtapiReadPendingData (
  IN IDE_BLK_IO_DEV     *IdeDev
  )
{
  UINT8     AltRegister;
  UINT16    TempWordBuffer;

  AltRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Alt.AltStatus);
  if ((AltRegister & ATA_STSREG_BSY) == ATA_STSREG_BSY) {
    return EFI_NOT_READY;
  }
  if ((AltRegister & (ATA_STSREG_BSY | ATA_STSREG_DRQ)) == ATA_STSREG_DRQ) {
    TempWordBuffer = IDEReadPortB (IdeDev->PciIo,IdeDev->IoPort->Alt.AltStatus);
    while ((TempWordBuffer & (ATA_STSREG_BSY | ATA_STSREG_DRQ)) == ATA_STSREG_DRQ) {
      IDEReadPortWMultiple (
        IdeDev->PciIo,
        IdeDev->IoPort->Data,
        1,
        &TempWordBuffer
        );
      TempWordBuffer = IDEReadPortB (IdeDev->PciIo,IdeDev->IoPort->Alt.AltStatus);
    }
  }
  return EFI_SUCCESS;
}

/**
  This function is called by either AtapiPacketCommandIn() or AtapiPacketCommandOut().
  It is used to transfer data between host and device. The data direction is specified
  by the fourth parameter.

  @param IdeDev     pointer pointing to IDE_BLK_IO_DEV data structure, used to record
                    all the information of the IDE device.
  @param Buffer     buffer contained data transferred between host and device.
  @param ByteCount  data size in byte unit of the buffer.
  @param Read       flag used to determine the data transfer direction.
                    Read equals 1, means data transferred from device to host;
                    Read equals 0, means data transferred from host to device.
  @param TimeOut    timeout value for wait DRQ ready before each data stream's transfer.

  @retval EFI_SUCCESS      data is transferred successfully.
  @retval EFI_DEVICE_ERROR the device failed to transfer data.
**/
EFI_STATUS
PioReadWriteData (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINT16          *Buffer,
  IN  UINT32          ByteCount,
  IN  BOOLEAN         Read,
  IN  UINTN           TimeOut
  )
{
  //
  // required transfer data in word unit.
  //
  UINT32      RequiredWordCount;

  //
  // actual transfer data in word unit.
  //
  UINT32      ActualWordCount;
  UINT32      WordCount;
  EFI_STATUS  Status;
  UINT16      *PtrBuffer;

  //
  // No data transfer is premitted.
  //
  if (ByteCount == 0) {
    return EFI_SUCCESS;
  }
  //
  // for performance, we assert the ByteCount is an even number
  // which is actually a resonable assumption
  ASSERT((ByteCount%2) == 0);

  PtrBuffer         = Buffer;
  RequiredWordCount = ByteCount / 2;
  //
  // ActuralWordCount means the word count of data really transferred.
  //
  ActualWordCount = 0;

  while (ActualWordCount < RequiredWordCount) {

    //
    // before each data transfer stream, the host should poll DRQ bit ready,
    // to see whether indicates device is ready to transfer data.
    //
    Status = DRQReady2 (IdeDev, TimeOut);
    if (EFI_ERROR (Status)) {
      return CheckErrorStatus (IdeDev);
    }

    //
    // read Status Register will clear interrupt
    //
    IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Status);

    //
    // get current data transfer size from Cylinder Registers.
    //
    WordCount = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->CylinderMsb) << 8;
    WordCount = WordCount | IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->CylinderLsb);
    WordCount = WordCount & 0xffff;
    WordCount /= 2;

    WordCount = MIN (WordCount, (RequiredWordCount - ActualWordCount));

    if (Read) {
      IDEReadPortWMultiple (
        IdeDev->PciIo,
        IdeDev->IoPort->Data,
        WordCount,
        PtrBuffer
        );
    } else {
      IDEWritePortWMultiple (
        IdeDev->PciIo,
        IdeDev->IoPort->Data,
        WordCount,
        PtrBuffer
        );
    }

    PtrBuffer += WordCount;
    ActualWordCount += WordCount;
  }

  if (Read) {
    //
    // In the case where the drive wants to send more data than we need to read,
    // the DRQ bit will be set and cause delays from DRQClear2().
    // We need to read data from the drive until it clears DRQ so we can move on.
    //
    AtapiReadPendingData (IdeDev);
  }

  //
  // After data transfer is completed, normally, DRQ bit should clear.
  //
  Status = DRQClear2 (IdeDev, ATAPITIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // read status register to check whether error happens.
  //
  return CheckErrorStatus (IdeDev);
}

/**
  This function is used to send out ATAPI commands conforms to the Packet Command
  with PIO Data In Protocol.

  @param IdeDev    pointer pointing to IDE_BLK_IO_DEV data structure, used
                   to record all the information of the IDE device.
  @param Packet    pointer pointing to ATAPI_PACKET_COMMAND data structure
                   which contains the contents of the command.
  @param Buffer    buffer contained data transferred from device to host.
  @param ByteCount data size in byte unit of the buffer.
  @param TimeOut   this parameter is used to specify the timeout value for the
                   PioReadWriteData() function.

  @retval EFI_SUCCESS       send out the ATAPI packet command successfully
                            and device sends data successfully.
  @retval EFI_DEVICE_ERROR  the device failed to send data.

**/
EFI_STATUS
AtapiPacketCommandIn (
  IN  IDE_BLK_IO_DEV        *IdeDev,
  IN  ATAPI_PACKET_COMMAND  *Packet,
  IN  UINT16                *Buffer,
  IN  UINT32                ByteCount,
  IN  UINTN                 TimeOut
  )
{
  UINT16      *CommandIndex;
  EFI_STATUS  Status;
  UINT32      Count;

  //
  // Set all the command parameters by fill related registers.
  // Before write to all the following registers, BSY and DRQ must be 0.
  //
  Status = DRQClear2 (IdeDev, ATAPITIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Select device via Device/Head Register.
  //
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->Head,
    (UINT8) ((IdeDev->Device << 4) | ATA_DEFAULT_CMD)  // DEFAULT_CMD: 0xa0 (1010,0000)
    );

  //
  // No OVL; No DMA
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Feature, 0x00);

  //
  // set the transfersize to ATAPI_MAX_BYTE_COUNT to let the device
  // determine how many data should be transferred.
  //
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->CylinderLsb,
    (UINT8) (ATAPI_MAX_BYTE_COUNT & 0x00ff)
    );
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->CylinderMsb,
    (UINT8) (ATAPI_MAX_BYTE_COUNT >> 8)
    );

  //
  //  ATA_DEFAULT_CTL:0x0a (0000,1010)
  //  Disable interrupt
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Alt.DeviceControl, ATA_DEFAULT_CTL);

  //
  // Send Packet command to inform device
  // that the following data bytes are command packet.
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Command, ATA_CMD_PACKET);

  Status = DRQReady (IdeDev, ATAPITIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Send out command packet
  //
  CommandIndex = Packet->Data16;
  for (Count = 0; Count < 6; Count++, CommandIndex++) {

    IDEWritePortW (IdeDev->PciIo, IdeDev->IoPort->Data, *CommandIndex);
    gBS->Stall (10);
  }

  //
  // call PioReadWriteData() function to get
  // requested transfer data form device.
  //
  return PioReadWriteData (IdeDev, Buffer, ByteCount, 1, TimeOut);
}
/**
  This function is used to send out ATAPI commands conforms to the Packet Command
  with PIO Data Out Protocol.

  @param IdeDev      pointer pointing to IDE_BLK_IO_DEV data structure, used
                     to record all the information of the IDE device.
  @param Packet      pointer pointing to ATAPI_PACKET_COMMAND data structure
                     which contains the contents of the command.
  @param Buffer      buffer contained data transferred from host to device.
  @param ByteCount   data size in byte unit of the buffer.
  @param TimeOut     this parameter is used to specify the timeout value
                     for the PioReadWriteData() function.
  @retval EFI_SUCCESS      send out the ATAPI packet command successfully
                           and device received data successfully.
  @retval EFI_DEVICE_ERROR the device failed to send data.

**/
EFI_STATUS
AtapiPacketCommandOut (
  IN  IDE_BLK_IO_DEV        *IdeDev,
  IN  ATAPI_PACKET_COMMAND  *Packet,
  IN  UINT16                *Buffer,
  IN  UINT32                ByteCount,
  IN  UINTN                 TimeOut
  )
{
  UINT16      *CommandIndex;
  EFI_STATUS  Status;
  UINT32      Count;

  //
  // set all the command parameters
  // Before write to all the following registers, BSY and DRQ must be 0.
  //
  Status = DRQClear2 (IdeDev, ATAPITIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Select device via Device/Head Register.
  //
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->Head,
    (UINT8) ((IdeDev->Device << 4) | ATA_DEFAULT_CMD)   // ATA_DEFAULT_CMD: 0xa0 (1010,0000)
    );

  //
  // No OVL; No DMA
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Feature, 0x00);

  //
  // set the transfersize to ATAPI_MAX_BYTE_COUNT to
  // let the device determine how many data should be transferred.
  //
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->CylinderLsb,
    (UINT8) (ATAPI_MAX_BYTE_COUNT & 0x00ff)
    );
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->CylinderMsb,
    (UINT8) (ATAPI_MAX_BYTE_COUNT >> 8)
    );

  //
  //  DEFAULT_CTL:0x0a (0000,1010)
  //  Disable interrupt
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Alt.DeviceControl, ATA_DEFAULT_CTL);

  //
  // Send Packet command to inform device
  // that the following data bytes are command packet.
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Command, ATA_CMD_PACKET);

  Status = DRQReady2 (IdeDev, ATAPITIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Send out command packet
  //
  CommandIndex = Packet->Data16;
  for (Count = 0; Count < 6; Count++, CommandIndex++) {
    IDEWritePortW (IdeDev->PciIo, IdeDev->IoPort->Data, *CommandIndex);
    gBS->Stall (10);
  }

  //
  // call PioReadWriteData() function to send requested transfer data to device.
  //
  return PioReadWriteData (IdeDev, Buffer, ByteCount, 0, TimeOut);
}
/**
  Sends out ATAPI Inquiry Packet Command to the specified device. This command will
  return INQUIRY data of the device.

  @param IdeDev pointer pointing to IDE_BLK_IO_DEV data structure, used
                to record all the information of the IDE device.

  @retval EFI_SUCCESS       Inquiry command completes successfully.
  @retval EFI_DEVICE_ERROR  Inquiry command failed.

  @note  Parameter "IdeDev" will be updated in this function.

**/
EFI_STATUS
AtapiInquiry (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
{
  ATAPI_PACKET_COMMAND  Packet;
  EFI_STATUS            Status;
  ATAPI_INQUIRY_DATA          *InquiryData;

  //
  // prepare command packet for the ATAPI Inquiry Packet Command.
  //
  ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
  Packet.Inquiry.opcode             = ATA_CMD_INQUIRY;
  Packet.Inquiry.page_code          = 0;
  Packet.Inquiry.allocation_length  = (UINT8) sizeof (ATAPI_INQUIRY_DATA);

  InquiryData                       = AllocatePool (sizeof (ATAPI_INQUIRY_DATA));
  if (InquiryData == NULL) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Send command packet and get requested Inquiry data.
  //
  Status = AtapiPacketCommandIn (
            IdeDev,
            &Packet,
            (UINT16 *) InquiryData,
            sizeof (ATAPI_INQUIRY_DATA),
            ATAPITIMEOUT
            );
  if (EFI_ERROR (Status)) {
    gBS->FreePool (InquiryData);
    return EFI_DEVICE_ERROR;
  }

  IdeDev->InquiryData = InquiryData;

  return EFI_SUCCESS;
}
/**
  This function is called by DiscoverIdeDevice() during its device
  identification.
  Its main purpose is to get enough information for the device media
  to fill in the Media data structure of the Block I/O Protocol interface.

  There are 5 steps to reach such objective:
  1. Sends out the ATAPI Identify Command to the specified device.
  Only ATAPI device responses to this command. If the command succeeds,
  it returns the Identify data structure which filled with information
  about the device. Since the ATAPI device contains removable media,
  the only meaningful information is the device module name.
  2. Sends out ATAPI Inquiry Packet Command to the specified device.
  This command will return inquiry data of the device, which contains
  the device type information.
  3. Allocate sense data space for future use. We don't detect the media
  presence here to improvement boot performance, especially when CD
  media is present. The media detection will be performed just before
  each BLK_IO read/write

  @param IdeDev pointer pointing to IDE_BLK_IO_DEV data structure, used
                 to record all the information of the IDE device.

  @retval EFI_SUCCESS       Identify ATAPI device successfully.
  @retval EFI_DEVICE_ERROR  ATAPI Identify Device Command failed or device type
                            is not supported by this IDE driver.
  @retval EFI_OUT_OF_RESOURCES Allocate memory for sense data failed

  @note   Parameter "IdeDev" will be updated in this function.
**/
EFI_STATUS
ATAPIIdentify (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
{
  EFI_IDENTIFY_DATA *AtapiIdentifyPointer;
  UINT8             DeviceSelect;
  EFI_STATUS        Status;

  //
  // device select bit
  //
  DeviceSelect          = (UINT8) ((IdeDev->Device) << 4);

  AtapiIdentifyPointer  = AllocatePool (sizeof (EFI_IDENTIFY_DATA));
  if (AtapiIdentifyPointer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Send ATAPI Identify Command to get IDENTIFY data.
  //
  Status = AtaPioDataIn (
            IdeDev,
            (VOID *) AtapiIdentifyPointer,
            sizeof (EFI_IDENTIFY_DATA),
            ATA_CMD_IDENTIFY_DEVICE,
            DeviceSelect,
            0,
            0,
            0,
            0
            );

  if (EFI_ERROR (Status)) {
    gBS->FreePool (AtapiIdentifyPointer);
    return EFI_DEVICE_ERROR;
  }

  IdeDev->IdData = AtapiIdentifyPointer;
  PrintAtaModuleName (IdeDev);

  //
  // Send ATAPI Inquiry Packet Command to get INQUIRY data.
  //
  Status = AtapiInquiry (IdeDev);
  if (EFI_ERROR (Status)) {
    gBS->FreePool (IdeDev->IdData);
    //
    // Make sure the pIdData will not be freed again.
    //
    IdeDev->IdData = NULL;
    return EFI_DEVICE_ERROR;
  }
  //
  // Get media removable info from INQUIRY data.
  //
  IdeDev->BlkIo.Media->RemovableMedia = (UINT8) ((IdeDev->InquiryData->RMB & 0x80) == 0x80);

  //
  // Identify device type via INQUIRY data.
  //
  switch (IdeDev->InquiryData->peripheral_type & 0x1f) {

  //
  // Magnetic Disk
  //
  case 0x00:

    //
    // device is LS120 or ZIP drive.
    //
    IdeDev->Type = IdeMagnetic;

    IdeDev->BlkIo.Media->MediaId      = 0;
    //
    // Give initial value
    //
    IdeDev->BlkIo.Media->MediaPresent = FALSE;

    IdeDev->BlkIo.Media->LastBlock  = 0;
    IdeDev->BlkIo.Media->BlockSize  = 0x200;
    break;

  //
  // CD-ROM
  //
  case 0x05:

    IdeDev->Type                      = IdeCdRom;
    IdeDev->BlkIo.Media->MediaId      = 0;
    //
    // Give initial value
    //
    IdeDev->BlkIo.Media->MediaPresent = FALSE;

    IdeDev->BlkIo.Media->LastBlock  = 0;
    IdeDev->BlkIo.Media->BlockSize  = 0x800;
    IdeDev->BlkIo.Media->ReadOnly   = TRUE;
    break;

  //
  // Tape
  //
  case 0x01:

  //
  // WORM
  //
  case 0x04:

  //
  // Optical
  //
  case 0x07:

  default:
    IdeDev->Type = IdeUnknown;
    gBS->FreePool (IdeDev->IdData);
    gBS->FreePool (IdeDev->InquiryData);
    //
    // Make sure the pIdData and pInquiryData will not be freed again.
    //
    IdeDev->IdData       = NULL;
    IdeDev->InquiryData  = NULL;
    return EFI_DEVICE_ERROR;
  }

  //
  // original sense data numbers
  //
  IdeDev->SenseDataNumber = 20;

  IdeDev->SenseData = AllocatePool (IdeDev->SenseDataNumber * sizeof (ATAPI_REQUEST_SENSE_DATA));
  if (IdeDev->SenseData == NULL) {
    gBS->FreePool (IdeDev->IdData);
    gBS->FreePool (IdeDev->InquiryData);
    //
    // Make sure the pIdData and pInquiryData will not be freed again.
    //
    IdeDev->IdData       = NULL;
    IdeDev->InquiryData  = NULL;
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}
/**
  Sends out ATAPI Request Sense Packet Command to the specified device. This command
  will return all the current Sense data in the device.  This function will pack
  all the Sense data in one single buffer.

  @param IdeDev       pointer pointing to IDE_BLK_IO_DEV data structure, used
                      to record all the information of the IDE device.
  @param SenseCounts  allocated in this function, and freed by the calling function.
                      This buffer is used to accommodate all the sense data returned
                      by the device.

  @retval EFI_SUCCESS      Request Sense command completes successfully.
  @retval EFI_DEVICE_ERROR Request Sense command failed.
**/
EFI_STATUS
AtapiRequestSense (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  OUT UINTN           *SenseCounts
  )
{
  EFI_STATUS            Status;
  ATAPI_REQUEST_SENSE_DATA    *Sense;
  UINT16                *Ptr;
  BOOLEAN               FetchSenseData;
  ATAPI_PACKET_COMMAND  Packet;

  *SenseCounts = 0;

  ZeroMem (IdeDev->SenseData, sizeof (ATAPI_REQUEST_SENSE_DATA) * (IdeDev->SenseDataNumber));
  //
  // fill command packet for Request Sense Packet Command
  //
  ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
  Packet.RequestSence.opcode            = ATA_CMD_REQUEST_SENSE;
  Packet.RequestSence.allocation_length = (UINT8) sizeof (ATAPI_REQUEST_SENSE_DATA);

  //
  // initialize pointer
  //
  Ptr = (UINT16 *) IdeDev->SenseData;
  //
  //  request sense data from device continuously until no sense data
  //  exists in the device.
  //
  for (FetchSenseData = TRUE; FetchSenseData;) {

    Sense = (ATAPI_REQUEST_SENSE_DATA *) Ptr;

    //
    // send out Request Sense Packet Command and get one Sense data form device
    //
    Status = AtapiPacketCommandIn (
              IdeDev,
              &Packet,
              Ptr,
              sizeof (ATAPI_REQUEST_SENSE_DATA),
              ATAPITIMEOUT
              );
    //
    // failed to get Sense data
    //
    if (EFI_ERROR (Status)) {
      if (*SenseCounts == 0) {
        return EFI_DEVICE_ERROR;
      } else {
        return EFI_SUCCESS;
      }
    }

    (*SenseCounts)++;
    //
    // We limit MAX sense data count to 20 in order to avoid dead loop. Some
    // incompatible ATAPI devices don't retrive NO_SENSE when there is no media.
    // In this case, dead loop occurs if we don't have a gatekeeper. 20 is
    // supposed to be large enough for any ATAPI device.
    //
    if ((Sense->sense_key != ATA_SK_NO_SENSE) && ((*SenseCounts) < 20)) {
      //
      // Ptr is word-based pointer
      //
      Ptr += (sizeof (ATAPI_REQUEST_SENSE_DATA) + 1) >> 1;

    } else {
      //
      // when no sense key, skip out the loop
      //
      FetchSenseData = FALSE;
    }
  }

  return EFI_SUCCESS;
}
/**
  This function is used to parse sense data. Only the first sense data is honoured

  @param IdeDev     Indicates the calling context.
  @param SenseCount Count of sense data.
  @param Result    The parsed result.

  @retval EFI_SUCCESS           Successfully parsed.
  @retval EFI_INVALID_PARAMETER Count of sense data is zero.

**/
EFI_STATUS
ParseSenseData (
  IN IDE_BLK_IO_DEV     *IdeDev,
  IN UINTN              SenseCount,
  OUT SENSE_RESULT      *Result
  )
{
  ATAPI_REQUEST_SENSE_DATA      *SenseData;

  if (SenseCount == 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Only use the first sense data
  //
  SenseData = IdeDev->SenseData;
  *Result   = SenseOtherSense;

  switch (SenseData->sense_key) {
  case ATA_SK_NO_SENSE:
    *Result = SenseNoSenseKey;
    break;
  case ATA_SK_NOT_READY:
    switch (SenseData->addnl_sense_code) {
    case ATA_ASC_NO_MEDIA:
      *Result = SenseNoMedia;
      break;
    case ATA_ASC_MEDIA_UPSIDE_DOWN:
      *Result = SenseMediaError;
      break;
    case ATA_ASC_NOT_READY:
      if (SenseData->addnl_sense_code_qualifier == ATA_ASCQ_IN_PROGRESS) {
        *Result = SenseDeviceNotReadyNeedRetry;
      } else {
        *Result = SenseDeviceNotReadyNoRetry;
      }
      break;
    }
    break;
  case ATA_SK_UNIT_ATTENTION:
    if (SenseData->addnl_sense_code == ATA_ASC_MEDIA_CHANGE) {
      *Result = SenseMediaChange;
    }
    break;
  case ATA_SK_MEDIUM_ERROR:
    switch (SenseData->addnl_sense_code) {
    case ATA_ASC_MEDIA_ERR1:
    case ATA_ASC_MEDIA_ERR2:
    case ATA_ASC_MEDIA_ERR3:
    case ATA_ASC_MEDIA_ERR4:
      *Result = SenseMediaError;
      break;
    }
    break;
  default:
    break;
  }

  return EFI_SUCCESS;
}

/**
  Sends out ATAPI Test Unit Ready Packet Command to the specified device
  to find out whether device is accessible.

  @param IdeDev    Pointer pointing to IDE_BLK_IO_DEV data structure, used
                   to record all the information of the IDE device.
  @param SResult   Sense result for this packet command.

  @retval EFI_SUCCESS      Device is accessible.
  @retval EFI_DEVICE_ERROR Device is not accessible.

**/
EFI_STATUS
AtapiTestUnitReady (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  OUT SENSE_RESULT    *SResult
  )
{
  ATAPI_PACKET_COMMAND  Packet;
  EFI_STATUS            Status;
  UINTN         SenseCount;

  //
  // fill command packet
  //
  ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
  Packet.TestUnitReady.opcode = ATA_CMD_TEST_UNIT_READY;

  //
  // send command packet
  //
  Status = AtapiPacketCommandIn (IdeDev, &Packet, NULL, 0, ATAPITIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AtapiRequestSense (IdeDev, &SenseCount);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ParseSenseData (IdeDev, SenseCount, SResult);
  return EFI_SUCCESS;
}


/**
  Sends out ATAPI Read Capacity Packet Command to the specified device.
  This command will return the information regarding the capacity of the
  media in the device.

  Current device status will impact device's response to the Read Capacity
  Command. For example, if the device once reset, the Read Capacity
  Command will fail. The Sense data record the current device status, so
  if the Read Capacity Command failed, the Sense data must be requested
  and be analyzed to determine if the Read Capacity Command should retry.

  @param IdeDev    Pointer pointing to IDE_BLK_IO_DEV data structure, used
                   to record all the information of the IDE device.
  @param SResult   Sense result for this packet command

  @retval EFI_SUCCESS      Read Capacity Command finally completes successfully.
  @retval EFI_DEVICE_ERROR Read Capacity Command failed because of device error.
  @retval EFI_NOT_READY    Operation succeeds but returned capacity is 0

  @note Parameter "IdeDev" will be updated in this function.


**/
EFI_STATUS
AtapiReadCapacity (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  OUT SENSE_RESULT    *SResult
  )
{
  //
  // status returned by Read Capacity Packet Command
  //
  EFI_STATUS                Status;
  EFI_STATUS                SenseStatus;
  ATAPI_PACKET_COMMAND      Packet;
  UINTN           SenseCount;

  //
  // used for capacity data returned from ATAPI device
  //
  ATAPI_READ_CAPACITY_DATA        Data;
  ATAPI_READ_FORMAT_CAPACITY_DATA FormatData;

  ZeroMem (&Data, sizeof (Data));
  ZeroMem (&FormatData, sizeof (FormatData));

  if (IdeDev->Type == IdeCdRom) {

    ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
    Packet.Inquiry.opcode = ATA_CMD_READ_CAPACITY;
    Status = AtapiPacketCommandIn (
               IdeDev,
               &Packet,
               (UINT16 *) &Data,
               sizeof (ATAPI_READ_CAPACITY_DATA),
               ATAPITIMEOUT
               );

  } else {
    //
    // Type == IdeMagnetic
    //
    ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
    Packet.ReadFormatCapacity.opcode                = ATA_CMD_READ_FORMAT_CAPACITY;
    Packet.ReadFormatCapacity.allocation_length_lo  = 12;
    Status = AtapiPacketCommandIn (
               IdeDev,
               &Packet,
               (UINT16 *) &FormatData,
               sizeof (ATAPI_READ_FORMAT_CAPACITY_DATA),
               ATAPITIMEOUT
               );
  }

  if (Status == EFI_TIMEOUT) {
    return Status;
  }

  SenseStatus = AtapiRequestSense (IdeDev, &SenseCount);

  if (!EFI_ERROR (SenseStatus)) {
  ParseSenseData (IdeDev, SenseCount, SResult);

  if (!EFI_ERROR (Status) && *SResult == SenseNoSenseKey) {
      if (IdeDev->Type == IdeCdRom) {

        IdeDev->BlkIo.Media->LastBlock = ((UINT32) Data.LastLba3 << 24) |
          (Data.LastLba2 << 16) |
          (Data.LastLba1 << 8) |
          Data.LastLba0;

        IdeDev->BlkIo.Media->MediaPresent = TRUE;

        IdeDev->BlkIo.Media->ReadOnly = TRUE;

        //
        // Because the user data portion in the sector of the Data CD supported
        // is always 0x800
        //
        IdeDev->BlkIo.Media->BlockSize = 0x800;
      }

      if (IdeDev->Type == IdeMagnetic) {

        if (FormatData.DesCode == 3) {
          IdeDev->BlkIo.Media->MediaPresent = FALSE;
          IdeDev->BlkIo.Media->LastBlock    = 0;
        } else {

          IdeDev->BlkIo.Media->LastBlock = ((UINT32) FormatData.LastLba3 << 24) |
            (FormatData.LastLba2 << 16) |
            (FormatData.LastLba1 << 8)  |
            FormatData.LastLba0;
          if (IdeDev->BlkIo.Media->LastBlock != 0) {
            IdeDev->BlkIo.Media->LastBlock--;

            IdeDev->BlkIo.Media->BlockSize = (FormatData.BlockSize2 << 16) |
              (FormatData.BlockSize1 << 8) |
              FormatData.BlockSize0;

            IdeDev->BlkIo.Media->MediaPresent = TRUE;
          } else {
            IdeDev->BlkIo.Media->MediaPresent = FALSE;
            //
            // Return EFI_NOT_READY operation succeeds but returned capacity is 0
            //
            return EFI_NOT_READY;
          }

          IdeDev->BlkIo.Media->BlockSize = 0x200;

        }
      }
    }

    return EFI_SUCCESS;

  } else {
    return EFI_DEVICE_ERROR;
  }
}
/**
  This function is used to test the current media write-protected or not residing
  in the LS-120 drive or ZIP drive.
  @param IdeDev          pointer pointing to IDE_BLK_IO_DEV data structure, used
                         to record all the information of the IDE device.
  @param WriteProtected  if True, current media is write protected.
                         if FALSE, current media is writable

  @retval EFI_SUCCESS         The media write-protected status is achieved successfully
  @retval EFI_DEVICE_ERROR    Get Media Status Command is failed.
**/
EFI_STATUS
IsLS120orZipWriteProtected (
  IN  IDE_BLK_IO_DEV    *IdeDev,
  OUT BOOLEAN           *WriteProtected
  )
{
  EFI_STATUS  Status;

  *WriteProtected = FALSE;

  Status          = LS120EnableMediaStatus (IdeDev, TRUE);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // the Get Media Status Command is only valid
  // if a Set Features/Enable Media Status Command has been priviously issued.
  //
  if (LS120GetMediaStatus (IdeDev) == EFI_WRITE_PROTECTED) {

    *WriteProtected = TRUE;
  } else {

    *WriteProtected = FALSE;
  }

  //
  // After Get Media Status Command completes,
  // Set Features/Disable Media Command should be sent.
  //
  Status = LS120EnableMediaStatus (IdeDev, FALSE);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Used before read/write blocks from/to ATAPI device media. Since ATAPI device
  media is removable, it is necessary to detect whether media is present and
  get current present media's information, and if media has been changed, Block
  I/O Protocol need to be reinstalled.

  @param IdeDev       pointer pointing to IDE_BLK_IO_DEV data structure, used
                      to record all the information of the IDE device.
  @param MediaChange  return value that indicates if the media of the device has been
                      changed.

  @retval EFI_SUCCESS       media found successfully.
  @retval EFI_DEVICE_ERROR  any error encounters during media detection.
  @retval EFI_NO_MEDIA      media not found.

  @note
  parameter IdeDev may be updated in this function.

**/
EFI_STATUS
AtapiDetectMedia (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  OUT BOOLEAN         *MediaChange
  )
{
  EFI_STATUS                    Status;
  EFI_STATUS                    CleanStateStatus;
  EFI_BLOCK_IO_MEDIA            OldMediaInfo;
  UINTN                         RetryTimes;
  UINTN                         RetryNotReady;
  SENSE_RESULT                  SResult;
  BOOLEAN                       WriteProtected;

  CopyMem (&OldMediaInfo, IdeDev->BlkIo.Media, sizeof (EFI_BLOCK_IO_MEDIA));
  *MediaChange  = FALSE;
  //
  // Retry for SenseDeviceNotReadyNeedRetry.
  // Each retry takes 1s and we limit the upper boundary to
  // 120 times about 2 min.
  //
  RetryNotReady = 120;

  //
  // Do Test Unit Ready
  //
 DoTUR:
  //
  // Retry 5 times
  //
  RetryTimes = 5;
  while (RetryTimes != 0) {

    Status = AtapiTestUnitReady (IdeDev, &SResult);

    if (EFI_ERROR (Status)) {
      //
      // Test Unit Ready error without sense data.
      // For some devices, this means there's extra data
      // that has not been read, so we read these extra
      // data out before going on.
      //
      CleanStateStatus = AtapiReadPendingData (IdeDev);
      if (EFI_ERROR (CleanStateStatus)) {
        //
        // Busy wait failed, try again
        //
        RetryTimes--;
      }
      //
      // Try again without counting down RetryTimes
      //
      continue;
    } else {
      switch (SResult) {
      case SenseNoSenseKey:
        if (IdeDev->BlkIo.Media->MediaPresent) {
          goto Done;
        } else {
          //
          // Media present but the internal structure need refreshed.
          // Try Read Capacity
          //
          goto DoRC;
        }
        break;

      case SenseDeviceNotReadyNeedRetry:
        if (--RetryNotReady == 0) {
          return EFI_DEVICE_ERROR;
        }
        gBS->Stall (1000 * STALL_1_MILLI_SECOND);
        continue;
        break;

      case SenseNoMedia:
        IdeDev->BlkIo.Media->MediaPresent = FALSE;
        IdeDev->BlkIo.Media->LastBlock    = 0;
        goto Done;
        break;

      case SenseDeviceNotReadyNoRetry:
      case SenseMediaError:
        return EFI_DEVICE_ERROR;

      case SenseMediaChange:
        IdeDev->BlkIo.Media->MediaId++;
        goto DoRC;
        break;

      default:
        RetryTimes--;
        break;
      }
    }
  }

  return EFI_DEVICE_ERROR;

  //
  // Do Read Capacity
  //
 DoRC:
    RetryTimes = 5;

    while (RetryTimes != 0) {

      Status = AtapiReadCapacity (IdeDev, &SResult);

      if (EFI_ERROR (Status)) {
        RetryTimes--;
        continue;
      } else {
        switch (SResult) {
        case SenseNoSenseKey:
          goto Done;
          break;

        case SenseDeviceNotReadyNeedRetry:
          //
          // We use Test Unit Ready to retry which
          // is faster.
          //
          goto DoTUR;
          break;

        case SenseNoMedia:
          IdeDev->BlkIo.Media->MediaPresent = FALSE;
          IdeDev->BlkIo.Media->LastBlock    = 0;
          goto Done;
          break;

        case SenseDeviceNotReadyNoRetry:
        case SenseMediaError:
          return EFI_DEVICE_ERROR;

        case SenseMediaChange:
          IdeDev->BlkIo.Media->MediaId++;
          continue;
          break;

        default:
          RetryTimes--;
          break;
        }
      }
    }

  return EFI_DEVICE_ERROR;

 Done:
  //
  // the following code is to check the write-protected for LS120 media
  //
  if ((IdeDev->BlkIo.Media->MediaPresent) && (IdeDev->Type == IdeMagnetic)) {

    Status = IsLS120orZipWriteProtected (IdeDev, &WriteProtected);
    if (!EFI_ERROR (Status)) {

      if (WriteProtected) {

        IdeDev->BlkIo.Media->ReadOnly = TRUE;
      } else {

        IdeDev->BlkIo.Media->ReadOnly = FALSE;
      }

    }
  }

  if (IdeDev->BlkIo.Media->MediaId != OldMediaInfo.MediaId) {
    //
    // Media change information got from the device
    //
    *MediaChange = TRUE;
  }

  if (IdeDev->BlkIo.Media->ReadOnly != OldMediaInfo.ReadOnly) {
    *MediaChange = TRUE;
    IdeDev->BlkIo.Media->MediaId += 1;
  }

  if (IdeDev->BlkIo.Media->BlockSize != OldMediaInfo.BlockSize) {
    *MediaChange = TRUE;
    IdeDev->BlkIo.Media->MediaId += 1;
  }

  if (IdeDev->BlkIo.Media->LastBlock != OldMediaInfo.LastBlock) {
    *MediaChange = TRUE;
    IdeDev->BlkIo.Media->MediaId += 1;
  }

  if (IdeDev->BlkIo.Media->MediaPresent != OldMediaInfo.MediaPresent) {
    if (IdeDev->BlkIo.Media->MediaPresent) {
      //
      // when change from no media to media present, reset the MediaId to 1.
      //
      IdeDev->BlkIo.Media->MediaId = 1;
    } else {
      //
      // when no media, reset the MediaId to zero.
      //
      IdeDev->BlkIo.Media->MediaId = 0;
    }

    *MediaChange = TRUE;
  }

  //
  // if any change on current existing media,
  // the Block I/O protocol need to be reinstalled.
  //
  if (*MediaChange) {
    gBS->ReinstallProtocolInterface (
          IdeDev->Handle,
          &gEfiBlockIoProtocolGuid,
          &IdeDev->BlkIo,
          &IdeDev->BlkIo
          );
  }

  if (IdeDev->BlkIo.Media->MediaPresent) {
    return EFI_SUCCESS;
  } else {
    return EFI_NO_MEDIA;
  }
}

/**
  This function is called by the AtapiBlkIoReadBlocks() to perform
  read from media in block unit.

  The main command used to access media here is READ(10) Command.
  READ(10) Command requests that the ATAPI device media transfer
  specified data to the host. Data is transferred in block(sector)
  unit. The maximum number of blocks that can be transferred once is
  65536. This is the main difference between READ(10) and READ(12)
  Command. The maximum number of blocks in READ(12) is 2 power 32.

  @param IdeDev           pointer pointing to IDE_BLK_IO_DEV data structure, used
                          to record all the information of the IDE device.
  @param Buffer           A pointer to the destination buffer for the data.
  @param Lba              The starting logical block address to read from on the
                          device media.
  @param NumberOfBlocks   The number of transfer data blocks.

  @return status is fully dependent on the return status of AtapiPacketCommandIn() function.

**/
EFI_STATUS
AtapiReadSectors (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *Buffer,
  IN  EFI_LBA         Lba,
  IN  UINTN           NumberOfBlocks
  )
{

  ATAPI_PACKET_COMMAND  Packet;
  ATAPI_READ10_CMD            *Read10Packet;
  EFI_STATUS            Status;
  UINTN                 BlocksRemaining;
  UINT32                Lba32;
  UINT32                BlockSize;
  UINT32                ByteCount;
  UINT16                SectorCount;
  VOID                  *PtrBuffer;
  UINT16                MaxBlock;
  UINTN                 TimeOut;

  //
  // fill command packet for Read(10) command
  //
  ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
  Read10Packet  = &Packet.Read10;
  Lba32         = (UINT32) Lba;
  PtrBuffer     = Buffer;

  BlockSize     = IdeDev->BlkIo.Media->BlockSize;

  //
  // limit the data bytes that can be transferred by one Read(10) Command
  //
  MaxBlock        = 65535;

  BlocksRemaining = NumberOfBlocks;

  Status          = EFI_SUCCESS;
  while (BlocksRemaining > 0) {

    if (BlocksRemaining <= MaxBlock) {

      SectorCount = (UINT16) BlocksRemaining;
    } else {

      SectorCount = MaxBlock;
    }

    //
    // fill the Packet data structure
    //

    Read10Packet->opcode = ATA_CMD_READ_10;

    //
    // Lba0 ~ Lba3 specify the start logical block address of the data transfer.
    // Lba0 is MSB, Lba3 is LSB
    //
    Read10Packet->Lba3  = (UINT8) (Lba32 & 0xff);
    Read10Packet->Lba2  = (UINT8) (Lba32 >> 8);
    Read10Packet->Lba1  = (UINT8) (Lba32 >> 16);
    Read10Packet->Lba0  = (UINT8) (Lba32 >> 24);

    //
    // TranLen0 ~ TranLen1 specify the transfer length in block unit.
    // TranLen0 is MSB, TranLen is LSB
    //
    Read10Packet->TranLen1  = (UINT8) (SectorCount & 0xff);
    Read10Packet->TranLen0  = (UINT8) (SectorCount >> 8);

    ByteCount               = SectorCount * BlockSize;

    if (IdeDev->Type == IdeCdRom) {
      TimeOut = CDROMLONGTIMEOUT;
    } else {
      TimeOut = ATAPILONGTIMEOUT;
    }

    Status = AtapiPacketCommandIn (
              IdeDev,
              &Packet,
              (UINT16 *) PtrBuffer,
              ByteCount,
              TimeOut
              );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Lba32 += SectorCount;
    PtrBuffer = (UINT8 *) PtrBuffer + SectorCount * BlockSize;
    BlocksRemaining -= SectorCount;
  }

  return Status;
}

/**
  This function is called by the AtapiBlkIoWriteBlocks() to perform
  write onto media in block unit.
  The main command used to access media here is Write(10) Command.
  Write(10) Command requests that the ATAPI device media transfer
  specified data to the host. Data is transferred in block (sector)
  unit. The maximum number of blocks that can be transferred once is
  65536.

  @param IdeDev          pointer pointing to IDE_BLK_IO_DEV data structure, used
                         to record all the information of the IDE device.
  @param Buffer          A pointer to the source buffer for the data.
  @param Lba             The starting logical block address to write onto
                         the device media.
  @param NumberOfBlocks  The number of transfer data blocks.

  @return status is fully dependent on the return status of AtapiPacketCommandOut() function.

**/
EFI_STATUS
AtapiWriteSectors (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *Buffer,
  IN  EFI_LBA         Lba,
  IN  UINTN           NumberOfBlocks
  )
{

  ATAPI_PACKET_COMMAND  Packet;
  ATAPI_READ10_CMD            *Read10Packet;

  EFI_STATUS            Status;
  UINTN                 BlocksRemaining;
  UINT32                Lba32;
  UINT32                BlockSize;
  UINT32                ByteCount;
  UINT16                SectorCount;
  VOID                  *PtrBuffer;
  UINT16                MaxBlock;

  //
  // fill command packet for Write(10) command
  // Write(10) command packet has the same data structure as
  // Read(10) command packet,
  // so here use the Read10Packet data structure
  // for the Write(10) command packet.
  //
  ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
  Read10Packet  = &Packet.Read10;

  Lba32         = (UINT32) Lba;
  PtrBuffer     = Buffer;

  BlockSize     = IdeDev->BlkIo.Media->BlockSize;

  //
  // limit the data bytes that can be transferred by one Read(10) Command
  //
  MaxBlock        = (UINT16) (65536 / BlockSize);

  BlocksRemaining = NumberOfBlocks;

  Status          = EFI_SUCCESS;
  while (BlocksRemaining > 0) {

    if (BlocksRemaining >= MaxBlock) {
      SectorCount = MaxBlock;
    } else {
      SectorCount = (UINT16) BlocksRemaining;
    }

    //
    // Command code is WRITE_10.
    //
    Read10Packet->opcode = ATA_CMD_WRITE_10;

    //
    // Lba0 ~ Lba3 specify the start logical block address of the data transfer.
    // Lba0 is MSB, Lba3 is LSB
    //
    Read10Packet->Lba3  = (UINT8) (Lba32 & 0xff);
    Read10Packet->Lba2  = (UINT8) (Lba32 >> 8);
    Read10Packet->Lba1  = (UINT8) (Lba32 >> 16);
    Read10Packet->Lba0  = (UINT8) (Lba32 >> 24);

    //
    // TranLen0 ~ TranLen1 specify the transfer length in block unit.
    // TranLen0 is MSB, TranLen is LSB
    //
    Read10Packet->TranLen1  = (UINT8) (SectorCount & 0xff);
    Read10Packet->TranLen0  = (UINT8) (SectorCount >> 8);

    ByteCount               = SectorCount * BlockSize;

    Status = AtapiPacketCommandOut (
              IdeDev,
              &Packet,
              (UINT16 *) PtrBuffer,
              ByteCount,
              ATAPILONGTIMEOUT
              );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Lba32 += SectorCount;
    PtrBuffer = ((UINT8 *) PtrBuffer + SectorCount * BlockSize);
    BlocksRemaining -= SectorCount;
  }

  return Status;
}
/**
  This function is used to implement the Soft Reset on the specified
  ATAPI device. Different from the AtaSoftReset(), here reset is a ATA
  Soft Reset Command special for ATAPI device, and it only take effects
  on the specified ATAPI device, not on the whole IDE bus.
  Since the ATAPI soft reset is needed when device is in exceptional
  condition (such as BSY bit is always set ), I think the Soft Reset
  command should be sent without waiting for the BSY clear and DRDY
  set.
  This function is called by IdeBlkIoReset(),
  a interface function of Block I/O protocol.

  @param IdeDev    pointer pointing to IDE_BLK_IO_DEV data structure, used
                   to record all the information of the IDE device.

  @retval EFI_SUCCESS      Soft reset completes successfully.
  @retval EFI_DEVICE_ERROR Any step during the reset process is failed.

**/
EFI_STATUS
AtapiSoftReset (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
{
  UINT8       Command;
  UINT8       DeviceSelect;
  EFI_STATUS  Status;

  //
  // for ATAPI device, no need to wait DRDY ready after device selecting.
  // (bit7 and bit5 are both set to 1 for backward compatibility)
  //
  DeviceSelect = (UINT8) (((BIT7 | BIT5) | (IdeDev->Device << 4)));
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Head, DeviceSelect);

  Command = ATA_CMD_SOFT_RESET;
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Command, Command);

  //
  // BSY cleared is the only status return to the host by the device
  // when reset is completed.
  // slave device needs at most 31s to clear BSY
  //
  Status = WaitForBSYClear (IdeDev, 31000);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // stall 5 seconds to make the device status stable
  //
  gBS->Stall (5000000);

  return EFI_SUCCESS;
}

/**
  This function is the ATAPI implementation for ReadBlocks in the
  Block I/O Protocol interface.

  @param IdeBlkIoDevice Indicates the calling context.
  @param MediaId        The media id that the read request is for.
  @param Lba            The starting logical block address to read from on the device.
  @param BufferSize     The size of the Buffer in bytes. This must be a multiple
                        of the intrinsic block size of the device.
  @param Buffer         A pointer to the destination buffer for the data. The caller
                        is responsible for either having implicit or explicit
                        ownership of the memory that data is read into.

  @retval EFI_SUCCESS           Read Blocks successfully.
  @retval EFI_DEVICE_ERROR      Read Blocks failed.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE   The BufferSize parameter is not a multiple of the
                                intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid,
                                or the data buffer is not valid.
**/
EFI_STATUS
AtapiBlkIoReadBlocks (
  IN IDE_BLK_IO_DEV   *IdeBlkIoDevice,
  IN UINT32           MediaId,
  IN EFI_LBA          Lba,
  IN UINTN            BufferSize,
  OUT VOID            *Buffer
  )
{
  EFI_BLOCK_IO_MEDIA  *Media;
  UINTN               BlockSize;
  UINTN               NumberOfBlocks;
  EFI_STATUS          Status;

  BOOLEAN             MediaChange;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  //
  // ATAPI device media is removable, so it is a must
  // to detect media first before read operation
  //
  MediaChange = FALSE;
  Status      = AtapiDetectMedia (IdeBlkIoDevice, &MediaChange);
  if (EFI_ERROR (Status)) {

    if (IdeBlkIoDevice->Cache != NULL) {
      gBS->FreePool (IdeBlkIoDevice->Cache);
      IdeBlkIoDevice->Cache = NULL;
    }

    return Status;
  }
  //
  // Get the intrinsic block size
  //
  Media           = IdeBlkIoDevice->BlkIo.Media;
  BlockSize       = Media->BlockSize;

  NumberOfBlocks  = BufferSize / BlockSize;

  if (!(Media->MediaPresent)) {

    if (IdeBlkIoDevice->Cache != NULL) {
      gBS->FreePool (IdeBlkIoDevice->Cache);
      IdeBlkIoDevice->Cache = NULL;
    }
    return EFI_NO_MEDIA;

  }

  if ((MediaId != Media->MediaId) || MediaChange) {

    if (IdeBlkIoDevice->Cache != NULL) {
      gBS->FreePool (IdeBlkIoDevice->Cache);
      IdeBlkIoDevice->Cache = NULL;
    }
    return EFI_MEDIA_CHANGED;
  }

  if (BufferSize % BlockSize != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Lba > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Lba + NumberOfBlocks - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Media->IoAlign > 1) && (((UINTN) Buffer & (Media->IoAlign - 1)) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // if all the parameters are valid, then perform read sectors command
  // to transfer data from device to host.
  //
  Status = AtapiReadSectors (IdeBlkIoDevice, Buffer, Lba, NumberOfBlocks);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Read blocks succeeded
  //

  //
  // save the first block to the cache for performance
  //
  if (Lba == 0 && (IdeBlkIoDevice->Cache == NULL)) {
    IdeBlkIoDevice->Cache = AllocatePool (BlockSize);
    if (IdeBlkIoDevice->Cache!= NULL) {
      CopyMem ((UINT8 *) IdeBlkIoDevice->Cache, (UINT8 *) Buffer, BlockSize);
    }
  }

  return EFI_SUCCESS;

}
/**
  This function is the ATAPI implementation for WriteBlocks in the
  Block I/O Protocol interface.

  @param IdeBlkIoDevice  Indicates the calling context.
  @param MediaId         The media id that the write request is for.
  @param Lba             The starting logical block address to write onto the device.
  @param BufferSize      The size of the Buffer in bytes. This must be a multiple
                         of the intrinsic block size of the device.
  @param Buffer          A pointer to the source buffer for the data. The caller
                         is responsible for either having implicit or explicit ownership
                         of the memory that data is written from.

  @retval EFI_SUCCESS            Write Blocks successfully.
  @retval EFI_DEVICE_ERROR       Write Blocks failed.
  @retval EFI_NO_MEDIA           There is no media in the device.
  @retval EFI_MEDIA_CHANGE       The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE    The BufferSize parameter is not a multiple of the
                                 intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER  The write request contains LBAs that are not valid,
                                 or the data buffer is not valid.

  @retval EFI_WRITE_PROTECTED    The write protected is enabled or the media does not support write
**/
EFI_STATUS
AtapiBlkIoWriteBlocks (
  IN IDE_BLK_IO_DEV   *IdeBlkIoDevice,
  IN UINT32           MediaId,
  IN EFI_LBA          Lba,
  IN UINTN            BufferSize,
  OUT VOID            *Buffer
  )
{

  EFI_BLOCK_IO_MEDIA  *Media;
  UINTN               BlockSize;
  UINTN               NumberOfBlocks;
  EFI_STATUS          Status;
  BOOLEAN             MediaChange;

  if (Lba == 0 && IdeBlkIoDevice->Cache != NULL) {
    gBS->FreePool (IdeBlkIoDevice->Cache);
    IdeBlkIoDevice->Cache = NULL;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  //
  // ATAPI device media is removable,
  // so it is a must to detect media first before write operation
  //
  MediaChange = FALSE;
  Status      = AtapiDetectMedia (IdeBlkIoDevice, &MediaChange);
  if (EFI_ERROR (Status)) {

    if (Lba == 0 && IdeBlkIoDevice->Cache != NULL) {
      gBS->FreePool (IdeBlkIoDevice->Cache);
      IdeBlkIoDevice->Cache = NULL;
    }
    return Status;
  }

  //
  // Get the intrinsic block size
  //
  Media           = IdeBlkIoDevice->BlkIo.Media;
  BlockSize       = Media->BlockSize;
  NumberOfBlocks  = BufferSize / BlockSize;

  if (!(Media->MediaPresent)) {

    if (Lba == 0 && IdeBlkIoDevice->Cache != NULL) {
      gBS->FreePool (IdeBlkIoDevice->Cache);
      IdeBlkIoDevice->Cache = NULL;
    }
    return EFI_NO_MEDIA;
  }

  if ((MediaId != Media->MediaId) || MediaChange) {

    if (Lba == 0 && IdeBlkIoDevice->Cache != NULL) {
      gBS->FreePool (IdeBlkIoDevice->Cache);
      IdeBlkIoDevice->Cache = NULL;
    }
    return EFI_MEDIA_CHANGED;
  }

  if (Media->ReadOnly) {
    return EFI_WRITE_PROTECTED;
  }

  if (BufferSize % BlockSize != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Lba > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Lba + NumberOfBlocks - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Media->IoAlign > 1) && (((UINTN) Buffer & (Media->IoAlign - 1)) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // if all the parameters are valid,
  // then perform write sectors command to transfer data from host to device.
  //
  Status = AtapiWriteSectors (IdeBlkIoDevice, Buffer, Lba, NumberOfBlocks);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;

}



