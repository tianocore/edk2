/** @file

  This file contains the implementation for a Platform Runtime Mechanism (PRM)
  loader driver.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PrmAcpiTable.h"
#include "PrmLoader.h"

#include <IndustryStandard/Acpi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrmContextBufferLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/PrmConfig.h>

#include <PrmContextBuffer.h>
#include <PrmMmio.h>

LIST_ENTRY          mPrmModuleList;

// Todo: Potentially refactor mPrmHandlerCount and mPrmModuleCount into localized structures
//       in the future.
UINT32              mPrmHandlerCount;
UINT32              mPrmModuleCount;

/**
  Gets a pointer to the export directory in a given PE/COFF image.

  @param[in]  ImageExportDirectory        A pointer to an export directory table in a PE/COFF image.
  @param[in]  PeCoffLoaderImageContext    A pointer to a PE_COFF_LOADER_IMAGE_CONTEXT structure that contains the
                                          PE/COFF image context for the Image containing the PRM Module Export
                                          Descriptor table.
  @param[out] ExportDescriptor            A pointer to a pointer to the PRM Module Export Descriptor table found
                                          in the ImageExportDirectory given.

  @retval EFI_SUCCESS                     The PRM Module Export Descriptor table was found successfully.
  @retval EFI_INVALID_PARAMETER           A required parameter is NULL.
  @retval EFI_NOT_FOUND                   The PRM Module Export Descriptor table was not found in the given
                                          ImageExportDirectory.

**/
EFI_STATUS
GetPrmModuleExportDescriptorTable (
  IN  EFI_IMAGE_EXPORT_DIRECTORY          *ImageExportDirectory,
  IN  PE_COFF_LOADER_IMAGE_CONTEXT        *PeCoffLoaderImageContext,
  OUT PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT **ExportDescriptor
  )
{
  UINTN                                   Index;
  EFI_PHYSICAL_ADDRESS                    CurrentImageAddress;
  UINT16                                  PrmModuleExportDescriptorOrdinal;
  CONST CHAR8                             *CurrentExportName;
  UINT16                                  *OrdinalTable;
  UINT32                                  *ExportNamePointerTable;
  UINT32                                  *ExportAddressTable;
  PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT     *TempExportDescriptor;

  DEBUG ((DEBUG_INFO, "%a %a - Entry.\n", _DBGMSGID_, __FUNCTION__));

  *ExportDescriptor = NULL;

  if (ImageExportDirectory == NULL ||
      PeCoffLoaderImageContext == NULL ||
      PeCoffLoaderImageContext->ImageAddress == 0 ||
      ExportDescriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((
    DEBUG_INFO,
    "  %a %a: %d exported names found in this image.\n",
    _DBGMSGID_,
    __FUNCTION__,
    ImageExportDirectory->NumberOfNames
    ));

  //
  // The export name pointer table and export ordinal table form two parallel arrays associated by index.
  //
  CurrentImageAddress = PeCoffLoaderImageContext->ImageAddress;
  ExportAddressTable = (UINT32 *) ((UINTN) CurrentImageAddress + ImageExportDirectory->AddressOfFunctions);
  ExportNamePointerTable = (UINT32 *) ((UINTN) CurrentImageAddress + ImageExportDirectory->AddressOfNames);
  OrdinalTable = (UINT16 *) ((UINTN) CurrentImageAddress + ImageExportDirectory->AddressOfNameOrdinals);

  for (Index = 0; Index < ImageExportDirectory->NumberOfNames; Index++) {
    CurrentExportName = (CONST CHAR8 *) ((UINTN) CurrentImageAddress + ExportNamePointerTable[Index]);
    DEBUG ((
      DEBUG_INFO,
      "  %a %a: Export Name[0x%x] - %a.\n",
      _DBGMSGID_,
      __FUNCTION__,
      Index,
      CurrentExportName
      ));
    if (
      AsciiStrnCmp (
        PRM_STRING(PRM_MODULE_EXPORT_DESCRIPTOR_NAME),
        CurrentExportName,
        AsciiStrLen (PRM_STRING(PRM_MODULE_EXPORT_DESCRIPTOR_NAME))
        ) == 0) {
      PrmModuleExportDescriptorOrdinal = OrdinalTable[Index];
      DEBUG ((
        DEBUG_INFO,
        "  %a %a: PRM Module Export Descriptor found. Ordinal = %d.\n",
        _DBGMSGID_,
        __FUNCTION__,
        PrmModuleExportDescriptorOrdinal
        ));
      if (PrmModuleExportDescriptorOrdinal >= ImageExportDirectory->NumberOfFunctions) {
        DEBUG ((DEBUG_ERROR, "%a %a: The PRM Module Export Descriptor ordinal value is invalid.\n", _DBGMSGID_, __FUNCTION__));
        return EFI_NOT_FOUND;
      }
      TempExportDescriptor = (PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT *) ((UINTN) CurrentImageAddress + ExportAddressTable[PrmModuleExportDescriptorOrdinal]);
      if (TempExportDescriptor->Header.Signature == PRM_MODULE_EXPORT_DESCRIPTOR_SIGNATURE) {
        *ExportDescriptor = TempExportDescriptor;
        DEBUG ((DEBUG_INFO, "  %a %a: PRM Module Export Descriptor found at 0x%x.\n", _DBGMSGID_, __FUNCTION__, (UINTN) ExportDescriptor));
      } else {
        DEBUG ((
          DEBUG_INFO,
          "  %a %a: PRM Module Export Descriptor found at 0x%x but signature check failed.\n",
          _DBGMSGID_,
          __FUNCTION__,
          (UINTN) TempExportDescriptor
          ));
      }
      DEBUG ((DEBUG_INFO, "  %a %a: Exiting export iteration since export descriptor found.\n", _DBGMSGID_, __FUNCTION__));
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Gets a pointer to the export directory in a given PE/COFF image.

  @param[in]  Image                       A pointer to a PE32/COFF image base address that is loaded into memory
                                          and already relocated to the memory base address. RVAs in the image given
                                          should be valid.
  @param[in]  PeCoffLoaderImageContext    A pointer to a PE_COFF_LOADER_IMAGE_CONTEXT structure that contains the
                                          PE/COFF image context for the Image given.
  @param[out] ImageExportDirectory        A pointer to a pointer to the export directory found in the Image given.

  @retval EFI_SUCCESS                     The export directory was found successfully.
  @retval EFI_INVALID_PARAMETER           A required parameter is NULL.
  @retval EFI_UNSUPPORTED                 The PE/COFF image given is not supported as a PRM Module.
  @retval EFI_NOT_FOUND                   The image export directory could not be found for this image.

**/
EFI_STATUS
GetExportDirectoryInPeCoffImage (
  IN  VOID                                *Image,
  IN  PE_COFF_LOADER_IMAGE_CONTEXT        *PeCoffLoaderImageContext,
  OUT EFI_IMAGE_EXPORT_DIRECTORY          **ImageExportDirectory
  )
{
  UINT16                                  Magic;
  UINT32                                  NumberOfRvaAndSizes;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION     OptionalHeaderPtrUnion;
  EFI_IMAGE_DATA_DIRECTORY                *DirectoryEntry;
  EFI_IMAGE_EXPORT_DIRECTORY              *ExportDirectory;
  EFI_IMAGE_SECTION_HEADER                *SectionHeader;

  if (Image == NULL || PeCoffLoaderImageContext == NULL || ImageExportDirectory == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DirectoryEntry  = NULL;
  ExportDirectory = NULL;

  //
  // NOTE: For backward compatibility, use the Machine field to identify a PE32/PE32+
  //       image instead of using the Magic field. Some systems might generate a PE32+
  //       image with PE32 magic.
  //
  switch (PeCoffLoaderImageContext->Machine) {
  case EFI_IMAGE_MACHINE_IA32:
    // Todo: Add EFI_IMAGE_MACHINE_ARMT
    //
    // Assume PE32 image with IA32 Machine field.
    //
    Magic = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    break;
  case EFI_IMAGE_MACHINE_X64:
    //
    // Assume PE32+ image with X64 Machine field
    //
    Magic = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    break;
  default:
    //
    // For unknown Machine field, use Magic in optional header
    //
    DEBUG ((
      DEBUG_WARN,
      "%a %a: The machine type for this image is not valid for a PRM module.\n",
      _DBGMSGID_,
      __FUNCTION__
      ));
    return EFI_UNSUPPORTED;
  }

  OptionalHeaderPtrUnion.Pe32 = (EFI_IMAGE_NT_HEADERS32 *) (
                                  (UINTN) Image +
                                  PeCoffLoaderImageContext->PeCoffHeaderOffset
                                  );

  //
  // Check the PE/COFF Header Signature. Determine if the image is valid and/or a TE image.
  //
  if (OptionalHeaderPtrUnion.Pe32->Signature != EFI_IMAGE_NT_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "%a %a: The PE signature is not valid for the current image.\n", _DBGMSGID_, __FUNCTION__));
    return EFI_UNSUPPORTED;
  }

  SectionHeader = (EFI_IMAGE_SECTION_HEADER *) (
                    (UINTN) Image +
                    PeCoffLoaderImageContext->PeCoffHeaderOffset +
                    sizeof (UINT32) +
                    sizeof (EFI_IMAGE_FILE_HEADER) +
                    PeCoffLoaderImageContext->SizeOfHeaders
                    );
  if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // Use the PE32 offset to get the Export Directory Entry
    //
    NumberOfRvaAndSizes = OptionalHeaderPtrUnion.Pe32->OptionalHeader.NumberOfRvaAndSizes;
    DirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *) &(OptionalHeaderPtrUnion.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXPORT]);
  } else if (OptionalHeaderPtrUnion.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    //
    // Use the PE32+ offset get the Export Directory Entry
    //
    NumberOfRvaAndSizes = OptionalHeaderPtrUnion.Pe32Plus->OptionalHeader.NumberOfRvaAndSizes;
    DirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *) &(OptionalHeaderPtrUnion.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXPORT]);
  } else {
    return EFI_UNSUPPORTED;
  }

  if (NumberOfRvaAndSizes <= EFI_IMAGE_DIRECTORY_ENTRY_EXPORT || DirectoryEntry->VirtualAddress == 0) {
    //
    // The export directory is not present
    //
    return EFI_NOT_FOUND;
  } else if (((UINT32) (~0) - DirectoryEntry->VirtualAddress) < DirectoryEntry->Size) {
    //
    // The directory address overflows
    //
    DEBUG ((DEBUG_ERROR, "%a %a: The export directory entry in this image results in overflow.\n", _DBGMSGID_, __FUNCTION__));
    return EFI_UNSUPPORTED;
  } else {
    DEBUG ((DEBUG_INFO, "%a %a: Export Directory Entry found in the image at 0x%x.\n", _DBGMSGID_, __FUNCTION__, (UINTN) OptionalHeaderPtrUnion.Pe32));
    DEBUG ((DEBUG_INFO, "  %a %a: Directory Entry Virtual Address = 0x%x.\n", _DBGMSGID_, __FUNCTION__, DirectoryEntry->VirtualAddress));

    ExportDirectory = (EFI_IMAGE_EXPORT_DIRECTORY *) ((UINTN) Image + DirectoryEntry->VirtualAddress);
    DEBUG ((
      DEBUG_INFO,
      "  %a %a: Export Directory Table found successfully at 0x%x. Name address = 0x%x. Name = %a.\n",
      _DBGMSGID_,
      __FUNCTION__,
      (UINTN) ExportDirectory,
      ((UINTN) Image + ExportDirectory->Name),
      (CHAR8 *) ((UINTN) Image + ExportDirectory->Name)
      ));
  }
  *ImageExportDirectory = ExportDirectory;

  return EFI_SUCCESS;
}

/**
  Returns the image major and image minor version in a given PE/COFF image.

  @param[in]  Image                       A pointer to a PE32/COFF image base address that is loaded into memory
                                          and already relocated to the memory base address. RVAs in the image given
                                          should be valid.
  @param[in]  PeCoffLoaderImageContext    A pointer to a PE_COFF_LOADER_IMAGE_CONTEXT structure that contains the
                                          PE/COFF image context for the Image given.
  @param[out] ImageMajorVersion           A pointer to a UINT16 buffer to hold the image major version.
  @param[out] ImageMinorVersion           A pointer to a UINT16 buffer to hold the image minor version.

  @retval EFI_SUCCESS                     The image version was read successfully.
  @retval EFI_INVALID_PARAMETER           A required parameter is NULL.
  @retval EFI_UNSUPPORTED                 The PE/COFF image given is not supported.

**/
EFI_STATUS
GetImageVersionInPeCoffImage (
  IN  VOID                                *Image,
  IN  PE_COFF_LOADER_IMAGE_CONTEXT        *PeCoffLoaderImageContext,
  OUT UINT16                              *ImageMajorVersion,
  OUT UINT16                              *ImageMinorVersion
  )
{
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION     OptionalHeaderPtrUnion;
  UINT16                                  Magic;

  DEBUG ((DEBUG_INFO, "    %a %a - Entry.\n", _DBGMSGID_, __FUNCTION__));

  if (Image == NULL || PeCoffLoaderImageContext == NULL || ImageMajorVersion == NULL || ImageMinorVersion == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // NOTE: For backward compatibility, use the Machine field to identify a PE32/PE32+
  //       image instead of using the Magic field. Some systems might generate a PE32+
  //       image with PE32 magic.
  //
  switch (PeCoffLoaderImageContext->Machine) {
  case EFI_IMAGE_MACHINE_IA32:
    // Todo: Add EFI_IMAGE_MACHINE_ARMT
    //
    // Assume PE32 image with IA32 Machine field.
    //
    Magic = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    break;
  case EFI_IMAGE_MACHINE_X64:
    //
    // Assume PE32+ image with X64 Machine field
    //
    Magic = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    break;
  default:
    //
    // For unknown Machine field, use Magic in optional header
    //
    DEBUG ((
      DEBUG_WARN,
      "%a %a: The machine type for this image is not valid for a PRM module.\n",
      _DBGMSGID_,
      __FUNCTION__
      ));
    return EFI_UNSUPPORTED;
  }

  OptionalHeaderPtrUnion.Pe32 = (EFI_IMAGE_NT_HEADERS32 *) (
                                  (UINTN) Image +
                                  PeCoffLoaderImageContext->PeCoffHeaderOffset
                                  );
  //
  // Check the PE/COFF Header Signature. Determine if the image is valid and/or a TE image.
  //
  if (OptionalHeaderPtrUnion.Pe32->Signature != EFI_IMAGE_NT_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "%a %a: The PE signature is not valid for the current image.\n", _DBGMSGID_, __FUNCTION__));
    return EFI_UNSUPPORTED;
  }

  if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // Use the PE32 offset to get the Export Directory Entry
    //
    *ImageMajorVersion = OptionalHeaderPtrUnion.Pe32->OptionalHeader.MajorImageVersion;
    *ImageMinorVersion = OptionalHeaderPtrUnion.Pe32->OptionalHeader.MinorImageVersion;
  } else {
    //
    // Use the PE32+ offset to get the Export Directory Entry
    //
    *ImageMajorVersion = OptionalHeaderPtrUnion.Pe32Plus->OptionalHeader.MajorImageVersion;
    *ImageMinorVersion = OptionalHeaderPtrUnion.Pe32Plus->OptionalHeader.MinorImageVersion;
  }

  DEBUG ((DEBUG_INFO, "      %a %a - Image Major Version: 0x%02x.\n", _DBGMSGID_, __FUNCTION__, *ImageMajorVersion));
  DEBUG ((DEBUG_INFO, "      %a %a - Image Minor Version: 0x%02x.\n", _DBGMSGID_, __FUNCTION__, *ImageMinorVersion));

  return EFI_SUCCESS;
}

/**
  Creates a new PRM Module Image Context linked list entry.

  @retval PrmModuleImageContextListEntry  If successful, a pointer a PRM Module Image Context linked list entry
                                          otherwise, NULL is returned.

**/
STATIC
PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY *
CreateNewPrmModuleImageContextListEntry (
  VOID
  )
{
  PRM_MODULE_IMAGE_CONTEXT                *PrmModuleImageContext;
  PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY     *PrmModuleImageContextListEntry;

  DEBUG ((DEBUG_INFO, "%a %a - Entry.\n", _DBGMSGID_, __FUNCTION__));

  PrmModuleImageContext = AllocateZeroPool (sizeof (*PrmModuleImageContext));
  if (PrmModuleImageContext == NULL) {
    return NULL;
  }
  DEBUG ((
    DEBUG_INFO,
    "  %a %a: Allocated PrmModuleImageContext at 0x%x of size 0x%x bytes.\n",
    _DBGMSGID_,
    __FUNCTION__,
    (UINTN) PrmModuleImageContext,
    sizeof (*PrmModuleImageContext)
    ));

  PrmModuleImageContextListEntry = AllocateZeroPool (sizeof (*PrmModuleImageContextListEntry));
  if (PrmModuleImageContextListEntry == NULL) {
    FreePool (PrmModuleImageContext);
    return NULL;
  }
  DEBUG ((
    DEBUG_INFO,
    "  %a %a: Allocated PrmModuleImageContextListEntry at 0x%x of size 0x%x bytes.\n",
    _DBGMSGID_,
    __FUNCTION__,
    (UINTN) PrmModuleImageContextListEntry,
    sizeof (*PrmModuleImageContextListEntry)
    ));

  PrmModuleImageContextListEntry->Signature = PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY_SIGNATURE;
  PrmModuleImageContextListEntry->Context = PrmModuleImageContext;

  return PrmModuleImageContextListEntry;
}

/**
  Discovers all PRM Modules loaded during the DXE boot phase.

  Each PRM Module discovered is placed into a linked list so the list can br processsed in the future.

  @retval EFI_SUCCESS                     All PRM Modules were discovered successfully.
  @retval EFI_NOT_FOUND                   The gEfiLoadedImageProtocolGuid protocol could not be found.
  @retval EFI_OUT_OF_RESOURCES            Insufficient memory resources to allocate the new PRM Context
                                          linked list nodes.

**/
EFI_STATUS
DiscoverPrmModules (
  VOID
  )
{
  EFI_STATUS                              Status;
  PRM_MODULE_IMAGE_CONTEXT                TempPrmModuleImageContext;
  PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY     *PrmModuleImageContextListEntry;
  EFI_LOADED_IMAGE_PROTOCOL               *LoadedImageProtocol;
  EFI_HANDLE                              *HandleBuffer;
  UINTN                                   HandleCount;
  UINTN                                   Index;

  DEBUG ((DEBUG_INFO, "%a %a - Entry.\n", _DBGMSGID_, __FUNCTION__));

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiLoadedImageProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status) && (HandleCount == 0)) {
    DEBUG ((DEBUG_ERROR, "%a %a: No LoadedImageProtocol instances found!\n", _DBGMSGID_, __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **) &LoadedImageProtocol
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    ZeroMem (&TempPrmModuleImageContext, sizeof (TempPrmModuleImageContext));
    TempPrmModuleImageContext.PeCoffImageContext.Handle    = LoadedImageProtocol->ImageBase;
    TempPrmModuleImageContext.PeCoffImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;

    Status = PeCoffLoaderGetImageInfo (&TempPrmModuleImageContext.PeCoffImageContext);
    if (EFI_ERROR (Status) || TempPrmModuleImageContext.PeCoffImageContext.ImageError != IMAGE_ERROR_SUCCESS) {
      DEBUG ((
        DEBUG_WARN,
        "%a %a: ImageHandle 0x%016lx is not a valid PE/COFF image. It cannot be considered a PRM module.\n",
        _DBGMSGID_,
        __FUNCTION__,
        (EFI_PHYSICAL_ADDRESS) (UINTN) LoadedImageProtocol->ImageBase
        ));
      continue;
    }
    if (TempPrmModuleImageContext.PeCoffImageContext.IsTeImage) {
      // A PRM Module is not allowed to be a TE image
      continue;
    }

    // Attempt to find an export table in this image
    Status =  GetExportDirectoryInPeCoffImage (
                LoadedImageProtocol->ImageBase,
                &TempPrmModuleImageContext.PeCoffImageContext,
                &TempPrmModuleImageContext.ExportDirectory
                );
    if (EFI_ERROR (Status)) {
      continue;
    }

    // Attempt to find the PRM Module Export Descriptor in the export table
    Status = GetPrmModuleExportDescriptorTable (
              TempPrmModuleImageContext.ExportDirectory,
              &TempPrmModuleImageContext.PeCoffImageContext,
              &TempPrmModuleImageContext.ExportDescriptor
              );
    if (EFI_ERROR (Status)) {
      continue;
    }
    // A PRM Module Export Descriptor was successfully found, this is considered a PRM Module.

    //
    // Create a new PRM Module image context node
    //
    PrmModuleImageContextListEntry = CreateNewPrmModuleImageContextListEntry ();
    if (PrmModuleImageContextListEntry == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    CopyMem (
      PrmModuleImageContextListEntry->Context,
      &TempPrmModuleImageContext,
      sizeof (*(PrmModuleImageContextListEntry->Context))
      );
    InsertTailList (&mPrmModuleList, &PrmModuleImageContextListEntry->Link);
    mPrmHandlerCount += TempPrmModuleImageContext.ExportDescriptor->Header.NumberPrmHandlers;
    mPrmModuleCount++; // Todo: Match with global variable refactor change in the future
    DEBUG ((DEBUG_INFO, "%a %a: New PRM Module inserted into list to be processed.\n", _DBGMSGID_, __FUNCTION__));
  }

  return EFI_SUCCESS;
}

/**
  Gets the address of an entry in an image export table by ASCII name.

  @param[in]  ExportName                  A pointer to an ASCII name string of the entry name.
  @param[in]  ImageBaseAddress            The base address of the PE/COFF image.
  @param[in]  ImageExportDirectory        A pointer to the export directory in the image.
  @param[out] ExportPhysicalAddress       A pointer that will be updated with the address of the address of the
                                          export entry if found.

  @retval EFI_SUCCESS                     The export entry was found successfully.
  @retval EFI_INVALID_PARAMETER           A required pointer argument is NULL.
  @retval EFI_NOT_FOUND                   An entry with the given ExportName was not found.

**/
EFI_STATUS
GetExportEntryAddress (
  IN  CONST CHAR8                         *ExportName,
  IN  EFI_PHYSICAL_ADDRESS                ImageBaseAddress,
  IN  EFI_IMAGE_EXPORT_DIRECTORY          *ImageExportDirectory,
  OUT EFI_PHYSICAL_ADDRESS                *ExportPhysicalAddress
  )
{
  UINTN                                   ExportNameIndex;
  UINT16                                  CurrentExportOrdinal;
  UINT32                                  *ExportAddressTable;
  UINT32                                  *ExportNamePointerTable;
  UINT16                                  *OrdinalTable;
  CONST CHAR8                             *ExportNameTablePointerName;

  if (ExportName == NULL || ImageBaseAddress == 0 || ImageExportDirectory == NULL || ExportPhysicalAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *ExportPhysicalAddress = 0;

  ExportAddressTable = (UINT32 *) ((UINTN) ImageBaseAddress + ImageExportDirectory->AddressOfFunctions);
  ExportNamePointerTable = (UINT32 *) ((UINTN) ImageBaseAddress + ImageExportDirectory->AddressOfNames);
  OrdinalTable = (UINT16 *) ((UINTN) ImageBaseAddress + ImageExportDirectory->AddressOfNameOrdinals);

  for (ExportNameIndex = 0; ExportNameIndex < ImageExportDirectory->NumberOfNames; ExportNameIndex++) {
    ExportNameTablePointerName = (CONST CHAR8 *) ((UINTN) ImageBaseAddress + ExportNamePointerTable[ExportNameIndex]);

    if (AsciiStrnCmp (ExportName, ExportNameTablePointerName, PRM_HANDLER_NAME_MAXIMUM_LENGTH) == 0) {
      CurrentExportOrdinal = OrdinalTable[ExportNameIndex];

      ASSERT (CurrentExportOrdinal < ImageExportDirectory->NumberOfFunctions);
      if (CurrentExportOrdinal >= ImageExportDirectory->NumberOfFunctions) {
        DEBUG ((DEBUG_ERROR, "  %a %a: The export ordinal value is invalid.\n", _DBGMSGID_, __FUNCTION__));
        break;
      }

      *ExportPhysicalAddress = (EFI_PHYSICAL_ADDRESS) ((UINTN) ImageBaseAddress + ExportAddressTable[CurrentExportOrdinal]);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Processes a list of PRM context entries to build a PRM ACPI table.

  The ACPI table buffer is allocated and the table structure is built inside this function.

  @param[out]  PrmAcpiDescriptionTable    A pointer to a pointer to a buffer that is allocated within this function
                                          and will contain the PRM ACPI table. In case of an error in this function,
                                          *PrmAcpiDescriptorTable will be NULL.

  @retval EFI_SUCCESS                     All PRM Modules were processed to construct the PRM ACPI table successfully.
  @retval EFI_INVALID_PARAMETER           THe parameter PrmAcpiDescriptionTable is NULL.
  @retval EFI_OUT_OF_RESOURCES            Insufficient memory resources to allocate the PRM ACPI table boot services
                                          memory data buffer.

**/
EFI_STATUS
ProcessPrmModules (
  OUT PRM_ACPI_DESCRIPTION_TABLE          **PrmAcpiDescriptionTable
  )
{
  EFI_IMAGE_EXPORT_DIRECTORY              *CurrentImageExportDirectory;
  PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT     *CurrentExportDescriptorStruct;
  LIST_ENTRY                              *Link;
  PRM_ACPI_DESCRIPTION_TABLE              *PrmAcpiTable;
  PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY     *TempListEntry;
  CONST CHAR8                             *CurrentExportDescriptorHandlerName;

  ACPI_PARAMETER_BUFFER_DESCRIPTOR        *CurrentModuleAcpiParamDescriptors;
  PRM_CONTEXT_BUFFER                      *CurrentContextBuffer;
  PRM_MODULE_CONTEXT_BUFFERS              *CurrentModuleContextBuffers;
  PRM_MODULE_INFORMATION_STRUCT           *CurrentModuleInfoStruct;
  PRM_HANDLER_INFORMATION_STRUCT          *CurrentHandlerInfoStruct;

  EFI_STATUS                              Status;
  EFI_PHYSICAL_ADDRESS                    CurrentImageAddress;
  UINTN                                   AcpiParamIndex;
  UINTN                                   HandlerIndex;
  UINT32                                  PrmAcpiDescriptionTableBufferSize;

  UINT64                                  HandlerPhysicalAddress;

  DEBUG ((DEBUG_INFO, "%a %a - Entry.\n", _DBGMSGID_, __FUNCTION__));

  if (PrmAcpiDescriptionTable == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Link = NULL;
  *PrmAcpiDescriptionTable = NULL;

  DEBUG ((DEBUG_INFO, "  %a %a: %d total PRM modules to process.\n", _DBGMSGID_, __FUNCTION__, mPrmModuleCount));
  DEBUG ((DEBUG_INFO, "  %a %a: %d total PRM handlers to process.\n", _DBGMSGID_, __FUNCTION__, mPrmHandlerCount));

  PrmAcpiDescriptionTableBufferSize = (OFFSET_OF (PRM_ACPI_DESCRIPTION_TABLE, PrmModuleInfoStructure) +
                                        (OFFSET_OF (PRM_MODULE_INFORMATION_STRUCT, HandlerInfoStructure) *  mPrmModuleCount) +
                                        (sizeof (PRM_HANDLER_INFORMATION_STRUCT) * mPrmHandlerCount)
                                        );
  DEBUG ((DEBUG_INFO, "  %a %a: Total PRM ACPI table size: 0x%x.\n", _DBGMSGID_, __FUNCTION__, PrmAcpiDescriptionTableBufferSize));

  PrmAcpiTable = AllocateZeroPool ((UINTN) PrmAcpiDescriptionTableBufferSize);
  if (PrmAcpiTable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PrmAcpiTable->Header.Signature        = PRM_TABLE_SIGNATURE;
  PrmAcpiTable->Header.Length           = PrmAcpiDescriptionTableBufferSize;
  PrmAcpiTable->Header.Revision         = PRM_TABLE_REVISION;
  PrmAcpiTable->Header.Checksum         = 0x0;
  CopyMem (&PrmAcpiTable->Header.OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (PrmAcpiTable->Header.OemId));
  PrmAcpiTable->Header.OemTableId       = PcdGet64 (PcdAcpiDefaultOemTableId);
  PrmAcpiTable->Header.OemRevision      = PcdGet32 (PcdAcpiDefaultOemRevision);
  PrmAcpiTable->Header.CreatorId        = PcdGet32 (PcdAcpiDefaultCreatorId);
  PrmAcpiTable->Header.CreatorRevision  = PcdGet32 (PcdAcpiDefaultCreatorRevision);
  PrmAcpiTable->PrmModuleInfoOffset     = OFFSET_OF (PRM_ACPI_DESCRIPTION_TABLE, PrmModuleInfoStructure);
  PrmAcpiTable->PrmModuleInfoCount      = mPrmModuleCount;

  //
  // Iterate across all PRM Modules on the list
  //
  CurrentModuleInfoStruct = &PrmAcpiTable->PrmModuleInfoStructure[0];
  EFI_LIST_FOR_EACH(Link, &mPrmModuleList)
  {
    TempListEntry = CR(Link, PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY, Link, PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY_SIGNATURE);
    CurrentImageAddress = TempListEntry->Context->PeCoffImageContext.ImageAddress;
    CurrentImageExportDirectory = TempListEntry->Context->ExportDirectory;
    CurrentExportDescriptorStruct = TempListEntry->Context->ExportDescriptor;
    CurrentModuleAcpiParamDescriptors = NULL;

    DEBUG ((
      DEBUG_INFO,
      "  %a %a: PRM Module - %a with %d handlers.\n",
      _DBGMSGID_,
      __FUNCTION__,
      (CHAR8 *) ((UINTN) CurrentImageAddress + CurrentImageExportDirectory->Name),
      CurrentExportDescriptorStruct->Header.NumberPrmHandlers
      ));

    CurrentModuleInfoStruct->StructureRevision = PRM_MODULE_INFORMATION_STRUCT_REVISION;
    CurrentModuleInfoStruct->StructureLength = (
                                             OFFSET_OF (PRM_MODULE_INFORMATION_STRUCT, HandlerInfoStructure) +
                                             (CurrentExportDescriptorStruct->Header.NumberPrmHandlers * sizeof (PRM_HANDLER_INFORMATION_STRUCT))
                                             );
    CopyGuid (&CurrentModuleInfoStruct->Identifier, &CurrentExportDescriptorStruct->Header.ModuleGuid);
    CurrentModuleInfoStruct->HandlerCount       = (UINT32) CurrentExportDescriptorStruct->Header.NumberPrmHandlers;
    CurrentModuleInfoStruct->HandlerInfoOffset  = OFFSET_OF (PRM_MODULE_INFORMATION_STRUCT, HandlerInfoStructure);

    CurrentModuleInfoStruct->MajorRevision = 0;
    CurrentModuleInfoStruct->MinorRevision = 0;
    Status =  GetImageVersionInPeCoffImage (
                (VOID *) (UINTN) CurrentImageAddress,
                &TempListEntry->Context->PeCoffImageContext,
                &CurrentModuleInfoStruct->MajorRevision,
                &CurrentModuleInfoStruct->MinorRevision
                );
    ASSERT_EFI_ERROR (Status);

    // It is currently valid for a PRM module not to use a context buffer
    Status = GetModuleContextBuffers (
              ByModuleGuid,
              &CurrentModuleInfoStruct->Identifier,
              &CurrentModuleContextBuffers
              );
    ASSERT (!EFI_ERROR (Status) || Status == EFI_NOT_FOUND);
    if (!EFI_ERROR (Status) && CurrentModuleContextBuffers != NULL) {
      CurrentModuleInfoStruct->RuntimeMmioRanges = (UINT64) (UINTN) CurrentModuleContextBuffers->RuntimeMmioRanges;
      CurrentModuleAcpiParamDescriptors = CurrentModuleContextBuffers->AcpiParameterBufferDescriptors;
    }

    //
    // Iterate across all PRM handlers in the PRM Module
    //
    for (HandlerIndex = 0; HandlerIndex < CurrentExportDescriptorStruct->Header.NumberPrmHandlers; HandlerIndex++) {
      CurrentHandlerInfoStruct = &(CurrentModuleInfoStruct->HandlerInfoStructure[HandlerIndex]);

      CurrentHandlerInfoStruct->StructureRevision = PRM_HANDLER_INFORMATION_STRUCT_REVISION;
      CurrentHandlerInfoStruct->StructureLength = sizeof (PRM_HANDLER_INFORMATION_STRUCT);
      CopyGuid (
        &CurrentHandlerInfoStruct->Identifier,
        &CurrentExportDescriptorStruct->PrmHandlerExportDescriptors[HandlerIndex].PrmHandlerGuid
        );

      CurrentExportDescriptorHandlerName = (CONST CHAR8 *) CurrentExportDescriptorStruct->PrmHandlerExportDescriptors[HandlerIndex].PrmHandlerName;

      Status =  GetContextBuffer (
                  &CurrentHandlerInfoStruct->Identifier,
                  CurrentModuleContextBuffers,
                  &CurrentContextBuffer
                  );
      if (!EFI_ERROR (Status)) {
        CurrentHandlerInfoStruct->StaticDataBuffer = (UINT64) (UINTN) CurrentContextBuffer->StaticDataBuffer;
      }

      Status =  GetExportEntryAddress (
                  CurrentExportDescriptorHandlerName,
                  CurrentImageAddress,
                  CurrentImageExportDirectory,
                  &HandlerPhysicalAddress
                  );
      ASSERT_EFI_ERROR (Status);
      if (!EFI_ERROR (Status)) {
        CurrentHandlerInfoStruct->PhysicalAddress = HandlerPhysicalAddress;
        DEBUG ((
          DEBUG_INFO,
          "    %a %a: Found %a handler physical address at 0x%016x.\n",
          _DBGMSGID_,
          __FUNCTION__,
          CurrentExportDescriptorHandlerName,
          CurrentHandlerInfoStruct->PhysicalAddress
          ));
      }

      //
      // Update the handler ACPI parameter buffer address if applicable
      //
      if (CurrentModuleAcpiParamDescriptors != NULL) {
        for (AcpiParamIndex = 0; AcpiParamIndex < CurrentModuleContextBuffers->AcpiParameterBufferDescriptorCount; AcpiParamIndex++) {
          if (CompareGuid (&CurrentModuleAcpiParamDescriptors[AcpiParamIndex].HandlerGuid, &CurrentHandlerInfoStruct->Identifier)) {
            CurrentHandlerInfoStruct->AcpiParameterBuffer = (UINT64) (UINTN) (
                                                              CurrentModuleAcpiParamDescriptors[AcpiParamIndex].AcpiParameterBufferAddress
                                                              );
          }
        }
      }
    }
    CurrentModuleInfoStruct = (PRM_MODULE_INFORMATION_STRUCT *) ((UINTN) CurrentModuleInfoStruct + CurrentModuleInfoStruct->StructureLength);
  }
  *PrmAcpiDescriptionTable = PrmAcpiTable;

  return EFI_SUCCESS;
}

/**
  Publishes the PRM ACPI table (PRMT).

  @param[in]  PrmAcpiDescriptionTable     A pointer to a buffer with a completely populated and valid PRM
                                          ACPI description table.

  @retval EFI_SUCCESS                     The PRM ACPI was installed successfully.
  @retval EFI_INVALID_PARAMETER           THe parameter PrmAcpiDescriptionTable is NULL or the table signature
                                          in the table provided is invalid.
  @retval EFI_NOT_FOUND                   The protocol gEfiAcpiTableProtocolGuid could not be found.
  @retval EFI_OUT_OF_RESOURCES            Insufficient memory resources to allocate the PRM ACPI table buffer.

**/
EFI_STATUS
PublishPrmAcpiTable (
  IN  PRM_ACPI_DESCRIPTION_TABLE          *PrmAcpiDescriptionTable
  )
{
  EFI_STATUS                              Status;
  EFI_ACPI_TABLE_PROTOCOL                 *AcpiTableProtocol;
  UINTN                                   TableKey;

  if (PrmAcpiDescriptionTable == NULL || PrmAcpiDescriptionTable->Header.Signature != PRM_TABLE_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **) &AcpiTableProtocol);
  if (!EFI_ERROR (Status)) {
    TableKey = 0;
    //
    // Publish the PRM ACPI table. The table checksum will be computed during installation.
    //
    Status = AcpiTableProtocol->InstallAcpiTable (
                                  AcpiTableProtocol,
                                  PrmAcpiDescriptionTable,
                                  PrmAcpiDescriptionTable->Header.Length,
                                  &TableKey
                                  );
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "%a %a: The PRMT ACPI table was installed successfully.\n", _DBGMSGID_, __FUNCTION__));
    }
  }
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  The PRM Loader END_OF_DXE protocol notification event handler.

  All PRM Modules that are eligible for dispatch should have been loaded the DXE Dispatcher at the
  time of this function invocation.

  The main responsibilities of the PRM Loader are executed from this function which include 3 phases:
    1.) Disover PRM Modules - Find all PRM modules loaded during DXE dispatch and insert a PRM Module
        Context entry into a linked list to be handed off to phase 2.
    2.) Process PRM Modules - Build a GUID to PRM handler mapping for each module that is described in the
        PRM ACPI table so the OS can resolve a PRM Handler GUID to the corresponding PRM Handler physical address.
    3.) Publish PRM ACPI Table - Publish the PRM ACPI table with the information gathered in the phase 2.

  @param[in]  Event                       Event whose notification function is being invoked.
  @param[in]  Context                     The pointer to the notification function's context,
                                          which is implementation-dependent.

  @retval EFI_SUCCESS                     The function executed successfully

**/
EFI_STATUS
EFIAPI
PrmLoaderEndOfDxeNotification (
  IN  EFI_EVENT                           Event,
  IN  VOID                                *Context
  )
{
  EFI_STATUS                              Status;
  PRM_ACPI_DESCRIPTION_TABLE              *PrmAcpiDescriptionTable;

  DEBUG ((DEBUG_INFO, "%a %a - Entry.\n", _DBGMSGID_, __FUNCTION__));

  InitializeListHead (&mPrmModuleList);

  Status = DiscoverPrmModules ();
  ASSERT_EFI_ERROR (Status);

  Status = ProcessPrmModules (&PrmAcpiDescriptionTable);
  ASSERT_EFI_ERROR (Status);

  Status = PublishPrmAcpiTable (PrmAcpiDescriptionTable);
  ASSERT_EFI_ERROR (Status);

  if (PrmAcpiDescriptionTable != NULL) {
    FreePool (PrmAcpiDescriptionTable);
  }
  gBS->CloseEvent (Event);

  return EFI_SUCCESS;
}

/**
  The entry point for this module.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Others         An error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
PrmLoaderEntryPoint (
  IN EFI_HANDLE                           ImageHandle,
  IN EFI_SYSTEM_TABLE                     *SystemTable
  )
{
  EFI_STATUS                              Status;
  EFI_EVENT                               EndOfDxeEvent;

  DEBUG ((DEBUG_INFO, "%a %a - Entry.\n", _DBGMSGID_, __FUNCTION__));

  //
  // Discover and process installed PRM modules at the End of DXE
  // The PRM ACPI table is published if one or PRM modules are discovered
  //
  Status = gBS->CreateEventEx(
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  PrmLoaderEndOfDxeNotification,
                  NULL,
                  &gEfiEndOfDxeEventGroupGuid,
                  &EndOfDxeEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a %a: EndOfDxe callback registration failed! %r.\n", _DBGMSGID_, __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}
