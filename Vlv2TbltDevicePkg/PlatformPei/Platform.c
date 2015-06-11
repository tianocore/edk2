/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

**/

#include "CommonHeader.h"

#include "Platform.h"
#include <Library/PciCf8Lib.h>
#include "PlatformBaseAddresses.h"
#include "PchAccess.h"
#include <Guid/PlatformInfo.h>
#include "Platform.h"
#include "PchCommonDefinitions.h"
#include <Ppi/MfgMemoryTest.h>
#include <Guid/SetupVariable.h>
#include <Guid/Vlv2Variable.h>
#include <Ppi/fTPMPolicy.h>

//
// Start::Alpine Valley platform
//
enum {
  SMBUS_READ_BYTE,
  SMBUS_WRITE_BYTE,
  SMBUS_READ_BLOCK,
  SMBUS_WRITE_BLOCK
};

#define EC_BASE           0xE0000000

//
// DEVICE 0 (Memroy Controller Hub)
//
#define MC_BUS              0x00
#define MC_DEV              0x00
#define MC_FUN              0x00
#define MC_DEV_FUN          (MC_DEV << 3)
#define MC_BUS_DEV_FUN      ((MC_BUS << 8) + MC_DEV_FUN)

//
// SysCtl SMBus address and block size
//
#define AV_SC_SMBUS_ADDRESS        	0x60
#define AV_SC_BYTE_LEN            	1
#define AV_SC_BLOCK_LEN            	4
#define AV_SC_SMBUS_WRCMD           1
#define AV_SC_SMBUS_RDCMD           0

//
// SysCtl registers offset
//
#define AV_SC_REG_PLATFORM_ID               24  // 0x18
#define AV_SC_REG_BOARD_ID                  28  // 0x1C
#define AV_SC_REG_FAB_ID                    32  // 0x20
#define AV_SC_REG_ECO_ID                    68  // 0x44
#define AV_SC_REG_DDR_DAUGHTER_CARD_ID      144 // 0x90
#define AV_SC_REG_SODIMM_CONFIG             36

//
// ID values
//
#define AV_SC_PLATFORM_ID_TABLET   0
#define AV_SC_PLATFORM_ID_NETBOOK  2
#define AV_SC_PLATFORM_ID_INTERPOSER 3 // Configuration TBD
#define AV_SC_BOARD_ID_AV_SVP      1492

#define BUS_TRIES                 3       // How many times to retry on Bus Errors

#define GTT_SIZE_1MB        1
#define GTT_SIZE_2MB        2

#define PciCfg16Read( PciExpressBase, Bus, Device, Function, Register ) \
  MmioRead16(PciExpressBase + \
    (UINTN)(Bus << 20) + \
    (UINTN)(Device << 15) + \
    (UINTN)(Function << 12) + \
    (UINTN)(Register))
#define PciCfg16Write( PciExpressBase, Bus, Device, Function, Register, Data ) \
    MmioWrite16(PciExpressBase + \
      (UINTN)(Bus << 20) + \
      (UINTN)(Device << 15) + \
      (UINTN)(Function << 12) + \
      (UINTN)(Register), \
      (UINT16)Data)


//
//Memory Test Manufacturing mode
//
UINT32 DataPatternForMemoryTest[] = {
    0x55555555, 0xAAAAAAAA, 0x55555510, 0x555555EF, 0x55555510, 0x555555EF, 0x55555510, 0x555555EF,
    0x55555555, 0xAAAAAAAA, 0x55551055, 0x5555EF55, 0x55551055, 0x5555EF55, 0x55551055, 0x5555EF55,
    0x55555555, 0xAAAAAAAA, 0x55105555, 0x55EF5555, 0x55105555, 0x55EF5555, 0x55105555, 0x55EF5555,
    0x55555555, 0xAAAAAAAA, 0x10555555, 0xEF555555, 0x10555555, 0xEF555555, 0x10555555, 0xEF555555
};
#define DATA_PATTERN_ARRAY_SIZE (sizeof(DataPatternForMemoryTest) / sizeof(UINT32))

//
//Memory Test Manufacturing mode
//
//
// The global indicator, the FvFileLoader callback will modify it to TRUE after loading PEIM into memory
//
BOOLEAN ImageInMemory = FALSE;

EFI_STATUS
EFIAPI
Stall (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN CONST EFI_PEI_STALL_PPI      *This,
  IN UINTN              Microseconds
  );

EFI_STATUS
EFIAPI
MfgMemoryTest (
  IN  CONST EFI_PEI_SERVICES                   **PeiServices,
  IN  PEI_MFG_MEMORY_TEST_PPI           *This,
  IN  UINT32                             BeginAddress,
  IN  UINT32                             MemoryLength
  );

static EFI_PEI_STALL_PPI  mStallPpi = {
  PEI_STALL_RESOLUTION,
  Stall
};

static PEI_MFG_MEMORY_TEST_PPI mPeiMfgMemoryTestPpi = {
  MfgMemoryTest
};

static EFI_PEI_PPI_DESCRIPTOR mInstallStallPpi[] = {
  {
  EFI_PEI_PPI_DESCRIPTOR_PPI,
  &gEfiPeiStallPpiGuid,
  &mStallPpi
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gPeiMfgMemoryTestPpiGuid,
    &mPeiMfgMemoryTestPpi
  }
 };

EFI_PEI_NOTIFY_DESCRIPTOR mMemoryDiscoveredNotifyList[1] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMemoryDiscoveredPpiGuid,
    MemoryDiscoveredPpiNotifyCallback
  }
};

EFI_STATUS
EFIAPI
InstallMonoStatusCode (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN CONST EFI_PEI_SERVICES          **PeiServices
  );


EFI_STATUS
ReadPlatformIds (
  IN CONST EFI_PEI_SERVICES             **PeiServices,
  IN OUT EFI_PLATFORM_INFO_HOB          *PlatformInfoHob
  );

//
// Start::Alpine Valley platform
//
EFI_STATUS
PeiSmbusExec (
  UINT16 SmbusBase,
  UINT8 SlvAddr,
  UINT8 Operation,
  UINT8 Offset,
  UINT8 *Length,
  UINT8 *Buffer
  );


EFI_STATUS
FtpmPolicyInit (
  IN CONST EFI_PEI_SERVICES             **PeiServices,
  IN SYSTEM_CONFIGURATION         *pSystemConfiguration
  )
{
  EFI_STATUS                      Status;
  EFI_PEI_PPI_DESCRIPTOR          *mFtpmPolicyPpiDesc;
  SEC_FTPM_POLICY_PPI             *mFtpmPolicyPpi;


  DEBUG((EFI_D_INFO, "FtpmPolicyInit Entry \n"));

  if (NULL == PeiServices ||  NULL == pSystemConfiguration) {
    DEBUG((EFI_D_ERROR, "Input error. \n"));
    return EFI_INVALID_PARAMETER;
  }
  
  Status = (*PeiServices)->AllocatePool(
                             PeiServices,
                             sizeof (EFI_PEI_PPI_DESCRIPTOR),
                             (void **)&mFtpmPolicyPpiDesc
                             );
  ASSERT_EFI_ERROR (Status);

  Status = (*PeiServices)->AllocatePool(
                             PeiServices,
                             sizeof (SEC_FTPM_POLICY_PPI),
                             (void **)&mFtpmPolicyPpi
                             );
  ASSERT_EFI_ERROR (Status);

  //
  // Initialize PPI
  //
  (*PeiServices)->SetMem ((VOID *)mFtpmPolicyPpi, sizeof (SEC_FTPM_POLICY_PPI), 0);
  mFtpmPolicyPpiDesc->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  mFtpmPolicyPpiDesc->Guid = &gSeCfTPMPolicyPpiGuid;
  mFtpmPolicyPpiDesc->Ppi = mFtpmPolicyPpi;


  DEBUG((EFI_D_INFO, "pSystemConfiguration->fTPM = 0x%x \n", pSystemConfiguration->fTPM)); 
  if(pSystemConfiguration->fTPM == 1) {
    mFtpmPolicyPpi->fTPMEnable = TRUE;
  } else {
    mFtpmPolicyPpi->fTPMEnable = FALSE;
  }

  Status = (*PeiServices)->InstallPpi(
                             PeiServices,
                             mFtpmPolicyPpiDesc
                             );
  ASSERT_EFI_ERROR (Status);

  DEBUG((EFI_D_INFO, "FtpmPolicyInit done \n"));
  
  return EFI_SUCCESS;
}


/**
  This routine attempts to acquire the SMBus

  @retval FAILURE as failed
  @retval SUCCESS as passed

**/
EFI_STATUS
AcquireBus (
    UINT16	SmbusBase
  )
{
  UINT8 StsReg;

  StsReg  = 0;
  StsReg  = (UINT8)IoRead8(SmbusBase + R_PCH_SMBUS_HSTS);
  if (StsReg & B_PCH_SMBUS_IUS) {
    return EFI_DEVICE_ERROR;
  } else if (StsReg & B_PCH_SMBUS_HBSY) {
    //
    // Clear Status Register and exit
    //
    // Wait for HSTS.HBSY to be clear
	  //
    do { StsReg = (UINT8) IoRead8(SmbusBase+R_PCH_SMBUS_HSTS); } while ((StsReg & B_PCH_SMBUS_HBSY) != 0);

	  //
    // Clear all status bits
	  //
    IoWrite8(SmbusBase+R_PCH_SMBUS_HSTS, 0xFE);
    return EFI_SUCCESS;
  } else {
    //
    // Clear out any odd status information (Will Not Clear In Use)
    //
    IoWrite8(SmbusBase+R_PCH_SMBUS_HSTS, StsReg);
    return EFI_SUCCESS;
  }
}
//
// End::Alpine Valley platform
//

/**
  This function checks the memory range in PEI.

  @param  PeiServices     Pointer to PEI Services.
  @param  This            Pei memory test PPI pointer.
  @param  BeginAddress    Beginning of the memory address to be checked.
  @param  MemoryLength    Bytes of memory range to be checked.
  @param  Operation       Type of memory check operation to be performed.
  @param  ErrorAddress    Return the address of the error memory address.

  @retval  EFI_SUCCESS         The operation completed successfully.
  @retval  EFI_DEVICE_ERROR    Memory test failed. It's not safe to use this range of memory.

**/
EFI_STATUS
EFIAPI
MfgMemoryTest (
  IN  CONST EFI_PEI_SERVICES                   **PeiServices,
  IN  PEI_MFG_MEMORY_TEST_PPI           *This,
  IN  UINT32                             BeginAddress,
  IN  UINT32                             MemoryLength
  )
{
  UINT32 i;
  UINT32 memAddr;
  UINT32 readData;
  UINT32 xorData;
  UINT32 TestFlag = 0;
  memAddr = BeginAddress;

  //
  //Output Message for MFG
  //
  DEBUG ((EFI_D_ERROR, "MFGMODE SET\n"));

  //
  //Writting the pattern in defined location.
  //
  while (memAddr < (BeginAddress+MemoryLength)) {
    for (i = 0; i < DATA_PATTERN_ARRAY_SIZE; i++) {
      if (memAddr > (BeginAddress+MemoryLength -4)) {
        memAddr = memAddr + 4;
        break;
      }
      *((volatile UINT32*) memAddr) = DataPatternForMemoryTest[i];
      memAddr = memAddr + 4;
    }
  }

  //
  //Verify the pattern.
  //
  memAddr = BeginAddress;
  while (memAddr < (BeginAddress+MemoryLength)) {
  for (i = 0; i < DATA_PATTERN_ARRAY_SIZE; i++) {
    if (memAddr > (BeginAddress+MemoryLength -4)) {
      memAddr = memAddr + 4;
      break;
    }
    readData = *((volatile UINT32*) memAddr);
    xorData = readData ^ DataPatternForMemoryTest[i];

	  //
    // If xorData is nonzero, this particular memAddr has a failure.
	  //
    if (xorData != 0x00000000) {
      DEBUG ((EFI_D_ERROR, "Expected value....: %x\n", DataPatternForMemoryTest[i]));
      DEBUG ((EFI_D_ERROR, "ReadData value....: %x\n", readData));
      DEBUG ((EFI_D_ERROR, "Pattern failure at....: %x\n", memAddr));
      TestFlag = 1;
    }
    memAddr = memAddr + 4;
    }
  }
  if (TestFlag) {
    return EFI_DEVICE_ERROR;
  }

  //
  //Output Message for MFG
  //
  DEBUG ((EFI_D_ERROR, "MFGMODE MEMORY TEST PASSED\n"));
  return EFI_SUCCESS;
}

BOOLEAN
IsRtcUipAlwaysSet (
  IN CONST EFI_PEI_SERVICES       **PeiServices
  )
{

  EFI_PEI_STALL_PPI *StallPpi;
  UINTN             Count;

  (**PeiServices).LocatePpi (PeiServices, &gEfiPeiStallPpiGuid, 0, NULL, (void **)&StallPpi);

  for (Count = 0; Count < 500; Count++) { // Maximum waiting approximates to 1.5 seconds (= 3 msec * 500)
    IoWrite8 (R_PCH_RTC_INDEX2, R_PCH_RTC_REGISTERA);
    if ((IoRead8 (R_PCH_RTC_TARGET2) & B_PCH_RTC_REGISTERA_UIP) == 0) {
      return FALSE;
    }

    StallPpi->Stall (PeiServices, StallPpi, 3000);
  }

  return TRUE;
}

EFI_STATUS
RtcPowerFailureHandler (
  IN CONST EFI_PEI_SERVICES       **PeiServices
  )
{

  UINT16          DataUint16;
  UINT8           DataUint8;
  BOOLEAN         RtcUipIsAlwaysSet;
  DataUint16        = MmioRead16 (PMC_BASE_ADDRESS + R_PCH_PMC_GEN_PMCON_1);
  RtcUipIsAlwaysSet = IsRtcUipAlwaysSet (PeiServices);
  if ((DataUint16 & B_PCH_PMC_GEN_PMCON_RTC_PWR_STS) || (RtcUipIsAlwaysSet)) {
    //
    // Execute the sequence below. This will ensure that the RTC state machine has been initialized.
    //
    // Step 1.
    // BIOS clears this bit by writing a '0' to it.
    //
    if (DataUint16 & B_PCH_PMC_GEN_PMCON_RTC_PWR_STS) {
      //
      // Set to invalid date in order to reset the time to
      // BIOS build time later in the boot (SBRUN.c file).
      //
      IoWrite8 (R_PCH_RTC_INDEX2, R_PCH_RTC_YEAR);
      IoWrite8 (R_PCH_RTC_TARGET2, 0x0FF);
      IoWrite8 (R_PCH_RTC_INDEX2, R_PCH_RTC_MONTH);
      IoWrite8 (R_PCH_RTC_TARGET2, 0x0FF);
      IoWrite8 (R_PCH_RTC_INDEX2, R_PCH_RTC_DAYOFMONTH);
      IoWrite8 (R_PCH_RTC_TARGET2, 0x0FF);
      IoWrite8 (R_PCH_RTC_INDEX2, R_PCH_RTC_DAYOFWEEK);
      IoWrite8 (R_PCH_RTC_TARGET2, 0x0FF);

      IoWrite8 (R_PCH_RTC_INDEX2, R_PCH_RTC_SECONDSALARM);
      IoWrite8 (R_PCH_RTC_TARGET2, 0x00);
      IoWrite8 (R_PCH_RTC_INDEX2, R_PCH_RTC_MINUTESALARM);
      IoWrite8 (R_PCH_RTC_TARGET2, 0x00);
      IoWrite8 (R_PCH_RTC_INDEX2, R_PCH_RTC_HOURSALARM);
      IoWrite8 (R_PCH_RTC_TARGET2, 0x00);
    }

    //
    // Step 2.
    // Set RTC Register 0Ah[6:4] to '110' or '111'.
    //
    IoWrite8 (R_PCH_RTC_INDEX2, R_PCH_RTC_REGISTERA);
    IoWrite8 (R_PCH_RTC_TARGET2, (V_PCH_RTC_REGISTERA_DV_DIV_RST1 | V_PCH_RTC_REGISTERA_RS_976P5US));

    //
    // Step 3.
    // Set RTC Register 0Bh[7].
    //
    IoWrite8 (R_PCH_RTC_INDEX2, R_PCH_RTC_REGISTERB);
    DataUint8 = (IoRead8 (R_PCH_RTC_TARGET2) | B_PCH_RTC_REGISTERB_SET);
    IoWrite8 (R_PCH_RTC_INDEX2, R_PCH_RTC_REGISTERB);
    IoWrite8 (R_PCH_RTC_TARGET2, DataUint8);

    //
    // Step 4.
    // Set RTC Register 0Ah[6:4] to '010'.
    //
    IoWrite8 (R_PCH_RTC_INDEX2, R_PCH_RTC_REGISTERA);
    IoWrite8 (R_PCH_RTC_TARGET2, (V_PCH_RTC_REGISTERA_DV_NORM_OP | V_PCH_RTC_REGISTERA_RS_976P5US));

    //
    // Step 5.
    // Clear RTC Register 0Bh[7].
    //
    IoWrite8 (R_PCH_RTC_INDEX2, R_PCH_RTC_REGISTERB);
    DataUint8 = (IoRead8 (R_PCH_RTC_TARGET2) & (UINT8)~B_PCH_RTC_REGISTERB_SET);
    IoWrite8 (R_PCH_RTC_INDEX2, R_PCH_RTC_REGISTERB);
    IoWrite8 (R_PCH_RTC_TARGET2, DataUint8);
  }

  return EFI_SUCCESS;
}


VOID
PchBaseInit (
  VOID
  )
{
  //
  // Program ACPI Power Management I/O Space Base Address
  //
  MmioWrite16 (
    MmPciAddress (0,
      DEFAULT_PCI_BUS_NUMBER_PCH,
      PCI_DEVICE_NUMBER_PCH_LPC,
      PCI_FUNCTION_NUMBER_PCH_LPC,
      R_PCH_LPC_ACPI_BASE
    ),
    (UINT16)((ACPI_BASE_ADDRESS & B_PCH_LPC_ACPI_BASE_BAR) | B_PCH_LPC_ACPI_BASE_EN)
  );

  //
  // Program GPIO Base Address
  //
  MmioWrite16 (
    MmPciAddress (0,
      DEFAULT_PCI_BUS_NUMBER_PCH,
      PCI_DEVICE_NUMBER_PCH_LPC,
      PCI_FUNCTION_NUMBER_PCH_LPC,
      R_PCH_LPC_GPIO_BASE
    ),
    (UINT16)((GPIO_BASE_ADDRESS & B_PCH_LPC_GPIO_BASE_BAR) | B_PCH_LPC_GPIO_BASE_EN)
  );

  //
  // Set PMC Base Address
  //
  MmioWrite32 (
    MmPciAddress (0,
      DEFAULT_PCI_BUS_NUMBER_PCH,
      PCI_DEVICE_NUMBER_PCH_LPC,
      PCI_FUNCTION_NUMBER_PCH_LPC,
      R_PCH_LPC_PMC_BASE
    ),
    (UINT32)((PMC_BASE_ADDRESS & B_PCH_LPC_PMC_BASE_BAR) | B_PCH_LPC_PMC_BASE_EN)
  );

  //
  // Set IO Base Address
  //
  MmioWrite32 (
    MmPciAddress (0,
      DEFAULT_PCI_BUS_NUMBER_PCH,
      PCI_DEVICE_NUMBER_PCH_LPC,
      PCI_FUNCTION_NUMBER_PCH_LPC,
      R_PCH_LPC_IO_BASE
    ),
    (UINT32)((IO_BASE_ADDRESS & B_PCH_LPC_IO_BASE_BAR) | B_PCH_LPC_IO_BASE_EN)
  );

  //
  // Set ILB Base Address
  //
  MmioWrite32 (
    MmPciAddress (0,
      DEFAULT_PCI_BUS_NUMBER_PCH,
      PCI_DEVICE_NUMBER_PCH_LPC,
      PCI_FUNCTION_NUMBER_PCH_LPC,
      R_PCH_LPC_ILB_BASE
    ),
    (UINT32)((ILB_BASE_ADDRESS & B_PCH_LPC_ILB_BASE_BAR) | B_PCH_LPC_ILB_BASE_EN)
  );

  //
  // Set PUnit Base Address
  //
  MmioWrite32 (
    MmPciAddress (0,
      DEFAULT_PCI_BUS_NUMBER_PCH,
      PCI_DEVICE_NUMBER_PCH_LPC,
      PCI_FUNCTION_NUMBER_PCH_LPC,
      R_PCH_LPC_PUNIT_BASE
    ),
    (UINT32)((PUNIT_BASE_ADDRESS & B_PCH_LPC_PUNIT_BASE_BAR) | B_PCH_LPC_PUNIT_BASE_EN)
  );

  //
  // Set SPI Base Address
  //
  MmioWrite32 (
    MmPciAddress (0,
      DEFAULT_PCI_BUS_NUMBER_PCH,
      PCI_DEVICE_NUMBER_PCH_LPC,
      PCI_FUNCTION_NUMBER_PCH_LPC,
      R_PCH_LPC_SPI_BASE
    ),
    (UINT32)((SPI_BASE_ADDRESS & B_PCH_LPC_SPI_BASE_BAR) | B_PCH_LPC_SPI_BASE_EN)
  );

  //
  // Set Root Complex Base Address
  //
  MmioWrite32 (
    MmPciAddress (0,
      DEFAULT_PCI_BUS_NUMBER_PCH,
      PCI_DEVICE_NUMBER_PCH_LPC,
      PCI_FUNCTION_NUMBER_PCH_LPC,
      R_PCH_LPC_RCBA
    ),
    (UINT32)((RCBA_BASE_ADDRESS & B_PCH_LPC_RCBA_BAR) | B_PCH_LPC_RCBA_EN)
  );

  //
  // Set MPHY Base Address
  //
  MmioWrite32 (
    MmPciAddress (0,
      DEFAULT_PCI_BUS_NUMBER_PCH,
      PCI_DEVICE_NUMBER_PCH_LPC,
      PCI_FUNCTION_NUMBER_PCH_LPC,
      R_PCH_LPC_MPHY_BASE
    ),
    (UINT32)((MPHY_BASE_ADDRESS & B_PCH_LPC_MPHY_BASE_BAR) | B_PCH_LPC_MPHY_BASE_EN)
  );
  MmioWrite16 (
    MmPciAddress (0,
      DEFAULT_PCI_BUS_NUMBER_PCH,
      PCI_DEVICE_NUMBER_PCH_SMBUS,
      PCI_FUNCTION_NUMBER_PCH_SMBUS,
      R_PCH_SMBUS_BASE
    ),
    (UINT16)(SMBUS_BASE_ADDRESS & B_PCH_SMBUS_BASE_BAR)
  );

  MmioOr8 (
    MmPciAddress (0,
      DEFAULT_PCI_BUS_NUMBER_PCH,
      PCI_DEVICE_NUMBER_PCH_SMBUS,
      PCI_FUNCTION_NUMBER_PCH_SMBUS,
      R_PCH_SMBUS_PCICMD
    ),
    B_PCH_SMBUS_PCICMD_IOSE
  );

}

/**
  This is the entrypoint of PEIM

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
PeiInitPlatform (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  UINTN                            SmbusRegBase;
  EFI_PLATFORM_INFO_HOB            PlatformInfo;
  EFI_STATUS                       Status= EFI_SUCCESS;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *Variable = NULL;
  UINTN                            VariableSize;
  SYSTEM_CONFIGURATION             SystemConfiguration;
  UINT32                           GGC = 0;

  EFI_PEI_PPI_DESCRIPTOR          *mVlvMmioPolicyPpiDesc;
  VLV_MMIO_POLICY_PPI             *mVlvMmioPolicyPpi;

  ZeroMem (&PlatformInfo, sizeof(PlatformInfo));

  Status =  InstallMonoStatusCode(FileHandle, PeiServices);
  ASSERT_EFI_ERROR (Status);


  //
  // Initialize Stall PPIs
  //
  Status = (*PeiServices)->InstallPpi (PeiServices, &mInstallStallPpi[0]);
  ASSERT_EFI_ERROR (Status);

  Status = (*PeiServices)->NotifyPpi (PeiServices, &mMemoryDiscoveredNotifyList[0]);
  ASSERT_EFI_ERROR (Status);
  SmbusRegBase = PchPciDeviceMmBase (
                   DEFAULT_PCI_BUS_NUMBER_PCH,
                   PCI_DEVICE_NUMBER_PCH_SMBUS,
                   PCI_FUNCTION_NUMBER_PCH_SMBUS
                   );
  //
  // Since PEI has no PCI enumerator, set the BAR & I/O space enable ourselves
  //
  MmioAndThenOr32 (SmbusRegBase + R_PCH_SMBUS_BASE, B_PCH_SMBUS_BASE_BAR, SMBUS_BASE_ADDRESS);

  MmioOr8 (SmbusRegBase + R_PCH_SMBUS_PCICMD, B_PCH_SMBUS_PCICMD_IOSE);

  PchBaseInit();

  //
  //Todo: confirm if we need program 8254
  //
  // Setting 8254
  // Program timer 1 as refresh timer
  //
  IoWrite8 (0x43, 0x54);
  IoWrite8 (0x41, 0x12);

  //
  // RTC power failure handling
  //
  RtcPowerFailureHandler (PeiServices);


  PchMmPci32( 0, 0, 2, 0, 0x50) = 0x210;

  VariableSize = sizeof (SYSTEM_CONFIGURATION);
  ZeroMem (&SystemConfiguration, VariableSize);

  //
  // Obtain variable services
  //
  Status = (*PeiServices)->LocatePpi(
                             PeiServices,
                             &gEfiPeiReadOnlyVariable2PpiGuid,
                             0,
                             NULL,
                             (void **)&Variable
                             );
  ASSERT_EFI_ERROR(Status);
  Status = Variable->GetVariable (
                       Variable,
                       L"Setup",
                       &gEfiSetupVariableGuid,
                       NULL,
                       &VariableSize,
                       &SystemConfiguration
					   );
  if (EFI_ERROR (Status) || VariableSize != sizeof(SYSTEM_CONFIGURATION)) {
    //The setup variable is corrupted
    VariableSize = sizeof(SYSTEM_CONFIGURATION);
    Status = Variable->GetVariable(
              Variable,
              L"SetupRecovery",
              &gEfiSetupVariableGuid,
              NULL,
              &VariableSize,
              &SystemConfiguration
              );
    ASSERT_EFI_ERROR (Status);
  }
  
  if (EFI_ERROR (Status)) {
    GGC = ((2 << 3) | 0x200);
    PciCfg16Write(EC_BASE, 0, 2, 0, 0x50, GGC);
    GGC = PciCfg16Read(EC_BASE, 0, 2, 0, 0x50);
    DEBUG((EFI_D_INFO , "GGC: 0x%08x GMSsize:0x%08x\n", GGC, (GGC & (BIT7|BIT6|BIT5|BIT4|BIT3))>>3));
  } else {
    if (SystemConfiguration.Igd == 1 && SystemConfiguration.PrimaryVideoAdaptor != 2) {
      GGC = (SystemConfiguration.IgdDvmt50PreAlloc << 3) |
            (SystemConfiguration.GTTSize == GTT_SIZE_1MB ? 0x100: 0x200);
      PciCfg16Write(EC_BASE, 0, 2, 0, 0x50, GGC);
      GGC = PciCfg16Read(EC_BASE, 0, 2, 0, 0x50);
      DEBUG((EFI_D_INFO , "GGC: 0x%08x GMSsize:0x%08x\n", GGC, (GGC & (BIT7|BIT6|BIT5|BIT4|BIT3))>>3));
    }
  }

  //
  // Initialize PlatformInfo HOB
  //
  Status = ReadPlatformIds(PeiServices, &PlatformInfo);
  ASSERT_EFI_ERROR (Status);

  //
  // 0 -> Disable , 1 -> Enable
  //
  if(SystemConfiguration.CfioPnpSettings == 1) {
    DEBUG((EFI_D_INFO, "CheckCfioPnpSettings: CFIO Pnp Settings Enabled\n"));
    PlatformInfo.CfioEnabled = 1;
  } else {
    DEBUG((EFI_D_INFO, "CheckCfioPnpSettings: CFIO Pnp Settings Disabled\n"));
    PlatformInfo.CfioEnabled = 0;
  }

  //
  // Build HOB for PlatformInfo
  //
  BuildGuidDataHob (
    &gEfiPlatformInfoGuid,
    &PlatformInfo,
    sizeof (EFI_PLATFORM_INFO_HOB)
    );


#ifdef FTPM_ENABLE
  Status = FtpmPolicyInit(PeiServices, &SystemConfiguration);
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "fTPM init failed.\n"));
  }
#endif


  //
  // Set the new boot mode for MRC
  //
#ifdef NOCS_S3_SUPPORT
  Status = UpdateBootMode (PeiServices);
  ASSERT_EFI_ERROR (Status);
#endif

  DEBUG((EFI_D_INFO, "Setup MMIO size ... \n\n"));

  //
  // Setup MMIO size
  //
  Status = (*PeiServices)->AllocatePool(
                             PeiServices,
                             sizeof (EFI_PEI_PPI_DESCRIPTOR),
                             (void **)&mVlvMmioPolicyPpiDesc
                             );
  ASSERT_EFI_ERROR (Status);
  Status = (*PeiServices)->AllocatePool(
                             PeiServices,
                             sizeof (VLV_MMIO_POLICY_PPI),
                             (void **)&mVlvMmioPolicyPpi
                             );
  ASSERT_EFI_ERROR (Status);
  (*PeiServices)->SetMem (
                    (VOID *)mVlvMmioPolicyPpi,
                    sizeof (VLV_MMIO_POLICY_PPI),
                    0
                    );
  mVlvMmioPolicyPpiDesc->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  mVlvMmioPolicyPpiDesc->Guid = &gVlvMmioPolicyPpiGuid;
  mVlvMmioPolicyPpiDesc->Ppi = mVlvMmioPolicyPpi;
  switch (SystemConfiguration.MmioSize) {
    case 0:      // 768MB
      mVlvMmioPolicyPpi->MmioSize = 0x300;
      break;
    case 1:      // 1GB
      mVlvMmioPolicyPpi->MmioSize = 0x400;
      break;
    case 2:      // 1.25GB
      mVlvMmioPolicyPpi->MmioSize = 0x500;
      break;
    case 3:      // 1.5GB
      mVlvMmioPolicyPpi->MmioSize = 0x600;
      break;
    case 4:      // 2GB
      mVlvMmioPolicyPpi->MmioSize = 0x800;
      break;
    default:
      mVlvMmioPolicyPpi->MmioSize = 0x800;
      break;
  }
  Status = (*PeiServices)->InstallPpi(
                             PeiServices,
                             mVlvMmioPolicyPpiDesc
                             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

EFI_STATUS
ReadPlatformIds (
  IN CONST EFI_PEI_SERVICES             **PeiServices,
  IN OUT EFI_PLATFORM_INFO_HOB          *PlatformInfoHob
  )
{
  {
    EFI_STATUS                      Status = EFI_SUCCESS;
    UINT8                           FabId = 0;
    UINTN                           DataSize;
    EFI_PLATFORM_INFO_HOB           TmpHob;
    EFI_PEI_READ_ONLY_VARIABLE2_PPI *PeiVar;

    Status = (**PeiServices).LocatePpi (
                               PeiServices,
                               &gEfiPeiReadOnlyVariable2PpiGuid,
                               0,
                               NULL,
                               (void **)&PeiVar
                               );
    ASSERT_EFI_ERROR (Status);

    DataSize = sizeof (EFI_PLATFORM_INFO_HOB);
    Status = PeiVar->GetVariable (
                       PeiVar,
                       L"PlatformInfo",
                       &gEfiVlv2VariableGuid,
                       NULL,
                       &DataSize,
                       &TmpHob
					   );

    if (Status == EFI_SUCCESS) {
      PlatformInfoHob->BoardId        = TmpHob.BoardId;
      PlatformInfoHob->MemCfgID       = TmpHob.MemCfgID;
      PlatformInfoHob->BoardRev       = TmpHob.BoardRev;
      PlatformInfoHob->PlatformFlavor = TmpHob.PlatformFlavor;
      return Status;
    }


    PlatformInfoHob->BoardId    = BOARD_ID_MINNOW2;
    DEBUG ((EFI_D_INFO,  "I'm Minnow2!\n"));

    PlatformInfoHob->MemCfgID   = 0;
    PlatformInfoHob->BoardRev   = FabId + 1;	// FabId = 0 means FAB1 (BoardRev = 1), FabId = 1 means FAB2 (BoardRev = 2)...
    PlatformInfoHob->PlatformFlavor = FlavorMobile;
  }

  return EFI_SUCCESS;
}

//
// Start::Alpine Valley platform
//
/**
  This routine reads SysCtl registers

  @param SmbusBase   SMBUS Base Address
  @param SlvAddr     Targeted Smbus Slave device address
  @param Operation   Which SMBus protocol will be used
  @param Offset      Offset of the register
  @param  Length     Number of bytes
  @param  Buffer     Buffer contains values read from registers

  @retval SUCCESS as passed
  @retval Others as failed

**/
EFI_STATUS
PeiSmbusExec (
  UINT16 SmbusBase,
  UINT8 SlvAddr,
  UINT8 Operation,
  UINT8 Offset,
  UINT8 *Length,
  UINT8 *Buffer
  )
{
  EFI_STATUS  Status=EFI_SUCCESS;
  UINT8       AuxcReg;
  UINT8       SmbusOperation = 0;
  UINT8       StsReg;
  UINT8       SlvAddrReg;
  UINT8       HostCmdReg;
  UINT8       BlockCount = 0;
  BOOLEAN     BufferTooSmall;
  UINT8       Index;
  UINT8       *CallBuffer;
  UINT8  	  RetryCount = BUS_TRIES;

  //
  // MrcSmbusExec supports byte and block read.
  // Only allow Byte or block access
  //
  if (!((*Length  == AV_SC_BYTE_LEN) || (*Length == AV_SC_BLOCK_LEN))) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // See if its ok to use the bus based upon INUSE_STS bit.
  //
  Status = AcquireBus (SmbusBase);
  ASSERT_EFI_ERROR(Status);

  CallBuffer = Buffer;

  //
  //SmbStatus Bits of interest
  //[6] = IUS (In Use Status)
  //[4] = FAIL
  //[3] = BERR (Bus Error = transaction collision)
  //[2] = DERR (Device Error = Illegal Command Field, Unclaimed Cycle, Host Device Timeout, CRC Error)
  //[1] = INTR (Successful completion of last command)
  //[0] = HOST BUSY
  //
  //
  // This is the main operation loop.  If the operation results in a Smbus
  // collision with another master on the bus, it attempts the requested
  // transaction again at least BUS_TRIES attempts.
  //
  while (RetryCount--) {
    //
    // Operation Specifics (pre-execution)
    //
    Status          = EFI_SUCCESS;
    SlvAddrReg      = SlvAddr;
    HostCmdReg      = Offset;
    AuxcReg         = 0;

	switch (Operation) {

	case SMBUS_WRITE_BYTE:
    IoWrite8 (SmbusBase+R_PCH_SMBUS_HD0, CallBuffer[0]);
		SmbusOperation = V_PCH_SMBUS_SMB_CMD_BYTE_DATA;
	break;

    case SMBUS_READ_BYTE:
      SmbusOperation = V_PCH_SMBUS_SMB_CMD_BYTE_DATA;
	  	SlvAddrReg |= B_PCH_SMBUS_RW_SEL_READ;
      if (*Length < 1) {
        Status = EFI_INVALID_PARAMETER;
      }
      	*Length = 1;
	break;

    case SMBUS_WRITE_BLOCK:
      SmbusOperation  = V_PCH_SMBUS_SMB_CMD_BLOCK;
      IoWrite8 (SmbusBase+R_PCH_SMBUS_HD0, *(UINT8 *) Length);
     	BlockCount = (UINT8) (*Length);
     	if ((*Length < 1) || (*Length > 32)) {
        Status = EFI_INVALID_PARAMETER;
        break;
    	}
      	AuxcReg |= B_PCH_SMBUS_E32B;
	break;

    case SMBUS_READ_BLOCK:
      SmbusOperation = V_PCH_SMBUS_SMB_CMD_BLOCK;
     	SlvAddrReg |= B_PCH_SMBUS_RW_SEL_READ;
     	if ((*Length < 1) || (*Length > 32)) {
        Status = EFI_INVALID_PARAMETER;
        break;
     	}
      	AuxcReg |= B_PCH_SMBUS_E32B;
	break;

    default:
      	Status = EFI_INVALID_PARAMETER;
	break;
    }

    //
    // Set Auxiliary Control register
    //
    IoWrite8 (SmbusBase+R_PCH_SMBUS_AUXC, AuxcReg);

    //
    // Reset the pointer of the internal buffer
    //
    IoRead8 (SmbusBase+R_PCH_SMBUS_HCTL);

    //
    // Now that the 32 byte buffer is turned on, we can write th block data
    // into it
    //
    if (Operation == SMBUS_WRITE_BLOCK) {
      for (Index = 0; Index < BlockCount; Index++) {
        //
        // Write next byte
        //
        IoWrite8 (SmbusBase+R_PCH_SMBUS_HBD, CallBuffer[Index]);
      }
    }

    //
    // Set SMBus slave address for the device to read
    //
    IoWrite8(SmbusBase+R_PCH_SMBUS_TSA, SlvAddrReg);

    //
    //
    // Set Command register for the offset to read
    //
    IoWrite8(SmbusBase+R_PCH_SMBUS_HCMD, HostCmdReg );

    //
    // Set Control Register to Set "operation command" protocol and start bit
    //
    IoWrite8(SmbusBase+R_PCH_SMBUS_HCTL, (UINT8) (SmbusOperation + B_PCH_SMBUS_START));

    //
    // Wait for IO to complete
    //
	do { StsReg = (UINT8) IoRead8(SmbusBase+0); } while ((StsReg & (BIT4|BIT3|BIT2|BIT1)) == 0);

	  if (StsReg & B_PCH_SMBUS_DERR) {
      Status = EFI_DEVICE_ERROR;
      break;
    } else if (StsReg & B_PCH_SMBUS_BERR) {
      //
      // Clear the Bus Error for another try
      //
      Status = EFI_DEVICE_ERROR;
      IoWrite8(SmbusBase+R_PCH_SMBUS_HSTS, B_PCH_SMBUS_BERR);

      //
      // Clear Status Registers
      //
      IoWrite8(SmbusBase+R_PCH_SMBUS_HSTS, B_PCH_SMBUS_HSTS_ALL);
      IoWrite8(SmbusBase+R_PCH_SMBUS_AUXS, B_PCH_SMBUS_CRCE);

      continue;
    }

    //
    // successfull completion
    // Operation Specifics (post-execution)
    //
    switch (Operation) {

    case SMBUS_READ_BYTE:
      CallBuffer[0] = (UINT8)(IoRead8 (SmbusBase+R_PCH_SMBUS_HD0));
      break;

    case SMBUS_WRITE_BLOCK:
      IoWrite8(SmbusBase+R_PCH_SMBUS_HSTS, B_PCH_SMBUS_BYTE_DONE_STS);
      break;

    case SMBUS_READ_BLOCK:
      BufferTooSmall = FALSE;

      //
      // Find out how many bytes will be in the block
      //
      BlockCount = (UINT8)(IoRead8 (SmbusBase+R_PCH_SMBUS_HD0));
      if (*Length < BlockCount) {
        BufferTooSmall = TRUE;
      } else {
        for (Index = 0; Index < BlockCount; Index++) {
          //
          // Read the byte
          //
          CallBuffer[Index] = (UINT8)IoRead8 (SmbusBase+R_PCH_SMBUS_HBD);
        }
      }

      *Length = BlockCount;
      if (BufferTooSmall) {
        Status = EFI_BUFFER_TOO_SMALL;
      }
      break;

    default:
      break;
    };

    if ((StsReg & B_PCH_SMBUS_BERR) && (Status == EFI_SUCCESS)) {
      //
      // Clear the Bus Error for another try
      //
      Status = EFI_DEVICE_ERROR;
      IoWrite8(SmbusBase+R_PCH_SMBUS_HSTS, B_PCH_SMBUS_BERR);

      continue;
    } else {
      break;
    }
  }

  //
  // Clear Status Registers and exit
  //
  IoWrite8(SmbusBase+R_PCH_SMBUS_HSTS, B_PCH_SMBUS_HSTS_ALL);
  IoWrite8(SmbusBase+R_PCH_SMBUS_AUXS, B_PCH_SMBUS_CRCE);
  IoWrite8(SmbusBase+R_PCH_SMBUS_AUXC, 0);
  return Status;
}
//
// End::Alpine Valley platform
//

