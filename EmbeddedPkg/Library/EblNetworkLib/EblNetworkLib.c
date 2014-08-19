/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/SimpleNetwork.h>
#include <Protocol/PxeBaseCode.h>


BOOLEAN   gUseIpv6 = FALSE;

EFI_STATUS
EFIAPI
EblGetCurrentIpAddress (
  IN OUT   EFI_IP_ADDRESS *Ip
  )
{
  EFI_STATUS                  Status;
  EFI_PXE_BASE_CODE_PROTOCOL  *Pxe;

  Status = gBS->LocateProtocol (&gEfiPxeBaseCodeProtocolGuid, NULL, (VOID **)&Pxe);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Status = Pxe->Start (Pxe, gUseIpv6);
  if (EFI_ERROR(Status) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  CopyMem (Ip, &Pxe->Mode->StationIp, sizeof (EFI_IP_ADDRESS));

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
EblGetCurrentMacAddress (
  IN OUT  EFI_MAC_ADDRESS *Mac
  )
{
  EFI_STATUS                    Status;
  EFI_SIMPLE_NETWORK_PROTOCOL   *SimpleNet;

  Status = gBS->LocateProtocol (&gEfiSimpleNetworkProtocolGuid, NULL, (VOID **)&SimpleNet);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  CopyMem (Mac, SimpleNet->Mode->CurrentAddress.Addr, sizeof (EFI_MAC_ADDRESS));
  return Status;
}


CHAR8 *
EFIAPI
EblLoadFileBootTypeString (
  IN  EFI_HANDLE Handle
  )
{
  EFI_STATUS    Status;
  VOID          *NullPtr;

  Status = gBS->HandleProtocol (Handle, &gEfiPxeBaseCodeProtocolGuid, &NullPtr);
  if (!EFI_ERROR (Status)) {
    return "EFI PXE Network Boot";
  }

  return "";
}

EFI_STATUS
EFIAPI
EblPerformDHCP (
  IN  BOOLEAN  SortOffers
  )
{
  EFI_STATUS                  Status;
  EFI_PXE_BASE_CODE_PROTOCOL  *Pxe;

  Status = gBS->LocateProtocol (&gEfiPxeBaseCodeProtocolGuid, NULL, (VOID **)&Pxe);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Status = Pxe->Start (Pxe, gUseIpv6);
  if (EFI_ERROR(Status) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  Status = Pxe->Dhcp(Pxe, TRUE);
  return Status;
}


EFI_STATUS
EFIAPI
EblSetStationIp (
  IN EFI_IP_ADDRESS *NewStationIp,  OPTIONAL
  IN EFI_IP_ADDRESS *NewSubnetMask  OPTIONAL
  )
{
  EFI_STATUS                  Status;
  EFI_PXE_BASE_CODE_PROTOCOL  *Pxe;

  Status = gBS->LocateProtocol (&gEfiPxeBaseCodeProtocolGuid, NULL, (VOID **)&Pxe);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Status = Pxe->Start (Pxe, gUseIpv6);
  if (EFI_ERROR(Status) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  Status = Pxe->SetStationIp (Pxe, NewStationIp, NewSubnetMask);
  return Status;
}


EFI_STATUS
EFIAPI
EblMtftp (
  IN EFI_PXE_BASE_CODE_TFTP_OPCODE             Operation,
  IN OUT VOID                                  *BufferPtr OPTIONAL,
  IN BOOLEAN                                   Overwrite,
  IN OUT UINT64                                *BufferSize,
  IN UINTN                                     *BlockSize OPTIONAL,
  IN EFI_IP_ADDRESS                            *ServerIp,
  IN UINT8                                     *Filename  OPTIONAL,
  IN EFI_PXE_BASE_CODE_MTFTP_INFO              *Info      OPTIONAL,
  IN BOOLEAN                                   DontUseBuffer
  )
{
  EFI_STATUS                  Status;
  EFI_PXE_BASE_CODE_PROTOCOL  *Pxe;

  Status = gBS->LocateProtocol (&gEfiPxeBaseCodeProtocolGuid, NULL, (VOID **)&Pxe);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Status = Pxe->Mtftp (
                  Pxe,
                  Operation,
                  BufferPtr,
                  Overwrite,
                  BufferSize,
                  BlockSize,
                  ServerIp,
                  Filename,
                  Info,
                  DontUseBuffer
                  );
  return Status;
}

