/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DataHubSubClassMemory.h
    
Abstract:

  Definitions for memory sub class data records

Revision History

--*/

#ifndef _DATAHUB_SUBCLASS_MEMORY_H_
#define _DATAHUB_SUBCLASS_MEMORY_H_

#include EFI_GUID_DEFINITION (DataHubRecords)


#define EFI_MEMORY_SUBCLASS_GUID \
  {0x4E8F4EBB, 0x64B9, 0x4e05, {0x9B, 0x18, 0x4C, 0xFE, 0x49, 0x23, 0x50, 0x97}}
  
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
  EfiMemoryArrayUseCacheMemory                = 0x07
} EFI_MEMORY_ARRAY_USE;

typedef enum _EFI_MEMORY_ERROR_CORRECTION {
  EfiMemoryErrorCorrectionOther               = 0x01,
  EfiMemoryErrorCorrectionUnknown             = 0x02,
  EfiMemoryErrorCorrectionNone                = 0x03,
  EfiMemoryErrorCorrectionParity              = 0x04,
  EfiMemoryErrorCorrectionSingleBitEcc        = 0x05,
  EfiMemoryErrorCorrectionMultiBitEcc         = 0x06,
  EfiMemoryErrorCorrectionCrc                 = 0x07
} EFI_MEMORY_ERROR_CORRECTION;

typedef struct {
  EFI_MEMORY_ARRAY_LOCATION   MemoryArrayLocation;
  EFI_MEMORY_ARRAY_USE        MemoryArrayUse;
  EFI_MEMORY_ERROR_CORRECTION MemoryErrorCorrection;
  UINT32                      MaximumMemoryCapacity;
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
  EfiMemoryFormFactorSrimm                    = 0x0E,
  EfiMemoryFormFactorFbDimm                   = 0x0F
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
  EfiMemoryTypeDdr2                           = 0x13,
  EfiMemoryTypeDdr2FbDimm                     = 0x14,
  EfiMemoryTypeDdr3                           = 0x18,
  EfiMemoryTypeFbd2                           = 0x19
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
  EfiMemoryStateEnabled =0,
  EfiMemoryStateUnknown,
  EfiMemoryStateUnsupported,
  EfiMemoryStateError,
  EfiMemoryStateAbsent,
  EfiMemoryStateDisabled,
  EfiMemoryStatePartial
} EFI_MEMORY_STATE;

typedef struct {
  EFI_STRING_TOKEN            MemoryDeviceLocator;
  EFI_STRING_TOKEN            MemoryBankLocator;
  EFI_STRING_TOKEN            MemoryManufacturer;
  EFI_STRING_TOKEN            MemorySerialNumber;
  EFI_STRING_TOKEN            MemoryAssetTag;
  EFI_STRING_TOKEN            MemoryPartNumber;
  EFI_INTER_LINK_DATA         MemoryArrayLink;
  EFI_INTER_LINK_DATA         MemorySubArrayLink;
  UINT16                      MemoryTotalWidth;
  UINT16                      MemoryDataWidth;
  UINT64                      MemoryDeviceSize;
  EFI_MEMORY_FORM_FACTOR      MemoryFormFactor;
  UINT8                       MemoryDeviceSet;
  EFI_MEMORY_ARRAY_TYPE       MemoryType;
  EFI_MEMORY_TYPE_DETAIL      MemoryTypeDetail;
  UINT16                      MemorySpeed;
  EFI_MEMORY_STATE            MemoryState;
  UINT8                       MemoryAttributes;
  UINT8                       MemoryBankConnections;
  UINT8                       MemoryErrorStatus;
} EFI_MEMORY_ARRAY_LINK;


#define EFI_MEMORY_ARRAY_START_ADDRESS_RECORD_NUMBER  0x00000004

typedef struct {
  EFI_PHYSICAL_ADDRESS        MemoryArrayStartAddress;
  EFI_PHYSICAL_ADDRESS        MemoryArrayEndAddress;
  EFI_INTER_LINK_DATA         PhysicalMemoryArrayLink;
  UINT16                      MemoryArrayPartitionWidth;
} EFI_MEMORY_ARRAY_START_ADDRESS;


#define EFI_MEMORY_DEVICE_START_ADDRESS_RECORD_NUMBER 0x00000005

typedef struct {
  EFI_PHYSICAL_ADDRESS        MemoryDeviceStartAddress;
  EFI_PHYSICAL_ADDRESS        MemoryDeviceEndAddress;
  EFI_INTER_LINK_DATA         PhysicalMemoryDeviceLink;
  EFI_INTER_LINK_DATA         PhysicalMemoryArrayLink;
  UINT8                       MemoryDevicePartitionRowPosition;
  UINT8                       MemoryDeviceInterleavePosition;
  UINT8                       MemoryDeviceInterleaveDataDepth;
} EFI_MEMORY_DEVICE_START_ADDRESS;


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

//
//  Memory. Controller Information - SMBIOS Type 5
//
#define EFI_MEMORY_CONTROLLER_INFORMATION_RECORD_NUMBER 0x00000008

typedef enum {  
  EfiErrorDetectingMethodOther   = 1,
  EfiErrorDetectingMethodUnknown = 2,
  EfiErrorDetectingMethodNone    = 3,
  EfiErrorDetectingMethodParity  = 4,
  EfiErrorDetectingMethod32Ecc   = 5,
  EfiErrorDetectingMethod64Ecc   = 6,
  EfiErrorDetectingMethod128Ecc  = 7,
  EfiErrorDetectingMethodCrc     = 8
} EFI_MEMORY_ERROR_DETECT_METHOD_TYPE;

typedef struct {
  UINT8  Other                 :1;
  UINT8  Unknown               :1;
  UINT8  None                  :1;
  UINT8  SingleBitErrorCorrect :1;
  UINT8  DoubleBitErrorCorrect :1;
  UINT8  ErrorScrubbing        :1;
  UINT8  Reserved              :2;
} EFI_MEMORY_ERROR_CORRECT_CAPABILITY;

typedef enum {  
  EfiMemoryInterleaveOther      = 1,
  EfiMemoryInterleaveUnknown    = 2,
  EfiMemoryInterleaveOneWay     = 3,
  EfiMemoryInterleaveTwoWay     = 4,
  EfiMemoryInterleaveFourWay    = 5,
  EfiMemoryInterleaveEightWay   = 6,
  EfiMemoryInterleaveSixteenWay = 7
} EFI_MEMORY_SUPPORT_INTERLEAVE_TYPE;

typedef struct {
  UINT16  Other                 :1;
  UINT16  Unknown               :1;
  UINT16  SeventyNs             :1;
  UINT16  SixtyNs               :1;
  UINT16  FiftyNs               :1;
  UINT16  Reserved              :11;
} EFI_MEMORY_SPEED_TYPE;

typedef struct {
  UINT16  Other                 :1;
  UINT16  Unknown               :1;
  UINT16  Standard              :1;
  UINT16  FastPageMode          :1;
  UINT16  EDO                   :1;
  UINT16  Parity                :1;
  UINT16  ECC                   :1;
  UINT16  SIMM                  :1;
  UINT16  DIMM                  :1;
  UINT16  BurstEdo              :1;
  UINT16  SDRAM                 :1;
  UINT16  Reserved              :5;
} EFI_MEMORY_SUPPORTED_TYPE;

typedef struct {
  UINT8  Five                  :1;
  UINT8  Three                 :1;
  UINT8  Two                   :1;
  UINT8  Reserved              :5;
} EFI_MEMORY_MODULE_VOLTAGE_TYPE;

//
//  EFI_MEMORY_CONTROLLER_INFORMATION is obsolete
//  Use EFI_MEMORY_CONTROLLER_INFORMATION_DATA instead
//
typedef struct {
  EFI_MEMORY_ERROR_DETECT_METHOD_TYPE   ErrorDetectingMethod;
  EFI_MEMORY_ERROR_CORRECT_CAPABILITY   ErrorCorrectingCapability;
  EFI_MEMORY_SUPPORT_INTERLEAVE_TYPE    MemorySupportedInterleave;
  EFI_MEMORY_SUPPORT_INTERLEAVE_TYPE    MemoryCurrentInterleave;
  UINT8                                 MaxMemoryModuleSize;
  EFI_MEMORY_SPEED_TYPE                 MemorySpeedType;
  EFI_MEMORY_SUPPORTED_TYPE             MemorySupportedType;
  EFI_MEMORY_MODULE_VOLTAGE_TYPE        MemoryModuleVoltage;
  UINT8                                 NumberofMemorySlot;
  EFI_MEMORY_ERROR_CORRECT_CAPABILITY   EnabledCorrectingCapability;
  UINT16                                *MemoryModuleConfigHandles;
} EFI_MEMORY_CONTROLLER_INFORMATION;

typedef struct {
  EFI_MEMORY_ERROR_DETECT_METHOD_TYPE   ErrorDetectingMethod;
  EFI_MEMORY_ERROR_CORRECT_CAPABILITY   ErrorCorrectingCapability;
  EFI_MEMORY_SUPPORT_INTERLEAVE_TYPE    MemorySupportedInterleave;
  EFI_MEMORY_SUPPORT_INTERLEAVE_TYPE    MemoryCurrentInterleave;
  UINT8                                 MaxMemoryModuleSize;
  EFI_MEMORY_SPEED_TYPE                 MemorySpeedType;
  EFI_MEMORY_SUPPORTED_TYPE             MemorySupportedType;
  EFI_MEMORY_MODULE_VOLTAGE_TYPE        MemoryModuleVoltage;
  UINT8                                 NumberofMemorySlot;
  EFI_MEMORY_ERROR_CORRECT_CAPABILITY   EnabledCorrectingCapability;
  EFI_INTER_LINK_DATA                   MemoryModuleConfig[1];
} EFI_MEMORY_CONTROLLER_INFORMATION_DATA;

//
//  Memory. Error Information - SMBIOS Type 18
//
#define EFI_MEMORY_32BIT_ERROR_INFORMATION_RECORD_NUMBER 0x00000009

typedef enum {  
  EfiMemoryErrorOther             = 1,
  EfiMemoryErrorUnknown           = 2,
  EfiMemoryErrorOk                = 3,
  EfiMemoryErrorBadRead           = 4,
  EfiMemoryErrorParity            = 5,
  EfiMemoryErrorSigleBit          = 6,
  EfiMemoryErrorDoubleBit         = 7,
  EfiMemoryErrorMultiBit          = 8,
  EfiMemoryErrorNibble            = 9,
  EfiMemoryErrorChecksum          = 10,
  EfiMemoryErrorCrc               = 11,
  EfiMemoryErrorCorrectSingleBit  = 12,
  EfiMemoryErrorCorrected         = 13,
  EfiMemoryErrorUnCorrectable     = 14
} EFI_MEMORY_ERROR_TYPE;

typedef enum {  
  EfiMemoryGranularityOther               = 1,
  EfiMemoryGranularityOtherUnknown        = 2,
  EfiMemoryGranularityDeviceLevel         = 3,
  EfiMemoryGranularityMemPartitionLevel   = 4
} EFI_MEMORY_ERROR_GRANULARITY_TYPE;

typedef enum {  
  EfiMemoryErrorOperationOther            = 1,
  EfiMemoryErrorOperationUnknown          = 2,
  EfiMemoryErrorOperationRead             = 3,
  EfiMemoryErrorOperationWrite            = 4,
  EfiMemoryErrorOperationPartialWrite     = 5
} EFI_MEMORY_ERROR_OPERATION_TYPE;

typedef struct {
  EFI_MEMORY_ERROR_TYPE               MemoryErrorType;
  EFI_MEMORY_ERROR_GRANULARITY_TYPE   MemoryErrorGranularity;
  EFI_MEMORY_ERROR_OPERATION_TYPE     MemoryErrorOperation;
  UINT32                              VendorSyndrome;
  UINT32                              MemoryArrayErrorAddress;
  UINT32                              DeviceErrorAddress;
  UINT32                              DeviceErrorResolution;
} EFI_MEMORY_32BIT_ERROR_INFORMATION;

//
//  Memory. Error Information - SMBIOS Type 33
//
#define EFI_MEMORY_64BIT_ERROR_INFORMATION_RECORD_NUMBER 0x0000000A

typedef struct {
  EFI_MEMORY_ERROR_TYPE               MemoryErrorType;
  EFI_MEMORY_ERROR_GRANULARITY_TYPE   MemoryErrorGranularity;
  EFI_MEMORY_ERROR_OPERATION_TYPE     MemoryErrorOperation;
  UINT32                              VendorSyndrome;
  UINT64                              MemoryArrayErrorAddress;
  UINT64                              DeviceErrorAddress;
  UINT32                              DeviceErrorResolution;
} EFI_MEMORY_64BIT_ERROR_INFORMATION;


typedef union _EFI_MEMORY_SUBCLASS_RECORDS {
  EFI_MEMORY_SIZE_DATA                  SizeData;
  EFI_MEMORY_ARRAY_LOCATION_DATA        ArrayLocationData;
  EFI_MEMORY_ARRAY_LINK                 ArrayLink;
  EFI_MEMORY_ARRAY_START_ADDRESS        ArrayStartAddress;
  EFI_MEMORY_DEVICE_START_ADDRESS       DeviceStartAddress;
  EFI_MEMORY_CHANNEL_TYPE_DATA          ChannelTypeData;
  EFI_MEMORY_CHANNEL_DEVICE_DATA        ChannelDeviceData;
  EFI_MEMORY_CONTROLLER_INFORMATION     MemoryControllerInfo;
  EFI_MEMORY_32BIT_ERROR_INFORMATION    Memory32bitErrorInfo;
  EFI_MEMORY_64BIT_ERROR_INFORMATION    Memory64bitErrorInfo;
} EFI_MEMORY_SUBCLASS_RECORDS;

typedef struct {
  EFI_SUBCLASS_TYPE1_HEADER             Header;
  EFI_MEMORY_SUBCLASS_RECORDS           Record;
} EFI_MEMORY_SUBCLASS_DRIVER_DATA;



#endif
