/**@file

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PeiSocketFileIo.c

Abstract:

  Windows Socket IO protocol implementation

  * Other names and brands may be claimed as the property of others.

**/


#include "Meta.h"

STATIC
EFI_STATUS
AssignWindowsFunctionPointers (
  IN WINDOWS_SOCKET_IO_CONTEXT *Context
  )
{
  EFI_STATUS                Status;
  PEI_NT_THUNK_PPI          *NtThunkPpi;
  EFI_WIN_NT_THUNK_PROTOCOL *NtThunkProtocol;

  TRACE_ENTER ();

  //
  // Check parameters for sanity
  //

  CHECK_NULL_RETURN_OR_ASSERT (Context, EFI_INVALID_PARAMETER, "Passed a bad parameter\n");
  
  //
  // Get the thunk PPI
  //

  Status = PeiServicesLocatePpi (&gPeiNtThunkPpiGuid,
                                  0,
                                  NULL,
                                  (VOID**)&NtThunkPpi);
  if (EFI_ERROR(Status)) {
    EFI_ERROR_RETURN_OR_ASSERT (Status, "PeiServicesLocatePpi failed, Status = %r\n", Status);
  }

  //
  // NtThunk is defined as a protocol, not a PPI so cast to a
  // protocol. This is fine as NtThunk is published by SecMain.exe and
  // is available in both PEI and DXE.
  //

  NtThunkProtocol = (EFI_WIN_NT_THUNK_PROTOCOL*)NtThunkPpi->NtThunk();


  //
  // Load the DLL
  //
  
  Context->WinsockHandle = NtThunkProtocol->LoadLibraryEx (L"wsock32.dll", NULL, 0);
  if (Context->WinsockHandle == NULL) {
    CHECK_NULL_RETURN_OR_ASSERT (Context->WinsockHandle, EFI_NOT_FOUND, "Context->WinsockHandle == NULL\n");
  }

  //
  // Set our function pointers
  //

  Context->WSAStartupFunctionPtr = (Win_WSAStartup *)(UINTN)NtThunkProtocol->GetProcAddress (Context->WinsockHandle, "WSAStartup");
  CHECK_NULL_RETURN_OR_ASSERT (Context->WSAStartupFunctionPtr, EFI_NOT_FOUND, "Context->WSAStartupFunctionPtr == NULL\n");

  Context->WSACleanupFunctionPtr = (Win_WSACleanup *)(UINTN)NtThunkProtocol->GetProcAddress (Context->WinsockHandle, "WSACleanup");
  CHECK_NULL_RETURN_OR_ASSERT (Context->WSACleanupFunctionPtr, EFI_NOT_FOUND, "Context->WSACleanupFunctionPtr == NULL\n");

  Context->WSAGetLastErrorFunctionPtr = (Win_WSAGetLastError *)(UINTN)NtThunkProtocol->GetProcAddress (Context->WinsockHandle, "WSAGetLastError");
  CHECK_NULL_RETURN_OR_ASSERT (Context->WSAGetLastErrorFunctionPtr, EFI_NOT_FOUND, "Context->WSAGetLastErrorFunctionPtr == NULL\n");

  Context->socketFunctionPtr = (Win_socket *)(UINTN)NtThunkProtocol->GetProcAddress (Context->WinsockHandle, "socket");
  CHECK_NULL_RETURN_OR_ASSERT (Context->socketFunctionPtr, EFI_NOT_FOUND, "Context->socketFunctionPtr == NULL\n");

  Context->connectFunctionPtr = (Win_connect *)(UINTN)NtThunkProtocol->GetProcAddress (Context->WinsockHandle, "connect");
  CHECK_NULL_RETURN_OR_ASSERT (Context->connectFunctionPtr, EFI_NOT_FOUND, "Context->connectFunctionPtr == NULL\n");

  Context->setsockoptFunctionPtr = (Win_setsockopt *)(UINTN)NtThunkProtocol->GetProcAddress (Context->WinsockHandle, "setsockopt");
  CHECK_NULL_RETURN_OR_ASSERT (Context->setsockoptFunctionPtr, EFI_NOT_FOUND, "Context->setsockoptFunctionPtr == NULL\n");

  Context->closesocketFunctionPtr = (Win_closesocket *)(UINTN)NtThunkProtocol->GetProcAddress (Context->WinsockHandle, "closesocket");
  CHECK_NULL_RETURN_OR_ASSERT (Context->closesocketFunctionPtr, EFI_NOT_FOUND, "Context->closesocketFunctionPtr == NULL\n");

  Context->sendFunctionPtr = (Win_send *)(UINTN)NtThunkProtocol->GetProcAddress (Context->WinsockHandle, "send");
  CHECK_NULL_RETURN_OR_ASSERT (Context->sendFunctionPtr, EFI_NOT_FOUND, "Context->sendFunctionPtr == NULL\n");

  Context->recvFunctionPtr = (Win_recv *)(UINTN)NtThunkProtocol->GetProcAddress (Context->WinsockHandle, "recv");
  CHECK_NULL_RETURN_OR_ASSERT (Context->recvFunctionPtr, EFI_NOT_FOUND, "Context->recvFunctionPtr == NULL\n");

  Context->inet_addrFunctionPtr = (Win_inet_addr *)(UINTN)NtThunkProtocol->GetProcAddress (Context->WinsockHandle, "inet_addr");
  CHECK_NULL_RETURN_OR_ASSERT (Context->inet_addrFunctionPtr, EFI_NOT_FOUND, "Context->inet_addrFunctionPtr == NULL\n");

  Context->htonsFunctionPtr = (Win_htons *)(UINTN)NtThunkProtocol->GetProcAddress (Context->WinsockHandle, "htons");
  CHECK_NULL_RETURN_OR_ASSERT (Context->htonsFunctionPtr, EFI_NOT_FOUND, "Context->htonsFunctionPtr == NULL\n");

  Context->htonlFunctionPtr = (Win_htonl *)(UINTN)NtThunkProtocol->GetProcAddress (Context->WinsockHandle, "htonl");
  CHECK_NULL_RETURN_OR_ASSERT (Context->htonlFunctionPtr, EFI_NOT_FOUND, "Context->htonlFunctionPtr == NULL\n");

  Context->ntohsFunctionPtr = (Win_ntohs *)(UINTN)NtThunkProtocol->GetProcAddress (Context->WinsockHandle, "ntohs");
  CHECK_NULL_RETURN_OR_ASSERT (Context->ntohsFunctionPtr, EFI_NOT_FOUND, "Context->ntohsFunctionPtr == NULL\n");

  Context->ntohlFunctionPtr = (Win_ntohl *)(UINTN)NtThunkProtocol->GetProcAddress (Context->WinsockHandle, "ntohl");
  CHECK_NULL_RETURN_OR_ASSERT (Context->ntohlFunctionPtr, EFI_NOT_FOUND, "Context->ntohlFunctionPtr == NULL\n");

  Context->gethostbynameFunctionPtr = (Win_gethostbyname *)(UINTN)NtThunkProtocol->GetProcAddress (Context->WinsockHandle, "gethostbyname");
  CHECK_NULL_RETURN_OR_ASSERT (Context->gethostbynameFunctionPtr, EFI_NOT_FOUND, "Context->gethostbynameFunctionPtr == NULL\n");

  Context->gethostnameFunctionPtr = (Win_gethostname *)(UINTN)NtThunkProtocol->GetProcAddress (Context->WinsockHandle, "gethostname");
  CHECK_NULL_RETURN_OR_ASSERT (Context->gethostnameFunctionPtr, EFI_NOT_FOUND, "Context->gethostnameFunctionPtr == NULL\n");

  Context->inet_ntoaFunctionPtr = (Win_inet_ntoa *)(UINTN)NtThunkProtocol->GetProcAddress (Context->WinsockHandle, "inet_ntoa");
  CHECK_NULL_RETURN_OR_ASSERT (Context->inet_ntoaFunctionPtr, EFI_NOT_FOUND, "Context->inet_ntoaFunctionPtr == NULL\n");

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // AssignWindowsFunctionPointers

EFI_STATUS
EFIAPI
PeiWindowsSocketIoEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS Status;
  PEI_NT_THUNK_PPI *NtThunkPpi;
  WINDOWS_SOCKET_IO_CONTEXT *Context;
  HOST_OS_SOCKET_IO_PPI *HostOsSocketIoPpi;

  TRACE_ENTER ();

  //
  // Get the thunk ppi
  //

  Status = PeiServicesLocatePpi (&gPeiNtThunkPpiGuid,
                                  0,
                                  NULL,
                                  (VOID **)&NtThunkPpi);
  if (EFI_ERROR(Status)) {
    EFI_ERROR_RETURN_OR_ASSERT (Status, "PeiServicesLocatePpi failed, Status = %r\n", Status);
  }

  //
  // Allocate memory for our context.
  //

  Status = PeiServicesAllocatePool (sizeof (WINDOWS_SOCKET_IO_CONTEXT), &Context);
  if (EFI_ERROR(Status)) {
    EFI_ERROR_RETURN_OR_ASSERT (Status, "PeiServicesAllocatePool failed, Status = %r\n", Status);
  }

  //
  // Initialize our context signature.
  //

  Context->Signature = WINDOWS_SOCKET_IO_CONTEXT_SIGNATURE;

  //
  // Load the DLL and assign the function pointers. We do this once
  // at startup for performance reasons.
  // 

  Status = AssignWindowsFunctionPointers (Context);
  if (EFI_ERROR (Status)) {
    EFI_ERROR_RETURN_OR_ASSERT (Status, "AssignWindowsFunctionPointers failed, Status = %r\n", Status);
  }

  HostOsSocketIoPpi = &Context->HostOsSocketIoPpi;

  //
  // Assign the protocol members
  //

  HostOsSocketIoPpi->Revision                 = HOST_OS_SOCKET_IO_PPI_REVISION;

  //
  // Assign our function pointers
  //

  HostOsSocketIoPpi->HostOsNetworkInit      = WindowsWSAStartup;
  HostOsSocketIoPpi->HostOsNetworkTeardown  = WindowsWSACleanup;
  HostOsSocketIoPpi->HostOsLastError        = WindowsWSAGetLastError;
  HostOsSocketIoPpi->HostOsCreateSocket     = WindowsSocket;
  HostOsSocketIoPpi->HostOsConnect          = WindowsConnect;
  HostOsSocketIoPpi->HostOsCloseSocket      = WindowsCloseSocket;
  HostOsSocketIoPpi->HostOsSend             = WindowsSend;
  HostOsSocketIoPpi->HostOsRecv             = WindowsRecv;
  HostOsSocketIoPpi->HostOsInetAddr         = WindowsInetAddr;
  HostOsSocketIoPpi->HostOsHtons            = WindowsHtons;
  HostOsSocketIoPpi->HostOsHtonl            = WindowsHtonl;
  HostOsSocketIoPpi->HostOsNtohs            = WindowsNtohs;
  HostOsSocketIoPpi->HostOsNtohl            = WindowsNtohl;
  HostOsSocketIoPpi->HostOsGetHostByName    = WindowsGetHostByName;
  HostOsSocketIoPpi->HostOsGetHostName      = WindowsGetHostName;
  HostOsSocketIoPpi->HostOsInetNtoa         = WindowsInet_Ntoa; 

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

} // PeiWindowsSocketIoEntry


