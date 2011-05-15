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

#ifndef __EMU_PTHREAD_THUNK__ 
#define __EMU_PTHREAD_THUNK__

#define EMU_PTHREAD_THUNK_PROTOCO_GUID  \
 { 0x3B1E4B7C, 0x09D8, 0x944F, { 0xA4, 0x08, 0x13, 0x09, 0xEB, 0x8B, 0x44, 0x27 } }


typedef struct _EMU_PTREAD_THUNK_PROTOCOL  EMU_PTREAD_THUNK_PROTOCOL;


typedef 
UINTN
(EFIAPI *PTREAD_THUNK_MUTEXT_LOCK) (
  IN VOID *Mutex
  );            


typedef 
UINTN
(EFIAPI *PTREAD_THUNK_MUTEXT_UNLOCK) (
  IN VOID *Mutex
  );                        


typedef 
UINTN
(EFIAPI *PTREAD_THUNK_MUTEX_TRY_LOCK) (
  IN VOID *Mutex
  );                      


typedef 
VOID *
(EFIAPI *PTREAD_THUNK_MUTEX_INIT) (
  IN VOID
  );                     


typedef 
UINTN
(EFIAPI *PTREAD_THUNK_MUTEX_DISTROY) (
  IN VOID *Mutex
  );                            



typedef 
VOID *
(EFIAPI *PTREAD_THUNK_THEAD_ENTRY) (
  IN  VOID *Context
  );

typedef 
UINTN
(EFIAPI *PTREAD_THUNK_CREATE_THREAD) (
  IN  VOID                      *Thread,
  IN  VOID                      *Attribute,
  IN  PTREAD_THUNK_THEAD_ENTRY  Start,
  IN  VOID                      *Context
  );

typedef 
VOID
(EFIAPI *PTREAD_THUNK_EXIT_THREAD) (
  IN VOID *ValuePtr
  );                            

  
typedef 
UINTN
(EFIAPI *PTREAD_THUNK_SELF) (
  VOID
  );                              


struct _EMU_PTREAD_THUNK_PROTOCOL {
  PTREAD_THUNK_MUTEXT_LOCK      MutextLock;
  PTREAD_THUNK_MUTEXT_UNLOCK    MutexUnlock;
  PTREAD_THUNK_MUTEX_TRY_LOCK   MutexTryLock;
  PTREAD_THUNK_MUTEX_INIT       MutexInit;
  PTREAD_THUNK_MUTEX_DISTROY    MutexDistroy;
  PTREAD_THUNK_CREATE_THREAD    CreateThread;
  PTREAD_THUNK_EXIT_THREAD      ExitThread;
  PTREAD_THUNK_SELF             Self;
};

extern EFI_GUID gEmuPthreadThunkProtocolGuid;

#endif

