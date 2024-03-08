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
  EArchCommonObjSerialPortInfo,                 ///<  2 - Generic Serial Port Info
  EArchCommonObjConsolePortInfo,                ///<  3 - Serial Console Port Info
  EArchCommonObjSerialDebugPortInfo,            ///<  4 - Serial Debug Port Info
  EArchCommonObjHypervisorVendorIdentity,       ///<  5 - Hypervisor Vendor Id
  EArchCommonObjFixedFeatureFlags,              ///<  6 - Fixed feature flags for FADT
  EArchCommonObjCmRef,                          ///<  7 - CM Object Reference
  EArchCommonObjPciConfigSpaceInfo,             ///<  8 - PCI Configuration Space Info
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

/** A structure that describes the
    Serial Port information for the Platform.

    ID: EArchCommonObjConsolePortInfo or
        EArchCommonObjSerialDebugPortInfo or
        EArchCommonObjSerialPortInfo
*/
typedef struct EArchCommonSerialPortInfo {
  /// The physical base address for the serial port
  UINT64    BaseAddress;

  /** The serial port interrupt.
      0 indicates that the serial port does not
      have an interrupt wired.
  */
  UINT32    Interrupt;

  /// The serial port baud rate
  UINT64    BaudRate;

  /// The serial port clock
  UINT32    Clock;

  /// Serial Port subtype
  UINT16    PortSubtype;

  /// The Base address length
  UINT64    BaseAddressLength;

  /// The access size
  UINT8     AccessSize;
} CM_ARCH_COMMON_SERIAL_PORT_INFO;

/** A structure that describes the
    Hypervisor Vendor ID information for the Platform.

    ID: EArchCommonObjHypervisorVendorIdentity
*/
typedef struct CmArchCommonHypervisorVendorIdentity {
  /// The hypervisor Vendor ID
  UINT64    HypervisorVendorId;
} CM_ARCH_COMMON_HYPERVISOR_VENDOR_ID;

/** A structure that describes the
    Fixed feature flags for the Platform.

    ID: EArchCommonObjFixedFeatureFlags
*/
typedef struct CmArchCommonFixedFeatureFlags {
  /// The Fixed feature flags
  UINT32    Flags;
} CM_ARCH_COMMON_FIXED_FEATURE_FLAGS;

/** A structure that describes a reference to another Configuration Manager
    object.

    This is useful for creating an array of reference tokens. The framework
    can then query the configuration manager for these arrays using the
    object ID EArchCommonObjCmRef.

    This can be used is to represent one-to-many relationships between objects.

    ID: EArchCommonObjCmRef
*/
typedef struct CmArchCommonObjRef {
  /// Token of the CM object being referenced
  CM_OBJECT_TOKEN    ReferenceToken;
} CM_ARCH_COMMON_OBJ_REF;

/** A structure that describes the
    PCI Configuration Space information for the Platform.

    ID: EArchCommonObjPciConfigSpaceInfo
*/
typedef struct CmArchCommonPciConfigSpaceInfo {
  /// The physical base address for the PCI segment
  UINT64             BaseAddress;

  /// The PCI segment group number
  UINT16             PciSegmentGroupNumber;

  /// The start bus number
  UINT8              StartBusNumber;

  /// The end bus number
  UINT8              EndBusNumber;

  /// Optional field: Reference Token for address mapping.
  /// Token identifying a CM_ARCH_COMMON_OBJ_REF structure.
  CM_OBJECT_TOKEN    AddressMapToken;

  /// Optional field: Reference Token for interrupt mapping.
  /// Token identifying a CM_ARCH_COMMON_OBJ_REF structure.
  CM_OBJECT_TOKEN    InterruptMapToken;
} CM_ARCH_COMMON_PCI_CONFIG_SPACE_INFO;

#pragma pack()

#endif // ARCH_COMMON_NAMESPACE_OBJECTS_H_
