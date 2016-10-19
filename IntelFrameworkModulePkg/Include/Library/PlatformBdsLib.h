/** @file
  Platform BDS library definition. A platform can implement 
  instances to support platform-specific behavior.

Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PLATFORM_BDS_LIB_H_
#define __PLATFORM_BDS_LIB_H_

#include <Protocol/GenericMemoryTest.h>
#include <Library/GenericBdsLib.h>

/**
  Perform the memory test base on the memory test intensive level,
  and update the memory resource.

  @param  Level         The memory test intensive level.

  @retval EFI_STATUS    Successfully test all the system memory, and update
                        the memory resource

**/
typedef
EFI_STATUS
(EFIAPI *BASEM_MEMORY_TEST)(
  IN EXTENDMEM_COVERAGE_LEVEL Level
  );

/**
  This routine is called to see if there are any capsules we need to process.
  If the boot mode is not UPDATE, then we do nothing. Otherwise, find the
  capsule HOBS and produce firmware volumes for them via the DXE service.
  Then call the dispatcher to dispatch drivers from them. Finally, check
  the status of the updates.

  This function should be called by BDS in case we need to do some
  sort of processing even if there is no capsule to process. We
  need to do this if an earlier update went away and we need to
  clear the capsule variable so on the next reset PEI does not see it and
  think there is a capsule available.

  @param BootMode                 The current boot mode

  @retval EFI_INVALID_PARAMETER   The boot mode is not correct for an update.
  @retval EFI_SUCCESS             There is no error when processing a capsule.

**/
typedef 
EFI_STATUS
(EFIAPI *PROCESS_CAPSULES)(
  IN EFI_BOOT_MODE BootMode
  );

/**
  Platform Bds initialization. Includes the platform firmware vendor, revision
  and so crc check.

**/
VOID
EFIAPI
PlatformBdsInit (
  VOID
  );

/**
  The function will execute with as the platform policy, current policy
  is driven by boot mode. IBV/OEM can customize this code for their specific
  policy action.

  @param  DriverOptionList        The header of the driver option link list
  @param  BootOptionList          The header of the boot option link list
  @param  ProcessCapsules         A pointer to ProcessCapsules()
  @param  BaseMemoryTest          A pointer to BaseMemoryTest()

**/
VOID
EFIAPI
PlatformBdsPolicyBehavior (
  IN LIST_ENTRY                      *DriverOptionList,
  IN LIST_ENTRY                      *BootOptionList,
  IN PROCESS_CAPSULES                ProcessCapsules,
  IN BASEM_MEMORY_TEST               BaseMemoryTest
  );

/**
  Hook point for a user-provided function, for after a boot attempt fails. 

  @param  Option                  A pointer to Boot Option that failed to boot.
  @param  Status                  The status returned from failed boot.
  @param  ExitData                The exit data returned from failed boot.
  @param  ExitDataSize            The exit data size returned from failed boot.

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
  is also a platform implementation, and can be customized by an IBV/OEM.

  @param  Option                  A pointer to the Boot Option that successfully booted.

**/
VOID
EFIAPI
PlatformBdsBootSuccess (
  IN  BDS_COMMON_OPTION  *Option
  );


/**
  This function locks platform flash that is not allowed to be updated during normal boot path.
  The flash layout is platform specific.

  **/
VOID
EFIAPI
PlatformBdsLockNonUpdatableFlash (
  VOID
  );

/**
  Lock the ConsoleIn device in system table. All key
  presses will be ignored until the Password is typed in. The only way to
  disable the password is to type it in to a ConIn device.

  @param  Password        The password used to lock ConIn device.

  @retval EFI_SUCCESS     Lock the Console In Spliter virtual handle successfully.
  @retval EFI_UNSUPPORTED Password not found.

**/
EFI_STATUS
EFIAPI
LockKeyboards (
  IN  CHAR16    *Password
  );

#endif
