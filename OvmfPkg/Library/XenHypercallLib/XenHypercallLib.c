/** @file
Prepare to use Xen Hypercall.

Copyright (c) 2011, Bei Guan <gbtju85@gmail.com>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Guid/XenInfo.h>

#include <Library/XenHypercallLib.h>

/**
  Initialize the hypercalls: Get Xen Hypercall page from HOB.
  
  @param[out]   A pointer to hypercall page.

**/
VOID *
InitializeHypercallPage (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS  GuidHob;
  EFI_XEN_INFO          *mXenInfo;
 
  GuidHob.Raw = GetFirstGuidHob (&gEfiXenInfoGuid);
  if (GuidHob.Raw != NULL) {
    mXenInfo = GET_GUID_HOB_DATA (GuidHob.Guid);
    if (mXenInfo != NULL) {
      return (VOID *) (UINTN) mXenInfo->HyperPages;
    }
  } 

  DEBUG ((EFI_D_ERROR, "Gasket_Fail to get Xen info from hob\n"));
  return NULL;
}

