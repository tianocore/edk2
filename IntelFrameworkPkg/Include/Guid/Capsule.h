/** @file
  Framework Capule related Definition.

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  Capsule Spec Version 0.9
**/

#ifndef _CAPSULE_GUID_H__
#define _CAPSULE_GUID_H__

//
// This is the GUID of the capsule header of the image on disk.
//
#define EFI_CAPSULE_GUID \
  { \
    0x3B6686BD, 0x0D76, 0x4030, {0xB7, 0x0E, 0xB5, 0x51, 0x9E, 0x2F, 0xC5, 0xA0 } \
  }

//
// This is the GUID of the configuration results file created by the capsule
// application.
//
#define EFI_CONFIG_FILE_NAME_GUID \
  { \
    0x98B8D59B, 0xE8BA, 0x48EE, {0x98, 0xDD, 0xC2, 0x95, 0x39, 0x2F, 0x1E, 0xDB } \
  }

///
/// Bits in the flags field of the capsule header.
/// This flag is set if the capsule can support setup changes, and cleared if it cannot.
///
#define EFI_CAPSULE_HEADER_FLAG_SETUP 0x00000001

#define CAPSULE_BLOCK_DESCRIPTOR_SIGNATURE  SIGNATURE_32 ('C', 'B', 'D', 'S')

//
// An array of these structs describe the blocks that make up a capsule for
// a capsule update.
//
typedef struct {
  UINT64                Length;     ///< Length of the data block.
  EFI_PHYSICAL_ADDRESS  Data;       ///< Physical address of the data block.
  UINT32                Signature;  ///< CBDS.
  UINT32                CheckSum;   ///< To sum this structure to 0.
} FRAMEWORK_EFI_CAPSULE_BLOCK_DESCRIPTOR;

typedef struct {
  EFI_GUID  OemGuid;
  UINT32    HeaderSize;
  //
  // UINT8                       OemHdrData[];
  //
} EFI_CAPSULE_OEM_HEADER;

typedef struct {
  ///
  /// A defined GUID that indicates the start of a capsule.
  ///
  EFI_GUID  CapsuleGuid;
  ///
  /// The size of the EFI_CAPSULE_HEADER structure.
  ///
  UINT32    HeaderSize;
  ///
  /// A bit-mapped list describing the capsule's attributes. 
  /// All undefined bits should be written as zero (0).
  ///
  UINT32    Flags;
  ///
  /// The length in bytes (27,415 for an image containing 27,415 bytes) of the entire image
  /// including all headers. If this value is greater than the size of the data presented in
  /// the capsule body, the image is separated across multiple media. If this
  /// value is less than the size of the data, it is an error.
  ///
  UINT32    CapsuleImageSize;
  ///
  /// A zero-based number that enables a capsule to be split into pieces and then
  /// recombined for easier transfer across media with limited size. The lower the
  /// SequenceNumber, the earlier in the final image that the part of the capsule is to
  /// appear. In capsules that are not split, this value shall be zero.
  ///
  UINT32    SequenceNumber;
  ///
  /// Used to group the various pieces of a split capsule to ensure that they comprise the
  /// same base image. It is valid for this item to be zero, in which case the capsule cannot
  /// be split into components.
  ///
  EFI_GUID  InstanceId;
  ///
  /// The offset in bytes from the beginning of the header to the start of an EFI string that
  /// contains a description of the identity of the subcapsules that make up the capsule. If
  /// the capsule is not split, this value should be zero. The same string should be
  /// presented for all subcapsules that constitute the same capsule.
  ///
  UINT32    OffsetToSplitInformation;
  ///
  /// The offset in bytes from the beginning of the header to the start of the part of the
  /// capsule that is to be transferred to DXE.
  ///
  UINT32    OffsetToCapsuleBody;
  ///
  /// The offset in bytes from the beginning of the header to the start of the OEM-defined
  /// header. This value must be less than OffsetToCapsuleBody.
  ///
  UINT32    OffsetToOemDefinedHeader;
  ///
  /// The offset in bytes from the beginning of the header to the start of human-readable
  /// text that describes the entity that created the capsule. This value must be less than OffsetToCapsuleBody.
  ///
  UINT32    OffsetToAuthorInformation;
  ///
  /// The offset in bytes from the beginning of the header to the start of human-readable
  /// text that describes the revision of the capsule and/or the capsule's contents. This
  /// value must be less than OffsetToCapsuleBody.
  ///
  UINT32    OffsetToRevisionInformation;
  ///
  /// The offset in bytes from the beginning of the header to the start of a one-line (less
  /// than 40 Unicode characters in any language) description of the capsule. It is intended
  /// to be used by OS-present applications when providing a list of capsules from which
  /// the user can choose. This value must be less than OffsetToCapsuleBody.
  ///
  UINT32    OffsetToShortDescription;
  ///
  /// The offset in bytes from the beginning of the header to the start of an EFI string
  ///
  UINT32    OffsetToLongDescription;
  ///
  /// This field is reserved for future use by this specification. For future compatibility,
  /// this field must be set to zero
  ///
  UINT32    OffsetToApplicableDevices;
} FRAMEWORK_EFI_CAPSULE_HEADER;

extern EFI_GUID gEfiCapsuleGuid;
extern EFI_GUID gEfiConfigFileNameGuid;

#endif
