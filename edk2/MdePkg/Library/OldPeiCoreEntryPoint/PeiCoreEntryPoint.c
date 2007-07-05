/** @file
  Entry point to a the PEI Core.

Copyright (c) 2006 - 2007, Intel Corporation<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

//
// The package level header files this module uses
//
#include <PiPei.h>
//
// The protocols, PPI and GUID defintions for this module
//
//
// The Library classes this module produced
//
#include <Library/OldPeiCoreEntryPoint.h>

/**

  Enrty point to PEI core.

  @param SecCoreData    Points to a data structure containing
                        information about the PEI core's
                        operating environment, such as the size
                        and location of temporary RAM, the stack
                        location and the BFV location. The type
                        EFI_SEC_PEI_HAND_OFF is

  @param PpiList        Points to a list of one or more PPI
                        descriptors to be installed initially by
                        the PEI core. An empty PPI list consists
                        of a single descriptor with the end-tag
                        EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST.
                        As part of its initialization phase, the
                        PEI Foundation will add these SEC-hosted
                        PPIs to its PPI database such that both
                        the PEI Foundation and any modules can
                        leverage the associated service calls
                        and/or code in these early PPIs.

**/
EFI_STATUS
EFIAPI 
_ModuleEntryPoint(
  IN EFI_PEI_STARTUP_DESCRIPTOR  *PeiStartupDescriptor
)
{
  return ProcessModuleEntryPointList (PeiStartupDescriptor, NULL);
}


/**
  
  Wrapper of enrty point to PEI core.
  
  @param SecCoreData    Points to a data structure containing
                        information about the PEI core's
                        operating environment, such as the size
                        and location of temporary RAM, the stack
                        location and the BFV location. The type
                        EFI_SEC_PEI_HAND_OFF is

  @param PpiList        Points to a list of one or more PPI
                        descriptors to be installed initially by
                        the PEI core. An empty PPI list consists
                        of a single descriptor with the end-tag
                        EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST.
                        As part of its initialization phase, the
                        PEI Foundation will add these SEC-hosted
                        PPIs to its PPI database such that both
                        the PEI Foundation and any modules can
                        leverage the associated service calls
                        and/or code in these early PPIs.

**/
EFI_STATUS
EFIAPI
EfiMain (
  IN EFI_PEI_STARTUP_DESCRIPTOR  *PeiStartupDescriptor
  )
{
  return _ModuleEntryPoint (PeiStartupDescriptor);
}
