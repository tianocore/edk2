import yaml
import CppHeaderParser
from VfrFormPkg import *
from antlr4 import *
from VfrCtypes import *

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from Common.LongFilePathSupport import LongFilePath

class Options():

    def __init__(self):
        self.VfrFileName = None
        self.VfrBaseFileName = None
        self.IncludePaths = None
        self.OutputDirectory = None
        self.CreateRecordListFile = True
        self.RecordListFileName = None
        self.CreateIfrPkgFile = True
        self.PkgOutputFileName = None
        self.COutputFileName = None
        self.CreateYamlFile = True
        self.YamlFileName = None
        self.CreateJsonFile = True
        self.JsonFileName = None
        self.SkipCPreprocessor = True
        self.CPreprocessorOptions = None
        self.PreprocessorOutputFileName = None
        self.HasOverrideClassGuid = False
        self.OverrideClassGuid = None
        self.WarningAsError = False
        self.AutoDefault = False
        self.CheckDefault = False

        self.ProcessedInFileName = None
        self.UniStrDefFileName = None
        self.ExpandedHeaderFileName = None # save header info for yaml
        self.ProcessedYAMLFileName = None
        self.CompileYaml = True

class KV():
    def __init__(self, Key, Value) -> None:
        self.Key = Key
        self.Value = Value

class PreProcessDB():
    def __init__(self, Options: Options) -> None:
        self.Options = Options
        self.VfrVarDataTypeDB = VfrVarDataTypeDB()

    def Preprocess(self):
        self.HeaderFiles = self._ExtractHeaderFiles()
        self.HeaderDict = self._GetHeaderDicts(self.HeaderFiles)
        self.UniDict = self._GetUniDicts()
        self.VfrDict = self._GetVfrDicts()
        self._GenExpandedHeaderFile()

    def TransValue(self, Value):
        if type(Value) == EFI_GUID:
            return Value
        else:
            StrValue = str(Value)
            if '0x' in StrValue:
                Value = int(StrValue, 0)
            else:
                Value = int(StrValue)
        # error handle , value is too large to store
        return Value

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
                    elif Items[i].find('0x') or Items[i].find('0X'):
                        NewValue += Items[i].strip()
                    else: # ignore flag types
                        #error
                        print('error')

                    if i != len(Items) - 1:
                        NewValue += ' | '
            return NewValue
        else:
            # error
            print('error')

    def _ExtractHeaderFiles(self):
        FileName = self.Options.VfrFileName
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

    def _SetUniStrDefFileName(self):
        if self.Options.ProcessedInFileName.find('/') == -1:
            self.Options.ProcessedInFileName = self.Options.ProcessedInFileName.replace('\\', '/')
        FileList = []
        if self.Options.ProcessedInFileName.find('DriverSample') != -1:
            FileList = self._FindIncludeHeaderFile('/edk2/', self.Options.ProcessedInFileName.split('/')[-3][:-3] + 'StrDefs.h')
        else:
            FileList = self._FindIncludeHeaderFile('/edk2/', self.Options.ProcessedInFileName.split('/')[-3] + 'StrDefs.h')
        if self.Options.UniStrDefFileName == []:
            EdkLogger.error("VfrCompiler", FILE_NOT_FOUND,
                            "File/directory %s not found in workspace" % (self.Options.ProcessedInFileName.split('/')[-3] + 'StrDefs.h'), None)
        self.Options.UniStrDefFileName = FileList[0]

    def _GetUniDicts(self):
        # key: STR_FORM_SET_TITLE_HELP, value: 0x1234
        # type: str
        if self.Options.UniStrDefFileName == None:
            self._SetUniStrDefFileName()
        # find UniFile
        FileName = self.Options.UniStrDefFileName
        CppHeader = CppHeaderParser.CppHeader(self.Options.UniStrDefFileName)
        UniDict = {}
        for Define in CppHeader.defines:
            Items = Define.split()
            if len(Items) == 2:
                UniDict[Items[0]] = Items[1] #
        return UniDict

    def _GetHeaderDicts(self, HeaderFiles):
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

    def _GetVfrDicts(self):
        VfrDict = {}
        File = open(self.Options.VfrFileName, 'r')
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

    def _FindIncludeHeaderFile(self, Start, Name):
        FileList = []
        for Relpath, Dirs, Files in os.walk(Start):
            if Name in Files:
                FullPath = os.path.join(Start, Relpath, Name)
                FileList.append(os.path.normpath(os.path.abspath(FullPath)))
        return FileList