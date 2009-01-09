/** @file

  The header structure of FTW working block region.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_WORKING_BLOCK_HEADER_H__
#define __EFI_WORKING_BLOCK_HEADER_H__

#define WORKING_BLOCK_VALID   0x1
#define WORKING_BLOCK_INVALID 0x2

///
/// EDKII Fault tolerant working block header
/// The header is immediately followed by the write queue data.
///
typedef struct {
  ///
  /// System Non Volatile FV Guid
  ///
  EFI_GUID  Signature;
  ///
  /// 32bit CRC caculated for this header
  ///
  UINT32    Crc;
  ///
  /// Working block valid bit
  ///
  UINT8     WorkingBlockValid : 1;
  UINT8     WorkingBlockInvalid : 1;
  UINT8     Reserved : 6;
  UINT8     Reserved3[3];
  ///
  /// Total size of the following write queue range.
  ///
  UINT64    WriteQueueSize;
  ///
  /// Write Queue data
  /// UINT8                WriteQueue[WriteQueueSize];
  ///
} EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER;

#endif
