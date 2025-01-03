/** @file

  Private definitions of the VirtioKeyboard driver

  Copyright (C) 2024, Red Hat

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VIRTIO_KEYBOARD_DXE_H_
#define _VIRTIO_KEYBOARD_DXE_H_

#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>

#include <IndustryStandard/Virtio.h>

#define VIRTIO_KBD_SIG  SIGNATURE_32 ('V', 'K', 'B', 'D')

#define KEYBOARD_MAX_RINGS   2
#define KEYBOARD_RX_BUFSIZE  64

// Fetch new key from VirtIO every 50ms
#define KEYBOARD_PROBE_TIME_MS  50

// Max range of recognized keyboard codes
#define MAX_KEYBOARD_CODE  255

typedef struct {
  UINTN                      Signature;
  EFI_KEY_DATA               KeyData;
  EFI_KEY_NOTIFY_FUNCTION    KeyNotificationFn;
  LIST_ENTRY                 NotifyEntry;
} VIRTIO_KBD_IN_EX_NOTIFY;

// Data structure representing payload delivered from VirtIo
typedef struct {
  UINT16    Type;
  UINT16    Code;
  UINT32    Value;
} VIRTIO_KBD_EVENT;

// Data structure representing ring buffer
typedef struct {
  VRING                   Ring;
  VOID                    *RingMap;
  DESC_INDICES            Indices;        /* Avail Ring */
  UINT16                  LastUsedIdx;    /* Used Ring */

  UINT32                  BufferSize;
  UINT32                  BufferCount;
  UINT32                  BufferPages;
  UINT8                   *Buffers;
  VOID                    *BufferMap;
  EFI_PHYSICAL_ADDRESS    DeviceAddress;

  BOOLEAN                 Ready;
} VIRTIO_KBD_RING;

// Declaration of data structure representing driver context
typedef struct {
  // Device signature
  UINT32                            Signature;

  // Hook for the function which shall be caled when driver is closed
  // before system state changes to boot
  EFI_EVENT                         ExitBoot;

  // Hooks for functions required by UEFI keyboard API
  // struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
  //    EFI_INPUT_RESET     Reset;
  //    EFI_INPUT_READ_KEY  ReadKeyStroke;
  //    EFI_EVENT           WaitForKey;
  // };
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL    Txt;

  // struct _EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL {
  //    EFI_INPUT_RESET_EX              Reset;
  //    EFI_INPUT_READ_KEY_EX           ReadKeyStrokeEx;
  //    EFI_EVENT                       WaitForKeyEx;
  //    EFI_SET_STATE                   SetState;
  //    EFI_REGISTER_KEYSTROKE_NOTIFY   RegisterKeyNotify;
  //    EFI_UNREGISTER_KEYSTROKE_NOTIFY UnregisterKeyNotify;
  // }
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL    TxtEx;

  // Virtio device hook
  VIRTIO_DEVICE_PROTOCOL               *VirtIo;

  // Hook for ring buffer
  VIRTIO_KBD_RING                      Rings[KEYBOARD_MAX_RINGS];

  // Timer event for checking key presses from VirtIo
  EFI_EVENT                            KeyReadTimer;

  // List for notifications
  LIST_ENTRY                           NotifyList;
  EFI_EVENT                            KeyNotifyTimer;

  // Last pressed key
  // typedef struct {
  //    UINT16  ScanCode;
  //    CHAR16  UnicodeChar;
  // } EFI_INPUT_KEY;
  EFI_INPUT_KEY                        LastKey;

  // Key modifiers
  BOOLEAN                              KeyActive[MAX_KEYBOARD_CODE];

  // If key is ready
  BOOLEAN                              KeyReady;
} VIRTIO_KBD_DEV;

// Helper functions to extract VIRTIO_KBD_DEV structure pointers
#define VIRTIO_KEYBOARD_FROM_THIS(KbrPointer) \
          CR (KbrPointer, VIRTIO_KBD_DEV, Txt, VIRTIO_KBD_SIG)
#define VIRTIO_KEYBOARD_EX_FROM_THIS(KbrPointer) \
          CR (KbrPointer, VIRTIO_KBD_DEV, TxtEx, VIRTIO_KBD_SIG)

// Bellow candidates to be included as Linux header
#define KEY_PRESSED  1

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_PROTOCOL API
EFI_STATUS
EFIAPI
VirtioKeyboardSimpleTextInputReset (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  );

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_PROTOCOL API
EFI_STATUS
EFIAPI
VirtioKeyboardSimpleTextInputReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  OUT EFI_INPUT_KEY                   *Key
  );

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_PROTOCOL API
VOID
EFIAPI
VirtioKeyboardWaitForKey (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  );

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL API
EFI_STATUS
EFIAPI
VirtioKeyboardResetEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  );

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL API
EFI_STATUS
EFIAPI
VirtioKeyboardReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  OUT EFI_KEY_DATA                       *KeyData
  );

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL API
VOID
EFIAPI
VirtioKeyboardWaitForKeyEx (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  );

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL API
EFI_STATUS
EFIAPI
VirtioKeyboardSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  );

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL API
EFI_STATUS
EFIAPI
VirtioKeyboardRegisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_DATA                       *KeyData,
  IN EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT VOID                              **NotifyHandle
  );

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL API
EFI_STATUS
EFIAPI
VirtioKeyboardUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
  );

#endif
