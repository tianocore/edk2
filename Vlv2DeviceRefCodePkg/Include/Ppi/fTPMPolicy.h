/*++

  Copyright (c) 2004  - 2015, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent
                                                                                   
--*/

#ifndef _SEC_FTPM_POLICY_PPI_H_
#define _SEC_FTPM_POLICY_PPI_H_

#define SEC_FTPM_POLICY_PPI_GUID \
  { \
    0x4fd1ba49, 0x8f90, 0x471a, 0xa2, 0xc9, 0x17, 0x3c, 0x7a, 0x73, 0x2f, 0xd0 \
  }

extern EFI_GUID  gSeCfTPMPolicyPpiGuid;

//
// PPI definition
//
typedef struct SEC_FTPM_POLICY_PPI {
  BOOLEAN                 fTPMEnable;
} SEC_FTPM_POLICY_PPI;

#endif
