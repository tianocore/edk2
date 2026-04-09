/** @file

  This file contains implementation for additional PE/COFF functionality needed to use
  Platform Runtime Mechanism (PRM) modules.

  Copyright (c) Microsoft Corporation
  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/PeImage.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PeCoffLib.h>

#include <PrmExportDescriptor.h>
#include <PrmModuleImageContext.h>

#define _DBGMSGID_  "[PRMPECOFFLIB]"

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
  IN  EFI_IMAGE_EXPORT_DIRECTORY           *ImageExportDirectory,
  IN  PE_COFF_LOADER_IMAGE_CONTEXT         *PeCoffLoaderImageContext,
  OUT PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT  **ExportDescriptor
  )
{
  UINTN                                Index;
  EFI_PHYSICAL_ADDRESS                 CurrentImageAddress;
  UINT16                               PrmModuleExportDescriptorOrdinal;
  CONST CHAR8                          *CurrentExportName;
  UINT16                               *OrdinalTable;
  UINT32                               *ExportNamePointerTable;
  UINT32                               *ExportAddressTable;
  PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT  *TempExportDescriptor;

  DEBUG ((DEBUG_INFO, "%a %a - Entry.\n", _DBGMSGID_, __func__));

  if ((ImageExportDirectory == NULL) ||
      (PeCoffLoaderImageContext == NULL) ||
      (PeCoffLoaderImageContext->ImageAddress == 0) ||
      (ExportDescriptor == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  *ExportDescriptor = NULL;

  DEBUG ((
    DEBUG_INFO,
    "  %a %a: %d exported names found in this image.\n",
    _DBGMSGID_,
    __func__,
    ImageExportDirectory->NumberOfNames
    ));

  //
  // The export name pointer table and export ordinal table form two parallel arrays associated by index.
  //
  CurrentImageAddress    = PeCoffLoaderImageContext->ImageAddress;
  ExportAddressTable     = (UINT32 *)((UINTN)CurrentImageAddress + ImageExportDirectory->AddressOfFunctions);
  ExportNamePointerTable = (UINT32 *)((UINTN)CurrentImageAddress + ImageExportDirectory->AddressOfNames);
  OrdinalTable           = (UINT16 *)((UINTN)CurrentImageAddress + ImageExportDirectory->AddressOfNameOrdinals);

  for (Index = 0; Index < ImageExportDirectory->NumberOfNames; Index++) {
    CurrentExportName = (CONST CHAR8 *)((UINTN)CurrentImageAddress + ExportNamePointerTable[Index]);
    DEBUG ((
      DEBUG_INFO,
      "  %a %a: Export Name[0x%x] - %a.\n",
      _DBGMSGID_,
      __func__,
      Index,
      CurrentExportName
      ));
    if (
        AsciiStrnCmp (
          PRM_STRING (PRM_MODULE_EXPORT_DESCRIPTOR_NAME),
          CurrentExportName,
          AsciiStrLen (PRM_STRING (PRM_MODULE_EXPORT_DESCRIPTOR_NAME))
          ) == 0)
    {
      PrmModuleExportDescriptorOrdinal = OrdinalTable[Index];
      DEBUG ((
        DEBUG_INFO,
        "  %a %a: PRM Module Export Descriptor found. Ordinal = %d.\n",
        _DBGMSGID_,
        __func__,
        PrmModuleExportDescriptorOrdinal
        ));
      if (PrmModuleExportDescriptorOrdinal >= ImageExportDirectory->NumberOfFunctions) {
        DEBUG ((DEBUG_ERROR, "%a %a: The PRM Module Export Descriptor ordinal value is invalid.\n", _DBGMSGID_, __func__));
        return EFI_NOT_FOUND;
      }

      TempExportDescriptor = (PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT *)((UINTN)CurrentImageAddress + ExportAddressTable[PrmModuleExportDescriptorOrdinal]);
      if (TempExportDescriptor->Header.Signature == PRM_MODULE_EXPORT_DESCRIPTOR_SIGNATURE) {
        *ExportDescriptor = TempExportDescriptor;
        DEBUG ((DEBUG_INFO, "  %a %a: PRM Module Export Descriptor found at 0x%x.\n", _DBGMSGID_, __func__, (UINTN)ExportDescriptor));
      } else {
        DEBUG ((
          DEBUG_INFO,
          "  %a %a: PRM Module Export Descriptor found at 0x%x but signature check failed.\n",
          _DBGMSGID_,
          __func__,
          (UINTN)TempExportDescriptor
          ));
      }

      DEBUG ((DEBUG_INFO, "  %a %a: Exiting export iteration since export descriptor found.\n", _DBGMSGID_, __func__));
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
  IN  VOID                          *Image,
  IN  PE_COFF_LOADER_IMAGE_CONTEXT  *PeCoffLoaderImageContext,
  OUT EFI_IMAGE_EXPORT_DIRECTORY    **ImageExportDirectory
  )
{
  UINT16                               Magic;
  UINT32                               NumberOfRvaAndSizes;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION  OptionalHeaderPtrUnion;
  EFI_IMAGE_DATA_DIRECTORY             *DirectoryEntry;
  EFI_IMAGE_EXPORT_DIRECTORY           *ExportDirectory;

  if ((Image == NULL) || (PeCoffLoaderImageContext == NULL) || (ImageExportDirectory == NULL)) {
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
      //
      // Assume PE32 image with IA32 Machine field.
      //
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
      break;
    case EFI_IMAGE_MACHINE_X64:
    case EFI_IMAGE_MACHINE_AARCH64:
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
        __func__
        ));
      return EFI_UNSUPPORTED;
  }

  OptionalHeaderPtrUnion.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)(
                                                           (UINTN)Image +
                                                           PeCoffLoaderImageContext->PeCoffHeaderOffset
                                                           );

  //
  // Check the PE/COFF Header Signature. Determine if the image is valid and/or a TE image.
  //
  if (OptionalHeaderPtrUnion.Pe32->Signature != EFI_IMAGE_NT_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "%a %a: The PE signature is not valid for the current image.\n", _DBGMSGID_, __func__));
    return EFI_UNSUPPORTED;
  }

  if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // Use the PE32 offset to get the Export Directory Entry
    //
    NumberOfRvaAndSizes = OptionalHeaderPtrUnion.Pe32->OptionalHeader.NumberOfRvaAndSizes;
    DirectoryEntry      = (EFI_IMAGE_DATA_DIRECTORY *)&(OptionalHeaderPtrUnion.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXPORT]);
  } else if (OptionalHeaderPtrUnion.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    //
    // Use the PE32+ offset get the Export Directory Entry
    //
    NumberOfRvaAndSizes = OptionalHeaderPtrUnion.Pe32Plus->OptionalHeader.NumberOfRvaAndSizes;
    DirectoryEntry      = (EFI_IMAGE_DATA_DIRECTORY *)&(OptionalHeaderPtrUnion.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXPORT]);
  } else {
    return EFI_UNSUPPORTED;
  }

  if ((NumberOfRvaAndSizes <= EFI_IMAGE_DIRECTORY_ENTRY_EXPORT) || (DirectoryEntry->VirtualAddress == 0)) {
    //
    // The export directory is not present
    //
    return EFI_NOT_FOUND;
  } else if (((UINT32)(~0) - DirectoryEntry->VirtualAddress) < DirectoryEntry->Size) {
    //
    // The directory address overflows
    //
    DEBUG ((DEBUG_ERROR, "%a %a: The export directory entry in this image results in overflow.\n", _DBGMSGID_, __func__));
    return EFI_UNSUPPORTED;
  } else {
    DEBUG ((DEBUG_INFO, "%a %a: Export Directory Entry found in the image at 0x%x.\n", _DBGMSGID_, __func__, (UINTN)OptionalHeaderPtrUnion.Pe32));
    DEBUG ((DEBUG_INFO, "  %a %a: Directory Entry Virtual Address = 0x%x.\n", _DBGMSGID_, __func__, DirectoryEntry->VirtualAddress));

    ExportDirectory = (EFI_IMAGE_EXPORT_DIRECTORY *)((UINTN)Image + DirectoryEntry->VirtualAddress);
    DEBUG ((
      DEBUG_INFO,
      "  %a %a: Export Directory Table found successfully at 0x%x. Name address = 0x%x. Name = %a.\n",
      _DBGMSGID_,
      __func__,
      (UINTN)ExportDirectory,
      ((UINTN)Image + ExportDirectory->Name),
      (CHAR8 *)((UINTN)Image + ExportDirectory->Name)
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
  IN  VOID                          *Image,
  IN  PE_COFF_LOADER_IMAGE_CONTEXT  *PeCoffLoaderImageContext,
  OUT UINT16                        *ImageMajorVersion,
  OUT UINT16                        *ImageMinorVersion
  )
{
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION  OptionalHeaderPtrUnion;
  UINT16                               Magic;

  DEBUG ((DEBUG_INFO, "    %a %a - Entry.\n", _DBGMSGID_, __func__));

  if ((Image == NULL) || (PeCoffLoaderImageContext == NULL) || (ImageMajorVersion == NULL) || (ImageMinorVersion == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // NOTE: For backward compatibility, use the Machine field to identify a PE32/PE32+
  //       image instead of using the Magic field. Some systems might generate a PE32+
  //       image with PE32 magic.
  //
  switch (PeCoffLoaderImageContext->Machine) {
    case EFI_IMAGE_MACHINE_IA32:
      //
      // Assume PE32 image
      //
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
      break;
    case EFI_IMAGE_MACHINE_X64:
    case EFI_IMAGE_MACHINE_AARCH64:
      //
      // Assume PE32+ image
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
        __func__
        ));
      return EFI_UNSUPPORTED;
  }

  OptionalHeaderPtrUnion.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)(
                                                           (UINTN)Image +
                                                           PeCoffLoaderImageContext->PeCoffHeaderOffset
                                                           );
  //
  // Check the PE/COFF Header Signature. Determine if the image is valid and/or a TE image.
  //
  if (OptionalHeaderPtrUnion.Pe32->Signature != EFI_IMAGE_NT_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "%a %a: The PE signature is not valid for the current image.\n", _DBGMSGID_, __func__));
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

  DEBUG ((DEBUG_INFO, "      %a %a - Image Major Version: 0x%02x.\n", _DBGMSGID_, __func__, *ImageMajorVersion));
  DEBUG ((DEBUG_INFO, "      %a %a - Image Minor Version: 0x%02x.\n", _DBGMSGID_, __func__, *ImageMinorVersion));

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
  IN  CONST CHAR8                 *ExportName,
  IN  EFI_PHYSICAL_ADDRESS        ImageBaseAddress,
  IN  EFI_IMAGE_EXPORT_DIRECTORY  *ImageExportDirectory,
  OUT EFI_PHYSICAL_ADDRESS        *ExportPhysicalAddress
  )
{
  UINTN        ExportNameIndex;
  UINT16       CurrentExportOrdinal;
  UINT32       *ExportAddressTable;
  UINT32       *ExportNamePointerTable;
  UINT16       *OrdinalTable;
  CONST CHAR8  *ExportNameTablePointerName;

  if ((ExportName == NULL) || (ImageBaseAddress == 0) || (ImageExportDirectory == NULL) || (ExportPhysicalAddress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *ExportPhysicalAddress = 0;

  ExportAddressTable     = (UINT32 *)((UINTN)ImageBaseAddress + ImageExportDirectory->AddressOfFunctions);
  ExportNamePointerTable = (UINT32 *)((UINTN)ImageBaseAddress + ImageExportDirectory->AddressOfNames);
  OrdinalTable           = (UINT16 *)((UINTN)ImageBaseAddress + ImageExportDirectory->AddressOfNameOrdinals);

  for (ExportNameIndex = 0; ExportNameIndex < ImageExportDirectory->NumberOfNames; ExportNameIndex++) {
    ExportNameTablePointerName = (CONST CHAR8 *)((UINTN)ImageBaseAddress + ExportNamePointerTable[ExportNameIndex]);

    if (AsciiStrnCmp (ExportName, ExportNameTablePointerName, PRM_HANDLER_NAME_MAXIMUM_LENGTH) == 0) {
      CurrentExportOrdinal = OrdinalTable[ExportNameIndex];

      ASSERT (CurrentExportOrdinal < ImageExportDirectory->NumberOfFunctions);
      if (CurrentExportOrdinal >= ImageExportDirectory->NumberOfFunctions) {
        DEBUG ((DEBUG_ERROR, "  %a %a: The export ordinal value is invalid.\n", _DBGMSGID_, __func__));
        break;
      }

      *ExportPhysicalAddress = (EFI_PHYSICAL_ADDRESS)((UINTN)ImageBaseAddress + ExportAddressTable[CurrentExportOrdinal]);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}
