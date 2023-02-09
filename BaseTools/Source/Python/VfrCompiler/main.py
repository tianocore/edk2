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
from VfrSyntaxVisitor import *
from VfrSyntaxLexer import *
from VfrSyntaxParser import *
from VfrCommon import *
from YamlTree import *
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from VfrError import *
from Common.LongFilePathSupport import LongFilePath

class COMPILER_RUN_STATUS(Enum):
    STATUS_STARTED = 0
    STATUS_INITIALIZED = 1
    STATUS_PREPROCESSED = 2
    STATUS_COMPILEED = 3
    STATUS_GENBINARY = 4
    STATUS_FINISHED = 5
    STATUS_FAILED = 6
    STATUS_DEAD = 7

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
parser.add_argument("VfrFileName", help = "Input Vfr file name")
parser.add_argument("--version", action="version", version="VfrCompile version {} Build {}".format(VFR_COMPILER_VERSION, BUILD_VERSION), help="prints version info")
parser.add_argument("-l", dest ="CreateRecordListFile",help = "create an output IFR listing file")
parser.add_argument("-y", dest ="CreateYamlFile",help = "create Yaml file")
parser.add_argument("-j", dest ="CreateJsonFile",help = "create Json file")
parser.add_argument("-c", dest ="CompileYaml",help = "compile Yaml file")
parser.add_argument("-i", dest = "IncludePaths", help= "add path argument") #
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
class VfrCompiler():

    def __init__(self, Args, Argc):

        self.Options = Options()
        self.VfrRoot = VfrTreeNode()
        self.VfrTree = VfrTree(self.VfrRoot, self.Options)
        self.YamlRoot = VfrTreeNode()
        self.YamlTree = YamlTree(self.YamlRoot, self.Options)
        self.RunStatus = COMPILER_RUN_STATUS.STATUS_STARTED
        self.PreProcessCmd = PREPROCESSOR_COMMAND
        self.PreProcessOpt = PREPROCESSOR_OPTIONS
        self.OptionIntialization(Args, Argc)
        if (not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)) and (not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)):
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_INITIALIZED)

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

        if Args.CreateRecordListFile:
            self.Options.CreateRecordListFile = True

        if Args.IncludePaths:
            Path = Args.IncludePaths
            if Path == None:
                EdkLogger.error("VfrCompiler", OPTION_MISSING, "-i missing path argument")
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
                return
            self.Options.IncludePaths = ''
            self.Options.IncludePaths += " -I " + Path

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
                else:
                    self.Options.OutputDirectory += '\\'
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

        if Args.CompileYaml:
            self.Options.CompileYaml = True

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

        if Args.VfrFileName:
            self.Options.VfrFileName = Args.VfrFileName
            if self.Options.VfrFileName == None:
                EdkLogger.error("VfrCompiler", OPTION_MISSING,"VFR file name is not specified.")
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

        if self.SetYamlFileName() != 0:
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
            return

        if self.SetJsonFileName() != 0:
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
            return

        if self.SetProcessedYAMLFileName() != 0:
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
            return


        if self.SetExpandedHeaderFileName() != 0:
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD)
            return

    def SetBaseFileName(self):
        if self.Options.VfrFileName == None:
            return -1
        pFileName = self.Options.VfrFileName
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

        self.Options.VfrBaseFileName = pFileName[:pFileName.find('.')]
        return 0

    def SetPkgOutputFileName(self):
        if self.Options.VfrBaseFileName == None:
            return -1
        self.Options.PkgOutputFileName = self.Options.OutputDirectory + self.Options.VfrBaseFileName + VFR_PACKAGE_FILENAME_EXTENSION
        return 0

    def SetCOutputFileName(self):
        if self.Options.VfrBaseFileName == None:
            return -1
        self.Options.COutputFileName = self.Options.OutputDirectory + self.Options.VfrBaseFileName + ".c"
        return 0

    def SetPreprocessorOutputFileName(self):
        if self.Options.VfrBaseFileName == None:
            return -1
        self.Options.PreprocessorOutputFileName = self.Options.OutputDirectory + self.Options.VfrBaseFileName + VFR_PREPROCESS_FILENAME_EXTENSION
        return 0

    def SetRecordListFileName(self):
        if self.Options.VfrBaseFileName == None:
            return -1
        self.Options.RecordListFileName = self.Options.OutputDirectory + self.Options.VfrBaseFileName + VFR_RECORDLIST_FILENAME_EXTENSION
        return 0

    def SetYamlFileName(self):
        if self.Options.VfrBaseFileName == None:
            return -1
        self.Options.YamlFileName = self.Options.OutputDirectory + self.Options.VfrBaseFileName + VFR_YAML_FILENAME_EXTENSION
        return 0

    def SetJsonFileName(self):
        if self.Options.VfrBaseFileName == None:
            return -1
        self.Options.JsonFileName = self.Options.OutputDirectory + self.Options.VfrBaseFileName + VFR_JSON_FILENAME_EXTENSION
        return 0

    def SetProcessedYAMLFileName(self):
        if self.Options.VfrBaseFileName == None:
            return -1
        self.Options.ProcessedYAMLFileName = self.Options.OutputDirectory + self.Options.VfrBaseFileName + 'Processed.yml'
        return 0

    def SetExpandedHeaderFileName(self):
        if self.Options.VfrBaseFileName == None:
            return -1
        self.Options.ExpandedHeaderFileName = self.Options.OutputDirectory + self.Options.VfrBaseFileName + 'Header' + VFR_PREPROCESS_FILENAME_EXTENSION
        return 0

    def PreProcess(self):
        if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_INITIALIZED):
            if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
        else:
            if self.Options.SkipCPreprocessor == False:  ##### wip
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_PREPROCESSED)
            else:
                try:
                    fFile = open(LongFilePath(self.Options.VfrFileName), mode='r')
                    fFile.close()
                except:
                    EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE, "Error opening the input VFR file")
                    if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                        self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
                    return

                PreProcessCmd = self.PreProcessCmd + " " + self.PreProcessOpt + " "
                if self.Options.IncludePaths != None:
                    PreProcessCmd += self.Options.IncludePaths + " "
                if self.Options.CPreprocessorOptions != None:
                    PreProcessCmd += self.Options.CPreprocessorOptions  + " "
                PreProcessCmd += self.Options.VfrFileName + " > "
                PreProcessCmd += self.Options.PreprocessorOutputFileName

                #PreProcessCmd = '/showIncludes /nologo /E /TC /DVFRCOMPILE /FIRamDiskDxeStrDefs.h  test/test.vfr > test/test.i'
                print(PreProcessCmd)
                if self.ExecuteCmd(PreProcessCmd) != 0:
                    EdkLogger.error("VfrCompiler", FILE_PARSE_FAILURE, "failed to spawn C preprocessor on VFR file {}".format(PreProcessCmd))
                    if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                        self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
                else:
                    self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_PREPROCESSED)

    def Compile(self):
        InFileName = self.Options.VfrFileName if self.Options.SkipCPreprocessor == True\
                else self.Options.PreprocessorOutputFileName #
        if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_PREPROCESSED):
            EdkLogger.error("VfrCompiler", FILE_PARSE_FAILURE, "compile error in file %s" % InFileName, InFileName)
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
        else:
            gVfrErrorHandle.SetInputFile(InFileName) #
            gVfrErrorHandle.SetWarningAsError(self.Options.WarningAsError)

            try:
                InputStream = FileStream(InFileName)
                VfrLexer = VfrSyntaxLexer(InputStream)
                VfrStream = CommonTokenStream(VfrLexer)
                VfrParser = VfrSyntaxParser(VfrStream)
            except:
                EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                                "File open failed for %s" % InFileName, InFileName)
                if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                    self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
                return

            self.Visitor = VfrSyntaxVisitor(self.VfrRoot, self.Options.OverrideClassGuid)
            self.Visitor.visit(VfrParser.vfrProgram())

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

            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_COMPILEED)

    def GenBinary(self): # gen hpk file
        if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_COMPILEED):
            if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
        else:
            if self.Options.CreateIfrPkgFile:
                self.VfrTree.GenBinary()

    def GenCFile(self): #gen c file
        if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_COMPILEED):
            if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
        else:
            if self.Options.CreateIfrPkgFile:
                self.VfrTree.GenCFile()


    def GenRecordListFile(self):
        if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_COMPILEED):
            if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
        else:
            if self.Options.CreateRecordListFile:
                self.VfrTree.GenRecordListFile()

    def GenBinaryFiles(self):
        if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_COMPILEED):
            if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
        else:
            self.VfrTree.GenBinaryFiles()

    def DumpYaml(self):
        if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_COMPILEED):
            if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
        else:
            if self.Options.CreateYamlFile:
                self.VfrTree.DumpYaml()

    def DumpJson(self):
        if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_COMPILEED):
            if not self.IS_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_DEAD):
                self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FAILED)
        else:
            if self.Options.CreateJsonFile:
                self.VfrTree.DumpJson()
            self.SET_RUN_STATUS(COMPILER_RUN_STATUS.STATUS_FINISHED)

    def PreProcessYaml(self): # wip
        self.YamlTree.PreProcess()

    def UpdateYamlWithDLT(self): # wip
        self.YamlTree.UpdateYamlWithDLT()

    def CompileYaml(self): # wip
        self.YamlTree.Compile()

    def SET_RUN_STATUS(self, Status):
        self.RunStatus = Status

    def IS_RUN_STATUS(self, Status):
        return self.RunStatus == Status

    def ExecuteCmd(self,cmd,work_dir=None):
        reError = '(^b"Error)'  #This will match if str(line) starts with the word "Error"
        # self.build_log.log(Log.LOG_DBG, "%s" % cmd)
        failed = False
        if cmd.strip().startswith('None '):
            failed = True
        try:
            start = int(round(time.time() * 1000))
            try:
                if work_dir:
                    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,cwd=work_dir)
                else:
                    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

                while True:
                    line = p.stdout.readline()
                    if not line:
                        break
                    #self.build_log.log(Log.LOG_DBG, line.strip().decode('ascii', errors='ignore'))
                    tmp_str = line.split()
                    error = re.search(reError, str(line)) #Need to do str(line) due to line being a byte object.
                    if line.strip().startswith('- Failed -'.encode(encoding='ascii')) or \
                            (len(tmp_str) >= 2 and tmp_str[0].endswith(':'.encode(encoding='ascii')) \
                            and tmp_str[1] == 'ERROR') or (len(tmp_str) >= 3 \
                            and tmp_str[0] == 'Error' and tmp_str[2] == '-') or \
                            ((len(tmp_str) >= 3) and ('error:'.encode(encoding='ascii') in tmp_str[2])):
                        failed = True
                    if error:
                        #self.build_log.log(Log.LOG_ERR, "Failed, please check log file for more details!")
                        failed = True

                p.communicate()

            except subprocess.CalledProcessError as  e:
                ret = e.returncode
            else:
                ret = 0
            finally:
                pass
                 #self.build_log.log(Log.LOG_DBG, '[cmd=%s]' % cmd)
            end = int(round(time.time() * 1000))

        except Exception as e:
            #self.build_log.log(Log.LOG_ERR, e)
            raise RuntimeError
        else:
            if failed:
                ret = -1
            if ret == 0:
                ret_str = 'SUCCESS'
            else:
                ret_str = 'FAIL'
            #self.build_log.log(Log.LOG_DBG, "\t - %s : %d ms\n" % (ret_str, end - start))
            if ret:
                #self.clean_up_temp_files()
                os._exit(ret)
        return ret

    def CopyFileToOutputDir(self):
        self.Options.ProcessedInFileName = self.VfrTree.FindIncludeHeaderFile('/edk2/', self.Options.VfrBaseFileName + VFR_PREPROCESS_FILENAME_EXTENSION)[0]
        if self.Options.ProcessedInFileName == None:
            EdkLogger.error("VfrCompiler", FILE_NOT_FOUND,
                                "File/directory %s not found in workspace" % (self.Options.VfrBaseFileName + VFR_PREPROCESS_FILENAME_EXTENSION), None)
        shutil.copyfile(self.Options.VfrFileName, self.Options.OutputDirectory + self.Options.VfrBaseFileName + '.vfr')
        shutil.copyfile(self.Options.ProcessedInFileName, self.Options.OutputDirectory + self.Options.VfrBaseFileName + '.i')

def main():
    Args = parser.parse_args()
    Argc = len(sys.argv)
    EdkLogger.SetLevel(WARNING_LOG_LEVEL)
    Compiler = VfrCompiler(Args, Argc)
    Compiler.CopyFileToOutputDir() # for development and testing
    Compiler.PreProcess()
    Compiler.Compile()
    Compiler.GenBinaryFiles()

    # Extended Features
    Compiler.DumpYaml()
    Compiler.DumpJson()
    Compiler.PreProcessYaml()
    Compiler.UpdateYamlWithDLT()
    # Compiler.CompileYaml()

    Status = Compiler.RunStatus
    if Status == COMPILER_RUN_STATUS.STATUS_DEAD or Status == COMPILER_RUN_STATUS.STATUS_FAILED:
        return 2

    return Status

if __name__=="__main__":
    exit(main())
