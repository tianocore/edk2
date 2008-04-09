/** @file
  This file include all platform action which can be customized
  by IBV/OEM.

Copyright (c) 2004 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BdsPlatform.h"

//
// BDS Platform Functions
//
VOID
PlatformBdsInit (
  IN EFI_BDS_ARCH_PROTOCOL_INSTANCE  *PrivateData
  )
/*++

Routine Description:

  Platform Bds init. Incude the platform firmware vendor, revision
  and so crc check.

Arguments:

  PrivateData  - The EFI_BDS_ARCH_PROTOCOL_INSTANCE instance

Returns:

  None.

--*/
{
  return;
}

EFI_STATUS
PlatformBdsConnectConsole (
  IN BDS_CONSOLE_CONNECT_ENTRY   *PlatformConsole
  )
/*++

Routine Description:

  Connect the predefined platform default console device. Always try to find
  and enable the vga device if have.

Arguments:

  PlatformConsole         - Predfined platform default console device array.
 
Returns:

  EFI_SUCCESS             - Success connect at least one ConIn and ConOut 
                            device, there must have one ConOut device is 
                            active vga device.
  
  EFI_STATUS              - Return the status of 
                            BdsLibConnectAllDefaultConsoles ()

--*/
{
  return EFI_SUCCESS;
}

VOID
PlatformBdsConnectSequence (
  VOID
  )
/*++

Routine Description:

  Connect with predeined platform connect sequence, 
  the OEM/IBV can customize with their own connect sequence.
  
Arguments:

  None.
 
Returns:

  None.
  
--*/
{
  return;
}

VOID
PlatformBdsGetDriverOption (
  IN OUT LIST_ENTRY              *BdsDriverLists
  )
/*++

Routine Description:

  Load the predefined driver option, OEM/IBV can customize this
  to load their own drivers
  
Arguments:

  BdsDriverLists  - The header of the driver option link list.
 
Returns:

  None.
  
--*/
{
  return;
}

VOID
PlatformBdsDiagnostics (
  IN EXTENDMEM_COVERAGE_LEVEL    MemoryTestLevel,
  IN BOOLEAN                     QuietBoot
  )
/*++

Routine Description:

  Perform the platform diagnostic, such like test memory. OEM/IBV also
  can customize this fuction to support specific platform diagnostic.
  
Arguments:

  MemoryTestLevel  - The memory test intensive level
  
  QuietBoot        - Indicate if need to enable the quiet boot
 
Returns:

  None.
  
--*/
{
  return;
}

VOID
PlatformBdsPolicyBehavior (
  IN EFI_BDS_ARCH_PROTOCOL_INSTANCE  *PrivateData,
  IN OUT LIST_ENTRY                  *DriverOptionList,
  IN OUT LIST_ENTRY                  *BootOptionList
  )
/*++

Routine Description:

  The function will excute with as the platform policy, current policy
  is driven by boot mode. IBV/OEM can customize this code for their specific
  policy action.
  
Arguments:

  PrivateData      - The EFI_BDS_ARCH_PROTOCOL_INSTANCE instance
  
  DriverOptionList - The header of the driver option link list
  
  BootOptionList   - The header of the boot option link list
 
Returns:

  None.
  
--*/
{
  return ;
}

VOID
PlatformBdsBootSuccess (
  IN  BDS_COMMON_OPTION   *Option
  )
/*++

Routine Description:
  
  Hook point after a boot attempt succeeds. We don't expect a boot option to
  return, so the EFI 1.0 specification defines that you will default to an
  interactive mode and stop processing the BootOrder list in this case. This
  is alos a platform implementation and can be customized by IBV/OEM.

Arguments:

  Option - Pointer to Boot Option that succeeded to boot.

Returns:
  
  None.

--*/
{
  return;
}

VOID
PlatformBdsBootFail (
  IN  BDS_COMMON_OPTION  *Option,
  IN  EFI_STATUS         Status,
  IN  CHAR16             *ExitData,
  IN  UINTN              ExitDataSize
  )
/*++

Routine Description:
  
  Hook point after a boot attempt fails.

Arguments:
  
  Option - Pointer to Boot Option that failed to boot.

  Status - Status returned from failed boot.

  ExitData - Exit data returned from failed boot.

  ExitDataSize - Exit data size returned from failed boot.

Returns:
  
  None.

--*/
{
  return;
}

EFI_STATUS
PlatformBdsNoConsoleAction (
  VOID
  )
/*++

Routine Description:
  
  This function is remained for IBV/OEM to do some platform action,
  if there no console device can be connected.

Arguments:
  
  None.
  
Returns:
  
  EFI_SUCCESS      - Direct return success now.

--*/
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PlatformBdsLockNonUpdatableFlash (
  VOID
  )
{
  return EFI_SUCCESS;
}
