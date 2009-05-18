/** @file
  This includes some definitions introduced in UEFI that will be used in both PEI and DXE phases.

  Copyright (c) 2006 - 2009, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __UEFI_MULTIPHASE_H__
#define __UEFI_MULTIPHASE_H__

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
  /// The code portions of a loaded Boot Services Driver
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
  EfiMaxMemoryType
} EFI_MEMORY_TYPE;

///
/// Data structure that precedes all of the standard EFI table types.
/// 
typedef struct {
  ///
  /// A 64-bit signature that identifies the type of table that follows.
  /// Unique signatures have been generated for the EFI System Table,
  /// the EFI Boot Services Table, and the EFI Runtime Services Table.
  ///
  UINT64  Signature;
  ///
  /// The revision of the EFI Specification to which this table
  /// conforms. The upper 16 bits of this field contain the major
  /// revision value, and the lower 16 bits contain the minor revision
  /// value. The minor revision values are limited to the range of 00..99.
  ///
  UINT32  Revision;
  ///
  /// The size, in bytes, of the entire table including the EFI_TABLE_HEADER.
  ///
  UINT32  HeaderSize;
  ///
  /// The 32-bit CRC for the entire table. This value is computed by
  /// setting this field to 0, and computing the 32-bit CRC for HeaderSize bytes.
  ///
  UINT32  CRC32;
  ///
  /// Reserved field that must be set to 0.
  ///
  UINT32  Reserved;
} EFI_TABLE_HEADER;

///
/// Attributes of variable.
/// 
#define EFI_VARIABLE_NON_VOLATILE                 0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS           0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS               0x00000004
#define EFI_VARIABLE_HARDWARE_ERROR_RECORD        0x00000008

///
/// This attribute is identified by the mnemonic 'HR' 
/// elsewhere in this specification.
/// 
#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS   0x00000010

//
// _WIN_CERTIFICATE.wCertificateType
// 
#define WIN_CERT_TYPE_EFI_PKCS115   0x0EF0
#define WIN_CERT_TYPE_EFI_GUID      0x0EF1

///
/// The WIN_CERTIFICATE structure is part of the PE/COFF specification.
///
typedef struct _WIN_CERTIFICATE {
  ///
  /// The length of the entire certificate,  
  /// including the length of the header, in bytes.                                
  ///
  UINT32  dwLength;
  ///
  /// The revision level of the WIN_CERTIFICATE 
  /// structure. The current revision level is 0x0200.                                   
  ///
  UINT16  wRevision;
  ///
  /// The certificate type. See WIN_CERT_TYPE_xxx for the UEFI      
  /// certificate types. The UEFI specification reserves the range of 
  /// certificate type values from 0x0EF0 to 0x0EFF.                          
  ///
  UINT16  wCertificateType;
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
typedef struct _EFI_CERT_BLOCK_RSA_2048_SHA256 {
  UINT32  HashType;
  UINT8   PublicKey[256];
  UINT8   Signature[256];
} EFI_CERT_BLOCK_RSA_2048_SHA256;


///
/// Certificate which encapsulates a GUID-specific digital signature
///
typedef struct _WIN_CERTIFICATE_UEFI_GUID {
  ///
  /// This is the standard WIN_CERTIFICATE header, where
  /// wCertificateType is set to WIN_CERT_TYPE_UEFI_GUID. 
  ///                         
  WIN_CERTIFICATE   Hdr;
  ///
  /// This is the unique id which determines the 
  /// format of the CertData. In this case, the  
  /// value is EFI_CERT_TYPE_RSA2048_SHA256_GUID.
  ///
  EFI_GUID          CertType;
  /// 
  /// The following is the certificate data. The format of
  /// the data is determined by the CertType. In this case the value is                     
  /// EFI_CERT_BLOCK_RSA_2048_SHA256.            
  ///
  /// UINT8            CertData[ANYSIZE_ARRAY];
  ///
} WIN_CERTIFICATE_UEFI_GUID;


///   
/// Certificate which encapsulates the RSASSA_PKCS1-v1_5 digital signature.
///  
/// The WIN_CERTIFICATE_UEFI_PKCS1_15 structure is derived from
/// WIN_CERTIFICATE and encapsulate the information needed to  
/// implement the RSASSA-PKCS1-v1_5 digital signature algorithm as  
/// specified in RFC2437.  
///  
typedef struct _WIN_CERTIFICATE_EFI_PKCS1_15 {     
  ///
  /// This is the standard WIN_CERTIFICATE header, where 
  /// wCertificateType is set to WIN_CERT_TYPE_UEFI_PKCS1_15.                       
  ///
  WIN_CERTIFICATE Hdr;
  ///
  /// This is the hashing algorithm which was performed on the
  /// UEFI executable when creating the digital signature. 
  ///
  EFI_GUID        HashAlgorithm;
  ///
  /// The following is the actual digital signature. The   
  /// size of the signature is the same size as the key 
  /// (1024-bit key is 128 bytes) and can be determined by 
  /// subtracting the length of the other parts of this header
  /// from the total length of the certificate as found in 
  /// Hdr.dwLength.                               
  ///
  /// UINT8 Signature[ANYSIZE_ARRAY];
  ///
} WIN_CERTIFICATE_EFI_PKCS1_15;



///   
/// AuthInfo is a WIN_CERTIFICATE using the wCertificateType
/// WIN_CERTIFICATE_UEFI_GUID and the CertType
/// EFI_CERT_TYPE_RSA2048_SHA256. If the attribute specifies
/// authenticated access, then the Data buffer should begin with an
/// authentication descriptor prior to the data payload and DataSize
/// should reflect the the data.and descriptor size. The caller
/// shall digest the Monotonic Count value and the associated data
/// for the variable update using the SHA-256 1-way hash algorithm.
/// The ensuing the 32-byte digest will be signed using the private
/// key associated w/ the public/private 2048-bit RSA key-pair. The
/// WIN_CERTIFICATE shall be used to describe the signature of the
/// Variable data *Data. In addition, the signature will also
/// include the MonotonicCount value to guard against replay attacks
///  
typedef struct {
  ///
  /// Included in the signature of        
  /// AuthInfo.Used to ensure freshness/no
  /// replay. Incremented during each     
  /// "Write" access.   
  ///                  
  UINT64                      MonotonicCount;
  ///
  /// Provides the authorization for the variable 
  /// access. It is a signature across the        
  /// variable data and the  Monotonic Count      
  /// value. Caller uses Private key that is      
  /// associated with a public key that has been  
  /// provisioned via the key exchange.           
  ///
  WIN_CERTIFICATE_UEFI_GUID   AuthInfo;
} EFI_VARIABLE_AUTHENTICATION;

#endif

