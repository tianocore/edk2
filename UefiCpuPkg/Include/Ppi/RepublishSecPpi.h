/** @file
  This file declares the Republish SEC PPI.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __REPUBLISH_SEC_PPI_H__
#define __REPUBLISH_SEC_PPI_H__

#include <Pi/PiPeiCis.h>

#define REPUBLISH_SEC_PPI_PPI_GUID \
  { \
    0x27a71b1e, 0x73ee, 0x43d6, { 0xac, 0xe3, 0x52, 0x1a, 0x2d, 0xc5, 0xd0, 0x92 } \
  }

typedef struct _REPUBLISH_SEC_PPI_PPI REPUBLISH_SEC_PPI_PPI;

/**
  This interface re-installs PPIs installed in SecCore from a post-memory PEIM.

  This is to allow a platform that may not support relocation of SecCore to update the PPI instance to a post-memory
  copy from a PEIM that has been shadowed to permanent memory.

  @retval EFI_SUCCESS    The SecCore PPIs were re-installed successfully.
  @retval Others         An error occurred re-installing the SecCore PPIs.

**/
typedef
EFI_STATUS
(EFIAPI *REPUBLISH_SEC_PPI_REPUBLISH_SEC_PPIS)(
  VOID
  );

///
///
///
struct _REPUBLISH_SEC_PPI_PPI {
  REPUBLISH_SEC_PPI_REPUBLISH_SEC_PPIS  RepublishSecPpis;
};

extern EFI_GUID gRepublishSecPpiPpiGuid;

#endif
