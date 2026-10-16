/** @file

  Copyright (c) 2026, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Base.h>
#include <HiiFormsGenerator.h>

#include <Uefi/UefiInternalFormRepresentation.h>

// Information pertaining to individual forms which gets
// used when freeing up the form resources.
typedef struct {
  /// List of HII form generators
  LIST_ENTRY                         List;

  /// Form info struct
  CONST CM_STD_OBJ_HII_FORMS_INFO    *HiiFormInfo;

  /// Form generator object
  HII_FORMS_GENERATOR                *Generator;

  /// HII handle
  EFI_HII_HANDLE                     HiiHandle;
} HII_FORM_INFO;

/** Free up all form resources

  Free up the form resources by iterating the HII_FORM_INFO
  and HII_FORM_HANDLE lists.

**/
VOID
EFIAPI
FreeHiiFormResources (
  VOID
  );
