/** @file
  This file defines the HttpTlsCipherList variable for HTTPS to configure Tls Cipher List.

Copyright (c) 2018 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __HTTP_TLS_CIPHER_LIST_H__
#define __HTTP_TLS_CIPHER_LIST_H__

//
// Private Variable for HTTPS to configure Tls Cipher List.
// The valid contents of variable must follow the TLS CipherList format defined in RFC 5246.
// The valid length of variable must be an integral multiple of 2.
// For example, if below cipher suites are preferred:
//    CipherSuite TLS_RSA_WITH_AES_128_CBC_SHA256 = {0x00,0x3C}
//   CipherSuite TLS_RSA_WITH_AES_256_CBC_SHA256 = {0x00,0x3D}
// Then, the contents of variable should be:
//   {0x00,0x3C,0x00,0x3D}
//
#define EDKII_HTTP_TLS_CIPHER_LIST_GUID \
  { \
    0x46ddb415, 0x5244, 0x49c7, { 0x93, 0x74, 0xf0, 0xe2, 0x98, 0xe7, 0xd3, 0x86 } \
  }

#define EDKII_HTTP_TLS_CIPHER_LIST_VARIABLE  L"HttpTlsCipherList"

extern EFI_GUID  gEdkiiHttpTlsCipherListGuid;

#endif
