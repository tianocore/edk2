/** @file
  The EFI Legacy BIOS Patform Protocol is used to mate a Legacy16 
  implementation with this EFI code. The EFI driver that produces 
  the Legacy BIOS protocol is generic and consumes this protocol.
  A driver that matches the Legacy16 produces this protocol

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    

  Module Name:  LegacyBiosPlatform.h

  @par Revision Reference:
  This protocol is defined in Framework for EFI Compatibility Support Module spec
  Version 0.96
**/

#ifndef _EFI_LEGACY_BIOS_PLATFORM_H
#define _EFI_LEGACY_BIOS_PLATFORM_H

#define EFI_LEGACY_BIOS_PLATFORM_PROTOCOL_GUID \
  { \
    0x783658a3, 0x4172, 0x4421, {0xa2, 0x99, 0xe0, 0x9, 0x7, 0x9c, 0xc, 0xb4 } \
  }

typedef struct _EFI_LEGACY_BIOS_PLATFORM_PROTOCOL EFI_LEGACY_BIOS_PLATFORM_PROTOCOL;

#pragma pack(1)
//
// Define structures for GetOemIntData
//  Note:
//    OemIntDataElenent is an array of structures from 0 to Count-1.
//    RawData is an array of bytes from 0 to RamDataLength-1.
//
typedef struct {
  UINT16  Int;
  UINT16  Ax;
  UINT32  RawDataLength;
  UINT8   RawData[1];
} EFI_OEM_INT_DATA_ELEMENT;

typedef struct {
  UINT16                    Count;
  EFI_OEM_INT_DATA_ELEMENT  OemIntDataElement[1];
} EFI_OEM_INT_DATA;
#pragma pack()

typedef enum {
  EfiGetPlatformBinaryMpTable      = 0,
  EfiGetPlatformBinaryOemIntData   = 1,
  EfiGetPlatformBinaryOem16Data    = 2,
  EfiGetPlatformBinaryOem32Data    = 3,
  EfiGetPlatformBinaryTpmBinary    = 4,
  EfiGetPlatformBinarySystemRom    = 5,
  EfiGetPlatformPciExpressBase     = 6,
  EfiGetPlatformPmmSize            = 7,
  EfiGetPlatformEndOpromShadowAddr = 8,

} EFI_GET_PLATFORM_INFO_MODE;

typedef enum {
  EfiGetPlatformVgaHandle       = 0,
  EfiGetPlatformIdeHandle       = 1,
  EfiGetPlatformIsaBusHandle    = 2,
  EfiGetPlatformUsbHandle       = 3
} EFI_GET_PLATFORM_HANDLE_MODE;

typedef enum {
  EfiPlatformHookPrepareToScanRom = 0,
  EfiPlatformHookShadowServiceRoms= 1,
  EfiPlatformHookAfterRomInit     = 2
} EFI_GET_PLATFORM_HOOK_MODE;

/**
  Finds the binary data or other platform information.

  @param  This                  Protocol instance pointer.
  @param  Mode                  Specifies what data to return
  @param  Table                 Pointer to MP table.
  @param  TableSize             Size in bytes of table.
  @param  Location              Legacy region requested
                                0x00 = Any location
                                Bit 0 = 0xF0000 region
                                Bit 1 = 0xE0000 region
                                Multiple bits can be set
  @param  Alignment             Address alignment for allocation.
                                Bit mapped. First non-zero bit from right
                                is alignment.
  @param  LegacySegment         Segment in LegacyBios where Table is stored
  @param  LegacyOffset          Offset in LegacyBios where Table is stored

  @retval EFI_SUCCESS           Data was returned successfully.
  @retval EFI_UNSUPPORTED       Mode is not supported on the platform.
  @retval EFI_NOT_FOUND         Binary image or table not found.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_GET_PLATFORM_INFO) (
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   *This,
  IN EFI_GET_PLATFORM_INFO_MODE          Mode,
  OUT VOID                               **Table,
  OUT UINTN                              *TableSize,
  OUT UINTN                              *Location,
  OUT UINTN                              *Alignment,
  IN  UINT16                             LegacySegment,
  IN  UINT16                             LegacyOffset
  )
;

/**
  Returns a buffer of handles for the requested sub-function.

  @param  This                  Protocol instance pointer.
  @param  Mode                  Specifies what handle to return.
  @param  Type                  Type from Device Path for Handle to represent.
  @param  HandleBuffer          Handles of the device/controller in priority order
                                with HandleBuffer[0] highest priority.
  @param  HandleCount           Number of handles in the buffer.
  @param  AdditionalData        Mode specific.

  @retval EFI_SUCCESS           Handle is valid
  @retval EFI_UNSUPPORTED       Mode is not supported on the platform.
  @retval EFI_NOT_FOUND         Handle is not known

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_GET_PLATFORM_HANDLE) (
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   *This,
  IN EFI_GET_PLATFORM_HANDLE_MODE        Mode,
  IN UINT16                              Type,
  OUT EFI_HANDLE                         **HandleBuffer,
  OUT UINTN                              *HandleCount,
  IN  VOID                               **AdditionalData OPTIONAL
  )
;

/**
  Load and initialize the Legacy BIOS SMM handler.

  @param  This                   Protocol instance pointer.
  @param  EfiToLegacy16BootTable Pointer to Legacy16 boot table.

  @retval EFI_SUCCESS           SMM code loaded.
  @retval EFI_DEVICE_ERROR      SMM code failed to load

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_SMM_INIT) (
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   *This,
  IN  VOID                               *EfiToLegacy16BootTable
  )
;

/**
  Allows platform to perform any required action after a LegacyBios operation.

  @param  This                  Protocol instance pointer.
  @param  Mode                  Specifies what handle to return.
  @param  Type                  Mode specific.
  @param  DeviceHandle          List of PCI devices in the system.
  @param  ShadowAddress         First free OpROM area, after other OpROMs have been dispatched.
  @param  Compatibility16Table  Pointer to Compatibility16Table.
  @param  AdditionalData        Mode specific Pointer to additional data returned - mode specific.

  @retval EFI_SUCCESS           RomImage is valid
  @retval EFI_UNSUPPORTED       Mode is not supported on the platform.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_HOOKS) (
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   *This,
  IN EFI_GET_PLATFORM_HOOK_MODE          Mode,
  IN UINT16                              Type,
  IN  EFI_HANDLE                         DeviceHandle,
  IN  OUT UINTN                          *ShadowAddress,
  IN  EFI_COMPATIBILITY16_TABLE          *Compatibility16Table,
  IN  VOID                               **AdditionalData OPTIONAL
  )
;

/**
  Returns information associated with PCI IRQ routing.

  @param  This                    Protocol instance pointer.
  @param  RoutingTable            Pointer to PCI IRQ Routing table.
  @param  RoutingTableEntries     Number of entries in table.
  @param  LocalPirqTable          $PIR table
  @param  PirqTableSize           $PIR table size
  @param  LocalIrqPriorityTable   List of interrupts in priority order to assign
  @param  IrqPriorityTableEntries Number of entries in priority table

  @retval EFI_SUCCESS           Data was successfully returned.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_GET_ROUTING_TABLE) (
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   *This,
  OUT VOID                               **RoutingTable,
  OUT UINTN                              *RoutingTableEntries,
  OUT VOID                               **LocalPirqTable, OPTIONAL
  OUT UINTN                              *PirqTableSize, OPTIONAL
  OUT VOID                               **LocalIrqPriorityTable, OPTIONAL
  OUT UINTN                              *IrqPriorityTableEntries OPTIONAL
  )
;

/**
  Translates the given PIRQ accounting for bridge

  @param  This                  Protocol instance pointer.
  @param  PciBus                PCI bus number for this device.
  @param  PciDevice             PCI device number for this device.
  @param  PciFunction           PCI function number for this device.
  @param  Pirq                  Input is PIRQ reported by device, output is true PIRQ.
  @param  PciIrq                The IRQ already assigned to the PIRQ or the IRQ to be
                                assigned to the PIRQ.

  @retval EFI_SUCCESS           The PIRQ was translated.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_TRANSLATE_PIRQ) (
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   *This,
  IN  UINTN                              PciBus,
  IN  UINTN                              PciDevice,
  IN  UINTN                              PciFunction,
  IN  OUT UINT8                          *Pirq,
  OUT UINT8                              *PciIrq
  )
;

/**
  Attempt to legacy boot the BootOption. If the EFI contexted has been 
  compromised this function will not return.

  @param  This                   Protocol instance pointer.
  @param  BbsDevicePath          EFI Device Path from BootXXXX variable.
  @param  BbsTable               Internal BBS table.
  @param  LoadOptionSize         Size of LoadOption in size.
  @param  LoadOption             LoadOption from BootXXXX variable
  @param  EfiToLegacy16BootTable Pointer to BootTable structure

  @retval EFI_SUCCESS           Ready to boot.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_PREPARE_TO_BOOT) (
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   *This,
  IN  BBS_BBS_DEVICE_PATH                *BbsDevicePath,
  IN  VOID                               *BbsTable,
  IN  UINT32                             LoadOptionsSize,
  IN  VOID                               *LoadOptions,
  IN  VOID                               *EfiToLegacy16BootTable
  )
;

/**
  @par Protocol Description:
  Abstracts the platform portion of the traditional BIOS. 

  @param GetPlatformInfo
  Gets binary data or other platform information.

  @param GetPlatformHandle
  Returns a buffer of all handles matching the requested subfunction. 

  @param SmmInit
  Loads and initializes the traditional BIOS SMM handler.

  @param PlatformHooks
  Allows platform to perform any required actions after a LegacyBios operation.

  @param GetRoutingTable
  Gets $PIR table. 

  @param TranslatePirq 
  Translates the given PIRQ to the final value after traversing any PCI bridges. 

  @param PrepareToBoot
  Final platform function before the system attempts to boot to a traditional OS. 

**/
struct _EFI_LEGACY_BIOS_PLATFORM_PROTOCOL {
  EFI_LEGACY_BIOS_PLATFORM_GET_PLATFORM_INFO    GetPlatformInfo;
  EFI_LEGACY_BIOS_PLATFORM_GET_PLATFORM_HANDLE  GetPlatformHandle;
  EFI_LEGACY_BIOS_PLATFORM_SMM_INIT             SmmInit;
  EFI_LEGACY_BIOS_PLATFORM_HOOKS                PlatformHooks;
  EFI_LEGACY_BIOS_PLATFORM_GET_ROUTING_TABLE    GetRoutingTable;
  EFI_LEGACY_BIOS_PLATFORM_TRANSLATE_PIRQ       TranslatePirq;
  EFI_LEGACY_BIOS_PLATFORM_PREPARE_TO_BOOT      PrepareToBoot;
};

extern EFI_GUID gEfiLegacyBiosPlatformProtocolGuid;

#endif
