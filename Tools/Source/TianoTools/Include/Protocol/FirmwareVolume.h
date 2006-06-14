/** @file
  This file declares Firmware Volume protocol.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  FirmwareVolume.h

  @par Revision Reference:
  This protocol is defined in Firmware Volume specification.
  Version 0.9

**/

#ifndef __FIRMWARE_VOLUME_H__
#define __FIRMWARE_VOLUME_H__


//
// Firmware Volume Protocol GUID definition
//
#define EFI_FIRMWARE_VOLUME_PROTOCOL_GUID \
  { \
    0x389F751F, 0x1838, 0x4388, {0x83, 0x90, 0xCD, 0x81, 0x54, 0xBD, 0x27, 0xF8 } \
  }

#define FV_DEVICE_SIGNATURE EFI_SIGNATURE_32 ('_', 'F', 'V', '_')

typedef struct _EFI_FIRMWARE_VOLUME_PROTOCOL  EFI_FIRMWARE_VOLUME_PROTOCOL;

//
// EFI_FV_ATTRIBUTES bit definitions
//
typedef UINT64  EFI_FV_ATTRIBUTES;

//
// ************************************************************
// EFI_FV_ATTRIBUTES bit definitions
// ************************************************************
//
#define EFI_FV_READ_DISABLE_CAP       0x0000000000000001ULL
#define EFI_FV_READ_ENABLE_CAP        0x0000000000000002ULL
#define EFI_FV_READ_STATUS            0x0000000000000004ULL

#define EFI_FV_WRITE_DISABLE_CAP      0x0000000000000008ULL
#define EFI_FV_WRITE_ENABLE_CAP       0x0000000000000010ULL
#define EFI_FV_WRITE_STATUS           0x0000000000000020ULL

#define EFI_FV_LOCK_CAP               0x0000000000000040ULL
#define EFI_FV_LOCK_STATUS            0x0000000000000080ULL
#define EFI_FV_WRITE_POLICY_RELIABLE  0x0000000000000100ULL

#define EFI_FV_ALIGNMENT_CAP          0x0000000000008000ULL
#define EFI_FV_ALIGNMENT_2            0x0000000000010000ULL
#define EFI_FV_ALIGNMENT_4            0x0000000000020000ULL
#define EFI_FV_ALIGNMENT_8            0x0000000000040000ULL
#define EFI_FV_ALIGNMENT_16           0x0000000000080000ULL
#define EFI_FV_ALIGNMENT_32           0x0000000000100000ULL
#define EFI_FV_ALIGNMENT_64           0x0000000000200000ULL
#define EFI_FV_ALIGNMENT_128          0x0000000000400000ULL
#define EFI_FV_ALIGNMENT_256          0x0000000000800000ULL
#define EFI_FV_ALIGNMENT_512          0x0000000001000000ULL
#define EFI_FV_ALIGNMENT_1K           0x0000000002000000ULL
#define EFI_FV_ALIGNMENT_2K           0x0000000004000000ULL
#define EFI_FV_ALIGNMENT_4K           0x0000000008000000ULL
#define EFI_FV_ALIGNMENT_8K           0x0000000010000000ULL
#define EFI_FV_ALIGNMENT_16K          0x0000000020000000ULL
#define EFI_FV_ALIGNMENT_32K          0x0000000040000000ULL
#define EFI_FV_ALIGNMENT_64K          0x0000000080000000ULL

//
// Protocol API definitions
//

/**
  Retrieves attributes, insures positive polarity of attribute bits, returns
  resulting attributes in output parameter

  @param  This Calling context
  @param  Attributes output buffer which contains attributes

  @retval EFI_INVALID_PARAMETER
  @retval EFI_SUCCESS

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FV_GET_ATTRIBUTES) (
  IN  EFI_FIRMWARE_VOLUME_PROTOCOL  *This,
  OUT EFI_FV_ATTRIBUTES             *Attributes
  );

/**
  Sets volume attributes

  @param  This Calling context
  @param  Attributes Buffer which contains attributes

  @retval EFI_INVALID_PARAMETER
  @retval EFI_DEVICE_ERROR
  @retval EFI_SUCCESS

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FV_SET_ATTRIBUTES) (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN OUT EFI_FV_ATTRIBUTES          *Attributes
  );

typedef UINT32  EFI_FV_FILE_ATTRIBUTES;

#define EFI_FV_FILE_ATTRIB_ALIGNMENT  0x0000001F

/**
  Read the requested file (NameGuid) and returns data in Buffer.

  @param  This Calling context
  @param  NameGuid Filename identifying which file to read
  @param  Buffer Pointer to pointer to buffer in which contents of file are returned.
  <br>
  If Buffer is NULL, only type, attributes, and size are returned as
  there is no output buffer.
  <br>
  If Buffer != NULL and *Buffer == NULL, the output buffer is allocated
  from BS pool by ReadFile
  <br>
  If Buffer != NULL and *Buffer != NULL, the output buffer has been
  allocated by the caller and is being passed in.
  
  @param  BufferSize Indicates the buffer size passed in, and on output the size
  required to complete the read
  @param  FoundType Indicates the type of the file who's data is returned
  @param  FileAttributes Indicates the attributes of the file who's data is resturned
  @param  AuthenticationStatus Indicates the authentication status of the data

  @retval EFI_SUCCESS
  @retval EFI_WARN_BUFFER_TOO_SMALL
  @retval EFI_NOT_FOUND
  @retval EFI_DEVICE_ERROR
  @retval EFI_ACCESS_DENIED

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FV_READ_FILE) (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN EFI_GUID                       *NameGuid,
  IN OUT VOID                       **Buffer,
  IN OUT UINTN                      *BufferSize,
  OUT EFI_FV_FILETYPE               *FoundType,
  OUT EFI_FV_FILE_ATTRIBUTES        *FileAttributes,
  OUT UINT32                        *AuthenticationStatus
  );

/**
  Read the requested section from the specified file and returns data in Buffer.

  @param  This Calling context
  @param  NameGuid Filename identifying the file from which to read
  @param  SectionType Indicates what section type to retrieve
  @param  SectionInstance Indicates which instance of SectionType to retrieve
  @param  Buffer Pointer to pointer to buffer in which contents of file are returned.
  <br>
  If Buffer is NULL, only type, attributes, and size are returned as
  there is no output buffer.
  <br>
  If Buffer != NULL and *Buffer == NULL, the output buffer is allocated
  from BS pool by ReadFile
  <br>
  If Buffer != NULL and *Buffer != NULL, the output buffer has been
  allocated by the caller and is being passed in.
  
  @param  BufferSize Indicates the buffer size passed in, and on output the size
  required to complete the read
  @param  AuthenticationStatus Indicates the authentication status of the data

  @retval EFI_SUCCESS
  @retval EFI_WARN_BUFFER_TOO_SMALL
  @retval EFI_OUT_OF_RESOURCES
  @retval EFI_NOT_FOUND
  @retval EFI_DEVICE_ERROR
  @retval EFI_ACCESS_DENIED

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FV_READ_SECTION) (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN EFI_GUID                       *NameGuid,
  IN EFI_SECTION_TYPE               SectionType,
  IN UINTN                          SectionInstance,
  IN OUT VOID                       **Buffer,
  IN OUT UINTN                      *BufferSize,
  OUT UINT32                        *AuthenticationStatus
  );

typedef UINT32  EFI_FV_WRITE_POLICY;

#define EFI_FV_UNRELIABLE_WRITE 0x00000000
#define EFI_FV_RELIABLE_WRITE   0x00000001

typedef struct {
  EFI_GUID                *NameGuid;
  EFI_FV_FILETYPE         Type;
  EFI_FV_FILE_ATTRIBUTES  FileAttributes;
  VOID                    *Buffer;
  UINT32                  BufferSize;
} EFI_FV_WRITE_FILE_DATA;

/**
  Write the supplied file (NameGuid) to the FV.

  @param  This Calling context
  @param  NumberOfFiles Indicates the number of file records pointed to by FileData
  @param  WritePolicy Indicates the level of reliability of the write with respect to
  things like power failure events.
  @param  FileData A pointer to an array of EFI_FV_WRITE_FILE_DATA structures.  Each
  element in the array indicates a file to write, and there are
  NumberOfFiles elements in the input array.

  @retval EFI_SUCCESS
  @retval EFI_OUT_OF_RESOURCES
  @retval EFI_DEVICE_ERROR
  @retval EFI_WRITE_PROTECTED
  @retval EFI_NOT_FOUND
  @retval EFI_INVALID_PARAMETER

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FV_WRITE_FILE) (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN UINT32                         NumberOfFiles,
  IN EFI_FV_WRITE_POLICY            WritePolicy,
  IN EFI_FV_WRITE_FILE_DATA         *FileData
  );

/**
  Given the input key, search for the next matching file in the volume.

  @param  This Calling context
  @param  Key Pointer to a caller allocated buffer that contains an implementation
  specific key that is used to track where to begin searching on
  successive calls.
  @param  FileType Indicates the file type to filter for
  @param  NameGuid Guid filename of the file found
  @param  Attributes Attributes of the file found
  @param  Size Size in bytes of the file found

  @retval EFI_SUCCESS
  @retval EFI_NOT_FOUND
  @retval EFI_DEVICE_ERROR
  @retval EFI_ACCESS_DENIED

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FV_GET_NEXT_FILE) (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN OUT VOID                       *Key,
  IN OUT EFI_FV_FILETYPE            *FileType,
  OUT EFI_GUID                      *NameGuid,
  OUT EFI_FV_FILE_ATTRIBUTES        *Attributes,
  OUT UINTN                         *Size
  );

/**
  @par Protocol Description:
  The Firmware Volume Protocol provides file-level access to the firmware volume.
  Each firmware volume driver must produce an instance of the Firmware Volume 
  Protocol if the firmware volume is to be visible to the system. The Firmware 
  Volume Protocol also provides mechanisms for determining and modifying some 
  attributes of the firmware volume.  

  @param GetVolumeAttributes
  Retrieves volume capabilities and current settings. 

  @param SetVolumeAttributes
  Modifies the current settings of the firmware volume.

  @param ReadFile
  Reads an entire file from the firmware volume. 

  @param ReadSection
  Reads a single section from a file into a buffer.

  @param WriteFile
  Writes an entire file into the firmware volume.

  @param GetNextFile
  Provides service to allow searching the firmware volume.

  @param KeySize
  Data field that indicates the size in bytes of the Key input buffer for 
  the GetNextFile() API. 

  @param ParentHandle
  Handle of the parent firmware volume.

**/
struct _EFI_FIRMWARE_VOLUME_PROTOCOL {
  EFI_FV_GET_ATTRIBUTES GetVolumeAttributes;
  EFI_FV_SET_ATTRIBUTES SetVolumeAttributes;
  EFI_FV_READ_FILE      ReadFile;
  EFI_FV_READ_SECTION   ReadSection;
  EFI_FV_WRITE_FILE     WriteFile;
  EFI_FV_GET_NEXT_FILE  GetNextFile;
  UINT32                KeySize;
  EFI_HANDLE            ParentHandle;
};

extern EFI_GUID gEfiFirmwareVolumeProtocolGuid;

#endif
