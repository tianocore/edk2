import imp
import sys
from enum import Enum


class VfrReturnCode(Enum) :
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


vfrErrorMessage = {
    VfrReturnCode.VFR_RETURN_SUCCESS: '',
    VfrReturnCode.VFR_RETURN_ERROR_SKIPED: '',
    VfrReturnCode.VFR_RETURN_FATAL_ERROR: 'fatal error!!',
    VfrReturnCode.VFR_RETURN_MISMATCHED: 'unexpected token',
    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER: 'invalid parameter',
    VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES: 'system out of memory',
    VfrReturnCode.VFR_RETURN_UNSUPPORTED: 'unsupported',
    VfrReturnCode.VFR_RETURN_REDEFINED: 'already defined',
    VfrReturnCode.VFR_RETURN_FORMID_REDEFINED: 'form id already defined',
    VfrReturnCode.VFR_RETURN_QUESTIONID_REDEFINED: 'question id already defined',
    VfrReturnCode.VFR_RETURN_VARSTOREID_REDEFINED: 'varstore id already defined',
    VfrReturnCode.VFR_RETURN_UNDEFINED: 'undefined',
    VfrReturnCode.VFR_RETURN_VAR_NOTDEFINED_BY_QUESTION: 'some variable has not defined by a question',
    VfrReturnCode.VFR_RETURN_VARSTORE_DATATYPE_REDEFINED_ERROR:
    'Data Structure is defined by more than one varstores: it can\'t be referred as varstore: only varstore name could be used.',
    VfrReturnCode.VFR_RETURN_GET_EFIVARSTORE_ERROR: 'get efi varstore error',
    VfrReturnCode.VFR_RETURN_EFIVARSTORE_USE_ERROR:
    'can not use the efi varstore like this',
    VfrReturnCode.VFR_RETURN_EFIVARSTORE_SIZE_ERROR:
    'unsupport efi varstore size should be <= 8 bytes',
    VfrReturnCode.VFR_RETURN_GET_NVVARSTORE_ERROR:
    'get name value varstore error',
    VfrReturnCode.VFR_RETURN_QVAR_REUSE:
    'variable reused by more than one question',
    VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED: 'flags unsupported',
    VfrReturnCode.VFR_RETURN_ERROR_ARRARY_NUM:
    'array number error: the valid value is in (0 ~ MAX_INDEX-1 for UEFI vfr and in (1 ~ MAX_INDEX for Framework Vfr',
    VfrReturnCode.VFR_RETURN_DATA_STRING_ERROR:
    'data field string error or not support',
    VfrReturnCode.VFR_RETURN_DEFAULT_VALUE_REDEFINED:
    'default value re-defined with different value',
    VfrReturnCode.VFR_RETURN_CONSTANT_ONLY:
    'only constant is allowed in the expression',
    VfrReturnCode.VFR_RETURN_VARSTORE_NAME_REDEFINED_ERROR:
    'Varstore name is defined by more than one varstores: it can\'t be referred as varstore: only varstore structure name could be used.',
    VfrReturnCode.VFR_RETURN_BIT_WIDTH_ERROR:
    'bit width must be <= sizeof (type * 8 and the max width can not > 32',
    VfrReturnCode.VFR_RETURN_STRING_TO_UINT_OVERFLOW:
    'String to UINT* Overflow',
    VfrReturnCode.VFR_RETURN_CODEUNDEFINED: 'undefined Error Code'
}

class EFI_VFR_WARNING_CODE(Enum) :
    VFR_WARNING_DEFAULT_VALUE_REDEFINED = 0
    VFR_WARNING_ACTION_WITH_TEXT_TWO = 1
    VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE = 2
    VFR_WARNING_CODEUNDEFINED = 3


VFR_WARNING_HANDLE_TABLE = {
    EFI_VFR_WARNING_CODE.VFR_WARNING_DEFAULT_VALUE_REDEFINED:
    ": default value re-defined with different value",
    EFI_VFR_WARNING_CODE.VFR_WARNING_ACTION_WITH_TEXT_TWO:
    ": Action opcode should not have TextTwo part",
    EFI_VFR_WARNING_CODE.VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE:
    ": Not recommend to use obsoleted framework opcode",
    EFI_VFR_WARNING_CODE.VFR_WARNING_CODEUNDEFINED: ": undefined Warning Code"
}

def CheckErrorReturn(f, v) :
    if f != v:
        return f
    
#class SVfrFileScopeRecord():
    
class CVfrErrorHandle():

    def __init__(self):
        self.__InputFileName = None
        self.__VfrErrorHandleTable = vfrErrorMessage
        self.__VfrWarningHandleTable = VFR_WARNING_HANDLE_TABLE
        self.__ScopeRecordListHead = None
        self.__ScopeRecordListTail = None
        self.__WarningAsError = False

    def SetWarningAsError(self, WarningAsError):
        self.__WarningAsError = WarningAsError

    def SetInputFile(self, InputFile):
        self.__InputFileName = InputFile

    def ParseFileScopeRecord(self):
        pass
    
    # def GetFileNameLineNum(self, LineNum):
    
    def HandleWarning(self, WarningCode, LineNum, TokenValue=None):
        pass

    def PrintMsg(self, LineNum, MsgType = 'Error', ErrorMsg = None):
        if MsgType == 'Warning':
            pass
        

    def HandleError(self, ErrorCode, LineNum=None, Message=None, TokenValue=None):
        if self.__VfrErrorHandleTable == None:
            return 1
        ErrorMsg = ''
        for key in self.__VfrErrorHandleTable.keys():
            if ErrorCode == self.__VfrErrorHandleTable[key]:
                ErrorMsg = self.__VfrErrorHandleTable[key]
                break
        if ErrorMsg != '':
            #EdkLogger.error('build', ErrorCode, ErrorMsg, None, LineNum, TokenValue)
            return 1
        else:
            return 0
