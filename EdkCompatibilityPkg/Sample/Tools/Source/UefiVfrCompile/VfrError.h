/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  VfrError.h

Abstract:

--*/

#ifndef _VFRERROR_H_
#define _VFRERROR_H_

#include "Tiano.h"
#include "EfiTypes.h"

typedef enum {
  VFR_RETURN_SUCCESS = 0,
  VFR_RETURN_ERROR_SKIPED,
  VFR_RETURN_FATAL_ERROR,
  VFR_RETURN_MISMATCHED,
  VFR_RETURN_INVALID_PARAMETER,
  VFR_RETURN_OUT_FOR_RESOURCES,
  VFR_RETURN_UNSUPPORTED,
  VFR_RETURN_REDEFINED,
  VFR_RETURN_FORMID_REDEFINED,
  VFR_RETURN_QUESTIONID_REDEFINED,
  VFR_RETURN_VARSTOREID_REDEFINED,
  VFR_RETURN_UNDEFINED,
  VFR_RETURN_VAR_NOTDEFINED_BY_QUESTION,
  VFR_RETURN_GET_EFIVARSTORE_ERROR,
  VFR_RETURN_EFIVARSTORE_USE_ERROR,
  VFR_RETURN_EFIVARSTORE_SIZE_ERROR,
  VFR_RETURN_GET_NVVARSTORE_ERROR,
  VFR_RETURN_QVAR_REUSE,
  VFR_RETURN_FLAGS_UNSUPPORTED,
  VFR_RETURN_ERROR_ARRARY_NUM,
  VFR_RETURN_DATA_STRING_ERROR,
  VFR_RETURN_DEFAULT_VALUE_REDEFINED,
  VFR_RETURN_CONSTANT_ONLY,
  VFR_RETURN_CODEUNDEFINED
} EFI_VFR_RETURN_CODE;

typedef struct _SVFR_ERROR_HANDLE {
  EFI_VFR_RETURN_CODE    mErrorCode;
  INT8                   *mErrorMsg;
} SVFR_ERROR_HANDLE;

struct SVfrFileScopeRecord {
  INT8                  *mFileName;
  UINT32                mWholeScopeLine;
  UINT32                mScopeLineStart;
  SVfrFileScopeRecord *mNext;

  SVfrFileScopeRecord (IN INT8 *, IN UINT32);
  ~SVfrFileScopeRecord();
};

class CVfrErrorHandle {
private:
  INT8                *mInputFileName;
  SVFR_ERROR_HANDLE   *mVfrErrorHandleTable;
  SVfrFileScopeRecord *mScopeRecordListHead;
  SVfrFileScopeRecord *mScopeRecordListTail;

public:
  CVfrErrorHandle (VOID);
  ~CVfrErrorHandle (VOID);

  VOID  SetInputFile (IN INT8 *);
  VOID  ParseFileScopeRecord (IN INT8 *, IN UINT32);
  VOID  GetFileNameLineNum (IN UINT32, OUT INT8 **, OUT UINT32 *);
  UINT8 HandleError (IN EFI_VFR_RETURN_CODE, IN UINT32 LineNum = 0, IN INT8 *TokName = "\0");
  VOID  PrintMsg (IN UINT32 LineNum = 0, IN INT8 *TokName = "\0", IN INT8 *MsgType = "Error", IN INT8 *ErrorMsg = "\0");
};

#define CHECK_ERROR_RETURN(f, v) do { EFI_VFR_RETURN_CODE r; if ((r = (f)) != (v)) { return r; } } while (0)

extern CVfrErrorHandle gCVfrErrorHandle;

#endif
