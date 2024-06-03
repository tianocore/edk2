/** @file
  Include file for Var Check Hii bin generation.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VAR_CHECK_HII_GEN_H_
#define _VAR_CHECK_HII_GEN_H_

#include "VarCheckHii.h"
extern VAR_CHECK_HII_VARIABLE_HEADER  *mVarCheckHiiBin;
extern UINTN                          mVarCheckHiiBinSize;

/**
  Dump Hii Package.

  @param[in] HiiPackage         Pointer to Hii Package.

**/
VOID
DumpHiiPackage (
  IN VOID  *HiiPackage
  );

/**
  Dump Hii Database.

  @param[in] HiiDatabase        Pointer to Hii Database.
  @param[in] HiiDatabaseSize    Hii Database size.

**/
VOID
DumpHiiDatabase (
  IN VOID   *HiiDatabase,
  IN UINTN  HiiDatabaseSize
  );

/**
  Allocates and zeros a buffer of type EfiBootServicesData.

  Allocates the number bytes specified by AllocationSize of type EfiBootServicesData, clears the
  buffer with zeros, and returns a pointer to the allocated buffer.  If AllocationSize is 0, then a
  valid buffer of 0 size is returned.  If there is not enough memory remaining to satisfy the
  request, then NULL is returned.

  @param  AllocationSize        The number of bytes to allocate and zero.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
InternalVarCheckAllocateZeroPool (
  IN UINTN  AllocationSize
  );

/**
  Frees a buffer that was previously allocated with one of the pool allocation functions in the
  Memory Allocation Library.

  Frees the buffer specified by Buffer.  Buffer must have been allocated on a previous call to the
  pool allocation services of the Memory Allocation Library.  If it is not possible to free pool
  resources, then this function will perform no actions.

  If Buffer was not allocated with a pool allocation function in the Memory Allocation Library,
  then ASSERT().

  @param  Buffer                The pointer to the buffer to free.

**/
VOID
EFIAPI
InternalVarCheckFreePool (
  IN VOID  *Buffer
  );

/**
  Var Check Parse Hii Package.

  @param[in] HiiPackage         Pointer to Hii Package.
  @param[in] FromFv             Hii Package from FV.

**/
VOID
VarCheckParseHiiPackage (
  IN VOID     *HiiPackage,
  IN BOOLEAN  FromFv
  );

/**
  Var Check Parse Hii Database.

  @param[in] HiiDatabase        Pointer to Hii Database.
  @param[in] HiiDatabaseSize    Hii Database size.

**/
VOID
VarCheckParseHiiDatabase (
  IN VOID   *HiiDatabase,
  IN UINTN  HiiDatabaseSize
  );

/**
  Generate from FV.

**/
VOID
VarCheckHiiGenFromFv (
  VOID
  );

/**
  Generate from Hii Database.

**/
VOID
VarCheckHiiGenFromHiiDatabase (
  VOID
  );

/**
  Generate VarCheckHiiBin from Hii Database and FV.

**/
VOID
EFIAPI
VarCheckHiiGen (
  VOID
  );

#endif
