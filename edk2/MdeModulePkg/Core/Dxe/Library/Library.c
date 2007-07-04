/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Library.c

Abstract:

  DXE Core library services.

--*/

#include <DxeMain.h>

UINTN mErrorLevel = EFI_D_ERROR | EFI_D_LOAD;

EFI_DEVICE_HANDLE_EXTENDED_DATA mStatusCodeData = {
  {
    sizeof (EFI_STATUS_CODE_DATA),
    0,
    EFI_STATUS_CODE_SPECIFIC_DATA_GUID
  },
  NULL
};

VOID
CoreReportProgressCodeSpecific (
  IN  EFI_STATUS_CODE_VALUE   Value,
  IN  EFI_HANDLE              Handle
  )
/*++

Routine Description:

  Report status code of type EFI_PROGRESS_CODE by caller ID gEfiDxeServicesTableGuid,
  with a handle as additional information.

Arguments:

  Value    - Describes the class/subclass/operation of the hardware or software entity
             that the Status Code relates to.

  Handle   - Additional information.

Returns:

  None

--*/
{
  mStatusCodeData.DataHeader.Size = sizeof (EFI_DEVICE_HANDLE_EXTENDED_DATA) - sizeof (EFI_STATUS_CODE_DATA);
  mStatusCodeData.Handle          = Handle;

  if ((gStatusCode != NULL) && (gStatusCode->ReportStatusCode != NULL) ) {
    gStatusCode->ReportStatusCode (
      EFI_PROGRESS_CODE,
      Value,
      0,
      &gEfiDxeServicesTableGuid,
      (EFI_STATUS_CODE_DATA *) &mStatusCodeData
      );
  }
}

VOID
CoreReportProgressCode (
  IN  EFI_STATUS_CODE_VALUE   Value
  )
/*++

Routine Description:

  Report status code of type EFI_PROGRESS_CODE by caller ID gEfiDxeServicesTableGuid.

Arguments:

  Value    - Describes the class/subclass/operation of the hardware or software entity
             that the Status Code relates to.

Returns:

  None

--*/
{
  if ((gStatusCode != NULL) && (gStatusCode->ReportStatusCode != NULL) ) {
    gStatusCode->ReportStatusCode (
      EFI_PROGRESS_CODE,
      Value,
      0,
      &gEfiDxeServicesTableGuid,
      NULL
      );
  }
}


VOID *
CoreAllocateBootServicesPool (
  IN  UINTN   AllocationSize
  )
/*++

Routine Description:

  Allocate pool of type EfiBootServicesData, the size is specified with AllocationSize.

Arguments:

  AllocationSize    - Size to allocate.

Returns:

  Pointer of the allocated pool.

--*/
{
  VOID  *Memory;

  CoreAllocatePool (EfiBootServicesData, AllocationSize, &Memory);
  return Memory;
}


VOID *
CoreAllocateZeroBootServicesPool (
  IN  UINTN   AllocationSize
  )
/*++

Routine Description:

  Allocate pool of type EfiBootServicesData and zero it, the size is specified with AllocationSize.

Arguments:

  AllocationSize    - Size to allocate.

Returns:

  Pointer of the allocated pool.

--*/
{
  VOID  *Memory;

  Memory = CoreAllocateBootServicesPool (AllocationSize);
  SetMem (Memory, (Memory == NULL) ? 0 : AllocationSize, 0);
  return Memory;
}


VOID *
CoreAllocateCopyPool (
  IN  UINTN   AllocationSize,
  IN  VOID    *Buffer
  )
/*++

Routine Description:

  Allocate pool of specified size with EfiBootServicesData type, and copy specified buffer to this pool.

Arguments:

  AllocationSize    - Size to allocate.

  Buffer            - Specified buffer that will be copy to the allocated pool

Returns:

  Pointer of the allocated pool.

--*/
{
  VOID  *Memory;

  Memory = CoreAllocateBootServicesPool (AllocationSize);
  CopyMem (Memory, Buffer, (Memory == NULL) ? 0 : AllocationSize);

  return Memory;
}



VOID *
CoreAllocateRuntimePool (
  IN  UINTN   AllocationSize
  )
/*++

Routine Description:

  Allocate pool of type EfiRuntimeServicesData, the size is specified with AllocationSize.

Arguments:

  AllocationSize    - Size to allocate.

Returns:

  Pointer of the allocated pool.

--*/
{
  VOID  *Memory;

  CoreAllocatePool (EfiRuntimeServicesData, AllocationSize, &Memory);
  return Memory;
}

VOID *
CoreAllocateRuntimeCopyPool (
  IN  UINTN   AllocationSize,
  IN  VOID    *Buffer
  )
/*++

Routine Description:

  Allocate pool of specified size with EfiRuntimeServicesData type, and copy specified buffer to this pool.

Arguments:

  AllocationSize    - Size to allocate.

  Buffer            - Specified buffer that will be copy to the allocated pool

Returns:

  Pointer of the allocated pool.

--*/

{
  VOID  *Memory;

  Memory = CoreAllocateRuntimePool (AllocationSize);
  CopyMem (Memory, Buffer, (Memory == NULL) ? 0 : AllocationSize);

  return Memory;
}



//
// Lock Stuff
//



EFI_STATUS
CoreAcquireLockOrFail (
  IN EFI_LOCK  *Lock
  )
/*++

Routine Description:

  Initialize a basic mutual exclusion lock.   Each lock
  provides mutual exclusion access at it's task priority
  level.  Since there is no-premption (at any TPL) or
  multiprocessor support, acquiring the lock only consists
  of raising to the locks TPL.

Arguments:

  Lock        - The EFI_LOCK structure to initialize

Returns:

  EFI_SUCCESS       - Lock Owned.
  EFI_ACCESS_DENIED - Reentrant Lock Acquisition, Lock not Owned.

--*/
{
  ASSERT (Lock != NULL);
  ASSERT (Lock->Lock != EfiLockUninitialized);

  if (Lock->Lock == EfiLockAcquired) {
    //
    // Lock is already owned, so bail out
    //
    return EFI_ACCESS_DENIED;
  }

  Lock->OwnerTpl = CoreRaiseTpl (Lock->Tpl);

  Lock->Lock = EfiLockAcquired;
  return EFI_SUCCESS;
}


VOID
CoreAcquireLock (
  IN EFI_LOCK  *Lock
  )
/*++

Routine Description:

  Raising to the task priority level of the mutual exclusion
  lock, and then acquires ownership of the lock.

Arguments:

  Lock - The lock to acquire

Returns:

  Lock owned

--*/
{
  ASSERT (Lock != NULL);
  ASSERT (Lock->Lock == EfiLockReleased);

  Lock->OwnerTpl = CoreRaiseTpl (Lock->Tpl);
  Lock->Lock     = EfiLockAcquired;
}


VOID
CoreReleaseLock (
  IN EFI_LOCK  *Lock
  )
/*++

Routine Description:

    Releases ownership of the mutual exclusion lock, and
    restores the previous task priority level.

Arguments:

    Lock - The lock to release

Returns:

    Lock unowned

--*/
{
  EFI_TPL Tpl;

  ASSERT (Lock != NULL);
  ASSERT (Lock->Lock == EfiLockAcquired);

  Tpl = Lock->OwnerTpl;

  Lock->Lock = EfiLockReleased;

  CoreRestoreTpl (Tpl);
}


UINTN
CoreDevicePathSize (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
/*++

Routine Description:

  Calculate the size of a whole device path.

Arguments:

  DevicePath - The pointer to the device path data.

Returns:

  Size of device path data structure..

--*/
{
  EFI_DEVICE_PATH_PROTOCOL     *Start;

  if (DevicePath == NULL) {
    return 0;
  }

  //
  // Search for the end of the device path structure
  //
  Start = DevicePath;
  while (!EfiIsDevicePathEnd (DevicePath)) {
    DevicePath = EfiNextDevicePathNode (DevicePath);
  }

  //
  // Compute the size and add back in the size of the end device path structure
  //
  return ((UINTN)DevicePath - (UINTN)Start) + sizeof(EFI_DEVICE_PATH_PROTOCOL);
}


BOOLEAN
CoreIsDevicePathMultiInstance (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
/*++

Routine Description:
  Return TRUE is this is a multi instance device path.

Arguments:
  DevicePath  - A pointer to a device path data structure.


Returns:
  TRUE - If DevicePath is multi instance. FALSE - If DevicePath is not multi
  instance.

--*/
{
  EFI_DEVICE_PATH_PROTOCOL *Node;

  if (DevicePath == NULL) {
    return FALSE;
  }

  Node = DevicePath;
  while (!EfiIsDevicePathEnd (Node)) {
    if (EfiIsDevicePathEndInstance (Node)) {
      return TRUE;
    }
    Node = EfiNextDevicePathNode (Node);
  }
  return FALSE;
}



EFI_DEVICE_PATH_PROTOCOL *
CoreDuplicateDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
/*++

Routine Description:
  Duplicate a new device path data structure from the old one.

Arguments:
  DevicePath  - A pointer to a device path data structure.

Returns:
  A pointer to the new allocated device path data.
  Caller must free the memory used by DevicePath if it is no longer needed.

--*/
{
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  UINTN                     Size;

  if (DevicePath == NULL) {
    return NULL;
  }

  //
  // Compute the size
  //
  Size = CoreDevicePathSize (DevicePath);

  //
  // Allocate space for duplicate device path
  //
  NewDevicePath = CoreAllocateCopyPool (Size, DevicePath);

  return NewDevicePath;
}



EFI_DEVICE_PATH_PROTOCOL *
CoreAppendDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *Src1,
  IN EFI_DEVICE_PATH_PROTOCOL  *Src2
  )
/*++

Routine Description:
  Function is used to append a Src1 and Src2 together.

Arguments:
  Src1  - A pointer to a device path data structure.

  Src2  - A pointer to a device path data structure.

Returns:

  A pointer to the new device path is returned.
  NULL is returned if space for the new device path could not be allocated from pool.
  It is up to the caller to free the memory used by Src1 and Src2 if they are no longer needed.

--*/
{
  UINTN                       Size;
  UINTN                       Size1;
  UINTN                       Size2;
  EFI_DEVICE_PATH_PROTOCOL    *NewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    *SecondDevicePath;

  if (Src1 == NULL && Src2 == NULL) {
    return NULL;
  }

  //
  // Allocate space for the combined device path. It only has one end node of
  // length EFI_DEVICE_PATH_PROTOCOL
  //
  Size1 = CoreDevicePathSize (Src1);
  Size2 = CoreDevicePathSize (Src2);
  Size = Size1 + Size2 - sizeof(EFI_DEVICE_PATH_PROTOCOL);

  NewDevicePath = CoreAllocateCopyPool (Size, Src1);
  if (NewDevicePath != NULL) {

     //
     // Over write Src1 EndNode and do the copy
     //
     SecondDevicePath = (EFI_DEVICE_PATH_PROTOCOL *)((CHAR8 *)NewDevicePath + (Size1 - sizeof(EFI_DEVICE_PATH_PROTOCOL)));
     CopyMem (SecondDevicePath, Src2, Size2);
  }

  return NewDevicePath;
}



EFI_EVENT
CoreCreateProtocolNotifyEvent (
  IN EFI_GUID             *ProtocolGuid,
  IN EFI_TPL              NotifyTpl,
  IN EFI_EVENT_NOTIFY     NotifyFunction,
  IN VOID                 *NotifyContext,
  OUT VOID                **Registration,
  IN  BOOLEAN             SignalFlag
  )
/*++

Routine Description:

  Create a protocol notification event and return it.

Arguments:

  ProtocolGuid    - Protocol to register notification event on.

  NotifyTpl        - Maximum TPL to signal the NotifyFunction.

  NotifyFuncition  - EFI notification routine.

  NotifyContext   - Context passed into Event when it is created.

  Registration    - Registration key returned from RegisterProtocolNotify().

  SignalFlag      -  Boolean value to decide whether kick the event after register or not.

Returns:

  The EFI_EVENT that has been registered to be signaled when a ProtocolGuid
  is added to the system.

--*/
{
  EFI_STATUS              Status;
  EFI_EVENT               Event;

  //
  // Create the event
  //

  Status = CoreCreateEvent (
            EVT_NOTIFY_SIGNAL,
            NotifyTpl,
            NotifyFunction,
            NotifyContext,
           &Event
            );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for protocol notifactions on this event
  //

  Status = CoreRegisterProtocolNotify (
            ProtocolGuid,
            Event,
            Registration
            );
  ASSERT_EFI_ERROR (Status);

  if (SignalFlag) {
    //
    // Kick the event so we will perform an initial pass of
    // current installed drivers
    //
    CoreSignalEvent (Event);
  }

  return Event;
}

