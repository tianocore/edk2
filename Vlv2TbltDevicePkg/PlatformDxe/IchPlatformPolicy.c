/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   

  This program and the accompanying materials are licensed and made available under

  the terms and conditions of the BSD License that accompanies this distribution.  

  The full text of the license may be found at                                     

  http://opensource.org/licenses/bsd-license.php.                                  

                                                                                   

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            

  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    

                                                                                   


Module Name:


  PchPlatformPolicy.c

Abstract:


--*/

#include "PlatformDxe.h"
#include <Protocol/PchPlatformPolicy.h>
#include <Protocol/VlvPlatformPolicy.h>
#include <Library/PchPlatformLib.h>

#include "AzaliaVerbTable.h"
#include "Protocol/GlobalNvsArea.h"
#include "Protocol/DxePchPolicyUpdateProtocol.h"

#define MOBILE_PLATFORM 1
#define DESKTOP_PLATFORM 2

EFI_GUID                        gDxePchPolicyUpdateProtocolGuid = DXE_PCH_POLICY_UPDATE_PROTOCOL_GUID;
DXE_PCH_POLICY_UPDATE_PROTOCOL  mDxePchPolicyUpdate = { 0 };

/**

  Updates the feature policies according to the setup variable.

  @retval VOID

**/
VOID
InitPchPlatformPolicy (
  IN EFI_PLATFORM_INFO_HOB      *PlatformInfo
  )
{
  DXE_PCH_PLATFORM_POLICY_PROTOCOL *DxePlatformPchPolicy;
  EFI_STATUS                       Status;
  EFI_GLOBAL_NVS_AREA_PROTOCOL     *GlobalNvsArea;
  UINT8                            PortIndex;
  EFI_HANDLE                       Handle;
  PCH_STEPPING                     SocStepping = PchA0;
  BOOLEAN                          ModifyVariable;

  ModifyVariable = FALSE;
  DEBUG ((EFI_D_INFO, "InitPchPlatformPolicy() - Start\n"));

  Status  = gBS->LocateProtocol (&gDxePchPlatformPolicyProtocolGuid, NULL, (VOID **) &DxePlatformPchPolicy);
  ASSERT_EFI_ERROR (Status);

  //
  //  Locate the Global NVS Protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiGlobalNvsAreaProtocolGuid,
                  NULL,
                  (VOID **) &GlobalNvsArea
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Update system information
  //
  DxePlatformPchPolicy->Revision  = DXE_PCH_PLATFORM_POLICY_PROTOCOL_REVISION_12;

  //
  // General initialization
  //
  DxePlatformPchPolicy->BusNumber = 0;

  //
  // VLV BIOS Spec Section 3.6 Flash Security Recommendation,
  // Intel strongly recommends that BIOS sets the BIOS Interface Lock Down bit. Enabling this bit
  // will mitigate malicious software attempts to replace the system BIOS option ROM with its own code.
  // We always enable this as a platform policy.
  //
  DxePlatformPchPolicy->LockDownConfig->BiosInterface = PCH_DEVICE_ENABLE;
  DxePlatformPchPolicy->LockDownConfig->BiosLock = mSystemConfiguration.SpiRwProtect;

  //
  // DeviceEnables
  //
  DxePlatformPchPolicy->DeviceEnabling->Lan = mSystemConfiguration.Lan;
  DxePlatformPchPolicy->DeviceEnabling->Azalia        = mSystemConfiguration.PchAzalia;
  DxePlatformPchPolicy->DeviceEnabling->Sata          = mSystemConfiguration.Sata;
  DxePlatformPchPolicy->DeviceEnabling->Smbus         = PCH_DEVICE_ENABLE;
  DxePlatformPchPolicy->DeviceEnabling->LpeEnabled    = mSystemConfiguration.Lpe;

  DxePlatformPchPolicy->UsbConfig->Ehci1Usbr          = PCH_DEVICE_DISABLE;

  DxePlatformPchPolicy->UsbConfig->UsbXhciLpmSupport =mSystemConfiguration.UsbXhciLpmSupport;

  //
  // Disable FFRD PR0 USB port2 for power saving since PR0 uses non-POR WWAN (but enable on PR0.3/PR0.5/PR1)
  //
  if ((PlatformInfo->BoardId == BOARD_ID_BL_FFRD) && (PlatformInfo->BoardRev == PR0))
    if (mSystemConfiguration.PchUsbPort[2] !=0) {
      mSystemConfiguration.PchUsbPort[2]=0;
      ModifyVariable = TRUE;
    }


  if (ModifyVariable) {
    Status = gRT->SetVariable (
                    NORMAL_SETUP_NAME,
                    &gEfiNormalSetupGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof(SYSTEM_CONFIGURATION),
                    &mSystemConfiguration
                    );
  }

  SocStepping = PchStepping();
  if (mSystemConfiguration.UsbAutoMode == 1) {   // auto mode is enabled
    if (PchA0 == SocStepping) {
    	//
      //  For A0, EHCI is enabled as default.
      //
      mSystemConfiguration.PchUsb20       = 1;
      mSystemConfiguration.PchUsb30Mode   = 0;
      mSystemConfiguration.UsbXhciSupport = 0;
      DEBUG ((EFI_D_INFO, "EHCI is enabled as default. SOC 0x%x\n", SocStepping));
    } else {
    	//
      //  For A1 and later, XHCI is enabled as default.
      //
      mSystemConfiguration.PchUsb20       = 0;
      mSystemConfiguration.PchUsb30Mode   = 1;
      mSystemConfiguration.UsbXhciSupport = 1;
      DEBUG ((EFI_D_INFO, "XHCI is enabled as default. SOC 0x%x\n", SocStepping));
    }
    //
    //overwrite the setting
    //
    Status = gRT->SetVariable(
                    NORMAL_SETUP_NAME,
                    &gEfiNormalSetupGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof(SYSTEM_CONFIGURATION),
                    &mSystemConfiguration
                    );
  }

  //
  // USB Device 29 configuration
  //
  DxePlatformPchPolicy->UsbConfig->Usb20Settings[0].Enable = mSystemConfiguration.PchUsb20;
  DxePlatformPchPolicy->UsbConfig->UsbPerPortCtl           = mSystemConfiguration.PchUsbPerPortCtl;
  if (mSystemConfiguration.PchUsbPerPortCtl != PCH_DEVICE_DISABLE) {
    for (PortIndex = 0; PortIndex < PCH_USB_MAX_PHYSICAL_PORTS; PortIndex++) {
      DxePlatformPchPolicy->UsbConfig->PortSettings[PortIndex].Enable = mSystemConfiguration.PchUsbPort[PortIndex];
    }
  }

  DxePlatformPchPolicy->UsbConfig->EhciDebug               = mSystemConfiguration.PchEhciDebug;

  //
  // xHCI (USB 3.0) related settings from setup variable
  //
  DxePlatformPchPolicy->UsbConfig->Usb30Settings.XhciStreams    = mSystemConfiguration.PchUsb30Streams;

  DxePlatformPchPolicy->UsbConfig->Usb30Settings.Mode           = mSystemConfiguration.PchUsb30Mode;

  //
  // Remove XHCI Pre-Boot Driver setup option selection from end-user view and automate loading of USB 3.0 BIOS driver based on XhciMode selection
  //
  switch (mSystemConfiguration.PchUsb30Mode) {
    case 0: // Disabled
      DxePlatformPchPolicy->UsbConfig->Usb30Settings.PreBootSupport = 0;
      break;
    case 1: // Enabled
      DxePlatformPchPolicy->UsbConfig->Usb30Settings.PreBootSupport = 1;
      break;
    case 2: // Auto
      DxePlatformPchPolicy->UsbConfig->Usb30Settings.PreBootSupport = 0;
      break;
    case 3: // Smart Auto
      DxePlatformPchPolicy->UsbConfig->Usb30Settings.PreBootSupport = 1;
      break;
    default:
      DxePlatformPchPolicy->UsbConfig->Usb30Settings.PreBootSupport = mSystemConfiguration.UsbXhciSupport;
      break;
  }



  DxePlatformPchPolicy->UsbConfig->UsbOtgSettings.Enable  = mSystemConfiguration.PchUsbOtg;

  DxePlatformPchPolicy->UsbConfig->PortSettings[0].Dock   = PCH_DEVICE_DISABLE;
  DxePlatformPchPolicy->UsbConfig->PortSettings[1].Dock   = PCH_DEVICE_DISABLE;
  DxePlatformPchPolicy->UsbConfig->PortSettings[2].Dock   = PCH_DEVICE_DISABLE;
  DxePlatformPchPolicy->UsbConfig->PortSettings[3].Dock   = PCH_DEVICE_DISABLE;

  DxePlatformPchPolicy->UsbConfig->PortSettings[0].Panel  = PCH_USB_FRONT_PANEL;
  DxePlatformPchPolicy->UsbConfig->PortSettings[1].Panel  = PCH_USB_FRONT_PANEL;
  DxePlatformPchPolicy->UsbConfig->PortSettings[2].Panel  = PCH_USB_BACK_PANEL;
  DxePlatformPchPolicy->UsbConfig->PortSettings[3].Panel  = PCH_USB_BACK_PANEL;

  //
  //
  // Enable USB Topology control and program the topology setting for every USB port
  // See Platform Design Guide for description of topologies
  //
  //
  // Port 0: ~5.3", Port 1: ~4.9", Port 2: ~4.7", Port 3: ~8.0"
  //
  DxePlatformPchPolicy->UsbConfig->Usb20PortLength[0]  = 0x53;
  DxePlatformPchPolicy->UsbConfig->Usb20PortLength[1]  = 0x49;
  DxePlatformPchPolicy->UsbConfig->Usb20PortLength[2]  = 0x47;
  DxePlatformPchPolicy->UsbConfig->Usb20PortLength[3]  = 0x80;

  DxePlatformPchPolicy->UsbConfig->Usb20OverCurrentPins[0]  = PchUsbOverCurrentPin0;
  DxePlatformPchPolicy->UsbConfig->Usb20OverCurrentPins[1]  = PchUsbOverCurrentPin0;
  DxePlatformPchPolicy->UsbConfig->Usb20OverCurrentPins[2]  = PchUsbOverCurrentPin1;
  DxePlatformPchPolicy->UsbConfig->Usb20OverCurrentPins[3]  = PchUsbOverCurrentPin1;

  DxePlatformPchPolicy->UsbConfig->Usb30OverCurrentPins[0]  = PchUsbOverCurrentPinSkip;//PchUsbOverCurrentPin0;

  DxePlatformPchPolicy->EhciPllCfgEnable = mSystemConfiguration.EhciPllCfgEnable;
  DEBUG ((EFI_D_INFO, "InitPchPlatformPolicy() DxePlatformPchPolicy->EhciPllCfgEnable = 0x%x \n",DxePlatformPchPolicy->EhciPllCfgEnable));
    DxePlatformPchPolicy->PciExpressConfig->PcieDynamicGating                                 = mSystemConfiguration.PcieDynamicGating;
  for (PortIndex = 0; PortIndex < PCH_PCIE_MAX_ROOT_PORTS; PortIndex++) {
    DxePlatformPchPolicy->PciExpressConfig->RootPort[PortIndex].Enable                        = mSystemConfiguration.IchPciExp[PortIndex];
    DxePlatformPchPolicy->PciExpressConfig->RootPort[PortIndex].SlotImplemented               = PCH_DEVICE_ENABLE;
    DxePlatformPchPolicy->PciExpressConfig->RootPort[PortIndex].FunctionNumber                = PortIndex;
    DxePlatformPchPolicy->PciExpressConfig->RootPort[PortIndex].PhysicalSlotNumber            = PortIndex;
    DxePlatformPchPolicy->PciExpressConfig->RootPort[PortIndex].Aspm                          = 4;
    DxePlatformPchPolicy->PciExpressConfig->RootPort[PortIndex].PmSci                         = PCH_DEVICE_DISABLE;
    DxePlatformPchPolicy->PciExpressConfig->RootPort[PortIndex].ExtSync                       = PCH_DEVICE_DISABLE;
    DxePlatformPchPolicy->PciExpressConfig->RootPort[PortIndex].HotPlug                       = PCH_DEVICE_DISABLE;
    DxePlatformPchPolicy->PciExpressConfig->RootPort[PortIndex].AdvancedErrorReporting        = PCH_DEVICE_DISABLE;
    DxePlatformPchPolicy->PciExpressConfig->RootPort[PortIndex].UnsupportedRequestReport      = PCH_DEVICE_DISABLE;
    DxePlatformPchPolicy->PciExpressConfig->RootPort[PortIndex].FatalErrorReport              = PCH_DEVICE_DISABLE;
    DxePlatformPchPolicy->PciExpressConfig->RootPort[PortIndex].NoFatalErrorReport            = PCH_DEVICE_DISABLE;
    DxePlatformPchPolicy->PciExpressConfig->RootPort[PortIndex].CorrectableErrorReport        = PCH_DEVICE_DISABLE;
    DxePlatformPchPolicy->PciExpressConfig->RootPort[PortIndex].PmeInterrupt                  = 0;
    DxePlatformPchPolicy->PciExpressConfig->RootPort[PortIndex].SystemErrorOnFatalError       = PCH_DEVICE_DISABLE;
    DxePlatformPchPolicy->PciExpressConfig->RootPort[PortIndex].SystemErrorOnNonFatalError    = PCH_DEVICE_DISABLE;
    DxePlatformPchPolicy->PciExpressConfig->RootPort[PortIndex].SystemErrorOnCorrectableError = PCH_DEVICE_DISABLE;
    DxePlatformPchPolicy->PciExpressConfig->RootPort[PortIndex].CompletionTimeout             = PchPciECompletionTO_Default;
  }

  //
  // SATA configuration
  //
  for (PortIndex = 0; PortIndex < PCH_AHCI_MAX_PORTS; PortIndex++) {
    if (mSystemConfiguration.SataType == 0) {
      DxePlatformPchPolicy->SataConfig->PortSettings[PortIndex].Enable   = PCH_DEVICE_ENABLE;
      DxePlatformPchPolicy->SataConfig->LegacyMode                       = PCH_DEVICE_ENABLE;
    } else {
      DxePlatformPchPolicy->SataConfig->PortSettings[PortIndex].Enable   = PCH_DEVICE_ENABLE;
      DxePlatformPchPolicy->SataConfig->LegacyMode                       = PCH_DEVICE_DISABLE;
    }
    if(mSystemConfiguration.Sata == 1){
      DxePlatformPchPolicy->SataConfig->PortSettings[PortIndex].Enable   = PCH_DEVICE_ENABLE;
    } else {
      DxePlatformPchPolicy->SataConfig->PortSettings[PortIndex].Enable   = PCH_DEVICE_DISABLE;
    }
    if(0 == PortIndex){
      DxePlatformPchPolicy->SataConfig->PortSettings[PortIndex].HotPlug    = PCH_DEVICE_DISABLE;
    } else if(1 == PortIndex){
      DxePlatformPchPolicy->SataConfig->PortSettings[PortIndex].HotPlug    = PCH_DEVICE_DISABLE;
    }

    DxePlatformPchPolicy->SataConfig->PortSettings[PortIndex].SpinUp     = PCH_DEVICE_DISABLE;
    DxePlatformPchPolicy->SataConfig->PortSettings[PortIndex].MechSw     = PCH_DEVICE_DISABLE;
  }
  DxePlatformPchPolicy->SataConfig->RaidAlternateId                 = PCH_DEVICE_DISABLE;
  DxePlatformPchPolicy->SataConfig->Raid0                           = PCH_DEVICE_ENABLE;
  DxePlatformPchPolicy->SataConfig->Raid1                           = PCH_DEVICE_ENABLE;
  DxePlatformPchPolicy->SataConfig->Raid10                          = PCH_DEVICE_ENABLE;
  DxePlatformPchPolicy->SataConfig->Raid5                           = PCH_DEVICE_ENABLE;
  DxePlatformPchPolicy->SataConfig->Irrt                            = PCH_DEVICE_ENABLE;
  DxePlatformPchPolicy->SataConfig->OromUiBanner                    = PCH_DEVICE_ENABLE;
  DxePlatformPchPolicy->SataConfig->HddUnlock                       = PCH_DEVICE_ENABLE;
  DxePlatformPchPolicy->SataConfig->LedLocate                       = PCH_DEVICE_ENABLE;
  DxePlatformPchPolicy->SataConfig->IrrtOnly                        = PCH_DEVICE_ENABLE;
  DxePlatformPchPolicy->SataConfig->SalpSupport                     = PCH_DEVICE_ENABLE;
  DxePlatformPchPolicy->SataConfig->TestMode                        = mSystemConfiguration.SataTestMode;

  //
  // AzaliaConfig
  //
  DxePlatformPchPolicy->AzaliaConfig->Pme       = mSystemConfiguration.AzaliaPme;
  DxePlatformPchPolicy->AzaliaConfig->HdmiCodec = mSystemConfiguration.HdmiCodec;
  DxePlatformPchPolicy->AzaliaConfig->DS        = mSystemConfiguration.AzaliaDs;
  DxePlatformPchPolicy->AzaliaConfig->AzaliaVCi = mSystemConfiguration.AzaliaVCiEnable;

  //
  // Set LPSS configuration according to setup value.
  //
  DxePlatformPchPolicy->LpssConfig->LpssPciModeEnabled   = mSystemConfiguration.LpssPciModeEnabled;

  DxePlatformPchPolicy->LpssConfig->Dma1Enabled    = mSystemConfiguration.LpssDma1Enabled;
  DxePlatformPchPolicy->LpssConfig->I2C0Enabled    = mSystemConfiguration.LpssI2C0Enabled;
  DxePlatformPchPolicy->LpssConfig->I2C1Enabled    = mSystemConfiguration.LpssI2C1Enabled;
  DxePlatformPchPolicy->LpssConfig->I2C2Enabled    = mSystemConfiguration.LpssI2C2Enabled;
  DxePlatformPchPolicy->LpssConfig->I2C3Enabled    = mSystemConfiguration.LpssI2C3Enabled;
  DxePlatformPchPolicy->LpssConfig->I2C4Enabled    = mSystemConfiguration.LpssI2C4Enabled;
  DxePlatformPchPolicy->LpssConfig->I2C5Enabled    = mSystemConfiguration.LpssI2C5Enabled;
  DxePlatformPchPolicy->LpssConfig->I2C6Enabled    = mSystemConfiguration.LpssI2C6Enabled;

  DxePlatformPchPolicy->LpssConfig->Dma0Enabled    = mSystemConfiguration.LpssDma0Enabled;;
  DxePlatformPchPolicy->LpssConfig->Pwm0Enabled    = mSystemConfiguration.LpssPwm0Enabled;
  DxePlatformPchPolicy->LpssConfig->Pwm1Enabled    = mSystemConfiguration.LpssPwm1Enabled;
  DxePlatformPchPolicy->LpssConfig->Hsuart0Enabled = mSystemConfiguration.LpssHsuart0Enabled;
  DxePlatformPchPolicy->LpssConfig->Hsuart1Enabled = mSystemConfiguration.LpssHsuart1Enabled;
  DxePlatformPchPolicy->LpssConfig->SpiEnabled     = mSystemConfiguration.LpssSpiEnabled;

  //
  // Set SCC configuration according to setup value.
  //
  DxePlatformPchPolicy->SccConfig->SdioEnabled   = mSystemConfiguration.LpssSdioEnabled;
  DxePlatformPchPolicy->SccConfig->SdcardEnabled = TRUE;
  DxePlatformPchPolicy->SccConfig->SdCardSDR25Enabled = mSystemConfiguration.LpssSdCardSDR25Enabled;
  DxePlatformPchPolicy->SccConfig->SdCardDDR50Enabled = mSystemConfiguration.LpssSdCardDDR50Enabled;
  DxePlatformPchPolicy->SccConfig->HsiEnabled    = mSystemConfiguration.LpssMipiHsi;

  if (mSystemConfiguration.eMMCBootMode== 1) {// Auto detection mode
  //
  // Silicon Stepping
  //
  switch (PchStepping()) {
    case PchA0: // A0 and A1
    case PchA1:
      DEBUG ((EFI_D_ERROR, "Auto Detect: SOC A0/A1: SCC eMMC 4.41 Configuration\n"));
      DxePlatformPchPolicy->SccConfig->eMMCEnabled            = 1;
      DxePlatformPchPolicy->SccConfig->eMMC45Enabled          = 0;
      DxePlatformPchPolicy->SccConfig->eMMC45DDR50Enabled     = 0;
      DxePlatformPchPolicy->SccConfig->eMMC45HS200Enabled     = 0;
      DxePlatformPchPolicy->SccConfig->eMMC45RetuneTimerValue = 0;
      break;
    case PchB0: // B0 and later
    default:
      DEBUG ((EFI_D_ERROR, "Auto Detect: SOC B0 and later: SCC eMMC 4.5 Configuration\n"));
      DxePlatformPchPolicy->SccConfig->eMMCEnabled            = 0;
      DxePlatformPchPolicy->SccConfig->eMMC45Enabled          = mSystemConfiguration.LpsseMMC45Enabled;
      DxePlatformPchPolicy->SccConfig->eMMC45DDR50Enabled     = mSystemConfiguration.LpsseMMC45DDR50Enabled;
      DxePlatformPchPolicy->SccConfig->eMMC45HS200Enabled     = mSystemConfiguration.LpsseMMC45HS200Enabled;
      DxePlatformPchPolicy->SccConfig->eMMC45RetuneTimerValue = mSystemConfiguration.LpsseMMC45RetuneTimerValue;
      break;
  }
 } else if (mSystemConfiguration.eMMCBootMode == 2) { // eMMC 4.41
    DEBUG ((EFI_D_ERROR, "Force to SCC eMMC 4.41 Configuration\n"));
    DxePlatformPchPolicy->SccConfig->eMMCEnabled            = 1;
    DxePlatformPchPolicy->SccConfig->eMMC45Enabled          = 0;
    DxePlatformPchPolicy->SccConfig->eMMC45DDR50Enabled     = 0;
    DxePlatformPchPolicy->SccConfig->eMMC45HS200Enabled     = 0;
    DxePlatformPchPolicy->SccConfig->eMMC45RetuneTimerValue = 0;

 } else if (mSystemConfiguration.eMMCBootMode == 3) { // eMMC 4.5
      DEBUG ((EFI_D_ERROR, "Force to eMMC 4.5 Configuration\n"));
      DxePlatformPchPolicy->SccConfig->eMMCEnabled            = 0;
      DxePlatformPchPolicy->SccConfig->eMMC45Enabled          = mSystemConfiguration.LpsseMMC45Enabled;
      DxePlatformPchPolicy->SccConfig->eMMC45DDR50Enabled     = mSystemConfiguration.LpsseMMC45DDR50Enabled;
      DxePlatformPchPolicy->SccConfig->eMMC45HS200Enabled     = mSystemConfiguration.LpsseMMC45HS200Enabled;
      DxePlatformPchPolicy->SccConfig->eMMC45RetuneTimerValue = mSystemConfiguration.LpsseMMC45RetuneTimerValue;

 } else { // Disable eMMC controllers
      DEBUG ((EFI_D_ERROR, "Disable eMMC controllers\n"));
      DxePlatformPchPolicy->SccConfig->eMMCEnabled            = 0;
      DxePlatformPchPolicy->SccConfig->eMMC45Enabled          = 0;
      DxePlatformPchPolicy->SccConfig->eMMC45DDR50Enabled     = 0;
      DxePlatformPchPolicy->SccConfig->eMMC45HS200Enabled     = 0;
      DxePlatformPchPolicy->SccConfig->eMMC45RetuneTimerValue = 0;
 }

  //
  // Reserved SMBus Address
  //
  DxePlatformPchPolicy->SmbusConfig->NumRsvdSmbusAddresses  = 4;
  DxePlatformPchPolicy->SmbusConfig->RsvdSmbusAddressTable  = mSmbusRsvdAddresses;

  //
  // MiscPm Configuration
  //
  DxePlatformPchPolicy->MiscPmConfig->WakeConfig.WolEnableOverride        = mSystemConfiguration.WakeOnLanS5;
  DxePlatformPchPolicy->MiscPmConfig->SlpLanLowDc                         = mSystemConfiguration.SlpLanLowDc;
  DxePlatformPchPolicy->MiscPmConfig->PowerResetStatusClear.MeWakeSts     = PCH_DEVICE_ENABLE;
  DxePlatformPchPolicy->MiscPmConfig->PowerResetStatusClear.MeHrstColdSts = PCH_DEVICE_ENABLE;
  DxePlatformPchPolicy->MiscPmConfig->PowerResetStatusClear.MeHrstWarmSts = PCH_DEVICE_ENABLE;

  //
  // Enable / disable serial IRQ according to setup value.
  //
  DxePlatformPchPolicy->SerialIrqConfig->SirqEnable = PCH_DEVICE_ENABLE;

  //
  // Set Serial IRQ Mode Select according to setup value.
  //
  DxePlatformPchPolicy->SerialIrqConfig->SirqMode = PchQuietMode;

  //
  // Program the default Sub System Vendor Device Id
  //
  DxePlatformPchPolicy->DefaultSvidSid->SubSystemVendorId = V_PCH_INTEL_VENDOR_ID;
  DxePlatformPchPolicy->DefaultSvidSid->SubSystemId       = V_PCH_DEFAULT_SID;

  mAzaliaVerbTable[9].VerbTableData = mAzaliaVerbTableData12;

  DxePlatformPchPolicy->AzaliaConfig->AzaliaVerbTableNum  = sizeof (mAzaliaVerbTable) / sizeof (PCH_AZALIA_VERB_TABLE);
  DxePlatformPchPolicy->AzaliaConfig->AzaliaVerbTable     = mAzaliaVerbTable;
  DxePlatformPchPolicy->AzaliaConfig->ResetWaitTimer      = 300;

  DxePlatformPchPolicy->IdleReserve = mSystemConfiguration.IdleReserve;
  DxePlatformPchPolicy->AcpiHWRed = PCH_DEVICE_DISABLE;

  //
  // Install DxePchPolicyUpdateProtocol
  //
  Handle                        = NULL;

  mDxePchPolicyUpdate.Revision  = DXE_PCH_POLICY_UPDATE_PROTOCOL_REVISION_1;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gDxePchPolicyUpdateProtocolGuid,
                  &mDxePchPolicyUpdate,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  DEBUG ((EFI_D_INFO, "InitPchPlatformPolicy() - End\n"));
}


DXE_VLV_PLATFORM_POLICY_PROTOCOL mDxePlatformVlvPolicy;

VOID
InitVlvPlatformPolicy (
  )
{
  DXE_VLV_PLATFORM_POLICY_PROTOCOL *DxePlatformVlvPolicy;
  EFI_STATUS                      Status;
  EFI_HANDLE                      Handle;

  ZeroMem (&mDxePlatformVlvPolicy, sizeof(DXE_VLV_PLATFORM_POLICY_PROTOCOL));

  DxePlatformVlvPolicy = &mDxePlatformVlvPolicy;


  DxePlatformVlvPolicy->GraphicReserve00 = mSystemConfiguration.GraphicReserve00;
  DxePlatformVlvPolicy->PavpMode = mSystemConfiguration.PavpMode;
  DxePlatformVlvPolicy->GraphicReserve01 = 1;
  DxePlatformVlvPolicy->GraphicReserve02 = mSystemConfiguration.GraphicReserve02;
  DxePlatformVlvPolicy->GraphicReserve03 = 1;
  DxePlatformVlvPolicy->GraphicReserve04 = 0;
  DxePlatformVlvPolicy->GraphicReserve05 = mSystemConfiguration.GraphicReserve05;
  DxePlatformVlvPolicy->IgdPanelFeatures.PFITStatus = mSystemConfiguration.PanelScaling;
  
  DxePlatformVlvPolicy->IgdPanelFeatures.LidStatus = 1;
  DxePlatformVlvPolicy->IdleReserve = mSystemConfiguration.IdleReserve;

  DxePlatformVlvPolicy->GraphicReserve06 = 1;

  if ( (mSystemConfiguration.Lpe == 1) || mSystemConfiguration.Lpe == 2) {
    DxePlatformVlvPolicy ->AudioTypeSupport = LPE_AUDIO ;
  } else if ( mSystemConfiguration.PchAzalia == 1 ) {
    DxePlatformVlvPolicy ->AudioTypeSupport = HD_AUDIO;
  } else {
    DxePlatformVlvPolicy ->AudioTypeSupport = NO_AUDIO;
  }

  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gDxeVlvPlatformPolicyGuid,
                  EFI_NATIVE_INTERFACE,
                  DxePlatformVlvPolicy
                  );
  ASSERT_EFI_ERROR(Status);

}
