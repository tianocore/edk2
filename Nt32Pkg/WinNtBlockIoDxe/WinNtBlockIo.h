/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  WinNtBlockIo.h

Abstract:

  Produce block IO abstractions for real devices on your PC using Win32 APIs.
  The configuration of what devices to mount or emulate comes from NT 
  environment variables. The variables must be visible to the Microsoft* 
  Developer Studio for them to work.

  * Other names and brands may be claimed as the property of others.

**/

#ifndef _WIN_NT_BLOCK_IO_H_
#define _WIN_NT_BLOCK_IO_H_

#include <Uefi.h>
#include <WinNtDxe.h>
#include <Protocol/WinNtThunk.h>
#include <Protocol/WinNtIo.h>
#include <Protocol/BlockIo.h>
#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#define FILENAME_BUFFER_SIZE  80

//
// Language supported for driverconfiguration protocol
//
#define LANGUAGESUPPORTED "eng"

typedef enum {
  EfiWinNtVirtualDisks,
  EfiWinNtPhysicalDisks,
  EifWinNtMaxTypeDisks
} WIN_NT_RAW_DISK_DEVICE_TYPE;

#define WIN_NT_BLOCK_IO_PRIVATE_SIGNATURE SIGNATURE_32 ('N', 'T', 'b', 'k')
typedef struct {
  UINTN                       Signature;

  EFI_LOCK                    Lock;

  CHAR16                      Filename[FILENAME_BUFFER_SIZE];
  UINTN                       ReadMode;
  UINTN                       ShareMode;
  UINTN                       OpenMode;

  HANDLE                      NtHandle;
  WIN_NT_RAW_DISK_DEVICE_TYPE DeviceType;

  UINT64                      LastBlock;
  UINTN                       BlockSize;
  UINT64                      NumberOfBlocks;

  EFI_HANDLE                  EfiHandle;
  EFI_BLOCK_IO_PROTOCOL       BlockIo;
  EFI_BLOCK_IO_MEDIA          Media;

  EFI_UNICODE_STRING_TABLE    *ControllerNameTable;

  EFI_WIN_NT_THUNK_PROTOCOL   *WinNtThunk;

} WIN_NT_BLOCK_IO_PRIVATE;

#define WIN_NT_BLOCK_IO_PRIVATE_DATA_FROM_THIS(a) \
         CR(a, WIN_NT_BLOCK_IO_PRIVATE, BlockIo, WIN_NT_BLOCK_IO_PRIVATE_SIGNATURE)

#define LIST_BUFFER_SIZE  512

//
// Block I/O Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL         gWinNtBlockIoDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL         gWinNtBlockIoComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL        gWinNtBlockIoComponentName2;
extern EFI_DRIVER_CONFIGURATION_PROTOCOL   gWinNtBlockIoDriverConfiguration;
extern EFI_DRIVER_DIAGNOSTICS_PROTOCOL     gWinNtBlockIoDriverDiagnostics;
extern EFI_DRIVER_DIAGNOSTICS2_PROTOCOL    gWinNtBlockIoDriverDiagnostics2;

//
// EFI Driver Binding Functions
//
EFI_STATUS
EFIAPI
WinNtBlockIoDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                    Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This                - TODO: add argument description
  Handle              - TODO: add argument description
  RemainingDevicePath - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
WinNtBlockIoDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                    Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This                - TODO: add argument description
  Handle              - TODO: add argument description
  RemainingDevicePath - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
WinNtBlockIoDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Handle,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This              - TODO: add argument description
  Handle            - TODO: add argument description
  NumberOfChildren  - TODO: add argument description
  ChildHandleBuffer - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

//
// Block IO protocol member functions
//
EFI_STATUS
EFIAPI
WinNtBlockIoReadBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  OUT VOID                  *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  MediaId     - TODO: add argument description
  Lba         - TODO: add argument description
  BufferSize  - TODO: add argument description
  Buffer      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
WinNtBlockIoWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  IN VOID                   *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  MediaId     - TODO: add argument description
  Lba         - TODO: add argument description
  BufferSize  - TODO: add argument description
  Buffer      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
WinNtBlockIoFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
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
WinNtBlockIoResetBlock (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN BOOLEAN                ExtendedVerification
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This                  - TODO: add argument description
  ExtendedVerification  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

//
// Private Worker functions
//
EFI_STATUS
WinNtBlockIoCreateMapping (
  IN EFI_WIN_NT_IO_PROTOCOL             *WinNtIo,
  IN EFI_HANDLE                         EfiDeviceHandle,
  IN CHAR16                             *Filename,
  IN BOOLEAN                            ReadOnly,
  IN BOOLEAN                            RemovableMedia,
  IN UINTN                              NumberOfBlocks,
  IN UINTN                              BlockSize,
  IN WIN_NT_RAW_DISK_DEVICE_TYPE        DeviceType
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  WinNtIo         - TODO: add argument description
  EfiDeviceHandle - TODO: add argument description
  Filename        - TODO: add argument description
  ReadOnly        - TODO: add argument description
  RemovableMedia  - TODO: add argument description
  NumberOfBlocks  - TODO: add argument description
  BlockSize       - TODO: add argument description
  DeviceType      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
WinNtBlockIoReadWriteCommon (
  IN  WIN_NT_BLOCK_IO_PRIVATE *Private,
  IN UINT32                   MediaId,
  IN EFI_LBA                  Lba,
  IN UINTN                    BufferSize,
  IN VOID                     *Buffer,
  IN CHAR8                    *CallerName
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Private     - TODO: add argument description
  MediaId     - TODO: add argument description
  Lba         - TODO: add argument description
  BufferSize  - TODO: add argument description
  Buffer      - TODO: add argument description
  CallerName  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
WinNtBlockIoError (
  IN WIN_NT_BLOCK_IO_PRIVATE      *Private
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Private - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
WinNtBlockIoOpenDevice (
  WIN_NT_BLOCK_IO_PRIVATE         *Private
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Private - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

CHAR16                                    *
GetNextElementPastTerminator (
  IN  CHAR16  *EnvironmentVariable,
  IN  CHAR16  Terminator
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  EnvironmentVariable - TODO: add argument description
  Terminator          - TODO: add argument description

Returns:

  TODO: add return values

--*/
;



EFI_STATUS
SetFilePointer64 (
  IN  WIN_NT_BLOCK_IO_PRIVATE    *Private,
  IN  INT64                      DistanceToMove,
  OUT UINT64                     *NewFilePointer,
  IN  DWORD                      MoveMethod
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Private         - TODO: add argument description
  DistanceToMove  - TODO: add argument description
  NewFilePointer  - TODO: add argument description
  MoveMethod      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

UINTN
Atoi (
  CHAR16  *String
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  String  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

#endif
