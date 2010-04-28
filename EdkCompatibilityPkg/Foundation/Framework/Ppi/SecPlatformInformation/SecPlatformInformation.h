/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  SecPlatformInformation.h
    
Abstract:

  Sec Platform Information PPI as defined in Tiano

--*/

#ifndef _PEI_SEC_PLATFORM_INFORMATION_PPI_H
#define _PEI_SEC_PLATFORM_INFORMATION_PPI_H

#define EFI_SEC_PLATFORM_INFORMATION_GUID \
  { \
    0x6f8c2b35, 0xfef4, 0x448d, {0x82, 0x56, 0xe1, 0x1b, 0x19, 0xd6, 0x10, 0x77} \
  }

EFI_FORWARD_DECLARATION (EFI_SEC_PLATFORM_INFORMATION_PPI);

extern EFI_GUID gEfiSecPlatformInformationPpiGuid;

typedef union {
  struct {
  UINT32 Status : 2;
  UINT32 Tested : 1;
  UINT32 Reserved1 :13;
  UINT32 VirtualMemoryUnavailable : 1;
  UINT32 Ia32ExecutionUnavailable : 1;
  UINT32 FloatingPointUnavailable : 1;
  UINT32 MiscFeaturesUnavailable : 1;
  UINT32 Reserved2 :12;
  } Bits;
  UINT32 Uint32;
} EFI_HEALTH_FLAGS;

typedef struct {
  EFI_HEALTH_FLAGS HealthFlags;
} SEC_PLATFORM_INFORMATION_RECORD;

typedef struct {
  UINTN BootPhase;      // entry r20 value
  UINTN UniqueId;       // PAL arbitration ID
  UINTN HealthStat;     // Health Status
  UINTN PALRetAddress;  // return address to PAL
} IPF_HANDOFF_STATUS;

typedef
EFI_STATUS
(EFIAPI *SEC_PLATFORM_INFORMATION) (
  IN EFI_PEI_SERVICES                    **PeiServices,
  IN OUT UINT64                          *StructureSize,
  IN OUT SEC_PLATFORM_INFORMATION_RECORD *PlatformInformationRecord
  );

struct _EFI_SEC_PLATFORM_INFORMATION_PPI {
  SEC_PLATFORM_INFORMATION  PlatformInformation;
};

#endif
