from SourceVfrSyntaxVisitor import *
from SourceVfrSyntaxLexer import *
from SourceVfrSyntaxParser import *
from VfrFormPkg import *
from VfrError import *
from VfrPreProcess import *

class YamlTree():
    def __init__(self, Root: VfrTreeNode, PreProcessDB: PreProcessDB, Options: Options):
        self.Options = Options
        self.Root = Root
        self.PreProcessDB = PreProcessDB
        self.VfrTree = VfrTree(Root, PreProcessDB, Options)
        self.YamlDict = None

    # transform string token and Header key-values to specific values for yaml
    def PreProcess(self):
        try:
            FileName = self.Options.YamlFileName
            f = open(FileName, 'r')
            #self.YamlDict = yaml.load(f.read(), Loader=yaml.FullLoader)
            self.YamlDict = yaml.safe_load(f)
            f.close()
        except:
            EdkLogger.error('VfrCompiler', FILE_OPEN_FAILURE,
                            'File open failed for %s' % FileName, None)

        # UniDict = self._GetUniDictsForYaml()
        # HeaderDict = self._GetHeaderDictsForYaml(self.YamlDict['include'])
        self._PreprocessYaml(self.YamlDict)

        try:
            FileName =  self.Options.ProcessedYAMLFileName
            f = open(FileName, 'w')
            f.write(yaml.dump(self.YamlDict, allow_unicode=True, default_flow_style=False))
            f.close()
        except:
            EdkLogger.error('VfrCompiler', FILE_CREATE_FAILURE,
                            'File create failed for %s' % FileName, None)

        #self._GenExpandedHeaderFile()

    def Compile(self):
        Parser = YamlParser(self.YamlDict, self.Root, self.Options)
        Parser.Parse()
        #print(self.YamlDict['formset'])
        self.VfrTree.DumpProcessedYaml()

    def _PreprocessYaml(self, YamlDict):
        for Key in YamlDict.keys():
            Value = YamlDict[Key]
            if isinstance(Value, list): # value is list
                for Item in Value:
                    if isinstance(Item, dict):
                        self._PreprocessYaml(Item)

            elif isinstance(Value, dict): # value is dict
                self._PreprocessYaml(Value)

            elif isinstance(Value, str):# value is str
                # get the specific value of string token
                Start = Value.find('(') + 1
                End = Value.find(')')
                if Value[Start:End] in self.PreProcessDB.UniDict.keys():
                    YamlDict[Key] = self.PreProcessDB.TransValue(self.PreProcessDB.UniDict[Value[Start:End]])
                    #YamlDict[Key] = 'STRING_TOKEN' + '(' +  self.PreProcessDB.UniDict[Value[Start:End]] + ')'
                # get {key:value} dicts from header files
                elif Value in self.PreProcessDB.HeaderDict.keys():
                    YamlDict[Key] = self.PreProcessDB.TransValue(self.PreProcessDB.HeaderDict[Value])
                elif '|' in Value:
                    Items = Value.split('|')
                    ValueList = []
                    for i in range(0, len(Items)):
                        # get {key:value} dicts from header files
                        if Items[i].strip() in self.PreProcessDB.HeaderDict.keys():
                            ValueList.append(self.PreProcessDB.TransValue(self.PreProcessDB.HeaderDict[Items[i].strip()]))
                        elif ('0x' in Items[i].strip()) or ('0X' in Items[i].strip()):
                            ValueList.append(self.PreProcessDB.TransValue(Items[i].strip()))
                        else:
                            ValueList.append(Items[i].strip())

                    YamlDict[Key] = ValueList
        return

QuestionheaderFlagsField = ['READ_ONLY', 'INTERACTIVE', 'RESET_REQUIRED', 'REST_STYLE', 'RECONNECT_REQUIRED', 'OPTIONS_ONLY', 'NV_ACCESS', 'LATE_CHECK']

class YamlParser():
    def __init__(self, YamlDict, Root: VfrTreeNode, Options: Options):
        self.YamlDict = YamlDict
        self.Options = Options
        self.Root = Root
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

    def Parse(self):
        self._ParseExpandedHeader()
        self._ParseVfrFormSetDefinition()

    # parse and collect data structures info in the ExpandedHeader.i files
    def _ParseExpandedHeader(self):
        try:
            InputStream = FileStream(self.Options.ProcessedInFileName) ###
            VfrLexer = SourceVfrSyntaxLexer(InputStream)
            VfrStream = CommonTokenStream(VfrLexer)
            VfrParser = SourceVfrSyntaxParser(VfrStream)
        except:
            EdkLogger.error('VfrCompiler', FILE_OPEN_FAILURE,
                            'File open failed for %s' % self.Options.ProcessedInFileName, None)

        self.Visitor = SourceVfrSyntaxVisitor(None, self.Options.OverrideClassGuid)
        gVfrVarDataTypeDB.Clear()
        self.Visitor.visit(VfrParser.vfrHeader())
        FileName = self.Options.OutputDirectory + self.Options.VfrBaseFileName + 'Header.txt'
        try:
            f = open(FileName, 'w')
            gVfrVarDataTypeDB.Dump(f)
            f.close()
        except:
            EdkLogger.error('VfrCompiler', FILE_OPEN_FAILURE,
                            'File open failed for %s' % FileName, None)

    def _DeclareStandardDefaultStorage(self):

        DSObj = IfrDefaultStore('Standard Defaults')
        gVfrDefaultStore.Clear()
        gVfrDefaultStore.RegisterDefaultStore(DSObj.DefaultStore, 'Standard Defaults', EFI_STRING_ID_INVALID, EFI_HII_DEFAULT_CLASS_STANDARD)
        DSObj.SetDefaultName (EFI_STRING_ID_INVALID)
        DSObj.SetDefaultId (EFI_HII_DEFAULT_CLASS_STANDARD)
        DsNode = VfrTreeNode(EFI_IFR_DEFAULTSTORE_OP, DSObj, gFormPkg.StructToStream(DSObj.GetInfo()))
        DSObjMF = IfrDefaultStore('Standard ManuFacturing')
        gVfrDefaultStore.RegisterDefaultStore(DSObjMF.DefaultStore, 'Standard ManuFacturing', EFI_STRING_ID_INVALID, EFI_HII_DEFAULT_CLASS_MANUFACTURING)
        DSObjMF.SetDefaultName (EFI_STRING_ID_INVALID)
        DSObjMF.SetDefaultId (EFI_HII_DEFAULT_CLASS_MANUFACTURING)
        DsNodeMF = VfrTreeNode(EFI_IFR_DEFAULTSTORE_OP, DSObjMF, gFormPkg.StructToStream(DSObjMF.GetInfo()))

        return DsNode, DsNodeMF

    def _ToList(self, Value):
        if type(Value) == str or type(Value) == int or type(Value) == EFI_GUID:
            return [Value]

    # parse Formset Definition
    def _ParseVfrFormSetDefinition(self):

        Formset = self.YamlDict['formset']

        ClassGuidNum = 0
        if 'classguid' in Formset.keys():
            GuidList = self._ToList(Formset['classguid'])
            ClassGuidNum = len(GuidList)

        DefaultClassGuid = EFI_HII_PLATFORM_SETUP_FORMSET_GUID

        if (self.Options.OverrideClassGuid != None and ClassGuidNum >= 4):
            pass
            #self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, ctx.start.line, None, 'Already has 4 class guids, can not add extra class guid!')

        if ClassGuidNum == 0:
            if self.Options.OverrideClassGuid != None:
                ClassGuidNum  = 2
            else:
                ClassGuidNum  = 1
            FSObj = IfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID))
            FSObj.SetClassGuid(DefaultClassGuid)
            if (self.Options.OverrideClassGuid != None):
                FSObj.SetClassGuid(self.Options.OverrideClassGuid)

        elif ClassGuidNum == 1:
            if self.Options.OverrideClassGuid != None:
                ClassGuidNum  += 1
            FSObj = IfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID))
            FSObj.SetClassGuid(GuidList[0])
            if (self.Options.OverrideClassGuid != None):
                FSObj.SetClassGuid(self.Options.OverrideClassGuid)

        elif ClassGuidNum == 2:
            if self.Options.OverrideClassGuid != None:
                ClassGuidNum += 1
            FSObj = IfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID))
            FSObj.SetClassGuid(GuidList[0])
            FSObj.SetClassGuid(GuidList[1])
            if (self.Options.OverrideClassGuid != None):
                FSObj.SetClassGuid(self.Options.OverrideClassGuid)

        elif ClassGuidNum == 3:
            if self.Options.OverrideClassGuid != None:
                ClassGuidNum  += 1
            FSObj = IfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID))
            FSObj.SetClassGuid(GuidList[0])
            FSObj.SetClassGuid(GuidList[1])
            FSObj.SetClassGuid(GuidList[2])
            if (self.Options.OverrideClassGuid != None):
                FSObj.SetClassGuid(self.Options.OverrideClassGuid)

        elif ClassGuidNum == 4:
            if self.Options.OverrideClassGuid != None:
                ClassGuidNum  += 1
            FSObj = IfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID))
            FSObj.SetClassGuid(GuidList[0])
            FSObj.SetClassGuid(GuidList[1])
            FSObj.SetClassGuid(GuidList[2])
            FSObj.SetClassGuid(GuidList[3])

        #FSObj.SetLineNo(ctx.start.line)
        FSObj.SetGuid(Formset['guid'])
        FSObj.SetFormSetTitle(Formset['title'])
        FSObj.SetHelp(Formset['help'])

        Node = VfrTreeNode(EFI_IFR_FORM_SET_OP, FSObj, gFormPkg.StructToStream(FSObj.GetInfo()), '')
        for i in range(0, len(GuidList)):
            Node.Buffer += gFormPkg.StructToStream(GuidList[i])

        self.Root.insertChild(Node)

        if 'class' in Formset.keys():
            self._ParseClassDefinition(self._ToList(Formset['class']), Node)

        if 'subclass' in Formset.keys():
            self._ParseSubClassDefinition(Formset['subclass'], Node)

        DsNode, DsNodeMF = self._DeclareStandardDefaultStorage()
        Node.insertChild(DsNode)
        Node.insertChild(DsNodeMF)

        if 'component' in Formset.keys():
            self._ParseVfrFormSetComponent(Formset['component'], Node)

    def _ParseClassDefinition(self, ClassList, ParentNode: VfrTreeNode):
        CObj = IfrClass()
        Class = 0
        for ClassName in ClassList:
            if ClassName == 'NON_DEVICE':
                Class |= EFI_NON_DEVICE_CLASS
            elif ClassName == 'DISK_DEVICE':
                Class |= EFI_DISK_DEVICE_CLASS
            elif ClassName == 'VIDEO_DEVICE':
                Class |= EFI_VIDEO_DEVICE_CLASS
            elif ClassName == 'NETWORK_DEVICE':
                Class |= EFI_NETWORK_DEVICE_CLASS
            elif ClassName == 'INPUT_DEVICE':
                Class |= EFI_INPUT_DEVICE_CLASS
            elif ClassName == 'ONBOARD_DEVICE':
                Class |= EFI_ON_BOARD_DEVICE_CLASS
            elif ClassName == 'OTHER_DEVICE':
                Class |= EFI_OTHER_DEVICE_CLASS
            else:
                Class |= ClassName
        CObj.SetClass(Class)
        Node = VfrTreeNode(EFI_IFR_GUID_OP, CObj, gFormPkg.StructToStream(CObj.GetInfo()))
        ParentNode.insertChild(Node)
        return Node

    def _ParseSubClassDefinition(self, SubClassVar, ParentNode: VfrTreeNode):
        SubObj = IfrSubClass()
        SubClass = 0
        if SubClassVar == 'SETUP_APPLICATION':
            SubClass |= EFI_SETUP_APPLICATION_SUBCLASS
        elif SubClassVar == 'GENERAL_APPLICATION':
            SubClass |= EFI_GENERAL_APPLICATION_SUBCLASS
        elif SubClassVar == 'FRONT_PAGE':
            SubClass |= EFI_FRONT_PAGE_SUBCLASS
        elif SubClassVar == 'SINGLE_USE':
            SubClass |= EFI_SINGLE_USE_SUBCLASS
        else:
            SubClass |= SubClassVar

        SubObj.SetSubClass(SubClass)
        Node = VfrTreeNode(EFI_IFR_GUID_OP, SubObj, gFormPkg.StructToStream(SubObj.GetInfo()))
        ParentNode.insertChild(Node)
        return Node

    def _ParseVfrFormSetComponent(self, ComponentList, Node):
        for Component in ComponentList:
            (key, value), = Component.items()
            if key == 'form':
                self._ParsevfrFormDefinition(value, Node)

    def _ParsevfrFormDefinition(self, Form, ParentNode: VfrTreeNode):
        FObj = IfrForm()
        FormId = Form['formid']
        FObj.SetFormId(FormId)

        FormTitle = Form['title']
        FObj.SetFormTitle(FormTitle)
        Node = VfrTreeNode(EFI_IFR_FORM_OP, FObj, gFormPkg.StructToStream(FObj.GetInfo()))
        ParentNode.insertChild(Node)
        if 'component' in Form.keys():
            self._ParsevfrFormComponent(Form['component'], Node)

    def _ParsevfrFormComponent(self, ComponentList, Node):
        for Component in ComponentList:
            (key, value), = Component.items()
            if key == 'label':
                self._ParsevfrStatementLabel(value, Node)
            if key == 'text':
                self._ParsevfrStatementStaticText(value, Node)


    def _ParsevfrStatementLabel(self, Label, ParentNode):
        LObj = IfrLabel()
        Number = Label['number']
        LObj.SetNumber(Number)
        Node = VfrTreeNode(EFI_IFR_GUID_OP, LObj, gFormPkg.StructToStream(LObj.GetInfo()))
        ParentNode.insertChild(Node)
        return Node

    def _ParseQuestionheaderFlagsField(self, Flag):
        QHFlag = 0
        if Flag == 'READ_ONLY':
            QHFlag = 0x01

        elif Flag == 'INTERACTIVE':
           QHFlag = 0x04

        elif Flag == 'RESET_REQUIRED':
            QHFlag = 0x10

        elif Flag == 'REST_STYLE':
            QHFlag = 0x20

        elif Flag == 'RECONNECT_REQUIRED':
            QHFlag = 0x40

        else:
            gVfrErrorHandle.HandleWarning(EFI_VFR_WARNING_CODE.VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE)
        return QHFlag

    def _ParsevfrStatementStaticText(self, Text, ParentNode):
        Help = Text['help']
        Prompt = Text['prompt']

        QId = EFI_QUESTION_ID_INVALID
        TxtTwo = EFI_STRING_ID_INVALID
        if 'text' in Text.keys():
            TxtTwo = Text['text']

        TextFlags = 0
        if 'flags' in Text.keys():
            for Flag in self._ToList(Text['flags']):
                RetFlag = 0
                if Flag in QuestionheaderFlagsField:
                    RetFlag = self._ParseQuestionheaderFlagsField(Flag)
                elif Flag != 0:
                    self._ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED)
                TextFlags |= RetFlag

        if TextFlags & EFI_IFR_FLAG_CALLBACK:
            if TxtTwo != EFI_STRING_ID_INVALID:
                gVfrErrorHandle.HandleWarning(EFI_VFR_WARNING_CODE.VFR_WARNING_ACTION_WITH_TEXT_TWO)
            AObj = IfrAction()
            QId, _ = self.VfrQuestionDB.RegisterQuestion(None, None, QId, gFormPkg)
            AObj.SetQuestionId(QId)
            AObj.SetHelp(Help)
            AObj.SetPrompt(Prompt)
            self._ErrorHandler(AObj.SetFlags(TextFlags))
            if 'key' in Text.keys():
                self.AssignQuestionKey(AObj, Text['key'])
            Node = VfrTreeNode(EFI_IFR_TEXT_OP, AObj, gFormPkg.StructToStream(AObj.GetInfo()))
            ParentNode.insertChild(Node)
        else:
            TObj = IfrText()
            TObj.SetHelp(Help)
            TObj.SetPrompt(Prompt)
            TObj.SetTextTwo(TxtTwo)
            Node = VfrTreeNode(EFI_IFR_TEXT_OP, TObj, gFormPkg.StructToStream(TObj.GetInfo()))
            ParentNode.insertChild(Node)

        return Node


    def AssignQuestionKey(self, OpObj, Key):

        if Key == None:
            return

        if OpObj.GetQFlags() & EFI_IFR_FLAG_CALLBACK:
            # if the question is not CALLBACK ignore the key.
            self.VfrQuestionDB.UpdateQuestionId(OpObj.GetQuestionId(), Key, gFormPkg)
            OpObj.SetQuestionId(Key)
        return

    def _ErrorHandler(self, ReturnCode, LineNum=None, TokenValue=None):
        self.ParserStatus += gVfrErrorHandle.HandleError(ReturnCode, LineNum, TokenValue)
