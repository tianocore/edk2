/** @file
  The internal header file of FvSimpleFileSystem driver.

Copyright (c) 2014, ARM Limited. All rights reserved.
Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FVFS_INTERNAL_H__
#define __FVFS_INTERNAL_H__

#include <Uefi.h>
#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/DriverBinding.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/UnicodeCollation.h>

#include <Guid/FileSystemInfo.h>
#include <Guid/FileInfo.h>
#include <Guid/FileSystemVolumeLabelInfo.h>

typedef struct _FV_FILESYSTEM_FILE       FV_FILESYSTEM_FILE;
typedef struct _FV_FILESYSTEM_FILE_INFO  FV_FILESYSTEM_FILE_INFO;
typedef struct _FV_FILESYSTEM_INSTANCE   FV_FILESYSTEM_INSTANCE;

//
// Struct representing an instance of the "filesystem". There will be one of
// these structs per FV.
//
struct _FV_FILESYSTEM_INSTANCE {
  UINT32                           Signature;
  LIST_ENTRY                       FileInfoHead;
  LIST_ENTRY                       FileHead;
  EFI_DRIVER_BINDING_PROTOCOL      *DriverBinding;
  EFI_FIRMWARE_VOLUME2_PROTOCOL    *FvProtocol;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  SimpleFs;
  FV_FILESYSTEM_FILE               *Root;
  CHAR16                           *VolumeLabel;
};

//
// Struct representing a opening file. Each opening operation on file will
// create such an instance except for the "root directory", which will only
// be created once for each FV.
//
struct _FV_FILESYSTEM_FILE {
  UINT32                           Signature;
  LIST_ENTRY                       Link;
  FV_FILESYSTEM_FILE_INFO          *DirReadNext;
  FV_FILESYSTEM_INSTANCE           *Instance;
  EFI_FILE_PROTOCOL                FileProtocol;
  FV_FILESYSTEM_FILE_INFO          *FvFileInfo;
  UINT64                           Position;
};

//
// Struct representing the info of a file.
//
struct _FV_FILESYSTEM_FILE_INFO {
  UINT32                           Signature;
  LIST_ENTRY                       Link;
  EFI_GUID                         NameGuid;
  EFI_FV_FILETYPE                  Type;
  EFI_FILE_INFO                    FileInfo;
};

#define FVFS_FILE_SIGNATURE        SIGNATURE_32 ('f', 'v', 'f', 'i')
#define FVFS_FILE_INFO_SIGNATURE   SIGNATURE_32 ('f', 'v', 'i', 'n')
#define FVFS_INSTANCE_SIGNATURE    SIGNATURE_32 ('f', 'v', 'f', 's')

#define FVFS_INSTANCE_FROM_SIMPLE_FS_THIS(This) CR (  \
          This,                                       \
          FV_FILESYSTEM_INSTANCE,                     \
          SimpleFs,                                   \
          FVFS_INSTANCE_SIGNATURE                     \
          )

#define FVFS_FILE_FROM_FILE_THIS(This) CR (           \
          This,                                       \
          FV_FILESYSTEM_FILE,                         \
          FileProtocol,                               \
          FVFS_FILE_SIGNATURE                         \
          )

#define FVFS_FILE_INFO_FROM_LINK(This) CR (           \
          This,                                       \
          FV_FILESYSTEM_FILE_INFO,                    \
          Link,                                       \
          FVFS_FILE_INFO_SIGNATURE                    \
          )

#define FVFS_FILE_FROM_LINK(FileLink) CR (FileLink, FV_FILESYSTEM_FILE, Link, FVFS_FILE_SIGNATURE)

#define FVFS_GET_FIRST_FILE(Instance) FVFS_FILE_FROM_LINK (GetFirstNode (&Instance->FileHead))

#define FVFS_GET_FIRST_FILE_INFO(Instance) FVFS_FILE_INFO_FROM_LINK (GetFirstNode (&Instance->FileInfoHead))


#define FV_FILETYPE_IS_EXECUTABLE(Type) ((Type) == EFI_FV_FILETYPE_PEIM                  || \
                                         (Type) == EFI_FV_FILETYPE_DRIVER                || \
                                         (Type) == EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER  || \
                                         (Type) == EFI_FV_FILETYPE_APPLICATION)

/**
  Open the root directory on a volume.

  @param  This     A pointer to the volume to open the root directory.
  @param  RootFile A pointer to the location to return the opened file handle for the
                   root directory.

  @retval EFI_SUCCESS          The device was opened.
  @retval EFI_UNSUPPORTED      This volume does not support the requested file system type.
  @retval EFI_NO_MEDIA         The device has no medium.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_ACCESS_DENIED    The service denied access to the file.
  @retval EFI_OUT_OF_RESOURCES The volume was not opened due to lack of resources.
  @retval EFI_MEDIA_CHANGED    The device has a different medium in it or the medium is no
                               longer supported. Any existing file handles for this volume are
                               no longer valid. To access the files on the new medium, the
                               volume must be reopened with OpenVolume().

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemOpenVolume (
  IN     EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
     OUT EFI_FILE_PROTOCOL               **RootFile
  );

/**
  Test to see if this driver supports ControllerHandle.

  @param  DriverBinding       Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device
  @retval EFI_ALREADY_STARTED This driver is already running on this device
  @retval other               This driver does not support this device

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemDriverSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *DriverBinding,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Start this driver on ControllerHandle by opening a FV protocol and
  installing a SimpleFileSystem protocol on ControllerHandle.

  @param  DriverBinding        Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemDriverStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *DriverBinding,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Stop this driver on ControllerHandle by removing SimpleFileSystem protocol and closing
  the FV protocol on ControllerHandle.

  @param  DriverBinding     Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL       *DriverBinding,
  IN  EFI_HANDLE                        ControllerHandle,
  IN  UINTN                             NumberOfChildren,
  IN  EFI_HANDLE                        *ChildHandleBuffer OPTIONAL
  );

/**
  Opens a new file relative to the source file's location.

  @param  This       A pointer to the EFI_FILE_PROTOCOL instance that is the file
                     handle to the source location. This would typically be an open
                     handle to a directory.
  @param  NewHandle  A pointer to the location to return the opened handle for the new
                     file.
  @param  FileName   The Null-terminated string of the name of the file to be opened.
                     The file name may contain the following path modifiers: "\", ".",
                     and "..".
  @param  OpenMode   The mode to open the file. The only valid combinations that the
                     file may be opened with are: Read, Read/Write, or Create/Read/Write.
  @param  Attributes Only valid for EFI_FILE_MODE_CREATE, in which case these are the
                     attribute bits for the newly created file.

  @retval EFI_SUCCESS          The file was opened.
  @retval EFI_NOT_FOUND        The specified file could not be found on the device.
  @retval EFI_NO_MEDIA         The device has no medium.
  @retval EFI_MEDIA_CHANGED    The device has a different medium in it or the medium is no
                               longer supported.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  An attempt was made to create a file, or open a file for write
                               when the media is write-protected.
  @retval EFI_ACCESS_DENIED    The service denied access to the file.
  @retval EFI_OUT_OF_RESOURCES Not enough resources were available to open the file.
  @retval EFI_VOLUME_FULL      The volume is full.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemOpen (
  IN     EFI_FILE_PROTOCOL    *This,
     OUT EFI_FILE_PROTOCOL    **NewHandle,
  IN     CHAR16               *FileName,
  IN     UINT64               OpenMode,
  IN     UINT64               Attributes
  );

/**
  Closes a specified file handle.

  @param  This          A pointer to the EFI_FILE_PROTOCOL instance that is the file
                        handle to close.

  @retval EFI_SUCCESS   The file was closed.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemClose (
  IN EFI_FILE_PROTOCOL  *This
  );

/**
  Reads data from a file.

  @param  This       A pointer to the EFI_FILE_PROTOCOL instance that is the file
                     handle to read data from.
  @param  BufferSize On input, the size of the Buffer. On output, the amount of data
                     returned in Buffer. In both cases, the size is measured in bytes.
  @param  Buffer     The buffer into which the data is read.

  @retval EFI_SUCCESS          Data was read.
  @retval EFI_NO_MEDIA         The device has no medium.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_DEVICE_ERROR     An attempt was made to read from a deleted file.
  @retval EFI_DEVICE_ERROR     On entry, the current file position is beyond the end of the file.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_BUFFER_TOO_SMALL The BufferSize is too small to read the current directory
                               entry. BufferSize has been updated with the size
                               needed to complete the request.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemRead (
  IN     EFI_FILE_PROTOCOL      *This,
  IN OUT UINTN                  *BufferSize,
     OUT VOID                   *Buffer
  );

/**
  Writes data to a file.

  @param  This       A pointer to the EFI_FILE_PROTOCOL instance that is the file
                     handle to write data to.
  @param  BufferSize On input, the size of the Buffer. On output, the amount of data
                     actually written. In both cases, the size is measured in bytes.
  @param  Buffer     The buffer of data to write.

  @retval EFI_SUCCESS          Data was written.
  @retval EFI_UNSUPPORTED      Writes to open directory files are not supported.
  @retval EFI_NO_MEDIA         The device has no medium.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_DEVICE_ERROR     An attempt was made to write to a deleted file.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  The file or medium is write-protected.
  @retval EFI_ACCESS_DENIED    The file was opened read only.
  @retval EFI_VOLUME_FULL      The volume is full.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemWrite (
  IN     EFI_FILE_PROTOCOL    *This,
  IN OUT UINTN                *BufferSize,
  IN     VOID                 *Buffer
  );

/**
  Returns a file's current position.

  @param  This            A pointer to the EFI_FILE_PROTOCOL instance that is the file
                          handle to get the current position on.
  @param  Position        The address to return the file's current position value.

  @retval EFI_SUCCESS      The position was returned.
  @retval EFI_UNSUPPORTED  The request is not valid on open directories.
  @retval EFI_DEVICE_ERROR An attempt was made to get the position from a deleted file.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemGetPosition (
  IN     EFI_FILE_PROTOCOL    *This,
     OUT UINT64               *Position
  );

/**
  Sets a file's current position.

  @param  This            A pointer to the EFI_FILE_PROTOCOL instance that is the
                          file handle to set the requested position on.
  @param  Position        The byte position from the start of the file to set.

  @retval EFI_SUCCESS      The position was set.
  @retval EFI_UNSUPPORTED  The seek request for nonzero is not valid on open
                           directories.
  @retval EFI_DEVICE_ERROR An attempt was made to set the position of a deleted file.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemSetPosition (
  IN EFI_FILE_PROTOCOL        *This,
  IN UINT64                   Position
  );

/**
  Flushes all modified data associated with a file to a device.

  @param  This A pointer to the EFI_FILE_PROTOCOL instance that is the file
               handle to flush.

  @retval EFI_SUCCESS          The data was flushed.
  @retval EFI_NO_MEDIA         The device has no medium.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  The file or medium is write-protected.
  @retval EFI_ACCESS_DENIED    The file was opened read-only.
  @retval EFI_VOLUME_FULL      The volume is full.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemFlush (
  IN EFI_FILE_PROTOCOL  *This
  );

/**
  Close and delete the file handle.

  @param  This                     A pointer to the EFI_FILE_PROTOCOL instance that is the
                                   handle to the file to delete.

  @retval EFI_SUCCESS              The file was closed and deleted, and the handle was closed.
  @retval EFI_WARN_DELETE_FAILURE  The handle was closed, but the file was not deleted.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemDelete (
  IN EFI_FILE_PROTOCOL *This
  );

/**
  Returns information about a file.

  @param  This            A pointer to the EFI_FILE_PROTOCOL instance that is the file
                          handle the requested information is for.
  @param  InformationType The type identifier for the information being requested.
  @param  BufferSize      On input, the size of Buffer. On output, the amount of data
                          returned in Buffer. In both cases, the size is measured in bytes.
  @param  Buffer          A pointer to the data buffer to return. The buffer's type is
                          indicated by InformationType.

  @retval EFI_SUCCESS          The information was returned.
  @retval EFI_UNSUPPORTED      The InformationType is not known.
  @retval EFI_NO_MEDIA         The device has no medium.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_BUFFER_TOO_SMALL The BufferSize is too small to read the current directory entry.
                               BufferSize has been updated with the size needed to complete
                               the request.
**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemGetInfo (
  IN     EFI_FILE_PROTOCOL    *This,
  IN     EFI_GUID             *InformationType,
  IN OUT UINTN                *BufferSize,
     OUT VOID                 *Buffer
  );

/**
  Sets information about a file.

  @param  This            A pointer to the EFI_FILE_PROTOCOL instance that is the file
                          handle the information is for.
  @param  InformationType The type identifier for the information being set.
  @param  BufferSize      The size, in bytes, of Buffer.
  @param  Buffer          A pointer to the data buffer to write. The buffer's type is
                          indicated by InformationType.

  @retval EFI_SUCCESS          The information was set.
  @retval EFI_UNSUPPORTED      The InformationType is not known.
  @retval EFI_NO_MEDIA         The device has no medium.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  InformationType is EFI_FILE_INFO_ID and the media is
                               read-only.
  @retval EFI_WRITE_PROTECTED  InformationType is EFI_FILE_PROTOCOL_SYSTEM_INFO_ID
                               and the media is read only.
  @retval EFI_WRITE_PROTECTED  InformationType is EFI_FILE_SYSTEM_VOLUME_LABEL_ID
                               and the media is read-only.
  @retval EFI_ACCESS_DENIED    An attempt is made to change the name of a file to a
                               file that is already present.
  @retval EFI_ACCESS_DENIED    An attempt is being made to change the EFI_FILE_DIRECTORY
                               Attribute.
  @retval EFI_ACCESS_DENIED    An attempt is being made to change the size of a directory.
  @retval EFI_ACCESS_DENIED    InformationType is EFI_FILE_INFO_ID and the file was opened
                               read-only and an attempt is being made to modify a field
                               other than Attribute.
  @retval EFI_VOLUME_FULL      The volume is full.
  @retval EFI_BAD_BUFFER_SIZE  BufferSize is smaller than the size of the type indicated
                               by InformationType.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemSetInfo (
  IN EFI_FILE_PROTOCOL        *This,
  IN EFI_GUID                 *InformationType,
  IN UINTN                    BufferSize,
  IN VOID                     *Buffer
  );

/**
  Get the size of the buffer that will be returned by FvFsReadFile.

  @param  FvProtocol                  A pointer to the EFI_FIRMWARE_VOLUME2_PROTOCOL instance.
  @param  FvFileInfo                  A pointer to the FV_FILESYSTEM_FILE_INFO instance that is a struct
                                      representing a file's info.

  @retval EFI_SUCCESS                 The file size was gotten correctly.
  @retval Others                      The file size wasn't gotten correctly.

**/
EFI_STATUS
FvFsGetFileSize (
  IN     EFI_FIRMWARE_VOLUME2_PROTOCOL     *FvProtocol,
  IN OUT FV_FILESYSTEM_FILE_INFO           *FvFileInfo
  );

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.

  @param  DriverName[out]       A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER DriverName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle[in]  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param  ChildHandle[in]       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param  ControllerName[out]   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

extern EFI_UNICODE_COLLATION_PROTOCOL  *mUnicodeCollation;
extern EFI_FILE_PROTOCOL               mFileSystemTemplate;
extern EFI_COMPONENT_NAME_PROTOCOL     gFvSimpleFileSystemComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL    gFvSimpleFileSystemComponentName2;

#endif
