/** @file
Framework PEIM to initialize memory on an DDR2 SDRAM Memory Controller.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef _PEI_QNC_MEMORY_INIT_H_
#define _PEI_QNC_MEMORY_INIT_H_

//
// The package level header files this module uses
//
#include <PiPei.h>
#include <IntelQNCPeim.h>
//
// The protocols, PPI and GUID defintions for this module
//
#include <Ppi/QNCMemoryInit.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/BaseMemoryLib.h>


VOID
EFIAPI
MrcStart (
  IN OUT MRCParams_t  *MrcData
  );

#endif
