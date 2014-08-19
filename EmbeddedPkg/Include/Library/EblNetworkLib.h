/** @file
  Abstractions for Ebl network accesses.

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EBL_NETWORK_LIB_H__
#define __EBL_NETWORK_LIB_H__

#include <Protocol/PxeBaseCode.h>


EFI_STATUS
EFIAPI
EblGetCurrentIpAddress (
  IN OUT EFI_IP_ADDRESS *Ip
  );

EFI_STATUS
EFIAPI
EblGetCurrentMacAddress (
  IN OUT  EFI_MAC_ADDRESS *Mac
  );

CHAR8 *
EFIAPI
EblLoadFileBootTypeString (
  IN  EFI_HANDLE Handle
  );

EFI_STATUS
EFIAPI
EblPerformDHCP (
  IN  BOOLEAN  SortOffers
  );

EFI_STATUS
EFIAPI
EblSetStationIp (
  IN EFI_IP_ADDRESS *NewStationIp,  OPTIONAL
  IN EFI_IP_ADDRESS *NewSubnetMask  OPTIONAL
  );

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
  );

#endif

