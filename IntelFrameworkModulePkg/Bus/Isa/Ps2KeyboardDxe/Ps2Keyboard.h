/**@file
  PS/2 keyboard driver header file

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PS2KEYBOARD_H
#define _PS2KEYBOARD_H

#include <PiDxe.h>
#include <Framework/StatusCode.h>

#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/IsaIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/Ps2Policy.h>

#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

//
// Driver Private Data
//
#define KEYBOARD_BUFFER_MAX_COUNT         32
#define KEYBOARD_CONSOLE_IN_DEV_SIGNATURE EFI_SIGNATURE_32 ('k', 'k', 'e', 'y')
#define KEYBOARD_CONSOLE_IN_EX_NOTIFY_SIGNATURE EFI_SIGNATURE_32 ('k', 'c', 'e', 'n')

typedef struct _KEYBOARD_CONSOLE_IN_EX_NOTIFY {
  UINTN                                 Signature;
  EFI_HANDLE                            NotifyHandle;
  EFI_KEY_DATA                          KeyData;
  EFI_KEY_NOTIFY_FUNCTION               KeyNotificationFn;
  LIST_ENTRY                            NotifyEntry;
} KEYBOARD_CONSOLE_IN_EX_NOTIFY;


typedef struct {
  UINTN                               Signature;

  EFI_HANDLE                          Handle;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL      ConIn;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL   ConInEx;
  EFI_ISA_IO_PROTOCOL                 *IsaIo;

  EFI_EVENT                           TimerEvent;

  UINT32                              DataRegisterAddress;
  UINT32                              StatusRegisterAddress;
  UINT32                              CommandRegisterAddress;

  EFI_INPUT_KEY                       Key;
  EFI_KEY_STATE                       KeyState;

  BOOLEAN                             LeftShift;
  BOOLEAN                             RightShift;  
  BOOLEAN                             LeftLogo;
  BOOLEAN                             RightLogo;
  BOOLEAN                             Menu;
  BOOLEAN                             SysReq;

  BOOLEAN                             Ctrl;
  BOOLEAN                             Alt;
  BOOLEAN                             Shift;
  BOOLEAN                             CapsLock;
  BOOLEAN                             NumLock;
  BOOLEAN                             ScrollLock;

  //
  // Buffer storing key scancodes
  //
  UINT8                               ScancodeBuf[KEYBOARD_BUFFER_MAX_COUNT];
  UINT32                              ScancodeBufStartPos;
  UINT32                              ScancodeBufEndPos;
  UINT32                              ScancodeBufCount;

  //
  // Indicators of the key pressing state, used in detecting Alt+Ctrl+Del
  //
  BOOLEAN                             Ctrled;
  BOOLEAN                             Alted;

  //
  // Error state
  //
  BOOLEAN                             KeyboardErr;

  EFI_UNICODE_STRING_TABLE            *ControllerNameTable;

  EFI_DEVICE_PATH_PROTOCOL            *DevicePath;
  //
  // Notification Function List
  //
  LIST_ENTRY                          NotifyList;
} KEYBOARD_CONSOLE_IN_DEV;

#define KEYBOARD_CONSOLE_IN_DEV_FROM_THIS(a)  CR (a, KEYBOARD_CONSOLE_IN_DEV, ConIn, KEYBOARD_CONSOLE_IN_DEV_SIGNATURE)
#define TEXT_INPUT_EX_KEYBOARD_CONSOLE_IN_DEV_FROM_THIS(a) \
  CR (a, \
      KEYBOARD_CONSOLE_IN_DEV, \
      ConInEx, \
      KEYBOARD_CONSOLE_IN_DEV_SIGNATURE \
      )

#define TABLE_END 0x0

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gKeyboardControllerDriver;
extern EFI_COMPONENT_NAME_PROTOCOL   gPs2KeyboardComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gPs2KeyboardComponentName2;

extern EFI_GUID                      gSimpleTextInExNotifyGuid;

//
// Driver entry point
//
EFI_STATUS
EFIAPI
InstallPs2KeyboardDriver (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  ImageHandle - GC_TODO: add argument description
  SystemTable - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

#define KEYBOARD_8042_DATA_REGISTER     0x60
#define KEYBOARD_8042_STATUS_REGISTER   0x64
#define KEYBOARD_8042_COMMAND_REGISTER  0x64

#define KEYBOARD_KBEN                   0xF4
#define KEYBOARD_CMDECHO_ACK            0xFA

#define KEYBOARD_TIMEOUT                65536   // 0.07s
#define KEYBOARD_WAITFORVALUE_TIMEOUT   1000000 // 1s
#define KEYBOARD_BAT_TIMEOUT            4000000 // 4s
#define KEYBOARD_TIMER_INTERVAL         200000  // 0.02s
#define SCANCODE_EXTENDED               0xE0
#define SCANCODE_EXTENDED1              0xE1
#define SCANCODE_CTRL_MAKE              0x1D
#define SCANCODE_CTRL_BREAK             0x9D
#define SCANCODE_ALT_MAKE               0x38
#define SCANCODE_ALT_BREAK              0xB8
#define SCANCODE_LEFT_SHIFT_MAKE        0x2A
#define SCANCODE_LEFT_SHIFT_BREAK       0xAA
#define SCANCODE_RIGHT_SHIFT_MAKE       0x36
#define SCANCODE_RIGHT_SHIFT_BREAK      0xB6
#define SCANCODE_CAPS_LOCK_MAKE         0x3A
#define SCANCODE_NUM_LOCK_MAKE          0x45
#define SCANCODE_SCROLL_LOCK_MAKE       0x46
#define SCANCODE_LEFT_LOGO_MAKE         0x5B //GUI key defined in Keyboard scan code
#define SCANCODE_LEFT_LOGO_BREAK        0xDB
#define SCANCODE_RIGHT_LOGO_MAKE        0x5C
#define SCANCODE_RIGHT_LOGO_BREAK       0xDC
#define SCANCODE_MENU_MAKE              0x5D //APPS key defined in Keyboard scan code 
#define SCANCODE_MENU_BREAK             0xDD
#define SCANCODE_SYS_REQ_MAKE           0x37
#define SCANCODE_MAX_MAKE               0x60

//
// Other functions that are used among .c files
//

EFI_STATUS
UpdateStatusLights (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn
  )
/*++

Routine Description:

  Show keyboard status light for ScrollLock, NumLock and CapsLock
  according to indicators in ConsoleIn.

Arguments:

  ConsoleIn   - driver private structure

Returns:

  EFI_SUCCESS - Show the status light successfully.
  EFI_TIMEOUT - Timeout when operating read/write on registers.

--*/  
;

EFI_STATUS
KeyboardRead (
  IN KEYBOARD_CONSOLE_IN_DEV  *ConsoleIn,
  OUT UINT8                   *Data
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  ConsoleIn - GC_TODO: add argument description
  Data      - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
KeyGetchar (
  IN OUT KEYBOARD_CONSOLE_IN_DEV *ConsoleIn
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  ConsoleIn - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
InitKeyboard (
  IN OUT KEYBOARD_CONSOLE_IN_DEV *ConsoleIn,
  IN BOOLEAN                     ExtendedVerification
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  ConsoleIn             - GC_TODO: add argument description
  ExtendedVerification  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
DisableKeyboard (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  ConsoleIn - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

VOID
EFIAPI
KeyboardTimerHandler (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Event   - GC_TODO: add argument description
  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
KeyboardEfiReset (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  IN  BOOLEAN                         ExtendedVerification
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  This                  - GC_TODO: add argument description
  ExtendedVerification  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
KeyboardReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  OUT EFI_INPUT_KEY                   *Key
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  This  - GC_TODO: add argument description
  Key   - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

VOID
EFIAPI
KeyboardWaitForKey (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Event   - GC_TODO: add argument description
  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

UINT8
KeyReadStatusRegister (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  ConsoleIn - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

/**
  Check whether there is Ps/2 Keyboard device in system by 0xF4 Keyboard Command
  If Keyboard receives 0xF4, it will respond with 'ACK'. If it doesn't respond, the device
  should not be in system.

  @param[in]  BiosKeyboardPrivate   Keyboard Private Data Structure

  @retval     TRUE                  Keyboard in System.
  @retval     FALSE                 Keyboard not in System.
**/
BOOLEAN
EFIAPI
CheckKeyboardConnect (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn
  )
;

VOID
EFIAPI
KeyboardWaitForKeyEx (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  )
/*++

Routine Description:

  Event notification function for SIMPLE_TEXT_INPUT_EX_PROTOCOL.WaitForKeyEx event
  Signal the event if there is key available

Arguments:

Returns:

--*/    
;  

//
// Simple Text Input Ex protocol function prototypes
//

EFI_STATUS
EFIAPI
KeyboardEfiResetEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  )
/*++

  Routine Description:
    Reset the input device and optionaly run diagnostics

  Arguments:
    This                 - Protocol instance pointer.
    ExtendedVerification - Driver may perform diagnostics on reset.

  Returns:
    EFI_SUCCESS           - The device was reset.
    EFI_DEVICE_ERROR      - The device is not functioning properly and could 
                            not be reset.

--*/
;

EFI_STATUS
EFIAPI
KeyboardReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
  OUT EFI_KEY_DATA                      *KeyData
  )
/*++

  Routine Description:
    Reads the next keystroke from the input device. The WaitForKey Event can 
    be used to test for existance of a keystroke via WaitForEvent () call.

  Arguments:
    This       - Protocol instance pointer.
    KeyData    - A pointer to a buffer that is filled in with the keystroke 
                 state data for the key that was pressed.

  Returns:
    EFI_SUCCESS           - The keystroke information was returned.
    EFI_NOT_READY         - There was no keystroke data availiable.
    EFI_DEVICE_ERROR      - The keystroke information was not returned due to 
                            hardware errors.
    EFI_INVALID_PARAMETER - KeyData is NULL.                        

--*/
;

EFI_STATUS
EFIAPI
KeyboardSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  )
/*++

  Routine Description:
    Set certain state for the input device.

  Arguments:
    This                  - Protocol instance pointer.
    KeyToggleState        - A pointer to the EFI_KEY_TOGGLE_STATE to set the 
                            state for the input device.
                          
  Returns:                
    EFI_SUCCESS           - The device state was set successfully.
    EFI_DEVICE_ERROR      - The device is not functioning correctly and could 
                            not have the setting adjusted.
    EFI_UNSUPPORTED       - The device does not have the ability to set its state.
    EFI_INVALID_PARAMETER - KeyToggleState is NULL.                       

--*/   
;

EFI_STATUS
EFIAPI
KeyboardRegisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_DATA                       *KeyData,
  IN EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT EFI_HANDLE                        *NotifyHandle
  )
/*++

  Routine Description:
    Register a notification function for a particular keystroke for the input device.

  Arguments:
    This                    - Protocol instance pointer.
    KeyData                 - A pointer to a buffer that is filled in with the keystroke 
                              information data for the key that was pressed.
    KeyNotificationFunction - Points to the function to be called when the key 
                              sequence is typed specified by KeyData.                        
    NotifyHandle            - Points to the unique handle assigned to the registered notification.                          

  Returns:
    EFI_SUCCESS             - The notification function was registered successfully.
    EFI_OUT_OF_RESOURCES    - Unable to allocate resources for necesssary data structures.
    EFI_INVALID_PARAMETER   - KeyData or NotifyHandle is NULL.                       
                              
--*/   
;

EFI_STATUS
EFIAPI
KeyboardUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_HANDLE                         NotificationHandle
  )
/*++

  Routine Description:
    Remove a registered notification function from a particular keystroke.

  Arguments:
    This                    - Protocol instance pointer.    
    NotificationHandle      - The handle of the notification function being unregistered.

  Returns:
    EFI_SUCCESS             - The notification function was unregistered successfully.
    EFI_INVALID_PARAMETER   - The NotificationHandle is invalid.
    EFI_NOT_FOUND           - Can not find the matching entry in database.  
                              
--*/   
;

#endif
