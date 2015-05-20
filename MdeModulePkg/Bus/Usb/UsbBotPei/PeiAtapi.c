/** @file
Pei USB ATATPI command implementations.

Copyright (c) 1999 - 2015, Intel Corporation. All rights reserved.<BR>
  
This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UsbBotPeim.h"
#include "BotPeim.h"

#define MAXSENSEKEY 5

/**
  Sends out ATAPI Inquiry Packet Command to the specified device. This command will
  return INQUIRY data of the device.

  @param PeiServices    The pointer of EFI_PEI_SERVICES.
  @param PeiBotDevice   The pointer to PEI_BOT_DEVICE instance.

  @retval EFI_SUCCESS       Inquiry command completes successfully.
  @retval EFI_DEVICE_ERROR  Inquiry command failed.

**/
EFI_STATUS
PeiUsbInquiry (
  IN  EFI_PEI_SERVICES  **PeiServices,
  IN  PEI_BOT_DEVICE    *PeiBotDevice
  )
{
  ATAPI_PACKET_COMMAND  Packet;
  EFI_STATUS            Status;
  ATAPI_INQUIRY_DATA          Idata;

  //
  // fill command packet
  //
  ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
  ZeroMem (&Idata, sizeof (ATAPI_INQUIRY_DATA));

  Packet.Inquiry.opcode             = ATA_CMD_INQUIRY;
  Packet.Inquiry.page_code          = 0;
  Packet.Inquiry.allocation_length  = 36;

  //
  // Send scsi INQUIRY command packet.
  // According to SCSI Primary Commands-2 spec, host only needs to
  // retrieve the first 36 bytes for standard INQUIRY data.
  //
  Status = PeiAtapiCommand (
            PeiServices,
            PeiBotDevice,
            &Packet,
            (UINT8) sizeof (ATAPI_PACKET_COMMAND),
            &Idata,
            36,
            EfiUsbDataIn,
            2000
            );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  if ((Idata.peripheral_type & 0x1f) == 0x05) {
    PeiBotDevice->DeviceType      = USBCDROM;
    PeiBotDevice->Media.BlockSize = 0x800;
    PeiBotDevice->Media2.ReadOnly       = TRUE;
    PeiBotDevice->Media2.RemovableMedia = TRUE;
    PeiBotDevice->Media2.BlockSize      = 0x800;
  } else {
    PeiBotDevice->DeviceType      = USBFLOPPY;
    PeiBotDevice->Media.BlockSize = 0x200;
    PeiBotDevice->Media2.ReadOnly       = FALSE;
    PeiBotDevice->Media2.RemovableMedia = TRUE;
    PeiBotDevice->Media2.BlockSize      = 0x200;
  }

  return EFI_SUCCESS;
}

/**
  Sends out ATAPI Test Unit Ready Packet Command to the specified device
  to find out whether device is accessible.

  @param PeiServices    The pointer of EFI_PEI_SERVICES.
  @param PeiBotDevice   The pointer to PEI_BOT_DEVICE instance.

  @retval EFI_SUCCESS        TestUnit command executed successfully.
  @retval EFI_DEVICE_ERROR   Device cannot be executed TestUnit command successfully.

**/
EFI_STATUS
PeiUsbTestUnitReady (
  IN  EFI_PEI_SERVICES  **PeiServices,
  IN  PEI_BOT_DEVICE    *PeiBotDevice
  )
{
  ATAPI_PACKET_COMMAND  Packet;
  EFI_STATUS            Status;

  //
  // fill command packet
  //
  ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
  Packet.TestUnitReady.opcode = ATA_CMD_TEST_UNIT_READY;

  //
  // send command packet
  //
  Status = PeiAtapiCommand (
            PeiServices,
            PeiBotDevice,
            &Packet,
            (UINT8) sizeof (ATAPI_PACKET_COMMAND),
            NULL,
            0,
            EfiUsbNoData,
            2000
            );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Sends out ATAPI Request Sense Packet Command to the specified device.

  @param PeiServices    The pointer of EFI_PEI_SERVICES.
  @param PeiBotDevice   The pointer to PEI_BOT_DEVICE instance.
  @param SenseCounts    Length of sense buffer.
  @param SenseKeyBuffer Pointer to sense buffer.

  @retval EFI_SUCCESS           Command executed successfully.
  @retval EFI_DEVICE_ERROR      Some device errors happen.

**/
EFI_STATUS
PeiUsbRequestSense (
  IN  EFI_PEI_SERVICES  **PeiServices,
  IN  PEI_BOT_DEVICE    *PeiBotDevice,
  OUT UINTN             *SenseCounts,
  IN  UINT8             *SenseKeyBuffer
  )
{
  EFI_STATUS                  Status;
  ATAPI_PACKET_COMMAND        Packet;
  UINT8                       *Ptr;
  BOOLEAN                     SenseReq;
  ATAPI_REQUEST_SENSE_DATA    *Sense;

  *SenseCounts = 0;

  //
  // fill command packet for Request Sense Packet Command
  //
  ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
  Packet.RequestSence.opcode            = ATA_CMD_REQUEST_SENSE;
  Packet.RequestSence.allocation_length = (UINT8) sizeof (ATAPI_REQUEST_SENSE_DATA);

  Ptr = SenseKeyBuffer;

  SenseReq = TRUE;

  //
  //  request sense data from device continuously
  //  until no sense data exists in the device.
  //
  while (SenseReq) {
    Sense = (ATAPI_REQUEST_SENSE_DATA *) Ptr;

    //
    // send out Request Sense Packet Command and get one Sense
    // data form device.
    //
    Status = PeiAtapiCommand (
              PeiServices,
              PeiBotDevice,
              &Packet,
              (UINT8) sizeof (ATAPI_PACKET_COMMAND),
              (VOID *) Ptr,
              sizeof (ATAPI_REQUEST_SENSE_DATA),
              EfiUsbDataIn,
              2000
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

    if (Sense->sense_key != ATA_SK_NO_SENSE) {

      Ptr += sizeof (ATAPI_REQUEST_SENSE_DATA);
      //
      // Ptr is byte based pointer
      //
      (*SenseCounts)++;

      if (*SenseCounts == MAXSENSEKEY) {
        break;
      }

    } else {
      //
      // when no sense key, skip out the loop
      //
      SenseReq = FALSE;
    }
  }

  return EFI_SUCCESS;
}

/**
  Sends out ATAPI Read Capacity Packet Command to the specified device.
  This command will return the information regarding the capacity of the
  media in the device.

  @param PeiServices    The pointer of EFI_PEI_SERVICES.
  @param PeiBotDevice   The pointer to PEI_BOT_DEVICE instance.

  @retval EFI_SUCCESS           Command executed successfully.
  @retval EFI_DEVICE_ERROR      Some device errors happen.

**/
EFI_STATUS
PeiUsbReadCapacity (
  IN  EFI_PEI_SERVICES  **PeiServices,
  IN  PEI_BOT_DEVICE    *PeiBotDevice
  )
{
  EFI_STATUS                  Status;
  ATAPI_PACKET_COMMAND        Packet;
  ATAPI_READ_CAPACITY_DATA    Data;
  UINT32                      LastBlock;

  ZeroMem (&Data, sizeof (ATAPI_READ_CAPACITY_DATA));
  ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));

  Packet.Inquiry.opcode = ATA_CMD_READ_CAPACITY;

  //
  // send command packet
  //
  Status = PeiAtapiCommand (
            PeiServices,
            PeiBotDevice,
            &Packet,
            (UINT8) sizeof (ATAPI_PACKET_COMMAND),
            (VOID *) &Data,
            sizeof (ATAPI_READ_CAPACITY_DATA),
            EfiUsbDataIn,
            2000
            );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }
  LastBlock = (Data.LastLba3 << 24) | (Data.LastLba2 << 16) | (Data.LastLba1 << 8) | Data.LastLba0;

  if (LastBlock == 0xFFFFFFFF) {
    DEBUG ((EFI_D_INFO, "The usb device LBA count is larger than 0xFFFFFFFF!\n"));
  }

  PeiBotDevice->Media.LastBlock    = LastBlock;
  PeiBotDevice->Media.MediaPresent = TRUE;

  PeiBotDevice->Media2.LastBlock    = LastBlock;
  PeiBotDevice->Media2.MediaPresent = TRUE;

  return EFI_SUCCESS;
}

/**
  Sends out ATAPI Read Format Capacity Data Command to the specified device.
  This command will return the information regarding the capacity of the
  media in the device.

  @param PeiServices    The pointer of EFI_PEI_SERVICES.
  @param PeiBotDevice   The pointer to PEI_BOT_DEVICE instance.

  @retval EFI_SUCCESS           Command executed successfully.
  @retval EFI_DEVICE_ERROR      Some device errors happen.

**/
EFI_STATUS
PeiUsbReadFormattedCapacity (
  IN  EFI_PEI_SERVICES  **PeiServices,
  IN  PEI_BOT_DEVICE    *PeiBotDevice
  )
{
  EFI_STATUS                      Status;
  ATAPI_PACKET_COMMAND            Packet;
  ATAPI_READ_FORMAT_CAPACITY_DATA FormatData;
  UINT32                          LastBlock;

  ZeroMem (&FormatData, sizeof (ATAPI_READ_FORMAT_CAPACITY_DATA));
  ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));

  Packet.ReadFormatCapacity.opcode                = ATA_CMD_READ_FORMAT_CAPACITY;
  Packet.ReadFormatCapacity.allocation_length_lo  = 12;

  //
  // send command packet
  //
  Status = PeiAtapiCommand (
            PeiServices,
            PeiBotDevice,
            &Packet,
            (UINT8) sizeof (ATAPI_PACKET_COMMAND),
            (VOID *) &FormatData,
            sizeof (ATAPI_READ_FORMAT_CAPACITY_DATA),
            EfiUsbDataIn,
            2000
            );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  if (FormatData.DesCode == 3) {
    //
    // Media is not present
    //
    PeiBotDevice->Media.MediaPresent  = FALSE;
    PeiBotDevice->Media.LastBlock     = 0;
    PeiBotDevice->Media2.MediaPresent  = FALSE;
    PeiBotDevice->Media2.LastBlock     = 0;

  } else {
    LastBlock = (FormatData.LastLba3 << 24) | (FormatData.LastLba2 << 16) | (FormatData.LastLba1 << 8) | FormatData.LastLba0;
    if (LastBlock == 0xFFFFFFFF) {
      DEBUG ((EFI_D_INFO, "The usb device LBA count is larger than 0xFFFFFFFF!\n"));
    }

    PeiBotDevice->Media.LastBlock = LastBlock;

    PeiBotDevice->Media.LastBlock--;

    PeiBotDevice->Media.MediaPresent = TRUE;

    PeiBotDevice->Media2.MediaPresent = TRUE;
    PeiBotDevice->Media2.LastBlock    = PeiBotDevice->Media.LastBlock;
  }

  return EFI_SUCCESS;
}

/**
  Execute Read(10) ATAPI command on a specific SCSI target.

  Executes the ATAPI Read(10) command on the ATAPI target specified by PeiBotDevice.

  @param PeiServices       The pointer of EFI_PEI_SERVICES.
  @param PeiBotDevice      The pointer to PEI_BOT_DEVICE instance.
  @param Buffer            The pointer to data buffer.
  @param Lba               The start logic block address of reading.
  @param NumberOfBlocks    The block number of reading.

  @retval EFI_SUCCESS           Command executed successfully.
  @retval EFI_DEVICE_ERROR      Some device errors happen.

**/
EFI_STATUS
PeiUsbRead10 (
  IN  EFI_PEI_SERVICES  **PeiServices,
  IN  PEI_BOT_DEVICE    *PeiBotDevice,
  IN  VOID              *Buffer,
  IN  EFI_PEI_LBA       Lba,
  IN  UINTN             NumberOfBlocks
  )
{
  ATAPI_PACKET_COMMAND  Packet;
  ATAPI_READ10_CMD      *Read10Packet;
  UINT16                MaxBlock;
  UINT16                BlocksRemaining;
  UINT16                SectorCount;
  UINT32                Lba32;
  UINT32                BlockSize;
  UINT32                ByteCount;
  VOID                  *PtrBuffer;
  EFI_STATUS            Status;
  UINT16                TimeOut;

  //
  // prepare command packet for the Inquiry Packet Command.
  //
  ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
  Read10Packet    = &Packet.Read10;
  Lba32           = (UINT32) Lba;
  PtrBuffer       = Buffer;

  BlockSize       = (UINT32) PeiBotDevice->Media.BlockSize;

  MaxBlock        = (UINT16) (65535 / BlockSize);
  BlocksRemaining = (UINT16) NumberOfBlocks;

  Status          = EFI_SUCCESS;
  while (BlocksRemaining > 0) {

    if (BlocksRemaining <= MaxBlock) {

      SectorCount = BlocksRemaining;

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

    TimeOut                 = (UINT16) (SectorCount * 2000);

    //
    // send command packet
    //
    Status = PeiAtapiCommand (
              PeiServices,
              PeiBotDevice,
              &Packet,
              (UINT8) sizeof (ATAPI_PACKET_COMMAND),
              (VOID *) PtrBuffer,
              ByteCount,
              EfiUsbDataIn,
              TimeOut
              );

    if (Status != EFI_SUCCESS) {
      return Status;
    }

    Lba32 += SectorCount;
    PtrBuffer       = (UINT8 *) PtrBuffer + SectorCount * BlockSize;
    BlocksRemaining = (UINT16) (BlocksRemaining - SectorCount);
  }

  return Status;
}

/**  
  Check if there is media according to sense data.

  @param  SenseData   Pointer to sense data.
  @param  SenseCounts Count of sense data.

  @retval TRUE    No media
  @retval FALSE   Media exists

**/
BOOLEAN
IsNoMedia (
  IN  ATAPI_REQUEST_SENSE_DATA *SenseData,
  IN  UINTN                    SenseCounts
  )
{
  ATAPI_REQUEST_SENSE_DATA  *SensePtr;
  UINTN                     Index;
  BOOLEAN                   NoMedia;

  NoMedia   = FALSE;
  SensePtr  = SenseData;

  for (Index = 0; Index < SenseCounts; Index++) {

    switch (SensePtr->sense_key) {

    case ATA_SK_NOT_READY:
      switch (SensePtr->addnl_sense_code) {
      //
      // if no media, fill IdeDev parameter with specific info.
      //
      case ATA_ASC_NO_MEDIA:
        NoMedia = TRUE;
        break;

      default:
        break;
      }
      break;

    default:
      break;
    }

    SensePtr++;
  }

  return NoMedia;
}

/**  
  Check if there is media error according to sense data.

  @param  SenseData   Pointer to sense data.
  @param  SenseCounts Count of sense data.

  @retval TRUE    Media error
  @retval FALSE   No media error

**/
BOOLEAN
IsMediaError (
  IN  ATAPI_REQUEST_SENSE_DATA    *SenseData,
  IN  UINTN                       SenseCounts
  )
{
  ATAPI_REQUEST_SENSE_DATA  *SensePtr;
  UINTN                     Index;
  BOOLEAN                   Error;

  SensePtr  = SenseData;
  Error     = FALSE;

  for (Index = 0; Index < SenseCounts; Index++) {

    switch (SensePtr->sense_key) {
    //
    // Medium error case
    //
    case ATA_SK_MEDIUM_ERROR:
      switch (SensePtr->addnl_sense_code) {
      case ATA_ASC_MEDIA_ERR1:
        //
        // fall through
        //
      case ATA_ASC_MEDIA_ERR2:
        //
        // fall through
        //
      case ATA_ASC_MEDIA_ERR3:
        //
        // fall through
        //
      case ATA_ASC_MEDIA_ERR4:
        Error = TRUE;
        break;

      default:
        break;
      }

      break;

    //
    // Medium upside-down case
    //
    case ATA_SK_NOT_READY:
      switch (SensePtr->addnl_sense_code) {
      case ATA_ASC_MEDIA_UPSIDE_DOWN:
        Error = TRUE;
        break;

      default:
        break;
      }
      break;

    default:
      break;
    }

    SensePtr++;
  }

  return Error;
}

/**  
  Check if media is changed according to sense data.

  @param  SenseData   Pointer to sense data.
  @param  SenseCounts Count of sense data.

  @retval TRUE    There is media change event.
  @retval FALSE   media is NOT changed.

**/
BOOLEAN
IsMediaChange (
  IN  ATAPI_REQUEST_SENSE_DATA *SenseData,
  IN  UINTN                    SenseCounts
  )
{
  ATAPI_REQUEST_SENSE_DATA  *SensePtr;
  UINTN                     Index;
  BOOLEAN                   MediaChange;

  MediaChange = FALSE;

  SensePtr    = SenseData;

  for (Index = 0; Index < SenseCounts; Index++) {
    //
    // catch media change sense key and addition sense data
    //
    switch (SensePtr->sense_key) {
    case ATA_SK_UNIT_ATTENTION:
      switch (SensePtr->addnl_sense_code) {
      case ATA_ASC_MEDIA_CHANGE:
        MediaChange = TRUE;
        break;

      default:
        break;
      }
      break;

    default:
      break;
    }

    SensePtr++;
  }

  return MediaChange;
}
