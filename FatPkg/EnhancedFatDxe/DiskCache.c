/** @file
  Cache implementation for EFI FAT File system driver.

Copyright (c) 2005 - 2013, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Fat.h"

/**

  This function is used by the Data Cache.

  When this function is called by write command, all entries in this range
  are older than the contents in disk, so they are invalid; just mark them invalid.

  When this function is called by read command, if any entry in this range
  is dirty, it means that the relative info directly read from media is older than
  than the info in the cache; So need to update the relative info in the Buffer.

  @param  Volume                - FAT file system volume.
  @param  IoMode                - This function is called by read command or write command
  @param  StartPageNo           - First PageNo to be checked in the cache.
  @param  EndPageNo             - Last PageNo to be checked in the cache.
  @param  Buffer                - The user buffer need to update. Only when doing the read command
                          and there is dirty cache in the cache range, this parameter will be used.

**/
STATIC
VOID
FatFlushDataCacheRange (
  IN  FAT_VOLUME  *Volume,
  IN  IO_MODE     IoMode,
  IN  UINTN       StartPageNo,
  IN  UINTN       EndPageNo,
  OUT UINT8       *Buffer
  )
{
  UINTN       PageNo;
  UINTN       GroupNo;
  UINTN       GroupMask;
  UINTN       PageSize;
  UINT8       PageAlignment;
  DISK_CACHE  *DiskCache;
  CACHE_TAG   *CacheTag;
  UINT8       *BaseAddress;

  DiskCache     = &Volume->DiskCache[CacheData];
  BaseAddress   = DiskCache->CacheBase;
  GroupMask     = DiskCache->GroupMask;
  PageAlignment = DiskCache->PageAlignment;
  PageSize      = (UINTN)1 << PageAlignment;

  for (PageNo = StartPageNo; PageNo < EndPageNo; PageNo++) {
    GroupNo  = PageNo & GroupMask;
    CacheTag = &DiskCache->CacheTag[GroupNo];
    if ((CacheTag->RealSize > 0) && (CacheTag->PageNo == PageNo)) {
      //
      // When reading data form disk directly, if some dirty data
      // in cache is in this rang, this data in the Buffer need to
      // be updated with the cache's dirty data.
      //
      if (IoMode == ReadDisk) {
        if (CacheTag->Dirty) {
          CopyMem (
            Buffer + ((PageNo - StartPageNo) << PageAlignment),
            BaseAddress + (GroupNo << PageAlignment),
            PageSize
            );
        }
      } else {
        //
        // Make all valid entries in this range invalid.
        //
        CacheTag->RealSize = 0;
      }
    }
  }
}

/**

  Exchange the cache page with the image on the disk

  @param  Volume                - FAT file system volume.
  @param  DataType              - Indicate the cache type.
  @param  IoMode                - Indicate whether to load this page from disk or store this page to disk.
  @param  CacheTag              - The Cache Tag for the current cache page.
  @param  Task                    point to task instance.

  @retval EFI_SUCCESS           - Cache page exchanged successfully.
  @return Others                - An error occurred when exchanging cache page.

**/
STATIC
EFI_STATUS
FatExchangeCachePage (
  IN FAT_VOLUME       *Volume,
  IN CACHE_DATA_TYPE  DataType,
  IN IO_MODE          IoMode,
  IN CACHE_TAG        *CacheTag,
  IN FAT_TASK         *Task
  )
{
  EFI_STATUS  Status;
  UINTN       GroupNo;
  UINTN       PageNo;
  UINTN       WriteCount;
  UINTN       RealSize;
  UINT64      EntryPos;
  UINT64      MaxSize;
  DISK_CACHE  *DiskCache;
  VOID        *PageAddress;
  UINT8       PageAlignment;

  DiskCache     = &Volume->DiskCache[DataType];
  PageNo        = CacheTag->PageNo;
  GroupNo       = PageNo & DiskCache->GroupMask;
  PageAlignment = DiskCache->PageAlignment;
  PageAddress   = DiskCache->CacheBase + (GroupNo << PageAlignment);
  EntryPos      = DiskCache->BaseAddress + LShiftU64 (PageNo, PageAlignment);
  RealSize      = CacheTag->RealSize;
  if (IoMode == ReadDisk) {
    RealSize = (UINTN)1 << PageAlignment;
    MaxSize  = DiskCache->LimitAddress - EntryPos;
    if (MaxSize < RealSize) {
      DEBUG ((DEBUG_INFO, "FatDiskIo: Cache Page OutBound occurred! \n"));
      RealSize = (UINTN)MaxSize;
    }
  }

  WriteCount = 1;
  if ((DataType == CacheFat) && (IoMode == WriteDisk)) {
    WriteCount = Volume->NumFats;
  }

  do {
    //
    // Only fat table writing will execute more than once
    //
    Status = FatDiskIo (Volume, IoMode, EntryPos, RealSize, PageAddress, Task);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    EntryPos += Volume->FatSize;
  } while (--WriteCount > 0);

  CacheTag->Dirty    = FALSE;
  CacheTag->RealSize = RealSize;
  return EFI_SUCCESS;
}

/**

  Get one cache page by specified PageNo.

  @param  Volume                - FAT file system volume.
  @param  CacheDataType         - The cache type: CACHE_FAT or CACHE_DATA.
  @param  PageNo                - PageNo to match with the cache.
  @param  CacheTag              - The Cache Tag for the current cache page.

  @retval EFI_SUCCESS           - Get the cache page successfully.
  @return other                 - An error occurred when accessing data.

**/
STATIC
EFI_STATUS
FatGetCachePage (
  IN FAT_VOLUME       *Volume,
  IN CACHE_DATA_TYPE  CacheDataType,
  IN UINTN            PageNo,
  IN CACHE_TAG        *CacheTag
  )
{
  EFI_STATUS  Status;
  UINTN       OldPageNo;

  OldPageNo = CacheTag->PageNo;
  if ((CacheTag->RealSize > 0) && (OldPageNo == PageNo)) {
    //
    // Cache Hit occurred
    //
    return EFI_SUCCESS;
  }

  //
  // Write dirty cache page back to disk
  //
  if ((CacheTag->RealSize > 0) && CacheTag->Dirty) {
    Status = FatExchangeCachePage (Volume, CacheDataType, WriteDisk, CacheTag, NULL);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Load new data from disk;
  //
  CacheTag->PageNo = PageNo;
  Status           = FatExchangeCachePage (Volume, CacheDataType, ReadDisk, CacheTag, NULL);

  return Status;
}

/**

  Read Length bytes from the position of Offset into Buffer, or
  write Length bytes from Buffer into the position of Offset.

  @param  Volume                - FAT file system volume.
  @param  CacheDataType         - The type of cache: CACHE_DATA or CACHE_FAT.
  @param  IoMode                - Indicate the type of disk access.
  @param  PageNo                - The number of unaligned cache page.
  @param  Offset                - The starting byte of cache page.
  @param  Length                - The number of bytes that is read or written
  @param  Buffer                - Buffer containing cache data.

  @retval EFI_SUCCESS           - The data was accessed correctly.
  @return Others                - An error occurred when accessing unaligned cache page.

**/
STATIC
EFI_STATUS
FatAccessUnalignedCachePage (
  IN     FAT_VOLUME       *Volume,
  IN     CACHE_DATA_TYPE  CacheDataType,
  IN     IO_MODE          IoMode,
  IN     UINTN            PageNo,
  IN     UINTN            Offset,
  IN     UINTN            Length,
  IN OUT VOID             *Buffer
  )
{
  EFI_STATUS  Status;
  VOID        *Source;
  VOID        *Destination;
  DISK_CACHE  *DiskCache;
  CACHE_TAG   *CacheTag;
  UINTN       GroupNo;

  DiskCache = &Volume->DiskCache[CacheDataType];
  GroupNo   = PageNo & DiskCache->GroupMask;
  CacheTag  = &DiskCache->CacheTag[GroupNo];
  Status    = FatGetCachePage (Volume, CacheDataType, PageNo, CacheTag);
  if (!EFI_ERROR (Status)) {
    Source      = DiskCache->CacheBase + (GroupNo << DiskCache->PageAlignment) + Offset;
    Destination = Buffer;
    if (IoMode != ReadDisk) {
      CacheTag->Dirty  = TRUE;
      DiskCache->Dirty = TRUE;
      Destination      = Source;
      Source           = Buffer;
    }

    CopyMem (Destination, Source, Length);
  }

  return Status;
}

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
  )
{
  EFI_STATUS  Status;
  UINTN       PageSize;
  UINTN       UnderRun;
  UINTN       OverRun;
  UINTN       AlignedSize;
  UINTN       Length;
  UINTN       PageNo;
  UINTN       AlignedPageCount;
  UINTN       OverRunPageNo;
  DISK_CACHE  *DiskCache;
  UINT64      EntryPos;
  UINT8       PageAlignment;

  ASSERT (Volume->CacheBuffer != NULL);

  Status        = EFI_SUCCESS;
  DiskCache     = &Volume->DiskCache[CacheDataType];
  EntryPos      = Offset - DiskCache->BaseAddress;
  PageAlignment = DiskCache->PageAlignment;
  PageSize      = (UINTN)1 << PageAlignment;
  PageNo        = (UINTN)RShiftU64 (EntryPos, PageAlignment);
  UnderRun      = ((UINTN)EntryPos) & (PageSize - 1);

  if (UnderRun > 0) {
    Length = PageSize - UnderRun;
    if (Length > BufferSize) {
      Length = BufferSize;
    }

    Status = FatAccessUnalignedCachePage (Volume, CacheDataType, IoMode, PageNo, UnderRun, Length, Buffer);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Buffer     += Length;
    BufferSize -= Length;
    PageNo++;
  }

  AlignedPageCount = BufferSize >> PageAlignment;
  OverRunPageNo    = PageNo + AlignedPageCount;
  //
  // The access of the Aligned data
  //
  if (AlignedPageCount > 0) {
    //
    // Accessing fat table cannot have alignment data
    //
    ASSERT (CacheDataType == CacheData);

    EntryPos    = Volume->RootPos + LShiftU64 (PageNo, PageAlignment);
    AlignedSize = AlignedPageCount << PageAlignment;
    Status      = FatDiskIo (Volume, IoMode, EntryPos, AlignedSize, Buffer, Task);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // If these access data over laps the relative cache range, these cache pages need
    // to be updated.
    //
    FatFlushDataCacheRange (Volume, IoMode, PageNo, OverRunPageNo, Buffer);
    Buffer     += AlignedSize;
    BufferSize -= AlignedSize;
  }

  //
  // The access of the OverRun data
  //
  OverRun = BufferSize;
  if (OverRun > 0) {
    //
    // Last read is not a complete page
    //
    Status = FatAccessUnalignedCachePage (Volume, CacheDataType, IoMode, OverRunPageNo, 0, OverRun, Buffer);
  }

  return Status;
}

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
  )
{
  EFI_STATUS       Status;
  CACHE_DATA_TYPE  CacheDataType;
  UINTN            GroupIndex;
  UINTN            GroupMask;
  DISK_CACHE       *DiskCache;
  CACHE_TAG        *CacheTag;

  for (CacheDataType = (CACHE_DATA_TYPE)0; CacheDataType < CacheMaxType; CacheDataType++) {
    DiskCache = &Volume->DiskCache[CacheDataType];
    if (DiskCache->Dirty) {
      //
      // Data cache or fat cache is dirty, write the dirty data back
      //
      GroupMask = DiskCache->GroupMask;
      for (GroupIndex = 0; GroupIndex <= GroupMask; GroupIndex++) {
        CacheTag = &DiskCache->CacheTag[GroupIndex];
        if ((CacheTag->RealSize > 0) && CacheTag->Dirty) {
          //
          // Write back all Dirty Data Cache Page to disk
          //
          Status = FatExchangeCachePage (Volume, CacheDataType, WriteDisk, CacheTag, Task);
          if (EFI_ERROR (Status)) {
            return Status;
          }
        }
      }

      DiskCache->Dirty = FALSE;
    }
  }

  //
  // Flush the block device.
  //
  Status = Volume->BlockIo->FlushBlocks (Volume->BlockIo);
  return Status;
}

/**

  Initialize the disk cache according to Volume's FatType.

  @param  Volume                - FAT file system volume.

  @retval EFI_SUCCESS           - The disk cache is successfully initialized.
  @retval EFI_OUT_OF_RESOURCES  - Not enough memory to allocate disk cache.

**/
EFI_STATUS
FatInitializeDiskCache (
  IN FAT_VOLUME  *Volume
  )
{
  DISK_CACHE  *DiskCache;
  UINTN       FatCacheGroupCount;
  UINTN       DataCacheSize;
  UINTN       FatCacheSize;
  UINT8       *CacheBuffer;

  DiskCache = Volume->DiskCache;
  //
  // Configure the parameters of disk cache
  //
  if (Volume->FatType == Fat12) {
    FatCacheGroupCount                 = FAT_FATCACHE_GROUP_MIN_COUNT;
    DiskCache[CacheFat].PageAlignment  = FAT_FATCACHE_PAGE_MIN_ALIGNMENT;
    DiskCache[CacheData].PageAlignment = FAT_DATACACHE_PAGE_MIN_ALIGNMENT;
  } else {
    FatCacheGroupCount                 = FAT_FATCACHE_GROUP_MAX_COUNT;
    DiskCache[CacheFat].PageAlignment  = FAT_FATCACHE_PAGE_MAX_ALIGNMENT;
    DiskCache[CacheData].PageAlignment = FAT_DATACACHE_PAGE_MAX_ALIGNMENT;
  }

  DiskCache[CacheData].GroupMask    = FAT_DATACACHE_GROUP_COUNT - 1;
  DiskCache[CacheData].BaseAddress  = Volume->RootPos;
  DiskCache[CacheData].LimitAddress = Volume->VolumeSize;
  DiskCache[CacheFat].GroupMask     = FatCacheGroupCount - 1;
  DiskCache[CacheFat].BaseAddress   = Volume->FatPos;
  DiskCache[CacheFat].LimitAddress  = Volume->FatPos + Volume->FatSize;
  FatCacheSize                      = FatCacheGroupCount << DiskCache[CacheFat].PageAlignment;
  DataCacheSize                     = FAT_DATACACHE_GROUP_COUNT << DiskCache[CacheData].PageAlignment;
  //
  // Allocate the Fat Cache buffer
  //
  CacheBuffer = AllocateZeroPool (FatCacheSize + DataCacheSize);
  if (CacheBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Volume->CacheBuffer            = CacheBuffer;
  DiskCache[CacheFat].CacheBase  = CacheBuffer;
  DiskCache[CacheData].CacheBase = CacheBuffer + FatCacheSize;
  return EFI_SUCCESS;
}
