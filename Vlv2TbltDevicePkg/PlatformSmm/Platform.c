/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

    Platform.c

Abstract:

    This is a generic template for a child of the IchSmm driver.


--*/

#include "SmmPlatform.h"
#include <Protocol/CpuIo2.h>


//
// Local variables
//
typedef struct {
  UINT8     Device;
  UINT8     Function;
} EFI_PCI_BUS_MASTER;

EFI_PCI_BUS_MASTER  mPciBm[] = {
  { PCI_DEVICE_NUMBER_PCH_PCIE_ROOT_PORTS, PCI_FUNCTION_NUMBER_PCH_PCIE_ROOT_PORT_1 },
  { PCI_DEVICE_NUMBER_PCH_PCIE_ROOT_PORTS, PCI_FUNCTION_NUMBER_PCH_PCIE_ROOT_PORT_2 },
  { PCI_DEVICE_NUMBER_PCH_PCIE_ROOT_PORTS, PCI_FUNCTION_NUMBER_PCH_PCIE_ROOT_PORT_3 },
  { PCI_DEVICE_NUMBER_PCH_PCIE_ROOT_PORTS, PCI_FUNCTION_NUMBER_PCH_PCIE_ROOT_PORT_4 },
  { PCI_DEVICE_NUMBER_PCH_USB, PCI_FUNCTION_NUMBER_PCH_EHCI }
};


UINT16                                  mAcpiBaseAddr;
SYSTEM_CONFIGURATION                    mSystemConfiguration;
EFI_SMM_VARIABLE_PROTOCOL               *mSmmVariable;
EFI_GLOBAL_NVS_AREA_PROTOCOL            *mGlobalNvsAreaPtr;

UINT16									                mPM1_SaveState16;
UINT32									                mGPE_SaveState32;

BOOLEAN                                 mSetSmmVariableProtocolSmiAllowed = TRUE;


//
// Variables. Need to initialize this from Setup
//
BOOLEAN                                 mWakeOnLanS5Variable;
BOOLEAN                                 mWakeOnRtcVariable;
UINT8                                   mWakeupDay;
UINT8                                   mWakeupHour;
UINT8                                   mWakeupMinute;
UINT8                                   mWakeupSecond;

//
// Use an enum. 0 is Stay Off, 1 is Last State, 2 is Stay On
//
UINT8                                   mAcLossVariable;


static
UINT8 mTco1Sources[] = {
  IchnNmi
};

UINTN
DevicePathSize (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

VOID
S4S5ProgClock();

EFI_STATUS
InitRuntimeScriptTable (
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

VOID
S5SleepWakeOnRtcCallBack (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  EFI_SMM_SX_DISPATCH_CONTEXT   *DispatchContext
  );


VOID
EnableS5WakeOnRtc();

UINT8
HexToBcd(
  UINT8 HexValue
  );

UINT8
BcdToHex(
  IN UINT8 BcdValue
  );


VOID
CpuSmmSxWorkAround(
  );

/**
  Initializes the SMM Handler Driver

  @param ImageHandle
  @param SystemTable

  @retval None

**/
EFI_STATUS
EFIAPI
InitializePlatformSmm (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                                Status;
  UINT8                                     Index;
  EFI_HANDLE                                Handle;
  EFI_SMM_POWER_BUTTON_DISPATCH_CONTEXT     PowerButtonContext;
  EFI_SMM_POWER_BUTTON_DISPATCH_PROTOCOL    *PowerButtonDispatch;
  EFI_SMM_ICHN_DISPATCH_CONTEXT             IchnContext;
  EFI_SMM_ICHN_DISPATCH_PROTOCOL            *IchnDispatch;
  EFI_SMM_SX_DISPATCH_PROTOCOL              *SxDispatch;
  EFI_SMM_SX_DISPATCH_CONTEXT               EntryDispatchContext;
  EFI_SMM_SW_DISPATCH_PROTOCOL              *SwDispatch;
  EFI_SMM_SW_DISPATCH_CONTEXT               SwContext;
  UINTN                                     VarSize;
  EFI_BOOT_MODE                             BootMode;

  Handle = NULL;

  //
  //  Locate the Global NVS Protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiGlobalNvsAreaProtocolGuid,
                  NULL,
                  (void **)&mGlobalNvsAreaPtr
                  );
  ASSERT_EFI_ERROR (Status);


  //
  // Get the ACPI Base Address
  //

  mAcpiBaseAddr = PchLpcPciCfg16( R_PCH_LPC_ACPI_BASE ) & B_PCH_LPC_ACPI_BASE_BAR;

  VarSize = sizeof(SYSTEM_CONFIGURATION);
  Status = SystemTable->RuntimeServices->GetVariable(
                          L"Setup",
                          &gEfiSetupVariableGuid,
                          NULL,
                          &VarSize,
                          &mSystemConfiguration
                          );
  if (EFI_ERROR (Status) || VarSize != sizeof(SYSTEM_CONFIGURATION)) {
    //The setup variable is corrupted
    VarSize = sizeof(SYSTEM_CONFIGURATION);
    Status = SystemTable->RuntimeServices->GetVariable(
              L"SetupRecovery",
              &gEfiSetupVariableGuid,
              NULL,
              &VarSize,
              &mSystemConfiguration
              );
    ASSERT_EFI_ERROR (Status);
  }  
  if (!EFI_ERROR(Status)) {
    mAcLossVariable = mSystemConfiguration.StateAfterG3;

    //
    // If LAN is disabled, WOL function should be disabled too.
    //
    if (mSystemConfiguration.Lan == 0x01){
      mWakeOnLanS5Variable = mSystemConfiguration.WakeOnLanS5;
    } else {
      mWakeOnLanS5Variable = FALSE;
    }

    mWakeOnRtcVariable = mSystemConfiguration.WakeOnRtcS5;
  }

  BootMode = GetBootModeHob ();

  //
  // Get the Power Button protocol
  //
  Status = gBS->LocateProtocol(
                  &gEfiSmmPowerButtonDispatchProtocolGuid,
                  NULL,
                  (void **)&PowerButtonDispatch
                  );
  ASSERT_EFI_ERROR(Status);

  if (BootMode != BOOT_ON_FLASH_UPDATE) {
    //
    // Register for the power button event
    //
    PowerButtonContext.Phase = PowerButtonEntry;
    Status = PowerButtonDispatch->Register(
                                    PowerButtonDispatch,
                                    PowerButtonCallback,
                                    &PowerButtonContext,
                                    &Handle
                                    );
    ASSERT_EFI_ERROR(Status);
  }
  //
  // Get the Sx dispatch protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiSmmSxDispatchProtocolGuid,
                  NULL,
                                  (void **)&SxDispatch
                  );
  ASSERT_EFI_ERROR(Status);

  //
  // Register entry phase call back function
  //
  EntryDispatchContext.Type  = SxS3;
  EntryDispatchContext.Phase = SxEntry;

  Status = SxDispatch->Register (
                         SxDispatch,
                           (EFI_SMM_SX_DISPATCH)SxSleepEntryCallBack,
                         &EntryDispatchContext,
                         &Handle
                         );


  EntryDispatchContext.Type  = SxS4;

  Status = SxDispatch->Register (
                         SxDispatch,
                         S4S5CallBack,
                         &EntryDispatchContext,
                         &Handle
                         );
  ASSERT_EFI_ERROR(Status);


  EntryDispatchContext.Type  = SxS5;

  Status = SxDispatch->Register (
                         SxDispatch,
                         S4S5CallBack,
                         &EntryDispatchContext,
                         &Handle
                         );
  ASSERT_EFI_ERROR(Status);

  Status = SxDispatch->Register (
                         SxDispatch,
                         S5SleepAcLossCallBack,
                         &EntryDispatchContext,
                         &Handle
                         );
  ASSERT_EFI_ERROR(Status);

  //
  //  Get the Sw dispatch protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiSmmSwDispatchProtocolGuid,
                  NULL,
                                  (void **)&SwDispatch
                  );
  ASSERT_EFI_ERROR(Status);

  //
  // Register ACPI enable handler
  //
  SwContext.SwSmiInputValue = ACPI_ENABLE;
  Status = SwDispatch->Register (
                         SwDispatch,
                         EnableAcpiCallback,
                         &SwContext,
                         &Handle
                         );
  ASSERT_EFI_ERROR(Status);

  //
  // Register ACPI disable handler
  //
  SwContext.SwSmiInputValue = ACPI_DISABLE;
  Status = SwDispatch->Register (
                         SwDispatch,
                         DisableAcpiCallback,
                         &SwContext,
                         &Handle
                         );
  ASSERT_EFI_ERROR(Status);


  //
  // Register for SmmReadyToBootCallback
  //
  SwContext.SwSmiInputValue = SMI_SET_SMMVARIABLE_PROTOCOL;
  Status = SwDispatch->Register(
                         SwDispatch,
                         SmmReadyToBootCallback,
                         &SwContext,
                         &Handle
                         );
  ASSERT_EFI_ERROR(Status);

  //
  // Get the ICHn protocol
  //
  Status = gBS->LocateProtocol(
                  &gEfiSmmIchnDispatchProtocolGuid,
                  NULL,
                  (void **)&IchnDispatch
                  );
  ASSERT_EFI_ERROR(Status);

  //
  // Register for the events that may happen that we do not care.
  // This is true for SMI related to TCO since TCO is enabled by BIOS WP
  //
  for (Index = 0; Index < sizeof(mTco1Sources)/sizeof(UINT8); Index++) {
    IchnContext.Type = mTco1Sources[Index];
    Status = IchnDispatch->Register(
                             IchnDispatch,
                             (EFI_SMM_ICHN_DISPATCH)DummyTco1Callback,
                             &IchnContext,
                             &Handle
                             );
    ASSERT_EFI_ERROR( Status );
  }

  //
  // Lock TCO_EN bit.
  //
  IoWrite16( mAcpiBaseAddr + R_PCH_TCO_CNT, IoRead16( mAcpiBaseAddr + R_PCH_TCO_CNT ) | B_PCH_TCO_CNT_LOCK );

  //
  // Set to power on from G3 dependent on WOL instead of AC Loss variable in order to support WOL from G3 feature.
  //
  //
  // Set wake from G3 dependent on AC Loss variable and Wake On LAN variable.
  // This is because no matter how, if WOL enabled or AC Loss variable not disabled, the board needs to wake from G3 to program the LAN WOL settings.
  // This needs to be done after LAN enable/disable so that the PWR_FLR state clear not impacted the WOL from G3 feature.
  //
  if (mAcLossVariable != 0x00) {
    SetAfterG3On (TRUE);
  } else {
    SetAfterG3On (FALSE);
  }




  return EFI_SUCCESS;
}

VOID
EFIAPI
SmmReadyToBootCallback (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  EFI_SMM_SW_DISPATCH_CONTEXT   *DispatchContext
  )
{
  EFI_STATUS Status;

  if (mSetSmmVariableProtocolSmiAllowed)
  {
  	//
    // It is okay to use gBS->LocateProtocol here because
    // we are still in trusted execution.
    //
  Status = gBS->LocateProtocol(
                  &gEfiSmmVariableProtocolGuid,
                  NULL,
                  (void **)&mSmmVariable
                  );

    ASSERT_EFI_ERROR(Status);

    //
    // mSetSmmVariableProtocolSmiAllowed will prevent this function from
    // being executed more than 1 time.
    //
    mSetSmmVariableProtocolSmiAllowed = FALSE;
  }

}

/**

  @param DispatchHandle   The handle of this callback, obtained when registering
  @param DispatchContext  The predefined context which contained sleep type and phase


  @retval EFI_SUCCESS     Operation successfully performed

**/
EFI_STATUS
EFIAPI    
SxSleepEntryCallBack (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  EFI_SMM_SX_DISPATCH_CONTEXT   *DispatchContext
  )
{
  EFI_STATUS              Status;

  Status = SaveRuntimeScriptTable ();
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // Workaround for S3 wake hang if C State is enabled
  //
  CpuSmmSxWorkAround();

  return EFI_SUCCESS;
}

VOID
CpuSmmSxWorkAround(
  )
{
  UINT64           MsrValue;

  MsrValue = AsmReadMsr64 (0xE2);

  if (MsrValue & BIT15) {
    return;
  }

  if (MsrValue & BIT10) {
    MsrValue &= ~BIT10;
    AsmWriteMsr64 (0xE2, MsrValue);
  }
}

VOID
ClearP2PBusMaster(
  )
{
  UINT8             Command;
  UINT8             Index;

  for (Index = 0; Index < sizeof(mPciBm)/sizeof(EFI_PCI_BUS_MASTER); Index++) {
    Command = MmioRead8 (
                MmPciAddress (0,
                  DEFAULT_PCI_BUS_NUMBER_PCH,
                  mPciBm[Index].Device,
                  mPciBm[Index].Function,
                  PCI_COMMAND_OFFSET
                )
              );
    Command &= ~EFI_PCI_COMMAND_BUS_MASTER;
    MmioWrite8 (
      MmPciAddress (0,
        DEFAULT_PCI_BUS_NUMBER_PCH,
        mPciBm[Index].Device,
        mPciBm[Index].Function,
        PCI_COMMAND_OFFSET
      ),
      Command
    );
  }
}

/**

  Set the AC Loss to turn on or off.

**/
VOID
SetAfterG3On (
  BOOLEAN Enable
  )
{
  UINT8             PmCon1;

  //
  // ICH handling portion
  //
  PmCon1 = MmioRead8 ( PMC_BASE_ADDRESS + R_PCH_PMC_GEN_PMCON_1 );
  PmCon1 &= ~B_PCH_PMC_GEN_PMCON_AFTERG3_EN;
  if (Enable) {
    PmCon1 |= B_PCH_PMC_GEN_PMCON_AFTERG3_EN;
  }
  MmioWrite8 (PMC_BASE_ADDRESS + R_PCH_PMC_GEN_PMCON_1, PmCon1);

}

/**
  When a power button event happens, it shuts off the machine

**/
VOID
EFIAPI
PowerButtonCallback (
  IN  EFI_HANDLE                              DispatchHandle,
  IN  EFI_SMM_POWER_BUTTON_DISPATCH_CONTEXT   *DispatchContext
  )
{
  //
  // Check what the state to return to after AC Loss. If Last State, then
  // set it to Off.
  //
  UINT16  data16;

  if (mWakeOnRtcVariable) {
    EnableS5WakeOnRtc();
  }

  if (mAcLossVariable == 1) {
    SetAfterG3On (TRUE);
  }

  ClearP2PBusMaster();

  //
  // Program clock chip
  //
  S4S5ProgClock();


  data16 = (UINT16)(IoRead16(mAcpiBaseAddr + R_PCH_ACPI_GPE0a_EN));
  data16 &= B_PCH_ACPI_GPE0a_EN_PCI_EXP;


  //
  // Clear Sleep SMI Status
  //
  IoWrite16 (mAcpiBaseAddr + R_PCH_SMI_STS,
                (UINT16)(IoRead16 (mAcpiBaseAddr + R_PCH_SMI_STS) | B_PCH_SMI_STS_ON_SLP_EN));
  //
  // Clear Sleep Type Enable
  //
  IoWrite16 (mAcpiBaseAddr + R_PCH_SMI_EN,
                (UINT16)(IoRead16 (mAcpiBaseAddr + R_PCH_SMI_EN) & (~B_PCH_SMI_EN_ON_SLP_EN)));

  //
  // Clear Power Button Status
  //
  IoWrite16(mAcpiBaseAddr + R_PCH_ACPI_PM1_STS, B_PCH_ACPI_PM1_STS_PWRBTN);

  //
  // Shut it off now!
  //
  IoWrite16(mAcpiBaseAddr + R_PCH_ACPI_PM1_CNT, V_PCH_ACPI_PM1_CNT_S5);
  IoWrite16(mAcpiBaseAddr + R_PCH_ACPI_PM1_CNT, B_PCH_ACPI_PM1_CNT_SLP_EN | V_PCH_ACPI_PM1_CNT_S5);

  //
  // Should not return
  //
  CpuDeadLoop();
}


/**
  @param DispatchHandle  - The handle of this callback, obtained when registering

  @param DispatchContext - The predefined context which contained sleep type and phase

**/
VOID
EFIAPI
S5SleepAcLossCallBack (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  EFI_SMM_SX_DISPATCH_CONTEXT   *DispatchContext
  )
{
  //
  // Check what the state to return to after AC Loss. If Last State, then
  // set it to Off.
  //
  if (mAcLossVariable == 1) {
    SetAfterG3On (TRUE);
  }
}

/**

  @param DispatchHandle   The handle of this callback, obtained when registering
  @param DispatchContext  The predefined context which contained sleep type and phase

  @retval Clears the Save State bit in the clock.

**/
VOID
EFIAPI
S4S5CallBack (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  EFI_SMM_SX_DISPATCH_CONTEXT   *DispatchContext
  )
{

  UINT32        Data32;

  //
  // Enable/Disable USB Charging
  //
  if (mSystemConfiguration.UsbCharging == 0x01) {
    Data32 = IoRead32 (GPIO_BASE_ADDRESS + R_PCH_GPIO_SC_LVL);
    Data32 |= BIT8;
    IoWrite32(GPIO_BASE_ADDRESS + R_PCH_GPIO_SC_LVL, Data32);
  }

}


VOID
S4S5ProgClock()
{
}

/**
  SMI handler to enable ACPI mode

  Dispatched on reads from APM port with value 0xA0

  Disables the SW SMI Timer.
  ACPI events are disabled and ACPI event status is cleared.
  SCI mode is then enabled.

   Disable SW SMI Timer

   Clear all ACPI event status and disable all ACPI events
   Disable PM sources except power button
   Clear status bits

   Disable GPE0 sources
   Clear status bits

   Disable GPE1 sources
   Clear status bits

   Guarantee day-of-month alarm is invalid (ACPI 5.0 Section 4.8.2.4 "Real Time Clock Alarm")

   Enable SCI

  @param DispatchHandle  - EFI Handle
  @param DispatchContext - Pointer to the EFI_SMM_SW_DISPATCH_CONTEXT

  @retval Nothing

**/
VOID
EFIAPI
EnableAcpiCallback (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  EFI_SMM_SW_DISPATCH_CONTEXT   *DispatchContext
  )
{
  UINT32 SmiEn;
  UINT16 Pm1Cnt;
  UINT16 wordValue;
  UINT32 RegData32;

  //
  // Disable SW SMI Timer
  //
  SmiEn = IoRead32(mAcpiBaseAddr + R_PCH_SMI_EN);
  SmiEn &= ~B_PCH_SMI_STS_SWSMI_TMR;
  IoWrite32(mAcpiBaseAddr + R_PCH_SMI_EN, SmiEn);

  wordValue = IoRead16(mAcpiBaseAddr + R_PCH_ACPI_PM1_STS);
  if(wordValue & B_PCH_ACPI_PM1_STS_WAK) {
	  IoWrite32((mAcpiBaseAddr + R_PCH_ACPI_GPE0a_EN), 0x0000);
	  IoWrite32((mAcpiBaseAddr + R_PCH_ACPI_GPE0a_STS), 0xffffffff);
  }
  else {
		mPM1_SaveState16 = IoRead16(mAcpiBaseAddr + R_PCH_ACPI_PM1_EN);

		//
		// Disable PM sources except power button
		//
    // power button is enabled only for PCAT. Disabled it on Tablet platform
    //
    IoWrite16(mAcpiBaseAddr + R_PCH_ACPI_PM1_EN, B_PCH_ACPI_PM1_EN_PWRBTN);
		IoWrite16(mAcpiBaseAddr + R_PCH_ACPI_PM1_STS, 0xffff);

		mGPE_SaveState32 = IoRead16(mAcpiBaseAddr + R_PCH_ACPI_GPE0a_EN);
		IoWrite32(mAcpiBaseAddr + R_PCH_ACPI_GPE0a_EN, 0x0000);
		IoWrite32(mAcpiBaseAddr + R_PCH_ACPI_GPE0a_STS, 0xffffffff);

  }

  //
  // Guarantee day-of-month alarm is invalid (ACPI 5.0 Section 4.8.2.4 "Real Time Clock Alarm")
  // Clear Status D reg VM bit, Date of month Alarm to make Data in CMOS RAM is no longer Valid
  //
  IoWrite8 (PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_REGISTER_D);
  IoWrite8 (PCAT_RTC_DATA_REGISTER, 0x0);


	RegData32 = IoRead32(ACPI_BASE_ADDRESS + R_PCH_ALT_GP_SMI_EN);
	RegData32 &= ~(BIT7);
    IoWrite32((ACPI_BASE_ADDRESS + R_PCH_ALT_GP_SMI_EN), RegData32);


  //
  // Enable SCI
  //
  Pm1Cnt = IoRead16(mAcpiBaseAddr + R_PCH_ACPI_PM1_CNT);
  Pm1Cnt |= B_PCH_ACPI_PM1_CNT_SCI_EN;
  IoWrite16(mAcpiBaseAddr + R_PCH_ACPI_PM1_CNT, Pm1Cnt);


}

/**
  SMI handler to disable ACPI mode

  Dispatched on reads from APM port with value 0xA1

  ACPI events are disabled and ACPI event status is cleared.
  SCI mode is then disabled.
   Clear all ACPI event status and disable all ACPI events
   Disable PM sources except power button
   Clear status bits
   Disable GPE0 sources
   Clear status bits
   Disable GPE1 sources
   Clear status bits
   Disable SCI

  @param DispatchHandle  - EFI Handle
  @param DispatchContext - Pointer to the EFI_SMM_SW_DISPATCH_CONTEXT

  @retval Nothing

**/
VOID
EFIAPI
DisableAcpiCallback (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  EFI_SMM_SW_DISPATCH_CONTEXT   *DispatchContext
  )
{
  UINT16 Pm1Cnt;

  IoWrite16(mAcpiBaseAddr + R_PCH_ACPI_PM1_STS, 0xffff);
  IoWrite16(mAcpiBaseAddr + R_PCH_ACPI_PM1_EN, mPM1_SaveState16);

  IoWrite32(mAcpiBaseAddr + R_PCH_ACPI_GPE0a_STS, 0xffffffff);
  IoWrite32(mAcpiBaseAddr + R_PCH_ACPI_GPE0a_EN, mGPE_SaveState32);

  //
  // Disable SCI
  //
  Pm1Cnt = IoRead16(mAcpiBaseAddr + R_PCH_ACPI_PM1_CNT);
  Pm1Cnt &= ~B_PCH_ACPI_PM1_CNT_SCI_EN;
  IoWrite16(mAcpiBaseAddr + R_PCH_ACPI_PM1_CNT, Pm1Cnt);

}

/**
  When an unknown event happen.

 @retval None

**/
VOID
DummyTco1Callback (
  IN  EFI_HANDLE                              DispatchHandle,
  IN  EFI_SMM_ICHN_DISPATCH_CONTEXT           *DispatchContext
  )
{
}

UINTN
DevicePathSize (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL     *Start;

  if (DevicePath == NULL) {
    return 0;
  }

  //
  // Search for the end of the device path structure
  //
  Start = DevicePath;
  while (!IsDevicePathEnd (DevicePath)) {
    DevicePath = NextDevicePathNode (DevicePath);
  }

  //
  // Compute the size and add back in the size of the end device path structure
  //
  return ((UINTN)DevicePath - (UINTN)Start) + sizeof(EFI_DEVICE_PATH_PROTOCOL);
}

/**

  @param DispatchHandle   The handle of this callback, obtained when registering
  @param DispatchContext  The predefined context which contained sleep type and phase

**/
VOID
S5SleepWakeOnRtcCallBack (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  EFI_SMM_SX_DISPATCH_CONTEXT   *DispatchContext
  )
{
  EnableS5WakeOnRtc();
}

/**

 @retval 1. Check Alarm interrupt is not set.
         2. Clear Alarm interrupt.
         2. Set RTC wake up date and time.
         2. Enable RTC wake up alarm.
         3. Enable ICH PM1 EN Bit 10(RTC_EN)

**/
VOID
EnableS5WakeOnRtc()
{
  UINT8             CmosData;
  UINTN             i;
  EFI_STATUS        Status;
  UINTN             VarSize;

  //
  // make sure EFI_SMM_VARIABLE_PROTOCOL is available
  //
  if (!mSmmVariable) {
    return;
  }

  VarSize = sizeof(SYSTEM_CONFIGURATION);

  //
  // read the variable into the buffer
  //
  Status = mSmmVariable->SmmGetVariable(
                           L"Setup",
                           &gEfiSetupVariableGuid,
                           NULL,
                           &VarSize,
                           &mSystemConfiguration
                           );
  if (EFI_ERROR(Status) || VarSize != sizeof(SYSTEM_CONFIGURATION)) {
    //The setup variable is corrupted
    VarSize = sizeof(SYSTEM_CONFIGURATION);
    Status = mSmmVariable->SmmGetVariable(
              L"SetupRecovery",
              &gEfiSetupVariableGuid,
              NULL,
              &VarSize,
              &mSystemConfiguration
              );
    ASSERT_EFI_ERROR (Status);
  }

  if (!mSystemConfiguration.WakeOnRtcS5) {
    return;
  }
  mWakeupDay = HexToBcd((UINT8)mSystemConfiguration.RTCWakeupDate);
  mWakeupHour = HexToBcd((UINT8)mSystemConfiguration.RTCWakeupTimeHour);
  mWakeupMinute = HexToBcd((UINT8)mSystemConfiguration.RTCWakeupTimeMinute);
  mWakeupSecond = HexToBcd((UINT8)mSystemConfiguration.RTCWakeupTimeSecond);

  //
  // Check RTC alarm interrupt is enabled.  If enabled, someone already
  // grabbed RTC alarm.  Just return.
  //
  IoWrite8(PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_REGISTER_B);
  if(IoRead8(PCAT_RTC_DATA_REGISTER) & B_RTC_ALARM_INT_ENABLE){
    return;
  }

  //
  // Set Date
  //
  IoWrite8(PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_REGISTER_D);
  CmosData = IoRead8(PCAT_RTC_DATA_REGISTER);
  CmosData &= ~(B_RTC_DATE_ALARM_MASK);
  CmosData |= mWakeupDay ;
  for(i = 0 ; i < 0xffff ; i++){
    IoWrite8(PCAT_RTC_DATA_REGISTER, CmosData);
    SmmStall(1);
    if(((CmosData = IoRead8(PCAT_RTC_DATA_REGISTER)) & B_RTC_DATE_ALARM_MASK)
         == mWakeupDay){
      break;
    }
  }

  //
  // Set Second
  //
  IoWrite8(PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_SECOND_ALARM);
  for(i = 0 ; i < 0xffff ; i++){
    IoWrite8(PCAT_RTC_DATA_REGISTER, mWakeupSecond);
    SmmStall(1);
    if(IoRead8(PCAT_RTC_DATA_REGISTER) == mWakeupSecond){
      break;
    }
  }

  //
  // Set Minute
  //
  IoWrite8(PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_MINUTE_ALARM);
  for(i = 0 ; i < 0xffff ; i++){
    IoWrite8(PCAT_RTC_DATA_REGISTER, mWakeupMinute);
    SmmStall(1);
    if(IoRead8(PCAT_RTC_DATA_REGISTER) == mWakeupMinute){
      break;
    }
  }

  //
  // Set Hour
  //
  IoWrite8(PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_HOUR_ALARM);
  for(i = 0 ; i < 0xffff ; i++){
    IoWrite8(PCAT_RTC_DATA_REGISTER, mWakeupHour);
    SmmStall(1);
    if(IoRead8(PCAT_RTC_DATA_REGISTER) == mWakeupHour){
      break;
    }
  }

  //
  // Wait for UIP to arm RTC alarm
  //
  IoWrite8(PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_REGISTER_A);
  while (IoRead8(PCAT_RTC_DATA_REGISTER) & 0x80);

  //
  // Read RTC register 0C to clear pending RTC interrupts
  //
  IoWrite8(PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_REGISTER_C);
  IoRead8(PCAT_RTC_DATA_REGISTER);

  //
  // Enable RTC Alarm Interrupt
  //
  IoWrite8(PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_REGISTER_B);
  IoWrite8(PCAT_RTC_DATA_REGISTER, IoRead8(PCAT_RTC_DATA_REGISTER) | B_RTC_ALARM_INT_ENABLE);

  //
  // Clear ICH RTC Status
  //
  IoWrite16(mAcpiBaseAddr + R_PCH_ACPI_PM1_STS, B_PCH_ACPI_PM1_STS_RTC);

  //
  // Enable ICH RTC event
  //
  IoWrite16(mAcpiBaseAddr + R_PCH_ACPI_PM1_EN,
              (UINT16)(IoRead16(mAcpiBaseAddr + R_PCH_ACPI_PM1_EN) | B_PCH_ACPI_PM1_EN_RTC));
}

UINT8
HexToBcd(
  IN UINT8 HexValue
  )
{
  UINTN   HighByte;
  UINTN   LowByte;

  HighByte    = (UINTN)HexValue / 10;
  LowByte     = (UINTN)HexValue % 10;

  return ((UINT8)(LowByte + (HighByte << 4)));
}

UINT8
BcdToHex(
  IN UINT8 BcdValue
  )
{
  UINTN   HighByte;
  UINTN   LowByte;

  HighByte    = (UINTN)((BcdValue >> 4) * 10);
  LowByte     = (UINTN)(BcdValue & 0x0F);

  return ((UINT8)(LowByte + HighByte));
}

