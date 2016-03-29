/** @file
  Header file for IDE mode of ATA host controller.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/



#ifndef __OPAL_PASSWORD_IDE_MODE_H__
#define __OPAL_PASSWORD_IDE_MODE_H__

typedef enum {
  EfiIdePrimary    = 0,
  EfiIdeSecondary  = 1,
  EfiIdeMaxChannel = 2
} EFI_IDE_CHANNEL;

typedef enum {
  EfiIdeMaster     = 0,
  EfiIdeSlave      = 1,
  EfiIdeMaxDevice  = 2
} EFI_IDE_DEVICE;

//
// IDE registers set
//
typedef struct {
  UINT16                          Data;
  UINT16                          ErrOrFeature;
  UINT16                          SectorCount;
  UINT16                          SectorNumber;
  UINT16                          CylinderLsb;
  UINT16                          CylinderMsb;
  UINT16                          Head;
  UINT16                          CmdOrStatus;
  UINT16                          AltOrDev;
} EFI_IDE_REGISTERS;

//
// Bit definitions in Programming Interface byte of the Class Code field
// in PCI IDE controller's Configuration Space
//
#define IDE_PRIMARY_OPERATING_MODE            BIT0
#define IDE_PRIMARY_PROGRAMMABLE_INDICATOR    BIT1
#define IDE_SECONDARY_OPERATING_MODE          BIT2
#define IDE_SECONDARY_PROGRAMMABLE_INDICATOR  BIT3

/**
  Get IDE i/o port registers' base addresses by mode.

  In 'Compatibility' mode, use fixed addresses.
  In Native-PCI mode, get base addresses from BARs in the PCI IDE controller's
  Configuration Space.

  The steps to get IDE i/o port registers' base addresses for each channel
  as follows:

  1. Examine the Programming Interface byte of the Class Code fields in PCI IDE
  controller's Configuration Space to determine the operating mode.

  2. a) In 'Compatibility' mode, use fixed addresses shown in the Table 1 below.
   ___________________________________________
  |           | Command Block | Control Block |
  |  Channel  |   Registers   |   Registers   |
  |___________|_______________|_______________|
  |  Primary  |  1F0h - 1F7h  |  3F6h - 3F7h  |
  |___________|_______________|_______________|
  | Secondary |  170h - 177h  |  376h - 377h  |
  |___________|_______________|_______________|

  Table 1. Compatibility resource mappings

  b) In Native-PCI mode, IDE registers are mapped into IO space using the BARs
  in IDE controller's PCI Configuration Space, shown in the Table 2 below.
   ___________________________________________________
  |           |   Command Block   |   Control Block   |
  |  Channel  |     Registers     |     Registers     |
  |___________|___________________|___________________|
  |  Primary  | BAR at offset 0x10| BAR at offset 0x14|
  |___________|___________________|___________________|
  | Secondary | BAR at offset 0x18| BAR at offset 0x1C|
  |___________|___________________|___________________|

  Table 2. BARs for Register Mapping

  @param[in] Bus                 The bus number of ata host controller.
  @param[in] Device              The device number of ata host controller.
  @param[in] Function            The function number of ata host controller.
  @param[in, out] IdeRegisters   Pointer to EFI_IDE_REGISTERS which is used to
                                 store the IDE i/o port registers' base addresses

  @retval EFI_UNSUPPORTED        Return this Value when the BARs is not IO type
  @retval EFI_SUCCESS            Get the Base address successfully
  @retval Other                  Read the pci configureation Data error

**/
EFI_STATUS
EFIAPI
GetIdeRegisterIoAddr (
  IN     UINTN                       Bus,
  IN     UINTN                       Device,
  IN     UINTN                       Function,
  IN OUT EFI_IDE_REGISTERS           *IdeRegisters
  );

/**
  Sends out an ATA Identify Command to the specified device.

  This function sends out the ATA Identify Command to the
  specified device. Only ATA device responses to this command. If
  the command succeeds, it returns the Identify Data structure which
  contains information about the device. This function extracts the
  information it needs to fill the IDE_BLK_IO_DEV Data structure,
  including device type, media block Size, media capacity, and etc.

  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS Data structure.
  @param Channel          The channel number of device.
  @param Device           The device number of device.
  @param Buffer           A pointer to Data Buffer which is used to contain IDENTIFY Data.

  @retval EFI_SUCCESS          Identify ATA device successfully.
  @retval EFI_DEVICE_ERROR     ATA Identify Device Command failed or device is not ATA device.
  @retval EFI_OUT_OF_RESOURCES Allocate memory failed.
**/
EFI_STATUS
EFIAPI
AtaIdentify (
  IN     EFI_IDE_REGISTERS             *IdeRegisters,
  IN     UINT8                         Channel,
  IN     UINT8                         Device,
  IN OUT ATA_IDENTIFY_DATA             *Buffer
  );

/**
  This function is used to send out ATA commands conforms to the PIO Data In Protocol.

  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS Data structure.
  @param Buffer           A pointer to the source Buffer for the Data.
  @param ByteCount        The Length of  the Data.
  @param Read             Flag used to determine the Data transfer direction.
                          Read equals 1, means Data transferred from device to host;
                          Read equals 0, means Data transferred from host to device.
  @param AtaCommandBlock  A pointer to EFI_ATA_COMMAND_BLOCK Data structure.
  @param AtaStatusBlock   A pointer to EFI_ATA_STATUS_BLOCK Data structure.
  @param Timeout          The time to complete the command.

  @retval EFI_SUCCESS      send out the ATA command and device send required Data successfully.
  @retval EFI_DEVICE_ERROR command sent failed.

**/
EFI_STATUS
EFIAPI
AtaPioDataInOut (
  IN     EFI_IDE_REGISTERS         *IdeRegisters,
  IN OUT VOID                      *Buffer,
  IN     UINT64                    ByteCount,
  IN     BOOLEAN                   Read,
  IN     EFI_ATA_COMMAND_BLOCK     *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK      *AtaStatusBlock,
  IN     UINT64                    Timeout
  );


#endif

