/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DataHubSubClassProcessor.h

Abstract:

  Definitions for processor sub class data records

Revision History

--*/

#ifndef _DATAHUB_SUBCLASS_PROCESSOR_H_
#define _DATAHUB_SUBCLASS_PROCESSOR_H_

#define EFI_PROCESSOR_SUBCLASS_VERSION    0x00010000

#define EFI_PROCESSOR_SUBCLASS_GUID \
  { 0x26fdeb7e, 0xb8af, 0x4ccf, {0xaa, 0x97, 0x02, 0x63, 0x3c, 0xe4, 0x8c, 0xa7} }


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

typedef STRING_REF            EFI_PROCESSOR_PART_NUMBER_DATA;

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
  EfiProcessorOther = 1,
  EfiProcessorUnknown = 2,
  EfiCentralProcessor = 3,
  EfiMathProcessor = 4,
  EfiDspProcessor = 5,
  EfiVideoProcessor = 6
} EFI_PROCESSOR_TYPE_DATA;

typedef enum {
  EfiProcessorFamilyOther        = 1, 
  EfiProcessorFamilyUnknown      = 2,
  EfiProcessorFamily8086         = 3, 
  EfiProcessorFamily80286        = 4,
  EfiProcessorFamilyIntel386     = 5, 
  EfiProcessorFamilyIntel486     = 6,
  EfiProcessorFamily8087         = 7,
  EfiProcessorFamily80287        = 8,
  EfiProcessorFamily80387        = 9, 
  EfiProcessorFamily80487        = 0x0A,
  EfiProcessorFamilyPentium      = 0x0B, 
  EfiProcessorFamilyPentiumPro   = 0x0C,
  EfiProcessorFamilyPentiumII    = 0x0D,
  EfiProcessorFamilyPentiumMMX   = 0x0E,
  EfiProcessorFamilyCeleron      = 0x0F,
  EfiProcessorFamilyPentiumIIXeon = 0x10,
  EfiProcessorFamilyPentiumIII   = 0x11, 
  EfiProcessorFamilyM1           = 0x12,
  EfiProcessorFamilyM2           = 0x13,
  EfiProcessorFamilyM1Reserved2 = 0x14,
  EfiProcessorFamilyM1Reserved3  = 0x15,
  EfiProcessorFamilyM1Reserved4  = 0x16,
  EfiProcessorFamilyM1Reserved5  = 0x17,
  EfiProcessorFamilyAmdDuron     = 0x18,
  EfiProcessorFamilyK5           = 0x19, 
  EfiProcessorFamilyK6           = 0x1A,
  EfiProcessorFamilyK6_2         = 0x1B,
  EfiProcessorFamilyK6_3         = 0x1C,
  EfiProcessorFamilyAmdAthlon    = 0x1D,
  EfiProcessorFamilyK6_2Plus     = 0x1E,
  EfiProcessorFamilyK5Reserved6  = 0x1F,
  EfiProcessorFamilyPowerPC      = 0x20,
  EfiProcessorFamilyPowerPC601   = 0x21,
  EfiProcessorFamilyPowerPC603   = 0x22,
  EfiProcessorFamilyPowerPC603Plus = 0x23,
  EfiProcessorFamilyPowerPC604   = 0x24,
  EfiProcessorFamilyPowerPC620   = 0x25,
  EfiProcessorFamilyPowerPC704   = 0x26,
  EfiProcessorFamilyPowerPC750   = 0x27,
  EfiProcessorFamilyIntelCoreDuo = 0x28,
  EfiProcessorFamilyIntelCoreDuoMobile = 0x29,
  EfiProcessorFamilyIntelCoreSoloMobile = 0x2A,
  EfiProcessorFamilyIntelAtom    = 0x2B,
  EfiProcessorFamilyAlpha2       = 0x30,
  EfiProcessorFamilyAlpha21064   = 0x31,
  EfiProcessorFamilyAlpha21066   = 0x32,
  EfiProcessorFamilyAlpha21164   = 0x33,
  EfiProcessorFamilyAlpha21164PC = 0x34,
  EfiProcessorFamilyAlpha21164a  = 0x35,
  EfiProcessorFamilyAlpha21264   = 0x36,
  EfiProcessorFamilyAlpha21364   = 0x37,
  EfiProcessorFamilyMips         = 0x40,
  EfiProcessorFamilyMIPSR4000    = 0x41,
  EfiProcessorFamilyMIPSR4200    = 0x42,
  EfiProcessorFamilyMIPSR4400    = 0x43,
  EfiProcessorFamilyMIPSR4600    = 0x44,
  EfiProcessorFamilyMIPSR10000   = 0x45,
  EfiProcessorFamilySparc        = 0x50,
  EfiProcessorFamilySuperSparc   = 0x51,
  EfiProcessorFamilymicroSparcII = 0x52,
  EfiProcessorFamilymicroSparcIIep = 0x53,
  EfiProcessorFamilyUltraSparc   = 0x54,
  EfiProcessorFamilyUltraSparcII = 0x55,
  EfiProcessorFamilyUltraSparcIIi = 0x56,
  EfiProcessorFamilyUltraSparcIII = 0x57,
  EfiProcessorFamilyUltraSparcIIIi = 0x58,
  EfiProcessorFamily68040        = 0x60,
  EfiProcessorFamily68xxx        = 0x61,
  EfiProcessorFamily68000        = 0x62,
  EfiProcessorFamily68010        = 0x63,
  EfiProcessorFamily68020        = 0x64,
  EfiProcessorFamily68030        = 0x65,
  EfiProcessorFamilyHobbit       = 0x70,
  EfiProcessorFamilyCrusoeTM5000 = 0x78,
  EfiProcessorFamilyCrusoeTM3000 = 0x79,
  EfiProcessorFamilyEfficeonTM8000 = 0x7A,
  EfiProcessorFamilyWeitek       = 0x80,
  EfiProcessorFamilyItanium      = 0x82,
  EfiProcessorFamilyAmdAthlon64  = 0x83,
  EfiProcessorFamilyAmdOpteron   = 0x84,
  EfiProcessorFamilyAmdSempron   = 0x85,
  EfiProcessorFamilyAmdTurion64Mobile = 0x86,
  EfiProcessorFamilyDualCoreAmdOpteron = 0x87,
  EfiProcessorFamilyAmdAthlon64X2DualCore = 0x88,
  EfiProcessorFamilyAmdTurion64X2Mobile   = 0x89,
  EfiProcessorFamilyQuadCoreAmdOpteron = 0x8A,
  EfiProcessorFamilyThirdGenerationAmdOpteron = 0x8B,
  EfiProcessorFamilyAmdPhenomFxQuadCore = 0x8C,
  EfiProcessorFamilyAmdPhenomX4QuadCore = 0x8D,
  EfiProcessorFamilyAmdPhenomX2DualCore = 0x8E,
  EfiProcessorFamilyAmdAthlonX2DualCore = 0x8F,
  EfiProcessorFamilyPARISC       = 0x90,
  EfiProcessorFamilyPaRisc8500   = 0x91,
  EfiProcessorFamilyPaRisc8000   = 0x92,
  EfiProcessorFamilyPaRisc7300LC = 0x93,
  EfiProcessorFamilyPaRisc7200   = 0x94,
  EfiProcessorFamilyPaRisc7100LC = 0x95,
  EfiProcessorFamilyPaRisc7100   = 0x96,
  EfiProcessorFamilyV30          = 0xA0,
  EfiProcessorFamilyQuadCoreIntelXeon3200Series  = 0xA1,
  EfiProcessorFamilyDualCoreIntelXeon3000Series  = 0xA2,
  EfiProcessorFamilyQuadCoreIntelXeon5300Series  = 0xA3,
  EfiProcessorFamilyDualCoreIntelXeon5100Series  = 0xA4,
  EfiProcessorFamilyDualCoreIntelXeon5000Series  = 0xA5,
  EfiProcessorFamilyDualCoreIntelXeonLV          = 0xA6,
  EfiProcessorFamilyDualCoreIntelXeonULV         = 0xA7,
  EfiProcessorFamilyDualCoreIntelXeon7100Series  = 0xA8,
  EfiProcessorFamilyQuadCoreIntelXeon5400Series  = 0xA9,
  EfiProcessorFamilyQuadCoreIntelXeon            = 0xAA,
  EfiProcessorFamilyDualCoreIntelXeon5200Series  = 0xAB,
  EfiProcessorFamilyDualCoreIntelXeon7200Series  = 0xAC,
  EfiProcessorFamilyQuadCoreIntelXeon7300Series  = 0xAD,
  EfiProcessorFamilyQuadCoreIntelXeon7400Series  = 0xAE,
  EfiProcessorFamilyMultiCoreIntelXeon7400Series = 0xAF,
  EfiProcessorFamilyPentiumIIIXeon = 0xB0,
  EfiProcessorFamilyPentiumIIISpeedStep = 0xB1,
  EfiProcessorFamilyPentium4     = 0xB2,
  EfiProcessorFamilyIntelXeon    = 0xB3,
  EfiProcessorFamilyAS400        = 0xB4,
  EfiProcessorFamilyIntelXeonMP  = 0xB5,
  EfiProcessorFamilyAMDAthlonXP = 0xB6,
  EfiProcessorFamilyAMDAthlonMP = 0xB7,
  EfiProcessorFamilyIntelItanium2 = 0xB8,
  EfiProcessorFamilyIntelPentiumM = 0xB9,
  EfiProcessorFamilyIntelCeleronD = 0xBA,
  EfiProcessorFamilyIntelPentiumD = 0xBB,
  EfiProcessorFamilyIntelPentiumEx = 0xBC,
  EfiProcessorFamilyIntelCoreSolo  = 0xBD,  // SMBIOS spec 2.6 correct this value
  EfiProcessorFamilyReserved       = 0xBE,
  EfiProcessorFamilyIntelCore2     = 0xBF,
  EfiProcessorFamilyIntelCore2Solo          = 0xC0,
  EfiProcessorFamilyIntelCore2Extreme       = 0xC1,
  EfiProcessorFamilyIntelCore2Quad          = 0xC2,
  EfiProcessorFamilyIntelCore2ExtremeMobile = 0xC3,
  EfiProcessorFamilyIntelCore2DuoMobile     = 0xC4,
  EfiProcessorFamilyIntelCore2SoloMobile    = 0xC5,
  EfiProcessorFamilyIntelCoreI7             = 0xC6,
  EfiProcessorFamilyDualCoreIntelCeleron    = 0xC7,
  EfiProcessorFamilyIBM390       = 0xC8,
  EfiProcessorFamilyG4           = 0xC9,
  EfiProcessorFamilyG5           = 0xCA,
  EfiProcessorFamilyG6           = 0xCB,
  EfiProcessorFamilyzArchitectur = 0xCC,
  EfiProcessorFamilyViaC7M      = 0xD2,
  EfiProcessorFamilyViaC7D      = 0xD3,
  EfiProcessorFamilyViaC7       = 0xD4,
  EfiProcessorFamilyViaEden     = 0xD5,
  EfiProcessorFamilyMultiCoreIntelXeon           = 0xD6,
  EfiProcessorFamilyDualCoreIntelXeon3Series     = 0xD7,
  EfiProcessorFamilyQuadCoreIntelXeon3Series     = 0xD8,
  EfiProcessorFamilyDualCoreIntelXeon5Series     = 0xDA,
  EfiProcessorFamilyQuadCoreIntelXeon5Series     = 0xDB,
  EfiProcessorFamilyDualCoreIntelXeon7Series     = 0xDD,
  EfiProcessorFamilyQuadCoreIntelXeon7Series     = 0xDE,
  EfiProcessorFamilyMultiCoreIntelXeon7Series    = 0xDF,
  EfiProcessorFamilyEmbeddedAmdOpteronQuadCore   = 0xE6,
  EfiProcessorFamilyAmdPhenomTripleCore          = 0xE7,
  EfiProcessorFamilyAmdTurionUltraDualCoreMobile = 0xE8,
  EfiProcessorFamilyAmdTurionDualCoreMobile      = 0xE9,
  EfiProcessorFamilyAmdAthlonDualCore            = 0xEA,
  EfiProcessorFamilyAmdSempronSI                 = 0xEB,
  EfiProcessorFamilyi860         = 0xFA,
  EfiProcessorFamilyi960         = 0xFB,
  EfiProcessorFamilyIndicatorFamily2    = 0xFE
} EFI_PROCESSOR_FAMILY_DATA;

typedef enum {
  EfiProcessorFamilySh3           = 0x104,
  EfiProcessorFamilySh4           = 0x105,
  EfiProcessorFamilyArm           = 0x118,
  EfiProcessorFamilyStrongArm     = 0x119,
  EfiProcessorFamily6x86          = 0x12C,
  EfiProcessorFamilyMediaGx       = 0x12D,
  EfiProcessorFamilyMii           = 0x12E,
  EfiProcessorFamilyWinChip       = 0x140,
  EfiProcessorFamilyDsp           = 0x15E,
  EfiProcessorFamilyVideo         = 0x1F4
} EFI_PROCESSOR_FAMILY2_DATA;  

typedef EFI_EXP_BASE10_DATA EFI_PROCESSOR_VOLTAGE_DATA;

typedef EFI_PHYSICAL_ADDRESS EFI_PROCESSOR_APIC_BASE_ADDRESS_DATA;

typedef UINT32 EFI_PROCESSOR_APIC_ID_DATA;

typedef UINT32 EFI_PROCESSOR_APIC_VERSION_NUMBER_DATA;

typedef enum {
  EfiProcessorIa32Microcode = 1,
  EfiProcessorIpfPalAMicrocode = 2,
  EfiProcessorIpfPalBMicrocode = 3
} EFI_PROCESSOR_MICROCODE_TYPE;

typedef struct {
  EFI_PROCESSOR_MICROCODE_TYPE  ProcessorMicrocodeType;
  UINT32                        ProcessorMicrocodeRevisionNumber;
} EFI_PROCESSOR_MICROCODE_REVISION_DATA;

typedef struct {
  UINT32      CpuStatus       :3;
  UINT32      Reserved1        :3;
  UINT32      SocketPopulated     :1;
  UINT32      Reserved2        :1;
  UINT32      ApicEnable        :1;
  UINT32      BootApplicationProcessor  :1;
  UINT32      Reserved3        :22;
} EFI_PROCESSOR_STATUS_DATA;

typedef enum {
  EfiCpuStatusUnknown = 0,
  EfiCpuStatusEnabled = 1,
  EfiCpuStatusDisabledByUser = 2,
  EfiCpuStatusDisabledbyBios = 3,
  EfiCpuStatusIdle = 4,
  EfiCpuStatusOther = 7
} EFI_CPU_STATUS;

typedef enum {
  EfiProcessorSocketOther = 1,
  EfiProcessorSocketUnknown = 2,
  EfiProcessorSocketDaughterBoard = 3,
  EfiProcessorSocketZIF = 4,
  EfiProcessorSocketReplacePiggyBack = 5,
  EfiProcessorSocketNone = 6,
  EfiProcessorSocketLIF = 7,
  EfiProcessorSocketSlot1 = 8,
  EfiProcessorSocketSlot2 = 9,
  EfiProcessorSocket370Pin = 0xA,
  EfiProcessorSocketSlotA = 0xB,
  EfiProcessorSocketSlotM = 0xC,
  EfiProcessorSocket423 = 0xD,
  EfiProcessorSocketA462 = 0xE,
  EfiProcessorSocket478 = 0xF,
  EfiProcessorSocket754 = 0x10,
  EfiProcessorSocket940 = 0x11,
  EfiProcessorSocket939 = 0x12,
  EfiProcessorSocketmPGA604 = 0x13,
  EfiProcessorSocketLGA771 = 0x14,
  EfiProcessorSocketLGA775 = 0x15,
  EfiProcessorSocketS1 = 0x16,
  EfiProcessorSocketAm2 = 0x17,
  EfiProcessorSocketF   = 0x18,
  EfiProcessorSocketLGA1366 = 0x19
} EFI_PROCESSOR_SOCKET_TYPE_DATA;

typedef STRING_REF EFI_PROCESSOR_SOCKET_NAME_DATA;

typedef EFI_INTER_LINK_DATA EFI_CACHE_ASSOCIATION_DATA;

typedef enum {
  EfiProcessorHealthy = 1,
  EfiProcessorPerfRestricted = 2,
  EfiProcessorFuncRestricted = 3
} EFI_PROCESSOR_HEALTH_STATUS;  

typedef UINTN   EFI_PROCESSOR_PACKAGE_NUMBER_DATA;

typedef UINT8   EFI_PROCESSOR_CORE_COUNT_DATA;
typedef UINT8   EFI_PROCESSOR_ENABLED_CORE_COUNT_DATA;
typedef UINT8   EFI_PROCESSOR_THREAD_COUNT_DATA;

typedef struct {
  UINT16  Reserved              :1;
  UINT16  Unknown               :1;
  UINT16  Capable64Bit          :1;
  UINT16  Reserved2             :13;
} EFI_PROCESSOR_CHARACTERISTICS_DATA;

typedef struct {
  EFI_PROCESSOR_SOCKET_NAME_DATA          ProcessorSocketName;
  EFI_PROCESSOR_TYPE_DATA                 ProcessorType;
  EFI_PROCESSOR_FAMILY_DATA               ProcessorFamily;
  EFI_PROCESSOR_MANUFACTURER_DATA         ProcessorManufacturer;
  EFI_PROCESSOR_ID_DATA                   ProcessorId;
  EFI_PROCESSOR_VERSION_DATA              ProcessorVersion;
  EFI_PROCESSOR_VOLTAGE_DATA              ProcessorVoltage;
  EFI_PROCESSOR_FSB_FREQUENCY_DATA        ProcessorFsbFrequency;
  EFI_PROCESSOR_MAX_CORE_FREQUENCY_DATA   ProcessorMaxCoreFrequency;  
  EFI_PROCESSOR_CORE_FREQUENCY_DATA       ProcessorCoreFrequency;
  EFI_PROCESSOR_STATUS_DATA               ProcessorStatus;  
  EFI_PROCESSOR_SOCKET_TYPE_DATA          ProcessorSocketType;
  EFI_CACHE_ASSOCIATION_DATA              ProcessorL1LinkData;
  EFI_CACHE_ASSOCIATION_DATA              ProcessorL2LinkData;
  EFI_CACHE_ASSOCIATION_DATA              ProcessorL3LinkData;
  EFI_PROCESSOR_SERIAL_NUMBER_DATA        ProcessorSerialNumber;
  EFI_PROCESSOR_ASSET_TAG_DATA            ProcessorAssetTag;
  EFI_PROCESSOR_PART_NUMBER_DATA          ProcessorPartNumber;
  EFI_PROCESSOR_CORE_COUNT_DATA           ProcessorCoreCount;
  EFI_PROCESSOR_ENABLED_CORE_COUNT_DATA   ProcessorEnabledCoreCount;
  EFI_PROCESSOR_THREAD_COUNT_DATA         ProcessorThreadCount;
  EFI_PROCESSOR_CHARACTERISTICS_DATA      ProcessorCharacteristics;
  EFI_PROCESSOR_FAMILY2_DATA              ProcessorFamily2;
} EFI_PROCESSOR_SOCKET_DATA;

typedef enum {
  ProcessorCoreFrequencyRecordType = 1,
  ProcessorFsbFrequencyRecordType = 2,
  ProcessorVersionRecordType = 3,
  ProcessorManufacturerRecordType = 4,
  ProcessorSerialNumberRecordType = 5,
  ProcessorIdRecordType = 6,
  ProcessorTypeRecordType = 7,
  ProcessorFamilyRecordType = 8,
  ProcessorVoltageRecordType = 9,
  ProcessorApicBaseAddressRecordType = 10,
  ProcessorApicIdRecordType = 11,
  ProcessorApicVersionNumberRecordType = 12,
  CpuUcodeRevisionDataRecordType = 13,
  ProcessorStatusRecordType = 14,
  ProcessorSocketTypeRecordType = 15,
  ProcessorSocketNameRecordType = 16,
  CacheAssociationRecordType = 17,
  ProcessorMaxCoreFrequencyRecordType = 18,
  ProcessorAssetTagRecordType = 19,
  ProcessorMaxFsbFrequencyRecordType = 20,
  ProcessorPackageNumberRecordType = 21,
  ProcessorCoreFrequencyListRecordType = 22,
  ProcessorFsbFrequencyListRecordType  = 23,
  ProcessorHealthStatusRecordType  = 24,
  ProcessorCoreCountRecordType = 25,
  ProcessorEnabledCoreCountRecordType = 26,
  ProcessorThreadCountRecordType = 27,
  ProcessorCharacteristicsRecordType = 28,
  ProcessorFamily2RecordType = 29,
  ProcessorPartNumberRecordType = 30,
  ProcessorSocketRecordType = 31
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
  EFI_PROCESSOR_PART_NUMBER_DATA          ProcessorPartNumber;
  EFI_PROCESSOR_HEALTH_STATUS             ProcessorHealthStatus;
  EFI_PROCESSOR_PACKAGE_NUMBER_DATA       ProcessorPackageNumber;
  EFI_PROCESSOR_CORE_COUNT_DATA           ProcessorCoreCount;
  EFI_PROCESSOR_ENABLED_CORE_COUNT_DATA   ProcessorEnabledCoreCount;
  EFI_PROCESSOR_THREAD_COUNT_DATA         ProcessorThreadCount;
  EFI_PROCESSOR_CHARACTERISTICS_DATA      ProcessorCharacteristics;
  EFI_PROCESSOR_FAMILY2_DATA              ProcessorFamily2;
  EFI_PROCESSOR_SOCKET_DATA               ProcessorSocket;
} EFI_CPU_VARIABLE_RECORD;

typedef struct {
  EFI_SUBCLASS_TYPE1_HEADER      DataRecordHeader;
  EFI_CPU_VARIABLE_RECORD        VariableRecord;
} EFI_CPU_DATA_RECORD;

#endif
