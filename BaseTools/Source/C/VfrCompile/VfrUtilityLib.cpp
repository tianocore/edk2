/** @file
  
  Vfr common library functions.

Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "stdio.h"
#include "stdlib.h"
#include "CommonLib.h"
#include "VfrUtilityLib.h"
#include "VfrFormPkg.h"

VOID
CVfrBinaryOutput::WriteLine (
  IN FILE         *pFile,
  IN UINT32       LineBytes,
  IN CONST CHAR8  *LineHeader,
  IN CHAR8        *BlkBuf,
  IN UINT32       BlkSize
  )
{
  UINT32    Index;

  if ((pFile == NULL) || (LineHeader == NULL) || (BlkBuf == NULL)) {
    return;
  }

  for (Index = 0; Index < BlkSize; Index++) {
    if ((Index % LineBytes) == 0) {
      fprintf (pFile, "\n%s", LineHeader);
    }
    fprintf (pFile, "0x%02X,  ", (UINT8)BlkBuf[Index]);
  }
}

VOID
CVfrBinaryOutput::WriteEnd (
  IN FILE         *pFile,
  IN UINT32       LineBytes,
  IN CONST CHAR8  *LineHeader,
  IN CHAR8        *BlkBuf,
  IN UINT32       BlkSize
  )
{
  UINT32    Index;

  if ((BlkSize == 0) || (pFile == NULL) || (LineHeader == NULL) || (BlkBuf == NULL)) {
    return;
  }

  for (Index = 0; Index < BlkSize - 1; Index++) {
    if ((Index % LineBytes) == 0) {
      fprintf (pFile, "\n%s", LineHeader);
    }
    fprintf (pFile, "0x%02X,  ", (UINT8)BlkBuf[Index]);
  }

  if ((Index % LineBytes) == 0) {
    fprintf (pFile, "\n%s", LineHeader);
  }
  fprintf (pFile, "0x%02X\n", (UINT8)BlkBuf[Index]);
}

SConfigInfo::SConfigInfo (
  IN UINT8              Type, 
  IN UINT16             Offset, 
  IN UINT32             Width, 
  IN EFI_IFR_TYPE_VALUE Value
  )
{
  mNext   = NULL;
  mOffset = Offset;
  mWidth  = (UINT16)Width;
  mValue  = new UINT8[mWidth];
  if (mValue == NULL) {
    return;
  }

  switch (Type) {
  case EFI_IFR_TYPE_NUM_SIZE_8 :
    memcpy (mValue, &Value.u8, mWidth);
    break;
  case EFI_IFR_TYPE_NUM_SIZE_16 :
    memcpy (mValue, &Value.u16, mWidth);
    break;
  case EFI_IFR_TYPE_NUM_SIZE_32 :
    memcpy (mValue, &Value.u32, mWidth);
    break;
  case EFI_IFR_TYPE_NUM_SIZE_64 :
    memcpy (mValue, &Value.u64, mWidth);
    break;
  case EFI_IFR_TYPE_BOOLEAN :
    memcpy (mValue, &Value.b, mWidth);
    break;
  case EFI_IFR_TYPE_TIME :
    memcpy (mValue, &Value.time, mWidth);
    break;
  case EFI_IFR_TYPE_DATE :
    memcpy (mValue, &Value.date, mWidth);
    break;
  case EFI_IFR_TYPE_STRING :
    memcpy (mValue, &Value.string, mWidth);
    break;
  case EFI_IFR_TYPE_BUFFER :
    memcpy (mValue, &Value.u8, mWidth);
    break;

  case EFI_IFR_TYPE_OTHER :
    return;
  }
}

SConfigInfo::~SConfigInfo (
  VOID
  )
{
  BUFFER_SAFE_FREE (mValue);
}

SConfigItem::SConfigItem (
  IN CHAR8               *Name,
  IN EFI_GUID            *Guid,
  IN CHAR8               *Id
  )
{
  mName          = NULL;
  mGuid          = NULL;
  mId            = NULL;
  mInfoStrList = NULL;
  mNext        = NULL;

  if (Name != NULL) {
    if ((mName = new CHAR8[strlen (Name) + 1]) != NULL) {
      strcpy (mName, Name);
    }
  }

  if (Guid != NULL) {
    if ((mGuid = (EFI_GUID *) new CHAR8[sizeof (EFI_GUID)]) != NULL) {
      memcpy (mGuid, Guid, sizeof (EFI_GUID));
    }
  }

  if (Id != NULL) {
    if ((mId = new CHAR8[strlen (Id) + 1]) != NULL) {
      strcpy (mId, Id);
    }
  }
}

SConfigItem::SConfigItem (
  IN CHAR8               *Name,
  IN EFI_GUID            *Guid,
  IN CHAR8               *Id,
  IN UINT8               Type,
  IN UINT16              Offset,
  IN UINT16              Width,
  IN EFI_IFR_TYPE_VALUE  Value
  )
{
  mName        = NULL;
  mGuid        = NULL;
  mId          = NULL;
  mInfoStrList = NULL;
  mNext        = NULL;

  if (Name != NULL) {
    if ((mName = new CHAR8[strlen (Name) + 1]) != NULL) {
      strcpy (mName, Name);
    }
  }

  if (Guid != NULL) {
    if ((mGuid = (EFI_GUID *) new CHAR8[sizeof (EFI_GUID)]) != NULL) {
      memcpy (mGuid, Guid, sizeof (EFI_GUID));
    }
  }

  if (Id != NULL) {
    if ((mId = new CHAR8[strlen (Id) + 1]) != NULL) {
      strcpy (mId, Id);
    }
  }

  mInfoStrList = new SConfigInfo(Type, Offset, Width, Value);
}

SConfigItem::~SConfigItem (
  VOID
  )
{
  SConfigInfo  *Info;

  BUFFER_SAFE_FREE (mName);
  BUFFER_SAFE_FREE (mGuid);
  BUFFER_SAFE_FREE (mId);
  while (mInfoStrList != NULL) {
    Info = mInfoStrList;
    mInfoStrList = mInfoStrList->mNext;

    BUFFER_SAFE_FREE (Info);
  }
}

UINT8
CVfrBufferConfig::Register (
  IN CHAR8               *Name,
  IN EFI_GUID            *Guid,
  IN CHAR8               *Id
  )
{
  SConfigItem *pNew;

  if (Select (Name, Guid) == 0) {
    return 1;
  }

  if ((pNew = new SConfigItem (Name, Guid, Id)) == NULL) {
    return 2;
  }

  if (mItemListHead == NULL) {
    mItemListHead = pNew;
    mItemListTail = pNew;
  } else {
    mItemListTail->mNext = pNew;
    mItemListTail = pNew;
  }
  mItemListPos    = pNew;

  return 0;
}

VOID
CVfrBufferConfig::Open (
  VOID
  )
{
  mItemListPos = mItemListHead;
}

BOOLEAN
CVfrBufferConfig::Eof(
  VOID
  )
{
  return (mItemListPos == NULL) ? TRUE : FALSE;
}

UINT8
CVfrBufferConfig::Select (
  IN CHAR8    *Name,
  IN EFI_GUID *Guid,
  IN CHAR8    *Id
  )
{
  SConfigItem *p;

  if (Name == NULL || Guid == NULL) {
    mItemListPos = mItemListHead;
    return 0;
  } else {
    for (p = mItemListHead; p != NULL; p = p->mNext) {
      if ((strcmp (p->mName, Name) != 0) || (memcmp (p->mGuid, Guid, sizeof (EFI_GUID)) != 0)) {
        continue;
      }

      if (Id != NULL) {
        if (p->mId == NULL || strcmp (p->mId, Id) != 0) {
          continue;
        }
      } else if (p->mId != NULL) {
        continue;
      }

      mItemListPos = p;
      return 0;
    }
  }

  return 1;
}

UINT8
CVfrBufferConfig::Write (
  IN CONST CHAR8         Mode,
  IN CHAR8               *Name,
  IN EFI_GUID            *Guid,
  IN CHAR8               *Id,
  IN UINT8               Type,
  IN UINT16              Offset,
  IN UINT32              Width,
  IN EFI_IFR_TYPE_VALUE  Value
  )
{
  UINT8         Ret;
  SConfigItem   *pItem;
  SConfigInfo   *pInfo;

  if ((Ret = Select (Name, Guid)) != 0) {
    return Ret;
  }

  switch (Mode) {
  case 'a' : // add
    if (Select (Name, Guid, Id) != 0) {
      if ((pItem = new SConfigItem (Name, Guid, Id, Type, Offset, (UINT16) Width, Value)) == NULL) {
        return 2;
      }
      if (mItemListHead == NULL) {
        mItemListHead = pItem;
        mItemListTail = pItem;
      } else {
        mItemListTail->mNext = pItem;
        mItemListTail = pItem;
      }
      mItemListPos = pItem;
    } else {
      // tranverse the list to find out if there's already the value for the same offset
      for (pInfo = mItemListPos->mInfoStrList; pInfo != NULL; pInfo = pInfo->mNext) {
        if (pInfo->mOffset == Offset) {
          return 0;
        }
      }
      if((pInfo = new SConfigInfo (Type, Offset, Width, Value)) == NULL) {
        return 2;
      }
      pInfo->mNext = mItemListPos->mInfoStrList;
      mItemListPos->mInfoStrList = pInfo;
    }
    break;

  case 'd' : // delete
    if (mItemListHead == mItemListPos) {
      mItemListHead = mItemListPos->mNext;
      delete mItemListPos;
      break;
    }

    for (pItem = mItemListHead; pItem->mNext != mItemListPos; pItem = pItem->mNext)
      ;

    pItem->mNext = mItemListPos->mNext;
    if (mItemListTail == mItemListPos) {
      mItemListTail = pItem;
    }
    delete mItemListPos;
    mItemListPos = pItem->mNext;
    break;

  case 'i' : // set info
    if (mItemListPos->mId != NULL) {
      delete mItemListPos->mId;
    }
    mItemListPos->mId = NULL;
    if (Id != NULL) {
      if ((mItemListPos->mId = new CHAR8[strlen (Id) + 1]) == NULL) {
        return 2;
      }
      strcpy (mItemListPos->mId, Id);
    }
    break;

  default :
    return 1;
  }

  return 0;
}


VOID
CVfrBufferConfig::Close (
  VOID
  )
{
  mItemListPos = NULL;
}

#define BYTES_PRE_LINE 0x10

VOID
CVfrBufferConfig::OutputCFile (
  IN FILE  *pFile,
  IN CHAR8 *BaseName
  )
{
  CVfrBinaryOutput Output;
  SConfigItem      *Item;
  SConfigInfo      *Info;
  UINT32           TotalLen;

  if (pFile == NULL) {
    return;
  }

  for (Item = mItemListHead; Item != NULL; Item = Item->mNext) {
    if (Item->mId != NULL || Item->mInfoStrList == NULL) {
      continue;
    }
    fprintf (pFile, "\nunsigned char %s%sBlockName[] = {", BaseName, Item->mName);

    TotalLen = sizeof (UINT32);
    for (Info = Item->mInfoStrList; Info != NULL; Info = Info->mNext) {
      TotalLen += sizeof (UINT16) * 2;
    }
    Output.WriteLine (pFile, BYTES_PRE_LINE, "  ", (CHAR8 *)&TotalLen, sizeof (UINT32));

    for (Info = Item->mInfoStrList; Info != NULL; Info = Info->mNext) {
      fprintf (pFile, "\n");
      Output.WriteLine (pFile, BYTES_PRE_LINE, "  ", (CHAR8 *)&Info->mOffset, sizeof (UINT16));
      Output.WriteLine (pFile, BYTES_PRE_LINE, "  ", (CHAR8 *)&Info->mWidth, sizeof (UINT16));
    }
    fprintf (pFile, "\n};\n");
  }

  for (Item = mItemListHead; Item != NULL; Item = Item->mNext) {
    if (Item->mId != NULL && Item->mInfoStrList != NULL) {
      fprintf (pFile, "\nunsigned char %s%sDefault%s[] = {", BaseName, Item->mName, Item->mId);

      TotalLen = sizeof (UINT32);
      for (Info = Item->mInfoStrList; Info != NULL; Info = Info->mNext) {
        TotalLen += Info->mWidth + sizeof (UINT16) * 2;
      }
      Output.WriteLine (pFile, BYTES_PRE_LINE, "  ", (CHAR8 *)&TotalLen, sizeof (UINT32));

      for (Info = Item->mInfoStrList; Info != NULL; Info = Info->mNext) {
        fprintf (pFile, "\n");
        Output.WriteLine (pFile, BYTES_PRE_LINE, "  ", (CHAR8 *)&Info->mOffset, sizeof (UINT16));
        Output.WriteLine (pFile, BYTES_PRE_LINE, "  ", (CHAR8 *)&Info->mWidth, sizeof (UINT16));
        if (Info->mNext == NULL) {
          Output.WriteEnd (pFile, BYTES_PRE_LINE, "  ", (CHAR8 *)Info->mValue, Info->mWidth);
        } else {
          Output.WriteLine (pFile, BYTES_PRE_LINE, "  ", (CHAR8 *)Info->mValue, Info->mWidth);
        }
      }
      fprintf (pFile, "\n};\n");
    }
  }
}

CVfrBufferConfig::CVfrBufferConfig (
  VOID
  )
{
  mItemListHead = NULL;
  mItemListTail = NULL;
  mItemListPos  = NULL;
}

CVfrBufferConfig::~CVfrBufferConfig (
  VOID
  )
{
  SConfigItem *p;

  while (mItemListHead != NULL) {
    p = mItemListHead;
    mItemListHead = mItemListHead->mNext;
    delete p;
  }

  mItemListHead = NULL;
  mItemListTail = NULL;
  mItemListPos  = NULL;
}

CVfrBufferConfig gCVfrBufferConfig;

static struct {
  CONST CHAR8  *mTypeName;
  UINT8  mType;
  UINT32 mSize;
  UINT32 mAlign;
} gInternalTypesTable [] = {
  {"UINT64",        EFI_IFR_TYPE_NUM_SIZE_64, sizeof (UINT64),       sizeof (UINT64)},
  {"UINT32",        EFI_IFR_TYPE_NUM_SIZE_32, sizeof (UINT32),       sizeof (UINT32)},
  {"UINT16",        EFI_IFR_TYPE_NUM_SIZE_16, sizeof (UINT16),       sizeof (UINT16)},
  {"UINT8",         EFI_IFR_TYPE_NUM_SIZE_8,  sizeof (UINT8),        sizeof (UINT8)},
  {"BOOLEAN",       EFI_IFR_TYPE_BOOLEAN,     sizeof (BOOLEAN),      sizeof (BOOLEAN)},
  {"EFI_HII_DATE",  EFI_IFR_TYPE_DATE,        sizeof (EFI_HII_DATE), sizeof (UINT16)},
  {"EFI_STRING_ID", EFI_IFR_TYPE_STRING,      sizeof (EFI_STRING_ID),sizeof (EFI_STRING_ID)},
  {"EFI_HII_TIME",  EFI_IFR_TYPE_TIME,        sizeof (EFI_HII_TIME), sizeof (UINT8)},
  {"EFI_HII_REF",   EFI_IFR_TYPE_REF,         sizeof (EFI_HII_REF),  sizeof (EFI_GUID)},
  {NULL,            EFI_IFR_TYPE_OTHER,       0,                     0}
};

STATIC
BOOLEAN
_IS_INTERNAL_TYPE (
  IN CHAR8 *TypeName
  )
{
  UINT32  Index;

  if (TypeName == NULL) {
    return FALSE;
  }

  for (Index = 0; gInternalTypesTable[Index].mTypeName != NULL; Index++) {
    if (strcmp (TypeName, gInternalTypesTable[Index].mTypeName) == 0) {
      return TRUE;
    }
  }

  return FALSE;
}

STATIC
CHAR8 *
TrimHex (
  IN  CHAR8   *Str,
  OUT bool    *IsHex
  )
{
  *IsHex = FALSE;

  while (*Str && *Str == ' ') {
    Str++;
  }
  while (*Str && *Str == '0') {
    Str++;
  }
  if (*Str && (*Str == 'x' || *Str == 'X')) {
    Str++;
    *IsHex = TRUE;
  }

  return Str;
}

UINT32
_STR2U32 (
  IN CHAR8 *Str
  )
{
  bool    IsHex;
  UINT32  Value;
  CHAR8    c;

  Str = TrimHex (Str, &IsHex);
  for (Value = 0; (c = *Str) != '\0'; Str++) {
    //
    // BUG: does not handle overflow here
    //
	(IsHex == TRUE) ? (Value <<= 4) : (Value *= 10);

    if ((IsHex == TRUE) && (c >= 'a') && (c <= 'f')) {
      Value += (c - 'a' + 10);
    }
    if ((IsHex == TRUE) && (c >= 'A') && (c <= 'F')) {
      Value += (c - 'A' + 10);
    }
    if (c >= '0' && c <= '9') {
      Value += (c - '0');
    }
  }

  return Value;
}

VOID
CVfrVarDataTypeDB::RegisterNewType (
  IN SVfrDataType  *New
  )
{
  New->mNext               = mDataTypeList;
  mDataTypeList            = New;
}

EFI_VFR_RETURN_CODE
CVfrVarDataTypeDB::ExtractStructTypeName (
  IN  CHAR8 *&VarStr,
  OUT CHAR8 *TName
  )
{
  if (TName == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }

  while((*VarStr != '\0') && (*VarStr != '.')) {
    *TName = *VarStr;
    VarStr++;
    TName++;
  }
  *TName = '\0';
  if (*VarStr == '.') {
    VarStr++;
  }

  return VFR_RETURN_SUCCESS;
}

EFI_VFR_RETURN_CODE
CVfrVarDataTypeDB::ExtractFieldNameAndArrary (
  IN  CHAR8   *&VarStr,
  IN  CHAR8   *FName,
  OUT UINT32 &ArrayIdx
  )
{
  UINT32 Idx;
  CHAR8   ArrayStr[MAX_NAME_LEN + 1];

  ArrayIdx = INVALID_ARRAY_INDEX;

  if (FName == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }

  while((*VarStr != '\0') &&
        (*VarStr != '.') &&
        (*VarStr != '[') &&
        (*VarStr != ']')) {
    *FName = *VarStr;
    VarStr++;
    FName++;
  }
  *FName = '\0';

  switch (*VarStr) {
  case '.' :
    VarStr++;
  case '\0':
    return VFR_RETURN_SUCCESS;
  case '[' :
    VarStr++;
    for (Idx = 0; (Idx < MAX_NAME_LEN) && (*VarStr != '\0') && (*VarStr != ']'); VarStr++, Idx++) {
      ArrayStr[Idx] = *VarStr;
    }
    ArrayStr[Idx] = '\0';

    if ((*VarStr != ']') && (ArrayStr[0] == '\0')) {
      return VFR_RETURN_DATA_STRING_ERROR;
    }
    ArrayIdx = _STR2U32 (ArrayStr);
    if (*VarStr == ']') {
      VarStr++;
    }
    if (*VarStr == '.') {
      VarStr++;
    }
    return VFR_RETURN_SUCCESS;
  case ']':
    return VFR_RETURN_DATA_STRING_ERROR;
  }

  return VFR_RETURN_SUCCESS;
}

EFI_VFR_RETURN_CODE
CVfrVarDataTypeDB::GetTypeField (
  IN  CONST CHAR8   *FName,
  IN  SVfrDataType  *Type,
  OUT SVfrDataField *&Field
  )
{
  SVfrDataField  *pField = NULL;

  if ((FName == NULL) && (Type == NULL)) {
    return VFR_RETURN_FATAL_ERROR;
  }

  for (pField = Type->mMembers; pField != NULL; pField = pField->mNext) {
    //
    // For type EFI_IFR_TYPE_TIME, because field name is not correctly wrote,
    // add code to adjust it.
    //
    if (Type->mType == EFI_IFR_TYPE_TIME) {
      if (strcmp (FName, "Hour") == 0) {
        FName = "Hours";
      } else if (strcmp (FName, "Minute") == 0) {
        FName = "Minuts";
      } else if (strcmp (FName, "Second") == 0) {
        FName = "Seconds";
      }
    }

    if (strcmp (pField->mFieldName, FName) == 0) {
      Field = pField;
      return VFR_RETURN_SUCCESS;
    }
  }

  return VFR_RETURN_UNDEFINED;
}

EFI_VFR_RETURN_CODE
CVfrVarDataTypeDB::GetFieldOffset (
  IN  SVfrDataField *Field,
  IN  UINT32        ArrayIdx,
  OUT UINT32        &Offset
  )
{
  if (Field == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }
  
  //
  // Framework Vfr file Array Index is from 1.
  // But Uefi Vfr file Array Index is from 0.
  //
  if (VfrCompatibleMode && ArrayIdx != INVALID_ARRAY_INDEX) {
    if (ArrayIdx == 0) {
      return VFR_RETURN_ERROR_ARRARY_NUM;
    }
    ArrayIdx = ArrayIdx - 1;
  }

  if ((ArrayIdx != INVALID_ARRAY_INDEX) && ((Field->mArrayNum == 0) || (Field->mArrayNum <= ArrayIdx))) {
    return VFR_RETURN_ERROR_ARRARY_NUM;
  }
  
  //
  // Be compatible with the current usage
  // If ArraryIdx is not specified, the first one is used.
  //
  // if ArrayNum is larger than zero, ArraryIdx must be specified.
  //
  // if ((ArrayIdx == INVALID_ARRAY_INDEX) && (Field->mArrayNum > 0)) {
  //   return VFR_RETURN_ERROR_ARRARY_NUM;
  // }
  //

  Offset = Field->mOffset + Field->mFieldType->mTotalSize * ((ArrayIdx == INVALID_ARRAY_INDEX) ? 0 : ArrayIdx);
  return VFR_RETURN_SUCCESS;
}

UINT8
CVfrVarDataTypeDB::GetFieldWidth (
  IN SVfrDataField *Field
  )
{
  if (Field == NULL) {
    return 0;
  }

  return Field->mFieldType->mType;
}

UINT32
CVfrVarDataTypeDB::GetFieldSize (
  IN SVfrDataField *Field,
  IN UINT32       ArrayIdx
  )
{
  if (Field == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }

  if ((ArrayIdx == INVALID_ARRAY_INDEX) && (Field->mArrayNum != 0)) {
    return Field->mFieldType->mTotalSize * Field->mArrayNum;
  } else {
    return Field->mFieldType->mTotalSize;
  }
}

VOID
CVfrVarDataTypeDB::InternalTypesListInit (
  VOID
  )
{
  SVfrDataType *New   = NULL;
  UINT32       Index;

  for (Index = 0; gInternalTypesTable[Index].mTypeName != NULL; Index++) {
    New                 = new SVfrDataType;
    if (New != NULL) {
      strcpy (New->mTypeName, gInternalTypesTable[Index].mTypeName);
      New->mType        = gInternalTypesTable[Index].mType;
      New->mAlign       = gInternalTypesTable[Index].mAlign;
      New->mTotalSize   = gInternalTypesTable[Index].mSize;
      if (strcmp (gInternalTypesTable[Index].mTypeName, "EFI_HII_DATE") == 0) {
        SVfrDataField *pYearField  = new SVfrDataField;
        SVfrDataField *pMonthField = new SVfrDataField;
        SVfrDataField *pDayField   = new SVfrDataField;

        strcpy (pYearField->mFieldName, "Year");
        GetDataType ((CHAR8 *)"UINT16", &pYearField->mFieldType);
        pYearField->mOffset      = 0;
        pYearField->mNext        = pMonthField;
        pYearField->mArrayNum    = 0;

        strcpy (pMonthField->mFieldName, "Month");
        GetDataType ((CHAR8 *)"UINT8", &pMonthField->mFieldType);
        pMonthField->mOffset     = 2;
        pMonthField->mNext       = pDayField;
        pMonthField->mArrayNum   = 0;

        strcpy (pDayField->mFieldName, "Day");
        GetDataType ((CHAR8 *)"UINT8", &pDayField->mFieldType);
        pDayField->mOffset       = 3;
        pDayField->mNext         = NULL;
        pDayField->mArrayNum     = 0;

        New->mMembers            = pYearField;
      } else if (strcmp (gInternalTypesTable[Index].mTypeName, "EFI_HII_TIME") == 0) {
        SVfrDataField *pHoursField   = new SVfrDataField;
        SVfrDataField *pMinutesField = new SVfrDataField;
        SVfrDataField *pSecondsField = new SVfrDataField;

        strcpy (pHoursField->mFieldName, "Hours");
        GetDataType ((CHAR8 *)"UINT8", &pHoursField->mFieldType);
        pHoursField->mOffset     = 0;
        pHoursField->mNext       = pMinutesField;
        pHoursField->mArrayNum   = 0;

        strcpy (pMinutesField->mFieldName, "Minutes");
        GetDataType ((CHAR8 *)"UINT8", &pMinutesField->mFieldType);
        pMinutesField->mOffset   = 1;
        pMinutesField->mNext     = pSecondsField;
        pMinutesField->mArrayNum = 0;

        strcpy (pSecondsField->mFieldName, "Seconds");
        GetDataType ((CHAR8 *)"UINT8", &pSecondsField->mFieldType);
        pSecondsField->mOffset   = 2;
        pSecondsField->mNext     = NULL;
        pSecondsField->mArrayNum = 0;

        New->mMembers            = pHoursField;
      } else if (strcmp (gInternalTypesTable[Index].mTypeName, "EFI_HII_REF") == 0) {
        SVfrDataField *pQuestionIdField   = new SVfrDataField;
        SVfrDataField *pFormIdField       = new SVfrDataField;
        SVfrDataField *pFormSetGuidField  = new SVfrDataField;
        SVfrDataField *pDevicePathField   = new SVfrDataField;

        strcpy (pQuestionIdField->mFieldName, "QuestionId");
        GetDataType ((CHAR8 *)"UINT16", &pQuestionIdField->mFieldType);
        pQuestionIdField->mOffset     = 0;
        pQuestionIdField->mNext       = pFormIdField;
        pQuestionIdField->mArrayNum   = 0;

        strcpy (pFormIdField->mFieldName, "FormId");
        GetDataType ((CHAR8 *)"UINT16", &pFormIdField->mFieldType);
        pFormIdField->mOffset   = 2;
        pFormIdField->mNext     = pFormSetGuidField;
        pFormIdField->mArrayNum = 0;

        strcpy (pFormSetGuidField->mFieldName, "FormSetGuid");
        GetDataType ((CHAR8 *)"EFI_GUID", &pFormSetGuidField->mFieldType);
        pFormSetGuidField->mOffset   = 4;
        pFormSetGuidField->mNext     = pDevicePathField;
        pFormSetGuidField->mArrayNum = 0;

        strcpy (pDevicePathField->mFieldName, "DevicePath");
        GetDataType ((CHAR8 *)"EFI_STRING_ID", &pDevicePathField->mFieldType);
        pDevicePathField->mOffset   = 20;
        pDevicePathField->mNext     = NULL;
        pDevicePathField->mArrayNum = 0;

        New->mMembers            = pQuestionIdField;
      } else {
        New->mMembers            = NULL;
      }
      New->mNext                 = NULL;
      RegisterNewType (New);
      New                        = NULL;
    }
  }
}

CVfrVarDataTypeDB::CVfrVarDataTypeDB (
  VOID
  )
{
  mDataTypeList  = NULL;
  mNewDataType   = NULL;
  mCurrDataField = NULL;
  mPackAlign     = DEFAULT_PACK_ALIGN;
  mPackStack     = NULL;
  mFirstNewDataTypeName = NULL;

  InternalTypesListInit ();
}

CVfrVarDataTypeDB::~CVfrVarDataTypeDB (
  VOID
  )
{
  SVfrDataType      *pType;
  SVfrDataField     *pField;
  SVfrPackStackNode *pPack;

  if (mNewDataType != NULL) {
    delete mNewDataType;
  }

  while (mDataTypeList != NULL) {
    pType = mDataTypeList;
    mDataTypeList = mDataTypeList->mNext;
    while(pType->mMembers != NULL) {
      pField = pType->mMembers;
      pType->mMembers = pType->mMembers->mNext;
      delete pField;
    }
	delete pType;
  }

  while (mPackStack != NULL) {
    pPack = mPackStack;
    mPackStack = mPackStack->mNext;
    delete pPack;
  }
}

EFI_VFR_RETURN_CODE
CVfrVarDataTypeDB::Pack (
  IN UINT32         LineNum,
  IN UINT8          Action,
  IN CHAR8           *Identifier,
  IN UINT32         Number
  )
{
  UINT32            PackAlign;
  CHAR8             Msg[MAX_STRING_LEN] = {0, };

  if (Action & VFR_PACK_SHOW) {
    sprintf (Msg, "value of pragma pack(show) == %d", mPackAlign);
    gCVfrErrorHandle.PrintMsg (LineNum, NULL, "Warning", Msg);
  }

  if (Action & VFR_PACK_PUSH) {
    SVfrPackStackNode *pNew = NULL;

    if ((pNew = new SVfrPackStackNode (Identifier, mPackAlign)) == NULL) {
      return VFR_RETURN_FATAL_ERROR;
    }
    pNew->mNext = mPackStack;
    mPackStack  = pNew;
  }

  if (Action & VFR_PACK_POP) {
    SVfrPackStackNode *pNode = NULL;

    if (mPackStack == NULL) {
      gCVfrErrorHandle.PrintMsg (LineNum, NULL, "Error", "#pragma pack(pop...) : more pops than pushes");
    }

    for (pNode = mPackStack; pNode != NULL; pNode = pNode->mNext) {
      if (pNode->Match (Identifier) == TRUE) {
        mPackAlign = pNode->mNumber;
        mPackStack = pNode->mNext;
      }
    }
  }

  if (Action & VFR_PACK_ASSIGN) {
    PackAlign = (Number > 1) ? Number + Number % 2 : Number;
    if ((PackAlign == 0) || (PackAlign > 16)) {
      gCVfrErrorHandle.PrintMsg (LineNum, NULL, "Error", "expected pragma parameter to be '1', '2', '4', '8', or '16'");
    } else {
      mPackAlign = PackAlign;
    }
  }

  return VFR_RETURN_SUCCESS;
}

VOID
CVfrVarDataTypeDB::DeclareDataTypeBegin (
  VOID
  )
{
  SVfrDataType *pNewType = NULL;

  pNewType               = new SVfrDataType;
  pNewType->mTypeName[0] = '\0';
  pNewType->mType        = EFI_IFR_TYPE_OTHER;
  pNewType->mAlign       = DEFAULT_ALIGN;
  pNewType->mTotalSize   = 0;
  pNewType->mMembers     = NULL;
  pNewType->mNext        = NULL;

  mNewDataType           = pNewType;
}

EFI_VFR_RETURN_CODE
CVfrVarDataTypeDB::SetNewTypeName (
  IN CHAR8   *TypeName
  )
{
  SVfrDataType *pType;

  if (mNewDataType == NULL) {
    return VFR_RETURN_ERROR_SKIPED;
  }
  if (TypeName == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }
  if (strlen(TypeName) >= MAX_NAME_LEN) {
    return VFR_RETURN_INVALID_PARAMETER;
  }

  for (pType = mDataTypeList; pType != NULL; pType = pType->mNext) {
    if (strcmp(pType->mTypeName, TypeName) == 0) {
      return VFR_RETURN_REDEFINED;
    }
  }

  strcpy(mNewDataType->mTypeName, TypeName);
  return VFR_RETURN_SUCCESS;
}

EFI_VFR_RETURN_CODE
CVfrVarDataTypeDB::DataTypeAddField (
  IN CHAR8   *FieldName,
  IN CHAR8   *TypeName,
  IN UINT32 ArrayNum
  )
{
  SVfrDataField       *pNewField  = NULL;
  SVfrDataType        *pFieldType = NULL;
  SVfrDataField       *pTmp;
  UINT32              Align;

  CHECK_ERROR_RETURN (GetDataType (TypeName, &pFieldType), VFR_RETURN_SUCCESS);

  if (strlen (FieldName) >= MAX_NAME_LEN) {
   return VFR_RETURN_INVALID_PARAMETER;
  }

  for (pTmp = mNewDataType->mMembers; pTmp != NULL; pTmp = pTmp->mNext) {
    if (strcmp (pTmp->mFieldName, FieldName) == 0) {
      return VFR_RETURN_REDEFINED;
    }
  }

  Align = MIN (mPackAlign, pFieldType->mAlign);

  if ((pNewField = new SVfrDataField) == NULL) {
    return VFR_RETURN_OUT_FOR_RESOURCES;
  }
  strcpy (pNewField->mFieldName, FieldName);
  pNewField->mFieldType    = pFieldType;
  pNewField->mArrayNum     = ArrayNum;
  if ((mNewDataType->mTotalSize % Align) == 0) {
    pNewField->mOffset     = mNewDataType->mTotalSize;
  } else {
    pNewField->mOffset     = mNewDataType->mTotalSize + ALIGN_STUFF(mNewDataType->mTotalSize, Align);
  }
  if (mNewDataType->mMembers == NULL) {
    mNewDataType->mMembers = pNewField;
    pNewField->mNext       = NULL;
  } else {
    for (pTmp = mNewDataType->mMembers; pTmp->mNext != NULL; pTmp = pTmp->mNext)
      ;
    pTmp->mNext            = pNewField;
    pNewField->mNext       = NULL;
  }

  mNewDataType->mAlign     = MIN (mPackAlign, MAX (pFieldType->mAlign, mNewDataType->mAlign));
  mNewDataType->mTotalSize = pNewField->mOffset + (pNewField->mFieldType->mTotalSize) * ((ArrayNum == 0) ? 1 : ArrayNum);

  return VFR_RETURN_SUCCESS;
}

VOID
CVfrVarDataTypeDB::DeclareDataTypeEnd (
  VOID
  )
{
  if (mNewDataType->mTypeName[0] == '\0') {
    return;
  }

  if ((mNewDataType->mTotalSize % mNewDataType->mAlign) !=0) {
    mNewDataType->mTotalSize += ALIGN_STUFF (mNewDataType->mTotalSize, mNewDataType->mAlign);
  }

  RegisterNewType (mNewDataType);
  if (mFirstNewDataTypeName == NULL) {
    mFirstNewDataTypeName = mNewDataType->mTypeName;
  }

  mNewDataType             = NULL;
}

EFI_VFR_RETURN_CODE
CVfrVarDataTypeDB::GetDataType (
  IN  CHAR8         *TypeName,
  OUT SVfrDataType **DataType
  )
{
  SVfrDataType *pDataType = NULL;

  if (TypeName == NULL) {
    return VFR_RETURN_ERROR_SKIPED;
  }

  if (DataType == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }

  *DataType = NULL;

  for (pDataType = mDataTypeList; pDataType != NULL; pDataType = pDataType->mNext) {
    if (strcmp (TypeName, pDataType->mTypeName) == 0) {
      *DataType = pDataType;
      return VFR_RETURN_SUCCESS;
    }
  }

  return VFR_RETURN_UNDEFINED;
}

EFI_VFR_RETURN_CODE
CVfrVarDataTypeDB::GetDataTypeSize (
  IN  UINT8   DataType,
  OUT UINT32 *Size
  )
{
  SVfrDataType *pDataType = NULL;

  if (Size == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }

  *Size    = 0;
  DataType = DataType & 0x0F;

  //
  // For user defined data type, the size can't be got by this function.
  //
  if (DataType == EFI_IFR_TYPE_OTHER) {
    return VFR_RETURN_SUCCESS;
  }

  for (pDataType = mDataTypeList; pDataType != NULL; pDataType = pDataType->mNext) {
    if (DataType == pDataType->mType) {
      *Size = pDataType->mTotalSize;
      return VFR_RETURN_SUCCESS;
    }
  }

  return VFR_RETURN_UNDEFINED;
}

EFI_VFR_RETURN_CODE
CVfrVarDataTypeDB::GetDataTypeSize (
  IN  CHAR8   *TypeName,
  OUT UINT32 *Size
  )
{
  SVfrDataType *pDataType = NULL;

  if (Size == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }

  *Size = 0;

  for (pDataType = mDataTypeList; pDataType != NULL; pDataType = pDataType->mNext) {
    if (strcmp (TypeName, pDataType->mTypeName) == 0) {
      *Size = pDataType->mTotalSize;
      return VFR_RETURN_SUCCESS;
    }
  }

  return VFR_RETURN_UNDEFINED;
}

EFI_VFR_RETURN_CODE
CVfrVarDataTypeDB::GetDataFieldInfo (
  IN  CHAR8     *VarStr,
  OUT UINT16   &Offset,
  OUT UINT8    &Type,
  OUT UINT32   &Size
  )
{
  CHAR8               TName[MAX_NAME_LEN], FName[MAX_NAME_LEN];
  UINT32              ArrayIdx, Tmp;
  SVfrDataType        *pType  = NULL;
  SVfrDataField       *pField = NULL;

  Offset = 0;
  Type   = EFI_IFR_TYPE_OTHER;
  Size   = 0;

  CHECK_ERROR_RETURN (ExtractStructTypeName (VarStr, TName), VFR_RETURN_SUCCESS);
  CHECK_ERROR_RETURN (GetDataType (TName, &pType), VFR_RETURN_SUCCESS);

  //
  // if it is not struct data type
  //
  Type  = pType->mType;
  Size  = pType->mTotalSize;

  while (*VarStr != '\0') {
  	CHECK_ERROR_RETURN(ExtractFieldNameAndArrary(VarStr, FName, ArrayIdx), VFR_RETURN_SUCCESS);
    CHECK_ERROR_RETURN(GetTypeField (FName, pType, pField), VFR_RETURN_SUCCESS);
    pType  = pField->mFieldType;
    CHECK_ERROR_RETURN(GetFieldOffset (pField, ArrayIdx, Tmp), VFR_RETURN_SUCCESS);
    Offset = (UINT16) (Offset + Tmp);
    Type   = GetFieldWidth (pField);
    Size   = GetFieldSize (pField, ArrayIdx);
  }
  return VFR_RETURN_SUCCESS;
}

EFI_VFR_RETURN_CODE
CVfrVarDataTypeDB::GetUserDefinedTypeNameList  (
  OUT CHAR8      ***NameList,
  OUT UINT32    *ListSize
  )
{
  UINT32       Index;
  SVfrDataType *pType;

  if ((NameList == NULL) || (ListSize == NULL)) {
    return VFR_RETURN_FATAL_ERROR;
  }

  *NameList = NULL;
  *ListSize = 0;

  for (pType = mDataTypeList; pType != NULL; pType = pType->mNext) {
    if (_IS_INTERNAL_TYPE(pType->mTypeName) == FALSE) {
      (*ListSize)++;
    }
  }

  if (*ListSize == 0) {
    return VFR_RETURN_SUCCESS;
  }

  if ((*NameList = new CHAR8*[*ListSize]) == NULL) {
    *ListSize = 0;
    return VFR_RETURN_OUT_FOR_RESOURCES;
  }

  for (Index = 0, pType = mDataTypeList; pType != NULL; pType = pType->mNext, Index++) {
    if (_IS_INTERNAL_TYPE(pType->mTypeName) == FALSE) {
      (*NameList)[Index] = pType->mTypeName;
    }
  }
  return VFR_RETURN_SUCCESS;
}

BOOLEAN
CVfrVarDataTypeDB::IsTypeNameDefined (
  IN CHAR8 *TypeName
  )
{
  SVfrDataType *pType;

  if (TypeName == NULL) {
    return FALSE;
  }

  for (pType = mDataTypeList; pType != NULL; pType = pType->mNext) {
    if (strcmp (pType->mTypeName, TypeName) == 0) {
      return TRUE;
    }
  }

  return FALSE;
}

VOID
CVfrVarDataTypeDB::Dump (
  IN FILE         *File
  )
{
  SVfrDataType  *pTNode;
  SVfrDataField *pFNode;

  fprintf (File, "\n\n***************************************************************\n");
  fprintf (File, "\t\tmPackAlign = %x\n", mPackAlign);
  for (pTNode = mDataTypeList; pTNode != NULL; pTNode = pTNode->mNext) {
    fprintf (File, "\t\tstruct %s : mAlign [%d] mTotalSize [0x%x]\n\n", pTNode->mTypeName, pTNode->mAlign, pTNode->mTotalSize);
    fprintf (File, "\t\tstruct %s {\n", pTNode->mTypeName);
    for (pFNode = pTNode->mMembers; pFNode != NULL; pFNode = pFNode->mNext) {
      if (pFNode->mArrayNum > 0) {
        fprintf (File, "\t\t\t+%08d[%08x] %s[%d] <%s>\n", pFNode->mOffset, pFNode->mOffset, 
                  pFNode->mFieldName, pFNode->mArrayNum, pFNode->mFieldType->mTypeName);
      } else {
        fprintf (File, "\t\t\t+%08d[%08x] %s <%s>\n", pFNode->mOffset, pFNode->mOffset, 
                  pFNode->mFieldName, pFNode->mFieldType->mTypeName);
      }
    }
    fprintf (File, "\t\t};\n");
  fprintf (File, "---------------------------------------------------------------\n");
  }
  fprintf (File, "***************************************************************\n");
}

#ifdef CVFR_VARDATATYPEDB_DEBUG
VOID
CVfrVarDataTypeDB::ParserDB (
  VOID
  )
{
  SVfrDataType  *pTNode;
  SVfrDataField *pFNode;

  printf ("***************************************************************\n");
  printf ("\t\tmPackAlign = %x\n", mPackAlign);
  for (pTNode = mDataTypeList; pTNode != NULL; pTNode = pTNode->mNext) {
    printf ("\t\tstruct %s : mAlign [%x] mTotalSize [%x]\n\n", pTNode->mTypeName, pTNode->mAlign, pTNode->mTotalSize);
    printf ("\t\tstruct %s {\n", pTNode->mTypeName);
    for (pFNode = pTNode->mMembers; pFNode != NULL; pFNode = pFNode->mNext) {
      printf ("\t\t\t%s\t%s\n", pFNode->mFieldType->mTypeName, pFNode->mFieldName);
    }
    printf ("\t\t};\n");
	printf ("---------------------------------------------------------------\n");
  }
  printf ("***************************************************************\n");
}
#endif

SVfrVarStorageNode::SVfrVarStorageNode (
  IN EFI_GUID              *Guid,
  IN CHAR8                 *StoreName,
  IN EFI_VARSTORE_ID       VarStoreId,
  IN EFI_STRING_ID         VarName,
  IN UINT32                VarSize,
  IN BOOLEAN               Flag
  )
{
  if (Guid != NULL) {
    mGuid = *Guid;
  } else {
    memset (&Guid, 0, sizeof (EFI_GUID));
  }
  if (StoreName != NULL) {
    mVarStoreName = new CHAR8[strlen(StoreName) + 1];
    strcpy (mVarStoreName, StoreName);
  } else {
    mVarStoreName = NULL;
  }
  mNext                            = NULL;
  mVarStoreId                      = VarStoreId;
  mVarStoreType                    = EFI_VFR_VARSTORE_EFI;
  mStorageInfo.mEfiVar.mEfiVarName = VarName;
  mStorageInfo.mEfiVar.mEfiVarSize = VarSize;
  mAssignedFlag                    = Flag;
}

SVfrVarStorageNode::SVfrVarStorageNode (
  IN EFI_GUID              *Guid,
  IN CHAR8                 *StoreName,
  IN EFI_VARSTORE_ID       VarStoreId,
  IN SVfrDataType          *DataType,
  IN BOOLEAN               Flag
  )
{
  if (Guid != NULL) {
    mGuid = *Guid;
  } else {
    memset (&Guid, 0, sizeof (EFI_GUID));
  }
  if (StoreName != NULL) {
    mVarStoreName = new CHAR8[strlen(StoreName) + 1];
    strcpy (mVarStoreName, StoreName);
  } else {
    mVarStoreName = NULL;
  }
  mNext                    = NULL;
  mVarStoreId              = VarStoreId;
  mVarStoreType            = EFI_VFR_VARSTORE_BUFFER;
  mStorageInfo.mDataType   = DataType;
  mAssignedFlag            = Flag;
}

SVfrVarStorageNode::SVfrVarStorageNode (
  IN CHAR8                 *StoreName,
  IN EFI_VARSTORE_ID       VarStoreId
  )
{
  if (StoreName != NULL) {
    mVarStoreName = new CHAR8[strlen(StoreName) + 1];
    strcpy (mVarStoreName, StoreName);
  } else {
    mVarStoreName = NULL;
  }
  mNext                              = NULL;
  mVarStoreId                        = VarStoreId;
  mVarStoreType                      = EFI_VFR_VARSTORE_NAME;
  mStorageInfo.mNameSpace.mNameTable = new EFI_VARSTORE_ID[DEFAULT_NAME_TABLE_ITEMS];
  mStorageInfo.mNameSpace.mTableSize = 0;
}

SVfrVarStorageNode::~SVfrVarStorageNode (
  VOID
  )
{
  if (mVarStoreName != NULL) {
    delete mVarStoreName;
  }

  if (mVarStoreType == EFI_VFR_VARSTORE_NAME) {
    delete mStorageInfo.mNameSpace.mNameTable;
  }
}

CVfrDataStorage::CVfrDataStorage (
  VOID
  )
{
  UINT32 Index;

  for (Index = 0; Index < EFI_FREE_VARSTORE_ID_BITMAP_SIZE; Index++) {
    mFreeVarStoreIdBitMap[Index] = 0;
  }

  // Question ID 0 is reserved.
  mFreeVarStoreIdBitMap[0] = 0x80000000;

  mBufferVarStoreList      = NULL;
  mEfiVarStoreList         = NULL;
  mNameVarStoreList        = NULL;
  mCurrVarStorageNode      = NULL;
  mNewVarStorageNode       = NULL;
}

CVfrDataStorage::~CVfrDataStorage (
  VOID
  )
{
  SVfrVarStorageNode *pNode;

  while (mBufferVarStoreList != NULL) {
    pNode = mBufferVarStoreList;
    mBufferVarStoreList = mBufferVarStoreList->mNext;
    delete pNode;
  }
  while (mEfiVarStoreList != NULL) {
    pNode = mEfiVarStoreList;
    mEfiVarStoreList = mEfiVarStoreList->mNext;
    delete pNode;
  }
  while (mNameVarStoreList != NULL) {
    pNode = mNameVarStoreList;
    mNameVarStoreList = mNameVarStoreList->mNext;
    delete pNode;
  }
  if (mNewVarStorageNode != NULL) {
    delete mNewVarStorageNode;
  }
}

EFI_VARSTORE_ID
CVfrDataStorage::GetFreeVarStoreId (
  EFI_VFR_VARSTORE_TYPE VarType
  )
{
  UINT32  Index, Mask, Offset;
  
  //
  // Assign the different ID range for the different type VarStore to support Framework Vfr
  //
  Index = 0;
  if ((!VfrCompatibleMode) || (VarType == EFI_VFR_VARSTORE_BUFFER)) {
    Index = 0;
  } else if (VarType == EFI_VFR_VARSTORE_EFI) {
    Index = 1;
  } else if (VarType == EFI_VFR_VARSTORE_NAME) {
    Index = 2;
  }

  for (; Index < EFI_FREE_VARSTORE_ID_BITMAP_SIZE; Index++) {
    if (mFreeVarStoreIdBitMap[Index] != 0xFFFFFFFF) {
      break;
    }
  }

  for (Offset = 0, Mask = 0x80000000; Mask != 0; Mask >>= 1, Offset++) {
    if ((mFreeVarStoreIdBitMap[Index] & Mask) == 0) {
      mFreeVarStoreIdBitMap[Index] |= Mask;
      return (EFI_VARSTORE_ID)((Index << EFI_BITS_SHIFT_PER_UINT32) + Offset);
    }
  }

  return EFI_VARSTORE_ID_INVALID;
}

BOOLEAN
CVfrDataStorage::ChekVarStoreIdFree (
  IN EFI_VARSTORE_ID VarStoreId
  )
{
  UINT32 Index  = (VarStoreId / EFI_BITS_PER_UINT32);
  UINT32 Offset = (VarStoreId % EFI_BITS_PER_UINT32);

  return (mFreeVarStoreIdBitMap[Index] & (0x80000000 >> Offset)) == 0;
}

VOID
CVfrDataStorage::MarkVarStoreIdUsed (
  IN EFI_VARSTORE_ID VarStoreId
  )
{
  UINT32 Index  = (VarStoreId / EFI_BITS_PER_UINT32);
  UINT32 Offset = (VarStoreId % EFI_BITS_PER_UINT32);

  mFreeVarStoreIdBitMap[Index] |= (0x80000000 >> Offset);
}

VOID
CVfrDataStorage::MarkVarStoreIdUnused (
  IN EFI_VARSTORE_ID VarStoreId
  )
{
  UINT32 Index  = (VarStoreId / EFI_BITS_PER_UINT32);
  UINT32 Offset = (VarStoreId % EFI_BITS_PER_UINT32);

  mFreeVarStoreIdBitMap[Index] &= ~(0x80000000 >> Offset);
}

EFI_VFR_RETURN_CODE
CVfrDataStorage::DeclareNameVarStoreBegin (
  IN CHAR8           *StoreName,
  IN EFI_VARSTORE_ID VarStoreId
  )
{
  SVfrVarStorageNode *pNode = NULL;
  EFI_VARSTORE_ID    TmpVarStoreId;

  if (StoreName == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }

  if (GetVarStoreId (StoreName, &TmpVarStoreId) == VFR_RETURN_SUCCESS) {
    return VFR_RETURN_REDEFINED;
  }
  
  if (VarStoreId == EFI_VARSTORE_ID_INVALID) {
    VarStoreId = GetFreeVarStoreId (EFI_VFR_VARSTORE_NAME);
  } else {
    if (ChekVarStoreIdFree (VarStoreId) == FALSE) {
      return VFR_RETURN_VARSTOREID_REDEFINED;
    }
    MarkVarStoreIdUsed (VarStoreId);
  }

  if ((pNode = new SVfrVarStorageNode (StoreName, VarStoreId)) == NULL) {
    return VFR_RETURN_UNDEFINED;
  }

  mNewVarStorageNode = pNode;

  return VFR_RETURN_SUCCESS;
}

EFI_VFR_RETURN_CODE
CVfrDataStorage::NameTableAddItem (
  IN EFI_STRING_ID  Item
  )
{
  EFI_VARSTORE_ID *NewTable, *OldTable;
  UINT32          TableSize;

  OldTable  = mNewVarStorageNode->mStorageInfo.mNameSpace.mNameTable;
  TableSize = mNewVarStorageNode->mStorageInfo.mNameSpace.mTableSize;

  if ((TableSize != 0) && ((TableSize % DEFAULT_NAME_TABLE_ITEMS) == 0)) {
    if ((NewTable = new EFI_VARSTORE_ID[TableSize + DEFAULT_NAME_TABLE_ITEMS]) == NULL) {
      return VFR_RETURN_OUT_FOR_RESOURCES;
    }
    memcpy (NewTable, OldTable, TableSize);
    mNewVarStorageNode->mStorageInfo.mNameSpace.mNameTable = NewTable;
  }

  mNewVarStorageNode->mStorageInfo.mNameSpace.mNameTable[TableSize++] = Item;
  mNewVarStorageNode->mStorageInfo.mNameSpace.mTableSize = TableSize;

  return VFR_RETURN_SUCCESS;
}

EFI_VFR_RETURN_CODE
CVfrDataStorage::DeclareNameVarStoreEnd (
  IN EFI_GUID *Guid
  )
{
  mNewVarStorageNode->mGuid = *Guid;
  mNewVarStorageNode->mNext = mNameVarStoreList;
  mNameVarStoreList         = mNewVarStorageNode;

  mNewVarStorageNode        = NULL;

  return VFR_RETURN_SUCCESS;
}

EFI_VFR_RETURN_CODE 
CVfrDataStorage::DeclareEfiVarStore (
  IN CHAR8          *StoreName, 
  IN EFI_GUID       *Guid, 
  IN EFI_STRING_ID  NameStrId,
  IN UINT32         VarSize,
  IN BOOLEAN        Flag
  )
{
  SVfrVarStorageNode *pNode;
  EFI_VARSTORE_ID    VarStoreId;

  if ((StoreName == NULL) || (Guid == NULL)) {
    return VFR_RETURN_FATAL_ERROR;
  }

  if (VarSize > sizeof (UINT64)) {
    return VFR_RETURN_EFIVARSTORE_SIZE_ERROR;
  }

  if (GetVarStoreId (StoreName, &VarStoreId, Guid) == VFR_RETURN_SUCCESS) {
    return VFR_RETURN_REDEFINED;
  }

  VarStoreId = GetFreeVarStoreId (EFI_VFR_VARSTORE_EFI);
  if ((pNode = new SVfrVarStorageNode (Guid, StoreName, VarStoreId, NameStrId, VarSize, Flag)) == NULL) {
    return VFR_RETURN_OUT_FOR_RESOURCES;
  }

  pNode->mNext       = mEfiVarStoreList;
  mEfiVarStoreList   = pNode;

  return VFR_RETURN_SUCCESS;
}

EFI_VFR_RETURN_CODE 
CVfrDataStorage::DeclareBufferVarStore (
  IN CHAR8             *StoreName, 
  IN EFI_GUID          *Guid, 
  IN CVfrVarDataTypeDB *DataTypeDB,
  IN CHAR8             *TypeName,
  IN EFI_VARSTORE_ID   VarStoreId,
  IN BOOLEAN           Flag
  )
{
  SVfrVarStorageNode   *pNew = NULL;
  SVfrDataType         *pDataType = NULL;
  EFI_VARSTORE_ID      TempVarStoreId;

  if ((StoreName == NULL) || (Guid == NULL) || (DataTypeDB == NULL)) {
    return VFR_RETURN_FATAL_ERROR;
  }

  if (GetVarStoreId (StoreName, &TempVarStoreId, Guid) == VFR_RETURN_SUCCESS) {
    return VFR_RETURN_REDEFINED;
  }

  CHECK_ERROR_RETURN(DataTypeDB->GetDataType (TypeName, &pDataType), VFR_RETURN_SUCCESS);

  if (VarStoreId == EFI_VARSTORE_ID_INVALID) {
    VarStoreId = GetFreeVarStoreId (EFI_VFR_VARSTORE_BUFFER);
  } else {
    if (ChekVarStoreIdFree (VarStoreId) == FALSE) {
      return VFR_RETURN_VARSTOREID_REDEFINED;
    }
    MarkVarStoreIdUsed (VarStoreId);
  }

  if ((pNew = new SVfrVarStorageNode (Guid, StoreName, VarStoreId, pDataType, Flag)) == NULL) {
    return VFR_RETURN_OUT_FOR_RESOURCES;
  }

  pNew->mNext         = mBufferVarStoreList;
  mBufferVarStoreList = pNew;

  if (gCVfrBufferConfig.Register(StoreName, Guid) != 0) {
    return VFR_RETURN_FATAL_ERROR;
  }

  return VFR_RETURN_SUCCESS;
}

EFI_VFR_RETURN_CODE 
CVfrDataStorage::GetVarStoreByDataType (
  IN  CHAR8              *DataTypeName,
  OUT SVfrVarStorageNode **VarNode,
  IN  EFI_GUID           *VarGuid
  )
{
  SVfrVarStorageNode    *pNode;
  SVfrVarStorageNode    *MatchNode;
  
  //
  // Framework VFR uses Data type name as varstore name, so don't need check again.
  //
  if (VfrCompatibleMode) {
    return VFR_RETURN_UNDEFINED;
  }

  MatchNode = NULL;
  for (pNode = mBufferVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (strcmp (pNode->mStorageInfo.mDataType->mTypeName, DataTypeName) != 0) {
      continue;
    }

    if ((VarGuid != NULL)) {
      if (memcmp (VarGuid, &pNode->mGuid, sizeof (EFI_GUID)) == 0) {
        *VarNode = pNode;
        return VFR_RETURN_SUCCESS;
      }
    } else {
      if (MatchNode == NULL) {
        MatchNode = pNode;
      } else {
        //
        // More than one varstores referred the same data structures.
        //
        return VFR_RETURN_VARSTORE_DATATYPE_REDEFINED_ERROR;
      }
    }
  }
  
  if (MatchNode == NULL) {
    return VFR_RETURN_UNDEFINED;
  }

  *VarNode = MatchNode;
  return VFR_RETURN_SUCCESS;
}

EFI_VARSTORE_ID 
CVfrDataStorage::CheckGuidField (
  IN  SVfrVarStorageNode   *pNode,
  IN  EFI_GUID             *StoreGuid,
  IN  BOOLEAN              *HasFoundOne,
  OUT EFI_VFR_RETURN_CODE  *ReturnCode
  )
{
  if (StoreGuid != NULL) {
    //
    // If has guid info, compare the guid filed.
    //
    if (memcmp (StoreGuid, &pNode->mGuid, sizeof (EFI_GUID)) == 0) {
      //
      // Both name and guid are same, this this varstore.
      //
      mCurrVarStorageNode = pNode;
      *ReturnCode = VFR_RETURN_SUCCESS;
      return TRUE;
    }
  } else {
    //
    // Not has Guid field, check whether this name is the only one.
    //
    if (*HasFoundOne) {
      //
      // The name has conflict, return name redefined.
      //
      *ReturnCode = VFR_RETURN_VARSTORE_NAME_REDEFINED_ERROR;
      return TRUE;
    }

    *HasFoundOne = TRUE;
    mCurrVarStorageNode = pNode;
  }

  return FALSE;
}

/**
  Base on the input store name and guid to find the varstore id. 

  If both name and guid are inputed, base on the name and guid to
  found the varstore. If only name inputed, base on the name to
  found the varstore and go on to check whether more than one varstore
  has the same name. If only has found one varstore, return this
  varstore; if more than one varstore has same name, return varstore
  name redefined error. If no varstore found by varstore name, call
  function GetVarStoreByDataType and use inputed varstore name as 
  data type name to search.
**/
EFI_VFR_RETURN_CODE 
CVfrDataStorage::GetVarStoreId (
  IN  CHAR8           *StoreName,
  OUT EFI_VARSTORE_ID *VarStoreId,
  IN  EFI_GUID        *StoreGuid
  )
{
  EFI_VFR_RETURN_CODE   ReturnCode;
  SVfrVarStorageNode    *pNode;
  BOOLEAN               HasFoundOne = FALSE;

  mCurrVarStorageNode = NULL;

  for (pNode = mBufferVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (strcmp (pNode->mVarStoreName, StoreName) == 0) {
      if (CheckGuidField(pNode, StoreGuid, &HasFoundOne, &ReturnCode)) {
        *VarStoreId = mCurrVarStorageNode->mVarStoreId;
        return ReturnCode;
      }
    }
  }

  for (pNode = mEfiVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (strcmp (pNode->mVarStoreName, StoreName) == 0) {
      if (CheckGuidField(pNode, StoreGuid, &HasFoundOne, &ReturnCode)) {
        *VarStoreId = mCurrVarStorageNode->mVarStoreId;
        return ReturnCode;
      }
    }
  }

  for (pNode = mNameVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (strcmp (pNode->mVarStoreName, StoreName) == 0) {
      if (CheckGuidField(pNode, StoreGuid, &HasFoundOne, &ReturnCode)) {
        *VarStoreId = mCurrVarStorageNode->mVarStoreId;
        return ReturnCode;
      }
    }
  }

  if (HasFoundOne) {
    *VarStoreId = mCurrVarStorageNode->mVarStoreId;
    return VFR_RETURN_SUCCESS;
  }

  *VarStoreId         = EFI_VARSTORE_ID_INVALID;

  //
  // Assume that Data strucutre name is used as StoreName, and check again. 
  //
  ReturnCode = GetVarStoreByDataType (StoreName, &pNode, StoreGuid);
  if (pNode != NULL) {
    mCurrVarStorageNode = pNode;
    *VarStoreId = pNode->mVarStoreId;
  }
  
  return ReturnCode;
}

EFI_VFR_RETURN_CODE
CVfrDataStorage::GetBufferVarStoreDataTypeName (
  IN  EFI_VARSTORE_ID        VarStoreId,
  OUT CHAR8                  **DataTypeName
  )
{
  SVfrVarStorageNode    *pNode;

  if (VarStoreId == EFI_VARSTORE_ID_INVALID) {
    return VFR_RETURN_FATAL_ERROR;
  }

  for (pNode = mBufferVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mVarStoreId == VarStoreId) {
      *DataTypeName = pNode->mStorageInfo.mDataType->mTypeName;
      return VFR_RETURN_SUCCESS;
    }
  }

  return VFR_RETURN_UNDEFINED;
}

EFI_VFR_VARSTORE_TYPE
CVfrDataStorage::GetVarStoreType (
  IN  EFI_VARSTORE_ID        VarStoreId
  )
{
  SVfrVarStorageNode    *pNode;
  EFI_VFR_VARSTORE_TYPE VarStoreType;

  VarStoreType = EFI_VFR_VARSTORE_INVALID;

  if (VarStoreId == EFI_VARSTORE_ID_INVALID) {
    return VarStoreType;
  }

  for (pNode = mBufferVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mVarStoreId == VarStoreId) {
      VarStoreType = pNode->mVarStoreType;
      return VarStoreType;
    }
  }

  for (pNode = mEfiVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mVarStoreId == VarStoreId) {
      VarStoreType = pNode->mVarStoreType;
      return VarStoreType;
    }
  }

  for (pNode = mNameVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mVarStoreId == VarStoreId) {
      VarStoreType = pNode->mVarStoreType;
      return VarStoreType;
    }
  }

  return VarStoreType;
}

EFI_GUID *
CVfrDataStorage::GetVarStoreGuid (
  IN  EFI_VARSTORE_ID        VarStoreId
  )
{
  SVfrVarStorageNode    *pNode;
  EFI_GUID              *VarGuid;

  VarGuid = NULL;

  if (VarStoreId == EFI_VARSTORE_ID_INVALID) {
    return VarGuid;
  }

  for (pNode = mBufferVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mVarStoreId == VarStoreId) {
      VarGuid = &pNode->mGuid;
      return VarGuid;
    }
  }

  for (pNode = mEfiVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mVarStoreId == VarStoreId) {
      VarGuid = &pNode->mGuid;
      return VarGuid;
    }
  }

  for (pNode = mNameVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mVarStoreId == VarStoreId) {
      VarGuid = &pNode->mGuid;
      return VarGuid;
    }
  }

  return VarGuid;
}

EFI_VFR_RETURN_CODE
CVfrDataStorage::GetVarStoreName (
  IN  EFI_VARSTORE_ID VarStoreId, 
  OUT CHAR8           **VarStoreName
  )
{
  SVfrVarStorageNode    *pNode;

  if (VarStoreName == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }

  for (pNode = mBufferVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mVarStoreId == VarStoreId) {
      *VarStoreName = pNode->mVarStoreName;
      return VFR_RETURN_SUCCESS;
    }
  }

  for (pNode = mEfiVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mVarStoreId == VarStoreId) {
      *VarStoreName = pNode->mVarStoreName;
      return VFR_RETURN_SUCCESS;
    }
  }

  for (pNode = mNameVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mVarStoreId == VarStoreId) {
      *VarStoreName = pNode->mVarStoreName;
      return VFR_RETURN_SUCCESS;
    }
  }

  *VarStoreName = NULL;
  return VFR_RETURN_UNDEFINED;
}

EFI_VFR_RETURN_CODE
CVfrDataStorage::GetEfiVarStoreInfo (
  IN OUT EFI_VARSTORE_INFO  *Info
  )
{
  if (Info == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }

  if (mCurrVarStorageNode == NULL) {
    return VFR_RETURN_GET_EFIVARSTORE_ERROR;
  }

  Info->mInfo.mVarName = mCurrVarStorageNode->mStorageInfo.mEfiVar.mEfiVarName;
  Info->mVarTotalSize  = mCurrVarStorageNode->mStorageInfo.mEfiVar.mEfiVarSize;
  switch (Info->mVarTotalSize) {
  case 1:
    Info->mVarType = EFI_IFR_TYPE_NUM_SIZE_8;
    break;
  case 2:
    Info->mVarType = EFI_IFR_TYPE_NUM_SIZE_16;
    break;
  case 4:
    Info->mVarType = EFI_IFR_TYPE_NUM_SIZE_32;
    break;
  case 8:
    Info->mVarType = EFI_IFR_TYPE_NUM_SIZE_64;
    break;
  default :
    return VFR_RETURN_FATAL_ERROR;
  }

  return VFR_RETURN_SUCCESS;
}

EFI_VFR_RETURN_CODE
CVfrDataStorage::GetNameVarStoreInfo (
  OUT EFI_VARSTORE_INFO  *Info,
  IN  UINT32             Index
  )
{
  if (Info == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }

  if (mCurrVarStorageNode == NULL) {
    return VFR_RETURN_GET_NVVARSTORE_ERROR;
  }
  
  //
  // Framework Vfr file Index is from 1, but Uefi Vfr file Index is from 0.
  //
  if (VfrCompatibleMode) {
    if (Index == 0) {
      return VFR_RETURN_ERROR_ARRARY_NUM;
    }
    Index --;
  }

  Info->mInfo.mVarName = mCurrVarStorageNode->mStorageInfo.mNameSpace.mNameTable[Index];

  return VFR_RETURN_SUCCESS;
}

SVfrDefaultStoreNode::SVfrDefaultStoreNode (
  IN EFI_IFR_DEFAULTSTORE *ObjBinAddr,
  IN CHAR8                *RefName, 
  IN EFI_STRING_ID        DefaultStoreNameId, 
  IN UINT16               DefaultId
  )
{
  mObjBinAddr = ObjBinAddr;

  if (RefName != NULL) {
    mRefName          = new CHAR8[strlen (RefName) + 1];
    strcpy (mRefName, RefName);
  } else {
    mRefName          = NULL;
  }

  mNext               = NULL;
  mDefaultId          = DefaultId;
  mDefaultStoreNameId = DefaultStoreNameId;
}

SVfrDefaultStoreNode::~SVfrDefaultStoreNode (
  VOID
  )
{
  if (mRefName != NULL) {
    delete mRefName;
  }
}

CVfrDefaultStore::CVfrDefaultStore (
  VOID
  )
{
  mDefaultStoreList = NULL;
}

CVfrDefaultStore::~CVfrDefaultStore (
  VOID
  )
{
  SVfrDefaultStoreNode *pTmp = NULL;

  while (mDefaultStoreList != NULL) {
    pTmp = mDefaultStoreList;
    mDefaultStoreList = mDefaultStoreList->mNext;
    delete pTmp;
  }
}

EFI_VFR_RETURN_CODE
CVfrDefaultStore::RegisterDefaultStore (
  IN CHAR8                *ObjBinAddr,
  IN CHAR8                *RefName,
  IN EFI_STRING_ID        DefaultStoreNameId,
  IN UINT16               DefaultId
  )
{
  SVfrDefaultStoreNode *pNode = NULL;

  if (RefName == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }

  for (pNode = mDefaultStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (strcmp (pNode->mRefName, RefName) == 0) {
      return VFR_RETURN_REDEFINED;
    }
  }

  if ((pNode = new SVfrDefaultStoreNode ((EFI_IFR_DEFAULTSTORE *)ObjBinAddr, RefName, DefaultStoreNameId, DefaultId)) == NULL) {
    return VFR_RETURN_OUT_FOR_RESOURCES;
  }

  pNode->mNext               = mDefaultStoreList;
  mDefaultStoreList          = pNode;

  return VFR_RETURN_SUCCESS;
}

/*
 * assign new reference name or new default store name id only if 
 * the original is invalid
 */
EFI_VFR_RETURN_CODE
CVfrDefaultStore::ReRegisterDefaultStoreById (
  IN UINT16          DefaultId,
  IN CHAR8           *RefName,
  IN EFI_STRING_ID   DefaultStoreNameId
  )
{
  SVfrDefaultStoreNode *pNode = NULL;

  for (pNode = mDefaultStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mDefaultId == DefaultId) {
      break;
    }
  }

  if (pNode == NULL) {
    return VFR_RETURN_UNDEFINED;
  } else {
    if (pNode->mDefaultStoreNameId == EFI_STRING_ID_INVALID) {
      pNode->mDefaultStoreNameId  = DefaultStoreNameId;
      if (pNode->mObjBinAddr != NULL) {
        pNode->mObjBinAddr->DefaultName = DefaultStoreNameId;
      }
    } else {
      return VFR_RETURN_REDEFINED;
    }

    if (RefName != NULL) {
      delete pNode->mRefName;
      pNode->mRefName = new CHAR8[strlen (RefName) + 1];
      if (pNode->mRefName != NULL) {
        strcpy (pNode->mRefName, RefName);
      }
    }
  }

  return VFR_RETURN_SUCCESS;
}

BOOLEAN
CVfrDefaultStore::DefaultIdRegistered (
  IN UINT16          DefaultId
  )
{
  SVfrDefaultStoreNode *pNode = NULL;

  for (pNode = mDefaultStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mDefaultId == DefaultId) {
      return TRUE;
    }
  }

  return FALSE;
}

EFI_VFR_RETURN_CODE
CVfrDefaultStore::GetDefaultId (
  IN  CHAR8           *RefName,
  OUT UINT16          *DefaultId
  )
{
  SVfrDefaultStoreNode *pTmp = NULL;

  if (DefaultId == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }

  for (pTmp = mDefaultStoreList; pTmp != NULL; pTmp = pTmp->mNext) {
    if (strcmp (pTmp->mRefName, RefName) == 0) {
      *DefaultId = pTmp->mDefaultId;
      return VFR_RETURN_SUCCESS;
    }
  }

  return VFR_RETURN_UNDEFINED;
}

EFI_VFR_RETURN_CODE
CVfrDefaultStore::BufferVarStoreAltConfigAdd (
  IN EFI_VARSTORE_ID    DefaultId,
  IN EFI_VARSTORE_INFO  &Info,
  IN CHAR8              *VarStoreName,
  IN EFI_GUID           *VarStoreGuid,
  IN UINT8              Type,
  IN EFI_IFR_TYPE_VALUE Value
  )
{
  SVfrDefaultStoreNode  *pNode = NULL;
  CHAR8                 NewAltCfg[2 * 2 * sizeof (UINT16) + 1] = {0,};
  INTN                  Returnvalue = 0;

  if (VarStoreName == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }

  for (pNode = mDefaultStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mDefaultId == DefaultId) {
      break;
    }
  }

  if (pNode == NULL) {
    return VFR_RETURN_UNDEFINED;
  }

  gCVfrBufferConfig.Open ();

  sprintf (NewAltCfg, "%04x", pNode->mDefaultId);
  if ((Returnvalue = gCVfrBufferConfig.Select(VarStoreName, VarStoreGuid)) == 0) {
    if ((Returnvalue = gCVfrBufferConfig.Write ('a', VarStoreName, VarStoreGuid, NewAltCfg, Type, Info.mInfo.mVarOffset, Info.mVarTotalSize, Value)) != 0) {
      goto WriteError;
    }
  }
  
  gCVfrBufferConfig.Close ();

  return VFR_RETURN_SUCCESS;

WriteError:
  gCVfrBufferConfig.Close ();
  return (EFI_VFR_RETURN_CODE)Returnvalue;
}

SVfrRuleNode::SVfrRuleNode (
  IN CHAR8        *RuleName,
  IN UINT8       RuleId
  )
{
  if (RuleName != NULL) {
    mRuleName = new CHAR8[strlen (RuleName) + 1];
    strcpy (mRuleName, RuleName);
  } else {
    mRuleName = NULL;
  }

  mNext       = NULL;
  mRuleId     = RuleId;
}

SVfrRuleNode::~SVfrRuleNode (
  VOID
  )
{
  if (mRuleName != NULL) {
    delete mRuleName;
  }
}

CVfrRulesDB::CVfrRulesDB ()
{
  mRuleList   = NULL;
  mFreeRuleId = EFI_VARSTORE_ID_START;
}

CVfrRulesDB::~CVfrRulesDB ()
{
  SVfrRuleNode *pNode;

  while(mRuleList != NULL) {
    pNode = mRuleList;
    mRuleList = mRuleList->mNext;
    delete pNode;
  }
}

VOID
CVfrRulesDB::RegisterRule (
  IN CHAR8  *RuleName
  )
{
  SVfrRuleNode *pNew;

  if (RuleName == NULL) {
    return ;
  }

  if ((pNew = new SVfrRuleNode (RuleName, mFreeRuleId)) == NULL) {
    return ;
  }

  mFreeRuleId++;

  pNew->mNext = mRuleList;
  mRuleList   = pNew;
}

UINT8
CVfrRulesDB::GetRuleId (
  IN CHAR8  *RuleName
  )
{
  SVfrRuleNode *pNode;

  if (RuleName == NULL) {
    return EFI_RULE_ID_INVALID;
  }

  for (pNode = mRuleList; pNode != NULL; pNode = pNode->mNext) {
    if (strcmp (pNode->mRuleName, RuleName) == 0) {
      return pNode->mRuleId;
    }
  }

  return EFI_RULE_ID_INVALID;
}

CVfrRulesDB gCVfrRulesDB;

EFI_VARSTORE_INFO::EFI_VARSTORE_INFO (
  VOID
  )
{
  mVarStoreId      = EFI_VARSTORE_ID_INVALID;
  mInfo.mVarName   = EFI_STRING_ID_INVALID;
  mInfo.mVarOffset = EFI_VAROFFSET_INVALID;
  mVarType         = EFI_IFR_TYPE_OTHER;
  mVarTotalSize    = 0;
}

EFI_VARSTORE_INFO::EFI_VARSTORE_INFO (
  IN EFI_VARSTORE_INFO &Info
  )
{
  mVarStoreId      = Info.mVarStoreId;
  mInfo.mVarName   = Info.mInfo.mVarName;
  mInfo.mVarOffset = Info.mInfo.mVarOffset;
  mVarType         = Info.mVarType;
  mVarTotalSize    = Info.mVarTotalSize;
}

BOOLEAN
EFI_VARSTORE_INFO::operator == (
  IN EFI_VARSTORE_INFO  *Info
  )
{
  if ((mVarStoreId == Info->mVarStoreId) &&
  	  (mInfo.mVarName == Info->mInfo.mVarName) &&
      (mInfo.mVarOffset == Info->mInfo.mVarOffset) &&
      (mVarType == Info->mVarType) &&
      (mVarTotalSize == Info->mVarTotalSize)) {
    return TRUE;
  }

  return FALSE;
}

static EFI_VARSTORE_INFO gEfiInvalidVarStoreInfo;

EFI_QUESTION_ID
CVfrQuestionDB::GetFreeQuestionId (
  VOID
  )
{
  UINT32  Index, Mask, Offset;

  for (Index = 0; Index < EFI_FREE_QUESTION_ID_BITMAP_SIZE; Index++) {
    if (mFreeQIdBitMap[Index] != 0xFFFFFFFF) {
      break;
    }
  }

  for (Offset = 0, Mask = 0x80000000; Mask != 0; Mask >>= 1, Offset++) {
    if ((mFreeQIdBitMap[Index] & Mask) == 0) {
      mFreeQIdBitMap[Index] |= Mask;
      return (EFI_QUESTION_ID)((Index << EFI_BITS_SHIFT_PER_UINT32) + Offset);
    }
  }

  return EFI_QUESTION_ID_INVALID;
}

BOOLEAN
CVfrQuestionDB::ChekQuestionIdFree (
  IN EFI_QUESTION_ID QId
  )
{
  UINT32 Index  = (QId / EFI_BITS_PER_UINT32);
  UINT32 Offset = (QId % EFI_BITS_PER_UINT32);

  return (mFreeQIdBitMap[Index] & (0x80000000 >> Offset)) == 0;
}

VOID
CVfrQuestionDB::MarkQuestionIdUsed (
  IN EFI_QUESTION_ID QId
  )
{
  UINT32 Index  = (QId / EFI_BITS_PER_UINT32);
  UINT32 Offset = (QId % EFI_BITS_PER_UINT32);

  mFreeQIdBitMap[Index] |= (0x80000000 >> Offset);
}

VOID
CVfrQuestionDB::MarkQuestionIdUnused (
  IN EFI_QUESTION_ID QId
  )
{
  UINT32 Index  = (QId / EFI_BITS_PER_UINT32);
  UINT32 Offset = (QId % EFI_BITS_PER_UINT32);

  mFreeQIdBitMap[Index] &= ~(0x80000000 >> Offset);
}

SVfrQuestionNode::SVfrQuestionNode (
  IN CHAR8  *Name,
  IN CHAR8  *VarIdStr,
  IN UINT32 BitMask
  )
{
  mName       = NULL;
  mVarIdStr   = NULL;
  mQuestionId = EFI_QUESTION_ID_INVALID;
  mBitMask    = BitMask;
  mNext       = NULL;
  mQtype      = QUESTION_NORMAL;

  if (Name == NULL) {
    mName = new CHAR8[strlen ("$DEFAULT") + 1];
    strcpy (mName, "$DEFAULT");
  } else {
    mName = new CHAR8[strlen (Name) + 1];
    strcpy (mName, Name);
  }

  if (VarIdStr != NULL) {
    mVarIdStr = new CHAR8[strlen (VarIdStr) + 1];
    strcpy (mVarIdStr, VarIdStr);
  } else {
    mVarIdStr = new CHAR8[strlen ("$") + 1];
    strcpy (mVarIdStr, "$");
  }
}

SVfrQuestionNode::~SVfrQuestionNode (
  VOID
  )
{
  if (mName != NULL) {
    delete mName;
  }

  if (mVarIdStr != NULL) {
    delete mVarIdStr;
  }
}

CVfrQuestionDB::CVfrQuestionDB ()
{
  UINT32 Index;

  for (Index = 0; Index < EFI_FREE_QUESTION_ID_BITMAP_SIZE; Index++) {
    mFreeQIdBitMap[Index] = 0;
  }

  // Question ID 0 is reserved.
  mFreeQIdBitMap[0] = 0x80000000;
  mQuestionList     = NULL;
}

CVfrQuestionDB::~CVfrQuestionDB ()
{
  SVfrQuestionNode     *pNode;

  while (mQuestionList != NULL) {
    pNode = mQuestionList;
    mQuestionList = mQuestionList->mNext;
    delete pNode;
  }
}

//
// Reset to init state
//
VOID
CVfrQuestionDB::ResetInit(
  IN VOID
  )
{
  UINT32               Index;
  SVfrQuestionNode     *pNode;

  while (mQuestionList != NULL) {
    pNode = mQuestionList;
    mQuestionList = mQuestionList->mNext;
    delete pNode;
  }

  for (Index = 0; Index < EFI_FREE_QUESTION_ID_BITMAP_SIZE; Index++) {
    mFreeQIdBitMap[Index] = 0;
  }

  // Question ID 0 is reserved.
  mFreeQIdBitMap[0] = 0x80000000;
  mQuestionList     = NULL;   
}

VOID
CVfrQuestionDB::PrintAllQuestion (
  VOID
  )
{
  SVfrQuestionNode *pNode = NULL;

  for (pNode = mQuestionList; pNode != NULL; pNode = pNode->mNext) {
    printf ("Question VarId is %s and QuesitonId is 0x%x\n", pNode->mVarIdStr, pNode->mQuestionId);
  }
}

EFI_VFR_RETURN_CODE
CVfrQuestionDB::RegisterQuestion (
  IN     CHAR8             *Name,
  IN     CHAR8             *VarIdStr,
  IN OUT EFI_QUESTION_ID   &QuestionId
  )
{
  SVfrQuestionNode *pNode = NULL;

  if ((Name != NULL) && (FindQuestion(Name) == VFR_RETURN_SUCCESS)) {
    return VFR_RETURN_REDEFINED;
  }

  if ((pNode = new SVfrQuestionNode (Name, VarIdStr)) == NULL) {
    return VFR_RETURN_OUT_FOR_RESOURCES;
  }

  if (QuestionId == EFI_QUESTION_ID_INVALID) {
    QuestionId = GetFreeQuestionId ();
  } else {
    //
    // For Framework Vfr, don't check question ID conflict.
    //
    if (!VfrCompatibleMode && ChekQuestionIdFree (QuestionId) == FALSE) {
      delete pNode;
      return VFR_RETURN_QUESTIONID_REDEFINED;
    }
    MarkQuestionIdUsed (QuestionId);
  }
  pNode->mQuestionId = QuestionId;

  pNode->mNext       = mQuestionList;
  mQuestionList      = pNode;

  gCFormPkg.DoPendingAssign (VarIdStr, (VOID *)&QuestionId, sizeof(EFI_QUESTION_ID));

  return VFR_RETURN_SUCCESS;
}

VOID
CVfrQuestionDB::RegisterOldDateQuestion (
  IN     CHAR8            *YearVarId,
  IN     CHAR8            *MonthVarId,
  IN     CHAR8            *DayVarId,
  IN OUT EFI_QUESTION_ID &QuestionId
  )
{
  SVfrQuestionNode *pNode[3] = {NULL, };
  UINT32           Index;

  if ((YearVarId == NULL) || (MonthVarId == NULL) || (DayVarId == NULL)) {
    return;
  }

  if ((pNode[0] = new SVfrQuestionNode (NULL, YearVarId, DATE_YEAR_BITMASK)) == NULL) {
    goto Err;
  }
  if ((pNode[1] = new SVfrQuestionNode (NULL, MonthVarId, DATE_MONTH_BITMASK)) == NULL) {
    goto Err;
  }
  if ((pNode[2] = new SVfrQuestionNode (NULL, DayVarId, DATE_DAY_BITMASK)) == NULL) {
    goto Err;
  }

  if (QuestionId == EFI_QUESTION_ID_INVALID) {
    QuestionId = GetFreeQuestionId ();
  } else {
    if (ChekQuestionIdFree (QuestionId) == FALSE) {
      goto Err;
    }
    MarkQuestionIdUsed (QuestionId);
  }

  pNode[0]->mQuestionId = QuestionId;
  pNode[1]->mQuestionId = QuestionId;
  pNode[2]->mQuestionId = QuestionId;
  pNode[0]->mQtype      = QUESTION_DATE;
  pNode[1]->mQtype      = QUESTION_DATE;
  pNode[2]->mQtype      = QUESTION_DATE;
  pNode[0]->mNext       = pNode[1];
  pNode[1]->mNext       = pNode[2];
  pNode[2]->mNext       = mQuestionList;
  mQuestionList         = pNode[0];

  gCFormPkg.DoPendingAssign (YearVarId, (VOID *)&QuestionId, sizeof(EFI_QUESTION_ID));
  gCFormPkg.DoPendingAssign (MonthVarId, (VOID *)&QuestionId, sizeof(EFI_QUESTION_ID));
  gCFormPkg.DoPendingAssign (DayVarId, (VOID *)&QuestionId, sizeof(EFI_QUESTION_ID));

  return;

Err:
  for (Index = 0; Index < 3; Index++) {
    if (pNode[Index] != NULL) {
      delete pNode[Index];
    }
  }
  QuestionId = EFI_QUESTION_ID_INVALID;
}

VOID
CVfrQuestionDB::RegisterNewDateQuestion (
  IN     CHAR8            *Name,
  IN     CHAR8            *BaseVarId,
  IN OUT EFI_QUESTION_ID &QuestionId
  )
{
  SVfrQuestionNode     *pNode[3] = {NULL, };
  UINT32               Len;
  CHAR8                *VarIdStr[3] = {NULL, };
  CHAR8                 Index;

  if (BaseVarId == NULL && Name == NULL) {
    if (QuestionId == EFI_QUESTION_ID_INVALID) {
      QuestionId = GetFreeQuestionId ();
    } else {
      if (ChekQuestionIdFree (QuestionId) == FALSE) {
        goto Err;
      }
      MarkQuestionIdUsed (QuestionId);
    }
    return;
  }

  if (BaseVarId != NULL) {
    Len = strlen (BaseVarId);

    VarIdStr[0] = new CHAR8[Len + strlen (".Year") + 1];
    if (VarIdStr[0] != NULL) {
      strcpy (VarIdStr[0], BaseVarId);
      strcat (VarIdStr[0], ".Year");
    }
    VarIdStr[1] = new CHAR8[Len + strlen (".Month") + 1];
    if (VarIdStr[1] != NULL) {
      strcpy (VarIdStr[1], BaseVarId);
      strcat (VarIdStr[1], ".Month");
    }
    VarIdStr[2] = new CHAR8[Len + strlen (".Day") + 1];
    if (VarIdStr[2] != NULL) {
      strcpy (VarIdStr[2], BaseVarId);
      strcat (VarIdStr[2], ".Day");
    }
  } else {
    Len = strlen (Name);

    VarIdStr[0] = new CHAR8[Len + strlen (".Year") + 1];
    if (VarIdStr[0] != NULL) {
      strcpy (VarIdStr[0], Name);
      strcat (VarIdStr[0], ".Year");
    }
    VarIdStr[1] = new CHAR8[Len + strlen (".Month") + 1];
    if (VarIdStr[1] != NULL) {
      strcpy (VarIdStr[1], Name);
      strcat (VarIdStr[1], ".Month");
    }
    VarIdStr[2] = new CHAR8[Len + strlen (".Day") + 1];
    if (VarIdStr[2] != NULL) {
      strcpy (VarIdStr[2], Name);
      strcat (VarIdStr[2], ".Day");
    }
  }

  if ((pNode[0] = new SVfrQuestionNode (Name, VarIdStr[0], DATE_YEAR_BITMASK)) == NULL) {
    goto Err;
  }
  if ((pNode[1] = new SVfrQuestionNode (Name, VarIdStr[1], DATE_MONTH_BITMASK)) == NULL) {
    goto Err;
  }
  if ((pNode[2] = new SVfrQuestionNode (Name, VarIdStr[2], DATE_DAY_BITMASK)) == NULL) {
    goto Err;
  }

  if (QuestionId == EFI_QUESTION_ID_INVALID) {
    QuestionId = GetFreeQuestionId ();
  } else {
    if (ChekQuestionIdFree (QuestionId) == FALSE) {
      goto Err;
    }
    MarkQuestionIdUsed (QuestionId);
  }

  pNode[0]->mQuestionId = QuestionId;
  pNode[1]->mQuestionId = QuestionId;
  pNode[2]->mQuestionId = QuestionId;
  pNode[0]->mQtype      = QUESTION_DATE;
  pNode[1]->mQtype      = QUESTION_DATE;
  pNode[2]->mQtype      = QUESTION_DATE;
  pNode[0]->mNext       = pNode[1];
  pNode[1]->mNext       = pNode[2];
  pNode[2]->mNext       = mQuestionList;
  mQuestionList         = pNode[0];

  for (Index = 0; Index < 3; Index++) {
    if (VarIdStr[Index] != NULL) {
      delete VarIdStr[Index];
    }
  }

  gCFormPkg.DoPendingAssign (VarIdStr[0], (VOID *)&QuestionId, sizeof(EFI_QUESTION_ID));
  gCFormPkg.DoPendingAssign (VarIdStr[1], (VOID *)&QuestionId, sizeof(EFI_QUESTION_ID));
  gCFormPkg.DoPendingAssign (VarIdStr[2], (VOID *)&QuestionId, sizeof(EFI_QUESTION_ID));

  return;

Err:
  for (Index = 0; Index < 3; Index++) {
    if (pNode[Index] != NULL) {
      delete pNode[Index];
    }

    if (VarIdStr[Index] != NULL) {
      delete VarIdStr[Index];
    }
  }
}

VOID
CVfrQuestionDB::RegisterOldTimeQuestion (
  IN     CHAR8            *HourVarId,
  IN     CHAR8            *MinuteVarId,
  IN     CHAR8            *SecondVarId,
  IN OUT EFI_QUESTION_ID &QuestionId
  )
{
  SVfrQuestionNode *pNode[3] = {NULL, };
  UINT32           Index;

  if ((HourVarId == NULL) || (MinuteVarId == NULL) || (SecondVarId == NULL)) {
    return;
  }

  if ((pNode[0] = new SVfrQuestionNode (NULL, HourVarId, TIME_HOUR_BITMASK)) == NULL) {
    goto Err;
  }
  if ((pNode[1] = new SVfrQuestionNode (NULL, MinuteVarId, TIME_MINUTE_BITMASK)) == NULL) {
    goto Err;
  }
  if ((pNode[2] = new SVfrQuestionNode (NULL, SecondVarId, TIME_SECOND_BITMASK)) == NULL) {
    goto Err;
  }

  if (QuestionId == EFI_QUESTION_ID_INVALID) {
    QuestionId = GetFreeQuestionId ();
  } else {
    if (ChekQuestionIdFree (QuestionId) == FALSE) {
      goto Err;
    }
    MarkQuestionIdUsed (QuestionId);
  }

  pNode[0]->mQuestionId = QuestionId;
  pNode[1]->mQuestionId = QuestionId;
  pNode[2]->mQuestionId = QuestionId;
  pNode[0]->mQtype      = QUESTION_TIME;
  pNode[1]->mQtype      = QUESTION_TIME;
  pNode[2]->mQtype      = QUESTION_TIME;
  pNode[0]->mNext       = pNode[1];
  pNode[1]->mNext       = pNode[2];
  pNode[2]->mNext       = mQuestionList;
  mQuestionList         = pNode[0];

  gCFormPkg.DoPendingAssign (HourVarId, (VOID *)&QuestionId, sizeof(EFI_QUESTION_ID));
  gCFormPkg.DoPendingAssign (MinuteVarId, (VOID *)&QuestionId, sizeof(EFI_QUESTION_ID));
  gCFormPkg.DoPendingAssign (SecondVarId, (VOID *)&QuestionId, sizeof(EFI_QUESTION_ID));

  return;

Err:
  for (Index = 0; Index < 3; Index++) {
    if (pNode[Index] != NULL) {
      delete pNode[Index];
    }
  }
  QuestionId = EFI_QUESTION_ID_INVALID;
}

VOID
CVfrQuestionDB::RegisterNewTimeQuestion (
  IN     CHAR8           *Name,
  IN     CHAR8           *BaseVarId,
  IN OUT EFI_QUESTION_ID &QuestionId
  )
{
  SVfrQuestionNode     *pNode[3] = {NULL, };
  UINT32               Len;
  CHAR8                *VarIdStr[3] = {NULL, };
  CHAR8                 Index;

  if (BaseVarId == NULL && Name == NULL) {
    if (QuestionId == EFI_QUESTION_ID_INVALID) {
      QuestionId = GetFreeQuestionId ();
    } else {
      if (ChekQuestionIdFree (QuestionId) == FALSE) {
        goto Err;
      }
      MarkQuestionIdUsed (QuestionId);
    }
    return;
  }

  if (BaseVarId != NULL) {
    Len = strlen (BaseVarId);

    VarIdStr[0] = new CHAR8[Len + strlen (".Hour") + 1];
    if (VarIdStr[0] != NULL) {
      strcpy (VarIdStr[0], BaseVarId);
      strcat (VarIdStr[0], ".Hour");
    }
    VarIdStr[1] = new CHAR8[Len + strlen (".Minute") + 1];
    if (VarIdStr[1] != NULL) {
      strcpy (VarIdStr[1], BaseVarId);
      strcat (VarIdStr[1], ".Minute");
    }
    VarIdStr[2] = new CHAR8[Len + strlen (".Second") + 1];
    if (VarIdStr[2] != NULL) {
      strcpy (VarIdStr[2], BaseVarId);
      strcat (VarIdStr[2], ".Second");
    }
  } else {
    Len = strlen (Name);

    VarIdStr[0] = new CHAR8[Len + strlen (".Hour") + 1];
    if (VarIdStr[0] != NULL) {
      strcpy (VarIdStr[0], Name);
      strcat (VarIdStr[0], ".Hour");
    }
    VarIdStr[1] = new CHAR8[Len + strlen (".Minute") + 1];
    if (VarIdStr[1] != NULL) {
      strcpy (VarIdStr[1], Name);
      strcat (VarIdStr[1], ".Minute");
    }
    VarIdStr[2] = new CHAR8[Len + strlen (".Second") + 1];
    if (VarIdStr[2] != NULL) {
      strcpy (VarIdStr[2], Name);
      strcat (VarIdStr[2], ".Second");
    }
  }

  if ((pNode[0] = new SVfrQuestionNode (Name, VarIdStr[0], TIME_HOUR_BITMASK)) == NULL) {
    goto Err;
  }
  if ((pNode[1] = new SVfrQuestionNode (Name, VarIdStr[1], TIME_MINUTE_BITMASK)) == NULL) {
    goto Err;
  }
  if ((pNode[2] = new SVfrQuestionNode (Name, VarIdStr[2], TIME_SECOND_BITMASK)) == NULL) {
    goto Err;
  }

  if (QuestionId == EFI_QUESTION_ID_INVALID) {
    QuestionId = GetFreeQuestionId ();
  } else {
    if (ChekQuestionIdFree (QuestionId) == FALSE) {
      goto Err;
    }
    MarkQuestionIdUsed (QuestionId);
  }

  pNode[0]->mQuestionId = QuestionId;
  pNode[1]->mQuestionId = QuestionId;
  pNode[2]->mQuestionId = QuestionId;
  pNode[0]->mQtype      = QUESTION_TIME;
  pNode[1]->mQtype      = QUESTION_TIME;
  pNode[2]->mQtype      = QUESTION_TIME;
  pNode[0]->mNext       = pNode[1];
  pNode[1]->mNext       = pNode[2];
  pNode[2]->mNext       = mQuestionList;
  mQuestionList         = pNode[0];

  for (Index = 0; Index < 3; Index++) {
    if (VarIdStr[Index] != NULL) {
      delete VarIdStr[Index];
    }
  }

  gCFormPkg.DoPendingAssign (VarIdStr[0], (VOID *)&QuestionId, sizeof(EFI_QUESTION_ID));
  gCFormPkg.DoPendingAssign (VarIdStr[1], (VOID *)&QuestionId, sizeof(EFI_QUESTION_ID));
  gCFormPkg.DoPendingAssign (VarIdStr[2], (VOID *)&QuestionId, sizeof(EFI_QUESTION_ID));

  return;

Err:
  for (Index = 0; Index < 3; Index++) {
    if (pNode[Index] != NULL) {
      delete pNode[Index];
    }

    if (VarIdStr[Index] != NULL) {
      delete VarIdStr[Index];
    }
  }
}

VOID 
CVfrQuestionDB::RegisterRefQuestion (
  IN     CHAR8           *Name,
  IN     CHAR8           *BaseVarId,
  IN OUT EFI_QUESTION_ID &QuestionId
  )
{
  SVfrQuestionNode     *pNode[4] = {NULL, };
  UINT32               Len;
  CHAR8                *VarIdStr[4] = {NULL, };
  CHAR8                 Index;

  if (BaseVarId == NULL && Name == NULL) {
    return;
  }

  if (BaseVarId != NULL) {
    Len = strlen (BaseVarId);

    VarIdStr[0] = new CHAR8[Len + strlen (".QuestionId") + 1];
    if (VarIdStr[0] != NULL) {
      strcpy (VarIdStr[0], BaseVarId);
      strcat (VarIdStr[0], ".QuestionId");
    }
    VarIdStr[1] = new CHAR8[Len + strlen (".FormId") + 1];
    if (VarIdStr[1] != NULL) {
      strcpy (VarIdStr[1], BaseVarId);
      strcat (VarIdStr[1], ".FormId");
    }
    VarIdStr[2] = new CHAR8[Len + strlen (".FormSetGuid") + 1];
    if (VarIdStr[2] != NULL) {
      strcpy (VarIdStr[2], BaseVarId);
      strcat (VarIdStr[2], ".FormSetGuid");
    }
    VarIdStr[3] = new CHAR8[Len + strlen (".DevicePath") + 1];
    if (VarIdStr[3] != NULL) {
      strcpy (VarIdStr[3], BaseVarId);
      strcat (VarIdStr[3], ".DevicePath");
    }
  } else {
    Len = strlen (Name);

    VarIdStr[0] = new CHAR8[Len + strlen (".QuestionId") + 1];
    if (VarIdStr[0] != NULL) {
      strcpy (VarIdStr[0], Name);
      strcat (VarIdStr[0], ".QuestionId");
    }
    VarIdStr[1] = new CHAR8[Len + strlen (".FormId") + 1];
    if (VarIdStr[1] != NULL) {
      strcpy (VarIdStr[1], Name);
      strcat (VarIdStr[1], ".FormId");
    }
    VarIdStr[2] = new CHAR8[Len + strlen (".FormSetGuid") + 1];
    if (VarIdStr[2] != NULL) {
      strcpy (VarIdStr[2], Name);
      strcat (VarIdStr[2], ".FormSetGuid");
    }
    VarIdStr[3] = new CHAR8[Len + strlen (".DevicePath") + 1];
    if (VarIdStr[3] != NULL) {
      strcpy (VarIdStr[3], Name);
      strcat (VarIdStr[3], ".DevicePath");
    }
  }

  if ((pNode[0] = new SVfrQuestionNode (Name, VarIdStr[0])) == NULL) {
    goto Err;
  }
  if ((pNode[1] = new SVfrQuestionNode (Name, VarIdStr[1])) == NULL) {
    goto Err;
  }
  if ((pNode[2] = new SVfrQuestionNode (Name, VarIdStr[2])) == NULL) {
    goto Err;
  }
  if ((pNode[3] = new SVfrQuestionNode (Name, VarIdStr[3])) == NULL) {
    goto Err;
  }

  if (QuestionId == EFI_QUESTION_ID_INVALID) {
    QuestionId = GetFreeQuestionId ();
  } else {
    if (ChekQuestionIdFree (QuestionId) == FALSE) {
      goto Err;
    }
    MarkQuestionIdUsed (QuestionId);
  }

  pNode[0]->mQuestionId = QuestionId;
  pNode[1]->mQuestionId = QuestionId;
  pNode[2]->mQuestionId = QuestionId;
  pNode[3]->mQuestionId = QuestionId;  
  pNode[0]->mQtype      = QUESTION_REF;
  pNode[1]->mQtype      = QUESTION_REF;
  pNode[2]->mQtype      = QUESTION_REF;
  pNode[3]->mQtype      = QUESTION_REF;  
  pNode[0]->mNext       = pNode[1];
  pNode[1]->mNext       = pNode[2];
  pNode[2]->mNext       = pNode[3];
  pNode[3]->mNext       = mQuestionList;  
  mQuestionList         = pNode[0];

  gCFormPkg.DoPendingAssign (VarIdStr[0], (VOID *)&QuestionId, sizeof(EFI_QUESTION_ID));
  gCFormPkg.DoPendingAssign (VarIdStr[1], (VOID *)&QuestionId, sizeof(EFI_QUESTION_ID));
  gCFormPkg.DoPendingAssign (VarIdStr[2], (VOID *)&QuestionId, sizeof(EFI_QUESTION_ID));
  gCFormPkg.DoPendingAssign (VarIdStr[3], (VOID *)&QuestionId, sizeof(EFI_QUESTION_ID));

  return;

  Err:
  for (Index = 0; Index < 4; Index++) {
    if (pNode[Index] != NULL) {
      delete pNode[Index];
    }

    if (VarIdStr[Index] != NULL) {
      delete VarIdStr[Index];
    }
  }
}

EFI_VFR_RETURN_CODE
CVfrQuestionDB::UpdateQuestionId (
  IN EFI_QUESTION_ID   QId,
  IN EFI_QUESTION_ID   NewQId
  )
{
  SVfrQuestionNode *pNode = NULL;
  
  if (QId == NewQId) {
    // don't update
    return VFR_RETURN_SUCCESS;
  }
  
  //
  // For Framework Vfr, don't check question ID conflict.
  //  
  if (!VfrCompatibleMode && ChekQuestionIdFree (NewQId) == FALSE) {
    return VFR_RETURN_REDEFINED;
  }

  for (pNode = mQuestionList; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mQuestionId == QId) {
      break;
    }
  }

  if (pNode == NULL) {
    return VFR_RETURN_UNDEFINED;
  }

  MarkQuestionIdUnused (QId);
  pNode->mQuestionId = NewQId;
  MarkQuestionIdUsed (NewQId);

  gCFormPkg.DoPendingAssign (pNode->mVarIdStr, (VOID *)&NewQId, sizeof(EFI_QUESTION_ID));

  return VFR_RETURN_SUCCESS;
}

VOID
CVfrQuestionDB::GetQuestionId (
  IN  CHAR8             *Name,
  IN  CHAR8             *VarIdStr,
  OUT EFI_QUESTION_ID   &QuestionId,
  OUT UINT32            &BitMask,
  OUT EFI_QUESION_TYPE  *QType
  )
{
  SVfrQuestionNode *pNode;

  QuestionId = EFI_QUESTION_ID_INVALID;
  BitMask    = 0x00000000;
  if (QType != NULL) {
    *QType = QUESTION_NORMAL;
  }

  if ((Name == NULL) && (VarIdStr == NULL)) {
    return ;
  }

  for (pNode = mQuestionList; pNode != NULL; pNode = pNode->mNext) {
    if (Name != NULL) {
      if (strcmp (pNode->mName, Name) != 0) {
        continue;
      }
    }

    if (VarIdStr != NULL) {
      if (strcmp (pNode->mVarIdStr, VarIdStr) != 0) {
        continue;
      }
  	}

    QuestionId = pNode->mQuestionId;
    BitMask    = pNode->mBitMask;
    if (QType != NULL) {
      *QType     = pNode->mQtype;
    }
    break;
  }

  return ;
}

EFI_VFR_RETURN_CODE
CVfrQuestionDB::FindQuestion (
  IN EFI_QUESTION_ID QuestionId
  )
{
  SVfrQuestionNode *pNode;

  if (QuestionId == EFI_QUESTION_ID_INVALID) {
    return VFR_RETURN_INVALID_PARAMETER;
  }

  for (pNode = mQuestionList; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mQuestionId == QuestionId) {
      return VFR_RETURN_SUCCESS;
    }
  }

  return VFR_RETURN_UNDEFINED;
}

EFI_VFR_RETURN_CODE
CVfrQuestionDB::FindQuestion (
  IN CHAR8 *Name
  )
{
  SVfrQuestionNode *pNode;

  if (Name == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }

  for (pNode = mQuestionList; pNode != NULL; pNode = pNode->mNext) {
    if (strcmp (pNode->mName, Name) == 0) {
      return VFR_RETURN_SUCCESS;
    }
  }

  return VFR_RETURN_UNDEFINED;
}

CVfrStringDB::CVfrStringDB ()
{
  mStringFileName = NULL;
}

CVfrStringDB::~CVfrStringDB ()
{
  if (mStringFileName != NULL) {
    delete mStringFileName;
  }
  mStringFileName = NULL;
}


VOID 
CVfrStringDB::SetStringFileName(IN CHAR8 *StringFileName)
{
  UINT32 FileLen = 0;

  if (StringFileName == NULL) {
    return;
  }

  FileLen = strlen (StringFileName) + 1;
  mStringFileName = new CHAR8[FileLen];
  if (mStringFileName == NULL) {
    return;
  }

  strcpy (mStringFileName, StringFileName);
  mStringFileName[FileLen - 1] = '\0';
}


/**
  Returns TRUE or FALSE whether SupportedLanguages contains the best matching language 
  from a set of supported languages.

  @param[in]  SupportedLanguages  A pointer to a Null-terminated ASCII string that
                                  contains a set of language codes.
  @param[in]  Language            A variable that contains pointers to Null-terminated
                                  ASCII strings that contain one language codes.

  @retval FALSE   The best matching language could not be found in SupportedLanguages.
  @retval TRUE    The best matching language could be found in SupportedLanguages.

**/
BOOLEAN
CVfrStringDB::GetBestLanguage (
  IN CONST CHAR8  *SupportedLanguages,
  IN CHAR8        *Language
  )
{
  UINTN        CompareLength;
  UINTN        LanguageLength;
  CONST CHAR8  *Supported;

  if (SupportedLanguages == NULL || Language == NULL){
    return FALSE;
  }

  //
  // Determine the length of the first RFC 4646 language code in Language
  //
  for (LanguageLength = 0; Language[LanguageLength] != 0 && Language[LanguageLength] != ';'; LanguageLength++);

  //
  // Trim back the length of Language used until it is empty
  //
  while (LanguageLength > 0) {
    //
    // Loop through all language codes in SupportedLanguages
    //
    for (Supported = SupportedLanguages; *Supported != '\0'; Supported += CompareLength) {
      //
      // Skip ';' characters in Supported
      //
      for (; *Supported != '\0' && *Supported == ';'; Supported++);
      //
      // Determine the length of the next language code in Supported
      //
      for (CompareLength = 0; Supported[CompareLength] != 0 && Supported[CompareLength] != ';'; CompareLength++);
      //
      // If Language is longer than the Supported, then skip to the next language
      //
      if (LanguageLength > CompareLength) {
        continue;
      }

      //
      // See if the first LanguageLength characters in Supported match Language
      //
      if (strncmp (Supported, Language, LanguageLength) == 0) {
        return TRUE;
      }
    }

    //
    // Trim Language from the right to the next '-' character 
    //
    for (LanguageLength--; LanguageLength > 0 && Language[LanguageLength] != '-'; LanguageLength--);
  }

  //
  // No matches were found 
  //
  return FALSE;
}


CHAR8 *
CVfrStringDB::GetVarStoreNameFormStringId (
  IN EFI_STRING_ID StringId
  )
{
  FILE        *pInFile    = NULL;
  UINT32      NameOffset;
  UINT32      Length;
  UINT8       *StringPtr;
  CHAR8       *StringName;
  CHAR16      *UnicodeString;
  CHAR8       *VarStoreName = NULL;
  CHAR8       *DestTmp;
  UINT8       *Current;
  EFI_STATUS  Status;
  CHAR8       LineBuf[EFI_IFR_MAX_LENGTH];
  UINT8       BlockType;
  EFI_HII_STRING_PACKAGE_HDR *PkgHeader;
  
  if (mStringFileName == '\0' ) {
    return NULL;
  }

  if ((pInFile = fopen (LongFilePath (mStringFileName), "rb")) == NULL) {
    return NULL;
  }

  //
  // Get file length.
  //
  fseek (pInFile, 0, SEEK_END);
  Length = ftell (pInFile);
  fseek (pInFile, 0, SEEK_SET);

  //
  // Get file data.
  //
  StringPtr = new UINT8[Length];
  if (StringPtr == NULL) {
    fclose (pInFile);
    return NULL;
  }
  fread ((char *)StringPtr, sizeof (UINT8), Length, pInFile);
  fclose (pInFile);

  PkgHeader = (EFI_HII_STRING_PACKAGE_HDR *) StringPtr;
  //
  // Check the String package.
  //
  if (PkgHeader->Header.Type != EFI_HII_PACKAGE_STRINGS) {
    delete StringPtr;
    return NULL;
  }

  //
  // Search the language, get best language base on RFC 4647 matching algorithm.
  //
  Current = StringPtr;
  while (!GetBestLanguage ("en", PkgHeader->Language)) {
    Current += PkgHeader->Header.Length;
    PkgHeader = (EFI_HII_STRING_PACKAGE_HDR *) Current;
    //
    // If can't find string package base on language, just return the first string package.
    //
    if (Current - StringPtr >= Length) {
      Current = StringPtr;
      PkgHeader = (EFI_HII_STRING_PACKAGE_HDR *) StringPtr;
      break;
    }
  }

  Current += PkgHeader->HdrSize;
  //
  // Find the string block according the stringId.
  //
  Status = FindStringBlock(Current, StringId, &NameOffset, &BlockType);
  if (Status != EFI_SUCCESS) {
    delete StringPtr;
    return NULL;
  }

  //
  // Get varstore name according the string type.
  //
  switch (BlockType) {
  case EFI_HII_SIBT_STRING_SCSU:
  case EFI_HII_SIBT_STRING_SCSU_FONT:
  case EFI_HII_SIBT_STRINGS_SCSU:
  case EFI_HII_SIBT_STRINGS_SCSU_FONT:
    StringName = (CHAR8*)(Current + NameOffset);
    VarStoreName = new CHAR8[strlen(StringName) + 1];
    strcpy (VarStoreName, StringName);
    break;
  case EFI_HII_SIBT_STRING_UCS2:
  case EFI_HII_SIBT_STRING_UCS2_FONT:
  case EFI_HII_SIBT_STRINGS_UCS2:
  case EFI_HII_SIBT_STRINGS_UCS2_FONT:
    UnicodeString = (CHAR16*)(Current + NameOffset);
    Length = GetUnicodeStringTextSize ((UINT8*)UnicodeString) ;
    DestTmp = new CHAR8[Length / 2 + 1];
    VarStoreName = DestTmp;
    while (*UnicodeString != '\0') {
      *(DestTmp++) = (CHAR8) *(UnicodeString++);
    }
    *DestTmp = '\0';
    break;
  default:
    break;
  }

  delete StringPtr;

  return VarStoreName;
}

EFI_STATUS
CVfrStringDB::FindStringBlock (
  IN  UINT8                           *StringData,
  IN  EFI_STRING_ID                   StringId,
  OUT UINT32                          *StringTextOffset,
  OUT UINT8                           *BlockType
  )
{
  UINT8                                *BlockHdr;
  EFI_STRING_ID                        CurrentStringId;
  UINT32                               BlockSize;
  UINT32                               Index;
  UINT8                                *StringTextPtr;
  UINT32                               Offset;
  UINT16                               StringCount;
  UINT16                               SkipCount;
  UINT8                                Length8;
  EFI_HII_SIBT_EXT2_BLOCK              Ext2;
  UINT32                               Length32;
  UINT32                               StringSize;

  CurrentStringId = 1;

  //
  // Parse the string blocks to get the string text and font.
  //
  BlockHdr  = StringData;
  BlockSize = 0;
  Offset    = 0;
  while (*BlockHdr != EFI_HII_SIBT_END) {
    switch (*BlockHdr) {
    case EFI_HII_SIBT_STRING_SCSU:
      Offset = sizeof (EFI_HII_STRING_BLOCK);
      StringTextPtr = BlockHdr + Offset;
      BlockSize += Offset + strlen ((CHAR8 *) StringTextPtr) + 1;
      CurrentStringId++;
      break;

    case EFI_HII_SIBT_STRING_SCSU_FONT:
      Offset = sizeof (EFI_HII_SIBT_STRING_SCSU_FONT_BLOCK) - sizeof (UINT8);
      StringTextPtr = BlockHdr + Offset;
      BlockSize += Offset + strlen ((CHAR8 *) StringTextPtr) + 1;
      CurrentStringId++;
      break;

    case EFI_HII_SIBT_STRINGS_SCSU:
      memcpy (&StringCount, BlockHdr + sizeof (EFI_HII_STRING_BLOCK), sizeof (UINT16));
      StringTextPtr = BlockHdr + sizeof (EFI_HII_SIBT_STRINGS_SCSU_BLOCK) - sizeof (UINT8);
      BlockSize += StringTextPtr - BlockHdr;

      for (Index = 0; Index < StringCount; Index++) {
        BlockSize += strlen ((CHAR8 *) StringTextPtr) + 1;
        if (CurrentStringId == StringId) {
          *BlockType        = *BlockHdr;
          *StringTextOffset = StringTextPtr - StringData;
          return EFI_SUCCESS;
        }
        StringTextPtr = StringTextPtr + strlen ((CHAR8 *) StringTextPtr) + 1;
        CurrentStringId++;
      }
      break;

    case EFI_HII_SIBT_STRINGS_SCSU_FONT:
      memcpy (
        &StringCount,
        BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8),
        sizeof (UINT16)
        );
      StringTextPtr = BlockHdr + sizeof (EFI_HII_SIBT_STRINGS_SCSU_FONT_BLOCK) - sizeof (UINT8);
      BlockSize += StringTextPtr - BlockHdr;

      for (Index = 0; Index < StringCount; Index++) {
        BlockSize += strlen ((CHAR8 *) StringTextPtr) + 1;
        if (CurrentStringId == StringId) {
          *BlockType        = *BlockHdr;
          *StringTextOffset = StringTextPtr - StringData;
          return EFI_SUCCESS;
        }
        StringTextPtr = StringTextPtr + strlen ((CHAR8 *) StringTextPtr) + 1;
        CurrentStringId++;
      }
      break;

    case EFI_HII_SIBT_STRING_UCS2:
      Offset        = sizeof (EFI_HII_STRING_BLOCK);
      StringTextPtr = BlockHdr + Offset;
      //
      // Use StringSize to store the size of the specified string, including the NULL
      // terminator.
      //
      StringSize = GetUnicodeStringTextSize (StringTextPtr);
      BlockSize += Offset + StringSize;
      CurrentStringId++;
      break;

    case EFI_HII_SIBT_STRING_UCS2_FONT:
      Offset = sizeof (EFI_HII_SIBT_STRING_UCS2_FONT_BLOCK)  - sizeof (CHAR16);
      StringTextPtr = BlockHdr + Offset;
      //
      // Use StrSize to store the size of the specified string, including the NULL
      // terminator.
      //
      StringSize = GetUnicodeStringTextSize (StringTextPtr);
      BlockSize += Offset + StringSize;
      CurrentStringId++;
      break;

    case EFI_HII_SIBT_STRINGS_UCS2:
      Offset = sizeof (EFI_HII_SIBT_STRINGS_UCS2_BLOCK) - sizeof (CHAR16);
      StringTextPtr = BlockHdr + Offset;
      BlockSize += Offset;
      memcpy (&StringCount, BlockHdr + sizeof (EFI_HII_STRING_BLOCK), sizeof (UINT16));
      for (Index = 0; Index < StringCount; Index++) {
        StringSize = GetUnicodeStringTextSize (StringTextPtr);
        BlockSize += StringSize;
        if (CurrentStringId == StringId) {
          *BlockType        = *BlockHdr;
          *StringTextOffset = StringTextPtr - StringData;
          return EFI_SUCCESS;
        }
        StringTextPtr = StringTextPtr + StringSize;
        CurrentStringId++;
      }
      break;

    case EFI_HII_SIBT_STRINGS_UCS2_FONT:
      Offset = sizeof (EFI_HII_SIBT_STRINGS_UCS2_FONT_BLOCK) - sizeof (CHAR16);
      StringTextPtr = BlockHdr + Offset;
      BlockSize += Offset;
      memcpy (
        &StringCount,
        BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8),
        sizeof (UINT16)
        );
      for (Index = 0; Index < StringCount; Index++) {
        StringSize = GetUnicodeStringTextSize (StringTextPtr);
        BlockSize += StringSize;
        if (CurrentStringId == StringId) {
          *BlockType        = *BlockHdr;
          *StringTextOffset = StringTextPtr - StringData;
          return EFI_SUCCESS;
        }
        StringTextPtr = StringTextPtr + StringSize;
        CurrentStringId++;
      }
      break;

    case EFI_HII_SIBT_DUPLICATE:
      if (CurrentStringId == StringId) {
        //
        // Incoming StringId is an id of a duplicate string block.
        // Update the StringId to be the previous string block.
        // Go back to the header of string block to search.
        //
        memcpy (
          &StringId,
          BlockHdr + sizeof (EFI_HII_STRING_BLOCK),
          sizeof (EFI_STRING_ID)
          );
        CurrentStringId = 1;
        BlockSize       = 0;
      } else {
        BlockSize       += sizeof (EFI_HII_SIBT_DUPLICATE_BLOCK);
        CurrentStringId++;
      }
      break;

    case EFI_HII_SIBT_SKIP1:
      SkipCount = (UINT16) (*(BlockHdr + sizeof (EFI_HII_STRING_BLOCK)));
      CurrentStringId = (UINT16) (CurrentStringId + SkipCount);
      BlockSize       +=  sizeof (EFI_HII_SIBT_SKIP1_BLOCK);
      break;

    case EFI_HII_SIBT_SKIP2:
      memcpy (&SkipCount, BlockHdr + sizeof (EFI_HII_STRING_BLOCK), sizeof (UINT16));
      CurrentStringId = (UINT16) (CurrentStringId + SkipCount);
      BlockSize       +=  sizeof (EFI_HII_SIBT_SKIP2_BLOCK);
      break;

    case EFI_HII_SIBT_EXT1:
      memcpy (
        &Length8,
        BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8),
        sizeof (UINT8)
        );
      BlockSize += Length8;
      break;

    case EFI_HII_SIBT_EXT2:
      memcpy (&Ext2, BlockHdr, sizeof (EFI_HII_SIBT_EXT2_BLOCK));
      BlockSize += Ext2.Length;
      break;

    case EFI_HII_SIBT_EXT4:
      memcpy (
        &Length32,
        BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8),
        sizeof (UINT32)
        );

      BlockSize += Length32;
      break;

    default:
      break;
    }

    if (StringId > 0 && StringId != (EFI_STRING_ID)(-1)) {
      *StringTextOffset = BlockHdr - StringData + Offset;
      *BlockType        = *BlockHdr;

      if (StringId == CurrentStringId - 1) {
        //
        // if only one skip item, return EFI_NOT_FOUND.
        //
        if(*BlockType == EFI_HII_SIBT_SKIP2 || *BlockType == EFI_HII_SIBT_SKIP1) {
          return EFI_NOT_FOUND;
        } else {
          return EFI_SUCCESS;
        }
      }

      if (StringId < CurrentStringId - 1) {
        return EFI_NOT_FOUND;
      }
    }
    BlockHdr  = StringData + BlockSize;
  }

  return EFI_NOT_FOUND;
}

UINT32
CVfrStringDB::GetUnicodeStringTextSize (
  IN  UINT8            *StringSrc
  )
{
  UINT32 StringSize;
  CHAR16 *StringPtr;

  StringSize = sizeof (CHAR16);
  StringPtr  = (UINT16*)StringSrc;
  while (*StringPtr++ != L'\0') {
    StringSize += sizeof (CHAR16);
  }

  return StringSize;
}

BOOLEAN  VfrCompatibleMode = FALSE;

CVfrVarDataTypeDB gCVfrVarDataTypeDB;


