/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

Module Name:

  BaseMemoryTest.h

Abstract:

  Pei memory test PPI as defined in Tiano

  Used to Pei memory test in PEI

--*/

#ifndef _BASE_MEMORY_TEST_H_
#define _BASE_MEMORY_TEST_H_

typedef struct _PEI_MFG_MEMORY_TEST_PPI PEI_MFG_MEMORY_TEST_PPI;

typedef
EFI_STATUS
(EFIAPI *PEI_MFG_MEMORY_TEST) (
  IN  CONST EFI_PEI_SERVICES                   **PeiServices,
  IN  PEI_MFG_MEMORY_TEST_PPI            * This,
  IN  UINT32                             BeginAddress,
  IN  UINT32                             MemoryLength
  );

typedef struct _PEI_MFG_MEMORY_TEST_PPI {
  PEI_MFG_MEMORY_TEST  MfgMemoryTest;
}PEI_MFG_MEMORY_TEST_PPI;


extern EFI_GUID gPeiMfgMemoryTestPpiGuid;

#endif
