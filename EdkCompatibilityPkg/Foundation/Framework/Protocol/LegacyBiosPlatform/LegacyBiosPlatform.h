/*++

Copyright (c) 1999 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  LegacyBiosPlatform.h
    
Abstract:

  The EFI Legacy BIOS Patform Protocol is used to mate a Legacy16 
  implementation with this EFI code. The EFI driver that produces 
  the Legacy BIOS protocol is generic and consumes this protocol.
  A driver that matches the Legacy16 produces this protocol
  
Revision History

  The EFI Legacy BIOS Platform Protocol is compliant with CSM spec 0.96.

--*/

#ifndef _EFI_LEGACY_BIOS_PLATFORM_H
#define _EFI_LEGACY_BIOS_PLATFORM_H

#define EFI_LEGACY_BIOS_PLATFORM_PROTOCOL_GUID \
  { \
    0x783658a3, 0x4172, 0x4421, {0xa2, 0x99, 0xe0, 0x9, 0x7, 0x9c, 0xc, 0xb4} \
  }

EFI_FORWARD_DECLARATION (EFI_LEGACY_BIOS_PLATFORM_PROTOCOL);

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION (LegacyBios)

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
  EfiGetPlatformBinaryMpTable        = 0,
  EfiGetPlatformBinaryOemIntData     = 1,
  EfiGetPlatformBinaryOem16Data      = 2,
  EfiGetPlatformBinaryOem32Data      = 3,
  EfiGetPlatformBinaryTpmBinary      = 4,
  EfiGetPlatformBinarySystemRom      = 5,
  EfiGetPlatformPciExpressBase       = 6,
  EfiGetPlatformPmmSize              = 7,
  EfiGetPlatformEndOfOpromShadowAddr = 8
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

typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_GET_PLATFORM_INFO) (
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   * This,
  IN EFI_GET_PLATFORM_INFO_MODE          Mode,
  OUT VOID                               **Table,
  OUT UINTN                              *TableSize,
  OUT UINTN                              *Location,
  OUT UINTN                              *Alignment,
  IN  UINT16                             LegacySegment,
  IN  UINT16                             LegacyOffset
  )
/*++

  Routine Description:
    Return a System ROM image for the platform

  Arguments:
    This                   - Protocol instance pointer.
    Mode                   - Specifies what data to return
    Table                  - Pointer to MP table.
    TableSize              - Size in bytes of table.
    Location               - Legacy region requested
                               0x00 = Any location
                               Bit 0 = 0xF0000 region
                               Bit 1 = 0xE0000 region
                               Multiple bits can be set
    Alignment              - Address alignment for allocation.
                             Bit mapped. First non-zero bit from right
                             is alignment.
                             
    LegacySegment          - Segment in LegacyBios where Table is stored
    LegacyOffset           - Offset in LegacyBios where Table is stored

  Returns:
    EFI_SUCCESS     - Data was returned successfully.
    EFI_UNSUPPORTED - Mode is not supported on the platform.
    EFI_NOT_FOUND   - Binary image or table not found.

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_GET_PLATFORM_HANDLE) (
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   * This,
  IN EFI_GET_PLATFORM_HANDLE_MODE        Mode,
  IN UINT16                              Type,
  OUT EFI_HANDLE                         **HandleBuffer,
  OUT UINTN                              *HandleCount,
  IN  VOID                               **AdditionalData OPTIONAL
  )
/*++

  Routine Description:
    Return the Legacy16 policy for which device should be the VGA controller
    used during a Legacy16 boot.

  Arguments:
    This             - Protocol instance pointer.
    Mode             - Specifies what handle to return.
    Type             - Type from Device Path for Handle to represent.
    HandleBuffer     - Handles of the device/controller in priority order 
                       with HandleBuffer[0] highest priority.
    HandleCount      - Number of handles in the buffer.
    AdditionalData   - Mode specific.


  Returns:
    EFI_SUCCESS     - Handle is valid
    EFI_UNSUPPORTED - Mode is not supported on the platform.
    EFI_NOT_FOUND   - Handle is not known

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_SMM_INIT) (
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   * This,
  IN  VOID                               *EfiToLegacy16BootTable
  );

/*++

  Routine Description:
    Load and initialize the Legacy BIOS SMM handler.

  Arguments:
    This                   - Protocol instance pointer.
    EfiToLegacy16BootTable - Pointer to Legacy16 boot table.
  Returns:
    EFI_SUCCESS      - SMM code loaded.
    EFI_DEVICE_ERROR - SMM code failed to load

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_HOOKS) (
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   * This,
  IN EFI_GET_PLATFORM_HOOK_MODE          Mode,
  IN UINT16                              Type,
  IN  EFI_HANDLE                         DeviceHandle,
  IN  OUT UINTN                          *ShadowAddress,
  IN  EFI_COMPATIBILITY16_TABLE          * Compatibility16Table,
  IN  VOID                               **AdditionalData OPTIONAL
  )
/*++

  Routine Description:
    Prepare to scan a ROM. 

  Arguments:
    This                   - Protocol instance pointer.
    Handle                 - Device handle
    ShadowAddress          - Address that ROM is shadowed at prior to 
                             initialization or first free ROM address,
                             depending upon mode.
    Compatibility16Table   - Pointer to Compatibility16Table.
    AdditionalData   - Mode specific.
      

  Returns:
    EFI_SUCCESS     - RomImage is valid
    EFI_UNSUPPORTED - Mode is not supported on the platform or platform 
                      policy is to not install this OPROM.
--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_GET_ROUTING_TABLE) (
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   * This,
  OUT VOID                               **RoutingTable,
  OUT UINTN                              *RoutingTableEntries,
  OUT VOID                               **LocalPirqTable, OPTIONAL
  OUT UINTN                              *PirqTableSize, OPTIONAL
  OUT VOID                               **LocalIrqPriorityTable, OPTIONAL
  OUT UINTN                              *IrqPriorityTableEntries OPTIONAL
  )
/*++

  Routine Description:
      1. List of IRQ routing entries and number of entries.
      2. Pointer to Entire $PIR table and length.
      3. List of IRQs to assign to PCI in priority.

  Arguments:
    This                - Protocol instance pointer.
    RoutingTable        - Pointer to PCI IRQ Routing table.
    RoutingTableEntries - Number of entries in table.
    LocalPirqTable         - $PIR table
    PirqTableSize          - $PIR table size
    LocalIrqPriorityTable  - List of interrupts in priority order to assign
    IrqPriorityTableEntries- Number of entries in priority table

  Returns:
    EFI_SUCCESS   - Table pointer returned

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_TRANSLATE_PIRQ) (
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   * This,
  IN  UINTN                              PciBus,
  IN  UINTN                              PciDevice,
  IN  UINTN                              PciFunction,
  IN  OUT UINT8                          *Pirq,
  OUT UINT8                              *PciIrq
  )
/*++

  Routine Description:
    Translate the PIRQ reported by the PCI device into the true PIRQ 
    from the internal IRQ routing information and IRQ assigned or to be
    assigned to device.

  Arguments:                           
    This        - Protocol instance pointer.
                  Base as defined below is the bus,device, function in
                  IRQ routing table.
    PciBus      - Base Bus for this device.
    PciDevice   - Base Device for this device.
    PciFunction - Base Function for this device.
    Pirq        - Input is PIRQ reported by device, output is true PIRQ.
    PciIrq      - The IRQ already assigned to the PIRQ or the IRQ to be
                  assigned to the PIRQ.

  Returns:
    EFI_SUCCESS  - Irq translated

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_PREPARE_TO_BOOT) (
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   * This,
  IN  BBS_BBS_DEVICE_PATH                * BbsDevicePath,
  IN  VOID                               *BbsTable,
  IN  UINT32                             LoadOptionsSize,
  IN  VOID                               *LoadOptions,
  IN  VOID                               *EfiToLegacy16BootTable
  )
/*++

  Routine Description:
    Attempt to legacy boot the BootOption. If the EFI contexted has been 
    compromised this function will not return.

  Arguments:
    This                      - Protocol instance pointer.
    BbsDevicePath             - EFI Device Path from BootXXXX variable.
    BbsTable                  - Internal BBS table.
    LoadOptionSize            - Size of LoadOption in size.
    LoadOption                - LoadOption from BootXXXX variable
    EfiToLegacy16BootTable    - Pointer to BootTable structure

  Returns:
    EFI_SUCCESS     - Removable media not present

--*/
;

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
