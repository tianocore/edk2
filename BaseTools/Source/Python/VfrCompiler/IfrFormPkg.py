from ast import For
from re import L
from sre_parse import FLAGS
from stat import FILE_ATTRIBUTE_SPARSE_FILE
from VfrCompiler.IfrCtypes import *
from VfrCompiler.IfrError import VfrReturnCode
from VfrCompiler.IfrUtility import *
from ctypes import *

gVfrVarDataTypeDB = VfrVarDataTypeDB()
gVfrDefaultStore = VfrDefaultStore()
gVfrDataStorage = VfrDataStorage()


class ReCordNode(Structure):
    def __init__(self, Record, LineNo):
        self.Record = Record
        self.LineNo = LineNo


class OpNode:
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
    OpNode(ctypes.sizeof(EFI_IFR_ONE_OF_OPTION), 0),  # EFI_IFR_ONE_OF_OPTION_OP
    OpNode(ctypes.sizeof(EFI_IFR_SUPPRESS_IF), 1),  # EFI_IFR_SUPPRESS_IF - 0x0A
    OpNode(ctypes.sizeof(EFI_IFR_LOCKED), 0),  # EFI_IFR_LOCKED_OP
    OpNode(ctypes.sizeof(EFI_IFR_ACTION), 1),  # EFI_IFR_ACTION_OP
    OpNode(ctypes.sizeof(EFI_IFR_RESET_BUTTON), 1),  # EFI_IFR_RESET_BUTTON_OP
    OpNode(ctypes.sizeof(EFI_IFR_FORM_SET), 1),  # EFI_IFR_FORM_SET_OP -0xE
    OpNode(ctypes.sizeof(EFI_IFR_REF), 0),  # EFI_IFR_REF_OP
    OpNode(ctypes.sizeof(EFI_IFR_NO_SUBMIT_IF), 1),  # EFI_IFR_NO_SUBMIT_IF_OP -0x10
    OpNode(ctypes.sizeof(EFI_IFR_INCONSISTENT_IF), 1),  # EFI_IFR_INCONSISTENT_IF_OP
    OpNode(ctypes.sizeof(EFI_IFR_EQ_ID_VAL), 0),  # EFI_IFR_EQ_ID_VAL_OP
    OpNode(ctypes.sizeof(EFI_IFR_EQ_ID_ID), 0),  # EFI_IFR_EQ_ID_ID_OP
    OpNode(ctypes.sizeof(EFI_IFR_EQ_ID_VAL_LIST), 0),  # EFI_IFR_EQ_ID_LIST_OP - 0x14
    OpNode(ctypes.sizeof(EFI_IFR_AND), 0),  # EFI_IFR_AND_OP
    OpNode(ctypes.sizeof(EFI_IFR_OR), 0),  # EFI_IFR_OR_OP
    OpNode(ctypes.sizeof(EFI_IFR_NOT), 0),  # EFI_IFR_NOT_OP
    OpNode(ctypes.sizeof(EFI_IFR_RULE), 1),  # EFI_IFR_RULE_OP
    OpNode(ctypes.sizeof(EFI_IFR_GRAY_OUT_IF), 1),  # EFI_IFR_GRAYOUT_IF_OP - 0x19
    OpNode(ctypes.sizeof(EFI_IFR_DATE), 1),  # EFI_IFR_DATE_OP
    OpNode(ctypes.sizeof(EFI_IFR_TIME), 1),  # EFI_IFR_TIME_OP
    OpNode(ctypes.sizeof(EFI_IFR_STRING), 1),  # EFI_IFR_STRING_OP
    OpNode(ctypes.sizeof(EFI_IFR_REFRESH), 0),  # EFI_IFR_REFRESH_OP
    OpNode(ctypes.sizeof(EFI_IFR_DISABLE_IF), 1),  # EFI_IFR_DISABLE_IF_OP - 0x1E
    OpNode(0, 0),  # 0x1F
    OpNode(ctypes.sizeof(EFI_IFR_TO_LOWER), 0),  # EFI_IFR_TO_LOWER_OP - 0x20
    OpNode(ctypes.sizeof(EFI_IFR_TO_UPPER), 0),  # EFI_IFR_TO_UPPER_OP - 0x21
    OpNode(ctypes.sizeof(EFI_IFR_MAP), 1),  # EFI_IFR_MAP - 0x22
    OpNode(ctypes.sizeof(EFI_IFR_ORDERED_LIST), 1),  # EFI_IFR_ORDERED_LIST_OP - 0x23
    OpNode(ctypes.sizeof(EFI_IFR_VARSTORE), 0),  # EFI_IFR_VARSTORE_OP
    OpNode(ctypes.sizeof(EFI_IFR_VARSTORE_NAME_VALUE), 0),  # EFI_IFR_VARSTORE_NAME_VALUE_OP
    OpNode(ctypes.sizeof(EFI_IFR_VARSTORE_EFI), 0),  # EFI_IFR_VARSTORE_EFI_OP
    OpNode(ctypes.sizeof(EFI_IFR_VARSTORE_DEVICE), 1),  # EFI_IFR_VARSTORE_DEVICE_OP
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
    OpNode(ctypes.sizeof(EFI_IFR_GREATER_EQUAL), 0),  # EFI_IFR_GREATER_EQUAL_OP
    OpNode(ctypes.sizeof(EFI_IFR_LESS_THAN), 0),  # EFI_IFR_LESS_THAN_OP
    OpNode(ctypes.sizeof(EFI_IFR_LESS_EQUAL), 0),  # EFI_IFR_LESS_EQUAL_OP - 0x34
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
    OpNode(ctypes.sizeof(EFI_IFR_QUESTION_REF1), 0),  # EFI_IFR_QUESTION_REF1_OP
    OpNode(ctypes.sizeof(EFI_IFR_QUESTION_REF2), 0),  # EFI_IFR_QUESTION_REF2_OP - 0x41
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
    OpNode(ctypes.sizeof(EFI_IFR_STRING_REF1), 0),  # EFI_IFR_STRING_REF1_OP - 0x4E
    OpNode(ctypes.sizeof(EFI_IFR_STRING_REF2), 0),  # EFI_IFR_STRING_REF2_OP
    OpNode(ctypes.sizeof(EFI_IFR_CONDITIONAL), 0),  # EFI_IFR_CONDITIONAL_OP
    OpNode(ctypes.sizeof(EFI_IFR_QUESTION_REF3), 0),  # EFI_IFR_QUESTION_REF3_OP
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
    OpNode(ctypes.sizeof(EFI_IFR_DEFAULTSTORE), 0),  # EFI_IFR_DEFAULTSTORE_OP - 0x5C
    OpNode(ctypes.sizeof(EFI_IFR_FORM_MAP), 1),  # EFI_IFR_FORM_MAP_OP - 0x5D
    OpNode(ctypes.sizeof(EFI_IFR_CATENATE), 0),  # EFI_IFR_CATENATE_OP
    OpNode(ctypes.sizeof(EFI_IFR_GUID), 0),  # EFI_IFR_GUID_OP
    OpNode(ctypes.sizeof(EFI_IFR_SECURITY), 0),  # EFI_IFR_SECURITY_OP - 0x60
    OpNode(ctypes.sizeof(EFI_IFR_MODAL_TAG), 0),  # EFI_IFR_MODAL_TAG_OP - 0x61
    OpNode(ctypes.sizeof(EFI_IFR_REFRESH_ID), 0),  # EFI_IFR_REFRESH_ID_OP - 0x62
    OpNode(ctypes.sizeof(EFI_IFR_WARNING_IF), 1),  # EFI_IFR_WARNING_IF_OP - 0x63
    OpNode(ctypes.sizeof(EFI_IFR_MATCH2), 0),
]


class OpBufferNode:
    def __init__(self, Buffer=None, Next=None):
        self.Buffer = Buffer
        self.Next = Next


class PACKAGE_DATA:
    def __init__(self, Bu) -> None:
        # self.Buffer = Buffer
        pass


class ASSIGN_FLAG(Enum):
    PENDING = 1
    ASSIGNED = 2


class SPendingAssign:
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


class InsertOpNode:
    def __init__(self, Data, OpCode):
        self.Data = Data
        self.OpCode = OpCode


class FormPkg:
    def __init__(self):
        self.PkgLength = 0
        self.Offset = 0
        self.PendingAssignList = None

    def Clear(self):
        self.PkgLength = 0
        self.Offset = 0
        self.PendingAssignList = None

    def BuildPkgHdr(self):
        PkgHdr = EFI_HII_PACKAGE_HEADER()
        PkgHdr.Type = EFI_HII_PACKAGE_FORM
        PkgHdr.Length = self.PkgLength + sizeof(EFI_HII_PACKAGE_HEADER)
        return PkgHdr

    def BuildPkg(self, Root):
        if Root == None:
            return
        if Root.OpCode != None and Root.OpCode != EFI_IFR_SHOWN_DEFAULTSTORE_OP:
            self.PkgLength += Root.Data.GetInfo().Header.Length
            Root.Offset = gFormPkg.Offset  #
            self.Offset += Root.Data.GetInfo().Header.Length
        if Root.Child != []:
            for ChildNode in Root.Child:
                self.BuildPkg(ChildNode)

    # Get data from ctypes to bytes.
    def StructToStream(self, Struct) -> bytes:
        Length = sizeof(Struct)
        P = cast(pointer(Struct), POINTER(c_char * Length))
        return P.contents.raw

    def AssignPending(self, Key, VarAddr, LineNo, Msg, Type=0):
        pNew = SPendingAssign(Key, VarAddr, LineNo, Msg, Type)
        if pNew == None:
            return VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES
        pNew.Next = self.PendingAssignList
        self.PendingAssignList = pNew
        return VfrReturnCode.VFR_RETURN_SUCCESS

    def DoPendingAssign(self, Key, Val):
        if Key == None or Val == None:
            return
        pNode = self.PendingAssignList
        while pNode != None:
            if pNode.Key == Key:
                pNode.AssignValue(Val)
            pNode = pNode.Next

    def HavePendingUnassigned(self):
        pNode = self.PendingAssignList
        while pNode != None:
            if pNode.Flag == ASSIGN_FLAG.PENDING:
                return True
            pNode = pNode.Next

        return False

    def PendingAssignPrintAll(self):
        pNode = self.PendingAssignList
        while pNode != None:
            if pNode.Flag == ASSIGN_FLAG.PENDING:
                gVfrErrorHandle.PrintMsg(pNode.LineNo, "Error", pNode.Msg, pNode.Key)
            pNode = pNode.Next

    def DeclarePendingQuestion(
        self,
        lVfrVarDataTypeDB: VfrVarDataTypeDB,
        lVfrDataStorage: VfrDataStorage,
        lVfrQuestionDB: VfrQuestionDB,
        LineNo=None,
    ):
        # Declare all questions as Numeric in DisableIf True
        ReturnList = []
        GuidObj = None
        DIObj = IfrDisableIf()
        DIObj.SetLineNo(LineNo)
        ReturnList.append(InsertOpNode(DIObj, EFI_IFR_DISABLE_IF_OP))
        # TrueOpcode
        TObj = IfrTrue(LineNo)
        ReturnList.append(InsertOpNode(TObj, EFI_IFR_TRUE_OP))
        pNode = self.PendingAssignList
        while pNode != None:
            if pNode.Flag == ASSIGN_FLAG.PENDING:
                Info = EFI_VARSTORE_INFO()
                VarStr = pNode.Key
                QId, ReturnCode = lVfrQuestionDB.RegisterQuestion(None, VarStr, EFI_QUESTION_ID_INVALID, gFormPkg)
                if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                    gVfrErrorHandle.HandleError(ReturnCode, pNode.LineNo, pNode.Key)
                    return ReturnList, ReturnCode
                # ifdef VFREXP_DEBUG
                # printf("Undefined Question name is %s and Id is 0x%x\n", VarStr, QId);
                # endif
                # Get Question Info, framework vfr VarName == StructName
                ArrayIdx, s, FName, ReturnCode = lVfrVarDataTypeDB.ExtractFieldNameAndArrary(VarStr, 0)
                if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                    gVfrErrorHandle.PrintMsg(pNode.LineNo, "Error", "Var string is not the valid C variable", pNode.Key)
                    return ReturnList, ReturnCode

                # Get VarStoreType
                Info.VarStoreId, ReturnCode = lVfrDataStorage.GetVarStoreId(FName)

                if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                    gVfrErrorHandle.PrintMsg(pNode.LineNo, "Error", "Var Store Type is not defined", FName)
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
                            NewStr = SName + "." + VarStr[s:]
                            (
                                Info.Info.VarOffset,
                                Info.VarType,
                                Info.VarTotalSize,
                                Info.IsBitVar,
                                ReturnCode,
                            ) = lVfrVarDataTypeDB.GetDataFieldInfo(NewStr)
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
                    MaxValue = (1 << Info.VarTotalSize) - 1
                    CNObj.SetMinMaxStepData(0, MaxValue, 0)
                    LFlags = EDKII_IFR_NUMERIC_SIZE_BIT & Info.VarTotalSize
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


class IfrLine:
    def __init__(self, LineNo=0):
        self.LineNo = LineNo

    def SetLineNo(self, LineNo):
        self.LineNo = LineNo

    def GetLineNo(self):
        return self.LineNo


class IfrBaseInfo:
    def __init__(self, Obj=None, QName=None, VarIdStr=""):
        self.Obj = Obj
        self.QName = QName
        self.VarIdStr = VarIdStr

        self.FlagsStream = ""
        self.HasKey = False
        self.HasQuestionId = False

    def SetQName(self, QName):
        self.QName = QName

    def SetVarIdStr(self, VarIdStr):
        self.VarIdStr = VarIdStr

    def SetFlagsStream(self, FlagsStream):
        self.FlagsStream = FlagsStream

    def SetHasKey(self, HasKey):
        self.HasKey = HasKey

    def SetHasQuestionId(self, HasQuestionId):
        self.HasQuestionId = HasQuestionId

    def GetInfo(self):
        return self.Obj


class IfrOpHeader:
    def __init__(self, OpHeader: EFI_IFR_OP_HEADER, OpCode=None, Length=0):
        self.OpHeader = OpHeader
        if OpCode != None:
            self.OpHeader.OpCode = OpCode

            self.OpHeader.Length = gOpcodeSizesScopeTable[OpCode].Size if Length == 0 else Length
            self.OpHeader.Scope = 1 if (gOpcodeSizesScopeTable[OpCode].Scope + gScopeCount > 0) else 0

    def GetLength(self):
        return self.OpHeader.Length

    def SetScope(self, Scope):
        self.OpHeader.Scope = Scope

    def UpdateHeader(self, Header):
        self.OpHeader = Header

    def IncLength(self, Size):
        self.OpHeader.Length += Size

    def DecLength(self, Size):
        self.OpHeader.Length -= Size

    def AdjustLength(self, BeforeSize, AfterSize):
        self.OpHeader.Length -= BeforeSize
        self.OpHeader.Length += AfterSize
        self.OpHeader.Length += 1

    def GetOpCode(self):
        return self.OpHeader.OpCode


class IfrStatementHeader:
    def __init__(self, sHeader: EFI_IFR_STATEMENT_HEADER):
        self.sHeader = sHeader
        self.sHeader.Help = EFI_STRING_ID_INVALID
        self.sHeader.Prompt = EFI_STRING_ID_INVALID

    def GetStatementHeader(self):
        return self.sHeader

    def SetPrompt(self, Prompt):
        self.sHeader.Prompt = Prompt

    def SetHelp(self, Help):
        self.sHeader.Help = Help


class IfrMinMaxStepData:
    def __init__(self, MinMaxStepData, NumericOpcode=False):
        self.MinMaxStepData = MinMaxStepData
        self.MinMaxStepData.MinValue = 0
        self.MinMaxStepData.MaxValue = 0
        self.MinMaxStepData.Step = 0
        self.ValueIsSet = False
        self.IsNumeric = NumericOpcode

    def SetMinMaxStepData(self, MinValue, MaxValue, Step):
        if self.ValueIsSet == False:
            self.MinMaxStepData.MinValue = MinValue
            self.MinMaxStepData.MaxValue = MaxValue
            self.MinMaxStepData.Step = Step

            self.ValueIsSet = True
        else:
            if MinValue < self.MinMaxStepData.MinValue:
                self.MinMaxStepData.MinValue = MinValue
            if MaxValue > self.MinMaxStepData.MaxValue:
                self.MinMaxStepData.MaxValue = MaxValue
            self.MinMaxStepData.Step = Step

    def IsNumericOpcode(self):
        return self.IsNumeric

    def UpdateIfrMinMaxStepData(self, MinMaxStepData):
        self.MinMaxStepData = MinMaxStepData

    def GetMinData(self):
        return self.MinMaxStepData.MinValue

    def GetMaxData(self):
        return self.MinMaxStepData.MaxValue

    def GetStepData(self):
        return self.MinMaxStepData.Step


class IfrFormSet(IfrLine, IfrOpHeader):
    def __init__(self, Size):
        self.FormSet = EFI_IFR_FORM_SET()
        self.ClassGuid = []
        IfrOpHeader.__init__(self, self.FormSet.Header, EFI_IFR_FORM_SET_OP, Size)
        self.FormSet.Help = EFI_STRING_ID_INVALID
        self.FormSet.FormSetTitle = EFI_STRING_ID_INVALID
        self.FormSet.Flags = 0
        self.FormSet.Guid = EFI_GUID(0, 0, 0, GuidArray(0, 0, 0, 0, 0, 0, 0, 0))

    def SetGuid(self, Guid):
        self.FormSet.Guid = Guid

    def GetGuid(self):
        return self.FormSet.Guid

    def SetFormSetTitle(self, FormSetTitle):
        self.FormSet.FormSetTitle = FormSetTitle

    def GetFormSetTitle(self):
        return self.FormSet.FormSetTitle

    def SetHelp(self, Help):
        self.FormSet.Help = Help

    def GetHelp(self):
        return self.FormSet.Help

    def SetClassGuid(self, Guid):
        self.ClassGuid.append(Guid)
        self.FormSet.Flags += 1

    def GetClassGuid(self):
        return self.ClassGuid

    def GetFlags(self):
        return self.FormSet.Flags

    def GetInfo(self):
        return self.FormSet


class IfrOneOfOption(IfrLine, IfrOpHeader):
    def __init__(self, ValueType, ValueList):
        Nums = len(ValueList)
        self.OneOfOption = Refine_EFI_IFR_ONE_OF_OPTION(ValueType, Nums)
        IfrOpHeader.__init__(self, self.OneOfOption.Header, EFI_IFR_ONE_OF_OPTION_OP, sizeof(self.OneOfOption))
        self.OneOfOption.Flags = 0
        self.OneOfOption.Option = EFI_STRING_ID_INVALID
        self.OneOfOption.Type = EFI_IFR_TYPE_OTHER
        self.ValueType = ValueType
        if ValueList != []:
            ArrayType = TypeDict[ValueType] * Nums
            ValueArray = ArrayType()
            for i in range(0, len(ValueList)):
                ValueArray[i] = ValueList[i]
            self.OneOfOption.Value = ValueArray

        self.IfrOptionKey = None
        self.FlagsStream = ""
        self.ValueStream = ""

    def SetOption(self, Option):
        self.OneOfOption.Option = Option

    def SetType(self, Type):
        self.OneOfOption.Type = Type

    def SetIfrOptionKey(self, IfrOptionKey):
        self.IfrOptionKey = IfrOptionKey

    def SetFlagsStream(self, FlagsStream):
        self.FlagsStream = FlagsStream

    def SetValueStream(self, ValueStream):
        self.ValueStream = ValueStream

    def GetValueType(self):
        return self.ValueType

    def SetValue(self, ValueList):
        ArrayType = TypeDict[self.ValueType] * (len(ValueList))
        ValueArray = ArrayType()
        for i in range(0, len(ValueList)):
            ValueArray[i] = ValueList[i]
        self.OneOfOption.Value = ValueArray

    def GetFlags(self):
        return self.OneOfOption.Flags

    def SetFlags(self, LFlags):
        self.OneOfOption.Flags = 0
        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, EFI_IFR_OPTION_DEFAULT)
        if Ret:
            self.OneOfOption.Flags |= EFI_IFR_OPTION_DEFAULT

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, EFI_IFR_OPTION_DEFAULT_MFG)
        if Ret:
            self.OneOfOption.Flags |= EFI_IFR_OPTION_DEFAULT_MFG

        if LFlags == EFI_IFR_TYPE_NUM_SIZE_8:
            LFlags = _FLAG_CLEAR(LFlags, EFI_IFR_TYPE_NUM_SIZE_8)
            self.OneOfOption.Flags |= EFI_IFR_TYPE_NUM_SIZE_8

        elif LFlags == EFI_IFR_TYPE_NUM_SIZE_16:
            LFlags = _FLAG_CLEAR(LFlags, EFI_IFR_TYPE_NUM_SIZE_16)
            self.OneOfOption.Flags |= EFI_IFR_TYPE_NUM_SIZE_16

        elif LFlags == EFI_IFR_TYPE_NUM_SIZE_32:
            LFlags = _FLAG_CLEAR(LFlags, EFI_IFR_TYPE_NUM_SIZE_32)
            self.OneOfOption.Flags |= EFI_IFR_TYPE_NUM_SIZE_32

        elif LFlags == EFI_IFR_TYPE_NUM_SIZE_64:
            LFlags = _FLAG_CLEAR(LFlags, EFI_IFR_TYPE_NUM_SIZE_64)
            self.OneOfOption.Flags |= EFI_IFR_TYPE_NUM_SIZE_64

        elif LFlags == EFI_IFR_TYPE_BOOLEAN:
            LFlags = _FLAG_CLEAR(LFlags, EFI_IFR_TYPE_BOOLEAN)
            self.OneOfOption.Flags |= EFI_IFR_TYPE_BOOLEAN

        elif LFlags == EFI_IFR_TYPE_TIME:
            LFlags = _FLAG_CLEAR(LFlags, EFI_IFR_TYPE_TIME)
            self.OneOfOption.Flags |= EFI_IFR_TYPE_TIME

        elif LFlags == EFI_IFR_TYPE_DATE:
            LFlags = _FLAG_CLEAR(LFlags, EFI_IFR_TYPE_DATE)
            self.OneOfOption.Flags |= EFI_IFR_TYPE_DATE

        elif LFlags == EFI_IFR_TYPE_STRING:
            LFlags = _FLAG_CLEAR(LFlags, EFI_IFR_TYPE_STRING)
            self.OneOfOption.Flags |= EFI_IFR_TYPE_STRING

        elif LFlags == EFI_IFR_TYPE_OTHER:
            LFlags = _FLAG_CLEAR(LFlags, EFI_IFR_TYPE_OTHER)
            self.OneOfOption.Flags |= EFI_IFR_TYPE_OTHER

        return VfrReturnCode.VFR_RETURN_SUCCESS if LFlags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED

    def GetInfo(self):
        return self.OneOfOption


class IfrOptionKey(IfrLine, IfrOpHeader):
    def __init__(self, QuestionId, Type, OptionValue, KeyValue):
        self.OptionKey = Refine_EFI_IFR_GUID_OPTIONKEY(Type)
        IfrOpHeader.__init__(self, self.OptionKey.Header, EFI_IFR_GUID_OP, ctypes.sizeof(self.OptionKey))
        self.OptionKey.ExtendOpCode = EFI_IFR_EXTEND_OP_OPTIONKEY
        self.OptionKey.Guid = EFI_IFR_FRAMEWORK_GUID
        self.OptionKey.QuestionId = QuestionId
        self.OptionKey.OptionValue = OptionValue
        self.OptionKey.KeyValue = KeyValue

    def GetInfo(self):
        return self.OptionKey


class IfrClass(IfrLine, IfrOpHeader):
    def __init__(
        self,
    ):
        self.Class = EFI_IFR_GUID_CLASS()  # static guid
        IfrOpHeader.__init__(self, self.Class.Header, EFI_IFR_GUID_OP, ctypes.sizeof(EFI_IFR_GUID_CLASS))
        self.Class.ExtendOpCode = EFI_IFR_EXTEND_OP_CLASS
        self.Class.Guid = EFI_IFR_TIANO_GUID
        self.Class.Class = EFI_NON_DEVICE_CLASS

        self.HasSubClass = False

    def SetClass(self, Class):
        self.Class.Class = Class

    def GetInfo(self):
        return self.Class


class IfrSubClass(IfrLine, IfrOpHeader):
    def __init__(
        self,
    ):
        self.SubClass = EFI_IFR_GUID_SUBCLASS()  # static guid
        IfrOpHeader.__init__(self, self.SubClass.Header, EFI_IFR_GUID_OP, ctypes.sizeof(EFI_IFR_GUID_SUBCLASS))
        self.SubClass.ExtendOpCode = EFI_IFR_EXTEND_OP_SUBCLASS
        self.SubClass.Guid = EFI_IFR_TIANO_GUID
        self.SubClass.SubClass = EFI_SETUP_APPLICATION_SUBCLASS

    def SetSubClass(self, SubClass):
        self.SubClass.SubClass = SubClass

    def GetInfo(self):
        return self.SubClass


class IfrDefaultStore(IfrLine, IfrOpHeader):
    def __init__(self, TypeName=None):
        self.DefaultStore = EFI_IFR_DEFAULTSTORE()
        IfrOpHeader.__init__(self, self.DefaultStore.Header, EFI_IFR_DEFAULTSTORE_OP)
        self.DefaultStore.DefaultName = EFI_STRING_ID_INVALID
        self.DefaultStore.DefaultId = EFI_VARSTORE_ID_INVALID
        self.Type = TypeName
        self.HasAttr = False

    def SetDefaultName(self, DefaultName):
        self.DefaultStore.DefaultName = DefaultName

    def SetType(self, Type):
        self.Type = Type

    def SetDefaultId(self, DefaultId):
        self.DefaultStore.DefaultId = DefaultId

    def SetDefaultStore(self, DefaultStore: EFI_IFR_DEFAULTSTORE):
        self.DefaultStore = DefaultStore
        IfrOpHeader.__init__(self, self.DefaultStore.Header, EFI_IFR_DEFAULTSTORE_OP)

    def GetDefaultId(self):
        return self.DefaultStore.DefaultId

    def GetInfo(self):
        return self.DefaultStore


class IfrVarStore(IfrLine, IfrOpHeader):
    def __init__(self, TypeName, StoreName):
        Nums = len(StoreName)
        self.Varstore = Refine_EFI_IFR_VARSTORE(Nums)
        IfrOpHeader.__init__(self, self.Varstore.Header, EFI_IFR_VARSTORE_OP, sizeof(self.Varstore) + 1)
        self.Varstore.VarStoreId = EFI_VARSTORE_ID_INVALID
        self.Varstore.Size = 0
        ArrayType = c_ubyte * (Nums)
        ValueArray = ArrayType()
        for i in range(0, Nums):
            ValueArray[i] = ord(StoreName[i])
        self.Varstore.Name = ValueArray

        # info saved for yaml generation
        self.Type = TypeName
        self.HasVarStoreId = False

    def SetGuid(self, Guid):
        self.Varstore.Guid = Guid

    def SetSize(self, Size):
        self.Varstore.Size = Size

    def SetVarStoreId(self, VarStoreId):
        self.Varstore.VarStoreId = VarStoreId

    def SetHasVarStoreId(self, HasVarStoreId):
        self.HasVarStoreId = HasVarStoreId

    def GetInfo(self):
        return self.Varstore


class IfrVarStoreEfi(IfrLine, IfrOpHeader):
    def __init__(self, TypeName, StoreName):
        Nums = len(StoreName)
        self.VarStoreEfi = Refine_EFI_IFR_VARSTORE_EFI(Nums)
        IfrOpHeader.__init__(self, self.VarStoreEfi.Header, EFI_IFR_VARSTORE_EFI_OP, sizeof(self.VarStoreEfi) + 1)
        self.VarStoreEfi.VarStoreId = EFI_VAROFFSET_INVALID
        self.VarStoreEfi.Size = 0
        ArrayType = c_ubyte * (Nums)
        ValueArray = ArrayType()
        for i in range(0, Nums):
            ValueArray[i] = ord(StoreName[i])
        self.VarStoreEfi.Name = ValueArray

        # info saved for yaml generation
        self.Type = TypeName
        self.HasVarStoreId = False
        self.AttributesText = None

    def SetGuid(self, Guid):
        self.VarStoreEfi.Guid = Guid

    def SetSize(self, Size):
        self.VarStoreEfi.Size = Size

    def SetVarStoreId(self, VarStoreId):
        self.VarStoreEfi.VarStoreId = VarStoreId

    def SetAttributes(self, Attributes):
        self.VarStoreEfi.Attributes = Attributes

    def SetHasVarStoreId(self, HasVarStoreId):
        self.HasVarStoreId = HasVarStoreId

    def SetAttributesText(self, AttributesText):
        self.AttributesText = AttributesText

    def GetInfo(self):
        return self.VarStoreEfi


class IfrVarStoreNameValue(IfrLine, IfrOpHeader):
    def __init__(self, TypeName):
        self.VarStoreNameValue = EFI_IFR_VARSTORE_NAME_VALUE()
        IfrOpHeader.__init__(self, self.VarStoreNameValue.Header, EFI_IFR_VARSTORE_NAME_VALUE_OP)
        self.VarStoreNameValue.VarStoreId = EFI_VAROFFSET_INVALID

        self.Type = TypeName
        self.NameItemList = []
        self.HasVarStoreId = False

    def SetGuid(self, Guid):
        self.VarStoreNameValue.Guid = Guid

    def SetVarStoreId(self, VarStoreId):
        self.VarStoreNameValue.VarStoreId = VarStoreId

    def SetNameItemList(self, NameItem):
        self.NameItemList.append(NameItem)

    def SetHasVarStoreId(self, HasVarStoreId):
        self.HasVarStoreId = HasVarStoreId

    def GetInfo(self):
        return self.VarStoreNameValue


EFI_BITS_PER_UINT32 = 1 << EFI_BITS_SHIFT_PER_UINT32
EFI_FORM_ID_MAX = 0xFFFF

EFI_FREE_FORM_ID_BITMAP_SIZE = int((EFI_FORM_ID_MAX + 1) / EFI_BITS_PER_UINT32)


class IfrFormId:
    def __init__(self):
        self.FormIdBitMap = []
        for i in range(0, EFI_FREE_FORM_ID_BITMAP_SIZE):
            self.FormIdBitMap.append(0)

    def Clear(self):
        self.FormIdBitMap = []
        for i in range(0, EFI_FREE_FORM_ID_BITMAP_SIZE):
            self.FormIdBitMap.append(0)

    def CheckFormIdFree(self, FormId):
        Index = int(FormId / EFI_BITS_PER_UINT32)
        Offset = FormId % EFI_BITS_PER_UINT32

        return (self.FormIdBitMap[Index] & (0x80000000 >> Offset)) == 0

    def MarkFormIdUsed(self, FormId):
        Index = int(FormId / EFI_BITS_PER_UINT32)
        Offset = FormId % EFI_BITS_PER_UINT32
        self.FormIdBitMap[Index] |= 0x80000000 >> Offset


gIfrFormId = IfrFormId()


class IfrForm(IfrLine, IfrOpHeader):
    def __init__(self):
        self.Form = EFI_IFR_FORM()
        IfrOpHeader.__init__(self, self.Form.Header, EFI_IFR_FORM_OP)
        self.Form.FormId = 0
        self.Form.FormTitle = EFI_STRING_ID_INVALID

    def SetFormId(self, FormId):
        # FormId can't be 0.
        if FormId == 0:
            return VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
        if gIfrFormId.CheckFormIdFree(FormId) == False:
            return VfrReturnCode.VFR_RETURN_FORMID_REDEFINED
        self.Form.FormId = FormId
        gIfrFormId.MarkFormIdUsed(FormId)
        return VfrReturnCode.VFR_RETURN_SUCCESS

    def SetFormTitle(self, FormTitle):
        self.Form.FormTitle = FormTitle

    def GetInfo(self):
        return self.Form


class IfrFormMap(IfrLine, IfrOpHeader):
    def __init__(self):
        self.FormMap = EFI_IFR_FORM_MAP()
        self.MethodMapList = []  # EFI_IFR_FORM_MAP_METHOD()
        IfrOpHeader.__init__(self, self.FormMap.Header, EFI_IFR_FORM_MAP_OP)
        self.FormMap.FormId = 0

    def SetFormId(self, FormId):
        if FormId == 0:
            return VfrReturnCode.VFR_RETURN_INVALID_PARAMETER

        if gIfrFormId.CheckFormIdFree(FormId) == False:
            return VfrReturnCode.VFR_RETURN_FORMID_REDEFINED
        self.FormMap.FormId = FormId
        gIfrFormId.MarkFormIdUsed(FormId)
        return VfrReturnCode.VFR_RETURN_SUCCESS

    def SetFormMapMethod(self, MethodTitle, MethodGuid: EFI_GUID):
        MethodMap = EFI_IFR_FORM_MAP_METHOD()
        MethodMap.MethodTitle = MethodTitle
        MethodMap.MethodIdentifier = MethodGuid
        self.MethodMapList.append(MethodMap)

    def GetInfo(self):
        return self.FormMap

    def GetMethodMapList(self):
        return self.MethodMapList


class IfrEnd(IfrLine, IfrOpHeader):
    def __init__(self):
        self.End = EFI_IFR_END()
        IfrOpHeader.__init__(self, self.End.Header, EFI_IFR_END_OP)

    def GetInfo(self):
        return self.End


class IfrBanner(IfrLine, IfrOpHeader):
    def __init__(
        self,
    ):
        self.Banner = EFI_IFR_GUID_BANNER()
        IfrOpHeader.__init__(self, self.Banner.Header, EFI_IFR_GUID_OP, ctypes.sizeof(EFI_IFR_GUID_BANNER))
        self.Banner.ExtendOpCode = EFI_IFR_EXTEND_OP_BANNER
        self.Banner.Guid = EFI_IFR_TIANO_GUID

        self.HasTimeOut = False

    def SetTitle(self, StringId):
        self.Banner.Title = StringId

    def SetLine(self, Line):
        self.Banner.LineNumber = Line

    def SetAlign(self, Align):
        self.Banner.Alignment = Align

    def SetHasTimeOut(self, HasTimeOut):
        self.HasTimeOut = HasTimeOut

    def GetInfo(self):
        return self.Banner


class IfrVarEqName(IfrLine, IfrOpHeader):
    def __init__(self, QuestionId, NameId):
        self.VarEqName = EFI_IFR_GUID_VAREQNAME()
        IfrOpHeader.__init__(self, self.VarEqName.Header, EFI_IFR_GUID_OP, ctypes.sizeof(EFI_IFR_GUID_VAREQNAME))
        self.VarEqName.ExtendOpCode = EFI_IFR_EXTEND_OP_VAREQNAME
        self.VarEqName.Guid = EFI_IFR_FRAMEWORK_GUID
        self.VarEqName.QuestionId = QuestionId
        self.VarEqName.NameId = NameId

    def GetInfo(self):
        return self.VarEqName


class IfrTimeout(IfrLine, IfrOpHeader):
    def __init__(self, Timeout=0):
        self.Timeout = EFI_IFR_GUID_TIMEOUT()
        IfrOpHeader.__init__(self, self.Timeout.Header, EFI_IFR_GUID_OP, ctypes.sizeof(EFI_IFR_GUID_TIMEOUT))
        self.Timeout.ExtendOpCode = EFI_IFR_EXTEND_OP_TIMEOUT
        self.Timeout.Guid = EFI_IFR_TIANO_GUID
        self.Timeout.TimeOut = Timeout

    def SetTimeout(self, Timeout):
        self.Timeout.TimeOut = Timeout

    def GetInfo(self):
        return self.Timeout


class IfrLabel(IfrLine, IfrOpHeader):
    def __init__(
        self,
    ):
        self.Label = EFI_IFR_GUID_LABEL()
        IfrOpHeader.__init__(self, self.Label.Header, EFI_IFR_GUID_OP, ctypes.sizeof(EFI_IFR_GUID_LABEL))
        self.Label.ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL
        self.Label.Guid = EFI_IFR_TIANO_GUID

    def SetNumber(self, Number):
        self.Label.Number = Number

    def GetInfo(self):
        return self.Label


class IfrRule(IfrLine, IfrOpHeader):
    def __init__(self, RuleName=None):
        self.Rule = EFI_IFR_RULE()
        self.RuleName = RuleName
        IfrOpHeader.__init__(self, self.Rule.Header, EFI_IFR_RULE_OP)
        self.Rule.RuleId = EFI_RULE_ID_INVALID

    def SetRuleId(self, RuleId):
        self.Rule.RuleId = RuleId

    def GetRuleName(self):
        return self.RuleName

    def SetRuleName(self, RuleName):
        self.RuleName = RuleName

    def GetInfo(self):
        return self.Rule


def _FLAG_TEST_AND_CLEAR(Flags, Mask):
    Ret = Flags & Mask
    Flags &= ~Mask
    return Flags, Ret


def _FLAG_CLEAR(Flags, Mask):
    Flags &= ~Mask
    return Flags


class IfrSubtitle(IfrLine, IfrOpHeader, IfrStatementHeader):
    def __init__(
        self,
    ):
        self.Subtitle = EFI_IFR_SUBTITLE()

        IfrOpHeader.__init__(self, self.Subtitle.Header, EFI_IFR_SUBTITLE_OP)
        IfrStatementHeader.__init__(self, self.Subtitle.Statement)
        self.Subtitle.Flags = 0

        self.FlagsStream = ""

    def SetFlags(self, Flags):
        Flags, Result = _FLAG_TEST_AND_CLEAR(Flags, EFI_IFR_FLAGS_HORIZONTAL)
        if Result:
            self.Subtitle.Flags |= EFI_IFR_FLAGS_HORIZONTAL

        return VfrReturnCode.VFR_RETURN_SUCCESS if Flags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED

    def SetFlagsStream(self, FlagsStream):
        self.FlagsStream = FlagsStream

    def GetInfo(self):
        return self.Subtitle


class IfrImage(IfrLine, IfrOpHeader):
    def __init__(
        self,
    ):
        self.Image = EFI_IFR_IMAGE()
        IfrOpHeader.__init__(self, self.Image.Header, EFI_IFR_IMAGE_OP)
        self.Image.Id = EFI_IMAGE_ID_INVALID

    def SetImageId(self, ImageId):
        self.Image.Id = ImageId

    def GetInfo(self):
        return self.Image


class IfrLocked(IfrLine, IfrOpHeader):
    def __init__(
        self,
    ):
        self.Lock = EFI_IFR_LOCKED()
        IfrOpHeader.__init__(self, self.Lock.Header, EFI_IFR_LOCKED_OP)

    def GetInfo(self):
        return self.Lock


class IfrModal(IfrLine, IfrOpHeader):
    def __init__(
        self,
    ):
        self.Modal = EFI_IFR_MODAL_TAG()
        IfrOpHeader.__init__(self, self.Modal.Header, EFI_IFR_MODAL_TAG_OP)

    def GetInfo(self):
        return self.Modal


EFI_IFR_QUESTION_FLAG_DEFAULT = 0


class IfrQuestionHeader(IfrStatementHeader):
    def __init__(self, QHeader, Flags=EFI_IFR_QUESTION_FLAG_DEFAULT):
        self.QHeader = QHeader
        IfrStatementHeader.__init__(self, self.QHeader.Header)
        self.QHeader.QuestionId = EFI_QUESTION_ID_INVALID
        self.QHeader.VarStoreId = EFI_VARSTORE_ID_INVALID
        self.QHeader.VarStoreInfo.VarName = EFI_STRING_ID_INVALID
        self.QHeader.VarStoreInfo.VarOffset = EFI_VAROFFSET_INVALID
        self.QHeader.Flags = Flags

    def GetQuestionId(self):
        return self.QHeader.QuestionId

    def SetQuestionId(self, QuestionId):
        self.QHeader.QuestionId = QuestionId

    def GetVarStoreId(self):
        return self.QHeader.VarStoreId

    def SetVarStoreInfo(self, BaseInfo):
        self.QHeader.VarStoreId = BaseInfo.VarStoreId
        self.QHeader.VarStoreInfo.VarName = BaseInfo.Info.VarName
        self.QHeader.VarStoreInfo.VarOffset = BaseInfo.Info.VarOffset

    def GetVarStoreInfo(self, Info):  # Bug
        Info.VarStoreId = self.QHeader.VarStoreId
        Info.VarStoreInfo = self.QHeader.VarStoreInfo
        return Info

    def GetQFlags(self):
        return self.QHeader.Flags

    def SetFlags(self, Flags):
        Flags, Ret = _FLAG_TEST_AND_CLEAR(Flags, EFI_IFR_FLAG_READ_ONLY)
        if Ret:
            self.QHeader.Flags |= EFI_IFR_FLAG_READ_ONLY

        Flags = _FLAG_CLEAR(Flags, 0x02)

        Flags, Ret = _FLAG_TEST_AND_CLEAR(Flags, EFI_IFR_FLAG_CALLBACK)
        if Ret:
            self.QHeader.Flags |= EFI_IFR_FLAG_CALLBACK

        # ignore NVAccessFlag
        Flags = _FLAG_CLEAR(Flags, 0x08)

        Flags, Ret = _FLAG_TEST_AND_CLEAR(Flags, EFI_IFR_FLAG_RESET_REQUIRED)
        if Ret:
            self.QHeader.Flags |= EFI_IFR_FLAG_RESET_REQUIRED

        Flags, Ret = _FLAG_TEST_AND_CLEAR(Flags, EFI_IFR_FLAG_RECONNECT_REQUIRED)
        if Ret:
            self.QHeader.Flags |= EFI_IFR_FLAG_RECONNECT_REQUIRED

        # Set LateCheck Flag to compatible for framework flag
        # but it uses 0x20 as its flag, if in the future UEFI may take this flag

        Flags, Ret = _FLAG_TEST_AND_CLEAR(Flags, 0x20)
        if Ret:
            self.QHeader.Flags |= 0x20

        Flags, Ret = _FLAG_TEST_AND_CLEAR(Flags, EFI_IFR_FLAG_OPTIONS_ONLY)
        if Ret:
            self.QHeader.Flags |= EFI_IFR_FLAG_OPTIONS_ONLY

        return VfrReturnCode.VFR_RETURN_SUCCESS if Flags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED

    def UpdateIfrQuestionHeader(self, qHeader):
        self.QHeader = qHeader


class IfrRef(IfrLine, IfrBaseInfo, IfrOpHeader, IfrQuestionHeader):
    def __init__(self, QName=None, VarIdStr=""):
        self.Ref = EFI_IFR_REF()
        IfrBaseInfo.__init__(self, self.Ref, QName, VarIdStr)
        IfrOpHeader.__init__(self, self.Ref.Header, EFI_IFR_REF_OP)
        IfrQuestionHeader.__init__(self, self.Ref.Question)
        self.Ref.FormId = 0

    def SetFormId(self, FormId):
        self.Ref.FormId = FormId


class IfrRef2(IfrLine, IfrBaseInfo, IfrOpHeader, IfrQuestionHeader):
    def __init__(self, QName=None, VarIdStr=""):
        self.Ref2 = EFI_IFR_REF2()
        IfrBaseInfo.__init__(self, self.Ref2, QName, VarIdStr)
        IfrOpHeader.__init__(self, self.Ref2.Header, EFI_IFR_REF_OP, sizeof(EFI_IFR_REF2))
        IfrQuestionHeader.__init__(self, self.Ref2.Question)
        self.Ref2.FormId = 0
        self.Ref2.QuestionId = EFI_QUESTION_ID_INVALID

    def SetFormId(self, FormId):
        self.Ref2.FormId = FormId

    def SetQId(self, QuestionId):
        self.Ref2.QuestionId = QuestionId


class IfrRef3(IfrLine, IfrBaseInfo, IfrOpHeader, IfrQuestionHeader):
    def __init__(self, QName=None, VarIdStr=""):
        self.Ref3 = EFI_IFR_REF3()
        IfrBaseInfo.__init__(self, self.Ref3, QName, VarIdStr)
        IfrOpHeader.__init__(self, self.Ref3.Header, EFI_IFR_REF_OP, sizeof(EFI_IFR_REF3))
        IfrQuestionHeader.__init__(self, self.Ref3.Question)
        self.Ref3.FormId = 0
        self.Ref3.QuestionId = EFI_QUESTION_ID_INVALID
        EFI_IFR_DEFAULT_GUID = EFI_GUID(0, 0, 0, GuidArray(0, 0, 0, 0, 0, 0, 0, 0))
        self.Ref3.FormSetId = EFI_IFR_DEFAULT_GUID

    def SetFormId(self, FormId):
        self.Ref3.FormId = FormId

    def SetQId(self, QuestionId):
        self.Ref3.QuestionId = QuestionId

    def SetFormSetId(self, FormSetId):
        self.Ref3.FormSetId = FormSetId


class IfrRef4(IfrLine, IfrBaseInfo, IfrOpHeader, IfrQuestionHeader):
    def __init__(self, QName=None, VarIdStr=""):
        self.Ref4 = EFI_IFR_REF4()
        IfrBaseInfo.__init__(self, self.Ref4, QName, VarIdStr)
        IfrOpHeader.__init__(self, self.Ref4.Header, EFI_IFR_REF_OP, sizeof(EFI_IFR_REF4))
        IfrQuestionHeader.__init__(self, self.Ref4.Question)
        self.Ref4.FormId = 0
        self.Ref4.QuestionId = EFI_QUESTION_ID_INVALID
        EFI_IFR_DEFAULT_GUID = EFI_GUID(0, 0, 0, GuidArray(0, 0, 0, 0, 0, 0, 0, 0))
        self.Ref4.FormSetId = EFI_IFR_DEFAULT_GUID
        self.Ref4.DevicePath = EFI_STRING_ID_INVALID

    def SetFormId(self, FormId):
        self.Ref4.FormId = FormId

    def SetQId(self, QuestionId):
        self.Ref4.QuestionId = QuestionId

    def SetFormSetId(self, FormSetId):
        self.Ref4.FormSetId = FormSetId

    def SetDevicePath(self, DevicePath):
        self.Ref4.DevicePath = DevicePath


class IfrRef5(IfrLine, IfrBaseInfo, IfrOpHeader, IfrQuestionHeader):
    def __init__(self, QName=None, VarIdStr=""):
        self.Ref5 = EFI_IFR_REF5()
        IfrBaseInfo.__init__(self, self.Ref5, QName, VarIdStr)
        IfrOpHeader.__init__(self, self.Ref5.Header, EFI_IFR_REF_OP, sizeof(EFI_IFR_REF5))
        IfrQuestionHeader.__init__(self, self.Ref5.Question)


class IfrAction(IfrLine, IfrBaseInfo, IfrOpHeader, IfrQuestionHeader):
    def __init__(self, QName=None, VarIdStr=""):
        self.Action = EFI_IFR_ACTION()
        IfrBaseInfo.__init__(self, self.Action, QName, VarIdStr)
        IfrOpHeader.__init__(self, self.Action.Header, EFI_IFR_ACTION_OP)
        IfrQuestionHeader.__init__(self, self.Action.Question)
        self.Action.QuestionConfig = EFI_STRING_ID_INVALID

    def SetQuestionConfig(self, QuestionConfig):
        self.Action.QuestionConfig = QuestionConfig


class IfrText(IfrLine, IfrOpHeader, IfrStatementHeader):
    def __init__(
        self,
    ):
        self.Text = EFI_IFR_TEXT()
        IfrOpHeader.__init__(self, self.Text.Header, EFI_IFR_TEXT_OP)
        IfrStatementHeader.__init__(self, self.Text.Statement)
        self.Text.TextTwo = EFI_STRING_ID_INVALID

    def SetTextTwo(self, StringId):
        self.Text.TextTwo = StringId

    def GetInfo(self):
        return self.Text


class IfrGuid(IfrLine, IfrOpHeader):
    def __init__(self, Size, Data=None):
        self.Guid = EFI_IFR_GUID()
        self.Data = Data  # databuffer is saved here
        IfrOpHeader.__init__(self, self.Guid.Header, EFI_IFR_GUID_OP, ctypes.sizeof(EFI_IFR_GUID) + Size)
        EFI_IFR_DEFAULT_GUID = EFI_GUID(0, 0, 0, GuidArray(0, 0, 0, 0, 0, 0, 0, 0))
        self.Guid.Guid = EFI_IFR_DEFAULT_GUID

    def SetGuid(self, Guid):
        self.Guid.Guid = Guid

    def SetData(self, Data):
        self.Data = Data

    def GetData(self):
        return self.Data

    def GetInfo(self):  #
        return self.Guid


class IfrExtensionGuid(IfrLine, IfrOpHeader):
    def __init__(self, Size=0, TypeName="", ArraySize=0, Data=None):
        self.Guid = EFI_IFR_GUID()
        if ArraySize != 0:
            self.DataType = TypeName + "[" + str(ArraySize) + "]"
        else:
            self.DataType = TypeName
        self.FieldList = []
        self.Data = Data  # databuffer is saved here
        IfrOpHeader.__init__(self, self.Guid.Header, EFI_IFR_GUID_OP, ctypes.sizeof(EFI_IFR_GUID) + Size)
        EFI_IFR_DEFAULT_GUID = EFI_GUID(0, 0, 0, GuidArray(0, 0, 0, 0, 0, 0, 0, 0))
        self.Guid.Guid = EFI_IFR_DEFAULT_GUID

    def SetGuid(self, Guid):
        self.Guid.Guid = Guid

    def SetData(self, Data):
        self.Data = Data

    def GetDataType(self):
        return self.DataType

    def GetFieldList(self):
        return self.FieldList

    def SetFieldList(self, TFName, TFValue):
        self.FieldList.append([TFName, TFValue])

    def GetData(self):
        return self.Data

    def GetInfo(self):  #
        return self.Guid


class IfrOrderedList(IfrLine, IfrBaseInfo, IfrOpHeader, IfrQuestionHeader):
    def __init__(self, QName=None, VarIdStr=""):
        self.OrderedList = EFI_IFR_ORDERED_LIST()
        IfrBaseInfo.__init__(self, self.OrderedList, QName, VarIdStr)
        IfrOpHeader.__init__(self, self.OrderedList.Header, EFI_IFR_ORDERED_LIST_OP)
        IfrQuestionHeader.__init__(self, self.OrderedList.Question)
        self.OrderedList.MaxContainers = 0
        self.OrderedList.Flags = 0

        self.HasMaxContainers = False

    def GetQuestion(self):
        return self

    def SetQHeaderFlags(self, Flags):
        IfrQuestionHeader.SetFlags(self, Flags)

    def SetMaxContainers(self, MaxContainers):
        self.OrderedList.MaxContainers = MaxContainers

    def SetHasMaxContainers(self, HasMaxContainers):
        self.HasMaxContainers = HasMaxContainers

    def SetFlags(self, HFlags, LFlags):
        ReturnCode = IfrQuestionHeader.SetFlags(self, HFlags)
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode
        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, EFI_IFR_UNIQUE_SET)
        if Ret:
            self.OrderedList.Flags |= EFI_IFR_UNIQUE_SET

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, EFI_IFR_NO_EMPTY_SET)
        if Ret:
            self.OrderedList.Flags |= EFI_IFR_NO_EMPTY_SET

        return VfrReturnCode.VFR_RETURN_SUCCESS if LFlags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED


class IfrString(IfrLine, IfrBaseInfo, IfrOpHeader, IfrQuestionHeader):
    def __init__(self, QName=None, VarIdStr=""):
        self.Str = EFI_IFR_STRING()
        IfrBaseInfo.__init__(self, self.Str, QName, VarIdStr)
        IfrOpHeader.__init__(self, self.Str.Header, EFI_IFR_STRING_OP)
        IfrQuestionHeader.__init__(self, self.Str.Question)
        self.Str.Flags = 0
        self.Str.MinSize = 0
        self.Str.MaxSize = 0

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
            self.Str.Flags |= EFI_IFR_STRING_MULTI_LINE

        return VfrReturnCode.VFR_RETURN_SUCCESS if LFlags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED

    def SetMinSize(self, MinSize):
        self.Str.MinSize = MinSize

    def SetMaxSize(self, MaxSize):
        self.Str.MaxSize = MaxSize


class IfrPassword(IfrLine, IfrBaseInfo, IfrOpHeader, IfrQuestionHeader):
    def __init__(self, QName=None, VarIdStr=""):
        self.Password = EFI_IFR_PASSWORD()
        IfrBaseInfo.__init__(self, self.Password, QName, VarIdStr)
        IfrOpHeader.__init__(self, self.Password.Header, EFI_IFR_PASSWORD_OP)
        IfrQuestionHeader.__init__(self, self.Password.Question)
        self.Password.MinSize = 0
        self.Password.MaxSize = 0

    def GetQuestion(self):
        return self

    def SetQHeaderFlags(self, Flags):
        IfrQuestionHeader.SetFlags(self, Flags)

    def SetMinSize(self, MinSize):
        self.Password.MinSize = MinSize

    def SetMaxSize(self, MaxSize):
        self.Password.MaxSize = MaxSize


class IfrDefault(IfrLine, IfrOpHeader):
    def __init__(self, ValueType, ValueList, DefaultId=EFI_HII_DEFAULT_CLASS_STANDARD, Type=EFI_IFR_TYPE_OTHER):
        Nums = len(ValueList)
        self.Default = Refine_EFI_IFR_DEFAULT(ValueType, Nums)
        IfrOpHeader.__init__(self, self.Default.Header, EFI_IFR_DEFAULT_OP, sizeof(self.Default))
        self.Default.Type = Type
        self.Default.DefaultId = DefaultId

        self.ValueType = ValueType
        self.DefaultStore = ""
        self.ValueStream = ""

        if ValueList != []:
            ArrayType = TypeDict[ValueType] * Nums
            ValueArray = ArrayType()
            for i in range(0, len(ValueList)):
                ValueArray[i] = ValueList[i]
            self.Default.Value = ValueArray

    def SetDefaultId(self, DefaultId):
        self.Default.DefaultId = DefaultId

    def GetValueType(self):
        return self.ValueType

    def SetType(self, Type):
        self.Default.Type = Type

    def SetValue(self, ValueList):
        ArrayType = TypeDict[self.ValueType] * (len(ValueList))
        ValueArray = ArrayType()
        for i in range(0, len(ValueList)):
            ValueArray[i] = ValueList[i]
        self.Default.Value = ValueArray

    def SetDefaultStore(self, DefaultStore):
        self.DefaultStore = DefaultStore

    def SetValueStream(self, ValueStream):
        self.ValueStream = ValueStream

    def GetInfo(self):
        return self.Default


class IfrDefault2(IfrLine, IfrOpHeader):
    def __init__(self, DefaultId=EFI_HII_DEFAULT_CLASS_STANDARD, Type=EFI_IFR_TYPE_OTHER):
        self.Default = EFI_IFR_DEFAULT_2()
        IfrOpHeader.__init__(self, self.Default.Header, EFI_IFR_DEFAULT_OP, sizeof(EFI_IFR_DEFAULT_2))
        self.Default.Type = Type
        self.Default.DefaultId = DefaultId

        self.DefaultStore = ""

    def SetDefaultId(self, DefaultId):
        self.Default.DefaultId = DefaultId

    def SetType(self, Type):
        self.Default.Type = Type

    def SetDefaultStore(self, DefaultStore):
        self.DefaultStore = DefaultStore

    def GetInfo(self):
        return self.Default


class IfrInconsistentIf(IfrLine, IfrOpHeader):
    def __init__(self):
        self.InconsistentIf = EFI_IFR_INCONSISTENT_IF()
        IfrOpHeader.__init__(self, self.InconsistentIf.Header, EFI_IFR_INCONSISTENT_IF_OP)
        self.InconsistentIf.Error = EFI_STRING_ID_INVALID

    def SetError(self, Error):
        self.InconsistentIf.Error = Error

    def GetInfo(self):
        return self.InconsistentIf


class IfrInconsistentIf2(IfrLine, IfrOpHeader):
    def __init__(self):
        self.InconsistentIf = EFI_IFR_INCONSISTENT_IF()
        IfrOpHeader.__init__(self, self.InconsistentIf.Header, EFI_IFR_INCONSISTENT_IF_OP)
        self.InconsistentIf.Error = EFI_STRING_ID_INVALID

    def SetError(self, Error):
        self.InconsistentIf.Error = Error

    def GetInfo(self):
        return self.InconsistentIf


class IfrNoSubmitIf(IfrLine, IfrOpHeader):
    def __init__(self):
        self.NoSubmitIf = EFI_IFR_NO_SUBMIT_IF()
        IfrOpHeader.__init__(self, self.NoSubmitIf.Header, EFI_IFR_NO_SUBMIT_IF_OP)
        self.NoSubmitIf.Error = EFI_STRING_ID_INVALID

    def SetError(self, Error):
        self.NoSubmitIf.Error = Error

    def GetInfo(self):
        return self.NoSubmitIf


class IfrDisableIf(IfrLine, IfrOpHeader):
    def __init__(self):
        self.DisableIf = EFI_IFR_DISABLE_IF()
        IfrOpHeader.__init__(self, self.DisableIf.Header, EFI_IFR_DISABLE_IF_OP)

    def GetInfo(self):
        return self.DisableIf


class IfrSuppressIf(IfrLine, IfrOpHeader):
    def __init__(self):
        self.SuppressIf = EFI_IFR_SUPPRESS_IF()
        IfrOpHeader.__init__(self, self.SuppressIf.Header, EFI_IFR_SUPPRESS_IF_OP)

    def GetInfo(self):
        return self.SuppressIf


class IfrGrayOutIf(IfrLine, IfrOpHeader):
    def __init__(self):
        self.GrayOutIf = EFI_IFR_GRAY_OUT_IF()
        IfrOpHeader.__init__(self, self.GrayOutIf.Header, EFI_IFR_GRAY_OUT_IF_OP)

    def GetInfo(self):
        return self.GrayOutIf


class IfrValue(IfrLine, IfrOpHeader):
    def __init__(self):
        self.Value = EFI_IFR_VALUE()
        IfrOpHeader.__init__(self, self.Value.Header, EFI_IFR_VALUE_OP)

    def GetInfo(self):
        return self.Value


class IfrRead(IfrLine, IfrOpHeader):
    def __init__(self):
        self.Read = EFI_IFR_READ()
        IfrOpHeader.__init__(self, self.Read.Header, EFI_IFR_READ_OP)

    def GetInfo(self):
        return self.Read


class IfrWrite(IfrLine, IfrOpHeader):
    def __init__(self):
        self.Write = EFI_IFR_WRITE()
        IfrOpHeader.__init__(self, self.Write.Header, EFI_IFR_WRITE_OP)

    def GetInfo(self):
        return self.Write


class IfrWarningIf(IfrLine, IfrOpHeader):
    def __init__(self):
        self.WarningIf = EFI_IFR_WARNING_IF()
        IfrOpHeader.__init__(self, self.WarningIf.Header, EFI_IFR_WARNING_IF_OP)
        self.WarningIf.Warning = EFI_STRING_ID_INVALID
        self.WarningIf.TimeOut = 0

        self.HasTimeOut = False

    def SetWarning(self, Warning):
        self.WarningIf.Warning = Warning

    def SetHasHasTimeOut(self, HasTimeOut):
        self.HasTimeOut = HasTimeOut

    def SetTimeOut(self, TimeOut):
        self.WarningIf.TimeOut = TimeOut

    def GetInfo(self):
        return self.WarningIf


class IfrRefresh(IfrLine, IfrOpHeader):
    def __init__(self):
        self.Refresh = EFI_IFR_REFRESH()
        IfrOpHeader.__init__(self, self.Refresh.Header, EFI_IFR_REFRESH_OP)
        self.Refresh.RefreshInterval = 0

    def SetRefreshInterval(self, RefreshInterval):
        self.Refresh.RefreshInterval = RefreshInterval

    def GetInfo(self):
        return self.Refresh


class IfrRefreshId(IfrLine, IfrOpHeader):
    def __init__(self):
        self.RefreshId = EFI_IFR_REFRESH_ID()
        IfrOpHeader.__init__(self, self.RefreshId.Header, EFI_IFR_REFRESH_ID_OP)
        self.RefreshId.RefreshEventGroupId = EFI_GUID(0, 0, 0, GuidArray(0, 0, 0, 0, 0, 0, 0, 0))

    def SetRefreshEventGroutId(self, RefreshEventGroupId):
        self.RefreshId.RefreshEventGroupId = RefreshEventGroupId

    def GetInfo(self):
        return self.RefreshId


class IfrVarStoreDevice(IfrLine, IfrOpHeader):
    def __init__(self):
        self.VarStoreDevice = EFI_IFR_VARSTORE_DEVICE()
        IfrOpHeader.__init__(self, self.VarStoreDevice.Header, EFI_IFR_VARSTORE_DEVICE_OP)
        self.VarStoreDevice.DevicePath = EFI_STRING_ID_INVALID

    def SetDevicePath(self, DevicePath):
        self.VarStoreDevice.DevicePath = DevicePath

    def GetInfo(self):
        return self.VarStoreDevice


class IfrDate(IfrLine, IfrBaseInfo, IfrOpHeader, IfrQuestionHeader):
    def __init__(self, QName=None, VarIdStr=""):
        self.Date = EFI_IFR_DATE()
        IfrBaseInfo.__init__(self, self.Date, QName, VarIdStr)
        IfrOpHeader.__init__(self, self.Date.Header, EFI_IFR_DATE_OP)
        IfrQuestionHeader.__init__(self, self.Date.Question)
        self.Date.Flags = 0

    def SetFlags(self, HFlags, LFlags):
        ReturnCode = IfrQuestionHeader.SetFlags(self, HFlags)
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, EFI_QF_DATE_YEAR_SUPPRESS)
        if Ret:
            self.Date.Flags |= EFI_QF_DATE_YEAR_SUPPRESS

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, EFI_QF_DATE_MONTH_SUPPRESS)
        if Ret:
            self.Date.Flags |= EFI_QF_DATE_MONTH_SUPPRESS

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, EFI_QF_DATE_DAY_SUPPRESS)
        if Ret:
            self.Date.Flags |= EFI_QF_DATE_DAY_SUPPRESS

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, QF_DATE_STORAGE_NORMAL)
        if Ret:
            self.Date.Flags |= QF_DATE_STORAGE_NORMAL

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, QF_DATE_STORAGE_TIME)
        if Ret:
            self.Date.Flags |= QF_DATE_STORAGE_TIME

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, QF_DATE_STORAGE_WAKEUP)
        if Ret:
            self.Date.Flags |= QF_DATE_STORAGE_WAKEUP

        return VfrReturnCode.VFR_RETURN_SUCCESS if LFlags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED


class IfrTime(IfrLine, IfrBaseInfo, IfrOpHeader, IfrQuestionHeader):
    def __init__(self, QName=None, VarIdStr=""):
        self.Time = EFI_IFR_TIME()
        IfrBaseInfo.__init__(self, self.Time, QName, VarIdStr)
        IfrOpHeader.__init__(self, self.Time.Header, EFI_IFR_TIME_OP)
        IfrQuestionHeader.__init__(self, self.Time.Question)
        self.Time.Flags = 0

    def SetFlags(self, HFlags, LFlags):
        ReturnCode = IfrQuestionHeader.SetFlags(self, HFlags)
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, QF_TIME_HOUR_SUPPRESS)
        if Ret:
            self.Time.Flags |= QF_TIME_HOUR_SUPPRESS

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, QF_TIME_MINUTE_SUPPRESS)
        if Ret:
            self.Time.Flags |= QF_TIME_MINUTE_SUPPRESS

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, QF_TIME_SECOND_SUPPRESS)
        if Ret:
            self.Time.Flags |= QF_TIME_SECOND_SUPPRESS

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, QF_TIME_STORAGE_NORMAL)
        if Ret:
            self.Time.Flags |= QF_TIME_STORAGE_NORMAL

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, QF_TIME_STORAGE_TIME)
        if Ret:
            self.Time.Flags |= QF_TIME_STORAGE_TIME

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, QF_TIME_STORAGE_WAKEUP)
        if Ret:
            self.Time.Flags |= QF_TIME_STORAGE_WAKEUP

        return VfrReturnCode.VFR_RETURN_SUCCESS if LFlags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED


class IfrNumeric(IfrLine, IfrBaseInfo, IfrOpHeader, IfrQuestionHeader, IfrMinMaxStepData):
    def __init__(self, Type, QName=None, VarIdStr=""):
        self.Numeric = Refine_EFI_IFR_NUMERIC(Type)
        IfrBaseInfo.__init__(self, self.Numeric, QName, VarIdStr)
        IfrOpHeader.__init__(self, self.Numeric.Header, EFI_IFR_NUMERIC_OP, sizeof(self.Numeric))
        IfrQuestionHeader.__init__(self, self.Numeric.Question)
        IfrMinMaxStepData.__init__(self, self.Numeric.Data, True)
        self.Numeric.Flags = EFI_IFR_NUMERIC_SIZE_1 | EFI_IFR_DISPLAY_UINT_DEC

        self.HasStep = False

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
            self.Numeric.Flags = LFlags | EFI_IFR_DISPLAY_UINT_DEC
        else:
            self.Numeric.Flags = LFlags
        return VfrReturnCode.VFR_RETURN_SUCCESS

    def SetFlagsForBitField(self, HFlags, LFlags, DisplaySettingsSpecified=False):
        ReturnCode = IfrQuestionHeader.SetFlags(self, HFlags)
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode
        if DisplaySettingsSpecified == False:
            self.Numeric.Flags = LFlags | EDKII_IFR_DISPLAY_UINT_DEC_BIT
        else:
            self.Numeric.Flags = LFlags
        return VfrReturnCode.VFR_RETURN_SUCCESS

    def GetNumericFlags(self):
        return self.Numeric.Flags

    def SetHasStep(self, HasStep):
        self.HasStep = HasStep


class IfrOneOf(IfrQuestionHeader, IfrLine, IfrBaseInfo, IfrOpHeader, IfrMinMaxStepData):
    def __init__(self, Type, QName=None, VarIdStr=""):
        self.OneOf = Refine_EFI_IFR_ONE_OF(Type)
        IfrBaseInfo.__init__(self, self.OneOf, QName, VarIdStr)
        IfrOpHeader.__init__(self, self.OneOf.Header, EFI_IFR_ONE_OF_OP, sizeof(self.OneOf))
        IfrQuestionHeader.__init__(self, self.OneOf.Question)
        IfrMinMaxStepData.__init__(self, self.OneOf.Data)
        self.OneOf.Flags = 0

        self.HasMinMax = False

        self.HasStep = False

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
            self.OneOf.Flags = LFlags
        else:
            self.OneOf.Flags = LFlags | EFI_IFR_DISPLAY_UINT_DEC
        return VfrReturnCode.VFR_RETURN_SUCCESS

    def SetFlagsForBitField(self, HFlags, LFlags):
        ReturnCode = IfrQuestionHeader.SetFlags(self, HFlags)
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode
        if LFlags & EDKII_IFR_DISPLAY_BIT:
            self.OneOf.Flags = LFlags
        else:
            self.OneOf.Flags = LFlags | EDKII_IFR_DISPLAY_UINT_DEC_BIT
        return VfrReturnCode.VFR_RETURN_SUCCESS

    def SetHasMinMax(self, HasMinMax):
        self.HasMinMax = HasMinMax

    def SetHasStep(self, HasStep):
        self.HasStep = HasStep


class IfrCheckBox(IfrLine, IfrBaseInfo, IfrOpHeader, IfrQuestionHeader):
    def __init__(self, QName=None, VarIdStr=""):
        self.CheckBox = EFI_IFR_CHECKBOX()
        IfrBaseInfo.__init__(self, self.CheckBox, QName, VarIdStr)
        IfrOpHeader.__init__(self, self.CheckBox.Header, EFI_IFR_CHECKBOX_OP)
        IfrQuestionHeader.__init__(self, self.CheckBox.Question)
        self.CheckBox.Flags = 0

    def GetQuestion(self):
        return self

    def SetQHeaderFlags(self, Flags):
        IfrQuestionHeader.SetFlags(self, Flags)

    def GetFlags(self):
        return self.CheckBox.Flags

    def SetFlags(self, HFlags, LFlags):
        ReturnCode = IfrQuestionHeader.SetFlags(self, HFlags)
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, EFI_IFR_CHECKBOX_DEFAULT)
        if Ret:
            self.CheckBox.Flags |= EFI_IFR_CHECKBOX_DEFAULT

        LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, EFI_IFR_CHECKBOX_DEFAULT_MFG)

        if Ret:
            self.CheckBox.Flags |= EFI_IFR_CHECKBOX_DEFAULT_MFG

        return VfrReturnCode.VFR_RETURN_SUCCESS if LFlags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED


class IfrResetButton(IfrLine, IfrOpHeader, IfrStatementHeader):
    def __init__(self, DefaultStore=None):
        self.ResetButton = EFI_IFR_RESET_BUTTON()
        IfrOpHeader.__init__(self, self.ResetButton.Header, EFI_IFR_RESET_BUTTON_OP)
        IfrStatementHeader.__init__(self, self.ResetButton.Statement)
        self.ResetButton.DefaultId = EFI_HII_DEFAULT_CLASS_STANDARD

        self.DefaultStore = DefaultStore

    def SetDefaultId(self, DefaultId):
        self.ResetButton.DefaultId = DefaultId

    def SetDefaultStore(self, DefaultStore):
        self.DefaultStore = DefaultStore

    def GetInfo(self):
        return self.ResetButton


class IfrOr(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Or = EFI_IFR_OR()
        IfrOpHeader.__init__(self, self.Or.Header, EFI_IFR_OR_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.Or


class IfrAnd(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.And = EFI_IFR_AND()
        IfrOpHeader.__init__(self, self.And.Header, EFI_IFR_AND_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.And


class IfrBitWiseOr(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.BitWiseOr = EFI_IFR_BITWISE_OR()
        IfrOpHeader.__init__(self, self.BitWiseOr.Header, EFI_IFR_BITWISE_OR_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.BitWiseOr


class IfrCatenate(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Catenate = EFI_IFR_CATENATE()
        IfrOpHeader.__init__(self, self.Catenate.Header, EFI_IFR_CATENATE_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.Catenate


class IfrDivide(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Divide = EFI_IFR_DIVIDE()
        IfrOpHeader.__init__(self, self.Divide.Header, EFI_IFR_DIVIDE_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.Divide


class IfrEqual(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Equal = EFI_IFR_EQUAL()
        IfrOpHeader.__init__(self, self.Equal.Header, EFI_IFR_EQUAL_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.Equal


class IfrGreaterEqual(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.GreaterEqual = EFI_IFR_GREATER_EQUAL()
        IfrOpHeader.__init__(self, self.GreaterEqual.Header, EFI_IFR_GREATER_EQUAL_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.GreaterEqual


class IfrGreaterThan(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.GreaterThan = EFI_IFR_GREATER_THAN()
        IfrOpHeader.__init__(self, self.GreaterThan.Header, EFI_IFR_GREATER_THAN_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.GreaterThan


class IfrLessEqual(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.LessEqual = EFI_IFR_LESS_EQUAL()
        IfrOpHeader.__init__(self, self.LessEqual.Header, EFI_IFR_LESS_EQUAL_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.LessEqual


class IfrLessThan(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.LessThan = EFI_IFR_LESS_THAN()
        IfrOpHeader.__init__(self, self.LessThan.Header, EFI_IFR_LESS_THAN_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.LessThan


class IfrMap(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Map = EFI_IFR_MAP()
        IfrOpHeader.__init__(self, self.Map.Header, EFI_IFR_MAP_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.Map


class IfrMatch(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Match = EFI_IFR_MATCH()
        IfrOpHeader.__init__(self, self.Match.Header, EFI_IFR_MATCH_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.Match


class IfrMatch2(IfrLine, IfrOpHeader):
    def __init__(self, LineNo, Guid):
        self.Match2 = EFI_IFR_MATCH2()
        IfrOpHeader.__init__(self, self.Match2.Header, EFI_IFR_MATCH2_OP)
        self.SetLineNo(LineNo)
        self.Match2.SyntaxType = Guid

    def GetInfo(self):
        return self.Match2


class IfrMultiply(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Multiply = EFI_IFR_MULTIPLY()
        IfrOpHeader.__init__(self, self.Multiply.Header, EFI_IFR_MULTIPLY_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.Multiply


class IfrModulo(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Modulo = EFI_IFR_MODULO()
        IfrOpHeader.__init__(self, self.Modulo.Header, EFI_IFR_MODULO_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.Modulo


class IfrNotEqual(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.NotEqual = EFI_IFR_NOT_EQUAL()
        IfrOpHeader.__init__(self, self.NotEqual.Header, EFI_IFR_NOT_EQUAL_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.NotEqual


class IfrShiftLeft(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.ShiftLeft = EFI_IFR_SHIFT_LEFT()
        IfrOpHeader.__init__(self, self.ShiftLeft.Header, EFI_IFR_SHIFT_LEFT_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.ShiftLeft


class IfrShiftRight(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.ShiftRight = EFI_IFR_SHIFT_RIGHT()
        IfrOpHeader.__init__(self, self.ShiftRight.Header, EFI_IFR_SHIFT_RIGHT_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.ShiftRight


class IfrSubtract(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Subtract = EFI_IFR_SUBTRACT()
        IfrOpHeader.__init__(self, self.Subtract.Header, EFI_IFR_SUBTRACT_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.Subtract


class IfrConditional(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Conditional = EFI_IFR_CONDITIONAL()
        IfrOpHeader.__init__(self, self.Conditional.Header, EFI_IFR_CONDITIONAL_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.Conditional


class IfrFind(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Find = EFI_IFR_FIND()
        IfrOpHeader.__init__(self, self.Find.Header, EFI_IFR_FIND_OP)
        self.SetLineNo(LineNo)

    def SetFormat(self, Format):
        self.Find.Format = Format

    def GetInfo(self):
        return self.Find


class IfrMid(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Mid = EFI_IFR_MID()
        IfrOpHeader.__init__(self, self.Mid.Header, EFI_IFR_MID_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.Mid


class IfrToken(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Token = EFI_IFR_TOKEN()
        IfrOpHeader.__init__(self, self.Token.Header, EFI_IFR_TOKEN_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.Token


class IfrSpan(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Span = EFI_IFR_SPAN()
        IfrOpHeader.__init__(self, self.Span.Header, EFI_IFR_SPAN_OP)
        self.SetLineNo(LineNo)
        self.Span.Flags = EFI_IFR_FLAGS_FIRST_MATCHING

    def SetFlags(self, LFlags):
        if LFlags == EFI_IFR_FLAGS_FIRST_MATCHING:
            self.Span.Flags |= EFI_IFR_FLAGS_FIRST_MATCHING
        else:
            LFlags, Ret = _FLAG_TEST_AND_CLEAR(LFlags, EFI_IFR_FLAGS_FIRST_NON_MATCHING)
            if Ret:
                self.Span.Flags |= EFI_IFR_FLAGS_FIRST_NON_MATCHING

        return VfrReturnCode.VFR_RETURN_SUCCESS if LFlags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED

    def GetInfo(self):
        return self.Span


class IfrBitWiseAnd(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.BitWiseAnd = EFI_IFR_BITWISE_AND()
        IfrOpHeader.__init__(self, self.BitWiseAnd.Header, EFI_IFR_BITWISE_AND_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.BitWiseAnd


class IfrBitWiseOr(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.BitWiseOr = EFI_IFR_BITWISE_OR()
        IfrOpHeader.__init__(self, self.BitWiseOr.Header, EFI_IFR_BITWISE_OR_OP)
        self.SetLineNo(LineNo)


class IfrAdd(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Add = EFI_IFR_ADD()
        IfrOpHeader.__init__(self, self.Add.Header, EFI_IFR_ADD_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.Add


class IfrToString(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.ToString = EFI_IFR_TO_STRING()
        IfrOpHeader.__init__(self, self.ToString.Header, EFI_IFR_TO_STRING_OP)
        self.SetLineNo(LineNo)

    def SetFormat(self, Format):
        self.ToString.Format = Format

    def GetInfo(self):
        return self.ToString


class IfrToUpper(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.ToUppper = EFI_IFR_TO_UPPER()
        IfrOpHeader.__init__(self, self.ToUppper.Header, EFI_IFR_TO_UPPER_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.ToUppper


class IfrToUint(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.ToUint = EFI_IFR_TO_UINT()
        IfrOpHeader.__init__(self, self.ToUint.Header, EFI_IFR_TO_UINT_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.ToUint


class IfrToLower(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.ToLower = EFI_IFR_TO_LOWER()
        IfrOpHeader.__init__(self, self.ToLower.Header, EFI_IFR_TO_LOWER_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.ToLower


class IfrToBoolean(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Boolean = EFI_IFR_TO_BOOLEAN()
        IfrOpHeader.__init__(self, self.Boolean.Header, EFI_IFR_TO_BOOLEAN_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.Boolean


class IfrNot(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Not = EFI_IFR_NOT()
        IfrOpHeader.__init__(self, self.Not.Header, EFI_IFR_NOT_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.Not


class IfrDup(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Dup = EFI_IFR_DUP()
        IfrOpHeader.__init__(self, self.Dup.Header, EFI_IFR_DUP_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.Dup.Header

    def GetInfo(self):
        return self.Dup


class IfrEqIdId(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.EqIdId = EFI_IFR_EQ_ID_ID()
        IfrOpHeader.__init__(self, self.EqIdId.Header, EFI_IFR_EQ_ID_ID_OP)
        self.SetLineNo(LineNo)
        self.EqIdId.QuestionId1 = EFI_QUESTION_ID_INVALID
        self.EqIdId.QuestionId2 = EFI_QUESTION_ID_INVALID

    def GetHeader(self):
        return self.EqIdId.Header

    def SetQuestionId1(self, QuestionId, VarIdStr, LineNo):
        if QuestionId != EFI_QUESTION_ID_INVALID:
            self.EqIdId.QuestionId1 = QuestionId
        else:
            gFormPkg.AssignPending(VarIdStr, self.EqIdId, LineNo, "no question refered", 1)

    def SetQuestionId2(self, QuestionId, VarIdStr, LineNo):
        if QuestionId != EFI_QUESTION_ID_INVALID:
            self.EqIdId.QuestionId2 = QuestionId
        else:
            gFormPkg.AssignPending(VarIdStr, self.EqIdId, LineNo, "no question refered", 2)

    def GetInfo(self):
        return self.EqIdId


class IfrEqIdVal(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.EqIdVal = EFI_IFR_EQ_ID_VAL()
        IfrOpHeader.__init__(self, self.EqIdVal.Header, EFI_IFR_EQ_ID_VAL_OP)
        self.SetLineNo(LineNo)
        self.EqIdVal.QuestionId = EFI_QUESTION_ID_INVALID

    def SetQuestionId(self, QuestionId, VarIdStr, LineNo):
        if QuestionId != EFI_QUESTION_ID_INVALID:
            self.EqIdVal.QuestionId = QuestionId
        else:
            gFormPkg.AssignPending(VarIdStr, self.EqIdVal, LineNo, "no question refered")

    def SetValue(self, Value):
        self.EqIdVal.Value = Value

    def GetHeader(self):
        return self.EqIdVal.Header

    def GetInfo(self):
        return self.EqIdVal


class IfrEqIdList(IfrLine, IfrOpHeader):
    def __init__(self, LineNo, Nums, ValueList=[]):
        self.EqIdVList = Refine_EFI_IFR_EQ_ID_VAL_LIST(Nums)
        IfrOpHeader.__init__(self, self.EqIdVList.Header, EFI_IFR_EQ_ID_VAL_LIST_OP, sizeof(self.EqIdVList))
        self.SetLineNo(LineNo)
        self.EqIdVList.QuestionId = EFI_QUESTION_ID_INVALID
        self.EqIdVList.ListLength = 0
        if ValueList != []:
            ArrayType = c_uint16 * Nums
            ValueArray = ArrayType()
            for i in range(0, len(ValueList)):
                ValueArray[i] = ValueList[i]
            self.EqIdVList.ValueList = ValueArray

    def SetQuestionId(self, QuestionId, VarIdStr, LineNo):
        if QuestionId != EFI_QUESTION_ID_INVALID:
            self.EqIdVList.QuestionId = QuestionId
        else:
            gFormPkg.AssignPending(VarIdStr, self.EqIdVList, LineNo, "no question refered")

    def SetListLength(self, ListLength):
        self.EqIdVList.ListLength = ListLength

    def SetValueList(self, ValueList):
        if ValueList != []:
            ArrayType = c_uint16 * len(ValueList)
            ValueArray = ArrayType()
            for i in range(0, len(ValueList)):
                ValueArray[i] = ValueList[i]
            self.EqIdVList.ValueList = ValueArray

    def GetHeader(self):
        return self.EqIdVList.Header

    def GetInfo(self):
        return self.EqIdVList


class IfrUint8(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Uint8 = EFI_IFR_UINT8()
        IfrOpHeader.__init__(self, self.Uint8.Header, EFI_IFR_UINT8_OP)
        self.SetLineNo(LineNo)

    def SetValue(self, Value):
        self.Uint8.Value = Value

    def GetHeader(self):
        return self.Uint8.Header

    def GetInfo(self):
        return self.Uint8


class IfrUint16(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Uint16 = EFI_IFR_UINT16()
        IfrOpHeader.__init__(self, self.Uint16.Header, EFI_IFR_UINT16_OP)
        self.SetLineNo(LineNo)

    def SetValue(self, Value):
        self.Uint16.Value = Value

    def GetHeader(self):
        return self.Uint16.Header

    def GetInfo(self):
        return self.Uint16


class IfrUint32(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Uint32 = EFI_IFR_UINT32()
        IfrOpHeader.__init__(self, self.Uint32.Header, EFI_IFR_UINT32_OP)
        self.SetLineNo(LineNo)

    def SetValue(self, Value):
        self.Uint32.Value = Value

    def GetHeader(self):
        return self.Uint32.Header

    def GetInfo(self):
        return self.Uint32


class IfrUint64(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Uint64 = EFI_IFR_UINT64()
        IfrOpHeader.__init__(self, self.Uint64.Header, EFI_IFR_UINT64_OP, sizeof(EFI_IFR_UINT64))
        self.SetLineNo(LineNo)

    def SetValue(self, Value):
        self.Uint64.Value = Value

    def GetHeader(self):
        return self.Uint64.Header

    def GetInfo(self):
        return self.Uint64


class IfrQuestionRef1(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.QuestionRef1 = EFI_IFR_QUESTION_REF1()
        IfrOpHeader.__init__(self, self.QuestionRef1.Header, EFI_IFR_QUESTION_REF1_OP)
        self.SetLineNo(LineNo)
        self.QuestionRef1.QuestionId = EFI_QUESTION_ID_INVALID

    def GetHeader(self):
        return self.QuestionRef1.Header

    def SetQuestionId(self, QuestionId, VarIdStr, LineNo):
        if QuestionId != EFI_QUESTION_ID_INVALID:
            self.QuestionRef1.QuestionId = QuestionId
        else:
            gFormPkg.AssignPending(VarIdStr, self.QuestionRef1, LineNo, "no question refered")

    def GetInfo(self):
        return self.QuestionRef1


class IfrQuestionRef2(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.QuestionRef2 = EFI_IFR_QUESTION_REF2()
        IfrOpHeader.__init__(self, self.QuestionRef2.Header, EFI_IFR_QUESTION_REF2_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.QuestionRef2.Header

    def GetInfo(self):
        return self.QuestionRef2


class IfrQuestionRef3(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.QuestionRef3 = EFI_IFR_QUESTION_REF3()
        IfrOpHeader.__init__(self, self.QuestionRef3.Header, \
            EFI_IFR_QUESTION_REF3_OP, sizeof(EFI_IFR_QUESTION_REF3))
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.QuestionRef3.Header

    def GetInfo(self):
        return self.QuestionRef3


class IfrQuestionRef3_2(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.QuestionRef3_2 = EFI_IFR_QUESTION_REF3_2()
        IfrOpHeader.__init__(self, self.QuestionRef3_2.Header, \
            EFI_IFR_QUESTION_REF3_OP, sizeof(EFI_IFR_QUESTION_REF3_2))
        self.SetLineNo(LineNo)
        self.QuestionRef3_2.DevicePath = EFI_STRING_ID_INVALID

    def SetDevicePath(self, DevicePath):
        self.QuestionRef3_2.DevicePath = DevicePath

    def GetHeader(self):
        return self.QuestionRef3_2.Header

    def GetInfo(self):
        return self.QuestionRef3_2


class IfrQuestionRef3_3(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.QuestionRef3_3 = EFI_IFR_QUESTION_REF3_3()
        IfrOpHeader.__init__(self, self.QuestionRef3_3.Header, \
            EFI_IFR_QUESTION_REF3_OP, sizeof(EFI_IFR_QUESTION_REF3_3))
        self.SetLineNo(LineNo)
        self.QuestionRef3_3.DevicePath = EFI_STRING_ID_INVALID
        self.QuestionRef3_3.Guid = EFI_GUID(0, 0, 0, GuidArray(0, 0, 0, 0, 0, 0, 0, 0))

    def SetDevicePath(self, DevicePath):
        self.QuestionRef3_3.DevicePath = DevicePath

    def SetGuid(self, Guid):
        self.QuestionRef3_3.Guid = Guid

    def GetHeader(self):
        return self.QuestionRef3_3.Header

    def GetInfo(self):
        return self.QuestionRef3_3


class IfrRuleRef(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.RuleRef = EFI_IFR_RULE_REF()
        IfrOpHeader.__init__(self, self.RuleRef.Header, EFI_IFR_RULE_REF_OP)
        self.SetLineNo(LineNo)
        self.RuleRef.RuleId = EFI_RULE_ID_INVALID

    def SetRuleId(self, RuleId):
        self.RuleRef.RuleId = RuleId

    def GetHeader(self):
        return self.RuleRef.Header

    def GetInfo(self):
        return self.RuleRef


class IfrStringRef1(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.StringRef1 = EFI_IFR_STRING_REF1()
        IfrOpHeader.__init__(self, self.StringRef1.Header, EFI_IFR_STRING_REF1_OP)
        self.SetLineNo(LineNo)
        self.StringRef1.StringId = EFI_STRING_ID_INVALID

    def SetStringId(self, StringId):
        self.StringRef1.StringId = StringId

    def GetHeader(self):
        return self.StringRef1.Header

    def GetInfo(self):
        return self.StringRef1


class IfrStringRef2(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.StringRef2 = EFI_IFR_STRING_REF2()
        IfrOpHeader.__init__(self, self.StringRef2.Header, EFI_IFR_STRING_REF2_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.StringRef2.Header

    def GetInfo(self):
        return self.StringRef2


class IfrThis(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.This = EFI_IFR_THIS()
        IfrOpHeader.__init__(self, self.This.Header, EFI_IFR_THIS_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.This.Header

    def GetInfo(self):
        return self.This


class IfrSecurity(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Security = EFI_IFR_SECURITY()
        IfrOpHeader.__init__(self, self.Security.Header, EFI_IFR_SECURITY_OP)
        self.SetLineNo(LineNo)
        self.Security.Permissions = EFI_GUID(0, 0, 0, GuidArray(0, 0, 0, 0, 0, 0, 0, 0))

    def SetPermissions(self, Permissions):
        self.Security.Permissions = Permissions

    def GetHeader(self):
        return self.Security.Header

    def GetInfo(self):
        return self.Security


class IfrGet(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Get = EFI_IFR_GET()
        IfrOpHeader.__init__(self, self.Get.Header, EFI_IFR_GET_OP)
        self.SetLineNo(LineNo)

    def SetVarInfo(self, BaseInfo: EFI_VARSTORE_INFO):
        self.Get.VarStoreId = BaseInfo.VarStoreId
        self.Get.VarStoreInfo.VarName = BaseInfo.Info.VarName
        self.Get.VarStoreInfo.VarOffset = BaseInfo.Info.VarOffset
        self.Get.VarStoreType = BaseInfo.VarType

    def GetHeader(self):
        return self.Get.Header

    def GetInfo(self):
        return self.Get


class IfrSet(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Set = EFI_IFR_SET()
        IfrOpHeader.__init__(self, self.Set.Header, EFI_IFR_SET_OP)
        self.SetLineNo(LineNo)

    def SetVarInfo(self, BaseInfo: EFI_VARSTORE_INFO):
        self.Set.VarStoreId = BaseInfo.VarStoreId
        self.Set.VarStoreInfo.VarName = BaseInfo.Info.VarName
        self.Set.VarStoreInfo.VarOffset = BaseInfo.Info.VarOffset
        self.Set.VarStoreType = BaseInfo.VarType

    def GetHeader(self):
        return self.Set.Header

    def GetInfo(self):
        return self.Set


class IfrTrue(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.TrueOp = EFI_IFR_TRUE()
        IfrOpHeader.__init__(self, self.TrueOp.Header, EFI_IFR_TRUE_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.TrueOp.Header

    def GetInfo(self):
        return self.TrueOp


class IfrFalse(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.FalseOp = EFI_IFR_TRUE()
        IfrOpHeader.__init__(self, self.FalseOp.Header, EFI_IFR_FALSE_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.FalseOp.Header

    def GetInfo(self):
        return self.FalseOp


class IfrOne(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.One = EFI_IFR_ONE()
        IfrOpHeader.__init__(self, self.One.Header, EFI_IFR_ONE_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.One.Header

    def GetInfo(self):
        return self.One


class IfrOnes(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Ones = EFI_IFR_ONE()
        IfrOpHeader.__init__(self, self.Ones.Header, EFI_IFR_ONES_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.Ones.Header

    def GetInfo(self):
        return self.Ones


class IfrZero(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Zero = EFI_IFR_ZERO()
        IfrOpHeader.__init__(self, self.Zero.Header, EFI_IFR_ZERO_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.Zero.Header

    def GetInfo(self):
        return self.Zero


class IfrUndefined(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Undefined = EFI_IFR_ZERO()
        IfrOpHeader.__init__(self, self.Undefined.Header, EFI_IFR_UNDEFINED_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.Undefined.Header

    def GetInfo(self):
        return self.Undefined


class IfrVersion(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Version = EFI_IFR_VERSION()
        IfrOpHeader.__init__(self, self.Version.Header, EFI_IFR_VERSION_OP)
        self.SetLineNo(LineNo)

    def GetHeader(self):
        return self.Version.Header

    def GetInfo(self):
        return self.Version


class IfrLength(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.Length = EFI_IFR_LENGTH()
        IfrOpHeader.__init__(self, self.Length.Header, EFI_IFR_LENGTH_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.Length


class IfrBitWiseNot(IfrLine, IfrOpHeader):
    def __init__(self, LineNo):
        self.BitWiseNot = EFI_IFR_BITWISE_NOT()
        IfrOpHeader.__init__(self, self.BitWiseNot.Header, EFI_IFR_BITWISE_NOT_OP)
        self.SetLineNo(LineNo)

    def GetInfo(self):
        return self.BitWiseNot


class ExpressionInfo:
    def __init__(self):
        self.RootLevel = 0
        self.ExpOpCount = 0
