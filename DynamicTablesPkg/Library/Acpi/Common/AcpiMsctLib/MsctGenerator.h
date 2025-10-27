/** @file
  ACPI Maximum System Characteristics Table (MSCT) Generator header file.

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MSCT_GENERATOR_H_
#define MSCT_GENERATOR_H_

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

/** Get architecture clock domain information.

  @param [in]      CfgMgrProtocol       Pointer to the Configuration Manager
                                        Protocol.
  @param [out]     MaxDomainDomain      Pointer to the maximum clock domain
                                        information structures.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
**/
EFI_STATUS
EFIAPI
GetArchClockDomainInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       UINT32                                        *MaxClockDomain
  );

#endif // MSCT_GENERATOR_H_
