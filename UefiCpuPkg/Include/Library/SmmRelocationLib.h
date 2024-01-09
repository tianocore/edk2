/** @file
  Header file for SMM Relocation Library.

  The SmmRelocationLib class provides the SmmRelocationInit()
  interface for platform to do the smbase relocation, which
  shall provide below 2 functionalities:
  1. Relocate SmBases for each processor.
  2. Create the SmBase HOB (gSmmBaseHobGuid).

  With SmmRelocationLib, PiSmmCpuDxeSmm driver (which runs at a later phase)
  shall:
  1. Consume the gSmmBaseHobGuid for the relocated smbase for each Processor.
  2. Execute early SMM init.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMM_RELOCATION_LIB_H_
#define SMM_RELOCATION_LIB_H_

#include <Ppi/MpServices2.h>

/**
  CPU SmmBase Relocation Init.

  This function is to relocate CPU SmmBase.

  @param[in] MpServices2        Pointer to this instance of the MpServices.

  @retval EFI_SUCCESS           CPU SmmBase Relocated successfully.
  @retval Others                CPU SmmBase Relocation failed.

**/
EFI_STATUS
EFIAPI
SmmRelocationInit (
  IN EDKII_PEI_MP_SERVICES2_PPI  *MpServices2
  );

#endif
