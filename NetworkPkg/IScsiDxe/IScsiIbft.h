/** @file
  Some extra definitions for iBFT.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ISCSI_IBFT_H_
#define _ISCSI_IBFT_H_

#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/IScsiBootFirmwareTable.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/PciIo.h>

#define IBFT_TABLE_VAR_NAME  L"iBFT"
#define IBFT_MAX_SIZE        4096
#define IBFT_HEAP_OFFSET     2048

#define IBFT_ROUNDUP(size)  NET_ROUNDUP ((size), EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_STRUCTURE_ALIGNMENT)

/**
  Publish and remove the iSCSI Boot Firmware Table according to the iSCSI
  session status.

**/
VOID
IScsiPublishIbft (
  IN VOID
  );

#endif
