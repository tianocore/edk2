/** @file
  Interface functions for the Memory Debug Log Library.

  Copyright (C) 2025, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _MEM_DEBUG_LOG_LIB_H_
#define _MEM_DEBUG_LOG_LIB_H_

#include <Uefi/UefiBaseType.h>
#include <Base.h>

/**
  Write a CHAR8 string to the memory debug log

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

#endif // _MEM_DEBUG_LOG_LIB_H_
