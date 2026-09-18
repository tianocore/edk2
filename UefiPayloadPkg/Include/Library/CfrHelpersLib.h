/** @file
  Contains helper functions for working with CFR.

  Copyright (c) 2023, 9elements GmbH.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _CFR_HELPERS_LIB_H_
#define _CFR_HELPERS_LIB_H_

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiMultiPhase.h>
#include <Pi/PiMultiPhase.h>
#include <Guid/CfrSetupMenuGuid.h>

/**
  CFR_VARBINARY records are variable-length, so they aren't formal fields.
  Implement this handling by returning the struct at this offset.

  By incrementing the offset, this function behaves like a queue.
  Optional fields handled by returning NULL immediately.

**/
CFR_VARBINARY *
EFIAPI
CfrExtractVarBinary (
  IN     UINT8   *Buffer,
  IN OUT UINTN   *Offset,
  IN     UINT32  TargetTag
  );

/**
  Return pointers into a buffer with the requested option's default value and size.
  This may be used by code that needs CFR defaults before the full CFR driver can write variables.

  TODO: Consider returning pools instead, caller to free.

  @retval EFI_SUCCESS            The default value is found.
  @retval EFI_INVALID_PARAMETER  The function parameters are invalid.
  @retval EFI_NOT_FOUND          The option cannot be found, or type doesn't have default values.

**/
EFI_STATUS
EFIAPI
CfrOptionGetDefaultValue (
  IN     CHAR8  *FormName            OPTIONAL,
  IN     CHAR8  *OptionName,
  IN OUT VOID   **DefaultValueData,
  IN OUT UINTN  *DefaultValueLength  OPTIONAL
  );

#endif
