/** @file
  Header file for Terminal driver.

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
Copyright (C) 2016 Silicon Graphics, Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _TERMINAL_H_
#define _TERMINAL_H_


#include <Uefi.h>

#include <Guid/GlobalVariable.h>
#include <Guid/PcAnsi.h>
#include <Guid/TtyTerm.h>
#include <Guid/StatusCodeDataTypeVariable.h>

#include <Protocol/SimpleTextOut.h>
#include <Protocol/SerialIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseLib.h>


#define RAW_FIFO_MAX_NUMBER 255
#define FIFO_MAX_NUMBER     128

typedef struct {
  UINT8 Head;
  UINT8 Tail;
  UINT8 Data[RAW_FIFO_MAX_NUMBER + 1];
} RAW_DATA_FIFO;

typedef struct {
  UINT8   Head;
  UINT8   Tail;
  UINT16  Data[FIFO_MAX_NUMBER + 1];
} UNICODE_FIFO;

typedef struct {
  UINT8         Head;
  UINT8         Tail;
  EFI_INPUT_KEY Data[FIFO_MAX_NUMBER + 1];
} EFI_KEY_FIFO;

typedef struct {
  UINTN   Columns;
  UINTN   Rows;
} TERMINAL_CONSOLE_MODE_DATA;

#define KEYBOARD_TIMER_INTERVAL         200000  // 0.02s

#define TERMINAL_DEV_SIGNATURE  SIGNATURE_32 ('t', 'm', 'n', 'l')

#define TERMINAL_CONSOLE_IN_EX_NOTIFY_SIGNATURE SIGNATURE_32 ('t', 'm', 'e', 'n')

typedef struct _TERMINAL_CONSOLE_IN_EX_NOTIFY {
  UINTN                                 Signature;
  EFI_KEY_DATA                          KeyData;
  EFI_KEY_NOTIFY_FUNCTION               KeyNotificationFn;
  LIST_ENTRY                            NotifyEntry;
} TERMINAL_CONSOLE_IN_EX_NOTIFY;

typedef enum {
  TerminalTypePcAnsi,
  TerminalTypeVt100,
  TerminalTypeVt100Plus,
  TerminalTypeVtUtf8,
  TerminalTypeTtyTerm,
  TerminalTypeLinux,
  TerminalTypeXtermR6,
  TerminalTypeVt400,
  TerminalTypeSCO
} TERMINAL_TYPE;

typedef struct {
  UINTN                               Signature;
  EFI_HANDLE                          Handle;
  TERMINAL_TYPE                       TerminalType;
  EFI_SERIAL_IO_PROTOCOL              *SerialIo;
  EFI_DEVICE_PATH_PROTOCOL            *DevicePath;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL      SimpleInput;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL     SimpleTextOutput;
  EFI_SIMPLE_TEXT_OUTPUT_MODE         SimpleTextOutputMode;
  TERMINAL_CONSOLE_MODE_DATA          *TerminalConsoleModeData;
  UINTN                               SerialInTimeOut;
  RAW_DATA_FIFO                       *RawFiFo;
  UNICODE_FIFO                        *UnicodeFiFo;
  EFI_KEY_FIFO                        *EfiKeyFiFo;
  EFI_KEY_FIFO                        *EfiKeyFiFoForNotify;
  EFI_UNICODE_STRING_TABLE            *ControllerNameTable;
  EFI_EVENT                           TimerEvent;
  EFI_EVENT                           TwoSecondTimeOut;
  UINT32                              InputState;
  UINT32                              ResetState;
  UINT16                              TtyEscapeStr[3];
  INTN                                TtyEscapeIndex;

  //
  // Esc could not be output to the screen by user,
  // but the terminal driver need to output it to
  // the terminal emulation software to send control sequence.
  // This boolean is used by the terminal driver only
  // to indicate whether the Esc could be sent or not.
  //
  BOOLEAN                             OutputEscChar;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL   SimpleInputEx;
  LIST_ENTRY                          NotifyList;
  EFI_EVENT                           KeyNotifyProcessEvent;
} TERMINAL_DEV;

#define INPUT_STATE_DEFAULT               0x00
#define INPUT_STATE_ESC                   0x01
#define INPUT_STATE_CSI                   0x02
#define INPUT_STATE_LEFTOPENBRACKET       0x04
#define INPUT_STATE_O                     0x08
#define INPUT_STATE_2                     0x10
#define INPUT_STATE_LEFTOPENBRACKET_TTY   0x20
#define INPUT_STATE_1                     0x40
#define INPUT_STATE_LEFTOPENBRACKET_2ND   0x80

#define RESET_STATE_DEFAULT               0x00
#define RESET_STATE_ESC_R                 0x01
#define RESET_STATE_ESC_R_ESC_R           0x02

#define TERMINAL_CON_IN_DEV_FROM_THIS(a)  CR (a, TERMINAL_DEV, SimpleInput, TERMINAL_DEV_SIGNATURE)
#define TERMINAL_CON_OUT_DEV_FROM_THIS(a) CR (a, TERMINAL_DEV, SimpleTextOutput, TERMINAL_DEV_SIGNATURE)
#define TERMINAL_CON_IN_EX_DEV_FROM_THIS(a)  CR (a, TERMINAL_DEV, SimpleInputEx, TERMINAL_DEV_SIGNATURE)

typedef union {
  UINT8 Utf8_1;
  UINT8 Utf8_2[2];
  UINT8 Utf8_3[3];
} UTF8_CHAR;

#define LEFTOPENBRACKET           0x5b  // '['
#define ACAP                      0x41
#define BCAP                      0x42
#define CCAP                      0x43
#define DCAP                      0x44

#define BACKSPACE                 8
#define ESC                       27
#define CSI                       0x9B
#define DEL                       127
#define BRIGHT_CONTROL_OFFSET     2
#define FOREGROUND_CONTROL_OFFSET 6
#define BACKGROUND_CONTROL_OFFSET 11
#define ROW_OFFSET                2
#define COLUMN_OFFSET             5
#define FW_BACK_OFFSET            2

typedef struct {
  UINT16  Unicode;
  CHAR8   PcAnsi;
  CHAR8   Ascii;
} UNICODE_TO_CHAR;

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gTerminalDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gTerminalComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gTerminalComponentName2;

/**
  The user Entry Point for module Terminal. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeTerminal (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );

/**
  Implements EFI_SIMPLE_TEXT_INPUT_PROTOCOL.Reset().
  This driver only perform dependent serial device reset regardless of
  the value of ExtendeVerification

  @param  This                     Indicates the calling context.
  @param  ExtendedVerification     Skip by this driver.

  @retval EFI_SUCCESS              The reset operation succeeds.
  @retval EFI_DEVICE_ERROR         The dependent serial port reset fails.

**/
EFI_STATUS
EFIAPI
TerminalConInReset (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL    *This,
  IN  BOOLEAN                           ExtendedVerification
  );


/**
  Implements EFI_SIMPLE_TEXT_INPUT_PROTOCOL.ReadKeyStroke().

  @param  This                Indicates the calling context.
  @param  Key                 A pointer to a buffer that is filled in with the
                              keystroke information for the key that was sent
                              from terminal.

  @retval EFI_SUCCESS         The keystroke information is returned successfully.
  @retval EFI_NOT_READY       There is no keystroke data available.
  @retval EFI_DEVICE_ERROR    The dependent serial device encounters error.

**/
EFI_STATUS
EFIAPI
TerminalConInReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  OUT EFI_INPUT_KEY                   *Key
  );

/**
  Check if the key already has been registered.

  @param  RegsiteredData           A pointer to a buffer that is filled in with the
                                   keystroke state data for the key that was
                                   registered.
  @param  InputData                A pointer to a buffer that is filled in with the
                                   keystroke state data for the key that was
                                   pressed.

  @retval TRUE                     Key be pressed matches a registered key.
  @retval FALSE                    Match failed.

**/
BOOLEAN
IsKeyRegistered (
  IN EFI_KEY_DATA  *RegsiteredData,
  IN EFI_KEY_DATA  *InputData
  );

/**
  Event notification function for EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL.WaitForKeyEx event
  Signal the event if there is key available

  @param  Event                    Indicates the event that invoke this function.
  @param  Context                  Indicates the calling context.

**/
VOID
EFIAPI
TerminalConInWaitForKeyEx (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  );

//
// Simple Text Input Ex protocol prototypes
//

/**
  Reset the input device and optionally run diagnostics

  @param  This                     Protocol instance pointer.
  @param  ExtendedVerification     Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS              The device was reset.
  @retval EFI_DEVICE_ERROR         The device is not functioning properly and could
                                   not be reset.

**/
EFI_STATUS
EFIAPI
TerminalConInResetEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  );

/**
  Reads the next keystroke from the input device. The WaitForKey Event can
  be used to test for existence of a keystroke via WaitForEvent () call.

  @param  This                     Protocol instance pointer.
  @param  KeyData                  A pointer to a buffer that is filled in with the
                                   keystroke state data for the key that was
                                   pressed.

  @retval EFI_SUCCESS              The keystroke information was returned.
  @retval EFI_NOT_READY            There was no keystroke data available.
  @retval EFI_DEVICE_ERROR         The keystroke information was not returned due
                                   to hardware errors.
  @retval EFI_INVALID_PARAMETER    KeyData is NULL.

**/
EFI_STATUS
EFIAPI
TerminalConInReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
  OUT EFI_KEY_DATA                      *KeyData
  );

/**
  Set certain state for the input device.

  @param  This                     Protocol instance pointer.
  @param  KeyToggleState           A pointer to the EFI_KEY_TOGGLE_STATE to set the
                                   state for the input device.

  @retval EFI_SUCCESS              The device state was set successfully.
  @retval EFI_DEVICE_ERROR         The device is not functioning correctly and
                                   could not have the setting adjusted.
  @retval EFI_UNSUPPORTED          The device does not have the ability to set its
                                   state.
  @retval EFI_INVALID_PARAMETER    KeyToggleState is NULL.

**/
EFI_STATUS
EFIAPI
TerminalConInSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  );

/**
  Register a notification function for a particular keystroke for the input device.

  @param  This                     Protocol instance pointer.
  @param  KeyData                  A pointer to a buffer that is filled in with the
                                   keystroke information data for the key that was
                                   pressed. If KeyData.Key, KeyData.KeyState.KeyToggleState
                                   and KeyData.KeyState.KeyShiftState are 0, then any incomplete
                                   keystroke will trigger a notification of the KeyNotificationFunction.
  @param  KeyNotificationFunction  Points to the function to be called when the key
                                   sequence is typed specified by KeyData. This notification function
                                   should be called at <=TPL_CALLBACK.
  @param  NotifyHandle             Points to the unique handle assigned to the
                                   registered notification.

  @retval EFI_SUCCESS              The notification function was registered
                                   successfully.
  @retval EFI_OUT_OF_RESOURCES     Unable to allocate resources for necesssary data
                                   structures.
  @retval EFI_INVALID_PARAMETER    KeyData or NotifyHandle is NULL.

**/
EFI_STATUS
EFIAPI
TerminalConInRegisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_DATA                       *KeyData,
  IN EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT VOID                              **NotifyHandle
  );

/**
  Remove a registered notification function from a particular keystroke.

  @param  This                     Protocol instance pointer.
  @param  NotificationHandle       The handle of the notification function being
                                   unregistered.

  @retval EFI_SUCCESS              The notification function was unregistered
                                   successfully.
  @retval EFI_INVALID_PARAMETER    The NotificationHandle is invalid.
  @retval EFI_NOT_FOUND            Can not find the matching entry in database.

**/
EFI_STATUS
EFIAPI
TerminalConInUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
  );

/**
  Event notification function for EFI_SIMPLE_TEXT_INPUT_PROTOCOL.WaitForKey event
  Signal the event if there is key available

  @param  Event                    Indicates the event that invoke this function.
  @param  Context                  Indicates the calling context.

**/
VOID
EFIAPI
TerminalConInWaitForKey (
  IN  EFI_EVENT     Event,
  IN  VOID          *Context
  );

/**
  Implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.Reset().
  If ExtendeVerification is TRUE, then perform dependent serial device reset,
  and set display mode to mode 0.
  If ExtendedVerification is FALSE, only set display mode to mode 0.

  @param  This                  Indicates the calling context.
  @param  ExtendedVerification  Indicates that the driver may perform a more
                                exhaustive verification operation of the device
                                during reset.

  @retval EFI_SUCCESS           The reset operation succeeds.
  @retval EFI_DEVICE_ERROR      The terminal is not functioning correctly or the serial port reset fails.

**/
EFI_STATUS
EFIAPI
TerminalConOutReset (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *This,
  IN  BOOLEAN                            ExtendedVerification
  );

/**
  Implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.OutputString().
  The Unicode string will be converted to terminal expressible data stream
  and send to terminal via serial port.

  @param  This                    Indicates the calling context.
  @param  WString                 The Null-terminated Unicode string to be displayed
                                  on the terminal screen.

  @retval EFI_SUCCESS             The string is output successfully.
  @retval EFI_DEVICE_ERROR        The serial port fails to send the string out.
  @retval EFI_WARN_UNKNOWN_GLYPH  Indicates that some of the characters in the Unicode string could not
                                  be rendered and are skipped.
  @retval EFI_UNSUPPORTED         If current display mode is out of range.

**/
EFI_STATUS
EFIAPI
TerminalConOutOutputString (
  IN   EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                            *WString
  );

/**
  Implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.TestString().
  If one of the characters in the *Wstring is
  neither valid Unicode drawing characters,
  not ASCII code, then this function will return
  EFI_UNSUPPORTED.

  @param  This              Indicates the calling context.
  @param  WString           The Null-terminated Unicode string to be tested.

  @retval EFI_SUCCESS       The terminal is capable of rendering the output string.
  @retval EFI_UNSUPPORTED   Some of the characters in the Unicode string cannot be rendered.

**/
EFI_STATUS
EFIAPI
TerminalConOutTestString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *WString
  );

/**
  Implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.QueryMode().
  It returns information for an available text mode
  that the terminal supports.
  In this driver, we support text mode 80x25 (mode 0),
  80x50 (mode 1), 100x31 (mode 2).

  @param This        Indicates the calling context.
  @param ModeNumber  The mode number to return information on.
  @param Columns     The returned columns of the requested mode.
  @param Rows        The returned rows of the requested mode.

  @retval EFI_SUCCESS       The requested mode information is returned.
  @retval EFI_UNSUPPORTED   The mode number is not valid.
  @retval EFI_DEVICE_ERROR

**/
EFI_STATUS
EFIAPI
TerminalConOutQueryMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber,
  OUT UINTN                            *Columns,
  OUT UINTN                            *Rows
  );

/**
  Implements EFI_SIMPLE_TEXT_OUT.SetMode().
  Set the terminal to a specified display mode.
  In this driver, we only support mode 0.

  @param This          Indicates the calling context.
  @param ModeNumber    The text mode to set.

  @retval EFI_SUCCESS       The requested text mode is set.
  @retval EFI_DEVICE_ERROR  The requested text mode cannot be set
                            because of serial device error.
  @retval EFI_UNSUPPORTED   The text mode number is not valid.

**/
EFI_STATUS
EFIAPI
TerminalConOutSetMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber
  );

/**
  Implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.SetAttribute().

  @param This        Indicates the calling context.
  @param Attribute   The attribute to set. Only bit0..6 are valid, all other bits
                     are undefined and must be zero.

  @retval EFI_SUCCESS        The requested attribute is set.
  @retval EFI_DEVICE_ERROR   The requested attribute cannot be set due to serial port error.
  @retval EFI_UNSUPPORTED    The attribute requested is not defined by EFI spec.

**/
EFI_STATUS
EFIAPI
TerminalConOutSetAttribute (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            Attribute
  );

/**
  Implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.ClearScreen().
  It clears the ANSI terminal's display to the
  currently selected background color.

  @param This     Indicates the calling context.

  @retval EFI_SUCCESS       The operation completed successfully.
  @retval EFI_DEVICE_ERROR  The terminal screen cannot be cleared due to serial port error.
  @retval EFI_UNSUPPORTED   The terminal is not in a valid display mode.

**/
EFI_STATUS
EFIAPI
TerminalConOutClearScreen (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This
  );

/**
  Implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.SetCursorPosition().

  @param This      Indicates the calling context.
  @param Column    The row to set cursor to.
  @param Row       The column to set cursor to.

  @retval EFI_SUCCESS       The operation completed successfully.
  @retval EFI_DEVICE_ERROR  The request fails due to serial port error.
  @retval EFI_UNSUPPORTED   The terminal is not in a valid text mode, or the cursor position
                            is invalid for current mode.

**/
EFI_STATUS
EFIAPI
TerminalConOutSetCursorPosition (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            Column,
  IN  UINTN                            Row
  );

/**
  Implements SIMPLE_TEXT_OUTPUT.EnableCursor().
  In this driver, the cursor cannot be hidden.

  @param This      Indicates the calling context.
  @param Visible   If TRUE, the cursor is set to be visible,
                   If FALSE, the cursor is set to be invisible.

  @retval EFI_SUCCESS      The request is valid.
  @retval EFI_UNSUPPORTED  The terminal does not support cursor hidden.

**/
EFI_STATUS
EFIAPI
TerminalConOutEnableCursor (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  BOOLEAN                          Visible
  );

/**
  Test to see if this driver supports Controller.

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval EFI_ALREADY_STARTED This driver is already running on this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
TerminalDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

/**
  Start this driver on Controller by opening a Serial IO protocol,
  reading Device Path, and creating a child handle with a Simple Text In,
  Simple Text In Ex and Simple Text Out protocol, and device path protocol.
  And store Console Device Environment Variables.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to Controller.
  @retval EFI_ALREADY_STARTED  This driver is already running on Controller.
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
TerminalDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );


/**
  Stop this driver on Controller by closing Simple Text In, Simple Text
  In Ex, Simple Text Out protocol, and removing parent device path from
  Console Device Environment Variables.

  @param  This              Protocol instance pointer.
  @param  Controller        Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed Controller.
  @retval other             This driver could not be removed from this device.

**/
EFI_STATUS
EFIAPI
TerminalDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  );

/**
  Free notify functions list.

  @param  ListHead               The list head

  @retval EFI_SUCCESS            Free the notify list successfully.
  @retval EFI_INVALID_PARAMETER  ListHead is NULL.

**/
EFI_STATUS
TerminalFreeNotifyList (
  IN OUT LIST_ENTRY           *ListHead
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
TerminalComponentNameGetDriverName (
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
TerminalComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );


//
// internal functions
//

/**
  Check for a pending key in the Efi Key FIFO or Serial device buffer.

  @param  This                     Indicates the calling context.

  @retval EFI_SUCCESS              There is key pending.
  @retval EFI_NOT_READY            There is no key pending.
  @retval EFI_DEVICE_ERROR         If Serial IO is not attached to serial device.

**/
EFI_STATUS
TerminalConInCheckForKey (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This
  );

/**
  Update terminal device path in Console Device Environment Variables.

  @param  VariableName           The Console Device Environment Variable.
  @param  ParentDevicePath       The terminal device path to be updated.

  @return None.

**/
VOID
TerminalUpdateConsoleDevVariable (
  IN CHAR16                    *VariableName,
  IN EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath
  );

/**
  Remove console device variable.

  @param  VariableName           A pointer to the variable name.
  @param  ParentDevicePath       A pointer to the parent device path.

**/
VOID
TerminalRemoveConsoleDevVariable (
  IN CHAR16                    *VariableName,
  IN EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath
  );

/**
  Build termial device path according to terminal type.

  @param  TerminalType           The terminal type is PC ANSI, VT100, VT100+, VT-UTF8, TTY-Term,
                                 Linux, XtermR6, VT400 and SCO.
  @param  ParentDevicePath       Parent device path.
  @param  TerminalDevicePath     Returned terminal device path, if building successfully.

  @retval EFI_UNSUPPORTED        Terminal does not belong to the supported type.
  @retval EFI_OUT_OF_RESOURCES   Generate terminal device path failed.
  @retval EFI_SUCCESS            Build terminal device path successfully.

**/
EFI_STATUS
SetTerminalDevicePath (
  IN  TERMINAL_TYPE               TerminalType,
  IN  EFI_DEVICE_PATH_PROTOCOL    *ParentDevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL    **TerminalDevicePath
  );

/**
  Get one key out of serial buffer.

  @param  SerialIo           Serial I/O protocl attached to the serial device.
  @param  Input              The fetched key.

  @retval EFI_NOT_READY      If serial buffer is empty.
  @retval EFI_DEVICE_ERROR   If reading serial buffer encounter error.
  @retval EFI_SUCCESS        If reading serial buffer successfully, put
                             the fetched key to the parameter output.

**/
EFI_STATUS
GetOneKeyFromSerial (
  EFI_SERIAL_IO_PROTOCOL  *SerialIo,
  UINT8                   *Input
  );

/**
  Insert one byte raw data into the Raw Data FIFO.

  @param  TerminalDevice       Terminal driver private structure.
  @param  Input                The key will be input.

  @retval TRUE                 If insert successfully.
  @retval FALSE                If Raw Data buffer is full before key insertion,
                               and the key is lost.

**/
BOOLEAN
RawFiFoInsertOneKey (
  TERMINAL_DEV  *TerminalDevice,
  UINT8         Input
  );

/**
  Remove one pre-fetched key out of the Raw Data FIFO.

  @param  TerminalDevice       Terminal driver private structure.
  @param  Output               The key will be removed.

  @retval TRUE                 If insert successfully.
  @retval FALSE                If Raw Data FIFO buffer is empty before remove operation.

**/
BOOLEAN
RawFiFoRemoveOneKey (
  TERMINAL_DEV  *TerminalDevice,
  UINT8         *Output
  );

/**
  Clarify whether Raw Data FIFO buffer is empty.

  @param  TerminalDevice       Terminal driver private structure

  @retval TRUE                 If Raw Data FIFO buffer is empty.
  @retval FALSE                If Raw Data FIFO buffer is not empty.

**/
BOOLEAN
IsRawFiFoEmpty (
  TERMINAL_DEV  *TerminalDevice
  );

/**
  Clarify whether Raw Data FIFO buffer is full.

  @param  TerminalDevice       Terminal driver private structure

  @retval TRUE                 If Raw Data FIFO buffer is full.
  @retval FALSE                If Raw Data FIFO buffer is not full.

**/
BOOLEAN
IsRawFiFoFull (
  TERMINAL_DEV  *TerminalDevice
  );

/**
  Insert one pre-fetched key into the FIFO buffer.

  @param  EfiKeyFiFo            Pointer to instance of EFI_KEY_FIFO.
  @param  Input                 The key will be input.

  @retval TRUE                  If insert successfully.
  @retval FALSE                 If FIFO buffer is full before key insertion,
                                and the key is lost.

**/
BOOLEAN
EfiKeyFiFoForNotifyInsertOneKey (
  EFI_KEY_FIFO                  *EfiKeyFiFo,
  EFI_INPUT_KEY                 *Input
  );

/**
  Remove one pre-fetched key out of the FIFO buffer.

  @param  EfiKeyFiFo            Pointer to instance of EFI_KEY_FIFO.
  @param  Output                The key will be removed.

  @retval TRUE                  If insert successfully.
  @retval FALSE                 If FIFO buffer is empty before remove operation.

**/
BOOLEAN
EfiKeyFiFoForNotifyRemoveOneKey (
  EFI_KEY_FIFO                  *EfiKeyFiFo,
  EFI_INPUT_KEY                 *Output
  );

/**
  Clarify whether FIFO buffer is empty.

  @param  EfiKeyFiFo            Pointer to instance of EFI_KEY_FIFO.

  @retval TRUE                  If FIFO buffer is empty.
  @retval FALSE                 If FIFO buffer is not empty.

**/
BOOLEAN
IsEfiKeyFiFoForNotifyEmpty (
  IN EFI_KEY_FIFO               *EfiKeyFiFo
  );

/**
  Clarify whether FIFO buffer is full.

  @param  EfiKeyFiFo            Pointer to instance of EFI_KEY_FIFO.

  @retval TRUE                  If FIFO buffer is full.
  @retval FALSE                 If FIFO buffer is not full.

**/
BOOLEAN
IsEfiKeyFiFoForNotifyFull (
  EFI_KEY_FIFO                  *EfiKeyFiFo
  );

/**
  Insert one pre-fetched key into the FIFO buffer.

  @param  TerminalDevice       Terminal driver private structure.
  @param  Key                  The key will be input.

  @retval TRUE                 If insert successfully.
  @retval FALSE                If FIFO buffer is full before key insertion,
                               and the key is lost.

**/
BOOLEAN
EfiKeyFiFoInsertOneKey (
  TERMINAL_DEV      *TerminalDevice,
  EFI_INPUT_KEY     *Key
  );

/**
  Remove one pre-fetched key out of the FIFO buffer.

  @param  TerminalDevice       Terminal driver private structure.
  @param  Output               The key will be removed.

  @retval TRUE                 If insert successfully.
  @retval FALSE                If FIFO buffer is empty before remove operation.

**/
BOOLEAN
EfiKeyFiFoRemoveOneKey (
  TERMINAL_DEV  *TerminalDevice,
  EFI_INPUT_KEY *Output
  );

/**
  Clarify whether FIFO buffer is empty.

  @param  TerminalDevice       Terminal driver private structure

  @retval TRUE                 If FIFO buffer is empty.
  @retval FALSE                If FIFO buffer is not empty.

**/
BOOLEAN
IsEfiKeyFiFoEmpty (
  TERMINAL_DEV  *TerminalDevice
  );

/**
  Clarify whether FIFO buffer is full.

  @param  TerminalDevice       Terminal driver private structure

  @retval TRUE                 If FIFO buffer is full.
  @retval FALSE                If FIFO buffer is not full.

**/
BOOLEAN
IsEfiKeyFiFoFull (
  TERMINAL_DEV  *TerminalDevice
  );

/**
  Insert one pre-fetched key into the Unicode FIFO buffer.

  @param  TerminalDevice       Terminal driver private structure.
  @param  Input                The key will be input.

  @retval TRUE                 If insert successfully.
  @retval FALSE                If Unicode FIFO buffer is full before key insertion,
                               and the key is lost.

**/
BOOLEAN
UnicodeFiFoInsertOneKey (
  TERMINAL_DEV      *TerminalDevice,
  UINT16            Input
  );

/**
  Remove one pre-fetched key out of the Unicode FIFO buffer.
  The caller should guarantee that Unicode FIFO buffer is not empty
  by IsUnicodeFiFoEmpty ().

  @param  TerminalDevice       Terminal driver private structure.
  @param  Output               The key will be removed.

**/
VOID
UnicodeFiFoRemoveOneKey (
  TERMINAL_DEV  *TerminalDevice,
  UINT16        *Output
  );

/**
  Clarify whether Unicode FIFO buffer is empty.

  @param  TerminalDevice       Terminal driver private structure

  @retval TRUE                 If Unicode FIFO buffer is empty.
  @retval FALSE                If Unicode FIFO buffer is not empty.

**/
BOOLEAN
IsUnicodeFiFoEmpty (
  TERMINAL_DEV  *TerminalDevice
  );

/**
  Clarify whether Unicode FIFO buffer is full.

  @param  TerminalDevice       Terminal driver private structure

  @retval TRUE                 If Unicode FIFO buffer is full.
  @retval FALSE                If Unicode FIFO buffer is not full.

**/
BOOLEAN
IsUnicodeFiFoFull (
  TERMINAL_DEV  *TerminalDevice
  );


/**
  Translate raw data into Unicode (according to different encode), and
  translate Unicode into key information. (according to different standard).

  @param  TerminalDevice       Terminal driver private structure.

**/
VOID
TranslateRawDataToEfiKey (
  IN  TERMINAL_DEV      *TerminalDevice
  );

//
// internal functions for PC ANSI
//

/**
  Translate all raw data in the Raw FIFI into unicode, and insert
  them into Unicode FIFO.

  @param TerminalDevice          The terminal device.

**/
VOID
AnsiRawDataToUnicode (
  IN  TERMINAL_DEV    *TerminalDevice
  );

/**
  Converts a stream of Unicode characters from a terminal input device into EFI Keys that
  can be read through the Simple Input Protocol.

  The table below shows the keyboard input mappings that this function supports.
  If the ESC sequence listed in one of the columns is presented, then it is translated
  into the coorespoding EFI Scan Code.  If a matching sequence is not found, then the raw
  key strokes are converted into EFI Keys.

  2 seconds are allowed for an ESC sequence to be completed.  If the ESC sequence is not
  completed in 2 seconds, then the raw key strokes of the partial ESC sequence are
  converted into EFI Keys.
  There is one special input sequence that will force the system to reset.
  This is ESC R ESC r ESC R.

  Symbols used in table below
  ===========================
    ESC = 0x1B
    CSI = 0x9B
    DEL = 0x7f
    ^   = CTRL
  +=========+======+===========+==========+==========+
  |         | EFI  | UEFI 2.0  |          |          |
  |         | Scan |           |  VT100+  |          |
  |   KEY   | Code |  PC ANSI  |  VTUTF8  |   VT100  |
  +=========+======+===========+==========+==========+
  | NULL    | 0x00 |           |          |          |
  | UP      | 0x01 | ESC [ A   | ESC [ A  | ESC [ A  |
  | DOWN    | 0x02 | ESC [ B   | ESC [ B  | ESC [ B  |
  | RIGHT   | 0x03 | ESC [ C   | ESC [ C  | ESC [ C  |
  | LEFT    | 0x04 | ESC [ D   | ESC [ D  | ESC [ D  |
  | HOME    | 0x05 | ESC [ H   | ESC h    | ESC [ H  |
  | END     | 0x06 | ESC [ F   | ESC k    | ESC [ K  |
  | INSERT  | 0x07 | ESC [ @   | ESC +    | ESC [ @  |
  |         |      | ESC [ L   |          | ESC [ L  |
  | DELETE  | 0x08 | ESC [ X   | ESC -    | ESC [ P  |
  | PG UP   | 0x09 | ESC [ I   | ESC ?    | ESC [ V  |
  |         |      |           |          | ESC [ ?  |
  | PG DOWN | 0x0A | ESC [ G   | ESC /    | ESC [ U  |
  |         |      |           |          | ESC [ /  |
  | F1      | 0x0B | ESC [ M   | ESC 1    | ESC O P  |
  | F2      | 0x0C | ESC [ N   | ESC 2    | ESC O Q  |
  | F3      | 0x0D | ESC [ O   | ESC 3    | ESC O w  |
  | F4      | 0x0E | ESC [ P   | ESC 4    | ESC O x  |
  | F5      | 0x0F | ESC [ Q   | ESC 5    | ESC O t  |
  | F6      | 0x10 | ESC [ R   | ESC 6    | ESC O u  |
  | F7      | 0x11 | ESC [ S   | ESC 7    | ESC O q  |
  | F8      | 0x12 | ESC [ T   | ESC 8    | ESC O r  |
  | F9      | 0x13 | ESC [ U   | ESC 9    | ESC O p  |
  | F10     | 0x14 | ESC [ V   | ESC 0    | ESC O M  |
  | Escape  | 0x17 | ESC       | ESC      | ESC      |
  | F11     | 0x15 |           | ESC !    |          |
  | F12     | 0x16 |           | ESC @    |          |
  +=========+======+===========+==========+==========+

Putty function key map:
  +=========+======+===========+=============+=============+=============+=========+
  |         | EFI  |           |             |             |             |         |
  |         | Scan |           |             |  Normal     |             |         |
  |   KEY   | Code |  VT100+   | Xterm R6    |  VT400      | Linux       | SCO     |
  +=========+======+===========+=============+=============+=============+=========+
  | F1      | 0x0B | ESC O P   | ESC O P     | ESC [ 1 1 ~ | ESC [ [ A   | ESC [ M |
  | F2      | 0x0C | ESC O Q   | ESC O Q     | ESC [ 1 2 ~ | ESC [ [ B   | ESC [ N |
  | F3      | 0x0D | ESC O R   | ESC O R     | ESC [ 1 3 ~ | ESC [ [ C   | ESC [ O |
  | F4      | 0x0E | ESC O S   | ESC O S     | ESC [ 1 4 ~ | ESC [ [ D   | ESC [ P |
  | F5      | 0x0F | ESC O T   | ESC [ 1 5 ~ | ESC [ 1 5 ~ | ESC [ [ E   | ESC [ Q |
  | F6      | 0x10 | ESC O U   | ESC [ 1 7 ~ | ESC [ 1 7 ~ | ESC [ 1 7 ~ | ESC [ R |
  | F7      | 0x11 | ESC O V   | ESC [ 1 8 ~ | ESC [ 1 8 ~ | ESC [ 1 8 ~ | ESC [ S |
  | F8      | 0x12 | ESC O W   | ESC [ 1 9 ~ | ESC [ 1 9 ~ | ESC [ 1 9 ~ | ESC [ T |
  | F9      | 0x13 | ESC O X   | ESC [ 2 0 ~ | ESC [ 2 0 ~ | ESC [ 2 0 ~ | ESC [ U |
  | F10     | 0x14 | ESC O Y   | ESC [ 2 1 ~ | ESC [ 2 1 ~ | ESC [ 2 1 ~ | ESC [ V |
  | Escape  | 0x17 | ESC       | ESC         | ESC         | ESC         | ESC     |
  | F11     | 0x15 | ESC O Z   | ESC [ 2 3 ~ | ESC [ 2 3 ~ | ESC [ 2 3 ~ | ESC [ W |
  | F12     | 0x16 | ESC O [   | ESC [ 2 4 ~ | ESC [ 2 4 ~ | ESC [ 2 4 ~ | ESC [ X |
  +=========+======+===========+=============+=============+=============+=========+


  Special Mappings
  ================
  ESC R ESC r ESC R = Reset System


  @param TerminalDevice   The terminal device to use to translate raw input into EFI Keys

**/
VOID
UnicodeToEfiKey (
  IN  TERMINAL_DEV    *TerminalDevice
  );

/**
  Check if input string is valid Ascii string, valid EFI control characters
  or valid text graphics.

  @param  TerminalDevice          The terminal device.
  @param  WString                 The input string.

  @retval EFI_UNSUPPORTED         If not all input characters are valid.
  @retval EFI_SUCCESS             If all input characters are valid.

**/
EFI_STATUS
AnsiTestString (
  IN  TERMINAL_DEV    *TerminalDevice,
  IN  CHAR16          *WString
  );

//
// internal functions for VTUTF8
//

/**
  Translate all VT-UTF8 characters in the Raw FIFI into unicode characters,
  and insert them into Unicode FIFO.

  @param VtUtf8Device          The terminal device.

**/
VOID
VTUTF8RawDataToUnicode (
  IN  TERMINAL_DEV    *VtUtf8Device
  );

/**
  Check if input string is valid VT-UTF8 string.

  @param  TerminalDevice          The terminal device.
  @param  WString                 The input string.

  @retval EFI_SUCCESS             If all input characters are valid.

**/
EFI_STATUS
VTUTF8TestString (
  IN  TERMINAL_DEV    *TerminalDevice,
  IN  CHAR16          *WString
  );

/**
  Translate one Unicode character into VT-UTF8 characters.

  UTF8 Encoding Table
  Bits per Character | Unicode Character Range | Unicode Binary  Encoding |  UTF8 Binary Encoding
        0-7           |     0x0000 - 0x007F      |     00000000 0xxxxxxx     |   0xxxxxxx
        8-11          |     0x0080 - 0x07FF      |     00000xxx xxxxxxxx     |   110xxxxx 10xxxxxx
       12-16          |     0x0800 - 0xFFFF      |     xxxxxxxx xxxxxxxx     |   1110xxxx 10xxxxxx 10xxxxxx


  @param  Unicode          Unicode character need translating.
  @param  Utf8Char         Return VT-UTF8 character set.
  @param  ValidBytes       The count of valid VT-UTF8 characters. If
                           ValidBytes is zero, no valid VT-UTF8 returned.

**/
VOID
UnicodeToUtf8 (
  IN  CHAR16      Unicode,
  OUT UTF8_CHAR   *Utf8Char,
  OUT UINT8       *ValidBytes
  );

/**
  Get one valid VT-UTF8 characters set from Raw Data FIFO.

  @param  Utf8Device          The terminal device.
  @param  Utf8Char            Returned valid VT-UTF8 characters set.
  @param  ValidBytes          The count of returned VT-VTF8 characters.
                              If ValidBytes is zero, no valid VT-UTF8 returned.

**/
VOID
GetOneValidUtf8Char (
  IN  TERMINAL_DEV      *Utf8Device,
  OUT UTF8_CHAR         *Utf8Char,
  OUT UINT8             *ValidBytes
  );

/**
  Translate VT-UTF8 characters into one Unicode character.

  UTF8 Encoding Table
  Bits per Character | Unicode Character Range | Unicode Binary  Encoding |  UTF8 Binary Encoding
        0-7           |     0x0000 - 0x007F      |     00000000 0xxxxxxx     |   0xxxxxxx
        8-11          |     0x0080 - 0x07FF      |     00000xxx xxxxxxxx     |   110xxxxx 10xxxxxx
       12-16          |     0x0800 - 0xFFFF      |     xxxxxxxx xxxxxxxx     |   1110xxxx 10xxxxxx 10xxxxxx


  @param  Utf8Char         VT-UTF8 character set needs translating.
  @param  ValidBytes       The count of valid VT-UTF8 characters.
  @param  UnicodeChar      Returned unicode character.

**/
VOID
Utf8ToUnicode (
  IN  UTF8_CHAR       Utf8Char,
  IN  UINT8           ValidBytes,
  OUT CHAR16          *UnicodeChar
  );

//
// functions for boxdraw unicode
//

/**
  Detects if a Unicode char is for Box Drawing text graphics.

  @param  Graphic      Unicode char to test.
  @param  PcAnsi       Optional pointer to return PCANSI equivalent of
                       Graphic.
  @param  Ascii        Optional pointer to return ASCII equivalent of
                       Graphic.

  @retval TRUE         If Graphic is a supported Unicode Box Drawing character.

**/
BOOLEAN
TerminalIsValidTextGraphics (
  IN  CHAR16  Graphic,
  OUT CHAR8   *PcAnsi, OPTIONAL
  OUT CHAR8   *Ascii OPTIONAL
  );

/**
  Detects if a valid ASCII char.

  @param  Ascii        An ASCII character.

  @retval TRUE         If it is a valid ASCII character.
  @retval FALSE        If it is not a valid ASCII character.

**/
BOOLEAN
TerminalIsValidAscii (
  IN  CHAR16  Ascii
  );

/**
  Detects if a valid EFI control character.

  @param  CharC        An input EFI Control character.

  @retval TRUE         If it is a valid EFI control character.
  @retval FALSE        If it is not a valid EFI control character.

**/
BOOLEAN
TerminalIsValidEfiCntlChar (
  IN  CHAR16  CharC
  );

/**
  Check if the device supports hot-plug through its device path.

  This function could be updated to check more types of Hot Plug devices.
  Currently, it checks USB and PCCard device.

  @param  DevicePath            Pointer to device's device path.

  @retval TRUE                  The devcie is a hot-plug device
  @retval FALSE                 The devcie is not a hot-plug device.

**/
BOOLEAN
IsHotPlugDevice (
  IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath
  );

/**
  Timer handler to poll the key from serial.

  @param  Event                    Indicates the event that invoke this function.
  @param  Context                  Indicates the calling context.
**/
VOID
EFIAPI
TerminalConInTimerHandler (
  IN EFI_EVENT            Event,
  IN VOID                 *Context
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

#endif
