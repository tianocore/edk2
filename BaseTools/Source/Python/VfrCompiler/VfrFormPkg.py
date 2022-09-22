from ast import For
from re import L
from sre_parse import FLAGS
from stat import FILE_ATTRIBUTE_SPARSE_FILE
from CommonCtypes import *
from VfrError import VfrReturnCode
from VfrUtility import *

from ctypes import*

class OpcodeSizesScopeNode():
    def __init__(self, Size, Scope):
        self.Size = Size
        self.Scope = Scope


OpcodeSizesScopeTable = [
    OpcodeSizesScopeNode(0, 0),  # EFI_IFR_INVALID - 0x00
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_FORM), 1),  # EFI_IFR_FORM_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_SUBTITLE),
                         1),  # EFI_IFR_SUBTITLE_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_TEXT), 0),  # EFI_IFR_TEXT_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_IMAGE), 0),  # EFI_IFR_IMAGE_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_ONE_OF),
                         1),  # EFI_IFR_ONE_OF_OP - 0x05
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_CHECKBOX),
                         1),  # EFI_IFR_CHECKBOX_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_NUMERIC),
                         1),  # EFI_IFR_NUMERIC_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_PASSWORD),
                         1),  # EFI_IFR_PASSWORD_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_ONE_OF_OPTION),
                         0),  # EFI_IFR_ONE_OF_OPTION_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_SUPPRESS_IF),
                         1),  # EFI_IFR_SUPPRESS_IF - 0x0A
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_LOCKED),
                         0),  # EFI_IFR_LOCKED_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_ACTION),
                         1),  # EFI_IFR_ACTION_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_RESET_BUTTON),
                         1),  # EFI_IFR_RESET_BUTTON_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_FORM_SET),
                         1),  # EFI_IFR_FORM_SET_OP -0xE
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_REF), 0),  # EFI_IFR_REF_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_NO_SUBMIT_IF),
                         1),  # EFI_IFR_NO_SUBMIT_IF_OP -0x10
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_INCONSISTENT_IF),
                         1),  # EFI_IFR_INCONSISTENT_IF_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_EQ_ID_VAL),
                         0),  # EFI_IFR_EQ_ID_VAL_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_EQ_ID_ID),
                         0),  # EFI_IFR_EQ_ID_ID_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_EQ_ID_VAL_LIST),
                         0),  # EFI_IFR_EQ_ID_LIST_OP - 0x14
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_AND), 0),  # EFI_IFR_AND_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_OR), 0),  # EFI_IFR_OR_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_NOT), 0),  # EFI_IFR_NOT_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_RULE), 1),  # EFI_IFR_RULE_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_GRAY_OUT_IF),
                         1),  # EFI_IFR_GRAYOUT_IF_OP - 0x19
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_DATE), 1),  # EFI_IFR_DATE_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_TIME), 1),  # EFI_IFR_TIME_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_STRING),
                         1),  # EFI_IFR_STRING_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_REFRESH),
                         0),  # EFI_IFR_REFRESH_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_DISABLE_IF),
                         1),  # EFI_IFR_DISABLE_IF_OP - 0x1E
    OpcodeSizesScopeNode(0, 0),  # 0x1F
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_TO_LOWER),
                         0),  # EFI_IFR_TO_LOWER_OP - 0x20
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_TO_UPPER),
                         0),  # EFI_IFR_TO_UPPER_OP - 0x21
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_MAP), 1),  # EFI_IFR_MAP - 0x22
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_ORDERED_LIST),
                         1),  # EFI_IFR_ORDERED_LIST_OP - 0x23
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_VARSTORE),
                         0),  # EFI_IFR_VARSTORE_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_VARSTORE_NAME_VALUE),
                         0),  # EFI_IFR_VARSTORE_NAME_VALUE_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_VARSTORE_EFI),
                         0),  # EFI_IFR_VARSTORE_EFI_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_VARSTORE_DEVICE),
                         1),  # EFI_IFR_VARSTORE_DEVICE_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_VERSION),
                         0),  # EFI_IFR_VERSION_OP - 0x28
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_END), 0),  # EFI_IFR_END_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_MATCH),
                         0),  # EFI_IFR_MATCH_OP - 0x2A
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_GET), 0),  # EFI_IFR_GET - 0x2B
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_SET), 0),  # EFI_IFR_SET - 0x2C
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_READ),
                         0),  # EFI_IFR_READ - 0x2D
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_WRITE),
                         0),  # EFI_IFR_WRITE - 0x2E
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_EQUAL),
                         0),  # EFI_IFR_EQUAL_OP - 0x2F
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_NOT_EQUAL),
                         0),  # EFI_IFR_NOT_EQUAL_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_GREATER_THAN),
                         0),  # EFI_IFR_GREATER_THAN_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_GREATER_EQUAL),
                         0),  # EFI_IFR_GREATER_EQUAL_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_LESS_THAN),
                         0),  # EFI_IFR_LESS_THAN_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_LESS_EQUAL),
                         0),  # EFI_IFR_LESS_EQUAL_OP - 0x34
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_BITWISE_AND),
                         0),  # EFI_IFR_BITWISE_AND_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_BITWISE_OR),
                         0),  # EFI_IFR_BITWISE_OR_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_BITWISE_NOT),
                         0),  # EFI_IFR_BITWISE_NOT_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_SHIFT_LEFT),
                         0),  # EFI_IFR_SHIFT_LEFT_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_SHIFT_RIGHT),
                         0),  # EFI_IFR_SHIFT_RIGHT_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_ADD),
                         0),  # EFI_IFR_ADD_OP - 0x3A
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_SUBTRACT),
                         0),  # EFI_IFR_SUBTRACT_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_MULTIPLY),
                         0),  # EFI_IFR_MULTIPLY_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_DIVIDE),
                         0),  # EFI_IFR_DIVIDE_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_MODULO),
                         0),  # EFI_IFR_MODULO_OP - 0x3E
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_RULE_REF),
                         0),  # EFI_IFR_RULE_REF_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_QUESTION_REF1),
                         0),  # EFI_IFR_QUESTION_REF1_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_QUESTION_REF2),
                         0),  # EFI_IFR_QUESTION_REF2_OP - 0x41
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_UINT8), 0),  # EFI_IFR_UINT8
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_UINT16), 0),  # EFI_IFR_UINT16
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_UINT32), 0),  # EFI_IFR_UINT32
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_UINT64), 0),  # EFI_IFR_UTNT64
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_TRUE),
                         0),  # EFI_IFR_TRUE_OP - 0x46
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_FALSE), 0),  # EFI_IFR_FALSE_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_TO_UINT),
                         0),  # EFI_IFR_TO_UINT_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_TO_STRING),
                         0),  # EFI_IFR_TO_STRING_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_TO_BOOLEAN),
                         0),  # EFI_IFR_TO_BOOLEAN_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_MID), 0),  # EFI_IFR_MID_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_FIND), 0),  # EFI_IFR_FIND_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_TOKEN), 0),  # EFI_IFR_TOKEN_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_STRING_REF1),
                         0),  # EFI_IFR_STRING_REF1_OP - 0x4E
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_STRING_REF2),
                         0),  # EFI_IFR_STRING_REF2_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_CONDITIONAL),
                         0),  # EFI_IFR_CONDITIONAL_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_QUESTION_REF3),
                         0),  # EFI_IFR_QUESTION_REF3_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_ZERO), 0),  # EFI_IFR_ZERO_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_ONE), 0),  # EFI_IFR_ONE_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_ONES), 0),  # EFI_IFR_ONES_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_UNDEFINED),
                         0),  # EFI_IFR_UNDEFINED_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_LENGTH),
                         0),  # EFI_IFR_LENGTH_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_DUP),
                         0),  # EFI_IFR_DUP_OP - 0x57
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_THIS), 0),  # EFI_IFR_THIS_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_SPAN), 0),  # EFI_IFR_SPAN_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_VALUE), 1),  # EFI_IFR_VALUE_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_DEFAULT),
                         0),  # EFI_IFR_DEFAULT_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_DEFAULTSTORE),
                         0),  # EFI_IFR_DEFAULTSTORE_OP - 0x5C
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_FORM_MAP),
                         1),  # EFI_IFR_FORM_MAP_OP - 0x5D
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_CATENATE),
                         0),  # EFI_IFR_CATENATE_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_GUID), 0),  # EFI_IFR_GUID_OP
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_SECURITY),
                         0),  # EFI_IFR_SECURITY_OP - 0x60
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_MODAL_TAG),
                         0),  # EFI_IFR_MODAL_TAG_OP - 0x61
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_REFRESH_ID),
                         0),  # EFI_IFR_REFRESH_ID_OP - 0x62
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_WARNING_IF),
                         1),  # EFI_IFR_WARNING_IF_OP - 0x63
    OpcodeSizesScopeNode(ctypes.sizeof(EFI_IFR_MATCH2), 0)
]

gCreateOp = True


class SBufferNode():
    def __init__(self, Buffer='', Next=None):
        self.Buffer = Buffer
        self.Next = Next

class CFormPkg(): # wip

    def __init__(self, BufferSize=0):

        Node = SBufferNode()
        if Node == None:
            return

        self.__BufferNodeQueueHead = Node
        self.__BufferNodeQueueTail = Node
        self.__CurrBufferNode = Node
        self.__ReadBufferNode = None
        self.__ReadBufferOffset = 0
        self.__PkgLength = 0
        self.__PendingAssignList = None
        self.__BufferSize = BufferSize

    def GetPkgLength(self):
        return self.__PkgLength

    def __createNewNode(self):
        Node = SBufferNode()
        return Node

    def __GetBinBufferNodeForAddr(self, BinBuffAddr):
        pass

    def __GetNodeBefore(self, CurrentNode):
        FirstNode = self.__BufferNodeQueueHead
        LastNode = self.__BufferNodeQueueHead
        while FirstNode != None:
            if FirstNode == CurrentNode:
                break

            LastNode = FirstNode
            FirstNode = FirstNode.Next

        if FirstNode == None:
            LastNode = None

        return LastNode

    def __InsertNodeBefore(self, CurrentNode, NewNode):
        LastNode = self.__GetNodeBefore(CurrentNode)
        if LastNode == None:
            return VfrReturnCode.VFR_RETURN_MISMATCHED

        NewNode.Next = LastNode.Next
        LastNode.Next = NewNode

    def IfrBinBufferGet(self, Len):
        if Len == 0 or Len > self.__BufferSize:
            return None
        if len(self.__CurrBufferNode.Buffer) + Len > self.__BufferSize:
            Node = self.__createNewNode()
            if Node == None:
                return None

            if self.__BufferNodeQueueTail == None:
                self.__BufferNodeQueueHead = self.__BufferNodeQueueTail = Node
            else:
                self.__BufferNodeQueueTail.Next = Node
                mBufferNodeQueueTail = Node
            self.__CurrBufferNode = Node

        self.__PkgLength += Len

        return self.__CurrBufferNode.Buffer # 返回

    # def DoPendingAssign(self, Key, VarlAddr, ValLEN):



gCFormPkg = CFormPkg()
gCreateOp = True

class CIfrObj(): # wip


    def __init__(self, Opcode, IfrObj, DelayEmit=False):
        self.__DelayEmit = DelayEmit
        self.__ObjBinLen = 0
        self.__ObjBinBuf = ''

        # output IfrObj = mObjBinBuf

        self.__LineNo = 0
        # self.__RecordIdx = 0  \ gCIfrRecordInfoDB.IfrRecordRegister
        self.__PkgOffset = gCFormPkg.GetPkgLength()

        # CIFROBJ_DEBUG_PRINT (OpCode)

    def SetObjBin(self, ObjBinLen):

        self.__ObjBinLen = ObjBinLen
        if self.__DelayEmit == False and gCreateOp == True:
            self.__ObjBinBuf = gCFormPkg.IfrBinBufferGet(self.__ObjBinLen)

    def SetLineNo(self,LineNo):
        self.__LineNo = LineNo

    def GetObjBinOffset(self):
        return self.__PkgOffset

    def GetObjBinLen(self):
        return len(self.__ObjBinBuf)

    def ExpendObjBin(self, Size):
        if self.__DelayEmit == True and (self.__ObjBinLen+ Size) > self.__ObjBinLen:
            self.__ObjBinLen = self.__ObjBinLen + Size
            return True
        else:
            return False

    def GetObjBin(self):
        return self.__ObjBinBuf


gScopeCount = 0
gIsOrderedList = False
gIsStringOp = False
gCurrentQuestion = None
gCurrentMinMaxData = None

def SetCurrentQuestion(Question):

    gCurrentQuestion = Question

    return gCurrentQuestion

class CIfrOpHeader():

    def __init__(self, OpCode, OpHeader: EFI_IFR_OP_HEADER, Length=0):
        self.__OpHeader = OpHeader
        self.__OpHeader.OpCode = OpCode

        self.__OpHeader.Length = OpcodeSizesScopeTable[OpCode].Size if Length == 0 else Length
        self.__OpHeader.Scope = 1 if (OpcodeSizesScopeTable[OpCode].Scope + gScopeCount > 0) else 0

    def GetLength(self):
        return self.__OpHeader.Length

    def SetScope(self, Scope):
        self.__OpHeader.Scope = Scope

    def GetScope(self):
        return self.__OpHeader.Scope

    def UpdateHeader(self, Header):
        self.__OpHeader = Header

    def IncLength(self, Size):
        self.__OpHeader.Length += Size

    def DecLength(self, Size):
        self.__OpHeader.Length -= Size
    
    def GetOpCode(self):
        return self.__OpHeader.OpCode


class CIfrStatementHeader():
    def __init__(self, sHeader: EFI_IFR_STATEMENT_HEADER):
        self.__sHeader = sHeader
        self.__sHeader.Help = EFI_STRING_ID_INVALID
        self.__sHeader.Prompt = EFI_STRING_ID_INVALID

    def GetStatementHeader(self):
        return self.__sHeader

    def SetPrompt(self, Prompt):
        self.__sHeader.Prompt = Prompt

    def SetHelp(self,Help):
        self.__sHeader.Help = Help


class CIfrMinMaxStepData():

    def __init__(self, MinMaxStepData, NumericOpcode=False):
        self.__MinMaxStepData = MinMaxStepData #
        self.__MinMaxStepData.u64.MinValue = 0
        self.__MinMaxStepData.u64.MaxValue = 0
        self.__MinMaxStepData.u64.Step = 0
        self.__ValueIsSet = False
        self.__IsNumeric = NumericOpcode

    def SetMinMaxStepData(self, MinValue, MaxValue, Step, VarType): #
        if self.__ValueIsSet == False:
            if VarType == EFI_IFR_TYPE_NUM_SIZE_64:
                self.__MinMaxStepData.u64.MinValue = MinValue
                self.__MinMaxStepData.u64.MaxValue = MaxValue
                self.__MinMaxStepData.u64.Step = Step
            if VarType == EFI_IFR_TYPE_NUM_SIZE_32:
                self.__MinMaxStepData.u32.MinValue = MinValue
                self.__MinMaxStepData.u32.MaxValue = MaxValue
                self.__MinMaxStepData.u32.Step = Step
            if VarType == EFI_IFR_TYPE_NUM_SIZE_16:
                self.__MinMaxStepData.u16.MinValue = MinValue
                self.__MinMaxStepData.u16.MaxValue = MaxValue
                self.__MinMaxStepData.u16.Step = Step
            if VarType == EFI_IFR_TYPE_NUM_SIZE_8:
                self.__MinMaxStepData.u8.MinValue = MinValue
                self.__MinMaxStepData.u8.MaxValue = MaxValue
                self.__MinMaxStepData.u8.Step = Step
            self.__ValueIsSet = True
        else:
            if VarType == EFI_IFR_TYPE_NUM_SIZE_64:
                if MinValue < self.__MinMaxStepData.u64.MinValue:
                    self.__MinMaxStepData.u64.MinValue = MinValue
                if MaxValue > self.__MinMaxStepData.u64.MaxValue:
                    self.__MinMaxStepData.u64.MaxValue = MaxValue
                self.__MinMaxStepData.u64.Step = Step
            if VarType == EFI_IFR_TYPE_NUM_SIZE_32:
                if MinValue < self.__MinMaxStepData.u32.MinValue:
                    self.__MinMaxStepData.u32.MinValue = MinValue
                if MaxValue > self.__MinMaxStepData.u32.MaxValue:
                    self.__MinMaxStepData.u32.MaxValue = MaxValue
                self.__MinMaxStepData.u32.Step = Step
            if VarType == EFI_IFR_TYPE_NUM_SIZE_16:
                if MinValue < self.__MinMaxStepData.u16.MinValue:
                    self.__MinMaxStepData.u16.MinValue = MinValue
                if MaxValue > self.__MinMaxStepData.u16.MaxValue:
                    self.__MinMaxStepData.u16.MaxValue = MaxValue
                self.__MinMaxStepData.u16.Step = Step
            if VarType == EFI_IFR_TYPE_NUM_SIZE_8:
                if MinValue < self.__MinMaxStepData.u8.MinValue:
                    self.__MinMaxStepData.u8.MinValue = MinValue
                if MaxValue > self.__MinMaxStepData.u8.MaxValue:
                    self.__MinMaxStepData.u8.MaxValue = MaxValue
                self.__MinMaxStepData.u8.Step = Step

    def IsNumericOpcode(self):
        return self.__IsNumeric

    def UpdateCIfrMinMaxStepData(self, MinMaxStepData):
        self.__MinMaxStepData = MinMaxStepData

    def GetMinData(self, VarType, IsBitVar): #

        if IsBitVar:
            return self.__MinMaxStepData.u32.MinValue

        else:
            if VarType == EFI_IFR_TYPE_NUM_SIZE_64:
                return self.__MinMaxStepData.u64.MinValue
            if VarType == EFI_IFR_TYPE_NUM_SIZE_32:
                return self.__MinMaxStepData.u32.MinValue
            if VarType == EFI_IFR_TYPE_NUM_SIZE_16:
                return self.__MinMaxStepData.u16.MinValue
            if VarType == EFI_IFR_TYPE_NUM_SIZE_8:
                return self.__MinMaxStepData.u8.MinValue

        return 0

    def GetMaxData(self, VarType, IsBitVar): #

        if IsBitVar:
            return self.__MinMaxStepData.u32.MaxValue

        else:
            if VarType == EFI_IFR_TYPE_NUM_SIZE_64:
                return self.__MinMaxStepData.u64.MaxValue
            if VarType == EFI_IFR_TYPE_NUM_SIZE_32:
                return self.__MinMaxStepData.u32.MaxValue
            if VarType == EFI_IFR_TYPE_NUM_SIZE_16:
                return self.__MinMaxStepData.u16.MaxValue
            if VarType == EFI_IFR_TYPE_NUM_SIZE_8:
                return self.__MinMaxStepData.u8.MaxValue

        return 0

    def GetStepData(self, VarType, IsBitVar): #

        if IsBitVar:
            return self.__MinMaxStepData.u32.Step

        else:
            if VarType == EFI_IFR_TYPE_NUM_SIZE_64:
                return self.__MinMaxStepData.u64.Step
            if VarType == EFI_IFR_TYPE_NUM_SIZE_32:
                return self.__MinMaxStepData.u32.Step
            if VarType == EFI_IFR_TYPE_NUM_SIZE_16:
                return self.__MinMaxStepData.u16.Step
            if VarType == EFI_IFR_TYPE_NUM_SIZE_8:
                return self.__MinMaxStepData.u8.Step

        return 0

class CIfrFormSet(CIfrObj, CIfrOpHeader):

    def __init__(self, Size):#
        self.__FormSet = EFI_IFR_FORM_SET()
        CIfrOpHeader.__init__(self, EFI_IFR_FORM_SET_OP, self.__FormSet.Header, Size)
        self.__FormSet.Help = EFI_STRING_ID_INVALID
        self.__FormSet.FormSetTitle = EFI_STRING_ID_INVALID
        self.__FormSet.Flags = 0
        self.__FormSet.Guid = EFI_GUID(0, 0, 0, GuidArray(0, 0, 0, 0, 0, 0, 0, 0))
        self.__ClassGuid = []

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


class CIfrOneOfOption(CIfrObj, CIfrOpHeader):

    def __init__(self, Size):  #
        self.__OneOfOption = EFI_IFR_ONE_OF_OPTION()
        CIfrOpHeader.__init__(self, EFI_IFR_ONE_OF_OPTION_OP, self.__OneOfOption.Header, Size)
        self.__OneOfOption.Flags = 0
        self.__OneOfOption.Option = EFI_STRING_ID_INVALID
        self.__OneOfOption.Type = EFI_IFR_TYPE_OTHER
        self.__OneOfOption.Value = EFI_IFR_TYPE_VALUE() #

    def SetOption(self, Option):
        self.__OneOfOption.Option = Option

    def SetType(self, Type):
        self.__OneOfOption.Type = Type

    def SetValue(self, Value): #
        self.__OneOfOption.Value = Value

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


class CIfrOptionKey(CIfrObj, CIfrOpHeader):

    def __init__(self, QuestionId, OptionValue, KeyValue):  
        
        self.__OptionKey = EFI_IFR_GUID_OPTIONKEY()
        CIfrOpHeader.__init__(self, EFI_IFR_GUID_OP, self.__OptionKey.Header,
                              ctypes.sizeof(EFI_IFR_GUID_OPTIONKEY))
        self.__OptionKey.ExtendOpCode = EFI_IFR_EXTEND_OP_OPTIONKEY
        self.__OptionKey.Guid = EFI_IFR_FRAMEWORK_GUID
        self.__OptionKey.QuestionId = QuestionId
        self.__OptionKey.OptionValue = OptionValue
        self.__OptionKey.KeyValue = KeyValue


class CIfrClass(CIfrObj, CIfrOpHeader):

    def __init__(self, ):  #
        self.__Class = EFI_IFR_GUID_CLASS()  # static guid
        CIfrOpHeader.__init__(self, EFI_IFR_GUID_OP, self.__Class.Header, ctypes.sizeof(EFI_IFR_GUID_CLASS))
        self.__Class.ExtendOpCode = EFI_IFR_EXTEND_OP_CLASS
        self.__Class.Guid = EFI_IFR_TIANO_GUID
        self.__Class.Class = EFI_NON_DEVICE_CLASS

    def SetClass(self, Class):
        self.__Class = Class


class CIfrSubClass(CIfrObj, CIfrOpHeader):

    def __init__(self, ):  #
        self.__SubClass = EFI_IFR_GUID_SUBCLASS()  # static guid
        CIfrOpHeader.__init__(self, EFI_IFR_GUID_OP, self.__SubClass.Header, ctypes.sizeof(EFI_IFR_GUID_SUBCLASS))
        self.__SubClass.ExtendOpCode = EFI_IFR_EXTEND_OP_SUBCLASS
        self.__SubClass.Guid = EFI_IFR_TIANO_GUID
        self.__SubClass.SubClass = EFI_SETUP_APPLICATION_SUBCLASS

    def SetSubClass(self, SubClass):
        self.__SubClass = SubClass


class CIfrDefaultStore(CIfrObj, CIfrOpHeader):

    def __init__(self, ):  #
        self.__DefaultStore = EFI_IFR_DEFAULTSTORE()
        CIfrOpHeader.__init__(self, EFI_IFR_DEFAULTSTORE_OP, self.__DefaultStore.Header)
        self.__DefaultStore.DefaultName = EFI_STRING_ID_INVALID
        self.__DefaultStore.DefaultId = EFI_VARSTORE_ID_INVALID

    def SetDefaultName(self, DefaultName):
        self.__DefaultStore.DefaultName = DefaultName

    def SetDefaultId(self, DefaultId):
        self.__DefaultStore.DefaultId = DefaultId

    def GetDefaultStore(self):
        return self.__DefaultStore

    def SetDefaultStore(self, DefaultStore: EFI_IFR_DEFAULTSTORE):
        self.__DefaultStore = DefaultStore


class CIfrVarStore(CIfrObj, CIfrOpHeader):

    def __init__(self, ):
        self.__Varstore = EFI_IFR_VARSTORE()
        CIfrOpHeader.__init__(self, EFI_IFR_VARSTORE_OP, self.__Varstore.Header)
        self.__Varstore.VarStoreId = EFI_VARSTORE_ID_INVALID
        self.__Varstore.Size = 0
        self.__Varstore.Name = '' #

    def SetGuid(self, Guid):
        self.__Varstore.Guid = Guid

    def SetSize(self, Size):
        self.__Varstore.Size = Size

    def SetVarStoreId(self, VarStoreId):
        self.__Varstore.VarStoreId = VarStoreId

    def SetName(self, Name):
        self.__Varstore.Name = Name  #


class CIfrVarStoreEfi(CIfrObj, CIfrOpHeader):

    def __init__(self, ):
        self.__VarStoreEfi = EFI_IFR_VARSTORE_EFI()
        CIfrOpHeader.__init__(self, EFI_IFR_VARSTORE_EFI_OP, self.__VarStoreEfi.Header)
        self.__VarStoreEfi.VarStoreId = EFI_VAROFFSET_INVALID
        self.__VarStoreEfi.Size = 0
        self.__VarStoreEfi.Name = '' #

    def SetGuid(self, Guid):
        self.__VarStoreEfi.Guid = Guid

    def SetSize(self, Size):
        self.__VarStoreEfi.Size = Size

    def SetVarStoreId(self, VarStoreId):
        self.__VarStoreEfi.VarStoreId = VarStoreId

    def SetName(self, Name):
        self.__VarStoreEfi.Name = Name  #

    def SetAttributes(self, Attributes):
        self.__VarStoreEfi.Attributes = Attributes


class CIfrVarStoreNameValue(CIfrObj, CIfrOpHeader):

    def __init__(self):
        self.__VarStoreNameValue = EFI_IFR_VARSTORE_NAME_VALUE()
        CIfrOpHeader.__init__(self, EFI_IFR_VARSTORE_NAME_VALUE_OP, self.__VarStoreNameValue.Header)
        self.__VarStoreNameValue.VarStoreId = EFI_VAROFFSET_INVALID

    def SetGuid(self, Guid):
        self.__VarStoreNameValue.Guid = Guid

    def SetVarStoreId(self, VarStoreId):
        self.__VarStoreNameValue.VarStoreId = VarStoreId


EFI_BITS_PER_UINT32 = 1 << EFI_BITS_SHIFT_PER_UINT32
EFI_FORM_ID_MAX = 0xFFFF

EFI_FREE_FORM_ID_BITMAP_SIZE = int((EFI_FORM_ID_MAX + 1) / EFI_BITS_PER_UINT32)


class CIfrFormId():

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


class CIfrForm(CIfrObj, CIfrOpHeader):

    def __init__(self):
        self.__Form = EFI_IFR_FORM()
        CIfrOpHeader.__init__(self, EFI_IFR_FORM_OP, self.__Form.Header)
        self.__Form.FormId = 0
        self.__Form.FormTitle = EFI_STRING_ID_INVALID

    def SetFormId(self, FormId):
        # FormId can't be 0.
        if FormId == 0:
            return VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
        if CIfrFormId.CheckFormIdFree(FormId) == False:
            return VfrReturnCode.VFR_RETURN_FORMID_REDEFINED
        self.__Form.FormId = FormId
        CIfrFormId.MarkFormIdUsed(FormId)
        return VfrReturnCode.VFR_RETURN_SUCCESS

    def SetFormTitle(self, FormTitle):
        self.__Form.FormTitle = FormTitle


class CIfrEnd(CIfrObj, CIfrOpHeader):

    def __init__(self, ):
        self.__End = EFI_IFR_END()
        CIfrOpHeader.__init__(self, EFI_IFR_END_OP, self.__End.Header)


class CIfrBanner(CIfrObj, CIfrOpHeader):

    def __init__(self, ):
        self.__Banner = EFI_IFR_GUID_BANNER()
        CIfrOpHeader.__init__(self, EFI_IFR_GUID_OP, self.__Banner.Header, ctypes.sizeof (EFI_IFR_GUID_BANNER))
        self.__Banner.ExtendOpCode = EFI_IFR_EXTEND_OP_BANNER
        self.__Banner.Guid = EFI_IFR_TIANO_GUID

    def SetTitle(self, StringId):
        self.__Banner.Title = StringId

    def SetLine(self, Line):
        self.__Banner.LineNumber = Line

    def SetAlign(self, Align):
        self.__Banner.Alignment = Align


class CIfrTimeout(CIfrObj, CIfrOpHeader):

    def __init__(self, Timeout=0):
        self.__Timeout = EFI_IFR_GUID_TIMEOUT()
        CIfrOpHeader.__init__(self, EFI_IFR_GUID_OP, self.__Timeout.Header, ctypes.sizeof (EFI_IFR_GUID_TIMEOUT))
        self.__Timeout.ExtendOpCode = EFI_IFR_EXTEND_OP_TIMEOUT
        self.__Timeout.Guid = EFI_IFR_TIANO_GUID
        self.__Timeout.TimeOut = Timeout

    def SetTimeout(self, Timeout):
        self.__Timeout.TimeOut = Timeout


class CIfrLabel(CIfrObj, CIfrOpHeader):

    def __init__(self, ):
        self.__Label = EFI_IFR_GUID_LABEL()
        CIfrOpHeader.__init__(self, EFI_IFR_GUID_OP, self.__Label.Header, ctypes.sizeof (EFI_IFR_GUID_LABEL))
        self.__Label.ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL
        self.__Label.Guid = EFI_IFR_TIANO_GUID

    def SetNumber(self, Number):
        self.__Label.Number = Number


class CIfrRule(CIfrObj, CIfrOpHeader):

    def __init__(self, ):
        self.__Rule = EFI_IFR_RULE()
        CIfrOpHeader.__init__(self, EFI_IFR_RULE_OP, self.__Rule.Header)
        self.__Rule.RuleId = EFI_RULE_ID_INVALID

    def SetRuleId(self, RuleId):
        self.__Rule.RuleId = RuleId


def _FLAG_TEST_AND_CLEAR(Flags, Mask):

    Ret = Flags & Mask
    Flags &= (~Mask)
    return Flags, Ret

def _FLAG_CLEAR(Flags, Mask):

    Flags &= (~Mask)
    return Flags

class CIfrSubtitle(CIfrObj, CIfrOpHeader, CIfrStatementHeader):

    def __init__(self,):
        self.__Subtitle = EFI_IFR_SUBTITLE()

        CIfrOpHeader.__init__(self,EFI_IFR_SUBTITLE_OP, self.__Subtitle.Header)
        CIfrStatementHeader.__init__(self, self.__Subtitle.Statement)

        self.__Subtitle.Flags = 0

    def SetFlags(self, Flags):
        Flags, Result = _FLAG_TEST_AND_CLEAR(Flags,EFI_IFR_FLAGS_HORIZONTAL)
        if Result:
            self.__Subtitle.Flags |= EFI_IFR_FLAGS_HORIZONTAL

        return VfrReturnCode.VFR_RETURN_SUCCESS if Flags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED


class CIfrImage(CIfrObj, CIfrOpHeader):

    def __init__(self, ):
        self.__Image = EFI_IFR_IMAGE()
        CIfrOpHeader.__init__(self,EFI_IFR_IMAGE_OP, self.__Image.Header)
        self.__Image.Id = EFI_IMAGE_ID_INVALID

    def SetImageId(self, ImageId):
        self.__Image.Id = ImageId


class CIfrLocked(CIfrObj, CIfrOpHeader):

    def __init__(self, ):
        self.__Lock = EFI_IFR_LOCKED()
        CIfrOpHeader.__init__(self, EFI_IFR_LOCKED_OP, self.__Lock.Header)


class CIfrModal(CIfrObj, CIfrOpHeader):

    def __init__(self, ):
        self.__Modal = EFI_IFR_MODAL_TAG()
        CIfrOpHeader.__init__(self, EFI_IFR_MODAL_TAG_OP, self.__Modal.Header)


EFI_IFR_QUESTION_FLAG_DEFAULT = 0

class CIfrQuestionHeader(CIfrStatementHeader):

    def __init__(self, qHeader, Flags=EFI_IFR_QUESTION_FLAG_DEFAULT):

        self.__qHeader = qHeader
        CIfrStatementHeader.__init__(self, self.__qHeader.Header)
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
        return  self.__qHeader.VarStoreId

    def SetVarStoreInfo(self, BaseInfo):

        self.__qHeader.VarStoreId = BaseInfo.VarStoreId
        self.__qHeader.VarStoreInfo.VarName = BaseInfo.Info.VarName
        self.__qHeader.VarStoreInfo.VarOffset = BaseInfo.Info.VarOffset


    def GetVarStoreInfo(self, Info): # Bug

        Info.VarStoreId = self.__qHeader.VarStoreId
        Info.VarStoreInfo = self.__qHeader.VarStoreInfo
        return Info


    def GetFlags(self):
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
        Flags = _FLAG_CLEAR (Flags, 0x08)

        Flags, Ret = _FLAG_TEST_AND_CLEAR(Flags, EFI_IFR_FLAG_RESET_REQUIRED)
        if Ret:
            self.__qHeader.Flags |= EFI_IFR_FLAG_RESET_REQUIRED

        Flags, Ret = _FLAG_TEST_AND_CLEAR(Flags, EFI_IFR_FLAG_RECONNECT_REQUIRED)
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

    def UpdateCIfrQuestionHeader(self, qHeader):
        self.__qHeader = qHeader


class CIfrRef(CIfrObj, CIfrOpHeader, CIfrQuestionHeader):
    def __init__(self, ):
        self.__Ref = EFI_IFR_REF()
        CIfrOpHeader.__init__(self,EFI_IFR_REF_OP, self.__Ref.Header)
        CIfrQuestionHeader.__init__(self, self.__Ref.Question)
        self.__Ref.FormId = 0

    def SetFormId(self, FormId):
        self.__Ref.FormId = FormId


class CIfrRef2(CIfrObj, CIfrOpHeader, CIfrQuestionHeader):
    def __init__(self, ):
        self.__Ref2 = EFI_IFR_REF2()
        CIfrOpHeader.__init__(self, EFI_IFR_REF_OP, self.__Ref2.Header)
        CIfrQuestionHeader.__init__(self, self.__Ref2.Question)
        self.__Ref2.FormId = 0
        self.__Ref2.QuestionId = EFI_QUESTION_ID_INVALID

    def SetFormId(self, FormId):
        self.__Ref2.FormId = FormId

    def SetQuestionId(self, QuestionId):

        self.__Ref2.QuestionId = QuestionId


class CIfrRef3(CIfrObj, CIfrOpHeader, CIfrQuestionHeader):
    def __init__(self, ):
        self.__Ref3 = EFI_IFR_REF3()
        CIfrOpHeader.__init__(self, EFI_IFR_REF_OP, self.__Ref3.Header)
        CIfrQuestionHeader.__init__(self, self.__Ref3.Question)
        self.__Ref3.FormId = 0
        self.__Ref3.QuestionId = EFI_QUESTION_ID_INVALID
        EFI_IFR_DEFAULT_GUID = EFI_GUID(0, 0, 0, GuidArray(0, 0, 0, 0, 0, 0, 0, 0))
        self.__Ref3.FormSetId = EFI_IFR_DEFAULT_GUID

    def SetFormId(self, FormId):
        self.__Ref3.FormId = FormId

    def SetQuestionId(self, QuestionId):

        self.__Ref3.QuestionId = QuestionId

    def SetFormSetId(self, FormSetId):
        self.__Ref3.FormSetId = FormSetId


class CIfrRef4(CIfrObj, CIfrOpHeader, CIfrQuestionHeader):
    def __init__(self, ):
        self.__Ref4 = EFI_IFR_REF4()
        CIfrOpHeader.__init__(self, EFI_IFR_REF_OP, self.__Ref4.Header)
        CIfrQuestionHeader.__init__(self, self.__Ref4.Question)
        self.__Ref4.FormId = 0
        self.__Ref4.QuestionId = EFI_QUESTION_ID_INVALID
        EFI_IFR_DEFAULT_GUID = EFI_GUID(0, 0, 0, GuidArray(0, 0, 0, 0, 0, 0, 0, 0))
        self.__Ref4.FormSetId = EFI_IFR_DEFAULT_GUID
        self.__Ref4.DevicePath = EFI_STRING_ID_INVALID

    def SetFormId(self, FormId):
        self.__Ref4.FormId = FormId

    def SetQuestionId(self, QuestionId):

        self.__Ref4.QuestionId = QuestionId

    def SetFormSetId(self, FormSetId):
        self.__Ref4.FormSetId = FormSetId

    def SetDevicePath(self, DevicePath):
        self.__Ref4.DevicePath = DevicePath

class CIfrRef5(CIfrObj, CIfrOpHeader, CIfrQuestionHeader):
    def __init__(self, ):
        self.__Ref5 = EFI_IFR_REF5()
        CIfrOpHeader.__init__(self, EFI_IFR_REF_OP, self.__Ref5.Header)
        CIfrQuestionHeader.__init__(self, self.__Ref5.Question)


class CIfrAction(CIfrObj, CIfrOpHeader, CIfrQuestionHeader):

    def __init__(self, ):
        self.__Action = EFI_IFR_ACTION()
        CIfrOpHeader.__init__(self, EFI_IFR_ACTION_OP, self.__Action.Header)
        CIfrQuestionHeader.__init__(self, self.__Action.Question)
        self.__Action.QuestionConfig = EFI_STRING_ID_INVALID


    def SetQuestionConfig(self, QuestionConfig):
        self.__Action.QuestionConfig = QuestionConfig

class CIfrText(CIfrObj, CIfrOpHeader, CIfrStatementHeader):
    def __init__(self, ):
        self.__Text = EFI_IFR_TEXT()
        CIfrOpHeader.__init__(self, EFI_IFR_TEXT_OP, self.__Text.Header)
        CIfrStatementHeader.__init__(self, self.__Text.Statement)
        self.__Text.TextTwo = EFI_STRING_ID_INVALID

    def SetTextTwo(self, StringId):
        self.__Text.TextTwo = StringId

class CIfrGuid(CIfrObj, CIfrOpHeader):

    def __init__(self, Size):
        self.__Guid = EFI_IFR_GUID()
        CIfrOpHeader.__init__(self, EFI_IFR_GUID_OP, self.__Guid.Header,
                              ctypes.sizeof(EFI_IFR_GUID) + Size)
        EFI_IFR_DEFAULT_GUID = EFI_GUID(0, 0, 0, GuidArray(0, 0, 0, 0, 0, 0, 0, 0))
        self.__Guid.Guid = EFI_IFR_DEFAULT_GUID

    def SetGuid(self, Guid):
        self.__Guid = Guid

    def SetData(self, Databuff, Size): # wip
        pass


class CIfrOrderedList(CIfrObj, CIfrOpHeader, CIfrQuestionHeader):

    def __init__(self):
        self.__OrderedList = EFI_IFR_ORDERED_LIST()
        CIfrOpHeader.__init__(self, EFI_IFR_ORDERED_LIST_OP,
                              self.__OrderedList.Header)
        CIfrQuestionHeader.__init__(self, self.__OrderedList.Question)
        self.__OrderedList.MaxContainers = 0
        self.__OrderedList.Flags = 0

    def GetQuestion(self):
        return self
        # gCurrentQuestion = self.__OrderedList.Question

    def SetMaxContainers(self, MaxContainers):
        self.__OrderedList.MaxContainers = MaxContainers

    def SetFlags(self, HFlags, LFlags):

        ReturnCode = CIfrQuestionHeader.SetFlags(self, HFlags) #
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode
        LFlags, Ret = _FLAG_TEST_AND_CLEAR (LFlags, EFI_IFR_UNIQUE_SET)
        if Ret:
            self.__OrderedList.Flags |= EFI_IFR_UNIQUE_SET
        
        LFlags, Ret = _FLAG_TEST_AND_CLEAR (LFlags, EFI_IFR_NO_EMPTY_SET)
        if Ret:
            self.__OrderedList.Flags |= EFI_IFR_NO_EMPTY_SET

        return VfrReturnCode.VFR_RETURN_SUCCESS if LFlags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED



class CIfrString(CIfrObj, CIfrOpHeader, CIfrQuestionHeader):

    def __init__(self):
        self.__Str = EFI_IFR_STRING()
        CIfrOpHeader.__init__(self, EFI_IFR_STRING_OP, self.__Str.Header)
        CIfrQuestionHeader.__init__(self, self.__Str.Question)
        self.__Str.Flags = 0
        self.__Str.MinSize = 0
        self.__Str.MaxSize = 0


    def GetQuestion(self):
        return self
        # gCurrentQuestion = self.__Str.Question

    def SetFlags(self, HFlags, LFlags):

        ReturnCode = CIfrQuestionHeader.SetFlags(self, HFlags) #
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

class CIfrPassword(CIfrObj, CIfrOpHeader, CIfrQuestionHeader):

    def __init__(self):
        self.__Password = EFI_IFR_PASSWORD()
        CIfrOpHeader.__init__(self, EFI_IFR_PASSWORD_OP,
                              self.__Password.Header)
        CIfrQuestionHeader.__init__(self, self.__Password.Question)
        self.__Password.MinSize = 0
        self.__Password.MaxSize = 0

    def GetQuestion(self):
        return self
        # gCurrentQuestion = self.__Password.Question

    def SetMinSize(self, MinSize):
        self.__Password.MinSize = MinSize

    def SetMaxSize(self, MaxSize):
        self.__Password.MaxSize = MaxSize

class CIfrDefault(CIfrObj, CIfrOpHeader): #

    def __init__(self,
                 Size,
                 DefaultId=EFI_HII_DEFAULT_CLASS_STANDARD,
                 Type=EFI_IFR_TYPE_OTHER,
                 Value=EFI_IFR_TYPE_VALUE()):  #
        self.__Default = EFI_IFR_DEFAULT()
        CIfrOpHeader.__init__(self, EFI_IFR_DEFAULT_OP, self.__Default.Header,
                              Size)
        self.__Default.Type = Type
        self.__Default.DefaultId = DefaultId
        self.__Default.Value = Value

    def SetDefaultId(self, DefaultId):
        self.__Default.DefaultId = DefaultId

    def SetType(self, Type):
        self.__Default.Type = Type

    def SetValue(self, Value): #
        self.__Default.Value = Value


class CIfrDefault2(CIfrObj, CIfrOpHeader):

    def __init__(self,
                 DefaultId=EFI_HII_DEFAULT_CLASS_STANDARD,
                 Type=EFI_IFR_TYPE_OTHER):
        self.__Default = EFI_IFR_DEFAULT_2()
        CIfrOpHeader.__init__(self, EFI_IFR_DEFAULT_OP, self.__Default.Header)
        self.__Default.Type = Type
        self.__Default.DefaultId = DefaultId

    def SetDefaultId(self, DefaultId):
        self.__Default.DefaultId = DefaultId

    def SetType(self, Type):
        self.__Default.Type = Type



class CIfrInconsistentIf(CIfrObj, CIfrOpHeader):

    def __init__(self):
        self.__InconsistentIf = EFI_IFR_INCONSISTENT_IF()
        CIfrOpHeader.__init__(self, EFI_IFR_INCONSISTENT_IF_OP,
                              self.__InconsistentIf.Header)
        self.__InconsistentIf.Error = EFI_STRING_ID_INVALID

    def SetError(self, Error):
        self.__InconsistentIf.Error = Error

class CIfrNoSubmitIf(CIfrObj, CIfrOpHeader):

    def __init__(self):
        self.__NoSubmitIf = EFI_IFR_NO_SUBMIT_IF()
        CIfrOpHeader.__init__(self, EFI_IFR_NO_SUBMIT_IF_OP,
                              self.__NoSubmitIf.Header)
        self.__NoSubmitIf.Error = EFI_STRING_ID_INVALID

    def SetError(self, Error):
        self.__NoSubmitIf.Error = Error

class CIfrDisableIf(CIfrObj, CIfrOpHeader):

    def __init__(self):
        self.__DisableIf = EFI_IFR_DISABLE_IF()
        CIfrOpHeader.__init__(self, EFI_IFR_DISABLE_IF_OP,
                              self.__DisableIf.Header)

class CIfrSuppressIf(CIfrObj, CIfrOpHeader):

    def __init__(self):
        self.__SuppressIf = EFI_IFR_SUPPRESS_IF()
        CIfrOpHeader.__init__(self, EFI_IFR_SUPPRESS_IF_OP,
                              self.__SuppressIf.Header)

class CIfrValue(CIfrObj, CIfrOpHeader):

    def __init__(self):
        self.__Value = EFI_IFR_VALUE()
        CIfrOpHeader.__init__(self, EFI_IFR_VALUE_OP,
                              self.__Value.Header)

class CIfrRead(CIfrObj, CIfrOpHeader):

    def __init__(self):
        self.__Read = EFI_IFR_READ()
        CIfrOpHeader.__init__(self, EFI_IFR_READ_OP,
                              self.__Read.Header)

class CIfrWrite(CIfrObj, CIfrOpHeader):

    def __init__(self):
        self.__Write = EFI_IFR_WRITE()
        CIfrOpHeader.__init__(self, EFI_IFR_WRITE_OP,
                              self.__Write.Header)

class CIfrWarningIf(CIfrObj, CIfrOpHeader):

    def __init__(self):
        self.__WarningIf = EFI_IFR_WARNING_IF()
        CIfrOpHeader.__init__(self, EFI_IFR_WARNING_IF_OP,
                              self.__WarningIf.Header)
        self.__WarningIf.Warning = EFI_STRING_ID_INVALID
        self.__WarningIf.TimeOut = 0

    def SetWarning(self, Warning):
        self.__WarningIf.Warning = Warning

    def SetTimeOut(self, TimeOut):
        self.__WarningIf.TimeOut = TimeOut



class CIfrRefresh(CIfrObj, CIfrOpHeader):

    def __init__(self):
        self.__Refresh = EFI_IFR_REFRESH()
        CIfrOpHeader.__init__(self, EFI_IFR_REFRESH_OP,
                              self.__Refresh.Header)
        self.__Refresh.RefreshInterval = 0

    def SetRefreshInterval(self, RefreshInterval):
        self.__Refresh.RefreshInterval = RefreshInterval

class CIfrRefreshId(CIfrObj, CIfrOpHeader):

    def __init__(self):
        self.__RefreshId = EFI_IFR_REFRESH_ID()
        CIfrOpHeader.__init__(self, EFI_IFR_REFRESH_ID_OP,
                              self.__RefreshId.Header)
        self.__RefreshId.RefreshEventGroupId = EFI_GUID(0, 0, 0, GuidArray(0, 0, 0, 0, 0, 0, 0, 0))

    def SetRefreshEventGroutId(self, RefreshEventGroupId):
        self.__RefreshId.RefreshEventGroupId = RefreshEventGroupId

class CIfrVarStoreDevice(CIfrObj, CIfrOpHeader):

    def __init__(self):
        self.__VarStoreDevice = EFI_IFR_VARSTORE_DEVICE()
        CIfrOpHeader.__init__(self, EFI_IFR_VARSTORE_DEVICE_OP,
                              self.__VarStoreDevice.Header)
        self.__VarStoreDevice.DevicePath = EFI_STRING_ID_INVALID

    def SetDevicePath(self, DevicePath):
        self.__VarStoreDevice.DevicePath = DevicePath


class CIfrDate(CIfrObj, CIfrOpHeader, CIfrQuestionHeader):

    def __init__(self):
        self.__Date = EFI_IFR_DATE()
        CIfrOpHeader.__init__(self, EFI_IFR_DATE_OP,
                              self.__Date.Header)
        CIfrQuestionHeader.__init__(self, self.__Date.Question)
        self.__Date.Flags = 0

    def SetFlags(self, HFlags, LFlags):

        ReturnCode = CIfrQuestionHeader.SetFlags(self, HFlags) #
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode

        LFlags, Ret = _FLAG_TEST_AND_CLEAR (LFlags, EFI_QF_DATE_YEAR_SUPPRESS)
        if Ret:
            self.__Date.Flags |= EFI_QF_DATE_YEAR_SUPPRESS

        LFlags, Ret = _FLAG_TEST_AND_CLEAR (LFlags, EFI_QF_DATE_MONTH_SUPPRESS)
        if Ret:
            self.__Date.Flags |= EFI_QF_DATE_MONTH_SUPPRESS

        LFlags, Ret = _FLAG_TEST_AND_CLEAR (LFlags, EFI_QF_DATE_DAY_SUPPRESS)
        if Ret:
            self.__Date.Flags |= EFI_QF_DATE_DAY_SUPPRESS

        LFlags, Ret = _FLAG_TEST_AND_CLEAR (LFlags, QF_DATE_STORAGE_NORMAL)
        if Ret:
            self.__Date.Flags |= QF_DATE_STORAGE_NORMAL

        LFlags, Ret = _FLAG_TEST_AND_CLEAR (LFlags, QF_DATE_STORAGE_TIME)
        if Ret:
            self.__Date.Flags |= QF_DATE_STORAGE_TIME

        LFlags, Ret = _FLAG_TEST_AND_CLEAR (LFlags, QF_DATE_STORAGE_WAKEUP)
        if Ret:
            self.__Date.Flags |= QF_DATE_STORAGE_WAKEUP

        return VfrReturnCode.VFR_RETURN_SUCCESS if LFlags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED


class CIfrTime(CIfrObj, CIfrOpHeader, CIfrQuestionHeader):

    def __init__(self):
        self.__Time = EFI_IFR_TIME()
        CIfrOpHeader.__init__(self, EFI_IFR_TIME_OP,
                              self.__Time.Header)
        CIfrQuestionHeader.__init__(self, self.__Time.Question)
        self.__Time.Flags = 0

    def SetFlags(self, HFlags, LFlags):

        ReturnCode = CIfrQuestionHeader.SetFlags(self, HFlags) #
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode

        LFlags, Ret = _FLAG_TEST_AND_CLEAR (LFlags, QF_TIME_HOUR_SUPPRESS)
        if Ret:
            self.__Time.Flags |= QF_TIME_HOUR_SUPPRESS

        LFlags, Ret = _FLAG_TEST_AND_CLEAR (LFlags, QF_TIME_MINUTE_SUPPRESS)
        if Ret:
            self.__Time.Flags |= QF_TIME_MINUTE_SUPPRESS

        LFlags, Ret = _FLAG_TEST_AND_CLEAR (LFlags, QF_TIME_SECOND_SUPPRESS)
        if Ret:
            self.__Time.Flags |= QF_TIME_SECOND_SUPPRESS

        LFlags, Ret = _FLAG_TEST_AND_CLEAR (LFlags, QF_TIME_STORAGE_NORMAL)
        if Ret:
            self.__Time.Flags |= QF_TIME_STORAGE_NORMAL

        LFlags, Ret = _FLAG_TEST_AND_CLEAR (LFlags, QF_TIME_STORAGE_TIME)
        if Ret:
            self.__Time.Flags |= QF_TIME_STORAGE_TIME

        LFlags, Ret = _FLAG_TEST_AND_CLEAR (LFlags, QF_TIME_STORAGE_WAKEUP)
        if Ret:
            self.__Time.Flags |= QF_TIME_STORAGE_WAKEUP

        return VfrReturnCode.VFR_RETURN_SUCCESS if LFlags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED


class CIfrNumeric(CIfrObj, CIfrOpHeader, CIfrQuestionHeader, CIfrMinMaxStepData):

    def __init__(self):
        self.__Numeric = EFI_IFR_NUMERIC() # data
        CIfrOpHeader.__init__(self, EFI_IFR_NUMERIC_OP, self.__Numeric.Header)
        CIfrQuestionHeader.__init__(self, self.__Numeric.Question)
        CIfrMinMaxStepData.__init__(self, self.__Numeric.Data, True)
        self.__Numeric.Flags = EFI_IFR_NUMERIC_SIZE_1 | EFI_IFR_DISPLAY_UINT_DEC

    def GetQuestion(self):
        return self

    def GetMinMaxData(self, MinMaxStepData):
        return self.__Numeric.Data
    
    def SetFlags(self, HFlags, LFlags, DisplaySettingsSpecified=False):
        ReturnCode = CIfrQuestionHeader.SetFlags(self, HFlags) #
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode
        if DisplaySettingsSpecified == False:
            self.__Numeric.Flags = LFlags | EFI_IFR_DISPLAY_UINT_DEC
        else:
            self.__Numeric.Flags = LFlags
        return VfrReturnCode.VFR_RETURN_SUCCESS
    
    def SetFlagsForBitField(self, HFlags, LFlags, DisplaySettingsSpecified=False):
        ReturnCode = CIfrQuestionHeader.SetFlags(self, HFlags) #
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
        self.UpdateHeader()
        
        

class CIfrOneOf(CIfrObj, CIfrOpHeader, CIfrQuestionHeader, ):  #

    def __init__(self):
        self.__OneOf = EFI_IFR_ONE_OF()
        CIfrOpHeader.__init__(self, EFI_IFR_ONE_OF_OP,
                              self.__OneOf.Header)
        CIfrQuestionHeader.__init__(self, self.__OneOf.Question)

class CIfrCheckBox(CIfrObj, CIfrOpHeader, CIfrQuestionHeader):

    def __init__(self):
        self.__CheckBox = EFI_IFR_CHECKBOX()
        CIfrOpHeader.__init__(self, EFI_IFR_CHECKBOX_OP,
                              self.__CheckBox.Header)
        CIfrQuestionHeader.__init__(self, self.__CheckBox.Question)
        self.__CheckBox.Flags = 0

    def GetQuestion(self):
        return self
        # gCurrentQuestion = self.__CheckBox.Question

    def GetFlags(self):
        return self.__CheckBox.Flags

    def SetFlags(self, HFlags, LFlags):

        ReturnCode = CIfrQuestionHeader.SetFlags(self, HFlags) #
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode
        
        LFlags, Ret = _FLAG_TEST_AND_CLEAR (LFlags, EFI_IFR_CHECKBOX_DEFAULT)
        if Ret:
            self.__CheckBox.Flags |= EFI_IFR_CHECKBOX_DEFAULT

        LFlags, Ret = _FLAG_TEST_AND_CLEAR (LFlags, EFI_IFR_CHECKBOX_DEFAULT_MFG)
        if Ret:
            self.__CheckBox.Flags |= EFI_IFR_CHECKBOX_DEFAULT_MFG

        VfrReturnCode.VFR_RETURN_SUCCESS if LFlags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED


class CIfrResetButton(CIfrObj, CIfrOpHeader, CIfrStatementHeader):

    def __init__(self):
        self.__ResetButton = EFI_IFR_RESET_BUTTON()
        CIfrOpHeader.__init__(self, EFI_IFR_RESET_BUTTON_OP,
                              self.__ResetButton.Header)
        CIfrStatementHeader.__init__(self, self.__ResetButton.Statement)
        self.__ResetButton.DefaultId = EFI_HII_DEFAULT_CLASS_STANDARD

    def SetDefaultId(self, DefaultId):
        self.__ResetButton.DefaultId = DefaultId
