import yaml
import json
import CppHeaderParser
from IfrFormPkg import *
from antlr4 import *
from IfrCtypes import *

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from Common.LongFilePathSupport import LongFilePath

class Options():

    def __init__(self):
        # open/close Vfr/Yamlcompiler
        self.LanuchVfrCompiler = False
        self.LanuchYamlCompiler = False

        self.ModuleName = None
        self.InputFileName = None
        self.BaseFileName = None
        self.IncludePaths = []
        self.OutputDirectory = None
        self.DebugDirectory = None
        self.CreateRecordListFile = True
        self.RecordListFileName = None
        self.CreateIfrPkgFile = True
        self.PkgOutputFileName = None
        self.COutputFileName = None
        self.SkipCPreprocessor = True
        self.CPreprocessorOptions = None
        self.PreprocessorOutputFileName = None
        self.HasOverrideClassGuid = False
        self.OverrideClassGuid = None
        self.WarningAsError = False
        self.AutoDefault = False
        self.CheckDefault = False

        self.CreateYamlFile = True
        self.YamlFileName = None
        self.CreateJsonFile = True
        self.JsonFileName = None

        self.UniStrDefFileName = None # {String Token : String Token ID} Uni file name
        self.UniStrDisplayFile = None # {String Token : DisPlay String}
        self.DeltaFileName = None
        self.YamlOutputFileName = None #

        # for test
        self.ProcessedVfrFileName = None # for test
        #self.ExpandedHeaderFileName = None # save header info for yaml
        self.ProcessedYAMLFileName = None # for test



class KV():
    def __init__(self, Key, Value) -> None:
        self.Key = Key
        self.Value = Value

class PreProcessDB():
    def __init__(self, Options: Options) -> None:
        self.Options = Options
        self.VfrVarDataTypeDB = VfrVarDataTypeDB()
        self.Preprocessed = False

    def Preprocess(self):
        self.HeaderFiles = self._ExtractHeaderFiles()
        self.HeaderDict = self._GetHeaderDicts(self.HeaderFiles)
        self.UniDict, self.UniDisPlayDict = self._GetUniDicts()
        self.VfrDict = self._GetVfrDicts()
        self.Preprocessed = True
        # self._GenExpandedHeaderFile()

    def TransValue(self, Value):
        if type(Value) == EFI_GUID:
            return Value
        else:
            StrValue = str(Value)
            if ('0x' in StrValue) or ('0X' in StrValue):
                Value = int(StrValue, 0)
            else:
                Value = int(StrValue)
        # error handle , value is too large to store
        return Value

    def RevertValue(self, Value) -> str:
        if type(Value) == EFI_GUID:
            return Value.to_string()
        else:
            if ('0x' in Value) or ('0X' in Value):
                StrValue = hex(Value)
            else:
                StrValue = str(Value)
        return  StrValue

    def Read(self, Key):
        if Key in self.UniDict.keys():
            return self.TransValue(self.UniDict[Key])
        elif Key in self.HeaderDict.keys():
            return self.TransValue(self.HeaderDict[Key])
        elif Key in self.VfrDict.keys():
            return self.TransValue(self.VfrDict[Key])
        elif '|' in Key:
            Items = Key.split('|')
            NewValue = ''
            for i in range(0, len(Items)):
                # get {key:value} dicts from header files
                if Items[i].strip() in self.HeaderDict.keys():
                    if type(self.HeaderDict[Items[i].strip()]) == EFI_GUID:
                        NewValue += self.HeaderDict[Items[i].strip()].to_string()
                    else:
                        NewValue += self.HeaderDict[Items[i].strip()]
                else:
                    NewValue += Items[i].strip()
                if i != len(Items) - 1:
                    NewValue += ' | '
            return NewValue
        else:
            # error
            print('error')

    def _ExtractHeaderFiles(self):
        FileName = self.Options.InputFileName
        try:
            fFile = open(LongFilePath(FileName), mode='r')
            line = fFile.readline()
            IsHeaderLine = False
            HeaderFiles = []
            while line:
                if "#include" in line:
                    IsHeaderLine = True
                    if line.find('<') != -1:
                        HeaderFile = line[line.find('<') + 1:line.find('>')]
                        HeaderFiles.append(HeaderFile)
                    if line.find('\"') != -1:
                        l = line.find('\"') + 1
                        r = l + line[l:].find('\"')
                        HeaderFile = line[l:r]
                        HeaderFiles.append(HeaderFile)
                line = fFile.readline()
                if IsHeaderLine == True and "#include" not in line:
                    break
            fFile.close()
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                            "File open failed for %s" % FileName, None)
        return HeaderFiles

    def _GenExpandedHeaderFile(self):
        try:
            FileName = self.Options.ExpandedHeaderFileName
            Output = open(FileName, 'w')
            for HeaderFile in self.HeaderFiles:
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

    # def _SetUniStrDefFileName(self):
    #     if self.Options.ProcessedVfrFileName.find('/') == -1:
    #         self.Options.ProcessedVfrFileName = self.Options.ProcessedVfrFileName.replace('\\', '/')
    #     FileList = []
    #     if self.Options.ProcessedVfrFileName.find('DriverSample') != -1:
    #         FileList = self._FindIncludeHeaderFile('/edk2/', self.Options.ProcessedVfrFileName.split('/')[-3][:-3] + 'StrDefs.h')
    #     else:
    #         FileList = self._FindIncludeHeaderFile('/edk2/', self.Options.ProcessedVfrFileName.split('/')[-3] + 'StrDefs.h')
    #     if self.Options.UniStrDefFileName == []:
    #         EdkLogger.error("VfrCompiler", FILE_NOT_FOUND,
    #                         "File/directory %s not found in workspace" % (self.Options.ProcessedVfrFileName.split('/')[-3] + 'StrDefs.h'), None)
    #     self.Options.UniStrDefFileName = FileList[0]

    def _GetUniDicts(self):

        if self.Options.UniStrDefFileName == None:
            # if self.Options.OutputDirectory.find('/') == -1:
            #     self.Options.OutputDirectory = self.Options.OutputDirectory.replace('\\', '/')
            #     DebugPath = "/".join(self.Options.OutputDirectory.split('/'))[:-1] + '/DEBUG/'
            self.Options.UniStrDefFileName = self.Options.DebugDirectory + self.Options.ModuleName + 'StrDefs.h'

        CppHeader = CppHeaderParser.CppHeader(self.Options.UniStrDefFileName)
        UniDict = {} # {String Token : String Token ID}
        for Define in CppHeader.defines:
            Items = Define.split()
            if len(Items) == 2:
                # key: STR_FORM_SET_TITLE_HELP, value: 0x1234, type: str
                UniDict[Items[0]] = Items[1]

        DisPlayUniDict = {} # {String Token : DisPlay String}
        if self.Options.LanuchYamlCompiler:
            if self.Options.UniStrDisplayFile == None:
                self.Options.UniStrDisplayFile = self.Options.OutputDirectory + self.Options.BaseFileName + 'Uni.json'
            f = open(self.Options.UniStrDisplayFile, encoding='utf-8')
            Dict = json.load(f)
            f.close()
            Dict = Dict[1]["en-US"]
            for Key in Dict.keys():
                if Key in UniDict.keys():
                    NewKey = '{}'.format('0x%04x' % (int(UniDict[Key],0)))
                    DisPlayUniDict[NewKey] = Dict[Key]

        return UniDict, DisPlayUniDict

    def _GetHeaderDicts(self, HeaderFiles):
        HeaderDict = {}
        for HeaderFile in HeaderFiles:
            FileName = HeaderFile.split('/')[-1]
            # FileList = self._FindIncludeHeaderFile("/edk2/", FileName)
            FileList = self._FindIncludeHeaderFile(self.Options.IncludePaths, FileName)
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
                IncludeHeaderFileList = self._FindIncludeHeaderFile(self.Options.IncludePaths, IncludeFileName)
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

    def _GetVfrDicts(self):
        VfrDict = {}
        File = open(self.Options.InputFileName, 'r')
        Defines = []
        Wrap = False
        IncompleteLine = ''
        for Line in File:
            if Wrap == True:
                Defines.append(IncompleteLine + Line.replace('\n', ''))
                Wrap = False
                continue
            if "#define" in Line:
                if Line.find('\\') != -1:
                    Wrap = True
                    IncompleteLine = Line.split(" ", 1)[1].replace('\\\n', '')
                else:
                    Defines.append(Line.split(" ", 1)[1].replace('\n', ''))
        self._ParseDefines(Defines, VfrDict)
        return VfrDict

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
                #Value = Guid.to_string()
                HeaderDict[Key] = Guid

    def _FindIncludeHeaderFile(self, IncludePaths, Name):
        FileList = []
        for Start in IncludePaths:
            for Relpath, Dirs, Files in os.walk(Start):
                if Name in Files:
                    FullPath = os.path.join(Start, Relpath, Name)
                    FileList.append(os.path.normpath(os.path.abspath(FullPath)))
        return FileList
