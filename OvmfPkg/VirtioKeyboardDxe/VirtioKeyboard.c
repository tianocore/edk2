/** @file

  This driver produces EFI_SIMPLE_TEXT_INPUT_PROTOCOL for virtarm devices.

  Copyright (C) 2024, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/VirtioLib.h>

#include <VirtioKeyboard.h>
#include <VirtioKeyCodes.h>

// -----------------------------------------------------------------------------
// Return buffer pointer out of the ring buffer
STATIC
VOID *
BufferPtr (
  IN VIRTIO_KBD_RING  *Ring,
  IN UINT32           BufferNr
  )
{
  return Ring->Buffers + Ring->BufferSize * BufferNr;
}

// -----------------------------------------------------------------------------
// Return buffer physical address out of the ring buffer
STATIC
EFI_PHYSICAL_ADDRESS
BufferAddr (
  IN VIRTIO_KBD_RING  *Ring,
  IN UINT32           BufferNr
  )
{
  return Ring->DeviceAddress + Ring->BufferSize * BufferNr;
}

// Return next buffer from ring
STATIC
UINT32
BufferNext (
  IN VIRTIO_KBD_RING  *Ring
  )
{
  return Ring->Indices.NextDescIdx % Ring->Ring.QueueSize;
}

// -----------------------------------------------------------------------------
// Push the buffer to the device
EFI_STATUS
EFIAPI
VirtioKeyboardRingSendBuffer (
  IN OUT VIRTIO_KBD_DEV  *Dev,
  IN     UINT16          Index,
  IN     VOID            *Data,
  IN     UINT32          DataSize,
  IN     BOOLEAN         Notify
  )
{
  VIRTIO_KBD_RING  *Ring    = Dev->Rings + Index;
  UINT32           BufferNr = BufferNext (Ring);
  UINT16           Idx      = *Ring->Ring.Avail.Idx;
  UINT16           Flags    = 0;

  ASSERT (DataSize <= Ring->BufferSize);

  if (Data) {
    /* driver -> device */
    CopyMem (BufferPtr (Ring, BufferNr), Data, DataSize);
  } else {
    /* device -> driver */
    Flags |= VRING_DESC_F_WRITE;
  }

  VirtioAppendDesc (
    &Ring->Ring,
    BufferAddr (Ring, BufferNr),
    DataSize,
    Flags,
    &Ring->Indices
    );

  Ring->Ring.Avail.Ring[Idx % Ring->Ring.QueueSize] =
    Ring->Indices.HeadDescIdx % Ring->Ring.QueueSize;
  Ring->Indices.HeadDescIdx = Ring->Indices.NextDescIdx;
  Idx++;

  // Force compiler to not optimize this code
  MemoryFence ();
  *Ring->Ring.Avail.Idx = Idx;
  MemoryFence ();

  if (Notify) {
    Dev->VirtIo->SetQueueNotify (Dev->VirtIo, Index);
  }

  return EFI_SUCCESS;
}

// -----------------------------------------------------------------------------
// Look for buffer ready to be processed
BOOLEAN
EFIAPI
VirtioKeyboardRingHasBuffer (
  IN OUT VIRTIO_KBD_DEV  *Dev,
  IN     UINT16          Index
  )
{
  VIRTIO_KBD_RING  *Ring   = Dev->Rings + Index;
  UINT16           UsedIdx = *Ring->Ring.Used.Idx;

  if (!Ring->Ready) {
    return FALSE;
  }

  if (Ring->LastUsedIdx == UsedIdx) {
    return FALSE;
  }

  return TRUE;
}

// -----------------------------------------------------------------------------
// Get data from buffer which is marked as ready from device
BOOLEAN
EFIAPI
VirtioKeyboardRingGetBuffer (
  IN OUT VIRTIO_KBD_DEV  *Dev,
  IN     UINT16          Index,
  OUT    VOID            *Data,
  OUT    UINT32          *DataSize
  )
{
  VIRTIO_KBD_RING           *Ring   = Dev->Rings + Index;
  UINT16                    UsedIdx = *Ring->Ring.Used.Idx;
  volatile VRING_USED_ELEM  *UsedElem;

  if (!Ring->Ready) {
    return FALSE;
  }

  if (Ring->LastUsedIdx == UsedIdx) {
    return FALSE;
  }

  UsedElem = Ring->Ring.Used.UsedElem + (Ring->LastUsedIdx % Ring->Ring.QueueSize);

  if (UsedElem->Len > Ring->BufferSize) {
    DEBUG ((DEBUG_ERROR, "%a:%d: %d: invalid length\n", __func__, __LINE__, Index));
    UsedElem->Len = 0;
  }

  if (Data && DataSize) {
    CopyMem (Data, BufferPtr (Ring, UsedElem->Id), UsedElem->Len);
    *DataSize = UsedElem->Len;
  }

  if (Index % 2 == 0) {
    /* RX - re-queue buffer */
    VirtioKeyboardRingSendBuffer (Dev, Index, NULL, Ring->BufferSize, FALSE);
  }

  Ring->LastUsedIdx++;
  return TRUE;
}

// -----------------------------------------------------------------------------
// Initialize ring buffer
EFI_STATUS
EFIAPI
VirtioKeyboardInitRing (
  IN OUT VIRTIO_KBD_DEV  *Dev,
  IN     UINT16          Index,
  IN     UINT32          BufferSize
  )
{
  VIRTIO_KBD_RING  *Ring = Dev->Rings + Index;
  EFI_STATUS       Status;
  UINT16           QueueSize;
  UINT64           RingBaseShift;

  //
  // step 4b -- allocate request virtqueue
  //
  Status = Dev->VirtIo->SetQueueSel (Dev->VirtIo, Index);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  Status = Dev->VirtIo->GetQueueNumMax (Dev->VirtIo, &QueueSize);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // VirtioKeyboard uses one descriptor
  //
  if (QueueSize < 1) {
    Status = EFI_UNSUPPORTED;
    goto Failed;
  }

  Status = VirtioRingInit (Dev->VirtIo, QueueSize, &Ring->Ring);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // If anything fails from here on, we must release the ring resources.
  //
  Status = VirtioRingMap (
             Dev->VirtIo,
             &Ring->Ring,
             &RingBaseShift,
             &Ring->RingMap
             );
  if (EFI_ERROR (Status)) {
    goto ReleaseQueue;
  }

  //
  // Additional steps for MMIO: align the queue appropriately, and set the
  // size. If anything fails from here on, we must unmap the ring resources.
  //
  Status = Dev->VirtIo->SetQueueNum (Dev->VirtIo, QueueSize);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  Status = Dev->VirtIo->SetQueueAlign (Dev->VirtIo, EFI_PAGE_SIZE);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  //
  // step 4c -- Report GPFN (guest-physical frame number) of queue.
  //
  Status = Dev->VirtIo->SetQueueAddress (
                          Dev->VirtIo,
                          &Ring->Ring,
                          RingBaseShift
                          );
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  Ring->BufferCount = QueueSize;
  Ring->BufferSize  = BufferSize;
  Ring->BufferPages = EFI_SIZE_TO_PAGES (Ring->BufferCount * Ring->BufferSize);

  Status = Dev->VirtIo->AllocateSharedPages (Dev->VirtIo, Ring->BufferPages, (VOID **)&Ring->Buffers);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  Status = VirtioMapAllBytesInSharedBuffer (
             Dev->VirtIo,
             VirtioOperationBusMasterCommonBuffer,
             Ring->Buffers,
             EFI_PAGES_TO_SIZE (Ring->BufferPages),
             &Ring->DeviceAddress,
             &Ring->BufferMap
             );
  if (EFI_ERROR (Status)) {
    goto ReleasePages;
  }

  VirtioPrepare (&Ring->Ring, &Ring->Indices);
  Ring->Ready = TRUE;

  return EFI_SUCCESS;

ReleasePages:
  Dev->VirtIo->FreeSharedPages (
                 Dev->VirtIo,
                 Ring->BufferPages,
                 Ring->Buffers
                 );
  Ring->Buffers = NULL;

UnmapQueue:
  Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, Ring->RingMap);
  Ring->RingMap = NULL;

ReleaseQueue:
  VirtioRingUninit (Dev->VirtIo, &Ring->Ring);

Failed:
  return Status;
}

// -----------------------------------------------------------------------------
// Deinitialize ring buffer
VOID
EFIAPI
VirtioKeyboardUninitRing (
  IN OUT VIRTIO_KBD_DEV  *Dev,
  IN     UINT16          Index
  )
{
  VIRTIO_KBD_RING  *Ring = Dev->Rings + Index;

  if (Ring->BufferMap) {
    Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, Ring->BufferMap);
    Ring->BufferMap = NULL;
  }

  if (Ring->Buffers) {
    Dev->VirtIo->FreeSharedPages (
                   Dev->VirtIo,
                   Ring->BufferPages,
                   Ring->Buffers
                   );
    Ring->Buffers = NULL;
  }

  if (!Ring->RingMap) {
    Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, Ring->RingMap);
    Ring->RingMap = NULL;
  }

  if (Ring->Ring.Base) {
    VirtioRingUninit (Dev->VirtIo, &Ring->Ring);
  }

  ZeroMem (Ring, sizeof (*Ring));
}

// -----------------------------------------------------------------------------
// Deinitialize all rings allocated in driver
STATIC
VOID
EFIAPI
VirtioKeyboardUninitAllRings (
  IN OUT VIRTIO_KBD_DEV  *Dev
  )
{
  UINT16  Index;

  for (Index = 0; Index < KEYBOARD_MAX_RINGS; Index++) {
    VirtioKeyboardUninitRing (Dev, Index);
  }
}

// -----------------------------------------------------------------------------
// Mark all buffers as ready to write and push to device
VOID
EFIAPI
VirtioKeyboardRingFillRx (
  IN OUT VIRTIO_KBD_DEV  *Dev,
  IN     UINT16          Index
  )
{
  VIRTIO_KBD_RING  *Ring = Dev->Rings + Index;
  UINT32           BufferNr;

  for (BufferNr = 0; BufferNr < Ring->BufferCount; BufferNr++) {
    VirtioKeyboardRingSendBuffer (Dev, Index, NULL, Ring->BufferSize, FALSE);
  }

  Dev->VirtIo->SetQueueNotify (Dev->VirtIo, Index);
}

// Forward declaration of module Uninit function
STATIC
VOID
EFIAPI
VirtioKeyboardUninit (
  IN OUT VIRTIO_KBD_DEV  *Dev
  );

// Forward declaration of module Init function
STATIC
EFI_STATUS
EFIAPI
VirtioKeyboardInit (
  IN OUT VIRTIO_KBD_DEV  *Dev
  );

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_PROTOCOL API
EFI_STATUS
EFIAPI
VirtioKeyboardSimpleTextInputReset (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  )
{
  VIRTIO_KBD_DEV  *Dev;

  Dev = VIRTIO_KEYBOARD_FROM_THIS (This);
  VirtioKeyboardUninit (Dev);
  VirtioKeyboardInit (Dev);

  return EFI_SUCCESS;
}

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_PROTOCOL API
EFI_STATUS
EFIAPI
VirtioKeyboardSimpleTextInputReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  OUT EFI_INPUT_KEY                   *Key
  )
{
  VIRTIO_KBD_DEV  *Dev;
  EFI_TPL         OldTpl;

  if (Key == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = VIRTIO_KEYBOARD_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  if (Dev->KeyReady) {
    // Get last key from the buffer
    *Key = Dev->LastKey;

    // Mark key as consumed
    Dev->KeyReady = FALSE;

    gBS->RestoreTPL (OldTpl);
    return EFI_SUCCESS;
  }

  gBS->RestoreTPL (OldTpl);

  return EFI_NOT_READY;
}

// -----------------------------------------------------------------------------
// Function converting VirtIO key codes to UEFI key codes
STATIC
VOID
EFIAPI
VirtioKeyboardConvertKeyCode (
  IN OUT VIRTIO_KBD_DEV  *Dev,
  IN UINT16              Code,
  OUT EFI_INPUT_KEY      *Key
  )
{
  // Key mapping in between Linux and UEFI
  // https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
  // https://dox.ipxe.org/SimpleTextIn_8h_source.html#l00048
  // https://uefi.org/specs/UEFI/2.10/Apx_B_Console.html

  static const UINT16  Map[] = {
    [KEY_1]             = '1',  '2',  '3', '4', '5', '6', '7', '8', '9', '0',
    [KEY_MINUS]         = '-',  '=',
    [KEY_Q]             = 'q',  'w',  'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    [KEY_LEFTBRACE]     = '[',  ']',
    [KEY_A]             = 'a',  's',  'd', 'f', 'g', 'h', 'j', 'k', 'l',
    [KEY_SEMICOLON]     = ';',  '\'', '`',
    [KEY_BACKSLASH]     = '\\',
    [KEY_Z]             = 'z',  'x',  'c', 'v', 'b', 'n', 'm',
    [KEY_COMMA]         = ',',  '.',  '/',
    [KEY_SPACE]         = ' ',
    [MAX_KEYBOARD_CODE] = 0x00
  };

  static const UINT16  MapShift[] = {
    [KEY_1]             = '!', '@',  '#', '$', '%', '^', '&', '*', '(', ')',
    [KEY_MINUS]         = '_', '+',
    [KEY_Q]             = 'Q', 'W',  'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    [KEY_LEFTBRACE]     = '{', '}',
    [KEY_A]             = 'A', 'S',  'D', 'F', 'G', 'H', 'J', 'K', 'L',
    [KEY_SEMICOLON]     = ':', '\"', '~',
    [KEY_BACKSLASH]     = '|',
    [KEY_Z]             = 'Z', 'X',  'C', 'V', 'B', 'N', 'M',
    [KEY_COMMA]         = '<', '>',  '?',
    [KEY_SPACE]         = ' ',
    [MAX_KEYBOARD_CODE] = 0x00
  };

  // Set default readings
  Key->ScanCode    = SCAN_NULL;
  Key->UnicodeChar = CHAR_NULL;

  // Check if key code is not out of the keyboard mapping boundaries
  if (Code >= MAX_KEYBOARD_CODE) {
    DEBUG ((DEBUG_INFO, "%a: Key code out of range \n", __func__));
    return;
  }

  // Handle F1 - F10 keys
  if ((Code >= KEY_F1) && (Code <= KEY_F10)) {
    Key->ScanCode = SCAN_F1 + (Code - KEY_F1);
    return;
  }

  switch (Code) {
    case KEY_PAGEUP:
      Key->ScanCode = SCAN_PAGE_UP;
      break;

    case KEY_PAGEDOWN:
      Key->ScanCode = SCAN_PAGE_DOWN;
      break;

    case KEY_HOME:
      Key->ScanCode = SCAN_HOME;
      break;

    case KEY_END:
      Key->ScanCode = SCAN_END;
      break;

    case KEY_DELETE:
      Key->ScanCode = SCAN_DELETE;
      break;

    case KEY_INSERT:
      Key->ScanCode = SCAN_INSERT;
      break;

    case KEY_UP:
      Key->ScanCode = SCAN_UP;
      break;

    case KEY_LEFT:
      Key->ScanCode = SCAN_LEFT;
      break;

    case KEY_RIGHT:
      Key->ScanCode = SCAN_RIGHT;
      break;

    case KEY_DOWN:
      Key->ScanCode = SCAN_DOWN;
      break;

    case KEY_BACKSPACE:
      Key->UnicodeChar = CHAR_BACKSPACE;
      break;

    case KEY_TAB:
      Key->UnicodeChar = CHAR_TAB;
      break;

    case KEY_ENTER:
      // Key->UnicodeChar = CHAR_LINEFEED;
      Key->UnicodeChar = CHAR_CARRIAGE_RETURN;
      break;

    case KEY_ESC:
      Key->ScanCode = SCAN_ESC;
      break;

    default:
      if (Dev->KeyActive[KEY_LEFTSHIFT] || Dev->KeyActive[KEY_RIGHTSHIFT]) {
        Key->ScanCode    = MapShift[Code];
        Key->UnicodeChar = MapShift[Code];
      } else {
        Key->ScanCode    = Map[Code];
        Key->UnicodeChar = Map[Code];
      }

      if (Dev->KeyActive[KEY_LEFTCTRL] || Dev->KeyActive[KEY_RIGHTCTRL]) {
        // Convert Ctrl+[a-z] and Ctrl+[A-Z] into [1-26] ASCII table entries
        Key->UnicodeChar &= 0x1F;
      }

      break;
  }
}

// -----------------------------------------------------------------------------
// Main function processing virtio keyboard events
STATIC
VOID
EFIAPI
VirtioKeyboardGetDeviceData (
  IN OUT VIRTIO_KBD_DEV  *Dev
  )
{
  BOOLEAN           HasData;
  UINT8             Data[KEYBOARD_RX_BUFSIZE + 1];
  UINT32            DataSize;
  VIRTIO_KBD_EVENT  Event;
  EFI_TPL           OldTpl;

  for ( ; ; ) {
    HasData = VirtioKeyboardRingGetBuffer (Dev, 0, Data, &DataSize);

    // Exit if no new data
    if (!HasData) {
      return;
    }

    if (DataSize < sizeof (Event)) {
      continue;
    }

    // Clearing last character is not needed as it will be overwritten anyway
    // Dev->LastKey.ScanCode = SCAN_NULL;
    // Dev->LastKey.UnicodeChar = CHAR_NULL;

    CopyMem (&Event, Data, sizeof (Event));

    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    switch (Event.Type) {
      case EV_SYN:
        // Sync event received
        break;

      case EV_KEY:
        // Key press event received
        // DEBUG ((DEBUG_INFO, "%a: ---------------------- \nType: %x Code: %x Value: %x\n",
        //              __func__, Event.Type, Event.Code, Event.Value));

        if (Event.Value == KEY_PRESSED) {
          // Key pressed event received
          Dev->KeyActive[(UINT8)Event.Code] = TRUE;

          // Evaluate key
          VirtioKeyboardConvertKeyCode (Dev, Event.Code, &Dev->LastKey);

          // Flag that printable character is ready to be send
          Dev->KeyReady = TRUE;
        } else {
          // Key released event received
          Dev->KeyActive[(UINT8)Event.Code] = FALSE;
        }

        break;

      default:
        DEBUG ((DEBUG_INFO, "%a: Unhandled VirtIo event\n", __func__));
        break;
    }

    gBS->RestoreTPL (OldTpl);
  }
}

// -----------------------------------------------------------------------------
// Callback hook for timer interrupt
STATIC
VOID
EFIAPI
VirtioKeyboardTimer (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  VIRTIO_KBD_DEV  *Dev = Context;

  VirtioKeyboardGetDeviceData (Dev);
}

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_PROTOCOL API
VOID
EFIAPI
VirtioKeyboardWaitForKey (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  VIRTIO_KBD_DEV  *Dev = VIRTIO_KEYBOARD_FROM_THIS (Context);

  //
  // Stall 1ms to give a chance to let other driver interrupt this routine
  // for their timer event.
  // e.g. UI setup or Shell, other drivers which are driven by timer event
  // will have a bad performance during this period,
  // e.g. usb keyboard driver.
  // Add a stall period can greatly increate other driver performance during
  // the WaitForKey is recursivly invoked. 1ms delay will make little impact
  // to the thunk keyboard driver, and user can not feel the delay at all when
  // input.
  gBS->Stall (1000);

  // Use TimerEvent callback function to check whether there's any key pressed
  VirtioKeyboardTimer (NULL, Dev);

  // If there is a new key ready - send signal
  if (Dev->KeyReady) {
    gBS->SignalEvent (Event);
  }
}

/// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL API
EFI_STATUS
EFIAPI
VirtioKeyboardResetEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  )
{
  VIRTIO_KBD_DEV  *Dev;
  EFI_STATUS      Status;
  EFI_TPL         OldTpl;

  Dev = VIRTIO_KEYBOARD_EX_FROM_THIS (This);

  // Call the reset function from SIMPLE_TEXT_INPUT protocol
  Status = Dev->Txt.Reset (
                      &Dev->Txt,
                      ExtendedVerification
                      );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL API
EFI_STATUS
EFIAPI
VirtioKeyboardReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  OUT EFI_KEY_DATA                       *KeyData
  )
{
  VIRTIO_KBD_DEV  *Dev;
  EFI_STATUS      Status;
  EFI_INPUT_KEY   Key;
  EFI_KEY_STATE   KeyState;

  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = VIRTIO_KEYBOARD_EX_FROM_THIS (This);

  // Get the last pressed key
  Status = Dev->Txt.ReadKeyStroke (&Dev->Txt, &Key);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  // Add key state informations
  KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID;
  KeyState.KeyToggleState = EFI_TOGGLE_STATE_VALID;

  // Shift key modifier
  if (Dev->KeyActive[KEY_LEFTSHIFT]) {
    KeyState.KeyShiftState |= EFI_LEFT_SHIFT_PRESSED;
  }

  if (Dev->KeyActive[KEY_RIGHTSHIFT]) {
    KeyState.KeyShiftState |= EFI_RIGHT_SHIFT_PRESSED;
  }

  // Ctrl key modifier
  if (Dev->KeyActive[KEY_LEFTCTRL]) {
    KeyState.KeyShiftState |= EFI_LEFT_CONTROL_PRESSED;
  }

  if (Dev->KeyActive[KEY_RIGHTCTRL]) {
    KeyState.KeyShiftState |= EFI_RIGHT_CONTROL_PRESSED;
  }

  // ALt key modifier
  if (Dev->KeyActive[KEY_LEFTALT]) {
    KeyState.KeyShiftState |= EFI_LEFT_ALT_PRESSED;
  }

  if (Dev->KeyActive[KEY_RIGHTALT]) {
    KeyState.KeyShiftState |= EFI_RIGHT_ALT_PRESSED;
  }

  // Return value only when there is no failure
  KeyData->Key      = Key;
  KeyData->KeyState = KeyState;

  return EFI_SUCCESS;
}

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL API
VOID
EFIAPI
VirtioKeyboardWaitForKeyEx (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  VIRTIO_KBD_DEV  *Dev;

  Dev = VIRTIO_KEYBOARD_EX_FROM_THIS (Context);
  VirtioKeyboardWaitForKey (Event, &Dev->Txt);
}

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL API
EFI_STATUS
EFIAPI
VirtioKeyboardSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  )
{
  if (KeyToggleState == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

BOOLEAN
IsKeyRegistered (
  IN EFI_KEY_DATA  *RegsiteredData,
  IN EFI_KEY_DATA  *InputData
  )

{
  ASSERT (RegsiteredData != NULL && InputData != NULL);

  if ((RegsiteredData->Key.ScanCode    != InputData->Key.ScanCode) ||
      (RegsiteredData->Key.UnicodeChar != InputData->Key.UnicodeChar))
  {
    return FALSE;
  }

  //
  // Assume KeyShiftState/KeyToggleState = 0 in Registered key data means
  // these state could be ignored.
  //
  if ((RegsiteredData->KeyState.KeyShiftState != 0) &&
      (RegsiteredData->KeyState.KeyShiftState != InputData->KeyState.KeyShiftState))
  {
    return FALSE;
  }

  if ((RegsiteredData->KeyState.KeyToggleState != 0) &&
      (RegsiteredData->KeyState.KeyToggleState != InputData->KeyState.KeyToggleState))
  {
    return FALSE;
  }

  return TRUE;
}

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL API
EFI_STATUS
EFIAPI
VirtioKeyboardRegisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_DATA                       *KeyData,
  IN EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT VOID                              **NotifyHandle
  )
{
  EFI_STATUS               Status;
  VIRTIO_KBD_DEV           *Dev;
  EFI_TPL                  OldTpl;
  LIST_ENTRY               *Link;
  VIRTIO_KBD_IN_EX_NOTIFY  *NewNotify;
  VIRTIO_KBD_IN_EX_NOTIFY  *CurrentNotify;

  if ((KeyData == NULL) ||
      (NotifyHandle == NULL) ||
      (KeyNotificationFunction == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Dev = VIRTIO_KEYBOARD_EX_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  // Check if the (KeyData, NotificationFunction) pair is already registered.
  for (Link = Dev->NotifyList.ForwardLink;
       Link != &Dev->NotifyList;
       Link = Link->ForwardLink)
  {
    CurrentNotify = CR (
                      Link,
                      VIRTIO_KBD_IN_EX_NOTIFY,
                      NotifyEntry,
                      VIRTIO_KBD_SIG
                      );
    if (IsKeyRegistered (&CurrentNotify->KeyData, KeyData)) {
      if (CurrentNotify->KeyNotificationFn == KeyNotificationFunction) {
        *NotifyHandle = CurrentNotify;
        Status        = EFI_SUCCESS;
        goto Exit;
      }
    }
  }

  NewNotify = (VIRTIO_KBD_IN_EX_NOTIFY *)AllocateZeroPool (sizeof (VIRTIO_KBD_IN_EX_NOTIFY));
  if (NewNotify == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  NewNotify->Signature         = VIRTIO_KBD_SIG;
  NewNotify->KeyNotificationFn = KeyNotificationFunction;
  CopyMem (&NewNotify->KeyData, KeyData, sizeof (EFI_KEY_DATA));
  InsertTailList (&Dev->NotifyList, &NewNotify->NotifyEntry);

  *NotifyHandle = NewNotify;
  Status        = EFI_SUCCESS;

Exit:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL API
EFI_STATUS
EFIAPI
VirtioKeyboardUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
  )
{
  EFI_STATUS               Status;
  VIRTIO_KBD_DEV           *Dev;
  EFI_TPL                  OldTpl;
  LIST_ENTRY               *Link;
  VIRTIO_KBD_IN_EX_NOTIFY  *CurrentNotify;

  if (NotificationHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (((VIRTIO_KBD_IN_EX_NOTIFY *)NotificationHandle)->Signature != VIRTIO_KBD_SIG) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = VIRTIO_KEYBOARD_EX_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  for (Link = Dev->NotifyList.ForwardLink;
       Link != &Dev->NotifyList;
       Link = Link->ForwardLink)
  {
    CurrentNotify = CR (
                      Link,
                      VIRTIO_KBD_IN_EX_NOTIFY,
                      NotifyEntry,
                      VIRTIO_KBD_SIG
                      );
    if (CurrentNotify == NotificationHandle) {
      RemoveEntryList (&CurrentNotify->NotifyEntry);

      Status = EFI_SUCCESS;
      goto Exit;
    }
  }

  // Notification has not been found
  Status = EFI_INVALID_PARAMETER;

Exit:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

// -----------------------------------------------------------------------------
// Driver init
STATIC
EFI_STATUS
EFIAPI
VirtioKeyboardInit (
  IN OUT VIRTIO_KBD_DEV  *Dev
  )
{
  UINT8       NextDevStat;
  EFI_STATUS  Status;
  UINT64      Features;

  //
  // Execute virtio-0.9.5, 2.2.1 Device Initialization Sequence.
  //
  NextDevStat = 0;             // step 1 -- reset device
  Status      = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  NextDevStat |= VSTAT_ACK;    // step 2 -- acknowledge device presence
  Status       = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  NextDevStat |= VSTAT_DRIVER; // step 3 -- we know how to drive it
  Status       = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // Set Page Size - MMIO VirtIo Specific
  //
  Status = Dev->VirtIo->SetPageSize (Dev->VirtIo, EFI_PAGE_SIZE);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // step 4a -- retrieve and validate features
  //
  Status = Dev->VirtIo->GetDeviceFeatures (Dev->VirtIo, &Features);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  Features &= VIRTIO_F_VERSION_1 | VIRTIO_F_IOMMU_PLATFORM;

  //
  // In virtio-1.0, feature negotiation is expected to complete before queue
  // discovery, and the device can also reject the selected set of features.
  //
  if (Dev->VirtIo->Revision >= VIRTIO_SPEC_REVISION (1, 0, 0)) {
    Status = Virtio10WriteFeatures (Dev->VirtIo, Features, &NextDevStat);
    if (EFI_ERROR (Status)) {
      goto Failed;
    }
  }

  Status = VirtioKeyboardInitRing (Dev, 0, KEYBOARD_RX_BUFSIZE);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // step 5 -- Report understood features and guest-tuneables.
  //
  if (Dev->VirtIo->Revision < VIRTIO_SPEC_REVISION (1, 0, 0)) {
    Features &= ~(UINT64)(VIRTIO_F_VERSION_1 | VIRTIO_F_IOMMU_PLATFORM);
    Status    = Dev->VirtIo->SetGuestFeatures (Dev->VirtIo, Features);
    if (EFI_ERROR (Status)) {
      goto Failed;
    }
  }

  //
  // step 6 -- initialization complete
  //
  NextDevStat |= VSTAT_DRIVER_OK;
  Status       = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // populate the exported interface's attributes
  //

  // struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
  //    EFI_INPUT_RESET     Reset;
  //    EFI_INPUT_READ_KEY  ReadKeyStroke;
  //    EFI_EVENT           WaitForKey;
  // };
  Dev->Txt.Reset         = (EFI_INPUT_RESET)VirtioKeyboardSimpleTextInputReset;
  Dev->Txt.ReadKeyStroke = VirtioKeyboardSimpleTextInputReadKeyStroke;
  Dev->Txt.WaitForKey    = (EFI_EVENT)VirtioKeyboardWaitForKey;

  // struct _EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL {
  //    EFI_INPUT_RESET_EX              Reset;
  //    EFI_INPUT_READ_KEY_EX           ReadKeyStrokeEx;
  //    EFI_EVENT                       WaitForKeyEx;
  //    EFI_SET_STATE                   SetState;
  //    EFI_REGISTER_KEYSTROKE_NOTIFY   RegisterKeyNotify;
  //    EFI_UNREGISTER_KEYSTROKE_NOTIFY UnregisterKeyNotify;
  // }
  Dev->TxtEx.Reset               = (EFI_INPUT_RESET_EX)VirtioKeyboardResetEx;
  Dev->TxtEx.ReadKeyStrokeEx     = VirtioKeyboardReadKeyStrokeEx;
  Dev->TxtEx.SetState            = VirtioKeyboardSetState;
  Dev->TxtEx.RegisterKeyNotify   = VirtioKeyboardRegisterKeyNotify;
  Dev->TxtEx.UnregisterKeyNotify = VirtioKeyboardUnregisterKeyNotify;
  InitializeListHead (&Dev->NotifyList);

  //
  // Setup the WaitForKey event
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  VirtioKeyboardWaitForKey,
                  &(Dev->Txt),
                  &((Dev->Txt).WaitForKey)
                  );
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // Setup the WaitForKeyEx event
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  VirtioKeyboardWaitForKeyEx,
                  &(Dev->TxtEx),
                  &((Dev->TxtEx).WaitForKeyEx)
                  );
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  VirtioKeyboardRingFillRx (Dev, 0);

  //
  // Event for reading key in time intervals
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  VirtioKeyboardTimer,
                  Dev,
                  &Dev->KeyReadTimer
                  );
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  Status = gBS->SetTimer (
                  Dev->KeyReadTimer,
                  TimerPeriodic,
                  EFI_TIMER_PERIOD_MILLISECONDS (KEYBOARD_PROBE_TIME_MS)
                  );
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  return EFI_SUCCESS;

Failed:
  VirtioKeyboardUninitAllRings (Dev);
  // VirtualKeyboardFreeNotifyList (&VirtualKeyboardPrivate->NotifyList);

  //
  // Notify the host about our failure to setup: virtio-0.9.5, 2.2.2.1 Device
  // Status. VirtIo access failure here should not mask the original error.
  //
  NextDevStat |= VSTAT_FAILED;
  Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);

  return Status; // reached only via Failed above
}

// -----------------------------------------------------------------------------
// Deinitialize driver
STATIC
VOID
EFIAPI
VirtioKeyboardUninit (
  IN OUT VIRTIO_KBD_DEV  *Dev
  )
{
  gBS->CloseEvent (Dev->KeyReadTimer);
  //
  // Reset the virtual device -- see virtio-0.9.5, 2.2.2.1 Device Status. When
  // VIRTIO_CFG_WRITE() returns, the host will have learned to stay away from
  // the old comms area.
  //
  Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, 0);

  VirtioKeyboardUninitAllRings (Dev);
}

// -----------------------------------------------------------------------------
// Handle device exit before switch to boot
STATIC
VOID
EFIAPI
VirtioKeyboardExitBoot (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  VIRTIO_KBD_DEV  *Dev;

  DEBUG ((DEBUG_INFO, "%a: Context=0x%p\n", __func__, Context));
  //
  // Reset the device. This causes the hypervisor to forget about the virtio
  // ring.
  //
  // We allocated said ring in EfiBootServicesData type memory, and code
  // executing after ExitBootServices() is permitted to overwrite it.
  //
  Dev = Context;
  Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, 0);
}

// -----------------------------------------------------------------------------
// Binding validation function
STATIC
EFI_STATUS
EFIAPI
VirtioKeyboardBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS              Status;
  VIRTIO_DEVICE_PROTOCOL  *VirtIo;

  //
  // Attempt to open the device with the VirtIo set of interfaces. On success,
  // the protocol is "instantiated" for the VirtIo device. Covers duplicate
  // open attempts (EFI_ALREADY_STARTED).
  //
  Status = gBS->OpenProtocol (
                  DeviceHandle,               // candidate device
                  &gVirtioDeviceProtocolGuid, // for generic VirtIo access
                  (VOID **)&VirtIo,           // handle to instantiate
                  This->DriverBindingHandle,  // requestor driver identity
                  DeviceHandle,               // ControllerHandle, according to
                                              // the UEFI Driver Model
                  EFI_OPEN_PROTOCOL_BY_DRIVER // get exclusive VirtIo access to
                                              // the device; to be released
                  );
  if (EFI_ERROR (Status)) {
    if (Status != EFI_UNSUPPORTED) {
      DEBUG ((DEBUG_INFO, "%a:%d: %r\n", __func__, __LINE__, Status));
    }

    return Status;
  }

  DEBUG ((DEBUG_INFO, "%a:%d: 0x%x\n", __func__, __LINE__, VirtIo->SubSystemDeviceId));
  if (VirtIo->SubSystemDeviceId != VIRTIO_SUBSYSTEM_INPUT) {
    Status = EFI_UNSUPPORTED;
  }

  //
  // We needed VirtIo access only transitorily, to see whether we support the
  // device or not.
  //
  gBS->CloseProtocol (
         DeviceHandle,
         &gVirtioDeviceProtocolGuid,
         This->DriverBindingHandle,
         DeviceHandle
         );
  return Status;
}

// -----------------------------------------------------------------------------
// Driver binding function API
STATIC
EFI_STATUS
EFIAPI
VirtioKeyboardBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  VIRTIO_KBD_DEV  *Dev;
  EFI_STATUS      Status;

  Dev = (VIRTIO_KBD_DEV *)AllocateZeroPool (sizeof *Dev);
  if (Dev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->OpenProtocol (
                  DeviceHandle,
                  &gVirtioDeviceProtocolGuid,
                  (VOID **)&Dev->VirtIo,
                  This->DriverBindingHandle,
                  DeviceHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto FreeVirtioKbd;
  }

  //
  // VirtIo access granted, configure virtio keyboard device.
  //
  Status = VirtioKeyboardInit (Dev);
  if (EFI_ERROR (Status)) {
    goto CloseVirtIo;
  }

  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_CALLBACK,
                  &VirtioKeyboardExitBoot,
                  Dev,
                  &Dev->ExitBoot
                  );
  if (EFI_ERROR (Status)) {
    goto UninitDev;
  }

  //
  // Setup complete, attempt to export the driver instance's EFI_SIMPLE_TEXT_INPUT_PROTOCOL
  // interface.
  //
  Dev->Signature = VIRTIO_KBD_SIG;
  Status         = gBS->InstallMultipleProtocolInterfaces (
                          &DeviceHandle,
                          &gEfiSimpleTextInProtocolGuid,
                          &Dev->Txt,
                          &gEfiSimpleTextInputExProtocolGuid,
                          &Dev->TxtEx,
                          NULL
                          );
  if (EFI_ERROR (Status)) {
    goto CloseExitBoot;
  }

  return EFI_SUCCESS;

CloseExitBoot:
  gBS->CloseEvent (Dev->ExitBoot);

UninitDev:
  VirtioKeyboardUninit (Dev);

CloseVirtIo:
  gBS->CloseProtocol (
         DeviceHandle,
         &gVirtioDeviceProtocolGuid,
         This->DriverBindingHandle,
         DeviceHandle
         );

FreeVirtioKbd:
  FreePool (Dev);

  return Status;
}

// -----------------------------------------------------------------------------
// Driver unbinding function API
STATIC
EFI_STATUS
EFIAPI
VirtioKeyboardBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                      Status;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *Txt;
  VIRTIO_KBD_DEV                  *Dev;

  Status = gBS->OpenProtocol (
                  DeviceHandle,                     // candidate device
                  &gEfiSimpleTextInProtocolGuid,    // retrieve the RNG iface
                  (VOID **)&Txt,                    // target pointer
                  This->DriverBindingHandle,        // requestor driver ident.
                  DeviceHandle,                     // lookup req. for dev.
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL    // lookup only, no new ref.
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Dev = VIRTIO_KEYBOARD_FROM_THIS (Txt);

  //
  // Handle Stop() requests for in-use driver instances gracefully.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  &DeviceHandle,
                  &gEfiSimpleTextInProtocolGuid,
                  &Dev->Txt,
                  &gEfiSimpleTextInputExProtocolGuid,
                  &Dev->TxtEx,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseEvent (Dev->ExitBoot);

  VirtioKeyboardUninit (Dev);

  gBS->CloseProtocol (
         DeviceHandle,
         &gVirtioDeviceProtocolGuid,
         This->DriverBindingHandle,
         DeviceHandle
         );

  FreePool (Dev);

  return EFI_SUCCESS;
}

// -----------------------------------------------------------------------------
// Forward declaration of global variable
STATIC
EFI_COMPONENT_NAME_PROTOCOL  gComponentName;

// -----------------------------------------------------------------------------
// Driver name to be displayed
STATIC
EFI_UNICODE_STRING_TABLE  mDriverNameTable[] = {
  { "eng;en", L"Virtio Keyboard Driver" },
  { NULL,     NULL                      }
};

// -----------------------------------------------------------------------------
// Driver name lookup function
STATIC
EFI_STATUS
EFIAPI
VirtioKeyboardGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mDriverNameTable,
           DriverName,
           (BOOLEAN)(This == &gComponentName) // Iso639Language
           );
}

// -----------------------------------------------------------------------------
// Device name to be displayed
STATIC
EFI_UNICODE_STRING_TABLE  mDeviceNameTable[] = {
  { "eng;en", L"RHEL virtio virtual keyboard BOB (Basic Operation Board)" },
  { NULL,     NULL                                                        }
};

// -----------------------------------------------------------------------------
STATIC
EFI_COMPONENT_NAME_PROTOCOL  gDeviceName;

// -----------------------------------------------------------------------------
STATIC
EFI_STATUS
EFIAPI
VirtioKeyboardGetDeviceName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   DeviceHandle,
  IN  EFI_HANDLE                   ChildHandle,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mDeviceNameTable,
           ControllerName,
           (BOOLEAN)(This == &gDeviceName) // Iso639Language
           );
}

// -----------------------------------------------------------------------------
// General driver UEFI interface for showing driver name
STATIC
EFI_COMPONENT_NAME_PROTOCOL  gComponentName = {
  &VirtioKeyboardGetDriverName,
  &VirtioKeyboardGetDeviceName,
  "eng" // SupportedLanguages, ISO 639-2 language codes
};

// -----------------------------------------------------------------------------
// General driver UEFI interface for showing driver name
STATIC
EFI_COMPONENT_NAME2_PROTOCOL  gComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)&VirtioKeyboardGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME)&VirtioKeyboardGetDeviceName,
  "en" // SupportedLanguages, RFC 4646 language codes
};

// -----------------------------------------------------------------------------
// General driver UEFI interface for loading / unloading driver
STATIC EFI_DRIVER_BINDING_PROTOCOL  gDriverBinding = {
  &VirtioKeyboardBindingSupported,
  &VirtioKeyboardBindingStart,
  &VirtioKeyboardBindingStop,
  0x10, // Version, must be in [0x10 .. 0xFFFFFFEF] for IHV-developed drivers
  NULL, // ImageHandle, to be overwritten by
        // EfiLibInstallDriverBindingComponentName2() in VirtioKeyboardEntryPoint()
  NULL  // DriverBindingHandle, ditto
};

// -----------------------------------------------------------------------------
// Driver entry point set in INF file, registers all driver functions into UEFI
EFI_STATUS
EFIAPI
VirtioKeyboardEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  DEBUG ((DEBUG_INFO, "Virtio keyboard has been loaded.......................\n"));
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gDriverBinding,
           ImageHandle,
           &gComponentName,
           &gComponentName2
           );
}
