/*++

Copyright (c)  2007 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  IScsiIbft.h

Abstract:

  Some extra definitions for iBFT.

--*/

#ifndef _ISCSI_IBFT_H_
#define _ISCSI_IBFT_H_

#include <industrystandard/IScsiBootFirmwareTable.h>
#include <protocol/AcpiSupport.h>
#include <protocol/PciIo.h>

#define IBFT_TABLE_VAR_NAME L"iBFT"
#define IBFT_MAX_SIZE       4096
#define IBFT_HEAP_OFFSET    2048

#define IBFT_ROUNDUP(size)  NET_ROUNDUP ((size), EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_STRUCTURE_ALIGNMENT)

VOID
IScsiPublishIbft (
  IN VOID
  );

#endif
