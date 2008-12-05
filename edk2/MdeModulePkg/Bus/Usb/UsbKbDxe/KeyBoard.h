/** @file
  Function prototype for USB Keyboard Driver.

Copyright (c) 2004 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_KEYBOARD_H_
#define _EFI_KEYBOARD_H_


#include "EfiKey.h"

/**
  Uses USB I/O to check whether the device is a USB Keyboard device.

  @param  UsbIo    Points to a USB I/O protocol instance.
  @retval None

**/
BOOLEAN
EFIAPI
IsUSBKeyboard (
  IN  EFI_USB_IO_PROTOCOL       *UsbIo
  );

/**
  Initialize USB Keyboard device and all private data structures.

  @param  UsbKeyboardDevice  The USB_KB_DEV instance.

  @retval EFI_SUCCESS        Initialization is successful.
  @retval EFI_DEVICE_ERROR   Configure hardware failed.

**/
EFI_STATUS
EFIAPI
InitUSBKeyboard (
  IN USB_KB_DEV   *UsbKeyboardDevice
  );

/**
  Initialize USB Keyboard layout.

  @param  UsbKeyboardDevice      The USB_KB_DEV instance.

  @retval EFI_SUCCESS            Initialization Success.
  @retval Other                  Keyboard layout initial failed.

**/
EFI_STATUS
EFIAPI
InitKeyboardLayout (
  IN USB_KB_DEV   *UsbKeyboardDevice
  );

/**
  Destroy resources for Keyboard layout.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.

**/
VOID
EFIAPI
ReleaseKeyboardLayoutResources (
  IN USB_KB_DEV  *UsbKeyboardDevice
  );

/**
  Handler function for USB Keyboard's asynchronous interrupt transfer.

  @param  Data             A pointer to a buffer that is filled with key data which is
                           retrieved via asynchronous interrupt transfer.
  @param  DataLength       Indicates the size of the data buffer.
  @param  Context          Pointing to USB_KB_DEV instance.
  @param  Result           Indicates the result of the asynchronous interrupt transfer.

  @retval EFI_SUCCESS      Handler is successful.
  @retval EFI_DEVICE_ERROR Hardware Error

**/
EFI_STATUS
EFIAPI
KeyboardHandler (
  IN  VOID          *Data,
  IN  UINTN         DataLength,
  IN  VOID          *Context,
  IN  UINT32        Result
  );

/**
  Timer handler for Delayed Recovery timer.

  @param  Event              The Delayed Recovery event.
  @param  Context            Points to the USB_KB_DEV instance.


**/
VOID
EFIAPI
USBKeyboardRecoveryHandler (
  IN    EFI_EVENT    Event,
  IN    VOID         *Context
  );

/**
  Retrieves a key character after parsing the raw data in keyboard buffer.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.
  @param  KeyChar              Points to the Key character after key parsing.

  @retval EFI_SUCCESS          Parse key is successful.
  @retval EFI_NOT_READY        Device is not ready.

**/
EFI_STATUS
EFIAPI
USBParseKey (
  IN OUT  USB_KB_DEV  *UsbKeyboardDevice,
  OUT     UINT8       *KeyChar
  );

/**
  Converts USB Keyboard code to EFI Scan Code.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.
  @param  KeyChar              Indicates the key code that will be interpreted.
  @param  Key                  A pointer to a buffer that is filled in with
                               the keystroke information for the key that
                               was pressed.

  @retval EFI_NOT_READY        Device is not ready
  @retval EFI_SUCCESS          Success.

**/
EFI_STATUS
EFIAPI
UsbKeyCodeToEfiInputKey (
  IN  USB_KB_DEV      *UsbKeyboardDevice,
  IN  UINT8           KeyChar,
  OUT EFI_INPUT_KEY   *Key
  );

/**
  Resets USB Keyboard Buffer.

  @param  KeyboardBuffer     Points to the USB Keyboard Buffer.

**/
VOID
EFIAPI
InitUSBKeyBuffer (
  IN OUT  USB_KB_BUFFER   *KeyboardBuffer
  );

/**
  Check whether USB Keyboard buffer is empty.

  @param  KeyboardBuffer     USB Keyboard Buffer.

  @retval TRUE               Key buffer is empty.
  @retval FALSE              Key buffer is not empty.

**/
BOOLEAN
EFIAPI
IsUSBKeyboardBufferEmpty (
  IN  USB_KB_BUFFER   *KeyboardBuffer
  );

/**
  Check whether USB Keyboard buffer is full.

  @param  KeyboardBuffer     USB Keyboard Buffer.

  @retval TRUE               Key buffer is full.
  @retval FALSE              Key buffer is not full.

**/
BOOLEAN
EFIAPI
IsUSBKeyboardBufferFull (
  IN  USB_KB_BUFFER   *KeyboardBuffer
  );

/**
  Inserts a key code into keyboard buffer.

  @param  KeyboardBuffer     Points to the USB Keyboard Buffer.
  @param  Key                Key code
  @param  Down               Special key

**/
VOID
EFIAPI
InsertKeyCode (
  IN OUT  USB_KB_BUFFER *KeyboardBuffer,
  IN      UINT8         Key,
  IN      UINT8         Down
  );

/**
  Pops a key code off from keyboard buffer.

  @param  KeyboardBuffer     Points to the USB Keyboard Buffer.
  @param  UsbKey             Points to the buffer that contains a usb key code.

  @retval EFI_SUCCESS        Success
  @retval EFI_DEVICE_ERROR   Hardware Error

**/
EFI_STATUS
EFIAPI
RemoveKeyCode (
  IN OUT  USB_KB_BUFFER *KeyboardBuffer,
  OUT     USB_KEY       *UsbKey
  );

/**
  Timer handler for Repeat Key timer.

  @param  Event              The Repeat Key event.
  @param  Context            Points to the USB_KB_DEV instance.


**/
VOID
EFIAPI
USBKeyboardRepeatHandler (
  IN    EFI_EVENT    Event,
  IN    VOID         *Context
  );

/**
  Sets USB Keyboard LED state.

  @param  UsbKeyboardDevice  The USB_KB_DEV instance.

**/
VOID
EFIAPI
SetKeyLED (
  IN  USB_KB_DEV    *UsbKeyboardDevice
  );

#endif
