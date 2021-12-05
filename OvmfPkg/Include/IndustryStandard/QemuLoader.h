/** @file
  Command structures for the QEMU FwCfg table loader interface.

  Copyright (C) 2014, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef QEMU_LOADER_H_
#define QEMU_LOADER_H_

#include <Base.h>
#include <IndustryStandard/QemuFwCfg.h>

//
// The types and the documentation reflects the SeaBIOS interface.
//
#define QEMU_LOADER_FNAME_SIZE  QEMU_FW_CFG_FNAME_SIZE

typedef enum {
  QemuLoaderCmdAllocate = 1,
  QemuLoaderCmdAddPointer,
  QemuLoaderCmdAddChecksum,
  QemuLoaderCmdWritePointer,
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
  UINT8     File[QEMU_LOADER_FNAME_SIZE]; // NUL-terminated
  UINT32    Alignment;                    // power of two
  UINT8     Zone;                         // QEMU_LOADER_ALLOC_ZONE values
} QEMU_LOADER_ALLOCATE;

//
// QemuLoaderCmdAddPointer: the bytes at
// [PointerOffset..PointerOffset+PointerSize) in the file PointerFile contain a
// relative pointer (an offset) into PointeeFile. Increment the relative
// pointer's value by the base address of where PointeeFile's contents have
// been placed (when QemuLoaderCmdAllocate has been executed for PointeeFile).
//
typedef struct {
  UINT8     PointerFile[QEMU_LOADER_FNAME_SIZE]; // NUL-terminated
  UINT8     PointeeFile[QEMU_LOADER_FNAME_SIZE]; // NUL-terminated
  UINT32    PointerOffset;
  UINT8     PointerSize;                      // one of 1, 2, 4, 8
} QEMU_LOADER_ADD_POINTER;

//
// QemuLoaderCmdAddChecksum: calculate the UINT8 checksum (as per
// CalculateChecksum8()) of the range [Start..Start+Length) in File. Store the
// UINT8 result at ResultOffset in the same File.
//
typedef struct {
  UINT8     File[QEMU_LOADER_FNAME_SIZE]; // NUL-terminated
  UINT32    ResultOffset;
  UINT32    Start;
  UINT32    Length;
} QEMU_LOADER_ADD_CHECKSUM;

//
// QemuLoaderCmdWritePointer: the bytes at
// [PointerOffset..PointerOffset+PointerSize) in the writeable fw_cfg file
// PointerFile are to receive the absolute address of PointeeFile, as allocated
// and downloaded by the firmware, incremented by the value of PointeeOffset.
// Store the sum of (a) the base address of where PointeeFile's contents have
// been placed (when QemuLoaderCmdAllocate has been executed for PointeeFile)
// and (b) PointeeOffset, to this portion of PointerFile.
//
// This command is similar to QemuLoaderCmdAddPointer; the difference is that
// the "pointer to patch" does not exist in guest-physical address space, only
// in "fw_cfg file space". In addition, the "pointer to patch" is not
// initialized by QEMU in-place with a possibly nonzero offset value: the
// relative offset into PointeeFile comes from the explicit PointeeOffset
// field.
//
typedef struct {
  UINT8     PointerFile[QEMU_LOADER_FNAME_SIZE]; // NUL-terminated
  UINT8     PointeeFile[QEMU_LOADER_FNAME_SIZE]; // NUL-terminated
  UINT32    PointerOffset;
  UINT32    PointeeOffset;
  UINT8     PointerSize;                      // one of 1, 2, 4, 8
} QEMU_LOADER_WRITE_POINTER;

typedef struct {
  UINT32    Type;                          // QEMU_LOADER_COMMAND_TYPE values
  union {
    QEMU_LOADER_ALLOCATE         Allocate;
    QEMU_LOADER_ADD_POINTER      AddPointer;
    QEMU_LOADER_ADD_CHECKSUM     AddChecksum;
    QEMU_LOADER_WRITE_POINTER    WritePointer;
    UINT8                        Padding[124];
  } Command;
} QEMU_LOADER_ENTRY;
#pragma pack ()

#endif
