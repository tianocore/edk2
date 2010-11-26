/*++
Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  VfrUtilityLib.h

Abstract:

--*/

#ifndef _VFRUTILITYLIB_H_
#define _VFRUTILITYLIB_H_

#include "Tiano.h"
#include "string.h"
#include "EfiTypes.h"
#include "EfiVfr.h"
#include "VfrError.h"

#define MAX_NAME_LEN                       64
#define DEFAULT_ALIGN                      1
#define DEFAULT_PACK_ALIGN                 0x8
#define DEFAULT_NAME_TABLE_ITEMS           1024

#define EFI_BITS_SHIFT_PER_UINT32          0x5
#define EFI_BITS_PER_UINT32                (1 << EFI_BITS_SHIFT_PER_UINT32)

#define BUFFER_SAFE_FREE(Buf)              do { if ((Buf) != NULL) { delete (Buf); } } while (0);

class CVfrBinaryOutput {
public:
  virtual VOID WriteLine (IN FILE *, IN UINT32, IN INT8 *, IN INT8 *, IN UINT32);
  virtual VOID WriteEnd (IN FILE *, IN UINT32, IN INT8 *, IN INT8 *, IN UINT32);
};

UINT32
_STR2U32 (
  IN INT8 *Str
  );

struct SConfigInfo {
  UINT16             mOffset;
  UINT16             mWidth;
  UINT8              *mValue;
  SConfigInfo        *mNext;

  SConfigInfo (IN UINT8, IN UINT16, IN UINT32, IN EFI_IFR_TYPE_VALUE);
  ~SConfigInfo (VOID);
};

struct SConfigItem {
  INT8          *mName;         // varstore name
  INT8          *mId;           // varstore ID
  SConfigInfo   *mInfoStrList;  // list of Offset/Value in the varstore
  SConfigItem   *mNext;

public:
  SConfigItem (IN INT8 *, IN INT8 *);
  SConfigItem (IN INT8 *, IN INT8 *, IN UINT8, IN UINT16, IN UINT16, IN EFI_IFR_TYPE_VALUE);
  virtual ~SConfigItem ();
};

class CVfrBufferConfig {
private:
  SConfigItem *mItemListHead;
  SConfigItem *mItemListTail;
  SConfigItem *mItemListPos;

public:
  CVfrBufferConfig (VOID);
  virtual ~CVfrBufferConfig (VOID);

  virtual UINT8   Register (IN INT8 *, IN INT8 *Info = NULL);
  virtual VOID    Open (VOID);
  virtual BOOLEAN Eof(VOID);
  virtual UINT8   Select (IN INT8 *, IN INT8 *Info = NULL);
  virtual UINT8   Write (IN CONST CHAR8, IN INT8 *, IN INT8 *, IN UINT8, IN UINT16, IN UINT32, IN EFI_IFR_TYPE_VALUE);
#if 0
  virtual UINT8   Read (OUT INT8 **, OUT INT8 **, OUT INT8 **, OUT INT8 **, OUT INT8 **);
#endif
  virtual VOID    Close (VOID);
  virtual VOID    OutputCFile (IN FILE *, IN INT8 *);
};

extern CVfrBufferConfig gCVfrBufferConfig;

#define ALIGN_STUFF(Size, Align) ((Align) - (Size) % (Align))
#define INVALID_ARRAY_INDEX      0xFFFFFFFF

struct SVfrDataType;

struct SVfrDataField {
  INT8                      mFieldName[MAX_NAME_LEN];
  SVfrDataType              *mFieldType;
  UINT32                    mOffset;
  UINT32                    mArrayNum;
  SVfrDataField             *mNext;
};

struct SVfrDataType {
  INT8                      mTypeName[MAX_NAME_LEN];
  UINT8                     mType;
  UINT32                    mAlign;
  UINT32                    mTotalSize;
  SVfrDataField             *mMembers;
  SVfrDataType              *mNext;
};

#define VFR_PACK_ASSIGN     0x01
#define VFR_PACK_SHOW       0x02
#define VFR_PACK_PUSH       0x04
#define VFR_PACK_POP        0x08

#define PACKSTACK_MAX_SIZE  0x400

struct SVfrPackStackNode {
  INT8                      *mIdentifier;
  UINT32                    mNumber;
  SVfrPackStackNode         *mNext;

  SVfrPackStackNode (IN INT8 *Identifier, IN UINT32 Number) {
    mIdentifier = NULL;
    mNumber     = Number;
    mNext       = NULL;

    if (Identifier != NULL) {
      mIdentifier = new INT8[strlen (Identifier) + 1];
      strcpy (mIdentifier, Identifier);
    }
  }

  ~SVfrPackStackNode (VOID) {
    if (mIdentifier != NULL) {
      delete mIdentifier;
    }
    mNext = NULL;
  }

  bool Match (IN INT8 *Identifier) {
    if (Identifier == NULL) {
      return TRUE;
    } else if (mIdentifier == NULL) {
      return FALSE;
    } else if (strcmp (Identifier, mIdentifier) == 0) {
      return TRUE;
    } else {
      return FALSE;
    }
  }
};

class CVfrVarDataTypeDB {
private:
  UINT32                    mPackAlign;
  SVfrPackStackNode         *mPackStack;

public:
  EFI_VFR_RETURN_CODE       Pack (IN UINT32, IN UINT8, IN INT8 *Identifier = NULL, IN UINT32 Number = DEFAULT_PACK_ALIGN);

private:
  SVfrDataType              *mDataTypeList;

  SVfrDataType              *mNewDataType;
  SVfrDataType              *mCurrDataType;
  SVfrDataField             *mCurrDataField;

  VOID InternalTypesListInit (VOID);
  VOID RegisterNewType (IN SVfrDataType *);

  EFI_VFR_RETURN_CODE ExtractStructTypeName (IN INT8 *&, OUT INT8 *);
  EFI_VFR_RETURN_CODE ExtractFieldNameAndArrary (IN INT8 *&, OUT INT8 *, OUT UINT32 &);
  EFI_VFR_RETURN_CODE GetTypeField (IN INT8 *, IN SVfrDataType *, IN SVfrDataField *&);
  EFI_VFR_RETURN_CODE GetFieldOffset (IN SVfrDataField *, IN UINT32, OUT UINT32 &);
  UINT8               GetFieldWidth (IN SVfrDataField *);
  UINT32              GetFieldSize (IN SVfrDataField *, IN UINT32);

public:
  CVfrVarDataTypeDB (VOID);
  ~CVfrVarDataTypeDB (VOID);

  VOID                DeclareDataTypeBegin (VOID);
  EFI_VFR_RETURN_CODE SetNewTypeName (IN INT8 *);
  EFI_VFR_RETURN_CODE DataTypeAddField (IN INT8 *, IN INT8 *, IN UINT32);
  VOID                DeclareDataTypeEnd (VOID);

  EFI_VFR_RETURN_CODE GetDataType (IN INT8 *, OUT SVfrDataType **);
  EFI_VFR_RETURN_CODE GetDataTypeSize (IN INT8 *, OUT UINT32 *);
  EFI_VFR_RETURN_CODE GetDataTypeSize (IN UINT8, OUT UINT32 *);
  EFI_VFR_RETURN_CODE GetDataFieldInfo (IN INT8 *, OUT UINT16 &, OUT UINT8 &, OUT UINT32 &);

  EFI_VFR_RETURN_CODE GetUserDefinedTypeNameList (OUT INT8 ***, OUT UINT32 *);
  BOOLEAN             IsTypeNameDefined (IN INT8 *);

#ifdef CVFR_VARDATATYPEDB_DEBUG
  VOID ParserDB ();
#endif
};

typedef enum {
  EFI_VFR_VARSTORE_INVALID,
  EFI_VFR_VARSTORE_BUFFER,
  EFI_VFR_VARSTORE_EFI,
  EFI_VFR_VARSTORE_NAME
} EFI_VFR_VARSTORE_TYPE;

struct SVfrVarStorageNode {
  EFI_GUID                  mGuid;
  INT8                      *mVarStoreName;
  EFI_VARSTORE_ID           mVarStoreId;
  struct SVfrVarStorageNode *mNext;

  EFI_VFR_VARSTORE_TYPE     mVarStoreType;
  union {
    // EFI Variable
    struct {
      EFI_STRING_ID           mEfiVarName;
      UINT32                  mEfiVarSize;
    } mEfiVar;

    // Buffer Storage
    SVfrDataType            *mDataType;

    // NameValue Storage
	struct {
      EFI_STRING_ID         *mNameTable;
      UINT32                mTableSize;
    } mNameSpace;
  } mStorageInfo;

public:
  SVfrVarStorageNode (IN EFI_GUID *, IN INT8 *, IN EFI_VARSTORE_ID, IN EFI_STRING_ID, IN UINT32);
  SVfrVarStorageNode (IN EFI_GUID *, IN INT8 *, IN EFI_VARSTORE_ID, IN SVfrDataType *);
  SVfrVarStorageNode (IN INT8 *, IN EFI_VARSTORE_ID);
  ~SVfrVarStorageNode (VOID);
};

struct EFI_VARSTORE_INFO {
  EFI_VARSTORE_ID           mVarStoreId;
  union {
    EFI_STRING_ID           mVarName;
    UINT16                  mVarOffset;
  } mInfo;
  UINT8                     mVarType;
  UINT32                    mVarTotalSize;

  EFI_VARSTORE_INFO (VOID);
  EFI_VARSTORE_INFO (IN EFI_VARSTORE_INFO &);
  BOOLEAN operator == (IN EFI_VARSTORE_INFO *);
};

#define EFI_VARSTORE_ID_MAX              0xFFFF
#define EFI_FREE_VARSTORE_ID_BITMAP_SIZE ((EFI_VARSTORE_ID_MAX + 1) / EFI_BITS_PER_UINT32)

class CVfrDataStorage {
private:
  UINT32                    mFreeVarStoreIdBitMap[EFI_FREE_VARSTORE_ID_BITMAP_SIZE];

  struct SVfrVarStorageNode *mBufferVarStoreList;
  struct SVfrVarStorageNode *mEfiVarStoreList;
  struct SVfrVarStorageNode *mNameVarStoreList;

  struct SVfrVarStorageNode *mCurrVarStorageNode;
  struct SVfrVarStorageNode *mNewVarStorageNode;

private:

  EFI_VARSTORE_ID GetFreeVarStoreId (VOID);
  BOOLEAN         ChekVarStoreIdFree (IN EFI_VARSTORE_ID);
  VOID            MarkVarStoreIdUsed (IN EFI_VARSTORE_ID);
  VOID            MarkVarStoreIdUnused (IN EFI_VARSTORE_ID);

public:
  CVfrDataStorage ();
  ~CVfrDataStorage ();

  EFI_VFR_RETURN_CODE DeclareNameVarStoreBegin (INT8 *);
  EFI_VFR_RETURN_CODE NameTableAddItem (EFI_STRING_ID);
  EFI_VFR_RETURN_CODE DeclareNameVarStoreEnd (EFI_GUID *);

  EFI_VFR_RETURN_CODE DeclareEfiVarStore (IN INT8 *, IN EFI_GUID *, IN EFI_STRING_ID, IN UINT32);

  EFI_VFR_RETURN_CODE DeclareBufferVarStore (IN INT8 *, IN EFI_GUID *, IN CVfrVarDataTypeDB *, IN INT8 *, IN EFI_VARSTORE_ID);

  EFI_VFR_RETURN_CODE GetVarStoreId (IN INT8 *, OUT EFI_VARSTORE_ID *);
  EFI_VFR_RETURN_CODE GetVarStoreType (IN INT8 *, OUT EFI_VFR_VARSTORE_TYPE &);
  EFI_VFR_VARSTORE_TYPE GetVarStoreType (IN EFI_VARSTORE_ID);
  EFI_VFR_RETURN_CODE GetVarStoreName (IN EFI_VARSTORE_ID, OUT INT8 **);

  EFI_VFR_RETURN_CODE GetBufferVarStoreDataTypeName (IN INT8 *, OUT INT8 **);
  EFI_VFR_RETURN_CODE GetEfiVarStoreInfo (IN EFI_VARSTORE_INFO *);
  EFI_VFR_RETURN_CODE GetNameVarStoreInfo (IN EFI_VARSTORE_INFO *, IN UINT32);

  EFI_VFR_RETURN_CODE BufferVarStoreRequestElementAdd (IN INT8 *, IN EFI_VARSTORE_INFO &);
};

#define EFI_QUESTION_ID_MAX              0xFFFF
#define EFI_FREE_QUESTION_ID_BITMAP_SIZE ((EFI_QUESTION_ID_MAX + 1) / EFI_BITS_PER_UINT32)
#define EFI_QUESTION_ID_INVALID          0x0

#define DATE_YEAR_BITMASK                0x0000FFFF
#define DATE_MONTH_BITMASK               0x00FF0000
#define DATE_DAY_BITMASK                 0xFF000000
#define TIME_HOUR_BITMASK                0x000000FF
#define TIME_MINUTE_BITMASK              0x0000FF00
#define TIME_SECOND_BITMASK              0x00FF0000

struct SVfrQuestionNode {
  INT8                      *mName;
  INT8                      *mVarIdStr;
  EFI_QUESTION_ID           mQuestionId;
  UINT32                    mBitMask;
  SVfrQuestionNode          *mNext;

  SVfrQuestionNode (IN INT8 *, IN INT8 *, IN UINT32 BitMask = 0);
  ~SVfrQuestionNode ();
};

class CVfrQuestionDB {
private:
  SVfrQuestionNode          *mQuestionList;
  UINT32                    mFreeQIdBitMap[EFI_FREE_QUESTION_ID_BITMAP_SIZE];

private:
  EFI_QUESTION_ID GetFreeQuestionId (VOID);
  BOOLEAN         ChekQuestionIdFree (IN EFI_QUESTION_ID);
  VOID            MarkQuestionIdUsed (IN EFI_QUESTION_ID);
  VOID            MarkQuestionIdUnused (IN EFI_QUESTION_ID);

public:
  CVfrQuestionDB ();
  ~CVfrQuestionDB();

  EFI_VFR_RETURN_CODE RegisterQuestion (IN INT8 *, IN INT8 *, IN OUT EFI_QUESTION_ID &);
  VOID                RegisterOldDateQuestion (IN INT8 *, IN INT8 *, IN INT8 *, IN OUT EFI_QUESTION_ID &);
  VOID                RegisterNewDateQuestion (IN INT8 *, IN INT8 *, IN OUT EFI_QUESTION_ID &);
  VOID                RegisterOldTimeQuestion (IN INT8 *, IN INT8 *, IN INT8 *, IN OUT EFI_QUESTION_ID &);
  VOID                RegisterNewTimeQuestion (IN INT8 *, IN INT8 *, IN OUT EFI_QUESTION_ID &);
  EFI_VFR_RETURN_CODE UpdateQuestionId (IN EFI_QUESTION_ID, IN EFI_QUESTION_ID);
  VOID                GetQuestionId (IN INT8 *, IN INT8 *, OUT EFI_QUESTION_ID &, OUT UINT32 &);
  EFI_VFR_RETURN_CODE FindQuestion (IN EFI_QUESTION_ID);
  EFI_VFR_RETURN_CODE FindQuestion (IN INT8 *);
 };

struct SVfrDefaultStoreNode {
  EFI_IFR_DEFAULTSTORE      *mObjBinAddr;
  INT8                      *mRefName;
  EFI_STRING_ID             mDefaultStoreNameId;
  UINT16                    mDefaultId;

  SVfrDefaultStoreNode      *mNext;

  SVfrDefaultStoreNode (IN EFI_IFR_DEFAULTSTORE *, IN INT8 *, IN EFI_STRING_ID, IN UINT16);
  ~SVfrDefaultStoreNode();
};

class CVfrDefaultStore {
private:
  SVfrDefaultStoreNode      *mDefaultStoreList;

public:
  CVfrDefaultStore ();
  ~CVfrDefaultStore ();

  EFI_VFR_RETURN_CODE RegisterDefaultStore (IN CHAR8 *, IN INT8 *, IN EFI_STRING_ID, IN UINT16);
  EFI_VFR_RETURN_CODE ReRegisterDefaultStoreById (IN UINT16, IN INT8 *, IN EFI_STRING_ID);
  BOOLEAN             DefaultIdRegistered (IN UINT16);
  EFI_VFR_RETURN_CODE GetDefaultId (IN INT8 *, OUT UINT16 *);
  EFI_VFR_RETURN_CODE BufferVarStoreAltConfigAdd (IN EFI_VARSTORE_ID, IN EFI_VARSTORE_INFO &, IN INT8 *, IN UINT8, IN EFI_IFR_TYPE_VALUE);
};

#define EFI_RULE_ID_START    0x01
#define EFI_RULE_ID_INVALID  0x00

struct SVfrRuleNode {
  UINT8                     mRuleId;
  INT8                      *mRuleName;
  SVfrRuleNode              *mNext;

  SVfrRuleNode(IN INT8 *, IN UINT8);
  ~SVfrRuleNode();
};

class CVfrRulesDB {
private:
  SVfrRuleNode              *mRuleList;
  UINT8                     mFreeRuleId;

public:
  CVfrRulesDB ();
  ~CVfrRulesDB();

  VOID RegisterRule (IN INT8 *);
  UINT8 GetRuleId (IN INT8 *);
};

#define MIN(v1, v2) (((v1) < (v2)) ? (v1) : (v2))
#define MAX(v1, v2) (((v1) > (v2)) ? (v1) : (v2))

#endif
