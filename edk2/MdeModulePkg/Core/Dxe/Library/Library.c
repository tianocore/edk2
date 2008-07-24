/** @file
  DXE Core library services.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <DxeMain.h>

UINTN mErrorLevel = DEBUG_ERROR | DEBUG_LOAD;

EFI_DXE_DEVICE_HANDLE_EXTENDED_DATA mStatusCodeData = {
  {
    sizeof (EFI_STATUS_CODE_DATA),
    0,
    EFI_STATUS_CODE_DXE_CORE_GUID
  },
  NULL
};


/**
  Report status code of type EFI_PROGRESS_CODE by caller ID gEfiCallerIdGuid,
  with a handle as additional information.

  @param  Value              Describes the class/subclass/operation of the
                             hardware or software entity that the Status Code
                             relates to.
  @param  Handle             Additional information.

**/
VOID
CoreReportProgressCodeSpecific (
  IN  EFI_STATUS_CODE_VALUE   Value,
  IN  EFI_HANDLE              Handle
  )
{
  mStatusCodeData.DataHeader.Size = sizeof (EFI_DXE_DEVICE_HANDLE_EXTENDED_DATA) - sizeof (EFI_STATUS_CODE_DATA);
  mStatusCodeData.Handle          = Handle;

  if ((gStatusCode != NULL) && (gStatusCode->ReportStatusCode != NULL) ) {
    gStatusCode->ReportStatusCode (
      EFI_PROGRESS_CODE,
      Value,
      0,
      &gEfiCallerIdGuid,
      (EFI_STATUS_CODE_DATA *) &mStatusCodeData
      );
  }
}


/**
  Report status code of type EFI_PROGRESS_CODE by caller ID gEfiCallerIdGuid.

  @param  Value              Describes the class/subclass/operation of the
                             hardware or software entity that the Status Code
                             relates to.

**/
VOID
CoreReportProgressCode (
  IN  EFI_STATUS_CODE_VALUE   Value
  )
{
  if ((gStatusCode != NULL) && (gStatusCode->ReportStatusCode != NULL) ) {
    gStatusCode->ReportStatusCode (
      EFI_PROGRESS_CODE,
      Value,
      0,
      &gEfiCallerIdGuid,
      NULL
      );
  }
}



/**
  Allocate pool of type EfiBootServicesData, the size is specified with AllocationSize.

  @param  AllocationSize     Size to allocate.

  @return Pointer of the allocated pool.

**/
VOID *
CoreAllocateBootServicesPool (
  IN  UINTN   AllocationSize
  )
{
  VOID  *Memory;

  CoreAllocatePool (EfiBootServicesData, AllocationSize, &Memory);
  return Memory;
}



/**
  Allocate pool of type EfiBootServicesData and zero it, the size is specified with AllocationSize.

  @param  AllocationSize     Size to allocate.

  @return Pointer of the allocated pool.

**/
VOID *
CoreAllocateZeroBootServicesPool (
  IN  UINTN   AllocationSize
  )
{
  VOID  *Memory;

  Memory = CoreAllocateBootServicesPool (AllocationSize);
  ZeroMem (Memory, (Memory == NULL) ? 0 : AllocationSize);
  return Memory;
}



/**
  Allocate pool of specified size with EfiBootServicesData type, and copy specified buffer to this pool.

  @param  AllocationSize     Size to allocate.
  @param  Buffer             Specified buffer that will be copy to the allocated
                             pool

  @return Pointer of the allocated pool.

**/
VOID *
CoreAllocateCopyPool (
  IN  UINTN   AllocationSize,
  IN  VOID    *Buffer
  )
{
  VOID  *Memory;

  Memory = CoreAllocateBootServicesPool (AllocationSize);
  CopyMem (Memory, Buffer, (Memory == NULL) ? 0 : AllocationSize);

  return Memory;
}




/**
  Allocate pool of type EfiRuntimeServicesData, the size is specified with AllocationSize.

  @param  AllocationSize     Size to allocate.

  @return Pointer of the allocated pool.

**/
VOID *
CoreAllocateRuntimePool (
  IN  UINTN   AllocationSize
  )
{
  VOID  *Memory;

  CoreAllocatePool (EfiRuntimeServicesData, AllocationSize, &Memory);
  return Memory;
}


/**
  Allocate pool of specified size with EfiRuntimeServicesData type, and copy specified buffer to this pool.

  @param  AllocationSize     Size to allocate.
  @param  Buffer             Specified buffer that will be copy to the allocated
                             pool

  @return Pointer of the allocated pool.

**/
VOID *
CoreAllocateRuntimeCopyPool (
  IN  UINTN   AllocationSize,
  IN  VOID    *Buffer
  )
{
  VOID  *Memory;

  Memory = CoreAllocateRuntimePool (AllocationSize);
  CopyMem (Memory, Buffer, (Memory == NULL) ? 0 : AllocationSize);

  return Memory;
}



//
// Lock Stuff
//


/**
  Initialize a basic mutual exclusion lock.   Each lock
  provides mutual exclusion access at it's task priority
  level.  Since there is no-premption (at any TPL) or
  multiprocessor support, acquiring the lock only consists
  of raising to the locks TPL.

  @param  Lock               The EFI_LOCK structure to initialize

  @retval EFI_SUCCESS        Lock Owned.
  @retval EFI_ACCESS_DENIED  Reentrant Lock Acquisition, Lock not Owned.

**/
EFI_STATUS
CoreAcquireLockOrFail (
  IN EFI_LOCK  *Lock
  )
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



/**
  Raising to the task priority level of the mutual exclusion
  lock, and then acquires ownership of the lock.

  @param  Lock               The lock to acquire

  @return Lock owned

**/
VOID
CoreAcquireLock (
  IN EFI_LOCK  *Lock
  )
{
  ASSERT (Lock != NULL);
  ASSERT (Lock->Lock == EfiLockReleased);

  Lock->OwnerTpl = CoreRaiseTpl (Lock->Tpl);
  Lock->Lock     = EfiLockAcquired;
}



/**
  Releases ownership of the mutual exclusion lock, and
  restores the previous task priority level.

  @param  Lock               The lock to release

  @return Lock unowned

**/
VOID
CoreReleaseLock (
  IN EFI_LOCK  *Lock
  )
{
  EFI_TPL Tpl;

  ASSERT (Lock != NULL);
  ASSERT (Lock->Lock == EfiLockAcquired);

  Tpl = Lock->OwnerTpl;

  Lock->Lock = EfiLockReleased;

  CoreRestoreTpl (Tpl);
}



/**
  Calculate the size of a whole device path.

  @param  DevicePath         The pointer to the device path data.

  @return Size of device path data structure..

**/
UINTN
CoreDevicePathSize (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
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
  return ((UINTN) DevicePath - (UINTN) Start) + sizeof(EFI_DEVICE_PATH_PROTOCOL);
}



/**
  Return TRUE is this is a multi instance device path.

  @param  DevicePath         A pointer to a device path data structure.

  @retval TRUE               If DevicePath is multi instance. FALSE - If
                             DevicePath is not multi instance.

**/
BOOLEAN
CoreIsDevicePathMultiInstance (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
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




/**
  Duplicate a new device path data structure from the old one.

  @param  DevicePath         A pointer to a device path data structure.

  @return A pointer to the new allocated device path data.
  @return Caller must free the memory used by DevicePath if it is no longer needed.

**/
EFI_DEVICE_PATH_PROTOCOL *
CoreDuplicateDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
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


/**
  Function is used to append a Src1 and Src2 together.

  @param  Src1               A pointer to a device path data structure.
  @param  Src2               A pointer to a device path data structure.

  @return A pointer to the new device path is returned.
  @return NULL is returned if space for the new device path could not be allocated from pool.
  @return It is up to the caller to free the memory used by Src1 and Src2 if they are no longer needed.

**/
EFI_DEVICE_PATH_PROTOCOL *
CoreAppendDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *Src1,
  IN EFI_DEVICE_PATH_PROTOCOL  *Src2
  )
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


/**
  Create a protocol notification event and return it.

  @param  ProtocolGuid       Protocol to register notification event on.
  @param  NotifyTpl          Maximum TPL to signal the NotifyFunction.
  @param  NotifyFunction     EFI notification routine.
  @param  NotifyContext      Context passed into Event when it is created.
  @param  Registration       Registration key returned from
                             RegisterProtocolNotify().
  @param  SignalFlag         Boolean value to decide whether kick the event after
                             register or not.

  @return The EFI_EVENT that has been registered to be signaled when a ProtocolGuid
          is added to the system.

**/
EFI_EVENT
CoreCreateProtocolNotifyEvent (
  IN EFI_GUID             *ProtocolGuid,
  IN EFI_TPL              NotifyTpl,
  IN EFI_EVENT_NOTIFY     NotifyFunction,
  IN VOID                 *NotifyContext,
  OUT VOID                **Registration,
  IN  BOOLEAN             SignalFlag
  )
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


