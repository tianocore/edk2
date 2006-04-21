/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  MonotonicCounter.h

Abstract:

  Produces the Monotonic Counter services as defined in the DXE CIS

--*/

#ifndef _MONOTONIC_COUNTER_DRIVER_H_
#define _MONOTONIC_COUNTER_DRIVER_H_

//
// Function Prototypes
//
EFI_STATUS
EFIAPI
MonotonicCounterDriverInitialize (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
;

#endif
