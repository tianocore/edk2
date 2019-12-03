/** @file

Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2018, Linaro Ltd. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VIRTUAL_KEYBOARD_H_
#define _VIRTUAL_KEYBOARD_H_


#include <Guid/StatusCodeDataTypeId.h>
#include <Protocol/DevicePath.h>
#include <Protocol/PlatformVirtualKeyboard.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>

//
// Driver Binding Externs
//
extern EFI_DRIVER_BINDING_PROTOCOL  gVirtualKeyboardDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gVirtualKeyboardComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gVirtualKeyboardComponentName2;


//
// VIRTUAL Keyboard Defines
//
#define CHAR_SCANCODE                        0xe0
#define CHAR_ESC                             0x1b

#define KEYBOARD_TIMEOUT                     65536   // 0.07s
#define KEYBOARD_WAITFORVALUE_TIMEOUT        1000000 // 1s
#define KEYBOARD_BAT_TIMEOUT                 4000000 // 4s
#define KEYBOARD_TIMER_INTERVAL              500000  // 0.5s

#define QUEUE_MAX_COUNT                      32

#define KEYBOARD_SCAN_CODE_MAX_COUNT         32

//
// VIRTUAL Keyboard Device Structure
//
#define VIRTUAL_KEYBOARD_DEV_SIGNATURE SIGNATURE_32 ('V', 'K', 'B', 'D')
#define VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY_SIGNATURE SIGNATURE_32 ('v', 'k', 'c', 'n')

typedef struct _VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY {
  UINTN                                      Signature;
  EFI_KEY_DATA                               KeyData;
  EFI_KEY_NOTIFY_FUNCTION                    KeyNotificationFn;
  LIST_ENTRY                                 NotifyEntry;
} VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY;

typedef struct {
  UINTN                                      Front;
  UINTN                                      Rear;
  EFI_KEY_DATA                               Buffer[QUEUE_MAX_COUNT];
} SIMPLE_QUEUE;

typedef struct {
  UINT8                                      Buffer[KEYBOARD_SCAN_CODE_MAX_COUNT];
  UINTN                                      Head;
  UINTN                                      Tail;
} SCAN_CODE_QUEUE;

typedef struct {
  UINTN                                      Signature;
  EFI_HANDLE                                 Handle;
  PLATFORM_VIRTUAL_KBD_PROTOCOL              *PlatformVirtual;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL             SimpleTextIn;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL          SimpleTextInputEx;

  //
  // Buffer storing EFI_KEY_DATA
  //
  SIMPLE_QUEUE                               Queue;
  SIMPLE_QUEUE                               QueueForNotify;

  //
  // Notification Function List
  //
  LIST_ENTRY                                 NotifyList;
  EFI_EVENT                                  KeyNotifyProcessEvent;
  EFI_EVENT                                  TimerEvent;
} VIRTUAL_KEYBOARD_DEV;

#define VIRTUAL_KEYBOARD_DEV_FROM_THIS(a)  CR (a, VIRTUAL_KEYBOARD_DEV, SimpleTextIn, VIRTUAL_KEYBOARD_DEV_SIGNATURE)
#define TEXT_INPUT_EX_VIRTUAL_KEYBOARD_DEV_FROM_THIS(a) \
  CR (a, \
      VIRTUAL_KEYBOARD_DEV, \
      SimpleTextInputEx, \
      VIRTUAL_KEYBOARD_DEV_SIGNATURE \
      )

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gVirtualKeyboardDriverBinding;

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
VirtualKeyboardDriverBindingSupported (
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
VirtualKeyboardDriverBindingStart (
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
VirtualKeyboardDriverBindingStop (
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

  @retval EFI_INVALID_PAVIRTUALETER Language is NULL.

  @retval EFI_INVALID_PAVIRTUALETER DriverName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
VirtualKeyboardComponentNameGetDriverName (
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

  @retval EFI_INVALID_PAVIRTUALETER ControllerHandle is NULL.

  @retval EFI_INVALID_PAVIRTUALETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PAVIRTUALETER Language is NULL.

  @retval EFI_INVALID_PAVIRTUALETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
VirtualKeyboardComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL      *This,
  IN  EFI_HANDLE                       ControllerHandle,
  IN  EFI_HANDLE                       ChildHandle        OPTIONAL,
  IN  CHAR8                            *Language,
  OUT CHAR16                           **ControllerName
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
VirtualKeyboardReset (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  IN  BOOLEAN                         ExtendedVerification
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
VirtualKeyboardResetEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
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
  @retval EFI_INVALID_PAVIRTUALETER KeyToggleState is NULL.

**/
EFI_STATUS
EFIAPI
VirtualKeyboardSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  );

/**
  Register a notification function for a particular keystroke for the input device.

  @param  This                    Protocol instance pointer.
  @param  KeyData                 A pointer to a buffer that is filled in with the keystroke
                                  information data for the key that was pressed.
  @param  KeyNotificationFunction Points to the function to be called when the key
                                  sequence is typed specified by KeyData.
  @param  NotifyHandle            Points to the unique handle assigned to the registered notification.


  @retval EFI_SUCCESS             The notification function was registered successfully.
  @retval EFI_OUT_OF_RESOURCES    Unable to allocate resources for necesssary data structures.
  @retval EFI_INVALID_PAVIRTUALETER   KeyData or NotifyHandle is NULL.

**/
EFI_STATUS
EFIAPI
VirtualKeyboardRegisterKeyNotify (
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
  @retval EFI_INVALID_PAVIRTUALETER   The NotificationHandle is invalid.

**/
EFI_STATUS
EFIAPI
VirtualKeyboardUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
  );

//
// Private worker functions
//
/**
  Free keyboard notify list.

  @param  ListHead   The list head

  @retval EFI_SUCCESS           Free the notify list successfully
  @retval EFI_INVALID_PAVIRTUALETER ListHead is invalid.

**/
EFI_STATUS
VirtualKeyboardFreeNotifyList (
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

  @param  Event       The event that be siganlled when any key has been stroked.
  @param  Context     Pointer of the protocol EFI_SIMPLE_TEXT_INPUT_PROTOCOL.

**/
VOID
EFIAPI
VirtualKeyboardWaitForKey (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  );

/**
  Waiting on the keyboard event, if there's any key pressed by the user, signal the event

  @param  Event    The event that be siganlled when any key has been stroked.
  @param  Context  Pointer of the protocol EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL.

**/
VOID
EFIAPI
VirtualKeyboardWaitForKeyEx (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  );

/**
  Timer event handler: read a series of key stroke from 8042
  and put them into memory key buffer.
  It is registered as running under TPL_NOTIFY

  @param  Event   The timer event
  @param  Context A VIRTUAL_KEYBOARD_DEV pointer

**/
VOID
EFIAPI
VirtualKeyboardTimerHandler (
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
  Read out the scan code of the key that has just been stroked.

  @param  This        Pointer of simple text Protocol.
  @param  Key         Pointer for store the key that read out.

  @retval EFI_SUCCESS The key is read out successfully.
  @retval other       The key reading failed.

**/
EFI_STATUS
EFIAPI
VirtualKeyboardReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  OUT EFI_INPUT_KEY                   *Key
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
  @retval  EFI_INVALID_PAVIRTUALETER KeyData is NULL.

**/
EFI_STATUS
EFIAPI
VirtualKeyboardReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
  OUT EFI_KEY_DATA                      *KeyData
  );

#endif /* _VIRTUAL_KEYBOARD_H_ */
