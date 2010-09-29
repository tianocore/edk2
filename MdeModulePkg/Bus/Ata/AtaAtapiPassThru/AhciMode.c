/** @file
  The file for AHCI mode of ATA host controller.
  
  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

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
  UINT32                  Data;

  ASSERT (PciIo != NULL);
  
  Data = 0;

  PciIo->Mem.Read (
               PciIo,
               EfiPciIoWidthUint32,
               EFI_AHCI_BAR_INDEX,
               (UINT64) Offset,
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
               (UINT64) Offset,
               1,
               &Data
               );

  return ;
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
  UINT32 Data;
  
  ASSERT (PciIo != NULL);

  Data  = AhciReadReg (PciIo, Offset);

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
  UINT32 Data;

  ASSERT (PciIo != NULL);

  Data  = AhciReadReg (PciIo, Offset);

  Data |= OrData;

  AhciWriteReg (PciIo, Offset, Data);
}

/**
  Wait for memory set to the test value.
    
  @param  PciIo      The PCI IO protocol instance.
  @param  Offset         The memory address to test.
  @param  MaskValue      The mask value of memory.
  @param  TestValue      The test value of memory.
  @param  Timeout        The time out value for wait memory set.

  @retval EFI_DEVICE_ERROR  The memory is not set.
  @retval EFI_TIMEOUT       The memory setting is time out.
  @retval EFI_SUCCESS       The memory is correct set.

**/
EFI_STATUS
EFIAPI
AhciWaitMemSet (
  IN  EFI_PCI_IO_PROTOCOL       *PciIo,
  IN  UINT32                    Offset,
  IN  UINT32                    MaskValue,
  IN  UINT32                    TestValue,
  IN  UINT64                    Timeout
  )
{
  UINT32     Value;  
  UINT32     Delay;

  Delay = (UINT32) (DivU64x32(Timeout, 1000) + 1);

  do {
    Value = AhciReadReg (PciIo, Offset) & MaskValue;

    if (Value == TestValue) {
      return EFI_SUCCESS;
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

  return EFI_DEVICE_ERROR;
}

/**
  Check if the device is still on port. It also checks if the AHCI controller 
  supports the address and data count will be transfered.

  @param  PciIo      The PCI IO protocol instance.
  @param  Port           The number of port.

  @retval EFI_SUCCESS      The device is attached to port and the transfer data is 
                           supported by AHCI controller.
  @retval EFI_UNSUPPORTED  The transfer address and count is not supported by AHCI
                           controller.
  @retval EFI_NOT_READY    The physical communication between AHCI controller and device
                           is not ready.

**/
EFI_STATUS
EFIAPI
AhciCheckDeviceStatus (
  IN  EFI_PCI_IO_PROTOCOL    *PciIo,
  IN  UINT8                  Port
  )
{
  UINT32      Data; 
  UINT32      Offset;

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;

  Data   = AhciReadReg (PciIo, Offset) & EFI_AHCI_PORT_SSTS_DET_MASK;

  if (Data == EFI_AHCI_PORT_SSTS_DET_PCE) {
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
  IN  EFI_PCI_IO_PROTOCOL    *PciIo,
  IN  UINT8                  Port
  )  
{
  UINT32 Offset;

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
  Enable the FIS running for giving port.
    
  @param      PciIo          The PCI IO protocol instance.
  @param      Port           The number of port.
  @param      Timeout        The timeout value of enabling FIS.

  @retval EFI_DEVICE_ERROR   The FIS enable setting fails.
  @retval EFI_TIMEOUT        The FIS enable setting is time out.
  @retval EFI_SUCCESS        The FIS enable successfully.

**/
EFI_STATUS
EFIAPI
AhciEnableFisReceive (
  IN  EFI_PCI_IO_PROTOCOL       *PciIo,
  IN  UINT8                     Port,
  IN  UINT64                    Timeout
  )     
{ 
  UINT32 Offset;

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
  AhciOrReg (PciIo, Offset, EFI_AHCI_PORT_CMD_FRE);

  return AhciWaitMemSet (
           PciIo, 
           Offset,
           EFI_AHCI_PORT_CMD_FR,
           EFI_AHCI_PORT_CMD_FR,
           Timeout
           );
}

/**
  Disable the FIS running for giving port.

  @param      PciIo          The PCI IO protocol instance.
  @param      Port           The number of port.
  @param      Timeout        The timeout value of disabling FIS.

  @retval EFI_DEVICE_ERROR   The FIS disable setting fails.
  @retval EFI_TIMEOUT        The FIS disable setting is time out.
  @retval EFI_UNSUPPORTED    The port is in running state.
  @retval EFI_SUCCESS        The FIS disable successfully.

**/
EFI_STATUS
EFIAPI
AhciDisableFisReceive (
  IN  EFI_PCI_IO_PROTOCOL       *PciIo,
  IN  UINT8                     Port,
  IN  UINT64                    Timeout
  )  
{
  UINT32 Offset;
  UINT32 Data;

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

  AhciAndReg (PciIo, Offset, (UINT32)~(EFI_AHCI_PORT_CMD_FRE));

  return AhciWaitMemSet (
           PciIo, 
           Offset,
           EFI_AHCI_PORT_CMD_FR,
           0,
           Timeout
           ); 
}



/**
  Build the command list, command table and prepare the fis receiver.
    
  @param    PciIo         The PCI IO protocol instance.
  @param    AhciRegisters         The pointer to the EFI_AHCI_REGISTERS.
  @param    Port          The number of port.
  @param    PortMultiplier    The timeout value of stop.
  @param    CommandFis      The control fis will be used for the transfer.
  @param    CommandList     The command list will be used for the transfer.
  @param    AtapiCommand      The atapi command will be used for the transfer.
  @param    AtapiCommandLength  The length of the atapi command.
  @param    CommandSlotNumber   The command slot will be used for the transfer.
  @param    DataPhysicalAddr      The pointer to the data buffer pci bus master address.
  @param    DataLength            The data count to be transferred.

**/  
VOID
EFIAPI
AhciBuildCommand (
  IN     EFI_PCI_IO_PROTOCOL        *PciIo,
  IN     EFI_AHCI_REGISTERS         *AhciRegisters,
  IN     UINT8                      Port,
  IN     UINT8                      PortMultiplier,
  IN     EFI_AHCI_COMMAND_FIS       *CommandFis,
  IN     EFI_AHCI_COMMAND_LIST      *CommandList,
  IN     EFI_AHCI_ATAPI_COMMAND     *AtapiCommand OPTIONAL,
  IN     UINT8                      AtapiCommandLength,
  IN     UINT8                      CommandSlotNumber,
  IN OUT VOID                       *DataPhysicalAddr,
  IN     UINT64                     DataLength
  )   
{
  UINT64     BaseAddr; 
  UINT64     PrdtNumber;
  UINT64     PrdtIndex;
  UINTN      RemainedData;
  UINTN      MemAddr;
  DATA_64    Data64;
  UINT32     Offset;

  //
  // Filling the PRDT
  //  
  PrdtNumber = (DataLength + EFI_AHCI_MAX_DATA_PER_PRDT - 1) / EFI_AHCI_MAX_DATA_PER_PRDT;

  //
  // According to AHCI 1.3 spec, a PRDT entry can point to a maximum 4MB data block.
  // It also limits that the maximum amount of the PRDT entry in the command table
  // is 65535.
  //
  ASSERT (PrdtNumber <= 65535);

  Data64.Uint64 = (UINTN) (AhciRegisters->AhciRFis) + sizeof (EFI_AHCI_RECEIVED_FIS) * Port;

  BaseAddr = Data64.Uint64;
  
  ZeroMem ((VOID *)((UINTN) BaseAddr), sizeof (EFI_AHCI_RECEIVED_FIS));  
    
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
    CommandList->AhciCmdC = (DataLength == 0) ? 1 : 0;

    AhciOrReg (PciIo, Offset, (EFI_AHCI_PORT_CMD_DLAE | EFI_AHCI_PORT_CMD_ATAPI));
  } else {
    AhciAndReg (PciIo, Offset, (UINT32)~(EFI_AHCI_PORT_CMD_DLAE | EFI_AHCI_PORT_CMD_ATAPI));
  }
  
  RemainedData = DataLength;
  MemAddr      = (UINTN) DataPhysicalAddr;
  CommandList->AhciCmdPrdtl = (UINT32)PrdtNumber;
  
  for (PrdtIndex = 0; PrdtIndex < PrdtNumber; PrdtIndex++) {
    if (RemainedData < EFI_AHCI_MAX_DATA_PER_PRDT) {     
      AhciRegisters->AhciCommandTable->PrdtTable[PrdtIndex].AhciPrdtDbc = (UINT32)RemainedData - 1;
    } else {
      AhciRegisters->AhciCommandTable->PrdtTable[PrdtIndex].AhciPrdtDbc = EFI_AHCI_MAX_DATA_PER_PRDT - 1;
    }

    Data64.Uint64 = (UINT64)MemAddr;
    AhciRegisters->AhciCommandTable->PrdtTable[PrdtIndex].AhciPrdtDba  = Data64.Uint32.Lower32;
    AhciRegisters->AhciCommandTable->PrdtTable[PrdtIndex].AhciPrdtDbau = Data64.Uint32.Upper32;
    RemainedData -= EFI_AHCI_MAX_DATA_PER_PRDT;    
    MemAddr      += EFI_AHCI_MAX_DATA_PER_PRDT;
  }

  //
  // Set the last PRDT to Interrupt On Complete
  //
  if (PrdtNumber > 0) {
    AhciRegisters->AhciCommandTable->PrdtTable[PrdtNumber - 1].AhciPrdtIoc = 1;
  }

  CopyMem (
    (VOID *) ((UINTN) AhciRegisters->AhciCmdList + (UINTN) CommandSlotNumber * sizeof (EFI_AHCI_COMMAND_LIST)),
    CommandList,
    sizeof (EFI_AHCI_COMMAND_LIST)
    );  

  Data64.Uint64 = (UINT64)(UINTN) AhciRegisters->AhciCommandTablePciAddr;
  AhciRegisters->AhciCmdList[CommandSlotNumber].AhciCmdCtba  = Data64.Uint32.Lower32;
  AhciRegisters->AhciCmdList[CommandSlotNumber].AhciCmdCtbau = Data64.Uint32.Upper32;
  AhciRegisters->AhciCmdList[CommandSlotNumber].AhciCmdPmp   = PortMultiplier;

}

/**
  Buid a command FIS.
    
  @param  CmdFis          A pointer to the EFI_AHCI_COMMAND_FIS data structure.
  @param  AtaCommandBlock   A pointer to the AhciBuildCommandFis data structure.

**/
VOID
EFIAPI
AhciBuildCommandFis (
  IN OUT EFI_AHCI_COMMAND_FIS    *CmdFis,
  IN     EFI_ATA_COMMAND_BLOCK   *AtaCommandBlock
  )
{
  ZeroMem (CmdFis, sizeof (EFI_AHCI_COMMAND_FIS));

  CmdFis->AhciCFisType = EFI_AHCI_FIS_REGISTER_H2D;
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

  CmdFis->AhciCFisDevHead     = AtaCommandBlock->AtaDeviceHead | 0xE0;
}

/**
  Start a PIO data transfer on specific port.
    
  @param  PciIo             The PCI IO protocol instance.
  @param  AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param  Port              The number of port.
  @param  PortMultiplier    The timeout value of stop.
  @param  AtapiCommand      The atapi command will be used for the transfer.
  @param  AtapiCommandLength  The length of the atapi command.
  @param  Read                The transfer direction.
  @param  AtaCommandBlock     The EFI_ATA_COMMAND_BLOCK data.
  @param  AtaStatusBlock      The EFI_ATA_STATUS_BLOCK data.
  @param  MemoryAddr          The pointer to the data buffer.
  @param  DataCount           The data count to be transferred.
  @param  Timeout             The timeout value of non data transfer.

  @retval EFI_DEVICE_ERROR    The PIO data transfer abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for transfer.
  @retval EFI_SUCCESS         The PIO data transfer executes successfully.

**/
EFI_STATUS
AhciPioTransfer (
  IN     EFI_PCI_IO_PROTOCOL        *PciIo,
  IN     EFI_AHCI_REGISTERS         *AhciRegisters,
  IN     UINT8                      Port,
  IN     UINT8                      PortMultiplier,
  IN     EFI_AHCI_ATAPI_COMMAND     *AtapiCommand OPTIONAL,
  IN     UINT8                      AtapiCommandLength,  
  IN     BOOLEAN                    Read,  
  IN     EFI_ATA_COMMAND_BLOCK      *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK       *AtaStatusBlock,
  IN OUT VOID                       *MemoryAddr,
  IN     UINT32                     DataCount,
  IN     UINT64                     Timeout 
  )
{
  EFI_STATUS                    Status;
  UINTN                         FisBaseAddr;
  UINT32                        Offset;
  UINT32                        Value;
  EFI_PHYSICAL_ADDRESS          PhyAddr;
  VOID                          *Map;
  UINTN                         MapLength;
  EFI_PCI_IO_PROTOCOL_OPERATION Flag;
  UINT32                        Delay;
  EFI_AHCI_COMMAND_FIS          CFis;
  EFI_AHCI_COMMAND_LIST         CmdList;

  if (Read) {
    Flag = EfiPciIoOperationBusMasterWrite;
  } else {
    Flag = EfiPciIoOperationBusMasterRead;
  }

  //
  // construct command list and command table with pci bus address
  //
  MapLength = DataCount;
  Status = PciIo->Map (
                    PciIo,
                    Flag,
                    MemoryAddr,
                    &MapLength,
                    &PhyAddr,
                    &Map
                    );

  if (EFI_ERROR (Status) || (DataCount != MapLength)) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  //
  // Package read needed
  //
  AhciBuildCommandFis (&CFis, AtaCommandBlock);

  ZeroMem (&CmdList, sizeof (EFI_AHCI_COMMAND_LIST));

  CmdList.AhciCmdCfl = EFI_AHCI_FIS_REGISTER_H2D_LENGTH / 4;
  CmdList.AhciCmdW   = Read ? 0 : 1;

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
  
  Status = AhciStartCommand (
             PciIo, 
             Port, 
             0,
             Timeout
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }
  
  //
  // Checking the status and wait the driver sending data
  //
  FisBaseAddr = (UINTN)AhciRegisters->AhciRFis + Port * sizeof (EFI_AHCI_RECEIVED_FIS);
  //
  // Wait device sends the PIO setup fis before data transfer
  //
  Delay = (UINT32) (DivU64x32(Timeout, 1000) + 1);
  do {
    Value = *(UINT32 *) (FisBaseAddr + EFI_AHCI_PIO_FIS_OFFSET);

    if ((Value & EFI_AHCI_FIS_TYPE_MASK) == EFI_AHCI_FIS_PIO_SETUP) {
      break;
    }

    //
    // Stall for 100 microseconds.
    //
    MicroSecondDelay(100);

    Delay--;    
  } while (Delay > 0);

  if (Delay == 0) {
    Status = EFI_TIMEOUT;
    goto Exit;
  }

  //
  // Wait for command compelte
  //
  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CI;
  Status = AhciWaitMemSet (
             PciIo,
             Offset,
             0xFFFFFFFF,
             0,
             Timeout
             );

  if (EFI_ERROR (Status)) {
    goto Exit;   
  }

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_IS;
  Status = AhciWaitMemSet (
             PciIo,
             Offset,            
             EFI_AHCI_PORT_IS_PSS,
             EFI_AHCI_PORT_IS_PSS,
             Timeout
             );  
  if (EFI_ERROR (Status)) {
    goto Exit;  
  }

Exit: 
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

  return Status;
}

/**
  Start a DMA data transfer on specific port

  @param  PciIo         The PCI IO protocol instance.
  @param  AhciRegisters         The pointer to the EFI_AHCI_REGISTERS.
  @param  Port          The number of port.
  @param  PortMultiplier      The timeout value of stop.
  @param  AtapiCommand        The atapi command will be used for the transfer.
  @param  AtapiCommandLength    The length of the atapi command.
  @param  Read                  The transfer direction.
  @param  AtaCommandBlock       The EFI_ATA_COMMAND_BLOCK data.
  @param  AtaStatusBlock        The EFI_ATA_STATUS_BLOCK data.
  @param  MemoryAddr            The pointer to the data buffer.
  @param  DataCount             The data count to be transferred.
  @param  Timeout               The timeout value of non data transfer.

  @retval EFI_DEVICE_ERROR  The DMA data transfer abort with error occurs.
  @retval EFI_TIMEOUT     The operation is time out.
  @retval EFI_UNSUPPORTED   The device is not ready for transfer.
  @retval EFI_SUCCESS     The DMA data transfer executes successfully.
   
**/
EFI_STATUS
EFIAPI
AhciDmaTransfer (
  IN     EFI_PCI_IO_PROTOCOL        *PciIo,
  IN     EFI_AHCI_REGISTERS         *AhciRegisters,
  IN     UINT8                      Port,
  IN     UINT8                      PortMultiplier,
  IN     EFI_AHCI_ATAPI_COMMAND     *AtapiCommand OPTIONAL,
  IN     UINT8                      AtapiCommandLength,
  IN     BOOLEAN                    Read,  
  IN     EFI_ATA_COMMAND_BLOCK      *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK       *AtaStatusBlock,
  IN OUT VOID                       *MemoryAddr,
  IN     UINTN                      DataCount,
  IN     UINT64                     Timeout
  )
{
  EFI_STATUS                    Status;
  UINT32                        Offset;
  EFI_PHYSICAL_ADDRESS          PhyAddr;
  VOID                          *Map;
  UINTN                         MapLength;
  EFI_PCI_IO_PROTOCOL_OPERATION Flag;
  EFI_AHCI_COMMAND_FIS          CFis;
  EFI_AHCI_COMMAND_LIST         CmdList;

  if (Read) {
    Flag = EfiPciIoOperationBusMasterWrite;
  } else {
    Flag = EfiPciIoOperationBusMasterRead;
  }

  //
  // construct command list and command table with pci bus address
  //
  MapLength = DataCount;
  Status = PciIo->Map (
                    PciIo,
                    Flag,
                    MemoryAddr,
                    &MapLength,
                    &PhyAddr,
                    &Map
                    );

  if (EFI_ERROR (Status) || (DataCount != MapLength)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Package read needed
  //
  AhciBuildCommandFis (&CFis, AtaCommandBlock);

  ZeroMem (&CmdList, sizeof (EFI_AHCI_COMMAND_LIST));

  CmdList.AhciCmdCfl = EFI_AHCI_FIS_REGISTER_H2D_LENGTH / 4;
  CmdList.AhciCmdW   = Read ? 0 : 1;

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
  
  Status = AhciStartCommand (
             PciIo, 
             Port, 
             0,
             Timeout
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }
  
  //
  // Wait device PRD processed
  //
  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_IS;
  Status = AhciWaitMemSet (
             PciIo,
             Offset,
             EFI_AHCI_PORT_IS_DPS,
             EFI_AHCI_PORT_IS_DPS,
             Timeout
             ); 
  
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Wait for command compelte
  //
  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CI;
  Status = AhciWaitMemSet (
             PciIo,
             Offset,
             0xFFFFFFFF,
             0,
             Timeout
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_IS;
  Status = AhciWaitMemSet (
             PciIo,
             Offset,
             EFI_AHCI_PORT_IS_DHRS,
             EFI_AHCI_PORT_IS_DHRS,
             Timeout
             );  
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

Exit: 
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
  
  return Status;
}

/**
  Start a non data transfer on specific port.
    
  @param  PciIo             The PCI IO protocol instance.
  @param  AhciRegisters         The pointer to the EFI_AHCI_REGISTERS.
  @param  Port              The number of port.
  @param  PortMultiplier      The timeout value of stop.
  @param  AtapiCommand      The atapi command will be used for the transfer.
  @param  AtapiCommandLength    The length of the atapi command.
  @param    AtaCommandBlock       The EFI_ATA_COMMAND_BLOCK data.
  @param    AtaStatusBlock        The EFI_ATA_STATUS_BLOCK data.
  @param  Timeout               The timeout value of non data transfer.

  @retval EFI_DEVICE_ERROR    The non data transfer abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for transfer.
  @retval EFI_SUCCESS         The non data transfer executes successfully.

**/ 
EFI_STATUS
EFIAPI
AhciNonDataTransfer (
  IN     EFI_PCI_IO_PROTOCOL           *PciIo,
  IN     EFI_AHCI_REGISTERS            *AhciRegisters,
  IN     UINT8                         Port,
  IN     UINT8                         PortMultiplier,
  IN     EFI_AHCI_ATAPI_COMMAND        *AtapiCommand OPTIONAL,
  IN     UINT8                         AtapiCommandLength,
  IN     EFI_ATA_COMMAND_BLOCK         *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock,
  IN     UINT64                        Timeout
  ) 
{
  EFI_STATUS                   Status;  
  UINTN                        FisBaseAddr;
  UINT32                       Offset;
  UINT32                       Value;
  UINT32                       Delay;
  
  EFI_AHCI_COMMAND_FIS         CFis;
  EFI_AHCI_COMMAND_LIST        CmdList;

  //
  // Package read needed
  //
  AhciBuildCommandFis (&CFis, AtaCommandBlock);

  ZeroMem (&CmdList, sizeof (EFI_AHCI_COMMAND_LIST));

  CmdList.AhciCmdCfl = EFI_AHCI_FIS_REGISTER_H2D_LENGTH / 4;

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
  
  Status = AhciStartCommand (
             PciIo, 
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
  FisBaseAddr = (UINTN)AhciRegisters->AhciRFis + Port * sizeof (EFI_AHCI_RECEIVED_FIS);
  //
  // Wait device sends the PIO setup fis before data transfer
  //
  Delay = (UINT32) (DivU64x32(Timeout, 1000) + 1);
  do {
    Value = *(UINT32 *) (FisBaseAddr + EFI_AHCI_D2H_FIS_OFFSET);

    if ((Value & EFI_AHCI_FIS_TYPE_MASK) == EFI_AHCI_FIS_REGISTER_D2H) {
      break;
    }

    //
    // Stall for 100 microseconds.
    //
    MicroSecondDelay(100);

    Delay --;    
  } while (Delay > 0);

  if (Delay == 0) {
    Status = EFI_TIMEOUT;
    goto Exit;
  }

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CI;

  Status = AhciWaitMemSet (
             PciIo,
             Offset,
             0xFFFFFFFF,
             0,
             Timeout
             );  
  
Exit:  
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

  return Status;
}

/**
  Stop command running for giving port
    
  @param  PciIo              The PCI IO protocol instance.
  @param  Port               The number of port.
  @param  Timeout            The timeout value of stop.
   
  @retval EFI_DEVICE_ERROR   The command stop unsuccessfully.
  @retval EFI_TIMEOUT        The operation is time out.
  @retval EFI_SUCCESS        The command stop successfully.

**/
EFI_STATUS
EFIAPI
AhciStopCommand (
  IN  EFI_PCI_IO_PROTOCOL       *PciIo,
  IN  UINT8                     Port,
  IN  UINT64                    Timeout
  )
{
  UINT32 Offset;
  UINT32 Data;

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
  Data   = AhciReadReg (PciIo, Offset);

  if ((Data & (EFI_AHCI_PORT_CMD_ST |  EFI_AHCI_PORT_CMD_CR)) == 0) {
    return EFI_SUCCESS;    
  }

  if ((Data & EFI_AHCI_PORT_CMD_ST) != 0) {
    AhciAndReg (PciIo, Offset, (UINT32)~(EFI_AHCI_PORT_CMD_ST));
  }

  return AhciWaitMemSet (
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
  @param  CommandSlot        The number of CommandSlot.
  @param  Timeout            The timeout value of start.
   
  @retval EFI_DEVICE_ERROR   The command start unsuccessfully.
  @retval EFI_TIMEOUT        The operation is time out.
  @retval EFI_SUCCESS        The command start successfully.

**/
EFI_STATUS
EFIAPI
AhciStartCommand (
  IN  EFI_PCI_IO_PROTOCOL       *PciIo,
  IN  UINT8                     Port,
  IN  UINT8                     CommandSlot,
  IN  UINT64                    Timeout
  )
{
  UINT32     CmdSlotBit;
  EFI_STATUS Status;
  UINT32     PortStatus;
  UINT32     StartCmd;
  UINT32     PortTfd;
  UINT32     Offset;
  UINT32     Capability;

  //
  // Collect AHCI controller information
  //
  Capability = AhciReadReg(PciIo, EFI_AHCI_CAPABILITY_OFFSET);

  CmdSlotBit = (UINT32) (1 << CommandSlot);

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

  //
  // Setting the command
  //
  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SACT;
  AhciAndReg (PciIo, Offset, 0);
  AhciOrReg (PciIo, Offset, CmdSlotBit);

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CI;
  AhciAndReg (PciIo, Offset, 0);
  AhciOrReg (PciIo, Offset, CmdSlotBit);

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
  PortStatus = AhciReadReg (PciIo, Offset);
  
  StartCmd = 0;
  if ((PortStatus & EFI_AHCI_PORT_CMD_ALPE) != 0) {
    StartCmd = AhciReadReg (PciIo, Offset);
    StartCmd &= ~EFI_AHCI_PORT_CMD_ICC_MASK;
    StartCmd |= EFI_AHCI_PORT_CMD_ACTIVE;
  }

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_TFD;
  PortTfd = AhciReadReg (PciIo, Offset);

  if ((PortTfd & (EFI_AHCI_PORT_TFD_BSY | EFI_AHCI_PORT_TFD_DRQ)) != 0) {
    if ((Capability & BIT24) != 0) {
      Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
      AhciOrReg (PciIo, Offset, EFI_AHCI_PORT_CMD_COL);

      AhciWaitMemSet (
        PciIo, 
        Offset,
        EFI_AHCI_PORT_CMD_COL,
        0,
        Timeout
        ); 
    }
  }

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
  AhciOrReg (PciIo, Offset, EFI_AHCI_PORT_CMD_ST | StartCmd);

  return EFI_SUCCESS;
}

/**
  Do AHCI port reset.

  @param  PciIo              The PCI IO protocol instance.
  @param  Port               The number of port.
  @param  Timeout            The timeout value of reset.
   
  @retval EFI_DEVICE_ERROR   The port reset unsuccessfully
  @retval EFI_TIMEOUT        The reset operation is time out.
  @retval EFI_SUCCESS        The port reset successfully.

**/
EFI_STATUS
EFIAPI
AhciPortReset (
  IN  EFI_PCI_IO_PROTOCOL       *PciIo,
  IN  UINT8                     Port,
  IN  UINT64                    Timeout
  )
{
  EFI_STATUS      Status;
  UINT32          Offset;  
  
  AhciClearPortStatus (PciIo, Port);

  AhciStopCommand (PciIo, Port, Timeout);

  AhciDisableFisReceive (PciIo, Port, Timeout);

  AhciEnableFisReceive (PciIo, Port, Timeout);

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SCTL;

  AhciOrReg (PciIo, Offset, EFI_AHCI_PORT_SCTL_DET_INIT);

  //
  // wait 5 milliseceond before de-assert DET  
  //
  MicroSecondDelay (5000);

  AhciAndReg (PciIo, Offset, (UINT32)EFI_AHCI_PORT_SCTL_MASK);

  //
  // wait 5 milliseceond before de-assert DET  
  //
  MicroSecondDelay (5000);

  //
  // Wait for communication to be re-established
  //
  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
  Status = AhciWaitMemSet (
             PciIo,
             Offset,
             EFI_AHCI_PORT_SSTS_DET_MASK,
             EFI_AHCI_PORT_SSTS_DET_PCE,
             Timeout
             ); 

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SERR;
  AhciOrReg (PciIo, Offset, EFI_AHCI_PORT_ERR_CLEAR);

  return EFI_SUCCESS;
}

/**
  Do AHCI HBA reset.
    
  @param  PciIo              The PCI IO protocol instance.
  @param  Timeout            The timeout value of reset.
 
   
  @retval EFI_DEVICE_ERROR   AHCI controller is failed to complete hardware reset.
  @retval EFI_TIMEOUT        The reset operation is time out.
  @retval EFI_SUCCESS        AHCI controller is reset successfully.

**/
EFI_STATUS
EFIAPI
AhciReset (
  IN  EFI_PCI_IO_PROTOCOL       *PciIo,
  IN  UINT64                    Timeout
  )    
{
  EFI_STATUS             Status;
  UINT32                 Delay;
  UINT32                 Value;

  AhciOrReg (PciIo, EFI_AHCI_GHC_OFFSET, EFI_AHCI_GHC_ENABLE);

  AhciOrReg (PciIo, EFI_AHCI_GHC_OFFSET, EFI_AHCI_GHC_RESET);

  Status  = EFI_TIMEOUT;

  Delay = (UINT32) (DivU64x32(Timeout, 1000) + 1);

  do {
    Value = AhciReadReg(PciIo, EFI_AHCI_GHC_OFFSET);

    if ((Value & EFI_AHCI_GHC_RESET) == 0) {
      break;
    }

    //
    // Stall for 100 microseconds.
    //
    MicroSecondDelay(100);

    Delay--;
  } while (Delay > 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Send Buffer cmd to specific device.
    
  @param  PciIo             The PCI IO protocol instance.
  @param  AhciRegisters         The pointer to the EFI_AHCI_REGISTERS.
  @param  Port              The number of port.
  @param  PortMultiplier      The timeout value of stop.
  @param    Buffer                The data buffer to store IDENTIFY PACKET data.

  @retval EFI_DEVICE_ERROR    The cmd abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for executing.
  @retval EFI_SUCCESS         The cmd executes successfully.

**/
EFI_STATUS
EFIAPI
AhciIdentify (
  IN EFI_PCI_IO_PROTOCOL      *PciIo,
  IN EFI_AHCI_REGISTERS       *AhciRegisters,
  IN UINT8                    Port,
  IN UINT8                    PortMultiplier,
  IN OUT EFI_IDENTIFY_DATA    *Buffer  
  )
{
  EFI_STATUS                   Status;
  EFI_ATA_COMMAND_BLOCK        AtaCommandBlock;
  EFI_ATA_STATUS_BLOCK         AtaStatusBlock;

  if (PciIo == NULL || AhciRegisters == NULL || Buffer == NULL) {
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
             ATA_ATAPI_TIMEOUT
             );

  return Status;
}

/**
  Send Buffer cmd to specific device.
    
  @param  PciIo             The PCI IO protocol instance.
  @param  AhciRegisters         The pointer to the EFI_AHCI_REGISTERS.
  @param  Port              The number of port.
  @param  PortMultiplier      The timeout value of stop.
  @param    Buffer                The data buffer to store IDENTIFY PACKET data.

  @retval EFI_DEVICE_ERROR    The cmd abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for executing.
  @retval EFI_SUCCESS         The cmd executes successfully.

**/
EFI_STATUS
EFIAPI
AhciIdentifyPacket (
  IN EFI_PCI_IO_PROTOCOL      *PciIo,
  IN EFI_AHCI_REGISTERS       *AhciRegisters,
  IN UINT8                    Port,
  IN UINT8                    PortMultiplier,
  IN OUT EFI_IDENTIFY_DATA    *Buffer  
  )
{
  EFI_STATUS                   Status;
  EFI_ATA_COMMAND_BLOCK        AtaCommandBlock;
  EFI_ATA_STATUS_BLOCK         AtaStatusBlock;

  if (PciIo == NULL || AhciRegisters == NULL) {
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
             ATA_ATAPI_TIMEOUT
             );

  return Status;
}

/**
  Send SET FEATURE cmd on specific device.
    
  @param  PciIo             The PCI IO protocol instance.
  @param  AhciRegisters         The pointer to the EFI_AHCI_REGISTERS.
  @param  Port              The number of port.
  @param  PortMultiplier      The timeout value of stop.
  @param    Feature               The data to send Feature register.
  @param  FeatureSpecificData   The specific data for SET FEATURE cmd.

  @retval EFI_DEVICE_ERROR    The cmd abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for executing.
  @retval EFI_SUCCESS         The cmd executes successfully.

**/
EFI_STATUS
EFIAPI
AhciDeviceSetFeature (
  IN EFI_PCI_IO_PROTOCOL    *PciIo,
  IN EFI_AHCI_REGISTERS     *AhciRegisters,
  IN UINT8                  Port,
  IN UINT8                  PortMultiplier,
  IN UINT16                 Feature,
  IN UINT32                 FeatureSpecificData
  )
{
  EFI_STATUS               Status;
  EFI_ATA_COMMAND_BLOCK    AtaCommandBlock;
  EFI_ATA_STATUS_BLOCK     AtaStatusBlock;

  ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
  ZeroMem (&AtaStatusBlock, sizeof (EFI_ATA_STATUS_BLOCK));
  
  AtaCommandBlock.AtaCommand      = ATA_CMD_SET_FEATURES;
  AtaCommandBlock.AtaFeatures     = (UINT8) Feature;
  AtaCommandBlock.AtaFeaturesExp  = (UINT8) (Feature >> 8);
  AtaCommandBlock.AtaSectorCount  = (UINT8) FeatureSpecificData;
  AtaCommandBlock.AtaSectorNumber = (UINT8) (FeatureSpecificData >> 8);
  AtaCommandBlock.AtaCylinderLow  = (UINT8) (FeatureSpecificData >> 16);
  AtaCommandBlock.AtaCylinderHigh = (UINT8) (FeatureSpecificData >> 24);

  Status = AhciNonDataTransfer (
             PciIo,
             AhciRegisters,
             (UINT8)Port,
             (UINT8)PortMultiplier,
             NULL,
             0,
             &AtaCommandBlock,
             &AtaStatusBlock,
             ATA_ATAPI_TIMEOUT
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
  IN  EFI_PCI_IO_PROTOCOL                           *PciIo,
  IN  EFI_AHCI_REGISTERS                            *AhciRegisters,
  IN  UINT8                                         Port,
  IN  UINT8                                         PortMultiplier,
  IN  EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET    *Packet
  )
{
  EFI_STATUS                   Status;
  VOID                         *Buffer;
  UINT32                       Length;
  EFI_ATA_COMMAND_BLOCK        AtaCommandBlock;
  EFI_ATA_STATUS_BLOCK         AtaStatusBlock;
  BOOLEAN                      Read;
  UINT8                        Retry;

  if (Packet == NULL || Packet->Cdb == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&AtaCommandBlock, sizeof (EFI_ATA_COMMAND_BLOCK));
  ZeroMem (&AtaStatusBlock, sizeof (EFI_ATA_STATUS_BLOCK));
  AtaCommandBlock.AtaCommand      = ATA_CMD_PACKET;
  //
  // No OVL; No DMA
  //
  AtaCommandBlock.AtaFeatures     = 0x00;
  //
  // set the transfersize to ATAPI_MAX_BYTE_COUNT to let the device
  // determine how many data should be transferred.
  //
  AtaCommandBlock.AtaCylinderLow  = (UINT8) (ATAPI_MAX_BYTE_COUNT & 0x00ff);
  AtaCommandBlock.AtaCylinderHigh = (UINT8) (ATAPI_MAX_BYTE_COUNT >> 8);

  if (Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_READ) {
    Buffer = Packet->InDataBuffer;
    Length = Packet->InTransferLength;
    Read = TRUE;
  } else {
    Buffer = Packet->OutDataBuffer;
    Length = Packet->OutTransferLength;
    Read = FALSE;
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
               Packet->Timeout
               );
  } else {
    //
    // READ_CAPACITY cmd may execute failure. Retry 5 times
    //
    if (((UINT8 *)Packet->Cdb)[0] == ATA_CMD_READ_CAPACITY) {
      Retry = 5;
    } else {
      Retry = 1;
    }
    do {
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
                 Packet->Timeout
                 );
      if (!EFI_ERROR (Status)) {
        break;
      }
      Retry--;
    } while (Retry != 0);
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
  IN     EFI_PCI_IO_PROTOCOL    *PciIo,
  IN OUT EFI_AHCI_REGISTERS     *AhciRegisters
  )
{
  EFI_STATUS            Status;
  UINTN                 Bytes;
  VOID                  *Buffer;

  UINT32                Capability;
  UINT8                 MaxPortNumber;
  UINT8                 MaxCommandSlotNumber;
  BOOLEAN               Support64Bit;
  UINT64                MaxReceiveFisSize;
  UINT64                MaxCommandListSize;
  UINT64                MaxCommandTableSize;

  Buffer = NULL;
  //
  // Collect AHCI controller information
  //
  Capability           = AhciReadReg(PciIo, EFI_AHCI_CAPABILITY_OFFSET);
  MaxPortNumber        = (UINT8) ((Capability & 0x1F) + 1);
  //
  // Get the number of command slots per port supported by this HBA.
  //
  MaxCommandSlotNumber = (UINT8) (((Capability & 0x1F00) >> 8) + 1);
  Support64Bit         = ((Capability & BIT31) != 0) ? TRUE : FALSE;

  MaxReceiveFisSize    = MaxPortNumber * sizeof (EFI_AHCI_RECEIVED_FIS);
  Status = PciIo->AllocateBuffer (
                    PciIo,
                    AllocateAnyPages,
                    EfiBootServicesData,
                    EFI_SIZE_TO_PAGES (MaxReceiveFisSize),
                    &Buffer,
                    0
                    );

  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (Buffer, MaxReceiveFisSize);

  AhciRegisters->AhciRFis          = Buffer;
  AhciRegisters->MaxReceiveFisSize = MaxReceiveFisSize;
  Bytes  = MaxReceiveFisSize;

  Status = PciIo->Map (
                    PciIo,
                    EfiPciIoOperationBusMasterCommonBuffer,
                    Buffer,
                    &Bytes,
                    &(EFI_PHYSICAL_ADDRESS)AhciRegisters->AhciRFisPciAddr,
                    &AhciRegisters->MapRFis
                    );

  if (EFI_ERROR (Status) || (Bytes != MaxReceiveFisSize)) {
    //
    // Map error or unable to map the whole RFis buffer into a contiguous region. 
    //
    Status = EFI_OUT_OF_RESOURCES;
    goto Error6;
  }

  if ((!Support64Bit) && ((EFI_PHYSICAL_ADDRESS)AhciRegisters->AhciRFisPciAddr > 0x100000000UL)) {
    //
    // The AHCI HBA doesn't support 64bit addressing, so should not get a >4G pci bus master address.
    //
    Status = EFI_DEVICE_ERROR;
    goto Error5;
  }

  //
  // Allocate memory for command list
  // Note that the implemenation is a single task model which only use a command list for all ports.
  //
  Buffer = NULL;
  MaxCommandListSize = MaxCommandSlotNumber * sizeof (EFI_AHCI_COMMAND_LIST);
  Status = PciIo->AllocateBuffer (
                    PciIo,
                    AllocateAnyPages,
                    EfiBootServicesData,
                    EFI_SIZE_TO_PAGES (MaxCommandListSize),
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

  ZeroMem (Buffer, MaxCommandListSize);

  AhciRegisters->AhciCmdList        = Buffer;
  AhciRegisters->MaxCommandListSize = MaxCommandListSize;
  Bytes  = MaxCommandListSize;

  Status = PciIo->Map (
                    PciIo,
                    EfiPciIoOperationBusMasterCommonBuffer,
                    Buffer,
                    &Bytes,
                    &(EFI_PHYSICAL_ADDRESS)AhciRegisters->AhciCmdListPciAddr,
                    &AhciRegisters->MapCmdList
                    );

  if (EFI_ERROR (Status) || (Bytes != MaxCommandListSize)) {
    //
    // Map error or unable to map the whole cmd list buffer into a contiguous region.
    //
    Status = EFI_OUT_OF_RESOURCES;
    goto Error4;
  }

  if ((!Support64Bit) && ((EFI_PHYSICAL_ADDRESS)AhciRegisters->AhciCmdListPciAddr > 0x100000000UL)) {
    //
    // The AHCI HBA doesn't support 64bit addressing, so should not get a >4G pci bus master address.
    //
    Status = EFI_DEVICE_ERROR;
    goto Error3;
  }

  //
  // Allocate memory for command table
  // According to AHCI 1.3 spec, a PRD table can contain maximum 65535 entries.
  //
  Buffer = NULL;
  MaxCommandTableSize = sizeof (EFI_AHCI_COMMAND_TABLE);

  Status = PciIo->AllocateBuffer (
                    PciIo,
                    AllocateAnyPages,
                    EfiBootServicesData,
                    EFI_SIZE_TO_PAGES (MaxCommandTableSize),
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

  ZeroMem (Buffer, MaxCommandTableSize);

  AhciRegisters->AhciCommandTable    = Buffer;
  AhciRegisters->MaxCommandTableSize = MaxCommandTableSize;
  Bytes  = MaxCommandTableSize;

  Status = PciIo->Map (
                    PciIo,
                    EfiPciIoOperationBusMasterCommonBuffer,
                    Buffer,
                    &Bytes,
                    &(EFI_PHYSICAL_ADDRESS)AhciRegisters->AhciCommandTablePciAddr,
                    &AhciRegisters->MapCommandTable
                    );

  if (EFI_ERROR (Status) || (Bytes != MaxCommandTableSize)) {
    //
    // Map error or unable to map the whole cmd list buffer into a contiguous region.
    //
    Status = EFI_OUT_OF_RESOURCES;
    goto Error2;
  }

  if ((!Support64Bit) && ((EFI_PHYSICAL_ADDRESS)AhciRegisters->AhciCommandTablePciAddr > 0x100000000UL)) {
    //
    // The AHCI HBA doesn't support 64bit addressing, so should not get a >4G pci bus master address.
    //
    Status = EFI_DEVICE_ERROR;
    goto Error1;
  }

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
           EFI_SIZE_TO_PAGES (MaxCommandTableSize),
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
           EFI_SIZE_TO_PAGES (MaxCommandListSize),
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
           EFI_SIZE_TO_PAGES (MaxReceiveFisSize),
           AhciRegisters->AhciRFis
           );

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
  IN  ATA_ATAPI_PASS_THRU_INSTANCE    *Instance
  )
{
  EFI_STATUS                       Status;
  EFI_PCI_IO_PROTOCOL              *PciIo;
  EFI_IDE_CONTROLLER_INIT_PROTOCOL *IdeInit;
  UINT32                           Capability;
  UINT8                            MaxPortNumber;
  UINT32                           PortImplementBitMap;
  UINT8                            MaxCommandSlotNumber;
  BOOLEAN                          Support64Bit;

  EFI_AHCI_REGISTERS               *AhciRegisters;

  UINT8                            Port;
  DATA_64                          Data64;
  UINT32                           Offset;
  UINT32                           Data;
  EFI_IDENTIFY_DATA                Buffer;
  EFI_ATA_DEVICE_TYPE              DeviceType;
  EFI_ATA_COLLECTIVE_MODE          *SupportedModes;
  EFI_ATA_TRANSFER_MODE            TransferMode;
  
  if (Instance == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PciIo   = Instance->PciIo;
  IdeInit = Instance->IdeControllerInit;

  Status = AhciReset (PciIo, ATA_ATAPI_TIMEOUT); 

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Enable AE before accessing any AHCI registers
  //
  AhciOrReg (PciIo, EFI_AHCI_GHC_OFFSET, EFI_AHCI_GHC_ENABLE);

  //
  // Collect AHCI controller information
  //
  Capability           = AhciReadReg(PciIo, EFI_AHCI_CAPABILITY_OFFSET);

  //
  // Get the number of command slots per port supported by this HBA.
  //
  MaxCommandSlotNumber = (UINT8) (((Capability & 0x1F00) >> 8) + 1);
  Support64Bit         = ((Capability & BIT31) != 0) ? TRUE : FALSE;

  //
  // Get the bit map of those ports exposed by this HBA.
  // It indicates which ports that the HBA supports are available for software to use. 
  //
  PortImplementBitMap  = AhciReadReg(PciIo, EFI_AHCI_PI_OFFSET);
  MaxPortNumber        = (UINT8) ((Capability & 0x1F) + 1);
  
  AhciRegisters = &Instance->AhciRegisters;
  Status = AhciCreateTransferDescriptor (PciIo, AhciRegisters);

  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Port = 0; Port < MaxPortNumber; Port ++) {  
    Data64.Uint64 = (UINTN) (AhciRegisters->AhciRFisPciAddr) + sizeof (EFI_AHCI_RECEIVED_FIS) * Port;
  
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_FB;
    AhciWriteReg (PciIo, Offset, Data64.Uint32.Lower32);
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_FBU;
    AhciWriteReg (PciIo, Offset, Data64.Uint32.Upper32);
  
    //
    // Single task envrionment, we only use one command table for all port
    //
    Data64.Uint64 = (UINTN) (AhciRegisters->AhciCmdListPciAddr);
  
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CLB;
    AhciWriteReg (PciIo, Offset, Data64.Uint32.Lower32);
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CLBU;
    AhciWriteReg (PciIo, Offset, Data64.Uint32.Upper32);
  
    if ((PortImplementBitMap & (BIT0 << Port)) != 0) {
      Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_CMD;
  
      if ((Capability & EFI_AHCI_PORT_CMD_ASP) != 0) {
        AhciOrReg (PciIo, Offset, EFI_AHCI_PORT_CMD_SUD);
      }
      Data = AhciReadReg (PciIo, Offset);
      if ((Data & EFI_AHCI_PORT_CMD_CPD) != 0) {
        AhciOrReg (PciIo, Offset, EFI_AHCI_PORT_CMD_POD);
      }
  
      AhciAndReg (PciIo, Offset, (UINT32)~(EFI_AHCI_PORT_CMD_FRE|EFI_AHCI_PORT_CMD_COL|EFI_AHCI_PORT_CMD_ST));
    }
  
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SCTL;
    AhciAndReg (PciIo, Offset, (UINT32)~(EFI_AHCI_PORT_SCTL_IPM_MASK));
 
    AhciAndReg (PciIo, Offset,(UINT32) ~(EFI_AHCI_PORT_SCTL_IPM_PSD));
    AhciOrReg (PciIo, Offset, EFI_AHCI_PORT_SCTL_IPM_PSD);
  
    AhciAndReg (PciIo, Offset, (UINT32)~(EFI_AHCI_PORT_SCTL_IPM_SSD));
    AhciOrReg (PciIo, Offset, EFI_AHCI_PORT_SCTL_IPM_SSD);
  
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_IE;
    AhciAndReg (PciIo, Offset, 0);
  
    Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SERR;
    AhciWriteReg (PciIo, Offset, AhciReadReg (PciIo, Offset));
  }

  //
  // Stall for 100 milliseconds.
  //
  MicroSecondDelay(100000);
  
  IdeInit->NotifyPhase (IdeInit, EfiIdeBeforeChannelEnumeration, Port);
  
  for (Port = 0; Port < MaxPortNumber; Port ++) {  
    if ((PortImplementBitMap & (BIT0 << Port)) != 0) {
    
      Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SSTS;
      Data = AhciReadReg (PciIo, Offset) & EFI_AHCI_PORT_SSTS_DET_MASK;

      if (Data == 0) {
        continue;
      }
      //
      // Found device in the port
      //
      if (Data == EFI_AHCI_PORT_SSTS_DET_PCE) {
        Offset = EFI_AHCI_PORT_START + Port * EFI_AHCI_PORT_REG_WIDTH + EFI_AHCI_PORT_SIG;

        Status = AhciWaitMemSet (
                   PciIo, 
                   Offset,
                   0x0000FFFF,
                   0x00000101,
                   ATA_ATAPI_TIMEOUT
                   );
        if (EFI_ERROR (Status)) {
          continue;
        }

        //
        // Now inform the IDE Controller Init Module.
        //
        IdeInit->NotifyPhase (IdeInit, EfiIdeBusBeforeDevicePresenceDetection, Port);

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
            continue;
          }

          DeviceType = EfiIdeHarddisk;
        } else {
          continue;
        }
    
        DEBUG ((EFI_D_INFO, "port [%d] port mulitplier [%d] has a [%a]\n", 
            Port, 0, DeviceType == EfiIdeCdrom ? "cdrom" : "harddisk"));

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
          DEBUG ((EFI_D_ERROR, "Calculate Mode Fail, Status = %r\n", Status));
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

        TransferMode.ModeNumber = (UINT8) (SupportedModes->PioMode.Mode);
    
        //
        // Set supported DMA mode on this IDE device. Note that UDMA & MDMA cann't
        // be set together. Only one DMA mode can be set to a device. If setting
        // DMA mode operation fails, we can continue moving on because we only use
        // PIO mode at boot time. DMA modes are used by certain kind of OS booting
        //
        if (SupportedModes->UdmaMode.Valid) {
          TransferMode.ModeCategory = EFI_ATA_MODE_UDMA;
          TransferMode.ModeNumber = (UINT8) (SupportedModes->UdmaMode.Mode);
        } else if (SupportedModes->MultiWordDmaMode.Valid) {
          TransferMode.ModeCategory = EFI_ATA_MODE_MDMA;
          TransferMode.ModeNumber = (UINT8) SupportedModes->MultiWordDmaMode.Mode;  
        }

        Status = AhciDeviceSetFeature (PciIo, AhciRegisters, Port, 0, 0x03, (UINT32)(*(UINT8 *)&TransferMode));
    
        if (EFI_ERROR (Status)) {
          DEBUG ((EFI_D_ERROR, "Set transfer Mode Fail, Status = %r\n", Status));
          continue;
        }
        //
        // Found a ATA or ATAPI device, add it into the device list.
        //
        CreateNewDeviceInfo (Instance, Port, 0, DeviceType, &Buffer);
      }
    }
  }
  return EFI_SUCCESS;
}



