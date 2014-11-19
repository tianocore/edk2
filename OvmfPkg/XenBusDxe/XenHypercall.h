/** @file
  Functions declarations to make Xen hypercalls.

  Copyright (C) 2014, Citrix Ltd.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __XENBUS_DXE_HYPERCALL_H__
#define __XENBUS_DXE_HYPERCALL_H__

/**
  This function will put the two arguments in the right place (registers) and
  call HypercallAddr, which correspond to an entry in the hypercall pages.

  @param HypercallAddr  A memory address where the hypercall to call is.
  @param Arg1           First argument.
  @param Arg2           Second argument.

  @return   Return 0 if success otherwise it return an errno.
**/
INTN
EFIAPI
XenHypercall2 (
  IN     VOID *HypercallAddr,
  IN OUT INTN Arg1,
  IN OUT INTN Arg2
  );

/**
  Get the page where all hypercall are from the XenInfo hob.

  @param Dev    A XENBUS_DEVICE instance.

  @retval EFI_NOT_FOUND   hyperpage could not be found.
  @retval EFI_SUCCESS     Successfully retrieve the hyperpage pointer.
**/
EFI_STATUS
XenHyperpageInit (
  XENBUS_DEVICE *Dev
  );

/**
  Return the value of the HVM parameter Index.

  @param Dev    A XENBUS_DEVICE instance.
  @param Index  The parameter to get, e.g. HVM_PARAM_STORE_EVTCHN.

  @return   The value of the asked parameter or 0 in case of error.
**/
UINT64
XenHypercallHvmGetParam (
  XENBUS_DEVICE *Dev,
  UINT32 Index
  );

/**
  Hypercall to do different operation on the memory.

  @param Dev        A XENBUS_DEVICE instance.
  @param Operation  The operation number, e.g. XENMEM_add_to_physmap.
  @param Arguments  The arguments associated to the operation.

  @return  Return the return value from the hypercall, 0 in case of success
           otherwise, an error code.
**/
INTN
XenHypercallMemoryOp (
  IN     XENBUS_DEVICE *Dev,
  IN     UINTN Operation,
  IN OUT VOID *Arguments
  );

/**
  Do an operation on the event channels.

  @param Dev        A XENBUS_DEVICE instance.
  @param Operation  The operation number, e.g. EVTCHNOP_send.
  @param Arguments  The argument associated to the operation.

  @return  Return the return value from the hypercall, 0 in case of success
           otherwise, an error code.
**/
INTN
XenHypercallEventChannelOp (
  IN     XENBUS_DEVICE *Dev,
  IN     INTN Operation,
  IN OUT VOID *Arguments
  );

/**
  Map the shared_info_t page into memory.

  @param Dev    A XENBUS_DEVICE instance.

  @retval EFI_SUCCESS     Dev->SharedInfo whill contain a pointer to
                          the shared info page
  @retval EFI_LOAD_ERROR  The shared info page could not be mapped. The
                          hypercall returned an error.
**/
EFI_STATUS
XenGetSharedInfoPage (
  IN OUT XENBUS_DEVICE *Dev
  );

#endif
