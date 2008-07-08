/** @file
  EDK II specific implementation of UEFI variable depend on data structure.

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

#define VARIABLE_STORE_SIGNATURE  EFI_SIGNATURE_32 ('$', 'V', 'S', 'S')

#define MAX_VARIABLE_SIZE                 FixedPcdGet32(PcdMaxVariableSize)

//
// Enlarges the hardware error record maximum variable size to 32K bytes
//
#define MAX_HARDWARE_ERROR_VARIABLE_SIZE  FixedPcdGet32(PcdMaxHardwareErrorVariableSize)

#define VARIABLE_DATA                     0x55AA

//
// Variable Store Header flags
//
#define VARIABLE_STORE_FORMATTED          0x5a
#define VARIABLE_STORE_HEALTHY            0xfe

//
// The alignment of variable's start offset.
// For IA32/X64 architecture, the alignment is set to 1, and
// 8 is for IPF archtecture.
//
#if defined (MDE_CPU_IPF)
#define ALIGNMENT         8
#else
#define ALIGNMENT         1
#endif

#define HEADER_ALIGNMENT  4

//
// Variable Store Status
//
typedef enum {
  EfiRaw,
  EfiValid,
  EfiInvalid,
  EfiUnknown
} VARIABLE_STORE_STATUS;

//
// Variable State flags
//
#define VAR_IN_DELETED_TRANSITION     0xfe  // Variable is in obsolete transistion
#define VAR_DELETED                   0xfd  // Variable is obsolete
#define VAR_HEADER_VALID_ONLY         0x7f  // Variable header has been valid
#define VAR_ADDED                     0x3f  // Variable has been completely added
                                            // 
#define IS_VARIABLE_STATE(_c, _Mask)  (BOOLEAN) (((~_c) & (~_Mask)) != 0)

#pragma pack(1)

//
// Variable Store region header
//
typedef struct {
  UINT32  Signature;
  UINT32  Size;
  UINT8   Format;
  UINT8   State;
  UINT16  Reserved;
  UINT32  Reserved1;
} VARIABLE_STORE_HEADER;

//
// Variable header structure
//
typedef struct {
  UINT16      StartId;
  UINT8       State;
  UINT8       Reserved;
  UINT32      Attributes;
  UINT32      NameSize;
  UINT32      DataSize;
  EFI_GUID    VendorGuid;
} VARIABLE_HEADER;

#pragma pack()

#endif // _EFI_VARIABLE_H_
