import imp
import sys
from enum import Enum


class VfrReturnCode(Enum):
    VFR_RETURN_SUCCESS = 0
    VFR_RETURN_ERROR_SKIPED = 1
    VFR_RETURN_FATAL_ERROR = 2
    VFR_RETURN_MISMATCHED = 3
    VFR_RETURN_INVALID_PARAMETER = 4
    VFR_RETURN_OUT_FOR_RESOURCES = 5
    VFR_RETURN_UNSUPPORTED = 6
    VFR_RETURN_REDEFINED = 7
    VFR_RETURN_FORMID_REDEFINED = 8
    VFR_RETURN_QUESTIONID_REDEFINED = 9
    VFR_RETURN_VARSTOREID_REDEFINED = 10
    VFR_RETURN_UNDEFINED = 11
    VFR_RETURN_VAR_NOTDEFINED_BY_QUESTION = 12
    VFR_RETURN_VARSTORE_DATATYPE_REDEFINED_ERROR = 13
    VFR_RETURN_GET_EFIVARSTORE_ERROR = 14
    VFR_RETURN_EFIVARSTORE_USE_ERROR = 15
    VFR_RETURN_EFIVARSTORE_SIZE_ERROR = 16
    VFR_RETURN_GET_NVVARSTORE_ERROR = 17
    VFR_RETURN_QVAR_REUSE = 18
    VFR_RETURN_FLAGS_UNSUPPORTED = 19
    VFR_RETURN_ERROR_ARRARY_NUM = 20
    VFR_RETURN_DATA_STRING_ERROR = 21
    VFR_RETURN_DEFAULT_VALUE_REDEFINED = 22
    VFR_RETURN_CONSTANT_ONLY = 23
    VFR_RETURN_VARSTORE_NAME_REDEFINED_ERROR = 24
    VFR_RETURN_BIT_WIDTH_ERROR = 25
    VFR_RETURN_STRING_TO_UINT_OVERFLOW = 26
    VFR_RETURN_CODEUNDEFINED = 27


class VfrWarningCode(Enum):
    VFR_WARNING_DEFAULT_VALUE_REDEFINED = 0
    VFR_WARNING_ACTION_WITH_TEXT_TWO = 1
    VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE = 2
    VFR_WARNING_CODEUNDEFINED = 3


def CheckErrorReturn(f, v):
    if f != v:
        return f


class CVfrErrorHandle():

    def __init__(self):
        self.__mInputFileName = ''
        self.__mVfrErrorHandleTable = None
        self.__mVfrWarningHandleTable = None
        self.__mScopeRecordListHead = None
        self.__mScopeRecordListTail = None
        self.__mWarningAsError = False

    def SetWarningAsError():
        pass

    def SetInputFile():
        pass

    def ParseFileScopeRecord():
        pass

    def PrintMsg(self, LineNum=0, TokName='', MsgType='Error', ErrorMsg=''):
        pass