/** @file
  Platform Key, Key Exchange Key, and Image signature database are defined 
  for the signed image validation.

  Copyright (c) 2009 - 2010, Intel Corporation
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  @par Revision Reference:
  GUIDs defined in UEFI 2.2 spec.
**/

#ifndef __IMAGE_AUTHTICATION_H__
#define __IMAGE_AUTHTICATION_H__

#include <Guid/GlobalVariable.h>

#define EFI_IMAGE_SECURITY_DATABASE_GUID \
  { \
    0xd719b2cb, 0x3d3a, 0x4596, { 0xa3, 0xbc, 0xda, 0xd0, 0xe, 0x67, 0x65, 0x6f } \
  }

///
/// Varialbe name with guid EFI_IMAGE_SECURITY_DATABASE_GUID 
/// for the authorized signature database.
///
#define EFI_IMAGE_SECURITY_DATABASE       L"db"
///
/// Varialbe name with guid EFI_IMAGE_SECURITY_DATABASE_GUID 
/// for the forbidden signature database.
///
#define EFI_IMAGE_SECURITY_DATABASE1      L"dbx"

#define SETUP_MODE                        1
#define USER_MODE                         0
///
/// Globally "SetupMode" variable to specify whether the system is currently operating 
/// in setup mode (1) or not (0). All other values are reserved.
///
#define EFI_SETUP_MODE_NAME               L"SetupMode"
///
/// Globally "PK" variable for the Platform Key Signature Database.
///
#define EFI_PLATFORM_KEY_NAME             L"PK"
///
/// Globally "KEK" variable for the Key Exchange Key Signature Database.
///
#define EFI_KEY_EXCHANGE_KEY_NAME         L"KEK"
///
/// Globally "SignatureSupport" variable returns an array of GUIDs, 
/// with each GUID representing a type of signature which the platform 
/// firmware supports for images and other data.
///
#define EFI_SIGNATURE_SUPPORT_NAME        L"SignatureSupport"

//***********************************************************************
// Signature Database
//***********************************************************************
///
/// The format of a signature database. 
///
#pragma pack(1)

typedef struct {
  ///
  /// An identifier which identifies the agent which added the signature to the list.
  ///
  EFI_GUID          SignatureOwner;
  ///
  /// The format of the signature is defined by the SignatureType.
  ///
  UINT8             SignatureData[1];
} EFI_SIGNATURE_DATA;

typedef struct {
  ///
  /// Type of the signature. GUID signature types are defined in below.
  ///
  EFI_GUID            SignatureType;
  ///
  /// Total size of the signature list, including this header.
  ///
  UINT32              SignatureListSize;
  ///
  /// Size of the signature header which precedes the array of signatures.
  ///
  UINT32              SignatureHeaderSize;
  ///
  /// Size of each signature.
  ///
  UINT32              SignatureSize; 
  ///
  /// Header before the array of signatures. The format of this header is specified 
  /// by the SignatureType.
  /// UINT8           SignatureHeader[SignatureHeaderSize];
  ///
  /// An array of signatures. Each signature is SignatureSize bytes in length. 
  /// EFI_SIGNATURE_DATA Signatures[][SignatureSize];
  ///
} EFI_SIGNATURE_LIST;

#pragma pack()

///
/// This identifies a signature containing a SHA-256 hash. The SignatureHeader size should 
/// always be 0.  The SignatureSize should always be 32 bytes.
///
#define EFI_CERT_SHA256_GUID \
  { \
    0xc1c41626, 0x504c, 0x4092, {0xac, 0xa9, 0x41, 0xf9, 0x36, 0x93, 0x43, 0x28} \
  }

///
/// This identifies a signature containing an RSA-2048 key. The SignatureHeader size should 
/// always be 0. The SignatureSize should always be 256 bytes.
///
#define EFI_CERT_RSA2048_GUID \
  { \
    0x3c5766e8, 0x269c, 0x4e34, {0xaa, 0x14, 0xed, 0x77, 0x6e, 0x85, 0xb3, 0xb6} \
  }

///
/// This identifies a signature containing a RSA-2048 signature of a SHA-256 hash. The 
/// SignatureHeader size should always be 0. The SignatureSize should always be 256 bytes.
///
#define EFI_CERT_RSA2048_SHA256_GUID \
  { \
    0xe2b36190, 0x879b, 0x4a3d, {0xad, 0x8d, 0xf2, 0xe7, 0xbb, 0xa3, 0x27, 0x84} \
  }

///
/// This identifies a signature containing a SHA-1 hash.  The SignatureHeader size should always
/// be 0. The SignatureSize should always be 20 bytes
///
#define EFI_CERT_SHA1_GUID \
  { \
    0x826ca512, 0xcf10, 0x4ac9, {0xb1, 0x87, 0xbe, 0x1, 0x49, 0x66, 0x31, 0xbd} \
  }

///
/// This identifies a signature containing a RSA-2048 signature of a SHA-1 hash. The 
/// SignatureHeader size should always be 0. The SignatureSize should always be 256 bytes.
///
#define EFI_CERT_RSA2048_SHA1_GUID \
  { \
    0x67f8444f, 0x8743, 0x48f1, {0xa3, 0x28, 0x1e, 0xaa, 0xb8, 0x73, 0x60, 0x80} \
  }

///
/// This identifies a signature based on an X.509 certificate. If the signature is an X.509 certificate then 
/// verification of the signature of an image should validate the public key certificate in the image using 
/// certificate path verification, up to this X.509 certificate as a trusted root.
///
#define EFI_CERT_X509 \
  { \
    0xa5c059a1, 0x94e4, 0x4aa7, {0x87, 0xb5, 0xab, 0x15, 0x5c, 0x2b, 0xf0, 0x72} \
  }

//***********************************************************************
// Image Execution Information Table Definition
//***********************************************************************
typedef UINT32 EFI_IMAGE_EXECUTION_ACTION;

#define EFI_IMAGE_EXECUTION_AUTHENTICATION      0x00000007 
#define EFI_IMAGE_EXECUTION_AUTH_UNTESTED       0x00000000
#define EFI_IMAGE_EXECUTION_AUTH_SIG_FAILED     0x00000001
#define EFI_IMAGE_EXECUTION_AUTH_SIG_PASSED     0x00000002
#define EFI_IMAGE_EXECUTION_AUTH_SIG_NOT_FOUND  0x00000003
#define EFI_IMAGE_EXECUTION_AUTH_SIG_FOUND      0x00000004
#define EFI_IMAGE_EXECUTION_POLICY_FAILED       0x00000005
#define EFI_IMAGE_EXECUTION_INITIALIZED         0x00000008

//
// EFI_IMAGE_EXECUTION_INFO is added to EFI System Configuration Table 
// and assigned the GUID EFI_IMAGE_SECURITY_DATABASE_GUID.
//
typedef struct {
  ///
  /// Describes the action taken by the firmware regarding this image.
  ///
  EFI_IMAGE_EXECUTION_ACTION    Action;
  ///
  /// Size of all of the entire structure.
  ///
  UINT32                        InfoSize;
  ///
  /// If this image was a UEFI device driver (for option ROM, for example) this is the 
  /// null-terminated, user-friendly name for the device. If the image was for an application, 
  /// then this is the name of the application. If this cannot be determined, then a simple 
  /// NULL character should be put in this position.
  /// CHAR16                    Name[];
  ///

  ///
  /// For device drivers, this is the device path of the device for which this device driver 
  /// was intended. In some cases, the driver itself may be stored as part of the system 
  /// firmware, but this field should record the device's path, not the firmware path. For 
  /// applications, this is the device path of the application. If this cannot be determined, 
  /// a simple end-of-path device node should be put in this position.
  /// EFI_DEVICE_PATH_PROTOCOL  DevicePath;
  ///

  ///
  /// Zero or more image signatures. If the image contained no signatures, 
  /// then this field is empty.
  ///
  EFI_SIGNATURE_LIST            Signature;
} EFI_IMAGE_EXECUTION_INFO;


typedef struct {
  ///
  /// Number of EFI_IMAGE_EXECUTION_INFO structures.
  ///
  UINTN                     NumberOfImages; 
  ///
  /// Number of image instances of EFI_IMAGE_EXECUTION_INFO structures.
  ///
  // EFI_IMAGE_EXECUTION_INFO  InformationInfo[] 
} EFI_IMAGE_EXECUTION_INFO_TABLE;

extern EFI_GUID gEfiImageSecurityDatabaseGuid;
extern EFI_GUID gEfiCertSha256Guid;
extern EFI_GUID gEfiCertRsa2048Guid;      
extern EFI_GUID gEfiCertRsa2048Sha256Guid;
extern EFI_GUID gEfiCertSha1Guid;
extern EFI_GUID gEfiCertRsa2048Sha1Guid;
extern EFI_GUID gEfiCertX509Guid;

#endif
