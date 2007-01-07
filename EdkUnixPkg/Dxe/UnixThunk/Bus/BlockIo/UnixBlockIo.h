/*++

Copyright (c) 2004 - 2005, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  UnixBlockIo.h

Abstract:

  Produce block IO abstractions for real devices on your PC using Posix APIs.
  The configuration of what devices to mount or emulate comes from UNIX 
  environment variables. The variables must be visible to the Microsoft* 
  Developer Studio for them to work.

  * Other names and brands may be claimed as the property of others.

--*/

#ifndef _UNIX_BLOCK_IO_H_
#define _UNIX_BLOCK_IO_H_

#define FILENAME_BUFFER_SIZE  80

//
// Language supported for driverconfiguration protocol
//
#define LANGUAGESUPPORTED "eng"

#define UNIX_BLOCK_IO_PRIVATE_SIGNATURE EFI_SIGNATURE_32 ('L', 'X', 'b', 'k')
typedef struct {
  UINTN                       Signature;

  EFI_LOCK                    Lock;

  char                        Filename[FILENAME_BUFFER_SIZE];
  UINTN                       ReadMode;
  UINTN                       Mode;

  int                         fd;

  UINT64                      LastBlock;
  UINTN                       BlockSize;
  UINT64                      NumberOfBlocks;

  EFI_HANDLE                  EfiHandle;
  EFI_BLOCK_IO_PROTOCOL       BlockIo;
  EFI_BLOCK_IO_MEDIA          Media;

  EFI_UNICODE_STRING_TABLE    *ControllerNameTable;

  EFI_UNIX_THUNK_PROTOCOL   *UnixThunk;

} UNIX_BLOCK_IO_PRIVATE;

#define UNIX_BLOCK_IO_PRIVATE_DATA_FROM_THIS(a) \
         CR(a, UNIX_BLOCK_IO_PRIVATE, BlockIo, UNIX_BLOCK_IO_PRIVATE_SIGNATURE)

#define LIST_BUFFER_SIZE  512

//
// Block I/O Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL        gUnixBlockIoDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL        gUnixBlockIoComponentName;
extern EFI_DRIVER_CONFIGURATION_PROTOCOL  gUnixBlockIoDriverConfiguration;
extern EFI_DRIVER_DIAGNOSTICS_PROTOCOL    gUnixBlockIoDriverDiagnostics;

//
// EFI Driver Binding Functions
//
EFI_STATUS
EFIAPI
UnixBlockIoDriverBindingSupported (
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
UnixBlockIoDriverBindingStart (
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
UnixBlockIoDriverBindingStop (
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

EFI_STATUS
SetFilePointer64 (
  IN  UNIX_BLOCK_IO_PRIVATE    *Private,
  IN  INT64                      DistanceToMove,
  OUT UINT64                     *NewFilePointer,
  IN  INT32                      MoveMethod
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
