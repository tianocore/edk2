/** @file
  EFI PEI Core dispatch services
  
Copyright (c) 2006 - 2010, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PeiMain.h"

///
/// temporary memory is filled with this initial value during SEC phase
///
#define INIT_CAR_VALUE 0x5AA55AA5

typedef struct {
  EFI_STATUS_CODE_DATA  DataHeader;
  EFI_HANDLE            Handle;
} PEIM_FILE_HANDLE_EXTENDED_DATA;

/**

  Discover all Peims and optional Apriori file in one FV. There is at most one
  Apriori file in one FV.


  @param Private          Pointer to the private data passed in from caller
  @param CoreFileHandle   The instance of PEI_CORE_FV_HANDLE.

**/
VOID
DiscoverPeimsAndOrderWithApriori (
  IN  PEI_CORE_INSTANCE    *Private,
  IN  PEI_CORE_FV_HANDLE   *CoreFileHandle
  )
{
  EFI_STATUS                          Status;
  EFI_PEI_FV_HANDLE                   FileHandle;
  EFI_PEI_FILE_HANDLE                 AprioriFileHandle;
  EFI_GUID                            *Apriori;
  UINTN                               Index;
  UINTN                               Index2;
  UINTN                               PeimIndex;
  UINTN                               PeimCount;
  EFI_GUID                            *Guid;
  EFI_PEI_FV_HANDLE                   TempFileHandles[FixedPcdGet32 (PcdPeiCoreMaxPeimPerFv)];
  EFI_GUID                            FileGuid[FixedPcdGet32 (PcdPeiCoreMaxPeimPerFv)];
  EFI_PEI_FIRMWARE_VOLUME_PPI         *FvPpi;
  EFI_FV_FILE_INFO                    FileInfo;
  
  FvPpi = CoreFileHandle->FvPpi;
  
  //
  // Walk the FV and find all the PEIMs and the Apriori file.
  //
  AprioriFileHandle = NULL;
  Private->CurrentFvFileHandles[0] = NULL;
  Guid = NULL;
  FileHandle = NULL;

  //
  // If the current Fv has been scanned, directly get its cachable record.
  //
  if (Private->Fv[Private->CurrentPeimFvCount].ScanFv) {
    CopyMem (Private->CurrentFvFileHandles, Private->Fv[Private->CurrentPeimFvCount].FvFileHandles, sizeof (Private->CurrentFvFileHandles));
    return;
  }

  //
  // Go ahead to scan this Fv, and cache FileHandles within it.
  //
  for (PeimCount = 0; PeimCount < PcdGet32 (PcdPeiCoreMaxPeimPerFv); PeimCount++) {
    Status = FvPpi->FindFileByType (FvPpi, PEI_CORE_INTERNAL_FFS_FILE_DISPATCH_TYPE, CoreFileHandle->FvHandle, &FileHandle);
    if (Status != EFI_SUCCESS) {
      break;
    }

    Private->CurrentFvFileHandles[PeimCount] = FileHandle;
  }
  
  //
  // Check whether the count of Peims exceeds the max support PEIMs in a FV image
  // If more Peims are required in a FV image, PcdPeiCoreMaxPeimPerFv can be set to a larger value in DSC file.
  //
  ASSERT (PeimCount < PcdGet32 (PcdPeiCoreMaxPeimPerFv));

  //
  // Get Apriori File handle
  //
  Private->AprioriCount = 0;
  Status = FvPpi->FindFileByName (FvPpi, &gPeiAprioriFileNameGuid, &CoreFileHandle->FvHandle, &AprioriFileHandle);
  if (!EFI_ERROR(Status) && AprioriFileHandle != NULL) {
    //
    // Read the Apriori file
    //
    Status = FvPpi->FindSectionByType (FvPpi, EFI_SECTION_RAW, AprioriFileHandle, (VOID **) &Apriori);
    if (!EFI_ERROR (Status)) {
      //
      // Calculate the number of PEIMs in the A Priori list
      //
      Status = FvPpi->GetFileInfo (FvPpi, AprioriFileHandle, &FileInfo);
      ASSERT_EFI_ERROR (Status);
      Private->AprioriCount = FileInfo.BufferSize & 0x00FFFFFF;
      Private->AprioriCount -= sizeof (EFI_COMMON_SECTION_HEADER);
      Private->AprioriCount /= sizeof (EFI_GUID);

      ZeroMem (FileGuid, sizeof (FileGuid));
      for (Index = 0; Index < PeimCount; Index++) {
        //
        // Make an array of file name guids that matches the FileHandle array so we can convert
        // quickly from file name to file handle
        //
        Status = FvPpi->GetFileInfo (FvPpi, Private->CurrentFvFileHandles[Index], &FileInfo);
        CopyMem (&FileGuid[Index], &FileInfo.FileName, sizeof(EFI_GUID));
      }

      //
      // Walk through FileGuid array to find out who is invalid PEIM guid in Apriori file.
      // Add avalible PEIMs in Apriori file into TempFileHandles array at first.
      //
      Index2 = 0;
      for (Index = 0; Index2 < Private->AprioriCount; Index++) {
        while (Index2 < Private->AprioriCount) {
          Guid = ScanGuid (FileGuid, PeimCount * sizeof (EFI_GUID), &Apriori[Index2++]);
          if (Guid != NULL) {
            break;
          }
        }
        if (Guid == NULL) {
          break;
        }
        PeimIndex = ((UINTN)Guid - (UINTN)&FileGuid[0])/sizeof (EFI_GUID);
        TempFileHandles[Index] = Private->CurrentFvFileHandles[PeimIndex];

        //
        // Since we have copied the file handle we can remove it from this list.
        //
        Private->CurrentFvFileHandles[PeimIndex] = NULL;
      }

      //
      // Update valid Aprioricount
      //
      Private->AprioriCount = Index;

      //
      // Add in any PEIMs not in the Apriori file
      //
      for (;Index < PeimCount; Index++) {
        for (Index2 = 0; Index2 < PeimCount; Index2++) {
          if (Private->CurrentFvFileHandles[Index2] != NULL) {
            TempFileHandles[Index] = Private->CurrentFvFileHandles[Index2];
            Private->CurrentFvFileHandles[Index2] = NULL;
            break;
          }
        }
      }
      //
      //Index the end of array contains re-range Pei moudle.
      //
      TempFileHandles[Index] = NULL;

      //
      // Private->CurrentFvFileHandles is currently in PEIM in the FV order.
      // We need to update it to start with files in the A Priori list and
      // then the remaining files in PEIM order.
      //
      CopyMem (Private->CurrentFvFileHandles, TempFileHandles, sizeof (Private->CurrentFvFileHandles));
    }
  }
  //
  // Cache the current Fv File Handle. So that we don't have to scan the Fv again.
  // Instead, we can retrieve the file handles within this Fv from cachable data.
  //
  Private->Fv[Private->CurrentPeimFvCount].ScanFv = TRUE;
  CopyMem (Private->Fv[Private->CurrentPeimFvCount].FvFileHandles, Private->CurrentFvFileHandles, sizeof (Private->CurrentFvFileHandles));

}

/**
  Shadow PeiCore module from flash to installed memory.
  
  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param PrivateInMem    PeiCore's private data structure

  @return PeiCore function address after shadowing.
**/
VOID*
ShadowPeiCore(
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN       PEI_CORE_INSTANCE    *PrivateInMem
  )
{
  EFI_PEI_FILE_HANDLE  PeiCoreFileHandle;
  EFI_PHYSICAL_ADDRESS EntryPoint;
  EFI_STATUS           Status;
  UINT32               AuthenticationState;

  PeiCoreFileHandle = NULL;

  //
  // Find the PEI Core in the BFV
  //
  Status = PrivateInMem->Fv[0].FvPpi->FindFileByType (
                                        PrivateInMem->Fv[0].FvPpi,
                                        EFI_FV_FILETYPE_PEI_CORE,
                                        PrivateInMem->Fv[0].FvHandle,
                                        &PeiCoreFileHandle
                                        );
  ASSERT_EFI_ERROR (Status);

  //
  // Shadow PEI Core into memory so it will run faster
  //
  Status = PeiLoadImage (
              PeiServices,
              *((EFI_PEI_FILE_HANDLE*)&PeiCoreFileHandle),
              PEIM_STATE_REGISITER_FOR_SHADOW,
              &EntryPoint,
              &AuthenticationState
              );
  ASSERT_EFI_ERROR (Status);

  //
  // Compute the PeiCore's function address after shaowed PeiCore.
  // _ModuleEntryPoint is PeiCore main function entry
  //
  return (VOID*) ((UINTN) EntryPoint + (UINTN) PeiCore - (UINTN) _ModuleEntryPoint);
}
//
// This is the minimum memory required by DxeCore initialization. When LMFA feature enabled,
// This part of memory still need reserved on the very top of memory so that the DXE Core could  
// use these memory for data initialization. This macro should be sync with the same marco
// defined in DXE Core.
//
#define MINIMUM_INITIAL_MEMORY_SIZE 0x10000
/**
  Hook function for Loading Module at Fixed Address feature
  
  This function should only be invoked when Loading Module at Fixed Address(LMFA) feature is enabled. When feature is
  configured as Load Modules at Fix Absolute Address, this function is to validate the top address assigned by user. When 
  feature is configured as Load Modules at Fixed Offset, the functino is to find the top address which is TOLM-TSEG in general.  
  And also the function will re-install PEI memory. 

  @param PrivateData         Pointer to the private data passed in from caller

**/
VOID
PeiLoadFixAddressHook(
  IN PEI_CORE_INSTANCE           *PrivateData
  )
{
  EFI_PHYSICAL_ADDRESS               TopLoadingAddress;
  UINT64                             PeiMemorySize;
  UINT64                             TotalReservedMemorySize;
  UINT64                             MemoryRangeEnd;
  EFI_PHYSICAL_ADDRESS               HighAddress; 
  EFI_HOB_RESOURCE_DESCRIPTOR        *ResourceHob;
  EFI_HOB_RESOURCE_DESCRIPTOR        *NextResourceHob;
  EFI_HOB_RESOURCE_DESCRIPTOR        *CurrentResourceHob;
  EFI_PEI_HOB_POINTERS               CurrentHob;
  EFI_PEI_HOB_POINTERS               Hob;
  EFI_PEI_HOB_POINTERS               NextHob;
  EFI_PHYSICAL_ADDRESS               MaxMemoryBaseAddress;
  UINT64                             MaxMemoryLength;
  //
  // Initialize Local Variables
  //
  CurrentResourceHob    = NULL;
  ResourceHob           = NULL;
  NextResourceHob       = NULL;
  MaxMemoryBaseAddress  = 0;
  MaxMemoryLength       = 0;
  HighAddress           = 0;
  TopLoadingAddress     = 0;
  MemoryRangeEnd      = 0;
  CurrentHob.Raw      = PrivateData->HobList.Raw;
  PeiMemorySize = PrivateData->PhysicalMemoryLength;
  //
  // The top reserved memory include 3 parts: the topest range is for DXE core initialization with the size  MINIMUM_INITIAL_MEMORY_SIZE
  // then RuntimeCodePage range and Boot time code range.
  //  
  TotalReservedMemorySize = MINIMUM_INITIAL_MEMORY_SIZE + EFI_PAGES_TO_SIZE(PcdGet32(PcdLoadFixAddressRuntimeCodePageNumber));
  TotalReservedMemorySize+= EFI_PAGES_TO_SIZE(PcdGet32(PcdLoadFixAddressBootTimeCodePageNumber)) ;  
  //
  // PEI memory range lies below the top reserved memory
  // 
  TotalReservedMemorySize += PeiMemorySize;
  
  DEBUG ((EFI_D_INFO, "LOADING MODULE FIXED INFO: PcdLoadFixAddressRuntimeCodePageNumber= %x.\n", PcdGet32(PcdLoadFixAddressRuntimeCodePageNumber)));
  DEBUG ((EFI_D_INFO, "LOADING MODULE FIXED INFO: PcdLoadFixAddressBootTimeCodePageNumber= %x.\n", PcdGet32(PcdLoadFixAddressBootTimeCodePageNumber)));
  DEBUG ((EFI_D_INFO, "LOADING MODULE FIXED INFO: PcdLoadFixAddressPeiCodePageNumber= %x.\n", PcdGet32(PcdLoadFixAddressPeiCodePageNumber)));   
  DEBUG ((EFI_D_INFO, "LOADING MODULE FIXED INFO: Total Reserved Memory Size = %lx.\n", TotalReservedMemorySize));
  //
  // Loop through the system memory typed hob to merge the adjacent memory range 
  //
  for (Hob.Raw = PrivateData->HobList.Raw; !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB(Hob)) {
    //                                                              
    // See if this is a resource descriptor HOB                     
    //
    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      
      ResourceHob = Hob.ResourceDescriptor;  
      //
      // If range described in this hob is not system memory or heigher than MAX_ADDRESS, ignored.
      //
      if (ResourceHob->ResourceType != EFI_RESOURCE_SYSTEM_MEMORY &&
          ResourceHob->PhysicalStart + ResourceHob->ResourceLength > MAX_ADDRESS)   {
        continue;
      }   
      
      for (NextHob.Raw = PrivateData->HobList.Raw; !END_OF_HOB_LIST(NextHob); NextHob.Raw = GET_NEXT_HOB(NextHob)) {       
        if (NextHob.Raw == Hob.Raw){
          continue;
        }  
        //
        // See if this is a resource descriptor HOB
        //
        if (GET_HOB_TYPE (NextHob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      
          NextResourceHob = NextHob.ResourceDescriptor;
          //
          // test if range described in this NextResourceHob is system memory and have the same attribute.
          // Note: Here is a assumption that system memory should always be healthy even without test.
          //    
          if (NextResourceHob->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY &&
             (((NextResourceHob->ResourceAttribute^ResourceHob->ResourceAttribute)&(~EFI_RESOURCE_ATTRIBUTE_TESTED)) == 0)){
              
              //
              // See if the memory range described in ResourceHob and NextResourceHob is adjacent
              //
              if ((ResourceHob->PhysicalStart <= NextResourceHob->PhysicalStart && 
                    ResourceHob->PhysicalStart + ResourceHob->ResourceLength >= NextResourceHob->PhysicalStart)|| 
                  (ResourceHob->PhysicalStart >= NextResourceHob->PhysicalStart&&
                     ResourceHob->PhysicalStart <= NextResourceHob->PhysicalStart + NextResourceHob->ResourceLength)) {
             
                MemoryRangeEnd = ((ResourceHob->PhysicalStart + ResourceHob->ResourceLength)>(NextResourceHob->PhysicalStart + NextResourceHob->ResourceLength)) ?
                                     (ResourceHob->PhysicalStart + ResourceHob->ResourceLength):(NextResourceHob->PhysicalStart + NextResourceHob->ResourceLength);
          
                ResourceHob->PhysicalStart = (ResourceHob->PhysicalStart < NextResourceHob->PhysicalStart) ? 
                                                    ResourceHob->PhysicalStart : NextResourceHob->PhysicalStart;
                
               
                ResourceHob->ResourceLength = (MemoryRangeEnd - ResourceHob->PhysicalStart);
                
                ResourceHob->ResourceAttribute = ResourceHob->ResourceAttribute & (~EFI_RESOURCE_ATTRIBUTE_TESTED);
                //
                // Delete the NextResourceHob by marking it as unused.
                //
                GET_HOB_TYPE (NextHob) = EFI_HOB_TYPE_UNUSED;
                
              }
           }
        } 
      }
    } 
  }
  //
  // Try to find and validate the TOP address.
  //  
  if ((INT64)FixedPcdGet64(PcdLoadModuleAtFixAddressEnable) > 0 ) {
    //
    // The LMFA feature is enabled as load module at fixed absolute address.
    //
    TopLoadingAddress = (EFI_PHYSICAL_ADDRESS)FixedPcdGet64(PcdLoadModuleAtFixAddressEnable);
    DEBUG ((EFI_D_INFO, "LOADING MODULE FIXED INFO: Loading module at fixed absolute address.\n"));
    //
    // validate the Address. Loop the resource descriptor HOB to make sure the address is in valid memory range
    //
    if ((TopLoadingAddress & EFI_PAGE_MASK) != 0) {
      DEBUG ((EFI_D_INFO, "LOADING MODULE FIXED ERROR:Top Address %lx is invalid since top address should be page align. \n", TopLoadingAddress)); 
      ASSERT (FALSE);    
    }
    //
    // Search for a memory region that is below MAX_ADDRESS and in which TopLoadingAddress lies 
    //
    for (Hob.Raw = PrivateData->HobList.Raw; !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB(Hob)) {
      //
      // See if this is a resource descriptor HOB
      //
      if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {

        ResourceHob = Hob.ResourceDescriptor;
        //
        // See if this resource descrior HOB describes tested system memory below MAX_ADDRESS
        //    
        if (ResourceHob->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY &&
            ResourceHob->PhysicalStart + ResourceHob->ResourceLength <= MAX_ADDRESS) {
            //
            // See if Top address specified by user is valid.
            //
            if (ResourceHob->PhysicalStart + TotalReservedMemorySize < TopLoadingAddress && 
                (ResourceHob->PhysicalStart + ResourceHob->ResourceLength - MINIMUM_INITIAL_MEMORY_SIZE) >= TopLoadingAddress) {
              CurrentResourceHob = ResourceHob; 
              CurrentHob = Hob;
              break;
            }
        }
      }  
    }  
    if (CurrentResourceHob != NULL) {
      DEBUG ((EFI_D_INFO, "LOADING MODULE FIXED INFO:Top Address %lx is valid \n",  TopLoadingAddress));
      TopLoadingAddress += MINIMUM_INITIAL_MEMORY_SIZE; 
    } else {
      DEBUG ((EFI_D_INFO, "LOADING MODULE FIXED ERROR:Top Address %lx is invalid \n",  TopLoadingAddress)); 
      DEBUG ((EFI_D_INFO, "LOADING MODULE FIXED ERROR:The recommended Top Address for the platform is: \n")); 
      //
      // Print the recomended Top address range.
      // 
      for (Hob.Raw = PrivateData->HobList.Raw; !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB(Hob)) {
        //
        // See if this is a resource descriptor HOB
        //
        if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
        
          ResourceHob = Hob.ResourceDescriptor;
          //
          // See if this resource descrior HOB describes tested system memory below MAX_ADDRESS
          //    
          if (ResourceHob->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY &&
              ResourceHob->PhysicalStart + ResourceHob->ResourceLength <= MAX_ADDRESS) {
              //
              // See if Top address specified by user is valid.
              //
              if (ResourceHob->ResourceLength > TotalReservedMemorySize) {
                 DEBUG ((EFI_D_INFO, "(%lx, %lx)\n",  
                          (ResourceHob->PhysicalStart + TotalReservedMemorySize -MINIMUM_INITIAL_MEMORY_SIZE), 
                          (ResourceHob->PhysicalStart + ResourceHob->ResourceLength -MINIMUM_INITIAL_MEMORY_SIZE) 
                        )); 
              }
          }
        }
      }  
      //
      // Assert here 
      //
      ASSERT (FALSE);      
    }     
  } else {
    //
    // The LMFA feature is enabled as load module at fixed offset relative to TOLM
    // Parse the Hob list to find the topest available memory. Generally it is (TOLM - TSEG)
    //
    //
    // Search for a tested memory region that is below MAX_ADDRESS
    //
    for (Hob.Raw = PrivateData->HobList.Raw; !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB(Hob)) {
      //
      // See if this is a resource descriptor HOB 
      //
      if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
        
        ResourceHob = Hob.ResourceDescriptor;                                                                                                                                                                                                                               
        //
        // See if this resource descrior HOB describes tested system memory below MAX_ADDRESS
        //
        if (ResourceHob->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY && 
            ResourceHob->PhysicalStart + ResourceHob->ResourceLength <= MAX_ADDRESS &&
            ResourceHob->ResourceLength > TotalReservedMemorySize) {
          //
          // See if this is the highest largest system memory region below MaxAddress
          //
          if (ResourceHob->PhysicalStart > HighAddress) {
             CurrentResourceHob = ResourceHob;
             CurrentHob = Hob;
             HighAddress = CurrentResourceHob->PhysicalStart;
          }
        }
      }  
    }
    if (CurrentResourceHob == NULL) {
      DEBUG ((EFI_D_INFO, "LOADING MODULE FIXED ERROR:The System Memory is too small\n")); 
      //
      // Assert here 
      //
      ASSERT (FALSE);      
    } else {
      TopLoadingAddress = CurrentResourceHob->PhysicalStart + CurrentResourceHob->ResourceLength ; 
    }         
  }
  
  if (CurrentResourceHob != NULL) {
    //
    // rebuild hob for PEI memmory and reserved memory
    //
    BuildResourceDescriptorHob (
      EFI_RESOURCE_SYSTEM_MEMORY,                       // MemoryType,
      (
      EFI_RESOURCE_ATTRIBUTE_PRESENT |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_TESTED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
      ),
      (TopLoadingAddress - TotalReservedMemorySize),                             // MemoryBegin
      TotalReservedMemorySize      // MemoryLength
    );
    //
    // rebuild hob for the remain memory if necessary
    //
    if (CurrentResourceHob->PhysicalStart < TopLoadingAddress - TotalReservedMemorySize) {
      BuildResourceDescriptorHob (
        EFI_RESOURCE_SYSTEM_MEMORY,                       // MemoryType,
        (
         EFI_RESOURCE_ATTRIBUTE_PRESENT |
         EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
         EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
         ),
         CurrentResourceHob->PhysicalStart,                             // MemoryBegin
         (TopLoadingAddress - TotalReservedMemorySize - CurrentResourceHob->PhysicalStart)      // MemoryLength
       );
    }
    if (CurrentResourceHob->PhysicalStart + CurrentResourceHob->ResourceLength  > TopLoadingAddress ) {
      BuildResourceDescriptorHob (
        EFI_RESOURCE_SYSTEM_MEMORY,                     
        (
         EFI_RESOURCE_ATTRIBUTE_PRESENT |
         EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
         EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
         ),
         TopLoadingAddress,                            
         (CurrentResourceHob->PhysicalStart + CurrentResourceHob->ResourceLength  - TopLoadingAddress)     
       );
    }
    //
    // Delete CurrentHob by marking it as unused since the the memory range described by is rebuilt.
    //
    GET_HOB_TYPE (CurrentHob) = EFI_HOB_TYPE_UNUSED;         
  }

  //
  // Cache the top address for Loading Module at Fixed Address feature
  //
  PrivateData->LoadModuleAtFixAddressTopAddress = TopLoadingAddress - MINIMUM_INITIAL_MEMORY_SIZE;
  DEBUG ((EFI_D_INFO, "LOADING MODULE FIXED INFO: Top address = %lx\n",  PrivateData->LoadModuleAtFixAddressTopAddress)); 
  //
  // reinstall the PEI memory relative to TopLoadingAddress
  //
  PrivateData->PhysicalMemoryBegin   = TopLoadingAddress - TotalReservedMemorySize;
  PrivateData->FreePhysicalMemoryTop = PrivateData->PhysicalMemoryBegin + PeiMemorySize;
}
/**
  Conduct PEIM dispatch.

  @param SecCoreData     Points to a data structure containing information about the PEI core's operating
                         environment, such as the size and location of temporary RAM, the stack location and
                         the BFV location.
  @param Private         Pointer to the private data passed in from caller

**/
VOID
PeiDispatcher (
  IN CONST EFI_SEC_PEI_HAND_OFF  *SecCoreData,
  IN PEI_CORE_INSTANCE           *Private
  )
{
  EFI_STATUS                          Status;
  UINT32                              Index1;
  UINT32                              Index2;
  CONST EFI_PEI_SERVICES              **PeiServices;
  EFI_PEI_FILE_HANDLE                 PeimFileHandle;
  UINTN                               FvCount;
  UINTN                               PeimCount;
  UINT32                              AuthenticationState;
  EFI_PHYSICAL_ADDRESS                EntryPoint;
  EFI_PEIM_ENTRY_POINT2               PeimEntryPoint;
  UINTN                               SaveCurrentPeimCount;
  UINTN                               SaveCurrentFvCount;
  EFI_PEI_FILE_HANDLE                 SaveCurrentFileHandle;
  PEIM_FILE_HANDLE_EXTENDED_DATA      ExtendedData;
  EFI_PHYSICAL_ADDRESS                NewPermenentMemoryBase;
  TEMPORARY_RAM_SUPPORT_PPI           *TemporaryRamSupportPpi;
  EFI_HOB_HANDOFF_INFO_TABLE          *OldHandOffTable;
  EFI_HOB_HANDOFF_INFO_TABLE          *NewHandOffTable;
  INTN                                StackOffset;
  INTN                                HeapOffset;
  PEI_CORE_INSTANCE                   *PrivateInMem;
  UINT64                              NewPeiStackSize;
  UINT64                              OldPeiStackSize;
  UINT64                              StackGap;
  EFI_FV_FILE_INFO                    FvFileInfo;
  UINTN                               OldCheckingTop;
  UINTN                               OldCheckingBottom;
  PEI_CORE_FV_HANDLE                  *CoreFvHandle;
  VOID                                *LoadFixPeiCodeBegin;

  PeiServices = (CONST EFI_PEI_SERVICES **) &Private->PS;
  PeimEntryPoint = NULL;
  PeimFileHandle = NULL;
  EntryPoint     = 0;

  if ((Private->PeiMemoryInstalled) && (Private->HobList.HandoffInformationTable->BootMode != BOOT_ON_S3_RESUME)) {
    //
    // Once real memory is available, shadow the RegisterForShadow modules. And meanwhile
    // update the modules' status from PEIM_STATE_REGISITER_FOR_SHADOW to PEIM_STATE_DONE.
    //
    SaveCurrentPeimCount  = Private->CurrentPeimCount;
    SaveCurrentFvCount    = Private->CurrentPeimFvCount;
    SaveCurrentFileHandle =  Private->CurrentFileHandle;

    for (Index1 = 0; Index1 <= SaveCurrentFvCount; Index1++) {
      for (Index2 = 0; (Index2 < PcdGet32 (PcdPeiCoreMaxPeimPerFv)) && (Private->Fv[Index1].FvFileHandles[Index2] != NULL); Index2++) {
        if (Private->Fv[Index1].PeimState[Index2] == PEIM_STATE_REGISITER_FOR_SHADOW) {
          PeimFileHandle = Private->Fv[Index1].FvFileHandles[Index2];
          Status = PeiLoadImage (
                    (CONST EFI_PEI_SERVICES **) &Private->PS,
                    PeimFileHandle,
                    PEIM_STATE_REGISITER_FOR_SHADOW,
                    &EntryPoint,
                    &AuthenticationState
                    );
          if (Status == EFI_SUCCESS) {
            //
            // PEIM_STATE_REGISITER_FOR_SHADOW move to PEIM_STATE_DONE
            //
            Private->Fv[Index1].PeimState[Index2]++;
            Private->CurrentFileHandle   = PeimFileHandle;
            Private->CurrentPeimFvCount  = Index1;
            Private->CurrentPeimCount    = Index2;
            //
            // Call the PEIM entry point
            //
            PeimEntryPoint = (EFI_PEIM_ENTRY_POINT2)(UINTN)EntryPoint;

            PERF_START (PeimFileHandle, "PEIM", NULL, 0);
            PeimEntryPoint(PeimFileHandle, (const EFI_PEI_SERVICES **) &Private->PS);
            PERF_END (PeimFileHandle, "PEIM", NULL, 0);
          }

          //
          // Process the Notify list and dispatch any notifies for
          // newly installed PPIs.
          //
          ProcessNotifyList (Private);
        }
      }
    }
    Private->CurrentFileHandle  = SaveCurrentFileHandle;
    Private->CurrentPeimFvCount = SaveCurrentFvCount;
    Private->CurrentPeimCount   = SaveCurrentPeimCount;
  }

  //
  // This is the main dispatch loop.  It will search known FVs for PEIMs and
  // attempt to dispatch them.  If any PEIM gets dispatched through a single
  // pass of the dispatcher, it will start over from the Bfv again to see
  // if any new PEIMs dependencies got satisfied.  With a well ordered
  // FV where PEIMs are found in the order their dependencies are also
  // satisfied, this dipatcher should run only once.
  //
  do {
    //
    // In case that reenter PeiCore happens, the last pass record is still available.   
    //
    if (!Private->PeimDispatcherReenter) {
      Private->PeimNeedingDispatch      = FALSE;
      Private->PeimDispatchOnThisPass   = FALSE;
    } else {
      Private->PeimDispatcherReenter    = FALSE;
    }
    
    for (FvCount = Private->CurrentPeimFvCount; FvCount < Private->FvCount; FvCount++) {
      CoreFvHandle = FindNextCoreFvHandle (Private, FvCount);
      ASSERT (CoreFvHandle != NULL);
      
      //
      // If the FV has corresponding EFI_PEI_FIRMWARE_VOLUME_PPI instance, then dispatch it.
      //
      if (CoreFvHandle->FvPpi == NULL) {
        continue;
      }
      
      Private->CurrentPeimFvCount = FvCount;

      if (Private->CurrentPeimCount == 0) {
        //
        // When going through each FV, at first, search Apriori file to
        // reorder all PEIMs to ensure the PEIMs in Apriori file to get
        // dispatch at first.
        //
        DiscoverPeimsAndOrderWithApriori (Private, CoreFvHandle);
      }

      //
      // Start to dispatch all modules within the current Fv.
      //
      for (PeimCount = Private->CurrentPeimCount;
           (PeimCount < PcdGet32 (PcdPeiCoreMaxPeimPerFv)) && (Private->CurrentFvFileHandles[PeimCount] != NULL);
           PeimCount++) {
        Private->CurrentPeimCount  = PeimCount;
        PeimFileHandle = Private->CurrentFileHandle = Private->CurrentFvFileHandles[PeimCount];

        if (Private->Fv[FvCount].PeimState[PeimCount] == PEIM_STATE_NOT_DISPATCHED) {
          if (!DepexSatisfied (Private, PeimFileHandle, PeimCount)) {
            Private->PeimNeedingDispatch = TRUE;
          } else {
            Status = CoreFvHandle->FvPpi->GetFileInfo (CoreFvHandle->FvPpi, PeimFileHandle, &FvFileInfo);
            ASSERT_EFI_ERROR (Status);
            if (FvFileInfo.FileType == EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE) {
              //
              // For Fv type file, Produce new FV PPI and FV hob
              //
              Status = ProcessFvFile (&Private->Fv[FvCount], PeimFileHandle);
              AuthenticationState = 0;
            } else {
              //
              // For PEIM driver, Load its entry point
              //
              Status = PeiLoadImage (
                         PeiServices,
                         PeimFileHandle,
                         PEIM_STATE_NOT_DISPATCHED,
                         &EntryPoint,
                         &AuthenticationState
                         );
            }

            if ((Status == EFI_SUCCESS)) {
              //
              // The PEIM has its dependencies satisfied, and its entry point
              // has been found, so invoke it.
              //
              PERF_START (PeimFileHandle, "PEIM", NULL, 0);

              ExtendedData.Handle = (EFI_HANDLE)PeimFileHandle;

              REPORT_STATUS_CODE_WITH_EXTENDED_DATA (
                EFI_PROGRESS_CODE,
                (EFI_SOFTWARE_PEI_CORE | EFI_SW_PC_INIT_BEGIN),
                (VOID *)(&ExtendedData),
                sizeof (ExtendedData)
                );

              Status = VerifyPeim (Private, CoreFvHandle->FvHandle, PeimFileHandle);
              if (Status != EFI_SECURITY_VIOLATION && (AuthenticationState == 0)) {
                //
                // PEIM_STATE_NOT_DISPATCHED move to PEIM_STATE_DISPATCHED
                //
                Private->Fv[FvCount].PeimState[PeimCount]++;

                if (FvFileInfo.FileType != EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE) {
                  //
                  // Call the PEIM entry point for PEIM driver
                  //
                  PeimEntryPoint = (EFI_PEIM_ENTRY_POINT2)(UINTN)EntryPoint;
                  PeimEntryPoint (PeimFileHandle, (const EFI_PEI_SERVICES **) PeiServices);
                }

                Private->PeimDispatchOnThisPass = TRUE;
              }

              REPORT_STATUS_CODE_WITH_EXTENDED_DATA (
                EFI_PROGRESS_CODE,
                (EFI_SOFTWARE_PEI_CORE | EFI_SW_PC_INIT_BEGIN),
                (VOID *)(&ExtendedData),
                sizeof (ExtendedData)
                );
              PERF_END (PeimFileHandle, "PEIM", NULL, 0);

            }

            if (Private->SwitchStackSignal) {
              //
              // Before switch stack from temporary memory to permenent memory, caculate the heap and stack
              // usage in temporary memory for debuging.
              //
              DEBUG_CODE_BEGIN ();
                UINT32  *StackPointer;
                
                for (StackPointer = (UINT32*)SecCoreData->StackBase;
                     (StackPointer < (UINT32*)((UINTN)SecCoreData->StackBase + SecCoreData->StackSize)) \
                     && (*StackPointer == INIT_CAR_VALUE);
                     StackPointer ++);
                     
                DEBUG ((EFI_D_INFO, "Total temporary memory:    %d bytes.\n", (UINT32)SecCoreData->TemporaryRamSize));
                DEBUG ((EFI_D_INFO, "  temporary memory stack ever used: %d bytes.\n",
                       (SecCoreData->StackSize - ((UINTN) StackPointer - (UINTN)SecCoreData->StackBase))
                      ));
                DEBUG ((EFI_D_INFO, "  temporary memory heap used:       %d bytes.\n",
                       ((UINTN) Private->HobList.HandoffInformationTable->EfiFreeMemoryBottom -
                       (UINTN) Private->HobList.Raw)
                      ));
              DEBUG_CODE_END ();
              
              if (FixedPcdGet64(PcdLoadModuleAtFixAddressEnable) != 0) {
                //
                // Loading Module at Fixed Address is enabled
                //
                PeiLoadFixAddressHook(Private);
              }
              
              //
              // Reserve the size of new stack at bottom of physical memory
              //
              OldPeiStackSize = (UINT64) SecCoreData->StackSize;
              NewPeiStackSize = (RShiftU64 (Private->PhysicalMemoryLength, 1) + EFI_PAGE_MASK) & ~EFI_PAGE_MASK;
              if (PcdGet32(PcdPeiCoreMaxPeiStackSize) > (UINT32) NewPeiStackSize) {
                Private->StackSize = NewPeiStackSize;
              } else {
                Private->StackSize = PcdGet32(PcdPeiCoreMaxPeiStackSize);
              }

              //
              // In theory, the size of new stack in permenent memory should large than
              // size of old stack in temporary memory.
              // But if new stack is smaller than the size of old stack, we also reserve
              // the size of old stack at bottom of permenent memory.
              //
              DEBUG ((EFI_D_INFO, "Old Stack size %d, New stack size %d\n", (INT32) OldPeiStackSize, (INT32) Private->StackSize));
              ASSERT (Private->StackSize >= OldPeiStackSize);
              StackGap = Private->StackSize - OldPeiStackSize;

              //
              // Update HandOffHob for new installed permenent memory
              //
              OldHandOffTable   = Private->HobList.HandoffInformationTable;
              OldCheckingBottom = (UINTN)(SecCoreData->TemporaryRamBase);
              OldCheckingTop    = (UINTN)(OldCheckingBottom + SecCoreData->TemporaryRamSize);

              //
              // The whole temporary memory will be migrated to physical memory.
              // CAUTION: The new base is computed accounding to gap of new stack.
              //
              NewPermenentMemoryBase = Private->PhysicalMemoryBegin + StackGap;
              
              //
              // Caculate stack offset and heap offset between temporary memory and new permement 
              // memory seperately.
              //
              StackOffset            = (UINTN) NewPermenentMemoryBase - (UINTN) SecCoreData->StackBase;
              HeapOffset             = (INTN) ((UINTN) Private->PhysicalMemoryBegin + Private->StackSize - \
                                               (UINTN) SecCoreData->PeiTemporaryRamBase);
              DEBUG ((EFI_D_INFO, "Heap Offset = 0x%lX Stack Offset = 0x%lX\n", (INT64)HeapOffset, (INT64)StackOffset));
              
              //
              // Caculate new HandOffTable and PrivateData address in permenet memory's stack
              //
              NewHandOffTable        = (EFI_HOB_HANDOFF_INFO_TABLE *)((UINTN)OldHandOffTable + HeapOffset);
              PrivateInMem           = (PEI_CORE_INSTANCE *)((UINTN) (VOID*) Private + StackOffset);

              //
              // TemporaryRamSupportPpi is produced by platform's SEC
              //
              Status = PeiLocatePpi (
                         (CONST EFI_PEI_SERVICES **) PeiServices,
                         &gEfiTemporaryRamSupportPpiGuid,
                         0,
                         NULL,
                         (VOID**)&TemporaryRamSupportPpi
                         );


              if (!EFI_ERROR (Status)) {
                //
                // Temporary Ram support Ppi is provided by platform, it will copy 
                // temporary memory to permenent memory and do stack switching.
                // After invoken temporary Ram support, following code's stack is in 
                // memory but not in temporary memory.
                //
                TemporaryRamSupportPpi->TemporaryRamMigration (
                                          (CONST EFI_PEI_SERVICES **) PeiServices,
                                          (EFI_PHYSICAL_ADDRESS)(UINTN) SecCoreData->TemporaryRamBase,
                                          (EFI_PHYSICAL_ADDRESS)(UINTN) NewPermenentMemoryBase,
                                          SecCoreData->TemporaryRamSize
                                          );

              } else {
                //
                // In IA32/x64/Itanium architecture, we need platform provide
                // TEMPORAY_RAM_MIGRATION_PPI.
                //
                ASSERT (FALSE);
              }


              //
              //
              // Fixup the PeiCore's private data
              //
              PrivateInMem->PS          = &PrivateInMem->ServiceTableShadow;
              PrivateInMem->CpuIo       = &PrivateInMem->ServiceTableShadow.CpuIo;
              PrivateInMem->HobList.Raw = (VOID*) ((UINTN) PrivateInMem->HobList.Raw + HeapOffset);
              PrivateInMem->StackBase   = (EFI_PHYSICAL_ADDRESS)(((UINTN)PrivateInMem->PhysicalMemoryBegin + EFI_PAGE_MASK) & ~EFI_PAGE_MASK);

              PeiServices = (CONST EFI_PEI_SERVICES **) &PrivateInMem->PS;

              //
              // Fixup for PeiService's address
              //
              SetPeiServicesTablePointer(PeiServices);

              //
              // Update HandOffHob for new installed permenent memory
              //
              NewHandOffTable->EfiEndOfHobList =
                (EFI_PHYSICAL_ADDRESS)((UINTN) NewHandOffTable->EfiEndOfHobList + HeapOffset);
              NewHandOffTable->EfiMemoryTop        = PrivateInMem->PhysicalMemoryBegin +
                                                     PrivateInMem->PhysicalMemoryLength;
              NewHandOffTable->EfiMemoryBottom     = PrivateInMem->PhysicalMemoryBegin;
              NewHandOffTable->EfiFreeMemoryTop    = PrivateInMem->FreePhysicalMemoryTop;
              NewHandOffTable->EfiFreeMemoryBottom = NewHandOffTable->EfiEndOfHobList +
                                                     sizeof (EFI_HOB_GENERIC_HEADER);

              //
              // We need convert the PPI desciptor's pointer
              //
              ConvertPpiPointers (PrivateInMem, 
                                  OldCheckingBottom, 
                                  OldCheckingTop, 
                                  HeapOffset
                                  );

              DEBUG ((EFI_D_INFO, "Stack Hob: BaseAddress=0x%lX Length=0x%lX\n",
                                  PrivateInMem->StackBase,
                                  PrivateInMem->StackSize));
              BuildStackHob (PrivateInMem->StackBase, PrivateInMem->StackSize);

              //
              // After the whole temporary memory is migrated, then we can allocate page in
              // permenent memory.
              //
              PrivateInMem->PeiMemoryInstalled     = TRUE;

              //
              // Indicate that PeiCore reenter
              //
              PrivateInMem->PeimDispatcherReenter  = TRUE;
              
              if (FixedPcdGet64(PcdLoadModuleAtFixAddressEnable) != 0) {
                //
                // if Loading Module at Fixed Address is enabled, This is the first invoke to page 
                // allocation for Pei Core segment. This memory segment should be reserved for loading PEIM
                //
                LoadFixPeiCodeBegin = AllocatePages((UINTN)PcdGet32(PcdLoadFixAddressPeiCodePageNumber));
                DEBUG ((EFI_D_INFO, "LOADING MODULE FIXED INFO: PeiCodeBegin = %x, PeiCodeTop= %x\n", (UINTN)LoadFixPeiCodeBegin, ((UINTN)LoadFixPeiCodeBegin) + PcdGet32(PcdLoadFixAddressPeiCodePageNumber) * EFI_PAGE_SIZE));                 
                //
                // if Loading Module at Fixed Address is enabled, allocate the PEI code memory range usage bit map array.
                // Every bit in the array indicate the status of the corresponding memory page, available or not
                //
                PrivateInMem->PeiCodeMemoryRangeUsageBitMap = AllocateZeroPool (((PcdGet32(PcdLoadFixAddressPeiCodePageNumber)>>6) + 1)*sizeof(UINT64));
              }
              //
              // Shadow PEI Core. When permanent memory is avaiable, shadow
              // PEI Core and PEIMs to get high performance.
              //
              PrivateInMem->ShadowedPeiCore = ShadowPeiCore (
                                                PeiServices,
                                                PrivateInMem
                                                );
              //
              // Process the Notify list and dispatch any notifies for
              // newly installed PPIs.
              //
              ProcessNotifyList (PrivateInMem);

              //
              // Entry PEI Phase 2
              //
              PeiCore (SecCoreData, NULL, PrivateInMem);

              //
              // Code should not come here
              //
              ASSERT_EFI_ERROR(FALSE);
            }

            //
            // Process the Notify list and dispatch any notifies for
            // newly installed PPIs.
            //
            ProcessNotifyList (Private);

            if ((Private->PeiMemoryInstalled) && (Private->Fv[FvCount].PeimState[PeimCount] == PEIM_STATE_REGISITER_FOR_SHADOW) &&   \
                (Private->HobList.HandoffInformationTable->BootMode != BOOT_ON_S3_RESUME)) {
              //
              // If memory is availble we shadow images by default for performance reasons.
              // We call the entry point a 2nd time so the module knows it's shadowed.
              //
              //PERF_START (PeiServices, L"PEIM", PeimFileHandle, 0);
              ASSERT (PeimEntryPoint != NULL);
              PeimEntryPoint (PeimFileHandle, (const EFI_PEI_SERVICES **) PeiServices);
              //PERF_END (PeiServices, L"PEIM", PeimFileHandle, 0);

              //
              // PEIM_STATE_REGISITER_FOR_SHADOW move to PEIM_STATE_DONE
              //
              Private->Fv[FvCount].PeimState[PeimCount]++;

              //
              // Process the Notify list and dispatch any notifies for
              // newly installed PPIs.
              //
              ProcessNotifyList (Private);
            }
          }
        }
      }

      //
      // We set to NULL here to optimize the 2nd entry to this routine after
      //  memory is found. This reprevents rescanning of the FV. We set to
      //  NULL here so we start at the begining of the next FV
      //
      Private->CurrentFileHandle = NULL;
      Private->CurrentPeimCount = 0;
      //
      // Before walking through the next FV,Private->CurrentFvFileHandles[]should set to NULL
      //
      SetMem (Private->CurrentFvFileHandles, sizeof (Private->CurrentFvFileHandles), 0);
    }

    //
    // Before making another pass, we should set Private->CurrentPeimFvCount =0 to go
    // through all the FV.
    //
    Private->CurrentPeimFvCount = 0;

    //
    // PeimNeedingDispatch being TRUE means we found a PEIM that did not get
    //  dispatched. So we need to make another pass
    //
    // PeimDispatchOnThisPass being TRUE means we dispatched a PEIM on this
    //  pass. If we did not dispatch a PEIM there is no point in trying again
    //  as it will fail the next time too (nothing has changed).
    //
  } while (Private->PeimNeedingDispatch && Private->PeimDispatchOnThisPass);

}

/**
  Initialize the Dispatcher's data members

  @param PrivateData     PeiCore's private data structure
  @param OldCoreData     Old data from SecCore
                         NULL if being run in non-permament memory mode.
  @param SecCoreData     Points to a data structure containing information about the PEI core's operating
                         environment, such as the size and location of temporary RAM, the stack location and
                         the BFV location.

  @return None.

**/
VOID
InitializeDispatcherData (
  IN PEI_CORE_INSTANCE            *PrivateData,
  IN PEI_CORE_INSTANCE            *OldCoreData,
  IN CONST EFI_SEC_PEI_HAND_OFF   *SecCoreData
  )
{
  if (OldCoreData == NULL) {
    PrivateData->PeimDispatcherReenter = FALSE;
    PeiInitializeFv (PrivateData, SecCoreData);
  } else {
    PeiReinitializeFv (PrivateData);
  }

  return;
}

/**
  This routine parses the Dependency Expression, if available, and
  decides if the module can be executed.


  @param Private         PeiCore's private data structure
  @param FileHandle      PEIM's file handle
  @param PeimCount       Peim count in all dispatched PEIMs.

  @retval TRUE   Can be dispatched
  @retval FALSE  Cannot be dispatched

**/
BOOLEAN
DepexSatisfied (
  IN PEI_CORE_INSTANCE          *Private,
  IN EFI_PEI_FILE_HANDLE        FileHandle,
  IN UINTN                      PeimCount
  )
{
  EFI_STATUS           Status;
  VOID                 *DepexData;

  if (PeimCount < Private->AprioriCount) {
    //
    // If its in the A priori file then we set Depex to TRUE
    //
    return TRUE;
  }

  //
  // Depex section not in the encapsulated section.
  //
  Status = PeiServicesFfsFindSectionData (
              EFI_SECTION_PEI_DEPEX,
              FileHandle,
              (VOID **)&DepexData
              );

  if (EFI_ERROR (Status)) {
    //
    // If there is no DEPEX, assume the module can be executed
    //
    return TRUE;
  }

  //
  // Evaluate a given DEPEX
  //
  return PeimDispatchReadiness (&Private->PS, DepexData);
}

/**
  This routine enable a PEIM to register itself to shadow when PEI Foundation
  discovery permanent memory.

  @param FileHandle             File handle of a PEIM.

  @retval EFI_NOT_FOUND         The file handle doesn't point to PEIM itself.
  @retval EFI_ALREADY_STARTED   Indicate that the PEIM has been registered itself.
  @retval EFI_SUCCESS           Successfully to register itself.

**/
EFI_STATUS
EFIAPI
PeiRegisterForShadow (
  IN EFI_PEI_FILE_HANDLE       FileHandle
  )
{
  PEI_CORE_INSTANCE            *Private;
  Private = PEI_CORE_INSTANCE_FROM_PS_THIS (GetPeiServicesTablePointer ());

  if (Private->CurrentFileHandle != FileHandle) {
    //
    // The FileHandle must be for the current PEIM
    //
    return EFI_NOT_FOUND;
  }

  if (Private->Fv[Private->CurrentPeimFvCount].PeimState[Private->CurrentPeimCount] >= PEIM_STATE_REGISITER_FOR_SHADOW) {
    //
    // If the PEIM has already entered the PEIM_STATE_REGISTER_FOR_SHADOW or PEIM_STATE_DONE then it's already been started
    //
    return EFI_ALREADY_STARTED;
  }

  Private->Fv[Private->CurrentPeimFvCount].PeimState[Private->CurrentPeimCount] = PEIM_STATE_REGISITER_FOR_SHADOW;

  return EFI_SUCCESS;
}



