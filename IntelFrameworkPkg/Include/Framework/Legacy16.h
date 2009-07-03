/** @file
  The header file provides interface definitions exposed by CSM (Compatible Support Module).
  
  The CSM provides compatibility support between the Framework and traditional, legacy BIOS code 
  and allows booting a traditional OS or booting an EFI OS off a device that requires a traditional 
  option ROM (OpROM).  

  These definitions are from Compatibility Support Module Spec Version 0.97.
  
  Copyright (c) 2007-2009, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FRAMEWORK_LEGACY_16_H_
#define _FRAMEWORK_LEGACY_16_H_

#include <Base.h>

#pragma pack(1)

typedef UINT8                       SERIAL_MODE;
typedef UINT8                       PARALLEL_MODE;

#define EFI_COMPATIBILITY16_TABLE_SIGNATURE SIGNATURE_32 ('I', 'F', 'E', '$')

///
/// There is a table located within the traditional BIOS in either the 0xF000:xxxx or 0xE000:xxxx
/// physical address range. It is located on a 16-byte boundary and provides the physical address of the
/// entry point for the Compatibility16 functions. These functions provide the platform-specific
/// information that is required by the generic EfiCompatibility code. The functions are invoked via
/// thunking by using EFI_LEGACY_BIOS_PROTOCOL.FarCall86() with the 32-bit physical
/// entry point.
///
typedef struct {
  ///
  /// The string "$EFI" denotes the start of the EfiCompatibility table. Byte 0 is "I," byte
  /// 1 is "F," byte 2 is "E," and byte 3 is "$" and is normally accessed as a DWORD or UINT32.
  ///
  UINT32                            Signature;
  
  ///
  /// The value required such that byte checksum of TableLength equals zero.
  ///
  UINT8                             TableChecksum;
  
  ///
  /// The length of this table.
  ///
  UINT8                             TableLength;
  
  ///
  /// The major EFI revision for which this table was generated.
  /// 
  UINT8                             EfiMajorRevision;
  
  ///
  /// The minor EFI revision for which this table was generated.
  ///
  UINT8                             EfiMinorRevision;
  
  ///
  /// The major revision of this table.
  ///
  UINT8                             TableMajorRevision;
  
  ///
  /// The minor revision of this table.
  ///
  UINT8                             TableMinorRevision;
  
  ///
  /// Reserved for future usage.
  ///
  UINT16                            Reserved;
  
  ///
  /// The segment of the entry point within the traditional BIOS for Compatibility16 functions.
  ///
  UINT16                            Compatibility16CallSegment;
  
  ///
  /// The offset of the entry point within the traditional BIOS for Compatibility16 functions.
  ///
  UINT16                            Compatibility16CallOffset;
  
  ///
  /// The segment of the entry point within the traditional BIOS for EfiCompatibility to invoke the PnP installation check.
  ///
  UINT16                            PnPInstallationCheckSegment;
  
  ///
  /// The Offset of the entry point within the traditional BIOS for EfiCompatibility to invoke the PnP installation check.
  ///
  UINT16                            PnPInstallationCheckOffset;
  
  ///
  /// EFI system resources table. Type EFI_SYSTEM_TABLE is defined in the IntelPlatform Innovation Framework for EFI 
  /// Driver Execution Environment Core Interface Specification (DXE CIS).
  ///
  UINT32                            EfiSystemTable; 
  
  ///
  /// The address of an OEM-provided identifier string. The string is null terminated.
  ///
  UINT32                            OemIdStringPointer;
  
  ///
  /// The 32-bit physical address where ACPI RSD PTR is stored within the traditional
  /// BIOS. The remained of the ACPI tables are located at their EFI addresses. The size
  /// reserved is the maximum for ACPI 2.0. The EfiCompatibility will fill in the ACPI
  /// RSD PTR with either the ACPI 1.0b or 2.0 values.
  ///
  UINT32                            AcpiRsdPtrPointer;
  
  ///
  /// The OEM revision number. Usage is undefined but provided for OEM module usage.
  ///
  UINT16                            OemRevision;
  
  ///
  /// The 32-bit physical address where INT15 E820 data is stored within the traditional
  /// BIOS. The EfiCompatibility code will fill in the E820Pointer value and copy the
  /// data to the indicated area.
  ///
  UINT32                            E820Pointer;
  
  ///
  /// The length of the E820 data and is filled in by the EfiCompatibility code.
  ///
  UINT32                            E820Length;
  
  ///
  /// The 32-bit physical address where the $PIR table is stored in the traditional BIOS.
  /// The EfiCompatibility code will fill in the IrqRoutingTablePointer value and
  /// copy the data to the indicated area.
  ///
  UINT32                            IrqRoutingTablePointer;
  
  ///
  /// The length of the $PIR table and is filled in by the EfiCompatibility code.
  ///
  UINT32                            IrqRoutingTableLength;
  
  ///
  /// The 32-bit physical address where the MP table is stored in the traditional BIOS.
  /// The EfiCompatibility code will fill in the MpTablePtr value and copy the data to the indicated area.
  ///
  UINT32                            MpTablePtr;
  
  ///
  /// The length of the MP table and is filled in by the EfiCompatibility code.
  ///
  UINT32                            MpTableLength;
  
  ///
  /// The segment of the OEM-specific INT table/code.
  /// 
  UINT16                            OemIntSegment;
  
  ///
  /// The offset of the OEM-specific INT table/code.
  ///
  UINT16                            OemIntOffset;
  
  ///
  /// The segment of the OEM-specific 32-bit table/code.
  ///
  UINT16                            Oem32Segment;
  
  ///
  /// The offset of the OEM-specific 32-bit table/code.
  ///
  UINT16                            Oem32Offset;
  
  ///
  /// The segment of the OEM-specific 16-bit table/code.
  ///
  UINT16                            Oem16Segment;
  
  ///
  /// The offset of the OEM-specific 16-bit table/code.
  ///
  UINT16                            Oem16Offset;
  
  ///
  /// The segment of the TPM binary passed to 16-bit CSM.
  ///
  UINT16                            TpmSegment;
  
  ///
  /// The offset of the TPM binary passed to 16-bit CSM.
  ///
  UINT16                            TpmOffset;
  
  ///
  /// A pointer to a string identifying the independent BIOS vendor.
  ///
  UINT32                            IbvPointer;
  
  ///
  /// This field is NULL for all systems not supporting PCI Express. This field is the base
  /// value of the start of the PCI Express memory-mapped configuration registers and
  /// must be filled in prior to EfiCompatibility code issuing the Compatibility16 function
  /// Compatibility16InitializeYourself().
  /// Compatibility16InitializeYourself() is defined in Compatability16
  /// Functions.
  ///
  UINT32                            PciExpressBase;
  
  ///
  /// Maximum PCI bus number assigned.
  ///
  UINT8                             LastPciBus;
} EFI_COMPATIBILITY16_TABLE;

///
/// Functions provided by the CSM binary which communicate between the EfiCompatibility 
/// and Compatability16 code.
///
typedef enum {
  ///
  /// Causes the Compatibility16 code to do any internal initialization required.
  /// Input:
  ///   AX = Compatibility16InitializeYourself
  ///   ES:BX = Pointer to EFI_TO_COMPATIBILITY16_INIT_TABLE
  /// Return:
  ///   AX = Return Status codes
  ///
  Compatibility16InitializeYourself    = 0x0000,
  
  ///
  /// Causes the Compatibility16 BIOS to perform any drive number translations to match the boot sequence.
  /// Input:
  ///   AX = Compatibility16UpdateBbs
  ///   ES:BX = Pointer to EFI_TO_COMPATIBILITY16_BOOT_TABLE
  /// Return:
  ///   AX = Returned status codes
  ///
  Compatibility16UpdateBbs             = 0x0001,
  
  ///
  /// Allows the Compatibility16 code to perform any final actions before booting. The Compatibility16
  /// code is read/write.
  /// Input:
  ///   AX = Compatibility16PrepareToBoot
  ///   ES:BX = Pointer to EFI_TO_COMPATIBILITY16_BOOT_TABLE structure  
  /// Return:
  ///   AX = Returned status codes
  ///
  Compatibility16PrepareToBoot         = 0x0002,
  
  ///
  /// Causes the Compatibility16 BIOS to boot. The Compatibility16 code is Read/Only.
  /// Input:
  ///   AX = Compatibility16Boot
  /// Output:
  ///   AX = Returned status codes
  ///
  Compatibility16Boot                  = 0x0003,
  
  ///
  /// Allows the Compatibility16 code to get the last device from which a boot was attempted. This is
  /// stored in CMOS and is the priority number of the last attempted boot device.
  /// Input:
  ///   AX = Compatibility16RetrieveLastBootDevice
  /// Output:
  ///   AX = Returned status codes
  ///   BX = Priority number of the boot device.
  ///
  Compatibility16RetrieveLastBootDevice= 0x0004,
  
  ///
  /// Allows the Compatibility16 code rehook INT13, INT18, and/or INT19 after dispatching a legacy OpROM.
  /// Input:
  ///   AX = Compatibility16DispatchOprom
  ///   ES:BX = Pointer to EFI_DISPATCH_OPROM_TABLE
  /// Output:
  ///   AX = Returned status codes
  ///   BX = Number of non-BBS-compliant devices found. Equals 0 if BBS compliant.
  ///
  Compatibility16DispatchOprom         = 0x0005,
  
  ///
  /// Finds a free area in the 0xFxxxx or 0xExxxx region of the specified length and returns the address
  /// of that region.
  /// Input:
  ///   AX = Compatibility16GetTableAddress
  ///   BX = Allocation region
  ///       00 = Allocate from either 0xE0000 or 0xF0000 64 KB blocks.
  ///       Bit 0 = 1 Allocate from 0xF0000 64 KB block
  ///       Bit 1 = 1 Allocate from 0xE0000 64 KB block
  ///   CX = Requested length in bytes.
  ///   DX = Required address alignment. Bit mapped. First non-zero bit from the right is the alignment.
  /// Output:
  ///   AX = Returned status codes
  ///   DS:BX = Address of the region
  ///
  Compatibility16GetTableAddress       = 0x0006,
  
  ///
  /// Enables the EfiCompatibility module to do any nonstandard processing of keyboard LEDs or state.
  /// Input:
  ///   AX = Compatibility16SetKeyboardLeds
  ///   CL = LED status.
  ///     Bit 0 – Scroll Lock 0 = Off
  ///     Bit 1 – NumLock
  ///     Bit 2 – Caps Lock
  /// Output:
  ///     AX = Returned status codes
  ///
  Compatibility16SetKeyboardLeds       = 0x0007,
  
  ///
  /// Enables the EfiCompatibility module to install an interrupt handler for PCI mass media devices that
  /// do not have an OpROM associated with them. An example is SATA.
  /// Input:
  ///   AX = Compatibility16InstallPciHandler
  ///   ES:BX = Pointer to EFI_LEGACY_INSTALL_PCI_HANDLER structure
  /// Output:
  ///   AX = Returned status codes
  ///
  Compatibility16InstallPciHandler     = 0x0008
} EFI_COMPATIBILITY_FUNCTIONS;


///
/// EFI_DISPATCH_OPROM_TABLE
///
typedef struct {
  UINT16  PnPInstallationCheckSegment;  ///< Pointer to the PnpInstallationCheck data structure.
  UINT16  PnPInstallationCheckOffset;   ///< Pointer to the PnpInstallationCheck data structure.
  UINT16  OpromSegment;                 ///< The segment where the OpROM was placed. Offset is assumed to be 3.
  UINT8   PciBus;                       ///< The PCI bus.
  UINT8   PciDeviceFunction;            ///< The PCI device * 0x08 | PCI function.
  UINT8   NumberBbsEntries;             ///< The number of valid BBS table entries upon entry and exit. The IBV code may
                                        ///< increase this number, if BBS-compliant devices also hook INTs in order to force the
                                        ///< OpROM BIOS Setup to be executed.
  VOID    *BbsTablePointer;             ///< Pointer to the BBS table.
  UINT16  OpromDestinationSegment;      ///< The segment where the OpROM can be relocated to. If this value is 0x0000, this
                                        ///< means that the relocation of this run time code is not supported.
} EFI_DISPATCH_OPROM_TABLE;

///
/// EFI_TO_COMPATIBILITY16_INIT_TABLE
///
typedef struct {
  ///
  /// Starting address of memory under 1 MB. The ending address is assumed to be 640 KB or 0x9FFFF.
  ///
  UINT32                            BiosLessThan1MB;
  
  ///
  /// Starting address of the high memory block.
  ///
  UINT32                            HiPmmMemory;
  
  ///
  /// Length of high memory block.
  ///
  UINT32                            HiPmmMemorySizeInBytes;
  
  ///
  /// The segment of the reverse thunk call code.
  ///
  UINT16                            ReverseThunkCallSegment;
  
  ///
  /// The offset of the reverse thunk call code.
  ///
  UINT16                            ReverseThunkCallOffset;
  
  ///
  /// The number of E820 entries copied to the Compatibility16 BIOS.
  ///
  UINT32                            NumberE820Entries;
  
  ///
  /// The amount of usable memory above 1 MB, e.g., E820 type 1 memory.
  ///
  UINT32                            OsMemoryAbove1Mb;
  
  ///
  /// The start of thunk code in main memory. Memory cannot be used by BIOS or PMM.
  ///
  UINT32                            ThunkStart;
  
  ///
  /// The size of the thunk code.
  ///
  UINT32                            ThunkSizeInBytes;
  
  ///
  /// Starting address of memory under 1 MB.
  ///
  UINT32                            LowPmmMemory;
  
  ///
  /// Length of low Memory block.
  ///
  UINT32                            LowPmmMemorySizeInBytes;
} EFI_TO_COMPATIBILITY16_INIT_TABLE;

///
/// DEVICE_PRODUCER_SERIAL & its modes
///
typedef struct {
  UINT16                            Address;    ///< I/O address assigned to the serial port
  UINT8                             Irq;        ///< IRQ assigned to the serial port.
  SERIAL_MODE                       Mode;       ///< Mode of serial port. Values are defined below.
} DEVICE_PRODUCER_SERIAL;

#define DEVICE_SERIAL_MODE_NORMAL               0x00
#define DEVICE_SERIAL_MODE_IRDA                 0x01
#define DEVICE_SERIAL_MODE_ASK_IR               0x02
#define DEVICE_SERIAL_MODE_DUPLEX_HALF          0x00
#define DEVICE_SERIAL_MODE_DUPLEX_FULL          0x10

///
/// DEVICE_PRODUCER_PARALLEL & its modes
///
typedef struct {
  UINT16                            Address;  ///< I/O address assigned to the parallel port
  UINT8                             Irq;      ///< IRQ assigned to the parallel port.
  UINT8                             Dma;      ///< DMA assigned to the parallel port.
  PARALLEL_MODE                     Mode;     ///< Mode of the parallel port. Values are defined below.
} DEVICE_PRODUCER_PARALLEL;

#define DEVICE_PARALLEL_MODE_MODE_OUTPUT_ONLY   0x00
#define DEVICE_PARALLEL_MODE_MODE_BIDIRECTIONAL 0x01
#define DEVICE_PARALLEL_MODE_MODE_EPP           0x02
#define DEVICE_PARALLEL_MODE_MODE_ECP           0x03

///
/// DEVICE_PRODUCER_FLOPPY
///
typedef struct {
  UINT16                            Address;          ///< I/O address assigned to the floppy
  UINT8                             Irq;              ///< IRQ assigned to the floppy.
  UINT8                             Dma;              ///< DMA assigned to the floppy.
  UINT8                             NumberOfFloppy;   ///< Number of floppies in the system.
} DEVICE_PRODUCER_FLOPPY;

///
/// LEGACY_DEVICE_FLAGS
///
typedef struct {
  UINT32                            A20Kybd : 1;      ///< A20 controller by keyboard controller.
  UINT32                            A20Port90 : 1;    ///< A20 controlled by port 0x92.
  UINT32                            Reserved : 30;    ///< Reserved for future usage.
} LEGACY_DEVICE_FLAGS;

///
/// DEVICE_PRODUCER_DATA_HEADER
///
typedef struct {
  DEVICE_PRODUCER_SERIAL            Serial[4];      ///< Data for serial port x. Type DEVICE_PRODUCER_SERIAL is defined below.
  DEVICE_PRODUCER_PARALLEL          Parallel[3];    ///< Data for parallel port x. Type DEVICE_PRODUCER_PARALLEL is defined below.
  DEVICE_PRODUCER_FLOPPY            Floppy;         ///< Data for floppy. Type DEVICE_PRODUCER_FLOPPY is defined below.
  UINT8                             MousePresent;   ///< Flag to indicate if mouse is present.
  LEGACY_DEVICE_FLAGS               Flags;          ///< Miscellaneous Boolean state information passed to CSM.
} DEVICE_PRODUCER_DATA_HEADER;

///
/// ATAPI_IDENTIFY
///
typedef struct {
  UINT16                            Raw[256];     ///< Raw data from the IDE IdentifyDrive command.
} ATAPI_IDENTIFY;

///
/// HDD_INFO & its status
///
typedef struct {
  ///
  /// Status of IDE device. Values are defined below. There is one HDD_INFO structure
  /// per IDE controller. The IdentifyDrive is per drive. Index 0 is master and index
  /// 1 is slave.
  ///
  UINT16                            Status;   
  
  ///
  /// PCI bus of IDE controller.
  ///
  UINT32                            Bus;
  
  ///
  /// PCI device of IDE controller.
  ///
  UINT32                            Device;
  
  ///
  /// PCI function of IDE controller.
  ///
  UINT32                            Function;
  
  ///
  /// Command ports base address.
  ///
  UINT16                            CommandBaseAddress;
  
  ///
  /// Control ports base address.
  ///
  UINT16                            ControlBaseAddress;
  
  ///
  /// Bus master address
  ///
  UINT16                            BusMasterAddress;
  
  UINT8                             HddIrq;
  
  ///
  /// Data that identifies the drive data, one per possible attached drive
  ///
  ATAPI_IDENTIFY                    IdentifyDrive[2];
} HDD_INFO;

#define HDD_PRIMARY               0x01
#define HDD_SECONDARY             0x02
#define HDD_MASTER_ATAPI_CDROM    0x04
#define HDD_SLAVE_ATAPI_CDROM     0x08
#define HDD_MASTER_IDE            0x20
#define HDD_SLAVE_IDE             0x40
#define HDD_MASTER_ATAPI_ZIPDISK  0x10
#define HDD_SLAVE_ATAPI_ZIPDISK   0x80

///
/// BBS_STATUS_FLAGS
///
typedef struct {
  UINT16                            OldPosition : 4;    ///< Prior priority.
  UINT16                            Reserved1 : 4;      ///< Reserved for future use.
  UINT16                            Enabled : 1;        ///< If 0, ignore this entry.
  UINT16                            Failed : 1;         ///< 0 = Not known if boot failure occurred.
                                                        ///< 1 = Boot attempted failed.
  
  ///
  /// State of media present.
  ///   00 = No bootable media is present in the device.
  ///   01 = Unknown if a bootable media present.
  ///   10 = Media is present and appears bootable.
  ///   11 = Reserved.
  ///
  UINT16                            MediaPresent : 2;
  UINT16                            Reserved2 : 4;      ///< Reserved for future use.
} BBS_STATUS_FLAGS;

///
/// BBS_TABLE, device type values & boot priority values
///
typedef struct {
  ///
  /// The boot priority for this boot device. Values are defined below.
  ///
  UINT16                            BootPriority;
  
  ///
  /// The PCI bus for this boot device.
  ///
  UINT32                            Bus;
  
  ///
  /// The PCI device for this boot device.
  ///
  UINT32                            Device;
  
  ///
  /// The PCI function for the boot device.
  ///
  UINT32                            Function;
  
  ///
  /// The PCI class for this boot device.
  ///
  UINT8                             Class;
  
  ///
  /// The PCI Subclass for this boot device.
  ///
  UINT8                             SubClass;
  
  ///
  /// Segment:offset address of an ASCIIZ description string describing the manufacturer.
  ///
  UINT16                            MfgStringOffset;
  
  ///
  /// Segment:offset address of an ASCIIZ description string describing the manufacturer.
  ///  
  UINT16                            MfgStringSegment;
  
  ///
  /// BBS device type. BBS device types are defined below.
  ///
  UINT16                            DeviceType;
  
  ///
  /// Status of this boot device. Type BBS_STATUS_FLAGS is defined below.
  ///
  BBS_STATUS_FLAGS                  StatusFlags;
  
  ///
  /// Segment:Offset address of boot loader for IPL devices or install INT13 handler for
  /// BCV devices.
  ///
  UINT16                            BootHandlerOffset;
  
  ///
  /// Segment:Offset address of boot loader for IPL devices or install INT13 handler for
  /// BCV devices.
  ///  
  UINT16                            BootHandlerSegment;
  
  ///
  /// Segment:offset address of an ASCIIZ description string describing this device.
  ///
  UINT16                            DescStringOffset;

  ///
  /// Segment:offset address of an ASCIIZ description string describing this device.
  ///
  UINT16                            DescStringSegment;
  
  ///
  /// Reserved.
  ///
  UINT32                            InitPerReserved;
  
  ///
  /// The use of these fields is IBV dependent. They can be used to flag that an OpROM
  /// has hooked the specified IRQ. The OpROM may be BBS compliant as some SCSI
  /// BBS-compliant OpROMs also hook IRQ vectors in order to run their BIOS Setup
  ///
  UINT32                            AdditionalIrq13Handler;
  
  ///
  /// The use of these fields is IBV dependent. They can be used to flag that an OpROM
  /// has hooked the specified IRQ. The OpROM may be BBS compliant as some SCSI
  /// BBS-compliant OpROMs also hook IRQ vectors in order to run their BIOS Setup
  ///  
  UINT32                            AdditionalIrq18Handler;
  
  ///
  /// The use of these fields is IBV dependent. They can be used to flag that an OpROM
  /// has hooked the specified IRQ. The OpROM may be BBS compliant as some SCSI
  /// BBS-compliant OpROMs also hook IRQ vectors in order to run their BIOS Setup
  ///  
  UINT32                            AdditionalIrq19Handler;
  
  ///
  /// The use of these fields is IBV dependent. They can be used to flag that an OpROM
  /// has hooked the specified IRQ. The OpROM may be BBS compliant as some SCSI
  /// BBS-compliant OpROMs also hook IRQ vectors in order to run their BIOS Setup
  ///  
  UINT32                            AdditionalIrq40Handler;
  UINT8                             AssignedDriveNumber;
  UINT32                            AdditionalIrq41Handler;
  UINT32                            AdditionalIrq46Handler;
  UINT32                            IBV1;
  UINT32                            IBV2;
} BBS_TABLE;

#define BBS_FLOPPY              0x01
#define BBS_HARDDISK            0x02
#define BBS_CDROM               0x03
#define BBS_PCMCIA              0x04
#define BBS_USB                 0x05
#define BBS_EMBED_NETWORK       0x06
#define BBS_BEV_DEVICE          0x80
#define BBS_UNKNOWN             0xff

#define BBS_DO_NOT_BOOT_FROM    0xFFFC
#define BBS_LOWEST_PRIORITY     0xFFFD
#define BBS_UNPRIORITIZED_ENTRY 0xFFFE
#define BBS_IGNORE_ENTRY        0xFFFF

///
/// SMM_ATTRIBUTES & relating type, port and data size constants
///
typedef struct {
  ///
  /// Access mechanism used to generate the soft SMI. Defined types are below. The other
  /// values are reserved for future usage.
  ///
  UINT16                            Type : 3;
  
  ///
  /// Size of "port" in bits. Defined values are below.
  ///
  UINT16                            PortGranularity : 3;
  
  ///
  /// Size of data in bits. Defined values are below.
  ///
  UINT16                            DataGranularity : 3;
  
  ///
  /// Reserved for future use.
  ///
  UINT16                            Reserved : 7;
} SMM_ATTRIBUTES;

#define STANDARD_IO       0x00
#define STANDARD_MEMORY   0x01

#define PORT_SIZE_8       0x00
#define PORT_SIZE_16      0x01
#define PORT_SIZE_32      0x02
#define PORT_SIZE_64      0x03

#define DATA_SIZE_8       0x00
#define DATA_SIZE_16      0x01
#define DATA_SIZE_32      0x02
#define DATA_SIZE_64      0x03

///
/// SMM_FUNCTION & relating constants
///
typedef struct {
  UINT16                            Function : 15;
  UINT16                            Owner : 1;
} SMM_FUNCTION;

#define INT15_D042        0x0000
#define GET_USB_BOOT_INFO 0x0001
#define DMI_PNP_50_57     0x0002

#define STANDARD_OWNER    0x0
#define OEM_OWNER         0x1

/**
  * SMM_ENTRY
  *
  * This structure assumes both port and data sizes are 1. SmmAttribute must be
  * properly to reflect that assumption.
**/
typedef struct {
  ///
  /// Describes the access mechanism, SmmPort, and SmmData sizes. Type
  /// SMM_ATTRIBUTES is defined below.
  ///
  SMM_ATTRIBUTES                    SmmAttributes;
  
  ///
  /// Function Soft SMI is to perform. Type SMM_FUNCTION is defined below.
  ///
  SMM_FUNCTION                      SmmFunction;
  
  ///
  /// SmmPort size depends upon SmmAttributes and ranges from2 bytes to 16 bytes
  ///
  UINT8                             SmmPort;
  
  ///
  /// SmmData size depends upon SmmAttributes and ranges from2 bytes to 16 bytes
  ///
  UINT8                             SmmData;
} SMM_ENTRY;

///
/// SMM_TABLE
///
typedef struct {
  UINT16                            NumSmmEntries;    ///< Number of entries represented by SmmEntry.
  SMM_ENTRY                         SmmEntry;         ///< One entry per function. Type SMM_ENTRY is defined below.
} SMM_TABLE;

///
/// UDC_ATTRIBUTES
///
typedef struct {
  ///
  /// This bit set indicates that the ServiceAreaData is valid.
  ///
  UINT8                             DirectoryServiceValidity : 1;
  
  ///
  /// This bit set indicates to use the Reserve Area Boot Code Address (RACBA) only if
  /// DirectoryServiceValidity is 0.
  ///
  UINT8                             RabcaUsedFlag : 1;
  
  ///
  /// This bit set indicates to execute hard disk diagnostics.
  ///
  UINT8                             ExecuteHddDiagnosticsFlag : 1;
  
  ///
  /// Reserved for future use. Set to 0.
  ///
  UINT8                             Reserved : 5;
} UDC_ATTRIBUTES;

///
/// UD_TABLE
///
typedef struct {
  ///
  /// This field contains the bit-mapped attributes of the PARTIES information. Type
  /// UDC_ATTRIBUTES is defined below.
  ///
  UDC_ATTRIBUTES                    Attributes;
  
  ///
  /// This field contains the zero-based device on which the selected
  /// ServiceDataArea is present. It is 0 for master and 1 for the slave device.  
  ///
  UINT8                             DeviceNumber;
  
  ///
  /// This field contains the zero-based index into the BbsTable for the parent device.
  /// This index allows the user to reference the parent device information such as PCI
  /// bus, device function.
  ///
  UINT8                             BbsTableEntryNumberForParentDevice;
  
  ///
  /// This field contains the zero-based index into the BbsTable for the boot entry.
  ///
  UINT8                             BbsTableEntryNumberForBoot;
  
  ///
  /// This field contains the zero-based index into the BbsTable for the HDD diagnostics entry.
  ///
  UINT8                             BbsTableEntryNumberForHddDiag;
  
  ///
  /// The raw Beer data.
  ///
  UINT8                             BeerData[128];
  
  ///
  /// The raw data of selected service area.
  ///
  UINT8                             ServiceAreaData[64];
} UD_TABLE;

#define EFI_TO_LEGACY_MAJOR_VERSION 0x02
#define EFI_TO_LEGACY_MINOR_VERSION 0x00
#define MAX_IDE_CONTROLLER          8

///
/// EFI_TO_COMPATIBILITY16_BOOT_TABLE
///
typedef struct {
  UINT16                            MajorVersion;                 ///< The EfiCompatibility major version number.
  UINT16                            MinorVersion;                 ///< The EfiCompatibility minor version number.
  UINT32                            AcpiTable;                    ///< Location of the RSDT ACPI table. < 4G range
  UINT32                            SmbiosTable;                  ///< Location of the SMBIOS table in EFI memory. < 4G range
  UINT32                            SmbiosTableLength;
  //
  // Legacy SIO state
  //
  DEVICE_PRODUCER_DATA_HEADER       SioData;                      ///< Standard traditional device information.
  UINT16                            DevicePathType;               ///< The default boot type.
  UINT16                            PciIrqMask;                   ///< Mask of which IRQs have been assigned to PCI.
  UINT32                            NumberE820Entries;            ///< Number of E820 entries. The number can change from the
                                                                  ///< Compatibility16InitializeYourself() function.
  //
  // Controller & Drive Identify[2] per controller information
  //
  HDD_INFO                          HddInfo[MAX_IDE_CONTROLLER];  ///< Hard disk drive information, including raw Identify Drive data.
  UINT32                            NumberBbsEntries;             ///< Number of entries in the BBS table
  UINT32                            BbsTable;                     ///< Pointer to the BBS table. Type BBS_TABLE is defined below.
  UINT32                            SmmTable;                     ///< Pointer to the SMM table. Type SMM_TABLE is defined below.
  UINT32                            OsMemoryAbove1Mb;             ///< The amount of usable memory above 1 MB, i.e. E820 type 1 memory. This value can
                                                                  ///< differ from the value in EFI_TO_COMPATIBILITY16_INIT_TABLE as more
                                                                  ///< memory may have been discovered.
  UINT32                            UnconventionalDeviceTable;    ///< Information to boot off an unconventional device like a PARTIES partition. Type
                                                                  ///< UD_TABLE is defined below.
} EFI_TO_COMPATIBILITY16_BOOT_TABLE;

///
/// EFI_LEGACY_INSTALL_PCI_HANDLER
///
typedef struct {
  UINT8                             PciBus;             ///< The PCI bus of the device.
  UINT8                             PciDeviceFun;       ///< The PCI device in bits 7:3 and function in bits 2:0.
  UINT8                             PciSegment;         ///< The PCI segment of the device.
  UINT8                             PciClass;           ///< The PCI class code of the device.
  UINT8                             PciSubclass;        ///< The PCI subclass code of the device.
  UINT8                             PciInterface;       ///< The PCI interface code of the device.
  //
  // Primary section
  //
  UINT8                             PrimaryIrq;         ///< The primary device IRQ.
  UINT8                             PrimaryReserved;    ///< Reserved.
  UINT16                            PrimaryControl;     ///< The primary device control I/O base.
  UINT16                            PrimaryBase;        ///< The primary device I/O base.
  UINT16                            PrimaryBusMaster;   ///< The primary device bus master I/O base.
  //
  // Secondary Section
  //
  UINT8                             SecondaryIrq;       ///< The secondary device IRQ.
  UINT8                             SecondaryReserved;  ///< Reserved.
  UINT16                            SecondaryControl;   ///< The secondary device control I/O base.
  UINT16                            SecondaryBase;      ///< The secondary device I/O base.
  UINT16                            SecondaryBusMaster; ///< The secondary device bus master I/O base.
} EFI_LEGACY_INSTALL_PCI_HANDLER;

//
// Restore default pack value
//
#pragma pack()

#endif  /* _FRAMEWORK_LEGACY_16_H_ */
