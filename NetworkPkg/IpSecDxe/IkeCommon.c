/** @file
  Common operation of the IKE
  
  Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>

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

//
// Initial the SPI
//
UINT32            mNextSpi  = IKE_SPI_BASE;

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

  @return a SPI in 4 bytes.

**/
UINT32
IkeGenerateSpi (
  VOID
  )
{
  //
  // TODO: should generate SPI randomly to avoid security issue
  //
  return mNextSpi++;
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

