/** @file
  The AhciPei driver is used to manage ATA hard disk device working under AHCI
  mode at PEI phase.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AhciPei.h"

#define ATA_CMD_TRUST_NON_DATA           0x5B
#define ATA_CMD_TRUST_RECEIVE            0x5C
#define ATA_CMD_TRUST_SEND               0x5E

//
// Look up table (IsWrite) for EFI_ATA_PASS_THRU_CMD_PROTOCOL
//
EFI_ATA_PASS_THRU_CMD_PROTOCOL  mAtaPassThruCmdProtocols[2] = {
  EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_IN,
  EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_OUT
};

//
// Look up table (Lba48Bit, IsIsWrite) for ATA_CMD
//
UINT8 mAtaCommands[2][2] = {
  {
    ATA_CMD_READ_SECTORS,            // 28-bit LBA; PIO read
    ATA_CMD_WRITE_SECTORS            // 28-bit LBA; PIO write
  },
  {
    ATA_CMD_READ_SECTORS_EXT,        // 48-bit LBA; PIO read
    ATA_CMD_WRITE_SECTORS_EXT        // 48-bit LBA; PIO write
  }
};

//
// Look up table (IsTrustSend) for ATA_CMD
//
UINT8  mAtaTrustCommands[2] = {
  ATA_CMD_TRUST_RECEIVE,    // PIO read
  ATA_CMD_TRUST_SEND        // PIO write
};

//
// Look up table (Lba48Bit) for maximum transfer block number
//
#define MAX_28BIT_TRANSFER_BLOCK_NUM     0x100
//
// Due to limited resource for VTd PEI DMA buffer on platforms, the driver
// limits the maximum transfer block number for 48-bit addressing.
// Here, setting to 0x800 means that for device with 512-byte block size, the
// maximum buffer for DMA mapping will be 1M bytes in size.
//
#define MAX_48BIT_TRANSFER_BLOCK_NUM     0x800

UINT32 mMaxTransferBlockNumber[2] = {
  MAX_28BIT_TRANSFER_BLOCK_NUM,
  MAX_48BIT_TRANSFER_BLOCK_NUM
};

//
// The maximum total sectors count in 28 bit addressing mode
//
#define MAX_28BIT_ADDRESSING_CAPACITY    0xfffffff


/**
  Read AHCI Operation register.

  @param[in] AhciBar    AHCI bar address.
  @param[in] Offset     The operation register offset.

  @return The register content read.

**/
UINT32
AhciReadReg (
  IN UINTN     AhciBar,
  IN UINT32    Offset
  )
{
  UINT32   Data;

  Data = 0;
  Data = MmioRead32 (AhciBar + Offset);

  return Data;
}

/**
  Write AHCI Operation register.

  @param[in] AhciBar    AHCI bar address.
  @param[in] Offset     The operation register offset.
  @param[in] Data       The Data used to write down.

**/
VOID
AhciWriteReg (
  IN UINTN     AhciBar,
  IN UINT32    Offset,
  IN UINT32    Data
  )
{
  MmioWrite32 (AhciBar + Offset, Data);
}

/**
  Do AND operation with the value of AHCI Operation register.

  @param[in] AhciBar    AHCI bar address.
  @param[in] Offset     The operation register offset.
  @param[in] AndData    The data used to do AND operation.

**/
VOID
AhciAndReg (
  IN UINTN     AhciBar,
  IN UINT32    Offset,
  IN UINT32    AndData
  )
{
  UINT32 Data;

  Data  = AhciReadReg (AhciBar, Offset);
  Data &= AndData;

  AhciWriteReg (AhciBar, Offset, Data);
}

/**
  Do OR operation with the Value of AHCI Operation register.

  @param[in] AhciBar    AHCI bar address.
  @param[in] Offset     The operation register offset.
  @param[in] OrData     The Data used to do OR operation.

**/
VOID
AhciOrReg (
  IN UINTN     AhciBar,
  IN UINT32    Offset,
  IN UINT32    OrData
  )
{
  UINT32 Data;

  Data  = AhciReadReg (AhciBar, Offset);
  Data |= OrData;

  AhciWriteReg (AhciBar, Offset, Data);
}

/**
  Wait for memory set to the test Value.

  @param[in] AhciBar      AHCI bar address.
  @param[in] Offset       The memory offset to test.
  @param[in] MaskValue    The mask Value of memory.
  @param[in] TestValue    The test Value of memory.
  @param[in] Timeout      The timeout, in 100ns units, for wait memory set.

  @retval EFI_DEVICE_ERROR    The memory is not set.
  @retval EFI_TIMEOUT         The memory setting is time out.
  @retval EFI_SUCCESS         The memory is correct set.

**/
EFI_STATUS
EFIAPI
AhciWaitMmioSet (
  IN UINTN     AhciBar,
  IN UINT32    Offset,
  IN UINT32    MaskValue,
  IN UINT32    TestValue,
  IN UINT64    Timeout
  )
{
  UINT32    Value;
  UINT32    Delay;

  Delay = (UINT32) (DivU64x32(Timeout, 1000) + 1);

  do {
    Value = AhciReadReg (AhciBar, Offset) & MaskValue;

    if (Value == TestValue) {
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
  Check the memory status to the test value.

  @param[in] Address       The memory address to test.
  @param[in] MaskValue     The mask value of memory.
  @param[in] TestValue     The test value of memory.

  @retval EFI_NOT_READY    The memory is not set.
  @retval EFI_SUCCESS      The memory is correct set.

**/
EFI_STATUS
AhciCheckMemSet (
  IN UINTN     Address,
  IN UINT32    MaskValue,
  IN UINT32    TestValue
  )
{
  UINT32     Value;

  Value  = *(volatile UINT32 *) Address;
  Value &= MaskValue;

  if (Value == TestValue) {
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_READY;
  }
}

/**
  Wait for the value of the specified system memory set to the test value.

  @param[in] Address      The system memory address to test.
  @param[in] MaskValue    The mask value of memory.
  @param[in] TestValue    The test value of memory.
  @param[in] Timeout      The timeout, in 100ns units, for wait memory set.

  @retval EFI_TIMEOUT    The system memory setting is time out.
  @retval EFI_SUCCESS    The system memory is correct set.

**/
EFI_STATUS
AhciWaitMemSet (
  IN  EFI_PHYSICAL_ADDRESS    Address,
  IN  UINT32                  MaskValue,
  IN  UINT32                  TestValue,
  IN  UINT64                  Timeout
  )
{
  UINT32     Value;
  UINT64     Delay;
  BOOLEAN    InfiniteWait;

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
    Value  = *(volatile UINT32 *) (UINTN) Address;
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

  Clear the port interrupt and error status. It will also clear HBA interrupt
  status.

  @param[in] AhciBar    AHCI bar address.
  @param[in] Port       The number of port.

**/
VOID
AhciClearPortStatus (
  IN UINTN    AhciBar,
  IN UINT8    Port
  )
{
  UINT32    Offset;

  //
  // Clear any error status
  //
  Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_SERR;
  AhciWriteReg (AhciBar, Offset, AhciReadReg (AhciBar, Offset));

  //
  // Clear any port interrupt status
  //
  Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_IS;
  AhciWriteReg (AhciBar, Offset, AhciReadReg (AhciBar, Offset));

  //
  // Clear any HBA interrupt status
  //
  AhciWriteReg (AhciBar, AHCI_IS_OFFSET, AhciReadReg (AhciBar, AHCI_IS_OFFSET));
}

/**
  Enable the FIS running for giving port.

  @param[in] AhciBar    AHCI bar address.
  @param[in] Port       The number of port.
  @param[in] Timeout    The timeout, in 100ns units, to enabling FIS.

  @retval EFI_DEVICE_ERROR    The FIS enable setting fails.
  @retval EFI_TIMEOUT         The FIS enable setting is time out.
  @retval EFI_SUCCESS         The FIS enable successfully.

**/
EFI_STATUS
AhciEnableFisReceive (
  IN UINTN     AhciBar,
  IN UINT8     Port,
  IN UINT64    Timeout
  )
{
  UINT32 Offset;

  Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD;
  AhciOrReg (AhciBar, Offset, AHCI_PORT_CMD_FRE);

  return EFI_SUCCESS;
}

/**
  Disable the FIS running for giving port.

  @param[in] AhciBar    AHCI bar address.
  @param[in] Port       The number of port.
  @param[in] Timeout    The timeout value of disabling FIS, uses 100ns as a unit.

  @retval EFI_DEVICE_ERROR    The FIS disable setting fails.
  @retval EFI_TIMEOUT         The FIS disable setting is time out.
  @retval EFI_UNSUPPORTED     The port is in running state.
  @retval EFI_SUCCESS         The FIS disable successfully.

**/
EFI_STATUS
AhciDisableFisReceive (
  IN UINTN     AhciBar,
  IN UINT8     Port,
  IN UINT64    Timeout
  )
{
  UINT32    Offset;
  UINT32    Data;

  Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD;
  Data   = AhciReadReg (AhciBar, Offset);

  //
  // Before disabling Fis receive, the DMA engine of the port should NOT be in
  // running status.
  //
  if ((Data & (AHCI_PORT_CMD_ST | AHCI_PORT_CMD_CR)) != 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check if the Fis receive DMA engine for the port is running.
  //
  if ((Data & AHCI_PORT_CMD_FR) != AHCI_PORT_CMD_FR) {
    return EFI_SUCCESS;
  }

  AhciAndReg (AhciBar, Offset, (UINT32)~(AHCI_PORT_CMD_FRE));

  return AhciWaitMmioSet (
           AhciBar,
           Offset,
           AHCI_PORT_CMD_FR,
           0,
           Timeout
           );
}

/**
  Build the command list, command table and prepare the fis receiver.

  @param[in]     Private              The pointer to the PEI_AHCI_CONTROLLER_PRIVATE_DATA.
  @param[in]     Port                 The number of port.
  @param[in]     PortMultiplier       The number of port multiplier.
  @param[in]     FisIndex             The offset index of the FIS base address.
  @param[in]     CommandFis           The control fis will be used for the transfer.
  @param[in]     CommandList          The command list will be used for the transfer.
  @param[in]     CommandSlotNumber    The command slot will be used for the transfer.
  @param[in,out] DataPhysicalAddr     The pointer to the data buffer pci bus master
                                      address.
  @param[in]     DataLength           The data count to be transferred.

**/
VOID
AhciBuildCommand (
  IN     PEI_AHCI_CONTROLLER_PRIVATE_DATA    *Private,
  IN     UINT8                               Port,
  IN     UINT8                               PortMultiplier,
  IN     UINT8                               FisIndex,
  IN     EFI_AHCI_COMMAND_FIS                *CommandFis,
  IN     EFI_AHCI_COMMAND_LIST               *CommandList,
  IN     UINT8                               CommandSlotNumber,
  IN OUT VOID                                *DataPhysicalAddr,
  IN     UINT32                              DataLength
  )
{
  EFI_AHCI_REGISTERS    *AhciRegisters;
  UINTN                 AhciBar;
  UINT64                BaseAddr;
  UINT32                PrdtNumber;
  UINT32                PrdtIndex;
  UINTN                 RemainedData;
  UINTN                 MemAddr;
  DATA_64               Data64;
  UINT32                Offset;

  AhciRegisters = &Private->AhciRegisters;
  AhciBar       = Private->MmioBase;

  //
  // Filling the PRDT
  //
  PrdtNumber = (UINT32)DivU64x32 (
                         (UINT64)DataLength + AHCI_MAX_DATA_PER_PRDT - 1,
                         AHCI_MAX_DATA_PER_PRDT
                         );

  //
  // According to AHCI 1.3 spec, a PRDT entry can point to a maximum 4MB data block.
  // It also limits that the maximum amount of the PRDT entry in the command table
  // is 65535.
  // Current driver implementation supports up to a maximum of AHCI_MAX_PRDT_NUMBER
  // PRDT entries.
  //
  ASSERT (PrdtNumber <= AHCI_MAX_PRDT_NUMBER);
  if (PrdtNumber > AHCI_MAX_PRDT_NUMBER) {
    return;
  }

  Data64.Uint64 = (UINTN) (AhciRegisters->AhciRFis) + sizeof (EFI_AHCI_RECEIVED_FIS) * FisIndex;

  BaseAddr = Data64.Uint64;

  ZeroMem ((VOID *)((UINTN) BaseAddr), sizeof (EFI_AHCI_RECEIVED_FIS));

  ZeroMem (AhciRegisters->AhciCmdTable, sizeof (EFI_AHCI_COMMAND_TABLE));

  CommandFis->AhciCFisPmNum = PortMultiplier;

  CopyMem (&AhciRegisters->AhciCmdTable->CommandFis, CommandFis, sizeof (EFI_AHCI_COMMAND_FIS));

  Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD;
  AhciAndReg (AhciBar, Offset, (UINT32)~(AHCI_PORT_CMD_DLAE | AHCI_PORT_CMD_ATAPI));

  RemainedData = (UINTN) DataLength;
  MemAddr      = (UINTN) DataPhysicalAddr;
  CommandList->AhciCmdPrdtl = PrdtNumber;

  for (PrdtIndex = 0; PrdtIndex < PrdtNumber; PrdtIndex++) {
    if (RemainedData < AHCI_MAX_DATA_PER_PRDT) {
      AhciRegisters->AhciCmdTable->PrdtTable[PrdtIndex].AhciPrdtDbc = (UINT32)RemainedData - 1;
    } else {
      AhciRegisters->AhciCmdTable->PrdtTable[PrdtIndex].AhciPrdtDbc = AHCI_MAX_DATA_PER_PRDT - 1;
    }

    Data64.Uint64 = (UINT64)MemAddr;
    AhciRegisters->AhciCmdTable->PrdtTable[PrdtIndex].AhciPrdtDba  = Data64.Uint32.Lower32;
    AhciRegisters->AhciCmdTable->PrdtTable[PrdtIndex].AhciPrdtDbau = Data64.Uint32.Upper32;
    RemainedData -= AHCI_MAX_DATA_PER_PRDT;
    MemAddr      += AHCI_MAX_DATA_PER_PRDT;
  }

  //
  // Set the last PRDT to Interrupt On Complete
  //
  if (PrdtNumber > 0) {
    AhciRegisters->AhciCmdTable->PrdtTable[PrdtNumber - 1].AhciPrdtIoc = 1;
  }

  CopyMem (
    (VOID *) ((UINTN) AhciRegisters->AhciCmdList + (UINTN) CommandSlotNumber * sizeof (EFI_AHCI_COMMAND_LIST)),
    CommandList,
    sizeof (EFI_AHCI_COMMAND_LIST)
    );

  Data64.Uint64 = (UINT64)(UINTN) AhciRegisters->AhciCmdTable;
  AhciRegisters->AhciCmdList[CommandSlotNumber].AhciCmdCtba  = Data64.Uint32.Lower32;
  AhciRegisters->AhciCmdList[CommandSlotNumber].AhciCmdCtbau = Data64.Uint32.Upper32;
  AhciRegisters->AhciCmdList[CommandSlotNumber].AhciCmdPmp   = PortMultiplier;
}

/**
  Build a command FIS.

  @param[in,out] CmdFis             A pointer to the EFI_AHCI_COMMAND_FIS data
                                    structure.
  @param[in]     AtaCommandBlock    A pointer to the EFI_ATA_COMMAND_BLOCK data
                                    structure.

**/
VOID
AhciBuildCommandFis (
  IN OUT EFI_AHCI_COMMAND_FIS     *CmdFis,
  IN     EFI_ATA_COMMAND_BLOCK    *AtaCommandBlock
  )
{
  ZeroMem (CmdFis, sizeof (EFI_AHCI_COMMAND_FIS));

  CmdFis->AhciCFisType = AHCI_FIS_REGISTER_H2D;
  //
  // Indicator it's a command
  //
  CmdFis->AhciCFisCmdInd      = 0x1;
  CmdFis->AhciCFisCmd         = AtaCommandBlock->AtaCommand;

  CmdFis->AhciCFisFeature     = AtaCommandBlock->AtaFeatures;
  CmdFis->AhciCFisFeatureExp  = AtaCommandBlock->AtaFeaturesExp;

  CmdFis->AhciCFisSecNum      = AtaCommandBlock->AtaSectorNumber;
  CmdFis->AhciCFisSecNumExp   = AtaCommandBlock->AtaSectorNumberExp;

  CmdFis->AhciCFisClyLow      = AtaCommandBlock->AtaCylinderLow;
  CmdFis->AhciCFisClyLowExp   = AtaCommandBlock->AtaCylinderLowExp;

  CmdFis->AhciCFisClyHigh     = AtaCommandBlock->AtaCylinderHigh;
  CmdFis->AhciCFisClyHighExp  = AtaCommandBlock->AtaCylinderHighExp;

  CmdFis->AhciCFisSecCount    = AtaCommandBlock->AtaSectorCount;
  CmdFis->AhciCFisSecCountExp = AtaCommandBlock->AtaSectorCountExp;

  CmdFis->AhciCFisDevHead     = (UINT8) (AtaCommandBlock->AtaDeviceHead | 0xE0);
}

/**
  Stop command running for giving port

  @param[in] AhciBar    AHCI bar address.
  @param[in] Port       The number of port.
  @param[in] Timeout    The timeout value, in 100ns units, to stop.

  @retval EFI_DEVICE_ERROR    The command stop unsuccessfully.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_SUCCESS         The command stop successfully.

**/
EFI_STATUS
AhciStopCommand (
  IN  UINTN     AhciBar,
  IN  UINT8     Port,
  IN  UINT64    Timeout
  )
{
  UINT32    Offset;
  UINT32    Data;

  Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD;
  Data   = AhciReadReg (AhciBar, Offset);

  if ((Data & (AHCI_PORT_CMD_ST | AHCI_PORT_CMD_CR)) == 0) {
    return EFI_SUCCESS;
  }

  if ((Data & AHCI_PORT_CMD_ST) != 0) {
    AhciAndReg (AhciBar, Offset, (UINT32)~(AHCI_PORT_CMD_ST));
  }

  return AhciWaitMmioSet (
           AhciBar,
           Offset,
           AHCI_PORT_CMD_CR,
           0,
           Timeout
           );
}

/**
  Start command for give slot on specific port.

  @param[in] AhciBar       AHCI bar address.
  @param[in] Port          The number of port.
  @param[in] CommandSlot   The number of Command Slot.
  @param[in] Timeout       The timeout value, in 100ns units, to start.

  @retval EFI_DEVICE_ERROR    The command start unsuccessfully.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_SUCCESS         The command start successfully.

**/
EFI_STATUS
AhciStartCommand (
  IN  UINTN     AhciBar,
  IN  UINT8     Port,
  IN  UINT8     CommandSlot,
  IN  UINT64    Timeout
  )
{
  UINT32        CmdSlotBit;
  EFI_STATUS    Status;
  UINT32        PortStatus;
  UINT32        StartCmd;
  UINT32        PortTfd;
  UINT32        Offset;
  UINT32        Capability;

  //
  // Collect AHCI controller information
  //
  Capability = AhciReadReg (AhciBar, AHCI_CAPABILITY_OFFSET);

  CmdSlotBit = (UINT32) (1 << CommandSlot);

  AhciClearPortStatus (
    AhciBar,
    Port
    );

  Status = AhciEnableFisReceive (
             AhciBar,
             Port,
             Timeout
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD;
  PortStatus = AhciReadReg (AhciBar, Offset);

  StartCmd = 0;
  if ((PortStatus & AHCI_PORT_CMD_ALPE) != 0) {
    StartCmd = AhciReadReg (AhciBar, Offset);
    StartCmd &= ~AHCI_PORT_CMD_ICC_MASK;
    StartCmd |= AHCI_PORT_CMD_ACTIVE;
  }

  Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_TFD;
  PortTfd = AhciReadReg (AhciBar, Offset);

  if ((PortTfd & (AHCI_PORT_TFD_BSY | AHCI_PORT_TFD_DRQ)) != 0) {
    if ((Capability & BIT24) != 0) {
      Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD;
      AhciOrReg (AhciBar, Offset, AHCI_PORT_CMD_CLO);

      AhciWaitMmioSet (
        AhciBar,
        Offset,
        AHCI_PORT_CMD_CLO,
        0,
        Timeout
        );
    }
  }

  Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD;
  AhciOrReg (AhciBar, Offset, AHCI_PORT_CMD_ST | StartCmd);

  //
  // Setting the command
  //
  Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CI;
  AhciAndReg (AhciBar, Offset, 0);
  AhciOrReg  (AhciBar, Offset, CmdSlotBit);

  return EFI_SUCCESS;
}

/**
  Start a PIO Data transfer on specific port.

  @param[in]     Private            The pointer to the PEI_AHCI_CONTROLLER_PRIVATE_DATA.
  @param[in]     Port               The number of port.
  @param[in]     PortMultiplier     The number of port multiplier.
  @param[in]     FisIndex           The offset index of the FIS base address.
  @param[in]     Read               The transfer direction.
  @param[in]     AtaCommandBlock    The EFI_ATA_COMMAND_BLOCK data.
  @param[in,out] AtaStatusBlock     The EFI_ATA_STATUS_BLOCK data.
  @param[in,out] MemoryAddr         The pointer to the data buffer.
  @param[in]     DataCount          The data count to be transferred.
  @param[in]     Timeout            The timeout value of PIO data transfer, uses
                                    100ns as a unit.

  @retval EFI_DEVICE_ERROR        The PIO data transfer abort with error occurs.
  @retval EFI_TIMEOUT             The operation is time out.
  @retval EFI_UNSUPPORTED         The device is not ready for transfer.
  @retval EFI_OUT_OF_RESOURCES    The operation fails due to lack of resources.
  @retval EFI_SUCCESS             The PIO data transfer executes successfully.

**/
EFI_STATUS
AhciPioTransfer (
  IN     PEI_AHCI_CONTROLLER_PRIVATE_DATA    *Private,
  IN     UINT8                               Port,
  IN     UINT8                               PortMultiplier,
  IN     UINT8                               FisIndex,
  IN     BOOLEAN                             Read,
  IN     EFI_ATA_COMMAND_BLOCK               *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK                *AtaStatusBlock,
  IN OUT VOID                                *MemoryAddr,
  IN     UINT32                              DataCount,
  IN     UINT64                              Timeout
  )
{
  EFI_STATUS                    Status;
  EDKII_IOMMU_OPERATION         MapOp;
  UINTN                         MapLength;
  EFI_PHYSICAL_ADDRESS          PhyAddr;
  VOID                          *MapData;
  EFI_AHCI_REGISTERS            *AhciRegisters;
  UINTN                         AhciBar;
  BOOLEAN                       InfiniteWait;
  UINT32                        Offset;
  UINT32                        OldRfisLo;
  UINT32                        OldRfisHi;
  UINT32                        OldCmdListLo;
  UINT32                        OldCmdListHi;
  DATA_64                       Data64;
  UINT32                        FisBaseAddr;
  UINT32                        Delay;
  EFI_AHCI_COMMAND_FIS          CFis;
  EFI_AHCI_COMMAND_LIST         CmdList;
  UINT32                        PortTfd;
  UINT32                        PrdCount;
  BOOLEAN                       PioFisReceived;
  BOOLEAN                       D2hFisReceived;

  //
  // Current driver implementation supports up to a maximum of AHCI_MAX_PRDT_NUMBER
  // PRDT entries.
  //
  if (DataCount / (UINT32)AHCI_MAX_PRDT_NUMBER > AHCI_MAX_DATA_PER_PRDT) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Driver only support a maximum of 0x%x PRDT entries, "
      "current number of data byte 0x%x is too large, maximum allowed is 0x%x.\n",
      __FUNCTION__, AHCI_MAX_PRDT_NUMBER, DataCount,
      AHCI_MAX_PRDT_NUMBER * AHCI_MAX_DATA_PER_PRDT
      ));
    return EFI_UNSUPPORTED;
  }

  MapOp     = Read ? EdkiiIoMmuOperationBusMasterWrite :
                     EdkiiIoMmuOperationBusMasterRead;
  MapLength = DataCount;
  Status    = IoMmuMap (
                MapOp,
                MemoryAddr,
                &MapLength,
                &PhyAddr,
                &MapData
                );
  if (EFI_ERROR (Status) || (MapLength != DataCount)) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to map data buffer.\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  AhciRegisters  = &Private->AhciRegisters;
  AhciBar        = Private->MmioBase;
  InfiniteWait   = (Timeout == 0) ? TRUE : FALSE;

  //
  // Fill FIS base address register
  //
  Offset        = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_FB;
  OldRfisLo     = AhciReadReg (AhciBar, Offset);
  Offset        = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_FBU;
  OldRfisHi     = AhciReadReg (AhciBar, Offset);
  Data64.Uint64 = (UINTN) (AhciRegisters->AhciRFis) + sizeof (EFI_AHCI_RECEIVED_FIS) * FisIndex;
  Offset        = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_FB;
  AhciWriteReg (AhciBar, Offset, Data64.Uint32.Lower32);
  Offset        = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_FBU;
  AhciWriteReg (AhciBar, Offset, Data64.Uint32.Upper32);

  //
  // Single task environment, we only use one command table for all port
  //
  Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CLB;
  OldCmdListLo  = AhciReadReg (AhciBar, Offset);
  Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CLBU;
  OldCmdListHi  = AhciReadReg (AhciBar, Offset);
  Data64.Uint64 = (UINTN) (AhciRegisters->AhciCmdList);
  Offset        = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CLB;
  AhciWriteReg (AhciBar, Offset, Data64.Uint32.Lower32);
  Offset        = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CLBU;
  AhciWriteReg (AhciBar, Offset, Data64.Uint32.Upper32);

  //
  // Package read needed
  //
  AhciBuildCommandFis (&CFis, AtaCommandBlock);

  ZeroMem (&CmdList, sizeof (EFI_AHCI_COMMAND_LIST));

  CmdList.AhciCmdCfl = AHCI_FIS_REGISTER_H2D_LENGTH / 4;
  CmdList.AhciCmdW   = Read ? 0 : 1;

  AhciBuildCommand (
    Private,
    Port,
    PortMultiplier,
    FisIndex,
    &CFis,
    &CmdList,
    0,
    (VOID *)(UINTN)PhyAddr,
    DataCount
    );

  Status = AhciStartCommand (
             AhciBar,
             Port,
             0,
             Timeout
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Checking the status and wait the driver sending Data
  //
  FisBaseAddr = (UINT32)(UINTN)AhciRegisters->AhciRFis + sizeof (EFI_AHCI_RECEIVED_FIS) * FisIndex;
  if (Read) {
    //
    // Wait device sends the PIO setup fis before data transfer
    //
    Status = EFI_TIMEOUT;
    Delay  = (UINT32) DivU64x32 (Timeout, 1000) + 1;
    do {
      PioFisReceived = FALSE;
      D2hFisReceived = FALSE;
      Offset = FisBaseAddr + AHCI_PIO_FIS_OFFSET;
      Status = AhciCheckMemSet (Offset, AHCI_FIS_TYPE_MASK, AHCI_FIS_PIO_SETUP);
      if (!EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "%a: PioFisReceived.\n", __FUNCTION__));
        PioFisReceived = TRUE;
      }
      //
      // According to SATA 2.6 spec section 11.7, D2h FIS means an error encountered.
      // But Qemu and Marvel 9230 sata controller may just receive a D2h FIS from
      // device after the transaction is finished successfully.
      // To get better device compatibilities, we further check if the PxTFD's
      // ERR bit is set. By this way, we can know if there is a real error happened.
      //
      Offset = FisBaseAddr + AHCI_D2H_FIS_OFFSET;
      Status = AhciCheckMemSet (Offset, AHCI_FIS_TYPE_MASK, AHCI_FIS_REGISTER_D2H);
      if (!EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "%a: D2hFisReceived.\n", __FUNCTION__));
        D2hFisReceived = TRUE;
      }

      if (PioFisReceived || D2hFisReceived) {
        Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_TFD;
        PortTfd = AhciReadReg (AhciBar, (UINT32) Offset);
        //
        // PxTFD will be updated if there is a D2H or SetupFIS received.
        //
        if ((PortTfd & AHCI_PORT_TFD_ERR) != 0) {
          Status = EFI_DEVICE_ERROR;
          break;
        }

        PrdCount = *(volatile UINT32 *) (&(AhciRegisters->AhciCmdList[0].AhciCmdPrdbc));
        if (PrdCount == DataCount) {
          Status = EFI_SUCCESS;
          break;
        }
      }

      //
      // Stall for 100 microseconds.
      //
      MicroSecondDelay(100);

      Delay--;
      if (Delay == 0) {
        Status = EFI_TIMEOUT;
      }
    } while (InfiniteWait || (Delay > 0));
  } else {
    //
    // Wait for D2H Fis is received
    //
    Offset = FisBaseAddr + AHCI_D2H_FIS_OFFSET;
    Status = AhciWaitMemSet (
               Offset,
               AHCI_FIS_TYPE_MASK,
               AHCI_FIS_REGISTER_D2H,
               Timeout
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AhciWaitMemSet (%r)\n", __FUNCTION__, Status));
      goto Exit;
    }

    Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_TFD;
    PortTfd = AhciReadReg (AhciBar, (UINT32) Offset);
    if ((PortTfd & AHCI_PORT_TFD_ERR) != 0) {
      Status = EFI_DEVICE_ERROR;
    }
  }

Exit:
  AhciStopCommand (
    AhciBar,
    Port,
    Timeout
    );

  AhciDisableFisReceive (
    AhciBar,
    Port,
    Timeout
    );

  if (MapData != NULL) {
    IoMmuUnmap (MapData);
  }

  Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_FB;
  AhciWriteReg (AhciBar, Offset, OldRfisLo);
  Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_FBU;
  AhciWriteReg (AhciBar, Offset, OldRfisHi);

  Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CLB;
  AhciWriteReg (AhciBar, Offset, OldCmdListLo);
  Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CLBU;
  AhciWriteReg (AhciBar, Offset, OldCmdListHi);

  return Status;
}

/**
  Start a non data transfer on specific port.

  @param[in]     Private            The pointer to the PEI_AHCI_CONTROLLER_PRIVATE_DATA.
  @param[in]     Port               The number of port.
  @param[in]     PortMultiplier     The number of port multiplier.
  @param[in]     FisIndex           The offset index of the FIS base address.
  @param[in]     AtaCommandBlock    The EFI_ATA_COMMAND_BLOCK data.
  @param[in,out] AtaStatusBlock     The EFI_ATA_STATUS_BLOCK data.
  @param[in]     Timeout            The timeout value of non data transfer, uses
                                    100ns as a unit.

  @retval EFI_DEVICE_ERROR        The non data transfer abort with error occurs.
  @retval EFI_TIMEOUT             The operation is time out.
  @retval EFI_UNSUPPORTED         The device is not ready for transfer.
  @retval EFI_SUCCESS             The non data transfer executes successfully.

**/
EFI_STATUS
AhciNonDataTransfer (
  IN     PEI_AHCI_CONTROLLER_PRIVATE_DATA    *Private,
  IN     UINT8                               Port,
  IN     UINT8                               PortMultiplier,
  IN     UINT8                               FisIndex,
  IN     EFI_ATA_COMMAND_BLOCK               *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK                *AtaStatusBlock,
  IN     UINT64                              Timeout
  )
{
  EFI_STATUS                  Status;
  UINTN                       AhciBar;
  EFI_AHCI_REGISTERS          *AhciRegisters;
  UINTN                       FisBaseAddr;
  UINTN                       Offset;
  UINT32                      PortTfd;
  EFI_AHCI_COMMAND_FIS        CFis;
  EFI_AHCI_COMMAND_LIST       CmdList;

  AhciBar        = Private->MmioBase;
  AhciRegisters = &Private->AhciRegisters;

  //
  // Package read needed
  //
  AhciBuildCommandFis (&CFis, AtaCommandBlock);

  ZeroMem (&CmdList, sizeof (EFI_AHCI_COMMAND_LIST));

  CmdList.AhciCmdCfl = AHCI_FIS_REGISTER_H2D_LENGTH / 4;

  AhciBuildCommand (
    Private,
    Port,
    PortMultiplier,
    FisIndex,
    &CFis,
    &CmdList,
    0,
    NULL,
    0
    );

  Status = AhciStartCommand (
             AhciBar,
             Port,
             0,
             Timeout
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Wait device sends the Response Fis
  //
  FisBaseAddr = (UINTN)AhciRegisters->AhciRFis + sizeof (EFI_AHCI_RECEIVED_FIS) * FisIndex;
  Offset      = FisBaseAddr + AHCI_D2H_FIS_OFFSET;
  Status      = AhciWaitMemSet (
                  Offset,
                  AHCI_FIS_TYPE_MASK,
                  AHCI_FIS_REGISTER_D2H,
                  Timeout
                  );

  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_TFD;
  PortTfd = AhciReadReg (AhciBar, (UINT32) Offset);
  if ((PortTfd & AHCI_PORT_TFD_ERR) != 0) {
    Status = EFI_DEVICE_ERROR;
  }

Exit:
  AhciStopCommand (
    AhciBar,
    Port,
    Timeout
    );

  AhciDisableFisReceive (
    AhciBar,
    Port,
    Timeout
    );

  return Status;
}

/**
  Do AHCI HBA reset.

  @param[in] AhciBar         AHCI bar address.
  @param[in] Timeout         The timeout, in 100ns units, to reset.

  @retval EFI_DEVICE_ERROR   AHCI controller is failed to complete hardware reset.
  @retval EFI_TIMEOUT        The reset operation is time out.
  @retval EFI_SUCCESS        AHCI controller is reset successfully.

**/
EFI_STATUS
AhciReset (
  IN UINTN     AhciBar,
  IN UINT64    Timeout
  )
{
  UINT32    Delay;
  UINT32    Value;
  UINT32    Capability;

  //
  // Collect AHCI controller information
  //
  Capability = AhciReadReg (AhciBar, AHCI_CAPABILITY_OFFSET);

  //
  // Enable AE before accessing any AHCI registers if Supports AHCI Mode Only is not set
  //
  if ((Capability & AHCI_CAP_SAM) == 0) {
    AhciOrReg (AhciBar, AHCI_GHC_OFFSET, AHCI_GHC_ENABLE);
  }

  AhciOrReg (AhciBar, AHCI_GHC_OFFSET, AHCI_GHC_RESET);

  Delay = (UINT32) (DivU64x32(Timeout, 1000) + 1);

  do {
    Value = AhciReadReg(AhciBar, AHCI_GHC_OFFSET);
    if ((Value & AHCI_GHC_RESET) == 0) {
      return EFI_SUCCESS;
    }

    //
    // Stall for 100 microseconds.
    //
    MicroSecondDelay(100);

    Delay--;
  } while (Delay > 0);

  return EFI_TIMEOUT;
}

/**
  Send Identify Drive command to a specific device.

  @param[in] Private           The pointer to the PEI_AHCI_CONTROLLER_PRIVATE_DATA.
  @param[in] Port              The number of port.
  @param[in] PortMultiplier    The port multiplier port number.
  @param[in] FisIndex          The offset index of the FIS base address.
  @param[in] Buffer            The data buffer to store IDENTIFY PACKET data.

  @retval EFI_SUCCESS              The cmd executes successfully.
  @retval EFI_INVALID_PARAMETER    Buffer is NULL.
  @retval EFI_DEVICE_ERROR         The cmd abort with error occurs.
  @retval EFI_TIMEOUT              The operation is time out.
  @retval EFI_UNSUPPORTED          The device is not ready for executing.

**/
EFI_STATUS
AhciIdentify (
  IN PEI_AHCI_CONTROLLER_PRIVATE_DATA    *Private,
  IN UINT8                               Port,
  IN UINT8                               PortMultiplier,
  IN UINT8                               FisIndex,
  IN ATA_IDENTIFY_DATA                   *Buffer
  )
{
  EFI_STATUS                     Status;
  EFI_ATA_COMMAND_BLOCK    Acb;
  EFI_ATA_STATUS_BLOCK     Asb;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&Acb, sizeof (EFI_ATA_COMMAND_BLOCK));
  ZeroMem (&Asb, sizeof (EFI_ATA_STATUS_BLOCK));

  Acb.AtaCommand     = ATA_CMD_IDENTIFY_DRIVE;
  Acb.AtaSectorCount = 1;

  Status = AhciPioTransfer (
             Private,
             Port,
             PortMultiplier,
             FisIndex,
             TRUE,
             &Acb,
             &Asb,
             Buffer,
             sizeof (ATA_IDENTIFY_DATA),
             ATA_TIMEOUT
             );

  return Status;
}


/**
  Collect the number of bits set within a port bitmap.

  @param[in] PortBitMap    A 32-bit wide bit map of ATA AHCI ports.

  @retval The number of bits set in the bitmap.

**/
UINT8
AhciGetNumberOfPortsFromMap (
  IN UINT32    PortBitMap
  )
{
  UINT8    NumberOfPorts;

  NumberOfPorts = 0;

  while (PortBitMap != 0) {
    if ((PortBitMap & ((UINT32)BIT0)) != 0) {
      NumberOfPorts++;
    }
    PortBitMap = PortBitMap >> 1;
  }

  return NumberOfPorts;
}

/**
  Get the specified port number from a port bitmap.

  @param[in]  PortBitMap    A 32-bit wide bit map of ATA AHCI ports.
  @param[in]  PortIndex     The specified port index.
  @param[out] Port          The port number of the port specified by PortIndex.

  @retval EFI_SUCCESS       The specified port is found and its port number is
                            in Port.
  @retval EFI_NOT_FOUND     Cannot find the specified port within the port bitmap.

**/
EFI_STATUS
AhciGetPortFromMap (
  IN  UINT32    PortBitMap,
  IN  UINT8     PortIndex,
  OUT UINT8     *Port
  )
{
  if (PortIndex == 0) {
    return EFI_NOT_FOUND;
  }

  *Port = 0;

  while (PortBitMap != 0) {
    if ((PortBitMap & ((UINT32)BIT0)) != 0) {
      PortIndex--;

      //
      // Found the port specified by PortIndex.
      //
      if (PortIndex == 0) {
        return EFI_SUCCESS;
      }
    }
    PortBitMap = PortBitMap >> 1;
    *Port      = *Port + 1;
  }

  return EFI_NOT_FOUND;
}

/**
  Allocate transfer-related data struct which is used at AHCI mode.

  @param[in,out] Private    A pointer to the PEI_AHCI_CONTROLLER_PRIVATE_DATA instance.

  @retval EFI_SUCCESS    Data structures are allocated successfully.
  @retval Others         Data structures are not allocated successfully.

**/
EFI_STATUS
AhciCreateTransferDescriptor (
  IN OUT PEI_AHCI_CONTROLLER_PRIVATE_DATA    *Private
  )
{
  EFI_STATUS              Status;
  UINTN                   AhciBar;
  EFI_AHCI_REGISTERS      *AhciRegisters;
  EFI_PHYSICAL_ADDRESS    DeviceAddress;
  VOID                    *Base;
  VOID                    *Mapping;
  UINT32                  Capability;
  UINT32                  PortImplementBitMap;
  UINT8                   MaxPortNumber;
  UINT8                   MaxCommandSlotNumber;
  UINTN                   MaxRFisSize;
  UINTN                   MaxCmdListSize;
  UINTN                   MaxCmdTableSize;

  AhciBar       = Private->MmioBase;
  AhciRegisters = &Private->AhciRegisters;

  //
  // Collect AHCI controller information
  //
  Capability = AhciReadReg (AhciBar, AHCI_CAPABILITY_OFFSET);

  //
  // Get the number of command slots per port supported by this HBA.
  //
  MaxCommandSlotNumber = (UINT8) (((Capability & 0x1F00) >> 8) + 1);
  ASSERT (MaxCommandSlotNumber > 0);
  if (MaxCommandSlotNumber == 0) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Get the highest bit of implemented ports which decides how many bytes are
  // allocated for recived FIS.
  //
  PortImplementBitMap = AhciReadReg (AhciBar, AHCI_PI_OFFSET);
  MaxPortNumber       = (UINT8)(UINTN)(HighBitSet32(PortImplementBitMap) + 1);
  if (MaxPortNumber == 0) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Get the number of ports that actually needed to be initialized.
  //
  MaxPortNumber = MIN (MaxPortNumber, AhciGetNumberOfPortsFromMap (Private->PortBitMap));

  //
  // Allocate memory for received FIS.
  //
  MaxRFisSize = MaxPortNumber * sizeof (EFI_AHCI_RECEIVED_FIS);
  Status = IoMmuAllocateBuffer (
             EFI_SIZE_TO_PAGES (MaxRFisSize),
             &Base,
             &DeviceAddress,
             &Mapping
             );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }
  ASSERT (DeviceAddress == ((EFI_PHYSICAL_ADDRESS) (UINTN) Base));
  AhciRegisters->AhciRFis    = Base;
  AhciRegisters->AhciRFisMap = Mapping;
  AhciRegisters->MaxRFisSize = MaxRFisSize;
  ZeroMem (AhciRegisters->AhciRFis, EFI_PAGE_SIZE * EFI_SIZE_TO_PAGES (MaxRFisSize));

  //
  // Allocate memory for command list.
  // Note that the implemenation is a single task model which only use a command
  // list for each port.
  //
  MaxCmdListSize = 1 * sizeof (EFI_AHCI_COMMAND_LIST);
  Status = IoMmuAllocateBuffer (
             EFI_SIZE_TO_PAGES (MaxCmdListSize),
             &Base,
             &DeviceAddress,
             &Mapping
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }
  ASSERT (DeviceAddress == ((EFI_PHYSICAL_ADDRESS) (UINTN) Base));
  AhciRegisters->AhciCmdList    = Base;
  AhciRegisters->AhciCmdListMap = Mapping;
  AhciRegisters->MaxCmdListSize = MaxCmdListSize;
  ZeroMem (AhciRegisters->AhciCmdList, EFI_PAGE_SIZE * EFI_SIZE_TO_PAGES (MaxCmdListSize));

  //
  // Allocate memory for command table
  // According to AHCI 1.3 spec, a PRD table can contain maximum 65535 entries.
  //
  MaxCmdTableSize = sizeof (EFI_AHCI_COMMAND_TABLE);
  Status = IoMmuAllocateBuffer (
             EFI_SIZE_TO_PAGES (MaxCmdTableSize),
             &Base,
             &DeviceAddress,
             &Mapping
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }
  ASSERT (DeviceAddress == ((EFI_PHYSICAL_ADDRESS) (UINTN) Base));
  AhciRegisters->AhciCmdTable    = Base;
  AhciRegisters->AhciCmdTableMap = Mapping;
  AhciRegisters->MaxCmdTableSize = MaxCmdTableSize;
  ZeroMem (AhciRegisters->AhciCmdTable, EFI_PAGE_SIZE * EFI_SIZE_TO_PAGES (MaxCmdTableSize));

  return EFI_SUCCESS;

ErrorExit:
  if (AhciRegisters->AhciRFisMap != NULL) {
    IoMmuFreeBuffer (
       EFI_SIZE_TO_PAGES (AhciRegisters->MaxRFisSize),
       AhciRegisters->AhciRFis,
       AhciRegisters->AhciRFisMap
       );
    AhciRegisters->AhciRFis = NULL;
  }

  if (AhciRegisters->AhciCmdListMap != NULL) {
    IoMmuFreeBuffer (
       EFI_SIZE_TO_PAGES (AhciRegisters->MaxCmdListSize),
       AhciRegisters->AhciCmdList,
       AhciRegisters->AhciCmdListMap
       );
    AhciRegisters->AhciCmdList = NULL;
  }

  return Status;
}

/**
  Gets ATA device Capacity according to ATA 6.

  This function returns the capacity of the ATA device if it follows
  ATA 6 to support 48 bit addressing.

  @param[in] IdentifyData    A pointer to ATA_IDENTIFY_DATA structure.

  @return The capacity of the ATA device or 0 if the device does not support
          48-bit addressing defined in ATA 6.

**/
EFI_LBA
GetAtapi6Capacity (
  IN ATA_IDENTIFY_DATA    *IdentifyData
  )
{
  EFI_LBA                       Capacity;
  EFI_LBA                       TmpLba;
  UINTN                         Index;

  if ((IdentifyData->command_set_supported_83 & BIT10) == 0) {
    //
    // The device doesn't support 48 bit addressing
    //
    return 0;
  }

  //
  // 48 bit address feature set is supported, get maximum capacity
  //
  Capacity = 0;
  for (Index = 0; Index < 4; Index++) {
    //
    // Lower byte goes first: word[100] is the lowest word, word[103] is highest
    //
    TmpLba = IdentifyData->maximum_lba_for_48bit_addressing[Index];
    Capacity |= LShiftU64 (TmpLba, 16 * Index);
  }

  return Capacity;
}

/**
  Identifies ATA device via the Identify data.

  This function identifies the ATA device and initializes the media information.

  @attention This is boundary function that may receive untrusted input.
  @attention The input is from peripheral hardware device.

  The Identify Drive command response data from an ATA device is the peripheral
  hardware input, so this routine will do basic validation for the Identify Drive
  command response data.

  @param[in,out] DeviceData    A pointer to PEI_AHCI_ATA_DEVICE_DATA structure.

  @retval EFI_SUCCESS        The device is successfully identified and media
                             information is correctly initialized.
  @retval EFI_UNSUPPORTED    The device is not a valid ATA device (hard disk).

**/
EFI_STATUS
IdentifyAtaDevice (
  IN OUT PEI_AHCI_ATA_DEVICE_DATA    *DeviceData
  )
{
  ATA_IDENTIFY_DATA          *IdentifyData;
  EFI_PEI_BLOCK_IO2_MEDIA    *Media;
  EFI_LBA                    Capacity;
  UINT32                     MaxSectorCount;
  UINT16                     PhyLogicSectorSupport;

  IdentifyData = DeviceData->IdentifyData;
  Media        = &DeviceData->Media;

  if ((IdentifyData->config & BIT15) != 0) {
    DEBUG ((
      DEBUG_ERROR, "%a: Not a hard disk device on Port 0x%x PortMultiplierPort 0x%x\n",
      __FUNCTION__, DeviceData->Port, DeviceData->PortMultiplier
      ));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((
    DEBUG_INFO, "%a: Identify Device: Port 0x%x PortMultiplierPort 0x%x\n",
    __FUNCTION__, DeviceData->Port, DeviceData->PortMultiplier
    ));

  //
  // Skip checking whether the WORD 88 (supported UltraDMA by drive), since the
  // driver only support PIO data transfer for now.
  //

  //
  // Get the capacity information of the device.
  //
  Capacity = GetAtapi6Capacity (IdentifyData);
  if (Capacity > MAX_28BIT_ADDRESSING_CAPACITY) {
    //
    // Capacity exceeds 120GB. 48-bit addressing is really needed
    //
    DeviceData->Lba48Bit = TRUE;
  } else {
    //
    // This is a hard disk <= 120GB capacity, treat it as normal hard disk
    //
    Capacity = ((UINT32)IdentifyData->user_addressable_sectors_hi << 16) |
                        IdentifyData->user_addressable_sectors_lo;
    DeviceData->Lba48Bit = FALSE;
  }

  if (Capacity == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Capacity (0) for ATA device.\n", __FUNCTION__));
    return EFI_UNSUPPORTED;
  }
  Media->LastBlock = (EFI_PEI_LBA) (Capacity - 1);

  Media->BlockSize = 0x200;
  //
  // Check whether Long Physical Sector Feature is supported
  //
  PhyLogicSectorSupport = IdentifyData->phy_logic_sector_support;
  DEBUG ((
    DEBUG_INFO, "%a: PhyLogicSectorSupport = 0x%x\n",
    __FUNCTION__, PhyLogicSectorSupport
    ));
  if ((PhyLogicSectorSupport & (BIT14 | BIT15)) == BIT14) {
    //
    // Check logical block size
    //
    if ((PhyLogicSectorSupport & BIT12) != 0) {
      Media->BlockSize = (UINT32) (((IdentifyData->logic_sector_size_hi << 16) |
                                     IdentifyData->logic_sector_size_lo) * sizeof (UINT16));
    }
  }

  //
  // Check BlockSize validity
  //
  MaxSectorCount = mMaxTransferBlockNumber[DeviceData->Lba48Bit];
  if ((Media->BlockSize == 0) || (Media->BlockSize > MAX_UINT32 / MaxSectorCount)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid BlockSize (0x%x).\n", __FUNCTION__, Media->BlockSize));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((
    DEBUG_INFO, "%a: BlockSize = 0x%x, LastBlock = 0x%lx\n",
    __FUNCTION__, Media->BlockSize, Media->LastBlock
    ));

  if ((IdentifyData->trusted_computing_support & BIT0) != 0) {
    DEBUG ((DEBUG_INFO, "%a: Found Trust Computing feature support.\n", __FUNCTION__));
    DeviceData->TrustComputing = TRUE;
  }

  Media->InterfaceType  = MSG_SATA_DP;
  Media->RemovableMedia = FALSE;
  Media->MediaPresent   = TRUE;
  Media->ReadOnly       = FALSE;

  return EFI_SUCCESS;
}

/**
  Allocate device information data structure to contain device information.
  And insert the data structure to the tail of device list for tracing.

  @param[in,out] Private               A pointer to the PEI_AHCI_CONTROLLER_PRIVATE_DATA
                                       instance.
  @param[in]     DeviceIndex           The device index.
  @param[in]     Port                  The port number of the ATA device to send
                                       the command.
  @param[in]     PortMultiplierPort    The port multiplier port number of the ATA
                                       device to send the command.
                                       If there is no port multiplier, then specify
                                       0xFFFF.
  @param[in]     FisIndex              The index of the FIS of the ATA device to
                                       send the command.
  @param[in]     IdentifyData          The data buffer to store the output of the
                                       IDENTIFY command.

  @retval EFI_SUCCESS                  Successfully insert the ATA device to the
                                       tail of device list.
  @retval EFI_OUT_OF_RESOURCES         Not enough resource.

**/
EFI_STATUS
CreateNewDevice (
  IN OUT PEI_AHCI_CONTROLLER_PRIVATE_DATA    *Private,
  IN     UINTN                               DeviceIndex,
  IN     UINT16                              Port,
  IN     UINT16                              PortMultiplier,
  IN     UINT8                               FisIndex,
  IN     ATA_IDENTIFY_DATA                   *IdentifyData
  )
{
  PEI_AHCI_ATA_DEVICE_DATA    *DeviceData;
  EFI_STATUS                  Status;

  DeviceData = AllocateZeroPool (sizeof (PEI_AHCI_ATA_DEVICE_DATA));
  if (DeviceData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (IdentifyData != NULL) {
    DeviceData->IdentifyData = AllocateCopyPool (sizeof (ATA_IDENTIFY_DATA), IdentifyData);
    if (DeviceData->IdentifyData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  DeviceData->Signature      = AHCI_PEI_ATA_DEVICE_DATA_SIGNATURE;
  DeviceData->Port           = Port;
  DeviceData->PortMultiplier = PortMultiplier;
  DeviceData->FisIndex       = FisIndex;
  DeviceData->DeviceIndex    = DeviceIndex;
  DeviceData->Private        = Private;

  Status = IdentifyAtaDevice (DeviceData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (DeviceData->TrustComputing) {
    Private->TrustComputingDevices++;
    DeviceData->TrustComputingDeviceIndex = Private->TrustComputingDevices;
  }
  Private->ActiveDevices++;
  InsertTailList (&Private->DeviceList, &DeviceData->Link);

  return EFI_SUCCESS;
}

/**
  Initialize ATA host controller at AHCI mode.

  The function is designed to initialize ATA host controller.

  @param[in,out] Private    A pointer to the PEI_AHCI_CONTROLLER_PRIVATE_DATA instance.

  @retval EFI_SUCCESS             The ATA AHCI controller is initialized successfully.
  @retval EFI_OUT_OF_RESOURCES    Not enough resource to complete while initializing
                                  the controller.
  @retval Others                  A device error occurred while initializing the
                                  controller.

**/
EFI_STATUS
AhciModeInitialization (
  IN OUT PEI_AHCI_CONTROLLER_PRIVATE_DATA    *Private
  )
{
  EFI_STATUS            Status;
  UINTN                 AhciBar;
  UINT32                Capability;
  UINT32                Value;
  UINT8                 MaxPortNumber;
  UINT32                PortImplementBitMap;
  UINT32                PortInitializeBitMap;
  EFI_AHCI_REGISTERS    *AhciRegisters;
  UINT8                 PortIndex;
  UINT8                 Port;
  DATA_64               Data64;
  UINT32                Data;
  UINT32                Offset;
  UINT32                PhyDetectDelay;
  UINTN                 DeviceIndex;
  ATA_IDENTIFY_DATA     IdentifyData;

  AhciBar = Private->MmioBase;

  Status = AhciReset (AhciBar, AHCI_PEI_RESET_TIMEOUT);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AHCI HBA reset failed with %r.\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  //
  // Collect AHCI controller information
  //
  Capability = AhciReadReg (AhciBar, AHCI_CAPABILITY_OFFSET);

  //
  // Make sure that GHC.AE bit is set before accessing any AHCI registers.
  //
  Value = AhciReadReg (AhciBar, AHCI_GHC_OFFSET);
  if ((Value & AHCI_GHC_ENABLE) == 0) {
    AhciOrReg (AhciBar, AHCI_GHC_OFFSET, AHCI_GHC_ENABLE);
  }

  Status = AhciCreateTransferDescriptor (Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Transfer-related data allocation failed with %r.\n",
      __FUNCTION__, Status
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Get the number of command slots per port supported by this HBA.
  //
  MaxPortNumber = (UINT8) ((Capability & 0x1F) + 1);

  //
  // Get the bit map of those ports exposed by this HBA.
  // It indicates which ports that the HBA supports are available for software
  // to use.
  //
  PortImplementBitMap = AhciReadReg (AhciBar, AHCI_PI_OFFSET);

  //
  // Get the number of ports that actually needed to be initialized.
  //
  MaxPortNumber = MIN (MaxPortNumber, (UINT8)(UINTN)(HighBitSet32(PortImplementBitMap) + 1));
  MaxPortNumber = MIN (MaxPortNumber, AhciGetNumberOfPortsFromMap (Private->PortBitMap));

  PortInitializeBitMap = Private->PortBitMap & PortImplementBitMap;
  AhciRegisters        = &Private->AhciRegisters;
  DeviceIndex          = 0;
  //
  // Enumerate ATA ports
  //
  for (PortIndex = 1; PortIndex <= MaxPortNumber; PortIndex ++) {
    Status = AhciGetPortFromMap (PortInitializeBitMap, PortIndex, &Port);
    if (EFI_ERROR (Status)) {
      //
      // No more available port, just break out of the loop.
      //
      break;
    }

    if ((PortImplementBitMap & (BIT0 << Port)) != 0) {
      //
      // Initialize FIS Base Address Register and Command List Base Address
      // Register for use.
      //
      Data64.Uint64 = (UINTN) (AhciRegisters->AhciRFis) +
                      sizeof (EFI_AHCI_RECEIVED_FIS) * (PortIndex - 1);
      Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_FB;
      AhciWriteReg (AhciBar, Offset, Data64.Uint32.Lower32);
      Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_FBU;
      AhciWriteReg (AhciBar, Offset, Data64.Uint32.Upper32);

      Data64.Uint64 = (UINTN) (AhciRegisters->AhciCmdList);
      Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CLB;
      AhciWriteReg (AhciBar, Offset, Data64.Uint32.Lower32);
      Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CLBU;
      AhciWriteReg (AhciBar, Offset, Data64.Uint32.Upper32);

      Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD;
      Data = AhciReadReg (AhciBar, Offset);
      if ((Data & AHCI_PORT_CMD_CPD) != 0) {
        AhciOrReg (AhciBar, Offset, AHCI_PORT_CMD_POD);
      }

      if ((Capability & AHCI_CAP_SSS) != 0) {
        AhciOrReg (AhciBar, Offset, AHCI_PORT_CMD_SUD);
      }

      //
      // Disable aggressive power management.
      //
      Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_SCTL;
      AhciOrReg (AhciBar, Offset, AHCI_PORT_SCTL_IPM_INIT);
      //
      // Disable the reporting of the corresponding interrupt to system software.
      //
      Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_IE;
      AhciAndReg (AhciBar, Offset, 0);

      //
      // Enable FIS Receive DMA engine for the first D2H FIS.
      //
      Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD;
      AhciOrReg (AhciBar, Offset, AHCI_PORT_CMD_FRE);

      //
      // Wait no longer than 15 ms to wait the Phy to detect the presence of a device.
      //
      PhyDetectDelay = AHCI_BUS_PHY_DETECT_TIMEOUT;
      Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_SSTS;
      do {
        Data = AhciReadReg (AhciBar, Offset) & AHCI_PORT_SSTS_DET_MASK;
        if ((Data == AHCI_PORT_SSTS_DET_PCE) || (Data == AHCI_PORT_SSTS_DET)) {
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
        Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD;
        AhciAndReg (AhciBar, Offset, (UINT32) ~(AHCI_PORT_CMD_SUD));
        DEBUG ((DEBUG_ERROR, "%a: No device detected at Port %d.\n", __FUNCTION__, Port));
        continue;
      }

      //
      // According to SATA1.0a spec section 5.2, we need to wait for PxTFD.BSY and PxTFD.DRQ
      // and PxTFD.ERR to be zero. The maximum wait time is 16s which is defined at ATA spec.
      //
      PhyDetectDelay = 16 * 1000;
      do {
        Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_SERR;
        if (AhciReadReg(AhciBar, Offset) != 0) {
          AhciWriteReg (AhciBar, Offset, AhciReadReg (AhciBar, Offset));
        }
        Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_TFD;

        Data = AhciReadReg (AhciBar, Offset) & AHCI_PORT_TFD_MASK;
        if (Data == 0) {
          break;
        }

        MicroSecondDelay (1000);
        PhyDetectDelay--;
      } while (PhyDetectDelay > 0);

      if (PhyDetectDelay == 0) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Port %d device presence detected but phy not ready (TFD=0x%x).\n",
          __FUNCTION__, Port, Data
          ));
        continue;
      }

      //
      // When the first D2H register FIS is received, the content of PxSIG register is updated.
      //
      Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_SIG;
      Status = AhciWaitMmioSet (
                 AhciBar,
                 Offset,
                 0x0000FFFF,
                 0x00000101,
                 160000000
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Error occurred when waiting for the first D2H register FIS - %r\n",
          __FUNCTION__, Status
          ));
        continue;
      }

      Data = AhciReadReg (AhciBar, Offset);
      if ((Data & AHCI_ATAPI_SIG_MASK) == AHCI_ATA_DEVICE_SIG) {
        Status = AhciIdentify (Private, Port, 0, PortIndex - 1, &IdentifyData);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "%a: AhciIdentify() failed with %r\n", __FUNCTION__, Status));
          continue;
        }
        DEBUG ((DEBUG_INFO, "%a: ATA hard disk found on Port %d.\n", __FUNCTION__, Port));
      } else {
        continue;
      }

      //
      // Found an ATA hard disk device, add it into the device list.
      //
      DeviceIndex++;
      CreateNewDevice (
        Private,
        DeviceIndex,
        Port,
        0xFFFF,
        PortIndex - 1,
        &IdentifyData
        );
    }
  }

  return EFI_SUCCESS;
}

/**
  Transfer data from ATA device.

  This function performs one ATA pass through transaction to transfer data from/to
  ATA device. It chooses the appropriate ATA command and protocol to invoke PassThru
  interface of ATA pass through.

  @param[in]     DeviceData        A pointer to PEI_AHCI_ATA_DEVICE_DATA structure.
  @param[in,out] Buffer            The pointer to the current transaction buffer.
  @param[in]     StartLba          The starting logical block address to be accessed.
  @param[in]     TransferLength    The block number or sector count of the transfer.
  @param[in]     IsWrite           Indicates whether it is a write operation.

  @retval EFI_SUCCESS    The data transfer is complete successfully.
  @return others         Some error occurs when transferring data.

**/
EFI_STATUS
TransferAtaDevice (
  IN     PEI_AHCI_ATA_DEVICE_DATA    *DeviceData,
  IN OUT VOID                        *Buffer,
  IN     EFI_LBA                     StartLba,
  IN     UINT32                      TransferLength,
  IN     BOOLEAN                     IsWrite
  )
{
  PEI_AHCI_CONTROLLER_PRIVATE_DATA    *Private;
  EDKII_PEI_ATA_PASS_THRU_PPI         *AtaPassThru;
  EFI_ATA_COMMAND_BLOCK               Acb;
  EFI_ATA_PASS_THRU_COMMAND_PACKET    Packet;

  Private     = DeviceData->Private;
  AtaPassThru = &Private->AtaPassThruPpi;

  //
  // Ensure Lba48Bit and IsWrite are valid boolean values
  //
  ASSERT ((UINTN) DeviceData->Lba48Bit < 2);
  ASSERT ((UINTN) IsWrite < 2);
  if (((UINTN) DeviceData->Lba48Bit >= 2) ||
      ((UINTN) IsWrite >= 2)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Prepare for ATA command block.
  //
  ZeroMem (&Acb, sizeof (EFI_ATA_COMMAND_BLOCK));
  Acb.AtaCommand = mAtaCommands[DeviceData->Lba48Bit][IsWrite];
  Acb.AtaSectorNumber = (UINT8) StartLba;
  Acb.AtaCylinderLow  = (UINT8) RShiftU64 (StartLba, 8);
  Acb.AtaCylinderHigh = (UINT8) RShiftU64 (StartLba, 16);
  Acb.AtaDeviceHead   = (UINT8) (BIT7 | BIT6 | BIT5 |
                                 (DeviceData->PortMultiplier == 0xFFFF ?
                                 0 : (DeviceData->PortMultiplier << 4)));
  Acb.AtaSectorCount  = (UINT8) TransferLength;
  if (DeviceData->Lba48Bit) {
    Acb.AtaSectorNumberExp = (UINT8) RShiftU64 (StartLba, 24);
    Acb.AtaCylinderLowExp  = (UINT8) RShiftU64 (StartLba, 32);
    Acb.AtaCylinderHighExp = (UINT8) RShiftU64 (StartLba, 40);
    Acb.AtaSectorCountExp  = (UINT8) (TransferLength >> 8);
  } else {
    Acb.AtaDeviceHead      = (UINT8) (Acb.AtaDeviceHead | RShiftU64 (StartLba, 24));
  }

  //
  // Prepare for ATA pass through packet.
  //
  ZeroMem (&Packet, sizeof (EFI_ATA_PASS_THRU_COMMAND_PACKET));
  if (IsWrite) {
    Packet.OutDataBuffer     = Buffer;
    Packet.OutTransferLength = TransferLength;
  } else {
    Packet.InDataBuffer      = Buffer;
    Packet.InTransferLength  = TransferLength;
  }
  Packet.Asb      = NULL;
  Packet.Acb      = &Acb;
  Packet.Protocol = mAtaPassThruCmdProtocols[IsWrite];
  Packet.Length   = EFI_ATA_PASS_THRU_LENGTH_SECTOR_COUNT;
  //
  // |------------------------|-----------------|
  // | ATA PIO Transfer Mode  |  Transfer Rate  |
  // |------------------------|-----------------|
  // |       PIO Mode 0       |  3.3Mbytes/sec  |
  // |------------------------|-----------------|
  // |       PIO Mode 1       |  5.2Mbytes/sec  |
  // |------------------------|-----------------|
  // |       PIO Mode 2       |  8.3Mbytes/sec  |
  // |------------------------|-----------------|
  // |       PIO Mode 3       | 11.1Mbytes/sec  |
  // |------------------------|-----------------|
  // |       PIO Mode 4       | 16.6Mbytes/sec  |
  // |------------------------|-----------------|
  //
  // As AtaBus is used to manage ATA devices, we have to use the lowest transfer
  // rate to calculate the possible maximum timeout value for each read/write
  // operation. The timout value is rounded up to nearest integar and here an
  // additional 30s is added to follow ATA spec in which it mentioned that the
  // device may take up to 30s to respond commands in the Standby/Idle mode.
  //
  // Calculate the maximum timeout value for PIO read/write operation.
  //
  Packet.Timeout = TIMER_PERIOD_SECONDS (
                     DivU64x32 (
                       MultU64x32 (TransferLength, DeviceData->Media.BlockSize),
                       3300000
                       ) + 31
                     );

  return AtaPassThru->PassThru (
                        AtaPassThru,
                        DeviceData->Port,
                        DeviceData->PortMultiplier,
                        &Packet
                        );
}

/**
  Trust transfer data from/to ATA device.

  This function performs one ATA pass through transaction to do a trust transfer
  from/to ATA device. It chooses the appropriate ATA command and protocol to invoke
  PassThru interface of ATA pass through.

  @param[in]     DeviceData     Pointer to PEI_AHCI_ATA_DEVICE_DATA structure.
  @param[in,out] Buffer         The pointer to the current transaction buffer.
  @param[in]     SecurityProtocolId
                                The value of the "Security Protocol" parameter
                                of the security protocol command to be sent.
  @param[in]     SecurityProtocolSpecificData
                                The value of the "Security Protocol Specific"
                                parameter of the security protocol command to
                                be sent.
  @param[in]     TransferLength The block number or sector count of the transfer.
  @param[in]     IsTrustSend    Indicates whether it is a trust send operation
                                or not.
  @param[in]     Timeout        The timeout, in 100ns units, to use for the execution
                                of the security protocol command. A Timeout value
                                of 0 means that this function will wait indefinitely
                                for the security protocol command to execute. If
                                Timeout is greater than zero, then this function
                                will return EFI_TIMEOUT if the time required to
                                execute the receive data command is greater than
                                Timeout.
  @param[out]    TransferLengthOut
                                A pointer to a buffer to store the size in bytes
                                of the data written to the buffer. Ignore it when
                                IsTrustSend is TRUE.

  @retval EFI_SUCCESS    The data transfer is complete successfully.
  @return others         Some error occurs when transferring data.

**/
EFI_STATUS
TrustTransferAtaDevice (
  IN     PEI_AHCI_ATA_DEVICE_DATA    *DeviceData,
  IN OUT VOID                        *Buffer,
  IN     UINT8                       SecurityProtocolId,
  IN     UINT16                      SecurityProtocolSpecificData,
  IN     UINTN                       TransferLength,
  IN     BOOLEAN                     IsTrustSend,
  IN     UINT64                      Timeout,
  OUT    UINTN                       *TransferLengthOut
  )
{
  PEI_AHCI_CONTROLLER_PRIVATE_DATA    *Private;
  EDKII_PEI_ATA_PASS_THRU_PPI         *AtaPassThru;
  EFI_ATA_COMMAND_BLOCK               Acb;
  EFI_ATA_PASS_THRU_COMMAND_PACKET    Packet;
  EFI_STATUS                          Status;
  VOID                                *NewBuffer;

  Private     = DeviceData->Private;
  AtaPassThru = &Private->AtaPassThruPpi;

  //
  // Ensure IsTrustSend are valid boolean values
  //
  ASSERT ((UINTN) IsTrustSend < 2);
  if ((UINTN) IsTrustSend >= 2) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Prepare for ATA command block.
  //
  ZeroMem (&Acb, sizeof (EFI_ATA_COMMAND_BLOCK));
  if (TransferLength == 0) {
    Acb.AtaCommand    = ATA_CMD_TRUST_NON_DATA;
  } else {
    Acb.AtaCommand    = mAtaTrustCommands[IsTrustSend];
  }
  Acb.AtaFeatures      = SecurityProtocolId;
  Acb.AtaSectorCount   = (UINT8) (TransferLength / 512);
  Acb.AtaSectorNumber  = (UINT8) ((TransferLength / 512) >> 8);
  //
  // NOTE: ATA Spec has no explicitly definition for Security Protocol Specific layout.
  // Here use big endian for Cylinder register.
  //
  Acb.AtaCylinderHigh  = (UINT8) SecurityProtocolSpecificData;
  Acb.AtaCylinderLow   = (UINT8) (SecurityProtocolSpecificData >> 8);
  Acb.AtaDeviceHead    = (UINT8) (BIT7 | BIT6 | BIT5 |
                                  (DeviceData->PortMultiplier == 0xFFFF ?
                                  0 : (DeviceData->PortMultiplier << 4)));

  //
  // Prepare for ATA pass through packet.
  //
  ZeroMem (&Packet, sizeof (EFI_ATA_PASS_THRU_COMMAND_PACKET));
  if (TransferLength == 0) {
    Packet.InTransferLength  = 0;
    Packet.OutTransferLength = 0;
    Packet.Protocol = EFI_ATA_PASS_THRU_PROTOCOL_ATA_NON_DATA;
  } else if (IsTrustSend) {
    //
    // Check the alignment of the incoming buffer prior to invoking underlying
    // ATA PassThru PPI.
    //
    if ((AtaPassThru->Mode->IoAlign > 1) &&
        !IS_ALIGNED (Buffer, AtaPassThru->Mode->IoAlign)) {
      NewBuffer = AllocateAlignedPages (
                    EFI_SIZE_TO_PAGES (TransferLength),
                    AtaPassThru->Mode->IoAlign
                    );
      if (NewBuffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      CopyMem (NewBuffer, Buffer, TransferLength);
      Buffer = NewBuffer;
    }
    Packet.OutDataBuffer = Buffer;
    Packet.OutTransferLength = (UINT32) TransferLength;
    Packet.Protocol = mAtaPassThruCmdProtocols[IsTrustSend];
  } else {
    Packet.InDataBuffer = Buffer;
    Packet.InTransferLength = (UINT32) TransferLength;
    Packet.Protocol = mAtaPassThruCmdProtocols[IsTrustSend];
  }
  Packet.Asb      = NULL;
  Packet.Acb      = &Acb;
  Packet.Timeout  = Timeout;
  Packet.Length   = EFI_ATA_PASS_THRU_LENGTH_BYTES;

  Status = AtaPassThru->PassThru (
                          AtaPassThru,
                          DeviceData->Port,
                          DeviceData->PortMultiplier,
                          &Packet
                          );
  if (TransferLengthOut != NULL) {
    if (!IsTrustSend) {
      *TransferLengthOut = Packet.InTransferLength;
    }
  }
  return Status;
}
