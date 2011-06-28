/** @file
  Emulator Thunk to abstract OS services from pure EFI code

  Copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>


UINTN                   gThunkPpiListSize = 0;
EFI_PEI_PPI_DESCRIPTOR  *gThunkPpiList = NULL;



EFI_PEI_PPI_DESCRIPTOR *
GetThunkPpiList (
  VOID
  )
{
  UINTN Index;

  if (gThunkPpiList == NULL) {
    return NULL;
  }

  Index = (gThunkPpiListSize/sizeof (EFI_PEI_PPI_DESCRIPTOR)) - 1;
  gThunkPpiList[Index].Flags |= EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;

  return gThunkPpiList;
}


EFI_STATUS
EFIAPI
AddThunkPpi (
  IN  UINTN     Flags,
  IN  EFI_GUID  *Guid,
  IN  VOID      *Ppi
  )
{
  UINTN Index;

  gThunkPpiList = ReallocatePool (
                    gThunkPpiListSize,
                    gThunkPpiListSize + sizeof (EFI_PEI_PPI_DESCRIPTOR),
                    gThunkPpiList
                    );
  if (gThunkPpiList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Index = (gThunkPpiListSize/sizeof (EFI_PEI_PPI_DESCRIPTOR));
  gThunkPpiList[Index].Flags = Flags;
  gThunkPpiList[Index].Guid  = Guid;
  gThunkPpiList[Index].Ppi   = Ppi;
  gThunkPpiListSize += sizeof (EFI_PEI_PPI_DESCRIPTOR);

  return EFI_SUCCESS;
}





