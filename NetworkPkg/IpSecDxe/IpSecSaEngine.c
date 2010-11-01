/** @file
  IPsec inbound and outbound traffic processing.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IpSecImpl.h"
#include "IpSecDebug.h"
#include "IpSecCryptIo.h"

extern LIST_ENTRY     mConfigData[IPsecConfigDataTypeMaximum];

/**
  The call back function of NetbufFromExt.

  @param[in]  Arg            The argument passed from the caller.

**/
VOID
EFIAPI
IpSecOnRecyclePacket (
  IN VOID                            *Arg
  )
{
}

/**
  This is a Notification function. It is called when the related IP6_TXTOKEN_WRAP
  is released.

  @param[in]  Event              The related event.
  @param[in]  Context            The data passed by the caller.

**/
VOID
EFIAPI
IpSecRecycleCallback (
  IN EFI_EVENT                       Event,
  IN VOID                            *Context
  )
{
  IPSEC_RECYCLE_CONTEXT *RecycleContext;

  RecycleContext = (IPSEC_RECYCLE_CONTEXT *) Context;

  if (RecycleContext->FragmentTable != NULL) {
    FreePool (RecycleContext->FragmentTable);
  }

  if (RecycleContext->PayloadBuffer != NULL) {
    FreePool (RecycleContext->PayloadBuffer);
  }

  FreePool (RecycleContext);
  gBS->CloseEvent (Event);

}

/**
  Calculate the extension header of IP. The return length only doesn't contain
  the fixed IP header length.

  @param[in]  IpHead             Points to an IP head to be calculated.
  @param[in]  LastHead           Points to the last header of the IP header.

  @return The length of the extension header.

**/
UINT16
IpSecGetPlainExtHeadSize (
  IN VOID                             *IpHead,
  IN UINT8                            *LastHead
  )
{
  UINT16  Size;

  Size = (UINT16) (LastHead - (UINT8 *) IpHead);

  if (Size > sizeof (EFI_IP6_HEADER)) {
    //
    // * (LastHead+1) point the last header's length but not include the first
    // 8 octers, so this formluation add 8 at the end.
    //
    Size = (UINT16) (Size - sizeof (EFI_IP6_HEADER) + *(LastHead + 1) + 8);
  } else {
    Size = 0;
  }

  return Size;
}

/**
  Authenticate the IpSec Payload and store the result in the IcvBuffer.

  @param[in]      BufferToAuth    The buffer to be Authenticated.
  @param[in]      AuthSize        The size of the buffer to be Authenticated.
  @param[in, out] IcvBuffer       The buffer to store the ICV.
  @param[in]      IcvSize         The size of ICV.
  @param[in]      Key             The Key passed to the CryptLib to generate a
                                  CRYPT_HANDLE.
  @param[in]      AuthAlgId       The Authentication Algorithm ID.

  @retval EFI_UNSUPPORTED     If the AuthAlg is not in the support list.
  @retval EFI_SUCCESS         Authenticated the payload successfully.
  @retval otherwise           Authentication of the payload failed.
**/
EFI_STATUS
IpSecAuthPayload (
  IN     UINT8                           *BufferToAuth,
  IN     UINTN                           AuthSize,
  IN OUT UINT8                           *IcvBuffer,
  IN     UINTN                           IcvSize,
  IN     VOID                            *Key,
  IN     UINT8                           AuthAlgId
  )
{
  switch (AuthAlgId) {
    case IKE_AALG_NONE :
    case IKE_AALG_NULL :
        return EFI_SUCCESS;

    default:
        return EFI_UNSUPPORTED;
  }
}

/**
  Verify if the Authentication payload is correct.

  @param[in]  EspBuffer          Points to the ESP wrapped buffer.
  @param[in]  EspSize            The size of the ESP wrapped buffer.
  @param[in]  SadEntry           The related SAD entry to store the authentication
                                 algorithm key.
  @param[in]  IcvSize            The length of ICV.

  @retval EFI_SUCCESS        The authentication data is correct.
  @retval EFI_ACCESS_DENIED  The authentication data is not correct.

**/
EFI_STATUS
IpSecEspAuthVerifyPayload (
  IN UINT8                           *EspBuffer,
  IN UINTN                           EspSize,
  IN IPSEC_SAD_ENTRY                 *SadEntry,
  IN UINTN                           *IcvSize
  )
{
  EFI_STATUS  Status;
  UINTN       AuthSize;
  UINT8       IcvBuffer[12];

  //
  // Calculate the size of authentication payload.
  //
  *IcvSize  = IpSecGetIcvLength (SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthAlgoId);
  AuthSize  = EspSize - *IcvSize;

  //
  // Calculate the icv buffer and size of the payload.
  //
  Status = IpSecAuthPayload (
             EspBuffer,
             AuthSize,
             IcvBuffer,
             *IcvSize,
             SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthKey,
             SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthAlgoId
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Compare the calculated icv and the appended original icv.
  //
  if (CompareMem (EspBuffer + AuthSize, IcvBuffer, *IcvSize) == 0) {
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_ERROR, "Error auth verify payload\n"));
  return EFI_ACCESS_DENIED;
}

/**
  ESP Decrypt the payload.

  @param[in, out] PayloadBuffer      Pointer to the buffer containing the ESP wrapped;
                                     to be decrypted on input, and plaintext on return. The
                                     number of bytes of data to be decrypted is
                                     specified by EncryptSize.
  @param[in]      EncryptSize        The size of the PayloadBuffer as input.
  @param[in]      SadEntry           The related SAD entry.
  @param[in]      IvSize             The size of IV.
  @param[out]     PlainPayloadSize   Contains the return value of decrypted size.
  @param[out]     PaddingSize        Contains the return value of Padding size.
  @param[out]     NextHeader         Contains the return value of the last protocol header
                                     of the IP packet.

  @retval EFI_UNSUPPORTED    The Algorithm pointed to by the SAD entry is not supported.
  @retval EFI_SUCCESS        The operation completed successfully.

**/
EFI_STATUS
IpSecEspDecryptPayload (
  IN OUT UINT8                       *PayloadBuffer,
  IN     UINTN                       EncryptSize,
  IN     IPSEC_SAD_ENTRY             *SadEntry,
  IN     UINTN                       *IvSize,
     OUT UINTN                       *PlainPayloadSize,
     OUT UINTN                       *PaddingSize,
     OUT UINT8                       *NextHeader
  )
{
  EFI_ESP_TAIL *EspTail;

  switch (SadEntry->Data->AlgoInfo.EspAlgoInfo.EncAlgoId) {
    case IKE_EALG_NULL:
      EspTail            = (EFI_ESP_TAIL *) (PayloadBuffer + EncryptSize - sizeof (EFI_ESP_TAIL));
      *PaddingSize       = EspTail->PaddingLength;
      *NextHeader        = EspTail->NextHeader;
      *PlainPayloadSize  = EncryptSize - EspTail->PaddingLength - sizeof (EFI_ESP_TAIL);
      break;

    case IKE_EALG_3DESCBC:
    case IKE_EALG_AESCBC:
      //
      // TODO: support these algorithm
      //
      return EFI_UNSUPPORTED;
    default :
      return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  ESP Encrypt the payload.

  @param[in, out] BufferToEncrypt Pointer to the buffer containing plaintext to be
                                  encrypted on input, and ciphertext on return. The
                                  number of bytes of data to be encrypted is
                                  specified by EncryptSize.
  @param[in, out] EncryptSize     The size of the plaintext on input, and the size of the
                                  ciphertext on return.
  @param[in]      IvBuffer        Points to IV data.
  @param[in]      IvSize          Size of IV.
  @param[in]      SadEntry        Related SAD entry.

  @retval EFI_UNSUPPORTED    The Algorithm pointed by SAD entry is not supported.
  @retval EFI_SUCCESS        The operation completed successfully.

**/
EFI_STATUS
IpSecEspEncryptPayload (
  IN OUT UINT8                       *BufferToEncrypt,
  IN OUT UINTN                       EncryptSize,
  IN     UINT8                       *IvBuffer,
  IN     UINTN                       IvSize,
  IN     IPSEC_SAD_ENTRY             *SadEntry
  )
{
  switch (SadEntry->Data->AlgoInfo.EspAlgoInfo.EncAlgoId) {
    case IKE_EALG_NULL:
      return EFI_SUCCESS;

    case IKE_EALG_3DESCBC:
    case IKE_EALG_AESCBC:
      //
      // TODO: support these algorithms
      //
      return EFI_UNSUPPORTED;
    default :
      return EFI_UNSUPPORTED;

  }
}

/**
  The actual entry to relative function processes the inbound traffic of ESP header.

  This function is the subfunction of IpSecProtectInboundPacket(). It checks the
  received packet security property and trim the ESP header and then returns without
  an IPsec protected IP Header and FramgmentTable.

  @param[in]      IpVersion          The version of IP.
  @param[in, out] IpHead             Points to the IP header containing the ESP header
                                     to be trimed on input, and without ESP header
                                     on return.
  @param[out]     LastHead           The Last Header in IP header on return.
  @param[in, out] OptionsBuffer      Pointer to the options buffer. It is optional.
  @param[in, out] OptionsLength      Length of the options buffer. It is optional.
  @param[in, out] FragmentTable      Pointer to a list of fragments in the form of IPsec
                                     protected on input, and without IPsec protected
                                     on return.
  @param[in, out] FragmentCount      The number of fragments.
  @param[out]     SpdEntry           Pointer to contain the address of SPD entry on return.
  @param[out]     RecycleEvent       The event for recycling of resources.

  @retval EFI_SUCCESS              The operation was successful.
  @retval EFI_ACCESS_DENIED        One or more following conditions is TRUE:
                                   - ESP header was not found.
                                   - The related SAD entry was not found.
                                   - The related SAD entry does not support the ESP protocol.
  @retval EFI_OUT_OF_RESOURCES     The required system resource can't be allocated.

**/
EFI_STATUS
IpSecEspInboundPacket (
  IN     UINT8                       IpVersion,
  IN OUT VOID                        *IpHead,
     OUT UINT8                       *LastHead,
  IN OUT VOID                        **OptionsBuffer, OPTIONAL
  IN OUT UINT32                      *OptionsLength,  OPTIONAL
  IN OUT EFI_IPSEC_FRAGMENT_DATA     **FragmentTable,
  IN OUT UINT32                      *FragmentCount,
     OUT IPSEC_SPD_ENTRY             **SpdEntry,
     OUT EFI_EVENT                   *RecycleEvent
  )
{
  EFI_STATUS            Status;
  NET_BUF               *Payload;
  UINTN                 EspSize;
  UINTN                 IvSize;
  UINTN                 PlainPayloadSize;
  UINTN                 PaddingSize;
  UINTN                 IcvSize;
  UINT8                 *ProcessBuffer;
  EFI_IP_ADDRESS        DestIp;
  EFI_ESP_HEADER        *EspHeader;
  EFI_ESP_TAIL          *EspTail;
  EFI_IPSEC_SA_ID       *SaId;
  IPSEC_SAD_DATA        *SadData;
  IPSEC_SAD_ENTRY       *SadEntry;
  IPSEC_RECYCLE_CONTEXT *RecycleContext;
  UINT32                Spi;
  UINT8                 NextHeader;
  UINT16                IpSecHeadSize;

  Status            = EFI_SUCCESS;
  Payload           = NULL;
  ProcessBuffer     = NULL;
  RecycleContext    = NULL;
  *RecycleEvent     = NULL;
  PlainPayloadSize  = 0;
  NextHeader        = 0;
  //
  // Build netbuf from fragment table first.
  //
  Payload = NetbufFromExt (
              (NET_FRAGMENT *) *FragmentTable,
              *FragmentCount,
              0,
              sizeof (EFI_ESP_HEADER),
              IpSecOnRecyclePacket,
              NULL
              );
  if (Payload == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }
  //
  // Get the esp size and eso header from netbuf.
  //
  EspSize   = Payload->TotalSize;
  EspHeader = (EFI_ESP_HEADER *) NetbufGetByte (Payload, 0, NULL);
  if (EspHeader == NULL) {
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }
  //
  // Parse destination address from ip header.
  //
  ZeroMem (&DestIp, sizeof (EFI_IP_ADDRESS));
  if (IpVersion == IP_VERSION_4) {
    CopyMem (
      &DestIp,
      &((IP4_HEAD *) IpHead)->Dst,
      sizeof (IP4_ADDR)
      );
  } else {
    CopyMem (
      &DestIp,
      &((EFI_IP6_HEADER *) IpHead)->DestinationAddress,
      sizeof (EFI_IPv6_ADDRESS)
      );
  }
  //
  // Lookup sad entry according to the spi and dest address.
  //
  Spi       = NTOHL (EspHeader->Spi);
  SadEntry  = IpSecLookupSadBySpi (Spi, &DestIp);
  if (SadEntry == NULL) {
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }

  SaId    = SadEntry->Id;
  SadData = SadEntry->Data;

  //
  // Only support esp protocol currently.
  //
  if (SaId->Proto != EfiIPsecESP) {
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }

  if (!SadData->ManualSet) {
    //
    // TODO: Check sa lifetime and sequence number
    //
  }
  //
  // Allocate buffer for decryption and authentication by esp.
  //
  ProcessBuffer = AllocateZeroPool (EspSize);
  if (ProcessBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  NetbufCopy (Payload, 0, (UINT32) EspSize, ProcessBuffer);

  //
  // Authenticate the esp wrapped buffer by the sad entry if has auth key.
  //
  IcvSize = 0;
  if (SadData->AlgoInfo.EspAlgoInfo.AuthKey != NULL) {
    Status = IpSecEspAuthVerifyPayload (
               ProcessBuffer,
               EspSize,
               SadEntry,
               &IcvSize
               );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }
  //
  // Decrypt the payload by the sad entry if has decrypt key.
  //
  IvSize = 0;
  if (SadData->AlgoInfo.EspAlgoInfo.EncKey != NULL) {
    Status = IpSecEspDecryptPayload (
               ProcessBuffer + sizeof (EFI_ESP_HEADER),
               EspSize - sizeof (EFI_ESP_HEADER) - IcvSize,
               SadEntry,
               &IvSize,
               &PlainPayloadSize,
               &PaddingSize,
               &NextHeader
               );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  } else {
    EspTail           = (EFI_ESP_TAIL *) (ProcessBuffer + EspSize - IcvSize - sizeof (EFI_ESP_TAIL));
    PaddingSize       = EspTail->PaddingLength;
    NextHeader        = EspTail->NextHeader;
    PlainPayloadSize  = EspSize - sizeof (EFI_ESP_HEADER) - IvSize - IcvSize - sizeof (EFI_ESP_TAIL) - PaddingSize;
  }
  //
  // TODO: handle anti-replay window
  //
  //
  // Decryption and authentication with esp has been done, so it's time to
  // reload the new packet, create recycle event and fixup ip header.
  //
  RecycleContext = AllocateZeroPool (sizeof (IPSEC_RECYCLE_CONTEXT));
  if (RecycleContext == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  IpSecRecycleCallback,
                  RecycleContext,
                  RecycleEvent
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }
  //
  // TODO: Who take responsible to handle the original fragment table?
  //
  *FragmentTable = AllocateZeroPool (sizeof (EFI_IPSEC_FRAGMENT_DATA));
  if (*FragmentTable == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  RecycleContext->PayloadBuffer       = ProcessBuffer;
  RecycleContext->FragmentTable       = *FragmentTable;
  (*FragmentTable)[0].FragmentBuffer  = ProcessBuffer + sizeof (EFI_ESP_HEADER) + IvSize;
  (*FragmentTable)[0].FragmentLength  = (UINT32) PlainPayloadSize;
  *FragmentCount                      = 1;

  //
  // Update the total length field in ip header since processed by esp.
  //
  if (IpVersion == IP_VERSION_4) {
    ((IP4_HEAD *) IpHead)->TotalLen = HTONS ((UINT16) (((IP4_HEAD *) IpHead)->HeadLen + PlainPayloadSize));
  } else {
    IpSecHeadSize                              = IpSecGetPlainExtHeadSize (IpHead, LastHead);
    ((EFI_IP6_HEADER *) IpHead)->PayloadLength = HTONS ((UINT16)(IpSecHeadSize + PlainPayloadSize));
  }
  //
  // Update the next layer field in ip header since esp header inserted.
  //
  *LastHead = NextHeader;

  //
  // Update the spd association of the sad entry.
  //
  *SpdEntry = SadData->SpdEntry;

ON_EXIT:
  if (Payload != NULL) {
    NetbufFree (Payload);
  }

  if (EFI_ERROR (Status)) {
    if (ProcessBuffer != NULL) {
      FreePool (ProcessBuffer);
    }

    if (RecycleContext != NULL) {
      FreePool (RecycleContext);
    }

    if (*RecycleEvent != NULL) {
      gBS->CloseEvent (*RecycleEvent);
    }
  }

  return Status;
}

/**
  The actual entry to the relative function processes the output traffic using the ESP protocol.

  This function is the subfunction of IpSecProtectOutboundPacket(). It protected
  the sending packet by encrypting its payload and inserting ESP header in the orginal
  IP header, then return the IpHeader and IPsec protected Fragmentable.

  @param[in]      IpVersion          The version of IP.
  @param[in, out] IpHead             Points to IP header containing the orginal IP header
                                     to be processed on input, and inserted ESP header
                                     on return.
  @param[in, out] LastHead           The Last Header in IP header.
  @param[in, out] OptionsBuffer      Pointer to the options buffer. It is optional.
  @param[in, out] OptionsLength      Length of the options buffer. It is optional.
  @param[in, out] FragmentTable      Pointer to a list of fragments to be protected by
                                     IPsec on input, and with IPsec protected
                                     on return.
  @param[in, out] FragmentCount      The number of fragments.
  @param[in]      SadEntry           The related SAD entry.
  @param[out]     RecycleEvent       The event for recycling of resources.

  @retval EFI_SUCCESS              The operation was successful.
  @retval EFI_OUT_OF_RESOURCES     The required system resources can't be allocated.

**/
EFI_STATUS
IpSecEspOutboundPacket (
  IN UINT8                           IpVersion,
  IN OUT VOID                        *IpHead,
  IN OUT UINT8                       *LastHead,
  IN OUT VOID                        **OptionsBuffer, OPTIONAL
  IN OUT UINT32                      *OptionsLength,  OPTIONAL
  IN OUT EFI_IPSEC_FRAGMENT_DATA     **FragmentTable,
  IN OUT UINT32                      *FragmentCount,
  IN     IPSEC_SAD_ENTRY             *SadEntry,
     OUT EFI_EVENT                   *RecycleEvent
  )
{
  EFI_STATUS            Status;
  UINTN                 Index;
  EFI_IPSEC_SA_ID       *SaId;
  IPSEC_SAD_DATA        *SadData;
  IPSEC_RECYCLE_CONTEXT *RecycleContext;
  UINT8                 *ProcessBuffer;
  UINTN                 BytesCopied;
  INTN                  EncryptBlockSize;// Size of encryption block, 4 bytes aligned and >= 4
  UINTN                 EspSize;         // Total size of esp wrapped ip payload
  UINTN                 IvSize;          // Size of IV, optional, might be 0
  UINTN                 PlainPayloadSize;// Original IP payload size
  UINTN                 PaddingSize;     // Size of padding
  UINTN                 EncryptSize;     // Size of data to be encrypted, start after IV and
                                         // stop before ICV
  UINTN                 IcvSize;         // Size of ICV, optional, might be 0
  UINT8                 *RestOfPayload;  // Start of Payload after IV
  UINT8                 *Padding;        // Start address of padding
  EFI_ESP_HEADER        *EspHeader;      // Start address of ESP frame
  EFI_ESP_TAIL          *EspTail;        // Address behind padding

  Status          = EFI_ACCESS_DENIED;
  SaId            = SadEntry->Id;
  SadData         = SadEntry->Data;
  ProcessBuffer   = NULL;
  RecycleContext  = NULL;
  *RecycleEvent   = NULL;

  if (!SadData->ManualSet &&
      SadData->AlgoInfo.EspAlgoInfo.EncKey == NULL &&
      SadData->AlgoInfo.EspAlgoInfo.AuthKey == NULL
      ) {
    //
    // Invalid manual sad entry configuration.
    //
    goto ON_EXIT;
  }
  //
  // Calculate enctrypt block size, need iv by default and 4 bytes alignment.
  //
  EncryptBlockSize  = 4;

  if (SadData->AlgoInfo.EspAlgoInfo.EncKey != NULL) {
    EncryptBlockSize  = IpSecGetEncryptBlockSize (SadEntry->Data->AlgoInfo.EspAlgoInfo.EncAlgoId);

    if (EncryptBlockSize < 0 || (EncryptBlockSize != 1 && EncryptBlockSize % 4 != 0)) {
      goto ON_EXIT;
    }
  }
  //
  // Calculate the plain payload size accroding to the fragment table.
  //
  PlainPayloadSize = 0;
  for (Index = 0; Index < *FragmentCount; Index++) {
    PlainPayloadSize += (*FragmentTable)[Index].FragmentLength;
  }
  //
  // Calculate icv size, optional by default and 4 bytes alignment.
  //
  IcvSize = 0;
  if (SadData->AlgoInfo.EspAlgoInfo.AuthKey != NULL) {
    IcvSize = IpSecGetIcvLength (SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthAlgoId);
    if (IcvSize % 4 != 0) {
      goto ON_EXIT;
    }
  }
  //
  // Calcuate the total size of esp wrapped ip payload.
  //
  IvSize        = IpSecGetEncryptIvLength (SadEntry->Data->AlgoInfo.EspAlgoInfo.EncAlgoId);
  EncryptSize   = (PlainPayloadSize + sizeof (EFI_ESP_TAIL) + EncryptBlockSize - 1) / EncryptBlockSize * EncryptBlockSize;
  PaddingSize   = EncryptSize - PlainPayloadSize - sizeof (EFI_ESP_TAIL);
  EspSize       = sizeof (EFI_ESP_HEADER) + IvSize + EncryptSize + IcvSize;

  ProcessBuffer = AllocateZeroPool (EspSize);
  if (ProcessBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }
  //
  // Calculate esp header and esp tail including header, payload and padding.
  //
  EspHeader     = (EFI_ESP_HEADER *) ProcessBuffer;
  RestOfPayload = (UINT8 *) (EspHeader + 1) + IvSize;
  Padding       = RestOfPayload + PlainPayloadSize;
  EspTail       = (EFI_ESP_TAIL *) (Padding + PaddingSize);

  //
  // Fill the sn and spi fields in esp header.
  //
  EspHeader->SequenceNumber = HTONL ((UINT32) SadData->SequenceNumber + 1);
  EspHeader->Spi            = HTONL (SaId->Spi);

  //
  // Copy the rest of payload (after iv) from the original fragment buffer.
  //
  BytesCopied = 0;
  for (Index = 0; Index < *FragmentCount; Index++) {
    CopyMem (
      (RestOfPayload + BytesCopied),
      (*FragmentTable)[Index].FragmentBuffer,
      (*FragmentTable)[Index].FragmentLength
      );
    BytesCopied += (*FragmentTable)[Index].FragmentLength;
  }
  //
  // Fill the padding buffer by natural number sequence.
  //
  for (Index = 0; Index < PaddingSize; Index++) {
    Padding[Index] = (UINT8) (Index + 1);
  }
  //
  // Fill the padding length and next header fields in esp tail.
  //
  EspTail->PaddingLength  = (UINT8) PaddingSize;
  EspTail->NextHeader     = *LastHead;

  //
  // Generate iv at random by crypt library.
  //
  Status = IpSecGenerateIv (
             (UINT8 *) (EspHeader + 1),
             IvSize
             );


  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }
  //
  // Encrypt the payload (after iv) by the sad entry if has encrypt key.
  //
  if (SadData->AlgoInfo.EspAlgoInfo.EncKey != NULL) {
    Status = IpSecEspEncryptPayload (
               RestOfPayload,
               EncryptSize,
               (UINT8 *) (EspHeader + 1),
               IvSize,
               SadEntry
               );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }
  //
  // Authenticate the esp wrapped buffer by the sad entry if has auth key.
  //
  if (SadData->AlgoInfo.EspAlgoInfo.AuthKey != NULL) {
    Status = IpSecAuthPayload (
               ProcessBuffer,
               EspSize - IcvSize,
               ProcessBuffer + EspSize - IcvSize,
               IcvSize,
               SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthKey,
               SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthAlgoId
               );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }
  //
  // Encryption and authentication with esp has been done, so it's time to
  // reload the new packet, create recycle event and fixup ip header.
  //
  RecycleContext = AllocateZeroPool (sizeof (IPSEC_RECYCLE_CONTEXT));
  if (RecycleContext == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  IpSecRecycleCallback,
                  RecycleContext,
                  RecycleEvent
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }
  //
  // TODO: Who take responsible to handle the original fragment table?
  //
  *FragmentTable = AllocateZeroPool (sizeof (EFI_IPSEC_FRAGMENT_DATA));
  if (*FragmentTable == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  RecycleContext->FragmentTable       = *FragmentTable;
  RecycleContext->PayloadBuffer       = ProcessBuffer;
  (*FragmentTable)[0].FragmentBuffer  = ProcessBuffer;
  (*FragmentTable)[0].FragmentLength  = (UINT32) EspSize;
  *FragmentCount                      = 1;

  //
  // Update the total length field in ip header since processed by esp.
  //
  if (IpVersion == IP_VERSION_4) {
    ((IP4_HEAD *) IpHead)->TotalLen = HTONS ((UINT16) (((IP4_HEAD *) IpHead)->HeadLen + EspSize));
  } else {
    ((EFI_IP6_HEADER *) IpHead)->PayloadLength = (UINT16) (IpSecGetPlainExtHeadSize (IpHead, LastHead) + EspSize);
  }
  //
  // Update the next layer field in ip header since esp header inserted.
  //
  *LastHead = IPSEC_ESP_PROTOCOL;

  //
  // Increase the sn number in sad entry according to rfc4303.
  //
  SadData->SequenceNumber++;

ON_EXIT:
  if (EFI_ERROR (Status)) {
    if (ProcessBuffer != NULL) {
      FreePool (ProcessBuffer);
    }

    if (RecycleContext != NULL) {
      FreePool (RecycleContext);
    }

    if (*RecycleEvent != NULL) {
      gBS->CloseEvent (*RecycleEvent);
    }
  }

  return Status;
}

/**
  This function processes the inbound traffic with IPsec.

  It checks the received packet security property, trims the ESP/AH header, and then
  returns without an IPsec protected IP Header and FragmentTable.

  @param[in]      IpVersion          The version of IP.
  @param[in, out] IpHead             Points to IP header containing the ESP/AH header
                                     to be trimed on input, and without ESP/AH header
                                     on return.
  @param[out]     LastHead           The Last Header in IP header on return.
  @param[in, out] OptionsBuffer      Pointer to the options buffer. It is optional.
  @param[in, out] OptionsLength      Length of the options buffer. It is optional.
  @param[in, out] FragmentTable      Pointer to a list of fragments in the form of IPsec
                                     protected on input, and without IPsec protected
                                     on return.
  @param[in, out] FragmentCount      Number of fragments.
  @param[out]     SpdEntry           Pointer to contain the address of SPD entry on return.
  @param[out]     RecycleEvent       Event for recycling of resources.

  @retval EFI_SUCCESS              The operation is successful.
  @retval EFI_UNSUPPORTED          If the IPSEC protocol is not supported.

**/
EFI_STATUS
IpSecProtectInboundPacket (
  IN     UINT8                       IpVersion,
  IN OUT VOID                        *IpHead,
     OUT UINT8                       *LastHead,
  IN OUT VOID                        **OptionsBuffer, OPTIONAL
  IN OUT UINT32                      *OptionsLength,  OPTIONAL
  IN OUT EFI_IPSEC_FRAGMENT_DATA     **FragmentTable,
  IN OUT UINT32                      *FragmentCount,
     OUT IPSEC_SPD_ENTRY             **SpdEntry,
     OUT EFI_EVENT                   *RecycleEvent
  )
{
  if (*LastHead == IPSEC_ESP_PROTOCOL) {
    //
    // Process the esp ipsec header of the inbound traffic.
    //
    return IpSecEspInboundPacket (
             IpVersion,
             IpHead,
             LastHead,
             OptionsBuffer,
             OptionsLength,
             FragmentTable,
             FragmentCount,
             SpdEntry,
             RecycleEvent
             );
  }
  //
  // The other protocols are not supported.
  //
  return EFI_UNSUPPORTED;
}

/**
  This fucntion processes the output traffic with IPsec.

  It protected the sending packet by encrypting it payload and inserting ESP/AH header
  in the orginal IP header, then return the IpHeader and IPsec protected Fragmentable.

  @param[in]      IpVersion          The version of IP.
  @param[in, out] IpHead             Point to IP header containing the orginal IP header
                                     to be processed on input, and inserted ESP/AH header
                                     on return.
  @param[in, out] LastHead           The Last Header in IP header.
  @param[in, out] OptionsBuffer      Pointer to the options buffer. It is optional.
  @param[in, out] OptionsLength      Length of the options buffer. It is optional.
  @param[in, out] FragmentTable      Pointer to a list of fragments to be protected by
                                     IPsec on input, and with IPsec protected
                                     on return.
  @param[in, out] FragmentCount      Number of fragments.
  @param[in]      SadEntry           Related SAD entry.
  @param[out]     RecycleEvent       Event for recycling of resources.

  @retval EFI_SUCCESS              The operation is successful.
  @retval EFI_UNSUPPORTED          If the IPSEC protocol is not supported.

**/
EFI_STATUS
IpSecProtectOutboundPacket (
  IN     UINT8                       IpVersion,
  IN OUT VOID                        *IpHead,
  IN OUT UINT8                       *LastHead,
  IN OUT VOID                        **OptionsBuffer, OPTIONAL
  IN OUT UINT32                      *OptionsLength,  OPTIONAL
  IN OUT EFI_IPSEC_FRAGMENT_DATA     **FragmentTable,
  IN OUT UINT32                      *FragmentCount,
  IN     IPSEC_SAD_ENTRY             *SadEntry,
     OUT EFI_EVENT                   *RecycleEvent
  )
{
  if (SadEntry->Id->Proto == EfiIPsecESP) {
    //
    // Process the esp ipsec header of the outbound traffic.
    //
    return IpSecEspOutboundPacket (
             IpVersion,
             IpHead,
             LastHead,
             OptionsBuffer,
             OptionsLength,
             FragmentTable,
             FragmentCount,
             SadEntry,
             RecycleEvent
             );
  }
  //
  // The other protocols are not supported.
  //
  return EFI_UNSUPPORTED;
}
