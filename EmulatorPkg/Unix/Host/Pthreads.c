/*++ @file
 POSIX Pthreads to emulate APs and implement threads

Copyright (c) 2011, Apple Inc. All rights reserved.
Copyright (c) 2011 - 2019, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "Host.h"
#include <pthread.h>


UINTN
EFIAPI
PthreadMutexLock (
  IN VOID *Mutex
  )
{
  return (UINTN)pthread_mutex_lock ((pthread_mutex_t *)Mutex);
}



UINTN
EFIAPI
PthreadMutexUnLock (
  IN VOID *Mutex
  )
{
  return (UINTN)pthread_mutex_unlock ((pthread_mutex_t *)Mutex);
}


UINTN
EFIAPI
PthreadMutexTryLock (
  IN VOID *Mutex
  )
{
  return (UINTN)pthread_mutex_trylock ((pthread_mutex_t *)Mutex);
}


VOID *
PthreadMutexInit (
  IN VOID
  )
{
  pthread_mutex_t *Mutex;
  int             err;

  Mutex = malloc (sizeof (pthread_mutex_t));
  err = pthread_mutex_init (Mutex, NULL);
  if (err == 0) {
    return Mutex;
  }

  return NULL;
}


UINTN
PthreadMutexDestroy (
  IN VOID *Mutex
  )
{
  if (Mutex != NULL) {
    return pthread_mutex_destroy ((pthread_mutex_t *)Mutex);
  }

  return -1;
}

// Can't store this data on PthreadCreate stack so we need a global
typedef struct {
  pthread_mutex_t             Mutex;
  THREAD_THUNK_THREAD_ENTRY   Start;
} THREAD_MANGLE;

THREAD_MANGLE mThreadMangle = {
  PTHREAD_MUTEX_INITIALIZER,
  NULL
};

VOID *
SecFakePthreadStart (
  VOID  *Context
  )
{
  THREAD_THUNK_THREAD_ENTRY Start;
  sigset_t                  SigMask;

  // Save global on the stack before we unlock
  Start   = mThreadMangle.Start;
  pthread_mutex_unlock (&mThreadMangle.Mutex);

  // Mask all signals to the APs
  sigfillset (&SigMask);
  pthread_sigmask (SIG_BLOCK, &SigMask, NULL);

  //
  // We have to start the thread in SEC as we need to follow
  // OS X calling conventions. We can then call back into
  // to the callers Start.
  //
  // This is a great example of how all problems in computer
  // science can be solved by adding another level of indirection
  //
 return  (VOID *)ReverseGasketUint64 ((UINTN)Start, (UINTN)Context);
}

UINTN
PthreadCreate (
  IN  VOID                      *Thread,
  IN  VOID                      *Attribute,
  IN  THREAD_THUNK_THREAD_ENTRY Start,
  IN  VOID                      *Context
  )
{
  int         err;
  BOOLEAN     EnabledOnEntry;

  //
  // Threads inherit interrupt state so disable interrupts before we start thread
  //
  if (SecInterruptEanbled ()) {
    SecDisableInterrupt ();
    EnabledOnEntry = TRUE;
  } else {
    EnabledOnEntry = FALSE;
  }

  // Acquire lock for global, SecFakePthreadStart runs in a different thread.
  pthread_mutex_lock (&mThreadMangle.Mutex);
  mThreadMangle.Start   = Start;

  err = pthread_create (Thread, Attribute, SecFakePthreadStart, Context);
  if (err != 0) {
    // Thread failed to launch so release the lock;
    pthread_mutex_unlock (&mThreadMangle.Mutex);
  }

  if (EnabledOnEntry) {
    // Restore interrupt state
    SecEnableInterrupt ();
  }

  return err;
}


VOID
PthreadExit (
  IN VOID *ValuePtr
  )
{
  pthread_exit (ValuePtr);
  return;
}


UINTN
PthreadSelf (
  VOID
  )
{
  // POSIX currently allows pthread_t to be a structure or arithmetic type.
  // Check out sys/types.h to make sure this will work if you are porting.
  // On OS X (Darwin) pthread_t is a pointer to a structure so this code works.
  return (UINTN)pthread_self ();
}


EMU_THREAD_THUNK_PROTOCOL gPthreadThunk = {
  GasketPthreadMutexLock,
  GasketPthreadMutexUnLock,
  GasketPthreadMutexTryLock,
  GasketPthreadMutexInit,
  GasketPthreadMutexDestroy,
  GasketPthreadCreate,
  GasketPthreadExit,
  GasketPthreadSelf
};


EFI_STATUS
PthreadOpen (
  IN  EMU_IO_THUNK_PROTOCOL   *This
  )
{
  if (This->Instance != 0) {
    // Only single instance is supported
    return EFI_NOT_FOUND;
  }

  if (This->ConfigString[0] == L'0') {
    // If AP count is zero no need for threads
    return EFI_NOT_FOUND;
  }

  This->Interface = &gPthreadThunk;

  return EFI_SUCCESS;
}


EFI_STATUS
PthreadClose (
  IN  EMU_IO_THUNK_PROTOCOL   *This
  )
{
  return EFI_SUCCESS;
}


EMU_IO_THUNK_PROTOCOL gPthreadThunkIo = {
  &gEmuThreadThunkProtocolGuid,
  NULL,
  NULL,
  0,
  GasketPthreadOpen,
  GasketPthreadClose,
  NULL
};


