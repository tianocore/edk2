/** @file
  Token Generator

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#pragma once

/** Generate a token.

  @return A token.
**/
CM_OBJECT_TOKEN
EFIAPI
GenerateToken (
  VOID
  );
