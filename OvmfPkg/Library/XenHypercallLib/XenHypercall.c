/** @file
  Functions to make Xen hypercalls.

  Copyright (C) 2014, Citrix Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <IndustryStandard/Xen/hvm/params.h>
#include <IndustryStandard/Xen/memory.h>

#include <Library/DebugLib.h>
#include <Library/XenHypercallLib.h>

RETURN_STATUS
EFIAPI
XenHypercallLibConstruct (
  VOID
  )
{
  XenHypercallLibInit ();
  //
  // We don't fail library construction, since that has catastrophic
  // consequences for client modules (whereas those modules may easily be
  // running on a non-Xen platform). Instead, XenHypercallIsAvailable()
  // will return FALSE.
  //
  return RETURN_SUCCESS;
}

UINT64
EFIAPI
XenHypercallHvmGetParam (
  IN UINT32  Index
  )
{
  xen_hvm_param_t  Parameter;
  INTN             Error;

  Parameter.domid = DOMID_SELF;
  Parameter.index = Index;
  Error           = XenHypercall2 (
                      __HYPERVISOR_hvm_op,
                      HVMOP_get_param,
                      (INTN)&Parameter
                      );
  if (Error != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "XenHypercall: Error %Ld trying to get HVM parameter %d\n",
      (INT64)Error,
      Index
      ));
    return 0;
  }

  return Parameter.value;
}

INTN
EFIAPI
XenHypercallMemoryOp (
  IN     UINTN  Operation,
  IN OUT VOID   *Arguments
  )
{
  return XenHypercall2 (
           __HYPERVISOR_memory_op,
           Operation,
           (INTN)Arguments
           );
}

INTN
EFIAPI
XenHypercallEventChannelOp (
  IN     INTN  Operation,
  IN OUT VOID  *Arguments
  )
{
  return XenHypercall2 (
           __HYPERVISOR_event_channel_op,
           Operation,
           (INTN)Arguments
           );
}
