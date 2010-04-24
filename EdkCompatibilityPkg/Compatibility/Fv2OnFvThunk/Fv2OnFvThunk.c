/** @file
Module produce FV2 on top of FV.

UEFI PI specification supersedes Inte's Framework Specification.
EFI_FIRMWARE_VOLUME_PROTOCOL defined in Intel Framework Pkg is replaced by
EFI_FIRMWARE_VOLUME2_PROTOCOL in MdePkg.
This module produces FV2 on top of FV. This module is used on platform when both of
these two conditions are true:
1) Framework module producing FV is present
2) And the rest of modules on the platform consume FV2

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
Module Name:

**/

#include <PiDxe.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/FirmwareVolume.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>

/**
  
  Because of constraints imposed by the underlying firmware
  storage, an instance of the Firmware Volume Protocol may not
  be to able to support all possible variations of this
  architecture. These constraints and the current state of the
  firmware volume are exposed to the caller using the
  GetVolumeAttributes() function. GetVolumeAttributes() is
  callable only from TPL_NOTIFY and below. Behavior of
  GetVolumeAttributes() at any EFI_TPL above TPL_NOTIFY is
  undefined. Type EFI_TPL is defined in RaiseTPL() in the UEFI
  2.0 specification.
  
  @param  This    Indicates the EFI_FIRMWARE_VOLUME2_PROTOCOL
                  instance.
  
  @param  FvAttributes  Pointer to an EFI_FV_ATTRIBUTES in which
                        the attributes and current settings are
                        returned.


  @retval EFI_SUCCESS   The firmware volume attributes were
                        returned.

**/
EFI_STATUS
EFIAPI
Fv2GetVolumeAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
  OUT       EFI_FV_ATTRIBUTES             *FvAttributes
  );


/**
  The SetVolumeAttributes() function is used to set configurable
  firmware volume attributes. Only EFI_FV_READ_STATUS,
  EFI_FV_WRITE_STATUS, and EFI_FV_LOCK_STATUS may be modified, and
   then only in accordance with the declared capabilities. All
  other bits of FvAttributes are ignored on input. On successful
  return, all bits of *FvAttributes are valid and it contains the
  completed EFI_FV_ATTRIBUTES for the volume. To modify an
  attribute, the corresponding status bit in the EFI_FV_ATTRIBUTES
  is set to the desired value on input. The EFI_FV_LOCK_STATUS bit
  does not affect the ability to read or write the firmware
  volume. Rather, once the EFI_FV_LOCK_STATUS bit is set, it
  prevents further modification to all the attribute bits.
  SetVolumeAttributes() is callable only from TPL_NOTIFY and
  below. Behavior of SetVolumeAttributes() at any EFI_TPL above
  TPL_NOTIFY is undefined. Type EFI_TPL is defined in
  RaiseTPL() in the UEFI 2.0 specification.


  @param  This  Indicates the EFI_FIRMWARE_VOLUME2_PROTOCOL
                instance.
  
  @param  FvAttributes  On input, FvAttributes is a pointer to
                        an EFI_FV_ATTRIBUTES containing the
                        desired firmware volume settings. On
                        successful return, it contains the new
                        settings of the firmware volume. On
                        unsuccessful return, FvAttributes is not
                        modified and the firmware volume
                        settings are not changed.
  
  @retval EFI_SUCCESS   The requested firmware volume attributes
                        were set and the resulting
                        EFI_FV_ATTRIBUTES is returned in
                        FvAttributes.

  @retval EFI_INVALID_PARAMETER FvAttributes:EFI_FV_READ_STATUS
                                is set to 1 on input, but the
                                device does not support enabling
                                reads
                                (FvAttributes:EFI_FV_READ_ENABLE
                                is clear on return from
                                GetVolumeAttributes()). Actual
                                volume attributes are unchanged.

  @retval EFI_INVALID_PARAMETER FvAttributes:EFI_FV_READ_STATUS
                                is cleared to 0 on input, but
                                the device does not support
                                disabling reads
                                (FvAttributes:EFI_FV_READ_DISABL
                                is clear on return from
                                GetVolumeAttributes()). Actual
                                volume attributes are unchanged.

  @retval EFI_INVALID_PARAMETER FvAttributes:EFI_FV_WRITE_STATUS
                                is set to 1 on input, but the
                                device does not support enabling
                                writes
                                (FvAttributes:EFI_FV_WRITE_ENABL
                                is clear on return from
                                GetVolumeAttributes()). Actual
                                volume attributes are unchanged.

  @retval EFI_INVALID_PARAMETER FvAttributes:EFI_FV_WRITE_STATUS
                                is cleared to 0 on input, but
                                the device does not support
                                disabling writes
                                (FvAttributes:EFI_FV_WRITE_DISAB
                                is clear on return from
                                GetVolumeAttributes()). Actual
                                volume attributes are unchanged.

  @retval EFI_INVALID_PARAMETER FvAttributes:EFI_FV_LOCK_STATUS
                                is set on input, but the device
                                does not support locking
                                (FvAttributes:EFI_FV_LOCK_CAP is
                                clear on return from
                                GetVolumeAttributes()). Actual
                                volume attributes are unchanged.

  @retval EFI_ACCESS_DENIED     Device is locked and does not
                                allow attribute modification
                                (FvAttributes:EFI_FV_LOCK_STATUS
                                is set on return from
                                GetVolumeAttributes()). Actual
                                volume attributes are unchanged.

**/
EFI_STATUS
EFIAPI
Fv2SetVolumeAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
  IN OUT    EFI_FV_ATTRIBUTES             *FvAttributes
  );


/**
  ReadFile() is used to retrieve any file from a firmware volume
  during the DXE phase. The actual binary encoding of the file in
  the firmware volume media may be in any arbitrary format as long
  as it does the following: It is accessed using the Firmware
  Volume Protocol. The image that is returned follows the image
  format defined in Code Definitions: PI Firmware File Format.
  If the input value of Buffer==NULL, it indicates the caller is
  requesting only that the type, attributes, and size of the
  file be returned and that there is no output buffer. In this
  case, the following occurs:
  - BufferSize is returned with the size that is required to
    successfully complete the read.
  - The output parameters FoundType and *FileAttributes are
  returned with valid values.
  - The returned value of *AuthenticationStatus is undefined.

  If the input value of Buffer!=NULL, the output buffer is
  specified by a double indirection of the Buffer parameter. The
  input value of *Buffer is used to determine if the output
  buffer is caller allocated or is dynamically allocated by
  ReadFile(). If the input value of *Buffer!=NULL, it indicates
  the output buffer is caller allocated. In this case, the input
   value of *BufferSize indicates the size of the
  caller-allocated output buffer. If the output buffer is not
  large enough to contain the entire requested output, it is
  filled up to the point that the output buffer is exhausted and
  EFI_WARN_BUFFER_TOO_SMALL is returned, and then BufferSize is
   returned with the size required to successfully complete the
  read. All other output parameters are returned with valid
  values. If the input value of *Buffer==NULL, it indicates the
  output buffer is to be allocated by ReadFile(). In this case,
  ReadFile() will allocate an appropriately sized buffer from
  boot services pool memory, which will be returned in Buffer.
  The size of the new buffer is returned in BufferSize and all
  other output parameters are returned with valid values.
  ReadFile() is callable only from TPL_NOTIFY and below.
  Behavior of ReadFile() at any EFI_TPL above TPL_NOTIFY is
  undefined. Type EFI_TPL is defined in RaiseTPL() in the UEFI
  2.0 specification.
  
  @param  This  Indicates the EFI_FIRMWARE_VOLUME2_PROTOCOL
                instance.
  
  @param  NameGuid  Pointer to an EFI_GUID, which is the file
                    name. All firmware file names are EFI_GUIDs.
                    A single firmware volume must not have two
                    valid files with the same file name
                    EFI_GUID.
  
  @param  Buffer  Pointer to a pointer to a buffer in which the
                  file contents are returned, not including the
                  file header.
  @param  BufferSize  Pointer to a caller-allocated UINTN. It
                      indicates the size of the memory
                      represented by Buffer.
  
  @param  FoundType   Pointer to a caller-allocated
                      EFI_FV_FILETYPE.
  
  @param  FileAttributes  Pointer to a  caller-allocated
                          EFI_FV_FILE_ATTRIBUTES.
  
  @param  AuthenticationStatus  Pointer to a caller-allocated
                                UINT32 in which the
                                authentication status is
                                returned.
  
  @retval EFI_SUCCESS   The call completed successfully.
  
  @retval EFI_WARN_BUFFER_TOO_SMALL   The buffer is too small to
                                      contain the requested
                                      output. The buffer is
                                      filled and the output is
                                      truncated.

  @retval EFI_OUT_OF_RESOURCES  An allocation failure occurred.

  @retval EFI_NOT_FOUND   Name was not found in the firmware
                          volume.

  @retval EFI_DEVICE_ERROR  A hardware error occurred when
                            attempting to access the firmware
                            volume.

  @retval EFI_ACCESS_DENIED The firmware volume is configured to
                            isallow reads.

**/
EFI_STATUS
EFIAPI
Fv2ReadFile (
  IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
  IN CONST  EFI_GUID                      *NameGuid,
  IN OUT    VOID                          **Buffer,
  IN OUT    UINTN                         *BufferSize,
  OUT       EFI_FV_FILETYPE               *FoundType,
  OUT       EFI_FV_FILE_ATTRIBUTES        *FileAttributes,
  OUT       UINT32                        *AuthenticationStatus
  );

/**
  ReadSection() is used to retrieve a specific section from a file
  within a firmware volume. The section returned is determined
  using a depth-first, left-to-right search algorithm through all
  sections found in the specified file. The output buffer is 
  specified by a double indirection of the Buffer parameter. 
  The input value of Buffer is used to determine 
  if the output buffer is caller allocated or is
  dynamically allocated by ReadSection(). If the input value of
  Buffer!=NULL, it indicates that the output buffer is caller
  allocated. In this case, the input value of *BufferSize
  indicates the size of the caller-allocated output buffer. If
  the output buffer is not large enough to contain the entire
  requested output, it is filled up to the point that the output
  buffer is exhausted and EFI_WARN_BUFFER_TOO_SMALL is returned,
  and then BufferSize is returned with the size that is required
  to successfully complete the read. All other
  output parameters are returned with valid values. If the input
  value of *Buffer==NULL, it indicates the output buffer is to
  be allocated by ReadSection(). In this case, ReadSection()
  will allocate an appropriately sized buffer from boot services
  pool memory, which will be returned in *Buffer. The size of
  the new buffer is returned in *BufferSize and all other output
  parameters are returned with valid values. ReadSection() is
  callable only from TPL_NOTIFY and below. Behavior of
  ReadSection() at any EFI_TPL above TPL_NOTIFY is
  undefined. Type EFI_TPL is defined in RaiseTPL() in the UEFI
  2.0 specification.


  @param This   Indicates the EFI_FIRMWARE_VOLUME2_PROTOCOL
                instance.
  
  @param NameGuid   Pointer to an EFI_GUID, which indicates the
                    file name from which the requested section
                    will be read.
  
  @param SectionType  Indicates the section type to return.
                      SectionType in conjunction with
                      SectionInstance indicates which section to
                      return.
  
  @param SectionInstance  Indicates which instance of sections
                          with a type of SectionType to return.
                          SectionType in conjunction with
                          SectionInstance indicates which
                          section to return. SectionInstance is
                          zero based.
  
  @param Buffer   Pointer to a pointer to a buffer in which the
                  section contents are returned, not including
                  the section header.
  
  @param BufferSize   Pointer to a caller-allocated UINTN. It
                      indicates the size of the memory
                      represented by Buffer.
  
  @param AuthenticationStatus Pointer to a caller-allocated
                              UINT32 in which the authentication
                              status is returned.
  
  
  @retval EFI_SUCCESS   The call completed successfully.
  
  @retval EFI_WARN_BUFFER_TOO_SMALL   The caller-allocated
                                      buffer is too small to
                                      contain the requested
                                      output. The buffer is
                                      filled and the output is
                                      truncated.
  
  @retval EFI_OUT_OF_RESOURCES  An allocation failure occurred.
  
  @retval EFI_NOT_FOUND   The requested file was not found in
                          the firmware volume. EFI_NOT_FOUND The
                          requested section was not found in the
                          specified file.
  
  @retval EFI_DEVICE_ERROR  A hardware error occurred when
                            attempting to access the firmware
                            volume.
  
  @retval EFI_ACCESS_DENIED The firmware volume is configured to
                            disallow reads. EFI_PROTOCOL_ERROR
                            The requested section was not found,
                            but the file could not be fully
                            parsed because a required
                            GUIDED_SECTION_EXTRACTION_PROTOCOL
                            was not found. It is possible the
                            requested section exists within the
                            file and could be successfully
                            extracted once the required
                            GUIDED_SECTION_EXTRACTION_PROTOCOL
                            is published.

**/
EFI_STATUS
EFIAPI
Fv2ReadSection (
  IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
  IN CONST  EFI_GUID                      *NameGuid,
  IN        EFI_SECTION_TYPE              SectionType,
  IN        UINTN                         SectionInstance,
  IN OUT    VOID                          **Buffer,
  IN OUT    UINTN                         *BufferSize,
  OUT       UINT32                        *AuthenticationStatus
  );

/**
  WriteFile() is used to write one or more files to a firmware
  volume. Each file to be written is described by an
  EFI_FV_WRITE_FILE_DATA structure. The caller must ensure that
  any required alignment for all files listed in the FileData
  array is compatible with the firmware volume. Firmware volume
  capabilities can be determined using the GetVolumeAttributes()
  call. Similarly, if the WritePolicy is set to
  EFI_FV_RELIABLE_WRITE, the caller must check the firmware volume
  capabilities to ensure EFI_FV_RELIABLE_WRITE is supported by the
  firmware volume. EFI_FV_UNRELIABLE_WRITE must always be
  supported. Writing a file with a size of zero
  (FileData[n].BufferSize == 0) deletes the file from the firmware
  volume if it exists. Deleting a file must be done one at a time.
  Deleting a file as part of a multiple file write is not allowed.
  Platform Initialization Specification VOLUME 3 Shared
  Architectural Elements 84 August 21, 2006 Version 1.0
  WriteFile() is callable only from TPL_NOTIFY and below.
  Behavior of WriteFile() at any EFI_TPL above TPL_NOTIFY is
  undefined. Type EFI_TPL is defined in RaiseTPL() in the UEFI 2.0
  specification.

  @param This           Indicates the EFI_FIRMWARE_VOLUME2_PROTOCOL instance. 

  @param NumberOfFiles  Indicates the number of elements in the 
                        array pointed to by FileData.

  @param WritePolicy  Indicates the level of reliability for the
                      write in the event of a power failure or
                      other system failure during the write
                      operation.

  @param FileData   Pointer to an array of
                    EFI_FV_WRITE_FILE_DATA. Each element of
                    FileData[] represents a file to be written.


  @retval EFI_SUCCESS The write completed successfully.
  
  @retval EFI_OUT_OF_RESOURCES  The firmware volume does not
                                have enough free space to
                                storefile(s).
  
  @retval EFI_DEVICE_ERROR  A hardware error occurred when
                            attempting to access the firmware volume.
  
  @retval EFI_WRITE_PROTECTED   The firmware volume is
                                configured to disallow writes.
  
  @retval EFI_NOT_FOUND   A delete was requested, but the
                          requested file was not found in the
                          firmware volume.
  
  @retval EFI_INVALID_PARAMETER   A delete was requested with a
                                  multiple file write.
  
  @retval EFI_INVALID_PARAMETER   An unsupported WritePolicy was
                                  requested.

  @retval EFI_INVALID_PARAMETER   An unknown file type was
                                  specified.

  @retval EFI_INVALID_PARAMETER   A file system specific error
                                  has occurred.
  
**/
EFI_STATUS 
EFIAPI
Fv2WriteFile (
  IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
  IN        UINT32                        NumberOfFiles,
  IN        EFI_FV_WRITE_POLICY           WritePolicy,
  IN        EFI_FV_WRITE_FILE_DATA        *FileData
  );

/**
  GetNextFile() is the interface that is used to search a firmware
  volume for a particular file. It is called successively until
  the desired file is located or the function returns
   EFI_NOT_FOUND. To filter uninteresting files from the output,
  the type of file to search for may be specified in FileType. For
  example, if *FileType is EFI_FV_FILETYPE_DRIVER, only files of
  this type will be returned in the output. If *FileType is
  EFI_FV_FILETYPE_ALL, no filtering of file types is done. The Key
  parameter is used to indicate a starting point of the search. If
  the buffer *Key is completely initialized to zero, the search
  re-initialized and starts at the beginning. Subsequent calls to
  GetNextFile() must maintain the value of *Key returned by the
  immediately previous call. The actual contents of *Key are
  implementation specific and no semantic content is implied.
  GetNextFile() is callable only from TPL_NOTIFY and below.
  Behavior of GetNextFile() at any EFI_TPL above TPL_NOTIFY is
  undefined. Type EFI_TPL is defined in RaiseTPL() in the UEFI 2.0
  specification. Status Codes Returned


  @param This Indicates the EFI_FIRMWARE_VOLUME2_PROTOCOL
              instance. 

  @param Key  Pointer to a caller-allocated buffer
              that contains implementation-specific data that is
              used to track where to begin the search for the
              next file. The size of the buffer must be at least
              This->KeySize bytes long. To re-initialize the
              search and begin from the beginning of the
              firmware volume, the entire buffer must be cleared
              to zero. Other than clearing the buffer to
              initiate a new search, the caller must not modify
              the data in the buffer between calls to
              GetNextFile().

  @param FileType   Pointer to a caller-allocated
                    EFI_FV_FILETYPE. The GetNextFile() API can
                    filter its search for files based on the
                    value of the FileType input. A *FileType
                    input of EFI_FV_FILETYPE_ALL causes
                    GetNextFile() to search for files of all
                    types. If a file is found, the file's type
                    is returned in FileType. *FileType is not
                    modified if no file is found.

  @param NameGuid   Pointer to a caller-allocated EFI_GUID. If a
                    matching file is found, the file's name is
                    returned in NameGuid. If no matching file is
                    found, *NameGuid is not modified.

  @param Attributes Pointer to a caller-allocated
                    EFI_FV_FILE_ATTRIBUTES. If a matching file
                    is found, the file's attributes are returned
                    in Attributes. If no matching file is found,
                    Attributes is not modified. Type
                    EFI_FV_FILE_ATTRIBUTES is defined in
                    ReadFile().

  @param Size   Pointer to a caller-allocated UINTN. If a
                matching file is found, the file's size is
                returned in *Size. If no matching file is found,
                Size is not modified.

  @retval EFI_SUCCESS The output parameters are filled with data
                      obtained from the first matching file that
                      was found.

  @retval FI_NOT_FOUND  No files of type FileType were found.


  @retval EFI_DEVICE_ERROR  A hardware error occurred when
                            attempting to access the firmware
                            volume.

  @retval EFI_ACCESS_DENIED The firmware volume is configured to
                            disallow reads.

   
**/
EFI_STATUS
EFIAPI
Fv2GetNextFile (
  IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
  IN OUT    VOID                          *Key,
  IN OUT    EFI_FV_FILETYPE               *FileType,
  OUT       EFI_GUID                      *NameGuid,
  OUT       EFI_FV_FILE_ATTRIBUTES        *Attributes,
  OUT       UINTN                         *Size
  );

/**
  The GetInfo() function returns information of type
  InformationType for the requested firmware volume. If the volume
  does not support the requested information type, then
  EFI_UNSUPPORTED is returned. If the buffer is not large enough
  to hold the requested structure, EFI_BUFFER_TOO_SMALL is
  returned and the BufferSize is set to the size of buffer that is
  required to make the request. The information types defined by
  this specification are required information types that all file
  systems must support.

  @param This A pointer to the EFI_FIRMWARE_VOLUME2_PROTOCOL
              instance that is the file handle the requested
              information is for.
  
  @param InformationType  The type identifier for the
                          information being requested.
  
  @param BufferSize   On input, the size of Buffer. On output,
                      the amount of data returned in Buffer. In
                      both cases, the size is measured in bytes.
  
  @param Buffer   A pointer to the data buffer to return. The
                  buffer's type is indicated by InformationType.
  
  
  @retval EFI_SUCCESS   The information was retrieved.
  
  @retval EFI_UNSUPPORTED   The InformationType is not known.
  
  @retval EFI_NO_MEDIA  The device has no medium.
  
  @retval EFI_DEVICE_ERROR  The device reported an error.
  
  @retval EFI_VOLUME_CORRUPTED  The file system structures are
                                corrupted.
  
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize is too small to
                                read the current directory
                                entry. BufferSize has been
                                updated with the size needed to
                                complete the request.


**/
EFI_STATUS
EFIAPI
Fv2GetInfo (
  IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
  IN CONST  EFI_GUID                      *InformationType,
  IN OUT    UINTN                         *BufferSize,
  OUT       VOID                          *Buffer
  );

/**

  The SetInfo() function sets information of type InformationType
  on the requested firmware volume.


  @param This   A pointer to the EFI_FIRMWARE_VOLUME2_PROTOCOL
                instance that is the file handle the information
                is for.

  @param InformationType  The type identifier for the
                          information being set.

  @param BufferSize   The size, in bytes, of Buffer.

  @param Buffer A pointer to the data buffer to write. The
                buffer's type is indicated by InformationType.

  @retval EFI_SUCCESS The information was set.

  @retval EFI_UNSUPPORTED The InformationType is not known.

  @retval EFI_NO_MEDIA  The device has no medium.

  @retval EFI_DEVICE_ERROR  The device reported an error.

  @retval EFI_VOLUME_CORRUPTED  The file system structures are
                                corrupted.


  @retval EFI_WRITE_PROTECTED The media is read only.

  @retval EFI_VOLUME_FULL   The volume is full.

  @retval EFI_BAD_BUFFER_SIZE BufferSize is smaller than the
                              size of the type indicated by
                              InformationType.

**/
EFI_STATUS
EFIAPI
Fv2SetInfo (
  IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
  IN CONST  EFI_GUID                      *InformationType,
  IN        UINTN                         BufferSize,
  IN CONST  VOID                          *Buffer
  );

//
//
//
#define FIRMWARE_VOLUME2_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('f', 'v', '2', 't')

typedef struct {
  UINTN                          Signature;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  FirmwareVolume2;
  EFI_FIRMWARE_VOLUME_PROTOCOL   *FirmwareVolume;
} FIRMWARE_VOLUME2_PRIVATE_DATA;

#define FIRMWARE_VOLUME2_PRIVATE_DATA_FROM_THIS(a) CR (a, FIRMWARE_VOLUME2_PRIVATE_DATA, FirmwareVolume2, FIRMWARE_VOLUME2_PRIVATE_DATA_SIGNATURE)

//
// Firmware Volume Protocol template
//
EFI_EVENT  mFv2Registration;

FIRMWARE_VOLUME2_PRIVATE_DATA gFirmwareVolume2PrivateDataTemplate = {
  FIRMWARE_VOLUME2_PRIVATE_DATA_SIGNATURE,
  {
    Fv2GetVolumeAttributes,
    Fv2SetVolumeAttributes,
    Fv2ReadFile,
    Fv2ReadSection,
    Fv2WriteFile,
    Fv2GetNextFile,
    0,
    NULL,
    Fv2GetInfo,
    Fv2SetInfo
  },
  NULL
};

//
// Module globals
//
/**
  This notification function is invoked when an instance of the
  EFI_FIRMWARE_VOLUME_PROTOCOL is produced. It installs another instance of the
  EFI_FIRMWARE_VOLUME2_PROTOCOL on the same handle.

  @param  Event                 The event that occured
  @param  Context               Context of event. Not used in this nofication function.

**/
VOID
EFIAPI
Fv2NotificationEvent (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
{
  EFI_STATUS                     Status;
  UINTN                          BufferSize;
  EFI_HANDLE                     Handle;
  FIRMWARE_VOLUME2_PRIVATE_DATA  *Private;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *FirmwareVolume2;

  while (TRUE) {
    BufferSize = sizeof (Handle);
    Status = gBS->LocateHandle (
                    ByRegisterNotify,
                    &gEfiFirmwareVolumeProtocolGuid,
                    mFv2Registration,
                    &BufferSize,
                    &Handle
                    );
    if (EFI_ERROR (Status)) {
      //
      // Exit Path of While Loop....
      //
      break;
    }

    //
    // Skip this handle if the Firmware Volume Protocol is already installed
    //
    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **)&FirmwareVolume2
                    );
    if (!EFI_ERROR (Status)) {
      continue;
    }

    //
    // Allocate private data structure
    //
    Private = AllocateCopyPool (sizeof (FIRMWARE_VOLUME2_PRIVATE_DATA), &gFirmwareVolume2PrivateDataTemplate);
    if (Private == NULL) {
      continue;
    }

    //
    // Retrieve the Firmware Volume2 Protocol
    //
    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiFirmwareVolumeProtocolGuid,
                    (VOID **)&Private->FirmwareVolume
                    );
    ASSERT_EFI_ERROR (Status);

    //
    // Fill in rest of private data structure
    //
    Private->FirmwareVolume2.KeySize      = Private->FirmwareVolume->KeySize;
    Private->FirmwareVolume2.ParentHandle = Private->FirmwareVolume->ParentHandle;

    //
    // Install Firmware Volume Protocol onto same handle
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Handle,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    &Private->FirmwareVolume2,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }
}


/**
  The user Entry Point for DXE driver. The user code starts with this function
  as the real entry point for the image goes into a library that calls this 
  function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeFirmwareVolume (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EfiCreateProtocolNotifyEvent (
    &gEfiFirmwareVolumeProtocolGuid,
    TPL_CALLBACK,
    Fv2NotificationEvent,
    NULL,
    &mFv2Registration
    );
  return EFI_SUCCESS;
}

/**
  Convert FV attributes defined in Framework Specification
  to FV attributes defined in PI specification.
  
  @param FvAttributes          The FV attributes defined in Framework Specification.
  
  @retval                      The FV attributes defined in PI Specification.
**/
EFI_FV_ATTRIBUTES
FvAttributesToFv2Attributes (
  EFI_FV_ATTRIBUTES FvAttributes
  )
{
  INTN                           Alignment;
  
  Alignment = LowBitSet64 (RShiftU64 (FvAttributes, 16) & 0xffff);
  if (Alignment != -1) {
    Alignment = Alignment << 16;
  } else {
    Alignment = 0;
  }
  FvAttributes = (FvAttributes & 0x1ff) | Alignment;

  return FvAttributes;
}

/**
  
  Because of constraints imposed by the underlying firmware
  storage, an instance of the Firmware Volume Protocol may not
  be to able to support all possible variations of this
  architecture. These constraints and the current state of the
  firmware volume are exposed to the caller using the
  GetVolumeAttributes() function. GetVolumeAttributes() is
  callable only from TPL_NOTIFY and below. Behavior of
  GetVolumeAttributes() at any EFI_TPL above TPL_NOTIFY is
  undefined. Type EFI_TPL is defined in RaiseTPL() in the UEFI
  2.0 specification.
  
  @param  This    Indicates the EFI_FIRMWARE_VOLUME2_PROTOCOL
                  instance.
  
  @param  FvAttributes  Pointer to an EFI_FV_ATTRIBUTES in which
                        the attributes and current settings are
                        returned.


  @retval EFI_SUCCESS   The firmware volume attributes were
                        returned.

**/
EFI_STATUS
EFIAPI
Fv2GetVolumeAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
  OUT       EFI_FV_ATTRIBUTES             *FvAttributes
  )
{
  EFI_STATUS                     Status;
  FIRMWARE_VOLUME2_PRIVATE_DATA  *Private;
  EFI_FIRMWARE_VOLUME_PROTOCOL   *FirmwareVolume;

  Private = FIRMWARE_VOLUME2_PRIVATE_DATA_FROM_THIS (This);
  FirmwareVolume = Private->FirmwareVolume;
  Status = FirmwareVolume->GetVolumeAttributes (
                             FirmwareVolume,
                             (FRAMEWORK_EFI_FV_ATTRIBUTES *)FvAttributes
                             );
  if (!EFI_ERROR (Status)) {
    *FvAttributes = FvAttributesToFv2Attributes (*FvAttributes);
  }
  return Status;
}

/**
  The SetVolumeAttributes() function is used to set configurable
  firmware volume attributes. Only EFI_FV_READ_STATUS,
  EFI_FV_WRITE_STATUS, and EFI_FV_LOCK_STATUS may be modified, and
   then only in accordance with the declared capabilities. All
  other bits of FvAttributes are ignored on input. On successful
  return, all bits of *FvAttributes are valid and it contains the
  completed EFI_FV_ATTRIBUTES for the volume. To modify an
  attribute, the corresponding status bit in the EFI_FV_ATTRIBUTES
  is set to the desired value on input. The EFI_FV_LOCK_STATUS bit
  does not affect the ability to read or write the firmware
  volume. Rather, once the EFI_FV_LOCK_STATUS bit is set, it
  prevents further modification to all the attribute bits.
  SetVolumeAttributes() is callable only from TPL_NOTIFY and
  below. Behavior of SetVolumeAttributes() at any EFI_TPL above
  TPL_NOTIFY is undefined. Type EFI_TPL is defined in
  RaiseTPL() in the UEFI 2.0 specification.


  @param  This  Indicates the EFI_FIRMWARE_VOLUME2_PROTOCOL
                instance.
  
  @param  FvAttributes  On input, FvAttributes is a pointer to
                        an EFI_FV_ATTRIBUTES containing the
                        desired firmware volume settings. On
                        successful return, it contains the new
                        settings of the firmware volume. On
                        unsuccessful return, FvAttributes is not
                        modified and the firmware volume
                        settings are not changed.
  
  @retval EFI_SUCCESS   The requested firmware volume attributes
                        were set and the resulting
                        EFI_FV_ATTRIBUTES is returned in
                        FvAttributes.

  @retval EFI_INVALID_PARAMETER FvAttributes:EFI_FV_READ_STATUS
                                is set to 1 on input, but the
                                device does not support enabling
                                reads
                                (FvAttributes:EFI_FV_READ_ENABLE
                                is clear on return from
                                GetVolumeAttributes()). Actual
                                volume attributes are unchanged.

  @retval EFI_INVALID_PARAMETER FvAttributes:EFI_FV_READ_STATUS
                                is cleared to 0 on input, but
                                the device does not support
                                disabling reads
                                (FvAttributes:EFI_FV_READ_DISABL
                                is clear on return from
                                GetVolumeAttributes()). Actual
                                volume attributes are unchanged.

  @retval EFI_INVALID_PARAMETER FvAttributes:EFI_FV_WRITE_STATUS
                                is set to 1 on input, but the
                                device does not support enabling
                                writes
                                (FvAttributes:EFI_FV_WRITE_ENABL
                                is clear on return from
                                GetVolumeAttributes()). Actual
                                volume attributes are unchanged.

  @retval EFI_INVALID_PARAMETER FvAttributes:EFI_FV_WRITE_STATUS
                                is cleared to 0 on input, but
                                the device does not support
                                disabling writes
                                (FvAttributes:EFI_FV_WRITE_DISAB
                                is clear on return from
                                GetVolumeAttributes()). Actual
                                volume attributes are unchanged.

  @retval EFI_INVALID_PARAMETER FvAttributes:EFI_FV_LOCK_STATUS
                                is set on input, but the device
                                does not support locking
                                (FvAttributes:EFI_FV_LOCK_CAP is
                                clear on return from
                                GetVolumeAttributes()). Actual
                                volume attributes are unchanged.

  @retval EFI_ACCESS_DENIED     Device is locked and does not
                                allow attribute modification
                                (FvAttributes:EFI_FV_LOCK_STATUS
                                is set on return from
                                GetVolumeAttributes()). Actual
                                volume attributes are unchanged.

**/
EFI_STATUS
EFIAPI
Fv2SetVolumeAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
  IN OUT    EFI_FV_ATTRIBUTES             *FvAttributes
  )
{
  FIRMWARE_VOLUME2_PRIVATE_DATA  *Private;
  EFI_FIRMWARE_VOLUME_PROTOCOL   *FirmwareVolume;
  FRAMEWORK_EFI_FV_ATTRIBUTES    FrameworkFvAttributes; 
  EFI_STATUS                     Status;
  UINTN                          Shift;

  if ((*FvAttributes & (EFI_FV2_READ_LOCK_STATUS | EFI_FV2_WRITE_LOCK_STATUS)) != 0) {
    //
    // Framework FV protocol does not support EFI_FV2_READ_LOCK_* | EFI_FV2_WRITE_LOCK_*
    //
    return EFI_INVALID_PARAMETER;
  }

  *FvAttributes = *FvAttributes & (EFI_FV2_READ_STATUS | EFI_FV2_WRITE_STATUS | EFI_FV2_LOCK_STATUS);

  Private = FIRMWARE_VOLUME2_PRIVATE_DATA_FROM_THIS (This);
  FirmwareVolume = Private->FirmwareVolume;

  FrameworkFvAttributes = (*FvAttributes & 0x1ff);
  Shift = (UINTN) RShiftU64(*FvAttributes & EFI_FV2_ALIGNMENT, 16);
  FrameworkFvAttributes = FrameworkFvAttributes | LShiftU64 (EFI_FV_ALIGNMENT_2, Shift);

  Status =  FirmwareVolume->SetVolumeAttributes (
                           FirmwareVolume,
                           &FrameworkFvAttributes
                           );

  if (!EFI_ERROR (Status)) {
    *FvAttributes = FvAttributesToFv2Attributes (FrameworkFvAttributes);
  }

  return Status;
}

/**
  ReadFile() is used to retrieve any file from a firmware volume
  during the DXE phase. The actual binary encoding of the file in
  the firmware volume media may be in any arbitrary format as long
  as it does the following: It is accessed using the Firmware
  Volume Protocol. The image that is returned follows the image
  format defined in Code Definitions: PI Firmware File Format.
  If the input value of Buffer==NULL, it indicates the caller is
  requesting only that the type, attributes, and size of the
  file be returned and that there is no output buffer. In this
  case, the following occurs:
  - BufferSize is returned with the size that is required to
    successfully complete the read.
  - The output parameters FoundType and *FileAttributes are
  returned with valid values.
  - The returned value of *AuthenticationStatus is undefined.

  If the input value of Buffer!=NULL, the output buffer is
  specified by a double indirection of the Buffer parameter. The
  input value of *Buffer is used to determine if the output
  buffer is caller allocated or is dynamically allocated by
  ReadFile(). If the input value of *Buffer!=NULL, it indicates
  the output buffer is caller allocated. In this case, the input
   value of *BufferSize indicates the size of the
  caller-allocated output buffer. If the output buffer is not
  large enough to contain the entire requested output, it is
  filled up to the point that the output buffer is exhausted and
  EFI_WARN_BUFFER_TOO_SMALL is returned, and then BufferSize is
   returned with the size required to successfully complete the
  read. All other output parameters are returned with valid
  values. If the input value of *Buffer==NULL, it indicates the
  output buffer is to be allocated by ReadFile(). In this case,
  ReadFile() will allocate an appropriately sized buffer from
  boot services pool memory, which will be returned in Buffer.
  The size of the new buffer is returned in BufferSize and all
  other output parameters are returned with valid values.
  ReadFile() is callable only from TPL_NOTIFY and below.
  Behavior of ReadFile() at any EFI_TPL above TPL_NOTIFY is
  undefined. Type EFI_TPL is defined in RaiseTPL() in the UEFI
  2.0 specification.
  
  @param  This  Indicates the EFI_FIRMWARE_VOLUME2_PROTOCOL
                instance.
  
  @param  NameGuid  Pointer to an EFI_GUID, which is the file
                    name. All firmware file names are EFI_GUIDs.
                    A single firmware volume must not have two
                    valid files with the same file name
                    EFI_GUID.
  
  @param  Buffer  Pointer to a pointer to a buffer in which the
                  file contents are returned, not including the
                  file header.
  @param  BufferSize  Pointer to a caller-allocated UINTN. It
                      indicates the size of the memory
                      represented by Buffer.
  
  @param  FoundType   Pointer to a caller-allocated
                      EFI_FV_FILETYPE.
  
  @param  FileAttributes  Pointer to a  caller-allocated
                          EFI_FV_FILE_ATTRIBUTES.
  
  @param  AuthenticationStatus  Pointer to a caller-allocated
                                UINT32 in which the
                                authentication status is
                                returned.
  
  @retval EFI_SUCCESS   The call completed successfully.
  
  @retval EFI_WARN_BUFFER_TOO_SMALL   The buffer is too small to
                                      contain the requested
                                      output. The buffer is
                                      filled and the output is
                                      truncated.

  @retval EFI_OUT_OF_RESOURCES  An allocation failure occurred.

  @retval EFI_NOT_FOUND   Name was not found in the firmware
                          volume.

  @retval EFI_DEVICE_ERROR  A hardware error occurred when
                            attempting to access the firmware
                            volume.

  @retval EFI_ACCESS_DENIED The firmware volume is configured to
                            isallow reads.

**/
EFI_STATUS
EFIAPI
Fv2ReadFile (
  IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
  IN CONST  EFI_GUID                      *NameGuid,
  IN OUT    VOID                          **Buffer,
  IN OUT    UINTN                         *BufferSize,
  OUT       EFI_FV_FILETYPE               *FoundType,
  OUT       EFI_FV_FILE_ATTRIBUTES        *FileAttributes,
  OUT       UINT32                        *AuthenticationStatus
  )
{
  FIRMWARE_VOLUME2_PRIVATE_DATA  *Private;
  EFI_FIRMWARE_VOLUME_PROTOCOL   *FirmwareVolume;

  Private = FIRMWARE_VOLUME2_PRIVATE_DATA_FROM_THIS (This);
  FirmwareVolume = Private->FirmwareVolume;

  return FirmwareVolume->ReadFile (
                           FirmwareVolume,
                           (EFI_GUID *)NameGuid,
                           Buffer,
                           BufferSize,
                           FoundType,
                           FileAttributes,
                           AuthenticationStatus
                           );
}

/**
  ReadSection() is used to retrieve a specific section from a file
  within a firmware volume. The section returned is determined
  using a depth-first, left-to-right search algorithm through all
  sections found in the specified file.The output buffer is specified 
  by a double indirection of the Buffer parameter. The input value of Buffer 
  is used to determine if the output buffer is caller allocated or is
  dynamically allocated by ReadSection(). If the input value of
  Buffer!=NULL, it indicates that the output buffer is caller
  allocated. In this case, the input value of *BufferSize
  indicates the size of the caller-allocated output buffer. If
  the output buffer is not large enough to contain the entire
  requested output, it is filled up to the point that the output
  buffer is exhausted and EFI_WARN_BUFFER_TOO_SMALL is returned,
  and then BufferSize is returned with the size that is required
  to successfully complete the read. All other
  output parameters are returned with valid values. If the input
  value of *Buffer==NULL, it indicates the output buffer is to
  be allocated by ReadSection(). In this case, ReadSection()
  will allocate an appropriately sized buffer from boot services
  pool memory, which will be returned in *Buffer. The size of
  the new buffer is returned in *BufferSize and all other output
  parameters are returned with valid values. ReadSection() is
  callable only from TPL_NOTIFY and below. Behavior of
  ReadSection() at any EFI_TPL above TPL_NOTIFY is
  undefined. Type EFI_TPL is defined in RaiseTPL() in the UEFI
  2.0 specification.


  @param This   Indicates the EFI_FIRMWARE_VOLUME2_PROTOCOL
                instance.
  
  @param NameGuid   Pointer to an EFI_GUID, which indicates the
                    file name from which the requested section
                    will be read.
  
  @param SectionType  Indicates the section type to return.
                      SectionType in conjunction with
                      SectionInstance indicates which section to
                      return.
  
  @param SectionInstance  Indicates which instance of sections
                          with a type of SectionType to return.
                          SectionType in conjunction with
                          SectionInstance indicates which
                          section to return. SectionInstance is
                          zero based.
  
  @param Buffer   Pointer to a pointer to a buffer in which the
                  section contents are returned, not including
                  the section header.
  
  @param BufferSize   Pointer to a caller-allocated UINTN. It
                      indicates the size of the memory
                      represented by Buffer.
  
  @param AuthenticationStatus Pointer to a caller-allocated
                              UINT32 in which the authentication
                              status is returned.
  
  
  @retval EFI_SUCCESS   The call completed successfully.
  
  @retval EFI_WARN_BUFFER_TOO_SMALL   The caller-allocated
                                      buffer is too small to
                                      contain the requested
                                      output. The buffer is
                                      filled and the output is
                                      truncated.
  
  @retval EFI_OUT_OF_RESOURCES  An allocation failure occurred.
  
  @retval EFI_NOT_FOUND   The requested file was not found in
                          the firmware volume. EFI_NOT_FOUND The
                          requested section was not found in the
                          specified file.
  
  @retval EFI_DEVICE_ERROR  A hardware error occurred when
                            attempting to access the firmware
                            volume.
  
  @retval EFI_ACCESS_DENIED The firmware volume is configured to
                            disallow reads. EFI_PROTOCOL_ERROR
                            The requested section was not found,
                            but the file could not be fully
                            parsed because a required
                            GUIDED_SECTION_EXTRACTION_PROTOCOL
                            was not found. It is possible the
                            requested section exists within the
                            file and could be successfully
                            extracted once the required
                            GUIDED_SECTION_EXTRACTION_PROTOCOL
                            is published.

**/
EFI_STATUS
EFIAPI
Fv2ReadSection (
  IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
  IN CONST  EFI_GUID                      *NameGuid,
  IN        EFI_SECTION_TYPE              SectionType,
  IN        UINTN                         SectionInstance,
  IN OUT    VOID                          **Buffer,
  IN OUT    UINTN                         *BufferSize,
  OUT       UINT32                        *AuthenticationStatus
  )
{
  FIRMWARE_VOLUME2_PRIVATE_DATA  *Private;
  EFI_FIRMWARE_VOLUME_PROTOCOL   *FirmwareVolume;

  Private = FIRMWARE_VOLUME2_PRIVATE_DATA_FROM_THIS (This);
  FirmwareVolume = Private->FirmwareVolume;

  return FirmwareVolume->ReadSection (
                           FirmwareVolume,
                           (EFI_GUID *)NameGuid,
                           SectionType,
                           SectionInstance,
                           Buffer,
                           BufferSize,
                           AuthenticationStatus
                           );
}

/**
  WriteFile() is used to write one or more files to a firmware
  volume. Each file to be written is described by an
  EFI_FV_WRITE_FILE_DATA structure. The caller must ensure that
  any required alignment for all files listed in the FileData
  array is compatible with the firmware volume. Firmware volume
  capabilities can be determined using the GetVolumeAttributes()
  call. Similarly, if the WritePolicy is set to
  EFI_FV_RELIABLE_WRITE, the caller must check the firmware volume
  capabilities to ensure EFI_FV_RELIABLE_WRITE is supported by the
  firmware volume. EFI_FV_UNRELIABLE_WRITE must always be
  supported. Writing a file with a size of zero
  (FileData[n].BufferSize == 0) deletes the file from the firmware
  volume if it exists. Deleting a file must be done one at a time.
  Deleting a file as part of a multiple file write is not allowed.
  Platform Initialization Specification VOLUME 3 Shared
  Architectural Elements 84 August 21, 2006 Version 1.0
  WriteFile() is callable only from TPL_NOTIFY and below.
  Behavior of WriteFile() at any EFI_TPL above TPL_NOTIFY is
  undefined. Type EFI_TPL is defined in RaiseTPL() in the UEFI 2.0
  specification.

  @param This           Indicates the EFI_FIRMWARE_VOLUME2_PROTOCOL instance. 

  @param NumberOfFiles  Indicates the number of
                        elements in the array pointed to by FileData.


  @param WritePolicy  Indicates the level of reliability for the
                      write in the event of a power failure or
                      other system failure during the write
                      operation.

  @param FileData   Pointer to an array of
                    EFI_FV_WRITE_FILE_DATA. Each element of
                    FileData[] represents a file to be written.


  @retval EFI_SUCCESS The write completed successfully.
  
  @retval EFI_OUT_OF_RESOURCES  The firmware volume does not
                                have enough free space to
                                storefile(s).
  
  @retval EFI_DEVICE_ERROR  A hardware error occurred when
                            attempting to access the firmware volume.
  
  @retval EFI_WRITE_PROTECTED   The firmware volume is
                                configured to disallow writes.
  
  @retval EFI_NOT_FOUND   A delete was requested, but the
                          requested file was not found in the
                          firmware volume.
  
  @retval EFI_INVALID_PARAMETER   A delete was requested with a
                                  multiple file write.
  
  @retval EFI_INVALID_PARAMETER   An unsupported WritePolicy was
                                  requested.

  @retval EFI_INVALID_PARAMETER   An unknown file type was
                                  specified.

  @retval EFI_INVALID_PARAMETER   A file system specific error
                                  has occurred.
  
**/
EFI_STATUS 
EFIAPI
Fv2WriteFile (
  IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
  IN        UINT32                        NumberOfFiles,
  IN        EFI_FV_WRITE_POLICY           WritePolicy,
  IN        EFI_FV_WRITE_FILE_DATA        *FileData
  )
{
  FIRMWARE_VOLUME2_PRIVATE_DATA  *Private;
  EFI_FIRMWARE_VOLUME_PROTOCOL   *FirmwareVolume;

  Private = FIRMWARE_VOLUME2_PRIVATE_DATA_FROM_THIS (This);
  FirmwareVolume = Private->FirmwareVolume;

  return FirmwareVolume->WriteFile (
                           FirmwareVolume,
                           NumberOfFiles,
                           WritePolicy,
                           (FRAMEWORK_EFI_FV_WRITE_FILE_DATA *)FileData
                           );
}

/**
  GetNextFile() is the interface that is used to search a firmware
  volume for a particular file. It is called successively until
  the desired file is located or the function returns
   EFI_NOT_FOUND. To filter uninteresting files from the output,
  the type of file to search for may be specified in FileType. For
  example, if *FileType is EFI_FV_FILETYPE_DRIVER, only files of
  this type will be returned in the output. If *FileType is
  EFI_FV_FILETYPE_ALL, no filtering of file types is done. The Key
  parameter is used to indicate a starting point of the search. If
  the buffer *Key is completely initialized to zero, the search
  re-initialized and starts at the beginning. Subsequent calls to
  GetNextFile() must maintain the value of *Key returned by the
  immediately previous call. The actual contents of *Key are
  implementation specific and no semantic content is implied.
  GetNextFile() is callable only from TPL_NOTIFY and below.
  Behavior of GetNextFile() at any EFI_TPL above TPL_NOTIFY is
  undefined. Type EFI_TPL is defined in RaiseTPL() in the UEFI 2.0
  specification. Status Codes Returned


  @param This Indicates the EFI_FIRMWARE_VOLUME2_PROTOCOL
              instance. 

  @param Key  Pointer to a caller-allocated buffer
              that contains implementation-specific data that is
              used to track where to begin the search for the
              next file. The size of the buffer must be at least
              This->KeySize bytes long. To re-initialize the
              search and begin from the beginning of the
              firmware volume, the entire buffer must be cleared
              to zero. Other than clearing the buffer to
              initiate a new search, the caller must not modify
              the data in the buffer between calls to
              GetNextFile().

  @param FileType   Pointer to a caller-allocated
                    EFI_FV_FILETYPE. The GetNextFile() API can
                    filter its search for files based on the
                    value of the FileType input. A *FileType
                    input of EFI_FV_FILETYPE_ALL causes
                    GetNextFile() to search for files of all
                    types. If a file is found, the file's type
                    is returned in FileType. *FileType is not
                    modified if no file is found.

  @param NameGuid   Pointer to a caller-allocated EFI_GUID. If a
                    matching file is found, the file's name is
                    returned in NameGuid. If no matching file is
                    found, *NameGuid is not modified.

  @param Attributes Pointer to a caller-allocated
                    EFI_FV_FILE_ATTRIBUTES. If a matching file
                    is found, the file's attributes are returned
                    in Attributes. If no matching file is found,
                    Attributes is not modified. Type
                    EFI_FV_FILE_ATTRIBUTES is defined in
                    ReadFile().

  @param Size   Pointer to a caller-allocated UINTN. If a
                matching file is found, the file's size is
                returned in *Size. If no matching file is found,
                Size is not modified.

  @retval EFI_SUCCESS The output parameters are filled with data
                      obtained from the first matching file that
                      was found.

  @retval FI_NOT_FOUND  No files of type FileType were found.


  @retval EFI_DEVICE_ERROR  A hardware error occurred when
                            attempting to access the firmware
                            volume.

  @retval EFI_ACCESS_DENIED The firmware volume is configured to
                            disallow reads.

   
**/
EFI_STATUS
EFIAPI
Fv2GetNextFile (
  IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
  IN OUT    VOID                          *Key,
  IN OUT    EFI_FV_FILETYPE               *FileType,
  OUT       EFI_GUID                      *NameGuid,
  OUT       EFI_FV_FILE_ATTRIBUTES        *Attributes,
  OUT       UINTN                         *Size
  )
{
  FIRMWARE_VOLUME2_PRIVATE_DATA  *Private;
  EFI_FIRMWARE_VOLUME_PROTOCOL   *FirmwareVolume;

  Private = FIRMWARE_VOLUME2_PRIVATE_DATA_FROM_THIS (This);
  FirmwareVolume = Private->FirmwareVolume;

  return FirmwareVolume->GetNextFile (
                           FirmwareVolume,
                           Key,
                           FileType,
                           NameGuid,
                           Attributes,
                           Size
                           );
}

/**
  The GetInfo() function returns information of type
  InformationType for the requested firmware volume. If the volume
  does not support the requested information type, then
  EFI_UNSUPPORTED is returned. If the buffer is not large enough
  to hold the requested structure, EFI_BUFFER_TOO_SMALL is
  returned and the BufferSize is set to the size of buffer that is
  required to make the request. The information types defined by
  this specification are required information types that all file
  systems must support.

  @param This A pointer to the EFI_FIRMWARE_VOLUME2_PROTOCOL
              instance that is the file handle the requested
              information is for.
  
  @param InformationType  The type identifier for the
                          information being requested.
  
  @param BufferSize   On input, the size of Buffer. On output,
                      the amount of data returned in Buffer. In
                      both cases, the size is measured in bytes.
  
  @param Buffer   A pointer to the data buffer to return. The
                  buffer's type is indicated by InformationType.
  
  
  @retval EFI_SUCCESS   The information was retrieved.
  
  @retval EFI_UNSUPPORTED   The InformationType is not known.
  
  @retval EFI_NO_MEDIA  The device has no medium.
  
  @retval EFI_DEVICE_ERROR  The device reported an error.
  
  @retval EFI_VOLUME_CORRUPTED  The file system structures are
                                corrupted.
  
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize is too small to
                                read the current directory
                                entry. BufferSize has been
                                updated with the size needed to
                                complete the request.


**/
EFI_STATUS
EFIAPI
Fv2GetInfo (
  IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
  IN CONST  EFI_GUID                      *InformationType,
  IN OUT    UINTN                         *BufferSize,
  OUT       VOID                          *Buffer
  )
{
  return EFI_UNSUPPORTED;
}

/**

  The SetInfo() function sets information of type InformationType
  on the requested firmware volume.


  @param This   A pointer to the EFI_FIRMWARE_VOLUME2_PROTOCOL
                instance that is the file handle the information
                is for.

  @param InformationType  The type identifier for the
                          information being set.

  @param BufferSize   The size, in bytes, of Buffer.

  @param Buffer A pointer to the data buffer to write. The
                buffer's type is indicated by InformationType.

  @retval EFI_SUCCESS The information was set.

  @retval EFI_UNSUPPORTED The InformationType is not known.

  @retval EFI_NO_MEDIA  The device has no medium.

  @retval EFI_DEVICE_ERROR  The device reported an error.

  @retval EFI_VOLUME_CORRUPTED  The file system structures are
                                corrupted.


  @retval EFI_WRITE_PROTECTED The media is read only.

  @retval EFI_VOLUME_FULL   The volume is full.

  @retval EFI_BAD_BUFFER_SIZE BufferSize is smaller than the
                              size of the type indicated by
                              InformationType.

**/
EFI_STATUS
EFIAPI
Fv2SetInfo (
  IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
  IN CONST  EFI_GUID                      *InformationType,
  IN        UINTN                         BufferSize,
  IN CONST  VOID                          *Buffer
  )
{
  return EFI_UNSUPPORTED;
}
