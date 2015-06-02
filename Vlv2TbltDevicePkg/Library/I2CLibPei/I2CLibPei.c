/** @file
  I2C PEI Lib Instance.

  Copyright (c) 1999- 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "I2CDelayPei.h"
#include "I2CIoLibPei.h"
#include "I2CAccess.h"
#include "I2CLibPei.h"
#include <PlatformBaseAddresses.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/HobLib.h>
#include <PchRegs/PchRegsPcu.h> 
#include <PchRegs/PchRegsLpss.h> 

#define LPSS_PCI_DEVICE_NUMBER  8

#define R_PCH_LPIO_I2C_MEM_RESETS                 0x804 // Software Reset
#define B_PCH_LPIO_I2C_MEM_RESETS_FUNC            BIT1  // Function Clock Domain Reset
#define B_PCH_LPIO_I2C_MEM_RESETS_APB             BIT0  // APB Domain Reset
#define R_PCH_LPSS_I2C_MEM_PCP                    0x800 // Private Clock Parameters

#define PEI_TEPM_LPSS_DMA_BAR                     0xFE900000
#define PEI_TEPM_LPSS_I2C0_BAR                    0xFE910000
#define PCI_CONFIG_SPACE_SIZE                     0x10000

EFI_GUID  mI2CPeiInitGuid = {
  0x96DED71A, 0xB9E7, 0x4EAD, 0x96, 0x2C, 0x01, 0x69, 0x3C, 0xED, 0x2A, 0x64
};


UINT16 I2CGPIO[]= {
  //
  // 19.1.6  I2C0
  // I2C0_SDA-OD-O -    write 0x2003CC81 to IOBASE + 0x0210
  // I2C0_SCL-OD-O -    write 0x2003CC81 to IOBASE + 0x0200
  //
  0x0210,
  0x0200,

  //
  // 19.1.7  I2C1
  // I2C1_SDA-OD-O/I - write 0x2003CC81 to IOBASE + 0x01F0
  // I2C1_SCL-OD-O/I - write 0x2003CC81 to IOBASE + 0x01E0
  //
  0x01F0,
  0x01E0,

  //
  // 19.1.8  I2C2
  // I2C2_SDA-OD-O/I - write 0x2003CC81 to IOBASE + 0x01D0
  // I2C2_SCL-OD-O/I - write 0x2003CC81 to IOBASE + 0x01B0
  //
  0x01D0,
  0x01B0,

  //
  // 19.1.9  I2C3
  // I2C3_SDA-OD-O/I - write 0x2003CC81 to IOBASE + 0x0190
  // I2C3_SCL-OD-O/I - write 0x2003CC81 to IOBASE + 0x01C0
  // 
  0x0190,
  0x01C0,

  //
  // 19.1.10 I2C4
  // I2C4_SDA-OD-O/I - write 0x2003CC81 to IOBASE + 0x01A0
  // I2C4_SCL-OD-O/I - write 0x2003CC81 to IOBASE + 0x0170
  //
  0x01A0,
  0x0170,

  // 
  // 19.1.11 I2C5
  // I2C5_SDA-OD-O/I - write 0x2003CC81 to IOBASE + 0x0150
  // I2C5_SCL-OD-O/I - write 0x2003CC81 to IOBASE + 0x0140
  // 
  0x0150,
  0x0140,

  //
  // 19.1.12 I2C6
  // I2C6_SDA-OD-O/I - write 0x2003CC81 to IOBASE + 0x0180
  // I2C6_SCL-OD-O/I -  write 0x2003CC81 to IOBASE + 0x0160
  // 
  0x0180,
  0x0160
};

/**
  Constructor of this library.

  @param   VOID

  @return  EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
IntelI2CPeiLibConstructor (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  UINTN Index;
  
  for (Index = 0; Index < sizeof(I2CGPIO)/sizeof(UINT16); Index ++) {
    I2CLibPeiMmioWrite32(IO_BASE_ADDRESS+I2CGPIO[Index], 0x2003CC81);
  }

  return EFI_SUCCESS;
}

/**
  Programe all I2C controllers on LPSS. 
  
  I2C0 is function 1 of LPSS. I2C1 is function 2 of LPSS, etc..

  @param   VOID

  @return  EFI_SUCCESS
**/
EFI_STATUS
ProgramPciLpssI2C (
  VOID
  )
{
  UINT32       PmcBase;
  UINT32       DevID;
  UINTN        PciMmBase=0;
  UINTN        Index;
  UINTN        Bar0;
  UINTN        Bar1;
  DEBUG ((EFI_D_INFO, "Pei ProgramPciLpssI2C() Start\n"));
  
  //
  // Set the VLV Function Disable Register to ZERO
  //
  PmcBase         = I2CLibPeiMmioRead32(PciD31F0RegBase + R_PCH_LPC_PMC_BASE) & B_PCH_LPC_PMC_BASE_BAR;
  
  if(I2CLibPeiMmioRead32(PmcBase + R_PCH_PMC_FUNC_DIS)&
      (B_PCH_PMC_FUNC_DIS_LPSS2_FUNC1 | B_PCH_PMC_FUNC_DIS_LPSS2_FUNC2
       | B_PCH_PMC_FUNC_DIS_LPSS2_FUNC3 | B_PCH_PMC_FUNC_DIS_LPSS2_FUNC4 | B_PCH_PMC_FUNC_DIS_LPSS2_FUNC5
       | B_PCH_PMC_FUNC_DIS_LPSS2_FUNC6 | B_PCH_PMC_FUNC_DIS_LPSS2_FUNC7)) {
    I2CLibPeiMmioWrite32(
      PmcBase+R_PCH_PMC_FUNC_DIS,
      I2CLibPeiMmioRead32(PmcBase + R_PCH_PMC_FUNC_DIS)& \
      ~(B_PCH_PMC_FUNC_DIS_LPSS2_FUNC1 | B_PCH_PMC_FUNC_DIS_LPSS2_FUNC2 \
        | B_PCH_PMC_FUNC_DIS_LPSS2_FUNC3 | B_PCH_PMC_FUNC_DIS_LPSS2_FUNC4 \
        | B_PCH_PMC_FUNC_DIS_LPSS2_FUNC5 | B_PCH_PMC_FUNC_DIS_LPSS2_FUNC6|B_PCH_PMC_FUNC_DIS_LPSS2_FUNC7)
      );
    DEBUG ((EFI_D_INFO, "ProgramPciLpssI2C() enable all I2C controllers\n"));
  }

  for(Index = 0; Index < LPSS_PCI_DEVICE_NUMBER; Index ++) {

    PciMmBase = MmPciAddress (
                  0,
                  DEFAULT_PCI_BUS_NUMBER_PCH,
                  PCI_DEVICE_NUMBER_PCH_LPSS_I2C,
                  Index,
                  0
                  );
    DevID =  I2CLibPeiMmioRead32(PciMmBase);

    Bar0 = PEI_TEPM_LPSS_DMA_BAR + (Index * PCI_CONFIG_SPACE_SIZE);
    Bar1 = Bar0 + 0x8000;

    DEBUG((EFI_D_ERROR, "Program Pci Lpss I2C Device  Function=%x DevID=%08x\n", Index, DevID));
    
    //
    // Check if device present
    //
    if (DevID  != 0xFFFFFFFF)  {
      if(!(I2CLibPeiMmioRead32 (PciMmBase + R_PCH_LPSS_I2C_STSCMD) & B_PCH_LPSS_I2C_STSCMD_MSE)) {
        //
        // Program BAR 0
        //
        I2CLibPeiMmioWrite32((UINTN) (PciMmBase + R_PCH_LPSS_I2C_BAR), (UINT32)(Bar0 & B_PCH_LPSS_I2C_BAR_BA));
        
        DEBUG ((EFI_D_ERROR, "I2CBaseAddress1 = 0x%x \n",I2CLibPeiMmioRead32 (PciMmBase+R_PCH_LPSS_I2C_BAR)));
        
        //
        // Program BAR 1
        //
        I2CLibPeiMmioWrite32 ((UINTN)(PciMmBase + R_PCH_LPSS_I2C_BAR1), (UINT32)(Bar1 & B_PCH_LPSS_I2C_BAR1_BA));
        DEBUG ((EFI_D_ERROR, "I2CBaseAddress1 = 0x%x \n",I2CLibPeiMmioRead32(PciMmBase+R_PCH_LPSS_I2C_BAR1)));
        
        //
        // Bus Master Enable & Memory Space Enable
        //
        I2CLibPeiMmioWrite32((UINTN) (PciMmBase + R_PCH_LPSS_I2C_STSCMD), (UINT32)(B_PCH_LPSS_I2C_STSCMD_BME | B_PCH_LPSS_I2C_STSCMD_MSE));
      }
      
      //
      // Release Resets
      //
      I2CLibPeiMmioWrite32 (Bar0 + R_PCH_LPIO_I2C_MEM_RESETS, (B_PCH_LPIO_I2C_MEM_RESETS_FUNC | B_PCH_LPIO_I2C_MEM_RESETS_APB));
      
      //
      // Activate Clocks
      //
      I2CLibPeiMmioWrite32 (Bar0 + R_PCH_LPSS_I2C_MEM_PCP, 0x80020003);//No use for A0

      DEBUG ((EFI_D_INFO, "ProgramPciLpssI2C() Programmed()\n"));
    }

  }
  
  DEBUG ((EFI_D_INFO, "Pei ProgramPciLpssI2C() End\n"));

  return EFI_SUCCESS;
}

/**
  Disable I2C Bus.

  @param I2cControllerIndex   Index of I2C controller.

  @return EFI_SUCCESS
**/
EFI_STATUS
I2cDisable (
  IN UINT8 I2cControllerIndex
  )
{
  UINTN  I2CBaseAddress;
  UINT32 NumTries = 10000;  // 0.1 seconds
  
  I2CBaseAddress = (UINT32) PEI_TEPM_LPSS_I2C0_BAR + I2cControllerIndex * PCI_CONFIG_SPACE_SIZE;
  
  I2CLibPeiMmioWrite16 (I2CBaseAddress + R_IC_ENABLE, 0);
  while (0 != ( I2CLibPeiMmioRead16 (I2CBaseAddress + R_IC_ENABLE_STATUS ) & 1)) {
    MicroSecondDelay (10);
    NumTries --;
    if(0 == NumTries) return EFI_NOT_READY;
  }
  
  return EFI_SUCCESS;
}

/**
  Enable I2C Bus.

  @param I2cControllerIndex   Index of I2C controller.

  @return EFI_SUCCESS
**/
EFI_STATUS
I2cEnable (
  IN UINT8 I2cControllerIndex
  )
{
  UINTN   I2CBaseAddress;
  UINT32 NumTries = 10000;  // 0.1 seconds
  
  I2CBaseAddress = (UINT32) PEI_TEPM_LPSS_I2C0_BAR+ I2cControllerIndex * PCI_CONFIG_SPACE_SIZE;
  I2CLibPeiMmioWrite16 (I2CBaseAddress + R_IC_ENABLE, 1);
  while (0 == ( I2CLibPeiMmioRead16 ( I2CBaseAddress + R_IC_ENABLE_STATUS ) & 1)) {
    MicroSecondDelay (10);
    NumTries --;
    if(0 == NumTries) return EFI_NOT_READY;
  }
  
  return EFI_SUCCESS;
}


/**
  Set the I2C controller bus clock frequency.

  @param[in] This           Address of the library's I2C context structure
  @param[in] PlatformData   Address of the platform configuration data
  @param[in] BusClockHertz  New I2C bus clock frequency in Hertz

  @retval RETURN_SUCCESS      The bus frequency was set successfully.
  @retval RETURN_UNSUPPORTED  The controller does not support this frequency.

**/
EFI_STATUS
I2cBusFrequencySet (
  IN UINTN   I2CBaseAddress,
  IN UINTN   BusClockHertz,
  IN UINT16  *I2cMode
  )
{
  DEBUG((EFI_D_INFO,"InputFreq BusClockHertz: %d\r\n",BusClockHertz));

  *I2cMode = B_IC_RESTART_EN | B_IC_SLAVE_DISABLE | B_MASTER_MODE;

  //
  //  Set the 100 KHz clock divider
  //
  //  From Table 10 of the I2C specification
  //
  //    High: 4.00 uS
  //    Low:  4.70 uS
  //
  I2CLibPeiMmioWrite16 ( I2CBaseAddress + R_IC_SS_SCL_HCNT, (UINT16)0x214 );
  I2CLibPeiMmioWrite16 ( I2CBaseAddress + R_IC_SS_SCL_LCNT, (UINT16)0x272 );
  
  //
  //    Set the 400 KHz clock divider
  //
  //    From Table 10 of the I2C specification
  //
  //      High: 0.60 uS
  //      Low:  1.30 uS
  //
  I2CLibPeiMmioWrite16 ( I2CBaseAddress + R_IC_FS_SCL_HCNT, (UINT16)0x50 );
  I2CLibPeiMmioWrite16 ( I2CBaseAddress + R_IC_FS_SCL_LCNT, (UINT16)0xAD );

  switch ( BusClockHertz ) {
    case 100 * 1000:
      I2CLibPeiMmioWrite32 ( I2CBaseAddress + R_IC_SDA_HOLD, (UINT16)0x40);//100K
      *I2cMode |= V_SPEED_STANDARD;
      break;
    case 400 * 1000:
      I2CLibPeiMmioWrite32 ( I2CBaseAddress + R_IC_SDA_HOLD, (UINT16)0x32);//400K
      *I2cMode |= V_SPEED_FAST;
      break;
    default:
      I2CLibPeiMmioWrite32 ( I2CBaseAddress + R_IC_SDA_HOLD, (UINT16)0x09);//3.4M
      *I2cMode |= V_SPEED_HIGH;
  }

  return EFI_SUCCESS;
}

/**
  Initializes the host controller to execute I2C commands.

  @param I2cControllerIndex Index of I2C controller in LPSS device. 0 represents I2C0, which is PCI function 1 of LPSS device.   
                                
  @return EFI_SUCCESS       Opcode initialization on the I2C host controller completed.
  @return EFI_DEVICE_ERROR  Device error, operation failed.
**/
EFI_STATUS
I2CInit (
  UINT8 I2cControllerIndex, 
  UINT16 SlaveAddress
  )
{
  EFI_STATUS   Status;
  UINT32       NumTries = 0;
  UINTN        I2CBaseAddress;
  UINT16       I2cMode;
  UINTN        PciMmBase=0;


  PciMmBase = MmPciAddress (
                0,
                DEFAULT_PCI_BUS_NUMBER_PCH,
                PCI_DEVICE_NUMBER_PCH_LPSS_I2C,
                (I2cControllerIndex + 1),
                0
                );

  I2CBaseAddress = I2CLibPeiMmioRead32 (PciMmBase+R_PCH_LPSS_I2C_BAR);

  //
  //  Verify the parameters
  //
  if (1023 < SlaveAddress ) {
    Status =  EFI_INVALID_PARAMETER;
    DEBUG((EFI_D_INFO,"I2cStartRequest Exit with Status %r\r\n", Status));
    return Status;
  }

  if(I2CBaseAddress ==  (PEI_TEPM_LPSS_I2C0_BAR + I2cControllerIndex * PCI_CONFIG_SPACE_SIZE)) {
    return EFI_SUCCESS;
  }
  ProgramPciLpssI2C();

  I2CBaseAddress = (UINT32) (PEI_TEPM_LPSS_I2C0_BAR + I2cControllerIndex * PCI_CONFIG_SPACE_SIZE);
  DEBUG ((EFI_D_ERROR, "I2CBaseAddress = 0x%x \n",I2CBaseAddress));
  NumTries = 10000; // 1 seconds
  while ((1 == ( I2CLibPeiMmioRead32 ( I2CBaseAddress + R_IC_STATUS) & STAT_MST_ACTIVITY ))) {
    MicroSecondDelay(10);
    NumTries --;
    if(0 == NumTries)
      return EFI_DEVICE_ERROR;
  }

  Status = I2cDisable (I2cControllerIndex);
  DEBUG((EFI_D_INFO, "I2cDisable Status = %r\r\n", Status));

  I2cBusFrequencySet(I2CBaseAddress, 400 * 1000, &I2cMode);//Set I2cMode

  I2CLibPeiMmioWrite16(I2CBaseAddress + R_IC_INTR_MASK, 0x0);
  if (0x7F < SlaveAddress) {
    SlaveAddress = (SlaveAddress & 0x3ff ) | IC_TAR_10BITADDR_MASTER;
  }
  I2CLibPeiMmioWrite16 (I2CBaseAddress + R_IC_TAR, (UINT16) SlaveAddress );
  I2CLibPeiMmioWrite16 (I2CBaseAddress + R_IC_RX_TL, 0);
  I2CLibPeiMmioWrite16 (I2CBaseAddress + R_IC_TX_TL, 0 );
  I2CLibPeiMmioWrite16 (I2CBaseAddress + R_IC_CON, I2cMode);

  Status = I2cEnable(I2cControllerIndex);
  DEBUG((EFI_D_INFO, "I2cEnable Status = %r\r\n", Status));
  I2CLibPeiMmioRead16 ( I2CBaseAddress + R_IC_CLR_TX_ABRT );
  
  return EFI_SUCCESS;
}

/**
  Reads a Byte from I2C Device.
 
  @param  I2cControllerIndex  I2C Bus no to which the I2C device has been connected
  @param  SlaveAddress        Device Address from which the byte value has to be read
  @param  Offset              Offset from which the data has to be read
  @param  *Byte               Address to which the value read has to be stored
                                
  @return  EFI_SUCCESS        If the byte value has been successfully read
  @return  EFI_DEVICE_ERROR   Operation Failed, Device Error
**/
EFI_STATUS ByteReadI2CBasic(
  IN  UINT8 I2cControllerIndex,
  IN  UINT8 SlaveAddress,
  IN  UINTN ReadBytes,
  OUT UINT8 *ReadBuffer,
  IN  UINT8 Start,
  IN  UINT8 End
  )
{

  EFI_STATUS Status;
  UINT32 I2cStatus;
  UINT16 ReceiveData;
  UINT8 *ReceiveDataEnd;
  UINT8 *ReceiveRequest;
  UINT16 RawIntrStat;
  UINTN   I2CBaseAddress;

  I2CBaseAddress = (UINT32)(PEI_TEPM_LPSS_I2C0_BAR + I2cControllerIndex * PCI_CONFIG_SPACE_SIZE);

  Status = EFI_SUCCESS;

  I2CInit(I2cControllerIndex, SlaveAddress);

  ReceiveDataEnd = &ReadBuffer [ReadBytes];
  if(ReadBytes) {
    ReceiveRequest = ReadBuffer;
    DEBUG((EFI_D_INFO,"Read: ---------------%d bytes to RX\r\n",ReceiveDataEnd - ReceiveRequest));

    while ((ReceiveDataEnd > ReceiveRequest) || (ReceiveDataEnd > ReadBuffer)) {
      //
      // Check for NACK
      //
      RawIntrStat = I2CLibPeiMmioRead16 (I2CBaseAddress + R_IC_RawIntrStat );
      if ( 0 != (RawIntrStat & I2C_INTR_TX_ABRT )) {
        I2CLibPeiMmioRead16 ( I2CBaseAddress + R_IC_CLR_TX_ABRT );
        Status = RETURN_DEVICE_ERROR;
        DEBUG((EFI_D_INFO,"TX ABRT ,%d bytes hasn't been transferred\r\n",ReceiveDataEnd - ReceiveRequest));
        break;
      }
      
      //
      // Determine if another byte was received
      //
      I2cStatus = I2CLibPeiMmioRead16 ( I2CBaseAddress + R_IC_STATUS );
      if ( 0 != ( I2cStatus & STAT_RFNE )) {
        ReceiveData = I2CLibPeiMmioRead16 ( I2CBaseAddress + R_IC_DATA_CMD );
        *ReadBuffer++ = (UINT8)ReceiveData;
        DEBUG((EFI_D_INFO,"MmioRead32 ,1 byte 0x:%x is received\r\n",ReceiveData));
      }

      if(ReceiveDataEnd==ReceiveRequest) {
        //
        // Waiting the last request to get data and make (ReceiveDataEnd > ReadBuffer) =TRUE.
        //
        continue;
      }
      
      //
      // Wait until a read request will fit
      //
      if ( 0 == ( I2cStatus & STAT_TFNF )) {
        MicroSecondDelay ( 10 );
        continue;
      }
      
      //
      // Issue the next read request
      //
      if(End && Start ) {
        I2CLibPeiMmioWrite16 ( I2CBaseAddress + R_IC_DATA_CMD, B_READ_CMD|B_CMD_RESTART|B_CMD_STOP);
      } else if (!End && Start ) {
        I2CLibPeiMmioWrite16 ( I2CBaseAddress + R_IC_DATA_CMD, B_READ_CMD|B_CMD_RESTART);
      } else if (End && !Start ) {
        I2CLibPeiMmioWrite16 ( I2CBaseAddress + R_IC_DATA_CMD, B_READ_CMD|B_CMD_STOP);
      } else if (!End && !Start ) {
        I2CLibPeiMmioWrite16 ( I2CBaseAddress + R_IC_DATA_CMD, B_READ_CMD);
      }
      ReceiveRequest += 1;
    }

  }
  return Status;

}

/**
  Writes a Byte to I2C Device.
 
  @param  I2cControllerIndex   I2C Bus no to which the I2C device has been connected
  @param  SlaveAddress         Device Address from which the byte value has to be written
  @param  Offset               Offset from which the data has to be read
  @param  *Byte                Address to which the value written is stored
                                
  @return  EFI_SUCCESS         IF the byte value has been successfully written
  @return  EFI_DEVICE_ERROR    Operation Failed, Device Error
**/
EFI_STATUS 
ByteWriteI2CBasic(
  IN  UINT8 I2cControllerIndex,
  IN  UINT8 SlaveAddress,
  IN  UINTN WriteBytes,
  IN  UINT8 *WriteBuffer,
  IN  UINT8 Start,
  IN  UINT8 End
  )
{

  EFI_STATUS Status;
  UINT32 I2cStatus;
  UINT8 *TransmitEnd;
  UINT16 RawIntrStat;
  UINTN   I2CBaseAddress;

  I2CBaseAddress = (UINT32)PEI_TEPM_LPSS_I2C0_BAR+ I2cControllerIndex * PCI_CONFIG_SPACE_SIZE;

  Status = EFI_SUCCESS;

  I2CInit(I2cControllerIndex, SlaveAddress);

  TransmitEnd = &WriteBuffer [WriteBytes];
  if( WriteBytes ) {

    DEBUG((EFI_D_INFO,"Write: --------------%d bytes to TX\r\n", TransmitEnd - WriteBuffer));

    while ( TransmitEnd > WriteBuffer) {
      I2cStatus = I2CLibPeiMmioRead16 (I2CBaseAddress + R_IC_STATUS);
      RawIntrStat = I2CLibPeiMmioRead16 (I2CBaseAddress + R_IC_RawIntrStat);
      if ( 0 != (RawIntrStat & I2C_INTR_TX_ABRT)) {
        I2CLibPeiMmioRead16 (I2CBaseAddress + R_IC_CLR_TX_ABRT);
        Status = RETURN_DEVICE_ERROR;
        DEBUG((EFI_D_ERROR,"TX ABRT TransmitEnd:0x%x WriteBuffer:0x%x\r\n", TransmitEnd, WriteBuffer));
        break;
      }
      if (0 == ( I2cStatus & STAT_TFNF)) {
        continue;
      }
      if(End && Start) {
        I2CLibPeiMmioWrite16 (I2CBaseAddress + R_IC_DATA_CMD, (*WriteBuffer++) | B_CMD_RESTART | B_CMD_STOP);
      } else if (!End && Start ) {
        I2CLibPeiMmioWrite16 (I2CBaseAddress + R_IC_DATA_CMD, (*WriteBuffer++) | B_CMD_RESTART);
      } else if (End && !Start ) {
        I2CLibPeiMmioWrite16 (I2CBaseAddress + R_IC_DATA_CMD, (*WriteBuffer++) | B_CMD_STOP);
      } else if (!End && !Start ) {
        I2CLibPeiMmioWrite16 (I2CBaseAddress + R_IC_DATA_CMD, (*WriteBuffer++));
      }
      
      // Add a small delay to work around some odd behavior being seen.  Without this delay bytes get dropped.
      MicroSecondDelay ( FIFO_WRITE_DELAY );
    }

  }
  
  if(EFI_ERROR(Status)) {
    DEBUG((EFI_D_INFO,"I2cStartRequest Exit with Status %r\r\n",Status));
  }
    
  return Status;
}

/**
  Reads a Byte from I2C Device.
 
  @param  I2cControllerIndex   I2C Bus no to which the I2C device has been connected
  @param  SlaveAddress         Device Address from which the byte value has to be read
  @param  Offset               Offset from which the data has to be read
  @param  ReadBytes            Number of bytes to be read
  @param  *ReadBuffer          Address to which the value read has to be stored
                                
  @return  EFI_SUCCESS       IF the byte value has been successfully read
  @return  EFI_DEVICE_ERROR  Operation Failed, Device Error
**/
EFI_STATUS 
ByteReadI2C(
  IN  UINT8 I2cControllerIndex,
  IN  UINT8 SlaveAddress,
  IN  UINT8 Offset,
  IN  UINTN ReadBytes,
  OUT UINT8 *ReadBuffer
  )
{
  EFI_STATUS        Status;

  DEBUG ((EFI_D_ERROR, "ByteReadI2C:---offset:0x%x\n",Offset));
  Status = ByteWriteI2CBasic(I2cControllerIndex, SlaveAddress, 1, &Offset,TRUE,FALSE);
  Status = ByteReadI2CBasic(I2cControllerIndex, SlaveAddress, ReadBytes, ReadBuffer, TRUE, TRUE);

  return Status;
}

/**
  Writes a Byte to I2C Device.
 
  @param  I2cControllerIndex  I2C Bus no to which the I2C device has been connected
  @param  SlaveAddress        Device Address from which the byte value has to be written
  @param  Offset              Offset from which the data has to be written
  @param  WriteBytes          Number of bytes to be written
  @param  *Byte               Address to which the value written is stored
                                
  @return  EFI_SUCCESS       IF the byte value has been successfully read
  @return  EFI_DEVICE_ERROR  Operation Failed, Device Error
**/
EFI_STATUS ByteWriteI2C(
  IN  UINT8 I2cControllerIndex,
  IN  UINT8 SlaveAddress,
  IN  UINT8 Offset,
  IN  UINTN WriteBytes,
  IN  UINT8 *WriteBuffer
  )
{
  EFI_STATUS        Status;

  DEBUG ((EFI_D_ERROR, "ByteWriteI2C:---offset/bytes/buf:0x%x,0x%x,0x%x,0x%x\n",Offset,WriteBytes,WriteBuffer,*WriteBuffer));
  Status = ByteWriteI2CBasic(I2cControllerIndex, SlaveAddress, 1, &Offset, TRUE, FALSE);
  Status = ByteWriteI2CBasic(I2cControllerIndex, SlaveAddress, WriteBytes, WriteBuffer, FALSE, TRUE);

  return Status;
}
