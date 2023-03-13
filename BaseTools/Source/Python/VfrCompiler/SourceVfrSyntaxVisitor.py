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
from VfrCtypes import *
from VfrFormPkg import *
from VfrUtility import *
from VfrTree import *
from VfrPreProcess import *
import ctypes
import struct

if __name__ is not None and "." in __name__:
    from .SourceVfrSyntaxParser import SourceVfrSyntaxParser
else:
    from SourceVfrSyntaxParser import SourceVfrSyntaxParser

# This class defines a complete generic visitor for a parse tree produced by SourceVfrSyntaxParser.
class SourceVfrSyntaxVisitor(ParseTreeVisitor):

    def __init__(self, PreProcessDB: PreProcessDB = None, Root=None, OverrideClassGuid=None):

        self.Root = Root if Root != None else VfrTreeNode()
        self.PreProcessDB = PreProcessDB
        self.OverrideClassGuid = OverrideClassGuid
        self.ParserStatus = 0

        self.Value = None
        self.LastFormNode = None
        self.IfrOpHdrIndex = 0
        self.ConstantOnlyInExpression = False
        self.IfrOpHdr = [None]
        self.IfrOpHdrLineNo = [0]
        self.UsedDefaultArray = []

        self.VfrRulesDB = VfrRulesDB()
        self.CurrQestVarInfo = EFI_VARSTORE_INFO()

        self.VfrQuestionDB = VfrQuestionDB()
        self.CurrentQuestion = None
        self.CurrentMinMaxData = None

        self.IsStringOp = False
        self.IsOrderedList = False
        self.IsCheckBoxOp = False
        self.NeedAdjustOpcode = False


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrProgram.
    def visitVfrProgram(self, ctx:SourceVfrSyntaxParser.VfrProgramContext):
        #self.VfrQuestionDB.PrintAllQuestion('Questions.txt')
        return self.visitChildren(ctx)

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrHeader.
    def visitVfrHeader(self, ctx:SourceVfrSyntaxParser.VfrHeaderContext):
        self.visitChildren(ctx)
        return gVfrVarDataTypeDB

    # Visit a parse tree produced by SourceVfrSyntaxParser#pragmaPackShowDef.
    def visitPragmaPackShowDef(self, ctx:SourceVfrSyntaxParser.PragmaPackShowDefContext):

        Line = ctx.start.line
        gVfrVarDataTypeDB.Pack(Line, VFR_PACK_SHOW)
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#pragmaPackStackDef.
    def visitPragmaPackStackDef(self, ctx:SourceVfrSyntaxParser.PragmaPackStackDefContext):

        Identifier = self.TransId(ctx.StringIdentifier())

        if str(ctx.getChild(0)) == 'push':
            Action = VFR_PACK_PUSH
        else:
            Action = VFR_PACK_POP

        if ctx.Number() != None:
            Action |= VFR_PACK_ASSIGN

        PackNumber = self.TransNum(ctx.Number(), DEFAULT_PACK_ALIGN)
        Line = ctx.start.line
        gVfrVarDataTypeDB.Pack(Line, Action, Identifier, PackNumber)
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#pragmaPackNumber.
    def visitPragmaPackNumber(self, ctx:SourceVfrSyntaxParser.PragmaPackNumberContext):

        Line = ctx.start.line
        PackNumber = self.TransNum(ctx.Number(), DEFAULT_PACK_ALIGN)

        gVfrVarDataTypeDB.Pack(Line, VFR_PACK_ASSIGN, None, PackNumber)
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrPragmaPackDefinition.
    def visitVfrPragmaPackDefinition(
            self, ctx: SourceVfrSyntaxParser.VfrPragmaPackDefinitionContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrDataStructDefinition.
    def visitVfrDataStructDefinition(self, ctx: SourceVfrSyntaxParser.VfrDataStructDefinitionContext):

        gVfrVarDataTypeDB.DeclareDataTypeBegin()

        if ctx.N1 != None:
            ReturnCode = gVfrVarDataTypeDB.SetNewTypeName(ctx.N1.text)
            self.ErrorHandler(ReturnCode, ctx.N1.line, ctx.N1.text)

        if ctx.N2 != None:
            ReturnCode = gVfrVarDataTypeDB.SetNewTypeName(ctx.N2.text)
            self.ErrorHandler(ReturnCode, ctx.N2.line, ctx.N2.text)

        self.visitChildren(ctx)

        gVfrVarDataTypeDB.DeclareDataTypeEnd()

        return None


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrDataUnionDefinition.
    def visitVfrDataUnionDefinition(self, ctx:SourceVfrSyntaxParser.VfrDataUnionDefinitionContext):
        gVfrVarDataTypeDB.DeclareDataTypeBegin()
        if ctx.N1 != None:
            ReturnCode = gVfrVarDataTypeDB.SetNewTypeName(ctx.N1.text)
            self.ErrorHandler(ReturnCode, ctx.N1.line, ctx.N1.text)

        if ctx.N2 != None:
            ReturnCode = gVfrVarDataTypeDB.SetNewTypeName(ctx.N2.text)
            self.ErrorHandler(ReturnCode, ctx.N2.line, ctx.N2.text)

        self.visitChildren(ctx)

        gVfrVarDataTypeDB.DeclareDataTypeEnd()
        return None


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrDataStructFields.
    def visitVfrDataStructFields(self, ctx:SourceVfrSyntaxParser.VfrDataStructFieldsContext):

        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#dataStructField64.
    def visitDataStructField64(self, ctx:SourceVfrSyntaxParser.DataStructField64Context):
        ArrayNum = self.TransNum(ctx.Number())
        ReturnCode = gVfrVarDataTypeDB.DataTypeAddField(ctx.N.text, 'UINT64', ArrayNum, ctx.FieldInUnion)
        self.ErrorHandler(ReturnCode, ctx.N.line, ctx.N.text)
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#dataStructField32.
    def visitDataStructField32(self, ctx:SourceVfrSyntaxParser.DataStructField32Context):
        ArrayNum = self.TransNum(ctx.Number())
        ReturnCode = gVfrVarDataTypeDB.DataTypeAddField(ctx.N.text, 'UINT32', ArrayNum, ctx.FieldInUnion)
        self.ErrorHandler(ReturnCode, ctx.N.line, ctx.N.text)
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#dataStructField16.
    def visitDataStructField16(self, ctx:SourceVfrSyntaxParser.DataStructField16Context):
        ArrayNum = self.TransNum(ctx.Number())
        ReturnCode = gVfrVarDataTypeDB.DataTypeAddField(ctx.N.text, 'UINT16', ArrayNum, ctx.FieldInUnion)
        self.ErrorHandler(ReturnCode, ctx.N.line, ctx.N.text)
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#dataStructField8.
    def visitDataStructField8(self, ctx:SourceVfrSyntaxParser.DataStructField8Context):
        ArrayNum = self.TransNum(ctx.Number())
        ReturnCode = gVfrVarDataTypeDB.DataTypeAddField(ctx.N.text, 'UINT8', ArrayNum, ctx.FieldInUnion)
        self.ErrorHandler(ReturnCode, ctx.N.line, ctx.N.text)
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#dataStructFieldBool.
    def visitDataStructFieldBool(self, ctx:SourceVfrSyntaxParser.DataStructFieldBoolContext):
        ArrayNum = self.TransNum(ctx.Number())
        ReturnCode = gVfrVarDataTypeDB.DataTypeAddField(ctx.N.text, 'BOOLEAN', ArrayNum, ctx.FieldInUnion)
        self.ErrorHandler(ReturnCode, ctx.N.line, ctx.N.text)
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#dataStructFieldString.
    def visitDataStructFieldString(self, ctx:SourceVfrSyntaxParser.DataStructFieldStringContext):
        ArrayNum = self.TransNum(ctx.Number())
        ReturnCode = gVfrVarDataTypeDB.DataTypeAddField(ctx.N.text, 'EFI_STRING_ID', ArrayNum, ctx.FieldInUnion)
        self.ErrorHandler(ReturnCode, ctx.N.line, ctx.N.text)
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#dataStructFieldDate.
    def visitDataStructFieldDate(self, ctx:SourceVfrSyntaxParser.DataStructFieldDateContext):
        ArrayNum = self.TransNum(ctx.Number())
        ReturnCode = gVfrVarDataTypeDB.DataTypeAddField(ctx.N.text, 'EFI_HII_DATE', ArrayNum, ctx.FieldInUnion)
        self.ErrorHandler(ReturnCode, ctx.N.line, ctx.N.text)
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#dataStructFieldTime.
    def visitDataStructFieldTime(self, ctx:SourceVfrSyntaxParser.DataStructFieldTimeContext):
        ArrayNum = self.TransNum(ctx.Number())
        ReturnCode = gVfrVarDataTypeDB.DataTypeAddField(ctx.N.text, 'EFI_HII_TIME', ArrayNum, ctx.FieldInUnion)
        self.ErrorHandler(ReturnCode, ctx.N.line, ctx.N.text)
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#dataStructFieldRef.
    def visitDataStructFieldRef(self, ctx:SourceVfrSyntaxParser.DataStructFieldRefContext):
        ArrayNum = self.TransNum(ctx.Number())
        ReturnCode = gVfrVarDataTypeDB.DataTypeAddField(ctx.N.text, 'EFI_HII_REF', ArrayNum, ctx.FieldInUnion)
        self.ErrorHandler(ReturnCode, ctx.N.line, ctx.N.text)
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#dataStructFieldUser.
    def visitDataStructFieldUser(self, ctx:SourceVfrSyntaxParser.DataStructFieldUserContext):
        ArrayNum = self.TransNum(ctx.Number())
        if ctx.T.text != 'CHAR16':
            ReturnCode = gVfrVarDataTypeDB.DataTypeAddField(ctx.N.text, ctx.T.text, ArrayNum, ctx.FieldInUnion)
        else:
            ReturnCode = gVfrVarDataTypeDB.DataTypeAddField(ctx.N.text, 'UINT16', ArrayNum, ctx.FieldInUnion)
        self.ErrorHandler(ReturnCode, ctx.N.line, ctx.N.text)
        return self.visitChildren(ctx)

    # Visit a parse tree produced by SourceVfrSyntaxParser#dataStructBitField64.
    def visitDataStructBitField64(self, ctx:SourceVfrSyntaxParser.DataStructBitField64Context):
        Width = self.TransNum(ctx.Number())
        if ctx.N != None:
            self.ErrorHandler(gVfrVarDataTypeDB.DataTypeAddBitField(ctx.N.text, 'UINT64', Width, ctx.FieldInUnion), ctx.N.line, ctx.N.text)
        else:
            self.ErrorHandler(gVfrVarDataTypeDB.DataTypeAddBitField(None, 'UINT64', Width, ctx.FieldInUnion), ctx.D.line, ctx.D.text)

        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#dataStructBitField32.
    def visitDataStructBitField32(self, ctx:SourceVfrSyntaxParser.DataStructBitField32Context):
        Width = self.TransNum(ctx.Number())
        if ctx.N != None:
            self.ErrorHandler(gVfrVarDataTypeDB.DataTypeAddBitField(ctx.N.text, 'UINT32', Width, ctx.FieldInUnion), ctx.N.line, ctx.N.text)
        else:
            self.ErrorHandler(gVfrVarDataTypeDB.DataTypeAddBitField(None, 'UINT32', Width, ctx.FieldInUnion), ctx.D.line, ctx.D.text)
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#dataStructBitField16.
    def visitDataStructBitField16(self, ctx:SourceVfrSyntaxParser.DataStructBitField16Context):
        Width = self.TransNum(ctx.Number())
        if ctx.N != None:
            self.ErrorHandler(gVfrVarDataTypeDB.DataTypeAddBitField(ctx.N.text, 'UINT16', Width, ctx.FieldInUnion), ctx.N.line, ctx.N.text)
        else:
            self.ErrorHandler(gVfrVarDataTypeDB.DataTypeAddBitField(None, 'UINT16', Width, ctx.FieldInUnion), ctx.D.line, ctx.D.text)
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#dataStructBitField8.
    def visitDataStructBitField8(self, ctx:SourceVfrSyntaxParser.DataStructBitField8Context):
        Width = self.TransNum(ctx.Number())
        if ctx.N != None:
            self.ErrorHandler(gVfrVarDataTypeDB.DataTypeAddBitField(ctx.N.text, 'UINT8', Width, ctx.FieldInUnion), ctx.N.line, ctx.N.text)
        else:
            self.ErrorHandler(gVfrVarDataTypeDB.DataTypeAddBitField(None, 'UINT8', Width, ctx.FieldInUnion), ctx.D.line, ctx.D.text)
        return self.visitChildren(ctx)

    def DeclareStandardDefaultStorage(self, LineNo):

        DSObj = IfrDefaultStore("Standard Defaults")
        gVfrDefaultStore.RegisterDefaultStore(DSObj.DefaultStore, "Standard Defaults", EFI_STRING_ID_INVALID, EFI_HII_DEFAULT_CLASS_STANDARD)
        DSObj.SetLineNo (LineNo)
        DSObj.SetDefaultName (EFI_STRING_ID_INVALID)
        DSObj.SetDefaultId (EFI_HII_DEFAULT_CLASS_STANDARD)
        DsNode = VfrTreeNode(EFI_IFR_DEFAULTSTORE_OP, DSObj, gFormPkg.StructToStream(DSObj.GetInfo()))
        DSObjMF = IfrDefaultStore("Standard ManuFacturing")
        gVfrDefaultStore.RegisterDefaultStore(DSObjMF.DefaultStore, "Standard ManuFacturing", EFI_STRING_ID_INVALID, EFI_HII_DEFAULT_CLASS_MANUFACTURING)
        DSObjMF.SetLineNo (LineNo)
        DSObjMF.SetDefaultName (EFI_STRING_ID_INVALID)
        DSObjMF.SetDefaultId (EFI_HII_DEFAULT_CLASS_MANUFACTURING)
        DsNodeMF = VfrTreeNode(EFI_IFR_DEFAULTSTORE_OP, DSObjMF, gFormPkg.StructToStream(DSObjMF.GetInfo()))

        return DsNode, DsNodeMF


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrFormSetDefinition.
    def visitVfrFormSetDefinition(self, ctx:SourceVfrSyntaxParser.VfrFormSetDefinitionContext):
        self.InsertChild(self.Root, ctx)
        self.InsertChild(ctx.Node, ctx.classDefinition())
        self.InsertChild(ctx.Node, ctx.subclassDefinition())
        DsNode, DsNodeMF = self.DeclareStandardDefaultStorage(ctx.start.line)
        ctx.Node.insertChild(DsNode)
        ctx.Node.insertChild(DsNodeMF)

        self.visitChildren(ctx)

        ClassGuidNum = 0
        GuidList = []
        if ctx.classguidDefinition() != None:
            GuidList = ctx.classguidDefinition().GuidList
            ClassGuidNum = len(GuidList)

        DefaultClassGuid = EFI_HII_PLATFORM_SETUP_FORMSET_GUID

        if (self.OverrideClassGuid != None and ClassGuidNum >= 4):
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.start.line, None, 'Already has 4 class guids, can not add extra class guid!')

        if ClassGuidNum == 0:
            if self.OverrideClassGuid != None:
                ClassGuidNum  = 2
            else:
                ClassGuidNum  = 1
            FSObj = IfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID))
            FSObj.SetClassGuid(DefaultClassGuid)
            if (self.OverrideClassGuid != None):
                FSObj.SetClassGuid(self.OverrideClassGuid)

        elif ClassGuidNum == 1:
            if self.OverrideClassGuid != None:
                ClassGuidNum  += 1
            FSObj = IfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID))
            FSObj.SetClassGuid(GuidList[0])
            if (self.OverrideClassGuid != None):
                FSObj.SetClassGuid(self.OverrideClassGuid)

        elif ClassGuidNum == 2:
            if self.OverrideClassGuid != None:
                ClassGuidNum += 1
            FSObj = IfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID))
            FSObj.SetClassGuid(GuidList[0])
            FSObj.SetClassGuid(GuidList[1])
            if (self.OverrideClassGuid != None):
                FSObj.SetClassGuid(self.OverrideClassGuid)

        elif ClassGuidNum == 3:
            if self.OverrideClassGuid != None:
                ClassGuidNum  += 1
            FSObj = IfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID))
            FSObj.SetClassGuid(GuidList[0])
            FSObj.SetClassGuid(GuidList[1])
            FSObj.SetClassGuid(GuidList[2])
            if (self.OverrideClassGuid != None):
                FSObj.SetClassGuid(self.OverrideClassGuid)

        elif ClassGuidNum == 4:
            if self.OverrideClassGuid != None:
                ClassGuidNum  += 1
            FSObj = IfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID))
            FSObj.SetClassGuid(GuidList[0])
            FSObj.SetClassGuid(GuidList[1])
            FSObj.SetClassGuid(GuidList[2])
            FSObj.SetClassGuid(GuidList[3])

        Guid = self.PreProcessDB.Read(ctx.S1.text)
        ctx.Node.Dict['guid'] = KV(ctx.S1.text, Guid)
        Title = self.PreProcessDB.Read(ctx.S2.text)
        ctx.Node.Dict['title'] = KV(ctx.S2.text, Title)
        Help = self.PreProcessDB.Read(ctx.S3.text)
        ctx.Node.Dict['help'] = KV(ctx.S3.text, Help)

        FSObj.SetLineNo(ctx.start.line)
        FSObj.SetGuid(Guid)
        FSObj.SetFormSetTitle(Title)
        FSObj.SetHelp(Help)

        ctx.Node.Data = FSObj
        ctx.Node.Buffer = gFormPkg.StructToStream(FSObj.GetInfo())
        for i in range(0, len(GuidList)):
            ctx.Node.Buffer += gFormPkg.StructToStream(GuidList[i])

        # Declare undefined Question so that they can be used in expression.
        InsertOpCodeList = None
        if gFormPkg.HavePendingUnassigned():
            InsertOpCodeList, ReturnCode = gFormPkg.DeclarePendingQuestion(
                gVfrVarDataTypeDB, gVfrDataStorage, self.VfrQuestionDB,
                ctx.stop.line)
            Status = 0 if ReturnCode == VfrReturnCode.VFR_RETURN_SUCCESS else 1
            self.ParserStatus += Status ###
            self.NeedAdjustOpcode = True

        self.InsertEndNode(ctx.Node, ctx.stop.line)

        if self.NeedAdjustOpcode:
            self.LastFormNode.Child.pop()
            for InsertOpCode in InsertOpCodeList:
                InsertNode = VfrTreeNode(InsertOpCode.OpCode, InsertOpCode.Data, gFormPkg.StructToStream(InsertOpCode.Data.GetInfo()))
                self.LastFormNode.insertChild(InsertNode)

        gFormPkg.BuildPkg(self.Root)
        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#classguidDefinition.
    def visitClassguidDefinition(self, ctx:SourceVfrSyntaxParser.ClassguidDefinitionContext):

        self.visitChildren(ctx)

        for GuidItem in ctx.StringIdentifier():
            GuidStr = str(GuidItem)
            Guid = self.PreProcessDB.Read(GuidStr)
            if 'classguid' not in ctx.Node.Dict.keys():
                ctx.Node.Dict['classguid'] = [KV(GuidStr, Guid)]
            else:
                ctx.Node.Dict['classguid'].append(KV(GuidStr, Guid))
            ctx.GuidList.append(Guid)
        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#classDefinition.
    def visitClassDefinition(self, ctx:SourceVfrSyntaxParser.ClassDefinitionContext):
        CObj = IfrClass()
        self.visitChildren(ctx)
        Class = 0
        for ClassNameCtx in ctx.validClassNames():
            Class |= ClassNameCtx.ClassName
        Line = ctx.start.line
        CObj.SetLineNo(Line)
        CObj.SetClass(Class)
        ctx.Node.Data = CObj
        ctx.Node.Buffer = gFormPkg.StructToStream(CObj.GetInfo())
        ctx.Node.Dict['class'] = KV(self.ExtractOriginalText(ctx), Class)

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#validClassNames.
    def visitValidClassNames(self, ctx:SourceVfrSyntaxParser.ValidClassNamesContext):

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
        elif ctx.S != None:
            ctx.ClassName = self.PreProcessDB.Read(ctx.S.text)
        else:
            ctx.ClassName = self.TransNum(ctx.Number())

        return ctx.ClassName


    # Visit a parse tree produced by SourceVfrSyntaxParser#subclassDefinition.
    def visitSubclassDefinition(self, ctx:SourceVfrSyntaxParser.SubclassDefinitionContext):
        SubObj = IfrSubClass()

        self.visitChildren(ctx)

        Line = ctx.start.line
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
        elif ctx.S != None:
            SubClass |= self.PreProcessDB.Read(ctx.S.text)
        else:
            SubClass = self.TransNum(ctx.Number())

        SubObj.SetSubClass(SubClass)
        ctx.Node.Dict['subclass'] = KV(self.ExtractOriginalText(ctx), SubClass)
        ctx.Node.Data = SubObj
        ctx.Node.Buffer = gFormPkg.StructToStream(SubObj.GetInfo())

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrFormSetList.
    def visitVfrFormSetList(self, ctx:SourceVfrSyntaxParser.VfrFormSetListContext):
        self.visitChildren(ctx)
        for Ctx in ctx.vfrFormSet():
            self.InsertChild(ctx.Node, Ctx)

        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrFormSet.
    def visitVfrFormSet(self, ctx:SourceVfrSyntaxParser.VfrFormSetContext):
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

        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementDefaultStore.
    def visitVfrStatementDefaultStore(self, ctx:SourceVfrSyntaxParser.VfrStatementDefaultStoreContext):

        self.visitChildren(ctx)

        Line = ctx.start.line
        RefName = ctx.D.text
        DSObj = IfrDefaultStore(RefName)
        DefaultStoreNameId = self.PreProcessDB.Read(ctx.P.text)
        ctx.Node.Dict['prompt'] = KV(ctx.P.text, DefaultStoreNameId)
        DefaultId = EFI_HII_DEFAULT_CLASS_STANDARD
        if ctx.A != None:
            DefaultId = self.TransNum(ctx.A.text)
            DSObj.HasAttr == True
        elif ctx.S != None:
            DefaultId = self.PreProcessDB.Read(ctx.S.text)
            ctx.Node.Dict['attribute'] = KV(ctx.S.text, DefaultId)
            DSObj.HasAttr == True

        if gVfrDefaultStore.DefaultIdRegistered(DefaultId) == False:
            self.ErrorHandler(gVfrDefaultStore.RegisterDefaultStore(DSObj.DefaultStore, RefName, DefaultStoreNameId, DefaultId), Line)
            DSObj.SetDefaultName(DefaultStoreNameId)
            DSObj.SetDefaultId (DefaultId)
            DSObj.SetLineNo(Line)
            ctx.Node.Data = DSObj
            ctx.Node.Buffer = gFormPkg.StructToStream(DSObj.GetInfo())
        else:
            pNode, ReturnCode = gVfrDefaultStore.ReRegisterDefaultStoreById(DefaultId, RefName, DefaultStoreNameId)
            self.ErrorHandler(ReturnCode, Line)
            ctx.Node.OpCode = EFI_IFR_SHOWN_DEFAULTSTORE_OP # For display in YAML
            DSObj.SetDefaultId (DefaultId)
            ctx.Node.Data = DSObj
        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementVarStoreLinear.
    def visitVfrStatementVarStoreLinear(self, ctx:SourceVfrSyntaxParser.VfrStatementVarStoreLinearContext):

        self.visitChildren(ctx)

        TypeName = str(ctx.getChild(1))
        if TypeName == 'CHAR16':
            TypeName = 'UINT16'

        IsBitVarStore = False
        if ctx.TN != None:
            IsBitVarStore = gVfrVarDataTypeDB.DataTypeHasBitField(ctx.TN.text)

        VarStoreId = EFI_VARSTORE_ID_INVALID
        if ctx.VarId() != None:
            if ctx.ID != None:
                VarStoreId = self.TransNum(ctx.ID.text)
                Tok = ctx.ID
            elif ctx.S != None:
                VarStoreId = self.PreProcessDB.Read(ctx.S.text)
                ctx.Node.Dict['varid'] = KV(ctx.S.text, VarStoreId)
                Tok = ctx.S
            self.CompareErrorHandler(VarStoreId!=0, True, Tok.line, Tok.text, 'varid 0 is not allowed.')
        StoreName = ctx.SN.text
        VSObj = IfrVarStore(TypeName, StoreName)
        Line = ctx.start.line
        VSObj.SetLineNo(Line)
        VSObj.SetHasVarStoreId(ctx.VarId() != None)
        Guid = self.PreProcessDB.Read(ctx.SG.text)
        ctx.Node.Dict['guid'] = KV(ctx.SG.text, Guid)
        self.ErrorHandler(gVfrDataStorage.DeclareBufferVarStore(StoreName, Guid, gVfrVarDataTypeDB, TypeName, VarStoreId, IsBitVarStore), Line)
        VSObj.SetGuid(Guid)
        VarStoreId, ReturnCode = gVfrDataStorage.GetVarStoreId(StoreName, Guid)
        self.ErrorHandler(ReturnCode, ctx.SN.line, ctx.SN.text)
        VSObj.SetVarStoreId(VarStoreId)
        Size, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByTypeName(TypeName)
        self.ErrorHandler(ReturnCode, Line)
        VSObj.SetSize(Size)
        ctx.Node.Data = VSObj
        ctx.Node.Buffer = gFormPkg.StructToStream(VSObj.GetInfo())
        ctx.Node.Buffer += bytes('\0',encoding='utf-8')

        return VSObj


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementVarStoreEfi.
    def visitVfrStatementVarStoreEfi(self, ctx:SourceVfrSyntaxParser.VfrStatementVarStoreEfiContext):

        self.visitChildren(ctx)
        Line = ctx.start.line

        Guid = self.PreProcessDB.Read(ctx.SG.text)
        ctx.Node.Dict['guid'] = KV(ctx.SG.text, Guid)

        CustomizedName = False
        IsBitVarStore = False
        VarStoreId = EFI_VARSTORE_ID_INVALID
        IsUEFI23EfiVarstore = True
        ReturnCode = None

        TypeName = str(ctx.getChild(1))

        if TypeName == 'CHAR16':
            TypeName = 'UINT16'

        elif ctx.TN != None:
            CustomizedName = True
            IsBitVarStore = gVfrVarDataTypeDB.DataTypeHasBitField(TypeName)

        if ctx.VarId() != None:
            if ctx.ID != None:
                VarStoreId = self.TransNum(ctx.ID.text)
            elif ctx.S1 != None:
                VarStoreId = self.PreProcessDB.Read(ctx.S1.text)
                ctx.Node.Dict['varid'] = KV(ctx.S1.text, VarStoreId)
            self.CompareErrorHandler(VarStoreId!=0, True, ctx.ID.line, ctx.ID.text, 'varid 0 is not allowed.')

        Attributes = 0
        AttributesText = ''
        for i in range(0, len(ctx.vfrVarStoreEfiAttr())):
            if type(ctx.vfrVarStoreEfiAttr(i).Attr) == str:
                Attr = self.PreProcessDB.Read(ctx.vfrVarStoreEfiAttr(i).Attr)
                if 'attribute' not in ctx.Node.Dict.keys():
                    ctx.Node.Dict['attribute'] = [KV(ctx.vfrVarStoreEfiAttr(i).Attr, Attr)]
                else:
                    ctx.Node.Dict['attribute'].append(KV(ctx.vfrVarStoreEfiAttr(i).Attr, Attr))

                Attributes |= Attr
            else:
                Attributes |= ctx.vfrVarStoreEfiAttr(i).Attr

            if i != len(ctx.vfrVarStoreEfiAttr()) - 1:
                AttributesText += '{} | '.format(ctx.vfrVarStoreEfiAttr(i).Attr)
            else:
                AttributesText += '{}'.format(ctx.vfrVarStoreEfiAttr(i).Attr)

        if ctx.SN != None:
            StoreName = ctx.SN.text
        else:
            IsUEFI23EfiVarstore = False
            NameStringId = self.PreProcessDB.Read(ctx.VN.text)
            ctx.Node.Dict['name'] = KV(ctx.VN.text, NameStringId)
            StoreName = gVfrStringDB.GetVarStoreNameFromStringId(NameStringId) #
            if StoreName == None:
                gVfrErrorHandle.HandleWarning(VfrReturnCode.VFR_RETURN_UNSUPPORTED, ctx.VN.line, 'Can\'t get varstore name for this StringId!')
            if not(CustomizedName):
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, Line, 'Old style efivarstore must have String Identifier!')
            if ctx.N != None:
                Size = self.TransNum(ctx.N.text)
                ctx.Node.Dict['varsize'] = KV(Size, Size)
            else:
                Size = self.PreProcessDB.Read(ctx.S2.text)
                ctx.Node.Dict['varsize'] = KV(ctx.S2.text, Size)
            if Size == 1: TypeName = 'UINT8'
            elif Size == 2: TypeName = 'UINT16'
            elif Size == 4: TypeName = 'UINT32'
            elif Size == 8: TypeName = 'UINT64'
            else:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, ctx.N.line, ctx.N.text)

        if IsUEFI23EfiVarstore:
            self.ErrorHandler(gVfrDataStorage.DeclareBufferVarStore(StoreName, Guid, gVfrVarDataTypeDB, TypeName, VarStoreId, IsBitVarStore, Attributes), Line)
            VarStoreId, ReturnCode = gVfrDataStorage.GetVarStoreId(StoreName, Guid)
            self.ErrorHandler(ReturnCode, ctx.SN.line, ctx.SN.text)
            Size, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByTypeName(TypeName)
            self.ErrorHandler(ReturnCode, Line)
        else:
            self.ErrorHandler(gVfrDataStorage.DeclareBufferVarStore(self.GetText(ctx.TN), Guid, gVfrVarDataTypeDB, TypeName, VarStoreId, IsBitVarStore, Attributes), Line) #
            VarStoreId, ReturnCode = gVfrDataStorage.GetVarStoreId(self.GetText(ctx.TN), Guid)
            self.ErrorHandler(ReturnCode, ctx.VN.line, ctx.VN.text)
            Size, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByTypeName(TypeName)
            self.ErrorHandler(ReturnCode, ctx.N.line)

        VSEObj = IfrVarStoreEfi(TypeName, StoreName)
        VSEObj.SetLineNo(Line)
        VSEObj.SetHasVarStoreId(ctx.VarId() != None)
        VSEObj.SetGuid(Guid)
        VSEObj.SetVarStoreId (VarStoreId)
        VSEObj.SetSize(Size)
        VSEObj.SetAttributes(Attributes)
        VSEObj.SetAttributesText(AttributesText)

        ctx.Node.Data = VSEObj
        ctx.Node.Buffer = gFormPkg.StructToStream(VSEObj.GetInfo())
        ctx.Node.Buffer += bytes('\0',encoding='utf-8')

        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrVarStoreEfiAttr.
    def visitVfrVarStoreEfiAttr(self, ctx:SourceVfrSyntaxParser.VfrVarStoreEfiAttrContext):

        self.visitChildren(ctx)
        if ctx.N != None:
            ctx.Attr = self.TransNum(ctx.N.text)
        elif ctx.S != None:
            ctx.Attr = ctx.S.text
        return ctx.Attr

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementVarStoreNameValue.
    def visitVfrStatementVarStoreNameValue(self, ctx:SourceVfrSyntaxParser.VfrStatementVarStoreNameValueContext):
        StoreName = ctx.SN.text
        VSNVObj = IfrVarStoreNameValue(StoreName)
        self.visitChildren(ctx)

        Guid = self.PreProcessDB.Read(ctx.SG.text)
        ctx.Node.Dict['guid'] = KV(ctx.SG.text, Guid)
        HasVarStoreId = False
        VarStoreId = EFI_VARSTORE_ID_INVALID

        if ctx.VarId() != None:
            HasVarStoreId = True
            if ctx.ID != None:
                VarStoreId = self.TransNum(ctx.ID.text)
            elif ctx.SID != None:
                VarStoreId = self.PreProcessDB.Read(ctx.SID.text)
                ctx.Node.Dict['varid'] = KV(ctx.SID.text, VarStoreId)
            self.CompareErrorHandler(VarStoreId !=0, True, ctx.ID.line, ctx.ID.text, 'varid 0 is not allowed')

        Created = False

        sIndex = 1 if  ctx.SID == None else 2
        eIndex = sIndex + len(ctx.Name())
        for i in range(sIndex, eIndex):
            if Created == False:
                self.ErrorHandler(gVfrDataStorage.DeclareNameVarStoreBegin(StoreName, VarStoreId), ctx.SN.line, ctx.SN.text)
                Created = True
            Item = self.PreProcessDB.Read(str(ctx.StringIdentifier(i)))
            if 'name' not in ctx.Node.Dict.keys():
                ctx.Node.Dict['name'] = [KV(str(ctx.StringIdentifier(i)), Item)]
            else:
                ctx.Node.Dict['name'].append(KV(str(ctx.StringIdentifier(i)), Item))
            VSNVObj.SetNameItemList(Item)
            gVfrDataStorage.NameTableAddItem(Item)

        gVfrDataStorage.DeclareNameVarStoreEnd(Guid)

        VSNVObj.SetLineNo(ctx.start.line)
        VSNVObj.SetGuid(Guid)
        VarstoreId, ReturnCode = gVfrDataStorage.GetVarStoreId(StoreName, Guid)
        self.ErrorHandler(ReturnCode, ctx.SN.line, ctx.SN.text)
        VSNVObj.SetVarStoreId(VarstoreId)
        VSNVObj.SetHasVarStoreId(HasVarStoreId)

        ctx.Node.Data = VSNVObj
        ctx.Node.Buffer = gFormPkg.StructToStream(VSNVObj.GetInfo())

        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementDisableIfFormSet.
    def visitVfrStatementDisableIfFormSet(self, ctx:SourceVfrSyntaxParser.VfrStatementDisableIfFormSetContext):

        DIObj = IfrDisableIf()
        DIObj.SetLineNo(ctx.start.line)
        self.ConstantOnlyInExpression = True
        ctx.Node.Data = DIObj
        Condition = 'disableif' + ' ' + self.ExtractOriginalText(ctx.vfrStatementExpression())
        ctx.Node.Condition = Condition
        self.visitChildren(ctx)
        self.InsertEndNode(ctx.Node, ctx.stop.line)

        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementSuppressIfFormSet.
    def visitVfrStatementSuppressIfFormSet(self, ctx:SourceVfrSyntaxParser.VfrStatementSuppressIfFormSetContext):

        SIObj = IfrSuppressIf()
        SIObj.SetLineNo(ctx.start.line)
        ctx.Node.Data = SIObj
        Condition = 'suppressif' + ' ' +  self.ExtractOriginalText(ctx.vfrStatementExpression())
        ctx.Node.Condition = Condition
        self.visitChildren(ctx)
        self.InsertEndNode(ctx.Node, ctx.stop.line)
        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#getStringId.
    def visitGetStringId(self, ctx:SourceVfrSyntaxParser.GetStringIdContext):

        ctx.StringId = self.TransNum(ctx.Number())

        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementHeader.
    def visitVfrStatementHeader(self, ctx:SourceVfrSyntaxParser.VfrStatementHeaderContext):

        if ctx.Node.Data != None:
            Prompt = self.PreProcessDB.Read(ctx.P.text)
            ctx.Node.Dict['prompt'] = KV(ctx.P.text, Prompt)
            ctx.Node.Data.SetPrompt(Prompt)
            Help = self.PreProcessDB.Read(ctx.H.text)
            ctx.Node.Dict['help'] = KV(ctx.H.text, Help)
            ctx.Node.Data.SetHelp(Help)

        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrQuestionHeader.
    def visitVfrQuestionHeader(self, ctx:SourceVfrSyntaxParser.VfrQuestionHeaderContext):

        return  self.visitChildren(ctx)

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrQuestionBaseInfo.
    def visitVfrQuestionBaseInfo(self, ctx:SourceVfrSyntaxParser.VfrQuestionBaseInfoContext):

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
            QName = ctx.QN.text
            ReturnCode = self.VfrQuestionDB.FindQuestionByName(QName)
            self.CompareErrorHandler(ReturnCode, VfrReturnCode.VFR_RETURN_UNDEFINED, ctx.QN.line, ctx.QN.text, 'has already been used please used anther name')

        VarIdStr = '' if ctx.VarId() == None else  ctx.vfrStorageVarId().VarIdStr
        if ctx.QuestionId() != None:
            if ctx.ID != None:
                QId = self.TransNum(ctx.ID.text)
                Tok = ctx.ID
            elif ctx.SID != None:
                QId = self.PreProcessDB.Read(ctx.SID.text)
                ctx.Node.Dict['questionid'] = KV(ctx.SID.text, QId)
                Tok = ctx.SID
            ReturnCode = self.VfrQuestionDB.FindQuestionById(QId)
            self.CompareErrorHandler(ReturnCode, VfrReturnCode.VFR_RETURN_UNDEFINED, Tok.line, Tok.text, 'has already been used please used anther number')

        if ctx.QType == EFI_QUESION_TYPE.QUESTION_NORMAL:
            #if self.IsCheckBoxOp:
                #ctx.BaseInfo.VarType = EFI_IFR_TYPE_BOOLEAN #
            QId, ReturnCode = self.VfrQuestionDB.RegisterQuestion(QName, VarIdStr, QId, gFormPkg)
            self.ErrorHandler(ReturnCode, ctx.start.line)

        elif ctx.QType == EFI_QUESION_TYPE.QUESTION_DATE:
            ctx.BaseInfo.VarType = EFI_IFR_TYPE_DATE
            QId, ReturnCode = self.VfrQuestionDB.RegisterNewDateQuestion(QName, VarIdStr, QId, gFormPkg)
            self.ErrorHandler(ReturnCode, ctx.start.line)

        elif ctx.QType == EFI_QUESION_TYPE.QUESTION_TIME:
            ctx.BaseInfo.VarType = EFI_IFR_TYPE_TIME
            QId, ReturnCode = self.VfrQuestionDB.RegisterNewTimeQuestion(QName, VarIdStr, QId, gFormPkg)
            self.ErrorHandler(ReturnCode, ctx.start.line)

        elif ctx.QType == EFI_QUESION_TYPE.QUESTION_REF:
            ctx.BaseInfo.VarType = EFI_IFR_TYPE_REF
            if VarIdStr != '': #stand for question with storage.
                QId, ReturnCode = self.VfrQuestionDB.RegisterRefQuestion(QName, VarIdStr, QId, gFormPkg)
                self.ErrorHandler(ReturnCode, ctx.start.line)
            else:
                QId, ReturnCode = self.VfrQuestionDB.RegisterQuestion(QName, None, QId, gFormPkg)
                self.ErrorHandler(ReturnCode, ctx.start.line)
        else:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_FATAL_ERROR, ctx.start.line)

        self.CurrQestVarInfo = ctx.BaseInfo

        if ctx.Node.OpCode == EFI_IFR_ONE_OF_OP:
            #need to further update the VarType
            #ctx.BaseInfo.VarType = EFI_IFR_TYPE_NUM_SIZE_64 #
            ctx.Node.Data = IfrOneOf(EFI_IFR_TYPE_NUM_SIZE_64, QName, VarIdStr)
            self.CurrentQuestion = ctx.Node.Data
            self.CurrentMinMaxData = ctx.Node.Data

        elif ctx.Node.OpCode == EFI_IFR_NUMERIC_OP:
            #ctx.BaseInfo.VarType = EFI_IFR_TYPE_NUM_SIZE_64 #
            ctx.Node.Data = IfrNumeric(EFI_IFR_TYPE_NUM_SIZE_64, QName, VarIdStr)
            self.CurrentQuestion = ctx.Node.Data
            self.CurrentMinMaxData = ctx.Node.Data

        if ctx.Node.Data != None:
            ctx.Node.Data.SetQName(QName)
            ctx.Node.Data.SetVarIdStr(VarIdStr)
            ctx.Node.Data.SetQuestionId(QId)
            ctx.Node.Data.SetHasQuestionId(ctx.QuestionId() != None)
            if ctx.BaseInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
                ctx.Node.Data.SetVarStoreInfo(ctx.BaseInfo)

        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#questionheaderFlagsField.
    def visitQuestionheaderFlagsField(self, ctx:SourceVfrSyntaxParser.QuestionheaderFlagsFieldContext):

        self.visitChildren(ctx)
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

        elif ctx.OptionOnlyFlag() != None:
            gVfrErrorHandle.HandleWarning(EFI_VFR_WARNING_CODE.VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE, ctx.O.line, ctx.O.text)

        elif ctx.NVAccessFlag() != None:
            gVfrErrorHandle.HandleWarning(EFI_VFR_WARNING_CODE.VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE, ctx.N.line, ctx.N.text)

        elif ctx.LateCheckFlag() != None:
            gVfrErrorHandle.HandleWarning(EFI_VFR_WARNING_CODE.VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE, ctx.L.line, ctx.L.text)

        return ctx.QHFlag

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStorageVarIdRule1.
    def visitVfrStorageVarIdRule1(self, ctx:SourceVfrSyntaxParser.VfrStorageVarIdRule1Context):

        self.visitChildren(ctx)

        SName = ctx.SN1.text
        ctx.VarIdStr += SName

        Idx = self.TransNum(ctx.I.text)
        ctx.VarIdStr += '['
        ctx.VarIdStr += ctx.I.text
        ctx.VarIdStr += ']'

        ctx.BaseInfo.VarStoreId, ReturnCode = gVfrDataStorage.GetVarStoreId(SName)
        if ctx.CheckFlag or ReturnCode == VfrReturnCode.VFR_RETURN_SUCCESS:
            self.ErrorHandler(ReturnCode, ctx.SN1.line, ctx.SN1.text)
            self.ErrorHandler(gVfrDataStorage.GetNameVarStoreInfo(ctx.BaseInfo, Idx), ctx.SN1.line, ctx.SN1.text)

        return ctx.VarIdStr


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStorageVarIdRule2.
    def visitVfrStorageVarIdRule2(self, ctx:SourceVfrSyntaxParser.VfrStorageVarIdRule2Context):

        self.visitChildren(ctx)

        VarStr = '' # type.field
        SName = ctx.SN2.text
        ctx.VarIdStr += SName
        ctx.BaseInfo.VarStoreId, ReturnCode = gVfrDataStorage.GetVarStoreId(SName)
        if ctx.CheckFlag or ReturnCode == VfrReturnCode.VFR_RETURN_SUCCESS:
            self.ErrorHandler(ReturnCode, ctx.SN2.line, ctx.SN2.text)
            VarStoreType = gVfrDataStorage.GetVarStoreType(ctx.BaseInfo.VarStoreId)
            if VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER or VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER_BITS:
                TName, ReturnCode2 = gVfrDataStorage.GetBufferVarStoreDataTypeName(ctx.BaseInfo.VarStoreId)
                self.ErrorHandler(ReturnCode2, ctx.SN2.line, ctx.SN2.text)
                VarStr += TName

        Count = len(ctx.Dot())
        for i in range(0, Count):
            if ctx.CheckFlag or ReturnCode == VfrReturnCode.VFR_RETURN_SUCCESS:
                cond = (VarStoreType != EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER) and (VarStoreType != EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER_BITS)
                ReturnCode = VfrReturnCode.VFR_RETURN_EFIVARSTORE_USE_ERROR if cond else VfrReturnCode.VFR_RETURN_SUCCESS
                self.ErrorHandler(ReturnCode, ctx.SN2.line, ctx.SN2.text)

            ctx.VarIdStr += '.'
            VarStr += '.'
            ctx.VarIdStr += ctx.arrayName(i).SubStr
            VarStr += ctx.arrayName(i).SubStrZ

        if VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_EFI:
            self.ErrorHandler(gVfrDataStorage.GetEfiVarStoreInfo(ctx.BaseInfo), ctx.SN2.line, ctx.SN2.text)

        elif VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER or VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER_BITS:
            ctx.BaseInfo.Info.VarOffset, ctx.BaseInfo.VarType, ctx.BaseInfo.VarTotalSize, ctx.BaseInfo.IsBitVar, ReturnCode = gVfrVarDataTypeDB.GetDataFieldInfo(VarStr)
            self.ErrorHandler(ReturnCode, ctx.SN2.line, VarStr)
            VarGuid = gVfrDataStorage.GetVarStoreGuid(ctx.BaseInfo.VarStoreId)
            self.ErrorHandler(gVfrBufferConfig.Register(SName, VarGuid), ctx.SN2.line)
            ReturnCode = VfrReturnCode(gVfrBufferConfig.Write(
                'a',
                SName,
                VarGuid,
                None,
                ctx.BaseInfo.VarType,
                ctx.BaseInfo.Info.VarOffset,
                ctx.BaseInfo.VarTotalSize,
                self.Value))
            self.ErrorHandler(ReturnCode, ctx.SN2.line)
            self.ErrorHandler(gVfrDataStorage.AddBufferVarStoreFieldInfo(ctx.BaseInfo), ctx.SN2.line)

        return ctx.VarIdStr

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrConstantValueField.
    def visitVfrConstantValueField(self, ctx:SourceVfrSyntaxParser.VfrConstantValueFieldContext):
        self.visitChildren(ctx)

        IntDecStyle = False
        if self.CurrentMinMaxData != None and self.CurrentMinMaxData.IsNumericOpcode():
            NumericQst = IfrNumeric(self.CurrentQuestion) #
            IntDecStyle = True if (NumericQst.GetNumericFlags() & EFI_IFR_DISPLAY) == 0 else False #

        if ctx.TrueSymbol() != None:
            ctx.ValueList.append(1)

        elif ctx.FalseSymbol() != None:
            ctx.ValueList.append(0)

        elif ctx.One() != None:
            ctx.ValueList.append(int(ctx.getText()))

        elif ctx.Ones() != None:
            ctx.ValueList.append(int(ctx.getText()))

        elif ctx.Zero() != None:
            ctx.ValueList.append(int(ctx.getText()))

        elif ctx.S1 != None:
            ctx.ValueList.append(self.PreProcessDB.Read(ctx.S1.text))
            ctx.Node.Dict['value'] = KV(ctx.S1.text, self.PreProcessDB.Read(ctx.S1.text))

        elif ctx.Colon() != []:
            Time = EFI_HII_TIME()
            Time.Hour = self.TransNum(ctx.Number(0))
            Time.Minute = self.TransNum(ctx.Number(1))
            Time.Second = self.TransNum(ctx.Number(2))
            ctx.ValueList.append(Time)

        elif ctx.Slash() != []:
            Date = EFI_HII_DATE()
            Date.Year = self.TransNum(ctx.Number(0))
            Date.Month = self.TransNum(ctx.Number(1))
            Date.Day = self.TransNum(ctx.Number(2))
            ctx.ValueList.append(Date)

        elif ctx.Semicolon() != []:
            Ref = EFI_HII_REF()
            Ref.QuestionId = self.TransNum(ctx.Number(0))
            Ref.FormId = self.TransNum(ctx.Number(1))
            Ref.FormSetGuid = self.PreProcessDB.Read(ctx.S2.text)
            ctx.Node.Dict['formsetguid'] = KV(ctx.S2.text, Ref.FormSetGuid)
            Ref.DevicePath = self.PreProcessDB.Read(ctx.S3.text)
            ctx.Node.Dict['devicepath'] = KV(ctx.S3.text, Ref.DevicePath)
            ctx.ValueList.append(Ref)

        elif ctx.StringToken() != None:
            ctx.ValueList.append(self.PreProcessDB.Read(ctx.S4.text))
            ctx.Node.Dict['string'] = KV(ctx.S4.text, self.PreProcessDB.Read(ctx.StringIdentifier()))

        elif ctx.OpenBrace() != None:
            ctx.ListType = True
            Type = self.CurrQestVarInfo.VarType
            for i in range(0, len(ctx.Number())):
                ctx.ValueList.append(self.TransNum(ctx.Number(i)))

        else:
            Negative = True if ctx.Negative() != None else False
            # The value stored in bit fields is always set to UINT32 type.
            if self.CurrQestVarInfo.IsBitVar:
                ctx.ValueList.append(self.TransNum(ctx.Number(0)))
            else:
                Type = self.CurrQestVarInfo.VarType
                Value = self.TransNum(ctx.Number(0))
                if Type == EFI_IFR_TYPE_NUM_SIZE_8:
                    if IntDecStyle:
                        if Negative:
                            if  Value > 0x80:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.start.line, "INT8 type can't big than 0x7F, small than -0x80")
                        else:
                            if Value > 0x7F:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.start.line, "INT8 type can't big than 0x7F, small than -0x80")
                    if Negative:
                        Value = ~Value + 1
                    ctx.ValueList.append(Value)

                elif Type == EFI_IFR_TYPE_NUM_SIZE_16:
                    if IntDecStyle:
                        if Negative:
                            if  Value > 0x8000:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.start.line, "INT16 type can't big than 0x7FFF, small than -0x8000")
                        else:
                            if Value > 0x7FFF:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.start.line, "INT16 type can't big than 0x7FFF, small than -0x8000")
                    if Negative:
                        Value = ~Value + 1
                    ctx.ValueList.append(Value)

                elif Type == EFI_IFR_TYPE_NUM_SIZE_32:
                    if IntDecStyle:
                        if Negative:
                            if  Value > 0x80000000:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.start.line, "INT32 type can't big than 0x7FFFFFFF, small than -0x80000000")
                        else:
                            if Value > 0X7FFFFFFF:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.start.line, "INT32 type can't big than 0x7FFFFFFF, small than -0x80000000")
                    if Negative:
                        Value = ~Value + 1
                    ctx.ValueList.append(Value)

                elif Type == EFI_IFR_TYPE_NUM_SIZE_64:
                    if IntDecStyle:
                        if Negative:
                            if  Value > 0x8000000000000000:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.start.line, "INT64 type can't big than 0x7FFFFFFFFFFFFFFF, small than -0x8000000000000000")
                        else:
                            if Value > 0x7FFFFFFFFFFFFFFF:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.start.line, "INT64 type can't big than 0x7FFFFFFFFFFFFFFF, small than -0x8000000000000000")
                    if Negative:
                        Value = ~Value + 1
                    ctx.ValueList.append(Value)

                if Type == EFI_IFR_TYPE_BOOLEAN:
                    ctx.ValueList.append(Value)

                if Type == EFI_IFR_TYPE_STRING:
                    ctx.ValueList.append(Value)


        return ctx.ValueList



    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrImageTag.
    def visitVfrImageTag(self, ctx:SourceVfrSyntaxParser.VfrImageTagContext):

        IObj = IfrImage()
        self.visitChildren(ctx)
        IObj.SetLineNo(ctx.start.line)
        ImageId = self.PreProcessDB.Read(str(ctx.StringIdentifier()))
        IObj.SetImageId(ImageId)
        ctx.Node.Dict['imageid'] = KV(str(ctx.StringIdentifier()), ImageId)
        ctx.Node.Data = IObj
        ctx.Node.Buffer = gFormPkg.StructToStream(IObj.GetInfo())
        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrLockedTag.
    def visitVfrLockedTag(self, ctx:SourceVfrSyntaxParser.VfrLockedTagContext):

        LObj = IfrLocked()
        self.visitChildren(ctx)
        LObj.SetLineNo(ctx.start.line)
        ctx.Node.Data = LObj
        ctx.Node.Buffer = gFormPkg.StructToStream(LObj.GetInfo())
        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementStatTag.
    def visitVfrStatementStatTag(self, ctx:SourceVfrSyntaxParser.VfrStatementStatTagContext):

        self.visitChildren(ctx)
        if ctx.vfrImageTag() != None:
            ctx.Node = ctx.vfrImageTag().Node
        elif ctx.vfrLockedTag() != None:
            ctx.Node = ctx.vfrLockedTag().Node

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementStatTagList.
    def visitVfrStatementStatTagList(self, ctx:SourceVfrSyntaxParser.VfrStatementStatTagListContext):
        self.visitChildren(ctx)
        for Ctx in ctx.vfrStatementStatTag():
            self.InsertChild(ctx.Node, Ctx)

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrFormDefinition.
    def visitVfrFormDefinition(self, ctx:SourceVfrSyntaxParser.VfrFormDefinitionContext):

        FObj = IfrForm()
        self.visitChildren(ctx)

        FObj.SetLineNo(ctx.start.line)
        if ctx.N != None:
            FormId = self.TransNum(ctx.N.text)
        else:
            FormId = self.PreProcessDB.Read(ctx.S.text)
            ctx.Node.Dict['formid'] = KV(ctx.S.text, FormId)
        FObj.SetFormId(FormId)
        FormTitle  = self.PreProcessDB.Read(ctx.ST.text)
        ctx.Node.Dict['title'] = KV(ctx.ST.text, FormTitle)
        FObj.SetFormTitle(FormTitle)

        ctx.Node.Data = FObj
        for Ctx in ctx.vfrForm():
            self.InsertChild(ctx.Node, Ctx)
        ctx.Node.Buffer = gFormPkg.StructToStream(FObj.GetInfo())

        self.InsertEndNode(ctx.Node, ctx.stop.line)
        self.LastFormNode = ctx.Node

        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrForm.
    def visitVfrForm(self, ctx:SourceVfrSyntaxParser.VfrFormContext):

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

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrFormMapDefinition.
    def visitVfrFormMapDefinition(self, ctx:SourceVfrSyntaxParser.VfrFormMapDefinitionContext):
        FMapObj = IfrFormMap()
        self.visitChildren(ctx)
        FormMapMethodNumber = len(ctx.MapTitle())
        Line = ctx.start.line
        FMapObj.SetLineNo(Line)
        if ctx.N != None:
            FormId = self.TransNum(ctx.N.text)
            SIndex = 0
        else:
            FormId = self.PreProcessDB.Read(ctx.S.text)
            ctx.Node.Dict['formid'] = KV(ctx.S.text, FormId)
            SIndex = 1
        FMapObj.SetFormId(FormId)
        if FormMapMethodNumber == 0:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, Line, 'No MapMethod is set for FormMap!')
        else:
            for i in range(SIndex, SIndex + FormMapMethodNumber*2, 2):
                MapTitle = self.PreProcessDB.Read(self.TransId(ctx.StringIdentifier(i)))
                ctx.Node.Dict['maptitle'] = KV(self.TransId(ctx.StringIdentifier(i)), MapTitle)
                MapGuid = self.PreProcessDB.Read(self.TransId(ctx.StringIdentifier(i + 1)))
                ctx.Node.Dict['mapguid'] = KV(self.TransId(ctx.StringIdentifier(i + 1)), MapGuid)
                FMapObj.SetFormMapMethod(MapTitle, MapGuid)
        FormMap = FMapObj.GetInfo()
        MethodMapList = FMapObj.GetMethodMapList()
        for MethodMap in MethodMapList: # Extend Header Size for MethodMapList
            FormMap.Header.Length += sizeof(EFI_IFR_FORM_MAP_METHOD)

        ctx.Node.Data = FMapObj
        ctx.Node.Buffer = gFormPkg.StructToStream(FormMap)
        for MethodMap in MethodMapList:
            ctx.Node.Buffer += gFormPkg.StructToStream(MethodMap)
        for Ctx in ctx.vfrForm():
            self.InsertChild(ctx.Node, Ctx)
        self.InsertEndNode(ctx.Node, ctx.stop.line)

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementImage.
    def visitVfrStatementImage(self, ctx:SourceVfrSyntaxParser.VfrStatementImageContext):

        self.visitChildren(ctx)
        ctx.Node = ctx.vfrImageTag().Node
        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementLocked.
    def visitVfrStatementLocked(self, ctx:SourceVfrSyntaxParser.VfrStatementLockedContext):

        self.visitChildren(ctx)
        ctx.Node = ctx.vfrLockedTag().Node
        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementRules.
    def visitVfrStatementRules(self, ctx:SourceVfrSyntaxParser.VfrStatementRulesContext):

        RObj = IfrRule()
        self.visitChildren(ctx)

        RObj.SetLineNo(ctx.start.line)
        RuleName = self.TransId(ctx.StringIdentifier())
        RObj.SetRuleName(RuleName)
        self.VfrRulesDB.RegisterRule(RuleName)
        RObj.SetRuleId(self.VfrRulesDB.GetRuleId(RuleName))
        ctx.Node.Data = RObj
        # expression
        ctx.Node.Expression = self.ExtractOriginalText(ctx.vfrStatementExpression())
        ctx.Node.Buffer = gFormPkg.StructToStream(RObj.GetInfo())
        self.InsertEndNode(ctx.Node, ctx.stop.line)
        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementStat.
    def visitVfrStatementStat(self, ctx:SourceVfrSyntaxParser.VfrStatementStatContext):

        self.visitChildren(ctx)
        if ctx.vfrStatementSubTitle() != None:
            ctx.Node = ctx.vfrStatementSubTitle().Node
        if ctx.vfrStatementStaticText() != None:
            ctx.Node = ctx.vfrStatementStaticText().Node
        if ctx.vfrStatementCrossReference() != None:
            ctx.Node = ctx.vfrStatementCrossReference().Node
        return ctx.Node



    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementSubTitle.
    def visitVfrStatementSubTitle(self, ctx:SourceVfrSyntaxParser.VfrStatementSubTitleContext):

        SObj = IfrSubtitle()

        Line = ctx.start.line
        SObj.SetLineNo(Line)

        Prompt = self.PreProcessDB.Read(ctx.S.text)
        ctx.Node.Dict['prompt'] = KV(ctx.S.text, Prompt)
        SObj.SetPrompt(Prompt)

        self.visitChildren(ctx)

        if ctx.vfrSubtitleFlags() != None:
            SObj.SetFlags(ctx.vfrSubtitleFlags().SubFlags)
            SObj.SetFlagsStream(self.ExtractOriginalText(ctx.vfrSubtitleFlags()))

        ctx.Node.Buffer = gFormPkg.StructToStream(SObj.GetInfo())
        # sequence question
        for Ctx in ctx.vfrStatementSubTitleComponent():
            self.InsertChild(ctx.Node, Ctx)
        ctx.Node.Data = SObj
        self.InsertEndNode(ctx.Node, ctx.stop.line)

        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementSubTitleComponent.
    def visitVfrStatementSubTitleComponent(self, ctx:SourceVfrSyntaxParser.VfrStatementSubTitleComponentContext):

        self.visitChildren(ctx)
        if ctx.vfrStatementQuestions() != None:
            ctx.Node = ctx.vfrStatementQuestions().Node
        elif ctx.vfrStatementStat() != None:
            ctx.Node = ctx.vfrStatementStat().Node
        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrSubtitleFlags.
    def visitVfrSubtitleFlags(self, ctx:SourceVfrSyntaxParser.VfrSubtitleFlagsContext):

        self.visitChildren(ctx)

        for FlagsFieldCtx in ctx.subtitleFlagsField():
            ctx.SubFlags |= FlagsFieldCtx.Flag
        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#subtitleFlagsField.
    def visitSubtitleFlagsField(self, ctx:SourceVfrSyntaxParser.SubtitleFlagsFieldContext):

        if ctx.Number() != None:
            ctx.Flag = self.TransNum(ctx.Number())
        elif ctx.S != None:
            ctx.Flag = self.PreProcessDB.Read(ctx.S.text)
        elif ctx.H != None:
            ctx.Flag = 0x01

        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementStaticText.
    def visitVfrStatementStaticText(self, ctx:SourceVfrSyntaxParser.VfrStatementStaticTextContext):

        self.visitChildren(ctx)

        QId = EFI_QUESTION_ID_INVALID
        Help = self.PreProcessDB.Read(ctx.S1.text)
        ctx.Node.Dict['help'] = KV(ctx.S1.text, Help)
        Prompt = self.PreProcessDB.Read(ctx.S2.text)
        ctx.Node.Dict['prompt'] = KV(ctx.S2.text, Prompt)
        TxtTwo = EFI_STRING_ID_INVALID
        if ctx.S3 != None:
            TxtTwo = self.PreProcessDB.Read(ctx.S3.text)
            ctx.Node.Dict['text'] = KV(ctx.S3.text, TxtTwo)

        TextFlags = 0
        FlagsStream = ''
        for i in range (0, len(ctx.staticTextFlagsField())):
            TextFlags |= ctx.staticTextFlagsField(i).Flag
            FlagsStream += self.ExtractOriginalText(ctx.staticTextFlagsField(i))
            if i != len(ctx.staticTextFlagsField()) - 1:
                FlagsStream += ' | '

        if TextFlags & EFI_IFR_FLAG_CALLBACK:
            if TxtTwo != EFI_STRING_ID_INVALID:
                gVfrErrorHandle.HandleWarning(EFI_VFR_WARNING_CODE.VFR_WARNING_ACTION_WITH_TEXT_TWO, ctx.S3.line, ctx.S3.text)

            AObj = IfrAction()
            QId, _ = self.VfrQuestionDB.RegisterQuestion(None, None, QId, gFormPkg)
            AObj.SetLineNo(ctx.F.line)
            AObj.SetQuestionId(QId)
            AObj.SetHelp(Help)
            AObj.SetPrompt(Prompt)
            AObj.SetFlagsStream(FlagsStream)
            self.ErrorHandler(AObj.SetFlags(TextFlags), ctx.F.line)
            if ctx.Key() != None:
                AObj.SetHasKey(True)
                if ctx.N != None:
                    Key = self.TransNum(ctx.N.text)
                else:
                    Key = self.PreProcessDB.Read(ctx.S4.text)
                    ctx.Node.Dict['key'] = KV(ctx.S4.text, Key)
                self.AssignQuestionKey(AObj, Key)
            ctx.Node.Data = AObj
            ctx.Node.OpCode = EFI_IFR_TEXT_OP #
            ctx.Node.Buffer = gFormPkg.StructToStream(AObj.GetInfo())
            self.InsertEndNode(ctx.Node, ctx.stop.line)
        else:
            TObj = IfrText()
            Line = ctx.start.line
            TObj.SetLineNo(Line)
            TObj.SetHelp(Help)
            TObj.SetPrompt(Prompt)
            TObj.SetTextTwo(TxtTwo)
            ctx.Node.Data = TObj
            ctx.Node.Buffer = gFormPkg.StructToStream(TObj.GetInfo())

        return ctx.Node



    # Visit a parse tree produced by SourceVfrSyntaxParser#staticTextFlagsField.
    def visitStaticTextFlagsField(self, ctx:SourceVfrSyntaxParser.StaticTextFlagsFieldContext):

        self.visitChildren(ctx)

        if ctx.Number() != None:
            ctx.Flag = self.TransNum(ctx.N.text)
            if ctx.Flag != 0:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, ctx.N.line)
        elif ctx.questionheaderFlagsField() != None:
            ctx.Flag = ctx.questionheaderFlagsField().QHFlag
        elif ctx.S != None:
            ctx.Flag = self.PreProcessDB.Read(ctx.S.text)

        return ctx.Flag


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementCrossReference.
    def visitVfrStatementCrossReference(self, ctx:SourceVfrSyntaxParser.VfrStatementCrossReferenceContext):

        self.visitChildren(ctx)
        if ctx.vfrStatementGoto() != None:
            ctx.Node = ctx.vfrStatementGoto().Node
        elif ctx.vfrStatementResetButton() != None:
            ctx.Node = ctx.vfrStatementResetButton().Node
        return ctx.Node



    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementGoto.
    def visitVfrStatementGoto(self, ctx:SourceVfrSyntaxParser.VfrStatementGotoContext):

        RefType = 5
        DevPath = EFI_STRING_ID_INVALID
        QId = EFI_QUESTION_ID_INVALID
        BitMask = 0
        Line = ctx.start.line
        R5Obj = IfrRef5()
        R5Obj.SetLineNo(Line)
        GObj = R5Obj

        if ctx.DevicePath() != None:
            RefType = 4
            DevPath = self.PreProcessDB.Read(ctx.S.text)
            ctx.Node.Dict['devicepath'] = KV(ctx.S.text, DevPath)
            FormSetGuid = self.PreProcessDB.Read(ctx.SG1.text)
            ctx.Node.Dict['formsetguid'] = KV(ctx.SG1.text, FormSetGuid)
            if ctx.NF1 != None:
                FId = self.TransNum(ctx.NF1.text)
            else:
                FId = self.PreProcessDB.Read(ctx.SF1.text)
                ctx.Node.Dict['formid'] = KV(ctx.SF1.text, FId)

            if ctx.NQ1 != None:
                QId = self.TransNum(ctx.NQ1.text)
            else:
                QId = self.PreProcessDB.Read(ctx.SQ1.text)
                ctx.Node.Dict['questionid'] = KV(ctx.SQ1.text, QId)
            R4Obj = IfrRef4()
            R4Obj.SetLineNo(Line)
            R4Obj.SetDevicePath(DevPath)
            R4Obj.SetFormId(FId)
            R4Obj.SetQId(QId)
            R4Obj.SetFormSetId(FormSetGuid)
            GObj = R4Obj

        elif ctx.FormSetGuid() != None:
            RefType = 3
            FormSetGuid = self.PreProcessDB.Read(ctx.SG2.text)
            ctx.Node.Dict['formsetguid'] = KV(ctx.SG2.text, FormSetGuid)
            if ctx.NF2 != None:
                FId = self.TransNum(ctx.NF2.text)
            else:
                FId = self.PreProcessDB.Read(ctx.SF2.text)
                ctx.Node.Dict['formid'] = KV(ctx.SF2.text, FId)

            if ctx.NQ2 != None:
                QId = self.TransNum(ctx.NQ2.text)
            else:
                QId = self.PreProcessDB.Read(ctx.SQ2.text)
                ctx.Node.Dict['questionid'] = KV(ctx.SQ2.text, QId)
            R3Obj = IfrRef3()
            R3Obj.SetLineNo(Line)
            R3Obj.SetFormId(FId)
            R3Obj.SetQId(QId)
            R3Obj.SetFormSetId(FormSetGuid)
            GObj = R3Obj

        elif ctx.Question() != None:
            if ctx.NF3 != None:
                FId = self.TransNum(ctx.NF3.text)
            else:
                FId = self.PreProcessDB.Read(ctx.SF3.text)
                ctx.Node.Dict['formid'] = KV(ctx.SF3.text, FId)
            RefType = 2
            if ctx.SQ3 != None:
                Name = ctx.SQ3.text
                QId, BitMask, _ = self.VfrQuestionDB.GetQuestionId(Name)
                if QId == EFI_QUESTION_ID_INVALID:
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNDEFINED, ctx.SQ3.line)
            else:
                QId = self.TransNum(ctx.NQ3.text)
            R2Obj = IfrRef2()
            R2Obj.SetLineNo(Line)
            R2Obj.SetFormId(FId)
            R2Obj.SetQId(QId)
            GObj = R2Obj

        elif ctx.N1 != None or ctx.S1 != None:
            RefType = 1
            if ctx.N1 != None:
                FId = self.TransNum(ctx.N1.text)
            else:
                FId = self.PreProcessDB.Read(ctx.S1.text)
                ctx.Node.Dict['formid'] = KV(ctx.S1.text, FId)
            RObj = IfrRef()
            RObj.SetLineNo(Line)
            RObj.SetFormId(FId)
            GObj = RObj

        ctx.Node.Data = GObj

        self.visitChildren(ctx)

        if self.CurrQestVarInfo.VarType == EFI_IFR_TYPE_OTHER:
            self.CurrQestVarInfo.VarType == EFI_IFR_TYPE_REF

        if ctx.FLAGS() != None:
            GObj.SetFlags(ctx.vfrGotoFlags().GotoFlags)
            GObj.SetFlagsStream(self.ExtractOriginalText(ctx.vfrGotoFlags()))

        if ctx.Key() != None:
            if ctx.KN != None:
                Key = self.TransNum(ctx.KN.text)
            else:
                Key = self.PreProcessDB.Read(ctx.SN.text)
                ctx.Node.Dict['key'] = KV(ctx.SN.text, Key)
            self.AssignQuestionKey(GObj, Key)
            GObj.SetHasKey(True)

        if ctx.vfrStatementQuestionOptionList() != None:
            GObj.SetScope(1)
            self.InsertEndNode(ctx.Node, ctx.E.line)
        ctx.Node.Buffer = gFormPkg.StructToStream(GObj.GetInfo())
        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrGotoFlags.
    def visitVfrGotoFlags(self, ctx:SourceVfrSyntaxParser.VfrGotoFlagsContext):

        self.visitChildren(ctx)
        for FlagsFieldCtx in ctx.gotoFlagsField():
            ctx.GotoFlags |= FlagsFieldCtx.Flag

        return ctx.GotoFlags



    # Visit a parse tree produced by SourceVfrSyntaxParser#gotoFlagsField.
    def visitGotoFlagsField(self, ctx:SourceVfrSyntaxParser.GotoFlagsFieldContext):

        self.visitChildren(ctx)

        if ctx.N != None:
            if self.TransNum(ctx.N.text) != 0:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, ctx.N.line)
        elif ctx.questionheaderFlagsField() != None:
            ctx.Flag = ctx.questionheaderFlagsField().QHFlag
        elif ctx.S != None:
            ctx.Flag = self.PreProcessDB.Read(ctx.S.text)

        return ctx.Flag


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementResetButton.
    def visitVfrStatementResetButton(self, ctx:SourceVfrSyntaxParser.VfrStatementResetButtonContext):
        Defaultstore = ctx.N.text
        RBObj = IfrResetButton(Defaultstore)
        ctx.Node.Data = RBObj
        self.visitChildren(ctx)
        Line = ctx.start.line
        RBObj.SetLineNo(Line)
        DefaultId, ReturnCode = gVfrDefaultStore.GetDefaultId(Defaultstore)
        self.ErrorHandler(ReturnCode, ctx.N.line)
        RBObj.SetDefaultId(DefaultId)

        ctx.Node.Buffer = gFormPkg.StructToStream(RBObj.GetInfo())
        self.InsertEndNode(ctx.Node, ctx.stop.line)

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementQuestions.
    def visitVfrStatementQuestions(self, ctx:SourceVfrSyntaxParser.VfrStatementQuestionsContext):

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

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementQuestionTag.
    def visitVfrStatementQuestionTag(self, ctx:SourceVfrSyntaxParser.VfrStatementQuestionTagContext):

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


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementInconsistentIf.
    def visitVfrStatementInconsistentIf(self, ctx:SourceVfrSyntaxParser.VfrStatementInconsistentIfContext):

        IIObj = IfrInconsistentIf2()
        self.visitChildren(ctx)
        IIObj.SetLineNo(ctx.start.line)
        Prompt = self.PreProcessDB.Read(ctx.S.text)
        ctx.Node.Dict['prompt'] = KV(ctx.S.text, Prompt)
        IIObj.SetError(Prompt)

        if ctx.F != None:
            Flags = ''
            for i in range(0, len(ctx.flagsField())):
                Flags += self.ExtractOriginalText(ctx.flagsField(i))
                if i != len(ctx.flagsField()) - 1:
                    Flags += ' | '

            ctx.Node.Dict['flags'] = KV(Flags, None)

        ctx.Node.Data = IIObj
        ctx.Node.Buffer = gFormPkg.StructToStream(IIObj.GetInfo())
        ctx.Node.Expression = self.ExtractOriginalText(ctx.vfrStatementExpression())
        self.InsertEndNode(ctx.Node, ctx.stop.line)

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementNoSubmitIf.
    def visitVfrStatementNoSubmitIf(self, ctx:SourceVfrSyntaxParser.VfrStatementNoSubmitIfContext):
        self.visitChildren(ctx)
        NSIObj = IfrNoSubmitIf()

        NSIObj.SetLineNo(ctx.start.line)
        Prompt = self.PreProcessDB.Read(ctx.S.text)
        ctx.Node.Dict['prompt'] = KV(ctx.S.text, Prompt)
        NSIObj.SetError(Prompt)

        if ctx.F != None:
            Flags = ''
            for i in range(0, len(ctx.flagsField())):
                Flags += self.ExtractOriginalText(ctx.flagsField(i))
                if i != len(ctx.flagsField()) - 1:
                    Flags += ' | '

            ctx.Node.Dict['flags'] = KV(Flags, None)

        ctx.Node.Data = NSIObj
        ctx.Node.Buffer = gFormPkg.StructToStream(NSIObj.GetInfo())
        ctx.Node.Expression = self.ExtractOriginalText(ctx.vfrStatementExpression())
        self.InsertEndNode(ctx.Node, ctx.stop.line)

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementDisableIfQuest.
    def visitVfrStatementDisableIfQuest(self, ctx:SourceVfrSyntaxParser.VfrStatementDisableIfQuestContext):
        DIObj = IfrDisableIf()
        self.visitChildren(ctx)

        DIObj.SetLineNo(ctx.start.line)
        ctx.Node.Data = DIObj
        ctx.Node.Buffer = gFormPkg.StructToStream(DIObj.GetInfo())
        ctx.Node.Condition = 'disableif' + ' ' + self.ExtractOriginalText(ctx.vfrStatementExpression())
        #ctx.Node.Expression = self.ExtractOriginalText(ctx.vfrStatementExpression())
        self.InsertEndNode(ctx.Node, ctx.stop.line)

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementRefresh.
    def visitVfrStatementRefresh(self, ctx:SourceVfrSyntaxParser.VfrStatementRefreshContext):
        RObj = IfrRefresh()
        self.visitChildren(ctx)
        RObj.SetLineNo(ctx.start.line)
        if ctx.N != None:
            Interval = self.TransNum(ctx.N.text)
        else:
            Interval = self.PreProcessDB.Read(ctx.S.text)
            ctx.Node.Dict['interval'] = KV(ctx.S.text, Interval)
        RObj.SetRefreshInterval(Interval)
        ctx.Node.Data = RObj
        ctx.Node.Buffer = gFormPkg.StructToStream(RObj.GetInfo())

        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementVarstoreDevice.
    def visitVfrStatementVarstoreDevice(self, ctx:SourceVfrSyntaxParser.VfrStatementVarstoreDeviceContext):
        VDObj = IfrVarStoreDevice()
        self.visitChildren(ctx)

        VDObj.SetLineNo(ctx.start.line)
        DevPath = self.PreProcessDB.Read(ctx.S.text)
        ctx.Node.Dict['devicepath'] = KV(ctx.S.text, DevPath)
        VDObj.SetDevicePath(DevPath)
        ctx.Node.Data = VDObj
        ctx.Node.Buffer = gFormPkg.StructToStream(VDObj.GetInfo())

        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementRefreshEvent.
    def visitVfrStatementRefreshEvent(self, ctx:SourceVfrSyntaxParser.VfrStatementRefreshEventContext):
        RiObj = IfrRefreshId()
        self.visitChildren(ctx)

        RiObj.SetLineNo(ctx.start.line)
        Guid = self.PreProcessDB.Read(ctx.S.text)
        ctx.Node.Dict['refreshguid'] = KV(ctx.S.text, Guid)
        RiObj.SetRefreshEventGroutId(Guid)
        ctx.Node.Data = RiObj
        ctx.Node.Buffer = gFormPkg.StructToStream(RiObj.GetInfo())

        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementWarningIf.
    def visitVfrStatementWarningIf(self, ctx:SourceVfrSyntaxParser.VfrStatementWarningIfContext):
        WIObj = IfrWarningIf()
        self.visitChildren(ctx)

        WIObj.SetLineNo(ctx.start.line)
        Prompt = self.PreProcessDB.Read(ctx.S.text)
        ctx.Node.Dict['prompt'] = KV(ctx.S.text, Prompt)
        WIObj.SetWarning(Prompt)
        if ctx.Timeout() != None:
            if ctx.N != None:
                TimeOut = self.TransNum(ctx.N.text)
            else:
                TimeOut = self.PreProcessDB.Read(ctx.ST.text)
                ctx.Node.Dict['timeout'] = KV(ctx.ST.text, TimeOut)
            WIObj.SetTimeOut(TimeOut)
            WIObj.SetHasHasTimeOut(True)
        ctx.Node.Data = WIObj
        ctx.Node.Buffer = gFormPkg.StructToStream(WIObj.GetInfo())
        ctx.Node.Expression = self.ExtractOriginalText(ctx.vfrStatementExpression())
        self.InsertEndNode(ctx.Node, ctx.stop.line)
        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementQuestionTagList.
    def visitVfrStatementQuestionTagList(self, ctx:SourceVfrSyntaxParser.VfrStatementQuestionTagListContext):

        self.visitChildren(ctx)
        for Ctx in ctx.vfrStatementQuestionTag():
            self.InsertChild(ctx.Node, Ctx)
        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementQuestionOptionTag.
    def visitVfrStatementQuestionOptionTag(self, ctx:SourceVfrSyntaxParser.VfrStatementQuestionOptionTagContext):

        self.visitChildren(ctx)
        if ctx.vfrStatementSuppressIfQuest() != None:
            ctx.Node = ctx.vfrStatementSuppressIfQuest().Node

        if ctx.vfrStatementGrayOutIfQuest() != None:
            ctx.Node = ctx.vfrStatementGrayOutIfQuest().Node

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


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementSuppressIfQuest.
    def visitVfrStatementSuppressIfQuest(self, ctx:SourceVfrSyntaxParser.VfrStatementSuppressIfQuestContext):

        SIObj = IfrSuppressIf()
        SIObj.SetLineNo(ctx.start.line)
        ctx.Node.Data = SIObj
        ctx.Node.Condition = 'suppressif' + ' ' + self.ExtractOriginalText(ctx.vfrStatementExpression())
        self.visitChildren(ctx)
        ctx.Node.Buffer = gFormPkg.StructToStream(SIObj.GetInfo())
        self.InsertEndNode(ctx.Node, ctx.stop.line)
        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementGrayOutIfQuest.
    def visitVfrStatementGrayOutIfQuest(self, ctx:SourceVfrSyntaxParser.VfrStatementGrayOutIfQuestContext):
        GOIObj = IfrGrayOutIf()
        GOIObj.SetLineNo(ctx.start.line)
        ctx.Node.Data = GOIObj
        ctx.Node.Condition = 'grayoutif' + ' ' + self.ExtractOriginalText(ctx.vfrStatementExpression())
        self.visitChildren(ctx)
        ctx.Node.Buffer = gFormPkg.StructToStream(GOIObj.GetInfo())
        self.InsertEndNode(ctx.Node, ctx.stop.line)
        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#flagsField.
    def visitFlagsField(self, ctx:SourceVfrSyntaxParser.FlagsFieldContext):

        if ctx.N != None:
            VfrErrorHandle.HandleWarning(EFI_VFR_WARNING_CODE.VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE, ctx.N.line, ctx.N.text)
        if ctx.L != None:
            VfrErrorHandle.HandleWarning(EFI_VFR_WARNING_CODE.VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE, ctx.L.line, ctx.L.text)

        return self.visitChildren(ctx)

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementDefault.
    def visitVfrStatementDefault(self, ctx:SourceVfrSyntaxParser.VfrStatementDefaultContext):

        self.visitChildren(ctx)
        IsExp = False
        DefaultId = EFI_HII_DEFAULT_CLASS_STANDARD
        Line = ctx.start.line

        if ctx.vfrConstantValueField() != None:
            ValueList = ctx.vfrConstantValueField().ValueList
            Value = ValueList[0]
            Type = self.CurrQestVarInfo.VarType


            if self.CurrentMinMaxData != None and self.CurrentMinMaxData.IsNumericOpcode():
                # check default value is valid for Numeric Opcode
                for i in range(0, len(ValueList)):
                    Value = ValueList[i]
                    if type(Value) == int:
                        if Value < self.CurrentMinMaxData.GetMinData() or Value > self.CurrentMinMaxData.GetMaxData():
                            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, Line, "Numeric default value must be between MinValue and MaxValue.")

            if Type == EFI_IFR_TYPE_OTHER:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_FATAL_ERROR, Line, "Default data type error.")

            DObj = IfrDefault(Type, ValueList)
            DObj.SetLineNo(Line)
            DObj.SetValueStream(self.ExtractOriginalText(ctx.vfrConstantValueField()))
            if not ctx.vfrConstantValueField().ListType:

                if self.IsStringOp:
                    DObj.SetType(EFI_IFR_TYPE_STRING)
                else:
                    if self.CurrQestVarInfo.IsBitVar:
                        DObj.SetType(EFI_IFR_TYPE_NUM_SIZE_32)
                    else:
                        DObj.SetType(self.CurrQestVarInfo.VarType)
            else:
                DObj.SetType(EFI_IFR_TYPE_BUFFER)

        else:
            IsExp = True
            DObj = IfrDefault2()
            DObj.SetLineNo(Line)
            DObj.SetScope(1)
            self.InsertChild(ctx.Node, ctx.vfrStatementValue())
            self.InsertEndNode(ctx.Node, Line)

        if ctx.DefaultStore() != None:
            DefaultId, ReturnCode = gVfrDefaultStore.GetDefaultId(ctx.SN.text)
            self.ErrorHandler(ReturnCode, ctx.SN.line, ctx.SN.text)
            DObj.SetDefaultId(DefaultId)
            DObj.SetDefaultStore(ctx.SN.text)

        self.CheckDuplicateDefaultValue(DefaultId, ctx.D.line, ctx.D.text)
        if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            VarStoreName, ReturnCode = gVfrDataStorage.GetVarStoreName(self.CurrQestVarInfo.VarStoreId)
            self.ErrorHandler(ReturnCode, Line)
            VarGuid = gVfrDataStorage.GetVarStoreGuid(self.CurrQestVarInfo.VarStoreId)
            VarStoreType = gVfrDataStorage.GetVarStoreType(self.CurrQestVarInfo.VarStoreId)
            if (IsExp == False) and (VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER):
                self.ErrorHandler(gVfrDefaultStore.BufferVarStoreAltConfigAdd(DefaultId, self.CurrQestVarInfo, VarStoreName, VarGuid, self.CurrQestVarInfo.VarType, Value), Line)
        ctx.Node.Data = DObj
        ctx.Node.Buffer = gFormPkg.StructToStream(DObj.GetInfo())
        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementValue.
    def visitVfrStatementValue(self, ctx:SourceVfrSyntaxParser.VfrStatementValueContext):

        VObj = IfrValue()
        self.visitChildren(ctx)

        VObj.SetLineNo(ctx.start.line)
        ctx.Node.Data = VObj
        ctx.Node.Buffer = gFormPkg.StructToStream(VObj.GetInfo())
        ctx.Node.Expression = self.ExtractOriginalText(ctx.vfrStatementExpression())
        self.InsertEndNode(ctx.Node, ctx.stop.line)

        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementOptions.
    def visitVfrStatementOptions(self, ctx:SourceVfrSyntaxParser.VfrStatementOptionsContext):

        self.visitChildren(ctx)
        ctx.Node = ctx.vfrStatementOneOfOption().Node
        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementOneOfOption.
    def visitVfrStatementOneOfOption(self, ctx:SourceVfrSyntaxParser.VfrStatementOneOfOptionContext):

        Line = ctx.start.line
        if self.CurrQestVarInfo.VarType == EFI_IFR_TYPE_OTHER:
            print("Get data type error.")
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_FATAL_ERROR, Line, "Get data type error.")

        self.visitChildren(ctx)

        ValueList = ctx.vfrConstantValueField().ValueList
        Value = ValueList[0]
        Type = self.CurrQestVarInfo.VarType
        if self.CurrentMinMaxData != None:
            #set min/max value for oneof opcode
            Step = self.CurrentMinMaxData.GetStepData()
            self.CurrentMinMaxData.SetMinMaxStepData(Value, Value, Step)

        if self.CurrQestVarInfo.IsBitVar:
            Type = EFI_IFR_TYPE_NUM_SIZE_32
        OOOObj = IfrOneOfOption(Type, ValueList)

        if not ctx.vfrConstantValueField().ListType:
            OOOObj.SetType(Type)
        else:
            OOOObj.SetType(EFI_IFR_TYPE_BUFFER)

        Text = self.PreProcessDB.Read(ctx.S.text)
        ctx.Node.Dict['text'] = KV(ctx.S.text, Text)

        OOOObj.SetLineNo(Line)
        OOOObj.SetOption(Text)
        OOOObj.SetValueStream(self.ExtractOriginalText(ctx.vfrConstantValueField()))
        OOOObj.SetFlagsStream(self.ExtractOriginalText(ctx.vfrOneOfOptionFlags()))
        self.ErrorHandler(OOOObj.SetFlags(ctx.vfrOneOfOptionFlags().LFlags), ctx.F.line)
        self.ErrorHandler(self.CurrentQuestion.SetQHeaderFlags(ctx.vfrOneOfOptionFlags().HFlags), ctx.F.line)

        # Array type only for default type OneOfOption.
        if (OOOObj.GetFlags() & (EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG)) == 0 and (ctx.vfrConstantValueField().ListType):
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_FATAL_ERROR, Line, "Default keyword should with array value type!")

        # Clear the default flag if the option not use array value but has default flag.
        if (OOOObj.GetFlags() & (EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG)) != 0 and (ctx.vfrConstantValueField().ListType == False) and (self.IsOrderedList):
            OOOObj.SetFlags(OOOObj.GetFlags() & ~(EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG))

        if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            VarStoreName, ReturnCode = gVfrDataStorage.GetVarStoreName(self.CurrQestVarInfo.VarStoreId)
            self.ErrorHandler(ReturnCode, Line)
            VarStoreGuid = gVfrDataStorage.GetVarStoreGuid(self.CurrQestVarInfo.VarStoreId)
            if OOOObj.GetFlags() & EFI_IFR_OPTION_DEFAULT:
                self.CheckDuplicateDefaultValue(EFI_HII_DEFAULT_CLASS_STANDARD, ctx.F.line, ctx.F.text)
                self.ErrorHandler(gVfrDefaultStore.BufferVarStoreAltConfigAdd(EFI_HII_DEFAULT_CLASS_STANDARD, self.CurrQestVarInfo, VarStoreName, VarStoreGuid, self.CurrQestVarInfo.VarType, Value), Line)
            if OOOObj.GetFlags() & EFI_IFR_OPTION_DEFAULT_MFG:
                self.CheckDuplicateDefaultValue(EFI_HII_DEFAULT_CLASS_MANUFACTURING, ctx.F.line, ctx.F.text)
                self.ErrorHandler(gVfrDefaultStore.BufferVarStoreAltConfigAdd(EFI_HII_DEFAULT_CLASS_MANUFACTURING, self.CurrQestVarInfo, VarStoreName, VarStoreGuid, self.CurrQestVarInfo.VarType, Value), Line)

        if ctx.Key() != None:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, ctx.KN.line, ctx.KN.text)
            #　Guid Option Key
            if ctx.KN != None:
                Key = self.TransNum(ctx.KN.text)
            else:
                Key = self.PreProcessDB.Read(ctx.KS.text)
                ctx.Node.Dict['key'] = KV(ctx.KS.text, Key)
            OOOObj.SetIfrOptionKey(Key)
            gIfrOptionKey = IfrOptionKey(self.CurrentQuestion.GetQuestionId(), Type, Value, self.TransNum(ctx.KN.text))
            Node = VfrTreeNode(EFI_IFR_GUID_OP, gIfrOptionKey, gFormPkg.StructToStream(gIfrOptionKey.GetInfo()))
            gIfrOptionKey.SetLineNo()
            ctx.Node.insertChild(Node)

        for Ctx in ctx.vfrImageTag():
            OOOObj.SetScope(1)
            self.InsertChild(ctx.Node, Ctx)
            self.InsertEndNode(ctx.Node, ctx.T.line)

        ctx.Node.Data = OOOObj
        ctx.Node.Buffer = gFormPkg.StructToStream(OOOObj.GetInfo())

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrOneOfOptionFlags.
    def visitVfrOneOfOptionFlags(self, ctx:SourceVfrSyntaxParser.VfrOneOfOptionFlagsContext):

        self.visitChildren(ctx)

        ctx.LFlags = self.CurrQestVarInfo.VarType
        for FlagsFieldCtx in ctx.oneofoptionFlagsField():
            ctx.HFlags |= FlagsFieldCtx.HFlag
            ctx.LFlags |= FlagsFieldCtx.LFlag

        return ctx.HFlags, ctx.LFlags


    # Visit a parse tree produced by SourceVfrSyntaxParser#oneofoptionFlagsField.
    def visitOneofoptionFlagsField(self, ctx:SourceVfrSyntaxParser.OneofoptionFlagsFieldContext):

        self.visitChildren(ctx)

        if ctx.Number() != None:
            ctx.LFlag = self.TransNum(ctx.Number())
        elif ctx.OptionDefault() != None:
            ctx.LFlag = 0x10
        elif ctx.OptionDefaultMfg() != None:
            ctx.LFlag = 0x20
        elif ctx.InteractiveFlag() != None:
            ctx.HFlag = 0x04
        elif ctx.ResetRequiredFlag() != None:
            ctx.HFlag = 0x10
        elif ctx.RestStyleFlag() != None:
            ctx.HFlag = 0x20
        elif ctx.ReconnectRequiredFlag() != None:
            ctx.HFlag = 0x40
        elif ctx.ManufacturingFlag() != None:
            ctx.LFlag = 0x20
        elif ctx.DefaultFlag() != None:
            ctx.LFlag = 0x10
        elif ctx.NVAccessFlag() != None:
            gVfrErrorHandle.HandleWarning (EFI_VFR_WARNING_CODE.VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE, ctx.A.line, ctx.A.text)
        elif ctx.LateCheckFlag() != None:
            gVfrErrorHandle.HandleWarning (EFI_VFR_WARNING_CODE.VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE, ctx.L.line, ctx.L.text)
        elif ctx.S != None:
            ctx.LFlag = self.PreProcessDB.Read(ctx.S.text)

        return ctx.HFlag, ctx.LFlag


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementRead.
    def visitVfrStatementRead(self, ctx:SourceVfrSyntaxParser.VfrStatementReadContext):

        RObj = IfrRead()
        self.visitChildren(ctx)

        RObj.SetLineNo(ctx.start.line)
        ctx.Node.Data = RObj
        ctx.Node.Buffer = gFormPkg.StructToStream(RObj.GetInfo())
        ctx.Node.Expression = self.ExtractOriginalText(ctx.vfrStatementExpression())

        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementWrite.
    def visitVfrStatementWrite(self, ctx:SourceVfrSyntaxParser.VfrStatementWriteContext):

        WObj = IfrWrite()
        self.visitChildren(ctx)

        WObj.SetLineNo(ctx.start.line)
        ctx.Node.Data = WObj
        ctx.Node.Buffer = gFormPkg.StructToStream(WObj.GetInfo())
        ctx.Node.Expression = self.ExtractOriginalText(ctx.vfrStatementExpression())

        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementQuestionOptionList.
    def visitVfrStatementQuestionOptionList(self, ctx:SourceVfrSyntaxParser.VfrStatementQuestionOptionListContext):

        self.visitChildren(ctx)
        for Ctx in ctx.vfrStatementQuestionOption():
            self.InsertChild(ctx.Node, Ctx)

        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementQuestionOption.
    def visitVfrStatementQuestionOption(self, ctx:SourceVfrSyntaxParser.VfrStatementQuestionOptionContext):

        self.visitChildren(ctx)
        if ctx.vfrStatementQuestionTag() != None:
            ctx.Node = ctx.vfrStatementQuestionTag().Node

        elif ctx.vfrStatementQuestionOptionTag() != None:
            ctx.Node = ctx.vfrStatementQuestionOptionTag().Node
        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementBooleanType.
    def visitVfrStatementBooleanType(self, ctx:SourceVfrSyntaxParser.VfrStatementBooleanTypeContext):

        self.visitChildren(ctx)
        if ctx.vfrStatementCheckBox() != None:
            if self.CurrQestVarInfo.IsBitVar:
                ctx.Node = ctx.vfrStatementCheckBox().GuidNode
            else:
                ctx.Node = ctx.vfrStatementCheckBox().Node
        else:
            ctx.Node = ctx.vfrStatementAction().Node

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementCheckBox.
    def visitVfrStatementCheckBox(self, ctx:SourceVfrSyntaxParser.VfrStatementCheckBoxContext):

        CBObj = IfrCheckBox()
        ctx.Node.Data = CBObj
        Line =  ctx.start.line
        CBObj.SetLineNo(Line)
        self.CurrentQuestion = CBObj
        self.IsCheckBoxOp = True

        self.visitChildren(ctx)

        # Create a GUID opcode to wrap the checkbox opcode, if it refer to bit varstore.
        if self.CurrQestVarInfo.IsBitVar:
            GuidObj = IfrGuid(0)
            GuidObj.SetGuid(EDKII_IFR_BIT_VARSTORE_GUID)
            GuidObj.SetLineNo(Line)
            GuidObj.SetScope(1) # position
            ctx.GuidNode.Data = GuidObj
            ctx.GuidNode.Buffer = gFormPkg.StructToStream(GuidObj.GetInfo())
            ctx.GuidNode.insertChild(ctx.Node)
            #ctx.GuidNode.Parent.insertChild(ctx.Node)
            self.InsertEndNode(ctx.GuidNode, ctx.stop.line)

        # check dataType
        if self.CurrQestVarInfo.VarType == EFI_IFR_TYPE_OTHER:
            self.CurrQestVarInfo.VarType = EFI_IFR_TYPE_BOOLEAN

        if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            # Check whether the question refers to a bit field, if yes. create a Guid to indicate the question refers to a bit field.
            if self.CurrQestVarInfo.IsBitVar:
                _, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.CurrQestVarInfo.VarType)
                self.ErrorHandler(ReturnCode, Line, "CheckBox varid is not the valid data type")
                if gVfrDataStorage.GetVarStoreType(self.CurrQestVarInfo.VarStoreId) == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER_BITS and self.CurrQestVarInfo.VarTotalSize != 1:
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, Line, "CheckBox varid only occupy 1 bit in Bit Varstore")
                else:
                    Size, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.CurrQestVarInfo.VarType)
                    self.ErrorHandler(ReturnCode, Line, "CheckBox varid is not the valid data type")
                    if Size != 0 and Size != self.CurrQestVarInfo.VarTotalSize:
                        self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, Line, "CheckBox varid doesn't support array")
                    elif gVfrDataStorage.GetVarStoreType(self.CurrQestVarInfo.VarStoreId) == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER and self.CurrQestVarInfo.VarTotalSize != sizeof(ctypes.c_bool):
                        self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, Line, "CheckBox varid only support BOOLEAN data type")

        if ctx.FLAGS() != None:
            CBObj.SetFlagsStream(self.ExtractOriginalText(ctx.vfrCheckBoxFlags()))
            CBObj.SetFlags(ctx.vfrCheckBoxFlags().HFlags, ctx.vfrCheckBoxFlags().LFlags)
            if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
                VarStoreName, ReturnCode = gVfrDataStorage.GetVarStoreName(self.CurrQestVarInfo.VarStoreId)
                self.CompareErrorHandler(ReturnCode, VfrReturnCode.VFR_RETURN_SUCCESS, Line, ctx.L.text, "Failed to retrieve varstore name")

                VarStoreGuid = gVfrDataStorage.GetVarStoreGuid(self.CurrQestVarInfo.VarStoreId)
                self.Value = True
                if CBObj.GetFlags() & 0x01:
                    self.CheckDuplicateDefaultValue(EFI_HII_DEFAULT_CLASS_STANDARD, ctx.F.line, ctx.F.text)
                    ReturnCode = gVfrDefaultStore.BufferVarStoreAltConfigAdd(EFI_HII_DEFAULT_CLASS_STANDARD,self.CurrQestVarInfo, VarStoreName, VarStoreGuid, self.CurrQestVarInfo.VarType, self.Value)
                    self.CompareErrorHandler(ReturnCode, VfrReturnCode.VFR_RETURN_SUCCESS, Line, ctx.L.text, "No standard default storage found")
                if CBObj.GetFlags() & 0x02:
                    self.CheckDuplicateDefaultValue(EFI_HII_DEFAULT_CLASS_MANUFACTURING, ctx.F.line, ctx.F.text)
                    ReturnCode =  gVfrDefaultStore.BufferVarStoreAltConfigAdd(EFI_HII_DEFAULT_CLASS_MANUFACTURING, self.CurrQestVarInfo, VarStoreName, VarStoreGuid, self.CurrQestVarInfo.VarType, self.Value)
                    self.CompareErrorHandler(ReturnCode, VfrReturnCode.VFR_RETURN_SUCCESS, Line, ctx.L.text, "No manufacturing default storage found")

        if ctx.Key() != None:
            if ctx.N != None:
                Key = self.TransNum(ctx.N.text)
            else:
                Key = self.PreProcessDB.Read(ctx.S.text)
                ctx.Node.Dict['key'] = KV(ctx.S.text, Key)
            self.AssignQuestionKey(CBObj, Key)
        ctx.Node.Buffer = gFormPkg.StructToStream(CBObj.GetInfo())
        self.InsertEndNode(ctx.Node, ctx.stop.line)
        self.IsCheckBoxOp = False

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrCheckBoxFlags.
    def visitVfrCheckBoxFlags(self, ctx:SourceVfrSyntaxParser.VfrCheckBoxFlagsContext):

        self.visitChildren(ctx)
        for FlagsFieldCtx in ctx.checkboxFlagsField():
            ctx.LFlags |= FlagsFieldCtx.LFlag
            ctx.HFlags |= FlagsFieldCtx.HFlag

        return ctx.HFlags, ctx.LFlags


    # Visit a parse tree produced by SourceVfrSyntaxParser#checkboxFlagsField.
    def visitCheckboxFlagsField(self, ctx:SourceVfrSyntaxParser.CheckboxFlagsFieldContext):

        self.visitChildren(ctx)

        if ctx.Number() != None:
            if self.TransNum(ctx.Number()) != 0:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, ctx.start.line)
        elif ctx.DefaultFlag() != None:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, ctx.D.line, ctx.D.text)
        elif ctx.ManufacturingFlag() != None:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, ctx.M.line, ctx.M.text)
        elif ctx.CheckBoxDefaultFlag() != None:
            ctx.LFlag = 0x01
        elif ctx.CheckBoxDefaultMfgFlag() != None:
            ctx.LFlag = 0x02
        elif ctx.questionheaderFlagsField() != None:
            ctx.HFlag = ctx.questionheaderFlagsField().QHFlag
        elif ctx.S != None:
            if self.PreProcessDB.Read(ctx.S.text) != 0:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, ctx.start.line)


        return ctx.HFlag, ctx.LFlag

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementAction.
    def visitVfrStatementAction(self, ctx:SourceVfrSyntaxParser.VfrStatementActionContext):

        AObj = IfrAction()
        ctx.Node.Data = AObj
        self.visitChildren(ctx)
        AObj.SetLineNo(ctx.start.line)
        Config = self.PreProcessDB.Read(ctx.S.text)
        ctx.Node.Dict['config'] = KV(ctx.S.text, Config)
        AObj.SetQuestionConfig(Config)
        if ctx.FLAGS() != None:
            AObj.SetFlags(ctx.vfrActionFlags().HFlags)
        ctx.Node.Buffer = gFormPkg.StructToStream(AObj.GetInfo())
        self.InsertEndNode(ctx.Node, ctx.stop.line)

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrActionFlags.
    def visitVfrActionFlags(self, ctx:SourceVfrSyntaxParser.VfrActionFlagsContext):

        self.visitChildren(ctx)
        for FlagsFieldCtx in ctx.actionFlagsField():
            ctx.HFlags |= FlagsFieldCtx.HFlag

        return ctx.HFlags


    # Visit a parse tree produced by SourceVfrSyntaxParser#actionFlagsField.
    def visitActionFlagsField(self, ctx:SourceVfrSyntaxParser.ActionFlagsFieldContext):

        self.visitChildren(ctx)

        if ctx.Number() != None:
            if self.TransNum(ctx.Number()) != 0:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, ctx.N.line)
        elif ctx.questionheaderFlagsField() != 0:
            ctx.HFlag = ctx.questionheaderFlagsField().QHFlag
        elif ctx.S != None:
            if self.PreProcessDB.Read(ctx.S.text) != 0:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, ctx.S.line)
        return ctx.HFlag


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementNumericType.
    def visitVfrStatementNumericType(self, ctx:SourceVfrSyntaxParser.VfrStatementNumericTypeContext):

        self.visitChildren(ctx)
        if ctx.vfrStatementNumeric() != None:
            if self.CurrQestVarInfo.IsBitVar:
                ctx.Node = ctx.vfrStatementNumeric().GuidNode
            else:
                ctx.Node = ctx.vfrStatementNumeric().Node
        elif ctx.vfrStatementOneOf() != None:
            if self.CurrQestVarInfo.IsBitVar:
                ctx.Node = ctx.vfrStatementOneOf().GuidNode
            else:
                ctx.Node = ctx.vfrStatementOneOf().Node

        return ctx.Node


    # Visit a parse tree produced by VfrSyntaxP-arser#vfrStatementNumeric.
    def visitVfrStatementNumeric(self, ctx:SourceVfrSyntaxParser.VfrStatementNumericContext):

        self.visitChildren(ctx)
        NObj = ctx.Node.Data

        NObj.SetLineNo(ctx.start.line)
        Line = ctx.start.line
        UpdateVarType = False

        # Create a GUID opcode to wrap the numeric opcode, if it refer to bit varstore.
        if self.CurrQestVarInfo.IsBitVar:
            GuidObj = IfrGuid(0)
            GuidObj.SetGuid(EDKII_IFR_BIT_VARSTORE_GUID)
            GuidObj.SetLineNo(Line)
            GuidObj.SetScope(1) # pos
            ctx.GuidNode.Data = GuidObj
            ctx.GuidNode.Buffer = gFormPkg.StructToStream(GuidObj.GetInfo())
            ctx.GuidNode.insertChild(ctx.Node)
            self.InsertEndNode(ctx.GuidNode, ctx.stop.line)

        # check data type
        if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            if self.CurrQestVarInfo.IsBitVar:
                LFlags = EDKII_IFR_NUMERIC_SIZE_BIT & self.CurrQestVarInfo.VarTotalSize
                self.ErrorHandler(NObj.SetFlagsForBitField(NObj.GetQFlags(),LFlags), Line)
            else:
                DataTypeSize, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.CurrQestVarInfo.VarType)
                self.ErrorHandler(ReturnCode, Line, 'Numeric varid is not the valid data type')
                if DataTypeSize != 0 and DataTypeSize != self.CurrQestVarInfo.VarTotalSize:
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, Line, 'Numeric varid doesn\'t support array')
                self.ErrorHandler(NObj.SetFlags(NObj.GetQFlags(), self.CurrQestVarInfo.VarType), Line)

        if ctx.FLAGS() != None:
            UpdateVarType = ctx.vfrNumericFlags().UpdateVarType
            if self.CurrQestVarInfo.IsBitVar:
                self.ErrorHandler(NObj.SetFlagsForBitField(ctx.vfrNumericFlags().HFlags,ctx.vfrNumericFlags().LFlags, ctx.vfrNumericFlags().IsDisplaySpecified), ctx.F.line)
            else:
                self.ErrorHandler(NObj.SetFlags(ctx.vfrNumericFlags().HFlags,ctx.vfrNumericFlags().LFlags, ctx.vfrNumericFlags().IsDisplaySpecified), ctx.F.line)
            NObj.SetFlagsStream(self.ExtractOriginalText(ctx.vfrNumericFlags()))

        if ctx.Key() != None:
            if ctx.N != None:
                Key = self.TransNum(ctx.N.text)
            else:
                Key = self.PreProcessDB.Read(ctx.S.text)
                ctx.Node.Dict['key'] = KV(ctx.S.text, Key)
            self.AssignQuestionKey(NObj, Key)
            NObj.SetHasKey(True)

        NObj.SetHasStep(ctx.vfrSetMinMaxStep().Step() != None)

        if self.CurrQestVarInfo.IsBitVar == False:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, Line, 'Numeric question only support UINT8, UINT16, UINT32 and UINT64 data type.')

        # modify the data for namevalue
        if UpdateVarType:
            UpdatedNObj = IfrNumeric(self.CurrQestVarInfo.VarType)
            UpdatedNObj.QName = NObj.QName
            UpdatedNObj.VarIdStr = NObj.VarIdStr
            UpdatedNObj.HasQuestionId = NObj.HasQuestionId
            UpdatedNObj.FlagsStream = NObj.FlagsStream
            UpdatedNObj.HasKey = NObj.HasKey
            UpdatedNObj.HasStep = NObj.HasStep
            UpdatedNObj.LineNo = NObj.LineNo
            UpdatedNObj.GetInfo().Question = NObj.GetInfo().Question
            UpdatedNObj.GetInfo().Flags = NObj.GetInfo().Flags
            UpdatedNObj.GetInfo().Data.MinValue = NObj.GetInfo().Data.MinValue
            UpdatedNObj.GetInfo().Data.MaxValue = NObj.GetInfo().Data.MaxValue
            UpdatedNObj.GetInfo().Data.Step = NObj.GetInfo().Data.Step
            NObj = UpdatedNObj

        ctx.Node.Data = NObj
        ctx.Node.Buffer = gFormPkg.StructToStream(NObj.GetInfo())
        self.InsertEndNode(ctx.Node, ctx.stop.line)

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrSetMinMaxStep.
    def visitVfrSetMinMaxStep(self, ctx:SourceVfrSyntaxParser.VfrSetMinMaxStepContext):
        IntDecStyle = False
        OpObj = ctx.Node.Data
        if ((self.CurrQestVarInfo.IsBitVar) and (OpObj.GetOpCode() == EFI_IFR_NUMERIC_OP) and ((OpObj.GetNumericFlags() & EDKII_IFR_DISPLAY_BIT) == 0)) or \
            ((self.CurrQestVarInfo.IsBitVar == False) and (OpObj.GetOpCode() == EFI_IFR_NUMERIC_OP) and ((OpObj.GetNumericFlags() & EFI_IFR_DISPLAY) == 0)):
            IntDecStyle = True
        MinNegative = False
        MaxNegative = False

        self.visitChildren(ctx)
        Min = self.TransNum(ctx.I.text)
        Max = self.TransNum(ctx.A.text)
        Step = self.TransNum(ctx.S.text) if ctx.Step() != None else 0

        if ctx.N1 !=None:
            MinNegative = True

        if IntDecStyle == False and MinNegative == True:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.I.line, '\'-\' can\'t be used when not in int decimal type.')
        if self.CurrQestVarInfo.IsBitVar:
            if (IntDecStyle == False) and (Min > (1 << self.CurrQestVarInfo.VarTotalSize) - 1): #
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.I.line, 'BIT type minimum can\'t small than 0, bigger than 2^BitWidth -1')
            else:
                Type = self.CurrQestVarInfo.VarType
                if Type == EFI_IFR_TYPE_NUM_SIZE_64:
                    if IntDecStyle:
                        if MinNegative:
                            if Min > 0x8000000000000000:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.I.line, 'INT64 type minimum can\'t small than -0x8000000000000000, big than 0x7FFFFFFFFFFFFFFF')
                        else:
                            if Min > 0x7FFFFFFFFFFFFFFF:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.I.line, 'INT64 type minimum can\'t small than -0x8000000000000000, big than 0x7FFFFFFFFFFFFFFF')
                    if MinNegative:
                        Min = ~Min + 1

                if Type == EFI_IFR_TYPE_NUM_SIZE_32:
                    if IntDecStyle:
                        if MinNegative:
                            if Min > 0x80000000:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.I.line, 'INT32 type minimum can\'t small than -0x80000000, big than 0x7FFFFFFF')
                        else:
                            if Min > 0x7FFFFFFF:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.I.line, 'INT32 type minimum can\'t small than -0x80000000, big than 0x7FFFFFFF')
                    if MinNegative:
                        Min = ~Min + 1

                if Type == EFI_IFR_TYPE_NUM_SIZE_16:
                    if IntDecStyle:
                        if MinNegative:
                            if Min > 0x8000:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.I.line, 'INT16 type minimum can\'t small than -0x8000, big than 0x7FFF')
                        else:
                            if Min > 0x7FFF:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.I.line, 'INT16 type minimum can\'t small than -0x8000, big than 0x7FFF')
                    if MinNegative:
                        Min = ~Min + 1

                if Type == EFI_IFR_TYPE_NUM_SIZE_8:
                    if IntDecStyle:
                        if MinNegative:
                            if Min > 0x80:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.I.line, 'INT8 type minimum can\'t small than -0x80, big than 0x7F')
                        else:
                            if Min > 0x7F:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.I.line, 'INT8 type minimum can\'t small than -0x80, big than 0x7F')
                    if MinNegative:
                        Min = ~Min + 1

        if ctx.N2 != None:
            MaxNegative = True
        if IntDecStyle == False and MaxNegative == True:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.A.line, ' \'-\' can\'t be used when not in int decimal type.')
        if self.CurrQestVarInfo.IsBitVar:
            if (IntDecStyle == False) and (Max > (1 << self.CurrQestVarInfo.VarTotalSize) - 1):
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.A.line, 'BIT type maximum can\'t be bigger than 2^BitWidth -1')
            else:
                Type = self.CurrQestVarInfo.VarType
                if Type == EFI_IFR_TYPE_NUM_SIZE_64:
                    if IntDecStyle:
                        if MaxNegative:
                            if Max > 0x8000000000000000:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.A.line, 'INT64 type minimum can\'t small than -0x8000000000000000, big than 0x7FFFFFFFFFFFFFFF')
                        else:
                            if Max > 0x7FFFFFFFFFFFFFFF:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.A.line, 'INT64 type minimum can\'t small than -0x8000000000000000, big than 0x7FFFFFFFFFFFFFFF')
                    if MaxNegative:
                        Max = ~Max + 1

                    if Max < Min: #
                        self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.A.line, 'Maximum can\'t be less than Minimum')


                if Type == EFI_IFR_TYPE_NUM_SIZE_32:
                    if IntDecStyle:
                        if MaxNegative:
                            if Max > 0x80000000:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.A.line, 'INT32 type minimum can\'t small than -0x80000000, big than 0x7FFFFFFF')
                        else:
                            if Max > 0x7FFFFFFF:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.A.line, 'INT32 type minimum can\'t small than -0x80000000, big than 0x7FFFFFFF')
                    if MaxNegative:
                        Max = ~Max + 1

                    if Max < Min: #
                        self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.A.line, 'Maximum can\'t be less than Minimum')


                if Type == EFI_IFR_TYPE_NUM_SIZE_16:
                    if IntDecStyle:
                        if MaxNegative:
                            if Max > 0x8000:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.A.line, 'INT16 type minimum can\'t small than -0x8000, big than 0x7FFF')
                        else:
                            if Max > 0x7FFF:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.A.line, 'INT16 type minimum can\'t small than -0x8000, big than 0x7FFF')
                    if MaxNegative:
                        Max = ~Max + 1

                    if Max < Min: #
                        self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.A.line, 'Maximum can\'t be less than Minimum')

                if Type == EFI_IFR_TYPE_NUM_SIZE_8:
                    if IntDecStyle:
                        if MaxNegative:
                            if Max > 0x80:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.A.line, 'INT8 type minimum can\'t small than -0x80, big than 0x7F')
                        else:
                            if Max > 0x7F:
                                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.A.line, 'INT8 type minimum can\'t small than -0x80, big than 0x7F')
                    if MaxNegative:
                        Max = ~Max + 1

                    if Max < Min: #
                        self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.A.line, 'Maximum can\'t be less than Minimum')

        OpObj.SetMinMaxStepData(Min, Max, Step)
        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrNumericFlags.
    def visitVfrNumericFlags(self, ctx:SourceVfrSyntaxParser.VfrNumericFlagsContext):

        # check data type flag
        ctx.LFlags = self.CurrQestVarInfo.VarType & EFI_IFR_NUMERIC_SIZE
        VarStoreType = gVfrDataStorage.GetVarStoreType(self.CurrQestVarInfo.VarStoreId)
        Line = ctx.start.line
        IsSetType = False
        self.visitChildren(ctx)

        for FlagsFieldCtx in ctx.numericFlagsField():
            ctx.HFlags |= FlagsFieldCtx.HFlag
            ctx.IsDisplaySpecified |= FlagsFieldCtx.IsDisplaySpecified
            IsSetType |=  FlagsFieldCtx.IsSetType
            if FlagsFieldCtx.NumericSizeOne() != None:
                if self.CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_1

            if FlagsFieldCtx.NumericSizeTwo() != None:
                if self.CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_2

            if FlagsFieldCtx.NumericSizeFour() != None:
                if self.CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_4

            if FlagsFieldCtx.NumericSizeEight() != None:
                if self.CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_8

            if FlagsFieldCtx.DisPlayIntDec() != None:
                if self.CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_INT_DEC
                else:
                    ctx.LFlags =  (ctx.LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_INT_DEC_BIT

            if FlagsFieldCtx.DisPlayUIntDec() != None:
                if self.CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_UINT_DEC
                else:
                    ctx.LFlags =  (ctx.LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_UINT_DEC_BIT

            if FlagsFieldCtx.DisPlayUIntHex() != None:
                if self.CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_UINT_HEX
                else:
                    ctx.LFlags =  (ctx.LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_UINT_HEX_BIT

        VarType = self.CurrQestVarInfo.VarType
        if self.CurrQestVarInfo.IsBitVar == False:
            if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
                if VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER or VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_EFI:
                    if self.CurrQestVarInfo.VarType != (ctx.LFlags & EFI_IFR_NUMERIC_SIZE):
                        self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, Line, 'Numeric Flag is not same to Numeric VarData type')
                else:
                    # update data type for name/value store
                    self.CurrQestVarInfo.VarType = ctx.LFlags & EFI_IFR_NUMERIC_SIZE
                    Size, _ = gVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.CurrQestVarInfo.VarType)
                    self.CurrQestVarInfo.VarTotalSize = Size
            elif IsSetType:
                self.CurrQestVarInfo.VarType = ctx.LFlags & EFI_IFR_NUMERIC_SIZE

        elif self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID and self.CurrQestVarInfo.IsBitVar:
            ctx.LFlags &= EDKII_IFR_DISPLAY_BIT
            ctx.LFlags |= EDKII_IFR_NUMERIC_SIZE_BIT & self.CurrQestVarInfo.VarTotalSize

        if VarType != self.CurrQestVarInfo.VarType:
            ctx.UpdateVarType = True

        return ctx.HFlags, ctx.LFlags


    # Visit a parse tree produced by SourceVfrSyntaxParser#numericFlagsField.
    def visitNumericFlagsField(self, ctx:SourceVfrSyntaxParser.NumericFlagsFieldContext):

        self.visitChildren(ctx)

        Line = ctx.start.line
        if ctx.Number() != None:
            if self.TransNum(ctx.N.text) != 0:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, Line)

        elif ctx.NumericSizeOne() != None:
            if self.CurrQestVarInfo.IsBitVar == False:
                ctx.IsSetType = True
            else:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, Line, 'Can not specify the size of the numeric value for BIT field')

        elif ctx.NumericSizeTwo() != None:
            if self.CurrQestVarInfo.IsBitVar == False:
                ctx.IsSetType = True
            else:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, Line, 'Can not specify the size of the numeric value for BIT field')

        elif ctx.NumericSizeFour() != None:
            if self.CurrQestVarInfo.IsBitVar == False:
                ctx.IsSetType = True
            else:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, Line, 'Can not specify the size of the numeric value for BIT field')

        elif ctx.NumericSizeEight() != None:
            if self.CurrQestVarInfo.IsBitVar == False:
                ctx.IsSetType = True
            else:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, Line, 'Can not specify the size of the numeric value for BIT field')

        elif ctx.DisPlayIntDec() != None:
            ctx.IsDisplaySpecified = True

        elif ctx.DisPlayUIntDec() != None:
            ctx.IsDisplaySpecified = True

        elif ctx.DisPlayUIntHex() != None:
            ctx.IsDisplaySpecified = True

        elif ctx.questionheaderFlagsField() != None:
            ctx.HFlag = ctx.questionheaderFlagsField().QHFlag

        elif ctx.S != None:
            if self.PreProcessDB.Read(ctx.S.text) != 0:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, Line)

        return ctx.HFlag


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementOneOf.
    def visitVfrStatementOneOf(self, ctx:SourceVfrSyntaxParser.VfrStatementOneOfContext):

        self.visitChildren(ctx)
        UpdateVarType = False
        OObj = ctx.Node.Data
        Line = ctx.start.line
        if self.CurrQestVarInfo.IsBitVar:
            GuidObj = IfrGuid(0)
            GuidObj.SetGuid(EDKII_IFR_BIT_VARSTORE_GUID)
            GuidObj.SetLineNo(ctx.start.line)
            GuidObj.SetScope(1) # pos
            ctx.GuidNode.Data = GuidObj
            ctx.GuidNode.Buffer = gFormPkg.StructToStream(GuidObj.GetInfo())
            ctx.GuidNode.insertChild(ctx.Node)
            self.InsertEndNode(ctx.GuidNode, ctx.stop.line)

        # check data type
        if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            if self.CurrQestVarInfo.IsBitVar:
                LFlags = EDKII_IFR_NUMERIC_SIZE_BIT & self.CurrQestVarInfo.VarTotalSize
                self.ErrorHandler(OObj.SetFlagsForBitField(OObj.GetQFlags(),LFlags), Line)
            else:
                DataTypeSize, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.CurrQestVarInfo.VarType)
                self.ErrorHandler(ReturnCode, Line, 'OneOf varid is not the valid data type')
                if DataTypeSize != 0 and DataTypeSize != self.CurrQestVarInfo.VarTotalSize:
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, Line, 'OneOf varid doesn\'t support array')
                self.ErrorHandler(OObj.SetFlags(OObj.GetQFlags(), self.CurrQestVarInfo.VarType), Line)

        if ctx.FLAGS() != None:
            UpdateVarType = ctx.vfrOneofFlagsField().UpdateVarType
            if self.CurrQestVarInfo.IsBitVar:
                self.ErrorHandler(OObj.SetFlagsForBitField(ctx.vfrOneofFlagsField().HFlags,ctx.vfrOneofFlagsField().LFlags), ctx.F.line)
            else:
                self.ErrorHandler(OObj.SetFlags(ctx.vfrOneofFlagsField().HFlags, ctx.vfrOneofFlagsField().LFlags), ctx.F.line)

        if (self.CurrQestVarInfo.IsBitVar == False) and (self.CurrQestVarInfo.VarType not in BasicTypes):
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, Line, 'OneOf question only support UINT8, UINT16, UINT32 and UINT64 data type.')

        # modify the data Vartype for NameValue
        if UpdateVarType:
            UpdatedOObj = IfrOneOf(self.CurrQestVarInfo.VarType)
            UpdatedOObj.QName = OObj.QName
            UpdatedOObj.VarIdStr = OObj.VarIdStr
            UpdatedOObj.HasQuestionId = OObj.HasQuestionId
            UpdatedOObj.GetInfo().Question = OObj.GetInfo().Question
            UpdatedOObj.GetInfo().Flags = OObj.GetInfo().Flags
            UpdatedOObj.GetInfo().Data.MinValue = OObj.GetInfo().Data.MinValue
            UpdatedOObj.GetInfo().Data.MaxValue = OObj.GetInfo().Data.MaxValue
            UpdatedOObj.GetInfo().Data.Step = OObj.GetInfo().Data.Step
            OObj = UpdatedOObj

        OObj.SetLineNo(ctx.start.line)
        if ctx.FLAGS() != None:
            OObj.SetFlagsStream(self.ExtractOriginalText(ctx.vfrOneofFlagsField()))

        if ctx.vfrSetMinMaxStep() != None:
            OObj.SetHasMinMax(True)
            if ctx.vfrSetMinMaxStep().Step() != None:
                OObj.SetHasStep(True)

        ctx.Node.Data = OObj
        ctx.Node.Buffer = gFormPkg.StructToStream(OObj.GetInfo())
        self.InsertEndNode(ctx.Node, ctx.stop.line)

        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrOneofFlagsField.
    def visitVfrOneofFlagsField(self, ctx:SourceVfrSyntaxParser.VfrOneofFlagsFieldContext):

        ctx.LFlags = self.CurrQestVarInfo.VarType & EFI_IFR_NUMERIC_SIZE
        VarStoreType = gVfrDataStorage.GetVarStoreType(self.CurrQestVarInfo.VarStoreId)
        Line = ctx.start.line
        IsSetType = False
        self.visitChildren(ctx)

        for FlagsFieldCtx in ctx.numericFlagsField():
            ctx.HFlags |= FlagsFieldCtx.HFlag
            IsSetType |= FlagsFieldCtx.IsSetType
            if FlagsFieldCtx.NumericSizeOne() != None:
                if self.CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_1

            if FlagsFieldCtx.NumericSizeTwo() != None:
                if self.CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_2

            if FlagsFieldCtx.NumericSizeFour() != None:
                if self.CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_4

            if FlagsFieldCtx.NumericSizeEight() != None:
                if self.CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_8

            if FlagsFieldCtx.DisPlayIntDec() != None:
                if self.CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_INT_DEC
                else:
                    ctx.LFlags =  (ctx.LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_INT_DEC_BIT

            if FlagsFieldCtx.DisPlayUIntDec() != None:
                if self.CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_UINT_DEC
                else:
                    ctx.LFlags =  (ctx.LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_UINT_DEC_BIT

            if FlagsFieldCtx.DisPlayUIntHex() != None:
                if self.CurrQestVarInfo.IsBitVar == False:
                    ctx.LFlags =  (ctx.LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_UINT_HEX
                else:
                    ctx.LFlags =  (ctx.LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_UINT_HEX_BIT

        VarType = self.CurrQestVarInfo.VarType
        if self.CurrQestVarInfo.IsBitVar == False:
            if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
                if VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER or VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_EFI:
                    if self.CurrQestVarInfo.VarType != (ctx.LFlags & EFI_IFR_NUMERIC_SIZE):
                        self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, Line, 'Numeric Flag is not same to Numeric VarData type')
                else:
                    # update data type for name/value store
                    self.CurrQestVarInfo.VarType = ctx.LFlags & EFI_IFR_NUMERIC_SIZE
                    Size, _ = gVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.CurrQestVarInfo.VarType)
                    self.CurrQestVarInfo.VarTotalSize = Size
            elif IsSetType:
                self.CurrQestVarInfo.VarType = ctx.LFlags & EFI_IFR_NUMERIC_SIZE

        elif self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            ctx.LFlags &= EDKII_IFR_DISPLAY_BIT
            ctx.LFlags |= EDKII_IFR_NUMERIC_SIZE_BIT & self.CurrQestVarInfo.VarTotalSize

        if VarType != self.CurrQestVarInfo.VarType:
            ctx.UpdateVarType = True

        return ctx.HFlags, ctx.LFlags


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementStringType.
    def visitVfrStatementStringType(self, ctx:SourceVfrSyntaxParser.VfrStatementStringTypeContext):

        self.visitChildren(ctx)
        if ctx.vfrStatementPassword() != None:
            ctx.Node = ctx.vfrStatementPassword().Node
        elif ctx.vfrStatementString() != None:
            ctx.Node = ctx.vfrStatementString().Node
        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementString.
    def visitVfrStatementString(self, ctx:SourceVfrSyntaxParser.VfrStatementStringContext):

        self.IsStringOp = True
        SObj = IfrString()
        ctx.Node.Data = SObj
        SObj.SetLineNo(ctx.start.line)
        self.CurrentQuestion = SObj

        self.visitChildren(ctx)

        if ctx.FLAGS() != None:
            HFlags = ctx.vfrStringFlagsField().HFlags
            LFlags = ctx.vfrStringFlagsField().LFlags
            self.ErrorHandler(SObj.SetFlags(HFlags, LFlags), ctx.F.line)
            SObj.SetFlagsStream(self.ExtractOriginalText(ctx.vfrStringFlagsField()))

        if ctx.Key() != None:
            if ctx.N != None:
                Key = self.TransNum(ctx.N.text)
            else:
                Key = self.PreProcessDB.Read(ctx.S.text)
                ctx.Node.Dict['key'] = KV(ctx.S.text, Key)
            self.AssignQuestionKey(SObj, Key)
            SObj.SetHasKey(True)

        if ctx.N1 != None:
            StringMinSize = self.TransNum(ctx.N1.text)
        else:
            StringMinSize = self.PreProcessDB.Read(ctx.S1.text)
            ctx.Node.Dict['minsize'] = KV(ctx.S1.text, StringMinSize)

        if ctx.N2 != None:
            StringMaxSize = self.TransNum(ctx.N2.text)
        else:
            StringMaxSize = self.PreProcessDB.Read(ctx.S2.text)
            ctx.Node.Dict['maxsize'] = KV(ctx.S2.text, StringMaxSize)

        VarArraySize = self.GetCurArraySize()
        if StringMinSize > 0xFF:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.Min.line, "String MinSize takes only one byte, which can't be larger than 0xFF.")
        if VarArraySize != 0 and StringMinSize > VarArraySize:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.Min.line, "String MinSize can't be larger than the max number of elements in string array.")
        SObj.SetMinSize(StringMinSize)

        if StringMaxSize > 0xFF:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.Max.line, "String MaxSize takes only one byte, which can't be larger than 0xFF.")
        elif VarArraySize != 0 and StringMaxSize > VarArraySize:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.Max.line, "String MaxSize can't be larger than the max number of elements in string array.")
        elif StringMaxSize < StringMinSize:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.Max.line, "String MaxSize can't be less than String MinSize.")
        SObj.SetMaxSize(StringMaxSize)

        ctx.Node.Buffer = gFormPkg.StructToStream(SObj.GetInfo())
        self.InsertEndNode(ctx.Node, ctx.stop.line)

        self.IsStringOp = False

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStringFlagsField.
    def visitVfrStringFlagsField(self, ctx:SourceVfrSyntaxParser.VfrStringFlagsFieldContext):

        self.visitChildren(ctx)
        for FlagsFieldCtx in ctx.stringFlagsField():
            ctx.HFlags |= FlagsFieldCtx.HFlag
            ctx.LFlags |= FlagsFieldCtx.LFlag

        return ctx.HFlags, ctx.LFlags


    # Visit a parse tree produced by SourceVfrSyntaxParser#stringFlagsField.
    def visitStringFlagsField(self, ctx:SourceVfrSyntaxParser.StringFlagsFieldContext):

        self.visitChildren(ctx)

        if ctx.Number() != None:
            if self.TransNum(ctx.Number()) != 0:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, ctx.start.line)
        elif ctx.questionheaderFlagsField() != None:
            ctx.HFlag = ctx.questionheaderFlagsField().QHFlag
        elif ctx.M != None:
            ctx.LFlag = 0x01
        elif ctx.S != None:
            if self.PreProcessDB.Read(ctx.S.text) != 0:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, ctx.start.line)

        return  ctx.HFlag, ctx.LFlag


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementPassword.
    def visitVfrStatementPassword(self, ctx:SourceVfrSyntaxParser.VfrStatementPasswordContext):

        PObj = IfrPassword()
        ctx.Node.Data = PObj
        PObj.SetLineNo(ctx.start.line)
        self.CurrentQuestion = PObj

        self.visitChildren(ctx)

        if ctx.Key() != None:
            if ctx.N != None:
                Key = self.TransNum(ctx.N.text)
            else:
                Key = self.PreProcessDB.Read(ctx.S.text)
                ctx.Node.Dict['key'] = KV(ctx.S.text, Key)
            self.AssignQuestionKey(PObj, Key)
            PObj.SetHasKey(True)

        if ctx.N1 != None:
            PassWordMinSize = self.TransNum(ctx.N1.text)
        else:
            PassWordMinSize = self.PreProcessDB.Read(ctx.S1.text)
            ctx.Node.Dict['minsize'] = KV(ctx.S1.text, PassWordMinSize)

        if ctx.N2 != None:
            PasswordMaxSize = self.TransNum(ctx.N2.text)
        else:
            PasswordMaxSize = self.PreProcessDB.Read(ctx.S2.text)
            ctx.Node.Dict['maxsize'] = KV(ctx.S2.text, PasswordMaxSize)

        if ctx.FLAGS() != None:
            HFlags = ctx.vfrPasswordFlagsField().HFlags
            self.ErrorHandler(PObj.SetFlags(HFlags), ctx.F.line)
            PObj.SetFlagsStream(self.ExtractOriginalText(ctx.vfrPasswordFlagsField()))

        VarArraySize = self.GetCurArraySize()
        if PassWordMinSize > 0xFF:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.Min.line, "String MinSize takes only one byte, which can't be larger than 0xFF.")
        if VarArraySize != 0 and PassWordMinSize > VarArraySize:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.Min.line, "String MinSize can't be larger than the max number of elements in string array.")
        PObj.SetMinSize(PassWordMinSize)

        if PasswordMaxSize > 0xFF:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.Max.line, "String MaxSize takes only one byte, which can't be larger than 0xFF.")
        elif VarArraySize != 0 and PasswordMaxSize > VarArraySize:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.Max.line, "String MaxSize can't be larger than the max number of elements in string array.")
        elif PasswordMaxSize < PassWordMinSize:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.Max.line, "String MaxSize can't be less than String MinSize.")
        PObj.SetMaxSize(PasswordMaxSize)

        ctx.Node.Buffer = gFormPkg.StructToStream(PObj.GetInfo())
        self.InsertEndNode(ctx.Node, ctx.stop.line)

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrPasswordFlagsField.
    def visitVfrPasswordFlagsField(self, ctx:SourceVfrSyntaxParser.VfrPasswordFlagsFieldContext):

        self.visitChildren(ctx)
        for FlagsFieldCtx in ctx.passwordFlagsField():
            ctx.HFlags |= FlagsFieldCtx.HFlag

        return ctx.HFlags


    # Visit a parse tree produced by SourceVfrSyntaxParser#passwordFlagsField.
    def visitPasswordFlagsField(self, ctx:SourceVfrSyntaxParser.PasswordFlagsFieldContext):

        self.visitChildren(ctx)

        if ctx.Number() != None:
            if self.TransNum(ctx.Number()) != 0:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, ctx.start.line)
        elif ctx.questionheaderFlagsField() != None:
            ctx.HFlag = ctx.questionheaderFlagsField().QHFlag
        elif ctx.S != None:
            if self.PreProcessDB.Read(ctx.S.text) != 0:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, ctx.start.line)
        return ctx.HFlag


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementOrderedList.
    def visitVfrStatementOrderedList(self, ctx:SourceVfrSyntaxParser.VfrStatementOrderedListContext):

        OLObj = IfrOrderedList()
        ctx.Node.Data = OLObj
        OLObj.SetLineNo(ctx.start.line)
        self.CurrentQuestion = OLObj
        self.IsOrderedList = True

        self.visitChildren(ctx)

        VarArraySize = self.GetCurArraySize()
        if VarArraySize > 0xFF:
            OLObj.SetMaxContainers(0xFF)
        else:
            OLObj.SetMaxContainers(VarArraySize)

        if ctx.MaxContainers() != None:
            if ctx.N != None:
                MaxContainers = self.TransNum(ctx.N.text)
            else:
                MaxContainers = self.PreProcessDB.Read(ctx.S.text)
                ctx.Node.Dict['maxcontainers'] = KV(ctx.S.text, MaxContainers)
            if MaxContainers > 0xFF:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.M.line, "OrderedList MaxContainers takes only one byte, which can't be larger than 0xFF.")
            elif VarArraySize != 0 and MaxContainers > VarArraySize:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.M.line, "OrderedList MaxContainers can't be larger than the max number of elements in array.")
            OLObj.SetMaxContainers(MaxContainers)
            OLObj.SetHasMaxContainers(True)

        if ctx.FLAGS() != None:

            HFlags = ctx.vfrOrderedListFlags().HFlags
            LFlags = ctx.vfrOrderedListFlags().LFlags
            self.ErrorHandler(OLObj.SetFlags(HFlags, LFlags), ctx.F.line)
            OLObj.SetFlagsStream(self.ExtractOriginalText(ctx.vfrOrderedListFlags()))

        ctx.Node.Buffer = gFormPkg.StructToStream(OLObj.GetInfo())
        self.InsertEndNode(ctx.Node, ctx.stop.line)
        self.IsOrderedList = False

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrOrderedListFlags.
    def visitVfrOrderedListFlags(self, ctx:SourceVfrSyntaxParser.VfrOrderedListFlagsContext):

        self.visitChildren(ctx)

        for FlagsFieldCtx in ctx.orderedlistFlagsField():
            ctx.HFlags |= FlagsFieldCtx.HFlag
            ctx.LFlags |= FlagsFieldCtx.LFlag

        return ctx.HFlags, ctx.LFlags


    # Visit a parse tree produced by SourceVfrSyntaxParser#orderedlistFlagsField.
    def visitOrderedlistFlagsField(self, ctx:SourceVfrSyntaxParser.OrderedlistFlagsFieldContext):

        self.visitChildren(ctx)

        if ctx.Number() != None:
            if self.TransNum(ctx.Number()) != 0:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, ctx.start.line)
        elif ctx.questionheaderFlagsField() != None:
            ctx.HFlag = ctx.questionheaderFlagsField().QHFlag
        elif ctx.UniQueFlag() != None:
            ctx.LFlag = 0x01
        elif ctx.NoEmptyFlag() != None:
            ctx.LFlag = 0x02
        elif ctx.S != None:
            if self.PreProcessDB.Read(ctx.S.text) != 0:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, ctx.start.line)


        return  ctx.HFlag, ctx.LFlag

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementDate.
    def visitVfrStatementDate(self, ctx:SourceVfrSyntaxParser.VfrStatementDateContext):

        DObj = IfrDate()
        ctx.Node.Data = DObj
        Line = ctx.start.line
        DObj.SetLineNo(Line)
        self.Value = ctx.Val
        self.visitChildren(ctx)

        if ctx.vfrQuestionHeader() != None:

            if self.CurrQestVarInfo.VarType == EFI_IFR_TYPE_OTHER:
                self.CurrQestVarInfo.VarType == EFI_IFR_TYPE_DATE

            if ctx.FLAGS() != None:
                DObj.SetFlagsStream(self.ExtractOriginalText(ctx.vfrDateFlags()))
                self.ErrorHandler(DObj.SetFlags(EFI_IFR_QUESTION_FLAG_DEFAULT, ctx.vfrDateFlags().LFlags), ctx.F1.line)

        else:

            Year = self.TransId(ctx.S1.text)
            Year += '.'
            Year += self.TransId(ctx.S2.text)

            Month = self.TransId(ctx.S5.text)
            Month += '.'
            Month += self.TransId(ctx.S6.text)

            Day = self.TransId(ctx.S9.text)
            Day += '.'
            Day += self.TransId(ctx.S10.text)

            Prompt = self.PreProcessDB.Read(ctx.S3.text)
            ctx.Node.Dict['prompt'] = KV(ctx.S3.text, Prompt)
            Help = self.PreProcessDB.Read(ctx.S4.text)
            ctx.Node.Dict['help'] = KV(ctx.S4.text, Help)

            if ctx.FLAGS() != None:
                DObj.SetFlagsStream(self.ExtractOriginalText(ctx.vfrDateFlags()))
                self.ErrorHandler(DObj.SetFlags(EFI_IFR_QUESTION_FLAG_DEFAULT, ctx.vfrDateFlags().LFlags), ctx.F2.line)

            QId, _ = self.VfrQuestionDB.RegisterOldDateQuestion(Year, Month, Day, EFI_QUESTION_ID_INVALID, gFormPkg)
            DObj.SetQuestionId(QId)
            DObj.SetFlags(EFI_IFR_QUESTION_FLAG_DEFAULT, QF_DATE_STORAGE_TIME)
            DObj.SetPrompt(Prompt)
            DObj.SetHelp(Help)

            DefaultObj = IfrDefault(EFI_IFR_TYPE_DATE, [ctx.Val], EFI_HII_DEFAULT_CLASS_STANDARD, ctx.Val, EFI_IFR_TYPE_DATE)
            DefaultObj.SetLineNo(Line)
            DefaultNode = VfrTreeNode(EFI_IFR_DEFAULT_OP, DefaultObj, gFormPkg.StructToStream(DefaultObj.GetInfo()))
            ctx.Node.insertChild(DefaultNode)

        ctx.Node.Buffer = gFormPkg.StructToStream(DObj.GetInfo())
        for Ctx in ctx.vfrStatementInconsistentIf():
            self.InsertChild(ctx.Node, Ctx)
        self.InsertEndNode(ctx.Node, ctx.stop.line)

        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#minMaxDateStepDefault.
    def visitMinMaxDateStepDefault(self, ctx:SourceVfrSyntaxParser.MinMaxDateStepDefaultContext):

        if ctx.Default() != None:
            Minimum = self.TransNum(ctx.Number(0))
            Maximum = self.TransNum(ctx.Number(1))
            if ctx.KeyValue == 0:
                ctx.Date.Year = self.TransNum(ctx.N.text)
                if ctx.Date.Year < Minimum or ctx.Date.Year > Maximum:
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.N.line, "Year default value must be between Min year and Max year.")
            if ctx.KeyValue == 1:
                ctx.Date.Month = self.TransNum(ctx.N.text)
                if ctx.Date.Month < 1 or ctx.Date.Month > 12:
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.D.line, "Month default value must be between Min 1 and Max 12.")
            if ctx.KeyValue == 2:
                ctx.Date.Day = self.TransNum(ctx.N.text)
                if ctx.Date.Day < 1 or ctx.Date.Day > 31:
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.D.line, "Day default value must be between Min 1 and Max 31.")
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrDateFlags.
    def visitVfrDateFlags(self, ctx:SourceVfrSyntaxParser.VfrDateFlagsContext):

        self.visitChildren(ctx)

        for FlagsFieldCtx in ctx.dateFlagsField():
            ctx.LFlags |= FlagsFieldCtx.LFlag

        return ctx.LFlags


    # Visit a parse tree produced by SourceVfrSyntaxParser#dateFlagsField.
    def visitDateFlagsField(self, ctx:SourceVfrSyntaxParser.DateFlagsFieldContext):

        self.visitChildren(ctx)

        if ctx.Number() != None:
            ctx.LFlag = self.TransNum(ctx.Number())
        elif ctx.YearSupppressFlag() != None:
            ctx.LFlag = 0x01
        elif ctx.MonthSuppressFlag() != None:
            ctx.LFlag = 0x02
        elif ctx.DaySuppressFlag() != None:
            ctx.LFlag = 0x04
        elif ctx.StorageNormalFlag() != None:
            ctx.LFlag = 0x00
        elif ctx.StorageTimeFlag() != None:
            ctx.LFlag = 0x010
        elif ctx.StorageWakeUpFlag() != None:
            ctx.LFlag = 0x20
        elif ctx.S != None:
            ctx.LFlag = self.PreProcessDB.Read(ctx.S.text)

        return ctx.LFlag


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementTime.
    def visitVfrStatementTime(self, ctx:SourceVfrSyntaxParser.VfrStatementTimeContext):

        TObj = IfrTime()
        ctx.Node.Data = TObj
        Line = ctx.start.line
        TObj.SetLineNo(Line)
        self.Value = ctx.Val
        self.visitChildren(ctx)

        if ctx.vfrQuestionHeader() != None:

            if self.CurrQestVarInfo.VarType == EFI_IFR_TYPE_OTHER:
                self.CurrQestVarInfo.VarType == EFI_IFR_TYPE_TIME

            if ctx.FLAGS() != None:
                TObj.SetFlagsStream(self.ExtractOriginalText(ctx.vfrTimeFlags()))
                self.ErrorHandler(TObj.SetFlags(EFI_IFR_QUESTION_FLAG_DEFAULT, ctx.vfrTimeFlags().LFlags), ctx.F1.line)
        else:
            Hour = self.TransId(ctx.S1.text)
            Hour += '.'
            Hour += self.TransId(ctx.S2.text)

            Minute = self.TransId(ctx.S5.text)
            Minute += '.'
            Minute += self.TransId(ctx.S6.text)

            Second = self.TransId(ctx.S9.text)
            Second += '.'
            Second += self.TransId(ctx.S10.text)

            Prompt = self.PreProcessDB.Read(ctx.S3.text)
            ctx.Node.Dict['prompt'] = KV(ctx.S3.text, Prompt)
            Help = self.PreProcessDB.Read(ctx.S4.text)
            ctx.Node.Dict['help'] = KV(ctx.S4.text, Help)

            if ctx.FLAGS() != None:
                TObj.SetFlagsStream(self.ExtractOriginalText(ctx.vfrTimeFlags()))
                self.ErrorHandler(TObj.SetFlags(EFI_IFR_QUESTION_FLAG_DEFAULT, ctx.vfrTimeFlags().LFlags), ctx.F2.line)

            QId, _ = self.VfrQuestionDB.RegisterOldTimeQuestion(Hour, Minute, Second, EFI_QUESTION_ID_INVALID, gFormPkg)
            TObj.SetQuestionId(QId)
            TObj.SetFlags(EFI_IFR_QUESTION_FLAG_DEFAULT, QF_TIME_STORAGE_TIME)
            TObj.SetPrompt(Prompt)
            TObj.SetHelp(Help)

            DefaultObj = IfrDefault(EFI_IFR_TYPE_TIME, [ctx.Val], EFI_HII_DEFAULT_CLASS_STANDARD, EFI_IFR_TYPE_TIME)
            DefaultObj.SetLineNo(Line)
            DefaultNode = VfrTreeNode(EFI_IFR_DEFAULT_OP, DefaultObj, gFormPkg.StructToStream(DefaultObj.GetInfo()))
            ctx.Node.insertChild(DefaultNode)

        ctx.Node.Buffer = gFormPkg.StructToStream(TObj.GetInfo())
        for Ctx in ctx.vfrStatementInconsistentIf():
            self.InsertChild(ctx.Node, Ctx)
        self.InsertEndNode(ctx.Node, ctx.stop.line)
        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#minMaxTimeStepDefault.
    def visitMinMaxTimeStepDefault(self, ctx:SourceVfrSyntaxParser.MinMaxTimeStepDefaultContext):

        if ctx.Default() != None:
            Minimum = self.TransNum(ctx.Number(0))
            Maximum = self.TransNum(ctx.Number(1))
            if ctx.KeyValue == 0:
                ctx.Time.Hour = self.TransNum(ctx.Number(len(ctx.Number())-1))
                if ctx.Time.Hour > 23:
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.N.line, "Hour default value must be between 0 and 23.")
            if ctx.KeyValue == 1:
                ctx.Time.Minute = self.TransNum(ctx.Number(len(ctx.Number())-1))
                if ctx.Time.Minute > 59:
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.N.line, "Minute default value must be between 0 and 59.")
            if ctx.KeyValue == 2:
                ctx.Time.Second = self.TransNum(ctx.Number(len(ctx.Number())-1))
                if ctx.Time.Second > 59:
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.N.line, "Second default value must be between 0 and 59.")
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrTimeFlags.
    def visitVfrTimeFlags(self, ctx:SourceVfrSyntaxParser.VfrTimeFlagsContext):

        self.visitChildren(ctx)

        for FlagsFieldCtx in ctx.timeFlagsField():
            ctx.LFlags |= FlagsFieldCtx.LFlag

        return ctx.LFlags

    # Visit a parse tree produced by SourceVfrSyntaxParser#timeFlagsField.
    def visitTimeFlagsField(self, ctx:SourceVfrSyntaxParser.TimeFlagsFieldContext):

        self.visitChildren(ctx)

        if ctx.Number() != None:
            ctx.LFlag = self.TransNum(ctx.Number())
        elif ctx.HourSupppressFlag() != None:
            ctx.LFlag = 0x01
        elif ctx.MinuteSuppressFlag() != None:
            ctx.LFlag = 0x02
        elif ctx.SecondSuppressFlag() != None:
            ctx.LFlag = 0x04
        elif ctx.StorageNormalFlag() != None:
            ctx.LFlag = 0x00
        elif ctx.StorageTimeFlag() != None:
            ctx.LFlag = 0x10
        elif ctx.StorageWakeUpFlag() != None:
            ctx.LFlag = 0x20
        elif ctx.S != None:
            ctx.LFlag = self.PreProcessDB.Read(ctx.S.text)

        return ctx.LFlag

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementConditional.
    def visitVfrStatementConditional(self, ctx:SourceVfrSyntaxParser.VfrStatementConditionalContext):

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


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementConditionalNew.
    def visitVfrStatementConditionalNew(self, ctx:SourceVfrSyntaxParser.VfrStatementConditionalNewContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementSuppressIfStat.
    def visitVfrStatementSuppressIfStat(self, ctx:SourceVfrSyntaxParser.VfrStatementSuppressIfStatContext):

        self.visitChildren(ctx)
        ctx.Node = ctx.vfrStatementSuppressIfStatNew().Node
        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementGrayOutIfStat.
    def visitVfrStatementGrayOutIfStat(self, ctx:SourceVfrSyntaxParser.VfrStatementGrayOutIfStatContext):

        self.visitChildren(ctx)
        ctx.Node = ctx.vfrStatementGrayOutIfStatNew().Node
        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementStatList.
    def visitVfrStatementStatList(self, ctx:SourceVfrSyntaxParser.VfrStatementStatListContext):

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


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementStatListOld.
    def visitVfrStatementStatListOld(self, ctx:SourceVfrSyntaxParser.VfrStatementStatListOldContext):
        return self.visitChildren(ctx)

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementDisableIfStat.
    def visitVfrStatementDisableIfStat(self, ctx:SourceVfrSyntaxParser.VfrStatementDisableIfStatContext):

        DIObj = IfrDisableIf()
        DIObj.SetLineNo(ctx.start.line)
        ctx.Node.Data = DIObj
        ctx.Node.Buffer = gFormPkg.StructToStream(DIObj.GetInfo())
        ctx.Node.Condition = 'disableif' + ' ' + self.ExtractOriginalText(ctx.vfrStatementExpression())
        self.visitChildren(ctx)
        for Ctx in ctx.vfrStatementStatList():
            self.InsertChild(ctx.Node, Ctx)
        self.InsertEndNode(ctx.Node, ctx.stop.line)

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementSuppressIfStatNew.
    def visitVfrStatementSuppressIfStatNew(self, ctx:SourceVfrSyntaxParser.VfrStatementSuppressIfStatNewContext):

        SIObj = IfrSuppressIf()
        SIObj.SetLineNo(ctx.start.line)
        ctx.Node.Data = SIObj
        ctx.Node.Buffer = gFormPkg.StructToStream(SIObj.GetInfo())
        ctx.Node.Condition = 'suppressif' + ' ' + self.ExtractOriginalText(ctx.vfrStatementExpression())
        self.visitChildren(ctx)
        for Ctx in ctx.vfrStatementStatList():
            self.InsertChild(ctx.Node, Ctx)
        self.InsertEndNode(ctx.Node, ctx.stop.line)

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementGrayOutIfStatNew.
    def visitVfrStatementGrayOutIfStatNew(self, ctx:SourceVfrSyntaxParser.VfrStatementGrayOutIfStatNewContext):

        GOIObj = IfrGrayOutIf()
        GOIObj.SetLineNo(ctx.start.line)
        ctx.Node.Data = GOIObj
        ctx.Node.Buffer = gFormPkg.StructToStream(GOIObj.GetInfo())
        ctx.Node.Condition = 'grayoutif' + ' ' + self.ExtractOriginalText(ctx.vfrStatementExpression())
        self.visitChildren(ctx)
        for Ctx in ctx.vfrStatementStatList():
            self.InsertChild(ctx.Node, Ctx)
        self.InsertEndNode(ctx.Node, ctx.stop.line)

        return ctx.Node



    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementInconsistentIfStat.
    def visitVfrStatementInconsistentIfStat(self, ctx:SourceVfrSyntaxParser.VfrStatementInconsistentIfStatContext):

        IIObj = IfrInconsistentIf()
        self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, ctx.start.line)
        IIObj.SetLineNo(ctx.start.line)
        self.visitChildren(ctx)
        Prompt = self.PreProcessDB.Read(ctx.S.text)
        ctx.Node.Dict['promot'] = KV(ctx.S.text, Prompt)
        IIObj.SetError(Prompt)
        ctx.Node.Data = IIObj
        ctx.Node.Buffer = gFormPkg.StructToStream(IIObj.GetInfo())
        ctx.Node.Condition = 'inconsistentif' + ' ' + self.ExtractOriginalText(ctx.vfrStatementExpression())
        self.InsertEndNode(ctx.Node, ctx.stop.line)

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementInvalid.
    def visitVfrStatementInvalid(self, ctx:SourceVfrSyntaxParser.VfrStatementInvalidContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementInvalidHidden.
    def visitVfrStatementInvalidHidden(self, ctx:SourceVfrSyntaxParser.VfrStatementInvalidHiddenContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementInvalidInventory.
    def visitVfrStatementInvalidInventory(self, ctx:SourceVfrSyntaxParser.VfrStatementInvalidInventoryContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementInvalidSaveRestoreDefaults.
    def visitVfrStatementInvalidSaveRestoreDefaults(self, ctx:SourceVfrSyntaxParser.VfrStatementInvalidSaveRestoreDefaultsContext):
        return self.visitChildren(ctx)


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementLabel.
    def visitVfrStatementLabel(self, ctx:SourceVfrSyntaxParser.VfrStatementLabelContext):

        LObj = IfrLabel()
        self.visitChildren(ctx)
        LObj.SetLineNo(ctx.start.line)
        if ctx.N != None:
            Label = self.TransNum(ctx.N.text)
        else:
            Label = self.PreProcessDB.Read(ctx.S.text)
            ctx.Node.Dict['label'] = KV(ctx.S.text, Label)
        LObj.SetNumber(Label)
        ctx.Node.Data = LObj
        ctx.Node.Buffer = gFormPkg.StructToStream(LObj.GetInfo())
        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatemex.BObjntBanner.
    def visitVfrStatementBanner(self, ctx:SourceVfrSyntaxParser.VfrStatementBannerContext):

        self.visitChildren(ctx)
        BObj = IfrBanner()
        BObj.SetLineNo(ctx.start.line)
        Title = self.PreProcessDB.Read(ctx.S.text)
        ctx.Node.Dict['title'] = KV(ctx.S.text, Title)
        BObj.SetTitle(Title)

        if ctx.Line() != None:
            if ctx.NL != None:
                Line = self.TransNum(ctx.NL.text)
            else:
                Line = self.PreProcessDB.Read(ctx.SL.text)
                ctx.Node.Dict['line'] = KV(ctx.SL.text, Line)
            BObj.SetLine(Line)
            if ctx.Left() != None:
                ctx.Node.Dict['align'] = KV('left', 0)
                BObj.SetAlign(0)
            if ctx.Center() != None:
                ctx.Node.Dict['align'] = KV('center', 1)
                BObj.SetAlign(1)
            if ctx.Right() != None:
                ctx.Node.Dict['align'] = KV('right', 2)
                BObj.SetAlign(2)

        ctx.Node.Data = BObj
        ctx.Node.Buffer = gFormPkg.StructToStream(BObj.GetInfo())

        if ctx.Timeout() != None:
            TObj = IfrTimeout()
            if ctx.TN != None:
                Timeout = self.TransNum(ctx.TN.text)
            else:
                Timeout = self.PreProcessDB.Read(ctx.TS.text)
                ctx.Node.Dict['timeout'] = KV(ctx.TS.text, Timeout)
            TObj.SetLineNo(ctx.start.line)
            TObj.SetTimeout(Timeout)
            Node = VfrTreeNode(EFI_IFR_GUID_OP, TObj, gFormPkg.StructToStream(TObj.GetInfo()))
            ctx.Node.insertChild(Node)
            BObj.SetHasTimeOut(True)

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementExtension.
    def visitVfrStatementExtension(self, ctx:SourceVfrSyntaxParser.VfrStatementExtensionContext):

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
                ctx.TypeName = self.TransId(ctx.D.text)
                ctx.IsStruct = True
            ctx.ArrayNum = self.TransNum(ctx.Number())
            ctx.TypeSize, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByTypeName(ctx.TypeName)
            self.ErrorHandler(ReturnCode, ctx.D.line)
            ctx.Size = ctx.TypeSize * ctx.ArrayNum if ctx.ArrayNum > 0 else ctx.TypeSize
        ctx.Buffer = Refine_EFI_IFR_BUFFER(ctx.Size)

        self.visitChildren(ctx)
        Line = ctx.start.line
        GuidObj = IfrExtensionGuid(ctx.Size, ctx.TypeName, ctx.ArrayNum)
        for Ctx in ctx.vfrExtensionData():
            GuidObj.SetFieldList(Ctx.FName, Ctx.TFValue)

        GuidObj.SetLineNo(Line)
        Guid = self.PreProcessDB.Read(ctx.S.text)
        ctx.Node.Dict['guid'] = KV(ctx.S.text, Guid)
        GuidObj.SetGuid(Guid)
        if ctx.TypeName != '':
            GuidObj.SetData(ctx.Buffer)
        GuidObj.SetScope(1)
        ctx.Node.Data = GuidObj
        ctx.Node.Buffer = gFormPkg.StructToStream(GuidObj.GetInfo())
        ctx.Node.Buffer += gFormPkg.StructToStream(ctx.Buffer)

        for Ctx in ctx.vfrStatementExtension():
            self.InsertChild(ctx.Node, Ctx)

        self.InsertEndNode(ctx.Node, ctx.stop.line)
        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrExtensionData.
    def visitVfrExtensionData(self, ctx:SourceVfrSyntaxParser.VfrExtensionDataContext):

        IsArray = False if ctx.I == None else True
        IsStruct = ctx.parentCtx.IsStruct
        Buffer = ctx.parentCtx.Buffer

        self.visitChildren(ctx)
        ctx.TFValue = self.TransNum(ctx.N.text)
        Data = self.TransNum(ctx.N.text)
        if IsArray:
            ctx.FName += 'data' +'[' + ctx.I.text + ']'
        else:
            ctx.FName += 'data'
        if IsStruct == False:
           # Buffer.Data = Data
            pass
        else:
            ctx.TFName += ctx.parentCtx.TypeName
            for i in range(0, len(ctx.arrayName())):
                ctx.TFName += '.'
                ctx.TFName += ctx.arrayName(i).SubStrZ
                ctx.FName += '.'
                ctx.FName += ctx.arrayName(i).SubStrZ

            FieldOffset, FieldType, FieldSize, BitField, _ = gVfrVarDataTypeDB.GetDataFieldInfo(ctx.TFName)
        '''
            if not BitField:
                if IsArray:
                    ArrayIndex = ctx.I.text
                    FieldOffset += ArrayIndex* ctx.parentCtx.TypeSize

                if FieldType == EFI_IFR_TYPE_NUM_SIZE_8 or FieldType == EFI_IFR_TYPE_BOOLEAN:
                    Buffer.Data[FieldOffset] = Data

                if FieldType == EFI_IFR_TYPE_NUM_SIZE_16 or FieldType == EFI_IFR_TYPE_STRING:
                        Bytes = Data.to_bytes(2, byteorder="little", signed=True)
                        for i in range(0, 2):
                            Buffer.Data[FieldOffset + i] = Bytes[i]

                if FieldType == EFI_IFR_TYPE_NUM_SIZE_32:
                        Bytes = Data.to_bytes(4, byteorder="little", signed=True)
                        for i in range(0, 4):
                            Buffer.Data[FieldOffset + i] = Bytes[i]

                if FieldType == EFI_IFR_TYPE_NUM_SIZE_64:
                        Bytes = Data.to_bytes(8, byteorder="little", signed=True)
                        for i in range(0, 8):
                            Buffer.Data[FieldOffset + i] = Bytes[i]
            else:
                Mask = 1 << FieldSize - 1
                Offset = int(FieldOffset / 8)
                PreBits = FieldOffset % 8
                Mask <= PreBits
                if FieldType == EFI_IFR_TYPE_NUM_SIZE_32:
                    Bytes = Data.to_bytes(2, byteorder="little", signed=True)
                    Data <<= PreBits
                    Data = (Data & ~Mask) | Data
                    for i in range(0, 2):
                        Buffer.Data[Offset + i] = Bytes[i]



                print('Offset')
                print(FieldOffset)
                print('Size')
                print(FieldSize)

                Begin = int(FieldOffset / 8)
                print('Begin')
                print(Begin)

                End = int((FieldOffset + FieldSize) / 8)
                print('End')
                print(End)
                PreBits = FieldOffset % 8
                Data <<= (End - Begin + 1) * 8 - PreBits - FieldSize

                if IsArray:
                    ArrayIndex = ctx.I.Text
                    for i in range(End, Begin - 1, -1):
                        Buffer.Data[ArrayIndex* ctx.parentCtx.TypeSize + i] |= (Data & 0xff)
                        Data >= 8
                        print('{:016b}'.format(Data))

                else:
                    for i in range(Begin, End + 1):
                        print("----------------------")
                        print("i: " + str(i))
                        print('{:08b}'.format(Buffer.Data[i]))
                        Buffer.Data[i + End - Begin] |= (Data & 0xff)
                        Data = Data >> 8
                        print('{:08b}'.format(Buffer.Data[i]))
                        print("----------------------")


                4          5       6        7
                00100000  00000010 00000100 00000000


                00000000  00000000 00000000 00000000

                00 00000001
                0000 0000
                37 % 8 = 5
                0000 0001
                左移三位
                0000 1000 41
                第四个字节和第五个字节 16 - prefix - size 左移的位数
                0000 0000 0000 0001 <- 左移七位 9 + 7 = 16

                0000 0000 0000 0000 0000 0000 0000 0000
                0000 0000 0000 0001
                41 / 8 = 5
                41%8 = 1
                41 + 10 = 51 /8 = 6
                第5个字节 和 第六个字节 16 - 1 - 10 = 5 左移的位数
                0000 0000
                          1000 0000 001 00000
            '''

        return self.visitChildren(ctx)

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementModal.
    def visitVfrStatementModal(self, ctx:SourceVfrSyntaxParser.VfrStatementModalContext):

        self.visitChildren(ctx)
        ctx.Node = ctx.vfrModalTag().Node
        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrModalTag.
    def visitVfrModalTag(self, ctx:SourceVfrSyntaxParser.VfrModalTagContext):

        MObj = IfrModal()
        self.visitChildren(ctx)
        MObj.SetLineNo(ctx.start.line)
        ctx.Node.Data = MObj
        ctx.Node.Buffer = gFormPkg.StructToStream(MObj.GetInfo())

        return ctx.Node

    def SaveOpHdrCond(self, OpHdr, Cond, LineNo=0):
        if Cond == True:
            if self.IfrOpHdr[self.IfrOpHdrIndex] != None:
                return
            self.IfrOpHdr[self.IfrOpHdrIndex] = OpHdr
            self.IfrOpHdrLineNo[self.IfrOpHdrIndex] = LineNo


    def InitOpHdrCond(self):
        self.IfrOpHdr.append(None)
        self.IfrOpHdrLineNo.append(0)

    def SetSavedOpHdrScope(self):
        if  self.IfrOpHdr[self.IfrOpHdrIndex] != None:
            self.IfrOpHdr[self.IfrOpHdrIndex].Scope = 1
            return True
        return False

    def ClearSavedOPHdr(self):
        self.IfrOpHdr[self.IfrOpHdrIndex] = None

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementExpression.
    def visitVfrStatementExpression(self, ctx:SourceVfrSyntaxParser.VfrStatementExpressionContext):

        # Root expression extension function called by other function. ##
        if ctx.ExpInfo.RootLevel == 0:
            self.IfrOpHdrIndex += 1
            if self.IfrOpHdrIndex >= MAX_IFR_EXPRESSION_DEPTH:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.start.line, 'The depth of expression exceeds the max supported level 8!')
            self.InitOpHdrCond()

        self.visitChildren(ctx)

        for i in range(0, len(ctx.andTerm())):
            ctx.Nodes.extend(ctx.andTerm(i).Nodes)
            if i != 0:
                ctx.ExpInfo.ExpOpCount += 1
                OObj = IfrOr(ctx.andTerm(i).Line)
                Node = VfrTreeNode(EFI_IFR_OR_OP, OObj, gFormPkg.StructToStream(OObj.GetInfo()))
                ctx.Nodes.append(Node)

        # Extend OpCode Scope only for the root expression.
        if ctx.ExpInfo.ExpOpCount > 1 and ctx.ExpInfo.RootLevel == 0:
            if self.SetSavedOpHdrScope():
                EObj = IfrEnd()
                if self.IfrOpHdrLineNo[self.IfrOpHdrIndex] != 0:
                    EObj.SetLineNo(self.IfrOpHdrLineNo[self.IfrOpHdrIndex])
                else:
                    EObj.SetLineNo(ctx.stop.line)
                Node = VfrTreeNode(EFI_IFR_END_OP, EObj, gFormPkg.StructToStream(EObj.GetInfo()))
                ctx.Nodes.append(Node)

        if ctx.ExpInfo.RootLevel == 0:
            self.ClearSavedOPHdr()
            self.IfrOpHdrIndex = self.IfrOpHdrIndex - 1

        for Node in ctx.Nodes:
            if ctx.ParentNode != None:
                ctx.ParentNode.insertChild(Node)

        self.ConstantOnlyInExpression = False

        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrStatementExpressionSub.
    def visitVfrStatementExpressionSub(self, ctx:SourceVfrSyntaxParser.VfrStatementExpressionSubContext):

        ctx.ExpInfo.RootLevel = ctx.parentCtx.ExpInfo.RootLevel + 1
        ctx.ExpInfo.ExpOpCount = ctx.parentCtx.ExpInfo.ExpOpCount

        self.visitChildren(ctx)

        for i in range(0, len(ctx.andTerm())):
            ctx.Nodes.extend(ctx.andTerm(i).Nodes)
            if i != 0:
                ctx.ExpInfo.ExpOpCount += 1
                OObj = IfrOr(ctx.andTerm(i).Line)
                Node = VfrTreeNode(EFI_IFR_OR_OP, OObj, gFormPkg.StructToStream(OObj.GetInfo()))
                ctx.Nodes.append(Node)

        ctx.ParentNodes.extend(ctx.Nodes) ######

        ctx.parentCtx.ExpInfo.ExpOpCount = ctx.ExpInfo.ExpOpCount ##########

        return ctx.Nodes

    # Visit a parse tree produced by SourceVfrSyntaxParser#andTerm.
    def visitAndTerm(self, ctx:SourceVfrSyntaxParser.AndTermContext):

        ctx.Line = ctx.start.line
        self.visitChildren(ctx)
        for i in range(0, len(ctx.bitwiseorTerm())):
            ctx.Nodes.extend(ctx.bitwiseorTerm(i).Nodes)
            if i != 0:
                ctx.ExpInfo.ExpOpCount += 1
                AObj = IfrAnd(ctx.bitwiseorTerm(i).Line)
                Node = VfrTreeNode(EFI_IFR_AND_OP, AObj, gFormPkg.StructToStream(AObj.GetInfo()))
                ctx.Nodes.append(Node)
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#bitwiseorTerm.
    def visitBitwiseorTerm(self, ctx:SourceVfrSyntaxParser.BitwiseorTermContext):
        ctx.Line = ctx.start.line
        self.visitChildren(ctx)
        for i in range(0, len(ctx.bitwiseandTerm())):
            ctx.Nodes.extend(ctx.bitwiseandTerm(i).Nodes)
            if i != 0:
                ctx.ExpInfo.ExpOpCount += 1
                BWOObj = IfrBitWiseOr(ctx.bitwiseandTerm(i).Line)
                Node = VfrTreeNode(EFI_IFR_BITWISE_OR_OP, BWOObj, gFormPkg.StructToStream(BWOObj.GetInfo()))
                ctx.Nodes.append(Node)

        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#bitwiseandTerm.
    def visitBitwiseandTerm(self, ctx:SourceVfrSyntaxParser.BitwiseandTermContext):
        ctx.Line = ctx.start.line
        self.visitChildren(ctx)
        for i in range(0, len(ctx.equalTerm())):
            ctx.Nodes.extend(ctx.equalTerm(i).Nodes)
            if i != 0:
                ctx.ExpInfo.ExpOpCount += 1
                BWAObj = IfrBitWiseAnd(ctx.equalTerm(i).Line)
                Node = VfrTreeNode(EFI_IFR_BITWISE_AND_OP, BWAObj, gFormPkg.StructToStream(BWAObj.GetInfo()))
                ctx.Nodes.append(Node)

        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#equalTerm.
    def visitEqualTerm(self, ctx:SourceVfrSyntaxParser.EqualTermContext):
        ctx.Line = ctx.start.line
        self.visitChildren(ctx)
        ctx.Nodes.extend(ctx.compareTerm().Nodes)

        for ChildCtx in ctx.equalTermSupplementary():
            ctx.ExpInfo.ExpOpCount += 1
            ctx.Nodes.extend(ChildCtx.Nodes)

        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#equalTermEqualRule.
    def visitEqualTermEqualRule(self, ctx:SourceVfrSyntaxParser.EqualTermEqualRuleContext):
        self.visitChildren(ctx)
        ctx.Nodes.extend(ctx.compareTerm().Nodes)
        EObj = IfrEqual(ctx.start.line)
        Node = VfrTreeNode(EFI_IFR_EQUAL_OP, EObj, gFormPkg.StructToStream(EObj.GetInfo()))
        ctx.Nodes.append(Node)
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#equalTermNotEqualRule.
    def visitEqualTermNotEqualRule(self, ctx:SourceVfrSyntaxParser.EqualTermNotEqualRuleContext):
        self.visitChildren(ctx)
        ctx.Nodes.extend(ctx.compareTerm().Nodes)
        NEObj = IfrNotEqual(ctx.start.line)
        Node = VfrTreeNode(EFI_IFR_NOT_EQUAL_OP, NEObj, gFormPkg.StructToStream(NEObj.GetInfo()))
        ctx.Nodes.append(Node)
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#compareTerm.
    def visitCompareTerm(self, ctx:SourceVfrSyntaxParser.CompareTermContext):

        self.visitChildren(ctx)
        ctx.Nodes.extend(ctx.shiftTerm().Nodes)

        for ChildCtx in ctx.compareTermSupplementary():
            ctx.ExpInfo.ExpOpCount += 1
            ctx.Nodes.extend(ChildCtx.Nodes)

        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#compareTermLessRule.
    def visitCompareTermLessRule(self, ctx:SourceVfrSyntaxParser.CompareTermLessRuleContext):
        self.visitChildren(ctx)
        ctx.Nodes.extend(ctx.shiftTerm().Nodes)
        LTObj = IfrLessThan(ctx.start.line)
        Node = VfrTreeNode(EFI_IFR_LESS_THAN_OP, LTObj, gFormPkg.StructToStream(LTObj.GetInfo()))
        ctx.Nodes.append(Node)
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#compareTermLessEqualRule.
    def visitCompareTermLessEqualRule(self, ctx:SourceVfrSyntaxParser.CompareTermLessEqualRuleContext):
        self.visitChildren(ctx)
        ctx.Nodes.extend(ctx.shiftTerm().Nodes)
        LEObj = IfrLessEqual(ctx.start.line)
        Node = VfrTreeNode(EFI_IFR_LESS_EQUAL_OP, LEObj, gFormPkg.StructToStream(LEObj.GetInfo()))
        ctx.Nodes.append(Node)
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#compareTermGreaterRule.
    def visitCompareTermGreaterRule(self, ctx:SourceVfrSyntaxParser.CompareTermGreaterRuleContext):
        self.visitChildren(ctx)
        ctx.Nodes.extend(ctx.shiftTerm().Nodes)
        GTObj = IfrGreaterThan(ctx.start.line)
        Node = VfrTreeNode(EFI_IFR_GREATER_THAN_OP, GTObj, gFormPkg.StructToStream(GTObj.GetInfo()))
        ctx.Nodes.append(Node)
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#compareTermGreaterEqualRule.
    def visitCompareTermGreaterEqualRule(self, ctx:SourceVfrSyntaxParser.CompareTermGreaterEqualRuleContext):
        self.visitChildren(ctx)
        ctx.Nodes.extend(ctx.shiftTerm().Nodes)
        GEObj = IfrGreaterEqual(ctx.start.line)
        Node = VfrTreeNode(EFI_IFR_GREATER_EQUAL_OP, GEObj, gFormPkg.StructToStream(GEObj.GetInfo()))
        ctx.Nodes.append(Node)
        return ctx.Nodes

    # Visit a parse tree produced by SourceVfrSyntaxParser#shiftTerm.
    def visitShiftTerm(self, ctx:SourceVfrSyntaxParser.ShiftTermContext):
        self.visitChildren(ctx)
        ctx.Nodes.extend(ctx.addMinusTerm().Nodes)

        for ChildCtx in ctx.shiftTermSupplementary():
            ctx.ExpInfo.ExpOpCount += 1
            ctx.Nodes.extend(ChildCtx.Nodes)

        return ctx.Nodes

    # Visit a parse tree produced by SourceVfrSyntaxParser#shiftTermLeft.
    def visitShiftTermLeft(self, ctx:SourceVfrSyntaxParser.ShiftTermLeftContext):
        self.visitChildren(ctx)
        ctx.Nodes.extend(ctx.addMinusTerm().Nodes)
        SLObj = IfrShiftLeft(ctx.start.line)
        Node = VfrTreeNode(EFI_IFR_SHIFT_LEFT_OP, SLObj, gFormPkg.StructToStream(SLObj.GetInfo()))
        ctx.Nodes.append(Node)
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#shiftTermRight.
    def visitShiftTermRight(self, ctx:SourceVfrSyntaxParser.ShiftTermRightContext):
        self.visitChildren(ctx)
        ctx.Nodes.extend(ctx.addMinusTerm().Nodes)
        SRObj = IfrShiftRight(ctx.start.line)
        Node = VfrTreeNode(EFI_IFR_SHIFT_RIGHT_OP, SRObj, gFormPkg.StructToStream(SRObj.GetInfo()))
        ctx.Nodes.append(Node)
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#addMinusTerm.
    def visitAddMinusTerm(self, ctx:SourceVfrSyntaxParser.AddMinusTermContext):
        self.visitChildren(ctx)
        ctx.Nodes.extend(ctx.multdivmodTerm().Nodes)

        for ChildCtx in ctx.addMinusTermSupplementary():
            ctx.ExpInfo.ExpOpCount += 1
            ctx.Nodes.extend(ChildCtx.Nodes)

        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#addMinusTermpAdd.
    def visitAddMinusTermpAdd(self, ctx:SourceVfrSyntaxParser.AddMinusTermpAddContext):
        self.visitChildren(ctx)
        ctx.Nodes.extend(ctx.multdivmodTerm().Nodes)
        AObj = IfrAdd(ctx.start.line)
        Node = VfrTreeNode(EFI_IFR_ADD_OP, AObj, gFormPkg.StructToStream(AObj.GetInfo()))
        ctx.Nodes.append(Node)
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#addMinusTermSubtract.
    def visitAddMinusTermSubtract(self, ctx:SourceVfrSyntaxParser.AddMinusTermSubtractContext):
        self.visitChildren(ctx)
        ctx.Nodes.extend(ctx.multdivmodTerm().Nodes)
        SObj = IfrSubtract(ctx.start.line)
        Node = VfrTreeNode(EFI_IFR_SUBTRACT_OP, SObj, gFormPkg.StructToStream(SObj.GetInfo()))
        ctx.Nodes.append(Node)
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#multdivmodTerm.
    def visitMultdivmodTerm(self, ctx:SourceVfrSyntaxParser.MultdivmodTermContext):
        self.visitChildren(ctx)
        ctx.Nodes.extend(ctx.castTerm().Nodes)
        for ChildCtx in ctx.multdivmodTermSupplementary():
            ctx.ExpInfo.ExpOpCount += 1
            ctx.Nodes.extend(ChildCtx.Nodes)

        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#multdivmodTermMul.
    def visitMultdivmodTermMul(self, ctx:SourceVfrSyntaxParser.MultdivmodTermMulContext):
        self.visitChildren(ctx)
        ctx.Nodes.extend(ctx.castTerm().Nodes)
        MObj = IfrMultiply(ctx.start.line)
        Node = VfrTreeNode(EFI_IFR_MULTIPLY_OP, MObj, gFormPkg.StructToStream(MObj.GetInfo()))
        ctx.Nodes.append(Node)
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#multdivmodTermDiv.
    def visitMultdivmodTermDiv(self, ctx:SourceVfrSyntaxParser.MultdivmodTermDivContext):
        self.visitChildren(ctx)
        ctx.Nodes.extend(ctx.castTerm().Nodes)
        DObj = IfrDivide(ctx.start.line)
        Node = VfrTreeNode(EFI_IFR_DIVIDE_OP, DObj, gFormPkg.StructToStream(DObj.GetInfo()))
        ctx.Nodes.append(Node)
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#multdivmodTermRound.
    def visitMultdivmodTermModulo(self, ctx:SourceVfrSyntaxParser.MultdivmodTermModuloContext):
        self.visitChildren(ctx)
        ctx.Nodes.extend(ctx.castTerm().Nodes)
        MObj = IfrModulo(ctx.start.line)
        Node = VfrTreeNode(EFI_IFR_MODULO_OP, MObj, gFormPkg.StructToStream(MObj.GetInfo()))
        ctx.Nodes.append(Node)
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#castTerm.
    def visitCastTerm(self, ctx:SourceVfrSyntaxParser.CastTermContext):
        self.visitChildren(ctx)
        CastType = 0xFF
        for ChildCtx in ctx.castTermSub():
            CastType = ChildCtx.CastType

        ctx.Nodes.extend(ctx.atomTerm().Nodes)
        if CastType == 0:
            TBObj = IfrToBoolean(ctx.start.line)
            Node = VfrTreeNode(EFI_IFR_TO_BOOLEAN_OP, TBObj, gFormPkg.StructToStream(TBObj.GetInfo()))
            ctx.Nodes.append(Node)
            ctx.ExpInfo.ExpOpCount += 1

        elif CastType == 1:
            TUObj = IfrToUint(ctx.start.line)
            Node = VfrTreeNode(EFI_IFR_TO_UINT_OP, TUObj, gFormPkg.StructToStream(TUObj.GetInfo()))
            ctx.Nodes.append(Node)
            ctx.ExpInfo.ExpOpCount += 1
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#castTermSub.
    def visitCastTermSub(self, ctx:SourceVfrSyntaxParser.CastTermSubContext):
        self.visitChildren(ctx)
        if ctx.Boolean() != None:
            ctx.CastType = 0
        elif ctx.Uint32() != None:
            ctx.CastType = 1
        elif ctx.Uint16() != None:
            ctx.CastType = 1
        elif ctx.Uint8() != None:
            ctx.CastType = 1
        return ctx.CastType


    # Visit a parse tree produced by SourceVfrSyntaxParser#atomTerm.
    def visitAtomTerm(self, ctx:SourceVfrSyntaxParser.AtomTermContext):

        self.visitChildren(ctx)
        if ctx.vfrExpressionCatenate() != None:
            ctx.Nodes = ctx.vfrExpressionCatenate().Nodes
        if ctx.vfrExpressionMatch() != None:
            ctx.Nodes = ctx.vfrExpressionMatch().Nodes
        if ctx.vfrExpressionMatch2() != None:
            ctx.Nodes = ctx.vfrExpressionMatch2().Nodes
        if ctx.vfrExpressionParen() != None:
            ctx.Nodes = ctx.vfrExpressionParen().Nodes
        if ctx.vfrExpressionBuildInFunction() != None:
            ctx.Nodes.append(ctx.vfrExpressionBuildInFunction().Node)
        if ctx.vfrExpressionConstant() != None:
            ctx.Nodes.append(ctx.vfrExpressionConstant().Node)
        if ctx.vfrExpressionUnaryOp() != None:
            ctx.Nodes = ctx.vfrExpressionUnaryOp().Nodes
        if ctx.vfrExpressionTernaryOp() != None:
            ctx.Nodes = ctx.vfrExpressionTernaryOp().Nodes
        if ctx.vfrExpressionMap() != None:
            ctx.Nodes = ctx.vfrExpressionMap().Nodes
        if ctx.atomTerm() != None:
            ctx.Nodes = ctx.atomTerm().Nodes
            NObj = IfrNot(ctx.start.line)
            Node = VfrTreeNode(EFI_IFR_NOT_OP, NObj, gFormPkg.StructToStream(NObj.GetInfo()))
            ctx.Nodes.append(Node)
            ctx.ExpInfo.ExpOpCount += 1
        return ctx.Nodes

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrExpressionCatenate.
    def visitVfrExpressionCatenate(self, ctx:SourceVfrSyntaxParser.VfrExpressionCatenateContext):
        ctx.ExpInfo.RootLevel += 1
        self.visitChildren(ctx)

        Line = ctx.start.line
        CObj = IfrCatenate(Line)
        Node = VfrTreeNode(EFI_IFR_CATENATE_OP, CObj, gFormPkg.StructToStream(CObj.GetInfo()))
        ctx.Nodes.append(Node)
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrExpressionMatch.
    def visitVfrExpressionMatch(self, ctx:SourceVfrSyntaxParser.VfrExpressionMatchContext):
        ctx.ExpInfo.RootLevel += 1
        self.visitChildren(ctx)

        Line = ctx.start.line
        MObj = IfrMatch(Line)
        Node = VfrTreeNode(EFI_IFR_MATCH_OP, MObj, gFormPkg.StructToStream(MObj.GetInfo()))
        ctx.Nodes.append(Node)
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrExpressionMatch2.
    def visitVfrExpressionMatch2(self, ctx:SourceVfrSyntaxParser.VfrExpressionMatch2Context):
        self.visitChildren(ctx)

        Line = ctx.start.line
        Guid = self.PreProcessDB.Read(ctx.S.text)
        M2Obj = IfrMatch2(Line, Guid)
        Node = VfrTreeNode(EFI_IFR_MATCH2_OP, M2Obj, gFormPkg.StructToStream(M2Obj.GetInfo()))
        Node.Dict['guid'] = KV(ctx.S.text, Guid)
        ctx.Nodes.append(Node)
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrExpressionParen.
    def visitVfrExpressionParen(self, ctx:SourceVfrSyntaxParser.VfrExpressionParenContext):
        self.visitChildren(ctx)
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrExpressionBuildInFunction.
    def visitVfrExpressionBuildInFunction(self, ctx:SourceVfrSyntaxParser.VfrExpressionBuildInFunctionContext):
        self.visitChildren(ctx)
        if ctx.dupExp() != None:
            ctx.Node = ctx.dupExp().Node
        if ctx.vareqvalExp() != None:
            ctx.Node = ctx.vareqvalExp().Node
        if ctx.ideqvalExp() != None:
            ctx.Node = ctx.ideqvalExp().Node
        if ctx.ideqidExp() != None:
            ctx.Node = ctx.ideqidExp().Node
        if ctx.ideqvallistExp() != None:
            ctx.Node = ctx.ideqvallistExp().Node
        if ctx.questionref1Exp() != None:
            ctx.Node = ctx.questionref1Exp().Node
        if ctx.rulerefExp() != None:
            ctx.Node = ctx.rulerefExp().Node
        if ctx.stringref1Exp() != None:
            ctx.Node = ctx.stringref1Exp().Node
        if ctx.pushthisExp() != None:
            ctx.Node = ctx.pushthisExp().Node
        if ctx.securityExp() != None:
            ctx.Node = ctx.securityExp().Node
        if ctx.getExp() != None:
            ctx.Node = ctx.getExp().Node
        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#dupExp.
    def visitDupExp(self, ctx:SourceVfrSyntaxParser.DupExpContext):

        self.visitChildren(ctx)
        Line = ctx.start.line
        DObj = IfrDup(Line)
        ctx.Node.Data = DObj
        ctx.Node.Buffer = gFormPkg.StructToStream(DObj.GetInfo())
        self.SaveOpHdrCond(DObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line) #
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vareqvalExp.
    def visitVareqvalExp(self, ctx:SourceVfrSyntaxParser.VareqvalExpContext):

        Line = ctx.start.line
        self.visitChildren(ctx)

        ReturnCode = VfrReturnCode.VFR_RETURN_UNSUPPORTED
        VarIdStr = 'var'
        VarIdStr += ctx.VN.text
        VarStoreId, ReturnCode = gVfrDataStorage.GetVarStoreId(VarIdStr)
        if ReturnCode == VfrReturnCode.VFR_RETURN_UNDEFINED:
            pass #########
        else:
            self.ErrorHandler(ReturnCode, ctx.VN.line)
        QId, Mask, _ = self.VfrQuestionDB.GetQuestionId(None, VarIdStr)
        ConstVal = self.TransNum(ctx.Number(1))
        if ctx.Equal() != None:
            if Mask == 0:
                EIVObj = IfrEqIdVal(Line)
                self.SaveOpHdrCond(EIVObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
                EIVObj.SetQuestionId(QId, VarIdStr, ctx.VN.line)
                EIVObj.SetValue(ConstVal)
                ctx.Node = VfrTreeNode(EFI_IFR_EQ_ID_VAL_OP, EIVObj)
                ctx.ExpInfo.ExpOpCount += 1
            else:
                ctx.Node = self.IdEqValDoSpecial(ctx.ExpInfo, Line, QId, VarIdStr, Mask, ConstVal, EFI_COMPARE_TYPE.EQUAL)
        elif ctx.LessEqual() != None:
            ctx.Node = self.IdEqValDoSpecial(ctx.ExpInfo, Line, QId, VarIdStr, Mask, ConstVal, EFI_COMPARE_TYPE.LESS_EQUAL)

        elif ctx.Less() != None:
            ctx.Node = self.IdEqValDoSpecial(ctx.ExpInfo, Line, QId, VarIdStr, Mask, ConstVal, EFI_COMPARE_TYPE.LESS_THAN)

        elif ctx.GreaterEqual() != None:
            ctx.Node = self.IdEqValDoSpecial(ctx.ExpInfo, Line, QId, VarIdStr, Mask, ConstVal, EFI_COMPARE_TYPE.GREATER_EQUAL)

        elif ctx.Greater() != None:
            ctx.Node = self.IdEqValDoSpecial(ctx.ExpInfo, Line, QId, VarIdStr, Mask, ConstVal, EFI_COMPARE_TYPE.GREATER_THAN)


        return ctx.Node


    def ConvertIdExpr(self, ExpInfo, LineNo, QId, VarIdStr, BitMask):

        QR1Obj = IfrQuestionRef1(LineNo)
        QR1Obj.SetQuestionId(QId, VarIdStr, LineNo)
        Node = VfrTreeNode(EFI_IFR_QUESTION_REF1_OP, QR1Obj)
        self.SaveOpHdrCond(QR1Obj.GetHeader(), (ExpInfo.ExpOpCount == 0))
        if BitMask != 0:
            U32Obj = IfrUint32(LineNo)
            U32Obj.SetValue(BitMask)
            Node.insertChild(VfrTreeNode(EFI_IFR_UINT32_OP, U32Obj, gFormPkg.StructToStream(U32Obj.GetInfo())))

            BWAObj = IfrBitWiseAnd(LineNo)
            Node.insertChild(VfrTreeNode(EFI_IFR_BITWISE_AND_OP, BWAObj, gFormPkg.StructToStream(BWAObj.GetInfo())))

            U8Obj = IfrUint8(LineNo)
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
            Node.insertChild(VfrTreeNode(EFI_IFR_UINT8_OP, U8Obj, gFormPkg.StructToStream(U8Obj.GetInfo())))

            SRObj = IfrShiftRight(LineNo)
            Node.insertChild(VfrTreeNode(EFI_IFR_SHIFT_RIGHT_OP, SRObj, gFormPkg.StructToStream(SRObj.GetInfo())))

        ExpInfo.ExpOpCount += 4
        return Node


    def IdEqValDoSpecial(self, ExpInfo, LineNo, QId, VarIdStr, BitMask, ConstVal, CompareType):

        Node = self.ConvertIdExpr(ExpInfo, LineNo, QId, VarIdStr, BitMask)
        if ConstVal > 0xFF:
            U16Obj = IfrUint16(LineNo)
            U16Obj.SetValue(ConstVal)
            Node.insertChild(VfrTreeNode(EFI_IFR_UINT16_OP, U16Obj, gFormPkg.StructToStream(U16Obj.GetInfo())))
        else:
            U8Obj = IfrUint8(LineNo)
            U8Obj.SetValue(ConstVal)
            Node.insertChild(VfrTreeNode(EFI_IFR_UINT8_OP, U8Obj, gFormPkg.StructToStream(U8Obj.GetInfo())))

        if CompareType == EFI_COMPARE_TYPE.EQUAL:
            EObj = IfrEqual(LineNo)
            Node.insertChild(VfrTreeNode(EFI_IFR_EQUAL_OP, EObj, gFormPkg.StructToStream(EObj.GetInfo())))


        if CompareType == EFI_COMPARE_TYPE.LESS_EQUAL:
            LEObj = IfrLessEqual(LineNo)
            Node.insertChild(VfrTreeNode(EFI_IFR_LESS_EQUAL_OP, LEObj, gFormPkg.StructToStream(LEObj.GetInfo())))

        if CompareType == EFI_COMPARE_TYPE.LESS_THAN:
            LTObj = IfrLessThan(LineNo)
            Node.insertChild(VfrTreeNode(EFI_IFR_LESS_THAN_OP, LTObj, gFormPkg.StructToStream(LTObj.GetInfo())))

        if CompareType == EFI_COMPARE_TYPE.GREATER_EQUAL:
            GEObj = IfrGreaterEqual(LineNo)
            Node.insertChild(VfrTreeNode(EFI_IFR_GREATER_EQUAL_OP, GEObj, gFormPkg.StructToStream(GEObj.GetInfo())))

        if CompareType == EFI_COMPARE_TYPE.GREATER_THAN:
            GTObj = IfrGreaterThan(LineNo)
            Node.insertChild(VfrTreeNode(EFI_IFR_GREATER_THAN_OP, GTObj, gFormPkg.StructToStream(GTObj.GetInfo())))

        ExpInfo.ExpOpCount += 2
        return Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#ideqvalExp.
    def visitIdeqvalExp(self, ctx:SourceVfrSyntaxParser.IdeqvalExpContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        Mask = ctx.vfrQuestionDataFieldName().Mask
        QId = ctx.vfrQuestionDataFieldName().QId
        VarIdStr = ctx.vfrQuestionDataFieldName().VarIdStr
        LineNo = ctx.vfrQuestionDataFieldName().Line
        ConstVal = self.TransNum(ctx.Number())
        if ctx.Equal() != None:
            if Mask == 0:
                EIVObj = IfrEqIdVal(Line)
                self.SaveOpHdrCond(EIVObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
                EIVObj.SetQuestionId(QId, VarIdStr, LineNo)
                EIVObj.SetValue(ConstVal)
                ctx.Node = VfrTreeNode(EFI_IFR_EQ_ID_VAL_OP, EIVObj)
                ctx.ExpInfo.ExpOpCount += 1
            else:
                ctx.Node = self.IdEqValDoSpecial(ctx.ExpInfo, Line, QId, VarIdStr, Mask, ConstVal, EFI_COMPARE_TYPE.EQUAL)

        elif ctx.LessEqual() != None:
            ctx.Node = self.IdEqValDoSpecial(ctx.ExpInfo, Line, QId, VarIdStr, Mask, ConstVal, EFI_COMPARE_TYPE.LESS_EQUAL)

        elif ctx.Less() != None:
            ctx.Node = self.IdEqValDoSpecial(ctx.ExpInfo, Line, QId, VarIdStr, Mask, ConstVal, EFI_COMPARE_TYPE.LESS_THAN)

        elif ctx.GreaterEqual() != None:
            ctx.Node = self.IdEqValDoSpecial(ctx.ExpInfo, Line, QId, VarIdStr, Mask, ConstVal, EFI_COMPARE_TYPE.GREATER_EQUAL)

        elif ctx.Greater() != None:
            ctx.Node = self.IdEqValDoSpecial(ctx.ExpInfo, Line, QId, VarIdStr, Mask, ConstVal, EFI_COMPARE_TYPE.GREATER_THAN)

        return ctx.Node


    def IdEqIdDoSpecial(self, ExpInfo, LineNo, QId1, VarIdStr1, Mask1, QId2, VarIdStr2, Mask2, CompareType):

        Node1 = self.ConvertIdExpr(ExpInfo, LineNo, QId1, VarIdStr1, Mask1)
        Node2 = self.ConvertIdExpr(ExpInfo, LineNo, QId2, VarIdStr2, Mask2)
        Node1.insertChild(Node2)

        if CompareType == EFI_COMPARE_TYPE.EQUAL:
            EObj = IfrEqual(LineNo)
            Node1.insertChild(VfrTreeNode(EFI_IFR_EQUAL_OP, EObj, gFormPkg.StructToStream(EObj.GetInfo())))

        if CompareType == EFI_COMPARE_TYPE.LESS_EQUAL:
            LEObj = IfrLessEqual(LineNo)
            Node1.insertChild(VfrTreeNode(EFI_IFR_LESS_EQUAL_OP, LEObj, gFormPkg.StructToStream(LEObj.GetInfo())))

        if CompareType == EFI_COMPARE_TYPE.LESS_THAN:
            LTObj = IfrLessThan(LineNo)
            Node1.insertChild(VfrTreeNode(EFI_IFR_LESS_THAN_OP, LTObj, gFormPkg.StructToStream(LTObj.GetInfo())))


        if CompareType == EFI_COMPARE_TYPE.GREATER_EQUAL:
            GEObj = IfrGreaterEqual(LineNo)
            Node1.insertChild(VfrTreeNode(EFI_IFR_GREATER_EQUAL_OP, GEObj, gFormPkg.StructToStream(GEObj.GetInfo())))

        if CompareType == EFI_COMPARE_TYPE.GREATER_THAN:
            GTObj = IfrGreaterThan(LineNo)
            Node1.insertChild(VfrTreeNode(EFI_IFR_GREATER_THAN_OP, GTObj, gFormPkg.StructToStream(GTObj.GetInfo())))

        ExpInfo.ExpOpCount += 1
        return Node1


    # Visit a parse tree produced by SourceVfrSyntaxParser#ideqidExp.
    def visitIdeqidExp(self, ctx:SourceVfrSyntaxParser.IdeqidExpContext):
        self.visitChildren(ctx)
        Line = ctx.start.line
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
                ctx.Node = self.IdEqIdDoSpecial(ctx.ExpInfo, Line, QId1, VarIdStr1, Mask1, QId2, VarIdStr2, Mask2, EFI_COMPARE_TYPE.EQUAL)
            else:
                EIIObj = IfrEqIdId(Line)
                self.SaveOpHdrCond(EIIObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
                EIIObj.SetQuestionId1(QId1, VarIdStr1, LineNo1)
                EIIObj.SetQuestionId2(QId2, VarIdStr2, LineNo2)
                ctx.Node = VfrTreeNode(EFI_IFR_EQ_ID_ID_OP, EIIObj)
                ctx.ExpInfo.ExpOpCount += 1

        elif ctx.LessEqual() != None:
            ctx.Node = self.IdEqIdDoSpecial(ctx.ExpInfo, Line, QId1, VarIdStr1, Mask1, QId2, VarIdStr2, Mask2, EFI_COMPARE_TYPE.LESS_EQUAL)

        elif ctx.Less() != None:
            ctx.Node = self.IdEqIdDoSpecial(ctx.ExpInfo, Line, QId1, VarIdStr1, Mask1, QId2, VarIdStr2, Mask2, EFI_COMPARE_TYPE.LESS_THAN)

        elif ctx.GreaterEqual() != None:
            ctx.Node = self.IdEqIdDoSpecial(ctx.ExpInfo, Line, QId1, VarIdStr1, Mask1, QId2, VarIdStr2, Mask2, EFI_COMPARE_TYPE.GREATER_EQUAL)

        elif ctx.Greater() != None:
            ctx.Node = self.IdEqIdDoSpecial(ctx.ExpInfo, Line, QId1, VarIdStr1, Mask1, QId2, VarIdStr2, Mask2, EFI_COMPARE_TYPE.GREATER_THAN)
        return ctx.Node


    def IdEqListDoSpecial(self, ExpInfo, LineNo, QId, VarIdStr, Mask, ListLen, ValueList):
        if ListLen == 0:
            return None

        Node = self.IdEqValDoSpecial(ExpInfo, LineNo, QId, VarIdStr, Mask, ValueList[0], EFI_COMPARE_TYPE.EQUAL)
        for i in range(1, ListLen):
            Node.insertChild(self.IdEqValDoSpecial(ExpInfo, LineNo, QId, VarIdStr, Mask, ValueList[i], EFI_COMPARE_TYPE.EQUAL))
            OObj = IfrOr(LineNo)
            Node.insertChild(VfrTreeNode(EFI_IFR_OR_OP, OObj, gFormPkg.StructToStream(OObj.GetInfo())))
            ExpInfo.ExpOpCount += 1

        return Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#ideqvallistExp.
    def visitIdeqvallistExp(self, ctx:SourceVfrSyntaxParser.IdeqvallistExpContext):
        self.visitChildren(ctx)
        Line = ctx.start.line
        Mask = ctx.vfrQuestionDataFieldName().Mask
        QId = ctx.vfrQuestionDataFieldName().QId
        VarIdStr = ctx.vfrQuestionDataFieldName().VarIdStr
        LineNo = ctx.vfrQuestionDataFieldName().Line
        ValueList = []
        for i in range(0, len(ctx.Number())):
            ValueList.append(self.TransNum(ctx.Number(i)))

        ListLen = len(ValueList)

        if Mask != 0:
            ctx.Node = self.IdEqListDoSpecial(ctx.ExpInfo, LineNo, QId, VarIdStr, Mask, ListLen, ValueList)
        else:
            EILObj = IfrEqIdList(Line, ListLen)
            if QId != EFI_QUESTION_ID_INVALID:
                EILObj.SetQuestionId(QId, VarIdStr, LineNo)
            EILObj.SetListLength(ListLen)
            EILObj.SetValueList(ValueList)
            self.SaveOpHdrCond(EILObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            if QId == EFI_QUESTION_ID_INVALID:
                EILObj.SetQuestionId(QId, VarIdStr, LineNo)

            ctx.Node = VfrTreeNode(EFI_IFR_EQ_ID_VAL_LIST_OP, EILObj)
            ctx.ExpInfo.ExpOpCount += 1
        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrQuestionDataFieldNameRule1.
    def visitVfrQuestionDataFieldNameRule1(self, ctx:SourceVfrSyntaxParser.VfrQuestionDataFieldNameRule1Context):
        ctx.Line = ctx.start.line
        self.visitChildren(ctx)
        ctx.VarIdStr += ctx.SN1.text
        ctx.VarIdStr += '['
        ctx.VarIdStr += ctx.I.text
        ctx.VarIdStr += ']'
        ctx.QId, ctx.Mask, _ = self.VfrQuestionDB.GetQuestionId(None, ctx.VarIdStr)
        if self.ConstantOnlyInExpression:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_CONSTANT_ONLY, ctx.SN1.line)
        return ctx.QId, ctx.Mask, ctx.VarIdStr


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrQuestionDataFieldNameRule2.
    def visitVfrQuestionDataFieldNameRule2(self, ctx:SourceVfrSyntaxParser.VfrQuestionDataFieldNameRule2Context):
        ctx.Line = ctx.start.line
        self.visitChildren(ctx)
        ctx.VarIdStr += ctx.SN2.text
        for i in range(0, len(ctx.arrayName())):
            ctx.VarIdStr += '.'
            if self.ConstantOnlyInExpression:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_CONSTANT_ONLY, ctx.SN2.line)
            ctx.VarIdStr += ctx.arrayName(i).SubStrZ

        ctx.QId, ctx.Mask, _ = self.VfrQuestionDB.GetQuestionId(None, ctx.VarIdStr)
        return ctx.QId, ctx.Mask, ctx.VarIdStr


    # Visit a parse tree produced by SourceVfrSyntaxParser#arrayName.
    def visitArrayName(self, ctx:SourceVfrSyntaxParser.ArrayNameContext):

        self.visitChildren(ctx)
        ctx.SubStr += self.TransId(ctx.StringIdentifier())
        ctx.SubStrZ += self.TransId(ctx.StringIdentifier())
        if ctx.N != None:
            Idx = self.TransNum(ctx.N.text)
            if Idx > 0:
                ctx.SubStr += '['
                ctx.SubStr += str(Idx)
                ctx.SubStr += ']'

            ctx.SubStrZ += '['
            ctx.SubStrZ += str(Idx)
            ctx.SubStrZ += ']'

        return ctx.SubStr, ctx.SubStrZ


    # Visit a parse tree produced by SourceVfrSyntaxParser#questionref1Exp.
    def visitQuestionref1Exp(self, ctx:SourceVfrSyntaxParser.Questionref1ExpContext):
        Line = ctx.start.line #
        QName = None #
        QId = EFI_QUESTION_ID_INVALID
        self.visitChildren(ctx)
        if ctx.StringIdentifier() != None:
            QName = self.TransId(ctx.StringIdentifier())
            QId, _ , _ = self.VfrQuestionDB.GetQuestionId(QName)

        elif ctx.Number() != None:
            QId = self.TransNum(ctx.Number())

        QR1Obj = IfrQuestionRef1(Line)
        self.SaveOpHdrCond(QR1Obj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)

        QR1Obj.SetQuestionId(QId, QName, Line)
        ctx.Node.Data = QR1Obj
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#rulerefExp.
    def visitRulerefExp(self, ctx:SourceVfrSyntaxParser.RulerefExpContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        RRObj = IfrRuleRef(Line)
        self.SaveOpHdrCond(RRObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
        RuleId = self.VfrRulesDB.GetRuleId(self.TransId(ctx.StringIdentifier()))
        RRObj.SetRuleId(RuleId)
        ctx.Node.Data = RRObj
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#stringref1Exp.
    def visitStringref1Exp(self, ctx:SourceVfrSyntaxParser.Stringref1ExpContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        RefStringId = self.PreProcessDB.Read(ctx.S.text)
        ctx.Node.Dict['stringid'] = KV(ctx.S.text, RefStringId)
        SR1Obj = IfrStringRef1(Line)
        self.SaveOpHdrCond(SR1Obj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
        SR1Obj.SetStringId(RefStringId)
        ctx.Node.Data = SR1Obj
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#pushthisExp.
    def visitPushthisExp(self, ctx:SourceVfrSyntaxParser.PushthisExpContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        TObj = IfrThis(Line)
        self.SaveOpHdrCond(TObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
        ctx.Node.Data = TObj
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.Node

    # Visit a parse tree produced by SourceVfrSyntaxParser#securityExp.
    def visitSecurityExp(self, ctx:SourceVfrSyntaxParser.SecurityExpContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        SObj = IfrSecurity(Line)
        self.SaveOpHdrCond(SObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
        Guid = self.PreProcessDB.Read(ctx.S.text)
        ctx.Node.Dict['guid'] = KV(ctx.S.text, Guid)
        SObj.SetPermissions(Guid)
        ctx.Node.Data = SObj
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.ExpInfo

    # Visit a parse tree produced by SourceVfrSyntaxParser#numericVarStoreType.
    def visitNumericVarStoreType(self, ctx:SourceVfrSyntaxParser.NumericVarStoreTypeContext):
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


    # Visit a parse tree produced by SourceVfrSyntaxParser#getExp.
    def visitGetExp(self, ctx:SourceVfrSyntaxParser.GetExpContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        if ctx.BaseInfo.VarStoreId == 0:
            # support Date/Time question
            VarIdStr = ctx.vfrStorageVarId().VarIdStr
            QId, Mask, QType = self.VfrQuestionDB.GetQuestionId(None, VarIdStr, EFI_QUESION_TYPE.QUESTION_NORMAL)
            if (QId == EFI_QUESTION_ID_INVALID) or (Mask == 0) or (QType == EFI_QUESION_TYPE.QUESTION_NORMAL):
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, Line, "Get/Set opcode can't get the enough varstore information")
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
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, Line, "Get/Set opcode can't get the enough varstore information")

        else:
            VarType = EFI_IFR_TYPE_UNDEFINED
            if ctx.FLAGS() != None:
                VarType = ctx.numericVarStoreType().VarType

            if (gVfrDataStorage.GetVarStoreType(ctx.BaseInfo.VarStoreId) == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_NAME) and (VarType == EFI_IFR_TYPE_UNDEFINED):
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, Line, "Get/Set opcode don't support name string")

            if VarType != EFI_IFR_TYPE_UNDEFINED:
                ctx.BaseInfo.VarType = VarType
                Size, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByDataType(ctx.BaseInfo.VarType)
                self.ErrorHandler(ReturnCode, Line, "Get/Set opcode can't get var type size")
                ctx.BaseInfo.VarTotalSize = Size

            Size, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByDataType(ctx.BaseInfo.VarType)
            self.ErrorHandler(ReturnCode, Line, "Get/Set opcode can't get var type size")

            if Size != ctx.BaseInfo.VarTotalSize:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, Line, "Get/Set opcode don't support data array")

        GObj = IfrGet(Line)
        self.SaveOpHdrCond(GObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
        GObj.SetVarInfo(ctx.BaseInfo)
        ctx.Node.Data = GObj
        ctx.ExpInfo.ExpOpCount += 1

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrExpressionConstant.
    def visitVfrExpressionConstant(self, ctx:SourceVfrSyntaxParser.VfrExpressionConstantContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        if ctx.TrueSymbol() != None:
            TObj = IfrTrue(Line)
            self.SaveOpHdrCond(TObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            ctx.Node = VfrTreeNode(EFI_IFR_TRUE_OP, TObj)
            ctx.ExpInfo.ExpOpCount += 1

        if ctx.FalseSymbol() != None:
            FObj = IfrFalse(Line)
            self.SaveOpHdrCond(FObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            ctx.Node = VfrTreeNode(EFI_IFR_FALSE_OP, FObj)
            ctx.ExpInfo.ExpOpCount += 1

        if ctx.One() != None:
            OObj = IfrOne(Line)
            self.SaveOpHdrCond(OObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            ctx.Node = VfrTreeNode(EFI_IFR_ONE_OP, OObj)
            ctx.ExpInfo.ExpOpCount += 1

        if ctx.Ones() != None:
            OObj = IfrOnes(Line)
            self.SaveOpHdrCond(OObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            ctx.Node = VfrTreeNode(EFI_IFR_ONES_OP, OObj)
            ctx.ExpInfo.ExpOpCount += 1

        if ctx.Zero() != None:
            ZObj = IfrZero(Line)
            self.SaveOpHdrCond(ZObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            ctx.Node = VfrTreeNode(EFI_IFR_ZERO_OP, ZObj)
            ctx.ExpInfo.ExpOpCount += 1

        if ctx.Undefined() != None:
            UObj = IfrUndefined(Line)
            self.SaveOpHdrCond(UObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            ctx.Node = VfrTreeNode(EFI_IFR_UNDEFINED_OP, UObj)
            ctx.ExpInfo.ExpOpCount += 1

        if ctx.Version() != None:
            VObj = IfrVersion(Line)
            self.SaveOpHdrCond(VObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            ctx.Node = VfrTreeNode(EFI_IFR_VERSION_OP, VObj)
            ctx.ExpInfo.ExpOpCount += 1

        if ctx.Number() != None:
            U64Obj = IfrUint64(Line)
            U64Obj.SetValue(self.TransNum(ctx.Number()))
            self.SaveOpHdrCond(U64Obj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            ctx.Node = VfrTreeNode(EFI_IFR_UINT64_OP, U64Obj)
            ctx.ExpInfo.ExpOpCount += 1

        return ctx.Node


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrExpressionUnaryOp.
    def visitVfrExpressionUnaryOp(self, ctx:SourceVfrSyntaxParser.VfrExpressionUnaryOpContext):
        self.visitChildren(ctx)
        if ctx.lengthExp() != None:
            ctx.Nodes = ctx.lengthExp().Nodes
        if ctx.bitwisenotExp() != None:
            ctx.Nodes = ctx.bitwisenotExp().Nodes
        if ctx.question23refExp() != None:
            ctx.Nodes = ctx.question23refExp().Nodes
        if ctx.stringref2Exp() != None:
            ctx.Nodes = ctx.stringref2Exp().Nodes
        if ctx.toboolExp() != None:
            ctx.Nodes = ctx.toboolExp().Nodes
        if ctx.tostringExp() != None:
            ctx.Nodes = ctx.tostringExp().Nodes
        if ctx.unintExp() != None:
            ctx.Nodes = ctx.unintExp().Nodes
        if ctx.toupperExp() != None:
            ctx.Nodes = ctx.toupperExp().Nodes
        if ctx.tolwerExp() != None:
            ctx.Nodes = ctx.tolwerExp().Nodes
        if ctx.setExp() != None:
            ctx.Nodes = ctx.setExp().Nodes
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#lengthExp.
    def visitLengthExp(self, ctx:SourceVfrSyntaxParser.LengthExpContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        LObj = IfrLength(Line)
        Node = VfrTreeNode(EFI_IFR_LENGTH_OP, LObj, gFormPkg.StructToStream(LObj.GetInfo()))
        ctx.Nodes.append(Node)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.Nodes

    # Visit a parse tree produced by SourceVfrSyntaxParser#bitwisenotExp.
    def visitBitwisenotExp(self, ctx:SourceVfrSyntaxParser.BitwisenotExpContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        BWNObj = IfrBitWiseNot(Line)
        Node = VfrTreeNode(EFI_IFR_BITWISE_NOT_OP, BWNObj, gFormPkg.StructToStream(BWNObj.GetInfo()))
        ctx.Nodes.append(Node)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#question23refExp.
    def visitQuestion23refExp(self, ctx:SourceVfrSyntaxParser.Question23refExpContext):
        Line = ctx.start.line
        Type = 0x1
        DevicePath = EFI_STRING_ID_INVALID
        self.visitChildren(ctx)
        if ctx.DevicePath() != None:
            Type = 0x2
            DevicePath = self.PreProcessDB.Read(ctx.S.text)

        if ctx.Uuid() != None:
            Type = 0x3
            Guid = self.PreProcessDB.Read(ctx.S2.text)

        if Type == 0x1:
            QR2Obj = IfrQuestionRef2(Line)
            self.SaveOpHdrCond(QR2Obj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            Node = VfrTreeNode(EFI_IFR_QUESTION_REF2_OP, QR2Obj)
            ctx.Nodes.append(Node)

        if Type == 0x2:
            QR3_2Obj = IfrQuestionRef3_2(Line)
            self.SaveOpHdrCond(QR3_2Obj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            QR3_2Obj.SetDevicePath(DevicePath)
            Node = VfrTreeNode(EFI_IFR_QUESTION_REF3_OP, QR3_2Obj)
            Node.Dict['devicepath'] = KV(ctx.S.text, DevicePath)
            ctx.Nodes.append(Node)

        if Type == 0x3:
            QR3_3Obj = IfrQuestionRef3_3(Line)
            self.SaveOpHdrCond(QR3_3Obj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
            QR3_3Obj.SetDevicePath(DevicePath)
            QR3_3Obj.SetGuid(Guid)
            Node = VfrTreeNode(EFI_IFR_QUESTION_REF3_OP, QR3_3Obj)
            Node.Dict['devicepath'] = KV(ctx.S2.text, Guid)
            ctx.Nodes.append(Node)

        ctx.ExpInfo.ExpOpCount += 1


        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#stringref2Exp.
    def visitStringref2Exp(self, ctx:SourceVfrSyntaxParser.Stringref2ExpContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        SR2Obj = IfrStringRef2(Line)
        Node = VfrTreeNode(EFI_IFR_STRING_REF2_OP, SR2Obj, gFormPkg.StructToStream(SR2Obj.GetInfo()))
        ctx.Nodes.append(Node)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#toboolExp.
    def visitToboolExp(self, ctx:SourceVfrSyntaxParser.ToboolExpContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        TBObj = IfrToBoolean(Line)
        Node = VfrTreeNode(EFI_IFR_TO_BOOLEAN_OP, TBObj, gFormPkg.StructToStream(TBObj.GetInfo()))
        ctx.Nodes.append(Node)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.Nodes

    # Visit a parse tree produced by SourceVfrSyntaxParser#tostringExp.
    def visitTostringExp(self, ctx:SourceVfrSyntaxParser.TostringExpContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        TSObj = IfrToString(Line)
        Fmt = self.TransNum(ctx.Number())
        TSObj.SetFormat(Fmt)
        Node = VfrTreeNode(EFI_IFR_TO_STRING_OP, TSObj, gFormPkg.StructToStream(TSObj.GetInfo()))
        ctx.Nodes.append(Node)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.Nodes

    # Visit a parse tree produced by SourceVfrSyntaxParser#unintExp.
    def visitUnintExp(self, ctx:SourceVfrSyntaxParser.UnintExpContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        TUObj = IfrToUint(Line)
        Node = VfrTreeNode(EFI_IFR_TO_UINT_OP, TUObj, gFormPkg.StructToStream(TUObj.GetInfo()))
        ctx.Nodes.append(Node)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#toupperExp.
    def visitToupperExp(self, ctx:SourceVfrSyntaxParser.ToupperExpContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        TUObj = IfrToUpper(Line)
        Node = VfrTreeNode(EFI_IFR_TO_UPPER_OP, TUObj, gFormPkg.StructToStream(TUObj.GetInfo()))
        ctx.Nodes.append(Node)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#tolwerExp.
    def visitTolwerExp(self, ctx:SourceVfrSyntaxParser.TolwerExpContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        TLObj = IfrToLower(Line)
        Node = VfrTreeNode(EFI_IFR_TO_LOWER_OP, TLObj, gFormPkg.StructToStream(TLObj.GetInfo()))
        ctx.Nodes.append(Node)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#setExp.
    def visitSetExp(self, ctx:SourceVfrSyntaxParser.SetExpContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        if ctx.BaseInfo.VarStoreId == 0:
            # support Date/Time question
            VarIdStr = ctx.vfrStorageVarId().VarIdStr
            QId, Mask, QType = self.VfrQuestionDB.GetQuestionId(None, VarIdStr, EFI_QUESION_TYPE.QUESTION_NORMAL)
            if (QId == EFI_QUESTION_ID_INVALID) or (Mask == 0) or (QType == EFI_QUESION_TYPE.QUESTION_NORMAL):
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, Line, "Get/Set opcode can't get the enough varstore information")
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
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, Line, "Get/Set opcode can't get the enough varstore information")

        else:
            VarType = EFI_IFR_TYPE_UNDEFINED
            if ctx.FLAGS() != None:
                VarType = ctx.numericVarStoreType().VarType

            if (gVfrDataStorage.GetVarStoreType(ctx.BaseInfo.VarStoreId) == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_NAME) and (VarType == EFI_IFR_TYPE_UNDEFINED):
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, Line, "Get/Set opcode don't support name string")

            if VarType != EFI_IFR_TYPE_UNDEFINED:
                ctx.BaseInfo.VarType = VarType
                Size, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByDataType(ctx.BaseInfo.VarType)
                self.ErrorHandler(ReturnCode, Line, "Get/Set opcode can't get var type size")
                ctx.BaseInfo.VarTotalSize = Size

            Size, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByDataType(ctx.BaseInfo.VarType)
            self.ErrorHandler(ReturnCode, Line, "Get/Set opcode can't get var type size")

            if Size != ctx.BaseInfo.VarTotalSize:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, Line, "Get/Set opcode don't support data array")

        TSObj = IfrSet(Line)
        self.SaveOpHdrCond(TSObj.GetHeader(), (ctx.ExpInfo.ExpOpCount == 0), Line)
        TSObj.SetVarInfo(ctx.BaseInfo)
        Node = VfrTreeNode(EFI_IFR_SET_OP, TSObj)
        ctx.Nodes.append(Node)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.Nodes

    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrExpressionTernaryOp.
    def visitVfrExpressionTernaryOp(self, ctx:SourceVfrSyntaxParser.VfrExpressionTernaryOpContext):
        self.visitChildren(ctx)
        if ctx.conditionalExp() != None:
            ctx.Nodes = ctx.conditionalExp().Nodes
        if ctx.findExp() != None:
            ctx.Nodes = ctx.findExp().Nodes
        if ctx.midExp() != None:
            ctx.Nodes = ctx.midExp().Nodes
        if ctx.tokenExp() != None:
            ctx.Nodes = ctx.tokenExp().Nodes
        if ctx.spanExp() != None:
            ctx.Nodes = ctx.spanExp().Nodes
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#conditionalExp.
    def visitConditionalExp(self, ctx:SourceVfrSyntaxParser.ConditionalExpContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        CObj = IfrConditional(Line)
        Node = VfrTreeNode(EFI_IFR_CONDITIONAL_OP, CObj, gFormPkg.StructToStream(CObj.GetInfo()))
        ctx.Nodes.append(Node)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.Nodes

    # Visit a parse tree produced by SourceVfrSyntaxParser#findExp.
    def visitFindExp(self, ctx:SourceVfrSyntaxParser.FindExpContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        FObj = IfrFind(Line)
        for i in range(0, len(ctx.findFormat())):
            Format = ctx.findFormat(i).Format
            FObj.SetFormat(Format)
        Node = VfrTreeNode(EFI_IFR_FIND_OP, FObj, gFormPkg.StructToStream(FObj.GetInfo()))
        ctx.Nodes.append(Node)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#findFormat.
    def visitFindFormat(self, ctx:SourceVfrSyntaxParser.FindFormatContext):
        self.visitChildren(ctx)
        if ctx.Sensitive() != None:
            ctx.Format = 0x00
        elif ctx.Insensitive() != None:
            ctx.Format = 0x01
        return ctx.Format


    # Visit a parse tree produced by SourceVfrSyntaxParser#midExp.
    def visitMidExp(self, ctx:SourceVfrSyntaxParser.MidExpContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        MObj = IfrMid(Line)
        Node = VfrTreeNode(EFI_IFR_MID_OP, MObj, gFormPkg.StructToStream(MObj.GetInfo()))
        ctx.Nodes.append(Node)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.Nodes

    # Visit a parse tree produced by SourceVfrSyntaxParser#tokenExp.
    def visitTokenExp(self, ctx:SourceVfrSyntaxParser.TokenExpContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        TObj = IfrToken(Line)
        Node = VfrTreeNode(EFI_IFR_TOKEN_OP, TObj, gFormPkg.StructToStream(TObj.GetInfo()))
        ctx.Nodes.append(Node)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#spanExp.
    def visitSpanExp(self, ctx:SourceVfrSyntaxParser.SpanExpContext):
        Line = ctx.start.line
        Flags = 0
        self.visitChildren(ctx)
        for FlagsCtx in ctx.spanFlags():
            Flags |= FlagsCtx.Flag
        SObj = IfrSpan(Line)
        SObj.SetFlags(Flags)
        Node = VfrTreeNode(EFI_IFR_SPAN_OP, SObj, gFormPkg.StructToStream(SObj.GetInfo()))
        ctx.Nodes.append(Node)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.Nodes


    # Visit a parse tree produced by SourceVfrSyntaxParser#spanFlags.
    def visitSpanFlags(self, ctx:SourceVfrSyntaxParser.SpanFlagsContext):
        self.visitChildren(ctx)
        if ctx.Number() != None:
            ctx.Flag = self.TransNum(ctx.Number())
        elif ctx.LastNonMatch() != None:
            ctx.Flag = 0x00
        elif ctx.FirstNonMatch() != None:
            ctx.Flag = 0x01
        return ctx.Flag


    # Visit a parse tree produced by SourceVfrSyntaxParser#vfrExpressionMap.
    def visitVfrExpressionMap(self, ctx:SourceVfrSyntaxParser.VfrExpressionMapContext):
        Line = ctx.start.line
        self.visitChildren(ctx)
        MObj = IfrMap(Line)
        Node = VfrTreeNode(EFI_IFR_MAP_OP, MObj, gFormPkg.StructToStream(MObj.GetInfo()))
        ctx.Nodes.append(Node)
        for Node in ctx.Node.Child:
            ctx.Nodes.append(Node)
        EObj = IfrEnd()
        EObj.SetLineNo(ctx.stop.line)
        Node = VfrTreeNode(EFI_IFR_END_OP, EObj, gFormPkg.StructToStream(EObj.GetInfo()))
        ctx.Nodes.append(Node)
        ctx.ExpInfo.ExpOpCount += 1
        return ctx.Nodes

    def GetText(self, ctx):
        if ctx == None:
            return None
        else:
            return ctx.text

    def TransId(self, StringIdentifierToken, DefaultValue=None):
        if StringIdentifierToken == None:
            return DefaultValue
        else:
            return str(StringIdentifierToken)

    def TransNum(self, NumberToken, DefaultValue=0):
        if NumberToken == None:
            return DefaultValue
        else:
            StrToken = str(NumberToken)
            if '0x' in StrToken:
                NumberToken = int(StrToken, 0)
            else:
                NumberToken = int(StrToken)
        # error handle , value is too large to store
        return NumberToken

    def AssignQuestionKey(self, OpObj, Key):

        if Key == None:
            return

        if OpObj.GetQFlags() & EFI_IFR_FLAG_CALLBACK:
            # if the question is not CALLBACK ignore the key.
            self.VfrQuestionDB.UpdateQuestionId(OpObj.GetQuestionId(), Key, gFormPkg)
            OpObj.SetQuestionId(Key)
        return

    def ExtractOriginalText(self, ctx):
        Source = ctx.start.getTokenSource()
        InputStream = Source.inputStream
        start, stop  = ctx.start.start, ctx.stop.stop
        Text = InputStream.getText(start, stop)
        return Text.replace('\r', '').replace('\n', '').replace('  ', ' ')

    def CheckDuplicateDefaultValue(self, DefaultId, Line, TokenValue):
        for i in range(0, len(self.UsedDefaultArray)):
            if self.UsedDefaultArray[i] == DefaultId:
                gVfrErrorHandle.HandleWarning(EFI_VFR_WARNING_CODE.VFR_WARNING_DEFAULT_VALUE_REDEFINED, Line, TokenValue)

        if len(self.UsedDefaultArray) >= EFI_IFR_MAX_DEFAULT_TYPE - 1:
            gVfrErrorHandle.HandleError(VfrReturnCode.VFR_RETURN_FATAL_ERROR, Line, TokenValue)

        self.UsedDefaultArray.append(DefaultId)

    def ErrorHandler(self, ReturnCode, LineNum, TokenValue=None):
        self.ParserStatus += gVfrErrorHandle.HandleError(ReturnCode, LineNum, TokenValue)

    def CompareErrorHandler(self, ReturnCode, ExpectedCode, LineNum, TokenValue=None, ErrorMsg=None):
        if ReturnCode != ExpectedCode:
            self.ParserStatus += 1
            gVfrErrorHandle.PrintMsg(LineNum, 'Error', ErrorMsg, TokenValue)

    def InsertChild(self, ParentNode: VfrTreeNode, ChildCtx):
        if ChildCtx != None and ChildCtx.Node != None:
            ParentNode.insertChild(ChildCtx.Node)

    def InsertEndNode(self, ParentNode, Line):
        EObj = IfrEnd()
        EObj.SetLineNo(Line)
        ENode = VfrTreeNode(EFI_IFR_END_OP, EObj, gFormPkg.StructToStream(EObj.GetInfo()))
        ParentNode.insertChild(ENode)

    def GetCurArraySize(self):

        Size = 1
        if self.CurrQestVarInfo.VarType == EFI_IFR_TYPE_NUM_SIZE_8:
            Size = 1
        if self.CurrQestVarInfo.VarType == EFI_IFR_TYPE_NUM_SIZE_16:
            Size = 2
        if self.CurrQestVarInfo.VarType == EFI_IFR_TYPE_NUM_SIZE_32:
            Size = 4
        if self.CurrQestVarInfo.VarType == EFI_IFR_TYPE_NUM_SIZE_64:
            Size = 8

        return int(self.CurrQestVarInfo.VarTotalSize / Size)

    def GetQuestionDB(self):
        return self.VfrQuestionDB



del SourceVfrSyntaxParser