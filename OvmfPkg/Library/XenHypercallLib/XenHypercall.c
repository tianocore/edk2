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

#include <IndustryStandard/Xen/hvm/params.h>
#include <IndustryStandard/Xen/memory.h>

#include <Library/DebugLib.h>
#include <Library/XenHypercallLib.h>

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
