/** @file
  Main SAL API's defined in SAL 3.0 specification. 

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __SAL_API_H__
#define __SAL_API_H__

///
/// FIT Types 
/// Table 2-2 of Intel Itanium Processor Family System Abstraction Layer Specification December 2003
///
#define EFI_SAL_FIT_FIT_HEADER_TYPE 0x00
#define EFI_SAL_FIT_PAL_B_TYPE      0x01

///
/// type from 0x02 to 0x0E is reserved.
///
#define EFI_SAL_FIT_PAL_A_TYPE  0x0F

///
/// OEM-defined type range is from 0x10 to 0x7E. Here we defined the PEI_CORE type as 0x10
///
#define EFI_SAL_FIT_PEI_CORE_TYPE 0x10
#define EFI_SAL_FIT_UNUSED_TYPE   0x7F

///
/// EFI_SAL_STATUS 
///
typedef UINTN EFI_SAL_STATUS;

#define EFI_SAL_SUCCESS               ((EFI_SAL_STATUS) 0)
#define EFI_SAL_MORE_RECORDS          ((EFI_SAL_STATUS) 3)
#define EFI_SAL_NOT_IMPLEMENTED       ((EFI_SAL_STATUS) - 1)
#define EFI_SAL_INVALID_ARGUMENT      ((EFI_SAL_STATUS) - 2)
#define EFI_SAL_ERROR                 ((EFI_SAL_STATUS) - 3)
#define EFI_SAL_VIRTUAL_ADDRESS_ERROR ((EFI_SAL_STATUS) - 4)
#define EFI_SAL_NO_INFORMATION        ((EFI_SAL_STATUS) - 5)
#define EFI_SAL_NOT_ENOUGH_SCRATCH    ((EFI_SAL_STATUS) - 9)

//
//  Return values from SAL
//
typedef struct {
  EFI_SAL_STATUS  Status; // register r8
  UINTN           r9;
  UINTN           r10;
  UINTN           r11;
} SAL_RETURN_REGS;

///
///  Delivery Mode of IPF CPU.
///
typedef enum {
  EFI_DELIVERY_MODE_INT,
  EFI_DELIVERY_MODE_MPreserved1,
  EFI_DELIVERY_MODE_PMI,
  EFI_DELIVERY_MODE_MPreserved2,
  EFI_DELIVERY_MODE_NMI,
  EFI_DELIVERY_MODE_INIT,
  EFI_DELIVERY_MODE_MPreserved3,
  EFI_DELIVERY_MODE_ExtINT
} EFI_DELIVERY_MODE;

typedef SAL_RETURN_REGS (EFIAPI *SAL_PROC)
  (
    IN UINT64 FunctionId,
    IN UINT64 Arg2,
    IN UINT64 Arg3,
    IN UINT64 Arg4,
    IN UINT64 Arg5,
    IN UINT64 Arg6,
    IN UINT64 Arg7,
    IN UINT64 Arg8
  );

//
// SAL Procedure FunctionId definition
//
#define EFI_SAL_SET_VECTORS             0x01000000
#define EFI_SAL_GET_STATE_INFO          0x01000001
#define EFI_SAL_GET_STATE_INFO_SIZE     0x01000002
#define EFI_SAL_CLEAR_STATE_INFO        0x01000003
#define EFI_SAL_MC_RENDEZ               0x01000004
#define EFI_SAL_MC_SET_PARAMS           0x01000005
#define EFI_SAL_REGISTER_PHYSICAL_ADDR  0x01000006
#define EFI_SAL_CACHE_FLUSH             0x01000008
#define EFI_SAL_CACHE_INIT              0x01000009
#define EFI_SAL_PCI_CONFIG_READ         0x01000010
#define EFI_SAL_PCI_CONFIG_WRITE        0x01000011
#define EFI_SAL_FREQ_BASE               0x01000012
#define EFI_SAL_PHYSICAL_ID_INFO        0x01000013
#define EFI_SAL_UPDATE_PAL              0x01000020

#define EFI_SAL_FUNCTION_ID_MASK        0x0000ffff
#define EFI_SAL_MAX_SAL_FUNCTION_ID     0x00000021

//
// SAL Procedure parameter definitions
// Not much point in using typedefs or enums because all params
// are UINT64 and the entry point is common
//
// EFI_SAL_SET_VECTORS
//
#define EFI_SAL_SET_MCA_VECTOR          0x0
#define EFI_SAL_SET_INIT_VECTOR         0x1
#define EFI_SAL_SET_BOOT_RENDEZ_VECTOR  0x2

typedef struct {
  UINT64  Length : 32;
  UINT64  ChecksumValid : 1;
  UINT64  Reserved1 : 7;
  UINT64  ByteChecksum : 8;
  UINT64  Reserved2 : 16;
} SAL_SET_VECTORS_CS_N;

//
// EFI_SAL_GET_STATE_INFO, EFI_SAL_GET_STATE_INFO_SIZE,
// EFI_SAL_CLEAR_STATE_INFO
//
#define EFI_SAL_MCA_STATE_INFO  0x0
#define EFI_SAL_INIT_STATE_INFO 0x1
#define EFI_SAL_CMC_STATE_INFO  0x2
#define EFI_SAL_CP_STATE_INFO   0x3

//
// EFI_SAL_MC_SET_PARAMS
//
#define EFI_SAL_MC_SET_RENDEZ_PARAM 0x1
#define EFI_SAL_MC_SET_WAKEUP_PARAM 0x2
#define EFI_SAL_MC_SET_CPE_PARAM    0x3

#define EFI_SAL_MC_SET_INTR_PARAM   0x1
#define EFI_SAL_MC_SET_MEM_PARAM    0x2

//
// EFI_SAL_REGISTER_PAL_PHYSICAL_ADDR
//
#define EFI_SAL_REGISTER_PAL_ADDR 0x0

//
// EFI_SAL_CACHE_FLUSH
//
#define EFI_SAL_FLUSH_I_CACHE       0x01
#define EFI_SAL_FLUSH_D_CACHE       0x02
#define EFI_SAL_FLUSH_BOTH_CACHE    0x03
#define EFI_SAL_FLUSH_MAKE_COHERENT 0x04

//
// EFI_SAL_PCI_CONFIG_READ, EFI_SAL_PCI_CONFIG_WRITE
//
#define EFI_SAL_PCI_CONFIG_ONE_BYTE   0x1
#define EFI_SAL_PCI_CONFIG_TWO_BYTES  0x2
#define EFI_SAL_PCI_CONFIG_FOUR_BYTES 0x4

typedef struct {
  UINT64  Register : 8;
  UINT64  Function : 3;
  UINT64  Device : 5;
  UINT64  Bus : 8;
  UINT64  Segment : 8;
  UINT64  Reserved : 32;
} SAL_PCI_ADDRESS;

//
// EFI_SAL_FREQ_BASE
//
#define EFI_SAL_CPU_INPUT_FREQ_BASE     0x0
#define EFI_SAL_PLATFORM_IT_FREQ_BASE   0x1
#define EFI_SAL_PLATFORM_RTC_FREQ_BASE  0x2

//
// EFI_SAL_UPDATE_PAL
//
#define EFI_SAL_UPDATE_BAD_PAL_VERSION  ((UINT64) -1)
#define EFI_SAL_UPDATE_PAL_AUTH_FAIL    ((UINT64) -2)
#define EFI_SAL_UPDATE_PAL_BAD_TYPE     ((UINT64) -3)
#define EFI_SAL_UPDATE_PAL_READONLY     ((UINT64) -4)
#define EFI_SAL_UPDATE_PAL_WRITE_FAIL   ((UINT64) -10)
#define EFI_SAL_UPDATE_PAL_ERASE_FAIL   ((UINT64) -11)
#define EFI_SAL_UPDATE_PAL_READ_FAIL    ((UINT64) -12)
#define EFI_SAL_UPDATE_PAL_CANT_FIT     ((UINT64) -13)

typedef struct {
  UINT32  Size;
  UINT32  MmddyyyyDate;
  UINT16  Version;
  UINT8   Type;
  UINT8   Reserved[5];
  UINT64  FwVendorId;
} SAL_UPDATE_PAL_DATA_BLOCK;

typedef struct _SAL_UPDATE_PAL_INFO_BLOCK {
  struct _SAL_UPDATE_PAL_INFO_BLOCK *Next;
  struct SAL_UPDATE_PAL_DATA_BLOCK  *DataBlock;
  UINT8                             StoreChecksum;
  UINT8                             Reserved[15];
} SAL_UPDATE_PAL_INFO_BLOCK;

//
// SAL System Table Definitions
//
#pragma pack(1)
typedef struct {
  UINT32  Signature;
  UINT32  Length;
  UINT16  SalRevision;
  UINT16  EntryCount;
  UINT8   CheckSum;
  UINT8   Reserved[7];
  UINT16  SalAVersion;
  UINT16  SalBVersion;
  UINT8   OemId[32];
  UINT8   ProductId[32];
  UINT8   Reserved2[8];
} SAL_SYSTEM_TABLE_HEADER;
#pragma pack()

#define EFI_SAL_ST_HEADER_SIGNATURE "SST_"
#define EFI_SAL_REVISION            0x0300
//
// SAL System Types
//
#define EFI_SAL_ST_ENTRY_POINT        0
#define EFI_SAL_ST_MEMORY_DESCRIPTOR  1
#define EFI_SAL_ST_PLATFORM_FEATURES  2
#define EFI_SAL_ST_TR_USAGE           3
#define EFI_SAL_ST_PTC                4
#define EFI_SAL_ST_AP_WAKEUP          5

//
// SAL System Type Sizes
//
#define EFI_SAL_ST_ENTRY_POINT_SIZE        48
#define EFI_SAL_ST_MEMORY_DESCRIPTOR_SIZE  32
#define EFI_SAL_ST_PLATFORM_FEATURES_SIZE  16
#define EFI_SAL_ST_TR_USAGE_SIZE           32
#define EFI_SAL_ST_PTC_SIZE                16
#define EFI_SAL_ST_AP_WAKEUP_SIZE          16

#pragma pack(1)
typedef struct {
  UINT8   Type; //  Type == 0
  UINT8   Reserved[7];
  UINT64  PalProcEntry;
  UINT64  SalProcEntry;
  UINT64  SalGlobalDataPointer;
  UINT64  Reserved2[2];
} SAL_ST_ENTRY_POINT_DESCRIPTOR;

#pragma pack(1)
typedef struct {
  UINT8 Type;                     // Type == 2
  UINT8 PlatformFeatures;
  UINT8 Reserved[14];
} SAL_ST_PLATFORM_FEATURES;
#pragma pack()

#define SAL_PLAT_FEAT_BUS_LOCK      0x01
#define SAL_PLAT_FEAT_PLAT_IPI_HINT 0x02
#define SAL_PLAT_FEAT_PROC_IPI_HINT 0x04

#pragma pack(1)
typedef struct {
  UINT8   Type;                   // Type == 3
  UINT8   TRType;
  UINT8   TRNumber;
  UINT8   Reserved[5];
  UINT64  VirtualAddress;
  UINT64  EncodedPageSize;
  UINT64  Reserved1;
} SAL_ST_TR_DECRIPTOR;
#pragma pack()

#define EFI_SAL_ST_TR_USAGE_INSTRUCTION 00
#define EFI_SAL_ST_TR_USAGE_DATA        01

#pragma pack(1)
typedef struct {
  UINT64  NumberOfProcessors;
  UINT64  LocalIDRegister;
} SAL_COHERENCE_DOMAIN_INFO;
#pragma pack()

#pragma pack(1)
typedef struct {
  UINT8                     Type; // Type == 4
  UINT8                     Reserved[3];
  UINT32                    NumberOfDomains;
  SAL_COHERENCE_DOMAIN_INFO *DomainInformation;
} SAL_ST_CACHE_COHERENCE_DECRIPTOR;
#pragma pack()

#pragma pack(1)
typedef struct {
  UINT8   Type;                   // Type == 5
  UINT8   WakeUpType;
  UINT8   Reserved[6];
  UINT64  ExternalInterruptVector;
} SAL_ST_AP_WAKEUP_DECRIPTOR;
#pragma pack()
//
// FIT Entry
//
#define EFI_SAL_FIT_ENTRY_PTR   (0x100000000 - 32)  // 4GB - 24
#define EFI_SAL_FIT_PALA_ENTRY  (0x100000000 - 48)  // 4GB - 32
#define EFI_SAL_FIT_PALB_TYPE   01

typedef struct {
  UINT64  Address;
  UINT8   Size[3];
  UINT8   Reserved;
  UINT16  Revision;
  UINT8   Type : 7;
  UINT8   CheckSumValid : 1;
  UINT8   CheckSum;
} EFI_SAL_FIT_ENTRY;

//
//  SAL Common Record Header
//
typedef struct {
  UINT16  Length;
  UINT8   Data[1024];
} SAL_OEM_DATA;

typedef struct {
  UINT8 Seconds;
  UINT8 Minutes;
  UINT8 Hours;
  UINT8 Reserved;
  UINT8 Day;
  UINT8 Month;
  UINT8 Year;
  UINT8 Century;
} SAL_TIME_STAMP;

typedef struct {
  UINT64          RecordId;
  UINT16          Revision;
  UINT8           ErrorSeverity;
  UINT8           ValidationBits;
  UINT32          RecordLength;
  SAL_TIME_STAMP  TimeStamp;
  UINT8           OemPlatformId[16];
} SAL_RECORD_HEADER;

typedef struct {
  GUID  		Guid;
  UINT16    Revision;
  UINT8     ErrorRecoveryInfo;
  UINT8     Reserved;
  UINT32    SectionLength;
} SAL_SEC_HEADER;

//
// SAL Processor Record
//
#define SAL_PROCESSOR_ERROR_RECORD_INFO \
  { \
    0xe429faf1, 0x3cb7, 0x11d4, {0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } \
  }

#define CHECK_INFO_VALID_BIT_MASK   0x1
#define REQUESTOR_ID_VALID_BIT_MASK 0x2
#define RESPONDER_ID_VALID_BIT_MASK 0x4
#define TARGER_ID_VALID_BIT_MASK    0x8
#define PRECISE_IP_VALID_BIT_MASK   0x10

typedef struct {
  UINT64  InfoValid : 1;
  UINT64  ReqValid : 1;
  UINT64  RespValid : 1;
  UINT64  TargetValid : 1;
  UINT64  IpValid : 1;
  UINT64  Reserved : 59;
  UINT64  Info;
  UINT64  Req;
  UINT64  Resp;
  UINT64  Target;
  UINT64  Ip;
} MOD_ERROR_INFO;

typedef struct {
  UINT8 CpuidInfo[40];
  UINT8 Reserved;
} CPUID_INFO;

typedef struct {
  UINT64  FrLow;
  UINT64  FrHigh;
} FR_STRUCT;

#define MIN_STATE_VALID_BIT_MASK  0x1
#define BR_VALID_BIT_MASK         0x2
#define CR_VALID_BIT_MASK         0x4
#define AR_VALID_BIT_MASK         0x8
#define RR_VALID_BIT_MASK         0x10
#define FR_VALID_BIT_MASK         0x20

typedef struct {
  UINT64    ValidFieldBits;
  UINT8     MinStateInfo[1024];
  UINT64    Br[8];
  UINT64    Cr[128];
  UINT64    Ar[128];
  UINT64    Rr[8];
  FR_STRUCT Fr[128];
} PSI_STATIC_STRUCT;

#define PROC_ERROR_MAP_VALID_BIT_MASK       0x1
#define PROC_STATE_PARAMETER_VALID_BIT_MASK 0x2
#define PROC_CR_LID_VALID_BIT_MASK          0x4
#define PROC_STATIC_STRUCT_VALID_BIT_MASK   0x8
#define CPU_INFO_VALID_BIT_MASK             0x1000000

typedef struct {
  SAL_SEC_HEADER    SectionHeader;
  UINT64            ValidationBits;
  UINT64            ProcErrorMap;
  UINT64            ProcStateParameter;
  UINT64            ProcCrLid;
  MOD_ERROR_INFO    CacheError[15];
  MOD_ERROR_INFO    TlbError[15];
  MOD_ERROR_INFO    BusError[15];
  MOD_ERROR_INFO    RegFileCheck[15];
  MOD_ERROR_INFO    MsCheck[15];
  CPUID_INFO        CpuInfo;
  PSI_STATIC_STRUCT PsiValidData;
} SAL_PROCESSOR_ERROR_RECORD;

//
//  Sal Platform memory Error Record
//
#define SAL_MEMORY_ERROR_RECORD_INFO \
  { \
    0xe429faf2, 0x3cb7, 0x11d4, {0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } \
  }

#define MEMORY_ERROR_STATUS_VALID_BIT_MASK                0x1
#define MEMORY_PHYSICAL_ADDRESS_VALID_BIT_MASK            0x2
#define MEMORY_ADDR_BIT_MASK                              0x4
#define MEMORY_NODE_VALID_BIT_MASK                        0x8
#define MEMORY_CARD_VALID_BIT_MASK                        0x10
#define MEMORY_MODULE_VALID_BIT_MASK                      0x20
#define MEMORY_BANK_VALID_BIT_MASK                        0x40
#define MEMORY_DEVICE_VALID_BIT_MASK                      0x80
#define MEMORY_ROW_VALID_BIT_MASK                         0x100
#define MEMORY_COLUMN_VALID_BIT_MASK                      0x200
#define MEMORY_BIT_POSITION_VALID_BIT_MASK                0x400
#define MEMORY_PLATFORM_REQUESTOR_ID_VALID_BIT_MASK       0x800
#define MEMORY_PLATFORM_RESPONDER_ID_VALID_BIT_MASK       0x1000
#define MEMORY_PLATFORM_TARGET_VALID_BIT_MASK             0x2000
#define MEMORY_PLATFORM_BUS_SPECIFIC_DATA_VALID_BIT_MASK  0x4000
#define MEMORY_PLATFORM_OEM_ID_VALID_BIT_MASK             0x8000
#define MEMORY_PLATFORM_OEM_DATA_STRUCT_VALID_BIT_MASK    0x10000

typedef struct {
  SAL_SEC_HEADER  SectionHeader;
  UINT64          ValidationBits;
  UINT64          MemErrorStatus;
  UINT64          MemPhysicalAddress;
  UINT64          MemPhysicalAddressMask;
  UINT16          MemNode;
  UINT16          MemCard;
  UINT16          MemModule;
  UINT16          MemBank;
  UINT16          MemDevice;
  UINT16          MemRow;
  UINT16          MemColumn;
  UINT16          MemBitPosition;
  UINT64          ModRequestorId;
  UINT64          ModResponderId;
  UINT64          ModTargetId;
  UINT64          BusSpecificData;
  UINT8           MemPlatformOemId[16];
} SAL_MEMORY_ERROR_RECORD;

//
//  PCI BUS Errors
//
#define SAL_PCI_BUS_ERROR_RECORD_INFO \
  { \
    0xe429faf4, 0x3cb7, 0x11d4, {0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } \
  }

#define PCI_BUS_ERROR_STATUS_VALID_BIT_MASK     0x1
#define PCI_BUS_ERROR_TYPE_VALID_BIT_MASK       0x2
#define PCI_BUS_ID_VALID_BIT_MASK               0x4
#define PCI_BUS_ADDRESS_VALID_BIT_MASK          0x8
#define PCI_BUS_DATA_VALID_BIT_MASK             0x10
#define PCI_BUS_CMD_VALID_BIT_MASK              0x20
#define PCI_BUS_REQUESTOR_ID_VALID_BIT_MASK     0x40
#define PCI_BUS_RESPONDER_ID_VALID_BIT_MASK     0x80
#define PCI_BUS_TARGET_VALID_BIT_MASK           0x100
#define PCI_BUS_OEM_ID_VALID_BIT_MASK           0x200
#define PCI_BUS_OEM_DATA_STRUCT_VALID_BIT_MASK  0x400

typedef struct {
  UINT8 BusNumber;
  UINT8 SegmentNumber;
} PCI_BUS_ID;

typedef struct {
  SAL_SEC_HEADER  SectionHeader;
  UINT64          ValidationBits;
  UINT64          PciBusErrorStatus;
  UINT16          PciBusErrorType;
  PCI_BUS_ID      PciBusId;
  UINT32          Reserved;
  UINT64          PciBusAddress;
  UINT64          PciBusData;
  UINT64          PciBusCommand;
  UINT64          PciBusRequestorId;
  UINT64          PciBusResponderId;
  UINT64          PciBusTargetId;
  UINT8           PciBusOemId[16];
} SAL_PCI_BUS_ERROR_RECORD;

//
//  PCI Component Errors
//
#define SAL_PCI_COMP_ERROR_RECORD_INFO \
  { \
    0xe429faf6, 0x3cb7, 0x11d4, {0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } \
  }

#define PCI_COMP_ERROR_STATUS_VALID_BIT_MASK    0x1
#define PCI_COMP_INFO_VALID_BIT_MASK            0x2
#define PCI_COMP_MEM_NUM_VALID_BIT_MASK         0x4
#define PCI_COMP_IO_NUM_VALID_BIT_MASK          0x8
#define PCI_COMP_REG_DATA_PAIR_VALID_BIT_MASK   0x10
#define PCI_COMP_OEM_DATA_STRUCT_VALID_BIT_MASK 0x20

typedef struct {
  UINT16  VendorId;
  UINT16  DeviceId;
  UINT8   ClassCode[3];
  UINT8   FunctionNumber;
  UINT8   DeviceNumber;
  UINT8   BusNumber;
  UINT8   SegmentNumber;
  UINT8   Reserved[5];
} PCI_COMP_INFO;

typedef struct {
  SAL_SEC_HEADER  SectionHeader;
  UINT64          ValidationBits;
  UINT64          PciComponentErrorStatus;
  PCI_COMP_INFO   PciComponentInfo;
  UINT32          PciComponentMemNum;
  UINT32          PciComponentIoNum;
  UINT8           PciBusOemId[16];
} SAL_PCI_COMPONENT_ERROR_RECORD;

//
//  Sal Device Errors Info.
//
#define SAL_DEVICE_ERROR_RECORD_INFO \
  { \
    0xe429faf3, 0x3cb7, 0x11d4, {0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } \
  }

#define SEL_RECORD_ID_VALID_BIT_MASK      0x1;
#define SEL_RECORD_TYPE_VALID_BIT_MASK    0x2;
#define SEL_GENERATOR_ID_VALID_BIT_MASK   0x4;
#define SEL_EVM_REV_VALID_BIT_MASK        0x8;
#define SEL_SENSOR_TYPE_VALID_BIT_MASK    0x10;
#define SEL_SENSOR_NUM_VALID_BIT_MASK     0x20;
#define SEL_EVENT_DIR_TYPE_VALID_BIT_MASK 0x40;
#define SEL_EVENT_DATA1_VALID_BIT_MASK    0x80;
#define SEL_EVENT_DATA2_VALID_BIT_MASK    0x100;
#define SEL_EVENT_DATA3_VALID_BIT_MASK    0x200;

typedef struct {
  SAL_SEC_HEADER  SectionHeader;
  UINT64          ValidationBits;
  UINT16          SelRecordId;
  UINT8           SelRecordType;
  UINT32          TimeStamp;
  UINT16          GeneratorId;
  UINT8           EvmRevision;
  UINT8           SensorType;
  UINT8           SensorNum;
  UINT8           EventDirType;
  UINT8           Data1;
  UINT8           Data2;
  UINT8           Data3;
} SAL_DEVICE_ERROR_RECORD;

//
//  Sal SMBIOS Device Errors Info.
//
#define SAL_SMBIOS_ERROR_RECORD_INFO \
  { \
    0xe429faf5, 0x3cb7, 0x11d4, {0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } \
  }

#define SMBIOS_EVENT_TYPE_VALID_BIT_MASK  0x1
#define SMBIOS_LENGTH_VALID_BIT_MASK      0x2
#define SMBIOS_TIME_STAMP_VALID_BIT_MASK  0x4
#define SMBIOS_DATA_VALID_BIT_MASK        0x8

typedef struct {
  SAL_SEC_HEADER  SectionHeader;
  UINT64          ValidationBits;
  UINT8           SmbiosEventType;
  UINT8           SmbiosLength;
  UINT8           SmbiosBcdTimeStamp[6];
} SAL_SMBIOS_DEVICE_ERROR_RECORD;

///
///  Sal Platform Specific Errors Info.
///
#define SAL_PLATFORM_ERROR_RECORD_INFO \
  { \
    0xe429faf7, 0x3cb7, 0x11d4, {0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } \
  }

#define PLATFORM_ERROR_STATUS_VALID_BIT_MASK    0x1
#define PLATFORM_REQUESTOR_ID_VALID_BIT_MASK    0x2
#define PLATFORM_RESPONDER_ID_VALID_BIT_MASK    0x4
#define PLATFORM_TARGET_VALID_BIT_MASK          0x8
#define PLATFORM_SPECIFIC_DATA_VALID_BIT_MASK   0x10
#define PLATFORM_OEM_ID_VALID_BIT_MASK          0x20
#define PLATFORM_OEM_DATA_STRUCT_VALID_BIT_MASK 0x40
#define PLATFORM_OEM_DEVICE_PATH_VALID_BIT_MASK 0x80

typedef struct {
  SAL_SEC_HEADER  SectionHeader;
  UINT64          ValidationBits;
  UINT64          PlatformErrorStatus;
  UINT64          PlatformRequestorId;
  UINT64          PlatformResponderId;
  UINT64          PlatformTargetId;
  UINT64          PlatformBusSpecificData;
  UINT8           OemComponentId[16];
} SAL_PLATFORM_SPECIFIC_ERROR_RECORD;

///
/// Union of all the possible Sal Record Types
///
typedef union {
  SAL_RECORD_HEADER                   *RecordHeader;
  SAL_PROCESSOR_ERROR_RECORD          *SalProcessorRecord;
  SAL_PCI_BUS_ERROR_RECORD            *SalPciBusRecord;
  SAL_PCI_COMPONENT_ERROR_RECORD      *SalPciComponentRecord;
  SAL_DEVICE_ERROR_RECORD             *ImpiRecord;
  SAL_SMBIOS_DEVICE_ERROR_RECORD      *SmbiosRecord;
  SAL_PLATFORM_SPECIFIC_ERROR_RECORD  *PlatformRecord;
  SAL_MEMORY_ERROR_RECORD             *MemoryRecord;
  UINT8                               *Raw;
} SAL_ERROR_RECORDS_POINTERS;

#pragma pack()

#endif
