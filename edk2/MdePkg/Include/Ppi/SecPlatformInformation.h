/** @file
  This file declares Sec Platform Information PPI.

  Copyright (c) 2006 - 2007, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  SecPlatformInformation.h

  @par Revision Reference:
  This PPI is defined in PI.
  Version 1.00.

**/

#ifndef __SEC_PLATFORM_INFORMATION_PPI_H__
#define __SEC_PLATFORM_INFORMATION_PPI_H__

#define EFI_SEC_PLATFORM_INFORMATION_GUID \
  { \
    0x6f8c2b35, 0xfef4, 0x448d, {0x82, 0x56, 0xe1, 0x1b, 0x19, 0xd6, 0x10, 0x77 } \
  }

typedef struct _EFI_SEC_PLATFORM_INFORMATION_PPI EFI_SEC_PLATFORM_INFORMATION_PPI;


///
/// EFI_HEALTH_FLAGS
///
typedef union {
  struct {
    UINT32   Status                   : 2;
    UINT32   Tested                   : 1;
    UINT32   Reserved1                :13;
    UINT32   VirtualMemoryUnavailable : 1;
    UINT32   Ia32ExecutionUnavailable : 1;
    UINT32   FloatingPointUnavailable : 1;
    UINT32   MiscFeaturesUnavailable  : 1;
    UINT32   Reserved2                :12;
  } Bits;
  UINT32     Uint32;
} EFI_HEALTH_FLAGS;

typedef struct {
  EFI_HEALTH_FLAGS HealthFlags;
} EFI_SEC_PLATFORM_INFORMATION_RECORD;



/**
  This interface conveys state information out of the Security (SEC) phase into PEI.

  @param  PeiServices               Pointer to the PEI Services Table.
  @param  StructureSize             Pointer to the variable describing size of the input buffer.
  @param  PlatformInformationRecord Pointer to the EFI_SEC_PLATFORM_INFORMATION_RECORD.

  @retval EFI_SUCCESS           The data was successfully returned.
  @retval EFI_BUFFER_TOO_SMALL  The buffer was too small.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SEC_PLATFORM_INFORMATION) (
	IN CONST 	EFI_PEI_SERVICES 										**PeiServices,
	IN OUT 		UINT64 															*StructureSize,
	OUT 			EFI_SEC_PLATFORM_INFORMATION_RECORD	*PlatformInformationRecord
);


/**
	Ppi Description:

  @param Name

**/
struct _EFI_SEC_PLATFORM_INFORMATION_PPI {
  EFI_SEC_PLATFORM_INFORMATION  PlatformInformation;
};


extern EFI_GUID gEfiSecPlatformInformationPpiGuid;

#endif
