/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*  
*  This program and the accompanying materials                          
*  are licensed and made available under the terms and conditions of the BSD License         
*  which accompanies this distribution.  The full text of the license may be found at        
*  http://opensource.org/licenses/bsd-license.php                                            
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
*
**/

#include "BdsInternal.h"

#include <Library/DxeServicesTableLib.h>
#include <Library/HobLib.h>

EFI_STATUS
ShutdownUefiBootServices( VOID )
{
  EFI_STATUS              Status;
  UINTN                   MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR   *MemoryMap;
  UINTN                   MapKey;
  UINTN                   DescriptorSize;
  UINT32                  DescriptorVersion;
  UINTN                   Pages;

  MemoryMap = NULL;
  MemoryMapSize = 0;
  do {
    Status = gBS->GetMemoryMap (
                    &MemoryMapSize,
                    MemoryMap,
                    &MapKey,
                    &DescriptorSize,
                    &DescriptorVersion
                    );
    if (Status == EFI_BUFFER_TOO_SMALL) {

      Pages = EFI_SIZE_TO_PAGES (MemoryMapSize) + 1;
      MemoryMap = AllocatePages (Pages);

      //
      // Get System MemoryMap
      //
      Status = gBS->GetMemoryMap (
                      &MemoryMapSize,
                      MemoryMap,
                      &MapKey,
                      &DescriptorSize,
                      &DescriptorVersion
                      );
      // Don't do anything between the GetMemoryMap() and ExitBootServices()
      if (!EFI_ERROR (Status)) {
        Status = gBS->ExitBootServices (gImageHandle, MapKey);
        if (EFI_ERROR (Status)) {
          FreePages (MemoryMap, Pages);
          MemoryMap = NULL;
          MemoryMapSize = 0;
        }
      }
    }
  } while (EFI_ERROR (Status));

  return Status;
}

EFI_STATUS
BdsConnectAllDrivers( VOID ) {
    UINTN                     HandleCount, Index;
    EFI_HANDLE                *HandleBuffer;
    EFI_STATUS                Status;

    do {
        // Locate all the driver handles
        Status = gBS->LocateHandleBuffer (
                    AllHandles,
                    NULL,
                    NULL,
                    &HandleCount,
                    &HandleBuffer
                    );
        if (EFI_ERROR (Status)) {
            break;
        }

        // Connect every handles
        for (Index = 0; Index < HandleCount; Index++) {
            gBS->ConnectController(HandleBuffer[Index], NULL, NULL, TRUE);
        }

        if (HandleBuffer != NULL) {
            FreePool (HandleBuffer);
        }
        
        // Check if new handles have been created after the start of the previous handles
        Status = gDS->Dispatch ();
    } while (!EFI_ERROR(Status));

    return EFI_SUCCESS;
}

STATIC EFI_STATUS InsertSystemMemoryResources(LIST_ENTRY *ResourceList, EFI_HOB_RESOURCE_DESCRIPTOR *ResHob) {
    BDS_SYSTEM_MEMORY_RESOURCE  NewResource;
    LIST_ENTRY                  *Link;
    BDS_SYSTEM_MEMORY_RESOURCE  *Resource;

    //DEBUG ((EFI_D_ERROR, "** InsertSystemMemoryResources(0x%X,0x%X)\n",(UINT32)ResHob->PhysicalStart,(UINT32)ResHob->ResourceLength));

    if (IsListEmpty (ResourceList)) {
        ZeroMem(&NewResource,sizeof(BDS_SYSTEM_MEMORY_RESOURCE));
        NewResource.PhysicalStart = ResHob->PhysicalStart;
        NewResource.ResourceLength = ResHob->ResourceLength;
        InsertTailList (ResourceList, &NewResource.Link);
        return EFI_SUCCESS;
    }

    //for (Link = GetFirstNode (ResourceList); !IsNull (ResourceList,Link); Link = GetNextNode (ResourceList,Link)) {
    Link = ResourceList->ForwardLink;
    while (Link != NULL && Link != ResourceList) {
        Resource = (BDS_SYSTEM_MEMORY_RESOURCE*)Link;
        //DEBUG ((EFI_D_ERROR, "   - (0x%X,0x%X)\n",(UINT32)Resource->PhysicalStart,(UINT32)Resource->ResourceLength));

        // Sanity Check. The resources should not overlapped.
        ASSERT(!((ResHob->PhysicalStart >= Resource->PhysicalStart) && (ResHob->PhysicalStart < (Resource->PhysicalStart + Resource->ResourceLength))));
        ASSERT(!((ResHob->PhysicalStart + ResHob->ResourceLength >= Resource->PhysicalStart) &&
            ((ResHob->PhysicalStart + ResHob->ResourceLength) < (Resource->PhysicalStart + Resource->ResourceLength))));

        // The new resource is attached after this resource descriptor
        if (ResHob->PhysicalStart == Resource->PhysicalStart + Resource->ResourceLength) {
            Resource->ResourceLength =  Resource->ResourceLength + ResHob->ResourceLength;
            //DEBUG ((EFI_D_ERROR, "** Attached new Length:0x%X\n",(UINT32)Resource->ResourceLength));
            return EFI_SUCCESS;
        }
        // The new resource is attached before this resource descriptor
        else if (ResHob->PhysicalStart + ResHob->ResourceLength == Resource->PhysicalStart) {
            Resource->PhysicalStart = ResHob->PhysicalStart;
            Resource->ResourceLength =  Resource->ResourceLength + ResHob->ResourceLength;
            //DEBUG ((EFI_D_ERROR, "** Attached2 new Length:0x%X\n",(UINT32)Resource->ResourceLength));
            return EFI_SUCCESS;
        }
        Link = Link->ForwardLink;
    }

    // None of the Resource of the list is attached to this ResHob. Create a new entry for it
    ZeroMem(&NewResource,sizeof(BDS_SYSTEM_MEMORY_RESOURCE));
    NewResource.PhysicalStart = ResHob->PhysicalStart;
    NewResource.ResourceLength = ResHob->ResourceLength;
    InsertTailList (ResourceList, &NewResource.Link);
    return EFI_SUCCESS;
}

EFI_STATUS GetSystemMemoryResources(LIST_ENTRY *ResourceList) {
    EFI_HOB_RESOURCE_DESCRIPTOR *ResHob;

    InitializeListHead (ResourceList);
    
    // Find the first System Memory Resource Descriptor
    ResHob = (EFI_HOB_RESOURCE_DESCRIPTOR *)GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);
    while ((ResHob != NULL) && (ResHob->ResourceType != EFI_RESOURCE_SYSTEM_MEMORY)) {
        ResHob = (EFI_HOB_RESOURCE_DESCRIPTOR *)GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR,(VOID *)((UINTN)ResHob + ResHob->Header.HobLength)); 
    }

    // Did not find any
    if (ResHob == NULL) {
        return EFI_NOT_FOUND;
    } else {
        InsertSystemMemoryResources(ResourceList, ResHob);
    }

    ResHob = (EFI_HOB_RESOURCE_DESCRIPTOR *)GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR,(VOID *)((UINTN)ResHob + ResHob->Header.HobLength)); 
    while (ResHob != NULL) {
        if (ResHob->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
            InsertSystemMemoryResources(ResourceList, ResHob);
        }
        ResHob = (EFI_HOB_RESOURCE_DESCRIPTOR *)GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR,(VOID *)((UINTN)ResHob + ResHob->Header.HobLength)); 
    }

    return EFI_SUCCESS;
}
