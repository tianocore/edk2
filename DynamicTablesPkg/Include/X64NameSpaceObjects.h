/** @file

  Defines the X64 Namespace Object.

  Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - X64 or x64 - X64 Architecture
**/

#ifndef X64_NAMESPACE_OBJECTS_H_
#define X64_NAMESPACE_OBJECTS_H_

/** The EX64_OBJECT_ID enum describes the Object IDs
    in the X64 Namespace
*/
typedef enum X64ObjectID {
  EX64ObjReserved,               ///<  0 - Reserved
  EX64ObjMax
} EX64_OBJECT_ID;

#endif // X64_NAMESPACE_OBJECTS_H_
