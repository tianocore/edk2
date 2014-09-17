/** @file
  Defines data structure that is the headers found at the runtime
  updatable firmware volumes, such as the FileSystemGuid of the
  working block, the header structure of the variable block, FTW
  working block, or event log block.

  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
