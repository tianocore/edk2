/** @file
  Interface functions for the Memory Debug Log Library.

  Copyright (C) 2025, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _MEM_DEBUG_LOG_LIB_H_
#define _MEM_DEBUG_LOG_LIB_H_

#include <Uefi/UefiBaseType.h>
#include <Base.h>

//
// Cap max buffer at 2MB (0x200 4K pages)
//
#define MAX_MEM_DEBUG_LOG_PAGES  0x200

#define MEM_DEBUG_LOG_MAGIC1  0x3167646d666d766f  // "ovmfmdg1"
#define MEM_DEBUG_LOG_MAGIC2  0x3267646d666d766f  // "ovmfmdg2"

#pragma pack(1)
//
// Mem Debug Log buffer header.
// The Log buffer is circular. Only the most
// recent messages are retained. Older messages
// will be discarded if the buffer overflows.
// The Debug Log starts just after the header.
//
typedef struct {
  //
  // Magic values
  // These fields are used by tools to locate the buffer in
  // memory. These MUST be the first two fields of the structure.
  // Use a 128 bit Magic to vastly reduce the possibility of
  // a collision with random data in memory.
  UINT64             Magic1;
  UINT64             Magic2;
  //
  // Header Size
  // This MUST be the third field of the structure
  //
  UINT64             HeaderSize;
  //
  // Debug log size (minus header)
  //
  UINT64             DebugLogSize;
  //
  // Protect the log from potential MP access (by APs during
  // vCPU init) to maintain integrity of the Head/Tail Offsets.
  // NOTE: MemDebugLogLock is used as a SPIN_LOCK (which is type
  // UINTN). Thus, we declared it as a UINT64 to ensure a
  // consistent structure size.
  //
  volatile UINT64    MemDebugLogLock;
  //
  // Debug log head offset
  //
  UINT64             DebugLogHeadOffset;
  //
  //  Debug log tail offset
  //
  UINT64             DebugLogTailOffset;
  //
  // Flag to indicate if the buffer wrapped and was thus truncated.
  //
  UINT64             Truncated;
  //
  // Firmware Build Version (PcdFirmwareVersionString)
  //
  CHAR8              FirmwareVersion[128];
} MEM_DEBUG_LOG_HDR;

//
// HOB used to pass the mem debug log buffer addr from PEI to DXE
//
typedef struct {
  EFI_PHYSICAL_ADDRESS    MemDebugLogBufAddr;
} MEM_DEBUG_LOG_HOB_DATA;

#pragma pack()

/**
  Write a CHAR8 string to the memory debug log.
  This is the interface function used by DebugLib.
  There are several versions for each boot
  phase (i.e. SEC, PEI, DXE, Runtime).
  Each version will obtain the proper memory debug log
  buffer address and call MemDebugLogWriteBuffer().

  @param[in] Buffer              The buffer containing the string of CHAR8s

  @param[in] Length              The buffer length (number of CHAR8s)
                                 not including the NULL terminator byte.

  @retval RETURN_SUCCESS         String succcessfully written to the memory log buffer.

  @retval RETURN_NOT_FOUND       Memory log buffer is not properly initialized.

  @retval EFI_INVALID_PARAMETER   Invalid input parameters.
**/
EFI_STATUS
EFIAPI
MemDebugLogWrite (
  IN  CHAR8  *Buffer,
  IN  UINTN  Length
  );

/**
  Return the memory debug log buffer size (in pages).
  This function is implemented by PEIM version of
  MemDebugLogLib only.

  @retval UINT32                 Buffer size in pages
**/
UINT32
EFIAPI
MemDebugLogPages (
  VOID
  );

/**
  Write a CHAR8 string to a memory debug log circular
  buffer located at the given address.

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
MemDebugLogWriteBuffer (
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
MemDebugLogInit (
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
MemDebugLogCopy (
  IN EFI_PHYSICAL_ADDRESS  MemDebugLogBufDestAddr,
  IN EFI_PHYSICAL_ADDRESS  MemDebugLogBufSrcAddr
  );

/**
  Obtain the Memory Debug Log Buffer Addr from the HOB

  @param MemDebugLogBufAddr       Address of memory debug log buffer.

  @retval RETURN_SUCCESS          Log buffer address successfuly obtained.

  @retval EFI_NOT_FOUND           HOB not found.
**/
EFI_STATUS
EFIAPI
MemDebugLogAddrFromHOB (
  EFI_PHYSICAL_ADDRESS  *MemDebugLogBufAddr
  );

/**
  Return whether the Memory Debug Logging feature is enabled

  @retval TRUE                    Feature is enabled

  @retval FALSE                   Feature is not enabled
**/
BOOLEAN
EFIAPI
MemDebugLogEnabled (
  VOID
  );

#endif // _MEM_DEBUG_LOG_LIB_H_
