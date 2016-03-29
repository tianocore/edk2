/** @file
  This driver is used for Opal Password Feature support at IDE mode.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "OpalPasswordSmm.h"

/**
  Write multiple words of Data to the IDE Data port.
  Call the IO abstraction once to do the complete read,
  not one word at a time

  @param  Port       IO port to read
  @param  Count      No. of UINT16's to read
  @param  Buffer     Pointer to the Data Buffer for read

**/
VOID
EFIAPI
IdeWritePortWMultiple (
  IN  UINT16                Port,
  IN  UINTN                 Count,
  IN  UINT16                *Buffer
  )
{
  UINTN Index;

  for (Index = 0; Index < Count; Index++) {
    IoWrite16 (Port, Buffer[Index]);
  }
}

/**
  Reads multiple words of Data from the IDE Data port.
  Call the IO abstraction once to do the complete read,
  not one word at a time

  @param  Port     IO port to read
  @param  Count    Number of UINT16's to read
  @param  Buffer   Pointer to the Data Buffer for read

**/
VOID
EFIAPI
IdeReadPortWMultiple (
  IN  UINT16                Port,
  IN  UINTN                 Count,
  IN  UINT16                *Buffer
  )
{
  UINTN Index;

  for (Index = 0; Index < Count; Index++) {
    Buffer[Count] = IoRead16 (Port);
  }
}

/**
  This function is used to analyze the Status Register and print out
  some debug information and if there is ERR bit set in the Status
  Register, the Error Register's Value is also be parsed and print out.

  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS Data structure.

**/
VOID
EFIAPI
DumpAllIdeRegisters (
  IN     EFI_IDE_REGISTERS        *IdeRegisters
  )
{
  ASSERT (IdeRegisters != NULL);

  DEBUG_CODE_BEGIN ();
  if ((IoRead8 (IdeRegisters->CmdOrStatus) & ATA_STSREG_DWF) != 0) {
    DEBUG ((EFI_D_ERROR, "CheckRegisterStatus()-- %02x : Error : Write Fault\n", IoRead8 (IdeRegisters->CmdOrStatus)));
  }

  if ((IoRead8 (IdeRegisters->CmdOrStatus) & ATA_STSREG_CORR) != 0) {
    DEBUG ((EFI_D_ERROR, "CheckRegisterStatus()-- %02x : Error : Corrected Data\n", IoRead8 (IdeRegisters->CmdOrStatus)));
  }

  if ((IoRead8 (IdeRegisters->CmdOrStatus) & ATA_STSREG_ERR) != 0) {
    if ((IoRead8 (IdeRegisters->ErrOrFeature) & ATA_ERRREG_BBK) != 0) {
      DEBUG ((EFI_D_ERROR, "CheckRegisterStatus()-- %02x : Error : Bad Block Detected\n", IoRead8 (IdeRegisters->ErrOrFeature)));
    }

    if ((IoRead8 (IdeRegisters->ErrOrFeature) & ATA_ERRREG_UNC) != 0) {
      DEBUG ((EFI_D_ERROR, "CheckRegisterStatus()-- %02x : Error : Uncorrectable Data\n", IoRead8 (IdeRegisters->ErrOrFeature)));
    }

    if ((IoRead8 (IdeRegisters->ErrOrFeature) & ATA_ERRREG_MC) != 0) {
      DEBUG ((EFI_D_ERROR, "CheckRegisterStatus()-- %02x : Error : Media Change\n", IoRead8 (IdeRegisters->ErrOrFeature)));
    }

    if ((IoRead8 (IdeRegisters->ErrOrFeature) & ATA_ERRREG_ABRT) != 0) {
      DEBUG ((EFI_D_ERROR, "CheckRegisterStatus()-- %02x : Error : Abort\n", IoRead8 (IdeRegisters->ErrOrFeature)));
    }

    if ((IoRead8 (IdeRegisters->ErrOrFeature) & ATA_ERRREG_TK0NF) != 0) {
      DEBUG ((EFI_D_ERROR, "CheckRegisterStatus()-- %02x : Error : Track 0 Not Found\n", IoRead8 (IdeRegisters->ErrOrFeature)));
    }

    if ((IoRead8 (IdeRegisters->ErrOrFeature) & ATA_ERRREG_AMNF) != 0) {
      DEBUG ((EFI_D_ERROR, "CheckRegisterStatus()-- %02x : Error : Address Mark Not Found\n", IoRead8 (IdeRegisters->ErrOrFeature)));
    }
  }
  DEBUG_CODE_END ();
}

/**
  This function is used to analyze the Status Register and print out
  some debug information and if there is ERR bit set in the Status
  Register, the Error Register's Value is also be parsed and print out.

  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS Data structure.

  @retval EFI_SUCCESS       No err information in the Status Register.
  @retval EFI_DEVICE_ERROR  Any err information in the Status Register.

**/
EFI_STATUS
EFIAPI
CheckStatusRegister (
  IN  EFI_IDE_REGISTERS        *IdeRegisters
  )
{
  EFI_STATUS      Status;
  UINT8           StatusRegister;

  ASSERT (IdeRegisters != NULL);

  StatusRegister = IoRead8 (IdeRegisters->CmdOrStatus);

  if ((StatusRegister & (ATA_STSREG_ERR | ATA_STSREG_DWF | ATA_STSREG_CORR)) == 0) {
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

/**
  This function is used to poll for the DRQ bit clear in the Status
  Register. DRQ is cleared when the device is finished transferring Data.
  So this function is called after Data transfer is finished.

  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS Data structure.
  @param Timeout          The time to complete the command.

  @retval EFI_SUCCESS     DRQ bit clear within the time out.
  @retval EFI_TIMEOUT     DRQ bit not clear within the time out.

  @note
  Read Status Register will clear interrupt status.

**/
EFI_STATUS
EFIAPI
DRQClear (
  IN  EFI_IDE_REGISTERS         *IdeRegisters,
  IN  UINT64                    Timeout
  )
{
  UINT32  Delay;
  UINT8   StatusRegister;

  ASSERT (IdeRegisters != NULL);

  Delay = (UINT32) (DivU64x32(Timeout, 1000) + 1);
  do {
    StatusRegister = IoRead8 (IdeRegisters->CmdOrStatus);

    //
    // wait for BSY == 0 and DRQ == 0
    //
    if ((StatusRegister & ATA_STSREG_BSY) == 0) {

      if ((StatusRegister & ATA_STSREG_DRQ) == ATA_STSREG_DRQ) {
        return EFI_DEVICE_ERROR;
      } else {
        return EFI_SUCCESS;
      }
    }

    //
    //  Stall for 100 microseconds.
    //
    MicroSecondDelay (100);

    Delay--;

  } while (Delay > 0);

    return EFI_TIMEOUT;
}
/**
  This function is used to poll for the DRQ bit clear in the Alternate
  Status Register. DRQ is cleared when the device is finished
  transferring Data. So this function is called after Data transfer
  is finished.

  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS Data structure.
  @param Timeout          The time to complete the command.

  @retval EFI_SUCCESS     DRQ bit clear within the time out.

  @retval EFI_TIMEOUT     DRQ bit not clear within the time out.
  @note   Read Alternate Status Register will not clear interrupt status.

**/
EFI_STATUS
EFIAPI
DRQClear2 (
  IN  EFI_IDE_REGISTERS    *IdeRegisters,
  IN  UINT64               Timeout
  )
{
  UINT32  Delay;
  UINT8   AltRegister;

  ASSERT (IdeRegisters != NULL);

  Delay = (UINT32) (DivU64x32(Timeout, 1000) + 1);
  do {
    AltRegister = IoRead8 (IdeRegisters->AltOrDev);

    //
    //  wait for BSY == 0 and DRQ == 0
    //
    if ((AltRegister & ATA_STSREG_BSY) == 0) {
      if ((AltRegister & ATA_STSREG_DRQ) == ATA_STSREG_DRQ) {
        return EFI_DEVICE_ERROR;
      } else {
        return EFI_SUCCESS;
      }
    }

    //
    // Stall for 100 microseconds.
    //
    MicroSecondDelay (100);

    Delay--;

  } while (Delay > 0);

  return EFI_TIMEOUT;
}


/**
  This function is used to poll for the DRQ bit set in the Alternate Status Register.
  DRQ is set when the device is ready to transfer Data. So this function is called after
  the command is sent to the device and before required Data is transferred.

  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS Data structure.
  @param Timeout          The time to complete the command.

  @retval EFI_SUCCESS     DRQ bit set within the time out.
  @retval EFI_TIMEOUT     DRQ bit not set within the time out.
  @retval EFI_ABORTED     DRQ bit not set caused by the command abort.
  @note  Read Alternate Status Register will not clear interrupt status.

**/
EFI_STATUS
EFIAPI
DRQReady2 (
  IN  EFI_IDE_REGISTERS    *IdeRegisters,
  IN  UINT64               Timeout
  )
{
  UINT32  Delay;
  UINT8   AltRegister;
  UINT8   ErrorRegister;

  ASSERT (IdeRegisters != NULL);

  Delay = (UINT32) (DivU64x32(Timeout, 1000) + 1);

  do {
    //
    //  Read Alternate Status Register will not clear interrupt status
    //
    AltRegister = IoRead8 (IdeRegisters->AltOrDev);
    //
    // BSY == 0 , DRQ == 1
    //
    if ((AltRegister & ATA_STSREG_BSY) == 0) {
      if ((AltRegister & ATA_STSREG_ERR) == ATA_STSREG_ERR) {
        ErrorRegister = IoRead8 (IdeRegisters->ErrOrFeature);

        if ((ErrorRegister & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
          return EFI_ABORTED;
        }
        return EFI_DEVICE_ERROR;
      }

      if ((AltRegister & ATA_STSREG_DRQ) == ATA_STSREG_DRQ) {
        return EFI_SUCCESS;
      } else {
        return EFI_NOT_READY;
      }
    }

    //
    // Stall for 100 microseconds.
    //
    MicroSecondDelay (100);

    Delay--;
  } while (Delay > 0);

  return EFI_TIMEOUT;
}

/**
  This function is used to poll for the BSY bit clear in the Status Register. BSY
  is clear when the device is not busy. Every command must be sent after device is not busy.

  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS Data structure.
  @param Timeout          The time to complete the command.

  @retval EFI_SUCCESS     BSY bit clear within the time out.
  @retval EFI_TIMEOUT     BSY bit not clear within the time out.

  @note Read Status Register will clear interrupt status.
**/
EFI_STATUS
EFIAPI
WaitForBSYClear (
  IN  EFI_IDE_REGISTERS    *IdeRegisters,
  IN  UINT64               Timeout
  )
{
  UINT32  Delay;
  UINT8   StatusRegister;

  ASSERT (IdeRegisters != NULL);

  Delay = (UINT32) (DivU64x32(Timeout, 1000) + 1);
  do {
    StatusRegister = IoRead8 (IdeRegisters->CmdOrStatus);

    if ((StatusRegister & ATA_STSREG_BSY) == 0x00) {
      return EFI_SUCCESS;
    }

    //
    // Stall for 100 microseconds.
    //
    MicroSecondDelay (100);

    Delay--;

  } while (Delay > 0);

  return EFI_TIMEOUT;
}

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
  )
{
  UINT16            CommandBlockBaseAddr;
  UINT16            ControlBlockBaseAddr;
  UINT8             ClassCode;
  UINT32            BaseAddress[4];

  if (IdeRegisters == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ClassCode = PciRead8 (PCI_LIB_ADDRESS (Bus, Device, Function, 0x9));
  BaseAddress[0] = PciRead32 (PCI_LIB_ADDRESS (Bus, Device, Function, 0x10));
  BaseAddress[1] = PciRead32 (PCI_LIB_ADDRESS (Bus, Device, Function, 0x14));
  BaseAddress[2] = PciRead32 (PCI_LIB_ADDRESS (Bus, Device, Function, 0x18));
  BaseAddress[3] = PciRead32 (PCI_LIB_ADDRESS (Bus, Device, Function, 0x1C));

  if ((ClassCode & IDE_PRIMARY_OPERATING_MODE) == 0) {
    CommandBlockBaseAddr = 0x1f0;
    ControlBlockBaseAddr = 0x3f6;
  } else {
    //
    // The BARs should be of IO type
    //
    if ((BaseAddress[0] & BIT0) == 0 ||
        (BaseAddress[1] & BIT0) == 0) {
      return EFI_UNSUPPORTED;
    }

    CommandBlockBaseAddr = (UINT16) (BaseAddress[0] & 0x0000fff8);
    ControlBlockBaseAddr = (UINT16) ((BaseAddress[1] & 0x0000fffc) + 2);
  }

  //
  // Calculate IDE primary channel I/O register base address.
  //
  IdeRegisters[EfiIdePrimary].Data              = CommandBlockBaseAddr;
  IdeRegisters[EfiIdePrimary].ErrOrFeature      = (UINT16) (CommandBlockBaseAddr + 0x01);
  IdeRegisters[EfiIdePrimary].SectorCount       = (UINT16) (CommandBlockBaseAddr + 0x02);
  IdeRegisters[EfiIdePrimary].SectorNumber      = (UINT16) (CommandBlockBaseAddr + 0x03);
  IdeRegisters[EfiIdePrimary].CylinderLsb       = (UINT16) (CommandBlockBaseAddr + 0x04);
  IdeRegisters[EfiIdePrimary].CylinderMsb       = (UINT16) (CommandBlockBaseAddr + 0x05);
  IdeRegisters[EfiIdePrimary].Head              = (UINT16) (CommandBlockBaseAddr + 0x06);
  IdeRegisters[EfiIdePrimary].CmdOrStatus       = (UINT16) (CommandBlockBaseAddr + 0x07);
  IdeRegisters[EfiIdePrimary].AltOrDev          = ControlBlockBaseAddr;

  if ((ClassCode & IDE_SECONDARY_OPERATING_MODE) == 0) {
    CommandBlockBaseAddr = 0x170;
    ControlBlockBaseAddr = 0x376;
  } else {
    //
    // The BARs should be of IO type
    //
    if ((BaseAddress[2] & BIT0) == 0 ||
        (BaseAddress[3] & BIT0) == 0) {
      return EFI_UNSUPPORTED;
    }

    CommandBlockBaseAddr = (UINT16) (BaseAddress[2] & 0x0000fff8);
    ControlBlockBaseAddr = (UINT16) ((BaseAddress[3] & 0x0000fffc) + 2);
  }

  //
  // Calculate IDE secondary channel I/O register base address.
  //
  IdeRegisters[EfiIdeSecondary].Data              = CommandBlockBaseAddr;
  IdeRegisters[EfiIdeSecondary].ErrOrFeature      = (UINT16) (CommandBlockBaseAddr + 0x01);
  IdeRegisters[EfiIdeSecondary].SectorCount       = (UINT16) (CommandBlockBaseAddr + 0x02);
  IdeRegisters[EfiIdeSecondary].SectorNumber      = (UINT16) (CommandBlockBaseAddr + 0x03);
  IdeRegisters[EfiIdeSecondary].CylinderLsb       = (UINT16) (CommandBlockBaseAddr + 0x04);
  IdeRegisters[EfiIdeSecondary].CylinderMsb       = (UINT16) (CommandBlockBaseAddr + 0x05);
  IdeRegisters[EfiIdeSecondary].Head              = (UINT16) (CommandBlockBaseAddr + 0x06);
  IdeRegisters[EfiIdeSecondary].CmdOrStatus       = (UINT16) (CommandBlockBaseAddr + 0x07);
  IdeRegisters[EfiIdeSecondary].AltOrDev          = ControlBlockBaseAddr;

  return EFI_SUCCESS;
}

/**
  Send ATA Ext command into device with NON_DATA protocol.

  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS Data structure.
  @param AtaCommandBlock  A pointer to EFI_ATA_COMMAND_BLOCK Data structure.
  @param Timeout          The time to complete the command.

  @retval  EFI_SUCCESS Reading succeed
  @retval  EFI_DEVICE_ERROR Error executing commands on this device.

**/
EFI_STATUS
EFIAPI
AtaIssueCommand (
  IN  EFI_IDE_REGISTERS         *IdeRegisters,
  IN  EFI_ATA_COMMAND_BLOCK     *AtaCommandBlock,
  IN  UINT64                    Timeout
  )
{
  EFI_STATUS  Status;
  UINT8       DeviceHead;
  UINT8       AtaCommand;

  ASSERT (IdeRegisters != NULL);
  ASSERT (AtaCommandBlock != NULL);

  DeviceHead = AtaCommandBlock->AtaDeviceHead;
  AtaCommand = AtaCommandBlock->AtaCommand;

  Status = WaitForBSYClear (IdeRegisters, Timeout);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Select device (bit4), set LBA mode(bit6) (use 0xe0 for compatibility)
  //
  IoWrite8 (IdeRegisters->Head, (UINT8) (0xe0 | DeviceHead));

  //
  // set all the command parameters
  // Before write to all the following registers, BSY and DRQ must be 0.
  //
  Status = DRQClear2 (IdeRegisters, Timeout);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Fill the feature register, which is a two-byte FIFO. Need write twice.
  //
  IoWrite8 (IdeRegisters->ErrOrFeature, AtaCommandBlock->AtaFeaturesExp);
  IoWrite8 (IdeRegisters->ErrOrFeature, AtaCommandBlock->AtaFeatures);

  //
  // Fill the sector count register, which is a two-byte FIFO. Need write twice.
  //
  IoWrite8 (IdeRegisters->SectorCount, AtaCommandBlock->AtaSectorCountExp);
  IoWrite8 (IdeRegisters->SectorCount, AtaCommandBlock->AtaSectorCount);

  //
  // Fill the start LBA registers, which are also two-byte FIFO
  //
  IoWrite8 (IdeRegisters->SectorNumber, AtaCommandBlock->AtaSectorNumberExp);
  IoWrite8 (IdeRegisters->SectorNumber, AtaCommandBlock->AtaSectorNumber);

  IoWrite8 (IdeRegisters->CylinderLsb, AtaCommandBlock->AtaCylinderLowExp);
  IoWrite8 (IdeRegisters->CylinderLsb, AtaCommandBlock->AtaCylinderLow);

  IoWrite8 (IdeRegisters->CylinderMsb, AtaCommandBlock->AtaCylinderHighExp);
  IoWrite8 (IdeRegisters->CylinderMsb, AtaCommandBlock->AtaCylinderHigh);

  //
  // Send command via Command Register
  //
  IoWrite8 (IdeRegisters->CmdOrStatus, AtaCommand);

  //
  // Stall at least 400 microseconds.
  //
  MicroSecondDelay (400);

  return EFI_SUCCESS;
}

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
  )
{
  UINTN       WordCount;
  UINTN       Increment;
  UINT16      *Buffer16;
  EFI_STATUS  Status;

  if ((IdeRegisters == NULL) || (Buffer == NULL) || (AtaCommandBlock == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Issue ATA command
  //
  Status = AtaIssueCommand (IdeRegisters, AtaCommandBlock, Timeout);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  Buffer16 = (UINT16 *) Buffer;

  //
  // According to PIO Data in protocol, host can perform a series of reads to
  // the Data register after each time device set DRQ ready;
  // The Data Size of "a series of read" is command specific.
  // For most ATA command, Data Size received from device will not exceed
  // 1 sector, hence the Data Size for "a series of read" can be the whole Data
  // Size of one command request.
  // For ATA command such as Read Sector command, the Data Size of one ATA
  // command request is often larger than 1 sector, according to the
  // Read Sector command, the Data Size of "a series of read" is exactly 1
  // sector.
  // Here for simplification reason, we specify the Data Size for
  // "a series of read" to 1 sector (256 words) if Data Size of one ATA command
  // request is larger than 256 words.
  //
  Increment = 256;

  //
  // used to record bytes of currently transfered Data
  //
  WordCount = 0;

  while (WordCount < RShiftU64(ByteCount, 1)) {
    //
    // Poll DRQ bit set, Data transfer can be performed only when DRQ is ready
    //
    Status = DRQReady2 (IdeRegisters, Timeout);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    //
    // Get the byte count for one series of read
    //
    if ((WordCount + Increment) > RShiftU64(ByteCount, 1)) {
      Increment = (UINTN)(RShiftU64(ByteCount, 1) - WordCount);
    }

    if (Read) {
      IdeReadPortWMultiple (
        IdeRegisters->Data,
        Increment,
        Buffer16
        );
    } else {
      IdeWritePortWMultiple (
        IdeRegisters->Data,
        Increment,
        Buffer16
        );
    }

    Status = CheckStatusRegister (IdeRegisters);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    WordCount += Increment;
    Buffer16  += Increment;
  }

  Status = DRQClear (IdeRegisters, Timeout);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

Exit:
  //
  // Dump All Ide registers to ATA_STATUS_BLOCK
  //
  DumpAllIdeRegisters (IdeRegisters);

  return Status;
}

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
  )
{
  EFI_STATUS             Status;
  EFI_ATA_COMMAND_BLOCK  AtaCommandBlock;

  ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));

  AtaCommandBlock.AtaCommand    = ATA_CMD_IDENTIFY_DRIVE;
  AtaCommandBlock.AtaDeviceHead = (UINT8)(Device << 0x4);

  Status = AtaPioDataInOut (
             IdeRegisters,
             Buffer,
             sizeof (ATA_IDENTIFY_DATA),
             TRUE,
             &AtaCommandBlock,
             NULL,
             ATA_TIMEOUT
             );

  return Status;
}



