/** @file
  This is the driver that produce AcpiVariable hob and slit SmramReserve hob
  for ECP platform.

Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Guid/SmramMemoryReserve.h>
#include <Guid/AcpiS3Context.h>

#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseMemoryLib.h>

/**
  Retrieves the data structure associated witht he GUIDed HOB of type gEfiSmmPeiSmramMemoryReserveGuid
  
  @retval NULL   A HOB of type gEfiSmmPeiSmramMemoryReserveGuid could not be found.
  @retval !NULL  A pointer to the GUID data from a HIB of type gEfiSmmPeiSmramMemoryReserveGuid

**/
EFI_SMRAM_HOB_DESCRIPTOR_BLOCK *
GetSrmamHobData (
  VOID
  )
{
  VOID  *GuidHob;

  //
  // Search SmramMemoryReserve HOB that describes SMRAM region
  //
  GuidHob = GetFirstGuidHob (&gEfiSmmPeiSmramMemoryReserveGuid);
  if (GuidHob == NULL) {
    return NULL;
  }
  return (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK *)GET_GUID_HOB_DATA (GuidHob);
}

/**
  This routine will split SmramReserve hob to reserve 1 page for SMRAM content in S3 phase
  for PI SMM core.
  
  @retval EFI_SUCCESS           The gEfiSmmPeiSmramMemoryReserveGuid is splited successfully.
  @retval EFI_NOT_FOUND         The gEfiSmmPeiSmramMemoryReserveGuid is not found.

**/
EFI_STATUS
EFIAPI
SplitSmramReserveHob (
  VOID
  )
{
  EFI_HOB_GUID_TYPE                *GuidHob;
  EFI_PEI_HOB_POINTERS             Hob;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK   *DescriptorBlock;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK   *NewDescriptorBlock;
  UINTN                            BufferSize;
  UINTN                            SmramRanges;
  UINTN                            Index;
  UINTN                            SubIndex;

  //
  // Retrieve the GUID HOB data that contains the set of SMRAM descriptyors
  //
  GuidHob = GetFirstGuidHob (&gEfiSmmPeiSmramMemoryReserveGuid);
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }

  DescriptorBlock = (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK *)GET_GUID_HOB_DATA (GuidHob);

  //
  // Allocate one extra EFI_SMRAM_DESCRIPTOR to describe a page of SMRAM memory that contains a pointer
  // to the SMM Services Table that is required on the S3 resume path
  //
  SmramRanges = DescriptorBlock->NumberOfSmmReservedRegions;
  BufferSize = sizeof (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK) + (SmramRanges * sizeof (EFI_SMRAM_DESCRIPTOR));

  Hob.Raw = BuildGuidHob (
              &gEfiSmmPeiSmramMemoryReserveGuid,
              BufferSize
              );
  ASSERT (Hob.Raw);
  NewDescriptorBlock = (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK *)Hob.Raw;

  //
  // Copy old EFI_SMRAM_HOB_DESCRIPTOR_BLOCK to new allocated region
  //
  CopyMem ((VOID *)Hob.Raw, DescriptorBlock, BufferSize - sizeof(EFI_SMRAM_DESCRIPTOR));

  //
  // Increase the number of SMRAM descriptors by 1 to make room for the ALLOCATED descriptor of size EFI_PAGE_SIZE
  //
  NewDescriptorBlock->NumberOfSmmReservedRegions = (UINT32)(SmramRanges + 1);

  ASSERT (SmramRanges >= 1);
  //
  // Copy last entry to the end - we assume TSEG is last entry, which is same assumption as Framework CPU/SMM driver
  //
  CopyMem (&NewDescriptorBlock->Descriptor[SmramRanges], &NewDescriptorBlock->Descriptor[SmramRanges - 1], sizeof(EFI_SMRAM_DESCRIPTOR));

  //
  // Update the last but 1 entry in the array with a size of EFI_PAGE_SIZE and put into the ALLOCATED state
  //
  NewDescriptorBlock->Descriptor[SmramRanges - 1].PhysicalSize    = EFI_PAGE_SIZE;
  NewDescriptorBlock->Descriptor[SmramRanges - 1].RegionState    |= EFI_ALLOCATED;

  //
  // Reduce the size of the last SMRAM descriptor by EFI_PAGE_SIZE 
  //
  NewDescriptorBlock->Descriptor[SmramRanges].PhysicalStart += EFI_PAGE_SIZE;
  NewDescriptorBlock->Descriptor[SmramRanges].CpuStart      += EFI_PAGE_SIZE;
  NewDescriptorBlock->Descriptor[SmramRanges].PhysicalSize  -= EFI_PAGE_SIZE;

  //
  // Now, we have created SmramReserve Hob for SmmAccess drive. But the issue is that, Framework SmmAccess will assume there is 2 SmramReserve region only.
  // Reporting 3 SmramReserve region will cause buffer overflow. Moreover, we would like to filter AB-SEG or H-SEG to avoid SMM cache-poisoning issue.
  // So we uses scan SmmReserve Hob to remove AB-SEG or H-SEG.
  //
  for (Index = 0; Index <= SmramRanges; Index++) {
    if (NewDescriptorBlock->Descriptor[Index].PhysicalSize == 0) {
      //
      // Skip zero entry
      //
      continue;
    }
    if (NewDescriptorBlock->Descriptor[Index].PhysicalStart < BASE_1MB) {
      //
      // Find AB-SEG or H-SEG
      // remove this region
      //
      for (SubIndex = Index; SubIndex < NewDescriptorBlock->NumberOfSmmReservedRegions - 1; SubIndex++) {
        CopyMem (&NewDescriptorBlock->Descriptor[SubIndex], &NewDescriptorBlock->Descriptor[SubIndex + 1], sizeof (EFI_SMRAM_DESCRIPTOR));
      }
      //
      // Zero last one
      //
      ZeroMem (&NewDescriptorBlock->Descriptor[SubIndex], sizeof(EFI_SMRAM_DESCRIPTOR));
      //
      // Decrease Number
      //
      NewDescriptorBlock->NumberOfSmmReservedRegions --;
      //
      // Decrease Index to let it test mew entry
      //
      Index --;
    }
  }

  //
  // Last step, we can scrub old one
  //
  ZeroMem (&GuidHob->Name, sizeof(GuidHob->Name));

  return EFI_SUCCESS;
}

/**
  This routine will create AcpiVariable hob to point the reserved smram in S3 phase
  for PI SMM core.
  
  @retval EFI_SUCCESS           The gEfiAcpiVariableGuid is created successfully.
  @retval EFI_NOT_FOUND         The gEfiSmmPeiSmramMemoryReserveGuid is not found.

**/
EFI_STATUS
EFIAPI
CreateAcpiVariableHob (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS             Hob;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK   *DescriptorBlock;
  UINTN                            SmramRanges;

  //
  // Retrieve the GUID HOB data that contains the set of SMRAM descriptyors
  //
  DescriptorBlock = GetSrmamHobData ();
  if (DescriptorBlock == NULL) {
    return EFI_NOT_FOUND;
  }

  Hob.Raw = BuildGuidHob (
              &gEfiAcpiVariableGuid,
              sizeof (EFI_SMRAM_DESCRIPTOR)
              );
  ASSERT (Hob.Raw);

  //
  // It should be already patch, so just copy last but 1 region directly.
  //
  SmramRanges = DescriptorBlock->NumberOfSmmReservedRegions;
  ASSERT (SmramRanges >= 2);
  if (SmramRanges >= 2) {
    CopyMem ((VOID *)Hob.Raw, &DescriptorBlock->Descriptor[SmramRanges - 2], sizeof (EFI_SMRAM_DESCRIPTOR));
  }

  return EFI_SUCCESS;
}

/**
  Driver Entry for AcpiVariableHobOnSmramReservHob PEIM
  
  @param   FileHandle       Handle of the file being invoked.
  @param   PeiServices      Describes the list of possible PEI Services.
  
  @retval EFI_SUCCESS      Success create gEfiAcpiVariableGuid and
                           split gEfiSmmPeiSmramMemoryReserveGuid.
  @retval EFI_NOT_FOUND    Can not get gEfiSmmPeiSmramMemoryReserveGuid hob

**/
EFI_STATUS
EFIAPI
AcpiVariableHobEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS              Status;

  //
  // Split SmramReserve hob, which is required for PI SMM Core for S3.
  //
  Status = SplitSmramReserveHob ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Create AcpiVariable hob, which is required for PI SMM Core for S3.
  //
  Status = CreateAcpiVariableHob ();

  return Status;
}
