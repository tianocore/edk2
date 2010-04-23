/** @file
  This file defines the data structures per HOB specification v0.9.

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  These definitions are from the HOB Spec 0.9 that were not adopted by the PI specifications.

**/

#ifndef _HOB_H_
#define _HOB_H_

///
/// Capsule volume HOB -- identical to a firmware volume.
/// This macro is defined to comply with the hob Framework Spec. And the marco was
/// retired in the PI1.0 specification.  
///
#define EFI_HOB_TYPE_CV           0x0008

typedef struct {
  EFI_HOB_GENERIC_HEADER            Header;
  EFI_PHYSICAL_ADDRESS              BaseAddress;
  UINT64                            Length;
} EFI_HOB_CAPSULE_VOLUME;

#endif
