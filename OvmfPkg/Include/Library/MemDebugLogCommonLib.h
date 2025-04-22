/** @file
  Memory Debug Log common defs/funcs to access the memory buffer.

  Copyright (C) 2025, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __MEM_DEBUG_LOG_COMMON_H_
#define __MEM_DEBUG_LOG_COMMON_H_

#include <Uefi/UefiBaseType.h>

//
// Cap max buffer at 4MB (0x400 4K pages)
//
#define MAX_MEM_DEBUG_LOG_PAGES  0x400

#define MEM_DEBUG_LOG_MAGIC1  0x646d656d666d766f  // "ovmfmemd"
#define MEM_DEBUG_LOG_MAGIC2  0x6367616d67756265  // "ebugmagc"

#pragma pack(1)
//
// Mem Debug Log buffer header.
// The Log buffer is circular. Only the most
// recent messages are retained. Older messages
// will be discarded if the buffer overflows.
// The Debug Log starts just after the header.
// Versioning the structure allows for future expansion.
//
typedef struct {
  //
  // Magic values used by tools to locate the buffer in memory
  // These MUST be the first two fields of the structure.
  // Use a 128 bit Magic to make sure to avoid any possible
  // collision with random data in memory.
  UINT64             Magic1;
  UINT64             Magic2;
  //
  // Structure version- MUST be third field and set to 1
  //
  UINT8              Version;
  //
  // Debug log size minus header
  //
  UINT32             DebugLogSize;
  //
  // Debug log head offset
  //
  UINT32             DebugLogHeadOffset;
  //
  //  Debug log tail offset
  //
  UINT32             DebugLogTailOffset;
  //
  // Protect the log from MP access
  //
  volatile UINT64    MemDebugLogLock;
  //
  // Flag to indicate if the buffer wrapped and was thus truncated.
  //
  UINT8              Truncated;
  //
  // Firmware Build Version (PcdFirmwareVersionString)
  //
  CHAR8              FirmwareVersion[32];
} MEM_DEBUG_LOG_HDR_V1;

//
// HOB used to pass the mem debug log buffer addr from PEI to DXE
//
typedef struct {
  EFI_PHYSICAL_ADDRESS    MemDebugLogBufAddr;
} MEM_DEBUG_LOG_HOB_DATA;

#pragma pack()

/**
  Write a CHAR8 string to the memory debug log circular buffer.

  @param MemDebugLogBufAddr       Address of the memory debug log buffer.

  @param Buffer                   Pointer to a CHAR8 string to write to the
                                  debug log buffer.

  @param Length                   Length of the CHAR8 string to write to the
                                  debug log buffer. Not including NULL terminator
                                  byte.

  @retval RETURN_SUCCESS          String succcessfully written to the memory log buffer.

  @retval RETURN_NOT_FOUND        Memory log buffer is not properly initialized.

  @retval EFI_INVALID_PARAMETER   Invalid input parameters.
**/
EFI_STATUS
EFIAPI
MemDebugLogWriteCommon (
  IN EFI_PHYSICAL_ADDRESS  MemDebugLogBufAddr,
  IN CHAR8                 *Buffer,
  IN UINTN                 Length
  );

/**
  Initialize the memory debug log buffer header

  @param MemDebugLogBufAddr       Address of the memory debug log buffer.

  @param MemDebugLogBufSize       Size of the memory debug log buffer.

  @retval RETURN_SUCCESS          Log buffer successfully initialized.

  @retval EFI_INVALID_PARAMETER   Invalid input parameters.
**/
EFI_STATUS
EFIAPI
MemDebugLogInitCommon (
  IN EFI_PHYSICAL_ADDRESS  MemDebugLogBufAddr,
  UINT32                   MemDebugLogBufSize
  );

/**
  Copy the memory debug log buffer

  @param MemDebugLogBufDestAddr   Address of destination memory debug log buffer.

  @param MemDebugLogBufSrcAddr    Address of source memory debug log buffer.

  @retval RETURN_SUCCESS          Log buffer successfuly copied.

  @retval EFI_INVALID_PARAMETER   Invalid input parameters.
**/
EFI_STATUS
EFIAPI
MemDebugLogCopyCommon (
  IN EFI_PHYSICAL_ADDRESS  MemDebugLogBufDestAddr,
  IN EFI_PHYSICAL_ADDRESS  MemDebugLogBufSrcAddr
  );

/**
  Invalidate the memory debug log buffer

  @param MemDebugLogBufAddr       Address of the memory debug log buffer.

  @retval RETURN_SUCCESS          Log buffer successfuly invalidated.

  @retval EFI_INVALID_PARAMETER   Invalid input parameters.
**/
EFI_STATUS
EFIAPI
MemDebugLogInvalidateCommon (
  IN EFI_PHYSICAL_ADDRESS  MemDebugLogBufAddr
  );

#endif // __MEM_DEBUG_LOG_COMMON_H_
