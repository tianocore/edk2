/** @file
  
  Vfr common library functions.

Copyright (c) 2004 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _VFRUTILITYLIB_H_
#define _VFRUTILITYLIB_H_

#include "string.h"
#include "Common/UefiBaseTypes.h"
#include "EfiVfr.h"
#include "VfrError.h"

extern BOOLEAN  VfrCompatibleMode;

#define MAX_NAME_LEN                       64
#define MAX_STRING_LEN                     0x100
#define DEFAULT_ALIGN                      1
#define DEFAULT_PACK_ALIGN                 0x8
#define DEFAULT_NAME_TABLE_ITEMS           1024

#define EFI_BITS_SHIFT_PER_UINT32          0x5
#define EFI_BITS_PER_UINT32                (1 << EFI_BITS_SHIFT_PER_UINT32)

#define BUFFER_SAFE_FREE(Buf)              do { if ((Buf) != NULL) { delete (Buf); } } while (0);

class CVfrBinaryOutput {
public:
  virtual VOID WriteLine (IN FILE *, IN UINT32, IN CONST CHAR8 *, IN CHAR8 *, IN UINT32);
  virtual VOID WriteEnd (IN FILE *, IN UINT32, IN CONST CHAR8 *, IN CHAR8 *, IN UINT32);
};

UINT32
_STR2U32 (
  IN CHAR8 *Str
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
  CHAR8         *mName;         // varstore name
  EFI_GUID      *mGuid;         // varstore guid, varstore name + guid deside one varstore
  CHAR8         *mId;           // default ID
  SConfigInfo   *mInfoStrList;  // list of Offset/Value in the varstore
  SConfigItem   *mNext;

public:
  SConfigItem (IN CHAR8 *, IN EFI_GUID *, IN CHAR8 *);
  SConfigItem (IN CHAR8 *, IN EFI_GUID *, IN CHAR8 *, IN UINT8, IN UINT16, IN UINT16, IN EFI_IFR_TYPE_VALUE);
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

  virtual UINT8   Register (IN CHAR8 *, IN EFI_GUID *,IN CHAR8 *Info = NULL);
  virtual VOID    Open (VOID);
  virtual BOOLEAN Eof(VOID);
  virtual UINT8   Select (IN CHAR8 *, IN EFI_GUID *, IN CHAR8 *Info = NULL);
  virtual UINT8   Write (IN CONST CHAR8, IN CHAR8 *, IN EFI_GUID *, IN CHAR8 *, IN UINT8, IN UINT16, IN UINT32, IN EFI_IFR_TYPE_VALUE);
#if 0
  virtual UINT8   Read (OUT CHAR8 **, OUT CHAR8 **, OUT CHAR8 **, OUT CHAR8 **, OUT CHAR8 **);
#endif
  virtual VOID    Close (VOID);
  virtual VOID    OutputCFile (IN FILE *, IN CHAR8 *);
};

extern CVfrBufferConfig gCVfrBufferConfig;

#define ALIGN_STUFF(Size, Align) ((Align) - (Size) % (Align))
#define INVALID_ARRAY_INDEX      0xFFFFFFFF

struct SVfrDataType;

struct SVfrDataField {
  CHAR8                     mFieldName[MAX_NAME_LEN];
  SVfrDataType              *mFieldType;
  UINT32                    mOffset;
  UINT32                    mArrayNum;
  SVfrDataField             *mNext;
};

struct SVfrDataType {
  CHAR8                     mTypeName[MAX_NAME_LEN];
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
  CHAR8                     *mIdentifier;
  UINT32                    mNumber;
  SVfrPackStackNode         *mNext;

  SVfrPackStackNode (IN CHAR8 *Identifier, IN UINT32 Number) {
    mIdentifier = NULL;
    mNumber     = Number;
    mNext       = NULL;

    if (Identifier != NULL) {
      mIdentifier = new CHAR8[strlen (Identifier) + 1];
      strcpy (mIdentifier, Identifier);
    }
  }

  ~SVfrPackStackNode (VOID) {
    if (mIdentifier != NULL) {
      delete mIdentifier;
    }
    mNext = NULL;
  }

  bool Match (IN CHAR8 *Identifier) {
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
  EFI_VFR_RETURN_CODE       Pack (IN UINT32, IN UINT8, IN CHAR8 *Identifier = NULL, IN UINT32 Number = DEFAULT_PACK_ALIGN);

private:
  SVfrDataType              *mDataTypeList;

  SVfrDataType              *mNewDataType;
  SVfrDataType              *mCurrDataType;
  SVfrDataField             *mCurrDataField;

  VOID InternalTypesListInit (VOID);
  VOID RegisterNewType (IN SVfrDataType *);

  EFI_VFR_RETURN_CODE ExtractStructTypeName (IN CHAR8 *&, OUT CHAR8 *);
  EFI_VFR_RETURN_CODE GetTypeField (IN CONST CHAR8 *, IN SVfrDataType *, IN SVfrDataField *&);
  EFI_VFR_RETURN_CODE GetFieldOffset (IN SVfrDataField *, IN UINT32, OUT UINT32 &);
  UINT8               GetFieldWidth (IN SVfrDataField *);
  UINT32              GetFieldSize (IN SVfrDataField *, IN UINT32);

public:
  CVfrVarDataTypeDB (VOID);
  ~CVfrVarDataTypeDB (VOID);

  VOID                DeclareDataTypeBegin (VOID);
  EFI_VFR_RETURN_CODE SetNewTypeName (IN CHAR8 *);
  EFI_VFR_RETURN_CODE DataTypeAddField (IN CHAR8 *, IN CHAR8 *, IN UINT32);
  VOID                DeclareDataTypeEnd (VOID);

  EFI_VFR_RETURN_CODE GetDataType (IN CHAR8 *, OUT SVfrDataType **);
  EFI_VFR_RETURN_CODE GetDataTypeSize (IN CHAR8 *, OUT UINT32 *);
  EFI_VFR_RETURN_CODE GetDataTypeSize (IN UINT8, OUT UINT32 *);
  EFI_VFR_RETURN_CODE GetDataFieldInfo (IN CHAR8 *, OUT UINT16 &, OUT UINT8 &, OUT UINT32 &);

  EFI_VFR_RETURN_CODE GetUserDefinedTypeNameList (OUT CHAR8 ***, OUT UINT32 *);
  EFI_VFR_RETURN_CODE ExtractFieldNameAndArrary (IN CHAR8 *&, OUT CHAR8 *, OUT UINT32 &);

  BOOLEAN             IsTypeNameDefined (IN CHAR8 *);

  VOID                Dump(IN FILE *);
  //
  // First the declared 
  //
  CHAR8               *mFirstNewDataTypeName;
#ifdef CVFR_VARDATATYPEDB_DEBUG
  VOID ParserDB ();
#endif
};

extern CVfrVarDataTypeDB  gCVfrVarDataTypeDB;

typedef enum {
  EFI_VFR_VARSTORE_INVALID,
  EFI_VFR_VARSTORE_BUFFER,
  EFI_VFR_VARSTORE_EFI,
  EFI_VFR_VARSTORE_NAME
} EFI_VFR_VARSTORE_TYPE;

struct SVfrVarStorageNode {
  EFI_GUID                  mGuid;
  CHAR8                     *mVarStoreName;
  EFI_VARSTORE_ID           mVarStoreId;
  BOOLEAN                   mAssignedFlag; //Create varstore opcode
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
  SVfrVarStorageNode (IN EFI_GUID *, IN CHAR8 *, IN EFI_VARSTORE_ID, IN EFI_STRING_ID, IN UINT32, IN BOOLEAN Flag = TRUE);
  SVfrVarStorageNode (IN EFI_GUID *, IN CHAR8 *, IN EFI_VARSTORE_ID, IN SVfrDataType *, IN BOOLEAN Flag = TRUE);
  SVfrVarStorageNode (IN CHAR8 *, IN EFI_VARSTORE_ID);
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

  EFI_VARSTORE_ID GetFreeVarStoreId (EFI_VFR_VARSTORE_TYPE VarType = EFI_VFR_VARSTORE_BUFFER);
  BOOLEAN         ChekVarStoreIdFree (IN EFI_VARSTORE_ID);
  VOID            MarkVarStoreIdUsed (IN EFI_VARSTORE_ID);
  VOID            MarkVarStoreIdUnused (IN EFI_VARSTORE_ID);
  EFI_VARSTORE_ID CheckGuidField (IN SVfrVarStorageNode *, 
                                  IN EFI_GUID *, 
                                  IN BOOLEAN *, 
                                  OUT EFI_VFR_RETURN_CODE *);

public:
  CVfrDataStorage ();
  ~CVfrDataStorage ();
  
  SVfrVarStorageNode * GetBufferVarStoreList () {
    return mBufferVarStoreList;
  }
  SVfrVarStorageNode * GetEfiVarStoreList () {
    return mEfiVarStoreList;
  }
  EFI_VFR_RETURN_CODE DeclareNameVarStoreBegin (CHAR8 *, EFI_VARSTORE_ID);
  EFI_VFR_RETURN_CODE NameTableAddItem (EFI_STRING_ID);
  EFI_VFR_RETURN_CODE DeclareNameVarStoreEnd (EFI_GUID *);

  EFI_VFR_RETURN_CODE DeclareEfiVarStore (IN CHAR8 *, IN EFI_GUID *, IN EFI_STRING_ID, IN UINT32, IN BOOLEAN Flag = TRUE);

  EFI_VFR_RETURN_CODE DeclareBufferVarStore (IN CHAR8 *, IN EFI_GUID *, IN CVfrVarDataTypeDB *, IN CHAR8 *, IN EFI_VARSTORE_ID, IN BOOLEAN Flag = TRUE);

  EFI_VFR_RETURN_CODE GetVarStoreId (IN CHAR8 *, OUT EFI_VARSTORE_ID *, IN EFI_GUID *VarGuid = NULL);
  EFI_VFR_VARSTORE_TYPE GetVarStoreType (IN EFI_VARSTORE_ID);
  EFI_GUID *          GetVarStoreGuid (IN  EFI_VARSTORE_ID);
  EFI_VFR_RETURN_CODE GetVarStoreName (IN EFI_VARSTORE_ID, OUT CHAR8 **);
  EFI_VFR_RETURN_CODE GetVarStoreByDataType (IN CHAR8 *, OUT SVfrVarStorageNode **, IN EFI_GUID *VarGuid = NULL);

  EFI_VFR_RETURN_CODE GetBufferVarStoreDataTypeName (IN EFI_VARSTORE_ID, OUT CHAR8 **);
  EFI_VFR_RETURN_CODE GetEfiVarStoreInfo (IN EFI_VARSTORE_INFO *);
  EFI_VFR_RETURN_CODE GetNameVarStoreInfo (IN EFI_VARSTORE_INFO *, IN UINT32);
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
  CHAR8                     *mName;
  CHAR8                     *mVarIdStr;
  EFI_QUESTION_ID           mQuestionId;
  UINT32                    mBitMask;
  SVfrQuestionNode          *mNext;
  EFI_QUESION_TYPE          mQtype;

  SVfrQuestionNode (IN CHAR8 *, IN CHAR8 *, IN UINT32 BitMask = 0);
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

  EFI_VFR_RETURN_CODE RegisterQuestion (IN CHAR8 *, IN CHAR8 *, IN OUT EFI_QUESTION_ID &);
  VOID                RegisterOldDateQuestion (IN CHAR8 *, IN CHAR8 *, IN CHAR8 *, IN OUT EFI_QUESTION_ID &);
  VOID                RegisterNewDateQuestion (IN CHAR8 *, IN CHAR8 *, IN OUT EFI_QUESTION_ID &);
  VOID                RegisterOldTimeQuestion (IN CHAR8 *, IN CHAR8 *, IN CHAR8 *, IN OUT EFI_QUESTION_ID &);
  VOID                RegisterNewTimeQuestion (IN CHAR8 *, IN CHAR8 *, IN OUT EFI_QUESTION_ID &);
  VOID                RegisterRefQuestion (IN CHAR8 *, IN CHAR8 *, IN OUT EFI_QUESTION_ID &);  
  EFI_VFR_RETURN_CODE UpdateQuestionId (IN EFI_QUESTION_ID, IN EFI_QUESTION_ID);
  VOID                GetQuestionId (IN CHAR8 *, IN CHAR8 *, OUT EFI_QUESTION_ID &, OUT UINT32 &, OUT EFI_QUESION_TYPE *QType = NULL);
  EFI_VFR_RETURN_CODE FindQuestion (IN EFI_QUESTION_ID);
  EFI_VFR_RETURN_CODE FindQuestion (IN CHAR8 *);
  VOID                PrintAllQuestion (IN VOID);
  VOID                ResetInit (IN VOID); 

  VOID SetCompatibleMode (IN BOOLEAN Mode) {
    VfrCompatibleMode = Mode;
  }
};

struct SVfrDefaultStoreNode {
  EFI_IFR_DEFAULTSTORE      *mObjBinAddr;
  CHAR8                     *mRefName;
  EFI_STRING_ID             mDefaultStoreNameId;
  UINT16                    mDefaultId;

  SVfrDefaultStoreNode      *mNext;

  SVfrDefaultStoreNode (IN EFI_IFR_DEFAULTSTORE *, IN CHAR8 *, IN EFI_STRING_ID, IN UINT16);
  ~SVfrDefaultStoreNode();
};

class CVfrDefaultStore {
private:
  SVfrDefaultStoreNode      *mDefaultStoreList;

public:
  CVfrDefaultStore ();
  ~CVfrDefaultStore ();

  EFI_VFR_RETURN_CODE RegisterDefaultStore (IN CHAR8 *, IN CHAR8 *, IN EFI_STRING_ID, IN UINT16);
  EFI_VFR_RETURN_CODE ReRegisterDefaultStoreById (IN UINT16, IN CHAR8 *, IN EFI_STRING_ID);
  BOOLEAN             DefaultIdRegistered (IN UINT16);
  EFI_VFR_RETURN_CODE GetDefaultId (IN CHAR8 *, OUT UINT16 *);
  EFI_VFR_RETURN_CODE BufferVarStoreAltConfigAdd (IN EFI_VARSTORE_ID, IN EFI_VARSTORE_INFO &, IN CHAR8 *, IN EFI_GUID *, IN UINT8, IN EFI_IFR_TYPE_VALUE);
};

#define EFI_RULE_ID_START    0x01
#define EFI_RULE_ID_INVALID  0x00

struct SVfrRuleNode {
  UINT8                     mRuleId;
  CHAR8                     *mRuleName;
  SVfrRuleNode              *mNext;

  SVfrRuleNode(IN CHAR8 *, IN UINT8);
  ~SVfrRuleNode();
};

class CVfrRulesDB {
private:
  SVfrRuleNode              *mRuleList;
  UINT8                     mFreeRuleId;

public:
  CVfrRulesDB ();
  ~CVfrRulesDB();

  VOID RegisterRule (IN CHAR8 *);
  UINT8 GetRuleId (IN CHAR8 *);
};

class CVfrStringDB {
private:
  CHAR8   *mStringFileName;

  EFI_STATUS FindStringBlock (
    IN  UINT8            *StringData,
    IN  EFI_STRING_ID    StringId,
    OUT UINT32           *StringTextOffset,
    OUT UINT8            *BlockType
    );

  UINT32 GetUnicodeStringTextSize (
    IN  UINT8            *StringSrc
    );
    
  BOOLEAN GetBestLanguage (
    IN CONST CHAR8  *SupportedLanguages,
    IN CHAR8        *Language
    );

public:
  CVfrStringDB ();
  ~CVfrStringDB ();

  VOID SetStringFileName (
    IN CHAR8 *StringFileName
    );

  CHAR8 * GetVarStoreNameFormStringId (
    IN EFI_STRING_ID StringId
    );

};

#endif
