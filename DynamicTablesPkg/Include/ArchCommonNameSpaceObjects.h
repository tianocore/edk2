/** @file

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard
**/

#ifndef ARCH_COMMON_NAMESPACE_OBJECTS_H_
#define ARCH_COMMON_NAMESPACE_OBJECTS_H_

#include <AcpiObjects.h>
#include <StandardNameSpaceObjects.h>

/** The EARCH_COMMON_OBJECT_ID enum describes the Object IDs
    in the Arch Common Namespace
*/
typedef enum ArchCommonObjectID {
  EArchCommonObjReserved,                       ///<  0 - Reserved
  EArchCommonObjMax
} EARCH_COMMON_OBJECT_ID;

#endif // ARCH_COMMON_NAMESPACE_OBJECTS_H_
