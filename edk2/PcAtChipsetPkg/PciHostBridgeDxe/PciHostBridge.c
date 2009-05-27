/** @file
  Pci Host Bridge driver: 
  Provides the basic interfaces to abstract a PCI Host Bridge Resource Allocation

  Copyright (c) 2008 - 2009, Intel Corporation<BR> All rights
  reserved. This program and the accompanying materials are
  licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php
  
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/ 

#include "PciHostBridge.h"

//
// Support 64 K IO space
//
#define RES_IO_BASE   0x1000
#define RES_IO_LIMIT  0xFFFF
//
// Support 4G address space
//
#define RES_MEM_BASE_1 0xF8000000
#define RES_MEM_LIMIT_1 (0xFEC00000 - 1)

//
// Hard code: Root Bridge Number within the host bridge
//            Root Bridge's attribute
//            Root Bridge's device path
//            Root Bridge's resource appeture
//
UINTN RootBridgeNumber[1] = { 1 };

UINT64 RootBridgeAttribute[1][1] = { EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM };

EFI_PCI_ROOT_BRIDGE_DEVICE_PATH mEfiPciRootBridgeDevicePath[1][1] = {
  {
    ACPI_DEVICE_PATH,
    ACPI_DP,
    (UINT8) (sizeof(ACPI_HID_DEVICE_PATH)),
    (UINT8) ((sizeof(ACPI_HID_DEVICE_PATH)) >> 8),
    EISA_PNP_ID(0x0A03),
    0,
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    END_DEVICE_PATH_LENGTH,
    0
  }
};

PCI_ROOT_BRIDGE_RESOURCE_APPETURE  mResAppeture[1][1] = {
  {0, 0, 0, 0xffffffff, 0, 1 << 16}
};

EFI_HANDLE mDriverImageHandle;

PCI_HOST_BRIDGE_INSTANCE mPciHostBridgeInstanceTemplate = {
  PCI_HOST_BRIDGE_SIGNATURE,  // Signature
  NULL,                       // HostBridgeHandle
  0,                          // RootBridgeNumber
  {NULL, NULL},               // Head
  FALSE,                      // ResourceSubiteed
  TRUE,                       // CanRestarted
  {
    NotifyPhase,
    GetNextRootBridge,
    GetAttributes,
    StartBusEnumeration,
    SetBusNumbers,
    SubmitResources,
    GetProposedResources,
    PreprocessController
  }
};

//
// Implementation
//
EFI_STATUS
EFIAPI
InitializePciHostBridge (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:
  Entry point of this driver

Arguments:

    ImageHandle -

    SystemTable -
    
Returns:

--*/
{
  EFI_STATUS                  Status;
  UINTN                       Loop1;
  UINTN                       Loop2;
  PCI_HOST_BRIDGE_INSTANCE    *HostBridge;
  PCI_ROOT_BRIDGE_INSTANCE    *PrivateData;
  IN EFI_PHYSICAL_ADDRESS     BaseAddress;
  IN UINT64                   Length;
 
  mDriverImageHandle = ImageHandle;
  
  //
  // Create Host Bridge Device Handle
  //
  for (Loop1 = 0; Loop1 < HOST_BRIDGE_NUMBER; Loop1++) {
    HostBridge = AllocateCopyPool (sizeof(PCI_HOST_BRIDGE_INSTANCE), &mPciHostBridgeInstanceTemplate);
    if (HostBridge == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  
    HostBridge->RootBridgeNumber = RootBridgeNumber[Loop1];
    InitializeListHead (&HostBridge->Head);

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &HostBridge->HostBridgeHandle,              
                    &gEfiPciHostBridgeResourceAllocationProtocolGuid, &HostBridge->ResAlloc,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      FreePool (HostBridge);
      return EFI_DEVICE_ERROR;
    }
  
    //
    // Create Root Bridge Device Handle in this Host Bridge
    //
  
    for (Loop2 = 0; Loop2 < HostBridge->RootBridgeNumber; Loop2++) {
      PrivateData = AllocateZeroPool (sizeof(PCI_ROOT_BRIDGE_INSTANCE));
      if (PrivateData == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      PrivateData->Signature = PCI_ROOT_BRIDGE_SIGNATURE;
      PrivateData->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)&mEfiPciRootBridgeDevicePath[Loop1][Loop2];

      RootBridgeConstructor (
        &PrivateData->Io, 
        HostBridge->HostBridgeHandle, 
        RootBridgeAttribute[Loop1][Loop2], 
        &mResAppeture[Loop1][Loop2]
        );
    
      Status = gBS->InstallMultipleProtocolInterfaces(
                      &PrivateData->Handle,              
                      &gEfiDevicePathProtocolGuid,      PrivateData->DevicePath,
                      &gEfiPciRootBridgeIoProtocolGuid, &PrivateData->Io,
                      NULL
                      );
      if (EFI_ERROR (Status)) {
        FreePool(PrivateData);
        return EFI_DEVICE_ERROR;
      }
 
      InsertTailList (&HostBridge->Head, &PrivateData->Link);
    }
  } 

  Status = gDS->AddIoSpace (
                  EfiGcdIoTypeIo, 
                  RES_IO_BASE, 
                  RES_IO_LIMIT - RES_IO_BASE + 1
                  );
  
  // PCI memory space from 3.75Gbytes->(4GBytes - BIOSFWH local APIC etc)
  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeMemoryMappedIo, 
                  RES_MEM_BASE_1, 
                  (RES_MEM_LIMIT_1 - RES_MEM_BASE_1 + 1),
                  0
                  );
  
  BaseAddress = 0x80000000;
  Length      = RES_MEM_BASE_1 - BaseAddress;
  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeMemoryMappedIo, 
                  BaseAddress, 
                  Length,
                  0
                  );
  
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
NotifyPhase(
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE    Phase
  )
/*++

Routine Description:
  Enter a certain phase of the PCI enumeration process

Arguments:
  This  -- The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL instance
  Phase -- The phase during enumeration
    
Returns:

--*/  
{
  PCI_HOST_BRIDGE_INSTANCE              *HostBridgeInstance;
  PCI_ROOT_BRIDGE_INSTANCE              *RootBridgeInstance;
  PCI_RESOURCE_TYPE                     Index;
  LIST_ENTRY                            *List;
  EFI_PHYSICAL_ADDRESS                  BaseAddress;
  UINT64                                AddrLen;
  UINTN                                 BitsOfAlignment;
  EFI_STATUS                            Status;
  EFI_STATUS                            ReturnStatus;
  
  HostBridgeInstance = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);
  
  switch (Phase) {

  case EfiPciHostBridgeBeginEnumeration:
    if (HostBridgeInstance->CanRestarted) {
      //
      // Reset the Each Root Bridge 
      //
      List = HostBridgeInstance->Head.ForwardLink;
  
      while (List != &HostBridgeInstance->Head) {
        RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
        for (Index = TypeIo; Index < TypeMax; Index++) {
          RootBridgeInstance->ResAllocNode[Index].Type      = Index;
          RootBridgeInstance->ResAllocNode[Index].Base      = 0;
          RootBridgeInstance->ResAllocNode[Index].Length    = 0;
          RootBridgeInstance->ResAllocNode[Index].Status    = ResNone;
        }
          
        List = List->ForwardLink;
      }
        
      HostBridgeInstance->ResourceSubmited = FALSE;
      HostBridgeInstance->CanRestarted     = TRUE;
    } else {
      //
      // Can not restart
      // 
      return EFI_NOT_READY;
    }  
    break;

  case EfiPciHostBridgeBeginBusAllocation:
    //
    // No specific action is required here, can perform any chipset specific programing
    //
    HostBridgeInstance->CanRestarted = FALSE;
    return EFI_SUCCESS;
    break;

  case EfiPciHostBridgeEndBusAllocation:
    //
    // No specific action is required here, can perform any chipset specific programing
    //
    //HostBridgeInstance->CanRestarted = FALSE;
    return EFI_SUCCESS;
    break;

  case EfiPciHostBridgeBeginResourceAllocation:
    //
    // No specific action is required here, can perform any chipset specific programing
    //
    //HostBridgeInstance->CanRestarted = FALSE;
    return EFI_SUCCESS;
    break;

  case EfiPciHostBridgeAllocateResources:
    ReturnStatus = EFI_SUCCESS;
    if (HostBridgeInstance->ResourceSubmited) {
      //
      // Take care of the resource dependencies between the root bridges 
      //
      List = HostBridgeInstance->Head.ForwardLink;

      while (List != &HostBridgeInstance->Head) {
        RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
        for (Index = TypeIo; Index < TypeBus; Index++) {
          if (RootBridgeInstance->ResAllocNode[Index].Status != ResNone) {
                                        
            AddrLen = RootBridgeInstance->ResAllocNode[Index].Length;
            
            //
            // Get the number of '1' in Alignment.
            //
            BitsOfAlignment = HighBitSet64 (RootBridgeInstance->ResAllocNode[Index].Alignment) + 1;
                                  
            switch (Index) {

              case TypeIo:  
                //
                // It is impossible for this chipset to align 0xFFFF for IO16
                // So clear it
                //
                if (BitsOfAlignment >= 16) {
                  BitsOfAlignment = 0;
                }
                  
                Status = gDS->AllocateIoSpace (
                                EfiGcdAllocateAnySearchBottomUp, 
                                EfiGcdIoTypeIo, 
                                BitsOfAlignment,
                                AddrLen,
                                &BaseAddress,
                                mDriverImageHandle,
                                NULL
                                );
                 
                if (!EFI_ERROR (Status)) {
                  RootBridgeInstance->ResAllocNode[Index].Base   = (UINTN)BaseAddress;
                  RootBridgeInstance->ResAllocNode[Index].Status = ResAllocated; 
                } else {
                  ReturnStatus = Status;  
                  if (Status != EFI_OUT_OF_RESOURCES) {
                    RootBridgeInstance->ResAllocNode[Index].Length = 0;
                  }
                }

                break;


              case TypeMem32:
                //
                // It is impossible for this chipset to align 0xFFFFFFFF for Mem32
                // So clear it 
                //
                  
                if (BitsOfAlignment >= 32) {
                  BitsOfAlignment = 0;
                }
                  
                Status = gDS->AllocateMemorySpace (
                                EfiGcdAllocateAnySearchBottomUp, 
                                EfiGcdMemoryTypeMemoryMappedIo, 
                                BitsOfAlignment,
                                AddrLen,
                                &BaseAddress,
                                mDriverImageHandle,
                                NULL
                                );
                  
                if (!EFI_ERROR (Status)) {
                  // We were able to allocate the PCI memory
                  RootBridgeInstance->ResAllocNode[Index].Base   = (UINTN)BaseAddress;
                  RootBridgeInstance->ResAllocNode[Index].Status = ResAllocated;
                    
                } else {
                  // Not able to allocate enough PCI memory
                  ReturnStatus = Status;  
                    
                  if (Status != EFI_OUT_OF_RESOURCES) {
                    RootBridgeInstance->ResAllocNode[Index].Length = 0;
                  } 
                  ASSERT (FALSE);
                }
                break;
                  
              case TypePMem32:                  
              case TypeMem64:                  
              case TypePMem64:
                  ReturnStatus = EFI_ABORTED;
                  break;  
              default:
                ASSERT (FALSE);
                break;
              }; //end switch
          }
        }
          
        List = List->ForwardLink;
      }
        
      return ReturnStatus;

    } else {
      return EFI_NOT_READY;
    }
    break;

  case EfiPciHostBridgeSetResources:
    break;

  case EfiPciHostBridgeFreeResources:
    ReturnStatus = EFI_SUCCESS;
    List = HostBridgeInstance->Head.ForwardLink;
    while (List != &HostBridgeInstance->Head) {
      RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
      for (Index = TypeIo; Index < TypeBus; Index++) {
        if (RootBridgeInstance->ResAllocNode[Index].Status == ResAllocated) {
          AddrLen = RootBridgeInstance->ResAllocNode[Index].Length;
          BaseAddress = RootBridgeInstance->ResAllocNode[Index].Base;
          switch (Index) {

          case TypeIo:  
            Status = gDS->FreeIoSpace (BaseAddress, AddrLen);                
            if (EFI_ERROR (Status)) {
              ReturnStatus = Status;
            }
            break;

          case TypeMem32:
            Status = gDS->FreeMemorySpace (BaseAddress, AddrLen);
            if (EFI_ERROR (Status)) {
              ReturnStatus = Status;
            }
            break;

          case TypePMem32:
            break;

          case TypeMem64:
            break;

          case TypePMem64:
            break; 

          default:
            ASSERT (FALSE);
            break;

          }; //end switch
          RootBridgeInstance->ResAllocNode[Index].Type      = Index;
          RootBridgeInstance->ResAllocNode[Index].Base      = 0;
          RootBridgeInstance->ResAllocNode[Index].Length    = 0;
          RootBridgeInstance->ResAllocNode[Index].Status    = ResNone;
        }
      }
        
      List = List->ForwardLink;
    }
              
    HostBridgeInstance->ResourceSubmited = FALSE;
    HostBridgeInstance->CanRestarted     = TRUE;      
    return ReturnStatus;
    break;

  case EfiPciHostBridgeEndResourceAllocation:
    HostBridgeInstance->CanRestarted = FALSE;
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }; // end switch
  
  return EFI_SUCCESS;  
}

EFI_STATUS
EFIAPI
GetNextRootBridge(
  IN       EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN OUT   EFI_HANDLE                                       *RootBridgeHandle
  )
/*++

Routine Description:
  Return the device handle of the next PCI root bridge that is associated with 
  this Host Bridge

Arguments:
  This -- The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance
  RootBridgeHandle -- Returns the device handle of the next PCI Root Bridge. 
                      On input, it holds the RootBridgeHandle returned by the most 
                      recent call to GetNextRootBridge().The handle for the first 
                      PCI Root Bridge is returned if RootBridgeHandle is NULL on input
    
Returns:

--*/  
{
  BOOLEAN                               NoRootBridge; 
  LIST_ENTRY                            *List; 
  PCI_HOST_BRIDGE_INSTANCE              *HostBridgeInstance;
  PCI_ROOT_BRIDGE_INSTANCE              *RootBridgeInstance;
  
  NoRootBridge = TRUE;
  HostBridgeInstance = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);
  List = HostBridgeInstance->Head.ForwardLink;
  
  
  while (List != &HostBridgeInstance->Head) {
    NoRootBridge = FALSE;
    RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
    if (*RootBridgeHandle == NULL) {
      //
      // Return the first Root Bridge Handle of the Host Bridge
      //
      *RootBridgeHandle = RootBridgeInstance->Handle;
      return EFI_SUCCESS;
    } else {
      if (*RootBridgeHandle == RootBridgeInstance->Handle) {
        //
        // Get next if have
        //
        List = List->ForwardLink;
        if (List!=&HostBridgeInstance->Head) {
          RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
          *RootBridgeHandle = RootBridgeInstance->Handle;
          return EFI_SUCCESS;  
        } else {
          return EFI_NOT_FOUND;
        }
      }
    }
      
    List = List->ForwardLink;
  } //end while
  
  if (NoRootBridge) {
    return EFI_NOT_FOUND;
  } else {
    return EFI_INVALID_PARAMETER;
  }
}

EFI_STATUS
EFIAPI
GetAttributes(
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN  EFI_HANDLE                                       RootBridgeHandle,
  OUT UINT64                                           *Attributes
  )
/*++

Routine Description:
  Returns the attributes of a PCI Root Bridge.

Arguments:
  This -- The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance
  RootBridgeHandle -- The device handle of the PCI Root Bridge 
                      that the caller is interested in
  Attribute -- The pointer to attributes of the PCI Root Bridge                    
    
Returns:

--*/    
{
  LIST_ENTRY                            *List; 
  PCI_HOST_BRIDGE_INSTANCE              *HostBridgeInstance;
  PCI_ROOT_BRIDGE_INSTANCE              *RootBridgeInstance;
  
  if (Attributes == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  HostBridgeInstance = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);
  List = HostBridgeInstance->Head.ForwardLink;
  
  while (List != &HostBridgeInstance->Head) {
    RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
    if (RootBridgeHandle == RootBridgeInstance->Handle) {
      *Attributes = RootBridgeInstance->RootBridgeAttrib;
      return EFI_SUCCESS;
    }
    List = List->ForwardLink;
  }
  
  //
  // RootBridgeHandle is not an EFI_HANDLE 
  // that was returned on a previous call to GetNextRootBridge()
  //
  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
StartBusEnumeration(
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN  EFI_HANDLE                                       RootBridgeHandle,
  OUT VOID                                             **Configuration
  )
/*++

Routine Description:
  This is the request from the PCI enumerator to set up 
  the specified PCI Root Bridge for bus enumeration process. 

Arguments:
  This -- The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance
  RootBridgeHandle -- The PCI Root Bridge to be set up
  Configuration -- Pointer to the pointer to the PCI bus resource descriptor
    
Returns:

--*/
{
  LIST_ENTRY                            *List; 
  PCI_HOST_BRIDGE_INSTANCE              *HostBridgeInstance;
  PCI_ROOT_BRIDGE_INSTANCE              *RootBridgeInstance;
  VOID                                  *Buffer;
  UINT8                                 *Temp;
  UINT64                                BusStart;
  UINT64                                BusEnd;
  
  HostBridgeInstance = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);
  List = HostBridgeInstance->Head.ForwardLink;
  
  while (List != &HostBridgeInstance->Head) {
    RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
    if (RootBridgeHandle == RootBridgeInstance->Handle) {
      //
      // Set up the Root Bridge for Bus Enumeration
      //
      BusStart = RootBridgeInstance->BusBase;
      BusEnd   = RootBridgeInstance->BusLimit;
      //
      // Program the Hardware(if needed) if error return EFI_DEVICE_ERROR
      //
      
      Buffer = AllocatePool (sizeof(EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) + sizeof(EFI_ACPI_END_TAG_DESCRIPTOR));
      if (Buffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      
      Temp = (UINT8 *)Buffer;
      
      ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Temp)->Desc = 0x8A;
      ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Temp)->Len  = 0x2B;
      ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Temp)->ResType = 2;
      ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Temp)->GenFlag = 0; 
      ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Temp)->SpecificFlag = 0;
      ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Temp)->AddrSpaceGranularity = 0;
      ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Temp)->AddrRangeMin = BusStart;
      ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Temp)->AddrRangeMax = 0;
      ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Temp)->AddrTranslationOffset = 0;       
      ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Temp)->AddrLen = BusEnd - BusStart + 1;
      
      Temp = Temp + sizeof(EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);
      ((EFI_ACPI_END_TAG_DESCRIPTOR *)Temp)->Desc = 0x79;      
      ((EFI_ACPI_END_TAG_DESCRIPTOR *)Temp)->Checksum = 0x0;
      
      *Configuration = Buffer;      
      return EFI_SUCCESS;
    }
    List = List->ForwardLink;
  }
  
  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
SetBusNumbers(
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN EFI_HANDLE                                       RootBridgeHandle,
  IN VOID                                             *Configuration
  )
/*++

Routine Description:
  This function programs the PCI Root Bridge hardware so that 
  it decodes the specified PCI bus range

Arguments:
  This -- The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance
  RootBridgeHandle -- The PCI Root Bridge whose bus range is to be programmed
  Configuration -- The pointer to the PCI bus resource descriptor
    
Returns:

--*/    
{
  LIST_ENTRY                            *List; 
  PCI_HOST_BRIDGE_INSTANCE              *HostBridgeInstance;
  PCI_ROOT_BRIDGE_INSTANCE              *RootBridgeInstance;
  UINT8                                 *Ptr;
  UINTN                                 BusStart;
  UINTN                                 BusEnd;
  UINTN                                 BusLen;
  
  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }
    
  Ptr = Configuration;
  
  //
  // Check the Configuration is valid
  //
  if(*Ptr != ACPI_ADDRESS_SPACE_DESCRIPTOR) {
    return EFI_INVALID_PARAMETER;
  }
  
  if (((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Ptr)->ResType != 2) {
    return EFI_INVALID_PARAMETER;
  }

  Ptr += sizeof(EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);
  if (*Ptr != ACPI_END_TAG_DESCRIPTOR) {
    return EFI_INVALID_PARAMETER;
  }
   
  HostBridgeInstance = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);
  List = HostBridgeInstance->Head.ForwardLink;
  
  Ptr = Configuration;
  
  while (List != &HostBridgeInstance->Head) {
    RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
    if (RootBridgeHandle == RootBridgeInstance->Handle) {
      BusStart = (UINTN)((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Ptr)->AddrRangeMin;
      BusLen = (UINTN)((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Ptr)->AddrLen;
      BusEnd = BusStart + BusLen - 1;
      
      if (BusStart > BusEnd) {
        return EFI_INVALID_PARAMETER;
      }
      
      if ((BusStart < RootBridgeInstance->BusBase) || (BusEnd > RootBridgeInstance->BusLimit)) {
        return EFI_INVALID_PARAMETER;
      }
      
      //
      // Update the Bus Range
      //
      RootBridgeInstance->ResAllocNode[TypeBus].Base   = BusStart;
      RootBridgeInstance->ResAllocNode[TypeBus].Length = BusLen;
      RootBridgeInstance->ResAllocNode[TypeBus].Status = ResAllocated;
      
      //
      // Program the Root Bridge Hardware
      //
       
      return EFI_SUCCESS;
    }
    
    List = List->ForwardLink;
  }
  
  return EFI_INVALID_PARAMETER;
}


EFI_STATUS
EFIAPI
SubmitResources(
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN EFI_HANDLE                                       RootBridgeHandle,
  IN VOID                                             *Configuration
  )
/*++

Routine Description:
  Submits the I/O and memory resource requirements for the specified PCI Root Bridge
  
Arguments:
  This -- The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance
  RootBridgeHandle -- The PCI Root Bridge whose I/O and memory resource requirements 
                      are being submitted
  Configuration -- The pointer to the PCI I/O and PCI memory resource descriptor                    
Returns:

--*/    
{
  LIST_ENTRY                            *List; 
  PCI_HOST_BRIDGE_INSTANCE              *HostBridgeInstance;
  PCI_ROOT_BRIDGE_INSTANCE              *RootBridgeInstance;
  UINT8                                 *Temp;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR     *ptr;
  UINT64                                AddrLen;
  UINT64                                Alignment;
  
  //
  // Check the input parameter: Configuration
  //
  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  HostBridgeInstance = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);
  List = HostBridgeInstance->Head.ForwardLink;
  
  Temp = (UINT8 *)Configuration;
  while ( *Temp == 0x8A) { 
    Temp += sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) ;
  }
  if (*Temp != 0x79) {
    return EFI_INVALID_PARAMETER;
  }
  
  Temp = (UINT8 *)Configuration;
  while (List != &HostBridgeInstance->Head) {
    RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
    if (RootBridgeHandle == RootBridgeInstance->Handle) {
      while ( *Temp == 0x8A) {
        ptr = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Temp ;

        //
        // Check Address Length
        //
        if (ptr->AddrLen > 0xffffffff) {
          return EFI_INVALID_PARAMETER;
        }

        //
        // Check address range alignment
        //
        if (ptr->AddrRangeMax >= 0xffffffff || ptr->AddrRangeMax != (GetPowerOfTwo64 (ptr->AddrRangeMax + 1) - 1)) {
          return EFI_INVALID_PARAMETER;
        }
        
        switch (ptr->ResType) {

        case 0:
            
          //
          // Check invalid Address Sapce Granularity
          //
          if (ptr->AddrSpaceGranularity != 32) {
            return EFI_INVALID_PARAMETER;
          }
            
          //
          // check the memory resource request is supported by PCI root bridge
          //
          if (RootBridgeInstance->RootBridgeAttrib == EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM &&
               ptr->SpecificFlag == 0x06) {
            return EFI_INVALID_PARAMETER;
          }
            
          AddrLen = ptr->AddrLen;
          Alignment = ptr->AddrRangeMax;
          if (ptr->AddrSpaceGranularity == 32) {
            if (ptr->SpecificFlag == 0x06) {
              //
              // Apply from GCD
              //
              RootBridgeInstance->ResAllocNode[TypePMem32].Status = ResSubmitted;
            } else {
              RootBridgeInstance->ResAllocNode[TypeMem32].Length = AddrLen;
              RootBridgeInstance->ResAllocNode[TypeMem32].Alignment = Alignment;
              RootBridgeInstance->ResAllocNode[TypeMem32].Status = ResRequested; 
              HostBridgeInstance->ResourceSubmited = TRUE;
            }
          }

          if (ptr->AddrSpaceGranularity == 64) {
            if (ptr->SpecificFlag == 0x06) {
              RootBridgeInstance->ResAllocNode[TypePMem64].Status = ResSubmitted;
            } else {
              RootBridgeInstance->ResAllocNode[TypeMem64].Status = ResSubmitted;
            }
          }
          break;

        case 1:
          AddrLen = (UINTN)ptr->AddrLen;
          Alignment = (UINTN)ptr->AddrRangeMax;
          RootBridgeInstance->ResAllocNode[TypeIo].Length  = AddrLen;
          RootBridgeInstance->ResAllocNode[TypeIo].Alignment = Alignment;
          RootBridgeInstance->ResAllocNode[TypeIo].Status  = ResRequested;
          HostBridgeInstance->ResourceSubmited = TRUE; 
          break;

        default:
            break;
        };
    
        Temp += sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) ;
      } 
      
      return EFI_SUCCESS;
    }
    
    List = List->ForwardLink;
  }
  
  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
GetProposedResources(
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN  EFI_HANDLE                                       RootBridgeHandle,
  OUT VOID                                             **Configuration
  )
/*++

Routine Description:
  This function returns the proposed resource settings for the specified 
  PCI Root Bridge

Arguments:
  This -- The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance
  RootBridgeHandle -- The PCI Root Bridge handle
  Configuration -- The pointer to the pointer to the PCI I/O 
                   and memory resource descriptor
    
Returns:

--*/    
{
  LIST_ENTRY                            *List; 
  PCI_HOST_BRIDGE_INSTANCE              *HostBridgeInstance;
  PCI_ROOT_BRIDGE_INSTANCE              *RootBridgeInstance;
  UINTN                                 Index;
  UINTN                                 Number; 
  VOID                                  *Buffer; 
  UINT8                                 *Temp;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR     *ptr;
  UINT64                                ResStatus;
    
  Buffer = NULL;
  Number = 0;
  //
  // Get the Host Bridge Instance from the resource allocation protocol
  //
  HostBridgeInstance = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);
  List = HostBridgeInstance->Head.ForwardLink;
  
  //
  // Enumerate the root bridges in this host bridge
  //
  while (List != &HostBridgeInstance->Head) {
    RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
    if (RootBridgeHandle == RootBridgeInstance->Handle) {
      for (Index = 0; Index < TypeBus; Index ++) {
        if (RootBridgeInstance->ResAllocNode[Index].Status != ResNone) {
          Number ++;
        }  
      }
      
      if (Number ==  0) {
        return EFI_INVALID_PARAMETER;
      }

      Buffer = AllocateZeroPool (Number * sizeof(EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) + sizeof(EFI_ACPI_END_TAG_DESCRIPTOR));
      if (Buffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      
      Temp = Buffer;
      for (Index = 0; Index < TypeBus; Index ++) {
        if (RootBridgeInstance->ResAllocNode[Index].Status != ResNone) {
          ptr = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Temp ;
          ResStatus = RootBridgeInstance->ResAllocNode[Index].Status;
          
          switch (Index) {

          case TypeIo:
            //
            // Io
            //
            ptr->Desc = 0x8A;
            ptr->Len  = 0x2B;
            ptr->ResType = 1;
            ptr->GenFlag = 0; 
            ptr->SpecificFlag = 0;
            ptr->AddrRangeMin = RootBridgeInstance->ResAllocNode[Index].Base;
            ptr->AddrRangeMax = 0;
            ptr->AddrTranslationOffset = \
                 (ResStatus == ResAllocated) ? EFI_RESOURCE_SATISFIED : EFI_RESOURCE_LESS;
            ptr->AddrLen = RootBridgeInstance->ResAllocNode[Index].Length;
            break;

          case TypeMem32:
            //
            // Memory 32
            // 
            ptr->Desc = 0x8A;
            ptr->Len  = 0x2B;
            ptr->ResType = 0;
            ptr->GenFlag = 0; 
            ptr->SpecificFlag = 0;
            ptr->AddrSpaceGranularity = 32;
            ptr->AddrRangeMin = RootBridgeInstance->ResAllocNode[Index].Base;
            ptr->AddrRangeMax = 0;
            ptr->AddrTranslationOffset = \
                 (ResStatus == ResAllocated) ? EFI_RESOURCE_SATISFIED : EFI_RESOURCE_LESS;              
            ptr->AddrLen = RootBridgeInstance->ResAllocNode[Index].Length;
            break;

          case TypePMem32:
            //
            // Prefetch memory 32
            //
            ptr->Desc = 0x8A;
            ptr->Len  = 0x2B;
            ptr->ResType = 0;
            ptr->GenFlag = 0; 
            ptr->SpecificFlag = 6;
            ptr->AddrSpaceGranularity = 32;
            ptr->AddrRangeMin = 0;
            ptr->AddrRangeMax = 0;
            ptr->AddrTranslationOffset = EFI_RESOURCE_NONEXISTENT;       
            ptr->AddrLen = 0;
            break;

          case TypeMem64:
            //
            // Memory 64
            //
            ptr->Desc = 0x8A;
            ptr->Len  = 0x2B;
            ptr->ResType = 0;
            ptr->GenFlag = 0; 
            ptr->SpecificFlag = 0;
            ptr->AddrSpaceGranularity = 64;
            ptr->AddrRangeMin = 0;
            ptr->AddrRangeMax = 0;
            ptr->AddrTranslationOffset = EFI_RESOURCE_NONEXISTENT;       
            ptr->AddrLen = 0;
            break;

          case TypePMem64:
            //
            // Prefetch memory 64
            //
            ptr->Desc = 0x8A;
            ptr->Len  = 0x2B;
            ptr->ResType = 0;
            ptr->GenFlag = 0; 
            ptr->SpecificFlag = 6;
            ptr->AddrSpaceGranularity = 64;
            ptr->AddrRangeMin = 0;
            ptr->AddrRangeMax = 0;
            ptr->AddrTranslationOffset = EFI_RESOURCE_NONEXISTENT;       
            ptr->AddrLen = 0;
            break;
          };
          
          Temp += sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);
        }  
      }
      
      ((EFI_ACPI_END_TAG_DESCRIPTOR *)Temp)->Desc = 0x79;      
      ((EFI_ACPI_END_TAG_DESCRIPTOR *)Temp)->Checksum = 0x0;
      
      *Configuration = Buffer;      
       
      return EFI_SUCCESS;
    }
    
    List = List->ForwardLink;
  }
  
  return EFI_INVALID_PARAMETER;
}

STATIC
VOID
UpdateRootBridgeAttributes (
  IN  PCI_ROOT_BRIDGE_INSTANCE                     *RootBridge,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS  PciAddress
  )
{
  EFI_STATUS                 Status;
  PCI_TYPE01                 PciConfigurationHeader;
  UINT64                     Attributes;

  //
  // Read the PCI Configuration Header for the device
  //
  Status = RootBridge->Io.Pci.Read (
    &RootBridge->Io,
    EfiPciWidthUint16,
    EFI_PCI_ADDRESS(
      PciAddress.Bus,
      PciAddress.Device,
      PciAddress.Function,
      0
      ),
    sizeof (PciConfigurationHeader) / sizeof (UINT16),
    &PciConfigurationHeader
    );
  if (EFI_ERROR (Status)) {
    return;
  }

  Attributes = RootBridge->Attributes;

  //
  // Look for devices with the VGA Palette Snoop enabled in the COMMAND register of the PCI Config Header
  //
  if (PciConfigurationHeader.Hdr.Command & 0x20) {
    Attributes |= EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO;
  }

  //
  // If the device is a PCI-PCI Bridge, then look at the Subordinate Bus Number
  //
  if (IS_PCI_BRIDGE(&PciConfigurationHeader)) {
    //
    // Look at the PPB Configuration for legacy decoding attributes
    //
    if (PciConfigurationHeader.Bridge.BridgeControl & 0x04) {
      Attributes |= EFI_PCI_ATTRIBUTE_ISA_IO;
      Attributes |= EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO;
    }
    if (PciConfigurationHeader.Bridge.BridgeControl & 0x08) {
      Attributes |= EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO;
      Attributes |= EFI_PCI_ATTRIBUTE_VGA_MEMORY;
      Attributes |= EFI_PCI_ATTRIBUTE_VGA_IO;
    }
  } else {
    //
    // See if the PCI device is an IDE controller
    //
    if (PciConfigurationHeader.Hdr.ClassCode[2] == 0x01 &&
        PciConfigurationHeader.Hdr.ClassCode[1] == 0x01    ) {
      if (PciConfigurationHeader.Hdr.ClassCode[0] & 0x80) {
        Attributes |= EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO;
        Attributes |= EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO;
      }
      if (PciConfigurationHeader.Hdr.ClassCode[0] & 0x01) {
        Attributes |= EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO;
      }
      if (PciConfigurationHeader.Hdr.ClassCode[0] & 0x04) {
        Attributes |= EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO;
      }
    }

    //
    // See if the PCI device is a legacy VGA controller
    //
    if (PciConfigurationHeader.Hdr.ClassCode[2] == 0x00 &&
        PciConfigurationHeader.Hdr.ClassCode[1] == 0x01    ) {
      Attributes |= EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO;
      Attributes |= EFI_PCI_ATTRIBUTE_VGA_MEMORY;
      Attributes |= EFI_PCI_ATTRIBUTE_VGA_IO;
    }

    //
    // See if the PCI device is a standard VGA controller
    //
    if (PciConfigurationHeader.Hdr.ClassCode[2] == 0x03 &&
        PciConfigurationHeader.Hdr.ClassCode[1] == 0x00    ) {
      Attributes |= EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO;
      Attributes |= EFI_PCI_ATTRIBUTE_VGA_MEMORY;
      Attributes |= EFI_PCI_ATTRIBUTE_VGA_IO;
    }
  }

  RootBridge->Attributes = Attributes;
  RootBridge->Supports = Attributes;
}

EFI_STATUS
EFIAPI
PreprocessController (
  IN  struct _EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *This,
  IN  EFI_HANDLE                                                RootBridgeHandle,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS               PciAddress,
  IN  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE              Phase
  )
/*++

Routine Description:
  This function is called for all the PCI controllers that the PCI 
  bus driver finds. Can be used to Preprogram the controller.

Arguments:
  This -- The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance
  RootBridgeHandle -- The PCI Root Bridge handle
  PciBusAddress -- Address of the controller on the PCI bus
  Phase         -- The Phase during resource allocation
    
Returns:
  EFI_SUCCESS
--*/    
{
  PCI_HOST_BRIDGE_INSTANCE              *HostBridgeInstance;
  PCI_ROOT_BRIDGE_INSTANCE              *RootBridgeInstance;
  LIST_ENTRY                            *List; 

  HostBridgeInstance = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);
  List = HostBridgeInstance->Head.ForwardLink;

  //
  // Enumerate the root bridges in this host bridge
  //
  while (List != &HostBridgeInstance->Head) {
    RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
    if (RootBridgeHandle == RootBridgeInstance->Handle) {
      UpdateRootBridgeAttributes (
        RootBridgeInstance,
        PciAddress
        );
      return EFI_SUCCESS;
    }
    List = List->ForwardLink;
  }

  return EFI_INVALID_PARAMETER;
}
