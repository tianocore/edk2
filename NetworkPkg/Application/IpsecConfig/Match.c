/** @file
  The implementation of match policy entry function in IpSecConfig application.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IpSecConfig.h"
#include "Indexer.h"
#include "Match.h"

/**
  Private function to validate a buffer that should be filled with zero.

  @param[in] Memory    The pointer to the buffer.
  @param[in] Size      The size of the buffer.

  @retval TRUE     The memory is filled with zero.
  @retval FALSE    The memory isn't filled with zero.
**/
BOOLEAN
IsMemoryZero (
  IN VOID     *Memory,
  IN UINTN    Size
  )
{
  UINTN    Index;

  for (Index = 0; Index < Size; Index++) {
    if (*((UINT8 *) Memory + Index) != 0) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Find the matching SPD with Indexer.

  @param[in] Selector    The pointer to the EFI_IPSEC_SPD_SELECTOR structure.
  @param[in] Data        The pointer to the EFI_IPSEC_SPD_DATA structure.
  @param[in] Indexer     The pointer to the SPD_ENTRY_INDEXER structure.

  @retval TRUE     The matched SPD is found.
  @retval FALSE    The matched SPD is not found.
**/
BOOLEAN
MatchSpdEntry (
  IN EFI_IPSEC_SPD_SELECTOR    *Selector,
  IN EFI_IPSEC_SPD_DATA        *Data,
  IN SPD_ENTRY_INDEXER         *Indexer
  )
{
  BOOLEAN    Match;

  Match = FALSE;
  if (Indexer->Name != NULL) {
    if ((Data->Name != NULL) && (AsciiStrCmp ((CHAR8 *) Indexer->Name, (CHAR8 *) Data->Name) == 0)) {
      Match = TRUE;
    }
  } else {
    if (Indexer->Index == 0) {
      Match = TRUE;
    }

    Indexer->Index--;
  }

  return Match;
}

/**
  Find the matching SAD with Indexer.

  @param[in] SaId       The pointer to the EFI_IPSEC_SA_ID structure.
  @param[in] Data       The pointer to the EFI_IPSEC_SA_DATA2 structure.
  @param[in] Indexer    The pointer to the SPD_ENTRY_INDEXER structure.

  @retval TRUE     The matched SAD is found.
  @retval FALSE    The matched SAD is not found.
**/
BOOLEAN
MatchSadEntry (
  IN EFI_IPSEC_SA_ID      *SaId,
  IN EFI_IPSEC_SA_DATA2   *Data,
  IN SAD_ENTRY_INDEXER    *Indexer
  )
{
  BOOLEAN    Match;

  Match = FALSE;
  if (!IsMemoryZero (&Indexer->SaId, sizeof (EFI_IPSEC_SA_ID))) {
    Match = (BOOLEAN) (CompareMem (&Indexer->SaId, SaId, sizeof (EFI_IPSEC_SA_ID)) == 0);
  } else {
    if (Indexer->Index == 0) {
      Match = TRUE;
    }
    Indexer->Index--;
  }

  return Match;
}

/**
  Find the matching PAD with Indexer.

  @param[in] PadId      The pointer to the EFI_IPSEC_PAD_ID structure.
  @param[in] Data       The pointer to the EFI_IPSEC_PAD_DATA structure.
  @param[in] Indexer    The pointer to the SPD_ENTRY_INDEXER structure.

  @retval TRUE     The matched PAD is found.
  @retval FALSE    The matched PAD is not found.
**/
BOOLEAN
MatchPadEntry (
  IN EFI_IPSEC_PAD_ID      *PadId,
  IN EFI_IPSEC_PAD_DATA    *Data,
  IN PAD_ENTRY_INDEXER     *Indexer
  )
{
  BOOLEAN                       Match;

  Match = FALSE;
  if (!IsMemoryZero (&Indexer->PadId, sizeof (EFI_IPSEC_PAD_ID))) {
    Match = (BOOLEAN) ((Indexer->PadId.PeerIdValid == PadId->PeerIdValid) &&
                       ((PadId->PeerIdValid &&
                         (StrCmp (
                            (CONST CHAR16 *) Indexer->PadId.Id.PeerId,
                            (CONST CHAR16 *) PadId->Id.PeerId
                            ) == 0)) ||
                        ((!PadId->PeerIdValid) &&
                         (Indexer->PadId.Id.IpAddress.PrefixLength == PadId->Id.IpAddress.PrefixLength) &&
                         (CompareMem (
                            &Indexer->PadId.Id.IpAddress.Address,
                            &PadId->Id.IpAddress.Address,
                            sizeof (EFI_IP_ADDRESS)
                            ) == 0))));
  } else {
    if (Indexer->Index == 0) {
      Match = TRUE;
    }

    Indexer->Index--;
  }

  return Match;
}

MATCH_POLICY_ENTRY mMatchPolicyEntry[] = {
  (MATCH_POLICY_ENTRY) MatchSpdEntry,
  (MATCH_POLICY_ENTRY) MatchSadEntry,
  (MATCH_POLICY_ENTRY) MatchPadEntry
};

