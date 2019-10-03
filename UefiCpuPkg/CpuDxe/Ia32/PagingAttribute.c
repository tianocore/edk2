/** @file
  Return Paging attribute.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuPageTable.h"


/**
  Get paging details.

  @param  PagingContextData      The paging context.
  @param  PageTableBase          Return PageTableBase field.
  @param  Attributes             Return Attributes field.

**/
VOID
GetPagingDetails (
  IN  PAGE_TABLE_LIB_PAGING_CONTEXT_DATA *PagingContextData,
  OUT UINTN                              **PageTableBase     OPTIONAL,
  OUT UINT32                             **Attributes        OPTIONAL
  )
{
  if (PageTableBase != NULL) {
    *PageTableBase = &PagingContextData->Ia32.PageTableBase;
  }
  if (Attributes != NULL) {
    *Attributes = &PagingContextData->Ia32.Attributes;
  }
}

