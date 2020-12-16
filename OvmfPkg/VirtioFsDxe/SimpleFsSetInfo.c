/** @file
  EFI_FILE_PROTOCOL.SetInfo() member function for the Virtio Filesystem driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Guid/FileSystemInfo.h>            // gEfiFileSystemInfoGuid
#include <Guid/FileSystemVolumeLabelInfo.h> // gEfiFileSystemVolumeLabelInfo...
#include <Library/BaseLib.h>                // StrCmp()
#include <Library/BaseMemoryLib.h>          // CompareGuid()
#include <Library/MemoryAllocationLib.h>    // FreePool()

#include "VirtioFsDxe.h"

/**
  Validate a buffer that the EFI_FILE_PROTOCOL.SetInfo() caller passes in for a
  particular InformationType GUID.

  The structure to be validated is supposed to end with a variable-length,
  NUL-terminated CHAR16 Name string.

  @param[in] SizeByProtocolCaller  The BufferSize parameter as provided by the
                                   EFI_FILE_PROTOCOL.SetInfo() caller.

  @param[in] MinimumStructSize     The minimum structure size that is required
                                   for the given InformationType GUID,
                                   including a single CHAR16 element from the
                                   trailing Name field.

  @param[in] IsSizeByInfoPresent   TRUE if and only if the expected structure
                                   starts with a UINT64 Size field that reports
                                   the actual structure size.

  @param[in] Buffer                The Buffer parameter as provided by the
                                   EFI_FILE_PROTOCOL.SetInfo() caller.

  @retval EFI_SUCCESS            Validation successful, Buffer is well-formed.

  @retval EFI_BAD_BUFFER_SIZE    The EFI_FILE_PROTOCOL.SetInfo()
                                 caller provided a BufferSize that is smaller
                                 than the minimum structure size required for
                                 the given InformationType GUID.

  @retval EFI_INVALID_PARAMETER  IsSizeByInfoPresent is TRUE, and the leading
                                 UINT64 Size field does not match the
                                 EFI_FILE_PROTOCOL.SetInfo() caller-provided
                                 BufferSize.

  @retval EFI_INVALID_PARAMETER  The trailing Name field does not consist of a
                                 whole multiple of CHAR16 elements.

  @retval EFI_INVALID_PARAMETER  The trailing Name field is not NUL-terminated.
**/
STATIC
EFI_STATUS
ValidateInfoStructure (
  IN UINTN   SizeByProtocolCaller,
  IN UINTN   MinimumStructSize,
  IN BOOLEAN IsSizeByInfoPresent,
  IN VOID    *Buffer
  )
{
  UINTN  NameFieldByteOffset;
  UINTN  NameFieldBytes;
  UINTN  NameFieldChar16s;
  CHAR16 *NameField;

  //
  // Make sure the internal function asking for validation passes in sane
  // values.
  //
  ASSERT (MinimumStructSize >= sizeof (CHAR16));
  NameFieldByteOffset = MinimumStructSize - sizeof (CHAR16);

  if (IsSizeByInfoPresent) {
    ASSERT (MinimumStructSize >= sizeof (UINT64) + sizeof (CHAR16));
    ASSERT (NameFieldByteOffset >= sizeof (UINT64));
  }

  //
  // Check whether the protocol caller provided enough bytes for the minimum
  // size of this info structure.
  //
  if (SizeByProtocolCaller < MinimumStructSize) {
    return EFI_BAD_BUFFER_SIZE;
  }

  //
  // If the info structure starts with a UINT64 Size field, check if that
  // agrees with the protocol caller-provided size.
  //
  if (IsSizeByInfoPresent) {
    UINT64 *SizeByInfo;

    SizeByInfo = Buffer;
    if (*SizeByInfo != SizeByProtocolCaller) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // The CHAR16 Name field at the end of the structure must have an even number
  // of bytes.
  //
  // The subtraction below cannot underflow, and yields at least
  // sizeof(CHAR16).
  //
  ASSERT (SizeByProtocolCaller >= NameFieldByteOffset);
  NameFieldBytes = SizeByProtocolCaller - NameFieldByteOffset;
  ASSERT (NameFieldBytes >= sizeof (CHAR16));
  if (NameFieldBytes % sizeof (CHAR16) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The CHAR16 Name field at the end of the structure must be NUL-terminated.
  //
  NameFieldChar16s = NameFieldBytes / sizeof (CHAR16);
  ASSERT (NameFieldChar16s >= 1);

  NameField = (CHAR16 *)((UINT8 *)Buffer + NameFieldByteOffset);
  if (NameField[NameFieldChar16s - 1] != L'\0') {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Rename a VIRTIO_FS_FILE as requested in EFI_FILE_INFO.FileName.

  @param[in,out] VirtioFsFile  The VIRTIO_FS_FILE to rename.

  @param[in] NewFileName       The new file name requested by
                               EFI_FILE_PROTOCOL.SetInfo().

  @retval EFI_SUCCESS        The canonical format destination path that is
                             determined from the input value of
                             VirtioFsFile->CanonicalPathname and from
                             NewFileName is identical to the input value of
                             VirtioFsFile->CanonicalPathname. This means that
                             EFI_FILE_INFO does not constitute a rename
                             request. VirtioFsFile has not been changed.

  @retval EFI_SUCCESS        VirtioFsFile has been renamed.
                             VirtioFsFile->CanonicalPathname has assumed the
                             destination pathname in canonical format.

  @retval EFI_ACCESS_DENIED  VirtioFsFile refers to the root directory, and
                             NewFileName expresses an actual rename/move
                             request.

  @retval EFI_ACCESS_DENIED  VirtioFsFile is the (possibly indirect) parent
                             directory of at least one other VIRTIO_FS_FILE
                             that is open for the same Virtio Filesystem
                             (identified by VirtioFsFile->OwnerFs). Renaming
                             VirtioFsFile would invalidate the canonical
                             pathnames of those VIRTIO_FS_FILE instances;
                             therefore the request has been rejected.

  @retval EFI_ACCESS_DENIED  VirtioFsFile is not open for writing, but
                             NewFileName expresses an actual rename/move
                             request.

  @retval EFI_NOT_FOUND      At least one dot-dot component in NewFileName
                             attempted to escape the root directory.

  @return                    Error codes propagated from underlying functions.
**/
STATIC
EFI_STATUS
Rename (
  IN OUT VIRTIO_FS_FILE *VirtioFsFile,
  IN     CHAR16         *NewFileName
  )
{

  VIRTIO_FS  *VirtioFs;
  EFI_STATUS Status;
  CHAR8      *Destination;
  BOOLEAN    RootEscape;
  UINT64     OldParentDirNodeId;
  CHAR8      *OldLastComponent;
  UINT64     NewParentDirNodeId;
  CHAR8      *NewLastComponent;

  VirtioFs = VirtioFsFile->OwnerFs;

  //
  // The root directory cannot be renamed.
  //
  if (AsciiStrCmp (VirtioFsFile->CanonicalPathname, "/") == 0) {
    if (StrCmp (NewFileName, L"") == 0) {
      //
      // Not a rename request anyway.
      //
      return EFI_SUCCESS;
    }
    return EFI_ACCESS_DENIED;
  }

  //
  // Compose the canonical pathname for the destination.
  //
  Status = VirtioFsComposeRenameDestination (VirtioFsFile->CanonicalPathname,
             NewFileName, &Destination, &RootEscape);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  if (RootEscape) {
    Status = EFI_NOT_FOUND;
    goto FreeDestination;
  }
  //
  // If the rename would leave VirtioFsFile->CanonicalPathname unchanged, then
  // EFI_FILE_PROTOCOL.SetInfo() isn't asking for a rename actually.
  //
  if (AsciiStrCmp (VirtioFsFile->CanonicalPathname, Destination) == 0) {
    Status = EFI_SUCCESS;
    goto FreeDestination;
  }
  //
  // Check if the rename would break the canonical pathnames of other
  // VIRTIO_FS_FILE instances of the same VIRTIO_FS.
  //
  if (VirtioFsFile->IsDirectory) {
    UINTN      PathLen;
    LIST_ENTRY *OpenFilesEntry;

    PathLen = AsciiStrLen (VirtioFsFile->CanonicalPathname);
    BASE_LIST_FOR_EACH (OpenFilesEntry, &VirtioFs->OpenFiles) {
      VIRTIO_FS_FILE *OtherFile;

      OtherFile = VIRTIO_FS_FILE_FROM_OPEN_FILES_ENTRY (OpenFilesEntry);
      if (OtherFile != VirtioFsFile &&
          AsciiStrnCmp (VirtioFsFile->CanonicalPathname,
            OtherFile->CanonicalPathname, PathLen) == 0 &&
          (OtherFile->CanonicalPathname[PathLen] == '\0' ||
           OtherFile->CanonicalPathname[PathLen] == '/')) {
        //
        // OtherFile refers to the same directory as VirtioFsFile, or is a
        // (possibly indirect) child of the directory referred to by
        // VirtioFsFile.
        //
        Status = EFI_ACCESS_DENIED;
        goto FreeDestination;
      }
    }
  }
  //
  // From this point on, the file needs to be open for writing.
  //
  if (!VirtioFsFile->IsOpenForWriting) {
    Status = EFI_ACCESS_DENIED;
    goto FreeDestination;
  }
  //
  // Split both source and destination canonical pathnames into (most specific
  // parent directory, last component) pairs.
  //
  Status = VirtioFsLookupMostSpecificParentDir (VirtioFs,
             VirtioFsFile->CanonicalPathname, &OldParentDirNodeId,
             &OldLastComponent);
  if (EFI_ERROR (Status)) {
    goto FreeDestination;
  }
  Status = VirtioFsLookupMostSpecificParentDir (VirtioFs, Destination,
             &NewParentDirNodeId, &NewLastComponent);
  if (EFI_ERROR (Status)) {
    goto ForgetOldParentDirNodeId;
  }
  //
  // Perform the rename. If the destination path exists, the rename will fail.
  //
  Status = VirtioFsFuseRename (VirtioFs, OldParentDirNodeId, OldLastComponent,
             NewParentDirNodeId, NewLastComponent);
  if (EFI_ERROR (Status)) {
    goto ForgetNewParentDirNodeId;
  }

  //
  // Swap in the new canonical pathname.
  //
  FreePool (VirtioFsFile->CanonicalPathname);
  VirtioFsFile->CanonicalPathname = Destination;
  Destination = NULL;
  Status = EFI_SUCCESS;

  //
  // Fall through.
  //
ForgetNewParentDirNodeId:
  if (NewParentDirNodeId != VIRTIO_FS_FUSE_ROOT_DIR_NODE_ID) {
    VirtioFsFuseForget (VirtioFs, NewParentDirNodeId);
  }

ForgetOldParentDirNodeId:
  if (OldParentDirNodeId != VIRTIO_FS_FUSE_ROOT_DIR_NODE_ID) {
    VirtioFsFuseForget (VirtioFs, OldParentDirNodeId);
  }

FreeDestination:
  if (Destination != NULL) {
    FreePool (Destination);
  }
  return Status;
}

/**
  Update the attributes of a VIRTIO_FS_FILE as requested in EFI_FILE_INFO.

  @param[in,out] VirtioFsFile  The VIRTIO_FS_FILE to update the attributes of.

  @param[in] NewFileInfo       The new attributes requested by
                               EFI_FILE_PROTOCOL.SetInfo(). NewFileInfo->Size
                               and NewFileInfo->FileName are ignored.

  @retval EFI_SUCCESS        No attributes had to be updated.

  @retval EFI_SUCCESS        The required set of attribute updates has been
                             determined and performed successfully.

  @retval EFI_ACCESS_DENIED  NewFileInfo requests an update to a property
                             different from the EFI_FILE_READ_ONLY bit in the
                             Attribute field, but VirtioFsFile is not open for
                             writing.

  @return                    Error codes propagated from underlying functions.
**/
STATIC
EFI_STATUS
UpdateAttributes (
  IN OUT VIRTIO_FS_FILE *VirtioFsFile,
  IN     EFI_FILE_INFO  *NewFileInfo
  )
{
  VIRTIO_FS                          *VirtioFs;
  EFI_STATUS                         Status;
  VIRTIO_FS_FUSE_ATTRIBUTES_RESPONSE FuseAttr;
  EFI_FILE_INFO                      FileInfo;
  BOOLEAN                            UpdateFileSize;
  UINT64                             FileSize;
  BOOLEAN                            UpdateAtime;
  BOOLEAN                            UpdateMtime;
  UINT64                             Atime;
  UINT64                             Mtime;
  BOOLEAN                            UpdateMode;
  UINT32                             Mode;

  VirtioFs = VirtioFsFile->OwnerFs;

  //
  // Fetch the current attributes first, so we can build the difference between
  // them and NewFileInfo.
  //
  Status = VirtioFsFuseGetAttr (VirtioFs, VirtioFsFile->NodeId, &FuseAttr);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = VirtioFsFuseAttrToEfiFileInfo (&FuseAttr, &FileInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Collect the updates.
  //
  if (VirtioFsFile->IsDirectory) {
    UpdateFileSize = FALSE;
  } else {
    VirtioFsGetFuseSizeUpdate (&FileInfo, NewFileInfo, &UpdateFileSize,
      &FileSize);
  }

  Status = VirtioFsGetFuseTimeUpdates (&FileInfo, NewFileInfo, &UpdateAtime,
             &UpdateMtime, &Atime, &Mtime);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = VirtioFsGetFuseModeUpdate (&FileInfo, NewFileInfo, &UpdateMode,
             &Mode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // If no attribute updates are necessary, we're done.
  //
  if (!UpdateFileSize && !UpdateAtime && !UpdateMtime && !UpdateMode) {
    return EFI_SUCCESS;
  }
  //
  // If the file is not open for writing, then only Mode may be updated (for
  // toggling EFI_FILE_READ_ONLY).
  //
  if (!VirtioFsFile->IsOpenForWriting &&
      (UpdateFileSize || UpdateAtime || UpdateMtime)) {
    return EFI_ACCESS_DENIED;
  }
  //
  // Send the FUSE_SETATTR request now.
  //
  Status = VirtioFsFuseSetAttr (
             VirtioFs,
             VirtioFsFile->NodeId,
             UpdateFileSize ? &FileSize : NULL,
             UpdateAtime    ? &Atime    : NULL,
             UpdateMtime    ? &Mtime    : NULL,
             UpdateMode     ? &Mode     : NULL
             );
  return Status;
}

/**
  Process an EFI_FILE_INFO setting request.
**/
STATIC
EFI_STATUS
SetFileInfo (
  IN EFI_FILE_PROTOCOL *This,
  IN UINTN             BufferSize,
  IN VOID              *Buffer
  )
{
  VIRTIO_FS_FILE *VirtioFsFile;
  EFI_STATUS     Status;
  EFI_FILE_INFO  *FileInfo;

  VirtioFsFile = VIRTIO_FS_FILE_FROM_SIMPLE_FILE (This);

  //
  // Validate if Buffer passes as EFI_FILE_INFO.
  //
  Status = ValidateInfoStructure (
             BufferSize,                    // SizeByProtocolCaller
             OFFSET_OF (EFI_FILE_INFO,
               FileName) + sizeof (CHAR16), // MinimumStructSize
             TRUE,                          // IsSizeByInfoPresent
             Buffer
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  FileInfo = Buffer;

  //
  // Perform the rename/move request, if any.
  //
  Status = Rename (VirtioFsFile, FileInfo->FileName);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Update any attributes requested.
  //
  Status = UpdateAttributes (VirtioFsFile, FileInfo);
  //
  // The UEFI spec does not speak about partial failure in
  // EFI_FILE_PROTOCOL.SetInfo(); we won't try to roll back the rename (if
  // there was one) in case the attribute updates fail.
  //
  return Status;
}

/**
  Process an EFI_FILE_SYSTEM_INFO setting request.
**/
STATIC
EFI_STATUS
SetFileSystemInfo (
  IN EFI_FILE_PROTOCOL *This,
  IN UINTN             BufferSize,
  IN VOID              *Buffer
  )
{
  VIRTIO_FS_FILE       *VirtioFsFile;
  VIRTIO_FS            *VirtioFs;
  EFI_STATUS           Status;
  EFI_FILE_SYSTEM_INFO *FileSystemInfo;

  VirtioFsFile = VIRTIO_FS_FILE_FROM_SIMPLE_FILE (This);
  VirtioFs     = VirtioFsFile->OwnerFs;

  //
  // Validate if Buffer passes as EFI_FILE_SYSTEM_INFO.
  //
  Status = ValidateInfoStructure (
             BufferSize,                       // SizeByProtocolCaller
             OFFSET_OF (EFI_FILE_SYSTEM_INFO,
               VolumeLabel) + sizeof (CHAR16), // MinimumStructSize
             TRUE,                             // IsSizeByInfoPresent
             Buffer
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  FileSystemInfo = Buffer;

  //
  // EFI_FILE_SYSTEM_INFO fields other than VolumeLabel cannot be changed, per
  // spec.
  //
  // If the label is being changed to its current value, report success;
  // otherwise, reject the request, as the Virtio Filesystem device does not
  // support changing the label.
  //
  if (StrCmp (FileSystemInfo->VolumeLabel, VirtioFs->Label) == 0) {
    return EFI_SUCCESS;
  }
  return EFI_WRITE_PROTECTED;
}

/**
  Process an EFI_FILE_SYSTEM_VOLUME_LABEL setting request.
**/
STATIC
EFI_STATUS
SetFileSystemVolumeLabelInfo (
  IN EFI_FILE_PROTOCOL *This,
  IN UINTN             BufferSize,
  IN VOID              *Buffer
  )
{
  VIRTIO_FS_FILE               *VirtioFsFile;
  VIRTIO_FS                    *VirtioFs;
  EFI_STATUS                   Status;
  EFI_FILE_SYSTEM_VOLUME_LABEL *FileSystemVolumeLabel;

  VirtioFsFile = VIRTIO_FS_FILE_FROM_SIMPLE_FILE (This);
  VirtioFs     = VirtioFsFile->OwnerFs;

  //
  // Validate if Buffer passes as EFI_FILE_SYSTEM_VOLUME_LABEL.
  //
  Status = ValidateInfoStructure (
             BufferSize,                              // SizeByProtocolCaller
             OFFSET_OF (EFI_FILE_SYSTEM_VOLUME_LABEL,
               VolumeLabel) + sizeof (CHAR16),        // MinimumStructSize
             FALSE,                                   // IsSizeByInfoPresent
             Buffer
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  FileSystemVolumeLabel = Buffer;

  //
  // If the label is being changed to its current value, report success;
  // otherwise, reject the request, as the Virtio Filesystem device does not
  // support changing the label.
  //
  if (StrCmp (FileSystemVolumeLabel->VolumeLabel, VirtioFs->Label) == 0) {
    return EFI_SUCCESS;
  }
  return EFI_WRITE_PROTECTED;
}

EFI_STATUS
EFIAPI
VirtioFsSimpleFileSetInfo (
  IN EFI_FILE_PROTOCOL *This,
  IN EFI_GUID          *InformationType,
  IN UINTN             BufferSize,
  IN VOID              *Buffer
  )
{
  if (CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    return SetFileInfo (This, BufferSize, Buffer);
  }

  if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid)) {
    return SetFileSystemInfo (This, BufferSize, Buffer);
  }

  if (CompareGuid (InformationType, &gEfiFileSystemVolumeLabelInfoIdGuid)) {
    return SetFileSystemVolumeLabelInfo (This, BufferSize, Buffer);
  }

  return EFI_UNSUPPORTED;
}
