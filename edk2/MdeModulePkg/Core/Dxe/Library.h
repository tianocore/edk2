/** @file
  Internal functions shared in DxeCore module.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _DXE_LIBRARY_H_
#define _DXE_LIBRARY_H_



/**
  Report status code of type EFI_PROGRESS_CODE by caller ID gEfiCallerIdGuid.

  @param  Value              Describes the class/subclass/operation of the
                             hardware or software entity that the Status Code
                             relates to.

**/
VOID
CoreReportProgressCode (
  IN  EFI_STATUS_CODE_VALUE   Value
  );


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
  );


/**
  Raising to the task priority level of the mutual exclusion
  lock, and then acquires ownership of the lock.

  @param  Lock               The lock to acquire

  @return Lock owned

**/
VOID
CoreAcquireLock (
  IN EFI_LOCK  *Lock
  );


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
  );


/**
  Releases ownership of the mutual exclusion lock, and
  restores the previous task priority level.

  @param  Lock               The lock to release

  @return Lock unowned

**/
VOID
CoreReleaseLock (
  IN EFI_LOCK  *Lock
  );

//
// Device Path functions
//


/**
  Calculate the size of a whole device path.

  @param  DevicePath         The pointer to the device path data.

  @return Size of device path data structure..

**/
UINTN
CoreDevicePathSize (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );


/**
  Return TRUE is this is a multi instance device path.

  @param  DevicePath         A pointer to a device path data structure.

  @retval TRUE               If DevicePath is multi instance. FALSE - If
                             DevicePath is not multi instance.

**/
BOOLEAN
CoreIsDevicePathMultiInstance (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );



/**
  Duplicate a new device path data structure from the old one.

  @param  DevicePath         A pointer to a device path data structure.

  @return A pointer to the new allocated device path data.
  @return Caller must free the memory used by DevicePath if it is no longer needed.

**/
EFI_DEVICE_PATH_PROTOCOL *
CoreDuplicateDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  );


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
  );


/**
  Allocate pool of type EfiBootServicesData, the size is specified with AllocationSize.

  @param  AllocationSize     Size to allocate.

  @return Pointer of the allocated pool.

**/
VOID *
CoreAllocateBootServicesPool (
  IN  UINTN   AllocationSize
  );


/**
  Allocate pool of type EfiBootServicesData and zero it, the size is specified with AllocationSize.

  @param  AllocationSize     Size to allocate.

  @return Pointer of the allocated pool.

**/
VOID *
CoreAllocateZeroBootServicesPool (
  IN  UINTN   AllocationSize
  );


/**
  Find a config table by name in system table's ConfigurationTable.

  @param  Guid           The table name to look for
  @param  Table          Pointer of the config table

  @retval EFI_NOT_FOUND  Could not find the table in system table's
                         ConfigurationTable.
  @retval EFI_SUCCESS    Table successfully found.

**/
EFI_STATUS
CoreGetConfigTable (
  IN EFI_GUID *Guid,
  OUT VOID    **Table
  );


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
  );


/**
  Allocate pool of type EfiRuntimeServicesData, the size is specified with AllocationSize.

  @param  AllocationSize     Size to allocate.

  @return Pointer of the allocated pool.

**/
VOID *
CoreAllocateRuntimePool (
  IN  UINTN   AllocationSize
  );


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
  );


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
  );

#endif
