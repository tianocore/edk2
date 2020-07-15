/** @file
  DXE Core library services.

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeMain.h"

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



