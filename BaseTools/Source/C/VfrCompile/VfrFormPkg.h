/** @file

  The definition of CFormPkg's member function

Copyright (c) 2004 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFIIFRCLASS_H_
#define _EFIIFRCLASS_H_

#include "string.h"
#include "EfiVfr.h"
#include "VfrError.h"
#include "VfrUtilityLib.h"

#define NO_QST_REFED "no question refered"

struct PACKAGE_DATA {
  CHAR8   *Buffer;
  UINT32  Size;
};

/*
 * The functions below are used for flags setting
 */
static inline BOOLEAN _FLAGS_ZERO (
  IN UINT8 &Flags
  )
{
  return Flags == 0;
}

static inline VOID _FLAG_CLEAR (
  IN UINT8 &Flags,
  IN UINT8 Mask
  )
{
  Flags &= (~Mask);
}

static inline UINT8 _FLAG_TEST_AND_CLEAR (
  IN UINT8 &Flags,
  IN UINT8 Mask
  )
{
  UINT8 Ret = Flags & Mask;
  Flags &= (~Mask);
  return Ret;
}

static inline UINT8 _IS_EQUAL (
  IN UINT8 &Flags,
  IN UINT8 Value
  )
{
  return Flags == Value;
}

/*
 * The definition of CIfrBin
 */
typedef enum {
  PENDING,
  ASSIGNED
} ASSIGN_FLAG;

struct SPendingAssign {
  CHAR8                   *mKey;  // key ! unique
  VOID                    *mAddr;
  UINT32                  mLen;
  ASSIGN_FLAG             mFlag;
  UINT32                  mLineNo;
  CHAR8                   *mMsg;
  struct SPendingAssign   *mNext;

  SPendingAssign (IN CHAR8 *, IN VOID *, IN UINT32, IN UINT32, IN CONST CHAR8 *);
  ~SPendingAssign ();

  VOID   SetAddrAndLen (IN VOID *, IN UINT32);
  VOID   AssignValue (IN VOID *, IN UINT32);
  CHAR8 * GetKey (VOID);

private:
  SPendingAssign (IN CONST SPendingAssign&);             // Prevent copy-construction
  SPendingAssign& operator= (IN CONST SPendingAssign&);  // Prevent assignment
};

struct SBufferNode {
  CHAR8              *mBufferStart;
  CHAR8              *mBufferEnd;
  CHAR8              *mBufferFree;
  struct SBufferNode *mNext;
};

typedef struct {
  EFI_GUID *OverrideClassGuid;
} INPUT_INFO_TO_SYNTAX;

class CFormPkg {
private:
  UINT32              mBufferSize;
  SBufferNode         *mBufferNodeQueueHead;
  SBufferNode         *mBufferNodeQueueTail;
  SBufferNode         *mCurrBufferNode;

  SBufferNode         *mReadBufferNode;
  UINT32              mReadBufferOffset;

  UINT32              mPkgLength;

  VOID                _WRITE_PKG_LINE (IN FILE *, IN UINT32 , IN CONST CHAR8 *, IN CHAR8 *, IN UINT32);
  VOID                _WRITE_PKG_END (IN FILE *, IN UINT32 , IN CONST CHAR8 *, IN CHAR8 *, IN UINT32);
  SBufferNode *       GetBinBufferNodeForAddr (IN CHAR8 *);
  SBufferNode *       CreateNewNode ();
  SBufferNode *       GetNodeBefore (IN SBufferNode *);
  EFI_VFR_RETURN_CODE InsertNodeBefore (IN SBufferNode *, IN SBufferNode *);

private:
  SPendingAssign      *PendingAssignList;

public:
  CFormPkg (IN UINT32 BufferSize = 4096);
  ~CFormPkg ();

  CHAR8             * IfrBinBufferGet (IN UINT32);
  inline UINT32       GetPkgLength (VOID);

  VOID                Open ();
  UINT32              Read (IN CHAR8 *, IN UINT32);
  VOID                Close ();

  EFI_VFR_RETURN_CODE BuildPkgHdr (OUT EFI_HII_PACKAGE_HEADER **);
  EFI_VFR_RETURN_CODE BuildPkg (IN FILE *, IN PACKAGE_DATA *PkgData = NULL);
  EFI_VFR_RETURN_CODE BuildPkg (OUT PACKAGE_DATA &);
  EFI_VFR_RETURN_CODE GenCFile (IN CHAR8 *, IN FILE *, IN PACKAGE_DATA *PkgData = NULL);

private:
  CFormPkg (IN CONST CFormPkg&);             // Prevent copy-construction
  CFormPkg& operator= (IN CONST CFormPkg&);  // Prevent assignment

public:
  EFI_VFR_RETURN_CODE AssignPending (IN CHAR8 *, IN VOID *, IN UINT32, IN UINT32, IN CONST CHAR8 *Msg = NULL);
  VOID                DoPendingAssign (IN CHAR8 *, IN VOID *, IN UINT32);
  bool                HavePendingUnassigned (VOID);
  VOID                PendingAssignPrintAll (VOID);
  EFI_VFR_RETURN_CODE   DeclarePendingQuestion (
    IN CVfrVarDataTypeDB   &lCVfrVarDataTypeDB,
    IN CVfrDataStorage     &lCVfrDataStorage,
    IN CVfrQuestionDB      &lCVfrQuestionDB,
    IN EFI_GUID            *LocalFormSetGuid,
    IN UINT32              LineNo,
    OUT CHAR8              **InsertOpcodeAddr
    );
  EFI_VFR_RETURN_CODE AdjustDynamicInsertOpcode (
    IN CHAR8              *LastFormEndAddr,
    IN CHAR8              *InsertOpcodeAddr,
    IN BOOLEAN            CreateOpcodeAfterParsingVfr
    );
  CHAR8 *             GetBufAddrBaseOnOffset (
    IN UINT32             Offset
    );
};

extern CFormPkg       gCFormPkg;
extern CVfrStringDB   gCVfrStringDB;
extern UINT32         gAdjustOpcodeOffset;
extern BOOLEAN        gNeedAdjustOpcode;

struct SIfrRecord {
  UINT32     mLineNo;
  CHAR8      *mIfrBinBuf;
  UINT8      mBinBufLen;
  UINT32     mOffset;
  SIfrRecord *mNext;

  SIfrRecord (VOID);
  ~SIfrRecord (VOID);
};


#define EFI_IFR_RECORDINFO_IDX_INVALUD 0xFFFFFF
#define EFI_IFR_RECORDINFO_IDX_START   0x0
#define EFI_HII_MAX_SUPPORT_DEFAULT_TYPE  0x08

struct QuestionDefaultRecord {
  BOOLEAN     mIsDefaultIdExist[EFI_HII_MAX_SUPPORT_DEFAULT_TYPE]; // Record the default id in mAllDefaultIdArray[EFI_HII_MAX_SUPPORT_DEFAULT_TYPE]
                                                                   // whether exists in current question.

  SIfrRecord  *mDefaultValueRecord;   // Point to the default value record in RecordList which has smallest default Id.
                                      // (for checkbox it may be NULL, because the dedault value is always true when the flag is set.)

  BOOLEAN     mIsDefaultOpcode;       // whether the default value with smallest default id is given by default opcode.
                                      // (for oneof and checkbox default info may be given by flag.)

  UINT16      mDefaultNumber;         // The default number of this question.
};

class CIfrRecordInfoDB {
private:
  bool       mSwitch;
  UINT32     mRecordCount;
  SIfrRecord *mIfrRecordListHead;
  SIfrRecord *mIfrRecordListTail;
  UINT8      mAllDefaultTypeCount;
  UINT16     mAllDefaultIdArray[EFI_HII_MAX_SUPPORT_DEFAULT_TYPE];

  SIfrRecord * GetRecordInfoFromIdx (IN UINT32);
  BOOLEAN          CheckQuestionOpCode (IN UINT8);
  BOOLEAN          CheckIdOpCode (IN UINT8);
  EFI_QUESTION_ID  GetOpcodeQuestionId (IN EFI_IFR_OP_HEADER *);
public:
  CIfrRecordInfoDB (VOID);
  ~CIfrRecordInfoDB (VOID);

  inline VOID TurnOn (VOID) {
    mSwitch = TRUE;
  }

  inline VOID TurnOff (VOID) {
    mSwitch = FALSE;
  }

  SIfrRecord * GetRecordInfoFromOffset (IN UINT32);
  VOID        IfrAdjustOffsetForRecord (VOID);
  BOOLEAN     IfrAdjustDynamicOpcodeInRecords (IN BOOLEAN);

  UINT32      IfrRecordRegister (IN UINT32, IN CHAR8 *, IN UINT8, IN UINT32);
  VOID        IfrRecordInfoUpdate (IN UINT32, IN UINT32, IN CHAR8*, IN UINT8, IN UINT32);
  VOID        IfrRecordOutput (IN FILE *, IN UINT32 LineNo);
  VOID        IfrRecordOutput (OUT PACKAGE_DATA &);
  EFI_VFR_RETURN_CODE  IfrRecordAdjust (VOID);
  VOID        IfrUpdateRecordInfoForDynamicOpcode (IN BOOLEAN);
  VOID        IfrCheckAddDefaultRecord (IN BOOLEAN, IN BOOLEAN);
  VOID        IfrGetDefaultStoreInfo ();
  VOID        IfrCreateDefaultRecord (IN UINT8 Size,IN UINT16 DefaultId,IN UINT8 Type,IN UINT32 LineNo,IN EFI_IFR_TYPE_VALUE Value);
  VOID        IfrCreateDefaultForQuestion (IN  SIfrRecord *, IN  QuestionDefaultRecord *);
  VOID        IfrParseDefaulInfoInQuestion (IN  SIfrRecord *, OUT QuestionDefaultRecord *);
  VOID        IfrAddDefaultToBufferConfig (IN  UINT16, IN  SIfrRecord *,IN  EFI_IFR_TYPE_VALUE);

private:
  CIfrRecordInfoDB (IN CONST CIfrRecordInfoDB&);             // Prevent copy-construction
  CIfrRecordInfoDB& operator= (IN CONST CIfrRecordInfoDB&);  // Prevent assignment
};

extern CIfrRecordInfoDB gCIfrRecordInfoDB;

/*
 * The definition of CIfrObj
 */
extern BOOLEAN  gCreateOp;

class CIfrObj {
private:
  BOOLEAN mDelayEmit;

  CHAR8   *mObjBinBuf;
  UINT8   mObjBinLen;
  UINT32  mLineNo;
  UINT32  mRecordIdx;
  UINT32  mPkgOffset;

public:
  CIfrObj (IN UINT8 OpCode, OUT CHAR8 **IfrObj = NULL, IN UINT8 ObjBinLen = 0, IN BOOLEAN DelayEmit = FALSE);
  virtual ~CIfrObj(VOID);

  VOID    _EMIT_PENDING_OBJ (VOID);

  inline VOID    SetLineNo (IN UINT32 LineNo) {
    mLineNo = LineNo;
  }

  template<typename T>
  inline T * GetObjBinAddr (VOID) {
    return reinterpret_cast<T *>(mObjBinBuf);
  }

  inline UINT32 GetObjBinOffset (VOID) {
    return mPkgOffset;
  }

  inline UINT8   GetObjBinLen (VOID) {
    return mObjBinLen;
  }

  inline bool ExpendObjBin (IN UINT8 Size) {
    if ((mDelayEmit == TRUE) && ((mObjBinLen + Size) > mObjBinLen)) {
      mObjBinLen = mObjBinLen + Size;
      return TRUE;
    } else {
      return FALSE;
    }
  }

  inline bool ShrinkObjBin (IN UINT8 Size) {
    if ((mDelayEmit == TRUE) && (mObjBinLen > Size)) {
      mObjBinLen -= Size;
      return TRUE;
    } else {
      return FALSE;
    }
  }
};

/*
 * The definition of CIfrOpHeader
 */
class CIfrOpHeader {
private:
  EFI_IFR_OP_HEADER *mHeader;

public:
  CIfrOpHeader (IN UINT8 OpCode, IN VOID *StartAddr, IN UINT8 Length = 0);
  CIfrOpHeader (IN CIfrOpHeader &);
  CIfrOpHeader& operator=(IN CONST CIfrOpHeader &);

  VOID IncLength (UINT8 Size) {
    if ((mHeader->Length + Size) > mHeader->Length) {
      mHeader->Length = mHeader->Length + Size;
    }
  }

  VOID DecLength (UINT8 Size) {
    if (mHeader->Length >= Size) {
      mHeader->Length -= Size;
    }
  }

  UINT8 GetLength () {
    return mHeader->Length;
  }

  UINT8 GetScope () {
    return mHeader->Scope;
  }

  VOID SetScope (IN UINT8 Scope) {
    mHeader->Scope = Scope;
  }

  VOID UpdateHeader (IN EFI_IFR_OP_HEADER *Header) {
    mHeader = Header;
  }

  UINT8 GetOpCode () {
    return mHeader->OpCode;
  }
};

extern UINT8 gScopeCount;

/*
 * The definition of CIfrStatementHeader
 */
class CIfrStatementHeader {
private:
  EFI_IFR_STATEMENT_HEADER *mHeader;

public:
  CIfrStatementHeader (
    IN EFI_IFR_STATEMENT_HEADER *StartAddr
  ) : mHeader ((EFI_IFR_STATEMENT_HEADER *)StartAddr) {
    mHeader         = StartAddr;
    mHeader->Help   = EFI_STRING_ID_INVALID;
    mHeader->Prompt = EFI_STRING_ID_INVALID;
  }

  EFI_IFR_STATEMENT_HEADER *GetStatementHeader () {
    return mHeader;
  }

  VOID SetPrompt (IN EFI_STRING_ID Prompt) {
    mHeader->Prompt = Prompt;
  }

  VOID SetHelp (IN EFI_STRING_ID Help) {
    mHeader->Help = Help;
  }
};

/*
 * The definition of CIfrQuestionHeader
 */
#define EFI_IFR_QUESTION_FLAG_DEFAULT 0

class CIfrQuestionHeader : public CIfrStatementHeader {
private:
  EFI_IFR_QUESTION_HEADER *mHeader;

  EFI_IFR_STATEMENT_HEADER * QH2SH (EFI_IFR_QUESTION_HEADER *Qheader) {
    return &(Qheader)->Header;
  }

public:
  EFI_QUESTION_ID QUESTION_ID (VOID) {
    return mHeader->QuestionId;
  }

  EFI_VARSTORE_ID VARSTORE_ID (VOID) {
    return mHeader->VarStoreId;
  }

  VOID VARSTORE_INFO (OUT EFI_VARSTORE_INFO *Info) {
    if (Info != NULL) {
      Info->mVarStoreId   = mHeader->VarStoreId;
      memmove (&Info->mVarStoreId, &mHeader->VarStoreInfo, sizeof (Info->mVarStoreId));
    }
  }

  UINT8 FLAGS (VOID) {
    return mHeader->Flags;
  }

public:
  CIfrQuestionHeader (
    IN EFI_IFR_QUESTION_HEADER *StartAddr,
    IN UINT8 Flags = EFI_IFR_QUESTION_FLAG_DEFAULT
  ) : CIfrStatementHeader (QH2SH(StartAddr)) {
    mHeader                         = StartAddr;
    mHeader->QuestionId             = EFI_QUESTION_ID_INVALID;
    mHeader->VarStoreId             = EFI_VARSTORE_ID_INVALID;
    mHeader->VarStoreInfo.VarName   = EFI_STRING_ID_INVALID;
    mHeader->VarStoreInfo.VarOffset = EFI_VAROFFSET_INVALID;
    mHeader->Flags                  = Flags;
  }

  VOID SetQuestionId (IN EFI_QUESTION_ID QuestionId) {
    mHeader->QuestionId = QuestionId;
  }

  VOID SetVarStoreInfo (IN EFI_VARSTORE_INFO *Info) {
    mHeader->VarStoreId             = Info->mVarStoreId;
    mHeader->VarStoreInfo.VarName   = Info->mInfo.mVarName;
    mHeader->VarStoreInfo.VarOffset = Info->mInfo.mVarOffset;
  }

  EFI_VFR_RETURN_CODE SetFlags (IN UINT8 Flags) {
    if (_FLAG_TEST_AND_CLEAR (Flags, EFI_IFR_FLAG_READ_ONLY)) {
      mHeader->Flags |= EFI_IFR_FLAG_READ_ONLY;
    }

    _FLAG_CLEAR (Flags, 0x02);

    if (_FLAG_TEST_AND_CLEAR (Flags, EFI_IFR_FLAG_CALLBACK)) {
      mHeader->Flags |= EFI_IFR_FLAG_CALLBACK;
    }

    //
    // ignore NVAccessFlag
    //
    _FLAG_CLEAR (Flags, 0x08);

    if (_FLAG_TEST_AND_CLEAR (Flags, EFI_IFR_FLAG_RESET_REQUIRED)) {
      mHeader->Flags |= EFI_IFR_FLAG_RESET_REQUIRED;
    }

    if (_FLAG_TEST_AND_CLEAR (Flags, EFI_IFR_FLAG_RECONNECT_REQUIRED)) {
      mHeader->Flags |= EFI_IFR_FLAG_RECONNECT_REQUIRED;
    }

    //
    //  Set LateCheck Flag to compatible for framework flag
    //  but it uses 0x20 as its flag, if in the future UEFI may take this flag
    //
    if (_FLAG_TEST_AND_CLEAR (Flags, 0x20)) {
      mHeader->Flags |= 0x20;
    }

    if (_FLAG_TEST_AND_CLEAR (Flags, EFI_IFR_FLAG_OPTIONS_ONLY)) {
      mHeader->Flags |= EFI_IFR_FLAG_OPTIONS_ONLY;
    }

    return _FLAGS_ZERO (Flags) ? VFR_RETURN_SUCCESS : VFR_RETURN_FLAGS_UNSUPPORTED;
  }

  VOID UpdateCIfrQuestionHeader (IN EFI_IFR_QUESTION_HEADER *Header) {
    mHeader = Header;
  }
};

/*
 * The definition of CIfrMinMaxStepData
 */
class CIfrMinMaxStepData {
private:
  MINMAXSTEP_DATA *mMinMaxStepData;
  BOOLEAN         ValueIsSet;
  BOOLEAN         IsNumeric;

public:
  CIfrMinMaxStepData (MINMAXSTEP_DATA *DataAddr, BOOLEAN NumericOpcode=FALSE) : mMinMaxStepData (DataAddr) {
    mMinMaxStepData->u64.MinValue = 0;
    mMinMaxStepData->u64.MaxValue = 0;
    mMinMaxStepData->u64.Step     = 0;
    ValueIsSet = FALSE;
    IsNumeric = NumericOpcode;
  }

  VOID SetMinMaxStepData (IN UINT64 MinValue, IN UINT64 MaxValue, IN UINT64 Step) {
    if (!ValueIsSet) {
      mMinMaxStepData->u64.MinValue = MinValue;
      mMinMaxStepData->u64.MaxValue = MaxValue;
      ValueIsSet = TRUE;
    } else {
      if (MinValue < mMinMaxStepData->u64.MinValue) {
        mMinMaxStepData->u64.MinValue = MinValue;
      }
      if (MaxValue > mMinMaxStepData->u64.MaxValue) {
        mMinMaxStepData->u64.MaxValue = MaxValue;
      }
    }
    mMinMaxStepData->u64.Step = Step;
  }

  VOID SetMinMaxStepData (IN UINT32 MinValue, IN UINT32 MaxValue, IN UINT32 Step) {
    if (!ValueIsSet) {
      mMinMaxStepData->u32.MinValue = MinValue;
      mMinMaxStepData->u32.MaxValue = MaxValue;
      ValueIsSet = TRUE;
    } else {
      if (MinValue < mMinMaxStepData->u32.MinValue) {
        mMinMaxStepData->u32.MinValue = MinValue;
      }
      if (MaxValue > mMinMaxStepData->u32.MaxValue) {
        mMinMaxStepData->u32.MaxValue = MaxValue;
      }
    }
    mMinMaxStepData->u32.Step = Step;
  }

  VOID SetMinMaxStepData (IN UINT16 MinValue, IN UINT16 MaxValue, IN UINT16 Step) {
    if (!ValueIsSet) {
      mMinMaxStepData->u16.MinValue = MinValue;
      mMinMaxStepData->u16.MaxValue = MaxValue;
      ValueIsSet = TRUE;
    } else {
      if (MinValue < mMinMaxStepData->u16.MinValue) {
        mMinMaxStepData->u16.MinValue = MinValue;
      }
      if (MaxValue > mMinMaxStepData->u16.MaxValue) {
        mMinMaxStepData->u16.MaxValue = MaxValue;
      }
    }
    mMinMaxStepData->u16.Step = Step;
  }

  VOID SetMinMaxStepData (IN UINT8 MinValue, IN UINT8 MaxValue, IN UINT8 Step) {
    if (!ValueIsSet) {
      mMinMaxStepData->u8.MinValue = MinValue;
      mMinMaxStepData->u8.MaxValue = MaxValue;
      ValueIsSet = TRUE;
    } else {
      if (MinValue < mMinMaxStepData->u8.MinValue) {
        mMinMaxStepData->u8.MinValue = MinValue;
      }
      if (MaxValue > mMinMaxStepData->u8.MaxValue) {
        mMinMaxStepData->u8.MaxValue = MaxValue;
      }
    }
    mMinMaxStepData->u8.Step = Step;
  }

  UINT64 GetMinData (UINT8 VarType, BOOLEAN IsBitVar) {
    UINT64 MinValue = 0;
    if (IsBitVar) {
      MinValue = mMinMaxStepData->u32.MinValue;
      return MinValue;
    }
    switch (VarType) {
    case EFI_IFR_TYPE_NUM_SIZE_64:
      MinValue = mMinMaxStepData->u64.MinValue;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_32:
      MinValue = (UINT64) mMinMaxStepData->u32.MinValue;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_16:
      MinValue = (UINT64) mMinMaxStepData->u16.MinValue;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_8:
      MinValue = (UINT64) mMinMaxStepData->u8.MinValue;
      break;
    default:
      break;
    }
    return MinValue;
  }

  UINT64 GetMaxData (UINT8 VarType, BOOLEAN IsBitVar) {
    UINT64 MaxValue = 0;
    if (IsBitVar) {
      MaxValue = mMinMaxStepData->u32.MaxValue;
      return MaxValue;
    }
    switch (VarType) {
    case EFI_IFR_TYPE_NUM_SIZE_64:
      MaxValue = mMinMaxStepData->u64.MaxValue;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_32:
      MaxValue = (UINT64) mMinMaxStepData->u32.MaxValue;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_16:
      MaxValue = (UINT64) mMinMaxStepData->u16.MaxValue;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_8:
      MaxValue = (UINT64) mMinMaxStepData->u8.MaxValue;
      break;
    default:
      break;
    }
    return MaxValue;
  }

  UINT64 GetStepData (UINT8 VarType, BOOLEAN IsBitVar) {
    UINT64 MaxValue = 0;
    if (IsBitVar) {
      MaxValue = mMinMaxStepData->u32.Step;
      return MaxValue;
    }
    switch (VarType) {
    case EFI_IFR_TYPE_NUM_SIZE_64:
      MaxValue = mMinMaxStepData->u64.Step;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_32:
      MaxValue = (UINT64) mMinMaxStepData->u32.Step;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_16:
      MaxValue = (UINT64) mMinMaxStepData->u16.Step;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_8:
      MaxValue = (UINT64) mMinMaxStepData->u8.Step;
      break;
    default:
      break;
    }
    return MaxValue;
  }

  BOOLEAN IsNumericOpcode () {
    return IsNumeric;
  }

  VOID UpdateCIfrMinMaxStepData (IN MINMAXSTEP_DATA *MinMaxStepData) {
    mMinMaxStepData = MinMaxStepData;
  }
};

static CIfrQuestionHeader *gCurrentQuestion  = NULL;
static CIfrMinMaxStepData *gCurrentMinMaxData = NULL;
static BOOLEAN            gIsOrderedList = FALSE;
static BOOLEAN            gIsStringOp = FALSE;

/*
 * The definition of all of the UEFI IFR Objects
 */
class CIfrFormSet : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_FORM_SET *mFormSet;
  EFI_GUID *mClassGuid;

public:
  CIfrFormSet (UINT8 Size) : CIfrObj (EFI_IFR_FORM_SET_OP, (CHAR8 **)NULL, Size),
                   CIfrOpHeader (EFI_IFR_FORM_SET_OP, &(GetObjBinAddr<EFI_IFR_FORM_SET>())->Header, Size), mFormSet(GetObjBinAddr<EFI_IFR_FORM_SET>()) {
    mFormSet->Help         = EFI_STRING_ID_INVALID;
    mFormSet->FormSetTitle = EFI_STRING_ID_INVALID;
    mFormSet->Flags        = 0;
    memset (&mFormSet->Guid, 0, sizeof (EFI_GUID));
    mClassGuid = (EFI_GUID *) (mFormSet + 1);
  }

  VOID SetGuid (IN EFI_GUID *Guid) {
    memmove (&mFormSet->Guid, Guid, sizeof (EFI_GUID));
  }

  VOID SetFormSetTitle (IN EFI_STRING_ID FormSetTitle) {
    mFormSet->FormSetTitle = FormSetTitle;
  }

  VOID SetHelp (IN EFI_STRING_ID Help) {
    mFormSet->Help = Help;
  }

  VOID SetClassGuid (IN EFI_GUID *Guid) {
    memmove (&(mClassGuid[mFormSet->Flags++]), Guid, sizeof (EFI_GUID));
  }

  UINT8 GetFlags() {
    return mFormSet->Flags;
  }
};

class CIfrEnd : public CIfrObj, public CIfrOpHeader {
public:
  CIfrEnd () : CIfrObj (EFI_IFR_END_OP),
               CIfrOpHeader (EFI_IFR_END_OP, &(GetObjBinAddr<EFI_IFR_END>())->Header) {}
};

class CIfrDefaultStore : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_DEFAULTSTORE *mDefaultStore;

public:
  CIfrDefaultStore () : CIfrObj (EFI_IFR_DEFAULTSTORE_OP),
                       CIfrOpHeader (EFI_IFR_DEFAULTSTORE_OP, &(GetObjBinAddr<EFI_IFR_DEFAULTSTORE>())->Header), mDefaultStore(GetObjBinAddr<EFI_IFR_DEFAULTSTORE>()) {
    mDefaultStore->DefaultId   = EFI_VARSTORE_ID_INVALID;
    mDefaultStore->DefaultName = EFI_STRING_ID_INVALID;
  }

  VOID SetDefaultName (IN EFI_STRING_ID DefaultName) {
    mDefaultStore->DefaultName = DefaultName;
  }

  VOID SetDefaultId (IN UINT16 DefaultId) {
    mDefaultStore->DefaultId = DefaultId;
  }
};

#define EFI_FORM_ID_MAX                    0xFFFF
#define EFI_FREE_FORM_ID_BITMAP_SIZE     ((EFI_FORM_ID_MAX + 1) / EFI_BITS_PER_UINT32)

class CIfrFormId {
public:
  STATIC UINT32 FormIdBitMap[EFI_FREE_FORM_ID_BITMAP_SIZE];

  STATIC BOOLEAN ChekFormIdFree (IN EFI_FORM_ID FormId) {
    UINT32 Index  = (FormId / EFI_BITS_PER_UINT32);
    UINT32 Offset = (FormId % EFI_BITS_PER_UINT32);

    return (FormIdBitMap[Index] & (0x80000000 >> Offset)) == 0;
  }

  STATIC VOID MarkFormIdUsed (IN EFI_FORM_ID FormId) {
    UINT32 Index  = (FormId / EFI_BITS_PER_UINT32);
    UINT32 Offset = (FormId % EFI_BITS_PER_UINT32);

    FormIdBitMap[Index] |= (0x80000000 >> Offset);
  }
};

class CIfrForm : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_FORM  *mForm;

public:
  CIfrForm () : CIfrObj (EFI_IFR_FORM_OP),
                CIfrOpHeader (EFI_IFR_FORM_OP, &(GetObjBinAddr<EFI_IFR_FORM>())->Header), mForm(GetObjBinAddr<EFI_IFR_FORM>()) {
    mForm->FormId    = 0;
    mForm->FormTitle = EFI_STRING_ID_INVALID;
  }

  EFI_VFR_RETURN_CODE SetFormId (IN EFI_FORM_ID FormId) {
    if (FormId == 0) {
      //
      // FormId can't be 0.
      //
      return VFR_RETURN_INVALID_PARAMETER;
    }
    if (CIfrFormId::ChekFormIdFree (FormId) == FALSE) {
      return VFR_RETURN_FORMID_REDEFINED;
    }
    mForm->FormId = FormId;
    CIfrFormId::MarkFormIdUsed (FormId);
    return VFR_RETURN_SUCCESS;
  }

  VOID SetFormTitle (IN EFI_STRING_ID FormTitle) {
    mForm->FormTitle = FormTitle;
  }
};

class CIfrFormMap : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_FORM_MAP        *mFormMap;
  EFI_IFR_FORM_MAP_METHOD *mMethodMap;

public:
  CIfrFormMap () : CIfrObj (EFI_IFR_FORM_MAP_OP, (CHAR8 **)NULL, sizeof (EFI_IFR_FORM_MAP), TRUE),
                   CIfrOpHeader (EFI_IFR_FORM_MAP_OP, &(GetObjBinAddr<EFI_IFR_FORM_MAP>())->Header), mFormMap(GetObjBinAddr<EFI_IFR_FORM_MAP>()) {
    mFormMap->FormId = 0;
    mMethodMap       = (EFI_IFR_FORM_MAP_METHOD *) (mFormMap + 1);
  }

  EFI_VFR_RETURN_CODE SetFormId (IN EFI_FORM_ID FormId) {
    if (FormId == 0) {
      //
      // FormId can't be 0.
      //
      return VFR_RETURN_INVALID_PARAMETER;
    }
    if (CIfrFormId::ChekFormIdFree (FormId) == FALSE) {
      return VFR_RETURN_FORMID_REDEFINED;
    }
    mFormMap->FormId = FormId;
    CIfrFormId::MarkFormIdUsed (FormId);
    return VFR_RETURN_SUCCESS;
  }

  VOID SetFormMapMethod (IN EFI_STRING_ID MethodTitle, IN EFI_GUID *MethodGuid) {
    if (ExpendObjBin (sizeof (EFI_IFR_FORM_MAP_METHOD))) {
      IncLength (sizeof (EFI_IFR_FORM_MAP_METHOD));

      mMethodMap->MethodTitle = MethodTitle;
      memmove (&(mMethodMap->MethodIdentifier), MethodGuid, sizeof (EFI_GUID));
      mMethodMap ++;
    }
  }
};

class CIfrVarStore : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_VARSTORE *mVarStore;

public:
  CIfrVarStore () : CIfrObj (EFI_IFR_VARSTORE_OP, (CHAR8 **)NULL, sizeof (EFI_IFR_VARSTORE), TRUE),
                   CIfrOpHeader (EFI_IFR_VARSTORE_OP, &(GetObjBinAddr<EFI_IFR_VARSTORE>())->Header), mVarStore(GetObjBinAddr<EFI_IFR_VARSTORE>()) {
    mVarStore->VarStoreId = EFI_VARSTORE_ID_INVALID;
    mVarStore->Size       = 0;
    memset (&mVarStore->Guid, 0, sizeof (EFI_GUID));
    mVarStore->Name[0]    = '\0';
  }

  VOID SetGuid (IN EFI_GUID *Guid) {
    memmove (&mVarStore->Guid, Guid, sizeof (EFI_GUID));
  }

  VOID SetVarStoreId (IN EFI_VARSTORE_ID VarStoreId) {
    mVarStore->VarStoreId = VarStoreId;
  }

  VOID SetSize (IN UINT16 Size) {
    mVarStore->Size = Size;
  }

  VOID SetName (IN CHAR8 *Name) {
    UINT8 Len;

    if (Name != NULL) {
      Len = (UINT8) strlen (Name);
      if (Len != 0) {
        if (ExpendObjBin (Len) == TRUE) {
          IncLength (Len);
          strcpy ((CHAR8 *)(mVarStore->Name), Name);
        }
      }
    }
  }
};

class CIfrVarStoreEfi : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_VARSTORE_EFI *mVarStoreEfi;

public:
  CIfrVarStoreEfi () : CIfrObj (EFI_IFR_VARSTORE_EFI_OP, (CHAR8 **)NULL, sizeof (EFI_IFR_VARSTORE_EFI), TRUE),
                      CIfrOpHeader (EFI_IFR_VARSTORE_EFI_OP, &(GetObjBinAddr<EFI_IFR_VARSTORE_EFI>())->Header), mVarStoreEfi(GetObjBinAddr<EFI_IFR_VARSTORE_EFI>()) {
    mVarStoreEfi->VarStoreId = EFI_VAROFFSET_INVALID;
    mVarStoreEfi->Size       = 0;
    memset (&mVarStoreEfi->Guid, 0, sizeof (EFI_GUID));
    mVarStoreEfi->Name[0]    = '\0';
  }

  VOID SetGuid (IN EFI_GUID *Guid) {
    memmove (&mVarStoreEfi->Guid, Guid, sizeof (EFI_GUID));
  }

  VOID SetVarStoreId (IN UINT16 VarStoreId) {
    mVarStoreEfi->VarStoreId = VarStoreId;
  }

  VOID SetAttributes (IN UINT32 Attributes) {
    mVarStoreEfi->Attributes = Attributes;
  }
  VOID SetSize (IN UINT16 Size) {
    mVarStoreEfi->Size = Size;
  }

  VOID SetName (IN CHAR8 *Name) {
    UINT8 Len;

    if (Name != NULL) {
      Len = (UINT8) strlen (Name);
      if (Len != 0) {
        if (ExpendObjBin (Len) == TRUE) {
          IncLength (Len);
          strcpy ((CHAR8 *)(mVarStoreEfi->Name), Name);
        }
      }
    }
  }

  VOID SetBinaryLength (IN UINT16 Size) {
    UINT16 Len;

    Len = sizeof (EFI_IFR_VARSTORE_EFI);
    if (Size > Len) {
      ExpendObjBin(Size - Len);
      IncLength(Size - Len);
    } else {
      ShrinkObjBin(Len - Size);
      DecLength(Len - Size);
    }
  }
};

class CIfrVarStoreNameValue : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_VARSTORE_NAME_VALUE *mVarStoreNameValue;

public:
  CIfrVarStoreNameValue () : CIfrObj (EFI_IFR_VARSTORE_NAME_VALUE_OP),
                              CIfrOpHeader (EFI_IFR_VARSTORE_NAME_VALUE_OP, &(GetObjBinAddr<EFI_IFR_VARSTORE_NAME_VALUE>())->Header), mVarStoreNameValue(GetObjBinAddr<EFI_IFR_VARSTORE_NAME_VALUE>()) {
    mVarStoreNameValue->VarStoreId = EFI_VAROFFSET_INVALID;
    memset (&mVarStoreNameValue->Guid, 0, sizeof (EFI_GUID));
  }

  VOID SetGuid (IN EFI_GUID *Guid) {
    memmove (&mVarStoreNameValue->Guid, Guid, sizeof (EFI_GUID));
  }

  VOID SetVarStoreId (IN UINT16 VarStoreId) {
    mVarStoreNameValue->VarStoreId = VarStoreId;
  }
};

class CIfrImage : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_IMAGE *mImage;

public:
  CIfrImage () : CIfrObj (EFI_IFR_IMAGE_OP),
                 CIfrOpHeader (EFI_IFR_IMAGE_OP, &(GetObjBinAddr<EFI_IFR_IMAGE>())->Header), mImage(GetObjBinAddr<EFI_IFR_IMAGE>()) {
    mImage->Id = EFI_IMAGE_ID_INVALID;
  }

  VOID SetImageId (IN EFI_IMAGE_ID ImageId) {
    mImage->Id = ImageId;
  }
};

class CIfrModal : public CIfrObj, public CIfrOpHeader {
public:
  CIfrModal () : CIfrObj (EFI_IFR_MODAL_TAG_OP),
                 CIfrOpHeader (EFI_IFR_MODAL_TAG_OP, &(GetObjBinAddr<EFI_IFR_MODAL_TAG>())->Header) {
  }
};


class CIfrLocked : public CIfrObj, public CIfrOpHeader {
public:
  CIfrLocked () : CIfrObj (EFI_IFR_LOCKED_OP),
                  CIfrOpHeader (EFI_IFR_LOCKED_OP, &(GetObjBinAddr<EFI_IFR_LOCKED>())->Header) {}
};

class CIfrRule : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_RULE *mRule;

public:
  CIfrRule () : CIfrObj (EFI_IFR_RULE_OP),
                CIfrOpHeader (EFI_IFR_RULE_OP, &(GetObjBinAddr<EFI_IFR_RULE>())->Header), mRule(GetObjBinAddr<EFI_IFR_RULE>()) {
    mRule->RuleId = EFI_RULE_ID_INVALID;
  }

  VOID SetRuleId (IN UINT8 RuleId) {
    mRule->RuleId = RuleId;
  }
};

static EFI_IFR_TYPE_VALUE gZeroEfiIfrTypeValue = {0, };

class CIfrDefault : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_DEFAULT *mDefault;

public:
  CIfrDefault (
    IN UINT8              Size,
    IN UINT16             DefaultId = EFI_HII_DEFAULT_CLASS_STANDARD,
    IN UINT8              Type      = EFI_IFR_TYPE_OTHER,
    IN EFI_IFR_TYPE_VALUE Value     = gZeroEfiIfrTypeValue
    ) : CIfrObj (EFI_IFR_DEFAULT_OP, (CHAR8 **)NULL, Size),
        CIfrOpHeader (EFI_IFR_DEFAULT_OP, &(GetObjBinAddr<EFI_IFR_DEFAULT>())->Header, Size), mDefault(GetObjBinAddr<EFI_IFR_DEFAULT>()) {
    mDefault->Type      = Type;
    mDefault->DefaultId = DefaultId;
    memmove (&(mDefault->Value), &Value, Size - OFFSET_OF (EFI_IFR_DEFAULT, Value));
  }

  VOID SetDefaultId (IN UINT16 DefaultId) {
    mDefault->DefaultId = DefaultId;
  }

  VOID SetType (IN UINT8 Type) {
    mDefault->Type = Type;
  }

  VOID SetValue (IN EFI_IFR_TYPE_VALUE Value) {
    memmove (&mDefault->Value, &Value, mDefault->Header.Length - OFFSET_OF (EFI_IFR_DEFAULT, Value));
  }
};

class CIfrDefault2 : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_DEFAULT_2 *mDefault;

public:
  CIfrDefault2 (
    IN UINT16             DefaultId = EFI_HII_DEFAULT_CLASS_STANDARD,
    IN UINT8              Type      = EFI_IFR_TYPE_OTHER
    ) : CIfrObj (EFI_IFR_DEFAULT_OP, (CHAR8 **)NULL, sizeof (EFI_IFR_DEFAULT_2)),
        CIfrOpHeader (EFI_IFR_DEFAULT_OP, &(GetObjBinAddr<EFI_IFR_DEFAULT_2>())->Header, sizeof (EFI_IFR_DEFAULT_2)), mDefault(GetObjBinAddr<EFI_IFR_DEFAULT_2>()) {
    mDefault->Type      = Type;
    mDefault->DefaultId = DefaultId;
  }

  VOID SetDefaultId (IN UINT16 DefaultId) {
    mDefault->DefaultId = DefaultId;
  }

  VOID SetType (IN UINT8 Type) {
    mDefault->Type = Type;
  }
};

class CIfrValue : public CIfrObj, public CIfrOpHeader{
public:
  CIfrValue () : CIfrObj (EFI_IFR_VALUE_OP),
                CIfrOpHeader (EFI_IFR_VALUE_OP, &(GetObjBinAddr<EFI_IFR_VALUE>())->Header) {}

};

class CIfrRead : public CIfrObj, public CIfrOpHeader{
public:
  CIfrRead () : CIfrObj (EFI_IFR_READ_OP),
                CIfrOpHeader (EFI_IFR_READ_OP, &(GetObjBinAddr<EFI_IFR_READ>())->Header) {}

};

class CIfrWrite : public CIfrObj, public CIfrOpHeader{
public:
  CIfrWrite () : CIfrObj (EFI_IFR_WRITE_OP),
                CIfrOpHeader (EFI_IFR_WRITE_OP, &(GetObjBinAddr<EFI_IFR_WRITE>())->Header) {}

};

class CIfrGet : public CIfrObj, public CIfrOpHeader{
private:
  EFI_IFR_GET *mGet;

public:
  CIfrGet (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_GET_OP),
      CIfrOpHeader (EFI_IFR_GET_OP, &(GetObjBinAddr<EFI_IFR_GET>())->Header), mGet(GetObjBinAddr<EFI_IFR_GET>()) {
    SetLineNo (LineNo);
  }

  VOID SetVarInfo (IN EFI_VARSTORE_INFO *Info) {
    mGet->VarStoreId             = Info->mVarStoreId;
    mGet->VarStoreInfo.VarName   = Info->mInfo.mVarName;
    mGet->VarStoreInfo.VarOffset = Info->mInfo.mVarOffset;
    mGet->VarStoreType           = Info->mVarType;
  }
};

class CIfrSet : public CIfrObj, public CIfrOpHeader{
private:
  EFI_IFR_SET *mSet;

public:
  CIfrSet (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_SET_OP),
      CIfrOpHeader (EFI_IFR_SET_OP, &(GetObjBinAddr<EFI_IFR_SET>())->Header), mSet(GetObjBinAddr<EFI_IFR_SET>()) {
    SetLineNo (LineNo);
  }

  VOID SetVarInfo (IN EFI_VARSTORE_INFO *Info) {
    mSet->VarStoreId             = Info->mVarStoreId;
    mSet->VarStoreInfo.VarName   = Info->mInfo.mVarName;
    mSet->VarStoreInfo.VarOffset = Info->mInfo.mVarOffset;
    mSet->VarStoreType           = Info->mVarType;
  }
};

class CIfrSubtitle : public CIfrObj, public CIfrOpHeader, public CIfrStatementHeader {
private:
  EFI_IFR_SUBTITLE   *mSubtitle;

public:
  CIfrSubtitle () : CIfrObj (EFI_IFR_SUBTITLE_OP),
                  CIfrOpHeader (EFI_IFR_SUBTITLE_OP, &(GetObjBinAddr<EFI_IFR_SUBTITLE>())->Header),
  CIfrStatementHeader (&(GetObjBinAddr<EFI_IFR_SUBTITLE>())->Statement), mSubtitle(GetObjBinAddr<EFI_IFR_SUBTITLE>()) {
    mSubtitle->Flags = 0;
  }

  EFI_VFR_RETURN_CODE SetFlags (IN UINT8 LFlags) {
    if (_FLAG_TEST_AND_CLEAR (LFlags, EFI_IFR_FLAGS_HORIZONTAL)) {
      mSubtitle->Flags |= EFI_IFR_FLAGS_HORIZONTAL;
    }

    return _FLAGS_ZERO (LFlags) ? VFR_RETURN_SUCCESS : VFR_RETURN_FLAGS_UNSUPPORTED;
  }
};

class CIfrText : public CIfrObj, public CIfrOpHeader, public CIfrStatementHeader {
private:
  EFI_IFR_TEXT *mText;

public:
  CIfrText () : CIfrObj (EFI_IFR_TEXT_OP),
               CIfrOpHeader (EFI_IFR_TEXT_OP, &(GetObjBinAddr<EFI_IFR_TEXT>())->Header),
               CIfrStatementHeader (&(GetObjBinAddr<EFI_IFR_TEXT>())->Statement), mText(GetObjBinAddr<EFI_IFR_TEXT>()) {
    mText->TextTwo = EFI_STRING_ID_INVALID;
  }

  VOID SetTextTwo (IN EFI_STRING_ID StringId) {
    mText->TextTwo = StringId;
  }
};

class CIfrRef : public CIfrObj, public CIfrOpHeader, public CIfrQuestionHeader {
private:
  EFI_IFR_REF *mRef;

public:
  CIfrRef () : CIfrObj (EFI_IFR_REF_OP),
              CIfrOpHeader (EFI_IFR_REF_OP, &(GetObjBinAddr<EFI_IFR_REF>())->Header),
              CIfrQuestionHeader (&(GetObjBinAddr<EFI_IFR_REF>())->Question), mRef(GetObjBinAddr<EFI_IFR_REF>()) {
    mRef->FormId = 0;
  }

  VOID SetFormId (IN EFI_FORM_ID FormId) {
    mRef->FormId = FormId;
  }
};

class CIfrRef2 : public CIfrObj, public CIfrOpHeader, public CIfrQuestionHeader {
private:
  EFI_IFR_REF2 *mRef2;

public:
  CIfrRef2 () : CIfrObj (EFI_IFR_REF_OP, (CHAR8 **)NULL, sizeof (EFI_IFR_REF2)),
               CIfrOpHeader (EFI_IFR_REF_OP, &(GetObjBinAddr<EFI_IFR_REF2>())->Header, sizeof (EFI_IFR_REF2)),
               CIfrQuestionHeader (&(GetObjBinAddr<EFI_IFR_REF2>())->Question), mRef2(GetObjBinAddr<EFI_IFR_REF2>()) {
    mRef2->FormId     = 0;
    mRef2->QuestionId = EFI_QUESTION_ID_INVALID;
  }

  VOID SetFormId (IN EFI_FORM_ID FormId) {
    mRef2->FormId = FormId;
  }

  VOID SetQuestionId (IN EFI_QUESTION_ID QuestionId) {
    mRef2->QuestionId = QuestionId;
  }
};

class CIfrRef3 : public CIfrObj, public CIfrOpHeader, public CIfrQuestionHeader {
private:
  EFI_IFR_REF3 *mRef3;

public:
  CIfrRef3 () : CIfrObj (EFI_IFR_REF_OP, (CHAR8 **)NULL, sizeof(EFI_IFR_REF3)),
               CIfrOpHeader (EFI_IFR_REF_OP, &(GetObjBinAddr<EFI_IFR_REF3>())->Header, sizeof (EFI_IFR_REF3)),
               CIfrQuestionHeader (&(GetObjBinAddr<EFI_IFR_REF3>())->Question), mRef3(GetObjBinAddr<EFI_IFR_REF3>()) {
    mRef3->FormId     = 0;
    mRef3->QuestionId = EFI_QUESTION_ID_INVALID;
    memset (&mRef3->FormSetId, 0, sizeof (EFI_GUID));
  }

  VOID SetFormId (IN EFI_FORM_ID FormId) {
    mRef3->FormId = FormId;
  }

  VOID SetQuestionId (IN EFI_QUESTION_ID QuestionId) {
    mRef3->QuestionId = QuestionId;
  }

  VOID SetFormSetId (IN EFI_GUID FormSetId) {
    mRef3->FormSetId = FormSetId;
  }
};

class CIfrRef4 : public CIfrObj, public CIfrOpHeader, public CIfrQuestionHeader {
private:
  EFI_IFR_REF4 *mRef4;

public:
  CIfrRef4 () : CIfrObj (EFI_IFR_REF_OP, (CHAR8 **)NULL, sizeof(EFI_IFR_REF4)),
               CIfrOpHeader (EFI_IFR_REF_OP, &(GetObjBinAddr<EFI_IFR_REF4>())->Header, sizeof(EFI_IFR_REF4)),
               CIfrQuestionHeader (&(GetObjBinAddr<EFI_IFR_REF4>())->Question), mRef4(GetObjBinAddr<EFI_IFR_REF4>()) {
    mRef4->FormId     = 0;
    mRef4->QuestionId = EFI_QUESTION_ID_INVALID;
    memset (&mRef4->FormSetId, 0, sizeof (EFI_GUID));
    mRef4->DevicePath = EFI_STRING_ID_INVALID;
  }

  VOID SetFormId (IN EFI_FORM_ID FormId) {
    mRef4->FormId = FormId;
  }

  VOID SetQuestionId (IN EFI_QUESTION_ID QuestionId) {
    mRef4->QuestionId = QuestionId;
  }

  VOID SetFormSetId (IN EFI_GUID FormSetId) {
    mRef4->FormSetId = FormSetId;
  }

  VOID SetDevicePath (IN EFI_STRING_ID DevicePath) {
    mRef4->DevicePath = DevicePath;
  }
};

class CIfrRef5 : public CIfrObj, public CIfrOpHeader, public CIfrQuestionHeader {
public:
  CIfrRef5 () : CIfrObj (EFI_IFR_REF_OP, (CHAR8 **)NULL, sizeof (EFI_IFR_REF5)),
              CIfrOpHeader (EFI_IFR_REF_OP, &(GetObjBinAddr<EFI_IFR_REF5>())->Header, sizeof (EFI_IFR_REF5)),
              CIfrQuestionHeader (&(GetObjBinAddr<EFI_IFR_REF5>())->Question) {
  }
};

class CIfrResetButton : public CIfrObj, public CIfrOpHeader, public CIfrStatementHeader {
private:
  EFI_IFR_RESET_BUTTON *mResetButton;

public:
  CIfrResetButton () : CIfrObj (EFI_IFR_RESET_BUTTON_OP),
                       CIfrOpHeader (EFI_IFR_RESET_BUTTON_OP, &(GetObjBinAddr<EFI_IFR_RESET_BUTTON>())->Header),
  CIfrStatementHeader (&(GetObjBinAddr<EFI_IFR_RESET_BUTTON>())->Statement), mResetButton(GetObjBinAddr<EFI_IFR_RESET_BUTTON>()) {
    mResetButton->DefaultId = EFI_HII_DEFAULT_CLASS_STANDARD;
  }

  VOID SetDefaultId (IN UINT16 DefaultId) {
    mResetButton->DefaultId = DefaultId;
  }
};

class CIfrCheckBox : public CIfrObj, public CIfrOpHeader, public CIfrQuestionHeader  {
private:
  EFI_IFR_CHECKBOX *mCheckBox;

public:
  CIfrCheckBox () : CIfrObj (EFI_IFR_CHECKBOX_OP),
                     CIfrOpHeader (EFI_IFR_CHECKBOX_OP, &(GetObjBinAddr<EFI_IFR_CHECKBOX>())->Header),
                     CIfrQuestionHeader (&(GetObjBinAddr<EFI_IFR_CHECKBOX>())->Question), mCheckBox(GetObjBinAddr<EFI_IFR_CHECKBOX>()) {
    mCheckBox->Flags = 0;
    gCurrentQuestion  = this;
  }

  ~CIfrCheckBox () {
    gCurrentQuestion  = NULL;
  }

  EFI_VFR_RETURN_CODE SetFlags (IN UINT8 HFlags, UINT8 LFlags) {
    EFI_VFR_RETURN_CODE Ret;

    Ret = CIfrQuestionHeader::SetFlags (HFlags);
    if (Ret != VFR_RETURN_SUCCESS) {
      return Ret;
    }

    if (_FLAG_TEST_AND_CLEAR (LFlags, EFI_IFR_CHECKBOX_DEFAULT)) {
      mCheckBox->Flags |= EFI_IFR_CHECKBOX_DEFAULT;
    }

    if (_FLAG_TEST_AND_CLEAR (LFlags, EFI_IFR_CHECKBOX_DEFAULT_MFG)) {
      mCheckBox->Flags |= EFI_IFR_CHECKBOX_DEFAULT_MFG;
    }

    return _FLAGS_ZERO (LFlags) ? VFR_RETURN_SUCCESS : VFR_RETURN_FLAGS_UNSUPPORTED;
  }

  UINT8 GetFlags (VOID) {
    return mCheckBox->Flags;
  }
};

class CIfrAction : public CIfrObj, public CIfrOpHeader, public CIfrQuestionHeader {
private:
  EFI_IFR_ACTION *mAction;

public:
  CIfrAction () : CIfrObj (EFI_IFR_ACTION_OP),
                 CIfrOpHeader (EFI_IFR_ACTION_OP, &(GetObjBinAddr<EFI_IFR_ACTION>())->Header),
                 CIfrQuestionHeader (&(GetObjBinAddr<EFI_IFR_ACTION>())->Question), mAction(GetObjBinAddr<EFI_IFR_ACTION>()) {
    mAction->QuestionConfig = EFI_STRING_ID_INVALID;
  }

  VOID SetQuestionConfig (IN EFI_STRING_ID QuestionConfig) {
    mAction->QuestionConfig = QuestionConfig;
  }
};

class CIfrDate : public CIfrObj, public CIfrOpHeader, public CIfrQuestionHeader {
private:
  EFI_IFR_DATE *mDate;

public:
  CIfrDate () : CIfrObj (EFI_IFR_DATE_OP),
               CIfrOpHeader (EFI_IFR_DATE_OP, &(GetObjBinAddr<EFI_IFR_DATE>())->Header),
               CIfrQuestionHeader (&(GetObjBinAddr<EFI_IFR_DATE>())->Question), mDate(GetObjBinAddr<EFI_IFR_DATE>()) {
    mDate->Flags = 0;
  }

  EFI_VFR_RETURN_CODE SetFlags (IN UINT8 HFlags, IN UINT8 LFlags) {
    EFI_VFR_RETURN_CODE Ret;

    Ret = CIfrQuestionHeader::SetFlags (HFlags);
    if (Ret != VFR_RETURN_SUCCESS) {
      return Ret;
    }

    if (_FLAG_TEST_AND_CLEAR (LFlags, EFI_QF_DATE_YEAR_SUPPRESS)) {
      mDate->Flags |= EFI_QF_DATE_YEAR_SUPPRESS;
    }

    if (_FLAG_TEST_AND_CLEAR (LFlags, EFI_QF_DATE_MONTH_SUPPRESS)) {
      mDate->Flags |= EFI_QF_DATE_MONTH_SUPPRESS;
    }

    if (_FLAG_TEST_AND_CLEAR (LFlags, EFI_QF_DATE_DAY_SUPPRESS)) {
      mDate->Flags |= EFI_QF_DATE_DAY_SUPPRESS;
    }

    if (_FLAG_TEST_AND_CLEAR (LFlags, QF_DATE_STORAGE_NORMAL)) {
      mDate->Flags |= QF_DATE_STORAGE_NORMAL;
    } else if (_FLAG_TEST_AND_CLEAR (LFlags, QF_DATE_STORAGE_TIME)) {
      mDate->Flags |= QF_DATE_STORAGE_TIME;
    } else if (_FLAG_TEST_AND_CLEAR (LFlags, QF_DATE_STORAGE_WAKEUP)) {
      mDate->Flags |= QF_DATE_STORAGE_WAKEUP;
    }

    return _FLAGS_ZERO (LFlags) ? VFR_RETURN_SUCCESS : VFR_RETURN_FLAGS_UNSUPPORTED;
  }
};

class CIfrNumeric : public CIfrObj, public CIfrOpHeader, public CIfrQuestionHeader, public CIfrMinMaxStepData {
private:
  EFI_IFR_NUMERIC *mNumeric;

public:
  CIfrNumeric () : CIfrObj (EFI_IFR_NUMERIC_OP, (CHAR8 **)NULL, sizeof (EFI_IFR_NUMERIC), TRUE),
                   CIfrOpHeader (EFI_IFR_NUMERIC_OP, &(GetObjBinAddr<EFI_IFR_NUMERIC>())->Header),
                   CIfrQuestionHeader (&(GetObjBinAddr<EFI_IFR_NUMERIC>())->Question),
                   CIfrMinMaxStepData (&(GetObjBinAddr<EFI_IFR_NUMERIC>())->data, TRUE), mNumeric(GetObjBinAddr<EFI_IFR_NUMERIC>()) {
    mNumeric->Flags  = EFI_IFR_NUMERIC_SIZE_1 | EFI_IFR_DISPLAY_UINT_DEC;
    gCurrentQuestion   = this;
    gCurrentMinMaxData = this;
  }

  ~CIfrNumeric () {
    gCurrentQuestion   = NULL;
    gCurrentMinMaxData = NULL;
  }

  VOID ShrinkBinSize (IN UINT16 Size) {
    //
    // Update the buffer size which is truly be used later.
    //
    ShrinkObjBin(Size);
    DecLength(Size);

    //
    // Allocate buffer in gCFormPkg.
    //
    _EMIT_PENDING_OBJ();

    //
    // Update the buffer pointer used by other class.
    //
    mNumeric = GetObjBinAddr<EFI_IFR_NUMERIC>();
    UpdateHeader (&mNumeric->Header);
    UpdateCIfrQuestionHeader(&mNumeric->Question);
    UpdateCIfrMinMaxStepData(&mNumeric->data);
  }

  EFI_VFR_RETURN_CODE SetFlags (IN UINT8 HFlags, IN UINT8 LFlags, BOOLEAN DisplaySettingsSpecified = FALSE) {
    EFI_VFR_RETURN_CODE Ret;

    Ret = CIfrQuestionHeader::SetFlags (HFlags);
    if (Ret != VFR_RETURN_SUCCESS) {
      return Ret;
    }

    if (DisplaySettingsSpecified == FALSE) {
      mNumeric->Flags = LFlags | EFI_IFR_DISPLAY_UINT_DEC;
    } else {
      mNumeric->Flags = LFlags;
    }
    return VFR_RETURN_SUCCESS;
  }

  EFI_VFR_RETURN_CODE SetFlagsForBitField (IN UINT8 HFlags, IN UINT8 LFlags, BOOLEAN DisplaySettingsSpecified = FALSE) {
    EFI_VFR_RETURN_CODE Ret;

    Ret = CIfrQuestionHeader::SetFlags (HFlags);
    if (Ret != VFR_RETURN_SUCCESS) {
      return Ret;
    }

    if (DisplaySettingsSpecified == FALSE) {
      mNumeric->Flags = LFlags | EDKII_IFR_DISPLAY_UINT_DEC_BIT;
    } else {
      mNumeric->Flags = LFlags;
    }
    return VFR_RETURN_SUCCESS;
  }

  UINT8 GetNumericFlags () {
    return mNumeric->Flags;
  }
};

class CIfrOneOf : public CIfrObj, public CIfrOpHeader, public CIfrQuestionHeader, public CIfrMinMaxStepData {
private:
  EFI_IFR_ONE_OF *mOneOf;

public:
  CIfrOneOf () : CIfrObj (EFI_IFR_ONE_OF_OP, (CHAR8 **)NULL, sizeof (EFI_IFR_ONE_OF), TRUE),
                 CIfrOpHeader (EFI_IFR_ONE_OF_OP, &(GetObjBinAddr<EFI_IFR_ONE_OF>())->Header),
                 CIfrQuestionHeader (&(GetObjBinAddr<EFI_IFR_ONE_OF>())->Question),
                 CIfrMinMaxStepData (&(GetObjBinAddr<EFI_IFR_ONE_OF>())->data), mOneOf(GetObjBinAddr<EFI_IFR_ONE_OF>()) {
    mOneOf->Flags    = 0;
    gCurrentQuestion   = this;
    gCurrentMinMaxData = this;
  }

  ~CIfrOneOf () {
    gCurrentQuestion   = NULL;
    gCurrentMinMaxData = NULL;
  }

  EFI_VFR_RETURN_CODE SetFlags (IN UINT8 HFlags, IN UINT8 LFlags) {
    EFI_VFR_RETURN_CODE Ret;

    Ret = CIfrQuestionHeader::SetFlags (HFlags);
    if (Ret != VFR_RETURN_SUCCESS) {
      return Ret;
    }

    if (LFlags & EFI_IFR_DISPLAY) {
      mOneOf->Flags = LFlags;
    } else {
      mOneOf->Flags = LFlags | EFI_IFR_DISPLAY_UINT_DEC;
    }
    return VFR_RETURN_SUCCESS;
  }

  EFI_VFR_RETURN_CODE SetFlagsForBitField (IN UINT8 HFlags, IN UINT8 LFlags) {
    EFI_VFR_RETURN_CODE Ret;

    Ret = CIfrQuestionHeader::SetFlags (HFlags);
    if (Ret != VFR_RETURN_SUCCESS) {
      return Ret;
    }

    if (LFlags & EDKII_IFR_DISPLAY_BIT) {
      mOneOf->Flags = LFlags;
    } else {
      mOneOf->Flags = LFlags | EDKII_IFR_DISPLAY_UINT_DEC_BIT;
    }
    return VFR_RETURN_SUCCESS;
  }

  VOID ShrinkBinSize (IN UINT16 Size) {
    //
    // Update the buffer size which is truly be used later.
    //
    ShrinkObjBin(Size);
    DecLength(Size);

    //
    // Allocate buffer in gCFormPkg.
    //
    _EMIT_PENDING_OBJ();

    //
    // Update the buffer pointer used by other class.
    //
    mOneOf = GetObjBinAddr<EFI_IFR_ONE_OF>();
    UpdateHeader (&mOneOf->Header);
    UpdateCIfrQuestionHeader(&mOneOf->Question);
    UpdateCIfrMinMaxStepData(&mOneOf->data);
  }
};

class CIfrString : public CIfrObj, public CIfrOpHeader, public CIfrQuestionHeader {
private:
  EFI_IFR_STRING *mString;

public:
  CIfrString () : CIfrObj (EFI_IFR_STRING_OP),
                 CIfrOpHeader (EFI_IFR_STRING_OP, &(GetObjBinAddr<EFI_IFR_STRING>())->Header),
                 CIfrQuestionHeader (&(GetObjBinAddr<EFI_IFR_STRING>())->Question), mString(GetObjBinAddr<EFI_IFR_STRING>()) {
    mString->Flags   = 0;
    mString->MinSize = 0;
    mString->MaxSize = 0;
    gCurrentQuestion = this;
  }

  ~CIfrString () {
    gCurrentQuestion = NULL;
  }

  EFI_VFR_RETURN_CODE SetFlags (IN UINT8 HFlags, IN UINT8 LFlags) {
    EFI_VFR_RETURN_CODE Ret;

    Ret = CIfrQuestionHeader::SetFlags (HFlags);
    if (Ret != VFR_RETURN_SUCCESS) {
      return Ret;
    }

    if (_FLAG_TEST_AND_CLEAR (LFlags, EFI_IFR_STRING_MULTI_LINE)) {
      mString->Flags |= EFI_IFR_STRING_MULTI_LINE;
    }

    return _FLAGS_ZERO (LFlags) ? VFR_RETURN_SUCCESS : VFR_RETURN_FLAGS_UNSUPPORTED;
  }

  VOID SetMinSize (IN UINT8 Flags) {
    mString->MinSize = Flags;
  }

  VOID SetMaxSize (IN UINT8 MaxSize) {
    mString->MaxSize = MaxSize;
  }
};

class CIfrPassword : public CIfrObj, public CIfrOpHeader, public CIfrQuestionHeader {
private:
  EFI_IFR_PASSWORD *mPassword;

public:
  CIfrPassword () : CIfrObj (EFI_IFR_PASSWORD_OP),
                    CIfrOpHeader (EFI_IFR_PASSWORD_OP, &(GetObjBinAddr<EFI_IFR_PASSWORD>())->Header),
                    CIfrQuestionHeader (&(GetObjBinAddr<EFI_IFR_PASSWORD>())->Question), mPassword(GetObjBinAddr<EFI_IFR_PASSWORD>()) {
    mPassword->MinSize = 0;
    mPassword->MaxSize = 0;
    gCurrentQuestion   = this;
  }

  ~CIfrPassword () {
    gCurrentQuestion = NULL;
  }

  VOID SetMinSize (IN UINT16 MinSize) {
    mPassword->MinSize = MinSize;
  }

  VOID SetMaxSize (IN UINT16 MaxSize) {
    mPassword->MaxSize = MaxSize;
  }
};

class CIfrOrderedList : public CIfrObj, public CIfrOpHeader, public CIfrQuestionHeader {
private:
  EFI_IFR_ORDERED_LIST *mOrderedList;

public:
  CIfrOrderedList () : CIfrObj (EFI_IFR_ORDERED_LIST_OP),
                      CIfrOpHeader (EFI_IFR_ORDERED_LIST_OP, &(GetObjBinAddr<EFI_IFR_ORDERED_LIST>())->Header),
                      CIfrQuestionHeader (&(GetObjBinAddr<EFI_IFR_ORDERED_LIST>())->Question), mOrderedList(GetObjBinAddr<EFI_IFR_ORDERED_LIST>()) {
    mOrderedList->MaxContainers = 0;
    mOrderedList->Flags         = 0;
    gCurrentQuestion            = this;
  }

  ~CIfrOrderedList () {
    gCurrentQuestion = NULL;
  }

  VOID SetMaxContainers (IN UINT8 MaxContainers) {
    mOrderedList->MaxContainers = MaxContainers;
  }

  EFI_VFR_RETURN_CODE SetFlags (IN UINT8 HFlags, IN UINT8 LFlags) {
    EFI_VFR_RETURN_CODE Ret;

    Ret = CIfrQuestionHeader::SetFlags (HFlags);
    if (Ret != VFR_RETURN_SUCCESS) {
      return Ret;
    }

    if (_FLAG_TEST_AND_CLEAR (LFlags, EFI_IFR_UNIQUE_SET)) {
      mOrderedList->Flags |= EFI_IFR_UNIQUE_SET;
    }

    if (_FLAG_TEST_AND_CLEAR (LFlags, EFI_IFR_NO_EMPTY_SET)) {
      mOrderedList->Flags |= EFI_IFR_NO_EMPTY_SET;
    }

    return _FLAGS_ZERO (LFlags) ? VFR_RETURN_SUCCESS : VFR_RETURN_FLAGS_UNSUPPORTED;
  }
};

class CIfrTime : public CIfrObj, public CIfrOpHeader, public CIfrQuestionHeader {
private:
  EFI_IFR_TIME *mTime;

public:
  CIfrTime () : CIfrObj (EFI_IFR_TIME_OP),
                CIfrOpHeader (EFI_IFR_TIME_OP, &(GetObjBinAddr<EFI_IFR_TIME>())->Header),
                CIfrQuestionHeader (&(GetObjBinAddr<EFI_IFR_TIME>())->Question), mTime(GetObjBinAddr<EFI_IFR_TIME>()) {
    mTime->Flags = 0;
  }

  EFI_VFR_RETURN_CODE SetFlags (IN UINT8 HFlags, IN UINT8 LFlags) {
    EFI_VFR_RETURN_CODE Ret;

    Ret = CIfrQuestionHeader::SetFlags (HFlags);
    if (Ret != VFR_RETURN_SUCCESS) {
      return Ret;
    }

    if (_FLAG_TEST_AND_CLEAR (LFlags, QF_TIME_HOUR_SUPPRESS)) {
      mTime->Flags |= QF_TIME_HOUR_SUPPRESS;
    }

    if (_FLAG_TEST_AND_CLEAR (LFlags, QF_TIME_MINUTE_SUPPRESS)) {
      mTime->Flags |= QF_TIME_MINUTE_SUPPRESS;
    }

    if (_FLAG_TEST_AND_CLEAR (LFlags, QF_TIME_SECOND_SUPPRESS)) {
      mTime->Flags |= QF_TIME_SECOND_SUPPRESS;
    }

    if (_FLAG_TEST_AND_CLEAR (LFlags, QF_TIME_STORAGE_NORMAL)) {
      mTime->Flags |= QF_TIME_STORAGE_NORMAL;
    } else if (_FLAG_TEST_AND_CLEAR (LFlags, QF_TIME_STORAGE_TIME)) {
      mTime->Flags |= QF_TIME_STORAGE_TIME;
    } else if (_FLAG_TEST_AND_CLEAR (LFlags, QF_TIME_STORAGE_WAKEUP)) {
      mTime->Flags |= QF_TIME_STORAGE_WAKEUP;
    }

    return _FLAGS_ZERO (LFlags) ? VFR_RETURN_SUCCESS : VFR_RETURN_FLAGS_UNSUPPORTED;
  }
};

class CIfrDisableIf : public CIfrObj, public CIfrOpHeader {
public:
  CIfrDisableIf () : CIfrObj (EFI_IFR_DISABLE_IF_OP),
                   CIfrOpHeader (EFI_IFR_DISABLE_IF_OP, &(GetObjBinAddr<EFI_IFR_DISABLE_IF>())->Header) {}
};

class CIfrSuppressIf : public CIfrObj, public CIfrOpHeader {
public:
  CIfrSuppressIf () : CIfrObj (EFI_IFR_SUPPRESS_IF_OP),
                     CIfrOpHeader (EFI_IFR_SUPPRESS_IF_OP, &(GetObjBinAddr<EFI_IFR_SUPPRESS_IF>())->Header) {}
};

class CIfrGrayOutIf : public CIfrObj, public CIfrOpHeader {
public:
  CIfrGrayOutIf () : CIfrObj (EFI_IFR_GRAY_OUT_IF_OP),
                    CIfrOpHeader (EFI_IFR_GRAY_OUT_IF_OP, &(GetObjBinAddr<EFI_IFR_GRAY_OUT_IF>())->Header) {}
};

class CIfrInconsistentIf : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_INCONSISTENT_IF *mInconsistentIf;

public:
  CIfrInconsistentIf () : CIfrObj (EFI_IFR_INCONSISTENT_IF_OP),
                        CIfrOpHeader (EFI_IFR_INCONSISTENT_IF_OP, &(GetObjBinAddr<EFI_IFR_INCONSISTENT_IF>())->Header), mInconsistentIf(GetObjBinAddr<EFI_IFR_INCONSISTENT_IF>()) {
    mInconsistentIf->Error = EFI_STRING_ID_INVALID;
  }

  VOID SetError (IN EFI_STRING_ID Error) {
    mInconsistentIf->Error = Error;
  }
};

class CIfrWarningIf : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_WARNING_IF *mWarningIf;

public:
  CIfrWarningIf () : CIfrObj (EFI_IFR_WARNING_IF_OP),
                        CIfrOpHeader (EFI_IFR_WARNING_IF_OP, &(GetObjBinAddr<EFI_IFR_WARNING_IF>())->Header), mWarningIf(GetObjBinAddr<EFI_IFR_WARNING_IF>()) {
    mWarningIf->Warning = EFI_STRING_ID_INVALID;
    mWarningIf->TimeOut = 0;
  }

  VOID SetWarning (IN EFI_STRING_ID Warning) {
    mWarningIf->Warning = Warning;
  }

  VOID SetTimeOut (IN UINT8 TimeOut) {
    mWarningIf->TimeOut = TimeOut;
  }
};

class CIfrNoSubmitIf : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_NO_SUBMIT_IF *mNoSubmitIf;

public:
  CIfrNoSubmitIf () : CIfrObj (EFI_IFR_NO_SUBMIT_IF_OP),
                     CIfrOpHeader (EFI_IFR_NO_SUBMIT_IF_OP, &(GetObjBinAddr<EFI_IFR_NO_SUBMIT_IF>())->Header), mNoSubmitIf(GetObjBinAddr<EFI_IFR_NO_SUBMIT_IF>()) {
    mNoSubmitIf->Error = EFI_STRING_ID_INVALID;
  }

  VOID SetError (IN EFI_STRING_ID Error) {
    mNoSubmitIf->Error = Error;
  }
};

class CIfrRefresh : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_REFRESH *mRefresh;

public:
  CIfrRefresh () : CIfrObj (EFI_IFR_REFRESH_OP),
                  CIfrOpHeader (EFI_IFR_REFRESH_OP, &(GetObjBinAddr<EFI_IFR_REFRESH>())->Header), mRefresh(GetObjBinAddr<EFI_IFR_REFRESH>()) {
    mRefresh->RefreshInterval = 0;
  }

  VOID SetRefreshInterval (IN UINT8 RefreshInterval) {
    mRefresh->RefreshInterval = RefreshInterval;
  }
};

class CIfrRefreshId : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_REFRESH_ID *mRefreshId;

public:
  CIfrRefreshId () : CIfrObj (EFI_IFR_REFRESH_ID_OP),
      CIfrOpHeader (EFI_IFR_REFRESH_ID_OP, &(GetObjBinAddr<EFI_IFR_REFRESH_ID>())->Header), mRefreshId(GetObjBinAddr<EFI_IFR_REFRESH_ID>()) {
    memset (&mRefreshId->RefreshEventGroupId, 0, sizeof (EFI_GUID));
  }

  VOID SetRefreshEventGroutId (IN EFI_GUID *RefreshEventGroupId) {
    memmove (&mRefreshId->RefreshEventGroupId, RefreshEventGroupId, sizeof (EFI_GUID));
  }
};

class CIfrVarStoreDevice : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_VARSTORE_DEVICE *mVarStoreDevice;

public:
  CIfrVarStoreDevice () : CIfrObj (EFI_IFR_VARSTORE_DEVICE_OP),
                          CIfrOpHeader (EFI_IFR_VARSTORE_DEVICE_OP, &(GetObjBinAddr<EFI_IFR_VARSTORE_DEVICE>())->Header), mVarStoreDevice(GetObjBinAddr<EFI_IFR_VARSTORE_DEVICE>()) {
    mVarStoreDevice->DevicePath = EFI_STRING_ID_INVALID;
  }

  VOID SetDevicePath (IN EFI_STRING_ID DevicePath) {
    mVarStoreDevice->DevicePath = DevicePath;
  }
};

class CIfrOneOfOption : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_ONE_OF_OPTION *mOneOfOption;

public:
  CIfrOneOfOption (UINT8 Size) : CIfrObj (EFI_IFR_ONE_OF_OPTION_OP, (CHAR8 **)NULL, Size),
                       CIfrOpHeader (EFI_IFR_ONE_OF_OPTION_OP, &(GetObjBinAddr<EFI_IFR_ONE_OF_OPTION>())->Header, Size), mOneOfOption(GetObjBinAddr<EFI_IFR_ONE_OF_OPTION>()) {
    mOneOfOption->Flags  = 0;
    mOneOfOption->Option = EFI_STRING_ID_INVALID;
    mOneOfOption->Type   = EFI_IFR_TYPE_OTHER;
    memset (&mOneOfOption->Value, 0, Size - OFFSET_OF (EFI_IFR_ONE_OF_OPTION, Value));
  }

  VOID SetOption (IN EFI_STRING_ID Option) {
    mOneOfOption->Option = Option;
  }

  EFI_VFR_RETURN_CODE SetFlags (IN UINT8 LFlags) {
    mOneOfOption->Flags = 0;
    if (_FLAG_TEST_AND_CLEAR (LFlags, EFI_IFR_OPTION_DEFAULT)) {
      mOneOfOption->Flags |= EFI_IFR_OPTION_DEFAULT;
    }

    if (_FLAG_TEST_AND_CLEAR (LFlags, EFI_IFR_OPTION_DEFAULT_MFG)) {
      mOneOfOption->Flags |= EFI_IFR_OPTION_DEFAULT_MFG;
    }

    if (_IS_EQUAL (LFlags, EFI_IFR_TYPE_NUM_SIZE_8)) {
      _FLAG_CLEAR (LFlags, EFI_IFR_TYPE_NUM_SIZE_8);
      mOneOfOption->Flags |= EFI_IFR_TYPE_NUM_SIZE_8;
    } else if (_IS_EQUAL (LFlags, EFI_IFR_TYPE_NUM_SIZE_16)) {
      _FLAG_CLEAR (LFlags, EFI_IFR_TYPE_NUM_SIZE_16);
      mOneOfOption->Flags |= EFI_IFR_TYPE_NUM_SIZE_16;
    } else if (_IS_EQUAL (LFlags, EFI_IFR_TYPE_NUM_SIZE_32)) {
      _FLAG_CLEAR (LFlags, EFI_IFR_TYPE_NUM_SIZE_32);
      mOneOfOption->Flags |= EFI_IFR_TYPE_NUM_SIZE_32;
    } else if (_IS_EQUAL (LFlags, EFI_IFR_TYPE_NUM_SIZE_64)) {
      _FLAG_CLEAR (LFlags, EFI_IFR_TYPE_NUM_SIZE_64);
      mOneOfOption->Flags |= EFI_IFR_TYPE_NUM_SIZE_64;
    } else if (_IS_EQUAL (LFlags, EFI_IFR_TYPE_BOOLEAN)) {
      _FLAG_CLEAR (LFlags, EFI_IFR_TYPE_BOOLEAN);
      mOneOfOption->Flags |= EFI_IFR_TYPE_BOOLEAN;
    } else if (_IS_EQUAL (LFlags, EFI_IFR_TYPE_TIME)) {
      _FLAG_CLEAR (LFlags, EFI_IFR_TYPE_TIME);
      mOneOfOption->Flags |= EFI_IFR_TYPE_TIME;
    } else if (_IS_EQUAL (LFlags, EFI_IFR_TYPE_DATE)) {
      _FLAG_CLEAR (LFlags, EFI_IFR_TYPE_DATE);
      mOneOfOption->Flags |= EFI_IFR_TYPE_DATE;
    } else if (_IS_EQUAL (LFlags, EFI_IFR_TYPE_STRING)) {
      _FLAG_CLEAR (LFlags, EFI_IFR_TYPE_STRING);
      mOneOfOption->Flags |= EFI_IFR_TYPE_STRING;
    } else if (_IS_EQUAL (LFlags, EFI_IFR_TYPE_OTHER)) {
      _FLAG_CLEAR (LFlags, EFI_IFR_TYPE_OTHER);
      mOneOfOption->Flags |= EFI_IFR_TYPE_OTHER;
    }

    return _FLAGS_ZERO (LFlags) ? VFR_RETURN_SUCCESS : VFR_RETURN_FLAGS_UNSUPPORTED;
  }

  VOID SetType (IN UINT8 Type) {
    mOneOfOption->Type = Type;
  }

  VOID SetValue (IN EFI_IFR_TYPE_VALUE Value) {
    memmove (&mOneOfOption->Value, &Value, mOneOfOption->Header.Length - OFFSET_OF (EFI_IFR_ONE_OF_OPTION, Value));
  }

  UINT8 GetFlags (VOID) {
    return mOneOfOption->Flags;
  }
};

static EFI_GUID IfrTianoGuid     = EFI_IFR_TIANO_GUID;
static EFI_GUID IfrFrameworkGuid = EFI_IFR_FRAMEWORK_GUID;

class CIfrClass : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_GUID_CLASS *mClass;

public:
  CIfrClass () : CIfrObj (EFI_IFR_GUID_OP, (CHAR8 **)NULL, sizeof (EFI_IFR_GUID_CLASS)),
                CIfrOpHeader (EFI_IFR_GUID_OP, &(GetObjBinAddr<EFI_IFR_GUID_CLASS>())->Header, sizeof (EFI_IFR_GUID_CLASS)), mClass(GetObjBinAddr<EFI_IFR_GUID_CLASS>()) {
    mClass->ExtendOpCode = EFI_IFR_EXTEND_OP_CLASS;
    mClass->Guid         = IfrTianoGuid;
    mClass->Class        = EFI_NON_DEVICE_CLASS;
  }

  VOID SetClass (IN UINT16 Class) {
    mClass->Class        = Class;
  }
};

class CIfrSubClass : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_GUID_SUBCLASS *mSubClass;

public:
  CIfrSubClass () : CIfrObj (EFI_IFR_GUID_OP, (CHAR8 **)NULL, sizeof (EFI_IFR_GUID_SUBCLASS)),
                    CIfrOpHeader (EFI_IFR_GUID_OP, &(GetObjBinAddr<EFI_IFR_GUID_SUBCLASS>())->Header, sizeof (EFI_IFR_GUID_SUBCLASS)), mSubClass(GetObjBinAddr<EFI_IFR_GUID_SUBCLASS>()) {
    mSubClass->ExtendOpCode = EFI_IFR_EXTEND_OP_SUBCLASS;
    mSubClass->Guid         = IfrTianoGuid;
    mSubClass->SubClass     = EFI_SETUP_APPLICATION_SUBCLASS;
  }

  VOID SetSubClass (IN UINT16 SubClass) {
    mSubClass->SubClass = SubClass;
  }
};

class CIfrLabel : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_GUID_LABEL *mLabel;

public:
  CIfrLabel () : CIfrObj (EFI_IFR_GUID_OP, (CHAR8 **)NULL, sizeof (EFI_IFR_GUID_LABEL)),
                CIfrOpHeader (EFI_IFR_GUID_OP, &(GetObjBinAddr<EFI_IFR_GUID_LABEL>())->Header, sizeof (EFI_IFR_GUID_LABEL)), mLabel(GetObjBinAddr<EFI_IFR_GUID_LABEL>()) {
    mLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
    mLabel->Guid         = IfrTianoGuid;
  }

  VOID SetNumber (IN UINT16 Number) {
    mLabel->Number = Number;
  }
};

class CIfrBanner : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_GUID_BANNER *mBanner;

public:
  CIfrBanner () : CIfrObj (EFI_IFR_GUID_OP, (CHAR8 **)NULL, sizeof (EFI_IFR_GUID_BANNER)),
                  CIfrOpHeader (EFI_IFR_GUID_OP, &(GetObjBinAddr<EFI_IFR_GUID_BANNER>())->Header, sizeof (EFI_IFR_GUID_BANNER)), mBanner(GetObjBinAddr<EFI_IFR_GUID_BANNER>()) {
    mBanner->ExtendOpCode = EFI_IFR_EXTEND_OP_BANNER;
    mBanner->Guid         = IfrTianoGuid;
  }

  VOID SetTitle (IN EFI_STRING_ID StringId) {
    mBanner->Title = StringId;
  }

  VOID SetLine (IN UINT16 Line) {
    mBanner->LineNumber = Line;
  }

  VOID SetAlign (IN UINT8 Align) {
    mBanner->Alignment = Align;
  }
};

class CIfrOptionKey : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_GUID_OPTIONKEY *mOptionKey;

public:
  CIfrOptionKey (
    IN EFI_QUESTION_ID QuestionId,
    IN EFI_IFR_TYPE_VALUE &OptionValue,
    IN EFI_QUESTION_ID KeyValue
  ) : CIfrObj (EFI_IFR_GUID_OP, (CHAR8 **)NULL, sizeof (EFI_IFR_GUID_OPTIONKEY)),
      CIfrOpHeader (EFI_IFR_GUID_OP, &(GetObjBinAddr<EFI_IFR_GUID_OPTIONKEY>())->Header, sizeof (EFI_IFR_GUID_OPTIONKEY)), mOptionKey(GetObjBinAddr<EFI_IFR_GUID_OPTIONKEY>()) {
    mOptionKey->ExtendOpCode = EFI_IFR_EXTEND_OP_OPTIONKEY;
    mOptionKey->Guid         = IfrFrameworkGuid;
    mOptionKey->QuestionId   = QuestionId;
    mOptionKey->OptionValue  = OptionValue;
    mOptionKey->KeyValue     = KeyValue;
  }
};

class CIfrVarEqName : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_GUID_VAREQNAME *mVarEqName;

public:
  CIfrVarEqName (
    IN EFI_QUESTION_ID QuestionId,
    IN EFI_STRING_ID   NameId
  ) : CIfrObj (EFI_IFR_GUID_OP, (CHAR8 **)NULL, sizeof (EFI_IFR_GUID_VAREQNAME)),
      CIfrOpHeader (EFI_IFR_GUID_OP, &(GetObjBinAddr<EFI_IFR_GUID_VAREQNAME>())->Header, sizeof (EFI_IFR_GUID_VAREQNAME)), mVarEqName(GetObjBinAddr<EFI_IFR_GUID_VAREQNAME>()) {
    mVarEqName->ExtendOpCode = EFI_IFR_EXTEND_OP_VAREQNAME;
    mVarEqName->Guid         = IfrFrameworkGuid;
    mVarEqName->QuestionId   = QuestionId;
    mVarEqName->NameId       = NameId;
  }
};

class CIfrTimeout : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_GUID_TIMEOUT *mTimeout;

public:
  CIfrTimeout (IN UINT16 Timeout = 0) : CIfrObj (EFI_IFR_GUID_OP, (CHAR8 **)NULL, sizeof (EFI_IFR_GUID_TIMEOUT)),
                                        CIfrOpHeader (EFI_IFR_GUID_OP, &(GetObjBinAddr<EFI_IFR_GUID_TIMEOUT>())->Header, sizeof (EFI_IFR_GUID_TIMEOUT)), mTimeout(GetObjBinAddr<EFI_IFR_GUID_TIMEOUT>()) {
    mTimeout->ExtendOpCode = EFI_IFR_EXTEND_OP_TIMEOUT;
    mTimeout->Guid         = IfrTianoGuid;
    mTimeout->TimeOut      = Timeout;
  }

  VOID SetTimeout (IN UINT16 Timeout) {
    mTimeout->TimeOut = Timeout;
  }
};

class CIfrGuid : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_GUID *mGuid;

public:
  CIfrGuid (UINT8 Size) : CIfrObj (EFI_IFR_GUID_OP, (CHAR8 **)NULL, sizeof (EFI_IFR_GUID)+Size),
                  CIfrOpHeader (EFI_IFR_GUID_OP, &(GetObjBinAddr<EFI_IFR_GUID>())->Header, sizeof (EFI_IFR_GUID)+Size), mGuid(GetObjBinAddr<EFI_IFR_GUID>()) {
    memset (&mGuid->Guid, 0, sizeof (EFI_GUID));
  }

  VOID SetGuid (IN EFI_GUID *Guid) {
    memmove (&mGuid->Guid, Guid, sizeof (EFI_GUID));
  }

  VOID SetData (IN UINT8* DataBuff, IN UINT8 Size) {
    memmove ((UINT8 *)mGuid + sizeof (EFI_IFR_GUID), DataBuff, Size);
  }
};

class CIfrDup : public CIfrObj, public CIfrOpHeader {
public:
  CIfrDup (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_DUP_OP),
      CIfrOpHeader (EFI_IFR_DUP_OP, &(GetObjBinAddr<EFI_IFR_DUP>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrEqIdId : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_EQ_ID_ID   *mEqIdId;

public:
  CIfrEqIdId (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_EQ_ID_ID_OP),
                 CIfrOpHeader (EFI_IFR_EQ_ID_ID_OP, &(GetObjBinAddr<EFI_IFR_EQ_ID_ID>())->Header), mEqIdId(GetObjBinAddr<EFI_IFR_EQ_ID_ID>()) {
    SetLineNo (LineNo);
    mEqIdId->QuestionId1 = EFI_QUESTION_ID_INVALID;
    mEqIdId->QuestionId2 = EFI_QUESTION_ID_INVALID;
  }

  VOID SetQuestionId1 (
  IN EFI_QUESTION_ID QuestionId,
  IN CHAR8            *VarIdStr,
  IN UINT32          LineNo
  ) {
    if (QuestionId != EFI_QUESTION_ID_INVALID) {
      mEqIdId->QuestionId1 = QuestionId;
    } else {
      gCFormPkg.AssignPending (VarIdStr, (VOID *)(&mEqIdId->QuestionId1), sizeof (EFI_QUESTION_ID), LineNo, NO_QST_REFED);
    }
  }

  VOID SetQuestionId2 (
  IN EFI_QUESTION_ID QuestionId,
  IN CHAR8            *VarIdStr,
  IN UINT32          LineNo
  ) {
    if (QuestionId != EFI_QUESTION_ID_INVALID) {
      mEqIdId->QuestionId2 = QuestionId;
    } else {
      gCFormPkg.AssignPending (VarIdStr, (VOID *)(&mEqIdId->QuestionId2), sizeof (EFI_QUESTION_ID), LineNo, NO_QST_REFED);
    }
  }
};

class CIfrEqIdVal : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_EQ_ID_VAL *mEqIdVal;

public:
  CIfrEqIdVal (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_EQ_ID_VAL_OP),
      CIfrOpHeader (EFI_IFR_EQ_ID_VAL_OP, &(GetObjBinAddr<EFI_IFR_EQ_ID_VAL>())->Header), mEqIdVal(GetObjBinAddr<EFI_IFR_EQ_ID_VAL>()) {
    SetLineNo (LineNo);
    mEqIdVal->QuestionId = EFI_QUESTION_ID_INVALID;
  }

  VOID SetQuestionId (
  IN EFI_QUESTION_ID QuestionId,
  IN CHAR8           *VarIdStr,
  IN UINT32          LineNo
  ) {
    if (QuestionId != EFI_QUESTION_ID_INVALID) {
      mEqIdVal->QuestionId = QuestionId;
    } else {
      gCFormPkg.AssignPending (VarIdStr, (VOID *)(&mEqIdVal->QuestionId), sizeof (EFI_QUESTION_ID), LineNo, NO_QST_REFED);
    }
  }

  VOID SetValue (IN UINT16 Value) {
    mEqIdVal->Value = Value;
  }
};

class CIfrEqIdList : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_EQ_ID_VAL_LIST *mEqIdVList;

public:
  CIfrEqIdList (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_EQ_ID_VAL_LIST_OP, (CHAR8 **)NULL, sizeof (EFI_IFR_EQ_ID_VAL_LIST), TRUE),
                   CIfrOpHeader (EFI_IFR_EQ_ID_VAL_LIST_OP, &(GetObjBinAddr<EFI_IFR_EQ_ID_VAL_LIST>())->Header), mEqIdVList(GetObjBinAddr<EFI_IFR_EQ_ID_VAL_LIST>()) {
    SetLineNo (LineNo);
    mEqIdVList->QuestionId   = EFI_QUESTION_ID_INVALID;
    mEqIdVList->ListLength   = 0;
    mEqIdVList->ValueList[0] = 0;
  }

  VOID UpdateIfrBuffer (
  ) {
    _EMIT_PENDING_OBJ();
    mEqIdVList = GetObjBinAddr<EFI_IFR_EQ_ID_VAL_LIST>();
    UpdateHeader (&mEqIdVList->Header);
  }

  VOID SetQuestionId (
  IN EFI_QUESTION_ID QuestionId,
  IN CHAR8           *VarIdStr,
  IN UINT32          LineNo
  ) {
    if (QuestionId != EFI_QUESTION_ID_INVALID) {
      mEqIdVList->QuestionId = QuestionId;
    } else {
      gCFormPkg.AssignPending (VarIdStr, (VOID *)(&mEqIdVList->QuestionId), sizeof (EFI_QUESTION_ID), LineNo, NO_QST_REFED);
    }
  }

  VOID SetListLength (IN UINT16 ListLength) {
    mEqIdVList->ListLength = ListLength;
  }

  VOID SetValueList (IN UINT16 Index, IN UINT16 Value) {
    if (Index == 0) {
      mEqIdVList->ValueList[0] = Value;
      return;
    }

    if (ExpendObjBin (sizeof (UINT16)) ==TRUE) {
      IncLength (sizeof (UINT16));
      mEqIdVList->ValueList[Index] = Value;
    }
  }
};

class CIfrQuestionRef1 : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_QUESTION_REF1 *mQuestionRef1;

public:
  CIfrQuestionRef1 (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_QUESTION_REF1_OP),
      CIfrOpHeader (EFI_IFR_QUESTION_REF1_OP, &(GetObjBinAddr<EFI_IFR_QUESTION_REF1>())->Header), mQuestionRef1(GetObjBinAddr<EFI_IFR_QUESTION_REF1>()) {
    SetLineNo (LineNo);
    mQuestionRef1->QuestionId = EFI_QUESTION_ID_INVALID;
  }

  VOID SetQuestionId (
  IN EFI_QUESTION_ID QuestionId,
  IN CHAR8           *VarIdStr,
  IN UINT32          LineNo
  ) {
    if (QuestionId != EFI_QUESTION_ID_INVALID) {
      mQuestionRef1->QuestionId = QuestionId;
    } else {
      gCFormPkg.AssignPending (VarIdStr, (VOID *)(&mQuestionRef1->QuestionId), sizeof (EFI_QUESTION_ID), LineNo, NO_QST_REFED);
    }
  }
};

class CIfrQuestionRef2 : public CIfrObj, public CIfrOpHeader {
public:
  CIfrQuestionRef2 (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_QUESTION_REF2_OP),
      CIfrOpHeader (EFI_IFR_QUESTION_REF2_OP, &(GetObjBinAddr<EFI_IFR_QUESTION_REF2>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrQuestionRef3 : public CIfrObj, public CIfrOpHeader {
public:
  CIfrQuestionRef3 (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_QUESTION_REF3_OP),
      CIfrOpHeader (EFI_IFR_QUESTION_REF3_OP, &(GetObjBinAddr<EFI_IFR_QUESTION_REF3>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrQuestionRef3_2 : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_QUESTION_REF3_2 *mQuestionRef3_2;

public:
  CIfrQuestionRef3_2 (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_QUESTION_REF3_OP, (CHAR8 **)NULL, sizeof (EFI_IFR_QUESTION_REF3_2)),
      CIfrOpHeader (EFI_IFR_QUESTION_REF3_OP, &(GetObjBinAddr<EFI_IFR_QUESTION_REF3_2>())->Header, sizeof (EFI_IFR_QUESTION_REF3_2)), mQuestionRef3_2(GetObjBinAddr<EFI_IFR_QUESTION_REF3_2>()) {
    SetLineNo (LineNo);
    mQuestionRef3_2->DevicePath = EFI_STRING_ID_INVALID;
  }

  VOID SetDevicePath (IN EFI_STRING_ID DevicePath) {
    mQuestionRef3_2->DevicePath = DevicePath;
  }
};

class CIfrQuestionRef3_3 : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_QUESTION_REF3_3 *mQuestionRef3_3;

public:
  CIfrQuestionRef3_3 (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_QUESTION_REF3_OP, (CHAR8 **)NULL, sizeof (EFI_IFR_QUESTION_REF3_3)),
      CIfrOpHeader (EFI_IFR_QUESTION_REF3_OP, &(GetObjBinAddr<EFI_IFR_QUESTION_REF3_3>())->Header, sizeof (EFI_IFR_QUESTION_REF3_3)), mQuestionRef3_3(GetObjBinAddr<EFI_IFR_QUESTION_REF3_3>()) {
    SetLineNo (LineNo);
    mQuestionRef3_3->DevicePath = EFI_STRING_ID_INVALID;
    memset (&mQuestionRef3_3->Guid, 0, sizeof (EFI_GUID));
  }

  VOID SetDevicePath (IN EFI_STRING_ID DevicePath) {
    mQuestionRef3_3->DevicePath = DevicePath;
  }

  VOID SetGuid (IN EFI_GUID *Guid) {
    mQuestionRef3_3->Guid = *Guid;
  }
};

class CIfrRuleRef : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_RULE_REF *mRuleRef;

public:
  CIfrRuleRef (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_RULE_REF_OP),
      CIfrOpHeader (EFI_IFR_RULE_REF_OP, &(GetObjBinAddr<EFI_IFR_RULE_REF>())->Header), mRuleRef(GetObjBinAddr<EFI_IFR_RULE_REF>()) {
    SetLineNo (LineNo);
    mRuleRef->RuleId = EFI_RULE_ID_INVALID;
  }

  VOID SetRuleId (IN UINT8 RuleId) {
    mRuleRef->RuleId = RuleId;
  }
};

class CIfrStringRef1 : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_STRING_REF1 *mStringRef1;

public:
  CIfrStringRef1 (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_STRING_REF1_OP),
      CIfrOpHeader (EFI_IFR_STRING_REF1_OP, &(GetObjBinAddr<EFI_IFR_STRING_REF1>())->Header), mStringRef1(GetObjBinAddr<EFI_IFR_STRING_REF1>()) {
    SetLineNo (LineNo);
    mStringRef1->StringId = EFI_STRING_ID_INVALID;
  }

  VOID SetStringId (IN EFI_STRING_ID StringId) {
    mStringRef1->StringId = StringId;
  }
};

class CIfrStringRef2 : public CIfrObj, public CIfrOpHeader {
public:
  CIfrStringRef2 (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_STRING_REF2_OP),
      CIfrOpHeader (EFI_IFR_STRING_REF2_OP, &(GetObjBinAddr<EFI_IFR_STRING_REF2>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrThis : public CIfrObj, public CIfrOpHeader {
public:
  CIfrThis (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_THIS_OP),
      CIfrOpHeader (EFI_IFR_THIS_OP, &(GetObjBinAddr<EFI_IFR_THIS>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrSecurity : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_SECURITY *mSecurity;

public:
  CIfrSecurity (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_SECURITY_OP),
      CIfrOpHeader (EFI_IFR_SECURITY_OP, &(GetObjBinAddr<EFI_IFR_SECURITY>())->Header), mSecurity(GetObjBinAddr<EFI_IFR_SECURITY>()) {
    SetLineNo (LineNo);
    memset (&mSecurity->Permissions, 0, sizeof (EFI_GUID));
  }

  VOID SetPermissions (IN EFI_GUID *Permissions) {
    memmove (&mSecurity->Permissions, Permissions, sizeof (EFI_GUID));
  }
};

class CIfrUint8 : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_UINT8 *mUint8;

public:
  CIfrUint8 (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_UINT8_OP),
      CIfrOpHeader (EFI_IFR_UINT8_OP, &(GetObjBinAddr<EFI_IFR_UINT8>())->Header), mUint8(GetObjBinAddr<EFI_IFR_UINT8>()) {
    SetLineNo (LineNo);
  }

  VOID SetValue (IN UINT8 Value) {
    mUint8->Value = Value;
  }
};

class CIfrUint16 : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_UINT16 *mUint16;

public:
  CIfrUint16 (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_UINT16_OP),
      CIfrOpHeader (EFI_IFR_UINT16_OP, &(GetObjBinAddr<EFI_IFR_UINT16>())->Header), mUint16(GetObjBinAddr<EFI_IFR_UINT16>()) {
    SetLineNo (LineNo);
  }

  VOID SetValue (IN UINT16 Value) {
    mUint16->Value = Value;
  }
};

class CIfrUint32 : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_UINT32 *mUint32;

public:
  CIfrUint32 (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_UINT32_OP),
      CIfrOpHeader (EFI_IFR_UINT32_OP, &(GetObjBinAddr<EFI_IFR_UINT32>())->Header), mUint32(GetObjBinAddr<EFI_IFR_UINT32>()) {
    SetLineNo (LineNo);
  }

  VOID SetValue (IN UINT32 Value) {
    mUint32->Value = Value;
  }
};

class CIfrUint64 : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_UINT64 *mUint64;

public:
  CIfrUint64 (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_UINT64_OP),
      CIfrOpHeader (EFI_IFR_UINT64_OP, &(GetObjBinAddr<EFI_IFR_UINT64>())->Header), mUint64(GetObjBinAddr<EFI_IFR_UINT64>()) {
    SetLineNo (LineNo);
  }

  VOID SetValue (IN UINT64 Value) {
    mUint64->Value = Value;
  }
};

class CIfrTrue : public CIfrObj, public CIfrOpHeader {
public:
  CIfrTrue (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_TRUE_OP),
      CIfrOpHeader (EFI_IFR_TRUE_OP, &(GetObjBinAddr<EFI_IFR_TRUE>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrFalse : public CIfrObj, public CIfrOpHeader {
public:
  CIfrFalse (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_FALSE_OP),
      CIfrOpHeader (EFI_IFR_FALSE_OP, &(GetObjBinAddr<EFI_IFR_FALSE>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrOne : public CIfrObj, public CIfrOpHeader {
public:
  CIfrOne (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_ONE_OP),
      CIfrOpHeader (EFI_IFR_ONE_OP, &(GetObjBinAddr<EFI_IFR_ONE>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrOnes : public CIfrObj, public CIfrOpHeader {
public:
  CIfrOnes (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_ONES_OP),
      CIfrOpHeader (EFI_IFR_ONES_OP, &(GetObjBinAddr<EFI_IFR_ONES>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrZero : public CIfrObj, public CIfrOpHeader {
public:
  CIfrZero (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_ZERO_OP),
      CIfrOpHeader (EFI_IFR_ZERO_OP, &(GetObjBinAddr<EFI_IFR_ZERO>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrUndefined : public CIfrObj, public CIfrOpHeader {
public:
  CIfrUndefined (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_UNDEFINED_OP),
      CIfrOpHeader (EFI_IFR_UNDEFINED_OP, &(GetObjBinAddr<EFI_IFR_UNDEFINED>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrVersion : public CIfrObj, public CIfrOpHeader {
public:
  CIfrVersion (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_VERSION_OP),
      CIfrOpHeader (EFI_IFR_VERSION_OP, &(GetObjBinAddr<EFI_IFR_VERSION>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrLength : public CIfrObj, public CIfrOpHeader {
public:
  CIfrLength (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_LENGTH_OP),
      CIfrOpHeader (EFI_IFR_LENGTH_OP, &(GetObjBinAddr<EFI_IFR_LENGTH>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrNot : public CIfrObj, public CIfrOpHeader {
public:
  CIfrNot (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_NOT_OP),
      CIfrOpHeader (EFI_IFR_NOT_OP, &(GetObjBinAddr<EFI_IFR_NOT>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrBitWiseNot : public CIfrObj, public CIfrOpHeader {
public:
  CIfrBitWiseNot (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_BITWISE_NOT_OP),
      CIfrOpHeader (EFI_IFR_BITWISE_NOT_OP, &(GetObjBinAddr<EFI_IFR_BITWISE_NOT>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrToBoolean : public CIfrObj, public CIfrOpHeader {
public:
  CIfrToBoolean (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_TO_BOOLEAN_OP),
      CIfrOpHeader (EFI_IFR_TO_BOOLEAN_OP, &(GetObjBinAddr<EFI_IFR_TO_BOOLEAN>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrToString : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_TO_STRING *mToString;

public:
  CIfrToString (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_TO_STRING_OP),
      CIfrOpHeader (EFI_IFR_TO_STRING_OP, &(GetObjBinAddr<EFI_IFR_TO_STRING>())->Header), mToString(GetObjBinAddr<EFI_IFR_TO_STRING>()) {
    SetLineNo (LineNo);
  }

  VOID SetFormat (IN UINT8 Format) {
    mToString->Format = Format;
  }
};

class CIfrToUint : public CIfrObj, public CIfrOpHeader {
public:
  CIfrToUint (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_TO_UINT_OP),
      CIfrOpHeader (EFI_IFR_TO_UINT_OP, &(GetObjBinAddr<EFI_IFR_TO_UINT>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrToUpper : public CIfrObj, public CIfrOpHeader {
public:
  CIfrToUpper (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_TO_UPPER_OP),
      CIfrOpHeader (EFI_IFR_TO_UPPER_OP, &(GetObjBinAddr<EFI_IFR_TO_UPPER>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrToLower : public CIfrObj, public CIfrOpHeader {
public:
  CIfrToLower (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_TO_LOWER_OP),
      CIfrOpHeader (EFI_IFR_TO_LOWER_OP, &(GetObjBinAddr<EFI_IFR_TO_LOWER>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrAdd : public CIfrObj, public CIfrOpHeader {
public:
  CIfrAdd (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_ADD_OP),
      CIfrOpHeader (EFI_IFR_ADD_OP, &(GetObjBinAddr<EFI_IFR_ADD>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrBitWiseAnd : public CIfrObj, public CIfrOpHeader {
public:
  CIfrBitWiseAnd (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_BITWISE_AND_OP),
      CIfrOpHeader (EFI_IFR_BITWISE_AND_OP, &(GetObjBinAddr<EFI_IFR_BITWISE_AND>())->Header) {
    SetLineNo(LineNo);
  }
};

class CIfrBitWiseOr : public CIfrObj, public CIfrOpHeader {
public:
  CIfrBitWiseOr (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_BITWISE_OR_OP),
      CIfrOpHeader (EFI_IFR_BITWISE_OR_OP, &(GetObjBinAddr<EFI_IFR_BITWISE_OR>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrAnd : public CIfrObj, public CIfrOpHeader {
public:
  CIfrAnd (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_AND_OP),
      CIfrOpHeader (EFI_IFR_AND_OP, &(GetObjBinAddr<EFI_IFR_AND>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrCatenate : public CIfrObj, public CIfrOpHeader {
public:
  CIfrCatenate (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_CATENATE_OP),
      CIfrOpHeader (EFI_IFR_CATENATE_OP, &(GetObjBinAddr<EFI_IFR_CATENATE>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrDivide : public CIfrObj, public CIfrOpHeader {
public:
  CIfrDivide (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_DIVIDE_OP),
      CIfrOpHeader (EFI_IFR_DIVIDE_OP, &(GetObjBinAddr<EFI_IFR_DIVIDE>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrEqual : public CIfrObj, public CIfrOpHeader {
public:
  CIfrEqual (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_EQUAL_OP),
      CIfrOpHeader (EFI_IFR_EQUAL_OP, &(GetObjBinAddr<EFI_IFR_EQUAL>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrGreaterEqual : public CIfrObj, public CIfrOpHeader {
public:
  CIfrGreaterEqual (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_GREATER_EQUAL_OP),
      CIfrOpHeader (EFI_IFR_GREATER_EQUAL_OP, &(GetObjBinAddr<EFI_IFR_GREATER_EQUAL>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrGreaterThan : public CIfrObj, public CIfrOpHeader {
public:
  CIfrGreaterThan (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_GREATER_THAN_OP),
      CIfrOpHeader (EFI_IFR_GREATER_THAN_OP, &(GetObjBinAddr<EFI_IFR_GREATER_THAN>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrLessEqual : public CIfrObj, public CIfrOpHeader {
public:
  CIfrLessEqual (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_LESS_EQUAL_OP),
      CIfrOpHeader (EFI_IFR_LESS_EQUAL_OP, &(GetObjBinAddr<EFI_IFR_LESS_EQUAL>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrLessThan : public CIfrObj, public CIfrOpHeader {
public:
  CIfrLessThan (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_LESS_THAN_OP),
      CIfrOpHeader (EFI_IFR_LESS_THAN_OP, &(GetObjBinAddr<EFI_IFR_LESS_THAN>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrMap : public CIfrObj, public CIfrOpHeader{
public:
  CIfrMap (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_MAP_OP),
      CIfrOpHeader (EFI_IFR_MAP_OP, &(GetObjBinAddr<EFI_IFR_MAP>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrMatch : public CIfrObj, public CIfrOpHeader {
public:
  CIfrMatch (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_MATCH_OP),
      CIfrOpHeader (EFI_IFR_MATCH_OP, &(GetObjBinAddr<EFI_IFR_MATCH>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrMatch2 : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_MATCH2 *mMatch2;

public:
  CIfrMatch2 (
  IN UINT32   LineNo,
  IN EFI_GUID *Guid
  ) : CIfrObj (EFI_IFR_MATCH2_OP),
      CIfrOpHeader (EFI_IFR_MATCH2_OP, &(GetObjBinAddr<EFI_IFR_MATCH2>())->Header), mMatch2(GetObjBinAddr<EFI_IFR_MATCH2>()) {
    SetLineNo (LineNo);
    memmove (&mMatch2->SyntaxType, Guid, sizeof (EFI_GUID));
  }
};

class CIfrMultiply : public CIfrObj, public CIfrOpHeader {
public:
  CIfrMultiply (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_MULTIPLY_OP),
      CIfrOpHeader (EFI_IFR_MULTIPLY_OP, &(GetObjBinAddr<EFI_IFR_MULTIPLY>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrModulo : public CIfrObj, public CIfrOpHeader {
public:
  CIfrModulo (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_MODULO_OP),
      CIfrOpHeader (EFI_IFR_MODULO_OP, &(GetObjBinAddr<EFI_IFR_MODULO>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrNotEqual : public CIfrObj, public CIfrOpHeader {
public:
  CIfrNotEqual (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_NOT_EQUAL_OP),
      CIfrOpHeader (EFI_IFR_NOT_EQUAL_OP, &(GetObjBinAddr<EFI_IFR_NOT_EQUAL>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrOr : public CIfrObj, public CIfrOpHeader {
public:
  CIfrOr (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_OR_OP),
      CIfrOpHeader (EFI_IFR_OR_OP, &(GetObjBinAddr<EFI_IFR_OR>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrShiftLeft : public CIfrObj, public CIfrOpHeader {
public:
  CIfrShiftLeft (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_SHIFT_LEFT_OP),
      CIfrOpHeader (EFI_IFR_SHIFT_LEFT_OP, &(GetObjBinAddr<EFI_IFR_SHIFT_LEFT>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrShiftRight : public CIfrObj, public CIfrOpHeader {
public:
  CIfrShiftRight (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_SHIFT_RIGHT_OP),
      CIfrOpHeader (EFI_IFR_SHIFT_RIGHT_OP, &(GetObjBinAddr<EFI_IFR_SHIFT_RIGHT>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrSubtract : public CIfrObj, public CIfrOpHeader {
public:
  CIfrSubtract (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_SUBTRACT_OP),
      CIfrOpHeader (EFI_IFR_SUBTRACT_OP, &(GetObjBinAddr<EFI_IFR_SUBTRACT>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrConditional : public CIfrObj, public CIfrOpHeader {
public:
  CIfrConditional (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_CONDITIONAL_OP),
      CIfrOpHeader (EFI_IFR_CONDITIONAL_OP, &(GetObjBinAddr<EFI_IFR_CONDITIONAL>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrFind : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_FIND *mFind;

public:
  CIfrFind (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_FIND_OP),
      CIfrOpHeader (EFI_IFR_FIND_OP, &(GetObjBinAddr<EFI_IFR_FIND>())->Header), mFind(GetObjBinAddr<EFI_IFR_FIND>()) {
    SetLineNo (LineNo);
  }

  VOID SetFormat (IN UINT8 Format) {
    mFind->Format = Format;
  }
};

class CIfrMid : public CIfrObj, public CIfrOpHeader {
public:
  CIfrMid (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_MID_OP),
      CIfrOpHeader (EFI_IFR_MID_OP, &(GetObjBinAddr<EFI_IFR_MID>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrToken : public CIfrObj, public CIfrOpHeader {
public:
  CIfrToken (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_TOKEN_OP),
      CIfrOpHeader (EFI_IFR_TOKEN_OP, &(GetObjBinAddr<EFI_IFR_TOKEN>())->Header) {
    SetLineNo (LineNo);
  }
};

class CIfrSpan : public CIfrObj, public CIfrOpHeader {
private:
  EFI_IFR_SPAN *mSpan;

public:
  CIfrSpan (
  IN UINT32 LineNo
  ) : CIfrObj (EFI_IFR_SPAN_OP),
      CIfrOpHeader (EFI_IFR_SPAN_OP, &(GetObjBinAddr<EFI_IFR_SPAN>())->Header), mSpan(GetObjBinAddr<EFI_IFR_SPAN>()) {
    SetLineNo (LineNo);
    mSpan->Flags = EFI_IFR_FLAGS_FIRST_MATCHING;
  }

  EFI_VFR_RETURN_CODE SetFlags (IN UINT8 LFlags) {
    if (_IS_EQUAL (LFlags, EFI_IFR_FLAGS_FIRST_MATCHING)) {
      mSpan->Flags |= EFI_IFR_FLAGS_FIRST_MATCHING;
    } else if (_FLAG_TEST_AND_CLEAR (LFlags, EFI_IFR_FLAGS_FIRST_NON_MATCHING)) {
      mSpan->Flags |= EFI_IFR_FLAGS_FIRST_NON_MATCHING;
    }

    return _FLAGS_ZERO (LFlags) ? VFR_RETURN_SUCCESS : VFR_RETURN_FLAGS_UNSUPPORTED;
  }
};

#endif
