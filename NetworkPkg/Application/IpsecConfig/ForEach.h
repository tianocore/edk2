/** @file
  The internal structure and function declaration of the implementation
  to go through each entry in IpSecConfig application.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FOREACH_H_
#define _FOREACH_H_

/**
  The prototype for the DumpSpdEntry()/DumpSadEntry()/DumpPadEntry().
  Print EFI_IPSEC_CONFIG_SELECTOR and corresponding content.

  @param[in] Selector    The pointer to the EFI_IPSEC_CONFIG_SELECTOR union.
  @param[in] Data        The pointer to the corresponding data.
  @param[in] Context     The pointer to the Index in SPD/SAD/PAD Database.

  @retval EFI_SUCCESS    Dump SPD/SAD/PAD information successfully.
**/
typedef
EFI_STATUS
(*VISIT_POLICY_ENTRY) (
  IN EFI_IPSEC_CONFIG_SELECTOR    *Selector,
  IN VOID                         *Data,
  IN VOID                         *Context
  );

/**
  Enumerate all entry in the database to execute a specified operation according to datatype.

  @param[in] DataType    The value of EFI_IPSEC_CONFIG_DATA_TYPE.
  @param[in] Routine     The pointer to function of a specified operation.
  @param[in] Context     The pointer to the context of a function.

  @retval EFI_SUCCESS    Execute specified operation successfully.
**/
EFI_STATUS
ForeachPolicyEntry (
  IN EFI_IPSEC_CONFIG_DATA_TYPE    DataType,
  IN VISIT_POLICY_ENTRY            Routine,
  IN VOID                          *Context
  );

#endif
