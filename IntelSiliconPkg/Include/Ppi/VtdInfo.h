/** @file
  The definition for VTD information PPI.

  This is a lightweight VTd information report in PEI phase.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __VTD_INFO_PPI_H__
#define __VTD_INFO_PPI_H__

#include <IndustryStandard/DmaRemappingReportingTable.h>

#define EDKII_VTD_INFO_PPI_GUID \
    { \
      0x8a59fcb3, 0xf191, 0x400c, { 0x97, 0x67, 0x67, 0xaf, 0x2b, 0x25, 0x68, 0x4a } \
    }

//
// VTD info PPI just use same data structure as DMAR table.
//
// The reported information must include what is needed in PEI phase, e.g.
//   the VTd engine (such as DRHD)
//   the reserved DMA address in PEI for eary graphic (such as RMRR for graphic UMA)
//
// The reported information can be and might be a subset of full DMAR table, e.g.
//   if some data is not avaiable (such as ANDD),
//   if some data is not needed (such as RMRR for legacy USB).
//
typedef EFI_ACPI_DMAR_HEADER EDKII_VTD_INFO_PPI;

extern EFI_GUID gEdkiiVTdInfoPpiGuid;

#endif

