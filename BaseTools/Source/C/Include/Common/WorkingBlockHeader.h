/** @file
  Defines data structure that is the headers found at the runtime
  updatable firmware volumes, such as the FileSystemGuid of the
  working block, the header structure of the variable block, FTW
  working block, or event log block.

  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_WORKING_BLOCK_HEADER_H__
#define __EFI_WORKING_BLOCK_HEADER_H__

//
// EFI Fault tolerant working block header
// The header is immediately followed by the write queue.
//
typedef struct {
  EFI_GUID  Signature;
  UINT32    Crc;
  UINT8     WorkingBlockValid : 1;
  UINT8     WorkingBlockInvalid : 1;
#define WORKING_BLOCK_VALID   0x1
#define WORKING_BLOCK_INVALID 0x2
  UINT8     Reserved : 6;
  UINT8     Reserved3[3];
  UINT64    WriteQueueSize;
  //
  // UINT8                WriteQueue[WriteQueueSize];
  //
} EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER;

#endif
