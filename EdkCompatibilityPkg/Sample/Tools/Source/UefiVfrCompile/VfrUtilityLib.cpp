/*++
Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  VfrUtilityLib.cpp

Abstract:

--*/

#include "stdio.h"
#include "stdlib.h"
#include "VfrUtilityLib.h"
#include "VfrFormPkg.h"

VOID
CVfrBinaryOutput::WriteLine (
  IN FILE   *pFile,
  IN UINT32 LineBytes,
  IN INT8   *LineHeader,
  IN INT8   *BlkBuf,
  IN UINT32 BlkSize
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
  IN FILE   *pFile,
  IN UINT32 LineBytes,
  IN INT8   *LineHeader,
  IN INT8   *BlkBuf,
  IN UINT32 BlkSize
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
  IN INT8                *Name,
  IN INT8                *Id
  )
{
  mName          = NULL;
  mId            = NULL;
  mInfoStrList = NULL;
  mNext        = NULL;

  if (Name != NULL) {
    if ((mName = new INT8[strlen (Name) + 1]) != NULL) {
      strcpy (mName, Name);
    }
  }

  if (Id != NULL) {
    if ((mId = new INT8[strlen (Id) + 1]) != NULL) {
      strcpy (mId, Id);
    }
  }
}

SConfigItem::SConfigItem (
  IN INT8                *Name,
  IN INT8                *Id,
  IN UINT8               Type,
  IN UINT16              Offset,
  IN UINT16              Width,
  IN EFI_IFR_TYPE_VALUE  Value
  )
{
  mName        = NULL;
  mId          = NULL;
  mInfoStrList = NULL;
  mNext        = NULL;

  if (Name != NULL) {
    if ((mName = new INT8[strlen (Name) + 1]) != NULL) {
      strcpy (mName, Name);
    }
  }

  if (Id != NULL) {
    if ((mId = new INT8[strlen (Id) + 1]) != NULL) {
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
  BUFFER_SAFE_FREE (mId);
  while (mInfoStrList != NULL) {
    Info = mInfoStrList;
    mInfoStrList = mInfoStrList->mNext;

    BUFFER_SAFE_FREE (Info);
  }
}

UINT8
CVfrBufferConfig::Register (
  IN INT8                *Name,
  IN INT8                *Id
  )
{
  SConfigItem *pNew;

  if (Select (Name) == 0) {
    return 1;
  }

  if ((pNew = new SConfigItem (Name, Id)) == NULL) {
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
  IN INT8  *Name,
  IN INT8  *Id
  )
{
  SConfigItem *p;

  if (Name == NULL) {
    mItemListPos = mItemListHead;
    return 0;
  } else {
    for (p = mItemListHead; p != NULL; p = p->mNext) {
      if (strcmp (p->mName, Name) != 0) {
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
  IN INT8                *Name,
  IN INT8                *Id,
  IN UINT8               Type,
  IN UINT16              Offset,
  IN UINT32              Width,
  IN EFI_IFR_TYPE_VALUE  Value
  )
{
  UINT8         Ret;
  SConfigItem   *pItem;
  SConfigInfo   *pInfo;

  if ((Ret = Select (Name)) != 0) {
    return Ret;
  }

  switch (Mode) {
  case 'a' : // add
    if (Select (Name, Id) != 0) {
      if ((pItem = new SConfigItem (Name, Id, Type, Offset, Width, Value)) == NULL) {
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
          // check if the value and width are the same; return error if not
          if ((Id != NULL) && (pInfo->mWidth != Width || memcmp(pInfo->mValue, &Value, Width) != 0)) {
            return VFR_RETURN_DEFAULT_VALUE_REDEFINED;
          }
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
      if ((mItemListPos->mId = new INT8[strlen (Id) + 1]) == NULL) {
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

#if 0
UINT8
CVfrBufferConfig::ReadId (
  OUT INT8   **Name,
  OUT INT8   **Id
  )
{
  if (mInfoStrItemListPos == NULL) {
    return 1; // end read or some error occur
  }

  if (Name != NULL) {
    *Name = new INT8 (strlen (mInfoStrItemListPos->mName + 1));
    strcpy (*Name, mInfoStrItemListPos->mName);
  }
  if (Id != NULL) {
    *Id = new INT8 (strlen (mInfoStrItemListPos->mId + 1));
    strcpy (*Id, mInfoStrItemListPos->mId);
  }

  return 0;
}

UINT8
CVfrBufferConfig::ReadInfo (
  IN  INT8      *Name,
  IN  UINT32    Index,
  IN OUT UINT32 &Number,
  OUT INT8      *Offset,
  OUT INT8      *Width,
  OUT INT8      *Value
  )
{
  UINT8         ret;
  SConfigInfo   *p;
  UINT32        idx;
  UINT32        num;

  if (Name != NULL) {
    if ((ret = Select (Name)) != 0) {
      return ret;
    }
  }

  if (mInfoStrItemListPos == NULL) {
    return 1; // end read or some error occur
  }

  p = mInfoStrItemListPos->mInfoStrList;
  for (idx = 0; (idx < Index) && (p != NULL); idx++) {
    p = p->mNext;
  }
  if (p == NULL) {
    return 1;
  }

  if (Offset != NULL) {
    Offset[0] = '\0';
  }
  if (Width != NULL) {
    Width[0] = '\0';
  }
  if (Value != NULL) {
    Value[0] = '\0';
  }

  while (num < Number) {
    if (Offset != NULL) {
      strcat (Offset, p->mOffset);
    }
    if (Width != NULL) {
      strcat (Width, p->mWidth);
    }
    if (Value != NULL) {
      strcat (Value, p->mValue);
    }

    num++;
    if ((p = p->mNext) == NULL) {
      break;
    }
  }
  Number = num;

  return 0;
}

VOID
CVfrBufferConfig::ReadNext (
  VOID
  )
{
  if (mItemListPos != NULL) {
    mItemListPos = mItemListPos->mNext;
  }
}
#endif

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
  IN INT8  *BaseName
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
    Output.WriteLine (pFile, BYTES_PRE_LINE, "  ", (INT8 *)&TotalLen, sizeof (UINT32));

    for (Info = Item->mInfoStrList; Info != NULL; Info = Info->mNext) {
      fprintf (pFile, "\n");
      Output.WriteLine (pFile, BYTES_PRE_LINE, "  ", (INT8 *)&Info->mOffset, sizeof (UINT16));
      Output.WriteLine (pFile, BYTES_PRE_LINE, "  ", (INT8 *)&Info->mWidth, sizeof (UINT16));
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
      Output.WriteLine (pFile, BYTES_PRE_LINE, "  ", (INT8 *)&TotalLen, sizeof (UINT32));

      for (Info = Item->mInfoStrList; Info != NULL; Info = Info->mNext) {
        fprintf (pFile, "\n");
        Output.WriteLine (pFile, BYTES_PRE_LINE, "  ", (INT8 *)&Info->mOffset, sizeof (UINT16));
        Output.WriteLine (pFile, BYTES_PRE_LINE, "  ", (INT8 *)&Info->mWidth, sizeof (UINT16));
        if (Info->mNext == NULL) {
          Output.WriteEnd (pFile, BYTES_PRE_LINE, "  ", (INT8 *)Info->mValue, Info->mWidth);
        } else {
          Output.WriteLine (pFile, BYTES_PRE_LINE, "  ", (INT8 *)Info->mValue, Info->mWidth);
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
  INT8   *mTypeName;
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
  {NULL,            EFI_IFR_TYPE_OTHER,       0,                     0}
};

STATIC
BOOLEAN
_IS_INTERNAL_TYPE (
  IN INT8 *TypeName
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
INT8 *
TrimHex (
  IN  INT8    *Str,
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
  IN INT8 *Str
  )
{
  bool    IsHex;
  UINT32  Value;
  INT8    c;

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
  IN  INT8 *&VarStr,
  OUT INT8 *TName
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
  IN  INT8   *&VarStr,
  IN  INT8   *FName,
  OUT UINT32 &ArrayIdx
  )
{
  UINT32 Idx;
  INT8   ArrayStr[MAX_NAME_LEN + 1];

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
    return VFR_RETURN_SUCCESS;
  case ']':
    return VFR_RETURN_DATA_STRING_ERROR;
  }

  return VFR_RETURN_SUCCESS;
}

EFI_VFR_RETURN_CODE
CVfrVarDataTypeDB::GetTypeField (
  IN  INT8          *FName,
  IN  SVfrDataType  *Type,
  OUT SVfrDataField *&Field
  )
{
  SVfrDataField  *pField = NULL;

  if ((FName == NULL) && (Type == NULL)) {
    return VFR_RETURN_FATAL_ERROR;
  }

  for (pField = Type->mMembers; pField != NULL; pField = pField->mNext) {
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

  if ((ArrayIdx != INVALID_ARRAY_INDEX) && ((Field->mArrayNum == 0) || (Field->mArrayNum <= ArrayIdx))) {
    return VFR_RETURN_ERROR_ARRARY_NUM;
  }

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
        GetDataType ("UINT8", &pYearField->mFieldType);
        pYearField->mOffset      = 0;
        pYearField->mNext        = pMonthField;
        pYearField->mArrayNum    = 0;

        strcpy (pMonthField->mFieldName, "Month");
        GetDataType ("UINT8", &pMonthField->mFieldType);
        pMonthField->mOffset     = 1;
        pMonthField->mNext       = pDayField;
        pMonthField->mArrayNum   = 0;

        strcpy (pDayField->mFieldName, "Day");
        GetDataType ("UINT8", &pDayField->mFieldType);
        pDayField->mOffset       = 2;
        pDayField->mNext         = NULL;
        pDayField->mArrayNum     = 0;

        New->mMembers            = pYearField;
      } else if (strcmp (gInternalTypesTable[Index].mTypeName, "EFI_HII_TIME") == 0) {
        SVfrDataField *pHoursField   = new SVfrDataField;
        SVfrDataField *pMinutesField = new SVfrDataField;
        SVfrDataField *pSecondsField = new SVfrDataField;

        strcpy (pHoursField->mFieldName, "Hours");
        GetDataType ("UINT8", &pHoursField->mFieldType);
        pHoursField->mOffset     = 0;
        pHoursField->mNext       = pMinutesField;
        pHoursField->mArrayNum   = 0;

        strcpy (pMinutesField->mFieldName, "Minutes");
        GetDataType ("UINT8", &pMinutesField->mFieldType);
        pMinutesField->mOffset   = 1;
        pMinutesField->mNext     = pSecondsField;
        pMinutesField->mArrayNum = 0;

        strcpy (pSecondsField->mFieldName, "Seconds");
        GetDataType ("UINT8", &pSecondsField->mFieldType);
        pSecondsField->mOffset   = 2;
        pSecondsField->mNext     = NULL;
        pSecondsField->mArrayNum = 0;

        New->mMembers            = pHoursField;
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
  IN INT8           *Identifier,
  IN UINT32         Number
  )
{
  UINT32            PackAlign;
  INT8              Msg[64] = {0, };

  if (Action & VFR_PACK_SHOW) {
    sprintf (Msg, "value of pragma pack(show) == %d", mPackAlign);
    gCVfrErrorHandle.PrintMsg (LineNum, "", "Warning", Msg);
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
      gCVfrErrorHandle.PrintMsg (LineNum, "", "Warning", "#pragma pack(pop...) : more pops than pushes");
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
      gCVfrErrorHandle.PrintMsg (LineNum, "", "Warning", "expected pragma parameter to be '1', '2', '4', '8', or '16'");
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
  IN INT8   *TypeName
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
  IN INT8   *FieldName,
  IN INT8   *TypeName,
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
  mNewDataType             = NULL;
}

EFI_VFR_RETURN_CODE
CVfrVarDataTypeDB::GetDataType (
  IN  INT8         *TypeName,
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
  IN  INT8   *TypeName,
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
  IN  INT8     *VarStr,
  OUT UINT16   &Offset,
  OUT UINT8    &Type,
  OUT UINT32   &Size
  )
{
  INT8                TName[MAX_NAME_LEN], FName[MAX_NAME_LEN];
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
    Offset += Tmp;
    Type   = GetFieldWidth (pField);
    Size   = GetFieldSize (pField, ArrayIdx);
  }
  return VFR_RETURN_SUCCESS;
}

EFI_VFR_RETURN_CODE
CVfrVarDataTypeDB::GetUserDefinedTypeNameList  (
  OUT INT8      ***NameList,
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

  if ((*NameList = new INT8*[*ListSize]) == NULL) {
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
  IN INT8 *TypeName
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
  IN INT8                  *StoreName,
  IN EFI_VARSTORE_ID       VarStoreId,
  IN EFI_STRING_ID         VarName,
  IN UINT32                VarSize
  )
{
  if (Guid != NULL) {
    mGuid = *Guid;
  } else {
    memset (&Guid, 0, sizeof (EFI_GUID));
  }
  if (StoreName != NULL) {
    mVarStoreName = new INT8[strlen(StoreName) + 1];
    strcpy (mVarStoreName, StoreName);
  } else {
    mVarStoreName = NULL;
  }
  mNext                            = NULL;
  mVarStoreId                      = VarStoreId;
  mVarStoreType                    = EFI_VFR_VARSTORE_EFI;
  mStorageInfo.mEfiVar.mEfiVarName = VarName;
  mStorageInfo.mEfiVar.mEfiVarSize = VarSize;
}

SVfrVarStorageNode::SVfrVarStorageNode (
  IN EFI_GUID              *Guid,
  IN INT8                  *StoreName,
  IN EFI_VARSTORE_ID       VarStoreId,
  IN SVfrDataType          *DataType
  )
{
  if (Guid != NULL) {
    mGuid = *Guid;
  } else {
    memset (&Guid, 0, sizeof (EFI_GUID));
  }
  if (StoreName != NULL) {
    mVarStoreName = new INT8[strlen(StoreName) + 1];
    strcpy (mVarStoreName, StoreName);
  } else {
    mVarStoreName = NULL;
  }
  mNext                    = NULL;
  mVarStoreId              = VarStoreId;
  mVarStoreType            = EFI_VFR_VARSTORE_BUFFER;
  mStorageInfo.mDataType   = DataType;
}

SVfrVarStorageNode::SVfrVarStorageNode (
  IN INT8                  *StoreName,
  IN EFI_VARSTORE_ID       VarStoreId
  )
{
  if (StoreName != NULL) {
    mVarStoreName = new INT8[strlen(StoreName) + 1];
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
  VOID
  )
{
  UINT32  Index, Mask, Offset;

  for (Index = 0; Index < EFI_FREE_VARSTORE_ID_BITMAP_SIZE; Index++) {
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
  IN INT8     *StoreName
  )
{
  SVfrVarStorageNode *pNode = NULL;
  EFI_VARSTORE_ID    VarStoreId;

  if (StoreName == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }

  for (pNode = mNameVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (strcmp (pNode->mVarStoreName, StoreName) == 0) {
      return VFR_RETURN_REDEFINED;
    }
  }

  VarStoreId = GetFreeVarStoreId ();
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
  IN INT8           *StoreName,
  IN EFI_GUID       *Guid,
  IN EFI_STRING_ID  NameStrId,
  IN UINT32         VarSize
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

  for (pNode = mEfiVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (strcmp (pNode->mVarStoreName, StoreName) == 0) {
      return VFR_RETURN_REDEFINED;
    }
  }

  VarStoreId = GetFreeVarStoreId ();
  if ((pNode = new SVfrVarStorageNode (Guid, StoreName, VarStoreId, NameStrId, VarSize)) == NULL) {
    return VFR_RETURN_OUT_FOR_RESOURCES;
  }

  pNode->mNext       = mNameVarStoreList;
  mNameVarStoreList  = pNode;

  return VFR_RETURN_SUCCESS;
}

EFI_VFR_RETURN_CODE
CVfrDataStorage::DeclareBufferVarStore (
  IN INT8              *StoreName,
  IN EFI_GUID          *Guid,
  IN CVfrVarDataTypeDB *DataTypeDB,
  IN INT8              *TypeName,
  IN EFI_VARSTORE_ID   VarStoreId
  )
{
  SVfrVarStorageNode   *pNew = NULL;
  SVfrDataType         *pDataType = NULL;

  if ((StoreName == NULL) || (Guid == NULL) || (DataTypeDB == NULL)) {
    return VFR_RETURN_FATAL_ERROR;
  }

  CHECK_ERROR_RETURN(DataTypeDB->GetDataType (TypeName, &pDataType), VFR_RETURN_SUCCESS);

  if (VarStoreId == EFI_VARSTORE_ID_INVALID) {
    VarStoreId = GetFreeVarStoreId ();
  } else {
    if (ChekVarStoreIdFree (VarStoreId) == FALSE) {
      return VFR_RETURN_VARSTOREID_REDEFINED;
    }
    MarkVarStoreIdUsed (VarStoreId);
  }

  if ((pNew = new SVfrVarStorageNode (Guid, StoreName, VarStoreId, pDataType)) == NULL) {
    return VFR_RETURN_OUT_FOR_RESOURCES;
  }

  pNew->mNext         = mBufferVarStoreList;
  mBufferVarStoreList = pNew;

  if (gCVfrBufferConfig.Register(StoreName) != 0) {
    return VFR_RETURN_FATAL_ERROR;
  }

  return VFR_RETURN_SUCCESS;
}

EFI_VFR_RETURN_CODE
CVfrDataStorage::GetVarStoreId (
  IN  INT8            *StoreName,
  OUT EFI_VARSTORE_ID *VarStoreId
  )
{
  SVfrVarStorageNode    *pNode;

  for (pNode = mBufferVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (strcmp (pNode->mVarStoreName, StoreName) == 0) {
      mCurrVarStorageNode = pNode;
      *VarStoreId = pNode->mVarStoreId;
      return VFR_RETURN_SUCCESS;
    }
  }

  for (pNode = mEfiVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (strcmp (pNode->mVarStoreName, StoreName) == 0) {
      mCurrVarStorageNode = pNode;
      *VarStoreId = pNode->mVarStoreId;
      return VFR_RETURN_SUCCESS;
    }
  }

  for (pNode = mNameVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (strcmp (pNode->mVarStoreName, StoreName) == 0) {
      mCurrVarStorageNode = pNode;
      *VarStoreId = pNode->mVarStoreId;
      return VFR_RETURN_SUCCESS;
    }
  }

  mCurrVarStorageNode = NULL;
  *VarStoreId        = EFI_VARSTORE_ID_INVALID;
  return VFR_RETURN_UNDEFINED;
}

EFI_VFR_RETURN_CODE
CVfrDataStorage::GetBufferVarStoreDataTypeName (
  IN  INT8                   *StoreName,
  OUT INT8                   **DataTypeName
  )
{
  SVfrVarStorageNode    *pNode;

  if ((StoreName == NULL) || (DataTypeName == NULL)) {
    return VFR_RETURN_FATAL_ERROR;
  }

  for (pNode = mBufferVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (strcmp (pNode->mVarStoreName, StoreName) == 0) {
      break;
    }
  }

  if (pNode == NULL) {
    return VFR_RETURN_UNDEFINED;
  }

  if (pNode->mStorageInfo.mDataType == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }

  *DataTypeName = pNode->mStorageInfo.mDataType->mTypeName;
  return VFR_RETURN_SUCCESS;
}

EFI_VFR_RETURN_CODE
CVfrDataStorage::GetVarStoreType (
  IN  INT8                   *StoreName,
  OUT EFI_VFR_VARSTORE_TYPE  &VarStoreType
  )
{
  SVfrVarStorageNode    *pNode;

  if (StoreName == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }

  for (pNode = mBufferVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (strcmp (pNode->mVarStoreName, StoreName) == NULL) {
      VarStoreType = pNode->mVarStoreType;
      return VFR_RETURN_SUCCESS;
    }
  }

  for (pNode = mEfiVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (strcmp (pNode->mVarStoreName, StoreName) == NULL) {
      VarStoreType = pNode->mVarStoreType;
      return VFR_RETURN_SUCCESS;
    }
  }

  for (pNode = mNameVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (strcmp (pNode->mVarStoreName, StoreName) == NULL) {
      VarStoreType = pNode->mVarStoreType;
      return VFR_RETURN_SUCCESS;
    }
  }

  VarStoreType = EFI_VFR_VARSTORE_INVALID;
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

EFI_VFR_RETURN_CODE
CVfrDataStorage::GetVarStoreName (
  IN  EFI_VARSTORE_ID VarStoreId,
  OUT INT8            **VarStoreName
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

  Info->mInfo.mVarName = mCurrVarStorageNode->mStorageInfo.mNameSpace.mNameTable[Index];

  return VFR_RETURN_SUCCESS;
}

EFI_VFR_RETURN_CODE
CVfrDataStorage::BufferVarStoreRequestElementAdd (
  IN INT8              *StoreName,
  IN EFI_VARSTORE_INFO &Info
  )
{
  INT8                  NewReqElt[128] = {'\0',};
  INT8                  *OldReqElt = NULL;
  SVfrVarStorageNode    *pNode = NULL;
  EFI_IFR_TYPE_VALUE    Value = {0};

  for (pNode = mBufferVarStoreList; pNode != NULL; pNode = pNode->mNext) {
    if (strcmp (pNode->mVarStoreName, StoreName) == NULL) {
      break;
    }
  }

  if (pNode == NULL) {
    return VFR_RETURN_UNDEFINED;
  }

  gCVfrBufferConfig.Open ();
  Value.u8 = 0;
  if (gCVfrBufferConfig.Write ('a', StoreName, NULL, EFI_IFR_TYPE_NUM_SIZE_8, Info.mInfo.mVarOffset, Info.mVarTotalSize, Value) != 0) {
    return VFR_RETURN_FATAL_ERROR;
  }
  gCVfrBufferConfig.Close ();

  return VFR_RETURN_SUCCESS;
}

SVfrDefaultStoreNode::SVfrDefaultStoreNode (
  IN EFI_IFR_DEFAULTSTORE *ObjBinAddr,
  IN INT8                 *RefName,
  IN EFI_STRING_ID        DefaultStoreNameId,
  IN UINT16               DefaultId
  )
{
  mObjBinAddr = ObjBinAddr;

  if (RefName != NULL) {
    mRefName          = new INT8[strlen (RefName) + 1];
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
  IN INT8                 *RefName,
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
  IN INT8            *RefName,
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
      pNode->mRefName = new INT8[strlen (RefName) + 1];
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
  IN  INT8            *RefName,
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

STATIC
EFI_VFR_RETURN_CODE
AltCfgItemPrintToBuffer (
  IN INT8               *NewAltCfg,
  IN EFI_VARSTORE_INFO  Info,
  IN UINT8              Type,
  IN EFI_IFR_TYPE_VALUE Value
  )
{
  UINT32 Index;
  UINT8  *BufChar = NULL;
  UINT32 Count    = 0;

  if (NewAltCfg != NULL) {
    Count = sprintf (
              NewAltCfg,
              "&OFFSET=%x&WIDTH=%x&VALUE=",
              Info.mInfo.mVarOffset,
              Info.mVarTotalSize
              );
    NewAltCfg += Count;

    switch (Type) {
    case EFI_IFR_TYPE_NUM_SIZE_8 :
      Count = sprintf (NewAltCfg, "%x", Value.u8);
      NewAltCfg += Count;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_16 :
      Count = sprintf (NewAltCfg, "%x", Value.u16);
      NewAltCfg += Count;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_32 :
      Count = sprintf (NewAltCfg, "%x", Value.u32);
      NewAltCfg += Count;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_64 :
      Count = sprintf (NewAltCfg, "%x", Value.u64);
      NewAltCfg += Count;
      break;
    case EFI_IFR_TYPE_BOOLEAN :
      Count = sprintf (NewAltCfg, "%x", Value.b);
      NewAltCfg += Count;
      break;
    case EFI_IFR_TYPE_TIME :
#if 1
      Count = sprintf (NewAltCfg, "%x", *((UINT32 *)(&Value.time)));
      NewAltCfg += Count;
#else
      BufChar = (UINT8 *)&Value.time;
      for (Index = 0; Index < sizeof(EFI_HII_TIME); Index++) {
        Count = sprintf (NewAltCfg, "%02x", (UINT8)BufChar[Index]);
        NewAltCfg += Count;
      }
#endif
      break;
    case EFI_IFR_TYPE_DATE :
#if 1
      Count = sprintf (NewAltCfg, "%x", *((UINT32 *)(&Value.date)));
      NewAltCfg += Count;
#else
      BufChar = (UINT8 *)&Value.date;
      for (Index = 0; Index < sizeof(EFI_HII_DATE); Index++) {
        Count = sprintf (NewAltCfg, "%02x", (UINT8)BufChar[Index]);
        NewAltCfg += Count;
      }
#endif
      break;
    case EFI_IFR_TYPE_STRING :
      Count = sprintf (NewAltCfg, "%x", Value.string);
      NewAltCfg += Count;
      break;
    case EFI_IFR_TYPE_OTHER :
      return VFR_RETURN_UNSUPPORTED;
	}
  }

  return VFR_RETURN_FATAL_ERROR;
}

EFI_VFR_RETURN_CODE
CVfrDefaultStore::BufferVarStoreAltConfigAdd (
  IN EFI_VARSTORE_ID    DefaultId,
  IN EFI_VARSTORE_INFO  &Info,
  IN INT8               *VarStoreName,
  IN UINT8              Type,
  IN EFI_IFR_TYPE_VALUE Value
  )
{
  SVfrDefaultStoreNode  *pNode = NULL;
  INT8                  NewAltCfg[2 * 2 * sizeof (UINT16) + 1] = {0,};
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
  if ((Returnvalue = gCVfrBufferConfig.Select(VarStoreName)) == 0) {
    if ((Returnvalue = gCVfrBufferConfig.Write ('a', VarStoreName, NewAltCfg, Type, Info.mInfo.mVarOffset, Info.mVarTotalSize, Value)) != 0) {
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
  IN INT8        *RuleName,
  IN UINT8       RuleId
  )
{
  if (RuleName != NULL) {
    mRuleName = new INT8[strlen (RuleName) + 1];
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
  IN INT8   *RuleName
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
  IN INT8   *RuleName
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
  IN INT8   *Name,
  IN INT8   *VarIdStr,
  IN UINT32 BitMask
  )
{
  mName       = NULL;
  mVarIdStr   = NULL;
  mQuestionId = EFI_QUESTION_ID_INVALID;
  mBitMask    = BitMask;
  mNext       = NULL;

  if (Name == NULL) {
    mName = new INT8[strlen ("$DEFAULT") + 1];
    strcpy (mName, "$DEFAULT");
  } else {
    mName = new INT8[strlen (Name) + 1];
    strcpy (mName, Name);
  }

  if (VarIdStr != NULL) {
    mVarIdStr = new INT8[strlen (VarIdStr) + 1];
    strcpy (mVarIdStr, VarIdStr);
  } else {
    mVarIdStr = new INT8[strlen ("$") + 1];
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

EFI_VFR_RETURN_CODE
CVfrQuestionDB::RegisterQuestion (
  IN     INT8              *Name,
  IN     INT8              *VarIdStr,
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
    if (ChekQuestionIdFree (QuestionId) == FALSE) {
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
  IN     INT8            *YearVarId,
  IN     INT8            *MonthVarId,
  IN     INT8            *DayVarId,
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
  IN     INT8            *Name,
  IN     INT8            *BaseVarId,
  IN OUT EFI_QUESTION_ID &QuestionId
  )
{
  SVfrQuestionNode     *pNode[3] = {NULL, };
  UINT32               Len;
  INT8                 *VarIdStr[3] = {NULL, };
  INT8                 Index;

  if (BaseVarId == NULL) {
    return;
  }

  Len = strlen (BaseVarId);

  VarIdStr[0] = new INT8[Len + strlen (".Year") + 1];
  if (VarIdStr[0] != NULL) {
    strcpy (VarIdStr[0], BaseVarId);
    strcat (VarIdStr[0], ".Year");
  }
  VarIdStr[1] = new INT8[Len + strlen (".Month") + 1];
  if (VarIdStr[1] != NULL) {
    strcpy (VarIdStr[1], BaseVarId);
    strcat (VarIdStr[1], ".Month");
  }
  VarIdStr[2] = new INT8[Len + strlen (".Day") + 1];
  if (VarIdStr[2] != NULL) {
    strcpy (VarIdStr[2], BaseVarId);
    strcat (VarIdStr[2], ".Day");
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
  IN     INT8            *HourVarId,
  IN     INT8            *MinuteVarId,
  IN     INT8            *SecondVarId,
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
  IN     INT8            *Name,
  IN     INT8            *BaseVarId,
  IN OUT EFI_QUESTION_ID &QuestionId
  )
{
  SVfrQuestionNode     *pNode[3] = {NULL, };
  UINT32               Len;
  INT8                 *VarIdStr[3] = {NULL, };
  INT8                 Index;

  if (BaseVarId == NULL) {
    return;
  }

  Len = strlen (BaseVarId);

  VarIdStr[0] = new INT8[Len + strlen (".Hour") + 1];
  if (VarIdStr[0] != NULL) {
    strcpy (VarIdStr[0], BaseVarId);
    strcat (VarIdStr[0], ".Hour");
  }
  VarIdStr[1] = new INT8[Len + strlen (".Minute") + 1];
  if (VarIdStr[1] != NULL) {
    strcpy (VarIdStr[1], BaseVarId);
    strcat (VarIdStr[1], ".Minute");
  }
  VarIdStr[2] = new INT8[Len + strlen (".Second") + 1];
  if (VarIdStr[2] != NULL) {
    strcpy (VarIdStr[2], BaseVarId);
    strcat (VarIdStr[2], ".Second");
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

EFI_VFR_RETURN_CODE
CVfrQuestionDB::UpdateQuestionId (
  IN EFI_QUESTION_ID   QId,
  IN EFI_QUESTION_ID   NewQId
  )
{
  SVfrQuestionNode *pNode = NULL;

  if (ChekQuestionIdFree (NewQId) == FALSE) {
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
  IN  INT8              *Name,
  IN  INT8              *VarIdStr,
  OUT EFI_QUESTION_ID   &QuestionId,
  OUT UINT32            &BitMask
  )
{
  SVfrQuestionNode *pNode;

  QuestionId = EFI_QUESTION_ID_INVALID;
  BitMask    = 0x00000000;

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
  IN INT8 *Name
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

