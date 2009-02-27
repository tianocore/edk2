/** @file

  The internal header file includes the common header files, defines
  internal structure and functions used by RuntimeVariable module.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _VARIABLE_H_
#define _VARIABLE_H_

#include <PiDxe.h>
#include <Protocol/VariableWrite.h>
#include <Protocol/FaultTolerantWriteLite.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/Variable.h>
#include <Library/PcdLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FvbServiceLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Guid/GlobalVariable.h>
#include <Guid/EventGroup.h>
#include <Guid/VariableFormat.h>

#define VARIABLE_RECLAIM_THRESHOLD (1024)

typedef struct {
  VARIABLE_HEADER *CurrPtr;
  VARIABLE_HEADER *EndPtr;
  VARIABLE_HEADER *StartPtr;
  BOOLEAN         Volatile;
} VARIABLE_POINTER_TRACK;

typedef struct {
  EFI_PHYSICAL_ADDRESS  VolatileVariableBase;
  EFI_PHYSICAL_ADDRESS  NonVolatileVariableBase;
  EFI_LOCK              VariableServicesLock;
  UINT32                ReentrantState;
} VARIABLE_GLOBAL;

typedef struct {
  VARIABLE_GLOBAL VariableGlobal;
  UINTN           VolatileLastVariableOffset;
  UINTN           NonVolatileLastVariableOffset;
  UINT32          FvbInstance;
} VARIABLE_MODULE_GLOBAL;

typedef struct {
  EFI_GUID    *Guid;
  CHAR16      *Name;
  UINT32      Attributes;
  UINTN       DataSize;
  VOID        *Data;
} VARIABLE_CACHE_ENTRY;

/**
  Writes a buffer to variable storage space, in the working block.

  This function writes a buffer to variable storage space into firmware
  volume block device. The destination is specified by parameter
  VariableBase. Fault Tolerant Write protocol is used for writing.

  @param  VariableBase   Base address of variable to write
  @param  Buffer         Point to the data buffer
  @param  BufferSize     The number of bytes of the data Buffer

  @retval EFI_SUCCESS    The function completed successfully
  @retval EFI_NOT_FOUND  Fail to locate Fault Tolerant Write protocol
  @retval EFI_ABORTED    The function could not complete successfully

**/
EFI_STATUS
FtwVariableSpace (
  IN EFI_PHYSICAL_ADDRESS   VariableBase,
  IN UINT8                  *Buffer,
  IN UINTN                  BufferSize
  );


#endif
