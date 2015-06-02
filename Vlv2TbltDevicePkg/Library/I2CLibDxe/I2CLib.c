/** @file
  Functions for accessing I2C registers.
  
  Copyright (c) 2004  - 2015, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                  
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                               
--*/

#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <PchRegs/PchRegsPcu.h> 
#include <PchRegs.h>
#include <PlatformBaseAddresses.h>
#include <PchRegs/PchRegsLpss.h> 
#include <Library/I2CLib.h>
#include <Protocol/GlobalNvsArea.h>
#include <Library/UefiBootServicesTableLib.h>
#include <I2CRegs.h>

#define GLOBAL_NVS_OFFSET(Field)    (UINTN)((CHAR8*)&((EFI_GLOBAL_NVS_AREA*)0)->Field - (CHAR8*)0)

#define PCIEX_BASE_ADDRESS  0xE0000000
#define PCI_EXPRESS_BASE_ADDRESS ((VOID *) (UINTN) PCIEX_BASE_ADDRESS)
#define MmPciAddress( Segment, Bus, Device, Function, Register ) \
         ((UINTN)PCI_EXPRESS_BASE_ADDRESS + \
         (UINTN)(Bus << 20) + \
         (UINTN)(Device << 15) + \
         (UINTN)(Function << 12) + \
         (UINTN)(Register) \
        )
#define PCI_D31F0_REG_BASE             PCIEX_BASE_ADDRESS + (UINT32) (31 << 15)

typedef struct _LPSS_PCI_DEVICE_INFO {
  UINTN        Segment;
  UINTN        BusNum;
  UINTN        DeviceNum;
  UINTN        FunctionNum;
  UINTN        Bar0;
  UINTN        Bar1;
} LPSS_PCI_DEVICE_INFO;

LPSS_PCI_DEVICE_INFO  mLpssPciDeviceList[] = {
  {0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_LPSS_DMAC1, PCI_FUNCTION_NUMBER_PCH_LPSS_DMAC, 0xFE900000, 0xFE908000},
  {0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_LPSS_I2C,   PCI_FUNCTION_NUMBER_PCH_LPSS_I2C0, 0xFE910000, 0xFE918000},
  {0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_LPSS_I2C,   PCI_FUNCTION_NUMBER_PCH_LPSS_I2C1, 0xFE920000, 0xFE928000},
  {0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_LPSS_I2C,   PCI_FUNCTION_NUMBER_PCH_LPSS_I2C2, 0xFE930000, 0xFE938000},
  {0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_LPSS_I2C,   PCI_FUNCTION_NUMBER_PCH_LPSS_I2C3, 0xFE940000, 0xFE948000},
  {0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_LPSS_I2C,   PCI_FUNCTION_NUMBER_PCH_LPSS_I2C4, 0xFE950000, 0xFE958000},
  {0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_LPSS_I2C,   PCI_FUNCTION_NUMBER_PCH_LPSS_I2C5, 0xFE960000, 0xFE968000},
  {0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_LPSS_I2C,   PCI_FUNCTION_NUMBER_PCH_LPSS_I2C6, 0xFE970000, 0xFE978000}
};

#define LPSS_PCI_DEVICE_NUMBER  sizeof(mLpssPciDeviceList)/sizeof(LPSS_PCI_DEVICE_INFO)

STATIC UINTN mI2CBaseAddress = 0;
STATIC UINT16 mI2CSlaveAddress = 0;

UINT16 mI2cMode=B_IC_RESTART_EN | B_IC_SLAVE_DISABLE | B_MASTER_MODE ;

UINTN mI2cNvsBaseAddress[] = {
        GLOBAL_NVS_OFFSET(LDMA2Addr),
        GLOBAL_NVS_OFFSET(I2C1Addr),
        GLOBAL_NVS_OFFSET(I2C2Addr),
        GLOBAL_NVS_OFFSET(I2C3Addr),
        GLOBAL_NVS_OFFSET(I2C4Addr),
        GLOBAL_NVS_OFFSET(I2C5Addr),
        GLOBAL_NVS_OFFSET(I2C6Addr),
        GLOBAL_NVS_OFFSET(I2C7Addr)
      };

/**
  This function get I2Cx controller base address (BAR0).

  @param I2cControllerIndex  Bus Number of I2C controller.

  @return I2C BAR. 
**/
UINTN
GetI2cBarAddr(
  IN    UINT8 I2cControllerIndex
  )
{
  EFI_STATUS           Status;
  EFI_GLOBAL_NVS_AREA_PROTOCOL  *GlobalNvsArea;
  UINTN  AcpiBaseAddr;
  UINTN  PciMmBase=0;

  ASSERT(gBS!=NULL);

  Status = gBS->LocateProtocol (
                  &gEfiGlobalNvsAreaProtocolGuid,
                  NULL,
                  &GlobalNvsArea
                  );
                  
  //
  // PCI mode from PEI ( Global NVS is not ready).
  //
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_INFO, "GetI2cBarAddr() gEfiGlobalNvsAreaProtocolGuid:%r\n", Status));
    //
    // Global NVS is not ready.
    //
    return 0;
  }

  AcpiBaseAddr =  *(UINTN*)((CHAR8*)GlobalNvsArea->Area + mI2cNvsBaseAddress[I2cControllerIndex + 1]);
  
  //
  //PCI mode from DXE (global NVS protocal) to LPSS OnReadytoBoot(swith to ACPI).
  //
  if(AcpiBaseAddr==0) {
    PciMmBase = MmPciAddress (
                  mLpssPciDeviceList[I2cControllerIndex + 1].Segment,
                  mLpssPciDeviceList[I2cControllerIndex + 1].BusNum,
                  mLpssPciDeviceList[I2cControllerIndex + 1].DeviceNum,
                  mLpssPciDeviceList[I2cControllerIndex + 1].FunctionNum,
                  0
                  );
    DEBUG((EFI_D_ERROR, "\nGetI2cBarAddr() I2C Device %x %x %x PciMmBase:%x\n", \
           mLpssPciDeviceList[I2cControllerIndex + 1].BusNum, \
           mLpssPciDeviceList[I2cControllerIndex + 1].DeviceNum, \
           mLpssPciDeviceList[I2cControllerIndex + 1].FunctionNum, PciMmBase));

    if (MmioRead32 (PciMmBase) != 0xFFFFFFFF)    {
      if((MmioRead32 (PciMmBase+R_PCH_LPSS_I2C_STSCMD)& B_PCH_LPSS_I2C_STSCMD_MSE)) {
        //
        // Get the address allocted.
        //
        mLpssPciDeviceList[I2cControllerIndex + 1].Bar0=MmioRead32 (PciMmBase+R_PCH_LPSS_I2C_BAR);     
        mLpssPciDeviceList[I2cControllerIndex + 1].Bar1=MmioRead32 (PciMmBase+R_PCH_LPSS_I2C_BAR1);
      }
    }
    AcpiBaseAddr =mLpssPciDeviceList[I2cControllerIndex+1].Bar0;
  }
  
  //
  // ACPI mode from BDS: LPSS OnReadytoBoot
  //
  else {
    DEBUG ((EFI_D_INFO, "GetI2cBarAddr() NVS Varialable is updated by this LIB or LPSS  \n"));
  }
  
  DEBUG ((EFI_D_INFO, "GetI2cBarAddr() I2cControllerIndex+1 0x%x AcpiBaseAddr:0x%x \n", (I2cControllerIndex + 1), AcpiBaseAddr));
  return AcpiBaseAddr;
}


/**
  This function enables I2C controllers.

  @param I2cControllerIndex  Bus Number of I2C controllers.

  @return Result of the I2C initialization.
**/
EFI_STATUS
ProgramPciLpssI2C (
  IN  UINT8 I2cControllerIndex
  )
{
  UINT32                        PmcBase;
  UINTN                         PciMmBase=0;
  EFI_STATUS                    Status;
  EFI_GLOBAL_NVS_AREA_PROTOCOL  *GlobalNvsArea;

  UINT32 PmcFunctionDsiable[]= {
    B_PCH_PMC_FUNC_DIS_LPSS2_FUNC1,
    B_PCH_PMC_FUNC_DIS_LPSS2_FUNC2,
    B_PCH_PMC_FUNC_DIS_LPSS2_FUNC3,
    B_PCH_PMC_FUNC_DIS_LPSS2_FUNC4,
    B_PCH_PMC_FUNC_DIS_LPSS2_FUNC5,
    B_PCH_PMC_FUNC_DIS_LPSS2_FUNC6,
    B_PCH_PMC_FUNC_DIS_LPSS2_FUNC7
  };

  DEBUG ((EFI_D_INFO, "ProgramPciLpssI2C() Start\n"));

  //
  // Set the VLV Function Disable Register to ZERO
  //
  PmcBase = MmioRead32 (PCI_D31F0_REG_BASE + R_PCH_LPC_PMC_BASE) & B_PCH_LPC_PMC_BASE_BAR;
  if(MmioRead32(PmcBase+R_PCH_PMC_FUNC_DIS)&PmcFunctionDsiable[I2cControllerIndex]) {
    DEBUG ((EFI_D_INFO, "ProgramPciLpssI2C() End:I2C[%x] is disabled\n",I2cControllerIndex));
    return EFI_NOT_READY;
  }
  
  DEBUG ((EFI_D_INFO, "ProgramPciLpssI2C()------------I2cControllerIndex=%x,PMC=%x\n",I2cControllerIndex,MmioRead32(PmcBase+R_PCH_PMC_FUNC_DIS)));

  {
    PciMmBase = MmPciAddress (
                  mLpssPciDeviceList[I2cControllerIndex+1].Segment,
                  mLpssPciDeviceList[I2cControllerIndex+1].BusNum,
                  mLpssPciDeviceList[I2cControllerIndex+1].DeviceNum,
                  mLpssPciDeviceList[I2cControllerIndex+1].FunctionNum,
                  0
                  );
                  
    DEBUG((EFI_D_ERROR, "Program Pci Lpss I2C Device  %x %x %x PciMmBase:%x\n", \
           mLpssPciDeviceList[I2cControllerIndex+1].BusNum, \
           mLpssPciDeviceList[I2cControllerIndex+1].DeviceNum, \
           mLpssPciDeviceList[I2cControllerIndex+1].FunctionNum, PciMmBase));

    if (MmioRead32 (PciMmBase) != 0xFFFFFFFF)     {
      if((MmioRead32 (PciMmBase+R_PCH_LPSS_I2C_STSCMD)& B_PCH_LPSS_I2C_STSCMD_MSE)) {
        //
        // Get the address allocted.
        //
        mLpssPciDeviceList[I2cControllerIndex+1].Bar0=MmioRead32 (PciMmBase+R_PCH_LPSS_I2C_BAR);     
        mLpssPciDeviceList[I2cControllerIndex+1].Bar1=MmioRead32 (PciMmBase+R_PCH_LPSS_I2C_BAR1);
        DEBUG((EFI_D_ERROR, "ProgramPciLpssI2C() bar0:0x%x bar1:0x%x\n",mLpssPciDeviceList[I2cControllerIndex+1].Bar0, mLpssPciDeviceList[I2cControllerIndex+1].Bar1));
      } else {
        
        //
        // Program BAR 0
        //
        ASSERT (((mLpssPciDeviceList[I2cControllerIndex+1].Bar0 & B_PCH_LPSS_I2C_BAR_BA) == mLpssPciDeviceList[I2cControllerIndex+1].Bar0) && (mLpssPciDeviceList[I2cControllerIndex+1].Bar0 != 0));
        MmioWrite32 ((UINTN) (PciMmBase + R_PCH_LPSS_I2C_BAR), (UINT32) (mLpssPciDeviceList[I2cControllerIndex+1].Bar0 & B_PCH_LPSS_I2C_BAR_BA));
       
        //
        // Program BAR 1
        //
        ASSERT (((mLpssPciDeviceList[I2cControllerIndex+1].Bar1 & B_PCH_LPSS_I2C_BAR1_BA) == mLpssPciDeviceList[I2cControllerIndex+1].Bar1) && (mLpssPciDeviceList[I2cControllerIndex+1].Bar1 != 0));
        MmioWrite32 ((UINTN) (PciMmBase + R_PCH_LPSS_I2C_BAR1), (UINT32) (mLpssPciDeviceList[I2cControllerIndex+1].Bar1 & B_PCH_LPSS_I2C_BAR1_BA));
        
        //
        // Bus Master Enable & Memory Space Enable
        //
        MmioOr32 ((UINTN) (PciMmBase + R_PCH_LPSS_I2C_STSCMD), (UINT32) (B_PCH_LPSS_I2C_STSCMD_BME | B_PCH_LPSS_I2C_STSCMD_MSE));
        ASSERT (MmioRead32 (mLpssPciDeviceList[I2cControllerIndex+1].Bar0) != 0xFFFFFFFF);
      }
      
      //
      // Release Resets
      //
      MmioWrite32 (mLpssPciDeviceList[I2cControllerIndex+1].Bar0 + R_PCH_LPIO_I2C_MEM_RESETS,(B_PCH_LPIO_I2C_MEM_RESETS_FUNC | B_PCH_LPIO_I2C_MEM_RESETS_APB));
      
      //
      // Activate Clocks
      //
      MmioWrite32 (mLpssPciDeviceList[I2cControllerIndex+1].Bar0 + R_PCH_LPSS_I2C_MEM_PCP,0x80020003);//No use for A0

      DEBUG ((EFI_D_INFO, "ProgramPciLpssI2C() Programmed()\n"));
    }
    
    //
    // BDS: already switched to ACPI mode
    //
    else {
      ASSERT(gBS!=NULL);
      Status = gBS->LocateProtocol (
                      &gEfiGlobalNvsAreaProtocolGuid,
                      NULL,
                      &GlobalNvsArea
                      );
      if (EFI_ERROR(Status)) {
        DEBUG ((EFI_D_INFO, "GetI2cBarAddr() gEfiGlobalNvsAreaProtocolGuid:%r\n", Status));
        //
        // gEfiGlobalNvsAreaProtocolGuid is not ready.
        //
        return 0;
      }
      mLpssPciDeviceList[I2cControllerIndex + 1].Bar0 = *(UINTN*)((CHAR8*)GlobalNvsArea->Area + mI2cNvsBaseAddress[I2cControllerIndex + 1]);
      DEBUG ((EFI_D_INFO, "ProgramPciLpssI2C(): is switched to ACPI 0x:%x \n",mLpssPciDeviceList[I2cControllerIndex + 1].Bar0));
    }
  }
  
  DEBUG ((EFI_D_INFO, "ProgramPciLpssI2C() End\n"));

  return EFI_SUCCESS;
}

/**
  Disable I2C Bus.

  @param VOID.

  @return Result of the I2C disabling.
**/
RETURN_STATUS
I2cDisable (
  VOID
  )
{ 
  //
  // 0.1 seconds
  //
  UINT32 NumTries = 10000;
  
  MmioWrite32 ( mI2CBaseAddress + R_IC_ENABLE, 0 );
  while ( 0 != ( MmioRead32 ( mI2CBaseAddress + R_IC_ENABLE_STATUS) & 1)) {
    MicroSecondDelay (10);
    NumTries --;
    if(0 == NumTries) {
      return RETURN_NOT_READY;
    }
  }
  
  return RETURN_SUCCESS;
}

/**
  Enable I2C Bus.

  @param VOID.

  @return Result of the I2C disabling.
**/
RETURN_STATUS
I2cEnable (
  VOID
  )
{
  //
  // 0.1 seconds
  //
  UINT32 NumTries = 10000;
  
  MmioWrite32 (mI2CBaseAddress + R_IC_ENABLE, 1);
  
  while (0 == (MmioRead32 (mI2CBaseAddress + R_IC_ENABLE_STATUS) & 1)) {
    MicroSecondDelay (10);
    NumTries --;
    if(0 == NumTries){
       return RETURN_NOT_READY;
    }
  }
  
  return RETURN_SUCCESS;
}

/**
  Enable I2C Bus.

  @param VOID.

  @return Result of the I2C enabling.
**/
RETURN_STATUS
I2cBusFrequencySet (
  IN UINTN BusClockHertz
  )
{
  DEBUG((EFI_D_INFO,"InputFreq BusClockHertz: %d\r\n",BusClockHertz));
  
  //
  //  Set the 100 KHz clock divider according to SV result and I2C spec
  //
  MmioWrite32 ( mI2CBaseAddress + R_IC_SS_SCL_HCNT, (UINT16)0x214 );
  MmioWrite32 ( mI2CBaseAddress + R_IC_SS_SCL_LCNT, (UINT16)0x272 );
  
  //
  //  Set the 400 KHz clock divider according to SV result and I2C spec
  //
  MmioWrite32 ( mI2CBaseAddress + R_IC_FS_SCL_HCNT, (UINT16)0x50 );
  MmioWrite32 ( mI2CBaseAddress + R_IC_FS_SCL_LCNT, (UINT16)0xAD );

  switch ( BusClockHertz ) {
    case 100 * 1000:
      MmioWrite32 ( mI2CBaseAddress + R_IC_SDA_HOLD, (UINT16)0x40);//100K
      mI2cMode = V_SPEED_STANDARD;
      break;
    case 400 * 1000:
      MmioWrite32 ( mI2CBaseAddress + R_IC_SDA_HOLD, (UINT16)0x32);//400K
      mI2cMode = V_SPEED_FAST;
      break;
    default:
      MmioWrite32 ( mI2CBaseAddress + R_IC_SDA_HOLD, (UINT16)0x09);//3.4M
      mI2cMode = V_SPEED_HIGH;
  }

  //
  //  Select the frequency counter,
  //  Enable restart condition,
  //  Enable master FSM, disable slave FSM.
  //
  mI2cMode |= B_IC_RESTART_EN | B_IC_SLAVE_DISABLE | B_MASTER_MODE;

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
  IN  UINT8  I2cControllerIndex,
  IN  UINT16 SlaveAddress
  )
{
  EFI_STATUS Status=RETURN_SUCCESS;
  UINT32    NumTries = 0;
  UINTN    GnvsI2cBarAddr=0;
  
  //
  // Verify the parameters
  //
  if ((1023 < SlaveAddress) || (6 < I2cControllerIndex)) {
    Status =  RETURN_INVALID_PARAMETER;
    DEBUG((EFI_D_INFO,"I2CInit Exit with RETURN_INVALID_PARAMETER\r\n"));
    return Status;
  }
  MmioWrite32 ( mI2CBaseAddress + R_IC_TAR, (UINT16)SlaveAddress );
  mI2CSlaveAddress = SlaveAddress;

  //
  // 1.PEI: program and init ( before pci enumeration).
  // 2.DXE:update address and re-init ( after pci enumeration).
  // 3.BDS:update ACPI address and re-init ( after acpi mode is enabled).
  //
  if(mI2CBaseAddress == mLpssPciDeviceList[I2cControllerIndex + 1].Bar0) {
    
    //
    // I2CInit is already  called.
    //
    GnvsI2cBarAddr=GetI2cBarAddr(I2cControllerIndex);
    
    if((GnvsI2cBarAddr == 0)||(GnvsI2cBarAddr == mI2CBaseAddress)) {
      DEBUG((EFI_D_INFO,"I2CInit Exit with mI2CBaseAddress:%x == [%x].Bar0\r\n",mI2CBaseAddress,I2cControllerIndex+1));
      return RETURN_SUCCESS;
    }
  }
  
  Status=ProgramPciLpssI2C(I2cControllerIndex);
  if(Status!=EFI_SUCCESS) {
    return Status;
  }


  mI2CBaseAddress = (UINT32) mLpssPciDeviceList[I2cControllerIndex + 1].Bar0;
  DEBUG ((EFI_D_ERROR, "mI2CBaseAddress = 0x%x \n",mI2CBaseAddress));
  
  //
  // 1 seconds.
  //
  NumTries = 10000; 
  while ((1 == ( MmioRead32 ( mI2CBaseAddress + R_IC_STATUS) & STAT_MST_ACTIVITY ))) {
    MicroSecondDelay(10);
    NumTries --;
    if(0 == NumTries) {
      DEBUG((EFI_D_INFO, "Try timeout\r\n"));
      return RETURN_DEVICE_ERROR;
    }
  }
  
  Status = I2cDisable();
  DEBUG((EFI_D_INFO, "I2cDisable Status = %r\r\n", Status));
  I2cBusFrequencySet(400 * 1000);

  MmioWrite32(mI2CBaseAddress + R_IC_INTR_MASK, 0x0);
  if (0x7f < SlaveAddress )
    SlaveAddress = ( SlaveAddress & 0x3ff ) | IC_TAR_10BITADDR_MASTER;
  MmioWrite32 ( mI2CBaseAddress + R_IC_TAR, (UINT16)SlaveAddress );
  MmioWrite32 ( mI2CBaseAddress + R_IC_RX_TL, 0);
  MmioWrite32 ( mI2CBaseAddress + R_IC_TX_TL, 0 );
  MmioWrite32 ( mI2CBaseAddress + R_IC_CON, mI2cMode);
  Status = I2cEnable();

  DEBUG((EFI_D_INFO, "I2cEnable Status = %r\r\n", Status));
  MmioRead32 ( mI2CBaseAddress + R_IC_CLR_TX_ABRT );
  
  return EFI_SUCCESS;
}

/**
  Reads a Byte from I2C Device.
 
  @param  I2cControllerIndex             I2C Bus no to which the I2C device has been connected
  @param  SlaveAddress      Device Address from which the byte value has to be read
  @param  Offset            Offset from which the data has to be read
  @param  *Byte             Address to which the value read has to be stored
  @param  Start               Whether a RESTART is issued before the byte is sent or received
  @param  End                 Whether STOP is generated after a data byte is sent or received  
                                  
  @return  EFI_SUCCESS       IF the byte value has been successfully read
  @return  EFI_DEVICE_ERROR  Operation Failed, Device Error
**/
EFI_STATUS 
ByteReadI2CBasic(
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
  UINT32 Count=0;

  Status = EFI_SUCCESS;

  ReceiveDataEnd = &ReadBuffer [ReadBytes];
  if( ReadBytes ) {

    ReceiveRequest = ReadBuffer;
    DEBUG((EFI_D_INFO,"Read: ---------------%d bytes to RX\r\n",ReceiveDataEnd - ReceiveRequest));

    while ((ReceiveDataEnd > ReceiveRequest) || (ReceiveDataEnd > ReadBuffer)) {
      
      //
      //  Check for NACK
      //
      RawIntrStat = (UINT16)MmioRead32 (mI2CBaseAddress + R_IC_RawIntrStat);
      if ( 0 != ( RawIntrStat & I2C_INTR_TX_ABRT )) {
        MmioRead32 ( mI2CBaseAddress + R_IC_CLR_TX_ABRT );
        Status = RETURN_DEVICE_ERROR;
        DEBUG((EFI_D_INFO,"TX ABRT ,%d bytes hasn't been transferred\r\n",ReceiveDataEnd - ReceiveRequest));
        break;
      }
      
      //
      // Determine if another byte was received
      //
      I2cStatus = (UINT16)MmioRead32 (mI2CBaseAddress + R_IC_STATUS);
      if (0 != ( I2cStatus & STAT_RFNE )) {
        ReceiveData = (UINT16)MmioRead32 ( mI2CBaseAddress + R_IC_DATA_CMD );
        *ReadBuffer++ = (UINT8)ReceiveData;
        DEBUG((EFI_D_INFO,"MmioRead32 ,1 byte 0x:%x is received\r\n",ReceiveData));
      }

      if(ReceiveDataEnd == ReceiveRequest) {
        MicroSecondDelay ( FIFO_WRITE_DELAY );
        DEBUG((EFI_D_INFO,"ReceiveDataEnd==ReceiveRequest------------%x\r\n",I2cStatus & STAT_RFNE));
        Count++;
        if(Count<1024) {
          //
          // To avoid sys hung  without ul-pmc device  on RVP,
          // waiting the last request to get data and make (ReceiveDataEnd > ReadBuffer) =TRUE.
          //
          continue;
        } else {
          break;
        }
      }
      
      //
      //  Wait until a read request will fit.
      //
      if (0 == (I2cStatus & STAT_TFNF)) {
        DEBUG((EFI_D_INFO,"Wait until a read request will fit\r\n"));
        MicroSecondDelay (10);
        continue;
      }
      
      //
      //  Issue the next read request.
      //
      if(End && Start) {
        MmioWrite32 ( mI2CBaseAddress + R_IC_DATA_CMD, B_READ_CMD|B_CMD_RESTART|B_CMD_STOP);
      } else if (!End && Start) {
        MmioWrite32 ( mI2CBaseAddress + R_IC_DATA_CMD, B_READ_CMD|B_CMD_RESTART);
      } else if (End && !Start) {
        MmioWrite32 ( mI2CBaseAddress + R_IC_DATA_CMD, B_READ_CMD|B_CMD_STOP);
      } else if (!End && !Start) {
        MmioWrite32 ( mI2CBaseAddress + R_IC_DATA_CMD, B_READ_CMD);
      }
      MicroSecondDelay (FIFO_WRITE_DELAY);

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
  @param  Start               Whether a RESTART is issued before the byte is sent or received
  @param  End                 Whether STOP is generated after a data byte is sent or received  
                                    
  @return  EFI_SUCCESS         IF the byte value has been successfully written
  @return  EFI_DEVICE_ERROR    Operation Failed, Device Error
**/
EFI_STATUS ByteWriteI2CBasic(
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
  UINT32 Count=0;

  Status = EFI_SUCCESS;

  Status=I2CInit(I2cControllerIndex, SlaveAddress);
  if(Status!=EFI_SUCCESS)
    return Status;

  TransmitEnd = &WriteBuffer[WriteBytes];
  if( WriteBytes ) {
    DEBUG((EFI_D_INFO,"Write: --------------%d bytes to TX\r\n",TransmitEnd - WriteBuffer));
    while (TransmitEnd > WriteBuffer) {
      I2cStatus = MmioRead32 (mI2CBaseAddress + R_IC_STATUS);
      RawIntrStat = (UINT16)MmioRead32 (mI2CBaseAddress + R_IC_RawIntrStat);
      if (0 != ( RawIntrStat & I2C_INTR_TX_ABRT)) {
        MmioRead32 ( mI2CBaseAddress + R_IC_CLR_TX_ABRT);
        Status = RETURN_DEVICE_ERROR;
        DEBUG((EFI_D_ERROR,"TX ABRT TransmitEnd:0x%x WriteBuffer:0x%x\r\n", TransmitEnd, WriteBuffer));
        break;
      }
      if (0 == (I2cStatus & STAT_TFNF)) {
        //
        // If TX not full , will  send cmd  or continue to wait
        //
        MicroSecondDelay (FIFO_WRITE_DELAY);
        continue;
      }

      if(End && Start) {
        MmioWrite32 (mI2CBaseAddress + R_IC_DATA_CMD, (*WriteBuffer++)|B_CMD_RESTART|B_CMD_STOP);
      } else if (!End && Start) {
        MmioWrite32 (mI2CBaseAddress + R_IC_DATA_CMD, (*WriteBuffer++)|B_CMD_RESTART);
      } else if (End && !Start) {
        MmioWrite32 (mI2CBaseAddress + R_IC_DATA_CMD, (*WriteBuffer++)|B_CMD_STOP);
      } else if (!End && !Start ) {
        MmioWrite32 (mI2CBaseAddress + R_IC_DATA_CMD, (*WriteBuffer++));
      }
      
      //
      // Add a small delay to work around some odd behavior being seen.  Without this delay bytes get dropped.
      //
      MicroSecondDelay ( FIFO_WRITE_DELAY );//wait after send cmd
      
      //
      // Time out
      //
      while(1) {
        RawIntrStat = MmioRead16 ( mI2CBaseAddress + R_IC_RawIntrStat );
        if (0 != ( RawIntrStat & I2C_INTR_TX_ABRT)) {
          MmioRead16 (mI2CBaseAddress + R_IC_CLR_TX_ABRT);
          Status = RETURN_DEVICE_ERROR;
          DEBUG((EFI_D_ERROR,"TX ABRT TransmitEnd:0x%x WriteBuffer:0x%x\r\n", TransmitEnd, WriteBuffer));
        }
        if(0 == MmioRead16(mI2CBaseAddress + R_IC_TXFLR)) break;

        MicroSecondDelay (FIFO_WRITE_DELAY);
        Count++;
        if(Count<1024) {
          //
          // to avoid sys hung without ul-pmc device on RVP.
          // Waiting the last request to get data and make (ReceiveDataEnd > ReadBuffer) =TRUE.
          //
          continue;
        } else {
          break;
        }
      }//while( 1 )
    }

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
EFI_STATUS ByteReadI2C(
  IN  UINT8 I2cControllerIndex,
  IN  UINT8 SlaveAddress,
  IN  UINT8 Offset,
  IN  UINTN ReadBytes,
  OUT UINT8 *ReadBuffer
  )
{
  EFI_STATUS          Status;

  DEBUG ((EFI_D_INFO, "ByteReadI2C:---offset:0x%x\n",Offset));
  Status = ByteWriteI2CBasic(I2cControllerIndex, SlaveAddress,1,&Offset,TRUE,FALSE);
  Status = ByteReadI2CBasic(I2cControllerIndex, SlaveAddress,ReadBytes,ReadBuffer,TRUE,TRUE);

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
  EFI_STATUS          Status;

  DEBUG ((EFI_D_INFO, "ByteWriteI2C:---offset/bytes/buf:0x%x,0x%x,0x%x,0x%x\n",Offset,WriteBytes,WriteBuffer,*WriteBuffer));
  Status = ByteWriteI2CBasic(I2cControllerIndex, SlaveAddress,1,&Offset,TRUE,FALSE);
  Status = ByteWriteI2CBasic(I2cControllerIndex, SlaveAddress,WriteBytes,WriteBuffer,FALSE,TRUE);

  return Status;
}
