/*++

Copyright (c) 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PlatformBdsLib.h

Abstract:

  Platform BDS library definition, include the file and data structure

--*/

#ifndef __PLATFORM_BDS_LIB_H_
#define __PLATFORM_BDS_LIB_H_

#include <PiDxe.h>
#include <Protocol/Bds.h>
#include <Protocol/GenericMemoryTest.h>

//
// Bds AP Context data
//
#define EFI_BDS_ARCH_PROTOCOL_INSTANCE_SIGNATURE  EFI_SIGNATURE_32 ('B', 'd', 's', 'A')
typedef struct _EFI_BDS_ARCH_PROTOCOL_INSTANCE EFI_BDS_ARCH_PROTOCOL_INSTANCE;

struct _EFI_BDS_ARCH_PROTOCOL_INSTANCE {
  UINTN                     Signature;
  EFI_HANDLE                Handle;
  EFI_BDS_ARCH_PROTOCOL     Bds;
  //
  // Save the current boot mode
  //
  EFI_BOOT_MODE             BootMode;
  //
  // Set true if boot with default settings
  //
  BOOLEAN                   DefaultBoot;
  //
  // The system default timeout for choose the boot option
  //
  UINT16                    TimeoutDefault;
  //
  // Memory Test Level
  //
  EXTENDMEM_COVERAGE_LEVEL  MemoryTestLevel;
};

//
// Platform BDS Functions
//
VOID
EFIAPI
PlatformBdsInit (
  IN EFI_BDS_ARCH_PROTOCOL_INSTANCE  *PrivateData
  )
;

VOID
EFIAPI
PlatformBdsPolicyBehavior (
  IN EFI_BDS_ARCH_PROTOCOL_INSTANCE  *PrivateData,
  IN LIST_ENTRY                      *DriverOptionList,
  IN LIST_ENTRY                      *BootOptionList
  )
;


VOID
PlatformBdsBootFail (
  IN  BDS_COMMON_OPTION  *Option,
  IN  EFI_STATUS         Status,
  IN  CHAR16             *ExitData,
  IN  UINTN              ExitDataSize
  )
;

VOID
PlatformBdsBootSuccess (
  IN  BDS_COMMON_OPTION *Option
  )
;

EFI_STATUS
EFIAPI
PlatformBdsLockNonUpdatableFlash (
  VOID
  )
;
#endif
