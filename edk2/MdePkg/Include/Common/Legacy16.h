/** @file
  API between 16-bit Legacy BIOS and EFI

  We need to figure out what the 16-bit code is going to use to
  represent these data structures. Is a pointer SEG:OFF or 32-bit...

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    

  Module Name:  Legacy16.h

  @par Revision Reference:
  These definitions are from Compatibility Support Module Spec Version 0.96.

**/

#ifndef LEGACY_16_H_
#define LEGACY_16_H_

#define EFI_TO_LEGACY_MAJOR_VERSION 0x02
#define EFI_TO_LEGACY_MINOR_VERSION 0x00

#pragma pack(1)
//
// EFI Legacy to Legacy16 data
// EFI_COMPATIBILITY16_TABLE has been moved to LegacyBios protocol defn file.
//
typedef struct {
  //
  // Memory map used to start up Legacy16 code
  //
  UINT32  BiosLessThan1MB;
  UINT32  HiPmmMemory;
  UINT32  PmmMemorySizeInBytes;

  UINT16  ReverseThunkCallSegment;
  UINT16  ReverseThunkCallOffset;
  UINT32  NumberE820Entries;
  UINT32  OsMemoryAbove1Mb;
  UINT32  ThunkStart;
  UINT32  ThunkSizeInBytes;
  UINT32  LowPmmMemory;
  UINT32  LowPmmMemorySizeInBytes;
} EFI_TO_COMPATIBILITY16_INIT_TABLE;

#pragma pack()
//
// Legacy16 Call types
//
typedef enum {
  Legacy16InitializeYourself    = 0x0000,
  Legacy16UpdateBbs             = 0x0001,
  Legacy16PrepareToBoot         = 0x0002,
  Legacy16Boot                  = 0x0003,
  Legacy16RetrieveLastBootDevice= 0x0004,
  Legacy16DispatchOprom         = 0x0005,
  Legacy16GetTableAddress       = 0x0006,
  Legacy16SetKeyboardLeds       = 0x0007,
  Legacy16InstallPciHandler     = 0x0008,
} EFI_COMPATIBILITY_FUNCTIONS;

#define F0000Region 0x01
#define E0000Region 0x02
//
// Legacy16 call prototypes
//  Input:  AX = EFI_COMPATIBILITY16_FUNCTIONS for all functions.
//  Output: AX = Return status for all functions. It follows EFI error
//               codes.
//
//  Legacy16InitializeYourself
//    Description: This is the first call to 16-bit code. It allows the
//                 16-bit to perform any internal initialization.
//    Input:  ES:BX pointer to EFI_TO_LEGACY16_INIT_TABLE
//    Output:
//  Legacy16UpdateBbs
//    Description: The 16-bit code updates the BBS table for non-compliant
//                 devices.
//    Input:  ES:BX pointer to EFI_TO_COMPATIBILITY16_BOOT_TABLE
//    Output:
//  Legacy16PrepareToBoot
//    Description: This is the last call to 16-bit code where 0xE0000 -0xFFFFF
//                 is read/write. 16-bit code does any final clean up.
//    Input:  ES:BX pointer to EFI_TO_COMPATIBILITY16_BOOT_TABLE
//    Output:
//  Legacy16Boot
//    Description: Do INT19.
//    Input:
//    Output:
//  Legacy16RetrieveLastBootDevice
//    Description: Return the priority number of the device that booted.
//    Input:
//    Output: BX = priority number of the last attempted boot device.
//  Legacy16DispatchOprom
//    Description: Pass control to the specified OPROM. Allows the 16-bit
//                 code to rehook INT 13,18 and/or 19 from non-BBS
//                 compliant devices.
//    Input:  ES:DI = Segment:Offset of PnPInstallationCheck
//            SI = OPROM segment. Offset assumed to be 3.
//            BH = PCI bus number.
//            BL = PCI device * 8 | PCI function.
//    Output: BX = Number of BBS non-compliant drives detected. Return
//                 zero for BBS compliant devices.
//  Legacy16GetTableAddress
//    Description: Allocate an area in the 0xE0000-0xFFFFF region.
//    Input:  BX = Allocation region.
//                 0x0 = Any region
//                 Bit 0 = 0xF0000 region
//                 Bit 1 = 0xE0000 region
//                 Multiple bits can be set.
//            CX = Length in bytes requested
//            DX = Required address alignment
//                 Bit mapped. First non-zero bit from right to left is
//                 alignment.
//    Output: DS:BX is assigned region.
//            AX = EFI_OUT_OF_RESOURCES if request cannot be granted.
//  Legacy16SetKeyboardLeds
//    Description: Perform any special action when keyboard LEDS change.
//                 Other code performs the LED change and updates standard
//                 BDA locations. This is for non-standard operations.
//    Input:  CL = LED status. 1 = set.
//                 Bit 0 = Scroll lock
//                 Bit 1 = Num lock
//                 Bit 2 = Caps lock
//    Output:
//  Legacy16InstallPciHandler
//    Description: Provides 16-bit code a hook to establish an interrupt
//                 handler for any PCI device requiring a PCI interrupt
//                 but having no OPROM. This is called before interrupt
//                 is assigned. 8259 will be disabled(even if sharded)
//                 and PCI Interrupt Line unprogrammed. Other code will
//                 program 8259 and PCI Interrupt Line.
//    Input:  ES:BX Pointer to EFI_LEGACY_INSTALL_PCI_HANDLER strcture
//    Output:
//
typedef UINT8 SERIAL_MODE;
typedef UINT8 PARALLEL_MODE;

#pragma pack(1)

#define DEVICE_SERIAL_MODE_NORMAL               0x00
#define DEVICE_SERIAL_MODE_IRDA                 0x01
#define DEVICE_SERIAL_MODE_ASK_IR               0x02
#define DEVICE_SERIAL_MODE_DUPLEX_HALF          0x00
#define DEVICE_SERIAL_MODE_DUPLEX_FULL          0x10

#define DEVICE_PARALLEL_MODE_MODE_OUTPUT_ONLY   0x00
#define DEVICE_PARALLEL_MODE_MODE_BIDIRECTIONAL 0x01
#define DEVICE_PARALLEL_MODE_MODE_EPP           0x02
#define DEVICE_PARALLEL_MODE_MODE_ECP           0x03

typedef struct {
  UINT16      Address;
  UINT8       Irq;
  SERIAL_MODE Mode;
} DEVICE_PRODUCER_SERIAL;

typedef struct {
  UINT16        Address;
  UINT8         Irq;
  UINT8         Dma;
  PARALLEL_MODE Mode;
} DEVICE_PRODUCER_PARALLEL;

typedef struct {
  UINT16  Address;
  UINT8   Irq;
  UINT8   Dma;
  UINT8   NumberOfFloppy;
} DEVICE_PRODUCER_FLOPPY;

typedef struct {
  UINT32  A20Kybd : 1;
  UINT32  A20Port90 : 1;
  UINT32  Reserved : 30;
} LEGACY_DEVICE_FLAGS;

typedef struct {
  DEVICE_PRODUCER_SERIAL    Serial[4];
  DEVICE_PRODUCER_PARALLEL  Parallel[3];
  DEVICE_PRODUCER_FLOPPY    Floppy;
  UINT8                     MousePresent;
  LEGACY_DEVICE_FLAGS       Flags;
} DEVICE_PRODUCER_DATA_HEADER;
//
// SMM Table definitions
// SMM table has a header that provides the number of entries. Following
// the header is a variable length amount of data.
//

#define STANDARD_IO      0x00
#define STANDARD_MEMORY  0x01

#define PORT_SIZE_8   0x00
#define PORT_SIZE_16  0x01
#define PORT_SIZE_32  0x02
#define PORT_SIZE_64  0x03

#define DATA_SIZE_8   0x00
#define DATA_SIZE_16  0x01
#define DATA_SIZE_32  0x02
#define DATA_SIZE_64  0x03

typedef struct {
  UINT16  Type : 3;
  UINT16  PortGranularity : 3;
  UINT16  DataGranularity : 3;
  UINT16  Reserved : 7;
} SMM_ATTRIBUTES;

#define INT15_D042        0x0000
#define GET_USB_BOOT_INFO 0x0001
#define DMI_PNP_50_57     0x0002

#define STANDARD_OWNER    0x0
#define OEM_OWNER         0x1

typedef struct {
  UINT16  Function : 15;
  UINT16  Owner : 1;
} SMM_FUNCTION;

typedef struct {
  SMM_ATTRIBUTES  SmmAttributes;
  SMM_FUNCTION    SmmFunction;
  //
  // Data size depends upon SmmAttributes and ranges from 2 bytes to
  // 16 bytes
  //
  // bugbug how to do variable length Data
  //
  UINT8           SmmPort;
  UINT8           SmmData;
} SMM_ENTRY;

typedef struct {
  UINT16    NumSmmEntries;
  SMM_ENTRY SmmEntry;
} SMM_TABLE;

//
// If MAX_IDE_CONTROLLER changes value 16-bit legacy code needs to change
//
#define MAX_IDE_CONTROLLER  8

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

typedef struct {
  UINT8   PciBus;
  UINT8   PciDeviceFun;
  UINT8   PciSegment;
  UINT8   PciClass;
  UINT8   PciSubclass;
  UINT8   PciInterface;
  UINT8   PrimaryIrq;
  UINT8   PrimaryReserved;
  UINT16  PrimaryControl;
  UINT16  PrimaryBase;
  UINT16  PrimaryBusMaster;
  UINT8   SecondaryIrq;
  UINT8   SecondaryReserved;
  UINT16  SecondaryControl;
  UINT16  SecondaryBase;
  UINT16  SecondaryBusMaster;
} EFI_LEGACY_INSTALL_PCI_HANDLER;

typedef struct {
  UINT16  PnPInstallationCheckSegment;
  UINT16  PnPInstallationCheckOffset;
  UINT16  OpromSegment;
  UINT8   PciBus;
  UINT8   PciDeviceFunction;
  UINT8   NumberBbsEntries;
  VOID    *BbsTablePointer;

} EFI_DISPATCH_OPROM_TABLE;

#pragma pack()

#endif
