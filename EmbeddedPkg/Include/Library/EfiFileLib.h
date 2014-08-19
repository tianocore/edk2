/** @file
  Library functions that perform file IO. Memory buffer, file system, and
  fimrware volume operations are supproted.

  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Basic support for opening files on different device types. The device string
  is in the form of DevType:Path. Current DevType is required as there is no
  current mounted device concept of current working directory concept implement
  by this library.

  Device names are case insensative and only check the leading characters for
  unique matches. Thus the following are all the same:
    LoadFile0:
    l0:
    L0:
    Lo0:

  Supported Device Names:
  A0x1234:0x12 - A memory buffer starting at address 0x1234 for 0x12 bytes
  l1:          - EFI LoadFile device one.
  B0:          - EFI BlockIo zero.
  fs3:         - EFI Simple File System device 3
  Fv2:         - EFI Firmware VOlume device 2
  1.2.3.4:name - TFTP IP and file name

**/

#ifndef __EFI_FILE_LIB_H__
#define __EFI_FILE_LIB_H__

#include <PiDxe.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/BlockIo.h>
#include <Protocol/LoadFile.h>
#include <Protocol/LoadFile.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>

#define MAX_PATHNAME    0x200

/// Type of the file that has been opened
typedef enum {
  EfiOpenLoadFile,
  EfiOpenMemoryBuffer,
  EfiOpenFirmwareVolume,
  EfiOpenFileSystem,
  EfiOpenBlockIo,
  EfiOpenTftp,
  EfiOpenMaxValue
} EFI_OPEN_FILE_TYPE;


/// Public information about the open file
typedef struct {
  UINTN                         Version;          // Common information
  EFI_OPEN_FILE_TYPE            Type;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  EFI_STATUS                    LastError;
  EFI_HANDLE                    EfiHandle;
  CHAR8                         *DeviceName;
  CHAR8                         *FileName;

  UINT64                        CurrentPosition;  // Information for Seek
  UINT64                        MaxPosition;

  UINTN                         BaseOffset;       // Base offset for hexdump command

  UINTN                         Size;             // Valid for all types other than l#:
  UINT8                         *Buffer;          // Information valid for A#:

  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;              // Information valid for Fv#:
  EFI_GUID                      FvNameGuid;
  EFI_SECTION_TYPE              FvSectionType;
  EFI_FV_FILETYPE               FvType;
  EFI_FV_FILE_ATTRIBUTES        FvAttributes;

  EFI_PHYSICAL_ADDRESS          FvStart;
  UINTN                         FvSize;
  UINTN                         FvHeaderSize;

  EFI_FILE                      *FsFileHandle;    // Information valid for Fs#:
  EFI_FILE_SYSTEM_INFO          *FsInfo;
  EFI_FILE_INFO                 *FsFileInfo;
  EFI_BLOCK_IO_MEDIA            *FsBlockIoMedia;  // Information valid for Fs#: or B#:
  EFI_BLOCK_IO_PROTOCOL         *FsBlockIo;       // Information valid for Fs#: or B#:

  UINTN                         DiskOffset;       // Information valid for B#:

  EFI_LOAD_FILE_PROTOCOL        *LoadFile;        // Information valid for l#:

  EFI_IP_ADDRESS                ServerIp;         // Information valid for t:
  BOOLEAN                       IsDirty;
  BOOLEAN                       IsBufferValid;

} EFI_OPEN_FILE;


/// Type of Seek to perform
typedef enum {
  EfiSeekStart,
  EfiSeekCurrent,
  EfiSeekEnd,
  EfiSeekMax
} EFI_SEEK_TYPE;


/**
  Open a device named by PathName. The PathName includes a device name and
  path separated by a :. See file header for more details on the PathName
  syntax. There is no checking to prevent a file from being opened more than
  one type.

  SectionType is only used to open an FV. Each file in an FV contains multiple
  sections and only the SectionType section is opened.

  For any file that is opened with EfiOpen() must be closed with EfiClose().

  @param  PathName    Path to parse to open
  @param  OpenMode    Same as EFI_FILE.Open()
  @param  SectionType Section in FV to open.

  @return NULL  Open failed
  @return Valid EFI_OPEN_FILE handle

**/
EFI_OPEN_FILE *
EfiOpen (
  IN        CHAR8               *PathName,
  IN  CONST UINT64              OpenMode,
  IN  CONST EFI_SECTION_TYPE    SectionType
  );

EFI_STATUS
EfiCopyFile (
  IN        CHAR8               *DestinationFile,
  IN        CHAR8               *SourceFile
  );

/**
  Use DeviceType and Index to form a valid PathName and try and open it.

  @param  DeviceType  Device type to open
  @param  Index       Device Index to use. Zero relative.

  @return NULL  Open failed
  @return Valid EFI_OPEN_FILE handle

**/
EFI_OPEN_FILE  *
EfiDeviceOpenByType (
  IN  EFI_OPEN_FILE_TYPE    DeviceType,
  IN  UINTN                 Index
  );


/**
  Close a file handle opened by EfiOpen() and free all resources allocated by
  EfiOpen().

  @param  Stream    Open File Handle

  @return EFI_INVALID_PARAMETER  Stream is not an Open File
  @return EFI_SUCCESS            Steam closed

**/
EFI_STATUS
EfiClose (
  IN  EFI_OPEN_FILE     *Stream
  );


/**
  Return the size of the file represented by Stream. Also return the current
  Seek position. Opening a file will enable a valid file size to be returned.
  LoadFile is an exception as a load file size is set to zero.

  @param  Stream    Open File Handle

  @return 0         Stream is not an Open File or a valid LoadFile handle

**/
UINTN
EfiTell (
  IN  EFI_OPEN_FILE     *Stream,
  OUT UINT64            *CurrentPosition   OPTIONAL
  );


/**
  Seek to the Offset location in the file. LoadFile and FV device types do
  not support EfiSeek(). It is not possible to grow the file size using
  EfiSeek().

  SeekType defines how use Offset to calculate the new file position:
  EfiSeekStart  : Position = Offset
  EfiSeekCurrent: Position is Offset bytes from the current position
  EfiSeekEnd    : Only supported if Offset is zero to seek to end of file.

  @param  Stream    Open File Handle
  @param  Offset    Offset to seek too.
  @param  SeekType  Type of seek to perform


  @return EFI_INVALID_PARAMETER  Stream is not an Open File
  @return EFI_UNSUPPORTED        LoadFile and FV does not support Seek
  @return EFI_NOT_FOUND          Seek past the end of the file.
  @return EFI_SUCCESS            Steam closed

**/
EFI_STATUS
EfiSeek (
  IN  EFI_OPEN_FILE     *Stream,
  IN  EFI_LBA           Offset,
  IN  EFI_SEEK_TYPE     SeekType
  );


/**
  Read BufferSize bytes from the current location in the file. For load file
  and FV case you must read the entire file.

  @param  Stream      Open File Handle
  @param  Buffer      Caller allocated buffer.
  @param  BufferSize  Size of buffer in bytes.


  @return EFI_SUCCESS           Stream is not an Open File
  @return EFI_END_OF_FILE Tried to read past the end of the file
  @return EFI_INVALID_PARAMETER Stream is not an open file handle
  @return EFI_BUFFER_TOO_SMALL  Buffer is not big enough to do the read
  @return "other"               Error returned from device read

**/
EFI_STATUS
EfiRead (
  IN  EFI_OPEN_FILE     *Stream,
  OUT VOID              *Buffer,
  OUT UINTN             *BufferSize
  );


/**
  Read the entire file into a buffer. This routine allocates the buffer and
  returns it to the user full of the read data.

  This is very useful for load file where it's hard to know how big the buffer
  must be.

  @param  Stream      Open File Handle
  @param  Buffer      Pointer to buffer to return.
  @param  BufferSize  Pointer to Size of buffer return..


  @return EFI_SUCCESS           Stream is not an Open File
  @return EFI_END_OF_FILE       Tried to read past the end of the file
  @return EFI_INVALID_PARAMETER Stream is not an open file handle
  @return EFI_BUFFER_TOO_SMALL  Buffer is not big enough to do the read
  @return "other"               Error returned from device read

**/
EFI_STATUS
EfiReadAllocatePool (
  IN  EFI_OPEN_FILE     *Stream,
  OUT VOID              **Buffer,
  OUT UINTN             *BufferSize
  );


/**
  Write data back to the file.

  @param  Stream      Open File Handle
  @param  Buffer      Pointer to buffer to return.
  @param  BufferSize  Pointer to Size of buffer return..


  @return EFI_SUCCESS           Stream is not an Open File
  @return EFI_END_OF_FILE       Tried to read past the end of the file
  @return EFI_INVALID_PARAMETER Stream is not an open file handle
  @return EFI_BUFFER_TOO_SMALL  Buffer is not big enough to do the read
  @return "other"               Error returned from device write

**/
EFI_STATUS
EfiWrite (
  IN  EFI_OPEN_FILE   *Stream,
  OUT VOID            *Buffer,
  OUT UINTN           *BufferSize
  );


/**
  Return the number of devices of the current type active in the system

  @param  Type      Device type to check

  @return 0         Invalid type

**/
UINTN
EfiGetDeviceCounts (
  IN  EFI_OPEN_FILE_TYPE     Type
  );


/**
  Set the Current Working Directory (CWD). If a call is made to EfiOpen () and
  the path does not contain a device name, The CWD is prepended to the path.

  @param  Cwd     Current Working Directory to set


  @return EFI_SUCCESS           CWD is set
  @return EFI_INVALID_PARAMETER Cwd is not a valid device:path

**/
EFI_STATUS
EfiSetCwd (
  IN  CHAR8   *Cwd
  );

/**
  Set the Current Working Directory (CWD). If a call is made to EfiOpen () and
  the path does not contain a device name, The CWD is prepended to the path.

  @param  Cwd     Current Working Directory


  @return NULL    No CWD set
  @return 'other' malloc'ed buffer contains CWD.

**/
CHAR8 *
EfiGetCwd (
  VOID
  );

#endif
