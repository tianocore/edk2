from VfrCompiler.SourceVfrSyntaxVisitor import *
from VfrCompiler.SourceVfrSyntaxLexer import *
from VfrCompiler.SourceVfrSyntaxParser import *
from VfrCompiler.IfrFormPkg import *
from VfrCompiler.IfrError import *
from VfrCompiler.IfrPreProcess import *
import re
import copy


class YamlTree:
    def __init__(self, Root: IfrTreeNode, PreProcessDB: PreProcessDB, Options: Options):
        self.Options = Options
        self.Root = Root
        self.PreProcessDB = PreProcessDB
        self.IfrTree = IfrTree(Root, PreProcessDB, Options)
        self.Modifier = DLTParser(Root)
        self.YamlDict = None
        self.Lines = []
        self.Visitor = None
        self.Parser = None
        self.GuidID = None

    # transform string token and Header key-values to specific values for yaml
    def PreProcess(self):
        try:
            FileName = self.Options.YamlFileName
            File = open(FileName, "r")
            # self.YamlDict = yaml.load(f.read(), Loader=yaml.FullLoader)
            self.YamlDict = yaml.safe_load(File)
            File.seek(0)
            # self.Lines = File.readlines()
            self.YamlDictDisplay = yaml.safe_load(File)
            File.close()
        except:
            EdkLogger.error(
                "VfrCompiler",
                FILE_READ_FAILURE,
                "File read failed for %s" % FileName,
                None,
            )
        # Get Yaml GuidId
        self.GuidID = self.YamlDict["formset"]["guid"]
        if "defines" in self.YamlDict.keys():
            self.PreProcessDB.VfrDict = copy.deepcopy(self.YamlDict["defines"])
        # Tranfer Strings into Specfic Values by PreprocessDB
        self.PreProcessYamlDict(self.YamlDict)
        self.PreProcessDisplayDict(self.YamlDictDisplay)
        try:
            FileName = self.Options.ProcessedYAMLFileName
            with open(FileName, "w") as f:
                f.write(
                    yaml.dump(
                        self.YamlDictDisplay,
                        width=float("inf"),
                        allow_unicode=True,
                        default_flow_style=False,
                    )
                )
        except:
            EdkLogger.error(
                "VfrCompiler",
                FILE_CREATE_FAILURE,
                "File create failed for %s" % FileName,
                None,
            )

    def Compile(self):
        gVfrVarDataTypeDB.Clear()
        gVfrDefaultStore.Clear()
        gVfrDataStorage.Clear()
        gFormPkg.Clear()
        gIfrFormId.Clear()
        self.Parser = YamlParser(self.Root, self.Options, self.PreProcessDB, self.YamlDict, self.GuidID)
        self.Parser.Parse()

    def GenBinaryFiles(self):
        self.IfrTree.GenBinaryFiles()
        self.IfrTree.DumpCompiledYaml()
        self.IfrTree.GenRecordListFile()

    def PreProcessDisplayDict(self, YamlDict):
        for Key in YamlDict.keys():
            if Key == "condition":
                continue
            Value = YamlDict[Key]
            if isinstance(Value, list):  # value is list
                for Item in Value:
                    if isinstance(Item, dict):
                        self.PreProcessDisplayDict(Item)

            elif isinstance(Value, dict):  # value is dict
                self.PreProcessDisplayDict(Value)

            elif isinstance(Value, str):  # value is str
                # get the specific value of string token
                Match = re.search(r"STRING_TOKEN\((.*?)\)", Value)
                if Match:
                    if self.PreProcessDB.UniDict and Match.group(1) in self.PreProcessDB.UniDict.keys():
                        YamlDict[Key] = self.PreProcessDB.DisplayValue(self.PreProcessDB.UniDict[Match.group(1)], True)
                        # YamlDict[Key] = 'STRING_TOKEN' + '(' +  self.PreProcessDB.UniDict[Value[Start:End]] + ')'
                else:
                    if self.PreProcessDB.UniDict and Value in self.PreProcessDB.UniDict.keys():
                        YamlDict[Key] = self.PreProcessDB.DisplayValue(self.PreProcessDB.UniDict[Value])
                    # get {key:value} dicts from header files
                    elif self.PreProcessDB.HeaderDict and Value in self.PreProcessDB.HeaderDict.keys():
                        YamlDict[Key] = self.PreProcessDB.DisplayValue(self.PreProcessDB.HeaderDict[Value])

                    elif self.PreProcessDB.VfrDict and Value in self.PreProcessDB.VfrDict.keys():
                        YamlDict[Key] = self.PreProcessDB.DisplayValue(self.PreProcessDB.VfrDict[Value])
                    elif "|" in Value:
                        Items = Value.split("|")
                        ValueList = []
                        for i in range(0, len(Items)):
                            # get {key:value} dicts from header files
                            if self.PreProcessDB.HeaderDict and Items[i].strip() in self.PreProcessDB.HeaderDict.keys():
                                ValueList.append(self.PreProcessDB.DisplayValue(self.PreProcessDB.HeaderDict[Items[i].strip()]))
                            elif self.PreProcessDB.VfrDict and Items[i].strip() in self.PreProcessDB.VfrDict.keys():
                                ValueList.append(self.PreProcessDB.DisplayValue(self.PreProcessDB.VfrDict[Items[i].strip()]))
                            elif self.PreProcessDB.UniDict and Items[i].strip() in self.PreProcessDB.UniDict.keys():
                                ValueList.append(self.PreProcessDB.DisplayValue(self.PreProcessDB.UniDict[Items[i].strip()]))
                            elif (Items[i].strip().startswith("0x")) or (Items[i].strip().startswith("0X")) or (Items[i].strip().isdigit()):
                                ValueList.append(self.PreProcessDB.DisplayValue(Items[i].strip()))
                            else:
                                ValueList.append(Items[i].strip())

                        YamlDict[Key] = ValueList

    def ConsumeDLT(self):
        self.Options.DeltaFileName = self.Options.OutputDirectory + "Vfr.dlt"  #
        FileLines = open(self.Options.DeltaFileName, "r")
        LastPos = None
        CurPos = None
        # index is used to mark condition numbers
        Dict = {"index": 0}
        QuestionNode = None
        for Line in FileLines:
            Parts = Line.split(" | ")
            CurPos = ".".join(Parts[0].split(".")[:-1])
            # Unify Position Guid Format
            re.sub(r"0x[0-9a-fA-F]+|\d+", lambda x: str(int(x.group(), 0)), CurPos)
            Key = Parts[0].split(".")[-1].lower()
            Value = Parts[1].replace("\n", "").strip()
            # tranfer str to int
            if Value.isdigit() or Value.startswith("0x") or Value.startswith("0X"):
                Value = int(Value, 0)

            if CurPos == LastPos:
                # add {key, Value} to Dict for Position/LastPos Question
                if Key == "condition":
                    Key = "condition1"

                if Key.startswith("condition"):
                    Dict["index"] += 1

                Dict[Key] = Value
                continue
            else:
                # LastPos Question ends,
                # do insert/delete/modify operation for last Position Question
                QuestionNode = self.Modifier.FindQuestionNode(self.Root, LastPos)
                if QuestionNode == None:
                    self.Modifier.AddQuestionNode(LastPos, Dict, self.Parser)
                elif "condition" in Dict.keys() and Dict["condition"] == "suppressif true":
                    self.Modifier.DeleteQuestionNode(QuestionNode)
                else:
                    self.Modifier.ModifyQuestionNode(QuestionNode, LastPos, Dict, self.Parser)

                # clear Dict
                Dict = {"index": 0}
                Dict[Key] = Value
                LastPos = CurPos

        FileLines.close()
        # for Question in DeltaDict['questions']:
        #     (QuestionType, QuestionDict), = Question.items()
        #     QuestionDict['Position'] = re.sub(r'0x[0-9a-fA-F]+|\d+', lambda x: str(int(x.group(), 0)), QuestionDict['Position'])
        #     QuestionNode = self.Modifier.FindQuestionNode(self.Root, QuestionDict['Position'])
        #     if QuestionNode == None:
        #         self.Modifier.AddQuestionNode(QuestionType, QuestionDict, self.Parser)
        #     else:
        #         if 'condition' in QuestionDict.keys() and QuestionDict['condition'] == 'suppressif true':
        #             self.Modifier.DeleteQuestionNode(QuestionNode)
        #         else:
        #             self.Modifier.ModifyQuestionNode(QuestionType, QuestionDict, QuestionNode, self.Parser)
        # # self.IfrTree.DumpProcessedYaml()

    def PreProcessYamlDict(self, YamlDict):
        for Key in YamlDict.keys():
            if Key == "condition":
                continue
            Value = YamlDict[Key]
            if isinstance(Value, list):  # value is list
                for Item in Value:
                    if isinstance(Item, dict):
                        self.PreProcessYamlDict(Item)

            elif isinstance(Value, dict):  # value is dict
                self.PreProcessYamlDict(Value)

            elif isinstance(Value, str):  # value is str
                # get the specific value of string token
                Match = re.search(r"STRING_TOKEN\((.*?)\)", Value)
                if Match:
                    if self.PreProcessDB.UniDict and Match.group(1) in self.PreProcessDB.UniDict.keys():
                        YamlDict[Key] = ValueDB(
                            Value,
                            self.PreProcessDB.TransValue(self.PreProcessDB.UniDict[Match.group(1)]),
                        )
                        # YamlDict[Key] = 'STRING_TOKEN' + '(' +  self.PreProcessDB.UniDict[Value[Start:End]] + ')'
                else:
                    if self.PreProcessDB.UniDict and Value in self.PreProcessDB.UniDict.keys():
                        YamlDict[Key] = ValueDB(
                            Value,
                            self.PreProcessDB.TransValue(self.PreProcessDB.UniDict[Value]),
                        )
                    # get {key:value} dicts from header files
                    elif self.PreProcessDB.HeaderDict and Value in self.PreProcessDB.HeaderDict.keys():
                        if Key == "formid":
                            YamlDict[Key] = ValueDB(
                                Value,
                                [
                                    Value,
                                    self.PreProcessDB.TransValue(self.PreProcessDB.HeaderDict[Value]),
                                ],
                            )
                        else:
                            YamlDict[Key] = ValueDB(
                                Value,
                                self.PreProcessDB.TransValue(self.PreProcessDB.HeaderDict[Value]),
                            )

                    elif self.PreProcessDB.VfrDict and Value in self.PreProcessDB.VfrDict.keys():
                        YamlDict[Key] = ValueDB(
                            Value,
                            self.PreProcessDB.TransValue(self.PreProcessDB.VfrDict[Value]),
                        )
                    elif "|" in Value:
                        Items = Value.split("|")
                        ValueList = []
                        for i in range(0, len(Items)):
                            # get {key:value} dicts from header files
                            if self.PreProcessDB.HeaderDict and Items[i].strip() in self.PreProcessDB.HeaderDict.keys():
                                ValueList.append(self.PreProcessDB.TransValue(self.PreProcessDB.HeaderDict[Items[i].strip()]))
                            elif self.PreProcessDB.VfrDict and Items[i].strip() in self.PreProcessDB.VfrDict.keys():
                                ValueList.append(self.PreProcessDB.TransValue(self.PreProcessDB.VfrDict[Items[i].strip()]))
                            elif self.PreProcessDB.UniDict and Items[i].strip() in self.PreProcessDB.UniDict.keys():
                                ValueList.append(self.PreProcessDB.TransValue(self.PreProcessDB.UniDict[Items[i].strip()]))
                            elif (Items[i].strip().startswith("0x")) or (Items[i].strip().startswith("0X")) or (Items[i].strip().isdigit()):
                                ValueList.append(self.PreProcessDB.TransValue(Items[i].strip()))
                            else:
                                ValueList.append(Items[i].strip())

                        YamlDict[Key] = ValueDB(Value, ValueList)
                    else:
                        YamlDict[Key] = ValueDB(Value, Value)
            else:
                YamlDict[Key] = ValueDB(Value, Value)

        return


QuestionheaderFlagsField = [
    "READ_ONLY",
    "INTERACTIVE",
    "RESET_REQUIRED",
    "REST_STYLE",
    "RECONNECT_REQUIRED",
    "OPTIONS_ONLY",
    "NV_ACCESS",
    "LATE_CHECK",
]
vfrStatementStat = ["subtitle", "text", "goto", "resetbutton"]
vfrStatementQuestions = [
    "checkbox",
    "action",
    "date",
    "numeric",
    "oneof",
    "string",
    "password",
    "orderedlist",
    "time",
]
vfrStatementConditional = ["disableif", "suppressif", "grayoutif", "inconsistentif"]


class YamlParser:
    def __init__(
        self,
        Root: IfrTreeNode,
        Options: Options,
        PreProcessDB: PreProcessDB,
        YamlDict,
        GuidID,
    ):
        self.Root = Root
        self.Options = Options
        self.PreProcessDB = PreProcessDB
        self.YamlDict = YamlDict
        self.GuidID = GuidID
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
        self.ParseVfrHeader()
        self.ParseVfrFormSetDefinition()

    # parse and collect data structures info in the ExpandedHeader.i files
    def ParseVfrHeader(self):
        # call C preprocessor, need to modify in the future
        # wip
        try:
            InputStream = FileStream(self.Options.ProcessedVfrFileName)  ###
            VfrLexer = SourceVfrSyntaxLexer(InputStream)
            VfrStream = CommonTokenStream(VfrLexer)
            self.VfrParser = SourceVfrSyntaxParser(VfrStream)
        except:
            EdkLogger.error(
                "VfrCompiler",
                FILE_OPEN_FAILURE,
                "File open failed for %s" % self.Options.ProcessedVfrFileName,
                None,
            )

        self.Visitor = SourceVfrSyntaxVisitor(None, self.Options.OverrideClassGuid)
        self.Visitor.visit(self.VfrParser.vfrHeader())
        # FileName = self.Options.OutputDirectory + self.Options.BaseFileName + 'Header.txt'
        # try:
        #     f = open(FileName, 'w')
        #     gVfrVarDataTypeDB.Dump(f)
        #     f.close()
        # except:
        #     EdkLogger.error('VfrCompiler', FILE_OPEN_FAILURE,
        #                     'File open failed for %s' % FileName, None)

    def _DeclareStandardDefaultStorage(self):
        DSObj = IfrDefaultStore("Standard Defaults")
        gVfrDefaultStore.RegisterDefaultStore(
            DSObj.DefaultStore,
            "Standard Defaults",
            EFI_STRING_ID_INVALID,
            EFI_HII_DEFAULT_CLASS_STANDARD,
        )
        DSObj.SetDefaultName(EFI_STRING_ID_INVALID)
        DSObj.SetDefaultId(EFI_HII_DEFAULT_CLASS_STANDARD)
        DsNode = IfrTreeNode(EFI_IFR_DEFAULTSTORE_OP, DSObj, gFormPkg.StructToStream(DSObj.GetInfo()))
        DSObjMF = IfrDefaultStore("Standard ManuFacturing")
        gVfrDefaultStore.RegisterDefaultStore(
            DSObjMF.DefaultStore,
            "Standard ManuFacturing",
            EFI_STRING_ID_INVALID,
            EFI_HII_DEFAULT_CLASS_MANUFACTURING,
        )
        DSObjMF.SetDefaultName(EFI_STRING_ID_INVALID)
        DSObjMF.SetDefaultId(EFI_HII_DEFAULT_CLASS_MANUFACTURING)
        DsNodeMF = IfrTreeNode(EFI_IFR_DEFAULTSTORE_OP, DSObjMF, gFormPkg.StructToStream(DSObjMF.GetInfo()))

        return DsNode, DsNodeMF

    def _ToList(self, Value):
        if isinstance(Value, list):
            return Value
        else:
            return [Value]

    def _GetLineNumber(self, Target):
        for Line_Num, Line in enumerate(self.Lines, start=1):
            if Target in Line:
                return Line_Num
        return None

    # parse Formset Definition
    def ParseVfrFormSetDefinition(self):
        Formset = self.YamlDict["formset"]
        ClassGuidNum = 0
        GuidList = []
        if "classguid" in Formset.keys():
            GuidList = self._ToList(Formset["classguid"].PostVal)
            ClassGuidNum = len(GuidList)

        DefaultClassGuid = EFI_HII_PLATFORM_SETUP_FORMSET_GUID

        if self.Options.OverrideClassGuid != None and ClassGuidNum >= 4:
            self.ErrorHandler(
                VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                "Already has 4 class guids, can not add extra class guid!",
            )

        if ClassGuidNum == 0:
            if self.Options.OverrideClassGuid != None:
                ClassGuidNum = 2
            else:
                ClassGuidNum = 1
            FSObj = IfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID))
            FSObj.SetClassGuid(DefaultClassGuid)
            if self.Options.OverrideClassGuid != None:
                FSObj.SetClassGuid(self.Options.OverrideClassGuid)

        elif ClassGuidNum == 1:
            if self.Options.OverrideClassGuid != None:
                ClassGuidNum += 1
            FSObj = IfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID))
            FSObj.SetClassGuid(GuidList[0])
            if self.Options.OverrideClassGuid != None:
                FSObj.SetClassGuid(self.Options.OverrideClassGuid)

        elif ClassGuidNum == 2:
            if self.Options.OverrideClassGuid != None:
                ClassGuidNum += 1
            FSObj = IfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID))
            FSObj.SetClassGuid(GuidList[0])
            FSObj.SetClassGuid(GuidList[1])
            if self.Options.OverrideClassGuid != None:
                FSObj.SetClassGuid(self.Options.OverrideClassGuid)

        elif ClassGuidNum == 3:
            if self.Options.OverrideClassGuid != None:
                ClassGuidNum += 1
            FSObj = IfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID))
            FSObj.SetClassGuid(GuidList[0])
            FSObj.SetClassGuid(GuidList[1])
            FSObj.SetClassGuid(GuidList[2])
            if self.Options.OverrideClassGuid != None:
                FSObj.SetClassGuid(self.Options.OverrideClassGuid)

        elif ClassGuidNum == 4:
            if self.Options.OverrideClassGuid != None:
                ClassGuidNum += 1
            FSObj = IfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID))
            FSObj.SetClassGuid(GuidList[0])
            FSObj.SetClassGuid(GuidList[1])
            FSObj.SetClassGuid(GuidList[2])
            FSObj.SetClassGuid(GuidList[3])

        FSObj.SetGuid(Formset["guid"].PostVal)
        FSObj.SetFormSetTitle(Formset["title"].PostVal)
        FSObj.SetHelp(Formset["help"].PostVal)

        Node = IfrTreeNode(EFI_IFR_FORM_SET_OP, FSObj, gFormPkg.StructToStream(FSObj.GetInfo()), "")

        if len(GuidList) == 0:
            Node.Buffer += gFormPkg.StructToStream(DefaultClassGuid)
        else:
            for i in range(0, len(GuidList)):
                Node.Buffer += gFormPkg.StructToStream(GuidList[i])

        self.Root.insertChild(Node)

        if "class" in Formset.keys():
            self.ParseClassDefinition(self._ToList(Formset["class"].PostVal), Node)

        if "subclass" in Formset.keys():
            self.ParseSubClassDefinition(Formset["subclass"].PostVal, Node)

        DsNode, DsNodeMF = self._DeclareStandardDefaultStorage()
        Node.insertChild(DsNode)
        Node.insertChild(DsNodeMF)

        if "component" in Formset.keys():
            self._ParseVfrFormSetComponent(Formset["component"], Node)

        # Declare undefined Question so that they can be used in expression.
        InsertOpCodeList = None
        if gFormPkg.HavePendingUnassigned():
            InsertOpCodeList, ReturnCode = gFormPkg.DeclarePendingQuestion(gVfrVarDataTypeDB, gVfrDataStorage, self.VfrQuestionDB)
            Status = 0 if ReturnCode == VfrReturnCode.VFR_RETURN_SUCCESS else 1
            self.ParserStatus += Status  ###
            self.NeedAdjustOpcode = True

        self.InsertEndNode(Node)

        if self.NeedAdjustOpcode:
            self.LastFormNode.Child.pop()
            for InsertOpCode in InsertOpCodeList:
                InsertNode = IfrTreeNode(
                    InsertOpCode.OpCode,
                    InsertOpCode.Data,
                    gFormPkg.StructToStream(InsertOpCode.Data.GetInfo()),
                )
                self.LastFormNode.insertChild(InsertNode)

        gFormPkg.BuildPkg(self.Root)
        return Node

    def ParseClassDefinition(self, ClassList, ParentNode: IfrTreeNode):
        CObj = IfrClass()
        Class = 0
        for ClassName in ClassList:
            if ClassName == "NON_DEVICE":
                Class |= EFI_NON_DEVICE_CLASS
            elif ClassName == "DISK_DEVICE":
                Class |= EFI_DISK_DEVICE_CLASS
            elif ClassName == "VIDEO_DEVICE":
                Class |= EFI_VIDEO_DEVICE_CLASS
            elif ClassName == "NETWORK_DEVICE":
                Class |= EFI_NETWORK_DEVICE_CLASS
            elif ClassName == "INPUT_DEVICE":
                Class |= EFI_INPUT_DEVICE_CLASS
            elif ClassName == "ONBOARD_DEVICE":
                Class |= EFI_ON_BOARD_DEVICE_CLASS
            elif ClassName == "OTHER_DEVICE":
                Class |= EFI_OTHER_DEVICE_CLASS
            else:
                Class |= ClassName
        CObj.SetClass(Class)
        Node = IfrTreeNode(EFI_IFR_GUID_OP, CObj, gFormPkg.StructToStream(CObj.GetInfo()))
        ParentNode.insertChild(Node)
        return Node

    def ParseSubClassDefinition(self, SubClassVar, ParentNode: IfrTreeNode):
        SubObj = IfrSubClass()
        SubClass = 0
        if SubClassVar == "SETUP_APPLICATION":
            SubClass |= EFI_SETUP_APPLICATION_SUBCLASS
        elif SubClassVar == "GENERAL_APPLICATION":
            SubClass |= EFI_GENERAL_APPLICATION_SUBCLASS
        elif SubClassVar == "FRONT_PAGE":
            SubClass |= EFI_FRONT_PAGE_SUBCLASS
        elif SubClassVar == "SINGLE_USE":
            SubClass |= EFI_SINGLE_USE_SUBCLASS
        else:
            SubClass |= SubClassVar

        SubObj.SetSubClass(SubClass)
        Node = IfrTreeNode(EFI_IFR_GUID_OP, SubObj, gFormPkg.StructToStream(SubObj.GetInfo()))
        ParentNode.insertChild(Node)
        return Node

    def _ParseVfrFormSetComponent(self, ComponentList, ParentNode):
        for Component in ComponentList:
            ((Key, Value),) = Component.items()
            if Key == "form":
                self.ParseVfrFormDefinition(Value, ParentNode)
                continue
            elif Key == "formmap":
                self.ParseVfrFormmapDefinition(Value, ParentNode)
            elif Key == "image":
                self.ParseVfrStatementImage(Value, ParentNode)
            elif Key == "varstore":
                self.ParseVfrStatementVarStoreLinear(Value, ParentNode)
            elif Key == "efivarstore":
                self.ParseVfrStatementVarStoreEfi(Value, ParentNode)
            elif Key == "namevaluevarstore":
                self.ParseVfrStatementVarStoreNameValue(Value, ParentNode)
            elif Key == "defaultstore":
                self.ParseVfrStatementDefaultStore(Value, ParentNode)
            elif Key == "disableif":
                self.ParseVfrStatementDisableIfFormSet(Value, ParentNode)
            elif Key == "suppressif":
                self.ParseVfrStatementSuppressIfFormSet(Value, ParentNode)
            elif Key == "guidop":
                self.ParseVfrStatementExtension(Value, ParentNode)

    def ParseVfrStatementDisableIfFormSet(self, DisableIf, ParentNode):
        DIObj = IfrDisableIf()
        Node = IfrTreeNode(EFI_IFR_DISABLE_IF_OP, DIObj, gFormPkg.StructToStream(DIObj.GetInfo()))
        ParentNode.insertChild(Node)
        self.ParserVfrStatementExpression(DisableIf["expression"].PostVal, Node)
        Node.Expression = DisableIf["expression"].PostVal
        Node.Condition = "disableif " + Node.Expression
        if "component" in DisableIf.keys():
            self._ParseVfrFormSetComponent(DisableIf["component"], Node)
        self.InsertEndNode(Node)

    def ParseVfrStatementSuppressIfFormSet(self, SuppressIf, ParentNode):
        SIObj = IfrSuppressIf()
        Node = IfrTreeNode(EFI_IFR_SUPPRESS_IF_OP, SIObj, gFormPkg.StructToStream(SIObj.GetInfo()))
        ParentNode.insertChild(Node)
        self.ParserVfrStatementExpression(SuppressIf["expression"].PostVal, Node)
        Node.Expression = SuppressIf["expression"].PostVal
        Node.Condition = "suppressif " + Node.Expression
        if "component" in SuppressIf.keys():
            self._ParseVfrFormSetComponent(SuppressIf["component"], Node)
        self.InsertEndNode(Node)

    def ParserVfrStatementExpression(self, Condition, ParentNode):
        VfrLexer = SourceVfrSyntaxLexer(InputStream(Condition))
        VfrStream = CommonTokenStream(VfrLexer)
        VfrParser = SourceVfrSyntaxParser(VfrStream)
        Visitor = SourceVfrSyntaxVisitor(self.PreProcessDB, None, None, self.VfrQuestionDB)
        Visitor.visit(VfrParser.vfrStatementExpression(ParentNode))

    def ParseVfrStatementDefaultStore(self, DefaultStore, ParentNode):
        RefName = DefaultStore["type"].PostVal
        DSObj = IfrDefaultStore(RefName)
        DefaultStoreNameId = DefaultStore["prompt"].PostVal
        DefaultId = EFI_HII_DEFAULT_CLASS_STANDARD
        if "attribute" in DefaultStore.keys():
            DefaultId = DefaultStore["attribute"].PostVal
        if gVfrDefaultStore.DefaultIdRegistered(DefaultId) == False:
            # f'" wrong value comes from Opcode defaultstore with value {DefaultStore['attribute'].PreVal}"
            self.ErrorHandler(gVfrDefaultStore.RegisterDefaultStore(DSObj.DefaultStore, RefName, DefaultStoreNameId, DefaultId))
            DSObj.SetDefaultName(DefaultStoreNameId)
            DSObj.SetDefaultId(DefaultId)
            Node = IfrTreeNode(EFI_IFR_DEFAULTSTORE_OP, DSObj, gFormPkg.StructToStream(DSObj.GetInfo()))
            ParentNode.insertChild(Node)
        else:
            pNode, ReturnCode = gVfrDefaultStore.ReRegisterDefaultStoreById(DefaultId, RefName, DefaultStoreNameId)
            self.ErrorHandler(ReturnCode, f"{DefaultStore['attribute'].PreVal}")
            # ctx.Node.OpCode = EFI_IFR_SHOWN_DEFAULTSTORE_OP # For display in YAML
            # ctx.Node.Data = DSObj

    def ParseVfrStatementVarStoreLinear(self, VarStore, ParentNode):
        TypeName = VarStore["type"].PostVal
        if TypeName == "CHAR16":
            TypeName = "UINT16"

        IsBitVarStore = False
        if TypeName not in BasicDataTypes:
            IsBitVarStore = gVfrVarDataTypeDB.DataTypeHasBitField(TypeName)

        VarStoreId = EFI_VARSTORE_ID_INVALID
        if "varid" in VarStore.keys():
            VarStoreId = VarStore["varid"].PostVal
            # self.CompareErrorHandler(VarStoreId!=0, True, Tok.line, Tok.text, 'varid 0 is not allowed.')
        StoreName = VarStore["name"].PostVal
        VSObj = IfrVarStore(TypeName, StoreName)
        Guid = VarStore["guid"].PostVal
        self.ErrorHandler(gVfrDataStorage.DeclareBufferVarStore(StoreName, Guid, gVfrVarDataTypeDB, TypeName, VarStoreId, IsBitVarStore))
        VSObj.SetGuid(Guid)
        VarStoreId, ReturnCode = gVfrDataStorage.GetVarStoreId(StoreName, Guid)
        self.ErrorHandler(ReturnCode)
        VSObj.SetVarStoreId(VarStoreId)
        Size, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByTypeName(TypeName)
        self.ErrorHandler(ReturnCode)
        VSObj.SetSize(Size)
        Node = IfrTreeNode(EFI_IFR_VARSTORE_OP, VSObj, gFormPkg.StructToStream(VSObj.GetInfo()))
        Node.Buffer += bytes("\0", encoding="utf-8")
        ParentNode.insertChild(Node)
        return Node

    def ParseVfrStatementVarStoreEfi(self, VarstoreEfi, ParentNode):
        CustomizedName = False
        IsBitVarStore = False
        VarStoreId = EFI_VARSTORE_ID_INVALID
        IsUEFI23EfiVarstore = True
        ReturnCode = None

        TypeName = VarstoreEfi["type"].PostVal
        if TypeName == "CHAR16":
            TypeName = "UINT16"

        if TypeName not in BasicDataTypes:
            IsBitVarStore = gVfrVarDataTypeDB.DataTypeHasBitField(TypeName)
            CustomizedName = True
        if "varid" in VarstoreEfi.keys():
            VarStoreId = VarstoreEfi["varid"].PostVal
            # self.CompareErrorHandler(VarStoreId!=0, True, ctx.ID.line, ctx.ID.text, 'varid 0 is not allowed.')
        StoreName = VarstoreEfi["name"].PostVal
        Guid = VarstoreEfi["guid"].PostVal
        Attributes = self._ParseVarstoreEfiAttr(VarstoreEfi)
        self.ErrorHandler(
            gVfrDataStorage.DeclareBufferVarStore(
                StoreName,
                Guid,
                gVfrVarDataTypeDB,
                TypeName,
                VarStoreId,
                IsBitVarStore,
                Attributes,
            )
        )
        VarStoreId, ReturnCode = gVfrDataStorage.GetVarStoreId(StoreName, Guid)
        self.ErrorHandler(ReturnCode)
        Size, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByTypeName(TypeName)
        self.ErrorHandler(ReturnCode)
        VSEObj = IfrVarStoreEfi(TypeName, StoreName)
        VSEObj.SetGuid(Guid)
        VSEObj.SetVarStoreId(VarStoreId)
        VSEObj.SetSize(Size)
        VSEObj.SetAttributes(Attributes)
        Node = IfrTreeNode(EFI_IFR_VARSTORE_EFI_OP, VSEObj, gFormPkg.StructToStream(VSEObj.GetInfo()))
        Node.Buffer += bytes("\0", encoding="utf-8")
        ParentNode.insertChild(Node)
        return Node

    def _ParseVarstoreEfiAttr(self, VarstoreEfi):
        Attributes = 0
        if "attribute" in VarstoreEfi.keys():
            VarstoreEfiList = self._ToList(VarstoreEfi["attribute"].PostVal)
            for Attr in VarstoreEfiList:
                Attributes |= Attr
        return Attributes

    def ParseVfrStatementVarStoreNameValue(self, NameValue, ParentNode):
        StoreName = NameValue["type"].PostVal
        VSNVObj = IfrVarStoreNameValue(StoreName)
        Guid = NameValue["guid"].PostVal
        VarStoreId = EFI_VARSTORE_ID_INVALID
        if "varid" in NameValue.keys():
            VarStoreId = NameValue["varid"].PostVal
            # self.CompareErrorHandler(VarStoreId!=0, True, ctx.ID.line, ctx.ID.text, 'varid 0 is not allowed.')

        Created = False
        for NameDict in NameValue["nametable"].PostVal:
            if Created == False:
                self.ErrorHandler(gVfrDataStorage.DeclareNameVarStoreBegin(StoreName, VarStoreId))
                Created = True
            NameItem = NameDict["name"].PostVal
            # print(NameItem)
            VSNVObj.SetNameItemList(NameItem)
            gVfrDataStorage.NameTableAddItem(NameItem)
        gVfrDataStorage.DeclareNameVarStoreEnd(Guid)

        VSNVObj.SetGuid(Guid)
        VarstoreId, ReturnCode = gVfrDataStorage.GetVarStoreId(StoreName, Guid)
        self.ErrorHandler(ReturnCode)
        VSNVObj.SetVarStoreId(VarstoreId)
        Node = IfrTreeNode(
            EFI_IFR_VARSTORE_NAME_VALUE_OP,
            VSNVObj,
            gFormPkg.StructToStream(VSNVObj.GetInfo()),
        )
        ParentNode.insertChild(Node)
        return Node

    def ParseVfrFormmapDefinition(self, Formmap, ParentNode):
        FMapObj = IfrFormMap()
        if isinstance(Formmap["formid"].PostVal, list):
            Pos = self.GuidID + "." + Formmap["formid"].PostVal[0]
            FMapObj.SetFormId(Formmap["formid"].PostVal[1])
        else:
            FMapObj.SetFormId(Formmap["formid"].PostVal)
            Pos = self.GuidID + "." + str(Formmap["formid"].PostVal)

        FormMapMethodNumber = len(Formmap["map"].PostVal) if "map" in Formmap.keys() else 0
        if FormMapMethodNumber == 0:
            self.ErrorHandler(
                VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                "No MapMethod is set for FormMap!",
            )
        else:
            for MapDict in Formmap["map"].PostVal:
                FMapObj.SetFormMapMethod(MapDict["maptitle"].PostVal, MapDict["mapguid"].PostVal)

        Node = IfrTreeNode(EFI_IFR_FORM_MAP_OP, FMapObj)
        ParentNode.insertChild(Node)

        if "component" in Formmap.keys():
            for Component in Formmap["component"]:
                ((Key, Value),) = Component.items()
                self._ParseVfrForm(Key, Value, Node, Pos)
        FormMap = FMapObj.GetInfo()
        MethodMapList = FMapObj.GetMethodMapList()
        for MethodMap in MethodMapList:  # Extend Header Size for MethodMapList
            FormMap.Header.Length += sizeof(EFI_IFR_FORM_MAP_METHOD)
        Node.Buffer = gFormPkg.StructToStream(FormMap)
        Node.Position = Pos
        for MethodMap in MethodMapList:
            Node.Buffer += gFormPkg.StructToStream(MethodMap)
        self.InsertEndNode(Node)
        return Node

    def ParseVfrFormDefinition(self, Form, ParentNode: IfrTreeNode):
        FObj = IfrForm()
        if isinstance(Form["formid"].PostVal, list):
            Pos = self.GuidID + "." + Form["formid"].PostVal[0]
            ReturnCode = FObj.SetFormId(Form["formid"].PostVal[1])
        else:
            ReturnCode = FObj.SetFormId(Form["formid"].PostVal)
            Pos = self.GuidID + "." + str(Form["formid"].PostVal)
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            self.ErrorHandler(ReturnCode, "FormId wrong!")
        FormTitle = Form["title"].PostVal
        FObj.SetFormTitle(FormTitle)
        Node = IfrTreeNode(EFI_IFR_FORM_OP, FObj, gFormPkg.StructToStream(FObj.GetInfo()), Pos)
        ParentNode.insertChild(Node)
        if "component" in Form.keys():
            for Component in Form["component"]:
                ((Key, Value),) = Component.items()
                self._ParseVfrForm(Key, Value, Node, Pos)
        self.InsertEndNode(Node)
        self.LastFormNode = Node
        return Node

    def _ParseVfrForm(self, Key, Value, ParentNode, Position):
        if Key == "image":
            self.ParseVfrStatementImage(Value, ParentNode)
        elif Key == "locked":
            self.ParseVfrStatementLocked(Value, ParentNode)
        elif Key == "rule":
            self.ParseVfrStatementRule(Value, ParentNode)
        elif Key == "default":
            self.ParseVfrStatementDefault(Value, ParentNode)
        elif Key in vfrStatementStat:
            self.ParseVfrStatementStat(Key, Value, ParentNode, Position)
        elif Key in vfrStatementQuestions:
            self.ParseVfrStatementQuestions(Key, Value, ParentNode, Position)
        elif Key in vfrStatementConditional:
            self.ParseVfrStatementConditional(Key, Value, ParentNode, Position)
        elif Key == "label":
            self.ParseVfrStatementLabel(Value, ParentNode)
        elif Key == "banner":
            self.ParseVfrStatementbanner(Value, ParentNode)
        elif Key == "guidop":
            self.ParseVfrStatementExtension(Value, ParentNode)
        elif Key == "modal":
            self.ParseVfrStatementModal(Value, ParentNode)
        elif Key == "refreshguid":
            self.ParseVfrStatementRefreshEvent(Value, ParentNode)

    def ParseVfrStatementDefault(self, Default, ParentNode):
        IsExp = False
        DefaultId = EFI_HII_DEFAULT_CLASS_STANDARD
        if "value" in Default.keys():
            ValueList = self._ParseVfrConstantValueField(Default["value"].PostVal)
            Value = ValueList[0]
            Type = self.CurrQestVarInfo.VarType
            if self.CurrentMinMaxData != None and self.CurrentMinMaxData.IsNumericOpcode():
                # check default value is valid for Numeric Opcode
                for i in range(0, len(ValueList)):
                    Value = ValueList[i]
                    if type(Value) == int:
                        if Value < self.CurrentMinMaxData.GetMinData() or Value > self.CurrentMinMaxData.GetMaxData():
                            self.ErrorHandler(
                                VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                                "Numeric default value must be between MinValue and MaxValue.",
                            )

            if Type == EFI_IFR_TYPE_OTHER:
                self.ErrorHandler(VfrReturnCode.VFR_RETURN_FATAL_ERROR, "Default data type error.")
            DObj = IfrDefault(Type, ValueList)
            DObj.ValueStream = str(Default["value"].PostVal)
            Node = IfrTreeNode(EFI_IFR_DEFAULT_OP, DObj)
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
        elif "value_exp" in Default.keys():
            IsExp = True
            DObj = IfrDefault2()
            Node = IfrTreeNode(EFI_IFR_DEFAULT_OP, DObj)
            DObj.SetScope(1)
            self.ParseVfrStatementValue(Default["value_exp"].PostVal, Node)
            self.InsertEndNode(Node)

        if "defaultstore" in Default.keys():
            DefaultId, ReturnCode = gVfrDefaultStore.GetDefaultId(Default["defaultstore"].PostVal)
            self.ErrorHandler(ReturnCode)
            DObj.SetDefaultId(DefaultId)

        self._CheckDuplicateDefaultValue(DefaultId)
        if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            VarStoreName, ReturnCode = gVfrDataStorage.GetVarStoreName(self.CurrQestVarInfo.VarStoreId)
            self.ErrorHandler(ReturnCode)
            VarGuid = gVfrDataStorage.GetVarStoreGuid(self.CurrQestVarInfo.VarStoreId)
            VarStoreType = gVfrDataStorage.GetVarStoreType(self.CurrQestVarInfo.VarStoreId)
            if (IsExp == False) and (VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER):
                self.ErrorHandler(
                    gVfrDefaultStore.BufferVarStoreAltConfigAdd(
                        DefaultId,
                        self.CurrQestVarInfo,
                        VarStoreName,
                        VarGuid,
                        self.CurrQestVarInfo.VarType,
                        Value,
                    )
                )

        Node.Buffer = gFormPkg.StructToStream(DObj.GetInfo())
        ParentNode.insertChild(Node)
        return Node

    def ParseVfrStatementRule(self, Rule, ParentNode):
        RObj = IfrRule()
        RuleName = Rule["name"].PostVal
        RObj.SetRuleName(RuleName)
        self.VfrRulesDB.RegisterRule(RuleName)
        RObj.SetRuleId(self.VfrRulesDB.GetRuleId(RuleName))
        Node = IfrTreeNode(EFI_IFR_RULE_OP, RObj, gFormPkg.StructToStream(RObj.GetInfo()))
        ParentNode.insertChild(Node)
        self.ParserVfrStatementExpression(Rule["expression"].PostVal, Node)
        Node.Expression = Rule["expression"].PostVal
        self.InsertEndNode(Node)
        return Node

    def ParseVfrStatementExtension(self, GuidOp, ParentNode):
        if len(GuidOp) == 1:
            GuidObj = IfrExtensionGuid()
            Buffer = None
            GuidObj.SetGuid(GuidOp["guid"].PostVal)
        else:
            DataKey = None
            DataValue = None
            Guid = None
            for Key in GuidOp.keys():
                if Key == "guid":
                    Guid = GuidOp["guid"].PostVal
                else:
                    DataKey = Key
                    DataValue = GuidOp[Key]
            Match = re.findall(r"\[(.*?)\]", DataKey)
            if Match:
                IsArray = True
                ArrayNum = int(Match[0])
                TypeName = re.sub(r"\[.*?\]", "", DataKey)
            else:
                IsArray = False
                ArrayNum = 0
                TypeName = DataKey
            TypeSize, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByTypeName(TypeName)
            self.ErrorHandler(ReturnCode)
            Size = TypeSize * ArrayNum if ArrayNum > 0 else TypeSize
            Buffer = bytearray(Size)
            GuidObj = IfrExtensionGuid(Size, TypeName, ArrayNum)
            GuidObj.SetGuid(Guid)
            for Key in DataValue:
                TFValue = DataValue[Key].PostVal
                if IsArray:
                    Start = Key.find("[")
                    End = Key.find("]")
                    Index = int(Key[Start + 1 : End])
                else:
                    Index = 0
                if Key.find(".") != -1:
                    TFName = TypeName + "." + Key.split(".", 1)[1]
                else:
                    TFName = TypeName
                FieldOffset, FieldType, FieldSize, BitField, _ = gVfrVarDataTypeDB.GetDataFieldInfo(TFName)
                ByteOffset = TypeSize * Index
                if not BitField:
                    Offset = ByteOffset + FieldOffset
                    Buffer[Offset : Offset + FieldSize] = TFValue.to_bytes(FieldSize, "little")
                else:
                    Mask = (1 << FieldSize) - 1
                    Offset = FieldOffset // 8
                    PreBits = FieldOffset % 8
                    Mask <<= PreBits
                    Value = int.from_bytes(Buffer[ByteOffset + Offset : ByteOffset + Offset + FieldSize], "little")
                    TFValue <<= PreBits
                    Value = (Value & (~Mask)) | TFValue
                    Buffer[ByteOffset + Offset : ByteOffset + Offset + FieldSize] = Value.to_bytes(FieldSize, "little")

        GuidObj.SetScope(1)
        GuidObj.SetData(Buffer)
        Node = IfrTreeNode(EFI_IFR_GUID_OP, GuidObj, gFormPkg.StructToStream(GuidObj.GetInfo()))
        ParentNode.insertChild(Node)
        if Buffer != None:
            NewBuffer = bytearray(Size)
            for i in range(0, len(NewBuffer)):
                NewBuffer[i] = Buffer[i]
            Node.Buffer = gFormPkg.StructToStream(GuidObj.GetInfo()) + NewBuffer
        self.InsertEndNode(Node)
        return Node

    def ParseVfrStatementbanner(self, Banner, ParentNode):
        BObj = IfrBanner()
        BObj.SetTitle(Banner["title"].PostVal)
        if "line" in Banner.keys():
            BObj.SetLine(Banner["line"].PostVal)
        if "left" in Banner.keys():
            BObj.SetAlign(0)
        if "center" in Banner.keys():
            BObj.SetAlign(1)
        if "right" in Banner.keys():
            BObj.SetAlign(2)
        Node = IfrTreeNode(EFI_IFR_GUID_OP, BObj, gFormPkg.StructToStream(BObj.GetInfo()))
        ParentNode.insertChild(Node)
        ########　timeout
        return Node

    def ParseVfrStatementModal(self, Modal, ParentNode):
        MObj = IfrModal()
        Node = IfrTreeNode(EFI_IFR_MODAL_TAG_OP, MObj, gFormPkg.StructToStream(MObj.GetInfo()))
        ParentNode.insertChild(Node)
        return Node

    def ParseVfrStatementRefreshEvent(self, RefreshEvent, ParentNode):
        RiObj = IfrRefreshId()
        RiObj.SetRefreshEventGroutId(RefreshEvent["guid"].PostVal)
        Node = IfrTreeNode(EFI_IFR_REFRESH_ID_OP, RiObj, gFormPkg.StructToStream(RiObj.GetInfo()))
        ParentNode.insertChild(Node)
        return Node

    def ParseVfrStatementStat(self, Key, Value, ParentNode, Position):
        if Key == "subtitle":
            self.ParseVfrStatementSubtitle(Value, ParentNode)
        if Key == "text":
            self.ParseVfrStatementStaticText(Value, ParentNode)
        if Key == "goto":
            self.ParseVfrStatementGoto(Value, ParentNode, Position)
        if Key == "resetbutton":
            self.ParseVfrStatementResetButton(Value, ParentNode)

    def ParseVfrStatementSubtitle(self, Subtitle, ParentNode):
        SObj = IfrSubtitle()
        Prompt = Subtitle["text"].PostVal
        SObj.SetPrompt(Prompt)
        SObj.SetFlags(self._ParseSubtitleFlags(Subtitle))
        Node = IfrTreeNode(EFI_IFR_SUBTITLE_OP, SObj, gFormPkg.StructToStream(SObj.GetInfo()))
        ParentNode.insertChild(Node)
        if "component" in Subtitle.keys():
            for Component in Subtitle["component"]:
                ((Key, Value),) = Component.items()
                self._ParseVfrStatementStatTag(Key, Value, Node)
                self.ParseVfrStatementStat(Key, Value, Node)
                self.ParseVfrStatementQuestions(Key, Value, Node)
        self.InsertEndNode(Node)
        return Node

    def _ParseVfrStatementStatTag(self, Key, Value, ParentNode):
        if Key == "image":
            self.ParseVfrStatementImage(Value, ParentNode)
        elif Key == "locked":
            self.ParseVfrStatementLocked(Value, ParentNode)

    def _ParseSubtitleFlags(self, Subtitle):
        SubFlags = 0
        if "flags" in Subtitle.keys():
            FlagList = self._ToList(Subtitle["flags"].PostVal)
            for Flag in FlagList:
                if Flag == "HORIZONTAL":
                    SubFlags |= 0x01
                else:
                    SubFlags |= Flag
        return SubFlags

    def ParseVfrStatementResetButton(self, ResetButton, ParentNode):
        Defaultstore = ResetButton["defaultstore"].PostVal
        RBObj = IfrResetButton()
        DefaultId, ReturnCode = gVfrDefaultStore.GetDefaultId(Defaultstore)
        self.ErrorHandler(ReturnCode)
        RBObj.SetDefaultId(DefaultId)

        self._ParseVfrStatementHeader(RBObj, ResetButton)
        Node = IfrTreeNode(EFI_IFR_RESET_BUTTON_OP, RBObj, gFormPkg.StructToStream(RBObj.GetInfo()))
        ParentNode.insertChild(Node)
        if "component" in ResetButton.keys():
            for Component in ResetButton["component"]:
                ((Key, Value),) = Component.items()
                self._ParseVfrStatementStatTag(Key, Value, Node)
        self.InsertEndNode(Node)

        return Node

    def ParseVfrStatementGoto(self, Goto, ParentNode, Position):
        R5Obj = IfrRef5()
        GObj = R5Obj

        # get formid
        if "formid" in Goto.keys():
            if isinstance(Goto["formid"].PostVal, list):
                FormId = Goto["formid"].PostVal[1]
            else:
                FormId = Goto["formid"].PostVal

        if "devicepath" in Goto.keys():
            R4Obj = IfrRef4()
            R4Obj.SetDevicePath(Goto["devicepath"].PostVal)
            R4Obj.SetFormId(FormId)
            R4Obj.SetQId(Goto["question"].PostVal)
            R4Obj.SetFormSetId(Goto["formsetguid"].PostVal)
            GObj = R4Obj
        elif "formsetguid" in Goto.keys():
            R3Obj = IfrRef3()
            R3Obj.SetFormId(FormId)
            R3Obj.SetQId(Goto["question"].PostVal)
            R3Obj.SetFormSetId(Goto["formsetguid"].PostVal)
            GObj = R3Obj
        elif "question" in Goto.keys():
            R2Obj = IfrRef2()
            R2Obj.SetFormId(FormId)
            R2Obj.SetQId(Goto["question"].PostVal)
            GObj = R2Obj
        elif "formid" in Goto.keys():
            RObj = IfrRef()
            RObj.SetFormId(FormId)
            GObj = RObj

        self._ParseVfrQuestionHeader(GObj, Goto, EFI_QUESION_TYPE.QUESTION_REF)
        GObj.SetFlags(self._ParseStatementStatFlags(Goto))
        if "key" in Goto.keys():
            self.AssignQuestionKey(GObj, Goto["key"].PostVal)
        Node = IfrTreeNode(EFI_IFR_REF_OP, GObj)
        self.SetPosition(Node, Position)
        ParentNode.insertChild(Node)
        if "component" in Goto.keys():
            GObj.SetScope(1)
            self._ParseVfrStatementQuestionOptionList(Goto["component"], Node)
            self.InsertEndNode(Node)

        Node.Buffer = gFormPkg.StructToStream(GObj.GetInfo())
        return Node

    def _ParseVfrStatementQuestionOptionList(self, ComponentList, ParentNode):
        for Component in ComponentList:
            ((Key, Value),) = Component.items()
            self._ParseVfrStatementQuestionTag(Key, Value, ParentNode)
            self._ParseVfrStatementQuestionOptionTag(Key, Value, ParentNode)

    def _ParseVfrStatementQuestionTag(self, Key, Value, ParentNode):
        if Key == "image":
            self.ParseVfrStatementImage(Value, ParentNode)
        elif Key == "locked":
            self.ParseVfrStatementLocked(Value, ParentNode)
        elif Key == "inconsistentif":
            self.ParseVfrStatementInconsistentIf(Value, ParentNode)
        elif Key == "nosubmitif":
            self.ParseVfrStatementNoSubmitIf(Value, ParentNode)
        elif Key == "disableif":
            self.ParseVfrStatementDisableIfQuest(Value, ParentNode)
        elif Key == "refresh":
            self.ParseVfrStatementRefresh(Value, ParentNode)
        elif Key == "varstoredevice":
            self.ParseVfrStatementVarstoreDevice(Value, ParentNode)
        elif Key == "guidop":
            self.ParseVfrStatementExtension(Value, ParentNode)
        elif Key == "refreshguid":
            self.ParseVfrStatementRefreshEvent(Value, ParentNode)
        elif Key == "warningif":
            self.ParseVfrStatementWarningIf(Value, ParentNode)

    def ParseVfrStatementInconsistentIf(self, InconsistentIf, ParentNode):
        IIObj = IfrInconsistentIf2()
        IIObj.SetError(InconsistentIf["prompt"].PostVal)
        Node = IfrTreeNode(EFI_IFR_INCONSISTENT_IF_OP, IIObj, gFormPkg.StructToStream(IIObj.GetInfo()))
        ParentNode.insertChild(Node)
        self.ParserVfrStatementExpression(InconsistentIf["expression"].PostVal, Node)
        Node.Expression = InconsistentIf["expression"].PostVal
        # Node.Condition = 'inconsistentif ' + Node.Expression
        self.InsertEndNode(Node)

    def ParseVfrStatementNoSubmitIf(self, NoSubmitIf, ParentNode):
        NSIObj = IfrNoSubmitIf()
        NSIObj.SetError(NoSubmitIf["prompt"].PostVal)
        Node = IfrTreeNode(EFI_IFR_NO_SUBMIT_IF_OP, NSIObj, gFormPkg.StructToStream(NSIObj.GetInfo()))
        ParentNode.insertChild(Node)
        self.ParserVfrStatementExpression(NoSubmitIf["expression"].PostVal, Node)
        Node.Expression = NoSubmitIf["expression"].PostVal
        self.InsertEndNode(Node)

    def ParseVfrStatementDisableIfQuest(self, DisableIfQuest, ParentNode):
        DIObj = IfrDisableIf()
        Node = IfrTreeNode(EFI_IFR_DISABLE_IF_OP, DIObj, gFormPkg.StructToStream(DIObj.GetInfo()))
        ParentNode.insertChild(Node)
        self.ParserVfrStatementExpression(DisableIfQuest["expression"].PostVal, Node)
        Node.Expression = DisableIfQuest["expression"].PostVal
        Node.Condition = "disableif " + Node.Expression
        if "component" in DisableIfQuest.keys():
            self._ParseVfrStatementQuestionOptionList(DisableIfQuest["component"], Node)
        self.InsertEndNode(Node)

    def ParseVfrStatementRefresh(self, Refresh, ParentNode):
        RObj = IfrRefresh()
        RObj.SetRefreshInterval(Refresh["interval"].PostVal)
        Node = IfrTreeNode(EFI_IFR_REFRESH_OP, RObj, gFormPkg.StructToStream(RObj.GetInfo()))
        ParentNode.insertChild(Node)
        return Node

    def ParseVfrStatementVarstoreDevice(self, VarStoreDevice, ParentNode):
        VDObj = IfrVarStoreDevice()
        VDObj.SetDevicePath(VarStoreDevice["devicepath"].PostVal)
        Node = IfrTreeNode(EFI_IFR_VARSTORE_DEVICE_OP, VDObj, gFormPkg.StructToStream(VDObj.GetInfo()))
        ParentNode.insertChild(Node)
        return Node

    def ParseVfrStatementWarningIf(self, WarningIf, ParentNode):
        WIObj = IfrWarningIf()
        WIObj.SetWarning(WarningIf["prompt"].PostVal)
        if "timeout" in WarningIf.keys():
            WIObj.SetTimeOut(WarningIf["timeout"].PostVal)
        Node = IfrTreeNode(EFI_IFR_WARNING_IF_OP, WIObj, gFormPkg.StructToStream(WIObj.GetInfo()))
        ParentNode.insertChild(Node)
        self.ParserVfrStatementExpression(WarningIf["expression"].PostVal, Node)
        Node.Expression = WarningIf["expression"].PostVal
        self.InsertEndNode(Node)
        return Node

    def _ParseVfrStatementQuestionOptionTag(self, Key, Value, ParentNode):
        if Key == "suppressif":
            self.ParseVfrStatementSuppressIfQuest(Value, ParentNode)
        elif Key == "grayoutif":
            self.ParseVfrStatementGrayOutIfQuest(Value, ParentNode)
        if Key == "value":
            self.ParseVfrStatementValue(Value, ParentNode)
        elif Key == "default":
            self.ParseVfrStatementDefault(Value, ParentNode)
        elif Key == "option":
            self.ParseVfrStatementOneofOption(Value, ParentNode)
        elif Key == "read":
            self.ParseVfrStatementRead(Value, ParentNode)
        elif Key == "write":
            self.ParseVfrStatementWrite(Value, ParentNode)

    def ParseVfrStatementSuppressIfQuest(self, SuppressIfQuest, ParentNode):
        SIObj = IfrSuppressIf()
        Node = IfrTreeNode(EFI_IFR_SUPPRESS_IF_OP, SIObj, gFormPkg.StructToStream(SIObj.GetInfo()))
        ParentNode.insertChild(Node)
        self.ParserVfrStatementExpression(SuppressIfQuest["expression"].PostVal, Node)
        Node.Expression = SuppressIfQuest["expression"].PostVal
        Node.Condition = "suppressif " + Node.Expression
        if "component" in SuppressIfQuest.keys():
            self._ParseVfrStatementQuestionOptionList(SuppressIfQuest["component"], Node)
        self.InsertEndNode(Node)

    def ParseVfrStatementGrayOutIfQuest(self, GrayOutIfQuest, ParentNode):
        GOIObj = IfrGrayOutIf()
        Node = IfrTreeNode(EFI_IFR_GRAY_OUT_IF_OP, GOIObj, gFormPkg.StructToStream(GOIObj.GetInfo()))
        ParentNode.insertChild(Node)
        self.ParserVfrStatementExpression(GrayOutIfQuest["expression"].PostVal, Node)
        Node.Expression = GrayOutIfQuest["expression"].PostVal
        Node.Condition = "grayoutif " + Node.Expression
        if "component" in GrayOutIfQuest.keys():
            self._ParseVfrStatementQuestionOptionList(GrayOutIfQuest["component"], Node)
        self.InsertEndNode(Node)

    def _ParseVfrConstantValueField(self, Value):  #########
        IntDecStyle = False
        if self.CurrentMinMaxData != None and self.CurrentMinMaxData.IsNumericOpcode():
            NumericQst = IfrNumeric(self.CurrentQuestion)  #
            IntDecStyle = True if (NumericQst.GetNumericFlags() & EFI_IFR_DISPLAY) == 0 else False  #
        ValueList = []
        if Value == True:
            ValueList.append(1)

        elif Value == False:
            ValueList.append(0)
        elif Value == "ONE":
            ValueList.append(int("ONE"))
        elif Value == "ONES":
            ValueList.append(int("ONES"))
        elif Value == "ZERO":
            ValueList.append(int("ZERO"))
        elif type(Value) == int:
            ValueList.append(Value)
        elif isinstance(Value, list):
            for Val in Value:
                ValueList.append(Val)
        elif Value.find(":") != -1:
            Val = Value[1:-1].split(":")
            Time = EFI_HII_TIME()
            Time.Hour = self.PreProcessDB.TransValue(Val[0].strip())
            Time.Minute = self.PreProcessDB.TransValue(Val[1].strip())
            Time.Second = self.PreProcessDB.TransValue(Val[2].strip())
            ValueList.append(Time)
        elif Value.find("/") != -1:
            Val = Value[1:-1].split("/")
            Date = EFI_HII_DATE()
            Date.Year = self.PreProcessDB.TransValue(Val[0].strip())
            Date.Month = self.PreProcessDB.TransValue(Val[1].strip())
            Date.Day = self.PreProcessDB.TransValue(Val[2].strip())
            ValueList.append(Date)
        elif Value.find(";") != -1:
            Val = Value[1:-1].split(";")
            Ref = EFI_HII_REF()
            Ref.QuestionId = self.PreProcessDB.TransValue(Val[0].strip())
            Ref.FormId = self.PreProcessDB.TransValue(Val[1].strip())
            Ref.FormSetGuid = self.PreProcessDB.TransValue(self.PreProcessDB.HeaderDict[Val[2].strip()])
            Start = Val[3].strip().find("(") + 1
            End = Val[3].strip().find(")")
            Ref.DevicePath = self.PreProcessDB.TransValue(self.PreProcessDB.UniDict[Val[3].strip()[Start:End]])
            ValueList.append(Ref)
        elif Value.find("STRING_TOKEN") != -1:
            Start = Value.find("(") + 1
            End = Value.find(")")
            ValueList.append(self.PreProcessDB.TransValue(self.PreProcessDB.UniDict[Value[Start:End]]))

        return ValueList

    def ParseVfrStatementOneofOption(self, Option, ParentNode):
        if self.CurrQestVarInfo.VarType == EFI_IFR_TYPE_OTHER:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_FATAL_ERROR, "Get data type error.")
        ValueList = self._ParseVfrConstantValueField(Option["value"].PostVal)
        Value = ValueList[0]
        Type = self.CurrQestVarInfo.VarType
        if self.CurrentMinMaxData != None:
            # set min/max value for oneof opcode
            Step = self.CurrentMinMaxData.GetStepData()
            self.CurrentMinMaxData.SetMinMaxStepData(Value, Value, Step)
        if self.CurrQestVarInfo.IsBitVar:
            Type = EFI_IFR_TYPE_NUM_SIZE_32
        OOOObj = IfrOneOfOption(Type, ValueList)
        if len(ValueList) == 1:
            OOOObj.SetType(Type)
        else:
            OOOObj.SetType(EFI_IFR_TYPE_BUFFER)

        OOOObj.SetOption(Option["text"].PostVal)
        HFlags, LFlags = self._ParseVfrStatementOneofOptionFlags(Option)
        self.ErrorHandler(OOOObj.SetFlags(LFlags))
        self.ErrorHandler(self.CurrentQuestion.SetQHeaderFlags(HFlags))

        # Array type only for default type OneOfOption.
        if (OOOObj.GetFlags() & (EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG)) == 0 and (len(ValueList) != 1):
            self.ErrorHandler(
                VfrReturnCode.VFR_RETURN_FATAL_ERROR,
                "Default keyword should with array value type!",
            )

        # Clear the default flag if the option not use array value but has default flag.
        if (OOOObj.GetFlags() & (EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG)) != 0 and (len(ValueList) == 1) and (self.IsOrderedList):
            OOOObj.SetFlags(OOOObj.GetFlags() & ~(EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG))

        if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            VarStoreName, ReturnCode = gVfrDataStorage.GetVarStoreName(self.CurrQestVarInfo.VarStoreId)
            self.ErrorHandler(ReturnCode)
            VarStoreGuid = gVfrDataStorage.GetVarStoreGuid(self.CurrQestVarInfo.VarStoreId)
            if OOOObj.GetFlags() & EFI_IFR_OPTION_DEFAULT:
                self._CheckDuplicateDefaultValue(EFI_HII_DEFAULT_CLASS_STANDARD)
                self.ErrorHandler(
                    gVfrDefaultStore.BufferVarStoreAltConfigAdd(
                        EFI_HII_DEFAULT_CLASS_STANDARD,
                        self.CurrQestVarInfo,
                        VarStoreName,
                        VarStoreGuid,
                        self.CurrQestVarInfo.VarType,
                        Value,
                    )
                )
            if OOOObj.GetFlags() & EFI_IFR_OPTION_DEFAULT_MFG:
                self._CheckDuplicateDefaultValue(EFI_HII_DEFAULT_CLASS_MANUFACTURING)
                self.ErrorHandler(
                    gVfrDefaultStore.BufferVarStoreAltConfigAdd(
                        EFI_HII_DEFAULT_CLASS_MANUFACTURING,
                        self.CurrQestVarInfo,
                        VarStoreName,
                        VarStoreGuid,
                        self.CurrQestVarInfo.VarType,
                        Value,
                    )
                )

        Node = IfrTreeNode(EFI_IFR_ONE_OF_OPTION_OP, OOOObj)
        Index = 0
        if ParentNode.Position != None:
            for ChildNode in ParentNode.Child:
                if ChildNode.OpCode == EFI_IFR_ONE_OF_OPTION_OP:
                    Index += 1
            Node.Position = ParentNode.Position + "." + "Option" + str(Index)

        ParentNode.insertChild(Node)

        if "key" in Option.keys():
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED)
            OOOObj.SetIfrOptionKey(Option["key"].PostVal)
            gIfrOptionKey = IfrOptionKey(self.CurrentQuestion.GetQuestionId(), Type, Value, Option["key"].PostVal)
            ChildNode = IfrTreeNode(
                EFI_IFR_GUID_OP,
                gIfrOptionKey,
                gFormPkg.StructToStream(gIfrOptionKey.GetInfo()),
            )
            Node.insertChild(ChildNode)

        if "component" in Option.keys():
            OOOObj.SetScope(1)
            for Component in Option["component"]:
                ((Key, Value),) = Component.items()
                if Key == "image":
                    self.ParseVfrStatementImage(Value, Node)
                    self.InsertEndNode(Node)

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
        if "flags" in Option.keys():
            FlagList = self._ToList(Option["flags"].PostVal)
            for Flag in FlagList:
                if Flag == "OPTION_DEFAULT":
                    LFlags |= 0x10
                elif Flag == "OPTION_DEFAULT_MFG":
                    LFlags |= 0x20
                elif Flag == "INTERACTIVE":
                    HFlags |= 0x04
                elif Flag == "RESET_REQUIRED":
                    HFlags |= 0x10
                elif Flag == "REST_STYLE":
                    HFlags |= 0x20
                elif Flag == "RECONNECT_REQUIRED":
                    HFlags |= 0x40
                elif Flag == "MANUFACTURING":
                    LFlags |= 0x20
                elif Flag == "DEFAULT":
                    LFlags |= 0x10
                elif Flag == "NV_ACCESS":
                    gVfrErrorHandle.HandleWarning(EFI_VFR_WARNING_CODE.VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE)
                elif Flag == "LATE_CHECK":
                    gVfrErrorHandle.HandleWarning(EFI_VFR_WARNING_CODE.VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE)
        return HFlags, LFlags

    def ParseVfrStatementValue(self, Value, ParentNode):
        VObj = IfrValue()
        Node = IfrTreeNode(EFI_IFR_VALUE_OP, VObj, gFormPkg.StructToStream(VObj.GetInfo()))
        ParentNode.insertChild(Node)
        if type(Value) == str:
            self.ParserVfrStatementExpression(Value, Node)
            Node.Expression = Value
        else:
            self.ParserVfrStatementExpression(Value["expression"].PostVal, Node)
            Node.Expression = Value["expression"].PostVal
        self.InsertEndNode(Node)
        return Node

    def ParseVfrStatementRead(self, Read, ParentNode):
        RObj = IfrRead()
        Node = IfrTreeNode(EFI_IFR_READ_OP, RObj, gFormPkg.StructToStream(RObj.GetInfo()))
        ParentNode.insertChild(Node)
        self.ParserVfrStatementExpression(Read["expression"].PostVal, Node)
        Node.Expression = Read["expression"].PostVal
        return Node

    def ParseVfrStatementWrite(self, Write, ParentNode):
        WObj = IfrWrite()
        Node = IfrTreeNode(EFI_IFR_WRITE_OP, WObj, gFormPkg.StructToStream(WObj.GetInfo()))
        ParentNode.insertChild(Node)
        self.ParserVfrStatementExpression(Write["expression"].PostVal, Node)
        Node.Expression = Write["expression"].PostVal
        return Node

    def _ParseStatementStatFlags(self, StatFlagsDict):
        Flags = 0
        if "flags" in StatFlagsDict.keys():
            FlagList = self._ToList(StatFlagsDict["flags"].PostVal)
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
        VarIdStr = ""

        ReturnCode = None
        if "name" in QDict.keys():
            QName = QDict["name"].PostVal
            ReturnCode = self.VfrQuestionDB.FindQuestionByName(QName)
            self.CompareErrorHandler(
                ReturnCode,
                VfrReturnCode.VFR_RETURN_UNDEFINED,
                QName,
                "has already been used please used anther name",
            )
        if "varid" in QDict.keys():
            VarIdStr = QDict["varid"].PostVal
            self._VfrStorageVarId(BaseInfo, VarIdStr)
        if "questionid" in QDict.keys():
            QId = QDict["questionid"].PostVal
            ReturnCode = self.VfrQuestionDB.FindQuestionById(QId)
            self.CompareErrorHandler(
                ReturnCode,
                VfrReturnCode.VFR_RETURN_UNDEFINED,
                QId,
                "has already been used please used anther number",
            )
        if QType == EFI_QUESION_TYPE.QUESTION_NORMAL:
            # if self.IsCheckBoxOp:
            # BaseInfo.VarType = EFI_IFR_TYPE_BOOLEAN #
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
            if VarIdStr != "":  # stand for question with storage.
                QId, ReturnCode = self.VfrQuestionDB.RegisterRefQuestion(QName, VarIdStr, QId, gFormPkg)
                self.ErrorHandler(ReturnCode)
            else:
                QId, ReturnCode = self.VfrQuestionDB.RegisterQuestion(QName, None, QId, gFormPkg)
                self.ErrorHandler(ReturnCode)
        else:
            self.ErrorHandler(VfrReturnCode.VFR_RETURN_FATAL_ERROR)

        QObj.SetQuestionId(QId)
        QObj.SetQName(QName)
        QObj.SetVarIdStr(VarIdStr)
        if BaseInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            QObj.SetVarStoreInfo(BaseInfo)

        self.CurrQestVarInfo = BaseInfo

        return BaseInfo

    def _ParseVfrStatementHeader(self, QObj, QDict):
        QObj.SetPrompt(QDict["prompt"].PostVal)
        QObj.SetHelp(QDict["help"].PostVal)

    def _VfrStorageVarId(self, BaseInfo, VarId):
        if "[" in VarId and "].PostVal" in VarId and "." not in VarId:
            SName = VarId[: VarId.find("[")]
            Idx = int(VarId[VarId.find("[") + 1])
            BaseInfo.VarStoreId, ReturnCode = gVfrDataStorage.GetVarStoreId(SName)
            self.ErrorHandler(ReturnCode)
            self.ErrorHandler(gVfrDataStorage.GetNameVarStoreInfo(BaseInfo, Idx))
        else:
            VarStr = ""  # type.field
            TName = ""
            if "." not in VarId:
                SName = VarId
                BaseInfo.VarStoreId, ReturnCode = gVfrDataStorage.GetVarStoreId(SName)
                self.ErrorHandler(ReturnCode)
                VarStoreType = gVfrDataStorage.GetVarStoreType(BaseInfo.VarStoreId)
                if VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER \
                    or VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER_BITS:
                    TName, ReturnCode2 = gVfrDataStorage.GetBufferVarStoreDataTypeName(BaseInfo.VarStoreId)
                    self.ErrorHandler(ReturnCode2)
                    VarStr += TName
            else:
                List = VarId.split(".")  # Mydd.d[2].vcc
                SName = List[0]
                BaseInfo.VarStoreId, ReturnCode = gVfrDataStorage.GetVarStoreId(SName)
                self.ErrorHandler(ReturnCode)
                VarStoreType = gVfrDataStorage.GetVarStoreType(BaseInfo.VarStoreId)
                if VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER \
                    or VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER_BITS:
                    TName, ReturnCode2 = gVfrDataStorage.GetBufferVarStoreDataTypeName(BaseInfo.VarStoreId)
                    self.ErrorHandler(ReturnCode2)
                VarStr = TName + VarId[VarId.find(".") :]
                cond = (VarStoreType != EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER) \
                    and (VarStoreType != EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER_BITS)
                ReturnCode = VfrReturnCode.VFR_RETURN_EFIVARSTORE_USE_ERROR if cond else VfrReturnCode.VFR_RETURN_SUCCESS
                self.ErrorHandler(ReturnCode)

            if VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_EFI:
                self.ErrorHandler(gVfrDataStorage.GetEfiVarStoreInfo(BaseInfo))

            elif VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER \
                or VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER_BITS:
                (
                    BaseInfo.Info.VarOffset,
                    BaseInfo.VarType,
                    BaseInfo.VarTotalSize,
                    BaseInfo.IsBitVar,
                    ReturnCode,
                ) = gVfrVarDataTypeDB.GetDataFieldInfo(VarStr)
                self.ErrorHandler(ReturnCode, VarStr)
                VarGuid = gVfrDataStorage.GetVarStoreGuid(BaseInfo.VarStoreId)
                self.ErrorHandler(gVfrBufferConfig.Register(SName, VarGuid))
                ReturnCode = VfrReturnCode(
                    gVfrBufferConfig.Write(
                        "a",
                        SName,
                        VarGuid,
                        None,
                        BaseInfo.VarType,
                        BaseInfo.Info.VarOffset,
                        BaseInfo.VarTotalSize,
                        self.Value,
                    )
                )
                self.ErrorHandler(ReturnCode)
                self.ErrorHandler(gVfrDataStorage.AddBufferVarStoreFieldInfo(BaseInfo))

    def ParseVfrStatementQuestions(self, Key, Value, ParentNode, Position):
        if Key == "checkbox":
            Node = self.ParseVfrStatementCheckBox(Value, ParentNode, Position)
        if Key == "action":
            Node = self.ParseVfrStatementAction(Value, ParentNode, Position)
        if Key == "date":
            Node = self.ParseVfrStatementDate(Value, ParentNode, Position)  #
        if Key == "numeric":
            Node = self.ParseVfrStatementNumeric(Value, ParentNode, Position)
        if Key == "oneof":
            Node = self.ParseVfrStatementOneof(Value, ParentNode, Position)
        if Key == "string":
            Node = self.ParseVfrStatementString(Value, ParentNode, Position)
        if Key == "password":
            Node = self.ParseVfrStatementPassword(Value, ParentNode, Position)
        if Key == "orderedlist":
            Node = self.ParseVfrStatementOrderedList(Value, ParentNode, Position)
        if Key == "time":
            Node = self.ParseVfrStatementTime(Value, ParentNode, Position)
        return Node

    def ParseVfrStatementConditional(self, Key, Value, ParentNode, Position):
        if Key == "disableif":
            self.ParseVfrStatementDisableIfStat(Value, ParentNode, Position)
        if Key == "suppressif":
            self.ParseVfrStatementSuppressIfStat(Value, ParentNode, Position)
        if Key == "grayoutif":
            self.ParseVfrStatementGrayOutIfStat(Value, ParentNode, Position)  #
        if Key == "inconsistentif":
            self.ParseVfrStatementInconsistentIfStat(Value, ParentNode, Position)

    def ParseVfrStatementDisableIfStat(self, DisableIf, ParentNode, Position):
        DIObj = IfrDisableIf()
        Node = IfrTreeNode(EFI_IFR_DISABLE_IF_OP, DIObj, gFormPkg.StructToStream(DIObj.GetInfo()))
        ParentNode.insertChild(Node)
        self.ParserVfrStatementExpression(DisableIf["expression"].PostVal, Node)
        Node.Expression = DisableIf["expression"].PostVal
        Node.Condition = "disableif " + Node.Expression
        if "component" in DisableIf.keys():
            for Component in DisableIf["component"]:
                ((Key, Value),) = Component.items()
                self._ParseVfrStatementStatList(Key, Value, Node, Position)
        self.InsertEndNode(Node)

    def ParseVfrStatementSuppressIfStat(self, SuppressIf, ParentNode, Position):
        SIObj = IfrSuppressIf()
        Node = IfrTreeNode(EFI_IFR_SUPPRESS_IF_OP, SIObj, gFormPkg.StructToStream(SIObj.GetInfo()))
        ParentNode.insertChild(Node)
        self.ParserVfrStatementExpression(SuppressIf["expression"].PostVal, Node)
        Node.Expression = SuppressIf["expression"].PostVal
        Node.Condition = "suppressif " + Node.Expression
        if "component" in SuppressIf.keys():
            for Component in SuppressIf["component"]:
                ((Key, Value),) = Component.items()
                self._ParseVfrStatementStatList(Key, Value, Node, Position)
        self.InsertEndNode(Node)

    def ParseVfrStatementGrayOutIfStat(self, GrayOutIf, ParentNode, Position):
        GOIObj = IfrGrayOutIf()
        Node = IfrTreeNode(EFI_IFR_GRAY_OUT_IF_OP, GOIObj, gFormPkg.StructToStream(GOIObj.GetInfo()))
        ParentNode.insertChild(Node)
        self.ParserVfrStatementExpression(GrayOutIf["expression"].PostVal, Node)
        Node.Expression = GrayOutIf["expression"].PostVal
        Node.Condition = "grayoutif " + Node.Expression
        if "component" in GrayOutIf.keys():
            for Component in GrayOutIf["component"]:
                ((Key, Value),) = Component.items()
                self._ParseVfrStatementStatList(Key, Value, Node, Position)
        self.InsertEndNode(Node)

    def ParseVfrStatementInconsistentIfStat(self, InconsistentIf, ParentNode, Position):
        IIObj = IfrInconsistentIf()
        IIObj.SetError(InconsistentIf["prompt"].PostVal)
        Node = IfrTreeNode(EFI_IFR_INCONSISTENT_IF_OP, IIObj, gFormPkg.StructToStream(IIObj.GetInfo()))
        ParentNode.insertChild(Node)
        self.ParserVfrStatementExpression(InconsistentIf["expression"].PostVal, Node)
        Node.Expression = InconsistentIf["expression"].PostVal
        Node.Condition = "inconsistentif " + Node.Expression
        if "component" in InconsistentIf.keys():
            for Component in InconsistentIf["component"]:
                ((Key, Value),) = Component.items()
                self._ParseVfrStatementStatList(Key, Value, Node, Position)
        self.InsertEndNode(Node)

    def _ParseVfrStatementStatList(self, Key, Value, ParentNode, Position):
        # print(Key)
        if Key in vfrStatementStat:
            self.ParseVfrStatementStat(Key, Value, ParentNode, Position)
        elif Key in vfrStatementQuestions:
            self.ParseVfrStatementQuestions(Key, Value, ParentNode, Position)
        elif Key in vfrStatementConditional:
            self.ParseVfrStatementConditional(Key, Value, ParentNode, Position)
        elif Key == "label":
            self.ParseVfrStatementLabel(Value, ParentNode)
        elif Key == "guidop":
            self.ParseVfrStatementExtension(Value, ParentNode)
        # vfrStatementInvalid

    def ParseVfrStatementTime(self, Time, ParentNode, Position):  # only support one condition
        TObj = IfrTime()
        Node = IfrTreeNode(EFI_IFR_TIME_OP, TObj)
        if "hour" not in Time.keys():
            self._ParseVfrQuestionHeader(TObj, Time, EFI_QUESION_TYPE.QUESTION_TIME)
            self.ErrorHandler(TObj.SetFlags(EFI_IFR_QUESTION_FLAG_DEFAULT, self._ParseTimeFlags(Time)))
            Node.Buffer = gFormPkg.StructToStream(TObj.GetInfo())
            self.SetPosition(Node, Position)
            ParentNode.insertChild(Node)
            if "component" in Time.keys():
                self._ParseVfrStatementQuestionOptionList(Time["component"], Node)
        else:
            self.ErrorHandler(TObj.SetFlags(EFI_IFR_QUESTION_FLAG_DEFAULT, self._ParseTimeFlags(Time)))
            QId, _ = self.VfrQuestionDB.RegisterOldTimeQuestion(
                Time["hour"].PostVal,
                Time["minute"].PostVal,
                Time["second"].PostVal,
                EFI_QUESTION_ID_INVALID,
                gFormPkg,
            )
            TObj.SetQuestionId(QId)
            TObj.SetFlags(EFI_IFR_QUESTION_FLAG_DEFAULT, QF_TIME_STORAGE_TIME)
            TObj.SetPrompt(Time["prompt"].PostVal)
            TObj.SetHelp(Time["help"].PostVal)
            Node.Buffer = gFormPkg.StructToStream(TObj.GetInfo())
            self.SetPosition(Node, Position)
            ParentNode.insertChild(Node)
            DefaultTime = EFI_HII_TIME()
            if "default_hour" in Time.keys():
                DefaultTime.Hour = Time["default_hour"].PostVal
                if DefaultTime.Hour > 23:
                    self.ErrorHandler(
                        VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                        "Hour default value must be between 0 and 23.",
                    )
            if "default_minute" in Time.keys():
                DefaultTime.Minute = Time["default_minute"].PostVal
                if DefaultTime.Minute > 59:
                    self.ErrorHandler(
                        VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                        "Minute default value must be between 0 and 59.",
                    )
            if "default_second" in Time.keys():
                DefaultTime.Second = Time["default_second"].PostVal
                if DefaultTime.Second > 59:
                    self.ErrorHandler(
                        VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                        "Second default value must be between 0 and 59.",
                    )
            DefaultObj = IfrDefault(
                EFI_IFR_TYPE_TIME,
                [DefaultTime],
                EFI_HII_DEFAULT_CLASS_STANDARD,
                EFI_IFR_TYPE_TIME,
            )
            DefaultNode = IfrTreeNode(
                EFI_IFR_DEFAULT_OP,
                DefaultObj,
                gFormPkg.StructToStream(DefaultObj.GetInfo()),
            )
            Node.insertChild(DefaultNode)
            if "component" in Time.keys():
                for Component in Time["component"]:
                    ((_, Value),) = Component.items()
                    self.ParseVfrStatementInconsistentIf(Value, Node)
        self.InsertEndNode(Node)

        return Node

    def _ParseTimeFlags(self, FlagsDict):
        LFlags = 0
        if "flags" in FlagsDict.keys():
            FlagList = self._ToList(FlagsDict["flags"].PostVal)
            for Flag in FlagList:
                if Flag == "HOUR_SUPPRESS":
                    LFlags |= 0x01
                elif Flag == "MINUTE_SUPPRESS":
                    LFlags |= 0x02
                elif Flag == "SECOND_SUPPRESS":
                    LFlags |= 0x04
                elif Flag == "STORAGE_NORMAL":
                    LFlags |= 0x00
                elif Flag == "STORAGE_TIME":
                    LFlags |= 0x10
                elif Flag == "STORAGE_WAKEUP":
                    LFlags |= 0x20
                else:
                    LFlags |= Flag
        return LFlags

    def _ParseMinMaxTimeStepDefault(self, Time):
        pass

    def ParseVfrStatementOrderedList(self, OrderedList, ParentNode, Position):
        OLObj = IfrOrderedList()
        self.CurrentQuestion = OLObj
        self.IsOrderedList = True
        self._ParseVfrQuestionHeader(OLObj, OrderedList, EFI_QUESION_TYPE.QUESTION_NORMAL)
        VarArraySize = self.GetCurArraySize()
        if VarArraySize > 0xFF:
            OLObj.SetMaxContainers(0xFF)
        else:
            OLObj.SetMaxContainers(VarArraySize)

        if "maxcontainers" in OrderedList.keys():
            MaxContainers = OrderedList["maxcontainers"].PostVal
            if MaxContainers > 0xFF:
                self.ErrorHandler(
                    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                    "OrderedList MaxContainers takes only one byte, which can't be larger than 0xFF.",
                )
            elif VarArraySize != 0 and MaxContainers > VarArraySize:
                self.ErrorHandler(
                    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                    "OrderedList MaxContainers can't be larger than the max number of elements in array.",
                )
            OLObj.SetMaxContainers(MaxContainers)

        HFlags, LFlags = self._ParseOrderedListFlags(OrderedList)
        self.ErrorHandler(OLObj.SetFlags(HFlags, LFlags))
        Node = IfrTreeNode(EFI_IFR_ORDERED_LIST_OP, OLObj, gFormPkg.StructToStream(OLObj.GetInfo()))
        self.SetPosition(Node, Position)
        ParentNode.insertChild(Node)
        if "component" in OrderedList.keys():
            self._ParseVfrStatementQuestionOptionList(OrderedList["component"], Node)
        self.InsertEndNode(Node)
        self.IsOrderedList = False
        return Node

    def _ParseOrderedListFlags(self, FlagsDict):
        LFlags = 0
        HFlags = 0
        if "flags" in FlagsDict.keys():
            FlagList = self._ToList(FlagsDict["flags"].PostVal)
            for Flag in FlagList:
                if Flag in QuestionheaderFlagsField:
                    HFlags |= self._ParseQuestionheaderFlagsField(Flag)
                elif Flag == "UNIQUE":
                    LFlags |= 0x01
                elif Flag == "NOEMPTY":
                    LFlags |= 0x02
                elif Flag != 0:
                    self.ErrorHandler(
                        VfrReturnCode.VFR_RETURN_UNSUPPORTED,
                        "CheckBox doesnot support flag!=0",
                    )
        return HFlags, LFlags

    def ParseVfrStatementCheckBox(self, CheckBox, ParentNode, Position):
        CBObj = IfrCheckBox()
        Node = IfrTreeNode(EFI_IFR_CHECKBOX_OP)
        self.CurrentQuestion = CBObj
        self.IsCheckBoxOp = True
        HasGuidNode = False
        self._ParseVfrQuestionBaseInfo(CBObj, CheckBox, EFI_QUESION_TYPE.QUESTION_NORMAL)

        # Create a GUID opcode to wrap the checkbox opcode, if it refer to bit varstore.
        if self.CurrQestVarInfo.IsBitVar:
            HasGuidNode = True
            GuidObj = IfrGuid(0)
            GuidObj.SetGuid(EDKII_IFR_BIT_VARSTORE_GUID)
            GuidObj.SetScope(1)  # Position
            GuidNode = IfrTreeNode(EFI_IFR_GUID_OP, GuidObj, gFormPkg.StructToStream(GuidObj.GetInfo()))
            ParentNode.insertChild(GuidNode)
            GuidNode.insertChild(Node)
            self.InsertEndNode(GuidNode)

        self._ParseVfrStatementHeader(CBObj, CheckBox)
        # check dataType
        if self.CurrQestVarInfo.VarType == EFI_IFR_TYPE_OTHER:
            self.CurrQestVarInfo.VarType = EFI_IFR_TYPE_BOOLEAN

        if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            # Check whether the question refers to a bit field, if yes. create a Guid to indicate the question refers to a bit field.
            if self.CurrQestVarInfo.IsBitVar:
                _, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.CurrQestVarInfo.VarType)
                self.ErrorHandler(ReturnCode, "CheckBox varid is not the valid data type")
                if gVfrDataStorage.GetVarStoreType(self.CurrQestVarInfo.VarStoreId) == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER_BITS \
                    and self.CurrQestVarInfo.VarTotalSize != 1:
                    self.ErrorHandler(
                        VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                        "CheckBox varid only occupy 1 bit in Bit Varstore",
                    )
                else:
                    Size, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.CurrQestVarInfo.VarType)
                    self.ErrorHandler(ReturnCode, "CheckBox varid is not the valid data type")
                    if Size != 0 and Size != self.CurrQestVarInfo.VarTotalSize:
                        self.ErrorHandler(
                            VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                            "CheckBox varid doesn't support array",
                        )
                    elif gVfrDataStorage.GetVarStoreType(self.CurrQestVarInfo.VarStoreId) == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER \
                        and self.CurrQestVarInfo.VarTotalSize != sizeof(
                        ctypes.c_bool
                    ):
                        self.ErrorHandler(
                            VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                            "CheckBox varid only support BOOLEAN data type",
                        )

        if "flags" in CheckBox.keys():
            HFlags, LFlags = self._ParseCheckBoxFlags(CheckBox)
            CBObj.SetFlags(HFlags, LFlags)
            if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
                VarStoreName, ReturnCode = gVfrDataStorage.GetVarStoreName(self.CurrQestVarInfo.VarStoreId)
                # self.CompareErrorHandler(ReturnCode, VfrReturnCode.VFR_RETURN_SUCCESS, Line, ctx.L.text, "Failed to retrieve varstore name")

                VarStoreGuid = gVfrDataStorage.GetVarStoreGuid(self.CurrQestVarInfo.VarStoreId)
                self.Value = True
                if CBObj.GetFlags() & 0x01:
                    self._CheckDuplicateDefaultValue(EFI_HII_DEFAULT_CLASS_STANDARD)
                    ReturnCode = gVfrDefaultStore.BufferVarStoreAltConfigAdd(
                        EFI_HII_DEFAULT_CLASS_STANDARD,
                        self.CurrQestVarInfo,
                        VarStoreName,
                        VarStoreGuid,
                        self.CurrQestVarInfo.VarType,
                        self.Value,
                    )
                    # self.CompareErrorHandler(ReturnCode, VfrReturnCode.VFR_RETURN_SUCCESS, Line, ctx.L.text, "No standard default storage found")
                if CBObj.GetFlags() & 0x02:
                    self._CheckDuplicateDefaultValue(EFI_HII_DEFAULT_CLASS_MANUFACTURING)
                    ReturnCode = gVfrDefaultStore.BufferVarStoreAltConfigAdd(
                        EFI_HII_DEFAULT_CLASS_MANUFACTURING,
                        self.CurrQestVarInfo,
                        VarStoreName,
                        VarStoreGuid,
                        self.CurrQestVarInfo.VarType,
                        self.Value,
                    )
                    # self.CompareErrorHandler(ReturnCode, VfrReturnCode.VFR_RETURN_SUCCESS, Line, ctx.L.text, "No manufacturing default storage found")

        if "key" in CheckBox.keys():
            self.AssignQuestionKey(CBObj, CheckBox["key"].PostVal)

        Node.Data = CBObj
        Node.Buffer = gFormPkg.StructToStream(CBObj.GetInfo())
        self.SetPosition(Node, Position)
        if HasGuidNode == False:
            ParentNode.insertChild(Node)
        if "component" in CheckBox.keys():
            self._ParseVfrStatementQuestionOptionList(CheckBox["component"], Node)
        self.IsCheckBoxOp = False
        self.InsertEndNode(Node)
        return Node

    def _ParseCheckBoxFlags(self, FlagsDict):
        LFlags = 0
        HFlags = 0
        if "flags" in FlagsDict.keys():
            FlagList = self._ToList(FlagsDict["flags"].PostVal)
            for Flag in FlagList:
                if Flag in QuestionheaderFlagsField:
                    HFlags |= self._ParseQuestionheaderFlagsField(Flag)
                elif Flag == "DEFAULT":
                    self.ErrorHandler(
                        VfrReturnCode.VFR_RETURN_UNSUPPORTED,
                        "CheckBox doesnot support 'DEFAULT' flag",
                    )
                elif Flag == "MANUFACTURING":
                    self.ErrorHandler(
                        VfrReturnCode.VFR_RETURN_UNSUPPORTED,
                        "CheckBox doesnot support 'MANUFACTURING' flag",
                    )
                elif Flag == "CHECKBOX_DEFAULT":
                    LFlags |= 0x01
                elif Flag == "CHECKBOX_DEFAULT_MFG":
                    LFlags |= 0x02
                elif Flag != 0:
                    self.ErrorHandler(
                        VfrReturnCode.VFR_RETURN_UNSUPPORTED,
                        "CheckBox doesnot support flag!=0",
                    )
        return HFlags, LFlags

    def ParseVfrStatementAction(self, Action, ParentNode, Position):
        AObj = IfrAction()
        self._ParseVfrQuestionHeader(AObj, Action, EFI_QUESION_TYPE.QUESTION_NORMAL)
        Config = Action["config"].PostVal
        AObj.SetQuestionConfig(Config)
        # No handler for Flags
        Node = IfrTreeNode(EFI_IFR_ACTION_OP, AObj, gFormPkg.StructToStream(AObj.GetInfo()))
        self.SetPosition(Node, Position)
        ParentNode.insertChild(Node)
        if "component" in Action.keys():
            for Component in Action["component"]:
                ((Key, Value),) = Component.items()
                self._ParseVfrStatementQuestionTag(Key, Value, Node)
        self.InsertEndNode(Node)
        return Node

    def ParseVfrStatementDate(self, Date, ParentNode, Position):
        DObj = IfrDate()
        Node = IfrTreeNode(EFI_IFR_DATE_OP, DObj)
        if "year" not in Date.keys():
            self._ParseVfrQuestionHeader(DObj, Date, EFI_QUESION_TYPE.QUESTION_DATE)
            self.ErrorHandler(DObj.SetFlags(EFI_IFR_QUESTION_FLAG_DEFAULT, self._ParseDateFlags(Date)))
            Node.Buffer = gFormPkg.StructToStream(DObj.GetInfo())
            self.SetPosition(Node, Position)
            ParentNode.insertChild(Node)
            if "component" in Date.keys():
                self._ParseVfrStatementQuestionOptionList(Date["component"], Node)
        else:
            self.ErrorHandler(DObj.SetFlags(EFI_IFR_QUESTION_FLAG_DEFAULT, self._ParseDateFlags(Date)))
            QId, _ = self.VfrQuestionDB.RegisterOldDateQuestion(
                Date["year"].PostVal,
                Date["month"].PostVal,
                Date["day"].PostVal,
                EFI_QUESTION_ID_INVALID,
                gFormPkg,
            )
            DObj.SetQuestionId(QId)
            DObj.SetFlags(EFI_IFR_QUESTION_FLAG_DEFAULT, QF_DATE_STORAGE_TIME)
            DObj.SetPrompt(Date["prompt"].PostVal)
            DObj.SetHelp(Date["help"].PostVal)
            Node.Buffer = gFormPkg.StructToStream(DObj.GetInfo())
            self.SetPosition(Node, Position)
            ParentNode.insertChild(Node)
            DefaultTDate = EFI_HII_DATE()
            if "default_year" in Date.keys():
                DefaultTDate.Year = Date["default_year"].PostVal
                if DefaultTDate.Year > Date["max_year"].PostVal or DefaultTDate.Year < Date["min_year"].PostVal:
                    self.ErrorHandler(
                        VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                        "Hour default value must be between {} and {}.".format(Date["min_year"].PostVal, Date["max_year"].PostVal),
                    )
            if "default_month" in Date.keys():
                DefaultTDate.Month = Date["default_month"].PostVal
                if DefaultTDate.Month < 1 or DefaultTDate.Month > 12:
                    self.ErrorHandler(
                        VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                        "Minute default value must be between 1 and 12.",
                    )
            if "default_day" in Date.keys():
                DefaultTDate.Day = Date["default_day"].PostVal
                if DefaultTDate.Day < 1 or DefaultTDate.Day > 31:
                    self.ErrorHandler(
                        VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                        "Second default value must be between 1 and 31.",
                    )
            DefaultObj = IfrDefault(
                EFI_IFR_TYPE_DATE,
                [DefaultTDate],
                EFI_HII_DEFAULT_CLASS_STANDARD,
                EFI_IFR_TYPE_DATE,
            )
            DefaultNode = IfrTreeNode(
                EFI_IFR_DEFAULT_OP,
                DefaultObj,
                gFormPkg.StructToStream(DefaultObj.GetInfo()),
            )
            Node.insertChild(DefaultNode)
            if "component" in Date.keys():
                for Component in Date["component"]:
                    ((_, Value),) = Component.items()
                    self.ParseVfrStatementInconsistentIf(Value, Node)
        self.InsertEndNode(Node)
        return Node

    def _ParseDateFlags(self, FlagsDict):
        LFlags = 0
        if "flags" in FlagsDict.keys():
            FlagList = self._ToList(FlagsDict["flags"].PostVal)
            for Flag in FlagList:
                if Flag == "YEAR_SUPPRESS":
                    LFlags |= 0x01
                elif Flag == "MONTH_SUPPRESS":
                    LFlags |= 0x02
                elif Flag == "DAY_SUPPRESS":
                    LFlags |= 0x04
                elif Flag == "STORAGE_NORMAL":
                    LFlags |= 0x00
                elif Flag == "STORAGE_TIME":
                    LFlags |= 0x010
                elif Flag == "STORAGE_WAKEUP":
                    LFlags |= 0x020
        return LFlags

    def ParseVfrStatementNumeric(self, Numeric, ParentNode, Position):
        NObj = IfrNumeric(EFI_IFR_TYPE_NUM_SIZE_64)
        Node = IfrTreeNode(EFI_IFR_NUMERIC_OP, NObj)
        self.CurrentQuestion = Node.Data
        self.CurrentMinMaxData = Node.Data
        UpdateVarType = False
        self._ParseVfrQuestionBaseInfo(NObj, Numeric, EFI_QUESION_TYPE.QUESTION_NORMAL)
        HasGuidNode = False

        # Create a GUID opcode to wrap the numeric opcode, if it refer to bit varstore.
        if self.CurrQestVarInfo.IsBitVar:
            HasGuidNode = True
            GuidObj = IfrGuid(0)
            GuidObj.SetGuid(EDKII_IFR_BIT_VARSTORE_GUID)
            GuidObj.SetScope(1)  # pos
            GuidNode = IfrTreeNode(EFI_IFR_GUID_OP, GuidObj, gFormPkg.StructToStream(GuidObj.GetInfo()))
            ParentNode.insertChild(GuidNode)
            GuidNode.insertChild(Node)
            self.InsertEndNode(GuidNode)

        self._ParseVfrStatementHeader(NObj, Numeric)
        # check data type
        if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            if self.CurrQestVarInfo.IsBitVar:
                LFlags = EDKII_IFR_NUMERIC_SIZE_BIT & self.CurrQestVarInfo.VarTotalSize
                self.ErrorHandler(NObj.SetFlagsForBitField(NObj.GetQFlags(), LFlags))
            else:
                DataTypeSize, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.CurrQestVarInfo.VarType)
                self.ErrorHandler(ReturnCode, "Numeric varid is not the valid data type")
                if DataTypeSize != 0 and DataTypeSize != self.CurrQestVarInfo.VarTotalSize:
                    self.ErrorHandler(
                        VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                        "Numeric varid doesn't support array",
                    )
                self.ErrorHandler(NObj.SetFlags(NObj.GetQFlags(), self.CurrQestVarInfo.VarType))

        if "flags" in Numeric.keys():
            HFlags, LFlags, IsDisplaySpecified, UpdateVarType = self._ParseNumericFlags(Numeric)
            if self.CurrQestVarInfo.IsBitVar:
                self.ErrorHandler(NObj.SetFlagsForBitField(HFlags, LFlags, IsDisplaySpecified))
            else:
                self.ErrorHandler(NObj.SetFlags(HFlags, LFlags, IsDisplaySpecified))

        if "key" in Numeric.keys():
            self.AssignQuestionKey(NObj, Numeric["key"].PostVal)

        if (self.CurrQestVarInfo.IsBitVar == False) and (self.CurrQestVarInfo.VarType not in BasicTypes):
            self.ErrorHandler(
                VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                "Numeric question only support UINT8, UINT16, UINT32 and UINT64 data type.",
            )

        # modify the data for namevalue
        if self.CurrQestVarInfo.VarType != EFI_IFR_TYPE_NUM_SIZE_64:
            if self.CurrQestVarInfo.IsBitVar:
                UpdatedNObj = IfrNumeric(EFI_IFR_TYPE_NUM_SIZE_32)
            else:
                UpdatedNObj = IfrNumeric(self.CurrQestVarInfo.VarType)
            UpdatedNObj.GetInfo().Question = NObj.GetInfo().Question
            UpdatedNObj.GetInfo().Flags = NObj.GetInfo().Flags
            UpdatedNObj.VarIdStr = NObj.VarIdStr
            UpdatedNObj.QName = NObj.QName
            NObj = UpdatedNObj
            Node.Data = NObj
            self.CurrentQuestion = Node.Data
            self.CurrentMinMaxData = Node.Data

        self._ParseVfrSetMinMaxStep(NObj, Numeric)

        if HasGuidNode == False:
            ParentNode.insertChild(Node)
        self.SetPosition(Node, Position)
        if "component" in Numeric.keys():
            self._ParseVfrStatementQuestionOptionList(Numeric["component"], Node)

        Node.Buffer = gFormPkg.StructToStream(NObj.GetInfo())
        self.InsertEndNode(Node)
        return Node

    def _ParseNumericFlags(self, FlagsDict):
        LFlags = 0
        VarStoreType = gVfrDataStorage.GetVarStoreType(self.CurrQestVarInfo.VarStoreId)
        HFlags = 0
        IsSetType = False
        IsDisplaySpecified = False
        UpdateVarType = False
        if "flags" in FlagsDict.keys():
            LFlags = self.CurrQestVarInfo.VarType & EFI_IFR_NUMERIC_SIZE
            FlagList = self._ToList(FlagsDict["flags"].PostVal)
            for Flag in FlagList:
                if Flag == "NUMERIC_SIZE_1":
                    if self.CurrQestVarInfo.IsBitVar == False:
                        LFlags = (LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_1
                        IsSetType = True
                    else:
                        self.ErrorHandler(
                            VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                            "Can not specify the size of the numeric value for BIT field",
                        )

                elif Flag == "NUMERIC_SIZE_2":
                    if self.CurrQestVarInfo.IsBitVar == False:
                        LFlags = (LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_2
                        IsSetType = True
                    else:
                        self.ErrorHandler(
                            VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                            "Can not specify the size of the numeric value for BIT field",
                        )

                elif Flag == "NUMERIC_SIZE_4":
                    if self.CurrQestVarInfo.IsBitVar == False:
                        LFlags = (LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_4
                        IsSetType = True
                    else:
                        self.ErrorHandler(
                            VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                            "Can not specify the size of the numeric value for BIT field",
                        )

                elif Flag == "NUMERIC_SIZE_8":
                    if self.CurrQestVarInfo.IsBitVar == False:
                        LFlags = (LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_8
                        IsSetType = True
                    else:
                        self.ErrorHandler(
                            VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                            "Can not specify the size of the numeric value for BIT field",
                        )

                elif Flag == "DISPLAY_INT_DEC":
                    if self.CurrQestVarInfo.IsBitVar == False:
                        LFlags = (LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_INT_DEC
                    else:
                        LFlags = (LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_INT_DEC_BIT
                    IsDisplaySpecified = True

                elif Flag == "DISPLAY_UINT_DEC":
                    if self.CurrQestVarInfo.IsBitVar == False:
                        LFlags = (LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_UINT_DEC
                    else:
                        LFlags = (LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_UINT_DEC_BIT
                    IsDisplaySpecified = True

                elif Flag == "DISPLAY_UINT_HEX":
                    if self.CurrQestVarInfo.IsBitVar == False:
                        LFlags = (LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_UINT_HEX
                    else:
                        LFlags = (LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_UINT_HEX_BIT
                    IsDisplaySpecified = True

                elif Flag in QuestionheaderFlagsField:
                    HFlags |= self._ParseQuestionheaderFlagsField(Flag)

                elif Flag != 0:
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED)

        VarType = self.CurrQestVarInfo.VarType
        if self.CurrQestVarInfo.IsBitVar == False:
            if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
                if VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER\
                or VarStoreType == EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_EFI:
                    if self.CurrQestVarInfo.VarType != (LFlags & EFI_IFR_NUMERIC_SIZE):
                        self.ErrorHandler(
                            VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                            "Numeric Flag is not same to Numeric VarData type",
                        )
                else:
                    # update data type for name/value store
                    self.CurrQestVarInfo.VarType = LFlags & EFI_IFR_NUMERIC_SIZE
                    Size, _ = gVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.CurrQestVarInfo.VarType)
                    self.CurrQestVarInfo.VarTotalSize = Size
            elif IsSetType:
                self.CurrQestVarInfo.VarType = LFlags & EFI_IFR_NUMERIC_SIZE

        elif self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            LFlags &= EDKII_IFR_DISPLAY_BIT
            LFlags |= EDKII_IFR_NUMERIC_SIZE_BIT & self.CurrQestVarInfo.VarTotalSize

        if VarType != self.CurrQestVarInfo.VarType:
            UpdateVarType = True

        return HFlags, LFlags, IsDisplaySpecified, UpdateVarType

    def _ParseVfrSetMinMaxStep(self, OpObj, QDict):
        IntDecStyle = False
        if ((self.CurrQestVarInfo.IsBitVar) and (OpObj.GetOpCode() == EFI_IFR_NUMERIC_OP)
            and ((OpObj.GetNumericFlags() & EDKII_IFR_DISPLAY_BIT) == 0)) or (
            (self.CurrQestVarInfo.IsBitVar == False) and (OpObj.GetOpCode() == EFI_IFR_NUMERIC_OP)
            and ((OpObj.GetNumericFlags() & EFI_IFR_DISPLAY) == 0)
        ):
            IntDecStyle = True
        MinNegative = False
        MaxNegative = False

        Min = QDict["minimum"].PostVal if "minimum" in QDict.keys() else 0
        Max = QDict["maximum"].PostVal if "maximum" in QDict.keys() else 0
        Step = QDict["step"].PostVal if "step" in QDict.keys() else 0
        if Min < 0:
            MinNegative = True
            Min = -Min

        if IntDecStyle == False and MinNegative == True:
            self.ErrorHandler(
                VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                "'-' can't be used when not in int decimal type.",
            )
        if self.CurrQestVarInfo.IsBitVar:
            if (IntDecStyle == False) and (Min > (1 << self.CurrQestVarInfo.VarTotalSize) - 1):  #
                self.ErrorHandler(
                    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                    "BIT type minimum can't small than 0, bigger than 2^BitWidth -1",
                )
            else:
                Type = self.CurrQestVarInfo.VarType
                if Type == EFI_IFR_TYPE_NUM_SIZE_64:
                    if IntDecStyle:
                        if MinNegative:
                            if Min > 0x8000000000000000:
                                self.ErrorHandler(
                                    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                                    "INT64 type minimum can't small than -0x8000000000000000, big than 0x7FFFFFFFFFFFFFFF",
                                )
                        else:
                            if Min > 0x7FFFFFFFFFFFFFFF:
                                self.ErrorHandler(
                                    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                                    "INT64 type minimum can't small than -0x8000000000000000, big than 0x7FFFFFFFFFFFFFFF",
                                )
                    if MinNegative:
                        Min = ~Min + 1

                if Type == EFI_IFR_TYPE_NUM_SIZE_32:
                    if IntDecStyle:
                        if MinNegative:
                            if Min > 0x80000000:
                                self.ErrorHandler(
                                    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                                    "INT32 type minimum can't small than -0x80000000, big than 0x7FFFFFFF",
                                )
                        else:
                            if Min > 0x7FFFFFFF:
                                self.ErrorHandler(
                                    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                                    "INT32 type minimum can't small than -0x80000000, big than 0x7FFFFFFF",
                                )
                    if MinNegative:
                        Min = ~Min + 1

                if Type == EFI_IFR_TYPE_NUM_SIZE_16:
                    if IntDecStyle:
                        if MinNegative:
                            if Min > 0x8000:
                                self.ErrorHandler(
                                    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                                    "INT16 type minimum can't small than -0x8000, big than 0x7FFF",
                                )
                        else:
                            if Min > 0x7FFF:
                                self.ErrorHandler(
                                    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                                    "INT16 type minimum can't small than -0x8000, big than 0x7FFF",
                                )
                    if MinNegative:
                        Min = ~Min + 1

                if Type == EFI_IFR_TYPE_NUM_SIZE_8:
                    if IntDecStyle:
                        if MinNegative:
                            if Min > 0x80:
                                self.ErrorHandler(
                                    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                                    "INT8 type minimum can't small than -0x80, big than 0x7F",
                                )
                        else:
                            if Min > 0x7F:
                                self.ErrorHandler(
                                    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                                    "INT8 type minimum can't small than -0x80, big than 0x7F",
                                )
                    if MinNegative:
                        Min = ~Min + 1
        if Max < 0:
            MaxNegative = True
            Max = -Max

        if IntDecStyle == False and MaxNegative == True:
            self.ErrorHandler(
                VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                " '-' can't be used when not in int decimal type.",
            )
        if self.CurrQestVarInfo.IsBitVar:
            if (IntDecStyle == False) and (Max > (1 << self.CurrQestVarInfo.VarTotalSize) - 1):
                self.ErrorHandler(
                    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                    "BIT type maximum can't be bigger than 2^BitWidth -1",
                )
            else:
                Type = self.CurrQestVarInfo.VarType
                if Type == EFI_IFR_TYPE_NUM_SIZE_64:
                    if IntDecStyle:
                        if MaxNegative:
                            if Max > 0x8000000000000000:
                                self.ErrorHandler(
                                    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                                    "INT64 type minimum can't small than -0x8000000000000000, big than 0x7FFFFFFFFFFFFFFF",
                                )
                        else:
                            if Max > 0x7FFFFFFFFFFFFFFF:
                                self.ErrorHandler(
                                    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                                    "INT64 type minimum can't small than -0x8000000000000000, big than 0x7FFFFFFFFFFFFFFF",
                                )
                    if MaxNegative:
                        Max = ~Max + 1

                    if Max < Min:  #
                        self.ErrorHandler(
                            VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                            "Maximum can't be less than Minimum",
                        )

                if Type == EFI_IFR_TYPE_NUM_SIZE_32:
                    if IntDecStyle:
                        if MaxNegative:
                            if Max > 0x80000000:
                                self.ErrorHandler(
                                    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                                    "INT32 type minimum can't small than -0x80000000, big than 0x7FFFFFFF",
                                )
                        else:
                            if Max > 0x7FFFFFFF:
                                self.ErrorHandler(
                                    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                                    "INT32 type minimum can't small than -0x80000000, big than 0x7FFFFFFF",
                                )
                    if MaxNegative:
                        Max = ~Max + 1

                    if Max < Min:  #
                        self.ErrorHandler(
                            VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                            "Maximum can't be less than Minimum",
                        )

                if Type == EFI_IFR_TYPE_NUM_SIZE_16:
                    if IntDecStyle:
                        if MaxNegative:
                            if Max > 0x8000:
                                self.ErrorHandler(
                                    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                                    "INT16 type minimum can't small than -0x8000, big than 0x7FFF",
                                )
                        else:
                            if Max > 0x7FFF:
                                self.ErrorHandler(
                                    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                                    "INT16 type minimum can't small than -0x8000, big than 0x7FFF",
                                )
                    if MaxNegative:
                        Max = ~Max + 1

                    if Max < Min:  #
                        self.ErrorHandler(
                            VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                            "Maximum can't be less than Minimum",
                        )

                if Type == EFI_IFR_TYPE_NUM_SIZE_8:
                    if IntDecStyle:
                        if MaxNegative:
                            if Max > 0x80:
                                self.ErrorHandler(
                                    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                                    "INT8 type minimum can't small than -0x80, big than 0x7F",
                                )
                        else:
                            if Max > 0x7F:
                                self.ErrorHandler(
                                    VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                                    "INT8 type minimum can't small than -0x80, big than 0x7F",
                                )
                    if MaxNegative:
                        Max = ~Max + 1

                    if Max < Min:  #
                        self.ErrorHandler(
                            VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                            "Maximum can't be less than Minimum",
                        )

        OpObj.SetMinMaxStepData(Min, Max, Step)

    def ParseVfrStatementOneof(self, OneOf, ParentNode, Position):
        OObj = IfrOneOf(EFI_IFR_TYPE_NUM_SIZE_64)
        Node = IfrTreeNode(EFI_IFR_ONE_OF_OP, OObj)
        self.CurrentQuestion = Node.Data
        self.CurrentMinMaxData = Node.Data
        UpdateVarType = False
        self._ParseVfrQuestionBaseInfo(OObj, OneOf, EFI_QUESION_TYPE.QUESTION_NORMAL)
        HasGuidNode = False

        # Create a GUID opcode to wrap the numeric opcode, if it refer to bit varstore.
        if self.CurrQestVarInfo.IsBitVar:
            HasGuidNode = True
            GuidObj = IfrGuid(0)
            GuidObj.SetGuid(EDKII_IFR_BIT_VARSTORE_GUID)
            GuidObj.SetScope(1)  # pos
            GuidNode = IfrTreeNode(EFI_IFR_GUID_OP, GuidObj, gFormPkg.StructToStream(GuidObj.GetInfo()))
            ParentNode.insertChild(GuidNode)
            GuidNode.insertChild(Node)
            self.InsertEndNode(GuidNode)

        self._ParseVfrStatementHeader(OObj, OneOf)

        # check data type
        if self.CurrQestVarInfo.VarStoreId != EFI_VARSTORE_ID_INVALID:
            if self.CurrQestVarInfo.IsBitVar:
                LFlags = EDKII_IFR_NUMERIC_SIZE_BIT & self.CurrQestVarInfo.VarTotalSize
                self.ErrorHandler(OObj.SetFlagsForBitField(OObj.GetQFlags(), LFlags))
            else:
                DataTypeSize, ReturnCode = gVfrVarDataTypeDB.GetDataTypeSizeByDataType(self.CurrQestVarInfo.VarType)
                self.ErrorHandler(ReturnCode, "OneOf varid is not the valid data type")
                if DataTypeSize != 0 and DataTypeSize != self.CurrQestVarInfo.VarTotalSize:
                    self.ErrorHandler(
                        VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                        "OneOf varid doesn't support array",
                    )
                self.ErrorHandler(OObj.SetFlags(OObj.GetQFlags(), self.CurrQestVarInfo.VarType))

        if "flags" in OneOf.keys():
            HFlags, LFlags, _, UpdateVarType = self._ParseNumericFlags(OneOf)

            if self.CurrQestVarInfo.IsBitVar:
                self.ErrorHandler(OObj.SetFlagsForBitField(HFlags, LFlags))
            else:
                self.ErrorHandler(OObj.SetFlags(HFlags, LFlags))

        if (self.CurrQestVarInfo.IsBitVar == False) and (self.CurrQestVarInfo.VarType not in BasicTypes):
            print(f"wrong error {self.Options.InputFileName}: {self.Options.YamlOutputFileName}")
            self.ErrorHandler(
                VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                f"wrong error {self.Options.InputFileName}: {self.Options.YamlOutputFileName}",
            )
            self.ErrorHandler(
                VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                "OneOf question only support UINT8, UINT16, UINT32 and UINT64 data type.",
            )

        # modify the data Vartype for NameValue
        if self.CurrQestVarInfo.VarType != EFI_IFR_TYPE_NUM_SIZE_64:
            if self.CurrQestVarInfo.IsBitVar:
                UpdatedOObj = IfrOneOf(EFI_IFR_TYPE_NUM_SIZE_32)
            else:
                UpdatedOObj = IfrOneOf(self.CurrQestVarInfo.VarType)
            UpdatedOObj.GetInfo().Question = OObj.GetInfo().Question
            UpdatedOObj.GetInfo().Flags = OObj.GetInfo().Flags
            UpdatedOObj.VarIdStr = OObj.VarIdStr
            UpdatedOObj.QName = OObj.QName
            OObj = UpdatedOObj
            Node.Data = OObj
            self.CurrentQuestion = Node.Data
            self.CurrentMinMaxData = Node.Data

        self._ParseVfrSetMinMaxStep(OObj, OneOf)
        if not HasGuidNode:
            ParentNode.insertChild(Node)
        self.SetPosition(Node, Position)
        if "component" in OneOf.keys():
            self._ParseVfrStatementQuestionOptionList(OneOf["component"], Node)
        Node.Buffer = gFormPkg.StructToStream(OObj.GetInfo())
        self.InsertEndNode(Node)
        return Node

    def ParseVfrStatementString(self, Str, ParentNode, Position):
        self.IsStringOp = True
        SObj = IfrString()
        self.CurrentQuestion = SObj
        self._ParseVfrQuestionHeader(SObj, Str, EFI_QUESION_TYPE.QUESTION_NORMAL)
        HFlags, LFlags = self._ParseStringFlags(Str)
        self.ErrorHandler(SObj.SetFlags(HFlags, LFlags))
        if "key" in Str.keys():
            self.AssignQuestionKey(SObj, Str["key"].PostVal)

        StringMinSize = Str["minsize"].PostVal
        StringMaxSize = Str["maxsize"].PostVal
        VarArraySize = self.GetCurArraySize()
        if StringMinSize > 0xFF:
            self.ErrorHandler(
                VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                "String MinSize takes only one byte, which can't be larger than 0xFF.",
            )
        if VarArraySize != 0 and StringMinSize > VarArraySize:
            self.ErrorHandler(
                VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                "String MinSize can't be larger than the max number of elements in string array.",
            )
        SObj.SetMinSize(StringMinSize)

        if StringMaxSize > 0xFF:
            self.ErrorHandler(
                VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                "String MaxSize takes only one byte, which can't be larger than 0xFF.",
            )
        elif VarArraySize != 0 and StringMaxSize > VarArraySize:
            self.ErrorHandler(
                VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                "String MaxSize can't be larger than the max number of elements in string array.",
            )
        elif StringMaxSize < StringMinSize:
            self.ErrorHandler(
                VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                "String MaxSize can't be less than String MinSize.",
            )
        SObj.SetMaxSize(StringMaxSize)
        Node = IfrTreeNode(EFI_IFR_STRING_OP, SObj, gFormPkg.StructToStream(SObj.GetInfo()))
        self.SetPosition(Node, Position)
        ParentNode.insertChild(Node)
        if "component" in Str.keys():
            self._ParseVfrStatementQuestionOptionList(Str["component"], Node)
        self.InsertEndNode(Node)
        self.IsStringOp = False
        return Node

    def _ParseStringFlags(self, FlagsDict):
        HFlags = 0
        LFlags = 0
        if "flags" in FlagsDict.keys():
            FlagList = self._ToList(FlagsDict["flags"].PostVal)
            for Flag in FlagList:
                if Flag in QuestionheaderFlagsField:
                    HFlags |= self._ParseQuestionheaderFlagsField(Flag)
                elif Flag == "MULTI_LINE":
                    LFlags |= 0x01
                elif Flag != 0:
                    self.ErrorHandler(VfrReturnCode.VFR_RETURN_UNSUPPORTED)
        return HFlags, LFlags

    def ParseVfrStatementPassword(self, Password, ParentNode, Position):
        PObj = IfrPassword()
        self.CurrentQuestion = PObj
        self._ParseVfrQuestionHeader(PObj, Password, EFI_QUESION_TYPE.QUESTION_NORMAL)

        self.ErrorHandler(PObj.SetFlags(self._ParseStatementStatFlags(Password)))

        if "key" in Password.keys():
            self.AssignQuestionKey(PObj, Password["key"].PostVal)

        PassWordMinSize = Password["minsize"].PostVal
        PasswordMaxSize = Password["maxsize"].PostVal
        VarArraySize = self.GetCurArraySize()
        if PassWordMinSize > 0xFF:
            self.ErrorHandler(
                VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                "String MinSize takes only one byte, which can't be larger than 0xFF.",
            )
        if VarArraySize != 0 and PassWordMinSize > VarArraySize:
            self.ErrorHandler(
                VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                "String MinSize can't be larger than the max number of elements in string array.",
            )
        PObj.SetMinSize(PassWordMinSize)
        if PasswordMaxSize > 0xFF:
            self.ErrorHandler(
                VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                "String MaxSize takes only one byte, which can't be larger than 0xFF.",
            )
        elif VarArraySize != 0 and PasswordMaxSize > VarArraySize:
            self.ErrorHandler(
                VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                "String MaxSize can't be larger than the max number of elements in string array.",
            )
        elif PasswordMaxSize < PassWordMinSize:
            self.ErrorHandler(
                VfrReturnCode.VFR_RETURN_INVALID_PARAMETER,
                "String MaxSize can't be less than String MinSize.",
            )
        PObj.SetMaxSize(PasswordMaxSize)
        Node = IfrTreeNode(EFI_IFR_PASSWORD_OP, PObj, gFormPkg.StructToStream(PObj.GetInfo()))
        self.SetPosition(Node, Position)
        ParentNode.insertChild(Node)
        if "component" in Password.keys():
            self._ParseVfrStatementQuestionOptionList(Password["component"], Node)
        self.InsertEndNode(Node)
        return Node

    def ParseVfrStatementImage(self, Image, ParentNode):
        IObj = IfrImage()
        ImageId = Image.PostVal
        IObj.SetImageId(ImageId)
        Node = IfrTreeNode(EFI_IFR_IMAGE_OP, IObj, gFormPkg.StructToStream(IObj.GetInfo()))

        ParentNode.insertChild(Node)
        return Node

    def ParseVfrStatementLocked(self, Locked, ParentNode):
        LObj = IfrLocked()
        Node = IfrTreeNode(EFI_IFR_LOCKED_OP, LObj, gFormPkg.StructToStream(LObj.GetInfo()))
        ParentNode.insertChild(Node)
        return Node

    def ParseVfrStatementLabel(self, Label, ParentNode):
        LObj = IfrLabel()
        Number = Label["number"].PostVal
        LObj.SetNumber(Number)
        Node = IfrTreeNode(EFI_IFR_GUID_OP, LObj, gFormPkg.StructToStream(LObj.GetInfo()))
        ParentNode.insertChild(Node)
        return Node

    def _ParseQuestionheaderFlagsField(self, Flag):
        QHFlag = 0
        if Flag == "READ_ONLY":
            QHFlag = 0x01

        elif Flag == "INTERACTIVE":
            QHFlag = 0x04

        elif Flag == "RESET_REQUIRED":
            QHFlag = 0x10

        elif Flag == "REST_STYLE":
            QHFlag = 0x20

        elif Flag == "RECONNECT_REQUIRED":
            QHFlag = 0x40

        else:
            gVfrErrorHandle.HandleWarning(EFI_VFR_WARNING_CODE.VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE)
        return QHFlag

    def ParseVfrStatementStaticText(self, Text, ParentNode):
        Help = Text["help"].PostVal
        Prompt = Text["prompt"].PostVal

        QId = EFI_QUESTION_ID_INVALID
        TxtTwo = EFI_STRING_ID_INVALID
        if "text" in Text.keys():
            TxtTwo = Text["text"].PostVal

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
            if "key" in Text.keys():
                self.AssignQuestionKey(AObj, Text["key"].PostVal)
            Node = IfrTreeNode(EFI_IFR_TEXT_OP, AObj, gFormPkg.StructToStream(AObj.GetInfo()))
            ParentNode.insertChild(Node)
            if "component" in Text.keys():
                for Component in Text["component"]:
                    ((Key, Value),) = Component.items()
                    if Key == "image":
                        self.ParseVfrStatementImage(Value, Node)
                    elif Key == "locked":
                        self.ParseVfrStatementLocked(Value, Node)
            self.InsertEndNode(Node)
        else:
            TObj = IfrText()
            TObj.SetHelp(Help)
            TObj.SetPrompt(Prompt)
            TObj.SetTextTwo(TxtTwo)
            Node = IfrTreeNode(EFI_IFR_TEXT_OP, TObj, gFormPkg.StructToStream(TObj.GetInfo()))
            ParentNode.insertChild(Node)
            if "component" in Text.keys():
                for Component in Text["component"]:
                    ((Key, Value),) = Component.items()
                    if Key == "image":
                        self.ParseVfrStatementImage(Value, Node)
                    elif Key == "locked":
                        self.ParseVfrStatementLocked(Value, Node)

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
            gVfrErrorHandle.PrintMsg(LineNum, "Error", ErrorMsg, TokenValue)

    def InsertEndNode(self, ParentNode):
        EObj = IfrEnd()
        ENode = IfrTreeNode(EFI_IFR_END_OP, EObj, gFormPkg.StructToStream(EObj.GetInfo()))
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

    def SetPosition(self, Node, Position):
        if Node.Data.VarIdStr != "" and Position != None:
            Node.Position = Position + "." + Node.Data.VarIdStr


class ModifiedNode:
    def __init__(self, Node, Operation) -> None:
        self.Node = Node
        self.Operation = Operation


class DLTParser:
    def __init__(self, Root) -> None:
        self.Root = Root
        self.ModifiedNodeList = []

    def FindQuestionNode(self, Root, Position):
        if Root == None:
            return None

        if Root.Position == Position:
            return Root

        if Root.Child != None:
            for ChildNode in Root.Child:
                Node = self.FindQuestionNode(ChildNode, Position)
                if Node != None:
                    return Node
        return None

    def AddQuestionNode(self, Position, QuestionDict, Parser: YamlParser, ParentNode=None):
        # find parentNode to insert
        if ParentNode == None:
            parts = Position.split(".")
            if parts[-1].startswith("option"):  # for option Opcode
                InsertedPos = ".".join(parts[:-1])
            else:  # for question opcode
                InsertedPos = ".".join(parts[:2])
            print(InsertedPos)
            # if digit is in position, turn to decimal integer
            InsertedPos = re.sub(r"0x[0-9a-fA-F]+|\d+", lambda x: str(int(x.group(), 0)), InsertedPos)
            ParentNode = self.FindQuestionNode(self.Root, InsertedPos)
            if ParentNode == None:
                print("error")  # raise error

        # QuestionDict['index'].PostVal records the number of conditional opcodes
        # add conditional opcode if condition is not none
        if QuestionDict["index"].PostVal > 0:
            for i in range(1, QuestionDict["index"].PostVal + 1):
                Condition = QuestionDict["condition" + str(i)]
                pattern = r"(.+?)\s+(.+)"
                match = re.match(pattern, Condition)
                if match:
                    Key = match.group(1)
                    Value = {}
                    Value["expression"].PostVal = match.group(2)
                    Parser.ParseVfrStatementConditional(Key, Value, ParentNode, Position)
                else:
                    print("error")

        QuestionObj = None
        QuestionNode = None
        # add question opcode

        if QuestionDict["opcode"].PostVal == "CheckBox":
            QuestionObj = IfrCheckBox()
            QuestionNode = IfrTreeNode(EFI_IFR_CHECKBOX_OP, QuestionObj)

        if QuestionDict["opcode"].PostVal == "Numeric":
            QuestionObj = IfrNumeric()
            QuestionNode = IfrTreeNode(EFI_IFR_NUMERIC_OP, QuestionObj)

        if QuestionDict["opcode"].PostVal == "OneOf":
            QuestionObj = IfrOneOf()
            QuestionNode = IfrTreeNode(EFI_IFR_ONE_OF_OP, QuestionObj)

        if QuestionDict["opcode"].PostVal == "String":
            QuestionObj = IfrString()
            QuestionNode = IfrTreeNode(EFI_IFR_STRING_OP, QuestionObj)

        if QuestionDict["opcode"].PostVal == "Password":
            QuestionObj = IfrPassword()
            QuestionNode = IfrTreeNode(EFI_IFR_PASSWORD_OP, QuestionObj)

        if QuestionDict["opcode"].PostVal == "OrderedList":
            QuestionObj = IfrOrderedList()
            QuestionNode = IfrTreeNode(EFI_IFR_ORDERED_LIST_OP, QuestionObj)

        if QuestionDict["opcode"].PostVal == "Time":
            QuestionObj = IfrTime()
            QuestionNode = IfrTreeNode(EFI_IFR_TIME_OP, QuestionObj)

        if QuestionObj["opcode"].PostVal == "Date":
            QuestionObj = IfrDate()
            QuestionNode = IfrTreeNode(EFI_IFR_DATE_OP, QuestionObj)

        if QuestionDict["opcode"].PostVal == "Action":
            QuestionObj = IfrAction()
            QuestionNode = IfrTreeNode(EFI_IFR_ACTION_OP, QuestionObj)

        if QuestionDict["opcode"].PostVal == "Ref":
            QuestionObj = IfrRef()
            QuestionNode = IfrTreeNode(EFI_IFR_REF_OP, QuestionObj)

        if QuestionDict["opcode"].PostVal == "Ref2":
            QuestionObj = IfrRef2()
            QuestionNode = IfrTreeNode(EFI_IFR_REF_OP, QuestionObj)

        if QuestionDict["opcode"].PostVal == "Ref3":
            QuestionObj = IfrRef3()
            QuestionNode = IfrTreeNode(EFI_IFR_REF_OP, QuestionObj)

        if QuestionDict["opcode"].PostVal == "Ref4":
            QuestionObj = IfrRef4()
            QuestionNode = IfrTreeNode(EFI_IFR_REF_OP, QuestionObj)

        if QuestionDict["opcode"].PostVal == "Ref5":
            QuestionObj = IfrRef5()
            QuestionNode = IfrTreeNode(EFI_IFR_REF_OP, QuestionObj)

        self.SetQuestionInfo(QuestionObj, QuestionDict)
        QuestionNode.Buffer = gFormPkg.StructToStream(QuestionObj)
        QuestionNode.Position = Position

        ParentNode.insertChild(QuestionNode)
        self.ModifiedNodeList.append(ModifiedNode(QuestionNode, "ADD"))

        return QuestionNode

    def ModifyQuestionNode(self, OperatedNode, Position, QuestionDict, Parser):
        NewNode = self.AddQuestionNode(Position, QuestionDict, Parser, OperatedNode.Parent)
        for i in range(0, len(OperatedNode.Parent.Child) - 1):
            if OperatedNode == OperatedNode.Parent.Child[i]:
                OperatedNode.Parent.Child[i] = NewNode
                NewNode.Parent.Child.pop()
                self.ModifiedNodeList.append(ModifiedNode(NewNode, "Modify"))
                return NewNode
        return NewNode

    def DeleteQuestionNode(self, OperatedNode: IfrTreeNode):
        # or directly delete the Node itself
        if OperatedNode.Condition == None:
            OperatedNode.Condition = "suppressif true"
        else:
            OperatedNode.Condition += " | " + "suppressif true"

        self.ModifiedNodeList.append(ModifiedNode(OperatedNode, "Delete"))
        return OperatedNode

    def SetQuestionInfo(self, OpCode, Dict):
        if "questionid" in Dict.keys():
            OpCode.Obj.Question.QuestionId = Dict["questionid"].PostVal
        if "varstoreid" in Dict.keys():
            OpCode.Obj.Question.VarStoreId = Dict["varstoreid"].PostVal
        if "varname" in Dict.keys():
            OpCode.Obj.Question.VarStoreInfo.VarName = Dict["varname"].PostVal
        if "varoffset" in Dict.keys():
            OpCode.Obj.Question.VarStoreInfo.VarOffset = Dict["varoffset"].PostVal
        if "questionflags" in Dict.keys():
            OpCode.Obj.Question.Flags = Dict["questionflags"].PostVal
        if "opcodeflags" in Dict.keys():
            OpCode.Obj.Flags = Dict["opcodeflags"].PostVal
        if "prompt" in Dict.keys():
            OpCode.Obj.Question.Header.Prompt = Dict["prompt"].PostVal
        if "help" in Dict.keys():
            OpCode.Obj.Question.Header.Help = Dict["help"].PostVal
