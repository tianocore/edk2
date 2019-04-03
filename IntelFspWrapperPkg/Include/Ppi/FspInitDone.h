/** @file
  Provides the services to return FSP hob list.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FSP_INIT_DONE_H_
#define _FSP_INIT_DONE_H_

typedef struct _FSP_INIT_DONE_PPI  FSP_INIT_DONE_PPI;

/**
  Return Hob list produced by FSP.

  @param[in]  PeiServices  The pointer to the PEI Services Table.
  @param[in]  This         The pointer to this instance of this PPI.
  @param[out] FspHobList   The pointer to Hob list produced by FSP.

  @return EFI_SUCCESS FReturn Hob list produced by FSP successfully.
**/
typedef
EFI_STATUS
(EFIAPI *FSP_INIT_DONE_GET_FSP_HOB_LIST)(
  IN  CONST EFI_PEI_SERVICES         **PeiServices,
  IN  FSP_INIT_DONE_PPI              *This,
  OUT VOID                           **FspHobList
  );

struct _FSP_INIT_DONE_PPI {
  FSP_INIT_DONE_GET_FSP_HOB_LIST      GetFspHobList;
};

extern EFI_GUID gFspInitDonePpiGuid;

#endif
