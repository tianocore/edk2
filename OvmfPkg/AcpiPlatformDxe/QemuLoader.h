/** @file
  Command structures for the QEMU FwCfg table loader interface.

  Copyright (C) 2014, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __QEMU_LOADER_H__
#define __QEMU_LOADER_H__

#include <Include/Base.h>
#include <Library/QemuFwCfgLib.h>

//
// The types and the documentation reflects the SeaBIOS interface.
//
#define QEMU_LOADER_FNAME_SIZE QEMU_FW_CFG_FNAME_SIZE

typedef enum {
  QemuLoaderCmdAllocate = 1,
  QemuLoaderCmdAddPointer,
  QemuLoaderCmdAddChecksum
} QEMU_LOADER_COMMAND_TYPE;

typedef enum {
  QemuLoaderAllocHigh = 1,
  QemuLoaderAllocFSeg
} QEMU_LOADER_ALLOC_ZONE;

#pragma pack (1)
//
// QemuLoaderCmdAllocate: download the fw_cfg file named File, to a buffer
// allocated in the zone specified by Zone, aligned at a multiple of Alignment.
//
typedef struct {
  UINT8  File[QEMU_LOADER_FNAME_SIZE]; // NUL-terminated
  UINT32 Alignment;                    // power of two
  UINT8  Zone;                         // QEMU_LOADER_ALLOC_ZONE values
} QEMU_LOADER_ALLOCATE;

//
// QemuLoaderCmdAddPointer: the bytes at
// [PointerOffset..PointerOffset+PointerSize) in the file PointerFile contain a
// relative pointer (an offset) into PointeeFile. Increment the relative
// pointer's value by the base address of where PointeeFile's contents have
// been placed (when QemuLoaderCmdAllocate has been executed for PointeeFile).
//
typedef struct {
  UINT8  PointerFile[QEMU_LOADER_FNAME_SIZE]; // NUL-terminated
  UINT8  PointeeFile[QEMU_LOADER_FNAME_SIZE]; // NUL-terminated
  UINT32 PointerOffset;
  UINT8  PointerSize;                         // one of 1, 2, 4, 8
} QEMU_LOADER_ADD_POINTER;

//
// QemuLoaderCmdAddChecksum: calculate the UINT8 checksum (as per
// CalculateChecksum8()) of the range [Start..Start+Length) in File. Store the
// UINT8 result at ResultOffset in the same File.
//
typedef struct {
  UINT8  File[QEMU_LOADER_FNAME_SIZE]; // NUL-terminated
  UINT32 ResultOffset;
  UINT32 Start;
  UINT32 Length;
} QEMU_LOADER_ADD_CHECKSUM;

typedef struct {
  UINT32 Type;                             // QEMU_LOADER_COMMAND_TYPE values
  union {
    QEMU_LOADER_ALLOCATE     Allocate;
    QEMU_LOADER_ADD_POINTER  AddPointer;
    QEMU_LOADER_ADD_CHECKSUM AddChecksum;
    UINT8                    Padding[124];
  } Command;
} QEMU_LOADER_ENTRY;
#pragma pack ()

#endif
