/** @file
*
*  Copyright (c) 2025, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef SMBIOS_SMC_LIB_H_
#define SMBIOS_SMC_LIB_H_

/** Returns the SOC ID, formatted for the SMBIOS Type 4 Processor ID field.

    @param Processor ID.

    @return 0 on success
    @return EFI_UNSUPPORTED if SMCCC_ARCH_SOC_ID is not implemented
**/
UINT64
SmbiosSmcGetSocId (
  UINT64  *ProcessorId
  );

#endif // SMBIOS_SMC_LIB_H_
