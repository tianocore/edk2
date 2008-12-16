/** @file
  API between 16-bit Legacy BIOS and EFI

  We need to figure out what the 16-bit code is going to use to
  represent these data structures. Is a pointer SEG:OFF or 32-bit...

  Copyright (c) 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  FrameworkLegacy16.h

  @par Revision Reference:
  These definitions are from Compatibility Support Module Spec Version 0.96.

**/

#ifndef _FRAMEWORK_LEGACY_16_H_
#define _FRAMEWORK_LEGACY_16_H_

#include <Base.h>

//
// All structures defined in this header file are packed on byte boundary
//
#pragma pack(1)

typedef UINT8 SERIAL_MODE;
typedef UINT8 PARALLEL_MODE;

///////////////////////////////////////////////////////////////////////////////
// EFI_COMPATIBILITY16_TABLE is located at a 16-byte boundary starting with the
// signature "$EFI"
///////////////////////////////////////////////////////////////////////////////

#define EFI_COMPATIBILITY16_TABLE_SIGNATURE SIGNATURE_32 ('I', 'F', 'E', '$')

typedef struct {
  UINT32  Signature;      // "$EFI"
  UINT8   TableChecksum;
  UINT8   TableLength;
  UINT8   EfiMajorRevision;
  UINT8   EfiMinorRevision;
  UINT8   TableMajorRevision;
  UINT8   TableMinorRevision;
  UINT16  Reserved;
  UINT16  Compatibility16CallSegment;
  UINT16  Compatibility16CallOffset;
  UINT16  PnPInstallationCheckSegment;
  UINT16  PnPInstallationCheckOffset;
  UINT32  EfiSystemTable; // The physical address of EFI_SYSTEM_TABLE
  UINT32  OemIdStringPointer;
  UINT32  AcpiRsdPtrPointer;
  UINT16  OemRevision;
  UINT32  E820Pointer;
  UINT32  E820Length;
  UINT32  IrqRoutingTablePointer;
  UINT32  IrqRoutingTableLength;
  UINT32  MpTablePtr;
  UINT32  MpTableLength;
  UINT16  OemIntSegment;
  UINT16  OemIntOffset;
  UINT16  Oem32Segment;
  UINT16  Oem32Offset;
  UINT16  Oem16Segment;
  UINT16  Oem16Offset;
  UINT16  TpmSegment;
  UINT16  TpmOffset;
  UINT32  IbvPointer;
  UINT32  PciExpressBase;
  UINT8   LastPciBus;
} EFI_COMPATIBILITY16_TABLE;

///////////////////////////////////////////////////////////////////////////////
// Functions provided by the CSM binary
///////////////////////////////////////////////////////////////////////////////
typedef enum {
  Legacy16InitializeYourself    = 0x0000,
  Legacy16UpdateBbs             = 0x0001,
  Legacy16PrepareToBoot         = 0x0002,
  Legacy16Boot                  = 0x0003,
  Legacy16RetrieveLastBootDevice= 0x0004,
  Legacy16DispatchOprom         = 0x0005,
  Legacy16GetTableAddress       = 0x0006,
  Legacy16SetKeyboardLeds       = 0x0007,
  Legacy16InstallPciHandler     = 0x0008
} EFI_COMPATIBILITY_FUNCTIONS;

///////////////////////////////////////////////////////////////////////////////
// EFI_TO_COMPATIBILITY16_INIT_TABLE
///////////////////////////////////////////////////////////////////////////////
typedef struct {
  UINT32  BiosLessThan1MB;
  UINT32  HiPmmMemory;
  UINT32  HiPmmMemorySizeInBytes;
  UINT16  ReverseThunkCallSegment;
  UINT16  ReverseThunkCallOffset;
  UINT32  NumberE820Entries;
  UINT32  OsMemoryAbove1Mb;
  UINT32  ThunkStart;
  UINT32  ThunkSizeInBytes;
  UINT32  LowPmmMemory;
  UINT32  LowPmmMemorySizeInBytes;
} EFI_TO_COMPATIBILITY16_INIT_TABLE;

///////////////////////////////////////////////////////////////////////////////
// EFI_TO_COMPATIBILITY16_BOOT_TABLE
///////////////////////////////////////////////////////////////////////////////

//
// DEVICE_PRODUCER_SERIAL & its modes
//
typedef struct {
  UINT16      Address;
  UINT8       Irq;
  SERIAL_MODE Mode;
} DEVICE_PRODUCER_SERIAL;

#define DEVICE_SERIAL_MODE_NORMAL               0x00
#define DEVICE_SERIAL_MODE_IRDA                 0x01
#define DEVICE_SERIAL_MODE_ASK_IR               0x02
#define DEVICE_SERIAL_MODE_DUPLEX_HALF          0x00
#define DEVICE_SERIAL_MODE_DUPLEX_FULL          0x10

//
// DEVICE_PRODUCER_PARALLEL & its modes
//
typedef struct {
  UINT16        Address;
  UINT8         Irq;
  UINT8         Dma;
  PARALLEL_MODE Mode;
} DEVICE_PRODUCER_PARALLEL;

#define DEVICE_PARALLEL_MODE_MODE_OUTPUT_ONLY   0x00
#define DEVICE_PARALLEL_MODE_MODE_BIDIRECTIONAL 0x01
#define DEVICE_PARALLEL_MODE_MODE_EPP           0x02
#define DEVICE_PARALLEL_MODE_MODE_ECP           0x03

//
// DEVICE_PRODUCER_FLOPPY
//
typedef struct {
  UINT16  Address;
  UINT8   Irq;
  UINT8   Dma;
  UINT8   NumberOfFloppy;
} DEVICE_PRODUCER_FLOPPY;

//
// LEGACY_DEVICE_FLAGS
//
typedef struct {
  UINT32  A20Kybd : 1;
  UINT32  A20Port90 : 1;
  UINT32  Reserved : 30;
} LEGACY_DEVICE_FLAGS;

//
// DEVICE_PRODUCER_DATA_HEADER
//
typedef struct {
  DEVICE_PRODUCER_SERIAL    Serial[4];
  DEVICE_PRODUCER_PARALLEL  Parallel[3];
  DEVICE_PRODUCER_FLOPPY    Floppy;
  UINT8                     MousePresent;
  LEGACY_DEVICE_FLAGS       Flags;
} DEVICE_PRODUCER_DATA_HEADER;

//
// ATAPI_IDENTIFY
//
typedef struct {
  UINT16  Raw[256];
} ATAPI_IDENTIFY;

//
// HDD_INFO & its status
//
typedef struct {
  UINT16          Status;
  UINT32          Bus;
  UINT32          Device;
  UINT32          Function;
  UINT16          CommandBaseAddress;
  UINT16          ControlBaseAddress;
  UINT16          BusMasterAddress;
  UINT8           HddIrq;
  ATAPI_IDENTIFY  IdentifyDrive[2];
} HDD_INFO;

#define HDD_PRIMARY               0x01
#define HDD_SECONDARY             0x02
#define HDD_MASTER_ATAPI_CDROM    0x04
#define HDD_SLAVE_ATAPI_CDROM     0x08
#define HDD_MASTER_IDE            0x20
#define HDD_SLAVE_IDE             0x40
#define HDD_MASTER_ATAPI_ZIPDISK  0x10
#define HDD_SLAVE_ATAPI_ZIPDISK   0x80

//
// BBS_STATUS_FLAGS
//
typedef struct {
  UINT16  OldPosition : 4;
  UINT16  Reserved1 : 4;
  UINT16  Enabled : 1;
  UINT16  Failed : 1;
  UINT16  MediaPresent : 2;
  UINT16  Reserved2 : 4;
} BBS_STATUS_FLAGS;

//
// BBS_TABLE, device type values & boot priority values
//
typedef struct {
  UINT16            BootPriority;
  UINT32            Bus;
  UINT32            Device;
  UINT32            Function;
  UINT8             Class;
  UINT8             SubClass;
  UINT16            MfgStringOffset;
  UINT16            MfgStringSegment;
  UINT16            DeviceType;
  BBS_STATUS_FLAGS  StatusFlags;
  UINT16            BootHandlerOffset;
  UINT16            BootHandlerSegment;
  UINT16            DescStringOffset;
  UINT16            DescStringSegment;
  UINT32            InitPerReserved;
  UINT32            AdditionalIrq13Handler;
  UINT32            AdditionalIrq18Handler;
  UINT32            AdditionalIrq19Handler;
  UINT32            AdditionalIrq40Handler;
  UINT8             AssignedDriveNumber;
  UINT32            AdditionalIrq41Handler;
  UINT32            AdditionalIrq46Handler;
  UINT32            IBV1;
  UINT32            IBV2;
} BBS_TABLE;

#define BBS_FLOPPY        0x01
#define BBS_HARDDISK      0x02
#define BBS_CDROM         0x03
#define BBS_PCMCIA        0x04
#define BBS_USB           0x05
#define BBS_EMBED_NETWORK 0x06
#define BBS_BEV_DEVICE    0x80
#define BBS_UNKNOWN       0xff

#define BBS_DO_NOT_BOOT_FROM    0xFFFC
#define BBS_LOWEST_PRIORITY     0xFFFD
#define BBS_UNPRIORITIZED_ENTRY 0xFFFE
#define BBS_IGNORE_ENTRY        0xFFFF

//
// SMM_ATTRIBUTES & relating type, port and data size constants
//
typedef struct {
  UINT16  Type : 3;
  UINT16  PortGranularity : 3;
  UINT16  DataGranularity : 3;
  UINT16  Reserved : 7;
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

//
// SMM_FUNCTION & relating constants
//
typedef struct {
  UINT16  Function : 15;
  UINT16  Owner : 1;
} SMM_FUNCTION;

#define INT15_D042        0x0000
#define GET_USB_BOOT_INFO 0x0001
#define DMI_PNP_50_57     0x0002

#define STANDARD_OWNER    0x0
#define OEM_OWNER         0x1

//
// SMM_ENTRY
//
// This structure assumes both port and data sizes are 1. SmmAttribute must be
// properly to reflect that assumption.
//
typedef struct {
  SMM_ATTRIBUTES  SmmAttributes;
  SMM_FUNCTION    SmmFunction;
  UINT8           SmmPort;
  UINT8           SmmData;
} SMM_ENTRY;

//
// SMM_TABLE
//
typedef struct {
  UINT16    NumSmmEntries;
  SMM_ENTRY SmmEntry;
} SMM_TABLE;

//
// UDC_ATTRIBUTES
//
typedef struct {
  UINT8 DirectoryServiceValidity : 1;
  UINT8 RabcaUsedFlag : 1;
  UINT8 ExecuteHddDiagnosticsFlag : 1;
  UINT8 Reserved : 5;
} UDC_ATTRIBUTES;

//
// UD_TABLE
//
typedef struct {
  UDC_ATTRIBUTES  Attributes;
  UINT8           DeviceNumber;
  UINT8           BbsTableEntryNumberForParentDevice;
  UINT8           BbsTableEntryNumberForBoot;
  UINT8           BbsTableEntryNumberForHddDiag;
  UINT8           BeerData[128];
  UINT8           ServiceAreaData[64];
} UD_TABLE;

//
// EFI_TO_COMPATIBILITY16_BOOT_TABLE
//
#define EFI_TO_LEGACY_MAJOR_VERSION 0x02
#define EFI_TO_LEGACY_MINOR_VERSION 0x00
#define MAX_IDE_CONTROLLER          8

typedef struct {
  UINT16                      MajorVersion;
  UINT16                      MinorVersion;
  UINT32                      AcpiTable;   // 4 GB range
  UINT32                      SmbiosTable; // 4 GB range
  UINT32                      SmbiosTableLength;

  //
  // Legacy SIO state
  //
  DEVICE_PRODUCER_DATA_HEADER SioData;

  UINT16                      DevicePathType;
  UINT16                      PciIrqMask;
  UINT32                      NumberE820Entries;

  //
  // Controller & Drive Identify[2] per controller information
  //
  HDD_INFO                    HddInfo[MAX_IDE_CONTROLLER];

  UINT32                      NumberBbsEntries;
  UINT32                      BbsTable;
  UINT32                      SmmTable;
  UINT32                      OsMemoryAbove1Mb;
  UINT32                      UnconventionalDeviceTable;
} EFI_TO_COMPATIBILITY16_BOOT_TABLE;

///////////////////////////////////////////////////////////////////////////////
// EFI_LEGACY_INSTALL_PCI_HANDLER
///////////////////////////////////////////////////////////////////////////////
typedef struct {
  UINT8   PciBus;
  UINT8   PciDeviceFun;
  UINT8   PciSegment;
  UINT8   PciClass;
  UINT8   PciSubclass;
  UINT8   PciInterface;

  //
  // Primary section
  //
  UINT8   PrimaryIrq;
  UINT8   PrimaryReserved;
  UINT16  PrimaryControl;
  UINT16  PrimaryBase;
  UINT16  PrimaryBusMaster;

  //
  // Secondary Section
  //
  UINT8   SecondaryIrq;
  UINT8   SecondaryReserved;
  UINT16  SecondaryControl;
  UINT16  SecondaryBase;
  UINT16  SecondaryBusMaster;
} EFI_LEGACY_INSTALL_PCI_HANDLER;

//
// Restore default pack value
//
#pragma pack()

#endif
