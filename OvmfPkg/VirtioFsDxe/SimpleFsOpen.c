/** @file
  EFI_FILE_PROTOCOL.Open() member function for the Virtio Filesystem driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>             // AsciiStrCmp()
#include <Library/MemoryAllocationLib.h> // AllocatePool()

#include "VirtioFsDxe.h"

/**
  Open the root directory, possibly for writing.

  @param[in,out] VirtioFs    The Virtio Filesystem device whose root directory
                             should be opened.

  @param[out] NewHandle      The new EFI_FILE_PROTOCOL instance through which
                             the root directory can be accessed.

  @param[in] OpenForWriting  TRUE if the root directory should be opened for
                             read-write access. FALSE if the root directory
                             should be opened for read-only access. Opening the
                             root directory for read-write access is useful for
                             calling EFI_FILE_PROTOCOL.Flush() or
                             EFI_FILE_PROTOCOL.SetInfo() later, for syncing or
                             touching the root directory, respectively.

  @retval EFI_SUCCESS        The root directory has been opened successfully.

  @retval EFI_ACCESS_DENIED  OpenForWriting is TRUE, but the root directory is
                             marked as read-only.

  @return                    Error codes propagated from underlying functions.
**/
STATIC
EFI_STATUS
OpenRootDirectory (
  IN OUT VIRTIO_FS       *VirtioFs,
  OUT EFI_FILE_PROTOCOL  **NewHandle,
  IN     BOOLEAN         OpenForWriting
  )
{
  EFI_STATUS      Status;
  VIRTIO_FS_FILE  *NewVirtioFsFile;

  //
  // VirtioFsOpenVolume() opens the root directory for read-only access. If the
  // current request is to open the root directory for read-write access, so
  // that EFI_FILE_PROTOCOL.Flush() or EFI_FILE_PROTOCOL.SetInfo()+timestamps
  // can be used on the root directory later, then we have to check for write
  // permission first.
  //
  if (OpenForWriting) {
    VIRTIO_FS_FUSE_ATTRIBUTES_RESPONSE  FuseAttr;
    EFI_FILE_INFO                       FileInfo;

    Status = VirtioFsFuseGetAttr (
               VirtioFs,
               VIRTIO_FS_FUSE_ROOT_DIR_NODE_ID,
               &FuseAttr
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = VirtioFsFuseAttrToEfiFileInfo (&FuseAttr, &FileInfo);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((FileInfo.Attribute & EFI_FILE_READ_ONLY) != 0) {
      return EFI_ACCESS_DENIED;
    }
  }

  Status = VirtioFsOpenVolume (&VirtioFs->SimpleFs, NewHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  NewVirtioFsFile                   = VIRTIO_FS_FILE_FROM_SIMPLE_FILE (*NewHandle);
  NewVirtioFsFile->IsOpenForWriting = OpenForWriting;
  return EFI_SUCCESS;
}

/**
  Open an existent regular file or non-root directory.

  @param[in,out] VirtioFs      The Virtio Filesystem device on which the
                               regular file or directory should be opened.

  @param[in] DirNodeId         The inode number of the immediate parent
                               directory of the regular file or directory to
                               open.

  @param[in] Name              The single-component filename of the regular
                               file or directory to open, under the immediate
                               parent directory identified by DirNodeId.

  @param[in] OpenForWriting    TRUE if the regular file or directory should be
                               opened for read-write access. FALSE if the
                               regular file or directory should be opened for
                               read-only access. Opening a directory for
                               read-write access is useful for deleting,
                               renaming, syncing or touching the directory
                               later.

  @param[out] NodeId           The inode number of the regular file or
                               directory, returned by the Virtio Filesystem
                               device.

  @param[out] FuseHandle       The open handle to the regular file or
                               directory, returned by the Virtio Filesystem
                               device.

  @param[out] NodeIsDirectory  Set to TRUE on output if Name was found to refer
                               to a directory. Set to FALSE if Name was found
                               to refer to a regular file.

  @retval EFI_SUCCESS        The regular file or directory has been looked up
                             and opened successfully.

  @retval EFI_ACCESS_DENIED  OpenForWriting is TRUE, but the regular file or
                             directory is marked read-only.

  @retval EFI_NOT_FOUND      A directory entry called Name was not found in the
                             directory identified by DirNodeId. (EFI_NOT_FOUND
                             is not returned for any other condition.)

  @return                    Errors propagated from underlying functions. If
                             the error code to propagate were EFI_NOT_FOUND, it
                             is remapped to EFI_DEVICE_ERROR.
**/
STATIC
EFI_STATUS
OpenExistentFileOrDirectory (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     DirNodeId,
  IN     CHAR8      *Name,
  IN     BOOLEAN    OpenForWriting,
  OUT UINT64        *NodeId,
  OUT UINT64        *FuseHandle,
  OUT BOOLEAN       *NodeIsDirectory
  )
{
  EFI_STATUS                          Status;
  UINT64                              ResolvedNodeId;
  VIRTIO_FS_FUSE_ATTRIBUTES_RESPONSE  FuseAttr;
  EFI_FILE_INFO                       FileInfo;
  BOOLEAN                             IsDirectory;
  UINT64                              NewFuseHandle;

  Status = VirtioFsFuseLookup (
             VirtioFs,
             DirNodeId,
             Name,
             &ResolvedNodeId,
             &FuseAttr
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = VirtioFsFuseAttrToEfiFileInfo (&FuseAttr, &FileInfo);
  if (EFI_ERROR (Status)) {
    goto ForgetResolvedNodeId;
  }

  if (OpenForWriting && ((FileInfo.Attribute & EFI_FILE_READ_ONLY) != 0)) {
    Status = EFI_ACCESS_DENIED;
    goto ForgetResolvedNodeId;
  }

  IsDirectory = (BOOLEAN)((FileInfo.Attribute & EFI_FILE_DIRECTORY) != 0);
  if (IsDirectory) {
    //
    // If OpenForWriting is TRUE here, that's not passed to
    // VirtioFsFuseOpenDir(); it does not affect the FUSE_OPENDIR request we
    // send. OpenForWriting=TRUE will only permit attempts to delete, rename,
    // flush (sync), and touch the directory.
    //
    Status = VirtioFsFuseOpenDir (VirtioFs, ResolvedNodeId, &NewFuseHandle);
  } else {
    Status = VirtioFsFuseOpen (
               VirtioFs,
               ResolvedNodeId,
               OpenForWriting,
               &NewFuseHandle
               );
  }

  if (EFI_ERROR (Status)) {
    goto ForgetResolvedNodeId;
  }

  *NodeId          = ResolvedNodeId;
  *FuseHandle      = NewFuseHandle;
  *NodeIsDirectory = IsDirectory;
  return EFI_SUCCESS;

ForgetResolvedNodeId:
  VirtioFsFuseForget (VirtioFs, ResolvedNodeId);
  return (Status == EFI_NOT_FOUND) ? EFI_DEVICE_ERROR : Status;
}

/**
  Create a directory.

  @param[in,out] VirtioFs  The Virtio Filesystem device on which the directory
                           should be created.

  @param[in] DirNodeId     The inode number of the immediate parent directory
                           of the directory to create.

  @param[in] Name          The single-component filename of the directory to
                           create, under the immediate parent directory
                           identified by DirNodeId.

  @param[out] NodeId       The inode number of the directory created, returned
                           by the Virtio Filesystem device.

  @param[out] FuseHandle   The open handle to the directory created, returned
                           by the Virtio Filesystem device.

  @retval EFI_SUCCESS  The directory has been created successfully.

  @return              Errors propagated from underlying functions.
**/
STATIC
EFI_STATUS
CreateDirectory (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     DirNodeId,
  IN     CHAR8      *Name,
  OUT UINT64        *NodeId,
  OUT UINT64        *FuseHandle
  )
{
  EFI_STATUS  Status;
  UINT64      NewChildDirNodeId;
  UINT64      NewFuseHandle;

  Status = VirtioFsFuseMkDir (VirtioFs, DirNodeId, Name, &NewChildDirNodeId);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = VirtioFsFuseOpenDir (VirtioFs, NewChildDirNodeId, &NewFuseHandle);
  if (EFI_ERROR (Status)) {
    goto RemoveNewChildDir;
  }

  *NodeId     = NewChildDirNodeId;
  *FuseHandle = NewFuseHandle;
  return EFI_SUCCESS;

RemoveNewChildDir:
  VirtioFsFuseRemoveFileOrDir (VirtioFs, DirNodeId, Name, TRUE /* IsDir */);
  VirtioFsFuseForget (VirtioFs, NewChildDirNodeId);
  return Status;
}

/**
  Create a regular file.

  @param[in,out] VirtioFs  The Virtio Filesystem device on which the regular
                           file should be created.

  @param[in] DirNodeId     The inode number of the immediate parent directory
                           of the regular file to create.

  @param[in] Name          The single-component filename of the regular file to
                           create, under the immediate parent directory
                           identified by DirNodeId.

  @param[out] NodeId       The inode number of the regular file created,
                           returned by the Virtio Filesystem device.

  @param[out] FuseHandle   The open handle to the regular file created,
                           returned by the Virtio Filesystem device.

  @retval EFI_SUCCESS  The regular file has been created successfully.

  @return              Errors propagated from underlying functions.
**/
STATIC
EFI_STATUS
CreateRegularFile (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     DirNodeId,
  IN     CHAR8      *Name,
  OUT UINT64        *NodeId,
  OUT UINT64        *FuseHandle
  )
{
  return VirtioFsFuseOpenOrCreate (
           VirtioFs,
           DirNodeId,
           Name,
           NodeId,
           FuseHandle
           );
}

EFI_STATUS
EFIAPI
VirtioFsSimpleFileOpen (
  IN     EFI_FILE_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL     **NewHandle,
  IN     CHAR16             *FileName,
  IN     UINT64             OpenMode,
  IN     UINT64             Attributes
  )
{
  VIRTIO_FS_FILE  *VirtioFsFile;
  VIRTIO_FS       *VirtioFs;
  BOOLEAN         OpenForWriting;
  BOOLEAN         PermitCreation;
  BOOLEAN         CreateDirectoryIfCreating;
  VIRTIO_FS_FILE  *NewVirtioFsFile;
  EFI_STATUS      Status;
  CHAR8           *NewCanonicalPath;
  BOOLEAN         RootEscape;
  UINT64          DirNodeId;
  CHAR8           *LastComponent;
  UINT64          NewNodeId;
  UINT64          NewFuseHandle;
  BOOLEAN         NewNodeIsDirectory;

  VirtioFsFile = VIRTIO_FS_FILE_FROM_SIMPLE_FILE (This);
  VirtioFs     = VirtioFsFile->OwnerFs;

  //
  // Validate OpenMode.
  //
  switch (OpenMode) {
    case EFI_FILE_MODE_READ:
      OpenForWriting = FALSE;
      PermitCreation = FALSE;
      break;
    case EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE:
      OpenForWriting = TRUE;
      PermitCreation = FALSE;
      break;
    case EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE:
      OpenForWriting = TRUE;
      PermitCreation = TRUE;
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  //
  // Set CreateDirectoryIfCreating to suppress incorrect compiler/analyzer
  // warnings.
  //
  CreateDirectoryIfCreating = FALSE;

  //
  // Validate the Attributes requested for the case when the file ends up being
  // created, provided creation is permitted.
  //
  if (PermitCreation) {
    if ((Attributes & ~EFI_FILE_VALID_ATTR) != 0) {
      //
      // Unknown attribute requested.
      //
      return EFI_INVALID_PARAMETER;
    }

    ASSERT (OpenForWriting);
    if ((Attributes & EFI_FILE_READ_ONLY) != 0) {
      DEBUG ((
        DEBUG_ERROR,
        ("%a: Label=\"%s\" CanonicalPathname=\"%a\" FileName=\"%s\" "
         "OpenMode=0x%Lx Attributes=0x%Lx: nonsensical request to possibly "
         "create a file marked read-only, for read-write access\n"),
        __func__,
        VirtioFs->Label,
        VirtioFsFile->CanonicalPathname,
        FileName,
        OpenMode,
        Attributes
        ));
      return EFI_INVALID_PARAMETER;
    }

    CreateDirectoryIfCreating = (BOOLEAN)((Attributes &
                                           EFI_FILE_DIRECTORY) != 0);
  }

  //
  // Referring to a file relative to a regular file makes no sense (or at least
  // it cannot be implemented consistently with how a file is referred to
  // relative to a directory). See USWG Mantis ticket #2367.
  //
  if (!VirtioFsFile->IsDirectory) {
    BOOLEAN  BugCompat;

    //
    // Tolerate this bug in the caller if FileName is absolute. If FileName is
    // absolute, then VirtioFsAppendPath() below will disregard
    // VirtioFsFile->CanonicalPathname.
    //
    BugCompat = (FileName[0] == L'\\');

    DEBUG ((
      BugCompat ? DEBUG_WARN : DEBUG_ERROR,
      ("%a: Label=\"%s\" CanonicalPathname=\"%a\" FileName=\"%s\": "
       "nonsensical request to open a file or directory relative to a regular "
       "file\n"),
      __func__,
      VirtioFs->Label,
      VirtioFsFile->CanonicalPathname,
      FileName
      ));
    if (!BugCompat) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Allocate the new VIRTIO_FS_FILE object.
  //
  NewVirtioFsFile = AllocatePool (sizeof *NewVirtioFsFile);
  if (NewVirtioFsFile == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Create the canonical pathname at which the desired file is expected to
  // exist.
  //
  Status = VirtioFsAppendPath (
             VirtioFsFile->CanonicalPathname,
             FileName,
             &NewCanonicalPath,
             &RootEscape
             );
  if (EFI_ERROR (Status)) {
    goto FreeNewVirtioFsFile;
  }

  if (RootEscape) {
    Status = EFI_ACCESS_DENIED;
    goto FreeNewCanonicalPath;
  }

  //
  // If the desired file is the root directory, just open the volume one more
  // time, without looking up anything.
  //
  if (AsciiStrCmp (NewCanonicalPath, "/") == 0) {
    FreePool (NewCanonicalPath);
    FreePool (NewVirtioFsFile);
    return OpenRootDirectory (VirtioFs, NewHandle, OpenForWriting);
  }

  //
  // Split the new canonical pathname into most specific parent directory
  // (given by DirNodeId) and last pathname component (i.e., immediate child
  // within that parent directory).
  //
  Status = VirtioFsLookupMostSpecificParentDir (
             VirtioFs,
             NewCanonicalPath,
             &DirNodeId,
             &LastComponent
             );
  if (EFI_ERROR (Status)) {
    goto FreeNewCanonicalPath;
  }

  //
  // Set NewNodeIsDirectory to suppress incorrect compiler/analyzer warnings.
  //
  NewNodeIsDirectory = FALSE;

  //
  // Try to open LastComponent directly under DirNodeId, as an existent regular
  // file or directory.
  //
  Status = OpenExistentFileOrDirectory (
             VirtioFs,
             DirNodeId,
             LastComponent,
             OpenForWriting,
             &NewNodeId,
             &NewFuseHandle,
             &NewNodeIsDirectory
             );
  //
  // If LastComponent could not be found under DirNodeId, but the request
  // allows us to create a new entry, attempt creating the requested regular
  // file or directory.
  //
  if ((Status == EFI_NOT_FOUND) && PermitCreation) {
    ASSERT (OpenForWriting);
    if (CreateDirectoryIfCreating) {
      Status = CreateDirectory (
                 VirtioFs,
                 DirNodeId,
                 LastComponent,
                 &NewNodeId,
                 &NewFuseHandle
                 );
    } else {
      Status = CreateRegularFile (
                 VirtioFs,
                 DirNodeId,
                 LastComponent,
                 &NewNodeId,
                 &NewFuseHandle
                 );
    }

    NewNodeIsDirectory = CreateDirectoryIfCreating;
  }

  //
  // Regardless of the branch taken, we're done with DirNodeId.
  //
  if (DirNodeId != VIRTIO_FS_FUSE_ROOT_DIR_NODE_ID) {
    VirtioFsFuseForget (VirtioFs, DirNodeId);
  }

  if (EFI_ERROR (Status)) {
    goto FreeNewCanonicalPath;
  }

  //
  // Populate the new VIRTIO_FS_FILE object.
  //
  NewVirtioFsFile->Signature              = VIRTIO_FS_FILE_SIG;
  NewVirtioFsFile->SimpleFile.Revision    = EFI_FILE_PROTOCOL_REVISION;
  NewVirtioFsFile->SimpleFile.Open        = VirtioFsSimpleFileOpen;
  NewVirtioFsFile->SimpleFile.Close       = VirtioFsSimpleFileClose;
  NewVirtioFsFile->SimpleFile.Delete      = VirtioFsSimpleFileDelete;
  NewVirtioFsFile->SimpleFile.Read        = VirtioFsSimpleFileRead;
  NewVirtioFsFile->SimpleFile.Write       = VirtioFsSimpleFileWrite;
  NewVirtioFsFile->SimpleFile.GetPosition = VirtioFsSimpleFileGetPosition;
  NewVirtioFsFile->SimpleFile.SetPosition = VirtioFsSimpleFileSetPosition;
  NewVirtioFsFile->SimpleFile.GetInfo     = VirtioFsSimpleFileGetInfo;
  NewVirtioFsFile->SimpleFile.SetInfo     = VirtioFsSimpleFileSetInfo;
  NewVirtioFsFile->SimpleFile.Flush       = VirtioFsSimpleFileFlush;
  NewVirtioFsFile->IsDirectory            = NewNodeIsDirectory;
  NewVirtioFsFile->IsOpenForWriting       = OpenForWriting;
  NewVirtioFsFile->OwnerFs                = VirtioFs;
  NewVirtioFsFile->CanonicalPathname      = NewCanonicalPath;
  NewVirtioFsFile->FilePosition           = 0;
  NewVirtioFsFile->NodeId                 = NewNodeId;
  NewVirtioFsFile->FuseHandle             = NewFuseHandle;
  NewVirtioFsFile->FileInfoArray          = NULL;
  NewVirtioFsFile->SingleFileInfoSize     = 0;
  NewVirtioFsFile->NumFileInfo            = 0;
  NewVirtioFsFile->NextFileInfo           = 0;

  //
  // One more file is now open for the filesystem.
  //
  InsertTailList (&VirtioFs->OpenFiles, &NewVirtioFsFile->OpenFilesEntry);

  *NewHandle = &NewVirtioFsFile->SimpleFile;
  return EFI_SUCCESS;

FreeNewCanonicalPath:
  FreePool (NewCanonicalPath);

FreeNewVirtioFsFile:
  FreePool (NewVirtioFsFile);

  return Status;
}
