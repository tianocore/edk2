from VfrSyntaxVisitor import *
from VfrSyntaxLexer import *
from VfrSyntaxParser import *
from VfrFormPkg import *
from VfrError import *
class YamlTree():
    def __init__(self, Root: VfrTreeNode, Options: Options):
        self.Options = Options
        self.Root = Root
        self.Config = None

    # transform string token and Header key-values to specific values for yaml
    def PreProcess(self):
        try:
            FileName = self.Options.YamlFileName
            f = open(FileName, 'r')
            #self.Config = yaml.load(f.read(), Loader=yaml.FullLoader)
            self.Config = yaml.safe_load(f)
            f.close()
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                            "File open failed for %s" % FileName, None)

        UniDict = self._GetUniDictsForYaml()
        HeaderDict = self._GetHeaderDictsForYaml(self.Config['include'])
        self._PreprocessYaml(self.Config, UniDict, HeaderDict)

        try:
            FileName =  self.Options.ProcessedYAMLFileName
            f = open(FileName, 'w')
            f.write(yaml.dump(self.Config, allow_unicode=True, default_flow_style=False))
            f.close()
        except:
            EdkLogger.error("VfrCompiler", FILE_CREATE_FAILURE,
                            "File create failed for %s" % FileName, None)

        self._GenExpandedHeaderFile()

    def Compile(self):
        self._ParseExpandedHeader()
        self._ParseVfrFormSetDefinition()


    def _PreprocessYaml(self, Config, UniDict, HeaderDict):
        for Key in Config.keys():
            Value = Config[Key]
            if isinstance(Value, list): # value is list
                for Item in Value:
                    if isinstance(Item, dict):
                        self._PreprocessYaml(Item, UniDict, HeaderDict)

            elif isinstance(Value, dict): # value is dict
                self._PreprocessYaml(Value, UniDict, HeaderDict)
            else:
                # get the specific value of string token
                if Value in UniDict.keys():
                    Config[Key] = 'STRING_TOKEN' + '(' +  UniDict[Value] + ')'
                # get {key:value} dicts from header files
                if Value in HeaderDict.keys():
                    Config[Key] = HeaderDict[Value]
        return

    # parse and collect data structures info in the ExpandedHeader.i files
    def _ParseExpandedHeader(self):
        try:
            InputStream = FileStream(self.Options.ExpandedHeaderFileName)
            VfrLexer = VfrSyntaxLexer(InputStream)
            VfrStream = CommonTokenStream(VfrLexer)
            VfrParser = VfrSyntaxParser(VfrStream)
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                            "File open failed for %s" % FileName, None)

        self.Visitor = VfrSyntaxVisitor(None, self.Options.OverrideClassGuid)
        gVfrVarDataTypeDB.Clear()
        self.Visitor.visit(VfrParser.vfrProgram())
        FileName = self.Options.OutputDirectory + self.Options.VfrBaseFileName + 'Header.txt'
        try:
            f = open(FileName, 'w')
            gVfrVarDataTypeDB.Dump(f)
            f.close()
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                            "File open failed for %s" % FileName, None)

    def _GetUniDictsForYaml(self):
        # key: STRING_TOKEN(STR_FORM_SET_TITLE_HELP), value: 0x1234
        # type: str
        FileName = self.Options.UniStrDefFileName
        CppHeader = CppHeaderParser.CppHeader(self.Options.UniStrDefFileName)
        UniDict = {}
        for Define in CppHeader.defines:
            Items = Define.split()
            if len(Items) == 2:
                UniDict['STRING_TOKEN' + '(' + Items[0] + ')'] = Items[1]
        return UniDict

    def _GenExpandedHeaderFile(self):
        try:
            FileName = self.Options.ExpandedHeaderFileName
            Output = open(FileName, 'w')
            for HeaderFile in self.Config['include']:
                FileName = HeaderFile.split('/')[-1]
                FileList = self._FindIncludeHeaderFile("/edk2/", FileName)
                CppHeader = None
                for File in FileList:
                    if File.find(HeaderFile.replace('/','\\')) != -1:
                        Output.write(open(File, 'r').read())
                        CppHeader = CppHeaderParser.CppHeader(File)
                        break
                if CppHeader == None:
                    EdkLogger.error("VfrCompiler", FILE_NOT_FOUND,
                                "File/directory %s not found in workspace" % (HeaderFile), None)

                for Include in CppHeader.includes:
                    Include = Include[1:-1]
                    IncludeFileName = Include.split('/')[1]
                    IncludeHeaderFileList = self._FindIncludeHeaderFile("/edk2/", IncludeFileName)
                    #print(IncludeHeaderFileList)
                    Flag = False
                    for File in IncludeHeaderFileList:
                        if File.find(Include.replace('/','\\')) != -1:
                            Output.write(open(File, 'r').read())
                            Flag = True
                            break
                    if Flag == False:
                        EdkLogger.error("VfrCompiler", FILE_NOT_FOUND,
                                        "File/directory %s not found in workspace" % IncludeFileName, None)
            Output.close()
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                            "File open failed for %s" % FileName, None)

    def _GetHeaderDictsForYaml(self, HeaderFiles):
        HeaderDict = {}
        for HeaderFile in HeaderFiles:
            FileName = HeaderFile.split('/')[-1]
            FileList = self._FindIncludeHeaderFile("/edk2/", FileName)
            CppHeader = None
            for File in FileList:
                if File.find(HeaderFile.replace('/','\\')) != -1:
                    CppHeader = CppHeaderParser.CppHeader(File)
                    self._ParseDefines(CppHeader.defines, HeaderDict)
                    break
            if CppHeader == None:
                EdkLogger.error("VfrCompiler", FILE_NOT_FOUND,
                            "File/directory %s not found in workspace" % (HeaderFile), None)

            for Include in CppHeader.includes:
                Include = Include[1:-1]
                IncludeFileName = Include.split('/')[1]
                IncludeHeaderFileList = self._FindIncludeHeaderFile("/edk2/", IncludeFileName)
                Flag = False
                for File in IncludeHeaderFileList:
                    if File.find(Include.replace('/','\\')) != -1:
                        CppHeader = CppHeaderParser.CppHeader(File)
                        self._ParseDefines(CppHeader.defines, HeaderDict)
                        Flag = True
                        break
                if Flag == False:
                    EdkLogger.error("VfrCompiler", FILE_NOT_FOUND,
                                    "File/directory %s not found in workspace" % IncludeFileName, None)
        #print(HeaderDict)
        return HeaderDict

    def _ParseDefines(self, Defines, HeaderDict):
        for Define in Defines:
            #print(Define)
            Items = Define.split()
            if len(Items) == 2:
                # key->str, Value->int
                HeaderDict[Items[0]] = Items[1]
                # HeaderDict[Items[0]] = int(Items[1], 16)
            elif len(Items) > 2 and Items[0].find('GUID') != -1:
                 # key->str, Value->str
                GuidStr = ''
                for i in range(1, len(Items)):
                    if Items[i] != '{' and Items[i] != '}' and Items[i] != '{{' and Items[i] !='}}' and Items[i] != '\\':
                        if ('{' in Items[i]):
                            Items[i] = Items[i][1:]
                        if '}' in Items[i]:
                            Items[i] = Items[i][:-1]

                        GuidStr += Items[i]
                GuidItems = GuidStr.split(',')
                Guid = EFI_GUID()
                Guid.Data1 = int(GuidItems[0], 16)
                Guid.Data2 = int(GuidItems[1], 16)
                Guid.Data3 = int(GuidItems[2], 16)
                for i in range(0, 8):
                    Guid.Data4[i] = int(GuidItems[i+3], 16)
                if Items[0].find('\\') != -1:
                    Items[0] = Items[0][:-1]
                Key = Items[0]
                Value = Guid.to_string()
                HeaderDict[Key] = Value

    def _FindIncludeHeaderFile(self, Start, Name):
        FileList = []
        for Relpath, Dirs, Files in os.walk(Start):
            if Name in Files:
                FullPath = os.path.join(Start, Relpath, Name)
                FileList.append(os.path.normpath(os.path.abspath(FullPath)))
        return FileList

    # parse Formset Definition
    def _ParseVfrFormSetDefinition(self):
        pass
'''
        GuidStr = self.FV["classguid"]
        GuidList = GuidStr.split("|")
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
        FSObj.SetGuid(self.FV["guid"])
        FSObj.SetFormSetTitle(self.FV["title"])
        FSObj.SetHelp(self.FV["help"])

        Node = VfrTreeNode(EFI_IFR_FORM_SET_OP, FSObj)
        Node.Buffer = gFormPkg.StructToStream(FSObj.GetInfo())
        for i in range(0, len(GuidList)):
            Node.Buffer += gFormPkg.StructToStream(GuidList[i])

        self.Root.insertChild(Node)

        if self.FV.has_key("class"):
            self._ParseClassDefinition()


        if self.FV.has_key("subclass"):
            pass

    def _ParseClassDefinition(self):
        pass
'''
