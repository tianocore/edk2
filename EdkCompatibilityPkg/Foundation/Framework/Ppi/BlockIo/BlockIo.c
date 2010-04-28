/*++

Copyright (c) 1999 - 2002, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  BlockIo.c

Abstract:

  BlockIo PPI GUID as defined in EFI 2.0

--*/

#include "Tiano.h"
#include "Pei.h"
#include EFI_PPI_DEFINITION (BlockIo)

EFI_GUID  gPeiBlockIoPpiGuid = PEI_BLOCK_IO_PPI_GUID;

EFI_GUID_STRING(&gPeiBlockIoPpiGuid, "BlockIo", "PEI Block I/O PPI");
