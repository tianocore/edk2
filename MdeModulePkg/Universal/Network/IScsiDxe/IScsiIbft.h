/** @file
  Some extra definitions for iBFT

Copyright (c) 2004 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  IScsiIbft.h

Abstract:

  Some extra definitions for iBFT.

**/

#ifndef _ISCSI_IBFT_H_
#define _ISCSI_IBFT_H_

#include <IndustryStandard/IScsiBootFirmwareTable.h>
#include <Protocol/AcpiSupport.h>
#include <Protocol/PciIo.h>

#define IBFT_TABLE_VAR_NAME L"iBFT"
#define IBFT_MAX_SIZE       4096
#define IBFT_HEAP_OFFSET    2048

#define IBFT_ROUNDUP(size)  NET_ROUNDUP ((size), EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_STRUCTURE_ALIGNMENT)

/**
  Publish and remove the iSCSI Boot Firmware Table according to the iSCSI
  session status.

  @param  None.

  @retval None.

**/
VOID
IScsiPublishIbft (
  IN VOID
  );

#endif
