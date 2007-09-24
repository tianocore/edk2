/** @file
  MDE PI library functions and macros for PEI phase

  Copyright (c) 2007, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __PEI_PI_LIB_H__
#define __PEI_PI_LIB_H__

#include <Pi/PiFirmwareFile.h>

VOID
EFIAPI
PeiPiLibBuildPiFvInfoPpi (
  IN EFI_PHYSICAL_ADDRESS    FvStart,
  IN UINT64                  FvLength,
  IN EFI_GUID                *ParentFvName,
  IN EFI_GUID                *PraentFileName
);

#endif

