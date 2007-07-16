/** @file
  DiskIo driver that layers it's self on every Block IO protocol in the system.
  DiskIo converts a block oriented device to a byte oriented device.

  Copyright (c) 2006 - 2007, Intel Corporation                                              
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _DISK_IO_H
#define _DISK_IO_H

#include <Uefi.h>
#include <Protocol/BlockIo.h>
#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/DiskIo.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>


#define DISK_IO_PRIVATE_DATA_SIGNATURE  EFI_SIGNATURE_32 ('d', 's', 'k', 'I')

#define DATA_BUFFER_BLOCK_NUM           (64)

typedef struct {
  UINTN                 Signature;
  EFI_DISK_IO_PROTOCOL  DiskIo;
  EFI_BLOCK_IO_PROTOCOL *BlockIo;
} DISK_IO_PRIVATE_DATA;

#define DISK_IO_PRIVATE_DATA_FROM_THIS(a) CR (a, DISK_IO_PRIVATE_DATA, DiskIo, DISK_IO_PRIVATE_DATA_SIGNATURE)

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL  gDiskIoDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gDiskIoComponentName;

//
// Prototypes
// Driver model protocol interface
//
EFI_STATUS
EFIAPI
DiskIoDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
DiskIoDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
DiskIoDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     ControllerHandle,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  );

//
// Disk I/O Protocol Interface
//
EFI_STATUS
EFIAPI
DiskIoReadDisk (
  IN EFI_DISK_IO_PROTOCOL  *This,
  IN UINT32                MediaId,
  IN UINT64                Offset,
  IN UINTN                 BufferSize,
  OUT VOID                 *Buffer
  );

EFI_STATUS
EFIAPI
DiskIoWriteDisk (
  IN EFI_DISK_IO_PROTOCOL  *This,
  IN UINT32                MediaId,
  IN UINT64                Offset,
  IN UINTN                 BufferSize,
  IN VOID                  *Buffer
  );

//
// EFI Component Name Functions
//
EFI_STATUS
EFIAPI
DiskIoComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

EFI_STATUS
EFIAPI
DiskIoComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

#endif
