/** @file

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _BIOS_KEYBOARD_H_
#define _BIOS_KEYBOARD_H_


#include <FrameworkDxe.h>
#include <Pi/PiDxeCis.h>

#include <Guid/StatusCodeDataTypeId.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/IsaIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/Ps2Policy.h>

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>

//
// Driver Binding Externs
//
extern EFI_DRIVER_BINDING_PROTOCOL  gBiosKeyboardDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gBiosKeyboardComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gBiosKeyboardComponentName2;


#include <IndustryStandard/Pci.h>

//
// BISO Keyboard Defines
//
#define CHAR_SCANCODE                   0xe0
#define CHAR_ESC                        0x1b

#define KEYBOARD_8042_DATA_REGISTER     0x60
#define KEYBOARD_8042_STATUS_REGISTER   0x64
#define KEYBOARD_8042_COMMAND_REGISTER  0x64

#define KEYBOARD_TIMEOUT                65536   // 0.07s
#define KEYBOARD_WAITFORVALUE_TIMEOUT   1000000 // 1s
#define KEYBOARD_BAT_TIMEOUT            4000000 // 4s
#define KEYBOARD_TIMER_INTERVAL         200000  // 0.02s
//  KEYBOARD COMMAND BYTE -- read by writing command KBC_CMDREG_VIA64_CMDBYTE_R to 64H, then read from 60H
//                           write by wrting command KBC_CMDREG_VIA64_CMDBYTE_W to 64H, then write to  60H
//  7: Reserved
//  6: PC/XT translation mode convert
//  5: Disable Auxiliary device interface
//  4: Disable keyboard interface
//  3: Reserved
//  2: System Flag: selftest successful
//  1: Enable Auxiliary device interrupt
//  0: Enable Keyboard interrupt )
//
#define KB_CMMBYTE_KSCAN2UNI_COV  (0x1 << 6)
#define KB_CMMBYTE_DISABLE_AUX    (0x1 << 5)
#define KB_CMMBYTE_DISABLE_KB     (0x1 << 4)
#define KB_CMMBYTE_SLFTEST_SUCC   (0x1 << 2)
#define KB_CMMBYTE_ENABLE_AUXINT  (0x1 << 1)
#define KB_CMMBYTE_ENABLE_KBINT   (0x1 << 0)

//
//  KEYBOARD CONTROLLER STATUS REGISTER - read from 64h
//  7: Parity error
//  6: General time out
//  5: Output buffer holds data for AUX
//  4: Keyboard is not locked
//  3: Command written via 64h  / Data written via 60h
//  2: KBC self-test successful / Power-on reset
//  1: Input buffer holds CPU data / empty
//  0: Output buffer holds keyboard data / empty
//
#define KBC_STSREG_VIA64_PARE (0x1 << 7)
#define KBC_STSREG_VIA64_TIM  (0x1 << 6)
#define KBC_STSREG_VIA64_AUXB (0x1 << 5)
#define KBC_STSREG_VIA64_KEYL (0x1 << 4)
#define KBC_STSREG_VIA64_C_D  (0x1 << 3)
#define KBC_STSREG_VIA64_SYSF (0x1 << 2)
#define KBC_STSREG_VIA64_INPB (0x1 << 1)
#define KBC_STSREG_VIA64_OUTB (0x1 << 0)

//
//  COMMANDs of KEYBOARD CONTROLLER COMMAND REGISTER - write to 64h
//
#define KBC_CMDREG_VIA64_CMDBYTE_R    0x20
#define KBC_CMDREG_VIA64_CMDBYTE_W    0x60
#define KBC_CMDREG_VIA64_AUX_DISABLE  0xA7
#define KBC_CMDREG_VIA64_AUX_ENABLE   0xA8
#define KBC_CMDREG_VIA64_KBC_SLFTEST  0xAA
#define KBC_CMDREG_VIA64_KB_CKECK     0xAB
#define KBC_CMDREG_VIA64_KB_DISABLE   0xAD
#define KBC_CMDREG_VIA64_KB_ENABLE    0xAE
#define KBC_CMDREG_VIA64_INTP_LOW_R   0xC0
#define KBC_CMDREG_VIA64_INTP_HIGH_R  0xC2
#define KBC_CMDREG_VIA64_OUTP_R       0xD0
#define KBC_CMDREG_VIA64_OUTP_W       0xD1
#define KBC_CMDREG_VIA64_OUTB_KB_W    0xD2
#define KBC_CMDREG_VIA64_OUTB_AUX_W   0xD3
#define KBC_CMDREG_VIA64_AUX_W        0xD4

//
//  echos of KEYBOARD CONTROLLER COMMAND - read from 60h
//
#define KBC_CMDECHO_KBCSLFTEST_OK 0x55
#define KBC_CMDECHO_KBCHECK_OK    0x00
#define KBC_CMDECHO_ACK           0xFA
#define KBC_CMDECHO_BATTEST_OK    0xAA
#define KBC_CMDECHO_BATTEST_FAILE 0xFC

//
// OUTPUT PORT COMMANDs - write port by writing KBC_CMDREG_VIA64_OUTP_W via 64H, then write the command to 60H
// drive data and clock of KB to high for at least 500us for BAT needs
//
#define KBC_OUTPORT_DCHIGH_BAT  0xC0
//
// scan code set type
//
#define KBC_INPBUF_VIA60_SCODESET1  0x01
#define KBC_INPBUF_VIA60_SCODESET2  0x02
#define KBC_INPBUF_VIA60_SCODESET3  0x03

//
//  COMMANDs written to INPUT BUFFER - write to 60h
//
#define KBC_INPBUF_VIA60_KBECHO   0xEE
#define KBC_INPBUF_VIA60_KBSCODE  0xF0
#define KBC_INPBUF_VIA60_KBTYPE   0xF2
#define KBC_INPBUF_VIA60_KBDELAY  0xF3
#define KBC_INPBUF_VIA60_KBEN     0xF4
#define KBC_INPBUF_VIA60_KBSTDDIS 0xF5
#define KBC_INPBUF_VIA60_KBSTDEN  0xF6
#define KBC_INPBUF_VIA60_KBRESEND 0xFE
#define KBC_INPBUF_VIA60_KBRESET  0xFF

//
// 0040h:0017h - KEYBOARD - STATUS FLAGS 1
//   7 INSert active
//   6 Caps Lock active
//   5 Num Lock active
//   4 Scroll Lock active
//   3 either Alt pressed
//   2 either Ctrl pressed
//   1 Left Shift pressed
//   0 Right Shift pressed
//
// 0040h:0018h - KEYBOARD - STATUS FLAGS 2
//   7: insert key is depressed
//   6: caps-lock key is depressed (does not work well)
//   5: num-lock key is depressed (does not work well)
//   4: scroll lock key is depressed (does not work well)
//   3: suspend key has been toggled (does not work well)
//   2: system key is pressed and held (does not work well)
//   1: left ALT key is pressed
//   0: left CTRL key is pressed
//
#define KB_INSERT_BIT             (0x1 << 7)
#define KB_CAPS_LOCK_BIT          (0x1 << 6)
#define KB_NUM_LOCK_BIT           (0x1 << 5)
#define KB_SCROLL_LOCK_BIT        (0x1 << 4)
#define KB_ALT_PRESSED            (0x1 << 3)
#define KB_CTRL_PRESSED           (0x1 << 2)
#define KB_LEFT_SHIFT_PRESSED     (0x1 << 1)
#define KB_RIGHT_SHIFT_PRESSED    (0x1 << 0)

#define KB_SUSPEND_PRESSED        (0x1 << 3)
#define KB_SYSREQ_PRESSED         (0x1 << 2)
#define KB_LEFT_ALT_PRESSED       (0x1 << 1)
#define KB_LEFT_CTRL_PRESSED      (0x1 << 0)

//
// BIOS Keyboard Device Structure
//
#define BIOS_KEYBOARD_DEV_SIGNATURE SIGNATURE_32 ('B', 'K', 'B', 'D')
#define BIOS_KEYBOARD_CONSOLE_IN_EX_NOTIFY_SIGNATURE SIGNATURE_32 ('c', 'b', 'k', 'h')

typedef struct _BIOS_KEYBOARD_CONSOLE_IN_EX_NOTIFY {
  UINTN                                      Signature;
  EFI_KEY_DATA                               KeyData;
  EFI_KEY_NOTIFY_FUNCTION                    KeyNotificationFn;
  LIST_ENTRY                                 NotifyEntry;
} BIOS_KEYBOARD_CONSOLE_IN_EX_NOTIFY;

#define QUEUE_MAX_COUNT         32
typedef struct {
  UINTN             Front;
  UINTN             Rear;
  EFI_KEY_DATA      Buffer[QUEUE_MAX_COUNT];
} SIMPLE_QUEUE;

typedef struct {
  UINTN                                       Signature;
  EFI_HANDLE                                  Handle;
  EFI_LEGACY_BIOS_PROTOCOL                    *LegacyBios;
  EFI_ISA_IO_PROTOCOL                         *IsaIo;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL              SimpleTextIn;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL           SimpleTextInputEx;
  UINT16                                      DataRegisterAddress;
  UINT16                                      StatusRegisterAddress;
  UINT16                                      CommandRegisterAddress;
  BOOLEAN                                     ExtendedKeyboard;

  EFI_KEY_STATE                               KeyState;
  //
  // Buffer storing EFI_KEY_DATA
  //
  SIMPLE_QUEUE                                Queue;
  SIMPLE_QUEUE                                QueueForNotify;

  //
  // Notification Function List
  //
  LIST_ENTRY                                  NotifyList;
  EFI_EVENT                                   KeyNotifyProcessEvent;
  EFI_EVENT                                   TimerEvent;
  
} BIOS_KEYBOARD_DEV;

#define BIOS_KEYBOARD_DEV_FROM_THIS(a)  CR (a, BIOS_KEYBOARD_DEV, SimpleTextIn, BIOS_KEYBOARD_DEV_SIGNATURE)
#define TEXT_INPUT_EX_BIOS_KEYBOARD_DEV_FROM_THIS(a) \
  CR (a, \
      BIOS_KEYBOARD_DEV, \
      SimpleTextInputEx, \
      BIOS_KEYBOARD_DEV_SIGNATURE \
      )

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gBiosKeyboardDriverBinding;

//
// Driver Binding Protocol functions
//

/**
  Check whether the driver supports this device.

  @param  This                   The Udriver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The driver supports this controller.
  @retval other                  This device isn't supported.

**/
EFI_STATUS
EFIAPI
BiosKeyboardDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Starts the device with this driver.

  @param  This                   The driver binding instance.
  @param  Controller             Handle of device to bind driver to.
  @param  RemainingDevicePath    Optional parameter use to pick a specific child
                                 device to start.

  @retval EFI_SUCCESS            The controller is controlled by the driver.
  @retval Other                  This controller cannot be started.

**/
EFI_STATUS
EFIAPI
BiosKeyboardDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Stop the device handled by this driver.

  @param  This                   The driver binding protocol.
  @param  Controller             The controller to release.
  @param  NumberOfChildren       The number of handles in ChildHandleBuffer.
  @param  ChildHandleBuffer      The array of child handle.

  @retval EFI_SUCCESS            The device was stopped.
  @retval EFI_DEVICE_ERROR       The device could not be stopped due to a device error.
  @retval Others                 Fail to uninstall protocols attached on the device.

**/
EFI_STATUS
EFIAPI
BiosKeyboardDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.

  @param  DriverName[out]       A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER DriverName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
BiosKeyboardComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );


/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle[in]  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param  ChildHandle[in]       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param  ControllerName[out]   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
BiosKeyboardComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );


//
// Simple Text Input Protocol functions
//
/**
  Reset the Keyboard and do BAT test for it, if (ExtendedVerification == TRUE) then do some extra keyboard validations.

  @param  This                  Pointer of simple text Protocol.
  @param  ExtendedVerification  Whether perform the extra validation of keyboard. True: perform; FALSE: skip.

  @retval EFI_SUCCESS           The command byte is written successfully.
  @retval EFI_DEVICE_ERROR      Errors occurred during resetting keyboard.

**/
EFI_STATUS
EFIAPI
BiosKeyboardReset (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  IN  BOOLEAN                         ExtendedVerification
  );

/**
  Read out the scan code of the key that has just been stroked.

  @param  This        Pointer of simple text Protocol.
  @param  Key         Pointer for store the key that read out.

  @retval EFI_SUCCESS The key is read out successfully.
  @retval other       The key reading failed.

**/
EFI_STATUS
EFIAPI
BiosKeyboardReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  OUT EFI_INPUT_KEY                   *Key
  );

//
// Private worker functions
//
/**
  Waiting on the keyboard event, if there's any key pressed by the user, signal the event

  @param  Event       The event that be siganlled when any key has been stroked.
  @param  Context     Pointer of the protocol EFI_SIMPLE_TEXT_INPUT_PROTOCOL.

**/
VOID
EFIAPI
BiosKeyboardWaitForKey (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  );

/**
  Check key buffer to get the key stroke status.

  @param  This         Pointer of the protocol EFI_SIMPLE_TEXT_IN_PROTOCOL.
  
  @retval EFI_SUCCESS  A key is being pressed now.
  @retval Other        No key is now pressed.

**/
EFI_STATUS
EFIAPI
BiosKeyboardCheckForKey (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This
  );

/**
  Convert unicode combined with scan code of key to the counterpart of EFIScancode of it.

  @param  KeyChar      Unicode of key.
  @param  ScanCode     Scan code of key.

  @return The value of EFI Scancode for the key.    
  @retval SCAN_NULL   No corresponding value in the EFI convert table is found for the key.

**/
UINT16
ConvertToEFIScanCode (
  IN  CHAR16  KeyChar,
  IN  UINT16  ScanCode
  );

/**
  Check whether there is Ps/2 Keyboard device in system by 0xF4 Keyboard Command
  If Keyboard receives 0xF4, it will respond with 'ACK'. If it doesn't respond, the device
  should not be in system. 

  @param  BiosKeyboardPrivate  Keyboard Private Data Struture

  @retval TRUE  Keyboard in System.
  @retval FALSE Keyboard not in System.

**/
BOOLEAN
CheckKeyboardConnect (
  IN  BIOS_KEYBOARD_DEV     *BiosKeyboardPrivate
  );

/**
  Timer event handler: read a series of key stroke from 8042
  and put them into memory key buffer. 
  It is registered as running under TPL_NOTIFY
  
  @param  Event   The timer event
  @param  Context A BIOS_KEYBOARD_DEV pointer

**/
VOID
EFIAPI
BiosKeyboardTimerHandler (
  IN EFI_EVENT    Event,
  IN VOID         *Context
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
  Reset the input device and optionaly run diagnostics
 
  @param  This                  Protocol instance pointer.
  @param  ExtendedVerification  Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS           The device was reset.
  @retval EFI_DEVICE_ERROR      The device is not functioning properly and could 
                                not be reset.

**/
EFI_STATUS
EFIAPI
BiosKeyboardResetEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  );

/**
  Reads the next keystroke from the input device. The WaitForKey Event can 
  be used to test for existance of a keystroke via WaitForEvent () call.

  @param  This         Protocol instance pointer.
  @param  KeyData      A pointer to a buffer that is filled in with the keystroke 
                       state data for the key that was pressed.
  
  @retval  EFI_SUCCESS           The keystroke information was returned.
  @retval  EFI_NOT_READY         There was no keystroke data availiable.
  @retval  EFI_DEVICE_ERROR      The keystroke information was not returned due to 
                                 hardware errors.
  @retval  EFI_INVALID_PARAMETER KeyData is NULL.                        
    
**/
EFI_STATUS
EFIAPI
BiosKeyboardReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
  OUT EFI_KEY_DATA                      *KeyData
  );

/**
  Set certain state for the input device.

  @param  This              Protocol instance pointer.
  @param  KeyToggleState    A pointer to the EFI_KEY_TOGGLE_STATE to set the 
                            state for the input device.

  @retval EFI_SUCCESS           The device state was set successfully.
  @retval EFI_DEVICE_ERROR      The device is not functioning correctly and could 
                                not have the setting adjusted.
  @retval EFI_UNSUPPORTED       The device does not have the ability to set its state.
  @retval EFI_INVALID_PARAMETER KeyToggleState is NULL.                       

**/   
EFI_STATUS
EFIAPI
BiosKeyboardSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  );

/**
  Register a notification function for a particular keystroke for the input device.

  @param  This                    Protocol instance pointer.
  @param  KeyData                 A pointer to a buffer that is filled in with the keystroke 
                                  information data for the key that was pressed. If KeyData.Key,
                                  KeyData.KeyState.KeyToggleState and KeyData.KeyState.KeyShiftState
                                  are 0, then any incomplete keystroke will trigger a notification of
                                  the KeyNotificationFunction.
  @param  KeyNotificationFunction Points to the function to be called when the key 
                                  sequence is typed specified by KeyData. This notification function
                                  should be called at <=TPL_CALLBACK.
  @param  NotifyHandle            Points to the unique handle assigned to the registered notification.

  
  @retval EFI_SUCCESS             The notification function was registered successfully.
  @retval EFI_OUT_OF_RESOURCES    Unable to allocate resources for necesssary data structures.
  @retval EFI_INVALID_PARAMETER   KeyData or NotifyHandle is NULL.

**/
EFI_STATUS
EFIAPI
BiosKeyboardRegisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_DATA                       *KeyData,
  IN EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT VOID                              **NotifyHandle
  );

/**
  Remove a registered notification function from a particular keystroke.

  @param  This                 Protocol instance pointer.    
  @param  NotificationHandle   The handle of the notification function being unregistered.
  
  @retval EFI_SUCCESS             The notification function was unregistered successfully.
  @retval EFI_INVALID_PARAMETER   The NotificationHandle is invalid.
                              
**/   
EFI_STATUS
EFIAPI
BiosKeyboardUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
  );

/**
  Wait for a specific value to be presented in
  Data register of Keyboard Controller by keyboard and then read it,
  used in keyboard commands ack

  @param   BiosKeyboardPrivate  Keyboard instance pointer.
  @param   Value                The value to be waited for
  @param   WaitForValueTimeOut  The limit of microseconds for timeout

  @retval  EFI_SUCCESS          The command byte is written successfully.
  @retval  EFI_TIMEOUT          Timeout occurred during writing.

**/
EFI_STATUS
KeyboardWaitForValue (
  IN BIOS_KEYBOARD_DEV  *BiosKeyboardPrivate,
  IN UINT8              Value,
  IN UINTN              WaitForValueTimeOut
  );

/**
  Write data byte to input buffer or input/output ports of Keyboard Controller with delay and waiting for buffer-empty state.

  @param   BiosKeyboardPrivate  Keyboard instance pointer.
  @param   Data                 Data byte to write.

  @retval  EFI_SUCCESS          The data byte is written successfully.
  @retval  EFI_TIMEOUT          Timeout occurred during writing.

**/
EFI_STATUS
KeyboardWrite (
  IN BIOS_KEYBOARD_DEV  *BiosKeyboardPrivate,
  IN UINT8              Data
  );

/**
  Free keyboard notify list.

  @param  ListHead   The list head

  @retval EFI_SUCCESS           Free the notify list successfully
  @retval EFI_INVALID_PARAMETER ListHead is invalid.

**/
EFI_STATUS
BiosKeyboardFreeNotifyList (
  IN OUT LIST_ENTRY           *ListHead
  );  

/**
  Check if key is registered.

  @param  RegsiteredData    A pointer to a buffer that is filled in with the keystroke 
                            state data for the key that was registered.
  @param  InputData         A pointer to a buffer that is filled in with the keystroke 
                            state data for the key that was pressed.

  @retval TRUE              Key be pressed matches a registered key.
  @retval FLASE             Match failed. 
  
**/
BOOLEAN
IsKeyRegistered (
  IN EFI_KEY_DATA  *RegsiteredData,
  IN EFI_KEY_DATA  *InputData
  );

/**
  Waiting on the keyboard event, if there's any key pressed by the user, signal the event

  @param  Event    The event that be siganlled when any key has been stroked.
  @param  Context  Pointer of the protocol EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL.
  
**/
VOID
EFIAPI
BiosKeyboardWaitForKeyEx (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  );

#endif

