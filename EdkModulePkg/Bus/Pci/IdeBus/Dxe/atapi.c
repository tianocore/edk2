/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  atapi.c
    
Abstract: 
    

Revision History
--*/

#include "idebus.h"

STATIC
EFI_STATUS
LS120GetMediaStatus (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++
  Name:
        LS120GetMediaStatus

  Purpose: 
        This function is used to get the current status of the media residing
        in the LS-120 drive or ZIP drive. The media status is returned in the 
        Error Status.

  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
          pointer pointing to IDE_BLK_IO_DEV data structure, used
          to record all the information of the IDE device.

  Returns:  
        EFI_SUCCESS
          The media status is achieved successfully and the media
          can be read/written.             
    
        EFI_DEVICE_ERROR
          Get Media Status Command is failed.

        EFI_NO_MEDIA
          There is no media in the drive.

        EFI_WRITE_PROTECTED
          The media is writing protected. 

  Notes:                
        This function must be called after the LS120EnableMediaStatus() 
        with second parameter set to TRUE 
        (means enable media status notification) is called.
--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
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

  if (StatusValue & bit1) {
    return EFI_NO_MEDIA;
  }

  if (StatusValue & bit6) {
    return EFI_WRITE_PROTECTED;
  } else {
    return EFI_SUCCESS;
  }
}

STATIC
EFI_STATUS
LS120EnableMediaStatus (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  BOOLEAN         Enable
  )
/*++
  Name:
        LS120EnableMediaStatus

  Purpose: 
        This function is used to send Enable Media Status Notification Command
        or Disable Media Status Notification Command.

  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
          pointer pointing to IDE_BLK_IO_DEV data structure, used
          to record all the information of the IDE device.

        BOOLEAN     IN    Enable
          a flag that indicates whether enable or disable media
          status notification.

  Returns:  
        EFI_SUCCESS
          If command completes successfully.

        EFI_DEVICE_ERROR
          If command failed.


  Notes:                
--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
// TODO:    Enable - add argument and description to function comment
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

EFI_STATUS
ATAPIIdentify (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++
  Name:
        ATAPIIdentify


  Purpose: 
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

  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
          pointer pointing to IDE_BLK_IO_DEV data structure, used
          to record all the information of the IDE device.

  Returns:  
        EFI_SUCCESS
          Identify ATAPI device successfully.

        EFI_DEVICE_ERROR
          ATAPI Identify Device Command failed or device type 
          is not supported by this IDE driver.

  Notes:
        Parameter "IdeDev" will be updated in this function.
--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
// TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
{
  EFI_IDENTIFY_DATA *AtapiIdentifyPointer;
  UINT8             DeviceSelect;
  EFI_STATUS        Status;

  //
  // device select bit
  //
  DeviceSelect          = 0;
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
            ATAPI_IDENTIFY_DEVICE_CMD,
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

  IdeDev->pIdData = AtapiIdentifyPointer;
  PrintAtaModuleName (IdeDev);

  //
  // Send ATAPI Inquiry Packet Command to get INQUIRY data.
  //
  Status = AtapiInquiry (IdeDev);
  if (EFI_ERROR (Status)) {
    gBS->FreePool (IdeDev->pIdData);
    //
    // Make sure the pIdData will not be freed again.
    //
    IdeDev->pIdData = NULL;
    return EFI_DEVICE_ERROR;
  }
  //
  // Get media removable info from INQUIRY data.
  //
  IdeDev->BlkIo.Media->RemovableMedia = (UINT8) ((IdeDev->pInquiryData->RMB & 0x80) == 0x80);

  //
  // Identify device type via INQUIRY data.
  //
  switch (IdeDev->pInquiryData->peripheral_type & 0x1f) {

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
    gBS->FreePool (IdeDev->pIdData);
    gBS->FreePool (IdeDev->pInquiryData);
    //
    // Make sure the pIdData and pInquiryData will not be freed again.
    //
    IdeDev->pIdData       = NULL;
    IdeDev->pInquiryData  = NULL;
    return EFI_DEVICE_ERROR;
  }

  //
  // original sense data numbers
  //
  IdeDev->SenseDataNumber = 20;

  IdeDev->SenseData = AllocatePool (IdeDev->SenseDataNumber * sizeof (REQUEST_SENSE_DATA));
  if (IdeDev->SenseData == NULL) {
    gBS->FreePool (IdeDev->pIdData);
    gBS->FreePool (IdeDev->pInquiryData);
    //
    // Make sure the pIdData and pInquiryData will not be freed again.
    //
    IdeDev->pIdData       = NULL;
    IdeDev->pInquiryData  = NULL;
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
AtapiInquiry (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++
  Name:
        AtapiInquiry

  Purpose: 
        Sends out ATAPI Inquiry Packet Command to the specified device.
        This command will return INQUIRY data of the device.

  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
          pointer pointing to IDE_BLK_IO_DEV data structure, used
          to record all the information of the IDE device.

  Returns:  
        EFI_SUCCESS
          Inquiry command completes successfully.

        EFI_DEVICE_ERROR
          Inquiry command failed.
  Notes:
        Parameter "IdeDev" will be updated in this function.
--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
{
  ATAPI_PACKET_COMMAND  Packet;
  EFI_STATUS            Status;
  INQUIRY_DATA          *InquiryData;

  //
  // prepare command packet for the ATAPI Inquiry Packet Command.
  //
  ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
  Packet.Inquiry.opcode             = INQUIRY;
  Packet.Inquiry.page_code          = 0;
  Packet.Inquiry.allocation_length  = sizeof (INQUIRY_DATA);

  InquiryData                       = AllocatePool (sizeof (INQUIRY_DATA));
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
            sizeof (INQUIRY_DATA),
            ATAPITIMEOUT
            );
  if (EFI_ERROR (Status)) {
    gBS->FreePool (InquiryData);
    return EFI_DEVICE_ERROR;
  }

  IdeDev->pInquiryData = InquiryData;

  return EFI_SUCCESS;
}

EFI_STATUS
AtapiPacketCommandIn (
  IN  IDE_BLK_IO_DEV        *IdeDev,
  IN  ATAPI_PACKET_COMMAND  *Packet,
  IN  UINT16                *Buffer,
  IN  UINT32                ByteCount,
  IN  UINTN                 TimeOut
  )
/*++
  Name:
        AtapiPacketCommandIn

  Purpose: 
        This function is used to send out ATAPI commands conforms to the 
        Packet Command with PIO Data In Protocol.

  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
          pointer pointing to IDE_BLK_IO_DEV data structure, used
          to record all the information of the IDE device.

        ATAPI_PACKET_COMMAND  IN  *Packet
          pointer pointing to ATAPI_PACKET_COMMAND data structure
          which contains the contents of the command.     
              
        UINT16      IN    *Buffer
          buffer contained data transferred from device to host.

        UINT32      IN    ByteCount
          data size in byte unit of the buffer.

        UINTN     IN    TimeOut
          this parameter is used to specify the timeout 
          value for the PioReadWriteData() function. 

  Returns:  
        EFI_SUCCESS
          send out the ATAPI packet command successfully 
          and device sends data successfully.

        EFI_DEVICE_ERROR
          the device failed to send data.                 

  Notes:
--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
// TODO:    Packet - add argument and description to function comment
// TODO:    Buffer - add argument and description to function comment
// TODO:    ByteCount - add argument and description to function comment
// TODO:    TimeOut - add argument and description to function comment
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
    (UINT8) ((IdeDev->Device << 4) | DEFAULT_CMD)  // DEFAULT_CMD: 0xa0 (1010,0000)
    );

  //
  // No OVL; No DMA
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Feature, 0x00);

  //
  // set the transfersize to MAX_ATAPI_BYTE_COUNT to let the device
  // determine how many data should be transferred.
  //
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->CylinderLsb,
    (UINT8) (MAX_ATAPI_BYTE_COUNT & 0x00ff)
    );
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->CylinderMsb,
    (UINT8) (MAX_ATAPI_BYTE_COUNT >> 8)
    );

  //
  //  DEFAULT_CTL:0x0a (0000,1010)
  //  Disable interrupt
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Alt.DeviceControl, DEFAULT_CTL);

  //
  // Send Packet command to inform device
  // that the following data bytes are command packet.
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Command, PACKET_CMD);

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

EFI_STATUS
AtapiPacketCommandOut (
  IN  IDE_BLK_IO_DEV        *IdeDev,
  IN  ATAPI_PACKET_COMMAND  *Packet,
  IN  UINT16                *Buffer,
  IN  UINT32                ByteCount,
  IN  UINTN                 TimeOut
  )
/*++
  Name:
        AtapiPacketCommandOut

  Purpose: 
        This function is used to send out ATAPI commands conforms to the 
        Packet Command with PIO Data Out Protocol.

  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
          pointer pointing to IDE_BLK_IO_DEV data structure, used
          to record all the information of the IDE device.

        ATAPI_PACKET_COMMAND  IN  *Packet
          pointer pointing to ATAPI_PACKET_COMMAND data structure
          which contains the contents of the command.

        VOID      IN    *Buffer
          buffer contained data transferred from host to device.

        UINT32      IN    ByteCount
          data size in byte unit of the buffer.

        UINTN     IN    TimeOut
          this parameter is used to specify the timeout 
          value for the PioReadWriteData() function. 

  Returns:  
        EFI_SUCCESS
          send out the ATAPI packet command successfully 
          and device received data successfully.

        EFI_DEVICE_ERROR
          the device failed to send data. 

  Notes:
  
--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
// TODO:    Packet - add argument and description to function comment
// TODO:    Buffer - add argument and description to function comment
// TODO:    ByteCount - add argument and description to function comment
// TODO:    TimeOut - add argument and description to function comment
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
    (UINT8) ((IdeDev->Device << 4) | DEFAULT_CMD)   // DEFAULT_CMD: 0xa0 (1010,0000)
    );

  //
  // No OVL; No DMA
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Feature, 0x00);

  //
  // set the transfersize to MAX_ATAPI_BYTE_COUNT to
  // let the device determine how many data should be transferred.
  //
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->CylinderLsb,
    (UINT8) (MAX_ATAPI_BYTE_COUNT & 0x00ff)
    );
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->CylinderMsb,
    (UINT8) (MAX_ATAPI_BYTE_COUNT >> 8)
    );

  //
  //  DEFAULT_CTL:0x0a (0000,1010)
  //  Disable interrupt
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Alt.DeviceControl, DEFAULT_CTL);

  //
  // Send Packet command to inform device
  // that the following data bytes are command packet.
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Command, PACKET_CMD);

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

EFI_STATUS
PioReadWriteData (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINT16          *Buffer,
  IN  UINT32          ByteCount,
  IN  BOOLEAN         Read,
  IN  UINTN           TimeOut
  )
/*++
  Name:
        PioReadWriteData

  Purpose: 
        This function is called by either AtapiPacketCommandIn() or 
        AtapiPacketCommandOut(). It is used to transfer data between
        host and device. The data direction is specified by the fourth
        parameter.


  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
          pointer pointing to IDE_BLK_IO_DEV data structure, used
          to record all the information of the IDE device.

        VOID      IN    *Buffer
          buffer contained data transferred between host and device.

        UINT32      IN    ByteCount
          data size in byte unit of the buffer.

        BOOLEAN     IN    Read
          flag used to determine the data transfer direction.
          Read equals 1, means data transferred from device to host;
          Read equals 0, means data transferred from host to device.

        UINTN     IN    TimeOut
          timeout value for wait DRQ ready before each data 
          stream's transfer.

  Returns:  
        EFI_SUCCESS
          data is transferred successfully.  

        EFI_DEVICE_ERROR
          the device failed to transfer data.                 

  Notes:

--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
// TODO:    Buffer - add argument and description to function comment
// TODO:    ByteCount - add argument and description to function comment
// TODO:    Read - add argument and description to function comment
// TODO:    TimeOut - add argument and description to function comment
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
  // containing status byte read from Status Register.
  //
  UINT8       StatusRegister;

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
    StatusRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Status);

    //
    // get current data transfer size from Cylinder Registers.
    //
    WordCount =
      (
        (IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->CylinderMsb) << 8) |
        IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->CylinderLsb)
      ) & 0xffff;
    WordCount /= 2;

    WordCount = EFI_MIN (WordCount, (RequiredWordCount - ActualWordCount));

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

EFI_STATUS
AtapiTestUnitReady (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++
  Name:
        AtapiTestUnitReady

  Purpose: 
        Sends out ATAPI Test Unit Ready Packet Command to the specified device
        to find out whether device is accessible.

  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
          pointer pointing to IDE_BLK_IO_DEV data structure, used
          to record all the information of the IDE device.

  Returns:  
        EFI_SUCCESS
          device is accessible.

        EFI_DEVICE_ERROR
          device is not accessible.

  Notes:

--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
{
  ATAPI_PACKET_COMMAND  Packet;
  EFI_STATUS            Status;

  //
  // fill command packet
  //
  ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
  Packet.TestUnitReady.opcode = TEST_UNIT_READY;

  //
  // send command packet
  //
  Status = AtapiPacketCommandIn (IdeDev, &Packet, NULL, 0, ATAPITIMEOUT);
  return Status;
}

EFI_STATUS
AtapiRequestSense (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  OUT UINTN           *SenseCounts
  )
/*++
  Name:
        AtapiRequestSense

  Purpose: 
        Sends out ATAPI Request Sense Packet Command to the specified device.
        This command will return all the current Sense data in the device. 
        This function will pack all the Sense data in one single buffer.

  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
          pointer pointing to IDE_BLK_IO_DEV data structure, used
          to record all the information of the IDE device.

        UINT16      OUT   **SenseBuffers
          allocated in this function, and freed by the calling function.
          This buffer is used to accommodate all the sense data returned 
          by the device.

        UINTN     OUT   *BufUnit
          record the unit size of the sense data block in the SenseBuffers,

        UINTN     OUT   *BufNumbers
          record the number of units in the SenseBuffers.

  Returns:  
        EFI_SUCCESS
          Request Sense command completes successfully.

        EFI_DEVICE_ERROR
          Request Sense command failed.

  Notes:

--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
// TODO:    SenseCounts - add argument and description to function comment
{
  EFI_STATUS            Status;
  REQUEST_SENSE_DATA    *Sense;
  UINT16                *Ptr;
  BOOLEAN               FetchSenseData;
  ATAPI_PACKET_COMMAND  Packet;

  *SenseCounts = 0;

  ZeroMem (IdeDev->SenseData, sizeof (REQUEST_SENSE_DATA) * (IdeDev->SenseDataNumber));
  //
  // fill command packet for Request Sense Packet Command
  //
  ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
  Packet.RequestSence.opcode            = REQUEST_SENSE;
  Packet.RequestSence.allocation_length = sizeof (REQUEST_SENSE_DATA);

  //
  // initialize pointer
  //
  Ptr = (UINT16 *) IdeDev->SenseData;
  //
  //  request sense data from device continuously until no sense data
  //  exists in the device.
  //
  for (FetchSenseData = TRUE; FetchSenseData;) {

    Sense = (REQUEST_SENSE_DATA *) Ptr;

    //
    // send out Request Sense Packet Command and get one Sense data form device
    //
    Status = AtapiPacketCommandIn (
              IdeDev,
              &Packet,
              Ptr,
              sizeof (REQUEST_SENSE_DATA),
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
    if ((Sense->sense_key != SK_NO_SENSE) && ((*SenseCounts) < 20)) {
      //
      // Ptr is word-based pointer
      //
      Ptr += sizeof (REQUEST_SENSE_DATA) / 2;

    } else {
      //
      // when no sense key, skip out the loop
      //
      FetchSenseData = FALSE;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
AtapiReadCapacity (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++
  Name:
        AtapiReadCapacity

  Purpose: 
        Sends out ATAPI Read Capacity Packet Command to the specified device.
        This command will return the information regarding the capacity of the
        media in the device.

        Current device status will impact device's response to the Read Capacity
        Command. For example, if the device once reset, the Read Capacity
        Command will fail. The Sense data record the current device status, so 
        if the Read Capacity Command failed, the Sense data must be requested
        and be analyzed to determine if the Read Capacity Command should retry.


  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
          pointer pointing to IDE_BLK_IO_DEV data structure, used
          to record all the information of the IDE device.

  Returns:  
        EFI_SUCCESS
          Read Capacity Command finally completes successfully.

        EFI_DEVICE_ERROR
          Read Capacity Command failed because of device error.

  Notes:
        parameter "IdeDev" will be updated in this function.
--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
// TODO:    EFI_NOT_READY - add return value to function comment
{
  //
  // status returned by Read Capacity Packet Command
  //
  EFI_STATUS                Status;
  ATAPI_PACKET_COMMAND      Packet;

  //
  // used for capacity data returned from ATAPI device
  //
  READ_CAPACITY_DATA        Data;
  READ_FORMAT_CAPACITY_DATA FormatData;

  ZeroMem (&Data, sizeof (Data));
  ZeroMem (&FormatData, sizeof (FormatData));

  if (IdeDev->Type == IdeCdRom) {

    ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
    Packet.Inquiry.opcode = READ_CAPACITY;
    Status = AtapiPacketCommandIn (
              IdeDev,
              &Packet,
              (UINT16 *) &Data,
              sizeof (READ_CAPACITY_DATA),
              ATAPITIMEOUT
              );

  } else {
    //
    // Type == IdeMagnetic
    //
    ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
    Packet.ReadFormatCapacity.opcode                = READ_FORMAT_CAPACITY;
    Packet.ReadFormatCapacity.allocation_length_lo  = 12;
    Status = AtapiPacketCommandIn (
              IdeDev,
              &Packet,
              (UINT16 *) &FormatData,
              sizeof (READ_FORMAT_CAPACITY_DATA),
              ATAPITIMEOUT
              );
  }

  if (!EFI_ERROR (Status)) {

    if (IdeDev->Type == IdeCdRom) {

      IdeDev->BlkIo.Media->LastBlock = (Data.LastLba3 << 24) |
        (Data.LastLba2 << 16) |
        (Data.LastLba1 << 8) |
        Data.LastLba0;

      if (IdeDev->BlkIo.Media->LastBlock != 0) {

        IdeDev->BlkIo.Media->BlockSize = (Data.BlockSize3 << 24) |
          (Data.BlockSize2 << 16) |
          (Data.BlockSize1 << 8) |
          Data.BlockSize0;

        IdeDev->BlkIo.Media->MediaPresent = TRUE;
      } else {
        IdeDev->BlkIo.Media->MediaPresent = FALSE;
        return EFI_DEVICE_ERROR;
      }

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

        IdeDev->BlkIo.Media->LastBlock =  (FormatData.LastLba3 << 24) |
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

    return EFI_SUCCESS;

  } else {

    return EFI_DEVICE_ERROR;
  }
}

EFI_STATUS
AtapiDetectMedia (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  OUT BOOLEAN         *MediaChange
  )
/*++
  Name:
        AtapiDetectMedia

  Purpose: 
        Used before read/write blocks from/to ATAPI device media. 
        Since ATAPI device media is removable, it is necessary to detect
        whether media is present and get current present media's
        information, and if media has been changed, Block I/O Protocol
        need to be reinstalled.

  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
          pointer pointing to IDE_BLK_IO_DEV data structure, used
          to record all the information of the IDE device.

        BOOLEAN         OUT   *MediaChange
          return value that indicates if the media of the device has been
          changed.

  Returns:  
        EFI_SUCCESS
          media found successfully.

        EFI_DEVICE_ERROR
          any error encounters during media detection.
          
        EFI_NO_MEDIA
          media not found.

  Notes:
        parameter IdeDev may be updated in this function.
--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
// TODO:    MediaChange - add argument and description to function comment
{
  EFI_STATUS          Status;
  EFI_STATUS          ReadCapacityStatus;
  EFI_BLOCK_IO_MEDIA  OldMediaInfo;
  UINTN               SenseCounts;
  UINTN               RetryIndex;
  UINTN               RetryTimes;
  UINTN               MaximumRetryTimes;
  UINTN               ReadyWaitFactor;
  BOOLEAN             NeedRetry;
  //
  // a flag used to determine whether need to perform Read Capacity command.
  //
  BOOLEAN             NeedReadCapacity;
  BOOLEAN             WriteProtected;

  //
  // init
  //
  CopyMem (&OldMediaInfo, IdeDev->BlkIo.Media, sizeof (OldMediaInfo));
  // OldMediaInfo        = *(IdeDev->BlkIo.Media);
  *MediaChange        = FALSE;
  ReadCapacityStatus  = EFI_DEVICE_ERROR;

  //
  // if there is no media, or media is not changed,
  // the request sense command will detect faster than read capacity command.
  // read capacity command can be bypassed, thus improve performance.
  //

  //
  // Test Unit Ready command is used to detect whether device is accessible,
  // the device will produce corresponding Sense data.
  //
  for (RetryIndex = 0; RetryIndex < 2; RetryIndex++) {

    Status = AtapiTestUnitReady (IdeDev);
    if (!EFI_ERROR (Status)) {
      //
      // skip the loop if test unit command succeeds.
      //
      break;
    }

    Status = AtapiSoftReset (IdeDev);

    if (EFI_ERROR (Status)) {
      AtaSoftReset (IdeDev);
    }
  }

  SenseCounts       = 0;
  NeedReadCapacity  = TRUE;

  //
  // at most retry 5 times
  //
  MaximumRetryTimes = 5;
  RetryTimes        = 1;

  for (RetryIndex = 0; 
       (RetryIndex < RetryTimes) && (RetryIndex < MaximumRetryTimes);
       RetryIndex++) {

    Status = AtapiRequestSense (IdeDev, &SenseCounts);

    if (!EFI_ERROR (Status)) {
      //
      // if first time there is no Sense Key, no need to read capacity any more
      //
      if (!HaveSenseKey (IdeDev->SenseData, SenseCounts) &&
          (IdeDev->BlkIo.Media->MediaPresent)) {

        if (RetryIndex == 0) {
          NeedReadCapacity = FALSE;
        }

      } else {
        //
        // No Media
        //
        if (IsNoMedia (IdeDev->SenseData, SenseCounts)) {
          NeedReadCapacity                  = FALSE;
          IdeDev->BlkIo.Media->MediaPresent = FALSE;
          IdeDev->BlkIo.Media->LastBlock    = 0;
        } else {
          //
          // Media Changed
          //
          if (IsMediaChange (IdeDev->SenseData, SenseCounts)) {
            NeedReadCapacity = TRUE;
            IdeDev->BlkIo.Media->MediaId++;
          }
          //
          // Media Error
          //
          if (IsMediaError (IdeDev->SenseData, SenseCounts)) {
            return EFI_DEVICE_ERROR;
          }
        }
      }
    } else {
      //
      // retry once more, if request sense command met errors.
      //
      RetryTimes++;
    }
  }

  if (NeedReadCapacity) {
    //
    // at most retry 5 times
    //
    MaximumRetryTimes = 5;
    //
    // initial retry twice
    //
    RetryTimes        = 2;
    ReadyWaitFactor = 2;

    for (RetryIndex = 0;
         (RetryIndex < RetryTimes) && (RetryIndex < MaximumRetryTimes);
         RetryIndex++) {

      ReadCapacityStatus  = AtapiReadCapacity (IdeDev);

      SenseCounts         = 0;

      if (!EFI_ERROR (ReadCapacityStatus)) {
        //
        // Read Capacity succeeded
        //
        break;

      } else {

        if (ReadCapacityStatus == EFI_NOT_READY) {
          //
          // If device not ready, wait here... waiting time increases by retry
          // times.
          //
          gBS->Stall (ReadyWaitFactor * 2000 * STALL_1_MILLI_SECOND);
          ReadyWaitFactor++;
          //
          // retry once more
          //
          RetryTimes++;
          continue;
        }
        
        //
        // Other errors returned, requery sense data
        //
        Status = AtapiRequestSense (IdeDev, &SenseCounts);

        //
        // If Request Sense data failed, reset the device and retry.
        //
        if (EFI_ERROR (Status)) {

          Status = AtapiSoftReset (IdeDev);

          //
          // if ATAPI soft reset fail,
          // use stronger reset mechanism -- ATA soft reset.
          //
          if (EFI_ERROR (Status)) {
            AtaSoftReset (IdeDev);
          }
          //
          // retry once more
          //
          RetryTimes++;
          continue;
        }
        
        //
        // No Media
        //
        if (IsNoMedia (IdeDev->SenseData, SenseCounts)) {

          IdeDev->BlkIo.Media->MediaPresent = FALSE;
          IdeDev->BlkIo.Media->LastBlock    = 0;
          return EFI_NO_MEDIA;
        }

        if (IsMediaError (IdeDev->SenseData, SenseCounts)) {
          return EFI_DEVICE_ERROR;
        }
        
        //
        // Media Changed
        //
        if (IsMediaChange (IdeDev->SenseData, SenseCounts)) {
          IdeDev->BlkIo.Media->MediaId++;
        }

        if (!IsDriveReady (IdeDev->SenseData, SenseCounts, &NeedRetry)) {
          
          //
          // Drive not ready: if NeedRetry, then retry once more;
          // else return error
          //
          if (NeedRetry) {
            //
            // Stall 1 second to wait for drive becoming ready
            //
            gBS->Stall (1000 * STALL_1_MILLI_SECOND);
            //
            // reset retry variable to zero,
            // to make it retry for "drive in progress of becoming ready".
            //
            RetryIndex = 0;
            continue;
          } else {
            AtapiSoftReset (IdeDev);
            return EFI_DEVICE_ERROR;
          }
        }
        //
        // if read capacity fail not for above reasons, retry once more
        //
        RetryTimes++;
      }

    }
  
    //
    // tell whether the readcapacity process is successful or not in the end
    //
    if (EFI_ERROR (ReadCapacityStatus)) {
      return EFI_DEVICE_ERROR;
    }
  }

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

  return EFI_SUCCESS;

}

EFI_STATUS
AtapiReadSectors (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *Buffer,
  IN  EFI_LBA         Lba,
  IN  UINTN           NumberOfBlocks
  )
/*++
  Name:
        AtapiReadSectors

  Purpose: 
        This function is called by the AtapiBlkIoReadBlocks() to perform
        read from media in block unit.

        The main command used to access media here is READ(10) Command. 
        READ(10) Command requests that the ATAPI device media transfer 
        specified data to the host. Data is transferred in block(sector) 
        unit. The maximum number of blocks that can be transferred once is
        65536. This is the main difference between READ(10) and READ(12) 
        Command. The maximum number of blocks in READ(12) is 2 power 32.

  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
          pointer pointing to IDE_BLK_IO_DEV data structure, used
          to record all the information of the IDE device.

        VOID      IN    *Buffer
          A pointer to the destination buffer for the data. 

        EFI_LBA     IN    Lba
          The starting logical block address to read from 
          on the device media.

        UINTN     IN    NumberOfBlocks
          The number of transfer data blocks.

  Returns:  
        return status is fully dependent on the return status
        of AtapiPacketCommandIn() function.

  Notes:

--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
// TODO:    Buffer - add argument and description to function comment
// TODO:    Lba - add argument and description to function comment
// TODO:    NumberOfBlocks - add argument and description to function comment
{

  ATAPI_PACKET_COMMAND  Packet;
  READ10_CMD            *Read10Packet;
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
  MaxBlock        = (UINT16) (65536 / BlockSize);

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

    Read10Packet->opcode = READ_10;

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

EFI_STATUS
AtapiWriteSectors (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *Buffer,
  IN  EFI_LBA         Lba,
  IN  UINTN           NumberOfBlocks
  )
/*++
  Name:
        AtapiWriteSectors

  Purpose: 
        This function is called by the AtapiBlkIoWriteBlocks() to perform
        write onto media in block unit.
        The main command used to access media here is Write(10) Command. 
        Write(10) Command requests that the ATAPI device media transfer 
        specified data to the host. Data is transferred in block (sector) 
        unit. The maximum number of blocks that can be transferred once is
        65536. 

  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
          pointer pointing to IDE_BLK_IO_DEV data structure, used
          to record all the information of the IDE device.

        VOID      IN    *Buffer
          A pointer to the source buffer for the data. 

        EFI_LBA     IN    Lba
          The starting logical block address to write onto 
          the device media.

        UINTN     IN    NumberOfBlocks
          The number of transfer data blocks.

  Returns:  
        return status is fully dependent on the return status
        of AtapiPacketCommandOut() function.

  Notes:

--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
// TODO:    Buffer - add argument and description to function comment
// TODO:    Lba - add argument and description to function comment
// TODO:    NumberOfBlocks - add argument and description to function comment
{

  ATAPI_PACKET_COMMAND  Packet;
  READ10_CMD            *Read10Packet;

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
    Read10Packet->opcode = WRITE_10;

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

EFI_STATUS
AtapiSoftReset (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++
  Name:
        AtapiSoftReset

  Purpose: 
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

            
  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
          pointer pointing to IDE_BLK_IO_DEV data structure, used
          to record all the information of the IDE device.

  Returns:  
        EFI_SUCCESS
          Soft reset completes successfully.

        EFI_DEVICE_ERROR
          Any step during the reset process is failed.

  Notes:

--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
{
  UINT8       Command;
  UINT8       DeviceSelect;
  EFI_STATUS  Status;

  //
  // for ATAPI device, no need to wait DRDY ready after device selecting.
  // (bit7 and bit5 are both set to 1 for backward compatibility)
  //
  DeviceSelect = (UINT8) (((bit7 | bit5) | (IdeDev->Device << 4)));
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Head, DeviceSelect);

  Command = ATAPI_SOFT_RESET_CMD;
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

EFI_STATUS
AtapiBlkIoReadBlocks (
  IN IDE_BLK_IO_DEV   *IdeBlkIoDevice,
  IN UINT32           MediaId,
  IN EFI_LBA          LBA,
  IN UINTN            BufferSize,
  OUT VOID            *Buffer
  )
/*++
  Name:
        AtapiBlkIoReadBlocks

  Purpose: 
        This function is the ATAPI implementation for ReadBlocks in the
        Block I/O Protocol interface.

  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeBlkIoDev
          Indicates the calling context.

        UINT32      IN    MediaId
          The media id that the read request is for.

        EFI_LBA     IN    LBA
          The starting logical block address to read from 
          on the device.

        UINTN     IN    BufferSize
          The size of the Buffer in bytes. This must be a
          multiple of the intrinsic block size of the device.

        VOID      OUT   *Buffer
          A pointer to the destination buffer for the data. 
          The caller is responsible for either having implicit
          or explicit ownership of the memory that data is read into.

  Returns:  
        EFI_SUCCESS 
          Read Blocks successfully.

        EFI_DEVICE_ERROR
          Read Blocks failed.

        EFI_NO_MEDIA
          There is no media in the device.

        EFI_MEDIA_CHANGED
          The MediaId is not for the current media.

        EFI_BAD_BUFFER_SIZE
          The BufferSize parameter is not a multiple of the 
          intrinsic block size of the device.

        EFI_INVALID_PARAMETER
          The read request contains LBAs that are not valid,
          or the data buffer is not valid.

  Notes:

--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeBlkIoDevice - add argument and description to function comment
// TODO:    MediaId - add argument and description to function comment
// TODO:    LBA - add argument and description to function comment
// TODO:    BufferSize - add argument and description to function comment
// TODO:    Buffer - add argument and description to function comment
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

  if (LBA > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if ((LBA + NumberOfBlocks - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Media->IoAlign > 1) && (((UINTN) Buffer & (Media->IoAlign - 1)) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // if all the parameters are valid, then perform read sectors command
  // to transfer data from device to host.
  //
  Status = AtapiReadSectors (IdeBlkIoDevice, Buffer, LBA, NumberOfBlocks);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }
  
  //
  // Read blocks succeeded
  //
  
  //
  // save the first block to the cache for performance
  //
  if (LBA == 0 && !IdeBlkIoDevice->Cache) {
    IdeBlkIoDevice->Cache = AllocatePool (BlockSize);
    if (IdeBlkIoDevice != NULL) {
      CopyMem ((UINT8 *) IdeBlkIoDevice->Cache, (UINT8 *) Buffer, BlockSize);
    }
  }

  return EFI_SUCCESS;

}

EFI_STATUS
AtapiBlkIoWriteBlocks (
  IN IDE_BLK_IO_DEV   *IdeBlkIoDevice,
  IN UINT32           MediaId,
  IN EFI_LBA          LBA,
  IN UINTN            BufferSize,
  OUT VOID            *Buffer
  )
/*++
  Name:
        AtapiBlkIoWriteBlocks

  Purpose: 
        This function is the ATAPI implementation for WriteBlocks in the
        Block I/O Protocol interface.

  Parameters:
        EFI_BLOCK_IO  IN    *This
          Indicates the calling context.

        UINT32      IN    MediaId
          The media id that the write request is for.

        EFI_LBA     IN    LBA
          The starting logical block address to write onto 
          the device.

        UINTN     IN    BufferSize
          The size of the Buffer in bytes. This must be a
          multiple of the intrinsic block size of the device.

        VOID      OUT   *Buffer
          A pointer to the source buffer for the data. 
          The caller is responsible for either having implicit
          or explicit ownership of the memory that data is 
          written from.

  Returns:  
        EFI_SUCCESS 
          Write Blocks successfully.

        EFI_DEVICE_ERROR
          Write Blocks failed.

        EFI_NO_MEDIA
          There is no media in the device.

        EFI_MEDIA_CHANGE
          The MediaId is not for the current media.

        EFI_BAD_BUFFER_SIZE
          The BufferSize parameter is not a multiple of the 
          intrinsic block size of the device.

        EFI_INVALID_PARAMETER
          The write request contains LBAs that are not valid,
          or the data buffer is not valid. 

  Notes:

--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeBlkIoDevice - add argument and description to function comment
// TODO:    MediaId - add argument and description to function comment
// TODO:    LBA - add argument and description to function comment
// TODO:    BufferSize - add argument and description to function comment
// TODO:    Buffer - add argument and description to function comment
// TODO:    EFI_MEDIA_CHANGED - add return value to function comment
// TODO:    EFI_WRITE_PROTECTED - add return value to function comment
{

  EFI_BLOCK_IO_MEDIA  *Media;
  UINTN               BlockSize;
  UINTN               NumberOfBlocks;
  EFI_STATUS          Status;
  BOOLEAN             MediaChange;

  if (LBA == 0 && IdeBlkIoDevice->Cache) {
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

    if (LBA == 0 && IdeBlkIoDevice->Cache) {
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

    if (LBA == 0 && IdeBlkIoDevice->Cache) {
      gBS->FreePool (IdeBlkIoDevice->Cache);
      IdeBlkIoDevice->Cache = NULL;
    }
    return EFI_NO_MEDIA;
  }

  if ((MediaId != Media->MediaId) || MediaChange) {

    if (LBA == 0 && IdeBlkIoDevice->Cache) {
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

  if (LBA > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if ((LBA + NumberOfBlocks - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Media->IoAlign > 1) && (((UINTN) Buffer & (Media->IoAlign - 1)) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // if all the parameters are valid,
  // then perform write sectors command to transfer data from host to device.
  //
  Status = AtapiWriteSectors (IdeBlkIoDevice, Buffer, LBA, NumberOfBlocks);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;

}

//
// The following functions are a set of helper functions,
// which are used to parse sense key returned by the device.
//

BOOLEAN
IsNoMedia (
  IN  REQUEST_SENSE_DATA    *SenseData,
  IN  UINTN                 SenseCounts
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  SenseData   - TODO: add argument description
  SenseCounts - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  REQUEST_SENSE_DATA  *SensePointer;
  UINTN               Index;
  BOOLEAN             NoMedia;

  NoMedia       = FALSE;
  SensePointer  = SenseData;

  for (Index = 0; Index < SenseCounts; Index++) {
    //
    // Sense Key is SK_NOT_READY (0x2),
    // Additional Sense Code is ASC_NO_MEDIA (0x3A)
    //
    if ((SensePointer->sense_key == SK_NOT_READY) &&
        (SensePointer->addnl_sense_code == ASC_NO_MEDIA)) {

      NoMedia = TRUE;
    }

    SensePointer++;
  }

  return NoMedia;
}

BOOLEAN
IsMediaError (
  IN  REQUEST_SENSE_DATA    *SenseData,
  IN  UINTN                 SenseCounts
  )
/*++
  Name:
        IsMediaError

  Purpose: 
        Test if the device meets a media error after media changed

  Parameters:
        EQUEST_SENSE_DATA   IN  *SenseData
          pointer pointing to ATAPI device sense data list.
        UINTN               IN  SenseCounts
          sense data number of the list          

  Returns:  
        TRUE
          Device meets a media error

        FALSE
          No media error
--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    SenseData - add argument and description to function comment
// TODO:    SenseCounts - add argument and description to function comment
{
  REQUEST_SENSE_DATA  *SensePointer;
  UINTN               Index;
  BOOLEAN             IsError;

  IsError       = FALSE;
  SensePointer  = SenseData;

  for (Index = 0; Index < SenseCounts; Index++) {

    switch (SensePointer->sense_key) {

    case SK_MEDIUM_ERROR:
      //
      // Sense Key is SK_MEDIUM_ERROR (0x3)
      //
      switch (SensePointer->addnl_sense_code) {
      case ASC_MEDIA_ERR1:
      case ASC_MEDIA_ERR2:
      case ASC_MEDIA_ERR3:
      case ASC_MEDIA_ERR4:
        IsError = TRUE;
        break;

      default:
        break;
      }

      break;

    case SK_NOT_READY:
      //
      // Sense Key is SK_NOT_READY (0x2)
      //
      switch (SensePointer->addnl_sense_code) {
      //
      // Additional Sense Code is ASC_MEDIA_UPSIDE_DOWN (0x6)
      //
      case ASC_MEDIA_UPSIDE_DOWN:
        IsError = TRUE;
        break;

      default:
        break;
      }
      break;

    default:
      break;
    }

    SensePointer++;
  }

  return IsError;
}

BOOLEAN
IsMediaChange (
  IN  REQUEST_SENSE_DATA    *SenseData,
  IN  UINTN                 SenseCounts
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  SenseData   - TODO: add argument description
  SenseCounts - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  REQUEST_SENSE_DATA  *SensePointer;
  UINTN               Index;
  BOOLEAN             IsMediaChange;

  IsMediaChange = FALSE;
  SensePointer  = SenseData;

  for (Index = 0; Index < SenseCounts; Index++) {
    //
    // Sense Key is SK_UNIT_ATTENTION (0x6)
    //
    if ((SensePointer->sense_key == SK_UNIT_ATTENTION) &&
        (SensePointer->addnl_sense_code == ASC_MEDIA_CHANGE)) {

      IsMediaChange = TRUE;
    }

    SensePointer++;
  }

  return IsMediaChange;
}

BOOLEAN
IsDriveReady (
  IN  REQUEST_SENSE_DATA    *SenseData,
  IN  UINTN                 SenseCounts,
  OUT BOOLEAN               *NeedRetry
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  SenseData   - TODO: add argument description
  SenseCounts - TODO: add argument description
  NeedRetry   - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  REQUEST_SENSE_DATA  *SensePointer;
  UINTN               Index;
  BOOLEAN             IsReady;

  IsReady       = TRUE;
  *NeedRetry    = FALSE;
  SensePointer  = SenseData;

  for (Index = 0; Index < SenseCounts; Index++) {

    switch (SensePointer->sense_key) {

    case SK_NOT_READY:
      //
      // Sense Key is SK_NOT_READY (0x2)
      //
      switch (SensePointer->addnl_sense_code) {
      case ASC_NOT_READY:
        //
        // Additional Sense Code is ASC_NOT_READY (0x4)
        //
        switch (SensePointer->addnl_sense_code_qualifier) {
        case ASCQ_IN_PROGRESS:
          //
          // Additional Sense Code Qualifier is ASCQ_IN_PROGRESS (0x1)
          //
          IsReady     = FALSE;
          *NeedRetry  = TRUE;
          break;

        default:
          IsReady     = FALSE;
          *NeedRetry  = FALSE;
          break;
        }
        break;

      default:
        break;
      }
      break;

    default:
      break;
    }

    SensePointer++;
  }

  return IsReady;
}

BOOLEAN
HaveSenseKey (
  IN  REQUEST_SENSE_DATA    *SenseData,
  IN  UINTN                 SenseCounts
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  SenseData   - TODO: add argument description
  SenseCounts - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  BOOLEAN Have;

  Have = TRUE;

  //
  // if first sense key in the Sense Data Array is SK_NO_SENSE,
  // it indicates there is no more sense key in the Sense Data Array.
  //
  if (SenseData->sense_key == SK_NO_SENSE) {
    Have = FALSE;
  }

  return Have;
}

EFI_STATUS
IsLS120orZipWriteProtected (
  IN  IDE_BLK_IO_DEV    *IdeDev,
  OUT BOOLEAN           *WriteProtected
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev          - TODO: add argument description
  WriteProtected  - TODO: add argument description

Returns:

  EFI_DEVICE_ERROR - TODO: Add description for return value
  EFI_DEVICE_ERROR - TODO: Add description for return value
  EFI_SUCCESS - TODO: Add description for return value

--*/
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
