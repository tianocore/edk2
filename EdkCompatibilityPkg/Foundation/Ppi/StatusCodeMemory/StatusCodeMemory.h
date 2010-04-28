/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  StatusCodeMemory.h
    
Abstract:

  Status Code memory descriptor PPI.  Contains information about memory that 
  the Status Code PEIM may use to journal Status Codes.

--*/

#ifndef _PEI_STATUS_CODE_MEMORY_PPI_H_
#define _PEI_STATUS_CODE_MEMORY_PPI_H_

//
// GUID definition
//
#define PEI_STATUS_CODE_MEMORY_PPI_GUID \
  { 0x26f8ab01, 0xd3cd, 0x489c, {0x98, 0x4f, 0xdf, 0xde, 0xf7, 0x68, 0x39, 0x5b} }

//
// Data types
//
typedef struct {
  EFI_STATUS_CODE_TYPE    Type;
  EFI_STATUS_CODE_VALUE   Value;
  UINT32                  Instance;
} EFI_STATUS_CODE_ENTRY;

//
// PPI definition
//
typedef struct {
  UINTN                   FirstEntry;
  UINTN                   LastEntry;
  EFI_PHYSICAL_ADDRESS    Address;
  UINTN                   Length;
} PEI_STATUS_CODE_MEMORY_PPI;

extern EFI_GUID gPeiStatusCodeMemoryPpiGuid;

#endif
