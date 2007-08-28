/** @file
  System reset Library Services.  This library class defines a set of
  methods to reset whole system.

  Copyright (c) 2005 - 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php
  
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __RESET_SYSTEM_LIB_H__
#define __RESET_SYSTEM_LIB_H__

/**
  Calling this function causes a system-wide reset. This sets
  all circuitry within the system to its initial state. This type of reset 
  is asynchronous to system operation and operates without regard to 
  cycle boundaries.

  System reset should not return, if it returns, it means the system does 
  not support cold reset.
**/
VOID
EFIAPI
ResetCold (
  VOID
  );

/**
  Calling this function causes a system-wide initialization. The processors 
  are set to their initial state, and pending cycles are not corrupted.

  System reset should not return, if it returns, it means the system does 
  not support warm reset.
**/
VOID
EFIAPI
ResetWarm (
  VOID
  );

/**
  Calling this function causes the system to enter a power state equivalent 
  to the ACPI G2/S5 or G3 states.
  
  System shutdown should not return, if it returns, it means the system does 
  not support shut down reset.
**/
VOID
EFIAPI
ResetShutdown (
  VOID
  );

/**
  Calling this function causes the system to enter S3 and then
  wake up immediately.
  
  Reset update should not return, if it returns, it means the
  library does not the feature.
**/
VOID
EFIAPI
EnterS3WithImmediateWake (
  VOID
  );
#endif
