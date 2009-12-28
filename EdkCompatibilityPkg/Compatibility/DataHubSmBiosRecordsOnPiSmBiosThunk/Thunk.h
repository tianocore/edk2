/** @file
  The common header file for the thunk driver.
  
Copyright (c) 2009 Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
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
  BY_SUBCLASS_INSTANCE_SUBINSTANCE_PRODUCER,
  BY_SUBCLASS_INSTANCE_PRODUCER,
  MAX_LOCATING_METHOD
} SMBIOS_STRUCTURE_LOCATING_METHOD;

typedef enum {
  RECORD_DATA_UNCHANGED_OFFSET_SPECIFIED,
  BY_FUNCTION_WITH_OFFSET_SPECIFIED,
  BY_FUNCTION,
  BY_FUNCTION_WITH_WHOLE_DATA_RECORD,
  MAX_FIELD_FILLING_METHOD
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
  )
;

/**
  Calculate the minimal length for a SMBIOS type. This length maybe not equal
  to sizeof (SMBIOS_RECORD_STRUCTURE), but defined in conformance chapter in SMBIOS specification.
  
  @param Type  SMBIOS's type.
  
  @return the minimal length of a smbios record.
**/
UINT32
SmbiosGetTypeMinimalLength (
  IN UINT8  Type
  )
;

/**
  Enlarge the structure buffer of a structure node in SMBIOS database.
  The function maybe lead the structure pointer for SMBIOS record changed.
  
  @param StructureNode The structure node whose structure buffer is to be enlarged.
  @param NewLength     The new length of SMBIOS record which does not include unformat area.
  @param OldBufferSize The old size of SMBIOS record buffer.
  @param NewSize       The new size is targeted for enlarged.
  
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
  Field Filling Function. Fill a standard Smbios string field. 
  Convert the unicode string to single byte chars.
  Only English language is supported.

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
  )
;

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
  )
;

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
  )
;

EFI_STATUS
SmbiosFldBase10ToWordWithMega (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldBase2ToWordWithKilo (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldTruncateToByte (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldProcessorType6 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldProcessorType9 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldProcessorType17 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldBase10ToByteWithNano (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldTruncateToWord (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldCacheType10 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

INT8
SmbiosCheckTrailingZero (
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldCacheType5 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMemoryType2 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMemoryType3 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMemoryType4 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMemoryType5 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMemoryType6 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMemoryType7 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMemoryType8 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMemoryType9 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMemoryType10 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType0 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldBase2ToByteWith64K (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType1 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType2 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType3 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType8 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType9 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType10 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType11 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType12 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType13 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType14 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType15 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType21 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;


EFI_STATUS
SmbiosFldMiscType32 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType38 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  );

EFI_STATUS
SmbiosFldMiscTypeOEM (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldSMBIOSType6 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset, 
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType22 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType22 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType23 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType24 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType25 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType26 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType27 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType28 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType29 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType30 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType34 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType36 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType38 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType39 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;

EFI_STATUS
SmbiosFldMiscType127 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
;
    
EFI_STATUS
SmbiosProtocolCreateRecord (
  IN      EFI_HANDLE              ProducerHandle, OPTIONAL
  IN      SMBIOS_STRUCTURE_NODE   *StructureNode
  );
  
EFI_SMBIOS_PROTOCOL*
GetSmbiosProtocol (
  VOID
  );
    
EFI_SMBIOS_TABLE_HEADER*
GetSmbiosBufferFromHandle (
  IN  EFI_SMBIOS_HANDLE  Handle,
  IN  EFI_SMBIOS_TYPE    Type,
  IN  EFI_HANDLE         ProducerHandle  OPTIONAL
  );
          
EFI_STATUS
EFIAPI
GetSmbiosStructureSize (
  IN   EFI_SMBIOS_TABLE_HEADER          *Head,
  OUT  UINT32                           *Size,
  OUT  UINT8                            *NumberOfStrings
  );
            
#endif
