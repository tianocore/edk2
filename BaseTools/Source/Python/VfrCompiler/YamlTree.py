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
vfrStatementStat = ['subtitle', 'text', 'goto', 'resetbutton']
vfrStatementQuestions = ['checkbox', 'action', 'date','numeric', 'oneof', 'string', 'password', 'orderedlist', 'time']
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
        self.ParseVfrFormSetDefinition()

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
    def ParseVfrFormSetDefinition(self):

        Formset = self.YamlDict['formset']

        ClassGuidNum = 0
        if 'classguid' in Formset.keys():
            GuidList = self._ToList(Formset['classguid'])
            ClassGuidNum = len(GuidList)

        DefaultClassGuid = EFI_HII_PLATFORM_SETUP_FORMSET_GUID

        if (self.Options.OverrideClassGuid != None and ClassGuidNum >= 4):
            pass
            #self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, None, 'Already has 4 class guids, can not add extra class guid!')

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

        #FSObj.SetLineN)
        FSObj.SetGuid(Formset['guid'])
        FSObj.SetFormSetTitle(Formset['title'])
        FSObj.SetHelp(Formset['help'])

        Node = VfrTreeNode(EFI_IFR_FORM_SET_OP, FSObj, gFormPkg.StructToStream(FSObj.GetInfo()), '')
        for i in range(0, len(GuidList)):
            Node.Buffer += gFormPkg.StructToStream(GuidList[i])

        self.Root.insertChild(Node)

        if 'class' in Formset.keys():
            self.ParseClassDefinition(self._ToList(Formset['class']), Node)

        if 'subclass' in Formset.keys():
            self.ParseSubClassDefinition(Formset['subclass'], Node)

        DsNode, DsNodeMF = self._DeclareStandardDefaultStorage()
        Node.insertChild(DsNode)
        Node.insertChild(DsNodeMF)

        if 'component' in Formset.keys():
            self._ParseVfrFormSetComponent(Formset['component'], Node)

    def ParseClassDefinition(self, ClassList, ParentNode: VfrTreeNode):
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

    def ParseSubClassDefinition(self, SubClassVar, ParentNode: VfrTreeNode):
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
                self.ParseVfrFormDefinition(value, Node)

    def ParseVfrFormDefinition(self, Form, ParentNode: VfrTreeNode):
        FObj = IfrForm()
        FormId = Form['formid']
        FObj.SetFormId(FormId)

        FormTitle = Form['title']
        FObj.SetFormTitle(FormTitle)
        Node = VfrTreeNode(EFI_IFR_FORM_OP, FObj, gFormPkg.StructToStream(FObj.GetInfo()))
        ParentNode.insertChild(Node)
        if 'component' in Form.keys():
            for Component in Form['component']:
                (Key, Value), = Component.items()
                if Key == 'image':
                    self.ParseVfrStatementImage(Value, ParentNode)
                elif Key == 'locked':
                    self.ParseVfrStatementLocked(Value, ParentNode)
                elif Key == 'rule':
                    self.ParseVfrStatementRule(Value, ParentNode)
                elif Key == 'default':
                    self.ParseVfrStatementDefault(Value, ParentNode)
                elif Key in vfrStatementStat:
                    self.ParseVfrStatementStat(Key, Value, ParentNode)
                elif Key in vfrStatementQuestions:
                    self.ParseVfrStatementQuestions(Key, Value, ParentNode)
                elif Key == 'label':
                    self.ParseVfrStatementLabel(Value, ParentNode)
                elif Key == 'banner':
                    self.ParseVfrStatementbanner(Value, ParentNode)
                elif Key == 'guidop':
                    self.ParseVfrStatementExtension(Value, ParentNode)
                elif Key == 'modal':
                    self.ParseVfrStatementModal(Value, ParentNode)
                elif Key == 'refreshguid':
                    self.ParseVfrStatementRefreshEvent(Value, ParentNode)

    def ParseVfrStatementDefault(self, Default, ParentNode):
        if 'value' in Default.keys():
            ValueList = self._ParseVfrConstantValueField(Default['value'])
            Value = ValueList[0]
            Type = self.CurrQestVarInfo.VarType
            if self.CurrentMinMaxData != None and self.CurrentMinMaxData.IsNumericOpcode():
                # check default value is valid for Numeric Opcode
                for i in range(0, len(ValueList)):
                    Value = ValueList[i]
                    if type(Value) == int:
                        if Value < self.CurrentMinMaxData.GetMinData() or Value > self.CurrentMinMaxData.GetMaxData():
                            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, "Numeric default value must be between MinValue and MaxValue.")

            if Type == EFI_IFR_TYPE_OTHER:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_FATAL_ERROR, "Default data type error.")
            DObj = IfrDefault(Type, ValueList)
            Node = VfrTreeNode(EFI_IFR_DEFAULT_OP, DObj)
            if len(ValueList) == 1:
                if self.IsStringOp:
                    DObj.SetType(EFI_IFR_TYPE_STRING)
                else:
                    if self.CurrQestVarInfo.IsBitVar:
                        DObj.SetType(EFI_IFR_TYPE_NUM_SIZE_32)
                    else:
                        DObj.SetType(self.CurrQestVarInfo.VarType)
            else:
                DObj.SetType(EFI_IFR_TYPE_BUFFER)
        elif 'value2' in Default.keys():
            IsExp = True
            DObj = IfrDefault2()
            Node = VfrTreeNode(EFI_IFR_DEFAULT_OP, DObj)
            DObj.SetScope(1)
            self.ParseVfrStatementValue(Default['value2'], Node)

        if 'defaultstore' in Default.keys():
            DefaultId, ReturnCode = gVfrDefaultStore.GetDefaultId(Default['defaultstore'])
            self.ErrorHandler(ReturnCode)
            DObj.SetDefaultId(DefaultId)

        self._CheckDuplicateDefaultValue(DefaultId)
        if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            VarStoreName, ReturnCode = gVfrDataStorage.GetVarStoreName(self.CurrQestVarInfo.VarStoreId)
            self.ErrorHandler(ReturnCode)
            VarGuid = gVfrDataStorage.GetVarStoreGuid(self.CurrQestVarInfo.VarStoreId)
            VarStoreType = gVfrDataStorage.GetVarStoreType(self.CurrQestVarInfo.VarStoreId)
            if (IsExp == False) and (VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER):
                self.ErrorHandler(gVfrDefaultStore.BufferVarStoreAltConfigAdd(DefaultId, self.CurrQestVarInfo, VarStoreName, VarGuid, self.CurrQestVarInfo.VarType, Value))

        Node.Buffer = gFormPkg.StructToStream(DObj.GetInfo())
        ParentNode.insertChild(Node)
        return Node

    def ParseVfrStatementRule(self, Rule, ParentNode):
        RObj = IfrRule()
        RuleName = Rule['rulename']
        RObj.SetRuleName(RuleName)
        self.VfrRulesDB.RegisterRule(RuleName)
        RObj.SetRuleId(self.VfrRulesDB.GetRuleId(RuleName))
        Node = VfrTreeNode(EFI_IFR_RULE_OP, RObj, gFormPkg.StructToStream(RObj.GetInfo()))
        ParentNode.insertChild(Node)
        # exp
        return Node

    def ParseVfrStatementExtension(self, GuidOp, ParentNode):
        pass

    def ParseVfrStatementbanner(self, Banner, ParentNode):
        BObj = IfrBanner()
        BObj.SetTitle(Banner['title'])
        if 'line' in Banner.keys():
            BObj.SetLine(Banner['line'])
        if 'left' in Banner.keys():
            BObj.SetAlign(0)
        if 'center' in Banner.keys():
            BObj.SetAlign(1)
        if 'right' in Banner.keys():
            BObj.SetAlign(2)
        Node = VfrTreeNode(EFI_IFR_GUID_OP, BObj, gFormPkg.StructToStream(BObj.GetInfo()))
        ParentNode.insertChild(Node)
        ########　timeout
        return Node


    def ParseVfrStatementModal(self, Modal, ParentNode):
        MObj = IfrModal()
        Node = VfrTreeNode(EFI_IFR_MODAL_TAG_OP, MObj, gFormPkg.StructToStream(MObj.GetInfo()))
        ParentNode.insertChild(Node)
        return Node

    def ParseVfrStatementRefreshEvent(self, RefreshEvent, ParentNode):
        RiObj = IfrRefreshId()
        RiObj.SetRefreshEventGroutId(RefreshEvent['refreshguid'])
        Node = VfrTreeNode(EFI_IFR_REFRESH_ID_OP, RiObj, gFormPkg.StructToStream(RiObj.GetInfo()))
        ParentNode.insertChild(Node)
        return Node

    def ParseVfrStatementStat(self, Key, Value, ParentNode):
        if Key == 'subtitle':
            self.ParseVfrStatementSubtitle(Value, ParentNode)
        if Key == 'text':
            self.ParseVfrStatementStaticText(Value, ParentNode)
        if Key == 'goto':
            self.ParseVfrStatementGoto(Value, ParentNode)
        if Key == 'resetbutton':
            self.ParseVfrStatementResetButton(Value, ParentNode)

    def ParseVfrStatementSubtitle(self, Subtitle, ParentNode):
        SObj = IfrSubtitle()
        Prompt = Subtitle['prompt']
        SObj.SetPrompt(Prompt)
        SObj.SetFlags(self._ParseSubtitleFlags(Subtitle))
        Node = VfrTreeNode(EFI_IFR_SUBTITLE_OP, SObj, gFormPkg.StructToStream(SObj.GetInfo()))
        ParentNode.insertChild(Node)
        if 'component' in Subtitle.keys():
            for Component in Subtitle['component']:
                (Key, Value), = Component.items()
                self._ParseVfrStatementStatTag(Key, Value, ParentNode)
                self.ParseVfrStatementStat(Key, Value, ParentNode)
                self.ParseVfrStatementQuestions(Key, Value, ParentNode)
        return Node

    def _ParseVfrStatementStatTag(self, Key, Value, ParentNode):
        if Key == 'image':
            self.ParseVfrStatementImage(Value, ParentNode)
        elif Key == 'locked':
            self.ParseVfrStatementLocked(Value, ParentNode)

    def _ParseSubtitleFlags(self, Subtitle):
        SubFlags = 0
        if 'flags' in Subtitle.keys():
            FlagList = self._ToList(Subtitle['flags'])
            for Flag in FlagList:
                if Flag == 'HORIZONTAL':
                    SubFlags |= 0x01
                else:
                    SubFlags |= Flag
        return SubFlags

    def ParseVfrStatementResetButton(self, ResetButton, ParentNode):
        Defaultstore = ResetButton['defaultstore']
        RBObj = IfrResetButton()
        DefaultId, ReturnCode = gVfrDefaultStore.GetDefaultId(Defaultstore)
        self.ErrorHandler(ReturnCode)
        RBObj.SetDefaultId(DefaultId)

        self._ParseVfrStatementHeader(RBObj, ResetButton)
        Node = VfrTreeNode(EFI_IFR_RESET_BUTTON_OP, RBObj, gFormPkg.StructToStream(RBObj.GetInfo()))
        ParentNode.insertChild(Node)
        if 'component' in ResetButton.keys():
            for Component in ResetButton['component']:
                (Key, Value), = Component.items()
                self._ParseVfrStatementStatTag(Key, Value, ParentNode)
        return Node

    def ParseVfrStatementGoto(self, Goto, ParentNode):
        R5Obj = IfrRef5()
        GObj = R5Obj

        if 'devicepath' in Goto.keys():
            R4Obj = IfrRef4()
            R4Obj.SetDevicePath(Goto['devicepath'])
            R4Obj.SetFormId(Goto['formid'])
            R4Obj.SetQId(Goto['questionid'])
            R4Obj.SetFormSetId(Goto['formsetguid'])
            GObj = R4Obj
        elif 'formsetguid' in Goto.keys():
            R3Obj = IfrRef3()
            R3Obj.SetFormId(Goto['formid'])
            R3Obj.SetQId(Goto['questionid'])
            R3Obj.SetFormSetId(Goto['formsetguid'])
            GObj = R3Obj
        elif 'formid' in Goto.keys():
            R2Obj = IfrRef2()
            R2Obj.SetFormId(Goto['formid'])
            R2Obj.SetQId(Goto['questionid'])
            GObj = R2Obj
        else:
            RObj = IfrRef()
            RObj.SetFormId(Goto['formid'])
            GObj = RObj

        self._ParseVfrQuestionHeader(Goto, EFI_QUESION_TYPE.QUESTION_REF)
        GObj.SetFlags(self._ParseStatementStatFlags(Goto))
        if 'key' in Goto.keys():
            self.AssignQuestionKey(GObj, Goto['key'])
        Node = VfrTreeNode(EFI_IFR_REF_OP, GObj)
        ParentNode.insertChild(Node)
        if 'component' in Goto.keys():
            GObj.SetScope(1)
            self._ParseVfrStatementQuestionOptionList(Goto['component'], Node)

        Node.Buffer = gFormPkg.StructToStream(GObj.GetInfo())

    def _ParseVfrStatementQuestionOptionList(self, ComponentList, ParentNode):
        for Component in ComponentList:
            (Key, Value), = Component.items()
            self._ParseVfrStatementQuestionTag(Key, Value, ParentNode)
            self._ParseVfrStatementQuestionOptionTag(Key, Value, ParentNode)

    def _ParseVfrStatementQuestionTag(self, Key, Value, ParentNode):
        if Key == 'image':
            self.ParseVfrStatementImage(Value, ParentNode)
        elif Key == 'locked':
            self.ParseVfrStatementLocked(Value, ParentNode)
        elif Key == 'refresh':
            self.ParseVfrStatementRefresh(Value, ParentNode)
        elif Key == 'varstoredevice':
            self.ParseVfrStatementVarstoreDevice(Value, ParentNode)
        elif Key == 'guidop':
            self.ParseVfrStatementExtension(Value, ParentNode)
        elif Key == 'refreshguid':
            self.ParseVfrStatementRefreshEvent(Value, ParentNode)
        elif Key == 'warningif':
            self.ParseVfrStatementWarningIf(Value, ParentNode)

    def ParseVfrStatementRefresh(self, Refresh, ParentNode):
        RObj = IfrRefresh()
        RObj.SetRefreshInterval(Refresh['interval'])
        Node = VfrTreeNode(EFI_IFR_REFRESH_OP, RObj, gFormPkg.StructToStream(RObj.GetInfo()))
        ParentNode.insertChild(Node)
        return Node

    def ParseVfrStatementVarstoreDevice(self, VarStoreDevice, ParentNode):
        VDObj = IfrVarStoreDevice()
        VDObj.SetDevicePath(VarStoreDevice['devicepath'])
        Node = VfrTreeNode(EFI_IFR_VARSTORE_DEVICE_OP, VDObj, gFormPkg.StructToStream(VDObj.GetInfo()))
        ParentNode.insertChild(Node)
        return Node

    def ParseVfrStatementWarningIf(self, WarningIf, ParentNode):
        WIObj = IfrWarningIf()
        WIObj.SetWarning(WarningIf['prompt'])
        if 'timeout' in WarningIf.keys():
            WIObj.SetTimeOut(WarningIf['timeout'])
        # Expression
        Node = VfrTreeNode(EFI_IFR_WARNING_IF_OP, WIObj, gFormPkg.StructToStream(WIObj.GetInfo()))
        ParentNode.insertChild(Node)
        return Node

    def _ParseVfrStatementQuestionOptionTag(self, Key, Value, ParentNode):
        if Key == 'value':
            self.ParseVfrStatementValue(Value, ParentNode)
        elif Key == 'default':
            self.ParseVfrStatementDefault(Value, ParentNode)
        elif Key == 'option':
            self.ParseVfrStatementOneofOption(Value, ParentNode)
        elif Key == 'read':
            self.ParseVfrStatementRead(Value, ParentNode)
        elif Key == 'write':
            self.ParseVfrStatementWrite(Value, ParentNode)

    def _ParseVfrConstantValueField(self, Value):
        pass


    def ParseVfrStatementOneofOption(self, Option, ParentNode):
        if self.CurrQestVarInfo.VarType == EFI_IFR_TYPE_OTHER:
            print("Get data type error.")
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_FATAL_ERROR, "Get data type error.")
        ValueList = self._ParseVfrConstantValueField(Option['value'])
        Value = ValueList[0]
        Type = self.CurrQestVarInfo.VarType
        if self.CurrentMinMaxData != None:
            #set min/max value for oneof opcode
            Step = self.CurrentMinMaxData.GetStepData()
            self.CurrentMinMaxData.SetMinMaxStepData(Value, Value, Step)
        if self.CurrQestVarInfo.IsBitVar:
            Type = EFI_IFR_TYPE_NUM_SIZE_32
        OOOObj = IfrOneOfOption(Type, ValueList)
        if len(ValueList) == 1:
            OOOObj.SetType(Type)
        else:
            OOOObj.SetType(EFI_IFR_TYPE_BUFFER)

        OOOObj.SetOption(Option['text'])
        HFlags, LFlags = self._ParseVfrStatementOneofOptionFlags(Option)
        self.ErrorHandler(OOOObj.SetFlags(LFlags))
        self.ErrorHandler(self.CurrentQuestion.SetQHeaderFlags(HFlags))

        # Array type only for default type OneOfOption.
        if (OOOObj.GetFlags() & (EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG)) == 0 and (len(ValueList) != 1):
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_FATAL_ERROR, "Default keyword should with array value type!")

        # Clear the default flag if the option not use array value but has default flag.
        if (OOOObj.GetFlags() & (EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG)) != 0 and (len(ValueList) == 1) and (self.IsOrderedList):
            OOOObj.SetFlags(OOOObj.GetFlags() & ~(EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG))

        if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            VarStoreName, ReturnCode = gVfrDataStorage.GetVarStoreName(self.CurrQestVarInfo.VarStoreId)
            self.ErrorHandler(ReturnCode)
            VarStoreGuid = gVfrDataStorage.GetVarStoreGuid(self.CurrQestVarInfo.VarStoreId)
            if OOOObj.GetFlags() & EFI_IFR_OPTION_DEFAULT:
                self._CheckDuplicateDefaultValue(EFI_HII_DEFAULT_CLASS_STANDARD)
                self.ErrorHandler(gVfrDefaultStore.BufferVarStoreAltConfigAdd(EFI_HII_DEFAULT_CLASS_STANDARD, self.CurrQestVarInfo, VarStoreName, VarStoreGuid, self.CurrQestVarInfo.VarType, Value))
            if OOOObj.GetFlags() & EFI_IFR_OPTION_DEFAULT_MFG:
                self._CheckDuplicateDefaultValue(EFI_HII_DEFAULT_CLASS_MANUFACTURING)
                self.ErrorHandler(gVfrDefaultStore.BufferVarStoreAltConfigAdd(EFI_HII_DEFAULT_CLASS_MANUFACTURING, self.CurrQestVarInfo, VarStoreName, VarStoreGuid, self.CurrQestVarInfo.VarType, Value))

        Node = VfrTreeNode(EFI_IFR_ONE_OF_OPTION_OP, OOOObj)
        ParentNode.insertChild(Node)

        if 'key' in Option.keys():
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED)
            OOOObj.SetIfrOptionKey(Option['key'])
            gIfrOptionKey = IfrOptionKey(self.CurrentQuestion.GetQuestionId(), Type, Value, Option['key'])
            ChildNode = VfrTreeNode(EFI_IFR_GUID_OP, gIfrOptionKey, gFormPkg.StructToStream(gIfrOptionKey.GetInfo()))
            Node.insertChild(ChildNode)

        if 'component' in Option.keys():
            OOOObj.SetScope(1)
            for Component in Option['component']:
                (Key, Value), = Component.items()
                if Key == 'image':
                    self.ParseVfrStatementImage(Value, Node)

        Node.Buffer = gFormPkg.StructToStream(OOOObj.GetInfo())
        return Node

    def _CheckDuplicateDefaultValue(self, DefaultId):
        for i in range(0, len(self.UsedDefaultArray)):
            if self.UsedDefaultArray[i] == DefaultId:
                gVfrErrorHandle.HandleWarning(EFI_VFR_WARNING_CODE.VFR_WARNING_DEFAULT_VALUE_REDEFINED)
        if len(self.UsedDefaultArray) >= EFI_IFR_MAX_DEFAULT_TYPE - 1:
            gVfrErrorHandle.HandleError(VfrReturnCode.VFR_RETURN_FATAL_ERROR)
        self.UsedDefaultArray.append(DefaultId)

    def _ParseVfrStatementOneofOptionFlags(self, Option):
        HFlags = 0
        LFlags = self.CurrQestVarInfo.VarType
        if 'flags' in Option.keys():
            FlagList = self._ToList(Option['flags'])
            for Flag in FlagList:
                if Flag == 'OPTION_DEFAULT':
                    LFlags |= 0x10
                elif Flag == 'OPTION_DEFAULT_MFG':
                    LFlags |= 0x20
                elif Flag == 'INTERACTIVE':
                    HFlags |= 0x04
                elif Flag == 'RESET_REQUIRED':
                    HFlags |= 0x10
                elif Flag == 'REST_STYLE':
                    HFlags |= 0x20
                elif Flag == 'RECONNECT_REQUIRED':
                    HFlags |= 0x40
                elif Flag == 'MANUFACTURING':
                    LFlags |= 0x20
                elif Flag == 'DEFAULT':
                    LFlags |= 0x10
                elif Flag == 'NV_ACCESS':
                    gVfrErrorHandle.HandleWarning (EFI_VFR_WARNING_CODE.VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE)
                elif Flag == 'LATE_CHECK':
                    gVfrErrorHandle.HandleWarning (EFI_VFR_WARNING_CODE.VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE)
        return HFlags, LFlags

    def ParseVfrStatementValue(self, Value, ParentNode):
        VObj = IfrValue()
        Node = VfrTreeNode(EFI_IFR_VALUE_OP, VObj, gFormPkg.StructToStream(VObj.GetInfo()))
        ParentNode.insertChild(Node)
        # expression
        return Node

    def ParseVfrStatementRead(self, Read, ParentNode):
        RObj = IfrRead()
        Node = VfrTreeNode(EFI_IFR_READ_OP, RObj, gFormPkg.StructToStream(RObj.GetInfo()))
        ParentNode.insertChild(Node)
        # expression
        return Node

    def ParseVfrStatementWrite(self, Write, ParentNode):
        WObj = IfrWrite()
        Node = VfrTreeNode(EFI_IFR_WRITE_OP, WObj, gFormPkg.StructToStream(WObj.GetInfo()))
        ParentNode.insertChild(Node)
        # expression
        return Node

    def _ParseStatementStatFlags(self, StatFlagsDict):
        Flags = 0
        if 'flags' in StatFlagsDict.keys():
            FlagList = self._ToList(StatFlagsDict['flags'])
            for Flag in FlagList:
                if Flag in QuestionheaderFlagsField:
                    Flags |= self._ParseQuestionheaderFlagsField(Flag)
                elif Flag != 0:
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED)
                else:
                    Flags |= Flag
        return Flags

    def _ParseVfrQuestionHeader(self, QObj, QDict, QType):
        self._ParseVfrQuestionBaseInfo(QObj, QDict, QType)
        self._ParseVfrStatementHeader(QObj, QDict)

    def _ParseVfrQuestionBaseInfo(self, QObj, QDict, QType):

        BaseInfo = EFI_VARSTORE_INFO()
        BaseInfo.VarType = EFI_IFR_TYPE_OTHER
        BaseInfo.VarTotalSize = 0
        BaseInfo.Info.VarOffset = EFI_VAROFFSET_INVALID
        BaseInfo.VarStoreId = EFI_VARSTORE_ID_INVALID
        BaseInfo.IsBitVar = False

        QName = None
        QId = EFI_QUESTION_ID_INVALID
        VarIdStr = ''

        ReturnCode = None
        if 'name' in QDict.keys():
            QName = QDict['name']
            ReturnCode = self.VfrQuestionDB.FindQuestionByName(QName)
            self.CompareErrorHandler(ReturnCode, VfrReturnCode.VFR_RETURN_UNDEFINED, QName, 'has already been used please used anther name')
        if 'varid' in QDict.keys():
            VarIdStr = self._VarIdToString(QDict['varid'])
        if 'questionid' in QDict.keys():
            QId = QDict['questionid']
            ReturnCode = self.VfrQuestionDB.FindQuestionById(QId)
            self.CompareErrorHandler(ReturnCode, VfrReturnCode.VFR_RETURN_UNDEFINED, QId, 'has already been used please used anther number')
        if  QType == EFI_QUESION_TYPE.QUESTION_NORMAL:
            #if self.IsCheckBoxOp:
                #BaseInfo.VarType = EFI_IFR_TYPE_BOOLEAN #
            QId, ReturnCode = self.VfrQuestionDB.RegisterQuestion(QName, VarIdStr, QId, gFormPkg)
            self.ErrorHandler(ReturnCode)

        elif QType == EFI_QUESION_TYPE.QUESTION_DATE:
            BaseInfo.VarType = EFI_IFR_TYPE_DATE
            QId, ReturnCode = self.VfrQuestionDB.RegisterNewDateQuestion(QName, VarIdStr, QId, gFormPkg)
            self.ErrorHandler(ReturnCode)

        elif QType == EFI_QUESION_TYPE.QUESTION_TIME:
            BaseInfo.VarType = EFI_IFR_TYPE_TIME
            QId, ReturnCode = self.VfrQuestionDB.RegisterNewTimeQuestion(QName, VarIdStr, QId, gFormPkg)
            self.ErrorHandler(ReturnCode)

        elif QType == EFI_QUESION_TYPE.QUESTION_REF:
            BaseInfo.VarType = EFI_IFR_TYPE_REF
            if VarIdStr != '': #stand for question with storage.
                QId, ReturnCode = self.VfrQuestionDB.RegisterRefQuestion(QName, VarIdStr, QId, gFormPkg)
                self.ErrorHandler(ReturnCode)
            else:
                QId, ReturnCode = self.VfrQuestionDB.RegisterQuestion(QName, None, QId, gFormPkg)
                self.ErrorHandler(ReturnCode)
        else:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_FATAL_ERROR)

        QObj.SetQuestionId(QId)
        if BaseInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            QObj.SetVarStoreInfo(BaseInfo)

        self.CurrQestVarInfo = BaseInfo

        return BaseInfo

    def _ParseVfrStatementHeader(QObj, QDict):
        QObj.SetPrompt(QDict['prompt'])
        QObj.SetHelp(QDict['help'])


    def _VarIdToString(self, VarId):
        pass


    def ParseVfrStatementQuestions(self, Key, Value, ParentNode):
        if Key == 'checkbox':
            self.ParseVfrStatementCheckBox(Value, ParentNode)
        if Key == 'action':
            self.ParseVfrStatementAction(Value, ParentNode)
        if Key == 'date':
            self.ParseVfrStatementDate(Value, ParentNode) #
        if Key == 'numeric':
            self.ParseVfrStatementNumeric(Value, ParentNode)
        if Key == 'oneof':
            self.ParseVfrStatementOneof(Value, ParentNode)
        if Key == 'string':
            self.ParseVfrStatementString(Value, ParentNode)
        if Key == 'password':
            self.ParseVfrStatementPassword(Value, ParentNode)
        if Key == 'orderedlist':
            self.ParseVfrStatementOrderedList(Value, ParentNode)
        if Key == 'time':
            self.ParseVfrStatementTime(Value, ParentNode)


    def ParseVfrStatementTime(self, Time, ParentNode):
        pass

    def _ParseTimeFlags(self, FlagsDict):
        LFlags = 0
        if 'flags' in FlagsDict.keys():
            FlagList = self._ToList(FlagsDict['flags'])
            for Flag in FlagList:
                if Flag == 'HOUR_SUPPRESS':
                    LFlags |= 0x01
                elif Flag == 'MINUTE_SUPPRESS':
                    LFlags |= 0x02
                elif Flag == 'SECOND_SUPPRESS':
                    LFlags |= 0x04
                elif Flag == 'STORAGE_NORMAL':
                    LFlags |= 0x00
                elif Flag == 'STORAGE_TIME':
                    LFlags |= 0x10
                elif Flag == 'STORAGE_WAKEUP':
                    LFlags |= 0x20
                else:
                    LFlags |= Flag
        return  LFlags

    def _ParseMinMaxTimeStepDefault(self, Time):
        pass



    def ParseVfrStatementOrderedList(self, OrderedList, ParentNode):
        OLObj = IfrOrderedList()
        self.CurrentQuestion = OLObj
        self.IsOrderedList = True
        self._ParseVfrQuestionHeader(OLObj, OrderedList, EFI_QUESION_TYPE.QUESTION_NORMAL)
        VarArraySize = self.GetCurArraySize()
        if VarArraySize > 0xFF:
            OLObj.SetMaxContainers(0xFF)
        else:
            OLObj.SetMaxContainers(VarArraySize)

        if 'maxcontainers' in OrderedList.keys():
            MaxContainers = OrderedList['maxcontainers']
            if MaxContainers > 0xFF:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, "OrderedList MaxContainers takes only one byte, which can't be larger than 0xFF.")
            elif VarArraySize != 0 and MaxContainers > VarArraySize:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, "OrderedList MaxContainers can't be larger than the max number of elements in array.")
            OLObj.SetMaxContainers(MaxContainers)

        self._ParseOrderedListFlags(OrderedList)
        Node = VfrTreeNode(EFI_IFR_ORDERED_LIST_OP, OLObj, gFormPkg.StructToStream(OLObj.GetInfo()))
        ParentNode.insertChild(Node)
        if 'component' in OrderedList.keys():
            self._ParseVfrStatementQuestionOptionList(OrderedList['component'], Node)
        self.IsOrderedList = False
        return Node

    def _ParseOrderedListFlags(self, FlagsDict):
        LFlags = 0
        HFlags = 0
        if 'flags' in FlagsDict.keys():
            FlagList = self._ToList(FlagsDict['flags'])
            for Flag in FlagList:
                if Flag in QuestionheaderFlagsField:
                    HFlags |= self._ParseQuestionheaderFlagsField(Flag)
                elif Flag == 'UNIQUE':
                    LFlags |= 0x01
                elif Flag == 'NOEMPTY':
                    LFlags |= 0x02
                elif Flag != 0:
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, 'CheckBox doesnot support flag!=0')
        return HFlags, LFlags


    def ParseVfrStatementCheckBox(self, CheckBox, ParentNode):
        CBObj = IfrCheckBox()
        self.CurrentQuestion = CBObj
        self.IsCheckBoxOp = True
        self._ParseVfrQuestionBaseInfo(CBObj, CheckBox, EFI_QUESION_TYPE.QUESTION_NORMAL)

        # Create a GUID opcode to wrap the checkbox opcode, if it refer to bit varstore.
        if self.CurrQestVarInfo.IsBitVar:
            GuidObj = IfrGuid(0)
            GuidObj.SetGuid(EDKII_IFR_BIT_VARSTORE_GUID)
            GuidObj.SetScope(1) # position
            GuidNode = VfrTreeNode(EFI_IFR_GUID_OP, GuidObj, gFormPkg.StructToStream(GuidObj.GetInfo()))
            ParentNode.insertChild(GuidNode)
            ParentNode = GuidNode

        self._ParseVfrStatementHeader(CBObj, CheckBox)
        # check dataType
        if self.CurrQestVarInfo.VarType == EFI_IFR_TYPE_OTHER:
            self.CurrQestVarInfo.VarType = EFI_IFR_TYPE_BOOLEAN

        if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            # Check whether the question refers to a bit field, if yes. create a Guid to indicate the question refers to a bit field.
            if self.CurrQestVarInfo.IsBitVar:
                _, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.CurrQestVarInfo.VarType)
                self.ErrorHandler(ReturnCode, "CheckBox varid is not the valid data type")
                if gVfrDataStorage.GetVarStoreType(self.CurrQestVarInfo.VarStoreId) == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER_BITS and self.CurrQestVarInfo.VarTotalSize != 1:
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, "CheckBox varid only occupy 1 bit in Bit Varstore")
                else:
                    Size, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.CurrQestVarInfo.VarType)
                    self.ErrorHandler(ReturnCode, "CheckBox varid is not the valid data type")
                    if Size != 0 and Size != self.CurrQestVarInfo.VarTotalSize:
                        self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,  "CheckBox varid doesn't support array")
                    elif gVfrDataStorage.GetVarStoreType(self.CurrQestVarInfo.VarStoreId) == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER and self.CurrQestVarInfo.VarTotalSize != sizeof(ctypes.c_bool):
                        self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, "CheckBox varid only support BOOLEAN data type")

        if 'flags' in CheckBox.keys():
            HFlags, LFlags = self._ParseCheckBoxFlags(CheckBox)
            CBObj.SetFlags(HFlags, LFlags)
            if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
                VarStoreName, ReturnCode = gVfrDataStorage.GetVarStoreName(self.CurrQestVarInfo.VarStoreId)
                #self.CompareErrorHandler(ReturnCode, VfrReturnCode.VFR_RETURN_SUCCESS, Line, ctx.L.text, "Failed to retrieve varstore name")

                VarStoreGuid = gVfrDataStorage.GetVarStoreGuid(self.CurrQestVarInfo.VarStoreId)
                self.Value = True
                if CBObj.GetFlags() & 0x01:
                    self._CheckDuplicateDefaultValue(EFI_HII_DEFAULT_CLASS_STANDARD)
                    ReturnCode = gVfrDefaultStore.BufferVarStoreAltConfigAdd(EFI_HII_DEFAULT_CLASS_STANDARD,self.CurrQestVarInfo, VarStoreName, VarStoreGuid, self.CurrQestVarInfo.VarType, self.Value)
                    #self.CompareErrorHandler(ReturnCode, VfrReturnCode.VFR_RETURN_SUCCESS, Line, ctx.L.text, "No standard default storage found")
                if CBObj.GetFlags() & 0x02:
                    self._CheckDuplicateDefaultValue(EFI_HII_DEFAULT_CLASS_MANUFACTURING)
                    ReturnCode =  gVfrDefaultStore.BufferVarStoreAltConfigAdd(EFI_HII_DEFAULT_CLASS_MANUFACTURING, self.CurrQestVarInfo, VarStoreName, VarStoreGuid, self.CurrQestVarInfo.VarType, self.Value)
                    #self.CompareErrorHandler(ReturnCode, VfrReturnCode.VFR_RETURN_SUCCESS, Line, ctx.L.text, "No manufacturing default storage found")

        if 'key' in CheckBox.keys():
            self.AssignQuestionKey(CBObj, CheckBox['key'])

        Node = VfrTreeNode(EFI_IFR_CHECKBOX_OP, CBObj, gFormPkg.StructToStream(CBObj.GetInfo()))
        ParentNode.insertChild(Node)
        if 'component' in CheckBox.keys():
            self._ParseVfrStatementQuestionOptionList(CheckBox['component'], Node)
        self.IsCheckBoxOp = False
        return Node

    def _ParseCheckBoxFlags(self, FlagsDict):
        LFlags = 0
        HFlags = 0
        if 'flags' in FlagsDict.keys():
            FlagList = self._ToList(FlagsDict['flags'])
            for Flag in FlagList:
                if Flag in QuestionheaderFlagsField:
                    HFlags |= self._ParseQuestionheaderFlagsField(Flag)
                elif Flag == 'DEFAULT':
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, 'CheckBox doesnot support \'DEFAULT\' flag')
                elif Flag == 'MANUFACTURING':
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, 'CheckBox doesnot support \'MANUFACTURING\' flag')
                elif Flag == 'CHECKBOX_DEFAULT':
                    LFlags |= 0x01
                elif Flag == 'CHECKBOX_DEFAULT_MFG':
                    LFlags |= 0x02
                elif Flag != 0:
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED, 'CheckBox doesnot support flag!=0')
        return HFlags, LFlags

    def ParseVfrStatementAction(self, Action, ParentNode):
        AObj = IfrAction()
        Config = Action['config']
        AObj.SetQuestionConfig(Config)
        # No handler for Flags
        Node = VfrTreeNode(EFI_IFR_ACTION_OP, AObj, gFormPkg.StructToStream(AObj.GetInfo()))
        ParentNode.insertChild(Node)
        if 'component' in Action.keys():
            for Component in Action['component']:
                (Key, Value), = Component.items()
                self._ParseVfrStatementQuestionTag(Key, Value, Node)
        return Node

    def ParseVfrStatementDate(self, Date, ParentNode):
        DObj = IfrDate()
        self._ParseVfrQuestionHeader(DObj, Date, EFI_QUESION_TYPE.QUESTION_DATE)
        self.ErrorHandler(DObj.SetFlags(EFI_IFR_QUESTION_FLAG_DEFAULT, self._ParseDateFlags(Date)))
        #pending

    def _ParseDateFlags(self, FlagsDict):
        LFlags = 0
        if 'flags' in FlagsDict.keys():
            FlagList = self._ToList(FlagsDict['flags'])
            for Flag in FlagList:
                if Flag == 'YEAR_SUPPRESS':
                    LFlags |= 0x01
                elif Flag == 'MONTH_SUPPRESS':
                    LFlags |= 0x02
                elif Flag == 'DAY_SUPPRESS':
                    LFlags |= 0x04
                elif Flag == 'STORAGE_NORMAL':
                    LFlags |= 0x00
                elif Flag == 'STORAGE_TIME':
                    LFlags |= 0x010
                elif Flag == 'STORAGE_WAKEUP':
                    LFlags |= 0x020
        return LFlags

    def ParseVfrStatementNumeric(self, Numeric, ParentNode):
        NObj = IfrNumeric(EFI_IFR_TYPE_NUM_SIZE_64)
        Node = VfrTreeNode(EFI_IFR_NUMERIC_OP)
        self.CurrentQuestion = Node.Data
        self.CurrentMinMaxData = Node.Data
        UpdateVarType = False
        self._ParseVfrQuestionBaseInfo(NObj, Numeric, EFI_QUESION_TYPE.QUESTION_NORMAL)

        # Create a GUID opcode to wrap the numeric opcode, if it refer to bit varstore.
        if self.CurrQestVarInfo.IsBitVar:
            GuidObj = IfrGuid(0)
            GuidObj.SetGuid(EDKII_IFR_BIT_VARSTORE_GUID)
            GuidObj.SetScope(1) # pos
            GuidNode = VfrTreeNode(EFI_IFR_GUID_OP, GuidObj, gFormPkg.StructToStream(GuidObj.GetInfo()))
            ParentNode.insertChild(GuidNode)
            ParentNode = GuidNode

        self._ParseVfrStatementHeader(NObj, Numeric)
        # check data type
        if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            if self.CurrQestVarInfo.IsBitVar:
                LFlags = EDKII_IFR_NUMERIC_SIZE_BIT & self.CurrQestVarInfo.VarTotalSize
                self.ErrorHandler(NObj.SetFlagsForBitField(NObj.GetQFlags(), LFlags))
            else:
                DataTypeSize, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.CurrQestVarInfo.VarType)
                self.ErrorHandler(ReturnCode, 'Numeric varid is not the valid data type')
                if DataTypeSize != 0 and DataTypeSize != self.CurrQestVarInfo.VarTotalSize:
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, 'Numeric varid doesn\'t support array')
                self.ErrorHandler(NObj.SetFlags(NObj.GetQFlags(), self.CurrQestVarInfo.VarType))

        if 'flags' in Numeric.keys():
            HFlags, LFlags, IsDisplaySpecified, UpdateVarType = self._ParseNumericFlags(Numeric)
            if self.CurrQestVarInfo.IsBitVar:
                self.ErrorHandler(NObj.SetFlagsForBitField(HFlags,LFlags, IsDisplaySpecified))
            else:
                self.ErrorHandler(NObj.SetFlags(HFlags, LFlags, IsDisplaySpecified))

        if 'key' in Numeric.keys():
            self.AssignQuestionKey(NObj, Numeric['key'])

        if self.CurrQestVarInfo.IsBitVar == False:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, 'Numeric question only support UINT8, UINT16, UINT32 and UINT64 data type.')

        # modify the data for namevalue
        if UpdateVarType:
            UpdatedNObj = IfrNumeric(self.CurrQestVarInfo.VarType)
            UpdatedNObj.GetInfo().Question = NObj.GetInfo().Question
            UpdatedNObj.GetInfo().Flags = NObj.GetInfo().Flags
            NObj = UpdatedNObj

        self._ParseVfrSetMinMaxStep(NObj, Numeric)

        Node.Data = NObj
        Node.Buffer = gFormPkg.StructToStream(NObj.GetInfo())
        ParentNode.insertChild(Node)
        if 'component' in Numeric.keys():
            self._ParseVfrStatementQuestionOptionList(Numeric['component'], Node)
        return Node

    def _ParseNumericFlags(self, FlagsDict):
        LFlags = 0
        VarStoreType = gVfrDataStorage.GetVarStoreType(self.CurrQestVarInfo.VarStoreId)
        HFlags = 0
        IsSetType = False
        IsDisplaySpecified = False
        UpdateVarType = False
        if 'flags' in FlagsDict.keys():
            LFlags = self.CurrQestVarInfo.VarType & EFI_IFR_NUMERIC_SIZE
            FlagList = self._ToList(FlagsDict['flags'])
            for Flag in FlagList:
                if Flag == 'NUMERIC_SIZE_1':
                    if self.CurrQestVarInfo.IsBitVar == False:
                        LFlags =  (LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_1
                        IsSetType = True
                    else:
                        self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, 'Can not specify the size of the numeric value for BIT field')

                elif Flag == 'NUMERIC_SIZE_2':
                    if self.CurrQestVarInfo.IsBitVar == False:
                        LFlags =  (LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_2
                        IsSetType = True
                    else:
                        self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, 'Can not specify the size of the numeric value for BIT field')

                elif Flag == 'NUMERIC_SIZE_4':
                    if self.CurrQestVarInfo.IsBitVar == False:
                        LFlags =  (LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_4
                        IsSetType = True
                    else:
                        self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, 'Can not specify the size of the numeric value for BIT field')

                elif Flag == 'NUMERIC_SIZE_8':
                    if self.CurrQestVarInfo.IsBitVar == False:
                        LFlags =  (LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_8
                        IsSetType = True
                    else:
                        self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, 'Can not specify the size of the numeric value for BIT field')

                elif Flag == 'DISPLAY_INT_DEC':
                    if self.CurrQestVarInfo.IsBitVar == False:
                        LFlags =  (LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_INT_DEC
                    else:
                        LFlags =  (LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_INT_DEC_BIT
                    IsDisplaySpecified = True

                elif Flag == 'DISPLAY_UINT_DEC':
                    if self.CurrQestVarInfo.IsBitVar == False:
                        LFlags =  (LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_UINT_DEC
                    else:
                        LFlags =  (LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_UINT_DEC_BIT
                    IsDisplaySpecified = True

                elif Flag == 'DISPLAY_UINT_HEX':
                    if self.CurrQestVarInfo.IsBitVar == False:
                        LFlags =  (LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_UINT_HEX
                    else:
                        LFlags =  (LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_UINT_HEX_BIT
                    IsDisplaySpecified = True

                elif Flag in QuestionheaderFlagsField:
                    HFlags |= self._ParseQuestionheaderFlagsField(Flag)

                elif Flag != 0:
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED)

        VarType = self.CurrQestVarInfo.VarType
        if self.CurrQestVarInfo.IsBitVar == False:
            if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
                if VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER or VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_EFI:
                    if self.CurrQestVarInfo.VarType != (LFlags & EFI_IFR_NUMERIC_SIZE):
                        self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, 'Numeric Flag is not same to Numeric VarData type')
                else:
                    # update data type for name/value store
                    self.CurrQestVarInfo.VarType = LFlags & EFI_IFR_NUMERIC_SIZE
                    Size, _ = gVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.CurrQestVarInfo.VarType)
                    self.CurrQestVarInfo.VarTotalSize = Size
            elif IsSetType:
                self.CurrQestVarInfo.VarType = LFlags & EFI_IFR_NUMERIC_SIZE

        elif self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID and self.CurrQestVarInfo.IsBitVar:
            LFlags &= EDKII_IFR_DISPLAY_BIT
            LFlags |= EDKII_IFR_NUMERIC_SIZE_BIT & self.CurrQestVarInfo.VarTotalSize

        if VarType != self.CurrQestVarInfo.VarType:
                    UpdateVarType = True

        return HFlags, LFlags, IsDisplaySpecified, UpdateVarType

    def _ParseVfrSetMinMaxStep(self, OpObj, QDict):
        IntDecStyle = False
        if ((self.CurrQestVarInfo.IsBitVar) and (OpObj.GetOpCode() == EFI_IFR_NUMERIC_OP) and ((OpObj.GetNumericFlags() & EDKII_IFR_DISPLAY_BIT) == 0)) or \
            ((self.CurrQestVarInfo.IsBitVar == False) and (OpObj.GetOpCode() == EFI_IFR_NUMERIC_OP) and ((OpObj.GetNumericFlags() & EFI_IFR_DISPLAY) == 0)):
            IntDecStyle = True
        MinNegative = False
        MaxNegative = False

        Min = QDict['minimum']
        Max = QDict['maximum']
        Step = QDict['step'] if 'step' in QDict.keys() else 0
        # wip
        OpObj.SetMinMaxStepData(Min, Max, Step)

    def ParseVfrStatementOneof(self, OneOf, ParentNode):
        OObj = IfrOneOf(EFI_IFR_TYPE_NUM_SIZE_64)
        Node = VfrTreeNode(EFI_IFR_ONE_OF_OP)
        self.CurrentQuestion = Node.Data
        self.CurrentMinMaxData = Node.Data
        UpdateVarType = False
        self._ParseVfrQuestionBaseInfo(OObj, OneOf, EFI_QUESION_TYPE.QUESTION_NORMAL)

        # Create a GUID opcode to wrap the numeric opcode, if it refer to bit varstore.
        if self.CurrQestVarInfo.IsBitVar:
            GuidObj = IfrGuid(0)
            GuidObj.SetGuid(EDKII_IFR_BIT_VARSTORE_GUID)
            GuidObj.SetScope(1) # pos
            GuidNode = VfrTreeNode(EFI_IFR_GUID_OP, GuidObj, gFormPkg.StructToStream(GuidObj.GetInfo()))
            ParentNode.insertChild(GuidNode)
            ParentNode = GuidNode

        self._ParseVfrStatementHeader(OObj, OneOf)

        # check data type
        if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            if self.CurrQestVarInfo.IsBitVar:
                LFlags = EDKII_IFR_NUMERIC_SIZE_BIT & self.CurrQestVarInfo.VarTotalSize
                self.ErrorHandler(OObj.SetFlagsForBitField(OObj.GetQFlags(),LFlags))
            else:
                DataTypeSize, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.CurrQestVarInfo.VarType)
                self.ErrorHandler(ReturnCode, 'OneOf varid is not the valid data type')
                if DataTypeSize != 0 and DataTypeSize != self.CurrQestVarInfo.VarTotalSize:
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, 'OneOf varid doesn\'t support array')
                self.ErrorHandler(OObj.SetFlags(OObj.GetQFlags(), self.CurrQestVarInfo.VarType))

        if 'flags' in OneOf.keys():
            HFlags, LFlags, _, UpdateVarType = self._ParseNumericFlags(OneOf)
            if self.CurrQestVarInfo.IsBitVar:
                self.ErrorHandler(OObj.SetFlagsForBitField(HFlags, LFlags))
            else:
                self.ErrorHandler(OObj.SetFlags(HFlags, LFlags))

        if (self.CurrQestVarInfo.IsBitVar == False) and (self.CurrQestVarInfo.VarType not in BasicTypes):
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, 'OneOf question only support UINT8, UINT16, UINT32 and UINT64 data type.')

        # modify the data Vartype for NameValue
        if UpdateVarType:
            UpdatedOObj = IfrOneOf(self.CurrQestVarInfo.VarType)
            UpdatedOObj.GetInfo().Question = OObj.GetInfo().Question
            UpdatedOObj.GetInfo().Flags = OObj.GetInfo().Flags
            OObj = UpdatedOObj


        self._ParseVfrSetMinMaxStep(OObj, OneOf)
        Node.Data = OObj
        Node.Buffer = gFormPkg.StructToStream(OObj.GetInfo())
        ParentNode.insertChild(Node)
        if 'component' in OneOf.keys():
            self._ParseVfrStatementQuestionOptionList(OneOf['component'], Node)
        return Node

    def ParseVfrStatementString(self, Str, ParentNode):
        self.IsStringOp = True
        SObj = IfrString()
        self.CurrentQuestion = SObj
        self._ParseVfrQuestionHeader(SObj, Str, EFI_QUESION_TYPE.QUESTION_NORMAL)
        HFlags, LFlags = self._ParseStringFlags(Str)
        self.ErrorHandler(SObj.SetFlags(HFlags, LFlags))
        if 'key' in Str.keys():
            self.AssignQuestionKey(SObj, Str['key'])

        StringMinSize = Str['minsize']
        StringMaxSize = Str['maxsize']
        VarArraySize = self.GetCurArraySize()
        if StringMinSize > 0xFF:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, "String MinSize takes only one byte, which can't be larger than 0xFF.")
        if VarArraySize != 0 and StringMinSize > VarArraySize:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, "String MinSize can't be larger than the max number of elements in string array.")
        SObj.SetMinSize(StringMinSize)

        if StringMaxSize > 0xFF:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, "String MaxSize takes only one byte, which can't be larger than 0xFF.")
        elif VarArraySize != 0 and StringMaxSize > VarArraySize:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, "String MaxSize can't be larger than the max number of elements in string array.")
        elif StringMaxSize < StringMinSize:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, "String MaxSize can't be less than String MinSize.")
        SObj.SetMaxSize(StringMaxSize)
        Node = VfrTreeNode(EFI_IFR_STRING_OP, SObj, gFormPkg.StructToStream(SObj.GetInfo()))
        ParentNode.insertChild(Node)
        if 'component' in Str.keys():
            self._ParseVfrStatementQuestionOptionList(Str['component'], Node)

        self.IsStringOp = False


    def _ParseStringFlags(self, FlagsDict):
        HFlags = 0
        LFlags = 0
        if 'flags' in FlagsDict.keys():
            FlagList = self._ToList(FlagsDict['flags'])
            for Flag in FlagList:
                if Flag in QuestionheaderFlagsField:
                    HFlags |= self._ParseQuestionheaderFlagsField(Flag)
                elif Flag == 'MULTI_LINE':
                    LFlags |= 0x01
                elif Flag != 0:
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED)
        return HFlags, LFlags


    def ParseVfrStatementPassword(self, Password, ParentNode):
        PObj = IfrPassword()
        self.CurrentQuestion = PObj

        self.ErrorHandler(PObj.SetFlags(self._ParseStatementStatFlags(Password)))

        if 'key' in Password.keys():
            self.AssignQuestionKey(PObj, Password['key'])

        PassWordMinSize = Password['minsize']
        PasswordMaxSize = Password['maxsize']
        VarArraySize = self.GetCurArraySize()
        if PassWordMinSize > 0xFF:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, "String MinSize takes only one byte, which can't be larger than 0xFF.")
        if VarArraySize != 0 and PassWordMinSize > VarArraySize:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, "String MinSize can't be larger than the max number of elements in string array.")
        PObj.SetMinSize(PassWordMinSize)
        if PasswordMaxSize > 0xFF:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, "String MaxSize takes only one byte, which can't be larger than 0xFF.")
        elif VarArraySize != 0 and PasswordMaxSize > VarArraySize:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, "String MaxSize can't be larger than the max number of elements in string array.")
        elif PasswordMaxSize < PassWordMinSize:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_INVALID_PARAMETER, "String MaxSize can't be less than String MinSize.")
        PObj.SetMaxSize(PasswordMaxSize)
        Node = VfrTreeNode(EFI_IFR_PASSWORD_OP, PObj, gFormPkg.StructToStream(PObj.GetInfo()))
        ParentNode.insertChild(Node)
        if 'component' in Password.keys():
            self._ParseVfrStatementQuestionOptionList(Password['component'], Node)
        return Node

    def ParseVfrStatementImage(self, Image, ParentNode):
        IObj = IfrImage()
        ImageId = Image['id']
        IObj.SetImageId(ImageId)
        Node = VfrTreeNode(EFI_IFR_IMAGE_OP, IObj, gFormPkg.StructToStream(IObj.GetInfo()))
        ParentNode.insertChild(Node)
        return Node


    def ParseVfrStatementLocked(self, Image, ParentNode):
        LObj = IfrLocked()
        Node = VfrTreeNode(EFI_IFR_LOCKED_OP, LObj, gFormPkg.StructToStream(LObj.GetInfo()))
        ParentNode.insertChild(Node)
        return Node

    def ParseVfrStatementLabel(self, Label, ParentNode):
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

    def ParseVfrStatementStaticText(self, Text, ParentNode):
        Help = Text['help']
        Prompt = Text['prompt']

        QId = EFI_QUESTION_ID_INVALID
        TxtTwo = EFI_STRING_ID_INVALID
        if 'text' in Text.keys():
            TxtTwo = Text['text']

        TextFlags = self._ParseStatementStatFlags(Text)

        if TextFlags & EFI_IFR_FLAG_CALLBACK:
            if TxtTwo != EFI_STRING_ID_INVALID:
                gVfrErrorHandle.HandleWarning(EFI_VFR_WARNING_CODE.VFR_WARNING_ACTION_WITH_TEXT_TWO)
            AObj = IfrAction()
            QId, _ = self.VfrQuestionDB.RegisterQuestion(None, None, QId, gFormPkg)
            AObj.SetQuestionId(QId)
            AObj.SetHelp(Help)
            AObj.SetPrompt(Prompt)
            self.ErrorHandler(AObj.SetFlags(TextFlags))
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

    def ErrorHandler(self, ReturnCode, TokenValue=None, LineNum=None):
        self.ParserStatus += gVfrErrorHandle.HandleError(ReturnCode, LineNum, TokenValue)

    def CompareErrorHandler(self, ReturnCode, ExpectedCode, TokenValue=None, ErrorMsg=None, LineNum=None):
        if ReturnCode != ExpectedCode:
            self.ParserStatus += 1
            gVfrErrorHandle.PrintMsg(LineNum, 'Error', ErrorMsg, TokenValue)