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

EFI_STATUS
XenHyperpageInit (
  IN OUT XENBUS_DEVICE *Dev
  )
{
  EFI_HOB_GUID_TYPE   *GuidHob;
  EFI_XEN_INFO        *XenInfo;

  GuidHob = GetFirstGuidHob (&gEfiXenInfoGuid);
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }
  XenInfo = (EFI_XEN_INFO *) GET_GUID_HOB_DATA (GuidHob);
  Dev->Hyperpage = XenInfo->HyperPages;
  return EFI_SUCCESS;
}

UINT64
XenHypercallHvmGetParam (
  IN XENBUS_DEVICE *Dev,
  IN UINT32        Index
  )
{
  xen_hvm_param_t     Parameter;
  INTN                Error;

  ASSERT (Dev->Hyperpage != NULL);

  Parameter.domid = DOMID_SELF;
  Parameter.index = Index;
  Error = XenHypercall2 ((UINT8*)Dev->Hyperpage + __HYPERVISOR_hvm_op * 32,
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
  IN     XENBUS_DEVICE *Dev,
  IN     UINTN Operation,
  IN OUT VOID *Arguments
  )
{
  ASSERT (Dev->Hyperpage != NULL);
  return XenHypercall2 ((UINT8*)Dev->Hyperpage + __HYPERVISOR_memory_op * 32,
                        Operation, (INTN) Arguments);
}

INTN
XenHypercallEventChannelOp (
  IN     XENBUS_DEVICE *Dev,
  IN     INTN Operation,
  IN OUT VOID *Arguments
  )
{
  ASSERT (Dev->Hyperpage != NULL);
  return XenHypercall2 ((UINT8*)Dev->Hyperpage + __HYPERVISOR_event_channel_op * 32,
                        Operation, (INTN) Arguments);
}

EFI_STATUS
XenGetSharedInfoPage (
  IN OUT XENBUS_DEVICE *Dev
  )
{
  xen_add_to_physmap_t Parameter;

  ASSERT (Dev->SharedInfo == NULL);

  Parameter.domid = DOMID_SELF;
  Parameter.space = XENMAPSPACE_shared_info;
  Parameter.idx = 0;

  //
  // using reserved page because the page is not released when Linux is
  // starting because of the add_to_physmap. QEMU might try to access the
  // page, and fail because it have no right to do so (segv).
  //
  Dev->SharedInfo = AllocateReservedPages (1);
  Parameter.gpfn = (UINTN) Dev->SharedInfo >> EFI_PAGE_SHIFT;
  if (XenHypercallMemoryOp (Dev, XENMEM_add_to_physmap, &Parameter) != 0) {
    FreePages (Dev->SharedInfo, 1);
    Dev->SharedInfo = NULL;
    return EFI_LOAD_ERROR;
  }

  return EFI_SUCCESS;
}
