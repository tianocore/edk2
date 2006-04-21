/** @file
  This file defines the data structures to support Status Code Data.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  StatusCodeDataTypeId.h

  @par Revision Reference:
  These definitions are from Framework of EFI Status Code Spec
  Version 0.92.

**/

#ifndef __STATUS_CODE_DATA_TYPE_ID_H__
#define __STATUS_CODE_DATA_TYPE_ID_H__


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

///
/// Microcode Update Extended Error Data
///
typedef struct {
  EFI_STATUS_CODE_DATA  DataHeader;
  UINT32                Version;
} EFI_COMPUTING_UNIT_MICROCODE_UPDATE_ERROR_DATA;

///
/// Asynchronous Timer Extended Error Data
///
typedef struct {
  EFI_STATUS_CODE_DATA  DataHeader;
  EFI_EXP_BASE10_DATA   TimerLimit;
} EFI_COMPUTING_UNIT_TIMER_EXPIRED_ERROR_DATA;

///
/// Host Processor Mismatch Extended Error Data
///
typedef struct {
  EFI_STATUS_CODE_DATA  DataHeader;
  UINT32                Instance;
  UINT16                Attributes;
} EFI_HOST_PROCESSOR_MISMATCH_ERROR_DATA;

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

///
/// Processor Disabled Extended Error Data
///
typedef struct {
  EFI_STATUS_CODE_DATA  DataHeader;
  UINT32                Cause;
  BOOLEAN               SoftwareDisabled;
} EFI_COMPUTING_UNIT_CPU_DISABLED_ERROR_DATA;

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
#pragma pack()

///
/// Memory Module Mismatch Extended Error Data
///
typedef struct {
  EFI_STATUS_CODE_DATA        DataHeader;
  EFI_STATUS_CODE_DIMM_NUMBER Instance;
} EFI_MEMORY_MODULE_MISMATCH_ERROR_DATA;

///
/// Memory Range Extended Data
///
typedef struct {
  EFI_STATUS_CODE_DATA  DataHeader;
  EFI_PHYSICAL_ADDRESS  Start;
  EFI_PHYSICAL_ADDRESS  Length;
} EFI_MEMORY_RANGE_EXTENDED_DATA;

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

///
/// Extended Error Data for Assert
///
typedef struct {
  EFI_STATUS_CODE_DATA        DataHeader;
  UINT32                      LineNumber;
  UINT32                      FileNameSize;
  EFI_STATUS_CODE_STRING_DATA *FileName;
} EFI_DEBUG_ASSERT_DATA;

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

///
/// Legacy Oprom extended data
///
typedef struct {
  EFI_STATUS_CODE_DATA  DataHeader;
  EFI_HANDLE            DeviceHandle;
  EFI_PHYSICAL_ADDRESS  RomImageBase;
} EFI_LEGACY_OPROM_EXTENDED_DATA;

#endif
