/** @file
  Firmware File System protocol. Layers on top of Firmware
  Block protocol to produce a file abstraction of FV based files.

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FW_VOL_DRIVER_H_
#define __FW_VOL_DRIVER_H_

#define FV2_DEVICE_SIGNATURE  SIGNATURE_32 ('_', 'F', 'V', '2')

//
// Used to track all non-deleted files
//
typedef struct {
  LIST_ENTRY             Link;
  EFI_FFS_FILE_HEADER    *FfsHeader;
  UINTN                  StreamHandle;
  BOOLEAN                FileCached;
} FFS_FILE_LIST_ENTRY;

typedef struct {
  UINTN                                 Signature;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *Fvb;
  EFI_HANDLE                            Handle;
  EFI_FIRMWARE_VOLUME2_PROTOCOL         Fv;

  EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeader;
  UINT8                                 *CachedFv;
  UINT8                                 *EndOfCachedFv;

  FFS_FILE_LIST_ENTRY                   *LastKey;

  LIST_ENTRY                            FfsFileListHeader;

  UINT32                                AuthenticationStatus;
  UINT8                                 ErasePolarity;
  BOOLEAN                               IsFfs3Fv;
  BOOLEAN                               IsMemoryMapped;
} FV_DEVICE;

#define FV_DEVICE_FROM_THIS(a)  CR(a, FV_DEVICE, Fv, FV2_DEVICE_SIGNATURE)

/**
  Retrieves attributes, insures positive polarity of attribute bits, returns
  resulting attributes in output parameter.

  @param  This             Pointer to EFI_FIRMWARE_VOLUME2_PROTOCOL.
  @param  Attributes       output buffer which contains attributes.

  @retval EFI_SUCCESS      Successfully got volume attributes.

**/
EFI_STATUS
EFIAPI
FvGetVolumeAttributes (
  IN  CONST EFI_FIRMWARE_VOLUME2_PROTOCOL  *This,
  OUT       EFI_FV_ATTRIBUTES              *Attributes
  );

/**
  Sets current attributes for volume

  @param  This             Pointer to EFI_FIRMWARE_VOLUME2_PROTOCOL.
  @param  Attributes       At input, contains attributes to be set.  At output
                           contains new value of FV.

  @retval EFI_UNSUPPORTED  Could not be set.

**/
EFI_STATUS
EFIAPI
FvSetVolumeAttributes (
  IN     CONST EFI_FIRMWARE_VOLUME2_PROTOCOL  *This,
  IN OUT       EFI_FV_ATTRIBUTES              *Attributes
  );

/**
  Given the input key, search for the next matching file in the volume.

  @param  This                       Pointer to EFI_FIRMWARE_VOLUME2_PROTOCOL.
  @param  Key                        Key is a pointer to a caller allocated
                                     buffer that contains implementation specific
                                     data that is used to track where to begin
                                     the search for the next file. The size of
                                     the buffer must be at least This->KeySize
                                     bytes long. To reinitialize the search and
                                     begin from the beginning of the firmware
                                     volume, the entire buffer must be cleared to
                                     zero. Other than clearing the buffer to
                                     initiate a new search, the caller must not
                                     modify the data in the buffer between calls
                                     to GetNextFile().
  @param  FileType                   FileType is a pointer to a caller allocated
                                     EFI_FV_FILETYPE. The GetNextFile() API can
                                     filter it's search for files based on the
                                     value of *FileType input. A *FileType input
                                     of 0 causes GetNextFile() to search for
                                     files of all types.  If a file is found, the
                                     file's type is returned in *FileType.
                                     *FileType is not modified if no file is
                                     found.
  @param  NameGuid                   NameGuid is a pointer to a caller allocated
                                     EFI_GUID. If a file is found, the file's
                                     name is returned in *NameGuid.  *NameGuid is
                                     not modified if no file is found.
  @param  Attributes                 Attributes is a pointer to a caller
                                     allocated EFI_FV_FILE_ATTRIBUTES.  If a file
                                     is found, the file's attributes are returned
                                     in *Attributes. *Attributes is not modified
                                     if no file is found.
  @param  Size                       Size is a pointer to a caller allocated
                                     UINTN. If a file is found, the file's size
                                     is returned in *Size. *Size is not modified
                                     if no file is found.

  @retval EFI_SUCCESS                Successfully find the file.
  @retval EFI_DEVICE_ERROR           Device error.
  @retval EFI_ACCESS_DENIED          Fv could not read.
  @retval EFI_NOT_FOUND              No matching file found.
  @retval EFI_INVALID_PARAMETER      Invalid parameter

**/
EFI_STATUS
EFIAPI
FvGetNextFile (
  IN CONST   EFI_FIRMWARE_VOLUME2_PROTOCOL  *This,
  IN OUT     VOID                           *Key,
  IN OUT     EFI_FV_FILETYPE                *FileType,
  OUT        EFI_GUID                       *NameGuid,
  OUT        EFI_FV_FILE_ATTRIBUTES         *Attributes,
  OUT        UINTN                          *Size
  );

/**
  Locates a file in the firmware volume and
  copies it to the supplied buffer.

  @param  This                       Pointer to EFI_FIRMWARE_VOLUME2_PROTOCOL.
  @param  NameGuid                   Pointer to an EFI_GUID, which is the
                                     filename.
  @param  Buffer                     Buffer is a pointer to pointer to a buffer
                                     in which the file or section contents or are
                                     returned.
  @param  BufferSize                 BufferSize is a pointer to caller allocated
                                     UINTN. On input *BufferSize indicates the
                                     size in bytes of the memory region pointed
                                     to by Buffer. On output, *BufferSize
                                     contains the number of bytes required to
                                     read the file.
  @param  FoundType                  FoundType is a pointer to a caller allocated
                                     EFI_FV_FILETYPE that on successful return
                                     from Read() contains the type of file read.
                                     This output reflects the file type
                                     irrespective of the value of the SectionType
                                     input.
  @param  FileAttributes             FileAttributes is a pointer to a caller
                                     allocated EFI_FV_FILE_ATTRIBUTES.  On
                                     successful return from Read(),
                                     *FileAttributes contains the attributes of
                                     the file read.
  @param  AuthenticationStatus       AuthenticationStatus is a pointer to a
                                     caller allocated UINTN in which the
                                     authentication status is returned.

  @retval EFI_SUCCESS                Successfully read to memory buffer.
  @retval EFI_WARN_BUFFER_TOO_SMALL  Buffer too small.
  @retval EFI_NOT_FOUND              Not found.
  @retval EFI_DEVICE_ERROR           Device error.
  @retval EFI_ACCESS_DENIED          Could not read.
  @retval EFI_INVALID_PARAMETER      Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES       Not enough buffer to be allocated.

**/
EFI_STATUS
EFIAPI
FvReadFile (
  IN CONST EFI_FIRMWARE_VOLUME2_PROTOCOL  *This,
  IN CONST EFI_GUID                       *NameGuid,
  IN OUT   VOID                           **Buffer,
  IN OUT   UINTN                          *BufferSize,
  OUT      EFI_FV_FILETYPE                *FoundType,
  OUT      EFI_FV_FILE_ATTRIBUTES         *FileAttributes,
  OUT      UINT32                         *AuthenticationStatus
  );

/**
  Locates a section in a given FFS File and
  copies it to the supplied buffer (not including section header).

  @param  This                       Pointer to EFI_FIRMWARE_VOLUME2_PROTOCOL.
  @param  NameGuid                   Pointer to an EFI_GUID, which is the
                                     filename.
  @param  SectionType                Indicates the section type to return.
  @param  SectionInstance            Indicates which instance of sections with a
                                     type of SectionType to return.
  @param  Buffer                     Buffer is a pointer to pointer to a buffer
                                     in which the file or section contents or are
                                     returned.
  @param  BufferSize                 BufferSize is a pointer to caller allocated
                                     UINTN.
  @param  AuthenticationStatus       AuthenticationStatus is a pointer to a
                                     caller allocated UINT32 in which the
                                     authentication status is returned.

  @retval EFI_SUCCESS                Successfully read the file section into
                                     buffer.
  @retval EFI_WARN_BUFFER_TOO_SMALL  Buffer too small.
  @retval EFI_NOT_FOUND              Section not found.
  @retval EFI_DEVICE_ERROR           Device error.
  @retval EFI_ACCESS_DENIED          Could not read.
  @retval EFI_INVALID_PARAMETER      Invalid parameter.

**/
EFI_STATUS
EFIAPI
FvReadFileSection (
  IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL  *This,
  IN CONST  EFI_GUID                       *NameGuid,
  IN        EFI_SECTION_TYPE               SectionType,
  IN        UINTN                          SectionInstance,
  IN OUT    VOID                           **Buffer,
  IN OUT    UINTN                          *BufferSize,
  OUT       UINT32                         *AuthenticationStatus
  );

/**
  Writes one or more files to the firmware volume.

  @param  This                   Pointer to EFI_FIRMWARE_VOLUME2_PROTOCOL.
  @param  NumberOfFiles          Number of files.
  @param  WritePolicy            WritePolicy indicates the level of reliability
                                 for the write in the event of a power failure or
                                 other system failure during the write operation.
  @param  FileData               FileData is an pointer to an array of
                                 EFI_FV_WRITE_DATA. Each element of array
                                 FileData represents a file to be written.

  @retval EFI_SUCCESS            Files successfully written to firmware volume
  @retval EFI_OUT_OF_RESOURCES   Not enough buffer to be allocated.
  @retval EFI_DEVICE_ERROR       Device error.
  @retval EFI_WRITE_PROTECTED    Write protected.
  @retval EFI_NOT_FOUND          Not found.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_UNSUPPORTED        This function not supported.

**/
EFI_STATUS
EFIAPI
FvWriteFile (
  IN CONST EFI_FIRMWARE_VOLUME2_PROTOCOL  *This,
  IN       UINT32                         NumberOfFiles,
  IN       EFI_FV_WRITE_POLICY            WritePolicy,
  IN       EFI_FV_WRITE_FILE_DATA         *FileData
  );

/**
  Return information of type InformationType for the requested firmware
  volume.

  @param  This             Pointer to EFI_FIRMWARE_VOLUME2_PROTOCOL.
  @param  InformationType  InformationType for requested.
  @param  BufferSize       On input, size of Buffer.On output, the amount of data
                           returned in Buffer.
  @param  Buffer           A poniter to the data buffer to return.

  @retval EFI_SUCCESS      Successfully got volume Information.

**/
EFI_STATUS
EFIAPI
FvGetVolumeInfo (
  IN  CONST EFI_FIRMWARE_VOLUME2_PROTOCOL  *This,
  IN  CONST EFI_GUID                       *InformationType,
  IN OUT UINTN                             *BufferSize,
  OUT VOID                                 *Buffer
  );

/**
  Set information of type InformationType for the requested firmware
  volume.

  @param  This             Pointer to EFI_FIRMWARE_VOLUME2_PROTOCOL.
  @param  InformationType  InformationType for requested.
  @param  BufferSize       On input, size of Buffer.On output, the amount of data
                           returned in Buffer.
  @param  Buffer           A poniter to the data buffer to return.

  @retval EFI_SUCCESS      Successfully set volume Information.

**/
EFI_STATUS
EFIAPI
FvSetVolumeInfo (
  IN  CONST EFI_FIRMWARE_VOLUME2_PROTOCOL  *This,
  IN  CONST EFI_GUID                       *InformationType,
  IN  UINTN                                BufferSize,
  IN CONST  VOID                           *Buffer
  );

/**
  Check if a block of buffer is erased.

  @param  ErasePolarity  Erase polarity attribute of the firmware volume
  @param  InBuffer       The buffer to be checked
  @param  BufferSize     Size of the buffer in bytes

  @retval TRUE           The block of buffer is erased
  @retval FALSE          The block of buffer is not erased

**/
BOOLEAN
IsBufferErased (
  IN UINT8  ErasePolarity,
  IN VOID   *InBuffer,
  IN UINTN  BufferSize
  );

/**
  Get the FFS file state by checking the highest bit set in the header's state field.

  @param  ErasePolarity  Erase polarity attribute of the firmware volume
  @param  FfsHeader      Points to the FFS file header

  @return FFS File state

**/
EFI_FFS_FILE_STATE
GetFileState (
  IN UINT8                ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  );

/**
  Set the FFS file state.

  @param  State                      The state to be set.
  @param  FfsHeader                  Points to the FFS file header

  @return None.

**/
VOID
SetFileState (
  IN UINT8                State,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  );

/**
  Check if it's a valid FFS file header.

  @param  ErasePolarity  Erase polarity attribute of the firmware volume
  @param  FfsHeader      Points to the FFS file header to be checked
  @param  FileState      FFS file state to be returned

  @retval TRUE           Valid FFS file header
  @retval FALSE          Invalid FFS file header

**/
BOOLEAN
IsValidFfsHeader (
  IN UINT8                ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader,
  OUT EFI_FFS_FILE_STATE  *FileState
  );

/**
  Check if it's a valid FFS file.
  Here we are sure that it has a valid FFS file header since we must call IsValidFfsHeader() first.

  @param  ErasePolarity  Erase polarity attribute of the firmware volume
  @param  FfsHeader      Points to the FFS file to be checked

  @retval TRUE           Valid FFS file
  @retval FALSE          Invalid FFS file

**/
BOOLEAN
IsValidFfsFile (
  IN UINT8                ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  );

#endif
