/** @file

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _LEGACY_BIOS_INTERFACE_
#define _LEGACY_BIOS_INTERFACE_


#include <FrameworkDxe.h>
#include <IndustryStandard/Pci.h>
#include <IndustryStandard/SmBios.h>
#include <IndustryStandard/Acpi10.h>

#include <Guid/SmBios.h>
#include <Guid/Acpi.h>
#include <Guid/DxeServices.h>
#include <Guid/LegacyBios.h>
#include <Guid/StatusCodeDataTypeId.h>
#include <Guid/ImageAuthentication.h>

#include <Protocol/BlockIo.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/PciIo.h>
#include <Protocol/Cpu.h>
#include <Protocol/Timer.h>
#include <Protocol/IsaIo.h>
#include <Protocol/LegacyRegion2.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/LegacyInterrupt.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/GenericMemoryTest.h>
#include <Protocol/LegacyBiosPlatform.h>
#include <Protocol/DevicePath.h>
#include <Protocol/Legacy8259.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/SerialIo.h>
#include <Protocol/SuperIo.h>
#include <Protocol/IoMmu.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/DebugAgentLib.h>

//
// BUGBUG: This entry maybe changed to PCD in future and wait for
//         redesign of BDS library
//
#define MAX_BBS_ENTRIES 0x100

//
// Thunk Status Codes
//   (These apply only to errors with the thunk and not to the code that was
//   thunked to.)
//
#define THUNK_OK              0x00
#define THUNK_ERR_A20_UNSUP   0x01
#define THUNK_ERR_A20_FAILED  0x02

//
// Vector base definitions
//
//
// 8259 Hardware definitions
//
#define LEGACY_MODE_BASE_VECTOR_MASTER     0x08
#define LEGACY_MODE_BASE_VECTOR_SLAVE      0x70

//
// The original PC used INT8-F for master PIC. Since these mapped over
// processor exceptions TIANO moved the master PIC to INT68-6F.
//
// The vector base for slave PIC is set as 0x70 for PC-AT compatibility.
//
#define PROTECTED_MODE_BASE_VECTOR_MASTER  0x68
#define PROTECTED_MODE_BASE_VECTOR_SLAVE   0x70

//
// When we call CSM16 functions, some CSM16 use es:[offset + 0xabcd] to get data passed from CSM32,
// offset + 0xabcd could overflow which exceeds 0xFFFF which is invalid in real mode.
// So this will keep offset as small as possible to avoid offset overflow in real mode.
//
#define NORMALIZE_EFI_SEGMENT(_Adr)      (UINT16) (((UINTN) (_Adr)) >> 4)
#define NORMALIZE_EFI_OFFSET(_Adr)       (UINT16) (((UINT16) ((UINTN) (_Adr))) & 0xf)

//
// Trace defines
//
//
#define LEGACY_BDA_TRACE    0x000
#define LEGACY_BIOS_TRACE   0x040
#define LEGACY_BOOT_TRACE   0x080
#define LEGACY_CMOS_TRACE   0x0C0
#define LEGACY_IDE_TRACE    0x100
#define LEGACY_MP_TRACE     0x140
#define LEGACY_PCI_TRACE    0x180
#define LEGACY_SIO_TRACE    0x1C0

#define LEGACY_PCI_TRACE_000 LEGACY_PCI_TRACE + 0x00
#define LEGACY_PCI_TRACE_001 LEGACY_PCI_TRACE + 0x01
#define LEGACY_PCI_TRACE_002 LEGACY_PCI_TRACE + 0x02
#define LEGACY_PCI_TRACE_003 LEGACY_PCI_TRACE + 0x03
#define LEGACY_PCI_TRACE_004 LEGACY_PCI_TRACE + 0x04
#define LEGACY_PCI_TRACE_005 LEGACY_PCI_TRACE + 0x05
#define LEGACY_PCI_TRACE_006 LEGACY_PCI_TRACE + 0x06
#define LEGACY_PCI_TRACE_007 LEGACY_PCI_TRACE + 0x07
#define LEGACY_PCI_TRACE_008 LEGACY_PCI_TRACE + 0x08
#define LEGACY_PCI_TRACE_009 LEGACY_PCI_TRACE + 0x09
#define LEGACY_PCI_TRACE_00A LEGACY_PCI_TRACE + 0x0A
#define LEGACY_PCI_TRACE_00B LEGACY_PCI_TRACE + 0x0B
#define LEGACY_PCI_TRACE_00C LEGACY_PCI_TRACE + 0x0C
#define LEGACY_PCI_TRACE_00D LEGACY_PCI_TRACE + 0x0D
#define LEGACY_PCI_TRACE_00E LEGACY_PCI_TRACE + 0x0E
#define LEGACY_PCI_TRACE_00F LEGACY_PCI_TRACE + 0x0F

#define BDA_VIDEO_MODE      0x49

#define IDE_PI_REGISTER_PNE     BIT0
#define IDE_PI_REGISTER_SNE     BIT2

typedef struct {
  UINTN   PciSegment;
  UINTN   PciBus;
  UINTN   PciDevice;
  UINTN   PciFunction;
  UINT32  ShadowAddress;
  UINT32  ShadowedSize;
  UINT8   DiskStart;
  UINT8   DiskEnd;
} ROM_INSTANCE_ENTRY;

//
// Values for RealModeGdt
//
#if defined (MDE_CPU_IA32)

#define NUM_REAL_GDT_ENTRIES  3
#define CONVENTIONAL_MEMORY_TOP 0xA0000   // 640 KB
#define INITIAL_VALUE_BELOW_1K  0x0

#elif defined (MDE_CPU_X64)

#define NUM_REAL_GDT_ENTRIES  8
#define CONVENTIONAL_MEMORY_TOP 0xA0000   // 640 KB
#define INITIAL_VALUE_BELOW_1K  0x0

#endif

#pragma pack(1)

//
// Define what a processor GDT looks like
//
typedef struct {
  UINT32  LimitLo : 16;
  UINT32  BaseLo : 16;
  UINT32  BaseMid : 8;
  UINT32  Type : 4;
  UINT32  System : 1;
  UINT32  Dpl : 2;
  UINT32  Present : 1;
  UINT32  LimitHi : 4;
  UINT32  Software : 1;
  UINT32  Reserved : 1;
  UINT32  DefaultSize : 1;
  UINT32  Granularity : 1;
  UINT32  BaseHi : 8;
} GDT32;

typedef struct {
  UINT16  LimitLow;
  UINT16  BaseLow;
  UINT8   BaseMid;
  UINT8   Attribute;
  UINT8   LimitHi;
  UINT8   BaseHi;
} GDT64;

//
// Define what a processor descriptor looks like
// This data structure must be kept in sync with ASM STRUCT in Thunk.inc
//
typedef struct {
  UINT16  Limit;
  UINT64  Base;
} DESCRIPTOR64;

typedef struct {
  UINT16  Limit;
  UINT32  Base;
} DESCRIPTOR32;

//
// Low stub lay out
//
#define LOW_STACK_SIZE      (8 * 1024)  // 8k?
#define EFI_MAX_E820_ENTRY  100
#define FIRST_INSTANCE      1
#define NOT_FIRST_INSTANCE  0

#if defined (MDE_CPU_IA32)
typedef struct {
  //
  // Space for the code
  //  The address of Code is also the beginning of the relocated Thunk code
  //
  CHAR8                             Code[4096]; // ?
  //
  // The address of the Reverse Thunk code
  //  Note that this member CONTAINS the address of the relocated reverse thunk
  //  code unlike the member variable 'Code', which IS the address of the Thunk
  //  code.
  //
  UINT32                            LowReverseThunkStart;

  //
  // Data for the code (cs releative)
  //
  DESCRIPTOR32                      GdtDesc;          // Protected mode GDT
  DESCRIPTOR32                      IdtDesc;          // Protected mode IDT
  UINT32                            FlatSs;
  UINT32                            FlatEsp;

  UINT32                            LowCodeSelector;  // Low code selector in GDT
  UINT32                            LowDataSelector;  // Low data selector in GDT
  UINT32                            LowStack;
  DESCRIPTOR32                      RealModeIdtDesc;

  //
  // real-mode GDT (temporary GDT with two real mode segment descriptors)
  //
  GDT32                             RealModeGdt[NUM_REAL_GDT_ENTRIES];
  DESCRIPTOR32                      RealModeGdtDesc;

  //
  // Members specifically for the reverse thunk
  //  The RevReal* members are used to store the current state of real mode
  //  before performing the reverse thunk.  The RevFlat* members must be set
  //  before calling the reverse thunk assembly code.
  //
  UINT16                            RevRealDs;
  UINT16                            RevRealSs;
  UINT32                            RevRealEsp;
  DESCRIPTOR32                      RevRealIdtDesc;
  UINT16                            RevFlatDataSelector;  // Flat data selector in GDT
  UINT32                            RevFlatStack;

  //
  // A low memory stack
  //
  CHAR8                             Stack[LOW_STACK_SIZE];

  //
  // Stack for flat mode after reverse thunk
  // @bug    - This may no longer be necessary if the reverse thunk interface
  //           is changed to have the flat stack in a different location.
  //
  CHAR8                             RevThunkStack[LOW_STACK_SIZE];

  //
  // Legacy16 Init memory map info
  //
  EFI_TO_COMPATIBILITY16_INIT_TABLE EfiToLegacy16InitTable;

  EFI_TO_COMPATIBILITY16_BOOT_TABLE EfiToLegacy16BootTable;

  CHAR8                             InterruptRedirectionCode[32];
  EFI_LEGACY_INSTALL_PCI_HANDLER    PciHandler;
  EFI_DISPATCH_OPROM_TABLE          DispatchOpromTable;
  BBS_TABLE                         BbsTable[MAX_BBS_ENTRIES];
} LOW_MEMORY_THUNK;

#elif defined (MDE_CPU_X64)

typedef struct {
  //
  // Space for the code
  //  The address of Code is also the beginning of the relocated Thunk code
  //
  CHAR8                             Code[4096]; // ?

  //
  // Data for the code (cs releative)
  //
  DESCRIPTOR64                      X64GdtDesc;          // Protected mode GDT
  DESCRIPTOR64                      X64IdtDesc;          // Protected mode IDT
  UINTN                             X64Ss;
  UINTN                             X64Esp;

  UINTN                             RealStack;
  DESCRIPTOR32                      RealModeIdtDesc;
  DESCRIPTOR32                      RealModeGdtDesc;

  //
  // real-mode GDT (temporary GDT with two real mode segment descriptors)
  //
  GDT64                             RealModeGdt[NUM_REAL_GDT_ENTRIES];
  UINT64                            PageMapLevel4;

  //
  // A low memory stack
  //
  CHAR8                             Stack[LOW_STACK_SIZE];

  //
  // Legacy16 Init memory map info
  //
  EFI_TO_COMPATIBILITY16_INIT_TABLE EfiToLegacy16InitTable;

  EFI_TO_COMPATIBILITY16_BOOT_TABLE EfiToLegacy16BootTable;

  CHAR8                             InterruptRedirectionCode[32];
  EFI_LEGACY_INSTALL_PCI_HANDLER    PciHandler;
  EFI_DISPATCH_OPROM_TABLE          DispatchOpromTable;
  BBS_TABLE                         BbsTable[MAX_BBS_ENTRIES];
} LOW_MEMORY_THUNK;

#endif

//
// PnP Expansion Header
//
typedef struct {
  UINT32  PnpSignature;
  UINT8   Revision;
  UINT8   Length;
  UINT16  NextHeader;
  UINT8   Reserved1;
  UINT8   Checksum;
  UINT32  DeviceId;
  UINT16  MfgPointer;
  UINT16  ProductNamePointer;
  UINT8   Class;
  UINT8   SubClass;
  UINT8   Interface;
  UINT8   DeviceIndicators;
  UINT16  Bcv;
  UINT16  DisconnectVector;
  UINT16  Bev;
  UINT16  Reserved2;
  UINT16  StaticResourceVector;
} LEGACY_PNP_EXPANSION_HEADER;

typedef struct {
  UINT8   PciSegment;
  UINT8   PciBus;
  UINT8   PciDevice;
  UINT8   PciFunction;
  UINT16  Vid;
  UINT16  Did;
  UINT16  SysSid;
  UINT16  SVid;
  UINT8   Class;
  UINT8   SubClass;
  UINT8   Interface;
  UINT8   Reserved;
  UINTN   RomStart;
  UINTN   ManufacturerString;
  UINTN   ProductNameString;
} LEGACY_ROM_AND_BBS_TABLE;

//
// Structure how EFI has mapped a devices HDD drive numbers.
// Boot to EFI aware OS or shell requires this mapping when
// 16-bit CSM assigns drive numbers.
// This mapping is ignored booting to a legacy OS.
//
typedef struct {
  UINT8 PciSegment;
  UINT8 PciBus;
  UINT8 PciDevice;
  UINT8 PciFunction;
  UINT8 StartDriveNumber;
  UINT8 EndDriveNumber;
} LEGACY_EFI_HDD_TABLE;

//
// This data is passed to Leacy16Boot
//
typedef enum {
  EfiAcpiAddressRangeMemory   = 1,
  EfiAcpiAddressRangeReserved = 2,
  EfiAcpiAddressRangeACPI     = 3,
  EfiAcpiAddressRangeNVS      = 4,
  EfiAddressRangePersistentMemory = 7
} EFI_ACPI_MEMORY_TYPE;

typedef struct {
  UINT64                BaseAddr;
  UINT64                Length;
  EFI_ACPI_MEMORY_TYPE  Type;
} EFI_E820_ENTRY64;

typedef struct {
  UINT32                BassAddrLow;
  UINT32                BaseAddrHigh;
  UINT32                LengthLow;
  UINT32                LengthHigh;
  EFI_ACPI_MEMORY_TYPE  Type;
} EFI_E820_ENTRY;

#pragma pack()

extern BBS_TABLE           *mBbsTable;

extern EFI_GENERIC_MEMORY_TEST_PROTOCOL *gGenMemoryTest;

extern BOOLEAN mEndOfDxe;

#define PORT_70 0x70
#define PORT_71 0x71

#define CMOS_0A     0x0a  ///< Status register A
#define CMOS_0D     0x0d  ///< Status register D
#define CMOS_0E     0x0e  ///< Diagnostic Status
#define CMOS_0F     0x0f  ///< Shutdown status
#define CMOS_10     0x10  ///< Floppy type
#define CMOS_12     0x12  ///< IDE type
#define CMOS_14     0x14  ///< Same as BDA 40:10
#define CMOS_15     0x15  ///< Low byte of base memory in 1k increments
#define CMOS_16     0x16  ///< High byte of base memory in 1k increments
#define CMOS_17     0x17  ///< Low byte of 1MB+ memory in 1k increments - max 15 MB
#define CMOS_18     0x18  ///< High byte of 1MB+ memory in 1k increments - max 15 MB
#define CMOS_19     0x19  ///< C: extended drive type
#define CMOS_1A     0x1a  ///< D: extended drive type
#define CMOS_2E     0x2e  ///< Most significient byte of standard checksum
#define CMOS_2F     0x2f  ///< Least significient byte of standard checksum
#define CMOS_30     0x30  ///< CMOS 0x17
#define CMOS_31     0x31  ///< CMOS 0x18
#define CMOS_32     0x32  ///< Century byte

//
// 8254 Timer registers
//
#define TIMER0_COUNT_PORT                         0x40
#define TIMER1_COUNT_PORT                         0x41
#define TIMER2_COUNT_PORT                         0x42
#define TIMER_CONTROL_PORT                        0x43

//
// Timer 0, Read/Write LSB then MSB, Square wave output, binary count use.
//
#define TIMER0_CONTROL_WORD         0x36

#define LEGACY_BIOS_INSTANCE_SIGNATURE  SIGNATURE_32 ('L', 'B', 'I', 'T')
typedef struct {
  UINTN                             Signature;

  EFI_HANDLE                        Handle;
  EFI_LEGACY_BIOS_PROTOCOL          LegacyBios;

  EFI_HANDLE                        ImageHandle;

  //
  // CPU Architectural Protocol
  //
  EFI_CPU_ARCH_PROTOCOL             *Cpu;

  //
  // Timer Architectural Protocol
  //
  EFI_TIMER_ARCH_PROTOCOL           *Timer;
  BOOLEAN                           TimerUses8254;

  //
  // Protocol to Lock and Unlock 0xc0000 - 0xfffff
  //
  EFI_LEGACY_REGION2_PROTOCOL       *LegacyRegion;

  EFI_LEGACY_BIOS_PLATFORM_PROTOCOL *LegacyBiosPlatform;

  //
  // Interrupt control for thunk and PCI IRQ
  //
  EFI_LEGACY_8259_PROTOCOL          *Legacy8259;

  //
  // PCI Interrupt PIRQ control
  //
  EFI_LEGACY_INTERRUPT_PROTOCOL     *LegacyInterrupt;

  //
  // Generic Memory Test
  //
  EFI_GENERIC_MEMORY_TEST_PROTOCOL  *GenericMemoryTest;

  //
  // TRUE if PCI Interrupt Line registers have been programmed.
  //
  BOOLEAN                           PciInterruptLine;

  //
  // Code space below 1MB needed by thunker to transition to real mode.
  // Contains stack and real mode code fragments
  //
  LOW_MEMORY_THUNK                  *IntThunk;

  //
  // Starting shadow address of the Legacy BIOS
  //
  UINT32                            BiosStart;
  UINT32                            LegacyBiosImageSize;

  //
  // Start of variables used by CsmItp.mac ITP macro file and/os LegacyBios
  //
  UINT8                             Dump[4];

  //
  // $EFI Legacy16 code entry info in memory < 1 MB;
  //
  EFI_COMPATIBILITY16_TABLE         *Legacy16Table;
  VOID                              *Legacy16InitPtr;
  VOID                              *Legacy16BootPtr;
  VOID                              *InternalIrqRoutingTable;
  UINT32                            NumberIrqRoutingEntries;
  VOID                              *BbsTablePtr;
  VOID                              *HddTablePtr;
  UINT32                            NumberHddControllers;

  //
  // Cached copy of Legacy16 entry point
  //
  UINT16                            Legacy16CallSegment;
  UINT16                            Legacy16CallOffset;

  //
  // Returned from $EFI and passed in to OPROMS
  //
  UINT16                            PnPInstallationCheckSegment;
  UINT16                            PnPInstallationCheckOffset;

  //
  // E820 table
  //
  EFI_E820_ENTRY                    E820Table[EFI_MAX_E820_ENTRY];
  UINT32                            NumberE820Entries;

  //
  // True if legacy VGA INT 10h handler installed
  //
  BOOLEAN                           VgaInstalled;

  //
  // Number of IDE drives
  //
  UINT8                             IdeDriveCount;

  //
  // Current Free Option ROM space. An option ROM must NOT go past
  // BiosStart.
  //
  UINT32                            OptionRom;

  //
  // Save Legacy16 unexpected interrupt vector. Reprogram INT 68-6F from
  // EFI values to legacy value just before boot.
  //
  UINT32                            BiosUnexpectedInt;
  UINT32                            ThunkSavedInt[8];
  UINT16                            ThunkSeg;
  LEGACY_EFI_HDD_TABLE              *LegacyEfiHddTable;
  UINT16                            LegacyEfiHddTableIndex;
  UINT8                             DiskEnd;
  UINT8                             Disk4075;
  UINT16                            TraceIndex;
  UINT16                            Trace[0x200];

  //
  // Indicate that whether GenericLegacyBoot is entered or not
  //
  BOOLEAN                           LegacyBootEntered;

  //
  // CSM16 PCI Interface Version
  //
  UINT16                            Csm16PciInterfaceVersion;

} LEGACY_BIOS_INSTANCE;


#pragma pack(1)

/*
  40:00-01 Com1
  40:02-03 Com2
  40:04-05 Com3
  40:06-07 Com4
  40:08-09 Lpt1
  40:0A-0B Lpt2
  40:0C-0D Lpt3
  40:0E-0E Ebda segment
  40:10-11 MachineConfig
  40:12    Bda12 - skip
  40:13-14 MemSize below 1MB
  40:15-16 Bda15_16 - skip
  40:17    Keyboard Shift status
  40:18-19 Bda18_19 - skip
  40:1A-1B Key buffer head
  40:1C-1D Key buffer tail
  40:1E-3D Bda1E_3D- key buffer -skip
  40:3E-3F FloppyData 3E = Calibration status 3F = Motor status
  40:40    FloppyTimeout
  40:41-74 Bda41_74 - skip
  40:75    Number of HDD drives
  40:76-77 Bda76_77 - skip
  40:78-79 78 = Lpt1 timeout, 79 = Lpt2 timeout
  40:7A-7B 7A = Lpt3 timeout, 7B = Lpt4 timeout
  40:7C-7D 7C = Com1 timeout, 7D = Com2 timeout
  40:7E-7F 7E = Com3 timeout, 7F = Com4 timeout
  40:80-81 Pointer to start of key buffer
  40:82-83 Pointer to end of key buffer
  40:84-87 Bda84_87 - skip
  40:88    HDD Data Xmit rate
  40:89-8f skip
  40:90    Floppy data rate
  40:91-95 skip
  40:96    Keyboard Status
  40:97    LED Status
  40:98-101 skip
*/
typedef struct {
  UINT16  Com1;
  UINT16  Com2;
  UINT16  Com3;
  UINT16  Com4;
  UINT16  Lpt1;
  UINT16  Lpt2;
  UINT16  Lpt3;
  UINT16  Ebda;
  UINT16  MachineConfig;
  UINT8   Bda12;
  UINT16  MemSize;
  UINT8   Bda15_16[0x02];
  UINT8   ShiftStatus;
  UINT8   Bda18_19[0x02];
  UINT16  KeyHead;
  UINT16  KeyTail;
  UINT16  Bda1E_3D[0x10];
  UINT16  FloppyData;
  UINT8   FloppyTimeout;
  UINT8   Bda41_74[0x34];
  UINT8   NumberOfDrives;
  UINT8   Bda76_77[0x02];
  UINT16  Lpt1_2Timeout;
  UINT16  Lpt3_4Timeout;
  UINT16  Com1_2Timeout;
  UINT16  Com3_4Timeout;
  UINT16  KeyStart;
  UINT16  KeyEnd;
  UINT8   Bda84_87[0x4];
  UINT8   DataXmit;
  UINT8   Bda89_8F[0x07];
  UINT8   FloppyXRate;
  UINT8   Bda91_95[0x05];
  UINT8   KeyboardStatus;
  UINT8   LedStatus;
} BDA_STRUC;
#pragma pack()

#define LEGACY_BIOS_INSTANCE_FROM_THIS(this)  CR (this, LEGACY_BIOS_INSTANCE, LegacyBios, LEGACY_BIOS_INSTANCE_SIGNATURE)

/**
  Thunk to 16-bit real mode and execute a software interrupt with a vector
  of BiosInt. Regs will contain the 16-bit register context on entry and
  exit.

  @param  This    Protocol instance pointer.
  @param  BiosInt Processor interrupt vector to invoke
  @param  Regs    Register contexted passed into (and returned) from thunk to
                  16-bit mode

  @retval FALSE   Thunk completed, and there were no BIOS errors in the target code.
                  See Regs for status.
  @retval TRUE     There was a BIOS erro in the target code.

**/
BOOLEAN
EFIAPI
LegacyBiosInt86 (
  IN  EFI_LEGACY_BIOS_PROTOCOL          *This,
  IN  UINT8                             BiosInt,
  IN  EFI_IA32_REGISTER_SET             *Regs
  );


/**
  Thunk to 16-bit real mode and call Segment:Offset. Regs will contain the
  16-bit register context on entry and exit. Arguments can be passed on
  the Stack argument

  @param  This                   Protocol instance pointer.
  @param  Segment                Segment of 16-bit mode call
  @param  Offset                 Offset of 16-bit mdoe call
  @param  Regs                   Register contexted passed into (and returned) from
                                 thunk to  16-bit mode
  @param  Stack                  Caller allocated stack used to pass arguments
  @param  StackSize              Size of Stack in bytes

  @retval FALSE                  Thunk completed, and there were no BIOS errors in
                                 the target code. See Regs for status.
  @retval TRUE                   There was a BIOS erro in the target code.

**/
BOOLEAN
EFIAPI
LegacyBiosFarCall86 (
  IN  EFI_LEGACY_BIOS_PROTOCOL          *This,
  IN  UINT16                            Segment,
  IN  UINT16                            Offset,
  IN  EFI_IA32_REGISTER_SET             *Regs,
  IN  VOID                              *Stack,
  IN  UINTN                             StackSize
  );


/**
  Test to see if a legacy PCI ROM exists for this device. Optionally return
  the Legacy ROM instance for this PCI device.

  @param  This                   Protocol instance pointer.
  @param  PciHandle              The PCI PC-AT OPROM from this devices ROM BAR will
                                 be loaded
  @param  RomImage               Return the legacy PCI ROM for this device
  @param  RomSize                Size of ROM Image
  @param  Flags                  Indicates if ROM found and if PC-AT.

  @retval EFI_SUCCESS            Legacy Option ROM available for this device
  @retval EFI_UNSUPPORTED        Legacy Option ROM not supported.

**/
EFI_STATUS
EFIAPI
LegacyBiosCheckPciRom (
  IN  EFI_LEGACY_BIOS_PROTOCOL          *This,
  IN  EFI_HANDLE                        PciHandle,
  OUT VOID                              **RomImage, OPTIONAL
  OUT UINTN                             *RomSize, OPTIONAL
  OUT UINTN                             *Flags
  );


/**
  Assign drive number to legacy HDD drives prior to booting an EFI
  aware OS so the OS can access drives without an EFI driver.
  Note: BBS compliant drives ARE NOT available until this call by
  either shell or EFI.

  @param  This                   Protocol instance pointer.
  @param  BbsCount               Number of BBS_TABLE structures
  @param  BbsTable               List BBS entries

  @retval EFI_SUCCESS            Drive numbers assigned

**/
EFI_STATUS
EFIAPI
LegacyBiosPrepareToBootEfi (
  IN EFI_LEGACY_BIOS_PROTOCOL         *This,
  OUT UINT16                          *BbsCount,
  OUT BBS_TABLE                       **BbsTable
  );


/**
  To boot from an unconventional device like parties and/or execute
  HDD diagnostics.

  @param  This                   Protocol instance pointer.
  @param  Attributes             How to interpret the other input parameters
  @param  BbsEntry               The 0-based index into the BbsTable for the parent
                                  device.
  @param  BeerData               Pointer to the 128 bytes of ram BEER data.
  @param  ServiceAreaData        Pointer to the 64 bytes of raw Service Area data.
                                 The caller must provide a pointer to the specific
                                 Service Area and not the start all Service Areas.
 EFI_INVALID_PARAMETER if error. Does NOT return if no error.

**/
EFI_STATUS
EFIAPI
LegacyBiosBootUnconventionalDevice (
  IN EFI_LEGACY_BIOS_PROTOCOL         *This,
  IN UDC_ATTRIBUTES                   Attributes,
  IN UINTN                            BbsEntry,
  IN VOID                             *BeerData,
  IN VOID                             *ServiceAreaData
  );


/**
  Load a legacy PC-AT OPROM on the PciHandle device. Return information
  about how many disks were added by the OPROM and the shadow address and
  size. DiskStart & DiskEnd are INT 13h drive letters. Thus 0x80 is C:

  @param  This                   Protocol instance pointer.
  @param  PciHandle              The PCI PC-AT OPROM from this devices ROM BAR will
                                 be loaded. This value is NULL if RomImage is
                                 non-NULL. This is the normal case.
  @param  RomImage               A PCI PC-AT ROM image. This argument is non-NULL
                                 if there is no hardware associated with the ROM
                                 and thus no PciHandle, otherwise is must be NULL.
                                 Example is PXE base code.
  @param  Flags                  Indicates if ROM found and if PC-AT.
  @param  DiskStart              Disk number of first device hooked by the ROM. If
                                 DiskStart is the same as DiskEnd no disked were
                                 hooked.
  @param  DiskEnd                Disk number of the last device hooked by the ROM.
  @param  RomShadowAddress       Shadow address of PC-AT ROM
  @param  RomShadowedSize        Size of RomShadowAddress in bytes

  @retval EFI_SUCCESS            Legacy ROM loaded for this device
  @retval EFI_INVALID_PARAMETER  PciHandle not found
  @retval EFI_UNSUPPORTED        There is no PCI ROM in the ROM BAR or no onboard
                                 ROM

**/
EFI_STATUS
EFIAPI
LegacyBiosInstallPciRom (
  IN  EFI_LEGACY_BIOS_PROTOCOL          * This,
  IN  EFI_HANDLE                        PciHandle,
  IN  VOID                              **RomImage,
  OUT UINTN                             *Flags,
  OUT UINT8                             *DiskStart, OPTIONAL
  OUT UINT8                             *DiskEnd, OPTIONAL
  OUT VOID                              **RomShadowAddress, OPTIONAL
  OUT UINT32                            *RomShadowedSize OPTIONAL
  );


/**
  Fill in the standard BDA for Keyboard LEDs

  @param  This                   Protocol instance pointer.
  @param  Leds                   Current LED status

  @retval EFI_SUCCESS            It should always work.

**/
EFI_STATUS
EFIAPI
LegacyBiosUpdateKeyboardLedStatus (
  IN EFI_LEGACY_BIOS_PROTOCOL           *This,
  IN UINT8                              Leds
  );


/**
  Get all BBS info

  @param  This                   Protocol instance pointer.
  @param  HddCount               Number of HDD_INFO structures
  @param  HddInfo                Onboard IDE controller information
  @param  BbsCount               Number of BBS_TABLE structures
  @param  BbsTable               List BBS entries

  @retval EFI_SUCCESS            Tables returned
  @retval EFI_NOT_FOUND          resource not found
  @retval EFI_DEVICE_ERROR       can not get BBS table

**/
EFI_STATUS
EFIAPI
LegacyBiosGetBbsInfo (
  IN  EFI_LEGACY_BIOS_PROTOCOL          *This,
  OUT UINT16                            *HddCount,
  OUT HDD_INFO                          **HddInfo,
  OUT UINT16                            *BbsCount,
  OUT BBS_TABLE                         **BbsTable
  );


/**
  Shadow all legacy16 OPROMs that haven't been shadowed.
  Warning: Use this with caution. This routine disconnects all EFI
  drivers. If used externally then caller must re-connect EFI
  drivers.

  @param  This                   Protocol instance pointer.

  @retval EFI_SUCCESS            OPROMs shadowed

**/
EFI_STATUS
EFIAPI
LegacyBiosShadowAllLegacyOproms (
  IN EFI_LEGACY_BIOS_PROTOCOL   *This
  );


/**
  Attempt to legacy boot the BootOption. If the EFI contexted has been
  compromised this function will not return.

  @param  This                   Protocol instance pointer.
  @param  BbsDevicePath          EFI Device Path from BootXXXX variable.
  @param  LoadOptionsSize        Size of LoadOption in size.
  @param  LoadOptions            LoadOption from BootXXXX variable

  @retval EFI_SUCCESS            Removable media not present

**/
EFI_STATUS
EFIAPI
LegacyBiosLegacyBoot (
  IN  EFI_LEGACY_BIOS_PROTOCOL          *This,
  IN  BBS_BBS_DEVICE_PATH               *BbsDevicePath,
  IN  UINT32                            LoadOptionsSize,
  IN  VOID                              *LoadOptions
  );


/**
  Allocate memory < 1 MB and copy the thunker code into low memory. Se up
  all the descriptors.

  @param  Private                Private context for Legacy BIOS

  @retval EFI_SUCCESS            Should only pass.

**/
EFI_STATUS
LegacyBiosInitializeThunk (
  IN  LEGACY_BIOS_INSTANCE    *Private
  );


/**
  Fill in the standard BDA and EBDA stuff before Legacy16 load

  @param  Private                Legacy BIOS Instance data

  @retval EFI_SUCCESS            It should always work.

**/
EFI_STATUS
LegacyBiosInitBda (
  IN  LEGACY_BIOS_INSTANCE    *Private
  );


/**
  Collect IDE Inquiry data from the IDE disks

  @param  Private                Legacy BIOS Instance data
  @param  HddInfo                Hdd Information
  @param  Flag                   Reconnect IdeController or not

  @retval EFI_SUCCESS            It should always work.

**/
EFI_STATUS
LegacyBiosBuildIdeData (
  IN  LEGACY_BIOS_INSTANCE      *Private,
  IN  HDD_INFO                  **HddInfo,
  IN  UINT16                    Flag
  );


/**
  Enable ide controller.  This gets disabled when LegacyBoot.c is about
  to run the Option ROMs.

  @param  Private                Legacy BIOS Instance data


**/
VOID
EnableIdeController (
  IN LEGACY_BIOS_INSTANCE       *Private
  );


/**
  If the IDE channel is in compatibility (legacy) mode, remove all
  PCI I/O BAR addresses from the controller.

  @param  IdeController          The handle of target IDE controller


**/
VOID
InitLegacyIdeController (
  IN EFI_HANDLE                 IdeController
  );


/**
  Program the interrupt routing register in all the PCI devices. On a PC AT system
  this register contains the 8259 IRQ vector that matches its PCI interrupt.

  @param  Private                Legacy  BIOS Instance data

  @retval EFI_SUCCESS            Succeed.
  @retval EFI_ALREADY_STARTED    All PCI devices have been processed.

**/
EFI_STATUS
PciProgramAllInterruptLineRegisters (
  IN  LEGACY_BIOS_INSTANCE      *Private
  );


/**
  Collect EFI Info about legacy devices.

  @param  Private                Legacy BIOS Instance data

  @retval EFI_SUCCESS            It should always work.

**/
EFI_STATUS
LegacyBiosBuildSioData (
  IN  LEGACY_BIOS_INSTANCE      *Private
  );


/**
  Shadow all the PCI legacy ROMs. Use data from the Legacy BIOS Protocol
  to chose the order. Skip any devices that have already have legacy
  BIOS run.

  @param  Private                Protocol instance pointer.

  @retval EFI_SUCCESS            Succeed.
  @retval EFI_UNSUPPORTED        Cannot get VGA device handle.

**/
EFI_STATUS
PciShadowRoms (
  IN  LEGACY_BIOS_INSTANCE      *Private
  );


/**
  Fill in the standard BDA and EBDA stuff prior to legacy Boot

  @param  Private                Legacy BIOS Instance data

  @retval EFI_SUCCESS            It should always work.

**/
EFI_STATUS
LegacyBiosCompleteBdaBeforeBoot (
  IN  LEGACY_BIOS_INSTANCE    *Private
  );


/**
  Fill in the standard CMOS stuff before Legacy16 load

  @param  Private                Legacy BIOS Instance data

  @retval EFI_SUCCESS            It should always work.

**/
EFI_STATUS
LegacyBiosInitCmos (
  IN  LEGACY_BIOS_INSTANCE    *Private
  );


/**
  Fill in the standard CMOS stuff prior to legacy Boot

  @param  Private                Legacy BIOS Instance data

  @retval EFI_SUCCESS            It should always work.

**/
EFI_STATUS
LegacyBiosCompleteStandardCmosBeforeBoot (
  IN  LEGACY_BIOS_INSTANCE    *Private
  );


/**
  Contains the code that is copied into low memory (below 640K).
  This code reflects interrupts 0x68-0x6f to interrupts 0x08-0x0f.
  This template must be copied into low memory, and the IDT entries
  0x68-0x6F must be point to the low memory copy of this code.  Each
  entry is 4 bytes long, so IDT entries 0x68-0x6F can be easily
  computed.

**/
VOID
InterruptRedirectionTemplate (
  VOID
  );


/**
  Build the E820 table.

  @param  Private                Legacy BIOS Instance data
  @param  Size                   Size of E820 Table

  @retval EFI_SUCCESS            It should always work.

**/
EFI_STATUS
LegacyBiosBuildE820 (
  IN  LEGACY_BIOS_INSTANCE    *Private,
  OUT UINTN                   *Size
  );

/**
  This function is to put all AP in halt state.

  @param  Private                Legacy BIOS Instance data

**/
VOID
ShutdownAPs (
  IN LEGACY_BIOS_INSTANCE              *Private
  );

/**
  Worker function for LegacyBiosGetFlatDescs, retrieving content of
  specific registers.

  @param  IntThunk  Pointer to IntThunk of Legacy BIOS context.

**/
VOID
GetRegisters (
  LOW_MEMORY_THUNK    *IntThunk
  );

/**
  Routine for calling real thunk code.

  @param  RealCode    The address of thunk code.
  @param  BiosInt     The Bios interrupt vector number.
  @param  CallAddress The address of 16-bit mode call.

  @return  Status returned by real thunk code

**/
UINTN
CallRealThunkCode (
  UINT8               *RealCode,
  UINT8               BiosInt,
  UINT32              CallAddress
  );

/**
  Routine for generating soft interrupt.

  @param Vector  The interrupt vector number.

**/
VOID
GenerateSoftInit (
  UINT8               Vector
  );

/**
  Allocate memory for legacy usage.

  @param  AllocateType               The type of allocation to perform.
  @param  MemoryType                 The type of memory to allocate.
  @param  StartPageAddress           Start address of range
  @param  Pages                      Number of pages to allocate
  @param  Result                     Result of allocation

  @retval EFI_SUCCESS                Legacy16 code loaded
  @retval Other                      No protocol installed, unload driver.

**/
EFI_STATUS
AllocateLegacyMemory (
  IN  EFI_ALLOCATE_TYPE         AllocateType,
  IN  EFI_MEMORY_TYPE           MemoryType,
  IN  EFI_PHYSICAL_ADDRESS      StartPageAddress,
  IN  UINTN                     Pages,
  OUT EFI_PHYSICAL_ADDRESS      *Result
  );

/**
  Get a region from the LegacyBios for Tiano usage. Can only be invoked once.

  @param  This                       Protocol instance pointer.
  @param  LegacyMemorySize           Size of required region
  @param  Region                     Region to use. 00 = Either 0xE0000 or 0xF0000
                                     block Bit0 = 1 0xF0000 block Bit1 = 1 0xE0000
                                     block
  @param  Alignment                  Address alignment. Bit mapped. First non-zero
                                     bit from right is alignment.
  @param  LegacyMemoryAddress        Region Assigned

  @retval EFI_SUCCESS                Region assigned
  @retval EFI_ACCESS_DENIED          Procedure previously invoked
  @retval Other                      Region not assigned

**/
EFI_STATUS
EFIAPI
LegacyBiosGetLegacyRegion (
  IN    EFI_LEGACY_BIOS_PROTOCOL *This,
  IN    UINTN                    LegacyMemorySize,
  IN    UINTN                    Region,
  IN    UINTN                    Alignment,
  OUT   VOID                     **LegacyMemoryAddress
  );

/**
  Get a region from the LegacyBios for Tiano usage. Can only be invoked once.

  @param  This                       Protocol instance pointer.
  @param  LegacyMemorySize           Size of data to copy
  @param  LegacyMemoryAddress        Legacy Region destination address Note: must
                                     be in region assigned by
                                     LegacyBiosGetLegacyRegion
  @param  LegacyMemorySourceAddress  Source of data

  @retval EFI_SUCCESS                Region assigned
  @retval EFI_ACCESS_DENIED          Destination outside assigned region

**/
EFI_STATUS
EFIAPI
LegacyBiosCopyLegacyRegion (
  IN EFI_LEGACY_BIOS_PROTOCOL *This,
  IN    UINTN                 LegacyMemorySize,
  IN    VOID                  *LegacyMemoryAddress,
  IN    VOID                  *LegacyMemorySourceAddress
  );

/**
  Find Legacy16 BIOS image in the FLASH device and shadow it into memory. Find
  the $EFI table in the shadow area. Thunk into the Legacy16 code after it had
  been shadowed.

  @param  Private                    Legacy BIOS context data

  @retval EFI_SUCCESS                Legacy16 code loaded
  @retval Other                      No protocol installed, unload driver.

**/
EFI_STATUS
ShadowAndStartLegacy16 (
  IN  LEGACY_BIOS_INSTANCE  *Private
  );

/**
  Checks the state of the floppy and if media is inserted.

  This routine checks the state of the floppy and if media is inserted.
  There are 3 cases:
  No floppy present         - Set BBS entry to ignore
  Floppy present & no media - Set BBS entry to lowest priority. We cannot
  set it to ignore since 16-bit CSM will
  indicate no floppy and thus drive A: is
  unusable. CSM-16 will not try floppy since
  lowest priority and thus not incur boot
  time penality.
  Floppy present & media    - Set BBS entry to some priority.

  @return  State of floppy media

**/
UINT8
HasMediaInFloppy (
  VOID
  );

/**
  Identify drive data must be updated to actual parameters before boot.
  This requires updating the checksum, if it exists.

  @param  IdentifyDriveData       ATA Identify Data
  @param  Checksum                checksum of the ATA Identify Data

  @retval EFI_SUCCESS             checksum calculated
  @retval EFI_SECURITY_VIOLATION  IdentifyData invalid

**/
EFI_STATUS
CalculateIdentifyDriveChecksum (
  IN  UINT8     *IdentifyDriveData,
  OUT UINT8     *Checksum
  );

/**
  Identify drive data must be updated to actual parameters before boot.

  @param  IdentifyDriveData       ATA Identify Data

**/
VOID
UpdateIdentifyDriveData (
  IN  UINT8     *IdentifyDriveData
  );

/**
  Complete build of BBS TABLE.

  @param  Private                 Legacy BIOS Instance data
  @param  BbsTable                BBS Table passed to 16-bit code

  @retval EFI_SUCCESS             Removable media not present

**/
EFI_STATUS
LegacyBiosBuildBbs (
  IN  LEGACY_BIOS_INSTANCE      *Private,
  IN  BBS_TABLE                 *BbsTable
  );

/**
  Read CMOS register through index/data port.

  @param[in]  Index   The index of the CMOS register to read.

  @return  The data value from the CMOS register specified by Index.

**/
UINT8
LegacyReadStandardCmos (
  IN UINT8  Index
  );

/**
  Write CMOS register through index/data port.

  @param[in]  Index  The index of the CMOS register to write.
  @param[in]  Value  The value of CMOS register to write.

  @return  The value written to the CMOS register specified by Index.

**/
UINT8
LegacyWriteStandardCmos (
  IN UINT8  Index,
  IN UINT8  Value
  );

/**
  Calculate the new standard CMOS checksum and write it.

  @param  Private      Legacy BIOS Instance data

  @retval EFI_SUCCESS  Calculate 16-bit checksum successfully

**/
EFI_STATUS
LegacyCalculateWriteStandardCmosChecksum (
  VOID
  );

/**
  Test to see if a legacy PCI ROM exists for this device. Optionally return
  the Legacy ROM instance for this PCI device.

  @param[in]  This                   Protocol instance pointer.
  @param[in]  PciHandle              The PCI PC-AT OPROM from this devices ROM BAR will be loaded
  @param[out] RomImage               Return the legacy PCI ROM for this device
  @param[out] RomSize                Size of ROM Image
  @param[out] RuntimeImageLength     Runtime size of ROM Image
  @param[out] Flags                  Indicates if ROM found and if PC-AT.
  @param[out] OpromRevision          Revision of the PCI Rom
  @param[out] ConfigUtilityCodeHeaderPointer of Configuration Utility Code Header

  @return EFI_SUCCESS            Legacy Option ROM available for this device
  @return EFI_ALREADY_STARTED    This device is already managed by its Oprom
  @return EFI_UNSUPPORTED        Legacy Option ROM not supported.

**/
EFI_STATUS
LegacyBiosCheckPciRomEx (
  IN EFI_LEGACY_BIOS_PROTOCOL           *This,
  IN  EFI_HANDLE                        PciHandle,
  OUT VOID                              **RomImage, OPTIONAL
  OUT UINTN                             *RomSize, OPTIONAL
  OUT UINTN                             *RuntimeImageLength, OPTIONAL
  OUT UINTN                             *Flags, OPTIONAL
  OUT UINT8                             *OpromRevision, OPTIONAL
  OUT VOID                              **ConfigUtilityCodeHeader OPTIONAL
  );

/**
  Relocate this image under 4G memory for IPF.

  @param  ImageHandle  Handle of driver image.
  @param  SystemTable  Pointer to system table.

  @retval EFI_SUCCESS  Image successfully relocated.
  @retval EFI_ABORTED  Failed to relocate image.

**/
EFI_STATUS
RelocateImageUnder4GIfNeeded (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );

/**
  Thunk to 16-bit real mode and call Segment:Offset. Regs will contain the
  16-bit register context on entry and exit. Arguments can be passed on
  the Stack argument

  @param  This       Protocol instance pointer.
  @param  Segment    Segment of 16-bit mode call
  @param  Offset     Offset of 16-bit mdoe call
  @param  Regs       Register contexted passed into (and returned) from thunk to
                     16-bit mode
  @param  Stack      Caller allocated stack used to pass arguments
  @param  StackSize  Size of Stack in bytes

  @retval FALSE      Thunk completed, and there were no BIOS errors in the target code.
                     See Regs for status.
  @retval TRUE       There was a BIOS erro in the target code.

**/
BOOLEAN
EFIAPI
InternalLegacyBiosFarCall (
  IN  EFI_LEGACY_BIOS_PROTOCOL        *This,
  IN  UINT16                          Segment,
  IN  UINT16                          Offset,
  IN  EFI_IA32_REGISTER_SET           *Regs,
  IN  VOID                            *Stack,
  IN  UINTN                           StackSize
  );

/**
  Load a legacy PC-AT OpROM for VGA controller.

  @param  Private                Driver private data.

  @retval EFI_SUCCESS            Legacy ROM successfully installed for this device.
  @retval EFI_DEVICE_ERROR       No VGA device handle found, or native EFI video
                                 driver cannot be successfully disconnected, or VGA
                                 thunk driver cannot be successfully connected.

**/
EFI_STATUS
LegacyBiosInstallVgaRom (
  IN  LEGACY_BIOS_INSTANCE            *Private
  );

#endif
