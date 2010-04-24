/** @file
  GUID and data structures for communication between SMM Base on SMM Base2 Thunk driver
  and SmmBaseHelper driver.

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef  _SMM_BASE_THUNK_COMMUNICATION_H_
#define  _SMM_BASE_THUNK_COMMUNICATION_H_

#include <Protocol/SmmBase.h>

#define EFI_SMM_BASE_THUNK_COMMUNICATION_GUID \
  { 0x6568a3d6, 0x15f, 0x4b4a, { 0x9c, 0x89, 0x1d, 0x14, 0x63, 0x14, 0x13, 0xa } }

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL       *FilePath;
  VOID                           *SourceBuffer;
  UINTN                          SourceSize;
  EFI_HANDLE                     *ImageHandle;
  BOOLEAN                        LegacyIA32Binary;
} SMMBASE_REGISTER_ARG;

typedef struct {
  EFI_HANDLE                     ImageHandle;
} SMMBASE_UNREGISTER_ARG;

typedef struct {
  EFI_HANDLE                     SmmImageHandle;
  EFI_SMM_CALLBACK_ENTRY_POINT   CallbackAddress;
  BOOLEAN                        MakeLast;
  BOOLEAN                        FloatingPointSave;
} SMMBASE_REGISTER_CALLBACK_ARG;

typedef struct {
  EFI_MEMORY_TYPE                PoolType;
  UINTN                          Size;
  VOID                           **Buffer;
} SMMBASE_ALLOCATE_POOL_ARG;

typedef struct {
  VOID                           *Buffer;
} SMMBASE_FREE_POOL_ARG;

typedef struct {
  EFI_HANDLE                     ImageHandle;
  VOID                           *CommunicationBuffer;
  UINTN                          *SourceSize;
} SMMBASE_COMMUNICATE_ARG;

typedef union {
  SMMBASE_REGISTER_ARG           Register;
  SMMBASE_UNREGISTER_ARG         UnRegister;
  SMMBASE_REGISTER_CALLBACK_ARG  RegisterCallback;
  SMMBASE_ALLOCATE_POOL_ARG      AllocatePool;
  SMMBASE_FREE_POOL_ARG          FreePool;
  SMMBASE_COMMUNICATE_ARG        Communicate;
} SMMBASE_FUNCTION_ARGS;

typedef enum {
  SmmBaseFunctionRegister,
  SmmBaseFunctionUnregister,
  SmmBaseFunctionRegisterCallback,
  SmmBaseFunctionAllocatePool,
  SmmBaseFunctionFreePool,
  SmmBaseFunctionCommunicate
} SMMBASE_FUNCTION;

typedef struct {
  SMMBASE_FUNCTION       Function;
  EFI_STATUS             Status;
  SMMBASE_FUNCTION_ARGS  Args;
  EFI_HANDLE             SmmBaseImageHandle;
} SMMBASE_FUNCTION_DATA;

#pragma pack(1)
typedef struct {
  EFI_GUID               HeaderGuid;
  UINTN                  MessageLength;
  SMMBASE_FUNCTION_DATA  FunctionData;
} SMMBASETHUNK_COMMUNICATION_DATA;
#pragma pack()

extern EFI_GUID gEfiSmmBaseThunkCommunicationGuid;

#endif

