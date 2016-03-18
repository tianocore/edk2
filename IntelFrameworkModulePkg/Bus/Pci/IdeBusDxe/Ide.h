/** @file
  Header file for IDE Bus Driver, containing the helper functions'
  prototype.

  Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  2002-6: Add Atapi6 enhancement, support >120GB hard disk, including
  Add - IDEBlkIoReadBlocksExt() func definition
  Add - IDEBlkIoWriteBlocksExt() func definition

**/

#ifndef _IDE_H_
#define _IDE_H_

//
// Helper functions Prototype
//
/**
  read a one-byte data from a IDE port.

  @param  PciIo  The PCI IO protocol instance
  @param  Port   the IDE Port number 

  return  the one-byte data read from IDE port
**/
UINT8
IDEReadPortB (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port
  );

/**
  Reads multiple words of data from the IDE data port.
  Call the IO abstraction once to do the complete read,
  not one word at a time.

  @param  PciIo Pointer to the EFI_PCI_IO instance
  @param  Port IO port to read
  @param  Count No. of UINT16's to read
  @param  Buffer Pointer to the data buffer for read

**/
VOID
IDEReadPortWMultiple (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINTN                 Count,
  OUT  VOID                 *Buffer
  );

/**
  write a 1-byte data to a specific IDE port.

  @param  PciIo  PCI IO protocol instance
  @param  Port   The IDE port to be writen
  @param  Data   The data to write to the port
**/
VOID
IDEWritePortB (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINT8                 Data
  );

/**
  write a 1-word data to a specific IDE port.

  @param  PciIo  PCI IO protocol instance
  @param  Port   The IDE port to be writen
  @param  Data   The data to write to the port
**/
VOID
IDEWritePortW (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINT16                Data
  );

/**
  Write multiple words of data to the IDE data port.
  Call the IO abstraction once to do the complete read,
  not one word at a time.

  @param  PciIo Pointer to the EFI_PCI_IO instance
  @param  Port IO port to read
  @param  Count No. of UINT16's to read
  @param  Buffer Pointer to the data buffer for read

**/
VOID
IDEWritePortWMultiple (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINTN                 Count,
  IN  VOID                  *Buffer
  );

/**
  Get IDE IO port registers' base addresses by mode. In 'Compatibility' mode,
  use fixed addresses. In Native-PCI mode, get base addresses from BARs in
  the PCI IDE controller's Configuration Space.

  The steps to get IDE IO port registers' base addresses for each channel
  as follows:

  1. Examine the Programming Interface byte of the Class Code fields in PCI IDE
  controller's Configuration Space to determine the operating mode.

  2. a) In 'Compatibility' mode, use fixed addresses shown in the Table 1 below.
  <pre>
  ___________________________________________
  |           | Command Block | Control Block |
  |  Channel  |   Registers   |   Registers   |
  |___________|_______________|_______________|
  |  Primary  |  1F0h - 1F7h  |  3F6h - 3F7h  |
  |___________|_______________|_______________|
  | Secondary |  170h - 177h  |  376h - 377h  |
  |___________|_______________|_______________|

  Table 1. Compatibility resource mappings
  </pre>

  b) In Native-PCI mode, IDE registers are mapped into IO space using the BARs
  in IDE controller's PCI Configuration Space, shown in the Table 2 below.
  <pre>
  ___________________________________________________
  |           |   Command Block   |   Control Block   |
  |  Channel  |     Registers     |     Registers     |
  |___________|___________________|___________________|
  |  Primary  | BAR at offset 0x10| BAR at offset 0x14|
  |___________|___________________|___________________|
  | Secondary | BAR at offset 0x18| BAR at offset 0x1C|
  |___________|___________________|___________________|

  Table 2. BARs for Register Mapping
  </pre>
  @note Refer to Intel ICH4 datasheet, Control Block Offset: 03F4h for
  primary, 0374h for secondary. So 2 bytes extra offset should be
  added to the base addresses read from BARs.

  For more details, please refer to PCI IDE Controller Specification and Intel
  ICH4 Datasheet.

  @param  PciIo Pointer to the EFI_PCI_IO_PROTOCOL instance
  @param  IdeRegsBaseAddr Pointer to IDE_REGISTERS_BASE_ADDR to
          receive IDE IO port registers' base addresses
  
  @retval EFI_UNSUPPORTED return this value when the BARs is not IO type
  @retval EFI_SUCCESS     Get the Base address successfully
  @retval other           read the pci configureation data error

**/
EFI_STATUS
GetIdeRegistersBaseAddr (
  IN  EFI_PCI_IO_PROTOCOL         *PciIo,
  OUT IDE_REGISTERS_BASE_ADDR     *IdeRegsBaseAddr
  );

/**
  This function is used to requery IDE resources. The IDE controller will
  probably switch between native and legacy modes during the EFI->CSM->OS
  transfer. We do this everytime before an BlkIo operation to ensure its
  succeess.

  @param  IdeDev The BLK_IO private data which specifies the IDE device

  @retval EFI_INVALID_PARAMETER return this value when the channel is invalid
  @retval EFI_SUCCESS           reassign the IDE IO resource successfully
  @retval other                 get the IDE current base address effor

**/
EFI_STATUS
ReassignIdeResources (
  IN  IDE_BLK_IO_DEV  *IdeDev
  );

/**
  Detect if there is disk attached to this port.

  @param  IdeDev The BLK_IO private data which specifies the IDE device.
  
  @retval EFI_NOT_FOUND   The device or channel is not found
  @retval EFI_SUCCESS     The device is found

**/
EFI_STATUS
DiscoverIdeDevice (
  IN IDE_BLK_IO_DEV *IdeDev
  );

/**
  This interface is used to initialize all state data related to the
  detection of one channel.

**/
VOID
InitializeIDEChannelData (
  VOID
  );

/**
  This function is used to poll for the DRQ bit clear in the Status
  Register. DRQ is cleared when the device is finished transferring data.
  So this function is called after data transfer is finished.

  @param IdeDev                 pointer pointing to IDE_BLK_IO_DEV data structure, used 
                                to record all the information of the IDE device.
  @param TimeoutInMilliSeconds  used to designate the timeout for the DRQ clear.

  @retval EFI_SUCCESS           DRQ bit clear within the time out.

  @retval EFI_TIMEOUT           DRQ bit not clear within the time out.

  @note
  Read Status Register will clear interrupt status.

**/
EFI_STATUS
DRQClear (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  );

/**
  This function is used to poll for the DRQ bit clear in the Alternate
  Status Register. DRQ is cleared when the device is finished
  transferring data. So this function is called after data transfer
  is finished.

  @param IdeDev                pointer pointing to IDE_BLK_IO_DEV data structure, used 
                               to record all the information of the IDE device.

  @param TimeoutInMilliSeconds used to designate the timeout for the DRQ clear.

  @retval EFI_SUCCESS          DRQ bit clear within the time out.

  @retval EFI_TIMEOUT          DRQ bit not clear within the time out.
  @note
  Read Alternate Status Register will not clear interrupt status.

**/
EFI_STATUS
DRQClear2 (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  );

/**
  This function is used to poll for the DRQ bit set in the
  Status Register.
  DRQ is set when the device is ready to transfer data. So this function
  is called after the command is sent to the device and before required
  data is transferred.

  @param IdeDev                pointer pointing to IDE_BLK_IO_DEV data structure,used to
                               record all the information of the IDE device.
  @param TimeoutInMilliSeconds used to designate the timeout for the DRQ ready.

  @retval EFI_SUCCESS          DRQ bit set within the time out.
  @retval EFI_TIMEOUT          DRQ bit not set within the time out.
  @retval EFI_ABORTED          DRQ bit not set caused by the command abort.

  @note  Read Status Register will clear interrupt status.

**/
EFI_STATUS
DRQReady (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  );

/**
  This function is used to poll for the DRQ bit set in the Alternate Status Register.
  DRQ is set when the device is ready to transfer data. So this function is called after 
  the command is sent to the device and before required data is transferred.

  @param IdeDev                pointer pointing to IDE_BLK_IO_DEV data structure, used to 
                               record all the information of the IDE device.

  @param TimeoutInMilliSeconds used to designate the timeout for the DRQ ready.

  @retval EFI_SUCCESS           DRQ bit set within the time out.
  @retval EFI_TIMEOUT           DRQ bit not set within the time out.
  @retval EFI_ABORTED           DRQ bit not set caused by the command abort.
  @note  Read Alternate Status Register will not clear interrupt status.

**/
EFI_STATUS
DRQReady2 (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  );

/**
  This function is used to poll for the BSY bit clear in the Status Register. BSY
  is clear when the device is not busy. Every command must be sent after device is not busy.

  @param IdeDev                pointer pointing to IDE_BLK_IO_DEV data structure, used 
                               to record all the information of the IDE device.
  @param TimeoutInMilliSeconds used to designate the timeout for the DRQ ready.

  @retval EFI_SUCCESS          BSY bit clear within the time out.
  @retval EFI_TIMEOUT          BSY bit not clear within the time out.

  @note Read Status Register will clear interrupt status.
**/
EFI_STATUS
WaitForBSYClear (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  );

/**
  This function is used to poll for the BSY bit clear in the Alternate Status Register. 
  BSY is clear when the device is not busy. Every command must be sent after device is 
  not busy.

  @param IdeDev               pointer pointing to IDE_BLK_IO_DEV data structure, used to record 
                              all the information of the IDE device.
  @param TimeoutInMilliSeconds used to designate the timeout for the DRQ ready.

  @retval EFI_SUCCESS         BSY bit clear within the time out.
  @retval EFI_TIMEOUT         BSY bit not clear within the time out.
  @note   Read Alternate Status Register will not clear interrupt status.

**/
EFI_STATUS
WaitForBSYClear2 (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  );

/**
  This function is used to poll for the DRDY bit set in the Status Register. DRDY
  bit is set when the device is ready to accept command. Most ATA commands must be 
  sent after DRDY set except the ATAPI Packet Command.

  @param IdeDev               pointer pointing to IDE_BLK_IO_DEV data structure, used
                              to record all the information of the IDE device.
  @param DelayInMilliSeconds  used to designate the timeout for the DRQ ready.

  @retval EFI_SUCCESS         DRDY bit set within the time out.
  @retval EFI_TIMEOUT         DRDY bit not set within the time out.

  @note  Read Status Register will clear interrupt status.
**/
EFI_STATUS
DRDYReady (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           DelayInMilliSeconds
  );

/**
  This function is used to poll for the DRDY bit set in the Alternate Status Register. 
  DRDY bit is set when the device is ready to accept command. Most ATA commands must 
  be sent after DRDY set except the ATAPI Packet Command.

  @param IdeDev              pointer pointing to IDE_BLK_IO_DEV data structure, used
                             to record all the information of the IDE device.
  @param DelayInMilliSeconds used to designate the timeout for the DRQ ready.

  @retval EFI_SUCCESS      DRDY bit set within the time out.
  @retval EFI_TIMEOUT      DRDY bit not set within the time out.

  @note  Read Alternate Status Register will clear interrupt status.

**/
EFI_STATUS
DRDYReady2 (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           DelayInMilliSeconds
  );

//
//  ATA device functions' prototype
//
/**
  Sends out an ATA Identify Command to the specified device.

  This function is called by DiscoverIdeDevice() during its device
  identification. It sends out the ATA Identify Command to the
  specified device. Only ATA device responses to this command. If
  the command succeeds, it returns the Identify data structure which
  contains information about the device. This function extracts the
  information it needs to fill the IDE_BLK_IO_DEV data structure,
  including device type, media block size, media capacity, and etc.

  @param IdeDev  pointer pointing to IDE_BLK_IO_DEV data structure,used to record 
                 all the information of the IDE device.

  @retval EFI_SUCCESS      Identify ATA device successfully.
  @retval EFI_DEVICE_ERROR ATA Identify Device Command failed or device is not ATA device.
  @note  parameter IdeDev will be updated in this function.

**/
EFI_STATUS
ATAIdentify (
  IN  IDE_BLK_IO_DEV  *IdeDev
  );

/**
  This function is called by ATAIdentify() or ATAPIIdentify() to print device's module name.

  @param  IdeDev   pointer pointing to IDE_BLK_IO_DEV data structure, used to record
                   all the information of the IDE device.
**/
VOID
PrintAtaModuleName (
  IN  IDE_BLK_IO_DEV  *IdeDev
  );
/**
  This function is used to send out ATA commands conforms to the PIO Data In Protocol.

  @param IdeDev       pointer pointing to IDE_BLK_IO_DEV data structure, used to record 
                      all the information of the IDE device.
  @param Buffer       buffer contained data transferred from device to host.
  @param ByteCount    data size in byte unit of the buffer.
  @param AtaCommand   value of the Command Register
  @param Head         value of the Head/Device Register
  @param SectorCount  value of the Sector Count Register
  @param SectorNumber value of the Sector Number Register
  @param CylinderLsb  value of the low byte of the Cylinder Register
  @param CylinderMsb  value of the high byte of the Cylinder Register
  
  @retval EFI_SUCCESS      send out the ATA command and device send required data successfully.
  @retval EFI_DEVICE_ERROR command sent failed.

**/
EFI_STATUS
AtaPioDataIn (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *Buffer,
  IN  UINT32          ByteCount,
  IN  UINT8           AtaCommand,
  IN  UINT8           Head,
  IN  UINT8           SectorCount,
  IN  UINT8           SectorNumber,
  IN  UINT8           CylinderLsb,
  IN  UINT8           CylinderMsb
  );

/**
  This function is used to send out ATA commands conforms to the
  PIO Data Out Protocol.

  @param IdeDev       pointer pointing to IDE_BLK_IO_DEV data structure, used
                      to record all the information of the IDE device.
  @param *Buffer      buffer contained data transferred from host to device.
  @param ByteCount    data size in byte unit of the buffer.
  @param AtaCommand   value of the Command Register
  @param Head         value of the Head/Device Register
  @param SectorCount  value of the Sector Count Register
  @param SectorNumber value of the Sector Number Register
  @param CylinderLsb  value of the low byte of the Cylinder Register
  @param CylinderMsb  value of the high byte of the Cylinder Register

  @retval EFI_SUCCESS      send out the ATA command and device received required
                           data successfully.
  @retval EFI_DEVICE_ERROR command sent failed.

**/
EFI_STATUS
AtaPioDataOut (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *Buffer,
  IN  UINT32          ByteCount,
  IN  UINT8           AtaCommand,
  IN  UINT8           Head,
  IN  UINT8           SectorCount,
  IN  UINT8           SectorNumber,
  IN  UINT8           CylinderLsb,
  IN  UINT8           CylinderMsb
  );

/**
  This function is used to analyze the Status Register and print out
  some debug information and if there is ERR bit set in the Status
  Register, the Error Register's value is also be parsed and print out.

  @param IdeDev  pointer pointing to IDE_BLK_IO_DEV data structure, used to 
                 record all the information of the IDE device.

  @retval EFI_SUCCESS       No err information in the Status Register.
  @retval EFI_DEVICE_ERROR  Any err information in the Status Register.

**/
EFI_STATUS
CheckErrorStatus (
  IN  IDE_BLK_IO_DEV  *IdeDev
  );

/**
  This function is used to implement the Soft Reset on the specified device. But,
  the ATA Soft Reset mechanism is so strong a reset method that it will force 
  resetting on both devices connected to the same cable.

  It is called by IdeBlkIoReset(), a interface function of Block
  I/O protocol.

  This function can also be used by the ATAPI device to perform reset when
  ATAPI Reset command is failed.

  @param IdeDev  pointer pointing to IDE_BLK_IO_DEV data structure, used to record
                 all the information of the IDE device.
  @retval EFI_SUCCESS       Soft reset completes successfully.
  @retval EFI_DEVICE_ERROR  Any step during the reset process is failed.

  @note  The registers initial values after ATA soft reset are different
         to the ATA device and ATAPI device.
**/
EFI_STATUS
AtaSoftReset (
  IN  IDE_BLK_IO_DEV  *IdeDev
  );

/**
  This function is the ATA implementation for ReadBlocks in the
  Block I/O Protocol interface.

  @param IdeBlkIoDevice Indicates the calling context.
  @param MediaId        The media id that the read request is for.
  @param Lba            The starting logical block address to read from on the device.
  @param BufferSize     The size of the Buffer in bytes. This must be a  multiple
                        of the intrinsic block size of the device.

  @param Buffer         A pointer to the destination buffer for the data. The caller
                        is responsible for either having implicit or explicit ownership
                        of the memory that data is read into.

  @retval EFI_SUCCESS          Read Blocks successfully.
  @retval EFI_DEVICE_ERROR     Read Blocks failed.
  @retval EFI_NO_MEDIA         There is no media in the device.
  @retval EFI_MEDIA_CHANGE     The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE  The BufferSize parameter is not a multiple of the
                               intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER  The read request contains LBAs that are not valid,
                                 or the data buffer is not valid.

  @note If Read Block error because of device error, this function will call
        AtaSoftReset() function to reset device.

**/
EFI_STATUS
AtaBlkIoReadBlocks (
  IN IDE_BLK_IO_DEV   *IdeBlkIoDevice,
  IN UINT32           MediaId,
  IN EFI_LBA          Lba,
  IN UINTN            BufferSize,
  OUT VOID            *Buffer
  );

/**
  This function is the ATA implementation for WriteBlocks in the
  Block I/O Protocol interface.

  @param IdeBlkIoDevice  Indicates the calling context.
  @param MediaId         The media id that the write request is for.
  @param Lba             The starting logical block address to write onto the device.
  @param BufferSize      The size of the Buffer in bytes. This must be a multiple
                         of the intrinsic block size of the device.
  @param Buffer          A pointer to the source buffer for the data.The caller
                         is responsible for either having implicit or explicit 
                         ownership of the memory that data is written from.

  @retval EFI_SUCCESS       Write Blocks successfully.
  @retval EFI_DEVICE_ERROR  Write Blocks failed.
  @retval EFI_NO_MEDIA      There is no media in the device.
  @retval EFI_MEDIA_CHANGE  The MediaId is not for the current media.

  @retval EFI_BAD_BUFFER_SIZE   The BufferSize parameter is not a multiple of the
                                intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the data buffer is not valid.

  @note If Write Block error because of device error, this function will call
        AtaSoftReset() function to reset device.
**/
EFI_STATUS
AtaBlkIoWriteBlocks (
  IN IDE_BLK_IO_DEV   *IdeBlkIoDevice,
  IN UINT32           MediaId,
  IN EFI_LBA          Lba,
  IN UINTN            BufferSize,
  OUT VOID            *Buffer
  );

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
  );

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
  );

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
  );

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
  );

/**
  Release resources of an IDE device before stopping it.

  @param IdeBlkIoDevice  Standard IDE device private data structure

**/
VOID
ReleaseIdeResources (
  IN  IDE_BLK_IO_DEV  *IdeBlkIoDevice
  );

/**
  Set the calculated Best transfer mode to a detected device

  @param IdeDev       Standard IDE device private data structure
  @param TransferMode The device transfer mode to be set
  @return Set transfer mode Command execute status.
**/
EFI_STATUS
SetDeviceTransferMode (
  IN IDE_BLK_IO_DEV       *IdeDev,
  IN ATA_TRANSFER_MODE    *TransferMode
  );
/**
  Send ATA command into device with NON_DATA protocol.

  @param  IdeDev Standard IDE device private data structure
  @param  AtaCommand The ATA command to be sent
  @param  Device The value in Device register
  @param  Feature The value in Feature register
  @param  SectorCount The value in SectorCount register
  @param  LbaLow The value in LBA_LOW register
  @param  LbaMiddle The value in LBA_MIDDLE register
  @param  LbaHigh The value in LBA_HIGH register

  @retval  EFI_SUCCESS Reading succeed
  @retval  EFI_ABORTED Command failed
  @retval  EFI_DEVICE_ERROR Device status error.

**/
EFI_STATUS
AtaNonDataCommandIn (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINT8           AtaCommand,
  IN  UINT8           Device,
  IN  UINT8           Feature,
  IN  UINT8           SectorCount,
  IN  UINT8           LbaLow,
  IN  UINT8           LbaMiddle,
  IN  UINT8           LbaHigh
  );

/**
  Send ATA Ext command into device with NON_DATA protocol.

  @param  IdeDev Standard IDE device private data structure
  @param  AtaCommand The ATA command to be sent
  @param  Device The value in Device register
  @param  Feature The value in Feature register
  @param  SectorCount The value in SectorCount register
  @param  LbaAddress The Lba address in 48-bit mode

  @retval  EFI_SUCCESS Reading succeed
  @retval  EFI_ABORTED Command failed
  @retval  EFI_DEVICE_ERROR Device status error.

**/
EFI_STATUS
AtaNonDataCommandInExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINT8           AtaCommand,
  IN  UINT8           Device,
  IN  UINT16          Feature,
  IN  UINT16          SectorCount,
  IN  EFI_LBA         LbaAddress
  );
/**
  Enable Long Physical Sector Feature for ATA device.

  @param   IdeDev  The IDE device data

  @retval  EFI_SUCCESS      The ATA device supports Long Physical Sector feature
                            and corresponding fields in BlockIo structure is updated.
  @retval  EFI_UNSUPPORTED  The device is not ATA device or Long Physical Sector
                            feature is not supported.
**/
EFI_STATUS
AtaEnableLongPhysicalSector (
  IN  IDE_BLK_IO_DEV  *IdeDev
  );

/**
  Set drive parameters for devices not support PACKETS command.

  @param IdeDev          Standard IDE device private data structure
  @param DriveParameters The device parameters to be set into the disk
  @return SetParameters Command execute status.

**/
EFI_STATUS
SetDriveParameters (
  IN IDE_BLK_IO_DEV       *IdeDev,
  IN ATA_DRIVE_PARMS      *DriveParameters
  );

/**
  Enable Interrupt on IDE controller.

  @param  IdeDev   Standard IDE device private data structure

  @retval  EFI_SUCCESS Enable Interrupt successfully
**/
EFI_STATUS
EnableInterrupt (
  IN IDE_BLK_IO_DEV       *IdeDev
  );
#endif
