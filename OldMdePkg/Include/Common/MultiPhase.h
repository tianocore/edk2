/** @file
  This includes some definitions that will be used in both PEI and DXE phases.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  MultiPhase.h

**/

#ifndef __MULTI_PHASE_H__
#define __MULTI_PHASE_H__

//
// Needed EFI defines for PEI
//
typedef UINT64  EFI_PHYSICAL_ADDRESS;

typedef enum {
  EfiReservedMemoryType,
  EfiLoaderCode,
  EfiLoaderData,
  EfiBootServicesCode,
  EfiBootServicesData,
  EfiRuntimeServicesCode,
  EfiRuntimeServicesData,
  EfiConventionalMemory,
  EfiUnusableMemory,
  EfiACPIReclaimMemory,
  EfiACPIMemoryNVS,
  EfiMemoryMappedIO,
  EfiMemoryMappedIOPortSpace,
  EfiPalCode,
  EfiMaxMemoryType
} EFI_MEMORY_TYPE;

typedef UINT32  EFI_STATUS_CODE_TYPE;
typedef UINT32  EFI_STATUS_CODE_VALUE;

typedef struct {
  UINT16    HeaderSize;
  UINT16    Size;
  EFI_GUID  Type;
} EFI_STATUS_CODE_DATA;

typedef struct {
  UINT64  Signature;
  UINT32  Revision;
  UINT32  HeaderSize;
  UINT32  CRC32;
  UINT32  Reserved;
} EFI_TABLE_HEADER;

#define EFI_PAGE_SIZE         4096


typedef VOID    *EFI_HANDLE;
typedef UINT16  EFI_HII_HANDLE;
typedef UINT16  STRING_REF;
typedef struct {
  INT16     Value;
  INT16     Exponent;
} EFI_EXP_BASE10_DATA;

//
// Define macros to build data structure signatures from characters.
//
#define EFI_SIGNATURE_16(A, B)        ((A) | (B << 8))
#define EFI_SIGNATURE_32(A, B, C, D)  (EFI_SIGNATURE_16 (A, B) | (EFI_SIGNATURE_16 (C, D) << 16))
#define EFI_SIGNATURE_64(A, B, C, D, E, F, G, H) \
    (EFI_SIGNATURE_32 (A, B, C, D) | ((UINT64) (EFI_SIGNATURE_32 (E, F, G, H)) << 32))


#include <Protocol/DevicePath.h>

#endif
