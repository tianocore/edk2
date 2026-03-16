/** @file
  Common utility functions for FMP Authentication PKCS7 library.

  Copyright (c) 2025, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FMP_AUTHENTICATION_COMMON_H_
#define FMP_AUTHENTICATION_COMMON_H_

extern BOOLEAN  mFmpAuthenticationExitBootServiceSignalled;

/**
  Allocates a temp buffer for FMP authentication

  @param[in]  AllocationSize    Bytes to be allocated.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

 **/
VOID *
EFIAPI
AllocateTempBuffer (
  IN  UINTN  AllocationSize
  );

/**
  Frees a temp buffer allocated with AllocateTempBuffer().

  @param[in]  Buffer  Pointer to the buffer to free.

 **/
VOID
EFIAPI
FreeTempBuffer (
  IN  VOID  *Buffer
  );

#endif
