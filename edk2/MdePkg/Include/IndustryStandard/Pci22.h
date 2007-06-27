/** @file
  Support for PCI 2.2 standard.

  Copyright (c) 2006 - 2007, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _PCI22_H
#define _PCI22_H

#define PCI_MAX_SEGMENT 0

#define PCI_MAX_BUS     255

#define PCI_MAX_DEVICE  31
#define PCI_MAX_FUNC    7

//
// Command
//
#define PCI_VGA_PALETTE_SNOOP_DISABLED  0x20

#pragma pack(push, 1)
typedef struct {
  UINT16  VendorId;
  UINT16  DeviceId;
  UINT16  Command;
  UINT16  Status;
  UINT8   RevisionID;
  UINT8   ClassCode[3];
  UINT8   CacheLineSize;
  UINT8   LatencyTimer;
  UINT8   HeaderType;
  UINT8   BIST;
} PCI_DEVICE_INDEPENDENT_REGION;

typedef struct {
  UINT32  Bar[6];
  UINT32  CISPtr;
  UINT16  SubsystemVendorID;
  UINT16  SubsystemID;
  UINT32  ExpansionRomBar;
  UINT8   CapabilityPtr;
  UINT8   Reserved1[3];
  UINT32  Reserved2;
  UINT8   InterruptLine;
  UINT8   InterruptPin;
  UINT8   MinGnt;
  UINT8   MaxLat;
} PCI_DEVICE_HEADER_TYPE_REGION;

typedef struct {
  PCI_DEVICE_INDEPENDENT_REGION Hdr;
  PCI_DEVICE_HEADER_TYPE_REGION Device;
} PCI_TYPE00;

typedef struct {
  UINT32  Bar[2];
  UINT8   PrimaryBus;
  UINT8   SecondaryBus;
  UINT8   SubordinateBus;
  UINT8   SecondaryLatencyTimer;
  UINT8   IoBase;
  UINT8   IoLimit;
  UINT16  SecondaryStatus;
  UINT16  MemoryBase;
  UINT16  MemoryLimit;
  UINT16  PrefetchableMemoryBase;
  UINT16  PrefetchableMemoryLimit;
  UINT32  PrefetchableBaseUpper32;
  UINT32  PrefetchableLimitUpper32;
  UINT16  IoBaseUpper16;
  UINT16  IoLimitUpper16;
  UINT8   CapabilityPtr;
  UINT8   Reserved[3];
  UINT32  ExpansionRomBAR;
  UINT8   InterruptLine;
  UINT8   InterruptPin;
  UINT16  BridgeControl;
} PCI_BRIDGE_CONTROL_REGISTER;

typedef struct {
  PCI_DEVICE_INDEPENDENT_REGION Hdr;
  PCI_BRIDGE_CONTROL_REGISTER   Bridge;
} PCI_TYPE01;

typedef union {
  PCI_TYPE00  Device;
  PCI_TYPE01  Bridge;
} PCI_TYPE_GENERIC;

typedef struct {
  UINT32  CardBusSocketReg; // Cardus Socket/ExCA Base
  // Address Register
  //
  UINT16  Reserved;
  UINT16  SecondaryStatus;      // Secondary Status
  UINT8   PciBusNumber;         // PCI Bus Number
  UINT8   CardBusBusNumber;     // CardBus Bus Number
  UINT8   SubordinateBusNumber; // Subordinate Bus Number
  UINT8   CardBusLatencyTimer;  // CardBus Latency Timer
  UINT32  MemoryBase0;          // Memory Base Register 0
  UINT32  MemoryLimit0;         // Memory Limit Register 0
  UINT32  MemoryBase1;
  UINT32  MemoryLimit1;
  UINT32  IoBase0;
  UINT32  IoLimit0;             // I/O Base Register 0
  UINT32  IoBase1;              // I/O Limit Register 0
  UINT32  IoLimit1;
  UINT8   InterruptLine;        // Interrupt Line
  UINT8   InterruptPin;         // Interrupt Pin
  UINT16  BridgeControl;        // Bridge Control
} PCI_CARDBUS_CONTROL_REGISTER;

//
// Definitions of PCI class bytes and manipulation macros.
//
#define PCI_CLASS_OLD                 0x00
#define PCI_CLASS_OLD_OTHER           0x00
#define PCI_CLASS_OLD_VGA             0x01

#define PCI_CLASS_MASS_STORAGE        0x01
#define PCI_CLASS_MASS_STORAGE_SCSI   0x00
#define PCI_CLASS_MASS_STORAGE_IDE    0x01  // obsolete
#define PCI_CLASS_IDE                 0x01
#define PCI_CLASS_MASS_STORAGE_FLOPPY 0x02
#define PCI_CLASS_MASS_STORAGE_IPI    0x03
#define PCI_CLASS_MASS_STORAGE_RAID   0x04
#define PCI_CLASS_MASS_STORAGE_OTHER  0x80

#define PCI_CLASS_NETWORK             0x02
#define PCI_CLASS_NETWORK_ETHERNET    0x00
#define PCI_CLASS_ETHERNET            0x00  // obsolete
#define PCI_CLASS_NETWORK_TOKENRING   0x01
#define PCI_CLASS_NETWORK_FDDI        0x02
#define PCI_CLASS_NETWORK_ATM         0x03
#define PCI_CLASS_NETWORK_ISDN        0x04
#define PCI_CLASS_NETWORK_OTHER       0x80

#define PCI_CLASS_DISPLAY             0x03
#define PCI_CLASS_DISPLAY_CTRL        0x03  // obsolete
#define PCI_CLASS_DISPLAY_VGA         0x00
#define PCI_CLASS_VGA                 0x00  // obsolete
#define PCI_CLASS_DISPLAY_XGA         0x01
#define PCI_CLASS_DISPLAY_3D          0x02
#define PCI_CLASS_DISPLAY_OTHER       0x80
#define PCI_CLASS_DISPLAY_GFX         0x80
#define PCI_CLASS_GFX                 0x80  // obsolete
#define PCI_CLASS_BRIDGE              0x06
#define PCI_CLASS_BRIDGE_HOST         0x00
#define PCI_CLASS_BRIDGE_ISA          0x01
#define PCI_CLASS_ISA                 0x01  // obsolete
#define PCI_CLASS_BRIDGE_EISA         0x02
#define PCI_CLASS_BRIDGE_MCA          0x03
#define PCI_CLASS_BRIDGE_P2P          0x04
#define PCI_CLASS_BRIDGE_PCMCIA       0x05
#define PCI_CLASS_BRIDGE_NUBUS        0x06
#define PCI_CLASS_BRIDGE_CARDBUS      0x07
#define PCI_CLASS_BRIDGE_RACEWAY      0x08
#define PCI_CLASS_BRIDGE_ISA_PDECODE  0x80
#define PCI_CLASS_ISA_POSITIVE_DECODE 0x80  // obsolete

#define PCI_CLASS_SCC                 0x07  // Simple communications controllers 
#define PCI_SUBCLASS_SERIAL           0x00
#define PCI_IF_GENERIC_XT             0x00
#define PCI_IF_16450                  0x01
#define PCI_IF_16550                  0x02
#define PCI_IF_16650                  0x03
#define PCI_IF_16750                  0x04
#define PCI_IF_16850                  0x05
#define PCI_IF_16950                  0x06
#define PCI_SUBCLASS_PARALLEL         0x01
#define PCI_IF_PARALLEL_PORT          0x00
#define PCI_IF_BI_DIR_PARALLEL_PORT   0x01
#define PCI_IF_ECP_PARALLEL_PORT      0x02
#define PCI_IF_1284_CONTROLLER        0x03
#define PCI_IF_1284_DEVICE            0xFE
#define PCI_SUBCLASS_MULTIPORT_SERIAL 0x02
#define PCI_SUBCLASS_MODEM            0x03
#define PCI_IF_GENERIC_MODEM          0x00
#define PCI_IF_16450_MODEM            0x01
#define PCI_IF_16550_MODEM            0x02
#define PCI_IF_16650_MODEM            0x03
#define PCI_IF_16750_MODEM            0x04
#define PCI_SUBCLASS_OTHER            0x80

#define PCI_CLASS_SYSTEM_PERIPHERAL   0x08
#define PCI_SUBCLASS_PIC              0x00
#define PCI_IF_8259_PIC               0x00
#define PCI_IF_ISA_PIC                0x01
#define PCI_IF_EISA_PIC               0x02
#define PCI_IF_APIC_CONTROLLER        0x10 // I/O APIC interrupt controller , 32 bye none-prefectable memory.  
#define PCI_IF_APIC_CONTROLLER2       0x20 
#define PCI_SUBCLASS_TIMER            0x02
#define PCI_IF_8254_TIMER             0x00
#define PCI_IF_ISA_TIMER              0x01
#define PCI_EISA_TIMER                0x02
#define PCI_SUBCLASS_RTC              0x03
#define PCI_IF_GENERIC_RTC            0x00
#define PCI_IF_ISA_RTC                0x00
#define PCI_SUBCLASS_PNP_CONTROLLER   0x04 // HotPlug Controller

#define PCI_CLASS_INPUT_DEVICE        0x09
#define PCI_SUBCLASS_KEYBOARD         0x00
#define PCI_SUBCLASS_PEN              0x01
#define PCI_SUBCLASS_MOUSE_CONTROLLER 0x02
#define PCI_SUBCLASS_SCAN_CONTROLLER  0x03
#define PCI_SUBCLASS_GAMEPORT         0x04

#define PCI_CLASS_DOCKING_STATION     0x0A

#define PCI_CLASS_PROCESSOR           0x0B
#define PCI_SUBCLASS_PROC_386         0x00
#define PCI_SUBCLASS_PROC_486         0x01
#define PCI_SUBCLASS_PROC_PENTIUM     0x02
#define PCI_SUBCLASS_PROC_ALPHA       0x10
#define PCI_SUBCLASS_PROC_POWERPC     0x20
#define PCI_SUBCLASS_PROC_MIPS        0x30
#define PCI_SUBCLASS_PROC_CO_PORC     0x40 // Co-Processor

#define PCI_CLASS_SERIAL              0x0C
#define PCI_CLASS_SERIAL_FIREWIRE     0x00
#define PCI_CLASS_SERIAL_ACCESS_BUS   0x01
#define PCI_CLASS_SERIAL_SSA          0x02
#define PCI_CLASS_SERIAL_USB          0x03
#define PCI_IF_EHCI                   0x20
#define PCI_CLASS_SERIAL_FIBRECHANNEL 0x04
#define PCI_CLASS_SERIAL_SMB          0x05

#define PCI_CLASS_WIRELESS            0x0D
#define PCI_SUBCLASS_IRDA             0x00
#define PCI_SUBCLASS_IR               0x01
#define PCI_SUBCLASS_RF               0x02

#define PCI_CLASS_INTELLIGENT_IO      0x0E

#define PCI_CLASS_SATELLITE           0x0F
#define PCI_SUBCLASS_TV               0x01
#define PCI_SUBCLASS_AUDIO            0x02
#define PCI_SUBCLASS_VOICE            0x03
#define PCI_SUBCLASS_DATA             0x04

#define PCI_SECURITY_CONTROLLER       0x10 // Encryption and decryption controller
#define PCI_SUBCLASS_NET_COMPUT       0x00
#define PCI_SUBCLASS_ENTERTAINMENT    0x10 

#define PCI_CLASS_DPIO                0x11

#define IS_CLASS1(_p, c)              ((_p)->Hdr.ClassCode[2] == (c))
#define IS_CLASS2(_p, c, s)           (IS_CLASS1 (_p, c) && ((_p)->Hdr.ClassCode[1] == (s)))
#define IS_CLASS3(_p, c, s, p)        (IS_CLASS2 (_p, c, s) && ((_p)->Hdr.ClassCode[0] == (p)))

#define IS_PCI_DISPLAY(_p)            IS_CLASS1 (_p, PCI_CLASS_DISPLAY)
#define IS_PCI_VGA(_p)                IS_CLASS3 (_p, PCI_CLASS_DISPLAY, PCI_CLASS_DISPLAY_VGA, 0)
#define IS_PCI_8514(_p)               IS_CLASS3 (_p, PCI_CLASS_DISPLAY, PCI_CLASS_DISPLAY_VGA, 1)
#define IS_PCI_GFX(_p)                IS_CLASS3 (_p, PCI_CLASS_DISPLAY, PCI_CLASS_DISPLAY_GFX, 0)
#define IS_PCI_OLD(_p)                IS_CLASS1 (_p, PCI_CLASS_OLD)
#define IS_PCI_OLD_VGA(_p)            IS_CLASS2 (_p, PCI_CLASS_OLD, PCI_CLASS_OLD_VGA)
#define IS_PCI_IDE(_p)                IS_CLASS2 (_p, PCI_CLASS_MASS_STORAGE, PCI_CLASS_MASS_STORAGE_IDE)
#define IS_PCI_SCSI(_p)               IS_CLASS3 (_p, PCI_CLASS_MASS_STORAGE, PCI_CLASS_MASS_STORAGE_SCSI, 0)
#define IS_PCI_RAID(_p)               IS_CLASS3 (_p, PCI_CLASS_MASS_STORAGE, PCI_CLASS_MASS_STORAGE_RAID, 0)
#define IS_PCI_LPC(_p)                IS_CLASS3 (_p, PCI_CLASS_BRIDGE, PCI_CLASS_BRIDGE_ISA, 0)
#define IS_PCI_P2P(_p)                IS_CLASS3 (_p, PCI_CLASS_BRIDGE, PCI_CLASS_BRIDGE_P2P, 0)
#define IS_PCI_P2P_SUB(_p)            IS_CLASS3 (_p, PCI_CLASS_BRIDGE, PCI_CLASS_BRIDGE_P2P, 1)
#define IS_PCI_16550_SERIAL(_p)       IS_CLASS3 (_p, PCI_CLASS_SCC, PCI_SUBCLASS_SERIAL, PCI_IF_16550)
#define IS_PCI_USB(_p)                IS_CLASS2 (_p, PCI_CLASS_SERIAL, PCI_CLASS_SERIAL_USB)

#define HEADER_TYPE_DEVICE            0x00
#define HEADER_TYPE_PCI_TO_PCI_BRIDGE 0x01
#define HEADER_TYPE_CARDBUS_BRIDGE    0x02

#define HEADER_TYPE_MULTI_FUNCTION    0x80
#define HEADER_LAYOUT_CODE            0x7f

#define IS_PCI_BRIDGE(_p)             (((_p)->Hdr.HeaderType & HEADER_LAYOUT_CODE) == (HEADER_TYPE_PCI_TO_PCI_BRIDGE))
#define IS_CARDBUS_BRIDGE(_p)         (((_p)->Hdr.HeaderType & HEADER_LAYOUT_CODE) == (HEADER_TYPE_CARDBUS_BRIDGE))
#define IS_PCI_MULTI_FUNC(_p)         ((_p)->Hdr.HeaderType & HEADER_TYPE_MULTI_FUNCTION)

#define PCI_DEVICE_ROMBAR             0x30
#define PCI_BRIDGE_ROMBAR             0x38

#define PCI_MAX_BAR                   0x0006
#define PCI_MAX_CONFIG_OFFSET         0x0100

#define PCI_VENDOR_ID_OFFSET                        0x00
#define PCI_DEVICE_ID_OFFSET                        0x02
#define PCI_COMMAND_OFFSET                          0x04
#define PCI_PRIMARY_STATUS_OFFSET                   0x06
#define PCI_REVISION_ID_OFFSET                      0x08
#define PCI_CLASSCODE_OFFSET                        0x09
#define PCI_CACHELINE_SIZE_OFFSET                   0x0C
#define PCI_LATENCY_TIMER_OFFSET                    0x0D
#define PCI_HEADER_TYPE_OFFSET                      0x0E
#define PCI_BIST_OFFSET                             0x0F
#define PCI_BASE_ADDRESSREG_OFFSET                  0x10
#define PCI_CARDBUS_CIS_OFFSET                      0x28
#define PCI_SVID_OFFSET                             0x2C // SubSystem Vendor id
#define PCI_SUBSYSTEM_VENDOR_ID_OFFSET              0x2C
#define PCI_SID_OFFSET                              0x2E // SubSystem ID
#define PCI_SUBSYSTEM_ID_OFFSET                     0x2E
#define PCI_EXPANSION_ROM_BASE                      0x30
#define PCI_CAPBILITY_POINTER_OFFSET                0x34
#define PCI_INT_LINE_OFFSET                         0x3C // Interrupt Line Register
#define PCI_INT_PIN_OFFSET                          0x3D // Interrupt Pin Register
#define PCI_MAXGNT_OFFSET                           0x3E // Max Grant Register
#define PCI_MAXLAT_OFFSET                           0x3F // Max Latency Register

#define PCI_BRIDGE_CONTROL_REGISTER_OFFSET          0x3E
#define PCI_BRIDGE_STATUS_REGISTER_OFFSET           0x1E

#define PCI_BRIDGE_PRIMARY_BUS_REGISTER_OFFSET      0x18
#define PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET    0x19
#define PCI_BRIDGE_SUBORDINATE_BUS_REGISTER_OFFSET  0x1a

//
// Interrupt Line "Unknown" or "No connection" value defined for x86 based system
//
#define PCI_INT_LINE_UNKNOWN                        0xFF               

typedef union {
  struct {
    UINT32  Reg : 8;
    UINT32  Func : 3;
    UINT32  Dev : 5;
    UINT32  Bus : 8;
    UINT32  Reserved : 7;
    UINT32  Enable : 1;
  } Bits;
  UINT32  Uint32;
} PCI_CONFIG_ACCESS_CF8;

#pragma pack()

#define PCI_EXPANSION_ROM_HEADER_SIGNATURE              0xaa55
#define PCI_DATA_STRUCTURE_SIGNATURE                    EFI_SIGNATURE_32 ('P', 'C', 'I', 'R')
#define PCI_CODE_TYPE_PCAT_IMAGE                        0x00
#define PCI_CODE_TYPE_EFI_IMAGE                         0x03
#define EFI_PCI_EXPANSION_ROM_HEADER_COMPRESSED         0x0001

#define EFI_PCI_COMMAND_IO_SPACE                        0x0001
#define EFI_PCI_COMMAND_MEMORY_SPACE                    0x0002
#define EFI_PCI_COMMAND_BUS_MASTER                      0x0004
#define EFI_PCI_COMMAND_SPECIAL_CYCLE                   0x0008
#define EFI_PCI_COMMAND_MEMORY_WRITE_AND_INVALIDATE     0x0010
#define EFI_PCI_COMMAND_VGA_PALETTE_SNOOP               0x0020
#define EFI_PCI_COMMAND_PARITY_ERROR_RESPOND            0x0040
#define EFI_PCI_COMMAND_STEPPING_CONTROL                0x0080
#define EFI_PCI_COMMAND_SERR                            0x0100
#define EFI_PCI_COMMAND_FAST_BACK_TO_BACK               0x0200

#define EFI_PCI_BRIDGE_CONTROL_PARITY_ERROR_RESPONSE    0x0001
#define EFI_PCI_BRIDGE_CONTROL_SERR                     0x0002
#define EFI_PCI_BRIDGE_CONTROL_ISA                      0x0004
#define EFI_PCI_BRIDGE_CONTROL_VGA                      0x0008
#define EFI_PCI_BRIDGE_CONTROL_VGA_16                   0x0010
#define EFI_PCI_BRIDGE_CONTROL_MASTER_ABORT             0x0020
#define EFI_PCI_BRIDGE_CONTROL_RESET_SECONDARY_BUS      0x0040
#define EFI_PCI_BRIDGE_CONTROL_FAST_BACK_TO_BACK        0x0080
#define EFI_PCI_BRIDGE_CONTROL_PRIMARY_DISCARD_TIMER    0x0100
#define EFI_PCI_BRIDGE_CONTROL_SECONDARY_DISCARD_TIMER  0x0200
#define EFI_PCI_BRIDGE_CONTROL_TIMER_STATUS             0x0400
#define EFI_PCI_BRIDGE_CONTROL_DISCARD_TIMER_SERR       0x0800

//
// Following are the PCI-CARDBUS bridge control bit
//
#define EFI_PCI_BRIDGE_CONTROL_IREQINT_ENABLE       0x0080
#define EFI_PCI_BRIDGE_CONTROL_RANGE0_MEMORY_TYPE   0x0100
#define EFI_PCI_BRIDGE_CONTROL_RANGE1_MEMORY_TYPE   0x0200
#define EFI_PCI_BRIDGE_CONTROL_WRITE_POSTING_ENABLE 0x0400

//
// Following are the PCI status control bit
//
#define EFI_PCI_STATUS_CAPABILITY             0x0010
#define EFI_PCI_STATUS_66MZ_CAPABLE           0x0020
#define EFI_PCI_FAST_BACK_TO_BACK_CAPABLE     0x0080
#define EFI_PCI_MASTER_DATA_PARITY_ERROR      0x0100

#define EFI_PCI_CAPABILITY_PTR                0x34
#define EFI_PCI_CARDBUS_BRIDGE_CAPABILITY_PTR 0x14

#pragma pack(1)
typedef struct {
  UINT16  Signature;    // 0xaa55
  UINT8   Reserved[0x16];
  UINT16  PcirOffset;
} PCI_EXPANSION_ROM_HEADER;

typedef struct {
  UINT16  Signature;    // 0xaa55
  UINT8   Size512;
  UINT8   InitEntryPoint[3];
  UINT8   Reserved[0x12];
  UINT16  PcirOffset;
} EFI_LEGACY_EXPANSION_ROM_HEADER;

typedef struct {
  UINT32  Signature;    // "PCIR"
  UINT16  VendorId;
  UINT16  DeviceId;
  UINT16  Reserved0;
  UINT16  Length;
  UINT8   Revision;
  UINT8   ClassCode[3];
  UINT16  ImageLength;
  UINT16  CodeRevision;
  UINT8   CodeType;
  UINT8   Indicator;
  UINT16  Reserved1;
} PCI_DATA_STRUCTURE;

//
// PCI Capability List IDs and records
//
#define EFI_PCI_CAPABILITY_ID_PMI     0x01
#define EFI_PCI_CAPABILITY_ID_AGP     0x02
#define EFI_PCI_CAPABILITY_ID_VPD     0x03
#define EFI_PCI_CAPABILITY_ID_SLOTID  0x04
#define EFI_PCI_CAPABILITY_ID_MSI     0x05
#define EFI_PCI_CAPABILITY_ID_HOTPLUG 0x06
#define EFI_PCI_CAPABILITY_ID_PCIX    0x07

typedef struct {
  UINT8 CapabilityID;
  UINT8 NextItemPtr;
} EFI_PCI_CAPABILITY_HDR;

//
// Capability EFI_PCI_CAPABILITY_ID_PMI
//
typedef struct {
  EFI_PCI_CAPABILITY_HDR  Hdr;
  UINT16                  PMC;
  UINT16                  PMCSR;
  UINT8                   BridgeExtention;
  UINT8                   Data;
} EFI_PCI_CAPABILITY_PMI;

//
// Capability EFI_PCI_CAPABILITY_ID_AGP
//
typedef struct {
  EFI_PCI_CAPABILITY_HDR  Hdr;
  UINT8                   Rev;
  UINT8                   Reserved;
  UINT32                  Status;
  UINT32                  Command;
} EFI_PCI_CAPABILITY_AGP;

//
// Capability EFI_PCI_CAPABILITY_ID_VPD
//
typedef struct {
  EFI_PCI_CAPABILITY_HDR  Hdr;
  UINT16                  AddrReg;
  UINT32                  DataReg;
} EFI_PCI_CAPABILITY_VPD;

//
// Capability EFI_PCI_CAPABILITY_ID_SLOTID
//
typedef struct {
  EFI_PCI_CAPABILITY_HDR  Hdr;
  UINT8                   ExpnsSlotReg;
  UINT8                   ChassisNo;
} EFI_PCI_CAPABILITY_SLOTID;

//
// Capability EFI_PCI_CAPABILITY_ID_MSI
//
typedef struct {
  EFI_PCI_CAPABILITY_HDR  Hdr;
  UINT16                  MsgCtrlReg;
  UINT32                  MsgAddrReg;
  UINT16                  MsgDataReg;
} EFI_PCI_CAPABILITY_MSI32;

typedef struct {
  EFI_PCI_CAPABILITY_HDR  Hdr;
  UINT16                  MsgCtrlReg;
  UINT32                  MsgAddrRegLsdw;
  UINT32                  MsgAddrRegMsdw;
  UINT16                  MsgDataReg;
} EFI_PCI_CAPABILITY_MSI64;

//
// Capability EFI_PCI_CAPABILITY_ID_HOTPLUG
//
typedef struct {
  EFI_PCI_CAPABILITY_HDR  Hdr;
  //
  // not finished - fields need to go here
  //
} EFI_PCI_CAPABILITY_HOTPLUG;

//
// Capability EFI_PCI_CAPABILITY_ID_PCIX
//
typedef struct {
  EFI_PCI_CAPABILITY_HDR  Hdr;
  UINT16                  CommandReg;
  UINT32                  StatusReg;
} EFI_PCI_CAPABILITY_PCIX;

typedef struct {
  EFI_PCI_CAPABILITY_HDR  Hdr;
  UINT16                  SecStatusReg;
  UINT32                  StatusReg;
  UINT32                  SplitTransCtrlRegUp;
  UINT32                  SplitTransCtrlRegDn;
} EFI_PCI_CAPABILITY_PCIX_BRDG;

#define DEVICE_ID_NOCARE    0xFFFF

#define PCI_ACPI_UNUSED     0
#define PCI_BAR_NOCHANGE    0
#define PCI_BAR_OLD_ALIGN   0xFFFFFFFFFFFFFFFFULL
#define PCI_BAR_EVEN_ALIGN  0xFFFFFFFFFFFFFFFEULL
#define PCI_BAR_SQUAD_ALIGN 0xFFFFFFFFFFFFFFFDULL
#define PCI_BAR_DQUAD_ALIGN 0xFFFFFFFFFFFFFFFCULL

#define PCI_BAR_IDX0        0x00
#define PCI_BAR_IDX1        0x01
#define PCI_BAR_IDX2        0x02
#define PCI_BAR_IDX3        0x03
#define PCI_BAR_IDX4        0x04
#define PCI_BAR_IDX5        0x05
#define PCI_BAR_ALL         0xFF

#pragma pack(pop)

#endif
