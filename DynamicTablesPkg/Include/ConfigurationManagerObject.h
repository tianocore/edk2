/** @file

  Copyright (c) 2017 - 2022, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#ifndef CONFIGURATION_MANAGER_OBJECT_H_
#define CONFIGURATION_MANAGER_OBJECT_H_

#include <ArmNameSpaceObjects.h>
#include <StandardNameSpaceObjects.h>

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
                0001 - ARM
                1000 - Custom/OEM
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

Object ID's in the ARM Namespace:
   0 - Reserved
   1 - Boot Architecture Info
   2 - CPU Info
   3 - Power Management Profile Info
   4 - GICC Info
   5 - GICD Info
   6 - GIC MSI Frame Info
   7 - GIC Redistributor Info
   8 - GIC ITS Info
   9 - Serial Console Port Info
  10 - Serial Debug Port Info
  11 - Generic Timer Info
  12 - Platform GT Block Info
  13 - Generic Timer Block Frame Info
  14 - Platform Generic Watchdog
  15 - PCI Configuration Space Info
  16 - Hypervisor Vendor Id
  17 - Fixed feature flags for FADT
  18 - ITS Group
  19 - Named Component
  20 - Root Complex
  21 - SMMUv1 or SMMUv2
  22 - SMMUv3
  23 - PMCG
  24 - GIC ITS Identifier Array
  25 - ID Mapping Array
  26 - SMMU Interrupt Array
  27 - Processor Hierarchy Info
  28 - Cache Info
  29 - Processor Hierarchy Node ID Info
  30 - CM Object Reference
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
  EObjNameSpaceStandard,      ///< Standard Objects Namespace
  EObjNameSpaceArm,           ///< ARM Objects Namespace
  EObjNameSpaceOem = 0x8,     ///< OEM Objects Namespace
  EObjNameSpaceMax
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
          ((((NameSpaceId) & NAMESPACE_ID_MASK) << NAMESPACE_ID_BIT_SHIFT) | \
            ((ObjectId) & OBJECT_ID_MASK))

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
    in the OEM Object Namespace.

  @param [in] ObjectId    The Object ID.

  @retval Returns an OEM Configuration Manager Object ID.
**/
#define CREATE_CM_OEM_OBJECT_ID(ObjectId) \
          (CREATE_CM_OBJECT_ID (EObjNameSpaceOem, ObjectId))

#endif // CONFIGURATION_MANAGER_OBJECT_H_
