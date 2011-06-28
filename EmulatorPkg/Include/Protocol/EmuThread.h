/** @file
  Emulator Thunk to abstract OS services from pure EFI code

  Copyright (c) 2010 - 2011, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EMU_THREAD_THUNK__
#define __EMU_THREAD_THUNK__


typedef struct _EMU_THREAD_THUNK_PROTOCOL  EMU_THREAD_THUNK_PROTOCOL;


typedef
UINTN
(EFIAPI *THREAD_THUNK_MUTEX_LOCK) (
  IN VOID *Mutex
  );


typedef
UINTN
(EFIAPI *THREAD_THUNK_MUTEX_UNLOCK) (
  IN VOID *Mutex
  );


typedef
UINTN
(EFIAPI *THREAD_THUNK_MUTEX_TRY_LOCK) (
  IN VOID *Mutex
  );


typedef
VOID *
(EFIAPI *THREAD_THUNK_MUTEX_INIT) (
  IN VOID
  );


typedef
UINTN
(EFIAPI *THREAD_THUNK_MUTEX_DISTROY) (
  IN VOID *Mutex
  );



typedef
VOID *
(EFIAPI *THREAD_THUNK_THREAD_ENTRY) (
  IN  VOID *Context
  );

typedef
UINTN
(EFIAPI *THREAD_THUNK_CREATE_THREAD) (
  IN  VOID                      *Thread,
  IN  VOID                      *Attribute,
  IN  THREAD_THUNK_THREAD_ENTRY Start,
  IN  VOID                      *Context
  );

typedef
VOID
(EFIAPI *THREAD_THUNK_EXIT_THREAD) (
  IN VOID *ValuePtr
  );


typedef
UINTN
(EFIAPI *THREAD_THUNK_SELF) (
  VOID
  );


struct _EMU_THREAD_THUNK_PROTOCOL {
  THREAD_THUNK_MUTEX_LOCK       MutexLock;
  THREAD_THUNK_MUTEX_UNLOCK     MutexUnlock;
  THREAD_THUNK_MUTEX_TRY_LOCK   MutexTryLock;
  THREAD_THUNK_MUTEX_INIT       MutexInit;
  THREAD_THUNK_MUTEX_DISTROY    MutexDistroy;
  THREAD_THUNK_CREATE_THREAD    CreateThread;
  THREAD_THUNK_EXIT_THREAD      ExitThread;
  THREAD_THUNK_SELF             Self;
};

extern EFI_GUID gEmuThreadThunkProtocolGuid;

#endif

