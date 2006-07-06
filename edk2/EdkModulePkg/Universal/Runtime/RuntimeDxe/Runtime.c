/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Runtime.c

Abstract:

  Runtime Architectural Protocol as defined in the DXE CIS

  This code is used to produce the EFI runtime virtual switch over

  THIS IS VERY DANGEROUS CODE BE VERY CAREFUL IF YOU CHANGE IT

  The transition for calling EFI Runtime functions in physical mode to calling
  them in virtual mode is very very complex. Every pointer in needs to be 
  converted from physical mode to virtual mode. Be very careful walking linked
  lists! Then to make it really hard the code it's self needs be relocated into
  the new virtual address space.

  So here is the concept. The code in this module will never ever be called in
  virtual mode. This is the code that collects the information needed to convert
  to virtual mode (DXE core registers runtime stuff with this code). Since this 
  code is used to fixup all runtime images, it CAN NOT fix it's self up. So some
  code has to stay behind and that is us.

  Also you need to be careful about when you allocate memory, as once we are in 
  runtime (including our EVT_SIGNAL_EXIT_BOOT_SERVICES event) you can no longer 
  allocate memory.

  Any runtime driver that gets loaded before us will not be callable in virtual 
  mode. This is due to the fact that the DXE core can not register the info 
  needed with us. This is good, since it keeps the code in this file from 
  getting registered.


Revision History:

  - Move the CalculateCrc32 function from Runtime Arch Protocol to Boot Service.
  Runtime Arch Protocol definition no longer contains CalculateCrc32. Boot Service
  Table now contains an item named CalculateCrc32.

--*/


#include "Runtime.h"

//
// This is a only short term solution.
// There is a change coming to the Runtime AP that
// will make it so the Runtime driver will not have to allocate any buffers. 
//
#define MAX_RUNTIME_IMAGE_NUM (64)
#define MAX_RUNTIME_EVENT_NUM (64)
RUNTIME_IMAGE_RELOCATION_DATA mRuntimeImageBuffer[MAX_RUNTIME_IMAGE_NUM];
RUNTIME_NOTIFY_EVENT_DATA     mRuntimeEventBuffer[MAX_RUNTIME_EVENT_NUM];
UINTN                         mRuntimeImageNumber;
UINTN                         mRuntimeEventNumber;

//
// The handle onto which the Runtime Architectural Protocol instance is installed
//
EFI_HANDLE                    mRuntimeHandle = NULL;

//
// The Runtime Architectural Protocol instance produced by this driver
//
EFI_RUNTIME_ARCH_PROTOCOL     mRuntime = {
  RuntimeDriverRegisterImage,
  RuntimeDriverRegisterEvent
};

//
// Global Variables
//
LIST_ENTRY                    mRelocationList             = INITIALIZE_LIST_HEAD_VARIABLE(mRelocationList);
LIST_ENTRY                    mEventList                  = INITIALIZE_LIST_HEAD_VARIABLE(mEventList);
BOOLEAN                       mEfiVirtualMode             = FALSE;
EFI_GUID                      mLocalEfiUgaIoProtocolGuid  = EFI_UGA_IO_PROTOCOL_GUID;
EFI_MEMORY_DESCRIPTOR         *mVirtualMap                = NULL;
UINTN                         mVirtualMapDescriptorSize;
UINTN                         mVirtualMapMaxIndex;

EFI_LOADED_IMAGE_PROTOCOL     *mMyLoadedImage;

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
STATIC EFI_GUID mEfiCapsuleHeaderGuid = EFI_CAPSULE_GUID;
#endif
//
// Worker Functions
//
VOID
RuntimeDriverCalculateEfiHdrCrc (
  IN OUT EFI_TABLE_HEADER  *Hdr
  )
/*++

Routine Description:

  Calcualte the 32-bit CRC in a EFI table using the Runtime Drivers
  internal function.  The EFI Boot Services Table can not be used because
  the EFI Boot Services Table was destroyed at ExitBootServices()

Arguments:

  Hdr  - Pointer to an EFI standard header

Returns:

  None

--*/
{
  UINT32  Crc;

  Hdr->CRC32  = 0;

  Crc         = 0;
  RuntimeDriverCalculateCrc32 ((UINT8 *) Hdr, Hdr->HeaderSize, &Crc);
  Hdr->CRC32 = Crc;
}

EFI_STATUS
EFIAPI
RuntimeDriverRegisterImage (
  IN  EFI_RUNTIME_ARCH_PROTOCOL  *This,
  IN  EFI_PHYSICAL_ADDRESS       ImageBase,
  IN  UINTN                      ImageSize,
  IN  VOID                       *RelocationData
  )
/*++

Routine Description:

  When a SetVirtualAddressMap() is performed all the runtime images loaded by 
  DXE must be fixed up with the new virtual address map. To facilitate this the 
  Runtime Architectural Protocol needs to be informed of every runtime driver 
  that is registered.  All the runtime images loaded by DXE should be registered 
  with this service by the DXE Core when ExitBootServices() is called.  The 
  images that are registered with this service must have successfully been 
  loaded into memory with the Boot Service LoadImage().  As a result, no 
  parameter checking needs to be performed.

Arguments:

  This           - The EFI_RUNTIME_ARCH_PROTOCOL instance. 

  ImageBase      - Start of image that has been loaded in memory. It is either 
                   a pointer to the DOS or PE header of the image.

  ImageSize      - Size of the image in bytes.

  RelocationData - Information about the fixups that were performed on ImageBase 
                   when it was loaded into memory. This information is needed 
                   when the virtual mode fix-ups are reapplied so that data that 
                   has been programmatically updated will not be fixed up. If 
                   code updates a global variable the code is responsible for 
                   fixing up the variable for virtual mode.

Returns: 

  EFI_SUCCESS          - The ImageBase has been registered.

--*/
{
  RUNTIME_IMAGE_RELOCATION_DATA *RuntimeImage;

  if (mMyLoadedImage->ImageBase == (VOID *) (UINTN) ImageBase) {
    //
    // We don't want to relocate our selves, as we only run in physical mode.
    //
    return EFI_SUCCESS;
  }

  RuntimeImage = &mRuntimeImageBuffer[mRuntimeImageNumber];
  mRuntimeImageNumber++;
  ASSERT (mRuntimeImageNumber < MAX_RUNTIME_IMAGE_NUM);

  RuntimeImage->Valid           = TRUE;
  RuntimeImage->ImageBase       = ImageBase;
  RuntimeImage->ImageSize       = ImageSize;
  RuntimeImage->RelocationData  = RelocationData;

  InsertTailList (&mRelocationList, &RuntimeImage->Link);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RuntimeDriverRegisterEvent (
  IN EFI_RUNTIME_ARCH_PROTOCOL  *This,
  IN UINT32                     Type,
  IN EFI_TPL                    NotifyTpl,
  IN EFI_EVENT_NOTIFY           NotifyFunction,
  IN VOID                       *NotifyContext,
  IN EFI_EVENT                  *Event
  )
/*++

Routine Description:

  This function is used to support the required runtime events. Currently only 
  runtime events of type EFI_EVENT_SIGNAL_VIRTUAL_ADDRESS_CHANGE needs to be 
  registered with this service.  All the runtime events that exist in the DXE 
  Core should be registered with this service when ExitBootServices() is called.  
  All the events that are registered with this service must have been created 
  with the Boot Service CreateEvent().  As a result, no parameter checking needs 
  to be performed.

Arguments:

  This           - The EFI_RUNTIME_ARCH_PROTOCOL instance. 

  Type           - The same as Type passed into CreateEvent().

  NotifyTpl      - The same as NotifyTpl passed into CreateEvent().

  NotifyFunction - The same as NotifyFunction passed into CreateEvent().

  NotifyContext  - The same as NotifyContext passed into CreateEvent().

  Event          - The EFI_EVENT returned by CreateEvent().  Event must be in 
                   runtime memory.

Returns: 

  EFI_SUCCESS          - The Event has been registered.

--*/
{
  RUNTIME_NOTIFY_EVENT_DATA *RuntimeEvent;

  RuntimeEvent = &mRuntimeEventBuffer[mRuntimeEventNumber];
  mRuntimeEventNumber++;
  ASSERT (mRuntimeEventNumber < MAX_RUNTIME_EVENT_NUM);

  RuntimeEvent->Type            = Type;
  RuntimeEvent->NotifyTpl       = NotifyTpl;
  RuntimeEvent->NotifyFunction  = NotifyFunction;
  RuntimeEvent->NotifyContext   = NotifyContext;
  RuntimeEvent->Event           = Event;

  InsertTailList (&mEventList, &RuntimeEvent->Link);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RuntimeDriverConvertPointer (
  IN     UINTN  DebugDisposition,
  IN OUT VOID   **ConvertAddress
  )
{
  UINTN                 Address;
  VOID                  *PlabelConvertAddress;
  UINT64                VirtEndOfRange;
  EFI_MEMORY_DESCRIPTOR *VirtEntry;
  UINTN                 Index;

  //
  // Make sure ConvertAddress is a valid pointer
  //
  if (ConvertAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Get the address to convert
  //
  Address = (UINTN) *ConvertAddress;

  //
  // If this is a null pointer, return if it's allowed
  //
  if (Address == 0) {
    if (DebugDisposition & EFI_OPTIONAL_POINTER) {
      return EFI_SUCCESS;
    }

    return EFI_INVALID_PARAMETER;
  }

  PlabelConvertAddress  = NULL;
  VirtEntry             = mVirtualMap;
  for (Index = 0; Index < mVirtualMapMaxIndex; Index++) {
    //
    // To prevent the inclusion of 64-bit math functions a UINTN was placed in
    //  front of VirtEntry->NumberOfPages to cast it to a 32-bit thing on IA-32
    //  platforms. If you get this ASSERT remove the UINTN and do a 64-bit
    //  multiply.
    //
    ASSERT ((VirtEntry->NumberOfPages < 0xffffffff) || (sizeof (UINTN) > 4));

    if ((VirtEntry->Attribute & EFI_MEMORY_RUNTIME) == EFI_MEMORY_RUNTIME) {
      if (Address >= VirtEntry->PhysicalStart) {
        VirtEndOfRange = VirtEntry->PhysicalStart + (((UINTN) VirtEntry->NumberOfPages) * EFI_PAGE_SIZE);
        if (Address < VirtEndOfRange) {
          //
          // Compute new address
          //
          *ConvertAddress = (VOID *) (Address - (UINTN) VirtEntry->PhysicalStart + (UINTN) VirtEntry->VirtualStart);
          return EFI_SUCCESS;
        } else if (Address < (VirtEndOfRange + 0x200000)) {
          //
          // On Itanium GP defines a window +/- 2 MB inside an image.
          // The compiler may asign a GP value outside of the image. Thus
          // it could fall out side of any of our valid regions
          //
          PlabelConvertAddress = (VOID *) (Address - (UINTN) VirtEntry->PhysicalStart + (UINTN) VirtEntry->VirtualStart);
        }
      }
    }

    VirtEntry = NextMemoryDescriptor (VirtEntry, mVirtualMapDescriptorSize);
  }

  if (DebugDisposition & EFI_IPF_GP_POINTER) {
    //
    // If it's an IPF GP and the GP was outside the image handle that case.
    //
    if (PlabelConvertAddress != NULL) {
      *ConvertAddress = PlabelConvertAddress;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
RuntimeDriverConvertInternalPointer (
  IN OUT VOID   **ConvertAddress
  )
{
  return RuntimeDriverConvertPointer (0x0, ConvertAddress);
}

EFI_STATUS
EFIAPI
RuntimeDriverSetVirtualAddressMap (
  IN UINTN                  MemoryMapSize,
  IN UINTN                  DescriptorSize,
  IN UINT32                 DescriptorVersion,
  IN EFI_MEMORY_DESCRIPTOR  *VirtualMap
  )
{
  RUNTIME_NOTIFY_EVENT_DATA     *RuntimeEvent;
  RUNTIME_IMAGE_RELOCATION_DATA *RuntimeImage;
  LIST_ENTRY                    *Link;
  UINTN                         Index;
  UINTN                         Index1;
  EFI_DRIVER_OS_HANDOFF_HEADER  *DriverOsHandoffHeader;
  EFI_DRIVER_OS_HANDOFF         *DriverOsHandoff;
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
  EFI_CAPSULE_TABLE             *CapsuleTable; 
#endif

  //
  // Can only switch to virtual addresses once the memory map is locked down,
  // and can only set it once
  //
  if (!EfiAtRuntime () || mEfiVirtualMode) {
    return EFI_UNSUPPORTED;
  }
  //
  // Only understand the original descriptor format
  //
  if (DescriptorVersion != EFI_MEMORY_DESCRIPTOR_VERSION || DescriptorSize < sizeof (EFI_MEMORY_DESCRIPTOR)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // BugBug: Add code here to verify the memory map. We should
  //  cache a copy of the system memory map in the EFI System Table
  //  as a GUID pointer pair.
  //
  //
  // Make sure all virtual translations are satisfied
  //
  mVirtualMapMaxIndex = MemoryMapSize / DescriptorSize;

  //
  // BugBug :The following code does not work hence commented out.
  // Need to be replaced by something that works.
  //
  //  VirtEntry = VirtualMap;
  //  for (Index = 0; Index < mVirtualMapMaxIndex; Index++) {
  //    if (((VirtEntry->Attribute & EFI_MEMORY_RUNTIME) == EFI_MEMORY_RUNTIME) &&
  //        (VirtEntry->VirtualStart != 0) ) {
  //        return EFI_NO_MAPPING;
  //    }
  //    VirtEntry = NextMemoryDescriptor(VirtEntry, DescriptorSize);
  //  }
  //
  // We are now committed to go to virtual mode, so lets get to it!
  //
  mEfiVirtualMode = TRUE;

  //
  // ConvertPointer() needs this mVirtualMap to do the conversion. So set up
  // globals we need to parse the virtual address map.
  //
  mVirtualMapDescriptorSize = DescriptorSize;
  mVirtualMap               = VirtualMap;

  //
  // Signal all the EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE events.
  // The core call RuntimeDriverRegisterEvent() for
  // every runtime event and we stored them in the mEventList
  //
  //
  // Currently the bug in StatusCode/RuntimeLib has been fixed, it will
  // check whether in Runtime or not (this is judged by looking at
  // mEfiAtRuntime global So this ReportStatusCode will work
  //
  REPORT_STATUS_CODE (
          EFI_PROGRESS_CODE,
          (EFI_SOFTWARE_EFI_BOOT_SERVICE | EFI_SW_RS_PC_SET_VIRTUAL_ADDRESS_MAP)
          );

  //
  // BugBug - Commented out for now because the status code driver is not
  // ready for runtime yet. The Status Code driver calls data hub with does
  // a bunch of Boot Service things (locks, AllocatePool) and hangs somewhere
  // on the way.
  //
  //  ReportStatusCode (
  //        EfiProgressCode,  EfiMaxErrorSeverity,
  //        0x03, 0x01, 12, // ReadyToBoot Progress code
  //        0x00, 30, L"ConvertPointer"
  //        );
  //
  for (Link = mEventList.ForwardLink; Link != &mEventList; Link = Link->ForwardLink) {
    RuntimeEvent = _CR (Link, RUNTIME_NOTIFY_EVENT_DATA, Link);
    if ((RuntimeEvent->Type & EFI_EVENT_SIGNAL_VIRTUAL_ADDRESS_CHANGE) == EFI_EVENT_SIGNAL_VIRTUAL_ADDRESS_CHANGE) {
      RuntimeEvent->NotifyFunction (
                      RuntimeEvent->Event,
                      RuntimeEvent->NotifyContext
                      );
    }
  }
  //
  // Relocate runtime images. Runtime images loaded before the runtime AP was
  // started will not be relocated, since they would not have gotten registered.
  // This includes the code in this file.
  //
  for (Link = mRelocationList.ForwardLink; Link != &mRelocationList; Link = Link->ForwardLink) {
    RuntimeImage = _CR (Link, RUNTIME_IMAGE_RELOCATION_DATA, Link);
    if (RuntimeImage->Valid) {
      RelocatePeImageForRuntime (RuntimeImage);
    }
  }
  //
  // Convert all the Runtime Services except ConvertPointer() and SetVirtualAddressMap()
  // and recompute the CRC-32
  //
  RuntimeDriverConvertInternalPointer ((VOID **) &gRT->GetTime);
  RuntimeDriverConvertInternalPointer ((VOID **) &gRT->SetTime);
  RuntimeDriverConvertInternalPointer ((VOID **) &gRT->GetWakeupTime);
  RuntimeDriverConvertInternalPointer ((VOID **) &gRT->SetWakeupTime);
  RuntimeDriverConvertInternalPointer ((VOID **) &gRT->ResetSystem);
  RuntimeDriverConvertInternalPointer ((VOID **) &gRT->GetNextHighMonotonicCount);
  RuntimeDriverConvertInternalPointer ((VOID **) &gRT->GetVariable);
  RuntimeDriverConvertInternalPointer ((VOID **) &gRT->SetVariable);
  RuntimeDriverConvertInternalPointer ((VOID **) &gRT->GetNextVariableName);
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
  RuntimeDriverConvertInternalPointer ((VOID **) &gRT->QueryVariableInfo);
  RuntimeDriverConvertInternalPointer ((VOID **) &gRT->UpdateCapsule);
  RuntimeDriverConvertInternalPointer ((VOID **) &gRT->QueryCapsuleCapabilities);
#endif
  RuntimeDriverCalculateEfiHdrCrc (&gRT->Hdr);

  //
  // Convert the UGA OS Handoff Table if it is present in the Configuration Table
  //
  for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
    if (CompareGuid (&mLocalEfiUgaIoProtocolGuid, &(gST->ConfigurationTable[Index].VendorGuid))) {
      DriverOsHandoffHeader = gST->ConfigurationTable[Index].VendorTable;
      for (Index1 = 0; Index1 < DriverOsHandoffHeader->NumberOfEntries; Index1++) {
        DriverOsHandoff = (EFI_DRIVER_OS_HANDOFF *)
          (
            (UINTN) DriverOsHandoffHeader +
            DriverOsHandoffHeader->HeaderSize +
            Index1 *
            DriverOsHandoffHeader->SizeOfEntries
          );
        RuntimeDriverConvertPointer (EFI_OPTIONAL_POINTER, (VOID **) &DriverOsHandoff->DevicePath);
        RuntimeDriverConvertPointer (EFI_OPTIONAL_POINTER, (VOID **) &DriverOsHandoff->PciRomImage);
      }

      RuntimeDriverConvertPointer (EFI_OPTIONAL_POINTER, (VOID **) &(gST->ConfigurationTable[Index].VendorTable));
    }
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
    if (CompareGuid (&mEfiCapsuleHeaderGuid, &(gST->ConfigurationTable[Index].VendorGuid))) {
      CapsuleTable = gST->ConfigurationTable[Index].VendorTable;
      for (Index1 = 0; Index1 < CapsuleTable->CapsuleArrayNumber; Index1++) {
        RuntimeDriverConvertPointer (EFI_OPTIONAL_POINTER, (VOID **) &CapsuleTable->CapsulePtr[Index1]);
      }     
      RuntimeDriverConvertPointer (EFI_OPTIONAL_POINTER, (VOID **) &(gST->ConfigurationTable[Index].VendorTable));
    }
#endif
  }
  //
  // Convert the runtime fields of the EFI System Table and recompute the CRC-32
  //
  RuntimeDriverConvertInternalPointer ((VOID **) &gST->FirmwareVendor);
  RuntimeDriverConvertInternalPointer ((VOID **) &gST->ConfigurationTable);
  RuntimeDriverConvertInternalPointer ((VOID **) &gST->RuntimeServices);
  RuntimeDriverCalculateEfiHdrCrc (&gST->Hdr);

  //
  // At this point, gRT and gST are physical pointers, but the contents of these tables
  // have been converted to runtime.
  //
  //
  // mVirtualMap is only valid during SetVirtualAddressMap() call
  //
  mVirtualMap = NULL;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RuntimeDriverInitialize (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
/*++

Routine Description:
  Install Runtime AP. This code includes the EfiDriverLib, but it functions at
  RT in physical mode. The only Lib services are gBS, gRT, and the DEBUG and
  ASSERT macros (they do ReportStatusCode).

Arguments:
  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns:

  EFI_SUCEESS - Runtime Driver Architectural Protocol Installed

  Other       - Return value from gBS->InstallMultipleProtocolInterfaces

--*/
{
  EFI_STATUS  Status;

  //
  // This image needs to be exclued from relocation for virtual mode, so cache
  // a copy of the Loaded Image protocol to test later.
  //
  Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **) &mMyLoadedImage
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Initialize the table used to compute 32-bit CRCs
  //
  RuntimeDriverInitializeCrc32Table ();

  //
  // Fill in the entries of the EFI Boot Services and EFI Runtime Services Tables
  //
  gBS->CalculateCrc32         = RuntimeDriverCalculateCrc32;
  gRT->SetVirtualAddressMap = RuntimeDriverSetVirtualAddressMap;
  gRT->ConvertPointer       = RuntimeDriverConvertPointer;

  //
  // Install the Runtime Architectural Protocol onto a new handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mRuntimeHandle,
                  &gEfiRuntimeArchProtocolGuid,
                  &mRuntime,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  mRuntimeImageNumber = 0;
  mRuntimeEventNumber = 0;

  return Status;
}
