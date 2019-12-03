/** @file
  Header file for IDE mode of ATA host controller.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef __ATA_HC_IDE_MODE_H__
#define __ATA_HC_IDE_MODE_H__

typedef enum {
  EfiIdePrimary    = 0,
  EfiIdeSecondary  = 1,
  EfiIdeMaxChannel = 2
} EFI_IDE_CHANNEL;

typedef enum {
  EfiIdeMaster    = 0,
  EfiIdeSlave     = 1,
  EfiIdeMaxDevice = 2
} EFI_IDE_DEVICE;

///
/// PIO mode definition
///
typedef enum {
  EfiAtaPioModeBelow2,
  EfiAtaPioMode2,
  EfiAtaPioMode3,
  EfiAtaPioMode4
} EFI_ATA_PIO_MODE;

//
// Multi word DMA definition
//
typedef enum {
  EfiAtaMdmaMode0,
  EfiAtaMdmaMode1,
  EfiAtaMdmaMode2
} EFI_ATA_MDMA_MODE;

//
// UDMA mode definition
//
typedef enum {
  EfiAtaUdmaMode0,
  EfiAtaUdmaMode1,
  EfiAtaUdmaMode2,
  EfiAtaUdmaMode3,
  EfiAtaUdmaMode4,
  EfiAtaUdmaMode5
} EFI_ATA_UDMA_MODE;

//
// Bus Master Reg
//
#define BMIC_NREAD      BIT3
#define BMIC_START      BIT0
#define BMIS_INTERRUPT  BIT2
#define BMIS_ERROR      BIT1

#define BMIC_OFFSET    0x00
#define BMIS_OFFSET    0x02
#define BMID_OFFSET    0x04

//
// IDE transfer mode
//
#define EFI_ATA_MODE_DEFAULT_PIO 0x00
#define EFI_ATA_MODE_FLOW_PIO    0x01
#define EFI_ATA_MODE_MDMA        0x04
#define EFI_ATA_MODE_UDMA        0x08

typedef struct {
  UINT32  RegionBaseAddr;
  UINT16  ByteCount;
  UINT16  EndOfTable;
} EFI_ATA_DMA_PRD;

typedef struct {
  UINT8 ModeNumber   : 3;
  UINT8 ModeCategory : 5;
} EFI_ATA_TRANSFER_MODE;

typedef struct {
  UINT8 Sector;
  UINT8 Heads;
  UINT8 MultipleSector;
} EFI_ATA_DRIVE_PARMS;

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

  UINT16                          BusMasterBaseAddr;
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

  @param[in]      PciIo          Pointer to the EFI_PCI_IO_PROTOCOL instance
  @param[in, out] IdeRegisters   Pointer to EFI_IDE_REGISTERS which is used to
                                 store the IDE i/o port registers' base addresses

  @retval EFI_UNSUPPORTED        Return this value when the BARs is not IO type
  @retval EFI_SUCCESS            Get the Base address successfully
  @retval Other                  Read the pci configureation data error

**/
EFI_STATUS
EFIAPI
GetIdeRegisterIoAddr (
  IN     EFI_PCI_IO_PROTOCOL         *PciIo,
  IN OUT EFI_IDE_REGISTERS           *IdeRegisters
  );

/**
  This function is used to send out ATAPI commands conforms to the Packet Command
  with PIO Data In Protocol.

  @param[in] PciIo          Pointer to the EFI_PCI_IO_PROTOCOL instance
  @param[in] IdeRegisters   Pointer to EFI_IDE_REGISTERS which is used to
                            store the IDE i/o port registers' base addresses
  @param[in] Channel        The channel number of device.
  @param[in] Device         The device number of device.
  @param[in] Packet         A pointer to EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET data structure.

  @retval EFI_SUCCESS       send out the ATAPI packet command successfully
                            and device sends data successfully.
  @retval EFI_DEVICE_ERROR  the device failed to send data.

**/
EFI_STATUS
EFIAPI
AtaPacketCommandExecute (
  IN  EFI_PCI_IO_PROTOCOL                           *PciIo,
  IN  EFI_IDE_REGISTERS                             *IdeRegisters,
  IN  UINT8                                         Channel,
  IN  UINT8                                         Device,
  IN  EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET    *Packet
  );

#endif

