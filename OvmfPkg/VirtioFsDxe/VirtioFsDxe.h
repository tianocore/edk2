/** @file
  Internal macro definitions, type definitions, and function declarations for
  the Virtio Filesystem device driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef VIRTIO_FS_DXE_H_
#define VIRTIO_FS_DXE_H_

#include <Base.h>                      // SIGNATURE_64()
#include <Guid/FileInfo.h>             // EFI_FILE_INFO
#include <IndustryStandard/VirtioFs.h> // VIRTIO_FS_TAG_BYTES
#include <Library/DebugLib.h>          // CR()
#include <Protocol/SimpleFileSystem.h> // EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
#include <Protocol/VirtioDevice.h>     // VIRTIO_DEVICE_PROTOCOL
#include <Uefi/UefiBaseType.h>         // EFI_EVENT

#define VIRTIO_FS_SIG  SIGNATURE_64 ('V', 'I', 'R', 'T', 'I', 'O', 'F', 'S')

#define VIRTIO_FS_FILE_SIG \
  SIGNATURE_64 ('V', 'I', 'O', 'F', 'S', 'F', 'I', 'L')

//
// The following limit applies to two kinds of pathnames.
//
// - The length of a POSIX-style, canonical pathname *at rest* never exceeds
//   VIRTIO_FS_MAX_PATHNAME_LENGTH. (Length is defined as the number of CHAR8
//   elements in the canonical pathname, excluding the terminating '\0'.) This
//   is an invariant that is ensured for canonical pathnames created, and that
//   is assumed about canonical pathname inputs (which all originate
//   internally).
//
// - If the length of a UEFI-style pathname *argument*, originating directly or
//   indirectly from the EFI_FILE_PROTOCOL caller, exceeds
//   VIRTIO_FS_MAX_PATHNAME_LENGTH, then the argument is rejected. (Length is
//   defined as the number of CHAR16 elements in the UEFI-style pathname,
//   excluding the terminating L'\0'.) This is a restriction that's checked on
//   external UEFI-style pathname inputs.
//
// The limit is not expected to be a practical limitation; it's only supposed
// to prevent attempts at overflowing size calculations. For both kinds of
// pathnames, separate limits could be used; a common limit is used purely for
// simplicity.
//
#define VIRTIO_FS_MAX_PATHNAME_LENGTH  ((UINTN)65535)

//
// Maximum value for VIRTIO_FS_FILE.NumFileInfo.
//
#define VIRTIO_FS_FILE_MAX_FILE_INFO  256

//
// Filesystem label encoded in UCS-2, transformed from the UTF-8 representation
// in "VIRTIO_FS_CONFIG.Tag", and NUL-terminated. Only the printable ASCII code
// points (U+0020 through U+007E) are supported.
//
typedef CHAR16 VIRTIO_FS_LABEL[VIRTIO_FS_TAG_BYTES + 1];

//
// Main context structure, expressing an EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
// interface on top of the Virtio Filesystem device.
//
typedef struct {
  //
  // Parts of this structure are initialized / torn down in various functions
  // at various call depths. The table to the right should make it easier to
  // track them.
  //
  //                              field         init function       init depth
  //                              -----------   ------------------  ----------
  UINT64                             Signature; // DriverBindingStart  0
  VIRTIO_DEVICE_PROTOCOL             *Virtio;   // DriverBindingStart  0
  VIRTIO_FS_LABEL                    Label;     // VirtioFsInit        1
  UINT16                             QueueSize; // VirtioFsInit        1
  VRING                              Ring;      // VirtioRingInit      2
  VOID                               *RingMap;  // VirtioRingMap       2
  UINT64                             RequestId; // FuseInitSession     1
  UINT32                             MaxWrite;  // FuseInitSession     1
  EFI_EVENT                          ExitBoot;  // DriverBindingStart  0
  LIST_ENTRY                         OpenFiles; // DriverBindingStart  0
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL    SimpleFs;  // DriverBindingStart  0
} VIRTIO_FS;

#define VIRTIO_FS_FROM_SIMPLE_FS(SimpleFsReference) \
  CR (SimpleFsReference, VIRTIO_FS, SimpleFs, VIRTIO_FS_SIG);

//
// Structure for describing a contiguous buffer, potentially mapped for Virtio
// transfer.
//
typedef struct {
  //
  // The following fields originate from the owner of the buffer.
  //
  VOID     *Buffer;
  UINTN    Size;
  //
  // All of the fields below, until the end of the structure, are
  // zero-initialized when the structure is initially validated.
  //
  // Mapped, MappedAddress and Mapping are updated when the buffer is mapped
  // for VirtioOperationBusMasterRead or VirtioOperationBusMasterWrite. They
  // are again updated when the buffer is unmapped.
  //
  BOOLEAN                 Mapped;
  EFI_PHYSICAL_ADDRESS    MappedAddress;
  VOID                    *Mapping;
  //
  // Transferred is updated after VirtioFlush() returns successfully:
  // - for VirtioOperationBusMasterRead, Transferred is set to Size;
  // - for VirtioOperationBusMasterWrite, Transferred is calculated from the
  //   UsedLen output parameter of VirtioFlush().
  //
  UINTN                   Transferred;
} VIRTIO_FS_IO_VECTOR;

//
// Structure for describing a list of IO Vectors.
//
typedef struct {
  //
  // The following fields originate from the owner of the buffers.
  //
  VIRTIO_FS_IO_VECTOR    *IoVec;
  UINTN                  NumVec;
  //
  // TotalSize is calculated when the scatter-gather list is initially
  // validated.
  //
  UINT32                 TotalSize;
} VIRTIO_FS_SCATTER_GATHER_LIST;

//
// Private context structure that exposes EFI_FILE_PROTOCOL on top of an open
// FUSE file reference.
//
typedef struct {
  UINT64               Signature;
  EFI_FILE_PROTOCOL    SimpleFile;
  BOOLEAN              IsDirectory;
  BOOLEAN              IsOpenForWriting;
  VIRTIO_FS            *OwnerFs;
  LIST_ENTRY           OpenFilesEntry;
  CHAR8                *CanonicalPathname;
  UINT64               FilePosition;
  //
  // In the FUSE wire protocol, every request except FUSE_INIT refers to a
  // file, namely by the "VIRTIO_FS_FUSE_REQUEST.NodeId" field; that is, by the
  // inode number of the file. However, some of the FUSE requests that we need
  // for some of the EFI_FILE_PROTOCOL member functions require an open file
  // handle *in addition* to the inode number. For simplicity, whenever a
  // VIRTIO_FS_FILE object is created, primarily defined by its NodeId field,
  // we also *open* the referenced file at once, and save the returned file
  // handle in the FuseHandle field. This way, when an EFI_FILE_PROTOCOL member
  // function must send a FUSE request that needs the file handle *in addition*
  // to the inode number, FuseHandle will be at our disposal at once.
  //
  UINT64    NodeId;
  UINT64    FuseHandle;
  //
  // EFI_FILE_INFO objects cached for an in-flight directory read.
  //
  // For reading through a directory stream with tolerable performance, we have
  // to call FUSE_READDIRPLUS each time with such a buffer that can deliver a
  // good number of variable size records (VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE
  // elements). Every time we do that, we turn the whole bunch into an array of
  // EFI_FILE_INFOs immediately. EFI_FILE_PROTOCOL.Read() invocations (on
  // directories) will be served from this EFI_FILE_INFO cache.
  //
  UINT8    *FileInfoArray;
  UINTN    SingleFileInfoSize;
  UINTN    NumFileInfo;
  UINTN    NextFileInfo;
} VIRTIO_FS_FILE;

#define VIRTIO_FS_FILE_FROM_SIMPLE_FILE(SimpleFileReference) \
  CR (SimpleFileReference, VIRTIO_FS_FILE, SimpleFile, VIRTIO_FS_FILE_SIG);

#define VIRTIO_FS_FILE_FROM_OPEN_FILES_ENTRY(OpenFilesEntryReference) \
  CR (OpenFilesEntryReference, VIRTIO_FS_FILE, OpenFilesEntry, \
    VIRTIO_FS_FILE_SIG);

//
// Initialization and helper routines for the Virtio Filesystem device.
//

EFI_STATUS
VirtioFsInit (
  IN OUT VIRTIO_FS  *VirtioFs
  );

VOID
VirtioFsUninit (
  IN OUT VIRTIO_FS  *VirtioFs
  );

VOID
EFIAPI
VirtioFsExitBoot (
  IN EFI_EVENT  ExitBootEvent,
  IN VOID       *VirtioFsAsVoid
  );

EFI_STATUS
VirtioFsSgListsValidate (
  IN     VIRTIO_FS                      *VirtioFs,
  IN OUT VIRTIO_FS_SCATTER_GATHER_LIST  *RequestSgList,
  IN OUT VIRTIO_FS_SCATTER_GATHER_LIST  *ResponseSgList OPTIONAL
  );

EFI_STATUS
VirtioFsSgListsSubmit (
  IN OUT VIRTIO_FS                      *VirtioFs,
  IN OUT VIRTIO_FS_SCATTER_GATHER_LIST  *RequestSgList,
  IN OUT VIRTIO_FS_SCATTER_GATHER_LIST  *ResponseSgList OPTIONAL
  );

EFI_STATUS
VirtioFsFuseNewRequest (
  IN OUT VIRTIO_FS              *VirtioFs,
  OUT VIRTIO_FS_FUSE_REQUEST    *Request,
  IN     UINT32                 RequestSize,
  IN     VIRTIO_FS_FUSE_OPCODE  Opcode,
  IN     UINT64                 NodeId
  );

EFI_STATUS
VirtioFsFuseCheckResponse (
  IN  VIRTIO_FS_SCATTER_GATHER_LIST  *ResponseSgList,
  IN  UINT64                         RequestId,
  OUT UINTN                          *TailBufferFill
  );

EFI_STATUS
VirtioFsErrnoToEfiStatus (
  IN INT32  Errno
  );

EFI_STATUS
VirtioFsAppendPath (
  IN     CHAR8   *LhsPath8,
  IN     CHAR16  *RhsPath16,
  OUT CHAR8      **ResultPath8,
  OUT BOOLEAN    *RootEscape
  );

EFI_STATUS
VirtioFsLookupMostSpecificParentDir (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN OUT CHAR8      *Path,
  OUT UINT64        *DirNodeId,
  OUT CHAR8         **LastComponent
  );

EFI_STATUS
VirtioFsGetBasename (
  IN     CHAR8  *Path,
  OUT CHAR16    *Basename     OPTIONAL,
  IN OUT UINTN  *BasenameSize
  );

EFI_STATUS
VirtioFsComposeRenameDestination (
  IN     CHAR8   *LhsPath8,
  IN     CHAR16  *RhsPath16,
  OUT CHAR8      **ResultPath8,
  OUT BOOLEAN    *RootEscape
  );

EFI_STATUS
VirtioFsFuseAttrToEfiFileInfo (
  IN     VIRTIO_FS_FUSE_ATTRIBUTES_RESPONSE  *FuseAttr,
  OUT EFI_FILE_INFO                          *FileInfo
  );

EFI_STATUS
VirtioFsFuseDirentPlusToEfiFileInfo (
  IN     VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE  *FuseDirent,
  IN OUT EFI_FILE_INFO                       *FileInfo
  );

VOID
VirtioFsGetFuseSizeUpdate (
  IN     EFI_FILE_INFO  *Info,
  IN     EFI_FILE_INFO  *NewInfo,
  OUT BOOLEAN           *Update,
  OUT UINT64            *Size
  );

EFI_STATUS
VirtioFsGetFuseTimeUpdates (
  IN     EFI_FILE_INFO  *Info,
  IN     EFI_FILE_INFO  *NewInfo,
  OUT BOOLEAN           *UpdateAtime,
  OUT BOOLEAN           *UpdateMtime,
  OUT UINT64            *Atime,
  OUT UINT64            *Mtime
  );

EFI_STATUS
VirtioFsGetFuseModeUpdate (
  IN     EFI_FILE_INFO  *Info,
  IN     EFI_FILE_INFO  *NewInfo,
  OUT BOOLEAN           *Update,
  OUT UINT32            *Mode
  );

//
// Wrapper functions for FUSE commands (primitives).
//

EFI_STATUS
VirtioFsFuseLookup (
  IN OUT VIRTIO_FS                        *VirtioFs,
  IN     UINT64                           DirNodeId,
  IN     CHAR8                            *Name,
  OUT UINT64                              *NodeId,
  OUT VIRTIO_FS_FUSE_ATTRIBUTES_RESPONSE  *FuseAttr
  );

EFI_STATUS
VirtioFsFuseForget (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     NodeId
  );

EFI_STATUS
VirtioFsFuseGetAttr (
  IN OUT VIRTIO_FS                        *VirtioFs,
  IN     UINT64                           NodeId,
  OUT VIRTIO_FS_FUSE_ATTRIBUTES_RESPONSE  *FuseAttr
  );

EFI_STATUS
VirtioFsFuseSetAttr (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     NodeId,
  IN     UINT64     *Size      OPTIONAL,
  IN     UINT64     *Atime     OPTIONAL,
  IN     UINT64     *Mtime     OPTIONAL,
  IN     UINT32     *Mode      OPTIONAL
  );

EFI_STATUS
VirtioFsFuseMkDir (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     ParentNodeId,
  IN     CHAR8      *Name,
  OUT UINT64        *NodeId
  );

EFI_STATUS
VirtioFsFuseRemoveFileOrDir (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     ParentNodeId,
  IN     CHAR8      *Name,
  IN     BOOLEAN    IsDir
  );

EFI_STATUS
VirtioFsFuseOpen (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     NodeId,
  IN     BOOLEAN    ReadWrite,
  OUT UINT64        *FuseHandle
  );

EFI_STATUS
VirtioFsFuseReadFileOrDir (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     NodeId,
  IN     UINT64     FuseHandle,
  IN     BOOLEAN    IsDir,
  IN     UINT64     Offset,
  IN OUT UINT32     *Size,
  OUT VOID          *Data
  );

EFI_STATUS
VirtioFsFuseWrite (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     NodeId,
  IN     UINT64     FuseHandle,
  IN     UINT64     Offset,
  IN OUT UINT32     *Size,
  IN     VOID       *Data
  );

EFI_STATUS
VirtioFsFuseStatFs (
  IN OUT VIRTIO_FS                    *VirtioFs,
  IN     UINT64                       NodeId,
  OUT VIRTIO_FS_FUSE_STATFS_RESPONSE  *FilesysAttr
  );

EFI_STATUS
VirtioFsFuseReleaseFileOrDir (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     NodeId,
  IN     UINT64     FuseHandle,
  IN     BOOLEAN    IsDir
  );

EFI_STATUS
VirtioFsFuseFsyncFileOrDir (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     NodeId,
  IN     UINT64     FuseHandle,
  IN     BOOLEAN    IsDir
  );

EFI_STATUS
VirtioFsFuseFlush (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     NodeId,
  IN     UINT64     FuseHandle
  );

EFI_STATUS
VirtioFsFuseInitSession (
  IN OUT VIRTIO_FS  *VirtioFs
  );

EFI_STATUS
VirtioFsFuseOpenDir (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     NodeId,
  OUT UINT64        *FuseHandle
  );

EFI_STATUS
VirtioFsFuseOpenOrCreate (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     ParentNodeId,
  IN     CHAR8      *Name,
  OUT UINT64        *NodeId,
  OUT UINT64        *FuseHandle
  );

EFI_STATUS
VirtioFsFuseRename (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     OldParentNodeId,
  IN     CHAR8      *OldName,
  IN     UINT64     NewParentNodeId,
  IN     CHAR8      *NewName
  );

//
// EFI_SIMPLE_FILE_SYSTEM_PROTOCOL member functions for the Virtio Filesystem
// driver.
//

EFI_STATUS
EFIAPI
VirtioFsOpenVolume (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL                **Root
  );

//
// EFI_FILE_PROTOCOL member functions for the Virtio Filesystem driver.
//

EFI_STATUS
EFIAPI
VirtioFsSimpleFileClose (
  IN EFI_FILE_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
VirtioFsSimpleFileDelete (
  IN EFI_FILE_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
VirtioFsSimpleFileFlush (
  IN EFI_FILE_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
VirtioFsSimpleFileGetInfo (
  IN     EFI_FILE_PROTOCOL  *This,
  IN     EFI_GUID           *InformationType,
  IN OUT UINTN              *BufferSize,
  OUT VOID                  *Buffer
  );

EFI_STATUS
EFIAPI
VirtioFsSimpleFileGetPosition (
  IN     EFI_FILE_PROTOCOL  *This,
  OUT UINT64                *Position
  );

EFI_STATUS
EFIAPI
VirtioFsSimpleFileOpen (
  IN     EFI_FILE_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL     **NewHandle,
  IN     CHAR16             *FileName,
  IN     UINT64             OpenMode,
  IN     UINT64             Attributes
  );

EFI_STATUS
EFIAPI
VirtioFsSimpleFileRead (
  IN     EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN              *BufferSize,
  OUT VOID                  *Buffer
  );

EFI_STATUS
EFIAPI
VirtioFsSimpleFileSetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN UINTN              BufferSize,
  IN VOID               *Buffer
  );

EFI_STATUS
EFIAPI
VirtioFsSimpleFileSetPosition (
  IN EFI_FILE_PROTOCOL  *This,
  IN UINT64             Position
  );

EFI_STATUS
EFIAPI
VirtioFsSimpleFileWrite (
  IN     EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN              *BufferSize,
  IN     VOID               *Buffer
  );

#endif // VIRTIO_FS_DXE_H_
