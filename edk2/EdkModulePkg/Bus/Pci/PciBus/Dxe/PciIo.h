/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciIo.h
  
Abstract:

  PCI Bus Driver

Revision History

--*/

#ifndef _EFI_PCI_IO_PROTOCOL_H
#define _EFI_PCI_IO_PROTOCOL_H

EFI_STATUS
InitializePciIoInstance (
  PCI_IO_DEVICE  *PciIoDevice
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIoDevice - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
PciIoVerifyBarAccess (
  PCI_IO_DEVICE                   *PciIoDevice,
  UINT8                           BarIndex,
  PCI_BAR_TYPE                    Type,
  IN EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN UINTN                        Count,
  UINT64                          *Offset
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIoDevice - TODO: add argument description
  BarIndex    - TODO: add argument description
  Type        - TODO: add argument description
  Width       - TODO: add argument description
  Count       - TODO: add argument description
  Offset      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
PciIoVerifyConfigAccess (
  PCI_IO_DEVICE                 *PciIoDevice,
  IN EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN UINTN                      Count,
  IN UINT64                     *Offset
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIoDevice - TODO: add argument description
  Width       - TODO: add argument description
  Count       - TODO: add argument description
  Offset      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PciIoPollMem (
  IN  EFI_PCI_IO_PROTOCOL        *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN  UINT8                      BarIndex,
  IN  UINT64                     Offset,
  IN  UINT64                     Mask,
  IN  UINT64                     Value,
  IN  UINT64                     Delay,
  OUT UINT64                     *Result
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This      - TODO: add argument description
  Width     - TODO: add argument description
  BarIndex  - TODO: add argument description
  Offset    - TODO: add argument description
  Mask      - TODO: add argument description
  Value     - TODO: add argument description
  Delay     - TODO: add argument description
  Result    - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PciIoPollIo (
  IN  EFI_PCI_IO_PROTOCOL        *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN  UINT8                      BarIndex,
  IN  UINT64                     Offset,
  IN  UINT64                     Mask,
  IN  UINT64                     Value,
  IN  UINT64                     Delay,
  OUT UINT64                     *Result
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This      - TODO: add argument description
  Width     - TODO: add argument description
  BarIndex  - TODO: add argument description
  Offset    - TODO: add argument description
  Mask      - TODO: add argument description
  Value     - TODO: add argument description
  Delay     - TODO: add argument description
  Result    - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PciIoMemRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This      - TODO: add argument description
  Width     - TODO: add argument description
  BarIndex  - TODO: add argument description
  Offset    - TODO: add argument description
  Count     - TODO: add argument description
  Buffer    - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PciIoMemWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This      - TODO: add argument description
  Width     - TODO: add argument description
  BarIndex  - TODO: add argument description
  Offset    - TODO: add argument description
  Count     - TODO: add argument description
  Buffer    - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PciIoIoRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This      - TODO: add argument description
  Width     - TODO: add argument description
  BarIndex  - TODO: add argument description
  Offset    - TODO: add argument description
  Count     - TODO: add argument description
  Buffer    - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PciIoIoWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This      - TODO: add argument description
  Width     - TODO: add argument description
  BarIndex  - TODO: add argument description
  Offset    - TODO: add argument description
  Count     - TODO: add argument description
  Buffer    - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PciIoConfigRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT32                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This    - TODO: add argument description
  Width   - TODO: add argument description
  Offset  - TODO: add argument description
  Count   - TODO: add argument description
  Buffer  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PciIoConfigWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT32                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This    - TODO: add argument description
  Width   - TODO: add argument description
  Offset  - TODO: add argument description
  Count   - TODO: add argument description
  Buffer  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PciIoCopyMem (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        DestBarIndex,
  IN     UINT64                       DestOffset,
  IN     UINT8                        SrcBarIndex,
  IN     UINT64                       SrcOffset,
  IN     UINTN                        Count
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This          - TODO: add argument description
  Width         - TODO: add argument description
  DestBarIndex  - TODO: add argument description
  DestOffset    - TODO: add argument description
  SrcBarIndex   - TODO: add argument description
  SrcOffset     - TODO: add argument description
  Count         - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PciIoMap (
  IN     EFI_PCI_IO_PROTOCOL            *This,
  IN     EFI_PCI_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                           *HostAddress,
  IN OUT UINTN                          *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS           *DeviceAddress,
  OUT    VOID                           **Mapping
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This          - TODO: add argument description
  Operation     - TODO: add argument description
  HostAddress   - TODO: add argument description
  NumberOfBytes - TODO: add argument description
  DeviceAddress - TODO: add argument description
  Mapping       - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PciIoUnmap (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  IN  VOID                 *Mapping
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This    - TODO: add argument description
  Mapping - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PciIoAllocateBuffer (
  IN  EFI_PCI_IO_PROTOCOL   *This,
  IN  EFI_ALLOCATE_TYPE     Type,
  IN  EFI_MEMORY_TYPE       MemoryType,
  IN  UINTN                 Pages,
  OUT VOID                  **HostAddress,
  IN  UINT64                Attributes
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  Type        - TODO: add argument description
  MemoryType  - TODO: add argument description
  Pages       - TODO: add argument description
  HostAddress - TODO: add argument description
  Attributes  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PciIoFreeBuffer (
  IN  EFI_PCI_IO_PROTOCOL   *This,
  IN  UINTN                 Pages,
  IN  VOID                  *HostAddress
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  Pages       - TODO: add argument description
  HostAddress - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PciIoFlush (
  IN  EFI_PCI_IO_PROTOCOL  *This
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PciIoGetLocation (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  OUT UINTN                *Segment,
  OUT UINTN                *Bus,
  OUT UINTN                *Device,
  OUT UINTN                *Function
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This      - TODO: add argument description
  Segment   - TODO: add argument description
  Bus       - TODO: add argument description
  Device    - TODO: add argument description
  Function  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
CheckBarType (
  IN PCI_IO_DEVICE       *PciIoDevice,
  UINT8                  BarIndex,
  PCI_BAR_TYPE           BarType
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIoDevice - TODO: add argument description
  BarIndex    - TODO: add argument description
  BarType     - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ModifyRootBridgeAttributes (
  IN  PCI_IO_DEVICE                            *PciIoDevice,
  IN  UINT64                                   Attributes,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIoDevice - TODO: add argument description
  Attributes  - TODO: add argument description
  Operation   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
SupportPaletteSnoopAttributes (
  IN  PCI_IO_DEVICE                            *PciIoDevice,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIoDevice - TODO: add argument description
  Operation   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PciIoAttributes (
  IN EFI_PCI_IO_PROTOCOL                       * This,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation,
  IN  UINT64                                   Attributes,
  OUT UINT64                                   *Result OPTIONAL
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  Operation   - TODO: add argument description
  Attributes  - TODO: add argument description
  Result      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PciIoGetBarAttributes (
  IN EFI_PCI_IO_PROTOCOL             * This,
  IN  UINT8                          BarIndex,
  OUT UINT64                         *Supports, OPTIONAL
  OUT VOID                           **Resources OPTIONAL
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This      - TODO: add argument description
  BarIndex  - TODO: add argument description
  Supports  - TODO: add argument description
  Resources - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PciIoSetBarAttributes (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     UINT64                       Attributes,
  IN     UINT8                        BarIndex,
  IN OUT UINT64                       *Offset,
  IN OUT UINT64                       *Length
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  Attributes  - TODO: add argument description
  BarIndex    - TODO: add argument description
  Offset      - TODO: add argument description
  Length      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
UpStreamBridgesAttributes (
  IN  PCI_IO_DEVICE                            *PciIoDevice,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation,
  IN  UINT64                                   Attributes
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIoDevice - TODO: add argument description
  Operation   - TODO: add argument description
  Attributes  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
PciDevicesOnTheSamePath (
  IN PCI_IO_DEVICE        *PciDevice1,
  IN PCI_IO_DEVICE        *PciDevice2
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciDevice1  - TODO: add argument description
  PciDevice2  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

#endif
