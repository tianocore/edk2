/** @file
  AML Test Helper utilities for validating generated SSDT CPU Topology tables.

  Provides utilities to parse and validate AML output against ACPI 6.6
  specification requirements.

  Copyright (c) 2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.6 Specification - s8.4 Declaring Processors
    - ACPI 6.6 Specification - s8.4.7.1 _CPC (Continuous Performance Control)
    - ACPI 6.6 Specification - s8.4.4 _LPI (Low Power Idle States)
    - ACPI 6.6 Specification - s8.4.5.5 _PSD (P-State Dependency)
    - ACPI 6.6 Specification - s8.4.1.1 _CST (C-State)
    - ACPI 6.6 Specification - s8.4.1.2 _CSD (C-State Dependency)
    - ACPI 6.6 Specification - s8.4.5.1 _PCT (Performance Control)
    - ACPI 6.6 Specification - s8.4.5.2 _PSS (Performance Supported States)
    - ACPI 6.6 Specification - s8.4.5.3 _PPC (Performance Present Capabilities)
**/

#ifndef AML_TEST_HELPER_H_
#define AML_TEST_HELPER_H_

#include <gtest/gtest.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/DebugLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include <Library/AmlLib/AmlLib.h>
  #include <IndustryStandard/AcpiAml.h>
  #include <AcpiObjects.h>
  #include <ArchCommonNameSpaceObjects.h>
}

// Forward declare internal AML types we need
// Note: These are partial definitions - we only include fields we access
// WARNING: These enum values MUST match the real EAML_NODE_TYPE!
// See DynamicTablesPkg/Library/Common/AmlLib/AmlDefines.h
typedef enum {
  EAmlNodeUnknown = 0,
  EAmlNodeRoot    = 1,
  EAmlNodeObject  = 2,
  EAmlNodeData    = 3,
} EAML_NODE_TYPE;

typedef struct AML_NODE_HEADER {
  // WARNING: These fields must match the real AML_NODE_HEADER layout!
  // See DynamicTablesPkg/Library/Common/AmlLib/AmlNodeDefines.h
  LIST_ENTRY                Link;           // Must be first field
  struct AML_NODE_HEADER    *Parent;        // Second field
  EAML_NODE_TYPE            NodeType;       // Third field - what we actually need
  // Other fields not declared (we don't access them)
} AML_NODE_HEADER;

typedef struct AML_OBJECT_NODE {
  AML_NODE_HEADER    Header;
  // Other fields not declared
} AML_OBJECT_NODE;

typedef enum {
  EAmlNodeDataTypeReserved = 0,
  // Add other types as needed
} EAML_NODE_DATA_TYPE;

typedef struct AML_DATA_NODE {
  AML_NODE_HEADER        Header;
  EAML_NODE_DATA_TYPE    DataType; // Missing field causing wrong offsets!
  UINT8                  *Buffer;
  UINT32                 Size;
  // Other fields not declared
} AML_DATA_NODE;

// Node type check macros
#define IS_AML_OBJECT_NODE(Node)  (((Node) != NULL) && ((((AML_NODE_HEADER *)(Node))->NodeType) == EAmlNodeObject))
#define IS_AML_DATA_NODE(Node)    (((Node) != NULL) && ((((AML_NODE_HEADER *)(Node))->NodeType) == EAmlNodeData))

// Parse index for fixed arguments
typedef enum {
  EAmlParseIndexTerm0 = 0,
  EAmlParseIndexTerm1 = 1,
} EAML_PARSE_INDEX;

// =============================================================================
// ACPI 6.6 Specification Constants
// =============================================================================

// ACPI 6.6 s8.4 - HID for processor device
#define ACPI_HID_PROCESSOR_DEVICE_STR  "ACPI0007"
// ACPI 6.6 s8.4 - HID for processor container device
#define ACPI_HID_PROCESSOR_CONTAINER_STR  "ACPI0010"

// ACPI 6.6 s8.4.7.1 - _CPC revision 3 has 23 entries
#define ACPI_CPC_REVISION_3              3u
#define ACPI_CPC_REVISION_3_NUM_ENTRIES  23u

// ACPI 6.6 s8.4.5.5 - _PSD package format
#define ACPI_PSD_NUM_ENTRIES  5u
#define ACPI_PSD_REVISION     0u

// ACPI 6.6 s8.4.5.5 - Coordination types
#define ACPI_COORD_TYPE_SW_ALL  0xFCu
#define ACPI_COORD_TYPE_SW_ANY  0xFDu
#define ACPI_COORD_TYPE_HW_ALL  0xFEu

// ACPI 6.6 s8.4.1.1 - _CST package format (4 fields per C-state entry)
#define ACPI_CST_ENTRY_FIELD_COUNT  4

// ACPI 6.6 s8.4.1.2 - _CSD package format
#define ACPI_CSD_NUM_ENTRIES_REV0  6u
#define ACPI_CSD_REVISION          0u

// ACPI 6.6 s8.4.5.2 - _PSS package format (6 fields per P-state entry)
#define ACPI_PSS_ENTRY_FIELD_COUNT  6

// ACPI 6.6 s8.4.5.1 - _PCT package format (2 registers)
#define ACPI_PCT_ENTRY_COUNT  2

// ACPI 6.6 s8.4.4 - _LPI package header fields (Revision, Level, Count)
#define ACPI_LPI_HEADER_FIELD_COUNT  3

// ARM CoreSight specification - ETE/ETM HID
#define ARM_HID_ET_DEVICE_STR  "ARMHC500"

/**
  Forward declarations for internal AmlLib functions we need.
  These are not part of the public API but necessary for test validation.
**/
extern "C" {
  AML_NODE_HEADER *
  EFIAPI
  AmlGetFixedArgument (
    IN  AML_OBJECT_NODE   *ObjectNode,
    IN  EAML_PARSE_INDEX  Index
    );

  AML_NODE_HEADER *
  EFIAPI
  AmlGetNextVariableArgument (
    IN  AML_NODE_HEADER  *Node,
    IN  AML_NODE_HEADER  *CurrVarArg
    );

  CHAR8 *
  EFIAPI
  AmlNodeGetName (
    IN  CONST AML_OBJECT_NODE  *ObjectNode
    );

  BOOLEAN
  EFIAPI
  AmlNodeCompareOpCode (
    IN  CONST  AML_OBJECT_NODE  *ObjectNode,
    IN         UINT8            OpCode,
    IN         UINT8            SubOpCode
    );
}

/**
  Callback function type for ForEachCpuDevice.
**/
typedef VOID (*CpuDeviceCallback)(
  AML_NODE_HANDLE  CpuDevice,
  UINT32           Index,
  VOID             *Context
  );

/**
  AML Test Helper class for validating generated SSDT tables.
**/
class AmlTestHelper {
public:

  /**
    Parse an SSDT table into an AML tree.

    @param[in]  Table     Pointer to the SSDT table.
    @param[out] RootNode  On success, the root node of the parsed AML tree.

    @retval EFI_SUCCESS   The table was successfully parsed.
    @retval Other         An error occurred during parsing.
  **/
  static EFI_STATUS
  ParseSsdtTable (
    IN  EFI_ACPI_DESCRIPTION_HEADER  *Table,
    OUT AML_ROOT_NODE_HANDLE         *RootNode
    );

  /**
    Helper to manually find a device in a scope by traversing children.

    @param[in]  ScopeNode    The scope node to search in.
    @param[in]  DeviceName   The 4-character device name (e.g., "C000").
    @param[out] OutNode      On success, the found device node.

    @retval EFI_SUCCESS   Device was found.
    @retval EFI_NOT_FOUND Device was not found.
  **/
  static EFI_STATUS
  FindDeviceInScope (
    IN  AML_NODE_HANDLE  ScopeNode,
    IN  CONST CHAR8      *DeviceName,
    OUT AML_NODE_HANDLE  *OutNode
    );

  /**
    Find a device node by its ASL path using manual traversal.

    @param[in]  RootNode  The root node of the AML tree.
    @param[in]  AslPath   The ASL path to search for (e.g., "\\_SB.C000").
    @param[out] OutNode   On success, the found node.

    @retval EFI_SUCCESS   The node was found.
    @retval Other         The node was not found or an error occurred.
  **/
  static EFI_STATUS
  FindDeviceByPath (
    IN  AML_ROOT_NODE_HANDLE  RootNode,
    IN  CONST CHAR8           *AslPath,
    OUT AML_NODE_HANDLE       *OutNode
    );

  /**
    Check if a node is a Device node.

    @param[in] Node  The node to check.

    @retval TRUE   The node is a Device.
    @retval FALSE  The node is not a Device.
  **/
  static BOOLEAN
  IsDeviceNode (
    IN  AML_NODE_HANDLE  Node
    );

  /**
    Check if a node is a Name() node.

    @param[in] Node  The node to check.

    @retval TRUE   The node is a Name statement.
    @retval FALSE  The node is not a Name statement.
  **/
  static BOOLEAN
  IsNameNode (
    IN  AML_NODE_HANDLE  Node
    );

  /**
    Find a named object within a device scope.

    @param[in]  DeviceNode  The device node to search within.
    @param[in]  Name        The 4-character name to find (e.g., "_HID", "_UID").
    @param[out] OutNode     On success, the found Name node.

    @retval EFI_SUCCESS     The named object was found.
    @retval EFI_NOT_FOUND   The named object was not found.
  **/
  static EFI_STATUS
  FindNamedObjectInDevice (
    IN  AML_NODE_HANDLE  DeviceNode,
    IN  CONST CHAR8      *Name,
    OUT AML_NODE_HANDLE  *OutNode
    );

  /**
    Get the string value from a Name(_HID, "string") statement.

    @param[in]  DeviceNode  The device node containing the _HID.
    @param[out] HidString   Buffer to receive the HID string (at least 9 bytes).
    @param[in]  BufferSize  Size of the HidString buffer.

    @retval EFI_SUCCESS     The HID was found and copied.
    @retval EFI_NOT_FOUND   _HID not found in device.
  **/
  static EFI_STATUS
  GetDeviceHid (
    IN  AML_NODE_HANDLE  DeviceNode,
    OUT CHAR8            *HidString,
    IN  UINTN            BufferSize
    );

  /**
    Get the integer value from a Name(_UID, integer) statement.

    @param[in]  DeviceNode  The device node containing the _UID.
    @param[out] Uid         The _UID value.

    @retval EFI_SUCCESS     The _UID was found.
    @retval EFI_NOT_FOUND   _UID not found in device.
  **/
  static EFI_STATUS
  GetDeviceUid (
    IN  AML_NODE_HANDLE  DeviceNode,
    OUT UINT64           *Uid
    );

  /**
    Check if a device has a named object (Method or Name).

    @param[in] DeviceNode  The device node to check.
    @param[in] Name        The 4-character name to find (e.g., "_CPC", "_LPI").

    @retval TRUE   The named object exists.
    @retval FALSE  The named object does not exist.
  **/
  static BOOLEAN
  DeviceHasNamedObject (
    IN  AML_NODE_HANDLE  DeviceNode,
    IN  CONST CHAR8      *Name
    );

  /**
    Check if a device has a specific _HID value.

    @param[in] DeviceNode  The device node to check.
    @param[in] HidValue    The HID string to match (e.g., "ACPI0007").

    @retval TRUE   The device has the specified HID.
    @retval FALSE  The device does not have the specified HID.
  **/
  static BOOLEAN
  DeviceHasHid (
    IN  AML_NODE_HANDLE  DeviceNode,
    IN  CONST CHAR8      *HidValue
    );

  /**
    Count the number of CPU device nodes under a scope.

    @param[in]  RootNode   The root node of the AML tree.
    @param[in]  ScopePath  The scope path (e.g., "\\_SB").
    @param[out] Count      The number of CPU devices found.

    @retval EFI_SUCCESS    The count was determined.
    @retval Other          An error occurred.
  **/
  static EFI_STATUS
  CountCpuDevices (
    IN  AML_ROOT_NODE_HANDLE  RootNode,
    IN  CONST CHAR8           *ScopePath,
    OUT UINT32                *Count
    );

  /**
    Get the name of a device node.

    @param[in]  DeviceNode  The device node.
    @param[out] NameBuffer  Buffer to receive the name (at least 5 bytes).
    @param[in]  BufferSize  Size of the buffer.

    @retval EFI_SUCCESS     The name was retrieved.
    @retval Other           An error occurred.
  **/
  static EFI_STATUS
  GetDeviceName (
    IN  AML_NODE_HANDLE  DeviceNode,
    OUT CHAR8            *NameBuffer,
    IN  UINTN            BufferSize
    );

  /**
    Delete an AML tree and free resources.

    @param[in] RootNode  The root node to delete.
  **/
  static VOID
  DeleteAmlTree (
    IN  AML_ROOT_NODE_HANDLE  RootNode
    );

  // =============================================================================
  // Package Node Operations
  // =============================================================================

  /**
    Check if a node is a Package node.

    @param[in] Node  The node to check.

    @retval TRUE   The node is a Package.
    @retval FALSE  The node is not a Package.
  **/
  static BOOLEAN
  IsPackageNode (
    IN  AML_NODE_HANDLE  Node
    );

  /**
    Check if a node is a Method node.

    @param[in] Node  The node to check.

    @retval TRUE   The node is a Method.
    @retval FALSE  The node is not a Method.
  **/
  static BOOLEAN
  IsMethodNode (
    IN  AML_NODE_HANDLE  Node
    );

  /**
    Count the number of elements in an AML Package.

    @param[in]  PackageNode  The Package node.
    @param[out] Count        The number of elements.

    @retval EFI_SUCCESS      Count was determined.
    @retval EFI_INVALID_PARAMETER  Not a Package node.
  **/
  static EFI_STATUS
  GetPackageElementCount (
    IN  AML_NODE_HANDLE  PackageNode,
    OUT UINT32           *Count
    );

  /**
    Get a specific element from an AML Package by index.

    @param[in]  PackageNode  The Package node.
    @param[in]  Index        Zero-based index of element to retrieve.
    @param[out] Element      The element node at that index.

    @retval EFI_SUCCESS      Element was found.
    @retval EFI_NOT_FOUND    Index out of bounds.
  **/
  static EFI_STATUS
  GetPackageElement (
    IN  AML_NODE_HANDLE  PackageNode,
    IN  UINT32           Index,
    OUT AML_NODE_HANDLE  *Element
    );

  /**
    Extract an integer value from an AML integer node.

    @param[in]  Node   The integer node (data node or integer opcode).
    @param[out] Value  The extracted integer value.

    @retval EFI_SUCCESS       Value extracted.
    @retval EFI_UNSUPPORTED   Node is not an integer type.
  **/
  static EFI_STATUS
  GetIntegerValue (
    IN  AML_NODE_HANDLE  Node,
    OUT UINT64           *Value
    );

  /**
    Find a named object (Name or Method) in a device and return the value node.

    @param[in]  DeviceNode  The device node to search within.
    @param[in]  Name        The 4-character name to find.
    @param[out] ValueNode   On success, the value/body node.

    @retval EFI_SUCCESS     Object found, ValueNode set.
    @retval EFI_NOT_FOUND   Object not found.
  **/
  static EFI_STATUS
  FindNamedObjectValue (
    IN  AML_NODE_HANDLE  DeviceNode,
    IN  CONST CHAR8      *Name,
    OUT AML_NODE_HANDLE  *ValueNode
    );

  // =============================================================================
  // _CPC Validation per ACPI 6.6 s8.4.7.1
  // =============================================================================

  /**
    @brief Validate _CPC package structure per ACPI 6.6 s8.4.7.1.

    ACPI 6.6 s8.4.7.1 specifies:
    - "For CPC revision 3, the package contains 23 entries"
    - Entry order is fixed: NumEntries, Revision, HighestPerformance, ...
    - Required fields must not be NULL resource
    - Integer fields use integer encoding if buffer is NULL resource

    @param[in] DeviceNode   The CPU device node containing _CPC.
    @param[out] EntryCount  Number of entries in the _CPC package.

    @retval EFI_SUCCESS     _CPC found and basic structure is valid.
    @retval EFI_NOT_FOUND   _CPC not present.
    @retval Other           Validation failed.
  **/
  static EFI_STATUS
  ValidateCpcPackageStructure (
    IN  AML_NODE_HANDLE  DeviceNode,
    OUT UINT32           *EntryCount
    );

  /**
    @brief Validate _CPC revision field per ACPI 6.6 s8.4.7.1.

    @param[in] DeviceNode  The CPU device node containing _CPC.
    @param[out] Revision   The revision value from _CPC[1].

    @retval EFI_SUCCESS    Revision extracted.
    @retval Other          Error or not found.
  **/
  static EFI_STATUS
  GetCpcRevision (
    IN  AML_NODE_HANDLE  DeviceNode,
    OUT UINT32           *Revision
    );

  // =============================================================================
  // _PSD Validation per ACPI 6.6 s8.4.5.5
  // =============================================================================

  /**
    @brief Validate _PSD package structure per ACPI 6.6 s8.4.5.5.

    ACPI 6.6 s8.4.5.5 specifies:
    - Package of packages, outer package has 1 element (the inner dependency package)
    - Inner package: { NumEntries, Revision, Domain, CoordType, NumProcessors }
    - NumEntries should be 5, Revision should be 0

    @param[in] DeviceNode   The CPU device node containing _PSD.
    @param[out] OuterCount  Number of entries in outer package.

    @retval EFI_SUCCESS     _PSD found and basic structure is valid.
    @retval EFI_NOT_FOUND   _PSD not present.
  **/
  static EFI_STATUS
  ValidatePsdPackageStructure (
    IN  AML_NODE_HANDLE  DeviceNode,
    OUT UINT32           *OuterCount
    );

  /**
    @brief Extract _PSD inner package fields per ACPI 6.6 s8.4.5.5.

    @param[in]  DeviceNode    The CPU device node containing _PSD.
    @param[out] NumEntries    Value of NumEntries field.
    @param[out] Revision      Value of Revision field.
    @param[out] Domain        Value of Domain field.
    @param[out] CoordType     Value of CoordType field.
    @param[out] NumProcessors Value of NumProcessors field.

    @retval EFI_SUCCESS     Fields extracted.
    @retval Other           Error or not found.
  **/
  static EFI_STATUS
  GetPsdFields (
    IN  AML_NODE_HANDLE  DeviceNode,
    OUT UINT32           *NumEntries,
    OUT UINT32           *Revision,
    OUT UINT32           *Domain,
    OUT UINT32           *CoordType,
    OUT UINT32           *NumProcessors
    );

  // =============================================================================
  // _LPI Validation per ACPI 6.6 s8.4.4
  // =============================================================================

  /**
    @brief Validate _LPI structure per ACPI 6.6 s8.4.4.

    ACPI 6.6 s8.4.4 specifies _LPI returns a package:
    - Package { Revision, LevelId, Count, LpiState0, LpiState1, ... }
    - Each LpiState is a package with min 10 entries

    @param[in]  DeviceNode  The device node containing _LPI.
    @param[out] Revision    The LPI revision.
    @param[out] LevelId     The level ID.
    @param[out] StateCount  Number of LPI states.

    @retval EFI_SUCCESS     _LPI found and header parsed.
    @retval EFI_NOT_FOUND   _LPI not present.
  **/
  static EFI_STATUS
  GetLpiHeader (
    IN  AML_NODE_HANDLE  DeviceNode,
    OUT UINT32           *Revision,
    OUT UINT32           *LevelId,
    OUT UINT32           *StateCount
    );

  // =============================================================================
  // _CST Validation per ACPI 6.6 s8.4.1.1
  // =============================================================================

  /**
    @brief Validate _CST structure per ACPI 6.6 s8.4.1.1.

    ACPI 6.6 s8.4.1.1 specifies _CST returns a package:
    - Package { Count, CState1, CState2, ... }
    - Each CState is a package { Register, Type, Latency, Power }

    @param[in]  DeviceNode  The device node containing _CST.
    @param[out] StateCount  Number of C-states reported.

    @retval EFI_SUCCESS     _CST found and count extracted.
    @retval EFI_NOT_FOUND   _CST not present.
  **/
  static EFI_STATUS
  GetCstStateCount (
    IN  AML_NODE_HANDLE  DeviceNode,
    OUT UINT32           *StateCount
    );

  /**
    @brief Get a specific C-state entry from _CST per ACPI 6.6 s8.4.1.1.

    @param[in]  DeviceNode  The device node containing _CST.
    @param[in]  StateIndex  Zero-based index of C-state (0 = first C-state).
    @param[out] Type        C-state type (1=C1, 2=C2, etc.).
    @param[out] Latency     Entry/exit latency in microseconds.
    @param[out] Power       Power consumption in mW.

    @retval EFI_SUCCESS     C-state entry extracted.
    @retval EFI_NOT_FOUND   State not present.
  **/
  static EFI_STATUS
  GetCstStateEntry (
    IN  AML_NODE_HANDLE  DeviceNode,
    IN  UINT32           StateIndex,
    OUT UINT8            *Type,
    OUT UINT16           *Latency,
    OUT UINT32           *Power
    );

  // =============================================================================
  // _CSD Validation per ACPI 6.6 s8.4.1.2
  // =============================================================================

  /**
    @brief Extract _CSD fields per ACPI 6.6 s8.4.1.2.

    @param[in]  DeviceNode    The device node containing _CSD.
    @param[out] NumEntries    Value of NumEntries field.
    @param[out] Revision      Value of Revision field.
    @param[out] Domain        Value of Domain field.
    @param[out] CoordType     Value of CoordType field.
    @param[out] NumProcessors Value of NumProcessors field.
    @param[out] Index         Value of Index field (C-state index).

    @retval EFI_SUCCESS     Fields extracted.
    @retval EFI_NOT_FOUND   _CSD not present.
  **/
  static EFI_STATUS
  GetCsdFields (
    IN  AML_NODE_HANDLE  DeviceNode,
    OUT UINT32           *NumEntries,
    OUT UINT32           *Revision,
    OUT UINT32           *Domain,
    OUT UINT32           *CoordType,
    OUT UINT32           *NumProcessors,
    OUT UINT32           *Index
    );

  // =============================================================================
  // _PSS Validation per ACPI 6.6 s8.4.5.2
  // =============================================================================

  /**
    @brief Validate _PSS structure per ACPI 6.6 s8.4.5.2.

    ACPI 6.6 s8.4.5.2 specifies _PSS returns a package:
    - Package { PState0, PState1, ... }
    - Each PState is a package { CoreFreq, Power, Latency, BusMasterLatency, Control, Status }

    @param[in]  DeviceNode  The device node containing _PSS.
    @param[out] StateCount  Number of P-states.

    @retval EFI_SUCCESS     _PSS found and count determined.
    @retval EFI_NOT_FOUND   _PSS not present.
  **/
  static EFI_STATUS
  GetPssStateCount (
    IN  AML_NODE_HANDLE  DeviceNode,
    OUT UINT32           *StateCount
    );

  /**
    @brief Get a specific P-state entry from _PSS per ACPI 6.6 s8.4.5.2.

    @param[in]  DeviceNode  The device node containing _PSS.
    @param[in]  StateIndex  Zero-based index of P-state.
    @param[out] CoreFreq    Core frequency in MHz.
    @param[out] Power       Power dissipation in mW.
    @param[out] Latency     Transition latency in us.
    @param[out] BusMasterLatency  Bus master latency in us.
    @param[out] Control     Value to write to control register.
    @param[out] StatusVal   Expected value from status register.

    @retval EFI_SUCCESS     P-state entry extracted.
    @retval EFI_NOT_FOUND   State not present.
  **/
  static EFI_STATUS
  GetPssStateEntry (
    IN  AML_NODE_HANDLE  DeviceNode,
    IN  UINT32           StateIndex,
    OUT UINT32           *CoreFreq,
    OUT UINT32           *Power,
    OUT UINT32           *Latency,
    OUT UINT32           *BusMasterLatency,
    OUT UINT32           *Control,
    OUT UINT32           *StatusVal
    );

  // =============================================================================
  // _PPC Validation per ACPI 6.6 s8.4.5.3
  // =============================================================================

  /**
    @brief Get _PPC value per ACPI 6.6 s8.4.5.3.

    @param[in]  DeviceNode  The device node containing _PPC.
    @param[out] PpcValue    The performance present capabilities limit.

    @retval EFI_SUCCESS     _PPC found and value extracted.
    @retval EFI_NOT_FOUND   _PPC not present.
  **/
  static EFI_STATUS
  GetPpcValue (
    IN  AML_NODE_HANDLE  DeviceNode,
    OUT UINT32           *PpcValue
    );

  // =============================================================================
  // _STA Validation per ACPI 6.6 s6.3.7
  // =============================================================================

  /**
    @brief Check if device has _STA method per ACPI 6.6 s6.3.7.

    @param[in] DeviceNode  The device node.

    @retval TRUE   _STA is present.
    @retval FALSE  _STA is not present.
  **/
  static BOOLEAN
  DeviceHasSta (
    IN  AML_NODE_HANDLE  DeviceNode
    );

  // =============================================================================
  // Hierarchy Traversal Helpers
  // =============================================================================

  /**
    @brief Count container devices (ACPI0010) under a scope.

    @param[in]  RootNode   The root of the AML tree.
    @param[in]  ScopePath  The scope path to search.
    @param[out] Count      Number of container devices found.

    @retval EFI_SUCCESS    Count determined.
    @retval Other          Error.
  **/
  static EFI_STATUS
  CountContainerDevices (
    IN  AML_ROOT_NODE_HANDLE  RootNode,
    IN  CONST CHAR8           *ScopePath,
    OUT UINT32                *Count
    );

  /**
    @brief Find all CPU devices and invoke callback for each.

    @param[in] RootNode   The root of the AML tree.
    @param[in] ScopePath  The scope path to search.
    @param[in] Callback   Function to call for each CPU device found.
    @param[in] Context    Context to pass to callback.

    @retval EFI_SUCCESS   All CPUs processed.
  **/
  static EFI_STATUS
  ForEachCpuDevice (
    IN  AML_ROOT_NODE_HANDLE  RootNode,
    IN  CONST CHAR8           *ScopePath,
    IN  CpuDeviceCallback     Callback,
    IN  VOID                  *Context
    );
};

#endif // AML_TEST_HELPER_H_
