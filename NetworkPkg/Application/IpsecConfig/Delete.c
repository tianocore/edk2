/** @file
  The implementation of delete policy entry function in IpSecConfig application.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IpSecConfig.h"
#include "Indexer.h"
#include "Delete.h"
#include "Match.h"
#include "ForEach.h"

/**
  Private function to delete entry information in database.

  @param[in] Selector    The pointer to EFI_IPSEC_CONFIG_SELECTOR structure.
  @param[in] Data        The pointer to Data.
  @param[in] Context     The pointer to DELETE_POLICY_ENTRY_CONTEXT.

  @retval EFI_ABORTED    Abort the iteration.
  @retval EFI_SUCCESS    Continue the iteration.
**/
EFI_STATUS
DeletePolicyEntry (
  IN EFI_IPSEC_CONFIG_SELECTOR      *Selector,
  IN VOID                           *Data,
  IN DELETE_POLICY_ENTRY_CONTEXT    *Context
  )
{
  if (mMatchPolicyEntry[Context->DataType] (Selector, Data, &Context->Indexer)) {
    Context->Status = mIpSecConfig->SetData (
                                      mIpSecConfig,
                                      Context->DataType,
                                      Selector,
                                      NULL,
                                      NULL
                                      );
    //
    // Abort the iteration after the insertion.
    //
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

/**
  Flush or delete entry information in the database according to datatype.

  @param[in] DataType        The value of EFI_IPSEC_CONFIG_DATA_TYPE.
  @param[in] ParamPackage    The pointer to the ParamPackage list.

  @retval EFI_SUCCESS      Delete entry information successfully.
  @retval EFI_NOT_FOUND    Can't find the specified entry.
  @retval Others           Some mistaken case.
**/
EFI_STATUS
FlushOrDeletePolicyEntry (
  IN EFI_IPSEC_CONFIG_DATA_TYPE    DataType,
  IN LIST_ENTRY                    *ParamPackage
  )
{
  EFI_STATUS                     Status;
  DELETE_POLICY_ENTRY_CONTEXT    Context;
  CONST CHAR16                   *ValueStr;

  //
  // If user wants to remove all.
  //
  if (ShellCommandLineGetFlag (ParamPackage, L"-f")) {
    Status = mIpSecConfig->SetData (
                             mIpSecConfig,
                             DataType,
                             NULL,
                             NULL,
                             NULL
                             );
  } else {
    ValueStr = ShellCommandLineGetValue (ParamPackage, L"-d");
    if (ValueStr == NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_INDEX_NOT_SPECIFIED), mHiiHandle, mAppName, ValueStr);
      return EFI_NOT_FOUND;
    }

    Status = mConstructPolicyEntryIndexer[DataType] (&Context.Indexer, ParamPackage);
    if (!EFI_ERROR (Status)) {
      Context.DataType  = DataType;
      Context.Status    = EFI_NOT_FOUND;
      ForeachPolicyEntry (DataType, (VISIT_POLICY_ENTRY) DeletePolicyEntry, &Context);
      Status = Context.Status;

      if (Status == EFI_NOT_FOUND) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_INDEX_NOT_FOUND), mHiiHandle, mAppName, ValueStr);
      } else if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_DELETE_FAILED), mHiiHandle, mAppName);
      }
    }
  }

  return Status;
}
