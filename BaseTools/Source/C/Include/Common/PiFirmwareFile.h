/** @file
  The firmware file related definitions in PI.

  @par Revision Reference:
  Version 1.4.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PI_FIRMWARE_FILE_H__
#define __PI_FIRMWARE_FILE_H__

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
#define EFI_FV_FILETYPE_SMM                   0x0A
#define EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE 0x0B
#define EFI_FV_FILETYPE_COMBINED_SMM_DXE      0x0C
#define EFI_FV_FILETYPE_SMM_CORE              0x0D
#define EFI_FV_FILETYPE_MM_STANDALONE         0x0E
#define EFI_FV_FILETYPE_MM_CORE_STANDALONE    0x0F
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
#define FFS_ATTRIB_LARGE_FILE         0x01
#define FFS_ATTRIB_DATA_ALIGNMENT2    0x02
#define FFS_ATTRIB_FIXED              0x04
#define FFS_ATTRIB_DATA_ALIGNMENT     0x38
#define FFS_ATTRIB_CHECKSUM           0x40
//
// FFS_FIXED_CHECKSUM is the checksum value used when the
// FFS_ATTRIB_CHECKSUM attribute bit is clear
//
#define FFS_FIXED_CHECKSUM  0xAA

//
// FFS File State Bits.
//
#define EFI_FILE_HEADER_CONSTRUCTION  0x01
#define EFI_FILE_HEADER_VALID         0x02
#define EFI_FILE_DATA_VALID           0x04
#define EFI_FILE_MARKED_FOR_UPDATE    0x08
#define EFI_FILE_DELETED              0x10
#define EFI_FILE_HEADER_INVALID       0x20

#define EFI_FILE_ALL_STATE_BITS       (EFI_FILE_HEADER_CONSTRUCTION | \
                                 EFI_FILE_HEADER_VALID | \
                                 EFI_FILE_DATA_VALID | \
                                 EFI_FILE_MARKED_FOR_UPDATE | \
                                 EFI_FILE_DELETED | \
                                 EFI_FILE_HEADER_INVALID \
                                  )

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

typedef struct {
  EFI_GUID                Name;
  EFI_FFS_INTEGRITY_CHECK IntegrityCheck;
  EFI_FV_FILETYPE         Type;
  EFI_FFS_FILE_ATTRIBUTES Attributes;
  UINT8                   Size[3];
  EFI_FFS_FILE_STATE      State;
  UINT64                  ExtendedSize;
} EFI_FFS_FILE_HEADER2;

#define MAX_FFS_SIZE        0x1000000

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
#define EFI_SECTION_SMM_DEPEX             0x1C

typedef struct {
  UINT8             Size[3];
  EFI_SECTION_TYPE  Type;
} EFI_COMMON_SECTION_HEADER;

typedef struct {
  UINT8             Size[3];
  EFI_SECTION_TYPE  Type;
  UINT32            ExtendedSize;
} EFI_COMMON_SECTION_HEADER2;

#define MAX_SECTION_SIZE        0x1000000

//
// Leaf section type that contains an
// IA-32 16-bit executable image.
//
typedef EFI_COMMON_SECTION_HEADER EFI_COMPATIBILITY16_SECTION;
typedef EFI_COMMON_SECTION_HEADER2 EFI_COMPATIBILITY16_SECTION2;

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

typedef struct {
  EFI_COMMON_SECTION_HEADER2  CommonHeader;
  UINT32                      UncompressedLength;
  UINT8                       CompressionType;
} EFI_COMPRESSION_SECTION2;

//
// Leaf section which could be used to determine the dispatch order of DXEs.
//
typedef EFI_COMMON_SECTION_HEADER EFI_DXE_DEPEX_SECTION;
typedef EFI_COMMON_SECTION_HEADER2 EFI_DXE_DEPEX_SECTION2;

//
// Leaf section witch contains a PI FV.
//
typedef EFI_COMMON_SECTION_HEADER EFI_FIRMWARE_VOLUME_IMAGE_SECTION;
typedef EFI_COMMON_SECTION_HEADER2 EFI_FIRMWARE_VOLUME_IMAGE_SECTION2;

//
// Leaf section which contains a single GUID.
//
typedef struct {
  EFI_COMMON_SECTION_HEADER   CommonHeader;
  EFI_GUID                    SubTypeGuid;
} EFI_FREEFORM_SUBTYPE_GUID_SECTION;

typedef struct {
  EFI_COMMON_SECTION_HEADER2  CommonHeader;
  EFI_GUID                    SubTypeGuid;
} EFI_FREEFORM_SUBTYPE_GUID_SECTION2;

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

typedef struct {
  EFI_COMMON_SECTION_HEADER2  CommonHeader;
  EFI_GUID                    SectionDefinitionGuid;
  UINT16                      DataOffset;
  UINT16                      Attributes;
} EFI_GUID_DEFINED_SECTION2;

//
// Leaf section which contains PE32+ image.
//
typedef EFI_COMMON_SECTION_HEADER EFI_PE32_SECTION;
typedef EFI_COMMON_SECTION_HEADER2 EFI_PE32_SECTION2;

//
// Leaf section which contains PIC image.
//
typedef EFI_COMMON_SECTION_HEADER EFI_PIC_SECTION;
typedef EFI_COMMON_SECTION_HEADER2 EFI_PIC_SECTION2;

//
// Leaf section which used to determine the dispatch order of PEIMs.
//
typedef EFI_COMMON_SECTION_HEADER EFI_PEI_DEPEX_SECTION;
typedef EFI_COMMON_SECTION_HEADER2 EFI_PEI_DEPEX_SECTION2;

//
// Leaf section which constains the position-independent-code image.
//
typedef EFI_COMMON_SECTION_HEADER EFI_TE_SECTION;
typedef EFI_COMMON_SECTION_HEADER2 EFI_TE_SECTION2;

//
// Leaf section which contains an array of zero or more bytes.
//
typedef EFI_COMMON_SECTION_HEADER EFI_RAW_SECTION;
typedef EFI_COMMON_SECTION_HEADER2 EFI_RAW_SECTION2;

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

typedef struct {
  EFI_COMMON_SECTION_HEADER2  CommonHeader;

  //
  // Array of unicode string.
  //
  CHAR16                      FileNameString[1];
} EFI_USER_INTERFACE_SECTION2;

//
// Leaf section which contains a numeric build number and
// an optional unicode string that represent the file revision.
//
typedef struct {
  EFI_COMMON_SECTION_HEADER   CommonHeader;
  UINT16                      BuildNumber;
  CHAR16                      VersionString[1];
} EFI_VERSION_SECTION;

typedef struct {
  EFI_COMMON_SECTION_HEADER2  CommonHeader;
  UINT16                      BuildNumber;
  CHAR16                      VersionString[1];
} EFI_VERSION_SECTION2;

//
// The argument passed as the SectionHeaderPtr parameter to the SECTION_SIZE()
// function-like macro below must not have side effects: SectionHeaderPtr is
// evaluated multiple times.
//
#define SECTION_SIZE(SectionHeaderPtr) ((UINT32) ( \
    (((EFI_COMMON_SECTION_HEADER *) (SectionHeaderPtr))->Size[0]      ) | \
    (((EFI_COMMON_SECTION_HEADER *) (SectionHeaderPtr))->Size[1] <<  8) | \
    (((EFI_COMMON_SECTION_HEADER *) (SectionHeaderPtr))->Size[2] << 16)))

#pragma pack()

typedef union {
  EFI_COMMON_SECTION_HEADER         *CommonHeader;
  EFI_COMPRESSION_SECTION           *CompressionSection;
  EFI_GUID_DEFINED_SECTION          *GuidDefinedSection;
  EFI_PE32_SECTION                  *Pe32Section;
  EFI_PIC_SECTION                   *PicSection;
  EFI_TE_SECTION                    *TeSection;
  EFI_PEI_DEPEX_SECTION             *PeimHeaderSection;
  EFI_DXE_DEPEX_SECTION             *DependencySection;
  EFI_VERSION_SECTION               *VersionSection;
  EFI_USER_INTERFACE_SECTION        *UISection;
  EFI_COMPATIBILITY16_SECTION       *Code16Section;
  EFI_FIRMWARE_VOLUME_IMAGE_SECTION *FVImageSection;
  EFI_FREEFORM_SUBTYPE_GUID_SECTION *FreeformSubtypeSection;
  EFI_RAW_SECTION                   *RawSection;
  //
  // For section whose size is equal or greater than 0x1000000
  //
  EFI_COMMON_SECTION_HEADER2         *CommonHeader2;
  EFI_COMPRESSION_SECTION2           *CompressionSection2;
  EFI_GUID_DEFINED_SECTION2          *GuidDefinedSection2;
  EFI_PE32_SECTION2                  *Pe32Section2;
  EFI_PIC_SECTION2                   *PicSection2;
  EFI_TE_SECTION2                    *TeSection2;
  EFI_PEI_DEPEX_SECTION2             *PeimHeaderSection2;
  EFI_DXE_DEPEX_SECTION2             *DependencySection2;
  EFI_VERSION_SECTION2               *VersionSection2;
  EFI_USER_INTERFACE_SECTION2        *UISection2;
  EFI_COMPATIBILITY16_SECTION2       *Code16Section2;
  EFI_FIRMWARE_VOLUME_IMAGE_SECTION2 *FVImageSection2;
  EFI_FREEFORM_SUBTYPE_GUID_SECTION2 *FreeformSubtypeSection2;
  EFI_RAW_SECTION2                   *RawSection2;
} EFI_FILE_SECTION_POINTER;

#endif

