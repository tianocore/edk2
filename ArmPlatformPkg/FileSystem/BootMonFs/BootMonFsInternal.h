/** @file
*
*  Copyright (c) 2012-2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __BOOTMONFS_INTERNAL_H__
#define __BOOTMONFS_INTERNAL_H__

#include <PiDxe.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/SimpleFileSystem.h>

#include <Guid/BootMonFsFileInfo.h>
#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>
#include <Guid/FileSystemVolumeLabelInfo.h>

#include "BootMonFsHw.h"

#define BOOTMON_FS_VOLUME_LABEL   L"NOR Flash"

typedef struct _BOOTMON_FS_INSTANCE BOOTMON_FS_INSTANCE;

typedef struct {
  LIST_ENTRY            Link;
  VOID*                 Buffer;
  UINTN                 Size;
  UINT64                Offset; // Offset from the start of the file
} BOOTMON_FS_FILE_REGION;

typedef struct {
  UINT32                Signature;
  LIST_ENTRY            Link;
  BOOTMON_FS_INSTANCE   *Instance;

  UINTN                 HwDescAddress;
  HW_IMAGE_DESCRIPTION  HwDescription;

  EFI_FILE_PROTOCOL     File;

  //
  // The following fields are relevant only if the file is open.
  //

  EFI_FILE_INFO         *Info;
  UINT64                Position;
  // If the file needs to be flushed then this list contain the memory
  // buffer that creates this file
  LIST_ENTRY            RegionToFlushLink;
  UINT64                OpenMode;
} BOOTMON_FS_FILE;

#define BOOTMON_FS_FILE_SIGNATURE              SIGNATURE_32('b', 'o', 't', 'f')
#define BOOTMON_FS_FILE_FROM_FILE_THIS(a)      CR (a, BOOTMON_FS_FILE, File, BOOTMON_FS_FILE_SIGNATURE)
#define BOOTMON_FS_FILE_FROM_LINK_THIS(a)      CR (a, BOOTMON_FS_FILE, Link, BOOTMON_FS_FILE_SIGNATURE)

struct _BOOTMON_FS_INSTANCE {
  UINT32                               Signature;
  EFI_HANDLE                           ControllerHandle;

  LIST_ENTRY                           Link;

  EFI_DRIVER_BINDING_PROTOCOL         *Binding;
  EFI_DISK_IO_PROTOCOL                *DiskIo;
  EFI_BLOCK_IO_PROTOCOL               *BlockIo;
  EFI_BLOCK_IO_MEDIA                  *Media;
  EFI_DEVICE_PATH_PROTOCOL            *DevicePath;

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL      Fs;

  EFI_FILE_SYSTEM_INFO                 FsInfo;
  CHAR16                               Label[20];

  BOOTMON_FS_FILE                     *RootFile; // All the other files are linked to this root
  BOOLEAN                              Initialized;
};

#define BOOTMON_FS_SIGNATURE            SIGNATURE_32('b', 'o', 't', 'm')
#define BOOTMON_FS_FROM_FS_THIS(a)      CR (a, BOOTMON_FS_INSTANCE, Fs, BOOTMON_FS_SIGNATURE)
#define BOOTMON_FS_FROM_LINK(a)         CR (a, BOOTMON_FS_INSTANCE, Link, BOOTMON_FS_SIGNATURE)

#include "BootMonFsApi.h"

#endif

