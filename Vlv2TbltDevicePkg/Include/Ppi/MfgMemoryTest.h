/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

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
