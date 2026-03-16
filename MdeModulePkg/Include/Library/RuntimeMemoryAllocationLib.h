/** @file
  Light-weight Memory Management Routines at Runtime Phase.

  Copyright (c) 2025, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#pragma once

/**
  Allocates a buffer at runtime phase.

  @param[in]  AllocationSize    Bytes to be allocated.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

 **/
VOID *
EFIAPI
RuntimeAllocateMem (
  IN  UINTN  AllocationSize
  );

/**
  Reallocates a buffer at runtime phase.

  @param[in]  OldPtr            Origin allocated memory.
  @param[in]  Size              Bytes to be allocated.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

 **/
VOID *
EFIAPI
RuntimeReallocateMem (
  IN  VOID   *OldPtr,
  IN  UINTN  Size
  );

/**
  Frees a buffer that was previously allocated at runtime phase.

  @param[in]  Buffer  Pointer to the buffer to free.

 **/
VOID
EFIAPI
RuntimeFreeMem (
  IN  VOID  *Buffer
  );
