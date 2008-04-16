/**@file

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/


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

/**
  Init HPC private data.
  
  @param  Event     event object
  @param  Context   HPC private data.
**/
VOID
EFIAPI
PciHPCInitialized (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
;

/**
  Compare two device path
  
  @param DevicePath1    the first device path want to be compared
  @param DevicePath2    the first device path want to be compared
  
  @retval TRUE    equal
  @retval FALSE   different
**/
BOOLEAN
EfiCompareDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL *DevicePath1,
  IN EFI_DEVICE_PATH_PROTOCOL *DevicePath2
  )
;

/**
  Init hot plug support and root hot plug private data.
  
**/
EFI_STATUS
InitializeHotPlugSupport (
  VOID
  )
;

/**
  Test whether PCI device is hot plug bus.
  
  @param PciIoDevice  PCI device instance
  
  @retval EFI_SUCCESS   PCI device is hot plug bus
  @retval EFI_NOT_FOUND PCI device is not hot plug bus
**/
EFI_STATUS
IsPciHotPlugBus (
  PCI_IO_DEVICE                       *PciIoDevice
  )
;

/**
  Test whether device path is for root pci hot plug bus
  
  @param HpbdevicePath  tested device path
  @param HpIndex        Return the index of root hot plug in global array.
  
  @retval TRUE  device path is for root pci hot plug
  @retval FALSE device path is not for root pci hot plug
**/
BOOLEAN
IsRootPciHotPlugBus (
  IN EFI_DEVICE_PATH_PROTOCOL         *HpbDevicePath,
  OUT UINTN                           *HpIndex
  )
;

/**
  Test whether device path is for root pci hot plug controller
  
  @param HpbdevicePath  tested device path
  @param HpIndex        Return the index of root hot plug in global array.
  
  @retval TRUE  device path is for root pci hot plug controller
  @retval FALSE device path is not for root pci hot plug controller
**/
BOOLEAN
IsRootPciHotPlugController (
  IN EFI_DEVICE_PATH_PROTOCOL         *HpcDevicePath,
  OUT UINTN                           *HpIndex
  )
;

/**
  Wrapper for creating event object for HPC 
  
  @param  HpIndex   index of hot plug device in global array
  @param  Event     event object
  
  @return status of create event invoken
**/
EFI_STATUS
CreateEventForHpc (
  IN UINTN       HpIndex,
  OUT EFI_EVENT  *Event
  )
;

/**
  Wait for all root HPC initialized.
  
  @param TimeoutInMicroSeconds  microseconds to wait for all root hpc's initialization
**/
EFI_STATUS
AllRootHPCInitialized (
  IN  UINTN           TimeoutInMicroSeconds
  )
;

/**
  Check HPC capability register block
  
  @param PciIoDevice PCI device instance
  
  @retval EFI_SUCCESS   PCI device is HPC
  @retval EFI_NOT_FOUND PCI device is not HPC
**/
EFI_STATUS
IsSHPC (
  PCI_IO_DEVICE                       *PciIoDevice
  )
;

/**
  Get resource padding for hot plug bus
  
  @param PciIoDevice PCI device instance
  
  @retval EFI_SUCCESS   success get padding and set it into PCI device instance
  @retval EFI_NOT_FOUND PCI device is not a hot plug bus.
**/
EFI_STATUS
GetResourcePaddingForHpb (
  IN PCI_IO_DEVICE *PciIoDevice
  )
;

#endif
