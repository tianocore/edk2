/** @file
  Sample to provide FSP hob process related function.

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/FspPlatformInfoLib.h>

#include <Guid/GuidHobFspEas.h>
#include <Guid/MemoryTypeInformation.h>
#include <Ppi/Capsule.h>

//
// Additional pages are used by DXE memory manager.
// It should be consistent between RetrieveRequiredMemorySize() and GetPeiMemSize()
//
#define PEI_ADDITIONAL_MEMORY_SIZE    (16 * EFI_PAGE_SIZE)

/**
  Get the mem size in memory type infromation table.

  @param[in] PeiServices  PEI Services table.

  @return the mem size in memory type infromation table.
**/
UINT64
GetMemorySizeInMemoryTypeInformation (
  IN EFI_PEI_SERVICES **PeiServices
  )
{
  EFI_STATUS                  Status;
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_MEMORY_TYPE_INFORMATION *MemoryData;
  UINT8                       Index;
  UINTN                       TempPageNum;

  MemoryData = NULL;
  Status     = (*PeiServices)->GetHobList ((CONST EFI_PEI_SERVICES**)PeiServices, (VOID **) &Hob.Raw);
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_GUID_EXTENSION &&
      CompareGuid (&Hob.Guid->Name, &gEfiMemoryTypeInformationGuid)) {
      MemoryData = (EFI_MEMORY_TYPE_INFORMATION *) (Hob.Raw + sizeof (EFI_HOB_GENERIC_HEADER) + sizeof (EFI_GUID));
      break;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  if (MemoryData == NULL) {
    return 0;
  }

  TempPageNum = 0;
  for (Index = 0; MemoryData[Index].Type != EfiMaxMemoryType; Index++) {
    //
    // Accumulate default memory size requirements
    //
    TempPageNum += MemoryData[Index].NumberOfPages;
  }

  return TempPageNum * EFI_PAGE_SIZE;
}

/**
  Get the mem size need to be reserved in PEI phase.

  @param[in] PeiServices  PEI Services table.

  @return the mem size need to be reserved in PEI phase.
**/
UINT64
RetrieveRequiredMemorySize (
  IN EFI_PEI_SERVICES **PeiServices
  )
{
  UINT64                      Size;

  Size = GetMemorySizeInMemoryTypeInformation (PeiServices);
  return Size + PEI_ADDITIONAL_MEMORY_SIZE;
}

/**
  Get the mem size need to be consumed and reserved in PEI phase.

  @param[in] PeiServices  PEI Services table.
  @param[in] BootMode     Current boot mode.

  @return the mem size need to be consumed and reserved in PEI phase.
**/
UINT64
GetPeiMemSize (
  IN EFI_PEI_SERVICES **PeiServices,
  IN UINT32           BootMode
  )
{
  UINT64                      Size;
  UINT64                      MinSize;

  if (BootMode == BOOT_IN_RECOVERY_MODE) {
    return PcdGet32 (PcdPeiRecoveryMinMemSize);
  }

  Size = GetMemorySizeInMemoryTypeInformation (PeiServices);

  if (BootMode == BOOT_ON_FLASH_UPDATE) {
    //
    // Maybe more size when in CapsuleUpdate phase ?
    //
    MinSize = PcdGet32 (PcdPeiMinMemSize);
  } else {
    MinSize = PcdGet32 (PcdPeiMinMemSize);
  }

  return MinSize + Size + PEI_ADDITIONAL_MEMORY_SIZE;
}

/**
  BIOS process FspBobList for Memory Resource Descriptor.

  @param[in] FspHobList  Pointer to the HOB data structure produced by FSP.

  @return If platform process the FSP hob list successfully.
**/
EFI_STATUS
EFIAPI
FspHobProcessForMemoryResource (
  IN VOID                 *FspHobList
  )
{
  EFI_PEI_HOB_POINTERS Hob;
  UINT64               LowMemorySize;
  UINT64               FspMemorySize;
  EFI_PHYSICAL_ADDRESS FspMemoryBase;
  UINT64               PeiMemSize;
  EFI_PHYSICAL_ADDRESS PeiMemBase;
  UINT64               S3PeiMemSize;
  EFI_PHYSICAL_ADDRESS S3PeiMemBase;
  BOOLEAN              FoundFspMemHob;
  EFI_STATUS           Status;
  EFI_BOOT_MODE        BootMode;
  EFI_PEI_CAPSULE_PPI  *Capsule;
  VOID                 *CapsuleBuffer;
  UINTN                CapsuleBufferLength;
  UINT64               RequiredMemSize;
  EFI_PEI_SERVICES     **PeiServices;

  PeiServices = (EFI_PEI_SERVICES **)GetPeiServicesTablePointer ();

  PeiServicesGetBootMode (&BootMode);

  PeiMemBase = 0;
  LowMemorySize = 0;
  FspMemorySize = 0;
  FspMemoryBase = 0;
  FoundFspMemHob = FALSE;

  //
  // Parse the hob list from fsp
  // Report all the resource hob except the memory between 1M and 4G
  //
  Hob.Raw = (UINT8 *)(UINTN)FspHobList;
  DEBUG((DEBUG_INFO, "FspHobList - 0x%x\n", FspHobList));

  while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw)) != NULL) {
    DEBUG((DEBUG_INFO, "\nResourceType: 0x%x\n", Hob.ResourceDescriptor->ResourceType));
    if ((Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) ||
        (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_MEMORY_RESERVED)) {
      DEBUG((DEBUG_INFO, "ResourceAttribute: 0x%x\n", Hob.ResourceDescriptor->ResourceAttribute));
      DEBUG((DEBUG_INFO, "PhysicalStart: 0x%x\n", Hob.ResourceDescriptor->PhysicalStart));
      DEBUG((DEBUG_INFO, "ResourceLength: 0x%x\n", Hob.ResourceDescriptor->ResourceLength));
      DEBUG((DEBUG_INFO, "Owner: %g\n\n", &Hob.ResourceDescriptor->Owner));
    }

    if ((Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY)  // Found the low memory length below 4G
        && (Hob.ResourceDescriptor->PhysicalStart >= BASE_1MB)
        && (Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength <= BASE_4GB)) {
        LowMemorySize += Hob.ResourceDescriptor->ResourceLength;
      Hob.Raw = GET_NEXT_HOB (Hob);
      continue;
    }

    if ((Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_MEMORY_RESERVED)  // Found the low memory length below 4G
        && (Hob.ResourceDescriptor->PhysicalStart >= BASE_1MB)
        && (Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength <= BASE_4GB)
        && (CompareGuid (&Hob.ResourceDescriptor->Owner, &gFspReservedMemoryResourceHobGuid))) {
      FoundFspMemHob = TRUE;
      FspMemoryBase = Hob.ResourceDescriptor->PhysicalStart;
      FspMemorySize = Hob.ResourceDescriptor->ResourceLength;
      DEBUG((DEBUG_INFO, "Find fsp mem hob, base 0x%x, len 0x%x\n", FspMemoryBase, FspMemorySize));
    }

    //
    // Report the resource hob
    //
    BuildResourceDescriptorHob (
      Hob.ResourceDescriptor->ResourceType,
      Hob.ResourceDescriptor->ResourceAttribute,
      Hob.ResourceDescriptor->PhysicalStart,
      Hob.ResourceDescriptor->ResourceLength
      );

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  if (!FoundFspMemHob) {
    DEBUG((DEBUG_INFO, "Didn't find the fsp used memory information.\n"));
    //ASSERT(FALSE);
  }

  DEBUG((DEBUG_INFO, "LowMemorySize: 0x%x.\n", LowMemorySize));
  DEBUG((DEBUG_INFO, "FspMemoryBase: 0x%x.\n", FspMemoryBase));
  DEBUG((DEBUG_INFO, "FspMemorySize: 0x%x.\n", FspMemorySize));

  if (BootMode == BOOT_ON_S3_RESUME) {
    BuildResourceDescriptorHob (
      EFI_RESOURCE_SYSTEM_MEMORY,
      (
         EFI_RESOURCE_ATTRIBUTE_PRESENT |
         EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
         // EFI_RESOURCE_ATTRIBUTE_TESTED |
         EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
      ),
      BASE_1MB,
      LowMemorySize
      );

    S3PeiMemBase = 0;
    S3PeiMemSize = 0;
    Status = GetS3MemoryInfo (&S3PeiMemBase, &S3PeiMemSize);
    ASSERT_EFI_ERROR (Status);
    DEBUG((DEBUG_INFO, "S3 memory %Xh - %Xh bytes\n", S3PeiMemBase, S3PeiMemSize));

    //
    // Make sure Stack and PeiMemory are not overlap
    //

    Status = PeiServicesInstallPeiMemory (
               S3PeiMemBase,
               S3PeiMemSize
               );
    ASSERT_EFI_ERROR (Status);
  } else {
    PeiMemSize = GetPeiMemSize (PeiServices, BootMode);
    DEBUG((DEBUG_INFO, "PEI memory size = %Xh bytes\n", PeiMemSize));

    //
    // Capsule mode
    //
    Capsule = NULL;
    CapsuleBuffer = NULL;
    CapsuleBufferLength = 0;
    if (BootMode == BOOT_ON_FLASH_UPDATE) {
      Status = PeiServicesLocatePpi (
                 &gEfiPeiCapsulePpiGuid,
                 0,
                 NULL,
                 (VOID **) &Capsule
                 );
      ASSERT_EFI_ERROR (Status);

      if (Status == EFI_SUCCESS) {
        //
        // Make sure Stack and CapsuleBuffer are not overlap
        //
        CapsuleBuffer = (VOID *)(UINTN)BASE_1MB;
        CapsuleBufferLength = (UINTN)(LowMemorySize - PeiMemSize);
        //
        // Call the Capsule PPI Coalesce function to coalesce the capsule data.
        //
        Status = Capsule->Coalesce (PeiServices, &CapsuleBuffer, &CapsuleBufferLength);
      }
    }

    RequiredMemSize = RetrieveRequiredMemorySize (PeiServices);
    DEBUG((DEBUG_INFO, "Required memory size = %Xh bytes\n", RequiredMemSize));

    //
    // Report the main memory
    //
    BuildResourceDescriptorHob (
      EFI_RESOURCE_SYSTEM_MEMORY,
      (
         EFI_RESOURCE_ATTRIBUTE_PRESENT |
         EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
         EFI_RESOURCE_ATTRIBUTE_TESTED |
         EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
      ),
      BASE_1MB,
      LowMemorySize
      );

    //
    // Make sure Stack and CapsuleBuffer are not overlap
    //

    //
    // Install efi memory
    //
    PeiMemBase = BASE_1MB + LowMemorySize - PeiMemSize;
    Status = PeiServicesInstallPeiMemory (
               PeiMemBase,
               PeiMemSize - RequiredMemSize
               );
    ASSERT_EFI_ERROR (Status);

    if (Capsule != NULL) {
      Status = Capsule->CreateState (PeiServices, CapsuleBuffer, CapsuleBufferLength);
    }
  }

  return EFI_SUCCESS;
}

/**
  BIOS process FspBobList for other data (not Memory Resource Descriptor).

  @param[in] FspHobList  Pointer to the HOB data structure produced by FSP.

  @return If platform process the FSP hob list successfully.
**/
EFI_STATUS
EFIAPI
FspHobProcessForOtherData (
  IN VOID                 *FspHobList
  )
{
  return EFI_SUCCESS;
}

/**
  BIOS process FspBobList.

  @param[in] FspHobList  Pointer to the HOB data structure produced by FSP.

  @return If platform process the FSP hob list successfully.
**/
EFI_STATUS
EFIAPI
FspHobProcess (
  IN VOID                 *FspHobList
  )
{
  EFI_STATUS  Status;

  Status = FspHobProcessForMemoryResource (FspHobList);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = FspHobProcessForOtherData (FspHobList);

  return Status;
}
