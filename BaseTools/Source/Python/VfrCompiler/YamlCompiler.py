from distutils.filelist import FileList
from pickletools import uint8
import sys
import yaml
import logging
import shutil
import subprocess
import time
import re
import argparse
from tkinter.ttk import Treeview
from antlr4 import *
# from VfrSyntaxVisitor import *
# from VfrSyntaxLexer import *
# from VfrSyntaxParser import *
from SourceVfrSyntaxParser import SourceVfrSyntaxParser
from SourceVfrSyntaxVisitor import SourceVfrSyntaxVisitor
from SourceVfrSyntaxLexer import SourceVfrSyntaxLexer
from IfrCommon import *
from YamlTree import *
from IfrPreProcess import *
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from VfrError import *
from Common.LongFilePathSupport import LongFilePath

class COMPILER_RUN_STATUS(Enum):
    STATUS_STARTED = 0
    STATUS_INITIALIZED = 1
    STATUS_VFR_PREPROCESSED = 2
    STATUS_VFR_COMPILEED = 3
    STATUS_VFR_GENBINARY = 4
    STATUS_YAML_GENERATED = 5
    STATUS_YAML_PREPROCESSED = 6
    STATUS_YAML_COMPILED = 7
    STATUS_YAML_DLT_CONSUMED = 8
    STATUS_YAML_GENBINARY = 9
    STATUS_FINISHED = 10
    STATUS_FAILED = 11
    STATUS_DEAD = 12

'''
 This is how we invoke the C preprocessor on the VFR source file
 to resolve #defines, #includes, etc. To make C source files
 shareable between VFR and drivers, define VFRCOMPILE so that
 #ifdefs can be used in shared .h files.
'''
PREPROCESSOR_COMMAND = "cl "
PREPROCESSOR_OPTIONS = "/nologo /E /TC /DVFRCOMPILE "

# Specify the filename extensions for the files we generate.
VFR__FILENAME_EXTENSION = ".vfr"
VFR_PREPROCESS_FILENAME_EXTENSION =  ".i"
VFR_PACKAGE_FILENAME_EXTENSION   =   ".hpk"
VFR_RECORDLIST_FILENAME_EXTENSION  =  ".lst"
VFR_YAML_FILENAME_EXTENSION  =  ".yml"
VFR_JSON_FILENAME_EXTENSION  =  ".json"

WARNING_LOG_LEVEL  = 15
UTILITY_NAME  = 'VfrCompiler'

parser=argparse.ArgumentParser(description="VfrCompiler", epilog= "VfrCompile version {} \
                               Build {} Copyright (c) 2004-2016 Intel Corporation.\
                               All rights reserved.".format(VFR_COMPILER_VERSION, BUILD_VERSION))
parser.add_argument("InputFileName", help = "Input source file name")
parser.add_argument("--vfr", dest ="LanuchVfrCompiler", action="store_true", help = "lanuch VfrCompiler")
parser.add_argument("--yml","--yaml", dest ="LanuchYamlCompiler", action="store_true", help = "lanuch YamlCompiler")
parser.add_argument("--version", action="version", version="VfrCompile version {} Build {}".format(VFR_COMPILER_VERSION, BUILD_VERSION), help="prints version info")
parser.add_argument("-l", dest ="CreateRecordListFile",help = "create an output IFR listing file")
parser.add_argument("-c", dest ="CreateYamlFile",help = "create Yaml file")
parser.add_argument("-j", dest ="CreateJsonFile",help = "create Json file")
# parser.add_argument("-c", dest ="LanuchYamlCompiler",help = "lanuch yaml compiler")
parser.add_argument("-i", dest = "IncludePaths", nargs='+', help= "add path argument") #
parser.add_argument("-o", "--output-directory", "-od", dest = "OutputDirectory", help= "deposit all output files to directory OutputDir, \
                    default is current directory")
parser.add_argument("-oo", "--old-output-directory", "-ood", dest = "OldOutputDirectory", help= "Former directory OutputDir of output files, \
                    default is current directory")
parser.add_argument("-b", "--create-ifr-package", "-ibin", dest = "CreateIfrPkgFile", help = "create an IFR HII pack file")
parser.add_argument("-n", "--no-pre-processing", "-nopp", dest = "SkipCPreprocessor", help = "do not preprocessing input file")
parser.add_argument("-f", "--pre-processing-flag", "-ppflag", dest = "CPreprocessorOptions", help = "C-preprocessor argument") #
parser.add_argument("-s", "--string-db", dest = "StringFileName", help = "input uni string package file")
parser.add_argument("-g", "--guid", dest = "OverrideClassGuid", help = "override class guid input,\
                    format is xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx")
parser.add_argument("-w", "--warning-as-error", dest = "WarningAsError", help = "treat warning as an error")
parser.add_argument("-a", "--autodefault", dest = "AutoDefault", help = "generate default value for question opcode if some default is missing")
parser.add_argument("-d", "--checkdefault", dest = "CheckDefault", help  = "check the default information in a question opcode")
parser.add_argument("-m", "--modulename", dest = "ModuleName", help  = "Module name")
class CmdParser():
    def __init__(self, Args, Argc):
        self.Options = Options()
        self.RunStatus = COMPILER_RUN_STATUS.STATUS_STARTED
        self.PreProcessCmd = PREPROCESSOR_COMMAND
        self.PreProcessOpt = PREPROCESSOR_OPTIONS
        self.OptionIntialization(Args, Argc)

    def OptionIntialization(self, Args, Argc):
        Status = EFI_SUCCESS

        if Argc == 1:
            parser.print_help()
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
            return

        # parse command line
        #if Args.version:
        #    self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
        #    return

        if Args.LanuchVfrCompiler:
            self.Options.LanuchVfrCompiler = True

        if Args.LanuchYamlCompiler:
            self.Options.LanuchYamlCompiler = True

        if Args.CreateRecordListFile:
            self.Options.CreateRecordListFile = True

        if Args.ModuleName:
            self.Options.ModuleName = Args.ModuleName

        if Args.IncludePaths:
            Paths = Args.IncludePaths
            if Paths == None:
                EdkLogger.error("VfrCompiler", OPTION_MISSING, "-i missing path argument")
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
                return
            for Path in Paths:
                self.Options.IncludePaths.append(Path.replace("Ic:", "")[1:])

        if Args.OutputDirectory:
            self.Options.OutputDirectory = Args.OutputDirectory
            if self.Options.OutputDirectory == None:
                EdkLogger.error("VfrCompiler", OPTION_MISSING, "-o missing output directory name")
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
                return
            LastChar = self.Options.OutputDirectory[len(self.Options.OutputDirectory) - 1]
            if LastChar != '/' and LastChar != '\\':
                if self.Options.OutputDirectory.find('/') != -1:
                    self.Options.OutputDirectory += '/'
                    self.Options.DebugDirectory = os.path.dirname(os.path.dirname(self.Options.OutputDirectory)) + '/DEBUG/'
                else:
                    self.Options.OutputDirectory += '\\'
                    self.Options.DebugDirectory = os.path.dirname(os.path.dirname(self.Options.OutputDirectory)) + '\\DEBUG\\'
            EdkLogger.debug(9, "Output Directory {}".format(self.Options.OutputDirectory))

        if Args.OldOutputDirectory:
            self.Options.OldOutputDirectory = Args.OldOutputDirectory
            if self.Options.OldOutputDirectory == None:
                EdkLogger.error("VfrCompiler", OPTION_MISSING, "-oo missing output directory name")
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
                return
            EdkLogger.debug(9, "Old Output Directory {}".format(self.Options.OldOutputDirectory))

        if Args.CreateIfrPkgFile:
            self.Options.CreateIfrPkgFile = True

        if Args.CreateYamlFile:
            self.Options.CreateYamlFile = True

        if Args.CreateJsonFile:
            self.Options.CreateJsonFile = True

        if Args.SkipCPreprocessor:
            self.Options.SkipCPreprocessor = True


        if Args.CPreprocessorOptions:
            Options = Args.CPreprocessorOptions
            if Options == None:
                EdkLogger.error("VfrCompiler", OPTION_MISSING, "-od - missing C-preprocessor argument")
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
                return
            self.Options.CPreprocessorOptions += ' ' + Options

        if Args.StringFileName:
            StringFileName = Args.StringFileName
            if StringFileName == None:
                EdkLogger.error("VfrCompiler", OPTION_MISSING, "-s missing input string file name")
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
                return
            gVfrStringDB.SetStringFileName(StringFileName)
            EdkLogger.debug(9, "Input string file path {}".format(StringFileName))


        if Args.OverrideClassGuid:
            res = StringToGuid(Args.OverrideClassGuid, self.Options.OverrideClassGuid)
            if type(res) == 'int':
                Status  = res
            else:
                Status = res[0]
                self.Options.OverrideClassGuid = res[1]

            if EFI_ERROR(Status):
                EdkLogger.error("VfrCompiler", FORMAT_INVALID, "Invalid format: {}".format(Args.OverrideClassGuid))
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
                return

            self.Options.HasOverrideClassGuid = True

        if Args.WarningAsError:
            self.Options.WarningAsError = True

        if Args.AutoDefault:
            self.Options.AutoDefault = True

        if Args.CheckDefault:
            self.Options.CheckDefault = True

        if Args.InputFileName:
            self.Options.InputFileName = Args.InputFileName
            if self.Options.InputFileName == None:
                EdkLogger.error("VfrCompiler", OPTION_MISSING,"Input file name is not specified.")
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
                return

            if self.Options.OutputDirectory == None:
                self.Options.OutputDirectory = ''

        if self.SetBaseFileName() != 0:
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
            return

        if self.SetPkgOutputFileName() != 0:
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
            return

        if self.SetCOutputFileName() != 0:
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
            return

        if self.SetPreprocessorOutputFileName() != 0:
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
            return

        if self.SetRecordListFileName() != 0:
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
            return

        if self.SetSourceYamlFileName() != 0:
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
            return

        if self.SetJsonFileName() != 0:
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
            return

        if self.SetYamlOutputFileName() != 0:
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
            return

    def SetBaseFileName(self):
        if self.Options.InputFileName == None:
            return -1
        pFileName = self.Options.InputFileName
        while (pFileName.find('\\') != -1) or (pFileName.find('/') != -1):

            if pFileName.find('\\') != -1:
                i = pFileName.find('\\')
            else:
                i = pFileName.find('/')

            if i == len(pFileName) - 1:
                return -1

            pFileName = pFileName[i+1:]

        if pFileName == '' or pFileName.find('.') == -1:
            return -1

        self.Options.BaseFileName = pFileName[:pFileName.find('.')]
        return 0

    def SetPkgOutputFileName(self):
        if self.Options.BaseFileName == None:
            return -1
        if self.Options.LanuchVfrCompiler:
            self.Options.PkgOutputFileName = self.Options.OutputDirectory + 'PyVfr_' + self.Options.BaseFileName  + VFR_PACKAGE_FILENAME_EXTENSION
        else:
            self.Options.PkgOutputFileName = self.Options.OutputDirectory + 'PyYml_' + self.Options.BaseFileName + VFR_PACKAGE_FILENAME_EXTENSION

        return 0

    def SetCOutputFileName(self):
        if self.Options.BaseFileName == None:
            return -1
        if self.Options.LanuchVfrCompiler:
            self.Options.COutputFileName = self.Options.DebugDirectory + 'PyVfr_' + self.Options.BaseFileName  + ".c"
        else:
            self.Options.COutputFileName = self.Options.DebugDirectory + 'PyYml_' + self.Options.BaseFileName  + ".c"

        return 0

    def SetPreprocessorOutputFileName(self):
        if self.Options.BaseFileName == None:
            return -1
        self.Options.PreprocessorOutputFileName = self.Options.OutputDirectory + self.Options.BaseFileName + VFR_PREPROCESS_FILENAME_EXTENSION
        self.Options.ProcessedVfrFileName = self.Options.OutputDirectory + self.Options.BaseFileName + VFR_PREPROCESS_FILENAME_EXTENSION
        return 0

    def SetRecordListFileName(self):
        if self.Options.BaseFileName == None:
            return -1
        if self.Options.LanuchVfrCompiler:
            self.Options.RecordListFileName = self.Options.DebugDirectory + 'PyVfr_' + self.Options.BaseFileName + VFR_RECORDLIST_FILENAME_EXTENSION
        else:
            self.Options.RecordListFileName = self.Options.DebugDirectory + 'PyYml_' + self.Options.BaseFileName + VFR_RECORDLIST_FILENAME_EXTENSION
        return 0

    def SetSourceYamlFileName(self):
        if self.Options.BaseFileName == None:
            return -1
        self.Options.YamlFileName = self.Options.DebugDirectory + self.Options.BaseFileName + VFR_YAML_FILENAME_EXTENSION
        return 0

    def SetJsonFileName(self):
        if self.Options.BaseFileName == None:
            return -1
        self.Options.JsonFileName = self.Options.DebugDirectory + self.Options.BaseFileName + VFR_JSON_FILENAME_EXTENSION
        return 0

    def SetYamlOutputFileName(self):
        if self.Options.BaseFileName == None:
            return -1
        # for test
        self.Options.ProcessedYAMLFileName = self.Options.DebugDirectory + self.Options.BaseFileName + 'Processed.yml'
        self.Options.YamlOutputFileName = self.Options.DebugDirectory + self.Options.BaseFileName + 'Compiled.yml'
        return 0

    def FindIncludeHeaderFile(self, Start, Name): ##########
        FileList = []
        for Relpath, Dirs, Files in os.walk(Start):
            if Name in Files:
                FullPath = os.path.join(Start, Relpath, Name)
                FileList.append(os.path.normpath(os.path.abspath(FullPath)))
        return FileList

    # def CopyFileToOutputDir(self): ############
        # self.Options.InputFileName = self.Options.OutputDirectory + self.Options.BaseFileName + '.vfr'
        # self.Options.ProcessedVfrFileName = self.FindIncludeHeaderFile('/edk2/', self.Options.BaseFileName + VFR_PREPROCESS_FILENAME_EXTENSION)[0]
        # if self.Options.ProcessedVfrFileName == None:
        #     EdkLogger.error("VfrCompiler", FILE_NOT_FOUND,
        #                         "File/directory %s not found in workspace" % (self.Options.BaseFileName + VFR_PREPROCESS_FILENAME_EXTENSION), None)
        # shutil.copyfile(self.Options.InputFileName, self.Options.OutputDirectory + self.Options.BaseFileName + '.vfr')
        # shutil.copyfile(self.Options.ProcessedVfrFileName, self.Options.OutputDirectory + self.Options.BaseFileName + '.i')

    def SET_RUN_STATUS(self, Status):
        self.RunStatus = Status

    def IS_RUN_STATUS(self, Status):
        return self.RunStatus == Status
class YamlCompiler(CmdParser):
    def __init__(self, Cmd: CmdParser):
        self.Options = Cmd.Options
        self.RunStatus = Cmd.RunStatus
        self.YamlRoot = IfrTreeNode()
        self.PreProcessDB = PreProcessDB(self.Options)
        self.YamlTree = YamlTree(self.YamlRoot, self.PreProcessDB, self.Options)
        if (not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)) and (not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)):
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_INITIALIZED)

    def PreProcess(self):
        if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_INITIALIZED):
            if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
        else:
            self.PreProcessDB.Preprocess()
            self.YamlTree.PreProcess()
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_YAML_PREPROCESSED)

    def Compile(self):
        gVfrErrorHandle.SetInputFile(self.Options.YamlFileName)
        gVfrErrorHandle.SetWarningAsError(self.Options.WarningAsError)
        if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_YAML_PREPROCESSED):
            if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
        else:
            self.YamlTree.Compile()
            if self.Options.CreateJsonFile:
                self.YamlTree.IfrTree.DumpJson()
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_YAML_COMPILED)

    def ConsumeDLT(self):
        if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_YAML_COMPILED):
            if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
        else:
            # self.YamlTree.ConsumeDLT()
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_YAML_DLT_CONSUMED)

    def GenBinaryFiles(self):
        if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_YAML_DLT_CONSUMED):
            if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
        else:
            self.Options.CreateRecordListFile = False
            self.YamlTree.GenBinaryFiles()
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FINISHED)

    def SET_RUN_STATUS(self, Status):
        self.RunStatus = Status

    def IS_RUN_STATUS(self, Status):
        return self.RunStatus == Status


class VfrCompiler():

    def __init__(self, Cmd: CmdParser):
        self.Options = Cmd.Options
        self.RunStatus = Cmd.RunStatus
        self.VfrRoot = IfrTreeNode()
        self.PreProcessDB = PreProcessDB(self.Options)
        self.VfrTree = IfrTree(self.VfrRoot, self.PreProcessDB, self.Options)
        if (not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)) and (not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)):
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_INITIALIZED)

    # Parse and collect data structures info in the ExpandedHeader.i files
    def ParseHeader(self):
        try:
            InputStream = FileStream(self.Options.ProcessedVfrFileName)
            VfrLexer = SourceVfrSyntaxLexer(InputStream)
            VfrStream = CommonTokenStream(VfrLexer)
            VfrParser = SourceVfrSyntaxParser(VfrStream)
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                            "File open failed for %s" % self.Options.ExpandedHeaderFileName, None)

        Visitor = SourceVfrSyntaxVisitor()
        #gVfrVarDataTypeDB.Clear()
        Visitor.visit(VfrParser.vfrHeader())
        pNode = gVfrVarDataTypeDB.GetDataTypeList()

    def PreProcess(self):
        if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_INITIALIZED):
            if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
        else:
            if self.Options.SkipCPreprocessor == False:
                # call C precessor first
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_VFR_PREPROCESSED)
            else:
                # makefile will calls commands to generate .i file
                self.PreProcessDB.Preprocess()
                self.ParseHeader()
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_VFR_PREPROCESSED)

    def Compile(self):
        InFileName = self.Options.InputFileName
        if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_VFR_PREPROCESSED):
            EdkLogger.error("VfrCompiler", FILE_PARSE_FAILURE, "compile error in file %s" % InFileName, InFileName)
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
        else:
            gVfrErrorHandle.SetInputFile(InFileName) #
            gVfrErrorHandle.SetWarningAsError(self.Options.WarningAsError)

            try:
                InputStream = FileStream(InFileName)
                VfrLexer = SourceVfrSyntaxLexer(InputStream)
                VfrStream = CommonTokenStream(VfrLexer)
                VfrParser = SourceVfrSyntaxParser(VfrStream)
            except:
                EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                                "File open failed for %s" % InFileName, InFileName)
                if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                    self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
                return
            self.Visitor = SourceVfrSyntaxVisitor(self.PreProcessDB, self.VfrRoot, self.Options.OverrideClassGuid)
            self.Visitor.visit(VfrParser.vfrFormSetDefinition())

            if self.Visitor.ParserStatus != 0:
                if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                    EdkLogger.error("VfrCompiler", FILE_PARSE_FAILURE, "compile error in file %s" % InFileName, InFileName)
                    self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
                return

            if gFormPkg.HavePendingUnassigned() == True:
                gFormPkg.PendingAssignPrintAll()
                if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                    EdkLogger.error("VfrCompiler", FILE_PARSE_FAILURE, "compile error in file %s" % InFileName, InFileName)
                    self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
                return

            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_VFR_COMPILEED)
            if self.Options.CreateJsonFile:
                self.VfrTree.DumpJson()

    def GenBinary(self): # gen hpk file
        if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_VFR_COMPILEED):
            if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
        else:
            if self.Options.CreateIfrPkgFile:
                self.VfrTree.GenBinary()

    def GenCFile(self): #gen c file
        if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_VFR_COMPILEED):
            if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
        else:
            if self.Options.CreateIfrPkgFile:
                self.VfrTree.GenCFile()

    def GenRecordListFile(self):
        if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_VFR_COMPILEED):
            if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
        else:
            if self.Options.CreateRecordListFile:
                self.VfrTree.GenRecordListFile()

    def GenBinaryFiles(self):
        if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_VFR_COMPILEED):
            if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
        else:
            self.VfrTree.GenBinaryFiles()
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FINISHED)

    def DumpSourceYaml(self):
        if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_VFR_COMPILEED) and not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FINISHED):
            if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
        else:
            if self.Options.CreateYamlFile:
                self.VfrTree.DumpSourceYaml()
                # self.VfrTree.DumpCompiledYaml()
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FINISHED)

    def SET_RUN_STATUS(self, Status):
        self.RunStatus = Status

    def IS_RUN_STATUS(self, Status):
        return self.RunStatus == Status

def main():
    Args = parser.parse_args()
    Argc = len(sys.argv)
    EdkLogger.SetLevel(WARNING_LOG_LEVEL)
    Cmd = CmdParser(Args, Argc)
    if Cmd.Options.LanuchVfrCompiler:
        Compiler = VfrCompiler(Cmd)
        Compiler.PreProcess()
        Compiler.Compile()
        Compiler.GenBinaryFiles()
        Compiler.DumpSourceYaml()

    if Cmd.Options.LanuchYamlCompiler:
        Compiler = YamlCompiler(Cmd)
        Compiler.PreProcess()
        Compiler.Compile()
        Compiler.ConsumeDLT()
        Compiler.GenBinaryFiles()

    Status = Compiler.RunStatus
    if Status == COMPILER_RUN_STATUS.STATUS_DEAD or Status == COMPILER_RUN_STATUS.STATUS_FAILED:
        return 2

    return EFI_SUCCESS

if __name__=="__main__":
    exit(main())
