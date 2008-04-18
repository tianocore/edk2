/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
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

STATIC
VOID
EFIAPI
OnVirtualAddressChange (
  IN EFI_EVENT            Event,
  IN VOID                 *Context
  );

STATIC
EFI_STATUS
EFIAPI
MemEraseStore(
  IN VARIABLE_STORAGE   *This
  );

STATIC
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

  Status = gBS->AllocatePool (EfiRuntimeServicesData, sizeof(VS_DEV), &Dev);
  ASSERT_EFI_ERROR (Status);

  ZeroMem (Dev, sizeof(VS_DEV));

  Dev->Signature   = VARIABLE_STORE_SIGNATURE;
  Dev->Size        = Size;

  Dev->VarStore.Erase    = MemEraseStore;
  Dev->VarStore.Write    = MemWriteStore;

  Status = gBS->AllocatePool (EfiRuntimeServicesData, Size, &VAR_DATA_PTR (Dev));
  ASSERT_EFI_ERROR (Status);

  DEBUG ((EFI_D_ERROR, "VStorage: Size = 0x%x\n", Size));
  
  *VarStore       = &Dev->VarStore;
  *GoVirtualEvent = OnVirtualAddressChange;

  return EFI_SUCCESS;
}

STATIC
VOID
EFIAPI
OnVirtualAddressChange (
  IN EFI_EVENT            Event,
  IN VOID                 *Context
  )
{
  VS_DEV                  *Dev;

  Dev = DEV_FROM_THIS (Context);

  EfiConvertPointer (0, &VAR_DATA_PTR (Dev));
  EfiConvertPointer (0, (VOID**)&Dev->VarStore.Erase);
  EfiConvertPointer (0, (VOID**)&Dev->VarStore.Write);
}

STATIC
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


STATIC
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
