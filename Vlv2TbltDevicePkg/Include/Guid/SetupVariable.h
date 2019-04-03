/** @file

  Copyright (c) 2004  - 2018, Intel Corporation. All rights reserved.<BR>


  SPDX-License-Identifier: BSD-2-Clause-Patent




Module Name:

    SetupVariable.h

Abstract:

    Driver configuration include file


**/

#ifndef _SETUP_VARIABLE_H
#define _SETUP_VARIABLE_H

//
// ---------------------------------------------------------------------------
//
// Driver Configuration
//
// ---------------------------------------------------------------------------
//

//
// {EC87D643-EBA4-4bb5-A1E5-3F3E36B20DA9}
//
#define SYSTEM_CONFIGURATION_GUID\
  { \
    0xec87d643, 0xeba4, 0x4bb5, 0xa1, 0xe5, 0x3f, 0x3e, 0x36, 0xb2, 0xd, 0xa9 \
  }

#define ROOT_SECURITY_GUID\
  { \
    0xd387d688, 0xeba4, 0x45b5, 0xa1, 0xe5, 0x3f, 0x3e, 0x36, 0xb2, 0xd, 0x37 \
  }

//
// {6936B3BD-4350-46d9-8940-1FA20961AEB1}
//
#define SYSTEM_ROOT_MAIN_GUID\
  { \
     0x6936b3bd, 0x4350, 0x46d9, 0x89, 0x40, 0x1f, 0xa2, 0x9, 0x61, 0xae, 0xb1 \
  }

//
// {21FEE8DB-0D29-477e-B5A9-96EB343BA99C}
//
#define ADDITIONAL_SYSTEM_INFO_GUID\
  { \
     0x21fee8db, 0xd29, 0x477e, 0xb5, 0xa9, 0x96, 0xeb, 0x34, 0x3b, 0xa9, 0x9c \
  }

#define SETUP_GUID { 0xEC87D643, 0xEBA4, 0x4BB5, 0xA1, 0xE5, 0x3F, 0x3E, 0x36, 0xB2, 0x0D, 0xA9 }

// {1B838190-4625-4ead-ABC9-CD5E6AF18FE0}
#define EFI_HII_EXPORT_DATABASE_GUID { 0x1b838190, 0x4625, 0x4ead, 0xab, 0xc9, 0xcd, 0x5e, 0x6a, 0xf1, 0x8f, 0xe0 }

#define PASSWORD_MAX_SIZE               20

#define MAX_CUSTOM_VID_TABLE_STATES 6
//
// Overclocking Source Defines
//
#define OVERCLOCK_SOURCE_BIOS       0
#define OVERCLOCK_SOURCE_OS         1

#define PCH_PCIE_MAX_ROOT_PORTS     4

#pragma pack(1)

// NOTE: When you add anything to this structure,
//   you MUST add it to the very bottom!!!!
//   You must make sure the structure size is able to divide by 32!
typedef struct {

  //
  // Floppy
  //
  UINT8         Floppy;
  UINT8         FloppyLockHide;

  UINT8         FloppyWriteProtect;
  UINT8         FloppyWriteProtectLockHide;

  //
  // System ports
  //
  UINT8         Serial;
  UINT8         SerialLockHide;

  UINT8         Serial2;
  UINT8         Serial2LockHide;

  UINT8         Parallel;
  UINT8         ParallelLockHide;

  UINT8         ParallelMode;
  UINT8         ParallelModeLockHide;

  UINT8         AllUsb;
  UINT8         UsbPortsLockHide;

  UINT8         Usb2;
  UINT8         Usb2LockHide;

  UINT8         UsbLegacy;
  UINT8         UsbLegacyLockHide;

  UINT8         Audio;
  UINT8         AudioLockHide;

  UINT8         Lan;
  UINT8         LanLockHide;

  //
  // Keyboard
  //
  UINT8         Numlock;
  UINT8         NumlockLockHide;

  //
  // ECIR
  //
  UINT8         ECIR;
  UINT8         ECIRLockHide;

  //
  // Power State
  //
  UINT8         PowerState;
  UINT8         PowerStateLockHide;

  //
  // Wake on RTC variables
  //
  UINT8         WakeOnRtcS5;
  UINT8         WakeOnRtcS5LockHide;
  UINT8         RTCWakeupDate;
  UINT8         RTCWakeupDateLockHide;
  UINT8         RTCWakeupTimeHour;
  UINT8         RTCWakeupHourLockHide;
  UINT8         RTCWakeupTimeMinute;
  UINT8         RTCWakeupMinuteLockHide;
  UINT8         RTCWakeupTimeSecond;
  UINT8         RTCWakeupSecondLockHide;

  //
  // Wake On Lan
  //
  UINT8         WakeOnLanS5;
  UINT8         WakeOnLanS5LockHide;

  //Spread spectrum
  UINT8         SpreadSpectrum;

  //
  // Boot Order
  //
  UINT8         BootOrder[8];
  UINT8         BootOrderLockHide;

  //
  // Hard Drive Boot Order
  //
  UINT8         HardDriveBootOrder[8];
  UINT8         HardDriveBootOrderLockHide;

  //
  // CD Drive Boot Order
  //
  UINT8         CdDriveBootOrder[4];
  UINT8         CdDriveBootOrderLockHide;

  //
  // FDD Drive Boot Order
  //
  UINT8         FddDriveBootOrder[4];
  UINT8         FddDriveBootOrderLockHide;

  //
  // Drive Boot Order
  //
  UINT8         DriveBootOrder[16];
  UINT8         DriveBootOrderLockHide;

  //
  // Boot Menu Type
  //
  UINT8         BootMenuType;
  UINT8         BootMenuTypeLockHide;

  //
  // Boot from Removable Devices
  //
  UINT8         BootFloppy;
  UINT8         BootFloppyLockHide;

  //
  // Boot from Optical Devices
  //
  UINT8         BootCd;
  UINT8         BootCdLockHide;

  //
  // Boot from Network
  //
  UINT8         BootNetwork;
  UINT8         BootNetworkLockHide;

  //
  // Boot USB
  //
  UINT8         BootUsb;
  UINT8         BootUsbLockHide;

  //
  // USB Zip Emulation Type
  //
  UINT8         UsbZipEmulation;
  UINT8         UsbZipEmulationLockHide;

  //
  // USB Devices Boot First in Boot Order
  //
  UINT8         UsbDevicesBootFirst;
  UINT8         UsbDevicesBootFirstLockHide;

  //
  // USB Boot Device SETUP Emulation
  //
  UINT8         UsbSetupDeviceEmulation;
  UINT8         UsbSetupDeviceEmulationLockHide;

  //
  // BIOS INT13 Emulation for USB Mass Devices
  //
  UINT8         UsbBIOSINT13DeviceEmulation;
  UINT8         UsbBIOSINT13DeviceEmulationLockHide;

  //
  // BIOS INT13 Emulation Size for USB Mass Devices
  //
  UINT16        UsbBIOSINT13DeviceEmulationSize;
  UINT8         UsbBIOSINT13DeviceEmulationSizeLockHide;

  //
  // Dummy place holder to prevent VFR compiler problem.
  //
  UINT16        DummyDataForVfrBug;  // Don't change or use.

  //
  // Language Select
  //
  UINT8         LanguageSelect;

  //
  // SATA Type (Ide, Ahci, Raid)
  //
  UINT8         SataType;
  UINT8         SataTypeLockHide;
  UINT8         SataTestMode;

  //
  // Fixed Disk Boot Sector (Fdbs)
  //
  UINT8         Fdbs;
  UINT8         FdbsLockHide;

  //
  // DisplaySetupPrompt
  //
  UINT8         DisplaySetupPrompt;
  UINT8         DisplaySetupPromptLockHide;

  //
  // ASF
  //
  UINT8         Asf;
  UINT8         AsfLockHide;

  //
  // Event Logging
  //
  UINT8         EventLogging;
  UINT8         EventLoggingLockHide;

  //
  // Clear Event Log
  //
  UINT8         ClearEvents;
  UINT8         ClearEventsLockHide;

  //
  // Expansion Card Text
  //
  UINT8         ExpansionCardText;
  UINT8         ExpansionCardTextLockHide;

  //
  // Video Adaptor
  //
  UINT8         PrimaryVideoAdaptor;
  UINT8         PrimaryVideoAdaptorLockHide;

  //
  // Chassis intrusion
  //
  UINT8         IntruderDetection;
  UINT8         IntruderDetectionLockHide;

  //
  // User Access Level
  //
  UINT8         UserPasswordLevel;
  UINT8         UserPasswordLevelLockHide;

  //
  // Maximum FSB Automatic/Disable
  //
  UINT8         MaxFsb;
  UINT8         MaxFsbLockHide;

  //
  // Hard Disk Pre-delay
  //
  UINT8         HddPredelay;
  UINT8         HddPredelayLockHide;

  //
  // S.M.A.R.T. Mode
  //
  UINT8         SmartMode;
  UINT8         SmartModeLockHide;

  //
  // ACPI Suspend State
  //
  UINT8         AcpiSuspendState;
  UINT8         AcpiSuspendStateLockHide;

  //
  // PCI Latency Timer
  //
  UINT8         PciLatency;
  UINT8         PciLatencyLockHide;

  //
  // Fan Control
  //
  UINT8         FanControl;
  UINT8         FanControlLockHide;

  //
  // CPU Fan Control
  //
  UINT8         CpuFanControl;
  UINT8         CpuFanControlLockHide;

  //
  // Lowest Fan Speed
  //
  UINT8         LowestFanSpeed;
  UINT8         LowestFanSpeedLockHide;

  //
  // Processor (CPU)
  //
  UINT8         CpuFlavor;

  UINT8         CpuidMaxValue;
  UINT8         CpuidMaxValueLockHide;

  UINT8         ExecuteDisableBit;
  UINT8         ExecuteDisableBitLockHide;

  //
  // EIST or GV3 setup option
  //
  UINT8         ProcessorEistEnable;
  UINT8         ProcessorEistEnableLockHide;

  //
  // C1E Enable
  //
  UINT8         ProcessorC1eEnable;
  UINT8         ProcessorC1eEnableLockHide;

  //
  // Enabling CPU C-States of processor
  //
  UINT8         ProcessorCcxEnable;
  UINT8         ProcessorCcxEnableLockHide;

  //
  // Package C-State Limit
  //
  UINT8         PackageCState;
  UINT8         PackageCStateLockHide;

  //
  // Enable/Disable NHM C3(ACPI C2) report to OS
  //
  UINT8         OSC2Report;
  UINT8         OSC2ReportLockHide;

  //
  // Enable/Disable NHM C6(ACPI C3) report to OS
  //
  UINT8         C6Enable;
  UINT8         C6EnableLockHide;

  //
  // Enable/Disable NHM C7(ACPI C3) report to OS
  //
  UINT8         C7Enable;
  UINT8         C7EnableLockHide;

  //
  // EIST/PSD Function select option
  //
  UINT8         ProcessorEistPsdFunc;
  UINT8         ProcessorEistPsdFuncLockHide;

  //
  //
  //
  UINT8         CPU00;
  UINT8         CPU01;

  //
  //
  //
  UINT8         CPU02;
  UINT8         CPU03;

  //
  //
  //
  UINT8         CPU04;
  UINT8         CPU05;

  //
  //
  //
  UINT8         CPU06;
  UINT8         CPU07;

  //
  //
  //
  UINT8         CPU08;
  UINT8         CPU09;

  //
  //
  //
  UINT8         CPU10;
  UINT8         CPU11;

  //
  //
  //
  UINT8         CPU12;
  UINT8         CPU13;

  //
  //
  //
  UINT8         CPU14;
  UINT8         CPU15;

  //
  //
  //
  UINT8         CPU16;
  UINT8         CPU17;

  //
  //
  //
  UINT8         CPU18;
  UINT8         CPU19;

  //
  //
  //
  UINT8         CPU20;
  UINT8         CPU21;

  //
  //
  //
  UINT8         CPU22;
  UINT8         CPU23;

  //
  //
  //
  UINT8         CPU24;
  UINT8         CPU25;

  //
  //
  //
  UINT8         CPU26;
  UINT8         CPU27;

  //
  //
  //
  UINT8         CPU28;
  UINT8         CPU29;

  //
  //
  //
  UINT8         CPU30;
  UINT8         CPU31;

  //
  //
  //
  UINT8         CPU32;
  UINT8         CPU33;

  //
  //
  //
  UINT8         CPU34;
  UINT8         CPU35;

  //
  //
  //
  UINT8         CPU36;
  UINT8         CPU37;

  //
  //
  //
  UINT8         CPU38;
  UINT8         CPU39;

  //
  //
  //
  UINT16        CPU40;
  UINT8         CPU41;

  //
  //
  //
  UINT8         CPU42;
  UINT8         CPU43;

  //
  //
  //
  UINT16        CPU44;
  UINT8         CPU45;

  //
  //
  //
  UINT8         CPU46;
  UINT8         CPU47;

  //
  //
  //
  UINT8         CPU48;
  UINT8         CPU49;

  //
  //
  //
  UINT8         CPU50;
  UINT8         CPU51;

  //
  //
  //
  UINT8         CPU52;
  UINT8         CPU53;

  //
  //
  //
  UINT8         CPU54;
  UINT8         CPU55;

  //
  //
  //
  UINT8         CPU56;
  UINT8         CPU57;

  //
  //
  //
  UINT8         CPU58;
  UINT8         CPU59;

  //
  //
  //
  UINT8         CPU60;
  UINT8         CPU61;

  //
  //
  //
  UINT8         CPU62;
  UINT8         CPU63;

  //
  //
  //
  UINT8         CPU64;
  UINT8         CPU65;

  //
  //
  //
  UINT8         CPU66;
  UINT8         CPU67;

  //
  //
  //
  UINT16        CPU68;
  UINT8         CPU69;

  //
  //
  //
  UINT16        CPU70;

  //
  //
  //
  UINT8         CPU71;

  //
  //
  //
  UINT8         MEM00;
  UINT8         MEM01;

  //
  //
  //
  UINT8         MEM02;
  UINT8         MEM03;

  UINT16        MEM04;
  UINT8         MEM05;

  UINT8         MEM06;
  UINT8         MEM07;

  UINT8         MEM08;
  UINT8         MEM09;

  UINT8         MEM10;
  UINT8         MEM11;

  UINT8         MEM12;
  UINT8         MEM13;

  UINT8         MEM14;
  UINT8         MEM15;

  UINT8         MEM16;
  UINT8         MEM17;

  UINT16        MEM18;
  UINT8         MEM19;

  UINT8         MEM20;
  UINT8         MEM21;

  UINT8         MEM22;
  UINT8         MEM23;

  UINT8         MEM24;
  UINT8         MEM25;

  UINT8         MEM26;
  UINT8         MEM27;

  UINT8         MEM28;
  UINT8         MEM29;

  UINT8         MEM30;
  UINT8         MEM31;

  UINT8         MEM32;
  UINT8         MEM33;

  UINT8         MEM34;
  UINT8         MEM35;

  //
  //
  //
  UINT8         MEM36;
  UINT8         MEM37;
  UINT8         MEM38;
  UINT8         MEM39;

  //
  //
  //
  UINT8         MEM40;
  UINT8         MEM41;
  UINT8         MEM42;
  UINT8         MEM43;
  UINT8         MEM44;
  UINT8         MEM45;
  UINT8         MEM46;
  UINT8         MEM47;


  //
  // Port 80 decode 0/1 - PCI/LPC
  UINT8         Port80Route;
  UINT8         Port80RouteLockHide;

  //
  // ECC Event Logging
  //
  UINT8         EccEventLogging;
  UINT8         EccEventLoggingLockHide;

  //
  // TPM Enable/Disable
  //
  UINT8         ETpm;

  //
  // TPM question  0 = Disabled, 1 = Enabled
  //
  UINT8         ETpmClear;

  //
  // Secondary SATA Controller question  0 = Disabled, 1 = Enabled
  //
  UINT8         ExtSata;
  UINT8         ExtSataLockHide;

  //
  // Mode selection for Secondary SATA Controller (0=IDE, 1=RAID)
  //
  UINT8         ExtSataMode;
  UINT8         ExtSataModeLockHide;

  //
  // LT Technology 0/1 -> Disable/Enable
  //
  UINT8         LtTechnology;
  UINT8         LtTechnologyLockHide;

  //
  // HPET Support 0/1 -> Disable/Enable
  //
  UINT8         Hpet;
  UINT8         HpetLockHide;

  //
  // ICH Function Level Reset enable/disable
  //
  UINT8         FlrCapability;
  UINT8         FlrCapabilityLockHide;

  // VT-d Option
  UINT8         VTdSupport;
  UINT8         VTdSupportLockHide;

  UINT8         InterruptRemap;
  UINT8         InterruptRemapLockHide;

  UINT8         Isoc;
  UINT8         IsocLockHide;

  UINT8         CoherencySupport;
  UINT8         CoherencySupportLockHide;

  UINT8         ATS;
  UINT8         ATSLockHide;

  UINT8         PassThroughDma;
  UINT8         PassThroughDmaLockHide;

  //
  // IGD option
  //
  UINT8         GraphicsDriverMemorySize;
  UINT8         GraphicsDriverMemorySizeLockHide;


  //
  // Discrete SATA Type (Ide, Raid, Ahci)
  //
  UINT8         ExtSataMode2;
  UINT8         ExtSataMode2LockHide;

  UINT8         ProcessorReserve00;
  UINT8         ProcessorReserve01;

  //
  // IGD Aperture Size question
  //
  UINT8         IgdApertureSize;
  UINT8         IgdApertureSizeLockHide;

  //
  // Boot Display Device
  //
  UINT8         BootDisplayDevice;
  UINT8         BootDisplayDeviceLockHide;


  //
  // System fan speed duty cycle
  //
  UINT8         SystemFanDuty;
  UINT8         SystemFanDutyLockHide;


  //
  // S3 state LED indicator
  //
  UINT8         S3StateIndicator;
  UINT8         S3StateIndicatorLockHide;

  //
  // S1 state LED indicator
  //
  UINT8         S1StateIndicator;
  UINT8         S1StateIndicatorLockHide;

  //
  // PS/2 Wake from S5
  //
  UINT8         WakeOnS5Keyboard;
  UINT8         WakeOnS5KeyboardLockHide;


  //
  // SATA Controller question  0 = Disabled, 1 = Enabled
  //
  UINT8         Sata;
  UINT8         SataLockHide;

  //
  // PS2 port
  //
  UINT8         PS2;

  //
  // No VideoBeep
  //
  UINT8         NoVideoBeepEnable;

  //
  // Integrated Graphics Device
  //
  UINT8         Igd;

  //
  // Video Device select order
  //
  UINT8         VideoSelectOrder[8];

  // Flash update sleep delay
  UINT8         FlashSleepDelay;
  UINT8         FlashSleepDelayLockHide;

  //
  // Boot Display Device2
  //
  UINT8         BootDisplayDevice2;
  UINT8         BootDisplayDevice2LockHide;

  //
  // Flat Panel
  //
  UINT8         EdpInterfaceType;
  UINT8         EdpInterfaceTypeLockHide;

  UINT8         LvdsInterfaceType;
  UINT8         LvdsInterfaceTypeLockHide;

  UINT8         ColorDepth;
  UINT8         ColorDepthLockHide;

  UINT8         EdidConfiguration;
  UINT8         EdidConfigurationLockHide;

  UINT8         PwmReserved;
  UINT8         MaxInverterPWMLockHide;

  UINT8         PreDefinedEdidConfiguration;
  UINT8         PreDefinedEdidConfigurationLockHide;

  UINT16        ScreenBrightnessResponseTime;
  UINT8         ScreenBrightnessResponseTimeLockHide;

  UINT8         Serial3;
  UINT8         Serial3LockHide;

  UINT8         Serial4;
  UINT8         Serial4LockHide;

  UINT8         CurrentSetupProfile;
  UINT8         CurrentSetupProfileLockHide;

  //
  // FSC system Variable
  //
  UINT8         CPUFanUsage;
  UINT8         CPUFanUsageLockHide;
  UINT16        CPUUnderSpeedthreshold;
  UINT8         CPUUnderSpeedthresholdLockHide;
  UINT8         CPUFanControlMode;
  UINT8         CPUFanControlModeLockHide;
  UINT16        Voltage12UnderVolts;
  UINT8         Voltage12UnderVoltsLockHide;
  UINT16        Voltage12OverVolts;
  UINT8         Voltage12OverVoltsLockHide;
  UINT16        Voltage5UnderVolts;
  UINT8         Voltage5UnderVoltsLockHide;
  UINT16        Voltage5OverVolts;
  UINT8         Voltage5OverVoltsLockHide;
  UINT16        Voltage3p3UnderVolts;
  UINT8         Voltage3p3UnderVoltsLockHide;
  UINT16        Voltage3p3OverVolts;
  UINT8         Voltage3p3OverVoltsLockHide;
  UINT16        Voltage2p5UnderVolts;
  UINT8         Voltage2p5UnderVoltsLockHide;
  UINT16        Voltage2p5OverVolts;
  UINT8         Voltage2p5OverVoltsLockHide;
  UINT16        VoltageVccpUnderVolts;
  UINT8         VoltageVccpUnderVoltsLockHide;
  UINT16        VoltageVccpOverVolts;
  UINT8         VoltageVccpOverVoltsLockHide;
  UINT16        Voltage5BackupUnderVolts;
  UINT8         Voltage5BackupUnderVoltsLockHide;
  UINT16        Voltage5BackupOverVolts;
  UINT8         Voltage5BackupOverVoltsLockHide;
  UINT16        VS3p3StbyUnderVolt;
  UINT8         VS3p3StbyUnderVoltLockHide;
  UINT16        VS3p3StbyOverVolt;
  UINT8         VS3p3StbyOverVoltLockHide;
  UINT8         CPUFanMinDutyCycle;
  UINT8         CPUFanMinDutyCycleLockHide;
  UINT8         CPUFanMaxDutyCycle;
  UINT8         CPUFanMaxDutyCycleLockHide;
  UINT8         CPUFanOnDutyCycle;
  UINT8         CPUFanOnDutyCycleLockHide;
  UINT16        CpuOverTemp;
  UINT8         CpuOverTempLockHide;
  UINT16        CpuControlTemp;
  UINT8         CpuControlTempLockHide;
  UINT16        CpuAllOnTemp;
  UINT8         CpuAllOnTempLockHide;
  UINT8         CpuResponsiveness;
  UINT8         CpuResponsivenessLockHide;
  UINT8         CpuDamping;
  UINT8         CpuDampingLockHide;
  UINT16        PchOverTemp;
  UINT8         PchOverTempLockHide;
  UINT16        PchControlTemp;
  UINT8         PchControlTempLockHide;
  UINT16        PchAllOnTemp;
  UINT8         PchAllOnTempLockHide;
  UINT8         PchResponsiveness;
  UINT8         PchResponsivenessLockHide;
  UINT8         PchDamping;
  UINT8         PchDampingLockHide;
  UINT16        MemoryOverTemp;
  UINT8         MemoryOverTempLockHide;
  UINT16        MemoryControlTemp;
  UINT8         MemoryControlTempLockHide;
  UINT16        MemoryAllOnTemp;
  UINT8         MemoryAllOnTempLockHide;
  UINT8         MemoryResponsiveness;
  UINT8         MemoryResponsivenessLockHide;
  UINT8         MemoryDamping;
  UINT8         MemoryDampingLockHide;
  UINT16        VROverTemp;
  UINT8         VROverTempLockHide;
  UINT16        VRControlTemp;
  UINT8         VRControlTempLockHide;
  UINT16        VRAllOnTemp;
  UINT8         VRAllOnTempLockHide;
  UINT8         VRResponsiveness;
  UINT8         VRResponsivenessLockHide;
  UINT8         VRDamping;
  UINT8         VRDampingLockHide;

  UINT8         LvdsBrightnessSteps;
  UINT8         LvdsBrightnessStepsLockHide;
  UINT8         EdpDataRate;
  UINT8         EdpDataRateLockHide;
  UINT16        LvdsPowerOnToBacklightEnableDelayTime;
  UINT8         LvdsPowerOnToBacklightEnableDelayTimeLockHide;
  UINT16        LvdsPowerOnDelayTime;
  UINT8         LvdsPowerOnDelayTimeLockHide;
  UINT16        LvdsBacklightOffToPowerDownDelayTime;
  UINT8         LvdsBacklightOffToPowerDownDelayTimeLockHide;
  UINT16        LvdsPowerDownDelayTime;
  UINT8         LvdsPowerDownDelayTimeLockHide;
  UINT16        LvdsPowerCycleDelayTime;
  UINT8         LvdsPowerCycleDelayTimeLockHide;

  UINT8         IgdFlatPanel;
  UINT8         IgdFlatPanelLockHide;
  UINT8         Lan2;
  UINT8         Lan2LockHide;

  UINT8         SwapMode;
  UINT8         SwapModeLockHide;

  UINT8         Sata0HotPlugCap;
  UINT8         Sata0HotPlugCapLockHide;
  UINT8         Sata1HotPlugCap;
  UINT8         Sata1HotPlugCapLockHide;

  UINT8         UsbCharging;
  UINT8         UsbChargingLockHide;

  UINT8         Cstates;
  UINT8         EnableC4;
  UINT8         EnableC6;

  UINT8          FastBoot;
  UINT8          EfiNetworkSupport;
  UINT8          PxeRom;

  //Add for PpmPlatformPlicy
  UINT8          PPM00;
  UINT8          PPM01;
  UINT8          PPM02;
  UINT8          PPM03;
  UINT8          PPM04;
  UINT8          PPM05;
  UINT8          PPM06;
  UINT8          PPM07;
  UINT8          PPM08;
  UINT8          PPM09;
  UINT8          PPM10;
  UINT8          QuietBoot;
  UINT8          LegacyUSBBooting;

  UINT8          PwmReserved02;
  //
  // Thermal Policy Values
  //
  UINT8           EnableDigitalThermalSensor;
  UINT8           PassiveThermalTripPoint;
  UINT8           PassiveTc1Value;
  UINT8           PassiveTc2Value;
  UINT8           PassiveTspValue;
  UINT8           DisableActiveTripPoints;
  UINT8           CriticalThermalTripPoint;
  UINT8           IchPciExp[4];
  UINT8           DeepStandby;
  UINT8           AlsEnable;
  UINT8           IgdLcdIBia;
  UINT8           LogBootTime;


  UINT8           PcieRootPortIOApic[4];
  UINT8           IffsEnable;
  UINT8           IffsOnS3RtcWake;
  UINT8           IffsS3WakeTimerMin;
  UINT8           IffsOnS3CritBattWake;
  UINT8           IffsCritBattWakeThreshold;
  UINT8           ScramblerSupport;
  UINT8           SecureBoot;
  UINT8           SecureBootCustomMode;
  UINT8           SecureBootUserPhysicalPresent;
  UINT8           CoreFreMultipSelect;
  UINT8           MaxCState;
  UINT8           PanelScaling;
  UINT8           IgdLcdIGmchBlc;
  UINT8           GfxBoost;
  UINT8           IgdThermal;
  UINT8           SEC00;
  UINT8           fTPM;
  UINT8           SEC02;
  UINT8           SEC03;
  UINT8           MeasuredBootEnable;
  UINT8           UseProductKey;
  //Image Signal Processor PCI Device Configuration
  //
  UINT8         ISPDevSel;
  UINT8         ISPEn;
  // Passwords
  UINT16          UserPassword[PASSWORD_MAX_SIZE];
  UINT16          AdminPassword[PASSWORD_MAX_SIZE];
  UINT8           Tdt;
  UINT8           Recovery;
  UINT8           Suspend;
  UINT8           TdtState;
  UINT8           TdtEnrolled;
  UINT8           PBAEnable;

  UINT8           HpetBootTime;
  UINT8           UsbDebug;
  UINT8           Lpe;
  //
  // LPSS Configuration
  //
  UINT8           LpssPciModeEnabled;
  //Scc
  UINT8           LpsseMMCEnabled;
  UINT8           LpssSdioEnabled;
  UINT8           LpssSdcardEnabled;
  UINT8           LpssSdCardSDR25Enabled;
  UINT8           LpssSdCardDDR50Enabled;
  UINT8           LpssMipiHsi;
  UINT8           LpsseMMC45Enabled;
  UINT8           LpsseMMC45DDR50Enabled;
  UINT8           LpsseMMC45HS200Enabled;
  UINT8           LpsseMMC45RetuneTimerValue;
  UINT8           eMMCBootMode;

  //LPSS2
  UINT8           LpssDma1Enabled;
  UINT8           LpssI2C0Enabled;
  UINT8           LpssI2C1Enabled;
  UINT8           LpssI2C2Enabled;
  UINT8           LpssI2C3Enabled;
  UINT8           LpssI2C4Enabled;
  UINT8           LpssI2C5Enabled;
  UINT8           LpssI2C6Enabled;
  //LPSS1
  UINT8           LpssDma0Enabled;
  UINT8           LpssPwm0Enabled;
  UINT8           LpssPwm1Enabled;
  UINT8           LpssHsuart0Enabled;
  UINT8           LpssHsuart1Enabled;
  UINT8           LpssSpiEnabled;
  UINT8           I2CTouchAd;

  UINT8   GTTSize;
  //
  // DVMT5.0 Graphic memory setting
  //
  UINT8   IgdDvmt50PreAlloc;
  UINT8   IgdDvmt50TotalAlloc;
  UINT8   IgdTurboEnabled;

  //
  // Usb Config
  //
  UINT8   UsbAutoMode;       // PCH controller Auto mode
  UINT8   UsbXhciSupport;
  UINT8   Hsic0;
  UINT8   PchUsb30Mode;
  UINT8   PchUsb30Streams;
  UINT8   PchUsb20;
  UINT8   PchUsbPerPortCtl;
  UINT8   PchUsbPort[8];
  UINT8   PchUsbRmh;
  UINT8   PchUsbOtg;
  UINT8   PchUsbVbusOn;       //OTG VBUS control
  UINT8   PchFSAOn;       //FSA control
  UINT8   EhciPllCfgEnable;


  //Gbe
  UINT8         PcieRootPortSpeed[PCH_PCIE_MAX_ROOT_PORTS];
  UINT8   SlpLanLowDc;

  UINT8   ISCT00;
  UINT8   ISCT01;
  UINT8   ISCT02;
  UINT8   ISCT03;
  UINT8    ISCT04;
  UINT8    ISCT05;
  UINT8    ISCT06;
  UINT8    ISCT07;
  //
  // Azalia Configuration
  //
  UINT8   PchAzalia;
  UINT8   AzaliaVCiEnable;
  UINT8   AzaliaDs;
  UINT8   AzaliaPme;
  UINT8   HdmiCodec;

  UINT8   UartInterface;
  UINT8   PcuUart1;
  //UINT8   PcuUart2;//for A0
  UINT8   StateAfterG3;
  UINT8   EnableClockSpreadSpec;
  UINT8   GraphicReserve00;
  UINT8   GOPEnable;
  UINT8   GOPBrightnessLevel;                     //Gop Brightness level
  UINT8   PavpMode;
  UINT8   SEC04;
  UINT8   SEC05;
  UINT8   SEC06;
  UINT8   SEC07;

  UINT8   HdmiCodecPortB;
  UINT8   HdmiCodecPortC;
  UINT8   HdmiCodecPortD;
  UINT8   LidStatus;
  UINT8   Reserved00;
  UINT8   Reserved01;
  UINT16  Reserved02;
  UINT16  Reserved03;
  UINT16  Reserved04;
  UINT16  Reserved05;
  UINT16  Reserved06;
  UINT16  Reserved07;
  UINT16  Reserved08;
  UINT16  Reserved09;
  UINT16  Reserved0A;
  UINT16  Reserved0B;
  UINT16  Reserved0C;
  UINT16  Reserved0D;
  UINT8   Reserved0E;
  UINT8   Reserved0F;
  UINT32  Reserved10;
  UINT32  Reserved11;
  UINT32  Reserved12;
  UINT32  Reserved13;
  UINT32  Reserved14;
  UINT8   Reserved15;
  UINT8   Reserved16;
  UINT8   Reserved17;
  UINT8   Reserved18;
  UINT8   Reserved19;
  UINT8   Reserved1A;
  UINT8   Reserved1B;
  UINT8   Reserved1C;
  UINT8   Reserved1D;
  UINT8   Reserved1E;
  UINT8   Reserved1F;
  UINT8   Reserved20;
  UINT8   PmicEnable;
  UINT8   IdleReserve;
  UINT8   TSEGSizeSel;
  UINT8   ACPIMemDbg;
  UINT8    ExISupport;
  UINT8   BatteryChargingSolution;                 //0-non ULPMC 1-ULPMC
  UINT8   PnpSettings;
  UINT8   CfioPnpSettings;
  UINT8   PchEhciDebug;
  UINT8   CRIDSettings;
  UINT8   ULPMCFWLock;
  UINT8   SpiRwProtect;
  UINT8   GraphicReserve02;
  UINT8   PDMConfig;
  UINT16  LmMemSize;
  UINT8   PunitBIOSConfig;
  UINT8   LpssSdioMode;
  UINT8   ENDBG2;
  UINT8   WittEnable;
  UINT8   UtsEnable;
  UINT8   TristateLpc;
  UINT8   GraphicReserve05;
  UINT8   UsbXhciLpmSupport;
  UINT8   EnableAESNI;
  UINT8   SecureErase;

  UINT8   MmioSize;


  UINT8   SAR1;

  UINT8   DisableCodec262;
  UINT8   ReservedO;
  UINT8   PcieDynamicGating;        // Need PMC enable it first from PMC 0x3_12 MCU 318.

  UINT8   MipiDsi;

  //Added flow control item for UART1 and UART2
  UINT8  LpssHsuart0FlowControlEnabled;
  UINT8  LpssHsuart1FlowControlEnabled;

  UINT8   SdCardRemovable; // ACPI reporting MMC/SD media as: removable/non-removable
  UINT8   GpioWakeCapability;
  UINT8   RtcBattery;
  UINT8   LpeAudioReportedByDSDT;
  
  UINT8   Uart1Int3511Com; // Report UART1 as COM with _HID INT3511
  CHAR16  SystemUuid[37];

} SYSTEM_CONFIGURATION;
#pragma pack()

#ifndef PLATFORM_SETUP_VARIABLE_NAME
#define PLATFORM_SETUP_VARIABLE_NAME             L"Setup"
#endif

#pragma pack(1)
typedef struct{
  // Passwords
  UINT16        UserPassword[PASSWORD_MAX_SIZE];
  UINT16        AdminPassword[PASSWORD_MAX_SIZE];
  UINT16        DummyDataForVfrBug;  // Don't change or use

} SYSTEM_PASSWORDS;
#pragma pack()

//
// #defines for Drive Presence
//
#define EFI_HDD_PRESENT       0x01
#define EFI_HDD_NOT_PRESENT   0x00
#define EFI_CD_PRESENT        0x02
#define EFI_CD_NOT_PRESENT    0x00

#define EFI_HDD_WARNING_ON    0x01
#define EFI_CD_WARNING_ON     0x02
#define EFI_SMART_WARNING_ON  0x04
#define EFI_HDD_WARNING_OFF   0x00
#define EFI_CD_WARNING_OFF    0x00
#define EFI_SMART_WARNING_OFF 0x00

#ifndef VFRCOMPILE
extern EFI_GUID gEfiSetupVariableGuid;
#endif

#define SETUP_DATA SYSTEM_CONFIGURATION

#endif // #ifndef _SETUP_VARIABLE

