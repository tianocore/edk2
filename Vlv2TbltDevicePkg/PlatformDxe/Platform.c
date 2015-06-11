/** @file

  Copyright (c) 2004  - 2015, Intel Corporation. All rights reserved.<BR>
                                                                                   

  This program and the accompanying materials are licensed and made available under

  the terms and conditions of the BSD License that accompanies this distribution.  

  The full text of the license may be found at                                     

  http://opensource.org/licenses/bsd-license.php.                                  

                                                                                   

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            

  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    

                                                                                   


Module Name:


  Platform.c

Abstract:

  Platform Initialization Driver.


--*/

#include "PlatformDxe.h"
#include "Platform.h"
#include "PchCommonDefinitions.h"
#include <Protocol/UsbPolicy.h>
#include <Protocol/PchPlatformPolicy.h>
#include <Protocol/TpmMp.h>
#include <Protocol/CpuIo2.h>
#include <Library/S3BootScriptLib.h>
#include <Guid/PciLanInfo.h>
#include <Guid/ItkData.h>
#include <Library/PciLib.h>
#include <PlatformBootMode.h>
#include <Guid/EventGroup.h>
#include <Guid/Vlv2Variable.h>
#include <Protocol/GlobalNvsArea.h>
#include <Protocol/IgdOpRegion.h>
#include <Library/PcdLib.h>
#include <Protocol/VariableLock.h>


//
// VLV2 GPIO GROUP OFFSET
//
#define GPIO_SCORE_OFFSET	0x0000
#define GPIO_NCORE_OFFSET	0x1000
#define GPIO_SSUS_OFFSET	0x2000

typedef struct {
  UINT32 offset;
  UINT32 val;
} CFIO_PNP_INIT;

GPIO_CONF_PAD_INIT mTB_BL_GpioInitData_SC_TRI_Exit_boot_Service[] =
{
//              Pad Name          GPIO Number     Used As   GPO Default  Function#     INT Capable   Interrupt Type   PULL H/L    MMIO Offset
  GPIO_INIT_ITEM("LPC_CLKOUT0       GPIOC_47 "     ,TRISTS   ,NA           ,F0           ,             ,                ,NONE       ,0x47),
  GPIO_INIT_ITEM("LPC_CLKOUT1       GPIOC_48 "     ,TRISTS   ,NA           ,F0           ,             ,                ,NONE       ,0x41),
};


EFI_GUID mSystemHiiExportDatabase = EFI_HII_EXPORT_DATABASE_GUID;
EFI_GUID mPlatformDriverGuid = EFI_PLATFORM_DRIVER_GUID;
SYSTEM_CONFIGURATION  mSystemConfiguration;
SYSTEM_PASSWORDS      mSystemPassword;
EFI_HANDLE            mImageHandle;
BOOLEAN               mMfgMode = FALSE;
VOID                  *mDxePlatformStringPack;
UINT32                mPlatformBootMode = PLATFORM_NORMAL_MODE;
extern CHAR16 gItkDataVarName[];


EFI_PLATFORM_INFO_HOB      mPlatformInfo;
EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *mPciRootBridgeIo;
EFI_EVENT  mReadyToBootEvent;

UINT8 mSmbusRsvdAddresses[] = PLATFORM_SMBUS_RSVD_ADDRESSES;
UINT8 mNumberSmbusAddress = sizeof( mSmbusRsvdAddresses ) / sizeof( mSmbusRsvdAddresses[0] );
UINT32 mSubsystemVidDid;
UINT32 mSubsystemAudioVidDid;

UINTN   mPciLanCount = 0;
VOID    *mPciLanInfo = NULL;
UINTN   SpiBase;

static EFI_SPEAKER_IF_PROTOCOL mSpeakerInterface = {
  ProgramToneFrequency,
  GenerateBeepTone
};

EFI_USB_POLICY_PROTOCOL         mUsbPolicyData = {0};


CFIO_PNP_INIT mTB_BL_GpioInitData_SC_TRI_S0ix_Exit_boot_Service[] =
{
  {0x410 ,0x20038e10},  //vlv.gpio.gpscore.cfio_regs_pad_lpc_clkout1_pconf0
  {0x470 ,0x20038e10},  //vlv.gpio.gpscore.cfio_regs_pad_lpc_clkout0_pconf0
  {0x560 ,0x20038e10},  //vlv.gpio.gpscore.cfio_regs_pad_ilb_serirq_pconf0
  {0x450 ,0x20038e10},  //vlv.gpio.gpscore.cfio_regs_pad_lpc_frameb_pconf0
  {0x480 ,0x20038e10},  //vlv.gpio.gpscore.cfio_regs_pad_lpc_clkrunb_pconf0
  {0x420 ,0x20038e10},  //vlv.gpio.gpscore.cfio_regs_pad_lpc_ad3_pconf0
  {0x430 ,0x20038e10},  //vlv.gpio.gpscore.cfio_regs_pad_lpc_ad2_pconf0
  {0x440 ,0x20038e10},  //vlv.gpio.gpscore.cfio_regs_pad_lpc_ad1_pconf0
  {0x460 ,0x20038e10},  //vlv.gpio.gpscore.cfio_regs_pad_lpc_ad0_pconf0
  {0x418 ,0x00000006},  //vlv.gpio.gpscore.cfio_regs_pad_lpc_clkout1_pad_val
  {0x478 ,0x00000006},  //vlv.gpio.gpscore.cfio_regs_pad_lpc_clkout0_pad_val
  {0x568 ,0x00000006},  //vlv.gpio.gpscore.cfio_regs_pad_ilb_serirq_pad_val
  {0x458 ,0x00000006},  //vlv.gpio.gpscore.cfio_regs_pad_lpc_frameb_pad_val
  {0x488 ,0x00000006},  //vlv.gpio.gpscore.cfio_regs_pad_lpc_clkrunb_pad_val
  {0x428 ,0x00000006},  //vlv.gpio.gpscore.cfio_regs_pad_lpc_ad3_pad_val
  {0x438 ,0x00000006},  //vlv.gpio.gpscore.cfio_regs_pad_lpc_ad2_pad_val
  {0x448 ,0x00000006},  //vlv.gpio.gpscore.cfio_regs_pad_lpc_ad1_pad_val
  {0x468 ,0x00000006},  //vlv.gpio.gpscore.cfio_regs_pad_lpc_ad0_pad_val
};

VOID
EfiOrMem (
  IN VOID   *Destination,
  IN VOID   *Source,
  IN UINTN  Length
  );

#if defined(FIRMWARE_ID_BACKWARD_COMPATIBLE) && (FIRMWARE_ID_BACKWARD_COMPATIBLE != 0)
STATIC
VOID
InitFirmwareId();
#endif


VOID
InitializeClockRouting(
  );

VOID
InitializeSlotInfo (
  );

#if defined(SENSOR_INFO_VAR_SUPPORT) && SENSOR_INFO_VAR_SUPPORT != 0
VOID
InitializeSensorInfoVariable (
  );
#endif

VOID
InitTcoReset (
  );

VOID
InitExI ();

VOID
InitItk();

VOID
InitPlatformBootMode();

VOID
InitMfgAndConfigModeStateVar();

VOID
InitPchPlatformPolicy (
  IN EFI_PLATFORM_INFO_HOB      *PlatformInfo
  );

VOID
InitVlvPlatformPolicy (
  );

VOID
InitSioPlatformPolicy(
  );

VOID
PchInitBeforeBoot(
  );

VOID
UpdateDVMTSetup(
  );

VOID
InitPlatformUsbPolicy (
  VOID
  );

VOID
InitRC6Policy(
  VOID
  );


EFI_STATUS
EFIAPI
SaveSetupRecoveryVar(
  VOID
  )
{
  EFI_STATUS                   Status = EFI_SUCCESS;
  UINTN                        SizeOfNvStore = 0;
  UINTN                        SizeOfSetupVar = 0;
  SYSTEM_CONFIGURATION         *SetupData = NULL;
  SYSTEM_CONFIGURATION         *RecoveryNvData = NULL;
  EDKII_VARIABLE_LOCK_PROTOCOL *VariableLock = NULL;


  DEBUG ((EFI_D_INFO, "SaveSetupRecoveryVar() Entry \n"));
  SizeOfNvStore = sizeof(SYSTEM_CONFIGURATION);
  RecoveryNvData = AllocateZeroPool (sizeof(SYSTEM_CONFIGURATION));
  if (NULL == RecoveryNvData) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit; 
  }
  
  Status = gRT->GetVariable(
                L"SetupRecovery",
                &gEfiNormalSetupGuid,
                NULL,
                &SizeOfNvStore,
                RecoveryNvData
                );
  
  if (EFI_ERROR (Status)) {
    // Don't find the "SetupRecovery" variable.
    // have to copy "Setup" variable to "SetupRecovery" variable.
    SetupData = AllocateZeroPool (sizeof(SYSTEM_CONFIGURATION));
    if (NULL == SetupData) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;      
    }
    SizeOfSetupVar = sizeof(SYSTEM_CONFIGURATION);
    Status = gRT->GetVariable(
                    NORMAL_SETUP_NAME,
                    &gEfiNormalSetupGuid,
                    NULL,
                    &SizeOfSetupVar,
                    SetupData
                    );
    ASSERT_EFI_ERROR (Status);
    
    Status = gRT->SetVariable (
                    L"SetupRecovery",
                    &gEfiNormalSetupGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof(SYSTEM_CONFIGURATION),
                    SetupData
                    );
    ASSERT_EFI_ERROR (Status);

    Status = gBS->LocateProtocol (&gEdkiiVariableLockProtocolGuid, NULL, (VOID **) &VariableLock);
      if (!EFI_ERROR (Status)) {
        Status = VariableLock->RequestToLock (VariableLock, L"SetupRecovery", &gEfiNormalSetupGuid);
        ASSERT_EFI_ERROR (Status);
    }
    
  }

Exit:
  if (RecoveryNvData)
    FreePool (RecoveryNvData);
  if (SetupData)
    FreePool (SetupData);
  
  return Status;
    
}


VOID
TristateLpcGpioConfig (
  IN UINT32             Gpio_Mmio_Offset,
  IN UINT32             Gpio_Pin_Num,
  GPIO_CONF_PAD_INIT*   Gpio_Conf_Data
  )

{
  UINT32    index;
  UINT32    mmio_conf0;
  UINT32    mmio_padval;
  PAD_CONF0 conf0_val;
  PAD_VAL   pad_val;

  //
  // GPIO WELL -- Memory base registers
  //

  //
  // A0 BIOS Spec doesn't mention it although X0 does. comment out now.
  // GPIO write 0x01001002 to IOBASE + Gpio_Mmio_Offset + 0x0900
  //

  for(index=0; index < Gpio_Pin_Num; index++)
  {
    //
    // Calculate the MMIO Address for specific GPIO pin CONF0 register pointed by index.
    //
    mmio_conf0 = IO_BASE_ADDRESS + Gpio_Mmio_Offset + R_PCH_CFIO_PAD_CONF0 + Gpio_Conf_Data[index].offset * 16;
    mmio_padval= IO_BASE_ADDRESS + Gpio_Mmio_Offset + R_PCH_CFIO_PAD_VAL   + Gpio_Conf_Data[index].offset * 16;

#ifdef EFI_DEBUG
    DEBUG ((EFI_D_INFO, "%s, ", Gpio_Conf_Data[index].pad_name));

#endif
    DEBUG ((EFI_D_INFO, "Usage = %d, Func# = %d, IntType = %d, Pull Up/Down = %d, MMIO Base = 0x%08x, ",
      Gpio_Conf_Data[index].usage,
      Gpio_Conf_Data[index].func,
      Gpio_Conf_Data[index].int_type,
      Gpio_Conf_Data[index].pull,
      mmio_conf0));

    //
    // Step 1: PadVal Programming
    //
    pad_val.dw = MmioRead32(mmio_padval);

    //
    // Config PAD_VAL only for GPIO (Non-Native) Pin
    //
    if(Native != Gpio_Conf_Data[index].usage)
    {
      pad_val.dw &= ~0x6; // Clear bits 1:2
      pad_val.dw |= (Gpio_Conf_Data[index].usage & 0x6);  // Set bits 1:2 according to PadVal

        //
        // set GPO default value
        //
        if(Gpio_Conf_Data[index].usage == GPO && Gpio_Conf_Data[index].gpod4 != NA)
        {
        pad_val.r.pad_val = Gpio_Conf_Data[index].gpod4;
        }
    }


    DEBUG ((EFI_D_INFO, "Set PAD_VAL = 0x%08x, ", pad_val.dw));

    MmioWrite32(mmio_padval, pad_val.dw);

    //
    // Step 2: CONF0 Programming
    // Read GPIO default CONF0 value, which is assumed to be default value after reset.
    //
    conf0_val.dw = MmioRead32(mmio_conf0);

    //
    // Set Function #
    //
    conf0_val.r.Func_Pin_Mux = Gpio_Conf_Data[index].func;

    if(GPO == Gpio_Conf_Data[index].usage)
    {
      //
      // If used as GPO, then internal pull need to be disabled
      //
      conf0_val.r.Pull_assign = 0;  // Non-pull
    }
    else
    {
      //
      // Set PullUp / PullDown
      //
      if(P_20K_H == Gpio_Conf_Data[index].pull)
      {
        conf0_val.r.Pull_assign = 0x1;  // PullUp
        conf0_val.r.Pull_strength = 0x2;// 20K
      }
      else if(P_20K_L == Gpio_Conf_Data[index].pull)
      {
        conf0_val.r.Pull_assign = 0x2;  // PullDown
        conf0_val.r.Pull_strength = 0x2;// 20K
      }
      else if(P_NONE == Gpio_Conf_Data[index].pull)
      {
        conf0_val.r.Pull_assign = 0;	// Non-pull
      }
      else
      {
        ASSERT(FALSE);  // Invalid value
      }
    }

    //
    // Set INT Trigger Type
    //
    conf0_val.dw &= ~0x0f000000;  // Clear bits 27:24

    //
    // Set INT Trigger Type
    //
    if(TRIG_ == Gpio_Conf_Data[index].int_type)
    {
      //
      // Interrupt not capable, clear bits 27:24
      //
    }
    else
    {
      conf0_val.dw |= (Gpio_Conf_Data[index].int_type & 0x0f)<<24;
    }

    DEBUG ((EFI_D_INFO, "Set CONF0 = 0x%08x\n", conf0_val.dw));

    //
    // Write back the targeted GPIO config value according to platform (board) GPIO setting
    //
    MmioWrite32 (mmio_conf0, conf0_val.dw);
  }

  // A0 BIOS Spec doesn't mention it although X0 does. comment out now.
  // GPIO SCORE write 0x01001002 to IOBASE + 0x0900
  //
}

VOID
EFIAPI
SpiBiosProtectionFunction(
  EFI_EVENT Event,
  VOID      *Context
  )
{

  UINTN                             mPciD31F0RegBase;
  UINTN                             BiosFlaLower0;
  UINTN                             BiosFlaLimit0;
  UINTN                             BiosFlaLower1;
  UINTN                             BiosFlaLimit1;  
  

  BiosFlaLower0 = PcdGet32(PcdFlashMicroCodeAddress)-PcdGet32(PcdFlashAreaBaseAddress);
  BiosFlaLimit0 = PcdGet32(PcdFlashMicroCodeSize)-1;  
  #ifdef MINNOW2_FSP_BUILD
  BiosFlaLower1 = PcdGet32(PcdFlashFvFspBase)-PcdGet32(PcdFlashAreaBaseAddress);
  BiosFlaLimit1 = (PcdGet32(PcdFlashFvRecoveryBase)-PcdGet32(PcdFlashFvFspBase)+PcdGet32(PcdFlashFvRecoverySize))-1;
  #else
  BiosFlaLower1 = PcdGet32(PcdFlashFvMainBase)-PcdGet32(PcdFlashAreaBaseAddress);
  BiosFlaLimit1 = (PcdGet32(PcdFlashFvRecoveryBase)-PcdGet32(PcdFlashFvMainBase)+PcdGet32(PcdFlashFvRecoverySize))-1;
  #endif

  
  mPciD31F0RegBase = MmPciAddress (0,
                         DEFAULT_PCI_BUS_NUMBER_PCH,
                         PCI_DEVICE_NUMBER_PCH_LPC,
                         PCI_FUNCTION_NUMBER_PCH_LPC,
                         0
                       );
  SpiBase          = MmioRead32(mPciD31F0RegBase + R_PCH_LPC_SPI_BASE) & B_PCH_LPC_SPI_BASE_BAR;

  //
  //Set SMM_BWP, WPD and LE bit
  //
  MmioOr32 ((UINTN) (SpiBase + R_PCH_SPI_BCR), (UINT8) B_PCH_SPI_BCR_SMM_BWP);
  MmioAnd32 ((UINTN) (SpiBase + R_PCH_SPI_BCR), (UINT8)(~B_PCH_SPI_BCR_BIOSWE));
  MmioOr32 ((UINTN) (SpiBase + R_PCH_SPI_BCR), (UINT8) B_PCH_SPI_BCR_BLE);

  //
  //First check if FLOCKDN or PR0FLOCKDN is set. No action if either of them set already.
  //
  if( (MmioRead16(SpiBase + R_PCH_SPI_HSFS) & B_PCH_SPI_HSFS_FLOCKDN) != 0 ||
      (MmioRead32(SpiBase + R_PCH_SPI_IND_LOCK)& B_PCH_SPI_IND_LOCK_PR0) != 0) {
    //
    //Already locked. we could take no action here
    //
    DEBUG((EFI_D_INFO, "PR0 already locked down. Stop configuring PR0.\n"));
    return;
  }

  //
  //Set PR0
  //
  MmioOr32((UINTN)(SpiBase + R_PCH_SPI_PR0),
    B_PCH_SPI_PR0_RPE|B_PCH_SPI_PR0_WPE|\
    (B_PCH_SPI_PR0_PRB_MASK&(BiosFlaLower0>>12))|(B_PCH_SPI_PR0_PRL_MASK&(BiosFlaLimit0>>12)<<16));

  //
  //Lock down PR0
  //
  MmioOr16 ((UINTN) (SpiBase + R_PCH_SPI_HSFS), (UINT16) (B_PCH_SPI_HSFS_FLOCKDN));

  //
  // Verify if it's really locked.
  //
  if ((MmioRead16 (SpiBase + R_PCH_SPI_HSFS) & B_PCH_SPI_HSFS_FLOCKDN) == 0) {
    DEBUG((EFI_D_ERROR, "Failed to lock down PR0.\n"));
  }

  //
  //Set PR1
  //

  MmioOr32((UINTN)(SpiBase + R_PCH_SPI_PR1),
    B_PCH_SPI_PR1_RPE|B_PCH_SPI_PR1_WPE|\
    (B_PCH_SPI_PR1_PRB_MASK&(BiosFlaLower1>>12))|(B_PCH_SPI_PR1_PRL_MASK&(BiosFlaLimit1>>12)<<16));

  //
  //Lock down PR1
  //
  MmioOr16 ((UINTN) (SpiBase + R_PCH_SPI_HSFS), (UINT16) (B_PCH_SPI_HSFS_FLOCKDN));

  //
  // Verify if it's really locked.
  //
  if ((MmioRead16 (SpiBase + R_PCH_SPI_HSFS) & B_PCH_SPI_HSFS_FLOCKDN) == 0) {
    DEBUG((EFI_D_ERROR, "Failed to lock down PR1.\n"));
  }
  return;

}

VOID
EFIAPI
InitPciDevPME (
  EFI_EVENT  Event,
  VOID       *Context
  )
{
  UINTN                  VarSize;
  EFI_STATUS             Status;

  VarSize = sizeof(SYSTEM_CONFIGURATION);
  Status = gRT->GetVariable(
                  NORMAL_SETUP_NAME,
                  &gEfiNormalSetupGuid,
                  NULL,
                  &VarSize,
                  &mSystemConfiguration
                  );

  //
  //Program HDA PME_EN
  //
  PchAzaliaPciCfg32Or (R_PCH_HDA_PCS, B_PCH_HDA_PCS_PMEE);

  //
  //Program SATA PME_EN
  //
  PchSataPciCfg32Or (R_PCH_SATA_PMCS, B_PCH_SATA_PMCS_PMEE);

  DEBUG ((EFI_D_INFO, "InitPciDevPME mSystemConfiguration.EhciPllCfgEnable = 0x%x \n",mSystemConfiguration.EhciPllCfgEnable));
 if (mSystemConfiguration.EhciPllCfgEnable != 1) {
  //
  //Program EHCI PME_EN
  //
  PchMmPci32Or (
    0,
    0,
    PCI_DEVICE_NUMBER_PCH_USB,
    PCI_FUNCTION_NUMBER_PCH_EHCI,
    R_PCH_EHCI_PWR_CNTL_STS,
    B_PCH_EHCI_PWR_CNTL_STS_PME_EN
    );
 }
   {
     UINTN                 EhciPciMmBase;
     UINT32                Buffer32 = 0;

    EhciPciMmBase = MmPciAddress (0,
                      0,
                      PCI_DEVICE_NUMBER_PCH_USB,
                      PCI_FUNCTION_NUMBER_PCH_EHCI,
                      0
                    );
    DEBUG ((EFI_D_INFO, "ConfigureAdditionalPm() EhciPciMmBase = 0x%x \n",EhciPciMmBase));
    Buffer32 = MmioRead32(EhciPciMmBase + R_PCH_EHCI_PWR_CNTL_STS);
    DEBUG ((EFI_D_INFO, "ConfigureAdditionalPm() R_PCH_EHCI_PWR_CNTL_STS = 0x%x \n",Buffer32));
  }
}

#if defined SUPPORT_LVDS_DISPLAY && SUPPORT_LVDS_DISPLAY

#endif


EFI_STATUS
EFIAPI
TristateLpcGpioS0i3Config (
  UINT32             Gpio_Mmio_Offset,
  UINT32             Gpio_Pin_Num,
  CFIO_PNP_INIT*   Gpio_Conf_Data
  )
{

  UINT32	  index;
  UINT32	  mmio_reg;
  UINT32	  mmio_val;

    DEBUG ((DEBUG_INFO, "TristateLpcGpioS0i3Config\n"));

    for(index=0; index < Gpio_Pin_Num; index++)
    {
      mmio_reg = IO_BASE_ADDRESS + Gpio_Mmio_Offset + Gpio_Conf_Data[index].offset;

      MmioWrite32(mmio_reg, Gpio_Conf_Data[index].val);
      mmio_val = 0;
      mmio_val = MmioRead32(mmio_reg);

      DEBUG ((EFI_D_INFO, "Set MMIO=0x%08x  PAD_VAL = 0x%08x,\n", mmio_reg, mmio_val));
    }

     return EFI_SUCCESS;
}


EFI_BOOT_SCRIPT_SAVE_PROTOCOL *mBootScriptSave;

/**
  Event Notification during exit boot service to enabel ACPI mode

   Disable SW SMI Timer, SMI from USB & Intel Specific USB 2

   Clear all ACPI event status and disable all ACPI events
   Disable PM sources except power button
   Clear status bits

   Guarantee day-of-month alarm is invalid (ACPI 5.0 Section 4.8.2.4 "Real Time Clock Alarm")

   Update EC to disable SMI and enable SCI

   Enable SCI

   Enable PME_B0_EN in GPE0a_EN

  @param Event  - EFI Event Handle
  @param Context - Pointer to Notify Context

  @retval  Nothing

**/
VOID
EFIAPI
EnableAcpiCallback (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  UINT32  RegData32;
  UINT16  Pm1Cnt;
  UINT16  AcpiBase;
  UINT32  Gpe0aEn;

  AcpiBase = MmioRead16 (
               PchPciDeviceMmBase (DEFAULT_PCI_BUS_NUMBER_PCH,
               PCI_DEVICE_NUMBER_PCH_LPC,
               PCI_FUNCTION_NUMBER_PCH_LPC) + R_PCH_LPC_ACPI_BASE
               ) & B_PCH_LPC_ACPI_BASE_BAR;

  DEBUG ((EFI_D_INFO, "EnableAcpiCallback: AcpiBase = %x\n", AcpiBase));

  //
  // Disable SW SMI Timer, SMI from USB & Intel Specific USB 2
  //
  RegData32 = IoRead32(AcpiBase + R_PCH_SMI_EN);
  RegData32 &= ~(B_PCH_SMI_EN_SWSMI_TMR | B_PCH_SMI_EN_LEGACY_USB2 | B_PCH_SMI_EN_INTEL_USB2);
  IoWrite32(AcpiBase + R_PCH_SMI_EN, RegData32);

  RegData32 = IoRead32(AcpiBase + R_PCH_SMI_STS);
  RegData32 |= B_PCH_SMI_STS_SWSMI_TMR;
  IoWrite32(AcpiBase + R_PCH_SMI_STS, RegData32);

  //
  // Disable PM sources except power button
  // power button is enabled only for PCAT. Disabled it on Tablet platform
  //

  IoWrite16(AcpiBase + R_PCH_ACPI_PM1_EN, B_PCH_ACPI_PM1_EN_PWRBTN);
  IoWrite16(AcpiBase + R_PCH_ACPI_PM1_STS, 0xffff);

  //
  // Guarantee day-of-month alarm is invalid (ACPI 5.0 Section 4.8.2.4 "Real Time Clock Alarm")
  // Clear Status D reg VM bit, Date of month Alarm to make Data in CMOS RAM is no longer Valid
  //
  IoWrite8 (PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_REGISTER_D);
  IoWrite8 (PCAT_RTC_DATA_REGISTER, 0x0);

  RegData32 = IoRead32(AcpiBase + R_PCH_ALT_GP_SMI_EN);
  RegData32 &= ~(BIT7);
  IoWrite32((AcpiBase + R_PCH_ALT_GP_SMI_EN), RegData32);

  //
  // Enable SCI
  //
  Pm1Cnt = IoRead16(AcpiBase + R_PCH_ACPI_PM1_CNT);
  Pm1Cnt |= B_PCH_ACPI_PM1_CNT_SCI_EN;
  IoWrite16(AcpiBase + R_PCH_ACPI_PM1_CNT, Pm1Cnt);

  IoWrite8(0x80, 0xA0);	//SW_SMI_ACPI_ENABLE

  //
  // Enable PME_B0_EN in GPE0a_EN
  // Caution: Enable PME_B0_EN must be placed after enabling SCI.
  // Otherwise, USB PME could not be handled as SMI event since no handler is there.
  //
  Gpe0aEn = IoRead32 (AcpiBase + R_PCH_ACPI_GPE0a_EN);
  Gpe0aEn |= B_PCH_ACPI_GPE0a_EN_PME_B0;
  IoWrite32(AcpiBase + R_PCH_ACPI_GPE0a_EN, Gpe0aEn);

}

/**

  Routine Description:

  This is the standard EFI driver point for the Driver. This
  driver is responsible for setting up any platform specific policy or
  initialization information.

  @param ImageHandle    Handle for the image of this driver.
  @param SystemTable    Pointer to the EFI System Table.

  @retval EFI_SUCCESS   Policy decisions set.

**/
EFI_STATUS
EFIAPI
InitializePlatform (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                          Status;
  UINTN                               VarSize;
  EFI_HANDLE                          Handle = NULL;
  EFI_EVENT                           mEfiExitBootServicesEvent;
  EFI_EVENT                           RtcEvent;
  VOID                                *RtcCallbackReg = NULL;
  
  mImageHandle = ImageHandle;

  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiSpeakerInterfaceProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mSpeakerInterface
                  );

  Status = gBS->LocateProtocol (
                  &gEfiPciRootBridgeIoProtocolGuid,
                  NULL,
                  (VOID **) &mPciRootBridgeIo
                  );
  ASSERT_EFI_ERROR (Status);

  VarSize = sizeof(EFI_PLATFORM_INFO_HOB);
  Status = gRT->GetVariable(
                  L"PlatformInfo",
                  &gEfiVlv2VariableGuid,
                  NULL,
                  &VarSize,
                  &mPlatformInfo
                  );

  //
  // Initialize Product Board ID variable
  //
  InitMfgAndConfigModeStateVar();
  InitPlatformBootMode();

  //
  // Install Observable protocol
  //
  InitializeObservableProtocol();

  Status = SaveSetupRecoveryVar();
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "InitializePlatform() Save SetupRecovery variable failed \n"));
  }

  VarSize = sizeof(SYSTEM_CONFIGURATION);
  Status = gRT->GetVariable(
                  NORMAL_SETUP_NAME,
                  &gEfiNormalSetupGuid,
                  NULL,
                  &VarSize,
                  &mSystemConfiguration
                  );
  if (EFI_ERROR (Status) || VarSize != sizeof(SYSTEM_CONFIGURATION)) {
    //The setup variable is corrupted
    VarSize = sizeof(SYSTEM_CONFIGURATION);
    Status = gRT->GetVariable(
              L"SetupRecovery",
              &gEfiNormalSetupGuid,
              NULL,
              &VarSize,
              &mSystemConfiguration
              );
    ASSERT_EFI_ERROR (Status);
    Status = gRT->SetVariable (
                    NORMAL_SETUP_NAME,
                    &gEfiNormalSetupGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof(SYSTEM_CONFIGURATION),
                    &mSystemConfiguration
                    );    
  }
    
  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             ReadyToBootFunction,
             NULL,
             &mReadyToBootEvent
             );

  //
  // Create a ReadyToBoot Event to run the PME init process
  //
  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             InitPciDevPME,
             NULL,
             &mReadyToBootEvent
             );
  //
  // Create a ReadyToBoot Event to run enable PR0/PR1 and lock down,unlock variable region
  //
  if(mSystemConfiguration.SpiRwProtect==1) {
    Status = EfiCreateEventReadyToBootEx (
               TPL_CALLBACK,
               SpiBiosProtectionFunction,
               NULL,
               &mReadyToBootEvent
               );
  }

  ReportStatusCodeEx (
    EFI_PROGRESS_CODE,
    EFI_COMPUTING_UNIT_CHIPSET | EFI_CU_PLATFORM_DXE_STEP1,
    0,
    &gEfiCallerIdGuid,
    NULL,
    NULL,
    0
    );

#if defined(SENSOR_INFO_VAR_SUPPORT) && SENSOR_INFO_VAR_SUPPORT != 0
  //
  // Initialize Sensor Info variable
  //
  InitializeSensorInfoVariable();
#endif
  InitPchPlatformPolicy(&mPlatformInfo);
  InitVlvPlatformPolicy();

  //
  //  Add usb policy
  //
  InitPlatformUsbPolicy();
  InitSioPlatformPolicy();
  InitializeClockRouting();
  InitializeSlotInfo();
  InitTcoReset();

  //
  //Init ExI
  //
  InitExI();

  ReportStatusCodeEx (
    EFI_PROGRESS_CODE,
    EFI_COMPUTING_UNIT_CHIPSET | EFI_CU_PLATFORM_DXE_STEP2,
    0,
    &gEfiCallerIdGuid,
    NULL,
    NULL,
    0
    );

  //
  // Install PCI Bus Driver Hook
  //
  PciBusDriverHook();

  InitItk();

  ReportStatusCodeEx (
    EFI_PROGRESS_CODE,
    EFI_COMPUTING_UNIT_CHIPSET | EFI_CU_PLATFORM_DXE_STEP3,
    0,
    &gEfiCallerIdGuid,
    NULL,
    NULL,
    0
    );


  //
  // Initialize Password States and Callbacks
  //
  PchInitBeforeBoot();

#if defined SUPPORT_LVDS_DISPLAY && SUPPORT_LVDS_DISPLAY

#endif

#if defined(FIRMWARE_ID_BACKWARD_COMPATIBLE) && (FIRMWARE_ID_BACKWARD_COMPATIBLE != 0)
  //
  // Re-write Firmware ID if it is changed
  //
  InitFirmwareId();
#endif

  ReportStatusCodeEx (
    EFI_PROGRESS_CODE,
    EFI_COMPUTING_UNIT_CHIPSET | EFI_CU_PLATFORM_DXE_STEP4,
    0,
    &gEfiCallerIdGuid,
    NULL,
    NULL,
    0
    );


  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  EnableAcpiCallback,
                  NULL,
                  &gEfiEventExitBootServicesGuid,
                  &mEfiExitBootServicesEvent
                  );

  //
  // Adjust RTC deafult time to be BIOS-built time.
  //
  Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    AdjustDefaultRtcTimeCallback,
                    NULL,
                    &RtcEvent
                    );
  if (!EFI_ERROR (Status)) {
      Status = gBS->RegisterProtocolNotify (
                      &gExitPmAuthProtocolGuid,
                      RtcEvent,
                      &RtcCallbackReg
                      );

  }

  return EFI_SUCCESS;
}

/**
  Source Or Destination with Length bytes.

  @param[in] Destination   Target memory
  @param[in] Source        Source memory
  @param[in] Length        Number of bytes

  @retval None

**/
VOID
EfiOrMem (
  IN VOID   *Destination,
  IN VOID   *Source,
  IN UINTN  Length
  )
{
  CHAR8 *Destination8;
  CHAR8 *Source8;

  if (Source < Destination) {
    Destination8  = (CHAR8 *) Destination + Length - 1;
    Source8       = (CHAR8 *) Source + Length - 1;
    while (Length--) {
      *(Destination8--) |= *(Source8--);
    }
  } else {
    Destination8  = (CHAR8 *) Destination;
    Source8       = (CHAR8 *) Source;
    while (Length--) {
      *(Destination8++) |= *(Source8++);
    }
  }
}

VOID
PchInitBeforeBoot()
{
  //
  // Saved SPI Opcode menu to fix EFI variable unable to write after S3 resume.
  //
  S3BootScriptSaveMemWrite (
                         EfiBootScriptWidthUint32,
                         (UINTN)(SPI_BASE_ADDRESS + (R_PCH_SPI_OPMENU0)),
                         1,
                         (VOID *)(UINTN)(SPI_BASE_ADDRESS + (R_PCH_SPI_OPMENU0)));

  S3BootScriptSaveMemWrite (
                         EfiBootScriptWidthUint32,
                         (UINTN)(SPI_BASE_ADDRESS + (R_PCH_SPI_OPMENU1)),
                         1,
                         (VOID *)(UINTN)(SPI_BASE_ADDRESS + (R_PCH_SPI_OPMENU1)));

  S3BootScriptSaveMemWrite (
                         EfiBootScriptWidthUint16,
                         (UINTN)(SPI_BASE_ADDRESS + R_PCH_SPI_OPTYPE),
                         1,
                         (VOID *)(UINTN)(SPI_BASE_ADDRESS + R_PCH_SPI_OPTYPE));

  S3BootScriptSaveMemWrite (
                         EfiBootScriptWidthUint16,
                         (UINTN)(SPI_BASE_ADDRESS + R_PCH_SPI_PREOP),
                         1,
                         (VOID *)(UINTN)(SPI_BASE_ADDRESS + R_PCH_SPI_PREOP));

  //
  // Saved MTPMC_1 for S3 resume.
  //
  S3BootScriptSaveMemWrite (
                         EfiBootScriptWidthUint32,
                         (UINTN)(PMC_BASE_ADDRESS + R_PCH_PMC_MTPMC1),
                         1,
                         (VOID *)(UINTN)(PMC_BASE_ADDRESS + R_PCH_PMC_MTPMC1));
  return;
}

VOID
EFIAPI
ReadyToBootFunction (
  EFI_EVENT  Event,
  VOID       *Context
  )
{
  EFI_STATUS                      Status;
  EFI_ISA_ACPI_PROTOCOL           *IsaAcpi;
  EFI_ISA_ACPI_DEVICE_ID          IsaDevice;
  UINTN                           Size;
  UINT16                          State;
  EFI_TPM_MP_DRIVER_PROTOCOL      *TpmMpDriver;
  EFI_CPU_IO_PROTOCOL             *CpuIo;
  UINT8                           Data;
  UINT8                           ReceiveBuffer [64];
  UINT32                          ReceiveBufferSize;

  UINT8 TpmForceClearCommand [] =              {0x00, 0xC1,
                                                0x00, 0x00, 0x00, 0x0A,
                                                0x00, 0x00, 0x00, 0x5D};
  UINT8 TpmPhysicalPresenceCommand [] =        {0x00, 0xC1,
                                                0x00, 0x00, 0x00, 0x0C,
                                                0x40, 0x00, 0x00, 0x0A,
                                                0x00, 0x00};
  UINT8 TpmPhysicalDisableCommand [] =         {0x00, 0xC1,
                                                0x00, 0x00, 0x00, 0x0A,
                                                0x00, 0x00, 0x00, 0x70};
  UINT8 TpmPhysicalEnableCommand [] =          {0x00, 0xC1,
                                                0x00, 0x00, 0x00, 0x0A,
                                                0x00, 0x00, 0x00, 0x6F};
  UINT8 TpmPhysicalSetDeactivatedCommand [] =  {0x00, 0xC1,
                                                0x00, 0x00, 0x00, 0x0B,
                                                0x00, 0x00, 0x00, 0x72,
                                                0x00};
  UINT8 TpmSetOwnerInstallCommand [] =         {0x00, 0xC1,
                                                0x00, 0x00, 0x00, 0x0B,
                                                0x00, 0x00, 0x00, 0x71,
                                                0x00};

  Size = sizeof(UINT16);
  Status = gRT->GetVariable (
                  VAR_EQ_FLOPPY_MODE_DECIMAL_NAME,
                  &gEfiNormalSetupGuid,
                  NULL,
                  &Size,
                  &State
                  );

  //
  // Disable Floppy Controller if needed
  //
  Status = gBS->LocateProtocol (&gEfiIsaAcpiProtocolGuid, NULL, (VOID **) &IsaAcpi);
  if (!EFI_ERROR(Status) && (State == 0x00)) {
    IsaDevice.HID = EISA_PNP_ID(0x604);
    IsaDevice.UID = 0;
    Status = IsaAcpi->EnableDevice(IsaAcpi, &IsaDevice, FALSE);
  }

  //
  // save LAN info to a variable
  //
  if (NULL != mPciLanInfo) {
    gRT->SetVariable (
           L"PciLanInfo",
           &gEfiPciLanInfoGuid,
           EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
           mPciLanCount * sizeof(PCI_LAN_INFO),
           mPciLanInfo
           );
  }

  if (NULL != mPciLanInfo) {
    gBS->FreePool (mPciLanInfo);
    mPciLanInfo = NULL;
  }
  

  //
  // Handle ACPI OS TPM requests here
  //
  Status = gBS->LocateProtocol (
                  &gEfiCpuIoProtocolGuid,
                  NULL,
                  (VOID **)&CpuIo
                  );
  Status = gBS->LocateProtocol (
                  &gEfiTpmMpDriverProtocolGuid,
                  NULL,
                  (VOID **)&TpmMpDriver
                  );
  if (!EFI_ERROR (Status))
  {
    Data = ReadCmosBank1Byte (CpuIo, ACPI_TPM_REQUEST);

    //
    // Clear pending ACPI TPM request indicator
    //
    WriteCmosBank1Byte (CpuIo, ACPI_TPM_REQUEST, 0x00);
    if (Data != 0)
    {
      WriteCmosBank1Byte (CpuIo, ACPI_TPM_LAST_REQUEST, Data);

      //
      // Assert Physical Presence for these commands
      //
      TpmPhysicalPresenceCommand [11] = 0x20;
      ReceiveBufferSize = sizeof(ReceiveBuffer);
      Status = TpmMpDriver->Transmit (
                              TpmMpDriver, TpmPhysicalPresenceCommand,
                              sizeof (TpmPhysicalPresenceCommand),
                              ReceiveBuffer, &ReceiveBufferSize
                              );
      //
      // PF PhysicalPresence = TRUE
      //
      TpmPhysicalPresenceCommand [11] = 0x08;
      ReceiveBufferSize = sizeof(ReceiveBuffer);
      Status = TpmMpDriver->Transmit (
                              TpmMpDriver, TpmPhysicalPresenceCommand,
                              sizeof (TpmPhysicalPresenceCommand),
                              ReceiveBuffer,
                              &ReceiveBufferSize
                              );
      if (Data == 0x01)
      {
        //
        // TPM_PhysicalEnable
        //
        ReceiveBufferSize = sizeof(ReceiveBuffer);
        Status = TpmMpDriver->Transmit (
                                TpmMpDriver, TpmPhysicalEnableCommand,
                                sizeof (TpmPhysicalEnableCommand),
                                ReceiveBuffer, &ReceiveBufferSize
                                );
      }
      if (Data == 0x02)
      {
        //
        // TPM_PhysicalDisable
        //
        ReceiveBufferSize = sizeof(ReceiveBuffer);
        Status = TpmMpDriver->Transmit (
                                TpmMpDriver, TpmPhysicalDisableCommand,
                                sizeof (TpmPhysicalDisableCommand),
                                ReceiveBuffer,
                                &ReceiveBufferSize
                                );
      }
      if (Data == 0x03)
      {
        //
        // TPM_PhysicalSetDeactivated=FALSE
        //
        ReceiveBufferSize = sizeof(ReceiveBuffer);
        TpmPhysicalSetDeactivatedCommand [10] = 0x00;
        Status = TpmMpDriver->Transmit (
                                TpmMpDriver,
                                TpmPhysicalSetDeactivatedCommand,
                                sizeof (TpmPhysicalSetDeactivatedCommand),
                                ReceiveBuffer, &ReceiveBufferSize
                                );
        gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
      }
      if (Data == 0x04)
      {
        //
        // TPM_PhysicalSetDeactivated=TRUE
        //
        ReceiveBufferSize = sizeof(ReceiveBuffer);
        TpmPhysicalSetDeactivatedCommand [10] = 0x01;
        Status = TpmMpDriver->Transmit (
                                TpmMpDriver,
                                TpmPhysicalSetDeactivatedCommand,
                                sizeof (TpmPhysicalSetDeactivatedCommand),
                                ReceiveBuffer,
                                &ReceiveBufferSize
                                );
        gRT->ResetSystem (
               EfiResetWarm,
               EFI_SUCCESS,
               0,
               NULL
               );
      }
      if (Data == 0x05)
      {
        //
        // TPM_ForceClear
        //
        ReceiveBufferSize = sizeof(ReceiveBuffer);
        Status = TpmMpDriver->Transmit (
                                TpmMpDriver,
                                TpmForceClearCommand,
                                sizeof (TpmForceClearCommand),
                                ReceiveBuffer,
                                &ReceiveBufferSize
                                );
        gRT->ResetSystem (
               EfiResetWarm,
               EFI_SUCCESS,
               0,
               NULL
               );
      }
      if (Data == 0x06)
      {
        //
        // TPM_PhysicalEnable
        //
        ReceiveBufferSize = sizeof(ReceiveBuffer);
        Status = TpmMpDriver->Transmit (
                                TpmMpDriver,
                                TpmPhysicalEnableCommand,
                                sizeof (TpmPhysicalEnableCommand),
                                ReceiveBuffer,
                                &ReceiveBufferSize
                                );
        //
        // TPM_PhysicalSetDeactivated=FALSE
        //
        ReceiveBufferSize = sizeof(ReceiveBuffer);
        TpmPhysicalSetDeactivatedCommand [10] = 0x00;
        Status = TpmMpDriver->Transmit (
                                TpmMpDriver,
                                TpmPhysicalSetDeactivatedCommand,
                                sizeof (TpmPhysicalSetDeactivatedCommand),
                                ReceiveBuffer,
                                &ReceiveBufferSize
                                );
        gRT->ResetSystem (
               EfiResetWarm,
               EFI_SUCCESS,
               0,
               NULL
               );
      }
      if (Data == 0x07)
      {
        //
        // TPM_PhysicalSetDeactivated=TRUE
        //
        ReceiveBufferSize = sizeof(ReceiveBuffer);
        TpmPhysicalSetDeactivatedCommand [10] = 0x01;
        Status = TpmMpDriver->Transmit (
                                TpmMpDriver,
                                TpmPhysicalSetDeactivatedCommand,
                                sizeof (TpmPhysicalSetDeactivatedCommand),
                                ReceiveBuffer,
                                &ReceiveBufferSize
                                );
        //
        // TPM_PhysicalDisable
        //
        ReceiveBufferSize = sizeof(ReceiveBuffer);
        Status = TpmMpDriver->Transmit (
                                TpmMpDriver,
                                TpmPhysicalDisableCommand,
                                sizeof (TpmPhysicalDisableCommand),
                                ReceiveBuffer,
                                &ReceiveBufferSize
                                );
        gRT->ResetSystem (
               EfiResetWarm,
               EFI_SUCCESS,
               0,
               NULL
               );
      }
      if (Data == 0x08)
      {
        //
        // TPM_SetOwnerInstall=TRUE
        //
        ReceiveBufferSize = sizeof(ReceiveBuffer);
        TpmSetOwnerInstallCommand [10] = 0x01;
        Status = TpmMpDriver->Transmit (
                                TpmMpDriver,
                                TpmSetOwnerInstallCommand,
                                sizeof (TpmSetOwnerInstallCommand),
                                ReceiveBuffer,
                                &ReceiveBufferSize
                                );
      }
      if (Data == 0x09)
      {
        //
        // TPM_SetOwnerInstall=FALSE
        //
        ReceiveBufferSize = sizeof(ReceiveBuffer);
        TpmSetOwnerInstallCommand [10] = 0x00;
        Status = TpmMpDriver->Transmit (
                                TpmMpDriver,
                                TpmSetOwnerInstallCommand,
                                sizeof (TpmSetOwnerInstallCommand),
                                ReceiveBuffer,
                                &ReceiveBufferSize
                                );
      }
      if (Data == 0x0A)
      {
        //
        // TPM_PhysicalEnable
        //
        ReceiveBufferSize = sizeof(ReceiveBuffer);
        Status = TpmMpDriver->Transmit (
                                TpmMpDriver,
                                TpmPhysicalEnableCommand,
                                sizeof (TpmPhysicalEnableCommand),
                                ReceiveBuffer,
                                &ReceiveBufferSize
                                );
        //
        // TPM_PhysicalSetDeactivated=FALSE
        //
        ReceiveBufferSize = sizeof(ReceiveBuffer);
        TpmPhysicalSetDeactivatedCommand [10] = 0x00;
        Status = TpmMpDriver->Transmit (
                                TpmMpDriver,
                                TpmPhysicalSetDeactivatedCommand,
                                sizeof (TpmPhysicalSetDeactivatedCommand),
                                ReceiveBuffer,
                                &ReceiveBufferSize
                                );
        //
        // Do TPM_SetOwnerInstall=TRUE on next reboot
        //

        WriteCmosBank1Byte (CpuIo, ACPI_TPM_REQUEST, 0xF0);

        gRT->ResetSystem (
               EfiResetWarm,
               EFI_SUCCESS,
               0,
               NULL
               );
      }
      if (Data == 0x0B)
      {
        //
        // TPM_SetOwnerInstall=FALSE
        //
        ReceiveBufferSize = sizeof(ReceiveBuffer);
        TpmSetOwnerInstallCommand [10] = 0x00;
        Status = TpmMpDriver->Transmit (
                                TpmMpDriver,
                                TpmSetOwnerInstallCommand,
                                sizeof (TpmSetOwnerInstallCommand),
                                ReceiveBuffer,
                                &ReceiveBufferSize
                                );
        //
        // TPM_PhysicalSetDeactivated=TRUE
        //
        ReceiveBufferSize = sizeof(ReceiveBuffer);
        TpmPhysicalSetDeactivatedCommand [10] = 0x01;
        Status = TpmMpDriver->Transmit (
                                TpmMpDriver,
                                TpmPhysicalSetDeactivatedCommand,
                                sizeof (TpmPhysicalSetDeactivatedCommand),
                                ReceiveBuffer,
                                &ReceiveBufferSize
                                );
        //
        // TPM_PhysicalDisable
        //
        ReceiveBufferSize = sizeof(ReceiveBuffer);
        Status = TpmMpDriver->Transmit (
                                TpmMpDriver,
                                TpmPhysicalDisableCommand,
                                sizeof (TpmPhysicalDisableCommand),
                                ReceiveBuffer,
                                &ReceiveBufferSize
                                );
        gRT->ResetSystem (
               EfiResetWarm,
               EFI_SUCCESS,
               0,
               NULL
               );
      }
      if (Data == 0x0E)
      {
        //
        // TPM_ForceClear
        //
        ReceiveBufferSize = sizeof(ReceiveBuffer);
        Status = TpmMpDriver->Transmit (
                                TpmMpDriver,
                                TpmForceClearCommand,
                                sizeof (TpmForceClearCommand),
                                ReceiveBuffer,
                                &ReceiveBufferSize
                                );
        //
        // TPM_PhysicalEnable
        //
        ReceiveBufferSize = sizeof(ReceiveBuffer);
        Status = TpmMpDriver->Transmit (
                                TpmMpDriver,
                                TpmPhysicalEnableCommand,
                                sizeof (TpmPhysicalEnableCommand),
                                ReceiveBuffer,
                                &ReceiveBufferSize
                                );
        //
        // TPM_PhysicalSetDeactivated=FALSE
        //
        ReceiveBufferSize = sizeof(ReceiveBuffer);
        TpmPhysicalSetDeactivatedCommand [10] = 0x00;
        Status = TpmMpDriver->Transmit (
                                TpmMpDriver,
                                TpmPhysicalSetDeactivatedCommand,
                                sizeof (TpmPhysicalSetDeactivatedCommand),
                                ReceiveBuffer,
                                &ReceiveBufferSize
                                );
        gRT->ResetSystem (
               EfiResetWarm,
               EFI_SUCCESS,
               0,
               NULL
               );
      }
      if (Data == 0xF0)
      {
        //
        // Second part of ACPI TPM request 0x0A: OEM custom TPM_SetOwnerInstall=TRUE
        //
        ReceiveBufferSize = sizeof(ReceiveBuffer);
        TpmSetOwnerInstallCommand [10] = 0x01;
        Status = TpmMpDriver->Transmit (
                                TpmMpDriver,
                                TpmSetOwnerInstallCommand,
                                sizeof (TpmSetOwnerInstallCommand),
                                ReceiveBuffer,
                                &ReceiveBufferSize
                                );
        WriteCmosBank1Byte (CpuIo, ACPI_TPM_LAST_REQUEST, 0x0A);
      }
      //
      // Deassert Physical Presence
      //
      TpmPhysicalPresenceCommand [11] = 0x10;
      ReceiveBufferSize = sizeof(ReceiveBuffer);
      Status = TpmMpDriver->Transmit (
                              TpmMpDriver,
                              TpmPhysicalPresenceCommand,
                              sizeof (TpmPhysicalPresenceCommand),
                              ReceiveBuffer,
                              &ReceiveBufferSize
                              );
    }
  }

  return;
}

/**

  Initializes manufacturing and config mode setting.

**/
VOID
InitMfgAndConfigModeStateVar()
{
  EFI_PLATFORM_SETUP_ID           *BootModeBuffer;
  VOID                            *HobList;


  HobList = GetFirstGuidHob(&gEfiPlatformBootModeGuid);
  if (HobList != NULL) {
    BootModeBuffer = GET_GUID_HOB_DATA (HobList);

      //
      // Check if in Manufacturing mode
      //
      if ( !CompareMem (
              &BootModeBuffer->SetupName,
              MANUFACTURE_SETUP_NAME,
              StrSize (MANUFACTURE_SETUP_NAME)
              ) ) {
        mMfgMode = TRUE;
      }



  }

}

/**

  Initializes manufacturing and config mode setting.

**/
VOID
InitPlatformBootMode()
{
  EFI_PLATFORM_SETUP_ID           *BootModeBuffer;
  VOID                            *HobList;

  HobList = GetFirstGuidHob(&gEfiPlatformBootModeGuid);
  if (HobList != NULL) {
    BootModeBuffer = GET_GUID_HOB_DATA (HobList);
    mPlatformBootMode = BootModeBuffer->PlatformBootMode;
  }
}

/**

  Initializes ITK.

**/
VOID
InitItk(
  )
{
  EFI_STATUS                          Status;
  UINT16                              ItkModBiosState;
  UINT8                               Value;
  UINTN                               DataSize;
  UINT32                              Attributes;

  //
  // Setup local variable according to ITK variable
  //
  //
  // Read ItkBiosModVar to determine if BIOS has been modified by ITK
  // If ItkBiosModVar = 0 or if variable hasn't been initialized then BIOS has not been modified by ITK modified
  // Set local variable VAR_EQ_ITK_BIOS_MOD_DECIMAL_NAME=0 if BIOS has not been modified by ITK
  //
  DataSize = sizeof (Value);
  Status = gRT->GetVariable (
                  ITK_BIOS_MOD_VAR_NAME,
                  &gItkDataVarGuid,
                  &Attributes,
                  &DataSize,
                  &Value
                  );
  if (Status == EFI_NOT_FOUND) {
    //
    // Variable not found, hasn't been initialized, intialize to 0
    //
    Value=0x00;
  //
  // Write variable to flash.
  //
  gRT->SetVariable (
         ITK_BIOS_MOD_VAR_NAME,
         &gItkDataVarGuid,
         EFI_VARIABLE_RUNTIME_ACCESS |
         EFI_VARIABLE_NON_VOLATILE |
         EFI_VARIABLE_BOOTSERVICE_ACCESS,
         sizeof (Value),
         &Value
         );

}
  if ( (!EFI_ERROR (Status)) || (Status == EFI_NOT_FOUND) ) {
    if (Value == 0x00) {
      ItkModBiosState = 0x00;
    } else {
      ItkModBiosState = 0x01;
    }
    gRT->SetVariable (
           VAR_EQ_ITK_BIOS_MOD_DECIMAL_NAME,
           &gEfiNormalSetupGuid,
           EFI_VARIABLE_BOOTSERVICE_ACCESS,
           2,
           (void *)&ItkModBiosState
           );
  }
}

#if defined(FIRMWARE_ID_BACKWARD_COMPATIBLE) && (FIRMWARE_ID_BACKWARD_COMPATIBLE != 0)

/**

  Initializes the BIOS FIRMWARE ID from the FIRMWARE_ID build variable.

**/
STATIC
VOID
InitFirmwareId(
  )
{
  EFI_STATUS   Status;
  CHAR16       FirmwareIdNameWithPassword[] = FIRMWARE_ID_NAME_WITH_PASSWORD;

  //
  // First try writing the variable without a password in case we are
  // upgrading from a BIOS without password protection on the FirmwareId
  //
  Status = gRT->SetVariable(
                  (CHAR16 *)&gFirmwareIdName,
                  &gFirmwareIdGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
                  EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof( FIRMWARE_ID ) - 1,
                  FIRMWARE_ID
                  );

  if (Status == EFI_INVALID_PARAMETER) {

    //
    // Since setting the firmware id without the password failed,
    // a password must be required.
    //
    Status = gRT->SetVariable(
                    (CHAR16 *)&FirmwareIdNameWithPassword,
                    &gFirmwareIdGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
                    EFI_VARIABLE_RUNTIME_ACCESS,
                    sizeof( FIRMWARE_ID ) - 1,
                    FIRMWARE_ID
                    );
  }
}
#endif

VOID
UpdateDVMTSetup(
  )
{
    //
  // Workaround to support IIA bug.
  // IIA request to change option value to 4, 5 and 7 relatively
  // instead of 1, 2, and 3 which follow Lakeport Specs.
  // Check option value, temporary hardcode GraphicsDriverMemorySize
  // Option value to fulfill IIA requirment. So that user no need to
  // load default and update setupvariable after update BIOS.
  //   Option value hardcoded as: 1 to 4, 2 to 5, 3 to 7.
  // *This is for broadwater and above product only.
  //

  SYSTEM_CONFIGURATION        SystemConfiguration;
  UINTN                       VarSize;
  EFI_STATUS                  Status;

  VarSize = sizeof(SYSTEM_CONFIGURATION);
  Status = gRT->GetVariable(
                  NORMAL_SETUP_NAME,
                  &gEfiNormalSetupGuid,
                  NULL,
                  &VarSize,
                  &SystemConfiguration
                  );

  if (EFI_ERROR (Status) || VarSize != sizeof(SYSTEM_CONFIGURATION)) {
    //The setup variable is corrupted
    VarSize = sizeof(SYSTEM_CONFIGURATION);
    Status = gRT->GetVariable(
              L"SetupRecovery",
              &gEfiNormalSetupGuid,
              NULL,
              &VarSize,
              &SystemConfiguration
              );
    ASSERT_EFI_ERROR (Status);
  }

  if((SystemConfiguration.GraphicsDriverMemorySize < 4) && !EFI_ERROR(Status) ) {
    switch (SystemConfiguration.GraphicsDriverMemorySize){
      case 1:
        SystemConfiguration.GraphicsDriverMemorySize = 4;
        break;
      case 2:
        SystemConfiguration.GraphicsDriverMemorySize = 5;
        break;
      case 3:
        SystemConfiguration.GraphicsDriverMemorySize = 7;
        break;
      default:
        break;
     }

    Status = gRT->SetVariable (
                    NORMAL_SETUP_NAME,
                    &gEfiNormalSetupGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof(SYSTEM_CONFIGURATION),
                    &SystemConfiguration
                    );
  }
}

VOID
InitPlatformUsbPolicy (
  VOID
  )

{
  EFI_HANDLE              Handle;
  EFI_STATUS              Status;

  Handle = NULL;

  mUsbPolicyData.Version                       = (UINT8)USB_POLICY_PROTOCOL_REVISION_2;
  mUsbPolicyData.UsbMassStorageEmulationType   = mSystemConfiguration.UsbBIOSINT13DeviceEmulation;
  if(mUsbPolicyData.UsbMassStorageEmulationType == 3) {
    mUsbPolicyData.UsbEmulationSize = mSystemConfiguration.UsbBIOSINT13DeviceEmulationSize;
  } else {
    mUsbPolicyData.UsbEmulationSize = 0;
  }
  mUsbPolicyData.UsbZipEmulationType         = mSystemConfiguration.UsbZipEmulation;
  mUsbPolicyData.UsbOperationMode              = HIGH_SPEED;

  //
  //  Some chipset need Period smi, 0 = LEGACY_PERIOD_UN_SUPP
  //
  mUsbPolicyData.USBPeriodSupport      = LEGACY_PERIOD_UN_SUPP;

  //
  //  Some platform need legacyfree, 0 = LEGACY_FREE_UN_SUPP
  //
  mUsbPolicyData.LegacyFreeSupport    = LEGACY_FREE_UN_SUPP;

  //
  //  Set Code base , TIANO_CODE_BASE =0x01, ICBD =0x00
  //
  mUsbPolicyData.CodeBase    = (UINT8)ICBD_CODE_BASE;

  //
  //  Some chispet 's LpcAcpibase are diffrent,set by platform or chipset,
  //  default is Ich  acpibase =0x040. acpitimerreg=0x08.
  mUsbPolicyData.LpcAcpiBase     = 0x40;
  mUsbPolicyData.AcpiTimerReg    = 0x08;

  //
  //  Set for reduce usb post time
  //
  mUsbPolicyData.UsbTimeTue           = 0x00;
  mUsbPolicyData.InternelHubExist     = 0x00;  //TigerPoint doesn't have RMH
  mUsbPolicyData.EnumWaitPortStableStall    = 100;


  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gUsbPolicyGuid,
                  EFI_NATIVE_INTERFACE,
                  &mUsbPolicyData
                  );
  ASSERT_EFI_ERROR(Status);

}

UINT8
ReadCmosBank1Byte (
  IN  EFI_CPU_IO_PROTOCOL             *CpuIo,
  IN  UINT8                           Index
  )
{
  UINT8                               Data;

  CpuIo->Io.Write (CpuIo, EfiCpuIoWidthUint8, 0x72, 1, &Index);
  CpuIo->Io.Read (CpuIo, EfiCpuIoWidthUint8, 0x73, 1, &Data);
  return Data;
}

VOID
WriteCmosBank1Byte (
  IN  EFI_CPU_IO_PROTOCOL             *CpuIo,
  IN  UINT8                           Index,
  IN  UINT8                           Data
  )
{
  CpuIo->Io.Write (
              CpuIo,
              EfiCpuIoWidthUint8,
              0x72,
              1,
              &Index
              );
  CpuIo->Io.Write (
              CpuIo,
              EfiCpuIoWidthUint8,
              0x73,
              1,
              &Data
              );
}

