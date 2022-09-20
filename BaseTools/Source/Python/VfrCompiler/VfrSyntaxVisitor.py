# Generated from VfrSyntax.g4 by ANTLR 4.7.2
from cgi import print_environ_usage
from email.errors import NonPrintableDefect
from enum import Flag
from fileinput import lineno
from itertools import count
from modulefinder import STORE_NAME
from sre_parse import FLAGS
from tokenize import Number
from antlr4 import *
from CommonCtypes import *
from VfrFormPkg import *
from VfrUtility import *
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
        self.__CIfrOpHdrIndex = 0
        self.__ConstantOnlyInExpression = False
        
        self.__CVfrRulesDB = CVfrRulesDB()
        self.__CIfrOpHdr = []  # MAX_IFR_EXPRESSION_DEPTH
        self.__CIfrOpHdrLineNo = []
        self.__CurrQestVarInfo = EFI_VARSTORE_INFO()
        
        self.__CVfrQuestionDB = CVfrQuestionDB()
        
        self.__IsStringOp = False # static BOOLEAN gIsStringOp = FALSE;
        self.__IsOrderedList = True # static BOOLEAN  gIsOrderedList = FALSE;
        self.__CurrentQuestion = None
        self.__CurrentMinMaxData = None # static CIfrMinMaxStepData *gCurrentMinMaxData = NULL;

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
       # print(gCurrentQuestion)

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

        CObj = None if ctx.classDefinition() == None else ctx.classDefinition().CObj
        SubObj = None if ctx.subclassDefinition() == None else ctx.subclassDefinition().SubObj

        self.__DeclareStandardDefaultStorage(Line)

        return FSObj, CObj, SubObj

    def __ShowGuid(self, guid):
        print('data1:' + str(guid.Data1) + ' data2:' + str(guid.Data2) + ' data3:' + str(guid.Data3) + ' data4:' +
              str(guid.Data4[0]) + ' ' + str(guid.Data4[1]) + ' ' + str(guid.Data4[2]) +' ' + str(guid.Data4[3])+' ' + str(guid.Data4[4]) +' ' + str(guid.Data4[5]) +' ' + str(guid.Data4[6]) + ' ' + str(guid.Data4[7]))

    # Visit a parse tree produced by VfrSyntaxParser#classguidDefinition.
    def visitClassguidDefinition(self, ctx:VfrSyntaxParser.ClassguidDefinitionContext):

        self.visitChildren(ctx)
        for GuidCtx in ctx.guidDefinition():
            ctx.GuidList.append(GuidCtx.Guid)

        return ctx.GuidList


    # Visit a parse tree produced by VfrSyntaxParser#classDefinition.
    def visitClassDefinition(self, ctx:VfrSyntaxParser.ClassDefinitionContext):

        self.visitChildren(ctx)
        Class = 0
        for ClassNameCtx in ctx.validClassNames():
            Class |= ClassNameCtx.ClassName
        Line = (None if ctx.start is None else ctx.start).line
        ctx.CObj.SetLineNo(Line)
        ctx.CObj.SetClass(Class)

        return ctx.CObj


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

        self.visitChildren(ctx)

        Line = (None if ctx.start is None else ctx.start).line
        ctx.SubObj.SetLineNo(Line)
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

        ctx.SubObj.SetSubClass(SubClass)

        return ctx.SubObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrFormSetList.
    def visitVfrFormSetList(self, ctx:VfrSyntaxParser.VfrFormSetListContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementDefaultStore.
    def visitVfrStatementDefaultStore(self, ctx:VfrSyntaxParser.VfrStatementDefaultStoreContext):

        self.visitChildren(ctx)

        RefName = self.__TransId(ctx.StringIdentifier())
        DefaultStoreNameId = self.__TransNum(ctx.Number(0))

        DefaultId = EFI_HII_DEFAULT_CLASS_STANDARD if ctx.Attribute()== None else self.__TransNum(ctx.Number(1))

        if gCVfrDefaultStore.DefaultIdRegistered(DefaultId) == False:
            ReturnCode = gCVfrDefaultStore.RegisterDefaultStore(ctx.DSObj.GetDefaultStore(), RefName, DefaultStoreNameId, DefaultId) #
            ctx.DSObj.SetDefaultName(DefaultStoreNameId)
            ctx.DSObj.SetDefaultId (DefaultId)
            ctx.DSObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        else:
            pNode, ReturnCode = gCVfrDefaultStore.ReRegisterDefaultStoreById(DefaultId, RefName, DefaultStoreNameId)
            ctx.DSObj.SetDefaultStore = pNode.ObjAddr
            ctx.DSObj.SetLineNo((None if ctx.start is None else ctx.start).line)

        return ctx.DSObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementVarStoreLinear.
    def visitVfrStatementVarStoreLinear(self, ctx:VfrSyntaxParser.VfrStatementVarStoreLinearContext):

        self.visitChildren(ctx)

        ctx.VSObj.SetLineNo((None if ctx.start is None else ctx.start).line)

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
        ctx.VSObj.SetGuid(Guid)
        VarStoreId, ReturnCode = gCVfrDataStorage.GetVarStoreId(StoreName, Guid) # VarId _PCATCH
        ctx.VSObj.SetVarStoreId(VarStoreId)
        Size, ReturnCode = gCVfrVarDataTypeDB.GetDataTypeSizeByTypeName(TypeName)
        ctx.VSObj.SetSize = Size
        ctx.VSObj.SetName = StoreName

        return ctx.VSObj



    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementVarStoreEfi.
    def visitVfrStatementVarStoreEfi(self, ctx:VfrSyntaxParser.VfrStatementVarStoreEfiContext):

        self.visitChildren(ctx)

        ctx.VSEObj.SetLineNo((None if ctx.start is None else ctx.start).line)

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
        ctx.VSEObj.SetAttributes(Attributes)

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

        ctx.VSEObj.SetGuid(Guid)
        ctx.VSEObj.SetVarStoreId (VarStoreId)
        ctx.VSEObj.SetSize(Size) #
        ctx.VSEObj.SetName(StoreName)

        return ctx.VSEObj

    # Visit a parse tree produced by VfrSyntaxParser#vfrVarStoreEfiAttr.
    def visitVfrVarStoreEfiAttr(self, ctx:VfrSyntaxParser.VfrVarStoreEfiAttrContext):
        
        self.visitChildren(ctx)

        ctx.Attr = self.__TransNum(ctx.Number())

        return ctx.Attr

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementVarStoreNameValue.
    def visitVfrStatementVarStoreNameValue(self, ctx:VfrSyntaxParser.VfrStatementVarStoreNameValueContext):

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

        ctx.VSNVObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        ctx.VSNVObj.SetGuid(Guid)
        VarstoreId, ReturnCode = gCVfrDataStorage.GetVarStoreId(StoreName, Guid)
        ctx.VSNVObj.SetVarStoreId(VarstoreId)

        return ctx.VSNVObj

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementDisableIfFormSet.
    def visitVfrStatementDisableIfFormSet(self, ctx:VfrSyntaxParser.VfrStatementDisableIfFormSetContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementSuppressIfFormSet.
    def visitVfrStatementSuppressIfFormSet(self, ctx:VfrSyntaxParser.VfrStatementSuppressIfFormSetContext):
        return self.visitChildren(ctx)


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
            
        if ctx.QuestionId() != None:
            QId = self.__TransNum(ctx.Number())
            ReturnCode = self.__CVfrQuestionDB.FindQuestionById(QId)
            if ReturnCode != VfrReturnCode.VFR_RETURN_UNDEFINED:
                print("has already been used please used anther number") ########
        

        if ctx.QType == EFI_QUESION_TYPE.QUESTION_NORMAL:
            QId, ReturnCode = self.__CVfrQuestionDB.RegisterQuestion(QName, VarIdStr, QId)

        elif ctx.QType == EFI_QUESION_TYPE.QUESTION_DATE:
            QId, ReturnCode = self.__CVfrQuestionDB.RegisterNewDateQuestion(QName, VarIdStr, QId)

        elif ctx.QType == EFI_QUESION_TYPE.QUESTION_TIME:
            QId, ReturnCode = self.__CVfrQuestionDB.RegisterNewTimeQuestion(QName, VarIdStr, QId)

        elif ctx.QType == EFI_QUESION_TYPE.QUESTION_REF:

            if VarIdStr != '': #stand for question with storage.
                QId, ReturnCode = self.__CVfrQuestionDB.RegisterRefQuestion(QName, VarIdStr, QId)
            else:
                QId, ReturnCode = self.__CVfrQuestionDB.RegisterQuestion(QName, None, QId)
        else:
            ReturnCode = VfrReturnCode.VFR_RETURN_FATAL_ERROR

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
        SName = self.__TransId(ctx.StringIdentifier(0))
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
            ctx.Value.b = True
        elif ctx.FalseSymbol() != None:
            ctx.Value.b = False
        elif ctx.One() != None:
            ctx.Value.u8 = int(ctx.getText()) #
        elif ctx.Ones() != None:
            ctx.Value.u64 = int(ctx.getText()) #
        elif ctx.Zero() != None:
            ctx.Value.u8 = int(ctx.getText()) #
        elif ctx.Colon() != None:
            ctx.Value.time.Hour = self.__TransNum(ctx.Number(0))
            ctx.Value.time.Minute = self.__TransNum(ctx.Number(1))
            ctx.Value.time.Second = self.__TransNum(ctx.Number(2))
        elif ctx.Slash() != None:
            ctx.Value.date.Year = self.__TransNum(ctx.Number(0))
            ctx.Value.date.Month = self.__TransNum(ctx.Number(1))
            ctx.Value.date.Day = self.__TransNum(ctx.Number(2))
        elif ctx.Semicolon() != None:
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
            for i in range(0, len(ctx.Number())):
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
        else:
            Negative = True if ctx.Negative() != None else False
            # The value stored in bit fields is always set to UINT32 type.
            if self.__CurrQestVarInfo.IsBitVar:
                ctx.Value.u32 = self.__TransNum(ctx.Number())
            else:
                Type = self.__CurrQestVarInfo.VarType
                if Type == EFI_IFR_TYPE_NUM_SIZE_8:
                    ctx.Value.u8 = self.__TransNum(ctx.Number())
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
                    ctx.Value.u16 = self.__TransNum(ctx.Number())
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
                    ctx.Value.u32 = self.__TransNum(ctx.Number())
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
                    ctx.Value.u64 = self.__TransNum(ctx.Number())
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
                    ctx.Value.b = self.__TransNum(ctx.Number())

                if Type == EFI_IFR_TYPE_BOOLEAN:
                    ctx.Value.string = self.__TransNum(ctx.Number())

        return ctx.Value


    # Visit a parse tree produced by VfrSyntaxParser#vfrImageTag.
    def visitVfrImageTag(self, ctx:VfrSyntaxParser.VfrImageTagContext):
        
        self.visitChildren(ctx)

        ctx.IObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        ctx.IObj.SetImageId(self.__TransNum(ctx.Number()))
        
        return ctx.IObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrLockedTag.
    def visitVfrLockedTag(self, ctx:VfrSyntaxParser.VfrLockedTagContext):
        
        self.visitChildren(ctx)
        ctx.LObj.SetLineNo((None if ctx.start is None else ctx.start).line)

        return ctx.LObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementStatTag.
    def visitVfrStatementStatTag(self, ctx:VfrSyntaxParser.VfrStatementStatTagContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementStatTagList.
    def visitVfrStatementStatTagList(self, ctx:VfrSyntaxParser.VfrStatementStatTagListContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrFormDefinition.
    def visitVfrFormDefinition(self, ctx:VfrSyntaxParser.VfrFormDefinitionContext):

        self.visitChildren(ctx)

        ctx.FObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        FormId = self.__TransNum(ctx.Number(0))
        ctx.FObj.SetFormId(FormId)
        FormTitle  = self.__TransNum(ctx.Number(1))
        ctx.FObj.SetFormId(FormTitle)

        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)

        return ctx.FObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrFormMapDefinition.
    def visitVfrFormMapDefinition(self, ctx:VfrSyntaxParser.VfrFormMapDefinitionContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementImage.
    def visitVfrStatementImage(self, ctx:VfrSyntaxParser.VfrStatementImageContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementLocked.
    def visitVfrStatementLocked(self, ctx:VfrSyntaxParser.VfrStatementLockedContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementRules.
    def visitVfrStatementRules(self, ctx:VfrSyntaxParser.VfrStatementRulesContext):

        self.visitChildren(ctx)
        
        ctx.RObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        RuleName = self.__TransId(ctx.StringIdentifier(0))
        self.__CVfrRulesDB.RegisterRule(RuleName)
        ctx.RObj.SetRuleId(self.__CVfrRulesDB.GetRuleId(RuleName))

        # expression
        # end rule
        return ctx.RObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementStat.
    def visitVfrStatementStat(self, ctx:VfrSyntaxParser.VfrStatementStatContext):

        return self.visitChildren(ctx)


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

        EObj = CIfrEnd()
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)

        return SObj


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

        QId = EFI_QUESTION_ID_INVALID
        Help = self.__TransNum(ctx.Number(0))
        Prompt = self.__TransNum(ctx.Number(1))
        TxtTwo = self.__TransNum(ctx.Number(2)) if len(ctx.Text()) == 2 else EFI_STRING_ID_INVALID

        self.visitChildren(ctx)
        
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

            EObj = CIfrEnd()
            Line = (None if ctx.stop is None else ctx.stop).line
            EObj.SetLineNo(Line)
            return AObj, None

        else:

            TObj = CIfrText()
            Line = (None if ctx.start is None else ctx.start).line
            TObj.SetLineNo(Line)
            TObj.SetHelp(Help)
            TObj.SetPrompt(Prompt)
            TObj.SetTextTwo(TxtTwo)
            return None, TObj



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
        return self.visitChildren(ctx)


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
        ctx.OHObj = R5Obj

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
            ctx.OHObj = R4Obj

        elif ctx.FormSetGuid() != None:
            RefType = 3
            FId = self.__TransNum(ctx.Number(0))
            QId = self.__TransNum(ctx.Number(1))
            R3Obj = CIfrRef3()
            R3Obj.SetLineNo(Line)
            R3Obj.SetFormId(FId)
            R3Obj.SetQuestionId(QId)
            ctx.OpObj = R3Obj
            ctx.OHObj = R3Obj

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
            ctx.OHObj = R2Obj


        elif str(ctx.getChild(1)) == str(ctx.Number(0)):
            RefType = 1
            FId = self.__TransNum(ctx.Number(0))
            RObj = CIfrRef()
            RObj.SetLineNo(Line)
            RObj.SetFormId(FId)
            ctx.OpObj = RObj
            ctx.OHObj = RObj

        self.visitChildren(ctx)

        if self.__CurrQestVarInfo.VarType == EFI_IFR_TYPE_OTHER:
            self.__CurrQestVarInfo.VarType == EFI_IFR_TYPE_REF

        if RefType == 4 or RefType == 3:
            ctx.OpObj.SetFormSetId(ctx.guidDefinition().Guid)
            ctx.OHObj.SetFormSetId(ctx.guidDefinition().Guid)
        
        if ctx.FLAGS() != None:
            ctx.OpObj.SetFlags(ctx.vfrGotoFlags().GotoFlags)
            ctx.OHObj.SetFlags(ctx.vfrGotoFlags().GotoFlags)

        if ctx.Key() != None:
            index = int(len(ctx.Number())) - 1
            Key = self.__TransNum(ctx.Number(index))
            self.__AssignQuestionKey(ctx.OpObj, Key)

        ctx.OHObj.SetScope(1)

        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)

        return ctx.OpObj, ctx.OHObj, EObj


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

        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)


        return RBObj, EObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementQuestions.
    def visitVfrStatementQuestions(self, ctx:VfrSyntaxParser.VfrStatementQuestionsContext):


        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementQuestionTag.
    def visitVfrStatementQuestionTag(self, ctx:VfrSyntaxParser.VfrStatementQuestionTagContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementInconsistentIf.
    def visitVfrStatementInconsistentIf(self, ctx:VfrSyntaxParser.VfrStatementInconsistentIfContext):
        
        self.visitChildren(ctx)
        
        ctx.IIObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        ctx.IIObj.SetError(self.__TransNum(ctx.Number()))
        
        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)

        return ctx.IIObj, EObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementNoSubmitIf.
    def visitVfrStatementNoSubmitIf(self, ctx:VfrSyntaxParser.VfrStatementNoSubmitIfContext):
        self.visitChildren(ctx)
        
        ctx.NSIObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        ctx.NSIObj.SetError(self.__TransNum(ctx.Number()))
        
        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)
        
        return ctx.NSIObj, EObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementDisableIfQuest.
    def visitVfrStatementDisableIfQuest(self, ctx:VfrSyntaxParser.VfrStatementDisableIfQuestContext):
        self.visitChildren(ctx)
        
        ctx.DIObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        
        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)
        return ctx.DIObj, EObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementRefresh.
    def visitVfrStatementRefresh(self, ctx:VfrSyntaxParser.VfrStatementRefreshContext):
        
        self.visitChildren(ctx)
        
        ctx.RObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        ctx.RObj.SetRefreshInterval(self.__TransNum(ctx.Number()))
        
        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)
        
        return ctx.RObj, EObj

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementVarstoreDevice.
    def visitVfrStatementVarstoreDevice(self, ctx:VfrSyntaxParser.VfrStatementVarstoreDeviceContext):
        self.visitChildren(ctx)
        
        ctx.VDObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        ctx.VDObj.SetDevicePath(self.__TransNum(ctx.Number()))
        
        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)
        
        return ctx.VDObj, EObj

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementRefreshEvent.
    def visitVfrStatementRefreshEvent(self, ctx:VfrSyntaxParser.VfrStatementRefreshEventContext):
        self.visitChildren(ctx)
        
        ctx.RiObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        ctx.RiObj.SetRefreshEventGroutId(ctx.guidDefinition().Guid)
        
        return ctx.RiObj

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementWarningIf.
    def visitVfrStatementWarningIf(self, ctx:VfrSyntaxParser.VfrStatementWarningIfContext):
        self.visitChildren(ctx)
        
        ctx.WIObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        ctx.WIObj.SetWarning(self.__TransNum(ctx.Number(0)))
        ctx.WIObj.SetTimeOut(self.__TransNum(ctx.Number(1)))
        
        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)
        return ctx.WIObj, EObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementQuestionTagList.
    def visitVfrStatementQuestionTagList(self, ctx:VfrSyntaxParser.VfrStatementQuestionTagListContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementQuestionOptionTag.
    def visitVfrStatementQuestionOptionTag(self, ctx:VfrSyntaxParser.VfrStatementQuestionOptionTagContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementSuppressIfQuest.
    def visitVfrStatementSuppressIfQuest(self, ctx:VfrSyntaxParser.VfrStatementSuppressIfQuestContext):
        self.visitChildren(ctx)
        
        ctx.SIObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        
        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)
        
        return ctx.SIObj, EObj

    def OFFSET_OF(self, Type, Field):
        pass
    
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
                    Size, ReturnCode = gCVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.__CurrQestVarInfo.VarType)
                
                # Size += self.OFFSET_OF (EFI_IFR_DEFAULT, Value) ########
            ctx.DObj = CIfrDefault(Size)
            ctx.DObj.SetLineNo((None if ctx.start is None else ctx.start).line)
                
            if ctx.vfrConstantValueField().ListType:
                ctx.DObj.SetType(EFI_IFR_TYPE_BUFFER)
            elif self.__IsStringOp:
                ctx.DObj.SetType(EFI_IFR_TYPE_STRING)
            else:
                if self.__CurrQestVarInfo.IsBitVar:
                    ctx.DObj.SetType(EFI_IFR_TYPE_NUM_SIZE_32)
                else:
                    ctx.DObj.SetType(self.__CurrQestVarInfo.VarType)
                    
            if ctx.vfrConstantValueField().ListType == False:
                ctx.DObj.SetValue(Value) 
        
        else:
            IsExp = True
            ctx.DObj = CIfrDefault2()
            ctx.DObj.SetLineNo((None if ctx.start is None else ctx.start).line)
            ctx.DObj.SetScope(1)
        
        if ctx.DefaultStore() != None:
            DefaultId, ReturnCode = gCVfrDefaultStore.GetDefaultId(self.__TransId(ctx.StringIdentifier()))
            ctx.DObj.SetDefaultId(DefaultId)
        
        self.__CheckDuplicateDefaultValue(DefaultId, ctx.Default())
        if self.__CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            VarStoreName, ReturnCode = gCVfrDataStorage.GetVarStoreName(self.__CurrQestVarInfo.VarStoreId)
            VarGuid = gCVfrDataStorage.GetVarStoreGuid(self.__CurrQestVarInfo.VarStoreId)
            VarStoreType = gCVfrDataStorage.GetVarStoreType(self.__CurrQestVarInfo.VarStoreId)
            if (IsExp == False) and (VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER):
                ReturnCode = gCVfrDefaultStore.BufferVarStoreAltConfigAdd(DefaultId,self.__CurrQestVarInfo,VarStoreName,VarGuid,self.__CurrQestVarInfo.VarType, Value)
        
        return ctx.DObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementValue.
    def visitVfrStatementValue(self, ctx:VfrSyntaxParser.VfrStatementValueContext):
        self.visitChildren(ctx)
        
        ctx.VObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        
        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)
        
        return ctx.VObj, EObj

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementOptions.
    def visitVfrStatementOptions(self, ctx:VfrSyntaxParser.VfrStatementOptionsContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementOneOfOption.
    def visitVfrStatementOneOfOption(self, ctx:VfrSyntaxParser.VfrStatementOneOfOptionContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrOneOfOptionFlags.
    def visitVfrOneOfOptionFlags(self, ctx:VfrSyntaxParser.VfrOneOfOptionFlagsContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#oneofoptionFlagsField.
    def visitOneofoptionFlagsField(self, ctx:VfrSyntaxParser.OneofoptionFlagsFieldContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementRead.
    def visitVfrStatementRead(self, ctx:VfrSyntaxParser.VfrStatementReadContext):
        self.visitChildren(ctx)
        
        ctx.RObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        
        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)
        
        return ctx.RObj, EObj

    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementWrite.
    def visitVfrStatementWrite(self, ctx:VfrSyntaxParser.VfrStatementWriteContext):
        self.visitChildren(ctx)
        
        ctx.WObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        
        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)
        
        return ctx.WObj, EObj
    
    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementQuestionOptionList.
    def visitVfrStatementQuestionOptionList(self, ctx:VfrSyntaxParser.VfrStatementQuestionOptionListContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementBooleanType.
    def visitVfrStatementBooleanType(self, ctx:VfrSyntaxParser.VfrStatementBooleanTypeContext):
        return self.visitChildren(ctx)
    
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
        
        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)
        
        return CBObj


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
        
        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)
        
        return AObj


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
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxP-arser#vfrStatementNumeric.
    def visitVfrStatementNumeric(self, ctx:VfrSyntaxParser.VfrStatementNumericContext):
        
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrSetMinMaxStep.
    def visitVfrSetMinMaxStep(self, ctx:VfrSyntaxParser.VfrSetMinMaxStepContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrNumericFlags.
    def visitVfrNumericFlags(self, ctx:VfrSyntaxParser.VfrNumericFlagsContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#numericFlagsField.
    def visitNumericFlagsField(self, ctx:VfrSyntaxParser.NumericFlagsFieldContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementOneOf.
    def visitVfrStatementOneOf(self, ctx:VfrSyntaxParser.VfrStatementOneOfContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrOneofFlagsField.
    def visitVfrOneofFlagsField(self, ctx:VfrSyntaxParser.VfrOneofFlagsFieldContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementStringType.
    def visitVfrStatementStringType(self, ctx:VfrSyntaxParser.VfrStatementStringTypeContext):
        return self.visitChildren(ctx)

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
        self.__CurrentQuestion = SObj.GetQuestion
        
        self.visitChildren(ctx)

        if ctx.Key() != None:
            Key = self.__TransNum(ctx.Number(0))
            self.__AssignQuestionKey(SObj, Key)
            StringMinSize = self.__TransNum(ctx.Number(1))
            StringMaxSize = self.__TransNum(ctx.Number(2))
        else:
            StringMinSize = self.__TransNum(ctx.Number(0))
            StringMaxSize = self.__TransNum(ctx.Number(1))
        
        if ctx.FLAGS() != None:
            HFlags = ctx.vfrStringFlagsField().HFlags
            LFlags = ctx.vfrStringFlagsField().LFlags
            ReturnCode = SObj.SetFlags(HFlags, LFlags)
        
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
        
        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)
        self.__IsStringOp = False
        
       
        
        return SObj, EObj


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
        self.__CurrentQuestion = PObj.GetQuestion
        
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
        
        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)
        
        return PObj, EObj


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
        self.__CurrentQuestion = OLObj.GetQuestion
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
        
        EObj = CIfrEnd() #
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)    
        
        self.__IsOrderedList = False
        
        return OLObj, EObj


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
            print(Year)
            
            Month = self.__TransId(ctx.StringIdentifier(2))
            Month += '.'
            Month += self.__TransId(ctx.StringIdentifier(3))
            print(Month)
            Day = self.__TransId(ctx.StringIdentifier(4))
            Day += '.'
            Day += self.__TransId(ctx.StringIdentifier(5))
            print(Day)
            
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
            
        return DObj

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
        
        return TObj


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
            ctx.LFlag = 0x010
        if ctx.StorageWakeUpFlag() != None:
            ctx.LFlag = 0x20
        
        return ctx.LFlag 


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementConditional.
    def visitVfrStatementConditional(self, ctx:VfrSyntaxParser.VfrStatementConditionalContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementStatList.
    def visitVfrStatementStatList(self, ctx:VfrSyntaxParser.VfrStatementStatListContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementDisableIfStat.
    def visitVfrStatementDisableIfStat(self, ctx:VfrSyntaxParser.VfrStatementDisableIfStatContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementSuppressIfStat.
    def visitVfrStatementSuppressIfStat(self, ctx:VfrSyntaxParser.VfrStatementSuppressIfStatContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementGrayOutIfStat.
    def visitVfrStatementGrayOutIfStat(self, ctx:VfrSyntaxParser.VfrStatementGrayOutIfStatContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementLabel.
    def visitVfrStatementLabel(self, ctx:VfrSyntaxParser.VfrStatementLabelContext):
        
        self.visitChildren(ctx)
        ctx.LObj.SetLineNo((None if ctx.start is None else ctx.start).line)
        ctx.LObj.SetNumber(self.__TransNum(ctx.Number()))

        return ctx.LObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementBanner.
    def visitVfrStatementBanner(self, ctx:VfrSyntaxParser.VfrStatementBannerContext):
        
        self.visitChildren(ctx)

        ctx.BObj.SetLineNo((None if ctx.start is None else ctx.start).line)

        ctx.BObj.SetTitle(self.__TransNum(ctx.Number(0)))
        ctx.BObj.SetLine(self.__TransNum(ctx.Number(1)))

        if ctx.Left() != None: ctx.BObj.SetAlign(0)
        if ctx.Center() != None: ctx.BObj.SetAlign(1)
        if ctx.Right() != None: ctx.BObj.SetAlign(2)

        ctx.TObj.SetTimeout(self.__TransNum(ctx.Number(2)))

        return ctx.BObj, ctx.TObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementExtension.
    def visitVfrStatementExtension(self, ctx:VfrSyntaxParser.VfrStatementExtensionContext):

        self.__mDataBuff = ''
        self.__mIsStruct = False
        if ctx.DataType() != None:

            if ctx.Uint64() != None:
                self.__mTypeName = 'UINT64'
            elif ctx.Uint32() != None:
                self.__mTypeName = 'UINT32'
            elif ctx.Uint16() != None:
                self.__mTypeName = 'UINT16'
            elif ctx.Uint8() != None:
                self.__mTypeName = 'UINT8'
            elif ctx.Boolean() != None:
                self.__mTypeName = 'BOOLEAN'
            elif ctx.EFI_STRING_ID() != None:
                self.__mTypeName = 'EFI_STRING_ID'
            elif ctx.EFI_HII_DATE() != None:
                self.__mTypeName = 'EFI_HII_DATE'
                self.__mIsStruct = True
            elif ctx.EFI_HII_TIME() != None:
                self.__mTypeName = 'EFI_HII_TIME'
                self.__mIsStruct = True
            elif ctx.EFI_HII_REF() != None:
                self.__mTypeName = 'EFI_HII_REF'
                self.__mIsStruct = True
            else:
                self.__mTypeName = self.__TransId(ctx.StringIdentifier())
                self.__mIsStruct = True

            self.__mArrayNum = self.__TransNum(ctx.Number())

            self.__mTypeSize, ReutrnCode = gCVfrVarDataTypeDB.GetDataTypeSizeByTypeName(self.__mTypeName)

            self.__mSize = self.__mTypeSize * self.__mArrayNum if self.__mArrayNum > 0 else self.__mTypeSize
            if self.__mSize > 128 - ctypes.sizeof(EFI_IFR_GUID):
                return
            for i in range(0, self.__mSize):
                self.__mDataBuff += '0'

        self.visitChildren(ctx)

        # print(self.__mTypeSize)

        Line = (None if ctx.start is None else ctx.start).line
        #########################关于setData这部分 size的必要性
        GuidObj = CIfrGuid(self.__mSize)
        if GuidObj != None:
            GuidObj.SetLineNo(Line)
            GuidObj.SetGuid(ctx.guidDefinition().Guid)
        if self.__mTypeName != None:
            pass  #########Setdata and Databuff
        # vfrStatementExtension
        GuidObj.SetScope(1)

        # CRT_END_OP
        EObj = CIfrEnd()
        Line = (None if ctx.stop is None else ctx.stop).line
        EObj.SetLineNo(Line)

        return GuidObj, EObj


    # Visit a parse tree produced by VfrSyntaxParser#vfrExtensionData.
    def visitVfrExtensionData(self, ctx:VfrSyntaxParser.VfrExtensionDataContext):
        self.__TFName = ''
        self.visitChildren(ctx)
        return

    # Visit a parse tree produced by VfrSyntaxParser#vfrExtensionDataComponent.
    def visitVfrExtensionDataComponent(self, ctx:VfrSyntaxParser.VfrExtensionDataComponentContext): # wip
        IsArray = False
        ArrayIdx = 0
        if len(ctx.Number()) == 2:
            IsArray = True

        if IsArray == True:
            ArrayIdx = self.__TransNum(self.__TransNum(ctx.Number(0)))
            if ArrayIdx >= self.__mArrayNum:
                return ############

        ByteOffset  = ArrayIdx * self.__mTypeSize #####

        if self.__mIsStruct == True:
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



        return None


    # Visit a parse tree produced by VfrSyntaxParser#vfrExtensionDataDotArea.
    def visitVfrExtensionDataDotArea(self, ctx:VfrSyntaxParser.VfrExtensionDataDotAreaContext):

        if self.__mIsStruct == True:

            self.__TFName += '.'
            self.__TFName += self.__TransId(ctx.StringIdentifier())

            Num = self.__TransNum(ctx.Number())

            if Num != None:
                self.__TFName += '['
                self.__TFName += str(Num)
                self.__TFName += ']'


        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementModal.
    def visitVfrStatementModal(self, ctx:VfrSyntaxParser.VfrStatementModalContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrStatementExpression.
    def visitVfrStatementExpression(self, ctx:VfrSyntaxParser.VfrStatementExpressionContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#andTerm.
    def visitAndTerm(self, ctx:VfrSyntaxParser.AndTermContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#bitwiseorTerm.
    def visitBitwiseorTerm(self, ctx:VfrSyntaxParser.BitwiseorTermContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#bitwiseandTerm.
    def visitBitwiseandTerm(self, ctx:VfrSyntaxParser.BitwiseandTermContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#equalTerm.
    def visitEqualTerm(self, ctx:VfrSyntaxParser.EqualTermContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#compareTerm.
    def visitCompareTerm(self, ctx:VfrSyntaxParser.CompareTermContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#shiftTerm.
    def visitShiftTerm(self, ctx:VfrSyntaxParser.ShiftTermContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#addMinusTerm.
    def visitAddMinusTerm(self, ctx:VfrSyntaxParser.AddMinusTermContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#multdivmodTerm.
    def visitMultdivmodTerm(self, ctx:VfrSyntaxParser.MultdivmodTermContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#castTerm.
    def visitCastTerm(self, ctx:VfrSyntaxParser.CastTermContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#atomTerm.
    def visitAtomTerm(self, ctx:VfrSyntaxParser.AtomTermContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrExpressionCatenate.
    def visitVfrExpressionCatenate(self, ctx:VfrSyntaxParser.VfrExpressionCatenateContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrExpressionMatch.
    def visitVfrExpressionMatch(self, ctx:VfrSyntaxParser.VfrExpressionMatchContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrExpressionParen.
    def visitVfrExpressionParen(self, ctx:VfrSyntaxParser.VfrExpressionParenContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrExpressionBuildInFunction.
    def visitVfrExpressionBuildInFunction(self, ctx:VfrSyntaxParser.VfrExpressionBuildInFunctionContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#dupExp.
    def visitDupExp(self, ctx:VfrSyntaxParser.DupExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#ideqvalExp.
    def visitIdeqvalExp(self, ctx:VfrSyntaxParser.IdeqvalExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#ideqidExp.
    def visitIdeqidExp(self, ctx:VfrSyntaxParser.IdeqidExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#ideqvallistExp.
    def visitIdeqvallistExp(self, ctx:VfrSyntaxParser.IdeqvallistExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrQuestionDataFieldName.
    def visitVfrQuestionDataFieldName(self, ctx:VfrSyntaxParser.VfrQuestionDataFieldNameContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#questionref1Exp.
    def visitQuestionref1Exp(self, ctx:VfrSyntaxParser.Questionref1ExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#rulerefExp.
    def visitRulerefExp(self, ctx:VfrSyntaxParser.RulerefExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#stringref1Exp.
    def visitStringref1Exp(self, ctx:VfrSyntaxParser.Stringref1ExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#pushthisExp.
    def visitPushthisExp(self, ctx:VfrSyntaxParser.PushthisExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#securityExp.
    def visitSecurityExp(self, ctx:VfrSyntaxParser.SecurityExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#getExp.
    def visitGetExp(self, ctx:VfrSyntaxParser.GetExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrExpressionConstant.
    def visitVfrExpressionConstant(self, ctx:VfrSyntaxParser.VfrExpressionConstantContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrExpressionUnaryOp.
    def visitVfrExpressionUnaryOp(self, ctx:VfrSyntaxParser.VfrExpressionUnaryOpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#lengthExp.
    def visitLengthExp(self, ctx:VfrSyntaxParser.LengthExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#bitwisenotExp.
    def visitBitwisenotExp(self, ctx:VfrSyntaxParser.BitwisenotExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#question23refExp.
    def visitQuestion23refExp(self, ctx:VfrSyntaxParser.Question23refExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#stringref2Exp.
    def visitStringref2Exp(self, ctx:VfrSyntaxParser.Stringref2ExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#toboolExp.
    def visitToboolExp(self, ctx:VfrSyntaxParser.ToboolExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#tostringExp.
    def visitTostringExp(self, ctx:VfrSyntaxParser.TostringExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#unintExp.
    def visitUnintExp(self, ctx:VfrSyntaxParser.UnintExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#toupperExp.
    def visitToupperExp(self, ctx:VfrSyntaxParser.ToupperExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#tolwerExp.
    def visitTolwerExp(self, ctx:VfrSyntaxParser.TolwerExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#setExp.
    def visitSetExp(self, ctx:VfrSyntaxParser.SetExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrExpressionTernaryOp.
    def visitVfrExpressionTernaryOp(self, ctx:VfrSyntaxParser.VfrExpressionTernaryOpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#conditionalExp.
    def visitConditionalExp(self, ctx:VfrSyntaxParser.ConditionalExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#findExp.
    def visitFindExp(self, ctx:VfrSyntaxParser.FindExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#findFormat.
    def visitFindFormat(self, ctx:VfrSyntaxParser.FindFormatContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#midExp.
    def visitMidExp(self, ctx:VfrSyntaxParser.MidExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#tokenExp.
    def visitTokenExp(self, ctx:VfrSyntaxParser.TokenExpContext):
        return self.visitChildren(ctx)

    # Visit a parse tree produced by VfrSyntaxParser#spanExp.
    def visitSpanExp(self, ctx:VfrSyntaxParser.SpanExpContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#spanFlags.
    def visitSpanFlags(self, ctx:VfrSyntaxParser.SpanFlagsContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrExpressionMap.
    def visitVfrExpressionMap(self, ctx:VfrSyntaxParser.VfrExpressionMapContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by VfrSyntaxParser#vfrExpressionMatch2.
    def visitVfrExpressionMatch2(self, ctx:VfrSyntaxParser.VfrExpressionMatch2Context):
        return self.visitChildren(ctx)



del VfrSyntaxParser