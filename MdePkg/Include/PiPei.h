/** @file

  Root include file for Mde Package SEC, PEIM, PEI_CORE type modules.

  This is the include file for any module of type PEIM. PEIM
  modules only use types defined via this include file and can
  be ported easily to any environment. 

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PI_PEI_H__
#define __PI_PEI_H__

#include <Uefi/UefiBaseType.h>
#include <Pi/PiPeiCis.h>
#include <Uefi/UefiMultiPhase.h>

//
// BUGBUG: The EFI_PEI_STARTUP_DESCRIPTOR definition does not follows PI specification.
//         After enabling PI for Nt32Pkg and tools generate correct autogen for PEI_CORE,
//         the following structure should be removed at once.
//
typedef struct {
  UINTN                   BootFirmwareVolume;
  UINTN                   SizeOfCacheAsRam;
  EFI_PEI_PPI_DESCRIPTOR  *DispatchTable;
} EFI_PEI_STARTUP_DESCRIPTOR;

#endif

