/** @file
  This file defines the data structures to support Status Code Data.

  Copyright (c) 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  FrameworkStatusCodeDataTypeId.h

  @par Revision Reference:
  These definitions are from Framework of EFI Status Code Spec
  Version 0.92.

**/

#ifndef __FRAMEWORK_STATUS_CODE_DATA_TYPE_ID_H__
#define __FRAMEWORK_STATUS_CODE_DATA_TYPE_ID_H__

#include <Framework/DataHubRecords.h>
#include <Protocol/DebugSupport.h>

///
/// The size of string
///
#define EFI_STATUS_CODE_DATA_MAX_STRING_SIZE  150

///
/// This is the max data size including all the headers which can be passed
/// as Status Code data. This data should be multiple of 8 byte
/// to avoid any kind of boundary issue. Also, sum of this data size (inclusive
/// of size of EFI_STATUS_CODE_DATA should not exceed the max record size of
/// data hub
///
#define EFI_STATUS_CODE_DATA_MAX_SIZE 200

#pragma pack(1)
typedef enum {
  EfiStringAscii,
  EfiStringUnicode,
  EfiStringToken
} EFI_STRING_TYPE;

typedef struct {
  EFI_HII_HANDLE  Handle;
  STRING_REF      Token;
} EFI_STATUS_CODE_STRING_TOKEN;

typedef union {
  CHAR8                         *Ascii;
  CHAR16                        *Unicode;
  EFI_STATUS_CODE_STRING_TOKEN  Hii;
} EFI_STATUS_CODE_STRING;

typedef struct {
  EFI_STATUS_CODE_DATA                          DataHeader;
  EFI_STRING_TYPE                               StringType;
  EFI_STATUS_CODE_STRING                        String;
} EFI_STATUS_CODE_STRING_DATA;

#pragma pack()

#pragma pack(1)

typedef struct {
  UINT32  ErrorLevel;
  //
  // 12 * sizeof (UINT64) Var Arg stack
  //
  // ascii DEBUG () Format string
  //
} EFI_DEBUG_INFO;

#pragma pack()

//
// declaration for EFI_EXP_DATA. This may change
//
// typedef UINTN   EFI_EXP_DATA;

///
/// Voltage Extended Error Data
///
typedef struct {
  EFI_STATUS_CODE_DATA  DataHeader;
  EFI_EXP_BASE10_DATA   Voltage;
  EFI_EXP_BASE10_DATA   Threshold;
} EFI_COMPUTING_UNIT_VOLTAGE_ERROR_DATA;

typedef struct {
  EFI_EXP_BASE10_DATA   Voltage;
  EFI_EXP_BASE10_DATA   Threshold;
} EFI_COMPUTING_UNIT_VOLTAGE_ERROR_DATA_PAYLOAD;

///
/// Microcode Update Extended Error Data
///
typedef struct {
  EFI_STATUS_CODE_DATA  DataHeader;
  UINT32                Version;
} EFI_COMPUTING_UNIT_MICROCODE_UPDATE_ERROR_DATA;

typedef struct {
  UINT32                Version;
} EFI_COMPUTING_UNIT_MICROCODE_UPDATE_ERROR_DATA_PAYLOAD;

///
/// Asynchronous Timer Extended Error Data
///
typedef struct {
  EFI_STATUS_CODE_DATA  DataHeader;
  EFI_EXP_BASE10_DATA   TimerLimit;
} EFI_COMPUTING_UNIT_TIMER_EXPIRED_ERROR_DATA;

typedef struct {
  EFI_EXP_BASE10_DATA   TimerLimit;
} EFI_COMPUTING_UNIT_TIMER_EXPIRED_ERROR_DATA_PAYLOAD;

///
/// Host Processor Mismatch Extended Error Data
///
typedef struct {
  EFI_STATUS_CODE_DATA  DataHeader;
  UINT32                Instance;
  UINT16                Attributes;
} EFI_HOST_PROCESSOR_MISMATCH_ERROR_DATA;

typedef struct {
  UINT32                Instance;
  UINT16                Attributes;
} EFI_HOST_PROCESSOR_MISMATCH_ERROR_DATA_PAYLOAD;

//
// EFI_COMPUTING_UNIT_MISMATCH_ATTRIBUTES
// All other attributes are reserved for future use and
// must be initialized to 0.
//
#define EFI_COMPUTING_UNIT_MISMATCH_SPEED       0x0001
#define EFI_COMPUTING_UNIT_MISMATCH_FSB_SPEED   0x0002
#define EFI_COMPUTING_UNIT_MISMATCH_FAMILY      0x0004
#define EFI_COMPUTING_UNIT_MISMATCH_MODEL       0x0008
#define EFI_COMPUTING_UNIT_MISMATCH_STEPPING    0x0010
#define EFI_COMPUTING_UNIT_MISMATCH_CACHE_SIZE  0x0020
#define EFI_COMPUTING_UNIT_MISMATCH_OEM1        0x1000
#define EFI_COMPUTING_UNIT_MISMATCH_OEM2        0x2000
#define EFI_COMPUTING_UNIT_MISMATCH_OEM3        0x4000
#define EFI_COMPUTING_UNIT_MISMATCH_OEM4        0x8000

///
/// Thermal Extended Error Data
///
typedef struct {
  EFI_STATUS_CODE_DATA  DataHeader;
  EFI_EXP_BASE10_DATA   Temperature;
  EFI_EXP_BASE10_DATA   Threshold;
} EFI_COMPUTING_UNIT_THERMAL_ERROR_DATA;

typedef struct {
  EFI_EXP_BASE10_DATA   Temperature;
  EFI_EXP_BASE10_DATA   Threshold;
} EFI_COMPUTING_UNIT_THERMAL_ERROR_DATA_PAYLOAD;

///
/// Processor Disabled Extended Error Data
///
typedef struct {
  EFI_STATUS_CODE_DATA  DataHeader;
  UINT32                Cause;
  BOOLEAN               SoftwareDisabled;
} EFI_COMPUTING_UNIT_CPU_DISABLED_ERROR_DATA;

typedef struct {
  UINT32                Cause;
  BOOLEAN               SoftwareDisabled;
} EFI_COMPUTING_UNIT_CPU_DISABLED_ERROR_DATA_PAYLOAD;

typedef enum {
  EfiInitCacheDataOnly,
  EfiInitCacheInstrOnly,
  EfiInitCacheBoth,
  EfiInitCacheUnspecified
} EFI_INIT_CACHE_TYPE;

///
/// Embedded cache init extended data
///
typedef struct {
  EFI_STATUS_CODE_DATA  DataHeader;
  UINT32                Level;
  EFI_INIT_CACHE_TYPE   Type;
} EFI_CACHE_INIT_DATA;

typedef struct {
  UINT32                Level;
  EFI_INIT_CACHE_TYPE   Type;
} EFI_CACHE_INIT_DATA_PAYLOAD;

//
// Memory Extended Error Data
//

///
/// Memory Error Granularity Definition
///
typedef UINT8 EFI_MEMORY_ERROR_GRANULARITY;

///
/// Memory Error Operation Definition
///
typedef UINT8 EFI_MEMORY_ERROR_OPERATION;

typedef struct {
  EFI_STATUS_CODE_DATA          DataHeader;
  EFI_MEMORY_ERROR_GRANULARITY  Granularity;
  EFI_MEMORY_ERROR_OPERATION    Operation;
  UINTN                         Syndrome;
  EFI_PHYSICAL_ADDRESS          Address;
  UINTN                         Resolution;
} EFI_MEMORY_EXTENDED_ERROR_DATA;

typedef struct {
  EFI_MEMORY_ERROR_GRANULARITY  Granularity;
  EFI_MEMORY_ERROR_OPERATION    Operation;
  UINTN                         Syndrome;
  EFI_PHYSICAL_ADDRESS          Address;
  UINTN                         Resolution;
} EFI_MEMORY_EXTENDED_ERROR_DATA_PAYLOAD;

//
// Memory Error Granularities
//
#define EFI_MEMORY_ERROR_OTHER      0x01
#define EFI_MEMORY_ERROR_UNKNOWN    0x02
#define EFI_MEMORY_ERROR_DEVICE     0x03
#define EFI_MEMORY_ERROR_PARTITION  0x04

//
// Memory Error Operations
//
#define EFI_MEMORY_OPERATION_OTHER          0x01
#define EFI_MEMORY_OPERATION_UNKNOWN        0x02
#define EFI_MEMORY_OPERATION_READ           0x03
#define EFI_MEMORY_OPERATION_WRITE          0x04
#define EFI_MEMORY_OPERATION_PARTIAL_WRITE  0x05

//
// Define shorthands to describe Group Operations
// Many memory init operations are essentially group
// operations.

/// A shorthand to describe that the operation is performed
/// on multiple devices within the array
///
#define EFI_MULTIPLE_MEMORY_DEVICE_OPERATION  0xfffe
///
/// A shorthand to describe that the operation is performed on all devices within the array
///
#define EFI_ALL_MEMORY_DEVICE_OPERATION 0xffff
///
/// A shorthand to describe that the operation is performed on multiple arrays
///
#define EFI_MULTIPLE_MEMORY_ARRAY_OPERATION 0xfffe
///
/// A shorthand to describe that the operation is performed on all the arrays
///
#define EFI_ALL_MEMORY_ARRAY_OPERATION  0xffff

//
// DIMM number
//
#pragma pack(1)
typedef struct {
  EFI_STATUS_CODE_DATA  DataHeader;
  UINT16                Array;
  UINT16                Device;
} EFI_STATUS_CODE_DIMM_NUMBER;

typedef struct {
  UINT16                Array;
  UINT16                Device;
} EFI_STATUS_CODE_DIMM_NUMBER_PAYLOAD;
#pragma pack()

///
/// Memory Module Mismatch Extended Error Data
///
typedef struct {
  EFI_STATUS_CODE_DATA        DataHeader;
  EFI_STATUS_CODE_DIMM_NUMBER Instance;
} EFI_MEMORY_MODULE_MISMATCH_ERROR_DATA;

typedef struct {
  EFI_STATUS_CODE_DIMM_NUMBER Instance;
} EFI_MEMORY_MODULE_MISMATCH_ERROR_DATA_PAYLOAD;

///
/// Memory Range Extended Data
///
typedef struct {
  EFI_STATUS_CODE_DATA  DataHeader;
  EFI_PHYSICAL_ADDRESS  Start;
  EFI_PHYSICAL_ADDRESS  Length;
} EFI_MEMORY_RANGE_EXTENDED_DATA;

typedef struct {
  EFI_PHYSICAL_ADDRESS  Start;
  EFI_PHYSICAL_ADDRESS  Length;
} EFI_MEMORY_RANGE_EXTENDED_DATA_PAYLOAD;

///
/// Device handle Extended Data. Used for many
/// errors and progress codes to point to the device.
///
typedef struct {
  EFI_STATUS_CODE_DATA  DataHeader;
  EFI_HANDLE            Handle;
} EFI_DEVICE_HANDLE_EXTENDED_DATA;

typedef struct {
  EFI_STATUS_CODE_DATA                 DataHeader;
  UINT8                                *DevicePath;
} EFI_DEVICE_PATH_EXTENDED_DATA;

typedef struct {
  EFI_STATUS_CODE_DATA           DataHeader;
  EFI_HANDLE                     ControllerHandle;
  EFI_HANDLE                     DriverBindingHandle;
  UINT16                         DevicePathSize;
  UINT8                          *RemainingDevicePath;
} EFI_STATUS_CODE_START_EXTENDED_DATA;

typedef struct {
  EFI_HANDLE            Handle;
} EFI_DEVICE_HANDLE_EXTENDED_DATA_PAYLOAD;

typedef struct {
  UINT8                                *DevicePath;
} EFI_DEVICE_PATH_EXTENDED_DATA_PAYLOAD;

typedef struct {
  EFI_HANDLE                     ControllerHandle;
  EFI_HANDLE                     DriverBindingHandle;
  UINT16                         DevicePathSize;
  UINT8                          *RemainingDevicePath;
} EFI_STATUS_CODE_START_EXTENDED_DATA_PAYLOAD;

///
/// Resource Allocation Failure Extended Error Data
///

/*
typedef struct {
  EFI_STATUS_CODE_DATA      DataHeader;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINT32                    Bar;
  VOID                      *ReqRes;
  VOID                      *AllocRes;
} EFI_RESOURCE_ALLOC_FAILURE_ERROR_DATA;
*/
typedef struct {
  EFI_STATUS_CODE_DATA               DataHeader;
  UINT32                             Bar;
  UINT16                             DevicePathSize;
  UINT16                             ReqResSize;
  UINT16                             AllocResSize;
  UINT8                              *DevicePath;
  UINT8                              *ReqRes;
  UINT8                              *AllocRes;
} EFI_RESOURCE_ALLOC_FAILURE_ERROR_DATA;

typedef struct {
  UINT32                             Bar;
  UINT16                             DevicePathSize;
  UINT16                             ReqResSize;
  UINT16                             AllocResSize;
  UINT8                              *DevicePath;
  UINT8                              *ReqRes;
  UINT8                              *AllocRes;
} EFI_RESOURCE_ALLOC_FAILURE_ERROR_DATA_PAYLOAD;

///
/// Extended Error Data for Assert
///
typedef struct {
  EFI_STATUS_CODE_DATA        DataHeader;
  UINT32                      LineNumber;
  UINT32                      FileNameSize;
  EFI_STATUS_CODE_STRING_DATA *FileName;
} EFI_DEBUG_ASSERT_DATA;

typedef struct {
  UINT32                      LineNumber;
  UINT32                      FileNameSize;
  EFI_STATUS_CODE_STRING_DATA *FileName;
} EFI_DEBUG_ASSERT_DATA_PAYLOAD;

///
/// System Context Data EBC/IA32/IPF
///
typedef union {
  EFI_SYSTEM_CONTEXT_EBC  SystemContextEbc;
  EFI_SYSTEM_CONTEXT_IA32 SystemContextIa32;
  EFI_SYSTEM_CONTEXT_IPF  SystemContextIpf;
} EFI_STATUS_CODE_EXCEP_SYSTEM_CONTEXT;

typedef struct {
  EFI_STATUS_CODE_DATA                  DataHeader;
  EFI_STATUS_CODE_EXCEP_SYSTEM_CONTEXT  Context;
} EFI_STATUS_CODE_EXCEP_EXTENDED_DATA;

typedef struct {
  EFI_STATUS_CODE_EXCEP_SYSTEM_CONTEXT  Context;
} EFI_STATUS_CODE_EXCEP_EXTENDED_DATA_PAYLOAD;

///
/// Legacy Oprom extended data
///
typedef struct {
  EFI_STATUS_CODE_DATA  DataHeader;
  EFI_HANDLE            DeviceHandle;
  EFI_PHYSICAL_ADDRESS  RomImageBase;
} EFI_LEGACY_OPROM_EXTENDED_DATA;

typedef struct {
  EFI_HANDLE            DeviceHandle;
  EFI_PHYSICAL_ADDRESS  RomImageBase;
} EFI_LEGACY_OPROM_EXTENDED_DATA_PAYLOAD;

#endif
