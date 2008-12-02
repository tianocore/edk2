/** @file
  Platform BDS library definition. Platform package can provide hook library
  instances to implement platform specific behavior.

Copyright (c) 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PLATFORM_BDS_LIB_H_
#define __PLATFORM_BDS_LIB_H_

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
  ///
  /// Save the current boot mode
  ///
  EFI_BOOT_MODE             BootMode;
  ///
  /// Set true if boot with default settings
  ///
  BOOLEAN                   DefaultBoot;
  ///
  /// The system default timeout for choose the boot option
  ///
  UINT16                    TimeoutDefault;
  ///
  /// Memory Test Level
  ///
  EXTENDMEM_COVERAGE_LEVEL  MemoryTestLevel;
};

/**
  Platform Bds init. Incude the platform firmware vendor, revision
  and so crc check.

  @param  PrivateData             The EFI_BDS_ARCH_PROTOCOL_INSTANCE instance

**/
VOID
EFIAPI
PlatformBdsInit (
  IN EFI_BDS_ARCH_PROTOCOL_INSTANCE  *PrivateData
  );

/**
  The function will excute with as the platform policy, current policy
  is driven by boot mode. IBV/OEM can customize this code for their specific
  policy action.

  @param  PrivateData             The EFI_BDS_ARCH_PROTOCOL_INSTANCE instance
  @param  DriverOptionList        The header of the driver option link list
  @param  BootOptionList          The header of the boot option link list

**/
VOID
EFIAPI
PlatformBdsPolicyBehavior (
  IN EFI_BDS_ARCH_PROTOCOL_INSTANCE  *PrivateData,
  IN LIST_ENTRY                      *DriverOptionList,
  IN LIST_ENTRY                      *BootOptionList
  );

/**
  Hook point after a boot attempt fails.

  @param  Option                  Pointer to Boot Option that failed to boot.
  @param  Status                  Status returned from failed boot.
  @param  ExitData                Exit data returned from failed boot.
  @param  ExitDataSize            Exit data size returned from failed boot.

**/
VOID
EFIAPI
PlatformBdsBootFail (
  IN  BDS_COMMON_OPTION  *Option,
  IN  EFI_STATUS         Status,
  IN  CHAR16             *ExitData,
  IN  UINTN              ExitDataSize
  );

/**
  Hook point after a boot attempt succeeds. We don't expect a boot option to
  return, so the UEFI 2.0 specification defines that you will default to an
  interactive mode and stop processing the BootOrder list in this case. This
  is alos a platform implementation and can be customized by IBV/OEM.

  @param  Option                  Pointer to Boot Option that succeeded to boot.

**/
VOID
EFIAPI
PlatformBdsBootSuccess (
  IN  BDS_COMMON_OPTION *Option
  );


/**
  This function locks platform flash that is not allowed to be updated during normal boot path.
  The flash layout is platform specific.

  @retval EFI_SUCCESS             The non-updatable flash areas.
**/
EFI_STATUS
EFIAPI
PlatformBdsLockNonUpdatableFlash (
  VOID
  );
#endif
