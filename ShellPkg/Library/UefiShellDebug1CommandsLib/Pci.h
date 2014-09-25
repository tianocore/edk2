/** @file
  Header file for Pci shell Debug1 function.

  Copyright (c) 2013 Hewlett-Packard Development Company, L.P.
  Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_SHELL_PCI_H_
#define _EFI_SHELL_PCI_H_

typedef enum {
  PciDevice,
  PciP2pBridge,
  PciCardBusBridge,
  PciUndefined
} PCI_HEADER_TYPE;

#define HEADER_TYPE_MULTI_FUNCTION    0x80

#define MAX_BUS_NUMBER                255
#define MAX_DEVICE_NUMBER             31
#define MAX_FUNCTION_NUMBER           7

#define EFI_PCI_CAPABILITY_ID_PCIEXP  0x10
#define EFI_PCI_CAPABILITY_ID_PCIX    0x07

#define CALC_EFI_PCI_ADDRESS(Bus, Dev, Func, Reg) \
    ((UINT64) ((((UINTN) Bus) << 24) + (((UINTN) Dev) << 16) + (((UINTN) Func) << 8) + ((UINTN) Reg)))

#define CALC_EFI_PCIEX_ADDRESS(Bus, Dev, Func, ExReg) ( \
      (UINT64) ((((UINTN) Bus) << 24) + (((UINTN) Dev) << 16) + (((UINTN) Func) << 8) + (LShiftU64 ((UINT64) ExReg, 32))) \
   );

#define INDEX_OF(Field)                               ((UINT8 *) (Field) - (UINT8 *) mConfigSpace)

#define PCI_BIT_0                                     0x00000001
#define PCI_BIT_1                                     0x00000002
#define PCI_BIT_2                                     0x00000004
#define PCI_BIT_3                                     0x00000008
#define PCI_BIT_4                                     0x00000010
#define PCI_BIT_5                                     0x00000020
#define PCI_BIT_6                                     0x00000040
#define PCI_BIT_7                                     0x00000080
#define PCI_BIT_8                                     0x00000100
#define PCI_BIT_9                                     0x00000200
#define PCI_BIT_10                                    0x00000400
#define PCI_BIT_11                                    0x00000800
#define PCI_BIT_12                                    0x00001000
#define PCI_BIT_13                                    0x00002000
#define PCI_BIT_14                                    0x00004000
#define PCI_BIT_15                                    0x00008000

//
// PCIE device/port types
//
#define PCIE_PCIE_ENDPOINT                            0
#define PCIE_LEGACY_PCIE_ENDPOINT                     1
#define PCIE_ROOT_COMPLEX_ROOT_PORT                   4
#define PCIE_SWITCH_UPSTREAM_PORT                     5
#define PCIE_SWITCH_DOWNSTREAM_PORT                   6
#define PCIE_PCIE_TO_PCIX_BRIDGE                      7
#define PCIE_PCIX_TO_PCIE_BRIDGE                      8
#define PCIE_ROOT_COMPLEX_INTEGRATED_PORT             9
#define PCIE_ROOT_COMPLEX_EVENT_COLLECTOR             10
#define PCIE_DEVICE_PORT_TYPE_MAX                     11

#define IS_PCIE_ENDPOINT(DevicePortType) \
    ((DevicePortType) == PCIE_PCIE_ENDPOINT || \
     (DevicePortType) == PCIE_LEGACY_PCIE_ENDPOINT || \
     (DevicePortType) == PCIE_ROOT_COMPLEX_INTEGRATED_PORT)

#define IS_PCIE_SWITCH(DevicePortType) \
    ((DevicePortType == PCIE_SWITCH_UPSTREAM_PORT) || \
     (DevicePortType == PCIE_SWITCH_DOWNSTREAM_PORT))

//
// Capabilities Register
//
#define PCIE_CAP_VERSION(PcieCapReg) \
    ((PcieCapReg) & 0x0f)
#define PCIE_CAP_DEVICEPORT_TYPE(PcieCapReg) \
    (((PcieCapReg) >> 4) & 0x0f)
#define PCIE_CAP_SLOT_IMPLEMENTED(PcieCapReg) \
    (((PcieCapReg) >> 8) & 0x1)
#define PCIE_CAP_INT_MSG_NUM(PcieCapReg) \
    (((PcieCapReg) >> 9) & 0x1f)
//
// Device Capabilities Register
//
#define PCIE_CAP_MAX_PAYLOAD(PcieDeviceCap) \
    ((PcieDeviceCap) & 0x7)
#define PCIE_CAP_PHANTOM_FUNC(PcieDeviceCap) \
    (((PcieDeviceCap) >> 3) & 0x3)
#define PCIE_CAP_EXTENDED_TAG(PcieDeviceCap) \
    (((PcieDeviceCap) >> 5) & 0x1)
#define PCIE_CAP_L0SLATENCY(PcieDeviceCap) \
    (((PcieDeviceCap) >> 6) & 0x7)
#define PCIE_CAP_L1LATENCY(PcieDeviceCap) \
    (((PcieDeviceCap) >> 9) & 0x7)
#define PCIE_CAP_ERR_REPORTING(PcieDeviceCap) \
    (((PcieDeviceCap) >> 15) & 0x1)
#define PCIE_CAP_SLOT_POWER_VALUE(PcieDeviceCap) \
    (((PcieDeviceCap) >> 18) & 0x0ff)
#define PCIE_CAP_SLOT_POWER_SCALE(PcieDeviceCap) \
    (((PcieDeviceCap) >> 26) & 0x3)
#define PCIE_CAP_FUNC_LEVEL_RESET(PcieDeviceCap) \
    (((PcieDeviceCap) >> 28) & 0x1)
//
// Device Control Register
//
#define PCIE_CAP_COR_ERR_REPORTING_ENABLE(PcieDeviceControl) \
    ((PcieDeviceControl) & 0x1)
#define PCIE_CAP_NONFAT_ERR_REPORTING_ENABLE(PcieDeviceControl) \
    (((PcieDeviceControl) >> 1) & 0x1)
#define PCIE_CAP_FATAL_ERR_REPORTING_ENABLE(PcieDeviceControl) \
    (((PcieDeviceControl) >> 2) & 0x1)
#define PCIE_CAP_UNSUP_REQ_REPORTING_ENABLE(PcieDeviceControl) \
    (((PcieDeviceControl) >> 3) & 0x1)
#define PCIE_CAP_RELAXED_ORDERING_ENABLE(PcieDeviceControl) \
    (((PcieDeviceControl) >> 4) & 0x1)
#define PCIE_CAP_MAX_PAYLOAD_SIZE(PcieDeviceControl) \
    (((PcieDeviceControl) >> 5) & 0x7)
#define PCIE_CAP_EXTENDED_TAG_ENABLE(PcieDeviceControl) \
    (((PcieDeviceControl) >> 8) & 0x1)
#define PCIE_CAP_PHANTOM_FUNC_ENABLE(PcieDeviceControl) \
    (((PcieDeviceControl) >> 9) & 0x1)
#define PCIE_CAP_AUX_PM_ENABLE(PcieDeviceControl) \
    (((PcieDeviceControl) >> 10) & 0x1)
#define PCIE_CAP_NO_SNOOP_ENABLE(PcieDeviceControl) \
    (((PcieDeviceControl) >> 11) & 0x1)
#define PCIE_CAP_MAX_READ_REQ_SIZE(PcieDeviceControl) \
    (((PcieDeviceControl) >> 12) & 0x7)
#define PCIE_CAP_BRG_CONF_RETRY(PcieDeviceControl) \
    (((PcieDeviceControl) >> 15) & 0x1)
//
// Device Status Register
//
#define PCIE_CAP_COR_ERR_DETECTED(PcieDeviceStatus) \
    ((PcieDeviceStatus) & 0x1)
#define PCIE_CAP_NONFAT_ERR_DETECTED(PcieDeviceStatus) \
    (((PcieDeviceStatus) >> 1) & 0x1)
#define PCIE_CAP_FATAL_ERR_DETECTED(PcieDeviceStatus) \
    (((PcieDeviceStatus) >> 2) & 0x1)
#define PCIE_CAP_UNSUP_REQ_DETECTED(PcieDeviceStatus) \
    (((PcieDeviceStatus) >> 3) & 0x1)
#define PCIE_CAP_AUX_POWER_DETECTED(PcieDeviceStatus) \
    (((PcieDeviceStatus) >> 4) & 0x1)
#define PCIE_CAP_TRANSACTION_PENDING(PcieDeviceStatus) \
    (((PcieDeviceStatus) >> 5) & 0x1)
//
// Link Capabilities Register
//
#define PCIE_CAP_MAX_LINK_SPEED(PcieLinkCap) \
    ((PcieLinkCap) & 0x0f)
#define PCIE_CAP_MAX_LINK_WIDTH(PcieLinkCap) \
    (((PcieLinkCap) >> 4) & 0x3f)
#define PCIE_CAP_ASPM_SUPPORT(PcieLinkCap) \
    (((PcieLinkCap) >> 10) & 0x3)
#define PCIE_CAP_L0S_LATENCY(PcieLinkCap) \
    (((PcieLinkCap) >> 12) & 0x7)
#define PCIE_CAP_L1_LATENCY(PcieLinkCap) \
    (((PcieLinkCap) >> 15) & 0x7)
#define PCIE_CAP_CLOCK_PM(PcieLinkCap) \
    (((PcieLinkCap) >> 18) & 0x1)
#define PCIE_CAP_SUP_DOWN_ERR_REPORTING(PcieLinkCap) \
    (((PcieLinkCap) >> 19) & 0x1)
#define PCIE_CAP_LINK_ACTIVE_REPORTING(PcieLinkCap) \
    (((PcieLinkCap) >> 20) & 0x1)
#define PCIE_CAP_LINK_BWD_NOTIF_CAP(PcieLinkCap) \
    (((PcieLinkCap) >> 21) & 0x1)
#define PCIE_CAP_PORT_NUMBER(PcieLinkCap) \
    (((PcieLinkCap) >> 24) & 0x0ff)
//
// Link Control Register
//
#define PCIE_CAP_ASPM_CONTROL(PcieLinkControl) \
    ((PcieLinkControl) & 0x3)
#define PCIE_CAP_RCB(PcieLinkControl) \
    (((PcieLinkControl) >> 3) & 0x1)
#define PCIE_CAP_LINK_DISABLE(PcieLinkControl) \
    (((PcieLinkControl) >> 4) & 0x1)
#define PCIE_CAP_COMMON_CLK_CONF(PcieLinkControl) \
    (((PcieLinkControl) >> 6) & 0x1)
#define PCIE_CAP_EXT_SYNC(PcieLinkControl) \
    (((PcieLinkControl) >> 7) & 0x1)
#define PCIE_CAP_CLK_PWR_MNG(PcieLinkControl) \
    (((PcieLinkControl) >> 8) & 0x1)
#define PCIE_CAP_HW_AUTO_WIDTH_DISABLE(PcieLinkControl) \
    (((PcieLinkControl) >> 9) & 0x1)
#define PCIE_CAP_LINK_BDW_MNG_INT_EN(PcieLinkControl) \
    (((PcieLinkControl) >> 10) & 0x1)
#define PCIE_CAP_LINK_AUTO_BDW_INT_EN(PcieLinkControl) \
    (((PcieLinkControl) >> 11) & 0x1)
//
// Link Status Register
//
#define PCIE_CAP_CUR_LINK_SPEED(PcieLinkStatus) \
    ((PcieLinkStatus) & 0x0f)
#define PCIE_CAP_NEGO_LINK_WIDTH(PcieLinkStatus) \
    (((PcieLinkStatus) >> 4) & 0x3f)
#define PCIE_CAP_LINK_TRAINING(PcieLinkStatus) \
    (((PcieLinkStatus) >> 11) & 0x1)
#define PCIE_CAP_SLOT_CLK_CONF(PcieLinkStatus) \
    (((PcieLinkStatus) >> 12) & 0x1)
#define PCIE_CAP_DATA_LINK_ACTIVE(PcieLinkStatus) \
    (((PcieLinkStatus) >> 13) & 0x1)
#define PCIE_CAP_LINK_BDW_MNG_STAT(PcieLinkStatus) \
    (((PcieLinkStatus) >> 14) & 0x1)
#define PCIE_CAP_LINK_AUTO_BDW_STAT(PcieLinkStatus) \
    (((PcieLinkStatus) >> 15) & 0x1)
//
// Slot Capabilities Register
//
#define PCIE_CAP_ATT_BUT_PRESENT(PcieSlotCap) \
    ((PcieSlotCap) & 0x1)
#define PCIE_CAP_PWR_CTRLLER_PRESENT(PcieSlotCap) \
    (((PcieSlotCap) >> 1) & 0x1)
#define PCIE_CAP_MRL_SENSOR_PRESENT(PcieSlotCap) \
    (((PcieSlotCap) >> 2) & 0x1)
#define PCIE_CAP_ATT_IND_PRESENT(PcieSlotCap) \
    (((PcieSlotCap) >> 3) & 0x1)
#define PCIE_CAP_PWD_IND_PRESENT(PcieSlotCap) \
    (((PcieSlotCap) >> 4) & 0x1)
#define PCIE_CAP_HOTPLUG_SUPPRISE(PcieSlotCap) \
    (((PcieSlotCap) >> 5) & 0x1)
#define PCIE_CAP_HOTPLUG_CAPABLE(PcieSlotCap) \
    (((PcieSlotCap) >> 6) & 0x1)
#define PCIE_CAP_SLOT_PWR_LIMIT_VALUE(PcieSlotCap) \
    (((PcieSlotCap) >> 7) & 0x0ff)
#define PCIE_CAP_SLOT_PWR_LIMIT_SCALE(PcieSlotCap) \
    (((PcieSlotCap) >> 15) & 0x3)
#define PCIE_CAP_ELEC_INTERLOCK_PRESENT(PcieSlotCap) \
    (((PcieSlotCap) >> 17) & 0x1)
#define PCIE_CAP_NO_COMM_COMPLETED_SUP(PcieSlotCap) \
    (((PcieSlotCap) >> 18) & 0x1)
#define PCIE_CAP_PHY_SLOT_NUM(PcieSlotCap) \
    (((PcieSlotCap) >> 19) & 0x1fff)
//
// Slot Control Register
//
#define PCIE_CAP_ATT_BUT_ENABLE(PcieSlotControl) \
    ((PcieSlotControl) & 0x1)
#define PCIE_CAP_PWR_FLT_DETECT_ENABLE(PcieSlotControl) \
    (((PcieSlotControl) >> 1) & 0x1)
#define PCIE_CAP_MRL_SENSOR_CHANGE_ENABLE(PcieSlotControl) \
    (((PcieSlotControl) >> 2) & 0x1)
#define PCIE_CAP_PRES_DETECT_CHANGE_ENABLE(PcieSlotControl) \
    (((PcieSlotControl) >> 3) & 0x1)
#define PCIE_CAP_COMM_CMPL_INT_ENABLE(PcieSlotControl) \
    (((PcieSlotControl) >> 4) & 0x1)
#define PCIE_CAP_HOTPLUG_INT_ENABLE(PcieSlotControl) \
    (((PcieSlotControl) >> 5) & 0x1)
#define PCIE_CAP_ATT_IND_CTRL(PcieSlotControl) \
    (((PcieSlotControl) >> 6) & 0x3)
#define PCIE_CAP_PWR_IND_CTRL(PcieSlotControl) \
    (((PcieSlotControl) >> 8) & 0x3)
#define PCIE_CAP_PWR_CTRLLER_CTRL(PcieSlotControl) \
    (((PcieSlotControl) >> 10) & 0x1)
#define PCIE_CAP_ELEC_INTERLOCK_CTRL(PcieSlotControl) \
    (((PcieSlotControl) >> 11) & 0x1)
#define PCIE_CAP_DLINK_STAT_CHANGE_ENABLE(PcieSlotControl) \
    (((PcieSlotControl) >> 12) & 0x1)
//
// Slot Status Register
//
#define PCIE_CAP_ATT_BUT_PRESSED(PcieSlotStatus) \
    ((PcieSlotStatus) & 0x1)
#define PCIE_CAP_PWR_FLT_DETECTED(PcieSlotStatus) \
    (((PcieSlotStatus) >> 1) & 0x1)
#define PCIE_CAP_MRL_SENSOR_CHANGED(PcieSlotStatus) \
    (((PcieSlotStatus) >> 2) & 0x1)
#define PCIE_CAP_PRES_DETECT_CHANGED(PcieSlotStatus) \
    (((PcieSlotStatus) >> 3) & 0x1)
#define PCIE_CAP_COMM_COMPLETED(PcieSlotStatus) \
    (((PcieSlotStatus) >> 4) & 0x1)
#define PCIE_CAP_MRL_SENSOR_STATE(PcieSlotStatus) \
    (((PcieSlotStatus) >> 5) & 0x1)
#define PCIE_CAP_PRES_DETECT_STATE(PcieSlotStatus) \
    (((PcieSlotStatus) >> 6) & 0x1)
#define PCIE_CAP_ELEC_INTERLOCK_STATE(PcieSlotStatus) \
    (((PcieSlotStatus) >> 7) & 0x1)
#define PCIE_CAP_DLINK_STAT_CHANGED(PcieSlotStatus) \
    (((PcieSlotStatus) >> 8) & 0x1)
//
// Root Control Register
//
#define PCIE_CAP_SYSERR_ON_CORERR_EN(PcieRootControl) \
    ((PcieRootControl) & 0x1)
#define PCIE_CAP_SYSERR_ON_NONFATERR_EN(PcieRootControl) \
    (((PcieRootControl) >> 1) & 0x1)
#define PCIE_CAP_SYSERR_ON_FATERR_EN(PcieRootControl) \
    (((PcieRootControl) >> 2) & 0x1)
#define PCIE_CAP_PME_INT_ENABLE(PcieRootControl) \
    (((PcieRootControl) >> 3) & 0x1)
#define PCIE_CAP_CRS_SW_VIS_ENABLE(PcieRootControl) \
    (((PcieRootControl) >> 4) & 0x1)
//
// Root Capabilities Register
//
#define PCIE_CAP_CRS_SW_VIS(PcieRootCap) \
    ((PcieRootCap) & 0x1)
//
// Root Status Register
//
#define PCIE_CAP_PME_REQ_ID(PcieRootStatus) \
    ((PcieRootStatus) & 0x0ffff)
#define PCIE_CAP_PME_STATUS(PcieRootStatus) \
    (((PcieRootStatus) >> 16) & 0x1)
#define PCIE_CAP_PME_PENDING(PcieRootStatus) \
    (((PcieRootStatus) >> 17) & 0x1)

#pragma pack(1)
//
// Common part of the PCI configuration space header for devices, P2P bridges,
// and cardbus bridges
//
typedef struct {
  UINT16  VendorId;
  UINT16  DeviceId;

  UINT16  Command;
  UINT16  Status;

  UINT8   RevisionId;
  UINT8   ClassCode[3];

  UINT8   CacheLineSize;
  UINT8   PrimaryLatencyTimer;
  UINT8   HeaderType;
  UINT8   Bist;

} PCI_COMMON_HEADER;

//
// PCI configuration space header for devices(after the common part)
//
typedef struct {
  UINT32  Bar[6];           // Base Address Registers
  UINT32  CardBusCISPtr;    // CardBus CIS Pointer
  UINT16  SubVendorId;      // Subsystem Vendor ID
  UINT16  SubSystemId;      // Subsystem ID
  UINT32  ROMBar;           // Expansion ROM Base Address
  UINT8   CapabilitiesPtr;  // Capabilities Pointer
  UINT8   Reserved[3];

  UINT32  Reserved1;

  UINT8   InterruptLine;    // Interrupt Line
  UINT8   InterruptPin;     // Interrupt Pin
  UINT8   MinGnt;           // Min_Gnt
  UINT8   MaxLat;           // Max_Lat
} PCI_DEVICE_HEADER;

//
// PCI configuration space header for pci-to-pci bridges(after the common part)
//
typedef struct {
  UINT32  Bar[2];                 // Base Address Registers
  UINT8   PrimaryBus;             // Primary Bus Number
  UINT8   SecondaryBus;           // Secondary Bus Number
  UINT8   SubordinateBus;         // Subordinate Bus Number
  UINT8   SecondaryLatencyTimer;  // Secondary Latency Timer
  UINT8   IoBase;                 // I/O Base
  UINT8   IoLimit;                // I/O Limit
  UINT16  SecondaryStatus;        // Secondary Status
  UINT16  MemoryBase;             // Memory Base
  UINT16  MemoryLimit;            // Memory Limit
  UINT16  PrefetchableMemBase;    // Pre-fetchable Memory Base
  UINT16  PrefetchableMemLimit;   // Pre-fetchable Memory Limit
  UINT32  PrefetchableBaseUpper;  // Pre-fetchable Base Upper 32 bits
  UINT32  PrefetchableLimitUpper; // Pre-fetchable Limit Upper 32 bits
  UINT16  IoBaseUpper;            // I/O Base Upper 16 bits
  UINT16  IoLimitUpper;           // I/O Limit Upper 16 bits
  UINT8   CapabilitiesPtr;        // Capabilities Pointer
  UINT8   Reserved[3];

  UINT32  ROMBar;                 // Expansion ROM Base Address
  UINT8   InterruptLine;          // Interrupt Line
  UINT8   InterruptPin;           // Interrupt Pin
  UINT16  BridgeControl;          // Bridge Control
} PCI_BRIDGE_HEADER;

//
// PCI configuration space header for cardbus bridges(after the common part)
//
typedef struct {
  UINT32  CardBusSocketReg; // Cardus Socket/ExCA Base
  // Address Register
  //
  UINT8   CapabilitiesPtr;      // 14h in pci-cardbus bridge.
  UINT8   Reserved;
  UINT16  SecondaryStatus;      // Secondary Status
  UINT8   PciBusNumber;         // PCI Bus Number
  UINT8   CardBusBusNumber;     // CardBus Bus Number
  UINT8   SubordinateBusNumber; // Subordinate Bus Number
  UINT8   CardBusLatencyTimer;  // CardBus Latency Timer
  UINT32  MemoryBase0;          // Memory Base Register 0
  UINT32  MemoryLimit0;         // Memory Limit Register 0
  UINT32  MemoryBase1;
  UINT32  MemoryLimit1;
  UINT32  IoBase0;
  UINT32  IoLimit0;             // I/O Base Register 0
  UINT32  IoBase1;              // I/O Limit Register 0
  UINT32  IoLimit1;

  UINT8   InterruptLine;        // Interrupt Line
  UINT8   InterruptPin;         // Interrupt Pin
  UINT16  BridgeControl;        // Bridge Control
} PCI_CARDBUS_HEADER;

//
// Data region after PCI configuration header(for cardbus bridge)
//
typedef struct {
  UINT16  SubVendorId;  // Subsystem Vendor ID
  UINT16  SubSystemId;  // Subsystem ID
  UINT32  LegacyBase;   // Optional 16-Bit PC Card Legacy
  // Mode Base Address
  //
  UINT32  Data[46];
} PCI_CARDBUS_DATA;

typedef union {
  PCI_DEVICE_HEADER   Device;
  PCI_BRIDGE_HEADER   Bridge;
  PCI_CARDBUS_HEADER  CardBus;
} NON_COMMON_UNION;

typedef struct {
  PCI_COMMON_HEADER Common;
  NON_COMMON_UNION NonCommon;
  UINT32  Data[48];
} PCI_CONFIG_SPACE;

typedef struct {
  UINT8   PcieCapId;
  UINT8   NextCapPtr;
  UINT16  PcieCapReg;
  UINT32  PcieDeviceCap;
  UINT16  DeviceControl;
  UINT16  DeviceStatus;
  UINT32  LinkCap;
  UINT16  LinkControl;
  UINT16  LinkStatus;
  UINT32  SlotCap;
  UINT16  SlotControl;
  UINT16  SlotStatus;
  UINT16  RootControl;
  UINT16  RsvdP;
  UINT32  RootStatus;
} PCIE_CAP_STRUCTURE;

#pragma pack()

#endif // _PCI_H_
