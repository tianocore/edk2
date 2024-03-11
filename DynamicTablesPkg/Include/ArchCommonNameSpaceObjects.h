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
  EArchCommonObjPciAddressMapInfo,              ///<  9 - Pci Address Map Info
  EArchCommonObjPciInterruptMapInfo,            ///< 10 - Pci Interrupt Map Info
  EArchCommonObjMemoryAffinityInfo,             ///< 11 - Memory Affinity Info
  EArchCommonObjDeviceHandleAcpi,               ///< 12 - Device Handle Acpi
  EArchCommonObjDeviceHandlePci,                ///< 13 - Device Handle Pci
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

/** A structure that describes a PCI Address Map.

  The memory-ranges used by the PCI bus are described by this object.

  ID: EArchCommonObjPciAddressMapInfo
*/
typedef struct CmArchCommonPciAddressMapInfo {
  /** Pci address space code

  Available values are:
   - 0: Configuration Space
   - 1: I/O Space
   - 2: 32-bit-address Memory Space
   - 3: 64-bit-address Memory Space
  */
  UINT8     SpaceCode;

  /// PCI address
  UINT64    PciAddress;

  /// Cpu address
  UINT64    CpuAddress;

  /// Address size
  UINT64    AddressSize;
} CM_ARCH_COMMON_PCI_ADDRESS_MAP_INFO;

/** A structure that describes the
    Generic Interrupts.
*/
typedef struct CmArchCommonGenericInterrupt {
  /// Interrupt number
  UINT32    Interrupt;

  /// Flags
  /// BIT0: 0: Interrupt is Level triggered
  ///       1: Interrupt is Edge triggered
  /// BIT1: 0: Interrupt is Active high
  ///       1: Interrupt is Active low
  UINT32    Flags;
} CM_ARCH_COMMON_GENERIC_INTERRUPT;

/** A structure that describes a PCI Interrupt Map.

  The legacy PCI interrupts used by PCI devices are described by this object.

  Cf Devicetree Specification - Release v0.3
  s2.4.3 "Interrupt Nexus Properties"

  ID: EArchCommonObjPciInterruptMapInfo
*/
typedef struct CmArchCommonPciInterruptMapInfo {
  /// Pci Bus.
  /// Value on 8 bits (max 255).
  UINT8    PciBus;

  /// Pci Device.
  /// Value on 5 bits (max 31).
  UINT8    PciDevice;

  /** PCI interrupt

  ACPI bindings are used:
  Cf. ACPI 6.4, s6.2.13 _PRT (PCI Routing Table):
      "0-INTA, 1-INTB, 2-INTC, 3-INTD"

  Device-tree bindings are shifted by 1:
      "INTA=1, INTB=2, INTC=3, INTD=4"
  */
  UINT8                               PciInterrupt;

  /** Interrupt controller interrupt.

  Cf Devicetree Specification - Release v0.3
  s2.4.3 "Interrupt Nexus Properties": "parent interrupt specifier"
  */
  CM_ARCH_COMMON_GENERIC_INTERRUPT    IntcInterrupt;
} CM_ARCH_COMMON_PCI_INTERRUPT_MAP_INFO;

/** A structure that describes the Memory Affinity Structure (Type 1) in SRAT

    ID: EArchCommonObjMemoryAffinityInfo
*/
typedef struct CmArchCommonMemoryAffinityInfo {
  /// The proximity domain to which the "range of memory" belongs.
  UINT32    ProximityDomain;

  /// Base Address
  UINT64    BaseAddress;

  /// Length
  UINT64    Length;

  /// Flags
  UINT32    Flags;
} CM_ARCH_COMMON_MEMORY_AFFINITY_INFO;

/** A structure that describes the ACPI Device Handle (Type 0) in the
    Generic Initiator Affinity structure in SRAT

    ID: EArchCommonObjDeviceHandleAcpi
*/
typedef struct CmArchCommonDeviceHandleAcpi {
  /// Hardware ID
  UINT64    Hid;

  /// Unique Id
  UINT32    Uid;
} CM_ARCH_COMMON_DEVICE_HANDLE_ACPI;

/** A structure that describes the PCI Device Handle (Type 1) in the
    Generic Initiator Affinity structure in SRAT

    ID: EArchCommonObjDeviceHandlePci
*/
typedef struct CmArchCommonDeviceHandlePci {
  /// PCI Segment Number
  UINT16    SegmentNumber;

  /// PCI Bus Number - Max 256 busses (Bits 15:8 of BDF)
  UINT8     BusNumber;

  /// PCI Device Number - Max 32 devices (Bits 7:3 of BDF)
  UINT8     DeviceNumber;

  /// PCI Function Number - Max 8 functions (Bits 2:0 of BDF)
  UINT8     FunctionNumber;
} CM_ARCH_COMMON_DEVICE_HANDLE_PCI;

#pragma pack()

#endif // ARCH_COMMON_NAMESPACE_OBJECTS_H_
