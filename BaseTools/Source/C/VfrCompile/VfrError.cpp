/** @file

  VfrCompiler error handler.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "VfrError.h"
#include "EfiUtilityMsgs.h"

static SVFR_ERROR_HANDLE VFR_ERROR_HANDLE_TABLE [] = {
  { VFR_RETURN_SUCCESS, NULL },
  { VFR_RETURN_ERROR_SKIPED, NULL },
  { VFR_RETURN_FATAL_ERROR, ": fatal error!!" },

  { VFR_RETURN_MISMATCHED, ": unexpected token" },
  { VFR_RETURN_INVALID_PARAMETER, ": invalid parameter" },
  { VFR_RETURN_OUT_FOR_RESOURCES, ": system out of memory" },
  { VFR_RETURN_UNSUPPORTED, ": unsupported" },
  { VFR_RETURN_REDEFINED, ": already defined" },
  { VFR_RETURN_FORMID_REDEFINED, ": form id already defined" },
  { VFR_RETURN_QUESTIONID_REDEFINED, ": question id already defined" },
  { VFR_RETURN_VARSTOREID_REDEFINED, ": varstore id already defined" },
  { VFR_RETURN_UNDEFINED, ": undefined" },
  { VFR_RETURN_VAR_NOTDEFINED_BY_QUESTION, ": some variable has not defined by a question"},
  { VFR_RETURN_VARSTORE_DATATYPE_REDEFINED_ERROR, ": Data Structure is defined by more than one varstores, it can't be referred as varstore, only varstore name could be used."},
  { VFR_RETURN_GET_EFIVARSTORE_ERROR, ": get efi varstore error"},
  { VFR_RETURN_EFIVARSTORE_USE_ERROR, ": can not use the efi varstore like this" },
  { VFR_RETURN_EFIVARSTORE_SIZE_ERROR, ": unsupport efi varstore size should be <= 8 bytes" },
  { VFR_RETURN_GET_NVVARSTORE_ERROR, ": get name value varstore error" },
  { VFR_RETURN_QVAR_REUSE, ": variable reused by more than one question" },
  { VFR_RETURN_FLAGS_UNSUPPORTED, ": flags unsupported" },
  { VFR_RETURN_ERROR_ARRARY_NUM, ": array number error, the valid value is in (0 ~ MAX_INDEX-1) for UEFI vfr and in (1 ~ MAX_INDEX) for Framework Vfr" },
  { VFR_RETURN_DATA_STRING_ERROR, ": data field string error or not support"},
  { VFR_RETURN_DEFAULT_VALUE_REDEFINED, ": default value re-defined with different value"},
  { VFR_RETURN_CONSTANT_ONLY, ": only constant is allowed in the expression"},
  { VFR_RETURN_VARSTORE_NAME_REDEFINED_ERROR, ": Varstore name is defined by more than one varstores, it can't be referred as varstore, only varstore structure name could be used."},
  { VFR_RETURN_BIT_WIDTH_ERROR, ": bit width must be <= sizeof (type) * 8 and the max width can not > 32" },
  { VFR_RETURN_STRING_TO_UINT_OVERFLOW, ": String to UINT* Overflow"},
  { VFR_RETURN_CODEUNDEFINED, ": undefined Error Code" }
};

static SVFR_WARNING_HANDLE VFR_WARNING_HANDLE_TABLE [] = {
  { VFR_WARNING_DEFAULT_VALUE_REDEFINED, ": default value re-defined with different value"},
  { VFR_WARNING_ACTION_WITH_TEXT_TWO, ": Action opcode should not have TextTwo part"},
  { VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE, ": Not recommend to use obsoleted framework opcode"},
  { VFR_WARNING_CODEUNDEFINED, ": undefined Warning Code" }
};

CVfrErrorHandle::CVfrErrorHandle (
  VOID
  )
{
  mInputFileName         = NULL;
  mScopeRecordListHead   = NULL;
  mScopeRecordListTail   = NULL;
  mVfrErrorHandleTable   = VFR_ERROR_HANDLE_TABLE;
  mVfrWarningHandleTable = VFR_WARNING_HANDLE_TABLE;
  mWarningAsError        = FALSE;
}

CVfrErrorHandle::~CVfrErrorHandle (
  VOID
  )
{
  SVfrFileScopeRecord *pNode = NULL;

  if (mInputFileName != NULL) {
    delete[] mInputFileName;
  }

  while (mScopeRecordListHead != NULL) {
    pNode = mScopeRecordListHead;
    mScopeRecordListHead = mScopeRecordListHead->mNext;
    delete pNode;
  }

  mScopeRecordListHead   = NULL;
  mScopeRecordListTail   = NULL;
  mVfrErrorHandleTable   = NULL;
  mVfrWarningHandleTable = NULL;
}

VOID
CVfrErrorHandle::SetWarningAsError (
  IN BOOLEAN  WarningAsError
  )
{
  mWarningAsError = WarningAsError;
}

VOID
CVfrErrorHandle::SetInputFile (
  IN CHAR8    *InputFile
  )
{
  if (InputFile != NULL) {
    mInputFileName = new CHAR8[strlen(InputFile) + 1];
    strcpy (mInputFileName, InputFile);
  }
}

SVfrFileScopeRecord::SVfrFileScopeRecord (
  IN CHAR8    *Record,
  IN UINT32   LineNum
  )
{
  UINT32      Index;
  CHAR8       *FileName = NULL;
  CHAR8       *Str      = NULL;

  mWholeScopeLine      = LineNum;
  mNext                = NULL;

  Str = strchr (Record, ' ');
  mScopeLineStart = atoi (++Str);

  Str = strchr (Str, '\"');
  FileName = ++Str;

  while((Str = strstr (FileName, "\\\\")) != NULL) {
    FileName = Str + 2;
  }
  if ((mFileName = new CHAR8[strlen(FileName)]) != NULL) {
    for (Index = 0; FileName[Index] != '\"'; Index++) {
      mFileName[Index] = FileName[Index];
    }
    mFileName[Index] = '\0';
  }

  return;
}

SVfrFileScopeRecord::~SVfrFileScopeRecord (
  VOID
  )
{
  if (mFileName != NULL) {
    delete[] mFileName;
  }
}

VOID
CVfrErrorHandle::ParseFileScopeRecord (
  IN CHAR8     *Record,
  IN UINT32    WholeScopeLine
  )
{
  SVfrFileScopeRecord *pNode        = NULL;

  if (Record == NULL) {
    return;
  }

  if ((pNode = new SVfrFileScopeRecord(Record, WholeScopeLine)) == NULL) {
    return;
  }

  if (mScopeRecordListHead == NULL) {
    mScopeRecordListTail = mScopeRecordListHead = pNode;
  } else {
    mScopeRecordListTail->mNext = pNode;
    mScopeRecordListTail        = pNode;
  }
}

VOID
CVfrErrorHandle::GetFileNameLineNum (
  IN  UINT32 LineNum,
  OUT CHAR8  **FileName,
  OUT UINT32 *FileLine
  )
{
  SVfrFileScopeRecord *pNode    = NULL;

  if ((FileName == NULL) || (FileLine == NULL)) {
    return;
  }

  *FileName = NULL;
  *FileLine = 0xFFFFFFFF;

  //
  // Some errors occur before scope record list been built.
  //
  if (mScopeRecordListHead == NULL) {
    *FileLine = LineNum;
    *FileName = mInputFileName;
    return ;
  }

  for (pNode = mScopeRecordListHead; pNode->mNext != NULL; pNode = pNode->mNext) {
    if ((LineNum > pNode->mWholeScopeLine) && (pNode->mNext->mWholeScopeLine > LineNum)) {
      *FileName = pNode->mFileName;
      *FileLine = LineNum - pNode->mWholeScopeLine + pNode->mScopeLineStart - 1;
      return ;
    }
  }

  *FileName = pNode->mFileName;
  *FileLine = LineNum - pNode->mWholeScopeLine + pNode->mScopeLineStart - 1;
}

VOID
CVfrErrorHandle::PrintMsg (
  IN UINT32               LineNum,
  IN CHAR8                *TokName,
  IN CONST CHAR8          *MsgType,
  IN CONST CHAR8          *ErrorMsg
  )
{
  CHAR8                  *FileName = NULL;
  UINT32                 FileLine;

  if (strncmp ("Warning", MsgType, strlen ("Warning")) == 0) {
    VerboseMsg ((CHAR8 *) ErrorMsg);
    return;
  }
  GetFileNameLineNum (LineNum, &FileName, &FileLine);
  Error (FileName, FileLine, 0x3000, TokName, (CHAR8 *) "\t%s\n", (CHAR8 *) ErrorMsg);
}

UINT8
CVfrErrorHandle::HandleError (
  IN EFI_VFR_RETURN_CODE  ErrorCode,
  IN UINT32               LineNum,
  IN CHAR8                *TokName
  )
{
  UINT32                 Index;
  CHAR8                  *FileName = NULL;
  UINT32                 FileLine;
  CONST CHAR8            *ErrorMsg = NULL;

  if (mVfrErrorHandleTable == NULL) {
    return 1;
  }

  for (Index = 0; mVfrErrorHandleTable[Index].mErrorCode != VFR_RETURN_CODEUNDEFINED; Index++) {
    if (ErrorCode == mVfrErrorHandleTable[Index].mErrorCode) {
      ErrorMsg = mVfrErrorHandleTable[Index].mErrorMsg;
      break;
    }
  }

  if (ErrorMsg != NULL) {
    GetFileNameLineNum (LineNum, &FileName, &FileLine);
    Error (FileName, FileLine, 0x3000, TokName, (CHAR8 *) "\t%s\n", (CHAR8 *) ErrorMsg);
    return 1;
  } else {
    return 0;
  }
}

UINT8
CVfrErrorHandle::HandleWarning (
  IN EFI_VFR_WARNING_CODE WarningCode,
  IN UINT32               LineNum,
  IN CHAR8                *TokName
  )
{
  UINT32                 Index;
  CHAR8                  *FileName = NULL;
  UINT32                 FileLine;
  CONST CHAR8            *WarningMsg = NULL;

  if (mVfrWarningHandleTable == NULL) {
    return 1;
  }

  GetFileNameLineNum (LineNum, &FileName, &FileLine);

  if (mWarningAsError) {
    Error (FileName, FileLine, 0x2220, (CHAR8 *) "warning treated as error", NULL);
  }

  for (Index = 0; mVfrWarningHandleTable[Index].mWarningCode != VFR_WARNING_CODEUNDEFINED; Index++) {
    if (WarningCode == mVfrWarningHandleTable[Index].mWarningCode) {
      WarningMsg = mVfrWarningHandleTable[Index].mWarningMsg;
      break;
    }
  }

  if (WarningMsg != NULL) {
    Warning (FileName, FileLine, 0, TokName, (CHAR8 *) "\t%s\n", (CHAR8 *) WarningMsg);
    return 1;
  } else {
    return 0;
  }
}

CVfrErrorHandle gCVfrErrorHandle;
