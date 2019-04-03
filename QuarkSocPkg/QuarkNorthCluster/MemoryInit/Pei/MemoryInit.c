/** @file
Framework PEIM to initialize memory on a QuarkNcSocId Memory Controller.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// Include common header file for this module.
//
#include "MemoryInit.h"

static PEI_QNC_MEMORY_INIT_PPI mPeiQNCMemoryInitPpi =
{ MrcStart };

static EFI_PEI_PPI_DESCRIPTOR PpiListPeiQNCMemoryInit =
{
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gQNCMemoryInitPpiGuid,
    &mPeiQNCMemoryInitPpi
};

void Mrc( MRCParams_t *MrcData);

/**

 Do memory initialization for QuarkNcSocId DDR3 SDRAM Controller

 @param  FfsHeader    Not used.
 @param  PeiServices  General purpose services available to every PEIM.

 @return EFI_SUCCESS  Memory initialization completed successfully.
 All other error conditions encountered result in an ASSERT.

 **/
EFI_STATUS
PeimMemoryInit(
    IN EFI_PEI_FILE_HANDLE FileHandle,
    IN CONST EFI_PEI_SERVICES **PeiServices
    )
{
  EFI_STATUS Status;

  Status = (**PeiServices).InstallPpi(PeiServices, &PpiListPeiQNCMemoryInit);

  return Status;
}

VOID
EFIAPI
MrcStart(
    IN OUT MRCParams_t *MrcData
    )
{

  Mrc(MrcData);
}
