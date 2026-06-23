/** @file

  Private definitions of the VirtioInput driver

  Copyright (C) 2024, Red Hat

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>

#include <IndustryStandard/Virtio.h>

#define VIRTIO_INPUT_SIG  SIGNATURE_32 ('V', 'I', 'N', 'P')

#define MAX_RINGS   2
#define RX_BUFSIZE  64

// Fetch new input from VirtIO every 50ms
#define PROBE_TIME_MS  50

// Max range of recognized keyboard codes
#define MAX_KEYBOARD_CODE  255

typedef struct {
  UINTN                      Signature;
  EFI_KEY_DATA               KeyData;
  EFI_KEY_NOTIFY_FUNCTION    KeyNotificationFn;
  LIST_ENTRY                 NotifyEntry;
} VIRTIO_INPUT_IN_EX_NOTIFY;

// Data structure representing payload delivered from VirtIo
typedef struct {
  UINT16    Type;
  UINT16    Code;
  UINT32    Value;
} VIRTIO_INPUT_EVENT;

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
} VIRTIO_INPUT_RING;

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
  VIRTIO_INPUT_RING                    Rings[MAX_RINGS];

  // Timer event for checking input from VirtIo
  EFI_EVENT                            PollTimer;

  // List for notifications
  LIST_ENTRY                           KeyNotifyList;

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
} VIRTIO_INPUT_DEV;

// Helper functions to extract VIRTIO_INPUT_DEV structure pointers
#define VIRTIO_INPUT_FROM_THIS(KbrPointer) \
          CR (KbrPointer, VIRTIO_INPUT_DEV, Txt, VIRTIO_INPUT_SIG)
#define VIRTIO_INPUT_EX_FROM_THIS(KbrPointer) \
          CR (KbrPointer, VIRTIO_INPUT_DEV, TxtEx, VIRTIO_INPUT_SIG)

// Bellow candidates to be included as Linux header
#define KEY_PRESSED  1
