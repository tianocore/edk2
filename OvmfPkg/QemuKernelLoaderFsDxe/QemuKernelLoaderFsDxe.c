/** @file
  DXE driver to expose the 'kernel', 'initrd' and 'cmdline' blobs
  provided by QEMU as files in an abstract file system

  Copyright (C) 2014-2016, Red Hat, Inc.
  Copyright (C) 2020, Arm, Limited.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>

#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>
#include <Guid/FileSystemVolumeLabelInfo.h>
#include <Guid/LinuxEfiInitrdMedia.h>
#include <Guid/QemuKernelLoaderFsMedia.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BlobVerifierLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LoadFile2.h>
#include <Protocol/SimpleFileSystem.h>

//
// Static data that hosts the fw_cfg blobs and serves file requests.
//
typedef struct {
  CHAR16    Name[48];
  struct {
    FIRMWARE_CONFIG_ITEM    SizeKey;
    FIRMWARE_CONFIG_ITEM    DataKey;
    UINT32                  Size;
  }                         FwCfgItem[2];
} KERNEL_BLOB_ITEMS;

typedef struct KERNEL_BLOB KERNEL_BLOB;
struct KERNEL_BLOB {
  CHAR16         Name[48];
  UINT32         Size;
  UINT8          *Data;
  KERNEL_BLOB    *Next;
};

STATIC KERNEL_BLOB_ITEMS  mKernelBlobItems[] = {
  {
    L"kernel",
    {
      { QemuFwCfgItemKernelSetupSize, QemuFwCfgItemKernelSetupData, },
      { QemuFwCfgItemKernelSize,      QemuFwCfgItemKernelData,      },
    }
  },  {
    L"initrd",
    {
      { QemuFwCfgItemInitrdSize,      QemuFwCfgItemInitrdData,      },
    }
  },  {
    L"cmdline",
    {
      { QemuFwCfgItemCommandLineSize, QemuFwCfgItemCommandLineData, },
    }
  }
};

STATIC KERNEL_BLOB  *mKernelBlobs;
STATIC UINT64       mKernelBlobCount;
STATIC UINT64       mKernelNamedBlobCount;
STATIC UINT64       mTotalBlobBytes;

//
// Device path for the handle that incorporates our "EFI stub filesystem".
//
#pragma pack (1)
typedef struct {
  VENDOR_DEVICE_PATH          VenMediaNode;
  EFI_DEVICE_PATH_PROTOCOL    EndNode;
} SINGLE_VENMEDIA_NODE_DEVPATH;
#pragma pack ()

STATIC CONST SINGLE_VENMEDIA_NODE_DEVPATH  mFileSystemDevicePath = {
  {
    {
      MEDIA_DEVICE_PATH, MEDIA_VENDOR_DP,
      { sizeof (VENDOR_DEVICE_PATH)       }
    },
    QEMU_KERNEL_LOADER_FS_MEDIA_GUID
  },  {
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { sizeof (EFI_DEVICE_PATH_PROTOCOL) }
  }
};

STATIC CONST SINGLE_VENMEDIA_NODE_DEVPATH  mInitrdDevicePath = {
  {
    {
      MEDIA_DEVICE_PATH, MEDIA_VENDOR_DP,
      { sizeof (VENDOR_DEVICE_PATH)       }
    },
    LINUX_EFI_INITRD_MEDIA_GUID
  },  {
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { sizeof (EFI_DEVICE_PATH_PROTOCOL) }
  }
};

//
// The "file in the EFI stub filesystem" abstraction.
//
STATIC EFI_TIME  mInitTime;

#define STUB_FILE_SIG  SIGNATURE_64 ('S', 'T', 'U', 'B', 'F', 'I', 'L', 'E')

typedef struct {
  UINT64               Signature; // Carries STUB_FILE_SIG.

  KERNEL_BLOB          *Blob;    // Index into mKernelBlob. KernelBlobTypeMax
                                 // denotes the root directory of the filesystem.

  UINT64               Position; // Byte position for regular files;
                                 // next directory entry to return for the root
                                 // directory.

  EFI_FILE_PROTOCOL    File;   // Standard protocol interface.
} STUB_FILE;

#define STUB_FILE_FROM_FILE(FilePointer) \
        CR (FilePointer, STUB_FILE, File, STUB_FILE_SIG)

//
// Protocol member functions for File.
//

/**
  Opens a new file relative to the source file's location.

  (Forward declaration.)

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
QemuKernelStubFileOpen (
  IN EFI_FILE_PROTOCOL   *This,
  OUT EFI_FILE_PROTOCOL  **NewHandle,
  IN CHAR16              *FileName,
  IN UINT64              OpenMode,
  IN UINT64              Attributes
  );

/**
  Closes a specified file handle.

  @param[in] This  A pointer to the EFI_FILE_PROTOCOL instance that is the file
                   handle to close.

  @retval EFI_SUCCESS  The file was closed.
**/
STATIC
EFI_STATUS
EFIAPI
QemuKernelStubFileClose (
  IN EFI_FILE_PROTOCOL  *This
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
QemuKernelStubFileDelete (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  FreePool (STUB_FILE_FROM_FILE (This));
  return EFI_WARN_DELETE_FAILURE;
}

/**
  Helper function that formats an EFI_FILE_INFO structure into the
  user-allocated buffer, for any valid KERNEL_BLOB (including NULL,
  which stands for the root directory).

  The interface follows the EFI_FILE_GET_INFO -- and for directories, the
  EFI_FILE_READ -- interfaces.

  @param[in]     Blob         The KERNEL_BLOB identifying the fw_cfg
                              blob backing the STUB_FILE that information is
                              being requested about. If Blob is NULL,
                              then information will be provided about the root
                              directory of the filesystem.

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
QemuKernelBlobTypeToFileInfo (
  IN KERNEL_BLOB  *Blob,
  IN OUT UINTN    *BufferSize,
  OUT VOID        *Buffer
  )
{
  CONST CHAR16  *Name;
  UINT64        FileSize;
  UINT64        Attribute;

  UINTN          NameSize;
  UINTN          FileInfoSize;
  EFI_FILE_INFO  *FileInfo;
  UINTN          OriginalBufferSize;

  if (Blob == NULL) {
    //
    // getting file info about the root directory
    //
    DEBUG ((DEBUG_INFO, "%a: file info: directory\n", __func__));
    Name      = L"";
    FileSize  = mKernelBlobCount;
    Attribute = EFI_FILE_READ_ONLY | EFI_FILE_DIRECTORY;
  } else {
    DEBUG ((DEBUG_INFO, "%a: file info: \"%s\"\n", __func__, Blob->Name));
    Name      = Blob->Name;
    FileSize  = Blob->Size;
    Attribute = EFI_FILE_READ_ONLY;
  }

  NameSize     = (StrLen (Name) + 1) * 2;
  FileInfoSize = SIZE_OF_EFI_FILE_INFO + NameSize;

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

  CopyMem (&FileInfo->CreateTime, &mInitTime, sizeof mInitTime);
  CopyMem (&FileInfo->LastAccessTime, &mInitTime, sizeof mInitTime);
  CopyMem (&FileInfo->ModificationTime, &mInitTime, sizeof mInitTime);
  CopyMem (FileInfo->FileName, Name, NameSize);

  return EFI_SUCCESS;
}

STATIC
KERNEL_BLOB *
FindKernelBlob (
  CHAR16  *FileName
  )
{
  KERNEL_BLOB  *Blob;

  for (Blob = mKernelBlobs; Blob != NULL; Blob = Blob->Next) {
    if (StrCmp (FileName, Blob->Name) == 0) {
      return Blob;
    }
  }

  return NULL;
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
QemuKernelStubFileRead (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  )
{
  STUB_FILE    *StubFile;
  KERNEL_BLOB  *Blob;
  UINT64       Left, Pos;

  StubFile = STUB_FILE_FROM_FILE (This);

  //
  // Scanning the root directory?
  //
  if (StubFile->Blob == NULL) {
    EFI_STATUS  Status;

    if (StubFile->Position == mKernelBlobCount) {
      //
      // Scanning complete.
      //
      *BufferSize = 0;
      return EFI_SUCCESS;
    }

    for (Pos = 0, Blob = mKernelBlobs;
         Pos < StubFile->Position;
         Pos++, Blob = Blob->Next)
    {
    }

    DEBUG ((DEBUG_INFO, "%a: file list: #%d \"%s\"\n", __func__, Pos, Blob->Name));

    Status = QemuKernelBlobTypeToFileInfo (
               Blob,
               BufferSize,
               Buffer
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    ++StubFile->Position;
    return EFI_SUCCESS;
  }

  //
  // Reading a file.
  //
  Blob = StubFile->Blob;
  if (StubFile->Position > Blob->Size) {
    return EFI_DEVICE_ERROR;
  }

  Left = Blob->Size - StubFile->Position;
  if (*BufferSize > Left) {
    *BufferSize = (UINTN)Left;
  }

  if (Blob->Data != NULL) {
    DEBUG ((DEBUG_INFO, "%a: file read: \"%s\", %d bytes\n", __func__, Blob->Name, *BufferSize));
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
QemuKernelStubFileWrite (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  IN VOID               *Buffer
  )
{
  STUB_FILE  *StubFile;

  StubFile = STUB_FILE_FROM_FILE (This);
  return (StubFile->Blob == NULL) ?
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
QemuKernelStubFileGetPosition (
  IN EFI_FILE_PROTOCOL  *This,
  OUT UINT64            *Position
  )
{
  STUB_FILE  *StubFile;

  StubFile = STUB_FILE_FROM_FILE (This);
  if (StubFile->Blob == NULL) {
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
QemuKernelStubFileSetPosition (
  IN EFI_FILE_PROTOCOL  *This,
  IN UINT64             Position
  )
{
  STUB_FILE    *StubFile;
  KERNEL_BLOB  *Blob;

  StubFile = STUB_FILE_FROM_FILE (This);

  if (StubFile->Blob == NULL) {
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
  Blob = StubFile->Blob;
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
QemuKernelStubFileGetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  )
{
  CONST STUB_FILE  *StubFile;
  UINTN            OriginalBufferSize;

  StubFile = STUB_FILE_FROM_FILE (This);

  if (CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    return QemuKernelBlobTypeToFileInfo (
             StubFile->Blob,
             BufferSize,
             Buffer
             );
  }

  OriginalBufferSize = *BufferSize;

  if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid)) {
    EFI_FILE_SYSTEM_INFO  *FileSystemInfo;

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
    EFI_FILE_SYSTEM_VOLUME_LABEL  *FileSystemVolumeLabel;

    *BufferSize = sizeof *FileSystemVolumeLabel;
    if (OriginalBufferSize < *BufferSize) {
      return EFI_BUFFER_TOO_SMALL;
    }

    FileSystemVolumeLabel                 = (EFI_FILE_SYSTEM_VOLUME_LABEL *)Buffer;
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
QemuKernelStubFileSetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN UINTN              BufferSize,
  IN VOID               *Buffer
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
QemuKernelStubFileFlush (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  return EFI_WRITE_PROTECTED;
}

//
// External definition of the file protocol template.
//
STATIC CONST EFI_FILE_PROTOCOL  mEfiFileProtocolTemplate = {
  EFI_FILE_PROTOCOL_REVISION, // revision 1
  QemuKernelStubFileOpen,
  QemuKernelStubFileClose,
  QemuKernelStubFileDelete,
  QemuKernelStubFileRead,
  QemuKernelStubFileWrite,
  QemuKernelStubFileGetPosition,
  QemuKernelStubFileSetPosition,
  QemuKernelStubFileGetInfo,
  QemuKernelStubFileSetInfo,
  QemuKernelStubFileFlush,
  NULL,                       // OpenEx, revision 2
  NULL,                       // ReadEx, revision 2
  NULL,                       // WriteEx, revision 2
  NULL                        // FlushEx, revision 2
};

STATIC
EFI_STATUS
EFIAPI
QemuKernelStubFileOpen (
  IN EFI_FILE_PROTOCOL   *This,
  OUT EFI_FILE_PROTOCOL  **NewHandle,
  IN CHAR16              *FileName,
  IN UINT64              OpenMode,
  IN UINT64              Attributes
  )
{
  CONST STUB_FILE  *StubFile;
  KERNEL_BLOB      *Blob;
  STUB_FILE        *NewStubFile;

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
  if (StubFile->Blob != NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // Locate the file.
  //
  if (FileName[0] == '\\') {
    // also accept absolute paths, i.e. '\kernel' for 'kernel'
    FileName++;
  }

  Blob = FindKernelBlob (FileName);

  if (Blob == NULL) {
    DEBUG ((DEBUG_INFO, "%a: file not found: \"%s\"\n", __func__, FileName));
    return EFI_NOT_FOUND;
  } else {
    DEBUG ((DEBUG_INFO, "%a: file opened: \"%s\"\n", __func__, FileName));
  }

  //
  // Found it.
  //
  NewStubFile = AllocatePool (sizeof *NewStubFile);
  if (NewStubFile == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewStubFile->Signature = STUB_FILE_SIG;
  NewStubFile->Blob      = Blob;
  NewStubFile->Position  = 0;
  CopyMem (
    &NewStubFile->File,
    &mEfiFileProtocolTemplate,
    sizeof mEfiFileProtocolTemplate
    );
  *NewHandle = &NewStubFile->File;

  return EFI_SUCCESS;
}

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
QemuKernelStubFileSystemOpenVolume (
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL               **Root
  )
{
  STUB_FILE  *StubFile;

  StubFile = AllocatePool (sizeof *StubFile);
  if (StubFile == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  StubFile->Signature = STUB_FILE_SIG;
  StubFile->Blob      = NULL;
  StubFile->Position  = 0;
  CopyMem (
    &StubFile->File,
    &mEfiFileProtocolTemplate,
    sizeof mEfiFileProtocolTemplate
    );
  *Root = &StubFile->File;

  return EFI_SUCCESS;
}

STATIC CONST EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  mFileSystem = {
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION,
  QemuKernelStubFileSystemOpenVolume
};

STATIC
EFI_STATUS
EFIAPI
QemuKernelInitrdLoadFile2 (
  IN      EFI_LOAD_FILE2_PROTOCOL   *This,
  IN      EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN      BOOLEAN                   BootPolicy,
  IN  OUT UINTN                     *BufferSize,
  OUT     VOID                      *Buffer     OPTIONAL
  )
{
  KERNEL_BLOB  *InitrdBlob;

  DEBUG ((DEBUG_INFO, "%a: initrd read\n", __func__));
  InitrdBlob = FindKernelBlob (L"initrd");
  ASSERT (InitrdBlob != NULL);
  ASSERT (InitrdBlob->Size > 0);

  if (BootPolicy) {
    return EFI_UNSUPPORTED;
  }

  if ((BufferSize == NULL) || !IsDevicePathValid (FilePath, 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((FilePath->Type != END_DEVICE_PATH_TYPE) ||
      (FilePath->SubType != END_ENTIRE_DEVICE_PATH_SUBTYPE))
  {
    return EFI_NOT_FOUND;
  }

  if ((Buffer == NULL) || (*BufferSize < InitrdBlob->Size)) {
    *BufferSize = InitrdBlob->Size;
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (Buffer, InitrdBlob->Data, InitrdBlob->Size);

  *BufferSize = InitrdBlob->Size;
  return EFI_SUCCESS;
}

STATIC CONST EFI_LOAD_FILE2_PROTOCOL  mInitrdLoadFile2 = {
  QemuKernelInitrdLoadFile2,
};

//
// Utility functions.
//

STATIC VOID
QemuKernelChunkedRead (
  UINT8   *Dest,
  UINT32  Bytes
  )
{
  UINT32  Chunk;

  while (Bytes > 0) {
    Chunk = (Bytes < SIZE_1MB) ? Bytes : SIZE_1MB;
    QemuFwCfgReadBytes (Chunk, Dest);
    Bytes -= Chunk;
    Dest  += Chunk;
  }
}

/**
  Populate a blob in mKernelBlob.

  param[in,out] Blob  Pointer to the KERNEL_BLOB_ITEMS that is
                      to be filled from fw_cfg.

  @retval EFI_SUCCESS           Blob has been populated. If fw_cfg reported a
                                size of zero for the blob, then Blob->Data has
                                been left unchanged.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for Blob->Data.
**/
STATIC
EFI_STATUS
QemuKernelFetchBlob (
  IN KERNEL_BLOB_ITEMS  *BlobItems
  )
{
  UINT32       Size;
  UINTN        Idx;
  UINT8        *ChunkData;
  KERNEL_BLOB  *Blob;
  EFI_STATUS   Status;

  //
  // Read blob size.
  //   Size != 0      ->  use size as-is
  //   SizeKey != 0   ->  read size from fw_cfg
  //   both are 0     ->  unused entry
  //
  for (Size = 0, Idx = 0; Idx < ARRAY_SIZE (BlobItems->FwCfgItem); Idx++) {
    if ((BlobItems->FwCfgItem[Idx].SizeKey == 0) &&
        (BlobItems->FwCfgItem[Idx].Size == 0))
    {
      break;
    }

    if (BlobItems->FwCfgItem[Idx].SizeKey) {
      QemuFwCfgSelectItem (BlobItems->FwCfgItem[Idx].SizeKey);
      BlobItems->FwCfgItem[Idx].Size = QemuFwCfgRead32 ();
    }

    Size += BlobItems->FwCfgItem[Idx].Size;
  }

  if (Size == 0) {
    return EFI_SUCCESS;
  }

  Blob = AllocatePool (sizeof (*Blob));
  if (Blob->Data == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (Blob, sizeof (*Blob));

  //
  // Read blob.
  //
  Status = StrCpyS (Blob->Name, sizeof (Blob->Name), BlobItems->Name);
  ASSERT (!EFI_ERROR (Status));
  Blob->Size = Size;
  Blob->Data = AllocatePool (Blob->Size);
  if (Blob->Data == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: failed to allocate %Ld bytes for \"%s\"\n",
      __func__,
      (INT64)Blob->Size,
      Blob->Name
      ));
    FreePool (Blob);
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: loading %Ld bytes for \"%s\"\n",
    __func__,
    (INT64)Blob->Size,
    Blob->Name
    ));

  ChunkData = Blob->Data;
  for (Idx = 0; Idx < ARRAY_SIZE (BlobItems->FwCfgItem); Idx++) {
    if (BlobItems->FwCfgItem[Idx].DataKey == 0) {
      break;
    }

    QemuFwCfgSelectItem (BlobItems->FwCfgItem[Idx].DataKey);
    QemuKernelChunkedRead (ChunkData, BlobItems->FwCfgItem[Idx].Size);
    ChunkData += BlobItems->FwCfgItem[Idx].Size;
  }

  Blob->Next   = mKernelBlobs;
  mKernelBlobs = Blob;
  mKernelBlobCount++;
  mTotalBlobBytes += Blob->Size;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
QemuKernelVerifyBlob (
  CHAR16      *FileName,
  EFI_STATUS  FetchStatus
  )
{
  KERNEL_BLOB  *Blob;
  EFI_STATUS   Status;

  if ((StrCmp (FileName, L"kernel") != 0) &&
      (StrCmp (FileName, L"initrd") != 0) &&
      (StrCmp (FileName, L"cmdline") != 0))
  {
    return EFI_SUCCESS;
  }

  Blob   = FindKernelBlob (FileName);
  Status = VerifyBlob (
             FileName,
             Blob ? Blob->Data : NULL,
             Blob ? Blob->Size : 0,
             FetchStatus
             );
  return Status;
}

STATIC
EFI_STATUS
QemuKernelFetchNamedBlobs (
  VOID
  )
{
  struct {
    UINT32    FileSize;
    UINT16    FileSelect;
    UINT16    Reserved;
    CHAR8     FileName[QEMU_FW_CFG_FNAME_SIZE];
  } *DirEntry;
  KERNEL_BLOB_ITEMS  Items;
  EFI_STATUS         Status;
  EFI_STATUS         FetchStatus;
  UINT32             Count;
  UINT32             Idx;

  QemuFwCfgSelectItem (QemuFwCfgItemFileDir);
  Count = SwapBytes32 (QemuFwCfgRead32 ());

  DirEntry = AllocatePool (sizeof (*DirEntry) * Count);
  QemuFwCfgReadBytes (sizeof (*DirEntry) * Count, DirEntry);

  for (Idx = 0; Idx < Count; ++Idx) {
    if (AsciiStrnCmp (DirEntry[Idx].FileName, "etc/boot/", 9) != 0) {
      continue;
    }

    ZeroMem (&Items, sizeof (Items));
    UnicodeSPrint (Items.Name, sizeof (Items.Name), L"%a", DirEntry[Idx].FileName + 9);
    Items.FwCfgItem[0].DataKey = SwapBytes16 (DirEntry[Idx].FileSelect);
    Items.FwCfgItem[0].Size    = SwapBytes32 (DirEntry[Idx].FileSize);

    FetchStatus = QemuKernelFetchBlob (&Items);
    Status      = QemuKernelVerifyBlob (
                    (CHAR16 *)Items.Name,
                    FetchStatus
                    );
    if (EFI_ERROR (Status)) {
      FreePool (DirEntry);
      return Status;
    }

    mKernelNamedBlobCount++;
  }

  FreePool (DirEntry);
  return EFI_SUCCESS;
}

//
// The entry point of the feature.
//

/**
  Download the kernel, the initial ramdisk, and the kernel command line from
  QEMU's fw_cfg. Construct a minimal SimpleFileSystem that contains the two
  image files.

  @retval EFI_NOT_FOUND         Kernel image was not found.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
  @retval EFI_PROTOCOL_ERROR    Unterminated kernel command line.

  @return                       Error codes from any of the underlying
                                functions. On success, the function doesn't
                                return.
**/
EFI_STATUS
EFIAPI
QemuKernelLoaderFsDxeEntrypoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINTN              BlobIdx;
  KERNEL_BLOB_ITEMS  *BlobItems;
  KERNEL_BLOB        *Blob;
  EFI_STATUS         Status;
  EFI_STATUS         FetchStatus;
  EFI_HANDLE         FileSystemHandle;
  EFI_HANDLE         InitrdLoadFile2Handle;

  if (!QemuFwCfgIsAvailable ()) {
    return EFI_NOT_FOUND;
  }

  Status = gRT->GetTime (&mInitTime, NULL /* Capabilities */);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: GetTime(): %r\n", __func__, Status));
    return Status;
  }

  //
  // Fetch named blobs.
  //
  DEBUG ((DEBUG_INFO, "%a: named blobs (etc/boot/*)\n", __func__));
  Status = QemuKernelFetchNamedBlobs ();
  if (EFI_ERROR (Status)) {
    goto FreeBlobs;
  }

  //
  // Fetch traditional blobs.
  //
  DEBUG ((DEBUG_INFO, "%a: traditional blobs\n", __func__));
  for (BlobIdx = 0; BlobIdx < ARRAY_SIZE (mKernelBlobItems); ++BlobIdx) {
    BlobItems = &mKernelBlobItems[BlobIdx];
    if (FindKernelBlob (BlobItems->Name)) {
      continue;
    }

    FetchStatus = QemuKernelFetchBlob (BlobItems);

    Status = QemuKernelVerifyBlob (
               (CHAR16 *)BlobItems->Name,
               FetchStatus
               );
    if (EFI_ERROR (Status)) {
      goto FreeBlobs;
    }
  }

  Blob = FindKernelBlob (L"kernel");
  if ((Blob == NULL) && (mKernelNamedBlobCount == 0)) {
    DEBUG ((DEBUG_INFO, "%a: no kernel and no named blobs present -> quit\n", __func__));
    Status = EFI_NOT_FOUND;
    goto FreeBlobs;
  }

  //
  // Create a new handle with a single VenMedia() node device path protocol on
  // it, plus a custom SimpleFileSystem protocol on it.
  //
  FileSystemHandle = NULL;
  Status           = gBS->InstallMultipleProtocolInterfaces (
                            &FileSystemHandle,
                            &gEfiDevicePathProtocolGuid,
                            &mFileSystemDevicePath,
                            &gEfiSimpleFileSystemProtocolGuid,
                            &mFileSystem,
                            NULL
                            );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: InstallMultipleProtocolInterfaces(): %r\n",
      __func__,
      Status
      ));
    goto FreeBlobs;
  }

  Blob = FindKernelBlob (L"initrd");
  if (Blob != NULL) {
    DEBUG ((DEBUG_INFO, "%a: initrd setup\n", __func__));
    InitrdLoadFile2Handle = NULL;
    Status                = gBS->InstallMultipleProtocolInterfaces (
                                   &InitrdLoadFile2Handle,
                                   &gEfiDevicePathProtocolGuid,
                                   &mInitrdDevicePath,
                                   &gEfiLoadFile2ProtocolGuid,
                                   &mInitrdLoadFile2,
                                   NULL
                                   );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: InstallMultipleProtocolInterfaces(): %r\n",
        __func__,
        Status
        ));
      goto UninstallFileSystemHandle;
    }
  }

  return EFI_SUCCESS;

UninstallFileSystemHandle:
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  FileSystemHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mFileSystemDevicePath,
                  &gEfiSimpleFileSystemProtocolGuid,
                  &mFileSystem,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

FreeBlobs:
  while (mKernelBlobs != NULL) {
    Blob         = mKernelBlobs;
    mKernelBlobs = Blob->Next;
    FreePool (Blob->Data);
    FreePool (Blob);
  }

  return Status;
}
