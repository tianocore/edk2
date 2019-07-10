/** @file

 Library to rebase PE image.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Rebase.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef __GNUC__
#include <unistd.h>
#else
#include <io.h>
#include <direct.h>
#endif
#include <PeCoffLib.h>
#include <CommonLib.h>
#include <IndustryStandard/PeImage.h>
#include <FvLib.h>
#include "EfiUtilityMsgs.h"

static
EFI_STATUS
FfsRebaseImageRead(
IN     VOID    *FileHandle,
IN     UINTN   FileOffset,
IN OUT UINT32  *ReadSize,
OUT    VOID    *Buffer
);

EFI_STATUS
RebaseFfs(
IN OUT  UINT64                 BaseAddress,
IN      CHAR8                 *FileName,
IN OUT  EFI_FFS_FILE_HEADER   *FfsFile,
IN      UINTN                 XipOffset
)
/*++

Routine Description:

This function determines if a file is XIP and should be rebased.  It will
rebase any PE32 sections found in the file using the base address.

Arguments:

FvInfo            A pointer to FV_INFO struture.
FileName          Ffs File PathName
FfsFile           A pointer to Ffs file image.
XipOffset         The offset address to use for rebasing the XIP file image.

Returns:

EFI_SUCCESS             The image was properly rebased.
EFI_INVALID_PARAMETER   An input parameter is invalid.
EFI_ABORTED             An error occurred while rebasing the input file image.
EFI_OUT_OF_RESOURCES    Could not allocate a required resource.
EFI_NOT_FOUND           No compressed sections could be found.

--*/
{
  EFI_STATUS                            Status;
  PE_COFF_LOADER_IMAGE_CONTEXT          ImageContext;
  PE_COFF_LOADER_IMAGE_CONTEXT          OrigImageContext;
  EFI_PHYSICAL_ADDRESS                  XipBase;
  EFI_PHYSICAL_ADDRESS                  NewPe32BaseAddress;
  UINTN                                 Index;
  EFI_FILE_SECTION_POINTER              CurrentPe32Section;
  EFI_FFS_FILE_STATE                    SavedState;
  EFI_IMAGE_OPTIONAL_HEADER_UNION       *ImgHdr;
  EFI_TE_IMAGE_HEADER                   *TEImageHeader;
  UINT8                                 *MemoryImagePointer;
  EFI_IMAGE_SECTION_HEADER              *SectionHeader;
  CHAR8                                 PeFileName[MAX_LONG_FILE_PATH];
  CHAR8                                 *Cptr;
  FILE                                  *PeFile;
  UINT8                                 *PeFileBuffer;
  UINT32                                PeFileSize;
  CHAR8                                 *PdbPointer;
  UINT32                                FfsHeaderSize;
  UINT32                                CurSecHdrSize;
  CHAR8                                 *LongFilePathName;

  Index = 0;
  MemoryImagePointer = NULL;
  TEImageHeader = NULL;
  ImgHdr = NULL;
  SectionHeader = NULL;
  Cptr = NULL;
  PeFile = NULL;
  PeFileBuffer = NULL;

  //
  // Don't need to relocate image when BaseAddress is zero and no ForceRebase Flag specified.
  //
  if (BaseAddress == 0) {
    return EFI_SUCCESS;
  }

  XipBase = BaseAddress + XipOffset;

  //
  // We only process files potentially containing PE32 sections.
  //
  switch (FfsFile->Type) {
  case EFI_FV_FILETYPE_SECURITY_CORE:
  case EFI_FV_FILETYPE_PEI_CORE:
  case EFI_FV_FILETYPE_PEIM:
  case EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER:
  case EFI_FV_FILETYPE_DRIVER:
  case EFI_FV_FILETYPE_DXE_CORE:
    break;
  case EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE:
    //
    // Rebase the inside FvImage.
    //
      GetChildFvFromFfs (BaseAddress, FfsFile, XipOffset);

    //
    // Search PE/TE section in FV sectin.
    //
    break;
  default:
    return EFI_SUCCESS;
  }

  FfsHeaderSize = GetFfsHeaderLength(FfsFile);
  //
  // Rebase each PE32 section
  //
  Status = EFI_SUCCESS;
  for (Index = 1;; Index++) {
    //
    // Init Value
    //
    NewPe32BaseAddress = 0;

    //
    // Find Pe Image
    //
    Status = GetSectionByType(FfsFile, EFI_SECTION_PE32, Index, &CurrentPe32Section);
    if (EFI_ERROR(Status)) {
      break;
    }
    CurSecHdrSize = GetSectionHeaderLength(CurrentPe32Section.CommonHeader);

    //
    // Initialize context
    //
    memset(&ImageContext, 0, sizeof (ImageContext));
    ImageContext.Handle = (VOID *)((UINTN)CurrentPe32Section.Pe32Section + CurSecHdrSize);
    ImageContext.ImageRead = (PE_COFF_LOADER_READ_FILE)FfsRebaseImageRead;
    Status = PeCoffLoaderGetImageInfo(&ImageContext);
    if (EFI_ERROR(Status)) {
      Error(NULL, 0, 3000, "Invalid PeImage", "The input file is %s and the return status is %x", FileName, (int)Status);
      return Status;
    }

    //if ((ImageContext.Machine == EFI_IMAGE_MACHINE_ARMT) ||
    //  (ImageContext.Machine == EFI_IMAGE_MACHINE_AARCH64)) {
    //  mArm = TRUE;
    //}

    //
    // Keep Image Context for PE image in FV
    //
    memcpy(&OrigImageContext, &ImageContext, sizeof (ImageContext));

    //
    // Get File PdbPointer
    //
    PdbPointer = PeCoffLoaderGetPdbPointer(ImageContext.Handle);
    if (PdbPointer == NULL) {
      PdbPointer = FileName;
    }

    //
    // Get PeHeader pointer
    //
    ImgHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)((UINTN)CurrentPe32Section.Pe32Section + CurSecHdrSize + ImageContext.PeCoffHeaderOffset);

    //
    // Calculate the PE32 base address, based on file type
    //
    switch (FfsFile->Type) {
    case EFI_FV_FILETYPE_SECURITY_CORE:
    case EFI_FV_FILETYPE_PEI_CORE:
    case EFI_FV_FILETYPE_PEIM:
    case EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER:
      //
      // Check if section-alignment and file-alignment match or not
      //
      if ((ImgHdr->Pe32.OptionalHeader.SectionAlignment != ImgHdr->Pe32.OptionalHeader.FileAlignment)) {
        //
        // Xip module has the same section alignment and file alignment.
        //
        Error(NULL, 0, 3000, "Invalid", "Section-Alignment and File-Alignment do not match : %s.", FileName);
        return EFI_ABORTED;
      }
      //
      // PeImage has no reloc section. It will try to get reloc data from the original EFI image.
      //
      if (ImageContext.RelocationsStripped) {
        //
        // Construct the original efi file Name
        //
        if (strlen (FileName) > MAX_LONG_FILE_PATH - 1) {
          Error(NULL, 0, 3000, "Invalid", "The file name for %s is too long.", FileName);
          return EFI_ABORTED;
        }
        strncpy(PeFileName, FileName, MAX_LONG_FILE_PATH - 1);
        PeFileName[MAX_LONG_FILE_PATH - 1] = 0;
        Cptr = PeFileName + strlen(PeFileName);
        while (*Cptr != '.') {
          Cptr--;
        }
        if (*Cptr != '.') {
          Error(NULL, 0, 3000, "Invalid", "The file %s has no .reloc section.", FileName);
          return EFI_ABORTED;
        }
        else {
          *(Cptr + 1) = 'e';
          *(Cptr + 2) = 'f';
          *(Cptr + 3) = 'i';
          *(Cptr + 4) = '\0';
        }
        LongFilePathName = LongFilePath(PeFileName);
        if (LongFilePathName == NULL) {
          Error(NULL, 0, 3000, "Invalid", "Fail to get long file path for file %s.", FileName);
          return EFI_ABORTED;
        }
        PeFile = fopen(LongFilePathName, "rb");
        if (PeFile == NULL) {
          Warning(NULL, 0, 0, "Invalid", "The file %s has no .reloc section.", FileName);
          //Error (NULL, 0, 3000, "Invalid", "The file %s has no .reloc section.", FileName);
          //return EFI_ABORTED;
          break;
        }
        //
        // Get the file size
        //
        PeFileSize = _filelength(fileno(PeFile));
        PeFileBuffer = (UINT8 *)malloc(PeFileSize);
        if (PeFileBuffer == NULL) {
          Error(NULL, 0, 4001, "Resource", "memory cannot be allocated on rebase of %s", FileName);
          fclose(PeFile);
          return EFI_OUT_OF_RESOURCES;
        }
        //
        // Read Pe File
        //
        fread(PeFileBuffer, sizeof (UINT8), PeFileSize, PeFile);
        //
        // close file
        //
        fclose(PeFile);
        //
        // Handle pointer to the original efi image.
        //
        ImageContext.Handle = PeFileBuffer;
        Status = PeCoffLoaderGetImageInfo(&ImageContext);
        if (EFI_ERROR(Status)) {
          Error(NULL, 0, 3000, "Invalid PeImage", "The input file is %s and the return status is %x", FileName, (int)Status);
          return Status;
        }
        ImageContext.RelocationsStripped = FALSE;
      }

      NewPe32BaseAddress = XipBase + (UINTN)CurrentPe32Section.Pe32Section + CurSecHdrSize - (UINTN)FfsFile;
      break;

    case EFI_FV_FILETYPE_DRIVER:
    case EFI_FV_FILETYPE_DXE_CORE:
      //
      // Check if section-alignment and file-alignment match or not
      //
      if ((ImgHdr->Pe32.OptionalHeader.SectionAlignment != ImgHdr->Pe32.OptionalHeader.FileAlignment)) {
        //
        // Xip module has the same section alignment and file alignment.
        //
        Error(NULL, 0, 3000, "Invalid", "Section-Alignment and File-Alignment do not match : %s.", FileName);
        return EFI_ABORTED;
      }
      NewPe32BaseAddress = XipBase + (UINTN)CurrentPe32Section.Pe32Section + CurSecHdrSize - (UINTN)FfsFile;
      break;

    default:
      //
      // Not supported file type
      //
      return EFI_SUCCESS;
    }

    //
    // Relocation doesn't exist
    //
    if (ImageContext.RelocationsStripped) {
      Warning(NULL, 0, 0, "Invalid", "The file %s has no .reloc section.", FileName);
      continue;
    }

    //
    // Relocation exist and rebase
    //
    //
    // Load and Relocate Image Data
    //
    MemoryImagePointer = (UINT8 *)malloc((UINTN)ImageContext.ImageSize + ImageContext.SectionAlignment);
    if (MemoryImagePointer == NULL) {
      Error(NULL, 0, 4001, "Resource", "memory cannot be allocated on rebase of %s", FileName);
      return EFI_OUT_OF_RESOURCES;
    }
    memset((VOID *)MemoryImagePointer, 0, (UINTN)ImageContext.ImageSize + ImageContext.SectionAlignment);
    ImageContext.ImageAddress = ((UINTN)MemoryImagePointer + ImageContext.SectionAlignment - 1) & (~((UINTN)ImageContext.SectionAlignment - 1));

    Status = PeCoffLoaderLoadImage(&ImageContext);
    if (EFI_ERROR(Status)) {
      Error(NULL, 0, 3000, "Invalid", "LocateImage() call failed on rebase of %s", FileName);
      free((VOID *)MemoryImagePointer);
      return Status;
    }

    ImageContext.DestinationAddress = NewPe32BaseAddress;
    Status = PeCoffLoaderRelocateImage(&ImageContext);
    if (EFI_ERROR(Status)) {
      Error(NULL, 0, 3000, "Invalid", "RelocateImage() call failed on rebase of %s", FileName);
      free((VOID *)MemoryImagePointer);
      return Status;
    }

    //
    // Copy Relocated data to raw image file.
    //
    SectionHeader = (EFI_IMAGE_SECTION_HEADER *)(
      (UINTN)ImgHdr +
      sizeof (UINT32)+
      sizeof (EFI_IMAGE_FILE_HEADER)+
      ImgHdr->Pe32.FileHeader.SizeOfOptionalHeader
      );

    for (Index = 0; Index < ImgHdr->Pe32.FileHeader.NumberOfSections; Index++, SectionHeader++) {
      CopyMem(
        (UINT8 *)CurrentPe32Section.Pe32Section + CurSecHdrSize + SectionHeader->PointerToRawData,
        (VOID*)(UINTN)(ImageContext.ImageAddress + SectionHeader->VirtualAddress),
        SectionHeader->SizeOfRawData
        );
    }

    free((VOID *)MemoryImagePointer);
    MemoryImagePointer = NULL;
    if (PeFileBuffer != NULL) {
      free(PeFileBuffer);
      PeFileBuffer = NULL;
    }

    //
    // Update Image Base Address
    //
    if (ImgHdr->Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      ImgHdr->Pe32.OptionalHeader.ImageBase = (UINT32)NewPe32BaseAddress;
    }
    else if (ImgHdr->Pe32Plus.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
      ImgHdr->Pe32Plus.OptionalHeader.ImageBase = NewPe32BaseAddress;
    }
    else {
      Error(NULL, 0, 3000, "Invalid", "unknown PE magic signature %X in PE32 image %s",
        ImgHdr->Pe32.OptionalHeader.Magic,
        FileName
        );
      return EFI_ABORTED;
    }

    //
    // Now update file checksum
    //
    if (FfsFile->Attributes & FFS_ATTRIB_CHECKSUM) {
      SavedState = FfsFile->State;
      FfsFile->IntegrityCheck.Checksum.File = 0;
      FfsFile->State = 0;
      FfsFile->IntegrityCheck.Checksum.File = CalculateChecksum8(
        (UINT8 *)((UINT8 *)FfsFile + FfsHeaderSize),
        GetFfsFileLength(FfsFile) - FfsHeaderSize
        );
      FfsFile->State = SavedState;
    }

  }

  if (FfsFile->Type != EFI_FV_FILETYPE_SECURITY_CORE &&
    FfsFile->Type != EFI_FV_FILETYPE_PEI_CORE &&
    FfsFile->Type != EFI_FV_FILETYPE_PEIM &&
    FfsFile->Type != EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER &&
    FfsFile->Type != EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE
    ) {
    //
    // Only Peim code may have a TE section
    //
    return EFI_SUCCESS;
  }

  //
  // Now process TE sections
  //
  for (Index = 1;; Index++) {
    NewPe32BaseAddress = 0;

    //
    // Find Te Image
    //
    Status = GetSectionByType(FfsFile, EFI_SECTION_TE, Index, &CurrentPe32Section);
    if (EFI_ERROR(Status)) {
      break;
    }

    CurSecHdrSize = GetSectionHeaderLength(CurrentPe32Section.CommonHeader);

    //
    // Calculate the TE base address, the FFS file base plus the offset of the TE section less the size stripped off
    // by GenTEImage
    //
    TEImageHeader = (EFI_TE_IMAGE_HEADER *)((UINT8 *)CurrentPe32Section.Pe32Section + CurSecHdrSize);

    //
    // Initialize context, load image info.
    //
    memset(&ImageContext, 0, sizeof (ImageContext));
    ImageContext.Handle = (VOID *)TEImageHeader;
    ImageContext.ImageRead = (PE_COFF_LOADER_READ_FILE)FfsRebaseImageRead;
    Status = PeCoffLoaderGetImageInfo(&ImageContext);
    if (EFI_ERROR(Status)) {
      Error(NULL, 0, 3000, "Invalid TeImage", "The input file is %s and the return status is %x", FileName, (int)Status);
      return Status;
    }

    //if ((ImageContext.Machine == EFI_IMAGE_MACHINE_ARMT) ||
    //  (ImageContext.Machine == EFI_IMAGE_MACHINE_AARCH64)) {
    //  mArm = TRUE;
    //}

    //
    // Keep Image Context for TE image in FV
    //
    memcpy(&OrigImageContext, &ImageContext, sizeof (ImageContext));

    //
    // Get File PdbPointer
    //
    PdbPointer = PeCoffLoaderGetPdbPointer(ImageContext.Handle);
    if (PdbPointer == NULL) {
      PdbPointer = FileName;
    }
    //
    // Set new rebased address.
    //
    NewPe32BaseAddress = XipBase + (UINTN)TEImageHeader + sizeof (EFI_TE_IMAGE_HEADER) \
      - TEImageHeader->StrippedSize - (UINTN)FfsFile;

    //
    // if reloc is stripped, try to get the original efi image to get reloc info.
    //
    if (ImageContext.RelocationsStripped) {
      //
      // Construct the original efi file name
      //
      if (strlen (FileName) > MAX_LONG_FILE_PATH - 1) {
        Error(NULL, 0, 3000, "Invalid", "The file name for %s is too long.", FileName);
        return EFI_ABORTED;
      }
      strncpy(PeFileName, FileName, MAX_LONG_FILE_PATH - 1);
      PeFileName[MAX_LONG_FILE_PATH - 1] = 0;
      Cptr = PeFileName + strlen(PeFileName);
      while (*Cptr != '.') {
        Cptr--;
      }

      if (*Cptr != '.') {
        Error(NULL, 0, 3000, "Invalid", "The file %s has no .reloc section.", FileName);
        return EFI_ABORTED;
      }
      else {
        *(Cptr + 1) = 'e';
        *(Cptr + 2) = 'f';
        *(Cptr + 3) = 'i';
        *(Cptr + 4) = '\0';
      }

      LongFilePathName = LongFilePath(PeFileName);
      if (LongFilePathName == NULL) {
        Error(NULL, 0, 3000, "Invalid", "Fail to get long file path for file %s.", FileName);
        return EFI_ABORTED;
      }
      PeFile = fopen(LongFilePathName, "rb");
      if (PeFile == NULL) {
        Warning(NULL, 0, 0, "Invalid", "The file %s has no .reloc section.", FileName);
        //Error (NULL, 0, 3000, "Invalid", "The file %s has no .reloc section.", FileName);
        //return EFI_ABORTED;
      }
      else {
        //
        // Get the file size
        //
        PeFileSize = _filelength(fileno(PeFile));
        PeFileBuffer = (UINT8 *)malloc(PeFileSize);
        if (PeFileBuffer == NULL) {
          Error(NULL, 0, 4001, "Resource", "memory cannot be allocated on rebase of %s", FileName);
          fclose(PeFile);
          return EFI_OUT_OF_RESOURCES;
        }
        //
        // Read Pe File
        //
        fread(PeFileBuffer, sizeof (UINT8), PeFileSize, PeFile);
        //
        // close file
        //
        fclose(PeFile);
        //
        // Append reloc section into TeImage
        //
        ImageContext.Handle = PeFileBuffer;
        Status = PeCoffLoaderGetImageInfo(&ImageContext);
        if (EFI_ERROR(Status)) {
          Error(NULL, 0, 3000, "Invalid TeImage", "The input file is %s and the return status is %x", FileName, (int)Status);
          return Status;
        }
        ImageContext.RelocationsStripped = FALSE;
      }
    }
    //
    // Relocation doesn't exist
    //
    if (ImageContext.RelocationsStripped) {
      Warning(NULL, 0, 0, "Invalid", "The file %s has no .reloc section.", FileName);
      continue;
    }

    //
    // Relocation exist and rebase
    //
    //
    // Load and Relocate Image Data
    //
    MemoryImagePointer = (UINT8 *)malloc((UINTN)ImageContext.ImageSize + ImageContext.SectionAlignment);
    if (MemoryImagePointer == NULL) {
      Error(NULL, 0, 4001, "Resource", "memory cannot be allocated on rebase of %s", FileName);
      return EFI_OUT_OF_RESOURCES;
    }
    memset((VOID *)MemoryImagePointer, 0, (UINTN)ImageContext.ImageSize + ImageContext.SectionAlignment);
    ImageContext.ImageAddress = ((UINTN)MemoryImagePointer + ImageContext.SectionAlignment - 1) & (~((UINTN)ImageContext.SectionAlignment - 1));

    Status = PeCoffLoaderLoadImage(&ImageContext);
    if (EFI_ERROR(Status)) {
      Error(NULL, 0, 3000, "Invalid", "LocateImage() call failed on rebase of %s", FileName);
      free((VOID *)MemoryImagePointer);
      return Status;
    }
    //
    // Reloacate TeImage
    //
    ImageContext.DestinationAddress = NewPe32BaseAddress;
    Status = PeCoffLoaderRelocateImage(&ImageContext);
    if (EFI_ERROR(Status)) {
      Error(NULL, 0, 3000, "Invalid", "RelocateImage() call failed on rebase of TE image %s", FileName);
      free((VOID *)MemoryImagePointer);
      return Status;
    }

    //
    // Copy the relocated image into raw image file.
    //
    SectionHeader = (EFI_IMAGE_SECTION_HEADER *)(TEImageHeader + 1);
    for (Index = 0; Index < TEImageHeader->NumberOfSections; Index++, SectionHeader++) {
      if (!ImageContext.IsTeImage) {
        CopyMem(
          (UINT8 *)TEImageHeader + sizeof (EFI_TE_IMAGE_HEADER)-TEImageHeader->StrippedSize + SectionHeader->PointerToRawData,
          (VOID*)(UINTN)(ImageContext.ImageAddress + SectionHeader->VirtualAddress),
          SectionHeader->SizeOfRawData
          );
      }
      else {
        CopyMem(
          (UINT8 *)TEImageHeader + sizeof (EFI_TE_IMAGE_HEADER)-TEImageHeader->StrippedSize + SectionHeader->PointerToRawData,
          (VOID*)(UINTN)(ImageContext.ImageAddress + sizeof (EFI_TE_IMAGE_HEADER)-TEImageHeader->StrippedSize + SectionHeader->VirtualAddress),
          SectionHeader->SizeOfRawData
          );
      }
    }

    //
    // Free the allocated memory resource
    //
    free((VOID *)MemoryImagePointer);
    MemoryImagePointer = NULL;
    if (PeFileBuffer != NULL) {
      free(PeFileBuffer);
      PeFileBuffer = NULL;
    }

    //
    // Update Image Base Address
    //
    TEImageHeader->ImageBase = NewPe32BaseAddress;

    //
    // Now update file checksum
    //
    if (FfsFile->Attributes & FFS_ATTRIB_CHECKSUM) {
      SavedState = FfsFile->State;
      FfsFile->IntegrityCheck.Checksum.File = 0;
      FfsFile->State = 0;
      FfsFile->IntegrityCheck.Checksum.File = CalculateChecksum8(
        (UINT8 *)((UINT8 *)FfsFile + FfsHeaderSize),
        GetFfsFileLength(FfsFile) - FfsHeaderSize
        );
      FfsFile->State = SavedState;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
FfsRebaseImageRead(
IN     VOID    *FileHandle,
IN     UINTN   FileOffset,
IN OUT UINT32  *ReadSize,
OUT    VOID    *Buffer
)
/*++

Routine Description:

Support routine for the PE/COFF Loader that reads a buffer from a PE/COFF file

Arguments:

FileHandle - The handle to the PE/COFF file

FileOffset - The offset, in bytes, into the file to read

ReadSize   - The number of bytes to read from the file starting at FileOffset

Buffer     - A pointer to the buffer to read the data into.

Returns:

EFI_SUCCESS - ReadSize bytes of data were read into Buffer from the PE/COFF file starting at FileOffset

--*/
{
  CHAR8   *Destination8;
  CHAR8   *Source8;
  UINT32  Length;

  Destination8 = Buffer;
  Source8 = (CHAR8 *)((UINTN)FileHandle + FileOffset);
  Length = *ReadSize;
  while (Length--) {
    *(Destination8++) = *(Source8++);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GetChildFvFromFfs (
  IN      UINT64                 BaseAddress,
  IN      EFI_FFS_FILE_HEADER   *FfsFile,
  IN      UINTN                 XipOffset
  )
/*++

Routine Description:

  This function gets all child FvImages in the input FfsFile, and records
  their base address to the parent image.

Arguments:
  FvInfo            A pointer to FV_INFO struture.
  FfsFile           A pointer to Ffs file image that may contain FvImage.
  XipOffset         The offset address to the parent FvImage base.

Returns:

  EFI_SUCCESS        Base address of child Fv image is recorded.
--*/
{
  EFI_STATUS                          Status;
  UINTN                               Index;
  EFI_FILE_SECTION_POINTER            SubFvSection;
  EFI_FIRMWARE_VOLUME_HEADER          *SubFvImageHeader;
  EFI_PHYSICAL_ADDRESS                SubFvBaseAddress;
  EFI_FIRMWARE_VOLUME_HEADER          *OrigFvHeader;
  UINT32                              OrigFvLength;
  EFI_PHYSICAL_ADDRESS                OrigFvBaseAddress;
  EFI_FFS_FILE_HEADER                 *CurrentFile;

  //
  // Initialize FV library, saving previous values
  //
  OrigFvHeader = NULL;
  GetFvHeader (&OrigFvHeader, &OrigFvLength);
  OrigFvBaseAddress = BaseAddress;
  for (Index = 1;; Index++) {
    //
    // Find FV section
    //
    Status = GetSectionByType (FfsFile, EFI_SECTION_FIRMWARE_VOLUME_IMAGE, Index, &SubFvSection);
    if (EFI_ERROR (Status)) {
      break;
    }
    SubFvImageHeader = (EFI_FIRMWARE_VOLUME_HEADER *) ((UINT8 *) SubFvSection.FVImageSection + GetSectionHeaderLength(SubFvSection.FVImageSection));

    //
    // Rebase on Flash
    //
    SubFvBaseAddress = OrigFvBaseAddress + (UINTN) SubFvImageHeader - (UINTN) FfsFile + XipOffset;
    //mFvBaseAddress[mFvBaseAddressNumber ++ ] = SubFvBaseAddress;
    BaseAddress = SubFvBaseAddress;
    InitializeFvLib(SubFvImageHeader, (UINT32) SubFvImageHeader->FvLength);

    Status = GetNextFile (NULL, &CurrentFile);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 0003, "error parsing FV image", "FFS file can't be found");
      continue;
    }
    while (CurrentFile) {
      RebaseFfs (BaseAddress, "", CurrentFile, (UINTN) CurrentFile - (UINTN) SubFvImageHeader);
      Status = GetNextFile (CurrentFile, &CurrentFile);
      if (EFI_ERROR (Status)) {
        break;
      }
    }
  }

  BaseAddress = OrigFvBaseAddress;
  if (OrigFvHeader != NULL) {
    InitializeFvLib(OrigFvHeader, OrigFvLength);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GetPe32Info (
  IN UINT8                  *Pe32,
  OUT UINT32                *EntryPoint,
  OUT UINT32                *BaseOfCode,
  OUT UINT16                *MachineType
  )
/*++

Routine Description:

  Retrieves the PE32 entry point offset and machine type from PE image or TeImage.
  See EfiImage.h for machine types.  The entry point offset is from the beginning
  of the PE32 buffer passed in.

Arguments:

  Pe32          Beginning of the PE32.
  EntryPoint    Offset from the beginning of the PE32 to the image entry point.
  BaseOfCode    Base address of code.
  MachineType   Magic number for the machine type.

Returns:

  EFI_SUCCESS             Function completed successfully.
  EFI_ABORTED             Error encountered.
  EFI_INVALID_PARAMETER   A required parameter was NULL.
  EFI_UNSUPPORTED         The operation is unsupported.

--*/
{
  EFI_IMAGE_DOS_HEADER             *DosHeader;
  EFI_IMAGE_OPTIONAL_HEADER_UNION  *ImgHdr;
  EFI_TE_IMAGE_HEADER              *TeHeader;

  //
  // Verify input parameters
  //
  if (Pe32 == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // First check whether it is one TE Image.
  //
  TeHeader = (EFI_TE_IMAGE_HEADER *) Pe32;
  if (TeHeader->Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
    //
    // By TeImage Header to get output
    //
    *EntryPoint   = TeHeader->AddressOfEntryPoint + sizeof (EFI_TE_IMAGE_HEADER) - TeHeader->StrippedSize;
    *BaseOfCode   = TeHeader->BaseOfCode + sizeof (EFI_TE_IMAGE_HEADER) - TeHeader->StrippedSize;
    *MachineType  = TeHeader->Machine;
  } else {

    //
    // Then check whether
    // First is the DOS header
    //
    DosHeader = (EFI_IMAGE_DOS_HEADER *) Pe32;

    //
    // Verify DOS header is expected
    //
    if (DosHeader->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
      Error (NULL, 0, 3000, "Invalid", "Unknown magic number in the DOS header, 0x%04X.", DosHeader->e_magic);
      return EFI_UNSUPPORTED;
    }
    //
    // Immediately following is the NT header.
    //
    ImgHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *) ((UINTN) Pe32 + DosHeader->e_lfanew);

    //
    // Verify NT header is expected
    //
    if (ImgHdr->Pe32.Signature != EFI_IMAGE_NT_SIGNATURE) {
      Error (NULL, 0, 3000, "Invalid", "Unrecognized image signature 0x%08X.", (unsigned) ImgHdr->Pe32.Signature);
      return EFI_UNSUPPORTED;
    }
    //
    // Get output
    //
    *EntryPoint   = ImgHdr->Pe32.OptionalHeader.AddressOfEntryPoint;
    *BaseOfCode   = ImgHdr->Pe32.OptionalHeader.BaseOfCode;
    *MachineType  = ImgHdr->Pe32.FileHeader.Machine;
  }

  //
  // Verify machine type is supported
  //
  if ((*MachineType != EFI_IMAGE_MACHINE_IA32) && (*MachineType != EFI_IMAGE_MACHINE_X64) && (*MachineType != EFI_IMAGE_MACHINE_EBC) &&
      (*MachineType != EFI_IMAGE_MACHINE_ARMT) && (*MachineType != EFI_IMAGE_MACHINE_AARCH64)) {
    Error (NULL, 0, 3000, "Invalid", "Unrecognized machine type in the PE32 file.");
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

