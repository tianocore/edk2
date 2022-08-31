from ast import For
from stat import FILE_ATTRIBUTE_SPARSE_FILE
from CommonCtypes import *
from VfrError import VfrReturnCode
from VfrUtility import *

from ctypes import *

gCreateOp = True


class SBufferNode():

    def __init__(self, Buffer='', Next=None):
        self.Buffer = Buffer
        self.Next = Next


class CFormPkg(object):

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

        return self.__CurrBufferNode.Buffer  # 返回


gCFormPkg = CFormPkg()
gCreateOp = True


#GetObjBinAddr return self.__ObjBinBuf的地址
class CIfrobj(object):  # wip

    #Opcode是用来计算Buff的大小的,但是因为这里用string,所以不需要手动知道存放的struct大小

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

    def SetLineNo(self, LineNo):
        self.__LineNo = LineNo

    def GetObjBinOffset(self):
        return self.__PkgOffset

    def GetObjBinLen(self):
        return len(self.__ObjBinBuf)

    def ExpendObjBin(self, Size):
        if self.__DelayEmit == True and (self.__ObjBinLen +
                                         Size) > self.__ObjBinLen:
            self.__ObjBinLen = self.__ObjBinLen + Size
            return True
        else:
            return False

    def GetObjBin(self):
        return self.__ObjBinBuf


class CIfrOpHeader(object):  # wip

    def __init__(self, OpCode, Length):
        self.__Header = EFI_IFR_OP_HEADER()
        self.__Header.Opcode = OpCode

    # self.__Header.Length = Length if Length != 0 else OpcodeSizesScopeTable[Opcode]

    def SetScope(self, Scope):
        self.__Scope = Scope


class CIfrStatementHeader():

    def __init__(self, sHeader):
        self.__sHeader = sHeader
        self.__sHeader.Help = EFI_STRING_ID_INVALID
        self.__sHeader.Prompt = EFI_STRING_ID_INVALID

    def GetStatementHeader(self):
        return self.__sHeader

    def SetPrompt(self, Prompt):
        self.__sHeader.Prompt = Prompt

    def SetHelp(self, Help):
        self.__sHeader.Help = Help


class CIfrFormSet(CIfrobj, CIfrOpHeader):

    def __init__(self, ):  #
        self.__FormSet = EFI_IFR_FORM_SET()
        self.__FormSet.Help = EFI_STRING_ID_INVALID
        self.__FormSet.FormSetTitle = EFI_STRING_ID_INVALID
        self.__FormSet.Flags = 0
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


class CIfrClass(CIfrobj, CIfrOpHeader):

    def __init__(self, ):  #
        self.__Class = EFI_IFR_GUID_CLASS()  # static guid
        self.__Class.ExtendOpCode = EFI_IFR_EXTEND_OP_CLASS
        self.__Class.Guid = EFI_IFR_TIANO_GUID
        self.__Class.Class = EFI_NON_DEVICE_CLASS

    def SetClass(self, Class):
        self.__Class = Class


class CIfrSubClass(CIfrobj, CIfrOpHeader):

    def __init__(self, ):  #
        self.__SubClass = EFI_IFR_GUID_CLASS()  # static guid
        self.__Class.ExtendOpCode = EFI_IFR_EXTEND_OP_SUBCLASS
        self.__Class.Guid = EFI_IFR_TIANO_GUID
        self.__Class.Class = EFI_SETUP_APPLICATION_SUBCLASS

    def SetSubClass(self, SubClass):
        self.__SubClass = SubClass


class CIfrDefaultStore(CIfrobj, CIfrOpHeader):

    def __init__(self, ):  #
        self.__DefaultStore = EFI_IFR_DEFAULTSTORE()
        self.__DefaultStore.DefaultName = EFI_STRING_ID_INVALID
        self.__DefaultStore.DefaultId = EFI_VARSTORE_ID_INVALID

    def SetDefaultName(self, DefaultName):
        self.__DefaultStore.DefaultName = DefaultName

    def SetDefaultId(self, DefaultId):
        self.__DefaultStore.DefaultId = DefaultId


class CIfrVarStore(CIfrobj, CIfrOpHeader):

    def __init__(self, ):
        self.__Varstore = EFI_IFR_VARSTORE()
        self.__Varstore.VarStoreId = EFI_VARSTORE_ID_INVALID
        self.__Varstore.Size = 0

    # self.__Varstore.Name[0] = '\0' #

    def SetGuid(self, Guid):
        self.__Varstore.Guid = Guid

    def SetSize(self, Size):
        self.__Varstore.Size = Size

    def SetVarStoreId(self, VarStoreId):
        self.__Varstore.VarStoreId = VarStoreId

    def SetName(self, Name):
        self.__Varstore.Name = Name  #pending


class CIfrVarStoreEfi(CIfrobj, CIfrOpHeader):

    def __init__(self, ):
        self.__VarStoreEfi = EFI_IFR_VARSTORE_EFI()
        self.__VarStoreEfi.VarStoreId = EFI_VAROFFSET_INVALID
        self.__VarStoreEfi.Size = 0

    #  self.__Varstore.Name[0] = '\0' #

    def SetGuid(self, Guid):
        self.__VarStoreEfi.Guid = Guid

    def SetSize(self, Size):
        self.__VarStoreEfi.Size = Size

    def SetVarStoreId(self, VarStoreId):
        self.__VarStoreEfi.VarStoreId = VarStoreId

    def SetName(self, Name):
        self.__VarStoreEfi.Name = Name  #pending

    def SetAttributes(self, Attributes):
        self.__VarStoreEfi.Attributes = Attributes


class CIfrVarStoreNameValue(CIfrobj, CIfrOpHeader):

    def __init__(self):
        self.__VarStoreNameValue = EFI_IFR_VARSTORE_NAME_VALUE()
        self.__VarStoreNameValue.VarStoreId = EFI_VAROFFSET_INVALID

    def SetGuid(self, Guid):
        self.__VarStoreEfi.Guid = Guid

    def SetVarStoreId(self, VarStoreId):
        self.__VarStoreEfi.VarStoreId = VarStoreId


EFI_BITS_PER_UINT32 = 1 << EFI_BITS_SHIFT_PER_UINT32
EFI_FORM_ID_MAX = 0xFFFF

EFI_FREE_FORM_ID_BITMAP_SIZE = int((EFI_FORM_ID_MAX + 1) / EFI_BITS_PER_UINT32)


class CIfrFormId(object):

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


class CIfrForm(CIfrobj, CIfrOpHeader):

    def __init__(self):
        self.__Form = EFI_IFR_FORM()
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


class CIfrEnd(CIfrobj, CIfrOpHeader):

    def __init__(self, ):
        pass


class CIfrBanner(CIfrobj, CIfrOpHeader):

    def __init__(self, ):
        self.__Banner = EFI_IFR_GUID_BANNER()
        self.__Banner.ExtendOpCode = EFI_IFR_EXTEND_OP_BANNER
        self.__Banner.Guid = EFI_IFR_TIANO_GUID

    def SetTitle(self, StringId):
        self.__Banner.Title = StringId

    def SetLine(self, Line):
        self.__Banner.LineNumber = Line

    def SetAlign(self, Align):
        self.__Banner.Alignment = Align


class CIfrTimeout(CIfrobj, CIfrOpHeader):

    def __init__(self, Timeout=0):
        self.__Timeout = EFI_IFR_GUID_TIMEOUT()
        self.__Timeout.ExtendOpCode = EFI_IFR_EXTEND_OP_TIMEOUT
        self.__Timeout.Guid = EFI_IFR_TIANO_GUID
        self.__Timeout.TimeOut = Timeout

    def SetTimeout(self, Timeout):
        self.__Timeout.TimeOut = Timeout


class CIfrLabel(CIfrobj, CIfrOpHeader):

    def __init__(self, ):
        self.__Label = EFI_IFR_GUID_LABEL()
        self.__Label.ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL
        self.__Label.Guid = EFI_IFR_TIANO_GUID

    def SetNumber(self, Number):
        self.__Label.Number = Number


class CIfrRule(CIfrobj, CIfrOpHeader):

    def __init__(self, ):
        self.__Rule = EFI_IFR_RULE()
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


class CIfrSubtitle(CIfrobj, CIfrOpHeader, CIfrStatementHeader):

    def __init__(self, ):
        self.__Subtitle = EFI_IFR_SUBTITLE()
        CIfrStatementHeader.__init__(self, self.__Subtitle.Statement)
        self.__Subtitle.Flags = 0

    def SetFlags(self, Flags):
        Flags, Result = _FLAG_TEST_AND_CLEAR(Flags, EFI_IFR_FLAGS_HORIZONTAL)
        if Result:
            self.__Subtitle.Flags |= EFI_IFR_FLAGS_HORIZONTAL

        return VfrReturnCode.VFR_RETURN_SUCCESS if Flags == 0 else VfrReturnCode.VFR_RETURN_FLAGS_UNSUPPORTED


class CIfrImage(CIfrobj, CIfrOpHeader):

    def __init__(self, ):
        self.__Image = EFI_IFR_IMAGE()
        self.__Image.Id = EFI_IMAGE_ID_INVALID

    def SetImageId(self, ImageId):
        self.__Image.Id = ImageId


class CIfrLocked(CIfrobj, CIfrOpHeader):

    def __init__(self, ):
        pass


class CIfrModal(CIfrobj, CIfrOpHeader):

    def __init__(self, ):
        pass


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
        return self.__qHeader.VarStoreId

    def SetVarStoreInfo(self, BaseInfo):

        self.__qHeader.VarStoreId = BaseInfo.VarStoreId
        self.__qHeader.VarStoreInfo.VarName = BaseInfo.Info.VarName
        self.__qHeader.VarStoreInfo.VarOffset = BaseInfo.Info.VarOffset

    def GetVarStoreInfo(self, Info):  # Bug

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

    def UpdateCIfrQuestionHeader(self, qHeader):
        self.__qHeader = qHeader


EFI_IFR_DEFAULT_GUID = EFI_GUID(0, 0, 0, GuidArray(0, 0, 0, 0, 0, 0, 0, 0))


class CIfrRef(CIfrobj, CIfrOpHeader, CIfrQuestionHeader):

    def __init__(self, ):
        self.__Ref = EFI_IFR_REF()
        CIfrQuestionHeader.__init__(self, self.__Ref.Question)
        self.__Ref.FormId = 0

    def SetFormId(self, FormId):
        self.__Ref.FormId = FormId


class CIfrRef2(CIfrobj, CIfrOpHeader, CIfrQuestionHeader):

    def __init__(self, ):
        self.__Ref2 = EFI_IFR_REF2()
        CIfrQuestionHeader.__init__(self, self.__Ref2.Question)
        self.__Ref2.FormId = 0
        self.__Ref2.QuestionId = EFI_QUESTION_ID_INVALID

    def SetFormId(self, FormId):
        self.__Ref2.FormId = FormId

    def SetQuestionId(self, QuestionId):

        self.__Ref2.QuestionId = QuestionId


class CIfrRef3(CIfrobj, CIfrOpHeader, CIfrQuestionHeader):

    def __init__(self, ):
        self.__Ref3 = EFI_IFR_REF3()
        CIfrQuestionHeader.__init__(self, self.__Ref3.Question)
        self.__Ref3.FormId = 0
        self.__Ref3.QuestionId = EFI_QUESTION_ID_INVALID
        self.__Ref3.FormSetId = EFI_IFR_DEFAULT_GUID

    def SetFormId(self, FormId):
        self.__Ref3.FormId = FormId

    def SetQuestionId(self, QuestionId):

        self.__Ref3.QuestionId = QuestionId

    def SetFormSetId(self, FormSetId):
        self.__Ref3.FormSetId = FormSetId


class CIfrRef4(CIfrobj, CIfrOpHeader, CIfrQuestionHeader):

    def __init__(self, ):
        self.__Ref4 = EFI_IFR_REF4()
        CIfrQuestionHeader.__init__(self, self.__Ref4.Question)
        self.__Ref4.FormId = 0
        self.__Ref4.QuestionId = EFI_QUESTION_ID_INVALID
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


class CIfrRef5(CIfrobj, CIfrOpHeader, CIfrQuestionHeader):

    def __init__(self, ):
        self.__Ref5 = EFI_IFR_REF5()
        CIfrQuestionHeader.__init__(self, self.__Ref5.Question)


class CIfrAction(CIfrobj, CIfrOpHeader, CIfrQuestionHeader):

    def __init__(self, ):
        self.__Action = EFI_IFR_ACTION()
        CIfrQuestionHeader.__init__(self, self.__Action.Question)
        self.__Action.QuestionConfig = EFI_STRING_ID_INVALID

    def SetQuestionConfig(self, QuestionConfig):
        self.__Action.QuestionConfig = QuestionConfig


class CIfrText(CIfrobj, CIfrOpHeader, CIfrStatementHeader):

    def __init__(self, ):
        self.__Text = EFI_IFR_TEXT()
        CIfrStatementHeader.__init__(self, self.__Text.Statement)
        self.__Text.TextTwo = EFI_STRING_ID_INVALID

    def SetTextTwo(self, StringId):
        self.__Text.TextTwo = StringId


class CIfrGuid(CIfrobj, CIfrOpHeader):

    def __init__(self, ):
        self.__Guid = None

    def SetGuid(self, Guid):
        self.__Guid = Guid

    def SetData(self, Databuff, Size):
        pass
