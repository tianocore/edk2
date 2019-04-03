/** @file
  The file defined some common structures used for communicating between SMM variable module and SMM variable wrapper module.

Copyright (c) 2011 - 2015, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMM_VARIABLE_COMMON_H_
#define _SMM_VARIABLE_COMMON_H_

#include <Protocol/VarCheck.h>

#define EFI_SMM_VARIABLE_WRITE_GUID \
  { 0x93ba1826, 0xdffb, 0x45dd, { 0x82, 0xa7, 0xe7, 0xdc, 0xaa, 0x3b, 0xbd, 0xf3 } }

extern EFI_GUID gSmmVariableWriteGuid;

//
// This structure is used for SMM variable. the collected statistics data is saved in SMRAM. It can be got from
// SMI handler. The communication buffer should be:
// EFI_SMM_COMMUNICATE_HEADER + SMM_VARIABLE_COMMUNICATE_HEADER + payload.
//
typedef struct {
  UINTN       Function;
  EFI_STATUS  ReturnStatus;
  UINT8       Data[1];
} SMM_VARIABLE_COMMUNICATE_HEADER;

//
// The payload for this function is SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE.
//
#define SMM_VARIABLE_FUNCTION_GET_VARIABLE            1
//
// The payload for this function is SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME.
//
#define SMM_VARIABLE_FUNCTION_GET_NEXT_VARIABLE_NAME  2
//
// The payload for this function is SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE.
//
#define SMM_VARIABLE_FUNCTION_SET_VARIABLE            3
//
// The payload for this function is SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO.
//
#define SMM_VARIABLE_FUNCTION_QUERY_VARIABLE_INFO     4
//
// It is a notify event, no extra payload for this function.
//
#define SMM_VARIABLE_FUNCTION_READY_TO_BOOT           5
//
// It is a notify event, no extra payload for this function.
//
#define SMM_VARIABLE_FUNCTION_EXIT_BOOT_SERVICE       6
//
// The payload for this function is VARIABLE_INFO_ENTRY. The GUID in EFI_SMM_COMMUNICATE_HEADER
// is gEfiSmmVariableProtocolGuid.
//
#define SMM_VARIABLE_FUNCTION_GET_STATISTICS          7
//
// The payload for this function is SMM_VARIABLE_COMMUNICATE_LOCK_VARIABLE
//
#define SMM_VARIABLE_FUNCTION_LOCK_VARIABLE           8

#define SMM_VARIABLE_FUNCTION_VAR_CHECK_VARIABLE_PROPERTY_SET  9

#define SMM_VARIABLE_FUNCTION_VAR_CHECK_VARIABLE_PROPERTY_GET  10

#define SMM_VARIABLE_FUNCTION_GET_PAYLOAD_SIZE        11

///
/// Size of SMM communicate header, without including the payload.
///
#define SMM_COMMUNICATE_HEADER_SIZE  (OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data))

///
/// Size of SMM variable communicate header, without including the payload.
///
#define SMM_VARIABLE_COMMUNICATE_HEADER_SIZE  (OFFSET_OF (SMM_VARIABLE_COMMUNICATE_HEADER, Data))

///
/// This structure is used to communicate with SMI handler by SetVariable and GetVariable.
///
typedef struct {
  EFI_GUID    Guid;
  UINTN       DataSize;
  UINTN       NameSize;
  UINT32      Attributes;
  CHAR16      Name[1];
} SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE;

///
/// This structure is used to communicate with SMI handler by GetNextVariableName.
///
typedef struct {
  EFI_GUID    Guid;
  UINTN       NameSize;     // Return name buffer size
  CHAR16      Name[1];
} SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME;

///
/// This structure is used to communicate with SMI handler by QueryVariableInfo.
///
typedef struct {
  UINT64          MaximumVariableStorageSize;
  UINT64          RemainingVariableStorageSize;
  UINT64          MaximumVariableSize;
  UINT32          Attributes;
} SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO;

typedef SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME SMM_VARIABLE_COMMUNICATE_LOCK_VARIABLE;

typedef struct {
  EFI_GUID                      Guid;
  UINTN                         NameSize;
  VAR_CHECK_VARIABLE_PROPERTY   VariableProperty;
  CHAR16                        Name[1];
} SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY;

typedef struct {
  UINTN                         VariablePayloadSize;
} SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE;

#endif // _SMM_VARIABLE_COMMON_H_
