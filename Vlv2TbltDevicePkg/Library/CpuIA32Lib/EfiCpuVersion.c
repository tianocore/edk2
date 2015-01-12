/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:


  EfiCpuVersion.c

Abstract:

  Provide cpu version extract considering extended family & model ID.
--*/

#include <Library/CpuIA32.h>

/**
  Extract CPU detail version infomation

  @param  FamilyId    FamilyId, including ExtendedFamilyId
  @param  Model       Model, including ExtendedModel
  @param  SteppingId  SteppingId
  @param  Processor   Processor

**/
VOID
EFIAPI
EfiCpuVersion (
  IN  OUT UINT16  *FamilyId,    OPTIONAL
  IN  OUT UINT8   *Model,       OPTIONAL
  IN  OUT UINT8   *SteppingId,  OPTIONAL
  IN  OUT UINT8   *Processor    OPTIONAL
  )

{
  EFI_CPUID_REGISTER Register;
  UINT8              TempFamilyId;

  EfiCpuid (EFI_CPUID_VERSION_INFO, &Register);

  if (SteppingId != NULL) {
    *SteppingId = (UINT8) (Register.RegEax & 0xF);
  }

  if (Processor != NULL) {
    *Processor = (UINT8) ((Register.RegEax >> 12) & 0x3);
  }

  if (Model != NULL || FamilyId != NULL) {
    TempFamilyId = (UINT8) ((Register.RegEax >> 8) & 0xF);

    if (Model != NULL) {
      *Model = (UINT8) ((Register.RegEax >> 4) & 0xF);
      if (TempFamilyId == 0x6 || TempFamilyId == 0xF) {
        *Model = (UINT8) (*Model  | ((Register.RegEax >> 12) & 0xF0));
      }
    }

    if (FamilyId != NULL) {
      *FamilyId = TempFamilyId;
      if (TempFamilyId == 0xF) {
        *FamilyId = (UINT8 ) (*FamilyId + (UINT16) ((Register.RegEax >> 20) & 0xFF));
      }
    }
  }
}
