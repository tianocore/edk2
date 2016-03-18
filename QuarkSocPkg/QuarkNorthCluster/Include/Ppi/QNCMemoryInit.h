/** @file
Memory Initialization PPI used in EFI PEI interface

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __QNC_MEMORY_INIT_H__
#define __QNC_MEMORY_INIT_H__

#include "mrc.h"

#define PEI_QNC_MEMORY_INIT_PPI_GUID \
  {0x21ff1fee, 0xd33a, 0x4fce, {0xa6, 0x5e, 0x95, 0x5e, 0xa3, 0xc4, 0x1f, 0x40}}




//
// PPI Function Declarations
//
typedef
VOID
(EFIAPI *PEI_QNC_MEMORY_INIT) (
  IN OUT    MRCParams_t     *MRCDATA
  );

typedef struct _PEI_QNC_MEMORY_INIT_PPI {
  PEI_QNC_MEMORY_INIT     MrcStart;
}PEI_QNC_MEMORY_INIT_PPI;

extern EFI_GUID gQNCMemoryInitPpiGuid;

#endif
