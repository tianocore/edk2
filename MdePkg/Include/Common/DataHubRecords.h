/** @file
  This file defines GUIDs and associated data structures for records posted to the Data Hub. 
  The producers of these records use these definitions to construct records.
  The consumers of these records use these definitions to retrieve, filter and parse records.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  DataHubRecords.h

  @par Revision Reference:
  DataHubRecord.h include all data hub sub class defitions from Cache subclass 
  spec 0.9, DataHub SubClass spec 0.9, Memory SubClass Spec 0.9, Processor 
  Subclass spec 0.9,Misc SubClass spec 0.9.

**/

#ifndef _DATAHUB_RECORDS_H_
#define _DATAHUB_RECORDS_H_

#define EFI_PROCESSOR_SUBCLASS_VERSION    0x00010000


#pragma pack(1)

typedef struct _USB_PORT_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH      PciRootBridgeDevicePath;
  PCI_DEVICE_PATH           PciBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} USB_PORT_DEVICE_PATH;

//
// IDE
//
typedef struct _IDE_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH      PciRootBridgeDevicePath;
  PCI_DEVICE_PATH           PciBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} IDE_DEVICE_PATH;

//
// RMC Connector
//
typedef struct _RMC_CONN_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH      PciRootBridgeDevicePath;
  PCI_DEVICE_PATH           PciBridgeDevicePath;
  PCI_DEVICE_PATH           PciBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} RMC_CONN_DEVICE_PATH;

//
// RIDE
//
typedef struct _RIDE_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH      PciRootBridgeDevicePath;
  PCI_DEVICE_PATH           PciBridgeDevicePath;
  PCI_DEVICE_PATH           PciBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} RIDE_DEVICE_PATH;

//
// Gigabit NIC
//
typedef struct _GB_NIC_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH      PciRootBridgeDevicePath;
  PCI_DEVICE_PATH           PciBridgeDevicePath;
  PCI_DEVICE_PATH           PciXBridgeDevicePath;
  PCI_DEVICE_PATH           PciXBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} GB_NIC_DEVICE_PATH;

//
// P/S2 Connector
//
typedef struct _PS2_CONN_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH      PciRootBridgeDevicePath;
  PCI_DEVICE_PATH           LpcBridgeDevicePath;
  ACPI_HID_DEVICE_PATH      LpcBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} PS2_CONN_DEVICE_PATH;

//
// Serial Port Connector
//
typedef struct _SERIAL_CONN_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH      PciRootBridgeDevicePath;
  PCI_DEVICE_PATH           LpcBridgeDevicePath;
  ACPI_HID_DEVICE_PATH      LpcBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} SERIAL_CONN_DEVICE_PATH;

//
// Parallel Port Connector
//
typedef struct _PARALLEL_CONN_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH      PciRootBridgeDevicePath;
  PCI_DEVICE_PATH           LpcBridgeDevicePath;
  ACPI_HID_DEVICE_PATH      LpcBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} PARALLEL_CONN_DEVICE_PATH;

//
// Floopy Connector
//
typedef struct _FLOOPY_CONN_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH      PciRootBridgeDevicePath;
  PCI_DEVICE_PATH           LpcBridgeDevicePath;
  ACPI_HID_DEVICE_PATH      LpcBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} FLOOPY_CONN_DEVICE_PATH;

typedef union _EFI_MISC_PORT_DEVICE_PATH {
  USB_PORT_DEVICE_PATH      UsbDevicePath;
  IDE_DEVICE_PATH           IdeDevicePath;
  RMC_CONN_DEVICE_PATH      RmcConnDevicePath;
  RIDE_DEVICE_PATH          RideDevicePath;
  GB_NIC_DEVICE_PATH        GbNicDevicePath;
  PS2_CONN_DEVICE_PATH      Ps2ConnDevicePath;
  SERIAL_CONN_DEVICE_PATH   SerialConnDevicePath;
  PARALLEL_CONN_DEVICE_PATH ParallelConnDevicePath;
  FLOOPY_CONN_DEVICE_PATH   FloppyConnDevicePath;
} EFI_MISC_PORT_DEVICE_PATH;

#pragma pack()

//
// String Token Definition
//
#define EFI_STRING_TOKEN    UINT16

typedef struct {
  UINT32    Version;
  UINT32    HeaderSize;
  UINT16    Instance;
  UINT16    SubInstance;
  UINT32    RecordType;    
} EFI_SUBCLASS_TYPE1_HEADER;

typedef struct {
  EFI_GUID    ProducerName;
  UINT16      Instance;
  UINT16      SubInstance;
} EFI_INTER_LINK_DATA;


//
// EXP data
//

typedef struct {
  UINT16    Value;
  UINT16    Exponent;
} EFI_EXP_BASE2_DATA;


typedef EFI_EXP_BASE10_DATA   EFI_PROCESSOR_MAX_CORE_FREQUENCY_DATA;

typedef EFI_EXP_BASE10_DATA   EFI_PROCESSOR_MAX_FSB_FREQUENCY_DATA;

typedef EFI_EXP_BASE10_DATA   EFI_PROCESSOR_CORE_FREQUENCY_DATA;

typedef EFI_EXP_BASE10_DATA  *EFI_PROCESSOR_CORE_FREQUENCY_LIST_DATA;

typedef EFI_EXP_BASE10_DATA  *EFI_PROCESSOR_FSB_FREQUENCY_LIST_DATA;

typedef EFI_EXP_BASE10_DATA   EFI_PROCESSOR_FSB_FREQUENCY_DATA;

typedef STRING_REF            EFI_PROCESSOR_VERSION_DATA;

typedef STRING_REF            EFI_PROCESSOR_MANUFACTURER_DATA;

typedef STRING_REF            EFI_PROCESSOR_SERIAL_NUMBER_DATA;

typedef STRING_REF            EFI_PROCESSOR_ASSET_TAG_DATA;

typedef struct {
  UINT32  ProcessorSteppingId:4;
  UINT32  ProcessorModel:     4;
  UINT32  ProcessorFamily:    4;
  UINT32  ProcessorType:      2;
  UINT32  ProcessorReserved1: 2;
  UINT32  ProcessorXModel:    4;
  UINT32  ProcessorXFamily:   8;
  UINT32  ProcessorReserved2: 4;
} EFI_PROCESSOR_SIGNATURE;

typedef struct {
  UINT32  ProcessorBrandIndex :8;
  UINT32  ProcessorClflush    :8;
  UINT32  ProcessorReserved   :8;
  UINT32  ProcessorDfltApicId :8;
} EFI_PROCESSOR_MISC_INFO;

typedef struct {
  UINT32  ProcessorFpu:       1;
  UINT32  ProcessorVme:       1;
  UINT32  ProcessorDe:        1;
  UINT32  ProcessorPse:       1;
  UINT32  ProcessorTsc:       1;
  UINT32  ProcessorMsr:       1;
  UINT32  ProcessorPae:       1;
  UINT32  ProcessorMce:       1;
  UINT32  ProcessorCx8:       1;
  UINT32  ProcessorApic:      1;
  UINT32  ProcessorReserved1: 1;
  UINT32  ProcessorSep:       1;
  UINT32  ProcessorMtrr:      1;
  UINT32  ProcessorPge:       1;
  UINT32  ProcessorMca:       1;
  UINT32  ProcessorCmov:      1;
  UINT32  ProcessorPat:       1;
  UINT32  ProcessorPse36:     1;
  UINT32  ProcessorPsn:       1;
  UINT32  ProcessorClfsh:     1;
  UINT32  ProcessorReserved2: 1;
  UINT32  ProcessorDs:        1;
  UINT32  ProcessorAcpi:      1;
  UINT32  ProcessorMmx:       1;
  UINT32  ProcessorFxsr:      1;
  UINT32  ProcessorSse:       1;
  UINT32  ProcessorSse2:      1;
  UINT32  ProcessorSs:        1;
  UINT32  ProcessorReserved3: 1;
  UINT32  ProcessorTm:        1;
  UINT32  ProcessorReserved4: 2;
} EFI_PROCESSOR_FEATURE_FLAGS;

typedef struct {
  EFI_PROCESSOR_SIGNATURE     Signature;
  EFI_PROCESSOR_MISC_INFO     MiscInfo;
  UINT32                      Reserved;
  EFI_PROCESSOR_FEATURE_FLAGS FeatureFlags;
} EFI_PROCESSOR_ID_DATA;

typedef enum {
  EfiProcessorOther    = 1,
  EfiProcessorUnknown  = 2,
  EfiCentralProcessor  = 3,
  EfiMathProcessor     = 4,
  EfiDspProcessor      = 5,
  EfiVideoProcessor    = 6
} EFI_PROCESSOR_TYPE_DATA;

typedef enum {
  EfiProcessorFamilyOther               = 1,
  EfiProcessorFamilyUnknown             = 2,
  EfiProcessorFamily8086                = 3,
  EfiProcessorFamily80286               = 4,
  EfiProcessorFamilyIntel386            = 5,
  EfiProcessorFamilyIntel486            = 6,
  EfiProcessorFamily8087                = 7,
  EfiProcessorFamily80287               = 8,
  EfiProcessorFamily80387               = 9,
  EfiProcessorFamily80487               = 0x0A,
  EfiProcessorFamilyPentium             = 0x0B,
  EfiProcessorFamilyPentiumPro          = 0x0C,
  EfiProcessorFamilyPentiumII           = 0x0D,
  EfiProcessorFamilyPentiumMMX          = 0x0E,
  EfiProcessorFamilyCeleron             = 0x0F,
  EfiProcessorFamilyPentiumIIXeon       = 0x10,
  EfiProcessorFamilyPentiumIII          = 0x11,
  EfiProcessorFamilyM1                  = 0x12,
  EfiProcessorFamilyM1Reserved1         = 0x13,
  EfiProcessorFamilyM1Reserved2         = 0x14,
  EfiProcessorFamilyM1Reserved3         = 0x15,
  EfiProcessorFamilyM1Reserved4         = 0x16,
  EfiProcessorFamilyM1Reserved5         = 0x17,
  EfiProcessorFamilyM1Reserved6         = 0x18,
  EfiProcessorFamilyK5                  = 0x19,
  EfiProcessorFamilyK5Reserved1         = 0x1A,
  EfiProcessorFamilyK5Reserved2         = 0x1B,
  EfiProcessorFamilyK5Reserved3         = 0x1C,
  EfiProcessorFamilyK5Reserved4         = 0x1D,
  EfiProcessorFamilyK5Reserved5         = 0x1E,
  EfiProcessorFamilyK5Reserved6         = 0x1F,
  EfiProcessorFamilyPowerPC             = 0x20,
  EfiProcessorFamilyPowerPC601          = 0x21,
  EfiProcessorFamilyPowerPC603          = 0x22,
  EfiProcessorFamilyPowerPC603Plus      = 0x23,
  EfiProcessorFamilyPowerPC604          = 0x24,
  EfiProcessorFamilyAlpha2              = 0x30,
  EfiProcessorFamilyMips                = 0x40,
  EfiProcessorFamilySparc               = 0x50,
  EfiProcessorFamily68040               = 0x60,
  EfiProcessorFamily68xxx               = 0x61,
  EfiProcessorFamily68000               = 0x62,
  EfiProcessorFamily68010               = 0x63,
  EfiProcessorFamily68020               = 0x64,
  EfiProcessorFamily68030               = 0x65,
  EfiProcessorFamilyHobbit              = 0x70,
  EfiProcessorFamilyWeitek              = 0x80,
  EfiProcessorFamilyPARISC              = 0x90,
  EfiProcessorFamilyV30                 = 0xA0,
  EfiProcessorFamilyPentiumIIIXeon      = 0xB0,
  EfiProcessorFamilyPentiumIIISpeedStep = 0xB1,
  EfiProcessorFamilyPentium4            = 0xB2,
  EfiProcessorFamilyIntelXeon           = 0xB3,
  EfiProcessorFamilyAS400               = 0xB4,
  EfiProcessorFamilyIntelXeonMP         = 0xB5,
  EfiProcessorFamilyAMDAthlonXP         = 0xB6,
  EfiProcessorFamilyAMDAthlonMP         = 0xB7,
  EfiProcessorFamilyIBM390              = 0xC8,
  EfiProcessorFamilyG4                  = 0xC9,
  EfiProcessorFamilyG5                  = 0xCA,
  EfiProcessorFamilyi860                = 0xFA,
  EfiProcessorFamilyi960                = 0xFB
} EFI_PROCESSOR_FAMILY_DATA;

typedef EFI_EXP_BASE10_DATA EFI_PROCESSOR_VOLTAGE_DATA;

typedef EFI_PHYSICAL_ADDRESS EFI_PROCESSOR_APIC_BASE_ADDRESS_DATA;

typedef UINT32 EFI_PROCESSOR_APIC_ID_DATA;

typedef UINT32 EFI_PROCESSOR_APIC_VERSION_NUMBER_DATA;

typedef enum {
  EfiProcessorIa32Microcode    = 1,
  EfiProcessorIpfPalAMicrocode = 2,
  EfiProcessorIpfPalBMicrocode = 3
} EFI_PROCESSOR_MICROCODE_TYPE;

typedef struct {
  EFI_PROCESSOR_MICROCODE_TYPE  ProcessorMicrocodeType;
  UINT32                        ProcessorMicrocodeRevisionNumber;
} EFI_PROCESSOR_MICROCODE_REVISION_DATA;

typedef struct {
  UINT32      CpuStatus                 :3;
  UINT32      Reserved1                 :3;
  UINT32      SocketPopulated           :1;
  UINT32      Reserved2                 :1;
  UINT32      ApicEnable                :1;
  UINT32      BootApplicationProcessor  :1;
  UINT32      Reserved3                 :22;
} EFI_PROCESSOR_STATUS_DATA;

typedef enum {
  EfiCpuStatusUnknown        = 0,
  EfiCpuStatusEnabled        = 1,
  EfiCpuStatusDisabledByUser = 2,
  EfiCpuStatusDisabledbyBios = 3,
  EfiCpuStatusIdle           = 4,
  EfiCpuStatusOther          = 7
} EFI_CPU_STATUS;

typedef enum {
  EfiProcessorSocketOther            = 1,
  EfiProcessorSocketUnknown          = 2,
  EfiProcessorSocketDaughterBoard    = 3,
  EfiProcessorSocketZIF              = 4,
  EfiProcessorSocketReplacePiggyBack = 5,
  EfiProcessorSocketNone             = 6,
  EfiProcessorSocketLIF              = 7,
  EfiProcessorSocketSlot1            = 8,
  EfiProcessorSocketSlot2            = 9,
  EfiProcessorSocket370Pin           = 0xA,
  EfiProcessorSocketSlotA            = 0xB,
  EfiProcessorSocketSlotM            = 0xC,
  EfiProcessorSocket423              = 0xD,
  EfiProcessorSocketA462             = 0xE,
  EfiProcessorSocket478              = 0xF,
  EfiProcessorSocket754              = 0x10,
  EfiProcessorSocket940              = 0x11,
  EfiProcessorSocketLG775            = 0x12

} EFI_PROCESSOR_SOCKET_TYPE_DATA;

typedef STRING_REF EFI_PROCESSOR_SOCKET_NAME_DATA;

typedef EFI_INTER_LINK_DATA EFI_CACHE_ASSOCIATION_DATA;

typedef enum {
  EfiProcessorHealthy        = 1,
  EfiProcessorPerfRestricted = 2,
  EfiProcessorFuncRestricted = 3
} EFI_PROCESSOR_HEALTH_STATUS;  

typedef UINTN   EFI_PROCESSOR_PACKAGE_NUMBER_DATA;


typedef enum {
  ProcessorCoreFrequencyRecordType     = 1,
  ProcessorFsbFrequencyRecordType      = 2,
  ProcessorVersionRecordType           = 3,
  ProcessorManufacturerRecordType      = 4,
  ProcessorSerialNumberRecordType      = 5,
  ProcessorIdRecordType                = 6,
  ProcessorTypeRecordType              = 7,
  ProcessorFamilyRecordType            = 8,
  ProcessorVoltageRecordType           = 9,
  ProcessorApicBaseAddressRecordType   = 10,
  ProcessorApicIdRecordType            = 11,
  ProcessorApicVersionNumberRecordType = 12,
  CpuUcodeRevisionDataRecordType       = 13,
  ProcessorStatusRecordType            = 14,
  ProcessorSocketTypeRecordType        = 15,
  ProcessorSocketNameRecordType        = 16,
  CacheAssociationRecordType           = 17,
  ProcessorMaxCoreFrequencyRecordType  = 18,
  ProcessorAssetTagRecordType          = 19,
  ProcessorMaxFsbFrequencyRecordType   = 20,
  ProcessorPackageNumberRecordType     = 21,
  ProcessorCoreFrequencyListRecordType = 22,
  ProcessorFsbFrequencyListRecordType  = 23,
  ProcessorHealthStatusRecordType      = 24
} EFI_CPU_VARIABLE_RECORD_TYPE;

typedef union {
  EFI_PROCESSOR_CORE_FREQUENCY_LIST_DATA  ProcessorCoreFrequencyList;
  EFI_PROCESSOR_FSB_FREQUENCY_LIST_DATA   ProcessorFsbFrequencyList;
  EFI_PROCESSOR_SERIAL_NUMBER_DATA        ProcessorSerialNumber;
  EFI_PROCESSOR_CORE_FREQUENCY_DATA       ProcessorCoreFrequency;
  EFI_PROCESSOR_FSB_FREQUENCY_DATA        ProcessorFsbFrequency;
  EFI_PROCESSOR_MAX_CORE_FREQUENCY_DATA   ProcessorMaxCoreFrequency;
  EFI_PROCESSOR_MAX_FSB_FREQUENCY_DATA    ProcessorMaxFsbFrequency;
  EFI_PROCESSOR_VERSION_DATA              ProcessorVersion;
  EFI_PROCESSOR_MANUFACTURER_DATA         ProcessorManufacturer;
  EFI_PROCESSOR_ID_DATA                   ProcessorId;
  EFI_PROCESSOR_TYPE_DATA                 ProcessorType;
  EFI_PROCESSOR_FAMILY_DATA               ProcessorFamily;
  EFI_PROCESSOR_VOLTAGE_DATA              ProcessorVoltage;
  EFI_PROCESSOR_APIC_BASE_ADDRESS_DATA    ProcessorApicBase;
  EFI_PROCESSOR_APIC_ID_DATA              ProcessorApicId;
  EFI_PROCESSOR_APIC_VERSION_NUMBER_DATA  ProcessorApicVersionNumber;
  EFI_PROCESSOR_MICROCODE_REVISION_DATA   CpuUcodeRevisionData;
  EFI_PROCESSOR_STATUS_DATA               ProcessorStatus;
  EFI_PROCESSOR_SOCKET_TYPE_DATA          ProcessorSocketType;
  EFI_PROCESSOR_SOCKET_NAME_DATA          ProcessorSocketName;
  EFI_PROCESSOR_ASSET_TAG_DATA            ProcessorAssetTag;
  EFI_PROCESSOR_HEALTH_STATUS             ProcessorHealthStatus;
  EFI_PROCESSOR_PACKAGE_NUMBER_DATA       ProcessorPackageNumber;
} EFI_CPU_VARIABLE_RECORD;

typedef struct {
  EFI_SUBCLASS_TYPE1_HEADER      DataRecordHeader;
  EFI_CPU_VARIABLE_RECORD        VariableRecord;
} EFI_CPU_DATA_RECORD;

#define EFI_CACHE_SUBCLASS_VERSION    0x00010000


typedef EFI_EXP_BASE2_DATA  EFI_CACHE_SIZE_DATA;

typedef EFI_EXP_BASE2_DATA  EFI_MAXIMUM_CACHE_SIZE_DATA;

typedef EFI_EXP_BASE10_DATA EFI_CACHE_SPEED_DATA;

typedef STRING_REF          EFI_CACHE_SOCKET_DATA;

typedef struct {
  UINT32  Other         :1;
  UINT32  Unknown       :1;
  UINT32  NonBurst      :1;
  UINT32  Burst         :1;
  UINT32  PipelineBurst :1;
  UINT32  Asynchronous  :1;
  UINT32  Synchronous   :1;
  UINT32  Reserved      :25;
} EFI_CACHE_SRAM_TYPE_DATA;

typedef enum {  
  EfiCacheErrorOther     = 1,
  EfiCacheErrorUnknown   = 2,
  EfiCacheErrorNone      = 3,
  EfiCacheErrorParity    = 4,
  EfiCacheErrorSingleBit = 5,
  EfiCacheErrorMultiBit  = 6
} EFI_CACHE_ERROR_TYPE_DATA;

typedef enum {  
  EfiCacheTypeOther       = 1,
  EfiCacheTypeUnknown     = 2,
  EfiCacheTypeInstruction = 3,
  EfiCacheTypeData        = 4,
  EfiCacheTypeUnified     = 5
} EFI_CACHE_TYPE_DATA;

typedef enum {  
  EfiCacheAssociativityOther        = 1,
  EfiCacheAssociativityUnknown      = 2,
  EfiCacheAssociativityDirectMapped = 3,
  EfiCacheAssociativity2Way         = 4,
  EfiCacheAssociativity4Way         = 5,
  EfiCacheAssociativityFully        = 6,
  EfiCacheAssociativity8Way         = 7,
  EfiCacheAssociativity16Way        = 8
} EFI_CACHE_ASSOCIATIVITY_DATA;

typedef struct {  
  UINT32    Level           :3;
  UINT32    Socketed        :1;
  UINT32    Reserved2       :1;
  UINT32    Location        :2;
  UINT32    Enable          :1;
  UINT32    OperationalMode :2;
  UINT32    Reserved1       :22;
} EFI_CACHE_CONFIGURATION_DATA;

#define EFI_CACHE_L1      1
#define EFI_CACHE_L2      2
#define EFI_CACHE_L3      3
#define EFI_CACHE_L4      4
#define EFI_CACHE_LMAX    EFI_CACHE_L4

#define EFI_CACHE_SOCKETED      1
#define EFI_CACHE_NOT_SOCKETED  0

typedef enum {
  EfiCacheInternal = 0,
  EfiCacheExternal = 1,
  EfiCacheReserved = 2,
  EfiCacheUnknown  = 3
} EFI_CACHE_LOCATION;
  
#define EFI_CACHE_ENABLED    1
#define EFI_CACHE_DISABLED   0

typedef enum {
  EfiCacheWriteThrough = 0,
  EfiCacheWriteBack    = 1,
  EfiCacheDynamicMode  = 2,
  EfiCacheUnknownMode  = 3
} EFI_CACHE_OPERATIONAL_MODE;



typedef enum {
  CacheSizeRecordType              = 1,
  MaximumSizeCacheRecordType       = 2,
  CacheSpeedRecordType             = 3,
  CacheSocketRecordType            = 4,
  CacheSramTypeRecordType          = 5,
  CacheInstalledSramTypeRecordType = 6,
  CacheErrorTypeRecordType         = 7,
  CacheTypeRecordType              = 8,
  CacheAssociativityRecordType     = 9,
  CacheConfigRecordType            = 10
} EFI_CACHE_VARIABLE_RECORD_TYPE;


typedef union {
  EFI_CACHE_SIZE_DATA             CacheSize;
  EFI_MAXIMUM_CACHE_SIZE_DATA     MaximumCacheSize;
  EFI_CACHE_SPEED_DATA            CacheSpeed;
  EFI_CACHE_SOCKET_DATA           CacheSocket;
  EFI_CACHE_SRAM_TYPE_DATA        CacheSramType;
  EFI_CACHE_SRAM_TYPE_DATA        CacheInstalledSramType;
  EFI_CACHE_ERROR_TYPE_DATA       CacheErrorType;
  EFI_CACHE_TYPE_DATA             CacheType;
  EFI_CACHE_ASSOCIATIVITY_DATA    CacheAssociativity;
  EFI_CACHE_CONFIGURATION_DATA    CacheConfig;
  EFI_CACHE_ASSOCIATION_DATA      CacheAssociation;
} EFI_CACHE_VARIABLE_RECORD;

typedef struct {
   EFI_SUBCLASS_TYPE1_HEADER      DataRecordHeader;
   EFI_CACHE_VARIABLE_RECORD      VariableRecord;  
} EFI_CACHE_DATA_RECORD;
  
#define EFI_MEMORY_SUBCLASS_VERSION     0x0100


#define EFI_MEMORY_SIZE_RECORD_NUMBER                 0x00000001

typedef enum _EFI_MEMORY_REGION_TYPE {
  EfiMemoryRegionMemory                       = 0x01,
  EfiMemoryRegionReserved                     = 0x02,
  EfiMemoryRegionAcpi                         = 0x03,
  EfiMemoryRegionNvs                          = 0x04
} EFI_MEMORY_REGION_TYPE;

typedef struct {
  UINT32                      ProcessorNumber;
  UINT16                      StartBusNumber;
  UINT16                      EndBusNumber;
  EFI_MEMORY_REGION_TYPE      MemoryRegionType;
  EFI_EXP_BASE2_DATA          MemorySize;
  EFI_PHYSICAL_ADDRESS        MemoryStartAddress;
} EFI_MEMORY_SIZE_DATA;


#define EFI_MEMORY_ARRAY_LOCATION_RECORD_NUMBER       0x00000002

typedef enum _EFI_MEMORY_ARRAY_LOCATION {
  EfiMemoryArrayLocationOther                 = 0x01,
  EfiMemoryArrayLocationUnknown               = 0x02,
  EfiMemoryArrayLocationSystemBoard           = 0x03,
  EfiMemoryArrayLocationIsaAddonCard          = 0x04,
  EfiMemoryArrayLocationEisaAddonCard         = 0x05,
  EfiMemoryArrayLocationPciAddonCard          = 0x06,
  EfiMemoryArrayLocationMcaAddonCard          = 0x07,
  EfiMemoryArrayLocationPcmciaAddonCard       = 0x08,
  EfiMemoryArrayLocationProprietaryAddonCard  = 0x09,
  EfiMemoryArrayLocationNuBus                 = 0x0A,
  EfiMemoryArrayLocationPc98C20AddonCard      = 0xA0,
  EfiMemoryArrayLocationPc98C24AddonCard      = 0xA1,
  EfiMemoryArrayLocationPc98EAddonCard        = 0xA2,
  EfiMemoryArrayLocationPc98LocalBusAddonCard = 0xA3
} EFI_MEMORY_ARRAY_LOCATION;

typedef enum _EFI_MEMORY_ARRAY_USE {
  EfiMemoryArrayUseOther                      = 0x01,
  EfiMemoryArrayUseUnknown                    = 0x02,
  EfiMemoryArrayUseSystemMemory               = 0x03,
  EfiMemoryArrayUseVideoMemory                = 0x04,
  EfiMemoryArrayUseFlashMemory                = 0x05,
  EfiMemoryArrayUseNonVolatileRam             = 0x06,
  EfiMemoryArrayUseCacheMemory                = 0x07,
} EFI_MEMORY_ARRAY_USE;

typedef enum _EFI_MEMORY_ERROR_CORRECTION {
  EfiMemoryErrorCorrectionOther               = 0x01,
  EfiMemoryErrorCorrectionUnknown             = 0x02,
  EfiMemoryErrorCorrectionNone                = 0x03,
  EfiMemoryErrorCorrectionParity              = 0x04,
  EfiMemoryErrorCorrectionSingleBitEcc        = 0x05,
  EfiMemoryErrorCorrectionMultiBitEcc         = 0x06,
  EfiMemoryErrorCorrectionCrc                 = 0x07,
} EFI_MEMORY_ERROR_CORRECTION;

typedef struct {
  EFI_MEMORY_ARRAY_LOCATION   MemoryArrayLocation;
  EFI_MEMORY_ARRAY_USE        MemoryArrayUse;
  EFI_MEMORY_ERROR_CORRECTION MemoryErrorCorrection;
  EFI_EXP_BASE2_DATA          MaximumMemoryCapacity;
  UINT16                      NumberMemoryDevices;
} EFI_MEMORY_ARRAY_LOCATION_DATA;


#define EFI_MEMORY_ARRAY_LINK_RECORD_NUMBER           0x00000003

typedef enum _EFI_MEMORY_FORM_FACTOR {
  EfiMemoryFormFactorOther                    = 0x01,
  EfiMemoryFormFactorUnknown                  = 0x02,
  EfiMemoryFormFactorSimm                     = 0x03,
  EfiMemoryFormFactorSip                      = 0x04,
  EfiMemoryFormFactorChip                     = 0x05,
  EfiMemoryFormFactorDip                      = 0x06,
  EfiMemoryFormFactorZip                      = 0x07,
  EfiMemoryFormFactorProprietaryCard          = 0x08,
  EfiMemoryFormFactorDimm                     = 0x09,
  EfiMemoryFormFactorTsop                     = 0x0A,
  EfiMemoryFormFactorRowOfChips               = 0x0B,
  EfiMemoryFormFactorRimm                     = 0x0C,
  EfiMemoryFormFactorSodimm                   = 0x0D,
  EfiMemoryFormFactorSrimm                    = 0x0E
} EFI_MEMORY_FORM_FACTOR;

typedef enum _EFI_MEMORY_ARRAY_TYPE {
  EfiMemoryTypeOther                          = 0x01,
  EfiMemoryTypeUnknown                        = 0x02,
  EfiMemoryTypeDram                           = 0x03,
  EfiMemoryTypeEdram                          = 0x04,
  EfiMemoryTypeVram                           = 0x05,
  EfiMemoryTypeSram                           = 0x06,
  EfiMemoryTypeRam                            = 0x07,
  EfiMemoryTypeRom                            = 0x08,
  EfiMemoryTypeFlash                          = 0x09,
  EfiMemoryTypeEeprom                         = 0x0A,
  EfiMemoryTypeFeprom                         = 0x0B,
  EfiMemoryTypeEprom                          = 0x0C,
  EfiMemoryTypeCdram                          = 0x0D,
  EfiMemoryType3Dram                          = 0x0E,
  EfiMemoryTypeSdram                          = 0x0F,
  EfiMemoryTypeSgram                          = 0x10,
  EfiMemoryTypeRdram                          = 0x11,
  EfiMemoryTypeDdr                            = 0x12,
  EfiMemoryTypeDdr2                           = 0x13
} EFI_MEMORY_ARRAY_TYPE;

typedef struct {
  UINT32                      Reserved        :1;
  UINT32                      Other           :1;
  UINT32                      Unknown         :1;
  UINT32                      FastPaged       :1;
  UINT32                      StaticColumn    :1;
  UINT32                      PseudoStatic    :1;
  UINT32                      Rambus          :1;
  UINT32                      Synchronous     :1;
  UINT32                      Cmos            :1;
  UINT32                      Edo             :1;
  UINT32                      WindowDram      :1;
  UINT32                      CacheDram       :1;
  UINT32                      Nonvolatile     :1;
  UINT32                      Reserved1       :19;
} EFI_MEMORY_TYPE_DETAIL;

typedef enum {
  EfiMemoryStateEnabled      = 0,
  EfiMemoryStateUnknown      = 1,
  EfiMemoryStateUnsupported  = 2,
  EfiMemoryStateError        = 3,
  EfiMemoryStateAbsent       = 4,
  EfiMemoryStateDisabled     = 5,
  EfiMemoryStatePartial      = 6
} EFI_MEMORY_STATE;

typedef struct {
  STRING_REF                  MemoryDeviceLocator;
  STRING_REF                  MemoryBankLocator;
  STRING_REF                  MemoryManufacturer;
  STRING_REF                  MemorySerialNumber;
  STRING_REF                  MemoryAssetTag;
  STRING_REF                  MemoryPartNumber;
  EFI_INTER_LINK_DATA         MemoryArrayLink;
  EFI_INTER_LINK_DATA         MemorySubArrayLink;
  UINT16                      MemoryTotalWidth;
  UINT16                      MemoryDataWidth;
  EFI_EXP_BASE2_DATA          MemoryDeviceSize;
  EFI_MEMORY_FORM_FACTOR      MemoryFormFactor;
  UINT8                       MemoryDeviceSet;
  EFI_MEMORY_ARRAY_TYPE       MemoryType;
  EFI_MEMORY_TYPE_DETAIL      MemoryTypeDetail;
  EFI_EXP_BASE10_DATA         MemorySpeed;
  EFI_MEMORY_STATE            MemoryState;
} EFI_MEMORY_ARRAY_LINK_DATA;


#define EFI_MEMORY_ARRAY_START_ADDRESS_RECORD_NUMBER  0x00000004

typedef struct {
  EFI_PHYSICAL_ADDRESS        MemoryArrayStartAddress;
  EFI_PHYSICAL_ADDRESS        MemoryArrayEndAddress;
  EFI_INTER_LINK_DATA         PhysicalMemoryArrayLink;
  UINT16                      MemoryArrayPartitionWidth;
} EFI_MEMORY_ARRAY_START_ADDRESS_DATA;


#define EFI_MEMORY_DEVICE_START_ADDRESS_RECORD_NUMBER 0x00000005

typedef struct {
  EFI_PHYSICAL_ADDRESS        MemoryDeviceStartAddress;
  EFI_PHYSICAL_ADDRESS        MemoryDeviceEndAddress;
  EFI_INTER_LINK_DATA         PhysicalMemoryDeviceLink;
  EFI_INTER_LINK_DATA         PhysicalMemoryArrayLink;
  UINT8                       MemoryDevicePartitionRowPosition;
  UINT8                       MemoryDeviceInterleavePosition;
  UINT8                       MemoryDeviceInterleaveDataDepth;
} EFI_MEMORY_DEVICE_START_ADDRESS_DATA;


//
//  Memory. Channel Device Type -  SMBIOS Type 37
//

#define EFI_MEMORY_CHANNEL_TYPE_RECORD_NUMBER         0x00000006

typedef enum _EFI_MEMORY_CHANNEL_TYPE {
  EfiMemoryChannelTypeOther                   = 1,
  EfiMemoryChannelTypeUnknown                 = 2,
  EfiMemoryChannelTypeRambus                  = 3,
  EfiMemoryChannelTypeSyncLink                = 4
} EFI_MEMORY_CHANNEL_TYPE;

typedef struct {
  EFI_MEMORY_CHANNEL_TYPE     MemoryChannelType;
  UINT8                       MemoryChannelMaximumLoad;
  UINT8                       MemoryChannelDeviceCount;
} EFI_MEMORY_CHANNEL_TYPE_DATA;

#define EFI_MEMORY_CHANNEL_DEVICE_RECORD_NUMBER       0x00000007

typedef struct {
  UINT8                       DeviceId;
  EFI_INTER_LINK_DATA         DeviceLink;
  UINT8                       MemoryChannelDeviceLoad;
} EFI_MEMORY_CHANNEL_DEVICE_DATA;



typedef union _EFI_MEMORY_SUBCLASS_RECORDS {
  EFI_MEMORY_SIZE_DATA                  SizeData;
  EFI_MEMORY_ARRAY_LOCATION_DATA        ArrayLocationData;
  EFI_MEMORY_ARRAY_LINK_DATA            ArrayLink;
  EFI_MEMORY_ARRAY_START_ADDRESS_DATA   ArrayStartAddress;
  EFI_MEMORY_DEVICE_START_ADDRESS_DATA  DeviceStartAddress;
  EFI_MEMORY_CHANNEL_TYPE_DATA          ChannelTypeData;
  EFI_MEMORY_CHANNEL_DEVICE_DATA        ChannelDeviceData;
} EFI_MEMORY_SUBCLASS_RECORDS;

typedef struct {
  EFI_SUBCLASS_TYPE1_HEADER             Header;
  EFI_MEMORY_SUBCLASS_RECORDS           Record;
} EFI_MEMORY_SUBCLASS_DRIVER_DATA;

#define EFI_MISC_SUBCLASS_VERSION     0x0100

#pragma pack(1)
//
//////////////////////////////////////////////////////////////////////////////
//
// Last PCI Bus Number
//
#define EFI_MISC_LAST_PCI_BUS_RECORD_NUMBER  0x00000001

typedef struct {
  UINT8   LastPciBus;
} EFI_MISC_LAST_PCI_BUS_DATA;

//
//////////////////////////////////////////////////////////////////////////////
//
// Misc. BIOS Vendor - SMBIOS Type 0
//
#define EFI_MISC_BIOS_VENDOR_RECORD_NUMBER  0x00000002

typedef struct {
  UINT64  Reserved1                         :2;
  UINT64  Unknown                           :1;
  UINT64  BiosCharacteristicsNotSupported   :1;
  UINT64  IsaIsSupported                    :1;
  UINT64  McaIsSupported                    :1;
  UINT64  EisaIsSupported                   :1;
  UINT64  PciIsSupported                    :1;
  UINT64  PcmciaIsSupported                 :1;
  UINT64  PlugAndPlayIsSupported            :1;
  UINT64  ApmIsSupported                    :1;
  UINT64  BiosIsUpgradable                  :1;
  UINT64  BiosShadowingAllowed              :1;
  UINT64  VlVesaIsSupported                 :1;
  UINT64  EscdSupportIsAvailable            :1;
  UINT64  BootFromCdIsSupported             :1;
  UINT64  SelectableBootIsSupported         :1;
  UINT64  RomBiosIsSocketed                 :1;
  UINT64  BootFromPcmciaIsSupported         :1;
  UINT64  EDDSpecificationIsSupported       :1;
  UINT64  JapaneseNecFloppyIsSupported      :1;
  UINT64  JapaneseToshibaFloppyIsSupported  :1;
  UINT64  Floppy525_360IsSupported          :1;
  UINT64  Floppy525_12IsSupported           :1;
  UINT64  Floppy35_720IsSupported           :1;
  UINT64  Floppy35_288IsSupported           :1;
  UINT64  PrintScreenIsSupported            :1;
  UINT64  Keyboard8042IsSupported           :1;
  UINT64  SerialIsSupported                 :1;
  UINT64  PrinterIsSupported                :1;
  UINT64  CgaMonoIsSupported                :1;
  UINT64  NecPc98                           :1;
  UINT64  AcpiIsSupported                   :1;
  UINT64  UsbLegacyIsSupported              :1;
  UINT64  AgpIsSupported                    :1;
  UINT64  I20BootIsSupported                :1;
  UINT64  Ls120BootIsSupported              :1;
  UINT64  AtapiZipDriveBootIsSupported      :1;
  UINT64  Boot1394IsSupported               :1;
  UINT64  SmartBatteryIsSupported           :1;
  UINT64  BiosBootSpecIsSupported           :1;
  UINT64  FunctionKeyNetworkBootIsSupported :1;
  UINT64  Reserved                          :22;
} EFI_MISC_BIOS_CHARACTERISTICS;

typedef struct {
  UINT64  BiosReserved                      :16;
  UINT64  SystemReserved                    :16;
  UINT64  Reserved                          :32;
} EFI_MISC_BIOS_CHARACTERISTICS_EXTENSION;

typedef struct {
  STRING_REF                      BiosVendor;
  STRING_REF                      BiosVersion;
  STRING_REF                      BiosReleaseDate;
  EFI_PHYSICAL_ADDRESS            BiosStartingAddress;
  EFI_EXP_BASE2_DATA              BiosPhysicalDeviceSize;
  EFI_MISC_BIOS_CHARACTERISTICS   BiosCharacteristics1;
  EFI_MISC_BIOS_CHARACTERISTICS_EXTENSION  BiosCharacteristics2;
} EFI_MISC_BIOS_VENDOR_DATA;       

//
//////////////////////////////////////////////////////////////////////////////
//
// Misc. System Manufacturer - SMBIOS Type 1
//
#define EFI_MISC_SYSTEM_MANUFACTURER_RECORD_NUMBER 0x00000003

typedef enum {  
  EfiSystemWakeupTypeReserved        = 0,
  EfiSystemWakeupTypeOther           = 1,
  EfiSystemWakeupTypeUnknown         = 2,
  EfiSystemWakeupTypeApmTimer        = 3,
  EfiSystemWakeupTypeModemRing       = 4,
  EfiSystemWakeupTypeLanRemote       = 5,
  EfiSystemWakeupTypePowerSwitch     = 6,
  EfiSystemWakeupTypePciPme          = 7,
  EfiSystemWakeupTypeAcPowerRestored = 8,
} EFI_MISC_SYSTEM_WAKEUP_TYPE;

typedef struct {
  STRING_REF                      SystemManufacturer;
  STRING_REF                      SystemProductName;
  STRING_REF                      SystemVersion;
  STRING_REF                      SystemSerialNumber;
  EFI_GUID                        SystemUuid;
  EFI_MISC_SYSTEM_WAKEUP_TYPE     SystemWakeupType;
} EFI_MISC_SYSTEM_MANUFACTURER_DATA;       

//
//////////////////////////////////////////////////////////////////////////////
//
// Misc. Base Board Manufacturer - SMBIOS Type 2
//
#define EFI_MISC_BASE_BOARD_MANUFACTURER_RECORD_NUMBER 0x00000004

typedef struct {
  UINT32  Motherboard           :1;
  UINT32  RequiresDaughterCard  :1;
  UINT32  Removable             :1;
  UINT32  Replaceable           :1;
  UINT32  HotSwappable          :1;
  UINT32  Reserved              :27;
} EFI_BASE_BOARD_FEATURE_FLAGS;

typedef enum {  
  EfiBaseBoardTypeUnknown                = 1,
  EfiBaseBoardTypeOther                  = 2,
  EfiBaseBoardTypeServerBlade            = 3,
  EfiBaseBoardTypeConnectivitySwitch     = 4,
  EfiBaseBoardTypeSystemManagementModule = 5,
  EfiBaseBoardTypeProcessorModule        = 6,
  EfiBaseBoardTypeIOModule               = 7,
  EfiBaseBoardTypeMemoryModule           = 8,
  EfiBaseBoardTypeDaughterBoard          = 9,
  EfiBaseBoardTypeMotherBoard            = 0xA,
  EfiBaseBoardTypeProcessorMemoryModule  = 0xB,
  EfiBaseBoardTypeProcessorIOModule      = 0xC,
  EfiBaseBoardTypeInterconnectBoard      = 0xD,
} EFI_BASE_BOARD_TYPE;

typedef struct {
  STRING_REF                      BaseBoardManufacturer;
  STRING_REF                      BaseBoardProductName;
  STRING_REF                      BaseBoardVersion;
  STRING_REF                      BaseBoardSerialNumber;
  STRING_REF                      BaseBoardAssetTag;
  STRING_REF                      BaseBoardChassisLocation;
  EFI_BASE_BOARD_FEATURE_FLAGS    BaseBoardFeatureFlags;
  EFI_BASE_BOARD_TYPE             BaseBoardType;
  EFI_INTER_LINK_DATA             BaseBoardChassisLink;
  UINT32                          BaseBoardNumberLinks;
  EFI_INTER_LINK_DATA             LinkN;
} EFI_MISC_BASE_BOARD_MANUFACTURER_DATA;       

//
//////////////////////////////////////////////////////////////////////////////
//
// Misc. System/Chassis Enclosure - SMBIOS Type 3
//
#define EFI_MISC_CHASSIS_MANUFACTURER_RECORD_NUMBER  0x00000005

typedef enum {  
  EfiMiscChassisTypeOther               = 0x1,
  EfiMiscChassisTypeUnknown             = 0x2,
  EfiMiscChassisTypeDeskTop             = 0x3,
  EfiMiscChassisTypeLowProfileDesktop   = 0x4,
  EfiMiscChassisTypePizzaBox            = 0x5,
  EfiMiscChassisTypeMiniTower           = 0x6,
  EfiMiscChassisTypeTower               = 0x7,
  EfiMiscChassisTypePortable            = 0x8,
  EfiMiscChassisTypeLapTop              = 0x9,
  EfiMiscChassisTypeNotebook            = 0xA,
  EfiMiscChassisTypeHandHeld            = 0xB,
  EfiMiscChassisTypeDockingStation      = 0xC,
  EfiMiscChassisTypeAllInOne            = 0xD,
  EfiMiscChassisTypeSubNotebook         = 0xE,
  EfiMiscChassisTypeSpaceSaving         = 0xF,
  EfiMiscChassisTypeLunchBox            = 0x10,
  EfiMiscChassisTypeMainServerChassis   = 0x11,
  EfiMiscChassisTypeExpansionChassis    = 0x12,
  EfiMiscChassisTypeSubChassis          = 0x13,
  EfiMiscChassisTypeBusExpansionChassis = 0x14,
  EfiMiscChassisTypePeripheralChassis   = 0x15,
  EfiMiscChassisTypeRaidChassis         = 0x16,
  EfiMiscChassisTypeRackMountChassis    = 0x17,
  EfiMiscChassisTypeSealedCasePc        = 0x18,
  EfiMiscChassisMultiSystemChassis      = 0x19,
} EFI_MISC_CHASSIS_TYPE;

typedef struct {
  UINT32  ChassisType       :16;
  UINT32  ChassisLockPresent:1;
  UINT32  Reserved          :15;
} EFI_MISC_CHASSIS_STATUS;

typedef enum {  
  EfiChassisStateOther           = 0x01,
  EfiChassisStateUnknown         = 0x02,
  EfiChassisStateSafe            = 0x03,
  EfiChassisStateWarning         = 0x04,
  EfiChassisStateCritical        = 0x05,
  EfiChassisStateNonRecoverable  = 0x06,
} EFI_MISC_CHASSIS_STATE;

typedef enum {  
  EfiChassisSecurityStatusOther                          = 0x01,
  EfiChassisSecurityStatusUnknown                        = 0x02,
  EfiChassisSecurityStatusNone                           = 0x03,
  EfiChassisSecurityStatusExternalInterfaceLockedOut     = 0x04,
  EfiChassisSecurityStatusExternalInterfaceLockedEnabled = 0x05,
} EFI_MISC_CHASSIS_SECURITY_STATE;

typedef struct {
  UINT32  RecordType  :1;
  UINT32  Reserved    :24;
} EFI_MISC_ELEMENT_TYPE;

typedef struct {
  EFI_MISC_ELEMENT_TYPE   ChassisElementType;
  EFI_INTER_LINK_DATA     ChassisElementStructure;
  EFI_BASE_BOARD_TYPE     ChassisBaseBoard;
  UINT32                  ChassisElementMinimum;
  UINT32                  ChassisElementMaximum;
} EFI_MISC_ELEMENTS; 

typedef struct {
  STRING_REF                      ChassisManufacturer;
  STRING_REF                      ChassisVersion;
  STRING_REF                      ChassisSerialNumber;
  STRING_REF                      ChassisAssetTag;
  EFI_MISC_CHASSIS_STATUS         ChassisType;
  EFI_MISC_CHASSIS_STATE          ChassisBootupState;
  EFI_MISC_CHASSIS_STATE          ChassisPowerSupplyState;
  EFI_MISC_CHASSIS_STATE          ChassisThermalState;
  EFI_MISC_CHASSIS_SECURITY_STATE ChassisSecurityState;
  UINT32                          ChassisOemDefined;
  UINT32                          ChassisHeight;
  UINT32                          ChassisNumberPowerCords;
  UINT32                          ChassisElementCount;
  UINT32                          ChassisElementRecordLength;//
  EFI_MISC_ELEMENTS               ChassisElements;
} EFI_MISC_CHASSIS_MANUFACTURER_DATA;

//
//////////////////////////////////////////////////////////////////////////////
//
// Misc. Port Connector Information - SMBIOS Type 8
//
#define EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_RECORD_NUMBER 0x00000006

typedef enum {  
  EfiPortConnectorTypeNone                   = 0x00,
  EfiPortConnectorTypeCentronics             = 0x01,
  EfiPortConnectorTypeMiniCentronics         = 0x02,
  EfiPortConnectorTypeProprietary            = 0x03,
  EfiPortConnectorTypeDB25Male               = 0x04,
  EfiPortConnectorTypeDB25Female             = 0x05,
  EfiPortConnectorTypeDB15Male               = 0x06,
  EfiPortConnectorTypeDB15Female             = 0x07,
  EfiPortConnectorTypeDB9Male                = 0x08,
  EfiPortConnectorTypeDB9Female              = 0x09,
  EfiPortConnectorTypeRJ11                   = 0x0A,
  EfiPortConnectorTypeRJ45                   = 0x0B,
  EfiPortConnectorType50PinMiniScsi          = 0x0C,
  EfiPortConnectorTypeMiniDin                = 0x0D,
  EfiPortConnectorTypeMicriDin               = 0x0E,
  EfiPortConnectorTypePS2                    = 0x0F,
  EfiPortConnectorTypeInfrared               = 0x10,
  EfiPortConnectorTypeHpHil                  = 0x11,
  EfiPortConnectorTypeUsb                    = 0x12,
  EfiPortConnectorTypeSsaScsi                = 0x13,
  EfiPortConnectorTypeCircularDin8Male       = 0x14,
  EfiPortConnectorTypeCircularDin8Female     = 0x15,
  EfiPortConnectorTypeOnboardIde             = 0x16,
  EfiPortConnectorTypeOnboardFloppy          = 0x17,
  EfiPortConnectorType9PinDualInline         = 0x18,
  EfiPortConnectorType25PinDualInline        = 0x19,
  EfiPortConnectorType50PinDualInline        = 0x1A,
  EfiPortConnectorType68PinDualInline        = 0x1B,
  EfiPortConnectorTypeOnboardSoundInput      = 0x1C,
  EfiPortConnectorTypeMiniCentronicsType14   = 0x1D,
  EfiPortConnectorTypeMiniCentronicsType26   = 0x1E,
  EfiPortConnectorTypeHeadPhoneMiniJack      = 0x1F,
  EfiPortConnectorTypeBNC                    = 0x20,
  EfiPortConnectorType1394                   = 0x21,
  EfiPortConnectorTypePC98                   = 0xA0,
  EfiPortConnectorTypePC98Hireso             = 0xA1,
  EfiPortConnectorTypePCH98                  = 0xA2,
  EfiPortConnectorTypePC98Note               = 0xA3,
  EfiPortConnectorTypePC98Full               = 0xA4,
  EfiPortConnectorTypeOther                  = 0xFF,
} EFI_MISC_PORT_CONNECTOR_TYPE;

typedef enum {  
  EfiPortTypeNone                      = 0x00,
  EfiPortTypeParallelXtAtCompatible    = 0x01,
  EfiPortTypeParallelPortPs2           = 0x02,
  EfiPortTypeParallelPortEcp           = 0x03,
  EfiPortTypeParallelPortEpp           = 0x04,
  EfiPortTypeParallelPortEcpEpp        = 0x05,
  EfiPortTypeSerialXtAtCompatible      = 0x06,
  EfiPortTypeSerial16450Compatible     = 0x07,
  EfiPortTypeSerial16550Compatible     = 0x08,
  EfiPortTypeSerial16550ACompatible    = 0x09,
  EfiPortTypeScsi                      = 0x0A,
  EfiPortTypeMidi                      = 0x0B,
  EfiPortTypeJoyStick                  = 0x0C,
  EfiPortTypeKeyboard                  = 0x0D,
  EfiPortTypeMouse                     = 0x0E,
  EfiPortTypeSsaScsi                   = 0x0F,
  EfiPortTypeUsb                       = 0x10,
  EfiPortTypeFireWire                  = 0x11,
  EfiPortTypePcmciaTypeI               = 0x12,
  EfiPortTypePcmciaTypeII              = 0x13,
  EfiPortTypePcmciaTypeIII             = 0x14,
  EfiPortTypeCardBus                   = 0x15,
  EfiPortTypeAccessBusPort             = 0x16,
  EfiPortTypeScsiII                    = 0x17,
  EfiPortTypeScsiWide                  = 0x18,
  EfiPortTypePC98                      = 0x19,
  EfiPortTypePC98Hireso                = 0x1A,
  EfiPortTypePCH98                     = 0x1B,
  EfiPortTypeVideoPort                 = 0x1C,
  EfiPortTypeAudioPort                 = 0x1D,
  EfiPortTypeModemPort                 = 0x1E,
  EfiPortTypeNetworkPort               = 0x1F,
  EfiPortType8251Compatible            = 0xA0,
  EfiPortType8251FifoCompatible        = 0xA1,
  EfiPortTypeOther                     = 0xFF,
} EFI_MISC_PORT_TYPE;

typedef struct {
  STRING_REF                    PortInternalConnectorDesignator;
  STRING_REF                    PortExternalConnectorDesignator;
  EFI_MISC_PORT_CONNECTOR_TYPE  PortInternalConnectorType;
  EFI_MISC_PORT_CONNECTOR_TYPE  PortExternalConnectorType;
  EFI_MISC_PORT_TYPE            PortType;
  EFI_MISC_PORT_DEVICE_PATH     PortPath;
} EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA;      

//
//////////////////////////////////////////////////////////////////////////////
//
// Misc. System Slots - SMBIOS Type 9
//
#define EFI_MISC_SYSTEM_SLOT_DESIGNATION_RECORD_NUMBER 0x00000007

typedef enum {  
  EfiSlotTypeOther                        = 0x01,
  EfiSlotTypeUnknown                      = 0x02,
  EfiSlotTypeIsa                          = 0x03,
  EfiSlotTypeMca                          = 0x04,
  EfiSlotTypeEisa                         = 0x05,
  EfiSlotTypePci                          = 0x06,
  EfiSlotTypePcmcia                       = 0x07,
  EfiSlotTypeVlVesa                       = 0x08,
  EfiSlotTypeProprietary                  = 0x09,
  EfiSlotTypeProcessorCardSlot            = 0x0A,
  EfiSlotTypeProprietaryMemoryCardSlot    = 0x0B,
  EfiSlotTypeIORiserCardSlot              = 0x0C,
  EfiSlotTypeNuBus                        = 0x0D,
  EfiSlotTypePci66MhzCapable              = 0x0E,
  EfiSlotTypeAgp                          = 0x0F,
  EfiSlotTypeApg2X                        = 0x10,
  EfiSlotTypeAgp4X                        = 0x11,
  EfiSlotTypePciX                         = 0x12,
  EfiSlotTypeAgp4x                        = 0x13,
  EfiSlotTypePC98C20                      = 0xA0,
  EfiSlotTypePC98C24                      = 0xA1,
  EfiSlotTypePC98E                        = 0xA2,
  EfiSlotTypePC98LocalBus                 = 0xA3,
  EfiSlotTypePC98Card                     = 0xA4,
  EfiSlotTypePciExpress                   = 0xA5
} EFI_MISC_SLOT_TYPE;

typedef enum {  
  EfiSlotDataBusWidthOther      = 0x01,
  EfiSlotDataBusWidthUnknown    = 0x02,
  EfiSlotDataBusWidth8Bit       = 0x03,
  EfiSlotDataBusWidth16Bit      = 0x04,
  EfiSlotDataBusWidth32Bit      = 0x05,
  EfiSlotDataBusWidth64Bit      = 0x06,
  EfiSlotDataBusWidth128Bit     = 0x07,
} EFI_MISC_SLOT_DATA_BUS_WIDTH;

typedef enum {  
  EfiSlotUsageOther     = 1,
  EfiSlotUsageUnknown   = 2,
  EfiSlotUsageAvailable = 3,
  EfiSlotUsageInUse     = 4,
} EFI_MISC_SLOT_USAGE;
  
typedef enum {  
  EfiSlotLengthOther   = 1,
  EfiSlotLengthUnknown = 2,
  EfiSlotLengthShort   = 3,
  EfiSlotLengthLong    = 4
} EFI_MISC_SLOT_LENGTH;

typedef struct {
  UINT32  CharacteristicsUnknown  :1;
  UINT32  Provides50Volts         :1;
  UINT32  Provides33Volts         :1;
  UINT32  SharedSlot              :1;
  UINT32  PcCard16Supported       :1;
  UINT32  CardBusSupported        :1;
  UINT32  ZoomVideoSupported      :1;
  UINT32  ModemRingResumeSupported:1;
  UINT32  PmeSignalSupported      :1;
  UINT32  HotPlugDevicesSupported :1;
  UINT32  SmbusSignalSupported    :1;
  UINT32  Reserved                :21;
} EFI_MISC_SLOT_CHARACTERISTICS;

typedef struct {
  STRING_REF                    SlotDesignation;
  EFI_MISC_SLOT_TYPE            SlotType;
  EFI_MISC_SLOT_DATA_BUS_WIDTH  SlotDataBusWidth;
  EFI_MISC_SLOT_USAGE           SlotUsage;
  EFI_MISC_SLOT_LENGTH          SlotLength;
  UINT16                        SlotId;
  EFI_MISC_SLOT_CHARACTERISTICS SlotCharacteristics;
  EFI_DEVICE_PATH_PROTOCOL      SlotDevicePath;
} EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA;      

//
//////////////////////////////////////////////////////////////////////////////
//
// Misc. Onboard Device - SMBIOS Type 10
//
#define EFI_MISC_ONBOARD_DEVICE_RECORD_NUMBER 0x00000008

typedef enum {  
  EfiOnBoardDeviceTypeOther          = 1,
  EfiOnBoardDeviceTypeUnknown        = 2,
  EfiOnBoardDeviceTypeVideo          = 3,
  EfiOnBoardDeviceTypeScsiController = 4,
  EfiOnBoardDeviceTypeEthernet       = 5,
  EfiOnBoardDeviceTypeTokenRing      = 6,
  EfiOnBoardDeviceTypeSound          = 7,
} EFI_MISC_ONBOARD_DEVICE_TYPE;

typedef struct {
  UINT32  DeviceType    :16;
  UINT32  DeviceEnabled :1;
  UINT32  Reserved      :15;
} EFI_MISC_ONBOARD_DEVICE_STATUS;

typedef struct {
  STRING_REF                           OnBoardDeviceDescription;
  EFI_MISC_ONBOARD_DEVICE_STATUS       OnBoardDeviceStatus;
  EFI_DEVICE_PATH_PROTOCOL             OnBoardDevicePath;
} EFI_MISC_ONBOARD_DEVICE_DATA;

//
//////////////////////////////////////////////////////////////////////////////
//
// Misc. BIOS Language Information - SMBIOS Type 11
//
#define EFI_MISC_OEM_STRING_RECORD_NUMBER 0x00000009

typedef struct {
  STRING_REF                          OemStringRef[1];
} EFI_MISC_OEM_STRING_DATA;

//
//////////////////////////////////////////////////////////////////////////////
//
// Misc. System Options - SMBIOS Type 12
//
typedef struct {
  STRING_REF                          SystemOptionStringRef[1];
} EFI_MISC_SYSTEM_OPTION_STRING_DATA;      

#define EFI_MISC_SYSTEM_OPTION_STRING_RECORD_NUMBER 0x0000000A

//
//////////////////////////////////////////////////////////////////////////////
//
// Misc. Number of Installable Languages - SMBIOS Type 13
//
#define EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES_RECORD_NUMBER 0x0000000B

typedef struct {
  UINT32                              AbbreviatedLanguageFormat :1;
  UINT32                              Reserved                  :31;
} EFI_MISC_LANGUAGE_FLAGS;

typedef struct {
  UINT16                              NumberOfInstallableLanguages;
  EFI_MISC_LANGUAGE_FLAGS             LanguageFlags;
  UINT16                              CurrentLanguageNumber;
} EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES_DATA;       

//
//////////////////////////////////////////////////////////////////////////////
//
// Misc. System Language String
//
#define EFI_MISC_SYSTEM_LANGUAGE_STRING_RECORD_NUMBER 0x0000000C

typedef struct {
  UINT16                              LanguageId;
  STRING_REF                          SystemLanguageString;
} EFI_MISC_SYSTEM_LANGUAGE_STRING_DATA;      

//
//////////////////////////////////////////////////////////////////////////////
//
// Group Associations - SMBIOS Type 14
//
#define EFI_MISC_GROUP_NAME_RECORD_NUMBER          0x0000000D

typedef struct {
  STRING_REF               GroupName;
  UINT16                   NumberGroupItems;
  UINT16                   GroupId;
} EFI_MISC_GROUP_NAME_DATA;  

//
//////////////////////////////////////////////////////////////////////////////
//
// Group Item Set Element
//
#define EFI_MISC_GROUP_ITEM_SET_RECORD_NUMBER      0x0000000E

typedef struct {
  EFI_GUID                 SubClass;         
  EFI_INTER_LINK_DATA      GroupLink;
  UINT16                   GroupId;
  UINT16                   GroupElementId;
} EFI_MISC_GROUP_ITEM_SET_DATA;

//
//////////////////////////////////////////////////////////////////////////////
//
//  Misc. Pointing Device Type - SMBIOS Type 21
//
#define EFI_MISC_POINTING_DEVICE_TYPE_RECORD_NUMBER 0x0000000F

typedef enum { 
  EfiPointingDeviceTypeOther         = 0x01,
  EfiPointingDeviceTypeUnknown       = 0x02,
  EfiPointingDeviceTypeMouse         = 0x03,
  EfiPointingDeviceTypeTrackBall     = 0x04,
  EfiPointingDeviceTypeTrackPoint    = 0x05,
  EfiPointingDeviceTypeGlidePoint    = 0x06,
  EfiPointingDeviceTouchPad          = 0x07,
  EfiPointingDeviceTouchScreen       = 0x08,
  EfiPointingDeviceOpticalSensor     = 0x09,
} EFI_MISC_POINTING_DEVICE_TYPE;

typedef enum {  
  EfiPointingDeviceInterfaceOther              = 0x01,
  EfiPointingDeviceInterfaceUnknown            = 0x02,
  EfiPointingDeviceInterfaceSerial             = 0x03,
  EfiPointingDeviceInterfacePs2                = 0x04,
  EfiPointingDeviceInterfaceInfrared           = 0x05,
  EfiPointingDeviceInterfaceHpHil              = 0x06,
  EfiPointingDeviceInterfaceBusMouse           = 0x07,
  EfiPointingDeviceInterfaceADB                = 0x08,
  EfiPointingDeviceInterfaceBusMouseDB9        = 0xA0,
  EfiPointingDeviceInterfaceBusMouseMicroDin   = 0xA1,
  EfiPointingDeviceInterfaceUsb                = 0xA2,
} EFI_MISC_POINTING_DEVICE_INTERFACE;

typedef struct {
  EFI_MISC_POINTING_DEVICE_TYPE       PointingDeviceType;
  EFI_MISC_POINTING_DEVICE_INTERFACE  PointingDeviceInterface;
  UINT16                              NumberPointingDeviceButtons;
  EFI_DEVICE_PATH_PROTOCOL            PointingDevicePath;
} EFI_MISC_PORTING_DEVICE_TYPE_DATA;

//
//////////////////////////////////////////////////////////////////////////////
//
// Portable Battery - SMBIOS Type 22
//
#define EFI_MISC_BATTERY_LOCATION_RECORD_NUMBER  0x00000010

typedef enum { 
  EfiBatteryDeviceChemistryTypeOther               = 0x01,
  EfiBatteryDeviceChemistryTypeUnknown             = 0x02,
  EfiBatteryDeviceChemistryTypeLeadAcid            = 0x03,
  EfiBatteryDeviceChemistryTypeNickelCadmium       = 0x04,
  EfiBatteryDeviceChemistryTypeNickelMetalHydride  = 0x05,
  EfiBatteryDeviceChemistryTypeLithiumIon          = 0x06,
  EfiBatteryDeviceChemistryTypeZincAir             = 0x07,
  EfiBatteryDeviceChemistryTypeLithiumPolymer      = 0x08,
} EFI_MISC_BATTERY_DEVICE_CHEMISTRY;

typedef struct  {
  UINT32 Date              :5;
  UINT32 Month             :4;
  UINT32 Year              :7;
  UINT32 Reserved          :16;
} EFI_MISC_BATTERY_SBDS_MANUFACTURE_DATE;

typedef struct {
  STRING_REF                         BatteryLocation;
  STRING_REF                         BatteryManufacturer;
  STRING_REF                         BatteryManufactureDate;
  STRING_REF                         BatterySerialNumber;
  STRING_REF                         BatteryDeviceName;
  STRING_REF                         BatterySbdsVersionNumber;
  STRING_REF                         BatterySbdsDeviceChemistry;
  EFI_MISC_BATTERY_DEVICE_CHEMISTRY  BatteryDeviceChemistry;
  EFI_EXP_BASE10_DATA                BatteryDesignCapacity;
  EFI_EXP_BASE10_DATA                BatteryDesignVoltage;
  UINT16                             BatteryMaximumError;
  UINT16                             BatterySbdsSerialNumber;
  EFI_MISC_BATTERY_SBDS_MANUFACTURE_DATE
                                     BatterySbdsManufacturingDate;
  UINT32                             BatteryOemSpecific;
} EFI_MISC_BATTERY_LOCATION_DATA;   

//
//////////////////////////////////////////////////////////////////////////////
//
// Misc. Reset Capabilities - SMBIOS Type 23
//
#define EFI_MISC_RESET_CAPABILITIES_RECORD_NUMBER 0x00000011

typedef struct {
  UINT32  Status              :1;
  UINT32  BootOption          :2;
  UINT32  BootOptionOnLimit   :2;
  UINT32  WatchdogTimerPresent:1;
  UINT32  Reserved            :26;
} EFI_MISC_RESET_CAPABILITIES_TYPE;

typedef struct {
  EFI_MISC_RESET_CAPABILITIES_TYPE  ResetCapabilities;
  UINT16                            ResetCount;
  UINT16                            ResetLimit;
  UINT16                            ResetTimerInterval;
  UINT16                            ResetTimeout;
} EFI_MISC_RESET_CAPABILITIES;
 
typedef struct {
    EFI_MISC_RESET_CAPABILITIES   ResetCapabilities;
    UINT16                        ResetCount;
    UINT16                        ResetLimit;
    UINT16                        ResetTimerInterval;
    UINT16                        ResetTimeout;
} EFI_MISC_RESET_CAPABILITIES_DATA;       

//
//////////////////////////////////////////////////////////////////////////////
//
// Misc. Hardware Security - SMBIOS Type 24
//
#define EFI_MISC_HARDWARE_SECURITY_RECORD_NUMBER 0x00000012

typedef enum {
  EfiHardwareSecurityStatusDisabled       = 0,
  EfiHardwareSecurityStatusEnabled        = 1,
  EfiHardwareSecurityStatusNotImplemented = 2,
  EfiHardwareSecurityStatusUnknown        = 3
} EFI_MISC_HARDWARE_SECURITY_STATUS; 

typedef struct {
  EFI_MISC_HARDWARE_SECURITY_STATUS   FrontPanelResetStatus       :2;  
  EFI_MISC_HARDWARE_SECURITY_STATUS   AdministratorPasswordStatus :2;  
  EFI_MISC_HARDWARE_SECURITY_STATUS   KeyboardPasswordStatus      :2;  
  EFI_MISC_HARDWARE_SECURITY_STATUS   PowerOnPasswordStatus       :2;  
  EFI_MISC_HARDWARE_SECURITY_STATUS   Reserved                    :24;  
} EFI_MISC_HARDWARE_SECURITY_SETTINGS;

typedef struct {
  EFI_MISC_HARDWARE_SECURITY_SETTINGS HardwareSecuritySettings;
} EFI_MISC_HARDWARE_SECURITY_SETTINGS_DATA;       

//
//////////////////////////////////////////////////////////////////////////////
//
// System Power Controls - SMBIOS Type 25
//
#define EFI_MISC_SCHEDULED_POWER_ON_MONTH_RECORD_NUMBER  0x00000013

typedef struct {
  UINT16             ScheduledPoweronMonth;
  UINT16             ScheduledPoweronDayOfMonth;
  UINT16             ScheduledPoweronHour;
  UINT16             ScheduledPoweronMinute;
  UINT16             ScheduledPoweronSecond;
} EFI_MISC_SCHEDULED_POWER_ON_MONTH_DATA;  

//
//////////////////////////////////////////////////////////////////////////////
//
// Voltage Probe - SMBIOS Type 26
//
#define EFI_MISC_VOLTAGE_PROBE_DESCRIPTION_RECORD_NUMBER  0x00000014

typedef struct {
  UINT32 VoltageProbeSite        :5;
  UINT32 VoltageProbeStatus      :3;
  UINT32 Reserved                :24;
} EFI_MISC_VOLTAGE_PROBE_LOCATION;

typedef struct {
  STRING_REF                      VoltageProbeDescription;
  EFI_MISC_VOLTAGE_PROBE_LOCATION VoltageProbeLocation;
  EFI_EXP_BASE10_DATA             VoltageProbeMaximumValue;
  EFI_EXP_BASE10_DATA             VoltageProbeMinimumValue;
  EFI_EXP_BASE10_DATA             VoltageProbeResolution;
  EFI_EXP_BASE10_DATA             VoltageProbeTolerance;
  EFI_EXP_BASE10_DATA             VoltageProbeAccuracy;
  EFI_EXP_BASE10_DATA             VoltageProbeNominalValue;
  EFI_EXP_BASE10_DATA             MDLowerNoncriticalThreshold;
  EFI_EXP_BASE10_DATA             MDUpperNoncriticalThreshold;
  EFI_EXP_BASE10_DATA             MDLowerCriticalThreshold;
  EFI_EXP_BASE10_DATA             MDUpperCriticalThreshold;
  EFI_EXP_BASE10_DATA             MDLowerNonrecoverableThreshold;
  EFI_EXP_BASE10_DATA             MDUpperNonrecoverableThreshold;
  UINT32                          VoltageProbeOemDefined;
} EFI_MISC_VOLTAGE_PROBE_DESCRIPTION_DATA; 

//
//////////////////////////////////////////////////////////////////////////////
//
// Cooling Device - SMBIOS Type 27
//
#define EFI_MISC_COOLING_DEVICE_TEMP_LINK_RECORD_NUMBER   0x00000015

typedef struct {
  UINT32 CoolingDevice                 :5;
  UINT32 CoolingDeviceStatus           :3;
  UINT32 Reserved                      :24;
} EFI_MISC_COOLING_DEVICE_TYPE;

typedef struct {
  EFI_MISC_COOLING_DEVICE_TYPE   CoolingDeviceType;
  EFI_INTER_LINK_DATA            CoolingDeviceTemperatureLink;
  UINT16                         CoolingDeviceUnitGroup;
  EFI_EXP_BASE10_DATA            CoolingDeviceNominalSpeed;
  UINT32                         CoolingDeviceOemDefined;
} EFI_MISC_COOLING_DEVICE_TEMP_LINK_DATA; 

//
//////////////////////////////////////////////////////////////////////////////
//
// Temperature Probe - SMBIOS Type 28
//
#define EFI_MISC_TEMPERATURE_PROBE_DESCRIPTION_RECORD_NUMBER   0x00000016

typedef struct {
  UINT32 TemperatureProbeSite          :5;
  UINT32 TemperatureProbeStatus        :3;
  UINT32 Reserved                      :24;
} EFI_MISC_TEMPERATURE_PROBE_LOCATION;

typedef struct {
  STRING_REF               TemperatureProbeDescription;
  EFI_MISC_TEMPERATURE_PROBE_LOCATION
                           TemperatureProbeLocation;
  EFI_EXP_BASE10_DATA      TemperatureProbeMaximumValue;
  EFI_EXP_BASE10_DATA      TemperatureProbeMinimumValue;
  EFI_EXP_BASE10_DATA      TemperatureProbeResolution;
  EFI_EXP_BASE10_DATA      TemperatureProbeTolerance;
  EFI_EXP_BASE10_DATA      TemperatureProbeAccuracy;
  EFI_EXP_BASE10_DATA      TemperatureProbeNominalValue; 
  EFI_EXP_BASE10_DATA      MDLowerNoncriticalThreshold;
  EFI_EXP_BASE10_DATA      MDUpperNoncriticalThreshold;
  EFI_EXP_BASE10_DATA      MDLowerCriticalThreshold;
  EFI_EXP_BASE10_DATA      MDUpperCriticalThreshold;
  EFI_EXP_BASE10_DATA      MDLowerNonrecoverableThreshold;
  EFI_EXP_BASE10_DATA      MDUpperNonrecoverableThreshold;
  UINT32                   TemperatureProbeOemDefined;
} EFI_MISC_TEMPERATURE_PROBE_DESCRIPTION_DATA;  

//
//////////////////////////////////////////////////////////////////////////////
//
// Electrical Current Probe - SMBIOS Type 29
//

#define EFI_MISC_ELECTRICAL_CURRENT_PROBE_DESCRIPTION_RECORD_NUMBER  0x00000017

typedef struct {
  UINT32 ElectricalCurrentProbeSite    :5;
  UINT32 ElectricalCurrentProbeStatus  :3;
  UINT32 Reserved                      :24;
} EFI_MISC_ELECTRICAL_CURRENT_PROBE_LOCATION;

typedef struct {
  STRING_REF               ElectricalCurrentProbeDescription;
  EFI_MISC_ELECTRICAL_CURRENT_PROBE_LOCATION
                           ElectricalCurrentProbeLocation;
  EFI_EXP_BASE10_DATA      ElectricalCurrentProbeMaximumValue;
  EFI_EXP_BASE10_DATA      ElectricalCurrentProbeMinimumValue;
  EFI_EXP_BASE10_DATA      ElectricalCurrentProbeResolution;
  EFI_EXP_BASE10_DATA      ElectricalCurrentProbeTolerance;
  EFI_EXP_BASE10_DATA      ElectricalCurrentProbeAccuracy;
  EFI_EXP_BASE10_DATA      ElectricalCurrentProbeNominalValue;
  EFI_EXP_BASE10_DATA      MDLowerNoncriticalThreshold;
  EFI_EXP_BASE10_DATA      MDUpperNoncriticalThreshold;
  EFI_EXP_BASE10_DATA      MDLowerCriticalThreshold;
  EFI_EXP_BASE10_DATA      MDUpperCriticalThreshold;
  EFI_EXP_BASE10_DATA      MDLowerNonrecoverableThreshold;
  EFI_EXP_BASE10_DATA      MDUpperNonrecoverableThreshold;
  UINT32                   ElectricalCurrentProbeOemDefined;
} EFI_MISC_ELECTRICAL_CURRENT_PROBE_DESCRIPTION_DATA;

//
//////////////////////////////////////////////////////////////////////////////
//
// Out-of-Band Remote Access - SMBIOS Type 30
//

#define EFI_MISC_REMOTE_ACCESS_MANUFACTURER_DESCRIPTION_RECORD_NUMBER  0x00000018

typedef struct  {
  UINT32 InboundConnectionEnabled            :1;
  UINT32 OutboundConnectionEnabled           :1;
  UINT32 Reserved                            :30;
} EFI_MISC_REMOTE_ACCESS_CONNECTIONS;

typedef struct {
  STRING_REF                             RemoteAccessManufacturerNameDescription;
  EFI_MISC_REMOTE_ACCESS_CONNECTIONS     RemoteAccessConnections;
} EFI_MISC_REMOTE_ACCESS_MANUFACTURER_DESCRIPTION_DATA;

//
//////////////////////////////////////////////////////////////////////////////
//
// Misc. BIS Entry Point - SMBIOS Type 31
//
#define EFI_MISC_BIS_ENTRY_POINT_RECORD_NUMBER          0x00000019

typedef struct {
  EFI_PHYSICAL_ADDRESS       BisEntryPoint;
} EFI_MISC_BIS_ENTRY_POINT_DATA;    

//
//////////////////////////////////////////////////////////////////////////////
//
// Misc. Boot Information - SMBIOS Type 32
//
#define EFI_MISC_BOOT_INFORMATION_STATUS_RECORD_NUMBER  0x0000001A

typedef enum {  
  EfiBootInformationStatusNoError                  = 0x00,
  EfiBootInformationStatusNoBootableMedia          = 0x01,
  EfiBootInformationStatusNormalOSFailedLoading    = 0x02,
  EfiBootInformationStatusFirmwareDetectedFailure  = 0x03,
  EfiBootInformationStatusOSDetectedFailure        = 0x04,
  EfiBootInformationStatusUserRequestedBoot        = 0x05,
  EfiBootInformationStatusSystemSecurityViolation  = 0x06,
  EfiBootInformationStatusPreviousRequestedImage   = 0x07,
  EfiBootInformationStatusWatchdogTimerExpired     = 0x08,
  EfiBootInformationStatusStartReserved            = 0x09,
  EfiBootInformationStatusStartOemSpecific         = 0x80,
  EfiBootInformationStatusStartProductSpecific     = 0xC0,
} EFI_MISC_BOOT_INFORMATION_STATUS_DATA_TYPE;

typedef struct {
  EFI_MISC_BOOT_INFORMATION_STATUS_DATA_TYPE BootInformationStatus;
  UINT8                                      BootInformationData[9];
} EFI_MISC_BOOT_INFORMATION_STATUS_DATA;

//
//////////////////////////////////////////////////////////////////////////////
//
// Management Device - SMBIOS Type 34
//
#define EFI_MISC_MANAGEMENT_DEVICE_DESCRIPTION_RECORD_NUMBER   0x0000001B

typedef enum { 
  EfiManagementDeviceTypeOther      = 0x01,
  EfiManagementDeviceTypeUnknown    = 0x02,
  EfiManagementDeviceTypeLm75       = 0x03,
  EfiManagementDeviceTypeLm78       = 0x04,
  EfiManagementDeviceTypeLm79       = 0x05,
  EfiManagementDeviceTypeLm80       = 0x06,
  EfiManagementDeviceTypeLm81       = 0x07,
  EfiManagementDeviceTypeAdm9240    = 0x08,
  EfiManagementDeviceTypeDs1780     = 0x09,
  EfiManagementDeviceTypeMaxim1617  = 0x0A,
  EfiManagementDeviceTypeGl518Sm    = 0x0B,
  EfiManagementDeviceTypeW83781D    = 0x0C,
  EfiManagementDeviceTypeHt82H791   = 0x0D,
} EFI_MISC_MANAGEMENT_DEVICE_TYPE;

typedef enum { 
  EfiManagementDeviceAddressTypeOther   = 1,
  EfiManagementDeviceAddressTypeUnknown = 2,
  EfiManagementDeviceAddressTypeIOPort  = 3,
  EfiManagementDeviceAddressTypeMemory  = 4,
  EfiManagementDeviceAddressTypeSmbus   = 5
} EFI_MISC_MANAGEMENT_DEVICE_ADDRESS_TYPE;

typedef struct {
  STRING_REF                       ManagementDeviceDescription;
  EFI_MISC_MANAGEMENT_DEVICE_TYPE  ManagementDeviceType;
  UINTN                            ManagementDeviceAddress;
  EFI_MISC_MANAGEMENT_DEVICE_ADDRESS_TYPE
                                   ManagementDeviceAddressType;
} EFI_MISC_MANAGEMENT_DEVICE_DESCRIPTION_DATA; 

//
//////////////////////////////////////////////////////////////////////////////
//
// Management Device Component - SMBIOS Type 35
//

#define EFI_MISC_MANAGEMENT_DEVICE_COMPONENT_DESCRIPTION_RECORD_NUMBER  0x0000001C

typedef struct {
  STRING_REF               ManagementDeviceComponentDescription;
  EFI_INTER_LINK_DATA      ManagementDeviceLink;
  EFI_INTER_LINK_DATA      ManagementDeviceComponentLink; 
} EFI_MISC_MANAGEMENT_DEVICE_COMPONENT_DESCRIPTION_DATA; 

//
//////////////////////////////////////////////////////////////////////////////
//
// IPMI Data Record - SMBIOS Type 38
//
typedef enum {  
  EfiIpmiOther = 0,
  EfiIpmiKcs   = 1,
  EfiIpmiSmic  = 2,
  EfiIpmiBt    = 3,
} EFI_MISC_IPMI_INTERFACE_TYPE;

typedef struct {
  UINT16  IpmiSpecLeastSignificantDigit:4;
  UINT16  IpmiSpecMostSignificantDigit: 4;
  UINT16  Reserved:                     8;
} EFI_MISC_IPMI_SPECIFICATION_REVISION;

typedef struct {
  EFI_MISC_IPMI_INTERFACE_TYPE          IpmiInterfaceType;
  EFI_MISC_IPMI_SPECIFICATION_REVISION  IpmiSpecificationRevision;
  UINT16                                IpmiI2CSlaveAddress;
  UINT16                                IpmiNvDeviceAddress;
  UINT64                                IpmiBaseAddress;
  EFI_DEVICE_PATH_PROTOCOL              IpmiDevicePath;
} EFI_MISC_IPMI_INTERFACE_TYPE_DATA;
       
#define EFI_MISC_IPMI_INTERFACE_TYPE_RECORD_NUMBER  0x0000001D

//
//////////////////////////////////////////////////////////////////////////////
//
//System Power supply Record - SMBIOS Type 39
//
typedef struct {
  UINT16  PowerSupplyHotReplaceable  :1;
  UINT16  PowerSupplyPresent         :1;
  UINT16  PowerSupplyUnplugged       :1;
  UINT16  InputVoltageRangeSwitch    :4;
  UINT16  PowerSupplyStatus          :3;
  UINT16  PowerSupplyType            :4;
  UINT16  Reserved                   :2;
} POWER_SUPPLY_CHARACTERISTICS;

typedef struct {
  UINT16                          PowerUnitGroup;
  STRING_REF                      PowerSupplyLocation;
  STRING_REF                      PowerSupplyDeviceName;
  STRING_REF                      PowerSupplyManufacturer;
  STRING_REF                      PowerSupplySerialNumber;
  STRING_REF                      PowerSupplyAssetTagNumber;
  STRING_REF                      PowerSupplyModelPartNumber;
  STRING_REF                      PowerSupplyRevisionLevel;
  UINT16                          PowerSupplyMaxPowerCapacity;
  POWER_SUPPLY_CHARACTERISTICS    PowerSupplyCharacteristics;
  EFI_INTER_LINK_DATA             PowerSupplyInputVoltageProbeLink;
  EFI_INTER_LINK_DATA             PowerSupplyCoolingDeviceLink;
  EFI_INTER_LINK_DATA             PowerSupplyInputCurrentProbeLink;
} EFI_MISC_SYSTEM_POWER_SUPPLY_DATA;

#define EFI_MISC_SYSTEM_POWER_SUPPLY_RECORD_NUMBER 0x0000001E

//
//////////////////////////////////////////////////////////////////////////////
//
// OEM Data Record - SMBIOS Type 0x80-0xFF
//
typedef struct {
  UINT8       Type;
  UINT8       Length;
  UINT16      Handle;
} SMBIOS_STRUCTURE_HDR;

typedef struct {
  SMBIOS_STRUCTURE_HDR          Header;
  UINT8                         RawData[1];
} EFI_MISC_SMBIOS_STRUCT_ENCAPSULATION_DATA; 

#define EFI_MISC_SMBIOS_STRUCT_ENCAP_RECORD_NUMBER  0x0000001F 

//
// Declare the following strutures alias to use them more conviniently.
//
typedef EFI_MISC_LAST_PCI_BUS_DATA                        EFI_MISC_LAST_PCI_BUS;
typedef EFI_MISC_BIOS_VENDOR_DATA                         EFI_MISC_BIOS_VENDOR;
typedef EFI_MISC_SYSTEM_MANUFACTURER_DATA                 EFI_MISC_SYSTEM_MANUFACTURER;
typedef EFI_MISC_BASE_BOARD_MANUFACTURER_DATA             EFI_MISC_BASE_BOARD_MANUFACTURER;
typedef EFI_MISC_CHASSIS_MANUFACTURER_DATA                EFI_MISC_CHASSIS_MANUFACTURER;
typedef EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA  EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR;
typedef EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA             EFI_MISC_SYSTEM_SLOT_DESIGNATION;
typedef EFI_MISC_ONBOARD_DEVICE_DATA                      EFI_MISC_ONBOARD_DEVICE;
typedef EFI_MISC_PORTING_DEVICE_TYPE_DATA                 EFI_MISC_ONBOARD_DEVICE_TYPE_DATA;
typedef EFI_MISC_OEM_STRING_DATA                          EFI_MISC_OEM_STRING;
typedef EFI_MISC_SYSTEM_OPTION_STRING_DATA                EFI_MISC_SYSTEM_OPTION_STRING;
typedef EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES_DATA     EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES;
typedef EFI_MISC_SYSTEM_LANGUAGE_STRING_DATA              EFI_MISC_SYSTEM_LANGUAGE_STRING;
typedef EFI_MISC_BIS_ENTRY_POINT_DATA                     EFI_MISC_BIS_ENTRY_POINT;
typedef EFI_MISC_BOOT_INFORMATION_STATUS_DATA             EFI_MISC_BOOT_INFORMATION_STATUS;
typedef EFI_MISC_SYSTEM_POWER_SUPPLY_DATA                 EFI_MISC_SYSTEM_POWER_SUPPLY;
typedef EFI_MISC_SMBIOS_STRUCT_ENCAPSULATION_DATA         EFI_MISC_SMBIOS_STRUCT_ENCAPSULATION;

typedef union {
  EFI_MISC_LAST_PCI_BUS_DATA                         LastPciBus;
  EFI_MISC_BIOS_VENDOR_DATA                          MiscBiosVendor;
  EFI_MISC_SYSTEM_MANUFACTURER_DATA                  MiscSystemManufacturer;
  EFI_MISC_BASE_BOARD_MANUFACTURER_DATA              MiscBaseBoardManufacturer;
  EFI_MISC_CHASSIS_MANUFACTURER_DATA                 MiscChassisManufacturer;  
  EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA   MiscPortInternalConnectorDesignator;
  EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA              MiscSystemSlotDesignation;
  EFI_MISC_ONBOARD_DEVICE_DATA                       MiscOnboardDevice;
  EFI_MISC_OEM_STRING_DATA                           MiscOemString;
  EFI_MISC_SYSTEM_OPTION_STRING_DATA                 MiscOptionString;
  EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES_DATA      NumberOfInstallableLanguages;
  EFI_MISC_SYSTEM_LANGUAGE_STRING_DATA               MiscSystemLanguageString;  
  EFI_MISC_GROUP_NAME_DATA                           MiscGroupNameData;
  EFI_MISC_GROUP_ITEM_SET_DATA                       MiscGroupItemSetData;
  EFI_MISC_PORTING_DEVICE_TYPE_DATA                  MiscPortingDeviceTypeData;
  EFI_MISC_RESET_CAPABILITIES_DATA                   MiscResetCapablilitiesData;
  EFI_MISC_HARDWARE_SECURITY_SETTINGS_DATA           MiscHardwareSecuritySettingsData;  
  EFI_MISC_SCHEDULED_POWER_ON_MONTH_DATA             MiscScheduledPowerOnMonthData;
  EFI_MISC_VOLTAGE_PROBE_DESCRIPTION_DATA            MiscVoltagePorbeDescriptionData;
  EFI_MISC_COOLING_DEVICE_TEMP_LINK_DATA             MiscCoolingDeviceTempLinkData;
  EFI_MISC_TEMPERATURE_PROBE_DESCRIPTION_DATA        MiscTemperatureProbeDescriptionData;   
  EFI_MISC_ELECTRICAL_CURRENT_PROBE_DESCRIPTION_DATA MiscElectricalCurrentProbeDescriptionData;
  EFI_MISC_REMOTE_ACCESS_MANUFACTURER_DESCRIPTION_DATA
                                                     MiscRemoteAccessManufacturerDescriptionData;
  EFI_MISC_BIS_ENTRY_POINT_DATA                      MiscBisEntryPoint;
  EFI_MISC_BOOT_INFORMATION_STATUS_DATA              MiscBootInformationStatus;
  EFI_MISC_MANAGEMENT_DEVICE_DESCRIPTION_DATA        MiscMangementDeviceDescriptionData;
  EFI_MISC_MANAGEMENT_DEVICE_COMPONENT_DESCRIPTION_DATA
                                                     MiscmangementDeviceComponentDescriptionData;
  EFI_MISC_IPMI_INTERFACE_TYPE_DATA                  MiscIpmiInterfaceTypeData;
  EFI_MISC_SYSTEM_POWER_SUPPLY_DATA                  MiscPowerSupplyInfo;
  EFI_MISC_SMBIOS_STRUCT_ENCAPSULATION_DATA          MiscSmbiosStructEncapsulation;  
} EFI_MISC_SUBCLASS_RECORDS;

//
//
//
typedef struct {
  EFI_SUBCLASS_TYPE1_HEADER       Header;
  EFI_MISC_SUBCLASS_RECORDS       Record;
} EFI_MISC_SUBCLASS_DRIVER_DATA;

#pragma pack()

//
// Sub Class Header type1
//

#define EFI_SUBCLASS_INSTANCE_RESERVED       0
#define EFI_SUBCLASS_INSTANCE_NON_APPLICABLE 0xFFFF  //16 bit

#endif

