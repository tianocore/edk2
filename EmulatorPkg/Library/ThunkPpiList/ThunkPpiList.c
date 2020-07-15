/** @file
  Emulator Thunk to abstract OS services from pure EFI code

  Copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

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





