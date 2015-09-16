/** @file
  The implementation of IPSEC_CONFIG_PROTOCOL.

  Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IpSecConfigImpl.h"
#include "IpSecDebug.h"

LIST_ENTRY                mConfigData[IPsecConfigDataTypeMaximum];
BOOLEAN                   mSetBySelf = FALSE;

//
// Common CompareSelector routine entry for SPD/SAD/PAD.
//
IPSEC_COMPARE_SELECTOR    mCompareSelector[] = {
  (IPSEC_COMPARE_SELECTOR) CompareSpdSelector,
  (IPSEC_COMPARE_SELECTOR) CompareSaId,
  (IPSEC_COMPARE_SELECTOR) ComparePadId
};

//
// Common IsZeroSelector routine entry for SPD/SAD/PAD.
//
IPSEC_IS_ZERO_SELECTOR    mIsZeroSelector[] = {
  (IPSEC_IS_ZERO_SELECTOR) IsZeroSpdSelector,
  (IPSEC_IS_ZERO_SELECTOR) IsZeroSaId,
  (IPSEC_IS_ZERO_SELECTOR) IsZeroPadId
};

//
// Common DuplicateSelector routine entry for SPD/SAD/PAD.
//
IPSEC_DUPLICATE_SELECTOR  mDuplicateSelector[] = {
  (IPSEC_DUPLICATE_SELECTOR) DuplicateSpdSelector,
  (IPSEC_DUPLICATE_SELECTOR) DuplicateSaId,
  (IPSEC_DUPLICATE_SELECTOR) DuplicatePadId
};

//
// Common FixPolicyEntry routine entry for SPD/SAD/PAD.
//
IPSEC_FIX_POLICY_ENTRY    mFixPolicyEntry[] = {
  (IPSEC_FIX_POLICY_ENTRY) FixSpdEntry,
  (IPSEC_FIX_POLICY_ENTRY) FixSadEntry,
  (IPSEC_FIX_POLICY_ENTRY) FixPadEntry
};

//
// Common UnfixPolicyEntry routine entry for SPD/SAD/PAD.
//
IPSEC_FIX_POLICY_ENTRY    mUnfixPolicyEntry[] = {
  (IPSEC_FIX_POLICY_ENTRY) UnfixSpdEntry,
  (IPSEC_FIX_POLICY_ENTRY) UnfixSadEntry,
  (IPSEC_FIX_POLICY_ENTRY) UnfixPadEntry
};

//
// Common SetPolicyEntry routine entry for SPD/SAD/PAD.
//
IPSEC_SET_POLICY_ENTRY    mSetPolicyEntry[] = {
  (IPSEC_SET_POLICY_ENTRY) SetSpdEntry,
  (IPSEC_SET_POLICY_ENTRY) SetSadEntry,
  (IPSEC_SET_POLICY_ENTRY) SetPadEntry
};

//
// Common GetPolicyEntry routine entry for SPD/SAD/PAD.
//
IPSEC_GET_POLICY_ENTRY    mGetPolicyEntry[] = {
  (IPSEC_GET_POLICY_ENTRY) GetSpdEntry,
  (IPSEC_GET_POLICY_ENTRY) GetSadEntry,
  (IPSEC_GET_POLICY_ENTRY) GetPadEntry
};

//
// Routine entry for IpSecConfig protocol.
//
EFI_IPSEC_CONFIG_PROTOCOL mIpSecConfigInstance = {
  EfiIpSecConfigSetData,
  EfiIpSecConfigGetData,
  EfiIpSecConfigGetNextSelector,
  EfiIpSecConfigRegisterNotify,
  EfiIpSecConfigUnregisterNotify
};

/**
  Get the all IPSec configuration variables and store those variables
  to the internal data structure.

  This founction is called by IpSecConfigInitialize() that is to intialize the 
  IPsecConfiguration Protocol.

  @param[in]  Private            Point to IPSEC_PRIVATE_DATA.

  @retval EFI_OUT_OF_RESOURCES   The required system resource could not be allocated.
  @retval EFI_SUCCESS            Restore the IPsec Configuration successfully.
  @retval  others                Other errors is found during the variable getting.

**/
EFI_STATUS
IpSecConfigRestore (
  IN IPSEC_PRIVATE_DATA               *Private
  );

/**
  Check if the specified EFI_IP_ADDRESS_INFO is in EFI_IP_ADDRESS_INFO list.

  @param[in]   AddressInfo         Pointer of IP_ADDRESS_INFO to be search in AddressInfo list.
  @param[in]   AddressInfoList     A list that contains IP_ADDRESS_INFOs.
  @param[in]   AddressCount        Point out how many IP_ADDRESS_INFO in the list.

  @retval  TRUE    The specified AddressInfo is in the AddressInfoList.
  @retval  FALSE   The specified AddressInfo is not in the AddressInfoList.
  
**/
BOOLEAN
IsInAddressInfoList(
  IN EFI_IP_ADDRESS_INFO              *AddressInfo,
  IN EFI_IP_ADDRESS_INFO              *AddressInfoList,
  IN UINT32                           AddressCount
  )
{
  UINT8           Index;
  EFI_IP_ADDRESS  ZeroAddress;

  ZeroMem(&ZeroAddress, sizeof (EFI_IP_ADDRESS));

  //
  // Zero Address means any address is matched.
  //
  if (AddressCount == 1) {
    if (CompareMem (
          &AddressInfoList[0].Address,
          &ZeroAddress,
          sizeof (EFI_IP_ADDRESS)
          ) == 0) {
      return TRUE;
    }
  }
  for (Index = 0; Index < AddressCount ; Index++) {
    if (CompareMem (
          AddressInfo,
          &AddressInfoList[Index].Address,
          sizeof (EFI_IP_ADDRESS)
          ) == 0 && 
          AddressInfo->PrefixLength == AddressInfoList[Index].PrefixLength
          ) { 
       return TRUE;
     }
  }
  return FALSE;
}
 
/**
  Compare two SPD Selectors.

  Compare two SPD Selector by the fields of LocalAddressCount/RemoteAddressCount/
  NextLayerProtocol/LocalPort/LocalPortRange/RemotePort/RemotePortRange and the 
  Local Addresses and remote Addresses.

  @param[in]   Selector1           Pointer of first SPD Selector.
  @param[in]   Selector2           Pointer of second SPD Selector.

  @retval  TRUE    This two Selector have the same value in above fields.
  @retval  FALSE   Not all above fields have the same value in these two Selectors.
  
**/
BOOLEAN
CompareSpdSelector (
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector1,
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector2
  )
{
  EFI_IPSEC_SPD_SELECTOR  *SpdSel1;
  EFI_IPSEC_SPD_SELECTOR  *SpdSel2;
  BOOLEAN                 IsMatch;
  UINTN                   Index;

  SpdSel1 = &Selector1->SpdSelector;
  SpdSel2 = &Selector2->SpdSelector;
  IsMatch = TRUE;

  //
  // Compare the LocalAddressCount/RemoteAddressCount/NextLayerProtocol/
  // LocalPort/LocalPortRange/RemotePort/RemotePortRange fields in the
  // two Spdselectors. Since the SPD supports two directions, it needs to 
  // compare two directions.
  //
  if ((SpdSel1->LocalAddressCount != SpdSel2->LocalAddressCount &&
       SpdSel1->LocalAddressCount != SpdSel2->RemoteAddressCount) ||
      (SpdSel1->RemoteAddressCount != SpdSel2->RemoteAddressCount &&
       SpdSel1->RemoteAddressCount != SpdSel2->LocalAddressCount) ||
       SpdSel1->NextLayerProtocol != SpdSel2->NextLayerProtocol ||
       SpdSel1->LocalPort != SpdSel2->LocalPort ||
       SpdSel1->LocalPortRange != SpdSel2->LocalPortRange ||
       SpdSel1->RemotePort != SpdSel2->RemotePort ||
       SpdSel1->RemotePortRange != SpdSel2->RemotePortRange
       ) {
    IsMatch = FALSE;
    return IsMatch;
  }
  
  //
  // Compare the all LocalAddress fields in the two Spdselectors.
  // First, SpdSel1->LocalAddress to SpdSel2->LocalAddress && Compare 
  // SpdSel1->RemoteAddress to SpdSel2->RemoteAddress. If all match, return
  // TRUE.
  //
  for (Index = 0; Index < SpdSel1->LocalAddressCount; Index++) {
    if (!IsInAddressInfoList (
          &SpdSel1->LocalAddress[Index],
          SpdSel2->LocalAddress,
          SpdSel2->LocalAddressCount
          )) {
      IsMatch = FALSE;
      break;
    }
  }
  if (IsMatch) {
    for (Index = 0; Index < SpdSel2->LocalAddressCount; Index++) {
      if (!IsInAddressInfoList (
            &SpdSel2->LocalAddress[Index],
            SpdSel1->LocalAddress,
            SpdSel1->LocalAddressCount
            )) {
        IsMatch = FALSE;
        break;
      }
    }
  }
  if (IsMatch) {
    for (Index = 0; Index < SpdSel1->RemoteAddressCount; Index++) {
      if (!IsInAddressInfoList (
            &SpdSel1->RemoteAddress[Index],
            SpdSel2->RemoteAddress,
            SpdSel2->RemoteAddressCount
            )) {
        IsMatch = FALSE;
        break;
      }
    }
  }
  if (IsMatch) {
    for (Index = 0; Index < SpdSel2->RemoteAddressCount; Index++) {
      if (!IsInAddressInfoList (
            &SpdSel2->RemoteAddress[Index],
            SpdSel1->RemoteAddress,
            SpdSel1->RemoteAddressCount
            )) {
        IsMatch = FALSE;
        break;
      }
    }
  }
  //
  // Finish the one direction compare. If it is matched, return; otherwise, 
  // compare the other direction.
  //
  if (IsMatch) {
    return IsMatch;
  }
  //
  // Secondly, the SpdSel1->LocalAddress doesn't equal to  SpdSel2->LocalAddress and 
  // SpdSel1->RemoteAddress doesn't equal to SpdSel2->RemoteAddress. Try to compare
  // the RemoteAddress to LocalAddress.
  //
  IsMatch = TRUE;
  for (Index = 0; Index < SpdSel1->RemoteAddressCount; Index++) {
    if (!IsInAddressInfoList (
          &SpdSel1->RemoteAddress[Index],
          SpdSel2->LocalAddress,
          SpdSel2->LocalAddressCount
          )) {
      IsMatch = FALSE;
      break;
    }
  }
  if (IsMatch) {
    for (Index = 0; Index < SpdSel2->RemoteAddressCount; Index++) {
      if (!IsInAddressInfoList (
            &SpdSel2->RemoteAddress[Index],
            SpdSel1->LocalAddress,
            SpdSel1->LocalAddressCount
            )) {
        IsMatch = FALSE;
        break;
      }
    }
  }
  if (IsMatch) {
    for (Index = 0; Index < SpdSel1->LocalAddressCount; Index++) {
      if (!IsInAddressInfoList (
            &SpdSel1->LocalAddress[Index],
            SpdSel2->RemoteAddress,
            SpdSel2->RemoteAddressCount
            )) {
        IsMatch = FALSE;
        break;
      }
    }
  }
  if (IsMatch) {
    for (Index = 0; Index < SpdSel2->LocalAddressCount; Index++) {
      if (!IsInAddressInfoList (
            &SpdSel2->LocalAddress[Index],
            SpdSel1->RemoteAddress,
            SpdSel1->RemoteAddressCount
            )) {
        IsMatch = FALSE;
        break;
      }
    }
  }
  return IsMatch;
}

/**
  Find if the two SPD Selectors has subordinative.

  Compare two SPD Selector by the fields of LocalAddressCount/RemoteAddressCount/
  NextLayerProtocol/LocalPort/LocalPortRange/RemotePort/RemotePortRange and the 
  Local Addresses and remote Addresses.

  @param[in]   Selector1           Pointer of first SPD Selector.
  @param[in]   Selector2           Pointer of second SPD Selector.

  @retval  TRUE    The first SPD Selector is subordinate Selector of second SPD Selector.
  @retval  FALSE   The first SPD Selector is not subordinate Selector of second 
                   SPD Selector.
  
**/
BOOLEAN
IsSubSpdSelector (
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector1,
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector2
  )
{
  EFI_IPSEC_SPD_SELECTOR  *SpdSel1;
  EFI_IPSEC_SPD_SELECTOR  *SpdSel2;
  BOOLEAN                 IsMatch;
  UINTN                   Index;

  SpdSel1 = &Selector1->SpdSelector;
  SpdSel2 = &Selector2->SpdSelector;
  IsMatch = TRUE;

  //
  // Compare the LocalAddressCount/RemoteAddressCount/NextLayerProtocol/
  // LocalPort/LocalPortRange/RemotePort/RemotePortRange fields in the
  // two Spdselectors. Since the SPD supports two directions, it needs to 
  // compare two directions.
  //
  if (SpdSel1->LocalAddressCount > SpdSel2->LocalAddressCount ||
      SpdSel1->RemoteAddressCount > SpdSel2->RemoteAddressCount ||
      (SpdSel1->NextLayerProtocol != SpdSel2->NextLayerProtocol && SpdSel2->NextLayerProtocol != 0xffff) ||
      (SpdSel1->LocalPort > SpdSel2->LocalPort && SpdSel2->LocalPort != 0)||
      (SpdSel1->LocalPortRange > SpdSel2->LocalPortRange && SpdSel1->LocalPort != 0)||
      (SpdSel1->RemotePort > SpdSel2->RemotePort && SpdSel2->RemotePort != 0) ||
      (SpdSel1->RemotePortRange > SpdSel2->RemotePortRange && SpdSel2->RemotePort != 0)
      ) {
    IsMatch = FALSE;
  }
  
  //
  // Compare the all LocalAddress fields in the two Spdselectors.
  // First, SpdSel1->LocalAddress to SpdSel2->LocalAddress && Compare 
  // SpdSel1->RemoteAddress to SpdSel2->RemoteAddress. If all match, return
  // TRUE.
  //
  if (IsMatch) {
    for (Index = 0; Index < SpdSel1->LocalAddressCount; Index++) {
      if (!IsInAddressInfoList (
            &SpdSel1->LocalAddress[Index],
            SpdSel2->LocalAddress,
            SpdSel2->LocalAddressCount
            )) {
        IsMatch = FALSE;
        break;
      }
    }

    if (IsMatch) {
      for (Index = 0; Index < SpdSel1->RemoteAddressCount; Index++) {
        if (!IsInAddressInfoList (
              &SpdSel1->RemoteAddress[Index],
              SpdSel2->RemoteAddress,
              SpdSel2->RemoteAddressCount
              )) {
          IsMatch = FALSE;
          break;
        }
      }
    }
  }
  if (IsMatch) {
    return IsMatch;
  }
  
  //
  //
  // The SPD selector in SPD entry is two way.
  //
  // Compare the LocalAddressCount/RemoteAddressCount/NextLayerProtocol/
  // LocalPort/LocalPortRange/RemotePort/RemotePortRange fields in the
  // two Spdselectors. Since the SPD supports two directions, it needs to 
  // compare two directions.
  //
  IsMatch = TRUE;
  if (SpdSel1->LocalAddressCount > SpdSel2->RemoteAddressCount ||
      SpdSel1->RemoteAddressCount > SpdSel2->LocalAddressCount ||
      (SpdSel1->NextLayerProtocol != SpdSel2->NextLayerProtocol && SpdSel2->NextLayerProtocol != 0xffff) ||
      (SpdSel1->LocalPort > SpdSel2->RemotePort && SpdSel2->RemotePort != 0)||
      (SpdSel1->LocalPortRange > SpdSel2->RemotePortRange && SpdSel1->RemotePort != 0)||
      (SpdSel1->RemotePort > SpdSel2->LocalPort && SpdSel2->LocalPort != 0) ||
      (SpdSel1->RemotePortRange > SpdSel2->LocalPortRange && SpdSel2->LocalPort != 0)
      ) {
    IsMatch = FALSE;
    return IsMatch;
  }
  
  //
  // Compare the all LocalAddress fields in the two Spdselectors.
  // First, SpdSel1->LocalAddress to SpdSel2->LocalAddress && Compare 
  // SpdSel1->RemoteAddress to SpdSel2->RemoteAddress. If all match, return
  // TRUE.
  //
  for (Index = 0; Index < SpdSel1->LocalAddressCount; Index++) {
    if (!IsInAddressInfoList (
          &SpdSel1->LocalAddress[Index],
          SpdSel2->RemoteAddress,
          SpdSel2->RemoteAddressCount
          )) {
      IsMatch = FALSE;
      break;
    }
  }

  if (IsMatch) {
    for (Index = 0; Index < SpdSel1->RemoteAddressCount; Index++) {
      if (!IsInAddressInfoList (
            &SpdSel1->RemoteAddress[Index],
            SpdSel2->LocalAddress,
            SpdSel2->LocalAddressCount
            )) {
        IsMatch = FALSE;
        break;
      }
    }
  }
  return IsMatch;
  
}

/**
  Compare two SA IDs.

  @param[in]   Selector1           Pointer of first SA ID.
  @param[in]   Selector2           Pointer of second SA ID.

  @retval  TRUE    This two Selectors have the same SA ID.
  @retval  FALSE   This two Selecotrs don't have the same SA ID.
  
**/
BOOLEAN
CompareSaId (
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector1,
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector2
  )
{
  EFI_IPSEC_SA_ID *SaId1;
  EFI_IPSEC_SA_ID *SaId2;
  BOOLEAN         IsMatch;

  SaId1   = &Selector1->SaId;
  SaId2   = &Selector2->SaId;
  IsMatch = TRUE;

  if (CompareMem (SaId1, SaId2, sizeof (EFI_IPSEC_SA_ID)) != 0) {
    IsMatch = FALSE;
  }

  return IsMatch;
}

/**
  Compare two PAD IDs.

  @param[in]   Selector1           Pointer of first PAD ID.
  @param[in]   Selector2           Pointer of second PAD ID.

  @retval  TRUE    This two Selectors have the same PAD ID.
  @retval  FALSE   This two Selecotrs don't have the same PAD ID.
  
**/
BOOLEAN
ComparePadId (
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector1,
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector2
  )
{
  EFI_IPSEC_PAD_ID  *PadId1;
  EFI_IPSEC_PAD_ID  *PadId2;
  BOOLEAN           IsMatch;

  PadId1  = &Selector1->PadId;
  PadId2  = &Selector2->PadId;
  IsMatch = TRUE;

  //
  // Compare the PeerIdValid fields in PadId.
  //
  if (PadId1->PeerIdValid != PadId2->PeerIdValid) {
    IsMatch = FALSE;
  }
  //
  // Compare the PeerId fields in PadId if PeerIdValid is true.
  //
  if (IsMatch &&
      PadId1->PeerIdValid &&
      AsciiStriCmp ((CONST CHAR8 *) PadId1->Id.PeerId, (CONST CHAR8 *) PadId2->Id.PeerId) != 0
      ) {
    IsMatch = FALSE;
  }
  //
  // Compare the IpAddress fields in PadId if PeerIdValid is false.
  //
  if (IsMatch &&
      !PadId1->PeerIdValid &&
      (PadId1->Id.IpAddress.PrefixLength != PadId2->Id.IpAddress.PrefixLength ||
       CompareMem (&PadId1->Id.IpAddress.Address, &PadId2->Id.IpAddress.Address, sizeof (EFI_IP_ADDRESS)) != 0)
      ) {
    IsMatch = FALSE;
  }

  return IsMatch;
}

/**
  Check if the SPD Selector is Zero by its LocalAddressCount and RemoteAddressCount
  fields.

  @param[in]  Selector      Pointer of the SPD Selector.

  @retval     TRUE          If the SPD Selector is Zero.
  @retval     FALSE         If the SPD Selector is not Zero.

**/
BOOLEAN
IsZeroSpdSelector (
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector
  )
{
  EFI_IPSEC_SPD_SELECTOR  *SpdSel;
  BOOLEAN                 IsZero;

  SpdSel  = &Selector->SpdSelector;
  IsZero  = FALSE;

  if (SpdSel->LocalAddressCount == 0 && SpdSel->RemoteAddressCount == 0) {
    IsZero = TRUE;
  }

  return IsZero;
}

/**
  Check if the SA ID is Zero by its DestAddress.

  @param[in]  Selector      Pointer of the SA ID.

  @retval     TRUE          If the SA ID is Zero.
  @retval     FALSE         If the SA ID is not Zero.

**/
BOOLEAN
IsZeroSaId (
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector
  )
{
  BOOLEAN                   IsZero;
  EFI_IPSEC_CONFIG_SELECTOR ZeroSelector;
  
  IsZero    = FALSE;

  ZeroMem (&ZeroSelector, sizeof (EFI_IPSEC_CONFIG_SELECTOR));

  if (CompareMem (&ZeroSelector, Selector, sizeof (EFI_IPSEC_CONFIG_SELECTOR)) == 0) {
    IsZero = TRUE;
  }

  return IsZero;
}

/**
  Check if the PAD ID is Zero.

  @param[in]  Selector      Pointer of the PAD ID.

  @retval     TRUE          If the PAD ID is Zero.
  @retval     FALSE         If the PAD ID is not Zero.

**/
BOOLEAN
IsZeroPadId (
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector
  )
{
  EFI_IPSEC_PAD_ID  *PadId;
  EFI_IPSEC_PAD_ID  ZeroId;
  BOOLEAN           IsZero;

  PadId   = &Selector->PadId;
  IsZero  = FALSE;

  ZeroMem (&ZeroId, sizeof (EFI_IPSEC_PAD_ID));

  if (CompareMem (PadId, &ZeroId, sizeof (EFI_IPSEC_PAD_ID)) == 0) {
    IsZero = TRUE;
  }

  return IsZero;
}

/**
  Copy Source SPD Selector to the Destination SPD Selector.

  @param[in, out] DstSel             Pointer of Destination SPD Selector.
  @param[in]      SrcSel             Pointer of Source SPD Selector.
  @param[in, out] Size               The size of the Destination SPD Selector. If it 
                                     not NULL and its value less than the size of 
                                     Source SPD Selector, the value of Source SPD 
                                     Selector's size will be passed to caller by this
                                     parameter.

  @retval EFI_INVALID_PARAMETER  If the Destination or Source SPD Selector is NULL
  @retval EFI_BUFFER_TOO_SMALL   If the input Size is less than size of the Source SPD Selector. 
  @retval EFI_SUCCESS            Copy Source SPD Selector to the Destination SPD
                                 Selector successfully.

**/
EFI_STATUS
DuplicateSpdSelector (
  IN OUT EFI_IPSEC_CONFIG_SELECTOR    *DstSel,
  IN     EFI_IPSEC_CONFIG_SELECTOR    *SrcSel,
  IN OUT UINTN                        *Size
  )
{
  EFI_IPSEC_SPD_SELECTOR  *Dst;
  EFI_IPSEC_SPD_SELECTOR  *Src;

  Dst = &DstSel->SpdSelector;
  Src = &SrcSel->SpdSelector;

  if (Dst == NULL || Src == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Size != NULL && (*Size) < SIZE_OF_SPD_SELECTOR (Src)) {
    *Size = SIZE_OF_SPD_SELECTOR (Src);
    return EFI_BUFFER_TOO_SMALL;
  }
  //
  // Copy the base structure of SPD selector.
  //
  CopyMem (Dst, Src, sizeof (EFI_IPSEC_SPD_SELECTOR));

  //
  // Copy the local address array of SPD selector.
  //
  Dst->LocalAddress = (EFI_IP_ADDRESS_INFO *) (Dst + 1);
  CopyMem (
    Dst->LocalAddress,
    Src->LocalAddress,
    sizeof (EFI_IP_ADDRESS_INFO) * Dst->LocalAddressCount
    );

  //
  // Copy the remote address array of SPD selector.
  //
  Dst->RemoteAddress = Dst->LocalAddress + Dst->LocalAddressCount;
  CopyMem (
    Dst->RemoteAddress,
    Src->RemoteAddress,
    sizeof (EFI_IP_ADDRESS_INFO) * Dst->RemoteAddressCount
    );

  return EFI_SUCCESS;
}

/**
  Copy Source SA ID to the Destination SA ID.

  @param[in, out] DstSel             Pointer of Destination SA ID.
  @param[in]      SrcSel             Pointer of Source SA ID.
  @param[in, out] Size               The size of the Destination SA ID. If it 
                                     not NULL and its value less than the size of 
                                     Source SA ID, the value of Source SA ID's size 
                                     will be passed to caller by this parameter.

  @retval EFI_INVALID_PARAMETER  If the Destination or Source SA ID is NULL.
  @retval EFI_BUFFER_TOO_SMALL   If the input Size less than size of source SA ID. 
  @retval EFI_SUCCESS            Copy Source SA ID  to the Destination SA ID successfully.

**/
EFI_STATUS
DuplicateSaId (
  IN OUT EFI_IPSEC_CONFIG_SELECTOR    *DstSel,
  IN     EFI_IPSEC_CONFIG_SELECTOR    *SrcSel,
  IN OUT UINTN                        *Size
  )
{
  EFI_IPSEC_SA_ID *Dst;
  EFI_IPSEC_SA_ID *Src;

  Dst = &DstSel->SaId;
  Src = &SrcSel->SaId;

  if (Dst == NULL || Src == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Size != NULL && *Size < sizeof (EFI_IPSEC_SA_ID)) {
    *Size = sizeof (EFI_IPSEC_SA_ID);
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (Dst, Src, sizeof (EFI_IPSEC_SA_ID));

  return EFI_SUCCESS;
}

/**
  Copy Source PAD ID to the Destination PAD ID.

  @param[in, out] DstSel             Pointer of Destination PAD ID.
  @param[in]      SrcSel             Pointer of Source PAD ID.
  @param[in, out] Size               The size of the Destination PAD ID. If it 
                                     not NULL and its value less than the size of 
                                     Source PAD ID, the value of Source PAD ID's size 
                                     will be passed to caller by this parameter.

  @retval EFI_INVALID_PARAMETER  If the Destination or Source PAD ID is NULL.
  @retval EFI_BUFFER_TOO_SMALL   If the input Size less than size of source PAD ID .
  @retval EFI_SUCCESS            Copy Source PAD ID  to the Destination PAD ID successfully.

**/
EFI_STATUS
DuplicatePadId (
  IN OUT EFI_IPSEC_CONFIG_SELECTOR    *DstSel,
  IN     EFI_IPSEC_CONFIG_SELECTOR    *SrcSel,
  IN OUT UINTN                        *Size
  )
{
  EFI_IPSEC_PAD_ID  *Dst;
  EFI_IPSEC_PAD_ID  *Src;

  Dst = &DstSel->PadId;
  Src = &SrcSel->PadId;

  if (Dst == NULL || Src == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Size != NULL && *Size < sizeof (EFI_IPSEC_PAD_ID)) {
    *Size = sizeof (EFI_IPSEC_PAD_ID);
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (Dst, Src, sizeof (EFI_IPSEC_PAD_ID));

  return EFI_SUCCESS;
}

/**
  Fix the value of some members of SPD Selector. 

  This function is called by IpSecCopyPolicyEntry()which copy the Policy 
  Entry into the Variable. Since some members in SPD Selector are pointers, 
  a physical address to relative address convertion is required before copying 
  this SPD entry into the variable.

  @param[in]       Selector              Pointer of SPD Selector.
  @param[in, out]  Data                  Pointer of SPD Data.

**/
VOID
FixSpdEntry (
  IN     EFI_IPSEC_SPD_SELECTOR            *Selector,
  IN OUT EFI_IPSEC_SPD_DATA                *Data
  )
{
  //
  // It assumes that all ref buffers in SPD selector and data are
  // stored in the continous memory and close to the base structure.
  //
  FIX_REF_BUF_ADDR (Selector->LocalAddress, Selector);
  FIX_REF_BUF_ADDR (Selector->RemoteAddress, Selector);

  if (Data->ProcessingPolicy != NULL) {
    if (Data->ProcessingPolicy->TunnelOption != NULL) {
      FIX_REF_BUF_ADDR (Data->ProcessingPolicy->TunnelOption, Data);
    }

    FIX_REF_BUF_ADDR (Data->ProcessingPolicy, Data);
  }

}

/**
  Fix the value of some members of SA ID. 

  This function is called by IpSecCopyPolicyEntry()which copy the Policy 
  Entry into the Variable. Since some members in SA ID are pointers, 
  a physical address to relative address conversion is required before copying 
  this SAD into the variable.

  @param[in]       SaId                  Pointer of SA ID
  @param[in, out]  Data                  Pointer of SA Data.

**/
VOID
FixSadEntry (
  IN     EFI_IPSEC_SA_ID                  *SaId,
  IN OUT EFI_IPSEC_SA_DATA2                *Data
  )
{
  //
  // It assumes that all ref buffers in SAD selector and data are
  // stored in the continous memory and close to the base structure.
  //
  if (Data->AlgoInfo.EspAlgoInfo.AuthKey != NULL) {
    FIX_REF_BUF_ADDR (Data->AlgoInfo.EspAlgoInfo.AuthKey, Data);
  }

  if (SaId->Proto == EfiIPsecESP && Data->AlgoInfo.EspAlgoInfo.EncKey != NULL) {
    FIX_REF_BUF_ADDR (Data->AlgoInfo.EspAlgoInfo.EncKey, Data);
  }

  if (Data->SpdSelector != NULL) {
    if (Data->SpdSelector->LocalAddress != NULL) {
      FIX_REF_BUF_ADDR (Data->SpdSelector->LocalAddress, Data);
    }

    FIX_REF_BUF_ADDR (Data->SpdSelector->RemoteAddress, Data);
    FIX_REF_BUF_ADDR (Data->SpdSelector, Data);
  }

}

/**
  Fix the value of some members of PAD ID. 

  This function is called by IpSecCopyPolicyEntry()which copy the Policy 
  Entry into the Variable. Since some members in PAD ID are pointers, 
  a physical address to relative address conversion is required before copying
  this PAD into the variable.

  @param[in]       PadId              Pointer of PAD ID.
  @param[in, out]  Data               Pointer of PAD Data.

**/
VOID
FixPadEntry (
  IN     EFI_IPSEC_PAD_ID                  *PadId,
  IN OUT EFI_IPSEC_PAD_DATA                *Data
  )
{
  //
  // It assumes that all ref buffers in pad selector and data are
  // stored in the continous memory and close to the base structure.
  //
  if (Data->AuthData != NULL) {
    FIX_REF_BUF_ADDR (Data->AuthData, Data);
  }

  if (Data->RevocationData != NULL) {
    FIX_REF_BUF_ADDR (Data->RevocationData, Data);
  }

}

/**
  Recover the value of some members of SPD Selector. 

  This function is corresponding to FixSpdEntry(). It recovers the value of members
  of SPD Selector that are fixed by FixSpdEntry().

  @param[in, out]  Selector              Pointer of SPD Selector.
  @param[in, out]  Data                  Pointer of SPD Data.

**/
VOID
UnfixSpdEntry (
  IN OUT EFI_IPSEC_SPD_SELECTOR           *Selector,
  IN OUT EFI_IPSEC_SPD_DATA               *Data
  )
{
  //
  // It assumes that all ref buffers in SPD selector and data are
  // stored in the continous memory and close to the base structure.
  //
  UNFIX_REF_BUF_ADDR (Selector->LocalAddress, Selector);
  UNFIX_REF_BUF_ADDR (Selector->RemoteAddress, Selector);

  if (Data->ProcessingPolicy != NULL) {
    UNFIX_REF_BUF_ADDR (Data->ProcessingPolicy, Data);
    if (Data->ProcessingPolicy->TunnelOption != NULL) {
      UNFIX_REF_BUF_ADDR (Data->ProcessingPolicy->TunnelOption, Data);
    }
  }
  
}

/**
  Recover the value of some members of SA ID. 

  This function is corresponding to FixSadEntry(). It recovers the value of members
  of SAD ID that are fixed by FixSadEntry().

  @param[in, out]  SaId              Pointer of SAD ID.
  @param[in, out]  Data              Pointer of SAD Data.

**/
VOID
UnfixSadEntry (
  IN OUT EFI_IPSEC_SA_ID                     *SaId,
  IN OUT EFI_IPSEC_SA_DATA2                   *Data
  )
{
  //
  // It assumes that all ref buffers in SAD selector and data are
  // stored in the continous memory and close to the base structure.
  //
  if (Data->AlgoInfo.EspAlgoInfo.AuthKey != NULL) {
    UNFIX_REF_BUF_ADDR (Data->AlgoInfo.EspAlgoInfo.AuthKey, Data);
  }

  if (SaId->Proto == EfiIPsecESP && Data->AlgoInfo.EspAlgoInfo.EncKey != NULL) {
    UNFIX_REF_BUF_ADDR (Data->AlgoInfo.EspAlgoInfo.EncKey, Data);
  }

  if (Data->SpdSelector != NULL) {
    UNFIX_REF_BUF_ADDR (Data->SpdSelector, Data);
    if (Data->SpdSelector->LocalAddress != NULL) {
      UNFIX_REF_BUF_ADDR (Data->SpdSelector->LocalAddress, Data);
    }

    UNFIX_REF_BUF_ADDR (Data->SpdSelector->RemoteAddress, Data);
  }

}

/**
  Recover the value of some members of PAD ID. 

  This function is corresponding to FixPadEntry(). It recovers the value of members
  of PAD ID that are fixed by FixPadEntry().

  @param[in]       PadId              Pointer of PAD ID.
  @param[in, out]  Data               Pointer of PAD Data.

**/
VOID
UnfixPadEntry (
  IN     EFI_IPSEC_PAD_ID                 *PadId,
  IN OUT EFI_IPSEC_PAD_DATA               *Data
  )
{
  //
  // It assumes that all ref buffers in pad selector and data are
  // stored in the continous memory and close to the base structure.
  //
  if (Data->AuthData != NULL) {
    UNFIX_REF_BUF_ADDR (Data->AuthData, Data);
  }

  if (Data->RevocationData != NULL) {
    UNFIX_REF_BUF_ADDR (Data->RevocationData, Data);
  }

}

/**
  Set the security policy information for the EFI IPsec driver.

  The IPsec configuration data has a unique selector/identifier separately to 
  identify a data entry.

  @param[in]  Selector           Pointer to an entry selector on operated 
                                 configuration data specified by DataType. 
                                 A NULL Selector causes the entire specified-type 
                                 configuration information to be flushed.
  @param[in]  Data               The data buffer to be set. The structure 
                                 of the data buffer should be EFI_IPSEC_SPD_DATA.
  @param[in]  Context            Pointer to one entry selector that describes 
                                 the expected position the new data entry will 
                                 be added. If Context is NULL, the new entry will
                                 be appended the end of database.

  @retval EFI_INVALID_PARAMETER  One or more of the following are TRUE:
                                   - Selector is not NULL and its LocalAddress 
                                     is NULL or its RemoteAddress is NULL.
                                   - Data is not NULL and its Action is Protected 
                                     and its plolicy is NULL.
                                   - Data is not NULL, its Action is not protected,
                                     and its policy is not NULL.
                                   - The Action of Data is Protected, its policy 
                                     mode is Tunnel, and its tunnel option is NULL.
                                   - The Action of Data is protected and its policy 
                                     mode is not Tunnel and it tunnel option is not NULL.
  @retval EFI_OUT_OF_RESOURCED  The required system resource could not be allocated.
  @retval EFI_SUCCESS           The specified configuration data was obtained successfully.

**/
EFI_STATUS
SetSpdEntry (
  IN EFI_IPSEC_CONFIG_SELECTOR       *Selector,
  IN VOID                            *Data,
  IN VOID                            *Context OPTIONAL
  )
{
  EFI_IPSEC_SPD_SELECTOR  *SpdSel;
  EFI_IPSEC_SPD_DATA      *SpdData;
  EFI_IPSEC_SPD_SELECTOR  *InsertBefore;
  LIST_ENTRY              *SpdList;
  LIST_ENTRY              *SadList;
  LIST_ENTRY              *SpdSas;
  LIST_ENTRY              *EntryInsertBefore;
  LIST_ENTRY              *Entry;
  LIST_ENTRY              *Entry2;
  LIST_ENTRY              *NextEntry;
  IPSEC_SPD_ENTRY         *SpdEntry;
  IPSEC_SAD_ENTRY         *SadEntry;
  UINTN                   SpdEntrySize;
  UINTN                   Index;

  SpdSel        = (Selector == NULL) ? NULL : &Selector->SpdSelector;
  SpdData       = (Data == NULL) ? NULL : (EFI_IPSEC_SPD_DATA *) Data;
  InsertBefore  = (Context == NULL) ? NULL : &((EFI_IPSEC_CONFIG_SELECTOR *) Context)->SpdSelector;
  SpdList       = &mConfigData[IPsecConfigDataTypeSpd];

  if (SpdSel != NULL) {
    if (SpdSel->LocalAddress == NULL || SpdSel->RemoteAddress == NULL) {
      return EFI_INVALID_PARAMETER;
    }
  }

  if (SpdData != NULL) {
    if ((SpdData->Action == EfiIPsecActionProtect && SpdData->ProcessingPolicy == NULL) ||
        (SpdData->Action != EfiIPsecActionProtect && SpdData->ProcessingPolicy != NULL)
        ) {
      return EFI_INVALID_PARAMETER;
    }

    if (SpdData->Action == EfiIPsecActionProtect) {
      if ((SpdData->ProcessingPolicy->Mode == EfiIPsecTunnel && SpdData->ProcessingPolicy->TunnelOption == NULL) ||
          (SpdData->ProcessingPolicy->Mode != EfiIPsecTunnel && SpdData->ProcessingPolicy->TunnelOption != NULL)
          ) {
        return EFI_INVALID_PARAMETER;
      }
    }
  }
  //
  // The default behavior is to insert the node ahead of the header.
  //
  EntryInsertBefore = SpdList;

  //
  // Remove the existed SPD entry.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, SpdList) {

    SpdEntry = IPSEC_SPD_ENTRY_FROM_LIST (Entry);

    if (SpdSel == NULL || 
        CompareSpdSelector ((EFI_IPSEC_CONFIG_SELECTOR *) SpdEntry->Selector, (EFI_IPSEC_CONFIG_SELECTOR *) SpdSel)
        ) {
      //
      // Record the existed entry position to keep the original order.
      //
      EntryInsertBefore = SpdEntry->List.ForwardLink;
      RemoveEntryList (&SpdEntry->List);

      //
      // Update the reverse ref of SAD entry in the SPD.sas list.
      //
      SpdSas = &SpdEntry->Data->Sas;
      
      //
      // TODO: Deleted the related SAs.
      //
      NET_LIST_FOR_EACH (Entry2, SpdSas) {
        SadEntry                  = IPSEC_SAD_ENTRY_FROM_SPD (Entry2);
        SadEntry->Data->SpdEntry  = NULL;
      }
      
      //
      // Free the existed SPD entry
      //
      FreePool (SpdEntry);
    }
  }
  //
  // Return success here if only want to remove the SPD entry.
  //
  if (SpdData == NULL || SpdSel == NULL) {
    return EFI_SUCCESS;
  }
  //
  // Search the appointed entry position if InsertBefore is not NULL.
  //
  if (InsertBefore != NULL) {

    NET_LIST_FOR_EACH (Entry, SpdList) {
      SpdEntry = IPSEC_SPD_ENTRY_FROM_LIST (Entry);

      if (CompareSpdSelector (
            (EFI_IPSEC_CONFIG_SELECTOR *) SpdEntry->Selector,
            (EFI_IPSEC_CONFIG_SELECTOR *) InsertBefore
            )) {
        EntryInsertBefore = Entry;
        break;
      }
    }
  }

  //
  // Do Padding for the different Arch.
  //
  SpdEntrySize  = ALIGN_VARIABLE (sizeof (IPSEC_SPD_ENTRY));
  SpdEntrySize  = ALIGN_VARIABLE (SpdEntrySize + (UINTN)SIZE_OF_SPD_SELECTOR (SpdSel));
  SpdEntrySize += IpSecGetSizeOfEfiSpdData (SpdData);

  SpdEntry = AllocateZeroPool (SpdEntrySize);

  if (SpdEntry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Fix the address of Selector and Data buffer and copy them, which is
  // continous memory and close to the base structure of SPD entry.
  //
  SpdEntry->Selector  = (EFI_IPSEC_SPD_SELECTOR *) ALIGN_POINTER ((SpdEntry + 1), sizeof (UINTN));
  SpdEntry->Data      = (IPSEC_SPD_DATA *) ALIGN_POINTER (
                                            ((UINT8 *) SpdEntry->Selector + SIZE_OF_SPD_SELECTOR (SpdSel)),
                                            sizeof (UINTN)
                                            );

  DuplicateSpdSelector (
    (EFI_IPSEC_CONFIG_SELECTOR *) SpdEntry->Selector,
    (EFI_IPSEC_CONFIG_SELECTOR *) SpdSel,
    NULL
    );

  CopyMem (
    SpdEntry->Data->Name,
    SpdData->Name,
    sizeof (SpdData->Name)
    );
  SpdEntry->Data->PackageFlag      = SpdData->PackageFlag;
  SpdEntry->Data->TrafficDirection = SpdData->TrafficDirection;
  SpdEntry->Data->Action           = SpdData->Action;
  
  //
  // Fix the address of ProcessingPolicy and copy it if need, which is continous
  // memory and close to the base structure of SAD data.
  //
  if (SpdData->Action != EfiIPsecActionProtect) {
    SpdEntry->Data->ProcessingPolicy = NULL;
  } else {
    SpdEntry->Data->ProcessingPolicy = (EFI_IPSEC_PROCESS_POLICY *) ALIGN_POINTER (
                                                                      SpdEntry->Data + 1,
                                                                      sizeof (UINTN)
                                                                      );
    IpSecDuplicateProcessPolicy (SpdEntry->Data->ProcessingPolicy, SpdData->ProcessingPolicy);
  }
  //
  // Update the sas list of the new SPD entry.
  //
  InitializeListHead (&SpdEntry->Data->Sas);

  SadList = &mConfigData[IPsecConfigDataTypeSad];

  NET_LIST_FOR_EACH (Entry, SadList) {
    SadEntry = IPSEC_SAD_ENTRY_FROM_LIST (Entry);

    for (Index = 0; Index < SpdData->SaIdCount; Index++) {

      if (CompareSaId (
            (EFI_IPSEC_CONFIG_SELECTOR *) &SpdData->SaId[Index],
            (EFI_IPSEC_CONFIG_SELECTOR *) SadEntry->Id
            )) {
        if (SadEntry->Data->SpdEntry != NULL) {  
          RemoveEntryList (&SadEntry->BySpd);
        }
        InsertTailList (&SpdEntry->Data->Sas, &SadEntry->BySpd);
        SadEntry->Data->SpdEntry = SpdEntry;             
      }
    }
  }
  //
  // Insert the new SPD entry.
  //
  InsertTailList (EntryInsertBefore, &SpdEntry->List);

  return EFI_SUCCESS;
}

/**
  Set the security association information for the EFI IPsec driver.

  The IPsec configuration data has a unique selector/identifier separately to 
  identify a data entry.

  @param[in]  Selector           Pointer to an entry selector on operated 
                                 configuration data specified by DataType. 
                                 A NULL Selector causes the entire specified-type 
                                 configuration information to be flushed.
  @param[in]  Data               The data buffer to be set. The structure 
                                 of the data buffer should be EFI_IPSEC_SA_DATA.
  @param[in]  Context            Pointer to one entry selector which describes 
                                 the expected position the new data entry will 
                                 be added. If Context is NULL,the new entry will
                                 be appended the end of database.

  @retval EFI_OUT_OF_RESOURCED  The required system resource could not be allocated.
  @retval EFI_SUCCESS           The specified configuration data was obtained successfully.

**/
EFI_STATUS
SetSadEntry (
  IN EFI_IPSEC_CONFIG_SELECTOR       *Selector,
  IN VOID                            *Data,
  IN VOID                            *Context OPTIONAL
  )
{
  IPSEC_SAD_ENTRY   *SadEntry;
  IPSEC_SPD_ENTRY   *SpdEntry;
  LIST_ENTRY        *Entry;
  LIST_ENTRY        *NextEntry;
  LIST_ENTRY        *SadList;
  LIST_ENTRY        *SpdList;
  EFI_IPSEC_SA_ID   *SaId;
  EFI_IPSEC_SA_DATA2 *SaData;
  EFI_IPSEC_SA_ID   *InsertBefore;
  LIST_ENTRY        *EntryInsertBefore;
  UINTN             SadEntrySize;
  
  SaId          = (Selector == NULL) ? NULL : &Selector->SaId;
  SaData        = (Data == NULL) ? NULL : (EFI_IPSEC_SA_DATA2 *) Data;
  InsertBefore  = (Context == NULL) ? NULL : &((EFI_IPSEC_CONFIG_SELECTOR *) Context)->SaId;
  SadList       = &mConfigData[IPsecConfigDataTypeSad];

  //
  // The default behavior is to insert the node ahead of the header.
  //
  EntryInsertBefore = SadList;

  //
  // Remove the existed SAD entry.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, SadList) {

    SadEntry = IPSEC_SAD_ENTRY_FROM_LIST (Entry);

    if (SaId == NULL || 
        CompareSaId (
          (EFI_IPSEC_CONFIG_SELECTOR *) SadEntry->Id,
          (EFI_IPSEC_CONFIG_SELECTOR *) SaId
          )) {
      //
      // Record the existed entry position to keep the original order.
      //
      EntryInsertBefore = SadEntry->List.ForwardLink;

      //
      // Update the related SAD.byspd field.
      //
      if (SadEntry->Data->SpdEntry != NULL) {
        RemoveEntryList (&SadEntry->BySpd);
      }

      RemoveEntryList (&SadEntry->List);
      FreePool (SadEntry);
    }
  }
  //
  // Return success here if only want to remove the SAD entry
  //
  if (SaData == NULL || SaId == NULL) {
    return EFI_SUCCESS;
  }
  //
  // Search the appointed entry position if InsertBefore is not NULL.
  //
  if (InsertBefore != NULL) {

    NET_LIST_FOR_EACH (Entry, SadList) {
      SadEntry = IPSEC_SAD_ENTRY_FROM_LIST (Entry);

      if (CompareSaId (
           (EFI_IPSEC_CONFIG_SELECTOR *) SadEntry->Id,
           (EFI_IPSEC_CONFIG_SELECTOR *) InsertBefore
           )) {
        EntryInsertBefore = Entry;
        break;
      }
    }
  }

  //
  // Do Padding for different Arch.
  //
  SadEntrySize  = ALIGN_VARIABLE (sizeof (IPSEC_SAD_ENTRY));
  SadEntrySize  = ALIGN_VARIABLE (SadEntrySize + sizeof (EFI_IPSEC_SA_ID));
  SadEntrySize  = ALIGN_VARIABLE (SadEntrySize + sizeof (IPSEC_SAD_DATA));
  
  if (SaId->Proto == EfiIPsecAH) {
    SadEntrySize += SaData->AlgoInfo.AhAlgoInfo.AuthKeyLength;
  } else {
    SadEntrySize  = ALIGN_VARIABLE (SadEntrySize + SaData->AlgoInfo.EspAlgoInfo.AuthKeyLength);
    SadEntrySize += ALIGN_VARIABLE (SaData->AlgoInfo.EspAlgoInfo.EncKeyLength);
  }

  if (SaData->SpdSelector != NULL) {
    SadEntrySize += SadEntrySize + (UINTN)SIZE_OF_SPD_SELECTOR (SaData->SpdSelector);
  }
  SadEntry      = AllocateZeroPool (SadEntrySize);

  if (SadEntry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Fix the address of Id and Data buffer and copy them, which is
  // continous memory and close to the base structure of SAD entry.
  //
  SadEntry->Id    = (EFI_IPSEC_SA_ID *) ALIGN_POINTER ((SadEntry + 1), sizeof (UINTN));
  SadEntry->Data  = (IPSEC_SAD_DATA *) ALIGN_POINTER ((SadEntry->Id + 1), sizeof (UINTN));

  CopyMem (SadEntry->Id, SaId, sizeof (EFI_IPSEC_SA_ID));

  SadEntry->Data->Mode                  = SaData->Mode;
  SadEntry->Data->SequenceNumber        = SaData->SNCount;
  SadEntry->Data->AntiReplayWindowSize  = SaData->AntiReplayWindows;

  ZeroMem (
    &SadEntry->Data->AntiReplayBitmap,
    sizeof (SadEntry->Data->AntiReplayBitmap)
    );

  ZeroMem (
    &SadEntry->Data->AlgoInfo,
    sizeof (EFI_IPSEC_ALGO_INFO)
    );

  SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthAlgoId     = SaData->AlgoInfo.EspAlgoInfo.AuthAlgoId;
  SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthKeyLength  = SaData->AlgoInfo.EspAlgoInfo.AuthKeyLength;

  if (SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthKeyLength != 0) {
    SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthKey = (VOID *) ALIGN_POINTER ((SadEntry->Data + 1), sizeof (UINTN));
    CopyMem (
      SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthKey,
      SaData->AlgoInfo.EspAlgoInfo.AuthKey,
      SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthKeyLength
      );
  }

  if (SaId->Proto == EfiIPsecESP) {
    SadEntry->Data->AlgoInfo.EspAlgoInfo.EncAlgoId    = SaData->AlgoInfo.EspAlgoInfo.EncAlgoId;
    SadEntry->Data->AlgoInfo.EspAlgoInfo.EncKeyLength = SaData->AlgoInfo.EspAlgoInfo.EncKeyLength;

    if (SadEntry->Data->AlgoInfo.EspAlgoInfo.EncKeyLength != 0) {
      SadEntry->Data->AlgoInfo.EspAlgoInfo.EncKey = (VOID *) ALIGN_POINTER (
                                                               ((UINT8 *) (SadEntry->Data + 1) + 
                                                                 SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthKeyLength),
                                                                 sizeof (UINTN)
                                                                 );
      CopyMem (
        SadEntry->Data->AlgoInfo.EspAlgoInfo.EncKey,
        SaData->AlgoInfo.EspAlgoInfo.EncKey,
        SadEntry->Data->AlgoInfo.EspAlgoInfo.EncKeyLength
        );
    }
  }

  CopyMem (
    &SadEntry->Data->SaLifetime,
    &SaData->SaLifetime,
    sizeof (EFI_IPSEC_SA_LIFETIME)
    );

  SadEntry->Data->PathMTU     = SaData->PathMTU;
  SadEntry->Data->SpdSelector = NULL;
  SadEntry->Data->ESNEnabled  = FALSE;
  SadEntry->Data->ManualSet   = SaData->ManualSet;

  //
  // Copy Tunnel Source/Destination Address
  //
  if (SaData->Mode == EfiIPsecTunnel) {
    CopyMem (
      &SadEntry->Data->TunnelDestAddress,
      &SaData->TunnelDestinationAddress,
      sizeof (EFI_IP_ADDRESS)
      );
    CopyMem (
      &SadEntry->Data->TunnelSourceAddress,
      &SaData->TunnelSourceAddress,
      sizeof (EFI_IP_ADDRESS)
      );
  }
  //
  // Update the spd.sas list of the spd entry specified by SAD selector
  //
  SpdList = &mConfigData[IPsecConfigDataTypeSpd];

  for (Entry = SpdList->ForwardLink; Entry != SpdList && SaData->SpdSelector != NULL; Entry = Entry->ForwardLink) {

    SpdEntry = IPSEC_SPD_ENTRY_FROM_LIST (Entry);
    if (IsSubSpdSelector (
          (EFI_IPSEC_CONFIG_SELECTOR *) SaData->SpdSelector,
          (EFI_IPSEC_CONFIG_SELECTOR *) SpdEntry->Selector
          ) && SpdEntry->Data->Action == EfiIPsecActionProtect) {
      SadEntry->Data->SpdEntry = SpdEntry;
      SadEntry->Data->SpdSelector = (EFI_IPSEC_SPD_SELECTOR *)((UINT8 *)SadEntry +
                                                                SadEntrySize -
                                                                (UINTN)SIZE_OF_SPD_SELECTOR (SaData->SpdSelector)
                                                                );
      DuplicateSpdSelector (
       (EFI_IPSEC_CONFIG_SELECTOR *) SadEntry->Data->SpdSelector,
       (EFI_IPSEC_CONFIG_SELECTOR *) SaData->SpdSelector,
       NULL
       );
      InsertTailList (&SpdEntry->Data->Sas, &SadEntry->BySpd);
    }
  }
  //
  // Insert the new SAD entry.
  //
  InsertTailList (EntryInsertBefore, &SadEntry->List);

  return EFI_SUCCESS;
}

/**
  Set the peer authorization configuration information for the EFI IPsec driver.

  The IPsec configuration data has a unique selector/identifier separately to 
  identify a data entry.

  @param[in]  Selector           Pointer to an entry selector on operated 
                                 configuration data specified by DataType. 
                                 A NULL Selector causes the entire specified-type 
                                 configuration information to be flushed.
  @param[in]  Data               The data buffer to be set. The structure 
                                 of the data buffer should be EFI_IPSEC_PAD_DATA.
  @param[in]  Context            Pointer to one entry selector that describes 
                                 the expected position the new data entry will 
                                 be added. If Context is NULL, the new entry will
                                 be appended the end of database.

  @retval EFI_OUT_OF_RESOURCES  The required system resources could not be allocated.
  @retval EFI_SUCCESS           The specified configuration data was obtained successfully.

**/
EFI_STATUS
SetPadEntry (
  IN EFI_IPSEC_CONFIG_SELECTOR       *Selector,
  IN VOID                            *Data,
  IN VOID                            *Context OPTIONAL
  )
{
  IPSEC_PAD_ENTRY     *PadEntry;
  EFI_IPSEC_PAD_ID    *PadId;
  EFI_IPSEC_PAD_DATA  *PadData;
  LIST_ENTRY          *PadList;
  LIST_ENTRY          *Entry;
  LIST_ENTRY          *NextEntry;
  EFI_IPSEC_PAD_ID    *InsertBefore;
  LIST_ENTRY          *EntryInsertBefore;
  UINTN               PadEntrySize;
   
  PadId         = (Selector == NULL) ? NULL : &Selector->PadId;
  PadData       = (Data == NULL) ? NULL : (EFI_IPSEC_PAD_DATA *) Data;
  InsertBefore  = (Context == NULL) ? NULL : &((EFI_IPSEC_CONFIG_SELECTOR *) Context)->PadId;
  PadList       = &mConfigData[IPsecConfigDataTypePad];

  //
  // The default behavior is to insert the node ahead of the header.
  //
  EntryInsertBefore = PadList;

  //
  // Remove the existed pad entry.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, PadList) {

    PadEntry = IPSEC_PAD_ENTRY_FROM_LIST (Entry);

    if (PadId == NULL || 
        ComparePadId ((EFI_IPSEC_CONFIG_SELECTOR *) PadEntry->Id, (EFI_IPSEC_CONFIG_SELECTOR *) PadId)
        ) {
      //
      // Record the existed entry position to keep the original order.
      //
      EntryInsertBefore = PadEntry->List.ForwardLink;
      RemoveEntryList (&PadEntry->List);

      FreePool (PadEntry);
    }
  }
  //
  // Return success here if only want to remove the pad entry
  //
  if (PadData == NULL || PadId == NULL) {
    return EFI_SUCCESS;
  }
  //
  // Search the appointed entry position if InsertBefore is not NULL.
  //
  if (InsertBefore != NULL) {

    NET_LIST_FOR_EACH (Entry, PadList) {
      PadEntry = IPSEC_PAD_ENTRY_FROM_LIST (Entry);

      if (ComparePadId (
            (EFI_IPSEC_CONFIG_SELECTOR *) PadEntry->Id,
            (EFI_IPSEC_CONFIG_SELECTOR *) InsertBefore
            )) {
        EntryInsertBefore = Entry;
        break;
      }
    }
  }

  //
  // Do PADDING for different arch.
  //
  PadEntrySize  = ALIGN_VARIABLE (sizeof (IPSEC_PAD_ENTRY));
  PadEntrySize  = ALIGN_VARIABLE (PadEntrySize + sizeof (EFI_IPSEC_PAD_ID));
  PadEntrySize  = ALIGN_VARIABLE (PadEntrySize + sizeof (EFI_IPSEC_PAD_DATA));
  PadEntrySize  = ALIGN_VARIABLE (PadEntrySize + (PadData->AuthData != NULL ? PadData->AuthDataSize : 0));
  PadEntrySize += PadData->RevocationData != NULL ? PadData->RevocationDataSize : 0;

  PadEntry      = AllocateZeroPool (PadEntrySize);

  if (PadEntry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Fix the address of Id and Data buffer and copy them, which is
  // continous memory and close to the base structure of pad entry.
  //
  PadEntry->Id    = (EFI_IPSEC_PAD_ID *) ALIGN_POINTER ((PadEntry + 1), sizeof (UINTN));
  PadEntry->Data  = (EFI_IPSEC_PAD_DATA *) ALIGN_POINTER ((PadEntry->Id + 1), sizeof (UINTN));

  CopyMem (PadEntry->Id, PadId, sizeof (EFI_IPSEC_PAD_ID));

  PadEntry->Data->AuthProtocol  = PadData->AuthProtocol;
  PadEntry->Data->AuthMethod    = PadData->AuthMethod;
  PadEntry->Data->IkeIdFlag     = PadData->IkeIdFlag;

  if (PadData->AuthData != NULL) {
    PadEntry->Data->AuthDataSize  = PadData->AuthDataSize;
    PadEntry->Data->AuthData      = (VOID *) ALIGN_POINTER (PadEntry->Data + 1, sizeof (UINTN));
    CopyMem (
      PadEntry->Data->AuthData,
      PadData->AuthData,
      PadData->AuthDataSize
      );
  } else {
    PadEntry->Data->AuthDataSize  = 0;
    PadEntry->Data->AuthData      = NULL;
  }

  if (PadData->RevocationData != NULL) {
    PadEntry->Data->RevocationDataSize  = PadData->RevocationDataSize;
    PadEntry->Data->RevocationData      = (VOID *) ALIGN_POINTER (
                                                    ((UINT8 *) (PadEntry->Data + 1) + PadData->AuthDataSize),
                                                    sizeof (UINTN)
                                                    );
    CopyMem (
      PadEntry->Data->RevocationData,
      PadData->RevocationData,
      PadData->RevocationDataSize
      );
  } else {
    PadEntry->Data->RevocationDataSize  = 0;
    PadEntry->Data->RevocationData      = NULL;
  }
  //
  // Insert the new pad entry.
  //
  InsertTailList (EntryInsertBefore, &PadEntry->List);

  return EFI_SUCCESS;
}

/**
  This function lookup the data entry from IPsec SPD. Return the configuration 
  value of the specified SPD Entry.

  @param[in]      Selector      Pointer to an entry selector which is an identifier 
                                of the SPD entry.
  @param[in, out] DataSize      On output the size of data returned in Data.
  @param[out]     Data          The buffer to return the contents of the IPsec 
                                configuration data. The type of the data buffer 
                                is associated with the DataType. 
 
  @retval EFI_SUCCESS           The specified configuration data was obtained successfully.
  @retval EFI_INVALID_PARAMETER Data is NULL and *DataSize is not zero.
  @retval EFI_NOT_FOUND         The configuration data specified by Selector is not found.
  @retval EFI_BUFFER_TOO_SMALL  The DataSize is too small for the result. DataSize has been
                                updated with the size needed to complete the request.

**/
EFI_STATUS
GetSpdEntry (
  IN     EFI_IPSEC_CONFIG_SELECTOR       *Selector,
  IN OUT UINTN                           *DataSize,
     OUT VOID                            *Data
  )
{
  IPSEC_SPD_ENTRY         *SpdEntry;
  IPSEC_SAD_ENTRY         *SadEntry;
  EFI_IPSEC_SPD_SELECTOR  *SpdSel;
  EFI_IPSEC_SPD_DATA      *SpdData;
  LIST_ENTRY              *SpdList;
  LIST_ENTRY              *SpdSas;
  LIST_ENTRY              *Entry;
  UINTN                   RequiredSize;

  SpdSel  = &Selector->SpdSelector;
  SpdData = (EFI_IPSEC_SPD_DATA *) Data;
  SpdList = &mConfigData[IPsecConfigDataTypeSpd];

  NET_LIST_FOR_EACH (Entry, SpdList) {
    SpdEntry = IPSEC_SPD_ENTRY_FROM_LIST (Entry);

    //
    // Find the required SPD entry
    //
    if (CompareSpdSelector (
          (EFI_IPSEC_CONFIG_SELECTOR *) SpdSel,
          (EFI_IPSEC_CONFIG_SELECTOR *) SpdEntry->Selector
          )) {

      RequiredSize = IpSecGetSizeOfSpdData (SpdEntry->Data);
      if (*DataSize < RequiredSize) {
        *DataSize = RequiredSize;
        return EFI_BUFFER_TOO_SMALL;
      }

      if (SpdData == NULL) {
        return EFI_INVALID_PARAMETER;
      }

      *DataSize = RequiredSize;

      //
      // Extract and fill all SaId array from the SPD.sas list
      //
      SpdSas              = &SpdEntry->Data->Sas;
      SpdData->SaIdCount  = 0;

      NET_LIST_FOR_EACH (Entry, SpdSas) {
        SadEntry = IPSEC_SAD_ENTRY_FROM_SPD (Entry);
        CopyMem (
          &SpdData->SaId[SpdData->SaIdCount++],
          SadEntry->Id,
          sizeof (EFI_IPSEC_SA_ID)
          );
      }
      //
      // Fill the other fields in SPD data.
      //
      CopyMem (SpdData->Name, SpdEntry->Data->Name, sizeof (SpdData->Name));

      SpdData->PackageFlag      = SpdEntry->Data->PackageFlag;
      SpdData->TrafficDirection = SpdEntry->Data->TrafficDirection;
      SpdData->Action           = SpdEntry->Data->Action;
      
      if (SpdData->Action != EfiIPsecActionProtect) {
        SpdData->ProcessingPolicy = NULL;
      } else {
        SpdData->ProcessingPolicy = (EFI_IPSEC_PROCESS_POLICY *) ((UINT8 *) SpdData + sizeof (EFI_IPSEC_SPD_DATA) + (SpdData->SaIdCount - 1) * sizeof (EFI_IPSEC_SA_ID));

        IpSecDuplicateProcessPolicy (
          SpdData->ProcessingPolicy,
          SpdEntry->Data->ProcessingPolicy
          );
      }

      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  This function lookup the data entry from IPsec SAD. Return the configuration 
  value of the specified SAD Entry.

  @param[in]      Selector      Pointer to an entry selector which is an identifier 
                                of the SAD entry.
  @param[in, out] DataSize      On output, the size of data returned in Data.
  @param[out]     Data          The buffer to return the contents of the IPsec 
                                configuration data. The type of the data buffer 
                                is associated with the DataType. 
 
  @retval EFI_SUCCESS           The specified configuration data was obtained successfully.
  @retval EFI_NOT_FOUND         The configuration data specified by Selector is not found.
  @retval EFI_BUFFER_TOO_SMALL  The DataSize is too small for the result. DataSize has been
                                updated with the size needed to complete the request.

**/
EFI_STATUS
GetSadEntry (
  IN     EFI_IPSEC_CONFIG_SELECTOR     *Selector,
  IN OUT UINTN                         *DataSize,
     OUT VOID                          *Data
  )
{
  IPSEC_SAD_ENTRY   *SadEntry;
  LIST_ENTRY        *Entry;
  LIST_ENTRY        *SadList;
  EFI_IPSEC_SA_ID   *SaId;
  EFI_IPSEC_SA_DATA2 *SaData;
  UINTN             RequiredSize;

  SaId    = &Selector->SaId;
  SaData  = (EFI_IPSEC_SA_DATA2 *) Data;
  SadList = &mConfigData[IPsecConfigDataTypeSad];

  NET_LIST_FOR_EACH (Entry, SadList) {
    SadEntry = IPSEC_SAD_ENTRY_FROM_LIST (Entry);

    //
    // Find the required SAD entry.
    //
    if (CompareSaId (
         (EFI_IPSEC_CONFIG_SELECTOR *) SaId,
         (EFI_IPSEC_CONFIG_SELECTOR *) SadEntry->Id
         )) {
      //
      // Calculate the required size of the SAD entry.
      // Data Layout is follows:
      // |EFI_IPSEC_SA_DATA
      // |AuthKey
      // |EncryptKey  (Optional)
      // |SpdSelector (Optional)     
      // 
      RequiredSize  = ALIGN_VARIABLE (sizeof (EFI_IPSEC_SA_DATA2));

      if (SaId->Proto == EfiIPsecAH) {
        RequiredSize  = ALIGN_VARIABLE (RequiredSize + SadEntry->Data->AlgoInfo.AhAlgoInfo.AuthKeyLength);
      } else {
        RequiredSize  = ALIGN_VARIABLE (RequiredSize + SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthKeyLength);
        RequiredSize  = ALIGN_VARIABLE (RequiredSize + SadEntry->Data->AlgoInfo.EspAlgoInfo.EncKeyLength);
      }

      if (SadEntry->Data->SpdSelector != NULL) {
        RequiredSize += SIZE_OF_SPD_SELECTOR (SadEntry->Data->SpdSelector);
      }
      
      if (*DataSize < RequiredSize) {
        *DataSize = RequiredSize;
        return EFI_BUFFER_TOO_SMALL;
      }
      
      //
      // Fill the data fields of SAD entry.
      //
      *DataSize                 = RequiredSize;
      SaData->Mode              = SadEntry->Data->Mode;
      SaData->SNCount           = SadEntry->Data->SequenceNumber;
      SaData->AntiReplayWindows = SadEntry->Data->AntiReplayWindowSize;

      CopyMem (
        &SaData->SaLifetime,
        &SadEntry->Data->SaLifetime,
        sizeof (EFI_IPSEC_SA_LIFETIME)
        );

      ZeroMem (
        &SaData->AlgoInfo,
        sizeof (EFI_IPSEC_ALGO_INFO)
        );

      if (SaId->Proto == EfiIPsecAH) {
        //
        // Copy AH alogrithm INFO to SaData
        //
        SaData->AlgoInfo.AhAlgoInfo.AuthAlgoId    = SadEntry->Data->AlgoInfo.AhAlgoInfo.AuthAlgoId;
        SaData->AlgoInfo.AhAlgoInfo.AuthKeyLength = SadEntry->Data->AlgoInfo.AhAlgoInfo.AuthKeyLength;
        if (SaData->AlgoInfo.AhAlgoInfo.AuthKeyLength != 0) {
          SaData->AlgoInfo.AhAlgoInfo.AuthKey = (VOID *) ALIGN_POINTER ((SaData + 1), sizeof (UINTN));
          CopyMem (
            SaData->AlgoInfo.AhAlgoInfo.AuthKey,
            SadEntry->Data->AlgoInfo.AhAlgoInfo.AuthKey,
            SaData->AlgoInfo.AhAlgoInfo.AuthKeyLength
            );
        }
      } else if (SaId->Proto == EfiIPsecESP) {
        //
        // Copy ESP alogrithem INFO to SaData
        //
        SaData->AlgoInfo.EspAlgoInfo.AuthAlgoId     = SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthAlgoId;
        SaData->AlgoInfo.EspAlgoInfo.AuthKeyLength  = SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthKeyLength;
        if (SaData->AlgoInfo.EspAlgoInfo.AuthKeyLength != 0) {
          SaData->AlgoInfo.EspAlgoInfo.AuthKey = (VOID *) ALIGN_POINTER ((SaData + 1), sizeof (UINTN));
          CopyMem (
            SaData->AlgoInfo.EspAlgoInfo.AuthKey,
            SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthKey,
            SaData->AlgoInfo.EspAlgoInfo.AuthKeyLength
            );
        }

        SaData->AlgoInfo.EspAlgoInfo.EncAlgoId    = SadEntry->Data->AlgoInfo.EspAlgoInfo.EncAlgoId;
        SaData->AlgoInfo.EspAlgoInfo.EncKeyLength = SadEntry->Data->AlgoInfo.EspAlgoInfo.EncKeyLength;

        if (SaData->AlgoInfo.EspAlgoInfo.EncKeyLength != 0) {
          SaData->AlgoInfo.EspAlgoInfo.EncKey = (VOID *) ALIGN_POINTER (
                                                          ((UINT8 *) (SaData + 1) +
                                                            SaData->AlgoInfo.EspAlgoInfo.AuthKeyLength),
                                                            sizeof (UINTN)
                                                            );
          CopyMem (
            SaData->AlgoInfo.EspAlgoInfo.EncKey,
            SadEntry->Data->AlgoInfo.EspAlgoInfo.EncKey,
            SaData->AlgoInfo.EspAlgoInfo.EncKeyLength
            );
        }
      }

      SaData->PathMTU = SadEntry->Data->PathMTU;

      //
      // Fill Tunnel Address if it is Tunnel Mode
      //
      if (SadEntry->Data->Mode == EfiIPsecTunnel) {
        CopyMem (
          &SaData->TunnelDestinationAddress,
          &SadEntry->Data->TunnelDestAddress,
          sizeof (EFI_IP_ADDRESS)
          );
        CopyMem (
          &SaData->TunnelSourceAddress,
          &SadEntry->Data->TunnelSourceAddress,
          sizeof (EFI_IP_ADDRESS)
          );
      }
      //
      // Fill the spd selector field of SAD data
      //
      if (SadEntry->Data->SpdSelector != NULL) {

        SaData->SpdSelector = (EFI_IPSEC_SPD_SELECTOR *) (
                                (UINT8 *)SaData +
                                RequiredSize -
                                SIZE_OF_SPD_SELECTOR (SadEntry->Data->SpdSelector)
                                );
       
        DuplicateSpdSelector (
          (EFI_IPSEC_CONFIG_SELECTOR *) SaData->SpdSelector,
          (EFI_IPSEC_CONFIG_SELECTOR *) SadEntry->Data->SpdSelector,
          NULL
          );

      } else {

        SaData->SpdSelector = NULL;
      }

      SaData->ManualSet = SadEntry->Data->ManualSet;

      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  This function lookup the data entry from IPsec PAD. Return the configuration 
  value of the specified PAD Entry.

  @param[in]      Selector      Pointer to an entry selector which is an identifier 
                                of the PAD entry.
  @param[in, out] DataSize      On output the size of data returned in Data.
  @param[out]     Data          The buffer to return the contents of the IPsec 
                                configuration data. The type of the data buffer 
                                is associated with the DataType. 
 
  @retval EFI_SUCCESS           The specified configuration data was obtained successfully.
  @retval EFI_NOT_FOUND         The configuration data specified by Selector is not found.
  @retval EFI_BUFFER_TOO_SMALL  The DataSize is too small for the result. DataSize has been
                                updated with the size needed to complete the request.

**/
EFI_STATUS
GetPadEntry (
  IN     EFI_IPSEC_CONFIG_SELECTOR   *Selector,
  IN OUT UINTN                       *DataSize,
     OUT VOID                        *Data
  )
{
  IPSEC_PAD_ENTRY     *PadEntry;
  LIST_ENTRY          *PadList;
  LIST_ENTRY          *Entry;
  EFI_IPSEC_PAD_ID    *PadId;
  EFI_IPSEC_PAD_DATA  *PadData;
  UINTN               RequiredSize;

  PadId   = &Selector->PadId;
  PadData = (EFI_IPSEC_PAD_DATA *) Data;
  PadList = &mConfigData[IPsecConfigDataTypePad];

  NET_LIST_FOR_EACH (Entry, PadList) {
    PadEntry = IPSEC_PAD_ENTRY_FROM_LIST (Entry);

    //
    // Find the required pad entry.
    //
    if (ComparePadId (
          (EFI_IPSEC_CONFIG_SELECTOR *) PadId,
          (EFI_IPSEC_CONFIG_SELECTOR *) PadEntry->Id
          )) {
      //
      // Calculate the required size of the pad entry.
      //
      RequiredSize  = ALIGN_VARIABLE (sizeof (EFI_IPSEC_PAD_DATA));
      RequiredSize  = ALIGN_VARIABLE (RequiredSize + PadEntry->Data->AuthDataSize);
      RequiredSize += PadEntry->Data->RevocationDataSize;

      if (*DataSize < RequiredSize) {
        *DataSize = RequiredSize;
        return EFI_BUFFER_TOO_SMALL;
      }
      //
      // Fill the data fields of pad entry
      //
      *DataSize             = RequiredSize;
      PadData->AuthProtocol = PadEntry->Data->AuthProtocol;
      PadData->AuthMethod   = PadEntry->Data->AuthMethod;
      PadData->IkeIdFlag    = PadEntry->Data->IkeIdFlag;

      //
      // Copy Authentication data.
      //
      if (PadEntry->Data->AuthData != NULL) {

        PadData->AuthDataSize = PadEntry->Data->AuthDataSize;
        PadData->AuthData     = (VOID *) ALIGN_POINTER ((PadData + 1), sizeof (UINTN));
        CopyMem (
          PadData->AuthData,
          PadEntry->Data->AuthData,
          PadData->AuthDataSize
          );
      } else {

        PadData->AuthDataSize = 0;
        PadData->AuthData     = NULL;
      }
      //
      // Copy Revocation Data.
      //
      if (PadEntry->Data->RevocationData != NULL) {

        PadData->RevocationDataSize = PadEntry->Data->RevocationDataSize;
        PadData->RevocationData     = (VOID *) ALIGN_POINTER (
                                                 ((UINT8 *) (PadData + 1) + PadData->AuthDataSize),
                                                  sizeof (UINTN)
                                                  );
        CopyMem (
          PadData->RevocationData,
          PadEntry->Data->RevocationData,
          PadData->RevocationDataSize
          );
      } else {

        PadData->RevocationDataSize = 0;
        PadData->RevocationData     = NULL;
      }

      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Copy Source Process Policy to the Destination Process Policy.

  @param[in]  Dst                  Pointer to the Source Process Policy.
  @param[in]  Src                  Pointer to the Destination Process Policy.

**/
VOID
IpSecDuplicateProcessPolicy (
  IN EFI_IPSEC_PROCESS_POLICY            *Dst,
  IN EFI_IPSEC_PROCESS_POLICY            *Src
  )
{
  //
  // Firstly copy the structure content itself.
  //
  CopyMem (Dst, Src, sizeof (EFI_IPSEC_PROCESS_POLICY));

  //
  // Recursively copy the tunnel option if needed.
  //
  if (Dst->Mode != EfiIPsecTunnel) {
    ASSERT (Dst->TunnelOption == NULL);
  } else {
    Dst->TunnelOption = (EFI_IPSEC_TUNNEL_OPTION *) ALIGN_POINTER ((Dst + 1), sizeof (UINTN));
    CopyMem (
      Dst->TunnelOption,
      Src->TunnelOption,
      sizeof (EFI_IPSEC_TUNNEL_OPTION)
      );
  }
}

/**
  Calculate the a whole size of EFI_IPSEC_SPD_DATA, which includes the buffer size pointed
  to by the pointer members.

  @param[in]  SpdData             Pointer to a specified EFI_IPSEC_SPD_DATA.

  @return the whole size the specified EFI_IPSEC_SPD_DATA.

**/
UINTN
IpSecGetSizeOfEfiSpdData (
  IN EFI_IPSEC_SPD_DATA               *SpdData
  )
{
  UINTN Size;

  Size = ALIGN_VARIABLE (sizeof (IPSEC_SPD_DATA));

  if (SpdData->Action == EfiIPsecActionProtect) {
    Size = ALIGN_VARIABLE (Size + sizeof (EFI_IPSEC_PROCESS_POLICY));

    if (SpdData->ProcessingPolicy->Mode == EfiIPsecTunnel) {
      Size = ALIGN_VARIABLE (Size + sizeof (EFI_IPSEC_TUNNEL_OPTION));
    }
  }

  return Size;
}

/**
  Calculate the a whole size of IPSEC_SPD_DATA which includes the buffer size pointed
  to by the pointer members and the buffer size used by the Sa List. 

  @param[in]  SpdData       Pointer to the specified IPSEC_SPD_DATA.

  @return the whole size of IPSEC_SPD_DATA.

**/
UINTN
IpSecGetSizeOfSpdData (
  IN IPSEC_SPD_DATA                   *SpdData
  )
{
  UINTN       Size;
  LIST_ENTRY  *Link;

  Size = sizeof (EFI_IPSEC_SPD_DATA) - sizeof (EFI_IPSEC_SA_ID);

  if (SpdData->Action == EfiIPsecActionProtect) {
    Size += sizeof (EFI_IPSEC_PROCESS_POLICY);

    if (SpdData->ProcessingPolicy->Mode == EfiIPsecTunnel) {
      Size += sizeof (EFI_IPSEC_TUNNEL_OPTION);
    }
  }

  NET_LIST_FOR_EACH (Link, &SpdData->Sas) {
    Size += sizeof (EFI_IPSEC_SA_ID);
  }

  return Size;
}

/**
  Get the IPsec Variable.

  Get the all variables which start with the string contained in VaraiableName.
  Since all IPsec related variable store in continual space, those kinds of 
  variable can be searched by the EfiGetNextVariableName. Those variables also are 
  returned in a continual buffer.
  
  @param[in]      VariableName          Pointer to a specified Variable Name.
  @param[in]      VendorGuid            Pointer to a specified Vendor Guid.
  @param[in]      Attributes            Point to memory location to return the attributes 
                                        of variable. If the point is NULL, the parameter 
                                        would be ignored.
  @param[in, out] DataSize              As input, point to the maximum size of return 
                                        Data-Buffer. As output, point to the actual 
                                        size of the returned Data-Buffer.
  @param[in]      Data                  Point to return Data-Buffer.
        
  @retval  EFI_ABORTED           If the Variable size which contained in the variable
                                 structure doesn't match the variable size obtained 
                                 from the EFIGetVariable.
  @retval  EFI_BUFFER_TOO_SMALL  The DataSize is too small for the result. DataSize has
                                 been updated with the size needed to complete the request.   
  @retval  EFI_SUCCESS           The function completed successfully.
  @retval  others                Other errors found during the variable getting.
**/
EFI_STATUS
IpSecGetVariable (
  IN     CHAR16                       *VariableName,
  IN     EFI_GUID                     *VendorGuid,
  IN     UINT32                       *Attributes, OPTIONAL
  IN OUT UINTN                        *DataSize,
  IN     VOID                         *Data
  )
{
  EFI_STATUS            Status;
  EFI_GUID              VendorGuidI;
  UINTN                 VariableNameLength;
  CHAR16                *VariableNameI;
  UINTN                 VariableNameISize;
  UINTN                 VariableNameISizeNew;
  UINTN                 VariableIndex;
  UINTN                 VariableCount;
  IP_SEC_VARIABLE_INFO  IpSecVariableInfo;
  UINTN                 DataSizeI;

  //
  // The variable name constructor is "VariableName + Info/0001/0002/... + NULL".
  // So the varialbe name is like "VariableNameInfo", "VariableName0001", ...
  // "VariableNameNULL".
  //
  VariableNameLength  = StrLen (VariableName);
  VariableNameISize   = (VariableNameLength + 5) * sizeof (CHAR16);
  VariableNameI       = AllocateZeroPool (VariableNameISize);
  ASSERT (VariableNameI != NULL);
  
  //
  // Construct the varible name of ipsecconfig meta data.
  //
  UnicodeSPrint (VariableNameI, VariableNameISize, L"%s%s", VariableName, L"Info");

  DataSizeI = sizeof (IpSecVariableInfo);

  Status = gRT->GetVariable (
                  VariableNameI,
                  VendorGuid,
                  Attributes,
                  &DataSizeI,
                  &IpSecVariableInfo
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  if (*DataSize < IpSecVariableInfo.VariableSize) {
    *DataSize = IpSecVariableInfo.VariableSize;
    Status    = EFI_BUFFER_TOO_SMALL;
    goto ON_EXIT;
  }

  VariableCount     = IpSecVariableInfo.VariableCount;
  VariableNameI[0]  = L'\0';

  while (VariableCount != 0) {
    //
    // Get the variable name one by one in the variable database.
    //
    VariableNameISizeNew = VariableNameISize;
    Status = gRT->GetNextVariableName (
                    &VariableNameISizeNew,
                    VariableNameI,
                    &VendorGuidI
                    );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      VariableNameI = ReallocatePool (
                        VariableNameISize,
                        VariableNameISizeNew,
                        VariableNameI
                        );
      if (VariableNameI == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        break;
      }
      VariableNameISize = VariableNameISizeNew;

      Status = gRT->GetNextVariableName (
                      &VariableNameISizeNew,
                      VariableNameI,
                      &VendorGuidI
                      );
    }

    if (EFI_ERROR (Status)) {
      break;
    }
    //
    // Check whether the current variable is the required "ipsecconfig".
    //
    if (StrnCmp (VariableNameI, VariableName, VariableNameLength) == 0 ||
        CompareGuid (VendorGuid, &VendorGuidI)
        ) {
      //
      // Parse the variable count of the current ipsecconfig data.
      //
      VariableIndex = StrDecimalToUintn (VariableNameI + VariableNameLength);
      if (VariableIndex!= 0 && VariableIndex <= IpSecVariableInfo.VariableCount) {
        //
        // Get the variable size of the current ipsecconfig data.
        //
        DataSizeI = 0;
        Status = gRT->GetVariable (
                        VariableNameI,
                        VendorGuid,
                        Attributes,
                        &DataSizeI,
                        NULL
                        );
        ASSERT (Status == EFI_BUFFER_TOO_SMALL);
        //
        // Validate the variable count and variable size.
        //
        if (VariableIndex != IpSecVariableInfo.VariableCount) {
          //
          // If the varaibe is not the last one, its size should be the max
          // size of the single variable.
          //
          if (DataSizeI != IpSecVariableInfo.SingleVariableSize) {
            return EFI_ABORTED;
          }
        } else {
          if (DataSizeI != IpSecVariableInfo.VariableSize % IpSecVariableInfo.SingleVariableSize) {
            return EFI_ABORTED;
          }
        }
        //
        // Get the variable data of the current ipsecconfig data and
        // store it into user buffer continously.
        //
        Status = gRT->GetVariable (
                        VariableNameI,
                        VendorGuid,
                        Attributes,
                        &DataSizeI,
                        (UINT8 *) Data + (VariableIndex - 1) * IpSecVariableInfo.SingleVariableSize
                        );
        ASSERT_EFI_ERROR (Status);
        VariableCount--;
      }
    }
  }
  //
  // The VariableCount in "VariableNameInfo" varaible should have the correct
  // numbers of variables which name starts with VariableName.
  //
  if (VariableCount != 0) {
    Status = EFI_ABORTED;
  }

ON_EXIT:
  if (VariableNameI != NULL) {
    FreePool (VariableNameI);
  }
  return Status;
}

/**
  Set the IPsec variables.

  Set all IPsec variables which start with the specified variable name. Those variables
  are set one by one.

  @param[in]  VariableName  The name of the vendor's variable. It is a
                            Null-Terminated Unicode String.
  @param[in]  VendorGuid    Unify identifier for vendor.
  @param[in]  Attributes    Point to memory location to return the attributes of 
                            variable. If the point is NULL, the parameter would be ignored.
  @param[in]  DataSize      The size in bytes of Data-Buffer.
  @param[in]  Data          Points to the content of the variable.

  @retval  EFI_SUCCESS      The firmware successfully stored the variable and its data, as
                            defined by the Attributes.
  @retval  others           Storing the variables failed.      

**/
EFI_STATUS
IpSecSetVariable (
  IN CHAR16                           *VariableName,
  IN EFI_GUID                         *VendorGuid,
  IN UINT32                           Attributes,
  IN UINTN                            DataSize,
  IN VOID                             *Data
  )
{
  EFI_STATUS            Status;
  CHAR16                *VariableNameI;
  UINTN                 VariableNameSize;
  UINTN                 VariableIndex;
  IP_SEC_VARIABLE_INFO  IpSecVariableInfo;
  UINT64                MaximumVariableStorageSize;
  UINT64                RemainingVariableStorageSize;
  UINT64                MaximumVariableSize;

  Status = gRT->QueryVariableInfo (
                  Attributes,
                  &MaximumVariableStorageSize,
                  &RemainingVariableStorageSize,
                  &MaximumVariableSize
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // "VariableName + Info/0001/0002/... + NULL"
  //
  VariableNameSize  = (StrLen (VariableName) + 5) * sizeof (CHAR16);
  VariableNameI     = AllocateZeroPool (VariableNameSize);

  if (VariableNameI == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }
  //
  // Construct the variable of ipsecconfig general information. Like the total
  // numbers of the Ipsecconfig variables, the total size of all ipsecconfig variables.
  //
  UnicodeSPrint (VariableNameI, VariableNameSize, L"%s%s", VariableName, L"Info");
  MaximumVariableSize -= VariableNameSize;
  
  IpSecVariableInfo.VariableCount       = (UINT32) ((DataSize + (UINTN) MaximumVariableSize - 1) / (UINTN) MaximumVariableSize);
  IpSecVariableInfo.VariableSize        = (UINT32) DataSize;
  IpSecVariableInfo.SingleVariableSize  = (UINT32) MaximumVariableSize;

  //
  // Set the variable of ipsecconfig general information.
  //
  Status = gRT->SetVariable (
                  VariableNameI,
                  VendorGuid,
                  Attributes,
                  sizeof (IpSecVariableInfo),
                  &IpSecVariableInfo
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error set ipsecconfig meta data with %r\n", Status));
    goto ON_EXIT;
  }

  for (VariableIndex = 0; VariableIndex < IpSecVariableInfo.VariableCount; VariableIndex++) {
    //
    // Construct and set the variable of ipsecconfig data one by one.
    // The index of variable name begin from 0001, and the varaible name
    // likes "VariableName0001", "VaraiableName0002"....
    // 
    UnicodeSPrint (VariableNameI, VariableNameSize, L"%s%04d", VariableName, VariableIndex + 1);
    Status = gRT->SetVariable (
                    VariableNameI,
                    VendorGuid,
                    Attributes,
                    (VariableIndex == IpSecVariableInfo.VariableCount - 1) ?
                    (DataSize % (UINTN) MaximumVariableSize) :
                    (UINTN) MaximumVariableSize,
                    (UINT8 *) Data + VariableIndex * (UINTN) MaximumVariableSize
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Error set ipsecconfig variable data with %r\n", Status));
      goto ON_EXIT;
    }
  }

ON_EXIT:
  if (VariableNameI != NULL) {
    FreePool (VariableNameI);
  }

  return Status;
}

/**
  Return the configuration value for the EFI IPsec driver. 

  This function lookup the data entry from IPsec database or IKEv2 configuration
  information. The expected data type and unique identification are described in
  DataType and Selector parameters.        

  @param[in]      This          Pointer to the EFI_IPSEC_CONFIG_PROTOCOL instance.
  @param[in]      DataType      The type of data to retrieve.
  @param[in]      Selector      Pointer to an entry selector that is an identifier of the IPsec 
                                configuration data entry.
  @param[in, out] DataSize      On output the size of data returned in Data.
  @param[out]     Data          The buffer to return the contents of the IPsec configuration data. 
                                The type of the data buffer associated with the DataType. 
 
  @retval EFI_SUCCESS           The specified configuration data was obtained successfully.
  @retval EFI_INVALID_PARAMETER One or more of the followings are TRUE:
                                - This is NULL.
                                - Selector is NULL.
                                - DataSize is NULL.
                                - Data is NULL and *DataSize is not zero
  @retval EFI_NOT_FOUND         The configuration data specified by Selector is not found.
  @retval EFI_UNSUPPORTED       The specified DataType is not supported.
  @retval EFI_BUFFER_TOO_SMALL  The DataSize is too small for the result. DataSize has been
                                updated with the size needed to complete the request.

**/
EFI_STATUS
EFIAPI
EfiIpSecConfigGetData (
  IN     EFI_IPSEC_CONFIG_PROTOCOL    *This,
  IN     EFI_IPSEC_CONFIG_DATA_TYPE   DataType,
  IN     EFI_IPSEC_CONFIG_SELECTOR    *Selector,
  IN OUT UINTN                        *DataSize,
     OUT VOID                         *Data
  )
{
  if (This == NULL || Selector == NULL || DataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (*DataSize != 0 && Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (DataType >= IPsecConfigDataTypeMaximum) {
    return EFI_UNSUPPORTED;
  }

  return mGetPolicyEntry[DataType](Selector, DataSize, Data);
}

/**
  Set the security association, security policy and peer authorization configuration
  information for the EFI IPsec driver. 

  This function is used to set the IPsec configuration information of type DataType for
  the EFI IPsec driver.
  The IPsec configuration data has a unique selector/identifier separately to identify
  a data entry. The selector structure depends on DataType's definition.
  Using SetData() with a Data of NULL causes the IPsec configuration data entry identified
  by DataType and Selector to be deleted.        

  @param[in] This               Pointer to the EFI_IPSEC_CONFIG_PROTOCOL instance.
  @param[in] DataType           The type of data to be set.
  @param[in] Selector           Pointer to an entry selector on operated configuration data 
                                specified by DataType. A NULL Selector causes the entire 
                                specified-type configuration information to be flushed.
  @param[in] Data               The data buffer to be set. The structure of the data buffer is 
                                associated with the DataType.
  @param[in] InsertBefore       Pointer to one entry selector which describes the expected
                                position the new data entry will be added. If InsertBefore is NULL,
                                the new entry will be appended to the end of the database.
 
  @retval EFI_SUCCESS           The specified configuration entry data was set successfully.
  @retval EFI_INVALID_PARAMETER One or more of the following are TRUE:
                                - This is NULL.
  @retval EFI_UNSUPPORTED       The specified DataType is not supported.
  @retval EFI_OUT_OF_RESOURCED  The required system resource could not be allocated.

**/
EFI_STATUS
EFIAPI
EfiIpSecConfigSetData (
  IN EFI_IPSEC_CONFIG_PROTOCOL        *This,
  IN EFI_IPSEC_CONFIG_DATA_TYPE       DataType,
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector,
  IN VOID                             *Data,
  IN EFI_IPSEC_CONFIG_SELECTOR        *InsertBefore OPTIONAL
  )
{
  EFI_STATUS  Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (DataType >= IPsecConfigDataTypeMaximum) {
    return EFI_UNSUPPORTED;
  }
  
  Status = mSetPolicyEntry[DataType](Selector, Data, InsertBefore);

  if (!EFI_ERROR (Status) && !mSetBySelf) {
    //
    // Save the updated config data into variable.
    //
    IpSecConfigSave ();
  }

  return Status;
}

/**
  Enumerates the current selector for IPsec configuration data entry. 

  This function is called multiple times to retrieve the entry Selector in IPsec
  configuration database. On each call to GetNextSelector(), the next entry 
  Selector are retrieved into the output interface.
 
  If the entire IPsec configuration database has been iterated, the error 
  EFI_NOT_FOUND is returned.
  If the Selector buffer is too small for the next Selector copy, an 
  EFI_BUFFER_TOO_SMALL error is returned, and SelectorSize is updated to reflect 
  the size of buffer needed.

  On the initial call to GetNextSelector() to start the IPsec configuration database
  search, a pointer to the buffer with all zero value is passed in Selector. Calls 
  to SetData() between calls to GetNextSelector may produce unpredictable results.         

  @param[in]      This          Pointer to the EFI_IPSEC_CONFIG_PROTOCOL instance.
  @param[in]      DataType      The type of IPsec configuration data to retrieve.
  @param[in, out] SelectorSize  The size of the Selector buffer.
  @param[in, out] Selector      On input, supplies the pointer to last Selector that was 
                                returned by GetNextSelector().
                                On output, returns one copy of the current entry Selector
                                of a given DataType. 
 
  @retval EFI_SUCCESS           The specified configuration data was obtained successfully.
  @retval EFI_INVALID_PARAMETER One or more of the followings are TRUE:
                                - This is NULL.
                                - SelectorSize is NULL.
                                - Selector is NULL.
  @retval EFI_NOT_FOUND         The next configuration data entry was not found.
  @retval EFI_UNSUPPORTED       The specified DataType is not supported.
  @retval EFI_BUFFER_TOO_SMALL  The SelectorSize is too small for the result. This parameter
                                has been updated with the size needed to complete the search 
                                request.

**/
EFI_STATUS
EFIAPI
EfiIpSecConfigGetNextSelector (
  IN     EFI_IPSEC_CONFIG_PROTOCOL    *This,
  IN     EFI_IPSEC_CONFIG_DATA_TYPE   DataType,
  IN OUT UINTN                        *SelectorSize,
  IN OUT EFI_IPSEC_CONFIG_SELECTOR    *Selector
  )
{
  LIST_ENTRY                *Link;
  IPSEC_COMMON_POLICY_ENTRY *CommonEntry;
  BOOLEAN                   IsFound;

  if (This == NULL || Selector == NULL || SelectorSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (DataType >= IPsecConfigDataTypeMaximum) {
    return EFI_UNSUPPORTED;
  }

  IsFound = FALSE;

  NET_LIST_FOR_EACH (Link, &mConfigData[DataType]) {
    CommonEntry = BASE_CR (Link, IPSEC_COMMON_POLICY_ENTRY, List);

    if (IsFound || (BOOLEAN)(mIsZeroSelector[DataType](Selector))) {
      //
      // If found the appointed entry, then duplicate the next one and return,
      // or if the appointed entry is zero, then return the first one directly.
      //
      return mDuplicateSelector[DataType](Selector, CommonEntry->Selector, SelectorSize);
    } else {
      //
      // Set the flag if find the appointed entry.
      //
      IsFound = mCompareSelector[DataType](Selector, CommonEntry->Selector);
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Register an event that is to be signaled whenever a configuration process on the
  specified IPsec configuration information is done. 

  The register function is not surpport now and always returns EFI_UNSUPPORTED.
  
  @param[in] This               Pointer to the EFI_IPSEC_CONFIG_PROTOCOL instance.
  @param[in] DataType           The type of data to be registered the event for.
  @param[in] Event              The event to be registered.
 
  @retval EFI_SUCCESS           The event is registered successfully.
  @retval EFI_INVALID_PARAMETER This is NULL or Event is NULL.
  @retval EFI_ACCESS_DENIED     The Event is already registered for the DataType.
  @retval EFI_UNSUPPORTED       The notify registration is unsupported, or the specified
                                DataType is not supported.

**/
EFI_STATUS
EFIAPI
EfiIpSecConfigRegisterNotify (
  IN EFI_IPSEC_CONFIG_PROTOCOL        *This,
  IN EFI_IPSEC_CONFIG_DATA_TYPE       DataType,
  IN EFI_EVENT                        Event
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Remove the specified event that was previously registered on the specified IPsec
  configuration data. 

  This function is not support now and alwasy return EFI_UNSUPPORTED.

  @param[in] This               Pointer to the EFI_IPSEC_CONFIG_PROTOCOL instance.
  @param[in] DataType           The configuration data type to remove the registered event for.
  @param[in] Event              The event to be unregistered.
 
  @retval EFI_SUCCESS           The event was removed successfully.
  @retval EFI_NOT_FOUND         The Event specified by DataType could not be found in the 
                                database.
  @retval EFI_INVALID_PARAMETER This is NULL or Event is NULL.
  @retval EFI_UNSUPPORTED       The notify registration is unsupported, or the specified
                                DataType is not supported.

**/
EFI_STATUS
EFIAPI
EfiIpSecConfigUnregisterNotify (
  IN EFI_IPSEC_CONFIG_PROTOCOL        *This,
  IN EFI_IPSEC_CONFIG_DATA_TYPE       DataType,
  IN EFI_EVENT                        Event
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Copy whole data in specified EFI_SIPEC_CONFIG_SELECTOR and the Data to a buffer.

  This function is a caller defined function, and it is called by the IpSecVisitConfigData().
  The orignal caller is IpSecConfigSave(), which calls the IpsecVisitConfigData() to 
  copy all types of IPsec Config datas into one buffer and store this buffer into firmware in
  the form of several variables.
  
  @param[in]      Type              A specified IPSEC_CONFIG_DATA_TYPE.
  @param[in]      Selector          Points to a EFI_IPSEC_CONFIG_SELECTOR to be copied
                                    to the buffer.
  @param[in]      Data              Points to data to be copied to the buffer. The
                                    Data type is related to the Type.
  @param[in]      SelectorSize      The size of the Selector.
  @param[in]      DataSize          The size of the Data.
  @param[in, out] Buffer            The buffer to store the Selector and Data.

  @retval EFI_SUCCESS            Copy the Selector and Data to a buffer successfully.
  @retval EFI_OUT_OF_RESOURCES   The required system resource could not be allocated.

**/
EFI_STATUS
IpSecCopyPolicyEntry (
  IN     EFI_IPSEC_CONFIG_DATA_TYPE   Type,
  IN     EFI_IPSEC_CONFIG_SELECTOR    *Selector,
  IN     VOID                         *Data,
  IN     UINTN                        SelectorSize,
  IN     UINTN                        DataSize,
  IN OUT IPSEC_VARIABLE_BUFFER        *Buffer
  )
{
  IPSEC_VAR_ITEM_HEADER SelectorHeader;
  IPSEC_VAR_ITEM_HEADER DataHeader;
  UINTN                 EntrySize;
  UINT8                 *TempPoint;
  
  if (Type == IPsecConfigDataTypeSad) {
    //
    // Don't save automatically-generated SA entry into variable.
    //
    if (((EFI_IPSEC_SA_DATA2 *) Data)->ManualSet == FALSE) {
      return EFI_SUCCESS;
    }
  }
  //
  // Increase the capacity size of the buffer if needed.
  //
  EntrySize  = ALIGN_VARIABLE (sizeof (SelectorHeader));
  EntrySize  = ALIGN_VARIABLE (EntrySize + SelectorSize);
  EntrySize  = ALIGN_VARIABLE (EntrySize + sizeof (SelectorHeader));
  EntrySize  = ALIGN_VARIABLE (EntrySize + DataSize);
  
  //EntrySize = SelectorSize + DataSize + 2 * sizeof (SelectorHeader);
  if (Buffer->Capacity - Buffer->Size < EntrySize) {
    //
    // Calculate the required buffer
    //
    Buffer->Capacity += EntrySize;
    TempPoint         = AllocatePool (Buffer->Capacity);
    
    if (TempPoint == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Copy the old Buffer to new buffer and free the old one.
    //
    CopyMem (TempPoint, Buffer->Ptr, Buffer->Size);
    FreePool (Buffer->Ptr);
    
    Buffer->Ptr       =  TempPoint;    
  }

  mFixPolicyEntry[Type](Selector, Data);

  //
  // Fill the selector header and copy it into buffer.
  //
  SelectorHeader.Type = (UINT8) (Type | IPSEC_VAR_ITEM_HEADER_LOGO_BIT);
  SelectorHeader.Size = (UINT16) SelectorSize;

  CopyMem (
    Buffer->Ptr + Buffer->Size,
    &SelectorHeader,
    sizeof (SelectorHeader)
    );
  Buffer->Size  = ALIGN_VARIABLE (Buffer->Size + sizeof (SelectorHeader));
  
  //
  // Copy the selector into buffer.
  //
  CopyMem (
    Buffer->Ptr + Buffer->Size,
    Selector,
    SelectorSize
    );
  Buffer->Size  = ALIGN_VARIABLE (Buffer->Size + SelectorSize);

  //
  // Fill the data header and copy it into buffer.
  //
  DataHeader.Type = (UINT8) Type;
  DataHeader.Size = (UINT16) DataSize;

  CopyMem (
    Buffer->Ptr + Buffer->Size,
    &DataHeader,
    sizeof (DataHeader)
    );
  Buffer->Size  = ALIGN_VARIABLE (Buffer->Size + sizeof (DataHeader));
  //
  // Copy the data into buffer.
  //
  CopyMem (
    Buffer->Ptr + Buffer->Size,
    Data,
    DataSize
    );
  Buffer->Size  = ALIGN_VARIABLE (Buffer->Size + DataSize);
  
  mUnfixPolicyEntry[Type](Selector, Data);

  return EFI_SUCCESS;
}

/**
  Visit all IPsec Configurations of specified Type and call the caller defined
  interface.

  @param[in]  DataType          The specified IPsec Config Data Type.
  @param[in]  Routine           The function defined by the caller.
  @param[in]  Context           The data passed to the Routine.

  @retval EFI_OUT_OF_RESOURCES   The required system resource could not be allocated
  @retval EFI_SUCCESS            This function completed successfully.

**/
EFI_STATUS
IpSecVisitConfigData (
  IN EFI_IPSEC_CONFIG_DATA_TYPE DataType,
  IN IPSEC_COPY_POLICY_ENTRY    Routine,
  IN VOID                       *Context
  )
{
  EFI_STATUS                GetNextStatus;
  EFI_STATUS                GetDataStatus;
  EFI_STATUS                RoutineStatus;
  EFI_IPSEC_CONFIG_SELECTOR *Selector;
  VOID                      *Data;
  UINTN                     SelectorSize;
  UINTN                     DataSize;
  UINTN                     SelectorBufferSize;
  UINTN                     DataBufferSize;
  BOOLEAN                   FirstGetNext;

  FirstGetNext        = TRUE;
  DataBufferSize      = 0;
  Data                = NULL;
  SelectorBufferSize  = sizeof (EFI_IPSEC_CONFIG_SELECTOR);
  Selector            = AllocateZeroPool (SelectorBufferSize);

  if (Selector == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  while (TRUE) {
    //
    // Get the real size of the selector.
    //
    SelectorSize = SelectorBufferSize;
    GetNextStatus = EfiIpSecConfigGetNextSelector (
                      &mIpSecConfigInstance,
                      DataType,
                      &SelectorSize,
                      Selector
                      );
    if (GetNextStatus == EFI_BUFFER_TOO_SMALL) {
      FreePool (Selector);
      SelectorBufferSize = SelectorSize;
      //
      // Allocate zero pool for the first selector, while store the last
      // selector content for the other selectors.
      //
      if (FirstGetNext) {
        Selector = AllocateZeroPool (SelectorBufferSize);
      } else {
        Selector = AllocateCopyPool (SelectorBufferSize, Selector);
      }

      if (Selector == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      //
      // Get the content of the selector.
      //
      GetNextStatus = EfiIpSecConfigGetNextSelector (
                        &mIpSecConfigInstance,
                        DataType,
                        &SelectorSize,
                        Selector
                        );
    }

    if (EFI_ERROR (GetNextStatus)) {
      break;
    }

    FirstGetNext = FALSE;

    //
    // Get the real size of the policy entry according to the selector.
    //
    DataSize = DataBufferSize;
    GetDataStatus = EfiIpSecConfigGetData (
                      &mIpSecConfigInstance,
                      DataType,
                      Selector,
                      &DataSize,
                      Data
                      );
    if (GetDataStatus == EFI_BUFFER_TOO_SMALL) {
      if (Data != NULL) {
        FreePool (Data);
      }

      DataBufferSize  = DataSize;
      Data            = AllocateZeroPool (DataBufferSize);

      if (Data == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      //
      // Get the content of the policy entry according to the selector.
      //
      GetDataStatus = EfiIpSecConfigGetData (
                        &mIpSecConfigInstance,
                        DataType,
                        Selector,
                        &DataSize,
                        Data
                        );
    }

    if (EFI_ERROR (GetDataStatus)) {
      break;
    }
    //
    // Prepare the buffer of updated policy entry, which is stored in
    // the continous memory, and then save into variable later.
    //
    RoutineStatus = Routine (
                      DataType,
                      Selector,
                      Data,
                      SelectorSize,
                      DataSize,
                      Context
                      );
    if (EFI_ERROR (RoutineStatus)) {
      break;
    }
  }

  if (Data != NULL) {
    FreePool (Data);
  }

  if (Selector != NULL) {
    FreePool (Selector);
  }

  return EFI_SUCCESS;
}

/**
  This function is the subfunction of  EFIIpSecConfigSetData.

  This function call IpSecSetVaraible to set the IPsec Configuration into the firmware.

  @retval EFI_OUT_OF_RESOURCES   The required system resource could not be allocated.
  @retval EFI_SUCCESS            Saved the configration successfully.
  @retval Others                 Other errors were found while obtaining the variable.

**/
EFI_STATUS
IpSecConfigSave (
  VOID
  )
{
  IPSEC_VARIABLE_BUFFER       Buffer;
  EFI_STATUS                  Status;
  EFI_IPSEC_CONFIG_DATA_TYPE  Type;

  Buffer.Size     = 0;
  Buffer.Capacity = IPSEC_DEFAULT_VARIABLE_SIZE;
  Buffer.Ptr      = AllocateZeroPool (Buffer.Capacity);

  if (Buffer.Ptr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // For each policy database, prepare the contious buffer to save into variable.
  //
  for (Type = IPsecConfigDataTypeSpd; Type < IPsecConfigDataTypeMaximum; Type++) {
    IpSecVisitConfigData (
      Type,
      (IPSEC_COPY_POLICY_ENTRY) IpSecCopyPolicyEntry,
      &Buffer
      );
  }
  //
  // Save the updated policy database into variable.
  //
  Status = IpSecSetVariable (
             IPSECCONFIG_VARIABLE_NAME,
             &gEfiIpSecConfigProtocolGuid,
             EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
             Buffer.Size,
             Buffer.Ptr
             );

  FreePool (Buffer.Ptr);

  return Status;
}

/**
  Get the all IPSec configuration variables and store those variables
  to the internal data structure.

  This founction is called by IpSecConfigInitialize() which is to intialize the 
  IPsecConfiguration Protocol.

  @param[in]  Private            Point to IPSEC_PRIVATE_DATA.

  @retval EFI_OUT_OF_RESOURCES   The required system resource could not be allocated
  @retval EFI_SUCCESS            Restore the IPsec Configuration successfully.
  @retval  others                Other errors is found while obtaining the variable.

**/
EFI_STATUS
IpSecConfigRestore (
  IN IPSEC_PRIVATE_DATA           *Private
  )
{
  EFI_STATUS                  Status;
  UINTN                       BufferSize;
  UINT8                       *Buffer;
  IPSEC_VAR_ITEM_HEADER       *Header;
  UINT8                       *Ptr;
  EFI_IPSEC_CONFIG_SELECTOR   *Selector;
  EFI_IPSEC_CONFIG_DATA_TYPE  Type;
  VOID                        *Data;
  UINT8                       Value;
  UINTN                       Size;

  Value       = 0;
  Size        = sizeof (Value);
  BufferSize  = 0;
  Buffer      = NULL;

  Status = gRT->GetVariable (
                  IPSECCONFIG_STATUS_NAME,
                  &gEfiIpSecConfigProtocolGuid,
                  NULL,
                  &Size,
                  &Value
             );

  if (!EFI_ERROR (Status) && Value == IPSEC_STATUS_ENABLED) {
    Private->IpSec.DisabledFlag = FALSE;
  }
  //
  // Get the real size of policy database in variable.
  //
  Status = IpSecGetVariable (
             IPSECCONFIG_VARIABLE_NAME,
             &gEfiIpSecConfigProtocolGuid,
             NULL,
             &BufferSize,
             Buffer
             );
  if (Status == EFI_BUFFER_TOO_SMALL) {

    Buffer = AllocateZeroPool (BufferSize);
    if (Buffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Get the content of policy database in variable.
    //
    Status = IpSecGetVariable (
               IPSECCONFIG_VARIABLE_NAME,
               &gEfiIpSecConfigProtocolGuid,
               NULL,
               &BufferSize,
               Buffer
               );
    if (EFI_ERROR (Status)) {
      FreePool (Buffer);
      return Status;
    }

    for (Ptr = Buffer; Ptr < Buffer + BufferSize;) {

      Header  = (IPSEC_VAR_ITEM_HEADER *) Ptr;
      Type    = (EFI_IPSEC_CONFIG_DATA_TYPE) (Header->Type & IPSEC_VAR_ITEM_HEADER_CONTENT_BIT);
      ASSERT (((Header->Type & 0x80) == IPSEC_VAR_ITEM_HEADER_LOGO_BIT) && (Type < IPsecConfigDataTypeMaximum));
      
      Selector  = (EFI_IPSEC_CONFIG_SELECTOR *) ALIGN_POINTER (Header + 1, sizeof (UINTN));
      Header    = (IPSEC_VAR_ITEM_HEADER *) ALIGN_POINTER (
                                              (UINT8 *) Selector + Header->Size, 
                                              sizeof (UINTN)
                                              );
      ASSERT (Header->Type == Type);

      Data = ALIGN_POINTER (Header + 1, sizeof (UINTN));

      mUnfixPolicyEntry[Type](Selector, Data);

      //
      // Update each policy entry according to the content in variable.
      //
      mSetBySelf = TRUE;
      Status = EfiIpSecConfigSetData (
                 &Private->IpSecConfig,
                 Type,
                 Selector,
                 Data,
                 NULL
                 );
      mSetBySelf = FALSE;

      if (EFI_ERROR (Status)) {
        FreePool (Buffer);
        return Status;
      }

      Ptr =  ALIGN_POINTER ((UINT8 *) Data + Header->Size, sizeof (UINTN));
    }

    FreePool (Buffer);
  }

  return EFI_SUCCESS;
}

/**
  Install and Initialize IPsecConfig protocol

  @param[in, out]  Private   Pointer to IPSEC_PRIVATE_DATA. After this function finish,
                             the pointer of IPsecConfig Protocol implementation will copy
                             into its IPsecConfig member.

  @retval     EFI_SUCCESS    Initialized the IPsecConfig Protocol successfully.
  @retval     Others         Initializing the IPsecConfig Protocol failed.
**/
EFI_STATUS
IpSecConfigInitialize (
  IN OUT IPSEC_PRIVATE_DATA        *Private
  )
{
  EFI_IPSEC_CONFIG_DATA_TYPE  Type;

  CopyMem (
    &Private->IpSecConfig,
    &mIpSecConfigInstance,
    sizeof (EFI_IPSEC_CONFIG_PROTOCOL)
    );

  //
  // Initialize the list head of policy database.
  //
  for (Type = IPsecConfigDataTypeSpd; Type < IPsecConfigDataTypeMaximum; Type++) {
    InitializeListHead (&mConfigData[Type]);
  }
  //
  // Restore the content of policy database according to the variable.
  //
  IpSecConfigRestore (Private);

  return gBS->InstallMultipleProtocolInterfaces (
                &Private->Handle,
                &gEfiIpSecConfigProtocolGuid,
                &Private->IpSecConfig,
                NULL
                );
}
