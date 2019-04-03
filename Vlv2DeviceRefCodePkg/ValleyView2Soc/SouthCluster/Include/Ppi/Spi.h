/**
**/
/**

Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent



  @file
  Spi.h

  @brief
  This file defines the EFI SPI PPI which implements the
  Intel(R) PCH SPI Host Controller Compatibility Interface.

**/
#ifndef _PEI_SPI_H_
#define _PEI_SPI_H_


#include <Protocol/Spi.h>


//
#define PEI_SPI_PPI_GUID \
  { \
    0xa38c6898, 0x2b5c, 0x4ff6, 0x93, 0x26, 0x2e, 0x63, 0x21, 0x2e, 0x56, 0xc2 \
  }
// Extern the GUID for PPI users.
//
extern EFI_GUID           gPeiSpiPpiGuid;

///
/// Reuse the EFI_SPI_PROTOCOL definitions
/// This is possible becaues the PPI implementation does not rely on a PeiService pointer,
/// as it uses EDKII Glue Lib to do IO accesses
///
typedef EFI_SPI_PROTOCOL  PEI_SPI_PPI;

#endif
