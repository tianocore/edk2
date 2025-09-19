/** @file
  Miscellaneous routines specific to Https for HttpDxe driver.

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HttpDriver.h"

/**
  Returns the first occurrence of a Null-terminated ASCII sub-string in a Null-terminated
  ASCII string and ignore case during the search process.

  This function scans the contents of the ASCII string specified by String
  and returns the first occurrence of SearchString and ignore case during the search process.
  If SearchString is not found in String, then NULL is returned. If the length of SearchString
  is zero, then String is returned.

  If String is NULL, then ASSERT().
  If SearchString is NULL, then ASSERT().

  @param[in]  String          A pointer to a Null-terminated ASCII string.
  @param[in]  SearchString    A pointer to a Null-terminated ASCII string to search for.

  @retval NULL            If the SearchString does not appear in String.
  @retval others          If there is a match return the first occurrence of SearchingString.
                          If the length of SearchString is zero,return String.

**/
CHAR8 *
AsciiStrCaseStr (
  IN      CONST CHAR8  *String,
  IN      CONST CHAR8  *SearchString
  )
{
  CONST CHAR8  *FirstMatch;
  CONST CHAR8  *SearchStringTmp;

  CHAR8  Src;
  CHAR8  Dst;

  //
  // ASSERT both strings are less long than PcdMaximumAsciiStringLength
  //
  ASSERT (AsciiStrSize (String) != 0);
  ASSERT (AsciiStrSize (SearchString) != 0);

  if (*SearchString == '\0') {
    return (CHAR8 *)String;
  }

  while (*String != '\0') {
    SearchStringTmp = SearchString;
    FirstMatch      = String;

    while (  (*SearchStringTmp != '\0')
          && (*String != '\0'))
    {
      Src = *String;
      Dst = *SearchStringTmp;

      if ((Src >= 'A') && (Src <= 'Z')) {
        Src += ('a' - 'A');
      }

      if ((Dst >= 'A') && (Dst <= 'Z')) {
        Dst += ('a' - 'A');
      }

      if (Src != Dst) {
        break;
      }

      String++;
      SearchStringTmp++;
    }

    if (*SearchStringTmp == '\0') {
      return (CHAR8 *)FirstMatch;
    }

    String = FirstMatch + 1;
  }

  return NULL;
}

/**
  The callback function to free the net buffer list.

  @param[in]  Arg The opaque parameter.

**/
VOID
EFIAPI
FreeNbufList (
  IN VOID  *Arg
  )
{
  ASSERT (Arg != NULL);

  NetbufFreeList ((LIST_ENTRY *)Arg);
  FreePool (Arg);
}

/**
  Check whether the Url is from Https.

  @param[in]    Url             The pointer to a HTTP or HTTPS URL string.

  @retval TRUE                  The Url is from HTTPS.
  @retval FALSE                 The Url is from HTTP.

**/
BOOLEAN
IsHttpsUrl (
  IN CHAR8  *Url
  )
{
  CHAR8  *Tmp;

  Tmp = NULL;

  Tmp = AsciiStrCaseStr (Url, HTTPS_FLAG);
  if ((Tmp != NULL) && (Tmp == Url)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Creates a Tls child handle, open EFI_TLS_PROTOCOL and EFI_TLS_CONFIGURATION_PROTOCOL.

  @param[in]  HttpInstance  Pointer to HTTP_PROTOCOL structure.

  @return  EFI_SUCCESS        TLS child handle is returned in HttpInstance->TlsChildHandle
                              with opened EFI_TLS_PROTOCOL and EFI_TLS_CONFIGURATION_PROTOCOL.
           EFI_DEVICE_ERROR   TLS service binding protocol is not found.
           Otherwise          Fail to create TLS chile handle.

**/
EFI_STATUS
EFIAPI
TlsCreateChild (
  IN  HTTP_PROTOCOL  *HttpInstance
  )
{
  EFI_HANDLE  ImageHandle;
  EFI_STATUS  Status;

  //
  // Use TlsSb to create Tls child and open the TLS protocol.
  //
  if (HttpInstance->LocalAddressIsIPv6) {
    ImageHandle = HttpInstance->Service->Ip6DriverBindingHandle;
  } else {
    ImageHandle = HttpInstance->Service->Ip4DriverBindingHandle;
  }

  //
  // Locate TlsServiceBinding protocol.
  //
  gBS->LocateProtocol (
         &gEfiTlsServiceBindingProtocolGuid,
         NULL,
         (VOID **)&HttpInstance->TlsSb
         );
  if (HttpInstance->TlsSb == NULL) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Create TLS protocol on HTTP handle, this creates the association between HTTP and TLS
  // for HTTP driver external usages.
  //
  Status = HttpInstance->TlsSb->CreateChild (HttpInstance->TlsSb, &HttpInstance->Handle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HttpInstance->TlsAlreadyCreated = TRUE;
  Status                          = gBS->OpenProtocol (
                                           HttpInstance->Handle,
                                           &gEfiTlsProtocolGuid,
                                           (VOID **)&HttpInstance->Tls,
                                           ImageHandle,
                                           HttpInstance->Handle,
                                           EFI_OPEN_PROTOCOL_GET_PROTOCOL
                                           );
  if (EFI_ERROR (Status)) {
    HttpInstance->TlsSb->DestroyChild (HttpInstance->TlsSb, HttpInstance->Handle);
    HttpInstance->TlsAlreadyCreated = FALSE;
    return Status;
  }

  Status = gBS->OpenProtocol (
                  HttpInstance->Handle,
                  &gEfiTlsConfigurationProtocolGuid,
                  (VOID **)&HttpInstance->TlsConfiguration,
                  ImageHandle,
                  HttpInstance->Handle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    HttpInstance->TlsSb->DestroyChild (HttpInstance->TlsSb, HttpInstance->Handle);
    HttpInstance->TlsAlreadyCreated = FALSE;
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Create event for the TLS receive and transmit tokens which are used to receive and
  transmit TLS related messages.

  @param[in, out]  HttpInstance       Pointer to HTTP_PROTOCOL structure.

  @retval EFI_SUCCESS            The events are created successfully.
  @retval others                 Other error as indicated.

**/
EFI_STATUS
EFIAPI
TlsCreateTxRxEvent (
  IN OUT HTTP_PROTOCOL  *HttpInstance
  )
{
  EFI_STATUS  Status;

  if (!HttpInstance->LocalAddressIsIPv6) {
    //
    // For Tcp4TlsTxToken.
    //
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    HttpCommonNotify,
                    &HttpInstance->TlsIsTxDone,
                    &HttpInstance->Tcp4TlsTxToken.CompletionToken.Event
                    );
    if (EFI_ERROR (Status)) {
      goto ERROR;
    }

    HttpInstance->Tcp4TlsTxData.Push                            = TRUE;
    HttpInstance->Tcp4TlsTxData.Urgent                          = FALSE;
    HttpInstance->Tcp4TlsTxData.DataLength                      = 0;
    HttpInstance->Tcp4TlsTxData.FragmentCount                   = 1;
    HttpInstance->Tcp4TlsTxData.FragmentTable[0].FragmentLength = HttpInstance->Tcp4TlsTxData.DataLength;
    HttpInstance->Tcp4TlsTxData.FragmentTable[0].FragmentBuffer = NULL;
    HttpInstance->Tcp4TlsTxToken.Packet.TxData                  = &HttpInstance->Tcp4TlsTxData;
    HttpInstance->Tcp4TlsTxToken.CompletionToken.Status         = EFI_NOT_READY;

    //
    // For Tcp4TlsRxToken.
    //
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    HttpCommonNotify,
                    &HttpInstance->TlsIsRxDone,
                    &HttpInstance->Tcp4TlsRxToken.CompletionToken.Event
                    );
    if (EFI_ERROR (Status)) {
      goto ERROR;
    }

    HttpInstance->Tcp4TlsRxData.DataLength                      = 0;
    HttpInstance->Tcp4TlsRxData.FragmentCount                   = 1;
    HttpInstance->Tcp4TlsRxData.FragmentTable[0].FragmentLength = HttpInstance->Tcp4TlsRxData.DataLength;
    HttpInstance->Tcp4TlsRxData.FragmentTable[0].FragmentBuffer = NULL;
    HttpInstance->Tcp4TlsRxToken.Packet.RxData                  = &HttpInstance->Tcp4TlsRxData;
    HttpInstance->Tcp4TlsRxToken.CompletionToken.Status         = EFI_NOT_READY;
  } else {
    //
    // For Tcp6TlsTxToken.
    //
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    HttpCommonNotify,
                    &HttpInstance->TlsIsTxDone,
                    &HttpInstance->Tcp6TlsTxToken.CompletionToken.Event
                    );
    if (EFI_ERROR (Status)) {
      goto ERROR;
    }

    HttpInstance->Tcp6TlsTxData.Push                            = TRUE;
    HttpInstance->Tcp6TlsTxData.Urgent                          = FALSE;
    HttpInstance->Tcp6TlsTxData.DataLength                      = 0;
    HttpInstance->Tcp6TlsTxData.FragmentCount                   = 1;
    HttpInstance->Tcp6TlsTxData.FragmentTable[0].FragmentLength = HttpInstance->Tcp6TlsTxData.DataLength;
    HttpInstance->Tcp6TlsTxData.FragmentTable[0].FragmentBuffer = NULL;
    HttpInstance->Tcp6TlsTxToken.Packet.TxData                  = &HttpInstance->Tcp6TlsTxData;
    HttpInstance->Tcp6TlsTxToken.CompletionToken.Status         = EFI_NOT_READY;

    //
    // For Tcp6TlsRxToken.
    //
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    HttpCommonNotify,
                    &HttpInstance->TlsIsRxDone,
                    &HttpInstance->Tcp6TlsRxToken.CompletionToken.Event
                    );
    if (EFI_ERROR (Status)) {
      goto ERROR;
    }

    HttpInstance->Tcp6TlsRxData.DataLength                      = 0;
    HttpInstance->Tcp6TlsRxData.FragmentCount                   = 1;
    HttpInstance->Tcp6TlsRxData.FragmentTable[0].FragmentLength = HttpInstance->Tcp6TlsRxData.DataLength;
    HttpInstance->Tcp6TlsRxData.FragmentTable[0].FragmentBuffer = NULL;
    HttpInstance->Tcp6TlsRxToken.Packet.RxData                  = &HttpInstance->Tcp6TlsRxData;
    HttpInstance->Tcp6TlsRxToken.CompletionToken.Status         = EFI_NOT_READY;
  }

  return Status;

ERROR:
  //
  // Error handling
  //
  TlsCloseTxRxEvent (HttpInstance);

  return Status;
}

/**
  Close events in the TlsTxToken and TlsRxToken.

  @param[in]  HttpInstance   Pointer to HTTP_PROTOCOL structure.

**/
VOID
EFIAPI
TlsCloseTxRxEvent (
  IN  HTTP_PROTOCOL  *HttpInstance
  )
{
  ASSERT (HttpInstance != NULL);
  if (!HttpInstance->LocalAddressIsIPv6) {
    if (NULL != HttpInstance->Tcp4TlsTxToken.CompletionToken.Event) {
      gBS->CloseEvent (HttpInstance->Tcp4TlsTxToken.CompletionToken.Event);
      HttpInstance->Tcp4TlsTxToken.CompletionToken.Event = NULL;
    }

    if (NULL != HttpInstance->Tcp4TlsRxToken.CompletionToken.Event) {
      gBS->CloseEvent (HttpInstance->Tcp4TlsRxToken.CompletionToken.Event);
      HttpInstance->Tcp4TlsRxToken.CompletionToken.Event = NULL;
    }
  } else {
    if (NULL != HttpInstance->Tcp6TlsTxToken.CompletionToken.Event) {
      gBS->CloseEvent (HttpInstance->Tcp6TlsTxToken.CompletionToken.Event);
      HttpInstance->Tcp6TlsTxToken.CompletionToken.Event = NULL;
    }

    if (NULL != HttpInstance->Tcp6TlsRxToken.CompletionToken.Event) {
      gBS->CloseEvent (HttpInstance->Tcp6TlsRxToken.CompletionToken.Event);
      HttpInstance->Tcp6TlsRxToken.CompletionToken.Event = NULL;
    }
  }
}

/**
  Read the TlsCaCertificate variable and configure it.

  @param[in, out]  HttpInstance       The HTTP instance private data.

  @retval EFI_SUCCESS            TlsCaCertificate is configured.
  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @retval EFI_NOT_FOUND          Fail to get 'TlsCaCertificate' variable.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
TlsConfigCertificate (
  IN OUT HTTP_PROTOCOL  *HttpInstance
  )
{
  EFI_STATUS          Status;
  UINT8               *CACert;
  UINTN               CACertSize;
  UINTN               Index;
  EFI_SIGNATURE_LIST  *CertList;
  EFI_SIGNATURE_DATA  *Cert;
  UINTN               CertArraySizeInBytes;
  UINTN               CertCount;
  UINT32              ItemDataSize;

  CACert     = NULL;
  CACertSize = 0;

  //
  // Try to read the TlsCaCertificate variable.
  //
  Status = gRT->GetVariable (
                  EFI_TLS_CA_CERTIFICATE_VARIABLE,
                  &gEfiTlsCaCertificateGuid,
                  NULL,
                  &CACertSize,
                  NULL
                  );

  if (EFI_ERROR (Status) && (Status != EFI_BUFFER_TOO_SMALL)) {
    return Status;
  }

  //
  // Allocate buffer and read the config variable.
  //
  CACert = AllocatePool (CACertSize);
  if (CACert == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gRT->GetVariable (
                  EFI_TLS_CA_CERTIFICATE_VARIABLE,
                  &gEfiTlsCaCertificateGuid,
                  NULL,
                  &CACertSize,
                  CACert
                  );
  if (EFI_ERROR (Status)) {
    //
    // GetVariable still error or the variable is corrupted.
    //
    goto FreeCACert;
  }

  ASSERT (CACert != NULL);

  //
  // Sanity check
  //
  Status       = EFI_INVALID_PARAMETER;
  CertCount    = 0;
  ItemDataSize = (UINT32)CACertSize;
  while (ItemDataSize > 0) {
    if (ItemDataSize < sizeof (EFI_SIGNATURE_LIST)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: truncated EFI_SIGNATURE_LIST header\n",
        __func__
        ));
      goto FreeCACert;
    }

    CertList = (EFI_SIGNATURE_LIST *)(CACert + (CACertSize - ItemDataSize));

    if (CertList->SignatureListSize < sizeof (EFI_SIGNATURE_LIST)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: SignatureListSize too small for EFI_SIGNATURE_LIST\n",
        __func__
        ));
      goto FreeCACert;
    }

    if (CertList->SignatureListSize > ItemDataSize) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: truncated EFI_SIGNATURE_LIST body\n",
        __func__
        ));
      goto FreeCACert;
    }

    if (!CompareGuid (&CertList->SignatureType, &gEfiCertX509Guid)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: only X509 certificates are supported\n",
        __func__
        ));
      Status = EFI_UNSUPPORTED;
      goto FreeCACert;
    }

    if (CertList->SignatureHeaderSize != 0) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: SignatureHeaderSize must be 0 for X509\n",
        __func__
        ));
      goto FreeCACert;
    }

    if (CertList->SignatureSize < sizeof (EFI_SIGNATURE_DATA)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: SignatureSize too small for EFI_SIGNATURE_DATA\n",
        __func__
        ));
      goto FreeCACert;
    }

    CertArraySizeInBytes = (CertList->SignatureListSize -
                            sizeof (EFI_SIGNATURE_LIST));
    if (CertArraySizeInBytes % CertList->SignatureSize != 0) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: EFI_SIGNATURE_DATA array not a multiple of SignatureSize\n",
        __func__
        ));
      goto FreeCACert;
    }

    CertCount    += CertArraySizeInBytes / CertList->SignatureSize;
    ItemDataSize -= CertList->SignatureListSize;
  }

  if (CertCount == 0) {
    DEBUG ((DEBUG_ERROR, "%a: no X509 certificates provided\n", __func__));
    goto FreeCACert;
  }

  //
  // Enumerate all data and erasing the target item.
  //
  ItemDataSize = (UINT32)CACertSize;
  CertList     = (EFI_SIGNATURE_LIST *)CACert;
  while ((ItemDataSize > 0) && (ItemDataSize >= CertList->SignatureListSize)) {
    Cert      = (EFI_SIGNATURE_DATA *)((UINT8 *)CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
    CertCount = (CertList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - CertList->SignatureHeaderSize) / CertList->SignatureSize;
    for (Index = 0; Index < CertCount; Index++) {
      //
      // EfiTlsConfigDataTypeCACertificate
      //
      Status = HttpInstance->TlsConfiguration->SetData (
                                                 HttpInstance->TlsConfiguration,
                                                 EfiTlsConfigDataTypeCACertificate,
                                                 Cert->SignatureData,
                                                 CertList->SignatureSize - sizeof (Cert->SignatureOwner)
                                                 );
      if (EFI_ERROR (Status)) {
        goto FreeCACert;
      }

      Cert = (EFI_SIGNATURE_DATA *)((UINT8 *)Cert + CertList->SignatureSize);
    }

    ItemDataSize -= CertList->SignatureListSize;
    CertList      = (EFI_SIGNATURE_LIST *)((UINT8 *)CertList + CertList->SignatureListSize);
  }

FreeCACert:
  FreePool (CACert);
  return Status;
}

/**
  Read the HttpTlsCipherList variable and configure it for HTTPS session.

  @param[in, out]  HttpInstance  The HTTP instance private data.

  @retval EFI_SUCCESS            The prefered HTTP TLS CipherList is configured.
  @retval EFI_NOT_FOUND          Fail to get 'HttpTlsCipherList' variable.
  @retval EFI_INVALID_PARAMETER  The contents of variable are invalid.
  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.

  @retval Others                 Other error as indicated.

**/
EFI_STATUS
TlsConfigCipherList (
  IN OUT HTTP_PROTOCOL  *HttpInstance
  )
{
  EFI_STATUS  Status;
  UINT8       *CipherList;
  UINTN       CipherListSize;

  CipherList     = NULL;
  CipherListSize = 0;

  //
  // Try to read the HttpTlsCipherList variable.
  //
  Status = gRT->GetVariable (
                  EDKII_HTTP_TLS_CIPHER_LIST_VARIABLE,
                  &gEdkiiHttpTlsCipherListGuid,
                  NULL,
                  &CipherListSize,
                  NULL
                  );
  ASSERT (EFI_ERROR (Status));
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  if (CipherListSize % sizeof (EFI_TLS_CIPHER) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Allocate buffer and read the config variable.
  //
  CipherList = AllocatePool (CipherListSize);
  if (CipherList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gRT->GetVariable (
                  EDKII_HTTP_TLS_CIPHER_LIST_VARIABLE,
                  &gEdkiiHttpTlsCipherListGuid,
                  NULL,
                  &CipherListSize,
                  CipherList
                  );
  if (EFI_ERROR (Status)) {
    //
    // GetVariable still error or the variable is corrupted.
    //
    goto ON_EXIT;
  }

  ASSERT (CipherList != NULL);

  Status = HttpInstance->Tls->SetSessionData (
                                HttpInstance->Tls,
                                EfiTlsCipherList,
                                CipherList,
                                CipherListSize
                                );

ON_EXIT:
  FreePool (CipherList);

  return Status;
}

/**
  Configure TLS session data.

  @param[in, out]  HttpInstance       The HTTP instance private data.

  @retval EFI_SUCCESS            TLS session data is configured.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
EFIAPI
TlsConfigureSession (
  IN OUT HTTP_PROTOCOL  *HttpInstance
  )
{
  EFI_STATUS  Status;

  //
  // TlsConfigData initialization
  //
  HttpInstance->TlsConfigData.ConnectionEnd    = EfiTlsClient;
  HttpInstance->TlsConfigData.VerifyMethod     = EFI_TLS_VERIFY_PEER;
  HttpInstance->TlsConfigData.VerifyHost.Flags = EFI_TLS_VERIFY_FLAG_NONE;
  HttpInstance->TlsConfigData.SessionState     = EfiTlsSessionNotStarted;

  if (HttpInstance->ProxyConnected) {
    ASSERT (HttpInstance->EndPointHostName != NULL);
    HttpInstance->TlsConfigData.VerifyHost.HostName = HttpInstance->EndPointHostName;
  } else {
    HttpInstance->TlsConfigData.VerifyHost.HostName = HttpInstance->RemoteHost;
  }

  //
  // EfiTlsConnectionEnd,
  // EfiTlsVerifyMethod,
  // EfiTlsVerifyHost,
  // EfiTlsSessionState
  //
  Status = HttpInstance->Tls->SetSessionData (
                                HttpInstance->Tls,
                                EfiTlsConnectionEnd,
                                &(HttpInstance->TlsConfigData.ConnectionEnd),
                                sizeof (EFI_TLS_CONNECTION_END)
                                );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HttpInstance->Tls->SetSessionData (
                                HttpInstance->Tls,
                                EfiTlsVerifyMethod,
                                &HttpInstance->TlsConfigData.VerifyMethod,
                                sizeof (EFI_TLS_VERIFY)
                                );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HttpInstance->Tls->SetSessionData (
                                HttpInstance->Tls,
                                EfiTlsVerifyHost,
                                &HttpInstance->TlsConfigData.VerifyHost,
                                sizeof (EFI_TLS_VERIFY_HOST)
                                );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HttpInstance->Tls->SetSessionData (
                                HttpInstance->Tls,
                                EfiTlsSessionState,
                                &(HttpInstance->TlsConfigData.SessionState),
                                sizeof (EFI_TLS_SESSION_STATE)
                                );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Tls Cipher List
  //
  Status = TlsConfigCipherList (HttpInstance);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "TlsConfigCipherList: return %r error.\n", Status));
    return Status;
  }

  //
  // Tls Config Certificate
  //
  Status = TlsConfigCertificate (HttpInstance);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      DEBUG ((DEBUG_WARN, "TLS Certificate is not found on the system!\n"));
      //
      // We still return EFI_SUCCESS to the caller when TlsConfigCertificate
      // returns error, for the use case the platform doesn't require
      // certificate for the specific HTTP session. This ensures
      // HttpInitSession function still initiated and returns EFI_SUCCESS to
      // the caller. The failure is pushed back to TLS DXE driver if the
      // HTTP communication actually requires certificate.
      //
    } else {
      DEBUG ((DEBUG_ERROR, "TLS Certificate Config Error!\n"));
      return Status;
    }
  }

  //
  // TlsCreateTxRxEvent
  //
  Status = TlsCreateTxRxEvent (HttpInstance);
  if (EFI_ERROR (Status)) {
    goto ERROR;
  }

  return Status;

ERROR:
  TlsCloseTxRxEvent (HttpInstance);

  return Status;
}

/**
  Transmit the Packet by processing the associated HTTPS token.

  @param[in, out]   HttpInstance    Pointer to HTTP_PROTOCOL structure.
  @param[in]        Packet          The packet to transmit.

  @retval EFI_SUCCESS            The packet is transmitted.
  @retval EFI_INVALID_PARAMETER  HttpInstance is NULL or Packet is NULL.
  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
EFIAPI
TlsCommonTransmit (
  IN OUT HTTP_PROTOCOL  *HttpInstance,
  IN     NET_BUF        *Packet
  )
{
  EFI_STATUS  Status;
  VOID        *Data;
  UINTN       Size;

  if ((HttpInstance == NULL) || (Packet == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!HttpInstance->LocalAddressIsIPv6) {
    Size = sizeof (EFI_TCP4_TRANSMIT_DATA) +
           (Packet->BlockOpNum - 1) * sizeof (EFI_TCP4_FRAGMENT_DATA);
  } else {
    Size = sizeof (EFI_TCP6_TRANSMIT_DATA) +
           (Packet->BlockOpNum - 1) * sizeof (EFI_TCP6_FRAGMENT_DATA);
  }

  Data = AllocatePool (Size);
  if (Data == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (!HttpInstance->LocalAddressIsIPv6) {
    ((EFI_TCP4_TRANSMIT_DATA *)Data)->Push       = TRUE;
    ((EFI_TCP4_TRANSMIT_DATA *)Data)->Urgent     = FALSE;
    ((EFI_TCP4_TRANSMIT_DATA *)Data)->DataLength = Packet->TotalSize;

    //
    // Build the fragment table.
    //
    ((EFI_TCP4_TRANSMIT_DATA *)Data)->FragmentCount = Packet->BlockOpNum;

    NetbufBuildExt (
      Packet,
      (NET_FRAGMENT *)&((EFI_TCP4_TRANSMIT_DATA *)Data)->FragmentTable[0],
      &((EFI_TCP4_TRANSMIT_DATA *)Data)->FragmentCount
      );

    HttpInstance->Tcp4TlsTxToken.Packet.TxData = (EFI_TCP4_TRANSMIT_DATA *)Data;

    Status = EFI_DEVICE_ERROR;

    //
    // Transmit the packet.
    //
    Status = HttpInstance->Tcp4->Transmit (HttpInstance->Tcp4, &HttpInstance->Tcp4TlsTxToken);
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    while (!HttpInstance->TlsIsTxDone) {
      HttpInstance->Tcp4->Poll (HttpInstance->Tcp4);
    }

    HttpInstance->TlsIsTxDone = FALSE;
    Status                    = HttpInstance->Tcp4TlsTxToken.CompletionToken.Status;
  } else {
    ((EFI_TCP6_TRANSMIT_DATA *)Data)->Push       = TRUE;
    ((EFI_TCP6_TRANSMIT_DATA *)Data)->Urgent     = FALSE;
    ((EFI_TCP6_TRANSMIT_DATA *)Data)->DataLength = Packet->TotalSize;

    //
    // Build the fragment table.
    //
    ((EFI_TCP6_TRANSMIT_DATA *)Data)->FragmentCount = Packet->BlockOpNum;

    NetbufBuildExt (
      Packet,
      (NET_FRAGMENT *)&((EFI_TCP6_TRANSMIT_DATA *)Data)->FragmentTable[0],
      &((EFI_TCP6_TRANSMIT_DATA *)Data)->FragmentCount
      );

    HttpInstance->Tcp6TlsTxToken.Packet.TxData = (EFI_TCP6_TRANSMIT_DATA *)Data;

    Status = EFI_DEVICE_ERROR;

    //
    // Transmit the packet.
    //
    Status = HttpInstance->Tcp6->Transmit (HttpInstance->Tcp6, &HttpInstance->Tcp6TlsTxToken);
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    while (!HttpInstance->TlsIsTxDone) {
      HttpInstance->Tcp6->Poll (HttpInstance->Tcp6);
    }

    HttpInstance->TlsIsTxDone = FALSE;
    Status                    = HttpInstance->Tcp6TlsTxToken.CompletionToken.Status;
  }

ON_EXIT:
  FreePool (Data);

  return Status;
}

/**
  Receive the Packet by processing the associated HTTPS token.

  @param[in, out]   HttpInstance    Pointer to HTTP_PROTOCOL structure.
  @param[in]        Packet          The packet to transmit.
  @param[in]        Timeout         The time to wait for connection done.

  @retval EFI_SUCCESS            The Packet is received.
  @retval EFI_INVALID_PARAMETER  HttpInstance is NULL or Packet is NULL.
  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @retval EFI_TIMEOUT            The operation is time out.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
EFIAPI
TlsCommonReceive (
  IN OUT HTTP_PROTOCOL  *HttpInstance,
  IN     NET_BUF        *Packet,
  IN     EFI_EVENT      Timeout
  )
{
  EFI_TCP4_RECEIVE_DATA  *Tcp4RxData;
  EFI_TCP6_RECEIVE_DATA  *Tcp6RxData;
  EFI_STATUS             Status;
  NET_FRAGMENT           *Fragment;
  UINT32                 FragmentCount;
  UINT32                 CurrentFragment;

  Tcp4RxData = NULL;
  Tcp6RxData = NULL;

  if ((HttpInstance == NULL) || (Packet == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  FragmentCount = Packet->BlockOpNum;
  Fragment      = AllocatePool (FragmentCount * sizeof (NET_FRAGMENT));
  if (Fragment == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Build the fragment table.
  //
  NetbufBuildExt (Packet, Fragment, &FragmentCount);

  if (!HttpInstance->LocalAddressIsIPv6) {
    Tcp4RxData = HttpInstance->Tcp4TlsRxToken.Packet.RxData;
    if (Tcp4RxData == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    Tcp4RxData->FragmentCount = 1;
  } else {
    Tcp6RxData = HttpInstance->Tcp6TlsRxToken.Packet.RxData;
    if (Tcp6RxData == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    Tcp6RxData->FragmentCount = 1;
  }

  CurrentFragment = 0;
  Status          = EFI_SUCCESS;

  while (CurrentFragment < FragmentCount) {
    if (!HttpInstance->LocalAddressIsIPv6) {
      Tcp4RxData->DataLength                      = Fragment[CurrentFragment].Len;
      Tcp4RxData->FragmentTable[0].FragmentLength = Fragment[CurrentFragment].Len;
      Tcp4RxData->FragmentTable[0].FragmentBuffer = Fragment[CurrentFragment].Bulk;
      Status                                      = HttpInstance->Tcp4->Receive (HttpInstance->Tcp4, &HttpInstance->Tcp4TlsRxToken);
    } else {
      Tcp6RxData->DataLength                      = Fragment[CurrentFragment].Len;
      Tcp6RxData->FragmentTable[0].FragmentLength = Fragment[CurrentFragment].Len;
      Tcp6RxData->FragmentTable[0].FragmentBuffer = Fragment[CurrentFragment].Bulk;
      Status                                      = HttpInstance->Tcp6->Receive (HttpInstance->Tcp6, &HttpInstance->Tcp6TlsRxToken);
    }

    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    while (!HttpInstance->TlsIsRxDone && ((Timeout == NULL) || EFI_ERROR (gBS->CheckEvent (Timeout)))) {
      //
      // Poll until some data is received or an error occurs.
      //
      if (!HttpInstance->LocalAddressIsIPv6) {
        HttpInstance->Tcp4->Poll (HttpInstance->Tcp4);
      } else {
        HttpInstance->Tcp6->Poll (HttpInstance->Tcp6);
      }
    }

    if (!HttpInstance->TlsIsRxDone) {
      //
      // Timeout occurs, cancel the receive request.
      //
      if (!HttpInstance->LocalAddressIsIPv6) {
        HttpInstance->Tcp4->Cancel (HttpInstance->Tcp4, &HttpInstance->Tcp4TlsRxToken.CompletionToken);
      } else {
        HttpInstance->Tcp6->Cancel (HttpInstance->Tcp6, &HttpInstance->Tcp6TlsRxToken.CompletionToken);
      }

      Status = EFI_TIMEOUT;
      goto ON_EXIT;
    } else {
      HttpInstance->TlsIsRxDone = FALSE;
    }

    if (!HttpInstance->LocalAddressIsIPv6) {
      Status = HttpInstance->Tcp4TlsRxToken.CompletionToken.Status;
      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }

      Fragment[CurrentFragment].Len -= Tcp4RxData->FragmentTable[0].FragmentLength;
      if (Fragment[CurrentFragment].Len == 0) {
        CurrentFragment++;
      } else {
        Fragment[CurrentFragment].Bulk += Tcp4RxData->FragmentTable[0].FragmentLength;
      }
    } else {
      Status = HttpInstance->Tcp6TlsRxToken.CompletionToken.Status;
      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }

      Fragment[CurrentFragment].Len -= Tcp6RxData->FragmentTable[0].FragmentLength;
      if (Fragment[CurrentFragment].Len == 0) {
        CurrentFragment++;
      } else {
        Fragment[CurrentFragment].Bulk += Tcp6RxData->FragmentTable[0].FragmentLength;
      }
    }
  }

ON_EXIT:

  if (Fragment != NULL) {
    FreePool (Fragment);
  }

  return Status;
}

/**
  Receive one TLS PDU. An TLS PDU contains an TLS record header and its
  corresponding record data. These two parts will be put into two blocks of buffers in the
  net buffer.

  @param[in, out]      HttpInstance    Pointer to HTTP_PROTOCOL structure.
  @param[out]          Pdu             The received TLS PDU.
  @param[in]           Timeout         The time to wait for connection done.

  @retval EFI_SUCCESS          An TLS PDU is received.
  @retval EFI_OUT_OF_RESOURCES Can't allocate memory resources.
  @retval EFI_PROTOCOL_ERROR   An unexpected TLS packet was received.
  @retval Others               Other errors as indicated.

**/
EFI_STATUS
EFIAPI
TlsReceiveOnePdu (
  IN OUT HTTP_PROTOCOL  *HttpInstance,
  OUT NET_BUF           **Pdu,
  IN     EFI_EVENT      Timeout
  )
{
  EFI_STATUS  Status;

  LIST_ENTRY  *NbufList;

  UINT32  Len;

  NET_BUF            *PduHdr;
  UINT8              *Header;
  TLS_RECORD_HEADER  RecordHeader;

  NET_BUF  *DataSeg;

  NbufList = NULL;
  PduHdr   = NULL;
  Header   = NULL;
  DataSeg  = NULL;

  NbufList = AllocatePool (sizeof (LIST_ENTRY));
  if (NbufList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeListHead (NbufList);

  //
  // Allocate buffer to receive one TLS header.
  //
  Len    = TLS_RECORD_HEADER_LENGTH;
  PduHdr = NetbufAlloc (Len);
  if (PduHdr == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Header = NetbufAllocSpace (PduHdr, Len, NET_BUF_TAIL);
  if (Header == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // First step, receive one TLS header.
  //
  Status = TlsCommonReceive (HttpInstance, PduHdr, Timeout);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  RecordHeader = *(TLS_RECORD_HEADER *)Header;
  if (((RecordHeader.ContentType == TlsContentTypeHandshake) ||
       (RecordHeader.ContentType == TlsContentTypeAlert) ||
       (RecordHeader.ContentType == TlsContentTypeChangeCipherSpec) ||
       (RecordHeader.ContentType == TlsContentTypeApplicationData)) &&
      (RecordHeader.Version.Major == 0x03) && /// Major versions are same.
      ((RecordHeader.Version.Minor == TLS10_PROTOCOL_VERSION_MINOR) ||
       (RecordHeader.Version.Minor == TLS11_PROTOCOL_VERSION_MINOR) ||
       (RecordHeader.Version.Minor == TLS12_PROTOCOL_VERSION_MINOR))
      )
  {
    InsertTailList (NbufList, &PduHdr->List);
  } else {
    Status = EFI_PROTOCOL_ERROR;
    goto ON_EXIT;
  }

  Len = SwapBytes16 (RecordHeader.Length);
  if (Len == 0) {
    //
    // No TLS payload.
    //
    goto FORM_PDU;
  }

  //
  // Allocate buffer to receive one TLS payload.
  //
  DataSeg = NetbufAlloc (Len);
  if (DataSeg == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  NetbufAllocSpace (DataSeg, Len, NET_BUF_TAIL);

  //
  // Second step, receive one TLS payload.
  //
  Status = TlsCommonReceive (HttpInstance, DataSeg, Timeout);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  InsertTailList (NbufList, &DataSeg->List);

FORM_PDU:
  //
  // Form the PDU from a list of PDU.
  //
  *Pdu = NetbufFromBufList (NbufList, 0, 0, FreeNbufList, NbufList);
  if (*Pdu == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
  }

ON_EXIT:

  if (EFI_ERROR (Status)) {
    //
    // Free the Nbufs in this NbufList and the NbufList itself.
    //
    FreeNbufList (NbufList);
  }

  return Status;
}

/**
  Connect one TLS session by finishing the TLS handshake process.

  @param[in]  HttpInstance       The HTTP instance private data.
  @param[in]  Timeout            The time to wait for connection done.

  @retval EFI_SUCCESS            The TLS session is established.
  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @retval EFI_ABORTED            TLS session state is incorrect.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
EFIAPI
TlsConnectSession (
  IN  HTTP_PROTOCOL  *HttpInstance,
  IN  EFI_EVENT      Timeout
  )
{
  EFI_STATUS  Status;
  UINT8       *BufferOut;
  UINTN       BufferOutSize;
  NET_BUF     *PacketOut;
  UINT8       *DataOut;
  NET_BUF     *Pdu;
  UINT8       *BufferIn;
  UINTN       BufferInSize;
  UINT8       *GetSessionDataBuffer;
  UINTN       GetSessionDataBufferSize;

  BufferOut = NULL;
  PacketOut = NULL;
  DataOut   = NULL;
  Pdu       = NULL;
  BufferIn  = NULL;

  //
  // Initialize TLS state.
  //
  HttpInstance->TlsSessionState = EfiTlsSessionNotStarted;
  Status                        = HttpInstance->Tls->SetSessionData (
                                                       HttpInstance->Tls,
                                                       EfiTlsSessionState,
                                                       &(HttpInstance->TlsSessionState),
                                                       sizeof (EFI_TLS_SESSION_STATE)
                                                       );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Create ClientHello
  //
  BufferOutSize = DEF_BUF_LEN;
  BufferOut     = AllocateZeroPool (BufferOutSize);
  if (BufferOut == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    return Status;
  }

  Status = HttpInstance->Tls->BuildResponsePacket (
                                HttpInstance->Tls,
                                NULL,
                                0,
                                BufferOut,
                                &BufferOutSize
                                );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    FreePool (BufferOut);
    BufferOut = AllocateZeroPool (BufferOutSize);
    if (BufferOut == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      return Status;
    }

    Status = HttpInstance->Tls->BuildResponsePacket (
                                  HttpInstance->Tls,
                                  NULL,
                                  0,
                                  BufferOut,
                                  &BufferOutSize
                                  );
  }

  if (EFI_ERROR (Status)) {
    FreePool (BufferOut);
    return Status;
  }

  //
  // Transmit ClientHello
  //
  PacketOut = NetbufAlloc ((UINT32)BufferOutSize);
  if (PacketOut == NULL) {
    FreePool (BufferOut);
    return EFI_OUT_OF_RESOURCES;
  }

  DataOut = NetbufAllocSpace (PacketOut, (UINT32)BufferOutSize, NET_BUF_TAIL);
  if (DataOut == NULL) {
    FreePool (BufferOut);
    NetbufFree (PacketOut);
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (DataOut, BufferOut, BufferOutSize);
  Status = TlsCommonTransmit (HttpInstance, PacketOut);

  FreePool (BufferOut);
  NetbufFree (PacketOut);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (HttpInstance->TlsSessionState != EfiTlsSessionDataTransferring && \
         ((Timeout == NULL) || EFI_ERROR (gBS->CheckEvent (Timeout))))
  {
    //
    // Receive one TLS record.
    //
    Status = TlsReceiveOnePdu (HttpInstance, &Pdu, Timeout);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    BufferInSize = Pdu->TotalSize;
    BufferIn     = AllocateZeroPool (BufferInSize);
    if (BufferIn == NULL) {
      NetbufFree (Pdu);
      Status = EFI_OUT_OF_RESOURCES;
      return Status;
    }

    NetbufCopy (Pdu, 0, (UINT32)BufferInSize, BufferIn);

    NetbufFree (Pdu);

    //
    // Handle Receive data.
    //
    BufferOutSize = DEF_BUF_LEN;
    BufferOut     = AllocateZeroPool (BufferOutSize);
    if (BufferOut == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      return Status;
    }

    Status = HttpInstance->Tls->BuildResponsePacket (
                                  HttpInstance->Tls,
                                  BufferIn,
                                  BufferInSize,
                                  BufferOut,
                                  &BufferOutSize
                                  );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      FreePool (BufferOut);
      BufferOut = AllocateZeroPool (BufferOutSize);
      if (BufferOut == NULL) {
        FreePool (BufferIn);
        Status = EFI_OUT_OF_RESOURCES;
        return Status;
      }

      Status = HttpInstance->Tls->BuildResponsePacket (
                                    HttpInstance->Tls,
                                    BufferIn,
                                    BufferInSize,
                                    BufferOut,
                                    &BufferOutSize
                                    );
    }

    FreePool (BufferIn);

    if (EFI_ERROR (Status)) {
      FreePool (BufferOut);
      return Status;
    }

    if (BufferOutSize != 0) {
      //
      // Transmit the response packet.
      //
      PacketOut = NetbufAlloc ((UINT32)BufferOutSize);
      if (PacketOut == NULL) {
        FreePool (BufferOut);
        return EFI_OUT_OF_RESOURCES;
      }

      DataOut = NetbufAllocSpace (PacketOut, (UINT32)BufferOutSize, NET_BUF_TAIL);
      if (DataOut == NULL) {
        FreePool (BufferOut);
        NetbufFree (PacketOut);

        return EFI_OUT_OF_RESOURCES;
      }

      CopyMem (DataOut, BufferOut, BufferOutSize);

      Status = TlsCommonTransmit (HttpInstance, PacketOut);

      NetbufFree (PacketOut);

      if (EFI_ERROR (Status)) {
        FreePool (BufferOut);
        return Status;
      }
    }

    FreePool (BufferOut);

    //
    // Get the session state, then decide whether need to continue handle received packet.
    //
    GetSessionDataBufferSize = DEF_BUF_LEN;
    GetSessionDataBuffer     = AllocateZeroPool (GetSessionDataBufferSize);
    if (GetSessionDataBuffer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      return Status;
    }

    Status = HttpInstance->Tls->GetSessionData (
                                  HttpInstance->Tls,
                                  EfiTlsSessionState,
                                  GetSessionDataBuffer,
                                  &GetSessionDataBufferSize
                                  );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      FreePool (GetSessionDataBuffer);
      GetSessionDataBuffer = AllocateZeroPool (GetSessionDataBufferSize);
      if (GetSessionDataBuffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        return Status;
      }

      Status = HttpInstance->Tls->GetSessionData (
                                    HttpInstance->Tls,
                                    EfiTlsSessionState,
                                    GetSessionDataBuffer,
                                    &GetSessionDataBufferSize
                                    );
    }

    if (EFI_ERROR (Status)) {
      FreePool (GetSessionDataBuffer);
      return Status;
    }

    ASSERT (GetSessionDataBufferSize == sizeof (EFI_TLS_SESSION_STATE));
    HttpInstance->TlsSessionState = *(EFI_TLS_SESSION_STATE *)GetSessionDataBuffer;

    FreePool (GetSessionDataBuffer);

    if (HttpInstance->TlsSessionState == EfiTlsSessionError) {
      return EFI_ABORTED;
    }
  }

  if (HttpInstance->TlsSessionState != EfiTlsSessionDataTransferring) {
    Status = EFI_ABORTED;
  }

  return Status;
}

/**
  Close the TLS session and send out the close notification message.

  @param[in]  HttpInstance       The HTTP instance private data.

  @retval EFI_SUCCESS            The TLS session is closed.
  @retval EFI_INVALID_PARAMETER  HttpInstance is NULL.
  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
EFIAPI
TlsCloseSession (
  IN  HTTP_PROTOCOL  *HttpInstance
  )
{
  EFI_STATUS  Status;

  UINT8  *BufferOut;
  UINTN  BufferOutSize;

  NET_BUF  *PacketOut;
  UINT8    *DataOut;

  Status    = EFI_SUCCESS;
  BufferOut = NULL;
  PacketOut = NULL;
  DataOut   = NULL;

  if (HttpInstance == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HttpInstance->TlsSessionState = EfiTlsSessionClosing;

  Status = HttpInstance->Tls->SetSessionData (
                                HttpInstance->Tls,
                                EfiTlsSessionState,
                                &(HttpInstance->TlsSessionState),
                                sizeof (EFI_TLS_SESSION_STATE)
                                );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BufferOutSize = DEF_BUF_LEN;
  BufferOut     = AllocateZeroPool (BufferOutSize);
  if (BufferOut == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    return Status;
  }

  Status = HttpInstance->Tls->BuildResponsePacket (
                                HttpInstance->Tls,
                                NULL,
                                0,
                                BufferOut,
                                &BufferOutSize
                                );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    FreePool (BufferOut);
    BufferOut = AllocateZeroPool (BufferOutSize);
    if (BufferOut == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      return Status;
    }

    Status = HttpInstance->Tls->BuildResponsePacket (
                                  HttpInstance->Tls,
                                  NULL,
                                  0,
                                  BufferOut,
                                  &BufferOutSize
                                  );
  }

  if (EFI_ERROR (Status)) {
    FreePool (BufferOut);
    return Status;
  }

  PacketOut = NetbufAlloc ((UINT32)BufferOutSize);
  if (PacketOut == NULL) {
    FreePool (BufferOut);
    return EFI_OUT_OF_RESOURCES;
  }

  DataOut = NetbufAllocSpace (PacketOut, (UINT32)BufferOutSize, NET_BUF_TAIL);
  if (DataOut == NULL) {
    FreePool (BufferOut);
    NetbufFree (PacketOut);
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (DataOut, BufferOut, BufferOutSize);

  Status = TlsCommonTransmit (HttpInstance, PacketOut);

  FreePool (BufferOut);
  NetbufFree (PacketOut);

  return Status;
}

/**
  Process one message according to the CryptMode.

  @param[in]           HttpInstance    Pointer to HTTP_PROTOCOL structure.
  @param[in]           Message         Pointer to the message buffer needed to processed.
                                       If ProcessMode is EfiTlsEncrypt, the message contain the TLS
                                       header and plain text TLS APP payload.
                                       If ProcessMode is EfiTlsDecrypt, the message contain the TLS
                                       header and cipher text TLS APP payload.
  @param[in]           MessageSize     Pointer to the message buffer size.
  @param[in]           ProcessMode     Process mode.
  @param[in, out]      Fragment        Only one Fragment returned after the Message is
                                       processed successfully.
                                       If ProcessMode is EfiTlsEncrypt, the fragment contain the TLS
                                       header and cipher text TLS APP payload.
                                       If ProcessMode is EfiTlsDecrypt, the fragment contain the TLS
                                       header and plain text TLS APP payload.

  @retval EFI_SUCCESS          Message is processed successfully.
  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @retval Others               Other errors as indicated.

**/
EFI_STATUS
EFIAPI
TlsProcessMessage (
  IN     HTTP_PROTOCOL       *HttpInstance,
  IN     UINT8               *Message,
  IN     UINTN               MessageSize,
  IN     EFI_TLS_CRYPT_MODE  ProcessMode,
  IN OUT NET_FRAGMENT        *Fragment
  )
{
  EFI_STATUS             Status;
  UINT8                  *Buffer;
  UINT32                 BufferSize;
  UINT32                 BytesCopied;
  EFI_TLS_FRAGMENT_DATA  *FragmentTable;
  UINT32                 FragmentCount;
  EFI_TLS_FRAGMENT_DATA  *OriginalFragmentTable;
  UINTN                  Index;

  Status                = EFI_SUCCESS;
  Buffer                = NULL;
  BufferSize            = 0;
  BytesCopied           = 0;
  FragmentTable         = NULL;
  OriginalFragmentTable = NULL;

  //
  // Rebuild fragment table from BufferIn.
  //
  FragmentCount = 1;
  FragmentTable = AllocateZeroPool (FragmentCount * sizeof (EFI_TLS_FRAGMENT_DATA));
  if (FragmentTable == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  FragmentTable->FragmentLength = (UINT32)MessageSize;
  FragmentTable->FragmentBuffer = Message;

  //
  // Record the original FragmentTable.
  //
  OriginalFragmentTable = FragmentTable;

  //
  // Process the Message.
  //
  Status = HttpInstance->Tls->ProcessPacket (
                                HttpInstance->Tls,
                                &FragmentTable,
                                &FragmentCount,
                                ProcessMode
                                );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Calculate the size according to FragmentTable.
  //
  for (Index = 0; Index < FragmentCount; Index++) {
    BufferSize += FragmentTable[Index].FragmentLength;
  }

  //
  // Allocate buffer for processed data.
  //
  Buffer = AllocateZeroPool (BufferSize);
  if (Buffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Copy the new FragmentTable buffer into Buffer.
  //
  for (Index = 0; Index < FragmentCount; Index++) {
    CopyMem (
      (Buffer + BytesCopied),
      FragmentTable[Index].FragmentBuffer,
      FragmentTable[Index].FragmentLength
      );
    BytesCopied += FragmentTable[Index].FragmentLength;

    //
    // Free the FragmentBuffer since it has been copied.
    //
    FreePool (FragmentTable[Index].FragmentBuffer);
  }

  Fragment->Len  = BufferSize;
  Fragment->Bulk = Buffer;

ON_EXIT:

  if (OriginalFragmentTable != NULL) {
    if ( FragmentTable == OriginalFragmentTable) {
      FragmentTable = NULL;
    }

    FreePool (OriginalFragmentTable);
    OriginalFragmentTable = NULL;
  }

  //
  // Caller has the responsibility to free the FragmentTable.
  //
  if (FragmentTable != NULL) {
    FreePool (FragmentTable);
    FragmentTable = NULL;
  }

  return Status;
}

/**
  Receive one fragment decrypted from one TLS record.

  @param[in]           HttpInstance    Pointer to HTTP_PROTOCOL structure.
  @param[in, out]      Fragment        The received Fragment.
  @param[in]           Timeout         The time to wait for connection done.

  @retval EFI_SUCCESS          One fragment is received.
  @retval EFI_OUT_OF_RESOURCES Can't allocate memory resources.
  @retval EFI_ABORTED          Something wrong decryption the message.
  @retval Others               Other errors as indicated.

**/
EFI_STATUS
EFIAPI
HttpsReceive (
  IN     HTTP_PROTOCOL  *HttpInstance,
  IN OUT NET_FRAGMENT   *Fragment,
  IN     EFI_EVENT      Timeout
  )
{
  EFI_STATUS         Status;
  NET_BUF            *Pdu;
  TLS_RECORD_HEADER  RecordHeader;
  UINT8              *BufferIn;
  UINTN              BufferInSize;
  NET_FRAGMENT       TempFragment;
  UINT8              *BufferOut;
  UINTN              BufferOutSize;
  NET_BUF            *PacketOut;
  UINT8              *DataOut;
  UINT8              *GetSessionDataBuffer;
  UINTN              GetSessionDataBufferSize;

  Status                   = EFI_SUCCESS;
  Pdu                      = NULL;
  BufferIn                 = NULL;
  BufferInSize             = 0;
  BufferOut                = NULL;
  BufferOutSize            = 0;
  PacketOut                = NULL;
  DataOut                  = NULL;
  GetSessionDataBuffer     = NULL;
  GetSessionDataBufferSize = 0;

  //
  // Receive only one TLS record
  //
  Status = TlsReceiveOnePdu (HttpInstance, &Pdu, Timeout);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BufferInSize = Pdu->TotalSize;
  BufferIn     = AllocateZeroPool (BufferInSize);
  if (BufferIn == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    NetbufFree (Pdu);
    return Status;
  }

  NetbufCopy (Pdu, 0, (UINT32)BufferInSize, BufferIn);

  NetbufFree (Pdu);

  //
  // Handle Receive data.
  //
  RecordHeader = *(TLS_RECORD_HEADER *)BufferIn;

  if ((RecordHeader.ContentType == TlsContentTypeApplicationData) &&
      (RecordHeader.Version.Major == 0x03) &&
      ((RecordHeader.Version.Minor == TLS10_PROTOCOL_VERSION_MINOR) ||
       (RecordHeader.Version.Minor == TLS11_PROTOCOL_VERSION_MINOR) ||
       (RecordHeader.Version.Minor == TLS12_PROTOCOL_VERSION_MINOR))
      )
  {
    //
    // Decrypt Packet.
    //
    Status = TlsProcessMessage (
               HttpInstance,
               BufferIn,
               BufferInSize,
               EfiTlsDecrypt,
               &TempFragment
               );

    FreePool (BufferIn);

    if (EFI_ERROR (Status)) {
      if (Status == EFI_ABORTED) {
        //
        // Something wrong decryption the message.
        // BuildResponsePacket() will be called to generate Error Alert message and send it out.
        //
        BufferOutSize = DEF_BUF_LEN;
        BufferOut     = AllocateZeroPool (BufferOutSize);
        if (BufferOut == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          return Status;
        }

        Status = HttpInstance->Tls->BuildResponsePacket (
                                      HttpInstance->Tls,
                                      NULL,
                                      0,
                                      BufferOut,
                                      &BufferOutSize
                                      );
        if (Status == EFI_BUFFER_TOO_SMALL) {
          FreePool (BufferOut);
          BufferOut = AllocateZeroPool (BufferOutSize);
          if (BufferOut == NULL) {
            Status = EFI_OUT_OF_RESOURCES;
            return Status;
          }

          Status = HttpInstance->Tls->BuildResponsePacket (
                                        HttpInstance->Tls,
                                        NULL,
                                        0,
                                        BufferOut,
                                        &BufferOutSize
                                        );
        }

        if (EFI_ERROR (Status)) {
          FreePool (BufferOut);
          return Status;
        }

        if (BufferOutSize != 0) {
          PacketOut = NetbufAlloc ((UINT32)BufferOutSize);
          if (PacketOut == NULL) {
            FreePool (BufferOut);
            return EFI_OUT_OF_RESOURCES;
          }

          DataOut = NetbufAllocSpace (PacketOut, (UINT32)BufferOutSize, NET_BUF_TAIL);
          if (DataOut == NULL) {
            FreePool (BufferOut);
            NetbufFree (PacketOut);
            return EFI_OUT_OF_RESOURCES;
          }

          CopyMem (DataOut, BufferOut, BufferOutSize);

          Status = TlsCommonTransmit (HttpInstance, PacketOut);

          NetbufFree (PacketOut);
        }

        FreePool (BufferOut);

        if (EFI_ERROR (Status)) {
          return Status;
        }

        return EFI_ABORTED;
      }

      return Status;
    }

    //
    // Parsing buffer.
    //
    ASSERT (((TLS_RECORD_HEADER *)(TempFragment.Bulk))->ContentType == TlsContentTypeApplicationData);

    BufferInSize = ((TLS_RECORD_HEADER *)(TempFragment.Bulk))->Length;
    BufferIn     = AllocateZeroPool (BufferInSize);
    if (BufferIn == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      return Status;
    }

    CopyMem (BufferIn, TempFragment.Bulk + TLS_RECORD_HEADER_LENGTH, BufferInSize);

    //
    // Free the buffer in TempFragment.
    //
    FreePool (TempFragment.Bulk);
  } else if ((RecordHeader.ContentType == TlsContentTypeAlert) &&
             (RecordHeader.Version.Major == 0x03) &&
             ((RecordHeader.Version.Minor == TLS10_PROTOCOL_VERSION_MINOR) ||
              (RecordHeader.Version.Minor == TLS11_PROTOCOL_VERSION_MINOR) ||
              (RecordHeader.Version.Minor == TLS12_PROTOCOL_VERSION_MINOR))
             )
  {
    BufferOutSize = DEF_BUF_LEN;
    BufferOut     = AllocateZeroPool (BufferOutSize);
    if (BufferOut == NULL) {
      FreePool (BufferIn);
      Status = EFI_OUT_OF_RESOURCES;
      return Status;
    }

    Status = HttpInstance->Tls->BuildResponsePacket (
                                  HttpInstance->Tls,
                                  BufferIn,
                                  BufferInSize,
                                  BufferOut,
                                  &BufferOutSize
                                  );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      FreePool (BufferOut);
      BufferOut = AllocateZeroPool (BufferOutSize);
      if (BufferOut == NULL) {
        FreePool (BufferIn);
        Status = EFI_OUT_OF_RESOURCES;
        return Status;
      }

      Status = HttpInstance->Tls->BuildResponsePacket (
                                    HttpInstance->Tls,
                                    BufferIn,
                                    BufferInSize,
                                    BufferOut,
                                    &BufferOutSize
                                    );
    }

    FreePool (BufferIn);

    if (EFI_ERROR (Status)) {
      FreePool (BufferOut);
      return Status;
    }

    if (BufferOutSize != 0) {
      PacketOut = NetbufAlloc ((UINT32)BufferOutSize);
      if (PacketOut == NULL) {
        FreePool (BufferOut);
        return EFI_OUT_OF_RESOURCES;
      }

      DataOut = NetbufAllocSpace (PacketOut, (UINT32)BufferOutSize, NET_BUF_TAIL);
      if (DataOut == NULL) {
        FreePool (BufferOut);
        NetbufFree (PacketOut);
        return EFI_OUT_OF_RESOURCES;
      }

      CopyMem (DataOut, BufferOut, BufferOutSize);

      Status = TlsCommonTransmit (HttpInstance, PacketOut);

      NetbufFree (PacketOut);
    }

    FreePool (BufferOut);

    //
    // Get the session state.
    //
    GetSessionDataBufferSize = DEF_BUF_LEN;
    GetSessionDataBuffer     = AllocateZeroPool (GetSessionDataBufferSize);
    if (GetSessionDataBuffer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      return Status;
    }

    Status = HttpInstance->Tls->GetSessionData (
                                  HttpInstance->Tls,
                                  EfiTlsSessionState,
                                  GetSessionDataBuffer,
                                  &GetSessionDataBufferSize
                                  );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      FreePool (GetSessionDataBuffer);
      GetSessionDataBuffer = AllocateZeroPool (GetSessionDataBufferSize);
      if (GetSessionDataBuffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        return Status;
      }

      Status = HttpInstance->Tls->GetSessionData (
                                    HttpInstance->Tls,
                                    EfiTlsSessionState,
                                    GetSessionDataBuffer,
                                    &GetSessionDataBufferSize
                                    );
    }

    if (EFI_ERROR (Status)) {
      FreePool (GetSessionDataBuffer);
      return Status;
    }

    ASSERT (GetSessionDataBufferSize == sizeof (EFI_TLS_SESSION_STATE));
    HttpInstance->TlsSessionState = *(EFI_TLS_SESSION_STATE *)GetSessionDataBuffer;

    FreePool (GetSessionDataBuffer);

    if (HttpInstance->TlsSessionState == EfiTlsSessionError) {
      DEBUG ((DEBUG_ERROR, "TLS Session State Error!\n"));
      return EFI_ABORTED;
    }

    BufferIn     = NULL;
    BufferInSize = 0;
  }

  Fragment->Bulk = BufferIn;
  Fragment->Len  = (UINT32)BufferInSize;

  return Status;
}
