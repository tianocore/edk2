/** @file

  Copyright (c) 2017 - 2024, Arm Limited. All rights reserved.
  Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - X64 or x64 - X64 Architecture
**/

#ifndef CONFIGURATION_MANAGER_OBJECT_H_
#define CONFIGURATION_MANAGER_OBJECT_H_

#include <ArchCommonNameSpaceObjects.h>
#include <ArmNameSpaceObjects.h>
#include <RiscVNameSpaceObjects.h>
#include <StandardNameSpaceObjects.h>
#include <X64NameSpaceObjects.h>

#pragma pack(1)

/** The CM_OBJECT_ID type is used to identify the Configuration Manager
    objects.

 Description of Configuration Manager Object ID
_______________________________________________________________________________
|31 |30 |29 |28 || 27 | 26 | 25 | 24 || 23 | 22 | 21 | 20 || 19 | 18 | 17 | 16|
-------------------------------------------------------------------------------
| Name Space ID ||  0 |  0 |  0 |  0 ||  0 |  0 |  0 |  0 ||  0 |  0 |  0 |  0|
_______________________________________________________________________________

Bits: [31:28] - Name Space ID
                0000 - Standard
                0001 - Arch Common
                0010 - ARM
                0011 - X64
                0100 - RISC-V
                1111 - Custom/OEM
                All other values are reserved.

Bits: [27:16] - Reserved.
_______________________________________________________________________________
|15 |14 |13 |12 || 11 | 10 |  9 |  8 ||  7 |  6 |  5 |  4 ||  3 |  2 |  1 |  0|
-------------------------------------------------------------------------------
| 0 | 0 | 0 | 0 ||  0 |  0 |  0 |  0 ||                 Object ID             |
_______________________________________________________________________________

Bits: [15:8] - Are reserved and must be zero.

Bits: [7:0] - Object ID

Object ID's in the Standard Namespace:
  0 - Configuration Manager Revision
  1 - ACPI Table List
  2 - SMBIOS Table List

Object ID's in the Arch Common Namespace:
   See EARCH_COMMON_OBJECT_ID.

Object ID's in the ARM Namespace:
   See EARM_OBJECT_ID.
*/
typedef UINT32 CM_OBJECT_ID;

//
// Helper macro to format a CM_OBJECT_ID.
//
#define FMT_CM_OBJECT_ID  "0x%lx"

/** A mask for Object ID
*/
#define OBJECT_ID_MASK  0xFF

/** A mask for Namespace ID
*/
#define NAMESPACE_ID_MASK  0xF

/** Starting bit position for Namespace ID
*/
#define NAMESPACE_ID_BIT_SHIFT  28

/** The EOBJECT_NAMESPACE_ID enum describes the defined namespaces
    for the Configuration Manager Objects.
*/
typedef enum ObjectNameSpaceID {
  EObjNameSpaceStandard,          ///< Standard Objects Namespace
  EObjNameSpaceArchCommon,        ///< Arch Common Objects Namespace
  EObjNameSpaceArm,               ///< ARM Objects Namespace
  EObjNameSpaceX64,               ///< X64 Objects Namespace
  EObjNameSpaceRiscV,             ///< RISC-V Objects Namespace
  EObjNameSpaceOem = 0xF,         ///< OEM Objects Namespace
  EObjNameSpaceMax,
} EOBJECT_NAMESPACE_ID;

/** A descriptor for Configuration Manager Objects.

  The Configuration Manager Protocol interface uses this descriptor
  to return the Configuration Manager Objects.
*/
typedef struct CmObjDescriptor {
  /// Object Id
  CM_OBJECT_ID    ObjectId;

  /// Size of the described Object or Object List
  UINT32          Size;

  /// Pointer to the described Object or Object List
  VOID            *Data;

  /// Count of objects in the list
  UINT32          Count;
} CM_OBJ_DESCRIPTOR;

#pragma pack()

/** This macro returns the namespace ID from the CmObjectID.

  @param [in] CmObjectId  The Configuration Manager Object ID.

  @retval Returns the Namespace ID corresponding to the CmObjectID.
**/
#define GET_CM_NAMESPACE_ID(CmObjectId)               \
          (((CmObjectId) >> NAMESPACE_ID_BIT_SHIFT) & \
            NAMESPACE_ID_MASK)

/** This macro returns the Object ID from the CmObjectID.

  @param [in] CmObjectId  The Configuration Manager Object ID.

  @retval Returns the Object ID corresponding to the CmObjectID.
**/
#define GET_CM_OBJECT_ID(CmObjectId)  ((CmObjectId) & OBJECT_ID_MASK)

/** This macro returns a Configuration Manager Object ID
    from the NameSpace ID and the ObjectID.

  @param [in] NameSpaceId The namespace ID for the Object.
  @param [in] ObjectId    The Object ID.

  @retval Returns the Configuration Manager Object ID.
**/
#define CREATE_CM_OBJECT_ID(NameSpaceId, ObjectId)                           \
          (((((CM_OBJECT_ID)NameSpaceId) & NAMESPACE_ID_MASK) << NAMESPACE_ID_BIT_SHIFT) | \
            (((CM_OBJECT_ID)ObjectId) & OBJECT_ID_MASK))

/** This macro returns a Configuration Manager Object ID
    in the Standard Object Namespace.

  @param [in] ObjectId    The Object ID.

  @retval Returns a Standard Configuration Manager Object ID.
**/
#define CREATE_CM_STD_OBJECT_ID(ObjectId) \
          (CREATE_CM_OBJECT_ID (EObjNameSpaceStandard, ObjectId))

/** This macro returns a Configuration Manager Object ID
    in the ARM Object Namespace.

  @param [in] ObjectId    The Object ID.

  @retval Returns an ARM Configuration Manager Object ID.
**/
#define CREATE_CM_ARM_OBJECT_ID(ObjectId) \
          (CREATE_CM_OBJECT_ID (EObjNameSpaceArm, ObjectId))

/** This macro returns a Configuration Manager Object ID
    in the RISC-V Object Namespace.

  @param [in] ObjectId    The Object ID.

  @retval Returns an RISC-V Configuration Manager Object ID.
**/
#define CREATE_CM_RISCV_OBJECT_ID(ObjectId) \
          (CREATE_CM_OBJECT_ID (EObjNameSpaceRiscV, ObjectId))

/** This macro returns a Configuration Manager Object ID
    in the Arch Common Object Namespace.

  @param [in] ObjectId    The Object ID.

  @retval Returns an Arch Common Configuration Manager Object ID.
**/
#define CREATE_CM_ARCH_COMMON_OBJECT_ID(ObjectId) \
          (CREATE_CM_OBJECT_ID (EObjNameSpaceArchCommon, ObjectId))

/** This macro returns a Configuration Manager Object ID
    in the OEM Object Namespace.

  @param [in] ObjectId    The Object ID.

  @retval Returns an OEM Configuration Manager Object ID.
**/
#define CREATE_CM_OEM_OBJECT_ID(ObjectId) \
          (CREATE_CM_OBJECT_ID (EObjNameSpaceOem, ObjectId))

/** This macro returns a Configuration Manager Object ID
    in the X64 Object Namespace.

  @param [in] ObjectId    The Object ID.

  @retval Returns X64 Configuration Manager Object ID.
**/
#define CREATE_CM_X64_OBJECT_ID(ObjectId) \
          (CREATE_CM_OBJECT_ID (EObjNameSpaceX64, ObjectId))

#endif // CONFIGURATION_MANAGER_OBJECT_H_
