/** 
	@file   
	ACPI 1.0b definitions from the ACPI Specification, revision 1.0b

  Copyright (c) 2006 - 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
**/

#ifndef _ACPI_1_0_H_
#define _ACPI_1_0_H_

//
// Common table header, this prefaces all ACPI tables, including FACS, but
// excluding the RSD PTR structure
//
typedef struct {
  UINT32  Signature;
  UINT32  Length;
} EFI_ACPI_COMMON_HEADER;

//
// Common ACPI description table header.  This structure prefaces most ACPI tables.
//
#pragma pack(1)

typedef struct {
  UINT32  Signature;
  UINT32  Length;
  UINT8   Revision;
  UINT8   Checksum;
  UINT8   OemId[6];
  UINT64  OemTableId;
  UINT32  OemRevision;
  UINT32  CreatorId;
  UINT32  CreatorRevision;
} EFI_ACPI_DESCRIPTION_HEADER;

#pragma pack()
//
// Define for Pci Host Bridge Resource Allocation
//
#define ACPI_ADDRESS_SPACE_DESCRIPTOR 0x8A
#define ACPI_END_TAG_DESCRIPTOR       0x79

#define ACPI_ADDRESS_SPACE_TYPE_MEM   0x00
#define ACPI_ADDRESS_SPACE_TYPE_IO    0x01
#define ACPI_ADDRESS_SPACE_TYPE_BUS   0x02

//
// Power Management Timer frequency is fixed at 3.579545MHz
//
#define ACPI_TIMER_FREQUENCY       3579545

//
// Make sure structures match spec
//
#pragma pack(1)

typedef struct {
  UINT8   Desc;
  UINT16  Len;
  UINT8   ResType;
  UINT8   GenFlag;
  UINT8   SpecificFlag;
  UINT64  AddrSpaceGranularity;
  UINT64  AddrRangeMin;
  UINT64  AddrRangeMax;
  UINT64  AddrTranslationOffset;
  UINT64  AddrLen;
} EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR;

typedef struct {
  UINT8 Desc;
  UINT8 Checksum;
} EFI_ACPI_END_TAG_DESCRIPTOR;

//
// General use definitions
//
#define EFI_ACPI_RESERVED_BYTE  0x00
#define EFI_ACPI_RESERVED_WORD  0x0000
#define EFI_ACPI_RESERVED_DWORD 0x00000000
#define EFI_ACPI_RESERVED_QWORD 0x0000000000000000

//
// Resource Type Specific Flags
// Ref ACPI specification 6.4.3.5.5
//
// Bit [0]    : Write Status, _RW
//
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_READ_WRITE                (1 << 0)
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_READ_ONLY                 (0 << 0)
//
// Bit [2:1]  : Memory Attributes, _MEM
//
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_NON_CACHEABLE             (0 << 1)
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE                 (1 << 1)
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_WRITE_COMBINING (2 << 1)
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE    (3 << 1)
//
// Bit [4:3]  : Memory Attributes, _MTP
//
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_ADDRESS_RANGE_MEMORY      (0 << 3)
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_ADDRESS_RANGE_RESERVED    (1 << 3)
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_ADDRESS_RANGE_ACPI        (2 << 3)
#define EFI_APCI_MEMORY_RESOURCE_SPECIFIC_FLAG_ADDRESS_RANGE_NVS         (3 << 3)
//
// Bit [5]    : Memory to I/O Translation, _TTP
//
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_TYPE_TRANSLATION          (1 << 5)
#define EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_TYPE_STATIC               (0 << 5)

#pragma pack()


//
// Ensure proper structure formats
//
#pragma pack(1)
//
// ACPI 1.0b table structures
//
//
// Root System Description Pointer Structure
//
typedef struct {
  UINT64  Signature;
  UINT8   Checksum;
  UINT8   OemId[6];
  UINT8   Reserved;
  UINT32  RsdtAddress;
} EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER;

//
// Root System Description Table
// No definition needed as it is a common description table header followed by a
// variable number of UINT32 table pointers.
//
//
// RSDT Revision (as defined in ACPI 1.0b spec.)
//
#define EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_TABLE_REVISION 0x01

//
// Fixed ACPI Description Table Structure (FADT)
//
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER Header;
  UINT32                      FirmwareCtrl;
  UINT32                      Dsdt;
  UINT8                       IntModel;
  UINT8                       Reserved1;
  UINT16                      SciInt;
  UINT32                      SmiCmd;
  UINT8                       AcpiEnable;
  UINT8                       AcpiDisable;
  UINT8                       S4BiosReq;
  UINT8                       Reserved2;
  UINT32                      Pm1aEvtBlk;
  UINT32                      Pm1bEvtBlk;
  UINT32                      Pm1aCntBlk;
  UINT32                      Pm1bCntBlk;
  UINT32                      Pm2CntBlk;
  UINT32                      PmTmrBlk;
  UINT32                      Gpe0Blk;
  UINT32                      Gpe1Blk;
  UINT8                       Pm1EvtLen;
  UINT8                       Pm1CntLen;
  UINT8                       Pm2CntLen;
  UINT8                       PmTmLen;
  UINT8                       Gpe0BlkLen;
  UINT8                       Gpe1BlkLen;
  UINT8                       Gpe1Base;
  UINT8                       Reserved3;
  UINT16                      PLvl2Lat;
  UINT16                      PLvl3Lat;
  UINT16                      FlushSize;
  UINT16                      FlushStride;
  UINT8                       DutyOffset;
  UINT8                       DutyWidth;
  UINT8                       DayAlrm;
  UINT8                       MonAlrm;
  UINT8                       Century;
  UINT8                       Reserved4;
  UINT8                       Reserved5;
  UINT8                       Reserved6;
  UINT32                      Flags;
} EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE;

//
// FADT Version (as defined in ACPI 1.0b spec.)
//
#define EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE_REVISION  0x01

//
// Fixed ACPI Description Table Fixed Feature Flags
// All other bits are reserved and must be set to 0.
//
#define EFI_ACPI_1_0_WBINVD       (1 << 0)
#define EFI_ACPI_1_0_WBINVD_FLUSH (1 << 1)
#define EFI_ACPI_1_0_PROC_C1      (1 << 2)
#define EFI_ACPI_1_0_P_LVL2_UP    (1 << 3)
#define EFI_ACPI_1_0_PWR_BUTTON   (1 << 4)
#define EFI_ACPI_1_0_SLP_BUTTON   (1 << 5)
#define EFI_ACPI_1_0_FIX_RTC      (1 << 6)
#define EFI_ACPI_1_0_RTC_S4       (1 << 7)
#define EFI_ACPI_1_0_TMR_VAL_EXT  (1 << 8)
#define EFI_ACPI_1_0_DCK_CAP      (1 << 9)

//
// Firmware ACPI Control Structure
//
typedef struct {
  UINT32  Signature;
  UINT32  Length;
  UINT32  HardwareSignature;
  UINT32  FirmwareWakingVector;
  UINT32  GlobalLock;
  UINT32  Flags;
  UINT8   Reserved[40];
} EFI_ACPI_1_0_FIRMWARE_ACPI_CONTROL_STRUCTURE;

//
// Firmware Control Structure Feature Flags
// All other bits are reserved and must be set to 0.
//
#define EFI_ACPI_1_0_S4BIOS_F (1 << 0)

//
// Multiple APIC Description Table header definition.  The rest of the table
// must be defined in a platform specific manner.
//
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER Header;
  UINT32                      LocalApicAddress;
  UINT32                      Flags;
} EFI_ACPI_1_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER;

//
// MADT Revision (as defined in ACPI 1.0b spec.)
//
#define EFI_ACPI_1_0_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION 0x01

//
// Multiple APIC Flags
// All other bits are reserved and must be set to 0.
//
#define EFI_ACPI_1_0_PCAT_COMPAT  (1 << 0)

//
// Multiple APIC Description Table APIC structure types
// All other values between 0x09 an 0xFF are reserved and
// will be ignored by OSPM.
//
#define EFI_ACPI_1_0_PROCESSOR_LOCAL_APIC           0x00
#define EFI_ACPI_1_0_IO_APIC                        0x01
#define EFI_ACPI_1_0_INTERRUPT_SOURCE_OVERRIDE      0x02
#define EFI_ACPI_1_0_NON_MASKABLE_INTERRUPT_SOURCE  0x03
#define EFI_ACPI_1_0_LOCAL_APIC_NMI                 0x04

//
// APIC Structure Definitions
//
//
// Processor Local APIC Structure Definition
//
typedef struct {
  UINT8   Type;
  UINT8   Length;
  UINT8   AcpiProcessorId;
  UINT8   ApicId;
  UINT32  Flags;
} EFI_ACPI_1_0_PROCESSOR_LOCAL_APIC_STRUCTURE;

//
// Local APIC Flags.  All other bits are reserved and must be 0.
//
#define EFI_ACPI_1_0_LOCAL_APIC_ENABLED (1 << 0)

//
// IO APIC Structure
//
typedef struct {
  UINT8   Type;
  UINT8   Length;
  UINT8   IoApicId;
  UINT8   Reserved;
  UINT32  IoApicAddress;
  UINT32  SystemVectorBase;
} EFI_ACPI_1_0_IO_APIC_STRUCTURE;

//
// Interrupt Source Override Structure
//
typedef struct {
  UINT8   Type;
  UINT8   Length;
  UINT8   Bus;
  UINT8   Source;
  UINT32  GlobalSystemInterruptVector;
  UINT16  Flags;
} EFI_ACPI_1_0_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE;

//
// Non-Maskable Interrupt Source Structure
//
typedef struct {
  UINT8   Type;
  UINT8   Length;
  UINT16  Flags;
  UINT32  GlobalSystemInterruptVector;
} EFI_ACPI_1_0_NON_MASKABLE_INTERRUPT_SOURCE_STRUCTURE;

//
// Local APIC NMI Structure
//
typedef struct {
  UINT8   Type;
  UINT8   Length;
  UINT8   AcpiProcessorId;
  UINT16  Flags;
  UINT8   LocalApicInti;
} EFI_ACPI_1_0_LOCAL_APIC_NMI_STRUCTURE;

//
// Smart Battery Description Table (SBST)
//
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER Header;
  UINT32                      WarningEnergyLevel;
  UINT32                      LowEnergyLevel;
  UINT32                      CriticalEnergyLevel;
} EFI_ACPI_1_0_SMART_BATTERY_DESCRIPTION_TABLE;

//
// Known table signatures
//
//
// "RSD PTR " Root System Description Pointer
//
#define EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE  0x2052545020445352ULL

//
// "APIC" Multiple APIC Description Table
//
#define EFI_ACPI_1_0_APIC_SIGNATURE 0x43495041

//
// "DSDT" Differentiated System Description Table
//
#define EFI_ACPI_1_0_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE  0x54445344

//
// "FACS" Firmware ACPI Control Structure
//
#define EFI_ACPI_1_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE  0x53434146

//
// "FACP" Fixed ACPI Description Table
//
#define EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE 0x50434146

//
// "PSDT" Persistent System Description Table
//
#define EFI_ACPI_1_0_PERSISTENT_SYSTEM_DESCRIPTION_TABLE_SIGNATURE  0x54445350

//
// "RSDT" Root System Description Table
//
#define EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_TABLE_SIGNATURE  0x54445352

//
// "SBST" Smart Battery Specification Table
//
#define EFI_ACPI_1_0_SMART_BATTERY_SPECIFICATION_TABLE_SIGNATURE  0x54534253

//
// "SSDT" Secondary System Description Table
//
#define EFI_ACPI_1_0_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE 0x54445353

#pragma pack()

#endif
