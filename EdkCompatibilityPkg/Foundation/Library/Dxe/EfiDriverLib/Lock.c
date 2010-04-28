/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Lock.c

Abstract:

  Support for locking lib services.

--*/

#include "Tiano.h"
#include "EfiDriverLib.h"

VOID
EfiInitializeLock (
  IN OUT EFI_LOCK *Lock,
  IN EFI_TPL      Priority
  )
/*++

Routine Description:

  Initialize a basic mutual exclusion lock.   Each lock
  provides mutual exclusion access at it's task priority
  level.  Since there is no-premption (at any TPL) or
  multiprocessor support, acquiring the lock only consists
  of raising to the locks TPL.

  Note on a check build ASSERT()s are used to ensure proper
  lock usage.
    
Arguments:

  Lock        - The EFI_LOCK structure to initialize

  Priority    - The task priority level of the lock

    
Returns:

  An initialized Efi Lock structure.

--*/
{
  Lock->Tpl       = Priority;
  Lock->OwnerTpl  = 0;
  Lock->Lock      = 0;
}

EFI_STATUS
EfiAcquireLockOrFail (
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
  if (Lock->Lock != 0) {
    //
    // Lock is already owned, so bail out
    //
    return EFI_ACCESS_DENIED;
  }

  Lock->OwnerTpl = gBS->RaiseTPL (Lock->Tpl);

  Lock->Lock += 1;
  return EFI_SUCCESS;
}

VOID
EfiAcquireLock (
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
  EFI_STATUS  Status;

  Status = EfiAcquireLockOrFail (Lock);

  //
  // Lock was already locked.
  //
  ASSERT_EFI_ERROR (Status);
}

VOID
EfiReleaseLock (
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

  Tpl = Lock->OwnerTpl;

  ASSERT (Lock->Lock == 1);
  Lock->Lock -= 1;

  gBS->RestoreTPL (Tpl);
}
