/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FirmwareVolume.h

Abstract:

  Firmware Volume protocol as defined in the Tiano Firmware Volume
  specification.

--*/

#ifndef _FW_VOL_H_
#define _FW_VOL_H_

//
// Statements that include other files
//
#include "EfiFirmwareVolumeHeader.h"
#include "EfiFirmwareFileSystem.h"
#include "EfiFirmwareVolume.h"
//
// Firmware Volume Protocol GUID definition
//
#define EFI_FIRMWARE_VOLUME_PROTOCOL_GUID \
  { \
    0x389F751F, 0x1838, 0x4388, {0x83, 0x90, 0xCD, 0x81, 0x54, 0xBD, 0x27, 0xF8} \
  }


//
// ************************************************************
// EFI_FV_ATTRIBUTES bit definitions
// ************************************************************
//
#define EFI_FV_READ_DISABLE_CAP       0x0000000000000001
#define EFI_FV_READ_ENABLE_CAP        0x0000000000000002
#define EFI_FV_READ_STATUS            0x0000000000000004

#define EFI_FV_WRITE_DISABLE_CAP      0x0000000000000008
#define EFI_FV_WRITE_ENABLE_CAP       0x0000000000000010
#define EFI_FV_WRITE_STATUS           0x0000000000000020

#define EFI_FV_LOCK_CAP               0x0000000000000040
#define EFI_FV_LOCK_STATUS            0x0000000000000080
#define EFI_FV_WRITE_POLICY_RELIABLE  0x0000000000000100

#define EFI_FV_ALIGNMENT_CAP          0x0000000000008000
#define EFI_FV_ALIGNMENT_2            0x0000000000010000
#define EFI_FV_ALIGNMENT_4            0x0000000000020000
#define EFI_FV_ALIGNMENT_8            0x0000000000040000
#define EFI_FV_ALIGNMENT_16           0x0000000000080000
#define EFI_FV_ALIGNMENT_32           0x0000000000100000
#define EFI_FV_ALIGNMENT_64           0x0000000000200000
#define EFI_FV_ALIGNMENT_128          0x0000000000400000
#define EFI_FV_ALIGNMENT_256          0x0000000000800000
#define EFI_FV_ALIGNMENT_512          0x0000000001000000
#define EFI_FV_ALIGNMENT_1K           0x0000000002000000
#define EFI_FV_ALIGNMENT_2K           0x0000000004000000
#define EFI_FV_ALIGNMENT_4K           0x0000000008000000
#define EFI_FV_ALIGNMENT_8K           0x0000000010000000
#define EFI_FV_ALIGNMENT_16K          0x0000000020000000
#define EFI_FV_ALIGNMENT_32K          0x0000000040000000
#define EFI_FV_ALIGNMENT_64K          0x0000000080000000

//
// Protocol API definitions
//
//
// Forward declaration of protocol data structure
//
typedef struct _EFI_FIRMWARE_VOLUME_PROTOCOL  EFI_FIRMWARE_VOLUME_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *FV_GET_ATTRIBUTES) (
  IN  EFI_FIRMWARE_VOLUME_PROTOCOL  * This,
  OUT EFI_FV_ATTRIBUTES             * Attributes
  );

/*++

Routine Description:
  Retrieves attributes, insures positive polarity of attribute bits, returns
  resulting attributes in output parameter

Arguments:
  This - Calling context
  Attributes - output buffer which contains attributes

Returns:
  EFI_INVALID_PARAMETER
  EFI_SUCCESS

--*/
typedef
EFI_STATUS
(EFIAPI *FV_SET_ATTRIBUTES) (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   * This,
  IN OUT EFI_FV_ATTRIBUTES          * Attributes
  );

/*++

Routine Description:
  Sets volume attributes

Arguments:
  This          Calling context
  Attributes    Buffer which contains attributes

Returns:
  EFI_INVALID_PARAMETER
  EFI_DEVICE_ERROR
  EFI_SUCCESS

--*/

typedef
EFI_STATUS
(EFIAPI *FV_READ_FILE) (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   * This,
  IN EFI_GUID                       * NameGuid,
  IN OUT VOID                       **Buffer,
  IN OUT UINTN                      *BufferSize,
  OUT EFI_FV_FILETYPE               * FoundType,
  OUT EFI_FV_FILE_ATTRIBUTES        * FileAttributes,
  OUT UINT32                        *AuthenticationStatus
  );

/*++

Routine Description:
    Read the requested file (NameGuid) and returns data in Buffer.

Arguments:
  This - Calling context
  NameGuid - Filename identifying which file to read 
  Buffer - Pointer to pointer to buffer in which contents of file are returned.
          
          If Buffer is NULL, only type, attributes, and size are returned as
          there is no output buffer.
          
          If Buffer != NULL and *Buffer == NULL, the output buffer is allocated
          from BS pool by ReadFile
          
          If Buffer != NULL and *Buffer != NULL, the output buffer has been
          allocated by the caller and is being passed in.
          
  BufferSize - Indicates the buffer size passed in, and on output the size
          required to complete the read
  FoundType - Indicates the type of the file who's data is returned
  FileAttributes - Indicates the attributes of the file who's data is resturned
  AuthenticationStatus - Indicates the authentication status of the data

Returns:
  EFI_SUCCESS
  EFI_WARN_BUFFER_TOO_SMALL
  EFI_NOT_FOUND
  EFI_DEVICE_ERROR
  EFI_ACCESS_DENIED
  
--*/
typedef
EFI_STATUS
(EFIAPI *FV_READ_SECTION) (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   * This,
  IN EFI_GUID                       * NameGuid,
  IN EFI_SECTION_TYPE               SectionType,
  IN UINTN                          SectionInstance,
  IN OUT VOID                       **Buffer,
  IN OUT UINTN                      *BufferSize,
  OUT UINT32                        *AuthenticationStatus
  );

/*++

Routine Description:
    Read the requested section from the specified file and returns data in Buffer.

Arguments:
  This - Calling context
  NameGuid - Filename identifying the file from which to read 
  SectionType - Indicates what section type to retrieve
  SectionInstance - Indicates which instance of SectionType to retrieve
  Buffer - Pointer to pointer to buffer in which contents of file are returned.
          
          If Buffer is NULL, only type, attributes, and size are returned as
          there is no output buffer.
          
          If Buffer != NULL and *Buffer == NULL, the output buffer is allocated
          from BS pool by ReadFile
          
          If Buffer != NULL and *Buffer != NULL, the output buffer has been
          allocated by the caller and is being passed in.
          
  BufferSize - Indicates the buffer size passed in, and on output the size
          required to complete the read
  AuthenticationStatus - Indicates the authentication status of the data

Returns:
  EFI_SUCCESS
  EFI_WARN_BUFFER_TOO_SMALL
  EFI_OUT_OF_RESOURCES
  EFI_NOT_FOUND
  EFI_DEVICE_ERROR
  EFI_ACCESS_DENIED
  
--*/

typedef
EFI_STATUS
(EFIAPI *FV_WRITE_FILE) (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   * This,
  IN UINT32                         NumberOfFiles,
  IN EFI_FV_WRITE_POLICY            WritePolicy,
  IN EFI_FV_WRITE_FILE_DATA         * FileData
  );

/*++

Routine Description:
  Write the supplied file (NameGuid) to the FV.

Arguments:
  This - Calling context
  NumberOfFiles - Indicates the number of file records pointed to by FileData
  WritePolicy - Indicates the level of reliability of the write with respect to
          things like power failure events.
  FileData - A pointer to an array of EFI_FV_WRITE_FILE_DATA structures.  Each
          element in the array indicates a file to write, and there are
          NumberOfFiles elements in the input array.

Returns:
  EFI_SUCCESS
  EFI_OUT_OF_RESOURCES
  EFI_DEVICE_ERROR
  EFI_WRITE_PROTECTED
  EFI_NOT_FOUND
  EFI_INVALID_PARAMETER
  
--*/
typedef
EFI_STATUS
(EFIAPI *FV_GET_NEXT_FILE) (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   * This,
  IN OUT VOID                       *Key,
  IN OUT EFI_FV_FILETYPE            * FileType,
  OUT EFI_GUID                      * NameGuid,
  OUT EFI_FV_FILE_ATTRIBUTES        * Attributes,
  OUT UINTN                         *Size
  );

/*++

Routine Description:
  Given the input key, search for the next matching file in the volume.

Arguments:
  This - Calling context
  Key - Pointer to a caller allocated buffer that contains an implementation
        specific key that is used to track where to begin searching on
        successive calls.
  FileType - Indicates the file type to filter for
  NameGuid - Guid filename of the file found
  Attributes - Attributes of the file found
  Size - Size in bytes of the file found

Returns:
  EFI_SUCCESS
  EFI_NOT_FOUND
  EFI_DEVICE_ERROR
  EFI_ACCESS_DENIED

--*/
struct _EFI_FIRMWARE_VOLUME_PROTOCOL {
  FV_GET_ATTRIBUTES GetVolumeAttributes;
  FV_SET_ATTRIBUTES SetVolumeAttributes;
  FV_READ_FILE      ReadFile;
  FV_READ_SECTION   ReadSection;
  FV_WRITE_FILE     WriteFile;
  FV_GET_NEXT_FILE  GetNextFile;
  UINT32            KeySize;
  EFI_HANDLE        ParentHandle;
};

extern EFI_GUID gEfiFirmwareVolumeProtocolGuid;

#endif
