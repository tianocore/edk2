/** @file

 Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>

 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __LOCK_BOX_LIB_IMPL_H__
#define __LOCK_BOX_LIB_IMPL_H__

#pragma pack(1)

typedef struct {
  UINT32    Signature;
  UINT32    SubPageBuffer;
  UINT32    SubPageRemaining;
} LOCK_BOX_GLOBAL;

#define LOCK_BOX_GLOBAL_SIGNATURE  SIGNATURE_32('L', 'B', 'G', 'S')

extern LOCK_BOX_GLOBAL  *mLockBoxGlobal;

#pragma pack()

/**
  Allocates a buffer of type EfiACPIMemoryNVS.

  Allocates the number bytes specified by AllocationSize of type
  EfiACPIMemoryNVS and returns a pointer to the allocated buffer.
  If AllocationSize is 0, then a valid buffer of 0 size is
  returned.  If there is not enough memory remaining to satisfy
  the request, then NULL is returned.

  @param  AllocationSize        The number of bytes to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateAcpiNvsPool (
  IN UINTN  AllocationSize
  );

RETURN_STATUS
EFIAPI
LockBoxLibInitialize (
  VOID
  );

#endif
