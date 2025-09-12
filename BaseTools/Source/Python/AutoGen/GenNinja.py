import os
import re
import json
from AutoGen import ninja_syntax
import uuid
import shutil
import platform

gNinjaBuildFilePath = ''

def CacheEdk2BuildCmd(CommandOption, NinjaBuildFilePath):
    WorkspaceDir = os.path.normcase(os.path.normpath(os.environ["WORKSPACE"]))
    NinjaDictFilePath = os.path.join(WorkspaceDir, '.ninja_dict')
    NinjaDict = {}
    if os.path.exists(NinjaDictFilePath):
        with open(NinjaDictFilePath, 'r') as Fd:
            NinjaDict = json.load(Fd)
        if CommandOption not in NinjaDict.keys():
            NinjaDict[CommandOption] = NinjaBuildFilePath
            with open(NinjaDictFilePath, 'w') as Fd:
                json.dump(NinjaDict, Fd, indent=4)
    else:
        NinjaDict[CommandOption] = NinjaBuildFilePath
        with open(NinjaDictFilePath, 'w') as Fd:
            json.dump(NinjaDict, Fd, indent=4)


def Edk2BuildCmdIsCached(CommandOption):
    WorkspaceDir = os.path.normcase(os.path.normpath(os.environ["WORKSPACE"]))
    NinjaDict = {}
    NinjaDictFilePath = os.path.join(WorkspaceDir, '.ninja_dict')
    if os.path.exists(NinjaDictFilePath):
        with open(NinjaDictFilePath, 'r') as Fd:
            NinjaDict = json.load(Fd)
            if CommandOption in NinjaDict.keys():
                NinjaBuildFilePath = NinjaDict[CommandOption]
                if os.path.exists(NinjaBuildFilePath):
                    return True, NinjaBuildFilePath
    return False, None


def CallNinjaBuild(NinjaExplain=False):
    EdkToolPath = os.environ['EDK_TOOLS_PATH']
    NinjaToolPath = ''
    DebugFlag = ''
    if platform.system() == 'Windows':
        NinjaToolPath = os.path.join(EdkToolPath, 'BinWrappers', 'WindowsLike', 'ninja.exe')
    if platform.system() == 'Linux':
        NinjaToolPath = os.path.join(EdkToolPath, 'BinWrappers', 'PosixLike', 'ninja')
    if NinjaExplain:
        DebugFlag = ' -d explain '
    os.system(f'{NinjaToolPath} {DebugFlag} -f {gNinjaBuildFilePath}')


class MakefileParser:
    def __init__(self, MakefileContent, NinjaBuildTargetList):
        self.Content = MakefileContent
        self.Variables = {}
        self.Targets = {}
        self.NinjaBuildTargetList = NinjaBuildTargetList
        self.FileUuid = str(uuid.uuid4())[:8]

    def Parse(self):
        # Replace $(var) with ${uuid_var} in the entire content
        Content = self.AddVariablesPrefix(self.Content)

        # Split the content into lines
        Lines = Content.split('\n')

        # Handle line continuation for variable values
        ProcessedLines = []
        Index = 0
        while Index < len(Lines):
            Line = Lines[Index].rstrip()
            while Line.endswith(' \\'):
                Line = Line[:-1].rstrip() + ' ' + Lines[Index + 1].strip()
                Index += 1
            ProcessedLines.append(Line)
            Index += 1

        # Parse variables and rules
        VariablePattern = re.compile(r'^(\w+)\s*=\s*(.*)')

        for Line in ProcessedLines:
            VariableMatch = VariablePattern.match(Line)

            if VariableMatch:
                VarName = VariableMatch.group(1)
                VarValue = VariableMatch.group(2)
                if platform.system() == "Windows" and VarName == 'NASM_INC':
                    VarValue =  VarValue.replace('^', '\\')
                PrefixedVarName = f"{self.FileUuid}_{VarName}"
                self.Variables[PrefixedVarName] = VarValue
                if VarName == 'DEPS_FLAGS':
                    self.Variables[PrefixedVarName] = VarValue.replace('$@.deps', '')
                if VarName == 'MAKE_FILE':
                    self.Variables[PrefixedVarName] = ''
                #print(f"Variable defined: {PrefixedVarName} = {VarValue}")
        for CurrentTarget, Deps, Commands in self.NinjaBuildTargetList:
            CurrentTarget = self.AddVariablesPrefix(CurrentTarget)
            CurrentTarget = self.ReplaceVariableWithValue(CurrentTarget)
            self.Targets[CurrentTarget] = {'dependencies': [], 'commands': []}
            DepStr = ''
            if isinstance(Deps, list):
                DepStr = ' '.join(Deps)
            else:
                DepStr = Deps
            DepStr = self.AddVariablesPrefix(DepStr)
            DepStr = self.ReplaceVariableWithValue(DepStr)

            for Dep in DepStr.strip().split(' '):
                self.Targets[CurrentTarget]['dependencies'].append(Dep)

            for Command in Commands:
                if Command.startswith('-'):
                    Command = Command[1:]
                Command = self.AddVariablesPrefix(Command)
                self.Targets[CurrentTarget]['commands'].append(Command)
                #print(f"Command Name: {Command}")

    def AddVariablesPrefix(self, Text):
        # Replace all variables with ${uuid_var}
        return re.sub(r'\$\((\w+)\)', r'${' + self.FileUuid + r'_\1}', Text)

    def ReplaceVariableWithValue(self, Text):
        # Replace ${uuid_var} with the actual variable value and handle Ninja's $ escaping
        def Replacer(Match):
            VarName = Match.group(1)
            FullVarName = f"{self.FileUuid}_{VarName}"
            return self.Variables.get(FullVarName, Match.group(0))

        #return re.sub(r'\${' + self.file_uuid + r'_(\w+)}', replacer, text)
        Pattern = re.compile(rf'\$\{{{re.escape(self.FileUuid)}_(\w+)\}}')

        while True:
            NewText = Pattern.sub(Replacer, Text)
            if NewText == Text:
                break
            Text = NewText

        return Text

    def GenerateWindowsNinjaBuild(self, NinjaFilePath):
        with open(NinjaFilePath, 'w') as NinjaFile:
            Writer = ninja_syntax.Writer(NinjaFile)

            # Write variables to the ninja file
            for Var, Value in self.Variables.items():
                if Value != '':
                    Writer.variable(Var, Value)

            RuleSeqNo = 0

            # Write rules to the ninja file
            for Target, Info in self.Targets.items():
                if '\\' in Target:
                    Writer.comment(Target)
                    if Info['commands']:
                        CombinedCommand =  'cmd /c "' + ' && '.join(Info['commands']) + '"'
                        if 'Trim' in CombinedCommand:
                            CombinedCommand = CombinedCommand.replace('Trim', shutil.which('Trim'))
                        Restat = True
                        if CombinedCommand.strip() == '':
                            continue
                        RuleName = f"rule_{self.FileUuid}_{RuleSeqNo}"
                        NasmInc = '${' + f'{self.FileUuid}_NASM_INC' + '}'
                        NasmCmd = '${' + f'{self.FileUuid}_NASM' + '}'
                        if NasmCmd in CombinedCommand:
                            PythonCmd =  os.environ['PYTHON_COMMAND']
                            EdkToolPath = os.environ['EDK_TOOLS_PATH']
                            NasmWrapperCmd = os.path.join(EdkToolPath, 'Scripts', 'NasmWrapper.py ')
                            CombinedCommand = CombinedCommand.replace('"' + NasmCmd + '"', f'{PythonCmd} {NasmWrapperCmd} {NasmCmd}').replace(NasmInc, '@$out.rsp')
                            Writer.rule(RuleName, CombinedCommand, restat=Restat, rspfile="$out.rsp",  rspfile_content=NasmInc)
                        else:
                            Deps = None
                            if 'DEPS_FLAGS' in CombinedCommand:
                                Deps = "msvc"
                            Inc = '${' + f'{self.FileUuid}_INC' + '}'
                            if Inc in CombinedCommand:
                                CombinedCommand = CombinedCommand.replace(Inc, '@$out.rsp')
                                Writer.rule(RuleName, CombinedCommand, restat=Restat, deps=Deps, rspfile="$out.rsp",  rspfile_content=Inc)
                            else:
                                Writer.rule(RuleName, CombinedCommand, restat=Restat, deps=Deps)
                            Writer.newline()
                        Writer.build(Target, RuleName, Info['dependencies'])
                        Writer.newline()
                        RuleSeqNo += 1
                    else:
                        Writer.build(Target, 'phony', Info['dependencies'])
                        Writer.newline()

                    Writer.newline()

            for Target in self.Variables[f'{self.FileUuid}_CODA_TARGET'].split():
                Writer.default(Target)

    def GenerateLinuxNinjaBuild(self, NinjaFilePath):
        with open(NinjaFilePath, 'w') as NinjaFile:
            Writer = ninja_syntax.Writer(NinjaFile)

            # Write variables to the ninja file
            for Var, Value in self.Variables.items():
                if Value != '':
                    Writer.variable(Var, Value)

            RuleSeqNo = 0

            # Write rules to the ninja file
            for Target, Info in self.Targets.items():
                if '/' in Target:
                    Writer.comment(Target)
                    if Info['commands']:
                        CombinedCommand = ' && '.join(Info['commands'])
                        Restat = True
                        if CombinedCommand.strip() == '':
                            continue
                        RuleName = f"rule_{self.FileUuid}_{RuleSeqNo}"
                        Depfile = None
                        Deps = None
                        if 'DEPS_FLAGS' in CombinedCommand:
                            Depfile = "$out.d"
                            Deps = "gcc"
                            DepsFlag = '${' + f"{self.FileUuid}_DEPS_FLAGS" + '}'
                            CombinedCommand = CombinedCommand.replace(DepsFlag, DepsFlag + ' $out.d')
                        Writer.rule(RuleName, CombinedCommand, restat=Restat, depfile=Depfile, deps=Deps)
                        Writer.newline()
                        Writer.build(Target, RuleName, Info['dependencies'])
                        Writer.newline()
                        RuleSeqNo += 1
                    else:
                        Writer.build(Target, 'phony', Info['dependencies'])
                        Writer.newline()

                    Writer.newline()

            for Target in self.Variables[f'{self.FileUuid}_CODA_TARGET'].split():
                Writer.default(Target.strip())

    def GenerateNinjaBuild(self, NinjaFilePath):
        if platform.system() == "Windows":
            self.GenerateWindowsNinjaBuild(NinjaFilePath)
        if platform.system() == "Linux":
            self.GenerateLinuxNinjaBuild(NinjaFilePath)


def GenerateBuildStageNinjaRule(MakeFileContent, NinjaBuildFilePath, NinjaBuildTargetList):
    Parser = MakefileParser(MakeFileContent, NinjaBuildTargetList)
    Parser.Parse()
    Parser.GenerateNinjaBuild(NinjaBuildFilePath)


def GeneratePrebuildStageNinjaRule(RegenerateNinjaFileCmd, MetaFileList):
    if platform.system() == "Windows":
        RegenerateNinjaFileCmd = 'cmd /c ' + RegenerateNinjaFileCmd

    with open(gNinjaBuildFilePath, 'a') as BuildFile:
        Writer = ninja_syntax.Writer(BuildFile)
        Writer.rule(name='REGENERATE_BUILD', command=RegenerateNinjaFileCmd, description='Regenerating build files...', generator=1)
        Writer.build(outputs=gNinjaBuildFilePath, rule='REGENERATE_BUILD', inputs=MetaFileList)


def MergeNinjaFile():
    BuildDir = os.path.dirname(gNinjaBuildFilePath)
    with open(gNinjaBuildFilePath, 'w') as BuildFile:
        Writer = ninja_syntax.Writer(BuildFile)
        Writer.variable('msvc_deps_prefix ', 'Note: including file:')
        for Root, Dirs, Files in os.walk(BuildDir):
            for Filename in Files:
                if Filename == 'build.ninja':
                    FilePath = os.path.join(Root, Filename)
                    if FilePath != gNinjaBuildFilePath:
                        Writer.include(FilePath)


class PostBuildNinjaRuleGenerator:
    def __init__(self, OutputFile):
        self.OutputFile = OutputFile
        self.Rules = {}
        self.Builds = []
        self.FdFiles = []

    def ParseCommand(self, Command):
        """Parse the command and extract options and parameters"""
        Parts = Command.strip().split()
        OutputFile = None
        InputFiles = []
        for Index in range(len(Parts)):
            if Parts[Index] == '-o' and Index + 1 < len(Parts):
                OutputFile = Parts[Index + 1]
            elif Parts[Index] == '-i' and Index + 1 < len(Parts):
                InputFiles.append(Parts[Index + 1])

        if not InputFiles:
            # If no -i option, use the second item after the last '-x' option as input files
            for Index in range(len(Parts) - 1, -1, -1):
                if Parts[Index].startswith('-') and len(Parts[Index]) > 1:
                    InputFiles = Parts[Index + 2:]
                    break

        return Parts[0], OutputFile, InputFiles, Command

    def GetEfiFileNames(self, InputFile):
        """Parse the input file and get all EFI_FILE_NAME values"""
        EfiFileNames = []
        with open(InputFile, 'r') as Fd:
            for Line in Fd:
                Line = Line.strip()
                if Line.startswith('EFI_FILE_NAME'):
                    _, Value = Line.split('=', 1)
                    EfiFileNames.append(Value.strip())
        return EfiFileNames

    def AddRule(self, RuleName, Command):
        """Add a rule"""
        if RuleName not in self.Rules:
            self.Rules[RuleName] = Command

    def AddBuild(self, OutputFile, RuleName, Inputs):
        """Add a build statement"""
        self.Builds.append((OutputFile, RuleName, Inputs))
        if OutputFile.endswith('.fd'):
            self.FdFiles.append(OutputFile)

    def GenerateNinjaFile(self):
        """Generate the Ninja file"""
        with open(self.OutputFile, 'a') as Fd:
            Writer = ninja_syntax.Writer(Fd)

            for RuleName, Command in self.Rules.items():
                Writer.rule(name=RuleName, command=Command)

            for OutputFile, RuleName, Inputs in self.Builds:
                Writer.build(outputs=OutputFile, rule=RuleName, inputs=Inputs)
                Writer.default(OutputFile)

            # Add rule for DisplayFvSpaceInfo
            if self.FdFiles:
                PythonCmd =  os.environ['PYTHON_COMMAND']
                EdkToolPath = os.environ['EDK_TOOLS_PATH']
                FdDir = os.path.dirname(self.FdFiles[0])
                LogFvSpaceInfoFn = os.path.join(FdDir, 'FvSpaceInfo.txt')
                LogFvSpaceInfoCmd = f'{PythonCmd} ' + os.path.join(EdkToolPath, 'Scripts', 'LogFvSpaceInfo.py ') + "-o {}".format(LogFvSpaceInfoFn) + " -i {}".format(FdDir)

                Writer.rule(name="LogFvSpaceInfo", command=LogFvSpaceInfoCmd)
                Writer.build(outputs=LogFvSpaceInfoFn, rule="LogFvSpaceInfo", inputs=self.FdFiles)
                Writer.default(LogFvSpaceInfoFn)

    def ProcessCommands(self, Commands):
        """Process the commands and generate rules and build statements"""
        for Command in Commands:
            Compiler, OutputFile, InputFiles, FullCommand = self.ParseCommand(Command)
            if Compiler == 'GenFv' and InputFiles:
                EfiFileNames = []
                for InputFile in InputFiles:
                    EfiFileNames.extend(self.GetEfiFileNames(InputFile))
                if EfiFileNames:
                    RuleName = f"rule_{os.path.basename(OutputFile).replace('.', '_')}_" + str(uuid.uuid4())[:8]
                    self.AddRule(RuleName, FullCommand)
                    self.AddBuild(OutputFile, RuleName, InputFiles + EfiFileNames)
            elif OutputFile and InputFiles:
                RuleName = f"rule_{os.path.basename(OutputFile).replace('.', '_')}_" + str(uuid.uuid4())[:8]
                self.AddRule(RuleName, FullCommand)
                if 'PatchBinary.py' in FullCommand:
                    InputFilesNew = InputFiles
                    InputFiles = []
                    for InputFile in InputFilesNew:
                        Match = re.match(r"([A-Za-z]:\\.*|/.*):\d", InputFile)
                        if Match:
                            InputFilePath = Match.group(1)
                            InputFiles.append(InputFilePath)
                self.AddBuild(OutputFile, RuleName, InputFiles)


def GeneratePostbuildStageNinjaRule(NinjaCmds):
    Generator = PostBuildNinjaRuleGenerator(gNinjaBuildFilePath)
    Generator.ProcessCommands(NinjaCmds)
    Generator.GenerateNinjaFile()
