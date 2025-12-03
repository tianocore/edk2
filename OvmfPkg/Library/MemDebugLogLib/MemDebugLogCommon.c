/** @file
  Memory Debug Log common defs/funcs to access the memory buffer.

  Copyright (C) 2025, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/PrintLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/MemDebugLogLib.h>
#include <Library/PcdLib.h>

#define MEMDEBUGLOG_COPYSIZE  0x200

STATIC
VOID
MemDebugLogLockInit (
  IN volatile UINT64  *MemDebugLogLock
  )
{
  InitializeSpinLock ((SPIN_LOCK *)MemDebugLogLock);
}

STATIC
BOOLEAN
MemDebugLogLockAcquire (
  IN volatile UINT64  *MemDebugLogLock
  )
{
  return AcquireSpinLockOrFail ((SPIN_LOCK *)MemDebugLogLock);
}

STATIC
VOID
MemDebugLogLockRelease (
  IN volatile UINT64  *MemDebugLogLock
  )
{
  ReleaseSpinLock ((SPIN_LOCK *)MemDebugLogLock);
}

EFI_STATUS
EFIAPI
MemDebugLogWriteBuffer (
  IN  EFI_PHYSICAL_ADDRESS  MemDebugLogBufAddr,
  IN  CHAR8                 *Buffer,
  IN  UINTN                 Length
  )
{
  volatile UINT64    *MemDebugLogLock;
  MEM_DEBUG_LOG_HDR  *MemDebugLogHdr;
  UINTN              BufSpaceLeft;
  CHAR8              *BufStart;
  CHAR8              *BufHead;
  CHAR8              *BufTail;
  CHAR8              *BufEnd;

  //
  // NOTE: we cannot call DEBUG or ASSERT from this function.
  //

  if (!MemDebugLogBufAddr || !Buffer) {
    return EFI_INVALID_PARAMETER;
  }

  if (Length == 0) {
    return EFI_SUCCESS;
  }

  MemDebugLogHdr  = (MEM_DEBUG_LOG_HDR *)(UINTN)MemDebugLogBufAddr;
  MemDebugLogLock = &(MemDebugLogHdr->MemDebugLogLock);

  //
  // Validate the header magic before proceeding
  //
  if ((MemDebugLogHdr->Magic1 != MEM_DEBUG_LOG_MAGIC1) ||
      (MemDebugLogHdr->Magic2 != MEM_DEBUG_LOG_MAGIC2))
  {
    return EFI_NOT_FOUND;
  }

  if (Length >= MemDebugLogHdr->DebugLogSize) {
    return EFI_INVALID_PARAMETER;
  }

  if (!MemDebugLogLockAcquire (MemDebugLogLock)) {
    return EFI_NOT_READY;
  }

  BufStart = (CHAR8 *)(UINTN)(MemDebugLogBufAddr + MemDebugLogHdr->HeaderSize);
  BufEnd   = (CHAR8 *)(UINTN)(MemDebugLogBufAddr + MemDebugLogHdr->HeaderSize + MemDebugLogHdr->DebugLogSize) - 1;
  BufHead  = BufStart + MemDebugLogHdr->DebugLogHeadOffset;
  BufTail  = BufStart + MemDebugLogHdr->DebugLogTailOffset;

  //
  // Maintain a circular (wrap around) log buffer
  // NOTES:
  // tail always points to next available slot to populate
  // Algorithm to process/display strings from buffer in time order:
  // 1. head==tail indicates empty buffer
  // 2. if (head < tail), process from head (tail-head) bytes
  // 3. if (head > tail), process from head (bufend-head) bytes
  //                      process from bufstart (tail-bufstart) bytes
  //

  if ((BufTail + Length) <= BufEnd) {
    //
    //  There's enough room from tail to end of the buffer
    //
    CopyMem (BufTail, Buffer, Length);
    //
    // If we have previously wrapped around, need to keep Head updated
    //
    if (BufHead == (BufTail + 1)) {
      BufHead += Length;
      //
      // Check if we need to wrap Head
      //
      if (BufHead > BufEnd) {
        BufHead = BufStart;
      }
    }

    BufTail += Length;
  } else {
    //
    // We need to wrap around.
    //
    // Fill remaining buffer space with initial part of the string
    //
    BufSpaceLeft = (UINTN)(BufEnd - BufTail + 1);
    CopyMem (BufTail, Buffer, BufSpaceLeft);

    //
    // Wrap to start of the buffer for the rest of the string
    //
    BufTail = BufStart;
    CopyMem (BufTail, (Buffer + BufSpaceLeft), (Length - BufSpaceLeft));
    BufTail += (Length - BufSpaceLeft);
    BufHead  = (BufTail + 1);

    MemDebugLogHdr->Truncated = 1;
  }

  //
  // Write the new buffer offsets back to the header
  //
  MemDebugLogHdr->DebugLogHeadOffset = BufHead - BufStart;
  MemDebugLogHdr->DebugLogTailOffset = BufTail - BufStart;

  MemDebugLogLockRelease (MemDebugLogLock);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MemDebugLogInit (
  IN EFI_PHYSICAL_ADDRESS  MemDebugLogBufAddr,
  UINT32                   MemDebugLogBufSize
  )
{
  MEM_DEBUG_LOG_HDR  *MemDebugLogHdr;

  if (MemDebugLogBufAddr == 0) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem ((VOID *)(UINTN)MemDebugLogBufAddr, MemDebugLogBufSize);

  MemDebugLogHdr                     = (MEM_DEBUG_LOG_HDR *)(UINTN)MemDebugLogBufAddr;
  MemDebugLogHdr->Magic1             = MEM_DEBUG_LOG_MAGIC1;
  MemDebugLogHdr->Magic2             = MEM_DEBUG_LOG_MAGIC2;
  MemDebugLogHdr->HeaderSize         = sizeof (MEM_DEBUG_LOG_HDR);
  MemDebugLogHdr->DebugLogSize       = (MemDebugLogBufSize - MemDebugLogHdr->HeaderSize);
  MemDebugLogHdr->DebugLogHeadOffset = 0;
  MemDebugLogHdr->DebugLogTailOffset = 0;
  MemDebugLogLockInit (&(MemDebugLogHdr->MemDebugLogLock));
  MemDebugLogHdr->Truncated = 0;
  AsciiSPrint (MemDebugLogHdr->FirmwareVersion, 128, "%s", (CHAR16 *)PcdGetPtr (PcdFirmwareVersionString));

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MemDebugLogCopy (
  IN EFI_PHYSICAL_ADDRESS  MemDebugLogBufDestAddr,
  IN EFI_PHYSICAL_ADDRESS  MemDebugLogBufSrcAddr
  )
{
  MEM_DEBUG_LOG_HDR  *MemDebugLogSrcHdr;
  MEM_DEBUG_LOG_HDR  *MemDebugLogDestHdr;
  CHAR8              *BufStart;
  CHAR8              *BufHead;
  CHAR8              *BufTail;
  CHAR8              *BufEnd;
  CHAR8              *BufPtr;

  if ((MemDebugLogBufSrcAddr == 0) || (MemDebugLogBufDestAddr == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  MemDebugLogSrcHdr  = (MEM_DEBUG_LOG_HDR *)(UINTN)MemDebugLogBufSrcAddr;
  MemDebugLogDestHdr = (MEM_DEBUG_LOG_HDR *)(UINTN)MemDebugLogBufDestAddr;

  BufStart = (CHAR8 *)(UINTN)(MemDebugLogBufSrcAddr + MemDebugLogSrcHdr->HeaderSize);
  BufEnd   = (CHAR8 *)(UINTN)(MemDebugLogBufSrcAddr + MemDebugLogSrcHdr->HeaderSize + MemDebugLogSrcHdr->DebugLogSize);
  BufHead  = BufStart + MemDebugLogSrcHdr->DebugLogHeadOffset;
  BufTail  = BufStart + MemDebugLogSrcHdr->DebugLogTailOffset;

  MemDebugLogDestHdr->Truncated = MemDebugLogSrcHdr->Truncated;

  if (BufHead == BufTail) {
    //
    // Source Debug Log empty
    //
    return EFI_SUCCESS;
  } else if (BufHead < BufTail) {
    //
    // Source buffer didn't wrap, so copy debug messages
    // from Source buffer (head to tail) to the Dest buffer
    // NOTE: we limit each copy to MEMDEBUGLOG_COPYSIZE
    // to ensure to not copy too much at a time and ensure
    // the dest buffer head/tail pointers are created properly.
    //
    for (BufPtr = BufHead; (BufTail - BufPtr) > MEMDEBUGLOG_COPYSIZE; BufPtr += MEMDEBUGLOG_COPYSIZE) {
      MemDebugLogWriteBuffer (MemDebugLogBufDestAddr, BufPtr, MEMDEBUGLOG_COPYSIZE);
    }

    //
    // write remaining bytes
    //
    MemDebugLogWriteBuffer (MemDebugLogBufDestAddr, BufPtr, (BufTail - BufPtr));
  } else {
    //
    // Source buffer wrapped.
    // First copy (bufend - head) chars from head to Dest buffer
    //
    for (BufPtr = BufHead; (BufEnd - BufPtr) > MEMDEBUGLOG_COPYSIZE; BufPtr += MEMDEBUGLOG_COPYSIZE) {
      MemDebugLogWriteBuffer (MemDebugLogBufDestAddr, BufPtr, MEMDEBUGLOG_COPYSIZE);
    }

    //
    // write remaining bytes
    //
    MemDebugLogWriteBuffer (MemDebugLogBufDestAddr, BufPtr, (BufEnd - BufPtr));

    //
    // Next, copy (bufend - head) chars from start to Dest buffer
    //
    for (BufPtr = BufStart; (BufTail - BufPtr) > MEMDEBUGLOG_COPYSIZE; BufPtr += MEMDEBUGLOG_COPYSIZE) {
      MemDebugLogWriteBuffer (MemDebugLogBufDestAddr, BufPtr, MEMDEBUGLOG_COPYSIZE);
    }

    //
    // write remaining bytes
    //
    MemDebugLogWriteBuffer (MemDebugLogBufDestAddr, BufPtr, (BufTail - BufPtr));
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MemDebugLogAddrFromHOB (
  EFI_PHYSICAL_ADDRESS  *MemDebugLogBufAddr
  )
{
  EFI_HOB_GUID_TYPE       *GuidHob;
  MEM_DEBUG_LOG_HOB_DATA  *HobData;

  GuidHob = GetFirstGuidHob (&gMemDebugLogHobGuid);
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  } else {
    HobData             = (MEM_DEBUG_LOG_HOB_DATA *)GET_GUID_HOB_DATA (GuidHob);
    *MemDebugLogBufAddr = HobData->MemDebugLogBufAddr;
  }

  return EFI_SUCCESS;
}

BOOLEAN
EFIAPI
MemDebugLogEnabled (
  VOID
  )
{
  return TRUE;
}
