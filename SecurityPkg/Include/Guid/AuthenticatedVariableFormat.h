/** @file
  The variable data structures are related to EDKII-specific 
  implementation of UEFI authenticated variables.
  AuthenticatedVariableFormat.h defines variable data headers 
  and variable storage region headers.

Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __AUTHENTICATED_VARIABLE_FORMAT_H__
#define __AUTHENTICATED_VARIABLE_FORMAT_H__

#define EFI_AUTHENTICATED_VARIABLE_GUID \
  { 0xaaf32c78, 0x947b, 0x439a, { 0xa1, 0x80, 0x2e, 0x14, 0x4e, 0xc3, 0x77, 0x92 } }

#define EFI_SECURE_BOOT_ENABLE_DISABLE \
  { 0xf0a30bc7, 0xaf08, 0x4556, { 0x99, 0xc4, 0x0, 0x10, 0x9, 0xc9, 0x3a, 0x44 } }


extern EFI_GUID gEfiAuthenticatedVariableGuid;
extern EFI_GUID gEfiSecureBootEnableDisableGuid;

///
/// "SecureBootEnable" variable for the Secure boot feature enable/disable.
///
#define EFI_SECURE_BOOT_ENABLE_NAME      L"SecureBootEnable"
#define SECURE_BOOT_ENABLE               1
#define SECURE_BOOT_DISABLE              0

extern EFI_GUID gEfiCustomModeEnableGuid;

///
///  "CustomMode" variable for two Secure Boot modes feature: "Custom" and "Standard".
///  Standard Secure Boot mode is the default mode as UEFI Spec's description.
///  Custom Secure Boot mode allows for more flexibility as specified in the following:
///    Can enroll or delete PK without existing PK's private key.
///    Can enroll or delete KEK without existing PK's private key.
///    Can enroll or delete signature from DB/DBX without KEK's private key.
///
#define EFI_CUSTOM_MODE_NAME          L"CustomMode"
#define CUSTOM_SECURE_BOOT_MODE       1
#define STANDARD_SECURE_BOOT_MODE     0

///
/// "certdb" variable stores the signer's certificates for non PK/KEK/DB/DBX
/// variables with EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS set.
/// 
///
#define EFI_CERT_DB_NAME L"certdb"

extern EFI_GUID gEfiCertDbGuid;

///
/// Alignment of variable name and data, according to the architecture:
/// * For IA-32 and Intel(R) 64 architectures: 1.
/// * For IA-64 architecture: 8.
///
#if defined (MDE_CPU_IPF)
#define ALIGNMENT         8
#else
#define ALIGNMENT         1
#endif

//
// GET_PAD_SIZE calculates the miminal pad bytes needed to make the current pad size satisfy the alignment requirement.
//
#if (ALIGNMENT == 1)
#define GET_PAD_SIZE(a) (0)
#else
#define GET_PAD_SIZE(a) (((~a) + 1) & (ALIGNMENT - 1))
#endif

///
/// Alignment of Variable Data Header in Variable Store region.
///
#define HEADER_ALIGNMENT  4
#define HEADER_ALIGN(Header)  (((UINTN) (Header) + HEADER_ALIGNMENT - 1) & (~(HEADER_ALIGNMENT - 1)))

///
/// Status of Variable Store Region.
///
typedef enum {
  EfiRaw,
  EfiValid,
  EfiInvalid,
  EfiUnknown
} VARIABLE_STORE_STATUS;

#pragma pack(1)

#define VARIABLE_STORE_SIGNATURE  EFI_AUTHENTICATED_VARIABLE_GUID

///
/// Variable Store Header Format and State.
///
#define VARIABLE_STORE_FORMATTED          0x5a
#define VARIABLE_STORE_HEALTHY            0xfe

///
/// Variable Store region header.
///
typedef struct {
  ///
  /// Variable store region signature.
  ///
  EFI_GUID  Signature;
  ///
  /// Size of entire variable store, 
  /// including size of variable store header but not including the size of FvHeader.
  ///
  UINT32  Size;
  ///
  /// Variable region format state.
  ///
  UINT8   Format;
  ///
  /// Variable region healthy state.
  ///
  UINT8   State;
  UINT16  Reserved;
  UINT32  Reserved1;
} VARIABLE_STORE_HEADER;

///
/// Variable data start flag.
///
#define VARIABLE_DATA                     0x55AA

///
/// Variable State flags.
///
#define VAR_IN_DELETED_TRANSITION     0xfe  ///< Variable is in obsolete transition.
#define VAR_DELETED                   0xfd  ///< Variable is obsolete.
#define VAR_HEADER_VALID_ONLY         0x7f  ///< Variable header has been valid.
#define VAR_ADDED                     0x3f  ///< Variable has been completely added.

///
/// Single Variable Data Header Structure.
///
typedef struct {
  ///
  /// Variable Data Start Flag.
  ///
  UINT16      StartId;
  ///
  /// Variable State defined above.
  ///
  UINT8       State;
  UINT8       Reserved;
  ///
  /// Attributes of variable defined in UEFI specification.
  ///
  UINT32      Attributes;
  ///
  /// Associated monotonic count value against replay attack.
  ///
  UINT64      MonotonicCount;
  ///
  /// Associated TimeStamp value against replay attack. 
  ///
  EFI_TIME    TimeStamp;
  ///
  /// Index of associated public key in database.
  ///
  UINT32      PubKeyIndex;
  ///
  /// Size of variable null-terminated Unicode string name.
  ///
  UINT32      NameSize;
  ///
  /// Size of the variable data without this header.
  ///
  UINT32      DataSize;
  ///
  /// A unique identifier for the vendor that produces and consumes this varaible.
  ///
  EFI_GUID    VendorGuid;
} VARIABLE_HEADER;

#pragma pack()

typedef struct _VARIABLE_INFO_ENTRY  VARIABLE_INFO_ENTRY;

///
/// This structure contains the variable list that is put in EFI system table.
/// The variable driver collects all variables that were used at boot service time and produces this list.
/// This is an optional feature to dump all used variables in shell environment. 
///
struct _VARIABLE_INFO_ENTRY {
  VARIABLE_INFO_ENTRY *Next;       ///< Pointer to next entry.
  EFI_GUID            VendorGuid;  ///< Guid of Variable.
  CHAR16              *Name;       ///< Name of Variable. 
  UINT32              Attributes;  ///< Attributes of variable defined in UEFI spec.
  UINT32              ReadCount;   ///< Number of times to read this variable.
  UINT32              WriteCount;  ///< Number of times to write this variable.
  UINT32              DeleteCount; ///< Number of times to delete this variable.
  UINT32              CacheCount;  ///< Number of times that cache hits this variable.
  BOOLEAN             Volatile;    ///< TRUE if volatile, FALSE if non-volatile.
};

#endif // __AUTHENTICATED_VARIABLE_FORMAT_H__
