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

  Support for locking lib services. These primitives may be implemented 
  as Esal calls but since these result in small code that us position
  independent, we can use lib functions. ESAL calls have a significant
  software overhead and too deep nesting is bad for the stack.

--*/

#include "Tiano.h"
#include "EfiDriverLib.h"

extern
BOOLEAN
EfiAtRuntime (
  VOID
  );

VOID
EfiInitializeLock (
  IN OUT EFI_LOCK *Lock,
  IN EFI_TPL      Priority
  )
/*++

Routine Description:

  Initialize a basic mutual exclusion lock. There is 
  no concept of TPL at runtime hence priority is
  ignored.
    
Arguments:

  Lock        - The EFI_LOCK structure to initialize

  Priority    - Ignored

    
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

  Initialize a basic mutual exclusion lock. For now,
  only allow one level of nesting.
    
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

  if (!EfiAtRuntime ()) {
    //
    // The check is just debug code for core inplementation. It must
    //  always be true in a driver
    //
    Lock->OwnerTpl = gBS->RaiseTPL (Lock->Tpl);
  }

  Lock->Lock += 1;
  return EFI_SUCCESS;
}

VOID
EfiAcquireLock (
  IN EFI_LOCK  *Lock
  )
/*++

Routine Description:

  Acquires ownership of the lock.
    
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

    Releases ownership of the mutual exclusion lock.
    
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

  if (!EfiAtRuntime ()) {
    //
    // The check is just debug code for core inplementation. It must
    //  always be true in a driver
    //
    gBS->RestoreTPL (Tpl);
  }
}
