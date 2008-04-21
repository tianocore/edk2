/** @file
  The firmware file related definitions in PI.

  Copyright (c) 2006 - 2007, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  @par Revision Reference:
  Version 1.0.

**/


#ifndef __PI_FIRMWARE_FILE_H__
#define __PI_FIRMWARE_FILE_H__

#include <ProcessorBind.h>

#pragma pack(1)
//
// Used to verify the integrity of the file.
// 
typedef union {
  struct {
    UINT8   Header;
    UINT8   File;
  } Checksum;
  UINT16    Checksum16;
} EFI_FFS_INTEGRITY_CHECK;

typedef UINT8 EFI_FV_FILETYPE;
typedef UINT8 EFI_FFS_FILE_ATTRIBUTES;
typedef UINT8 EFI_FFS_FILE_STATE;

//
// File Types Definitions
// 
#define EFI_FV_FILETYPE_ALL                   0x00
#define EFI_FV_FILETYPE_RAW                   0x01
#define EFI_FV_FILETYPE_FREEFORM              0x02
#define EFI_FV_FILETYPE_SECURITY_CORE         0x03
#define EFI_FV_FILETYPE_PEI_CORE              0x04
#define EFI_FV_FILETYPE_DXE_CORE              0x05
#define EFI_FV_FILETYPE_PEIM                  0x06
#define EFI_FV_FILETYPE_DRIVER                0x07
#define EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER  0x08
#define EFI_FV_FILETYPE_APPLICATION           0x09
#define EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE 0x0B
#define EFI_FV_FILETYPE_OEM_MIN               0xc0
#define EFI_FV_FILETYPE_OEM_MAX               0xdf
#define EFI_FV_FILETYPE_DEBUG_MIN             0xe0
#define EFI_FV_FILETYPE_DEBUG_MAX             0xef
#define EFI_FV_FILETYPE_FFS_MIN               0xf0
#define EFI_FV_FILETYPE_FFS_MAX               0xff
#define EFI_FV_FILETYPE_FFS_PAD               0xf0
// 
// FFS File Attributes.
// 
#define FFS_ATTRIB_FIXED              0x04
#define FFS_ATTRIB_DATA_ALIGNMENT     0x38
#define FFS_ATTRIB_CHECKSUM           0x40

// 
// FFS File State Bits.
// 
#define EFI_FILE_HEADER_CONSTRUCTION  0x01
#define EFI_FILE_HEADER_VALID         0x02
#define EFI_FILE_DATA_VALID           0x04
#define EFI_FILE_MARKED_FOR_UPDATE    0x08
#define EFI_FILE_DELETED              0x10
#define EFI_FILE_HEADER_INVALID       0x20


//
// Each file begins with the header that describe the 
// contents and state of the files.
// 
typedef struct {
  EFI_GUID                Name;
  EFI_FFS_INTEGRITY_CHECK IntegrityCheck;
  EFI_FV_FILETYPE         Type;
  EFI_FFS_FILE_ATTRIBUTES Attributes;
  UINT8                   Size[3];
  EFI_FFS_FILE_STATE      State;
} EFI_FFS_FILE_HEADER;


typedef UINT8 EFI_SECTION_TYPE;

//
// Pseudo type. It is
// used as a wild card when retrieving sections. The section
// type EFI_SECTION_ALL matches all section types.
//
#define EFI_SECTION_ALL                   0x00

//
// Encapsulation section Type values
//
#define EFI_SECTION_COMPRESSION           0x01

#define EFI_SECTION_GUID_DEFINED          0x02

//
// Leaf section Type values
//
#define EFI_SECTION_PE32                  0x10
#define EFI_SECTION_PIC                   0x11
#define EFI_SECTION_TE                    0x12
#define EFI_SECTION_DXE_DEPEX             0x13
#define EFI_SECTION_VERSION               0x14
#define EFI_SECTION_USER_INTERFACE        0x15
#define EFI_SECTION_COMPATIBILITY16       0x16
#define EFI_SECTION_FIRMWARE_VOLUME_IMAGE 0x17
#define EFI_SECTION_FREEFORM_SUBTYPE_GUID 0x18
#define EFI_SECTION_RAW                   0x19
#define EFI_SECTION_PEI_DEPEX             0x1B

typedef struct {
  UINT8             Size[3];
  EFI_SECTION_TYPE  Type;
} EFI_COMMON_SECTION_HEADER;

//
// Leaf section type that contains an 
// IA-32 16-bit executable image.
// 
typedef EFI_COMMON_SECTION_HEADER EFI_COMPATIBILITY16_SECTION;

//
// CompressionType of EFI_COMPRESSION_SECTION.
// 
#define EFI_NOT_COMPRESSED        0x00
#define EFI_STANDARD_COMPRESSION  0x01
//
// An encapsulation section type in which the 
// section data is compressed.
// 
typedef struct {
  EFI_COMMON_SECTION_HEADER   CommonHeader;
  UINT32                      UncompressedLength;
  UINT8                       CompressionType;
} EFI_COMPRESSION_SECTION;

//
// Leaf section which could be used to determine the dispatch order of DXEs.
// 
typedef EFI_COMMON_SECTION_HEADER EFI_DXE_DEPEX_SECTION;

//
// Leaf section witch contains a PI FV.
// 
typedef EFI_COMMON_SECTION_HEADER EFI_FIRMWARE_VOLUME_IMAGE_SECTION;

//
// Leaf section which contains a single GUID.
// 
typedef struct {
  EFI_COMMON_SECTION_HEADER   CommonHeader;
  EFI_GUID                    SubTypeGuid;
} EFI_FREEFORM_SUBTYPE_GUID_SECTION;

//
// Attributes of EFI_GUID_DEFINED_SECTION
// 
#define EFI_GUIDED_SECTION_PROCESSING_REQUIRED  0x01
#define EFI_GUIDED_SECTION_AUTH_STATUS_VALID    0x02
//
// Leaf section which is encapsulation defined by specific GUID
// 
typedef struct {
  EFI_COMMON_SECTION_HEADER   CommonHeader;
  EFI_GUID                    SectionDefinitionGuid;
  UINT16                      DataOffset;
  UINT16                      Attributes;
} EFI_GUID_DEFINED_SECTION;

//
// Leaf section which contains PE32+ image.
// 
typedef EFI_COMMON_SECTION_HEADER EFI_PE32_SECTION;


//
// Leaf section which used to determine the dispatch order of PEIMs.
// 
typedef EFI_COMMON_SECTION_HEADER EFI_PEI_DEPEX_SECTION;

//
// Leaf section which constains the position-independent-code image.
// 
typedef EFI_COMMON_SECTION_HEADER EFI_TE_SECTION;

//
// Leaf section which contains an array of zero or more bytes.
// 
typedef EFI_COMMON_SECTION_HEADER EFI_RAW_SECTION;

//
// Leaf section which contains a unicode string that 
// is human readable file name.
// 
typedef struct {
  EFI_COMMON_SECTION_HEADER   CommonHeader;

  //
  // Array of unicode string.
  // 
  CHAR16                      FileNameString[1];
} EFI_USER_INTERFACE_SECTION;


//
// Leaf section which contains a numeric build number and
// an optional unicode string that represent the file revision. 
// 
typedef struct {
  EFI_COMMON_SECTION_HEADER   CommonHeader;
  UINT16                      BuildNumber;
  CHAR16                      VersionString[1];
} EFI_VERSION_SECTION;


#define SECTION_SIZE(SectionHeaderPtr) \
    ((UINT32) (*((UINT32 *) ((EFI_COMMON_SECTION_HEADER *) SectionHeaderPtr)->Size) & 0x00ffffff))

#pragma pack()

#endif

