/** @file
  PS/2 keyboard driver header file

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PS2KEYBOARD_H_
#define _PS2KEYBOARD_H_

#include <Uefi.h>

#include <Protocol/SuperIo.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/DevicePath.h>
#include <Protocol/Ps2Policy.h>

#include <Library/IoLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TimerLib.h>
#include <Library/PcdLib.h>

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gKeyboardControllerDriver;
extern EFI_COMPONENT_NAME_PROTOCOL   gPs2KeyboardComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gPs2KeyboardComponentName2;

//
// Driver Private Data
//
#define KEYBOARD_CONSOLE_IN_DEV_SIGNATURE       SIGNATURE_32 ('k', 'k', 'e', 'y')
#define KEYBOARD_CONSOLE_IN_EX_NOTIFY_SIGNATURE SIGNATURE_32 ('k', 'c', 'e', 'n')

typedef struct _KEYBOARD_CONSOLE_IN_EX_NOTIFY {
  UINTN                               Signature;
  EFI_KEY_DATA                        KeyData;
  EFI_KEY_NOTIFY_FUNCTION             KeyNotificationFn;
  LIST_ENTRY                          NotifyEntry;
} KEYBOARD_CONSOLE_IN_EX_NOTIFY;

#define KEYBOARD_SCAN_CODE_MAX_COUNT  32
typedef struct {
  UINT8                               Buffer[KEYBOARD_SCAN_CODE_MAX_COUNT];
  UINTN                               Head;
  UINTN                               Tail;
} SCAN_CODE_QUEUE;

#define KEYBOARD_EFI_KEY_MAX_COUNT    256
typedef struct {
  EFI_KEY_DATA                        Buffer[KEYBOARD_EFI_KEY_MAX_COUNT];
  UINTN                               Head;
  UINTN                               Tail;
} EFI_KEY_QUEUE;

typedef struct {
  UINTN                               Signature;

  EFI_HANDLE                          Handle;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL      ConIn;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL   ConInEx;

  EFI_EVENT                           TimerEvent;

  UINT32                              DataRegisterAddress;
  UINT32                              StatusRegisterAddress;
  UINT32                              CommandRegisterAddress;

  BOOLEAN                             LeftCtrl;
  BOOLEAN                             RightCtrl;
  BOOLEAN                             LeftAlt;
  BOOLEAN                             RightAlt;
  BOOLEAN                             LeftShift;
  BOOLEAN                             RightShift;
  BOOLEAN                             LeftLogo;
  BOOLEAN                             RightLogo;
  BOOLEAN                             Menu;
  BOOLEAN                             SysReq;

  BOOLEAN                             CapsLock;
  BOOLEAN                             NumLock;
  BOOLEAN                             ScrollLock;

  BOOLEAN                             IsSupportPartialKey;
  //
  // Queue storing key scancodes
  //
  SCAN_CODE_QUEUE                     ScancodeQueue;
  EFI_KEY_QUEUE                       EfiKeyQueue;
  EFI_KEY_QUEUE                       EfiKeyQueueForNotify;

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
  EFI_EVENT                           KeyNotifyProcessEvent;
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
// Driver entry point
//
/**
  The user Entry Point for module Ps2Keyboard. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InstallPs2KeyboardDriver (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );

#define KEYBOARD_8042_DATA_REGISTER     0x60
#define KEYBOARD_8042_STATUS_REGISTER   0x64
#define KEYBOARD_8042_COMMAND_REGISTER  0x64

#define KEYBOARD_KBEN                   0xF4
#define KEYBOARD_CMDECHO_ACK            0xFA

#define KEYBOARD_MAX_TRY                256     // 256
#define KEYBOARD_TIMEOUT                65536   // 0.07s
#define KEYBOARD_WAITFORVALUE_TIMEOUT   1000000 // 1s
#define KEYBOARD_BAT_TIMEOUT            4000000 // 4s
#define KEYBOARD_TIMER_INTERVAL         200000  // 0.02s
#define SCANCODE_EXTENDED0              0xE0
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
#define SCANCODE_DELETE_MAKE            0x53
#define SCANCODE_LEFT_LOGO_MAKE         0x5B //GUI key defined in Keyboard scan code
#define SCANCODE_LEFT_LOGO_BREAK        0xDB
#define SCANCODE_RIGHT_LOGO_MAKE        0x5C
#define SCANCODE_RIGHT_LOGO_BREAK       0xDC
#define SCANCODE_MENU_MAKE              0x5D //APPS key defined in Keyboard scan code
#define SCANCODE_MENU_BREAK             0xDD
#define SCANCODE_SYS_REQ_MAKE           0x37
#define SCANCODE_SYS_REQ_BREAK          0xB7
#define SCANCODE_SYS_REQ_MAKE_WITH_ALT  0x54
#define SCANCODE_SYS_REQ_BREAK_WITH_ALT 0xD4

#define SCANCODE_MAX_MAKE               0x60


#define KEYBOARD_STATUS_REGISTER_HAS_OUTPUT_DATA     BIT0        ///< 0 - Output register has no data; 1 - Output register has data
#define KEYBOARD_STATUS_REGISTER_HAS_INPUT_DATA      BIT1        ///< 0 - Input register has no data;  1 - Input register has data
#define KEYBOARD_STATUS_REGISTER_SYSTEM_FLAG         BIT2        ///< Set to 0 after power on reset
#define KEYBOARD_STATUS_REGISTER_INPUT_DATA_TYPE     BIT3        ///< 0 - Data in input register is data; 1 - Data in input register is command
#define KEYBOARD_STATUS_REGISTER_ENABLE_FLAG         BIT4        ///< 0 - Keyboard is disable; 1 - Keyboard is enable
#define KEYBOARD_STATUS_REGISTER_TRANSMIT_TIMEOUT    BIT5        ///< 0 - Transmit is complete without timeout; 1 - Transmit is timeout without complete
#define KEYBOARD_STATUS_REGISTER_RECEIVE_TIMEOUT     BIT6        ///< 0 - Receive is complete without timeout; 1 - Receive is timeout without complete
#define KEYBOARD_STATUS_REGISTER_PARITY              BIT7        ///< 0 - Odd parity; 1 - Even parity

#define KEYBOARD_8042_COMMAND_READ                          0x20
#define KEYBOARD_8042_COMMAND_WRITE                         0x60
#define KEYBOARD_8042_COMMAND_DISABLE_MOUSE_INTERFACE       0xA7
#define KEYBOARD_8042_COMMAND_ENABLE_MOUSE_INTERFACE        0xA8
#define KEYBOARD_8042_COMMAND_CONTROLLER_SELF_TEST          0xAA
#define KEYBOARD_8042_COMMAND_KEYBOARD_INTERFACE_SELF_TEST  0xAB
#define KEYBOARD_8042_COMMAND_DISABLE_KEYBOARD_INTERFACE    0xAD

#define KEYBOARD_8048_COMMAND_CLEAR_OUTPUT_DATA             0xF4
#define KEYBOARD_8048_COMMAND_RESET                         0xFF
#define KEYBOARD_8048_COMMAND_SELECT_SCAN_CODE_SET          0xF0

#define KEYBOARD_8048_RETURN_8042_BAT_SUCCESS               0xAA
#define KEYBOARD_8048_RETURN_8042_BAT_ERROR                 0xFC
#define KEYBOARD_8048_RETURN_8042_ACK                       0xFA


//
// Keyboard Controller Status
//
#define KBC_PARE  0x80  // Parity Error
#define KBC_TIM   0x40  // General Time Out

//
// Other functions that are used among .c files
//
/**
  Show keyboard status lights according to
  indicators in ConsoleIn.

  @param ConsoleIn Pointer to instance of KEYBOARD_CONSOLE_IN_DEV

  @return status

**/
EFI_STATUS
UpdateStatusLights (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn
  );

/**
  write key to keyboard.

  @param ConsoleIn Pointer to instance of KEYBOARD_CONSOLE_IN_DEV
  @param Data      value wanted to be written

  @retval EFI_TIMEOUT - GC_TODO: Add description for return value
  @retval EFI_SUCCESS - GC_TODO: Add description for return value

**/
EFI_STATUS
KeyboardRead (
  IN KEYBOARD_CONSOLE_IN_DEV  *ConsoleIn,
  OUT UINT8                   *Data
  );

/**
  Get scancode from scancode buffer and translate into EFI-scancode and unicode defined by EFI spec.

  The function is always called in TPL_NOTIFY.

  @param ConsoleIn KEYBOARD_CONSOLE_IN_DEV instance pointer

**/
VOID
KeyGetchar (
  IN OUT KEYBOARD_CONSOLE_IN_DEV *ConsoleIn
  );

/**
  Process key notify.

  @param  Event                 Indicates the event that invoke this function.
  @param  Context               Indicates the calling context.
**/
VOID
EFIAPI
KeyNotifyProcessHandler (
  IN  EFI_EVENT                 Event,
  IN  VOID                      *Context
  );

/**
  Perform 8042 controller and keyboard Initialization.
  If ExtendedVerification is TRUE, do additional test for
  the keyboard interface

  @param ConsoleIn - KEYBOARD_CONSOLE_IN_DEV instance pointer
  @param ExtendedVerification - indicates a thorough initialization

  @retval EFI_DEVICE_ERROR Fail to init keyboard
  @retval EFI_SUCCESS      Success to init keyboard
**/
EFI_STATUS
InitKeyboard (
  IN OUT KEYBOARD_CONSOLE_IN_DEV *ConsoleIn,
  IN BOOLEAN                     ExtendedVerification
  );


/**
  Timer event handler: read a series of scancodes from 8042
  and put them into memory scancode buffer.
  it read as much scancodes to either fill
  the memory buffer or empty the keyboard buffer.
  It is registered as running under TPL_NOTIFY

  @param Event - The timer event
  @param Context - A KEYBOARD_CONSOLE_IN_DEV pointer

**/
VOID
EFIAPI
KeyboardTimerHandler (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  );

/**
  logic reset keyboard
  Implement SIMPLE_TEXT_IN.Reset()
  Perform 8042 controller and keyboard initialization

  @param This    Pointer to instance of EFI_SIMPLE_TEXT_INPUT_PROTOCOL
  @param ExtendedVerification Indicate that the driver may perform a more
                              exhaustive verification operation of the device during
                              reset, now this par is ignored in this driver

**/
EFI_STATUS
EFIAPI
KeyboardEfiReset (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  IN  BOOLEAN                         ExtendedVerification
  );

/**
  Implement SIMPLE_TEXT_IN.ReadKeyStroke().
  Retrieve key values for driver user.

  @param This    Pointer to instance of EFI_SIMPLE_TEXT_INPUT_PROTOCOL
  @param Key     The output buffer for key value

  @retval EFI_SUCCESS success to read key stroke
**/
EFI_STATUS
EFIAPI
KeyboardReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  OUT EFI_INPUT_KEY                   *Key
  );

/**
  Event notification function for SIMPLE_TEXT_IN.WaitForKey event
  Signal the event if there is key available

  @param Event    the event object
  @param Context  waitting context

**/
VOID
EFIAPI
KeyboardWaitForKey (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  );

/**
  Read status register.

  @param ConsoleIn  Pointer to instance of KEYBOARD_CONSOLE_IN_DEV

  @return value in status register

**/
UINT8
KeyReadStatusRegister (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn
  );

/**
  Check whether there is Ps/2 Keyboard device in system by 0xF4 Keyboard Command
  If Keyboard receives 0xF4, it will respond with 'ACK'. If it doesn't respond, the device
  should not be in system.

  @param[in]  ConsoleIn   Pointer to instance of KEYBOARD_CONSOLE_IN_DEV

  @retval     TRUE                  Keyboard in System.
  @retval     FALSE                 Keyboard not in System.
**/
BOOLEAN
EFIAPI
CheckKeyboardConnect (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn
  );

/**
  Event notification function for SIMPLE_TEXT_INPUT_EX_PROTOCOL.WaitForKeyEx event
  Signal the event if there is key available

  @param Event    event object
  @param Context  waiting context

**/
VOID
EFIAPI
KeyboardWaitForKeyEx (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  );

//
// Simple Text Input Ex protocol function prototypes
//

/**
  Reset the input device and optionaly run diagnostics

  @param This                 - Protocol instance pointer.
  @param ExtendedVerification - Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS           - The device was reset.
  @retval EFI_DEVICE_ERROR      - The device is not functioning properly and could
                                  not be reset.

**/
EFI_STATUS
EFIAPI
KeyboardEfiResetEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  );

/**
    Reads the next keystroke from the input device. The WaitForKey Event can
    be used to test for existance of a keystroke via WaitForEvent () call.


    @param This       - Protocol instance pointer.
    @param KeyData    - A pointer to a buffer that is filled in with the keystroke
                 state data for the key that was pressed.

    @retval EFI_SUCCESS           - The keystroke information was returned.
    @retval EFI_NOT_READY         - There was no keystroke data availiable.
    @retval EFI_DEVICE_ERROR      - The keystroke information was not returned due to
                            hardware errors.
    @retval EFI_INVALID_PARAMETER - KeyData is NULL.

**/
EFI_STATUS
EFIAPI
KeyboardReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
  OUT EFI_KEY_DATA                      *KeyData
  );

/**
  Set certain state for the input device.

  @param This              - Protocol instance pointer.
  @param KeyToggleState    - A pointer to the EFI_KEY_TOGGLE_STATE to set the
                        state for the input device.

  @retval EFI_SUCCESS           - The device state was set successfully.
  @retval EFI_DEVICE_ERROR      - The device is not functioning correctly and could
                            not have the setting adjusted.
  @retval EFI_UNSUPPORTED       - The device does not have the ability to set its state.
  @retval EFI_INVALID_PARAMETER - KeyToggleState is NULL.

**/
EFI_STATUS
EFIAPI
KeyboardSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  );

/**
    Register a notification function for a particular keystroke for the input device.

    @param This                    - Protocol instance pointer.
    @param KeyData                 - A pointer to a buffer that is filled in with the keystroke
                                     information data for the key that was pressed. If KeyData.Key,
                                     KeyData.KeyState.KeyToggleState and KeyData.KeyState.KeyShiftState are 0,
                                     then any incomplete keystroke will trigger a notification of the KeyNotificationFunction.
    @param KeyNotificationFunction - Points to the function to be called when the key
                                     sequence is typed specified by KeyData. This notification function
                                     should be called at <=TPL_CALLBACK.
    @param NotifyHandle            - Points to the unique handle assigned to the registered notification.

    @retval EFI_SUCCESS             - The notification function was registered successfully.
    @retval EFI_OUT_OF_RESOURCES    - Unable to allocate resources for necesssary data structures.
    @retval EFI_INVALID_PARAMETER   - KeyData or NotifyHandle is NULL.

**/
EFI_STATUS
EFIAPI
KeyboardRegisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_DATA                       *KeyData,
  IN EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT VOID                              **NotifyHandle
  );

/**
    Remove a registered notification function from a particular keystroke.

    @param This                    - Protocol instance pointer.
    @param NotificationHandle      - The handle of the notification function being unregistered.


    @retval EFI_SUCCESS             - The notification function was unregistered successfully.
    @retval EFI_INVALID_PARAMETER   - The NotificationHandle is invalid.
    @retval EFI_NOT_FOUND           - Can not find the matching entry in database.

**/
EFI_STATUS
EFIAPI
KeyboardUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
  );

/**
  Push one key data to the EFI key buffer.

  @param Queue     Pointer to instance of EFI_KEY_QUEUE.
  @param KeyData   The key data to push.
**/
VOID
PushEfikeyBufTail (
  IN  EFI_KEY_QUEUE         *Queue,
  IN  EFI_KEY_DATA          *KeyData
  );

/**
  Judge whether is a registed key

  @param RegsiteredData       A pointer to a buffer that is filled in with the keystroke
                              state data for the key that was registered.
  @param InputData            A pointer to a buffer that is filled in with the keystroke
                              state data for the key that was pressed.

  @retval TRUE                Key be pressed matches a registered key.
  @retval FLASE               Match failed.

**/
BOOLEAN
IsKeyRegistered (
  IN EFI_KEY_DATA  *RegsiteredData,
  IN EFI_KEY_DATA  *InputData
  );

/**
  Initialize the key state.

  @param  ConsoleIn     The KEYBOARD_CONSOLE_IN_DEV instance.
  @param  KeyState      A pointer to receive the key state information.
**/
VOID
InitializeKeyState (
  IN  KEYBOARD_CONSOLE_IN_DEV *ConsoleIn,
  OUT EFI_KEY_STATE           *KeyState
  );

#endif
