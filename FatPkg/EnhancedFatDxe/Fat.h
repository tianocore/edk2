/** @file
  Main header file for EFI FAT file system driver.

Copyright (c) 2005 - 2013, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

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
#define FAT_VOLUME_SIGNATURE   SIGNATURE_32 ('f', 'a', 't', 'v')
#define FAT_IFILE_SIGNATURE    SIGNATURE_32 ('f', 'a', 't', 'i')
#define FAT_ODIR_SIGNATURE     SIGNATURE_32 ('f', 'a', 't', 'd')
#define FAT_DIRENT_SIGNATURE   SIGNATURE_32 ('f', 'a', 't', 'e')
#define FAT_OFILE_SIGNATURE    SIGNATURE_32 ('f', 'a', 't', 'o')
#define FAT_TASK_SIGNATURE     SIGNATURE_32 ('f', 'a', 't', 'T')
#define FAT_SUBTASK_SIGNATURE  SIGNATURE_32 ('f', 'a', 't', 'S')

#define ASSERT_VOLUME_LOCKED(a)  ASSERT_LOCKED (&FatFsLock)

#define IFILE_FROM_FHAND(a)  CR (a, FAT_IFILE, Handle, FAT_IFILE_SIGNATURE)

#define DIRENT_FROM_LINK(a)  CR (a, FAT_DIRENT, Link, FAT_DIRENT_SIGNATURE)

#define VOLUME_FROM_ROOT_DIRENT(a)  CR (a, FAT_VOLUME, RootDirEnt, FAT_VOLUME_SIGNATURE)

#define VOLUME_FROM_VOL_INTERFACE(a)  CR (a, FAT_VOLUME, VolumeInterface, FAT_VOLUME_SIGNATURE);

#define ODIR_FROM_DIRCACHELINK(a)  CR (a, FAT_ODIR, DirCacheLink, FAT_ODIR_SIGNATURE)

#define OFILE_FROM_CHECKLINK(a)  CR (a, FAT_OFILE, CheckLink, FAT_OFILE_SIGNATURE)

#define OFILE_FROM_CHILDLINK(a)  CR (a, FAT_OFILE, ChildLink, FAT_OFILE_SIGNATURE)

//
// Minimum sector size is 512B, Maximum sector size is 4096B
// Max sectors per cluster is 128
//
#define MAX_BLOCK_ALIGNMENT                12
#define MIN_BLOCK_ALIGNMENT                9
#define MAX_SECTORS_PER_CLUSTER_ALIGNMENT  7

//
// Efi Time Definition
//
#define IS_LEAP_YEAR(a)  (((a) % 4 == 0) && (((a) % 100 != 0) || ((a) % 400 == 0)))

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
#define MAX_SPEC_RETRY      4
#define SPEC_BASE_TAG_LEN   6
#define HASH_BASE_TAG_LEN   2
#define HASH_VALUE_TAG_LEN  (SPEC_BASE_TAG_LEN - HASH_BASE_TAG_LEN)

//
// Path name separator is back slash
//
#define PATH_NAME_SEPARATOR  L'\\'

#define EFI_PATH_STRING_LENGTH   260
#define EFI_FILE_STRING_LENGTH   255
#define FAT_MAX_ALLOCATE_SIZE    0xA00000
#define LC_ISO_639_2_ENTRY_SIZE  3
#define MAX_LANG_CODE_SIZE       100

#define FAT_MAX_DIR_CACHE_COUNT  8
#define FAT_MAX_DIRENTRY_COUNT   0xFFFF
typedef CHAR8 LC_ISO_639_2;

//
// The fat types we support
//
typedef enum {
  Fat12,
  Fat16,
  Fat32,
  FatUndefined
} FAT_VOLUME_TYPE;

typedef enum {
  CacheFat,
  CacheData,
  CacheMaxType
} CACHE_DATA_TYPE;

//
// Used in FatDiskIo
//
typedef enum {
  ReadDisk  = 0,     // raw disk read
  WriteDisk = 1,     // raw disk write
  ReadFat   = 2,     // read fat cache
  WriteFat  = 3,     // write fat cache
  ReadData  = 6,     // read data cache
  WriteData = 7      // write data cache
} IO_MODE;

#define CACHE_ENABLED(a)  ((a) >= 2)
#define RAW_ACCESS(a)     ((IO_MODE)((a) & 0x1))
#define CACHE_TYPE(a)     ((CACHE_DATA_TYPE)((a) >> 2))

//
// Disk cache tag
//
typedef struct {
  UINTN      PageNo;
  UINTN      RealSize;
  BOOLEAN    Dirty;
} CACHE_TAG;

typedef struct {
  UINT64       BaseAddress;
  UINT64       LimitAddress;
  UINT8        *CacheBase;
  BOOLEAN      Dirty;
  UINT8        PageAlignment;
  UINTN        GroupMask;
  CACHE_TAG    CacheTag[FAT_DATACACHE_GROUP_COUNT];
} DISK_CACHE;

//
// Hash table size
//
#define HASH_TABLE_SIZE  0x400
#define HASH_TABLE_MASK  (HASH_TABLE_SIZE - 1)

//
// The directory entry for opened directory
//

typedef struct _FAT_DIRENT FAT_DIRENT;
typedef struct _FAT_ODIR   FAT_ODIR;
typedef struct _FAT_OFILE  FAT_OFILE;
typedef struct _FAT_VOLUME FAT_VOLUME;

struct _FAT_DIRENT {
  UINTN                  Signature;
  UINT16                 EntryPos;              // The position of this directory entry in the parent directory file
  UINT8                  EntryCount;            // The count of the directory entry in the parent directory file
  BOOLEAN                Invalid;               // Indicate whether this directory entry is valid
  CHAR16                 *FileString;           // The unicode long file name for this directory entry
  FAT_OFILE              *OFile;                // The OFile of the corresponding directory entry
  FAT_DIRENT             *ShortNameForwardLink; // Hash successor link for short filename
  FAT_DIRENT             *LongNameForwardLink;  // Hash successor link for long filename
  LIST_ENTRY             Link;                  // Connection of every directory entry
  FAT_DIRECTORY_ENTRY    Entry;                 // The physical directory entry stored in disk
};

struct _FAT_ODIR {
  UINTN         Signature;
  UINT32        CurrentEndPos;                // Current end position of the directory
  UINT32        CurrentPos;                   // Current position of the directory
  LIST_ENTRY    *CurrentCursor;               // Current directory entry pointer
  LIST_ENTRY    ChildList;                    // List of all directory entries
  BOOLEAN       EndOfDir;                     // Indicate whether we have reached the end of the directory
  LIST_ENTRY    DirCacheLink;                 // Linked in Volume->DirCacheList when discarded
  UINTN         DirCacheTag;                  // The identification of the directory when in directory cache
  FAT_DIRENT    *LongNameHashTable[HASH_TABLE_SIZE];
  FAT_DIRENT    *ShortNameHashTable[HASH_TABLE_SIZE];
};

typedef struct {
  UINTN                Signature;
  EFI_FILE_PROTOCOL    Handle;
  UINT64               Position;
  BOOLEAN              ReadOnly;
  FAT_OFILE            *OFile;
  LIST_ENTRY           Tasks;                 // List of all FAT_TASKs
  LIST_ENTRY           Link;                  // Link to other IFiles
} FAT_IFILE;

typedef struct {
  UINTN                Signature;
  EFI_FILE_IO_TOKEN    *FileIoToken;
  FAT_IFILE            *IFile;
  LIST_ENTRY           Subtasks;              // List of all FAT_SUBTASKs
  LIST_ENTRY           Link;                  // Link to other FAT_TASKs
} FAT_TASK;

typedef struct {
  UINTN                 Signature;
  EFI_DISK_IO2_TOKEN    DiskIo2Token;
  FAT_TASK              *Task;
  BOOLEAN               Write;
  UINT64                Offset;
  VOID                  *Buffer;
  UINTN                 BufferSize;
  LIST_ENTRY            Link;
} FAT_SUBTASK;

//
// FAT_OFILE - Each opened file
//
struct _FAT_OFILE {
  UINTN         Signature;
  FAT_VOLUME    *Volume;
  //
  // A permanent error code to return to all accesses to
  // this opened file
  //
  EFI_STATUS    Error;
  //
  // A list of the IFILE instances for this OFile
  //
  LIST_ENTRY    Opens;

  //
  // The dynamic information
  //
  UINTN         FileSize;
  UINTN         FileCluster;
  UINTN         FileCurrentCluster;
  UINTN         FileLastCluster;

  //
  // Dirty is set if there have been any updates to the
  // file
  // Archive is set if the archive attribute in the file's
  // directory entry needs to be set when performing flush
  // PreserveLastMod is set if the last modification of the
  // file is specified by SetInfo API
  //
  BOOLEAN       Dirty;
  BOOLEAN       IsFixedRootDir;
  BOOLEAN       PreserveLastModification;
  BOOLEAN       Archive;
  //
  // Set by an OFile SetPosition
  //
  UINTN         Position;       // within file
  UINT64        PosDisk;        // on the disk
  UINTN         PosRem;         // remaining in this disk run
  //
  // The opened parent, full path length and currently opened child files
  //
  FAT_OFILE     *Parent;
  UINTN         FullPathLen;
  LIST_ENTRY    ChildHead;
  LIST_ENTRY    ChildLink;

  //
  // The opened directory structure for a directory; if this
  // OFile represents a file, then ODir = NULL
  //
  FAT_ODIR      *ODir;
  //
  // The directory entry for the Ofile
  //
  FAT_DIRENT    *DirEnt;

  //
  // Link in Volume's reference list
  //
  LIST_ENTRY    CheckLink;
};

struct _FAT_VOLUME {
  UINTN                              Signature;

  EFI_HANDLE                         Handle;
  BOOLEAN                            Valid;
  BOOLEAN                            DiskError;

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL    VolumeInterface;

  //
  // If opened, the parent handle and BlockIo interface
  //
  EFI_BLOCK_IO_PROTOCOL              *BlockIo;
  EFI_DISK_IO_PROTOCOL               *DiskIo;
  EFI_DISK_IO2_PROTOCOL              *DiskIo2;
  UINT32                             MediaId;
  BOOLEAN                            ReadOnly;

  //
  // Computed values from fat bpb info
  //
  UINT64                             VolumeSize;
  UINT64                             FatPos;           // Disk pos of fat tables
  UINT64                             RootPos;          // Disk pos of root directory
  UINT64                             FirstClusterPos;  // Disk pos of first cluster
  UINTN                              FatSize;          // Number of bytes in each fat
  UINTN                              MaxCluster;       // Max cluster number
  UINTN                              ClusterSize;      // Cluster size of fat partition
  UINT8                              ClusterAlignment; // Equal to log_2 (clustersize);
  FAT_VOLUME_TYPE                    FatType;

  //
  // Current part of fat table that's present
  //
  UINT64                             FatEntryPos;    // Location of buffer
  UINTN                              FatEntrySize;   // Size of buffer
  UINT32                             FatEntryBuffer; // The buffer
  FAT_INFO_SECTOR                    FatInfoSector;  // Free cluster info
  UINTN                              FreeInfoPos;    // Pos with the free cluster info
  BOOLEAN                            FreeInfoValid;  // If free cluster info is valid
  //
  // Unpacked Fat BPB info
  //
  UINTN                              NumFats;
  UINTN                              RootEntries; // < FAT32, root dir is fixed size
  UINTN                              RootCluster; // >= FAT32, root cluster chain head
  //
  // info for marking the volume dirty or not
  //
  BOOLEAN                            FatDirty;    // If fat-entries have been updated
  UINT32                             DirtyValue;
  UINT32                             NotDirtyValue;

  //
  // The root directory entry and opened root file
  //
  FAT_DIRENT                         RootDirEnt;
  //
  // File Name of root OFile, it is empty string
  //
  CHAR16                             RootFileString[1];
  FAT_OFILE                          *Root;

  //
  // New OFiles are added to this list so they
  // can be cleaned up if they aren't referenced.
  //
  LIST_ENTRY                         CheckRef;

  //
  // Directory cache List
  //
  LIST_ENTRY                         DirCacheList;
  UINTN                              DirCacheCount;

  //
  // Disk Cache for this volume
  //
  VOID                               *CacheBuffer;
  DISK_CACHE                         DiskCache[CacheMaxType];
};

//
// Function Prototypes
//

/**

  Implements Open() of Simple File System Protocol.

  @param  FHand                 - File handle of the file serves as a starting reference point.
  @param  NewHandle             - Handle of the file that is newly opened.
  @param  FileName              - File name relative to FHand.
  @param  OpenMode              - Open mode.
  @param  Attributes            - Attributes to set if the file is created.


  @retval EFI_INVALID_PARAMETER - The FileName is NULL or the file string is empty.
                          The OpenMode is not supported.
                          The Attributes is not the valid attributes.
  @retval EFI_OUT_OF_RESOURCES  - Can not allocate the memory for file string.
  @retval EFI_SUCCESS           - Open the file successfully.
  @return Others                - The status of open file.

**/
EFI_STATUS
EFIAPI
FatOpen (
  IN  EFI_FILE_PROTOCOL  *FHand,
  OUT EFI_FILE_PROTOCOL  **NewHandle,
  IN  CHAR16             *FileName,
  IN  UINT64             OpenMode,
  IN  UINT64             Attributes
  )
;

/**

  Implements OpenEx() of Simple File System Protocol.

  @param  FHand                 - File handle of the file serves as a starting reference point.
  @param  NewHandle             - Handle of the file that is newly opened.
  @param  FileName              - File name relative to FHand.
  @param  OpenMode              - Open mode.
  @param  Attributes            - Attributes to set if the file is created.
  @param  Token                 - A pointer to the token associated with the transaction.

  @retval EFI_INVALID_PARAMETER - The FileName is NULL or the file string is empty.
                          The OpenMode is not supported.
                          The Attributes is not the valid attributes.
  @retval EFI_OUT_OF_RESOURCES  - Can not allocate the memory for file string.
  @retval EFI_SUCCESS           - Open the file successfully.
  @return Others                - The status of open file.

**/
EFI_STATUS
EFIAPI
FatOpenEx (
  IN  EFI_FILE_PROTOCOL     *FHand,
  OUT EFI_FILE_PROTOCOL     **NewHandle,
  IN  CHAR16                *FileName,
  IN  UINT64                OpenMode,
  IN  UINT64                Attributes,
  IN OUT EFI_FILE_IO_TOKEN  *Token
  )
;

/**

  Get the file's position of the file

  @param  FHand                 - The handle of file.
  @param  Position              - The file's position of the file.

  @retval EFI_SUCCESS           - Get the info successfully.
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file.
  @retval EFI_UNSUPPORTED       - The open file is not a file.

**/
EFI_STATUS
EFIAPI
FatGetPosition (
  IN  EFI_FILE_PROTOCOL  *FHand,
  OUT UINT64             *Position
  )
;

/**

  Get the some types info of the file into Buffer

  @param  FHand                 - The handle of file.
  @param  Type                  - The type of the info.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing volume info.

  @retval EFI_SUCCESS           - Get the info successfully.
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file.

**/
EFI_STATUS
EFIAPI
FatGetInfo (
  IN     EFI_FILE_PROTOCOL  *FHand,
  IN     EFI_GUID           *Type,
  IN OUT UINTN              *BufferSize,
  OUT VOID                  *Buffer
  )
;

/**

  Set the some types info of the file into Buffer.

  @param  FHand                 - The handle of file.
  @param  Type                  - The type of the info.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing volume info.

  @retval EFI_SUCCESS           - Set the info successfully.
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file.

**/
EFI_STATUS
EFIAPI
FatSetInfo (
  IN EFI_FILE_PROTOCOL  *FHand,
  IN EFI_GUID           *Type,
  IN UINTN              BufferSize,
  IN VOID               *Buffer
  )
;

/**

  Flushes all data associated with the file handle.

  @param  FHand                 - Handle to file to flush

  @retval EFI_SUCCESS           - Flushed the file successfully
  @retval EFI_WRITE_PROTECTED   - The volume is read only
  @retval EFI_ACCESS_DENIED     - The volume is not read only
                          but the file is read only
  @return Others                - Flushing of the file is failed

**/
EFI_STATUS
EFIAPI
FatFlush (
  IN EFI_FILE_PROTOCOL  *FHand
  )
;

/**

  Flushes all data associated with the file handle.

  @param  FHand                 - Handle to file to flush.
  @param  Token                 - A pointer to the token associated with the transaction.

  @retval EFI_SUCCESS           - Flushed the file successfully.
  @retval EFI_WRITE_PROTECTED   - The volume is read only.
  @retval EFI_ACCESS_DENIED     - The file is read only.
  @return Others                - Flushing of the file failed.

**/
EFI_STATUS
EFIAPI
FatFlushEx (
  IN EFI_FILE_PROTOCOL  *FHand,
  IN EFI_FILE_IO_TOKEN  *Token
  )
;

/**

  Flushes & Closes the file handle.

  @param  FHand                 - Handle to the file to delete.

  @retval EFI_SUCCESS           - Closed the file successfully.

**/
EFI_STATUS
EFIAPI
FatClose (
  IN EFI_FILE_PROTOCOL  *FHand
  )
;

/**

  Deletes the file & Closes the file handle.

  @param  FHand                    - Handle to the file to delete.

  @retval EFI_SUCCESS              - Delete the file successfully.
  @retval EFI_WARN_DELETE_FAILURE  - Fail to delete the file.

**/
EFI_STATUS
EFIAPI
FatDelete (
  IN EFI_FILE_PROTOCOL  *FHand
  )
;

/**

  Set the file's position of the file.

  @param  FHand                 - The handle of file
  @param  Position              - The file's position of the file

  @retval EFI_SUCCESS           - Set the info successfully
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file
  @retval EFI_UNSUPPORTED       - Set a directory with a not-zero position

**/
EFI_STATUS
EFIAPI
FatSetPosition (
  IN EFI_FILE_PROTOCOL  *FHand,
  IN UINT64             Position
  )
;

/**

  Get the file info.

  @param FHand                 - The handle of the file.
  @param BufferSize            - Size of Buffer.
  @param Buffer                - Buffer containing read data.

  @retval EFI_SUCCESS           - Get the file info successfully.
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file.
  @retval EFI_VOLUME_CORRUPTED  - The file type of open file is error.
  @return other                 - An error occurred when operation the disk.

**/
EFI_STATUS
EFIAPI
FatRead (
  IN     EFI_FILE_PROTOCOL  *FHand,
  IN OUT UINTN              *BufferSize,
  OUT VOID                  *Buffer
  )
;

/**

  Get the file info.

  @param FHand                 - The handle of the file.
  @param Token                 - A pointer to the token associated with the transaction.

  @retval EFI_SUCCESS           - Get the file info successfully.
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file.
  @retval EFI_VOLUME_CORRUPTED  - The file type of open file is error.
  @return other                 - An error occurred when operation the disk.

**/
EFI_STATUS
EFIAPI
FatReadEx (
  IN     EFI_FILE_PROTOCOL  *FHand,
  IN OUT EFI_FILE_IO_TOKEN  *Token
  )
;

/**

  Set the file info.

  @param  FHand                 - The handle of the file.
  @param  BufferSize            - Size of Buffer.
  @param Buffer                - Buffer containing write data.

  @retval EFI_SUCCESS           - Set the file info successfully.
  @retval EFI_WRITE_PROTECTED   - The disk is write protected.
  @retval EFI_ACCESS_DENIED     - The file is read-only.
  @retval EFI_DEVICE_ERROR      - The OFile is not valid.
  @retval EFI_UNSUPPORTED       - The open file is not a file.
                        - The writing file size is larger than 4GB.
  @return other                 - An error occurred when operation the disk.

**/
EFI_STATUS
EFIAPI
FatWrite (
  IN     EFI_FILE_PROTOCOL  *FHand,
  IN OUT UINTN              *BufferSize,
  IN     VOID               *Buffer
  )
;

/**

  Get the file info.

  @param  FHand                 - The handle of the file.
  @param  Token                 - A pointer to the token associated with the transaction.

  @retval EFI_SUCCESS           - Get the file info successfully.
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file.
  @retval EFI_VOLUME_CORRUPTED  - The file type of open file is error.
  @return other                 - An error occurred when operation the disk.

**/
EFI_STATUS
EFIAPI
FatWriteEx (
  IN     EFI_FILE_PROTOCOL  *FHand,
  IN OUT EFI_FILE_IO_TOKEN  *Token
  )
;

//
// DiskCache.c
//

/**

  Initialize the disk cache according to Volume's FatType.

  @param  Volume                - FAT file system volume.

  @retval EFI_SUCCESS           - The disk cache is successfully initialized.
  @retval EFI_OUT_OF_RESOURCES  - Not enough memory to allocate disk cache.

**/
EFI_STATUS
FatInitializeDiskCache (
  IN FAT_VOLUME  *Volume
  );

/**

  Read BufferSize bytes from the position of Offset into Buffer,
  or write BufferSize bytes from Buffer into the position of Offset.

  Base on the parameter of CACHE_DATA_TYPE, the data access will be divided into
  the access of FAT cache (CACHE_FAT) and the access of Data cache (CACHE_DATA):

  1. Access of FAT cache (CACHE_FAT): Access the data in the FAT cache, if there is cache
     page hit, just return the cache page; else update the related cache page and return
     the right cache page.
  2. Access of Data cache (CACHE_DATA):
     The access data will be divided into UnderRun data, Aligned data and OverRun data;
     The UnderRun data and OverRun data will be accessed by the Data cache,
     but the Aligned data will be accessed with disk directly.

  @param  Volume                - FAT file system volume.
  @param  CacheDataType         - The type of cache: CACHE_DATA or CACHE_FAT.
  @param  IoMode                - Indicate the type of disk access.
  @param  Offset                - The starting byte offset to read from.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing cache data.
  @param  Task                    point to task instance.

  @retval EFI_SUCCESS           - The data was accessed correctly.
  @retval EFI_MEDIA_CHANGED     - The MediaId does not match the current device.
  @return Others                - An error occurred when accessing cache.

**/
EFI_STATUS
FatAccessCache (
  IN     FAT_VOLUME       *Volume,
  IN     CACHE_DATA_TYPE  CacheDataType,
  IN     IO_MODE          IoMode,
  IN     UINT64           Offset,
  IN     UINTN            BufferSize,
  IN OUT UINT8            *Buffer,
  IN     FAT_TASK         *Task
  );

/**

  Flush all the dirty cache back, include the FAT cache and the Data cache.

  @param  Volume                - FAT file system volume.
  @param  Task                    point to task instance.

  @retval EFI_SUCCESS           - Flush all the dirty cache back successfully
  @return other                 - An error occurred when writing the data into the disk

**/
EFI_STATUS
FatVolumeFlushCache (
  IN FAT_VOLUME  *Volume,
  IN FAT_TASK    *Task
  );

//
// Flush.c
//

/**

  Flush the data associated with an open file.
  In this implementation, only last Mod/Access time is updated.

  @param  OFile                 - The open file.

  @retval EFI_SUCCESS           - The OFile is flushed successfully.
  @return Others                - An error occurred when flushing this OFile.

**/
EFI_STATUS
FatOFileFlush (
  IN FAT_OFILE  *OFile
  );

/**

  Check the references of the OFile.
  If the OFile (that is checked) is no longer
  referenced, then it is freed.

  @param  OFile                 - The OFile to be checked.

  @retval TRUE                  - The OFile is not referenced and freed.
  @retval FALSE                 - The OFile is kept.

**/
BOOLEAN
FatCheckOFileRef (
  IN FAT_OFILE  *OFile
  );

/**

  Set the OFile and its child OFile with the error Status

  @param  OFile                 - The OFile whose permanent error code is to be set.
  @param  Status                - Error code to be set.

**/
VOID
FatSetVolumeError (
  IN FAT_OFILE   *OFile,
  IN EFI_STATUS  Status
  );

/**

  Close the open file instance.

  @param  IFile                 - Open file instance.

  @retval EFI_SUCCESS           - Closed the file successfully.

**/
EFI_STATUS
FatIFileClose (
  FAT_IFILE  *IFile
  );

/**

  Set error status for a specific OFile, reference checking the volume.
  If volume is already marked as invalid, and all resources are freed
  after reference checking, the file system protocol is uninstalled and
  the volume structure is freed.

  @param  Volume                - the Volume that is to be reference checked and unlocked.
  @param  OFile                 - the OFile whose permanent error code is to be set.
  @param  EfiStatus             - error code to be set.
  @param  Task                    point to task instance.

  @retval EFI_SUCCESS           - Clean up the volume successfully.
  @return Others                - Cleaning up of the volume is failed.

**/
EFI_STATUS
FatCleanupVolume (
  IN FAT_VOLUME  *Volume,
  IN FAT_OFILE   *OFile,
  IN EFI_STATUS  EfiStatus,
  IN FAT_TASK    *Task
  );

//
// FileSpace.c
//

/**

  Shrink the end of the open file base on the file size.

  @param  OFile                 - The open file.

  @retval EFI_SUCCESS           - Shrinked successfully.
  @retval EFI_VOLUME_CORRUPTED  - There are errors in the file's clusters.

**/
EFI_STATUS
FatShrinkEof (
  IN FAT_OFILE  *OFile
  );

/**

  Grow the end of the open file base on the NewSizeInBytes.

  @param  OFile                 - The open file.
  @param  NewSizeInBytes        - The new size in bytes of the open file.

  @retval EFI_SUCCESS           - The file is grown successfully.
  @retval EFI_UNSUPPORTED       - The file size is larger than 4GB.
  @retval EFI_VOLUME_CORRUPTED  - There are errors in the files' clusters.
  @retval EFI_VOLUME_FULL       - The volume is full and can not grow the file.

**/
EFI_STATUS
FatGrowEof (
  IN FAT_OFILE  *OFile,
  IN UINT64     NewSizeInBytes
  );

/**

  Get the size of directory of the open file.

  @param  Volume                - The File System Volume.
  @param  Cluster               - The Starting cluster.

  @return The physical size of the file starting at the input cluster, if there is error in the
  cluster chain, the return value is 0.

**/
UINTN
FatPhysicalDirSize (
  IN FAT_VOLUME  *Volume,
  IN UINTN       Cluster
  );

/**

  Get the physical size of a file on the disk.

  @param  Volume                - The file system volume.
  @param  RealSize              - The real size of a file.

  @return The physical size of a file on the disk.

**/
UINT64
FatPhysicalFileSize (
  IN FAT_VOLUME  *Volume,
  IN UINTN       RealSize
  );

/**

  Seek OFile to requested position, and calculate the number of
  consecutive clusters from the position in the file

  @param  OFile                 - The open file.
  @param  Position              - The file's position which will be accessed.
  @param  PosLimit              - The maximum length current reading/writing may access

  @retval EFI_SUCCESS           - Set the info successfully.
  @retval EFI_VOLUME_CORRUPTED  - Cluster chain corrupt.

**/
EFI_STATUS
FatOFilePosition (
  IN FAT_OFILE  *OFile,
  IN UINTN      Position,
  IN UINTN      PosLimit
  );

/**

  Update the free cluster info of FatInfoSector of the volume.

  @param  Volume                - FAT file system volume.

**/
VOID
FatComputeFreeInfo (
  IN FAT_VOLUME  *Volume
  );

//
// Init.c
//

/**

  Allocates volume structure, detects FAT file system, installs protocol,
  and initialize cache.

  @param  Handle                - The handle of parent device.
  @param  DiskIo                - The DiskIo of parent device.
  @param  DiskIo2               - The DiskIo2 of parent device.
  @param  BlockIo               - The BlockIo of parent device.

  @retval EFI_SUCCESS           - Allocate a new volume successfully.
  @retval EFI_OUT_OF_RESOURCES  - Can not allocate the memory.
  @return Others                - Allocating a new volume failed.

**/
EFI_STATUS
FatAllocateVolume (
  IN  EFI_HANDLE             Handle,
  IN  EFI_DISK_IO_PROTOCOL   *DiskIo,
  IN  EFI_DISK_IO2_PROTOCOL  *DiskIo2,
  IN  EFI_BLOCK_IO_PROTOCOL  *BlockIo
  );

/**

  Detects FAT file system on Disk and set relevant fields of Volume.

  @param Volume                - The volume structure.

  @retval EFI_SUCCESS           - The Fat File System is detected successfully
  @retval EFI_UNSUPPORTED       - The volume is not FAT file system.
  @retval EFI_VOLUME_CORRUPTED  - The volume is corrupted.

**/
EFI_STATUS
FatOpenDevice (
  IN OUT FAT_VOLUME  *Volume
  );

/**

  Called by FatDriverBindingStop(), Abandon the volume.

  @param  Volume                - The volume to be abandoned.

  @retval EFI_SUCCESS           - Abandoned the volume successfully.
  @return Others                - Can not uninstall the protocol interfaces.

**/
EFI_STATUS
FatAbandonVolume (
  IN FAT_VOLUME  *Volume
  );

//
// Misc.c
//

/**

  Create the task

  @param  IFile                 - The instance of the open file.
  @param  Token                 - A pointer to the token associated with the transaction.

  @return FAT_TASK *            - Return the task instance.

**/
FAT_TASK *
FatCreateTask (
  FAT_IFILE          *IFile,
  EFI_FILE_IO_TOKEN  *Token
  );

/**

  Destroy the task.

  @param  Task                  - The task to be destroyed.

**/
VOID
FatDestroyTask (
  FAT_TASK  *Task
  );

/**

  Wait all non-blocking requests complete.

  @param  IFile                 - The instance of the open file.

**/
VOID
FatWaitNonblockingTask (
  FAT_IFILE  *IFile
  );

/**

  Remove the subtask from subtask list.

  @param  Subtask               - The subtask to be removed.

  @return LIST_ENTRY *          - The next node in the list.

**/
LIST_ENTRY *
FatDestroySubtask (
  FAT_SUBTASK  *Subtask
  );

/**

  Execute the task.

  @param  IFile                 - The instance of the open file.
  @param  Task                  - The task to be executed.

  @retval EFI_SUCCESS           - The task was executed successfully.
  @return other                 - An error occurred when executing the task.

**/
EFI_STATUS
FatQueueTask (
  IN FAT_IFILE  *IFile,
  IN FAT_TASK   *Task
  );

/**

  Set the volume as dirty or not.

  @param  Volume                - FAT file system volume.
  @param  IoMode                - The access mode.
  @param  DirtyValue            - Set the volume as dirty or not.

  @retval EFI_SUCCESS           - Set the new FAT entry value successfully.
  @return other                 - An error occurred when operation the FAT entries.

**/
EFI_STATUS
FatAccessVolumeDirty (
  IN FAT_VOLUME  *Volume,
  IN IO_MODE     IoMode,
  IN VOID        *DirtyValue
  );

/**

  General disk access function.

  @param  Volume                - FAT file system volume.
  @param  IoMode                - The access mode (disk read/write or cache access).
  @param  Offset                - The starting byte offset to read from.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing read data.
  @param  Task                    point to task instance.

  @retval EFI_SUCCESS           - The operation is performed successfully.
  @retval EFI_VOLUME_CORRUPTED  - The access is
  @return Others                - The status of read/write the disk

**/
EFI_STATUS
FatDiskIo (
  IN FAT_VOLUME  *Volume,
  IN IO_MODE     IoMode,
  IN UINT64      Offset,
  IN UINTN       BufferSize,
  IN OUT VOID    *Buffer,
  IN FAT_TASK    *Task
  );

/**

  Lock the volume.

**/
VOID
FatAcquireLock (
  VOID
  );

/**

  Unlock the volume.

**/
VOID
FatReleaseLock (
  VOID
  );

/**

  Lock the volume.
  If the lock is already in the acquired state, then EFI_ACCESS_DENIED is returned.
  Otherwise, EFI_SUCCESS is returned.

  @retval EFI_SUCCESS           - The volume is locked.
  @retval EFI_ACCESS_DENIED     - The volume could not be locked because it is already locked.

**/
EFI_STATUS
FatAcquireLockOrFail (
  VOID
  );

/**

  Free directory entry.

  @param  DirEnt                - The directory entry to be freed.

**/
VOID
FatFreeDirEnt (
  IN FAT_DIRENT  *DirEnt
  );

/**

  Free volume structure (including the contents of directory cache and disk cache).

  @param  Volume                - The volume structure to be freed.

**/
VOID
FatFreeVolume (
  IN FAT_VOLUME  *Volume
  );

/**

  Translate EFI time to FAT time.

  @param  ETime                 - The time of EFI_TIME.
  @param  FTime                 - The time of FAT_DATE_TIME.

**/
VOID
FatEfiTimeToFatTime (
  IN EFI_TIME        *ETime,
  OUT FAT_DATE_TIME  *FTime
  );

/**

  Translate Fat time to EFI time.

  @param  FTime                 - The time of FAT_DATE_TIME.
  @param  ETime                 - The time of EFI_TIME..

**/
VOID
FatFatTimeToEfiTime (
  IN FAT_DATE_TIME  *FTime,
  OUT EFI_TIME      *ETime
  );

/**

  Get Current FAT time.

  @param  FatTime               - Current FAT time.

**/
VOID
FatGetCurrentFatTime (
  OUT FAT_DATE_TIME  *FatTime
  );

/**

  Check whether a time is valid.

  @param  Time                  - The time of EFI_TIME.

  @retval TRUE                  - The time is valid.
  @retval FALSE                 - The time is not valid.

**/
BOOLEAN
FatIsValidTime (
  IN EFI_TIME  *Time
  );

//
// UnicodeCollation.c
//

/**
  Initialize Unicode Collation support.

  It tries to locate Unicode Collation 2 protocol and matches it with current
  platform language code. If for any reason the first attempt fails, it then tries to
  use Unicode Collation Protocol.

  @param  AgentHandle          The handle used to open Unicode Collation (2) protocol.

  @retval EFI_SUCCESS          The Unicode Collation (2) protocol has been successfully located.
  @retval Others               The Unicode Collation (2) protocol has not been located.

**/
EFI_STATUS
InitializeUnicodeCollationSupport (
  IN EFI_HANDLE  AgentHandle
  );

/**
  Convert FAT string to unicode string.

  @param  FatSize               The size of FAT string.
  @param  Fat                   The FAT string.
  @param  String                The unicode string.

  @return None.

**/
VOID
FatFatToStr (
  IN UINTN    FatSize,
  IN CHAR8    *Fat,
  OUT CHAR16  *String
  );

/**
  Convert unicode string to Fat string.

  @param  String                The unicode string.
  @param  FatSize               The size of the FAT string.
  @param  Fat                   The FAT string.

  @retval TRUE                  Convert successfully.
  @retval FALSE                 Convert error.

**/
BOOLEAN
FatStrToFat (
  IN  CHAR16  *String,
  IN  UINTN   FatSize,
  OUT CHAR8   *Fat
  );

/**
  Lowercase a string

  @param  Str                   The string which will be lower-cased.

**/
VOID
FatStrLwr (
  IN CHAR16  *Str
  );

/**
  Uppercase a string.

  @param  Str                   The string which will be upper-cased.

**/
VOID
FatStrUpr (
  IN CHAR16  *Str
  );

/**
  Performs a case-insensitive comparison of two Null-terminated Unicode strings.

  @param  Str1                   A pointer to a Null-terminated Unicode string.
  @param  Str2                   A pointer to a Null-terminated Unicode string.

  @retval 0                    S1 is equivalent to S2.
  @retval >0                   S1 is lexically greater than S2.
  @retval <0                   S1 is lexically less than S2.
**/
INTN
FatStriCmp (
  IN CHAR16  *Str1,
  IN CHAR16  *Str2
  );

//
// Open.c
//

/**

  Open a file for a file name relative to an existing OFile.
  The IFile of the newly opened file is passed out.

  @param  OFile                 - The file that serves as a starting reference point.
  @param  NewIFile              - The newly generated IFile instance.
  @param  FileName              - The file name relative to the OFile.
  @param  OpenMode              - Open mode.
  @param  Attributes            - Attributes to set if the file is created.


  @retval EFI_SUCCESS           - Open the file successfully.
  @retval EFI_INVALID_PARAMETER - The open mode is conflict with the attributes
                          or the file name is not valid.
  @retval EFI_NOT_FOUND         - Conflicts between dir intention and attribute.
  @retval EFI_WRITE_PROTECTED   - Can't open for write if the volume is read only.
  @retval EFI_ACCESS_DENIED     - If the file's attribute is read only, and the
                          open is for read-write fail it.
  @retval EFI_OUT_OF_RESOURCES  - Can not allocate the memory.

**/
EFI_STATUS
FatOFileOpen (
  IN FAT_OFILE   *OFile,
  OUT FAT_IFILE  **NewIFile,
  IN CHAR16      *FileName,
  IN UINT64      OpenMode,
  IN UINT8       Attributes
  );

/**

  Create an Open instance for the existing OFile.
  The IFile of the newly opened file is passed out.

  @param  OFile                 - The file that serves as a starting reference point.
  @param  PtrIFile              - The newly generated IFile instance.

  @retval EFI_OUT_OF_RESOURCES  - Can not allocate the memory for the IFile
  @retval EFI_SUCCESS           - Create the new IFile for the OFile successfully

**/
EFI_STATUS
FatAllocateIFile (
  IN FAT_OFILE   *OFile,
  OUT FAT_IFILE  **PtrIFile
  );

//
// OpenVolume.c
//

/**

  Implements Simple File System Protocol interface function OpenVolume().

  @param  This                  - Calling context.
  @param  File                  - the Root Directory of the volume.

  @retval EFI_OUT_OF_RESOURCES  - Can not allocate the memory.
  @retval EFI_VOLUME_CORRUPTED  - The FAT type is error.
  @retval EFI_SUCCESS           - Open the volume successfully.

**/
EFI_STATUS
EFIAPI
FatOpenVolume (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL                **File
  );

//
// ReadWrite.c
//

/**

  This function reads data from a file or writes data to a file.
  It uses OFile->PosRem to determine how much data can be accessed in one time.

  @param  OFile                 - The open file.
  @param  IoMode                - Indicate whether the access mode is reading or writing.
  @param  Position              - The position where data will be accessed.
  @param  DataBufferSize        - Size of Buffer.
  @param  UserBuffer            - Buffer containing data.
  @param  Task                    point to task instance.

  @retval EFI_SUCCESS           - Access the data successfully.
  @return other                 - An error occurred when operating on the disk.

**/
EFI_STATUS
FatAccessOFile (
  IN FAT_OFILE  *OFile,
  IN IO_MODE    IoMode,
  IN UINTN      Position,
  IN UINTN      *DataBufferSize,
  IN UINT8      *UserBuffer,
  IN FAT_TASK   *Task
  );

/**

  Expand OFile by appending zero bytes at the end of OFile.

  @param  OFile                 - The open file.
  @param  ExpandedSize          - The number of zero bytes appended at the end of the file.

  @retval EFI_SUCCESS           - The file is expanded successfully.
  @return other                 - An error occurred when expanding file.

**/
EFI_STATUS
FatExpandOFile (
  IN FAT_OFILE  *OFile,
  IN UINT64     ExpandedSize
  );

/**

  Write zero pool from the WritePos to the end of OFile.

  @param  OFile                 - The open file to write zero pool.
  @param  WritePos              - The number of zero bytes written.

  @retval EFI_SUCCESS           - Write the zero pool successfully.
  @retval EFI_OUT_OF_RESOURCES  - Not enough memory to perform the operation.
  @return other                 - An error occurred when writing disk.

**/
EFI_STATUS
FatWriteZeroPool (
  IN FAT_OFILE  *OFile,
  IN UINTN      WritePos
  );

/**

  Truncate the OFile to smaller file size.

  @param  OFile                 - The open file.
  @param  TruncatedSize         - The new file size.

  @retval EFI_SUCCESS           - The file is truncated successfully.
  @return other                 - An error occurred when truncating file.

**/
EFI_STATUS
FatTruncateOFile (
  IN FAT_OFILE  *OFile,
  IN UINTN      TruncatedSize
  );

//
// DirectoryManage.c
//

/**

  Set the OFile's current directory cursor to the list head.

  @param OFile                 - The directory OFile whose directory cursor is reset.

**/
VOID
FatResetODirCursor (
  IN FAT_OFILE  *OFile
  );

/**

  Set the directory's cursor to the next and get the next directory entry.

  @param  OFile                 - The parent OFile.
  @param PtrDirEnt             - The next directory entry.

  @retval EFI_SUCCESS           - We get the next directory entry successfully.
  @return other                 - An error occurred when get next directory entry.

**/
EFI_STATUS
FatGetNextDirEnt (
  IN  FAT_OFILE   *OFile,
  OUT FAT_DIRENT  **PtrDirEnt
  );

/**

  Remove this directory entry node from the list of directory entries and hash table.

  @param  OFile                - The parent OFile.
  @param  DirEnt               - The directory entry to be removed.

  @retval EFI_SUCCESS          - The directory entry is successfully removed.
  @return other                - An error occurred when removing the directory entry.

**/
EFI_STATUS
FatRemoveDirEnt (
  IN FAT_OFILE   *OFile,
  IN FAT_DIRENT  *DirEnt
  );

/**

  Save the directory entry to disk.

  @param  OFile                 - The parent OFile which needs to update.
  @param  DirEnt                - The directory entry to be saved.

  @retval EFI_SUCCESS           - Store the directory entry successfully.
  @return other                 - An error occurred when writing the directory entry.

**/
EFI_STATUS
FatStoreDirEnt (
  IN FAT_OFILE   *OFile,
  IN FAT_DIRENT  *DirEnt
  );

/**

  Create a directory entry in the parent OFile.

  @param  OFile                 - The parent OFile.
  @param  FileName              - The filename of the newly-created directory entry.
  @param  Attributes            - The attribute of the newly-created directory entry.
  @param  PtrDirEnt             - The pointer to the newly-created directory entry.

  @retval EFI_SUCCESS           - The directory entry is successfully created.
  @retval EFI_OUT_OF_RESOURCES  - Not enough memory to create the directory entry.
  @return other                 - An error occurred when creating the directory entry.

**/
EFI_STATUS
FatCreateDirEnt (
  IN  FAT_OFILE   *OFile,
  IN  CHAR16      *FileName,
  IN  UINT8       Attributes,
  OUT FAT_DIRENT  **PtrDirEnt
  );

/**

  Determine whether the directory entry is "." or ".." entry.

  @param  DirEnt               - The corresponding directory entry.

  @retval TRUE                 - The directory entry is "." or ".." directory entry
  @retval FALSE                - The directory entry is not "." or ".." directory entry

**/
BOOLEAN
FatIsDotDirEnt (
  IN FAT_DIRENT  *DirEnt
  );

/**

  Set the OFile's cluster and size info in its directory entry.

  @param  OFile                 - The corresponding OFile.

**/
VOID
FatUpdateDirEntClusterSizeInfo (
  IN FAT_OFILE  *OFile
  );

/**

  Copy all the information of DirEnt2 to DirEnt1 except for 8.3 name.

  @param  DirEnt1               - The destination directory entry.
  @param  DirEnt2               - The source directory entry.

**/
VOID
FatCloneDirEnt (
  IN  FAT_DIRENT  *DirEnt1,
  IN  FAT_DIRENT  *DirEnt2
  );

/**

  Get the directory entry's info into Buffer.

  @param  Volume                - FAT file system volume.
  @param  DirEnt                - The corresponding directory entry.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing file info.

  @retval EFI_SUCCESS           - Get the file info successfully.
  @retval EFI_BUFFER_TOO_SMALL  - The buffer is too small.

**/
EFI_STATUS
FatGetDirEntInfo (
  IN FAT_VOLUME  *Volume,
  IN FAT_DIRENT  *DirEnt,
  IN OUT UINTN   *BufferSize,
  OUT VOID       *Buffer
  );

/**

  Open the directory entry to get the OFile.

  @param  Parent                - The parent OFile.
  @param  DirEnt                - The directory entry to be opened.

  @retval EFI_SUCCESS           - The directory entry is successfully opened.
  @retval EFI_OUT_OF_RESOURCES  - not enough memory to allocate a new OFile.
  @return other                 - An error occurred when opening the directory entry.

**/
EFI_STATUS
FatOpenDirEnt (
  IN FAT_OFILE   *OFile,
  IN FAT_DIRENT  *DirEnt
  );

/**

  Create "." and ".." directory entries in the newly-created parent OFile.

  @param  OFile                 - The parent OFile.

  @retval EFI_SUCCESS           - The dot directory entries are successfully created.
  @return other                 - An error occurred when creating the directory entry.

**/
EFI_STATUS
FatCreateDotDirEnts (
  IN FAT_OFILE  *OFile
  );

/**

  Close the directory entry and free the OFile.

  @param  DirEnt               - The directory entry to be closed.

**/
VOID
FatCloseDirEnt (
  IN FAT_DIRENT  *DirEnt
  );

/**

  Traverse filename and open all OFiles that can be opened.
  Update filename pointer to the component that can't be opened.
  If more than one name component remains, returns an error;
  otherwise, return the remaining name component so that the caller might choose to create it.

  @param  PtrOFile              - As input, the reference OFile; as output, the located OFile.
  @param  FileName              - The file name relevant to the OFile.
  @param  Attributes            - The attribute of the destination OFile.
  @param  NewFileName           - The remaining file name.

  @retval EFI_NOT_FOUND         - The file name can't be opened and there is more than one
                          components within the name left (this means the name can
                          not be created either).
  @retval EFI_INVALID_PARAMETER - The parameter is not valid.
  @retval EFI_SUCCESS           - Open the file successfully.
  @return other                 - An error occurred when locating the OFile.

**/
EFI_STATUS
FatLocateOFile (
  IN OUT FAT_OFILE  **PtrOFile,
  IN     CHAR16     *FileName,
  IN     UINT8      Attributes,
  OUT CHAR16        *NewFileName
  );

/**

  Get the directory entry for the volume.

  @param  Volume                - FAT file system volume.
  @param  Name                  - The file name of the volume.

  @retval EFI_SUCCESS           - Update the volume with the directory entry successfully.
  @return others                - An error occurred when getting volume label.

**/
EFI_STATUS
FatGetVolumeEntry (
  IN FAT_VOLUME  *Volume,
  IN CHAR16      *Name
  );

/**

  Set the relevant directory entry into disk for the volume.

  @param  Volume              - FAT file system volume.
  @param  Name                - The new file name of the volume.

  @retval EFI_SUCCESS         - Update the Volume successfully.
  @retval EFI_UNSUPPORTED     - The input label is not a valid volume label.
  @return other               - An error occurred when setting volume label.

**/
EFI_STATUS
FatSetVolumeEntry (
  IN FAT_VOLUME  *Volume,
  IN CHAR16      *Name
  );

//
// Hash.c
//

/**

  Search the long name hash table for the directory entry.

  @param  ODir                  - The directory to be searched.
  @param  LongNameString        - The long name string to search.

  @return The previous long name hash node of the directory entry.

**/
FAT_DIRENT **
FatLongNameHashSearch (
  IN FAT_ODIR  *ODir,
  IN CHAR16    *LongNameString
  );

/**

  Search the short name hash table for the directory entry.

  @param  ODir                  - The directory to be searched.
  @param  ShortNameString       - The short name string to search.

  @return The previous short name hash node of the directory entry.

**/
FAT_DIRENT **
FatShortNameHashSearch (
  IN FAT_ODIR  *ODir,
  IN CHAR8     *ShortNameString
  );

/**

  Insert directory entry to hash table.

  @param  ODir                  - The parent directory.
  @param  DirEnt                - The directory entry node.

**/
VOID
FatInsertToHashTable (
  IN FAT_ODIR    *ODir,
  IN FAT_DIRENT  *DirEnt
  );

/**

  Delete directory entry from hash table.

  @param  ODir                  - The parent directory.
  @param  DirEnt                - The directory entry node.

**/
VOID
FatDeleteFromHashTable (
  IN FAT_ODIR    *ODir,
  IN FAT_DIRENT  *DirEnt
  );

//
// FileName.c
//

/**

  This function checks whether the input FileName is a valid 8.3 short name.
  If the input FileName is a valid 8.3, the output is the 8.3 short name;
  otherwise, the output is the base tag of 8.3 short name.

  @param  FileName              - The input unicode filename.
  @param  File8Dot3Name         - The output ascii 8.3 short name or base tag of 8.3 short name.

  @retval TRUE                  - The input unicode filename is a valid 8.3 short name.
  @retval FALSE                 - The input unicode filename is not a valid 8.3 short name.

**/
BOOLEAN
FatCheckIs8Dot3Name (
  IN CHAR16  *FileName,
  OUT CHAR8  *File8Dot3Name
  );

/**

  This function generates 8Dot3 name from user specified name for a newly created file.

  @param  Parent                - The parent directory.
  @param  DirEnt                - The directory entry whose 8Dot3Name needs to be generated.

**/
VOID
FatCreate8Dot3Name (
  IN FAT_OFILE   *Parent,
  IN FAT_DIRENT  *DirEnt
  );

/**

  Convert the ascii fat name to the unicode string and strip trailing spaces,
  and if necessary, convert the unicode string to lower case.

  @param  FatName               - The Char8 string needs to be converted.
  @param  Len                   - The length of the fat name.
  @param  LowerCase             - Indicate whether to convert the string to lower case.
  @param  Str                   - The result of the conversion.

**/
VOID
FatNameToStr (
  IN CHAR8   *FatName,
  IN UINTN   Len,
  IN UINTN   LowerCase,
  IN CHAR16  *Str
  );

/**

  Set the caseflag value for the directory entry.

  @param DirEnt                - The logical directory entry whose caseflag value is to be set.

**/
VOID
FatSetCaseFlag (
  IN FAT_DIRENT  *DirEnt
  );

/**

  Convert the 8.3 ASCII fat name to cased Unicode string according to case flag.

  @param  DirEnt                - The corresponding directory entry.
  @param  FileString            - The output Unicode file name.
  @param  FileStringMax           The max length of FileString.

**/
VOID
FatGetFileNameViaCaseFlag (
  IN     FAT_DIRENT  *DirEnt,
  IN OUT CHAR16      *FileString,
  IN     UINTN       FileStringMax
  );

/**

  Get the Check sum for a short name.

  @param  ShortNameString       - The short name for a file.

  @retval Sum                   - UINT8 checksum.

**/
UINT8
FatCheckSum (
  IN CHAR8  *ShortNameString
  );

/**

  Takes Path as input, returns the next name component
  in Name, and returns the position after Name (e.g., the
  start of the next name component)

  @param  Path                  - The path of one file.
  @param  Name                  - The next name component in Path.

  The position after Name in the Path

**/
CHAR16 *
FatGetNextNameComponent (
  IN  CHAR16  *Path,
  OUT CHAR16  *Name
  );

/**

  Check whether the IFileName is valid long file name. If the IFileName is a valid
  long file name, then we trim the possible leading blanks and leading/trailing dots.
  the trimmed filename is stored in OutputFileName

  @param  InputFileName         - The input file name.
  @param  OutputFileName        - The output file name.

  @retval TRUE                  - The InputFileName is a valid long file name.
  @retval FALSE                 - The InputFileName is not a valid long file name.

**/
BOOLEAN
FatFileNameIsValid (
  IN  CHAR16  *InputFileName,
  OUT CHAR16  *OutputFileName
  );

//
// DirectoryCache.c
//

/**

  Discard the directory structure when an OFile will be freed.
  Volume will cache this directory if the OFile does not represent a deleted file.

  @param  OFile                 - The OFile whose directory structure is to be discarded.

**/
VOID
FatDiscardODir (
  IN FAT_OFILE  *OFile
  );

/**

  Request the directory structure when an OFile is newly generated.
  If the directory structure is cached by volume, then just return this directory;
  Otherwise, allocate a new one for OFile.

  @param  OFile                 - The OFile which requests directory structure.

**/
VOID
FatRequestODir (
  IN FAT_OFILE  *OFile
  );

/**

  Clean up all the cached directory structures when the volume is going to be abandoned.

  @param  Volume                - FAT file system volume.

**/
VOID
FatCleanupODirCache (
  IN FAT_VOLUME  *Volume
  );

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gFatDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gFatComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gFatComponentName2;
extern EFI_LOCK                      FatFsLock;
extern EFI_LOCK                      FatTaskLock;
extern EFI_FILE_PROTOCOL             FatFileInterface;

#endif
