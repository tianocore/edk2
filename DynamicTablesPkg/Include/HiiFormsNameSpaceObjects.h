/** @file

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Hii or HII - Human Interface Infrastructure
    - Obj or OBJ - Object
    - Std or STD - Standard
**/

#pragma once

#include <HiiFormsGenerator.h>

#pragma pack(1)

/** The EHII_FORMS_OBJECT_ID enum describes the Object IDs
    in the HII Forms namespace
*/
typedef enum HiiFormsObjectID {
  EHiiFormsObjReserved,                                            ///<  0 - Reserved
  EHiiFormsObjList,                                                ///< 1 - Hii Forms Info List
  EHiiFormsObjMax
} EHII_FORMS_OBJECT_ID;

/** A structure used to describe the HII Forms generators to be invoked.
*/
typedef struct CmHiiFormsObjInfo {
  /// The Hii Form Generator ID
  HII_FORMS_GENERATOR_ID    FormGeneratorId;

  CM_OBJECT_TOKEN           FormsetToken;
} CM_HII_FORMS_OBJ_INFO;

#pragma pack()
