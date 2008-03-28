/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DataHubSubClassCache.h
    
Abstract:

  Definitions for Cache sub class data records

Revision History

--*/

#ifndef _DATAHUB_SUBCLASS_CACHE_H_
#define _DATAHUB_SUBCLASS_CACHE_H_

#define EFI_CACHE_SUBCLASS_VERSION    0x00010000

#define EFI_CACHE_SUBCLASS_GUID \
{ 0x7f0013a7, 0xdc79, 0x4b22, {0x80, 0x99, 0x11, 0xf7, 0x5f, 0xdc, 0x82, 0x9d} }

typedef EFI_EXP_BASE2_DATA  EFI_CACHE_SIZE_DATA;

typedef EFI_EXP_BASE2_DATA  EFI_MAXIMUM_CACHE_SIZE_DATA;

typedef EFI_EXP_BASE10_DATA EFI_CACHE_SPEED_DATA;

typedef STRING_REF  EFI_CACHE_SOCKET_DATA;

typedef struct {
  UINT32  Other     :1;
  UINT32  Unknown   :1;
  UINT32  NonBurst    :1;
  UINT32  Burst     :1;
  UINT32  PipelineBurst :1;
  UINT32  Asynchronous  :1;
  UINT32  Synchronous :1;
  UINT32  Reserved    :25;
} EFI_CACHE_SRAM_TYPE_DATA;

typedef enum {  
  EfiCacheErrorOther = 1,
  EfiCacheErrorUnknown = 2,
  EfiCacheErrorNone = 3,
  EfiCacheErrorParity = 4,
  EfiCacheErrorSingleBit = 5,
  EfiCacheErrorMultiBit = 6
} EFI_CACHE_ERROR_TYPE_DATA;

typedef enum {  
  EfiCacheTypeOther = 1,
  EfiCacheTypeUnknown = 2,
  EfiCacheTypeInstruction = 3,
  EfiCacheTypeData = 4,
  EfiCacheTypeUnified = 5
} EFI_CACHE_TYPE_DATA;

typedef enum {  
  EfiCacheAssociativityOther = 1,
  EfiCacheAssociativityUnknown = 2,
  EfiCacheAssociativityDirectMapped = 3,
  EfiCacheAssociativity2Way = 4,
  EfiCacheAssociativity4Way = 5,
  EfiCacheAssociativityFully = 6,
  EfiCacheAssociativity8Way = 7,
  EfiCacheAssociativity16Way = 8,
  EfiCacheAssociativity24Way = 9
} EFI_CACHE_ASSOCIATIVITY_DATA;

typedef struct {  
  UINT32    Level       :3;
  UINT32    Socketed    :1;
  UINT32    Reserved2   :1;
  UINT32    Location    :2;
  UINT32    Enable      :1;
  UINT32    OperationalMode :2;
  UINT32    Reserved1   :22;
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
  CacheSizeRecordType = 1,
  MaximumSizeCacheRecordType = 2,
  CacheSpeedRecordType = 3,
  CacheSocketRecordType = 4,
  CacheSramTypeRecordType = 5,
  CacheInstalledSramTypeRecordType = 6,
  CacheErrorTypeRecordType = 7,
  CacheTypeRecordType = 8,
  CacheAssociativityRecordType = 9,
  CacheConfigRecordType = 10
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


#endif
