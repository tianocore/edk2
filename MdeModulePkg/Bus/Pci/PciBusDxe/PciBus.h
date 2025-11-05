/** @file
  Header files and data structures needed by PCI Bus module.

Copyright (c) 2006 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_PCI_BUS_H_
#define _EFI_PCI_BUS_H_

#include <PiDxe.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <Protocol/PciIo.h>
#include <Protocol/LoadFile2.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciHotPlugRequest.h>
#include <Protocol/DevicePath.h>
#include <Protocol/PciPlatform.h>
#include <Protocol/PciHotPlugInit.h>
#include <Protocol/Decompress.h>
#include <Protocol/BusSpecificDriverOverride.h>
#include <Protocol/IncompatiblePciDeviceSupport.h>
#include <Protocol/PciOverride.h>
#include <Protocol/PciEnumerationComplete.h>
#include <Protocol/IoMmu.h>
#include <Protocol/DeviceSecurity.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/PeImage.h>
#include <IndustryStandard/Acpi.h>

typedef struct _PCI_IO_DEVICE  PCI_IO_DEVICE;
typedef struct _PCI_BAR        PCI_BAR;

#define EFI_PCI_RID(Bus, Device, Function)  (((UINT32)Bus << 8) + ((UINT32)Device << 3) + (UINT32)Function)
#define EFI_PCI_BUS_OF_RID(RID)             ((UINT32)RID >> 8)

#define     EFI_PCI_IOV_POLICY_ARI    0x0001
#define     EFI_PCI_IOV_POLICY_SRIOV  0x0002
#define     EFI_PCI_IOV_POLICY_MRIOV  0x0004

typedef enum {
  PciBarTypeUnknown = 0,
  PciBarTypeIo16,
  PciBarTypeIo32,
  PciBarTypeMem32,
  PciBarTypePMem32,
  PciBarTypeMem64,
  PciBarTypePMem64,
  PciBarTypeOpRom,
  PciBarTypeIo,
  PciBarTypeMem,
  PciBarTypeMaxType
} PCI_BAR_TYPE;

#include "ComponentName.h"
#include "PciIo.h"
#include "PciCommand.h"
#include "PciDeviceSupport.h"
#include "PciEnumerator.h"
#include "PciEnumeratorSupport.h"
#include "PciDriverOverride.h"
#include "PciRomTable.h"
#include "PciOptionRomSupport.h"
#include "PciPowerManagement.h"
#include "PciHotPlugSupport.h"
#include "PciLib.h"

#define VGABASE1   0x3B0
#define VGALIMIT1  0x3BB

#define VGABASE2   0x3C0
#define VGALIMIT2  0x3DF

#define ISABASE   0x100
#define ISALIMIT  0x3FF

//
// PCI BAR parameters
//
struct _PCI_BAR {
  UINT64          BaseAddress;
  UINT64          Length;
  UINT64          Alignment;
  PCI_BAR_TYPE    BarType;
  BOOLEAN         BarTypeFixed;
  UINT16          Offset;
};

//
// defined in PCI Card Specification, 8.0
//
#define PCI_CARD_MEMORY_BASE_0     0x1C
#define PCI_CARD_MEMORY_LIMIT_0    0x20
#define PCI_CARD_MEMORY_BASE_1     0x24
#define PCI_CARD_MEMORY_LIMIT_1    0x28
#define PCI_CARD_IO_BASE_0_LOWER   0x2C
#define PCI_CARD_IO_BASE_0_UPPER   0x2E
#define PCI_CARD_IO_LIMIT_0_LOWER  0x30
#define PCI_CARD_IO_LIMIT_0_UPPER  0x32
#define PCI_CARD_IO_BASE_1_LOWER   0x34
#define PCI_CARD_IO_BASE_1_UPPER   0x36
#define PCI_CARD_IO_LIMIT_1_LOWER  0x38
#define PCI_CARD_IO_LIMIT_1_UPPER  0x3A
#define PCI_CARD_BRIDGE_CONTROL    0x3E

#define PCI_CARD_PREFETCHABLE_MEMORY_0_ENABLE  BIT8
#define PCI_CARD_PREFETCHABLE_MEMORY_1_ENABLE  BIT9

#define RB_IO_RANGE      1
#define RB_MEM32_RANGE   2
#define RB_PMEM32_RANGE  3
#define RB_MEM64_RANGE   4
#define RB_PMEM64_RANGE  5

#define PPB_BAR_0         0
#define PPB_BAR_1         1
#define PPB_IO_RANGE      2
#define PPB_MEM32_RANGE   3
#define PPB_PMEM32_RANGE  4
#define PPB_PMEM64_RANGE  5
#define PPB_MEM64_RANGE   0xFF

#define P2C_BAR_0  0
#define P2C_MEM_1  1
#define P2C_MEM_2  2
#define P2C_IO_1   3
#define P2C_IO_2   4

#define EFI_BRIDGE_IO32_DECODE_SUPPORTED       0x0001
#define EFI_BRIDGE_PMEM32_DECODE_SUPPORTED     0x0002
#define EFI_BRIDGE_PMEM64_DECODE_SUPPORTED     0x0004
#define EFI_BRIDGE_IO16_DECODE_SUPPORTED       0x0008
#define EFI_BRIDGE_PMEM_MEM_COMBINE_SUPPORTED  0x0010
#define EFI_BRIDGE_MEM64_DECODE_SUPPORTED      0x0020
#define EFI_BRIDGE_MEM32_DECODE_SUPPORTED      0x0040

#define PCI_MAX_HOST_BRIDGE_NUM  0x0010

//
// Define option for attribute
//
#define EFI_SET_SUPPORTS    0
#define EFI_SET_ATTRIBUTES  1

#define PCI_IO_DEVICE_SIGNATURE  SIGNATURE_32 ('p', 'c', 'i', 'o')

struct _PCI_IO_DEVICE {
  UINT32                                       Signature;
  EFI_HANDLE                                   Handle;
  EFI_PCI_IO_PROTOCOL                          PciIo;
  LIST_ENTRY                                   Link;

  EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL    PciDriverOverride;
  EFI_DEVICE_PATH_PROTOCOL                     *DevicePath;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *PciRootBridgeIo;
  EFI_LOAD_FILE2_PROTOCOL                      LoadFile2;

  //
  // PCI configuration space header type
  //
  PCI_TYPE00                                   Pci;

  //
  // Bus number, Device number, Function number
  //
  UINT8                                        BusNumber;
  UINT8                                        DeviceNumber;
  UINT8                                        FunctionNumber;

  //
  // BAR for this PCI Device
  //
  PCI_BAR                                      PciBar[PCI_MAX_BAR];

  //
  // The bridge device this pci device is subject to
  //
  PCI_IO_DEVICE                                *Parent;

  //
  // A linked list for children Pci Device if it is bridge device
  //
  LIST_ENTRY                                   ChildList;

  //
  // TRUE if the PCI bus driver creates the handle for this PCI device
  //
  BOOLEAN                                      Registered;

  //
  // TRUE if the PCI bus driver successfully allocates the resource required by
  // this PCI device
  //
  BOOLEAN                                      Allocated;

  //
  // The attribute this PCI device currently set
  //
  UINT64                                       Attributes;

  //
  // The attributes this PCI device actually supports
  //
  UINT64                                       Supports;

  //
  // The resource decode the bridge supports
  //
  UINT32                                       Decodes;

  //
  // TRUE if the ROM image is from the PCI Option ROM BAR
  //
  BOOLEAN                                      EmbeddedRom;

  //
  // The OptionRom Size
  //
  UINT32                                       RomSize;

  //
  // TRUE if all OpROM (in device or in platform specific position) have been processed
  //
  BOOLEAN                                      AllOpRomProcessed;

  //
  // TRUE if there is any EFI driver in the OptionRom
  //
  BOOLEAN                                      BusOverride;

  //
  // A list tracking reserved resource on a bridge device
  //
  LIST_ENTRY                                   ReservedResourceList;

  //
  // A list tracking image handle of platform specific overriding driver
  //
  LIST_ENTRY                                   OptionRomDriverList;

  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR            *ResourcePaddingDescriptors;
  EFI_HPC_PADDING_ATTRIBUTES                   PaddingAttributes;

  //
  // Bus number ranges for a PCI Root Bridge device
  //
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR            *BusNumberRanges;

  BOOLEAN                                      IsPciExp;
  BOOLEAN                                      IsAriEnabled;
  //
  // For SR-IOV
  //
  UINT8                                        PciExpressCapabilityOffset;
  UINT32                                       AriCapabilityOffset;
  UINT32                                       SrIovCapabilityOffset;
  UINT32                                       MrIovCapabilityOffset;
  PCI_BAR                                      VfPciBar[PCI_MAX_BAR];
  UINT32                                       SystemPageSize;
  UINT16                                       InitialVFs;
  UINT16                                       ReservedBusNum;
  //
  // Per PCI to PCI Bridge spec, I/O window is 4K aligned,
  // but some chipsets support non-standard I/O window alignments less than 4K.
  // This field is used to support this case.
  //
  UINT16                                       BridgeIoAlignment;
  UINT32                                       ResizableBarOffset;
  UINT32                                       ResizableBarNumber;
};

#define PCI_IO_DEVICE_FROM_PCI_IO_THIS(a) \
  CR (a, PCI_IO_DEVICE, PciIo, PCI_IO_DEVICE_SIGNATURE)

#define PCI_IO_DEVICE_FROM_PCI_DRIVER_OVERRIDE_THIS(a) \
  CR (a, PCI_IO_DEVICE, PciDriverOverride, PCI_IO_DEVICE_SIGNATURE)

#define PCI_IO_DEVICE_FROM_LINK(a) \
  CR (a, PCI_IO_DEVICE, Link, PCI_IO_DEVICE_SIGNATURE)

#define PCI_IO_DEVICE_FROM_LOAD_FILE2_THIS(a) \
  CR (a, PCI_IO_DEVICE, LoadFile2, PCI_IO_DEVICE_SIGNATURE)

//
// Global Variables
//
extern EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL  *gIncompatiblePciDeviceSupport;
extern EFI_DRIVER_BINDING_PROTOCOL                   gPciBusDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL                   gPciBusComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL                  gPciBusComponentName2;
extern BOOLEAN                                       gFullEnumeration;
extern UINTN                                         gPciHostBridgeNumber;
extern EFI_HANDLE                                    gPciHostBrigeHandles[PCI_MAX_HOST_BRIDGE_NUM];
extern UINT64                                        gAllOne;
extern UINT64                                        gAllZero;
extern EFI_PCI_PLATFORM_PROTOCOL                     *gPciPlatformProtocol;
extern EFI_PCI_OVERRIDE_PROTOCOL                     *gPciOverrideProtocol;
extern BOOLEAN                                       mReserveIsaAliases;
extern BOOLEAN                                       mReserveVgaAliases;

/**
  Macro that checks whether device is a GFX device.

  @param  _p      Specified device.

  @retval TRUE    Device is a GFX device.
  @retval FALSE   Device is not a GFX device.

**/
#define IS_PCI_GFX(_p)  IS_CLASS2 (_p, PCI_CLASS_DISPLAY, PCI_CLASS_DISPLAY_OTHER)

/**
  Test to see if this driver supports ControllerHandle. Any ControllerHandle
  than contains a gEfiPciRootBridgeIoProtocolGuid protocol can be supported.

  @param  This                Protocol instance pointer.
  @param  Controller          Handle of device to test.
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval EFI_ALREADY_STARTED This driver is already running on this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
PciBusDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Start this driver on ControllerHandle and enumerate Pci bus and start
  all device under PCI bus.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to bind driver to.
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle.
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle.
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
PciBusDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Stop this driver on ControllerHandle. Support stopping any child handles
  created by this driver.

  @param  This              Protocol instance pointer.
  @param  Controller        Handle of device to stop driver on.
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle.
  @retval other             This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
PciBusDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

#endif
