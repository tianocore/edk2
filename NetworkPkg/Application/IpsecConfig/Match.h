/** @file
  The internal structure and function declaration of 
  match policy entry function in IpSecConfig application.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _MATCH_H_
#define _MATCH_H_

/**
  The prototype for the MatchSpdEntry()/MatchSadEntry()/MatchPadEntry().
  The functionality is to find the matching SPD/SAD/PAD with Indexer.

  @param[in] Selector    The pointer to the EFI_IPSEC_CONFIG_SELECTOR union.
  @param[in] Data        The pointer to corresponding Data.
  @param[in] Indexer     The pointer to the POLICY_ENTRY_INDEXER union.

  @retval TRUE     The matched SPD/SAD/PAD is found.
  @retval FALSE    The matched SPD/SAD/PAD is not found.
**/
typedef
BOOLEAN
(* MATCH_POLICY_ENTRY) (
  IN EFI_IPSEC_CONFIG_SELECTOR    *Selector,
  IN VOID                         *Data,
  IN POLICY_ENTRY_INDEXER         *Indexer
  );

extern MATCH_POLICY_ENTRY mMatchPolicyEntry[];

#endif
