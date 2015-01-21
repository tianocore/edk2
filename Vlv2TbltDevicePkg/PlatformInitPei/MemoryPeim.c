/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:


  MemoryPeim.c

Abstract:

  Tiano PEIM to provide the platform support functionality.
  This file implements the Platform Memory Range PPI

--*/

#include "PlatformEarlyInit.h"

//
// Need min. of 48MB PEI phase
//
#define  PEI_MIN_MEMORY_SIZE               (6 * 0x800000)
#define  PEI_RECOVERY_MIN_MEMORY_SIZE      (6 * 0x800000)

//
// This is the memory needed for PEI to start up DXE.
//
// Over-estimating this size will lead to higher fragmentation
// of main memory.  Under-estimation of this will cause catastrophic
// failure of PEI to load DXE.  Generally, the failure may only be
// realized during capsule updates.
//
#define PRERESERVED_PEI_MEMORY ( \
  EFI_SIZE_TO_PAGES (3 * 0x800000)   /* PEI Core memory based stack          */ \
  )

EFI_MEMORY_TYPE_INFORMATION mDefaultMemoryTypeInformation[] = {
  { EfiACPIReclaimMemory,       0x40  },    // 0x40 pages = 256k for ASL
  { EfiACPIMemoryNVS,           0x100 },    // 0x100 pages = 1 MB for S3, SMM, HII, etc
  { EfiReservedMemoryType,      0x600 },    // 48k for BIOS Reserved
  { EfiMemoryMappedIO,          0     },
  { EfiMemoryMappedIOPortSpace, 0     },
  { EfiPalCode,                 0     },
  { EfiRuntimeServicesCode,     0x200 },
  { EfiRuntimeServicesData,     0x100 },
  { EfiLoaderCode,              0x100 },
  { EfiLoaderData,              0x100 },
  { EfiBootServicesCode,        0x800 },
  { EfiBootServicesData,        0x2500},
  { EfiConventionalMemory,      0     },
  { EfiUnusableMemory,          0     },
  { EfiMaxMemoryType,           0     }
};

STATIC
EFI_STATUS
GetMemorySize (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  OUT UINT64              *LowMemoryLength,
  OUT UINT64              *HighMemoryLength
  );



EFI_STATUS
EFIAPI
SetPeiCacheMode (
  IN  CONST EFI_PEI_SERVICES    **PeiServices
  )
{
  EFI_STATUS              Status;
  PEI_CACHE_PPI           *CachePpi;

  EFI_BOOT_MODE           BootMode;
  UINT64                  MemoryLength;
  UINT64                  MemOverflow;
  UINT64                  MemoryLengthUc;
  UINT64                  MaxMemoryLength;
  UINT64                  LowMemoryLength;
  UINT64                  HighMemoryLength;
  UINT8                   Index;
  MTRR_SETTINGS           MtrrSetting;

  //
  // Load Cache PPI
  //
  Status = (**PeiServices).LocatePpi (
             PeiServices,
             &gPeiCachePpiGuid,    // GUID
             0,                    // Instance
             NULL,                 // EFI_PEI_PPI_DESCRIPTOR
             (void **)&CachePpi             // PPI
             );
  if (!EFI_ERROR(Status)) {
    //
    // Clear the CAR Settings (Default Cache Type => UC)
    //
    DEBUG ((EFI_D_INFO, "Reset cache attribute and disable CAR. \n"));
    CachePpi->ResetCache(
                (EFI_PEI_SERVICES**)PeiServices,
                CachePpi
                );
 }


  //
  // Variable initialization
  //
  LowMemoryLength = 0;
  HighMemoryLength = 0;
  MemoryLengthUc = 0;

  Status = (*PeiServices)->GetBootMode (
                             PeiServices,
                             &BootMode
                             );

  //
  // Determine memory usage
  //
  GetMemorySize (
    PeiServices,
    &LowMemoryLength,
    &HighMemoryLength
    );

  LowMemoryLength  = (EFI_PHYSICAL_ADDRESS)MmPci32( 0, 0, 2, 0, 0x70);
  LowMemoryLength   &=  0xFFF00000ULL;

  MaxMemoryLength = LowMemoryLength;

  //
  // Round up to nearest 256MB with high memory and 64MB w/o high memory
  //
  if (HighMemoryLength != 0 ) {
    MemOverflow = (LowMemoryLength & 0x0fffffff);
    if (MemOverflow != 0) {
      MaxMemoryLength = LowMemoryLength + (0x10000000 - MemOverflow);
    }
  } else {
    MemOverflow = (LowMemoryLength & 0x03ffffff);
    if (MemOverflow != 0) {
      MaxMemoryLength = LowMemoryLength + (0x4000000 - MemOverflow);
    }
  }

  ZeroMem (&MtrrSetting, sizeof(MTRR_SETTINGS));
  for (Index = 0; Index < 2; Index++) {
    MtrrSetting.Fixed.Mtrr[Index]=0x0606060606060606;
   }
  for (Index = 2; Index < 11; Index++) {
    MtrrSetting.Fixed.Mtrr[Index]=0x0505050505050505;
   }

  //
  // Cache the flash area to improve the boot performance in PEI phase
  //
  Index = 0;
  MtrrSetting.Variables.Mtrr[0].Base = (FixedPcdGet32 (PcdFlashAreaBaseAddress) | CacheWriteProtected);
  MtrrSetting.Variables.Mtrr[0].Mask = ((~((UINT64)(FixedPcdGet32 (PcdFlashAreaSize) - 1))) & MTRR_LIB_CACHE_VALID_ADDRESS) | MTRR_LIB_CACHE_MTRR_ENABLED;
  Index ++;

  MemOverflow =0;
  while (MaxMemoryLength > MemOverflow){
    MtrrSetting.Variables.Mtrr[Index].Base = (MemOverflow & MTRR_LIB_CACHE_VALID_ADDRESS) | CacheWriteBack;
    MemoryLength = MaxMemoryLength - MemOverflow;
    MemoryLength = GetPowerOfTwo64 (MemoryLength);
    MtrrSetting.Variables.Mtrr[Index].Mask = ((~(MemoryLength - 1)) & MTRR_LIB_CACHE_VALID_ADDRESS) | MTRR_LIB_CACHE_MTRR_ENABLED;

    MemOverflow += MemoryLength;
    Index++;
  }

  MemoryLength = LowMemoryLength;

  while (MaxMemoryLength != MemoryLength) {
    MemoryLengthUc = GetPowerOfTwo64 (MaxMemoryLength - MemoryLength);

    MtrrSetting.Variables.Mtrr[Index].Base = ((MaxMemoryLength - MemoryLengthUc) & MTRR_LIB_CACHE_VALID_ADDRESS) | CacheUncacheable;
    MtrrSetting.Variables.Mtrr[Index].Mask= ((~(MemoryLengthUc   - 1)) & MTRR_LIB_CACHE_VALID_ADDRESS) | MTRR_LIB_CACHE_MTRR_ENABLED;
    MaxMemoryLength -= MemoryLengthUc;
    Index++;
  }

  MemOverflow =0x100000000;
  while (HighMemoryLength > 0) {
    MtrrSetting.Variables.Mtrr[Index].Base = (MemOverflow & MTRR_LIB_CACHE_VALID_ADDRESS) | CacheWriteBack;
    MemoryLength = HighMemoryLength;
    MemoryLength = GetPowerOfTwo64 (MemoryLength);

    if (MemoryLength > MemOverflow){
      MemoryLength = MemOverflow;
    }

    MtrrSetting.Variables.Mtrr[Index].Mask = ((~(MemoryLength - 1)) & MTRR_LIB_CACHE_VALID_ADDRESS) | MTRR_LIB_CACHE_MTRR_ENABLED;

    MemOverflow += MemoryLength;
    HighMemoryLength -= MemoryLength;
    Index++;
  }


  for (Index = 0; Index < MTRR_NUMBER_OF_VARIABLE_MTRR; Index++) {
    if (MtrrSetting.Variables.Mtrr[Index].Base == 0){
      break;
    }
    DEBUG ((EFI_D_INFO, "Base=%lx, Mask=%lx\n",MtrrSetting.Variables.Mtrr[Index].Base ,MtrrSetting.Variables.Mtrr[Index].Mask));
  }

  //
  // set FE/E bits for IA32_MTRR_DEF_TYPE
  //
  MtrrSetting.MtrrDefType |=  3 <<10;

  MtrrSetAllMtrrs(&MtrrSetting);



  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SetDxeCacheMode (
  IN  CONST EFI_PEI_SERVICES    **PeiServices
  )
{
  //
  // This is not needed for now.
  //
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetMemorySize (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  OUT UINT64              *LowMemoryLength,
  OUT UINT64              *HighMemoryLength
  )
{
  EFI_STATUS              Status;
  EFI_PEI_HOB_POINTERS    Hob;

  *HighMemoryLength = 0;
  *LowMemoryLength = 0x100000;

  //
  // Get the HOB list for processing
  //
  Status = (*PeiServices)->GetHobList (PeiServices, (void **)&Hob.Raw);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // Collect memory ranges
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
        //
        // Need memory above 1MB to be collected here
        //
        if (Hob.ResourceDescriptor->PhysicalStart >= 0x100000 &&
            Hob.ResourceDescriptor->PhysicalStart < (EFI_PHYSICAL_ADDRESS) 0x100000000) {
          *LowMemoryLength += (UINT64) (Hob.ResourceDescriptor->ResourceLength);
        } else if (Hob.ResourceDescriptor->PhysicalStart >= (EFI_PHYSICAL_ADDRESS) 0x100000000) {
          *HighMemoryLength += (UINT64) (Hob.ResourceDescriptor->ResourceLength);
        }
      }
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  return EFI_SUCCESS;
}


/**
  Publish Memory Type Information.

  @param  NULL

  @retval EFI_SUCCESS    Success.
  @retval Others         Errors have occurred.
**/

EFI_STATUS
EFIAPI
PublishMemoryTypeInfo (
  void
  )
{
  EFI_STATUS                      Status;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI *Variable;
  UINTN                           DataSize;
  EFI_MEMORY_TYPE_INFORMATION     MemoryData[EfiMaxMemoryType + 1];

  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
            (void **)&Variable
             );
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "WARNING: Locating Pei variable failed 0x%x \n", Status));
    DEBUG((EFI_D_ERROR, "Build Hob from default\n"));
    //
    // Build the default GUID'd HOB for DXE
    //
    BuildGuidDataHob (
      &gEfiMemoryTypeInformationGuid,
      mDefaultMemoryTypeInformation,
      sizeof (mDefaultMemoryTypeInformation)
      );

    return Status;
  }


  DataSize = sizeof (MemoryData);

  //
  // This variable is saved in BDS stage. Now read it back
  //
  Status = Variable->GetVariable (
                       Variable,
                       EFI_MEMORY_TYPE_INFORMATION_VARIABLE_NAME,
                       &gEfiMemoryTypeInformationGuid,
                       NULL,
                       &DataSize,
                       &MemoryData
                       );
  if (EFI_ERROR (Status)) {
  	//
    //build default
    //
    DEBUG((EFI_D_ERROR, "Build Hob from default\n"));
    BuildGuidDataHob (
      &gEfiMemoryTypeInformationGuid,
      mDefaultMemoryTypeInformation,
      sizeof (mDefaultMemoryTypeInformation)
      );

  } else {
  	//
    // Build the GUID'd HOB for DXE from variable
    //
    DEBUG((EFI_D_ERROR, "Build Hob from variable \n"));
    BuildGuidDataHob (
      &gEfiMemoryTypeInformationGuid,
      MemoryData,
      DataSize
      );
  }

  return Status;
}

