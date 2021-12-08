/** @file
  The EFI Legacy BIOS Platform Protocol is used to mate a Legacy16
  implementation with this EFI code. The EFI driver that produces
  the Legacy BIOS protocol is generic and consumes this protocol.
  A driver that matches the Legacy16 produces this protocol

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  This protocol is defined in Framework for EFI Compatibility Support Module spec
  Version 0.97.

**/

#ifndef _EFI_LEGACY_BIOS_PLATFORM_H_
#define _EFI_LEGACY_BIOS_PLATFORM_H_

///
/// Legacy BIOS Platform depends on HDD_INFO and EFI_COMPATIBILITY16_TABLE that
/// are defined with the Legacy BIOS Protocol
///
#include <Protocol/LegacyBios.h>

#define EFI_LEGACY_BIOS_PLATFORM_PROTOCOL_GUID \
  { \
    0x783658a3, 0x4172, 0x4421, {0xa2, 0x99, 0xe0, 0x9, 0x7, 0x9c, 0xc, 0xb4 } \
  }

typedef struct _EFI_LEGACY_BIOS_PLATFORM_PROTOCOL EFI_LEGACY_BIOS_PLATFORM_PROTOCOL;

/**
  This enum specifies the Mode param values for GetPlatformInfo()
**/
typedef enum {
  ///
  /// This mode is invoked twice. The first invocation has LegacySegment and
  /// LegacyOffset set to 0. The mode returns the MP table address in EFI memory, along with its size.
  /// The second invocation has LegacySegment and LegacyOffset set to the location
  /// in the 0xF0000 or 0xE0000 block to which the MP table is to be copied. The second
  /// invocation allows any MP table address fixes to occur in the EFI memory copy of the
  /// MP table. The caller, not EfiGetPlatformBinaryMpTable, copies the modified MP
  /// table to the allocated region in 0xF0000 or 0xE0000 block after the second invocation.
  ///
  /// The function parameters associated with this mode are:
  ///
  ///   Table Pointer to the MP table.
  ///
  ///   TableSize Size in bytes of the MP table.
  ///
  ///   Location Location to place table. 0x00. Either 0xE0000 or 0xF0000 64 KB blocks.
  ///     Bit 0 = 1 0xF0000 64 KB block.
  ///     Bit 1 = 1 0xE0000 64 KB block.
  ///     Multiple bits can be set.
  ///
  ///   Alignment Bit-mapped address alignment granularity.
  ///     The first nonzero bit from the right is the address granularity.
  ///
  //    LegacySegment Segment in which EfiCompatibility code will place the MP table.
  ///
  ///   LegacyOffset Offset in which EfiCompatibility code will place the MP table.
  ///
  /// The return values associated with this mode are:
  ///
  ///   EFI_SUCCESS The MP table was returned.
  ///
  ///   EFI_UNSUPPORTED The MP table is not supported on this platform.
  ///
  EfiGetPlatformBinaryMpTable = 0,
  ///
  /// This mode returns a block of data. The content and usage is IBV or OEM defined.
  /// OEMs or IBVs normally use this function for nonstandard Compatibility16 runtime soft
  /// INTs. It is the responsibility of this routine to coalesce multiple OEM 16 bit functions, if
  /// they exist, into one coherent package that is understandable by the Compatibility16 code.
  /// This function is invoked twice. The first invocation has LegacySegment and
  /// LegacyOffset set to 0. The function returns the table address in EFI memory, as well as its size.
  /// The second invocation has LegacySegment and LegacyOffset set to the location
  /// in the 0xF0000 or 0xE0000 block to which the data (table) is to be copied. The second
  /// invocation allows any data (table) address fixes to occur in the EFI memory copy of
  /// the table. The caller, not GetOemIntData(), copies the modified data (table) to the
  /// allocated region in 0xF0000 or 0xE0000 block after the second invocation.
  ///
  /// The function parameters associated with this mode are:
  ///
  ///   Table Pointer to OEM legacy 16 bit code or data.
  ///
  ///   TableSize Size of data.
  ///
  ///   Location Location to place table. 0x00. Either 0xE0000 or 0xF0000 64 KB blocks.
  ///       Bit 0 = 1 0xF0000 64 KB block.
  ///       Bit 1 = 1 0xE0000 64 KB block.
  ///       Multiple bits can be set.
  ///
  ///   Alignment Bit mapped address alignment granularity.
  ///     The first nonzero bit from the right is the address granularity.
  ///
  ///   LegacySegment Segment in which EfiCompatibility code will place the table or data.
  ///
  ///   LegacyOffset Offset in which EfiCompatibility code will place the table or data.
  ///
  /// The return values associated with this mode are:
  ///
  ///   EFI_SUCCESS The data was returned successfully.
  ///
  ///   EFI_UNSUPPORTED Oem INT is not supported on this platform.
  ///
  EfiGetPlatformBinaryOemIntData = 1,
  ///
  /// This mode returns a block of data. The content and usage is IBV defined. OEMs or
  /// IBVs normally use this mode for nonstandard Compatibility16 runtime 16 bit routines. It
  /// is the responsibility of this routine to coalesce multiple OEM 16 bit functions, if they
  /// exist, into one coherent package that is understandable by the Compatibility16 code.
  ///
  /// Example usage: A legacy mobile BIOS that has a pre-existing runtime
  /// interface to return the battery status to calling applications.
  ///
  /// This mode is invoked twice. The first invocation has LegacySegment and
  /// LegacyOffset set to 0. The mode returns the table address in EFI memory and its size.
  /// The second invocation has LegacySegment and LegacyOffset set to the location
  /// in the 0xF0000 or 0xE0000 block to which the table is to be copied. The second
  /// invocation allows any table address fixes to occur in the EFI memory copy of the table.
  /// The caller, not EfiGetPlatformBinaryOem16Data, copies the modified table to
  /// the allocated region in 0xF0000 or 0xE0000 block after the second invocation.
  ///
  /// The function parameters associated with this mode are:
  ///
  ///   Table Pointer to OEM legacy 16 bit code or data.
  ///
  ///   TableSize Size of data.
  ///
  ///   Location Location to place the table. 0x00. Either 0xE0000 or 0xF0000 64 KB blocks.
  ///      Bit 0 = 1 0xF0000 64 KB block.
  ///      Bit 1 = 1 0xE0000 64 KB block.
  ///      Multiple bits can be set.
  ///
  ///   Alignment Bit mapped address alignment granularity.
  ///     The first nonzero bit from the right is the address granularity.
  ///
  ///   LegacySegment Segment in which EfiCompatibility code will place the table or data.
  ///
  ///   LegacyOffset Offset in which EfiCompatibility code will place the table or data.
  ///
  /// The return values associated with this mode are:
  ///
  ///   EFI_SUCCESS The data was returned successfully.
  ///
  ///   EFI_UNSUPPORTED Oem16 is not supported on this platform.
  ///
  EfiGetPlatformBinaryOem16Data = 2,
  ///
  /// This mode returns a block of data. The content and usage are IBV defined. OEMs or
  /// IBVs normally use this mode for nonstandard Compatibility16 runtime 32 bit routines. It
  /// is the responsibility of this routine to coalesce multiple OEM 32 bit functions, if they
  /// exist, into one coherent package that is understandable by the Compatibility16 code.
  ///
  /// Example usage: A legacy mobile BIOS that has a pre existing runtime
  /// interface to return the battery status to calling applications.
  ///
  /// This mode is invoked twice. The first invocation has LegacySegment and
  /// LegacyOffset set to 0. The mode returns the table address in EFI memory and its size.
  ///
  /// The second invocation has LegacySegment and LegacyOffset set to the location
  /// in the 0xF0000 or 0xE0000 block to which the table is to be copied. The second
  /// invocation allows any table address fix ups to occur in the EFI memory copy of the table.
  /// The caller, not EfiGetPlatformBinaryOem32Data, copies the modified table to
  /// the allocated region in 0xF0000 or 0xE0000 block after the second invocation..
  ///
  /// Note: There are two generic mechanisms by which this mode can be used.
  /// Mechanism 1: This mode returns the data and the Legacy BIOS Protocol copies
  /// the data into the F0000 or E0000 block in the Compatibility16 code. The
  /// EFI_COMPATIBILITY16_TABLE entries Oem32Segment and Oem32Offset can
  /// be viewed as two UINT16 entries.
  /// Mechanism 2: This mode directly fills in the EFI_COMPATIBILITY16_TABLE with
  /// a pointer to the INT15 E820 region containing the 32 bit code. It returns
  /// EFI_UNSUPPORTED. The EFI_COMPATIBILITY16_TABLE entries,
  /// Oem32Segment and Oem32Offset, can be viewed as two UINT16 entries or
  /// as a single UINT32 entry as determined by the IBV.
  ///
  /// The function parameters associated with this mode are:
  ///
  ///   TableSize Size of data.
  ///
  ///   Location Location to place the table. 0x00 or 0xE0000 or 0xF0000 64 KB blocks.
  ///       Bit 0 = 1 0xF0000 64 KB block.
  ///       Bit 1 = 1 0xE0000 64 KB block.
  ///       Multiple bits can be set.
  ///
  ///   Alignment Bit mapped address alignment granularity.
  ///       The first nonzero bit from the right is the address granularity.
  ///
  ///   LegacySegment Segment in which EfiCompatibility code will place the table or data.
  ///
  ///   LegacyOffset Offset in which EfiCompatibility code will place the table or data.
  ///
  /// The return values associated with this mode are:
  ///   EFI_SUCCESS The data was returned successfully.
  ///   EFI_UNSUPPORTED Oem32 is not supported on this platform.
  ///
  EfiGetPlatformBinaryOem32Data = 3,
  ///
  /// This mode returns a TPM binary image for the onboard TPM device.
  ///
  /// The function parameters associated with this mode are:
  ///
  ///   Table TPM binary image for the onboard TPM device.
  ///
  ///   TableSize Size of BinaryImage in bytes.
  ///
  ///   Location Location to place the table. 0x00. Either 0xE0000 or 0xF0000 64 KB blocks.
  ///      Bit 0 = 1 0xF0000 64 KB block.
  ///      Bit 1 = 1 0xE0000 64 KB block.
  ///      Multiple bits can be set.
  ///
  ///   Alignment Bit mapped address alignment granularity.
  ///     The first nonzero bit from the right is the address granularity.
  ///
  ///   LegacySegment Segment in which EfiCompatibility code will place the table or data.
  ///
  ///   LegacyOffset Offset in which EfiCompatibility code will place the table or data.
  ///
  /// The return values associated with this mode are:
  ///
  ///   EFI_SUCCESS BinaryImage is valid.
  ///
  ///   EFI_UNSUPPORTED Mode is not supported on this platform.
  ///
  ///   EFI_NOT_FOUND No BinaryImage was found.
  ///
  EfiGetPlatformBinaryTpmBinary = 4,
  ///
  /// The mode finds the Compatibility16 Rom Image.
  ///
  /// The function parameters associated with this mode are:
  ///
  ///    System ROM image for the platform.
  ///
  ///    TableSize Size of Table in bytes.
  ///
  ///    Location Ignored.
  ///
  ///    Alignment Ignored.
  ///
  ///    LegacySegment Ignored.
  ///
  ///    LegacyOffset Ignored.
  ///
  /// The return values associated with this mode are:
  ///
  ///    EFI_SUCCESS ROM image found.
  ///
  ///    EFI_NOT_FOUND ROM not found.
  ///
  EfiGetPlatformBinarySystemRom = 5,
  ///
  /// This mode returns the Base address of PciExpress memory mapped configuration
  /// address space.
  ///
  /// The function parameters associated with this mode are:
  ///
  ///    Table System ROM image for the platform.
  ///
  ///    TableSize Size of Table in bytes.
  ///
  ///    Location Ignored.
  ///
  ///    Alignment Ignored.
  ///
  ///    LegacySegment Ignored.
  ///
  ///    LegacyOffset Ignored.
  ///
  /// The return values associated with this mode are:
  ///
  ///   EFI_SUCCESS Address is valid.
  ///
  ///   EFI_UNSUPPORTED System does not PciExpress.
  ///
  EfiGetPlatformPciExpressBase = 6,
  ///
  EfiGetPlatformPmmSize = 7,
  ///
  EfiGetPlatformEndOpromShadowAddr = 8,
  ///
} EFI_GET_PLATFORM_INFO_MODE;

/**
  This enum specifies the Mode param values for GetPlatformHandle().
**/
typedef enum {
  ///
  /// This mode returns the Compatibility16 policy for the device that should be the VGA
  /// controller used during a Compatibility16 boot.
  ///
  /// The function parameters associated with this mode are:
  ///
  ///   Type 0x00.
  ///
  ///   HandleBuffer Buffer of all VGA handles found.
  ///
  ///   HandleCount Number of VGA handles found.
  ///
  ///   AdditionalData NULL.
  ///
  EfiGetPlatformVgaHandle = 0,
  ///
  /// This mode returns the Compatibility16 policy for the device that should be the IDE
  /// controller used during a Compatibility16 boot.
  ///
  /// The function parameters associated with this mode are:
  ///
  ///   Type 0x00.
  ///
  ///   HandleBuffer Buffer of all IDE handles found.
  ///
  ///   HandleCount Number of IDE handles found.
  ///
  ///   AdditionalData Pointer to HddInfo.
  ///     Information about all onboard IDE controllers.
  ///
  EfiGetPlatformIdeHandle = 1,
  ///
  /// This mode returns the Compatibility16 policy for the device that should be the ISA bus
  /// controller used during a Compatibility16 boot.
  ///
  /// The function parameters associated with this mode are:
  ///
  ///   Type 0x00.
  ///
  ///   HandleBuffer Buffer of all ISA bus handles found.
  ///
  ///   HandleCount Number of ISA bus handles found.
  ///
  ///   AdditionalData NULL.
  ///
  EfiGetPlatformIsaBusHandle = 2,
  ///
  /// This mode returns the Compatibility16 policy for the device that should be the USB
  /// device used during a Compatibility16 boot.
  ///
  /// The function parameters associated with this mode are:
  ///
  ///   Type 0x00.
  ///
  ///   HandleBuffer Buffer of all USB handles found.
  ///
  ///   HandleCount Number of USB bus handles found.
  ///
  ///   AdditionalData NULL.
  ///
  EfiGetPlatformUsbHandle = 3
} EFI_GET_PLATFORM_HANDLE_MODE;

/**
  This enum specifies the Mode param values for PlatformHooks().
  Note: Any OEM defined hooks start with 0x8000.
**/
typedef enum {
  ///
  /// This mode allows any preprocessing before scanning OpROMs.
  ///
  /// The function parameters associated with this mode are:
  ///
  ///     Type 0.
  ///
  ///     DeviceHandle Handle of device OpROM is associated with.
  ///
  ///     ShadowAddress Address where OpROM is shadowed.
  ///
  ///     Compatibility16Table NULL.
  ///
  ///     AdditionalData NULL.
  ///
  EfiPlatformHookPrepareToScanRom = 0,
  ///
  /// This mode shadows legacy OpROMS that may not have a physical device associated with
  /// them. It returns EFI_SUCCESS if the ROM was shadowed.
  ///
  /// The function parameters associated with this mode are:
  ///
  ///     Type 0.
  ///
  ///     DeviceHandle 0.
  ///
  ///     ShadowAddress First free OpROM area, after other OpROMs have been dispatched..
  ///
  ///     Compatibility16Table Pointer to the Compatibility16 Table.
  ///
  ///       AdditionalData NULL.
  ///
  EfiPlatformHookShadowServiceRoms = 1,
  ///
  /// This mode allows platform to perform any required operation after an OpROM has
  /// completed its initialization.
  ///
  /// The function parameters associated with this mode are:
  ///
  ///       Type 0.
  ///
  ///       DeviceHandle Handle of device OpROM is associated with.
  ///
  ///       ShadowAddress Address where OpROM is shadowed.
  ///
  ///       Compatibility16Table NULL.
  ///
  ///       AdditionalData NULL.
  ///
  EfiPlatformHookAfterRomInit = 2
} EFI_GET_PLATFORM_HOOK_MODE;

///
/// This IRQ has not been assigned to PCI.
///
#define PCI_UNUSED  0x00
///
/// This IRQ has been assigned to PCI.
///
#define PCI_USED  0xFF
///
/// This IRQ has been used by an SIO legacy device and cannot be used by PCI.
///
#define LEGACY_USED  0xFE

#pragma pack(1)

typedef struct {
  ///
  /// IRQ for this entry.
  ///
  UINT8    Irq;
  ///
  /// Status of this IRQ.
  ///
  /// PCI_UNUSED 0x00. This IRQ has not been assigned to PCI.
  ///
  /// PCI_USED 0xFF. This IRQ has been assigned to PCI.
  ///
  /// LEGACY_USED 0xFE. This IRQ has been used by an SIO legacy
  /// device and cannot be used by PCI.
  ///
  UINT8    Used;
} EFI_LEGACY_IRQ_PRIORITY_TABLE_ENTRY;

//
// Define PIR table structures
//
#define EFI_LEGACY_PIRQ_TABLE_SIGNATURE  SIGNATURE_32 ('$', 'P', 'I', 'R')

typedef struct {
  ///
  /// $PIR.
  ///
  UINT32    Signature;
  ///
  /// 0x00.
  ///
  UINT8     MinorVersion;
  ///
  /// 0x01 for table version 1.0.
  ///
  UINT8     MajorVersion;
  ///
  /// 0x20 + RoutingTableEntries * 0x10.
  ///
  UINT16    TableSize;
  ///
  /// PCI interrupt router bus.
  ///
  UINT8     Bus;
  ///
  /// PCI interrupt router device/function.
  ///
  UINT8     DevFun;
  ///
  /// If nonzero, bit map of IRQs reserved for PCI.
  ///
  UINT16    PciOnlyIrq;
  ///
  /// Vendor ID of a compatible PCI interrupt router.
  ///
  UINT16    CompatibleVid;
  ///
  /// Device ID of a compatible PCI interrupt router.
  ///
  UINT16    CompatibleDid;
  ///
  /// If nonzero, a value passed directly to the IRQ miniport's Initialize function.
  ///
  UINT32    Miniport;
  ///
  /// Reserved for future usage.
  ///
  UINT8     Reserved[11];
  ///
  /// This byte plus the sum of all other bytes in the LocalPirqTable equal 0x00.
  ///
  UINT8     Checksum;
} EFI_LEGACY_PIRQ_TABLE_HEADER;

typedef struct {
  ///
  /// If nonzero, a value assigned by the IBV.
  ///
  UINT8     Pirq;
  ///
  /// If nonzero, the IRQs that can be assigned to this device.
  ///
  UINT16    IrqMask;
} EFI_LEGACY_PIRQ_ENTRY;

typedef struct {
  ///
  /// PCI bus of the entry.
  ///
  UINT8                    Bus;
  ///
  /// PCI device of this entry.
  ///
  UINT8                    Device;
  ///
  /// An IBV value and IRQ mask for PIRQ pins A through D.
  ///
  EFI_LEGACY_PIRQ_ENTRY    PirqEntry[4];
  ///
  /// If nonzero, the slot number assigned by the board manufacturer.
  ///
  UINT8                    Slot;
  ///
  /// Reserved for future use.
  ///
  UINT8                    Reserved;
} EFI_LEGACY_IRQ_ROUTING_ENTRY;

#pragma pack()

/**
  Finds the binary data or other platform information.

  @param  This                  The protocol instance pointer.
  @param  Mode                  Specifies what data to return. See See EFI_GET_PLATFORM_INFO_MODE enum.
  @param  Table                 Mode specific.  See EFI_GET_PLATFORM_INFO_MODE enum.
  @param  TableSize              Mode specific.  See EFI_GET_PLATFORM_INFO_MODE enum.
  @param  Location               Mode specific.  See EFI_GET_PLATFORM_INFO_MODE enum.
  @param  Alignment             Mode specific.  See EFI_GET_PLATFORM_INFO_MODE enum.
  @param  LegacySegment         Mode specific.  See EFI_GET_PLATFORM_INFO_MODE enum.
  @param  LegacyOffset          Mode specific.  See EFI_GET_PLATFORM_INFO_MODE enum.

  @retval EFI_SUCCESS           Data returned successfully.
  @retval EFI_UNSUPPORTED       Mode is not supported on the platform.
  @retval EFI_NOT_FOUND         Binary image or table not found.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_GET_PLATFORM_INFO)(
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   *This,
  IN EFI_GET_PLATFORM_INFO_MODE          Mode,
  OUT VOID                               **Table,
  OUT UINTN                              *TableSize,
  OUT UINTN                              *Location,
  OUT UINTN                              *Alignment,
  IN  UINT16                             LegacySegment,
  IN  UINT16                             LegacyOffset
  );

/**
  Returns a buffer of handles for the requested subfunction.

  @param  This                  The protocol instance pointer.
  @param  Mode                  Specifies what handle to return. See EFI_GET_PLATFORM_HANDLE_MODE enum.
  @param  Type                  Mode specific. See EFI_GET_PLATFORM_HANDLE_MODE enum.
  @param  HandleBuffer          Mode specific. See EFI_GET_PLATFORM_HANDLE_MODE enum.
  @param  HandleCount           Mode specific. See EFI_GET_PLATFORM_HANDLE_MODE enum.
  @param  AdditionalData        Mode specific. See EFI_GET_PLATFORM_HANDLE_MODE enum.

  @retval EFI_SUCCESS           Handle is valid.
  @retval EFI_UNSUPPORTED       Mode is not supported on the platform.
  @retval EFI_NOT_FOUND         Handle is not known.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_GET_PLATFORM_HANDLE)(
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   *This,
  IN EFI_GET_PLATFORM_HANDLE_MODE        Mode,
  IN UINT16                              Type,
  OUT EFI_HANDLE                         **HandleBuffer,
  OUT UINTN                              *HandleCount,
  IN  VOID                               **AdditionalData OPTIONAL
  );

/**
  Load and initialize the Legacy BIOS SMM handler.

  @param  This                   The protocol instance pointer.
  @param  EfiToLegacy16BootTable A pointer to Legacy16 boot table.

  @retval EFI_SUCCESS           SMM code loaded.
  @retval EFI_DEVICE_ERROR      SMM code failed to load

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_SMM_INIT)(
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   *This,
  IN  VOID                               *EfiToLegacy16BootTable
  );

/**
  Allows platform to perform any required action after a LegacyBios operation.
  Invokes the specific sub function specified by Mode.

  @param  This                  The protocol instance pointer.
  @param  Mode                  Specifies what handle to return. See EFI_GET_PLATFORM_HOOK_MODE enum.
  @param  Type                  Mode specific.  See EFI_GET_PLATFORM_HOOK_MODE enum.
  @param  DeviceHandle          Mode specific.  See EFI_GET_PLATFORM_HOOK_MODE enum.
  @param  ShadowAddress         Mode specific.  See EFI_GET_PLATFORM_HOOK_MODE enum.
  @param  Compatibility16Table  Mode specific.  See EFI_GET_PLATFORM_HOOK_MODE enum.
  @param  AdditionalData        Mode specific.  See EFI_GET_PLATFORM_HOOK_MODE enum.

  @retval EFI_SUCCESS           The operation performed successfully. Mode specific.
  @retval EFI_UNSUPPORTED       Mode is not supported on the platform.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_HOOKS)(
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   *This,
  IN EFI_GET_PLATFORM_HOOK_MODE          Mode,
  IN UINT16                              Type,
  IN  EFI_HANDLE                         DeviceHandle  OPTIONAL,
  IN  OUT UINTN                          *ShadowAddress  OPTIONAL,
  IN  EFI_COMPATIBILITY16_TABLE          *Compatibility16Table  OPTIONAL,
  OUT  VOID                               **AdditionalData OPTIONAL
  );

/**
  Returns information associated with PCI IRQ routing.
  This function returns the following information associated with PCI IRQ routing:
    * An IRQ routing table and number of entries in the table.
    * The $PIR table and its size.
    * A list of PCI IRQs and the priority order to assign them.

  @param  This                    The protocol instance pointer.
  @param  RoutingTable            The pointer to PCI IRQ Routing table.
                                  This location is the $PIR table minus the header.
  @param  RoutingTableEntries     The number of entries in table.
  @param  LocalPirqTable          $PIR table.
  @param  PirqTableSize           $PIR table size.
  @param  LocalIrqPriorityTable   A list of interrupts in priority order to assign.
  @param  IrqPriorityTableEntries The number of entries in the priority table.

  @retval EFI_SUCCESS           Data was successfully returned.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_GET_ROUTING_TABLE)(
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   *This,
  OUT VOID                               **RoutingTable,
  OUT UINTN                              *RoutingTableEntries,
  OUT VOID                               **LocalPirqTable  OPTIONAL,
  OUT UINTN                              *PirqTableSize  OPTIONAL,
  OUT VOID                               **LocalIrqPriorityTable  OPTIONAL,
  OUT UINTN                              *IrqPriorityTableEntries OPTIONAL
  );

/**
  Translates the given PIRQ accounting for bridge.
  This function translates the given PIRQ back through all buses, if required,
  and returns the true PIRQ and associated IRQ.

  @param  This                  The protocol instance pointer.
  @param  PciBus                The PCI bus number for this device.
  @param  PciDevice             The PCI device number for this device.
  @param  PciFunction           The PCI function number for this device.
  @param  Pirq                  Input is PIRQ reported by device, and output is true PIRQ.
  @param  PciIrq                The IRQ already assigned to the PIRQ, or the IRQ to be
                                assigned to the PIRQ.

  @retval EFI_SUCCESS           The PIRQ was translated.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_TRANSLATE_PIRQ)(
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   *This,
  IN  UINTN                              PciBus,
  IN  UINTN                              PciDevice,
  IN  UINTN                              PciFunction,
  IN  OUT UINT8                          *Pirq,
  OUT UINT8                              *PciIrq
  );

/**
  Attempt to legacy boot the BootOption. If the EFI contexted has been
  compromised this function will not return.

  @param  This                   The protocol instance pointer.
  @param  BbsDevicePath          The EFI Device Path from BootXXXX variable.
  @param  BbsTable               The Internal BBS table.
  @param  LoadOptionSize         The size of LoadOption in size.
  @param  LoadOption             The LoadOption from BootXXXX variable
  @param  EfiToLegacy16BootTable A pointer to BootTable structure

  @retval EFI_SUCCESS           Ready to boot.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PLATFORM_PREPARE_TO_BOOT)(
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   *This,
  IN  BBS_BBS_DEVICE_PATH                *BbsDevicePath,
  IN  VOID                               *BbsTable,
  IN  UINT32                             LoadOptionsSize,
  IN  VOID                               *LoadOptions,
  IN  VOID                               *EfiToLegacy16BootTable
  );

/**
  This protocol abstracts the platform portion of the traditional BIOS.
**/
struct _EFI_LEGACY_BIOS_PLATFORM_PROTOCOL {
  ///
  ///  Gets binary data or other platform information.
  ///
  EFI_LEGACY_BIOS_PLATFORM_GET_PLATFORM_INFO      GetPlatformInfo;
  ///
  ///  Returns a buffer of all handles matching the requested subfunction.
  ///
  EFI_LEGACY_BIOS_PLATFORM_GET_PLATFORM_HANDLE    GetPlatformHandle;
  ///
  ///  Loads and initializes the traditional BIOS SMM handler.
  EFI_LEGACY_BIOS_PLATFORM_SMM_INIT               SmmInit;
  ///
  ///  Allows platform to perform any required actions after a LegacyBios operation.
  ///
  EFI_LEGACY_BIOS_PLATFORM_HOOKS                  PlatformHooks;
  ///
  ///  Gets $PIR table.
  EFI_LEGACY_BIOS_PLATFORM_GET_ROUTING_TABLE      GetRoutingTable;
  ///
  ///  Translates the given PIRQ to the final value after traversing any PCI bridges.
  ///
  EFI_LEGACY_BIOS_PLATFORM_TRANSLATE_PIRQ         TranslatePirq;
  ///
  ///  Final platform function before the system attempts to boot to a traditional OS.
  ///
  EFI_LEGACY_BIOS_PLATFORM_PREPARE_TO_BOOT        PrepareToBoot;
};

extern EFI_GUID  gEfiLegacyBiosPlatformProtocolGuid;

#endif
