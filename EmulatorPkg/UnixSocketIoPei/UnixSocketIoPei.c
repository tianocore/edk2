/**@file

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PeiUnixSocketFileIo.c

Abstract:

  Unix Socket IO protocol implementation

  * Other names and brands may be claimed as the property of others.

**/


#include "UnixSocketIoPei.h"

/**
  Assign function pointers to various socket related
  functions in libc.

  @param [in out] Context   Ptr to the driver context.

  @retval  EFI_SUCCESS      Ptrs assigned.
  @retval  !EFI_SUCCESS     Ptrs not assigned.

**/

EFI_STATUS
AssignFunctionPointers (
  IN OUT UNIX_SOCKET_IO_CONTEXT *Context
  )
{
  EFI_STATUS                Status;
  EMU_DYNAMIC_LOAD_PROTOCOL *DynamicLoadPpi;

  TRACE_ENTER ();

  //
  // Check parameters for sanity
  //

  CHECK_NULL_RETURN_OR_ASSERT (Context, EFI_INVALID_PARAMETER, "Passed a bad parameter\n");
  
  //
  // Get the dynamic load ppi
  //

  Status = PeiServicesLocatePpi (&gEmuDynamicLoadProtocolGuid,
                                  0,
                                  NULL,
                                  (VOID **)&DynamicLoadPpi);
  if (EFI_ERROR(Status)) {
    EFI_ERROR_RETURN_OR_ASSERT (Status, "PeiServicesLocatePpi failed, Status = %r\n", Status);
  }

  Context->LibraryHandle = DynamicLoadPpi->Dlopen ("libc.so.6", 1);
  CHECK_NULL_RETURN_OR_ASSERT (Context->LibraryHandle, EFI_INVALID_PARAMETER, "Context->LibraryHandle is null\n");

  //
  // Set our function pointers
  //

  Context->SocketFn = DynamicLoadPpi->Dlsym (Context->LibraryHandle, "socket");
  CHECK_NULL_RETURN_OR_ASSERT (Context->SocketFn, EFI_NOT_FOUND, "Context->SocketFn == NULL\n");

  Context->ConnectFn = DynamicLoadPpi->Dlsym (Context->LibraryHandle, "connect");
  CHECK_NULL_RETURN_OR_ASSERT (Context->ConnectFn, EFI_NOT_FOUND, "Context->ConnectFn == NULL\n");

  Context->CloseFn = DynamicLoadPpi->Dlsym (Context->LibraryHandle, "close");
  CHECK_NULL_RETURN_OR_ASSERT (Context->CloseFn, EFI_NOT_FOUND, "Context->CloseFn == NULL\n");

  Context->SendFn = DynamicLoadPpi->Dlsym (Context->LibraryHandle, "send");
  CHECK_NULL_RETURN_OR_ASSERT (Context->SendFn, EFI_NOT_FOUND, "Context->SendFn == NULL\n");

  Context->RecvFn = DynamicLoadPpi->Dlsym (Context->LibraryHandle, "recv");
  CHECK_NULL_RETURN_OR_ASSERT (Context->RecvFn, EFI_NOT_FOUND, "Context->RecvFn == NULL\n");

  Context->InetAddrFn = DynamicLoadPpi->Dlsym (Context->LibraryHandle, "inet_addr");
  CHECK_NULL_RETURN_OR_ASSERT (Context->InetAddrFn, EFI_NOT_FOUND, "Context->InetAddrFn == NULL\n");

  Context->HtonsFn = DynamicLoadPpi->Dlsym (Context->LibraryHandle, "htons");
  CHECK_NULL_RETURN_OR_ASSERT (Context->HtonsFn, EFI_NOT_FOUND, "Context->HtonsFn == NULL\n");

  Context->HtonlFn = DynamicLoadPpi->Dlsym (Context->LibraryHandle, "htonl");
  CHECK_NULL_RETURN_OR_ASSERT (Context->HtonlFn, EFI_NOT_FOUND, "Context->HtonlFn == NULL\n");

  Context->NtohsFn = DynamicLoadPpi->Dlsym (Context->LibraryHandle, "ntohs");
  CHECK_NULL_RETURN_OR_ASSERT (Context->NtohsFn, EFI_NOT_FOUND, "Context->NtohsFn == NULL\n");

  Context->NtohlFn = DynamicLoadPpi->Dlsym (Context->LibraryHandle, "ntohl");
  CHECK_NULL_RETURN_OR_ASSERT (Context->NtohlFn, EFI_NOT_FOUND, "Context->NtohlFn == NULL\n");

  Context->GethostbynameFn = DynamicLoadPpi->Dlsym (Context->LibraryHandle, "gethostbyname");
  CHECK_NULL_RETURN_OR_ASSERT (Context->GethostbynameFn, EFI_NOT_FOUND, "Context->GethostbynameFn == NULL\n");

  Context->GethostnameFn = DynamicLoadPpi->Dlsym (Context->LibraryHandle, "gethostname");
  CHECK_NULL_RETURN_OR_ASSERT (Context->GethostnameFn, EFI_NOT_FOUND, "Context->GethostnameFn == NULL\n");

  Context->errno = DynamicLoadPpi->Dlsym (Context->LibraryHandle, "errno");
  CHECK_NULL_RETURN_OR_ASSERT (Context->SocketFn, EFI_NOT_FOUND, "errno == NULL\n");

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // AssignFunctionPointers

/**
  Entry point to the Socket IO driver. Initializes and 
  installs the socket PPI if successful.

  @param [in] FileHandle    Handle of the file being invoked.
  @param [in] PeiServices   An indirect pointer to the PEI Services Table published by the PEI Foundation.

  @retval  EFI_SUCCESS      The driver initialized normally.
  @retval  !EFI_SUCCESS     The driver failed to initialize normally.

**/

EFI_STATUS
EFIAPI
PeiUnixSocketIoEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS Status;

  UNIX_SOCKET_IO_CONTEXT *Context;
  HOST_OS_SOCKET_IO_PPI *HostOsSocketIoPpi;

  TRACE_ENTER ();

  //
  // Allocate memory for our context.
  //

  Status = PeiServicesAllocatePool (sizeof (UNIX_SOCKET_IO_CONTEXT), &Context);
  if (EFI_ERROR(Status)) {
    EFI_ERROR_RETURN_OR_ASSERT (Status, "PeiServicesAllocatePool failed, Status = %r\n", Status);
  }

  //
  // Initialize our context signature.
  //

  Context->Signature = UNIX_SOCKET_IO_CONTEXT_SIGNATURE;

  //
  // Assign the function pointers. We do this once
  // at startup for performance reasons.
  // 

  Status = AssignFunctionPointers (Context);
  if (EFI_ERROR (Status)) {
    EFI_ERROR_RETURN_OR_ASSERT (Status, "AssignFunctionPointers failed, Status = %r\n", Status);
  }

  HostOsSocketIoPpi = &Context->HostOsSocketIoPpi;

  //
  // Assign the protocol members
  //

  HostOsSocketIoPpi->Revision                 = HOST_OS_SOCKET_IO_PPI_REVISION;

  //
  // Assign our function pointers
  //

  HostOsSocketIoPpi->HostOsNetworkInit      = NetworkInit;
  HostOsSocketIoPpi->HostOsNetworkTeardown  = NetworkTeardown;
  HostOsSocketIoPpi->HostOsLastError        = GetLastError;
  HostOsSocketIoPpi->HostOsCreateSocket     = CreateSocket;
  HostOsSocketIoPpi->HostOsConnect          = Connect;
  HostOsSocketIoPpi->HostOsConnectUnix      = ConnectUnix;
  HostOsSocketIoPpi->HostOsCloseSocket      = Close;
  HostOsSocketIoPpi->HostOsSend             = Send;
  HostOsSocketIoPpi->HostOsRecv             = Recv;
  HostOsSocketIoPpi->HostOsInetAddr         = InetAddr;
  HostOsSocketIoPpi->HostOsHtons            = Htons;
  HostOsSocketIoPpi->HostOsHtonl            = Htonl;
  HostOsSocketIoPpi->HostOsNtohs            = Ntohs;
  HostOsSocketIoPpi->HostOsNtohl            = Ntohl;
  HostOsSocketIoPpi->HostOsGetHostByName    = GetHostByName;
  HostOsSocketIoPpi->HostOsGetHostName      = GetHostName;

  //
  // Fill in the PPI Descriptor.
  //

  Context->PpiList.Flags = EFI_PEI_PPI_DESCRIPTOR_PPI  | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  Context->PpiList.Guid  = &gHostOsSocketIoPpiGuid;
  Context->PpiList.Ppi   = &Context->HostOsSocketIoPpi;

  //
  // Install the PPI.
  //

  Status = PeiServicesInstallPpi (&(Context->PpiList));
  if (EFI_ERROR (Status)) {
    EFI_ERROR_RETURN_OR_ASSERT (Status, "PeiServicesInstallPpi failed, Status = %r\n", Status);
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // PeiUnixSocketIoEntry


