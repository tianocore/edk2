/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:


  ExI.c

Abstract:

  ExI configuration based on setup option


--*/


#include "PlatformDxe.h"

#define PchLpcPciCfg32(Register)  MmioRead32 (MmPciAddress (0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_LPC, 0, Register))

//
// Procedure: GetPmcBase
//
// Description: This function read content of B:D:F 0:31:0, offset 44h (for
// PmcBase)
//
// Input: None
//
// Output:  32 bit PmcBase
//
UINT32
GetPmcBase (
  VOID
  )
{
  return (PchLpcPciCfg32 (R_PCH_LPC_PMC_BASE) & B_PCH_LPC_PMC_BASE_BAR);
}

/**
  Configure ExI.

  @param ImageHandle    Pointer to the loaded image protocol for this driver
  @param SystemTable    Pointer to the EFI System Table

  @retval EFI_SUCCESS   The driver initializes correctly.
**/
VOID
InitExI (
  )
{
  EFI_STATUS                  Status;

  SYSTEM_CONFIGURATION          SystemConfiguration;
  UINTN       VarSize;

  VarSize = sizeof(SYSTEM_CONFIGURATION);

  Status = gRT->GetVariable(
                  L"Setup",
                  &gEfiNormalSetupGuid,
                  NULL,
                  &VarSize,
                  &SystemConfiguration
                  );

  if (EFI_ERROR (Status) || VarSize != sizeof(SYSTEM_CONFIGURATION)) {
    //The setup variable is corrupted
    VarSize = sizeof(SYSTEM_CONFIGURATION);
    Status = gRT->GetVariable(
              L"SetupRecovery",
              &gEfiNormalSetupGuid,
              NULL,
              &VarSize,
              &SystemConfiguration
              );
    ASSERT_EFI_ERROR (Status);
  }  

  if (SystemConfiguration.ExISupport == 1) {
	  MmioOr32 ((UINTN) (GetPmcBase() + R_PCH_PMC_MTPMC1), (UINT32) BIT0+BIT1+BIT2);
  } else if (SystemConfiguration.ExISupport == 0) {
    MmioAnd32 ((UINTN) (GetPmcBase() + R_PCH_PMC_MTPMC1), ~((UINT32) BIT0+BIT1+BIT2)); //clear bit 0,1,2
  }
}
