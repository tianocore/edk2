/** @file
  System reset Library Services.  This library class defines a set of
  methods that reset the whole system.

Copyright (c) 2005 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __RESET_SYSTEM_LIB_H__
#define __RESET_SYSTEM_LIB_H__

/**
  This function causes a system-wide reset (cold reset), in which
  all circuitry within the system returns to its initial state. This type of reset 
  is asynchronous to system operation and operates without regard to 
  cycle boundaries.

  If this function returns, it means that the system does not support cold reset. 
**/
VOID
EFIAPI
ResetCold (
  VOID
  );

/**
  This function causes a system-wide initialization (warm reset), in which all processors 
  are set to their initial state. Pending cycles are not corrupted.

  If this function returns, it means that the system does not support warm reset.
**/
VOID
EFIAPI
ResetWarm (
  VOID
  );

/**
  This function causes the system to enter a power state equivalent 
  to the ACPI G2/S5 or G3 states.
  
  If this function returns, it means that the system does not support shutdown reset.
**/
VOID
EFIAPI
ResetShutdown (
  VOID
  );

/**
  This function causes the system to enter S3 and then wake up immediately.
  
  If this function returns, it means that the system does not support S3 feature.
**/
VOID
EFIAPI
EnterS3WithImmediateWake (
  VOID
  );

/**
  This function causes a systemwide reset. The exact type of the reset is
  defined by the EFI_GUID that follows the Null-terminated Unicode string passed
  into ResetData. If the platform does not recognize the EFI_GUID in ResetData
  the platform must pick a supported reset type to perform.The platform may
  optionally log the parameters from any non-normal reset that occurs.

  @param[in]  DataSize   The size, in bytes, of ResetData.
  @param[in]  ResetData  The data buffer starts with a Null-terminated string,
                         followed by the EFI_GUID.
**/
VOID
EFIAPI
ResetPlatformSpecific (
  IN UINTN   DataSize,
  IN VOID    *ResetData
  );

#endif
