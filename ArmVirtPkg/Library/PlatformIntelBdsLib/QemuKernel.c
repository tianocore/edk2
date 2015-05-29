/** @file
  Try to load an EFI-stubbed ARM Linux kernel from QEMU's fw_cfg.

  This implementation differs from OvmfPkg/Library/LoadLinuxLib. An EFI
  stub in the subject kernel is a hard requirement here.

  Copyright (C) 2014, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>
#include <Guid/FileSystemVolumeLabelInfo.h>
#include <Library/PrintLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>

#include "IntelBdsPlatform.h"

//
// Static data that hosts the fw_cfg blobs and serves file requests.
//
typedef enum {
  KernelBlobTypeKernel,
  KernelBlobTypeInitrd,
  KernelBlobTypeCommandLine,
  KernelBlobTypeMax
} KERNEL_BLOB_TYPE;

typedef struct {
  FIRMWARE_CONFIG_ITEM CONST SizeKey;
  FIRMWARE_CONFIG_ITEM CONST DataKey;
  CONST CHAR16 *       CONST Name;
  UINT32                     Size;
  UINT8                      *Data;
} KERNEL_BLOB;

STATIC KERNEL_BLOB mKernelBlob[KernelBlobTypeMax] = {
  { QemuFwCfgItemKernelSize,      QemuFwCfgItemKernelData,      L"kernel"  },
  { QemuFwCfgItemInitrdSize,      QemuFwCfgItemInitrdData,      L"initrd"  },
  { QemuFwCfgItemCommandLineSize, QemuFwCfgItemCommandLineData, L"cmdline" }
};

STATIC UINT64 mTotalBlobBytes;

//
// Device path for the handle that incorporates our "EFI stub filesystem". The
// GUID is arbitrary and need not be standardized or advertized.
//
#pragma pack(1)
typedef struct {
  VENDOR_DEVICE_PATH       VenHwNode;
  EFI_DEVICE_PATH_PROTOCOL EndNode;
} SINGLE_VENHW_NODE_DEVPATH;
#pragma pack()

STATIC CONST SINGLE_VENHW_NODE_DEVPATH mFileSystemDevicePath = {
  {
    { HARDWARE_DEVICE_PATH, HW_VENDOR_DP, { sizeof (VENDOR_DEVICE_PATH) } },
    {
      0xb0fae7e7, 0x6b07, 0x49d0,
      { 0x9e, 0x5b, 0x3b, 0xde, 0xc8, 0x3b, 0x03, 0x9d }
    }
  },

  {
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { sizeof (EFI_DEVICE_PATH_PROTOCOL) }
  }
};

//
// The "file in the EFI stub filesystem" abstraction.
//
STATIC EFI_TIME mInitTime;

#define STUB_FILE_SIG SIGNATURE_64 ('S', 'T', 'U', 'B', 'F', 'I', 'L', 'E')

typedef struct {
  UINT64            Signature; // Carries STUB_FILE_SIG.

  KERNEL_BLOB_TYPE  BlobType;  // Index into mKernelBlob. KernelBlobTypeMax
                               // denotes the root directory of the filesystem.

  UINT64            Position;  // Byte position for regular files;
                               // next directory entry to return for the root
                               // directory.

  EFI_FILE_PROTOCOL File;      // Standard protocol interface.
} STUB_FILE;

#define STUB_FILE_FROM_FILE(FilePointer) \
        CR (FilePointer, STUB_FILE, File, STUB_FILE_SIG)

//
// Tentative definition of the file protocol template. The initializer
// (external definition) will be provided later.
//
STATIC CONST EFI_FILE_PROTOCOL mEfiFileProtocolTemplate;


//
// Protocol member functions for File.
//

/**
  Opens a new file relative to the source file's location.

  @param[in]  This        A pointer to the EFI_FILE_PROTOCOL instance that is
                          the file handle to the source location. This would
                          typically be an open handle to a directory.

  @param[out] NewHandle   A pointer to the location to return the opened handle
                          for the new file.

  @param[in]  FileName    The Null-terminated string of the name of the file to
                          be opened. The file name may contain the following
                          path modifiers: "\", ".", and "..".

  @param[in]  OpenMode    The mode to open the file. The only valid
                          combinations that the file may be opened with are:
                          Read, Read/Write, or Create/Read/Write.

  @param[in]  Attributes  Only valid for EFI_FILE_MODE_CREATE, in which case
                          these are the attribute bits for the newly created
                          file.

  @retval EFI_SUCCESS           The file was opened.
  @retval EFI_NOT_FOUND         The specified file could not be found on the
                                device.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_MEDIA_CHANGED     The device has a different medium in it or the
                                medium is no longer supported.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED   An attempt was made to create a file, or open a
                                file for write when the media is
                                write-protected.
  @retval EFI_ACCESS_DENIED     The service denied access to the file.
  @retval EFI_OUT_OF_RESOURCES  Not enough resources were available to open the
                                file.
  @retval EFI_VOLUME_FULL       The volume is full.
**/
STATIC
EFI_STATUS
EFIAPI
StubFileOpen (
  IN EFI_FILE_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL **NewHandle,
  IN CHAR16             *FileName,
  IN UINT64             OpenMode,
  IN UINT64             Attributes
  )
{
  CONST STUB_FILE *StubFile;
  UINTN           BlobType;
  STUB_FILE       *NewStubFile;

  //
  // We're read-only.
  //
  switch (OpenMode) {
    case EFI_FILE_MODE_READ:
      break;

    case EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE:
    case EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE:
      return EFI_WRITE_PROTECTED;

    default:
      return EFI_INVALID_PARAMETER;
  }

  //
  // Only the root directory supports opening files in it.
  //
  StubFile = STUB_FILE_FROM_FILE (This);
  if (StubFile->BlobType != KernelBlobTypeMax) {
    return EFI_UNSUPPORTED;
  }

  //
  // Locate the file.
  //
  for (BlobType = 0; BlobType < KernelBlobTypeMax; ++BlobType) {
    if (StrCmp (FileName, mKernelBlob[BlobType].Name) == 0) {
      break;
    }
  }
  if (BlobType == KernelBlobTypeMax) {
    return EFI_NOT_FOUND;
  }

  //
  // Found it.
  //
  NewStubFile = AllocatePool (sizeof *NewStubFile);
  if (NewStubFile == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewStubFile->Signature = STUB_FILE_SIG;
  NewStubFile->BlobType  = (KERNEL_BLOB_TYPE)BlobType;
  NewStubFile->Position  = 0;
  CopyMem (&NewStubFile->File, &mEfiFileProtocolTemplate,
    sizeof mEfiFileProtocolTemplate);
  *NewHandle = &NewStubFile->File;

  return EFI_SUCCESS;
}


/**
  Closes a specified file handle.

  @param[in] This  A pointer to the EFI_FILE_PROTOCOL instance that is the file
                   handle to close.

  @retval EFI_SUCCESS  The file was closed.
**/
STATIC
EFI_STATUS
EFIAPI
StubFileClose (
  IN EFI_FILE_PROTOCOL *This
  )
{
  FreePool (STUB_FILE_FROM_FILE (This));
  return EFI_SUCCESS;
}


/**
  Close and delete the file handle.

  @param[in] This  A pointer to the EFI_FILE_PROTOCOL instance that is the
                   handle to the file to delete.

  @retval EFI_SUCCESS              The file was closed and deleted, and the
                                   handle was closed.
  @retval EFI_WARN_DELETE_FAILURE  The handle was closed, but the file was not
                                   deleted.

**/
STATIC
EFI_STATUS
EFIAPI
StubFileDelete (
  IN EFI_FILE_PROTOCOL *This
  )
{
  FreePool (STUB_FILE_FROM_FILE (This));
  return EFI_WARN_DELETE_FAILURE;
}


/**
  Helper function that formats an EFI_FILE_INFO structure into the
  user-allocated buffer, for any valid KERNEL_BLOB_TYPE value (including
  KernelBlobTypeMax, which stands for the root directory).

  The interface follows the EFI_FILE_GET_INFO -- and for directories, the
  EFI_FILE_READ -- interfaces.

  @param[in]     BlobType     The KERNEL_BLOB_TYPE value identifying the fw_cfg
                              blob backing the STUB_FILE that information is
                              being requested about. If BlobType equals
                              KernelBlobTypeMax, then information will be
                              provided about the root directory of the
                              filesystem.

  @param[in,out] BufferSize  On input, the size of Buffer. On output, the
                             amount of data returned in Buffer. In both cases,
                             the size is measured in bytes.

  @param[out]    Buffer      A pointer to the data buffer to return. The
                             buffer's type is EFI_FILE_INFO.

  @retval EFI_SUCCESS           The information was returned.
  @retval EFI_BUFFER_TOO_SMALL  BufferSize is too small to store the
                                EFI_FILE_INFO structure. BufferSize has been
                                updated with the size needed to complete the
                                request.
**/
STATIC
EFI_STATUS
ConvertKernelBlobTypeToFileInfo (
  IN KERNEL_BLOB_TYPE BlobType,
  IN OUT UINTN        *BufferSize,
  OUT VOID            *Buffer
  )
{
  CONST CHAR16  *Name;
  UINT64        FileSize;
  UINT64        Attribute;

  UINTN         NameSize;
  UINTN         FileInfoSize;
  EFI_FILE_INFO *FileInfo;
  UINTN         OriginalBufferSize;

  if (BlobType == KernelBlobTypeMax) {
    //
    // getting file info about the root directory
    //
    Name      = L"\\";
    FileSize  = KernelBlobTypeMax;
    Attribute = EFI_FILE_READ_ONLY | EFI_FILE_DIRECTORY;
  } else {
    CONST KERNEL_BLOB *Blob;

    Blob      = &mKernelBlob[BlobType];
    Name      = Blob->Name;
    FileSize  = Blob->Size;
    Attribute = EFI_FILE_READ_ONLY;
  }

  NameSize     = (StrLen(Name) + 1) * 2;
  FileInfoSize = OFFSET_OF (EFI_FILE_INFO, FileName) + NameSize;
  ASSERT (FileInfoSize >= sizeof *FileInfo);

  OriginalBufferSize = *BufferSize;
  *BufferSize        = FileInfoSize;
  if (OriginalBufferSize < *BufferSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  FileInfo               = (EFI_FILE_INFO *)Buffer;
  FileInfo->Size         = FileInfoSize;
  FileInfo->FileSize     = FileSize;
  FileInfo->PhysicalSize = FileSize;
  FileInfo->Attribute    = Attribute;

  CopyMem (&FileInfo->CreateTime,       &mInitTime, sizeof mInitTime);
  CopyMem (&FileInfo->LastAccessTime,   &mInitTime, sizeof mInitTime);
  CopyMem (&FileInfo->ModificationTime, &mInitTime, sizeof mInitTime);
  CopyMem (FileInfo->FileName,          Name,       NameSize);

  return EFI_SUCCESS;
}


/**
  Reads data from a file, or continues scanning a directory.

  @param[in]     This        A pointer to the EFI_FILE_PROTOCOL instance that
                             is the file handle to read data from.

  @param[in,out] BufferSize  On input, the size of the Buffer. On output, the
                             amount of data returned in Buffer. In both cases,
                             the size is measured in bytes. If the read goes
                             beyond the end of the file, the read length is
                             truncated to the end of the file.

                             If This is a directory, the function reads the
                             directory entry at the current position and
                             returns the entry (as EFI_FILE_INFO) in Buffer. If
                             there are no more directory entries, the
                             BufferSize is set to zero on output.

  @param[out]    Buffer      The buffer into which the data is read.

  @retval EFI_SUCCESS           Data was read.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_DEVICE_ERROR      An attempt was made to read from a deleted
                                file.
  @retval EFI_DEVICE_ERROR      On entry, the current file position is beyond
                                the end of the file.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize is too small to store the
                                current directory entry as a EFI_FILE_INFO
                                structure. BufferSize has been updated with the
                                size needed to complete the request, and the
                                directory position has not been advanced.
**/
STATIC
EFI_STATUS
EFIAPI
StubFileRead (
  IN EFI_FILE_PROTOCOL *This,
  IN OUT UINTN         *BufferSize,
  OUT VOID             *Buffer
  )
{
  STUB_FILE         *StubFile;
  CONST KERNEL_BLOB *Blob;
  UINT64            Left;

  StubFile = STUB_FILE_FROM_FILE (This);

  //
  // Scanning the root directory?
  //
  if (StubFile->BlobType == KernelBlobTypeMax) {
    EFI_STATUS Status;

    if (StubFile->Position == KernelBlobTypeMax) {
      //
      // Scanning complete.
      //
      *BufferSize = 0;
      return EFI_SUCCESS;
    }

    Status = ConvertKernelBlobTypeToFileInfo (StubFile->Position, BufferSize,
               Buffer);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    ++StubFile->Position;
    return EFI_SUCCESS;
  }

  //
  // Reading a file.
  //
  Blob = &mKernelBlob[StubFile->BlobType];
  if (StubFile->Position > Blob->Size) {
    return EFI_DEVICE_ERROR;
  }

  Left = Blob->Size - StubFile->Position;
  if (*BufferSize > Left) {
    *BufferSize = (UINTN)Left;
  }
  if (Blob->Data != NULL) {
    CopyMem (Buffer, Blob->Data + StubFile->Position, *BufferSize);
  }
  StubFile->Position += *BufferSize;
  return EFI_SUCCESS;
}


/**
  Writes data to a file.

  @param[in]     This        A pointer to the EFI_FILE_PROTOCOL instance that
                             is the file handle to write data to.

  @param[in,out] BufferSize  On input, the size of the Buffer. On output, the
                             amount of data actually written. In both cases,
                             the size is measured in bytes.

  @param[in]     Buffer      The buffer of data to write.

  @retval EFI_SUCCESS           Data was written.
  @retval EFI_UNSUPPORTED       Writes to open directory files are not
                                supported.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_DEVICE_ERROR      An attempt was made to write to a deleted file.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED   The file or medium is write-protected.
  @retval EFI_ACCESS_DENIED     The file was opened read only.
  @retval EFI_VOLUME_FULL       The volume is full.
**/
STATIC
EFI_STATUS
EFIAPI
StubFileWrite (
  IN EFI_FILE_PROTOCOL *This,
  IN OUT UINTN         *BufferSize,
  IN VOID              *Buffer
  )
{
  STUB_FILE *StubFile;

  StubFile = STUB_FILE_FROM_FILE (This);
  return (StubFile->BlobType == KernelBlobTypeMax) ?
         EFI_UNSUPPORTED :
         EFI_WRITE_PROTECTED;
}


/**
  Returns a file's current position.

  @param[in]  This      A pointer to the EFI_FILE_PROTOCOL instance that is the
                        file handle to get the current position on.

  @param[out] Position  The address to return the file's current position
                        value.

  @retval EFI_SUCCESS      The position was returned.
  @retval EFI_UNSUPPORTED  The request is not valid on open directories.
  @retval EFI_DEVICE_ERROR An attempt was made to get the position from a
                           deleted file.
**/
STATIC
EFI_STATUS
EFIAPI
StubFileGetPosition (
  IN EFI_FILE_PROTOCOL *This,
  OUT UINT64           *Position
  )
{
  STUB_FILE *StubFile;

  StubFile = STUB_FILE_FROM_FILE (This);
  if (StubFile->BlobType == KernelBlobTypeMax) {
    return EFI_UNSUPPORTED;
  }

  *Position = StubFile->Position;
  return EFI_SUCCESS;
}


/**
  Sets a file's current position.

  @param[in] This      A pointer to the EFI_FILE_PROTOCOL instance that is the
                       file handle to set the requested position on.

  @param[in] Position  The byte position from the start of the file to set. For
                       regular files, MAX_UINT64 means "seek to end". For
                       directories, zero means "rewind directory scan".

  @retval EFI_SUCCESS       The position was set.
  @retval EFI_UNSUPPORTED   The seek request for nonzero is not valid on open
                            directories.
  @retval EFI_DEVICE_ERROR  An attempt was made to set the position of a
                            deleted file.
**/
STATIC
EFI_STATUS
EFIAPI
StubFileSetPosition (
  IN EFI_FILE_PROTOCOL *This,
  IN UINT64            Position
  )
{
  STUB_FILE   *StubFile;
  KERNEL_BLOB *Blob;

  StubFile = STUB_FILE_FROM_FILE (This);

  if (StubFile->BlobType == KernelBlobTypeMax) {
    if (Position == 0) {
      //
      // rewinding a directory scan is allowed
      //
      StubFile->Position = 0;
      return EFI_SUCCESS;
    }
    return EFI_UNSUPPORTED;
  }

  //
  // regular file seek
  //
  Blob = &mKernelBlob[StubFile->BlobType];
  if (Position == MAX_UINT64) {
    //
    // seek to end
    //
    StubFile->Position = Blob->Size;
  } else {
    //
    // absolute seek from beginning -- seeking past the end is allowed
    //
    StubFile->Position = Position;
  }
  return EFI_SUCCESS;
}


/**
  Returns information about a file.

  @param[in]     This             A pointer to the EFI_FILE_PROTOCOL instance
                                  that is the file handle the requested
                                  information is for.

  @param[in]     InformationType  The type identifier GUID for the information
                                  being requested. The following information
                                  types are supported, storing the
                                  corresponding structures in Buffer:

                                  - gEfiFileInfoGuid: EFI_FILE_INFO

                                  - gEfiFileSystemInfoGuid:
                                    EFI_FILE_SYSTEM_INFO

                                  - gEfiFileSystemVolumeLabelInfoIdGuid:
                                    EFI_FILE_SYSTEM_VOLUME_LABEL

  @param[in,out] BufferSize       On input, the size of Buffer. On output, the
                                  amount of data returned in Buffer. In both
                                  cases, the size is measured in bytes.

  @param[out]    Buffer           A pointer to the data buffer to return. The
                                  buffer's type is indicated by
                                  InformationType.

  @retval EFI_SUCCESS           The information was returned.
  @retval EFI_UNSUPPORTED       The InformationType is not known.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize is too small to store the
                                information structure requested by
                                InformationType. BufferSize has been updated
                                with the size needed to complete the request.
**/
STATIC
EFI_STATUS
EFIAPI
StubFileGetInfo (
  IN EFI_FILE_PROTOCOL *This,
  IN EFI_GUID          *InformationType,
  IN OUT UINTN         *BufferSize,
  OUT VOID             *Buffer
  )
{
  CONST STUB_FILE *StubFile;
  UINTN           OriginalBufferSize;

  StubFile = STUB_FILE_FROM_FILE (This);

  if (CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    return ConvertKernelBlobTypeToFileInfo (StubFile->BlobType, BufferSize,
             Buffer);
  }

  OriginalBufferSize = *BufferSize;

  if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid)) {
    EFI_FILE_SYSTEM_INFO *FileSystemInfo;

    *BufferSize = sizeof *FileSystemInfo;
    if (OriginalBufferSize < *BufferSize) {
      return EFI_BUFFER_TOO_SMALL;
    }

    FileSystemInfo                 = (EFI_FILE_SYSTEM_INFO *)Buffer;
    FileSystemInfo->Size           = sizeof *FileSystemInfo;
    FileSystemInfo->ReadOnly       = TRUE;
    FileSystemInfo->VolumeSize     = mTotalBlobBytes;
    FileSystemInfo->FreeSpace      = 0;
    FileSystemInfo->BlockSize      = 1;
    FileSystemInfo->VolumeLabel[0] = L'\0';

    return EFI_SUCCESS;
  }

  if (CompareGuid (InformationType, &gEfiFileSystemVolumeLabelInfoIdGuid)) {
    EFI_FILE_SYSTEM_VOLUME_LABEL *FileSystemVolumeLabel;

    *BufferSize = sizeof *FileSystemVolumeLabel;
    if (OriginalBufferSize < *BufferSize) {
      return EFI_BUFFER_TOO_SMALL;
    }

    FileSystemVolumeLabel = (EFI_FILE_SYSTEM_VOLUME_LABEL *)Buffer;
    FileSystemVolumeLabel->VolumeLabel[0] = L'\0';

    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}


/**
  Sets information about a file.

  @param[in] File             A pointer to the EFI_FILE_PROTOCOL instance that
                              is the file handle the information is for.

  @param[in] InformationType  The type identifier for the information being
                              set.

  @param[in] BufferSize       The size, in bytes, of Buffer.

  @param[in] Buffer           A pointer to the data buffer to write. The
                              buffer's type is indicated by InformationType.

  @retval EFI_SUCCESS           The information was set.
  @retval EFI_UNSUPPORTED       The InformationType is not known.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED   InformationType is EFI_FILE_INFO_ID and the
                                media is read-only.
  @retval EFI_WRITE_PROTECTED   InformationType is
                                EFI_FILE_PROTOCOL_SYSTEM_INFO_ID and the media
                                is read only.
  @retval EFI_WRITE_PROTECTED   InformationType is
                                EFI_FILE_SYSTEM_VOLUME_LABEL_ID and the media
                                is read-only.
  @retval EFI_ACCESS_DENIED     An attempt is made to change the name of a file
                                to a file that is already present.
  @retval EFI_ACCESS_DENIED     An attempt is being made to change the
                                EFI_FILE_DIRECTORY Attribute.
  @retval EFI_ACCESS_DENIED     An attempt is being made to change the size of
                                a directory.
  @retval EFI_ACCESS_DENIED     InformationType is EFI_FILE_INFO_ID and the
                                file was opened read-only and an attempt is
                                being made to modify a field other than
                                Attribute.
  @retval EFI_VOLUME_FULL       The volume is full.
  @retval EFI_BAD_BUFFER_SIZE   BufferSize is smaller than the size of the type
                                indicated by InformationType.
**/
STATIC
EFI_STATUS
EFIAPI
StubFileSetInfo (
  IN EFI_FILE_PROTOCOL *This,
  IN EFI_GUID          *InformationType,
  IN UINTN             BufferSize,
  IN VOID              *Buffer
  )
{
  return EFI_WRITE_PROTECTED;
}


/**
  Flushes all modified data associated with a file to a device.

  @param [in] This  A pointer to the EFI_FILE_PROTOCOL instance that is the
                    file handle to flush.

  @retval EFI_SUCCESS           The data was flushed.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED   The file or medium is write-protected.
  @retval EFI_ACCESS_DENIED     The file was opened read-only.
  @retval EFI_VOLUME_FULL       The volume is full.
**/
STATIC
EFI_STATUS
EFIAPI
StubFileFlush (
  IN EFI_FILE_PROTOCOL *This
  )
{
  return EFI_WRITE_PROTECTED;
}

//
// External definition of the file protocol template.
//
STATIC CONST EFI_FILE_PROTOCOL mEfiFileProtocolTemplate = {
  EFI_FILE_PROTOCOL_REVISION, // revision 1
  StubFileOpen,
  StubFileClose,
  StubFileDelete,
  StubFileRead,
  StubFileWrite,
  StubFileGetPosition,
  StubFileSetPosition,
  StubFileGetInfo,
  StubFileSetInfo,
  StubFileFlush,
  NULL,                       // OpenEx, revision 2
  NULL,                       // ReadEx, revision 2
  NULL,                       // WriteEx, revision 2
  NULL                        // FlushEx, revision 2
};


//
// Protocol member functions for SimpleFileSystem.
//

/**
  Open the root directory on a volume.

  @param[in]  This  A pointer to the volume to open the root directory on.

  @param[out] Root  A pointer to the location to return the opened file handle
                    for the root directory in.

  @retval EFI_SUCCESS           The device was opened.
  @retval EFI_UNSUPPORTED       This volume does not support the requested file
                                system type.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_ACCESS_DENIED     The service denied access to the file.
  @retval EFI_OUT_OF_RESOURCES  The volume was not opened due to lack of
                                resources.
  @retval EFI_MEDIA_CHANGED     The device has a different medium in it or the
                                medium is no longer supported. Any existing
                                file handles for this volume are no longer
                                valid. To access the files on the new medium,
                                the volume must be reopened with OpenVolume().
**/
STATIC
EFI_STATUS
EFIAPI
StubFileSystemOpenVolume (
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
  OUT EFI_FILE_PROTOCOL              **Root
  )
{
  STUB_FILE *StubFile;

  StubFile = AllocatePool (sizeof *StubFile);
  if (StubFile == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  StubFile->Signature = STUB_FILE_SIG;
  StubFile->BlobType  = KernelBlobTypeMax;
  StubFile->Position  = 0;
  CopyMem (&StubFile->File, &mEfiFileProtocolTemplate,
    sizeof mEfiFileProtocolTemplate);
  *Root = &StubFile->File;

  return EFI_SUCCESS;
}

STATIC CONST EFI_SIMPLE_FILE_SYSTEM_PROTOCOL mFileSystem = {
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION,
  StubFileSystemOpenVolume
};


//
// Utility functions.
//

/**
  Populate a blob in mKernelBlob.

  param[in,out] Blob  Pointer to the KERNEL_BLOB element in mKernelBlob that is
                      to be filled from fw_cfg.

  @retval EFI_SUCCESS           Blob has been populated. If fw_cfg reported a
                                size of zero for the blob, then Blob->Data has
                                been left unchanged.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for Blob->Data.
**/
STATIC
EFI_STATUS
FetchBlob (
  IN OUT KERNEL_BLOB *Blob
  )
{
  UINT32 Left;

  //
  // Read blob size.
  //
  QemuFwCfgSelectItem (Blob->SizeKey);
  Blob->Size = QemuFwCfgRead32 ();
  if (Blob->Size == 0) {
    return EFI_SUCCESS;
  }

  //
  // Read blob.
  //
  Blob->Data = AllocatePool (Blob->Size);
  if (Blob->Data == NULL) {
    DEBUG ((EFI_D_ERROR, "%a: failed to allocate %Ld bytes for \"%s\"\n",
      __FUNCTION__, (INT64)Blob->Size, Blob->Name));
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((EFI_D_INFO, "%a: loading %Ld bytes for \"%s\"\n", __FUNCTION__,
    (INT64)Blob->Size, Blob->Name));
  QemuFwCfgSelectItem (Blob->DataKey);

  Left = Blob->Size;
  do {
    UINT32 Chunk;

    Chunk = (Left < SIZE_1MB) ? Left : SIZE_1MB;
    QemuFwCfgReadBytes (Chunk, Blob->Data + (Blob->Size - Left));
    Left -= Chunk;
    DEBUG ((EFI_D_VERBOSE, "%a: %Ld bytes remaining for \"%s\"\n",
      __FUNCTION__, (INT64)Left, Blob->Name));
  } while (Left > 0);
  return EFI_SUCCESS;
}


//
// The entry point of the feature.
//

/**
  Download the kernel, the initial ramdisk, and the kernel command line from
  QEMU's fw_cfg. Construct a minimal SimpleFileSystem that contains the two
  image files, and load and start the kernel from it.

  The kernel will be instructed via its command line to load the initrd from
  the same Simple FileSystem.

  @retval EFI_NOT_FOUND         Kernel image was not found.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
  @retval EFI_PROTOCOL_ERROR    Unterminated kernel command line.

  @return                       Error codes from any of the underlying
                                functions. On success, the function doesn't
                                return.
**/
EFI_STATUS
EFIAPI
TryRunningQemuKernel (
  VOID
  )
{
  UINTN                     BlobType;
  KERNEL_BLOB               *CurrentBlob;
  KERNEL_BLOB               *KernelBlob, *InitrdBlob, *CommandLineBlob;
  EFI_STATUS                Status;
  EFI_HANDLE                FileSystemHandle;
  EFI_DEVICE_PATH_PROTOCOL  *KernelDevicePath;
  EFI_HANDLE                KernelImageHandle;
  EFI_LOADED_IMAGE_PROTOCOL *KernelLoadedImage;

  Status = gRT->GetTime (&mInitTime, NULL /* Capabilities */);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: GetTime(): %r\n", __FUNCTION__, Status));
    return Status;
  }

  //
  // Fetch all blobs.
  //
  for (BlobType = 0; BlobType < KernelBlobTypeMax; ++BlobType) {
    CurrentBlob = &mKernelBlob[BlobType];
    Status = FetchBlob (CurrentBlob);
    if (EFI_ERROR (Status)) {
      goto FreeBlobs;
    }
    mTotalBlobBytes += CurrentBlob->Size;
  }
  KernelBlob      = &mKernelBlob[KernelBlobTypeKernel];
  InitrdBlob      = &mKernelBlob[KernelBlobTypeInitrd];
  CommandLineBlob = &mKernelBlob[KernelBlobTypeCommandLine];

  if (KernelBlob->Data == NULL) {
    Status = EFI_NOT_FOUND;
    goto FreeBlobs;
  }

  //
  // Create a new handle with a single VenHw() node device path protocol on it,
  // plus a custom SimpleFileSystem protocol on it.
  //
  FileSystemHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (&FileSystemHandle,
                  &gEfiDevicePathProtocolGuid,       &mFileSystemDevicePath,
                  &gEfiSimpleFileSystemProtocolGuid, &mFileSystem,
                  NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: InstallMultipleProtocolInterfaces(): %r\n",
      __FUNCTION__, Status));
    goto FreeBlobs;
  }

  //
  // Create a device path for the kernel image to be loaded from that will call
  // back into our file system.
  //
  KernelDevicePath = FileDevicePath (FileSystemHandle, KernelBlob->Name);
  if (KernelDevicePath == NULL) {
    DEBUG ((EFI_D_ERROR, "%a: failed to allocate kernel device path\n",
      __FUNCTION__));
    Status = EFI_OUT_OF_RESOURCES;
    goto UninstallProtocols;
  }

  //
  // Load the image. This should call back into our file system.
  //
  Status = gBS->LoadImage (
                  FALSE,             // BootPolicy: exact match required
                  gImageHandle,      // ParentImageHandle
                  KernelDevicePath,
                  NULL,              // SourceBuffer
                  0,                 // SourceSize
                  &KernelImageHandle
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: LoadImage(): %r\n", __FUNCTION__, Status));
    goto FreeKernelDevicePath;
  }

  //
  // Construct the kernel command line.
  //
  Status = gBS->OpenProtocol (
                  KernelImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&KernelLoadedImage,
                  gImageHandle,                  // AgentHandle
                  NULL,                          // ControllerHandle
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  ASSERT_EFI_ERROR (Status);

  if (CommandLineBlob->Data == NULL) {
    KernelLoadedImage->LoadOptionsSize = 0;
  } else {
    //
    // Verify NUL-termination of the command line.
    //
    if (CommandLineBlob->Data[CommandLineBlob->Size - 1] != '\0') {
      DEBUG ((EFI_D_ERROR, "%a: kernel command line is not NUL-terminated\n",
        __FUNCTION__));
      Status = EFI_PROTOCOL_ERROR;
      goto UnloadKernelImage;
    }

    //
    // Drop the terminating NUL, convert to UTF-16.
    //
    KernelLoadedImage->LoadOptionsSize = (CommandLineBlob->Size - 1) * 2;
  }

  if (InitrdBlob->Data != NULL) {
    //
    // Append ' initrd=<name>' in UTF-16.
    //
    KernelLoadedImage->LoadOptionsSize +=
                                        (8 + StrLen(InitrdBlob->Name)) * 2;
  }

  if (KernelLoadedImage->LoadOptionsSize == 0) {
    KernelLoadedImage->LoadOptions = NULL;
  } else {
    //
    // NUL-terminate in UTF-16.
    //
    KernelLoadedImage->LoadOptionsSize += 2;

    KernelLoadedImage->LoadOptions = AllocatePool (
                                       KernelLoadedImage->LoadOptionsSize);
    if (KernelLoadedImage->LoadOptions == NULL) {
      KernelLoadedImage->LoadOptionsSize = 0;
      Status = EFI_OUT_OF_RESOURCES;
      goto UnloadKernelImage;
    }

    UnicodeSPrintAsciiFormat (
      KernelLoadedImage->LoadOptions,
      KernelLoadedImage->LoadOptionsSize,
      "%a%a%s",
      (CommandLineBlob->Data == NULL) ?  "" : (CHAR8 *)CommandLineBlob->Data,
      (InitrdBlob->Data      == NULL) ?  "" : " initrd=",
      (InitrdBlob->Data      == NULL) ? L"" : InitrdBlob->Name
      );
    DEBUG ((EFI_D_INFO, "%a: command line: \"%s\"\n", __FUNCTION__,
      (CHAR16 *)KernelLoadedImage->LoadOptions));
  }

  //
  // Start the image.
  //
  Status = gBS->StartImage (
                KernelImageHandle,
                NULL,              // ExitDataSize
                NULL               // ExitData
                );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: StartImage(): %r\n", __FUNCTION__, Status));
  }

  if (KernelLoadedImage->LoadOptions != NULL) {
    FreePool (KernelLoadedImage->LoadOptions);
  }
  KernelLoadedImage->LoadOptionsSize = 0;

UnloadKernelImage:
  gBS->UnloadImage (KernelImageHandle);

FreeKernelDevicePath:
  FreePool (KernelDevicePath);

UninstallProtocols:
  gBS->UninstallMultipleProtocolInterfaces (FileSystemHandle,
         &gEfiSimpleFileSystemProtocolGuid, &mFileSystem,
         &gEfiDevicePathProtocolGuid,       &mFileSystemDevicePath,
         NULL);

FreeBlobs:
  while (BlobType > 0) {
    CurrentBlob = &mKernelBlob[--BlobType];
    if (CurrentBlob->Data != NULL) {
      FreePool (CurrentBlob->Data);
      CurrentBlob->Size = 0;
      CurrentBlob->Data = NULL;
    }
  }

  return Status;
}
