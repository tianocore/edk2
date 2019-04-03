/** @file
  Common defines and definitions for a FwVolDxe driver.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FWVOL_DRIVER_H_
#define _FWVOL_DRIVER_H_

#include <PiDxe.h>

#include <Guid/FirmwareFileSystem2.h>
#include <Guid/FirmwareFileSystem3.h>
#include <Protocol/SectionExtraction.h>
#include <Protocol/FaultTolerantWrite.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/FirmwareVolumeBlock.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HobLib.h>

#define FV_DEVICE_SIGNATURE           SIGNATURE_32 ('_', 'F', 'V', '_')

//
// Define two helper macro to extract the Capability field or Status field in FVB
// bit fields
//
#define EFI_FVB2_CAPABILITIES (EFI_FVB2_READ_DISABLED_CAP | \
                              EFI_FVB2_READ_ENABLED_CAP | \
                              EFI_FVB2_WRITE_DISABLED_CAP | \
                              EFI_FVB2_WRITE_ENABLED_CAP | \
                              EFI_FVB2_LOCK_CAP \
                              )

#define EFI_FVB2_STATUS (EFI_FVB2_READ_STATUS | EFI_FVB2_WRITE_STATUS | EFI_FVB2_LOCK_STATUS)

#define MAX_FILES 32

//
// Used to calculate from address -> Lba
//
typedef struct {
  LIST_ENTRY      Link;
  EFI_LBA         LbaIndex;
  UINT8           *StartingAddress;
  UINTN           BlockLength;
} LBA_ENTRY;

//
// Used to track free space in the Fv
//
typedef struct {
  LIST_ENTRY      Link;
  UINT8           *StartingAddress;
  UINTN           Length;
} FREE_SPACE_ENTRY;

//
// Used to track all non-deleted files
//
typedef struct {
  LIST_ENTRY      Link;
  UINT8           *FfsHeader;
} FFS_FILE_LIST_ENTRY;

typedef struct {
  UINTN                               Signature;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_FIRMWARE_VOLUME2_PROTOCOL       Fv;
  EFI_FIRMWARE_VOLUME_HEADER          *FwVolHeader;
  UINT8                               *Key;
  EFI_HANDLE                          Handle;

  UINT8                               ErasePolarity;
  EFI_PHYSICAL_ADDRESS                CachedFv;
  LIST_ENTRY                          LbaHeader;
  LIST_ENTRY                          FreeSpaceHeader;
  LIST_ENTRY                          FfsFileListHeader;

  FFS_FILE_LIST_ENTRY                 *CurrentFfsFile;
  BOOLEAN                             IsFfs3Fv;
  UINT32                              AuthenticationStatus;
} FV_DEVICE;

#define FV_DEVICE_FROM_THIS(a)  CR (a, FV_DEVICE, Fv, FV_DEVICE_SIGNATURE)

/**
  Retrieves attributes, insures positive polarity of attribute bits, returns
  resulting attributes in output parameter.

  @param  This             Calling context
  @param  Attributes       output buffer which contains attributes

  @retval EFI_SUCCESS      Successfully got volume attributes

**/
EFI_STATUS
EFIAPI
FvGetVolumeAttributes (
  IN  CONST EFI_FIRMWARE_VOLUME2_PROTOCOL  *This,
  OUT EFI_FV_ATTRIBUTES             *Attributes
  );

/**
  Sets current attributes for volume.

  @param  This          Calling context
  @param  Attributes    On input, FvAttributes is a pointer to
                        an EFI_FV_ATTRIBUTES containing the
                        desired firmware volume settings. On
                        successful return, it contains the new
                        settings of the firmware volume. On
                        unsuccessful return, FvAttributes is not
                        modified and the firmware volume
                        settings are not changed.

  @retval EFI_SUCCESS             The requested firmware volume attributes
                                  were set and the resulting
                                  EFI_FV_ATTRIBUTES is returned in
                                  FvAttributes.
  @retval EFI_ACCESS_DENIED       Atrribute is locked down.
  @retval EFI_INVALID_PARAMETER   Atrribute is not valid.

**/
EFI_STATUS
EFIAPI
FvSetVolumeAttributes (
  IN CONST EFI_FIRMWARE_VOLUME2_PROTOCOL   *This,
  IN OUT EFI_FV_ATTRIBUTES          *Attributes
  );

/**
  Given the input key, search for the next matching file in the volume.

  @param  This                       Indicates the calling context.
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
  IN CONST EFI_FIRMWARE_VOLUME2_PROTOCOL   *This,
  IN OUT VOID                       *Key,
  IN OUT EFI_FV_FILETYPE            *FileType,
  OUT EFI_GUID                      *NameGuid,
  OUT EFI_FV_FILE_ATTRIBUTES        *Attributes,
  OUT UINTN                         *Size
  );

/**
  Locates a file in the firmware volume and
  copies it to the supplied buffer.

  @param  This                       Indicates the calling context.
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
  IN CONST EFI_FIRMWARE_VOLUME2_PROTOCOL   *This,
  IN CONST EFI_GUID                       *NameGuid,
  IN OUT VOID                       **Buffer,
  IN OUT UINTN                      *BufferSize,
  OUT EFI_FV_FILETYPE               *FoundType,
  OUT EFI_FV_FILE_ATTRIBUTES        *FileAttributes,
  OUT UINT32                        *AuthenticationStatus
  );

/**
  Locates a section in a given FFS File and
  copies it to the supplied buffer (not including section header).

  @param  This                       Indicates the calling context.
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
  IN CONST EFI_FIRMWARE_VOLUME2_PROTOCOL   *This,
  IN CONST EFI_GUID                       *NameGuid,
  IN EFI_SECTION_TYPE               SectionType,
  IN UINTN                          SectionInstance,
  IN OUT VOID                       **Buffer,
  IN OUT UINTN                      *BufferSize,
  OUT UINT32                        *AuthenticationStatus
  );

/**
  Writes one or more files to the firmware volume.

  @param  This                   Indicates the calling context.
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
  IN CONST EFI_FIRMWARE_VOLUME2_PROTOCOL   *This,
  IN UINT32                         NumberOfFiles,
  IN EFI_FV_WRITE_POLICY            WritePolicy,
  IN EFI_FV_WRITE_FILE_DATA         *FileData
  );

/**
  Return information of type InformationType for the requested firmware
  volume.

  @param This             Pointer to EFI_FIRMWARE_VOLUME2_PROTOCOL.
  @param InformationType  InformationType for requested.
  @param BufferSize       On input, size of Buffer.On output, the amount of
                          data returned in Buffer.
  @param Buffer           A poniter to the data buffer to return.

  @return EFI_UNSUPPORTED Could not get.

**/
EFI_STATUS
EFIAPI
FvGetVolumeInfo (
  IN  CONST EFI_FIRMWARE_VOLUME2_PROTOCOL       *This,
  IN  CONST EFI_GUID                            *InformationType,
  IN OUT UINTN                                  *BufferSize,
  OUT VOID                                      *Buffer
  );


/**
  Set information with InformationType into the requested firmware volume.

  @param  This             Pointer to EFI_FIRMWARE_VOLUME2_PROTOCOL.
  @param  InformationType  InformationType for requested.
  @param  BufferSize       Size of Buffer data.
  @param  Buffer           A poniter to the data buffer to be set.

  @retval EFI_UNSUPPORTED  Could not set.

**/
EFI_STATUS
EFIAPI
FvSetVolumeInfo (
  IN  CONST EFI_FIRMWARE_VOLUME2_PROTOCOL       *This,
  IN  CONST EFI_GUID                            *InformationType,
  IN  UINTN                                     BufferSize,
  IN CONST  VOID                                *Buffer
  );

/**
  Writes data beginning at Lba:Offset from FV. The write terminates either
  when *NumBytes of data have been written, or when the firmware end is
  reached.  *NumBytes is updated to reflect the actual number of bytes
  written.

  @param FvDevice        Cached Firmware Volume
  @param Offset          Offset in the block at which to begin write
  @param NumBytes        At input, indicates the requested write size.
                         At output, indicates the actual number of bytes written.
  @param Buffer          Buffer containing source data for the write.

  @retval EFI_SUCCESS  Data is successfully written into FV.
  @return error        Data is failed written.

**/
EFI_STATUS
FvcWrite (
  IN     FV_DEVICE                            *FvDevice,
  IN     UINTN                                Offset,
  IN OUT UINTN                                *NumBytes,
  IN     UINT8                                *Buffer
  );


/**
  Check if a block of buffer is erased.

  @param  ErasePolarity  Erase polarity attribute of the firmware volume
  @param  Buffer         The buffer to be checked
  @param  BufferSize     Size of the buffer in bytes

  @retval TRUE           The block of buffer is erased
  @retval FALSE          The block of buffer is not erased

**/
BOOLEAN
IsBufferErased (
  IN UINT8    ErasePolarity,
  IN UINT8    *Buffer,
  IN UINTN    BufferSize
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
  Verify checksum of the firmware volume header.

  @param  FvHeader       Points to the firmware volume header to be checked

  @retval TRUE           Checksum verification passed
  @retval FALSE          Checksum verification failed

**/
BOOLEAN
VerifyFvHeaderChecksum (
  IN EFI_FIRMWARE_VOLUME_HEADER *FvHeader
  );

/**
  Check if it's a valid FFS file header.

  @param  ErasePolarity  Erase polarity attribute of the firmware volume
  @param  FfsHeader      Points to the FFS file header to be checked

  @retval TRUE           Valid FFS file header
  @retval FALSE          Invalid FFS file header

**/
BOOLEAN
IsValidFFSHeader (
  IN UINT8                ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  );

/**
  Check if it's a valid FFS file.
  Here we are sure that it has a valid FFS file header since we must call IsValidFfsHeader() first.

  @param  FvDevice       Cached FV image.
  @param  FfsHeader      Points to the FFS file to be checked

  @retval TRUE           Valid FFS file
  @retval FALSE          Invalid FFS file

**/
BOOLEAN
IsValidFFSFile (
  IN FV_DEVICE            *FvDevice,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  );

/**
  Given the supplied FW_VOL_BLOCK_PROTOCOL, allocate a buffer for output and
  copy the real length volume header into it.

  @param  Fvb                   The FW_VOL_BLOCK_PROTOCOL instance from which to
                                read the volume header
  @param  FwVolHeader           Pointer to pointer to allocated buffer in which
                                the volume header is returned.

  @retval EFI_OUT_OF_RESOURCES  No enough buffer could be allocated.
  @retval EFI_SUCCESS           Successfully read volume header to the allocated
                                buffer.
  @retval EFI_ACCESS_DENIED     Read status of FV is not enabled.
  @retval EFI_INVALID_PARAMETER The FV Header signature is not as expected or
                                the file system could not be understood.
**/
EFI_STATUS
GetFwVolHeader (
  IN     EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL     *Fvb,
  OUT EFI_FIRMWARE_VOLUME_HEADER                **FwVolHeader
  );

/**
  Convert the Buffer Address to LBA Entry Address.

  @param FvDevice        Cached FvDevice
  @param BufferAddress   Address of Buffer
  @param LbaListEntry    Pointer to the got LBA entry that contains the address.

  @retval EFI_NOT_FOUND  Buffer address is out of FvDevice.
  @retval EFI_SUCCESS    LBA entry is found for Buffer address.

**/
EFI_STATUS
Buffer2LbaEntry (
  IN     FV_DEVICE              *FvDevice,
  IN     EFI_PHYSICAL_ADDRESS   BufferAddress,
  OUT LBA_ENTRY                 **LbaListEntry
  );

/**
  Convert the Buffer Address to LBA Address & Offset.

  @param FvDevice        Cached FvDevice
  @param BufferAddress   Address of Buffer
  @param Lba             Pointer to the gob Lba value
  @param Offset          Pointer to the got Offset

  @retval EFI_NOT_FOUND  Buffer address is out of FvDevice.
  @retval EFI_SUCCESS    LBA and Offset is found for Buffer address.

**/
EFI_STATUS
Buffer2Lba (
  IN     FV_DEVICE              *FvDevice,
  IN     EFI_PHYSICAL_ADDRESS   BufferAddress,
  OUT EFI_LBA                   *Lba,
  OUT UINTN                     *Offset
  );

/**
  Set File State in the FfsHeader.

  @param  State          File state to be set into FFS header.
  @param  FfsHeader      Points to the FFS file header

**/
VOID
SetFileState (
  IN UINT8                State,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  );

/**
  Create a PAD File in the Free Space.

  @param FvDevice        Firmware Volume Device.
  @param FreeSpaceEntry  Indicating in which Free Space(Cache) the Pad file will be inserted.
  @param Size            Pad file Size, not include the header.
  @param PadFileEntry    The Ffs File Entry that points to this Pad File.

  @retval EFI_SUCCESS            Successfully create a PAD file.
  @retval EFI_OUT_OF_RESOURCES   No enough free space to create a PAD file.
  @retval EFI_INVALID_PARAMETER  Size is not 8 byte alignment.
  @retval EFI_DEVICE_ERROR       Free space is not erased.
**/
EFI_STATUS
FvCreatePadFileInFreeSpace (
  IN  FV_DEVICE           *FvDevice,
  IN  FREE_SPACE_ENTRY    *FreeSpaceEntry,
  IN  UINTN               Size,
  OUT FFS_FILE_LIST_ENTRY **PadFileEntry
  );

/**
  Create a new file within a PAD file area.

  @param FvDevice        Firmware Volume Device.
  @param FfsFileBuffer   A buffer that holds an FFS file,(it contains a File Header which is in init state).
  @param BufferSize      The size of FfsFileBuffer.
  @param ActualFileSize  The actual file length, it may not be multiples of 8.
  @param FileName        The FFS File Name.
  @param FileType        The FFS File Type.
  @param FileAttributes  The Attributes of the FFS File to be created.

  @retval EFI_SUCCESS           Successfully create a new file within the found PAD file area.
  @retval EFI_OUT_OF_RESOURCES  No suitable PAD file is found.
  @retval other errors          New file is created failed.

**/
EFI_STATUS
FvCreateNewFileInsidePadFile (
  IN  FV_DEVICE               *FvDevice,
  IN  UINT8                   *FfsFileBuffer,
  IN  UINTN                   BufferSize,
  IN  UINTN                   ActualFileSize,
  IN  EFI_GUID                *FileName,
  IN  EFI_FV_FILETYPE         FileType,
  IN  EFI_FV_FILE_ATTRIBUTES  FileAttributes
  );

/**
  Write multiple files into FV in reliable method.

  @param FvDevice        Firmware Volume Device.
  @param NumOfFiles      Total File number to be written.
  @param FileData        The array of EFI_FV_WRITE_FILE_DATA structure,
                         used to get name, attributes, type, etc
  @param FileOperation   The array of operation for each file.

  @retval EFI_SUCCESS            Files are added into FV.
  @retval EFI_OUT_OF_RESOURCES   No enough free PAD files to add the input files.
  @retval EFI_INVALID_PARAMETER  File number is less than or equal to 1.
  @retval EFI_UNSUPPORTED        File number exceeds the supported max numbers of files.

**/
EFI_STATUS
FvCreateMultipleFiles (
  IN  FV_DEVICE               *FvDevice,
  IN  UINTN                   NumOfFiles,
  IN  EFI_FV_WRITE_FILE_DATA  *FileData,
  IN  BOOLEAN                 *FileOperation
  );

/**
  Calculate the checksum for the FFS header.

  @param FfsHeader   FFS File Header which needs to calculate the checksum

**/
VOID
SetHeaderChecksum (
  IN EFI_FFS_FILE_HEADER *FfsHeader
  );

/**
  Calculate the checksum for the FFS File.

  @param FfsHeader       FFS File Header which needs to calculate the checksum
  @param ActualFileSize  The whole Ffs File Length.

**/
VOID
SetFileChecksum (
  IN EFI_FFS_FILE_HEADER *FfsHeader,
  IN UINTN               ActualFileSize
  );

/**
  Get the alignment value from File Attributes.

  @param FfsAttributes  FFS attribute

  @return Alignment value.

**/
UINTN
GetRequiredAlignment (
  IN EFI_FV_FILE_ATTRIBUTES FfsAttributes
  );

/**
  Locate Pad File for writing, this is got from FV Cache.

  @param FvDevice           Cached Firmware Volume.
  @param Size               The required FFS file size.
  @param RequiredAlignment  FFS File Data alignment requirement.
  @param PadSize            Pointer to the size of leading Pad File.
  @param PadFileEntry       Pointer to the Pad File Entry that meets the requirement.

  @retval EFI_SUCCESS     The required pad file is found.
  @retval EFI_NOT_FOUND   The required pad file can't be found.

**/
EFI_STATUS
FvLocatePadFile (
  IN  FV_DEVICE           *FvDevice,
  IN  UINTN               Size,
  IN  UINTN               RequiredAlignment,
  OUT UINTN               *PadSize,
  OUT FFS_FILE_LIST_ENTRY **PadFileEntry
  );

/**
  Locate a suitable pad file for multiple file writing.

  @param FvDevice          Cached Firmware Volume.
  @param NumOfFiles        The number of Files that needed updating
  @param BufferSize        The array of each file size.
  @param RequiredAlignment The array of of FFS File Data alignment requirement.
  @param PadSize           The array of size of each leading Pad File.
  @param TotalSizeNeeded   The totalsize that can hold these files.
  @param PadFileEntry      Pointer to the Pad File Entry that meets the requirement.

  @retval EFI_SUCCESS     The required pad file is found.
  @retval EFI_NOT_FOUND   The required pad file can't be found.

**/
EFI_STATUS
FvSearchSuitablePadFile (
  IN FV_DEVICE              *FvDevice,
  IN UINTN                  NumOfFiles,
  IN UINTN                  *BufferSize,
  IN UINTN                  *RequiredAlignment,
  OUT UINTN                 *PadSize,
  OUT UINTN                 *TotalSizeNeeded,
  OUT FFS_FILE_LIST_ENTRY   **PadFileEntry
  );

/**
  Locate a Free Space entry which can hold these files, including
  meeting the alignment requirements.

  @param FvDevice          Cached Firmware Volume.
  @param NumOfFiles        The number of Files that needed updating
  @param BufferSize        The array of each file size.
  @param RequiredAlignment The array of of FFS File Data alignment requirement.
  @param PadSize           The array of size of each leading Pad File.
  @param TotalSizeNeeded   The got total size that can hold these files.
  @param FreeSpaceEntry    The Free Space Entry that can hold these files.

  @retval EFI_SUCCESS     The free space entry is found.
  @retval EFI_NOT_FOUND   The free space entry can't be found.

**/
EFI_STATUS
FvSearchSuitableFreeSpace (
  IN FV_DEVICE              *FvDevice,
  IN UINTN                  NumOfFiles,
  IN UINTN                  *BufferSize,
  IN UINTN                  *RequiredAlignment,
  OUT UINTN                 *PadSize,
  OUT UINTN                 *TotalSizeNeeded,
  OUT FREE_SPACE_ENTRY      **FreeSpaceEntry
  );

/**
  Change FFS file header state and write to FV.

  @param  FvDevice     Cached FV image.
  @param  FfsHeader    Points to the FFS file header to be updated.
  @param  State        FFS file state to be set.

  @retval EFI_SUCCESS  File state is writen into FV.
  @retval others       File state can't be writen into FV.

**/
EFI_STATUS
UpdateHeaderBit (
  IN FV_DEVICE            *FvDevice,
  IN EFI_FFS_FILE_HEADER  *FfsHeader,
  IN EFI_FFS_FILE_STATE   State
  );

/**
  Convert EFI_FV_FILE_ATTRIBUTES to FFS_FILE_ATTRIBUTES.

  @param FvFileAttrib    The value of EFI_FV_FILE_ATTRIBUTES
  @param FfsFileAttrib   Pointer to the got FFS_FILE_ATTRIBUTES value.

**/
VOID
FvFileAttrib2FfsFileAttrib (
  IN     EFI_FV_FILE_ATTRIBUTES  FvFileAttrib,
  OUT UINT8                      *FfsFileAttrib
  );

#endif
