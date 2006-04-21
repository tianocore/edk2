/** @file
  The EFI Legacy BIOS Protocol is used to abstract legacy Option ROM usage
  under EFI and Legacy OS boot.

  Note: The names for EFI_IA32_REGISTER_SET elements were picked to follow 
  well known naming conventions.

  Thunk - A thunk is a transition from one processor mode to another. A Thunk
  is a transition from native EFI mode to 16-bit mode. A reverse thunk
  would be a transition from 16-bit mode to native EFI mode.

  You most likely should not use this protocol! Find the EFI way to solve the
  problem to make your code portable

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  LegacyBios.h

  @par Revision Reference:
  This protocol is defined in Framework for EFI Compatibility Support Module spec
  Version 0.96

**/

#ifndef _EFI_LEGACY_BIOS_H
#define _EFI_LEGACY_BIOS_H

#define EFI_LEGACY_BIOS_PROTOCOL_GUID \
  { \
    0xdb9a1e3d, 0x45cb, 0x4abb, {0x85, 0x3b, 0xe5, 0x38, 0x7f, 0xdb, 0x2e, 0x2d } \
  }

typedef struct _EFI_LEGACY_BIOS_PROTOCOL EFI_LEGACY_BIOS_PROTOCOL;

//
// Convert from 32-bit address (_Adr) to Segment:Offset 16-bit form
//
#define EFI_SEGMENT(_Adr)     (UINT16) ((UINT16) (((UINTN) (_Adr)) >> 4) & 0xf000)
#define EFI_OFFSET(_Adr)      (UINT16) (((UINT16) ((UINTN) (_Adr))) & 0xffff)
#define BYTE_GRANULARITY      0x01
#define WORD_GRANULARITY      0x02
#define DWORD_GRANULARITY     0x04
#define QWORD_GRANULARITY     0x08
#define PARAGRAPH_GRANULARITY 0x10

#define CARRY_FLAG            0x01

//*********************************************************
// EFI_EFLAGS_REG
//*********************************************************
typedef struct {
  UINT32 CF:1;
  UINT32 Reserved1:1;
  UINT32 PF:1;
  UINT32 Reserved2:1;
  UINT32 AF:1;
  UINT32 Reserved3:1;
  UINT32 ZF:1;
  UINT32 SF:1;
  UINT32 TF:1;
  UINT32 IF:1;
  UINT32 DF:1;
  UINT32 OF:1;
  UINT32 IOPL:2;
  UINT32 NT:1;
  UINT32 Reserved4:2;
  UINT32 VM:1;
  UINT32 Reserved5:14;
} EFI_EFLAGS_REG;

//*********************************************************
// EFI_DWORD_REGS
//*********************************************************

typedef struct {
    UINT32           EAX;
    UINT32           EBX;
    UINT32           ECX;
    UINT32           EDX;
    UINT32           ESI;
    UINT32           EDI;
    EFI_EFLAGS_REG   EFlags;
    UINT16           ES;
    UINT16           CS;
    UINT16           SS;
    UINT16           DS;
    UINT16           FS;
    UINT16           GS;
    UINT32           EBP;
    UINT32           ESP;
} EFI_DWORD_REGS;

//*******************************************
// EFI_FLAGS_REG
//*******************************************
typedef struct {
  UINT16     CF:1;
  UINT16     Reserved1:1;
  UINT16     PF:1;
  UINT16     Reserved2:1;
  UINT16     AF:1;
  UINT16     Reserved3:1;
  UINT16     ZF:1;
  UINT16     SF:1;
  UINT16     TF:1;
  UINT16     IF:1;
  UINT16     DF:1;
  UINT16     OF:1;
  UINT16     IOPL:2;
  UINT16     NT:1;
  UINT16     Reserved4:1;
} EFI_FLAGS_REG;


//*********************************************************
// EFI_WORD_REGS
//*********************************************************

typedef struct {
    UINT16           AX;
    UINT16           ReservedAX;
    UINT16           BX;
    UINT16           ReservedBX;    
    UINT16           CX;
    UINT16           ReservedCX;
    UINT16           DX;
    UINT16           ReservedDX;
    UINT16           SI;
    UINT16           ReservedSI;
    UINT16           DI;
    UINT16           ReservedDI;
    EFI_FLAGS_REG    Flags;
    UINT16           ReservedFlags;
    UINT16           ES;
    UINT16           CS;
    UINT16           SS;
    UINT16           DS;
    UINT16           FS;
     UINT16           GS; 
    UINT16           BP;
    UINT16           ReservedBP; 
    UINT16           SP;
    UINT16           ReservedSP; 
} EFI_WORD_REGS;

//*********************************************************
// EFI_BYTE_REGS
//*********************************************************

typedef struct {
    UINT8   AL, AH;
    UINT16  ReservedAX;
    UINT8   BL, BH;
    UINT16  ReservedBX;
    UINT8   CL, CH;
    UINT16  ReservedCX;
    UINT8   DL, DH;
    UINT16  ReservedDX;
} EFI_BYTE_REGS;

typedef union {
  EFI_DWORD_REGS  E;
  EFI_WORD_REGS   X;
  EFI_BYTE_REGS   H;
} EFI_IA32_REGISTER_SET;

#pragma pack(1)
//
// $EFI table created by Legacy16 code and consumed by EFI Legacy driver
//
typedef struct {
  UINT32  Signature;      // "$EFI"
  UINT8   TableChecksum;
  UINT8   TableLength;
  UINT8   EfiMajorRevision;
  UINT8   EfiMinorRevision;
  UINT8   TableMajorRevision;
  UINT8   TableMinorRevision;
  UINT8   Reserved[2];
  UINT16  Legacy16CallSegment;
  UINT16  Legacy16CallOffset;

  UINT16  PnPInstallationCheckSegment;
  UINT16  PnPInstallationCheckOffset;

  UINT32  EfiSystemTable;
  UINT32  OemStringPointer;
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
//
// define maximum number of HDD system supports
//
#define MAX_HDD_ENTRIES 0x30

typedef struct {
  UINT16  Raw[256];
} ATAPI_IDENTIFY;

//
// HDD_INFO status
//
#define HDD_PRIMARY               0x01
#define HDD_SECONDARY             0x02
#define HDD_MASTER_ATAPI_CDROM    0x04
#define HDD_SLAVE_ATAPI_CDROM     0x08
#define HDD_MASTER_IDE            0x20
#define HDD_SLAVE_IDE             0x40
#define HDD_MASTER_ATAPI_ZIPDISK  0x10
#define HDD_SLAVE_ATAPI_ZIPDISK   0x80

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

//
// Parties data structures
//
typedef struct {
  UINT8 DirectoryServiceValidity : 1;
  UINT8 RabcaUsedFlag : 1;
  UINT8 ExecuteHddDiagnosticsFlag : 1;
  UINT8 Reserved : 5;
} UDC_ATTRIBUTES;

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
// define BBS Device Types
//
#define BBS_FLOPPY        0x01
#define BBS_HARDDISK      0x02
#define BBS_CDROM         0x03
#define BBS_PCMCIA        0x04
#define BBS_USB           0x05
#define BBS_EMBED_NETWORK 0x06
#define BBS_BEV_DEVICE    0x80
#define BBS_UNKNOWN       0xff

typedef struct {
  UINT16  OldPosition : 4;
  UINT16  Reserved1 : 4;
  UINT16  Enabled : 1;
  UINT16  Failed : 1;
  UINT16  MediaPresent : 2;
  UINT16  Reserved2 : 4;
} BBS_STATUS_FLAGS;

#define MAX_BBS_ENTRIES 0x100
//
// BBS_IGNORE_ENTRY is placed in the BootPriority field if the entry is to
// be skipped.
// BBS_UNPRIORITIZED_ENTRY is placed in the BootPriority field before
// priority has been assigned but indicates valid entry.
// BBS_LOWEST_PRIORITY is normally used for removable media with no media
//   inserted. This allows the 16-bit CSM to allocate a drive letter to
//   the device.
// BBS_DO_NOT_BOOT_FROM is used for devices that the 16-bit CSM is to assign
//   a drive letter to but never boot from.
//
// AdditionalIrq??Handler usage is IBV specific. The fields have been added
// for:
//   1. Saving non-BBS card info about IRQs taken by card.
//   2. For BBS compliant cards that hook IRQs in order to have their SETUP
//      executed.
//
#define BBS_DO_NOT_BOOT_FROM    0xFFFC
#define BBS_LOWEST_PRIORITY     0xFFFD
#define BBS_UNPRIORITIZED_ENTRY 0xFFFE
#define BBS_IGNORE_ENTRY        0xFFFF

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

#pragma pack()

/**
  Thunk to 16-bit real mode and execute a software interrupt with a vector 
  of BiosInt. Regs will contain the 16-bit register context on entry and 
  exit.

  @param  This Protocol instance pointer.
  @param  BiosInt Processor interrupt vector to invoke
  @param  Reg Register contexted passed into (and returned) from thunk to
  16-bit mode

  @retval  FALSE Thunk completed, and there were no BIOS errors in the target code.
  See Regs for status.
  @retval  TRUE There was a BIOS erro in the target code.

**/
typedef
BOOLEAN
(EFIAPI *EFI_LEGACY_BIOS_INT86) (
  IN EFI_LEGACY_BIOS_PROTOCOL         *This,
  IN  UINT8                           BiosInt,
  IN OUT  EFI_IA32_REGISTER_SET       *Regs
  )
;

/**
  Thunk to 16-bit real mode and call Segment:Offset. Regs will contain the 
  16-bit register context on entry and exit. Arguments can be passed on 
  the Stack argument

  @param  This Protocol instance pointer.
  @param  Segment Segemnt of 16-bit mode call
  @param  Offset Offset of 16-bit mdoe call
  @param  Reg Register contexted passed into (and returned) from thunk to
  16-bit mode
  @param  Stack Caller allocated stack used to pass arguments
  @param  StackSize Size of Stack in bytes

  @retval  FALSE Thunk completed, and there were no BIOS errors in the target code.
  See Regs for status.
  @retval  TRUE There was a BIOS erro in the target code.

**/
typedef
BOOLEAN
(EFIAPI *EFI_LEGACY_BIOS_FARCALL86) (
  IN EFI_LEGACY_BIOS_PROTOCOL         *This,
  IN  UINT16                          Segment,
  IN  UINT16                          Offset,
  IN  EFI_IA32_REGISTER_SET           *Regs,
  IN  VOID                            *Stack,
  IN  UINTN                           StackSize
  )
;

typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_CHECK_ROM) (
  IN EFI_LEGACY_BIOS_PROTOCOL         *This,
  IN  EFI_HANDLE                      PciHandle,
  OUT VOID                            **RomImage, OPTIONAL
  OUT UINTN                           *RomSize, OPTIONAL
  OUT UINTN                           *Flags

/**
  Test to see if a legacy PCI ROM exists for this device. Optionally return
  the Legacy ROM instance for this PCI device.

  @param  This Protocol instance pointer.
  @param  PciHandle The PCI PC-AT OPROM from this devices ROM BAR will be loaded
  @param  RomImage Return the legacy PCI ROM for this device
  @param  RomSize Size of ROM Image
  @param  Flags Indicates if ROM found and if PC-AT.

  @retval  EFI_SUCCESS Legacy Option ROM availible for this device
  @retval  EFI_UNSUPPORTED Legacy Option ROM not supported.

**/
  )
;

/**
  Load a legacy PC-AT OPROM on the PciHandle device. Return information
  about how many disks were added by the OPROM and the shadow address and
  size. DiskStart & DiskEnd are INT 13h drive letters. Thus 0x80 is C:

  @param  This Protocol instance pointer.
  @param  PciHandle The PCI PC-AT OPROM from this devices ROM BAR will be loaded.
  This value is NULL if RomImage is non-NULL. This is the normal
  case.
  @param  RomImage A PCI PC-AT ROM image. This argument is non-NULL if there is
  no hardware associated with the ROM and thus no PciHandle,
  otherwise is must be NULL.
  Example is PXE base code.
  @param  Flags Return Status if ROM was found and if was Legacy OPROM.
  @param  DiskStart Disk number of first device hooked by the ROM. If DiskStart
  is the same as DiskEnd no disked were hooked.
  @param  DiskEnd Disk number of the last device hooked by the ROM.
  @param  RomShadowAddress Shadow address of PC-AT ROM
  @param  RomShadowSize Size of RomShadowAddress in bytes

  @retval  EFI_SUCCESS Thunk completed, see Regs for status.
  @retval  EFI_INVALID_PARAMETER PciHandle not found

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_INSTALL_ROM) (
  IN EFI_LEGACY_BIOS_PROTOCOL         *This,
  IN  EFI_HANDLE                      PciHandle,
  IN  VOID                            **RomImage,
  OUT UINTN                           *Flags,
  OUT UINT8                           *DiskStart, OPTIONAL
  OUT UINT8                           *DiskEnd, OPTIONAL
  OUT VOID                            **RomShadowAddress, OPTIONAL
  OUT UINT32                          *ShadowedRomSize OPTIONAL
  )
;

/**
  Attempt to legacy boot the BootOption. If the EFI contexted has been 
  compromised this function will not return.

  @param  This Protocol instance pointer.
  @param  BootOption EFI Device Path from BootXXXX variable.
  @param  LoadOptionSize Size of LoadOption in size.
  @param  LoadOption LoadOption from BootXXXX variable

  @retval  EFI_SUCCESS Removable media not present

**/
/**
  Update BDA with current Scroll, Num & Cap lock LEDS

  @param  This Protocol instance pointer.
  @param  Leds Status of current Scroll, Num & Cap lock LEDS
  Bit 0 is Scroll Lock  0 = Not locked
  Bit 1 is Num Lock
  Bit 2 is Caps Lock

  @retval  EFI_SUCCESS Removable media not present

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_BOOT) (
  IN EFI_LEGACY_BIOS_PROTOCOL         *This,
  IN  BBS_BBS_DEVICE_PATH             *BootOption,
  IN  UINT32                          LoadOptionsSize,
  IN  VOID                            *LoadOptions
  )
;
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_UPDATE_KEYBOARD_LED_STATUS) (
  IN EFI_LEGACY_BIOS_PROTOCOL         *This,
  IN  UINT8                           Leds
  )
;

/**
  Retrieve legacy BBS info and assign boot priority.

  @param  This Protocol instance pointer.
  @param  HddCount Number of HDD_INFO structures
  @param  HddInfo Onboard IDE controller information
  @param  BbsCount Number of BBS_TABLE structures
  @param  BbsTable List BBS entries

  @retval  EFI_SUCCESS Tables returned

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_GET_BBS_INFO) (
  IN EFI_LEGACY_BIOS_PROTOCOL         *This,
  OUT UINT16                          *HddCount,
  OUT HDD_INFO                        **HddInfo,
  OUT UINT16                          *BbsCount,
  OUT IN BBS_TABLE                    **BbsTable
  )
;

/**
  Assign drive number to legacy HDD drives prior to booting an EFI
  aware OS so the OS can access drives without an EFI driver.

  @param  This Protocol instance pointer.
  @param  BbsCount Number of BBS_TABLE structures
  @param  BbsTable List BBS entries

  @retval  EFI_SUCCESS Drive numbers assigned

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_PREPARE_TO_BOOT_EFI) (
  IN EFI_LEGACY_BIOS_PROTOCOL         *This,
  OUT UINT16                          *BbsCount,
  OUT IN BBS_TABLE                    **BbsTable
  )
;

/**
  To boot from an unconventional device like parties and/or execute
  HDD diagnostics.

  @param  This Protocol instance pointer.
  @param  Attributes How to interpret the other input parameters
  @param  BbsEntry The 0-based index into the BbsTable for the parent
  device.
  @param  BeerData Pointer to the 128 bytes of ram BEER data.
  @param  ServiceAreaData Pointer to the 64 bytes of raw Service Area data. The
  caller must provide a pointer to the specific Service
  Area and not the start all Service Areas.

  EFI_INVALID_PARAMETER if error. Does NOT return if no error.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_BOOT_UNCONVENTIONAL_DEVICE) (
  IN EFI_LEGACY_BIOS_PROTOCOL         *This,
  IN UDC_ATTRIBUTES                   Attributes,
  IN UINTN                            BbsEntry,
  IN VOID                             *BeerData,
  IN VOID                             *ServiceAreaData
  )
;

/**
  Shadow all legacy16 OPROMs that haven't been shadowed.
  Warning: Use this with caution. This routine disconnects all EFI
  drivers. If used externally then caller must re-connect EFI
  drivers.

  @retval  EFI_SUCCESS OPROMs shadowed

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_SHADOW_ALL_LEGACY_OPROMS) (
  IN EFI_LEGACY_BIOS_PROTOCOL *This
  )
;

/**
  Get a region from the LegacyBios for S3 usage.

  @param  This Protocol instance pointer.
  @param  LegacyMemorySize Size of required region
  @param  Region Region to use.
  00 = Either 0xE0000 or 0xF0000 block
  Bit0 = 1 0xF0000 block
  Bit1 = 1 0xE0000 block
  @param  Alignment Address alignment. Bit mapped. First non-zero
  bit from right is alignment.
  @param  LegacyMemoryAddress Region Assigned

  @retval  EFI_SUCCESS Region assigned
  
  @retval  Other Region not assigned

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_GET_LEGACY_REGION) (
  IN EFI_LEGACY_BIOS_PROTOCOL *This,
  IN    UINTN                 LegacyMemorySize,
  IN    UINTN                 Region,
  IN    UINTN                 Alignment,
  OUT   VOID                  **LegacyMemoryAddress
  )
;

/**
  Get a region from the LegacyBios for Tiano usage. Can only be invoked once.

  @param  This Protocol instance pointer.
  @param  LegacyMemorySize Size of data to copy
  @param  LegacyMemoryAddress Legacy Region destination address
  Note: must be in region assigned by
  LegacyBiosGetLegacyRegion
  @param  LegacyMemorySourceAddress Source of data

  @retval  EFI_SUCCESS Region assigned
  @retval  EFI_ACCESS_DENIED Destination outside assigned region

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_BIOS_COPY_LEGACY_REGION) (
  IN EFI_LEGACY_BIOS_PROTOCOL *This,
  IN    UINTN                 LegacyMemorySize,
  IN    VOID                  *LegacyMemoryAddress,
  IN    VOID                  *LegacyMemorySourceAddress
  )
;

/**
  @par Protocol Description:
  Abstracts the traditional BIOS from the rest of EFI. The LegacyBoot() 
  member function allows the BDS to support booting a traditional OS. 
  EFI thunks drivers that make EFI bindings for BIOS INT services use 
  all the other member functions.

  @param Int86
  Performs traditional software INT. See the Int86() function description.

  @param FarCall86
  Performs a far call into Compatibility16 or traditional OpROM code. 

  @param CheckPciRom
  Checks if a traditional OpROM exists for this device.

  @param InstallPciRom
  Loads a traditional OpROM in traditional OpROM address space.

  @param LegacyBoot
  Boots a traditional OS.

  @param UpdateKeyboardLedStatus
  Updates BDA to reflect the current EFI keyboard LED status.

  @param GetBbsInfo
  Allows an external agent, such as BIOS Setup, to get the BBS data. 

  @param ShadowAllLegacyOproms
  Causes all legacy OpROMs to be shadowed.

  @param PrepareToBootEfi
  Performs all actions prior to boot. Used when booting an EFI-aware OS 
  rather than a legacy OS. 

  @param GetLegacyRegion
  Allows EFI to reserve an area in the 0xE0000 or 0xF0000 block.

  @param CopyLegacyRegion
  Allows EFI to copy data to the area specified by GetLegacyRegion. 

  @param BootUnconventionalDevice
  Allows the user to boot off an unconventional device such as a PARTIES partition.

**/
struct _EFI_LEGACY_BIOS_PROTOCOL {
  EFI_LEGACY_BIOS_INT86                       Int86;
  EFI_LEGACY_BIOS_FARCALL86                   FarCall86;
  EFI_LEGACY_BIOS_CHECK_ROM                   CheckPciRom;
  EFI_LEGACY_BIOS_INSTALL_ROM                 InstallPciRom;
  EFI_LEGACY_BIOS_BOOT                        LegacyBoot;
  EFI_LEGACY_BIOS_UPDATE_KEYBOARD_LED_STATUS  UpdateKeyboardLedStatus;
  EFI_LEGACY_BIOS_GET_BBS_INFO                GetBbsInfo;
  EFI_LEGACY_BIOS_PREPARE_TO_BOOT_EFI         PrepareToBootEfi;
  EFI_LEGACY_BIOS_SHADOW_ALL_LEGACY_OPROMS    ShadowAllLegacyOproms;
  EFI_LEGACY_BIOS_GET_LEGACY_REGION           GetLegacyRegion;
  EFI_LEGACY_BIOS_COPY_LEGACY_REGION          CopyLegacyRegion;
  EFI_LEGACY_BIOS_BOOT_UNCONVENTIONAL_DEVICE  BootUnconventionalDevice;
};

extern EFI_GUID gEfiLegacyBiosProtocolGuid;

#endif
