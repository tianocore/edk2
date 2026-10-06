/** @file
  Header file for AHCI mode of ATA host controller.

  Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AtaAtapiPassThru.h"

/**
  read a one-byte data from a IDE port.

  @param  PciIo  A pointer to EFI_PCI_IO_PROTOCOL data structure
  @param  Port   The IDE Port number

  @return  the one-byte data read from IDE port
**/
UINT8
EFIAPI
IdeReadPortB (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINT16               Port
  )
{
  UINT8  Data;

  ASSERT (PciIo != NULL);

  Data = 0;
  //
  // perform 1-byte data read from register
  //
  PciIo->Io.Read (
              PciIo,
              EfiPciIoWidthUint8,
              EFI_PCI_IO_PASS_THROUGH_BAR,
              (UINT64)Port,
              1,
              &Data
              );
  return Data;
}

/**
  write a 1-byte data to a specific IDE port.

  @param  PciIo  A pointer to EFI_PCI_IO_PROTOCOL data structure
  @param  Port   The IDE port to be written
  @param  Data   The data to write to the port
**/
VOID
EFIAPI
IdeWritePortB (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINT16               Port,
  IN  UINT8                Data
  )
{
  ASSERT (PciIo != NULL);

  //
  // perform 1-byte data write to register
  //
  PciIo->Io.Write (
              PciIo,
              EfiPciIoWidthUint8,
              EFI_PCI_IO_PASS_THROUGH_BAR,
              (UINT64)Port,
              1,
              &Data
              );
}

/**
  write a 1-word data to a specific IDE port.

  @param  PciIo  A pointer to EFI_PCI_IO_PROTOCOL data structure
  @param  Port   The IDE port to be written
  @param  Data   The data to write to the port
**/
VOID
EFIAPI
IdeWritePortW (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINT16               Port,
  IN  UINT16               Data
  )
{
  ASSERT (PciIo != NULL);

  //
  // perform 1-word data write to register
  //
  PciIo->Io.Write (
              PciIo,
              EfiPciIoWidthUint16,
              EFI_PCI_IO_PASS_THROUGH_BAR,
              (UINT64)Port,
              1,
              &Data
              );
}

/**
  write a 2-word data to a specific IDE port.

  @param  PciIo  A pointer to EFI_PCI_IO_PROTOCOL data structure
  @param  Port   The IDE port to be written
  @param  Data   The data to write to the port
**/
VOID
EFIAPI
IdeWritePortDW (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINT16               Port,
  IN  UINT32               Data
  )
{
  ASSERT (PciIo != NULL);

  //
  // perform 2-word data write to register
  //
  PciIo->Io.Write (
              PciIo,
              EfiPciIoWidthUint32,
              EFI_PCI_IO_PASS_THROUGH_BAR,
              (UINT64)Port,
              1,
              &Data
              );
}

/**
  Write multiple words of data to the IDE data port.
  Call the IO abstraction once to do the complete read,
  not one word at a time

  @param  PciIo      A pointer to EFI_PCI_IO_PROTOCOL data structure
  @param  Port       IO port to read
  @param  Count      No. of UINT16's to read
  @param  Buffer     Pointer to the data buffer for read

**/
VOID
EFIAPI
IdeWritePortWMultiple (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINT16               Port,
  IN  UINTN                Count,
  IN  VOID                 *Buffer
  )
{
  ASSERT (PciIo  != NULL);
  ASSERT (Buffer != NULL);

  //
  // perform UINT16 data write to the FIFO
  //
  PciIo->Io.Write (
              PciIo,
              EfiPciIoWidthFifoUint16,
              EFI_PCI_IO_PASS_THROUGH_BAR,
              (UINT64)Port,
              Count,
              (UINT16 *)Buffer
              );
}

/**
  Reads multiple words of data from the IDE data port.
  Call the IO abstraction once to do the complete read,
  not one word at a time

  @param  PciIo    A pointer to EFI_PCI_IO_PROTOCOL data structure
  @param  Port     IO port to read
  @param  Count    Number of UINT16's to read
  @param  Buffer   Pointer to the data buffer for read

**/
VOID
EFIAPI
IdeReadPortWMultiple (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINT16               Port,
  IN  UINTN                Count,
  IN  VOID                 *Buffer
  )
{
  ASSERT (PciIo  != NULL);
  ASSERT (Buffer != NULL);

  //
  // Perform UINT16 data read from FIFO
  //
  PciIo->Io.Read (
              PciIo,
              EfiPciIoWidthFifoUint16,
              EFI_PCI_IO_PASS_THROUGH_BAR,
              (UINT64)Port,
              Count,
              (UINT16 *)Buffer
              );
}

/**
  This function is used to analyze the Status Register and print out
  some debug information and if there is ERR bit set in the Status
  Register, the Error Register's value is also be parsed and print out.

  @param PciIo            A pointer to EFI_PCI_IO_PROTOCOL data structure.
  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param AtaStatusBlock   A pointer to EFI_ATA_STATUS_BLOCK data structure.

**/
VOID
EFIAPI
DumpAllIdeRegisters (
  IN     EFI_PCI_IO_PROTOCOL   *PciIo,
  IN     EFI_IDE_REGISTERS     *IdeRegisters,
  IN OUT EFI_ATA_STATUS_BLOCK  *AtaStatusBlock
  )
{
  EFI_ATA_STATUS_BLOCK  StatusBlock;

  ASSERT (PciIo != NULL);
  ASSERT (IdeRegisters != NULL);

  ZeroMem (&StatusBlock, sizeof (EFI_ATA_STATUS_BLOCK));

  StatusBlock.AtaStatus          = IdeReadPortB (PciIo, IdeRegisters->CmdOrStatus);
  StatusBlock.AtaError           = IdeReadPortB (PciIo, IdeRegisters->ErrOrFeature);
  StatusBlock.AtaSectorCount     = IdeReadPortB (PciIo, IdeRegisters->SectorCount);
  StatusBlock.AtaSectorCountExp  = IdeReadPortB (PciIo, IdeRegisters->SectorCount);
  StatusBlock.AtaSectorNumber    = IdeReadPortB (PciIo, IdeRegisters->SectorNumber);
  StatusBlock.AtaSectorNumberExp = IdeReadPortB (PciIo, IdeRegisters->SectorNumber);
  StatusBlock.AtaCylinderLow     = IdeReadPortB (PciIo, IdeRegisters->CylinderLsb);
  StatusBlock.AtaCylinderLowExp  = IdeReadPortB (PciIo, IdeRegisters->CylinderLsb);
  StatusBlock.AtaCylinderHigh    = IdeReadPortB (PciIo, IdeRegisters->CylinderMsb);
  StatusBlock.AtaCylinderHighExp = IdeReadPortB (PciIo, IdeRegisters->CylinderMsb);
  StatusBlock.AtaDeviceHead      = IdeReadPortB (PciIo, IdeRegisters->Head);

  if (AtaStatusBlock != NULL) {
    //
    // Dump the content of all ATA registers.
    //
    CopyMem (AtaStatusBlock, &StatusBlock, sizeof (EFI_ATA_STATUS_BLOCK));
  }

  DEBUG_CODE_BEGIN ();
  if ((StatusBlock.AtaStatus & ATA_STSREG_DWF) != 0) {
    DEBUG ((DEBUG_ERROR, "CheckRegisterStatus()-- %02x : Error : Write Fault\n", StatusBlock.AtaStatus));
  }

  if ((StatusBlock.AtaStatus & ATA_STSREG_CORR) != 0) {
    DEBUG ((DEBUG_ERROR, "CheckRegisterStatus()-- %02x : Error : Corrected Data\n", StatusBlock.AtaStatus));
  }

  if ((StatusBlock.AtaStatus & ATA_STSREG_ERR) != 0) {
    if ((StatusBlock.AtaError & ATA_ERRREG_BBK) != 0) {
      DEBUG ((DEBUG_ERROR, "CheckRegisterStatus()-- %02x : Error : Bad Block Detected\n", StatusBlock.AtaError));
    }

    if ((StatusBlock.AtaError & ATA_ERRREG_UNC) != 0) {
      DEBUG ((DEBUG_ERROR, "CheckRegisterStatus()-- %02x : Error : Uncorrectable Data\n", StatusBlock.AtaError));
    }

    if ((StatusBlock.AtaError & ATA_ERRREG_MC) != 0) {
      DEBUG ((DEBUG_ERROR, "CheckRegisterStatus()-- %02x : Error : Media Change\n", StatusBlock.AtaError));
    }

    if ((StatusBlock.AtaError & ATA_ERRREG_ABRT) != 0) {
      DEBUG ((DEBUG_ERROR, "CheckRegisterStatus()-- %02x : Error : Abort\n", StatusBlock.AtaError));
    }

    if ((StatusBlock.AtaError & ATA_ERRREG_TK0NF) != 0) {
      DEBUG ((DEBUG_ERROR, "CheckRegisterStatus()-- %02x : Error : Track 0 Not Found\n", StatusBlock.AtaError));
    }

    if ((StatusBlock.AtaError & ATA_ERRREG_AMNF) != 0) {
      DEBUG ((DEBUG_ERROR, "CheckRegisterStatus()-- %02x : Error : Address Mark Not Found\n", StatusBlock.AtaError));
    }
  }

  DEBUG_CODE_END ();
}

/**
  This function is used to analyze the Status Register at the condition that BSY is zero.
  if there is ERR bit set in the Status Register, then return error.

  @param PciIo            A pointer to EFI_PCI_IO_PROTOCOL data structure.
  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.

  @retval EFI_SUCCESS       No err information in the Status Register.
  @retval EFI_DEVICE_ERROR  Any err information in the Status Register.

**/
EFI_STATUS
EFIAPI
CheckStatusRegister (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  EFI_IDE_REGISTERS    *IdeRegisters
  )
{
  UINT8  StatusRegister;

  ASSERT (PciIo != NULL);
  ASSERT (IdeRegisters != NULL);

  StatusRegister = IdeReadPortB (PciIo, IdeRegisters->CmdOrStatus);

  if ((StatusRegister & ATA_STSREG_BSY) == 0) {
    if ((StatusRegister & (ATA_STSREG_ERR | ATA_STSREG_DWF | ATA_STSREG_CORR)) == 0) {
      return EFI_SUCCESS;
    } else {
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}

/**
  This function is used to poll for the DRQ bit clear in the Status
  Register. DRQ is cleared when the device is finished transferring data.
  So this function is called after data transfer is finished.

  @param PciIo            A pointer to EFI_PCI_IO_PROTOCOL data structure.
  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param Timeout          The time to complete the command, uses 100ns as a unit.

  @retval EFI_SUCCESS     DRQ bit clear within the time out.

  @retval EFI_TIMEOUT     DRQ bit not clear within the time out.

  @note
  Read Status Register will clear interrupt status.

**/
EFI_STATUS
EFIAPI
DRQClear (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  EFI_IDE_REGISTERS    *IdeRegisters,
  IN  UINT64               Timeout
  )
{
  UINT64   Delay;
  UINT8    StatusRegister;
  BOOLEAN  InfiniteWait;

  ASSERT (PciIo != NULL);
  ASSERT (IdeRegisters != NULL);

  if (Timeout == 0) {
    InfiniteWait = TRUE;
  } else {
    InfiniteWait = FALSE;
  }

  Delay = DivU64x32 (Timeout, 1000) + 1;
  do {
    StatusRegister = IdeReadPortB (PciIo, IdeRegisters->CmdOrStatus);

    //
    // Wait for BSY == 0, then judge if DRQ is clear
    //
    if ((StatusRegister & ATA_STSREG_BSY) == 0) {
      if ((StatusRegister & ATA_STSREG_DRQ) == ATA_STSREG_DRQ) {
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
  } while (InfiniteWait || (Delay > 0));

  return EFI_TIMEOUT;
}

/**
  This function is used to poll for the DRQ bit clear in the Alternate
  Status Register. DRQ is cleared when the device is finished
  transferring data. So this function is called after data transfer
  is finished.

  @param PciIo            A pointer to EFI_PCI_IO_PROTOCOL data structure.
  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param Timeout          The time to complete the command, uses 100ns as a unit.

  @retval EFI_SUCCESS     DRQ bit clear within the time out.

  @retval EFI_TIMEOUT     DRQ bit not clear within the time out.
  @note   Read Alternate Status Register will not clear interrupt status.

**/
EFI_STATUS
EFIAPI
DRQClear2 (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  EFI_IDE_REGISTERS    *IdeRegisters,
  IN  UINT64               Timeout
  )
{
  UINT64   Delay;
  UINT8    AltRegister;
  BOOLEAN  InfiniteWait;

  ASSERT (PciIo != NULL);
  ASSERT (IdeRegisters != NULL);

  if (Timeout == 0) {
    InfiniteWait = TRUE;
  } else {
    InfiniteWait = FALSE;
  }

  Delay = DivU64x32 (Timeout, 1000) + 1;
  do {
    AltRegister = IdeReadPortB (PciIo, IdeRegisters->AltOrDev);

    //
    // Wait for BSY == 0, then judge if DRQ is clear
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
  } while (InfiniteWait || (Delay > 0));

  return EFI_TIMEOUT;
}

/**
  This function is used to poll for the DRQ bit set in the
  Status Register.
  DRQ is set when the device is ready to transfer data. So this function
  is called after the command is sent to the device and before required
  data is transferred.

  @param PciIo            A pointer to EFI_PCI_IO_PROTOCOL data structure.
  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param Timeout          The time to complete the command, uses 100ns as a unit.

  @retval EFI_SUCCESS           BSY bit cleared and DRQ bit set within the
                                timeout.

  @retval EFI_TIMEOUT           BSY bit not cleared within the timeout.

  @retval EFI_ABORTED           Polling abandoned due to command abort.

  @retval EFI_DEVICE_ERROR      Polling abandoned due to a non-abort error.

  @retval EFI_NOT_READY         BSY bit cleared within timeout, and device
                                reported "command complete" by clearing DRQ
                                bit.

  @note  Read Status Register will clear interrupt status.

**/
EFI_STATUS
EFIAPI
DRQReady (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  EFI_IDE_REGISTERS    *IdeRegisters,
  IN  UINT64               Timeout
  )
{
  UINT64   Delay;
  UINT8    StatusRegister;
  UINT8    ErrorRegister;
  BOOLEAN  InfiniteWait;

  ASSERT (PciIo != NULL);
  ASSERT (IdeRegisters != NULL);

  if (Timeout == 0) {
    InfiniteWait = TRUE;
  } else {
    InfiniteWait = FALSE;
  }

  Delay = DivU64x32 (Timeout, 1000) + 1;
  do {
    //
    // Read Status Register will clear interrupt
    //
    StatusRegister = IdeReadPortB (PciIo, IdeRegisters->CmdOrStatus);

    //
    // Wait for BSY == 0, then judge if DRQ is clear or ERR is set
    //
    if ((StatusRegister & ATA_STSREG_BSY) == 0) {
      if ((StatusRegister & ATA_STSREG_ERR) == ATA_STSREG_ERR) {
        ErrorRegister = IdeReadPortB (PciIo, IdeRegisters->ErrOrFeature);

        if ((ErrorRegister & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
          return EFI_ABORTED;
        }

        return EFI_DEVICE_ERROR;
      }

      if ((StatusRegister & ATA_STSREG_DRQ) == ATA_STSREG_DRQ) {
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
  } while (InfiniteWait || (Delay > 0));

  return EFI_TIMEOUT;
}

/**
  This function is used to poll for the DRQ bit set in the Alternate Status Register.
  DRQ is set when the device is ready to transfer data. So this function is called after
  the command is sent to the device and before required data is transferred.

  @param PciIo            A pointer to EFI_PCI_IO_PROTOCOL data structure.
  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param Timeout          The time to complete the command, uses 100ns as a unit.

  @retval EFI_SUCCESS           BSY bit cleared and DRQ bit set within the
                                timeout.

  @retval EFI_TIMEOUT           BSY bit not cleared within the timeout.

  @retval EFI_ABORTED           Polling abandoned due to command abort.

  @retval EFI_DEVICE_ERROR      Polling abandoned due to a non-abort error.

  @retval EFI_NOT_READY         BSY bit cleared within timeout, and device
                                reported "command complete" by clearing DRQ
                                bit.

  @note  Read Alternate Status Register will not clear interrupt status.

**/
EFI_STATUS
EFIAPI
DRQReady2 (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  EFI_IDE_REGISTERS    *IdeRegisters,
  IN  UINT64               Timeout
  )
{
  UINT64   Delay;
  UINT8    AltRegister;
  UINT8    ErrorRegister;
  BOOLEAN  InfiniteWait;

  ASSERT (PciIo != NULL);
  ASSERT (IdeRegisters != NULL);

  if (Timeout == 0) {
    InfiniteWait = TRUE;
  } else {
    InfiniteWait = FALSE;
  }

  Delay = DivU64x32 (Timeout, 1000) + 1;

  do {
    //
    // Read Alternate Status Register will not clear interrupt status
    //
    AltRegister = IdeReadPortB (PciIo, IdeRegisters->AltOrDev);
    //
    // Wait for BSY == 0, then judge if DRQ is clear or ERR is set
    //
    if ((AltRegister & ATA_STSREG_BSY) == 0) {
      if ((AltRegister & ATA_STSREG_ERR) == ATA_STSREG_ERR) {
        ErrorRegister = IdeReadPortB (PciIo, IdeRegisters->ErrOrFeature);

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
  } while (InfiniteWait || (Delay > 0));

  return EFI_TIMEOUT;
}

/**
  This function is used to poll for the BSY bit clear in the Status Register. BSY
  is clear when the device is not busy. Every command must be sent after device is not busy.

  @param PciIo            A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data structure.
  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param Timeout          The time to complete the command, uses 100ns as a unit.

  @retval EFI_SUCCESS          BSY bit clear within the time out.
  @retval EFI_TIMEOUT          BSY bit not clear within the time out.

  @note Read Status Register will clear interrupt status.
**/
EFI_STATUS
EFIAPI
WaitForBSYClear (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  EFI_IDE_REGISTERS    *IdeRegisters,
  IN  UINT64               Timeout
  )
{
  UINT64   Delay;
  UINT8    StatusRegister;
  BOOLEAN  InfiniteWait;

  ASSERT (PciIo != NULL);
  ASSERT (IdeRegisters != NULL);

  if (Timeout == 0) {
    InfiniteWait = TRUE;
  } else {
    InfiniteWait = FALSE;
  }

  Delay = DivU64x32 (Timeout, 1000) + 1;
  do {
    StatusRegister = IdeReadPortB (PciIo, IdeRegisters->CmdOrStatus);

    if ((StatusRegister & ATA_STSREG_BSY) == 0x00) {
      return EFI_SUCCESS;
    }

    //
    // Stall for 100 microseconds.
    //
    MicroSecondDelay (100);

    Delay--;
  } while (InfiniteWait || (Delay > 0));

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

  @param[in]      PciIo          Pointer to the EFI_PCI_IO_PROTOCOL instance
  @param[in, out] IdeRegisters   Pointer to EFI_IDE_REGISTERS which is used to
                                 store the IDE i/o port registers' base addresses

  @retval EFI_UNSUPPORTED        Return this value when the BARs is not IO type
  @retval EFI_SUCCESS            Get the Base address successfully
  @retval Other                  Read the pci configuration data error

**/
EFI_STATUS
EFIAPI
GetIdeRegisterIoAddr (
  IN     EFI_PCI_IO_PROTOCOL  *PciIo,
  IN OUT EFI_IDE_REGISTERS    *IdeRegisters
  )
{
  EFI_STATUS  Status;
  PCI_TYPE00  PciData;
  UINT16      CommandBlockBaseAddr;
  UINT16      ControlBlockBaseAddr;
  UINT16      BusMasterBaseAddr;

  if ((PciIo == NULL) || (IdeRegisters == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        0,
                        sizeof (PciData),
                        &PciData
                        );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  BusMasterBaseAddr = (UINT16)((PciData.Device.Bar[4] & 0x0000fff0));

  if ((PciData.Hdr.ClassCode[0] & IDE_PRIMARY_OPERATING_MODE) == 0) {
    CommandBlockBaseAddr = 0x1f0;
    ControlBlockBaseAddr = 0x3f6;
  } else {
    //
    // The BARs should be of IO type
    //
    if (((PciData.Device.Bar[0] & BIT0) == 0) ||
        ((PciData.Device.Bar[1] & BIT0) == 0))
    {
      return EFI_UNSUPPORTED;
    }

    CommandBlockBaseAddr = (UINT16)(PciData.Device.Bar[0] & 0x0000fff8);
    ControlBlockBaseAddr = (UINT16)((PciData.Device.Bar[1] & 0x0000fffc) + 2);
  }

  //
  // Calculate IDE primary channel I/O register base address.
  //
  IdeRegisters[EfiIdePrimary].Data              = CommandBlockBaseAddr;
  IdeRegisters[EfiIdePrimary].ErrOrFeature      = (UINT16)(CommandBlockBaseAddr + 0x01);
  IdeRegisters[EfiIdePrimary].SectorCount       = (UINT16)(CommandBlockBaseAddr + 0x02);
  IdeRegisters[EfiIdePrimary].SectorNumber      = (UINT16)(CommandBlockBaseAddr + 0x03);
  IdeRegisters[EfiIdePrimary].CylinderLsb       = (UINT16)(CommandBlockBaseAddr + 0x04);
  IdeRegisters[EfiIdePrimary].CylinderMsb       = (UINT16)(CommandBlockBaseAddr + 0x05);
  IdeRegisters[EfiIdePrimary].Head              = (UINT16)(CommandBlockBaseAddr + 0x06);
  IdeRegisters[EfiIdePrimary].CmdOrStatus       = (UINT16)(CommandBlockBaseAddr + 0x07);
  IdeRegisters[EfiIdePrimary].AltOrDev          = ControlBlockBaseAddr;
  IdeRegisters[EfiIdePrimary].BusMasterBaseAddr = BusMasterBaseAddr;

  if ((PciData.Hdr.ClassCode[0] & IDE_SECONDARY_OPERATING_MODE) == 0) {
    CommandBlockBaseAddr = 0x170;
    ControlBlockBaseAddr = 0x376;
  } else {
    //
    // The BARs should be of IO type
    //
    if (((PciData.Device.Bar[2] & BIT0) == 0) ||
        ((PciData.Device.Bar[3] & BIT0) == 0))
    {
      return EFI_UNSUPPORTED;
    }

    CommandBlockBaseAddr = (UINT16)(PciData.Device.Bar[2] & 0x0000fff8);
    ControlBlockBaseAddr = (UINT16)((PciData.Device.Bar[3] & 0x0000fffc) + 2);
  }

  //
  // Calculate IDE secondary channel I/O register base address.
  //
  IdeRegisters[EfiIdeSecondary].Data              = CommandBlockBaseAddr;
  IdeRegisters[EfiIdeSecondary].ErrOrFeature      = (UINT16)(CommandBlockBaseAddr + 0x01);
  IdeRegisters[EfiIdeSecondary].SectorCount       = (UINT16)(CommandBlockBaseAddr + 0x02);
  IdeRegisters[EfiIdeSecondary].SectorNumber      = (UINT16)(CommandBlockBaseAddr + 0x03);
  IdeRegisters[EfiIdeSecondary].CylinderLsb       = (UINT16)(CommandBlockBaseAddr + 0x04);
  IdeRegisters[EfiIdeSecondary].CylinderMsb       = (UINT16)(CommandBlockBaseAddr + 0x05);
  IdeRegisters[EfiIdeSecondary].Head              = (UINT16)(CommandBlockBaseAddr + 0x06);
  IdeRegisters[EfiIdeSecondary].CmdOrStatus       = (UINT16)(CommandBlockBaseAddr + 0x07);
  IdeRegisters[EfiIdeSecondary].AltOrDev          = ControlBlockBaseAddr;
  IdeRegisters[EfiIdeSecondary].BusMasterBaseAddr = (UINT16)(BusMasterBaseAddr + 0x8);

  return EFI_SUCCESS;
}

/**
  Send ATA Ext command into device with NON_DATA protocol.

  @param PciIo            A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data structure.
  @param IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param AtaCommandBlock  A pointer to EFI_ATA_COMMAND_BLOCK data structure.
  @param Timeout          The time to complete the command, uses 100ns as a unit.

  @retval  EFI_SUCCESS Reading succeed
  @retval  EFI_DEVICE_ERROR Error executing commands on this device.

**/
EFI_STATUS
EFIAPI
AtaIssueCommand (
  IN  EFI_PCI_IO_PROTOCOL    *PciIo,
  IN  EFI_IDE_REGISTERS      *IdeRegisters,
  IN  EFI_ATA_COMMAND_BLOCK  *AtaCommandBlock,
  IN  UINT64                 Timeout
  )
{
  EFI_STATUS  Status;
  UINT8       DeviceHead;
  UINT8       AtaCommand;

  ASSERT (PciIo != NULL);
  ASSERT (IdeRegisters != NULL);
  ASSERT (AtaCommandBlock != NULL);

  DeviceHead = AtaCommandBlock->AtaDeviceHead;
  AtaCommand = AtaCommandBlock->AtaCommand;

  Status = WaitForBSYClear (PciIo, IdeRegisters, Timeout);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Select device (bit4), set LBA mode(bit6) (use 0xe0 for compatibility)
  //
  IdeWritePortB (PciIo, IdeRegisters->Head, (UINT8)(0xe0 | DeviceHead));

  //
  // set all the command parameters
  // Before write to all the following registers, BSY and DRQ must be 0.
  //
  Status = DRQClear2 (PciIo, IdeRegisters, Timeout);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Fill the feature register, which is a two-byte FIFO. Need write twice.
  //
  IdeWritePortB (PciIo, IdeRegisters->ErrOrFeature, AtaCommandBlock->AtaFeaturesExp);
  IdeWritePortB (PciIo, IdeRegisters->ErrOrFeature, AtaCommandBlock->AtaFeatures);

  //
  // Fill the sector count register, which is a two-byte FIFO. Need write twice.
  //
  IdeWritePortB (PciIo, IdeRegisters->SectorCount, AtaCommandBlock->AtaSectorCountExp);
  IdeWritePortB (PciIo, IdeRegisters->SectorCount, AtaCommandBlock->AtaSectorCount);

  //
  // Fill the start LBA registers, which are also two-byte FIFO
  //
  IdeWritePortB (PciIo, IdeRegisters->SectorNumber, AtaCommandBlock->AtaSectorNumberExp);
  IdeWritePortB (PciIo, IdeRegisters->SectorNumber, AtaCommandBlock->AtaSectorNumber);

  IdeWritePortB (PciIo, IdeRegisters->CylinderLsb, AtaCommandBlock->AtaCylinderLowExp);
  IdeWritePortB (PciIo, IdeRegisters->CylinderLsb, AtaCommandBlock->AtaCylinderLow);

  IdeWritePortB (PciIo, IdeRegisters->CylinderMsb, AtaCommandBlock->AtaCylinderHighExp);
  IdeWritePortB (PciIo, IdeRegisters->CylinderMsb, AtaCommandBlock->AtaCylinderHigh);

  //
  // Send command via Command Register
  //
  IdeWritePortB (PciIo, IdeRegisters->CmdOrStatus, AtaCommand);

  //
  // Stall at least 400 microseconds.
  //
  MicroSecondDelay (400);

  return EFI_SUCCESS;
}

/**
  This function is used to send out ATA commands conforms to the PIO Data In Protocol.

  @param[in]      PciIo            A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data
                                   structure.
  @param[in]      IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param[in, out] Buffer           A pointer to the source buffer for the data.
  @param[in]      ByteCount        The length of the data.
  @param[in]      Read             Flag used to determine the data transfer direction.
                                   Read equals 1, means data transferred from device
                                   to host;Read equals 0, means data transferred
                                   from host to device.
  @param[in]      AtaCommandBlock  A pointer to EFI_ATA_COMMAND_BLOCK data structure.
  @param[in, out] AtaStatusBlock   A pointer to EFI_ATA_STATUS_BLOCK data structure.
  @param[in]      Timeout          The time to complete the command, uses 100ns as a unit.
  @param[in]      Task             Optional. Pointer to the ATA_NONBLOCK_TASK
                                   used by non-blocking mode.

  @retval EFI_SUCCESS      send out the ATA command and device send required data successfully.
  @retval EFI_DEVICE_ERROR command sent failed.

**/
EFI_STATUS
EFIAPI
AtaPioDataInOut (
  IN     EFI_PCI_IO_PROTOCOL    *PciIo,
  IN     EFI_IDE_REGISTERS      *IdeRegisters,
  IN OUT VOID                   *Buffer,
  IN     UINT64                 ByteCount,
  IN     BOOLEAN                Read,
  IN     EFI_ATA_COMMAND_BLOCK  *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK   *AtaStatusBlock,
  IN     UINT64                 Timeout,
  IN     ATA_NONBLOCK_TASK      *Task
  )
{
  UINT64      WordCount;
  UINTN       Increment;
  UINT16      *Buffer16;
  EFI_STATUS  Status;

  if ((PciIo == NULL) || (IdeRegisters == NULL) || (Buffer == NULL) || (AtaCommandBlock == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Issue ATA command
  //
  Status = AtaIssueCommand (PciIo, IdeRegisters, AtaCommandBlock, Timeout);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  Buffer16 = (UINT16 *)Buffer;

  //
  // According to PIO data in protocol, host can perform a series of reads to
  // the data register after each time device set DRQ ready;
  // The data size of "a series of read" is command specific.
  // For most ATA command, data size received from device will not exceed
  // 1 sector, hence the data size for "a series of read" can be the whole data
  // size of one command request.
  // For ATA command such as Read Sector command, the data size of one ATA
  // command request is often larger than 1 sector, according to the
  // Read Sector command, the data size of "a series of read" is exactly 1
  // sector.
  // Here for simplification reason, we specify the data size for
  // "a series of read" to 1 sector (256 words) if data size of one ATA command
  // request is larger than 256 words.
  //
  Increment = 256;

  //
  // used to record bytes of currently transferred data
  //
  WordCount = 0;

  while (WordCount < RShiftU64 (ByteCount, 1)) {
    //
    // Poll DRQ bit set, data transfer can be performed only when DRQ is ready
    //
    Status = DRQReady2 (PciIo, IdeRegisters, Timeout);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    //
    // Get the byte count for one series of read
    //
    if ((WordCount + Increment) > RShiftU64 (ByteCount, 1)) {
      Increment = (UINTN)(RShiftU64 (ByteCount, 1) - WordCount);
    }

    if (Read) {
      IdeReadPortWMultiple (
        PciIo,
        IdeRegisters->Data,
        Increment,
        Buffer16
        );
    } else {
      IdeWritePortWMultiple (
        PciIo,
        IdeRegisters->Data,
        Increment,
        Buffer16
        );
    }

    Status = CheckStatusRegister (PciIo, IdeRegisters);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    WordCount += Increment;
    Buffer16  += Increment;
  }

  Status = DRQClear (PciIo, IdeRegisters, Timeout);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

Exit:
  //
  // Dump All Ide registers to ATA_STATUS_BLOCK
  //
  DumpAllIdeRegisters (PciIo, IdeRegisters, AtaStatusBlock);

  //
  // Not support the Non-blocking now,just do the blocking process.
  //
  return Status;
}

/**
  Send ATA command into device with NON_DATA protocol

  @param[in]      PciIo            A pointer to ATA_ATAPI_PASS_THRU_INSTANCE
                                   data structure.
  @param[in]      IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param[in]      AtaCommandBlock  A pointer to EFI_ATA_COMMAND_BLOCK data
                                   structure.
  @param[in, out] AtaStatusBlock   A pointer to EFI_ATA_STATUS_BLOCK data structure.
  @param[in]      Timeout          The time to complete the command, uses 100ns as a unit.
  @param[in]      Task             Optional. Pointer to the ATA_NONBLOCK_TASK
                                   used by non-blocking mode.

  @retval  EFI_SUCCESS Reading succeed
  @retval  EFI_ABORTED Command failed
  @retval  EFI_DEVICE_ERROR Device status error.

**/
EFI_STATUS
EFIAPI
AtaNonDataCommandIn (
  IN     EFI_PCI_IO_PROTOCOL    *PciIo,
  IN     EFI_IDE_REGISTERS      *IdeRegisters,
  IN     EFI_ATA_COMMAND_BLOCK  *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK   *AtaStatusBlock,
  IN     UINT64                 Timeout,
  IN     ATA_NONBLOCK_TASK      *Task
  )
{
  EFI_STATUS  Status;

  if ((PciIo == NULL) || (IdeRegisters == NULL) || (AtaCommandBlock == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Issue ATA command
  //
  Status = AtaIssueCommand (PciIo, IdeRegisters, AtaCommandBlock, Timeout);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  //
  // Wait for command completion
  //
  Status = WaitForBSYClear (PciIo, IdeRegisters, Timeout);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  Status = CheckStatusRegister (PciIo, IdeRegisters);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

Exit:
  //
  // Dump All Ide registers to ATA_STATUS_BLOCK
  //
  DumpAllIdeRegisters (PciIo, IdeRegisters, AtaStatusBlock);

  //
  // Not support the Non-blocking now,just do the blocking process.
  //
  return Status;
}

/**
  Wait for memory to be set.

  @param[in]  PciIo           The PCI IO protocol instance.
  @param[in]  IdeRegisters    A pointer to EFI_IDE_REGISTERS data structure.
  @param[in]  Timeout         The time to complete the command, uses 100ns as a unit.

  @retval EFI_DEVICE_ERROR  The memory is not set.
  @retval EFI_TIMEOUT       The memory setting is time out.
  @retval EFI_SUCCESS       The memory is correct set.

**/
EFI_STATUS
AtaUdmStatusWait (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  EFI_IDE_REGISTERS    *IdeRegisters,
  IN  UINT64               Timeout
  )
{
  UINT8       RegisterValue;
  EFI_STATUS  Status;
  UINT16      IoPortForBmis;
  UINT64      Delay;
  BOOLEAN     InfiniteWait;

  if (Timeout == 0) {
    InfiniteWait = TRUE;
  } else {
    InfiniteWait = FALSE;
  }

  Delay = DivU64x32 (Timeout, 1000) + 1;

  do {
    Status = CheckStatusRegister (PciIo, IdeRegisters);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      break;
    }

    IoPortForBmis = (UINT16)(IdeRegisters->BusMasterBaseAddr + BMIS_OFFSET);
    RegisterValue = IdeReadPortB (PciIo, IoPortForBmis);
    if (((RegisterValue & BMIS_ERROR) != 0) || (Timeout == 0)) {
      DEBUG ((DEBUG_ERROR, "ATA UDMA operation fails\n"));
      Status = EFI_DEVICE_ERROR;
      break;
    }

    if ((RegisterValue & BMIS_INTERRUPT) != 0) {
      Status = EFI_SUCCESS;
      break;
    }

    //
    // Stall for 100 microseconds.
    //
    MicroSecondDelay (100);
    Delay--;
  } while (InfiniteWait || (Delay > 0));

  return Status;
}

/**
  Check if the memory to be set.

  @param[in]  PciIo           The PCI IO protocol instance.
  @param[in]  Task            Optional. Pointer to the ATA_NONBLOCK_TASK
                              used by non-blocking mode.
  @param[in]  IdeRegisters    A pointer to EFI_IDE_REGISTERS data structure.

  @retval EFI_DEVICE_ERROR  The memory setting met a issue.
  @retval EFI_NOT_READY     The memory is not set.
  @retval EFI_TIMEOUT       The memory setting is time out.
  @retval EFI_SUCCESS       The memory is correct set.

**/
EFI_STATUS
AtaUdmStatusCheck (
  IN     EFI_PCI_IO_PROTOCOL  *PciIo,
  IN     ATA_NONBLOCK_TASK    *Task,
  IN     EFI_IDE_REGISTERS    *IdeRegisters
  )
{
  UINT8       RegisterValue;
  UINT16      IoPortForBmis;
  EFI_STATUS  Status;

  Task->RetryTimes--;

  Status = CheckStatusRegister (PciIo, IdeRegisters);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  IoPortForBmis = (UINT16)(IdeRegisters->BusMasterBaseAddr + BMIS_OFFSET);
  RegisterValue = IdeReadPortB (PciIo, IoPortForBmis);

  if ((RegisterValue & BMIS_ERROR) != 0) {
    DEBUG ((DEBUG_ERROR, "ATA UDMA operation fails\n"));
    return EFI_DEVICE_ERROR;
  }

  if ((RegisterValue & BMIS_INTERRUPT) != 0) {
    return EFI_SUCCESS;
  }

  if (!Task->InfiniteWait && (Task->RetryTimes == 0)) {
    return EFI_TIMEOUT;
  } else {
    //
    // The memory is not set.
    //
    return EFI_NOT_READY;
  }
}

/**
  Perform an ATA Udma operation (Read, ReadExt, Write, WriteExt).

  @param[in]      Instance         A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data
                                   structure.
  @param[in]      IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param[in]      Read             Flag used to determine the data transfer
                                   direction. Read equals 1, means data transferred
                                   from device to host;Read equals 0, means data
                                   transferred from host to device.
  @param[in]      DataBuffer       A pointer to the source buffer for the data.
  @param[in]      DataLength       The length of  the data.
  @param[in]      AtaCommandBlock  A pointer to EFI_ATA_COMMAND_BLOCK data structure.
  @param[in, out] AtaStatusBlock   A pointer to EFI_ATA_STATUS_BLOCK data structure.
  @param[in]      Timeout          The time to complete the command, uses 100ns as a unit.
  @param[in]      Task             Optional. Pointer to the ATA_NONBLOCK_TASK
                                   used by non-blocking mode.

  @retval EFI_SUCCESS          the operation is successful.
  @retval EFI_OUT_OF_RESOURCES Build PRD table failed
  @retval EFI_UNSUPPORTED      Unknown channel or operations command
  @retval EFI_DEVICE_ERROR     Ata command execute failed

**/
EFI_STATUS
EFIAPI
AtaUdmaInOut (
  IN     ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN     EFI_IDE_REGISTERS             *IdeRegisters,
  IN     BOOLEAN                       Read,
  IN     VOID                          *DataBuffer,
  IN     UINT64                        DataLength,
  IN     EFI_ATA_COMMAND_BLOCK         *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock,
  IN     UINT64                        Timeout,
  IN     ATA_NONBLOCK_TASK             *Task
  )
{
  EFI_STATUS  Status;
  UINT16      IoPortForBmic;
  UINT16      IoPortForBmis;
  UINT16      IoPortForBmid;

  UINTN                 PrdTableSize;
  EFI_PHYSICAL_ADDRESS  PrdTableMapAddr;
  VOID                  *PrdTableMap;
  EFI_PHYSICAL_ADDRESS  PrdTableBaseAddr;
  EFI_ATA_DMA_PRD       *TempPrdBaseAddr;
  UINTN                 PrdTableNum;

  UINT8  RegisterValue;
  UINTN  PageCount;
  UINTN  ByteCount;
  UINTN  ByteRemaining;
  UINT8  DeviceControl;

  VOID                           *BufferMap;
  EFI_PHYSICAL_ADDRESS           BufferMapAddress;
  EFI_PCI_IO_PROTOCOL_OPERATION  PciIoOperation;

  UINT8                DeviceHead;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  EFI_TPL              OldTpl;

  UINTN                 AlignmentMask;
  UINTN                 RealPageCount;
  EFI_PHYSICAL_ADDRESS  BaseAddr;
  EFI_PHYSICAL_ADDRESS  BaseMapAddr;

  Status        = EFI_SUCCESS;
  PrdTableMap   = NULL;
  BufferMap     = NULL;
  PageCount     = 0;
  RealPageCount = 0;
  BaseAddr      = 0;
  PciIo         = Instance->PciIo;

  if ((PciIo == NULL) || (IdeRegisters == NULL) || (DataBuffer == NULL) || (AtaCommandBlock == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Before starting the Blocking BlockIO operation, push to finish all non-blocking
  // BlockIO tasks.
  // Delay 1ms to simulate the blocking time out checking.
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  while ((Task == NULL) && (!IsListEmpty (&Instance->NonBlockingTaskList))) {
    AsyncNonBlockingTransferRoutine (NULL, Instance);
    //
    // Stall for 1 milliseconds.
    //
    MicroSecondDelay (1000);
  }

  gBS->RestoreTPL (OldTpl);

  //
  // The data buffer should be even alignment
  //
  if (((UINTN)DataBuffer & 0x1) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Set relevant IO Port address.
  //
  IoPortForBmic = (UINT16)(IdeRegisters->BusMasterBaseAddr + BMIC_OFFSET);
  IoPortForBmis = (UINT16)(IdeRegisters->BusMasterBaseAddr + BMIS_OFFSET);
  IoPortForBmid = (UINT16)(IdeRegisters->BusMasterBaseAddr + BMID_OFFSET);

  //
  // For Blocking mode, start the command.
  // For non-blocking mode, when the command is not started, start it, otherwise
  // go to check the status.
  //
  if (((Task != NULL) && (!Task->IsStart)) || (Task == NULL)) {
    //
    // Calculate the number of PRD entry.
    // Every entry in PRD table can specify a 64K memory region.
    //
    PrdTableNum = (UINTN)(RShiftU64 (DataLength, 16) + 1);

    //
    // Make sure that the memory region of PRD table is not cross 64K boundary
    //
    PrdTableSize = PrdTableNum * sizeof (EFI_ATA_DMA_PRD);
    if (PrdTableSize > 0x10000) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Allocate buffer for PRD table initialization.
    // Note Ide Bus Master spec said the descriptor table must be aligned on a 4 byte
    // boundary and the table cannot cross a 64K boundary in memory.
    //
    PageCount     = EFI_SIZE_TO_PAGES (PrdTableSize);
    RealPageCount = PageCount + EFI_SIZE_TO_PAGES (SIZE_64KB);

    //
    // Make sure that PageCount plus EFI_SIZE_TO_PAGES (SIZE_64KB) does not overflow.
    //
    ASSERT (RealPageCount > PageCount);

    Status = PciIo->AllocateBuffer (
                      PciIo,
                      AllocateAnyPages,
                      EfiBootServicesData,
                      RealPageCount,
                      (VOID **)&BaseAddr,
                      0
                      );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }

    ByteCount = EFI_PAGES_TO_SIZE (RealPageCount);
    Status    = PciIo->Map (
                         PciIo,
                         EfiPciIoOperationBusMasterCommonBuffer,
                         (VOID *)(UINTN)BaseAddr,
                         &ByteCount,
                         &BaseMapAddr,
                         &PrdTableMap
                         );
    if (EFI_ERROR (Status) || (ByteCount != EFI_PAGES_TO_SIZE (RealPageCount))) {
      //
      // If the data length actually mapped is not equal to the requested amount,
      // it means the DMA operation may be broken into several discontinuous smaller chunks.
      // Can't handle this case.
      //
      PciIo->FreeBuffer (PciIo, RealPageCount, (VOID *)(UINTN)BaseAddr);
      return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem ((VOID *)((UINTN)BaseAddr), ByteCount);

    //
    // Calculate the 64K align address as PRD Table base address.
    //
    AlignmentMask    = SIZE_64KB - 1;
    PrdTableBaseAddr = ((UINTN)BaseAddr + AlignmentMask) & ~AlignmentMask;
    PrdTableMapAddr  = ((UINTN)BaseMapAddr + AlignmentMask) & ~AlignmentMask;

    //
    // Map the host address of DataBuffer to DMA master address.
    //
    if (Read) {
      PciIoOperation = EfiPciIoOperationBusMasterWrite;
    } else {
      PciIoOperation = EfiPciIoOperationBusMasterRead;
    }

    ByteCount = (UINTN)DataLength;
    Status    = PciIo->Map (
                         PciIo,
                         PciIoOperation,
                         DataBuffer,
                         &ByteCount,
                         &BufferMapAddress,
                         &BufferMap
                         );
    if (EFI_ERROR (Status) || (ByteCount != DataLength)) {
      PciIo->Unmap (PciIo, PrdTableMap);
      PciIo->FreeBuffer (PciIo, RealPageCount, (VOID *)(UINTN)BaseAddr);
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // According to Ata spec, it requires the buffer address and size to be even.
    //
    ASSERT ((BufferMapAddress & 0x1) == 0);
    ASSERT ((ByteCount & 0x1) == 0);

    //
    // Fill the PRD table with appropriate bus master address of data buffer and data length.
    //
    ByteRemaining   = ByteCount;
    TempPrdBaseAddr = (EFI_ATA_DMA_PRD *)(UINTN)PrdTableBaseAddr;
    while (ByteRemaining != 0) {
      if (ByteRemaining <= 0x10000) {
        TempPrdBaseAddr->RegionBaseAddr = (UINT32)((UINTN)BufferMapAddress);
        TempPrdBaseAddr->ByteCount      = (UINT16)ByteRemaining;
        TempPrdBaseAddr->EndOfTable     = 0x8000;
        break;
      }

      TempPrdBaseAddr->RegionBaseAddr = (UINT32)((UINTN)BufferMapAddress);
      TempPrdBaseAddr->ByteCount      = (UINT16)0x0;

      ByteRemaining    -= 0x10000;
      BufferMapAddress += 0x10000;
      TempPrdBaseAddr++;
    }

    //
    // Start to enable the DMA operation
    //
    DeviceHead = AtaCommandBlock->AtaDeviceHead;

    IdeWritePortB (PciIo, IdeRegisters->Head, (UINT8)(0xe0 | DeviceHead));

    //
    // Enable interrupt to support UDMA
    //
    DeviceControl = 0;
    IdeWritePortB (PciIo, IdeRegisters->AltOrDev, DeviceControl);

    //
    // Read BMIS register and clear ERROR and INTR bit
    //
    RegisterValue  = IdeReadPortB (PciIo, IoPortForBmis);
    RegisterValue |= (BMIS_INTERRUPT | BMIS_ERROR);
    IdeWritePortB (PciIo, IoPortForBmis, RegisterValue);

    //
    // Set the base address to BMID register
    //
    IdeWritePortDW (PciIo, IoPortForBmid, (UINT32)PrdTableMapAddr);

    //
    // Set BMIC register to identify the operation direction
    //
    RegisterValue = IdeReadPortB (PciIo, IoPortForBmic);
    if (Read) {
      RegisterValue |= BMIC_NREAD;
    } else {
      RegisterValue &= ~((UINT8)BMIC_NREAD);
    }

    IdeWritePortB (PciIo, IoPortForBmic, RegisterValue);

    if (Task != NULL) {
      Task->Map            = BufferMap;
      Task->TableMap       = PrdTableMap;
      Task->MapBaseAddress = (EFI_ATA_DMA_PRD *)(UINTN)BaseAddr;
      Task->PageCount      = RealPageCount;
      Task->IsStart        = TRUE;
    }

    //
    // Issue ATA command
    //
    Status = AtaIssueCommand (PciIo, IdeRegisters, AtaCommandBlock, Timeout);

    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    Status = CheckStatusRegister (PciIo, IdeRegisters);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    //
    // Set START bit of BMIC register
    //
    RegisterValue  = IdeReadPortB (PciIo, IoPortForBmic);
    RegisterValue |= BMIC_START;
    IdeWritePortB (PciIo, IoPortForBmic, RegisterValue);
  }

  //
  // Check the INTERRUPT and ERROR bit of BMIS
  //
  if (Task != NULL) {
    Status = AtaUdmStatusCheck (PciIo, Task, IdeRegisters);
  } else {
    Status = AtaUdmStatusWait (PciIo, IdeRegisters, Timeout);
  }

  //
  // For blocking mode, clear registers and free buffers.
  // For non blocking mode, when the related registers have been set or time
  // out, or a error has been happened, it needs to clear the register and free
  // buffer.
  //
  if ((Task == NULL) || (Status != EFI_NOT_READY)) {
    //
    // Read BMIS register and clear ERROR and INTR bit
    //
    RegisterValue  = IdeReadPortB (PciIo, IoPortForBmis);
    RegisterValue |= (BMIS_INTERRUPT | BMIS_ERROR);
    IdeWritePortB (PciIo, IoPortForBmis, RegisterValue);

    //
    // Read Status Register of IDE device to clear interrupt
    //
    RegisterValue = IdeReadPortB (PciIo, IdeRegisters->CmdOrStatus);

    //
    // Clear START bit of BMIC register
    //
    RegisterValue  = IdeReadPortB (PciIo, IoPortForBmic);
    RegisterValue &= ~((UINT8)BMIC_START);
    IdeWritePortB (PciIo, IoPortForBmic, RegisterValue);

    //
    // Disable interrupt of Select device
    //
    DeviceControl  = IdeReadPortB (PciIo, IdeRegisters->AltOrDev);
    DeviceControl |= ATA_CTLREG_IEN_L;
    IdeWritePortB (PciIo, IdeRegisters->AltOrDev, DeviceControl);
    //
    // Stall for 10 milliseconds.
    //
    MicroSecondDelay (10000);
  }

Exit:
  //
  // Free all allocated resource
  //
  if ((Task == NULL) || (Status != EFI_NOT_READY)) {
    if (Task != NULL) {
      PciIo->Unmap (PciIo, Task->TableMap);
      PciIo->FreeBuffer (PciIo, Task->PageCount, Task->MapBaseAddress);
      PciIo->Unmap (PciIo, Task->Map);
    } else {
      PciIo->Unmap (PciIo, PrdTableMap);
      PciIo->FreeBuffer (PciIo, RealPageCount, (VOID *)(UINTN)BaseAddr);
      PciIo->Unmap (PciIo, BufferMap);
    }

    //
    // Dump All Ide registers to ATA_STATUS_BLOCK
    //
    DumpAllIdeRegisters (PciIo, IdeRegisters, AtaStatusBlock);
  }

  return Status;
}

/**
  This function reads the pending data in the device.

  @param PciIo         A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data structure.
  @param IdeRegisters  A pointer to EFI_IDE_REGISTERS data structure.

  @retval EFI_SUCCESS   Successfully read.
  @retval EFI_NOT_READY The BSY is set avoiding reading.

**/
EFI_STATUS
EFIAPI
AtaPacketReadPendingData (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  EFI_IDE_REGISTERS    *IdeRegisters
  )
{
  UINT8   AltRegister;
  UINT16  TempWordBuffer;

  AltRegister = IdeReadPortB (PciIo, IdeRegisters->AltOrDev);
  if ((AltRegister & ATA_STSREG_BSY) == ATA_STSREG_BSY) {
    return EFI_NOT_READY;
  }

  if ((AltRegister & (ATA_STSREG_BSY | ATA_STSREG_DRQ)) == ATA_STSREG_DRQ) {
    TempWordBuffer = IdeReadPortB (PciIo, IdeRegisters->AltOrDev);
    while ((TempWordBuffer & (ATA_STSREG_BSY | ATA_STSREG_DRQ)) == ATA_STSREG_DRQ) {
      IdeReadPortWMultiple (
        PciIo,
        IdeRegisters->Data,
        1,
        &TempWordBuffer
        );
      TempWordBuffer = IdeReadPortB (PciIo, IdeRegisters->AltOrDev);
    }
  }

  return EFI_SUCCESS;
}

/**
  This function is called by AtaPacketCommandExecute().
  It is used to transfer data between host and device. The data direction is specified
  by the fourth parameter.

  @param PciIo         A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data structure.
  @param IdeRegisters  A pointer to EFI_IDE_REGISTERS data structure.
  @param Buffer        Buffer contained data transferred between host and device.
  @param ByteCount     Data size in byte unit of the buffer.
  @param Read          Flag used to determine the data transfer direction.
                       Read equals 1, means data transferred from device to host;
                       Read equals 0, means data transferred from host to device.
  @param Timeout       Timeout value for wait DRQ ready before each data stream's transfer
                       , uses 100ns as a unit.

  @retval EFI_SUCCESS      data is transferred successfully.
  @retval EFI_DEVICE_ERROR the device failed to transfer data.
**/
EFI_STATUS
EFIAPI
AtaPacketReadWrite (
  IN     EFI_PCI_IO_PROTOCOL  *PciIo,
  IN     EFI_IDE_REGISTERS    *IdeRegisters,
  IN OUT VOID                 *Buffer,
  IN OUT UINT32               *ByteCount,
  IN     BOOLEAN              Read,
  IN     UINT64               Timeout
  )
{
  UINT32      RequiredWordCount;
  UINT32      ActualWordCount;
  UINT32      WordCount;
  EFI_STATUS  Status;
  UINT16      *PtrBuffer;

  PtrBuffer         = Buffer;
  RequiredWordCount = *ByteCount >> 1;

  //
  // No data transfer is permitted.
  //
  if (RequiredWordCount == 0) {
    return EFI_SUCCESS;
  }

  //
  // ActualWordCount means the word count of data really transferred.
  //
  ActualWordCount = 0;

  while (ActualWordCount < RequiredWordCount) {
    //
    // before each data transfer stream, the host should poll DRQ bit ready,
    // to see whether indicates device is ready to transfer data.
    //
    Status = DRQReady2 (PciIo, IdeRegisters, Timeout);
    if (EFI_ERROR (Status)) {
      if (Status == EFI_NOT_READY) {
        //
        // Device provided less data than we intended to read, or wanted less
        // data than we intended to write, but it may still be successful.
        //
        break;
      } else {
        return Status;
      }
    }

    //
    // get current data transfer size from Cylinder Registers.
    //
    WordCount  = IdeReadPortB (PciIo, IdeRegisters->CylinderMsb) << 8;
    WordCount  = WordCount | IdeReadPortB (PciIo, IdeRegisters->CylinderLsb);
    WordCount  = WordCount & 0xffff;
    WordCount /= 2;

    WordCount = MIN (WordCount, (RequiredWordCount - ActualWordCount));

    if (Read) {
      IdeReadPortWMultiple (
        PciIo,
        IdeRegisters->Data,
        WordCount,
        PtrBuffer
        );
    } else {
      IdeWritePortWMultiple (
        PciIo,
        IdeRegisters->Data,
        WordCount,
        PtrBuffer
        );
    }

    //
    // read status register to check whether error happens.
    //
    Status = CheckStatusRegister (PciIo, IdeRegisters);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    PtrBuffer       += WordCount;
    ActualWordCount += WordCount;
  }

  if (Read) {
    //
    // In the case where the drive wants to send more data than we need to read,
    // the DRQ bit will be set and cause delays from DRQClear2().
    // We need to read data from the drive until it clears DRQ so we can move on.
    //
    AtaPacketReadPendingData (PciIo, IdeRegisters);
  }

  //
  // read status register to check whether error happens.
  //
  Status = CheckStatusRegister (PciIo, IdeRegisters);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // After data transfer is completed, normally, DRQ bit should clear.
  //
  Status = DRQClear (PciIo, IdeRegisters, Timeout);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  *ByteCount = ActualWordCount << 1;
  return Status;
}

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
  IN  EFI_PCI_IO_PROTOCOL                         *PciIo,
  IN  EFI_IDE_REGISTERS                           *IdeRegisters,
  IN  UINT8                                       Channel,
  IN  UINT8                                       Device,
  IN  EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *Packet
  )
{
  EFI_ATA_COMMAND_BLOCK  AtaCommandBlock;
  EFI_STATUS             Status;
  UINT8                  Count;
  UINT8                  PacketCommand[12];

  ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));

  //
  // Fill ATAPI Command Packet according to CDB.
  // For Atapi cmd, its length should be less than or equal to 12 bytes.
  //
  if (Packet->CdbLength > 12) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (PacketCommand, 12);
  CopyMem (PacketCommand, Packet->Cdb, Packet->CdbLength);

  //
  // No OVL; No DMA
  //
  AtaCommandBlock.AtaFeatures = 0x00;
  //
  // set the transfersize to ATAPI_MAX_BYTE_COUNT to let the device
  // determine how many data should be transferred.
  //
  AtaCommandBlock.AtaCylinderLow  = (UINT8)(ATAPI_MAX_BYTE_COUNT & 0x00ff);
  AtaCommandBlock.AtaCylinderHigh = (UINT8)(ATAPI_MAX_BYTE_COUNT >> 8);
  AtaCommandBlock.AtaDeviceHead   = (UINT8)(Device << 0x4);
  AtaCommandBlock.AtaCommand      = ATA_CMD_PACKET;

  IdeWritePortB (PciIo, IdeRegisters->Head, (UINT8)(0xe0 | (Device << 0x4)));
  //
  //  Disable interrupt
  //
  IdeWritePortB (PciIo, IdeRegisters->AltOrDev, ATA_DEFAULT_CTL);

  //
  // Issue ATA PACKET command firstly
  //
  Status = AtaIssueCommand (PciIo, IdeRegisters, &AtaCommandBlock, Packet->Timeout);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DRQReady (PciIo, IdeRegisters, Packet->Timeout);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Send out ATAPI command packet
  //
  for (Count = 0; Count < 6; Count++) {
    IdeWritePortW (PciIo, IdeRegisters->Data, *((UINT16 *)PacketCommand + Count));
    //
    // Stall for 10 microseconds.
    //
    MicroSecondDelay (10);
  }

  //
  // Read/Write the data of ATAPI Command
  //
  if (Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_READ) {
    Status = AtaPacketReadWrite (
               PciIo,
               IdeRegisters,
               Packet->InDataBuffer,
               &Packet->InTransferLength,
               TRUE,
               Packet->Timeout
               );
  } else {
    Status = AtaPacketReadWrite (
               PciIo,
               IdeRegisters,
               Packet->OutDataBuffer,
               &Packet->OutTransferLength,
               FALSE,
               Packet->Timeout
               );
  }

  return Status;
}

/**
  Set the calculated Best transfer mode to a detected device.

  @param Instance               A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data structure.
  @param Channel                The channel number of device.
  @param Device                 The device number of device.
  @param TransferMode           A pointer to EFI_ATA_TRANSFER_MODE data structure.
  @param AtaStatusBlock         A pointer to EFI_ATA_STATUS_BLOCK data structure.

  @retval EFI_SUCCESS          Set transfer mode successfully.
  @retval EFI_DEVICE_ERROR     Set transfer mode failed.
  @retval EFI_OUT_OF_RESOURCES Allocate memory failed.

**/
EFI_STATUS
EFIAPI
SetDeviceTransferMode (
  IN     ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN     UINT8                         Channel,
  IN     UINT8                         Device,
  IN     EFI_ATA_TRANSFER_MODE         *TransferMode,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock
  )
{
  EFI_STATUS             Status;
  EFI_ATA_COMMAND_BLOCK  AtaCommandBlock;

  ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));

  AtaCommandBlock.AtaCommand     = ATA_CMD_SET_FEATURES;
  AtaCommandBlock.AtaDeviceHead  = (UINT8)(Device << 0x4);
  AtaCommandBlock.AtaFeatures    = 0x03;
  AtaCommandBlock.AtaSectorCount = *((UINT8 *)TransferMode);

  //
  // Send SET FEATURE command (sub command 0x03) to set pio mode.
  //
  Status = AtaNonDataCommandIn (
             Instance->PciIo,
             &Instance->IdeRegisters[Channel],
             &AtaCommandBlock,
             AtaStatusBlock,
             ATA_ATAPI_TIMEOUT,
             NULL
             );

  return Status;
}

/**
  Set drive parameters for devices not support PACKETS command.

  @param Instance         A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data structure.
  @param Channel          The channel number of device.
  @param Device           The device number of device.
  @param DriveParameters  A pointer to EFI_ATA_DRIVE_PARMS data structure.
  @param AtaStatusBlock   A pointer to EFI_ATA_STATUS_BLOCK data structure.

  @retval EFI_SUCCESS          Set drive parameter successfully.
  @retval EFI_DEVICE_ERROR     Set drive parameter failed.
  @retval EFI_OUT_OF_RESOURCES Allocate memory failed.

**/
EFI_STATUS
EFIAPI
SetDriveParameters (
  IN     ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN     UINT8                         Channel,
  IN     UINT8                         Device,
  IN     EFI_ATA_DRIVE_PARMS           *DriveParameters,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock
  )
{
  EFI_STATUS             Status;
  EFI_ATA_COMMAND_BLOCK  AtaCommandBlock;

  ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));

  AtaCommandBlock.AtaCommand     = ATA_CMD_INIT_DRIVE_PARAM;
  AtaCommandBlock.AtaSectorCount = DriveParameters->Sector;
  AtaCommandBlock.AtaDeviceHead  = (UINT8)((Device << 0x4) + DriveParameters->Heads);

  //
  // Send Init drive parameters
  //
  Status = AtaNonDataCommandIn (
             Instance->PciIo,
             &Instance->IdeRegisters[Channel],
             &AtaCommandBlock,
             AtaStatusBlock,
             ATA_ATAPI_TIMEOUT,
             NULL
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Init Drive Parameters Fail, Status = %r\n", Status));
  }

  //
  // Send Set Multiple parameters
  //
  AtaCommandBlock.AtaCommand     = ATA_CMD_SET_MULTIPLE_MODE;
  AtaCommandBlock.AtaSectorCount = DriveParameters->MultipleSector;
  AtaCommandBlock.AtaDeviceHead  = (UINT8)(Device << 0x4);

  Status = AtaNonDataCommandIn (
             Instance->PciIo,
             &Instance->IdeRegisters[Channel],
             &AtaCommandBlock,
             AtaStatusBlock,
             ATA_ATAPI_TIMEOUT,
             NULL
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Set Multiple Mode Parameters Fail, Status = %r\n", Status));
  }

  return Status;
}

/**
  Send SMART Return Status command to check if the execution of SMART cmd is successful or not.

  @param Instance         A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data structure.
  @param Channel          The channel number of device.
  @param Device           The device number of device.
  @param AtaStatusBlock   A pointer to EFI_ATA_STATUS_BLOCK data structure.

  @retval EFI_SUCCESS     Successfully get the return status of S.M.A.R.T command execution.
  @retval Others          Fail to get return status data.

**/
EFI_STATUS
EFIAPI
IdeAtaSmartReturnStatusCheck (
  IN     ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN     UINT8                         Channel,
  IN     UINT8                         Device,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock
  )
{
  EFI_STATUS             Status;
  EFI_ATA_COMMAND_BLOCK  AtaCommandBlock;
  UINT8                  LBAMid;
  UINT8                  LBAHigh;

  ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));

  AtaCommandBlock.AtaCommand      = ATA_CMD_SMART;
  AtaCommandBlock.AtaFeatures     = ATA_SMART_RETURN_STATUS;
  AtaCommandBlock.AtaCylinderLow  = ATA_CONSTANT_4F;
  AtaCommandBlock.AtaCylinderHigh = ATA_CONSTANT_C2;
  AtaCommandBlock.AtaDeviceHead   = (UINT8)((Device << 0x4) | 0xe0);

  //
  // Send S.M.A.R.T Read Return Status command to device
  //
  Status = AtaNonDataCommandIn (
             Instance->PciIo,
             &Instance->IdeRegisters[Channel],
             &AtaCommandBlock,
             AtaStatusBlock,
             ATA_ATAPI_TIMEOUT,
             NULL
             );

  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_ATA_BUS_SMART_DISABLED)
      );
    return EFI_DEVICE_ERROR;
  }

  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_ATA_BUS_SMART_ENABLE)
    );

  LBAMid  = IdeReadPortB (Instance->PciIo, Instance->IdeRegisters[Channel].CylinderLsb);
  LBAHigh = IdeReadPortB (Instance->PciIo, Instance->IdeRegisters[Channel].CylinderMsb);

  if ((LBAMid == 0x4f) && (LBAHigh == 0xc2)) {
    //
    // The threshold exceeded condition is not detected by the device
    //
    DEBUG ((DEBUG_INFO, "The S.M.A.R.T threshold exceeded condition is not detected\n"));
    REPORT_STATUS_CODE (
      EFI_PROGRESS_CODE,
      (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_ATA_BUS_SMART_UNDERTHRESHOLD)
      );
  } else if ((LBAMid == 0xf4) && (LBAHigh == 0x2c)) {
    //
    // The threshold exceeded condition is detected by the device
    //
    DEBUG ((DEBUG_INFO, "The S.M.A.R.T threshold exceeded condition is detected\n"));
    REPORT_STATUS_CODE (
      EFI_PROGRESS_CODE,
      (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_ATA_BUS_SMART_OVERTHRESHOLD)
      );
  }

  return EFI_SUCCESS;
}

/**
  Enable SMART command of the disk if supported.

  @param Instance         A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data structure.
  @param Channel          The channel number of device.
  @param Device           The device number of device.
  @param IdentifyData     A pointer to data buffer which is used to contain IDENTIFY data.
  @param AtaStatusBlock   A pointer to EFI_ATA_STATUS_BLOCK data structure.

**/
VOID
EFIAPI
IdeAtaSmartSupport (
  IN     ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN     UINT8                         Channel,
  IN     UINT8                         Device,
  IN     EFI_IDENTIFY_DATA             *IdentifyData,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock
  )
{
  EFI_STATUS             Status;
  EFI_ATA_COMMAND_BLOCK  AtaCommandBlock;

  //
  // Detect if the device supports S.M.A.R.T.
  //
  if ((IdentifyData->AtaData.command_set_supported_82 & 0x0001) != 0x0001) {
    //
    // S.M.A.R.T is not supported by the device
    //
    DEBUG ((
      DEBUG_INFO,
      "S.M.A.R.T feature is not supported at [%a] channel [%a] device!\n",
      (Channel == 1) ? "secondary" : "primary",
      (Device == 1) ? "slave" : "master"
      ));
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_ATA_BUS_SMART_NOTSUPPORTED)
      );
  } else {
    //
    // Check if the feature is enabled. If not, then enable S.M.A.R.T.
    //
    if ((IdentifyData->AtaData.command_set_feature_enb_85 & 0x0001) != 0x0001) {
      REPORT_STATUS_CODE (
        EFI_PROGRESS_CODE,
        (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_ATA_BUS_SMART_DISABLE)
        );

      ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));

      AtaCommandBlock.AtaCommand      = ATA_CMD_SMART;
      AtaCommandBlock.AtaFeatures     = ATA_SMART_ENABLE_OPERATION;
      AtaCommandBlock.AtaCylinderLow  = ATA_CONSTANT_4F;
      AtaCommandBlock.AtaCylinderHigh = ATA_CONSTANT_C2;
      AtaCommandBlock.AtaDeviceHead   = (UINT8)((Device << 0x4) | 0xe0);

      //
      // Send S.M.A.R.T Enable command to device
      //
      Status = AtaNonDataCommandIn (
                 Instance->PciIo,
                 &Instance->IdeRegisters[Channel],
                 &AtaCommandBlock,
                 AtaStatusBlock,
                 ATA_ATAPI_TIMEOUT,
                 NULL
                 );

      if (!EFI_ERROR (Status)) {
        //
        // Send S.M.A.R.T AutoSave command to device
        //
        ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));

        AtaCommandBlock.AtaCommand      = ATA_CMD_SMART;
        AtaCommandBlock.AtaFeatures     = 0xD2;
        AtaCommandBlock.AtaSectorCount  = 0xF1;
        AtaCommandBlock.AtaCylinderLow  = ATA_CONSTANT_4F;
        AtaCommandBlock.AtaCylinderHigh = ATA_CONSTANT_C2;
        AtaCommandBlock.AtaDeviceHead   = (UINT8)((Device << 0x4) | 0xe0);

        Status = AtaNonDataCommandIn (
                   Instance->PciIo,
                   &Instance->IdeRegisters[Channel],
                   &AtaCommandBlock,
                   AtaStatusBlock,
                   ATA_ATAPI_TIMEOUT,
                   NULL
                   );
        if (!EFI_ERROR (Status)) {
          Status = IdeAtaSmartReturnStatusCheck (
                     Instance,
                     Channel,
                     Device,
                     AtaStatusBlock
                     );
        }
      }
    }

    DEBUG ((
      DEBUG_INFO,
      "Enabled S.M.A.R.T feature at [%a] channel [%a] device!\n",
      (Channel == 1) ? "secondary" : "primary",
      (Device == 1) ? "slave" : "master"
      ));
  }

  return;
}

/**
  Sends out an ATA Identify Command to the specified device.

  This function is called by DiscoverIdeDevice() during its device
  identification. It sends out the ATA Identify Command to the
  specified device. Only ATA device responses to this command. If
  the command succeeds, it returns the Identify data structure which
  contains information about the device. This function extracts the
  information it needs to fill the IDE_BLK_IO_DEV data structure,
  including device type, media block size, media capacity, and etc.

  @param Instance         A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data structure.
  @param Channel          The channel number of device.
  @param Device           The device number of device.
  @param Buffer           A pointer to data buffer which is used to contain IDENTIFY data.
  @param AtaStatusBlock   A pointer to EFI_ATA_STATUS_BLOCK data structure.

  @retval EFI_SUCCESS          Identify ATA device successfully.
  @retval EFI_DEVICE_ERROR     ATA Identify Device Command failed or device is not ATA device.
  @retval EFI_OUT_OF_RESOURCES Allocate memory failed.

**/
EFI_STATUS
EFIAPI
AtaIdentify (
  IN     ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN     UINT8                         Channel,
  IN     UINT8                         Device,
  IN OUT EFI_IDENTIFY_DATA             *Buffer,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock
  )
{
  EFI_STATUS             Status;
  EFI_ATA_COMMAND_BLOCK  AtaCommandBlock;

  ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));

  AtaCommandBlock.AtaCommand    = ATA_CMD_IDENTIFY_DRIVE;
  AtaCommandBlock.AtaDeviceHead = (UINT8)(Device << 0x4);

  Status = AtaPioDataInOut (
             Instance->PciIo,
             &Instance->IdeRegisters[Channel],
             Buffer,
             sizeof (EFI_IDENTIFY_DATA),
             TRUE,
             &AtaCommandBlock,
             AtaStatusBlock,
             ATA_ATAPI_TIMEOUT,
             NULL
             );

  return Status;
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

  @param Instance         A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data structure.
  @param Channel          The channel number of device.
  @param Device           The device number of device.
  @param Buffer           A pointer to data buffer which is used to contain IDENTIFY data.
  @param AtaStatusBlock   A pointer to EFI_ATA_STATUS_BLOCK data structure.

  @retval EFI_SUCCESS          Identify ATAPI device successfully.
  @retval EFI_DEVICE_ERROR     ATA Identify Packet Device Command failed or device type
                               is not supported by this IDE driver.
  @retval EFI_OUT_OF_RESOURCES Allocate memory failed.

**/
EFI_STATUS
EFIAPI
AtaIdentifyPacket (
  IN     ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN     UINT8                         Channel,
  IN     UINT8                         Device,
  IN OUT EFI_IDENTIFY_DATA             *Buffer,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock
  )
{
  EFI_STATUS             Status;
  EFI_ATA_COMMAND_BLOCK  AtaCommandBlock;

  ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));

  AtaCommandBlock.AtaCommand    = ATA_CMD_IDENTIFY_DEVICE;
  AtaCommandBlock.AtaDeviceHead = (UINT8)(Device << 0x4);

  //
  // Send ATAPI Identify Command to get IDENTIFY data.
  //
  Status = AtaPioDataInOut (
             Instance->PciIo,
             &Instance->IdeRegisters[Channel],
             (VOID *)Buffer,
             sizeof (EFI_IDENTIFY_DATA),
             TRUE,
             &AtaCommandBlock,
             AtaStatusBlock,
             ATA_ATAPI_TIMEOUT,
             NULL
             );

  return Status;
}

/**
  This function is used for detect whether the IDE device exists in the
  specified Channel as the specified Device Number.

  There is two IDE channels: one is Primary Channel, the other is
  Secondary Channel.(Channel is the logical name for the physical "Cable".)
  Different channel has different register group.

  On each IDE channel, at most two IDE devices attach,
  one is called Device 0 (Master device), the other is called Device 1
  (Slave device). The devices on the same channel co-use the same register
  group, so before sending out a command for a specified device via command
  register, it is a must to select the current device to accept the command
  by set the device number in the Head/Device Register.

  @param Instance         A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data structure.
  @param IdeChannel       The channel number of device.

  @retval EFI_SUCCESS successfully detects device.
  @retval other       any failure during detection process will return this value.

**/
EFI_STATUS
EFIAPI
DetectAndConfigIdeDevice (
  IN  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN  UINT8                         IdeChannel
  )
{
  EFI_STATUS           Status;
  UINT8                SectorCountReg;
  UINT8                LBALowReg;
  UINT8                LBAMidReg;
  UINT8                LBAHighReg;
  EFI_ATA_DEVICE_TYPE  DeviceType;
  UINT8                IdeDevice;
  EFI_IDE_REGISTERS    *IdeRegisters;
  EFI_IDENTIFY_DATA    Buffer;

  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *IdeInit;
  EFI_PCI_IO_PROTOCOL               *PciIo;

  EFI_ATA_COLLECTIVE_MODE  *SupportedModes;
  EFI_ATA_TRANSFER_MODE    TransferMode;
  EFI_ATA_DRIVE_PARMS      DriveParameters;

  IdeRegisters = &Instance->IdeRegisters[IdeChannel];
  IdeInit      = Instance->IdeControllerInit;
  PciIo        = Instance->PciIo;

  for (IdeDevice = 0; IdeDevice < EfiIdeMaxDevice; IdeDevice++) {
    //
    // Select Master or Slave device to get the return signature for ATA DEVICE DIAGNOSTIC cmd.
    //
    IdeWritePortB (PciIo, IdeRegisters->Head, (UINT8)((IdeDevice << 4) | 0xe0));

    //
    // Send ATA Device Execut Diagnostic command.
    // This command should work no matter DRDY is ready or not
    //
    IdeWritePortB (PciIo, IdeRegisters->CmdOrStatus, ATA_CMD_EXEC_DRIVE_DIAG);

    Status = WaitForBSYClear (PciIo, IdeRegisters, 350000000);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "New detecting method: Send Execute Diagnostic Command: WaitForBSYClear: Status: %d\n", Status));
      continue;
    }

    //
    // Select Master or Slave device to get the return signature for ATA DEVICE DIAGNOSTIC cmd.
    //
    IdeWritePortB (PciIo, IdeRegisters->Head, (UINT8)((IdeDevice << 4) | 0xe0));
    //
    // Stall for 1 milliseconds.
    //
    MicroSecondDelay (1000);

    SectorCountReg = IdeReadPortB (PciIo, IdeRegisters->SectorCount);
    LBALowReg      = IdeReadPortB (PciIo, IdeRegisters->SectorNumber);
    LBAMidReg      = IdeReadPortB (PciIo, IdeRegisters->CylinderLsb);
    LBAHighReg     = IdeReadPortB (PciIo, IdeRegisters->CylinderMsb);

    //
    // Refer to ATA/ATAPI 4 Spec, section 9.1
    //
    if ((SectorCountReg == 0x1) && (LBALowReg == 0x1) && (LBAMidReg == 0x0) && (LBAHighReg == 0x0)) {
      DeviceType = EfiIdeHarddisk;
    } else if ((LBAMidReg == 0x14) && (LBAHighReg == 0xeb)) {
      DeviceType = EfiIdeCdrom;
    } else {
      continue;
    }

    //
    // Send IDENTIFY cmd to the device to test if it is really attached.
    //
    if (DeviceType == EfiIdeHarddisk) {
      Status = AtaIdentify (Instance, IdeChannel, IdeDevice, &Buffer, NULL);
      //
      // if identifying ata device is failure, then try to send identify packet cmd.
      //
      if (EFI_ERROR (Status)) {
        REPORT_STATUS_CODE (EFI_PROGRESS_CODE, (EFI_PERIPHERAL_FIXED_MEDIA | EFI_P_EC_NOT_DETECTED));

        DeviceType = EfiIdeCdrom;
        Status     = AtaIdentifyPacket (Instance, IdeChannel, IdeDevice, &Buffer, NULL);
      }
    } else {
      Status = AtaIdentifyPacket (Instance, IdeChannel, IdeDevice, &Buffer, NULL);
      //
      // if identifying atapi device is failure, then try to send identify cmd.
      //
      if (EFI_ERROR (Status)) {
        DeviceType = EfiIdeHarddisk;
        Status     = AtaIdentify (Instance, IdeChannel, IdeDevice, &Buffer, NULL);
      }
    }

    if (EFI_ERROR (Status)) {
      //
      // No device is found at this port
      //
      continue;
    }

    DEBUG ((
      DEBUG_INFO,
      "[%a] channel [%a] [%a] device\n",
      (IdeChannel == 1) ? "secondary" : "primary  ",
      (IdeDevice == 1) ? "slave " : "master",
      DeviceType == EfiIdeCdrom ? "cdrom   " : "harddisk"
      ));
    //
    // If the device is a hard disk, then try to enable S.M.A.R.T feature
    //
    if ((DeviceType == EfiIdeHarddisk) && PcdGetBool (PcdAtaSmartEnable)) {
      IdeAtaSmartSupport (
        Instance,
        IdeChannel,
        IdeDevice,
        &Buffer,
        NULL
        );
    }

    //
    // Submit identify data to IDE controller init driver
    //
    IdeInit->SubmitData (IdeInit, IdeChannel, IdeDevice, &Buffer);

    //
    // Now start to config ide device parameter and transfer mode.
    //
    Status = IdeInit->CalculateMode (
                        IdeInit,
                        IdeChannel,
                        IdeDevice,
                        &SupportedModes
                        );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Calculate Mode Fail, Status = %r\n", Status));
      continue;
    }

    //
    // Set best supported PIO mode on this IDE device
    //
    if (SupportedModes->PioMode.Mode <= EfiAtaPioMode2) {
      TransferMode.ModeCategory = EFI_ATA_MODE_DEFAULT_PIO;
    } else {
      TransferMode.ModeCategory = EFI_ATA_MODE_FLOW_PIO;
    }

    TransferMode.ModeNumber = (UINT8)(SupportedModes->PioMode.Mode);

    if (SupportedModes->ExtModeCount == 0) {
      Status = SetDeviceTransferMode (Instance, IdeChannel, IdeDevice, &TransferMode, NULL);

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Set transfer Mode Fail, Status = %r\n", Status));
        continue;
      }
    }

    //
    // Set supported DMA mode on this IDE device. Note that UDMA & MDMA can't
    // be set together. Only one DMA mode can be set to a device. If setting
    // DMA mode operation fails, we can continue moving on because we only use
    // PIO mode at boot time. DMA modes are used by certain kind of OS booting
    //
    if (SupportedModes->UdmaMode.Valid) {
      TransferMode.ModeCategory = EFI_ATA_MODE_UDMA;
      TransferMode.ModeNumber   = (UINT8)(SupportedModes->UdmaMode.Mode);
      Status                    = SetDeviceTransferMode (Instance, IdeChannel, IdeDevice, &TransferMode, NULL);

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Set transfer Mode Fail, Status = %r\n", Status));
        continue;
      }
    } else if (SupportedModes->MultiWordDmaMode.Valid) {
      TransferMode.ModeCategory = EFI_ATA_MODE_MDMA;
      TransferMode.ModeNumber   = (UINT8)SupportedModes->MultiWordDmaMode.Mode;
      Status                    = SetDeviceTransferMode (Instance, IdeChannel, IdeDevice, &TransferMode, NULL);

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Set transfer Mode Fail, Status = %r\n", Status));
        continue;
      }
    }

    //
    // Set Parameters for the device:
    // 1) Init
    // 2) Establish the block count for READ/WRITE MULTIPLE (EXT) command
    //
    if (DeviceType == EfiIdeHarddisk) {
      //
      // Init drive parameters
      //
      DriveParameters.Sector         = (UINT8)((ATA5_IDENTIFY_DATA *)(&Buffer.AtaData))->sectors_per_track;
      DriveParameters.Heads          = (UINT8)(((ATA5_IDENTIFY_DATA *)(&Buffer.AtaData))->heads - 1);
      DriveParameters.MultipleSector = (UINT8)((ATA5_IDENTIFY_DATA *)(&Buffer.AtaData))->multi_sector_cmd_max_sct_cnt;

      SetDriveParameters (Instance, IdeChannel, IdeDevice, &DriveParameters, NULL);
    }

    //
    // Set IDE controller Timing Blocks in the PCI Configuration Space
    //
    IdeInit->SetTiming (IdeInit, IdeChannel, IdeDevice, SupportedModes);

    //
    // IDE controller and IDE device timing is configured successfully.
    // Now insert the device into device list.
    //
    Status = CreateNewDeviceInfo (Instance, IdeChannel, IdeDevice, DeviceType, &Buffer);
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (DeviceType == EfiIdeHarddisk) {
      REPORT_STATUS_CODE (EFI_PROGRESS_CODE, (EFI_PERIPHERAL_FIXED_MEDIA | EFI_P_PC_ENABLE));
    }
  }

  return EFI_SUCCESS;
}

/**
  Initialize ATA host controller at IDE mode.

  The function is designed to initialize ATA host controller.

  @param[in]  Instance          A pointer to the ATA_ATAPI_PASS_THRU_INSTANCE instance.

**/
EFI_STATUS
EFIAPI
IdeModeInitialization (
  IN  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance
  )
{
  EFI_STATUS                        Status;
  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *IdeInit;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  UINT8                             Channel;
  UINT8                             IdeChannel;
  BOOLEAN                           ChannelEnabled;
  UINT8                             MaxDevices;

  IdeInit = Instance->IdeControllerInit;
  PciIo   = Instance->PciIo;
  Channel = IdeInit->ChannelCount;

  //
  // Obtain IDE IO port registers' base addresses
  //
  Status = GetIdeRegisterIoAddr (PciIo, Instance->IdeRegisters);
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  for (IdeChannel = 0; IdeChannel < Channel; IdeChannel++) {
    IdeInit->NotifyPhase (IdeInit, EfiIdeBeforeChannelEnumeration, IdeChannel);

    //
    // now obtain channel information fron IdeControllerInit protocol.
    //
    Status = IdeInit->GetChannelInfo (
                        IdeInit,
                        IdeChannel,
                        &ChannelEnabled,
                        &MaxDevices
                        );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[GetChannel, Status=%x]", Status));
      continue;
    }

    if (!ChannelEnabled) {
      continue;
    }

    ASSERT (MaxDevices <= 2);
    //
    // Now inform the IDE Controller Init Module.
    //
    IdeInit->NotifyPhase (IdeInit, EfiIdeBeforeChannelReset, IdeChannel);

    //
    // No reset channel function implemented.
    //
    IdeInit->NotifyPhase (IdeInit, EfiIdeAfterChannelReset, IdeChannel);

    //
    // Now inform the IDE Controller Init Module.
    //
    IdeInit->NotifyPhase (IdeInit, EfiIdeBusBeforeDevicePresenceDetection, IdeChannel);

    //
    // Detect all attached ATA devices and set the transfer mode for each device.
    //
    DetectAndConfigIdeDevice (Instance, IdeChannel);
  }

  //
  // All configurations done! Notify IdeController to do post initialization
  // work such as saving IDE controller PCI settings for S3 resume
  //
  IdeInit->NotifyPhase (IdeInit, EfiIdeBusPhaseMaximum, 0);

ErrorExit:
  return Status;
}
