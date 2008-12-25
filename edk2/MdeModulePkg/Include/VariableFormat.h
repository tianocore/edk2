/** @file
  The variable data structure related to EDK II specific UEFI variable implementation.

  Copyright (c) 2006 - 2008 Intel Corporation. <BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __VARIABLE_FORMAT_H__
#define __VARIABLE_FORMAT_H__

///
/// Maximum buffer for the single variable.
///
#ifndef MAX_VARIABLE_SIZE
#define MAX_VARIABLE_SIZE                 FixedPcdGet32(PcdMaxVariableSize)
#endif

///
/// Maximum buffer for Hardware error record variable
///
#ifndef MAX_HARDWARE_ERROR_VARIABLE_SIZE
#define MAX_HARDWARE_ERROR_VARIABLE_SIZE  FixedPcdGet32(PcdMaxHardwareErrorVariableSize)
#endif

///
/// The alignment of variable's start offset.
/// For IA32/X64 architecture, the alignment is set to 1, and
/// 8 is for IPF archtecture.
///
#if defined (MDE_CPU_IPF)
#define ALIGNMENT         8
#else
#define ALIGNMENT         1
#endif

#define HEADER_ALIGNMENT  4

///
/// Variable Store Status
///
typedef enum {
  EfiRaw,
  EfiValid,
  EfiInvalid,
  EfiUnknown
} VARIABLE_STORE_STATUS;

#pragma pack(1)

#define VARIABLE_STORE_SIGNATURE  SIGNATURE_32 ('$', 'V', 'S', 'S')

///
/// Variable Store Header Format and State
///
#define VARIABLE_STORE_FORMATTED          0x5a
#define VARIABLE_STORE_HEALTHY            0xfe

///
/// Variable Store region header
///
typedef struct {
  ///
  /// Variable store region signature.
  ///
  UINT32  Signature;
  ///
  /// Size of variable store region including this header
  ///
  UINT32  Size;
  ///
  /// variable region format state
  ///
  UINT8   Format;
  ///
  /// variable region healthy state
  ///
  UINT8   State;
  UINT16  Reserved;
  UINT32  Reserved1;
} VARIABLE_STORE_HEADER;

///
/// Variable data start flag
///
#define VARIABLE_DATA                     0x55AA

///
/// Variable State flags
///
#define VAR_IN_DELETED_TRANSITION     0xfe  ///< Variable is in obsolete transistion
#define VAR_DELETED                   0xfd  ///< Variable is obsolete
#define VAR_HEADER_VALID_ONLY         0x7f  ///< Variable header has been valid
#define VAR_ADDED                     0x3f  ///< Variable has been completely added

///
/// Removed
///
#define IS_VARIABLE_STATE(_c, _Mask)  (BOOLEAN) (((~_c) & (~_Mask)) != 0)

///
/// Variable Data Header Structure
///
typedef struct {
  ///
  /// Variable Data Start Flag
  ///
  UINT16      StartId;
  ///
  /// Variable State defined above
  ///
  UINT8       State;
  UINT8       Reserved;
  ///
  /// Attributes of variable defined in UEFI spec
  ///
  UINT32      Attributes;
  ///
  /// Size of variable Null-terminated Unicode string name
  ///
  UINT32      NameSize;
  ///
  /// Size of the variable data without this header
  ///
  UINT32      DataSize;
  ///
  /// A unique identifier for the vendor.
  ///
  EFI_GUID    VendorGuid;
} VARIABLE_HEADER;

#pragma pack()

#endif // _EFI_VARIABLE_H_
