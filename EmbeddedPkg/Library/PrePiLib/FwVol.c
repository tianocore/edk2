/** @file
  Implementation of the 6 PEI Ffs (FV) APIs in library form.

  This code only knows about a FV if it has a EFI_HOB_TYPE_FV entry in the HOB list

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PrePi.h>
#include <Library/ExtractGuidedSectionLib.h>

#define GET_OCCUPIED_SIZE(ActualSize, Alignment) \
  (ActualSize) + (((Alignment) - ((ActualSize) & ((Alignment) - 1))) & ((Alignment) - 1))

/**
  Returns the highest bit set of the State field

  @param ErasePolarity   Erase Polarity  as defined by EFI_FVB2_ERASE_POLARITY
                         in the Attributes field.
  @param FfsHeader       Pointer to FFS File Header


  @retval the highest bit in the State field

**/
STATIC
EFI_FFS_FILE_STATE
GetFileState (
  IN UINT8                ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
{
  EFI_FFS_FILE_STATE  FileState;
  EFI_FFS_FILE_STATE  HighestBit;

  FileState = FfsHeader->State;

  if (ErasePolarity != 0) {
    FileState = (EFI_FFS_FILE_STATE) ~FileState;
  }

  HighestBit = 0x80;
  while (HighestBit != 0 && (HighestBit & FileState) == 0) {
    HighestBit >>= 1;
  }

  return HighestBit;
}

/**
  Calculates the checksum of the header of a file.
  The header is a zero byte checksum, so zero means header is good

  @param FfsHeader       Pointer to FFS File Header

  @retval Checksum of the header

**/
STATIC
UINT8
CalculateHeaderChecksum (
  IN EFI_FFS_FILE_HEADER  *FileHeader
  )
{
  UINT8  *Ptr;
  UINTN  Index;
  UINT8  Sum;

  Sum = 0;
  Ptr = (UINT8 *)FileHeader;

  for (Index = 0; Index < sizeof (EFI_FFS_FILE_HEADER) - 3; Index += 4) {
    Sum = (UINT8)(Sum + Ptr[Index]);
    Sum = (UINT8)(Sum + Ptr[Index+1]);
    Sum = (UINT8)(Sum + Ptr[Index+2]);
    Sum = (UINT8)(Sum + Ptr[Index+3]);
  }

  for ( ; Index < sizeof (EFI_FFS_FILE_HEADER); Index++) {
    Sum = (UINT8)(Sum + Ptr[Index]);
  }

  //
  // State field (since this indicates the different state of file).
  //
  Sum = (UINT8)(Sum - FileHeader->State);
  //
  // Checksum field of the file is not part of the header checksum.
  //
  Sum = (UINT8)(Sum - FileHeader->IntegrityCheck.Checksum.File);

  return Sum;
}

/**
  Given a FileHandle return the VolumeHandle

  @param FileHandle   File handle to look up
  @param VolumeHandle Match for FileHandle

  @retval TRUE  VolumeHandle is valid

**/
STATIC
BOOLEAN
EFIAPI
FileHandleToVolume (
  IN   EFI_PEI_FILE_HANDLE  FileHandle,
  OUT  EFI_PEI_FV_HANDLE    *VolumeHandle
  )
{
  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader;
  EFI_PEI_HOB_POINTERS        Hob;

  Hob.Raw = GetHobList ();
  if (Hob.Raw == NULL) {
    return FALSE;
  }

  do {
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_FV, Hob.Raw);
    if (Hob.Raw != NULL) {
      FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)(Hob.FirmwareVolume->BaseAddress);
      if (((UINT64)(UINTN)FileHandle > (UINT64)(UINTN)FwVolHeader) &&   \
          ((UINT64)(UINTN)FileHandle <= ((UINT64)(UINTN)FwVolHeader + FwVolHeader->FvLength - 1)))
      {
        *VolumeHandle = (EFI_PEI_FV_HANDLE)FwVolHeader;
        return TRUE;
      }

      Hob.Raw = GetNextHob (EFI_HOB_TYPE_FV, GET_NEXT_HOB (Hob));
    }
  } while (Hob.Raw != NULL);

  return FALSE;
}

/**
  Given the input file pointer, search for the next matching file in the
  FFS volume as defined by SearchType. The search starts from FileHeader inside
  the Firmware Volume defined by FwVolHeader.

  @param FileHandle   File handle to look up
  @param VolumeHandle Match for FileHandle


**/
EFI_STATUS
FindFileEx (
  IN  CONST EFI_PEI_FV_HANDLE    FvHandle,
  IN  CONST EFI_GUID             *FileName    OPTIONAL,
  IN        EFI_FV_FILETYPE      SearchType,
  IN OUT    EFI_PEI_FILE_HANDLE  *FileHandle
  )
{
  EFI_FIRMWARE_VOLUME_HEADER      *FwVolHeader;
  EFI_FFS_FILE_HEADER             **FileHeader;
  EFI_FFS_FILE_HEADER             *FfsFileHeader;
  EFI_FIRMWARE_VOLUME_EXT_HEADER  *FwVolExHeaderInfo;
  UINT32                          FileLength;
  UINT32                          FileOccupiedSize;
  UINT32                          FileOffset;
  UINT64                          FvLength;
  UINT8                           ErasePolarity;
  UINT8                           FileState;

  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)FvHandle;
  FileHeader  = (EFI_FFS_FILE_HEADER **)FileHandle;

  FvLength = FwVolHeader->FvLength;
  if (FwVolHeader->Attributes & EFI_FVB2_ERASE_POLARITY) {
    ErasePolarity = 1;
  } else {
    ErasePolarity = 0;
  }

  //
  // If FileHeader is not specified (NULL) or FileName is not NULL,
  // start with the first file in the firmware volume.  Otherwise,
  // start from the FileHeader.
  //
  if ((*FileHeader == NULL) || (FileName != NULL)) {
    FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FwVolHeader + FwVolHeader->HeaderLength);
    if (FwVolHeader->ExtHeaderOffset != 0) {
      FwVolExHeaderInfo = (EFI_FIRMWARE_VOLUME_EXT_HEADER *)(((UINT8 *)FwVolHeader) + FwVolHeader->ExtHeaderOffset);
      FfsFileHeader     = (EFI_FFS_FILE_HEADER *)(((UINT8 *)FwVolExHeaderInfo) + FwVolExHeaderInfo->ExtHeaderSize);
    }
  } else {
    //
    // Length is 24 bits wide so mask upper 8 bits
    // FileLength is adjusted to FileOccupiedSize as it is 8 byte aligned.
    //
    FileLength       = *(UINT32 *)(*FileHeader)->Size & 0x00FFFFFF;
    FileOccupiedSize = GET_OCCUPIED_SIZE (FileLength, 8);
    FfsFileHeader    = (EFI_FFS_FILE_HEADER *)((UINT8 *)*FileHeader + FileOccupiedSize);
  }

  // FFS files begin with a header that is aligned on an 8-byte boundary
  FfsFileHeader = ALIGN_POINTER (FfsFileHeader, 8);

  FileOffset = (UINT32)((UINT8 *)FfsFileHeader - (UINT8 *)FwVolHeader);
  ASSERT (FileOffset <= 0xFFFFFFFF);

  while (FileOffset < (FvLength - sizeof (EFI_FFS_FILE_HEADER))) {
    //
    // Get FileState which is the highest bit of the State
    //
    FileState = GetFileState (ErasePolarity, FfsFileHeader);

    switch (FileState) {
      case EFI_FILE_HEADER_INVALID:
        FileOffset   += sizeof (EFI_FFS_FILE_HEADER);
        FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsFileHeader + sizeof (EFI_FFS_FILE_HEADER));
        break;

      case EFI_FILE_DATA_VALID:
      case EFI_FILE_MARKED_FOR_UPDATE:
        if (CalculateHeaderChecksum (FfsFileHeader) != 0) {
          ASSERT (FALSE);
          *FileHeader = NULL;
          return EFI_NOT_FOUND;
        }

        FileLength       = *(UINT32 *)(FfsFileHeader->Size) & 0x00FFFFFF;
        FileOccupiedSize = GET_OCCUPIED_SIZE (FileLength, 8);

        if (FileName != NULL) {
          if (CompareGuid (&FfsFileHeader->Name, (EFI_GUID *)FileName)) {
            *FileHeader = FfsFileHeader;
            return EFI_SUCCESS;
          }
        } else if (((SearchType == FfsFileHeader->Type) || (SearchType == EFI_FV_FILETYPE_ALL)) &&
                   (FfsFileHeader->Type != EFI_FV_FILETYPE_FFS_PAD))
        {
          *FileHeader = FfsFileHeader;
          return EFI_SUCCESS;
        }

        FileOffset   += FileOccupiedSize;
        FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsFileHeader + FileOccupiedSize);
        break;

      case EFI_FILE_DELETED:
        FileLength       = *(UINT32 *)(FfsFileHeader->Size) & 0x00FFFFFF;
        FileOccupiedSize = GET_OCCUPIED_SIZE (FileLength, 8);
        FileOffset      += FileOccupiedSize;
        FfsFileHeader    = (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsFileHeader + FileOccupiedSize);
        break;

      default:
        *FileHeader = NULL;
        return EFI_NOT_FOUND;
    }
  }

  *FileHeader = NULL;
  return EFI_NOT_FOUND;
}

/**
  Return the size of a section whether SECTION or SECTION2

  @param  Section           - Pointer to start of section header

  @retval Size in bytes.
 **/
STATIC
UINT32
GetSectionNSize (
  IN EFI_COMMON_SECTION_HEADER  *Section
  )
{
  UINT32  SectionSize;

  if (IS_SECTION2 (Section)) {
    ASSERT (SECTION2_SIZE (Section) > 0x00FFFFFF);
    SectionSize = SECTION2_SIZE (Section);
  } else {
    SectionSize = SECTION_SIZE (Section);
  }

  return SectionSize;
}

/**
  Return the size of a section header whether SECTION or SECTION2

  @param  Section           - Pointer to start of section header

  @retval Size in bytes.
 **/
STATIC
UINT32
GetCommonSectionNHeaderSize (
  IN EFI_COMMON_SECTION_HEADER  *Section
  )
{
  if (IS_SECTION2 (Section)) {
    return sizeof (EFI_COMMON_SECTION_HEADER2);
  } else {
    return sizeof (EFI_COMMON_SECTION_HEADER);
  }
}

/**
  Return the size of a compression section header whether SECTION or SECTION2

  @param  Section           - Pointer to start of section header

  @retval Size in bytes.
 **/
STATIC
UINT32
GetCompressionSectionNHeaderSize (
  IN EFI_COMMON_SECTION_HEADER  *Section
  )
{
  ASSERT (Section->Type == EFI_SECTION_COMPRESSION);

  if (IS_SECTION2 (Section)) {
    return sizeof (EFI_COMPRESSION_SECTION2);
  } else {
    return sizeof (EFI_COMPRESSION_SECTION);
  }
}

/**
  Return the compression type of a section whether SECTION or SECTION2

  @param  Section           - Pointer to start of section header

  @retval EFI_SECTION_TYPE
 **/
STATIC
EFI_SECTION_TYPE
GetSectionNCompressionType (
  IN EFI_COMMON_SECTION_HEADER  *Section
  )
{
  ASSERT (Section->Type == EFI_SECTION_COMPRESSION);

  if (IS_SECTION2 (Section)) {
    return ((EFI_COMPRESSION_SECTION2 *)Section)->CompressionType;
  } else {
    return ((EFI_COMPRESSION_SECTION *)Section)->CompressionType;
  }
}

/**
  Go through the file to search SectionType section,
  when meeting an encapsuled section.

  @param  SectionType       - Filter to find only section of this type.
  @param  SectionCheckHook  - A hook which can check if the section is the target one.
  @param  Section           - From where to search.
  @param  SectionSize       - The file size to search.
  @param  OutputBuffer      - Pointer to the section to search.

  @retval EFI_SUCCESS
**/
EFI_STATUS
FfsProcessSection (
  IN EFI_SECTION_TYPE           SectionType,
  IN FFS_CHECK_SECTION_HOOK     SectionCheckHook,
  IN EFI_COMMON_SECTION_HEADER  *Section,
  IN UINTN                      SectionSize,
  OUT VOID                      **OutputBuffer
  )
{
  EFI_STATUS  Status;
  UINT32      SectionLength;
  UINTN       ParsedLength;
  UINT32      DstBufferSize;
  VOID        *DstBuffer;

  ParsedLength = 0;

  while (ParsedLength < SectionSize) {
    UINT32  SectionHeaderSize;

    SectionHeaderSize = GetCommonSectionNHeaderSize (Section);

    if (Section->Type == SectionType) {
      if (SectionCheckHook != NULL) {
        if (SectionCheckHook (Section) != EFI_SUCCESS) {
          goto CheckNextSection;
        }
      }

      *OutputBuffer = (VOID *)((UINT8 *)Section + SectionHeaderSize);

      return EFI_SUCCESS;
    } else if ((Section->Type == EFI_SECTION_COMPRESSION) || (Section->Type == EFI_SECTION_GUID_DEFINED)) {
      CHAR8   *CompressedData;
      UINT32  CompressionSectionHeaderSize;
      VOID    *ScratchBuffer;
      UINT32  ScratchBufferSize;

      if (Section->Type == EFI_SECTION_COMPRESSION) {
        UINT32  CompressedDataLength;

        CompressionSectionHeaderSize = GetCompressionSectionNHeaderSize (Section);
        SectionLength                = GetSectionNSize (Section);

        CompressedData       = (CHAR8 *)((UINTN)(Section) + CompressionSectionHeaderSize);
        CompressedDataLength = SectionLength - CompressionSectionHeaderSize;

        if (GetSectionNCompressionType (Section) != EFI_STANDARD_COMPRESSION) {
          return EFI_UNSUPPORTED;
        }

        Status = UefiDecompressGetInfo (
                   CompressedData,
                   CompressedDataLength,
                   &DstBufferSize,
                   &ScratchBufferSize
                   );
      } else {
        // Section->Type == EFI_SECTION_GUID_DEFINED)
        UINT16  SectionAttribute;

        Status = ExtractGuidedSectionGetInfo (
                   Section,
                   &DstBufferSize,
                   &ScratchBufferSize,
                   &SectionAttribute
                   );
      }

      if (EFI_ERROR (Status)) {
        //
        // GetInfo failed
        //
        DEBUG ((DEBUG_ERROR, "Decompress GetInfo Failed - %r\n", Status));
        return EFI_NOT_FOUND;
      }

      //
      // Allocate scratch buffer
      //
      ScratchBuffer = (VOID *)(UINTN)AllocatePages (EFI_SIZE_TO_PAGES (ScratchBufferSize));
      if (ScratchBuffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      //
      // Allocate destination buffer, extra one page for adjustment
      //
      DstBuffer = (VOID *)(UINTN)AllocatePages (EFI_SIZE_TO_PAGES (DstBufferSize) + 1);
      if (DstBuffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      //
      // DstBuffer still is one section. Adjust DstBuffer offset, skip EFI section header
      // to make section data at page alignment.
      //
      DstBuffer = (UINT8 *)DstBuffer + EFI_PAGE_SIZE - SectionHeaderSize;

      //
      // Call decompress function
      //
      if (Section->Type == EFI_SECTION_COMPRESSION) {
        CompressedData = (CHAR8 *)((UINTN)(Section) + CompressionSectionHeaderSize);

        Status = UefiDecompress (
                   CompressedData,
                   DstBuffer,
                   ScratchBuffer
                   );
      } else if (Section->Type == EFI_SECTION_GUID_DEFINED) {
        UINT32  AuthenticationStatus;

        Status = ExtractGuidedSectionDecode (
                   Section,
                   &DstBuffer,
                   ScratchBuffer,
                   &AuthenticationStatus
                   );
      }

      if (EFI_ERROR (Status)) {
        //
        // Decompress failed
        //
        DEBUG ((DEBUG_ERROR, "Decompress Failed - %r\n", Status));
        return EFI_NOT_FOUND;
      }

      return FfsProcessSection (
               SectionType,
               SectionCheckHook,
               DstBuffer,
               DstBufferSize,
               OutputBuffer
               );
    }

CheckNextSection:
    SectionLength = GetSectionNSize (Section);

    //
    // SectionLength is adjusted it is 4 byte aligned.
    // Go to the next section
    //
    SectionLength = GET_OCCUPIED_SIZE (SectionLength, 4);
    ASSERT (SectionLength != 0);
    ParsedLength += SectionLength;
    Section       = (EFI_COMMON_SECTION_HEADER *)((UINT8 *)Section + SectionLength);
  }

  return EFI_NOT_FOUND;
}

/**
  This service enables discovery sections of a given type within a valid FFS file.
  Caller also can provide a SectionCheckHook to do additional checking.

  @param  SectionType           The value of the section type to find.
  @param  SectionCheckHook      A hook which can check if the section is the target one.
  @param  FileHandle            A pointer to the file header that contains the set of sections to
                                be searched.
  @param  SectionData           A pointer to the discovered section, if successful.

  @retval EFI_SUCCESS           The section was found.
  @retval EFI_NOT_FOUND         The section was not found.

**/
EFI_STATUS
EFIAPI
FfsFindSectionDataWithHook (
  IN EFI_SECTION_TYPE        SectionType,
  IN FFS_CHECK_SECTION_HOOK  SectionCheckHook,
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  OUT VOID                   **SectionData
  )
{
  EFI_FFS_FILE_HEADER        *FfsFileHeader;
  UINT32                     FileSize;
  EFI_COMMON_SECTION_HEADER  *Section;

  FfsFileHeader = (EFI_FFS_FILE_HEADER *)(FileHandle);

  //
  // Size is 24 bits wide so mask upper 8 bits.
  // Does not include FfsFileHeader header size
  // FileSize is adjusted to FileOccupiedSize as it is 8 byte aligned.
  //
  Section   = (EFI_COMMON_SECTION_HEADER *)(FfsFileHeader + 1);
  FileSize  = *(UINT32 *)(FfsFileHeader->Size) & 0x00FFFFFF;
  FileSize -= sizeof (EFI_FFS_FILE_HEADER);

  return FfsProcessSection (
           SectionType,
           SectionCheckHook,
           Section,
           FileSize,
           SectionData
           );
}

/**
  This service enables discovery sections of a given type within a valid FFS file.

  @param  SectionType           The value of the section type to find.
  @param  FileHandle            A pointer to the file header that contains the set of sections to
                                be searched.
  @param  SectionData           A pointer to the discovered section, if successful.

  @retval EFI_SUCCESS           The section was found.
  @retval EFI_NOT_FOUND         The section was not found.

**/
EFI_STATUS
EFIAPI
FfsFindSectionData (
  IN EFI_SECTION_TYPE     SectionType,
  IN EFI_PEI_FILE_HANDLE  FileHandle,
  OUT VOID                **SectionData
  )
{
  return FfsFindSectionDataWithHook (SectionType, NULL, FileHandle, SectionData);
}

/**
  This service enables discovery of additional firmware files.

  @param  SearchType            A filter to find files only of this type.
  @param  FwVolHeader           Pointer to the firmware volume header of the volume to search.
                                This parameter must point to a valid FFS volume.
  @param  FileHeader            Pointer to the current file from which to begin searching.

  @retval EFI_SUCCESS           The file was found.
  @retval EFI_NOT_FOUND         The file was not found.
  @retval EFI_NOT_FOUND         The header checksum was not zero.

**/
EFI_STATUS
EFIAPI
FfsFindNextFile (
  IN UINT8                    SearchType,
  IN EFI_PEI_FV_HANDLE        VolumeHandle,
  IN OUT EFI_PEI_FILE_HANDLE  *FileHandle
  )
{
  return FindFileEx (VolumeHandle, NULL, SearchType, FileHandle);
}

/**
  This service enables discovery of additional firmware volumes.

  @param  Instance              This instance of the firmware volume to find.  The value 0 is the
                                Boot Firmware Volume (BFV).
  @param  FwVolHeader           Pointer to the firmware volume header of the volume to return.

  @retval EFI_SUCCESS           The volume was found.
  @retval EFI_NOT_FOUND         The volume was not found.

**/
EFI_STATUS
EFIAPI
FfsFindNextVolume (
  IN UINTN                  Instance,
  IN OUT EFI_PEI_FV_HANDLE  *VolumeHandle
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = GetHobList ();
  if (Hob.Raw == NULL) {
    return EFI_NOT_FOUND;
  }

  do {
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_FV, Hob.Raw);
    if (Hob.Raw != NULL) {
      if (Instance-- == 0) {
        *VolumeHandle = (EFI_PEI_FV_HANDLE)(UINTN)(Hob.FirmwareVolume->BaseAddress);
        return EFI_SUCCESS;
      }

      Hob.Raw = GetNextHob (EFI_HOB_TYPE_FV, GET_NEXT_HOB (Hob));
    }
  } while (Hob.Raw != NULL);

  return EFI_NOT_FOUND;
}

/**
  Find a file in the volume by name

  @param FileName       A pointer to the name of the file to
                        find within the firmware volume.

  @param VolumeHandle   The firmware volume to search FileHandle
                        Upon exit, points to the found file's
                        handle or NULL if it could not be found.

  @retval EFI_SUCCESS             File was found.

  @retval EFI_NOT_FOUND           File was not found.

  @retval EFI_INVALID_PARAMETER   VolumeHandle or FileHandle or
                                  FileName was NULL.

**/
EFI_STATUS
EFIAPI
FfsFindFileByName (
  IN  CONST EFI_GUID       *FileName,
  IN  EFI_PEI_FV_HANDLE    VolumeHandle,
  OUT EFI_PEI_FILE_HANDLE  *FileHandle
  )
{
  EFI_STATUS  Status;

  if ((VolumeHandle == NULL) || (FileName == NULL) || (FileHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FindFileEx (VolumeHandle, FileName, 0, FileHandle);
  if (Status == EFI_NOT_FOUND) {
    *FileHandle = NULL;
  }

  return Status;
}

/**
  Get information about the file by name.

  @param FileHandle   Handle of the file.

  @param FileInfo     Upon exit, points to the file's
                      information.

  @retval EFI_SUCCESS             File information returned.

  @retval EFI_INVALID_PARAMETER   If FileHandle does not
                                  represent a valid file.

  @retval EFI_INVALID_PARAMETER   If FileInfo is NULL.

**/
EFI_STATUS
EFIAPI
FfsGetFileInfo (
  IN EFI_PEI_FILE_HANDLE  FileHandle,
  OUT EFI_FV_FILE_INFO    *FileInfo
  )
{
  UINT8                FileState;
  UINT8                ErasePolarity;
  EFI_FFS_FILE_HEADER  *FileHeader;
  EFI_PEI_FV_HANDLE    VolumeHandle;

  if ((FileHandle == NULL) || (FileInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  VolumeHandle = 0;
  //
  // Retrieve the FirmwareVolume which the file resides in.
  //
  if (!FileHandleToVolume (FileHandle, &VolumeHandle)) {
    return EFI_INVALID_PARAMETER;
  }

  if (((EFI_FIRMWARE_VOLUME_HEADER *)VolumeHandle)->Attributes & EFI_FVB2_ERASE_POLARITY) {
    ErasePolarity = 1;
  } else {
    ErasePolarity = 0;
  }

  //
  // Get FileState which is the highest bit of the State
  //
  FileState = GetFileState (ErasePolarity, (EFI_FFS_FILE_HEADER *)FileHandle);

  switch (FileState) {
    case EFI_FILE_DATA_VALID:
    case EFI_FILE_MARKED_FOR_UPDATE:
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  FileHeader = (EFI_FFS_FILE_HEADER *)FileHandle;
  CopyMem (&FileInfo->FileName, &FileHeader->Name, sizeof (EFI_GUID));
  FileInfo->FileType       = FileHeader->Type;
  FileInfo->FileAttributes = FileHeader->Attributes;
  FileInfo->BufferSize     = ((*(UINT32 *)FileHeader->Size) & 0x00FFFFFF) -  sizeof (EFI_FFS_FILE_HEADER);
  FileInfo->Buffer         = (FileHeader + 1);
  return EFI_SUCCESS;
}

/**
  Get Information about the volume by name

  @param VolumeHandle   Handle of the volume.

  @param VolumeInfo     Upon exit, points to the volume's
                        information.

  @retval EFI_SUCCESS             File information returned.

  @retval EFI_INVALID_PARAMETER   If FileHandle does not
                                  represent a valid file.

  @retval EFI_INVALID_PARAMETER   If FileInfo is NULL.

**/
EFI_STATUS
EFIAPI
FfsGetVolumeInfo (
  IN EFI_PEI_FV_HANDLE  VolumeHandle,
  OUT EFI_FV_INFO       *VolumeInfo
  )
{
  EFI_FIRMWARE_VOLUME_HEADER      FwVolHeader;
  EFI_FIRMWARE_VOLUME_EXT_HEADER  *FwVolExHeaderInfo;

  if (VolumeInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // VolumeHandle may not align at 8 byte,
  // but FvLength is UINT64 type, which requires FvHeader align at least 8 byte.
  // So, Copy FvHeader into the local FvHeader structure.
  //
  CopyMem (&FwVolHeader, VolumeHandle, sizeof (EFI_FIRMWARE_VOLUME_HEADER));
  //
  // Check Fv Image Signature
  //
  if (FwVolHeader.Signature != EFI_FVH_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  VolumeInfo->FvAttributes = FwVolHeader.Attributes;
  VolumeInfo->FvStart      = (VOID *)VolumeHandle;
  VolumeInfo->FvSize       = FwVolHeader.FvLength;
  CopyMem (&VolumeInfo->FvFormat, &FwVolHeader.FileSystemGuid, sizeof (EFI_GUID));

  if (FwVolHeader.ExtHeaderOffset != 0) {
    FwVolExHeaderInfo = (EFI_FIRMWARE_VOLUME_EXT_HEADER *)(((UINT8 *)VolumeHandle) + FwVolHeader.ExtHeaderOffset);
    CopyMem (&VolumeInfo->FvName, &FwVolExHeaderInfo->FvName, sizeof (EFI_GUID));
  }

  return EFI_SUCCESS;
}

/**
  Search through every FV until you find a file of type FileType

  @param FileType        File handle of a Fv type file.
  @param Volumehandle    On success Volume Handle of the match
  @param FileHandle      On success File Handle of the match

  @retval EFI_NOT_FOUND  FV image can't be found.
  @retval EFI_SUCCESS    Successfully found FileType

**/
EFI_STATUS
EFIAPI
FfsAnyFvFindFirstFile (
  IN  EFI_FV_FILETYPE      FileType,
  OUT EFI_PEI_FV_HANDLE    *VolumeHandle,
  OUT EFI_PEI_FILE_HANDLE  *FileHandle
  )
{
  EFI_STATUS  Status;
  UINTN       Instance;

  //
  // Search every FV for the DXE Core
  //
  Instance    = 0;
  *FileHandle = NULL;

  while (1) {
    Status = FfsFindNextVolume (Instance++, VolumeHandle);
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = FfsFindNextFile (FileType, *VolumeHandle, FileHandle);
    if (!EFI_ERROR (Status)) {
      break;
    }
  }

  return Status;
}

/**
  Get Fv image from the FV type file, then add FV & FV2 Hob.

  @param FileHandle  File handle of a Fv type file.


  @retval EFI_NOT_FOUND  FV image can't be found.
  @retval EFI_SUCCESS    Successfully to process it.

**/
EFI_STATUS
EFIAPI
FfsProcessFvFile (
  IN  EFI_PEI_FILE_HANDLE  FvFileHandle
  )
{
  EFI_STATUS            Status;
  EFI_PEI_FV_HANDLE     FvImageHandle;
  EFI_FV_INFO           FvImageInfo;
  UINT32                FvAlignment;
  VOID                  *FvBuffer;
  EFI_PEI_HOB_POINTERS  HobFv2;

  FvBuffer = NULL;

  //
  // Check if this EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE file has already
  // been extracted.
  //
  HobFv2.Raw = GetHobList ();
  while ((HobFv2.Raw = GetNextHob (EFI_HOB_TYPE_FV2, HobFv2.Raw)) != NULL) {
    if (CompareGuid (&(((EFI_FFS_FILE_HEADER *)FvFileHandle)->Name), &HobFv2.FirmwareVolume2->FileName)) {
      //
      // this FILE has been dispatched, it will not be dispatched again.
      //
      return EFI_SUCCESS;
    }

    HobFv2.Raw = GET_NEXT_HOB (HobFv2);
  }

  //
  // Find FvImage in FvFile
  //
  Status = FfsFindSectionDataWithHook (EFI_SECTION_FIRMWARE_VOLUME_IMAGE, NULL, FvFileHandle, (VOID **)&FvImageHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Collect FvImage Info.
  //
  ZeroMem (&FvImageInfo, sizeof (FvImageInfo));
  Status = FfsGetVolumeInfo (FvImageHandle, &FvImageInfo);
  ASSERT_EFI_ERROR (Status);

  //
  // FvAlignment must be more than 8 bytes required by FvHeader structure.
  //
  FvAlignment = 1 << ((FvImageInfo.FvAttributes & EFI_FVB2_ALIGNMENT) >> 16);
  if (FvAlignment < 8) {
    FvAlignment = 8;
  }

  //
  // Check FvImage
  //
  if ((UINTN)FvImageInfo.FvStart % FvAlignment != 0) {
    FvBuffer = AllocateAlignedPages (EFI_SIZE_TO_PAGES ((UINT32)FvImageInfo.FvSize), FvAlignment);
    if (FvBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (FvBuffer, FvImageInfo.FvStart, (UINTN)FvImageInfo.FvSize);
    //
    // Update FvImageInfo after reload FvImage to new aligned memory
    //
    FfsGetVolumeInfo ((EFI_PEI_FV_HANDLE)FvBuffer, &FvImageInfo);
  }

  //
  // Inform HOB consumer phase, i.e. DXE core, the existence of this FV
  //
  BuildFvHob ((EFI_PHYSICAL_ADDRESS)(UINTN)FvImageInfo.FvStart, FvImageInfo.FvSize);

  //
  // Makes the encapsulated volume show up in DXE phase to skip processing of
  // encapsulated file again.
  //
  BuildFv2Hob (
    (EFI_PHYSICAL_ADDRESS)(UINTN)FvImageInfo.FvStart,
    FvImageInfo.FvSize,
    &FvImageInfo.FvName,
    &(((EFI_FFS_FILE_HEADER *)FvFileHandle)->Name)
    );

  return EFI_SUCCESS;
}
