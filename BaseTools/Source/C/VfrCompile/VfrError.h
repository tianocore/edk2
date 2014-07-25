/** @file
  
  VfrCompiler Error definition

Copyright (c) 2004 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _VFRERROR_H_
#define _VFRERROR_H_

#include "Common/UefiBaseTypes.h"

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
  VFR_RETURN_VARSTORE_DATATYPE_REDEFINED_ERROR,
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
  VFR_RETURN_VARSTORE_NAME_REDEFINED_ERROR,
  VFR_RETURN_CODEUNDEFINED
} EFI_VFR_RETURN_CODE;

typedef enum {
  VFR_WARNING_DEFAULT_VALUE_REDEFINED = 0,
  VFR_WARNING_STRING_TO_UINT_OVERFLOW,
  VFR_WARNING_ACTION_WITH_TEXT_TWO,
  VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE,
  VFR_WARNING_CODEUNDEFINED
} EFI_VFR_WARNING_CODE;

typedef struct _SVFR_ERROR_HANDLE {
  EFI_VFR_RETURN_CODE    mErrorCode;
  CONST CHAR8            *mErrorMsg;
} SVFR_ERROR_HANDLE;

typedef struct _SVFR_WARNING_HANDLE {
  EFI_VFR_WARNING_CODE    mWarningCode;
  CONST CHAR8            *mWarningMsg;
} SVFR_WARNING_HANDLE;

struct SVfrFileScopeRecord {
  CHAR8                 *mFileName;
  UINT32                mWholeScopeLine;
  UINT32                mScopeLineStart;
  SVfrFileScopeRecord *mNext;

  SVfrFileScopeRecord (IN CHAR8 *, IN UINT32);
  ~SVfrFileScopeRecord();
};

class CVfrErrorHandle {
private:
  CHAR8               *mInputFileName;
  SVFR_ERROR_HANDLE   *mVfrErrorHandleTable;
  SVFR_WARNING_HANDLE *mVfrWarningHandleTable;
  SVfrFileScopeRecord *mScopeRecordListHead;
  SVfrFileScopeRecord *mScopeRecordListTail;
  BOOLEAN             mWarningAsError;

public:
  CVfrErrorHandle (VOID);
  ~CVfrErrorHandle (VOID);

  VOID  SetWarningAsError (IN BOOLEAN);
  VOID  SetInputFile (IN CHAR8 *);
  VOID  ParseFileScopeRecord (IN CHAR8 *, IN UINT32);
  VOID  GetFileNameLineNum (IN UINT32, OUT CHAR8 **, OUT UINT32 *);
  UINT8 HandleError (IN EFI_VFR_RETURN_CODE, IN UINT32 LineNum = 0, IN CHAR8 *TokName = NULL);
  UINT8 HandleWarning (IN EFI_VFR_WARNING_CODE, IN UINT32 LineNum = 0, IN CHAR8 *TokName = NULL);
  VOID  PrintMsg (IN UINT32 LineNum = 0, IN CHAR8 *TokName = NULL, IN CONST CHAR8 *MsgType = "Error", IN CONST CHAR8 *ErrorMsg = "");
};

#define CHECK_ERROR_RETURN(f, v) do { EFI_VFR_RETURN_CODE r; if ((r = (f)) != (v)) { return r; } } while (0)

extern CVfrErrorHandle gCVfrErrorHandle;

#endif
