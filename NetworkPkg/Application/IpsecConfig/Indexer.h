/** @file
  The internal structure and function declaration to construct ENTRY_INDEXER in
  IpSecConfig application.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _INDEXER_H_
#define _INDEXER_H_

typedef struct {
  UINT8    *Name;
  UINTN    Index;    // Used only if Name is NULL.
} SPD_ENTRY_INDEXER;

typedef struct {
  EFI_IPSEC_SA_ID    SaId;
  UINTN              Index;
} SAD_ENTRY_INDEXER;

typedef struct {
  EFI_IPSEC_PAD_ID    PadId;
  UINTN               Index;
} PAD_ENTRY_INDEXER;

typedef union {
  SPD_ENTRY_INDEXER    Spd;
  SAD_ENTRY_INDEXER    Sad;
  PAD_ENTRY_INDEXER    Pad;
} POLICY_ENTRY_INDEXER;

/**
  The prototype for the ConstructSpdIndexer()/ConstructSadIndexer()/ConstructPadIndexer().
  Fill in SPD_ENTRY_INDEXER/SAD_ENTRY_INDEXER/PAD_ENTRY_INDEXER through ParamPackage list.

  @param[in, out] Indexer         The pointer to the POLICY_ENTRY_INDEXER union.
  @param[in]      ParamPackage    The pointer to the ParamPackage list.

  @retval EFI_SUCCESS    Filled in POLICY_ENTRY_INDEXER successfully.
**/
typedef
EFI_STATUS
(* CONSTRUCT_POLICY_ENTRY_INDEXER) (
  IN POLICY_ENTRY_INDEXER    *Indexer,
  IN LIST_ENTRY              *ParamPackage
);

extern CONSTRUCT_POLICY_ENTRY_INDEXER mConstructPolicyEntryIndexer[];
#endif
