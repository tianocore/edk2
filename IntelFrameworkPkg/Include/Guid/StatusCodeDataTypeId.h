/** @file
  GUID used to identify id for the caller who is initiating the Status Code.

  Copyright (c) 2006, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  StatusCodeDataTypeId.h

  @par Revision Reference:
  GUIDs defined in Status Codes Specification 0.92

**/

#ifndef __STATUS_CODE_DATA_TYPE_ID_GUID_H__
#define __STATUS_CODE_DATA_TYPE_ID_GUID_H__

#include <PiPei.h>
#include <Framework/StatusCode.h>
#include <Framework/DataHubRecords.h>
#include <Protocol/DebugSupport.h>
#include <Protocol/FrameworkHii.h>

//////////////////////////////////////////////////////////////////////////////////////////
// String Data Type defintion. This is part of Status Code Specification
//////////////////////////////////////////////////////////////////////////////////////////
#define EFI_STATUS_CODE_DATA_TYPE_STRING_GUID \
  { 0x92D11080, 0x496F, 0x4D95, { 0xBE, 0x7E, 0x03, 0x74, 0x88, 0x38, 0x2B, 0x0A } }

#pragma pack(1)

typedef enum {
  EfiStringAscii,
  EfiStringUnicode,
  EfiStringToken
} EFI_STRING_TYPE;

typedef struct {
  FRAMEWORK_EFI_HII_HANDLE  Handle;
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

extern EFI_GUID gEfiStatusCodeDataTypeStringGuid;


//////////////////////////////////////////////////////////////////////////////////////////
// Special Data Type defintion. This is part of Status Code Specification
//////////////////////////////////////////////////////////////////////////////////////////
#define EFI_STATUS_CODE_SPECIFIC_DATA_GUID \
  { 0x335984bd, 0xe805, 0x409a, { 0xb8, 0xf8, 0xd2, 0x7e, 0xce, 0x5f, 0xf7, 0xa6 } }

#pragma pack(1)

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
/// This structure provides the voltage at the time of error. It also provides 
/// the threshold value indicating the minimum or maximum voltage that is considered 
/// an error. If the voltage is less then the threshold, the error indicates that the 
/// voltage fell below the minimum acceptable value. If the voltage is greater then the threshold, 
/// the error indicates that the voltage rose above the maximum acceptable value.
///
typedef struct {
  ///
  /// The data header identifying the data.  
  ///
  EFI_STATUS_CODE_DATA  DataHeader;
  ///
  /// The voltage value at the time of the error.
  ///
  EFI_EXP_BASE10_DATA   Voltage;
  ///
  /// The voltage threshold.
  ///
  EFI_EXP_BASE10_DATA   Threshold;
} EFI_COMPUTING_UNIT_VOLTAGE_ERROR_DATA;

///
/// Microcode Update Extended Error Data
///
typedef struct {
  ///
  /// The data header identifying the data. 
  ///
  EFI_STATUS_CODE_DATA  DataHeader;
  ///
  /// The version of the microcode update from the header.
  ///
  UINT32                Version;
} EFI_COMPUTING_UNIT_MICROCODE_UPDATE_ERROR_DATA;

///
/// Asynchronous Timer Extended Error Data
/// The timer limit provides the timeout value of the timer prior to expiration.
///
typedef struct {
  ///
  /// The data header identifying the data. 
  ///
  EFI_STATUS_CODE_DATA  DataHeader;
  ///
  /// The number of seconds that the computing unit timer was configured to expire.
  ///
  EFI_EXP_BASE10_DATA   TimerLimit;
} EFI_COMPUTING_UNIT_TIMER_EXPIRED_ERROR_DATA;

///
/// Host Processor Mismatch Extended Error Data
/// This provides information to indicate which processors mismatch, and how they mismatch. The 
/// status code contains the instance number of the processor that is in error. This structure's 
/// Instance indicates the second processor that does not match. This differentiation allows the 
/// consumer to determine which two processors do not match. The Attributes indicate what 
/// mismatch is being reported. Because Attributes is a bit field, more than one mismatch can be 
/// reported with one error code.
///
typedef struct {
  ///
  /// The data header identifying the data. 
  ///
  EFI_STATUS_CODE_DATA  DataHeader;
  ///
  /// The unit number of the computing unit that does not match.
  ///  
  UINT32                Instance;
  /// 
  /// The attributes describing the failure. 
  ///  
  UINT16                Attributes;
} EFI_HOST_PROCESSOR_MISMATCH_ERROR_DATA;

///
/// Thermal Extended Error Data
/// This structure provides the temperature at the time of error. It also provides the threshold value 
/// indicating the minimum temperature that is considered an error.
///
typedef struct {
  ///
  /// The data header identifying the data. 
  ///
  EFI_STATUS_CODE_DATA  DataHeader;
  ///
  /// The thermal value at the time of the error.
  ///
  EFI_EXP_BASE10_DATA   Temperature;
  ///
  /// The thermal threshold.
  ///
  EFI_EXP_BASE10_DATA   Threshold;
} EFI_COMPUTING_UNIT_THERMAL_ERROR_DATA;

//
// Valid cache types
//
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

///
/// Processor Disabled Extended Error Data
/// This structure provides details as to why and how the computing unit was disabled. The causes 
/// should cover the typical reasons a processor would be disabled. How the processor was disabled is 
/// important because there are distinct differences between hardware and software disabling.
///
typedef struct {
  ///
  /// The data header identifying the data. 
  ///
  EFI_STATUS_CODE_DATA  DataHeader;
  ///
  /// The reason for disabling the processor. 
  /// 
  UINT32                Cause;
  ///
  /// TRUE if the processor is disabled via software means such as not listing it in the ACPI tables. 
  /// Such a processor will respond to Interprocessor Interrupts (IPIs). FALSE if the processor is hardware 
  /// disabled, which means it is invisible to software and will not respond to IPIs.
  ///
  BOOLEAN               SoftwareDisabled;
} EFI_COMPUTING_UNIT_CPU_DISABLED_ERROR_DATA;

///
/// Memory Error Operation Definition
///
typedef UINT8 EFI_MEMORY_ERROR_OPERATION;

///
/// Memory Error Granularity Definition
///
typedef UINT8 EFI_MEMORY_ERROR_GRANULARITY;

///
/// This structure provides specific details about the memory error that was detected. It provides 
/// enough information so that consumers can identify the exact failure and provides enough 
/// information to enable corrective action if necessary.
///
typedef struct {
  ///
  /// The data header identifying the data. 
  ///
  EFI_STATUS_CODE_DATA          DataHeader;
  ///
  /// The error granularity type.
  ///
  EFI_MEMORY_ERROR_GRANULARITY  Granularity;
  ///
  /// The operation that resulted in the error being detected. 
  ///
  EFI_MEMORY_ERROR_OPERATION    Operation;
  ///
  /// The error syndrome, vendor-specific ECC syndrome, or CRC data associated with 
  /// the error.  If unknown, should be initialized to 0.
  ///
  UINTN                         Syndrome;
  ///
  /// The physical address of the error. 
  ///
  EFI_PHYSICAL_ADDRESS          Address;
  ///
  /// The range, in bytes, within which the error address can be determined.
  ///
  UINTN                         Resolution;
} EFI_MEMORY_EXTENDED_ERROR_DATA;

///
/// This extended data provides some context that consumers can use to locate a DIMM within the 
/// overall memory scheme.  
///
typedef struct {
  ///
  /// The data header identifying the data. 
  ///
  EFI_STATUS_CODE_DATA  DataHeader;
  ///
  /// The memory array number.
  ///
  UINT16                Array;
  ///
  /// The device number within that Array.
  ///
  UINT16                Device;
} EFI_STATUS_CODE_DIMM_NUMBER;

///
/// Memory Module Mismatch Extended Error Data
/// 
typedef struct {
  ///
  /// The data header identifying the data.
  ///
  EFI_STATUS_CODE_DATA        DataHeader;
  ///
  /// The instance number of the memory module that does not match. 
  ///
  EFI_STATUS_CODE_DIMM_NUMBER Instance;
} EFI_MEMORY_MODULE_MISMATCH_ERROR_DATA;

///
/// Memory Range Extended Data
/// This extended data may be used to convey the specifics of a memory range.  Ranges are specified 
/// with a start address and a length.
///
typedef struct {
  ///
  /// The data header identifying the data. 
  ///
  EFI_STATUS_CODE_DATA  DataHeader;
  ///
  /// The starting address of the memory range. 
  ///
  EFI_PHYSICAL_ADDRESS  Start;
  ///
  /// The length in bytes of the memory range.
  ///
  EFI_PHYSICAL_ADDRESS  Length;
} EFI_MEMORY_RANGE_EXTENDED_DATA;

///
/// Extended Error Data for Assert
/// The data indicates the location of the assertion that failed in the source code.  This information 
/// includes the file name and line number that are necessary to find the failing assertion in source code.
///
typedef struct {
  ///
  /// The data header identifying the data.
  /// 
  EFI_STATUS_CODE_DATA        DataHeader;
  ///
  /// The line number of the source file where the fault was generated.
  ///
  UINT32                      LineNumber;
  ///
  /// The size in bytes of FileName.
  ///
  UINT32                      FileNameSize;
  ///
  /// A pointer to a NULL-terminated ASCII or Unicode string that represents the file 
  /// name of the source file where the fault was generated. 
  ///
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

///
/// This extended data allows the processor context that is present at the time of the exception to be 
/// reported with the exception. The format and contents of the context data varies depending on the 
/// processor architecture. 
///
typedef struct {
  ///
  /// The data header identifying the data.  
  ///
  EFI_STATUS_CODE_DATA                  DataHeader;
  ///
  /// The system context. 
  ///
  EFI_STATUS_CODE_EXCEP_SYSTEM_CONTEXT  Context;
} EFI_STATUS_CODE_EXCEP_EXTENDED_DATA;

///
/// This extended data records information about a Start() function call. Start() is a member of 
/// the EFI 1.10 Driver Binding Protocol.
///
typedef struct {
  /// 
  /// The data header identifying the data.  
  ///
  EFI_STATUS_CODE_DATA           DataHeader;
  ///
  /// The controller handle. 
  ///
  EFI_HANDLE                     ControllerHandle;
  ///
  /// The driver binding handle.
  ///
  EFI_HANDLE                     DriverBindingHandle;
  /// 
  /// The size of the RemainingDevicePath. It is zero if the Start() function is 
  /// called with RemainingDevicePath = NULL. 
  ///
  UINT16                         DevicePathSize;
  ///
  /// Matches the RemainingDevicePath parameter being passed to the Start() 
  /// function. Note that this parameter is the variable-length device path and not a pointer 
  /// to the device path.
  ///  
  UINT8                          *RemainingDevicePath;
} EFI_STATUS_CODE_START_EXTENDED_DATA;

///
/// Legacy Oprom extended data
/// The device handle and ROM image base can be used by consumers to determine which option 
/// ROM failed. Due to the black-box nature of legacy option ROMs, the amount of information that 
/// can be obtained may be limited.
///
typedef struct {
  ///
  /// The data header identifying the data.
  ///
  EFI_STATUS_CODE_DATA  DataHeader;
  ///
  /// The handle corresponding to the device that this legacy option ROM is being invoked.
  ///
  EFI_HANDLE            DeviceHandle;
  ///
  /// The base address of the shadowed legacy ROM image.  
  /// May or may not point to the shadow RAM area. 
  ///
  EFI_PHYSICAL_ADDRESS  RomImageBase;
} EFI_LEGACY_OPROM_EXTENDED_DATA;

#pragma pack()

extern EFI_GUID gEfiStatusCodeSpecificDataGuid;

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
// Debug Assert Data. This is part of Status Code Specification
//
#define EFI_STATUS_CODE_DATA_TYPE_ASSERT_GUID \
 { 0xDA571595, 0x4D99, 0x487C, { 0x82, 0x7C, 0x26, 0x22, 0x67, 0x7D, 0x33, 0x07 } }


extern EFI_GUID gEfiStatusCodeDataTypeAssertGuid;

//
// Debug DataType defintions. User Defined Data Types.
//
#define EFI_STATUS_CODE_DATA_TYPE_DEBUG_GUID \
  { 0x9A4E9246, 0xD553, 0x11D5, { 0x87, 0xE2, 0x00, 0x06, 0x29, 0x45, 0xC3, 0xb9 } }

extern EFI_GUID gEfiStatusCodeDataTypeDebugGuid;

//
// Progress Code. User Defined Data Type Guid.
//
#define EFI_STATUS_CODE_DATA_TYPE_ERROR_GUID \
  { 0xAB359CE3, 0x99B3, 0xAE18, { 0xC8, 0x9D, 0x95, 0xD3, 0xB0, 0x72, 0xE1, 0x9B } }

extern EFI_GUID gEfiStatusCodeDataTypeErrorGuid;

//
// Progress Code. User Defined Data Type Guid.
//
#define EFI_STATUS_CODE_DATA_TYPE_PROGRESS_CODE_GUID \
  { 0xA356AB39, 0x35C4, 0x35DA, { 0xB3, 0x7A, 0xF8, 0xEA, 0x9E, 0x8B, 0x36, 0xA3 } }

extern EFI_GUID gEfiStatusCodeDataTypeProgressCodeGuid;


#endif
