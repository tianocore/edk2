/*++

Copyright (c) 2005 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  Fat.h

Abstract:

  Main header file for EFI FAT file system driver

Revision History

--*/

#ifndef _FAT_H_
#define _FAT_H_

#include <Uefi.h>

#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>
#include <Guid/FileSystemVolumeLabelInfo.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/UnicodeCollation.h>

#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "FatFileSystem.h"

//
// The FAT signature
//
#define FAT_VOLUME_SIGNATURE         SIGNATURE_32 ('f', 'a', 't', 'v')
#define FAT_IFILE_SIGNATURE          SIGNATURE_32 ('f', 'a', 't', 'i')
#define FAT_ODIR_SIGNATURE           SIGNATURE_32 ('f', 'a', 't', 'd')
#define FAT_DIRENT_SIGNATURE         SIGNATURE_32 ('f', 'a', 't', 'e')
#define FAT_OFILE_SIGNATURE          SIGNATURE_32 ('f', 'a', 't', 'o')
#define FAT_TASK_SIGNATURE           SIGNATURE_32 ('f', 'a', 't', 'T')
#define FAT_SUBTASK_SIGNATURE        SIGNATURE_32 ('f', 'a', 't', 'S')

#define ASSERT_VOLUME_LOCKED(a)      ASSERT_LOCKED (&FatFsLock)

#define IFILE_FROM_FHAND(a)          CR (a, FAT_IFILE, Handle, FAT_IFILE_SIGNATURE)

#define DIRENT_FROM_LINK(a)          CR (a, FAT_DIRENT, Link, FAT_DIRENT_SIGNATURE)

#define VOLUME_FROM_ROOT_DIRENT(a)   CR (a, FAT_VOLUME, RootDirEnt, FAT_VOLUME_SIGNATURE)

#define VOLUME_FROM_VOL_INTERFACE(a) CR (a, FAT_VOLUME, VolumeInterface, FAT_VOLUME_SIGNATURE);

#define ODIR_FROM_DIRCACHELINK(a)    CR (a, FAT_ODIR, DirCacheLink, FAT_ODIR_SIGNATURE)

#define OFILE_FROM_CHECKLINK(a)      CR (a, FAT_OFILE, CheckLink, FAT_OFILE_SIGNATURE)

#define OFILE_FROM_CHILDLINK(a)      CR (a, FAT_OFILE, ChildLink, FAT_OFILE_SIGNATURE)

//
// Minimum sector size is 512B, Maximum sector size is 4096B
// Max sectors per cluster is 128
//
#define MAX_BLOCK_ALIGNMENT               12
#define MIN_BLOCK_ALIGNMENT               9
#define MAX_SECTORS_PER_CLUSTER_ALIGNMENT 7

//
// Efi Time Definition
//
#define IS_LEAP_YEAR(a)                   (((a) % 4 == 0) && (((a) % 100 != 0) || ((a) % 400 == 0)))

//
// Minimum fat page size is 8K, maximum fat page alignment is 32K
// Minimum data page size is 8K, maximum fat page alignment is 64K
//
#define FAT_FATCACHE_PAGE_MIN_ALIGNMENT   13
#define FAT_FATCACHE_PAGE_MAX_ALIGNMENT   15
#define FAT_DATACACHE_PAGE_MIN_ALIGNMENT  13
#define FAT_DATACACHE_PAGE_MAX_ALIGNMENT  16
#define FAT_DATACACHE_GROUP_COUNT         64
#define FAT_FATCACHE_GROUP_MIN_COUNT      1
#define FAT_FATCACHE_GROUP_MAX_COUNT      16

//
// Used in 8.3 generation algorithm
//
#define MAX_SPEC_RETRY          4
#define SPEC_BASE_TAG_LEN       6
#define HASH_BASE_TAG_LEN       2
#define HASH_VALUE_TAG_LEN      (SPEC_BASE_TAG_LEN - HASH_BASE_TAG_LEN)

//
// Path name separator is back slash
//
#define PATH_NAME_SEPARATOR     L'\\'


#define EFI_PATH_STRING_LENGTH  260
#define EFI_FILE_STRING_LENGTH  255
#define FAT_MAX_ALLOCATE_SIZE   0xA00000
#define LC_ISO_639_2_ENTRY_SIZE 3
#define MAX_LANG_CODE_SIZE      100

#define FAT_MAX_DIR_CACHE_COUNT 8
#define FAT_MAX_DIRENTRY_COUNT  0xFFFF
typedef CHAR8                   LC_ISO_639_2;

//
// The fat types we support
//
typedef enum {
  FAT12,
  FAT16,
  FAT32,
  FatUndefined
} FAT_VOLUME_TYPE;

typedef enum {
  CACHE_FAT,
  CACHE_DATA,
  CACHE_MAX_TYPE
} CACHE_DATA_TYPE;

//
// Used in FatDiskIo
//
typedef enum {
  READ_DISK     = 0,  // raw disk read
  WRITE_DISK    = 1,  // raw disk write
  READ_FAT      = 2,  // read fat cache
  WRITE_FAT     = 3,  // write fat cache
  READ_DATA     = 6,  // read data cache
  WRITE_DATA    = 7   // write data cache
} IO_MODE;

#define CACHE_ENABLED(a)  ((a) >= 2)
#define RAW_ACCESS(a)     ((IO_MODE)((a) & 0x1))
#define CACHE_TYPE(a)     ((CACHE_DATA_TYPE)((a) >> 2))

//
// Disk cache tag
//
typedef struct {
  UINTN   PageNo;
  UINTN   RealSize;
  BOOLEAN Dirty;
} CACHE_TAG;

typedef struct {
  UINT64    BaseAddress;
  UINT64    LimitAddress;
  UINT8     *CacheBase;
  BOOLEAN   Dirty;
  UINT8     PageAlignment;
  UINTN     GroupMask;
  CACHE_TAG CacheTag[FAT_DATACACHE_GROUP_COUNT];
} DISK_CACHE;

//
// Hash table size
//
#define HASH_TABLE_SIZE  0x400
#define HASH_TABLE_MASK  (HASH_TABLE_SIZE - 1)

//
// The directory entry for opened directory
//
typedef struct _FAT_DIRENT {
  UINTN               Signature;
  UINT16              EntryPos;               // The position of this directory entry in the parent directory file
  UINT8               EntryCount;             // The count of the directory entry in the parent directory file
  BOOLEAN             Invalid;                // Indicate whether this directory entry is valid
  CHAR16              *FileString;            // The unicode long file name for this directory entry
  struct _FAT_OFILE   *OFile;                 // The OFile of the corresponding directory entry
  struct _FAT_DIRENT  *ShortNameForwardLink;  // Hash successor link for short filename
  struct _FAT_DIRENT  *LongNameForwardLink;   // Hash successor link for long filename
  LIST_ENTRY          Link;                   // Connection of every directory entry
  FAT_DIRECTORY_ENTRY Entry;                  // The physical directory entry stored in disk
} FAT_DIRENT;

typedef struct _FAT_ODIR {
  UINTN               Signature;
  UINT32              CurrentEndPos;          // Current end position of the directory
  UINT32              CurrentPos;             // Current position of the directory
  LIST_ENTRY          *CurrentCursor;         // Current directory entry pointer
  LIST_ENTRY          ChildList;              // List of all directory entries
  BOOLEAN             EndOfDir;               // Indicate whether we have reached the end of the directory
  LIST_ENTRY          DirCacheLink;           // Linked in Volume->DirCacheList when discarded
  UINTN               DirCacheTag;            // The identification of the directory when in directory cache
  FAT_DIRENT          *LongNameHashTable[HASH_TABLE_SIZE];
  FAT_DIRENT          *ShortNameHashTable[HASH_TABLE_SIZE];
} FAT_ODIR;

typedef struct {
  UINTN               Signature;
  EFI_FILE_PROTOCOL   Handle;
  UINT64              Position;
  BOOLEAN             ReadOnly;
  struct _FAT_OFILE   *OFile;
  LIST_ENTRY          Tasks;                  // List of all FAT_TASKs
  LIST_ENTRY          Link;                   // Link to other IFiles
} FAT_IFILE;

typedef struct {
  UINTN               Signature;
  EFI_FILE_IO_TOKEN   *FileIoToken;
  FAT_IFILE           *IFile;
  LIST_ENTRY          Subtasks;               // List of all FAT_SUBTASKs
  LIST_ENTRY          Link;                   // Link to other FAT_TASKs
} FAT_TASK;

typedef struct {
  UINTN               Signature;
  EFI_DISK_IO2_TOKEN  DiskIo2Token;
  FAT_TASK            *Task;
  BOOLEAN             Write;
  UINT64              Offset;
  VOID                *Buffer;
  UINTN               BufferSize;
  LIST_ENTRY          Link;
} FAT_SUBTASK;

//
// FAT_OFILE - Each opened file
//
typedef struct _FAT_OFILE {
  UINTN               Signature;
  struct _FAT_VOLUME  *Volume;
  //
  // A permanant error code to return to all accesses to
  // this opened file
  //
  EFI_STATUS          Error;
  //
  // A list of the IFILE instances for this OFile
  //
  LIST_ENTRY          Opens;

  //
  // The dynamic infomation
  //
  UINTN               FileSize;
  UINTN               FileCluster;
  UINTN               FileCurrentCluster;
  UINTN               FileLastCluster;

  //
  // Dirty is set if there have been any updates to the
  // file
  // Archive is set if the archive attribute in the file's
  // directory entry needs to be set when performing flush
  // PreserveLastMod is set if the last modification of the
  // file is specified by SetInfo API
  //
  BOOLEAN             Dirty;
  BOOLEAN             IsFixedRootDir;
  BOOLEAN             PreserveLastModification;
  BOOLEAN             Archive;
  //
  // Set by an OFile SetPosition
  //
  UINTN               Position; // within file
  UINT64              PosDisk;  // on the disk
  UINTN               PosRem;   // remaining in this disk run
  //
  // The opened parent, full path length and currently opened child files
  //
  struct _FAT_OFILE   *Parent;
  UINTN               FullPathLen;
  LIST_ENTRY          ChildHead;
  LIST_ENTRY          ChildLink;

  //
  // The opened directory structure for a directory; if this
  // OFile represents a file, then ODir = NULL
  //
  FAT_ODIR            *ODir;
  //
  // The directory entry for the Ofile
  //
  FAT_DIRENT          *DirEnt;

  //
  // Link in Volume's reference list
  //
  LIST_ENTRY          CheckLink;
} FAT_OFILE;

typedef struct _FAT_VOLUME {
  UINTN                           Signature;

  EFI_HANDLE                      Handle;
  BOOLEAN                         Valid;
  BOOLEAN                         DiskError;

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL VolumeInterface;

  //
  // If opened, the parent handle and BlockIo interface
  //
  EFI_BLOCK_IO_PROTOCOL           *BlockIo;
  EFI_DISK_IO_PROTOCOL            *DiskIo;
  EFI_DISK_IO2_PROTOCOL           *DiskIo2;
  UINT32                          MediaId;
  BOOLEAN                         ReadOnly;

  //
  // Computed values from fat bpb info
  //
  UINT64                          VolumeSize;
  UINT64                          FatPos;           // Disk pos of fat tables
  UINT64                          RootPos;          // Disk pos of root directory
  UINT64                          FirstClusterPos;  // Disk pos of first cluster
  UINTN                           FatSize;          // Number of bytes in each fat
  UINTN                           MaxCluster;       // Max cluster number
  UINTN                           ClusterSize;      // Cluster size of fat partition
  UINT8                           ClusterAlignment; // Equal to log_2 (clustersize);
  FAT_VOLUME_TYPE                 FatType;

  //
  // Current part of fat table that's present
  //
  UINT64                          FatEntryPos;    // Location of buffer
  UINTN                           FatEntrySize;   // Size of buffer
  UINT32                          FatEntryBuffer; // The buffer
  FAT_INFO_SECTOR                 FatInfoSector;  // Free cluster info
  UINTN                           FreeInfoPos;    // Pos with the free cluster info
  BOOLEAN                         FreeInfoValid;  // If free cluster info is valid
  //
  // Unpacked Fat BPB info
  //
  UINTN                           NumFats;
  UINTN                           RootEntries;    // < FAT32, root dir is fixed size
  UINTN                           RootCluster;    // >= FAT32, root cluster chain head
  //
  // info for marking the volume dirty or not
  //
  BOOLEAN                         FatDirty;       // If fat-entries have been updated
  UINT32                          DirtyValue;
  UINT32                          NotDirtyValue;

  //
  // The root directory entry and opened root file
  //
  FAT_DIRENT                      RootDirEnt;
  //
  // File Name of root OFile, it is empty string
  //
  CHAR16                          RootFileString[1];
  struct _FAT_OFILE               *Root;

  //
  // New OFiles are added to this list so they
  // can be cleaned up if they aren't referenced.
  //
  LIST_ENTRY                      CheckRef;

  //
  // Directory cache List
  //
  LIST_ENTRY                      DirCacheList;
  UINTN                           DirCacheCount;

  //
  // Disk Cache for this volume
  //
  VOID                            *CacheBuffer;
  DISK_CACHE                      DiskCache[CACHE_MAX_TYPE];
} FAT_VOLUME;

//
// Function Prototypes
//
EFI_STATUS
EFIAPI
FatOpen (
  IN  EFI_FILE_PROTOCOL *FHand,
  OUT EFI_FILE_PROTOCOL **NewHandle,
  IN  CHAR16            *FileName,
  IN  UINT64            OpenMode,
  IN  UINT64            Attributes
  )
/*++
Routine Description:

  Implements Open() of Simple File System Protocol.

Arguments:

  FHand                 - File handle of the file serves as a starting reference point.
  NewHandle             - Handle of the file that is newly opened.
  FileName              - File name relative to FHand.
  OpenMode              - Open mode.
  Attributes            - Attributes to set if the file is created.

Returns:

  EFI_INVALID_PARAMETER - The FileName is NULL or the file string is empty.
                          The OpenMode is not supported.
                          The Attributes is not the valid attributes.
  EFI_OUT_OF_RESOURCES  - Can not allocate the memory for file string.
  EFI_SUCCESS           - Open the file successfully.
  Others                - The status of open file.

--*/
;

EFI_STATUS
EFIAPI
FatOpenEx (
  IN  EFI_FILE_PROTOCOL       *FHand,
  OUT EFI_FILE_PROTOCOL       **NewHandle,
  IN  CHAR16                  *FileName,
  IN  UINT64                  OpenMode,
  IN  UINT64                  Attributes,
  IN OUT EFI_FILE_IO_TOKEN    *Token
  )
/*++
Routine Description:

  Implements OpenEx() of Simple File System Protocol.

Arguments:

  FHand                 - File handle of the file serves as a starting reference point.
  NewHandle             - Handle of the file that is newly opened.
  FileName              - File name relative to FHand.
  OpenMode              - Open mode.
  Attributes            - Attributes to set if the file is created.
  Token                 - A pointer to the token associated with the transaction.

Returns:

  EFI_INVALID_PARAMETER - The FileName is NULL or the file string is empty.
                          The OpenMode is not supported.
                          The Attributes is not the valid attributes.
  EFI_OUT_OF_RESOURCES  - Can not allocate the memory for file string.
  EFI_SUCCESS           - Open the file successfully.
  Others                - The status of open file.

--*/
;

EFI_STATUS
EFIAPI
FatGetPosition (
  IN  EFI_FILE_PROTOCOL *FHand,
  OUT UINT64            *Position
  )
/*++

Routine Description:

  Get the file's position of the file

Arguments:

  FHand                 - The handle of file.
  Position              - The file's position of the file.

Returns:

  EFI_SUCCESS           - Get the info successfully.
  EFI_DEVICE_ERROR      - Can not find the OFile for the file.
  EFI_UNSUPPORTED       - The open file is not a file.

--*/
;

EFI_STATUS
EFIAPI
FatGetInfo (
  IN     EFI_FILE_PROTOCOL      *FHand,
  IN     EFI_GUID               *Type,
  IN OUT UINTN                  *BufferSize,
     OUT VOID                   *Buffer
  )
/*++

Routine Description:

  Get the some types info of the file into Buffer

Arguments:

  FHand                 - The handle of file.
  Type                  - The type of the info.
  BufferSize            - Size of Buffer.
  Buffer                - Buffer containing volume info.

Returns:

  EFI_SUCCESS           - Get the info successfully.
  EFI_DEVICE_ERROR      - Can not find the OFile for the file.

--*/
;

EFI_STATUS
EFIAPI
FatSetInfo (
  IN EFI_FILE_PROTOCOL  *FHand,
  IN EFI_GUID           *Type,
  IN UINTN              BufferSize,
  IN VOID               *Buffer
  )
/*++

Routine Description:

  Set the some types info of the file into Buffer

Arguments:

  FHand                 - The handle of file.
  Type                  - The type of the info.
  BufferSize            - Size of Buffer.
  Buffer                - Buffer containing volume info.

Returns:

  EFI_SUCCESS           - Set the info successfully.
  EFI_DEVICE_ERROR      - Can not find the OFile for the file.

--*/
;

EFI_STATUS
EFIAPI
FatFlush (
  IN EFI_FILE_PROTOCOL  *FHand
  )
/*++

Routine Description:

  Flushes all data associated with the file handle

Arguments:

  FHand                 - Handle to file to flush

Returns:

  EFI_SUCCESS           - Flushed the file successfully
  EFI_WRITE_PROTECTED   - The volume is read only
  EFI_ACCESS_DENIED     - The volume is not read only
                          but the file is read only
  Others                - Flushing of the file is failed

--*/
;

EFI_STATUS
EFIAPI
FatFlushEx (
  IN EFI_FILE_PROTOCOL  *FHand,
  IN EFI_FILE_IO_TOKEN  *Token
  )
/*++

Routine Description:

  Flushes all data associated with the file handle.

Arguments:

  FHand                 - Handle to file to flush.
  Token                 - A pointer to the token associated with the transaction.

Returns:

  EFI_SUCCESS           - Flushed the file successfully.
  EFI_WRITE_PROTECTED   - The volume is read only.
  EFI_ACCESS_DENIED     - The file is read only.
  Others                - Flushing of the file failed.

--*/
;

EFI_STATUS
EFIAPI
FatClose (
  IN EFI_FILE_PROTOCOL  *FHand
  )
/*++

Routine Description:

  Flushes & Closes the file handle.

Arguments:

  FHand                 - Handle to the file to delete.

Returns:

  EFI_SUCCESS           - Closed the file successfully.

--*/
;

EFI_STATUS
EFIAPI
FatDelete (
  IN EFI_FILE_PROTOCOL  *FHand
  )
/*++

Routine Description:

  Deletes the file & Closes the file handle.

Arguments:

  FHand                    - Handle to the file to delete.

Returns:

  EFI_SUCCESS              - Delete the file successfully.
  EFI_WARN_DELETE_FAILURE  - Fail to delete the file.

--*/
;

EFI_STATUS
EFIAPI
FatSetPosition (
  IN EFI_FILE_PROTOCOL  *FHand,
  IN UINT64             Position
  )
/*++

Routine Description:

  Set the file's position of the file

Arguments:

  FHand                 - The handle of file
  Position              - The file's position of the file

Returns:

  EFI_SUCCESS           - Set the info successfully
  EFI_DEVICE_ERROR      - Can not find the OFile for the file
  EFI_UNSUPPORTED       - Set a directory with a not-zero position

--*/
;

EFI_STATUS
EFIAPI
FatRead (
  IN     EFI_FILE_PROTOCOL    *FHand,
  IN OUT UINTN                *BufferSize,
     OUT VOID                 *Buffer
  )
/*++

Routine Description:

  Get the file info.

Arguments:

  FHand                 - The handle of the file.
  BufferSize            - Size of Buffer.
  Buffer                - Buffer containing read data.

Returns:

  EFI_SUCCESS           - Get the file info successfully.
  EFI_DEVICE_ERROR      - Can not find the OFile for the file.
  EFI_VOLUME_CORRUPTED  - The file type of open file is error.
  other                 - An error occurred when operation the disk.

--*/
;

EFI_STATUS
EFIAPI
FatReadEx (
  IN     EFI_FILE_PROTOCOL  *FHand,
  IN OUT EFI_FILE_IO_TOKEN  *Token
  )
/*++

Routine Description:

  Get the file info.

Arguments:

  FHand                 - The handle of the file.
  Token                 - A pointer to the token associated with the transaction.

Returns:

  EFI_SUCCESS           - Get the file info successfully.
  EFI_DEVICE_ERROR      - Can not find the OFile for the file.
  EFI_VOLUME_CORRUPTED  - The file type of open file is error.
  other                 - An error occurred when operation the disk.

--*/
;

EFI_STATUS
EFIAPI
FatWrite (
  IN     EFI_FILE_PROTOCOL      *FHand,
  IN OUT UINTN                  *BufferSize,
  IN     VOID                   *Buffer
  )
/*++

Routine Description:

  Set the file info.

Arguments:

  FHand                 - The handle of the file.
  BufferSize            - Size of Buffer.
  Buffer                - Buffer containing write data.

Returns:

  EFI_SUCCESS           - Set the file info successfully.
  EFI_WRITE_PROTECTED   - The disk is write protected.
  EFI_ACCESS_DENIED     - The file is read-only.
  EFI_DEVICE_ERROR      - The OFile is not valid.
  EFI_UNSUPPORTED       - The open file is not a file.
                        - The writing file size is larger than 4GB.
  other                 - An error occurred when operation the disk.

--*/
;

EFI_STATUS
EFIAPI
FatWriteEx (
  IN     EFI_FILE_PROTOCOL  *FHand,
  IN OUT EFI_FILE_IO_TOKEN  *Token
  )
/*++

Routine Description:

  Get the file info.

Arguments:

  FHand                 - The handle of the file.
  Token                 - A pointer to the token associated with the transaction.

Returns:

  EFI_SUCCESS           - Get the file info successfully.
  EFI_DEVICE_ERROR      - Can not find the OFile for the file.
  EFI_VOLUME_CORRUPTED  - The file type of open file is error.
  other                 - An error occurred when operation the disk.

--*/
;

//
// DiskCache.c
//
EFI_STATUS
FatInitializeDiskCache (
  IN FAT_VOLUME              *Volume
  );

EFI_STATUS
FatAccessCache (
  IN     FAT_VOLUME          *Volume,
  IN     CACHE_DATA_TYPE     CacheDataType,
  IN     IO_MODE             IoMode,
  IN     UINT64              Offset,
  IN     UINTN               BufferSize,
  IN OUT UINT8               *Buffer,
  IN     FAT_TASK            *Task
  );

EFI_STATUS
FatVolumeFlushCache (
  IN FAT_VOLUME              *Volume,
  IN FAT_TASK                *Task
  );

//
// Flush.c
//
EFI_STATUS
FatOFileFlush (
  IN FAT_OFILE          *OFile
  );

BOOLEAN
FatCheckOFileRef (
  IN FAT_OFILE          *OFile
  );

VOID
FatSetVolumeError (
  IN FAT_OFILE          *OFile,
  IN EFI_STATUS         Status
  );

EFI_STATUS
FatIFileClose (
  FAT_IFILE             *IFile
  );

EFI_STATUS
FatCleanupVolume (
  IN FAT_VOLUME         *Volume,
  IN FAT_OFILE          *OFile,
  IN EFI_STATUS         EfiStatus,
  IN FAT_TASK           *Task
  );

//
// FileSpace.c
//
EFI_STATUS
FatShrinkEof (
  IN FAT_OFILE          *OFile
  );

EFI_STATUS
FatGrowEof (
  IN FAT_OFILE          *OFile,
  IN UINT64             NewSizeInBytes
  );

UINTN
FatPhysicalDirSize (
  IN FAT_VOLUME         *Volume,
  IN UINTN              Cluster
  );

UINT64
FatPhysicalFileSize (
  IN FAT_VOLUME         *Volume,
  IN UINTN              RealSize
  );

EFI_STATUS
FatOFilePosition (
  IN FAT_OFILE            *OFile,
  IN UINTN                Position,
  IN UINTN                PosLimit
  );

VOID
FatComputeFreeInfo (
  IN FAT_VOLUME         *Volume
  );

//
// Init.c
//
EFI_STATUS
FatAllocateVolume (
  IN  EFI_HANDLE                     Handle,
  IN  EFI_DISK_IO_PROTOCOL           *DiskIo,
  IN  EFI_DISK_IO2_PROTOCOL          *DiskIo2,
  IN  EFI_BLOCK_IO_PROTOCOL          *BlockIo
  );

EFI_STATUS
FatOpenDevice (
  IN OUT FAT_VOLUME     *Volume
  );

EFI_STATUS
FatAbandonVolume (
  IN FAT_VOLUME         *Volume
  );

//
// Misc.c
//
FAT_TASK *
FatCreateTask (
  FAT_IFILE           *IFile,
  EFI_FILE_IO_TOKEN   *Token
  );

VOID
FatDestroyTask (
  FAT_TASK            *Task
  );

VOID
FatWaitNonblockingTask (
  FAT_IFILE           *IFile
  );

LIST_ENTRY *
FatDestroySubtask (
  FAT_SUBTASK         *Subtask
  );

EFI_STATUS
FatQueueTask (
  IN FAT_IFILE        *IFile,
  IN FAT_TASK         *Task
  );

EFI_STATUS
FatAccessVolumeDirty (
  IN FAT_VOLUME         *Volume,
  IN IO_MODE            IoMode,
  IN VOID               *DirtyValue
  );

EFI_STATUS
FatDiskIo (
  IN FAT_VOLUME         *Volume,
  IN IO_MODE            IoMode,
  IN UINT64             Offset,
  IN UINTN              BufferSize,
  IN OUT VOID           *Buffer,
  IN FAT_TASK           *Task
  );

VOID
FatAcquireLock (
  VOID
  );

VOID
FatReleaseLock (
  VOID
  );

EFI_STATUS
FatAcquireLockOrFail (
  VOID
  );

VOID
FatFreeDirEnt (
  IN FAT_DIRENT         *DirEnt
  );

VOID
FatFreeVolume (
  IN FAT_VOLUME         *Volume
  );

VOID
FatEfiTimeToFatTime (
  IN EFI_TIME           *ETime,
  OUT FAT_DATE_TIME     *FTime
  );

VOID
FatFatTimeToEfiTime (
  IN FAT_DATE_TIME      *FTime,
  OUT EFI_TIME          *ETime
  );

VOID
FatGetCurrentFatTime (
  OUT FAT_DATE_TIME     *FatTime
  );

BOOLEAN
FatIsValidTime (
  IN EFI_TIME           *Time
  );

//
// UnicodeCollation.c
//
EFI_STATUS
InitializeUnicodeCollationSupport (
  IN EFI_HANDLE    AgentHandle
  );

VOID
FatFatToStr (
  IN UINTN              FatSize,
  IN CHAR8              *Fat,
  OUT CHAR16            *String
  );

BOOLEAN
FatStrToFat (
  IN  CHAR16            *String,
  IN  UINTN             FatSize,
  OUT CHAR8             *Fat
  );

VOID
FatStrLwr (
  IN CHAR16             *Str
  );

VOID
FatStrUpr (
  IN CHAR16             *Str
  );

INTN
FatStriCmp (
  IN CHAR16             *Str1,
  IN CHAR16             *Str2
  );

//
// Open.c
//
EFI_STATUS
FatOFileOpen (
  IN FAT_OFILE          *OFile,
  OUT FAT_IFILE         **NewIFile,
  IN CHAR16             *FileName,
  IN UINT64             OpenMode,
  IN UINT8              Attributes
  );

EFI_STATUS
FatAllocateIFile (
  IN FAT_OFILE          *OFile,
  OUT FAT_IFILE         **PtrIFile
  );

//
// OpenVolume.c
//
EFI_STATUS
EFIAPI
FatOpenVolume (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
  OUT EFI_FILE_PROTOCOL               **File
  );

//
// ReadWrite.c
//
EFI_STATUS
FatAccessOFile (
  IN FAT_OFILE          *OFile,
  IN IO_MODE            IoMode,
  IN UINTN              Position,
  IN UINTN              *DataBufferSize,
  IN UINT8              *UserBuffer,
  IN FAT_TASK           *Task
  );

EFI_STATUS
FatExpandOFile (
  IN FAT_OFILE          *OFile,
  IN UINT64             ExpandedSize
  );

EFI_STATUS
FatWriteZeroPool (
  IN FAT_OFILE          *OFile,
  IN UINTN              WritePos
  );

EFI_STATUS
FatTruncateOFile (
  IN FAT_OFILE          *OFile,
  IN UINTN              TruncatedSize
  );

//
// DirectoryManage.c
//
VOID
FatResetODirCursor (
  IN FAT_OFILE          *OFile
  );

EFI_STATUS
FatGetNextDirEnt (
  IN  FAT_OFILE         *OFILE,
  OUT FAT_DIRENT        **PtrDirEnt
  );

EFI_STATUS
FatRemoveDirEnt (
  IN FAT_OFILE          *OFile,
  IN FAT_DIRENT         *DirEnt
  );

EFI_STATUS
FatStoreDirEnt (
  IN FAT_OFILE          *OFile,
  IN FAT_DIRENT         *DirEnt
  );

EFI_STATUS
FatCreateDirEnt (
  IN  FAT_OFILE         *OFile,
  IN  CHAR16            *FileName,
  IN  UINT8             Attributes,
  OUT FAT_DIRENT        **PtrDirEnt
  );

BOOLEAN
FatIsDotDirEnt (
  IN FAT_DIRENT         *DirEnt
  );

VOID
FatUpdateDirEntClusterSizeInfo (
  IN FAT_OFILE          *OFile
  );

VOID
FatCloneDirEnt (
  IN  FAT_DIRENT        *DirEnt1,
  IN  FAT_DIRENT        *DirEnt2
  );

EFI_STATUS
FatGetDirEntInfo (
  IN FAT_VOLUME         *Volume,
  IN FAT_DIRENT         *DirEnt,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  );

EFI_STATUS
FatOpenDirEnt (
  IN FAT_OFILE          *OFile,
  IN FAT_DIRENT         *DirEnt
  );

EFI_STATUS
FatCreateDotDirEnts (
  IN FAT_OFILE          *OFile
  );

VOID
FatCloseDirEnt (
  IN FAT_DIRENT         *DirEnt
  );

EFI_STATUS
FatLocateOFile (
  IN OUT FAT_OFILE      **PtrOFile,
  IN     CHAR16         *FileName,
  IN     UINT8          Attributes,
     OUT CHAR16         *NewFileName
  );

EFI_STATUS
FatGetVolumeEntry (
  IN FAT_VOLUME         *Volume,
  IN CHAR16             *Name
  );

EFI_STATUS
FatSetVolumeEntry (
  IN FAT_VOLUME         *Volume,
  IN CHAR16             *Name
  );

//
// Hash.c
//
FAT_DIRENT **
FatLongNameHashSearch (
  IN FAT_ODIR           *ODir,
  IN CHAR16             *LongNameString
  );

FAT_DIRENT **
FatShortNameHashSearch (
  IN FAT_ODIR           *ODir,
  IN CHAR8              *ShortNameString
  );

VOID
FatInsertToHashTable (
  IN FAT_ODIR           *ODir,
  IN FAT_DIRENT         *DirEnt
  );

VOID
FatDeleteFromHashTable (
  IN FAT_ODIR           *ODir,
  IN FAT_DIRENT         *DirEnt
  );

//
// FileName.c
//
BOOLEAN
FatCheckIs8Dot3Name (
  IN CHAR16             *FileName,
  OUT CHAR8             *File8Dot3Name
  );

VOID
FatCreate8Dot3Name (
  IN FAT_OFILE          *Parent,
  IN FAT_DIRENT         *DirEnt
  );

VOID
FatNameToStr (
  IN CHAR8              *FatName,
  IN UINTN              Len,
  IN UINTN              LowerCase,
  IN CHAR16             *Str
  );

VOID
FatSetCaseFlag (
  IN FAT_DIRENT         *DirEnt
  );

VOID
FatGetFileNameViaCaseFlag (
  IN  FAT_DIRENT        *DirEnt,
  OUT CHAR16            *FileString
  );

UINT8
FatCheckSum (
  IN CHAR8              *ShortNameString
  );

CHAR16*
FatGetNextNameComponent (
  IN  CHAR16            *Path,
  OUT CHAR16            *Name
  );

BOOLEAN
FatFileNameIsValid (
  IN  CHAR16  *InputFileName,
  OUT CHAR16  *OutputFileName
  );

//
// DirectoryCache.c
//
VOID
FatDiscardODir (
  IN FAT_OFILE    *OFile
  );

VOID
FatRequestODir (
  IN FAT_OFILE    *OFile
  );

VOID
FatCleanupODirCache (
  IN FAT_VOLUME   *Volume
  );

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL     gFatDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL     gFatComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL    gFatComponentName2;
extern EFI_LOCK                        FatFsLock;
extern EFI_LOCK                        FatTaskLock;
extern EFI_FILE_PROTOCOL               FatFileInterface;

#endif
