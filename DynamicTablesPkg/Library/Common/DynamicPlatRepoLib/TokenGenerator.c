/** @file
  Token Generator

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#include <Protocol/ConfigurationManagerProtocol.h>

/** Generate a token.

  @return A token.
**/
CM_OBJECT_TOKEN
EFIAPI
GenerateToken (
  VOID
  )
{
  // Start Tokens at 1 to avoid collisions with CM_NULL_TOKEN.
  STATIC UINTN  CurrentToken = 1;

  return (CM_OBJECT_TOKEN)(CurrentToken++);
}
