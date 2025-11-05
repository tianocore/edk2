/*++ @file
The PCD, gEmulatorPkgTokenSpaceGuid.PcdPeiServicesTablePage, points to a magic page
of memory that is like SRAM on an embedded system. This file defines what goes
where in the magic page.

Copyright (c) 2011, Apple Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EMU_MAGIC_PAGE_LIB_H__
#define __EMU_MAGIC_PAGE_LIB_H__

#include <PiPei.h>
#include <Library/PcdLib.h>
#include <Protocol/EmuThunk.h>

typedef struct {
  // Used by PEI Core and PEIMs to store the PEI Services pointer.
  // Privilege issues prevent using the PI mechanism in the emulator.
  CONST EFI_PEI_SERVICES    **PeiServicesTablePointer;

  // Used by SecPeiServicesLib
  EFI_PEI_PPI_DESCRIPTOR    *PpiList;

  // Needed by PEI PEI PeCoffLoaderExtraActionLib
  EMU_THUNK_PROTOCOL        *Thunk;
} EMU_MAGIC_PAGE_LAYOUT;

#define EMU_MAGIC_PAGE()  ((EMU_MAGIC_PAGE_LAYOUT *)((UINTN)PcdGet64 (PcdPeiServicesTablePage)))

#endif
