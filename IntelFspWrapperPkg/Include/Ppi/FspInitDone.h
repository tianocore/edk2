/** @file
  Provides the services to return FSP hob list.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
