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

Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>

#include <Guid/EventGroup.h>
#include <Guid/MemoryAttributesTable.h>
#include <Guid/PropertiesTable.h>

#include <Protocol/FirmwareVolume2.h>
#include <Protocol/BlockIo.h>
#include <Protocol/SimpleFileSystem.h>

#include "DxeMain.h"

#define CACHE_ATTRIBUTE_MASK   (EFI_MEMORY_UC | EFI_MEMORY_WC | EFI_MEMORY_WT | EFI_MEMORY_WB | EFI_MEMORY_UCE | EFI_MEMORY_WP)
#define MEMORY_ATTRIBUTE_MASK  (EFI_MEMORY_RP | EFI_MEMORY_XP | EFI_MEMORY_RO)

//
// Image type definitions
//
#define IMAGE_UNKNOWN                         0x00000001
#define IMAGE_FROM_FV                         0x00000002

//
// Protection policy bit definition
//
#define DO_NOT_PROTECT                         0x00000000
#define PROTECT_IF_ALIGNED_ELSE_ALLOW          0x00000001

UINT32   mImageProtectionPolicy;

/**
  Sort code section in image record, based upon CodeSegmentBase from low to high.

  @param  ImageRecord    image record to be sorted
**/
VOID
SortImageRecordCodeSection (
  IN IMAGE_PROPERTIES_RECORD              *ImageRecord
  );

/**
  Check if code section in image record is valid.

  @param  ImageRecord    image record to be checked

  @retval TRUE  image record is valid
  @retval FALSE image record is invalid
**/
BOOLEAN
IsImageRecordCodeSectionValid (
  IN IMAGE_PROPERTIES_RECORD              *ImageRecord
  );

/**
  Get the image type.

  @param[in]    File       This is a pointer to the device path of the file that is
                           being dispatched.

  @return UINT32           Image Type
**/
UINT32
GetImageType (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL   *File
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL          *TempDevicePath;

  if (File == NULL) {
    return IMAGE_UNKNOWN;
  }

  //
  // First check to see if File is from a Firmware Volume
  //
  DeviceHandle      = NULL;
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) File;
  Status = gBS->LocateDevicePath (
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
  IN EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage,
  IN EFI_DEVICE_PATH_PROTOCOL    *LoadedImageDevicePath
  )
{
  BOOLEAN     InSmm;
  UINT32      ImageType;
  UINT32      ProtectionPolicy;

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
  IN UINT64   BaseAddress,
  IN UINT64   Length,
  IN UINT64   Attributes
  )
{
  EFI_STATUS                       Status;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Descriptor;
  UINT64                           FinalAttributes;

  Status = CoreGetMemorySpaceDescriptor(BaseAddress, &Descriptor);
  ASSERT_EFI_ERROR(Status);

  FinalAttributes = (Descriptor.Attributes & CACHE_ATTRIBUTE_MASK) | (Attributes & MEMORY_ATTRIBUTE_MASK);

  DEBUG ((DEBUG_INFO, "SetUefiImageMemoryAttributes - 0x%016lx - 0x%016lx (0x%016lx)\n", BaseAddress, Length, FinalAttributes));

  ASSERT(gCpu != NULL);
  gCpu->SetMemoryAttributes (gCpu, BaseAddress, Length, FinalAttributes);
}

/**
  Set UEFI image protection attributes.

  @param[in]  ImageRecord    A UEFI image record
  @param[in]  Protect        TRUE:  Protect the UEFI image.
                             FALSE: Unprotect the UEFI image.
**/
VOID
SetUefiImageProtectionAttributes (
  IN IMAGE_PROPERTIES_RECORD     *ImageRecord,
  IN BOOLEAN                     Protect
  )
{
  IMAGE_PROPERTIES_RECORD_CODE_SECTION      *ImageRecordCodeSection;
  LIST_ENTRY                                *ImageRecordCodeSectionLink;
  LIST_ENTRY                                *ImageRecordCodeSectionEndLink;
  LIST_ENTRY                                *ImageRecordCodeSectionList;
  UINT64                                    CurrentBase;
  UINT64                                    ImageEnd;
  UINT64                                    Attribute;

  ImageRecordCodeSectionList = &ImageRecord->CodeSegmentList;

  CurrentBase = ImageRecord->ImageBase;
  ImageEnd    = ImageRecord->ImageBase + ImageRecord->ImageSize;

  ImageRecordCodeSectionLink = ImageRecordCodeSectionList->ForwardLink;
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
      if (Protect) {
        Attribute = EFI_MEMORY_XP;
      } else {
        Attribute = 0;
      }
      SetUefiImageMemoryAttributes (
        CurrentBase,
        ImageRecordCodeSection->CodeSegmentBase - CurrentBase,
        Attribute
        );
    }
    //
    // CODE
    //
    if (Protect) {
      Attribute = EFI_MEMORY_RO;
    } else {
      Attribute = 0;
    }
    SetUefiImageMemoryAttributes (
      ImageRecordCodeSection->CodeSegmentBase,
      ImageRecordCodeSection->CodeSegmentSize,
      Attribute
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
    if (Protect) {
      Attribute = EFI_MEMORY_XP;
    } else {
      Attribute = 0;
    }
    SetUefiImageMemoryAttributes (
      CurrentBase,
      ImageEnd - CurrentBase,
      Attribute
      );
  }
  return ;
}

/**
  Return if the PE image section is aligned.

  @param[in]  SectionAlignment    PE/COFF section alignment
  @param[in]  MemoryType          PE/COFF image memory type

  @retval TRUE  The PE image section is aligned.
  @retval FALSE The PE image section is not aligned.
**/
BOOLEAN
IsMemoryProtectionSectionAligned (
  IN UINT32           SectionAlignment,
  IN EFI_MEMORY_TYPE  MemoryType
  )
{
  UINT32  PageAlignment;

  switch (MemoryType) {
  case EfiRuntimeServicesCode:
  case EfiACPIMemoryNVS:
    PageAlignment = EFI_ACPI_RUNTIME_PAGE_ALLOCATION_ALIGNMENT;
    break;
  case EfiRuntimeServicesData:
  case EfiACPIReclaimMemory:
    ASSERT (FALSE);
    PageAlignment = EFI_ACPI_RUNTIME_PAGE_ALLOCATION_ALIGNMENT;
    break;
  case EfiBootServicesCode:
  case EfiLoaderCode:
  case EfiReservedMemoryType:
    PageAlignment = EFI_PAGE_SIZE;
    break;
  default:
    ASSERT (FALSE);
    PageAlignment = EFI_PAGE_SIZE;
    break;
  }

  if ((SectionAlignment & (PageAlignment - 1)) != 0) {
    return FALSE;
  } else {
    return TRUE;
  }
}

/**
  Free Image record.

  @param[in]  ImageRecord    A UEFI image record
**/
VOID
FreeImageRecord (
  IN IMAGE_PROPERTIES_RECORD              *ImageRecord
  )
{
  LIST_ENTRY                           *CodeSegmentListHead;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION *ImageRecordCodeSection;

  CodeSegmentListHead = &ImageRecord->CodeSegmentList;
  while (!IsListEmpty (CodeSegmentListHead)) {
    ImageRecordCodeSection = CR (
                               CodeSegmentListHead->ForwardLink,
                               IMAGE_PROPERTIES_RECORD_CODE_SECTION,
                               Link,
                               IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE
                               );
    RemoveEntryList (&ImageRecordCodeSection->Link);
    FreePool (ImageRecordCodeSection);
  }

  if (ImageRecord->Link.ForwardLink != NULL) {
    RemoveEntryList (&ImageRecord->Link);
  }
  FreePool (ImageRecord);
}

/**
  Protect or unprotect UEFI image common function.

  @param[in]  LoadedImage              The loaded image protocol
  @param[in]  LoadedImageDevicePath    The loaded image device path protocol
  @param[in]  Protect                  TRUE:  Protect the UEFI image.
                                       FALSE: Unprotect the UEFI image.
**/
VOID
ProtectUefiImageCommon (
  IN EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage,
  IN EFI_DEVICE_PATH_PROTOCOL    *LoadedImageDevicePath,
  IN BOOLEAN                     Protect
  )
{
  VOID                                 *ImageAddress;
  EFI_IMAGE_DOS_HEADER                 *DosHdr;
  UINT32                               PeCoffHeaderOffset;
  UINT32                               SectionAlignment;
  EFI_IMAGE_SECTION_HEADER             *Section;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION  Hdr;
  UINT8                                *Name;
  UINTN                                Index;
  IMAGE_PROPERTIES_RECORD              *ImageRecord;
  CHAR8                                *PdbPointer;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION *ImageRecordCodeSection;
  UINT16                               Magic;
  BOOLEAN                              IsAligned;
  UINT32                               ProtectionPolicy;

  DEBUG ((DEBUG_INFO, "ProtectUefiImageCommon - 0x%x\n", LoadedImage));
  DEBUG ((DEBUG_INFO, "  - 0x%016lx - 0x%016lx\n", (EFI_PHYSICAL_ADDRESS)(UINTN)LoadedImage->ImageBase, LoadedImage->ImageSize));

  if (gCpu == NULL) {
    return ;
  }

  ProtectionPolicy = GetUefiImageProtectionPolicy (LoadedImage, LoadedImageDevicePath);
  switch (ProtectionPolicy) {
  case DO_NOT_PROTECT:
    return ;
  case PROTECT_IF_ALIGNED_ELSE_ALLOW:
    break;
  default:
    ASSERT(FALSE);
    return ;
  }

  ImageRecord = AllocateZeroPool (sizeof(*ImageRecord));
  if (ImageRecord == NULL) {
    return ;
  }
  ImageRecord->Signature = IMAGE_PROPERTIES_RECORD_SIGNATURE;

  //
  // Step 1: record whole region
  //
  ImageRecord->ImageBase = (EFI_PHYSICAL_ADDRESS)(UINTN)LoadedImage->ImageBase;
  ImageRecord->ImageSize = LoadedImage->ImageSize;

  ImageAddress = LoadedImage->ImageBase;

  PdbPointer = PeCoffLoaderGetPdbPointer ((VOID*) (UINTN) ImageAddress);
  if (PdbPointer != NULL) {
    DEBUG ((DEBUG_VERBOSE, "  Image - %a\n", PdbPointer));
  }

  //
  // Check PE/COFF image
  //
  DosHdr = (EFI_IMAGE_DOS_HEADER *) (UINTN) ImageAddress;
  PeCoffHeaderOffset = 0;
  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    PeCoffHeaderOffset = DosHdr->e_lfanew;
  }

  Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)((UINT8 *) (UINTN) ImageAddress + PeCoffHeaderOffset);
  if (Hdr.Pe32->Signature != EFI_IMAGE_NT_SIGNATURE) {
    DEBUG ((DEBUG_VERBOSE, "Hdr.Pe32->Signature invalid - 0x%x\n", Hdr.Pe32->Signature));
    // It might be image in SMM.
    goto Finish;
  }

  //
  // Get SectionAlignment
  //
  if (Hdr.Pe32->FileHeader.Machine == IMAGE_FILE_MACHINE_IA64 && Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // NOTE: Some versions of Linux ELILO for Itanium have an incorrect magic value
    //       in the PE/COFF Header. If the MachineType is Itanium(IA64) and the
    //       Magic value in the OptionalHeader is EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC
    //       then override the magic value to EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC
    //
    Magic = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
  } else {
    //
    // Get the magic value from the PE/COFF Optional Header
    //
    Magic = Hdr.Pe32->OptionalHeader.Magic;
  }
  if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    SectionAlignment  = Hdr.Pe32->OptionalHeader.SectionAlignment;
  } else {
    SectionAlignment  = Hdr.Pe32Plus->OptionalHeader.SectionAlignment;
  }

  IsAligned = IsMemoryProtectionSectionAligned (SectionAlignment, LoadedImage->ImageCodeType);
  if (!IsAligned) {
    DEBUG ((DEBUG_VERBOSE, "!!!!!!!!  ProtectUefiImageCommon - Section Alignment(0x%x) is incorrect  !!!!!!!!\n",
      SectionAlignment));
    PdbPointer = PeCoffLoaderGetPdbPointer ((VOID*) (UINTN) ImageAddress);
    if (PdbPointer != NULL) {
      DEBUG ((DEBUG_VERBOSE, "!!!!!!!!  Image - %a  !!!!!!!!\n", PdbPointer));
    }
    goto Finish;
  }

  Section = (EFI_IMAGE_SECTION_HEADER *) (
               (UINT8 *) (UINTN) ImageAddress +
               PeCoffHeaderOffset +
               sizeof(UINT32) +
               sizeof(EFI_IMAGE_FILE_HEADER) +
               Hdr.Pe32->FileHeader.SizeOfOptionalHeader
               );
  ImageRecord->CodeSegmentCount = 0;
  InitializeListHead (&ImageRecord->CodeSegmentList);
  for (Index = 0; Index < Hdr.Pe32->FileHeader.NumberOfSections; Index++) {
    Name = Section[Index].Name;
    DEBUG ((
      DEBUG_VERBOSE,
      "  Section - '%c%c%c%c%c%c%c%c'\n",
      Name[0],
      Name[1],
      Name[2],
      Name[3],
      Name[4],
      Name[5],
      Name[6],
      Name[7]
      ));

    if ((Section[Index].Characteristics & EFI_IMAGE_SCN_CNT_CODE) != 0) {
      DEBUG ((DEBUG_VERBOSE, "  VirtualSize          - 0x%08x\n", Section[Index].Misc.VirtualSize));
      DEBUG ((DEBUG_VERBOSE, "  VirtualAddress       - 0x%08x\n", Section[Index].VirtualAddress));
      DEBUG ((DEBUG_VERBOSE, "  SizeOfRawData        - 0x%08x\n", Section[Index].SizeOfRawData));
      DEBUG ((DEBUG_VERBOSE, "  PointerToRawData     - 0x%08x\n", Section[Index].PointerToRawData));
      DEBUG ((DEBUG_VERBOSE, "  PointerToRelocations - 0x%08x\n", Section[Index].PointerToRelocations));
      DEBUG ((DEBUG_VERBOSE, "  PointerToLinenumbers - 0x%08x\n", Section[Index].PointerToLinenumbers));
      DEBUG ((DEBUG_VERBOSE, "  NumberOfRelocations  - 0x%08x\n", Section[Index].NumberOfRelocations));
      DEBUG ((DEBUG_VERBOSE, "  NumberOfLinenumbers  - 0x%08x\n", Section[Index].NumberOfLinenumbers));
      DEBUG ((DEBUG_VERBOSE, "  Characteristics      - 0x%08x\n", Section[Index].Characteristics));

      //
      // Step 2: record code section
      //
      ImageRecordCodeSection = AllocatePool (sizeof(*ImageRecordCodeSection));
      if (ImageRecordCodeSection == NULL) {
        return ;
      }
      ImageRecordCodeSection->Signature = IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE;

      ImageRecordCodeSection->CodeSegmentBase = (UINTN)ImageAddress + Section[Index].VirtualAddress;
      ImageRecordCodeSection->CodeSegmentSize = ALIGN_VALUE(Section[Index].SizeOfRawData, SectionAlignment);

      DEBUG ((DEBUG_VERBOSE, "ImageCode: 0x%016lx - 0x%016lx\n", ImageRecordCodeSection->CodeSegmentBase, ImageRecordCodeSection->CodeSegmentSize));

      InsertTailList (&ImageRecord->CodeSegmentList, &ImageRecordCodeSection->Link);
      ImageRecord->CodeSegmentCount++;
    }
  }

  if (ImageRecord->CodeSegmentCount == 0) {
    DEBUG ((DEBUG_ERROR, "!!!!!!!!  ProtectUefiImageCommon - CodeSegmentCount is 0  !!!!!!!!\n"));
    PdbPointer = PeCoffLoaderGetPdbPointer ((VOID*) (UINTN) ImageAddress);
    if (PdbPointer != NULL) {
      DEBUG ((DEBUG_ERROR, "!!!!!!!!  Image - %a  !!!!!!!!\n", PdbPointer));
    }
    goto Finish;
  }

  //
  // Final
  //
  SortImageRecordCodeSection (ImageRecord);
  //
  // Check overlap all section in ImageBase/Size
  //
  if (!IsImageRecordCodeSectionValid (ImageRecord)) {
    DEBUG ((DEBUG_ERROR, "IsImageRecordCodeSectionValid - FAIL\n"));
    goto Finish;
  }

  //
  // Round up the ImageSize, some CPU arch may return EFI_UNSUPPORTED if ImageSize is not aligned.
  // Given that the loader always allocates full pages, we know the space after the image is not used.
  //
  ImageRecord->ImageSize = ALIGN_VALUE(LoadedImage->ImageSize, EFI_PAGE_SIZE);

  //
  // CPU ARCH present. Update memory attribute directly.
  //
  SetUefiImageProtectionAttributes (ImageRecord, Protect);

  //
  // Clean up
  //
  FreeImageRecord (ImageRecord);

Finish:
  return ;
}

/**
  Protect UEFI image.

  @param[in]  LoadedImage              The loaded image protocol
  @param[in]  LoadedImageDevicePath    The loaded image device path protocol
**/
VOID
ProtectUefiImage (
  IN EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage,
  IN EFI_DEVICE_PATH_PROTOCOL    *LoadedImageDevicePath
  )
{
  if (PcdGet32(PcdImageProtectionPolicy) != 0) {
    ProtectUefiImageCommon (LoadedImage, LoadedImageDevicePath, TRUE);
  }
}

/**
  Unprotect UEFI image.

  @param[in]  LoadedImage              The loaded image protocol
  @param[in]  LoadedImageDevicePath    The loaded image device path protocol
**/
VOID
UnprotectUefiImage (
  IN EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage,
  IN EFI_DEVICE_PATH_PROTOCOL    *LoadedImageDevicePath
  )
{
  if (PcdGet32(PcdImageProtectionPolicy) != 0) {
    ProtectUefiImageCommon (LoadedImage, LoadedImageDevicePath, FALSE);
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
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
{
  EFI_STATUS                  Status;
  EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL    *LoadedImageDevicePath;
  UINTN                       NoHandles;
  EFI_HANDLE                  *HandleBuffer;
  UINTN                       Index;

  DEBUG ((DEBUG_INFO, "MemoryProtectionCpuArchProtocolNotify:\n"));
  Status = CoreLocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&gCpu);
  if (EFI_ERROR (Status)) {
    return;
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiLoadedImageProtocolGuid,
                  NULL,
                  &NoHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status) && (NoHandles == 0)) {
    return ;
  }

  for (Index = 0; Index < NoHandles; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **)&LoadedImage
                    );
    if (EFI_ERROR(Status)) {
      continue;
    }
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiLoadedImageDevicePathProtocolGuid,
                    (VOID **)&LoadedImageDevicePath
                    );
    if (EFI_ERROR(Status)) {
      LoadedImageDevicePath = NULL;
    }

    ProtectUefiImage (LoadedImage, LoadedImageDevicePath);
  }

  CoreCloseEvent (Event);
  return;
}

/**
  ExitBootServices Callback function for memory protection.
**/
VOID
MemoryProtectionExitBootServicesCallback (
  VOID
  )
{
  EFI_RUNTIME_IMAGE_ENTRY       *RuntimeImage;
  LIST_ENTRY                    *Link;

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
      SetUefiImageMemoryAttributes ((UINT64)(UINTN)RuntimeImage->ImageBase, ALIGN_VALUE(RuntimeImage->ImageSize, EFI_PAGE_SIZE), 0);
    }
  }
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
  VOID        *Registration;

  mImageProtectionPolicy = PcdGet32(PcdImageProtectionPolicy);

  if (mImageProtectionPolicy != 0) {
    Status = CoreCreateEvent (
               EVT_NOTIFY_SIGNAL,
               TPL_CALLBACK,
               MemoryProtectionCpuArchProtocolNotify,
               NULL,
               &Event
               );
    ASSERT_EFI_ERROR(Status);

    //
    // Register for protocol notifactions on this event
    //
    Status = CoreRegisterProtocolNotify (
               &gEfiCpuArchProtocolGuid,
               Event,
               &Registration
               );
    ASSERT_EFI_ERROR(Status);
  }
  return ;
}
