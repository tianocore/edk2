/** @file
  Guid for Pcd DataBase Signature.

Copyright (c) 2012 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PCD_DATABASE_SIGNATURE_GUID_H_
#define _PCD_DATABASE_SIGNATURE_GUID_H_

#define PCD_DATA_BASE_SIGNATURE_GUID \
{ 0x3c7d193c, 0x682c, 0x4c14, { 0xa6, 0x8f, 0x55, 0x2d, 0xea, 0x4f, 0x43, 0x7e } }

extern EFI_GUID gPcdDataBaseSignatureGuid;

//
// Common definitions
//
typedef UINT64 SKU_ID;

#define PCD_TYPE_SHIFT        28

#define PCD_TYPE_DATA         (0x0U << PCD_TYPE_SHIFT)
#define PCD_TYPE_HII          (0x8U << PCD_TYPE_SHIFT)
#define PCD_TYPE_VPD          (0x4U << PCD_TYPE_SHIFT)
#define PCD_TYPE_STRING       (0x1U << PCD_TYPE_SHIFT)

#define PCD_TYPE_ALL_SET      (PCD_TYPE_DATA | PCD_TYPE_HII | PCD_TYPE_VPD | PCD_TYPE_STRING)

#define PCD_DATUM_TYPE_SHIFT  24

#define PCD_DATUM_TYPE_POINTER  (0x0U << PCD_DATUM_TYPE_SHIFT)
#define PCD_DATUM_TYPE_UINT8    (0x1U << PCD_DATUM_TYPE_SHIFT)
#define PCD_DATUM_TYPE_UINT16   (0x2U << PCD_DATUM_TYPE_SHIFT)
#define PCD_DATUM_TYPE_UINT32   (0x4U << PCD_DATUM_TYPE_SHIFT)
#define PCD_DATUM_TYPE_UINT64   (0x8U << PCD_DATUM_TYPE_SHIFT)

#define PCD_DATUM_TYPE_ALL_SET  (PCD_DATUM_TYPE_POINTER | \
                                 PCD_DATUM_TYPE_UINT8   | \
                                 PCD_DATUM_TYPE_UINT16  | \
                                 PCD_DATUM_TYPE_UINT32  | \
                                 PCD_DATUM_TYPE_UINT64)

#define PCD_DATUM_TYPE_SHIFT2 20

#define PCD_DATUM_TYPE_UINT8_BOOLEAN (0x1U << PCD_DATUM_TYPE_SHIFT2)

#define PCD_DATABASE_OFFSET_MASK (~(PCD_TYPE_ALL_SET | PCD_DATUM_TYPE_ALL_SET | PCD_DATUM_TYPE_UINT8_BOOLEAN))

typedef struct  {
  UINT32  ExTokenNumber;
  UINT16  TokenNumber;          // Token Number for Dynamic-Ex PCD.
  UINT16  ExGuidIndex;          // Index of GuidTable in units of GUID.
} DYNAMICEX_MAPPING;

typedef struct {
  UINT32  StringIndex;          // Offset in String Table in units of UINT8.
  UINT32  DefaultValueOffset;   // Offset of the Default Value.
  UINT16  GuidTableIndex;       // Offset in Guid Table in units of GUID.
  UINT16  Offset;               // Offset in Variable.
  UINT32  Attributes;           // Variable attributes.
  UINT16  Property;             // Variable property.
  UINT16  Reserved;
} VARIABLE_HEAD;

typedef struct {
  UINT32  Offset;
} VPD_HEAD;

typedef UINT32 STRING_HEAD;

typedef UINT16 SIZE_INFO;

typedef struct {
  UINT32  TokenSpaceCNameIndex; // Offset in String Table in units of UINT8.
  UINT32  PcdCNameIndex;        // Offset in String Table in units of UINT8.
} PCD_NAME_INDEX;

typedef UINT32 TABLE_OFFSET;

typedef struct {
    GUID                  Signature;            // PcdDataBaseGuid.
    UINT32                BuildVersion;
    UINT32                Length;               // Length of DEFAULT SKU PCD DB
    SKU_ID                SystemSkuId;          // Current SkuId value.
    UINT32                LengthForAllSkus;     // Length of all SKU PCD DB
    UINT32                UninitDataBaseSize;   // Total size for PCD those default value with 0.
    TABLE_OFFSET          LocalTokenNumberTableOffset;
    TABLE_OFFSET          ExMapTableOffset;
    TABLE_OFFSET          GuidTableOffset;
    TABLE_OFFSET          StringTableOffset;
    TABLE_OFFSET          SizeTableOffset;
    TABLE_OFFSET          SkuIdTableOffset;
    TABLE_OFFSET          PcdNameTableOffset;
    UINT16                LocalTokenCount;      // LOCAL_TOKEN_NUMBER for all.
    UINT16                ExTokenCount;         // EX_TOKEN_NUMBER for DynamicEx.
    UINT16                GuidTableCount;       // The Number of Guid in GuidTable.
    UINT8                 Pad[6];               // Pad bytes to satisfy the alignment.

    //
    // Default initialized external PCD database binary structure
    //
    // Padding is needed to keep necessary alignment
    //
    //SKU_ID                         SkuIdTable[];            // SkuIds system supports.
    //UINT64                         ValueUint64[];
    //UINT32                         ValueUint32[];
    //VPD_HEAD                       VpdHead[];               // VPD Offset
    //DYNAMICEX_MAPPING              ExMapTable[];            // DynamicEx PCD mapped to LocalIndex in LocalTokenNumberTable. It can be accessed by the ExMapTableOffset.
    //UINT32                         LocalTokenNumberTable[]; // Offset | DataType | PCD Type. It can be accessed by LocalTokenNumberTableOffset.
    //GUID                           GuidTable[];             // GUID for DynamicEx and HII PCD variable Guid. It can be accessed by the GuidTableOffset.
    //STRING_HEAD                    StringHead[];            // String PCD
    //PCD_NAME_INDEX                 PcdNameTable[];          // PCD name index info. It can be accessed by the PcdNameTableOffset.
    //VARIABLE_HEAD                  VariableHead[];          // HII PCD
    //UINT8                          StringTable[];           // String for String PCD value and HII PCD Variable Name. It can be accessed by StringTableOffset.
    //SIZE_INFO                      SizeTable[];             // MaxSize and CurSize for String PCD. It can be accessed by SizeTableOffset.
    //UINT16                         ValueUint16[];
    //UINT8                          ValueUint8[];
    //BOOLEAN                        ValueBoolean[];

} PCD_DATABASE_INIT;

//
// PEI and DXE Pcd driver use the same PCD database
//
typedef PCD_DATABASE_INIT  PEI_PCD_DATABASE;
typedef PCD_DATABASE_INIT  DXE_PCD_DATABASE;


typedef struct {
  PEI_PCD_DATABASE  *PeiDb;
  DXE_PCD_DATABASE  *DxeDb;
} PCD_DATABASE;

typedef struct {
  UINT32 Offset:24;
  UINT32 Value:8;
} PCD_DATA_DELTA;

typedef struct {
  SKU_ID SkuId;
  UINT16 DefaultId;
  UINT8  Reserved[6];
} PCD_DEFAULT_INFO;

typedef struct {
  //
  // Full size, it must be at 8 byte alignment.
  //
  UINT32 DataSize;
  //
  // HeaderSize includes HeaderSize fields and DefaultInfo arrays
  //
  UINT32 HeaderSize;
  //
  // DefaultInfo arrays those have the same default setting.
  //
  PCD_DEFAULT_INFO DefaultInfo[1];
  //
  // Default data is stored as variable storage or the array of DATA_DELTA.
  //
} PCD_DEFAULT_DATA;

#define PCD_NV_STORE_DEFAULT_BUFFER_SIGNATURE SIGNATURE_32('N', 'S', 'D', 'B')

typedef struct {
  //
  // PCD_NV_STORE_DEFAULT_BUFFER_SIGNATURE
  //
  UINT32    Signature;
  //
  // Length of the taken default buffer
  //
  UINT32    Length;
  //
  // Length of the total reserved buffer
  //
  UINT32    MaxLength;
  //
  // Reserved for 8 byte alignment
  //
  UINT32    Reserved;
  // one or more PCD_DEFAULT_DATA
} PCD_NV_STORE_DEFAULT_BUFFER_HEADER;

//
// NvStoreDefaultValueBuffer layout:
// +-------------------------------------+
// | PCD_NV_STORE_DEFAULT_BUFFER_HEADER  |
// +-------------------------------------+
// | PCD_DEFAULT_DATA (DEFAULT, Standard)|
// +-------------------------------------+
// | PCD_DATA_DELTA   (DEFAULT, Standard)|
// +-------------------------------------+
// | ......                              |
// +-------------------------------------+
// | PCD_DEFAULT_DATA (SKU A, Standard)  |
// +-------------------------------------+
// | PCD_DATA_DELTA   (SKU A, Standard)  |
// +-------------------------------------+
// | ......                              |
// +-------------------------------------+
//

#pragma pack(1)
typedef struct {
  SKU_ID    SkuId;
  SKU_ID    SkuIdCompared;
  UINT32    Length;
  // PCD_DATA_DELTA   DeltaData[]
} PCD_DATABASE_SKU_DELTA;

//
// PCD database layout:
// +---------------------------------+
// | PCD_DATABASE_INIT (DEFAULT SKU) |
// +---------------------------------+
// | PCD_DATABASE_SKU_DELTA (SKU A)  |
// +---------------------------------+
// | PCD_DATABASE_SKU_DELTA (SKU B)  |
// +---------------------------------+
// | ......                          |
// +---------------------------------+
//
#pragma pack()

#endif
