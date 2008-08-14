/** @file
  ElTorito Partitions Format Definition.

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

--*/

#ifndef _ELTORITO_H_
#define _ELTORITO_H_

//
// CDROM_VOLUME_DESCRIPTOR.Types
//
#define CDVOL_TYPE_STANDARD 0x0
#define CDVOL_TYPE_CODED    0x1
#define CDVOL_TYPE_END      0xFF

///
/// CDROM_VOLUME_DESCRIPTOR.Id
///
#define CDVOL_ID  "CD001"

///
/// CDROM_VOLUME_DESCRIPTOR.SystemId
///
#define CDVOL_ELTORITO_ID "EL TORITO SPECIFICATION"

//
// Indicator types
//
#define ELTORITO_ID_CATALOG               0x01
#define ELTORITO_ID_SECTION_BOOTABLE      0x88
#define ELTORITO_ID_SECTION_NOT_BOOTABLE  0x00
#define ELTORITO_ID_SECTION_HEADER        0x90
#define ELTORITO_ID_SECTION_HEADER_FINAL  0x91

//
// ELTORITO_CATALOG.Boot.MediaTypes
//
#define ELTORITO_NO_EMULATION 0x00
#define ELTORITO_12_DISKETTE  0x01
#define ELTORITO_14_DISKETTE  0x02
#define ELTORITO_28_DISKETTE  0x03
#define ELTORITO_HARD_DISK    0x04


#pragma pack(1)

///
/// El Torito Volume Descriptor
/// Note that the CDROM_VOLUME_DESCRIPTOR does not match the ISO-9660
/// descriptor.  For some reason descriptor used by El Torito is
/// different, but they start the same.   The El Torito descriptor
/// is left shifted 1 byte starting with the SystemId.  (Note this
/// causes the field to get unaligned)
///
typedef struct {
  UINT8   Type;
  CHAR8   Id[5];  // CD001
  UINT8   Version;
  CHAR8   SystemId[26];
  CHAR8   Unused[38];
  UINT8   EltCatalog[4];
  CHAR8   Unused2[5];
  UINT32  VolSpaceSize[2];
} CDROM_VOLUME_DESCRIPTOR;

///
/// Catalog Entry
///
typedef union {
  struct {
    CHAR8       Reserved[0x20];
  } Unknown;

  ///
  /// Catalog validation entry (Catalog header)
  ///
  struct {
    UINT8   Indicator;
    UINT8   PlatformId;
    UINT16  Reserved;
    CHAR8   ManufacId[24];
    UINT16  Checksum;
    UINT16  Id55AA;
  } Catalog;

  ///
  /// Initial/Default Entry or Section Entry
  ///
  struct {
    UINT8   Indicator;
    UINT8   MediaType : 4;
    UINT8   Reserved1 : 4;
    UINT16  LoadSegment;
    UINT8   SystemType;
    UINT8   Reserved2;
    UINT16  SectorCount;
    UINT32  Lba;
  } Boot;

  ///
  /// Section Header Entry
  ///
  struct {
    UINT8   Indicator;
    UINT8   PlatformId;
    UINT16  SectionEntries;
    CHAR8   Id[28];
  } Section;

} ELTORITO_CATALOG;

#pragma pack()

#endif
