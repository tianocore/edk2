/** @file
  The implementation to go through each entry in IpSecConfig application.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "IpSecConfig.h"
#include "ForEach.h"


/**
  Enumerate all entries in the database to execute specified operations according to datatype.

  @param[in] DataType    The value of EFI_IPSEC_CONFIG_DATA_TYPE.
  @param[in] Routine     The pointer to the function of a specified operation.
  @param[in] Context     The pointer to the context of a function.

  @retval EFI_SUCCESS    Execute specified operation successfully.
**/
EFI_STATUS
ForeachPolicyEntry (
  IN EFI_IPSEC_CONFIG_DATA_TYPE    DataType,
  IN VISIT_POLICY_ENTRY            Routine,
  IN VOID                          *Context
  )
{
  EFI_STATUS                   GetNextStatus;
  EFI_STATUS                   GetDataStatus;
  EFI_IPSEC_CONFIG_SELECTOR    *Selector;
  VOID                         *Data;
  UINTN                        SelectorSize;
  UINTN                        DataSize;
  BOOLEAN                      FirstGetNext;

  FirstGetNext = TRUE;
  SelectorSize = sizeof (EFI_IPSEC_CONFIG_SELECTOR);
  Selector     = AllocateZeroPool (SelectorSize);

  DataSize     = 0;
  Data         = NULL;

  while (TRUE) {
    GetNextStatus = mIpSecConfig->GetNextSelector (
                                    mIpSecConfig,
                                    DataType,
                                    &SelectorSize,
                                    Selector
                                    );
    if (GetNextStatus == EFI_BUFFER_TOO_SMALL) {
      gBS->FreePool (Selector);
      Selector = FirstGetNext ? AllocateZeroPool (SelectorSize) : AllocatePool (SelectorSize);

      GetNextStatus = mIpSecConfig->GetNextSelector (
                                      mIpSecConfig,
                                      DataType,
                                      &SelectorSize,
                                      Selector
                                      );
    }

    if (EFI_ERROR (GetNextStatus)) {
      break;
    }

    FirstGetNext = FALSE;

    GetDataStatus = mIpSecConfig->GetData (
                                    mIpSecConfig,
                                    DataType,
                                    Selector,
                                    &DataSize,
                                    Data
                                    );
    if (GetDataStatus == EFI_BUFFER_TOO_SMALL) {
      if (Data != NULL) {
        gBS->FreePool (Data);
      }

      Data = AllocateZeroPool (DataSize);
      GetDataStatus = mIpSecConfig->GetData (
                                      mIpSecConfig,
                                      DataType,
                                      Selector,
                                      &DataSize,
                                      Data
                                      );
    }

    ASSERT_EFI_ERROR (GetDataStatus);

    if (EFI_ERROR (Routine (Selector, Data, Context))) {
      break;
    }
  }

  if (Data != NULL) {
    gBS->FreePool (Data);
  }

  if (Selector != NULL) {
    gBS->FreePool (Selector);
  }

  return EFI_SUCCESS;
}

