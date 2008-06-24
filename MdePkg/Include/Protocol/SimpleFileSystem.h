/** @file
  SimpleFileSystem protocol as defined in the UEFI 2.0 specification.

  The SimpleFileSystem protocol is the programatic access to the FAT (12,16,32) 
  file system specified in UEFI 2.0. It can also be used to abstract a file  
  system other than FAT.

  UEFI 2.0 can boot from any valid EFI image contained in a SimpleFileSystem

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __SIMPLE_FILE_SYSTEM_H__
#define __SIMPLE_FILE_SYSTEM_H__

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
  { \
    0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

typedef struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

typedef struct _EFI_FILE_PROTOCOL         EFI_FILE_PROTOCOL;
typedef struct _EFI_FILE_PROTOCOL         *EFI_FILE_HANDLE;


//
// Protocol GUID defined in EFI1.1.
// 
#define SIMPLE_FILE_SYSTEM_PROTOCOL       EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID

//
// Protocol defined in EFI1.1.
// 
typedef EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   EFI_FILE_IO_INTERFACE;
typedef struct _EFI_FILE_PROTOCOL         EFI_FILE;

/**
  Open the root directory on a volume.

  @param  This Protocol instance pointer.
  @param  Root Returns an Open file handle for the root directory

  @retval EFI_SUCCESS          The device was opened.
  @retval EFI_UNSUPPORTED      This volume does not suppor the file system.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted
  @retval EFI_ACCESS_DENIED    The service denied access to the file
  @retval EFI_OUT_OF_RESOURCES The volume was not opened due to lack of resources

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME)(
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL    *This,
  OUT EFI_FILE_PROTOCOL                 **Root
  )
;

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION  0x00010000
//
// Revision defined in EFI1.1
// 
#define EFI_FILE_IO_INTERFACE_REVISION  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION

struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
  UINT64                                      Revision;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME OpenVolume;
};

/**
  Open the root directory on a volume.

  @param  This       Protocol instance pointer.
  @param  NewHandle  Returns File Handle for FileName
  @param  FileName   Null terminated string. "\", ".", and ".." are supported
  @param  OpenMode   Open mode for file.
  @param  Attributes Only used for EFI_FILE_MODE_CREATE

  @retval EFI_SUCCESS          The device was opened.
  @retval EFI_NOT_FOUND        The specified file could not be found on the device
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_MEDIA_CHANGED    The media has changed
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted
  @retval EFI_ACCESS_DENIED    The service denied access to the file
  @retval EFI_OUT_OF_RESOURCES The volume was not opened due to lack of resources
  @retval EFI_VOLUME_FULL      The volume is full.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FILE_OPEN)(
  IN EFI_FILE_PROTOCOL        *This,
  OUT EFI_FILE_PROTOCOL       **NewHandle,
  IN CHAR16                   *FileName,
  IN UINT64                   OpenMode,
  IN UINT64                   Attributes
  )
;

//
// Open modes
//
#define EFI_FILE_MODE_READ    0x0000000000000001ULL
#define EFI_FILE_MODE_WRITE   0x0000000000000002ULL
#define EFI_FILE_MODE_CREATE  0x8000000000000000ULL

//
// File attributes
//
#define EFI_FILE_READ_ONLY  0x0000000000000001ULL
#define EFI_FILE_HIDDEN     0x0000000000000002ULL
#define EFI_FILE_SYSTEM     0x0000000000000004ULL
#define EFI_FILE_RESERVED   0x0000000000000008ULL
#define EFI_FILE_DIRECTORY  0x0000000000000010ULL
#define EFI_FILE_ARCHIVE    0x0000000000000020ULL
#define EFI_FILE_VALID_ATTR 0x0000000000000037ULL

/**
  Close the file handle

  @param  This          Protocol instance pointer.

  @retval EFI_SUCCESS   The device was opened.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FILE_CLOSE)(
  IN EFI_FILE  *This
  )
;

/**
  Close and delete the file handle

  @param  This                     Protocol instance pointer.
                                   
  @retval EFI_SUCCESS              The device was opened.
  @retval EFI_WARN_DELETE_FAILURE  The handle was closed but the file was not deleted

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FILE_DELETE)(
  IN EFI_FILE  *This
  )
;

/**
  Read data from the file.

  @param  This       Protocol instance pointer.
  @param  BufferSize On input size of buffer, on output amount of data in buffer.
  @param  Buffer     The buffer in which data is read.

  @retval EFI_SUCCESS          Data was read.
  @retval EFI_NO_MEDIA         The device has no media
  @retval EFI_DEVICE_ERROR     The device reported an error
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted
  @retval EFI_BUFFER_TO_SMALL  BufferSize is too small. BufferSize contains required size

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FILE_READ)(
  IN EFI_FILE_PROTOCOL        *This,
  IN OUT UINTN                *BufferSize,
  OUT VOID                    *Buffer
  )
;

/**
  Write data from to the file.

  @param  This       Protocol instance pointer.
  @param  BufferSize On input size of buffer, on output amount of data in buffer.
  @param  Buffer     The buffer in which data to write.

  @retval EFI_SUCCESS          Data was written.
  @retval EFI_UNSUPPORT        Writes to Open directory are not supported
  @retval EFI_NO_MEDIA         The device has no media
  @retval EFI_DEVICE_ERROR     The device reported an error
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted
  @retval EFI_WRITE_PROTECTED  The device is write protected
  @retval EFI_ACCESS_DENIED    The file was open for read only
  @retval EFI_VOLUME_FULL      The volume is full

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FILE_WRITE)(
  IN EFI_FILE_PROTOCOL        *This,
  IN OUT UINTN                *BufferSize,
  IN VOID                     *Buffer
  )
;

/**
  Set a files current position

  @param  This            Protocol instance pointer.
  @param  Position        Byte possition from the start of the file
                          
  @retval EFI_SUCCESS     Data was written.
  @retval EFI_UNSUPPORTED Seek request for non-zero is not valid on open.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FILE_SET_POSITION)(
  IN EFI_FILE_PROTOCOL        *This,
  IN UINT64                   Position
  )
;

/**
  Get a files current position

  @param  This            Protocol instance pointer.
  @param  Position        Byte possition from the start of the file
                          
  @retval EFI_SUCCESS     Data was written.
  @retval EFI_UNSUPPORTED Seek request for non-zero is not valid on open.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FILE_GET_POSITION)(
  IN EFI_FILE_PROTOCOL        *This,
  OUT UINT64                  *Position
  )
;

/**
  Get information about a file

  @param  This            Protocol instance pointer.
  @param  InformationType Type of info to return in Buffer
  @param  BufferSize      On input size of buffer, on output amount of data in buffer.
  @param  Buffer          The buffer to return data.

  @retval EFI_SUCCESS          Data was returned.
  @retval EFI_UNSUPPORT        InformationType is not supported
  @retval EFI_NO_MEDIA         The device has no media
  @retval EFI_DEVICE_ERROR     The device reported an error
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted
  @retval EFI_WRITE_PROTECTED  The device is write protected
  @retval EFI_ACCESS_DENIED    The file was open for read only
  @retval EFI_BUFFER_TOO_SMALL Buffer was too small, required size returned in BufferSize

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FILE_GET_INFO)(
  IN EFI_FILE_PROTOCOL        *This,
  IN EFI_GUID                 *InformationType,
  IN OUT UINTN                *BufferSize,
  OUT VOID                    *Buffer
  )
;

/**
  Set information about a file

  @param  File            Protocol instance pointer.
  @param  InformationType Type of info in Buffer
  @param  BufferSize      Size of buffer.
  @param  Buffer          The data to write.

  @retval EFI_SUCCESS          Data was returned.
  @retval EFI_UNSUPPORT        InformationType is not supported
  @retval EFI_NO_MEDIA         The device has no media
  @retval EFI_DEVICE_ERROR     The device reported an error
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted
  @retval EFI_WRITE_PROTECTED  The device is write protected
  @retval EFI_ACCESS_DENIED    The file was open for read only

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FILE_SET_INFO)(
  IN EFI_FILE_PROTOCOL        *This,
  IN EFI_GUID                 *InformationType,
  IN UINTN                    BufferSize,
  IN VOID                     *Buffer
  )
;

/**
  Flush data back for the file handle

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS          Data was written.
  @retval EFI_UNSUPPORT        Writes to Open directory are not supported
  @retval EFI_NO_MEDIA         The device has no media
  @retval EFI_DEVICE_ERROR     The device reported an error
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted
  @retval EFI_WRITE_PROTECTED  The device is write protected
  @retval EFI_ACCESS_DENIED    The file was open for read only
  @retval EFI_VOLUME_FULL      The volume is full

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FILE_FLUSH)(
  IN EFI_FILE  *This
  )
;

#define EFI_FILE_PROTOCOL_REVISION   0x00010000
//
// Revision defined in EFI1.1.
// 
#define EFI_FILE_REVISION   EFI_FILE_PROTOCOL_REVISION

struct _EFI_FILE_PROTOCOL {
  UINT64                Revision;
  EFI_FILE_OPEN         Open;
  EFI_FILE_CLOSE        Close;
  EFI_FILE_DELETE       Delete;
  EFI_FILE_READ         Read;
  EFI_FILE_WRITE        Write;
  EFI_FILE_GET_POSITION GetPosition;
  EFI_FILE_SET_POSITION SetPosition;
  EFI_FILE_GET_INFO     GetInfo;
  EFI_FILE_SET_INFO     SetInfo;
  EFI_FILE_FLUSH        Flush;
};


extern EFI_GUID gEfiSimpleFileSystemProtocolGuid;

#endif
