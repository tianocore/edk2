from ast import For
from re import L
from sre_parse import FLAGS
from stat import FILE_ATTRIBUTE_SPARSE_FILE
from CommonCtypes import *
from VfrError import VfrReturnCode
from VfrUtility import *

from ctypes import *

gVfrVarDataTypeDB = VfrVarDataTypeDB()
gVfrDefaultStore = VfrDefaultStore()
gVfrDataStorage = VfrDataStorage()

class ReCordNode(Structure):
    def __init__(self, Record, LineNo):
        self.Record = Record
        self.LineNo = LineNo

class OpNode():

    def __init__(self, Size, Scope):
        self.Size = Size
        self.Scope = Scope

gOpcodeSizesScopeTable = [
    OpNode(0, 0),  # EFI_IFR_INVALID - 0x00
    OpNode(ctypes.sizeof(EFI_IFR_FORM), 1),  # EFI_IFR_FORM_OP
    OpNode(ctypes.sizeof(EFI_IFR_SUBTITLE), 1),  # EFI_IFR_SUBTITLE_OP
    OpNode(ctypes.sizeof(EFI_IFR_TEXT), 0),  # EFI_IFR_TEXT_OP
    OpNode(ctypes.sizeof(EFI_IFR_IMAGE), 0),  # EFI_IFR_IMAGE_OP
    OpNode(ctypes.sizeof(EFI_IFR_ONE_OF), 1),  # EFI_IFR_ONE_OF_OP - 0x05
    OpNode(ctypes.sizeof(EFI_IFR_CHECKBOX), 1),  # EFI_IFR_CHECKBOX_OP
    OpNode(ctypes.sizeof(EFI_IFR_NUMERIC), 1),  # EFI_IFR_NUMERIC_OP
    OpNode(ctypes.sizeof(EFI_IFR_PASSWORD), 1),  # EFI_IFR_PASSWORD_OP
    OpNode(ctypes.sizeof(EFI_IFR_ONE_OF_OPTION),
           0),  # EFI_IFR_ONE_OF_OPTION_OP
    OpNode(ctypes.sizeof(EFI_IFR_SUPPRESS_IF),
           1),  # EFI_IFR_SUPPRESS_IF - 0x0A
    OpNode(ctypes.sizeof(EFI_IFR_LOCKED), 0),  # EFI_IFR_LOCKED_OP
    OpNode(ctypes.sizeof(EFI_IFR_ACTION), 1),  # EFI_IFR_ACTION_OP
    OpNode(ctypes.sizeof(EFI_IFR_RESET_BUTTON), 1),  # EFI_IFR_RESET_BUTTON_OP
    OpNode(ctypes.sizeof(EFI_IFR_FORM_SET), 1),  # EFI_IFR_FORM_SET_OP -0xE
    OpNode(ctypes.sizeof(EFI_IFR_REF), 0),  # EFI_IFR_REF_OP
    OpNode(ctypes.sizeof(EFI_IFR_NO_SUBMIT_IF),
           1),  # EFI_IFR_NO_SUBMIT_IF_OP -0x10
    OpNode(ctypes.sizeof(EFI_IFR_INCONSISTENT_IF),
           1),  # EFI_IFR_INCONSISTENT_IF_OP
    OpNode(ctypes.sizeof(EFI_IFR_EQ_ID_VAL), 0),  # EFI_IFR_EQ_ID_VAL_OP
    OpNode(ctypes.sizeof(EFI_IFR_EQ_ID_ID), 0),  # EFI_IFR_EQ_ID_ID_OP
    OpNode(ctypes.sizeof(EFI_IFR_EQ_ID_VAL_LIST),
           0),  # EFI_IFR_EQ_ID_LIST_OP - 0x14
    OpNode(ctypes.sizeof(EFI_IFR_AND), 0),  # EFI_IFR_AND_OP
    OpNode(ctypes.sizeof(EFI_IFR_OR), 0),  # EFI_IFR_OR_OP
    OpNode(ctypes.sizeof(EFI_IFR_NOT), 0),  # EFI_IFR_NOT_OP
    OpNode(ctypes.sizeof(EFI_IFR_RULE), 1),  # EFI_IFR_RULE_OP
    OpNode(ctypes.sizeof(EFI_IFR_GRAY_OUT_IF),
           1),  # EFI_IFR_GRAYOUT_IF_OP - 0x19
    OpNode(ctypes.sizeof(EFI_IFR_DATE), 1),  # EFI_IFR_DATE_OP
    OpNode(ctypes.sizeof(EFI_IFR_TIME), 1),  # EFI_IFR_TIME_OP
    OpNode(ctypes.sizeof(EFI_IFR_STRING), 1),  # EFI_IFR_STRING_OP
    OpNode(ctypes.sizeof(EFI_IFR_REFRESH), 0),  # EFI_IFR_REFRESH_OP
    OpNode(ctypes.sizeof(EFI_IFR_DISABLE_IF),
           1),  # EFI_IFR_DISABLE_IF_OP - 0x1E
    OpNode(0, 0),  # 0x1F
    OpNode(ctypes.sizeof(EFI_IFR_TO_LOWER), 0),  # EFI_IFR_TO_LOWER_OP - 0x20
    OpNode(ctypes.sizeof(EFI_IFR_TO_UPPER), 0),  # EFI_IFR_TO_UPPER_OP - 0x21
    OpNode(ctypes.sizeof(EFI_IFR_MAP), 1),  # EFI_IFR_MAP - 0x22
    OpNode(ctypes.sizeof(EFI_IFR_ORDERED_LIST),
           1),  # EFI_IFR_ORDERED_LIST_OP - 0x23
    OpNode(ctypes.sizeof(EFI_IFR_VARSTORE), 0),  # EFI_IFR_VARSTORE_OP
    OpNode(ctypes.sizeof(EFI_IFR_VARSTORE_NAME_VALUE),
           0),  # EFI_IFR_VARSTORE_NAME_VALUE_OP
    OpNode(ctypes.sizeof(EFI_IFR_VARSTORE_EFI), 0),  # EFI_IFR_VARSTORE_EFI_OP
    OpNode(ctypes.sizeof(EFI_IFR_VARSTORE_DEVICE),
           1),  # EFI_IFR_VARSTORE_DEVICE_OP
    OpNode(ctypes.sizeof(EFI_IFR_VERSION), 0),  # EFI_IFR_VERSION_OP - 0x28
    OpNode(ctypes.sizeof(EFI_IFR_END), 0),  # EFI_IFR_END_OP
    OpNode(ctypes.sizeof(EFI_IFR_MATCH), 0),  # EFI_IFR_MATCH_OP - 0x2A
    OpNode(ctypes.sizeof(EFI_IFR_GET), 0),  # EFI_IFR_GET - 0x2B
    OpNode(ctypes.sizeof(EFI_IFR_SET), 0),  # EFI_IFR_SET - 0x2C
    OpNode(ctypes.sizeof(EFI_IFR_READ), 0),  # EFI_IFR_READ - 0x2D
    OpNode(ctypes.sizeof(EFI_IFR_WRITE), 0),  # EFI_IFR_WRITE - 0x2E
    OpNode(ctypes.sizeof(EFI_IFR_EQUAL), 0),  # EFI_IFR_EQUAL_OP - 0x2F
    OpNode(ctypes.sizeof(EFI_IFR_NOT_EQUAL), 0),  # EFI_IFR_NOT_EQUAL_OP
    OpNode(ctypes.sizeof(EFI_IFR_GREATER_THAN), 0),  # EFI_IFR_GREATER_THAN_OP
    OpNode(ctypes.sizeof(EFI_IFR_GREATER_EQUAL),
           0),  # EFI_IFR_GREATER_EQUAL_OP
    OpNode(ctypes.sizeof(EFI_IFR_LESS_THAN), 0),  # EFI_IFR_LESS_THAN_OP
    OpNode(ctypes.sizeof(EFI_IFR_LESS_EQUAL),
           0),  # EFI_IFR_LESS_EQUAL_OP - 0x34
    OpNode(ctypes.sizeof(EFI_IFR_BITWISE_AND), 0),  # EFI_IFR_BITWISE_AND_OP
    OpNode(ctypes.sizeof(EFI_IFR_BITWISE_OR), 0),  # EFI_IFR_BITWISE_OR_OP
    OpNode(ctypes.sizeof(EFI_IFR_BITWISE_NOT), 0),  # EFI_IFR_BITWISE_NOT_OP
    OpNode(ctypes.sizeof(EFI_IFR_SHIFT_LEFT), 0),  # EFI_IFR_SHIFT_LEFT_OP
    OpNode(ctypes.sizeof(EFI_IFR_SHIFT_RIGHT), 0),  # EFI_IFR_SHIFT_RIGHT_OP
    OpNode(ctypes.sizeof(EFI_IFR_ADD), 0),  # EFI_IFR_ADD_OP - 0x3A
    OpNode(ctypes.sizeof(EFI_IFR_SUBTRACT), 0),  # EFI_IFR_SUBTRACT_OP
    OpNode(ctypes.sizeof(EFI_IFR_MULTIPLY), 0),  # EFI_IFR_MULTIPLY_OP
    OpNode(ctypes.sizeof(EFI_IFR_DIVIDE), 0),  # EFI_IFR_DIVIDE_OP
    OpNode(ctypes.sizeof(EFI_IFR_MODULO), 0),  # EFI_IFR_MODULO_OP - 0x3E
    OpNode(ctypes.sizeof(EFI_IFR_RULE_REF), 0),  # EFI_IFR_RULE_REF_OP
    OpNode(ctypes.sizeof(EFI_IFR_QUESTION_REF1),
           0),  # EFI_IFR_QUESTION_REF1_OP
    OpNode(ctypes.sizeof(EFI_IFR_QUESTION_REF2),
           0),  # EFI_IFR_QUESTION_REF2_OP - 0x41
    OpNode(ctypes.sizeof(EFI_IFR_UINT8), 0),  # EFI_IFR_UINT8
    OpNode(ctypes.sizeof(EFI_IFR_UINT16), 0),  # EFI_IFR_UINT16
    OpNode(ctypes.sizeof(EFI_IFR_UINT32), 0),  # EFI_IFR_UINT32
    OpNode(ctypes.sizeof(EFI_IFR_UINT64), 0),  # EFI_IFR_UTNT64
    OpNode(ctypes.sizeof(EFI_IFR_TRUE), 0),  # EFI_IFR_TRUE_OP - 0x46
    OpNode(ctypes.sizeof(EFI_IFR_FALSE), 0),  # EFI_IFR_FALSE_OP
    OpNode(ctypes.sizeof(EFI_IFR_TO_UINT), 0),  # EFI_IFR_TO_UINT_OP
    OpNode(ctypes.sizeof(EFI_IFR_TO_STRING), 0),  # EFI_IFR_TO_STRING_OP
    OpNode(ctypes.sizeof(EFI_IFR_TO_BOOLEAN), 0),  # EFI_IFR_TO_BOOLEAN_OP
    OpNode(ctypes.sizeof(EFI_IFR_MID), 0),  # EFI_IFR_MID_OP
    OpNode(ctypes.sizeof(EFI_IFR_FIND), 0),  # EFI_IFR_FIND_OP
    OpNode(ctypes.sizeof(EFI_IFR_TOKEN), 0),  # EFI_IFR_TOKEN_OP
    OpNode(ctypes.sizeof(EFI_IFR_STRING_REF1),
           0),  # EFI_IFR_STRING_REF1_OP - 0x4E
    OpNode(ctypes.sizeof(EFI_IFR_STRING_REF2), 0),  # EFI_IFR_STRING_REF2_OP
    OpNode(ctypes.sizeof(EFI_IFR_CONDITIONAL), 0),  # EFI_IFR_CONDITIONAL_OP
    OpNode(ctypes.sizeof(EFI_IFR_QUESTION_REF3),
           0),  # EFI_IFR_QUESTION_REF3_OP
    OpNode(ctypes.sizeof(EFI_IFR_ZERO), 0),  # EFI_IFR_ZERO_OP
    OpNode(ctypes.sizeof(EFI_IFR_ONE), 0),  # EFI_IFR_ONE_OP
    OpNode(ctypes.sizeof(EFI_IFR_ONES), 0),  # EFI_IFR_ONES_OP
    OpNode(ctypes.sizeof(EFI_IFR_UNDEFINED), 0),  # EFI_IFR_UNDEFINED_OP
    OpNode(ctypes.sizeof(EFI_IFR_LENGTH), 0),  # EFI_IFR_LENGTH_OP
    OpNode(ctypes.sizeof(EFI_IFR_DUP), 0),  # EFI_IFR_DUP_OP - 0x57
    OpNode(ctypes.sizeof(EFI_IFR_THIS), 0),  # EFI_IFR_THIS_OP
    OpNode(ctypes.sizeof(EFI_IFR_SPAN), 0),  # EFI_IFR_SPAN_OP
    OpNode(ctypes.sizeof(EFI_IFR_VALUE), 1),  # EFI_IFR_VALUE_OP
    OpNode(ctypes.sizeof(EFI_IFR_DEFAULT), 0),  # EFI_IFR_DEFAULT_OP
    OpNode(ctypes.sizeof(EFI_IFR_DEFAULTSTORE),
           0),  # EFI_IFR_DEFAULTSTORE_OP - 0x5C
    OpNode(ctypes.sizeof(EFI_IFR_FORM_MAP), 1),  # EFI_IFR_FORM_MAP_OP - 0x5D
    OpNode(ctypes.sizeof(EFI_IFR_CATENATE), 0),  # EFI_IFR_CATENATE_OP
    OpNode(ctypes.sizeof(EFI_IFR_GUID), 0),  # EFI_IFR_GUID_OP
    OpNode(ctypes.sizeof(EFI_IFR_SECURITY), 0),  # EFI_IFR_SECURITY_OP - 0x60
    OpNode(ctypes.sizeof(EFI_IFR_MODAL_TAG), 0),  # EFI_IFR_MODAL_TAG_OP - 0x61
    OpNode(ctypes.sizeof(EFI_IFR_REFRESH_ID),
           0),  # EFI_IFR_REFRESH_ID_OP - 0x62
    OpNode(ctypes.sizeof(EFI_IFR_WARNING_IF),
           1),  # EFI_IFR_WARNING_IF_OP - 0x63
    OpNode(ctypes.sizeof(EFI_IFR_MATCH2), 0)
]

class OpBufferNode():

    def __init__(self, Buffer=None, Next=None):
        self.Buffer = Buffer
        self.Next = Next

class PACKAGE_DATA():

    def __init__(self, Bu) -> None:
        #self.Buffer = Buffer
        pass

class ASSIGN_FLAG(Enum) :
    PENDING = 1
    ASSIGNED = 2

class SPendingAssign():

    def __init__(self, Key, ValAddr, LineNo, Msg, Type=0):
        self.Key = Key
        self.Addr = ValAddr
        self.Flag = ASSIGN_FLAG.PENDING
        self.LineNo = LineNo
        self.Msg = Msg
        self.Type = Type
        self.Next = None

    def AssignValue(self, Val):
        if self.Type == 1:
            self.Addr.QuestionId1 = Val
        elif self.Type == 2:
            self.Addr.QuestionId2 = Val
        else:
            self.Addr.QuestionId = Val

        self.Flag = ASSIGN_FLAG.ASSIGNED

class InsertOpNode():
    def __init__(self, Data, OpCode):
        self.Data = Data
        self.OpCode = OpCode
class FormPkg():

    def __init__(self):
        self.PkgLength = 0
        self.Offset = 0
        self.__PendingAssignList = None

    def BuildPkgHdr(self):
        PkgHdr = EFI_HII_PACKAGE_HEADER()
        PkgHdr.Type = EFI_HII_PACKAGE_FORM
        PkgHdr.Length = self.PkgLength + sizeof(EFI_HII_PACKAGE_HEADER)
        return PkgHdr

    def BuildPkg(self, Root):
        if Root.OpCode != None:
            self.PkgLength += Root.Data.GetInfo().Header.Length
            Root.Offset = gFormPkg.Offset
            self.Offset += Root.Data.GetInfo().Header.Length
        if Root.Child != []:
            for ChildNode in Root.Child:
                self.BuildPkg(ChildNode)

    # Get data from ctypes to bytes.
    def StructToStream(self, s) -> bytes:
        Length = sizeof(s)
        P = cast(pointer(s), POINTER(c_char * Length))
        return P.contents.raw


    def AssignPending(self, Key, VarAddr, LineNo, Msg, Type=0):
        pNew = SPendingAssign(Key, VarAddr, LineNo, Msg, Type)
        if pNew == None:
            return VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES
        pNew.Next = self.__PendingAssignList
        self.__PendingAssignList = pNew
        return VfrReturnCode.VFR_RETURN_SUCCESS

    def DoPendingAssign(self, Key, Val):
        if Key == None or Val == None:
            return
        pNode = self.__PendingAssignList
        while pNode != None:
            if pNode.Key == Key:
                pNode.AssignValue(Val)
            pNode = pNode.Next

    def HavePendingUnassigned(self):
        pNode = self.__PendingAssignList
        while pNode != None:
            if pNode.Flag == ASSIGN_FLAG.PENDING:
                return True
            pNode = pNode.Next

        return False

    def PendingAssignPrintAll(self):
        pNode = self.__PendingAssignList
        while pNode != None:
            if pNode.Flag == ASSIGN_FLAG.PENDING:
                gVfrErrorHandle.PrintMsg(pNode.LineNo, 'Error', pNode.Msg, pNode.Key)
            pNode = pNode.Next

    def DeclarePendingQuestion(self, lVfrVarDataTypeDB: VfrVarDataTypeDB, lVfrDataStorage: VfrDataStorage, lVfrQuestionDB: VfrQuestionDB, LineNo):
        # Declare all questions as Numeric in DisableIf True
        ReturnList  = []
        GuidObj = None
        DIObj = IfrDisableIf()
        DIObj.SetLineNo(LineNo)
        ReturnList.append(InsertOpNode(DIObj, EFI_IFR_DISABLE_IF_OP))
        # TrueOpcode
        TObj = IfrTrue(LineNo)
        ReturnList.append(InsertOpNode(TObj, EFI_IFR_TRUE_OP))
        pNode = self.__PendingAssignList
        while pNode != None:
            if pNode.Flag == ASSIGN_FLAG.PENDING:
                Info = EFI_VARSTORE_INFO()
                VarStr = pNode.Key
                QId, ReturnCode = lVfrQuestionDB.RegisterQuestion(None, VarStr, EFI_QUESTION_ID_INVALID, gFormPkg)
                if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                    gVfrErrorHandle.HandleError(ReturnCode, pNode.LineNo, pNode.Key)
                    return ReturnList, ReturnCode
                #ifdef VFREXP_DEBUG
                #printf("Undefined Question name is %s and Id is 0x%x\n", VarStr, QId);
                #endif
                # Get Question Info, framework vfr VarName == StructName
                ArrayIdx, s, FName, ReturnCode = lVfrVarDataTypeDB.ExtractFieldNameAndArrary(VarStr, 0)
                if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                    gVfrErrorHandle.PrintMsg(pNode.LineNo, 'Error', 'Var string is not the valid C variable', pNode.Key)
                    return ReturnList, ReturnCode

                # Get VarStoreType
                Info.VarStoreId, ReturnCode = lVfrDataStorage.GetVarStoreId(FName)

                if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                    gVfrErrorHandle.PrintMsg (pNode.LineNo, "Error", "Var Store Type is not defined", FName)
                    return ReturnList, ReturnCode

                VarStoreType = lVfrDataStorage.GetVarStoreType(Info.VarStoreId)
                if s == len(VarStr) and ArrayIdx != INVALID_ARRAY_INDEX:
                    ReturnCode = lVfrDataStorage.GetNameVarStoreInfo(Info, ArrayIdx)
                else:
                    if VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_EFI:
                        ReturnCode = lVfrDataStorage.GetEfiVarStoreInfo(Info)
                    elif VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER or VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER_BITS:
                        VarStr = pNode.Key
                        SName, ReturnCode = lVfrDataStorage.GetBufferVarStoreDataTypeName(Info.VarStoreId)
                        if ReturnCode == VfrReturnCode.VFR_RETURN_SUCCESS:
                            NewStr = SName + '.' + VarStr[s:]
                            Info.Info.VarOffset, Info.VarType, Info.VarTotalSize, Info.IsBitVar, ReturnCode = lVfrVarDataTypeDB.GetDataFieldInfo(NewStr)
                    else:
                        ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED

                if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                    gVfrErrorHandle.HandleError(ReturnCode, pNode.LineNo, pNode.Key)
                # If the storage is bit fields, create Guid opcode to wrap the numeric opcode.
                if Info.IsBitVar:
                    GuidObj = IfrGuid(0)
                    GuidObj.SetGuid(EDKII_IFR_BIT_VARSTORE_GUID)
                    GuidObj.SetLineNo(LineNo)
                    ReturnList.append(InsertOpNode(GuidObj, EFI_IFR_GUID_OP))


                if Info.IsBitVar:
                    Info.VarType = EFI_IFR_TYPE_NUM_SIZE_32

                # Numeric doesn't support BOOLEAN data type.
                # BOOLEAN type has the same data size to UINT8.
                elif Info.VarType == EFI_IFR_TYPE_BOOLEAN:
                    Info.VarType = EFI_IFR_TYPE_NUM_SIZE_8

                CNObj = IfrNumeric(Info.VarType)
                CNObj.SetLineNo(LineNo)
                CNObj.SetPrompt(0x0)
                CNObj.SetHelp(0x0)
                CNObj.SetQuestionId(QId)
                CNObj.SetVarStoreInfo(Info)
                ReturnList.append(InsertOpNode(CNObj, EFI_IFR_GUID_OP))

                if Info.IsBitVar:
                    MaxValue = 1 << Info.VarTotalSize - 1
                    CNObj.SetMinMaxStepData(0, MaxValue, 0)
                    LFlags = (EDKII_IFR_NUMERIC_SIZE_BIT & Info.VarTotalSize)
                    CNObj.SetFlagsForBitField(0, LFlags)
                else:
                    CNObj.SetFlags(0, Info.VarType)
                    CNObj.SetMinMaxStepData(0, -1, 0)

                if VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_EFI:
                    VNObj = IfrVarEqName(QId, Info.Info.VarName)
                    VNObj.SetLineNo(LineNo)
                    ReturnList.append(InsertOpNode(VNObj, EFI_IFR_GUID_OP))

                CEObj = IfrEnd()
                CEObj.SetLineNo(LineNo)
                ReturnList.append(InsertOpNode(CEObj, EFI_IFR_END_OP))

                if GuidObj != None:
                    CEObjGuid = IfrEnd()
                    CEObjGuid.SetLineNo(LineNo)
                    ReturnList.append(InsertOpNode(CEObjGuid, EFI_IFR_END_OP))
                    GuidObj.SetScope(1)
                    GuidObj = None
            pNode = pNode.Next

        SEObj = IfrEnd()
        SEObj.SetLineNo(LineNo)
        ReturnList.append(InsertOpNode(SEObj, EFI_IFR_END_OP))
        return ReturnList, VfrReturnCode.VFR_RETURN_SUCCESS


gFormPkg = FormPkg()
gCreateOp = True
BYTES_PRE_LINE = 0x10

gIfrObjPrintDebugTable = [
    "EFI_IFR_INVALID",
    "EFI_IFR_FORM",
    "EFI_IFR_SUBTITLE",
    "EFI_IFR_TEXT",
    "EFI_IFR_IMAGE",
    "EFI_IFR_ONE_OF",
    "EFI_IFR_CHECKBOX",
    "EFI_IFR_NUMERIC",
    "EFI_IFR_PASSWORD",
    "EFI_IFR_ONE_OF_OPTION",
    "EFI_IFR_SUPPRESS_IF",
    "EFI_IFR_LOCKED",
    "EFI_IFR_ACTION",
    "EFI_IFR_RESET_BUTTON",
    "EFI_IFR_FORM_SET",
    "EFI_IFR_REF",
    "EFI_IFR_NO_SUBMIT_IF",
    "EFI_IFR_INCONSISTENT_IF",
    "EFI_IFR_EQ_ID_VAL",
    "EFI_IFR_EQ_ID_ID",
    "EFI_IFR_EQ_ID_LIST",
    "EFI_IFR_AND",
    "EFI_IFR_OR",
    "EFI_IFR_NOT",
    "EFI_IFR_RULE",
    "EFI_IFR_GRAY_OUT_IF",
    "EFI_IFR_DATE",
    "EFI_IFR_TIME",
    "EFI_IFR_STRING",
    "EFI_IFR_REFRESH",
    "EFI_IFR_DISABLE_IF",
    "EFI_IFR_INVALID",
    "EFI_IFR_TO_LOWER",
    "EFI_IFR_TO_UPPER",
    "EFI_IFR_MAP",
    "EFI_IFR_ORDERED_LIST",
    "EFI_IFR_VARSTORE",
    "EFI_IFR_VARSTORE_NAME_VALUE",
    "EFI_IFR_VARSTORE_EFI",
    "EFI_IFR_VARSTORE_DEVICE",
    "EFI_IFR_VERSION",
    "EFI_IFR_END",
    "EFI_IFR_MATCH",
    "EFI_IFR_GET",
    "EFI_IFR_SET",
    "EFI_IFR_READ",
    "EFI_IFR_WRITE",
    "EFI_IFR_EQUAL",
    "EFI_IFR_NOT_EQUAL",
    "EFI_IFR_GREATER_THAN",
    "EFI_IFR_GREATER_EQUAL",
    "EFI_IFR_LESS_THAN",
    "EFI_IFR_LESS_EQUAL",
    "EFI_IFR_BITWISE_AND",
    "EFI_IFR_BITWISE_OR",
    "EFI_IFR_BITWISE_NOT",
    "EFI_IFR_SHIFT_LEFT",
    "EFI_IFR_SHIFT_RIGHT",
    "EFI_IFR_ADD",
    "EFI_IFR_SUBTRACT",
    "EFI_IFR_MULTIPLY",
    "EFI_IFR_DIVIDE",
    "EFI_IFR_MODULO",
    "EFI_IFR_RULE_REF",
    "EFI_IFR_QUESTION_REF1",
    "EFI_IFR_QUESTION_REF2",
    "EFI_IFR_UINT8",
    "EFI_IFR_UINT16",
    "EFI_IFR_UINT32",
    "EFI_IFR_UINT64",
    "EFI_IFR_TRUE",
    "EFI_IFR_FALSE",
    "EFI_IFR_TO_UINT",
    "EFI_IFR_TO_STRING",
    "EFI_IFR_TO_BOOLEAN",
    "EFI_IFR_MID",
    "EFI_IFR_FIND",
    "EFI_IFR_TOKEN",
    "EFI_IFR_STRING_REF1",
    "EFI_IFR_STRING_REF2",
    "EFI_IFR_CONDITIONAL",
    "EFI_IFR_QUESTION_REF3",
    "EFI_IFR_ZERO",
    "EFI_IFR_ONE",
    "EFI_IFR_ONES",
    "EFI_IFR_UNDEFINED",
    "EFI_IFR_LENGTH",
    "EFI_IFR_DUP",
    "EFI_IFR_THIS",
    "EFI_IFR_SPAN",
    "EFI_IFR_VALUE",
    "EFI_IFR_DEFAULT",
    "EFI_IFR_DEFAULTSTORE",
    "EFI_IFR_FORM_MAP",
    "EFI_IFR_CATENATE",
    "EFI_IFR_GUID",
    "EFI_IFR_SECURITY",
    "EFI_IFR_MODAL_TAG",
    "EFI_IFR_REFRESH_ID",
    "EFI_IFR_WARNING_IF",
    "EFI_IFR_MATCH2",
]

gScopeCount = 0
gIsOrderedList = False
gIsStringOp = False
gCurrentMinMaxData = None

class IfrLine():

    def __init__(self, LineNo=0):
        self.__LineNo = LineNo

    def SetLineNo(self, LineNo):
        self.__LineNo = LineNo

    def GetLineNo(self):
        return self.__LineNo
class IfrOpHeader():

    def __init__(self, OpHeader: EFI_IFR_OP_HEADER, OpCode=None, Length=0):
        self.__OpHeader = OpHeader
        if OpCode != None:
            self.__OpHeader.OpCode = OpCode

            self.__OpHeader.Length = gOpcodeSizesScopeTable[
                OpCode].Size if Length == 0 else Length
            self.__OpHeader.Scope = 1 if (
                gOpcodeSizesScopeTable[OpCode].Scope + gScopeCount > 0) else 0

    def GetLength(self):
        return self.__OpHeader.Length

    def SetScope(self, Scope):
        self.__OpHeader.Scope = Scope

    def UpdateHeader(self, Header):
        self.__OpHeader = Header

    def IncLength(self, Size):
        self.__OpHeader.Length += Size

    def DecLength(self, Size):
        self.__OpHeader.Length -= Size

    def AdjustLength(self, BeforeSize, AfterSize):
        self.__OpHeader.Length -= BeforeSize
        self.__OpHeader.Length += AfterSize
        self.__OpHeader.Length += 1

    def GetOpCode(self):
        return self.__OpHeader.OpCode


class IfrStatementHeader():

    def __init__(self, sHeader: EFI_IFR_STATEMENT_HEADER):
        self.__sHeader = sHeader
        self.__sHeader.Help = EFI_STRING_ID_INVALID
        self.__sHeader.Prompt = EFI_STRING_ID_INVALID

    def GetStatementHeader(self):
        return self.__sHeader

    def SetPrompt(self, Prompt):
        self.__sHeader.Prompt = Prompt

    def SetHelp(self, Help):
        self.__sHeader.Help = Help


class IfrMinMaxStepData():

    def __init__(self, MinMaxStepData, NumericOpcode=False):
        self.__MinMaxStepData = MinMaxStepData
        self.__MinMaxStepData.MinValue = 0
        self.__MinMaxStepData.MaxValue = 0
        self.__MinMaxStepData.Step = 0
        self.__ValueIsSet = False
        self.__IsNumeric = NumericOpcode

    def SetMinMaxStepData(self, MinValue, MaxValue, Step):

        if self.__ValueIsSet == False:
            self.__MinMaxStepData.MinValue = MinValue
            self.__MinMaxStepData.MaxValue = MaxValue
            self.__MinMaxStepData.Step = Step

            self.__ValueIsSet = True
        else:
            if MinValue < self.__MinMaxStepData.MinValue:
                self.__MinMaxStepData.MinValue = MinValue
            if MaxValue > self.__MinMaxStepData.MaxValue:
                self.__MinMaxStepData.MaxValue = MaxValue
            self.__MinMaxStepData.Step = Step

    def IsNumericOpcode(self):
        return self.__IsNumeric

    def UpdateIfrMinMaxStepData(self, MinMaxStepData):
        self.__MinMaxStepData = MinMaxStepData

    def GetMinData(self):
        return self.__MinMaxStepData.MinValue

    def GetMaxData(self):
        return self.__MinMaxStepData.MaxValue

    def GetStepData(self):
        return self.__MinMaxStepData.Step


class IfrFormSet(IfrLine, IfrOpHeader):

    def __init__(self, Size):
        self.__FormSet = EFI_IFR_FORM_SET()
        self.__ClassGuid = []
        IfrOpHeader.__init__(self, self.__FormSet.Header, EFI_IFR_FORM_SET_OP,
                              Size)
        self.__FormSet.Help = EFI_STRING_ID_INVALID
        self.__FormSet.FormSetTitle = EFI_STRING_ID_INVALID
        self.__FormSet.Flags = 0
        self.__FormSet.Guid = EFI_GUID(0, 0, 0,
                                       GuidArray(0, 0, 0, 0, 0, 0, 0, 0))

    def SetGuid(self, Guid):
        self.__FormSet.Guid = Guid

    def GetGuid(self):
        return self.__FormSet.Guid

    def SetFormSetTitle(self, FormSetTitle):
        self.__FormSet.FormSetTitle = FormSetTitle

    def GetFormSetTitle(self):
        return self.__FormSet.FormSetTitle

    def SetHelp(self, Help):
        self.__FormSet.Help = Help

    def GetHelp(self):
        return self.__FormSet.Help

    def SetClassGuid(self, Guid):
        self.__ClassGuid.append(Guid)
        self.__FormSet.Flags += 1

    def GetClassGuid(self):
        return self.__ClassGuid

    def GetFlags(self):
        return self.__FormSet.Flags

    def GetInfo(self):
        return self.__FormSet

class IfrOneOfOption(IfrLine, IfrOpHeader):

    def __init__(self, ValueType, ValueList):
        Nums = len(ValueList)
        self.__OneOfOption = Refine_EFI_IFR_ONE_OF_OPTION(ValueType, Nums)
        IfrOpHeader.__init__(self, self.__OneOfOption.Header, EFI_IFR_ONE_OF_OPTION_OP, sizeof(self.__OneOfOption))
        self.__OneOfOption.Flags = 0
        self.__OneOfOption.Option = EFI_STRING_ID_INVALID
        self.__OneOfOption.Type = EFI_IFR_TYPE_OTHER
        self.__ValueType = ValueType
        if ValueList != []:
            ArrayType = TypeDict[ValueType] * Nums
            ValueArray = ArrayType()
            for i in range(0, len(ValueList)):
                ValueArray[i] = ValueList[i]
            self.__OneOfOption.Value = ValueArray

    def SetOption(self, Option):
        self.__OneOfOption.Option = Option

    def SetType(self, Type):
        self.__OneOfOption.Type = Type

    def GetValueType(self):
        return self.__ValueType

    def SetValue(self, ValueList):
        ArrayType = TypeDict[self.__ValueType] * (len(ValueList))
        ValueArray = ArrayType()
        for i in range(0, len(ValueList)):
            ValueArray[i] = ValueList[i]
        self.__OneOfOption.Value = ValueArray

    def GetFlags(self):
        return self.__OneOfOption.Flags

    def SetFlags(self, LFlags):

        self.__OneOfOption.Flags = 0
        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, EFI_IFR_OPTION_DEFAULT)
        if Ret:
            self.__OneOfOption.Flags |= EFI_IFR_OPTION_DEFAULT

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, EFI_IFR_OPTION_DEFAULT_MFG)
        if Ret:
            self.__OneOfOption.Flags |= EFI_IFR_OPTION_DEFAULT_MFG

        if LFlags == EFI_IFR_TYPE_NUM_SIZE_8:
            LFlags = _FLAG_CLEAR(LFlags, EFI_IFR_TYPE_NUM_SIZE_8)
            self.__OneOfOption.Flags |= EFI_IFR_TYPE_NUM_SIZE_8

        elif LFlags == EFI_IFR_TYPE_NUM_SIZE_16:
            LFlags = _FLAG_CLEAR(LFlags, EFI_IFR_TYPE_NUM_SIZE_16)
            self.__OneOfOption.Flags |= EFI_IFR_TYPE_NUM_SIZE_16

        elif LFlags == EFI_IFR_TYPE_NUM_SIZE_32:
            LFlags = _FLAG_CLEAR(LFlags, EFI_IFR_TYPE_NUM_SIZE_32)
            self.__OneOfOption.Flags |= EFI_IFR_TYPE_NUM_SIZE_32

        elif LFlags == EFI_IFR_TYPE_NUM_SIZE_64:
            LFlags = _FLAG_CLEAR(LFlags, EFI_IFR_TYPE_NUM_SIZE_64)
            self.__OneOfOption.Flags |= EFI_IFR_TYPE_NUM_SIZE_64

        elif LFlags == EFI_IFR_TYPE_BOOLEAN:
            LFlags = _FLAG_CLEAR(LFlags, EFI_IFR_TYPE_BOOLEAN)
            self.__OneOfOption.Flags |= EFI_IFR_TYPE_BOOLEAN

        elif LFlags == EFI_IFR_TYPE_TIME:
            LFlags = _FLAG_CLEAR(LFlags, EFI_IFR_TYPE_TIME)
            self.__OneOfOption.Flags |= EFI_IFR_TYPE_TIME

        elif LFlags == EFI_IFR_TYPE_DATE:
            LFlags = _FLAG_CLEAR(LFlags, EFI_IFR_TYPE_DATE)
            self.__OneOfOption.Flags |= EFI_IFR_TYPE_DATE

        elif LFlags == EFI_IFR_TYPE_STRING:
            LFlags = _FLAG_CLEAR(LFlags, EFI_IFR_TYPE_STRING)
            self.__OneOfOption.Flags |= EFI_IFR_TYPE_STRING

        elif LFlags == EFI_IFR_TYPE_OTHER:
            LFlags = _FLAG_CLEAR(LFlags, EFI_IFR_TYPE_OTHER)
            self.__OneOfOption.Flags |= EFI_IFR_TYPE_OTHER

        return VfrReturnCode.VFR_RETURN_SUCCESS if LFlags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED

    def GetInfo(self):
        return self.__OneOfOption


class IfrOptionKey(IfrLine, IfrOpHeader):

    def __init__(self, QuestionId, Type, OptionValue, KeyValue):

        self.__OptionKey = Refine_EFI_IFR_GUID_OPTIONKEY(Type)
        IfrOpHeader.__init__(self, self.__OptionKey.Header, EFI_IFR_GUID_OP,
                              ctypes.sizeof(self.__OptionKey))
        self.__OptionKey.ExtendOpCode = EFI_IFR_EXTEND_OP_OPTIONKEY
        self.__OptionKey.Guid = EFI_IFR_FRAMEWORK_GUID
        self.__OptionKey.QuestionId = QuestionId
        self.__OptionKey.OptionValue = OptionValue
        self.__OptionKey.KeyValue = KeyValue

    def GetInfo(self):
        return self.__OptionKey


class IfrClass(IfrLine, IfrOpHeader):

    def __init__(self, ):
        self.__Class = EFI_IFR_GUID_CLASS()  # static guid
        IfrOpHeader.__init__(self, self.__Class.Header, EFI_IFR_GUID_OP,
                              ctypes.sizeof(EFI_IFR_GUID_CLASS))
        self.__Class.ExtendOpCode = EFI_IFR_EXTEND_OP_CLASS
        self.__Class.Guid = EFI_IFR_TIANO_GUID
        self.__Class.Class = EFI_NON_DEVICE_CLASS

    def SetClass(self, Class):
        self.__Class.Class = Class

    def GetInfo(self):
        return self.__Class


class IfrSubClass(IfrLine, IfrOpHeader):

    def __init__(self, ):
        self.__SubClass = EFI_IFR_GUID_SUBCLASS()  # static guid
        IfrOpHeader.__init__(self, self.__SubClass.Header, EFI_IFR_GUID_OP,
                              ctypes.sizeof(EFI_IFR_GUID_SUBCLASS))
        self.__SubClass.ExtendOpCode = EFI_IFR_EXTEND_OP_SUBCLASS
        self.__SubClass.Guid = EFI_IFR_TIANO_GUID
        self.__SubClass.SubClass = EFI_SETUP_APPLICATION_SUBCLASS

    def SetSubClass(self, SubClass):
        self.__SubClass.SubClass = SubClass

    def GetInfo(self):
        return self.__SubClass


class IfrDefaultStore(IfrLine, IfrOpHeader):

    def __init__(self, Name=None):
        self.__DefaultStore = EFI_IFR_DEFAULTSTORE()
        self.__DefaultStoreName = Name
        IfrOpHeader.__init__(self, self.__DefaultStore.Header,
                              EFI_IFR_DEFAULTSTORE_OP)
        self.__DefaultStore.DefaultName = EFI_STRING_ID_INVALID
        self.__DefaultStore.DefaultId = EFI_VARSTORE_ID_INVALID

    def SetDefaultName(self, DefaultName):
        self.__DefaultStore.DefaultName = DefaultName

    def SetDefaultStoreName(self, Name):
        self.__DefaultStoreName = Name

    def SetDefaultId(self, DefaultId):
        self.__DefaultStore.DefaultId = DefaultId

    def GetDefaultStore(self):
        return self.__DefaultStore

    def SetDefaultStore(self, DefaultStore: EFI_IFR_DEFAULTSTORE):
        self.__DefaultStore = DefaultStore
        IfrOpHeader.__init__(self, self.__DefaultStore.Header,
                              EFI_IFR_DEFAULTSTORE_OP)

    def GetDefaultId(self):
        return self.__DefaultStore.DefaultId

    def GetInfo(self):
        return self.__DefaultStore

    def GetName(self):
        return self.__DefaultStoreName

class IfrVarStore(IfrLine, IfrOpHeader):

    def __init__(self, StoreName):
        Nums = len(StoreName)
        self.__Varstore = Refine_EFI_IFR_VARSTORE(Nums)
        IfrOpHeader.__init__(self, self.__Varstore.Header,
                              EFI_IFR_VARSTORE_OP, sizeof(self.__Varstore) + 1)
        self.__Varstore.VarStoreId = EFI_VARSTORE_ID_INVALID
        self.__Varstore.Size = 0
        ArrayType = c_ubyte * (Nums)
        ValueArray = ArrayType()
        for i in range(0, Nums):

            ValueArray[i] = ord(StoreName[i])
        self.__Varstore.Name = ValueArray

    def SetGuid(self, Guid):
        self.__Varstore.Guid = Guid

    def SetSize(self, Size):
        self.__Varstore.Size = Size

    def SetVarStoreId(self, VarStoreId):
        self.__Varstore.VarStoreId = VarStoreId

    def GetInfo(self):
        return self.__Varstore


class IfrVarStoreEfi(IfrLine, IfrOpHeader):

    def __init__(self, StoreName):
        Nums = len(StoreName)
        self.__VarStoreEfi = Refine_EFI_IFR_VARSTORE_EFI(Nums)
        IfrOpHeader.__init__(self, self.__VarStoreEfi.Header,
                              EFI_IFR_VARSTORE_EFI_OP, sizeof(self.__VarStoreEfi) + 1)
        self.__VarStoreEfi.VarStoreId = EFI_VAROFFSET_INVALID
        self.__VarStoreEfi.Size = 0
        ArrayType = c_ubyte * (Nums)
        ValueArray = ArrayType()
        for i in range(0, Nums):
            ValueArray[i] = ord(StoreName[i])
        self.__VarStoreEfi.Name = ValueArray

    def SetGuid(self, Guid):
        self.__VarStoreEfi.Guid = Guid

    def SetSize(self, Size):
        self.__VarStoreEfi.Size = Size

    def SetVarStoreId(self, VarStoreId):
        self.__VarStoreEfi.VarStoreId = VarStoreId

    def SetAttributes(self, Attributes):
        self.__VarStoreEfi.Attributes = Attributes

    def GetInfo(self):
        return self.__VarStoreEfi


class IfrVarStoreNameValue(IfrLine, IfrOpHeader):

    def __init__(self):
        self.__VarStoreNameValue = EFI_IFR_VARSTORE_NAME_VALUE()
        IfrOpHeader.__init__(self, self.__VarStoreNameValue.Header,
                              EFI_IFR_VARSTORE_NAME_VALUE_OP)
        self.__VarStoreNameValue.VarStoreId = EFI_VAROFFSET_INVALID

    def SetGuid(self, Guid):
        self.__VarStoreNameValue.Guid = Guid

    def SetVarStoreId(self, VarStoreId):
        self.__VarStoreNameValue.VarStoreId = VarStoreId

    def GetInfo(self):
        return self.__VarStoreNameValue


EFI_BITS_PER_UINT32 = 1 << EFI_BITS_SHIFT_PER_UINT32
EFI_FORM_ID_MAX = 0xFFFF

EFI_FREE_FORM_ID_BITMAP_SIZE = int((EFI_FORM_ID_MAX + 1) / EFI_BITS_PER_UINT32)


class IfrFormId():

    FormIdBitMap = []
    for i in range(0, EFI_FREE_FORM_ID_BITMAP_SIZE):
        FormIdBitMap.append(0)

    @classmethod
    def CheckFormIdFree(cls, FormId):

        Index = int(FormId / EFI_BITS_PER_UINT32)
        Offset = FormId % EFI_BITS_PER_UINT32

        return (cls.FormIdBitMap[Index] & (0x80000000 >> Offset)) == 0

    @classmethod
    def MarkFormIdUsed(cls, FormId):

        Index = int(FormId / EFI_BITS_PER_UINT32)
        Offset = FormId % EFI_BITS_PER_UINT32
        cls.FormIdBitMap[Index] |= (0x80000000 >> Offset)


class IfrForm(IfrLine, IfrOpHeader):

    def __init__(self):
        self.__Form = EFI_IFR_FORM()
        IfrOpHeader.__init__(self, self.__Form.Header, EFI_IFR_FORM_OP)
        self.__Form.FormId = 0
        self.__Form.FormTitle = EFI_STRING_ID_INVALID

    def SetFormId(self, FormId):
        # FormId can't be 0.
        if FormId == 0:
            return VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
        if IfrFormId.CheckFormIdFree(FormId) == False:
            return VfrReturnCode.VFR_RETURN_FORMID_REDEFINED
        self.__Form.FormId = FormId
        IfrFormId.MarkFormIdUsed(FormId)
        return VfrReturnCode.VFR_RETURN_SUCCESS

    def SetFormTitle(self, FormTitle):
        self.__Form.FormTitle = FormTitle

    def GetInfo(self):
        return self.__Form


class IfrFormMap(IfrLine, IfrOpHeader):

    def __init__(self):
        self.__FormMap = EFI_IFR_FORM_MAP()
        self.__MethodMapList = []  # EFI_IFR_FORM_MAP_METHOD()
        IfrOpHeader.__init__(self, self.__FormMap.Header, EFI_IFR_FORM_MAP_OP)
        self.__FormMap.FormId = 0

    def SetFormId(self, FormId):
        if FormId == 0:
            return VfrReturnCode.VFR_RETURN_INVALID_PARAMETER

        if IfrFormId.CheckFormIdFree(FormId) == False:
            return VfrReturnCode.VFR_RETURN_FORMID_REDEFINED
        self.__FormMap.FormId = FormId
        IfrFormId.MarkFormIdUsed(FormId)
        return VfrReturnCode.VFR_RETURN_SUCCESS

    def SetFormMapMethod(self, MethodTitle, MethodGuid: EFI_GUID):
        MethodMap = EFI_IFR_FORM_MAP_METHOD()
        MethodMap.MethodTitle = MethodTitle
        MethodMap.MethodIdentifier = MethodGuid
        self.__MethodMapList.append(MethodMap)

    def GetInfo(self):
        return self.__FormMap

    def GetMethodMapList(self):
        return self.__MethodMapList


class IfrEnd(IfrLine, IfrOpHeader):

    def __init__(self):
        self.__End = EFI_IFR_END()
        IfrOpHeader.__init__(self, self.__End.Header, EFI_IFR_END_OP)

    def GetInfo(self):
        return self.__End


class IfrBanner(IfrLine, IfrOpHeader):

    def __init__(self, ):
        self.__Banner = EFI_IFR_GUID_BANNER()
        IfrOpHeader.__init__(self, self.__Banner.Header, EFI_IFR_GUID_OP,
                              ctypes.sizeof(EFI_IFR_GUID_BANNER))
        self.__Banner.ExtendOpCode = EFI_IFR_EXTEND_OP_BANNER
        self.__Banner.Guid = EFI_IFR_TIANO_GUID

    def SetTitle(self, StringId):
        self.__Banner.Title = StringId

    def SetLine(self, Line):
        self.__Banner.LineNumber = Line

    def SetAlign(self, Align):
        self.__Banner.Alignment = Align

    def GetInfo(self):
        return self.__Banner

class IfrVarEqName(IfrLine, IfrOpHeader):

    def __init__(self, QuestionId, NameId):
        self.__VarEqName = EFI_IFR_GUID_VAREQNAME()
        IfrOpHeader.__init__(self, self.__VarEqName.Header, EFI_IFR_GUID_OP,
                              ctypes.sizeof(EFI_IFR_GUID_VAREQNAME))
        self.__VarEqName.ExtendOpCode = EFI_IFR_EXTEND_OP_VAREQNAME
        self.__VarEqName.Guid = EFI_IFR_FRAMEWORK_GUID
        self.__VarEqName.QuestionId = QuestionId
        self.__VarEqName.NameId = NameId

    def GetInfo(self):
        return self.__VarEqName

class IfrTimeout(IfrLine, IfrOpHeader):

    def __init__(self, Timeout=0):
        self.__Timeout = EFI_IFR_GUID_TIMEOUT()
        IfrOpHeader.__init__(self, self.__Timeout.Header, EFI_IFR_GUID_OP,
                              ctypes.sizeof(EFI_IFR_GUID_TIMEOUT))
        self.__Timeout.ExtendOpCode = EFI_IFR_EXTEND_OP_TIMEOUT
        self.__Timeout.Guid = EFI_IFR_TIANO_GUID
        self.__Timeout.TimeOut = Timeout

    def SetTimeout(self, Timeout):
        self.__Timeout.TimeOut = Timeout

    def GetInfo(self):
        return self.__Timeout


class IfrLabel(IfrLine, IfrOpHeader):

    def __init__(self, ):
        self.__Label = EFI_IFR_GUID_LABEL()
        IfrOpHeader.__init__(self, self.__Label.Header, EFI_IFR_GUID_OP,
                              ctypes.sizeof(EFI_IFR_GUID_LABEL))
        self.__Label.ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL
        self.__Label.Guid = EFI_IFR_TIANO_GUID

    def SetNumber(self, Number):
        self.__Label.Number = Number

    def GetInfo(self):
        return self.__Label


class IfrRule(IfrLine, IfrOpHeader):

    def __init__(self, ):
        self.__Rule = EFI_IFR_RULE()
        IfrOpHeader.__init__(self, self.__Rule.Header, EFI_IFR_RULE_OP)
        self.__Rule.RuleId = EFI_RULE_ID_INVALID

    def SetRuleId(self, RuleId):
        self.__Rule.RuleId = RuleId

    def GetInfo(self):
        return self.__Rule


def _FLAG_TEST_AND_CLEAR(Flags, Mask):

    Ret = Flags & Mask
    Flags &= (~Mask)
    return Flags, Ret


def _FLAG_CLEAR(Flags, Mask):

    Flags &= (~Mask)
    return Flags


class IfrSubtitle(IfrLine, IfrOpHeader, IfrStatementHeader):

    def __init__(self, ):
        self.__Subtitle = EFI_IFR_SUBTITLE()

        IfrOpHeader.__init__(self, self.__Subtitle.Header,
                              EFI_IFR_SUBTITLE_OP)
        IfrStatementHeader.__init__(self, self.__Subtitle.Statement)

        self.__Subtitle.Flags = 0

    def SetFlags(self, Flags):
        Flags, Result = _FLAG_TEST_AND_CLEAR(Flags, EFI_IFR_FLAGS_HORIZONTAL)
        if Result:
            self.__Subtitle.Flags |= EFI_IFR_FLAGS_HORIZONTAL

        return VfrReturnCode.VFR_RETURN_SUCCESS if Flags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED

    def GetInfo(self):
        return self.__Subtitle


class IfrImage(IfrLine, IfrOpHeader):

    def __init__(self, ):
        self.__Image = EFI_IFR_IMAGE()
        IfrOpHeader.__init__(self, self.__Image.Header, EFI_IFR_IMAGE_OP)
        self.__Image.Id = EFI_IMAGE_ID_INVALID

    def SetImageId(self, ImageId):
        self.__Image.Id = ImageId

    def GetInfo(self):
        return self.__Image


class IfrLocked(IfrLine, IfrOpHeader):

    def __init__(self, ):
        self.__Lock = EFI_IFR_LOCKED()
        IfrOpHeader.__init__(self, self.__Lock.Header, EFI_IFR_LOCKED_OP)

    def GetInfo(self):
        return self.__Lock


class IfrModal(IfrLine, IfrOpHeader):

    def __init__(self, ):
        self.__Modal = EFI_IFR_MODAL_TAG()
        IfrOpHeader.__init__(self, self.__Modal.Header, EFI_IFR_MODAL_TAG_OP)

    def GetInfo(self):
        return self.__Modal


EFI_IFR_QUESTION_FLAG_DEFAULT = 0


class IfrQuestionHeader(IfrStatementHeader):

    def __init__(self, qHeader, Flags=EFI_IFR_QUESTION_FLAG_DEFAULT):

        self.__qHeader = qHeader
        IfrStatementHeader.__init__(self, self.__qHeader.Header)
        self.__qHeader.QuestionId = EFI_QUESTION_ID_INVALID
        self.__qHeader.VarStoreId = EFI_VARSTORE_ID_INVALID
        self.__qHeader.VarStoreInfo.VarName = EFI_STRING_ID_INVALID
        self.__qHeader.VarStoreInfo.VarOffset = EFI_VAROFFSET_INVALID
        self.__qHeader.Flags = Flags

    def GetQuestionId(self):
        return self.__qHeader.QuestionId

    def SetQuestionId(self, QuestionId):

        self.__qHeader.QuestionId = QuestionId

    def GetVarStoreId(self):
        return self.__qHeader.VarStoreId

    def SetVarStoreInfo(self, BaseInfo):

        self.__qHeader.VarStoreId = BaseInfo.VarStoreId
        self.__qHeader.VarStoreInfo.VarName = BaseInfo.Info.VarName
        self.__qHeader.VarStoreInfo.VarOffset = BaseInfo.Info.VarOffset

    def GetVarStoreInfo(self, Info):  # Bug

        Info.VarStoreId = self.__qHeader.VarStoreId
        Info.VarStoreInfo = self.__qHeader.VarStoreInfo
        return Info

    def GetQFlags(self):
        return self.__qHeader.Flags

    def SetFlags(self, Flags):

        Flags, Ret = _FLAG_TEST_AND_CLEAR(Flags, EFI_IFR_FLAG_READ_ONLY)
        if Ret:
            self.__qHeader.Flags |= EFI_IFR_FLAG_READ_ONLY

        Flags = _FLAG_CLEAR(Flags, 0x02)

        Flags, Ret = _FLAG_TEST_AND_CLEAR(Flags, EFI_IFR_FLAG_CALLBACK)
        if Ret:
            self.__qHeader.Flags |= EFI_IFR_FLAG_CALLBACK

        # ignore NVAccessFlag
        Flags = _FLAG_CLEAR(Flags, 0x08)

        Flags, Ret = _FLAG_TEST_AND_CLEAR(Flags, EFI_IFR_FLAG_RESET_REQUIRED)
        if Ret:
            self.__qHeader.Flags |= EFI_IFR_FLAG_RESET_REQUIRED

        Flags, Ret = _FLAG_TEST_AND_CLEAR(Flags,
                                          EFI_IFR_FLAG_RECONNECT_REQUIRED)
        if Ret:
            self.__qHeader.Flags |= EFI_IFR_FLAG_RECONNECT_REQUIRED

        # Set LateCheck Flag to compatible for framework flag
        # but it uses 0x20 as its flag, if in the future UEFI may take this flag

        Flags, Ret = _FLAG_TEST_AND_CLEAR(Flags, 0x20)
        if Ret:
            self.__qHeader.Flags |= 0x20

        Flags, Ret = _FLAG_TEST_AND_CLEAR(Flags, EFI_IFR_FLAG_OPTIONS_ONLY)
        if Ret:
            self.__qHeader.Flags |= EFI_IFR_FLAG_OPTIONS_ONLY

        return VfrReturnCode.VFR_RETURN_SUCCESS if Flags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED

    def UpdateIfrQuestionHeader(self, qHeader):
        self.__qHeader = qHeader


class IfrRef(IfrLine, IfrOpHeader, IfrQuestionHeader):

    def __init__(self, ):
        self.__Ref = EFI_IFR_REF()
        IfrOpHeader.__init__(self, self.__Ref.Header, EFI_IFR_REF_OP)
        IfrQuestionHeader.__init__(self, self.__Ref.Question)
        self.__Ref.FormId = 0

    def SetFormId(self, FormId):
        self.__Ref.FormId = FormId

    def GetInfo(self):
        return self.__Ref


class IfrRef2(IfrLine, IfrOpHeader, IfrQuestionHeader):

    def __init__(self, ):
        self.__Ref2 = EFI_IFR_REF2()
        IfrOpHeader.__init__(self, self.__Ref2.Header, EFI_IFR_REF_OP, sizeof(EFI_IFR_REF2))
        IfrQuestionHeader.__init__(self, self.__Ref2.Question)
        self.__Ref2.FormId = 0
        self.__Ref2.QuestionId = EFI_QUESTION_ID_INVALID

    def SetFormId(self, FormId):
        self.__Ref2.FormId = FormId

    def SetQId(self, QuestionId):

        self.__Ref2.QuestionId = QuestionId

    def GetInfo(self):
        return self.__Ref2


class IfrRef3(IfrLine, IfrOpHeader, IfrQuestionHeader):

    def __init__(self, ):
        self.__Ref3 = EFI_IFR_REF3()
        IfrOpHeader.__init__(self, self.__Ref3.Header, EFI_IFR_REF_OP, sizeof(EFI_IFR_REF3))
        IfrQuestionHeader.__init__(self, self.__Ref3.Question)
        self.__Ref3.FormId = 0
        self.__Ref3.QuestionId = EFI_QUESTION_ID_INVALID
        EFI_IFR_DEFAULT_GUID = EFI_GUID(0, 0, 0,
                                        GuidArray(0, 0, 0, 0, 0, 0, 0, 0))
        self.__Ref3.FormSetId = EFI_IFR_DEFAULT_GUID

    def SetFormId(self, FormId):
        self.__Ref3.FormId = FormId

    def SetQId(self, QuestionId):

        self.__Ref3.QuestionId = QuestionId

    def SetFormSetId(self, FormSetId):
        self.__Ref3.FormSetId = FormSetId

    def GetInfo(self):
        return self.__Ref3


class IfrRef4(IfrLine, IfrOpHeader, IfrQuestionHeader):

    def __init__(self, ):
        self.__Ref4 = EFI_IFR_REF4()
        IfrOpHeader.__init__(self, self.__Ref4.Header, EFI_IFR_REF_OP, sizeof(EFI_IFR_REF4))
        IfrQuestionHeader.__init__(self, self.__Ref4.Question)
        self.__Ref4.FormId = 0
        self.__Ref4.QuestionId = EFI_QUESTION_ID_INVALID
        EFI_IFR_DEFAULT_GUID = EFI_GUID(0, 0, 0,
                                        GuidArray(0, 0, 0, 0, 0, 0, 0, 0))
        self.__Ref4.FormSetId = EFI_IFR_DEFAULT_GUID
        self.__Ref4.DevicePath = EFI_STRING_ID_INVALID

    def SetFormId(self, FormId):
        self.__Ref4.FormId = FormId

    def SetQId(self, QuestionId):

        self.__Ref4.QuestionId = QuestionId

    def SetFormSetId(self, FormSetId):
        self.__Ref4.FormSetId = FormSetId

    def SetDevicePath(self, DevicePath):
        self.__Ref4.DevicePath = DevicePath

    def GetInfo(self):
        return self.__Ref4


class IfrRef5(IfrLine, IfrOpHeader, IfrQuestionHeader):

    def __init__(self, ):
        self.__Ref5 = EFI_IFR_REF5()
        IfrOpHeader.__init__(self, self.__Ref5.Header, EFI_IFR_REF_OP, sizeof(EFI_IFR_REF5))
        IfrQuestionHeader.__init__(self, self.__Ref5.Question)

    def GetInfo(self):
        return self.__Ref5


class IfrAction(IfrLine, IfrOpHeader, IfrQuestionHeader):

    def __init__(self, ):
        self.__Action = EFI_IFR_ACTION()
        IfrOpHeader.__init__(self, self.__Action.Header, EFI_IFR_ACTION_OP)
        IfrQuestionHeader.__init__(self, self.__Action.Question)
        self.__Action.QuestionConfig = EFI_STRING_ID_INVALID

    def SetQuestionConfig(self, QuestionConfig):
        self.__Action.QuestionConfig = QuestionConfig

    def GetInfo(self):
        return self.__Action


class IfrText(IfrLine, IfrOpHeader, IfrStatementHeader):

    def __init__(self, ):
        self.__Text = EFI_IFR_TEXT()
        IfrOpHeader.__init__(self, self.__Text.Header, EFI_IFR_TEXT_OP)
        IfrStatementHeader.__init__(self, self.__Text.Statement)
        self.__Text.TextTwo = EFI_STRING_ID_INVALID

    def SetTextTwo(self, StringId):
        self.__Text.TextTwo = StringId

    def GetInfo(self):
        return self.__Text


class IfrGuid(IfrLine, IfrOpHeader):

    def __init__(self, Size, Data=None):
        self.__Guid = EFI_IFR_GUID()
        self.__Data = Data # databuffer is saved here
        IfrOpHeader.__init__(self, self.__Guid.Header, EFI_IFR_GUID_OP,
                              ctypes.sizeof(EFI_IFR_GUID) + Size)
        EFI_IFR_DEFAULT_GUID = EFI_GUID(0, 0, 0,
                                        GuidArray(0, 0, 0, 0, 0, 0, 0, 0))
        self.__Guid.Guid = EFI_IFR_DEFAULT_GUID

    def SetGuid(self, Guid):
        self.__Guid.Guid = Guid

    def SetData(self, Data):
        self.__Data = Data

    def GetData(self):
        return self.__Data

    def GetInfo(self): #
        return self.__Guid


class IfrOrderedList(IfrLine, IfrOpHeader, IfrQuestionHeader):

    def __init__(self):
        self.__OrderedList = EFI_IFR_ORDERED_LIST()
        IfrOpHeader.__init__(self, self.__OrderedList.Header,
                              EFI_IFR_ORDERED_LIST_OP)
        IfrQuestionHeader.__init__(self, self.__OrderedList.Question)
        self.__OrderedList.MaxContainers = 0
        self.__OrderedList.Flags = 0

    def GetQuestion(self):
        return self

    def SetQHeaderFlags(self, Flags):
        IfrQuestionHeader.SetFlags(self, Flags)

    def SetMaxContainers(self, MaxContainers):
        self.__OrderedList.MaxContainers = MaxContainers

    def SetFlags(self, HFlags, LFlags):

        ReturnCode = IfrQuestionHeader.SetFlags(self, HFlags)
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode
        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, EFI_IFR_UNIQUE_SET)
        if Ret:
            self.__OrderedList.Flags |= EFI_IFR_UNIQUE_SET

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, EFI_IFR_NO_EMPTY_SET)
        if Ret:
            self.__OrderedList.Flags |= EFI_IFR_NO_EMPTY_SET

        return VfrReturnCode.VFR_RETURN_SUCCESS if LFlags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED

    def GetInfo(self):
        return self.__OrderedList


class IfrString(IfrLine, IfrOpHeader, IfrQuestionHeader):

    def __init__(self):
        self.__Str = EFI_IFR_STRING()
        IfrOpHeader.__init__(self, self.__Str.Header, EFI_IFR_STRING_OP)
        IfrQuestionHeader.__init__(self, self.__Str.Question)
        self.__Str.Flags = 0
        self.__Str.MinSize = 0
        self.__Str.MaxSize = 0

    def GetQuestion(self):
        return self

    def SetQHeaderFlags(self, Flags):
        IfrQuestionHeader.SetFlags(self, Flags)

    def SetFlags(self, HFlags, LFlags):

        ReturnCode = IfrQuestionHeader.SetFlags(self, HFlags)
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, EFI_IFR_STRING_MULTI_LINE)
        if Ret:
            self.__Str.Flags |= EFI_IFR_STRING_MULTI_LINE

        return VfrReturnCode.VFR_RETURN_SUCCESS if LFlags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED

    def SetMinSize(self, MinSize):
        self.__Str.MinSize = MinSize

    def SetMaxSize(self, MaxSize):
        self.__Str.MaxSize = MaxSize

    def GetInfo(self):
        return self.__Str


class IfrPassword(IfrLine, IfrOpHeader, IfrQuestionHeader):

    def __init__(self):
        self.__Password = EFI_IFR_PASSWORD()
        IfrOpHeader.__init__(self, self.__Password.Header,
                              EFI_IFR_PASSWORD_OP)
        IfrQuestionHeader.__init__(self, self.__Password.Question)
        self.__Password.MinSize = 0
        self.__Password.MaxSize = 0

    def GetQuestion(self):
        return self

    def SetQHeaderFlags(self, Flags):
        IfrQuestionHeader.SetFlags(self, Flags)

    def SetMinSize(self, MinSize):
        self.__Password.MinSize = MinSize

    def SetMaxSize(self, MaxSize):
        self.__Password.MaxSize = MaxSize

    def GetInfo(self):
        return self.__Password

class IfrDefault(IfrLine, IfrOpHeader):

    def __init__(self,
                 ValueType,
                 ValueList,
                 DefaultId=EFI_HII_DEFAULT_CLASS_STANDARD,
                 Type=EFI_IFR_TYPE_OTHER):
        Nums = len(ValueList)
        self.__Default = Refine_EFI_IFR_DEFAULT(ValueType, Nums)
        IfrOpHeader.__init__(self, self.__Default.Header, EFI_IFR_DEFAULT_OP, sizeof(self.__Default))

        self.__Default.Type = Type
        self.__Default.DefaultId = DefaultId
        self.__ValueType = ValueType

        if ValueList != []:
            ArrayType = TypeDict[ValueType] * Nums
            ValueArray = ArrayType()
            for i in range(0, len(ValueList)):
                ValueArray[i] = ValueList[i]
            self.__Default.Value = ValueArray

    def SetDefaultId(self, DefaultId):
        self.__Default.DefaultId = DefaultId

    def GetValueType(self):
        return self.__ValueType

    def SetType(self, Type):
        self.__Default.Type = Type

    def SetValue(self, ValueList):
        ArrayType = TypeDict[self.__ValueType] * (len(ValueList))
        ValueArray = ArrayType()
        for i in range(0, len(ValueList)):
            ValueArray[i] = ValueList[i]
        self.__Default.Value = ValueArray

    def GetInfo(self):
        return self.__Default


class IfrDefault2(IfrLine, IfrOpHeader):

    def __init__(self,
                 DefaultId=EFI_HII_DEFAULT_CLASS_STANDARD,
                 Type=EFI_IFR_TYPE_OTHER):
        self.__Default = EFI_IFR_DEFAULT_2()
        IfrOpHeader.__init__(self, self.__Default.Header, EFI_IFR_DEFAULT_OP, sizeof(EFI_IFR_DEFAULT_2))
        self.__Default.Type = Type
        self.__Default.DefaultId = DefaultId

    def SetDefaultId(self, DefaultId):
        self.__Default.DefaultId = DefaultId

    def SetType(self, Type):
        self.__Default.Type = Type

    def GetInfo(self):
        return self.__Default


class IfrInconsistentIf(IfrLine, IfrOpHeader):

    def __init__(self):
        self.__InconsistentIf = EFI_IFR_INCONSISTENT_IF()
        IfrOpHeader.__init__(self, self.__InconsistentIf.Header,
                              EFI_IFR_INCONSISTENT_IF_OP)
        self.__InconsistentIf.Error = EFI_STRING_ID_INVALID

    def SetError(self, Error):
        self.__InconsistentIf.Error = Error

    def GetInfo(self):
        return self.__InconsistentIf


class IfrNoSubmitIf(IfrLine, IfrOpHeader):

    def __init__(self):
        self.__NoSubmitIf = EFI_IFR_NO_SUBMIT_IF()
        IfrOpHeader.__init__(self, self.__NoSubmitIf.Header,
                              EFI_IFR_NO_SUBMIT_IF_OP)
        self.__NoSubmitIf.Error = EFI_STRING_ID_INVALID

    def SetError(self, Error):
        self.__NoSubmitIf.Error = Error

    def GetInfo(self):
        return self.__NoSubmitIf


class IfrDisableIf(IfrLine, IfrOpHeader):

    def __init__(self):
        self.__DisableIf = EFI_IFR_DISABLE_IF()
        IfrOpHeader.__init__(self, self.__DisableIf.Header,
                              EFI_IFR_DISABLE_IF_OP)

    def GetInfo(self):
        return self.__DisableIf


class IfrSuppressIf(IfrLine, IfrOpHeader):

    def __init__(self):
        self.__SuppressIf = EFI_IFR_SUPPRESS_IF()
        IfrOpHeader.__init__(self, self.__SuppressIf.Header,
                              EFI_IFR_SUPPRESS_IF_OP)

    def GetInfo(self):
        return self.__SuppressIf


class IfrGrayOutIf(IfrLine, IfrOpHeader):

    def __init__(self):
        self.__GrayOutIf = EFI_IFR_GRAY_OUT_IF()
        IfrOpHeader.__init__(self, self.__GrayOutIf.Header,
                              EFI_IFR_GRAY_OUT_IF_OP)

    def GetInfo(self):
        return self.__GrayOutIf


class IfrValue(IfrLine, IfrOpHeader):

    def __init__(self):
        self.__Value = EFI_IFR_VALUE()
        IfrOpHeader.__init__(self, self.__Value.Header, EFI_IFR_VALUE_OP)

    def GetInfo(self):
        return self.__Value


class IfrRead(IfrLine, IfrOpHeader):

    def __init__(self):
        self.__Read = EFI_IFR_READ()
        IfrOpHeader.__init__(self, self.__Read.Header, EFI_IFR_READ_OP)

    def GetInfo(self):
        return self.__Read


class IfrWrite(IfrLine, IfrOpHeader):

    def __init__(self):
        self.__Write = EFI_IFR_WRITE()
        IfrOpHeader.__init__(self, self.__Write.Header, EFI_IFR_WRITE_OP)

    def GetInfo(self):
        return self.__Write


class IfrWarningIf(IfrLine, IfrOpHeader):

    def __init__(self):
        self.__WarningIf = EFI_IFR_WARNING_IF()
        IfrOpHeader.__init__(self, self.__WarningIf.Header,
                              EFI_IFR_WARNING_IF_OP)
        self.__WarningIf.Warning = EFI_STRING_ID_INVALID
        self.__WarningIf.TimeOut = 0

    def SetWarning(self, Warning):
        self.__WarningIf.Warning = Warning

    def SetTimeOut(self, TimeOut):
        self.__WarningIf.TimeOut = TimeOut

    def GetInfo(self):
        return self.__WarningIf


class IfrRefresh(IfrLine, IfrOpHeader):

    def __init__(self):
        self.__Refresh = EFI_IFR_REFRESH()
        IfrOpHeader.__init__(self, self.__Refresh.Header, EFI_IFR_REFRESH_OP)
        self.__Refresh.RefreshInterval = 0

    def SetRefreshInterval(self, RefreshInterval):
        self.__Refresh.RefreshInterval = RefreshInterval

    def GetInfo(self):
        return self.__Refresh


class IfrRefreshId(IfrLine, IfrOpHeader):

    def __init__(self):
        self.__RefreshId = EFI_IFR_REFRESH_ID()
        IfrOpHeader.__init__(self, self.__RefreshId.Header,
                              EFI_IFR_REFRESH_ID_OP)
        self.__RefreshId.RefreshEventGroupId = EFI_GUID(
            0, 0, 0, GuidArray(0, 0, 0, 0, 0, 0, 0, 0))

    def SetRefreshEventGroutId(self, RefreshEventGroupId):
        self.__RefreshId.RefreshEventGroupId = RefreshEventGroupId

    def GetInfo(self):
        return self.__RefreshId


class IfrVarStoreDevice(IfrLine, IfrOpHeader):

    def __init__(self):
        self.__VarStoreDevice = EFI_IFR_VARSTORE_DEVICE()
        IfrOpHeader.__init__(self, self.__VarStoreDevice.Header,
                              EFI_IFR_VARSTORE_DEVICE_OP)
        self.__VarStoreDevice.DevicePath = EFI_STRING_ID_INVALID

    def SetDevicePath(self, DevicePath):
        self.__VarStoreDevice.DevicePath = DevicePath

    def GetInfo(self):
        return self.__VarStoreDevice


class IfrDate(IfrLine, IfrOpHeader, IfrQuestionHeader):

    def __init__(self):
        self.__Date = EFI_IFR_DATE()
        IfrOpHeader.__init__(self, self.__Date.Header, EFI_IFR_DATE_OP)
        IfrQuestionHeader.__init__(self, self.__Date.Question)
        self.__Date.Flags = 0

    def SetFlags(self, HFlags, LFlags):

        ReturnCode = IfrQuestionHeader.SetFlags(self, HFlags)
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, EFI_QF_DATE_YEAR_SUPPRESS)
        if Ret:
            self.__Date.Flags |= EFI_QF_DATE_YEAR_SUPPRESS

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, EFI_QF_DATE_MONTH_SUPPRESS)
        if Ret:
            self.__Date.Flags |= EFI_QF_DATE_MONTH_SUPPRESS

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, EFI_QF_DATE_DAY_SUPPRESS)
        if Ret:
            self.__Date.Flags |= EFI_QF_DATE_DAY_SUPPRESS

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, QF_DATE_STORAGE_NORMAL)
        if Ret:
            self.__Date.Flags |= QF_DATE_STORAGE_NORMAL

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, QF_DATE_STORAGE_TIME)
        if Ret:
            self.__Date.Flags |= QF_DATE_STORAGE_TIME

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, QF_DATE_STORAGE_WAKEUP)
        if Ret:
            self.__Date.Flags |= QF_DATE_STORAGE_WAKEUP

        return VfrReturnCode.VFR_RETURN_SUCCESS if LFlags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED

    def GetInfo(self):
        return self.__Date


class IfrTime(IfrLine, IfrOpHeader, IfrQuestionHeader):

    def __init__(self):
        self.__Time = EFI_IFR_TIME()
        IfrOpHeader.__init__(self, self.__Time.Header, EFI_IFR_TIME_OP)
        IfrQuestionHeader.__init__(self, self.__Time.Question)
        self.__Time.Flags = 0

    def SetFlags(self, HFlags, LFlags):

        ReturnCode = IfrQuestionHeader.SetFlags(self, HFlags)
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, QF_TIME_HOUR_SUPPRESS)
        if Ret:
            self.__Time.Flags |= QF_TIME_HOUR_SUPPRESS

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, QF_TIME_MINUTE_SUPPRESS)
        if Ret:
            self.__Time.Flags |= QF_TIME_MINUTE_SUPPRESS

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, QF_TIME_SECOND_SUPPRESS)
        if Ret:
            self.__Time.Flags |= QF_TIME_SECOND_SUPPRESS

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, QF_TIME_STORAGE_NORMAL)
        if Ret:
            self.__Time.Flags |= QF_TIME_STORAGE_NORMAL

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, QF_TIME_STORAGE_TIME)
        if Ret:
            self.__Time.Flags |= QF_TIME_STORAGE_TIME

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, QF_TIME_STORAGE_WAKEUP)
        if Ret:
            self.__Time.Flags |= QF_TIME_STORAGE_WAKEUP

        return VfrReturnCode.VFR_RETURN_SUCCESS if LFlags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED

    def GetInfo(self):
        return self.__Time


class IfrNumeric(IfrLine, IfrOpHeader, IfrQuestionHeader,
                  IfrMinMaxStepData):

    def __init__(self, Type):
        self.__Numeric = Refine_EFI_IFR_NUMERIC(Type)
        IfrOpHeader.__init__(self, self.__Numeric.Header, EFI_IFR_NUMERIC_OP, sizeof(self.__Numeric))
        IfrQuestionHeader.__init__(self, self.__Numeric.Question)
        IfrMinMaxStepData.__init__(self, self.__Numeric.Data, True)
        self.__Numeric.Flags = EFI_IFR_NUMERIC_SIZE_1 | EFI_IFR_DISPLAY_UINT_DEC

    def GetQuestion(self):
        return self

    def GetMinMaxData(self):
        return self

    def SetQHeaderFlags(self, Flags):
        IfrQuestionHeader.SetFlags(self, Flags)

    def SetFlags(self, HFlags, LFlags, DisplaySettingsSpecified=False):
        ReturnCode = IfrQuestionHeader.SetFlags(self, HFlags)
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode
        if DisplaySettingsSpecified == False:
            self.__Numeric.Flags = LFlags | EFI_IFR_DISPLAY_UINT_DEC
        else:
            self.__Numeric.Flags = LFlags
        return VfrReturnCode.VFR_RETURN_SUCCESS

    def SetFlagsForBitField(self,
                            HFlags,
                            LFlags,
                            DisplaySettingsSpecified=False):
        ReturnCode = IfrQuestionHeader.SetFlags(self, HFlags)
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode
        if DisplaySettingsSpecified == False:
            self.__Numeric.Flags = LFlags | EDKII_IFR_DISPLAY_UINT_DEC_BIT
        else:
            self.__Numeric.Flags = LFlags
        return VfrReturnCode.VFR_RETURN_SUCCESS

    def GetNumericFlags(self):
        return self.__Numeric.Flags

    def ShrinkBinSize(self, Size):
        self.ShrinkBinSize(Size)
        self.DecLength(Size)
        #  _EMIT_PENDING_OBJ();
        Numeric = EFI_IFR_NUMERIC()
        self.UpdateHeader(Numeric.Header)

    def GetInfo(self):
        return self.__Numeric


class IfrOneOf(
        IfrQuestionHeader,
        IfrLine,
        IfrOpHeader,
        IfrMinMaxStepData):

    def __init__(self, Type):
        self.__OneOf = Refine_EFI_IFR_ONE_OF(Type)
        IfrOpHeader.__init__(self, self.__OneOf.Header, EFI_IFR_ONE_OF_OP,
                              sizeof(self.__OneOf))
        IfrQuestionHeader.__init__(self, self.__OneOf.Question)
        IfrMinMaxStepData.__init__(self, self.__OneOf.Data)
        self.__OneOf.Flags = 0

    def GetQuestion(self):
        return self

    def GetMinMaxData(self):
        return self

    def SetQHeaderFlags(self, Flags):
        IfrQuestionHeader.SetFlags(self, Flags)

    def SetFlags(self, HFlags, LFlags):
        ReturnCode = IfrQuestionHeader.SetFlags(self, HFlags)
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode
        if LFlags & EFI_IFR_DISPLAY:
            self.__OneOf.Flags = LFlags
        else:
            self.__OneOf.Flags = LFlags | EFI_IFR_DISPLAY_UINT_DEC
        return VfrReturnCode.VFR_RETURN_SUCCESS

    def SetFlagsForBitField(self, HFlags, LFlags):
        ReturnCode = IfrQuestionHeader.SetFlags(self, HFlags)
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode
        if LFlags & EDKII_IFR_DISPLAY_BIT:
            self.__OneOf.Flags = LFlags
        else:
            self.__OneOf.Flags = LFlags | EDKII_IFR_DISPLAY_UINT_DEC_BIT
        return VfrReturnCode.VFR_RETURN_SUCCESS

    def GetInfo(self):
        return self.__OneOf


class IfrCheckBox(IfrLine, IfrOpHeader, IfrQuestionHeader):

    def __init__(self):
        self.__CheckBox = EFI_IFR_CHECKBOX()
        IfrOpHeader.__init__(self, self.__CheckBox.Header,
                              EFI_IFR_CHECKBOX_OP)
        IfrQuestionHeader.__init__(self, self.__CheckBox.Question)
        self.__CheckBox.Flags = 0

    def GetQuestion(self):
        return self

    def SetQHeaderFlags(self, Flags):
        IfrQuestionHeader.SetFlags(self, Flags)

    def GetFlags(self):
        return self.__CheckBox.Flags

    def SetFlags(self, HFlags, LFlags):

        ReturnCode = IfrQuestionHeader.SetFlags(self, HFlags)
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, EFI_IFR_CHECKBOX_DEFAULT)
        if Ret:
            self.__CheckBox.Flags |= EFI_IFR_CHECKBOX_DEFAULT

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags,
                                           EFI_IFR_CHECKBOX_DEFAULT_MFG)

        if Ret:
            self.__CheckBox.Flags |= EFI_IFR_CHECKBOX_DEFAULT_MFG

        return VfrReturnCode.VFR_RETURN_SUCCESS if LFlags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED

    def GetInfo(self):
        return self.__CheckBox


class IfrResetButton(IfrLine, IfrOpHeader, IfrStatementHeader):

    def __init__(self):
        self.__ResetButton = EFI_IFR_RESET_BUTTON()
        IfrOpHeader.__init__(self, self.__ResetButton.Header,
                              EFI_IFR_RESET_BUTTON_OP)
        IfrStatementHeader.__init__(self, self.__ResetButton.Statement)
        self.__ResetButton.DefaultId = EFI_HII_DEFAULT_CLASS_STANDARD

    def SetDefaultId(self, DefaultId):
        self.__ResetButton.DefaultId = DefaultId

    def GetInfo(self):
        return self.__ResetButton


class IfrOr(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Or = EFI_IFR_OR()
        IfrOpHeader.__init__(self, self.__Or.Header, EFI_IFR_OR_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__Or


class IfrAnd(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__And = EFI_IFR_AND()
        IfrOpHeader.__init__(self, self.__And.Header, EFI_IFR_AND_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__And


class IfrBitWiseOr(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__BitWiseOr = EFI_IFR_BITWISE_OR()
        IfrOpHeader.__init__(self, self.__BitWiseOr.Header,
                              EFI_IFR_BITWISE_OR_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__BitWiseOr


class IfrCatenate(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Catenate = EFI_IFR_CATENATE()
        IfrOpHeader.__init__(self, self.__Catenate.Header,
                              EFI_IFR_CATENATE_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__Catenate


class IfrDivide(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Divide = EFI_IFR_DIVIDE()
        IfrOpHeader.__init__(self, self.__Divide.Header, EFI_IFR_DIVIDE_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__Divide


class IfrEqual(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Equal = EFI_IFR_EQUAL()
        IfrOpHeader.__init__(self, self.__Equal.Header, EFI_IFR_EQUAL_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__Equal


class IfrGreaterEqual(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__GreaterEqual = EFI_IFR_GREATER_EQUAL()
        IfrOpHeader.__init__(self, self.__GreaterEqual.Header,
                              EFI_IFR_GREATER_EQUAL_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__GreaterEqual


class IfrGreaterThan(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__GreaterThan = EFI_IFR_GREATER_THAN()
        IfrOpHeader.__init__(self, self.__GreaterThan.Header,
                              EFI_IFR_GREATER_THAN_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__GreaterThan


class IfrLessEqual(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__LessEqual = EFI_IFR_LESS_EQUAL()
        IfrOpHeader.__init__(self, self.__LessEqual.Header,
                              EFI_IFR_LESS_EQUAL_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__LessEqual


class IfrLessThan(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__LessThan = EFI_IFR_LESS_THAN()
        IfrOpHeader.__init__(self, self.__LessThan.Header,
                              EFI_IFR_LESS_THAN_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__LessThan


class IfrMap(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Map = EFI_IFR_MAP()
        IfrOpHeader.__init__(self, self.__Map.Header, EFI_IFR_MAP_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__Map


class IfrMatch(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Match = EFI_IFR_MATCH()
        IfrOpHeader.__init__(self, self.__Match.Header, EFI_IFR_MATCH_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__Match


class IfrMatch2(IfrLine, IfrOpHeader):

    def __init__(self, LineNo, Guid):
        self.__Match2 = EFI_IFR_MATCH2()
        IfrOpHeader.__init__(self, self.__Match2.Header, EFI_IFR_MATCH2_OP)
        self.SetLineNo(LineNo)
        self.__Match2.SyntaxType = Guid

    def GetInfo(self):
        return self.__Match2


class IfrMultiply(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Multiply = EFI_IFR_MULTIPLY()
        IfrOpHeader.__init__(self, self.__Multiply.Header,
                              EFI_IFR_MULTIPLY_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__Multiply


class IfrModulo(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Modulo = EFI_IFR_MODULO()
        IfrOpHeader.__init__(self, self.__Modulo.Header, EFI_IFR_MODULO_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__Modulo


class IfrNotEqual(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__NotEqual = EFI_IFR_NOT_EQUAL()
        IfrOpHeader.__init__(self, self.__NotEqual.Header,
                              EFI_IFR_NOT_EQUAL_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__NotEqual


class IfrShiftLeft(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__ShiftLeft = EFI_IFR_SHIFT_LEFT()
        IfrOpHeader.__init__(self, self.__ShiftLeft.Header,
                              EFI_IFR_SHIFT_LEFT_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__ShiftLeft


class IfrShiftRight(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__ShiftRight = EFI_IFR_SHIFT_RIGHT()
        IfrOpHeader.__init__(self, self.__ShiftRight.Header,
                              EFI_IFR_SHIFT_RIGHT_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__ShiftRight


class IfrSubtract(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Subtract = EFI_IFR_SUBTRACT()
        IfrOpHeader.__init__(self, self.__Subtract.Header,
                              EFI_IFR_SUBTRACT_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__Subtract


class IfrConditional(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Conditional = EFI_IFR_CONDITIONAL()
        IfrOpHeader.__init__(self, self.__Conditional.Header,
                              EFI_IFR_CONDITIONAL_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__Conditional


class IfrFind(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Find = EFI_IFR_FIND()
        IfrOpHeader.__init__(self, self.__Find.Header, EFI_IFR_FIND_OP)
        self.SetLineNo(LineNo)

    def SetFormat(self, Format):
        self.__Find.Format = Format

    def GetInfo(self):
        return self.__Find


class IfrMid(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Mid = EFI_IFR_MID()
        IfrOpHeader.__init__(self, self.__Mid.Header, EFI_IFR_MID_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__Mid


class IfrToken(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Token = EFI_IFR_TOKEN()
        IfrOpHeader.__init__(self, self.__Token.Header, EFI_IFR_TOKEN_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__Token


class IfrSpan(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Span = EFI_IFR_SPAN()
        IfrOpHeader.__init__(self, self.__Span.Header, EFI_IFR_SPAN_OP)
        self.SetLineNo(LineNo)
        self.__Span.Flags = EFI_IFR_FLAGS_FIRST_MATCHING

    def SetFlags(self, LFlags):
        if LFlags == EFI_IFR_FLAGS_FIRST_MATCHING:
            self.__Span.Flags |= EFI_IFR_FLAGS_FIRST_MATCHING
        else:
            LFlags, Ret = _FLAG_TEST_AND_CLEAR(
                LFlags, EFI_IFR_FLAGS_FIRST_NON_MATCHING)
            if Ret:
                self.__Span.Flags |= EFI_IFR_FLAGS_FIRST_NON_MATCHING

        return VfrReturnCode.VFR_RETURN_SUCCESS if LFlags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED

    def GetInfo(self):
        return self.__Span


class IfrBitWiseAnd(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__BitWiseAnd = EFI_IFR_BITWISE_AND()
        IfrOpHeader.__init__(self, self.__BitWiseAnd.Header,
                              EFI_IFR_BITWISE_AND_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__BitWiseAnd


class IfrBitWiseOr(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__BitWiseOr = EFI_IFR_BITWISE_OR()
        IfrOpHeader.__init__(self, self.__BitWiseOr.Header,
                              EFI_IFR_BITWISE_OR_OP)
        self.SetLineNo(LineNo)


class IfrAdd(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Add = EFI_IFR_ADD()
        IfrOpHeader.__init__(self, self.__Add.Header, EFI_IFR_ADD_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__Add


class IfrToString(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__ToString = EFI_IFR_TO_STRING()
        IfrOpHeader.__init__(self, self.__ToString.Header,
                              EFI_IFR_TO_STRING_OP)
        self.SetLineNo(LineNo)

    def SetFormat(self, Format):
        self.__ToString.Format = Format

    def GetInfo(self):
        return self.__ToString


class IfrToUpper(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__ToUppper = EFI_IFR_TO_UPPER()
        IfrOpHeader.__init__(self, self.__ToUppper.Header,
                              EFI_IFR_TO_UPPER_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__ToUppper


class IfrToUint(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__ToUint = EFI_IFR_TO_UINT()
        IfrOpHeader.__init__(self, self.__ToUint.Header, EFI_IFR_TO_UINT_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__ToUint


class IfrToLower(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__ToLower = EFI_IFR_TO_LOWER()
        IfrOpHeader.__init__(self, self.__ToLower.Header, EFI_IFR_TO_LOWER_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__ToLower


class IfrToBoolean(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Boolean = EFI_IFR_TO_BOOLEAN()
        IfrOpHeader.__init__(self, self.__Boolean.Header,
                              EFI_IFR_TO_BOOLEAN_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__Boolean


class IfrNot(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Not = EFI_IFR_NOT()
        IfrOpHeader.__init__(self, self.__Not.Header, EFI_IFR_NOT_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__Not


class IfrDup(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Dup = EFI_IFR_DUP()
        IfrOpHeader.__init__(self, self.__Dup.Header, EFI_IFR_DUP_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.__Dup.Header

    def GetInfo(self):
        return self.__Dup


class IfrEqIdId(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__EqIdId = EFI_IFR_EQ_ID_ID()
        IfrOpHeader.__init__(self, self.__EqIdId.Header, EFI_IFR_EQ_ID_ID_OP)
        self.SetLineNo(LineNo)
        self.__EqIdId.QuestionId1 = EFI_QUESTION_ID_INVALID
        self.__EqIdId.QuestionId2 = EFI_QUESTION_ID_INVALID

    def GetHeader(self):
        return self.__EqIdId.Header

    def SetQuestionId1(self, QuestionId, VarIdStr, LineNo):
        if QuestionId != EFI_QUESTION_ID_INVALID:
            self.__EqIdId.QuestionId1 = QuestionId
        else:
            gFormPkg.AssignPending(VarIdStr, self.__EqIdId, LineNo, "no question refered", 1)

    def SetQuestionId2(self, QuestionId, VarIdStr, LineNo):
        if QuestionId != EFI_QUESTION_ID_INVALID:
            self.__EqIdId.QuestionId2 = QuestionId
        else:
            gFormPkg.AssignPending(VarIdStr, self.__EqIdId, LineNo, "no question refered", 2)

    def GetInfo(self):
        return self.__EqIdId


class IfrEqIdVal(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__EqIdVal = EFI_IFR_EQ_ID_VAL()
        IfrOpHeader.__init__(self, self.__EqIdVal.Header,
                              EFI_IFR_EQ_ID_VAL_OP)
        self.SetLineNo(LineNo)
        self.__EqIdVal.QuestionId = EFI_QUESTION_ID_INVALID

    def SetQuestionId(self, QuestionId, VarIdStr, LineNo):
        if QuestionId != EFI_QUESTION_ID_INVALID:
            self.__EqIdVal.QuestionId = QuestionId
        else:
            gFormPkg.AssignPending(VarIdStr, self.__EqIdVal, LineNo, "no question refered")

    def SetValue(self, Value):
        self.__EqIdVal.Value = Value

    def GetHeader(self):
        return self.__EqIdVal.Header

    def GetInfo(self):
        return self.__EqIdVal


class IfrEqIdList(IfrLine, IfrOpHeader):

    def __init__(self, LineNo, Nums, ValueList=[]):
        self.__EqIdVList = Refine_EFI_IFR_EQ_ID_VAL_LIST(Nums)
        IfrOpHeader.__init__(self, self.__EqIdVList.Header, EFI_IFR_EQ_ID_VAL_LIST_OP, sizeof(self.__EqIdVList))
        self.SetLineNo(LineNo)
        self.__EqIdVList.QuestionId = EFI_QUESTION_ID_INVALID
        self.__EqIdVList.ListLength = 0
        if ValueList != []:
            ArrayType = c_uint16 * Nums
            ValueArray = ArrayType()
            for i in range(0, len(ValueList)):
                ValueArray[i] = ValueList[i]
            self.__EqIdVList.ValueList = ValueArray

    def SetQuestionId(self, QuestionId, VarIdStr, LineNo):
        if QuestionId != EFI_QUESTION_ID_INVALID:
            self.__EqIdVList.QuestionId = QuestionId
        else:
            gFormPkg.AssignPending(VarIdStr, self.__EqIdVList, LineNo, "no question refered")

    def SetListLength(self, ListLength):
        self.__EqIdVList.ListLength = ListLength

    def SetValueList(self, ValueList):
        if ValueList != []:
            ArrayType = c_uint16 * len(ValueList)
            ValueArray = ArrayType()
            for i in range(0, len(ValueList)):
                ValueArray[i] = ValueList[i]
            self.__EqIdVList.ValueList = ValueArray

    def GetHeader(self):
        return self.__EqIdVList.Header

    def GetInfo(self):
        return self.__EqIdVList


class IfrUint8(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Uint8 = EFI_IFR_UINT8()
        IfrOpHeader.__init__(self, self.__Uint8.Header, EFI_IFR_UINT8_OP)
        self.SetLineNo(LineNo)

    def SetValue(self, Value):
        self.__Uint8.Value = Value

    def GetHeader(self):
        return self.__Uint8.Header

    def GetInfo(self):
        return self.__Uint8


class IfrUint16(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Uint16 = EFI_IFR_UINT16()
        IfrOpHeader.__init__(self, self.__Uint16.Header, EFI_IFR_UINT16_OP)
        self.SetLineNo(LineNo)

    def SetValue(self, Value):
        self.__Uint16.Value = Value

    def GetHeader(self):
        return self.__Uint16.Header

    def GetInfo(self):
        return self.__Uint16


class IfrUint32(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Uint32 = EFI_IFR_UINT32()
        IfrOpHeader.__init__(self, self.__Uint32.Header, EFI_IFR_UINT32_OP)
        self.SetLineNo(LineNo)

    def SetValue(self, Value):
        self.__Uint32.Value = Value

    def GetHeader(self):
        return self.__Uint32.Header

    def GetInfo(self):
        return self.__Uint32


class IfrUint64(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Uint64 = EFI_IFR_UINT64()
        IfrOpHeader.__init__(self, self.__Uint64.Header, EFI_IFR_UINT64_OP, sizeof(EFI_IFR_UINT64))
        self.SetLineNo(LineNo)

    def SetValue(self, Value):
        self.__Uint64.Value = Value

    def GetHeader(self):
        return self.__Uint64.Header

    def GetInfo(self):
        return self.__Uint64


class IfrQuestionRef1(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__QuestionRef1 = EFI_IFR_QUESTION_REF1()
        IfrOpHeader.__init__(self, self.__QuestionRef1.Header,
                              EFI_IFR_QUESTION_REF1_OP)
        self.SetLineNo(LineNo)
        self.__QuestionRef1.QuestionId = EFI_QUESTION_ID_INVALID

    def GetHeader(self):
        return self.__QuestionRef1.Header

    def SetQuestionId(self, QuestionId, VarIdStr, LineNo):
        if QuestionId != EFI_QUESTION_ID_INVALID:
            self.__QuestionRef1.QuestionId = QuestionId
        else:
            gFormPkg.AssignPending(VarIdStr, self.__QuestionRef1, LineNo, "no question refered")

    def GetInfo(self):
        return self.__QuestionRef1


class IfrQuestionRef2(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__QuestionRef2 = EFI_IFR_QUESTION_REF2()
        IfrOpHeader.__init__(self, self.__QuestionRef2.Header,
                              EFI_IFR_QUESTION_REF2_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.__QuestionRef2.Header

    def GetInfo(self):
        return self.__QuestionRef2


class IfrQuestionRef3(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__QuestionRef3 = EFI_IFR_QUESTION_REF3()
        IfrOpHeader.__init__(self, self.__QuestionRef3.Header,
                              EFI_IFR_QUESTION_REF3_OP, sizeof(EFI_IFR_QUESTION_REF3))
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.__QuestionRef3.Header

    def GetInfo(self):
        return self.__QuestionRef3


class IfrQuestionRef3_2(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__QuestionRef3_2 = EFI_IFR_QUESTION_REF3_2()
        IfrOpHeader.__init__(self, self.__QuestionRef3_2.Header,
                              EFI_IFR_QUESTION_REF3_OP, sizeof(EFI_IFR_QUESTION_REF3_2))
        self.SetLineNo(LineNo)
        self.__QuestionRef3_2.DevicePath = EFI_STRING_ID_INVALID

    def SetDevicePath(self, DevicePath):
        self.__QuestionRef3_2.DevicePath = DevicePath

    def GetHeader(self):
        return self.__QuestionRef3_2.Header

    def GetInfo(self):
        return self.__QuestionRef3_2


class IfrQuestionRef3_3(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__QuestionRef3_3 = EFI_IFR_QUESTION_REF3_3()
        IfrOpHeader.__init__(self, self.__QuestionRef3_3.Header,
                              EFI_IFR_QUESTION_REF3_OP, sizeof(EFI_IFR_QUESTION_REF3_3))
        self.SetLineNo(LineNo)
        self.__QuestionRef3_3.DevicePath = EFI_STRING_ID_INVALID
        self.__QuestionRef3_3.Guid = EFI_GUID(
            0, 0, 0, GuidArray(0, 0, 0, 0, 0, 0, 0, 0))

    def SetDevicePath(self, DevicePath):
        self.__QuestionRef3_3.DevicePath = DevicePath

    def SetGuid(self, Guid):
        self.__QuestionRef3_3.Guid = Guid

    def GetHeader(self):
        return self.__QuestionRef3_3.Header

    def GetInfo(self):
        return self.__QuestionRef3_3


class IfrRuleRef(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__RuleRef = EFI_IFR_RULE_REF()
        IfrOpHeader.__init__(self, self.__RuleRef.Header, EFI_IFR_RULE_REF_OP)
        self.SetLineNo(LineNo)
        self.__RuleRef.RuleId = EFI_RULE_ID_INVALID

    def SetRuleId(self, RuleId):
        self.__RuleRef.RuleId = RuleId

    def GetHeader(self):
        return self.__RuleRef.Header

    def GetInfo(self):
        return self.__RuleRef


class IfrStringRef1(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__StringRef1 = EFI_IFR_STRING_REF1()
        IfrOpHeader.__init__(self, self.__StringRef1.Header,
                              EFI_IFR_STRING_REF1_OP)
        self.SetLineNo(LineNo)
        self.__StringRef1.StringId = EFI_STRING_ID_INVALID

    def SetStringId(self, StringId):
        self.__StringRef1.StringId = StringId

    def GetHeader(self):
        return self.__StringRef1.Header

    def GetInfo(self):
        return self.__StringRef1


class IfrStringRef2(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__StringRef2 = EFI_IFR_STRING_REF2()
        IfrOpHeader.__init__(self, self.__StringRef2.Header,
                              EFI_IFR_STRING_REF2_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.__StringRef2.Header

    def GetInfo(self):
        return self.__StringRef2


class IfrThis(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__This = EFI_IFR_THIS()
        IfrOpHeader.__init__(self, self.__This.Header, EFI_IFR_THIS_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.__This.Header

    def GetInfo(self):
        return self.__This


class IfrSecurity(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Security = EFI_IFR_SECURITY()
        IfrOpHeader.__init__(self, self.__Security.Header,
                              EFI_IFR_SECURITY_OP)
        self.SetLineNo(LineNo)
        self.__Security.Permissions = EFI_GUID(
            0, 0, 0, GuidArray(0, 0, 0, 0, 0, 0, 0, 0))

    def SetPermissions(self, Permissions):
        self.__Security.Permissions = Permissions

    def GetHeader(self):
        return self.__Security.Header

    def GetInfo(self):
        return self.__Security


class IfrGet(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Get = EFI_IFR_GET()
        IfrOpHeader.__init__(self, self.__Get.Header, EFI_IFR_GET_OP)
        self.SetLineNo(LineNo)

    def SetVarInfo(self, BaseInfo: EFI_VARSTORE_INFO):
        self.__Get.VarStoreId = BaseInfo.VarStoreId
        self.__Get.VarStoreInfo.VarName = BaseInfo.Info.VarName
        self.__Get.VarStoreInfo.VarOffset = BaseInfo.Info.VarOffset
        self.__Get.VarStoreType = BaseInfo.VarType

    def GetHeader(self):
        return self.__Get.Header

    def GetInfo(self):
        return self.__Get


class IfrSet(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Set = EFI_IFR_SET()
        IfrOpHeader.__init__(self, self.__Set.Header, EFI_IFR_SET_OP)
        self.SetLineNo(LineNo)

    def SetVarInfo(self, BaseInfo: EFI_VARSTORE_INFO):
        self.__Set.VarStoreId = BaseInfo.VarStoreId
        self.__Set.VarStoreInfo.VarName = BaseInfo.Info.VarName
        self.__Set.VarStoreInfo.VarOffset = BaseInfo.Info.VarOffset
        self.__Set.VarStoreType = BaseInfo.VarType

    def GetHeader(self):
        return self.__Set.Header

    def GetInfo(self):
        return self.__Set


class IfrTrue(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__True = EFI_IFR_TRUE()
        IfrOpHeader.__init__(self, self.__True.Header, EFI_IFR_TRUE_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.__True.Header

    def GetInfo(self):
        return self.__True


class IfrFalse(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__False = EFI_IFR_TRUE()
        IfrOpHeader.__init__(self, self.__False.Header, EFI_IFR_FALSE_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.__False.Header

    def GetInfo(self):
        return self.__False


class IfrOne(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__One = EFI_IFR_ONE()
        IfrOpHeader.__init__(self, self.__One.Header, EFI_IFR_ONE_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.__One.Header

    def GetInfo(self):
        return self.__One


class IfrOnes(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Ones = EFI_IFR_ONE()
        IfrOpHeader.__init__(self, self.__Ones.Header, EFI_IFR_ONES_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.__Ones.Header

    def GetInfo(self):
        return self.__Ones


class IfrZero(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Zero = EFI_IFR_ZERO()
        IfrOpHeader.__init__(self, self.__Zero.Header, EFI_IFR_ZERO_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.__Zero.Header

    def GetInfo(self):
        return self.__Zero


class IfrUndefined(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Undefined = EFI_IFR_ZERO()
        IfrOpHeader.__init__(self, self.__Undefined.Header,
                              EFI_IFR_UNDEFINED_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.__Undefined.Header

    def GetInfo(self):
        return self.__Undefined


class IfrVersion(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Version = EFI_IFR_VERSION()
        IfrOpHeader.__init__(self, self.__Version.Header, EFI_IFR_VERSION_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.__Version.Header

    def GetInfo(self):
        return self.__Version


class IfrLength(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__Length = EFI_IFR_LENGTH()
        IfrOpHeader.__init__(self, self.__Length.Header, EFI_IFR_LENGTH_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__Length


class IfrBitWiseNot(IfrLine, IfrOpHeader):

    def __init__(self, LineNo):
        self.__BitWiseNot = EFI_IFR_BITWISE_NOT()
        IfrOpHeader.__init__(self, self.__BitWiseNot.Header,
                              EFI_IFR_BITWISE_NOT_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.__BitWiseNot


class ExpressionInfo():

    def __init__(self):
        self.RootLevel = 0
        self.ExpOpCount = 0