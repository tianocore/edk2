/** @file
  DXE driver to expose the 'kernel', 'initrd' and 'cmdline' blobs
  provided by Cloud Hypervisor as files in an abstract file system

  Copyright (C) 2022, Arm, Limited.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <PiDxe.h>

#include <libfdt.h>
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
#include <Library/PcdLib.h>
#include <Library/PrePiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LoadFile2.h>
#include <Protocol/SimpleFileSystem.h>

//
// Static data that hosts the data blobs and serves file requests.
//
typedef enum {
  KernelBlobTypeKernel,
  KernelBlobTypeInitrd,
  KernelBlobTypeCommandLine,
  KernelBlobTypeMax
} KERNEL_BLOB_TYPE;

typedef struct {
  CONST CHAR16    Name[8];
  UINT32          Size;
  UINT8           *Data;
} KERNEL_BLOB;

STATIC KERNEL_BLOB  mKernelBlob[KernelBlobTypeMax] = {
  {
    L"kernel",
  },{
    L"initrd",
  },{
    L"cmdline",
  }
};

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
STATIC UINT64    mTotalBlobBytes;

#define STUB_FILE_SIG  SIGNATURE_64 ('S', 'T', 'U', 'B', 'F', 'I', 'L', 'E')

typedef struct {
  UINT64               Signature; // Carries STUB_FILE_SIG.

  KERNEL_BLOB_TYPE     BlobType; // Index into mKernelBlob. KernelBlobTypeMax
                                 // denotes the root directory of the filesystem.

  UINT64               Position; // Byte position for regular files;
                                 // next directory entry to return for the root
                                 // directory.

  EFI_FILE_PROTOCOL    File;   // Standard protocol interface.
} STUB_FILE;

#define STUB_FILE_FROM_FILE(FilePointer) \
        CR (FilePointer, STUB_FILE, File, STUB_FILE_SIG)

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
EFI_STATUS
ConvertKernelBlobTypeToFileInfo (
  IN KERNEL_BLOB_TYPE  BlobType,
  IN OUT UINTN         *BufferSize,
  OUT VOID             *Buffer
  )
{
  CONST CHAR16  *Name;
  UINT64        FileSize;
  UINT64        Attribute;

  UINTN          NameSize;
  UINTN          FileInfoSize;
  EFI_FILE_INFO  *FileInfo;
  UINTN          OriginalBufferSize;

  if (BlobType == KernelBlobTypeMax) {
    //
    // getting file info about the root directory
    //
    Name      = L"\\";
    FileSize  = KernelBlobTypeMax;
    Attribute = EFI_FILE_READ_ONLY | EFI_FILE_DIRECTORY;
  } else {
    CONST KERNEL_BLOB  *Blob;

    Blob      = &mKernelBlob[BlobType];
    Name      = Blob->Name;
    FileSize  = Blob->Size;
    Attribute = EFI_FILE_READ_ONLY;
  }

  NameSize     = (StrLen (Name) + 1) * 2;
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

  CopyMem (&FileInfo->CreateTime, &mInitTime, sizeof mInitTime);
  CopyMem (&FileInfo->LastAccessTime, &mInitTime, sizeof mInitTime);
  CopyMem (&FileInfo->ModificationTime, &mInitTime, sizeof mInitTime);
  CopyMem (FileInfo->FileName, Name, NameSize);

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
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  )
{
  STUB_FILE          *StubFile;
  CONST KERNEL_BLOB  *Blob;
  UINT64             Left;

  StubFile = STUB_FILE_FROM_FILE (This);

  //
  // Scanning the root directory?
  //
  if (StubFile->BlobType == KernelBlobTypeMax) {
    EFI_STATUS  Status;

    if (StubFile->Position == KernelBlobTypeMax) {
      //
      // Scanning complete.
      //
      *BufferSize = 0;
      return EFI_SUCCESS;
    }

    Status = ConvertKernelBlobTypeToFileInfo (
               (KERNEL_BLOB_TYPE)StubFile->Position,
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
  Closes a specified file handle.

  @param[in] This  A pointer to the EFI_FILE_PROTOCOL instance that is the file
                   handle to close.

  @retval EFI_SUCCESS  The file was closed.
**/
STATIC
EFI_STATUS
EFIAPI
StubFileClose (
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
StubFileDelete (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  FreePool (STUB_FILE_FROM_FILE (This));
  return EFI_WARN_DELETE_FAILURE;
}

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
StubFileOpen (
  IN EFI_FILE_PROTOCOL   *This,
  OUT EFI_FILE_PROTOCOL  **NewHandle,
  IN CHAR16              *FileName,
  IN UINT64              OpenMode,
  IN UINT64              Attributes
  );

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
    return ConvertKernelBlobTypeToFileInfo (
             StubFile->BlobType,
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

//
// External definition of the file protocol template.
//
STATIC CONST EFI_FILE_PROTOCOL  mEfiFileProtocolTemplate = {
  EFI_FILE_PROTOCOL_REVISION, // revision 1
  StubFileOpen,
  StubFileClose,
  StubFileDelete,
  StubFileRead,
  NULL,
  NULL,
  NULL,
  StubFileGetInfo,
  NULL,
  NULL,
  NULL,                       // OpenEx, revision 2
  NULL,                       // ReadEx, revision 2
  NULL,                       // WriteEx, revision 2
  NULL                        // FlushEx, revision 2
};

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
StubFileOpen (
  IN EFI_FILE_PROTOCOL   *This,
  OUT EFI_FILE_PROTOCOL  **NewHandle,
  IN CHAR16              *FileName,
  IN UINT64              OpenMode,
  IN UINT64              Attributes
  )
{
  CONST STUB_FILE  *StubFile;
  UINTN            BlobType;
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
StubFileSystemOpenVolume (
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
  StubFile->BlobType  = KernelBlobTypeMax;
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
  StubFileSystemOpenVolume
};

/**
  Load initrd data to memory

  @param[in]      This        Not used here.
  @param[in]      FilePath    Contain some metadata of file device where holds the initrd.
  @param[in]      BootPolicy  Not support here.
  @param[in,out]  BufferSize  On input, contains the memory buff size.
                              On output, contains the initrd blob size.
  @param[out]     Buffer      A pointer point to memory which contains initrd data.

  @retval EFI_UNSUPPORTED        Once the Boot Policy is true.
  @retval EFI_INVALID_PARAMETER  Once there is no BuffSize given or no available file device.
  @retval EFI_NOT_FOUND          File path doesn't match.
  @retval EFI_BUFFER_TOO_SMALL   Buff size too small.
  @retval EFI_SUCCESS            Success.
**/
STATIC
EFI_STATUS
EFIAPI
InitrdLoadFile2 (
  IN      EFI_LOAD_FILE2_PROTOCOL   *This,
  IN      EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN      BOOLEAN                   BootPolicy,
  IN  OUT UINTN                     *BufferSize,
  OUT     VOID                      *Buffer     OPTIONAL
  )
{
  CONST KERNEL_BLOB  *InitrdBlob = &mKernelBlob[KernelBlobTypeInitrd];

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
  InitrdLoadFile2,
};

/**
  Fetch kernel, initrd and commandline into Blob.

  @param Blob    On input, choose which blob to be fetched.
                 On output, carry the full blob info.

  @retval EFI_NOT_FOUND    Kernel is not fetched or no chosen node found in DT.
  @retval EFI_SUCCESS      If there is kernel fetched or the Block->Name is not kernel.

**/
STATIC
EFI_STATUS
FetchBlob (
  IN OUT KERNEL_BLOB  *Blob
  )
{
  VOID          *DeviceTreeBase;
  UINT64        InitrdStart, InitrdEnd;
  INT32         Len;
  CONST UINT64  *Prop;
  INT32         ChosenNode;

  DeviceTreeBase = (VOID *)(UINTN)PcdGet64 (PcdDeviceTreeInitialBaseAddress);
  ASSERT (DeviceTreeBase != NULL);

  Blob->Size = 0;

  //
  // Make sure we have a valid device tree blob
  //
  ASSERT (fdt_check_header (DeviceTreeBase) == 0);

  //
  // Find chosen node
  //
  ChosenNode = fdt_path_offset (DeviceTreeBase, "/chosen");
  if (ChosenNode < 0) {
    return EFI_NOT_FOUND;
  }

  switch (Blob->Name[0]) {
    case 'k':
      Prop = fdt_getprop (DeviceTreeBase, ChosenNode, "linux,kernel-start", &Len);
      if ((Prop == NULL) || (Len < 0)) {
        return EFI_NOT_FOUND;
      }

      Blob->Data = (UINT8 *)fdt64_to_cpu (ReadUnaligned64 (Prop));
      Prop       = fdt_getprop (DeviceTreeBase, ChosenNode, "linux,kernel-size", &Len);
      if ((Prop == NULL) || (Len < 0)) {
        return EFI_SUCCESS;
      }

      Blob->Size = fdt64_to_cpu (ReadUnaligned64 (Prop));
      break;
    case 'i':
      Prop = fdt_getprop (DeviceTreeBase, ChosenNode, "linux,initrd-start", &Len);
      if ((Prop == NULL) || (Len < 0)) {
        return EFI_SUCCESS;
      }

      InitrdStart = fdt64_to_cpu (ReadUnaligned64 (Prop));
      Blob->Data  = (UINT8 *)InitrdStart;
      Prop        = fdt_getprop (DeviceTreeBase, ChosenNode, "linux,initrd-end", &Len);
      if ((Prop == NULL) || (Len < 0)) {
        return EFI_SUCCESS;
      }

      InitrdEnd  = fdt64_to_cpu (ReadUnaligned64 (Prop));
      Blob->Size = InitrdEnd - InitrdStart;
      break;
    case 'c':
      Prop = fdt_getprop (DeviceTreeBase, ChosenNode, "bootargs", &Len);
      if ((Prop == NULL) || (Len < 0)) {
        return EFI_SUCCESS;
      }

      Blob->Data = (UINT8 *)Prop;
      Blob->Size = Len;
      break;
    default:
      return EFI_SUCCESS;
  }

  return EFI_SUCCESS;
}

//
// The entry point of the feature.
//

/**
  Download the kernel, the initial ramdisk, and the kernel command line from
  memory or DT. Construct a minimal SimpleFileSystem that contains the two
  image files.

  @param ImageHandle  Image handle.
  @param SystemTable  EFI system table.

  @return                       Error codes from any of the underlying
                                functions. On success, the function doesn't
                                return.
**/
EFI_STATUS
EFIAPI
CloudHvKernelLoaderFsDxeEntrypoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINTN        BlobType;
  KERNEL_BLOB  *CurrentBlob;
  KERNEL_BLOB  *KernelBlob;
  EFI_STATUS   Status;
  EFI_HANDLE   FileSystemHandle;
  EFI_HANDLE   InitrdLoadFile2Handle;

  Status = gRT->GetTime (&mInitTime, NULL /* Capabilities */);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: GetTime(): %r\n", __FUNCTION__, Status));
    return Status;
  }

  //
  // Fetch all blobs.
  //
  for (BlobType = 0; BlobType < KernelBlobTypeMax; ++BlobType) {
    CurrentBlob = &mKernelBlob[BlobType];
    Status      = FetchBlob (CurrentBlob);
    if (EFI_ERROR (Status)) {
      goto FreeBlobs;
    }

    Status = VerifyBlob (
               CurrentBlob->Name,
               CurrentBlob->Data,
               CurrentBlob->Size
               );
    if (EFI_ERROR (Status)) {
      goto FreeBlobs;
    }

    mTotalBlobBytes += CurrentBlob->Size;
  }

  KernelBlob = &mKernelBlob[KernelBlobTypeKernel];

  if (KernelBlob->Data == NULL) {
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
      __FUNCTION__,
      Status
      ));
    goto FreeBlobs;
  }

  if (KernelBlob[KernelBlobTypeInitrd].Size > 0) {
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
        __FUNCTION__,
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
