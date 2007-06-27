/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Console.h

Abstract:

  Console based on Win32 APIs.

  This file attaches a SimpleTextIn protocol to a previously open window.
  
  The constructor for this protocol depends on an open window. Currently
  the SimpleTextOut protocol creates a window when it's constructor is called.
  Thus this code must run after the constructor for the SimpleTextOut 
  protocol
  
--*/

#ifndef _CONSOLE_H_
#define _CONSOLE_H_


#define WIN_NT_SIMPLE_TEXT_PRIVATE_DATA_SIGNATURE \
          EFI_SIGNATURE_32('N','T','s','c')

typedef struct {
  UINT64                        Signature;

  EFI_HANDLE                    Handle;

  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  SimpleTextOut;
  EFI_SIMPLE_TEXT_OUTPUT_MODE   SimpleTextOutMode;

  EFI_WIN_NT_IO_PROTOCOL        *WinNtIo;
  EFI_WIN_NT_THUNK_PROTOCOL     *WinNtThunk;

  //
  // SimpleTextOut Private Data including Win32 types.
  //
  HANDLE                        NtOutHandle;
  HANDLE                        NtInHandle;

  COORD                         MaxScreenSize;
  COORD                         Possition;
  WORD                          Attribute;
  BOOLEAN                       CursorEnable;

  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   SimpleTextIn;

  EFI_UNICODE_STRING_TABLE      *ControllerNameTable;

} WIN_NT_SIMPLE_TEXT_PRIVATE_DATA;

#define WIN_NT_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS(a) \
         CR(a, WIN_NT_SIMPLE_TEXT_PRIVATE_DATA, SimpleTextOut, WIN_NT_SIMPLE_TEXT_PRIVATE_DATA_SIGNATURE)

#define WIN_NT_SIMPLE_TEXT_IN_PRIVATE_DATA_FROM_THIS(a) \
         CR(a, WIN_NT_SIMPLE_TEXT_PRIVATE_DATA, SimpleTextIn, WIN_NT_SIMPLE_TEXT_PRIVATE_DATA_SIGNATURE)

//
// Console Globale Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL  gWinNtConsoleDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gWinNtConsoleComponentName;

typedef struct {
  UINTN ColumnsX;
  UINTN RowsY;
} WIN_NT_SIMPLE_TEXT_OUT_MODE;

//
// Simple Text Out protocol member functions
//

STATIC
EFI_STATUS
EFIAPI
WinNtSimpleTextOutReset (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL     *This,
  IN BOOLEAN                          ExtendedVerification
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

STATIC
EFI_STATUS
EFIAPI
WinNtSimpleTextOutOutputString (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *This,
  IN CHAR16                         *String
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This    - TODO: add argument description
  String  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

STATIC
EFI_STATUS
EFIAPI
WinNtSimpleTextOutTestString (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *This,
  IN CHAR16                         *String
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This    - TODO: add argument description
  String  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

STATIC
EFI_STATUS
EFIAPI
WinNtSimpleTextOutQueryMode (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *This,
  IN UINTN                          ModeNumber,
  OUT UINTN                         *Columns,
  OUT UINTN                         *Rows
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  ModeNumber  - TODO: add argument description
  Columns     - TODO: add argument description
  Rows        - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

STATIC
EFI_STATUS
EFIAPI
WinNtSimpleTextOutSetMode (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *This,
  IN UINTN                          ModeNumber
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  ModeNumber  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

STATIC
EFI_STATUS
EFIAPI
WinNtSimpleTextOutSetAttribute (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *This,
  IN UINTN                          Attribute
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This      - TODO: add argument description
  Attribute - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

STATIC
EFI_STATUS
EFIAPI
WinNtSimpleTextOutClearScreen (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *This
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

STATIC
EFI_STATUS
EFIAPI
WinNtSimpleTextOutSetCursorPosition (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *This,
  IN UINTN                          Column,
  IN UINTN                          Row
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This    - TODO: add argument description
  Column  - TODO: add argument description
  Row     - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

STATIC
EFI_STATUS
EFIAPI
WinNtSimpleTextOutEnableCursor (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *This,
  IN BOOLEAN                        Enable
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This    - TODO: add argument description
  Enable  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

//
// Simple Text Out constructor and destructor.
//
EFI_STATUS
WinNtSimpleTextOutOpenWindow (
  IN OUT  WIN_NT_SIMPLE_TEXT_PRIVATE_DATA *Private
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
WinNtSimpleTextOutCloseWindow (
  IN OUT  WIN_NT_SIMPLE_TEXT_PRIVATE_DATA *Console
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Console - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

//
// Simple Text In protocol member functions.
//
STATIC
EFI_STATUS
EFIAPI
WinNtSimpleTextInReset (
  IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL          *This,
  IN BOOLEAN                              ExtendedVerification
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

STATIC
EFI_STATUS
EFIAPI
WinNtSimpleTextInReadKeyStroke (
  IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL          *This,
  OUT EFI_INPUT_KEY                       *Key
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This  - TODO: add argument description
  Key   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

STATIC
VOID
EFIAPI
WinNtSimpleTextInWaitForKey (
  IN EFI_EVENT          Event,
  IN VOID               *Context
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

//
// Simple Text In constructor
//
EFI_STATUS
WinNtSimpleTextInAttachToWindow (
  IN  WIN_NT_SIMPLE_TEXT_PRIVATE_DATA *Private
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

//
// Main Entry Point
//
EFI_STATUS
EFIAPI
InitializeWinNtConsole (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ImageHandle - TODO: add argument description
  SystemTable - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AppendDevicePathInstanceToVar (
  IN  CHAR16                    *VariableName,
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePathInstance
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  VariableName        - TODO: add argument description
  DevicePathInstance  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

#endif
