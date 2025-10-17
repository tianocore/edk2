/** @file
  UEFI Memory Protection support.

  If the UEFI image is page aligned, the image code section is set to read only
  and the image data section is set to non-executable.

  1) This policy is applied for all UEFI image including boot service driver,
     runtime driver or application.
  2) This policy is applied only if the UEFI image meets the page alignment
     requirement.
  3) This policy is applied only if the Source UEFI image matches the
     PcdImageProtectionPolicy definition.
  4) This policy is not applied to the non-PE image region.

  The DxeCore calls CpuArchProtocol->SetMemoryAttributes() to protect
  the image. If the CpuArch protocol is not installed yet, the DxeCore
  enqueues the protection request. Once the CpuArch is installed, the
  DxeCore dequeues the protection request and applies policy.

  Once the image is unloaded, the protection is removed automatically.

Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/ImagePropertiesRecordLib.h>

#include <Guid/EventGroup.h>
#include <Guid/MemoryAttributesTable.h>

#include <Protocol/FirmwareVolume2.h>
#include <Protocol/MemoryAttribute.h>
#include <Protocol/SimpleFileSystem.h>

#include "DxeMain.h"
#include "Mem/HeapGuard.h"

//
// Image type definitions
//
#define IMAGE_UNKNOWN  0x00000001
#define IMAGE_FROM_FV  0x00000002

//
// Protection policy bit definition
//
#define DO_NOT_PROTECT                 0x00000000
#define PROTECT_IF_ALIGNED_ELSE_ALLOW  0x00000001

#define MEMORY_TYPE_OS_RESERVED_MIN   0x80000000
#define MEMORY_TYPE_OEM_RESERVED_MIN  0x70000000

#define PREVIOUS_MEMORY_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) - (Size)))

UINT32  mImageProtectionPolicy;

extern LIST_ENTRY  mGcdMemorySpaceMap;

STATIC LIST_ENTRY  mProtectedImageRecordList;

EFI_MEMORY_ATTRIBUTE_PROTOCOL  *gMemoryAttributeProtocol;

/**
  Get the image type.

  @param[in]    File       This is a pointer to the device path of the file that is
                           being dispatched.

  @return UINT32           Image Type
**/
UINT32
GetImageType (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL  *File
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;

  if (File == NULL) {
    return IMAGE_UNKNOWN;
  }

  //
  // First check to see if File is from a Firmware Volume
  //
  DeviceHandle   = NULL;
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *)File;
  Status         = gBS->LocateDevicePath (
                          &gEfiFirmwareVolume2ProtocolGuid,
                          &TempDevicePath,
                          &DeviceHandle
                          );
  if (!EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    DeviceHandle,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    NULL,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      return IMAGE_FROM_FV;
    }
  }

  return IMAGE_UNKNOWN;
}

/**
  Get UEFI image protection policy based upon image type.

  @param[in]  ImageType    The UEFI image type

  @return UEFI image protection policy
**/
UINT32
GetProtectionPolicyFromImageType (
  IN UINT32  ImageType
  )
{
  if ((ImageType & mImageProtectionPolicy) == 0) {
    return DO_NOT_PROTECT;
  } else {
    return PROTECT_IF_ALIGNED_ELSE_ALLOW;
  }
}

/**
  Get UEFI image protection policy based upon loaded image device path.

  @param[in]  LoadedImage              The loaded image protocol
  @param[in]  LoadedImageDevicePath    The loaded image device path protocol

  @return UEFI image protection policy
**/
UINT32
GetUefiImageProtectionPolicy (
  IN EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage,
  IN EFI_DEVICE_PATH_PROTOCOL   *LoadedImageDevicePath
  )
{
  BOOLEAN  InSmm;
  UINT32   ImageType;
  UINT32   ProtectionPolicy;

  //
  // Check SMM
  //
  InSmm = FALSE;
  if (gSmmBase2 != NULL) {
    gSmmBase2->InSmm (gSmmBase2, &InSmm);
  }

  if (InSmm) {
    return FALSE;
  }

  //
  // Check DevicePath
  //
  if (LoadedImage == gDxeCoreLoadedImage) {
    ImageType = IMAGE_FROM_FV;
  } else {
    ImageType = GetImageType (LoadedImageDevicePath);
  }

  ProtectionPolicy = GetProtectionPolicyFromImageType (ImageType);
  return ProtectionPolicy;
}

/**
  Set UEFI image memory attributes.

  @param[in]  BaseAddress            Specified start address
  @param[in]  Length                 Specified length
  @param[in]  Attributes             Specified attributes
**/
VOID
SetUefiImageMemoryAttributes (
  IN UINT64  BaseAddress,
  IN UINT64  Length,
  IN UINT64  Attributes
  )
{
  EFI_STATUS                       Status;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Descriptor;
  UINT64                           FinalAttributes;
  UINT64                           CurrentAddress;
  UINT64                           CurrentLength;
  UINT64                           ImageEnd;
  UINT64                           DescEnd;

  CurrentAddress = BaseAddress;
  ImageEnd       = BaseAddress + Length;

  // we loop here because we may have multiple memory space descriptors that overlap the requested range
  // this will definitely be the case for unprotecting an image, because that calls this function for the entire image,
  // which we split into different GCD descriptors when we protected it.
  while (CurrentAddress < BaseAddress + Length) {
    Status = CoreGetMemorySpaceDescriptor (CurrentAddress, &Descriptor);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a - Failed to get memory space descriptor for address %llx with status %r. Cannot protect image.\n",
        __func__,
        CurrentAddress,
        Status
        ));
      ASSERT_EFI_ERROR (Status);
      return;
    }

    DescEnd = Descriptor.BaseAddress + Descriptor.Length;

    // ensure that we only change the attributes for the range that we are interested in, not the entire descriptor, we
    // may also be in the middle of a descriptor, so ensure our length is not larger than the descriptor length
    if (ImageEnd > DescEnd) {
      CurrentLength = DescEnd - CurrentAddress;
    } else {
      CurrentLength = ImageEnd - CurrentAddress;
    }

    // Preserve the existing caching and virtual attributes, but remove the hardware access bits
    FinalAttributes = (Descriptor.Attributes & ~EFI_MEMORY_ACCESS_MASK) | (Attributes & EFI_MEMORY_ATTRIBUTE_MASK);

    DEBUG ((DEBUG_VERBOSE, "SetUefiImageMemoryAttributes - 0x%016lx - 0x%016lx (0x%016lx)\n", CurrentAddress, CurrentLength, FinalAttributes));

    // check to see if the capabilities support the attributes we want to set. If not, set the capabilities appropriately
    if ((Descriptor.Capabilities & FinalAttributes) != FinalAttributes) {
      Status = CoreSetMemorySpaceCapabilities (
                 CurrentAddress,
                 CurrentLength,
                 Descriptor.Capabilities | FinalAttributes
                 );

      // if we failed to set the capabilities, we should try to continue, it is possible we could succeed
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a failed setting capabilities on %llx of length %llx with capabilities %llx - %r\n",
          __func__,
          CurrentAddress,
          CurrentLength,
          Descriptor.Capabilities | FinalAttributes,
          Status
          ));
        ASSERT_EFI_ERROR (Status);
      }
    }

    // Call into the GCD to update the attributes there. It will call into the CPU Arch protocol to update the
    // page table attributes
    Status = CoreSetMemorySpaceAttributes (
               CurrentAddress,
               CurrentLength,
               FinalAttributes
               );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a failed on %llx of length %llx with attributes %llx - %r\n",
        __func__,
        CurrentAddress,
        CurrentLength,
        FinalAttributes,
        Status
        ));
      ASSERT_EFI_ERROR (Status);
    }

    if (((FinalAttributes & (EFI_MEMORY_ACCESS_MASK | EFI_CACHE_ATTRIBUTE_MASK)) == 0) && (gCpu != NULL)) {
      // if the passed hardware attributes are 0, CoreSetMemorySpaceAttributes() will not call into the CPU Arch protocol
      // to set the attributes, so we need to do it manually here. This can be the case when we are unprotecting an
      // image if no caching attributes are set. If gCpu has not been populated yet, we'll still have updated the GCD
      // descriptor and we should sync the attributes with the CPU Arch protocol when it is available.
      Status = gCpu->SetMemoryAttributes (gCpu, CurrentAddress, CurrentLength, 0);
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a failed to update page table for %llx of length %llx with attributes 0 - %r\n",
          __func__,
          CurrentAddress,
          CurrentLength,
          Status
          ));
        ASSERT_EFI_ERROR (Status);
      }
    }

    // we may have started in the middle of a descriptor, so we need to move to the beginning of the next descriptor,
    // or the end of the image, whichever is smaller
    CurrentAddress += CurrentLength;
  }
}

/**
  Set UEFI image protection attributes.

  @param[in]  ImageRecord    A UEFI image record
**/
VOID
SetUefiImageProtectionAttributes (
  IN IMAGE_PROPERTIES_RECORD  *ImageRecord
  )
{
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *ImageRecordCodeSection;
  LIST_ENTRY                            *ImageRecordCodeSectionLink;
  LIST_ENTRY                            *ImageRecordCodeSectionEndLink;
  LIST_ENTRY                            *ImageRecordCodeSectionList;
  UINT64                                CurrentBase;
  UINT64                                ImageEnd;

  ImageRecordCodeSectionList = &ImageRecord->CodeSegmentList;

  CurrentBase = ImageRecord->ImageBase;
  ImageEnd    = ImageRecord->ImageBase + ImageRecord->ImageSize;

  ImageRecordCodeSectionLink    = ImageRecordCodeSectionList->ForwardLink;
  ImageRecordCodeSectionEndLink = ImageRecordCodeSectionList;
  while (ImageRecordCodeSectionLink != ImageRecordCodeSectionEndLink) {
    ImageRecordCodeSection = CR (
                               ImageRecordCodeSectionLink,
                               IMAGE_PROPERTIES_RECORD_CODE_SECTION,
                               Link,
                               IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE
                               );
    ImageRecordCodeSectionLink = ImageRecordCodeSectionLink->ForwardLink;

    ASSERT (CurrentBase <= ImageRecordCodeSection->CodeSegmentBase);
    if (CurrentBase < ImageRecordCodeSection->CodeSegmentBase) {
      //
      // DATA
      //
      SetUefiImageMemoryAttributes (
        CurrentBase,
        ImageRecordCodeSection->CodeSegmentBase - CurrentBase,
        EFI_MEMORY_XP
        );
    }

    //
    // CODE
    //
    SetUefiImageMemoryAttributes (
      ImageRecordCodeSection->CodeSegmentBase,
      ImageRecordCodeSection->CodeSegmentSize,
      EFI_MEMORY_RO
      );
    CurrentBase = ImageRecordCodeSection->CodeSegmentBase + ImageRecordCodeSection->CodeSegmentSize;
  }

  //
  // Last DATA
  //
  ASSERT (CurrentBase <= ImageEnd);
  if (CurrentBase < ImageEnd) {
    //
    // DATA
    //
    SetUefiImageMemoryAttributes (
      CurrentBase,
      ImageEnd - CurrentBase,
      EFI_MEMORY_XP
      );
  }

  return;
}

/**
  Return the section alignment requirement for the PE image section type.

  @param[in]  MemoryType  PE/COFF image memory type

  @retval     The required section alignment for this memory type

**/
STATIC
UINT32
GetMemoryProtectionSectionAlignment (
  IN EFI_MEMORY_TYPE  MemoryType
  )
{
  UINT32  SectionAlignment;

  switch (MemoryType) {
    case EfiRuntimeServicesCode:
    case EfiACPIMemoryNVS:
    case EfiReservedMemoryType:
      SectionAlignment = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
      break;
    case EfiRuntimeServicesData:
      ASSERT (FALSE);
      SectionAlignment = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
      break;
    case EfiBootServicesCode:
    case EfiLoaderCode:
      SectionAlignment = EFI_PAGE_SIZE;
      break;
    case EfiACPIReclaimMemory:
    default:
      ASSERT (FALSE);
      SectionAlignment = EFI_PAGE_SIZE;
      break;
  }

  return SectionAlignment;
}

/**
  Protect UEFI PE/COFF image.

  @param[in]  LoadedImage              The loaded image protocol
  @param[in]  LoadedImageDevicePath    The loaded image device path protocol
**/
VOID
ProtectUefiImage (
  IN EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage,
  IN EFI_DEVICE_PATH_PROTOCOL   *LoadedImageDevicePath
  )
{
  IMAGE_PROPERTIES_RECORD  *ImageRecord;
  UINT32                   ProtectionPolicy;
  EFI_STATUS               Status;
  UINT32                   RequiredAlignment;

  DEBUG ((DEBUG_INFO, "ProtectUefiImageCommon - 0x%x\n", LoadedImage));
  DEBUG ((DEBUG_INFO, "  - 0x%016lx - 0x%016lx\n", (EFI_PHYSICAL_ADDRESS)(UINTN)LoadedImage->ImageBase, LoadedImage->ImageSize));

  if (gCpu == NULL) {
    return;
  }

  ProtectionPolicy = GetUefiImageProtectionPolicy (LoadedImage, LoadedImageDevicePath);
  switch (ProtectionPolicy) {
    case DO_NOT_PROTECT:
      return;
    case PROTECT_IF_ALIGNED_ELSE_ALLOW:
      break;
    default:
      ASSERT (FALSE);
      return;
  }

  ImageRecord = AllocateZeroPool (sizeof (*ImageRecord));
  if (ImageRecord == NULL) {
    return;
  }

  RequiredAlignment = GetMemoryProtectionSectionAlignment (LoadedImage->ImageCodeType);

  Status = CreateImagePropertiesRecord (
             LoadedImage->ImageBase,
             LoadedImage->ImageSize,
             &RequiredAlignment,
             ImageRecord
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a failed to create image properties record\n", __func__));

    // if we failed to create the image properties record, this may mean that the image is not aligned properly
    // the GCD will believe that this memory is non-executable, because the NX initialization routine doesn't know what
    // memory is image memory or not, even though the page table has the correct attributes, so we need to set the
    // attributes here to RWX so that future updates to the GCD do not apply the NX attributes to this memory in the
    // page table (as can happen when applying virtual attributes). This may have the side effect of marking other
    // memory as RWX, since this image may not be page aligned, but that is safe to do, it may just remove some
    // page protections, but it already has to to execute this image.
    SetUefiImageMemoryAttributes (
      (UINT64)(UINTN)LoadedImage->ImageBase & ~EFI_PAGE_MASK,
      (LoadedImage->ImageSize + EFI_PAGE_MASK) & ~EFI_PAGE_MASK,
      0
      );
    FreePool (ImageRecord);
    goto Finish;
  }

  //
  // CPU ARCH present. Update memory attribute directly.
  //
  SetUefiImageProtectionAttributes (ImageRecord);

  //
  // Record the image record in the list so we can undo the protections later
  //
  InsertTailList (&mProtectedImageRecordList, &ImageRecord->Link);

Finish:
  return;
}

/**
  Unprotect UEFI image.

  @param[in]  LoadedImage              The loaded image protocol
  @param[in]  LoadedImageDevicePath    The loaded image device path protocol
**/
VOID
UnprotectUefiImage (
  IN EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage,
  IN EFI_DEVICE_PATH_PROTOCOL   *LoadedImageDevicePath
  )
{
  IMAGE_PROPERTIES_RECORD  *ImageRecord;
  LIST_ENTRY               *ImageRecordLink;

  if (PcdGet32 (PcdImageProtectionPolicy) != 0) {
    for (ImageRecordLink = mProtectedImageRecordList.ForwardLink;
         ImageRecordLink != &mProtectedImageRecordList;
         ImageRecordLink = ImageRecordLink->ForwardLink)
    {
      ImageRecord = CR (
                      ImageRecordLink,
                      IMAGE_PROPERTIES_RECORD,
                      Link,
                      IMAGE_PROPERTIES_RECORD_SIGNATURE
                      );

      if (ImageRecord->ImageBase == (EFI_PHYSICAL_ADDRESS)(UINTN)LoadedImage->ImageBase) {
        SetUefiImageMemoryAttributes (
          ImageRecord->ImageBase,
          ImageRecord->ImageSize,
          0
          );
        DeleteImagePropertiesRecord (ImageRecord);
        return;
      }
    }
  }
}

/**
  Return the EFI memory permission attribute associated with memory
  type 'MemoryType' under the configured DXE memory protection policy.

  @param MemoryType       Memory type.
**/
STATIC
UINT64
GetPermissionAttributeForMemoryType (
  IN EFI_MEMORY_TYPE  MemoryType
  )
{
  UINT64  TestBit;

  if ((UINT32)MemoryType >= MEMORY_TYPE_OS_RESERVED_MIN) {
    TestBit = BIT63;
  } else if ((UINT32)MemoryType >= MEMORY_TYPE_OEM_RESERVED_MIN) {
    TestBit = BIT62;
  } else {
    TestBit = LShiftU64 (1, MemoryType);
  }

  if ((PcdGet64 (PcdDxeNxMemoryProtectionPolicy) & TestBit) != 0) {
    return EFI_MEMORY_XP;
  } else {
    return 0;
  }
}

/**
  Sort memory map entries based upon PhysicalStart, from low to high.

  @param  MemoryMap              A pointer to the buffer in which firmware places
                                 the current memory map.
  @param  MemoryMapSize          Size, in bytes, of the MemoryMap buffer.
  @param  DescriptorSize         Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
**/
STATIC
VOID
SortMemoryMap (
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                      MemoryMapSize,
  IN UINTN                      DescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *NextMemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;
  EFI_MEMORY_DESCRIPTOR  TempMemoryMap;

  MemoryMapEntry     = MemoryMap;
  NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  MemoryMapEnd       = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + MemoryMapSize);
  while (MemoryMapEntry < MemoryMapEnd) {
    while (NextMemoryMapEntry < MemoryMapEnd) {
      if (MemoryMapEntry->PhysicalStart > NextMemoryMapEntry->PhysicalStart) {
        CopyMem (&TempMemoryMap, MemoryMapEntry, sizeof (EFI_MEMORY_DESCRIPTOR));
        CopyMem (MemoryMapEntry, NextMemoryMapEntry, sizeof (EFI_MEMORY_DESCRIPTOR));
        CopyMem (NextMemoryMapEntry, &TempMemoryMap, sizeof (EFI_MEMORY_DESCRIPTOR));
      }

      NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (NextMemoryMapEntry, DescriptorSize);
    }

    MemoryMapEntry     = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
    NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  }
}

/**
  Merge adjacent memory map entries if they use the same memory protection policy

  @param[in, out]  MemoryMap              A pointer to the buffer in which firmware places
                                          the current memory map.
  @param[in, out]  MemoryMapSize          A pointer to the size, in bytes, of the
                                          MemoryMap buffer. On input, this is the size of
                                          the current memory map.  On output,
                                          it is the size of new memory map after merge.
  @param[in]       DescriptorSize         Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
**/
STATIC
VOID
MergeMemoryMapForProtectionPolicy (
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN OUT UINTN                  *MemoryMapSize,
  IN UINTN                      DescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;
  UINT64                 MemoryBlockLength;
  EFI_MEMORY_DESCRIPTOR  *NewMemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *NextMemoryMapEntry;
  UINT64                 Attributes;

  SortMemoryMap (MemoryMap, *MemoryMapSize, DescriptorSize);

  MemoryMapEntry    = MemoryMap;
  NewMemoryMapEntry = MemoryMap;
  MemoryMapEnd      = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + *MemoryMapSize);
  while ((UINTN)MemoryMapEntry < (UINTN)MemoryMapEnd) {
    CopyMem (NewMemoryMapEntry, MemoryMapEntry, sizeof (EFI_MEMORY_DESCRIPTOR));
    NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);

    do {
      MemoryBlockLength = (UINT64)(EFI_PAGES_TO_SIZE ((UINTN)MemoryMapEntry->NumberOfPages));
      Attributes        = GetPermissionAttributeForMemoryType (MemoryMapEntry->Type);

      if (((UINTN)NextMemoryMapEntry < (UINTN)MemoryMapEnd) &&
          (Attributes == GetPermissionAttributeForMemoryType (NextMemoryMapEntry->Type)) &&
          ((MemoryMapEntry->PhysicalStart + MemoryBlockLength) == NextMemoryMapEntry->PhysicalStart))
      {
        MemoryMapEntry->NumberOfPages += NextMemoryMapEntry->NumberOfPages;
        if (NewMemoryMapEntry != MemoryMapEntry) {
          NewMemoryMapEntry->NumberOfPages += NextMemoryMapEntry->NumberOfPages;
        }

        NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (NextMemoryMapEntry, DescriptorSize);
        continue;
      } else {
        MemoryMapEntry = PREVIOUS_MEMORY_DESCRIPTOR (NextMemoryMapEntry, DescriptorSize);
        break;
      }
    } while (TRUE);

    MemoryMapEntry    = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
    NewMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (NewMemoryMapEntry, DescriptorSize);
  }

  *MemoryMapSize = (UINTN)NewMemoryMapEntry - (UINTN)MemoryMap;

  return;
}

/**
  Remove exec permissions from all regions whose type is identified by
  PcdDxeNxMemoryProtectionPolicy.
**/
STATIC
VOID
InitializeDxeNxMemoryProtectionPolicy (
  VOID
  )
{
  UINTN                      MemoryMapSize;
  UINTN                      MapKey;
  UINTN                      DescriptorSize;
  UINT32                     DescriptorVersion;
  EFI_MEMORY_DESCRIPTOR      *MemoryMap;
  EFI_MEMORY_DESCRIPTOR      *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR      *MemoryMapEnd;
  EFI_STATUS                 Status;
  UINT64                     Attributes;
  LIST_ENTRY                 *Link;
  EFI_GCD_MAP_ENTRY          *Entry;
  EFI_PEI_HOB_POINTERS       Hob;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryHob;
  EFI_PHYSICAL_ADDRESS       StackBase;

  //
  // Get the EFI memory map.
  //
  MemoryMapSize = 0;
  MemoryMap     = NULL;

  Status = gBS->GetMemoryMap (
                  &MemoryMapSize,
                  MemoryMap,
                  &MapKey,
                  &DescriptorSize,
                  &DescriptorVersion
                  );
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);
  do {
    MemoryMap = (EFI_MEMORY_DESCRIPTOR *)AllocatePool (MemoryMapSize);
    ASSERT (MemoryMap != NULL);
    Status = gBS->GetMemoryMap (
                    &MemoryMapSize,
                    MemoryMap,
                    &MapKey,
                    &DescriptorSize,
                    &DescriptorVersion
                    );
    if (EFI_ERROR (Status)) {
      FreePool (MemoryMap);
    }
  } while (Status == EFI_BUFFER_TOO_SMALL);

  ASSERT_EFI_ERROR (Status);

  StackBase = 0;
  if (PcdGetBool (PcdCpuStackGuard)) {
    //
    // Get the base of stack from Hob.
    //
    Hob.Raw = GetHobList ();
    while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw)) != NULL) {
      MemoryHob = Hob.MemoryAllocation;
      if (CompareGuid (&gEfiHobMemoryAllocStackGuid, &MemoryHob->AllocDescriptor.Name)) {
        DEBUG ((
          DEBUG_INFO,
          "%a: StackBase = 0x%016lx  StackSize = 0x%016lx\n",
          __func__,
          MemoryHob->AllocDescriptor.MemoryBaseAddress,
          MemoryHob->AllocDescriptor.MemoryLength
          ));

        StackBase = MemoryHob->AllocDescriptor.MemoryBaseAddress;
        //
        // Ensure the base of the stack is page-size aligned.
        //
        ASSERT ((StackBase & EFI_PAGE_MASK) == 0);
        break;
      }

      Hob.Raw = GET_NEXT_HOB (Hob);
    }

    //
    // Ensure the base of stack can be found from Hob when stack guard is
    // enabled.
    //
    ASSERT (StackBase != 0);
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: applying strict permissions to active memory regions\n",
    __func__
    ));

  MergeMemoryMapForProtectionPolicy (MemoryMap, &MemoryMapSize, DescriptorSize);

  MemoryMapEntry = MemoryMap;
  MemoryMapEnd   = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + MemoryMapSize);
  while ((UINTN)MemoryMapEntry < (UINTN)MemoryMapEnd) {
    Attributes = GetPermissionAttributeForMemoryType (MemoryMapEntry->Type);
    if (Attributes != 0) {
      SetUefiImageMemoryAttributes (
        MemoryMapEntry->PhysicalStart,
        LShiftU64 (MemoryMapEntry->NumberOfPages, EFI_PAGE_SHIFT),
        Attributes
        );

      //
      // Add EFI_MEMORY_RP attribute for page 0 if NULL pointer detection is
      // enabled.
      //
      if ((MemoryMapEntry->PhysicalStart == 0) &&
          (PcdGet8 (PcdNullPointerDetectionPropertyMask) != 0))
      {
        ASSERT (MemoryMapEntry->NumberOfPages > 0);
        SetUefiImageMemoryAttributes (
          0,
          EFI_PAGES_TO_SIZE (1),
          EFI_MEMORY_RP | Attributes
          );
      }

      //
      // Add EFI_MEMORY_RP attribute for the first page of the stack if stack
      // guard is enabled.
      //
      if ((StackBase != 0) &&
          ((StackBase >= MemoryMapEntry->PhysicalStart) &&
           (StackBase <  MemoryMapEntry->PhysicalStart +
            LShiftU64 (MemoryMapEntry->NumberOfPages, EFI_PAGE_SHIFT))) &&
          PcdGetBool (PcdCpuStackGuard))
      {
        SetUefiImageMemoryAttributes (
          StackBase,
          EFI_PAGES_TO_SIZE (1),
          EFI_MEMORY_RP | Attributes
          );
      }
    }

    MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  }

  FreePool (MemoryMap);

  //
  // Apply the policy for RAM regions that we know are present and
  // accessible, but have not been added to the UEFI memory map (yet).
  //
  if (GetPermissionAttributeForMemoryType (EfiConventionalMemory) != 0) {
    DEBUG ((
      DEBUG_INFO,
      "%a: applying strict permissions to inactive memory regions\n",
      __func__
      ));

    CoreAcquireGcdMemoryLock ();

    Link = mGcdMemorySpaceMap.ForwardLink;
    while (Link != &mGcdMemorySpaceMap) {
      Entry = CR (Link, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);

      if ((Entry->GcdMemoryType == EfiGcdMemoryTypeReserved) &&
          (Entry->EndAddress < MAX_ADDRESS) &&
          ((Entry->Capabilities & (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED)) ==
           (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED)))
      {
        Attributes = GetPermissionAttributeForMemoryType (EfiConventionalMemory) |
                     (Entry->Attributes & EFI_CACHE_ATTRIBUTE_MASK);

        DEBUG ((
          DEBUG_INFO,
          "Untested GCD memory space region: - 0x%016lx - 0x%016lx (0x%016lx)\n",
          Entry->BaseAddress,
          Entry->EndAddress - Entry->BaseAddress + 1,
          Attributes
          ));

        ASSERT (gCpu != NULL);
        gCpu->SetMemoryAttributes (
                gCpu,
                Entry->BaseAddress,
                Entry->EndAddress - Entry->BaseAddress + 1,
                Attributes
                );
      }

      Link = Link->ForwardLink;
    }

    CoreReleaseGcdMemoryLock ();
  }
}

/**
  A notification for CPU_ARCH protocol.

  @param[in]  Event                 Event whose notification function is being invoked.
  @param[in]  Context               Pointer to the notification function's context,
                                    which is implementation-dependent.

**/
VOID
EFIAPI
MemoryProtectionCpuArchProtocolNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                 Status;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL   *LoadedImageDevicePath;
  UINTN                      NoHandles;
  EFI_HANDLE                 *HandleBuffer;
  UINTN                      Index;

  DEBUG ((DEBUG_INFO, "MemoryProtectionCpuArchProtocolNotify:\n"));
  Status = CoreLocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&gCpu);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Apply the memory protection policy on non-BScode/RTcode regions.
  //
  if (PcdGet64 (PcdDxeNxMemoryProtectionPolicy) != 0) {
    InitializeDxeNxMemoryProtectionPolicy ();
  }

  //
  // Call notify function meant for Heap Guard.
  //
  HeapGuardCpuArchProtocolNotify ();

  if (mImageProtectionPolicy == 0) {
    goto Done;
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiLoadedImageProtocolGuid,
                  NULL,
                  &NoHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status) && (NoHandles == 0)) {
    goto Done;
  }

  for (Index = 0; Index < NoHandles; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **)&LoadedImage
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiLoadedImageDevicePathProtocolGuid,
                    (VOID **)&LoadedImageDevicePath
                    );
    if (EFI_ERROR (Status)) {
      LoadedImageDevicePath = NULL;
    }

    ProtectUefiImage (LoadedImage, LoadedImageDevicePath);
  }

  FreePool (HandleBuffer);

Done:
  CoreCloseEvent (Event);
}

/**
  ExitBootServices Callback function for memory protection.
**/
VOID
MemoryProtectionExitBootServicesCallback (
  VOID
  )
{
  EFI_RUNTIME_IMAGE_ENTRY  *RuntimeImage;
  LIST_ENTRY               *Link;

  //
  // We need remove the RT protection, because RT relocation need write code segment
  // at SetVirtualAddressMap(). We cannot assume OS/Loader has taken over page table at that time.
  //
  // Firmware does not own page tables after ExitBootServices(), so the OS would
  // have to relax protection of RT code pages across SetVirtualAddressMap(), or
  // delay setting protections on RT code pages until after SetVirtualAddressMap().
  // OS may set protection on RT based upon EFI_MEMORY_ATTRIBUTES_TABLE later.
  //
  if (mImageProtectionPolicy != 0) {
    for (Link = gRuntime->ImageHead.ForwardLink; Link != &gRuntime->ImageHead; Link = Link->ForwardLink) {
      RuntimeImage = BASE_CR (Link, EFI_RUNTIME_IMAGE_ENTRY, Link);
      SetUefiImageMemoryAttributes ((UINT64)(UINTN)RuntimeImage->ImageBase, ALIGN_VALUE (RuntimeImage->ImageSize, EFI_PAGE_SIZE), 0);
    }
  }
}

/**
  Disable NULL pointer detection after EndOfDxe. This is a workaround resort in
  order to skip unfixable NULL pointer access issues detected in OptionROM or
  boot loaders.

  @param[in]  Event     The Event this notify function registered to.
  @param[in]  Context   Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
DisableNullDetectionAtTheEndOfDxe (
  EFI_EVENT  Event,
  VOID       *Context
  )
{
  EFI_STATUS                       Status;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Desc;

  DEBUG ((DEBUG_INFO, "DisableNullDetectionAtTheEndOfDxe(): start\r\n"));
  //
  // Disable NULL pointer detection by enabling first 4K page
  //
  Status = CoreGetMemorySpaceDescriptor (0, &Desc);
  ASSERT_EFI_ERROR (Status);

  if ((Desc.Capabilities & EFI_MEMORY_RP) == 0) {
    Status = CoreSetMemorySpaceCapabilities (
               0,
               EFI_PAGE_SIZE,
               Desc.Capabilities | EFI_MEMORY_RP
               );
    ASSERT_EFI_ERROR (Status);
  }

  Status = CoreSetMemorySpaceAttributes (
             0,
             EFI_PAGE_SIZE,
             Desc.Attributes & ~EFI_MEMORY_RP
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Page 0 might have be allocated to avoid misuses. Free it here anyway.
  //
  CoreFreePages (0, 1);

  CoreCloseEvent (Event);
  DEBUG ((DEBUG_INFO, "DisableNullDetectionAtTheEndOfDxe(): end\r\n"));

  return;
}

/**
  A notification for the Memory Attribute Protocol Installation.

  @param[in]  Event                 Event whose notification function is being invoked.
  @param[in]  Context               Pointer to the notification function's context,
                                    which is implementation-dependent.

**/
VOID
EFIAPI
MemoryAttributeProtocolNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gEfiMemoryAttributeProtocolGuid, NULL, (VOID **)&gMemoryAttributeProtocol);

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "%a - Unable to locate the memory attribute protocol! Status = %r\n",
      __func__,
      Status
      ));
  }

  CoreCloseEvent (Event);
}

/**
  Initialize Memory Protection support.
**/
VOID
EFIAPI
CoreInitializeMemoryProtection (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;
  EFI_EVENT   EndOfDxeEvent;
  VOID        *Registration;

  mImageProtectionPolicy = PcdGet32 (PcdImageProtectionPolicy);

  InitializeListHead (&mProtectedImageRecordList);

  //
  // Sanity check the PcdDxeNxMemoryProtectionPolicy setting:
  // - code regions should have no EFI_MEMORY_XP attribute
  // - EfiConventionalMemory and EfiBootServicesData should use the
  //   same attribute
  //
  ASSERT ((GetPermissionAttributeForMemoryType (EfiBootServicesCode) & EFI_MEMORY_XP) == 0);
  ASSERT ((GetPermissionAttributeForMemoryType (EfiRuntimeServicesCode) & EFI_MEMORY_XP) == 0);
  ASSERT ((GetPermissionAttributeForMemoryType (EfiLoaderCode) & EFI_MEMORY_XP) == 0);
  ASSERT (
    GetPermissionAttributeForMemoryType (EfiBootServicesData) ==
    GetPermissionAttributeForMemoryType (EfiConventionalMemory)
    );

  Status = CoreCreateEvent (
             EVT_NOTIFY_SIGNAL,
             TPL_CALLBACK,
             MemoryProtectionCpuArchProtocolNotify,
             NULL,
             &Event
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for protocol notifactions on this event
  //
  Status = CoreRegisterProtocolNotify (
             &gEfiCpuArchProtocolGuid,
             Event,
             &Registration
             );
  ASSERT_EFI_ERROR (Status);

  // Register an event to populate the memory attribute protocol
  Status = CoreCreateEvent (
             EVT_NOTIFY_SIGNAL,
             TPL_CALLBACK,
             MemoryAttributeProtocolNotify,
             NULL,
             &Event
             );

  // if we fail to create the event or the protocol notify, we should still continue, we won't be able to query the
  // memory attributes on FreePages(), so we may encounter a driver or bootloader that has not set attributes back to
  // RW, but this matches the state of the world before this protocol was introduced, so it is not a regression.
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to create event for the Memory Attribute Protocol notification: %r\n", __func__, Status));
    ASSERT_EFI_ERROR (Status);
  }

  // Register for protocol notification
  Status = CoreRegisterProtocolNotify (
             &gEfiMemoryAttributeProtocolGuid,
             Event,
             &Registration
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to register for the Memory Attribute Protocol notification: %r\n", __func__, Status));
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Register a callback to disable NULL pointer detection at EndOfDxe
  //
  if ((PcdGet8 (PcdNullPointerDetectionPropertyMask) & (BIT0|BIT7))
      == (BIT0|BIT7))
  {
    Status = CoreCreateEventEx (
               EVT_NOTIFY_SIGNAL,
               TPL_NOTIFY,
               DisableNullDetectionAtTheEndOfDxe,
               NULL,
               &gEfiEndOfDxeEventGroupGuid,
               &EndOfDxeEvent
               );
    ASSERT_EFI_ERROR (Status);
  }

  return;
}

/**
  Returns whether we are currently executing in SMM mode.
**/
STATIC
BOOLEAN
IsInSmm (
  VOID
  )
{
  BOOLEAN  InSmm;

  InSmm = FALSE;
  if (gSmmBase2 != NULL) {
    gSmmBase2->InSmm (gSmmBase2, &InSmm);
  }

  return InSmm;
}

/**
  Manage memory permission attributes on a memory range, according to the
  configured DXE memory protection policy.

  @param  OldType           The old memory type of the range
  @param  NewType           The new memory type of the range
  @param  Memory            The base address of the range
  @param  Length            The size of the range (in bytes)

  @return EFI_SUCCESS       If we are executing in SMM mode. No permission attributes
                            are updated in this case
  @return EFI_SUCCESS       If the the CPU arch protocol is not installed yet
  @return EFI_SUCCESS       If no DXE memory protection policy has been configured
  @return EFI_SUCCESS       If OldType and NewType use the same permission attributes
  @return other             Return value of gCpu->SetMemoryAttributes()

**/
EFI_STATUS
EFIAPI
ApplyMemoryProtectionPolicy (
  IN  EFI_MEMORY_TYPE       OldType,
  IN  EFI_MEMORY_TYPE       NewType,
  IN  EFI_PHYSICAL_ADDRESS  Memory,
  IN  UINT64                Length
  )
{
  UINT64  OldAttributes;
  UINT64  NewAttributes;

  //
  // The policy configured in PcdDxeNxMemoryProtectionPolicy
  // does not apply to allocations performed in SMM mode.
  //
  if (IsInSmm ()) {
    return EFI_SUCCESS;
  }

  //
  // If the CPU arch protocol is not installed yet, we cannot manage memory
  // permission attributes, and it is the job of the driver that installs this
  // protocol to set the permissions on existing allocations.
  //
  if (gCpu == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Check if a DXE memory protection policy has been configured
  //
  if (PcdGet64 (PcdDxeNxMemoryProtectionPolicy) == 0) {
    return EFI_SUCCESS;
  }

  //
  // Don't overwrite Guard pages, which should be the first and/or last page,
  // if any.
  //
  if (IsHeapGuardEnabled (GUARD_HEAP_TYPE_PAGE|GUARD_HEAP_TYPE_POOL)) {
    if (IsGuardPage (Memory)) {
      Memory += EFI_PAGE_SIZE;
      Length -= EFI_PAGE_SIZE;
      if (Length == 0) {
        return EFI_SUCCESS;
      }
    }

    if (IsGuardPage (Memory + Length - EFI_PAGE_SIZE)) {
      Length -= EFI_PAGE_SIZE;
      if (Length == 0) {
        return EFI_SUCCESS;
      }
    }
  }

  //
  // Update the executable permissions according to the DXE memory
  // protection policy, but only if
  // - the policy is different between the old and the new type, or
  // - this is a newly added region (OldType == EfiMaxMemoryType)
  //
  NewAttributes = GetPermissionAttributeForMemoryType (NewType);

  if (OldType != EfiMaxMemoryType) {
    OldAttributes = GetPermissionAttributeForMemoryType (OldType);
    if (OldAttributes == NewAttributes) {
      // policy is the same between OldType and NewType
      return EFI_SUCCESS;
    }
  } else if (NewAttributes == 0) {
    // newly added region of a type that does not require protection
    return EFI_SUCCESS;
  }

  return gCpu->SetMemoryAttributes (gCpu, Memory, Length, NewAttributes);
}
