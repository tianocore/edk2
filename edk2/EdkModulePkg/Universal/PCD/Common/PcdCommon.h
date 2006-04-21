/** @file
Common functions used by PCD PEIM and PCD DXE.

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


Module Name: PcdCommon.h

**/

#ifndef __PCD_COMMON_H__
#define __PCD_COMMON_H__

//
// Enumeration for PCD_DATA_TYPE
//
typedef enum {
  PcdByte8,
  PcdByte16,
  PcdByte32,
  PcdByte64,
  PcdPointer,
  PcdBoolean
} PCD_DATA_TYPE;


//
// The definitions for Global PCD Length Fields
//
#define PCD_LENGTH_BIT8   0x01
#define PCD_LENGTH_BIT16  0x02
#define PCD_LENGTH_BIT24  0x03
#define PCD_LENGTH_BIT32  0x04



/*
 * This data structure is used in <PCD_IMAGE> transverse
 */
typedef struct {
  UINTN                EntryCount;
  UINTN                GlobalOffsetLength;
  UINTN                GlobalTokenLength;
  UINTN                GlobalGuidTabIdxLength;
  UINTN                GlobalDatumLength;
  UINTN                GlobalStrTabIdxLength;
  
  CONST UINT8          *DataDefaultStart;
  UINTN                DataDefaultLength;
  UINTN                WholeDataDefaultLength;
  CONST UINT8          *IndexStart;
  UINTN                IndexLength;
  CONST GUID           *GuidTableStart;
  UINTN                GuidTableLength;
  CONST UINT16         *StringTableStart;
  UINTN                StringTableLength;
  /* Length of the <PCD_IMAGE> in byte.
     This info is from Section header
     in FFS */
  UINTN               ImageLength;
  CONST UINT8         *ImageStart;

} PCD_IMAGE_RECORD;



typedef struct {
  BOOLEAN       HiiEnable;
  BOOLEAN       SkuEnable;
  BOOLEAN       VpdEnable;
  BOOLEAN       SkuDataArrayEnable;
  PCD_DATA_TYPE DataType;
  BOOLEAN       ExtendedGuidPresent;
} PCD_STATEBYTE;



typedef struct  {
  //
  // All Pointer's Offset in byte
  //
  UINT32           TokenNumber;
  PCD_STATEBYTE    StateByte;
  UINT32           HiiData;
  UINT32           SkuIdArray;   //Pointer
  UINT32           ExtendedDataOffset;
  UINT32           DatumSize;
  UINT16           DynamicExGuid; //Pointer
  UINT8            SkuCount;
} PCD_INDEX;



/* 
 * PCD Image Definition according PCD Specification 0.51. 
 *
 */
#pragma pack(1)
typedef struct {
  UINT8 ImageLength[3]; 
  //
  // The length of PCD_FFS_ENCODING is included
  // in ImageLength
  //
  UINT8 DataBufferLength[3];
  UINT8 WholeDataBufferLength[3];
  UINT8 PcdIndexLength[3];
  UINT8 GuidTableLength[3];
  //
  // The StringTable can be computed using:
  // ImageLength, DataBufferLength, PcdIndexLength, GuidTableLength,
  // and length of PCD_FFS_ENCODING
  //
  UINT8 EntryCount[3];
  UINT8 GlobalOffsetLength[1];
  UINT8 GlobalTokenLength[1];
  UINT8 GuidLength[1];
  UINT8 GlobalDatumLength[1];
  UINT8 GlobalStrTabIdxLength[1];
} PCD_FFS_ENCODING;
#pragma pack()


  
typedef struct {
  UINTN      DatabaseLen;
  UINTN      EntryCount;
  UINTN      InfoLength;
  UINTN      GuidTableOffset;
  UINTN      PcdIndexOffset;
  UINTN      StringTableOffset;
  UINTN      CallbackTableOffset;
  UINTN      ImageIndexOffset;
  UINTN      DataBufferOffset;
  UINTN      MaxCallbackNum;
  UINTN      HiiVariableOffsetLength;
  UINTN      HiiGuidOffsetLength;
  UINTN      ExtendedOffsetLength;
  UINT8      *VpdStart;
  UINTN      SkuId;
} PCD_DATABASE_HEADER;



typedef struct {
  PCD_DATABASE_HEADER Info;
  EFI_GUID            GuidTable[1];
} PCD_DATABASE;

extern EFI_GUID gPcdDataBaseHobGuid;


/**
  The function returns the actual address of item in the PCD
  database according to its Segment and Offset.

  @param[out] Offset        The offset within the segment.
  @param[in]  SegmentStart  The starting address of the segment.
  @param[in]  DatabaseStart The base address of the PCD DataBase.
  

  @retval EFI_SUCESS If data value is found according to SKU_ID.
  @retval EFI_NOT_FOUND If not such a value is found.

--*/
UINT8 *
GetAbsoluteAddress (
  IN UINTN        Offset, 
  IN UINTN        SegmentStart, 
  IN CONST VOID   *Base
  )
;



/**
  The function return the number of Unicode Character in a NULL terminated string.
  The NULL is  NOT counted.
  
  @param[in] String The unicode string starts from an unaligned address.
  
  @retval UINTN     The number of Unicode characters.
--*/
UINTN
GetUnalignedStrLen (
  UINT16 *String
);


/**
  The function retrieves the PCD data value according to 
  TokenNumber and Guid space given.

  @param[in]  Info        The PCD Database Info.
  @param[in]  TokenNumber The token number.
  @param[in]  Guid        The Guid space.
  @param[in]  Type        The storage type.
  @param[out] Data        The output data.

  @retval VOID

--*/
VOID
GetPcdEntryWorker (
  IN     CONST PCD_DATABASE_HEADER  *Info,
  IN     UINTN                      TokenNumber,
  IN     CONST GUID                 *Guid,  OPTIONAL
  IN     PCD_DATA_TYPE              Type,
  OUT    VOID                       *Data
  )
;



/**
  The function retrieves the PCD data value according to 
  TokenNumber and Guid space given.

  @param[in]  Info        The PCD Database info.
  @param[in]  TokenNumber The token number.
  @param[in]  Guid        The Guid space.

  @retval     UINTN       The size of the PCD Entry.

--*/
UINTN
GetPcdEntrySizeWorker (
  IN CONST PCD_DATABASE_HEADER  *Info,
  IN UINTN                      TokenNumber,
  IN CONST GUID                 *Guid  OPTIONAL
  )
;



/**
  The function looks for the next PCD ENTRY.
  If *TokenNumber is 0, the first TokenNumber in
  the GUID token space is return.
  If there is no next TokenNumber found,
  *TokenNumber will be 0.

  @param[in]      Info              The PCD Database info.
  @param[in,out]  TokenNumber       The token number.
  @param[in]      Guid              The Guid space.
  
  @retval     EFI_NOT_FOUND     Can't find the PCD_ENTRY.
  @retval     EFI_SUCCESS       Operation succesful.

--*/
EFI_STATUS
GetNextTokenWorker (
  IN CONST PCD_DATABASE_HEADER    *Info,
  IN OUT UINTN                    *TokenNumber,
  IN CONST GUID                   *Guid     OPTIONAL
  )
;



/**
  The function is the worker function to set the data of a PCD entry.
  
  @param[in] PcdIndex The PCD Index.
  @param[in] Info     The attributes of the PCD database.
  @param[in] Data     The input data.
  
  @retval VOID
--*/
EFI_STATUS
SetPcdData (
  IN CONST PCD_INDEX           *PcdIndex,
  IN CONST PCD_DATABASE_HEADER *Info,
  IN CONST VOID                *Data
  )
;


/**
  The function is provided by PCD PEIM and PCD DXE driver to
  do the work of reading a HII variable from variable service.
  
  @param[in] VariableGuid     The Variable GUID.
  @param[in] VariableName     The Variable Name.
  @param[out] VariableData    The output data.
  @param[out] VariableSize    The size of the variable.
  
  @retval EFI_SUCCESS         Operation successful.
  @retval EFI_SUCCESS         Variablel not found.
--*/
EFI_STATUS
GetHiiVariable (
  IN  EFI_GUID     *VariableGuid,
  IN  UINT16       *VariableName,
  OUT VOID         **VariableData,
  OUT UINTN        *VariableSize
  )
;



/**
  The function is provided by PCD PEIM and PCD DXE driver to
  do the work of reading a HII variable from variable service.
  
  @param[in] VariableGuid     The Variable GUID.
  @param[in] VariableName     The Variable Name.
  @param[in] Data             The input data.
  @param[out] VariableSize    The size of the variable.
  @param[in] Offset           The offset of the variable data that a PCD entry will starts from.
  
  @retval EFI_SUCCESS         Operation successful.
  @retval EFI_SUCCESS         Variablel not found.
--*/
EFI_STATUS
SetHiiVariable (
  IN  EFI_GUID     *VariableGuid,
  IN  UINT16       *VariableName,
  IN  CONST VOID   *Data,
  IN  UINTN        VariableSize,
  IN  UINTN        Offset
  )
;

/**
  The function locates the PCD_INDEX according to TokeNumber and GUID space given.

  @param[in] TokenNumber The token number.
  @param[in] Guid        The GUID token space.
  @param[out] Info       The attributes of the PCD database.

  @retval PCD_INDEX*     The PCD_INDEX found.
--*/
PCD_INDEX *
FindPcdIndex (
  IN   UINTN                     TokenNumber,
  IN   CONST GUID                *Guid,
  IN   CONST PCD_DATABASE_HEADER *Info,
  OUT  UINTN                     *Index
  )
;

/**
  (WQBUGBUG: You must handle the new SKU_ID encoding.
  The function is the worker function to get the data of a PCD entry.

  @param[in] PcdIndex The PCD Index.
  @param[in] Info     The attributes of the PCD database.
  @param[out] Data    The output data.

  @retval VOID
--*/
UINT8*
GetPcdDataPtr (
  IN CONST PCD_INDEX           *PcdIndex,
  IN CONST PCD_DATABASE_HEADER *Info
  )
;

/**
  The function retrieves the PCD data value according to
  the PCD_DATA_TYPE specified.

  @param[in] Type The PCD_DATA_TYPE used to interpret the data.
  @param[in] InData The input data.
  @param[in] OutData The output data.
  @param[in] Len The length of the data; it is mainly used for PcdPointer type.

  @retval VOID
--*/
VOID
GetDataBasedOnType (
  IN PCD_DATA_TYPE Type,
  IN VOID          *InData,
  OUT VOID         *OutData,
  IN UINTN         Len
  )
;
#endif
