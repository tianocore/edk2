from cgi import print_environ_usage
from email.errors import NonASCIILocalPartDefect, NonPrintableDefect
from enum import Flag
from fileinput import lineno
from http.client import ResponseNotReady
from itertools import count
from modulefinder import STORE_NAME
from msilib.schema import CreateFolder
from re import T
from sre_parse import FLAGS
from tokenize import Number
from antlr4 import *
from CommonCtypes import *
from VfrFormPkg import *
from VfrUtility import *
from VfrTree import *
import ctypes
import struct

if __name__ is not None and "." in __name__:
    from .VfrSyntaxParser import VfrSyntaxParser
else:
    from VfrSyntaxParser import VfrSyntaxParser

gCVfrVarDataTypeDB = CVfrVarDataTypeDB()
gCVfrDefaultStore =  CVfrDefaultStore()
gCVfrDataStorage = CVfrDataStorage()

# This class defines a complete generic visitor for a parse tree produced by VfrSyntaxParser.

class VfrSyntaxVisitor(ParseTreeVisitor):
    gZeroEfiIfrTypeValue = EFI_IFR_TYPE_VALUE()

    def __init__(self):
        self.__OverrideClassGuid = None
        self.__ParserStatus = 0
        self.__CIfrOpHdrIndex = -1
        self.__ConstantOnlyInExpression = False

        self.__CVfrRulesDB = CVfrRulesDB()
        self.__CIfrOpHdr = []  # MAX_IFR_EXPRESSION_DEPTH
        self.__CIfrOpHdrLineNo = []
        self.__CurrQestVarInfo = EFI_VARSTORE_INFO()

        self.__CVfrQuestionDB = CVfrQuestionDB()
        self.__CurrentQuestion = None
        self.__CurrentMinMaxData = None # static CIfrMinMaxStepData *gCurrentMinMaxData = NULL;

        self.__IsStringOp = False # static BOOLEAN gIsStringOp = FALSE;
        self.__IsOrderedList = False # static BOOLEAN  gIsOrderedList = FALSE;
        self.__IsCheckBoxOp = False
        self.__IsOneOfOp = False
        self.__IsNumericOp = False
        self.__IsTimeOp = False

        self.__Root = VfrTreeNode()
    
    def GetRoot(self):
        return self.__Root
    
    def GetQuestionDB(self):
        return self.__CVfrQuestionDB

    def __TransId(self, StringIdentifierToken, DefaultValue=None):
        if StringIdentifierToken == None:
            return DefaultValue
        else:
            return str(StringIdentifierToken)

    def __TransNum(self, NumberToken, DefaultValue=0):
        if NumberToken == None:
            return DefaultValue
        else:
            NumberToken = int(str(NumberToken), 0)
        # error handle , value is too large to store
        return NumberToken

    def __ExtractOriginalText(self, ctx):
        Source = ctx.start.getTokenSource()
        InputStream = Source.inputStream
        start, stop  = ctx.start.start, ctx.stop.stop
        return InputStream.getText(start, stop)


    # Visit a parse tree produced by VfrSyntaxParser#vfrProgram.
    def visitVfrProgram(self, ctx:VfrSyntaxParser.VfrProgramContext):
        # Basic DataType declare

        gCVfrVarDataTypeDB.DeclareDataTypeBegin()
        gCVfrVarDataTypeDB.SetNewTypeName('UINT8')
        gCVfrVarDataTypeDB.SetNewTypeTotalSize(sizeof(ctypes.c_ubyte))
        gCVfrVarDataTypeDB.SetNewTypeType(EFI_IFR_TYPE_NUM_SIZE_8)
        gCVfrVarDataTypeDB.DeclareDataTypeEnd()

        gCVfrVarDataTypeDB.DeclareDataTypeBegin()
        gCVfrVarDataTypeDB.SetNewTypeName('UINT16')
        gCVfrVarDataTypeDB.SetNewTypeTotalSize(sizeof(ctypes.c_ushort))
        gCVfrVarDataTypeDB.SetNewTypeType(EFI_IFR_TYPE_NUM_SIZE_16)
        gCVfrVarDataTypeDB.DeclareDataTypeEnd()

        gCVfrVarDataTypeDB.DeclareDataTypeBegin()
        gCVfrVarDataTypeDB.SetNewTypeName('UINT32')
        gCVfrVarDataTypeDB.SetNewTypeTotalSize(sizeof(ctypes.c_ulong))
        gCVfrVarDataTypeDB.SetNewTypeType(EFI_IFR_TYPE_NUM_SIZE_32)
        gCVfrVarDataTypeDB.DeclareDataTypeEnd()

        gCVfrVarDataTypeDB.DeclareDataTypeBegin()
        gCVfrVarDataTypeDB.SetNewTypeName('UINT64')
        gCVfrVarDataTypeDB.SetNewTypeTotalSize(sizeof(ctypes.c_ulonglong))
        gCVfrVarDataTypeDB.SetNewTypeType(EFI_IFR_TYPE_NUM_SIZE_64)
        gCVfrVarDataTypeDB.DeclareDataTypeEnd()

        gCVfrVarDataTypeDB.DeclareDataTypeBegin()
        gCVfrVarDataTypeDB.SetNewTypeName('BOOLEAN')
        gCVfrVarDataTypeDB.SetNewTypeTotalSize(sizeof(ctypes.c_ubyte))
        gCVfrVarDataTypeDB.SetNewTypeType(EFI_IFR_TYPE_BOOLEAN)
        gCVfrVarDataTypeDB.DeclareDataTypeEnd()

        gCVfrVarDataTypeDB.DeclareDataTypeBegin()
        gCVfrVarDataTypeDB.SetNewTypeName('EFI_STRING_ID')
        gCVfrVarDataTypeDB.SetNewTypeTotalSize(sizeof(ctypes.c_ushort))
        gCVfrVarDataTypeDB.SetNewTypeType(EFI_IFR_TYPE_STRING)
        gCVfrVarDataTypeDB.DeclareDataTypeEnd()

        gCVfrVarDataTypeDB.DeclareDataTypeBegin()
        gCVfrVarDataTypeDB.SetNewTypeName('EFI_HII_DATE')
        gCVfrVarDataTypeDB.SetNewTypeType(EFI_IFR_TYPE_DATE)
        gCVfrVarDataTypeDB.SetNewTypeTotalSize(sizeof(EFI_HII_DATE))
        gCVfrVarDataTypeDB.DataTypeAddField('Year', 'UINT16', 0, False)
        gCVfrVarDataTypeDB.DataTypeAddField('Month', 'UINT8', 0, False)
        gCVfrVarDataTypeDB.DataTypeAddField('Day', 'UINT8', 0, False)
        gCVfrVarDataTypeDB.DeclareDataTypeEnd()

        gCVfrVarDataTypeDB.DeclareDataTypeBegin()
        gCVfrVarDataTypeDB.SetNewTypeName('EFI_HII_TIME')
        gCVfrVarDataTypeDB.SetNewTypeType(EFI_IFR_TYPE_TIME)
        gCVfrVarDataTypeDB.SetNewTypeTotalSize(sizeof(EFI_HII_TIME))
        gCVfrVarDataTypeDB.DataTypeAddField('Hour', 'UINT8', 0, False)
        gCVfrVarDataTypeDB.DataTypeAddField('Minute', 'UINT8', 0, False)
        gCVfrVarDataTypeDB.DataTypeAddField('Second', 'UINT8', 0, False)
        gCVfrVarDataTypeDB.DeclareDataTypeEnd()

        gCVfrVarDataTypeDB.DeclareDataTypeBegin()
        gCVfrVarDataTypeDB.SetNewTypeName('EFI_GUID')
        gCVfrVarDataTypeDB.SetNewTypeType(EFI_IFR_TYPE_OTHER)
        gCVfrVarDataTypeDB.SetNewTypeTotalSize(sizeof(EFI_GUID))
        gCVfrVarDataTypeDB.DataTypeAddField('Data1', 'UINT32', 0, False)
        gCVfrVarDataTypeDB.DataTypeAddField('Data2', 'UINT16', 0, False)
        gCVfrVarDataTypeDB.DataTypeAddField('Data3', 'UINT16', 0, False)
        gCVfrVarDataTypeDB.DataTypeAddField('Data4', 'UINT8', 8, False)
        gCVfrVarDataTypeDB.DeclareDataTypeEnd()


        gCVfrVarDataTypeDB.DeclareDataTypeBegin()
        gCVfrVarDataTypeDB.SetNewTypeName('EFI_HII_REF')
        gCVfrVarDataTypeDB.SetNewTypeType(EFI_IFR_TYPE_REF)
        gCVfrVarDataTypeDB.SetNewTypeTotalSize(sizeof(EFI_HII_REF))
        gCVfrVarDataTypeDB.DataTypeAddField('QuestionId', 'UINT16', 0, False)
        gCVfrVarDataTypeDB.DataTypeAddField('FormId', 'UINT16', 0, False)
        gCVfrVarDataTypeDB.DataTypeAddField('FormSetGuid', 'EFI_GUID', 0, False)
        gCVfrVarDataTypeDB.DataTypeAddField('DevicePath', 'EFI_STRING_ID', 0, False)
        gCVfrVarDataTypeDB.DeclareDataTypeEnd()

        self.visitChildren(ctx)

        self.__CVfrQuestionDB.PrintAllQuestion('test\\Questions.txt')
        gCVfrVarDataTypeDB.Dump("test\\DataTypeInfo.txt")
        
        return

    # Visit a parse tree produced by VfrSyntaxParser#pragmaPackShowDef.
    def visitPragmaPackShowDef(self, ctx:VfrSyntaxParser.PragmaPackShowDefContext):

        Line = (None if ctx.start is None else ctx.start).line
        gCVfrVarDataTypeDB.Pack(Line, VFR_PACK_SHOW)
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#pragmaPackStackDef.
    def visitPragmaPackStackDef(self, ctx:VfrSyntaxParser.PragmaPackStackDefContext):

        Identifier = self.__TransId(ctx.StringIdentifier())

        if str(ctx.getChild(0)) == 'push':
            Action = VFR_PACK_PUSH
        else:
            Action = VFR_PACK_POP

        if ctx.Number() != None:
            Action |= VFR_PACK_ASSIGN

        PackNumber = self.__TransNum(ctx.Number(), DEFAULT_PACK_ALIGN)
        Line = (None if ctx.start is None else ctx.start).line
        gCVfrVarDataTypeDB.Pack(Line, Action, Identifier, PackNumber)
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#pragmaPackNumber.
    def visitPragmaPackNumber(self, ctx:VfrSyntaxParser.PragmaPackNumberContext):

        Line = (None if ctx.start is None else ctx.start).line
        PackNumber = self.__TransNum(ctx.Number(), DEFAULT_PACK_ALIGN)

        gCVfrVarDataTypeDB.Pack(Line, VFR_PACK_ASSIGN, None, PackNumber)
        return self.visitChildren(ctx)

    # Visit a parse tree produced by VfrSyntaxParser#vfrPragmaPackDefinition.
    def visitVfrPragmaPackDefinition(
            self, ctx: VfrSyntaxParser.VfrPragmaPackDefinitionContext):
        return self.visitChildren(ctx)

    # Visit a parse tree produced by VfrSyntaxParser#vfrDataStructDefinition.
    def visitVfrDataStructDefinition(self, ctx: VfrSyntaxParser.VfrDataStructDefinitionContext):

        gCVfrVarDataTypeDB.DeclareDataTypeBegin()

        for Identifier in ctx.StringIdentifier():
            gCVfrVarDataTypeDB.SetNewTypeName(self.__TransId(Identifier))  #error handle _PCATCH

        self.visitChildren(ctx)

        gCVfrVarDataTypeDB.DeclareDataTypeEnd()

        return None


    # Visit a parse tree produced by VfrSyntaxParser#vfrDataUnionDefinition.
    def visitVfrDataUnionDefinition(self, ctx:VfrSyntaxParser.VfrDataUnionDefinitionContext):
        gCVfrVarDataTypeDB.DeclareDataTypeBegin()

        for Identifier in  ctx.StringIdentifier() :
            gCVfrVarDataTypeDB.SetNewTypeName(self.__TransId(Identifier)) #error handle _PCATCH

        self.visitChildren(ctx)

        gCVfrVarDataTypeDB.DeclareDataTypeEnd()
        return None


    # Visit a parse tree produced by VfrSyntaxParser#vfrDataStructFields.
    def visitVfrDataStructFields(self, ctx:VfrSyntaxParser.VfrDataStructFieldsContext):

        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#dataStructField64.
    def visitDataStructField64(self, ctx:VfrSyntaxParser.DataStructField64Context):
        ArrayNum = self.__TransNum(ctx.Number())
        gCVfrVarDataTypeDB.DataTypeAddField(self.__TransId(ctx.StringIdentifier()), 'UINT64', ArrayNum, ctx.FieldInUnion) #error handle _PCATCH
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#dataStructField32.
    def visitDataStructField32(self, ctx:VfrSyntaxParser.DataStructField32Context):
        ArrayNum = self.__TransNum(ctx.Number())
        gCVfrVarDataTypeDB.DataTypeAddField(self.__TransId(ctx.StringIdentifier()), 'UINT32', ArrayNum, ctx.FieldInUnion)#error handle _PCATCH
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#dataStructField16.
    def visitDataStructField16(self, ctx:VfrSyntaxParser.DataStructField16Context):
        ArrayNum = self.__TransNum(ctx.Number())
        gCVfrVarDataTypeDB.DataTypeAddField(self.__TransId(ctx.StringIdentifier()), 'UINT16', ArrayNum, ctx.FieldInUnion)#error handle _PCATCH
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#dataStructField8.
    def visitDataStructField8(self, ctx:VfrSyntaxParser.DataStructField8Context):
        ArrayNum = self.__TransNum(ctx.Number())
        gCVfrVarDataTypeDB.DataTypeAddField(self.__TransId(ctx.StringIdentifier()), 'UINT8', ArrayNum, ctx.FieldInUnion)#error handle _PCATCH
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#dataStructFieldBool.
    def visitDataStructFieldBool(self, ctx:VfrSyntaxParser.DataStructFieldBoolContext):
        ArrayNum = self.__TransNum(ctx.Number())
        gCVfrVarDataTypeDB.DataTypeAddField(self.__TransId(ctx.StringIdentifier()), 'BOOLEAN', ArrayNum, ctx.FieldInUnion)#error handle _PCATCH
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#dataStructFieldString.
    def visitDataStructFieldString(self, ctx:VfrSyntaxParser.DataStructFieldStringContext):
        ArrayNum = self.__TransNum(ctx.Number())
        gCVfrVarDataTypeDB.DataTypeAddField(self.__TransId(ctx.StringIdentifier()), 'EFI_STRING_ID', ArrayNum, ctx.FieldInUnion)#error handle _PCATCH
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#dataStructFieldDate.
    def visitDataStructFieldDate(self, ctx:VfrSyntaxParser.DataStructFieldDateContext):
        ArrayNum = self.__TransNum(ctx.Number())
        gCVfrVarDataTypeDB.DataTypeAddField(self.__TransId(ctx.StringIdentifier()), 'EFI_HII_DATE', ArrayNum, ctx.FieldInUnion)#error handle _PCATCH
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#dataStructFieldTime.
    def visitDataStructFieldTime(self, ctx:VfrSyntaxParser.DataStructFieldTimeContext):
        ArrayNum = self.__TransNum(ctx.Number())
        gCVfrVarDataTypeDB.DataTypeAddField(self.__TransId(ctx.StringIdentifier()), 'EFI_HII_TIME', ArrayNum, ctx.FieldInUnion)#error handle _PCATCH
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#dataStructFieldRef.
    def visitDataStructFieldRef(self, ctx:VfrSyntaxParser.DataStructFieldRefContext):
        ArrayNum = self.__TransNum(ctx.Number())
        gCVfrVarDataTypeDB.DataTypeAddField(self.__TransId(ctx.StringIdentifier()), 'EFI_HII_REF', ArrayNum, ctx.FieldInUnion)#error handle _PCATCH
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#dataStructFieldUser.
    def visitDataStructFieldUser(self, ctx:VfrSyntaxParser.DataStructFieldUserContext):
        ArrayNum = self.__TransNum(ctx.Number())
        gCVfrVarDataTypeDB.DataTypeAddField(self.__TransId(ctx.StringIdentifier(1)), self.__TransId(ctx.StringIdentifier(0)), ArrayNum, ctx.FieldInUnion)#error handle _PCATCH
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#dataStructBitField64.
    def visitDataStructBitField64(self, ctx:VfrSyntaxParser.DataStructBitField64Context):
        Width = self.__TransNum(ctx.Number())
        gCVfrVarDataTypeDB.DataTypeAddBitField(self.__TransId(ctx.StringIdentifier()), 'UINT64', Width, ctx.FieldInUnion)#error handle _PCATCH
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#dataStructBitField32.
    def visitDataStructBitField32(self, ctx:VfrSyntaxParser.DataStructBitField32Context):
        Width = self.__TransNum(ctx.Number())
        gCVfrVarDataTypeDB.DataTypeAddBitField(self.__TransId(ctx.StringIdentifier()), 'UINT32', Width, ctx.FieldInUnion)#error handle _PCATCH
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#dataStructBitField16.
    def visitDataStructBitField16(self, ctx:VfrSyntaxParser.DataStructBitField16Context):
        Width = self.__TransNum(ctx.Number())
        gCVfrVarDataTypeDB.DataTypeAddBitField(self.__TransId(ctx.StringIdentifier()), 'UINT16', Width, ctx.FieldInUnion)#error handle _PCATCH
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#dataStructBitField8.
    def visitDataStructBitField8(self, ctx:VfrSyntaxParser.DataStructBitField8Context):
        Width = self.__TransNum(ctx.Number())
        gCVfrVarDataTypeDB.DataTypeAddBitField(self.__TransId(ctx.StringIdentifier()), 'UINT8', Width, ctx.FieldInUnion)#error handle _PCATCH
        return self.visitChildren(ctx)

    def __DeclareStandardDefaultStorage(self, LineNo):

        DSObj = CIfrDefaultStore()
        gCVfrDefaultStore.RegisterDefaultStore(DSObj.GetDefaultStore(), "Standard Defaults", EFI_STRING_ID_INVALID, EFI_HII_DEFAULT_CLASS_STANDARD)
        DSObj.SetLineNo (LineNo)
        DSObj.SetDefaultName (EFI_STRING_ID_INVALID)
        DSObj.SetDefaultId (EFI_HII_DEFAULT_CLASS_STANDARD)

        DSObjMF = CIfrDefaultStore()
        gCVfrDefaultStore.RegisterDefaultStore(DSObjMF.GetDefaultStore(), "Standard ManuFacturing", EFI_STRING_ID_INVALID, EFI_HII_DEFAULT_CLASS_MANUFACTURING)
        DSObjMF.SetLineNo (LineNo)
        DSObjMF.SetDefaultName (EFI_STRING_ID_INVALID)
        DSObjMF.SetDefaultId (EFI_HII_DEFAULT_CLASS_MANUFACTURING)


    # Visit a parse tree produced by VfrSyntaxParser#vfrFormSetDefinition.
    def visitVfrFormSetDefinition(self, ctx:VfrSyntaxParser.VfrFormSetDefinitionContext):
        self.InsertChild(self.__Root, ctx)
        self.visitChildren(ctx)

        ClassGuidNum = 0
        if ctx.classguidDefinition() != None:
            GuidList = ctx.classguidDefinition().GuidList
            ClassGuidNum = len(GuidList)

        Line = (None if ctx.start is None else ctx.start).line
        DefaultClassGuid = EFI_HII_PLATFORM_SETUP_FORMSET_GUID

        if (self.__OverrideClassGuid != None and ClassGuidNum >=4):
            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
            print('Already has 4 class guids, can not add extra class guid!')

        if ClassGuidNum == 0:
            if self.__OverrideClassGuid != None:
                ClassGuidNum  = 2
            else:
                ClassGuidNum  = 1
            FSObj = CIfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID))
            FSObj.SetClassGuid(DefaultClassGuid)
            if (self.__OverrideClassGuid != None):
                FSObj.SetClassGuid(self.__OverrideClassGuid)

        if ClassGuidNum == 1:
            if self.__OverrideClassGuid != None:
                ClassGuidNum  += 1
            FSObj = CIfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID))
            FSObj.SetClassGuid(GuidList[0])
            if (self.__OverrideClassGuid != None):
                FSObj.SetClassGuid(self.__OverrideClassGuid)

        if ClassGuidNum == 2:
            if self.__OverrideClassGuid != None:
                ClassGuidNum += 1
            FSObj = CIfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID))
            FSObj.SetClassGuid(GuidList[0])
            FSObj.SetClassGuid(GuidList[1])
            if (self.__OverrideClassGuid != None):
                FSObj.SetClassGuid(self.__OverrideClassGuid)

        if ClassGuidNum == 3:
            if self.__OverrideClassGuid != None:
                ClassGuidNum  += 1
            FSObj = CIfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID))
            FSObj.SetClassGuid(GuidList[0])
            FSObj.SetClassGuid(GuidList[1])
            FSObj.SetClassGuid(GuidList[2])
            if (self.__OverrideClassGuid != None):
                FSObj.SetClassGuid(self.__OverrideClassGuid)

        if ClassGuidNum == 4:
            if self.__OverrideClassGuid != None:
                ClassGuidNum  += 1
            FSObj = CIfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID))
            FSObj.SetClassGuid(GuidList[0])
            FSObj.SetClassGuid(GuidList[1])
            FSObj.SetClassGuid(GuidList[2])
            FSObj.SetClassGuid(GuidList[3])

        FSObj.SetLineNo(Line)
        FSObj.SetGuid(ctx.guidDefinition().Guid)
        FSObj.SetFormSetTitle(self.__TransNum(ctx.Number(0)))
        FSObj.SetHelp(self.__TransNum(ctx.Number(1)))

        ctx.Node.Data = FSObj
        self.InsertChild(ctx.Node, ctx.classDefinition())
        self.InsertChild(ctx.Node, ctx.subclassDefinition())
        self.__DeclareStandardDefaultStorage(Line)
        #############################

        '''
        print('** FormSet Info **')
        Info = ctx.Node.Data.GetInfo()
        print(ctx.guidDefinition().getText())
        # self.__ShowGuid(Info.Guid)
        print(Info.FormSetTitle)
        print(Info.Help)
        '''

        return ctx.Node

    def InsertChild(self, ParentNode: VfrTreeNode, ChildCtx):
        if ChildCtx != None and ChildCtx.Node != None:
            ParentNode.insertChild(ChildCtx.Node)

    def __ShowGuid(self, guid):
        print('data1:' + str(guid.Data1) + ' data2:' + str(guid.Data2) + ' data3:' + str(guid.Data3) + ' data4:' +
              str(guid.Data4[0]) + ' ' + str(guid.Data4[1]) + ' ' + str(guid.Data4[2]) +' ' + str(guid.Data4[3])+' ' + str(guid.Data4[4]) +' ' + str(guid.Data4[5]) +' ' + str(guid.Data4[6]) + ' ' + str(guid.Data4[7]))

    # Visit a parse tree produced by VfrSyntaxParser#classguidDefinition.
    def visitClassguidDefinition(self, ctx:VfrSyntaxParser.ClassguidDefinitionContext):

        self.visitChildren(ctx)
        
        for GuidCtx in ctx.guidDefinition():
            ctx.GuidList.append(GuidCtx.Guid)
            # Node = VfrTreeNode()
            # Node.Data = GuidCtx.Guid
            # ctx.Node.insertChild(Node)
        

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#classDefinition.
    def visitClassDefinition(self, ctx:VfrSyntaxParser.ClassDefinitionContext):
        CObj = CIfrClass()
        self.visitChildren(ctx)
        Class = 0
        for ClassNameCtx in ctx.validClassNames():
            Class |= ClassNameCtx.ClassName
        Line = (None if ctx.start is None else ctx.start).line
        CObj.SetLineNo(Line)
        CObj.SetClass(Class)
        ctx.Node.Data = CObj
        # ctx.Node.Parent = ctx.parentCtx.Node

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#validClassNames.
    def visitValidClassNames(self, ctx:VfrSyntaxParser.ValidClassNamesContext):

        self.visitChildren(ctx)

        if ctx.ClassNonDevice() != None:
            ctx.ClassName = EFI_NON_DEVICE_CLASS
        elif ctx.ClassDiskDevice() != None:
            ctx.ClassName = EFI_DISK_DEVICE_CLASS
        elif ctx.ClassVideoDevice() != None:
            ctx.ClassName = EFI_VIDEO_DEVICE_CLASS
        elif ctx.ClassNetworkDevice() != None:
            ctx.ClassName = EFI_NETWORK_DEVICE_CLASS
        elif ctx.ClassInputDevice() != None:
            ctx.ClassName = EFI_INPUT_DEVICE_CLASS
        elif ctx.ClassOnBoardDevice() != None:
            ctx.ClassName = EFI_ON_BOARD_DEVICE_CLASS
        elif ctx.ClassOtherDevice() != None:
            ctx.ClassName = EFI_OTHER_DEVICE_CLASS
        else:
            ctx.ClassName = self.__TransNum(ctx.Number())

        return ctx.ClassName


    # Visit a parse tree produced by VfrSyntaxParser#subclassDefinition.
    def visitSubclassDefinition(self, ctx:VfrSyntaxParser.SubclassDefinitionContext):
        SubObj = CIfrSubClass()

        self.visitChildren(ctx)

        Line = (None if ctx.start is None else ctx.start).line
        SubObj.SetLineNo(Line)
        SubClass = 0
        if ctx.SubclassSetupApplication() != None:
            SubClass |= EFI_SETUP_APPLICATION_SUBCLASS
        elif ctx.SubclassGeneralApplication() != None:
            SubClass |= EFI_GENERAL_APPLICATION_SUBCLASS
        elif ctx.SubclassFrontPage() != None:
            SubClass |= EFI_FRONT_PAGE_SUBCLASS
        elif ctx.SubclassSingleUse() != None:
            SubClass |= EFI_SINGLE_USE_SUBCLASS
        else:
            SubClass = self.__TransNum(ctx.Number())

        SubObj.SetSubClass(SubClass)
        ctx.Node.Data = SubObj
        # ctx.Node.Parent = ctx.parentCtx.Node

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrFormSetList.
    def visitVfrFormSetList(self, ctx:VfrSyntaxParser.VfrFormSetListContext):
        self.visitChildren(ctx)
        for Ctx in ctx.vfrFormSet():
            self.InsertChild(ctx.Node, Ctx)
        
        return ctx.Node

    # Visit a parse tree produced by VfrSyntaxParser#vfrFormSet.
    def visitVfrFormSet(self, ctx:VfrSyntaxParser.VfrFormSetContext):
        self.visitChildren(ctx)

        if ctx.vfrFormDefinition() != None:
            ctx.Node = ctx.vfrFormDefinition().Node
        if ctx.vfrFormMapDefinition() != None:
            ctx.Node = ctx.vfrFormMapDefinition().Node
        if ctx.vfrStatementImage() != None:
            ctx.Node = ctx.vfrStatementImage().Node
        if ctx.vfrStatementVarStoreLinear() != None:
            ctx.Node = ctx.vfrStatementVarStoreLinear().Node
        if ctx.vfrStatementVarStoreEfi() != None:
            ctx.Node = ctx.vfrStatementVarStoreEfi().Node
        if ctx.vfrStatementVarStoreNameValue() != None:
            ctx.Node = ctx.vfrStatementVarStoreNameValue().Node
        if ctx.vfrStatementDefaultStore() != None:
            ctx.Node = ctx.vfrStatementDefaultStore().Node
        if ctx.vfrStatementDisableIfFormSet() != None:
            ctx.Node = ctx.vfrStatementDisableIfFormSet().Node
        if ctx.vfrStatementSuppressIfFormSet() != None:
            ctx.Node = ctx.vfrStatementSuppressIfFormSet().Node
        if ctx.vfrStatementExtension() != None:
            ctx.Node = ctx.vfrStatementExtension().Node

        # ctx.Node.Parent = ctx.parentCtx.Node

        return ctx.Node

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementDefaultStore.
    def visitVfrStatementDefaultStore(self, ctx:VfrSyntaxParser.VfrStatementDefaultStoreContext):
        DSObj = CIfrDefaultStore()

        self.visitChildren(ctx)

        RefName = self.__TransId(ctx.StringIdentifier())
        DefaultStoreNameId = self.__TransNum(ctx.Number(0))

        DefaultId = EFI_HII_DEFAULT_CLASS_STANDARD if ctx.Attribute()== None else self.__TransNum(ctx.Number(1))

        if gCVfrDefaultStore.DefaultIdRegistered(DefaultId) == False:
            ReturnCode = gCVfrDefaultStore.RegisterDefaultStore(DSObj.GetDefaultStore(), RefName, DefaultStoreNameId, DefaultId) #
            DSObj.SetDefaultName(DefaultStoreNameId)
            DSObj.SetDefaultId (DefaultId)
            DSObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        else:
            pNode, ReturnCode = gCVfrDefaultStore.ReRegisterDefaultStoreById(DefaultId, RefName, DefaultStoreNameId)
            DSObj.SetDefaultStore = pNode.ObjAddr
            DSObj.SetLineNo((None if ctx.start is None else ctx.start).line)

        ctx.Node.Data = DSObj

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementVarStoreLinear.
    def visitVfrStatementVarStoreLinear(self, ctx:VfrSyntaxParser.VfrStatementVarStoreLinearContext):
        VSObj = CIfrVarStore()

        self.visitChildren(ctx)

        VSObj.SetLineNo((None if ctx.start is None else ctx.start).line)

        TypeName = str(ctx.getChild(1))
        if TypeName == 'CHAR16':
            TypeName = 'UINT16'

        IsBitVarStore = False
        if len(ctx.StringIdentifier()) == 2: # TypeName = StringIdentifier
            IsBitVarStore = gCVfrVarDataTypeDB.DataTypeHasBitField(self.__TransId(ctx.StringIdentifier(0)))

        # VarId _PCATCH
        VarStoreId = self.__TransNum(ctx.Number(), EFI_VARSTORE_ID_INVALID)
        StoreName = self.__TransId(ctx.StringIdentifier(0)) if ctx.StringIdentifier(1) == None else self.__TransId(ctx.StringIdentifier(1))
        Guid = ctx.guidDefinition().Guid
        ReturnCode = gCVfrDataStorage.DeclareBufferVarStore(StoreName, Guid, gCVfrVarDataTypeDB, TypeName, VarStoreId, IsBitVarStore)#
        VSObj.SetGuid(Guid)
        VarStoreId, ReturnCode = gCVfrDataStorage.GetVarStoreId(StoreName, Guid) # VarId _PCATCH
        VSObj.SetVarStoreId(VarStoreId)
        Size, ReturnCode = gCVfrVarDataTypeDB.GetDataTypeSizeByTypeName(TypeName)
        VSObj.SetSize(Size)
        VSObj.SetName(StoreName)

        ctx.Node.Data = VSObj
        '''
        
        print('** Varstore info **')
        Info = ctx.Node.Data.GetInfo()
        print(Info.VarStoreId)
        print(Info.Name)
        print(Info.Size)
        print(ctx.guidDefinition().getText())
        '''


        return VSObj



    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementVarStoreEfi.
    def visitVfrStatementVarStoreEfi(self, ctx:VfrSyntaxParser.VfrStatementVarStoreEfiContext):

        VSEObj = CIfrVarStoreEfi()
        self.visitChildren(ctx)

        VSEObj.SetLineNo((None if ctx.start is None else ctx.start).line)

        Guid = ctx.guidDefinition().Guid

        CustomizedName = False
        IsBitVarStore = False
        VarStoreId = EFI_VARSTORE_ID_INVALID
        IsUEFI23EfiVarstore = True
        ReturnCode = None

        TypeName = str(ctx.getChild(1))

        if TypeName == 'CHAR16':
            TypeName = 'UINT16'

        elif TypeName == self.__TransId(ctx.StringIdentifier(0)):
            CustomizedName = True
            IsBitVarStore = gCVfrVarDataTypeDB.DataTypeHasBitField(TypeName)

        if ctx.VarId() != None:
            VarStoreId = self.__TransNum(ctx.Number(0))

        Attributes = 0
        for AtrCtx in ctx.vfrVarStoreEfiAttr():
            Attributes |= AtrCtx.Attr
        VSEObj.SetAttributes(Attributes)

        if (CustomizedName and ctx.StringIdentifier(1) != None):
            StoreName = self.__TransId(ctx.StringIdentifier(1))
        elif not(CustomizedName) and ctx.StringIdentifier(0) != None:
            StoreName = self.__TransId(ctx.StringIdentifier(0))
        else:
            IsUEFI23EfiVarstore = False
            NameStringId = self.__TransNum(ctx.Number(len(ctx.Number())-2))
            StoreName = gCVfrStringDB.GetVarStoreNameFromStringId(NameStringId)
            if StoreName == None:
                ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
                print('Cant get varstore name for this StringId!')
            if not(CustomizedName):
                ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
                print('Old style efivarstore must have String Identifier!')
                return
            Size = self.__TransNum(ctx.Number(len(ctx.Number())-1))
            if Size == 1: TypeName = 'UINT8'
            elif Size == 2: TypeName = 'UINT16'
            elif Size == 4: TypeName = 'UINT32'
            elif Size == 8: TypeName = 'UINT64'
            else: ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED

        if IsUEFI23EfiVarstore:
            gCVfrDataStorage.DeclareBufferVarStore(StoreName, Guid, gCVfrVarDataTypeDB, TypeName, VarStoreId, IsBitVarStore) #
            VarStoreId, ReturnCode = gCVfrDataStorage.GetVarStoreId(StoreName, Guid) #
            Size, ReturnCode = gCVfrVarDataTypeDB.GetDataTypeSizeByTypeName(TypeName)
        else:
            gCVfrDataStorage.DeclareBufferVarStore(self.__TransId(ctx.StringIdentifier(0)), Guid, gCVfrVarDataTypeDB, TypeName, VarStoreId, IsBitVarStore) #
            VarStoreId, ReturnCode = gCVfrDataStorage.GetVarStoreId(self.__TransId(ctx.StringIdentifier(0)), Guid) #
            Size, ReturnCode = gCVfrVarDataTypeDB.GetDataTypeSizeByTypeName(TypeName)

        VSEObj.SetGuid(Guid)
        VSEObj.SetVarStoreId (VarStoreId)
        VSEObj.SetSize(Size) #
        VSEObj.SetName(StoreName)

        ctx.Node.Data = VSEObj
        '''
        
        print('** efiVarstore info **')
        Info = ctx.Node.Data.GetInfo()
        print(Info.VarStoreId)
        print(Info.Name)
        print(Info.Size)
        print(ctx.guidDefinition().getText())
        '''

        return ctx.Node

    # Visit a parse tree produced by VfrSyntaxParser#vfrVarStoreEfiAttr.
    def visitVfrVarStoreEfiAttr(self, ctx:VfrSyntaxParser.VfrVarStoreEfiAttrContext):

        self.visitChildren(ctx)

        ctx.Attr = self.__TransNum(ctx.Number())

        return ctx.Attr

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementVarStoreNameValue.
    def visitVfrStatementVarStoreNameValue(self, ctx:VfrSyntaxParser.VfrStatementVarStoreNameValueContext):

        VSNVObj = CIfrVarStoreNameValue()
        self.visitChildren(ctx)

        Guid = ctx.guidDefinition().Guid
        HasVarStoreId = False
        VarStoreId = EFI_VARSTORE_ID_INVALID

        if ctx.VarId() != None:
            HasVarStoreId = True
            VarStoreId = self.__TransNum(ctx.Number(0))


        StoreName = self.__TransId(ctx.StringIdentifier())
        Created = False

        sIndex = 0 if  HasVarStoreId == False else 1
        eIndex = len(ctx.Name())
        for i in range(sIndex, eIndex):
            if Created == False:
                ReturnCode = gCVfrDataStorage.DeclareNameVarStoreBegin(StoreName, VarStoreId)
                Created = True
            Item = self.__TransNum(ctx.Number(i))
            gCVfrDataStorage.NameTableAddItem(Item)

        gCVfrDataStorage.DeclareNameVarStoreEnd(Guid)

        VSNVObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        VSNVObj.SetGuid(Guid)
        VarstoreId, ReturnCode = gCVfrDataStorage.GetVarStoreId(StoreName, Guid)
        VSNVObj.SetVarStoreId(VarstoreId)

        ctx.Node.Data = VSNVObj
        '''
        
        print('** namevaluevarstore info **')
        Info = ctx.Node.Data.GetInfo()
        print(Info.VarStoreId)
        print(ctx.guidDefinition().getText())
        '''

        return ctx.Node

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementDisableIfFormSet.
    def visitVfrStatementDisableIfFormSet(self, ctx:VfrSyntaxParser.VfrStatementDisableIfFormSetContext):

        DIObj = CIfrDisableIf()
        DIObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        self.__ConstantOnlyInExpression = False # position 
        ctx.Node.Data = DIObj
        Condition = 'disableif' + ' ' + self.__ExtractOriginalText(ctx.vfrStatementExpression())
        ctx.Node.Condition = Condition
        
        self.visitChildren(ctx)

        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)


        return ctx.Node

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementSuppressIfFormSet.
    def visitVfrStatementSuppressIfFormSet(self, ctx:VfrSyntaxParser.VfrStatementSuppressIfFormSetContext):

        SIObj = CIfrSuppressIf()
        SIObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        ctx.Node.Data = SIObj
        Condition = 'suppressif' + ' ' +  self.__ExtractOriginalText(ctx.vfrStatementExpression())
        ctx.Node.Condition = Condition
        self.visitChildren(ctx)


        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)


        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#guidSubDefinition.
    def visitGuidSubDefinition(self, ctx:VfrSyntaxParser.GuidSubDefinitionContext):

        ctx.Guid.Data4[0] = self.__TransNum(ctx.Number(0))
        ctx.Guid.Data4[1] = self.__TransNum(ctx.Number(1))
        ctx.Guid.Data4[2] = self.__TransNum(ctx.Number(2))
        ctx.Guid.Data4[3] = self.__TransNum(ctx.Number(3))
        ctx.Guid.Data4[4] = self.__TransNum(ctx.Number(4))
        ctx.Guid.Data4[5] = self.__TransNum(ctx.Number(5))
        ctx.Guid.Data4[6] = self.__TransNum(ctx.Number(6))
        ctx.Guid.Data4[7] = self.__TransNum(ctx.Number(7))

        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#guidDefinition.
    def visitGuidDefinition(self, ctx:VfrSyntaxParser.GuidDefinitionContext):

        self.visitChildren(ctx)

        ctx.Guid.Data1 = self.__TransNum(ctx.Number(0))
        ctx.Guid.Data2 = self.__TransNum(ctx.Number(1))
        ctx.Guid.Data3 = self.__TransNum(ctx.Number(2))

        return None


    # Visit a parse tree produced by VfrSyntaxParser#getStringId.
    def visitGetStringId(self, ctx:VfrSyntaxParser.GetStringIdContext):

        ctx.StringId = self.__TransNum(ctx.Number())

        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementHeader.
    def visitVfrStatementHeader(self, ctx:VfrSyntaxParser.VfrStatementHeaderContext):

        ctx.OpObj = ctx.parentCtx.OpObj
        if ctx.OpObj != None:
            Prompt = self.__TransNum(ctx.Number(0))
            ctx.OpObj.SetPrompt(Prompt)
            Help = self.__TransNum(ctx.Number(1))
            ctx.OpObj.SetHelp(Help)

        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrQuestionHeader.
    def visitVfrQuestionHeader(self, ctx:VfrSyntaxParser.VfrQuestionHeaderContext):

        ctx.OpObj = ctx.parentCtx.OpObj

        return  self.visitChildren(ctx)

    # Visit a parse tree produced by VfrSyntaxParser#vfrQuestionBaseInfo.
    def visitVfrQuestionBaseInfo(self, ctx:VfrSyntaxParser.VfrQuestionBaseInfoContext):

        ctx.OpObj = ctx.parentCtx.OpObj

        ctx.BaseInfo.VarType = EFI_IFR_TYPE_OTHER
        ctx.BaseInfo.VarTotalSize = 0
        ctx.BaseInfo.Info.VarOffset = EFI_VAROFFSET_INVALID
        ctx.BaseInfo.VarStoreId = EFI_VARSTORE_ID_INVALID
        ctx.BaseInfo.IsBitVar = False

        QName = None
        QId = EFI_QUESTION_ID_INVALID
        ReturnCode = None

        self.visitChildren(ctx)

        if ctx.Name() != None:
            QName = self.__TransId(ctx.StringIdentifier())
            ReturnCode = self.__CVfrQuestionDB.FindQuestionByName(QName)
            if ReturnCode != VfrReturnCode.VFR_RETURN_UNDEFINED:
                print("has already been used please used anther name") ########


        VarIdStr = '' if ctx.VarId() == None else  ctx.vfrStorageVarId().VarIdStr
        # if self.__IsNumericOp:
        #   print(VarIdStr)

        if ctx.QuestionId() != None:
            QId = self.__TransNum(ctx.Number())
            ReturnCode = self.__CVfrQuestionDB.FindQuestionById(QId)
            if ReturnCode != VfrReturnCode.VFR_RETURN_UNDEFINED:
                print("has already been used please used anther number") ########


        if ctx.QType == EFI_QUESION_TYPE.QUESTION_NORMAL:
            if self.__IsCheckBoxOp:
                ctx.BaseInfo.VarType = EFI_IFR_TYPE_BOOLEAN
            QId, ReturnCode = self.__CVfrQuestionDB.RegisterQuestion(QName, VarIdStr, QId)
            if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                print(ReturnCode)

        elif ctx.QType == EFI_QUESION_TYPE.QUESTION_DATE:
            ctx.BaseInfo.VarType = EFI_IFR_TYPE_DATE
            QId, ReturnCode = self.__CVfrQuestionDB.RegisterNewDateQuestion(QName, VarIdStr, QId)
            if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                print(ReturnCode)

        elif ctx.QType == EFI_QUESION_TYPE.QUESTION_TIME:
            ctx.BaseInfo.VarType = EFI_IFR_TYPE_TIME
            QId, ReturnCode = self.__CVfrQuestionDB.RegisterNewTimeQuestion(QName, VarIdStr, QId)
            if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                print(ReturnCode)

        elif ctx.QType == EFI_QUESION_TYPE.QUESTION_REF:
            ctx.BaseInfo.VarType = EFI_IFR_TYPE_REF
            if VarIdStr != '': #stand for question with storage.
                QId, ReturnCode = self.__CVfrQuestionDB.RegisterRefQuestion(QName, VarIdStr, QId)
                if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                    print(ReturnCode)
            else:
                QId, ReturnCode = self.__CVfrQuestionDB.RegisterQuestion(QName, None, QId)
                if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                    print(ReturnCode)
        else:
            ReturnCode = VfrReturnCode.VFR_RETURN_FATAL_ERROR
            print("fatal error and  not support")

        self.__CurrQestVarInfo = ctx.BaseInfo

        if ctx.OpObj != None:
            if ReturnCode == VfrReturnCode.VFR_RETURN_SUCCESS:
                ctx.OpObj.SetQuestionId(QId)
            if ctx.BaseInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
                ctx.OpObj.SetVarStoreInfo(ctx.BaseInfo)

        return ctx.OpObj


    # Visit a parse tree produced by VfrSyntaxParser#questionheaderFlagsField.
    def visitQuestionheaderFlagsField(self, ctx:VfrSyntaxParser.QuestionheaderFlagsFieldContext):

        if ctx.ReadOnlyFlag() != None:
            ctx.QHFlag = 0x01

        elif ctx.InteractiveFlag() != None:
            ctx.QHFlag = 0x04

        elif ctx.ResetRequiredFlag() != None:
            ctx.QHFlag = 0x10

        elif ctx.RestStyleFlag() != None:
            ctx.QHFlag = 0x20

        elif ctx.ReconnectRequiredFlag() != None:
            ctx.QHFlag = 0x40
        else:
            pass #error handler

        return self.visitChildren(ctx)

    # Visit a parse tree produced by VfrSyntaxParser#vfrStorageVarIdRule1.
    def visitVfrStorageVarIdRule1(self, ctx:VfrSyntaxParser.VfrStorageVarIdRule1Context):

        self.visitChildren(ctx)

        SName = self.__TransId(ctx.StringIdentifier())
        ctx.VarIdStr += SName

        Idx = self.__TransNum(ctx.Number())
        ctx.VarIdStr += '['
        ctx.VarIdStr += str(Idx)
        ctx.VarIdStr += ']'

        ctx.BaseInfo.VarStoreId, ReturnCode = gCVfrDataStorage.GetVarStoreId(SName)

        if ctx.CheckFlag or ReturnCode == VfrReturnCode.VFR_RETURN_SUCCESS:
            ReturnCode = gCVfrDataStorage.GetNameVarStoreInfo(ctx.BaseInfo, Idx) #

        return ctx.VarIdStr


    # Visit a parse tree produced by VfrSyntaxParser#vfrStorageVarIdRule2.
    def visitVfrStorageVarIdRule2(self, ctx:VfrSyntaxParser.VfrStorageVarIdRule2Context):

        self.visitChildren(ctx)

        VarStr = '' # type.field
        SName = self.__TransId(ctx.StringIdentifier())
        ctx.VarIdStr += SName
        ctx.BaseInfo.VarStoreId, ReturnCode = gCVfrDataStorage.GetVarStoreId(SName)

        if ctx.CheckFlag or ReturnCode == VfrReturnCode.VFR_RETURN_SUCCESS:
            VarStoreType = gCVfrDataStorage.GetVarStoreType(ctx.BaseInfo.VarStoreId)
            if VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER or VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER_BITS:
                TName, ReturnCode2 = gCVfrDataStorage.GetBufferVarStoreDataTypeName(ctx.BaseInfo.VarStoreId)

                if ReturnCode2 == VfrReturnCode.VFR_RETURN_SUCCESS:
                    VarStr += TName

        Count = len(ctx.Dot())
        for i in range(0, Count):
            if ctx.CheckFlag or ReturnCode == VfrReturnCode.VFR_RETURN_SUCCESS:
                cond = (VarStoreType != EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER) and (VarStoreType != EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER_BITS)
                ReturnCode = VfrReturnCode.VFR_RETURN_EFIVARSTORE_USE_ERROR if cond else VfrReturnCode.VFR_RETURN_SUCCESS

            ctx.VarIdStr += '.'
            VarStr += '.'
            ctx.VarIdStr += ctx.arrayName(i).SubStr
            VarStr += ctx.arrayName(i).SubStr
            ''' 
            ctx.VarIdStr += self.__TransId(ctx.StringIdentifier(i+1))
            VarStr += self.__TransId(ctx.StringIdentifier(i+1))
            if ctx.Number(i) != None:
                Idx = self.__TransNum(ctx.Number(i))
                if Idx > 0:
                    ctx.VarIdStr += '['
                    ctx.VarIdStr += str(Idx)
                    ctx.VarIdStr += ']'

                VarStr += '['
                VarStr += str(Idx)
                VarStr += ']'
            '''

        if VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_EFI:
            ReturnCode = gCVfrDataStorage.GetEfiVarStoreInfo(ctx.BaseInfo)

        elif VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER or VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER_BITS:
            ctx.BaseInfo.Info.VarOffset, ctx.BaseInfo.VarType, ctx.BaseInfo.VarTotalSize, ctx.BaseInfo.IsBitVar, ReturnCode = gCVfrVarDataTypeDB.GetDataFieldInfo(VarStr)

            VarGuid = gCVfrDataStorage.GetVarStoreGuid(ctx.BaseInfo.VarStoreId)

            ReturnCode = gCVfrBufferConfig.Register(SName, VarGuid)
            Dummy = self.gZeroEfiIfrTypeValue
            gCVfrBufferConfig.Write(
                'a',
                SName,
                VarGuid,
                None,
                ctx.BaseInfo.VarType,
                ctx.BaseInfo.Info.VarOffset,
                ctx.BaseInfo.VarTotalSize,
                Dummy) #　the definition of dummy is needed to check

            gCVfrDataStorage.AddBufferVarStoreFieldInfo(ctx.BaseInfo)

        return ctx.VarIdStr

    # Visit a parse tree produced by VfrSyntaxParser#vfrConstantValueField.
    def visitVfrConstantValueField(self, ctx:VfrSyntaxParser.VfrConstantValueFieldContext):
        self.visitChildren(ctx)

        IntDecStyle = False
        if self.__CurrentMinMaxData != None and self.__CurrentMinMaxData.IsNumericOpcode():
            NumericQst = CIfrNumeric(self.__CurrentQuestion) #
            IntDecStyle = True if (NumericQst.GetNumericFlags() & EFI_IFR_DISPLAY) == 0 else False #

        if ctx.TrueSymbol() != None:
            ctx.ValueStr = 'TRUE'
            ctx.Value.b = True
        elif ctx.FalseSymbol() != None:
            ctx.ValueStr = 'FALSE'
            ctx.Value.b = False
        elif ctx.One() != None:
            ctx.ValueStr = 'ONE'
            ctx.Value.u8 = int(ctx.getText()) #
        elif ctx.Ones() != None:
            ctx.ValueStr = 'ONES'
            ctx.Value.u64 = int(ctx.getText()) #
        elif ctx.Zero() != None:
            ctx.ValueStr = 'ZERO'
            ctx.Value.u8 = int(ctx.getText()) #
        elif ctx.Colon() != []:
            ctx.ValueStr += str(ctx.Number(0))
            ctx.ValueStr += ':'
            ctx.ValueStr += str(ctx.Number(1))
            ctx.ValueStr += ':'
            ctx.ValueStr += str(ctx.Number(2))
            ctx.Value.time.Hour = self.__TransNum(ctx.Number(0))
            ctx.Value.time.Minute = self.__TransNum(ctx.Number(1))
            ctx.Value.time.Second = self.__TransNum(ctx.Number(2))
        elif ctx.Slash() != []:
            ctx.ValueStr += str(ctx.Number(0))
            ctx.ValueStr += '/'
            ctx.ValueStr += str(ctx.Number(1))
            ctx.ValueStr += '/'
            ctx.ValueStr += str(ctx.Number(2))
            ctx.Value.date.Year = self.__TransNum(ctx.Number(0))
            ctx.Value.date.Month = self.__TransNum(ctx.Number(1))
            ctx.Value.date.Day = self.__TransNum(ctx.Number(2))
        elif ctx.Semicolon() != []:
            ctx.Value.ref.QuestionId = self.__TransNum(ctx.Number(0))
            ctx.Value.ref.FormId = self.__TransNum(ctx.Number(1))
            ctx.Value.ref.DevicePath = self.__TransNum(ctx.Number(2))
            ctx.Value.ref.FormSetGuid = ctx.guidDefinition().Guid
        elif ctx.StringToken() != None:
            ctx.Value.string = self.__TransNum(ctx.Number(0))
        elif ctx.OpenBrace() != None:
            ctx.ListType = True
            ctx.Value = []
            TempValue = EFI_IFR_TYPE_VALUE()
            Type = self.__CurrQestVarInfo.VarType
            ctx.ValueStr += '{'
            ctx.ValueStr += str(ctx.Number(0))
            for i in range(1, len(ctx.Number())):
                ctx.ValueStr += ','
                ctx.ValueStr += str(ctx.Number(i))
                if Type == EFI_IFR_TYPE_NUM_SIZE_8:
                    TempValue.u8 = self.__TransNum(ctx.Number(i))
                    ctx.Value.append(TempValue)

                if Type == EFI_IFR_TYPE_NUM_SIZE_16:
                    TempValue.u16 = self.__TransNum(ctx.Number(i))
                    ctx.Value.append(TempValue)

                if Type == EFI_IFR_TYPE_NUM_SIZE_32:
                    TempValue.u32 = self.__TransNum(ctx.Number(i))
                    ctx.Value.append(TempValue)

                if Type == EFI_IFR_TYPE_NUM_SIZE_64:
                    TempValue.u64 = self.__TransNum(ctx.Number(i))
                    ctx.Value.append(TempValue)
            ctx.ValueStr += '}'
        else:
            Negative = True if ctx.Negative() != None else False
            # The value stored in bit fields is always set to UINT32 type.
            if self.__CurrQestVarInfo.IsBitVar:
                ctx.Value.u32 = self.__TransNum(ctx.Number(0))
            else:
                Type = self.__CurrQestVarInfo.VarType
                if Type == EFI_IFR_TYPE_NUM_SIZE_8:
                    ctx.Value.u8 = self.__TransNum(ctx.Number(0))
                    if IntDecStyle:
                        if Negative:
                            if  ctx.Value.u8 > 0x80:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print("INT8 type can't big than 0x7F, small than -0x80")
                        else:
                            if ctx.Value.u8 > 0x7F:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print("INT8 type can't big than 0x7F, small than -0x80")
                    if Negative:
                        ctx.Value.u8 = ~ctx.Value.u8 + 1

                if Type == EFI_IFR_TYPE_NUM_SIZE_16:
                    ctx.Value.u16 = self.__TransNum(ctx.Number(0))
                    if IntDecStyle:
                        if Negative:
                            if  ctx.Value.u16 > 0x8000:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print("INT16 type can't big than 0x7FFF, small than -0x8000")
                        else:
                            if ctx.Value.u16 > 0x7FFF:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print("INT16 type can't big than 0x7FFF, small than -0x8000")
                    if Negative:
                        ctx.Value.u16 = ~ctx.Value.u16 + 1

                if Type == EFI_IFR_TYPE_NUM_SIZE_32:
                    ctx.Value.u32 = self.__TransNum(ctx.Number(0))
                    if IntDecStyle:
                        if Negative:
                            if  ctx.Value.u32 > 0x80000000:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print("INT32 type can't big than 0x7FFFFFFF, small than -0x80000000")
                        else:
                            if ctx.Value.u32 > 0X7FFFFFFF:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print("INT32 type can't big than 0x7FFFFFFF, small than -0x80000000")
                    if Negative:
                        ctx.Value.u32 = ~ctx.Value.u32 + 1

                if Type == EFI_IFR_TYPE_NUM_SIZE_64:
                    ctx.Value.u64 = self.__TransNum(ctx.Number(0))
                    if IntDecStyle:
                        if Negative:
                            if  ctx.Value.u64 > 0x8000000000000000:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print("INT64 type can't big than 0x7FFFFFFFFFFFFFFF, small than -0x8000000000000000")
                        else:
                            if ctx.Value.u64 > 0x7FFFFFFFFFFFFFFF:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print("INT64 type can't big than 0x7FFFFFFFFFFFFFFF, small than -0x8000000000000000")
                    if Negative:
                        ctx.Value.u64 = ~ctx.Value.u64 + 1

                if Type == EFI_IFR_TYPE_BOOLEAN:
                    ctx.Value.b = self.__TransNum(ctx.Number(0))

                if Type == EFI_IFR_TYPE_BOOLEAN:
                    ctx.Value.string = self.__TransNum(ctx.Number(0))

        return ctx.Value


    # Visit a parse tree produced by VfrSyntaxParser#vfrImageTag.
    def visitVfrImageTag(self, ctx:VfrSyntaxParser.VfrImageTagContext):
        IObj = CIfrImage()
        self.visitChildren(ctx)

        IObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        IObj.SetImageId(self.__TransNum(ctx.Number()))
        ctx.Node.Data = IObj

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrLockedTag.
    def visitVfrLockedTag(self, ctx:VfrSyntaxParser.VfrLockedTagContext):

        LObj=CIfrLocked()
        self.visitChildren(ctx)
        LObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        ctx.Node.Data = LObj

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementStatTag.
    def visitVfrStatementStatTag(self, ctx:VfrSyntaxParser.VfrStatementStatTagContext):
        self.visitChildren(ctx)
        if ctx.vfrImageTag() != None:
            ctx.Node = ctx.vfrImageTag().Node
        else:
            ctx.Node = ctx.vfrLockedTag().Node

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementStatTagList.
    def visitVfrStatementStatTagList(self, ctx:VfrSyntaxParser.VfrStatementStatTagListContext):
        self.visitChildren(ctx)
        for Ctx in ctx.vfrStatementStatTag():
            self.InsertChild(ctx.Node, Ctx)
        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrFormDefinition.
    def visitVfrFormDefinition(self, ctx:VfrSyntaxParser.VfrFormDefinitionContext):

        FObj = CIfrForm()
        self.visitChildren(ctx)

        FObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        FormId = self.__TransNum(ctx.Number(0))
        FObj.SetFormId(FormId)
        FormTitle  = self.__TransNum(ctx.Number(1))
        FObj.SetFormTitle(FormTitle)

        ctx.Node.Data = FObj
        for Ctx in ctx.vfrForm():
            self.InsertChild(ctx.Node, Ctx)

        '''
        
        print('** form info **')
        Info = ctx.Node.Data.GetInfo()
        print(Info.FormId)
        print(Info.FormTitle)
        '''

        return ctx.Node

    # Visit a parse tree produced by VfrSyntaxParser#vfrForm.
    def visitVfrForm(self, ctx:VfrSyntaxParser.VfrFormContext):
        self.visitChildren(ctx)
        if ctx.vfrStatementImage() != None:
            ctx.Node = ctx.vfrStatementImage().Node
        if ctx.vfrStatementLocked() != None:
            ctx.Node = ctx.vfrStatementLocked().Node
        if ctx.vfrStatementRules() != None:
            ctx.Node = ctx.vfrStatementRules().Node
        if ctx.vfrStatementDefault() != None:
            ctx.Node = ctx.vfrStatementDefault().Node
        if ctx.vfrStatementStat() != None:
            ctx.Node = ctx.vfrStatementStat().Node
        if ctx.vfrStatementQuestions() != None:
            ctx.Node = ctx.vfrStatementQuestions().Node
        if ctx.vfrStatementConditional() != None:
            ctx.Node = ctx.vfrStatementConditional().Node
        if ctx.vfrStatementLabel() != None:
            ctx.Node = ctx.vfrStatementLabel().Node
        if ctx.vfrStatementBanner() != None:
            ctx.Node = ctx.vfrStatementBanner().Node
        if ctx.vfrStatementInvalid() != None:
            ctx.Node = ctx.vfrStatementInvalid().Node
        if ctx.vfrStatementExtension() != None:
            ctx.Node = ctx.vfrStatementExtension().Node
        if ctx.vfrStatementModal() != None:
            ctx.Node = ctx.vfrStatementModal().Node
        if ctx.vfrStatementRefreshEvent() != None:
            ctx.Node = ctx.vfrStatementRefreshEvent().Node

        # ctx.Node.Parent = ctx.parentCtx.Node
        return ctx.Node



    # Visit a parse tree produced by VfrSyntaxParser#vfrFormMapDefinition.
    def visitVfrFormMapDefinition(self, ctx:VfrSyntaxParser.VfrFormMapDefinitionContext):

        FMapObj = CIfrFormMap()
        self.visitChildren(ctx)
        FMapObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        FMapObj.SetFormId(self.__TransNum(ctx.Number(0)))
        FormMapMethodNumber = len(ctx.MapTitle())
        if FormMapMethodNumber == 0:
            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
            print('No MapMethod is set for FormMap!')
        else:
            for i in range(0, FormMapMethodNumber):
                FMapObj.SetFormMapMethod(self.__TransNum(ctx.Number(i+1)), ctx.guidDefinition(i).Guid)
        ctx.Node.Data = FMapObj
        for Ctx in ctx.vfrForm():
            self.InsertChild(ctx.Node, Ctx)

        return ctx.Node



    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementImage.
    def visitVfrStatementImage(self, ctx:VfrSyntaxParser.VfrStatementImageContext):
        self.visitChildren(ctx)
        ctx.Node = ctx.vfrImageTag().Node
        return ctx.Node



    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementLocked.
    def visitVfrStatementLocked(self, ctx:VfrSyntaxParser.VfrStatementLockedContext):
        self.visitChildren(ctx)
        ctx.Node = ctx.vfrLockedTag().Node
        return ctx.Node

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementRules.
    def visitVfrStatementRules(self, ctx:VfrSyntaxParser.VfrStatementRulesContext):

        RObj=CIfrRule()
        self.visitChildren(ctx)

        RObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        RuleName = self.__TransId(ctx.StringIdentifier(0))
        self.__CVfrRulesDB.RegisterRule(RuleName)
        RObj.SetRuleId(self.__CVfrRulesDB.GetRuleId(RuleName))
        ctx.Node.Data = RObj
        ctx.Node.Expression = self.__ExtractOriginalText(ctx.vfrStatementExpression())

        # expression
        # end rule
        return ctx.Node

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementStat.
    def visitVfrStatementStat(self, ctx:VfrSyntaxParser.VfrStatementStatContext):

        self.visitChildren(ctx)
        if ctx.vfrStatementSubTitle() != None:
            ctx.Node = ctx.vfrStatementSubTitle().Node
        if ctx.vfrStatementStaticText() != None:
            ctx.Node = ctx.vfrStatementStaticText().Node
        if ctx.vfrStatementCrossReference() != None:
            ctx.Node = ctx.vfrStatementCrossReference().Node
        return ctx.Node



    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementSubTitle.
    def visitVfrStatementSubTitle(self, ctx:VfrSyntaxParser.VfrStatementSubTitleContext):

        SObj = ctx.OpObj

        Line = (None if ctx.start is None else ctx.start).line
        SObj.SetLineNo(Line)

        Prompt = self.__TransNum(ctx.Number())
        SObj.SetPrompt(Prompt)
        
        self.visitChildren(ctx)

        if ctx.vfrSubtitleFlags() != None:
            SObj.SetFlags(ctx.vfrSubtitleFlags().SubFlags)

        ctx.Node.Data = SObj
        self.InsertChild(ctx.Node, ctx.vfrStatementStatTagList())
        # sequence question
        for Ctx in ctx.vfrStatementSubTitleComponent():
            self.InsertChild(ctx.Node, Ctx)
        '''
        print('** Form - Subtitle **')
        Info = ctx.Node.Data.GetInfo()
        print(Info.Statement.Prompt)
        '''


        return ctx.Node

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementSubTitleComponent.
    def visitVfrStatementSubTitleComponent(self, ctx:VfrSyntaxParser.VfrStatementSubTitleComponentContext):
        self.visitChildren(ctx)
        if ctx.vfrStatementQuestions() != None:
            ctx.Node = ctx.vfrStatementQuestions().Node
        elif ctx.vfrStatementStat() != None:
            ctx.Node = ctx.vfrStatementStat().Node
        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrSubtitleFlags.
    def visitVfrSubtitleFlags(self, ctx:VfrSyntaxParser.VfrSubtitleFlagsContext):

        self.visitChildren(ctx)

        for FlagsFieldCtx in ctx.subtitleFlagsField():
            ctx.SubFlags |= FlagsFieldCtx.Flag

        return None


    # Visit a parse tree produced by VfrSyntaxParser#subtitleFlagsField.
    def visitSubtitleFlagsField(self, ctx:VfrSyntaxParser.SubtitleFlagsFieldContext):

        if ctx.Number() != None:
            ctx.Flag = self.__TransNum(ctx.Number())
        else:
            ctx.Flag = 0x01

        return self.visitChildren(ctx)

    def __AssignQuestionKey(self, OpObj, Key):

        if Key == None:
            return
        if OpObj.GetFlags() & EFI_IFR_FLAG_CALLBACK:
            # if the question is not CALLBACK ignore the key.
            self.__CVfrQuestionDB.UpdateQuestionId(OpObj.GetQuestionId(), Key)
            OpObj.SetQuestionId(Key)
        return


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementStaticText.
    def visitVfrStatementStaticText(self, ctx:VfrSyntaxParser.VfrStatementStaticTextContext):

        self.visitChildren(ctx)

        QId = EFI_QUESTION_ID_INVALID
        Help = self.__TransNum(ctx.Number(0))
        Prompt = self.__TransNum(ctx.Number(1))
        TxtTwo = self.__TransNum(ctx.Number(2)) if len(ctx.Text()) == 3 else EFI_STRING_ID_INVALID

        TextFlags = 0
        for FlagsFieldCtx in ctx.staticTextFlagsField():
            TextFlags |= FlagsFieldCtx.Flag

        if TextFlags & EFI_IFR_FLAG_CALLBACK:
            if TxtTwo != EFI_STRING_ID_INVALID:
                pass #error Handler

            AObj = CIfrAction()
            QId, ReturnCode = self.__CVfrQuestionDB.RegisterQuestion(None, None, QId)
            AObj.SetLineNo(ctx.staticTextFlagsField(0).Line)
            AObj.SetQuestionId(QId)
            AObj.SetHelp(Help)
            AObj.SetPrompt(Prompt)
            ReturnCode = AObj.SetFlags(TextFlags)
            if ctx.Key() != None:
                Key = self.__TransNum(ctx.Number(len(ctx.Number()) - 1))
                self.__AssignQuestionKey(AObj, Key)
            ctx.Node.Data = AObj
            ctx.Node.OpCode = EFI_IFR_ACTION_OP

            '''
            print('** Form - Text -Action **')
            Info = ctx.Node.Data.GetInfo()
            print(Info.Question.Header.Prompt)
            print(Info.Question.Header.Help)
            print(Info.Question.QuestionId)
            print(Info.Question.VarStoreId)
            print(Info.Question.Flags)
            print(Info.QuestionConfig)
            '''


        else:
            TObj = CIfrText()
            Line = (None if ctx.start is None else ctx.start).line
            TObj.SetLineNo(Line)
            TObj.SetHelp(Help)
            TObj.SetPrompt(Prompt)
            TObj.SetTextTwo(TxtTwo)
            ctx.Node.Data = TObj

            '''
            print('** Form - Text **')
            Info = ctx.Node.Data.GetInfo()
            print(Info.Statement.Prompt)
            print(Info.Statement.Help)
            print(Info.TextTwo)
            '''

        return ctx.Node



    # Visit a parse tree produced by VfrSyntaxParser#staticTextFlagsField.
    def visitStaticTextFlagsField(self, ctx:VfrSyntaxParser.StaticTextFlagsFieldContext):

        self.visitChildren(ctx)

        if ctx.Number() != None:
            ctx.Flag = self.__TransNum(ctx.Number())
            if ctx.Flag != 0:
                ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
            else:
                ReturnCode = VfrReturnCode.VFR_RETURN_SUCCESS
        else:

            ctx.Flag = ctx.questionheaderFlagsField().QHFlag

        ctx.Line = (None if ctx.stop is None else ctx.stop).line

        return ctx.Flag


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementCrossReference.
    def visitVfrStatementCrossReference(self, ctx:VfrSyntaxParser.VfrStatementCrossReferenceContext):
        self.visitChildren(ctx)
        if ctx.vfrStatementGoto() != None:
            ctx.Node = ctx.vfrStatementGoto().Node
        elif ctx.vfrStatementResetButton() != None:
            ctx.Node = ctx.vfrStatementResetButton().Node
        return ctx.Node



    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementGoto.
    def visitVfrStatementGoto(self, ctx:VfrSyntaxParser.VfrStatementGotoContext):

        RefType = 5
        DevPath = EFI_STRING_ID_INVALID
        QId = EFI_QUESTION_ID_INVALID
        BitMask = 0
        Line = (None if ctx.start is None else ctx.start).line
        R5Obj = CIfrRef5()
        R5Obj.SetLineNo(Line)
        ctx.OpObj = R5Obj
        #ctx.OHObj = R5Obj

        if ctx.DevicePath() != None:
            RefType = 4
            DevPath = self.__TransNum(ctx.Number(0))
            FId = self.__TransNum(ctx.Number(1))
            QId = self.__TransNum(ctx.Number(2))
            R4Obj = CIfrRef4()
            R4Obj.SetLineNo(Line)
            R4Obj.SetDevicePath(DevPath)
            R4Obj.SetFormId(FId)
            R4Obj.SetQuestionId(QId)
            ctx.OpObj = R4Obj
            #ctx.OHObj = R4Obj

        elif ctx.FormSetGuid() != None:
            RefType = 3
            FId = self.__TransNum(ctx.Number(0))
            QId = self.__TransNum(ctx.Number(1))
            R3Obj = CIfrRef3()
            R3Obj.SetLineNo(Line)
            R3Obj.SetFormId(FId)
            R3Obj.SetQuestionId(QId)
            ctx.OpObj = R3Obj
            #ctx.OHObj = R3Obj

        elif ctx.FormId() != None:
            FId = self.__TransNum(ctx.Number(0))
            RefType = 2
            if ctx.StringIdentifier() != None:
                Name = self.__TransId(ctx.StringIdentifier())
                QId, BitMask, _ = self.__CVfrQuestionDB.GetQuestionId(Name)
                if QId == EFI_QUESTION_ID_INVALID:
                    print('undefined') #error handle VFR_RETURN_UNDEFINED
            else:
                QId = self.__TransNum(ctx.Number(1))
            R2Obj = CIfrRef2()
            R2Obj.SetLineNo(Line)
            R2Obj.SetFormId(FId)
            R2Obj.SetQuestionId(QId)
            ctx.OpObj = R2Obj
        # ctx.OHObj = R2Obj


        elif str(ctx.getChild(1)) == str(ctx.Number(0)):
            RefType = 1
            FId = self.__TransNum(ctx.Number(0))
            RObj = CIfrRef()
            RObj.SetLineNo(Line)
            RObj.SetFormId(FId)
            ctx.OpObj = RObj
        # ctx.OHObj = RObj

        self.visitChildren(ctx)

        if self.__CurrQestVarInfo.VarType == EFI_IFR_TYPE_OTHER:
            self.__CurrQestVarInfo.VarType == EFI_IFR_TYPE_REF

        if RefType == 4 or RefType == 3:
            ctx.OpObj.SetFormSetId(ctx.guidDefinition().Guid)
        # ctx.OHObj.SetFormSetId(ctx.guidDefinition().Guid)

        if ctx.FLAGS() != None:
            ctx.OpObj.SetFlags(ctx.vfrGotoFlags().GotoFlags)
        # ctx.OHObj.SetFlags(ctx.vfrGotoFlags().GotoFlags)

        if ctx.Key() != None:
            index = int(len(ctx.Number())) - 1
            Key = self.__TransNum(ctx.Number(index))
            self.__AssignQuestionKey(ctx.OpObj, Key)

        # ctx.OHObj.SetScope(1)
        ctx.OpObj.SetScope(1)
        ctx.Node.Data = ctx.OpObj
        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)

        '''
        print('** vfrStatementQuestions - Goto **')
        Info = ctx.Node.Data.GetInfo()
        if RefType == 4: 
            print('4')
            print(Info.FormId)
            print(Info.QuestionId)
            print(ctx.guidDefinition().getText())
            print(Info.DevicePath)
        if RefType == 3: 
            print('3')
            print(Info.FormId)
            print(Info.QuestionId)
            print(ctx.guidDefinition().getText())
        if RefType == 2: 
            print('2')
            print(Info.FormId)
            print(Info.QuestionId)
        if RefType == 1: 
            print('1')
            print(Info.FormId)
        print(Info.Question.Header.Prompt)
        print(Info.Question.Header.Help)
        '''


        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrGotoFlags.
    def visitVfrGotoFlags(self, ctx:VfrSyntaxParser.VfrGotoFlagsContext):

        self.visitChildren(ctx)
        for FlagsFieldCtx in ctx.gotoFlagsField():
            ctx.GotoFlags |= FlagsFieldCtx.Flag

        return ctx.GotoFlags



    # Visit a parse tree produced by VfrSyntaxParser#gotoFlagsField.
    def visitGotoFlagsField(self, ctx:VfrSyntaxParser.GotoFlagsFieldContext):

        self.visitChildren(ctx)

        if ctx.Number() != None:
            if self.__TransNum(ctx.Number()) != 0:
                ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
            else:
                ReturnCode = VfrReturnCode.VFR_RETURN_SUCCESS
        else:
            ctx.Flag = ctx.questionheaderFlagsField().QHFlag

        return ctx.Flag


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementResetButton.
    def visitVfrStatementResetButton(self, ctx:VfrSyntaxParser.VfrStatementResetButtonContext):

        self.visitChildren(ctx)

        RBObj = ctx.OpObj
        Line = (None if ctx.start is None else ctx.start).line
        RBObj.SetLineNo(Line)
        defaultstore = self.__TransId(ctx.StringIdentifier())
        DefaultId, ReturnCode = gCVfrDefaultStore.GetDefaultId(defaultstore)
        if ReturnCode == VfrReturnCode.VFR_RETURN_SUCCESS:
            RBObj.SetDefaultId(DefaultId)

        ctx.Node.Data = RBObj
        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)

        '''
        print('** vfrStatementQuestions - Rebutton **')
        Info = ctx.Node.Data.GetInfo()

        print(Info.Statement.Prompt)
        print(Info.Statement.Help)
        print(Info.DefaultId)
        '''

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementQuestions.
    def visitVfrStatementQuestions(self, ctx:VfrSyntaxParser.VfrStatementQuestionsContext):

        self.visitChildren(ctx)
        if ctx.vfrStatementBooleanType() != None:
            ctx.Node = ctx.vfrStatementBooleanType().Node
        if ctx.vfrStatementDate() != None:
            ctx.Node = ctx.vfrStatementDate().Node
        if ctx.vfrStatementNumericType() != None:
            ctx.Node = ctx.vfrStatementNumericType().Node
        if ctx.vfrStatementStringType() != None:
            ctx.Node = ctx.vfrStatementStringType().Node
        if ctx.vfrStatementOrderedList() != None:
            ctx.Node = ctx.vfrStatementOrderedList().Node
        if ctx.vfrStatementTime() != None:
            ctx.Node = ctx.vfrStatementTime().Node

        # ctx.Node.Parent = ctx.parentCtx.Node

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementQuestionTag.
    def visitVfrStatementQuestionTag(self, ctx:VfrSyntaxParser.VfrStatementQuestionTagContext):
        self.visitChildren(ctx)
        if ctx.vfrStatementStatTag() != None:
            ctx.Node = ctx.vfrStatementStatTag().Node
        if ctx.vfrStatementInconsistentIf() != None:
            ctx.Node = ctx.vfrStatementInconsistentIf().Node
        if ctx.vfrStatementNoSubmitIf() != None:
            ctx.Node = ctx.vfrStatementNoSubmitIf().Node
        if ctx.vfrStatementDisableIfQuest() != None:
            ctx.Node = ctx.vfrStatementDisableIfQuest().Node
        if ctx.vfrStatementRefresh() != None:
            ctx.Node = ctx.vfrStatementRefresh().Node
        if ctx.vfrStatementVarstoreDevice() != None:
            ctx.Node = ctx.vfrStatementVarstoreDevice().Node
        if ctx.vfrStatementExtension() != None:
            ctx.Node = ctx.vfrStatementExtension().Node
        if ctx.vfrStatementRefreshEvent() != None:
            ctx.Node = ctx.vfrStatementRefreshEvent().Node
        if ctx.vfrStatementWarningIf() != None:
            ctx.Node = ctx.vfrStatementWarningIf().Node

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementInconsistentIf.
    def visitVfrStatementInconsistentIf(self, ctx:VfrSyntaxParser.VfrStatementInconsistentIfContext):

        IIObj = CIfrInconsistentIf()
        self.visitChildren(ctx)

        IIObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        IIObj.SetError(self.__TransNum(ctx.Number()))

        ctx.Node.Data = IIObj
        ctx.Node.Expression = self.__ExtractOriginalText(ctx.vfrStatementExpression())

        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementNoSubmitIf.
    def visitVfrStatementNoSubmitIf(self, ctx:VfrSyntaxParser.VfrStatementNoSubmitIfContext):
        NSIObj = CIfrNoSubmitIf()
        self.visitChildren(ctx)

        NSIObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        NSIObj.SetError(self.__TransNum(ctx.Number()))
        ctx.Node.Data = NSIObj
        ctx.Node.Expression = self.__ExtractOriginalText(ctx.vfrStatementExpression())

        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementDisableIfQuest.
    def visitVfrStatementDisableIfQuest(self, ctx:VfrSyntaxParser.VfrStatementDisableIfQuestContext):
        DIObj = CIfrDisableIf()
        self.visitChildren(ctx)

        DIObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        ctx.Node.Data = DIObj
        ctx.Node.Expression = self.__ExtractOriginalText(ctx.vfrStatementExpression())

        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)
        return DIObj, EObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementRefresh.
    def visitVfrStatementRefresh(self, ctx:VfrSyntaxParser.VfrStatementRefreshContext):
        RObj = CIfrRefresh()
        self.visitChildren(ctx)

        RObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        RObj.SetRefreshInterval(self.__TransNum(ctx.Number()))
        ctx.Node.Data = RObj

        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)

        return RObj, EObj

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementVarstoreDevice.
    def visitVfrStatementVarstoreDevice(self, ctx:VfrSyntaxParser.VfrStatementVarstoreDeviceContext):
        VDObj = CIfrVarStoreDevice()
        self.visitChildren(ctx)

        VDObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        VDObj.SetDevicePath(self.__TransNum(ctx.Number()))
        ctx.Node.Data = VDObj

        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)

        return ctx.Node

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementRefreshEvent.
    def visitVfrStatementRefreshEvent(self, ctx:VfrSyntaxParser.VfrStatementRefreshEventContext):
        RiObj = CIfrRefreshId()
        self.visitChildren(ctx)

        RiObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        RiObj.SetRefreshEventGroutId(ctx.guidDefinition().Guid)
        ctx.Node.Data = RiObj

        return ctx.Node

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementWarningIf.
    def visitVfrStatementWarningIf(self, ctx:VfrSyntaxParser.VfrStatementWarningIfContext):
        WIObj = CIfrWarningIf()
        self.visitChildren(ctx)

        WIObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        WIObj.SetWarning(self.__TransNum(ctx.Number(0)))
        WIObj.SetTimeOut(self.__TransNum(ctx.Number(1)))
        ctx.Node.Data = WIObj
        ctx.Node.Expression = self.__ExtractOriginalText(ctx.vfrStatementExpression())

        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)
        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementQuestionTagList.
    def visitVfrStatementQuestionTagList(self, ctx:VfrSyntaxParser.VfrStatementQuestionTagListContext):
        self.visitChildren(ctx)
        for Ctx in ctx.vfrStatementQuestionTag():
            self.InsertChild(ctx.Node, Ctx)
        return ctx.Node

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementQuestionOptionTag.
    def visitVfrStatementQuestionOptionTag(self, ctx:VfrSyntaxParser.VfrStatementQuestionOptionTagContext):
        self.visitChildren(ctx)
        if ctx.vfrStatementSuppressIfQuest() != None:
            ctx.Node = ctx.vfrStatementSuppressIfQuest().Node

        if ctx.vfrStatementValue() != None:
            ctx.Node = ctx.vfrStatementValue().Node

        if ctx.vfrStatementDefault() != None:
            ctx.Node = ctx.vfrStatementDefault().Node

        if ctx.vfrStatementOptions() != None:
            ctx.Node = ctx.vfrStatementOptions().Node

        if ctx.vfrStatementRead() != None:
            ctx.Node = ctx.vfrStatementRead().Node

        if ctx.vfrStatementWrite() != None:
            ctx.Node = ctx.vfrStatementWrite().Node

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementSuppressIfQuest.
    def visitVfrStatementSuppressIfQuest(self, ctx:VfrSyntaxParser.VfrStatementSuppressIfQuestContext):

        SIObj = CIfrSuppressIf2()
        SIObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        ctx.Node.Data = SIObj
        ctx.Node.Condition = 'suppressif' + ' ' + self.__ExtractOriginalText(ctx.vfrStatementExpression())

        return ctx.Node

    def OFFSET_OF(self, Type, Field):
        pass

    # Visit a parse tree produced by VfrSyntaxParser#flagsField.
    def visitFlagsField(self, ctx:VfrSyntaxParser.FlagsFieldContext):
        return self.visitChildren(ctx)

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementDefault.
    def visitVfrStatementDefault(self, ctx:VfrSyntaxParser.VfrStatementDefaultContext):

        self.visitChildren(ctx)
        IsExp = False
        DefaultId = EFI_HII_DEFAULT_CLASS_STANDARD

        if ctx.vfrConstantValueField() != None:
            Value = ctx.vfrConstantValueField().Value
            Type = self.__CurrQestVarInfo.VarType
            Size = 0

            if self.__CurrentMinMaxData != None and self.__CurrentMinMaxData.IsNumericOpcode():
                # check default value is valid for Numeric Opcode
                NumericQst = CIfrNumeric (self.__CurrentQuestion) #
                if (NumericQst.GetNumericFlags() & EFI_IFR_DISPLAY) == 0 and self.__CurrQestVarInfo.IsBitVar == False: #
                    if Type == EFI_IFR_TYPE_NUM_SIZE_8:
                        if Value.u8 < self.__CurrentMinMaxData.GetMinData(Type, False) or Value.u8 > self.__CurrentMinMaxData.GetMaxData(Type, False):
                            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                            print("Numeric default value must be between MinValue and MaxValue.")

                    if Type == EFI_IFR_TYPE_NUM_SIZE_16:
                        if Value.u16 < self.__CurrentMinMaxData.GetMinData(Type, False) or Value.u16 > self.__CurrentMinMaxData.GetMaxData(Type, False):
                            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                            print("Numeric default value must be between MinValue and MaxValue.")

                    if Type == EFI_IFR_TYPE_NUM_SIZE_32:
                        if Value.u32 < self.__CurrentMinMaxData.GetMinData(Type, False) or Value.u32 > self.__CurrentMinMaxData.GetMaxData(Type, False):
                            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                            print("Numeric default value must be between MinValue and MaxValue.")

                    if Type == EFI_IFR_TYPE_NUM_SIZE_64:
                        if Value.u64 < self.__CurrentMinMaxData.GetMinData(Type, False) or Value.u64 > self.__CurrentMinMaxData.GetMaxData(Type, False):
                            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                            print("Numeric default value must be between MinValue and MaxValue.")

                else:
                    # Value for question stored in bit fields is always set to UINT32 type.
                    if self.__CurrQestVarInfo.IsBitVar:
                        if Value.u32 < self.__CurrentMinMaxData.GetMinData(Type, True) or  Value.u32 > self.__CurrentMinMaxData.GetMaxData(Type, True):
                            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                            print("Numeric default value must be between MinValue and MaxValue.")
                    else:
                        if Value.u64 < self.__CurrentMinMaxData.GetMinData(Type, False) or  Value.u64 > self.__CurrentMinMaxData.GetMaxData(Type, False):
                            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                            print("Numeric default value must be between MinValue and MaxValue.")

            if Type == EFI_IFR_TYPE_OTHER:
                ReturnCode = VfrReturnCode.VFR_RETURN_FATAL_ERROR
                print("Default data type error.")
                Size = sizeof(EFI_IFR_TYPE_VALUE)
            elif ctx.vfrConstantValueField().ListType:
                Size = len(Value)
                if Type == EFI_IFR_TYPE_NUM_SIZE_16:
                    Size *= sizeof(c_ushort)
                if Type == EFI_IFR_TYPE_NUM_SIZE_32:
                    Size *= sizeof(c_ulong)
                if Type == EFI_IFR_TYPE_NUM_SIZE_64:
                    Size *= sizeof(c_ulonglong)
            else:
                if self.__CurrQestVarInfo.IsBitVar:
                    Size = sizeof(c_ulong)
                else:
                    Size, ReturnCode = gCVfrVarDataTypeDB.GetDataTypeSizeByDataType(Type)

            # Size += self.OFFSET_OF (EFI_IFR_DEFAULT, Value) ########
            DObj = CIfrDefault(Size)
            DObj.SetLineNo((None if ctx.start is None else ctx.start).line)

            if ctx.vfrConstantValueField().ListType:
                DObj.SetType(EFI_IFR_TYPE_BUFFER)
            elif self.__IsStringOp:
                DObj.SetType(EFI_IFR_TYPE_STRING)
            else:
                if self.__CurrQestVarInfo.IsBitVar:
                    DObj.SetType(EFI_IFR_TYPE_NUM_SIZE_32)
                else:
                    DObj.SetType(self.__CurrQestVarInfo.VarType)

            if ctx.vfrConstantValueField().ListType == False:
                DObj.SetValue(Value)

        else:
            IsExp = True
            DObj = CIfrDefault2()
            DObj.SetLineNo((None if ctx.start is None else ctx.start).line)
            DObj.SetScope(1)

        if ctx.DefaultStore() != None:
            DefaultId, ReturnCode = gCVfrDefaultStore.GetDefaultId(self.__TransId(ctx.StringIdentifier()))
            DObj.SetDefaultId(DefaultId)

        self.__CheckDuplicateDefaultValue(DefaultId, ctx.Default())
        if self.__CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            VarStoreName, ReturnCode = gCVfrDataStorage.GetVarStoreName(self.__CurrQestVarInfo.VarStoreId)
            VarGuid = gCVfrDataStorage.GetVarStoreGuid(self.__CurrQestVarInfo.VarStoreId)
            VarStoreType = gCVfrDataStorage.GetVarStoreType(self.__CurrQestVarInfo.VarStoreId)
            if (IsExp == False) and (VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER):
                ReturnCode = gCVfrDefaultStore.BufferVarStoreAltConfigAdd(DefaultId,self.__CurrQestVarInfo,VarStoreName,VarGuid,self.__CurrQestVarInfo.VarType, Value)

        ctx.Node.Data = DObj
        self.InsertChild(ctx.Node, ctx.vfrStatementValue())

        '''
        if self.__IsNumericOp:
            print('** Form - vfrStatementDefault - Default **')
            Info = ctx.Node.Data.GetInfo()
            print(Info.Type)
            print(Info.DefaultId)
            # print(Info.Value)
        '''

        return ctx.Node



    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementValue.
    def visitVfrStatementValue(self, ctx:VfrSyntaxParser.VfrStatementValueContext):
        VObj = CIfrValue()
        self.visitChildren(ctx)

        VObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        ctx.Node.Data = VObj
        ctx.Node.Expression = self.__ExtractOriginalText(ctx.vfrStatementExpression())

        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)

        return ctx.Node

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementOptions.
    def visitVfrStatementOptions(self, ctx:VfrSyntaxParser.VfrStatementOptionsContext):
        self.visitChildren(ctx)
        ctx.Node = ctx.vfrStatementOneOfOption().Node
        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementOneOfOption.
    def visitVfrStatementOneOfOption(self, ctx:VfrSyntaxParser.VfrStatementOneOfOptionContext):

        if self.__CurrQestVarInfo.VarType == EFI_IFR_TYPE_OTHER:
            ReturnCode = VfrReturnCode.VFR_RETURN_FATAL_ERROR
            print("Get data type error.")

        self.visitChildren(ctx)

        Value = ctx.vfrConstantValueField().Value
        ValueStr = ctx.vfrConstantValueField().ValueStr
        Type = self.__CurrQestVarInfo.VarType
        Size = 0
        if self.__CurrentMinMaxData != None:
            #set min/max value for oneof opcode
            Step = self.__CurrentMinMaxData.GetStepData(self.__CurrQestVarInfo.VarType, self.__CurrQestVarInfo.IsBitVar)
            if self.__CurrQestVarInfo.IsBitVar:
                self.__CurrentMinMaxData.SetMinMaxStepData(Value.u32, Value.u32, Step, EFI_IFR_TYPE_NUM_SIZE_32)
            else:
                if Type == EFI_IFR_TYPE_NUM_SIZE_64:
                    self.__CurrentMinMaxData.SetMinMaxStepData(Value.u64, Value.u64, Step, EFI_IFR_TYPE_NUM_SIZE_64)
                if Type == EFI_IFR_TYPE_NUM_SIZE_32:
                    self.__CurrentMinMaxData.SetMinMaxStepData(Value.u32, Value.u32, Step, EFI_IFR_TYPE_NUM_SIZE_32)
                if Type == EFI_IFR_TYPE_NUM_SIZE_16:
                    self.__CurrentMinMaxData.SetMinMaxStepData(Value.u16, Value.u16, Step, EFI_IFR_TYPE_NUM_SIZE_16)
                if Type == EFI_IFR_TYPE_NUM_SIZE_8:
                    self.__CurrentMinMaxData.SetMinMaxStepData(Value.u8, Value.u8, Step, EFI_IFR_TYPE_NUM_SIZE_8)

        if self.__CurrQestVarInfo.VarType == EFI_IFR_TYPE_OTHER:
            Size = sizeof(EFI_IFR_TYPE_VALUE)
        elif ctx.vfrConstantValueField().ListType:
            Size = len(Value)
            if Type == EFI_IFR_TYPE_NUM_SIZE_16:
                Size *= sizeof(c_ushort)
            if Type == EFI_IFR_TYPE_NUM_SIZE_32:
                Size *= sizeof(c_ulong)
            if Type == EFI_IFR_TYPE_NUM_SIZE_64:
                Size *= sizeof(c_ulonglong)
        else:
            # For the oneof stored in bit fields, set the option type as UINT32.
            if self.__CurrQestVarInfo.IsBitVar:
                Size = sizeof(c_long)
            else:
                Size, ReturnCode = gCVfrVarDataTypeDB.GetDataTypeSizeByDataType(Type)
        # Size + = offset
        OOOObj = CIfrOneOfOption(Size)
        OOOObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        OOOObj.SetOption(self.__TransNum(ctx.Number(0)))
        if ctx.vfrConstantValueField().ListType:
            OOOObj.SetType(EFI_IFR_TYPE_BUFFER)
        else:
            if self.__CurrQestVarInfo.IsBitVar:
                OOOObj.SetType(EFI_IFR_TYPE_NUM_SIZE_32)
            else:
                OOOObj.SetType(Type)

        OOOObj.SetValue(Value) #
        ReturnCode = OOOObj.SetFlags(ctx.vfrOneOfOptionFlags().LFlags)
        ReturnCode = self.__CurrentQuestion.SetQHeaderFlags(ctx.vfrOneOfOptionFlags().HFlags)

        # Array type only for default type OneOfOption.
        if (OOOObj.GetFlags() & (EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG)) == 0 and (ctx.vfrConstantValueField().ListType):
            ReturnCode = VfrReturnCode.VFR_RETURN_FATAL_ERROR
            print("Default keyword should with array value type!")

        # Clear the default flag if the option not use array value but has default flag.
        if (OOOObj.GetFlags() & (EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG)) != 0 and (ctx.vfrConstantValueField().ListType == False) and (self.__IsOrderedList):
            OOOObj.SetFlags(OOOObj.GetFlags() & ~(EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG))

        if self.__CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            VarStoreName, ReturnCode = gCVfrDataStorage.GetVarStoreName(self.__CurrQestVarInfo.VarStoreId)
            VarStoreGuid = gCVfrDataStorage.GetVarStoreGuid(self.__CurrQestVarInfo.VarStoreId)
            if OOOObj.GetFlags() & EFI_IFR_OPTION_DEFAULT:
                self.__CheckDuplicateDefaultValue(EFI_HII_DEFAULT_CLASS_STANDARD, ctx.FLAGS()) #
                ReturnCode = gCVfrDefaultStore.BufferVarStoreAltConfigAdd(EFI_HII_DEFAULT_CLASS_STANDARD, self.__CurrQestVarInfo, VarStoreName, VarStoreGuid, self.__CurrQestVarInfo.VarType, Value)
            if OOOObj.GetFlags() & EFI_IFR_OPTION_DEFAULT_MFG:
                self.__CheckDuplicateDefaultValue(EFI_HII_DEFAULT_CLASS_MANUFACTURING, ctx.FLAGS()) #
                ReturnCode = gCVfrDefaultStore.BufferVarStoreAltConfigAdd(EFI_HII_DEFAULT_CLASS_MANUFACTURING, self.__CurrQestVarInfo, VarStoreName, VarStoreGuid, self.__CurrQestVarInfo.VarType, Value)

        if ctx.Key() != None:
            ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
            #　Guid Option Key
            IfrOptionKey = CIfrOptionKey(self.__CurrentQuestion.GetQuestionId(), Value, self.__TransNum(ctx.Number(1)))
            IfrOptionKey.SetLineNo()
        if ctx.vfrImageTag() != None:
            OOOObj.SetScope(1) #
            EObj = CIfrEnd() #
            Line = (None if ctx.stop is None else ctx.stop).line
            EObj.SetLineNo(Line)

        ctx.Node.Data = OOOObj
        for Ctx in ctx.vfrImageTag():
            self.InsertChild(ctx.Node, Ctx)
        '''
        print('** vfrStatementQuestions - Oneof-Option **')
        Info = ctx.Node.Data.GetInfo()
        print(Info.Option)
        print(Info.Flags)
        print(Info.Type)
        print(Info.Value)
        '''

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrOneOfOptionFlags.
    def visitVfrOneOfOptionFlags(self, ctx:VfrSyntaxParser.VfrOneOfOptionFlagsContext):

        self.visitChildren(ctx)

        ctx.LFlags = self.__CurrQestVarInfo.VarType
        for FlagsFieldCtx in ctx.oneofoptionFlagsField():
            ctx.HFlags |= FlagsFieldCtx.HFlag
            ctx.LFlags |= FlagsFieldCtx.LFlag

        ctx.LineNum = (None if ctx.start is None else ctx.start).line

        return ctx.HFlags, ctx.LFlags


    # Visit a parse tree produced by VfrSyntaxParser#oneofoptionFlagsField.
    def visitOneofoptionFlagsField(self, ctx:VfrSyntaxParser.OneofoptionFlagsFieldContext):

        self.visitChildren(ctx)

        if ctx.Number() != None:
            ctx.LFlag = self.__TransNum(ctx.Number())
        if ctx.OptionDefault() != None:
            ctx.LFlag = 0x10
        if ctx.OptionDefaultMfg() != None:
            ctx.LFlag = 0x20
        if ctx.InteractiveFlag() != None:
            ctx.HFlag = 0x04
        if ctx.ResetRequiredFlag() != None:
            ctx.HFlag = 0x10
        if ctx.RestStyleFlag() != None:
            ctx.HFlag = 0x20
        if ctx.ReconnectRequiredFlag() != None:
            ctx.HFlag = 0x40
        if ctx.ManufacturingFlag() != None:
            ctx.LFlag = 0x20
        if ctx.DefaultFlag() != None:
            ctx.LFlag = 0x10
        if ctx.NVAccessFlag() != None and ctx.LateCheckFlag() != None:
            pass # error handle

        return ctx.HFlag, ctx.LFlag


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementRead.
    def visitVfrStatementRead(self, ctx:VfrSyntaxParser.VfrStatementReadContext):
        RObj = CIfrRead()
        self.visitChildren(ctx)

        RObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        ctx.Node.Data = RObj
        ctx.Node.Expression = self.__ExtractOriginalText(ctx.vfrStatementExpression())
        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)

        return ctx.Node

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementWrite.
    def visitVfrStatementWrite(self, ctx:VfrSyntaxParser.VfrStatementWriteContext):
        WObj = CIfrWrite()
        self.visitChildren(ctx)

        WObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        ctx.Node.Data = WObj
        ctx.Node.Expression = self.__ExtractOriginalText(ctx.vfrStatementExpression())

        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)

        return ctx.Node

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementQuestionOptionList.
    def visitVfrStatementQuestionOptionList(self, ctx:VfrSyntaxParser.VfrStatementQuestionOptionListContext):

        self.visitChildren(ctx)
        for Ctx in ctx.vfrStatementQuestionOption():
            self.InsertChild(ctx.Node, Ctx)
        

        '''
        for ChildNode in ctx.Node.Child:
            if ChildNode.Condition != None:
                print("test Condition")
                print(ChildNode.Condition)
        '''
        
        return ctx.Node

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementQuestionOption.
    def visitVfrStatementQuestionOption(self, ctx:VfrSyntaxParser.VfrStatementQuestionOptionContext):
        self.visitChildren(ctx)
        if ctx.vfrStatementQuestionTag() != None:
            ctx.Node = ctx.vfrStatementQuestionTag().Node
            
        elif ctx.vfrStatementQuestionOptionTag() != None:
            ctx.Node = ctx.vfrStatementQuestionOptionTag().Node
        # ctx.Node.Parent = ctx.parentCtx.Node
        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementBooleanType.
    def visitVfrStatementBooleanType(self, ctx:VfrSyntaxParser.VfrStatementBooleanTypeContext):
        self.visitChildren(ctx)
        if ctx.vfrStatementCheckBox() != None:
            ctx.Node = ctx.vfrStatementCheckBox().Node
        else:
            ctx.Node = ctx.vfrStatementAction().Node

        return ctx.Node


    def __GetCurrQestDataType(self): #
        return self.__CurrQestVarInfo.VarType

    def __GetCurrQestVarInfo(self): #
        return self.__CurrQestVarInfo

    def __CheckDuplicateDefaultValue(self, DefaultId,Tok): #
        pass


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementCheckBox.
    def visitVfrStatementCheckBox(self, ctx:VfrSyntaxParser.VfrStatementCheckBoxContext):

        CBObj = ctx.OpObj
        Line =  (None if ctx.start is None else ctx.start).line
        CBObj.SetLineNo(Line)
        self.__CurrentQuestion = CBObj.GetQuestion()
        self.__IsCheckBoxOp = True

        self.visitChildren(ctx)

        # Create a GUID opcode to wrap the checkbox opcode, if it refer to bit varstore.
        if self.__CurrQestVarInfo.IsBitVar:
            GuidObj = CIfrGuid(0)
            GuidObj.SetGuid(EDKII_IFR_BIT_VARSTORE_GUID)
            GuidObj.SetLineNo(Line)
            GuidObj.SetScope(1) #

        # check dataType
        if self.__CurrQestVarInfo.VarType == EFI_IFR_TYPE_OTHER:
            self.__CurrQestVarInfo.VarType = EFI_IFR_TYPE_BOOLEAN

        if self.__CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            # Check whether the question refers to a bit field, if yes. create a Guid to indicate the question refers to a bit field.
            if self.__CurrQestVarInfo.IsBitVar:
                _, ReturnCode = gCVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.__CurrQestVarInfo.VarType)
                if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                    print("CheckBox varid is not the valid data type")
                if gCVfrDataStorage.GetVarStoreType(self.__CurrQestVarInfo.VarStoreId) == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER_BITS and self.__CurrQestVarInfo.VarTotalSize != 1:
                    ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                    print("CheckBox varid only occupy 1 bit in Bit Varstore")
                else:
                    Size, ReturnCode = gCVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.__CurrQestVarInfo.VarType)
                    if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                        print("CheckBox varid is not the valid data type")
                    if Size != 0 and Size != self.__CurrQestVarInfo.VarTotalSize:
                        print(Size)
                        print(self.__CurrQestVarInfo.VarTotalSize)
                        ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                        print("CheckBox varid doesn't support array")
                    elif gCVfrDataStorage.GetVarStoreType(self.__CurrQestVarInfo.VarStoreId) == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER and self.__CurrQestVarInfo.VarTotalSize != sizeof(ctypes.c_bool):
                        ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                            print("CheckBox varid only support BOOLEAN data type")

        if ctx.FLAGS() != None:
            CBObj.SetFlags(ctx.vfrCheckBoxFlags().HFlags, ctx.vfrCheckBoxFlags().LFlags)
            if self.__CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
                VarStoreName, ReturnCode = gCVfrDataStorage.GetVarStoreName(self.__CurrQestVarInfo.VarStoreId)
                if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                    print("Failed to retrieve varstore name")

                VarStoreGuid = gCVfrDataStorage.GetVarStoreGuid(self.__CurrQestVarInfo.VarStoreId)
                if CBObj.GetFlags() & 0x01:
                    self.__CheckDuplicateDefaultValue(EFI_HII_DEFAULT_CLASS_STANDARD, ctx.FLAGS()) ####### error handle
                    self.gZeroEfiIfrTypeValue.b = True
                    ReturnCode = gCVfrDefaultStore.BufferVarStoreAltConfigAdd(EFI_HII_DEFAULT_CLASS_STANDARD,self.__CurrQestVarInfo, VarStoreName, VarStoreGuid, self.__CurrQestVarInfo.VarType, self.gZeroEfiIfrTypeValue)
                    if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                        print( "No standard default storage found")
                if CBObj.GetFlags() & 0x02:
                    self.__CheckDuplicateDefaultValue(EFI_HII_DEFAULT_CLASS_STANDARD, ctx.FLAGS()) #######
                    ReturnCode =  gCVfrDefaultStore.BufferVarStoreAltConfigAdd(EFI_HII_DEFAULT_CLASS_MANUFACTURING,self.__CurrQestVarInfo, VarStoreName, VarStoreGuid, self.__CurrQestVarInfo.VarType, self.gZeroEfiIfrTypeValue)
                    if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                        print( "No manufacturing default storage found")
        if ctx.Key() != None:
            Key = self.__TransNum(ctx.Number())
            self.__AssignQuestionKey(CBObj, Key)

        ctx.Node.Data = CBObj
        self.__IsCheckBoxOp = False

        '''
        print('** Form - vfrStatementQuestions - CheckBox **')
        Info = ctx.Node.Data.GetInfo()
        print(ctx.Node.Condition)
        print(Info.Question.Header.Prompt)
        print(Info.Question.Header.Help)
        print(Info.Question.QuestionId)
        print(Info.Question.VarStoreId)
        print(Info.Question.VarStoreInfo.VarName)
        print(Info.Question.VarStoreInfo.VarOffset)
        print(Info.Flags)
        '''


        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrCheckBoxFlags.
    def visitVfrCheckBoxFlags(self, ctx:VfrSyntaxParser.VfrCheckBoxFlagsContext):

        self.visitChildren(ctx)
        for FlagsFieldCtx in ctx.checkboxFlagsField():
            ctx.LFlags |= FlagsFieldCtx.LFlag
            ctx.HFlags |= FlagsFieldCtx.HFlag

        return ctx.HFlags, ctx.LFlags


    # Visit a parse tree produced by VfrSyntaxParser#checkboxFlagsField.
    def visitCheckboxFlagsField(self, ctx:VfrSyntaxParser.CheckboxFlagsFieldContext):

        self.visitChildren(ctx)

        if ctx.Number() != None:
            if self.__TransNum(ctx.Number()) != 0:
                ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
            else:
                ReturnCode = VfrReturnCode.VFR_RETURN_SUCCESS
        elif ctx.DefaultFlag() != None or ctx.ManufacturingFlag() != None:
            ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
        elif ctx.CheckBoxDefaultFlag() != None:
            ctx.LFlag = 0x01
        elif ctx.CheckBoxDefaultMfgFlag() != None:
            ctx.LFlag = 0x02
        else:
            ctx.HFlag = ctx.questionheaderFlagsField().QHFlag

        return ctx.HFlag, ctx.LFlag

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementAction.
    def visitVfrStatementAction(self, ctx:VfrSyntaxParser.VfrStatementActionContext):

        self.visitChildren(ctx)
        AObj = ctx.OpObj
        AObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        AObj.SetQuestionConfig(self._TransNum(ctx.Number()))

        ctx.Node.Data = AObj
        self.InsertChild(ctx.Node, ctx.vfrStatementQuestionTagList())

        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrActionFlags.
    def visitVfrActionFlags(self, ctx:VfrSyntaxParser.VfrActionFlagsContext):

        self.visitChildren(ctx)
        for FlagsFieldCtx in ctx.actionFlagsField():
            ctx.HFlags |= FlagsFieldCtx.HFlag

        ctx.LineNum = (None if ctx.start is None else ctx.start).line

        return ctx.HFlags


    # Visit a parse tree produced by VfrSyntaxParser#actionFlagsField.
    def visitActionFlagsField(self, ctx:VfrSyntaxParser.ActionFlagsFieldContext):

        self.visitChildren(ctx)

        if ctx.Number() != None:
            if self.__TransNum(ctx.Number()) != 0:
                ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
            else:
                ReturnCode = VfrReturnCode.VFR_RETURN_SUCCESS
        else:
            ctx.HFlag = ctx.questionheaderFlagsField().QHFlag
        return ctx.HFlag


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementNumericType.
    def visitVfrStatementNumericType(self, ctx:VfrSyntaxParser.VfrStatementNumericTypeContext):

        self.visitChildren(ctx)
        if ctx.vfrStatementNumeric() != None:
            ctx.Node = ctx.vfrStatementNumeric().Node
        elif ctx.vfrStatementOneOf() != None:
            ctx.Node = ctx.vfrStatementOneOf().Node

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxP-arser#vfrStatementNumeric.
    def visitVfrStatementNumeric(self, ctx:VfrSyntaxParser.VfrStatementNumericContext):

        self.__IsNumericOp = True
        self.visitChildren(ctx)
        NObj = ctx.OpObj
        NObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        self.__CurrentQuestion = NObj.GetQuestion()

        if self.__CurrQestVarInfo.IsBitVar:
            GuidObj = CIfrGuid(0)
            GuidObj.SetGuid(EDKII_IFR_BIT_VARSTORE_GUID)
            GuidObj.SetLineNo((None if ctx.start is None else ctx.start).line)
            GuidObj.SetScope(1) # pos

        # check data type
        if self.__CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            if self.__CurrQestVarInfo.IsBitVar:
                LFlags = EDKII_IFR_NUMERIC_SIZE_BIT & self.__CurrQestVarInfo.VarTotalSize
                ReturnCode = NObj.SetFlagsForBitField(NObj.GetFlags(),LFlags)
            else:
                DataTypeSize, ReturnCode = gCVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.__CurrQestVarInfo.VarType)
                if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                    print('Numeric varid is not the valid data type')
                if DataTypeSize != 0 and DataTypeSize != self.__CurrQestVarInfo.VarTotalSize:
                    ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                    print('Numeric varid doesn\'t support array')
                ReturnCode = NObj.SetFlags(NObj.GetFlags(), self.__CurrQestVarInfo.VarType)


        if ctx.FLAGS() != None:
            if self.__CurrQestVarInfo.IsBitVar:
                ReturnCode = NObj.SetFlagsForBitField(ctx.vfrNumericFlags().HFlags,ctx.vfrNumericFlags().LFlags, ctx.vfrNumericFlags().IsDisplaySpecified)
            else:
                ReturnCode = NObj.SetFlags(ctx.vfrNumericFlags().HFlags,ctx.vfrNumericFlags().LFlags, ctx.vfrNumericFlags().IsDisplaySpecified)

        if ctx.Key() != None:
            Key = self.__TransNum(ctx.Number())
            self.__AssignQuestionKey(NObj,Key)

        ShrinkSize = 0
        IsSupported = True
        if self.__CurrQestVarInfo.IsBitVar == False:
            Type = self.__CurrQestVarInfo.VarType
            # Base on the type to know the actual used size, shrink the buffer size allocate before.
            if Type == EFI_IFR_TYPE_NUM_SIZE_8:
                ShrinkSize = 21
            elif Type == EFI_IFR_TYPE_NUM_SIZE_16:
                ShrinkSize = 18
            elif Type == EFI_IFR_TYPE_NUM_SIZE_32:
                ShrinkSize = 12
            elif Type == EFI_IFR_TYPE_NUM_SIZE_64:
                ShrinkSize = 0
            else:
                IsSupported = False
        else:
            #　Question stored in bit fields saved as UINT32 type, so the ShrinkSize same as EFI_IFR_TYPE_NUM_SIZE_32.
            ShrinkSize = 12

        #######　NObj->ShrinkBinSize (ShrinkSize);
        if IsSupported == False:
            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
            print('Numeric question only support UINT8, UINT16, UINT32 and UINT64 data type.')

        ctx.Node.Data = NObj
        self.__IsNumericOp = False

        '''
        print('** Form - vfrStatementQuestions - Numeric **')
        Info = ctx.Node.Data.GetInfo()
        print(Info.Question.Header.Prompt)
        print(Info.Question.Header.Help)
        print(Info.Question.QuestionId)
        print(Info.Question.VarStoreId)
        print(Info.Question.VarStoreInfo.VarName)
        print(Info.Question.VarStoreInfo.VarOffset)
        print(Info.Question.Flags)
        if ctx.Node.Data.GetVarType() == EFI_IFR_TYPE_NUM_SIZE_64:
            print(Info.Data.u64.MinValue)
            print(Info.Data.u64.MaxValue)
            print(Info.Data.u64.Step)

        if ctx.Node.Data.GetVarType() == EFI_IFR_TYPE_NUM_SIZE_32:
            print(Info.Data.u32.MinValue)
            print(Info.Data.u32.MaxValue)
            print(Info.Data.u32.Step)
            
        if ctx.Node.Data.GetVarType() == EFI_IFR_TYPE_NUM_SIZE_16:
            print(Info.Data.u16.MinValue)
            print(Info.Data.u16.MaxValue)
            print(Info.Data.u16.Step)
            
        if ctx.Node.Data.GetVarType() == EFI_IFR_TYPE_NUM_SIZE_8:
            print(Info.Data.u8.MinValue)
            print(Info.Data.u8.MaxValue)
            print(Info.Data.u8.Step)
        '''

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrSetMinMaxStep.
    def visitVfrSetMinMaxStep(self, ctx:VfrSyntaxParser.VfrSetMinMaxStepContext):
        IntDecStyle = False
        if ((self.__CurrQestVarInfo.IsBitVar) and (ctx.OpObj.GetOpCode() == EFI_IFR_NUMERIC_OP) and ((ctx.OpObj.GetNumericFlags() & EDKII_IFR_DISPLAY_BIT) == 0)) or \
            ((self.__CurrQestVarInfo.IsBitVar == False) and (ctx.OpObj.GetOpCode() == EFI_IFR_NUMERIC_OP) and ((ctx.OpObj.GetNumericFlags() & EFI_IFR_DISPLAY) == 0)):
            IntDecStyle = True
        MinNegative = False
        MaxNegative = False

        self.visitChildren(ctx)

        Min = self.__TransNum(ctx.Number(0))
        Max = self.__TransNum(ctx.Number(1))
        Step = self.__TransNum(ctx.Number(2)) if ctx.Step() != None else 0

        if len(ctx.Negative()) == 2 or (len(ctx.Negative()) == 1 and str(ctx.getChild(2)) == '-'):
            MinNegative = True

        if IntDecStyle == False and MinNegative == True:
            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
            print(' \'-\' can\'t be used when not in int decimal type.')
        if self.__CurrQestVarInfo.IsBitVar:
            if (IntDecStyle == False) and (Min > (1 << self.__CurrQestVarInfo.VarTotalSize) - 1): #
                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                print('BIT type minimum can\'t small than 0, bigger than 2^BitWidth -1')
            else:
                Type = self.__CurrQestVarInfo.VarType
                if Type == EFI_IFR_TYPE_NUM_SIZE_64:
                    if IntDecStyle:
                        if MinNegative:
                            if Min > 0x8000000000000000:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print('INT64 type minimum can\'t small than -0x8000000000000000, big than 0x7FFFFFFFFFFFFFFF')
                        else:
                            if Min > 0x7FFFFFFFFFFFFFFF:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print('INT64 type minimum can\'t small than -0x8000000000000000, big than 0x7FFFFFFFFFFFFFFF')
                    if MinNegative:
                        Min = ~Min + 1

                if Type == EFI_IFR_TYPE_NUM_SIZE_32:
                    if IntDecStyle:
                        if MinNegative:
                            if Min > 0x80000000:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print('INT32 type minimum can\'t small than -0x80000000, big than 0x7FFFFFFF')
                        else:
                            if Min > 0x7FFFFFFF:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print('INT32 type minimum can\'t small than -0x80000000, big than 0x7FFFFFFF')
                    if MinNegative:
                        Min = ~Min + 1

                if Type == EFI_IFR_TYPE_NUM_SIZE_16:
                    if IntDecStyle:
                        if MinNegative:
                            if Min > 0x8000:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print('INT16 type minimum can\'t small than -0x8000, big than 0x7FFF')
                        else:
                            if Min > 0x7FFF:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print('INT16 type minimum can\'t small than -0x8000, big than 0x7FFF')
                    if MinNegative:
                        Min = ~Min + 1

                if Type == EFI_IFR_TYPE_NUM_SIZE_8:
                    if IntDecStyle:
                        if MinNegative:
                            if Min > 0x80:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print('INT8 type minimum can\'t small than -0x80, big than 0x7F')
                        else:
                            if Min > 0x7F:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print('INT8 type minimum can\'t small than -0x80, big than 0x7F')
                    if MinNegative:
                        Min = ~Min + 1

        if len(ctx.Negative()) == 2 or (len(ctx.Negative()) == 1 and str(ctx.getChild(2)) == str(ctx.Number(0))):
            MaxNegative = True
        if IntDecStyle == False and MaxNegative == True:
            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
            print(' \'-\' can\'t be used when not in int decimal type.')
        if self.__CurrQestVarInfo.IsBitVar:
            if (IntDecStyle == False) and (Max > (1 << self.__CurrQestVarInfo.VarTotalSize) - 1):
                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                print('BIT type maximum can\'t be bigger than 2^BitWidth -1')
            else:
                Type = self.__CurrQestVarInfo.VarType
                if Type == EFI_IFR_TYPE_NUM_SIZE_64:
                    if IntDecStyle:
                        if MaxNegative:
                            if Max > 0x8000000000000000:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print('INT64 type minimum can\'t small than -0x8000000000000000, big than 0x7FFFFFFFFFFFFFFF')
                        else:
                            if Max > 0x7FFFFFFFFFFFFFFF:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print('INT64 type minimum can\'t small than -0x8000000000000000, big than 0x7FFFFFFFFFFFFFFF')
                    if MaxNegative:
                        Max = ~Max + 1

                    if Max < Min: #
                        ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                        print('Maximum can\'t be less than Minimum')


                if Type == EFI_IFR_TYPE_NUM_SIZE_32:
                    if IntDecStyle:
                        if MaxNegative:
                            if Max > 0x80000000:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print('INT32 type minimum can\'t small than -0x80000000, big than 0x7FFFFFFF')
                        else:
                            if Max > 0x7FFFFFFF:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print('INT32 type minimum can\'t small than -0x80000000, big than 0x7FFFFFFF')
                    if MaxNegative:
                        Max = ~Max + 1

                    if Max < Min: #
                        ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                        print('Maximum can\'t be less than Minimum')


                if Type == EFI_IFR_TYPE_NUM_SIZE_16:
                    if IntDecStyle:
                        if MaxNegative:
                            if Max > 0x8000:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print('INT16 type minimum can\'t small than -0x8000, big than 0x7FFF')
                        else:
                            if Max > 0x7FFF:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print('INT16 type minimum can\'t small than -0x8000, big than 0x7FFF')
                    if MaxNegative:
                        Max = ~Max + 1

                    if Max < Min: #
                        ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                        print('Maximum can\'t be less than Minimum')

                if Type == EFI_IFR_TYPE_NUM_SIZE_8:
                    if IntDecStyle:
                        if MaxNegative:
                            if Max > 0x80:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print('INT8 type minimum can\'t small than -0x80, big than 0x7F')
                        else:
                            if Max > 0x7F:
                                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                                print('INT8 type minimum can\'t small than -0x80, big than 0x7F')
                    if MaxNegative:
                        Max = ~Max + 1

                    if Max < Min: #
                        ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                        print('Maximum can\'t be less than Minimum')

        if self.__CurrQestVarInfo.IsBitVar:
            ctx.OpObj.SetMinMaxStepData(Min, Max, Step, EFI_IFR_TYPE_NUM_SIZE_32)
        else:
            Type = self.__CurrQestVarInfo.VarType
            if Type == EFI_IFR_TYPE_NUM_SIZE_64:
                ctx.OpObj.SetMinMaxStepData(Min, Max, Step, EFI_IFR_TYPE_NUM_SIZE_64)
            if Type == EFI_IFR_TYPE_NUM_SIZE_32:
                ctx.OpObj.SetMinMaxStepData(Min, Max, Step, EFI_IFR_TYPE_NUM_SIZE_32)
            if Type == EFI_IFR_TYPE_NUM_SIZE_16:
                ctx.OpObj.SetMinMaxStepData(Min, Max, Step, EFI_IFR_TYPE_NUM_SIZE_16)
            if Type == EFI_IFR_TYPE_NUM_SIZE_8:
                ctx.OpObj.SetMinMaxStepData(Min, Max, Step, EFI_IFR_TYPE_NUM_SIZE_8)

        return ctx.OpObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrNumericFlags.
    def visitVfrNumericFlags(self, ctx:VfrSyntaxParser.VfrNumericFlagsContext):

        ctx.LFlags = self.__CurrQestVarInfo.VarType & EFI_IFR_NUMERIC_SIZE
        VarStoreType = gCVfrDataStorage.GetVarStoreType(self.__CurrQestVarInfo.VarStoreId)
        ctx.LineNum = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)

        for FlagsFieldCtx in ctx.numericFlagsField():
            ctx.HFlags |= FlagsFieldCtx.HFlag
            ctx.IsDisplaySpecified = FlagsFieldCtx.IsDisplaySpecified
            if FlagsFieldCtx.NumericSizeOne() != None:
                if self.__CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_1

            if FlagsFieldCtx.NumericSizeTwo() != None:
                if self.__CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_2

            if FlagsFieldCtx.NumericSizeFour() != None:
                if self.__CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_4

            if FlagsFieldCtx.NumericSizeEight() != None:
                if self.__CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_8

            if FlagsFieldCtx.DisPlayIntDec() != None:
                if self.__CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_INT_DEC
                else:
                    ctx.LFlags =  (ctx.LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_INT_DEC_BIT

            if FlagsFieldCtx.DisPlayUIntHex() != None:
                if self.__CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_UINT_DEC
                else:
                    ctx.LFlags =  (ctx.LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_UINT_DEC_BIT

            if FlagsFieldCtx.DisPlayUIntHex() != None:
                if self.__CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_UINT_HEX
                else:
                    ctx.LFlags =  (ctx.LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_UINT_HEX_BIT

        if self.__CurrQestVarInfo.IsBitVar == False:
            if self.__CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
                if VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER or VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_EFI:
                    if self.__CurrQestVarInfo.VarType != (ctx.LFlags & EFI_IFR_NUMERIC_SIZE):
                        ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                        print('Numeric Flag is not same to Numeric VarData type')
                else:
                    # update data type for name/value store
                    self.__CurrQestVarInfo.VarType = ctx.LFlags & EFI_IFR_NUMERIC_SIZE
                    Size, ReturnCode = gCVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.__CurrQestVarInfo.VarType)
                    self.__CurrQestVarInfo.VarTotalSize = Size
            elif ctx.numericFlagsField().IsSetType:
                self.__CurrQestVarInfo.VarType = ctx.LFlags & EFI_IFR_NUMERIC_SIZE

        elif self.__CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID and self.__CurrQestVarInfo.IsBitVar:
            ctx.LFlags &= EDKII_IFR_DISPLAY_BIT
            ctx.LFlags |= EDKII_IFR_NUMERIC_SIZE_BIT & self.__CurrQestVarInfo.VarTotalSize

        return ctx.HFlags, ctx.LFlags


    # Visit a parse tree produced by VfrSyntaxParser#numericFlagsField.
    def visitNumericFlagsField(self, ctx:VfrSyntaxParser.NumericFlagsFieldContext):

        self.visitChildren(ctx)

        if ctx.Number() != None:
            if self.__TransNum(ctx.Number()) != 0:
                ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
            else:
                ReturnCode = VfrReturnCode.VFR_RETURN_SUCCESS

        if ctx.NumericSizeOne() != None:
            if self.__CurrQestVarInfo.IsBitVar == False:
                ctx.IsSetType = True
            else:
                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                print('Can not specify the size of the numeric value for BIT field')

        if ctx.NumericSizeTwo() != None:
            if self.__CurrQestVarInfo.IsBitVar == False:
                ctx.IsSetType = True
            else:
                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                print('Can not specify the size of the numeric value for BIT field')

        if ctx.NumericSizeFour() != None:
            if self.__CurrQestVarInfo.IsBitVar == False:
                ctx.IsSetType = True
            else:
                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                print('Can not specify the size of the numeric value for BIT field')

        if ctx.NumericSizeEight() != None:
            if self.__CurrQestVarInfo.IsBitVar == False:
                ctx.IsSetType = True
            else:
                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                print('Can not specify the size of the numeric value for BIT field')

        if ctx.DisPlayIntDec() != None:
            ctx.IsDisplaySpecified = True

        if ctx.DisPlayUIntHex() != None:
            ctx.IsDisplaySpecified = True

        if ctx.DisPlayUIntHex() != None:
            ctx.IsDisplaySpecified = True

        if ctx.questionheaderFlagsField() != None:
            ctx.HFlag = ctx.questionheaderFlagsField().QHFlag

        return ctx.HFlag


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementOneOf.
    def visitVfrStatementOneOf(self, ctx:VfrSyntaxParser.VfrStatementOneOfContext):
        OObj = ctx.OpObj
        OObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        self.__CurrentQuestion = OObj.GetQuestion()
        self.__IsOneOfOp = True
        self.visitChildren(ctx)

        if self.__CurrQestVarInfo.IsBitVar:
            GuidObj = CIfrGuid(0)
            GuidObj.SetGuid(EDKII_IFR_BIT_VARSTORE_GUID)
            GuidObj.SetLineNo((None if ctx.start is None else ctx.start).line)
            GuidObj.SetScope(1) # pos

        # check data type
        if self.__CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            if self.__CurrQestVarInfo.IsBitVar:
                LFlags = EDKII_IFR_NUMERIC_SIZE_BIT & self.__CurrQestVarInfo.VarTotalSize
                ReturnCode = OObj.SetFlagsForBitField(OObj.GetFlags(),LFlags)
            else:
                DataTypeSize, ReturnCode = gCVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.__CurrQestVarInfo.VarType)
                if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                    print('OneOf varid is not the valid data type')
                if DataTypeSize != 0 and DataTypeSize != self.__CurrQestVarInfo.VarTotalSize:
                    ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                    print('OneOf varid doesn\'t support array')
                ReturnCode = OObj.SetFlags(OObj.GetFlags(), self.__CurrQestVarInfo.VarType)

        if ctx.FLAGS() != None:
            if self.__CurrQestVarInfo.IsBitVar:
                ReturnCode = OObj.SetFlagsForBitField(ctx.vfrOneofFlagsField().HFlags,ctx.vfrOneofFlagsField().LFlags)
            else:
                ReturnCode = OObj.SetFlags(ctx.vfrOneofFlagsField().HFlags, ctx.vfrOneofFlagsField().LFlags)

        ShrinkSize = 0
        IsSupported = True
        if self.__CurrQestVarInfo.IsBitVar == False:
            Type = self.__CurrQestVarInfo.VarType
            # Base on the type to know the actual used size, shrink the buffer size allocate before.
            if Type == EFI_IFR_TYPE_NUM_SIZE_8:
                ShrinkSize = 21
            elif Type == EFI_IFR_TYPE_NUM_SIZE_16:
                ShrinkSize = 18
            elif Type == EFI_IFR_TYPE_NUM_SIZE_32:
                ShrinkSize = 12
            elif Type == EFI_IFR_TYPE_NUM_SIZE_64:
                ShrinkSize = 0 #
            else:
                IsSupported = False
        else:
            #　Question stored in bit fields saved as UINT32 type, so the ShrinkSize same as EFI_IFR_TYPE_NUM_SIZE_32.
            ShrinkSize = 12

        #######　NObj->ShrinkBinSize (ShrinkSize);
        if IsSupported == False:
            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
            print('OneOf question only support UINT8, UINT16, UINT32 and UINT64 data type.')

        ctx.Node.Data = OObj
        self.__IsOneOfOp = False

        '''
        print('** Form - vfrStatementQuestions - Oneof **')
        Info = ctx.Node.Data.GetInfo()
        print(Info.Question.Header.Prompt)
        print(Info.Question.Header.Help)
        print(Info.Question.QuestionId)
        print(Info.Question.VarStoreId)
        print(Info.Question.VarStoreInfo.VarName)
        print(Info.Question.VarStoreInfo.VarOffset)
        print(Info.Question.Flags)
        '''

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrOneofFlagsField.
    def visitVfrOneofFlagsField(self, ctx:VfrSyntaxParser.VfrOneofFlagsFieldContext):

        ctx.LFlags = self.__CurrQestVarInfo.VarType & EFI_IFR_NUMERIC_SIZE
        VarStoreType = gCVfrDataStorage.GetVarStoreType(self.__CurrQestVarInfo.VarStoreId)
        ctx.LineNum = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)

        for FlagsFieldCtx in ctx.numericFlagsField():
            ctx.HFlags |= FlagsFieldCtx.HFlag
            ctx.IsDisplaySpecified = FlagsFieldCtx.IsDisplaySpecified
            if FlagsFieldCtx.NumericSizeOne() != None:
                if self.__CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_1

            if FlagsFieldCtx.NumericSizeTwo() != None:
                if self.__CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_2

            if FlagsFieldCtx.NumericSizeFour() != None:
                if self.__CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_4

            if FlagsFieldCtx.NumericSizeEight() != None:
                if self.__CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_8

            if FlagsFieldCtx.DisPlayIntDec() != None:
                if self.__CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_INT_DEC
                else:
                    ctx.LFlags =  (ctx.LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_INT_DEC_BIT

            if FlagsFieldCtx.DisPlayUIntHex() != None:
                if self.__CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_UINT_DEC
                else:
                    ctx.LFlags =  (ctx.LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_UINT_DEC_BIT

            if FlagsFieldCtx.DisPlayUIntHex() != None:
                if self.__CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_UINT_HEX
                else:
                    ctx.LFlags =  (ctx.LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_UINT_HEX_BIT

        if self.__CurrQestVarInfo.IsBitVar == False:
            if self.__CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
                if VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER or VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_EFI:
                    if self.__CurrQestVarInfo.VarType != (ctx.LFlags & EFI_IFR_NUMERIC_SIZE):
                        ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                        print('Numeric Flag is not same to Numeric VarData type')
                else:
                    # update data type for name/value store
                    self.__CurrQestVarInfo.VarType = ctx.LFlags & EFI_IFR_NUMERIC_SIZE
                    Size, ReturnCode = gCVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.__CurrQestVarInfo.VarType)
                    self.__CurrQestVarInfo.VarTotalSize = Size
            elif ctx.numericFlagsField().IsSetType:
                self.__CurrQestVarInfo.VarType = ctx.LFlags & EFI_IFR_NUMERIC_SIZE

        elif self.__CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            ctx.LFlags &= EDKII_IFR_DISPLAY_BIT
            ctx.LFlags |= EDKII_IFR_NUMERIC_SIZE_BIT & self.__CurrQestVarInfo.VarTotalSize

        return ctx.HFlags, ctx.LFlags


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementStringType.
    def visitVfrStatementStringType(self, ctx:VfrSyntaxParser.VfrStatementStringTypeContext):
        self.visitChildren(ctx)
        if ctx.vfrStatementPassword() != None:
            ctx.Node = ctx.vfrStatementPassword().Node
        elif ctx.vfrStatementString() != None:
            ctx.Node = ctx.vfrStatementString().Node
        return ctx.Node

    def _GET_CURRQEST_ARRAY_SIZE(self):

        Size = 1
        if self.__CurrQestVarInfo.VarType == EFI_IFR_TYPE_NUM_SIZE_8:
            Size = 1
        if self.__CurrQestVarInfo.VarType == EFI_IFR_TYPE_NUM_SIZE_16:
            Size = 2
        if self.__CurrQestVarInfo.VarType == EFI_IFR_TYPE_NUM_SIZE_32:
            Size = 4
        if self.__CurrQestVarInfo.VarType == EFI_IFR_TYPE_NUM_SIZE_64:
            Size = 8

        return int(self.__CurrQestVarInfo.VarTotalSize / Size)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementString.
    def visitVfrStatementString(self, ctx:VfrSyntaxParser.VfrStatementStringContext):

        self.__IsStringOp = True
        SObj = ctx.OpObj
        SObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        self.__CurrentQuestion = SObj.GetQuestion()

        self.visitChildren(ctx)

        if ctx.FLAGS() != None:
            HFlags = ctx.vfrStringFlagsField().HFlags
            LFlags = ctx.vfrStringFlagsField().LFlags
            ReturnCode = SObj.SetFlags(HFlags, LFlags)

        if ctx.Key() != None:
            Key = self.__TransNum(ctx.Number(0))
            self.__AssignQuestionKey(SObj, Key)
            StringMinSize = self.__TransNum(ctx.Number(1))
            StringMaxSize = self.__TransNum(ctx.Number(2))
        else:
            StringMinSize = self.__TransNum(ctx.Number(0))
            StringMaxSize = self.__TransNum(ctx.Number(1))

        VarArraySize = self._GET_CURRQEST_ARRAY_SIZE()
        if StringMinSize > 0xFF:
            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
            print("String MinSize takes only one byte, which can't be larger than 0xFF.")
        if VarArraySize != 0 and StringMinSize > VarArraySize:
            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
            print("String MinSize can't be larger than the max number of elements in string array.")
        SObj.SetMinSize(StringMinSize)

        if StringMaxSize > 0xFF:
            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
            print("String MaxSize takes only one byte, which can't be larger than 0xFF.")
        elif VarArraySize != 0 and StringMaxSize > VarArraySize:
            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
            print("String MaxSize can't be larger than the max number of elements in string array.")
        elif StringMaxSize < StringMinSize:
            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
            print("String MaxSize can't be less than String MinSize.")
        SObj.SetMaxSize(StringMaxSize)

        ctx.Node.Data = SObj
        

        '''
        print('** Form - vfrStatementQuestions - String **')
        Info = ctx.Node.Data.GetInfo()
        print(Info.Question.Header.Prompt)
        print(Info.Question.Header.Help)
        print(Info.Question.QuestionId)
        print(Info.Question.VarStoreId)
        print(Info.Question.VarStoreInfo.VarName)
        print(Info.Question.VarStoreInfo.VarOffset)
        print(Info.Question.Flags)
        print(Info.Flags)
        print(Info.MaxSize)
        print(Info.MinSize)

        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)
        '''
        self.__IsStringOp = False

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStringFlagsField.
    def visitVfrStringFlagsField(self, ctx:VfrSyntaxParser.VfrStringFlagsFieldContext):

        self.visitChildren(ctx)
        for FlagsFieldCtx in ctx.stringFlagsField():
            ctx.HFlags |= FlagsFieldCtx.HFlag
            ctx.LFlags |= FlagsFieldCtx.LFlag

        ctx.LineNum = (None if ctx.start is None else ctx.start).line

        return ctx.HFlags, ctx.LFlags


    # Visit a parse tree produced by VfrSyntaxParser#stringFlagsField.
    def visitStringFlagsField(self, ctx:VfrSyntaxParser.StringFlagsFieldContext):

        self.visitChildren(ctx)

        if ctx.Number() != None:
            if self.__TransNum(ctx.Number()) != 0:
                ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
            else:
                ReturnCode = VfrReturnCode.VFR_RETURN_SUCCESS
        elif ctx.questionheaderFlagsField() != None:

            ctx.HFlag = ctx.questionheaderFlagsField().QHFlag
        else:
            ctx.LFlag = 0x01

        return  ctx.HFlag, ctx.LFlag


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementPassword.
    def visitVfrStatementPassword(self, ctx:VfrSyntaxParser.VfrStatementPasswordContext):

        PObj = ctx.OpObj
        PObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        self.__CurrentQuestion = PObj.GetQuestion()

        self.visitChildren(ctx)

        if ctx.Key() != None:
            Key = self.__TransNum(ctx.Number(0))
            self.__AssignQuestionKey(PObj, Key)
            PassWordMinSize = self.__TransNum(ctx.Number(1))
            PasswordMaxSize = self.__TransNum(ctx.Number(2))
        else:
            PassWordMinSize = self.__TransNum(ctx.Number(0))
            PasswordMaxSize = self.__TransNum(ctx.Number(1))

        if ctx.FLAGS() != None:
            HFlags = ctx.vfrPasswordFlagsField().HFlags
            ReturnCode = PObj.SetFlags(HFlags)

        VarArraySize = self._GET_CURRQEST_ARRAY_SIZE()
        if PassWordMinSize > 0xFF:
            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
            print("String MinSize takes only one byte, which can't be larger than 0xFF.")
        if VarArraySize != 0 and PassWordMinSize > VarArraySize:
            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
            print("String MinSize can't be larger than the max number of elements in string array.")
        PObj.SetMinSize(PassWordMinSize)

        if PasswordMaxSize > 0xFF:
            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
            print("String MaxSize takes only one byte, which can't be larger than 0xFF.")
        elif VarArraySize != 0 and PasswordMaxSize > VarArraySize:
            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
            print("String MaxSize can't be larger than the max number of elements in string array.")
        elif PasswordMaxSize < PassWordMinSize:
            ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
            print("String MaxSize can't be less than String MinSize.")
        PObj.SetMaxSize(PasswordMaxSize)

        ctx.Node.Data = PObj

        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrPasswordFlagsField.
    def visitVfrPasswordFlagsField(self, ctx:VfrSyntaxParser.VfrPasswordFlagsFieldContext):

        self.visitChildren(ctx)
        for FlagsFieldCtx in ctx.passwordFlagsField():
            ctx.HFlags |= FlagsFieldCtx.HFlag
        ctx.LineNum = (None if ctx.start is None else ctx.start).line

        return ctx.HFlags


    # Visit a parse tree produced by VfrSyntaxParser#passwordFlagsField.
    def visitPasswordFlagsField(self, ctx:VfrSyntaxParser.PasswordFlagsFieldContext):

        self.visitChildren(ctx)

        if ctx.Number() != None:
            if self.__TransNum(ctx.Number()) != 0:
                ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
            else:
                ReturnCode = VfrReturnCode.VFR_RETURN_SUCCESS
        else:
            ctx.HFlag = ctx.questionheaderFlagsField().QHFlag

        return ctx.HFlag


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementOrderedList.
    def visitVfrStatementOrderedList(self, ctx:VfrSyntaxParser.VfrStatementOrderedListContext):
        OLObj = ctx.OpObj
        OLObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        self.__CurrentQuestion = OLObj.GetQuestion()
        self.__IsOrderedList = True

        self.visitChildren(ctx)

        VarArraySize = self._GET_CURRQEST_ARRAY_SIZE()
        if VarArraySize > 0xFF:
            OLObj.SetMaxContainers(0xFF)
        else:
            OLObj.SetMaxContainers(VarArraySize)

        if ctx.MaxContainers() != None:
            MaxContainers = self.__TransNum(ctx.Number())
            if MaxContainers > 0xFF:
                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                print("OrderedList MaxContainers takes only one byte, which can't be larger than 0xFF.")
            elif VarArraySize != 0 and MaxContainers > VarArraySize:
                ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                print("OrderedList MaxContainers can't be larger than the max number of elements in array.")
            OLObj.SetMaxContainers(MaxContainers)

        ctx.Node.Data = OLObj

        '''
        print('** Form - vfrStatementQuestions - OrderedList **')
        Info = ctx.Node.Data.GetInfo()
        print(Info.Question.Header.Prompt)
        print(Info.Question.Header.Help)
        print(Info.Question.QuestionId)
        print(Info.Question.VarStoreId)
        print(Info.MaxContainers)
        print(Info.Question.Flags)
        '''


        self.__IsOrderedList = False

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrOrderedListFlags.
    def visitVfrOrderedListFlags(self, ctx:VfrSyntaxParser.VfrOrderedListFlagsContext):

        self.visitChildren(ctx)

        for FlagsFieldCtx in ctx.orderedlistFlagsField():
            ctx.HFlags |= FlagsFieldCtx.HFlag
            ctx.LFlags |= FlagsFieldCtx.LFlag

        ctx.LineNum = (None if ctx.start is None else ctx.start).line

        return ctx.HFlags, ctx.LFlags


    # Visit a parse tree produced by VfrSyntaxParser#orderedlistFlagsField.
    def visitOrderedlistFlagsField(self, ctx:VfrSyntaxParser.OrderedlistFlagsFieldContext):
        self.visitChildren(ctx)

        if ctx.Number() != None:
            if self.__TransNum(ctx.Number()) != 0:
                ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
            else:
                ReturnCode = VfrReturnCode.VFR_RETURN_SUCCESS
        elif ctx.questionheaderFlagsField() != None:
            ctx.HFlag = ctx.questionheaderFlagsField().QHFlag
        elif ctx.UniQueFlag() != None:
            ctx.LFlag = 0x01
        elif ctx.NoEmptyFlag() != None:
            ctx.LFlag = 0x02

        return  ctx.HFlag, ctx.LFlag

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementDate.
    def visitVfrStatementDate(self, ctx:VfrSyntaxParser.VfrStatementDateContext):

        DObj = ctx.OpObj
        Line = (None if ctx.start is None else ctx.start).line
        DObj.SetLineNo(Line)

        self.visitChildren(ctx)

        if ctx.vfrQuestionHeader() != None:

            if self.__CurrQestVarInfo.VarType == EFI_IFR_TYPE_OTHER:
                self.__CurrQestVarInfo.VarType == EFI_IFR_TYPE_DATE

            if ctx.FLAGS() != None:
                ReturnCode = DObj.SetFlags(EFI_IFR_QUESTION_FLAG_DEFAULT, ctx.vfrDateFlags().LFlags)

        else:

            Year = self.__TransId(ctx.StringIdentifier(0))
            Year += '.'
            Year += self.__TransId(ctx.StringIdentifier(1))

            Month = self.__TransId(ctx.StringIdentifier(2))
            Month += '.'
            Month += self.__TransId(ctx.StringIdentifier(3))

            Day = self.__TransId(ctx.StringIdentifier(4))
            Day += '.'
            Day += self.__TransId(ctx.StringIdentifier(5))

            if ctx.FLAGS() != None:
                ReturnCode = DObj.SetFlags(EFI_IFR_QUESTION_FLAG_DEFAULT, ctx.vfrDateFlags().LFlags)

            QId, ReturnCode = self.__CVfrQuestionDB.RegisterOldDateQuestion(Year, Month, Day, EFI_QUESTION_ID_INVALID)
            DObj.SetQuestionId(QId)
            DObj.SetFlags(EFI_IFR_QUESTION_FLAG_DEFAULT, QF_DATE_STORAGE_TIME)
            DObj.SetPrompt(self.__TransNum(ctx.Number(0)))
            DObj.SetHelp(self.__TransNum(ctx.Number(1)))

            # Size = OFFSET_OF (EFI_IFR_DEFAULT, Value) + sizeof (EFI_HII_DATE);
            Size = 0
            DefaultObj = CIfrDefault(Size, EFI_HII_DEFAULT_CLASS_STANDARD, EFI_IFR_TYPE_DATE, ctx.Val)
            DefaultObj.SetLineNo(Line)

        ctx.Node.Data = DObj
        for Ctx in ctx.vfrStatementInconsistentIf():
            self.InsertChild(ctx.Node, Ctx)

        '''
        print('** Form - vfrStatementQuestions - Date **')
        Info = ctx.Node.Data.GetInfo()
        print(Info.Question.QuestionId)
        print(Info.Question.VarStoreId)
        print(Info.Question.VarStoreInfo.VarName)
        print(Info.Question.VarStoreInfo.VarOffset)
        print(Info.Question.Header.Prompt)
        print(Info.Question.Header.Help)
        print(Info.Question.Flags)
        '''



        return ctx.Node

    # Visit a parse tree produced by VfrSyntaxParser#minMaxDateStepDefault.
    def visitMinMaxDateStepDefault(self, ctx:VfrSyntaxParser.MinMaxDateStepDefaultContext):


        if ctx.Default() != None:
            Minimum = self.__TransNum(ctx.Number(0))
            Maximum = self.__TransNum(ctx.Number(1))
            if ctx.KeyValue == 0:
                ctx.Date.Year = self.__TransNum(ctx.Number(len(ctx.Number())-1))
                if ctx.Date.Year < Minimum or ctx.Date.Year > Maximum:
                    ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                    print("Year default value must be between Min year and Max year.")
            if ctx.KeyValue == 1:
                ctx.Date.Month = self.__TransNum(ctx.Number(len(ctx.Number())-1))
                if ctx.Date.Month < 1 or ctx.Date.Month > 12:
                    ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                    print("Month default value must be between Min 1 and Max 12.")
            if ctx.KeyValue == 2:
                ctx.Date.Day = self.__TransNum(ctx.Number(len(ctx.Number())-1))
                if ctx.Date.Day < 1 or ctx.Date.Day > 31:
                    ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                    print("Day default value must be between Min 1 and Max 31.")

        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrDateFlags.
    def visitVfrDateFlags(self, ctx:VfrSyntaxParser.VfrDateFlagsContext):

        self.visitChildren(ctx)

        for FlagsFieldCtx in ctx.dateFlagsField():
            ctx.LFlags |= FlagsFieldCtx.LFlag

        ctx.LineNum = (None if ctx.start is None else ctx.start).line

        return ctx.LFlags


    # Visit a parse tree produced by VfrSyntaxParser#dateFlagsField.
    def visitDateFlagsField(self, ctx:VfrSyntaxParser.DateFlagsFieldContext):

        self.visitChildren(ctx)

        if ctx.Number() != None:
            ctx.LFlag = self.__TransNum(ctx.Number())
        if ctx.YearSupppressFlag() != None:
            ctx.LFlag = 0x01
        if ctx.MonthSuppressFlag() != None:
            ctx.LFlag = 0x02
        if ctx.DaySuppressFlag() != None:
            ctx.LFlag = 0x04
        if ctx.StorageNormalFlag() != None:
            ctx.LFlag = 0x00
        if ctx.StorageTimeFlag() != None:
            ctx.LFlag = 0x010
        if ctx.StorageWakeUpFlag() != None:
            ctx.LFlag = 0x20

        return ctx.LFlag


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementTime.
    def visitVfrStatementTime(self, ctx:VfrSyntaxParser.VfrStatementTimeContext):

        TObj = ctx.OpObj
        Line = (None if ctx.start is None else ctx.start).line
        TObj.SetLineNo(Line)
        self.__IsTimeOp = True

        self.visitChildren(ctx)

        if ctx.vfrQuestionHeader() != None:

            if self.__CurrQestVarInfo.VarType == EFI_IFR_TYPE_OTHER:
                self.__CurrQestVarInfo.VarType == EFI_IFR_TYPE_TIME

            if ctx.FLAGS() != None:
                ReturnCode = TObj.SetFlags(EFI_IFR_QUESTION_FLAG_DEFAULT, ctx.vfrTimeFlags().LFlags)
        else:

            Hour = self.__TransId(ctx.StringIdentifier(0))
            Hour += '.'
            Hour += self.__TransId(ctx.StringIdentifier(1))

            Minute = self.__TransId(ctx.StringIdentifier(2))
            Minute += '.'
            Minute += self.__TransId(ctx.StringIdentifier(3))

            Second = self.__TransId(ctx.StringIdentifier(4))
            Second += '.'
            Second += self.__TransId(ctx.StringIdentifier(5))

            if ctx.FLAGS() != None:
                ReturnCode = TObj.SetFlags(EFI_IFR_QUESTION_FLAG_DEFAULT, ctx.vfrTimeFlags().LFlags)

            QId, ReturnCode = self.__CVfrQuestionDB.RegisterOldTimeQuestion(Hour, Minute, Second, EFI_QUESTION_ID_INVALID)
            TObj.SetQuestionId(QId)
            TObj.SetFlags(EFI_IFR_QUESTION_FLAG_DEFAULT, QF_TIME_STORAGE_TIME)
            TObj.SetPrompt(self.__TransNum(ctx.Number(0)))
            TObj.SetHelp(self.__TransNum(ctx.Number(1)))

            # Size = OFFSET_OF (EFI_IFR_DEFAULT, Value) + sizeof (EFI_HII_TIME);
            Size = 0
            DefaultObj = CIfrDefault(Size, EFI_HII_DEFAULT_CLASS_STANDARD, EFI_IFR_TYPE_TIME, ctx.Val)
            DefaultObj.SetLineNo(Line)

        ctx.Node.Data = TObj
        self.__IsTimeOp = False
        for Ctx in ctx.vfrStatementInconsistentIf():
            self.InsertChild(ctx.Node, Ctx)

        '''
        print('** Form - vfrStatementQuestions - Time **')
        Info = ctx.Node.Data.GetInfo()
        print(Info.Question.QuestionId)
        print(Info.Question.VarStoreId)
        print(Info.Question.VarStoreInfo.VarName)
        print(Info.Question.VarStoreInfo.VarOffset)
        print(Info.Question.Header.Prompt)
        print(Info.Question.Header.Help)
        print(Info.Flags)
        '''

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#minMaxTimeStepDefault.
    def visitMinMaxTimeStepDefault(self, ctx:VfrSyntaxParser.MinMaxTimeStepDefaultContext):

        if ctx.Default() != None:
            Minimum = self.__TransNum(ctx.Number(0))
            Maximum = self.__TransNum(ctx.Number(1))
            if ctx.KeyValue == 0:
                ctx.Time.Hour = self.__TransNum(ctx.Number(len(ctx.Number())-1))
                if ctx.Time.Hour > 23:
                    ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                    print("Hour default value must be between 0 and 23.")
            if ctx.KeyValue == 1:
                ctx.Time.Minute = self.__TransNum(ctx.Number(len(ctx.Number())-1))
                if ctx.Time.Minute > 59:
                    ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                    print("Minute default value must be between 0 and 59.")
            if ctx.KeyValue == 2:
                ctx.Time.Second = self.__TransNum(ctx.Number(len(ctx.Number())-1))
                if ctx.Time.Second > 59:
                    ReturnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                    print("Second default value must be between 0 and 59.")
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrTimeFlags.
    def visitVfrTimeFlags(self, ctx:VfrSyntaxParser.VfrTimeFlagsContext):
        self.visitChildren(ctx)

        for FlagsFieldCtx in ctx.timeFlagsField():
            ctx.LFlags |= FlagsFieldCtx.LFlag
        ctx.LineNum = (None if ctx.start is None else ctx.start).line

        return ctx.LFlags

    # Visit a parse tree produced by VfrSyntaxParser#timeFlagsField.
    def visitTimeFlagsField(self, ctx:VfrSyntaxParser.TimeFlagsFieldContext):

        self.visitChildren(ctx)

        if ctx.Number() != None:
            ctx.LFlag = self.__TransNum(ctx.Number())
        if ctx.HourSupppressFlag() != None:
            ctx.LFlag = 0x01
        if ctx.MinuteSuppressFlag() != None:
            ctx.LFlag = 0x02
        if ctx.SecondSuppressFlag() != None:
            ctx.LFlag = 0x04
        if ctx.StorageNormalFlag() != None:
            ctx.LFlag = 0x00
        if ctx.StorageTimeFlag() != None:
            ctx.LFlag = 0x10
        if ctx.StorageWakeUpFlag() != None:
            ctx.LFlag = 0x20

        return ctx.LFlag

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementConditional.
    def visitVfrStatementConditional(self, ctx:VfrSyntaxParser.VfrStatementConditionalContext):
        self.visitChildren(ctx)
        if ctx.vfrStatementDisableIfStat()!= None:
            ctx.Node = ctx.vfrStatementDisableIfStat().Node
        if ctx.vfrStatementSuppressIfStat()!= None:
            ctx.Node = ctx.vfrStatementSuppressIfStat().Node
        if ctx.vfrStatementGrayOutIfStat()!= None:
            ctx.Node = ctx.vfrStatementGrayOutIfStat().Node
        if ctx.vfrStatementInconsistentIfStat()!= None:
            ctx.Node = ctx.vfrStatementInconsistentIfStat().Node


        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementConditionalNew.
    def visitVfrStatementConditionalNew(self, ctx:VfrSyntaxParser.VfrStatementConditionalNewContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementSuppressIfStat.
    def visitVfrStatementSuppressIfStat(self, ctx:VfrSyntaxParser.VfrStatementSuppressIfStatContext):
        self.visitChildren(ctx)

        ctx.Node = ctx.vfrStatementSuppressIfStatNew().Node
        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementGrayOutIfStat.
    def visitVfrStatementGrayOutIfStat(self, ctx:VfrSyntaxParser.VfrStatementGrayOutIfStatContext):
        self.visitChildren(ctx)
        ctx.Node = ctx.vfrStatementGrayOutIfStatNew().Node
        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementStatList.
    def visitVfrStatementStatList(self, ctx:VfrSyntaxParser.VfrStatementStatListContext):
        self.visitChildren(ctx)
        if ctx.vfrStatementStat() != None:
            ctx.Node = ctx.vfrStatementStat().Node
        if ctx.vfrStatementQuestions() != None:
            ctx.Node = ctx.vfrStatementQuestions().Node
        if ctx.vfrStatementConditional() != None:
            ctx.Node = ctx.vfrStatementConditional().Node
        if ctx.vfrStatementLabel() != None:
            ctx.Node = ctx.vfrStatementLabel().Node
        if ctx.vfrStatementExtension() != None:
            ctx.Node = ctx.vfrStatementExtension().Node
        if ctx.vfrStatementInvalid() != None:
            ctx.Node = ctx.vfrStatementInvalid().Node
        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementStatListOld.
    def visitVfrStatementStatListOld(self, ctx:VfrSyntaxParser.VfrStatementStatListOldContext):
        return self.visitChildren(ctx)
    
    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementDisableIfStat.
    def visitVfrStatementDisableIfStat(self, ctx:VfrSyntaxParser.VfrStatementDisableIfStatContext):
        DIObj = CIfrDisableIf()
        DIObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        ctx.Node.Data = DIObj
        ctx.Node.Condition = 'disableif' + ' ' + self.__ExtractOriginalText(ctx.vfrStatementExpression())
        self.visitChildren(ctx)
        for Ctx in ctx.vfrStatementStatList():
            self.InsertChild(ctx.Node, Ctx)

        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)
        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementSuppressIfStatNew.
    def visitVfrStatementSuppressIfStatNew(self, ctx:VfrSyntaxParser.VfrStatementSuppressIfStatNewContext):
        SIObj = CIfrSuppressIf()
        SIObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        ctx.Node.Data = SIObj
        ctx.Node.Condition = 'suppressif' + ' ' + self.__ExtractOriginalText(ctx.vfrStatementExpression())
        self.visitChildren(ctx)
        for Ctx in ctx.vfrStatementStatList():
            self.InsertChild(ctx.Node, Ctx)

        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementGrayOutIfStatNew.
    def visitVfrStatementGrayOutIfStatNew(self, ctx:VfrSyntaxParser.VfrStatementGrayOutIfStatNewContext):

        GOIObj = CIfrGrayOutIf()
        GOIObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        ctx.Node.Data = GOIObj
        # ctx.Node.Condition = 'grayoutif' + ' ' + ctx.vfrStatementExpression().Exp
        ctx.Node.Condition = 'grayoutif' + ' ' + self.__ExtractOriginalText(ctx.vfrStatementExpression())
        self.visitChildren(ctx)
        for Ctx in ctx.vfrStatementStatList():
            self.InsertChild(ctx.Node, Ctx)

        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)

        return ctx.Node



    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementInconsistentIfStat.
    def visitVfrStatementInconsistentIfStat(self, ctx:VfrSyntaxParser.VfrStatementInconsistentIfStatContext):
        IIObj = CIfrInconsistentIf()
        ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
        IIObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        self.visitChildren(ctx)
        IIObj.SetError(self.__TransNum(ctx.Number()))
        ctx.Node.Data = IIObj
        ctx.Node.Condition = 'inconsistentif' + ' ' + self.__ExtractOriginalText(ctx.vfrStatementExpression())

        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementInvalid.
    def visitVfrStatementInvalid(self, ctx:VfrSyntaxParser.VfrStatementInvalidContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementInvalidHidden.
    def visitVfrStatementInvalidHidden(self, ctx:VfrSyntaxParser.VfrStatementInvalidHiddenContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementInvalidInventory.
    def visitVfrStatementInvalidInventory(self, ctx:VfrSyntaxParser.VfrStatementInvalidInventoryContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementInvalidSaveRestoreDefaults.
    def visitVfrStatementInvalidSaveRestoreDefaults(self, ctx:VfrSyntaxParser.VfrStatementInvalidSaveRestoreDefaultsContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementLabel.
    def visitVfrStatementLabel(self, ctx:VfrSyntaxParser.VfrStatementLabelContext):
        LObj = CIfrLabel()
        self.visitChildren(ctx)
        LObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        LObj.SetNumber(self.__TransNum(ctx.Number()))
        ctx.Node.Data = LObj

        '''
        print('** Form - vfrStatementLabel **')
        Info = ctx.Node.Data.GetInfo()
        print(ctx.Node.Condition)
        print(Info.Number)
        '''
        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatemex.BObjntBanner.
    def visitVfrStatementBanner(self, ctx:VfrSyntaxParser.VfrStatementBannerContext):

        self.visitChildren(ctx)

        BObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        if ctx.Line() != None:
            BObj = CIfrBanner()
            BObj.SetLineNo((None if ctx.start is None else ctx.start).line)
            BObj.SetTitle(self.__TransNum(ctx.Number(0)))
            BObj.SetLine(self.__TransNum(ctx.Number(1)))
            if ctx.Left() != None: BObj.SetAlign(0)
            if ctx.Center() != None: BObj.SetAlign(1)
            if ctx.Right() != None: BObj.SetAlign(2)
            ctx.Node.Data = BObj
        elif ctx.Timeout() != None:
            TObj = CIfrTimeout()
            TObj.SetLineNo((None if ctx.start is None else ctx.start).line)
            TObj.SetTimeout(self.__TransNum(ctx.Number(2)))
            ctx.Node.Data = TObj

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementExtension.
    def visitVfrStatementExtension(self, ctx:VfrSyntaxParser.VfrStatementExtensionContext):

        ctx.IsStruct = False
        if ctx.DataType() != None:
            if ctx.Uint64() != None:
                ctx.TypeName = 'UINT64'
            elif ctx.Uint32() != None:
                ctx.TypeName = 'UINT32'
            elif ctx.Uint16() != None:
                ctx.TypeName = 'UINT16'
            elif ctx.Uint8() != None:
                ctx.TypeName = 'UINT8'
            elif ctx.Boolean() != None:
                ctx.TypeName = 'BOOLEAN'
            elif ctx.EFI_STRING_ID() != None:
                ctx.TypeName = 'EFI_STRING_ID'
            elif ctx.EFI_HII_DATE() != None:
                ctx.TypeName = 'EFI_HII_DATE'
                ctx.IsStruct = True
            elif ctx.EFI_HII_TIME() != None:
                ctx.TypeName = 'EFI_HII_TIME'
                ctx.IsStruct = True
            elif ctx.EFI_HII_REF() != None:
                ctx.TypeName = 'EFI_HII_REF'
                ctx.IsStruct = True
            else:
                ctx.TypeName = self.__TransId(ctx.StringIdentifier())
                ctx.IsStruct = True
            ctx.ArrayNum = self.__TransNum(ctx.Number())
            ctx.TypeSize, ReturnCode = gCVfrVarDataTypeDB.GetDataTypeSizeByTypeName(ctx.TypeName)
            ctx.Size = ctx.TypeSize * ctx.ArrayNum if ctx.ArrayNum > 0 else ctx.TypeSize

        self.visitChildren(ctx)

        Line = (None if ctx.start is None else ctx.start).line
        GuidObj = CIfrGuid(ctx.Size)
        GuidObj.SetLineNo(Line)
        GuidObj.SetGuid(ctx.guidDefinition().Guid)
        if ctx.TypeName != None:
            GuidObj.SetData(ctx.Databuff)
        # vfrStatementExtension
        GuidObj.SetScope(1)
        ctx.Node.Data = GuidObj
        for Ctx in ctx.vfrStatementExtension():
            self.InsertChild(ctx.Node, Ctx)
        # CRT_END_OP
        EObj = CIfrEnd()
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)
        return ctx.Node

    # Visit a parse tree produced by VfrSyntaxParser#vfrExtensionData.
    def visitVfrExtensionData(self, ctx:VfrSyntaxParser.VfrExtensionDataContext):
        '''
        self.visitChildren(ctx)
        IsArray = False if ctx.OpenBracket() == None else True
        ArrayIdx = 0
        if IsArray == True:
            ArrayIdx = self.__TransNum(self.__TransNum(ctx.Number(0)))
            ByteOffset  = ArrayIdx * ctx.parentCtx.TypeSize #####
            if ctx.parentCtx.IsStruct == True:
                self.__TFName += self.__mTypeName
            self.visitChildren(ctx)
            i = 0 if IsArray == False else 1
            if self.__mIsStruct == False:
                Data = self.__TransNum(ctx.Number(i))
            else:
                """             FieldOffset, FieldType, FieldSize, BitField, ReturnCode = gCVfrVarDataTypeDB.GetDataFieldInfo(self.__TFName)
                if BitField:
                    Mask = (1 << FieldSize) - 1
                    Offset = int(FieldOffset / 8)
                    PreBits = FieldOffset % 8
                    Mask <<= PreBits
                Data = self.__TransNum(ctx.Number(i)) """
                pass
                ######################## bit operation
            self.__TFName = ''
        '''
        return self.visitChildren(ctx)

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementModal.
    def visitVfrStatementModal(self, ctx:VfrSyntaxParser.VfrStatementModalContext):
        self.visitChildren(ctx)
        ctx.Node = ctx.vfrModalTag().Node
        return ctx.Node

    # Visit a parse tree produced by VfrSyntaxParser#vfrModalTag.
    def visitVfrModalTag(self, ctx:VfrSyntaxParser.VfrModalTagContext):
        MObj = CIfrModal()
        self.visitChildren(ctx)
        MObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        ctx.Node.Data = MObj

        return ctx.Node

    def __SaveOpHdrCond(self, OpHdr, Cond, LineNo=0):
        if Cond == True:
            if self.__CIfrOpHdr[self.__CIfrOpHdrIndex] != None:
                return
            self.__CIfrOpHdr[self.__CIfrOpHdrIndex] = CIfrOpHeader(OpHdr) #
            self.__CIfrOpHdrLineNo[self.__CIfrOpHdrIndex] = LineNo


    def __InitOpHdrCond(self):
        self.__CIfrOpHdr.append(None)
        self.__CIfrOpHdrLineNo.append(0)

    def __SetSavedOpHdrScope(self):
        if  self.__CIfrOpHdr[self.__CIfrOpHdrIndex] != None:
            self.__CIfrOpHdr[self.__CIfrOpHdrIndex].SetScope(1)
            return True
        return False

    def __ClearSavedOPHdr(self):
        self.__CIfrOpHdr[self.__CIfrOpHdrIndex] = None

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementExpression.
    def visitVfrStatementExpression(self, ctx:VfrSyntaxParser.VfrStatementExpressionContext):
        # Root expression extension function called by other function.
        #########################################################
        ctx.Exp = ctx.getText()
        if ctx.ExpInfo.RootLevel == 0:
            self.__CIfrOpHdrIndex += 1
            if self.__CIfrOpHdrIndex >= MAX_IFR_EXPRESSION_DEPTH:
                returnCode = VfrReturnCode.VFR_RETURN_INVALID_PARAMETER
                print('The depth of expression exceeds the max supported level 8!')
            self.__InitOpHdrCond()

        self.visitChildren(ctx)

        for SubCtx in ctx.vfrStatementExpressionSupplementary():
            ctx.ExpInfo.ExpOpCount += 1
            OObj = CIfrOr(SubCtx.Line)

        # Extend OpCode Scope only for the root expression.
        if ctx.ExpInfo.ExpOpCount > 1 and ctx.ExpInfo.RootLevel == 0:
            if self.__SetSavedOpHdrScope():
                EObj = CIfrEnd()
                if self.__CIfrOpHdrLineNo[self.__CIfrOpHdrIndex] != 0:
                    EObj.SetLineNo(self.__CIfrOpHdrLineNo[self.__CIfrOpHdrIndex])

        if ctx.ExpInfo.RootLevel == 0:
            self.__ClearSavedOPHdr()
            self.__CIfrOpHdrIndex = self.__CIfrOpHdrIndex - 1

        return ctx.ExpInfo


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementExpressionSupplementary.
    def visitVfrStatementExpressionSupplementary(self, ctx:VfrSyntaxParser.VfrStatementExpressionSupplementaryContext):
        ctx.Line = (None if ctx.start is None else ctx.start).line
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementExpressionSub.
    def visitVfrStatementExpressionSub(self, ctx:VfrSyntaxParser.VfrStatementExpressionSubContext):
        # need more test, seems right
        ctx.ExpInfo.RootLevel = ctx.parentCtx.ExpInfo.RootLevel + 1
        ctx.ExpInfo.ExpOpCount = ctx.parentCtx.ExpInfo.ExpOpCount

        self.visitChildren(ctx)

        ctx.parentCtx.ExpInfo.ExpOpCount = ctx.ExpInfo.ExpOpCount

        return ctx.ExpInfo

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementExpressionSubSupplementary.
    def visitVfrStatementExpressionSubSupplementary(self, ctx:VfrSyntaxParser.VfrStatementExpressionSubSupplementaryContext):
        ctx.Line = (None if ctx.start is None else ctx.start).line
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#andTerm.
    def visitAndTerm(self, ctx:VfrSyntaxParser.AndTermContext):
        self.visitChildren(ctx)

        for SubCtx in ctx.andTermSupplementary():
            ctx.ExpInfo.ExpOpCount += 1
            AObj = CIfrAnd(SubCtx.Line)
            ctx.CIfrAndList.append(AObj)

        return ctx.ExpInfo, ctx.CIfrAndList

    # Visit a parse tree produced by VfrSyntaxParser#andTermSupplementary.
    def visitAndTermSupplementary(self, ctx:VfrSyntaxParser.AndTermSupplementaryContext):
        ctx.Line = (None if ctx.start is None else ctx.start).line
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#bitwiseorTerm.
    def visitBitwiseorTerm(self, ctx:VfrSyntaxParser.BitwiseorTermContext):
        self.visitChildren(ctx)
        for SubCtx in ctx.bitwiseorTermSupplementary():
            ctx.ExpInfo.ExpOpCount += 1
            BWOObj = CIfrBitWiseOr(SubCtx.Line)
            ctx.CIfrBitWiseOrList.append(BWOObj)

        return ctx.ExpInfo, ctx.CIfrBitWiseOrList


    # Visit a parse tree produced by VfrSyntaxParser#bitwiseorTermSupplementary.
    def visitBitwiseorTermSupplementary(self, ctx:VfrSyntaxParser.BitwiseorTermSupplementaryContext):
        ctx.Line = (None if ctx.start is None else ctx.start).line
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#bitwiseandTerm.
    def visitBitwiseandTerm(self, ctx:VfrSyntaxParser.BitwiseandTermContext):
        self.visitChildren(ctx)
        for SubCtx in ctx.bitwiseandTermSupplementary():
            ctx.ExpInfo.ExpOpCount += 1
            BWAObj = CIfrBitWiseAnd(SubCtx.Line)
            ctx.CIfrBitWiseAndList.append(BWAObj)

        return ctx.ExpInfo, ctx.CIfrBitWiseAndList


    # Visit a parse tree produced by VfrSyntaxParser#bitwiseandTermSupplementary.
    def visitBitwiseandTermSupplementary(self, ctx:VfrSyntaxParser.BitwiseandTermSupplementaryContext):
        ctx.Line = (None if ctx.start is None else ctx.start).line
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#equalTerm.
    def visitEqualTerm(self, ctx:VfrSyntaxParser.EqualTermContext):
        self.visitChildren(ctx)
        for i in range(0, len(ctx.equalTermSupplementary())):
            ctx.ExpInfo.ExpOpCount += 1

        return ctx.ExpInfo, ctx.CIfrEqualList, ctx.CIfrNotEqualList


    # Visit a parse tree produced by VfrSyntaxParser#equalTermEqualRule.
    def visitEqualTermEqualRule(self, ctx:VfrSyntaxParser.EqualTermEqualRuleContext):
        self.visitChildren(ctx)
        ctx.Line = (None if ctx.start is None else ctx.start).line
        EObj = CIfrEqual(ctx.Line)
        ctx.CIfrEqualList.append(EObj)

        return EObj


    # Visit a parse tree produced by VfrSyntaxParser#equalTermNotEqualRule.
    def visitEqualTermNotEqualRule(self, ctx:VfrSyntaxParser.EqualTermNotEqualRuleContext):
        self.visitChildren(ctx)
        ctx.Line = (None if ctx.start is None else ctx.start).line
        NEObj = CIfrNotEqual(ctx.Line)
        ctx.CIfrNotEqualList.append(NEObj)
        return NEObj


    # Visit a parse tree produced by VfrSyntaxParser#compareTerm.
    def visitCompareTerm(self, ctx:VfrSyntaxParser.CompareTermContext):
        self.visitChildren(ctx)
        for i in range(0, len(ctx.compareTermSupplementary())):
            ctx.ExpInfo.ExpOpCount += 1
        return ctx.ExpInfo, ctx.CIfrLessThanList, ctx.CIfrLessEqualList, ctx.CIfrGreaterThanList, ctx.CIfrGreaterEqualList


    # Visit a parse tree produced by VfrSyntaxParser#compareTermLessRule.
    def visitCompareTermLessRule(self, ctx:VfrSyntaxParser.CompareTermLessRuleContext):
        self.visitChildren(ctx)
        ctx.Line = (None if ctx.start is None else ctx.start).line
        LTObj = CIfrLessThan(ctx.Line)
        ctx.CIfrLessThanList.append(LTObj)
        return LTObj


    # Visit a parse tree produced by VfrSyntaxParser#compareTermLessEqualRule.
    def visitCompareTermLessEqualRule(self, ctx:VfrSyntaxParser.CompareTermLessEqualRuleContext):
        self.visitChildren(ctx)
        ctx.Line = (None if ctx.start is None else ctx.start).line
        LEObj = CIfrLessEqual(ctx.Line)
        ctx.CIfrLessEqualList.append(LEObj)
        return LEObj


    # Visit a parse tree produced by VfrSyntaxParser#compareTermGreaterRule.
    def visitCompareTermGreaterRule(self, ctx:VfrSyntaxParser.CompareTermGreaterRuleContext):
        self.visitChildren(ctx)
        ctx.Line = (None if ctx.start is None else ctx.start).line
        GTObj = CIfrGreaterThan(ctx.Line)
        ctx.CIfrGreaterThanList.append(GTObj)
        return GTObj


    # Visit a parse tree produced by VfrSyntaxParser#compareTermGreaterEqualRule.
    def visitCompareTermGreaterEqualRule(self, ctx:VfrSyntaxParser.CompareTermGreaterEqualRuleContext):
        self.visitChildren(ctx)
        ctx.Line = (None if ctx.start is None else ctx.start).line
        GEObj = CIfrGreaterEqual(ctx.Line)
        ctx.CIfrGreaterEqualList.append(GEObj)
        return GEObj

    # Visit a parse tree produced by VfrSyntaxParser#shiftTerm.
    def visitShiftTerm(self, ctx:VfrSyntaxParser.ShiftTermContext):
        self.visitChildren(ctx)
        for i in range(0, len(ctx.shiftTermSupplementary())):
            ctx.ExpInfo.ExpOpCount += 1
        return ctx.ExpInfo, ctx.CIfrShiftLeftList, ctx.CIfrShiftRightList

    # Visit a parse tree produced by VfrSyntaxParser#shiftTermLeft.
    def visitShiftTermLeft(self, ctx:VfrSyntaxParser.ShiftTermLeftContext):
        self.visitChildren(ctx)
        ctx.Line = (None if ctx.start is None else ctx.start).line
        SLObj = CIfrShiftLeft(ctx.Line)
        ctx.CIfrShiftLeftList.append(SLObj)
        return SLObj


    # Visit a parse tree produced by VfrSyntaxParser#shiftTermRight.
    def visitShiftTermRight(self, ctx:VfrSyntaxParser.ShiftTermRightContext):
        self.visitChildren(ctx)
        ctx.Line = (None if ctx.start is None else ctx.start).line
        SRObj = CIfrShiftRight(ctx.Line)
        ctx.CIfrShiftRightList.append(SRObj)
        return SRObj


    # Visit a parse tree produced by VfrSyntaxParser#addMinusTerm.
    def visitAddMinusTerm(self, ctx:VfrSyntaxParser.AddMinusTermContext):
        self.visitChildren(ctx)
        for i in range(0, len(ctx.addMinusTermSupplementary())):
            ctx.ExpInfo.ExpOpCount += 1
        return ctx.ExpInfo, ctx.CIfrAddList, ctx.CIfrSubtractList


    # Visit a parse tree produced by VfrSyntaxParser#addMinusTermpAdd.
    def visitAddMinusTermpAdd(self, ctx:VfrSyntaxParser.AddMinusTermpAddContext):
        self.visitChildren(ctx)
        ctx.Line = (None if ctx.start is None else ctx.start).line
        AObj = CIfrAdd(ctx.Line)
        ctx.CIfrAddList.append(AObj)
        return AObj


    # Visit a parse tree produced by VfrSyntaxParser#addMinusTermSubtract.
    def visitAddMinusTermSubtract(self, ctx:VfrSyntaxParser.AddMinusTermSubtractContext):
        self.visitChildren(ctx)
        ctx.Line = (None if ctx.start is None else ctx.start).line
        SObj = CIfrSubtract(ctx.Line)
        ctx.CIfrSubtractList.append(SObj)
        return SObj


    # Visit a parse tree produced by VfrSyntaxParser#multdivmodTerm.
    def visitMultdivmodTerm(self, ctx:VfrSyntaxParser.MultdivmodTermContext):
        self.visitChildren(ctx)
        for i in range(0, len(ctx.multdivmodTermSupplementary())):
            ctx.ExpInfo.ExpOpCount += 1
        return ctx.ExpInfo, ctx.CIfrMultiplyList, ctx.CIfrDivideList,  ctx.CIfrModuloList


    # Visit a parse tree produced by VfrSyntaxParser#multdivmodTermMul.
    def visitMultdivmodTermMul(self, ctx:VfrSyntaxParser.MultdivmodTermMulContext):
        self.visitChildren(ctx)
        ctx.Line = (None if ctx.start is None else ctx.start).line
        MObj = CIfrMultiply(ctx.Line)
        ctx.CIfrMultiplyList.append(MObj)
        return MObj


    # Visit a parse tree produced by VfrSyntaxParser#multdivmodTermDiv.
    def visitMultdivmodTermDiv(self, ctx:VfrSyntaxParser.MultdivmodTermDivContext):
        self.visitChildren(ctx)
        ctx.Line = (None if ctx.start is None else ctx.start).line
        DObj = CIfrDivide(ctx.Line)
        ctx.CIfrDivideList.append(DObj)
        return DObj


    # Visit a parse tree produced by VfrSyntaxParser#multdivmodTermRound.
    def visitMultdivmodTermModulo(self, ctx:VfrSyntaxParser.MultdivmodTermModuloContext):
        self.visitChildren(ctx)
        ctx.Line = (None if ctx.start is None else ctx.start).line
        MObj = CIfrModulo(ctx.Line)
        ctx.CIfrModuloList.append(MObj)
        return MObj


    # Visit a parse tree produced by VfrSyntaxParser#castTerm.
    def visitCastTerm(self, ctx:VfrSyntaxParser.CastTermContext):
        ###################################
        self.visitChildren(ctx)
        CastType = 0xFF
        if ctx.Boolean() != []:
            CastType = 0
        elif ctx.Uint64() != []:
            CastType = 1
        elif ctx.Uint32() != []:
            CastType = 1
        elif ctx.Uint16() != []:
            CastType = 1
        elif ctx.Uint8() != []:
            CastType = 1

        Line = (None if ctx.start is None else ctx.start).line
        if CastType == 0:
            ctx.TBObj = CIfrToBoolean(Line)
        elif CastType == 1:
            ctx.TUObj = CIfrToUint(Line)

        ctx.ExpInfo.ExpOpCount += 1
        return ctx.TBObj, ctx.TUObj


    # Visit a parse tree produced by VfrSyntaxParser#atomTerm.
    def visitAtomTerm(self, ctx:VfrSyntaxParser.AtomTermContext):
        self.visitChildren(ctx)
        if ctx.NOT() != None:
            Line = (None if ctx.start is None else ctx.start).line
            NObj = CIfrNot(Line)


    # Visit a parse tree produced by VfrSyntaxParser#vfrExpressionCatenate.
    def visitVfrExpressionCatenate(self, ctx:VfrSyntaxParser.VfrExpressionCatenateContext):
        ctx.ExpInfo.RootLevel += 1
        self.visitChildren(ctx)

        Line = (None if ctx.start is None else ctx.start).line
        ctx.CObj = CIfrCatenate(Line)
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.CObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrExpressionMatch.
    def visitVfrExpressionMatch(self, ctx:VfrSyntaxParser.VfrExpressionMatchContext):
        ctx.ExpInfo.RootLevel += 1
        self.visitChildren(ctx)

        Line = (None if ctx.start is None else ctx.start).line
        ctx.MObj = CIfrMatch(Line)
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.MObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrExpressionMatch2.
    def visitVfrExpressionMatch2(self, ctx:VfrSyntaxParser.VfrExpressionMatch2Context):
        self.visitChildren(ctx)

        Line = (None if ctx.start is None else ctx.start).line
        Guid = ctx.guidDefinition().Guid
        ctx.M2Obj = CIfrMatch2(Line, Guid)
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.M2Obj


    # Visit a parse tree produced by VfrSyntaxParser#vfrExpressionParen.
    def visitVfrExpressionParen(self, ctx:VfrSyntaxParser.VfrExpressionParenContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrExpressionBuildInFunction.
    def visitVfrExpressionBuildInFunction(self, ctx:VfrSyntaxParser.VfrExpressionBuildInFunctionContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#dupExp.
    def visitDupExp(self, ctx:VfrSyntaxParser.DupExpContext):

        self.visitChildren(ctx)
        Line = (None if ctx.start is None else ctx.start).line
        DObj = CIfrDup(Line)
        self.__SaveOpHdrCond(DObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line) #
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.ExpInfo


    # Visit a parse tree produced by VfrSyntaxParser#vareqvalExp.
    def visitVareqvalExp(self, ctx:VfrSyntaxParser.VareqvalExpContext): ###########pending
        Line = (None if ctx.start is None else ctx.start).line
        ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
        VarIdStr = 'var'
        VarIdStr += str(ctx.Number(0))

        return self.visitChildren(ctx)


    def ConvertIdExpr(self, ExpInfo, LineNo, QId, VarIdStr, BitMask):
        QR1Obj = CIfrQuestionRef1(LineNo)
        QR1Obj.SetQuestionId(QId, VarIdStr, LineNo)
        self.__SaveOpHdrCond(QR1Obj.GetHeader(), (ExpInfo.ExpOpCount == 0))
        if BitMask != 0:
            U32Obj = CIfrUint32(LineNo)
            U32Obj.SetValue(BitMask)

            BWAObj = CIfrBitWiseAnd(LineNo)

            U8Obj = CIfrUint8(LineNo)
            if BitMask == DATE_YEAR_BITMASK:
                U8Obj.SetValue (0)
            elif BitMask == TIME_SECOND_BITMASK:
                U8Obj.SetValue (0x10)
            elif BitMask == DATE_DAY_BITMASK:
                U8Obj.SetValue (0x18)
            elif BitMask == TIME_HOUR_BITMASK:
                U8Obj.SetValue (0)
            elif BitMask == TIME_MINUTE_BITMASK:
                U8Obj.SetValue (0x8)

            SRObj = CIfrShiftRight(LineNo)

        ExpInfo.ExpOpCount += 4


    def IdEqValDoSpecial(self, ExpInfo, LineNo, QId, VarIdStr, BitMask, ConstVal, CompareType):
        self.ConvertIdExpr(ExpInfo, LineNo, QId, VarIdStr, BitMask)
        if ConstVal > 0xFF:
            U16Obj = CIfrUint16(LineNo)
            U16Obj.SetValue(ConstVal)
        else:
            U8Obj = CIfrUint8(LineNo)
            U8Obj.SetValue(ConstVal)


        if CompareType == EFI_COMPARE_TYPE.EQUAL:
            EObj = CIfrEqual(LineNo)

        if CompareType == EFI_COMPARE_TYPE.LESS_EQUAL:
            LEObj = CIfrLessEqual(LineNo)

        if CompareType == EFI_COMPARE_TYPE.LESS_THAN:
            LTObj = CIfrLessThan(LineNo)

        if CompareType == EFI_COMPARE_TYPE.GREATER_EQUAL:
            GEObj = CIfrGreaterEqual(LineNo)

        if CompareType == EFI_COMPARE_TYPE.GREATER_THAN:
            GTObj = CIfrGreaterThan(LineNo)

        ExpInfo.ExpOpCount += 2


    # Visit a parse tree produced by VfrSyntaxParser#ideqvalExp.
    def visitIdeqvalExp(self, ctx:VfrSyntaxParser.IdeqvalExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        Mask = ctx.vfrQuestionDataFieldName().Mask
        QId = ctx.vfrQuestionDataFieldName().QId
        VarIdStr = ctx.vfrQuestionDataFieldName().VarIdStr
        LineNo = ctx.vfrQuestionDataFieldName().Line
        ConstVal = self.__TransNum(ctx.Number())
        if ctx.Equal() != None:
            if Mask == 0:
                EIVObj = CIfrEqIdVal(Line)
                self.__SaveOpHdrCond(EIVObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
                EIVObj.SetQuestionId(QId, VarIdStr, LineNo)
                EIVObj.SetValue(ConstVal)
                ctx.ExpInfo.ExpOpCount += 1
            else:
                self.IdEqValDoSpecial(ctx.ExpInfo, Line, QId, VarIdStr, Mask, ConstVal, EFI_COMPARE_TYPE.EQUAL)

        elif ctx.LessEqual() != None:
            self.IdEqValDoSpecial(ctx.ExpInfo, Line, QId, VarIdStr, Mask, ConstVal, EFI_COMPARE_TYPE.LESS_EQUAL)

        elif ctx.Less() != None:
            self.IdEqValDoSpecial(ctx.ExpInfo, Line, QId, VarIdStr, Mask, ConstVal, EFI_COMPARE_TYPE.LESS_THAN)

        elif ctx.GreaterEqual() != None:
            self.IdEqValDoSpecial(ctx.ExpInfo, Line, QId, VarIdStr, Mask, ConstVal, EFI_COMPARE_TYPE.GREATER_EQUAL)

        elif ctx.Greater() != None:
            self.IdEqValDoSpecial(ctx.ExpInfo, Line, QId, VarIdStr, Mask, ConstVal, EFI_COMPARE_TYPE.GREATER_THAN)

        return ctx.ExpInfo


    def IdEqIdDoSpecial(self, ExpInfo, LineNo, QId1, VarIdStr1, Mask1, QId2, VarIdStr2, Mask2, CompareType):

        self.ConvertIdExpr(ExpInfo, LineNo, QId1, VarIdStr1, Mask1)
        self.ConvertIdExpr(ExpInfo, LineNo, QId2, VarIdStr2, Mask2)

        if CompareType == EFI_COMPARE_TYPE.EQUAL:
            EObj = CIfrEqual(LineNo)

        if CompareType == EFI_COMPARE_TYPE.LESS_EQUAL:
            LEObj = CIfrLessEqual(LineNo)

        if CompareType == EFI_COMPARE_TYPE.LESS_THAN:
            LTObj = CIfrLessThan(LineNo)

        if CompareType == EFI_COMPARE_TYPE.GREATER_EQUAL:
            GEObj = CIfrGreaterEqual(LineNo)

        if CompareType == EFI_COMPARE_TYPE.GREATER_THAN:
            GTObj = CIfrGreaterThan(LineNo)

        ExpInfo.ExpOpCount += 1


    # Visit a parse tree produced by VfrSyntaxParser#ideqidExp.
    def visitIdeqidExp(self, ctx:VfrSyntaxParser.IdeqidExpContext):
        self.visitChildren(ctx)
        Line = (None if ctx.start is None else ctx.start).line
        Mask1 = ctx.vfrQuestionDataFieldName(0).Mask
        QId1 = ctx.vfrQuestionDataFieldName(0).QId
        VarIdStr1 = ctx.vfrQuestionDataFieldName(0).VarIdStr
        LineNo1 = ctx.vfrQuestionDataFieldName(0).Line

        Mask2 = ctx.vfrQuestionDataFieldName(1).Mask
        QId2 = ctx.vfrQuestionDataFieldName(1).QId
        VarIdStr2 = ctx.vfrQuestionDataFieldName(1).VarIdStr
        LineNo2 = ctx.vfrQuestionDataFieldName(1).Line

        if ctx.Equal() != None:
            if Mask1 & Mask2:
                self.IdEqIdDoSpecial(ctx.ExpInfo, Line, QId1, VarIdStr1, Mask1, QId2, VarIdStr2, Mask2, EFI_COMPARE_TYPE.EQUAL)
            else:
                EIIObj = CIfrEqIdId(Line)
                self.__SaveOpHdrCond(EIIObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
                EIIObj.SetQuestionId1(QId1, VarIdStr1, LineNo1)
                EIIObj.SetQuestionId1(QId2, VarIdStr2, LineNo2)
                ctx.ExpInfo.ExpOpCount += 1

        elif ctx.LessEqual() != None:
            self.IdEqIdDoSpecial(ctx.ExpInfo, Line, QId1, VarIdStr1, Mask1, QId2, VarIdStr2, Mask2, EFI_COMPARE_TYPE.LESS_EQUAL)

        elif ctx.Less() != None:
            self.IdEqIdDoSpecial(ctx.ExpInfo, Line, QId1, VarIdStr1, Mask1, QId2, VarIdStr2, Mask2, EFI_COMPARE_TYPE.LESS_THAN)

        elif ctx.GreaterEqual() != None:
            self.IdEqIdDoSpecial(ctx.ExpInfo, Line, QId1, VarIdStr1, Mask1, QId2, VarIdStr2, Mask2, EFI_COMPARE_TYPE.GREATER_EQUAL)

        elif ctx.Greater() != None:
            self.IdEqIdDoSpecial(ctx.ExpInfo, Line, QId1, VarIdStr1, Mask1, QId2, VarIdStr2, Mask2, EFI_COMPARE_TYPE.GREATER_THAN)
        return ctx.ExpInfo


    def IdEqListDoSpecial(self, ExpInfo, LineNo, QId, VarIdStr, Mask, ListLen, ValueList):
        if ListLen == 0:
            return

        self.IdEqValDoSpecial(ExpInfo, LineNo, QId, VarIdStr, Mask, ValueList[0], EFI_COMPARE_TYPE.EQUAL)
        for i in range(1, ListLen):
            self.IdEqValDoSpecial(ExpInfo, LineNo, QId, VarIdStr, Mask, ValueList[i], EFI_COMPARE_TYPE.EQUAL)
            OObj = CIfrOr(LineNo)
            ExpInfo.ExpOpCount += 1


    # Visit a parse tree produced by VfrSyntaxParser#ideqvallistExp.
    def visitIdeqvallistExp(self, ctx:VfrSyntaxParser.IdeqvallistExpContext):
        self.visitChildren(ctx)
        '''
        Line = (None if ctx.start is None else ctx.start).line
        Mask = ctx.vfrQuestionDataFieldName().Mask
        QId = ctx.vfrQuestionDataFieldName().QId
        VarIdStr = ctx.vfrQuestionDataFieldName().VarIdStr
        LineNo = ctx.vfrQuestionDataFieldName().Line
        ValueList = []
        for i in range(0, len(ctx.Number())):
            ValueList.append(self.__TransNum(ctx.Number(i)))
        
        ListLen = len(ValueList)
        
        if Mask != 0:
            self.IdEqListDoSpecial(ctx.ExpInfo, LineNo, QId, VarIdStr, Mask, ListLen, ValueList)
        else:
            EILObj = CIfrEqIdList(Line, ListLen)
            if QId != EFI_QUESTION_ID_INVALID:
                EILObj.SetQuestionId(QId, VarIdStr, LineNo)
            EILObj.SetListLength(ListLen)  
            for i in range(0, ListLen):
                EILObj.SetValueList(i, ValueList[i])
                
            # ctypes里面的ValueList size为2,如果index大于2,如何modify ctypes struct里面的变量大小
            #EILObj.UpdateIfrBuffer() 
            
            self.__SaveOpHdrCond(EILObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            if QId == EFI_QUESTION_ID_INVALID:
                EILObj.SetQuestionId(QId, VarIdStr, LineNo)
            ctx.ExpInfo.ExpOpCount += 1
        '''
        return ctx.ExpInfo

    # Visit a parse tree produced by VfrSyntaxParser#vfrQuestionDataFieldNameRule1.
    def visitVfrQuestionDataFieldNameRule1(self, ctx:VfrSyntaxParser.VfrQuestionDataFieldNameRule1Context):
        ctx.Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        ctx.VarIdStr += self.__TransId(ctx.StringIdentifier())
        Idx = self.__TransNum(ctx.Number())
        ctx.VarIdStr += '['
        ctx.VarIdStr += str(Idx)
        ctx.VarIdStr += ']'
        ctx.QId, ctx.Mask, _ = self.__CVfrQuestionDB.GetQuestionId(None, ctx.VarIdStr)
        if self.__ConstantOnlyInExpression:
            ReturnCode = VfrReturnCode.VFR_RETURN_CONSTANT_ONLY
        return ctx.QId, ctx.Mask, ctx.VarIdStr, ctx.Line


    # Visit a parse tree produced by VfrSyntaxParser#vfrQuestionDataFieldNameRule2.
    def visitVfrQuestionDataFieldNameRule2(self, ctx:VfrSyntaxParser.VfrQuestionDataFieldNameRule2Context):
        ctx.Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        ctx.VarIdStr += self.__TransId(ctx.StringIdentifier())
        for i in range(0, len(ctx.arrayName())):
            ctx.VarIdStr += '.'
            if self.__ConstantOnlyInExpression:
                ReturnCode = VfrReturnCode.VFR_RETURN_CONSTANT_ONLY
            ctx.VarIdStr += ctx.arrayName(i).SubStr

        ctx.QId, ctx.Mask, _ = self.__CVfrQuestionDB.GetQuestionId(None, ctx.VarIdStr)
        return ctx.QId, ctx.Mask, ctx.VarIdStr, ctx.Line


    # Visit a parse tree produced by VfrSyntaxParser#arrayName.
    def visitArrayName(self, ctx:VfrSyntaxParser.ArrayNameContext):

        self.visitChildren(ctx)
        ctx.SubStr += self.__TransId(ctx.StringIdentifier())
        Idx = self.__TransNum(ctx.Number())
        if Idx > 0:
            ctx.SubStr += '['
            ctx.SubStr += str(Idx)
            ctx.SubStr += ']'

        return ctx.SubStr


    # Visit a parse tree produced by VfrSyntaxParser#questionref1Exp.
    def visitQuestionref1Exp(self, ctx:VfrSyntaxParser.Questionref1ExpContext):
        Line = (None if ctx.start is None else ctx.start).line #
        QName = None #
        QId = EFI_QUESTION_ID_INVALID
        self.visitChildren(ctx)
        if ctx.StringIdentifier() != None:
            QName = self.__TransId(ctx.StringIdentifier())
            QId, _ , _ = self.__CVfrQuestionDB.GetQuestionId(QName)

        elif ctx.Number() != None:
            QId = self.__TransNum(ctx.Number())

        QR1Obj = CIfrQuestionRef1(Line)
        self.__SaveOpHdrCond(QR1Obj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
        QR1Obj.SetQuestionId(QId, QName, Line)
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.ExpInfo


    # Visit a parse tree produced by VfrSyntaxParser#rulerefExp.
    def visitRulerefExp(self, ctx:VfrSyntaxParser.RulerefExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        RRObj = CIfrRuleRef(Line)
        self.__SaveOpHdrCond(RRObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
        RuleId = self.__CVfrRulesDB.GetRuleId(self.__TransId(ctx.StringIdentifier()))
        RRObj.SetRuleId(RuleId)
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.ExpInfo


    # Visit a parse tree produced by VfrSyntaxParser#stringref1Exp.
    def visitStringref1Exp(self, ctx:VfrSyntaxParser.Stringref1ExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        RefStringId = self.__TransNum(ctx.Number())
        SR1Obj = CIfrStringRef1(Line)
        self.__SaveOpHdrCond(SR1Obj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
        SR1Obj.SetStringId(RefStringId)
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.ExpInfo


    # Visit a parse tree produced by VfrSyntaxParser#pushthisExp.
    def visitPushthisExp(self, ctx:VfrSyntaxParser.PushthisExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        TObj = CIfrThis(Line)
        self.__SaveOpHdrCond(TObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.ExpInfo

    # Visit a parse tree produced by VfrSyntaxParser#securityExp.
    def visitSecurityExp(self, ctx:VfrSyntaxParser.SecurityExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        SObj = CIfrSecurity(Line)
        self.__SaveOpHdrCond(SObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
        SObj.SetPermissions(ctx.guidDefinition().Guid)
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.ExpInfo

    # Visit a parse tree produced by VfrSyntaxParser#numericVarStoreType.
    def visitNumericVarStoreType(self, ctx:VfrSyntaxParser.NumericVarStoreTypeContext):
        self.visitChildren(ctx)
        if ctx.NumericSizeOne() != None:
            ctx.VarType = EFI_IFR_NUMERIC_SIZE_1
        if ctx.NumericSizeTwo() != None:
            ctx.VarType = EFI_IFR_NUMERIC_SIZE_2
        if ctx.NumericSizeFour() != None:
            ctx.VarType = EFI_IFR_NUMERIC_SIZE_4
        if ctx.NumericSizeEight() != None:
            ctx.VarType = EFI_IFR_NUMERIC_SIZE_8

        return ctx.VarType


    # Visit a parse tree produced by VfrSyntaxParser#getExp.
    def visitGetExp(self, ctx:VfrSyntaxParser.GetExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        if ctx.BaseInfo.VarStoreId == 0:
            # support Date/Time question
            VarIdStr = ctx.vfrStorageVarId().VarIdStr
            QId, Mask, QType = self.__CVfrQuestionDB.GetQuestionId(None, VarIdStr, EFI_QUESION_TYPE.QUESTION_NORMAL)
            if (QId == EFI_QUESTION_ID_INVALID) or (Mask == 0) or (QType == EFI_QUESION_TYPE.QUESTION_NORMAL):
                ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
                print("Get/Set opcode can't get the enough varstore information")
            if QType == EFI_QUESION_TYPE.QUESTION_DATE:
                ctx.BaseInfo.VarType = EFI_IFR_TYPE_DATE
            elif QType == EFI_QUESION_TYPE.QUESTION_TIME:
                ctx.BaseInfo.VarType = EFI_IFR_TYPE_TIME

            if Mask == DATE_YEAR_BITMASK:
                ctx.BaseInfo.VarOffset = 0
            elif Mask == DATE_DAY_BITMASK:
                ctx.BaseInfo.VarOffset = 3
            elif Mask == TIME_HOUR_BITMASK:
                ctx.BaseInfo.VarOffset = 0
            elif Mask == TIME_MINUTE_BITMASK:
                ctx.BaseInfo.VarOffset = 1
            elif Mask == TIME_SECOND_BITMASK:
                ctx.BaseInfo.VarOffset = 2
            else:
                ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
                print("Get/Set opcode can't get the enough varstore information")

        else:
            VarType = EFI_IFR_TYPE_UNDEFINED
            if ctx.FLAGS() != None:
                VarType = ctx.numericVarStoreType().VarType

            if (gCVfrDataStorage.GetVarStoreType(ctx.BaseInfo.VarStoreId) == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_NAME) and (VarType == EFI_IFR_TYPE_UNDEFINED):
                ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
                print("Get/Set opcode don't support name string")

            if VarType != EFI_IFR_TYPE_UNDEFINED:
                ctx.BaseInfo.VarType = VarType
                Size, ReturnCode = gCVfrVarDataTypeDB.GetDataTypeSizeByDataType(ctx.BaseInfo.VarType)
                if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                    print("Get/Set opcode can't get var type size")
                ctx.BaseInfo.VarTotalSize = Size

            Size, ReturnCode = gCVfrVarDataTypeDB.GetDataTypeSizeByDataType(ctx.BaseInfo.VarType)
            if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                print("Get/Set opcode can't get var type size")

            if Size != ctx.BaseInfo.VarTotalSize:
                ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
                print("Get/Set opcode don't support data array")

        ctx.GObj = CIfrGet(Line)
        self.__SaveOpHdrCond(ctx.GObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
        ctx.GObj.SetVarInfo(ctx.BaseInfo)
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.GObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrExpressionConstant.
    def visitVfrExpressionConstant(self, ctx:VfrSyntaxParser.VfrExpressionConstantContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        if ctx.TrueSymbol() != None:
            TObj = CIfrTrue(Line)
            self.__SaveOpHdrCond(TObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            ctx.ExpInfo.ExpOpCount += 1

        if ctx.FalseSymbol() != None:
            FObj = CIfrFalse(Line)
            self.__SaveOpHdrCond(FObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            ctx.ExpInfo.ExpOpCount += 1

        if ctx.One() != None:
            OObj = CIfrOne(Line)
            self.__SaveOpHdrCond(OObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            ctx.ExpInfo.ExpOpCount += 1

        if ctx.Ones() != None:
            OObj = CIfrOnes(Line)
            self.__SaveOpHdrCond(OObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            ctx.ExpInfo.ExpOpCount += 1

        if ctx.Zero() != None:
            ZObj = CIfrZero(Line)
            self.__SaveOpHdrCond(ZObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            ctx.ExpInfo.ExpOpCount += 1

        if ctx.Undefined() != None:
            UObj = CIfrUndefined(Line)
            self.__SaveOpHdrCond(UObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            ctx.ExpInfo.ExpOpCount += 1

        if ctx.Version() != None:
            VObj = CIfrVersion(Line)
            self.__SaveOpHdrCond(VObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            ctx.ExpInfo.ExpOpCount += 1

        if ctx.Number() != None:
            U64Obj = CIfrUint64(Line)
            U64Obj.SetValue(self.__TransNum(ctx.Number()))
            self.__SaveOpHdrCond(U64Obj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            ctx.ExpInfo.ExpOpCount += 1

        return ctx.ExpInfo


    # Visit a parse tree produced by VfrSyntaxParser#vfrExpressionUnaryOp.
    def visitVfrExpressionUnaryOp(self, ctx:VfrSyntaxParser.VfrExpressionUnaryOpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#lengthExp.
    def visitLengthExp(self, ctx:VfrSyntaxParser.LengthExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        ctx.LObj = CIfrLength(Line)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.LObj

    # Visit a parse tree produced by VfrSyntaxParser#bitwisenotExp.
    def visitBitwisenotExp(self, ctx:VfrSyntaxParser.BitwisenotExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        ctx.BWNObj = CIfrBitWiseNot(Line)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.BWNObj


    # Visit a parse tree produced by VfrSyntaxParser#question23refExp.
    def visitQuestion23refExp(self, ctx:VfrSyntaxParser.Question23refExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        Type = 0x1
        DevicePath = EFI_STRING_ID_INVALID
        self.visitChildren(ctx)
        if ctx.DevicePath() != None:
            Type = 0x2
            DevicePath = self.__TransNum(ctx.Number())

        if ctx.Uuid() != None:
            Type = 0x3

        if Type == 0x1:
            QR2Obj = CIfrQuestionRef2(Line)
            self.__SaveOpHdrCond(QR2Obj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)

        if Type == 0x2:
            QR3_2Obj = CIfrQuestionRef3_2(Line)
            self.__SaveOpHdrCond(QR3_2Obj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            QR3_2Obj.SetDevicePath(DevicePath)

        if Type == 0x3:
            QR3_3Obj = CIfrQuestionRef3_3(Line)
            self.__SaveOpHdrCond(QR3_3Obj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            QR3_3Obj.SetDevicePath(DevicePath)
            QR3_3Obj.SetGuid(ctx.guidDefinition().Guid)

        ctx.ExpInfo.ExpOpCount += 1


        return ctx.ExpInfo


    # Visit a parse tree produced by VfrSyntaxParser#stringref2Exp.
    def visitStringref2Exp(self, ctx:VfrSyntaxParser.Stringref2ExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        ctx.SR2Obj = CIfrStringRef2(Line)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.SR2Obj


    # Visit a parse tree produced by VfrSyntaxParser#toboolExp.
    def visitToboolExp(self, ctx:VfrSyntaxParser.ToboolExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        ctx.TBObj = CIfrToBoolean(Line)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.TBObj

    # Visit a parse tree produced by VfrSyntaxParser#tostringExp.
    def visitTostringExp(self, ctx:VfrSyntaxParser.TostringExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        ctx.TSObj = CIfrToString(Line)
        Fmt = self.__TransNum(ctx.Number())
        ctx.TSObj.SetFormat(Fmt)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.TSObj


    # Visit a parse tree produced by VfrSyntaxParser#unintExp.
    def visitUnintExp(self, ctx:VfrSyntaxParser.UnintExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        ctx.TUObj = CIfrToUint(Line)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.TUObj


    # Visit a parse tree produced by VfrSyntaxParser#toupperExp.
    def visitToupperExp(self, ctx:VfrSyntaxParser.ToupperExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        ctx.TUObj = CIfrToUpper(Line)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.TUObj


    # Visit a parse tree produced by VfrSyntaxParser#tolwerExp.
    def visitTolwerExp(self, ctx:VfrSyntaxParser.TolwerExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        ctx.TLObj = CIfrToLower(Line)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.TLObj


    # Visit a parse tree produced by VfrSyntaxParser#setExp.
    def visitSetExp(self, ctx:VfrSyntaxParser.SetExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        if ctx.BaseInfo.VarStoreId == 0:
            # support Date/Time question
            VarIdStr = ctx.vfrStorageVarId().VarIdStr
            QId, Mask, QType = self.__CVfrQuestionDB.GetQuestionId(None, VarIdStr, EFI_QUESION_TYPE.QUESTION_NORMAL)
            if (QId == EFI_QUESTION_ID_INVALID) or (Mask == 0) or (QType == EFI_QUESION_TYPE.QUESTION_NORMAL):
                ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
                print("Get/Set opcode can't get the enough varstore information")
            if QType == EFI_QUESION_TYPE.QUESTION_DATE:
                ctx.BaseInfo.VarType = EFI_IFR_TYPE_DATE
            elif QType == EFI_QUESION_TYPE.QUESTION_TIME:
                ctx.BaseInfo.VarType = EFI_IFR_TYPE_TIME

            if Mask == DATE_YEAR_BITMASK:
                ctx.BaseInfo.VarOffset = 0
            elif Mask == DATE_DAY_BITMASK:
                ctx.BaseInfo.VarOffset = 3
            elif Mask == TIME_HOUR_BITMASK:
                ctx.BaseInfo.VarOffset = 0
            elif Mask == TIME_MINUTE_BITMASK:
                ctx.BaseInfo.VarOffset = 1
            elif Mask == TIME_SECOND_BITMASK:
                ctx.BaseInfo.VarOffset = 2
            else:
                ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
                print("Get/Set opcode can't get the enough varstore information")

        else:
            VarType = EFI_IFR_TYPE_UNDEFINED
            if ctx.FLAGS() != None:
                VarType = ctx.numericVarStoreType().VarType

            if (gCVfrDataStorage.GetVarStoreType(ctx.BaseInfo.VarStoreId) == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_NAME) and (VarType == EFI_IFR_TYPE_UNDEFINED):
                ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
                print("Get/Set opcode don't support name string")

            if VarType != EFI_IFR_TYPE_UNDEFINED:
                ctx.BaseInfo.VarType = VarType
                Size, ReturnCode = gCVfrVarDataTypeDB.GetDataTypeSizeByDataType(ctx.BaseInfo.VarType)
                if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                    print("Get/Set opcode can't get var type size")
                ctx.BaseInfo.VarTotalSize = Size

            Size, ReturnCode = gCVfrVarDataTypeDB.GetDataTypeSizeByDataType(ctx.BaseInfo.VarType)
            if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                print("Get/Set opcode can't get var type size")

            if Size != ctx.BaseInfo.VarTotalSize:
                ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
                print("Get/Set opcode don't support data array")

        ctx.TSObj = CIfrSet(Line)
        self.__SaveOpHdrCond(ctx.TSObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
        ctx.TSObj.SetVarInfo(ctx.BaseInfo)
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.TSObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrExpressionTernaryOp.
    def visitVfrExpressionTernaryOp(self, ctx:VfrSyntaxParser.VfrExpressionTernaryOpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#conditionalExp.
    def visitConditionalExp(self, ctx:VfrSyntaxParser.ConditionalExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        ctx.CObj = CIfrConditional(Line)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.CObj


    # Visit a parse tree produced by VfrSyntaxParser#findExp.
    def visitFindExp(self, ctx:VfrSyntaxParser.FindExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        FObj = CIfrFind(Line)
        Format = 0
        for i in range(0, len(ctx.findFormat())):
            Format = ctx.findFormat(i).Format

        FObj.SetFormat(Format)
        ctx.ExpInfo.ExpOpCount += 1
        return FObj


    # Visit a parse tree produced by VfrSyntaxParser#findFormat.
    def visitFindFormat(self, ctx:VfrSyntaxParser.FindFormatContext):
        self.visitChildren(ctx)
        if ctx.Sensitive() != None:
            ctx.Format = 0x00
        elif ctx.Insensitive() != None:
            ctx.Format = 0x01
        return ctx.Format


    # Visit a parse tree produced by VfrSyntaxParser#midExp.
    def visitMidExp(self, ctx:VfrSyntaxParser.MidExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        ctx.MObj = CIfrMid(Line)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.MObj

    # Visit a parse tree produced by VfrSyntaxParser#tokenExp.
    def visitTokenExp(self, ctx:VfrSyntaxParser.TokenExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        ctx.TObj = CIfrToken(Line)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.TObj


    # Visit a parse tree produced by VfrSyntaxParser#spanExp.
    def visitSpanExp(self, ctx:VfrSyntaxParser.SpanExpContext):
        Line = (None if ctx.start is None else ctx.start).line
        Flags = 0
        self.visitChildren(ctx)
        for FlagsCtx in ctx.spanFlags():
            Flags |= FlagsCtx.Flag
        ctx.SObj = CIfrSpan(Line)
        ctx.SObj.SetFlags(Flags)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.SObj


    # Visit a parse tree produced by VfrSyntaxParser#spanFlags.
    def visitSpanFlags(self, ctx:VfrSyntaxParser.SpanFlagsContext):
        self.visitChildren(ctx)
        if ctx.Number() != None:
            ctx.Flag = self.__TransNum(ctx.Number())
        elif ctx.LastNonMatch() != None:
            ctx.Flag = 0x00
        elif ctx.FirstNonMatch() != None:
            ctx.Flag = 0x01
        return ctx.Flag


    # Visit a parse tree produced by VfrSyntaxParser#vfrExpressionMap.
    def visitVfrExpressionMap(self, ctx:VfrSyntaxParser.VfrExpressionMapContext):
        Line = (None if ctx.start is None else ctx.start).line
        self.visitChildren(ctx)
        ctx.MObj = CIfrMap(Line)
        EObj = CIfrEnd()
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.MObj

    def DumpYaml(self, Root, FileName):
        with open(FileName, 'w') as f:
            self.DumpYamlDfs(Root, f)
        f.close()
    
    def DumpYamlDfs(self, Root, f):
        
        if Root.OpCode != None:
            
            if Root.OpCode == EFI_IFR_FORM_SET_OP:
                Info = Root.Data.GetInfo()
                f.write('Formset:\n')
                f.write('  Guid:  {' + '{}, {}, {},'.format('0x%x'%(Info.Guid.Data1),'0x%x'%(Info.Guid.Data2), '0x%x'%(Info.Guid.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.Guid.Data4[0]), '0x%x'%(Info.Guid.Data4[1]), '0x%x'%(Info.Guid.Data4[2]), '0x%x'%(Info.Guid.Data4[3]), \
                    '0x%x'%(Info.Guid.Data4[4]), '0x%x'%(Info.Guid.Data4[5]), '0x%x'%(Info.Guid.Data4[6]), '0x%x'%(Info.Guid.Data4[7])) + ' }}\n')
                f.write('  Title:  {}  # Title STRING_ID\n'.format(Info.FormSetTitle))
                f.write('  Help:  {}  # Help STRING_ID\n'.format(Info.Help))
                for Guid in Root.Data.GetClassGuid():
                    f.write('  ClassGuid:  {' + '{}, {}, {},'.format('0x%x'%(Guid.Data1),'0x%x'%(Guid.Data2), '0x%x'%(Guid.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Guid.Data4[0]), '0x%x'%(Guid.Data4[1]), '0x%x'%(Guid.Data4[2]), '0x%x'%(Guid.Data4[3]), \
                    '0x%x'%(Guid.Data4[4]), '0x%x'%(Guid.Data4[5]), '0x%x'%(Guid.Data4[6]), '0x%x'%(Guid.Data4[7])) + ' }}\n')
                           
            if Root.OpCode == EFI_IFR_VARSTORE_OP:
                Info = Root.Data.GetInfo()
                f.write('  - varstore:\n')
                f.write('      varid:  {}\n'.format(Info.VarStoreId))
                f.write('      name:  {}\n'.format(Info.Name))
                f.write('      size:  {}\n'.format(Info.Size))
                f.write('      guid:  {' + '{}, {}, {},'.format('0x%x'%(Info.Guid.Data1),'0x%x'%(Info.Guid.Data2), '0x%x'%(Info.Guid.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.Guid.Data4[0]), '0x%x'%(Info.Guid.Data4[1]), '0x%x'%(Info.Guid.Data4[2]), '0x%x'%(Info.Guid.Data4[3]), \
                    '0x%x'%(Info.Guid.Data4[4]), '0x%x'%(Info.Guid.Data4[5]), '0x%x'%(Info.Guid.Data4[6]), '0x%x'%(Info.Guid.Data4[7])) + ' }}\n')

            if Root.OpCode == EFI_IFR_VARSTORE_EFI_OP:
                Info = Root.Data.GetInfo()
                f.write('  - efivarstore:\n')
                f.write('      varid:  {}\n'.format(Info.VarStoreId))
                f.write('      name:  {}\n'.format(Info.Name))
                f.write('      size:  {}\n'.format(Info.Size))
                f.write('      guid:  {' + '{}, {}, {},'.format('0x%x'%(Info.Guid.Data1),'0x%x'%(Info.Guid.Data2), '0x%x'%(Info.Guid.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.Guid.Data4[0]), '0x%x'%(Info.Guid.Data4[1]), '0x%x'%(Info.Guid.Data4[2]), '0x%x'%(Info.Guid.Data4[3]), \
                    '0x%x'%(Info.Guid.Data4[4]), '0x%x'%(Info.Guid.Data4[5]), '0x%x'%(Info.Guid.Data4[6]), '0x%x'%(Info.Guid.Data4[7])) + ' }}\n')
                f.write('      attribute:  {}\n'.format(Info.Attributes))

            if Root.OpCode == EFI_IFR_VARSTORE_NAME_VALUE_OP:
                Info = Root.Data.GetInfo()
                f.write('  - namevaluevarstore:\n')
                f.write('      varid:  {}\n'.format(Info.VarStoreId))
                # f.write('      name:  {}\n'.format(Info.Name))
                f.write('      guid:  {' + '{}, {}, {},'.format('0x%x'%(Info.Guid.Data1),'0x%x'%(Info.Guid.Data2), '0x%x'%(Info.Guid.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.Guid.Data4[0]), '0x%x'%(Info.Guid.Data4[1]), '0x%x'%(Info.Guid.Data4[2]), '0x%x'%(Info.Guid.Data4[3]), \
                    '0x%x'%(Info.Guid.Data4[4]), '0x%x'%(Info.Guid.Data4[5]), '0x%x'%(Info.Guid.Data4[6]), '0x%x'%(Info.Guid.Data4[7])) + ' }}\n')
                
            if Root.OpCode == EFI_IFR_FORM_OP:
                Info = Root.Data.GetInfo()
                f.write('  - form:\n')
                if Root.Condition != None:
                    f.write('      condition:  {}\n'.format(Root.Condition))
                f.write('      FormId:  {}  # FormId STRING_ID\n'.format(Info.FormId))
                f.write('      FormTitle:  {}  # FormTitle STRING_ID\n'.format(Info.FormTitle))

            if Root.OpCode == EFI_IFR_FORM_MAP_OP:
                Info, MethodMapList = Root.Data.GetInfo()
                f.write('  - formmap:\n')
                if Root.Condition != None:
                    f.write('      condition:  {}\n'.format(Root.Condition))
                f.write('      FormId:  {}  # FormId STRING_ID\n'.format(Info.FormId))
                for MethodMap in  MethodMapList:    
                    f.write('      maptitle:  {}\n'.format(MethodMap.MethodTitle))
                    f.write('      mapguid:  {' + '{}, {}, {},'.format('0x%x'%(MethodMap.MethodIdentifier.Data1),'0x%x'%(MethodMap.MethodIdentifier.Data2), '0x%x'%(MethodMap.MethodIdentifier.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(MethodMap.MethodIdentifier.Data4[0]), '0x%x'%(MethodMap.MethodIdentifier.Data4[1]), '0x%x'%(MethodMap.MethodIdentifier.Data4[2]), '0x%x'%(MethodMap.MethodIdentifier.Data4[3]), \
                    '0x%x'%(MethodMap.MethodIdentifier.Data4[4]), '0x%x'%(MethodMap.MethodIdentifier.Data4[5]), '0x%x'%(MethodMap.MethodIdentifier.Data4[6]), '0x%x'%(MethodMap.MethodIdentifier.Data4[7])) + ' }}\n')
                    
            if Root.OpCode == EFI_IFR_SUBTITLE_OP:
                Info = Root.Data.GetInfo()
                f.write('      - subtitle:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Statement.Prompt))

            if Root.OpCode == EFI_IFR_TEXT_OP:
                Info = Root.Data.GetInfo()
                f.write('      - text:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Statement.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Statement.Help))
                f.write('          text:  {}  # Statement Help STRING_ID\n'.format(Info.TextTwo))
            
            if Root.OpCode == EFI_IFR_ACTION_OP:
                Info = Root.Data.GetInfo()  
                f.write('      - text:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.Header.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Question.Header.Help))
                f.write('          questionid:  {}  # Question QuestionId\n'.format(Info.Question.QuestionId))
                f.write('          varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                f.write('          flags:  {}  # Question Flags\n'.format(Info.Question.Flags))
                f.write('          questionconfig:  {}  # QuestionConfig\n'.format(Info.QuestionConfig))  

            if Root.OpCode == EFI_IFR_ONE_OF_OP:
                Info = Root.Data.GetInfo()  
                f.write('      - oneof:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                    
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.Header.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Question.Header.Help))
                f.write('          questionid:  {}  # Question QuestionId\n'.format(Info.Question.QuestionId))
                f.write('          varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                f.write('          varname:  {}  # Question VarName STRING_ID\n'.format(Info.Question.VarStoreInfo.VarName))
                f.write('          varoffset:  {}  # Question VarOffset\n'.format(Info.Question.VarStoreInfo.VarOffset))
                f.write('          flags:  {}  # Question Flags\n'.format(Info.Question.Flags)) 

            if Root.OpCode == EFI_IFR_ONE_OF_OPTION_OP:
                Info = Root.Data.GetInfo()  
                f.write('          - option:  {}\n'.format(Info.Option))
                if Root.Condition != None:
                    f.write('              condition:  {}\n'.format(Root.Condition))
                    
                f.write('              option flag:  {}\n'.format(Info.Flags))
                f.write('              option type:  {}\n'.format(Info.Type))
                if Info.Type == EFI_IFR_TYPE_DATE:
                    f.write('              option value:  {}/{}/{}\n'.format(Info.Value.date.Year, Info.Value.date.Month, Info.Value.date.Day))
                if Info.Type == EFI_IFR_TYPE_TIME:
                    f.write('              option value:  {}:{}:{}\n'.format(Info.Value.time.Hour, Info.Value.time.Minute, Info.Value.time.Second))
                if Info.Type == EFI_IFR_TYPE_REF:
                    f.write('              option value:  {};{};'.format(Info.Value.ref.QuestionId, Info.Value.ref.FormId) +  '{' + '{}, {}, {},'.format('0x%x'%(Info.Value.ref.FormSetGuid.Data1),'0x%x'%(Info.Value.ref.FormSetGuid.Data2), '0x%x'%(Info.Value.ref.FormSetGuid.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.Value.ref.FormSetGuid.Data4[0]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[1]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[2]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[3]), \
                    '0x%x'%(Info.Value.ref.FormSetGuid.Data4[4]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[5]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[6]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[7])) + ' }}\n' + ';{}'.format(Info.Value.ref.DevicePath))
                if Info.Type == EFI_IFR_TYPE_STRING:
                    f.write('              option value:  {}\n'.format(Info.Value.string))    
                if Info.Type == EFI_IFR_TYPE_NUM_SIZE_8:
                    f.write('              option value:  {}\n'.format(Info.Value.u8))   
                if Info.Type == EFI_IFR_TYPE_NUM_SIZE_16:
                    f.write('              option value:  {}\n'.format(Info.Value.u16))  
                if Info.Type == EFI_IFR_TYPE_NUM_SIZE_32:
                    f.write('              option value:  {}\n'.format(Info.Value.u32))  
                if Info.Type == EFI_IFR_TYPE_NUM_SIZE_64:
                    f.write('              option value:  {}\n'.format(Info.Value.u64))  
                if Info.Type == EFI_IFR_TYPE_BOOLEAN:
                    f.write('              option value:  {}\n'.format(Info.Value.b))  

            if Root.OpCode == EFI_IFR_DEFAULT_OP:
                Info = Root.Data.GetInfo()  
                f.write('          - default:\n')
                if Root.Condition != None:
                    f.write('              condition:  {}\n'.format(Root.Condition))
                if type(Root.Data) == CIfrDefault:
                    f.write('              type:  {}\n'.format(Info.Type))
                    f.write('              varstoreid:  {}\n'.format(Info.DefaultId))
                    if Info.Type == EFI_IFR_TYPE_DATE:
                        f.write('              value:  {}/{}/{}\n'.format(Info.Value.date.Year, Info.Value.date.Month, Info.Value.date.Day))
                    if Info.Type == EFI_IFR_TYPE_TIME:
                        f.write('              value:  {}:{}:{}\n'.format(Info.Value.time.Hour, Info.Value.time.Minute, Info.Value.time.Second))
                    if Info.Type == EFI_IFR_TYPE_REF:
                        f.write('              option value:  {};{};'.format(Info.Value.ref.QuestionId, Info.Value.ref.FormId) +  '{' + '{}, {}, {},'.format('0x%x'%(Info.Value.ref.FormSetGuid.Data1),'0x%x'%(Info.Value.ref.FormSetGuid.Data2), '0x%x'%(Info.Value.ref.FormSetGuid.Data3)) \
                        + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.Value.ref.FormSetGuid.Data4[0]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[1]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[2]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[3]), \
                        '0x%x'%(Info.Value.ref.FormSetGuid.Data4[4]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[5]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[6]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[7])) + ' }}' + ';{}\n'.format(Info.Value.ref.DevicePath))
                    if Info.Type == EFI_IFR_TYPE_STRING:
                        f.write('              value:  {}\n'.format(Info.Value.string))    
                    if Info.Type == EFI_IFR_TYPE_NUM_SIZE_8:
                        f.write('              value:  {}\n'.format(Info.Value.u8))   
                    if Info.Type == EFI_IFR_TYPE_NUM_SIZE_16:
                        f.write('              value:  {}\n'.format(Info.Value.u16))  
                    if Info.Type == EFI_IFR_TYPE_NUM_SIZE_32:
                        f.write('              value:  {}\n'.format(Info.Value.u32))  
                    if Info.Type == EFI_IFR_TYPE_NUM_SIZE_64:
                        f.write('              value:  {}\n'.format(Info.Value.u64))  
                    if Info.Type == EFI_IFR_TYPE_BOOLEAN:
                        f.write('              value:  {}\n'.format(Info.Value.b))  
                    '''
                    if Info.Type == EFI_IFR_TYPE_BUFFER:
                        for value in Info.Value
                        f.write('              value:  {}\n'.format(Info.Value.b)) 
                    '''
                if type(Root.Data) == CIfrDefault2:
                    f.write('              type:  {}\n'.format(Info.Type))
                    f.write('              defaultid:  {}\n'.format(Info.DefaultId))
                    

            if Root.OpCode == EFI_IFR_ORDERED_LIST_OP:
                Info = Root.Data.GetInfo()  
                f.write('      - orderedlist:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                    
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.Header.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Question.Header.Help))
                f.write('          questionid:  {}  # Question QuestionId\n'.format(Info.Question.QuestionId))
                f.write('          varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                f.write('          maxContainers:  {}\n'.format(Info.MaxContainers))
                f.write('          flags:  {}\n'.format(Info.Question.Flags)) 

            if Root.OpCode == EFI_IFR_NUMERIC_OP:
                Info = Root.Data.GetInfo()  
                f.write('      - numeric:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))    
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.Header.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Question.Header.Help))
                f.write('          questionid:  {}  # Question QuestionId\n'.format(Info.Question.QuestionId))
                f.write('          varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                f.write('          varname:  {}  # Question VarName STRING_ID\n'.format(Info.Question.VarStoreInfo.VarName))
                f.write('          varoffset:  {}  # Question VarOffset\n'.format(Info.Question.VarStoreInfo.VarOffset))
                f.write('          flags:  {}  # Question Flags\n'.format(Info.Question.Flags)) 
                
                if Root.Data.GetVarType() == EFI_IFR_TYPE_NUM_SIZE_64:
                    f.write('          maxvalue:  {}\n'.format(Info.Data.u64.MaxValue))
                    f.write('          minvalue:  {}\n'.format(Info.Data.u64.MinValue))
                    f.write('          step:  {}\n'.format(Info.Data.u64.Step))

                if Root.Data.GetVarType() == EFI_IFR_TYPE_NUM_SIZE_32:
                    f.write('          maxvalue:  {}\n'.format(Info.Data.u32.MaxValue))
                    f.write('          minvalue:  {}\n'.format(Info.Data.u32.MinValue))
                    f.write('          step:  {}\n'.format(Info.Data.u32.Step))
                    
                if Root.Data.GetVarType() == EFI_IFR_TYPE_NUM_SIZE_16:
                    f.write('          maxvalue:  {}\n'.format(Info.Data.u16.MaxValue))
                    f.write('          minvalue:  {}\n'.format(Info.Data.u16.MinValue))
                    f.write('          step:  {}\n'.format(Info.Data.u16.Step))
                    
                if Root.Data.GetVarType() == EFI_IFR_TYPE_NUM_SIZE_8:
                    f.write('          maxvalue:  {}\n'.format(Info.Data.u8.MaxValue))
                    f.write('          minvalue:  {}\n'.format(Info.Data.u8.MinValue))
                    f.write('          step:  {}\n'.format(Info.Data.u8.Step))

            if Root.OpCode == EFI_IFR_CHECKBOX_OP:
                Info = Root.Data.GetInfo()  
                f.write('      - checkbox:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.Header.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Question.Header.Help))
                f.write('          questionid:  {}  # Question QuestionId\n'.format(Info.Question.QuestionId))
                f.write('          varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                f.write('          varname:  {}  # Question VarName STRING_ID\n'.format(Info.Question.VarStoreInfo.VarName))
                f.write('          varoffset:  {}  # Question VarOffse\n'.format(Info.Question.VarStoreInfo.VarOffset))
                f.write('          flags:  {}  # Flags\n'.format(Info.Flags)) 

            if Root.OpCode == EFI_IFR_TIME_OP:
                Info = Root.Data.GetInfo()  
                f.write('      - time:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                f.write('          questionid:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.QuestionId))
                f.write('          varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                f.write('          varname:  {}\n'.format(Info.Question.VarStoreInfo.VarName))
                f.write('          varoffset:  {}\n'.format(Info.Question.VarStoreInfo.VarOffset))
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.Header.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Question.Header.Help))
                f.write('          flags:  {}\n'.format(Info.Question.Flags)) 

            if Root.OpCode == EFI_IFR_DATE_OP:
                Info = Root.Data.GetInfo()  
                f.write('      - date:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                f.write('          questionid:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.QuestionId))
                f.write('          varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                f.write('          varname:  {}\n'.format(Info.Question.VarStoreInfo.VarName))
                f.write('          varoffset:  {}\n'.format(Info.Question.VarStoreInfo.VarOffset))
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.Header.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Question.Header.Help))
                f.write('          flags:  {}\n'.format(Info.Question.Flags)) 
                
            
            if Root.OpCode == EFI_IFR_STRING_OP:
                Info = Root.Data.GetInfo()  
                f.write('      - string:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.Header.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Question.Header.Help))
                f.write('          questionid:  {}  # Question QuestionId\n'.format(Info.Question.QuestionId))
                f.write('          varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                f.write('          varname:  {}  # Question VarName STRING_ID\n'.format(Info.Question.VarStoreInfo.VarName))
                f.write('          varoffset:  {}  # Question VarOffset\n'.format(Info.Question.VarStoreInfo.VarOffset))
                f.write('          flags:  {}  # Question Flags\n'.format(Info.Question.Flags)) 
                f.write('          stringflags:  {}\n'.format(Info.Flags)) 
                f.write('          stringminsize:  {}\n'.format(Info.MinSize)) 
                f.write('          stringmaxsize:  {}\n'.format(Info.MaxSize)) 
                
            if Root.OpCode == EFI_IFR_RESET_BUTTON_OP:
                Info = Root.Data.GetInfo()  
                f.write('      - resetbutton:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Statement.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Statement.Help))
                f.write('          defaultid:  {}\n'.format(Info.DefaultId))

            if Root.OpCode == EFI_IFR_REF_OP:
                Info = Root.Data.GetInfo()  
                f.write('      - goto:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                
                if type(Root.Data) == CIfrRef4:
                    f.write('          formid:  {}\n'.format(Info.FormId))
                    f.write('          formsetid:  {' + '{}, {}, {},'.format('0x%x'%(Info.FormSetId.Data1),'0x%x'%(Info.FormSetId.Data2), '0x%x'%(Info.FormSetId.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.FormSetId.Data4[0]), '0x%x'%(Info.FormSetId.Data4[1]), '0x%x'%(Info.FormSetId.Data4[2]), '0x%x'%(Info.FormSetId.Data4[3]), \
                    '0x%x'%(Info.FormSetId.Data4[4]), '0x%x'%(Info.FormSetId.Data4[5]), '0x%x'%(Info.FormSetId.Data4[6]), '0x%x'%(Info.FormSetId.Data4[7])) + ' }}\n')
                    f.write('          questionid:  {}\n'.format(Info.Question.QuestionId))
                    f.write('          devicepath:  {}\n'.format(Info.DevicePath))

                if type(Root.Data) == CIfrRef3:
                    f.write('          formid:  {}\n'.format(Info.FormId))
                    f.write('          formsetid:  {' + '{}, {}, {},'.format('0x%x'%(Info.FormSetId.Data1),'0x%x'%(Info.FormSetId.Data2), '0x%x'%(Info.FormSetId.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.FormSetId.Data4[0]), '0x%x'%(Info.FormSetId.Data4[1]), '0x%x'%(Info.FormSetId.Data4[2]), '0x%x'%(Info.FormSetId.Data4[3]), \
                    '0x%x'%(Info.FormSetId.Data4[4]), '0x%x'%(Info.FormSetId.Data4[5]), '0x%x'%(Info.FormSetId.Data4[6]), '0x%x'%(Info.FormSetId.Data4[7])) + ' }}\n')
                    f.write('          questionid:  {}\n'.format(Info.Question.QuestionId))

                if type(Root.Data) == CIfrRef2:
                    f.write('          formid:  {}\n'.format(Info.FormId))
                    f.write('          questionid:  {}\n'.format(Info.Question.QuestionId))

                if type(Root.Data) == CIfrRef:
                    f.write('          formid:  {}\n'.format(Info.FormId))
                    f.write('          questionid:  {}\n'.format(Info.Question.QuestionId))

                f.write('          prompt:  {}\n'.format(Info.Question.Header.Prompt)) 
                f.write('          help:  {}\n'.format(Info.Question.Header.Help)) 

            if Root.OpCode == EFI_IFR_GUID_OP:
                Info = Root.Data.GetInfo() 
                if type(Root.Data) == CIfrLabel: # type(Info) == EFI_IFR_GUID_LABEL
                    f.write('      - label:\n')
                    if Root.Condition != None:
                        f.write('          condition:  {}\n'.format(Root.Condition))
                        
                    f.write('          labelnumber:  {}  # LabelNumber\n'.format(Info.Number))

                if type(Root.Data) == CIfrBanner: 
                    f.write('      - banner:\n')
                    if Root.Condition != None:
                        f.write('          condition:  {}\n'.format(Root.Condition))
                        
                    f.write('          title:  {}\n'.format(Info.Title))
                    f.write('          linenumber:  {}\n'.format(Info.LineNumber))
                    f.write('          align:  {}\n'.format(Info.Alignment))

                if type(Root.Data) == CIfrTimeout: 
                    f.write('      - banner:\n')
                    if Root.Condition != None:
                        f.write('          condition:  {}\n'.format(Root.Condition))
                        
                    f.write('          timeout:  {}\n'.format(Info.TimeOut))
                

        if Root.Child != []:
            for ChildNode in Root.Child:
                
               # if type(ChildNode.Data) != CIfrSuppressIf2:
                
                if Root.OpCode in ConditionOps:
                    if ChildNode.OpCode in ConditionOps:
                        ChildNode.Condition = Root.Condition + ' | ' + ChildNode.Condition
                    else:
                        ChildNode.Condition = Root.Condition

                self.DumpYamlDfs(ChildNode, f)
        
        return 


del VfrSyntaxParser