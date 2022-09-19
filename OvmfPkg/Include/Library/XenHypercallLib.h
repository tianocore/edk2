/** @file
  Functions declarations to make Xen hypercalls.

  Copyright (C) 2014, Citrix Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __XEN_HYPERCALL_LIB_H__
#define __XEN_HYPERCALL_LIB_H__

/**
  To call when the gEfiXenInfoGuid HOB became available after the library init
  function has already been executed.

  This allow to make hypercall in the PEIM stage.
**/
RETURN_STATUS
EFIAPI
XenHypercallLibInit (
  VOID
  );

/**
  Check if the Xen Hypercall library is able to make calls to the Xen
  hypervisor.

  Client code should call further functions in this library only if, and after,
  this function returns TRUE.

  @retval TRUE   Hypercalls are available.
  @retval FALSE  Hypercalls are not available.
**/
BOOLEAN
EFIAPI
XenHypercallIsAvailable (
  VOID
  );

/**
  This function will put the two arguments in the right place (registers) and
  invoke the hypercall identified by HypercallID.

  @param HypercallID    The symbolic ID of the hypercall to be invoked
  @param Arg1           First argument.
  @param Arg2           Second argument.

  @return   Return 0 if success otherwise it return an errno.
**/
INTN
EFIAPI
XenHypercall2 (
  IN     UINTN  HypercallID,
  IN OUT INTN   Arg1,
  IN OUT INTN   Arg2
  );

/**
  Return the value of the HVM parameter Index.

  @param Index  The parameter to get, e.g. HVM_PARAM_STORE_EVTCHN.

  @return   The value of the asked parameter or 0 in case of error.
**/
UINT64
EFIAPI
XenHypercallHvmGetParam (
  UINT32  Index
  );

/**
  Hypercall to do different operation on the memory.

  @param Operation  The operation number, e.g. XENMEM_add_to_physmap.
  @param Arguments  The arguments associated to the operation.

  @return  Return the return value from the hypercall, 0 in case of success
           otherwise, an error code.
**/
INTN
EFIAPI
XenHypercallMemoryOp (
  IN     UINTN  Operation,
  IN OUT VOID   *Arguments
  );

/**
  Do an operation on the event channels.

  @param Operation  The operation number, e.g. EVTCHNOP_send.
  @param Arguments  The argument associated to the operation.

  @return  Return the return value from the hypercall, 0 in case of success
           otherwise, an error code.
**/
INTN
EFIAPI
XenHypercallEventChannelOp (
  IN     INTN  Operation,
  IN OUT VOID  *Arguments
  );

INTN
EFIAPI
XenHypercallSchedOp (
  IN     INTN  Operation,
  IN OUT VOID  *Arguments
  );

#endif
