/** @file
  The common header file for the thunk driver.
  
Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _DATAHUB_TO_SMBIOS_THUNK_
#define _DATAHUB_TO_SMBIOS_THUNK_

#include <FrameworkDxe.h>
#include <IndustryStandard/SmBios.h>

#include <Guid/EventGroup.h>
#include <Guid/SmBios.h>
#include <Protocol/DataHub.h>
#include <Guid/DataHubRecords.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/Smbios.h>

#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/HiiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

//
// Conversion Table that describes the translation method for
// Data Hub Data Records of certain SubClass and RecordNumber
//
typedef enum {
  BySubclassInstanceSubinstanceProducer,
  BySubClassInstanceProducer,
  MaxLocatingMethod
} SMBIOS_STRUCTURE_LOCATING_METHOD;

typedef enum {
  RecordDataUnchangedOffsetSpecified,
  ByFunctionWithOffsetSpecified,
  ByFunction,
  ByFunctionWithWholeDataRecord,
  MaxFieldFillingMethod
} SMBIOS_FIELD_FILLING_METHOD;

typedef struct _SMBIOS_STRUCTURE_NODE SMBIOS_STRUCTURE_NODE;

typedef struct {
  UINT8   Type;
  UINT8   Length;
  UINT16  Handle;
  UINT8   Tailing[2];
} EFI_SMBIOS_TABLE_TYPE127;

typedef
EFI_STATUS
(*SMBIOS_FIELD_FILLING_FUNCTION) (
  IN OUT  SMBIOS_STRUCTURE_NODE                     * StructureNode,
  IN      UINT32                                    Offset          OPTIONAL,
  IN      VOID                                      *RecordData,
  IN      UINT32                                    RecordDataSize
  );

typedef struct {
  //
  // Data Hub Data Record's SubClass and RecordNumber
  //
  EFI_GUID                          SubClass;
  UINT32                            RecordType;

  //
  // Translation method applied
  //
  UINT8                             SmbiosType;
  SMBIOS_STRUCTURE_LOCATING_METHOD  StructureLocatingMethod;
  SMBIOS_FIELD_FILLING_METHOD       FieldFillingMethod;
  UINT32                            FieldOffset;
  SMBIOS_FIELD_FILLING_FUNCTION     FieldFillingFunction;

} SMBIOS_CONVERSION_TABLE_ENTRY;

//
// SMBIOS_LINK_DATA_FIXUP nodes indicate the Link fields that
// need to be fixed up when creating the resulting Smbios image.
//
#define SMBIOS_LINK_DATA_FIXUP_NODE_SIGNATURE SIGNATURE_32 ('S', 'm', 'l', 'n')

typedef struct {

  UINT32              Signature;
  LIST_ENTRY          Link;

  UINT32              Offset;
  UINT8               TargetType;
  EFI_GUID            SubClass;
  EFI_INTER_LINK_DATA LinkData;

} SMBIOS_LINK_DATA_FIXUP_NODE;

//
// The global Structure List node.
// The Structure List is populated as more and more
// Structures (of various types) are discovered and inserted.
// The nodes in the Structure List will be concatenated
// to form the ultimate SMBIOS table.
//
#define SMBIOS_STRUCTURE_NODE_SIGNATURE SIGNATURE_32 ('S', 'm', 'b', 's')

struct _SMBIOS_STRUCTURE_NODE {

  UINT32            Signature;
  LIST_ENTRY        Link;

  //
  // Tags
  //
  EFI_GUID          SubClass;
  UINT16            Instance;
  UINT16            SubInstance;
  EFI_GUID          ProducerName;

  //
  // the Smbios structure
  //
  UINT32            StructureSize;        // Actual structure size including strings

  EFI_SMBIOS_TABLE_HEADER *Structure;
  
  
  EFI_SMBIOS_HANDLE SmbiosHandle;         // Smbios Handle in SMBIOS database.
  
  EFI_SMBIOS_TYPE   SmbiosType;
  
  LIST_ENTRY    LinkDataFixup;
  
};

//
// Smbios type info table. Indicates minimum length
// for each Smbios type as the indicator of the initial size of buffer
// allocated for the structure instance of a specific type.
//
typedef struct {

  UINT8   Type;
  UINT8   MinLength;  // Minimal structure size including
  // TWO trailing bytes of 0x00
  //
  BOOLEAN IsRequired; // Required structure type defined by Smbios Spec
  BOOLEAN IsCreated;  // Created in this run
} SMBIOS_TYPE_INFO_TABLE_ENTRY;

//
// EDK framwork Memory Data hub definition to support EDK/Framework driver.
//
typedef struct {
  STRING_REF                  MemoryDeviceLocator;
  STRING_REF                  MemoryBankLocator;
  STRING_REF                  MemoryManufacturer;
  STRING_REF                  MemorySerialNumber;
  STRING_REF                  MemoryAssetTag;
  STRING_REF                  MemoryPartNumber;
  EFI_INTER_LINK_DATA         MemoryArrayLink;
  EFI_INTER_LINK_DATA         MemorySubArrayLink;
  UINT16                      MemoryTotalWidth;
  UINT16                      MemoryDataWidth;
  UINT64                      MemoryDeviceSize;
  EFI_MEMORY_FORM_FACTOR      MemoryFormFactor;
  UINT8                       MemoryDeviceSet;
  EFI_MEMORY_ARRAY_TYPE       MemoryType;
  EFI_MEMORY_TYPE_DETAIL      MemoryTypeDetail;
  UINT16                      MemorySpeed;
  EFI_MEMORY_STATE            MemoryState;
  UINT8                       MemoryAttributes;
} FRAMEWORK_MEMORY_ARRAY_LINK_DATA;

typedef struct {
  EFI_MEMORY_ARRAY_LOCATION   MemoryArrayLocation;
  EFI_MEMORY_ARRAY_USE        MemoryArrayUse;
  EFI_MEMORY_ERROR_CORRECTION MemoryErrorCorrection;
  UINT32                      MaximumMemoryCapacity;
  UINT16                      NumberMemoryDevices;
} FRAMEWORK_MEMORY_ARRAY_LOCATION_DATA;

//
// Global variables
//
extern SMBIOS_CONVERSION_TABLE_ENTRY  mConversionTable[];
extern SMBIOS_TYPE_INFO_TABLE_ENTRY   mTypeInfoTable[];
extern LIST_ENTRY                     mStructureList;

//
// Function Prototypes
//
/**
  Smbios data filter function. This function is invoked when there is data records
  available in the Data Hub. 

  @param Event         The event that is signaled.
  @param Context       not used here.
**/
VOID
EFIAPI
SmbiosDataFilter (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  );

//
// Function prototypes
//
/**
  Process a datahub's record and find corresponding translation way to translate
  to SMBIOS record.
  
  @param Record  Point to datahub record.
**/
VOID
SmbiosProcessDataRecord (
  IN EFI_DATA_RECORD_HEADER  *Record
  );

/**
  Calculate the minimal length for a SMBIOS type. This length maybe not equal
  to sizeof (SMBIOS_RECORD_STRUCTURE), but defined in conformance chapter in SMBIOS specification.
  
  @param Type  SMBIOS's type.
  
  @return the minimal length of a smbios record.
**/
UINT32
SmbiosGetTypeMinimalLength (
  IN UINT8  Type
  );

/**
  Enlarge the structure buffer of a structure node in SMBIOS database.
  The function maybe lead the structure pointer for SMBIOS record changed.
  
  @param StructureNode The structure node whose structure buffer is to be enlarged.
  @param NewLength     The new length of SMBIOS record which does not include unformat area.
  @param OldBufferSize The old size of SMBIOS record buffer.
  @param NewBufferSize The new size is targeted for enlarged.
  
  @retval EFI_OUT_OF_RESOURCES  No more memory to allocate new record
  @retval EFI_SUCCESS           Success to enlarge the record buffer size.
**/
EFI_STATUS
SmbiosEnlargeStructureBuffer (
  IN OUT  SMBIOS_STRUCTURE_NODE *StructureNode,
  UINT8                         NewLength,
  UINTN                         OldBufferSize,
  UINTN                         NewBufferSize
  );

/**
  Update the structure buffer of a structure node in SMBIOS database.
  The function lead the structure pointer for SMBIOS record changed.
  
  @param StructureNode The structure node whose structure buffer is to be enlarged.
  @param NewRecord     The new SMBIOS record.
  
**/
VOID
SmbiosUpdateStructureBuffer (
  IN OUT  SMBIOS_STRUCTURE_NODE *StructureNode,
  IN EFI_SMBIOS_TABLE_HEADER    *NewRecord
  );

/**
  Fill a standard Smbios string field. 
  
  This function will convert the unicode string to single byte chars, and only
  English language is supported.
  This function changes the Structure pointer value of the structure node, 
  which should be noted by Caller.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_INVALID_PARAMETER   RecordDataSize is too larger
  @retval EFI_OUT_OF_RESOURCES    No memory to allocate new buffer for string
  @retval EFI_SUCCESS             Sucess append string for a SMBIOS record.
**/
EFI_STATUS
SmbiosFldString (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Fill the inter link field for a SMBIOS recorder.
  
  Some SMBIOS recorder need to reference the handle of another SMBIOS record. But
  maybe another SMBIOS record has not been added, so put the InterLink request into
  a linked list and the interlink will be fixedup when a new SMBIOS record is added.
  
  @param StructureNode        Point to SMBIOS_STRUCTURE_NODE which reference another record's handle
  @param LinkSmbiosNodeOffset The offset in this record for holding the handle of another SMBIOS record
  @param LinkSmbiosType       The type of SMBIOS record want to be linked.
  @param InterLink            Point to EFI_INTER_LINK_DATA will be put linked list.
  @param SubClassGuid         The guid of subclass for linked SMBIOS record.
  
  @retval EFI_SUCESS  The linked record is found and no need fixup in future.
  @retval !EFI_SUCESS The linked record can not be found and InterLink is put a fixing-p linked list.
**/
EFI_STATUS
SmbiosFldInterLink (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT16                    LinkSmbiosNodeOffset,
  IN      UINT8                     LinkSmbiosType,
  IN      EFI_INTER_LINK_DATA       *InterLink,
  IN      EFI_GUID                  *SubClassGuid
  );

/**
  Find a handle that matches the Link Data and the target Smbios type.
  
  @param  TargetType     the Smbios type
  @param  SubClass       the SubClass
  @param  LinkData       Specifies Instance, SubInstance and ProducerName
  @param  Handle         the HandleNum found  
  
  @retval EFI_NOT_FOUND Can not find the record according to handle
  @retval EFI_SUCCESS   Success to find the handle
**/
EFI_STATUS
SmbiosFindHandle (
  IN      UINT8               TargetType,
  IN      EFI_GUID            *SubClass,
  IN      EFI_INTER_LINK_DATA *LinkData,
  IN OUT  UINT16              *HandleNum
  );

/**
  Field Filling Function. Transform an EFI_EXP_BASE10_DATA to a word, with 'Mega'
  as the unit.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_INVALID_PARAMETER  RecordDataSize is invalid. 
  @retval EFI_SUCCESS            RecordData is successed to be filled into given SMBIOS record.
**/
EFI_STATUS
SmbiosFldBase10ToWordWithMega (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function. Transform an EFI_EXP_BASE2_DATA to a word, with 'Kilo'
  as the unit. Granularity implemented for Cache Size.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_INVALID_PARAMETER  RecordDataSize is invalid. 
  @retval EFI_SUCCESS            RecordData is successed to be filled into given SMBIOS record.
**/
EFI_STATUS
SmbiosFldBase2ToWordWithKilo (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function: truncate record data to byte and fill in the
  field as indicated by Offset.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_INVALID_PARAMETER  RecordDataSize is invalid. 
  @retval EFI_SUCCESS            RecordData is successed to be filled into given SMBIOS record.
**/
EFI_STATUS
SmbiosFldTruncateToByte (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Processor SubClass record type 6 -- ProcessorID.
  Offset is mandatory.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldProcessorType6 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Processor SubClass record type 9 -- Voltage.
  Offset is mandatory.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldProcessorType9 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Processor SubClass record type 17 -- Cache association.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldProcessorType17 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function. Transform an EFI_EXP_BASE2_DATA to a word, with 10exp-9
  as the unit.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_INVALID_PARAMETER  RecordDataSize is invalid. 
  @retval EFI_SUCCESS            RecordData is successed to be filled into given SMBIOS record.
**/
EFI_STATUS
SmbiosFldBase10ToByteWithNano (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function: truncate record data to byte and fill in the
  field as indicated by Offset.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_INVALID_PARAMETER  RecordDataSize is invalid. 
  @retval EFI_SUCCESS            RecordData is successed to be filled into given SMBIOS record.
**/
EFI_STATUS
SmbiosFldTruncateToWord (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Cache SubClass record type 10 -- Cache Config.
  Offset is mandatory

  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldCacheType10 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Check if OEM structure has included 2 trailing 0s in data record.
  
  @param RecordData       Point to record data will be checked.
  @param RecordDataSize   The size of record data.
  
  @retval 0    2 trailing 0s exist in unformatted section
  @retval 1    1 trailing 0 exists at the end of unformatted section
  @retval -1   There is no 0 at the end of unformatted section
**/
INT8
SmbiosCheckTrailingZero (
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Cache SubClass record type 5&6 -- Cache SRAM type.
  Offset is mandatory
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldCacheType5 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Memory SubClass record type 2 -- Physical Memory
  Array.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMemoryType2 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Memory SubClass record type 3 -
  - Memory Device: SMBIOS Type 17
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMemoryType3 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Memory SubClass record type 4
  -- Memory Array Mapped Address: SMBIOS Type 19
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMemoryType4 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Memory SubClass record type 5
  -- Memory Device Mapped Address: SMBIOS Type 20
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMemoryType5 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Memory SubClass record type 6
  -- Memory Channel Type: SMBIOS Type 37
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMemoryType6 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Memory SubClass record type 7
  -- Memory Channel Device: SMBIOS Type 37
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMemoryType7 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Memory SubClass record type 8
  -- Memory Controller information: SMBIOS Type 5
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMemoryType8 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Memory SubClass record type 
  -- Memory 32 Bit Error Information: SMBIOS Type 18
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMemoryType9 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Memory SubClass record type 
  -- Memory 64 Bit Error Information: SMBIOS Type 33
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMemoryType10 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 0 -- Bios Information.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType0 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function. Transform an EFI_EXP_BASE2_DATA to a byte, with '64k'
  as the unit.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_INVALID_PARAMETER  RecordDataSize is invalid. 
  @retval EFI_SUCCESS            RecordData is successed to be filled into given SMBIOS record.
**/
EFI_STATUS
SmbiosFldBase2ToByteWith64K (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 1 -- System Information.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType1 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for record type 2 -- Base Board Manufacture.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType2 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 3 -
  - System Enclosure or Chassis.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType3 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 8 -- Port Connector.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType8 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 9 -- System slot.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType9 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 10 - Onboard Device.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType10 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 11 - OEM Strings.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType11 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 12 - System Options.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType12 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 13 - BIOS Language.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType13 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 14 - System Language String
  Current solution assumes that EFI_MISC_SYSTEM_LANGUAGE_STRINGs are logged with
  their LanguageId having ascending orders.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType14 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 15 -- System Event Log.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType15 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 21 - Pointing Device.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType21 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );


/**
  Field Filling Function for Misc SubClass record type 32 -- System Boot Information.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType32 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 38 -- IPMI device info.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType38 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 0x80-0xFF -- OEM.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscTypeOEM (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Memory SubClass record type 3 -
  - Memory Device: SMBIOS Type 6
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldSMBIOSType6 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 22 - Portable Battery.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType22 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 22 - Portable Battery.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType22 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 23 - System Reset.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType23 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 24 - Hardware Security.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType24 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 25 - System Power Controls.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType25 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 26 - Voltage Probe.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType26 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 27 - Cooling Device.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType27 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 28 -- Temperature Probe.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType28 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 29 -- Electrical Current Probe.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType29 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 30 -- Out-of-Band Remote Access.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType30 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 34 -- Management Device.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType34 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 35 -- Management Device Component.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType35 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 36 -- Management Device Threshold.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType36 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 38 -- IPMI device info.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType38 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 39 -- Power supply.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType39 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

/**
  Field Filling Function for Misc SubClass record type 127 - End-of-Table.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType127 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );
    
/**
  Create a blank smbios record. The datahub record is only a field of smbios record.
  So before fill any field from datahub's record. A blank smbios record need to be 
  created.
  
  @param ProducerHandle   The produce handle for a datahub record
  @param StructureNode    Point to SMBIOS_STRUCTURE_NODE
  
  @retval EFI_OUT_OF_RESOURCES Fail to allocate memory for new blank SMBIOS record.
  @retval EFI_SUCCESS          Success to create blank smbios record.
**/
EFI_STATUS
SmbiosProtocolCreateRecord (
  IN      EFI_HANDLE              ProducerHandle, OPTIONAL
  IN      SMBIOS_STRUCTURE_NODE   *StructureNode
  );
  
/**
  Get pointer of EFI_SMBIOS_PROTOCOL.
  
  @return pointer of EFI_SMBIOS_PROTOCOL.
**/
EFI_SMBIOS_PROTOCOL*
GetSmbiosProtocol (
  VOID
  );
    
/**
  Get pointer of a SMBIOS record's buffer according to its handle.
  
  @param Handle         The handle of SMBIOS record want to be searched.
  @param Type           The type of SMBIOS record want to be searched.
  @param ProducerHandle The producer handle of SMBIOS record.
  
  @return EFI_SMBIOS_TABLE_HEADER Point to a SMBIOS record's buffer.
**/  
EFI_SMBIOS_TABLE_HEADER*
GetSmbiosBufferFromHandle (
  IN  EFI_SMBIOS_HANDLE  Handle,
  IN  EFI_SMBIOS_TYPE    Type,
  IN  EFI_HANDLE         ProducerHandle  OPTIONAL
  );
          
/**

  Get the full size of smbios structure including optional strings that follow the formatted structure.

  @param Head                   Pointer to the beginning of smbios structure.
  @param Size                   The returned size.
  @param NumberOfStrings        The returned number of optional strings that follow the formatted structure.

  @retval EFI_SUCCESS           Size retured in Size.
  @retval EFI_INVALID_PARAMETER Input smbios structure mal-formed or Size is NULL.
  
**/
EFI_STATUS
EFIAPI
GetSmbiosStructureSize (
  IN   EFI_SMBIOS_TABLE_HEADER          *Head,
  OUT  UINT32                           *Size,
  OUT  UINT8                            *NumberOfStrings
  );
            
#endif
