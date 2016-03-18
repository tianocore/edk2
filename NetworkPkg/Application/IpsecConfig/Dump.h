/** @file
  The function declaration of dump policy entry function in IpSecConfig application.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _DUMP_H_
#define _DUMP_H_

/**
  Print all entry information in the database according to datatype.

  @param[in] DataType        The value of EFI_IPSEC_CONFIG_DATA_TYPE.
  @param[in] ParamPackage    The pointer to the ParamPackage list.

  @retval EFI_SUCCESS    Dump all information successfully.
  @retval Others         Some mistaken case.
**/
EFI_STATUS
ListPolicyEntry (
  IN EFI_IPSEC_CONFIG_DATA_TYPE    DataType,
  IN LIST_ENTRY                    *ParamPackage
  );

#endif
