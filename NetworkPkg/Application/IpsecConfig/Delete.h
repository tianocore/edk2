/** @file
  The internal structure and function declaration of delete policy entry function
  in IpSecConfig application.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __DELETE_H_
#define __DELETE_H_

typedef struct {
  EFI_IPSEC_CONFIG_DATA_TYPE    DataType;
  POLICY_ENTRY_INDEXER          Indexer;
  EFI_STATUS                    Status;      //Indicate whether deletion succeeds.
} DELETE_POLICY_ENTRY_CONTEXT;

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
  );

#endif
