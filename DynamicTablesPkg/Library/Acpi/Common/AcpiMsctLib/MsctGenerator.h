/** @file
  ACPI Maximum System Characteristics Table (MSCT) Generator header file.

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

/** Get architecture processor domain information.

  @param [in]      CfgMgrProtocol       Pointer to the Configuration Manager
                                        Protocol.
  @param [out]     ProcDomainInfo       Pointer to the processor domain information.
  @param [out]     ProcDomainInfoCount  Pointer to the count of processor domain
                                        information structures.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
  @retval Other                 Other EFI_STATUS error from called functions.
**/
EFI_STATUS
EFIAPI
GetArchProcessorDomainInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       UINT32                                        **ProcDomainInfo,
  OUT       UINT32                                        *ProcDomainInfoCount
  );

/** Get the count of unique architecture clock domains.

  Enumerates all CPU interfaces and returns the number of unique clock
  proximity domain IDs in the system.

  @param [in]      CfgMgrProtocol    Pointer to the Configuration Manager
                                     Protocol.
  @param [out]     ClockDomainCount  On success, the cardinal count of unique
                                     clock domains.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
  @retval Other                 Other EFI_STATUS error from called functions.
**/
EFI_STATUS
EFIAPI
GetArchClockDomainInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       UINT32                                        *ClockDomainCount
  );
