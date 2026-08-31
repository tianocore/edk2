/** @file
  Default (stub) arch-specific ACPI MSCT Generator implementation.

  Used for architectures that do not provide processor or clock domain
  information via the Configuration Manager (e.g. RISCV64, LOONGARCH64).
  Both functions return EFI_NOT_FOUND so that the common generator
  gracefully omits the arch-specific topology data from the MSCT.

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include "MsctGenerator.h"

/** Get architecture processor domain information.

  This stub implementation returns EFI_NOT_FOUND for architectures that
  do not provide processor proximity domain data via the Configuration Manager.

  @param [in]      CfgMgrProtocol       Pointer to the Configuration Manager
                                        Protocol.
  @param [out]     ProcDomainInfo       Pointer to the processor domain information.
  @param [out]     ProcDomainInfoCount  Pointer to the count of processor domain
                                        information structures.

  @retval EFI_NOT_FOUND  Not supported on this architecture.
**/
EFI_STATUS
EFIAPI
GetArchProcessorDomainInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       UINT32                                        **ProcDomainInfo,
  OUT       UINT32                                        *ProcDomainInfoCount
  )
{
  return EFI_NOT_FOUND;
}

/** Get the count of unique architecture clock domains.

  This stub implementation returns EFI_NOT_FOUND for architectures that
  do not provide clock domain data via the Configuration Manager.

  @param [in]      CfgMgrProtocol    Pointer to the Configuration Manager
                                     Protocol.
  @param [out]     ClockDomainCount  On success, the cardinal count of unique
                                     clock domains.

  @retval EFI_NOT_FOUND  Not supported on this architecture.
**/
EFI_STATUS
EFIAPI
GetArchClockDomainInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       UINT32                                        *ClockDomainCount
  )
{
  return EFI_NOT_FOUND;
}
