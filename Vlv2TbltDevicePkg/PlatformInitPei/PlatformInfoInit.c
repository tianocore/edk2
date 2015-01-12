/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

  PlatformInfoInit.c

Abstract:
  Platform Info Driver.

--*/

#include "PlatformEarlyInit.h"

#define LEN_64M       0x4000000

//
// Default PCI32 resource size
//
#define RES_MEM32_MIN_LEN   0x38000000

#define RES_IO_BASE   0x0D00
#define RES_IO_LIMIT  0xFFFF

#define MemoryCeilingVariable   L"MemCeil."

EFI_STATUS
CheckOsSelection (
  IN CONST EFI_PEI_SERVICES          **PeiServices,
  IN SYSTEM_CONFIGURATION            *SystemConfiguration
  )
{
  EFI_STATUS                   Status;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI   *Variable;
  UINTN                        VariableSize;
  EFI_OS_SELECTION_HOB         *OsSelectionHob;
  UINT8                        OsSelection;
  UINT8                        *LpssDataHobPtr;
  UINT8                        *LpssDataVarPtr;
  UINTN                        i;

  Status = (*PeiServices)->LocatePpi (
                             PeiServices,
                             &gEfiPeiReadOnlyVariable2PpiGuid,
                             0,
                             NULL,
                                      (void **)&Variable
                             );
  if (!EFI_ERROR(Status)) {
    VariableSize = sizeof (OsSelection);
    Status = Variable->GetVariable (
                         Variable,
                         L"OsSelection",
                         &gOsSelectionVariableGuid,
                         NULL,
                         &VariableSize,
                         &OsSelection
                         );

    if (!EFI_ERROR(Status) && (SystemConfiguration->ReservedO != OsSelection)) {
      //
      // Build HOB for OsSelection
      //
      OsSelectionHob = BuildGuidHob (&gOsSelectionVariableGuid, sizeof (EFI_OS_SELECTION_HOB));
      ASSERT (OsSelectionHob != NULL);

      OsSelectionHob->OsSelectionChanged = TRUE;
      OsSelectionHob->OsSelection        = OsSelection;
      SystemConfiguration->ReservedO   = OsSelectionHob->OsSelection;

      //
      // Load LPSS and SCC defalut configurations
      //
      OsSelectionHob->LpssData.LpsseMMCEnabled            = FALSE;
      OsSelectionHob->LpssData.LpssSdioEnabled            = TRUE;
      OsSelectionHob->LpssData.LpssSdcardEnabled          = TRUE;
      OsSelectionHob->LpssData.LpssSdCardSDR25Enabled     = FALSE;
      OsSelectionHob->LpssData.LpssSdCardDDR50Enabled     = TRUE;
      OsSelectionHob->LpssData.LpssMipiHsi                = FALSE;
      OsSelectionHob->LpssData.LpsseMMC45Enabled          = TRUE;
      OsSelectionHob->LpssData.LpsseMMC45DDR50Enabled     = TRUE;
      OsSelectionHob->LpssData.LpsseMMC45HS200Enabled     = FALSE;
      OsSelectionHob->LpssData.LpsseMMC45RetuneTimerValue = 8;
      OsSelectionHob->LpssData.eMMCBootMode               = 1;     // Auto Detect


      SystemConfiguration->Lpe       = OsSelectionHob->Lpe;
      SystemConfiguration->PchAzalia = SystemConfiguration->PchAzalia;
      LpssDataHobPtr = &OsSelectionHob->LpssData.LpssPciModeEnabled;
      LpssDataVarPtr = &SystemConfiguration->LpssPciModeEnabled;

      for (i = 0; i < sizeof(EFI_PLATFORM_LPSS_DATA); i++) {
        *LpssDataVarPtr = *LpssDataHobPtr;
        LpssDataVarPtr++;
        LpssDataHobPtr++;
      }
    }
  }

  return EFI_SUCCESS;
}


EFI_STATUS
PlatformInfoUpdate (
  IN CONST EFI_PEI_SERVICES          **PeiServices,
  IN OUT EFI_PLATFORM_INFO_HOB *PlatformInfoHob,
  IN SYSTEM_CONFIGURATION      *SystemConfiguration
  )
{
  EFI_STATUS                   Status;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI   *Variable;
  UINTN                        VariableSize;
  UINT32                       MemoryCeiling;

  //
  // Checking PCI32 resource from previous boot to determine the memory ceiling
  //
  Status = (*PeiServices)->LocatePpi (
                             PeiServices,
                             &gEfiPeiReadOnlyVariable2PpiGuid,
                             0,
                             NULL,
                                      (void **)&Variable
                             );
  if (!EFI_ERROR(Status)) {
    //
    // Get the memory ceiling
    //
    VariableSize = sizeof(MemoryCeiling);
    Status = Variable->GetVariable (
                         Variable,
                         MemoryCeilingVariable,
                         &gEfiGlobalVariableGuid,
                         NULL,
                         &VariableSize,
                         &MemoryCeiling
                         );
    if(!EFI_ERROR(Status)) {
      //
      // Set the new PCI32 resource Base if the variable available
      //
      PlatformInfoHob->PciData.PciResourceMem32Base = MemoryCeiling;
      PlatformInfoHob->MemData.MemMaxTolm = MemoryCeiling;
      PlatformInfoHob->MemData.MemTolm = MemoryCeiling;

      //
      // Platform PCI MMIO Size in unit of 1MB
      //
      PlatformInfoHob->MemData.MmioSize = 0x1000 - (UINT16)(PlatformInfoHob->MemData.MemMaxTolm >> 20);
    }
  }

  return EFI_SUCCESS;
}

/**
  Initialize the platform related info hob according to the
  pre-determine value or setup option

  @retval EFI_SUCCESS    Memory initialization completed successfully.
  @retval Others         All other error conditions encountered result in an ASSERT.
**/
EFI_STATUS
InitializePlatform (
  IN CONST EFI_PEI_SERVICES       **PeiServices,
  IN EFI_PLATFORM_INFO_HOB        *PlatformInfoHob,
  IN SYSTEM_CONFIGURATION         *SystemConfiguration
)
{
//
// -- cchew10 need to update here.
//
  return EFI_SUCCESS;
}

