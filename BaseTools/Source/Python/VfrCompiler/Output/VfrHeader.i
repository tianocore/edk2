/** @file
  This includes some definitions introduced in UEFI that will be used in both PEI and DXE phases.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __UEFI_MULTIPHASE_H__
#define __UEFI_MULTIPHASE_H__

///
/// Attributes of variable.
///
#define EFI_VARIABLE_NON_VOLATILE        0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS  0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS      0x00000004
///
/// This attribute is identified by the mnemonic 'HR'
/// elsewhere in this specification.
///
#define EFI_VARIABLE_HARDWARE_ERROR_RECORD  0x00000008
///
/// Attributes of Authenticated Variable
///
#define EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS  0x00000020
#define EFI_VARIABLE_APPEND_WRITE                           0x00000040
///
/// NOTE: EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS is deprecated and should be considered reserved.
///
#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS  0x00000010

#ifndef VFRCOMPILE
  #include <Guid/WinCertificate.h>
///
/// Enumeration of memory types introduced in UEFI.
///
typedef enum {
  ///
  /// Not used.
  ///
  EfiReservedMemoryType,
  ///
  /// The code portions of a loaded application.
  /// (Note that UEFI OS loaders are UEFI applications.)
  ///
  EfiLoaderCode,
  ///
  /// The data portions of a loaded application and the default data allocation
  /// type used by an application to allocate pool memory.
  ///
  EfiLoaderData,
  ///
  /// The code portions of a loaded Boot Services Driver.
  ///
  EfiBootServicesCode,
  ///
  /// The data portions of a loaded Boot Serves Driver, and the default data
  /// allocation type used by a Boot Services Driver to allocate pool memory.
  ///
  EfiBootServicesData,
  ///
  /// The code portions of a loaded Runtime Services Driver.
  ///
  EfiRuntimeServicesCode,
  ///
  /// The data portions of a loaded Runtime Services Driver and the default
  /// data allocation type used by a Runtime Services Driver to allocate pool memory.
  ///
  EfiRuntimeServicesData,
  ///
  /// Free (unallocated) memory.
  ///
  EfiConventionalMemory,
  ///
  /// Memory in which errors have been detected.
  ///
  EfiUnusableMemory,
  ///
  /// Memory that holds the ACPI tables.
  ///
  EfiACPIReclaimMemory,
  ///
  /// Address space reserved for use by the firmware.
  ///
  EfiACPIMemoryNVS,
  ///
  /// Used by system firmware to request that a memory-mapped IO region
  /// be mapped by the OS to a virtual address so it can be accessed by EFI runtime services.
  ///
  EfiMemoryMappedIO,
  ///
  /// System memory-mapped IO region that is used to translate memory
  /// cycles to IO cycles by the processor.
  ///
  EfiMemoryMappedIOPortSpace,
  ///
  /// Address space reserved by the firmware for code that is part of the processor.
  ///
  EfiPalCode,
  ///
  /// A memory region that operates as EfiConventionalMemory,
  /// however it happens to also support byte-addressable non-volatility.
  ///
  EfiPersistentMemory,
  EfiMaxMemoryType
} EFI_MEMORY_TYPE;

///
/// Enumeration of reset types.
///
typedef enum {
  ///
  /// Used to induce a system-wide reset. This sets all circuitry within the
  /// system to its initial state.  This type of reset is asynchronous to system
  /// operation and operates withgout regard to cycle boundaries.  EfiColdReset
  /// is tantamount to a system power cycle.
  ///
  EfiResetCold,
  ///
  /// Used to induce a system-wide initialization. The processors are set to their
  /// initial state, and pending cycles are not corrupted.  If the system does
  /// not support this reset type, then an EfiResetCold must be performed.
  ///
  EfiResetWarm,
  ///
  /// Used to induce an entry into a power state equivalent to the ACPI G2/S5 or G3
  /// state.  If the system does not support this reset type, then when the system
  /// is rebooted, it should exhibit the EfiResetCold attributes.
  ///
  EfiResetShutdown,
  ///
  /// Used to induce a system-wide reset. The exact type of the reset is defined by
  /// the EFI_GUID that follows the Null-terminated Unicode string passed into
  /// ResetData. If the platform does not recognize the EFI_GUID in ResetData the
  /// platform must pick a supported reset type to perform. The platform may
  /// optionally log the parameters from any non-normal reset that occurs.
  ///
  EfiResetPlatformSpecific
} EFI_RESET_TYPE;

///
/// Data structure that precedes all of the standard EFI table types.
///
typedef struct {
  ///
  /// A 64-bit signature that identifies the type of table that follows.
  /// Unique signatures have been generated for the EFI System Table,
  /// the EFI Boot Services Table, and the EFI Runtime Services Table.
  ///
  UINT64    Signature;
  ///
  /// The revision of the EFI Specification to which this table
  /// conforms. The upper 16 bits of this field contain the major
  /// revision value, and the lower 16 bits contain the minor revision
  /// value. The minor revision values are limited to the range of 00..99.
  ///
  UINT32    Revision;
  ///
  /// The size, in bytes, of the entire table including the EFI_TABLE_HEADER.
  ///
  UINT32    HeaderSize;
  ///
  /// The 32-bit CRC for the entire table. This value is computed by
  /// setting this field to 0, and computing the 32-bit CRC for HeaderSize bytes.
  ///
  UINT32    CRC32;
  ///
  /// Reserved field that must be set to 0.
  ///
  UINT32    Reserved;
} EFI_TABLE_HEADER;

///
/// AuthInfo is a WIN_CERTIFICATE using the wCertificateType
/// WIN_CERTIFICATE_UEFI_GUID and the CertType
/// EFI_CERT_TYPE_RSA2048_SHA256_GUID. If the attribute specifies
/// authenticated access, then the Data buffer should begin with an
/// authentication descriptor prior to the data payload and DataSize
/// should reflect the the data.and descriptor size. The caller
/// shall digest the Monotonic Count value and the associated data
/// for the variable update using the SHA-256 1-way hash algorithm.
/// The ensuing the 32-byte digest will be signed using the private
/// key associated w/ the public/private 2048-bit RSA key-pair. The
/// WIN_CERTIFICATE shall be used to describe the signature of the
/// Variable data *Data. In addition, the signature will also
/// include the MonotonicCount value to guard against replay attacks.
///
typedef struct {
  ///
  /// Included in the signature of
  /// AuthInfo.Used to ensure freshness/no
  /// replay. Incremented during each
  /// "Write" access.
  ///
  UINT64    MonotonicCount;
  ///
  /// Provides the authorization for the variable
  /// access. It is a signature across the
  /// variable data and the  Monotonic Count
  /// value. Caller uses Private key that is
  /// associated with a public key that has been
  /// provisioned via the key exchange.
  ///
  WIN_CERTIFICATE_UEFI_GUID    AuthInfo;
} EFI_VARIABLE_AUTHENTICATION;

///
/// When the attribute EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS is
/// set, then the Data buffer shall begin with an instance of a complete (and serialized)
/// EFI_VARIABLE_AUTHENTICATION_2 descriptor. The descriptor shall be followed by the new
/// variable value and DataSize shall reflect the combined size of the descriptor and the new
/// variable value. The authentication descriptor is not part of the variable data and is not
/// returned by subsequent calls to GetVariable().
///
typedef struct {
  ///
  /// For the TimeStamp value, components Pad1, Nanosecond, TimeZone, Daylight and
  /// Pad2 shall be set to 0. This means that the time shall always be expressed in GMT.
  ///
  EFI_TIME                     TimeStamp;
  ///
  /// Only a CertType of  EFI_CERT_TYPE_PKCS7_GUID is accepted.
  ///
  WIN_CERTIFICATE_UEFI_GUID    AuthInfo;
} EFI_VARIABLE_AUTHENTICATION_2;
#endif // VFRCOMPILE

#endif
/** @file
  GUID for UEFI WIN_CERTIFICATE structure.

  Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  GUID defined in UEFI 2.0 spec.
**/

#ifndef __EFI_WIN_CERTIFICATE_H__
#define __EFI_WIN_CERTIFICATE_H__

//
// _WIN_CERTIFICATE.wCertificateType
//
#define WIN_CERT_TYPE_PKCS_SIGNED_DATA  0x0002
#define WIN_CERT_TYPE_EFI_PKCS115       0x0EF0
#define WIN_CERT_TYPE_EFI_GUID          0x0EF1

///
/// The WIN_CERTIFICATE structure is part of the PE/COFF specification.
///
typedef struct {
  ///
  /// The length of the entire certificate,
  /// including the length of the header, in bytes.
  ///
  UINT32    dwLength;
  ///
  /// The revision level of the WIN_CERTIFICATE
  /// structure. The current revision level is 0x0200.
  ///
  UINT16    wRevision;
  ///
  /// The certificate type. See WIN_CERT_TYPE_xxx for the UEFI
  /// certificate types. The UEFI specification reserves the range of
  /// certificate type values from 0x0EF0 to 0x0EFF.
  ///
  UINT16    wCertificateType;
  ///
  /// The following is the actual certificate. The format of
  /// the certificate depends on wCertificateType.
  ///
  /// UINT8 bCertificate[ANYSIZE_ARRAY];
  ///
} WIN_CERTIFICATE;

///
/// WIN_CERTIFICATE_UEFI_GUID.CertType
///
#define EFI_CERT_TYPE_RSA2048_SHA256_GUID \
  {0xa7717414, 0xc616, 0x4977, {0x94, 0x20, 0x84, 0x47, 0x12, 0xa7, 0x35, 0xbf } }

///
/// WIN_CERTIFICATE_UEFI_GUID.CertData
///
typedef struct {
  EFI_GUID    HashType;
  UINT8       PublicKey[256];
  UINT8       Signature[256];
} EFI_CERT_BLOCK_RSA_2048_SHA256;

///
/// Certificate which encapsulates a GUID-specific digital signature
///
typedef struct {
  ///
  /// This is the standard WIN_CERTIFICATE header, where
  /// wCertificateType is set to WIN_CERT_TYPE_EFI_GUID.
  ///
  WIN_CERTIFICATE    Hdr;
  ///
  /// This is the unique id which determines the
  /// format of the CertData. .
  ///
  EFI_GUID           CertType;
  ///
  /// The following is the certificate data. The format of
  /// the data is determined by the CertType.
  /// If CertType is EFI_CERT_TYPE_RSA2048_SHA256_GUID,
  /// the CertData will be EFI_CERT_BLOCK_RSA_2048_SHA256 structure.
  ///
  UINT8              CertData[1];
} WIN_CERTIFICATE_UEFI_GUID;

///
/// Certificate which encapsulates the RSASSA_PKCS1-v1_5 digital signature.
///
/// The WIN_CERTIFICATE_UEFI_PKCS1_15 structure is derived from
/// WIN_CERTIFICATE and encapsulate the information needed to
/// implement the RSASSA-PKCS1-v1_5 digital signature algorithm as
/// specified in RFC2437.
///
typedef struct {
  ///
  /// This is the standard WIN_CERTIFICATE header, where
  /// wCertificateType is set to WIN_CERT_TYPE_UEFI_PKCS1_15.
  ///
  WIN_CERTIFICATE    Hdr;
  ///
  /// This is the hashing algorithm which was performed on the
  /// UEFI executable when creating the digital signature.
  ///
  EFI_GUID           HashAlgorithm;
  ///
  /// The following is the actual digital signature. The
  /// size of the signature is the same size as the key
  /// (1024-bit key is 128 bytes) and can be determined by
  /// subtracting the length of the other parts of this header
  /// from the total length of the certificate as found in
  /// Hdr.dwLength.
  ///
  /// UINT8 Signature[];
  ///
} WIN_CERTIFICATE_EFI_PKCS1_15;

extern EFI_GUID  gEfiCertTypeRsa2048Sha256Guid;

#endif
/** @file

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>*
(C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  NVDataStruc.h

Abstract:

  NVData structure used by the sample driver

Revision History:


**/

#ifndef _NVDATASTRUC_H_
#define _NVDATASTRUC_H_

#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/HiiFormMapMethodGuid.h>
#include <Guid/DriverSampleHii.h>
#include <Guid/ZeroGuid.h>

#define CONFIGURATION_VARSTORE_ID  0x1234
#define BITS_VARSTORE_ID           0x2345

#pragma pack(1)

//
// !!! For a structure with a series of bit fields and used as a storage in vfr file, and if the bit fields do not add up to the size of the defined type.
// In the C code use sizeof() to get the size the strucure, the results may vary form the compiler(VS,GCC...).
// But the size of the storage calculated by VfrCompiler is fixed (calculate with alignment).
// To avoid above case, we need to make the total bit width in the structure aligned with the size of the defined type for these bit fields. We can:
// 1. Add bit field (with/without name) with remianing with for padding.
// 2. Add unnamed bit field with 0 for padding, the amount of padding is determined by the alignment characteristics of the members of the structure.
//
typedef struct {
  UINT16    NestByteField;
  UINT8                     : 1; // unamed field can be used for padding
  UINT8     NestBitCheckbox : 1;
  UINT8     NestBitOneof    : 2;
  UINT8                     : 0; // Special width 0 can be used to force alignment at the next word boundary
  UINT8     NestBitNumeric  : 4;
} MY_BITS_DATA;

typedef union {
  UINT8    UnionNumeric;
  UINT8    UnionNumericAlias;
} MY_EFI_UNION_DATA;

typedef struct {
  UINT16               MyStringData[40];
  UINT16               SomethingHiddenForHtml;
  UINT8                HowOldAreYouInYearsManual;
  UINT16               HowTallAreYouManual;
  UINT8                HowOldAreYouInYears;
  UINT16               HowTallAreYou;
  UINT8                MyFavoriteNumber;
  UINT8                TestLateCheck;
  UINT8                TestLateCheck2;
  UINT8                QuestionAboutTreeHugging;
  UINT8                ChooseToActivateNuclearWeaponry;
  UINT8                SuppressGrayOutSomething;
  UINT8                OrderedList[8];
  UINT16               BootOrder[8];
  UINT8                BootOrderLarge;
  UINT8                DynamicRefresh;
  UINT8                DynamicOneof;
  UINT8                DynamicOrderedList[5];
  UINT8                Reserved;
  EFI_HII_REF          RefData;
  UINT8                NameValueVar0;
  UINT16               NameValueVar1;
  UINT16               NameValueVar2[20];
  UINT8                SerialPortNo;
  UINT8                SerialPortStatus;
  UINT16               SerialPortIo;
  UINT8                SerialPortIrq;
  UINT8                GetDefaultValueFromCallBack;
  UINT8                GetDefaultValueFromAccess;
  EFI_HII_TIME         Time;
  UINT8                RefreshGuidCount;
  UINT8                Match2;
  UINT8                GetDefaultValueFromCallBackForOrderedList[3];
  UINT8                BitCheckbox  : 1;
  UINT8                ReservedBits : 7; // Reserved bit fields for padding.
  UINT16               BitOneof     : 6;
  UINT16                            : 0; // Width 0 used to force alignment.
  UINT16               BitNumeric   : 12;
  MY_BITS_DATA         MyBitData;
  MY_EFI_UNION_DATA    MyUnionData;
  UINT8                QuestionXUefiKeywordRestStyle;
  UINT8                QuestionNonXUefiKeywordRestStyle;
} DRIVER_SAMPLE_CONFIGURATION;

//
// 2nd NV data structure definition
//
typedef struct {
  UINT8     Field8;
  UINT16    Field16;
  UINT8     OrderedList[3];
  UINT16    SubmittedCallback;
} MY_EFI_VARSTORE_DATA;

//
// 3rd NV data structure definition
//
typedef struct {
  MY_BITS_DATA    BitsData;
  UINT32          EfiBitGrayoutTest : 5;
  UINT32          EfiBitNumeric     : 4;
  UINT32          EfiBitOneof       : 10;
  UINT32          EfiBitCheckbox    : 1;
  UINT32                            : 0; // Width 0 used to force alignment.
} MY_EFI_BITS_VARSTORE_DATA;

//
// Labels definition
//
#define LABEL_UPDATE1  0x1234
#define LABEL_UPDATE2  0x2234
#define LABEL_UPDATE3  0x3234
#define LABEL_END      0x2223

#pragma pack()

#endif
/** @file
  GUID indicates that the form set contains forms designed to be used
  for platform configuration and this form set will be displayed.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  GUID defined in UEFI 2.1.

**/

#ifndef __HII_PLATFORM_SETUP_FORMSET_GUID_H__
#define __HII_PLATFORM_SETUP_FORMSET_GUID_H__

#define EFI_HII_PLATFORM_SETUP_FORMSET_GUID \
  { 0x93039971, 0x8545, 0x4b04, { 0xb4, 0x5e, 0x32, 0xeb, 0x83, 0x26, 0x4, 0xe } }

#define EFI_HII_DRIVER_HEALTH_FORMSET_GUID \
  { 0xf22fc20c, 0x8cf4, 0x45eb, { 0x8e, 0x6, 0xad, 0x4e, 0x50, 0xb9, 0x5d, 0xd3 } }

#define EFI_HII_USER_CREDENTIAL_FORMSET_GUID \
  { 0x337f4407, 0x5aee, 0x4b83, { 0xb2, 0xa7, 0x4e, 0xad, 0xca, 0x30, 0x88, 0xcd } }

#define EFI_HII_REST_STYLE_FORMSET_GUID \
  { 0x790217bd, 0xbecf, 0x485b, { 0x91, 0x70, 0x5f, 0xf7, 0x11, 0x31, 0x8b, 0x27 } }

extern EFI_GUID  gEfiHiiPlatformSetupFormsetGuid;
extern EFI_GUID  gEfiHiiDriverHealthFormsetGuid;
extern EFI_GUID  gEfiHiiUserCredentialFormsetGuid;
extern EFI_GUID  gEfiHiiRestStyleFormsetGuid;

#endif
/** @file
  Guid used to identify HII FormMap configuration method.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  GUID defined in UEFI 2.2 spec.
**/

#ifndef __EFI_HII_FORMMAP_GUID_H__
#define __EFI_HII_FORMMAP_GUID_H__

#define EFI_HII_STANDARD_FORM_GUID \
  { 0x3bd2f4ec, 0xe524, 0x46e4, { 0xa9, 0xd8, 0x51, 0x1, 0x17, 0x42, 0x55, 0x62 } }

extern EFI_GUID  gEfiHiiStandardFormGuid;

#endif
/** @file
  GUIDs used as HII FormSet and HII Package list GUID in Driver Sample driver.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DRIVER_SAMPLE_HII_GUID_H__
#define __DRIVER_SAMPLE_HII_GUID_H__

#define DRIVER_SAMPLE_FORMSET_GUID \
  { \
    0xA04A27f4, 0xDF00, 0x4D42, {0xB5, 0x52, 0x39, 0x51, 0x13, 0x02, 0x11, 0x3D} \
  }

#define DRIVER_SAMPLE_INVENTORY_GUID \
  { \
    0xb3f56470, 0x6141, 0x4621, {0x8f, 0x19, 0x70, 0x4e, 0x57, 0x7a, 0xa9, 0xe8} \
  }

#define EFI_IFR_REFRESH_ID_OP_GUID \
  { \
    0xF5E655D9, 0x02A6, 0x46f2, {0x9E, 0x76, 0xB8, 0xBE, 0x8E, 0x60, 0xAB, 0x22} \
  }

extern EFI_GUID  gDriverSampleFormSetGuid;
extern EFI_GUID  gDriverSampleInventoryGuid;
extern EFI_GUID  gEfiIfrRefreshIdOpGuid;

#endif
/** @file
  GUID has all zero values.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ZERO_GUID_H__
#define __ZERO_GUID_H__

#define ZERO_GUID \
  { \
    0x0, 0x0, 0x0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0} \
  }

extern EFI_GUID  gZeroGuid;

#endif
