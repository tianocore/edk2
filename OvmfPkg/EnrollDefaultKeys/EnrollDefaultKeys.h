/** @file
  Type definitions and object declarations for the EnrollDefaultKeys
  application.

  Copyright (C) 2014-2019, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ENROLL_DEFAULT_KEYS_H_
#define ENROLL_DEFAULT_KEYS_H_

#include <Uefi/UefiBaseType.h>

//
// Convenience structure types for constructing "signature lists" for
// authenticated UEFI variables.
//
// The most important thing about the variable payload is that it is a list of
// lists, where the element size of any given *inner* list is constant.
//
// Since X509 certificates vary in size, each of our *inner* lists will contain
// one element only (one X.509 certificate). This is explicitly mentioned in
// the UEFI specification, in "28.4.1 Signature Database", in a Note.
//
// The list structure looks as follows:
//
// struct EFI_VARIABLE_AUTHENTICATION_2 {                           |
//   struct EFI_TIME {                                              |
//     UINT16 Year;                                                 |
//     UINT8  Month;                                                |
//     UINT8  Day;                                                  |
//     UINT8  Hour;                                                 |
//     UINT8  Minute;                                               |
//     UINT8  Second;                                               |
//     UINT8  Pad1;                                                 |
//     UINT32 Nanosecond;                                           |
//     INT16  TimeZone;                                             |
//     UINT8  Daylight;                                             |
//     UINT8  Pad2;                                                 |
//   } TimeStamp;                                                   |
//                                                                  |
//   struct WIN_CERTIFICATE_UEFI_GUID {                           | |
//     struct WIN_CERTIFICATE {                                   | |
//       UINT32 dwLength; ----------------------------------------+ |
//       UINT16 wRevision;                                        | |
//       UINT16 wCertificateType;                                 | |
//     } Hdr;                                                     | +- DataSize
//                                                                | |
//     EFI_GUID CertType;                                         | |
//     UINT8    CertData[1] = { <--- "struct hack"                | |
//       struct EFI_SIGNATURE_LIST {                            | | |
//         EFI_GUID SignatureType;                              | | |
//         UINT32   SignatureListSize; -------------------------+ | |
//         UINT32   SignatureHeaderSize;                        | | |
//         UINT32   SignatureSize; ---------------------------+ | | |
//         UINT8    SignatureHeader[SignatureHeaderSize];     | | | |
//                                                            v | | |
//         struct EFI_SIGNATURE_DATA {                        | | | |
//           EFI_GUID SignatureOwner;                         | | | |
//           UINT8    SignatureData[1] = { <--- "struct hack" | | | |
//             X.509 payload                                  | | | |
//           }                                                | | | |
//         } Signatures[];                                      | | |
//       } SigLists[];                                            | |
//     };                                                         | |
//   } AuthInfo;                                                  | |
// };                                                               |
//
// Given that the "struct hack" invokes undefined behavior (which is why C99
// introduced the flexible array member), and because subtracting those pesky
// sizes of 1 is annoying, and because the format is fully specified in the
// UEFI specification, we'll introduce two matching convenience structures that
// are customized for our X.509 purposes.
//
#pragma pack (1)
typedef struct {
  EFI_TIME TimeStamp;

  //
  // dwLength covers data below
  //
  UINT32   dwLength;
  UINT16   wRevision;
  UINT16   wCertificateType;
  EFI_GUID CertType;
} SINGLE_HEADER;

typedef struct {
  //
  // SignatureListSize covers data below
  //
  EFI_GUID SignatureType;
  UINT32   SignatureListSize;
  UINT32   SignatureHeaderSize; // constant 0
  UINT32   SignatureSize;

  //
  // SignatureSize covers data below
  //
  EFI_GUID SignatureOwner;

  //
  // X.509 certificate follows
  //
} REPEATING_HEADER;
#pragma pack ()


//
// A structure that collects the values of UEFI variables related to Secure
// Boot.
//
typedef struct {
  UINT8 SetupMode;
  UINT8 SecureBoot;
  UINT8 SecureBootEnable;
  UINT8 CustomMode;
  UINT8 VendorKeys;
} SETTINGS;


//
// Refer to "AuthData.c" for details on the following objects.
//
extern CONST UINT8 mMicrosoftKek[];
extern CONST UINTN mSizeOfMicrosoftKek;

extern CONST UINT8 mMicrosoftPca[];
extern CONST UINTN mSizeOfMicrosoftPca;

extern CONST UINT8 mMicrosoftUefiCa[];
extern CONST UINTN mSizeOfMicrosoftUefiCa;

extern CONST UINT8 mSha256OfDevNull[];
extern CONST UINTN mSizeOfSha256OfDevNull;

#endif /* ENROLL_DEFAULT_KEYS_H_ */
