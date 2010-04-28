/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Bis.h

Abstract:

  This file defines the BIS protocol.

--*/

#ifndef _BIS_H_
#define _BIS_H_

#include <EfiSpec.h>

//
// Basic types
//
typedef VOID    *BIS_APPLICATION_HANDLE;
typedef UINT16  BIS_ALG_ID;
typedef UINT32  BIS_CERT_ID;

//
// EFI_BIS_DATA type.
//
// EFI_BIS_DATA instances obtained from BIS must be freed by calling Free( ).
//
typedef struct _EFI_BIS_DATA {
  UINT32  Length; // Length of Data in 8 bit bytes.
  UINT8   *Data;  // 32 Bit Flat Address of data.
} EFI_BIS_DATA;

//
// EFI_BIS_VERSION type.
//
typedef struct _EFI_BIS_VERSION {
  UINT32  Major;  // BIS Interface version number.
  UINT32  Minor;  // Build number.
} EFI_BIS_VERSION;

//
// ----------------------------------------------------//
// Use these values to initialize EFI_BIS_VERSION.Major
// and to interpret results of Initialize.
// ----------------------------------------------------//
//
#define BIS_CURRENT_VERSION_MAJOR BIS_VERSION_1
#define BIS_VERSION_1             1

//
// EFI_BIS_SIGNATURE_INFO type.
//
typedef struct _EFI_BIS_SIGNATURE_INFO {
  BIS_CERT_ID CertificateID;  // Truncated hash of platform Boot Object
  //  authorization certificate.
  //
  BIS_ALG_ID  AlgorithmID;  // A signature algorithm number.
  UINT16      KeyLength;    // Length of alg. keys in bits.
} EFI_BIS_SIGNATURE_INFO;

//
// Currently defined values for EFI_BIS_SIGNATURE_INFO.AlgorithmID.
// The exact numeric values come from
//    "Common Data Security Architecture (CDSA) Specification".
//
#define BIS_ALG_DSA     (41)  // CSSM_ALGID_DSA
#define BIS_ALG_RSA_MD5 (42)  // CSSM_ALGID_MD5_WITH_RSA
// Currently defined values for EFI_BIS_SIGNATURE_INFO.CertificateId.
//
#define BIS_CERT_ID_DSA     BIS_ALG_DSA     // CSSM_ALGID_DSA
#define BIS_CERT_ID_RSA_MD5 BIS_ALG_RSA_MD5 // CSSM_ALGID_MD5_WITH_RSA
// The  following  is a mask value that gets applied to the truncated hash of a
// platform  Boot Object Authorization Certificate to create the certificateID.
// A certificateID must not have any bits set to the value 1 other than bits in
// this mask.
//
#define BIS_CERT_ID_MASK  (0xFF7F7FFF)

//
// Macros for dealing with the EFI_BIS_DATA object obtained
// from BIS_GetSignatureInfo()
// BIS_GET_SIGINFO_COUNT - tells how many EFI_BIS_SIGNATURE_INFO
//  elements are contained in a EFI_BIS_DATA struct pointed to
//  by the provided EFI_BIS_DATA*.
//
#define BIS_GET_SIGINFO_COUNT(BisDataPtr) ((BisDataPtr)->Length / sizeof (EFI_BIS_SIGNATURE_INFO))

//
// BIS_GET_SIGINFO_ARRAY - produces a EFI_BIS_SIGNATURE_INFO*
//  from a given EFI_BIS_DATA*.
//
#define BIS_GET_SIGINFO_ARRAY(BisDataPtr) ((EFI_BIS_SIGNATURE_INFO *) (BisDataPtr)->Data)

//
// Binary Value of "X-Intel-BIS-ParameterSet" Attribute.
// (Value is Base64 encoded in actual signed manifest).
// {EDD35E31-07B9-11d2-83A3-00A0C91FADCF}
//
#define BOOT_OBJECT_AUTHORIZATION_PARMSET_GUIDVALUE \
  { \
    0xedd35e31, 0x7b9, 0x11d2, \
    { \
      0x83, 0xa3, 0x0, 0xa0, 0xc9, 0x1f, 0xad, 0xcf \
    } \
  }

//
// -----------------------------------//
//  EFI_BIS_PROTOCOL
// -----------------------------------//
//
#define EFI_BIS_PROTOCOL_GUID \
  { \
    0x0b64aab0, 0x5429, 0x11d4, {0x98, 0x16, 0x00, 0xa0, 0xc9, 0x1f, 0xad, 0xcf} \
  }

typedef struct _EFI_BIS_PROTOCOL  EFI_BIS_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *EFI_BIS_INITIALIZE) (
  IN     EFI_BIS_PROTOCOL        * This,              // this
  OUT    BIS_APPLICATION_HANDLE  * AppHandle,         // Application handle.
  IN OUT EFI_BIS_VERSION         * InterfaceVersion,  // ver needed/available.
  IN     EFI_BIS_DATA            * TargetAddress      // Address of BIS platform.
  );

typedef
EFI_STATUS
(EFIAPI *EFI_BIS_FREE) (
  IN BIS_APPLICATION_HANDLE  AppHandle,               // From Initialize( ).
  IN EFI_BIS_DATA            * ToFree                 // EFI_BIS_DATA being freed.
  );

typedef
EFI_STATUS
(EFIAPI *EFI_BIS_SHUTDOWN) (
  IN BIS_APPLICATION_HANDLE  AppHandle                // From Initialize( ).
  );

typedef
EFI_STATUS
(EFIAPI *EFI_BIS_GET_BOOT_OBJECT_AUTHORIZATION_CERTIFICATE) (
  IN  BIS_APPLICATION_HANDLE  AppHandle,              // From Initialize( ).
  OUT EFI_BIS_DATA            **Certificate           // Pointer to certificate.
  );

typedef
EFI_STATUS
(EFIAPI *EFI_BIS_VERIFY_BOOT_OBJECT) (
  IN  BIS_APPLICATION_HANDLE AppHandle,               // From Initialize( ).
  IN  EFI_BIS_DATA           * Credentials,           // Verification signed manifest.
  IN  EFI_BIS_DATA           * DataObject,            // Boot object to verify.
  OUT BOOLEAN                *IsVerified              // Result of verifcation.
  );

typedef
EFI_STATUS
(EFIAPI *EFI_BIS_GET_BOOT_OBJECT_AUTHORIZATION_CHECKFLAG) (
  IN  BIS_APPLICATION_HANDLE  AppHandle,              // From Initialize( ).
  OUT BOOLEAN                 *CheckIsRequired        // Value of check flag.
  );

typedef
EFI_STATUS
(EFIAPI *EFI_BIS_GET_BOOT_OBJECT_AUTHORIZATION_UPDATE_TOKEN) (
  IN  BIS_APPLICATION_HANDLE  AppHandle,              // From Initialize( ).
  OUT EFI_BIS_DATA            **UpdateToken           // Value of update token.
  );

typedef
EFI_STATUS
(EFIAPI *EFI_BIS_UPDATE_BOOT_OBJECT_AUTHORIZATION) (
  IN  BIS_APPLICATION_HANDLE AppHandle,               // From Initialize( ).
  IN  EFI_BIS_DATA           * RequestCredential,     // Update Request Manifest.
  OUT EFI_BIS_DATA           **NewUpdateToken         // Next update token.
  );

typedef
EFI_STATUS
(EFIAPI *EFI_BIS_VERIFY_OBJECT_WITH_CREDENTIAL) (
  IN  BIS_APPLICATION_HANDLE AppHandle,               //  From Initialize( ).
  IN  EFI_BIS_DATA           * Credentials,           //  Verification signed manifest.
  IN  EFI_BIS_DATA           * DataObject,            //  Boot object to verify.
  IN  EFI_BIS_DATA           * SectionName,           //  Name of credential section to use.
  IN  EFI_BIS_DATA           * AuthorityCertificate,  // Certificate for credentials.
  OUT BOOLEAN                *IsVerified              // Result of verifcation.
  );

typedef
EFI_STATUS
(EFIAPI *EFI_BIS_GET_SIGNATURE_INFO) (
  IN  BIS_APPLICATION_HANDLE  AppHandle,              //  From Initialize( ).
  OUT EFI_BIS_DATA            **SignatureInfo         // Signature info struct.
  );

#define EFI_BIS_PROTOCOL_REVISION 0x00010000

struct _EFI_BIS_PROTOCOL {
  //
  // member vars
  //
  UINT64                                              Revision;

  //
  // methods
  //
  EFI_BIS_INITIALIZE                                  Initialize;
  EFI_BIS_SHUTDOWN                                    Shutdown;
  EFI_BIS_FREE                                        Free;
  EFI_BIS_GET_BOOT_OBJECT_AUTHORIZATION_CERTIFICATE   GetBootObjectAuthorizationCertificate;
  EFI_BIS_GET_BOOT_OBJECT_AUTHORIZATION_CHECKFLAG     GetBootObjectAuthorizationCheckFlag;
  EFI_BIS_GET_BOOT_OBJECT_AUTHORIZATION_UPDATE_TOKEN  GetBootObjectAuthorizationUpdateToken;
  EFI_BIS_GET_SIGNATURE_INFO                          GetSignatureInfo;
  EFI_BIS_UPDATE_BOOT_OBJECT_AUTHORIZATION            UpdateBootObjectAuthorization;
  EFI_BIS_VERIFY_BOOT_OBJECT                          VerifyBootObject;
  EFI_BIS_VERIFY_OBJECT_WITH_CREDENTIAL               VerifyObjectWithCredential;
};

extern EFI_GUID gEfiBisProtocolGuid;

#endif
