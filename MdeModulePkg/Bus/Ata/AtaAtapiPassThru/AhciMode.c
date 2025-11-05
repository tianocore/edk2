/** @file
  The file for AHCI mode of ATA host controller.

  Copyright (c) 2010 - 2020, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AtaAtapiPassThru.h"

/**
  Read AHCI Operation register.

  @param  PciIo        The PCI IO protocol instance.
  @param  Offset       The operation register offset.

  @return The register content read.

**/
UINT32
EFIAPI
AhciReadReg (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINT32              Offset
  )
{
  UINT32  Data;

  ASSERT (PciIo != NULL);

  Data = 0;

  PciIo->Mem.Read (
               PciIo,
               EfiPciIoWidthUint32,
               EFI_AHCI_BAR_INDEX,
               (UINT64)Offset,
               1,
               &Data
               );

  return Data;
}

/**
  Write AHCI Operation register.

  @param  PciIo        The PCI IO protocol instance.
  @param  Offset       The operation register offset.
  @param  Data         The data used to write down.

**/
VOID
EFIAPI
AhciWriteReg (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN UINT32               Data
  )
{
  ASSERT (PciIo != NULL);

  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint32,
               EFI_AHCI_BAR_INDEX,
               (UINT64)Offset,
               1,
               &Data
               );

  return;
}

/**
  Do AND operation with the value of AHCI Operation register.

  @param  PciIo        The PCI IO protocol instance.
  @param  Offset       The operation register offset.
  @param  AndData      The data used to do AND operation.

**/
VOID
EFIAPI
AhciAndReg (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN UINT32               AndData
  )
{
  UINT32  Data;

  ASSERT (PciIo != NULL);

  Data = AhciReadReg (PciIo, Offset);

  Data &= AndData;

  AhciWriteReg (PciIo, Offset, Data);
}

/**
  Do OR operation with the value of AHCI Operation register.

  @param  PciIo        The PCI IO protocol instance.
  @param  Offset       The operation register offset.
  @param  OrData       The data used to do OR operation.

**/
VOID
EFIAPI
AhciOrReg (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN UINT32               OrData
  )
{
  UINT32  Data;

  ASSERT (PciIo != NULL);

  Data = AhciReadReg (PciIo, Offset);

  Data |= OrData;

  AhciWriteReg (PciIo, Offset, Data);
}

/**
  Wait for the value of the specified MMIO register set to the test value.

  @param  PciIo             The PCI IO protocol instance.
  @param  Offset            The MMIO address to test.
  @param  MaskValue         The mask value of memory.
  @param  TestValue         The test value of memory.
  @param  Timeout           The time out value for wait memory set, uses 100ns as a unit.

  @retval EFI_TIMEOUT       The MMIO setting is time out.
  @retval EFI_SUCCESS       The MMIO is correct set.

**/
EFI_STATUS
EFIAPI
AhciWaitMmioSet (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINTN                Offset,
  IN  UINT32               MaskValue,
  IN  UINT32               TestValue,
  IN  UINT64               Timeout
  )
{
  UINT32   Value;
  UINT64   Delay;
  BOOLEAN  InfiniteWait;

  if (Timeout == 0) {
    InfiniteWait = TRUE;
  } else {
    InfiniteWait = FALSE;
  }

  Delay = DivU64x32 (Timeout, 1000) + 1;

  do {
    //
    // Access PCI MMIO space to see if the value is the tested one.
    //
    Value = AhciReadReg (PciIo, (UINT32)Offset) & MaskValue;

    if (Value == TestValue) {
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
  Wait for the value of the specified system memory set to the test value.

  @param  Address           The system memory address to test.
  @param  MaskValue         The mask value of memory.
  @param  TestValue         The test value of memory.
  @param  Timeout           The time out value for wait memory set, uses 100ns as a unit.

  @retval EFI_TIMEOUT       The system memory setting is time out.
  @retval EFI_SUCCESS       The system memory is correct set.

**/
EFI_STATUS
EFIAPI
AhciWaitMemSet (
  IN  EFI_PHYSICAL_ADDRESS  Address,
  IN  UINT32                MaskValue,
  IN  UINT32                TestValue,
  IN  UINT64                Timeout
  )
{
  UINT32   Value;
  UINT64   Delay;
  BOOLEAN  InfiniteWait;

  if (Timeout == 0) {
    InfiniteWait = TRUE;
  } else {
    InfiniteWait = FALSE;
  }

  Delay =  DivU64x32 (Timeout, 1000) + 1;

  do {
    //
    // Access system memory to see if the value is the tested one.
    //
    // The system memory pointed by Address will be updated by the
    // SATA Host Controller, "volatile" is introduced to prevent
    // compiler from optimizing the access to the memory address
    // to only read once.
    //
    Value  = *(volatile UINT32 *)(UINTN)Address;
    Value &= MaskValue;

    if (Value == TestValue) {
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
  Check the memory status to the test value.

  @param[in] Address    The memory address to test.
  @param[in] MaskValue  The mask value of memory.
  @param[in] TestValue  The test value of memory.

  @retval EFI_NOT_READY     The memory is not set.
  @retval EFI_SUCCESS       The memory is correct set.
**/
EFI_STATUS
EFIAPI
AhciCheckMemSet (
  IN     UINTN   Address,
  IN     UINT32  MaskValue,
  IN     UINT32  TestValue
  )
{
  UINT32  Value;

  Value  = *(volatile UINT32 *)Address;
  Value &= MaskValue;

  if (Value == TestValue) {
    return EFI_SUCCESS;
  }

  return EFI_NOT_READY;
}

/**

  Clear the port interrupt and error status. It will also clear
  HBA interrupt status.

  @param      PciIo          The PCI IO protocol instance.
  @param      Port           The number of port.

**/
VOID
EFIAPI
AhciClearPortStatus (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINT8                Port
  )
{
  UINT32  Offset;

  //
  // Clear any error status
  //
  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SERR;
  AhciWriteReg (PciIo, Offset, AhciReadReg (PciIo, Offset));

  //
  // Clear any port interrupt status
  //
  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_IS;
  AhciWriteReg (PciIo, Offset, AhciReadReg (PciIo, Offset));

  //
  // Clear any HBA interrupt status
  //
  AhciWriteReg (PciIo, EFI_AHCI_IS_OFFSET, AhciReadReg (PciIo, EFI_AHCI_IS_OFFSET));
}

/**
  This function is used to dump the Status Registers and if there is ERR bit set
  in the Status Register, the Error Register's value is also be dumped.

  @param  PciIo            The PCI IO protocol instance.
  @param  AhciRegisters    The pointer to the EFI_AHCI_REGISTERS.
  @param  Port             The number of port.
  @param  AtaStatusBlock   A pointer to EFI_ATA_STATUS_BLOCK data structure.

**/
VOID
EFIAPI
AhciDumpPortStatus (
  IN     EFI_PCI_IO_PROTOCOL   *PciIo,
  IN     EFI_AHCI_REGISTERS    *AhciRegisters,
  IN     UINT8                 Port,
  IN OUT EFI_ATA_STATUS_BLOCK  *AtaStatusBlock
  )
{
  UINTN       Offset;
  UINT32      Data;
  UINTN       FisBaseAddr;
  EFI_STATUS  Status;

  ASSERT (PciIo != NULL);

  if (AtaStatusBlock != NULL) {
    ZeroMem (AtaStatusBlock, sizeof (EFI_ATA_STATUS_BLOCK));

    FisBaseAddr = (UINTN)AhciRegisters->AhciRFis + Port * sizeof (EFI_AHCI_RECEIVED_FIS);
    Offset      = FisBaseAddr + EFI_AHCI_D2H_FIS_OFFSET;

    Status = AhciCheckMemSet (Offset, EFI_AHCI_FIS_TYPE_MASK, EFI_AHCI_FIS_REGISTER_D2H);
    if (!EFI_ERROR (Status)) {
      //
      // If D2H FIS is received, update StatusBlock with its content.
      //
      CopyMem (AtaStatusBlock, (UINT8 *)Offset, sizeof (EFI_ATA_STATUS_BLOCK));
    } else {
      //
      // If D2H FIS is not received, only update Status & Error field through PxTFD
      // as there is no other way to get the content of the Shadow Register Block.
      //
      Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_TFD;
      Data   = AhciReadReg (PciIo, (UINT32)Offset);

      AtaStatusBlock->AtaStatus = (UINT8)Data;
      if ((AtaStatusBlock->AtaStatus & BIT0) != 0) {
        AtaStatusBlock->AtaError = (UINT8)(Data >> 8);
      }
    }
  }
}

/**
  Enable the FIS running for giving port.

  @param      PciIo          The PCI IO protocol instance.
  @param      Port           The number of port.
  @param      Timeout        The timeout value of enabling FIS, uses 100ns as a unit.

  @retval EFI_DEVICE_ERROR   The FIS enable setting fails.
  @retval EFI_TIMEOUT        The FIS enable setting is time out.
  @retval EFI_SUCCESS        The FIS enable successfully.

**/
EFI_STATUS
EFIAPI
AhciEnableFisReceive (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINT8                Port,
  IN  UINT64               Timeout
  )
{
  UINT32  Offset;

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
  AhciOrReg (PciIo, Offset, EFI_AHCI_PORT_CMD_FRE);

  return EFI_SUCCESS;
}

/**
  Disable the FIS running for giving port.

  @param      PciIo          The PCI IO protocol instance.
  @param      Port           The number of port.
  @param      Timeout        The timeout value of disabling FIS, uses 100ns as a unit.

  @retval EFI_DEVICE_ERROR   The FIS disable setting fails.
  @retval EFI_TIMEOUT        The FIS disable setting is time out.
  @retval EFI_UNSUPPORTED    The port is in running state.
  @retval EFI_SUCCESS        The FIS disable successfully.

**/
EFI_STATUS
EFIAPI
AhciDisableFisReceive (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINT8                Port,
  IN  UINT64               Timeout
  )
{
  UINT32  Offset;
  UINT32  Data;

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
  Data   = AhciReadReg (PciIo, Offset);

  //
  // Before disabling Fis receive, the DMA engine of the port should NOT be in running status.
  //
  if ((Data & (EFI_AHCI_PORT_CMD_ST | EFI_AHCI_PORT_CMD_CR)) != 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check if the Fis receive DMA engine for the port is running.
  //
  if ((Data & EFI_AHCI_PORT_CMD_FR) != EFI_AHCI_PORT_CMD_FR) {
    return EFI_SUCCESS;
  }

  AhciAndReg (PciIo, Offset, (UINT32) ~(EFI_AHCI_PORT_CMD_FRE));

  return AhciWaitMmioSet (
           PciIo,
           Offset,
           EFI_AHCI_PORT_CMD_FR,
           0,
           Timeout
           );
}

/**
  Build the command list, command table and prepare the fis receiver.

  @param    PciIo                 The PCI IO protocol instance.
  @param    AhciRegisters         The pointer to the EFI_AHCI_REGISTERS.
  @param    Port                  The number of port.
  @param    PortMultiplier        The timeout value of stop.
  @param    CommandFis            The control fis will be used for the transfer.
  @param    CommandList           The command list will be used for the transfer.
  @param    AtapiCommand          The atapi command will be used for the transfer.
  @param    AtapiCommandLength    The length of the atapi command.
  @param    CommandSlotNumber     The command slot will be used for the transfer.
  @param    DataPhysicalAddr      The pointer to the data buffer pci bus master address.
  @param    DataLength            The data count to be transferred.

**/
VOID
EFIAPI
AhciBuildCommand (
  IN     EFI_PCI_IO_PROTOCOL     *PciIo,
  IN     EFI_AHCI_REGISTERS      *AhciRegisters,
  IN     UINT8                   Port,
  IN     UINT8                   PortMultiplier,
  IN     EFI_AHCI_COMMAND_FIS    *CommandFis,
  IN     EFI_AHCI_COMMAND_LIST   *CommandList,
  IN     EFI_AHCI_ATAPI_COMMAND  *AtapiCommand OPTIONAL,
  IN     UINT8                   AtapiCommandLength,
  IN     UINT8                   CommandSlotNumber,
  IN OUT VOID                    *DataPhysicalAddr,
  IN     UINT32                  DataLength
  )
{
  UINT64   BaseAddr;
  UINT32   PrdtNumber;
  UINT32   PrdtIndex;
  UINTN    RemainedData;
  UINTN    MemAddr;
  DATA_64  Data64;
  UINT32   Offset;

  //
  // Filling the PRDT
  //
  PrdtNumber = (UINT32)DivU64x32 (((UINT64)DataLength + EFI_AHCI_MAX_DATA_PER_PRDT - 1), EFI_AHCI_MAX_DATA_PER_PRDT);

  //
  // According to AHCI 1.3 spec, a PRDT entry can point to a maximum 4MB data block.
  // It also limits that the maximum amount of the PRDT entry in the command table
  // is 65535.
  //
  ASSERT (PrdtNumber <= 65535);

  Data64.Uint64 = (UINTN)(AhciRegisters->AhciRFis) + sizeof (EFI_AHCI_RECEIVED_FIS) * Port;

  BaseAddr = Data64.Uint64;

  ZeroMem ((VOID *)((UINTN)BaseAddr), sizeof (EFI_AHCI_RECEIVED_FIS));

  ZeroMem (AhciRegisters->AhciCommandTable, sizeof (EFI_AHCI_COMMAND_TABLE));

  CommandFis->AhciCFisPmNum = PortMultiplier;

  CopyMem (&AhciRegisters->AhciCommandTable->CommandFis, CommandFis, sizeof (EFI_AHCI_COMMAND_FIS));

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
  if (AtapiCommand != NULL) {
    CopyMem (
      &AhciRegisters->AhciCommandTable->AtapiCmd,
      AtapiCommand,
      AtapiCommandLength
      );

    CommandList->AhciCmdA = 1;
    CommandList->AhciCmdP = 1;

    AhciOrReg (PciIo, Offset, (EFI_AHCI_PORT_CMD_DLAE | EFI_AHCI_PORT_CMD_ATAPI));
  } else {
    AhciAndReg (PciIo, Offset, (UINT32) ~(EFI_AHCI_PORT_CMD_DLAE | EFI_AHCI_PORT_CMD_ATAPI));
  }

  RemainedData              = (UINTN)DataLength;
  MemAddr                   = (UINTN)DataPhysicalAddr;
  CommandList->AhciCmdPrdtl = PrdtNumber;

  for (PrdtIndex = 0; PrdtIndex < PrdtNumber; PrdtIndex++) {
    if (RemainedData < EFI_AHCI_MAX_DATA_PER_PRDT) {
      AhciRegisters->AhciCommandTable->PrdtTable[PrdtIndex].AhciPrdtDbc = (UINT32)RemainedData - 1;
    } else {
      AhciRegisters->AhciCommandTable->PrdtTable[PrdtIndex].AhciPrdtDbc = EFI_AHCI_MAX_DATA_PER_PRDT - 1;
    }

    Data64.Uint64                                                      = (UINT64)MemAddr;
    AhciRegisters->AhciCommandTable->PrdtTable[PrdtIndex].AhciPrdtDba  = Data64.Uint32.Lower32;
    AhciRegisters->AhciCommandTable->PrdtTable[PrdtIndex].AhciPrdtDbau = Data64.Uint32.Upper32;
    RemainedData                                                      -= EFI_AHCI_MAX_DATA_PER_PRDT;
    MemAddr                                                           += EFI_AHCI_MAX_DATA_PER_PRDT;
  }

  //
  // Set the last PRDT to Interrupt On Complete
  //
  if (PrdtNumber > 0) {
    AhciRegisters->AhciCommandTable->PrdtTable[PrdtNumber - 1].AhciPrdtIoc = 1;
  }

  CopyMem (
    (VOID *)((UINTN)AhciRegisters->AhciCmdList + (UINTN)CommandSlotNumber * sizeof (EFI_AHCI_COMMAND_LIST)),
    CommandList,
    sizeof (EFI_AHCI_COMMAND_LIST)
    );

  Data64.Uint64                                              = (UINT64)(UINTN)AhciRegisters->AhciCommandTablePciAddr;
  AhciRegisters->AhciCmdList[CommandSlotNumber].AhciCmdCtba  = Data64.Uint32.Lower32;
  AhciRegisters->AhciCmdList[CommandSlotNumber].AhciCmdCtbau = Data64.Uint32.Upper32;
  AhciRegisters->AhciCmdList[CommandSlotNumber].AhciCmdPmp   = PortMultiplier;
}

/**
  Build a command FIS.

  @param  CmdFis            A pointer to the EFI_AHCI_COMMAND_FIS data structure.
  @param  AtaCommandBlock   A pointer to the AhciBuildCommandFis data structure.

**/
VOID
EFIAPI
AhciBuildCommandFis (
  IN OUT EFI_AHCI_COMMAND_FIS   *CmdFis,
  IN     EFI_ATA_COMMAND_BLOCK  *AtaCommandBlock
  )
{
  ZeroMem (CmdFis, sizeof (EFI_AHCI_COMMAND_FIS));

  CmdFis->AhciCFisType = EFI_AHCI_FIS_REGISTER_H2D;
  //
  // Indicator it's a command
  //
  CmdFis->AhciCFisCmdInd = 0x1;
  CmdFis->AhciCFisCmd    = AtaCommandBlock->AtaCommand;

  CmdFis->AhciCFisFeature    = AtaCommandBlock->AtaFeatures;
  CmdFis->AhciCFisFeatureExp = AtaCommandBlock->AtaFeaturesExp;

  CmdFis->AhciCFisSecNum    = AtaCommandBlock->AtaSectorNumber;
  CmdFis->AhciCFisSecNumExp = AtaCommandBlock->AtaSectorNumberExp;

  CmdFis->AhciCFisClyLow    = AtaCommandBlock->AtaCylinderLow;
  CmdFis->AhciCFisClyLowExp = AtaCommandBlock->AtaCylinderLowExp;

  CmdFis->AhciCFisClyHigh    = AtaCommandBlock->AtaCylinderHigh;
  CmdFis->AhciCFisClyHighExp = AtaCommandBlock->AtaCylinderHighExp;

  CmdFis->AhciCFisSecCount    = AtaCommandBlock->AtaSectorCount;
  CmdFis->AhciCFisSecCountExp = AtaCommandBlock->AtaSectorCountExp;

  CmdFis->AhciCFisDevHead = (UINT8)(AtaCommandBlock->AtaDeviceHead | 0xE0);
}

/**
  Wait until SATA device reports it is ready for operation.

  @param[in] PciIo    Pointer to AHCI controller PciIo.
  @param[in] Port     SATA port index on which to reset.

  @retval EFI_SUCCESS  Device ready for operation.
  @retval EFI_TIMEOUT  Device failed to get ready within required period.
**/
EFI_STATUS
AhciWaitDeviceReady (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT8                Port
  )
{
  UINT32  PhyDetectDelay;
  UINT32  Data;
  UINT32  Offset;

  //
  // According to SATA1.0a spec section 5.2, we need to wait for PxTFD.BSY and PxTFD.DRQ
  // and PxTFD.ERR to be zero. The maximum wait time is 16s which is defined at ATA spec.
  //
  PhyDetectDelay = 16 * 1000;
  do {
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SERR;
    if (AhciReadReg (PciIo, Offset) != 0) {
      AhciWriteReg (PciIo, Offset, AhciReadReg (PciIo, Offset));
    }

    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_TFD;

    Data = AhciReadReg (PciIo, Offset) & EFI_AHCI_PORT_TFD_MASK;
    if (Data == 0) {
      break;
    }

    MicroSecondDelay (1000);
    PhyDetectDelay--;
  } while (PhyDetectDelay > 0);

  if (PhyDetectDelay == 0) {
    DEBUG ((DEBUG_ERROR, "Port %d Device not ready (TFD=0x%X)\n", Port, Data));
    return EFI_TIMEOUT;
  } else {
    return EFI_SUCCESS;
  }
}

/**
  Reset the SATA port. Algorithm follows AHCI spec 1.3.1 section 10.4.2

  @param[in] PciIo    Pointer to AHCI controller PciIo.
  @param[in] Port     SATA port index on which to reset.

  @retval EFI_SUCCESS  Port reset.
  @retval Others       Failed to reset the port.
**/
EFI_STATUS
AhciResetPort (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT8                Port
  )
{
  UINT32      Offset;
  EFI_STATUS  Status;

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SCTL;
  AhciOrReg (PciIo, Offset, EFI_AHCI_PORT_SCTL_DET_INIT);
  //
  // SW is required to keep DET set to 0x1 at least for 1 milisecond to ensure that
  // at least one COMRESET signal is sent.
  //
  MicroSecondDelay (1000);
  AhciAndReg (PciIo, Offset, ~(UINT32)EFI_AHCI_PORT_SSTS_DET_MASK);

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
  Status = AhciWaitMmioSet (PciIo, Offset, EFI_AHCI_PORT_SSTS_DET_MASK, EFI_AHCI_PORT_SSTS_DET_PCE, ATA_ATAPI_TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return AhciWaitDeviceReady (PciIo, Port);
}

/**
  Recovers the SATA port from error condition.
  This function implements algorithm described in
  AHCI spec 1.3.1 section 6.2.2

  @param[in] PciIo    Pointer to AHCI controller PciIo.
  @param[in] Port     SATA port index on which to check.

  @retval EFI_SUCCESS  Port recovered.
  @retval Others       Failed to recover port.
**/
EFI_STATUS
AhciRecoverPortError (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT8                Port
  )
{
  UINT32      Offset;
  UINT32      PortInterrupt;
  UINT32      PortTfd;
  EFI_STATUS  Status;

  Offset        = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_IS;
  PortInterrupt = AhciReadReg (PciIo, Offset);
  if ((PortInterrupt & EFI_AHCI_PORT_IS_FATAL_ERROR_MASK) == 0) {
    //
    // No fatal error detected. Exit with success as port should still be operational.
    // No need to clear IS as it will be cleared when the next command starts.
    //
    return EFI_SUCCESS;
  }

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
  AhciAndReg (PciIo, Offset, ~(UINT32)EFI_AHCI_PORT_CMD_ST);

  Status = AhciWaitMmioSet (PciIo, Offset, EFI_AHCI_PORT_CMD_CR, 0, ATA_ATAPI_TIMEOUT);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Ahci port %d is in hung state, aborting recovery\n", Port));
    return Status;
  }

  //
  // If TFD.BSY or TFD.DRQ is still set it means that drive is hung and software has
  // to reset it before sending any additional commands.
  //
  Offset  = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_TFD;
  PortTfd = AhciReadReg (PciIo, Offset);
  if ((PortTfd & (EFI_AHCI_PORT_TFD_BSY | EFI_AHCI_PORT_TFD_DRQ)) != 0) {
    Status = AhciResetPort (PciIo, Port);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to reset the port %d\n", Port));
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}

/**
  This function will check if the failed command should be retired. Only error
  conditions which are a result of transient conditions on a link(either to system or to device).

  @param[in] PciIo    Pointer to AHCI controller PciIo.
  @param[in] Port     SATA port index on which to check.

  @retval TRUE   Command failure was caused by transient condition and should be retried
  @retval FALSE  Command should not be retried
**/
BOOLEAN
AhciShouldCmdBeRetried (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT8                Port
  )
{
  UINT32  Offset;
  UINT32  PortInterrupt;
  UINT32  Serr;
  UINT32  Tfd;

  Offset        = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_IS;
  PortInterrupt = AhciReadReg (PciIo, Offset);
  Offset        = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SERR;
  Serr          = AhciReadReg (PciIo, Offset);
  Offset        = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_TFD;
  Tfd           = AhciReadReg (PciIo, Offset);

  //
  // This can occur if there was a CRC error on a path from system memory to
  // host controller.
  //
  if (PortInterrupt & EFI_AHCI_PORT_IS_HBDS) {
    return TRUE;
    //
    // This can occur if there was a CRC error detected by host during communication
    // with the device
    //
  } else if ((PortInterrupt & (EFI_AHCI_PORT_IS_IFS | EFI_AHCI_PORT_IS_INFS)) &&
             (Serr & EFI_AHCI_PORT_SERR_CRCE))
  {
    return TRUE;
    //
    // This can occur if there was a CRC error detected by device during communication
    // with the host. Device returns error status to host with D2H FIS.
    //
  } else if ((PortInterrupt & EFI_AHCI_PORT_IS_TFES) &&
             (Tfd & EFI_AHCI_PORT_TFD_ERR_INT_CRC))
  {
    return TRUE;
  }

  return FALSE;
}

/**
  Checks if specified FIS has been received.

  @param[in] PciIo    Pointer to AHCI controller PciIo.
  @param[in] Port     SATA port index on which to check.
  @param[in] FisType  FIS type for which to check.

  @retval EFI_SUCCESS       FIS received.
  @retval EFI_NOT_READY     FIS not received yet.
  @retval EFI_DEVICE_ERROR  AHCI controller reported an error on port.
**/
EFI_STATUS
AhciCheckFisReceived (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT8                Port,
  IN SATA_FIS_TYPE        FisType
  )
{
  UINT32  Offset;
  UINT32  PortInterrupt;
  UINT32  PortTfd;

  Offset        = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_IS;
  PortInterrupt = AhciReadReg (PciIo, Offset);
  if ((PortInterrupt & EFI_AHCI_PORT_IS_ERROR_MASK) != 0) {
    DEBUG ((DEBUG_ERROR, "AHCI: Error interrupt reported PxIS: %X\n", PortInterrupt));
    return EFI_DEVICE_ERROR;
  }

  //
  // For PIO setup FIS - According to SATA 2.6 spec section 11.7, D2h FIS means an error encountered.
  // But Qemu and Marvel 9230 sata controller may just receive a D2h FIS from device
  // after the transaction is finished successfully.
  // To get better device compatibilities, we further check if the PxTFD's ERR bit is set.
  // By this way, we can know if there is a real error happened.
  //
  if (((FisType == SataFisD2H) && ((PortInterrupt & EFI_AHCI_PORT_IS_DHRS) != 0)) ||
      ((FisType == SataFisPioSetup) && ((PortInterrupt & (EFI_AHCI_PORT_IS_PSS | EFI_AHCI_PORT_IS_DHRS)) != 0)) ||
      ((FisType == SataFisDmaSetup) && ((PortInterrupt & (EFI_AHCI_PORT_IS_DSS | EFI_AHCI_PORT_IS_DHRS)) != 0)))
  {
    Offset  = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_TFD;
    PortTfd = AhciReadReg (PciIo, (UINT32)Offset);
    if ((PortTfd & EFI_AHCI_PORT_TFD_ERR) != 0) {
      return EFI_DEVICE_ERROR;
    } else {
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_READY;
}

/**
  Waits until specified FIS has been received.

  @param[in] PciIo    Pointer to AHCI controller PciIo.
  @param[in] Port     SATA port index on which to check.
  @param[in] Timeout  Time after which function should stop polling.
  @param[in] FisType  FIS type for which to check.

  @retval EFI_SUCCESS       FIS received.
  @retval EFI_TIMEOUT       FIS failed to arrive within a specified time period.
  @retval EFI_DEVICE_ERROR  AHCI controller reported an error on port.
**/
EFI_STATUS
AhciWaitUntilFisReceived (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT8                Port,
  IN UINT64               Timeout,
  IN SATA_FIS_TYPE        FisType
  )
{
  EFI_STATUS  Status;
  BOOLEAN     InfiniteWait;
  UINT64      Delay;

  Delay =  DivU64x32 (Timeout, 1000) + 1;
  if (Timeout == 0) {
    InfiniteWait = TRUE;
  } else {
    InfiniteWait = FALSE;
  }

  do {
    Status = AhciCheckFisReceived (PciIo, Port, FisType);
    if (Status != EFI_NOT_READY) {
      return Status;
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
  Prints contents of the ATA command block into the debug port.

  @param[in] AtaCommandBlock  AtaCommandBlock to print.
  @param[in] DebugLevel       Debug level on which to print.
**/
VOID
AhciPrintCommandBlock (
  IN EFI_ATA_COMMAND_BLOCK  *AtaCommandBlock,
  IN UINT32                 DebugLevel
  )
{
  DEBUG ((DebugLevel, "ATA COMMAND BLOCK:\n"));
  DEBUG ((DebugLevel, "AtaCommand: %d\n", AtaCommandBlock->AtaCommand));
  DEBUG ((DebugLevel, "AtaFeatures: %X\n", AtaCommandBlock->AtaFeatures));
  DEBUG ((DebugLevel, "AtaSectorNumber: %d\n", AtaCommandBlock->AtaSectorNumber));
  DEBUG ((DebugLevel, "AtaCylinderLow: %X\n", AtaCommandBlock->AtaCylinderHigh));
  DEBUG ((DebugLevel, "AtaCylinderHigh: %X\n", AtaCommandBlock->AtaCylinderHigh));
  DEBUG ((DebugLevel, "AtaDeviceHead: %d\n", AtaCommandBlock->AtaDeviceHead));
  DEBUG ((DebugLevel, "AtaSectorNumberExp: %d\n", AtaCommandBlock->AtaSectorNumberExp));
  DEBUG ((DebugLevel, "AtaCylinderLowExp: %X\n", AtaCommandBlock->AtaCylinderLowExp));
  DEBUG ((DebugLevel, "AtaCylinderHighExp: %X\n", AtaCommandBlock->AtaCylinderHighExp));
  DEBUG ((DebugLevel, "AtaFeaturesExp: %X\n", AtaCommandBlock->AtaFeaturesExp));
  DEBUG ((DebugLevel, "AtaSectorCount: %d\n", AtaCommandBlock->AtaSectorCount));
  DEBUG ((DebugLevel, "AtaSectorCountExp: %d\n", AtaCommandBlock->AtaSectorCountExp));
}

/**
  Prints contents of the ATA status block into the debug port.

  @param[in] AtaStatusBlock   AtaStatusBlock to print.
  @param[in] DebugLevel       Debug level on which to print.
**/
VOID
AhciPrintStatusBlock (
  IN EFI_ATA_STATUS_BLOCK  *AtaStatusBlock,
  IN UINT32                DebugLevel
  )
{
  //
  // Skip NULL pointer
  //
  if (AtaStatusBlock == NULL) {
    return;
  }

  //
  // Only print status and error since we have all of the rest printed as
  // a part of command block print.
  //
  DEBUG ((DebugLevel, "ATA STATUS BLOCK:\n"));
  DEBUG ((DebugLevel, "AtaStatus: %d\n", AtaStatusBlock->AtaStatus));
  DEBUG ((DebugLevel, "AtaError: %d\n", AtaStatusBlock->AtaError));
}

/**
  Start a PIO data transfer on specific port.

  @param[in]       PciIo               The PCI IO protocol instance.
  @param[in]       AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param[in]       Port                The number of port.
  @param[in]       PortMultiplier      The timeout value of stop.
  @param[in]       AtapiCommand        The atapi command will be used for the
                                       transfer.
  @param[in]       AtapiCommandLength  The length of the atapi command.
  @param[in]       Read                The transfer direction.
  @param[in]       AtaCommandBlock     The EFI_ATA_COMMAND_BLOCK data.
  @param[in, out]  AtaStatusBlock      The EFI_ATA_STATUS_BLOCK data.
  @param[in, out]  MemoryAddr          The pointer to the data buffer.
  @param[in]       DataCount           The data count to be transferred.
  @param[in]       Timeout             The timeout value of non data transfer, uses 100ns as a unit.
  @param[in]       Task                Optional. Pointer to the ATA_NONBLOCK_TASK
                                       used by non-blocking mode.

  @retval EFI_DEVICE_ERROR    The PIO data transfer abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for transfer.
  @retval EFI_SUCCESS         The PIO data transfer executes successfully.

**/
EFI_STATUS
EFIAPI
AhciPioTransfer (
  IN     EFI_PCI_IO_PROTOCOL     *PciIo,
  IN     EFI_AHCI_REGISTERS      *AhciRegisters,
  IN     UINT8                   Port,
  IN     UINT8                   PortMultiplier,
  IN     EFI_AHCI_ATAPI_COMMAND  *AtapiCommand OPTIONAL,
  IN     UINT8                   AtapiCommandLength,
  IN     BOOLEAN                 Read,
  IN     EFI_ATA_COMMAND_BLOCK   *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK    *AtaStatusBlock,
  IN OUT VOID                    *MemoryAddr,
  IN     UINT32                  DataCount,
  IN     UINT64                  Timeout,
  IN     ATA_NONBLOCK_TASK       *Task
  )
{
  EFI_STATUS                     Status;
  EFI_PHYSICAL_ADDRESS           PhyAddr;
  VOID                           *Map;
  UINTN                          MapLength;
  EFI_PCI_IO_PROTOCOL_OPERATION  Flag;
  EFI_AHCI_COMMAND_FIS           CFis;
  EFI_AHCI_COMMAND_LIST          CmdList;
  UINT32                         PrdCount;
  UINT32                         Retry;
  EFI_STATUS                     RecoveryStatus;
  BOOLEAN                        DoRetry;

  if (Read) {
    Flag = EfiPciIoOperationBusMasterWrite;
  } else {
    Flag = EfiPciIoOperationBusMasterRead;
  }

  //
  // construct command list and command table with pci bus address
  //
  MapLength = DataCount;
  Status    = PciIo->Map (
                       PciIo,
                       Flag,
                       MemoryAddr,
                       &MapLength,
                       &PhyAddr,
                       &Map
                       );

  if (EFI_ERROR (Status) || (DataCount != MapLength)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  //
  // Package read needed
  //
  AhciBuildCommandFis (&CFis, AtaCommandBlock);

  ZeroMem (&CmdList, sizeof (EFI_AHCI_COMMAND_LIST));

  CmdList.AhciCmdCfl = EFI_AHCI_FIS_REGISTER_H2D_LENGTH / 4;
  CmdList.AhciCmdW   = Read ? 0 : 1;

  for (Retry = 0; Retry < AHCI_COMMAND_RETRIES; Retry++) {
    AhciBuildCommand (
      PciIo,
      AhciRegisters,
      Port,
      PortMultiplier,
      &CFis,
      &CmdList,
      AtapiCommand,
      AtapiCommandLength,
      0,
      (VOID *)(UINTN)PhyAddr,
      DataCount
      );

    DEBUG ((DEBUG_VERBOSE, "Starting command for PIO transfer:\n"));
    AhciPrintCommandBlock (AtaCommandBlock, DEBUG_VERBOSE);
    Status = AhciStartCommand (
               PciIo,
               Port,
               0,
               Timeout
               );
    if (EFI_ERROR (Status)) {
      break;
    }

    if (Read && (AtapiCommand == 0)) {
      Status = AhciWaitUntilFisReceived (PciIo, Port, Timeout, SataFisPioSetup);
      if (Status == EFI_SUCCESS) {
        PrdCount = *(volatile UINT32 *)(&(AhciRegisters->AhciCmdList[0].AhciCmdPrdbc));
        if (PrdCount == DataCount) {
          Status = EFI_SUCCESS;
        } else {
          Status = EFI_DEVICE_ERROR;
        }
      }
    } else {
      Status = AhciWaitUntilFisReceived (PciIo, Port, Timeout, SataFisD2H);
    }

    if (Status == EFI_DEVICE_ERROR) {
      DEBUG ((DEBUG_ERROR, "PIO command failed at retry %d\n", Retry));
      DoRetry        = AhciShouldCmdBeRetried (PciIo, Port); // needs to be called before error recovery
      RecoveryStatus = AhciRecoverPortError (PciIo, Port);
      if (!DoRetry || EFI_ERROR (RecoveryStatus)) {
        break;
      }
    } else {
      break;
    }
  }

  AhciStopCommand (
    PciIo,
    Port,
    Timeout
    );

  AhciDisableFisReceive (
    PciIo,
    Port,
    Timeout
    );

  PciIo->Unmap (
           PciIo,
           Map
           );

  AhciDumpPortStatus (PciIo, AhciRegisters, Port, AtaStatusBlock);

  if (Status == EFI_DEVICE_ERROR) {
    DEBUG ((DEBUG_ERROR, "Failed to execute command for PIO transfer:\n"));
    //
    // Repeat command block here to make sure it is printed on
    // device error debug level.
    //
    AhciPrintCommandBlock (AtaCommandBlock, DEBUG_ERROR);
    AhciPrintStatusBlock (AtaStatusBlock, DEBUG_ERROR);
  } else {
    AhciPrintStatusBlock (AtaStatusBlock, DEBUG_VERBOSE);
  }

  return Status;
}

/**
  Start a DMA data transfer on specific port

  @param[in]       Instance            The ATA_ATAPI_PASS_THRU_INSTANCE protocol instance.
  @param[in]       AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param[in]       Port                The number of port.
  @param[in]       PortMultiplier      The timeout value of stop.
  @param[in]       AtapiCommand        The atapi command will be used for the
                                       transfer.
  @param[in]       AtapiCommandLength  The length of the atapi command.
  @param[in]       Read                The transfer direction.
  @param[in]       AtaCommandBlock     The EFI_ATA_COMMAND_BLOCK data.
  @param[in, out]  AtaStatusBlock      The EFI_ATA_STATUS_BLOCK data.
  @param[in, out]  MemoryAddr          The pointer to the data buffer.
  @param[in]       DataCount           The data count to be transferred.
  @param[in]       Timeout             The timeout value of non data transfer, uses 100ns as a unit.
  @param[in]       Task                Optional. Pointer to the ATA_NONBLOCK_TASK
                                       used by non-blocking mode.

  @retval EFI_DEVICE_ERROR    The DMA data transfer abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for transfer.
  @retval EFI_SUCCESS         The DMA data transfer executes successfully.

**/
EFI_STATUS
EFIAPI
AhciDmaTransfer (
  IN     ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN     EFI_AHCI_REGISTERS            *AhciRegisters,
  IN     UINT8                         Port,
  IN     UINT8                         PortMultiplier,
  IN     EFI_AHCI_ATAPI_COMMAND        *AtapiCommand OPTIONAL,
  IN     UINT8                         AtapiCommandLength,
  IN     BOOLEAN                       Read,
  IN     EFI_ATA_COMMAND_BLOCK         *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock,
  IN OUT VOID                          *MemoryAddr,
  IN     UINT32                        DataCount,
  IN     UINT64                        Timeout,
  IN     ATA_NONBLOCK_TASK             *Task
  )
{
  EFI_STATUS                     Status;
  EFI_PHYSICAL_ADDRESS           PhyAddr;
  VOID                           *Map;
  UINTN                          MapLength;
  EFI_PCI_IO_PROTOCOL_OPERATION  Flag;
  EFI_AHCI_COMMAND_FIS           CFis;
  EFI_AHCI_COMMAND_LIST          CmdList;
  EFI_PCI_IO_PROTOCOL            *PciIo;
  EFI_TPL                        OldTpl;
  UINT32                         Retry;
  EFI_STATUS                     RecoveryStatus;
  BOOLEAN                        DoRetry;

  Map   = NULL;
  PciIo = Instance->PciIo;

  if (PciIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Set Status to suppress incorrect compiler/analyzer warnings
  //
  Status = EFI_SUCCESS;

  //
  // DMA buffer allocation. Needs to be done only once for both sync and async
  // DMA transfers irrespective of number of retries.
  //
  if ((Task == NULL) || ((Task != NULL) && (Task->Map == NULL))) {
    if (Read) {
      Flag = EfiPciIoOperationBusMasterWrite;
    } else {
      Flag = EfiPciIoOperationBusMasterRead;
    }

    MapLength = DataCount;
    Status    = PciIo->Map (
                         PciIo,
                         Flag,
                         MemoryAddr,
                         &MapLength,
                         &PhyAddr,
                         &Map
                         );

    if (EFI_ERROR (Status) || (DataCount != MapLength)) {
      return EFI_BAD_BUFFER_SIZE;
    }

    if (Task != NULL) {
      Task->Map = Map;
    }
  }

  if ((Task == NULL) || ((Task != NULL) && !Task->IsStart)) {
    AhciBuildCommandFis (&CFis, AtaCommandBlock);

    ZeroMem (&CmdList, sizeof (EFI_AHCI_COMMAND_LIST));

    CmdList.AhciCmdCfl = EFI_AHCI_FIS_REGISTER_H2D_LENGTH / 4;
    CmdList.AhciCmdW   = Read ? 0 : 1;
  }

  if (Task == NULL) {
    //
    // Before starting the Blocking BlockIO operation, push to finish all non-blocking
    // BlockIO tasks.
    // Delay 100us to simulate the blocking time out checking.
    //
    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    while (!IsListEmpty (&Instance->NonBlockingTaskList)) {
      AsyncNonBlockingTransferRoutine (NULL, Instance);
      //
      // Stall for 100us.
      //
      MicroSecondDelay (100);
    }

    gBS->RestoreTPL (OldTpl);
    for (Retry = 0; Retry < AHCI_COMMAND_RETRIES; Retry++) {
      AhciBuildCommand (
        PciIo,
        AhciRegisters,
        Port,
        PortMultiplier,
        &CFis,
        &CmdList,
        AtapiCommand,
        AtapiCommandLength,
        0,
        (VOID *)(UINTN)PhyAddr,
        DataCount
        );

      DEBUG ((DEBUG_VERBOSE, "Starting command for sync DMA transfer:\n"));
      AhciPrintCommandBlock (AtaCommandBlock, DEBUG_VERBOSE);
      Status = AhciStartCommand (
                 PciIo,
                 Port,
                 0,
                 Timeout
                 );
      if (EFI_ERROR (Status)) {
        break;
      }

      Status = AhciWaitUntilFisReceived (PciIo, Port, Timeout, SataFisD2H);
      if (Status == EFI_DEVICE_ERROR) {
        DEBUG ((DEBUG_ERROR, "DMA command failed at retry: %d\n", Retry));
        DoRetry        = AhciShouldCmdBeRetried (PciIo, Port); // needs to be called before error recovery
        RecoveryStatus = AhciRecoverPortError (PciIo, Port);
        if (!DoRetry || EFI_ERROR (RecoveryStatus)) {
          break;
        }
      } else {
        break;
      }
    }
  } else {
    if (!Task->IsStart) {
      AhciBuildCommand (
        PciIo,
        AhciRegisters,
        Port,
        PortMultiplier,
        &CFis,
        &CmdList,
        AtapiCommand,
        AtapiCommandLength,
        0,
        (VOID *)(UINTN)PhyAddr,
        DataCount
        );

      DEBUG ((DEBUG_VERBOSE, "Starting command for async DMA transfer:\n"));
      AhciPrintCommandBlock (AtaCommandBlock, DEBUG_VERBOSE);
      Status = AhciStartCommand (
                 PciIo,
                 Port,
                 0,
                 Timeout
                 );
      if (!EFI_ERROR (Status)) {
        Task->IsStart = TRUE;
      }
    }

    if (Task->IsStart) {
      Status = AhciCheckFisReceived (PciIo, Port, SataFisD2H);
      if (Status == EFI_DEVICE_ERROR) {
        DEBUG ((DEBUG_ERROR, "DMA command failed at retry: %d\n", Task->RetryTimes));
        DoRetry        = AhciShouldCmdBeRetried (PciIo, Port); // call this before error recovery
        RecoveryStatus = AhciRecoverPortError (PciIo, Port);
        //
        // If recovery passed mark the Task as not started and change the status
        // to EFI_NOT_READY. This will make the higher level call this function again
        // and on next call the command will be re-issued due to IsStart being FALSE.
        // This also makes the next condition decrement the RetryTimes.
        //
        if (DoRetry && (RecoveryStatus == EFI_SUCCESS)) {
          Task->IsStart = FALSE;
          Status        = EFI_NOT_READY;
        }
      }

      if (Status == EFI_NOT_READY) {
        if (!Task->InfiniteWait && (Task->RetryTimes == 0)) {
          Status = EFI_TIMEOUT;
        } else {
          Task->RetryTimes--;
        }
      }
    }
  }

  //
  // For Blocking mode, the command should be stopped, the Fis should be disabled
  // and the PciIo should be unmapped.
  // For non-blocking mode, only when a error is happened (if the return status is
  // EFI_NOT_READY that means the command doesn't finished, try again.), first do the
  // context cleanup, then set the packet's Asb status.
  //
  if ((Task == NULL) ||
      ((Task != NULL) && (Status != EFI_NOT_READY))
      )
  {
    AhciStopCommand (
      PciIo,
      Port,
      Timeout
      );

    AhciDisableFisReceive (
      PciIo,
      Port,
      Timeout
      );

    PciIo->Unmap (
             PciIo,
             (Task != NULL) ? Task->Map : Map
             );

    if (Task != NULL) {
      Task->Packet->Asb->AtaStatus = 0x01;
    }
  }

  AhciDumpPortStatus (PciIo, AhciRegisters, Port, AtaStatusBlock);

  if (Status == EFI_DEVICE_ERROR) {
    DEBUG ((DEBUG_ERROR, "Failed to execute command for DMA transfer:\n"));
    //
    // Repeat command block here to make sure it is printed on
    // device error debug level.
    //
    AhciPrintCommandBlock (AtaCommandBlock, DEBUG_ERROR);
    AhciPrintStatusBlock (AtaStatusBlock, DEBUG_ERROR);
  } else {
    AhciPrintStatusBlock (AtaStatusBlock, DEBUG_VERBOSE);
  }

  return Status;
}

/**
  Start a non data transfer on specific port.

  @param[in]       PciIo               The PCI IO protocol instance.
  @param[in]       AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param[in]       Port                The number of port.
  @param[in]       PortMultiplier      The timeout value of stop.
  @param[in]       AtapiCommand        The atapi command will be used for the
                                       transfer.
  @param[in]       AtapiCommandLength  The length of the atapi command.
  @param[in]       AtaCommandBlock     The EFI_ATA_COMMAND_BLOCK data.
  @param[in, out]  AtaStatusBlock      The EFI_ATA_STATUS_BLOCK data.
  @param[in]       Timeout             The timeout value of non data transfer, uses 100ns as a unit.
  @param[in]       Task                Optional. Pointer to the ATA_NONBLOCK_TASK
                                       used by non-blocking mode.

  @retval EFI_DEVICE_ERROR    The non data transfer abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for transfer.
  @retval EFI_SUCCESS         The non data transfer executes successfully.

**/
EFI_STATUS
EFIAPI
AhciNonDataTransfer (
  IN     EFI_PCI_IO_PROTOCOL     *PciIo,
  IN     EFI_AHCI_REGISTERS      *AhciRegisters,
  IN     UINT8                   Port,
  IN     UINT8                   PortMultiplier,
  IN     EFI_AHCI_ATAPI_COMMAND  *AtapiCommand OPTIONAL,
  IN     UINT8                   AtapiCommandLength,
  IN     EFI_ATA_COMMAND_BLOCK   *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK    *AtaStatusBlock,
  IN     UINT64                  Timeout,
  IN     ATA_NONBLOCK_TASK       *Task
  )
{
  EFI_STATUS             Status;
  EFI_AHCI_COMMAND_FIS   CFis;
  EFI_AHCI_COMMAND_LIST  CmdList;
  UINT32                 Retry;
  EFI_STATUS             RecoveryStatus;
  BOOLEAN                DoRetry;

  //
  // Package read needed
  //
  AhciBuildCommandFis (&CFis, AtaCommandBlock);

  ZeroMem (&CmdList, sizeof (EFI_AHCI_COMMAND_LIST));

  CmdList.AhciCmdCfl = EFI_AHCI_FIS_REGISTER_H2D_LENGTH / 4;

  for (Retry = 0; Retry < AHCI_COMMAND_RETRIES; Retry++) {
    AhciBuildCommand (
      PciIo,
      AhciRegisters,
      Port,
      PortMultiplier,
      &CFis,
      &CmdList,
      AtapiCommand,
      AtapiCommandLength,
      0,
      NULL,
      0
      );

    DEBUG ((DEBUG_VERBOSE, "Starting command for non data transfer:\n"));
    AhciPrintCommandBlock (AtaCommandBlock, DEBUG_VERBOSE);
    Status = AhciStartCommand (
               PciIo,
               Port,
               0,
               Timeout
               );
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = AhciWaitUntilFisReceived (PciIo, Port, Timeout, SataFisD2H);
    if (Status == EFI_DEVICE_ERROR) {
      DEBUG ((DEBUG_ERROR, "Non data transfer failed at retry %d\n", Retry));
      DoRetry        = AhciShouldCmdBeRetried (PciIo, Port); // call this before error recovery
      RecoveryStatus = AhciRecoverPortError (PciIo, Port);
      if (!DoRetry || EFI_ERROR (RecoveryStatus)) {
        break;
      }
    } else {
      break;
    }
  }

  AhciStopCommand (
    PciIo,
    Port,
    Timeout
    );

  AhciDisableFisReceive (
    PciIo,
    Port,
    Timeout
    );

  AhciDumpPortStatus (PciIo, AhciRegisters, Port, AtaStatusBlock);

  if (Status == EFI_DEVICE_ERROR) {
    DEBUG ((DEBUG_ERROR, "Failed to execute command for non data transfer:\n"));
    //
    // Repeat command block here to make sure it is printed on
    // device error debug level.
    //
    AhciPrintCommandBlock (AtaCommandBlock, DEBUG_ERROR);
    AhciPrintStatusBlock (AtaStatusBlock, DEBUG_ERROR);
  } else {
    AhciPrintStatusBlock (AtaStatusBlock, DEBUG_VERBOSE);
  }

  return Status;
}

/**
  Stop command running for giving port

  @param  PciIo              The PCI IO protocol instance.
  @param  Port               The number of port.
  @param  Timeout            The timeout value of stop, uses 100ns as a unit.

  @retval EFI_DEVICE_ERROR   The command stop unsuccessfully.
  @retval EFI_TIMEOUT        The operation is time out.
  @retval EFI_SUCCESS        The command stop successfully.

**/
EFI_STATUS
EFIAPI
AhciStopCommand (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINT8                Port,
  IN  UINT64               Timeout
  )
{
  UINT32  Offset;
  UINT32  Data;

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
  Data   = AhciReadReg (PciIo, Offset);

  if ((Data & (EFI_AHCI_PORT_CMD_ST |  EFI_AHCI_PORT_CMD_CR)) == 0) {
    return EFI_SUCCESS;
  }

  if ((Data & EFI_AHCI_PORT_CMD_ST) != 0) {
    AhciAndReg (PciIo, Offset, (UINT32) ~(EFI_AHCI_PORT_CMD_ST));
  }

  return AhciWaitMmioSet (
           PciIo,
           Offset,
           EFI_AHCI_PORT_CMD_CR,
           0,
           Timeout
           );
}

/**
  Start command for give slot on specific port.

  @param  PciIo              The PCI IO protocol instance.
  @param  Port               The number of port.
  @param  CommandSlot        The number of Command Slot.
  @param  Timeout            The timeout value of start, uses 100ns as a unit.

  @retval EFI_DEVICE_ERROR   The command start unsuccessfully.
  @retval EFI_TIMEOUT        The operation is time out.
  @retval EFI_SUCCESS        The command start successfully.

**/
EFI_STATUS
EFIAPI
AhciStartCommand (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINT8                Port,
  IN  UINT8                CommandSlot,
  IN  UINT64               Timeout
  )
{
  UINT32      CmdSlotBit;
  EFI_STATUS  Status;
  UINT32      PortStatus;
  UINT32      StartCmd;
  UINT32      PortTfd;
  UINT32      Offset;
  UINT32      Capability;

  //
  // Collect AHCI controller information
  //
  Capability = AhciReadReg (PciIo, EFI_AHCI_CAPABILITY_OFFSET);

  CmdSlotBit = (UINT32)(1 << CommandSlot);

  AhciClearPortStatus (
    PciIo,
    Port
    );

  Status = AhciEnableFisReceive (
             PciIo,
             Port,
             Timeout
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Offset     = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
  PortStatus = AhciReadReg (PciIo, Offset);

  StartCmd = 0;
  if ((PortStatus & EFI_AHCI_PORT_CMD_ALPE) != 0) {
    StartCmd  = AhciReadReg (PciIo, Offset);
    StartCmd &= ~EFI_AHCI_PORT_CMD_ICC_MASK;
    StartCmd |= EFI_AHCI_PORT_CMD_ACTIVE;
  }

  Offset  = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_TFD;
  PortTfd = AhciReadReg (PciIo, Offset);

  if ((PortTfd & (EFI_AHCI_PORT_TFD_BSY | EFI_AHCI_PORT_TFD_DRQ)) != 0) {
    if ((Capability & BIT24) != 0) {
      Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
      AhciOrReg (PciIo, Offset, EFI_AHCI_PORT_CMD_CLO);

      AhciWaitMmioSet (
        PciIo,
        Offset,
        EFI_AHCI_PORT_CMD_CLO,
        0,
        Timeout
        );
    }
  }

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
  AhciOrReg (PciIo, Offset, EFI_AHCI_PORT_CMD_ST | StartCmd);

  //
  // Setting the command
  //
  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CI;
  AhciAndReg (PciIo, Offset, 0);
  AhciOrReg (PciIo, Offset, CmdSlotBit);

  return EFI_SUCCESS;
}

/**
  Do AHCI HBA reset.

  @param  PciIo              The PCI IO protocol instance.
  @param  Timeout            The timeout value of reset, uses 100ns as a unit.

  @retval EFI_DEVICE_ERROR   AHCI controller is failed to complete hardware reset.
  @retval EFI_TIMEOUT        The reset operation is time out.
  @retval EFI_SUCCESS        AHCI controller is reset successfully.

**/
EFI_STATUS
EFIAPI
AhciReset (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINT64               Timeout
  )
{
  UINT64  Delay;
  UINT32  Value;

  //
  // Make sure that GHC.AE bit is set before accessing any AHCI registers.
  //
  Value = AhciReadReg (PciIo, EFI_AHCI_GHC_OFFSET);

  if ((Value & EFI_AHCI_GHC_ENABLE) == 0) {
    AhciOrReg (PciIo, EFI_AHCI_GHC_OFFSET, EFI_AHCI_GHC_ENABLE);
  }

  AhciOrReg (PciIo, EFI_AHCI_GHC_OFFSET, EFI_AHCI_GHC_RESET);

  Delay = DivU64x32 (Timeout, 1000) + 1;

  do {
    Value = AhciReadReg (PciIo, EFI_AHCI_GHC_OFFSET);

    if ((Value & EFI_AHCI_GHC_RESET) == 0) {
      break;
    }

    //
    // Stall for 100 microseconds.
    //
    MicroSecondDelay (100);

    Delay--;
  } while (Delay > 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Send SMART Return Status command to check if the execution of SMART cmd is successful or not.

  @param  PciIo               The PCI IO protocol instance.
  @param  AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param  Port                The number of port.
  @param  PortMultiplier      The port multiplier port number.
  @param  AtaStatusBlock      A pointer to EFI_ATA_STATUS_BLOCK data structure.

  @retval EFI_SUCCESS     Successfully get the return status of S.M.A.R.T command execution.
  @retval Others          Fail to get return status data.

**/
EFI_STATUS
EFIAPI
AhciAtaSmartReturnStatusCheck (
  IN EFI_PCI_IO_PROTOCOL       *PciIo,
  IN EFI_AHCI_REGISTERS        *AhciRegisters,
  IN UINT8                     Port,
  IN UINT8                     PortMultiplier,
  IN OUT EFI_ATA_STATUS_BLOCK  *AtaStatusBlock
  )
{
  EFI_STATUS             Status;
  EFI_ATA_COMMAND_BLOCK  AtaCommandBlock;
  UINT8                  LBAMid;
  UINT8                  LBAHigh;
  UINTN                  FisBaseAddr;
  UINT32                 Value;

  ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));

  AtaCommandBlock.AtaCommand      = ATA_CMD_SMART;
  AtaCommandBlock.AtaFeatures     = ATA_SMART_RETURN_STATUS;
  AtaCommandBlock.AtaCylinderLow  = ATA_CONSTANT_4F;
  AtaCommandBlock.AtaCylinderHigh = ATA_CONSTANT_C2;

  //
  // Send S.M.A.R.T Read Return Status command to device
  //
  Status = AhciNonDataTransfer (
             PciIo,
             AhciRegisters,
             (UINT8)Port,
             (UINT8)PortMultiplier,
             NULL,
             0,
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

  FisBaseAddr = (UINTN)AhciRegisters->AhciRFis + Port * sizeof (EFI_AHCI_RECEIVED_FIS);

  Value = *(UINT32 *)(FisBaseAddr + EFI_AHCI_D2H_FIS_OFFSET);

  if ((Value & EFI_AHCI_FIS_TYPE_MASK) == EFI_AHCI_FIS_REGISTER_D2H) {
    LBAMid  = ((UINT8 *)(UINTN)(FisBaseAddr + EFI_AHCI_D2H_FIS_OFFSET))[5];
    LBAHigh = ((UINT8 *)(UINTN)(FisBaseAddr + EFI_AHCI_D2H_FIS_OFFSET))[6];

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
  }

  return EFI_SUCCESS;
}

/**
  Enable SMART command of the disk if supported.

  @param  PciIo               The PCI IO protocol instance.
  @param  AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param  Port                The number of port.
  @param  PortMultiplier      The port multiplier port number.
  @param  IdentifyData        A pointer to data buffer which is used to contain IDENTIFY data.
  @param  AtaStatusBlock      A pointer to EFI_ATA_STATUS_BLOCK data structure.

**/
VOID
EFIAPI
AhciAtaSmartSupport (
  IN EFI_PCI_IO_PROTOCOL       *PciIo,
  IN EFI_AHCI_REGISTERS        *AhciRegisters,
  IN UINT8                     Port,
  IN UINT8                     PortMultiplier,
  IN EFI_IDENTIFY_DATA         *IdentifyData,
  IN OUT EFI_ATA_STATUS_BLOCK  *AtaStatusBlock
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
      "S.M.A.R.T feature is not supported at port [%d] PortMultiplier [%d]!\n",
      Port,
      PortMultiplier
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

      //
      // Send S.M.A.R.T Enable command to device
      //
      Status = AhciNonDataTransfer (
                 PciIo,
                 AhciRegisters,
                 (UINT8)Port,
                 (UINT8)PortMultiplier,
                 NULL,
                 0,
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

        Status = AhciNonDataTransfer (
                   PciIo,
                   AhciRegisters,
                   (UINT8)Port,
                   (UINT8)PortMultiplier,
                   NULL,
                   0,
                   &AtaCommandBlock,
                   AtaStatusBlock,
                   ATA_ATAPI_TIMEOUT,
                   NULL
                   );
      }
    }

    AhciAtaSmartReturnStatusCheck (
      PciIo,
      AhciRegisters,
      (UINT8)Port,
      (UINT8)PortMultiplier,
      AtaStatusBlock
      );

    DEBUG ((
      DEBUG_INFO,
      "Enabled S.M.A.R.T feature at port [%d] PortMultiplier [%d]!\n",
      Port,
      PortMultiplier
      ));
  }

  return;
}

/**
  Send Buffer cmd to specific device.

  @param  PciIo               The PCI IO protocol instance.
  @param  AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param  Port                The number of port.
  @param  PortMultiplier      The port multiplier port number.
  @param  Buffer              The data buffer to store IDENTIFY PACKET data.

  @retval EFI_DEVICE_ERROR    The cmd abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for executing.
  @retval EFI_SUCCESS         The cmd executes successfully.

**/
EFI_STATUS
EFIAPI
AhciIdentify (
  IN EFI_PCI_IO_PROTOCOL    *PciIo,
  IN EFI_AHCI_REGISTERS     *AhciRegisters,
  IN UINT8                  Port,
  IN UINT8                  PortMultiplier,
  IN OUT EFI_IDENTIFY_DATA  *Buffer
  )
{
  EFI_STATUS             Status;
  EFI_ATA_COMMAND_BLOCK  AtaCommandBlock;
  EFI_ATA_STATUS_BLOCK   AtaStatusBlock;

  if ((PciIo == NULL) || (AhciRegisters == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
  ZeroMem (&AtaStatusBlock, sizeof (EFI_ATA_STATUS_BLOCK));

  AtaCommandBlock.AtaCommand     = ATA_CMD_IDENTIFY_DRIVE;
  AtaCommandBlock.AtaSectorCount = 1;

  Status = AhciPioTransfer (
             PciIo,
             AhciRegisters,
             Port,
             PortMultiplier,
             NULL,
             0,
             TRUE,
             &AtaCommandBlock,
             &AtaStatusBlock,
             Buffer,
             sizeof (EFI_IDENTIFY_DATA),
             ATA_ATAPI_TIMEOUT,
             NULL
             );

  return Status;
}

/**
  Send Buffer cmd to specific device.

  @param  PciIo               The PCI IO protocol instance.
  @param  AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param  Port                The number of port.
  @param  PortMultiplier      The port multiplier port number.
  @param  Buffer              The data buffer to store IDENTIFY PACKET data.

  @retval EFI_DEVICE_ERROR    The cmd abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for executing.
  @retval EFI_SUCCESS         The cmd executes successfully.

**/
EFI_STATUS
EFIAPI
AhciIdentifyPacket (
  IN EFI_PCI_IO_PROTOCOL    *PciIo,
  IN EFI_AHCI_REGISTERS     *AhciRegisters,
  IN UINT8                  Port,
  IN UINT8                  PortMultiplier,
  IN OUT EFI_IDENTIFY_DATA  *Buffer
  )
{
  EFI_STATUS             Status;
  EFI_ATA_COMMAND_BLOCK  AtaCommandBlock;
  EFI_ATA_STATUS_BLOCK   AtaStatusBlock;

  if ((PciIo == NULL) || (AhciRegisters == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
  ZeroMem (&AtaStatusBlock, sizeof (EFI_ATA_STATUS_BLOCK));

  AtaCommandBlock.AtaCommand     = ATA_CMD_IDENTIFY_DEVICE;
  AtaCommandBlock.AtaSectorCount = 1;

  Status = AhciPioTransfer (
             PciIo,
             AhciRegisters,
             Port,
             PortMultiplier,
             NULL,
             0,
             TRUE,
             &AtaCommandBlock,
             &AtaStatusBlock,
             Buffer,
             sizeof (EFI_IDENTIFY_DATA),
             ATA_ATAPI_TIMEOUT,
             NULL
             );

  return Status;
}

/**
  Send SET FEATURE cmd on specific device.

  @param  PciIo               The PCI IO protocol instance.
  @param  AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param  Port                The number of port.
  @param  PortMultiplier      The port multiplier port number.
  @param  Feature             The data to send Feature register.
  @param  FeatureSpecificData The specific data for SET FEATURE cmd.
  @param  Timeout             The timeout value of SET FEATURE cmd, uses 100ns as a unit.

  @retval EFI_DEVICE_ERROR    The cmd abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for executing.
  @retval EFI_SUCCESS         The cmd executes successfully.

**/
EFI_STATUS
EFIAPI
AhciDeviceSetFeature (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN EFI_AHCI_REGISTERS   *AhciRegisters,
  IN UINT8                Port,
  IN UINT8                PortMultiplier,
  IN UINT16               Feature,
  IN UINT32               FeatureSpecificData,
  IN UINT64               Timeout
  )
{
  EFI_STATUS             Status;
  EFI_ATA_COMMAND_BLOCK  AtaCommandBlock;
  EFI_ATA_STATUS_BLOCK   AtaStatusBlock;

  ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
  ZeroMem (&AtaStatusBlock, sizeof (EFI_ATA_STATUS_BLOCK));

  AtaCommandBlock.AtaCommand      = ATA_CMD_SET_FEATURES;
  AtaCommandBlock.AtaFeatures     = (UINT8)Feature;
  AtaCommandBlock.AtaFeaturesExp  = (UINT8)(Feature >> 8);
  AtaCommandBlock.AtaSectorCount  = (UINT8)FeatureSpecificData;
  AtaCommandBlock.AtaSectorNumber = (UINT8)(FeatureSpecificData >> 8);
  AtaCommandBlock.AtaCylinderLow  = (UINT8)(FeatureSpecificData >> 16);
  AtaCommandBlock.AtaCylinderHigh = (UINT8)(FeatureSpecificData >> 24);

  Status = AhciNonDataTransfer (
             PciIo,
             AhciRegisters,
             (UINT8)Port,
             (UINT8)PortMultiplier,
             NULL,
             0,
             &AtaCommandBlock,
             &AtaStatusBlock,
             Timeout,
             NULL
             );

  return Status;
}

/**
  This function is used to send out ATAPI commands conforms to the Packet Command
  with PIO Protocol.

  @param PciIo              The PCI IO protocol instance.
  @param AhciRegisters      The pointer to the EFI_AHCI_REGISTERS.
  @param Port               The number of port.
  @param PortMultiplier     The number of port multiplier.
  @param Packet             A pointer to EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET structure.

  @retval EFI_SUCCESS       send out the ATAPI packet command successfully
                            and device sends data successfully.
  @retval EFI_DEVICE_ERROR  the device failed to send data.

**/
EFI_STATUS
EFIAPI
AhciPacketCommandExecute (
  IN  EFI_PCI_IO_PROTOCOL                         *PciIo,
  IN  EFI_AHCI_REGISTERS                          *AhciRegisters,
  IN  UINT8                                       Port,
  IN  UINT8                                       PortMultiplier,
  IN  EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *Packet
  )
{
  EFI_STATUS             Status;
  VOID                   *Buffer;
  UINT32                 Length;
  EFI_ATA_COMMAND_BLOCK  AtaCommandBlock;
  EFI_ATA_STATUS_BLOCK   AtaStatusBlock;
  BOOLEAN                Read;

  if ((Packet == NULL) || (Packet->Cdb == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
  ZeroMem (&AtaStatusBlock, sizeof (EFI_ATA_STATUS_BLOCK));
  AtaCommandBlock.AtaCommand = ATA_CMD_PACKET;
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

  if (Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_READ) {
    Buffer = Packet->InDataBuffer;
    Length = Packet->InTransferLength;
    Read   = TRUE;
  } else {
    Buffer = Packet->OutDataBuffer;
    Length = Packet->OutTransferLength;
    Read   = FALSE;
  }

  if (Length == 0) {
    Status = AhciNonDataTransfer (
               PciIo,
               AhciRegisters,
               Port,
               PortMultiplier,
               Packet->Cdb,
               Packet->CdbLength,
               &AtaCommandBlock,
               &AtaStatusBlock,
               Packet->Timeout,
               NULL
               );
  } else {
    Status = AhciPioTransfer (
               PciIo,
               AhciRegisters,
               Port,
               PortMultiplier,
               Packet->Cdb,
               Packet->CdbLength,
               Read,
               &AtaCommandBlock,
               &AtaStatusBlock,
               Buffer,
               Length,
               Packet->Timeout,
               NULL
               );
  }

  return Status;
}

/**
  Allocate transfer-related data struct which is used at AHCI mode.

  @param  PciIo                 The PCI IO protocol instance.
  @param  AhciRegisters         The pointer to the EFI_AHCI_REGISTERS.

**/
EFI_STATUS
EFIAPI
AhciCreateTransferDescriptor (
  IN     EFI_PCI_IO_PROTOCOL  *PciIo,
  IN OUT EFI_AHCI_REGISTERS   *AhciRegisters
  )
{
  EFI_STATUS  Status;
  UINTN       Bytes;
  VOID        *Buffer;

  UINT32                Capability;
  UINT32                PortImplementBitMap;
  UINT8                 MaxPortNumber;
  UINT8                 MaxCommandSlotNumber;
  BOOLEAN               Support64Bit;
  UINT64                MaxReceiveFisSize;
  UINT64                MaxCommandListSize;
  UINT64                MaxCommandTableSize;
  EFI_PHYSICAL_ADDRESS  AhciRFisPciAddr;
  EFI_PHYSICAL_ADDRESS  AhciCmdListPciAddr;
  EFI_PHYSICAL_ADDRESS  AhciCommandTablePciAddr;

  Buffer = NULL;
  //
  // Collect AHCI controller information
  //
  Capability = AhciReadReg (PciIo, EFI_AHCI_CAPABILITY_OFFSET);
  //
  // Get the number of command slots per port supported by this HBA.
  //
  MaxCommandSlotNumber = (UINT8)(((Capability & 0x1F00) >> 8) + 1);
  Support64Bit         = (BOOLEAN)(((Capability & BIT31) != 0) ? TRUE : FALSE);

  PortImplementBitMap = AhciReadReg (PciIo, EFI_AHCI_PI_OFFSET);
  //
  // Get the highest bit of implemented ports which decides how many bytes are allocated for received FIS.
  //
  MaxPortNumber = (UINT8)(UINTN)(HighBitSet32 (PortImplementBitMap) + 1);
  if (MaxPortNumber == 0) {
    return EFI_DEVICE_ERROR;
  }

  MaxReceiveFisSize = MaxPortNumber * sizeof (EFI_AHCI_RECEIVED_FIS);
  Status            = PciIo->AllocateBuffer (
                               PciIo,
                               AllocateAnyPages,
                               EfiBootServicesData,
                               EFI_SIZE_TO_PAGES ((UINTN)MaxReceiveFisSize),
                               &Buffer,
                               0
                               );

  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (Buffer, (UINTN)MaxReceiveFisSize);

  AhciRegisters->AhciRFis          = Buffer;
  AhciRegisters->MaxReceiveFisSize = MaxReceiveFisSize;
  Bytes                            = (UINTN)MaxReceiveFisSize;

  Status = PciIo->Map (
                    PciIo,
                    EfiPciIoOperationBusMasterCommonBuffer,
                    Buffer,
                    &Bytes,
                    &AhciRFisPciAddr,
                    &AhciRegisters->MapRFis
                    );

  if (EFI_ERROR (Status) || (Bytes != MaxReceiveFisSize)) {
    //
    // Map error or unable to map the whole RFis buffer into a contiguous region.
    //
    Status = EFI_OUT_OF_RESOURCES;
    goto Error6;
  }

  if ((!Support64Bit) && (AhciRFisPciAddr > 0x100000000ULL)) {
    //
    // The AHCI HBA doesn't support 64bit addressing, so should not get a >4G pci bus master address.
    //
    Status = EFI_DEVICE_ERROR;
    goto Error5;
  }

  AhciRegisters->AhciRFisPciAddr = (EFI_AHCI_RECEIVED_FIS *)(UINTN)AhciRFisPciAddr;

  //
  // Allocate memory for command list
  // Note that the implementation is a single task model which only use a command list for all ports.
  //
  Buffer             = NULL;
  MaxCommandListSize = MaxCommandSlotNumber * sizeof (EFI_AHCI_COMMAND_LIST);
  Status             = PciIo->AllocateBuffer (
                                PciIo,
                                AllocateAnyPages,
                                EfiBootServicesData,
                                EFI_SIZE_TO_PAGES ((UINTN)MaxCommandListSize),
                                &Buffer,
                                0
                                );

  if (EFI_ERROR (Status)) {
    //
    // Free mapped resource.
    //
    Status = EFI_OUT_OF_RESOURCES;
    goto Error5;
  }

  ZeroMem (Buffer, (UINTN)MaxCommandListSize);

  AhciRegisters->AhciCmdList        = Buffer;
  AhciRegisters->MaxCommandListSize = MaxCommandListSize;
  Bytes                             = (UINTN)MaxCommandListSize;

  Status = PciIo->Map (
                    PciIo,
                    EfiPciIoOperationBusMasterCommonBuffer,
                    Buffer,
                    &Bytes,
                    &AhciCmdListPciAddr,
                    &AhciRegisters->MapCmdList
                    );

  if (EFI_ERROR (Status) || (Bytes != MaxCommandListSize)) {
    //
    // Map error or unable to map the whole cmd list buffer into a contiguous region.
    //
    Status = EFI_OUT_OF_RESOURCES;
    goto Error4;
  }

  if ((!Support64Bit) && (AhciCmdListPciAddr > 0x100000000ULL)) {
    //
    // The AHCI HBA doesn't support 64bit addressing, so should not get a >4G pci bus master address.
    //
    Status = EFI_DEVICE_ERROR;
    goto Error3;
  }

  AhciRegisters->AhciCmdListPciAddr = (EFI_AHCI_COMMAND_LIST *)(UINTN)AhciCmdListPciAddr;

  //
  // Allocate memory for command table
  // According to AHCI 1.3 spec, a PRD table can contain maximum 65535 entries.
  //
  Buffer              = NULL;
  MaxCommandTableSize = sizeof (EFI_AHCI_COMMAND_TABLE);

  Status = PciIo->AllocateBuffer (
                    PciIo,
                    AllocateAnyPages,
                    EfiBootServicesData,
                    EFI_SIZE_TO_PAGES ((UINTN)MaxCommandTableSize),
                    &Buffer,
                    0
                    );

  if (EFI_ERROR (Status)) {
    //
    // Free mapped resource.
    //
    Status = EFI_OUT_OF_RESOURCES;
    goto Error3;
  }

  ZeroMem (Buffer, (UINTN)MaxCommandTableSize);

  AhciRegisters->AhciCommandTable    = Buffer;
  AhciRegisters->MaxCommandTableSize = MaxCommandTableSize;
  Bytes                              = (UINTN)MaxCommandTableSize;

  Status = PciIo->Map (
                    PciIo,
                    EfiPciIoOperationBusMasterCommonBuffer,
                    Buffer,
                    &Bytes,
                    &AhciCommandTablePciAddr,
                    &AhciRegisters->MapCommandTable
                    );

  if (EFI_ERROR (Status) || (Bytes != MaxCommandTableSize)) {
    //
    // Map error or unable to map the whole cmd list buffer into a contiguous region.
    //
    Status = EFI_OUT_OF_RESOURCES;
    goto Error2;
  }

  if ((!Support64Bit) && (AhciCommandTablePciAddr > 0x100000000ULL)) {
    //
    // The AHCI HBA doesn't support 64bit addressing, so should not get a >4G pci bus master address.
    //
    Status = EFI_DEVICE_ERROR;
    goto Error1;
  }

  AhciRegisters->AhciCommandTablePciAddr = (EFI_AHCI_COMMAND_TABLE *)(UINTN)AhciCommandTablePciAddr;

  return EFI_SUCCESS;
  //
  // Map error or unable to map the whole CmdList buffer into a contiguous region.
  //
Error1:
  PciIo->Unmap (
           PciIo,
           AhciRegisters->MapCommandTable
           );
Error2:
  PciIo->FreeBuffer (
           PciIo,
           EFI_SIZE_TO_PAGES ((UINTN)MaxCommandTableSize),
           AhciRegisters->AhciCommandTable
           );
Error3:
  PciIo->Unmap (
           PciIo,
           AhciRegisters->MapCmdList
           );
Error4:
  PciIo->FreeBuffer (
           PciIo,
           EFI_SIZE_TO_PAGES ((UINTN)MaxCommandListSize),
           AhciRegisters->AhciCmdList
           );
Error5:
  PciIo->Unmap (
           PciIo,
           AhciRegisters->MapRFis
           );
Error6:
  PciIo->FreeBuffer (
           PciIo,
           EFI_SIZE_TO_PAGES ((UINTN)MaxReceiveFisSize),
           AhciRegisters->AhciRFis
           );

  return Status;
}

/**
  Read logs from SATA device.

  @param  PciIo               The PCI IO protocol instance.
  @param  AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param  Port                The number of port.
  @param  PortMultiplier      The multiplier of port.
  @param  Buffer              The data buffer to store SATA logs.
  @param  LogNumber           The address of the log.
  @param  PageNumber          The page number of the log.

  @retval EFI_INVALID_PARAMETER  PciIo, AhciRegisters or Buffer is NULL.
  @retval others                 Return status of AhciPioTransfer().
**/
EFI_STATUS
AhciReadLogExt (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN EFI_AHCI_REGISTERS   *AhciRegisters,
  IN UINT8                Port,
  IN UINT8                PortMultiplier,
  IN OUT UINT8            *Buffer,
  IN UINT8                LogNumber,
  IN UINT8                PageNumber
  )
{
  EFI_ATA_COMMAND_BLOCK  AtaCommandBlock;
  EFI_ATA_STATUS_BLOCK   AtaStatusBlock;

  if ((PciIo == NULL) || (AhciRegisters == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ///
  /// Read log from device
  ///
  ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
  ZeroMem (&AtaStatusBlock, sizeof (EFI_ATA_STATUS_BLOCK));
  ZeroMem (Buffer, 512);

  AtaCommandBlock.AtaCommand      = ATA_CMD_READ_LOG_EXT;
  AtaCommandBlock.AtaSectorCount  = 1;
  AtaCommandBlock.AtaSectorNumber = LogNumber;
  AtaCommandBlock.AtaCylinderLow  = PageNumber;

  return AhciPioTransfer (
           PciIo,
           AhciRegisters,
           Port,
           PortMultiplier,
           NULL,
           0,
           TRUE,
           &AtaCommandBlock,
           &AtaStatusBlock,
           Buffer,
           512,
           ATA_ATAPI_TIMEOUT,
           NULL
           );
}

/**
  Enable DEVSLP of the disk if supported.

  @param  PciIo               The PCI IO protocol instance.
  @param  AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param  Port                The number of port.
  @param  PortMultiplier      The multiplier of port.
  @param  IdentifyData        A pointer to data buffer which is used to contain IDENTIFY data.

  @retval EFI_SUCCESS         The DEVSLP is enabled per policy successfully.
  @retval EFI_UNSUPPORTED     The DEVSLP isn't supported by the controller/device and policy requires to enable it.
**/
EFI_STATUS
AhciEnableDevSlp (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN EFI_AHCI_REGISTERS   *AhciRegisters,
  IN UINT8                Port,
  IN UINT8                PortMultiplier,
  IN EFI_IDENTIFY_DATA    *IdentifyData
  )
{
  EFI_STATUS               Status;
  UINT32                   Offset;
  UINT32                   Capability2;
  UINT8                    LogData[512];
  DEVSLP_TIMING_VARIABLES  DevSlpTiming;
  UINT32                   PortCmd;
  UINT32                   PortDevSlp;

  if (mAtaAtapiPolicy->DeviceSleepEnable != 1) {
    return EFI_SUCCESS;
  }

  //
  // Do not enable DevSlp if DevSlp is not supported.
  //
  Capability2 = AhciReadReg (PciIo, AHCI_CAPABILITY2_OFFSET);
  DEBUG ((DEBUG_INFO, "AHCI CAPABILITY2 = %08x\n", Capability2));
  if ((Capability2 & AHCI_CAP2_SDS) == 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Do not enable DevSlp if DevSlp is not present
  // Do not enable DevSlp if Hot Plug or Mechanical Presence Switch is supported
  //
  Offset     = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH;
  PortCmd    = AhciReadReg (PciIo, Offset + EFI_AHCI_PORT_CMD);
  PortDevSlp = AhciReadReg (PciIo, Offset + AHCI_PORT_DEVSLP);
  DEBUG ((DEBUG_INFO, "Port CMD/DEVSLP = %08x / %08x\n", PortCmd, PortDevSlp));
  if (((PortDevSlp & AHCI_PORT_DEVSLP_DSP) == 0) ||
      ((PortCmd & (EFI_AHCI_PORT_CMD_HPCP | EFI_AHCI_PORT_CMD_MPSP)) != 0)
      )
  {
    return EFI_UNSUPPORTED;
  }

  //
  // Do not enable DevSlp if the device doesn't support DevSlp
  //
  DEBUG ((
    DEBUG_INFO,
    "IDENTIFY DEVICE: [77] = %04x, [78] = %04x, [79] = %04x\n",
    IdentifyData->AtaData.reserved_77,
    IdentifyData->AtaData.serial_ata_features_supported,
    IdentifyData->AtaData.serial_ata_features_enabled
    ));
  if ((IdentifyData->AtaData.serial_ata_features_supported & BIT8) == 0) {
    DEBUG ((
      DEBUG_INFO,
      "DevSlp feature is not supported for device at port [%d] PortMultiplier [%d]!\n",
      Port,
      PortMultiplier
      ));
    return EFI_UNSUPPORTED;
  }

  //
  // Enable DevSlp when it is not enabled.
  //
  if ((IdentifyData->AtaData.serial_ata_features_enabled & BIT8) != 0) {
    Status = AhciDeviceSetFeature (
               PciIo,
               AhciRegisters,
               Port,
               0,
               ATA_SUB_CMD_ENABLE_SATA_FEATURE,
               0x09,
               ATA_ATAPI_TIMEOUT
               );
    DEBUG ((
      DEBUG_INFO,
      "DevSlp set feature for device at port [%d] PortMultiplier [%d] - %r\n",
      Port,
      PortMultiplier,
      Status
      ));
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = AhciReadLogExt (PciIo, AhciRegisters, Port, PortMultiplier, LogData, 0x30, 0x08);

  //
  // Clear PxCMD.ST and PxDEVSLP.ADSE before updating PxDEVSLP.DITO and PxDEVSLP.MDAT.
  //
  AhciWriteReg (PciIo, Offset + EFI_AHCI_PORT_CMD, PortCmd & ~EFI_AHCI_PORT_CMD_ST);
  PortDevSlp &= ~AHCI_PORT_DEVSLP_ADSE;
  AhciWriteReg (PciIo, Offset + AHCI_PORT_DEVSLP, PortDevSlp);

  //
  // Set PxDEVSLP.DETO and PxDEVSLP.MDAT to 0.
  //
  PortDevSlp &= ~AHCI_PORT_DEVSLP_DETO_MASK;
  PortDevSlp &= ~AHCI_PORT_DEVSLP_MDAT_MASK;
  AhciWriteReg (PciIo, Offset + AHCI_PORT_DEVSLP, PortDevSlp);
  DEBUG ((DEBUG_INFO, "Read Log Ext at port [%d] PortMultiplier [%d] - %r\n", Port, PortMultiplier, Status));
  if (EFI_ERROR (Status)) {
    //
    // Assume DEVSLP TIMING VARIABLES is not supported if the Identify Device Data log (30h, 8) fails
    //
    ZeroMem (&DevSlpTiming, sizeof (DevSlpTiming));
  } else {
    CopyMem (&DevSlpTiming, &LogData[48], sizeof (DevSlpTiming));
    DEBUG ((
      DEBUG_INFO,
      "DevSlpTiming: Supported(%d), Deto(%d), Madt(%d)\n",
      DevSlpTiming.Supported,
      DevSlpTiming.Deto,
      DevSlpTiming.Madt
      ));
  }

  //
  // Use 20ms as default DETO when DEVSLP TIMING VARIABLES is not supported or the DETO is 0.
  //
  if ((DevSlpTiming.Supported == 0) || (DevSlpTiming.Deto == 0)) {
    DevSlpTiming.Deto = 20;
  }

  //
  // Use 10ms as default MADT when DEVSLP TIMING VARIABLES is not supported or the MADT is 0.
  //
  if ((DevSlpTiming.Supported == 0) || (DevSlpTiming.Madt == 0)) {
    DevSlpTiming.Madt = 10;
  }

  PortDevSlp |= DevSlpTiming.Deto << 2;
  PortDevSlp |= DevSlpTiming.Madt << 10;
  AhciOrReg (PciIo, Offset + AHCI_PORT_DEVSLP, PortDevSlp);

  if (mAtaAtapiPolicy->AggressiveDeviceSleepEnable == 1) {
    if ((Capability2 & AHCI_CAP2_SADM) != 0) {
      PortDevSlp &= ~AHCI_PORT_DEVSLP_DITO_MASK;
      PortDevSlp |= (625 << 15);
      AhciWriteReg (PciIo, Offset + AHCI_PORT_DEVSLP, PortDevSlp);

      PortDevSlp |= AHCI_PORT_DEVSLP_ADSE;
      AhciWriteReg (PciIo, Offset + AHCI_PORT_DEVSLP, PortDevSlp);
    }
  }

  AhciWriteReg (PciIo, Offset + EFI_AHCI_PORT_CMD, PortCmd);

  DEBUG ((
    DEBUG_INFO,
    "Enabled DevSlp feature at port [%d] PortMultiplier [%d], Port CMD/DEVSLP = %08x / %08x\n",
    Port,
    PortMultiplier,
    PortCmd,
    PortDevSlp
    ));

  return EFI_SUCCESS;
}

/**
  Spin-up disk if IDD was incomplete or PUIS feature is enabled

  @param  PciIo               The PCI IO protocol instance.
  @param  AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param  Port                The number of port.
  @param  PortMultiplier      The multiplier of port.
  @param  IdentifyData        A pointer to data buffer which is used to contain IDENTIFY data.

**/
EFI_STATUS
AhciSpinUpDisk (
  IN EFI_PCI_IO_PROTOCOL    *PciIo,
  IN EFI_AHCI_REGISTERS     *AhciRegisters,
  IN UINT8                  Port,
  IN UINT8                  PortMultiplier,
  IN OUT EFI_IDENTIFY_DATA  *IdentifyData
  )
{
  EFI_STATUS             Status;
  EFI_ATA_COMMAND_BLOCK  AtaCommandBlock;
  EFI_ATA_STATUS_BLOCK   AtaStatusBlock;
  UINT8                  Buffer[512];

  if (IdentifyData->AtaData.specific_config == ATA_SPINUP_CFG_REQUIRED_IDD_INCOMPLETE) {
    //
    // Use SET_FEATURE subcommand to spin up the device.
    //
    Status = AhciDeviceSetFeature (
               PciIo,
               AhciRegisters,
               Port,
               PortMultiplier,
               ATA_SUB_CMD_PUIS_SET_DEVICE_SPINUP,
               0x00,
               ATA_SPINUP_TIMEOUT
               );
    DEBUG ((
      DEBUG_INFO,
      "CMD_PUIS_SET_DEVICE_SPINUP for device at port [%d] PortMultiplier [%d] - %r!\n",
      Port,
      PortMultiplier,
      Status
      ));
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    ASSERT (IdentifyData->AtaData.specific_config == ATA_SPINUP_CFG_NOT_REQUIRED_IDD_INCOMPLETE);

    //
    // Use READ_SECTORS to spin up the device if SpinUp SET FEATURE subcommand is not supported
    //
    ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
    ZeroMem (&AtaStatusBlock, sizeof (EFI_ATA_STATUS_BLOCK));
    //
    // Perform READ SECTORS PIO Data-In command to Read LBA 0
    //
    AtaCommandBlock.AtaCommand     = ATA_CMD_READ_SECTORS;
    AtaCommandBlock.AtaSectorCount = 0x1;

    Status = AhciPioTransfer (
               PciIo,
               AhciRegisters,
               Port,
               PortMultiplier,
               NULL,
               0,
               TRUE,
               &AtaCommandBlock,
               &AtaStatusBlock,
               &Buffer,
               sizeof (Buffer),
               ATA_SPINUP_TIMEOUT,
               NULL
               );
    DEBUG ((
      DEBUG_INFO,
      "Read LBA 0 for device at port [%d] PortMultiplier [%d] - %r!\n",
      Port,
      PortMultiplier,
      Status
      ));
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Read the complete IDENTIFY DEVICE data.
  //
  ZeroMem (IdentifyData, sizeof (*IdentifyData));
  Status = AhciIdentify (PciIo, AhciRegisters, Port, PortMultiplier, IdentifyData);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Read IDD failed for device at port [%d] PortMultiplier [%d] - %r!\n",
      Port,
      PortMultiplier,
      Status
      ));
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "IDENTIFY DEVICE: [0] = %016x, [2] = %016x, [83] = %016x, [86] = %016x\n",
    IdentifyData->AtaData.config,
    IdentifyData->AtaData.specific_config,
    IdentifyData->AtaData.command_set_supported_83,
    IdentifyData->AtaData.command_set_feature_enb_86
    ));
  //
  // Check if IDD is incomplete
  //
  if ((IdentifyData->AtaData.config & BIT2) != 0) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Enable/disable/skip PUIS of the disk according to policy.

  @param  PciIo               The PCI IO protocol instance.
  @param  AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param  Port                The number of port.
  @param  PortMultiplier      The multiplier of port.

**/
EFI_STATUS
AhciPuisEnable (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN EFI_AHCI_REGISTERS   *AhciRegisters,
  IN UINT8                Port,
  IN UINT8                PortMultiplier
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;
  if (mAtaAtapiPolicy->PuisEnable == 0) {
    Status = AhciDeviceSetFeature (PciIo, AhciRegisters, Port, PortMultiplier, ATA_SUB_CMD_DISABLE_PUIS, 0x00, ATA_ATAPI_TIMEOUT);
  } else if (mAtaAtapiPolicy->PuisEnable == 1) {
    Status = AhciDeviceSetFeature (PciIo, AhciRegisters, Port, PortMultiplier, ATA_SUB_CMD_ENABLE_PUIS, 0x00, ATA_ATAPI_TIMEOUT);
  }

  DEBUG ((
    DEBUG_INFO,
    "%a PUIS feature at port [%d] PortMultiplier [%d] - %r!\n",
    (mAtaAtapiPolicy->PuisEnable == 0) ? "Disable" : (
                                                      (mAtaAtapiPolicy->PuisEnable == 1) ? "Enable" : "Skip"
                                                      ),
    Port,
    PortMultiplier,
    Status
    ));
  return Status;
}

/**
  Initialize ATA host controller at AHCI mode.

  The function is designed to initialize ATA host controller.

  @param[in]  Instance          A pointer to the ATA_ATAPI_PASS_THRU_INSTANCE instance.

**/
EFI_STATUS
EFIAPI
AhciModeInitialization (
  IN  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance
  )
{
  EFI_STATUS                        Status;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *IdeInit;
  UINT32                            Capability;
  UINT8                             MaxPortNumber;
  UINT32                            PortImplementBitMap;

  EFI_AHCI_REGISTERS  *AhciRegisters;

  UINT8                    Port;
  DATA_64                  Data64;
  UINT32                   Offset;
  UINT32                   Data;
  EFI_IDENTIFY_DATA        Buffer;
  EFI_ATA_DEVICE_TYPE      DeviceType;
  EFI_ATA_COLLECTIVE_MODE  *SupportedModes;
  EFI_ATA_TRANSFER_MODE    TransferMode;
  UINT32                   PhyDetectDelay;
  UINT32                   Value;

  if (Instance == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PciIo   = Instance->PciIo;
  IdeInit = Instance->IdeControllerInit;

  Status = AhciReset (PciIo, EFI_AHCI_BUS_RESET_TIMEOUT);

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Collect AHCI controller information
  //
  Capability = AhciReadReg (PciIo, EFI_AHCI_CAPABILITY_OFFSET);

  //
  // Make sure that GHC.AE bit is set before accessing any AHCI registers.
  //
  Value = AhciReadReg (PciIo, EFI_AHCI_GHC_OFFSET);

  if ((Value & EFI_AHCI_GHC_ENABLE) == 0) {
    AhciOrReg (PciIo, EFI_AHCI_GHC_OFFSET, EFI_AHCI_GHC_ENABLE);
  }

  //
  // Enable 64-bit DMA support in the PCI layer if this controller
  // supports it.
  //
  if ((Capability & EFI_AHCI_CAP_S64A) != 0) {
    Status = PciIo->Attributes (
                      PciIo,
                      EfiPciIoAttributeOperationEnable,
                      EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE,
                      NULL
                      );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_WARN,
        "AhciModeInitialization: failed to enable 64-bit DMA on 64-bit capable controller (%r)\n",
        Status
        ));
    }
  }

  //
  // Get the number of command slots per port supported by this HBA.
  //
  MaxPortNumber = (UINT8)((Capability & 0x1F) + 1);

  //
  // Get the bit map of those ports exposed by this HBA.
  // It indicates which ports that the HBA supports are available for software to use.
  //
  PortImplementBitMap = AhciReadReg (PciIo, EFI_AHCI_PI_OFFSET);

  AhciRegisters = &Instance->AhciRegisters;
  Status        = AhciCreateTransferDescriptor (PciIo, AhciRegisters);

  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Port = 0; Port < EFI_AHCI_MAX_PORTS; Port++) {
    if ((PortImplementBitMap & (((UINT32)BIT0) << Port)) != 0) {
      //
      // According to AHCI spec, MaxPortNumber should be equal or greater than the number of implemented ports.
      //
      if ((MaxPortNumber--) == 0) {
        //
        // Should never be here.
        //
        ASSERT (FALSE);
        return EFI_SUCCESS;
      }

      IdeInit->NotifyPhase (IdeInit, EfiIdeBeforeChannelEnumeration, Port);

      //
      // Initialize FIS Base Address Register and Command List Base Address Register for use.
      //
      Data64.Uint64 = (UINTN)(AhciRegisters->AhciRFisPciAddr) + sizeof (EFI_AHCI_RECEIVED_FIS) * Port;
      Offset        = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_FB;
      AhciWriteReg (PciIo, Offset, Data64.Uint32.Lower32);
      Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_FBU;
      AhciWriteReg (PciIo, Offset, Data64.Uint32.Upper32);

      Data64.Uint64 = (UINTN)(AhciRegisters->AhciCmdListPciAddr);
      Offset        = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CLB;
      AhciWriteReg (PciIo, Offset, Data64.Uint32.Lower32);
      Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CLBU;
      AhciWriteReg (PciIo, Offset, Data64.Uint32.Upper32);

      Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
      Data   = AhciReadReg (PciIo, Offset);
      if ((Data & EFI_AHCI_PORT_CMD_CPD) != 0) {
        AhciOrReg (PciIo, Offset, EFI_AHCI_PORT_CMD_POD);
      }

      if ((Capability & EFI_AHCI_CAP_SSS) != 0) {
        AhciOrReg (PciIo, Offset, EFI_AHCI_PORT_CMD_SUD);
      }

      //
      // Disable aggressive power management.
      //
      Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SCTL;
      AhciOrReg (PciIo, Offset, EFI_AHCI_PORT_SCTL_IPM_INIT);
      //
      // Disable the reporting of the corresponding interrupt to system software.
      //
      Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_IE;
      AhciAndReg (PciIo, Offset, 0);

      //
      // Now inform the IDE Controller Init Module.
      //
      IdeInit->NotifyPhase (IdeInit, EfiIdeBusBeforeDevicePresenceDetection, Port);

      //
      // Enable FIS Receive DMA engine for the first D2H FIS.
      //
      Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
      AhciOrReg (PciIo, Offset, EFI_AHCI_PORT_CMD_FRE);

      //
      // Wait for the Phy to detect the presence of a device.
      //
      PhyDetectDelay = EFI_AHCI_BUS_PHY_DETECT_TIMEOUT;
      Offset         = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
      do {
        Data = AhciReadReg (PciIo, Offset) & EFI_AHCI_PORT_SSTS_DET_MASK;
        if ((Data == EFI_AHCI_PORT_SSTS_DET_PCE) || (Data == EFI_AHCI_PORT_SSTS_DET)) {
          break;
        }

        MicroSecondDelay (1000);
        PhyDetectDelay--;
      } while (PhyDetectDelay > 0);

      if (PhyDetectDelay == 0) {
        //
        // No device detected at this port.
        // Clear PxCMD.SUD for those ports at which there are no device present.
        //
        Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
        AhciAndReg (PciIo, Offset, (UINT32) ~(EFI_AHCI_PORT_CMD_SUD));
        continue;
      }

      Status = AhciWaitDeviceReady (PciIo, Port);
      if (EFI_ERROR (Status)) {
        continue;
      }

      //
      // When the first D2H register FIS is received, the content of PxSIG register is updated.
      //
      Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SIG;
      Status = AhciWaitMmioSet (
                 PciIo,
                 Offset,
                 0x0000FFFF,
                 0x00000101,
                 EFI_TIMER_PERIOD_SECONDS (16)
                 );
      if (EFI_ERROR (Status)) {
        continue;
      }

      Data = AhciReadReg (PciIo, Offset);
      if ((Data & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATAPI_DEVICE_SIG) {
        Status = AhciIdentifyPacket (PciIo, AhciRegisters, Port, 0, &Buffer);

        if (EFI_ERROR (Status)) {
          continue;
        }

        DeviceType = EfiIdeCdrom;
      } else if ((Data & EFI_AHCI_ATAPI_SIG_MASK) == EFI_AHCI_ATA_DEVICE_SIG) {
        Status = AhciIdentify (PciIo, AhciRegisters, Port, 0, &Buffer);

        if (EFI_ERROR (Status)) {
          REPORT_STATUS_CODE (EFI_PROGRESS_CODE, (EFI_PERIPHERAL_FIXED_MEDIA | EFI_P_EC_NOT_DETECTED));
          continue;
        }

        DEBUG ((
          DEBUG_INFO,
          "IDENTIFY DEVICE: [0] = %016x, [2] = %016x, [83] = %016x, [86] = %016x\n",
          Buffer.AtaData.config,
          Buffer.AtaData.specific_config,
          Buffer.AtaData.command_set_supported_83,
          Buffer.AtaData.command_set_feature_enb_86
          ));
        if ((Buffer.AtaData.config & BIT2) != 0) {
          //
          // SpinUp disk if device reported incomplete IDENTIFY DEVICE.
          //
          Status = AhciSpinUpDisk (
                     PciIo,
                     AhciRegisters,
                     Port,
                     0,
                     &Buffer
                     );
          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_ERROR, "Spin up standby device failed - %r\n", Status));
            continue;
          }
        }

        DeviceType = EfiIdeHarddisk;
      } else {
        continue;
      }

      DEBUG ((
        DEBUG_INFO,
        "port [%d] port multitplier [%d] has a [%a]\n",
        Port,
        0,
        DeviceType == EfiIdeCdrom ? "cdrom" : "harddisk"
        ));

      //
      // If the device is a hard disk, then try to enable S.M.A.R.T feature
      //
      if ((DeviceType == EfiIdeHarddisk) && PcdGetBool (PcdAtaSmartEnable)) {
        AhciAtaSmartSupport (
          PciIo,
          AhciRegisters,
          Port,
          0,
          &Buffer,
          NULL
          );
      }

      //
      // Submit identify data to IDE controller init driver
      //
      IdeInit->SubmitData (IdeInit, Port, 0, &Buffer);

      //
      // Now start to config ide device parameter and transfer mode.
      //
      Status = IdeInit->CalculateMode (
                          IdeInit,
                          Port,
                          0,
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

      //
      // Set supported DMA mode on this IDE device. Note that UDMA & MDMA can't
      // be set together. Only one DMA mode can be set to a device. If setting
      // DMA mode operation fails, we can continue moving on because we only use
      // PIO mode at boot time. DMA modes are used by certain kind of OS booting
      //
      if (SupportedModes->UdmaMode.Valid) {
        TransferMode.ModeCategory = EFI_ATA_MODE_UDMA;
        TransferMode.ModeNumber   = (UINT8)(SupportedModes->UdmaMode.Mode);
      } else if (SupportedModes->MultiWordDmaMode.Valid) {
        TransferMode.ModeCategory = EFI_ATA_MODE_MDMA;
        TransferMode.ModeNumber   = (UINT8)SupportedModes->MultiWordDmaMode.Mode;
      }

      Status = AhciDeviceSetFeature (PciIo, AhciRegisters, Port, 0, 0x03, (UINT32)(*(UINT8 *)&TransferMode), ATA_ATAPI_TIMEOUT);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Set transfer Mode Fail, Status = %r\n", Status));
        continue;
      }

      //
      // Found a ATA or ATAPI device, add it into the device list.
      //
      CreateNewDeviceInfo (Instance, Port, 0xFFFF, DeviceType, &Buffer);
      if (DeviceType == EfiIdeHarddisk) {
        REPORT_STATUS_CODE (EFI_PROGRESS_CODE, (EFI_PERIPHERAL_FIXED_MEDIA | EFI_P_PC_ENABLE));
        AhciEnableDevSlp (
          PciIo,
          AhciRegisters,
          Port,
          0,
          &Buffer
          );
      }

      //
      // Enable/disable PUIS according to policy setting if PUIS is capable (Word[83].BIT5 is set).
      //
      if ((Buffer.AtaData.command_set_supported_83 & BIT5) != 0) {
        Status = AhciPuisEnable (
                   PciIo,
                   AhciRegisters,
                   Port,
                   0
                   );
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "PUIS enable/disable failed, Status = %r\n", Status));
          continue;
        }
      }
    }
  }

  return EFI_SUCCESS;
}
