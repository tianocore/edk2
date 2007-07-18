/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciHotPlugSupport.h
  
Abstract:

  

Revision History

--*/

#ifndef _EFI_PCI_HOT_PLUG_SUPPORT_H
#define _EFI_PCI_HOT_PLUG_SUPPORT_H


//
// stall 1 second
//
#define STALL_1_SECOND        1000000 

typedef struct {
  EFI_EVENT Event;
  BOOLEAN   Initialized;
  VOID      *Padding;
} ROOT_HPC_DATA;

extern EFI_PCI_HOT_PLUG_INIT_PROTOCOL *gPciHotPlugInit;
extern EFI_HPC_LOCATION               *gPciRootHpcPool;
extern UINTN                          gPciRootHpcCount;
extern ROOT_HPC_DATA                  *gPciRootHpcData;

VOID
EFIAPI
PciHPCInitialized (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Event   - TODO: add argument description
  Context - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
EfiCompareDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL *DevicePath1,
  IN EFI_DEVICE_PATH_PROTOCOL *DevicePath2
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  DevicePath1 - TODO: add argument description
  DevicePath2 - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
InitializeHotPlugSupport (
  VOID
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  None

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
IsPciHotPlugBus (
  PCI_IO_DEVICE                       *PciIoDevice
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

BOOLEAN
IsRootPciHotPlugBus (
  IN EFI_DEVICE_PATH_PROTOCOL         *HpbDevicePath,
  OUT UINTN                           *HpIndex
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  HpbDevicePath - TODO: add argument description
  HpIndex       - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
IsRootPciHotPlugController (
  IN EFI_DEVICE_PATH_PROTOCOL         *HpcDevicePath,
  OUT UINTN                           *HpIndex
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  HpcDevicePath - TODO: add argument description
  HpIndex       - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
CreateEventForHpc (
  IN UINTN       HpIndex,
  OUT EFI_EVENT  *Event
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  HpIndex - TODO: add argument description
  Event   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AllRootHPCInitialized (
  IN  UINTN           TimeoutInMicroSeconds
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:
  TimeoutInMicroSeconds - microseconds to wait for all root hpc's initialization

Returns:
  EFI_SUCCESS - All root hpc's initialization is finished before the timeout
  EFI_TIMEOUT - Time out

--*/
;

EFI_STATUS
IsSHPC (
  PCI_IO_DEVICE                       *PciIoDevice
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
GetResourcePaddingForHpb (
  IN PCI_IO_DEVICE *PciIoDevice
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

#endif
