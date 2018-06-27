/** @file
  Common operation of the IKE

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ike.h"
#include "IkeCommon.h"
#include "IpSecConfigImpl.h"
#include "IpSecDebug.h"

/**
  Check whether the new generated Spi has existed.

  @param[in]   IkeSaSession   Pointer to the Child SA Session.
  @param[in]   SpiValue       SPI Value.

  @retval  TRUE    This SpiValue has existed in the Child SA Session
  @retval  FALSE   This SpiValue doesn't exist in the Child SA Session.

**/
BOOLEAN
IkeSpiValueExisted (
  IN IKEV2_SA_SESSION      *IkeSaSession,
  IN UINT32                SpiValue
  )
{
  LIST_ENTRY              *Entry;
  LIST_ENTRY              *Next;
  IKEV2_CHILD_SA_SESSION  *SaSession;

  Entry     = NULL;
  Next      = NULL;
  SaSession = NULL;

  //
  // Check whether the SPI value has existed in ChildSaEstablishSessionList.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, Next, &IkeSaSession->ChildSaEstablishSessionList) {
    SaSession= IKEV2_CHILD_SA_SESSION_BY_IKE_SA (Entry);
    if (SaSession->LocalPeerSpi == SpiValue) {
      return TRUE;
    }
  }

  //
  // Check whether the SPI value has existed in ChildSaSessionList.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, Next, &IkeSaSession->ChildSaSessionList) {
    SaSession= IKEV2_CHILD_SA_SESSION_BY_IKE_SA (Entry);
    if (SaSession->LocalPeerSpi == SpiValue) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Call Crypto Lib to generate a random value with eight-octet length.

  @return the 64 byte vaule.

**/
UINT64
IkeGenerateCookie (
  VOID
  )
{
  UINT64     Cookie;
  EFI_STATUS Status;

  Status = IpSecCryptoIoGenerateRandomBytes ((UINT8 *)&Cookie, sizeof (UINT64));
  if (EFI_ERROR (Status)) {
    return 0;
  } else {
    return Cookie;
  }
}

/**
  Generate the random data for Nonce payload.

  @param[in]  NonceSize      Size of the data in bytes.

  @return Buffer which contains the random data of the spcified size.

**/
UINT8 *
IkeGenerateNonce (
  IN UINTN              NonceSize
  )
{
  UINT8                  *Nonce;
  EFI_STATUS             Status;

  Nonce = AllocateZeroPool (NonceSize);
  if (Nonce == NULL) {
    return NULL;
  }

  Status = IpSecCryptoIoGenerateRandomBytes (Nonce, NonceSize);
  if (EFI_ERROR (Status)) {
    FreePool (Nonce);
    return NULL;
  } else {
    return Nonce;
  }
}

/**
  Convert the IKE Header from Network order to Host order.

  @param[in, out]  Header    The pointer of the IKE_HEADER.

**/
VOID
IkeHdrNetToHost (
  IN OUT IKE_HEADER *Header
  )
{
  Header->InitiatorCookie = NTOHLL (Header->InitiatorCookie);
  Header->ResponderCookie = NTOHLL (Header->ResponderCookie);
  Header->MessageId       = NTOHL (Header->MessageId);
  Header->Length          = NTOHL (Header->Length);
}

/**
  Convert the IKE Header from Host order to Network order.

  @param[in, out] Header     The pointer of the IKE_HEADER.

**/
VOID
IkeHdrHostToNet (
  IN OUT IKE_HEADER *Header
  )
{
  Header->InitiatorCookie = HTONLL (Header->InitiatorCookie);
  Header->ResponderCookie = HTONLL (Header->ResponderCookie);
  Header->MessageId       = HTONL (Header->MessageId);
  Header->Length          = HTONL (Header->Length);
}

/**
  Allocate a buffer of IKE_PAYLOAD and set its Signature.

  @return A buffer of IKE_PAYLOAD.

**/
IKE_PAYLOAD *
IkePayloadAlloc (
  VOID
  )
{
  IKE_PAYLOAD *IkePayload;

  IkePayload            = (IKE_PAYLOAD *) AllocateZeroPool (sizeof (IKE_PAYLOAD));
  if (IkePayload == NULL) {
    return NULL;
  }

  IkePayload->Signature = IKE_PAYLOAD_SIGNATURE;

  return IkePayload;
}

/**
  Free a specified IKE_PAYLOAD buffer.

  @param[in]  IkePayload   Pointer of IKE_PAYLOAD to be freed.

**/
VOID
IkePayloadFree (
  IN IKE_PAYLOAD *IkePayload
  )
{
  if (IkePayload == NULL) {
    return;
  }
  //
  // If this IkePayload is not referred by others, free it.
  //
  if (!IkePayload->IsPayloadBufExt && (IkePayload->PayloadBuf != NULL)) {
    FreePool (IkePayload->PayloadBuf);
  }

  FreePool (IkePayload);
}

/**
  Generate an new SPI.

  @param[in]       IkeSaSession   Pointer to IKEV2_SA_SESSION related to this Child SA
                                  Session.
  @param[in, out]  SpiValue       Pointer to the new generated SPI value.

  @retval EFI_SUCCESS         The operation performs successfully.
  @retval Otherwise           The operation is failed.

**/
EFI_STATUS
IkeGenerateSpi (
  IN     IKEV2_SA_SESSION         *IkeSaSession,
  IN OUT UINT32                   *SpiValue
  )
{
  EFI_STATUS   Status;

  Status = EFI_SUCCESS;

  while (TRUE) {
    //
    // Generate SPI randomly
    //
    Status = IpSecCryptoIoGenerateRandomBytes ((UINT8 *)SpiValue, sizeof (UINT32));
    if (EFI_ERROR (Status)) {
      break;
    }

    //
    // The set of SPI values in the range 1 through 255 are reserved by the
    // Internet Assigned Numbers Authority (IANA) for future use; a reserved
    // SPI value will not normally be assigned by IANA unless the use of the
    // assigned SPI value is specified in an RFC.
    //
    if (*SpiValue < IKE_SPI_BASE) {
      *SpiValue += IKE_SPI_BASE;
    }

    //
    // Check whether the new generated SPI has existed.
    //
    if (!IkeSpiValueExisted (IkeSaSession, *SpiValue)) {
      break;
    }
  }

  return Status;
}

/**
  Generate a random data for IV

  @param[in]  IvBuffer  The pointer of the IV buffer.
  @param[in]  IvSize    The IV size.

  @retval     EFI_SUCCESS  Create a random data for IV.
  @retval     otherwise    Failed.

**/
EFI_STATUS
IkeGenerateIv (
  IN UINT8                           *IvBuffer,
  IN UINTN                           IvSize
  )
{
  return IpSecCryptoIoGenerateRandomBytes (IvBuffer, IvSize);
}


/**
  Find SPD entry by a specified SPD selector.

  @param[in] SpdSel       Point to SPD Selector to be searched for.

  @retval Point to SPD Entry if the SPD entry found.
  @retval NULL if not found.

**/
IPSEC_SPD_ENTRY *
IkeSearchSpdEntry (
  IN EFI_IPSEC_SPD_SELECTOR             *SpdSel
  )
{
  IPSEC_SPD_ENTRY *SpdEntry;
  LIST_ENTRY      *SpdList;
  LIST_ENTRY      *Entry;

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
      return SpdEntry;
    }

  }

  return NULL;
}

/**
  Get the IKE Version from the IKE_SA_SESSION.

  @param[in]  Session  Pointer of the IKE_SA_SESSION.

**/
UINT8
IkeGetVersionFromSession (
  IN UINT8    *Session
  )
{
  if (*(UINT32 *) Session == IKEV2_SA_SESSION_SIGNATURE) {
    return ((IKEV2_SA_SESSION *) Session)->SessionCommon.IkeVer;
  } else {
    //
    // Add IKEv1 support here.
    //
    return 0;
  }
}

