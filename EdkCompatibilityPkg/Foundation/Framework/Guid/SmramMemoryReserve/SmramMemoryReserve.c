/*++

Copyright (c) 1999 - 2002, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  SmramMemoryReserve.c
    
Abstract:

  GUID for use in reserving SMRAM regions.

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION(SmramMemoryReserve)

EFI_GUID gEfiSmmPeiSmramMemoryReserve  = EFI_SMM_PEI_SMRAM_MEMORY_RESERVE;

EFI_GUID_STRING(&gEfiSmmPeiSmramMemoryReserve, "SMRAM Memory Reserve", "SMRAM Memory Reserve");
