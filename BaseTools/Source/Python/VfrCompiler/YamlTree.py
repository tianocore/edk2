from VfrSyntaxVisitor import *
from VfrSyntaxLexer import *
from VfrSyntaxParser import *
from VfrFormPkg import *
from VfrError import *
class YamlTree():
    def __init__(self, Root: VfrTreeNode, Options: Options):
        self.Options = Options
        self.Root = Root

    def PreProcess(self):
        FileName = self.Options.YamlFileName
        try:
            f = open(FileName, 'r')
            #YamlDict = yaml.load(f, Loader=yaml.FullLoader)
            Config = yaml.safe_load(f)
            f.close()
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                            "File open failed for %s" % FileName, None)
        #print(Config)
        if 'include' in Config.keys():
            self.ProcessHeader(Config['include'])

        if 'formset' in Config.keys():
            # transform string token to specific values for yaml, and generate a new yaml file
            self.ProcessFormset(Config['formset'])

        # for Key in Config:
        #    print(Key)

    def ProcessHeader(self, includepaths):
        FileName = self.Options.OutputDirectory + self.Options.VfrBaseFileName + 'Header.vfr'
        try:
            f = open(FileName, 'w')
            for includepath in includepaths:
                f.write("#include <" + includepath + '>\n')
            f.close()
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                            "File open failed for %s" % FileName, None)

        # call C preprocessor, gen .i file
        # delete .vfr

        # parse  and collect data structures info in the header files
        try:
            InputStream = FileStream(self.Options.HeaderFileName)
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

    def ProcessFormset(self, FormsetValues):
        FileName =  self.Options.OutputDirectory + self.Options.VfrBaseFileName + 'Processed.yml'

        self.FV = FormsetValues
        KeyValueDict = self.GetUniDictsForYaml()
        for key in KeyValueDict:
            pass


    def GetUniDictsForYaml(self):
        FileName = self.Options.UNIStrFileName
        try:
            f = open(FileName, 'r')
            Lines = f.read()
            f.close()
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                            "File open failed for %s" % FileName, None)
        KeyValueDict = {}
        for Line in Lines.split('\n'):
            Items = Line.split('|')
            Key = Items[1].strip()
            Value = int(Items[0].split(' ')[1], 16)
            KeyValueDict[Key] = Value
        return KeyValueDict


    def Compile(self):
        self._ParseVfrFormSetDefinition()

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
