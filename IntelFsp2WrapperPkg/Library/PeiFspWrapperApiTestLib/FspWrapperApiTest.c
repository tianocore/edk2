/** @file
  Provide FSP wrapper API test related function.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Guid/GuidHobFspEas.h>

/**
  Test the output of FSP API - FspMemoryInit.

  @param[in]  FspmUpdDataPtr Address pointer to the FSP_MEMORY_INIT_PARAMS structure.
  @param[in]  HobListPtr     Address of the HobList pointer.

  @return test result on output of FspMemoryInit API.
**/
EFI_STATUS
EFIAPI
TestFspMemoryInitApiOutput (
  IN  VOID        *FspmUpdDataPtr,
  IN  VOID        **HobListPtr
  )
{
  DEBUG_CODE_BEGIN ();
  EFI_PEI_HOB_POINTERS        Hob;

  Hob.Raw = (UINT8 *)(*(HobListPtr));
  while (TRUE) {
    if (END_OF_HOB_LIST(Hob) == TRUE) {
      DEBUG((DEBUG_INFO, "gFspBootLoaderTolumHobGuid not Found\n"));
      break;
    }
    if ((CompareGuid (&Hob.ResourceDescriptor->Owner, &gFspBootLoaderTolumHobGuid))) {
      DEBUG ((DEBUG_INFO, "gFspBootLoaderTolumHobGuid Found\n"));
      DEBUG ((DEBUG_INFO, "Fill Boot Loader reserved memory range with 0x5A for testing purpose\n"));
      SetMem ((VOID *)(UINTN)Hob.ResourceDescriptor->PhysicalStart, (UINTN)Hob.ResourceDescriptor->ResourceLength, 0x5A);
      break;
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }
  DEBUG_CODE_END ();

  return RETURN_SUCCESS;
}

/**
  Test the output of FSP API - TempRamExit.

  @param[in] TempRamExitParam    Address pointer to the TempRamExit parameters structure.

  @return test result on output of TempRamExit API.
**/
EFI_STATUS
EFIAPI
TestFspTempRamExitApiOutput (
  IN VOID         *TempRamExitParam
  )
{
  return RETURN_SUCCESS;
}

/**
  Test the output of FSP API - FspSiliconInit.

  @param[in] FspsUpdDataPtr Address pointer to the Silicon Init parameters structure.

  @return test result on output of FspSiliconInit API.
**/
EFI_STATUS
EFIAPI
TestFspSiliconInitApiOutput (
  IN  VOID        *FspsUpdDataPtr
  )
{
  return RETURN_SUCCESS;
}
