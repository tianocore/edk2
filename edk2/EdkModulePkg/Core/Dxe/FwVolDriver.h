/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FwVolDriver.h

Abstract:

  Firmware File System protocol. Layers on top of Firmware
  Block protocol to produce a file abstraction of FV based files.

--*/

#ifndef __FWVOL_H
#define __FWVOL_H


//
// Used to track all non-deleted files
//
typedef struct {
  LIST_ENTRY                      Link;
  EFI_FFS_FILE_HEADER             *FfsHeader;
  UINTN                           StreamHandle;
  EFI_SECTION_EXTRACTION_PROTOCOL *Sep;
} FFS_FILE_LIST_ENTRY;

typedef struct {
  UINTN                                   Signature;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL      *Fvb;
  EFI_HANDLE                              Handle;
  EFI_FIRMWARE_VOLUME_PROTOCOL            Fv;

  EFI_FIRMWARE_VOLUME_HEADER              *FwVolHeader;
  UINT8                                   *CachedFv;
  UINT8                                   *EndOfCachedFv;

  FFS_FILE_LIST_ENTRY                     *LastKey;

  LIST_ENTRY                              FfsFileListHeader;

  UINT8                                   ErasePolarity;
} FV_DEVICE;

#define FV_DEVICE_FROM_THIS(a) CR(a, FV_DEVICE, Fv, FV_DEVICE_SIGNATURE)


EFI_STATUS
EFIAPI
FvGetVolumeAttributes (
  IN    EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  OUT   EFI_FV_ATTRIBUTES              *Attributes
  )
/*++

Routine Description:
    Retrieves attributes, insures positive polarity of attribute bits, returns
    resulting attributes in output parameter

Arguments:
    This        - Calling context
    Attributes  - output buffer which contains attributes

Returns:
    EFI_SUCCESS         - Successfully got volume attributes

--*/
;

EFI_STATUS
EFIAPI
FvSetVolumeAttributes (
  IN     EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN OUT EFI_FV_ATTRIBUTES              *Attributes
  )
/*++

Routine Description:
    Sets current attributes for volume

Arguments:
    This       - Calling context
    Attributes - At input, contains attributes to be set.  At output contains
      new value of FV

Returns:
    EFI_UNSUPPORTED   - Could not be set.
--*/
;

EFI_STATUS
EFIAPI
FvGetNextFile (
  IN     EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN OUT VOID                           *Key,
  IN OUT EFI_FV_FILETYPE                *FileType,
  OUT    EFI_GUID                       *NameGuid,
  OUT    EFI_FV_FILE_ATTRIBUTES         *Attributes,
  OUT    UINTN                          *Size
  )
/*++

Routine Description:
    Given the input key, search for the next matching file in the volume.

Arguments:
    This          -   Indicates the calling context.
    FileType      -   FileType is a pointer to a caller allocated
                      EFI_FV_FILETYPE. The GetNextFile() API can filter it's
                      search for files based on the value of *FileType input.
                      A *FileType input of 0 causes GetNextFile() to search for
                      files of all types.  If a file is found, the file's type
                      is returned in *FileType.  *FileType is not modified if
                      no file is found.
    Key           -   Key is a pointer to a caller allocated buffer that
                      contains implementation specific data that is used to
                      track where to begin the search for the next file.
                      The size of the buffer must be at least This->KeySize
                      bytes long. To reinitialize the search and begin from
                      the beginning of the firmware volume, the entire buffer
                      must be cleared to zero. Other than clearing the buffer
                      to initiate a new search, the caller must not modify the
                      data in the buffer between calls to GetNextFile().
    NameGuid      -   NameGuid is a pointer to a caller allocated EFI_GUID.
                      If a file is found, the file's name is returned in
                      *NameGuid.  *NameGuid is not modified if no file is
                      found.
    Attributes    -   Attributes is a pointer to a caller allocated
                      EFI_FV_FILE_ATTRIBUTES.  If a file is found, the file's
                      attributes are returned in *Attributes. *Attributes is
                      not modified if no file is found.
    Size          -   Size is a pointer to a caller allocated UINTN.
                      If a file is found, the file's size is returned in *Size.
                      *Size is not modified if no file is found.

Returns:    
    EFI_SUCCESS                 - Successfully find the file.
    EFI_DEVICE_ERROR            - Device error.
    EFI_ACCESS_DENIED           - Fv could not read.
    EFI_NOT_FOUND               - No matching file found.
    EFI_INVALID_PARAMETER       - Invalid parameter

--*/
;


EFI_STATUS
EFIAPI
FvReadFile (
  IN     EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN     EFI_GUID                       *NameGuid,
  IN OUT VOID                           **Buffer,
  IN OUT UINTN                          *BufferSize,
  OUT    EFI_FV_FILETYPE                *FoundType,
  OUT    EFI_FV_FILE_ATTRIBUTES         *FileAttributes,
  OUT    UINT32                         *AuthenticationStatus
  )
/*++

Routine Description:
    Locates a file in the firmware volume and
    copies it to the supplied buffer.

Arguments:
    This              -   Indicates the calling context.
    NameGuid          -   Pointer to an EFI_GUID, which is the filename.
    Buffer            -   Buffer is a pointer to pointer to a buffer in
                          which the file or section contents or are returned.
    BufferSize        -   BufferSize is a pointer to caller allocated
                          UINTN. On input *BufferSize indicates the size
                          in bytes of the memory region pointed to by
                          Buffer. On output, *BufferSize contains the number
                          of bytes required to read the file.
    FoundType         -   FoundType is a pointer to a caller allocated
                          EFI_FV_FILETYPE that on successful return from Read()
                          contains the type of file read.  This output reflects
                          the file type irrespective of the value of the
                          SectionType input.
    FileAttributes    -   FileAttributes is a pointer to a caller allocated
                          EFI_FV_FILE_ATTRIBUTES.  On successful return from
                          Read(), *FileAttributes contains the attributes of
                          the file read.
    AuthenticationStatus -  AuthenticationStatus is a pointer to a caller
                          allocated UINTN in which the authentication status
                          is returned.
Returns:
    EFI_SUCCESS                   - Successfully read to memory buffer.
    EFI_WARN_BUFFER_TOO_SMALL     - Buffer too small.
    EFI_NOT_FOUND                 - Not found.
    EFI_DEVICE_ERROR              - Device error.
    EFI_ACCESS_DENIED             - Could not read.
    EFI_INVALID_PARAMETER         - Invalid parameter.
    EFI_OUT_OF_RESOURCES          - Not enough buffer to be allocated.

--*/
;

EFI_STATUS
EFIAPI
FvReadFileSection (
  IN     EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN     EFI_GUID                       *NameGuid,
  IN     EFI_SECTION_TYPE               SectionType,
  IN     UINTN                          SectionInstance,
  IN OUT VOID                           **Buffer,
  IN OUT UINTN                          *BufferSize,
  OUT    UINT32                         *AuthenticationStatus
  )
/*++

  Routine Description:
    Locates a section in a given FFS File and
    copies it to the supplied buffer (not including section header).

  Arguments:
    This              -   Indicates the calling context.
    NameGuid          -   Pointer to an EFI_GUID, which is the filename.
    SectionType       -   Indicates the section type to return.
    SectionInstance   -   Indicates which instance of sections with a type of
                          SectionType to return.
    Buffer            -   Buffer is a pointer to pointer to a buffer in which
                          the file or section contents or are returned.
    BufferSize        -   BufferSize is a pointer to caller allocated UINTN.
    AuthenticationStatus -AuthenticationStatus is a pointer to a caller
                          allocated UINT32 in which the authentication status
                          is returned.

  Returns:
    EFI_SUCCESS                     - Successfully read the file section into buffer.
    EFI_WARN_BUFFER_TOO_SMALL       - Buffer too small.
    EFI_NOT_FOUND                   - Section not found.
    EFI_DEVICE_ERROR                - Device error.
    EFI_ACCESS_DENIED               - Could not read.
    EFI_INVALID_PARAMETER           - Invalid parameter.

--*/
;

EFI_STATUS
EFIAPI
FvWriteFile (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL       *This,
  IN UINT32                             NumberOfFiles,
  IN EFI_FV_WRITE_POLICY                WritePolicy,
  IN EFI_FV_WRITE_FILE_DATA             *FileData
  )
/*++

    Routine Description:
      Writes one or more files to the firmware volume.

    Arguments:
    This        - Indicates the calling context.
    WritePolicy - WritePolicy indicates the level of reliability for
                  the write in the event of a power failure or other
                  system failure during the write operation.
    FileData    - FileData is an pointer to an array of EFI_FV_WRITE_DATA.
                  Each element of FileData[] represents a file to be written.

    Returns:
      EFI_SUCCESS                   - Files successfully written to firmware volume
      EFI_OUT_OF_RESOURCES          - Not enough buffer to be allocated.
      EFI_DEVICE_ERROR              - Device error.
      EFI_WRITE_PROTECTED           - Write protected.
      EFI_NOT_FOUND                 - Not found.
      EFI_INVALID_PARAMETER         - Invalid parameter.
      EFI_UNSUPPORTED               - This function not supported.

--*/
;


  
//
//Internal functions
//
typedef enum {
  EfiCheckSumUint8    = 0,
  EfiCheckSumUint16   = 1,
  EfiCheckSumUint32   = 2,
  EfiCheckSumUint64   = 3,
  EfiCheckSumMaximum  = 4
} EFI_CHECKSUM_TYPE;


BOOLEAN
IsBufferErased (
  IN UINT8    ErasePolarity,
  IN VOID     *Buffer,
  IN UINTN    BufferSize
  )
/*++

Routine Description:
  Check if a block of buffer is erased

Arguments:
  ErasePolarity -  Erase polarity attribute of the firmware volume
  Buffer        -  The buffer to be checked
  BufferSize    -  Size of the buffer in bytes
    
Returns:
  TRUE  -  The block of buffer is erased
  FALSE -  The block of buffer is not erased
    
--*/
;

EFI_FFS_FILE_STATE 
GetFileState (
  IN UINT8                ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
/*++

Routine Description:
  Get the FFS file state by checking the highest bit set in the header's state field

Arguments:
  ErasePolarity -  Erase polarity attribute of the firmware volume
  FfsHeader     -  Points to the FFS file header
    
Returns:
  FFS File state 
    
--*/
;

VOID
SetFileState (
  IN UINT8                State,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
/*++

Routine Description:
  Set the FFS file state.

Arguments:
  State         -  The state to be set.
  FfsHeader     -  Points to the FFS file header
    
Returns:
  None.
    
--*/
;

BOOLEAN
VerifyFvHeaderChecksum (
  IN EFI_FIRMWARE_VOLUME_HEADER *FvHeader
  )
/*++

Routine Description:
  Verify checksum of the firmware volume header 

Arguments:
  FvHeader  -  Points to the firmware volume header to be checked
    
Returns:
  TRUE  -  Checksum verification passed
  FALSE -  Checksum verification failed
    
--*/
;
    
BOOLEAN
IsValidFfsHeader (
  IN  UINT8                ErasePolarity,
  IN  EFI_FFS_FILE_HEADER  *FfsHeader,
  OUT EFI_FFS_FILE_STATE   *FileState
  )
/*++

Routine Description:
  Check if it's a valid FFS file header

Arguments:
  ErasePolarity -  Erase polarity attribute of the firmware volume
  FfsHeader     -  Points to the FFS file header to be checked
  FileState     -  FFS file state to be returned
    
Returns:
  TRUE  -  Valid FFS file header
  FALSE -  Invalid FFS file header
    
--*/
;

BOOLEAN
IsValidFfsFile (
  IN UINT8                ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
/*++

Routine Description:
  Check if it's a valid FFS file. 
  Here we are sure that it has a valid FFS file header since we must call IsValidFfsHeader() first.

Arguments:
  ErasePolarity -  Erase polarity attribute of the firmware volume
  FfsHeader     -  Points to the FFS file to be checked
    
Returns:
  TRUE  -  Valid FFS file
  FALSE -  Invalid FFS file
    
--*/
;

EFI_STATUS
GetFwVolHeader (
  IN  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL      *Fvb,
  OUT EFI_FIRMWARE_VOLUME_HEADER              **FwVolHeader
  )
/*++

Routine Description:
  given the supplied FW_VOL_BLOCK_PROTOCOL, allocate a buffer for output and
  copy the volume header into it.

Arguments:
  Fvb - The FW_VOL_BLOCK_PROTOCOL instance from which to read the volume
          header
  FwVolHeader - Pointer to pointer to allocated buffer in which the volume
                  header is returned.

Returns:
  Status code.

--*/
;


EFI_STATUS
FvCheck (
  IN OUT FV_DEVICE  *FvDevice
  )
/*++

Routine Description:
  Check if a FV is consistent and allocate cache

Arguments:
  FvDevice - pointer to the FvDevice to be checked.

Returns:
  EFI_OUT_OF_RESOURCES    - Not enough buffer to be allocated.
  EFI_SUCCESS             - FV is consistent and cache is allocated.
  EFI_VOLUME_CORRUPTED    - File system is corrupted.

--*/
;

#endif
