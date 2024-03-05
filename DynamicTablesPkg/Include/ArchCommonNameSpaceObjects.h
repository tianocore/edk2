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
  EArchCommonObjPowerManagementProfileInfo,     ///<  1 - Power Management Profile Info
  EArchCommonObjMax
} EARCH_COMMON_OBJECT_ID;

#pragma pack(1)

/** A structure that describes the
    Power Management Profile Information for the Platform.

    ID: EArchCommonObjPowerManagementProfileInfo
*/
typedef struct CmArchCommonPowerManagementProfileInfo {
  /** This is the Preferred_PM_Profile field of the FADT Table
      described in the ACPI Specification
  */
  UINT8    PowerManagementProfile;
} CM_ARCH_COMMON_POWER_MANAGEMENT_PROFILE_INFO;


#pragma pack()

#endif // ARCH_COMMON_NAMESPACE_OBJECTS_H_
