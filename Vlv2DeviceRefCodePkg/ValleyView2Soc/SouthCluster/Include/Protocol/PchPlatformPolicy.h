/**
**/
/**

Copyright (c) 2013  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



  @file
  PchPlatformPolicy.h

  @brief
  PCH policy protocol produced by a platform driver specifying various
  expected PCH settings. This protocol is consumed by the PCH drivers.

**/
#ifndef _PCH_PLATFORM_POLICY_H_
#define _PCH_PLATFORM_POLICY_H_


//
#include "PchRegs.h"
#ifndef ECP_FLAG
#include "Uefi.h"
#endif

#define DXE_PCH_PLATFORM_POLICY_PROTOCOL_GUID \
  { \
    0x4b0165a9, 0x61d6, 0x4e23, 0xa0, 0xb5, 0x3e, 0xc7, 0x9c, 0x2e, 0x30, 0xd5 \
  }
extern EFI_GUID                                   gDxePchPlatformPolicyProtocolGuid;

///
/// Forward reference for ANSI C compatibility
///
typedef struct _DXE_PCH_PLATFORM_POLICY_PROTOCOL  DXE_PCH_PLATFORM_POLICY_PROTOCOL;

///
/// Protocol revision number
/// Any backwards compatible changes to this protocol will result in an update in the revision number
/// Major changes will require publication of a new protocol
///
/// Revision 1: Original version
///
#define DXE_PCH_PLATFORM_POLICY_PROTOCOL_REVISION_1 1
#define DXE_PCH_PLATFORM_POLICY_PROTOCOL_REVISION_2 2
#define DXE_PCH_PLATFORM_POLICY_PROTOCOL_REVISION_3 3
#define DXE_PCH_PLATFORM_POLICY_PROTOCOL_REVISION_4 4
#define DXE_PCH_PLATFORM_POLICY_PROTOCOL_REVISION_5 5
#define DXE_PCH_PLATFORM_POLICY_PROTOCOL_REVISION_6 6
#define DXE_PCH_PLATFORM_POLICY_PROTOCOL_REVISION_7 7
#define DXE_PCH_PLATFORM_POLICY_PROTOCOL_REVISION_8 8
#define DXE_PCH_PLATFORM_POLICY_PROTOCOL_REVISION_9 9
#define DXE_PCH_PLATFORM_POLICY_PROTOCOL_REVISION_10 10
#define DXE_PCH_PLATFORM_POLICY_PROTOCOL_REVISION_11 11
#define DXE_PCH_PLATFORM_POLICY_PROTOCOL_REVISION_12 12

///
/// Generic definitions for device enabling/disabling used by PCH code.
///
#define PCH_DEVICE_ENABLE   1
#define PCH_DEVICE_DISABLE  0

///
/// ---------------------------- Device Enabling ------------------------------
///
/// PCH Device enablings
///
typedef struct {
  UINT8 Lan               : 1;    /// 0: Disable; 1: Enable
  UINT8 Azalia            : 2;    /// 0: Disable; 1: Enable; 2: Auto
  UINT8 Sata              : 1;    /// 0: Disable; 1: Enable
  UINT8 Smbus             : 1;    /// 0: Disable; 1: Enable
  UINT8 LpeEnabled        : 2;    /// 0: Disabled; 1: PCI Mode 2: ACPI Mode
  UINT8 Reserved[1];              /// Reserved fields for future expansion w/o protocol change
} PCH_DEVICE_ENABLING;

///
/// ---------------------------- USB Config -----------------------------
///
///
/// Overcurrent pins
///
typedef enum {
  PchUsbOverCurrentPin0 = 0,
  PchUsbOverCurrentPin1,
  PchUsbOverCurrentPin2,
  PchUsbOverCurrentPin3,
  PchUsbOverCurrentPin4,
  PchUsbOverCurrentPin5,
  PchUsbOverCurrentPin6,
  PchUsbOverCurrentPin7,
  PchUsbOverCurrentPinSkip,
  PchUsbOverCurrentPinMax
} PCH_USB_OVERCURRENT_PIN;

typedef struct {
  UINT8   Enable            : 1;    /// 0: Disable; 1: Enable. This would take effect while UsbPerPortCtl is enabled
  UINT8   Panel             : 1;    /// 0: Back Panel Port; 1: Front Panel Port.
  UINT8   Dock              : 1;    /// 0: Not docking port; 1: Docking Port.
  UINT8   Rsvdbits          : 5;
} PCH_USB_PORT_SETTINGS;

typedef struct {
  UINT8 Enable              : 1;    /// 0: Disable; 1: Enable
  UINT8 Rsvdbits            : 7;
} PCH_USB20_CONTROLLER_SETTINGS;

typedef struct {
  UINT8 Enable              : 2;    /// 0: 0: Disabled; 1: PCI Mode 2: ACPI Mode
  UINT8 Rsvdbits            : 6;
} PCH_USBOTG_CONTROLLER_SETTINGS;

#define PCH_XHCI_MODE_OFF         0
#define PCH_XHCI_MODE_ON          1
#define PCH_XHCI_MODE_AUTO        2
#define PCH_XHCI_MODE_SMARTAUTO   3

#define PCH_EHCI_DEBUG_OFF        0
#define PCH_EHCI_DEBUG_ON         1

#define PCH_USB_FRONT_PANEL       1
#define PCH_USB_BACK_PANEL        0

typedef struct {
  UINT8 Mode               : 2;    /// 0: Disable; 1: Enable, 2: Auto, 3: Smart Auto
  UINT8 PreBootSupport     : 1;    /// 0: No xHCI driver available; 1: xHCI driver available
  UINT8 XhciStreams        : 1;    /// 0: Disable; 1: Enable
  UINT8 Rsvdbits           : 4;
} PCH_USB30_CONTROLLER_SETTINGS;

typedef struct {
  UINT8 UsbPerPortCtl       : 1;    /// 0: Disable; 1: Enable Per-port enable control
  UINT8 Ehci1Usbr           : 1;    /// 0: Disable; 1: Enable EHCI 1 USBR
  UINT8 RsvdBits            : 6;
  PCH_USB_PORT_SETTINGS          PortSettings[PCH_USB_MAX_PHYSICAL_PORTS];
  PCH_USB20_CONTROLLER_SETTINGS  Usb20Settings[PchEhciControllerMax];
  PCH_USB30_CONTROLLER_SETTINGS  Usb30Settings;
  PCH_USBOTG_CONTROLLER_SETTINGS UsbOtgSettings;
  PCH_USB_OVERCURRENT_PIN        Usb20OverCurrentPins[PCH_USB_MAX_PHYSICAL_PORTS];
  PCH_USB_OVERCURRENT_PIN        Usb30OverCurrentPins[PCH_XHCI_MAX_USB3_PORTS];
  ///
  /// The length of Usb Port to configure the USB transmitter,
  /// Bits [16:4] represents length of Usb Port in inches using octal format and [3:0] is for the decimal Point.
  ///
  UINT16                        Usb20PortLength[PCH_EHCI_MAX_PORTS];
  UINT16                        EhciDebug;
  UINT16                        UsbXhciLpmSupport;

} PCH_USB_CONFIG;

///
/// ---------------------------- PCI Express Config ----------------------
///
/// The values before AutoConfig match the setting of PCI Express Base Specification 1.1, please be careful for adding new feature
///
typedef enum {
  PchPcieAspmDisabled,
  PchPcieAspmL0s,
  PchPcieAspmL1,
  PchPcieAspmL0sL1,
  PchPcieAspmAutoConfig,
  PchPcieAspmMax
} PCH_PCI_EXPRESS_ASPM_CONTROL;

///
/// Refer to PCH EDS for the PCH implementation values corresponding
/// to below PCI-E spec defined ranges
///
typedef enum {
  PchPciECompletionTO_Default,
  PchPciECompletionTO_50_100us,
  PchPciECompletionTO_1_10ms,
  PchPciECompletionTO_16_55ms,
  PchPciECompletionTO_65_210ms,
  PchPciECompletionTO_260_900ms,
  PchPciECompletionTO_1_3P5s,
  PchPciECompletionTO_4_13s,
  PchPciECompletionTO_17_64s,
  PchPciECompletionTO_Disabled
} PCH_PCIE_COMPLETION_TIMEOUT;

typedef struct {
  UINT8 Enable                          : 1;    /// Root Port enabling, 0: Disable; 1: Enable.
  UINT8 Hide                            : 1;    /// Whether or not to hide the configuration space of this port
  UINT8 SlotImplemented                 : 1;
  UINT8 HotPlug                         : 1;
  UINT8 PmSci                           : 1;
  UINT8 ExtSync                         : 1;    /// Extended Synch
  UINT8 Rsvdbits                        : 2;
  ///
  /// Error handlings
  ///
  UINT8 UnsupportedRequestReport        : 1;
  UINT8 FatalErrorReport                : 1;
  UINT8 NoFatalErrorReport              : 1;
  UINT8 CorrectableErrorReport          : 1;
  UINT8 PmeInterrupt                    : 1;
  UINT8 SystemErrorOnFatalError         : 1;
  UINT8 SystemErrorOnNonFatalError      : 1;
  UINT8 SystemErrorOnCorrectableError   : 1;

  UINT8 AdvancedErrorReporting          : 1;
  UINT8 TransmitterHalfSwing            : 1;
  UINT8 Reserved                        : 6;    /// Reserved fields for future expansion w/o protocol change

  UINT8 FunctionNumber;                         /// The function number this root port is mapped to.
  UINT8 PhysicalSlotNumber;
  PCH_PCIE_COMPLETION_TIMEOUT   CompletionTimeout;
  PCH_PCI_EXPRESS_ASPM_CONTROL  Aspm;
} PCH_PCI_EXPRESS_ROOT_PORT_CONFIG;

typedef struct {
  /**
    VendorId

      The vendor Id of Pci Express card ASPM setting override, 0xFFFF means any Vendor ID

    DeviceId

      The Device Id of Pci Express card ASPM setting override, 0xFFFF means any Device ID

    RevId

      The Rev Id of Pci Express card ASPM setting override, 0xFF means all steppings

    BaseClassCode

      The Base Class Code of Pci Express card ASPM setting override, 0xFF means all base class

    SubClassCode

      The Sub Class Code of Pci Express card ASPM setting override, 0xFF means all sub class


    EndPointAspm

      The override ASPM setting from End point
  **/
  UINT16                        VendorId;
  UINT16                        DeviceId;
  UINT8                         RevId;
  UINT8                         BaseClassCode;
  UINT8                         SubClassCode;
  PCH_PCI_EXPRESS_ASPM_CONTROL  EndPointAspm;
} PCH_PCIE_DEVICE_ASPM_OVERRIDE;

typedef struct {
  UINT16  VendorId; ///< PCI configuration space offset 0
  UINT16  DeviceId; ///< PCI configuration space offset 2
  UINT8   RevId;    ///< PCI configuration space offset 8; 0xFF means all steppings
  /**
    SnoopLatency bit definition
    Note: All Reserved bits must be set to 0

    BIT[15]     - When set to 1b, indicates that the values in bits 9:0 are valid
                  When clear values in bits 9:0 will be ignored
    BITS[14:13] - Reserved
    BITS[12:10] - Value in bits 9:0 will be multiplied with the scale in these bits
                  000b - 1 ns
                  001b - 32 ns
                  010b - 1024 ns
                  011b - 32,768 ns
                  100b - 1,048,576 ns
                  101b - 33,554,432 ns
                  110b - Reserved
                  111b - Reserved
    BITS[9:0]   - Snoop Latency Value. The value in these bits will be multiplied with
                  the scale in bits 12:10
  **/
  UINT16  SnoopLatency;
  /**
    NonSnoopLatency bit definition
    Note: All Reserved bits must be set to 0

    BIT[15]     - When set to 1b, indicates that the values in bits 9:0 are valid
                  When clear values in bits 9:0 will be ignored
    BITS[14:13] - Reserved
    BITS[12:10] - Value in bits 9:0 will be multiplied with the scale in these bits
                  000b - 1 ns
                  001b - 32 ns
                  010b - 1024 ns
                  011b - 32,768 ns
                  100b - 1,048,576 ns
                  101b - 33,554,432 ns
                  110b - Reserved
                  111b - Reserved
    BITS[9:0]   - Non Snoop Latency Value. The value in these bits will be multiplied with
                  the scale in bits 12:10
  **/
  UINT16  NonSnoopLatency;
} PCH_PCIE_DEVICE_LTR_OVERRIDE;

typedef struct {
  ///
  /// Temp Bus Number range available to be assigned to
  /// each root port and its downstream devices for initialization
  /// of these devices before PCI Bus enumeration
  ///
  UINT8                             TempRootPortBusNumMin;
  UINT8                             TempRootPortBusNumMax;
  PCH_PCI_EXPRESS_ROOT_PORT_CONFIG  RootPort[PCH_PCIE_MAX_ROOT_PORTS];
  BOOLEAN                           RootPortClockGating;
  UINT8                             NumOfDevAspmOverride;     /// Number of PCI Express card Aspm setting override
  PCH_PCIE_DEVICE_ASPM_OVERRIDE     *DevAspmOverride;         /// The Pointer which is point to Pci Express card Aspm setting override
  UINT8                             PcieDynamicGating;        /// Need PMC enable it first from PMC 0x3_12 MCU 318.
} PCH_PCI_EXPRESS_CONFIG;


///
/// ---------------------------- SATA Config -----------------------------
///
typedef enum {
  PchSataSpeedSupportGen1 = 1,
  PchSataSpeedSupportGen2
} PCH_SATA_SPEED_SUPPORT;

typedef struct {
  UINT8 Enable          : 1;    /// 0: Disable; 1: Enable
  UINT8 HotPlug         : 1;    /// 0: Disable; 1: Enable
  UINT8 MechSw          : 1;    /// 0: Disable; 1: Enable
  UINT8 External        : 1;    /// 0: Disable; 1: Enable
  UINT8 SpinUp          : 1;    /// 0: Disable; 1: Enable the COMRESET initialization Sequence to the device
  UINT8 Rsvdbits        : 3;    /// Reserved fields for future expansion w/o protocol change
} PCH_SATA_PORT_SETTINGS;

typedef struct {
  PCH_SATA_PORT_SETTINGS  PortSettings[PCH_AHCI_MAX_PORTS];
  UINT8 RaidAlternateId : 1;    /// 0: Disable; 1: Enable
  UINT8 Raid0           : 1;    /// 0: Disable; 1: Enable RAID0
  UINT8 Raid1           : 1;    /// 0: Disable; 1: Enable RAID1
  UINT8 Raid10          : 1;    /// 0: Disable; 1: Enable RAID10
  UINT8 Raid5           : 1;    /// 0: Disable; 1: Enable RAID5
  UINT8 Irrt            : 1;    /// 0: Disable; 1: Enable Intel Rapid Recovery Technology
  UINT8 OromUiBanner    : 1;    /// 0: Disable; 1: Enable OROM UI and BANNER
  UINT8 HddUnlock       : 1;    /// 0: Disable; 1: Indicates that the HDD password unlock in the OS is enabled

  UINT8 LedLocate       : 1;    /// 0: Disable; 1: Indicates that the LED/SGPIO hardware is attached and ping to locate feature is enabled on the OS
  UINT8 IrrtOnly        : 1;    /// 0: Disable; 1: Allow only IRRT drives to span internal and external ports
  UINT8 TestMode        : 1;    /// 0: Disable; 1: Allow entrance to the PCH SATA test modes
  UINT8 SalpSupport     : 1;    /// 0: Disable; 1: Enable Aggressive Link Power Management
  UINT8 LegacyMode      : 1;    /// 0: Native PCI mode; 1: Legacy mode, when SATA controller is operating in IDE mode
  UINT8 SpeedSupport    : 4;    /// Indicates the maximum speed the SATA controller can support
  /// 1h: 1.5 Gb/s (Gen 1); 2h: 3 Gb/s(Gen 2)

  UINT8 Rsvdbits        : 7;    // Reserved fields for future expansion w/o protocol change
} PCH_SATA_CONFIG;
///
/// --------------------------- AZALIA Config ------------------------------
///
typedef struct {
  UINT32  VendorDeviceId;
  UINT16  SubSystemId;
  UINT8   RevisionId;                       /// 0xFF applies to all steppings
  UINT8   FrontPanelSupport;
  UINT16  NumberOfRearJacks;
  UINT16  NumberOfFrontJacks;
} PCH_AZALIA_VERB_TABLE_HEADER;

typedef struct {
  PCH_AZALIA_VERB_TABLE_HEADER  VerbTableHeader;
  UINT32                        *VerbTableData;
} PCH_AZALIA_VERB_TABLE;

typedef struct {
  UINT8                 Pme       : 1;      /// 0: Disable; 1: Enable
  UINT8                 DS        : 1;      /// 0: Docking is not supported; 1:Docking is supported
  UINT8                 DA        : 1;      /// 0: Docking is not attached; 1:Docking is attached
  UINT8                 HdmiCodec : 1;      /// 0: Disable; 1: Enable
  UINT8                 AzaliaVCi : 1;      /// 0: Disable; 1: Enable
  UINT8                 Rsvdbits  : 3;
  UINT8                 AzaliaVerbTableNum; /// Number of verb tables provided by platform
  PCH_AZALIA_VERB_TABLE *AzaliaVerbTable;   /// Pointer to the actual verb table(s)
  UINT16                ResetWaitTimer;     /// The delay timer after Azalia reset, the value is number of microseconds
} PCH_AZALIA_CONFIG;

///
/// --------------------------- Smbus Config ------------------------------
///
typedef struct {
  UINT8 NumRsvdSmbusAddresses;
  UINT8 *RsvdSmbusAddressTable;
} PCH_SMBUS_CONFIG;

///
/// --------------------------- Miscellaneous PM Config ------------------------------
///
typedef struct {
  UINT8 MeWakeSts           : 1;
  UINT8 MeHrstColdSts       : 1;
  UINT8 MeHrstWarmSts       : 1;
  UINT8 MeHostPowerDn       : 1;
  UINT8 WolOvrWkSts         : 1;
  UINT8 Rsvdbits            : 3;
} PCH_POWER_RESET_STATUS;

typedef struct {
  UINT8  PmeB0S5Dis         : 1;
  UINT8  WolEnableOverride  : 1;
  UINT8  Rsvdbits           : 6;
} PCH_WAKE_CONFIG;

typedef enum {
  PchSlpS360us,
  PchSlpS31ms,
  PchSlpS350ms,
  PchSlpS32s
} PCH_SLP_S3_MIN_ASSERT;

typedef enum {
  PchSlpS4PchTime,   /// The time defined in EDS Power Sequencing and Reset Signal Timings table
  PchSlpS41s,
  PchSlpS42s,
  PchSlpS43s,
  PchSlpS44s
} PCH_SLP_S4_MIN_ASSERT;

typedef struct {
  ///
  /// Specify which Power/Reset bits need to be cleared by
  /// the PCH Init Driver.
  /// Usually platform drivers take care of these bits, but if
  /// not, let PCH Init driver clear the bits.
  ///
  PCH_POWER_RESET_STATUS  PowerResetStatusClear;
  ///
  /// Specify Wake Policy
  ///
  PCH_WAKE_CONFIG         WakeConfig;
  ///
  /// SLP_XX Minimum Assertion Width Policy
  ///
  PCH_SLP_S3_MIN_ASSERT   PchSlpS3MinAssert;
  PCH_SLP_S4_MIN_ASSERT   PchSlpS4MinAssert;
  UINT8                   SlpStrchSusUp : 1;  /// Enable/Disable SLP_X Stretching After SUS Well Power Up
  UINT8                   SlpLanLowDc   : 1;
  UINT8                   Rsvdbits      : 6;
} PCH_MISC_PM_CONFIG;

///
/// --------------------------- Subsystem Vendor ID / Subsystem ID Config -----
///
typedef struct {
  UINT16  SubSystemVendorId;
  UINT16  SubSystemId;
} PCH_DEFAULT_SVID_SID;

///
/// --------------------------- Lock Down Config ------------------------------
///
typedef struct {
  UINT8  GlobalSmi      : 1;
  UINT8  BiosInterface  : 1;
  UINT8  RtcLock        : 1;
  UINT8  BiosLock       : 1;
  UINT8  Rsvdbits       : 4;
  UINT8  PchBiosLockSwSmiNumber;
} PCH_LOCK_DOWN_CONFIG;
//
// --------------------------- Serial IRQ Config ------------------------------
//
typedef enum {
  PchQuietMode,
  PchContinuousMode
} PCH_SIRQ_MODE;
///
/// Refer to SoC EDS for the details of Start Frame Pulse Width in Continuous and Quiet mode
///

typedef struct {
  BOOLEAN                 SirqEnable;       /// Determines if enable Serial IRQ
  PCH_SIRQ_MODE           SirqMode;         /// Serial IRQ Mode Select
} PCH_LPC_SIRQ_CONFIG;

///
/// --------------------------- Power Optimizer Config ------------------------------
///
typedef struct {
  UINT8  NumOfDevLtrOverride;                            /// Number of Pci Express card listed in LTR override table
  PCH_PCIE_DEVICE_LTR_OVERRIDE *DevLtrOverride;          /// Pointer to Pci Express devices LTR override table
} PCH_PWR_OPT_CONFIG;

///
/// --------------------- Low Power Input Output Config ------------------------
///
typedef struct {
  UINT8                   LpssPciModeEnabled    : 1;    /// Determines if LPSS PCI Mode enabled
  UINT8                   Dma0Enabled           : 1;     /// Determines if LPSS DMA1 enabled
  UINT8                   Dma1Enabled           : 1;     /// Determines if LPSS DMA2 enabled
  UINT8                   I2C0Enabled           : 1;     /// Determines if LPSS I2C #1 enabled
  UINT8                   I2C1Enabled           : 1;     /// Determines if LPSS I2C #2 enabled
  UINT8                   I2C2Enabled           : 1;     /// Determines if LPSS I2C #3 enabled
  UINT8                   I2C3Enabled           : 1;     /// Determines if LPSS I2C #4 enabled
  UINT8                   I2C4Enabled           : 1;     /// Determines if LPSS I2C #5 enabled
  UINT8                   I2C5Enabled           : 1;     /// Determines if LPSS I2C #6 enabled
  UINT8                   I2C6Enabled           : 1;     /// Determines if LPSS I2C #7 enabled
  UINT8                   Pwm0Enabled           : 1;     /// Determines if LPSS PWM #1 enabled
  UINT8                   Pwm1Enabled           : 1;     /// Determines if LPSS PWM #2 enabled
  UINT8                   Hsuart0Enabled        : 1;     /// Determines if LPSS HSUART #1 enabled
  UINT8                   Hsuart1Enabled        : 1;     /// Determines if LPSS HSUART #2 enabled
  UINT8                   SpiEnabled            : 1;     /// Determines if LPSS SPI enabled
  UINT8                   Rsvdbits              : 2;
} PCH_LPSS_CONFIG;

///
/// ----------------------------- SCC Config --------------------------------
///
typedef struct {
  UINT8                   eMMCEnabled           : 1;      /// Determines if SCC eMMC enabled
  UINT8                   SdioEnabled           : 1;      /// Determines if SCC SDIO enabled
  UINT8                   SdcardEnabled         : 1;      /// Determines if SCC SD Card enabled
  UINT8                   HsiEnabled            : 1;      /// Determines if SCC HSI enabled
  UINT8                   eMMC45Enabled         : 1;      /// Determines if SCC eMMC 4.5 enabled
  UINT8                   eMMC45DDR50Enabled    : 1;  /// Determines if DDR50 enabled for eMMC 4.5
  UINT8                   eMMC45HS200Enabled    : 1;  /// Determines if HS200nabled for eMMC 4.5
  UINT8                   Rsvdbits              : 1;
  UINT8                   SdCardSDR25Enabled    : 1;    /// Determines if SDR25 for SD Card
  UINT8                   SdCardDDR50Enabled    : 1;    /// Determines if DDR50 for SD Card
  UINT8                   Rsvdbits1             : 6;
  UINT8                   eMMC45RetuneTimerValue;  /// Determines retune timer value.
} PCH_SCC_CONFIG;

///
/// ------------ General PCH Platform Policy protocol definition ------------
///
struct _DXE_PCH_PLATFORM_POLICY_PROTOCOL {
  UINT8                   Revision;
  UINT8                   BusNumber;  /// PCI Bus Number of the PCH device
  PCH_DEVICE_ENABLING     *DeviceEnabling;
  PCH_USB_CONFIG          *UsbConfig;
  PCH_PCI_EXPRESS_CONFIG  *PciExpressConfig;

  PCH_SATA_CONFIG         *SataConfig;
  PCH_AZALIA_CONFIG       *AzaliaConfig;
  PCH_SMBUS_CONFIG        *SmbusConfig;
  PCH_MISC_PM_CONFIG      *MiscPmConfig;
  PCH_DEFAULT_SVID_SID    *DefaultSvidSid;
  PCH_LOCK_DOWN_CONFIG    *LockDownConfig;
  PCH_LPC_SIRQ_CONFIG     *SerialIrqConfig;
  PCH_PWR_OPT_CONFIG      *PwrOptConfig;
  PCH_LPSS_CONFIG         *LpssConfig;
  PCH_SCC_CONFIG          *SccConfig;
  UINT8                   IdleReserve;
  UINT8                   EhciPllCfgEnable;
  UINT8                   AcpiHWRed; //Hardware Reduced Mode
};

#endif
