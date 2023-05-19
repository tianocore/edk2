import yaml
import json
import CppHeaderParser
from IfrFormPkg import *
from antlr4 import *
from IfrCtypes import *
import re

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

class ValueDB():
    def __init__(self, PreVal, PostVal) -> None:
        self.PreVal = PreVal
        self.PostVal = PostVal

class PreProcessDB():
    def __init__(self, Options: Options) -> None:
        self.Options = Options
        self.VfrVarDataTypeDB = VfrVarDataTypeDB()
        self.Preprocessed = False

    def Preprocess(self):
        self.HeaderFiles = self._ExtractHeaderFiles()
        self.HeaderDict = self._GetHeaderDicts(self.HeaderFiles) # Read Guid definitions in Header files
        self.UniDict, self.UniDisPlayDict = self._GetUniDicts() # Read Uni string token/id definitions in StrDef.h file
        self.VfrDict = self._GetVfrDicts() # Read definitions in source file
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
            EdkLogger.error("VfrCompiler", PARAMETER_INVALID,
                            "Invalid parameter:  %s" % Key, None)

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
            EdkLogger.error("VfrCompiler", FILE_PARSE_FAILURE,
                            "File parse failed for %s" % FileName, None)
        return HeaderFiles

    # def _GenExpandedHeaderFile(self):
    #     try:
    #         FileName = self.Options.ExpandedHeaderFileName
    #         Output = open(FileName, 'w')
    #         for HeaderFile in self.HeaderFiles:
    #             FileName = HeaderFile.split('/')[-1]
    #             FileList = self._FindIncludeHeaderFile("/edk2/", FileName)
    #             CppHeader = None
    #             for File in FileList:
    #                 if File.find(HeaderFile.replace('/','\\')) != -1:
    #                     Output.write(open(File, 'r').read())
    #                     CppHeader = CppHeaderParser.CppHeader(File)
    #                     break
    #             if CppHeader == None:
    #                 EdkLogger.error("VfrCompiler", FILE_NOT_FOUND,
    #                             "File/directory %s not found in workspace" % (HeaderFile), None)

    #             for Include in CppHeader.includes:
    #                 Include = Include[1:-1]
    #                 IncludeFileName = Include.split('/')[1]
    #                 IncludeHeaderFileList = self._FindIncludeHeaderFile("/edk2/", IncludeFileName)
    #                 Flag = False
    #                 for File in IncludeHeaderFileList:
    #                     if File.find(Include.replace('/','\\')) != -1:
    #                         Output.write(open(File, 'r').read())
    #                         Flag = True
    #                         break
    #                 if Flag == False:
    #                     EdkLogger.error("VfrCompiler", FILE_NOT_FOUND,
    #                                     "File/directory %s not found in workspace" % IncludeFileName, None)
    #         Output.close()
    #     except:
    #         EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
    #                         "File open failed for %s" % FileName, None)


    def _GetUniDicts(self):
        if self.Options.UniStrDefFileName == None:
            self.Options.UniStrDefFileName = self.Options.DebugDirectory + self.Options.ModuleName + 'StrDefs.h'
        # find UniFile
        FileName = self.Options.UniStrDefFileName
        with open(FileName, "r") as File:
            Content = File.read()
        UniDict = {} # {String Token : String Token ID}
        self._ParseDefines(FileName, UniDict)

        if self.Options.UniStrDisplayFile == None:
            self.Options.UniStrDisplayFile = self.Options.OutputDirectory + self.Options.BaseFileName + 'Uni.json'

        DisPlayUniDict = {} # {String Token : DisPlay String}
        if self.Options.LanuchYamlCompiler:
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
            FileList = self._FindIncludeHeaderFile(self.Options.IncludePaths, FileName)
            CppHeader = None
            for File in FileList:
                if File.find(HeaderFile.replace('/','\\')) != -1:
                    self._ParseDefines(File, HeaderDict)
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
                        self._ParseDefines(File, HeaderDict)
                        Flag = True
                        break
                if Flag == False:
                    EdkLogger.error("VfrCompiler", FILE_NOT_FOUND,
                                    "File/directory %s not found in workspace" % IncludeFileName, None)
        return HeaderDict

    def _GetVfrDicts(self):
        VfrDict = {}
        if self.Options.LanuchVfrCompiler:
            FileName = self.Options.InputFileName
            self._ParseDefines(FileName, VfrDict)
        return VfrDict

    def _ParseDefines(self, FileName, Dict):
        Pattern = r"#define\s+(\w+)\s+(.*?)(?:(?<!\\)\n|$)"
        with open(FileName, "r") as File:
            Content = File.read()

        Matches = re.findall(Pattern, Content, re.DOTALL)
        for Match in Matches:
            Key = Match[0]
            Value = re.sub(r"\s+", " ", Match[1].replace("\\\n", "").strip())
            SubDefineMatches = re.findall(r"#define\s+(\w+)\s+(.*?)(?:(?<!\\)\n|$)", Value, re.DOTALL)
            if SubDefineMatches:
                for SubMatch in SubDefineMatches:
                    SubKey = SubMatch[0]
                    SubValue = re.sub(r"\s+", " ", SubMatch[1].replace("\\\n", "").strip())
                    if SubValue.find("//") != -1:
                        SubValue = SubValue.split("//")[0].strip()

                    if SubValue.find('{') != -1:
                        GuidList = re.findall(r"0x[0-9a-fA-F]+", SubValue)
                        GuidList = [int(num, 16) for num in GuidList]
                        SubValue = EFI_GUID()
                        SubValue.from_list(GuidList)
                        # SubValue = SubValue.to_string()
                    Dict[SubKey] = SubValue
            else:
                if Value.find("//") != -1:
                    Value = Value.split("//")[0].strip()
                if Value.find('{') != -1:
                    GuidList = re.findall(r"0x[0-9a-fA-F]+", Value)
                    GuidList = [int(num, 16) for num in GuidList]
                    Value = EFI_GUID()
                    Value.from_list(GuidList)
                    # Value =  Value.to_string()
                Dict[Key] = Value

    def _FindIncludeHeaderFile(self, IncludePaths, Name):
        FileList = []
        for Start in IncludePaths:
            for Relpath, Dirs, Files in os.walk(Start):
                if Name in Files:
                    FullPath = os.path.join(Start, Relpath, Name)
                    FileList.append(os.path.normpath(os.path.abspath(FullPath)))
        return FileList
