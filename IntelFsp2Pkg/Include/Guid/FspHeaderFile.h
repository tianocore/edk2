/** @file
  Intel FSP Header File definition from Intel Firmware Support Package External
  Architecture Specification v2.0 and above.

  Copyright (c) 2014 - 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>

#ifndef __FSP_HEADER_FILE_H__
#define __FSP_HEADER_FILE_H__

#define FSP_HEADER_REVISION_3  3

#define FSPE_HEADER_REVISION_1  1
#define FSPP_HEADER_REVISION_1  1

///
/// Fixed FSP header offset in the FSP image
///
#define FSP_INFO_HEADER_OFF  0x94

#define OFFSET_IN_FSP_INFO_HEADER(x)  (UINT32)&((FSP_INFO_HEADER *)(UINTN)0)->x

#define FSP_INFO_HEADER_SIGNATURE  SIGNATURE_32 ('F', 'S', 'P', 'H')

#define IMAGE_ATTRIBUTE_GRAPHICS_SUPPORT       BIT0
#define IMAGE_ATTRIBUTE_DISPATCH_MODE_SUPPORT  BIT1
#define IMAGE_ATTRIBUTE_64BIT_MODE_SUPPORT     BIT2
#define FSP_IA32                               0
#define FSP_X64                                1

  #pragma pack(1)

///
/// FSP Information Header as described in FSP v2.0 Spec section 5.1.1.
///
typedef struct {
  ///
  /// Byte 0x00: Signature ('FSPH') for the FSP Information Header.
  ///
  UINT32    Signature;
  ///
  /// Byte 0x04: Length of the FSP Information Header.
  ///
  UINT32    HeaderLength;
  ///
  /// Byte 0x08: Reserved.
  ///
  UINT8     Reserved1[2];
  ///
  /// Byte 0x0A: Indicates compliance with a revision of this specification in the BCD format.
  ///            For revision v2.4 the value will be 0x24.
  ///
  UINT8     SpecVersion;
  ///
  /// Byte 0x0B: Revision of the FSP Information Header.
  ///            The Current value for this field is 0x7.
  ///
  UINT8     HeaderRevision;
  ///
  /// Byte 0x0C: Revision of the FSP binary.
  ///            Major.Minor.Revision.Build
  ///            If FSP HeaderRevision is <= 5, the ImageRevision can be decoded as follows:
  ///               7 : 0  - Build Number
  ///              15 : 8  - Revision
  ///              23 : 16 - Minor Version
  ///              31 : 24 - Major Version
  ///            If FSP HeaderRevision is >= 6, ImageRevision specifies the low-order bytes of the build number and revision
  ///            while ExtendedImageRevision specifies the high-order bytes of the build number and revision.
  ///               7 : 0  - Low Byte of Build Number
  ///              15 : 8  - Low Byte of Revision
  ///              23 : 16 - Minor Version
  ///              31 : 24 - Major Version
  ///
  UINT32    ImageRevision;
  ///
  /// Byte 0x10: Signature string that will help match the FSP Binary to a supported HW configuration.
  ///
  CHAR8     ImageId[8];
  ///
  /// Byte 0x18: Size of the entire FSP binary.
  ///
  UINT32    ImageSize;
  ///
  /// Byte 0x1C: FSP binary preferred base address.
  ///
  UINT32    ImageBase;
  ///
  /// Byte 0x20: Attribute for the FSP binary.
  ///   Bit 0: Graphics Support - Set to 1 when FSP supports enabling Graphics Display.
  ///   Bit 1: Dispatch Mode Support - Set to 1 when FSP supports the optional Dispatch Mode API defined in Section 7.2 and 9. This bit is only valid if FSP HeaderRevision is >= 4.
  ///   Bit 2: 64-bit mode support - Set to 1 to indicate FSP supports 64-bit long mode interfaces. Set to 0 to indicate FSP supports 32-bit mode interfaces. This bit is only valid if FSP HeaderRevision is >= 7.
  ///   Bit 3: FSP Variable Services Support - Set to 1 to indicate FSP utilizes the FSP Variable Services defined in Section 9.6 to store non-volatile data. This bit is only valid if FSP HeaderRevision is >= 7.
  ///   Bits 15:4 - Reserved
  ///
  UINT16    ImageAttribute;
  ///
  /// Byte 0x22: Attributes of the FSP Component.
  ///   Bit 0 - Build Type
  ///     0 - Debug Build
  ///     1 - Release Build
  ///   Bit 1 - Release Type
  ///     0 - Test Release
  ///     1 - Official Release
  ///   Bit 11:2 - Reserved
  ///   Bits 15:12 - Component Type
  ///     0000 - Reserved
  ///     0001 - FSP-T
  ///     0010 - FSP-M
  ///     0011 - FSP-S
  ///     0100 - FSP-I (FSP SMM)
  ///     0101 to 0111 - Reserved
  ///     1000 - FSP-O
  ///     1001 to 1111 - Reserved
  ///
  UINT16    ComponentAttribute;
  ///
  /// Byte 0x24: Offset of the FSP configuration region.
  ///
  UINT32    CfgRegionOffset;
  ///
  /// Byte 0x28: Size of the FSP configuration region.
  ///
  UINT32    CfgRegionSize;
  ///
  /// Byte 0x2C: Reserved2.
  ///
  UINT32    Reserved2;
  ///
  /// Byte 0x30: The offset for the API to setup a temporary stack till the memory is initialized.
  ///
  UINT32    TempRamInitEntryOffset;
  ///
  /// Byte 0x34: Reserved3.
  ///
  UINT32    Reserved3;
  ///
  /// Byte 0x38: The offset for the API to inform the FSP about the different stages in the boot process.
  ///
  UINT32    NotifyPhaseEntryOffset;
  ///
  /// Byte 0x3C: The offset for the API to initialize the memory.
  ///
  UINT32    FspMemoryInitEntryOffset;
  ///
  /// Byte 0x40: The offset for the API to tear down temporary RAM.
  ///
  UINT32    TempRamExitEntryOffset;
  ///
  /// Byte 0x44: The offset for the API to initialize the CPU and chipset.
  ///
  UINT32    FspSiliconInitEntryOffset;
  ///
  /// Byte 0x48: Offset for the API for the optional Multi-Phase processor and chipset initialization.
  ///            This value is only valid if FSP HeaderRevision is >= 5.
  ///            If the value is set to 0x00000000, then this API is not available in this component.
  ///
  UINT32    FspMultiPhaseSiInitEntryOffset;
  ///
  /// Byte 0x4C: Extended revision of the FSP binary.
  ///            This value is only valid if FSP HeaderRevision is >= 6.
  ///            ExtendedImageRevision specifies the high-order byte of the revision and build number in the FSP binary revision.
  ///               7 : 0 - High Byte of Build Number
  ///              15 : 8 - High Byte of Revision
  ///            The FSP binary build number can be decoded as follows:
  ///            Build Number = (ExtendedImageRevision[7:0] << 8) | ImageRevision[7:0]
  ///            Revision = (ExtendedImageRevision[15:8] << 8) | ImageRevision[15:8]
  ///            Minor Version = ImageRevision[23:16]
  ///            Major Version = ImageRevision[31:24]
  ///
  UINT16    ExtendedImageRevision;
  ///
  /// Byte 0x4E: Reserved4.
  ///
  UINT16    Reserved4;
  ///
  /// Byte 0x50: Offset for the API for the Multi-Phase memory initialization.
  ///
  UINT32    FspMultiPhaseMemInitEntryOffset;
  ///
  /// Byte 0x54: Offset for the API to initialize SMM.
  ///
  UINT32    FspSmmInitEntryOffset;
} FSP_INFO_HEADER;

///
/// Signature of the FSP Extended Header
///
#define FSP_INFO_EXTENDED_HEADER_SIGNATURE  SIGNATURE_32 ('F', 'S', 'P', 'E')

///
/// FSP Information Extended Header as described in FSP v2.0 Spec section 5.1.2.
///
typedef struct {
  ///
  /// Byte 0x00: Signature ('FSPE') for the FSP Extended Information Header.
  ///
  UINT32    Signature;
  ///
  /// Byte 0x04: Length of the table in bytes, including all additional FSP producer defined data.
  ///
  UINT32    Length;
  ///
  /// Byte 0x08: FSP producer defined revision of the table.
  ///
  UINT8     Revision;
  ///
  /// Byte 0x09: Reserved for future use.
  ///
  UINT8     Reserved;
  ///
  /// Byte 0x0A: FSP producer identification string
  ///
  CHAR8     FspProducerId[6];
  ///
  /// Byte 0x10: FSP producer implementation revision number. Larger numbers are assumed to be newer revisions.
  ///
  UINT32    FspProducerRevision;
  ///
  /// Byte 0x14: Size of the FSP producer defined data (n) in bytes.
  ///
  UINT32    FspProducerDataSize;
  ///
  /// Byte 0x18: FSP producer defined data of size (n) defined by FspProducerDataSize.
  ///
} FSP_INFO_EXTENDED_HEADER;

//
// A generic table search algorithm for additional tables can be implemented with a
// signature search algorithm until a terminator signature 'FSPP' is found.
//
#define FSP_FSPP_SIGNATURE         SIGNATURE_32 ('F', 'S', 'P', 'P')
#define FSP_PATCH_TABLE_SIGNATURE  FSP_FSPP_SIGNATURE

///
/// FSP Patch Table as described in FSP v2.0 Spec section 5.1.5.
///
typedef struct {
  ///
  /// Byte 0x00: FSP Patch Table Signature "FSPP".
  ///
  UINT32    Signature;
  ///
  /// Byte 0x04: Size including the PatchData.
  ///
  UINT16    HeaderLength;
  ///
  /// Byte 0x06: Revision is set to 0x01.
  ///
  UINT8     HeaderRevision;
  ///
  /// Byte 0x07: Reserved for future use.
  ///
  UINT8     Reserved;
  ///
  /// Byte 0x08: Number of entries to Patch.
  ///
  UINT32    PatchEntryNum;
  ///
  /// Byte 0x0C: Patch Data.
  ///
  // UINT32  PatchData[];
} FSP_PATCH_TABLE;

  #pragma pack()

extern EFI_GUID  gFspHeaderFileGuid;

#endif
