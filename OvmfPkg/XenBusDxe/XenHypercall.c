/** @file
  Functions to make Xen hypercalls.

  Copyright (C) 2014, Citrix Ltd.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Library/HobLib.h>
#include <Guid/XenInfo.h>

#include "XenBusDxe.h"
#include "XenHypercall.h"

#include <IndustryStandard/Xen/hvm/params.h>
#include <IndustryStandard/Xen/memory.h>

STATIC VOID       *HyperPage;

//
// Interface exposed by the ASM implementation of the core hypercall
//
INTN
EFIAPI
__XenHypercall2 (
  IN     VOID *HypercallAddr,
  IN OUT INTN Arg1,
  IN OUT INTN Arg2
  );

EFI_STATUS
XenHyperpageInit (
  )
{
  EFI_HOB_GUID_TYPE   *GuidHob;
  EFI_XEN_INFO        *XenInfo;

  GuidHob = GetFirstGuidHob (&gEfiXenInfoGuid);
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }
  XenInfo = (EFI_XEN_INFO *) GET_GUID_HOB_DATA (GuidHob);
  HyperPage = XenInfo->HyperPages;
  return EFI_SUCCESS;
}

UINT64
XenHypercallHvmGetParam (
  IN UINT32        Index
  )
{
  xen_hvm_param_t     Parameter;
  INTN                Error;

  Parameter.domid = DOMID_SELF;
  Parameter.index = Index;
  Error = XenHypercall2 (__HYPERVISOR_hvm_op,
                         HVMOP_get_param, (INTN) &Parameter);
  if (Error != 0) {
    DEBUG ((EFI_D_ERROR,
            "XenHypercall: Error %d trying to get HVM parameter %d\n",
            Error, Index));
    return 0;
  }
  return Parameter.value;
}

INTN
XenHypercallMemoryOp (
  IN     UINTN Operation,
  IN OUT VOID *Arguments
  )
{
  return XenHypercall2 (__HYPERVISOR_memory_op,
                        Operation, (INTN) Arguments);
}

INTN
XenHypercallEventChannelOp (
  IN     INTN Operation,
  IN OUT VOID *Arguments
  )
{
  return XenHypercall2 (__HYPERVISOR_event_channel_op,
                        Operation, (INTN) Arguments);
}

INTN
EFIAPI
XenHypercall2 (
  IN     UINTN  HypercallID,
  IN OUT INTN   Arg1,
  IN OUT INTN   Arg2
  )
{
  ASSERT (HyperPage != NULL);

  return __XenHypercall2 ((UINT8*)HyperPage + HypercallID * 32, Arg1, Arg2);
}
