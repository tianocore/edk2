/*++

Caution: This file is used for Duet platform only, do not use them in real platform.
All variable code, variable metadata, and variable data used by Duet platform are on 
disk. They can be changed by user. BIOS is not able to protoect those.
Duet trusts all meta data from disk. If variable code, variable metadata and variable
data is modified in inproper way, the behavior is undefined.

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    MemStorage.c

Abstract:

    handles variable store/reads with emulated memory

Revision History

--*/
#include "FSVariable.h"

VOID
EFIAPI
OnVirtualAddressChangeMs (
  IN EFI_EVENT            Event,
  IN VOID                 *Context
  );

EFI_STATUS
EFIAPI
MemEraseStore(
  IN VARIABLE_STORAGE   *This
  );

EFI_STATUS
EFIAPI
MemWriteStore (
  IN VARIABLE_STORAGE   *This,
  IN UINTN                Offset,
  IN UINTN                BufferSize,
  IN VOID                 *Buffer
  );

EFI_STATUS
MemStorageConstructor (
  OUT VARIABLE_STORAGE          **VarStore,
  OUT EFI_EVENT_NOTIFY          *GoVirtualEvent,
  IN  UINTN                     Size
  )
{
  EFI_STATUS                  Status;
  VS_DEV                      *Dev;

  Status = gBS->AllocatePool (EfiRuntimeServicesData, sizeof(VS_DEV), (VOID **) &Dev);
  ASSERT_EFI_ERROR (Status);

  ZeroMem (Dev, sizeof(VS_DEV));

  Dev->Signature   = VS_DEV_SIGNATURE;
  Dev->Size        = Size;

  Dev->VarStore.Erase    = MemEraseStore;
  Dev->VarStore.Write    = MemWriteStore;

  Status = gBS->AllocatePool (EfiRuntimeServicesData, Size, (VOID **) &VAR_DATA_PTR (Dev));
  ASSERT_EFI_ERROR (Status);

  DEBUG ((EFI_D_ERROR, "VStorage: Size = 0x%x\n", Size));
  
  *VarStore       = &Dev->VarStore;
  *GoVirtualEvent = OnVirtualAddressChangeMs;

  return EFI_SUCCESS;
}

VOID
EFIAPI
OnVirtualAddressChangeMs (
  IN EFI_EVENT            Event,
  IN VOID                 *Context
  )
{
  VS_DEV                  *Dev;

  Dev = DEV_FROM_THIS (Context);

  EfiConvertPointer (0, (VOID **)&VAR_DATA_PTR (Dev));
  EfiConvertPointer (0, (VOID **)&Dev->VarStore.Erase);
  EfiConvertPointer (0, (VOID **)&Dev->VarStore.Write);
}

EFI_STATUS
EFIAPI
MemEraseStore(
  IN VARIABLE_STORAGE   *This
  )
{
  VS_DEV              *Dev;

  Dev = DEV_FROM_THIS(This);
  SetMem (VAR_DATA_PTR (Dev), Dev->Size, VAR_DEFAULT_VALUE);
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
MemWriteStore (
  IN VARIABLE_STORAGE     *This,
  IN UINTN                Offset,
  IN UINTN                BufferSize,
  IN VOID                 *UserBuffer
  )
{
  VS_DEV              *Dev;

  Dev = DEV_FROM_THIS(This);

  ASSERT (Offset + BufferSize < Dev->Size);

  // For better performance
  if (VAR_DATA_PTR (Dev) + Offset != UserBuffer) {
    CopyMem (VAR_DATA_PTR (Dev) + Offset, UserBuffer, BufferSize);
  }
  return EFI_SUCCESS;
}
