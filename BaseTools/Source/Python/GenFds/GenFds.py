## @file
# generate flash image
#
#  Copyright (c) 2007 - 2019, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import print_function
from __future__ import absolute_import
from re import compile
from optparse import OptionParser
from sys import exit
from glob import glob
from struct import unpack
from linecache import getlines
from io import BytesIO

import Common.LongFilePathOs as os
from Common.TargetTxtClassObject import TargetTxtDict
from Common.DataType import *
import Common.GlobalData as GlobalData
from Common import EdkLogger
from Common.StringUtils import NormPath
from Common.Misc import DirCache, PathClass, GuidStructureStringToGuidString
from Common.Misc import SaveFileOnChange, ClearDuplicatedInf
from Common.BuildVersion import gBUILD_VERSION
from Common.MultipleWorkspace import MultipleWorkspace as mws
from Common.BuildToolError import FatalError, GENFDS_ERROR, CODE_ERROR, FORMAT_INVALID, RESOURCE_NOT_AVAILABLE, FILE_NOT_FOUND, OPTION_MISSING, FORMAT_NOT_SUPPORTED, OPTION_VALUE_INVALID, PARAMETER_INVALID
from Workspace.WorkspaceDatabase import WorkspaceDatabase

from .FdfParser import FdfParser, Warning
from .GenFdsGlobalVariable import GenFdsGlobalVariable
from .FfsFileStatement import FileStatement
import Common.DataType as DataType
from struct import Struct

## Version and Copyright
versionNumber = "1.0" + ' ' + gBUILD_VERSION
__version__ = "%prog Version " + versionNumber
__copyright__ = "Copyright (c) 2007 - 2018, Intel Corporation  All rights reserved."

## Tool entrance method
#
# This method mainly dispatch specific methods per the command line options.
# If no error found, return zero value so the caller of this tool can know
# if it's executed successfully or not.
#
#   @retval 0     Tool was successful
#   @retval 1     Tool failed
#
def main():
    global Options
    Options = myOptionParser()
    EdkLogger.Initialize()
    return GenFdsApi(OptionsToCommandDict(Options))

def resetFdsGlobalVariable():
    GenFdsGlobalVariable.FvDir = ''
    GenFdsGlobalVariable.OutputDirDict = {}
    GenFdsGlobalVariable.BinDir = ''
    # will be FvDir + os.sep + 'Ffs'
    GenFdsGlobalVariable.FfsDir = ''
    GenFdsGlobalVariable.FdfParser = None
    GenFdsGlobalVariable.LibDir = ''
    GenFdsGlobalVariable.WorkSpace = None
    GenFdsGlobalVariable.WorkSpaceDir = ''
    GenFdsGlobalVariable.ConfDir = ''
    GenFdsGlobalVariable.OutputDirFromDscDict = {}
    GenFdsGlobalVariable.TargetName = ''
    GenFdsGlobalVariable.ToolChainTag = ''
    GenFdsGlobalVariable.RuleDict = {}
    GenFdsGlobalVariable.ArchList = None
    GenFdsGlobalVariable.ActivePlatform = None
    GenFdsGlobalVariable.FvAddressFileName = ''
    GenFdsGlobalVariable.VerboseMode = False
    GenFdsGlobalVariable.DebugLevel = -1
    GenFdsGlobalVariable.SharpCounter = 0
    GenFdsGlobalVariable.SharpNumberPerLine = 40
    GenFdsGlobalVariable.FdfFile = ''
    GenFdsGlobalVariable.FdfFileTimeStamp = 0
    GenFdsGlobalVariable.FixedLoadAddress = False
    GenFdsGlobalVariable.PlatformName = ''

    GenFdsGlobalVariable.BuildRuleFamily = DataType.TAB_COMPILER_MSFT
    GenFdsGlobalVariable.ToolChainFamily = DataType.TAB_COMPILER_MSFT
    GenFdsGlobalVariable.__BuildRuleDatabase = None
    GenFdsGlobalVariable.GuidToolDefinition = {}
    GenFdsGlobalVariable.FfsCmdDict = {}
    GenFdsGlobalVariable.SecCmdList = []
    GenFdsGlobalVariable.CopyList   = []
    GenFdsGlobalVariable.ModuleFile = ''
    GenFdsGlobalVariable.EnableGenfdsMultiThread = True

    GenFdsGlobalVariable.LargeFileInFvFlags = []
    GenFdsGlobalVariable.EFI_FIRMWARE_FILE_SYSTEM3_GUID = '5473C07A-3DCB-4dca-BD6F-1E9689E7349A'
    GenFdsGlobalVariable.LARGE_FILE_SIZE = 0x1000000

    GenFdsGlobalVariable.SectionHeader = Struct("3B 1B")

    # FvName, FdName, CapName in FDF, Image file name
    GenFdsGlobalVariable.ImageBinDict = {}

def GenFdsApi(FdsCommandDict, WorkSpaceDataBase=None):
    global Workspace
    Workspace = ""
    ArchList = None
    ReturnCode = 0
    resetFdsGlobalVariable()

    try:
        if FdsCommandDict.get("verbose"):
            EdkLogger.SetLevel(EdkLogger.VERBOSE)
            GenFdsGlobalVariable.VerboseMode = True

        if FdsCommandDict.get("FixedAddress"):
            GenFdsGlobalVariable.FixedLoadAddress = True

        if FdsCommandDict.get("quiet"):
            EdkLogger.SetLevel(EdkLogger.QUIET)
        if FdsCommandDict.get("debug"):
            EdkLogger.SetLevel(FdsCommandDict.get("debug") + 1)
            GenFdsGlobalVariable.DebugLevel = FdsCommandDict.get("debug")
        else:
            EdkLogger.SetLevel(EdkLogger.INFO)

        if not FdsCommandDict.get("Workspace",os.environ.get('WORKSPACE')):
            EdkLogger.error("GenFds", OPTION_MISSING, "WORKSPACE not defined",
                            ExtraData="Please use '-w' switch to pass it or set the WORKSPACE environment variable.")
        elif not os.path.exists(FdsCommandDict.get("Workspace",os.environ.get('WORKSPACE'))):
            EdkLogger.error("GenFds", PARAMETER_INVALID, "WORKSPACE is invalid",
                            ExtraData="Please use '-w' switch to pass it or set the WORKSPACE environment variable.")
        else:
            Workspace = os.path.normcase(FdsCommandDict.get("Workspace",os.environ.get('WORKSPACE')))
            GenFdsGlobalVariable.WorkSpaceDir = Workspace
            if FdsCommandDict.get("debug"):
                GenFdsGlobalVariable.VerboseLogger("Using Workspace:" + Workspace)
            if FdsCommandDict.get("GenfdsMultiThread"):
                GenFdsGlobalVariable.EnableGenfdsMultiThread = True
            else:
                GenFdsGlobalVariable.EnableGenfdsMultiThread = False
        os.chdir(GenFdsGlobalVariable.WorkSpaceDir)

        # set multiple workspace
        PackagesPath = os.getenv("PACKAGES_PATH")
        mws.setWs(GenFdsGlobalVariable.WorkSpaceDir, PackagesPath)

        if FdsCommandDict.get("fdf_file"):
            FdfFilename = FdsCommandDict.get("fdf_file")[0].Path
            FdfFilename = GenFdsGlobalVariable.ReplaceWorkspaceMacro(FdfFilename)

            if FdfFilename[0:2] == '..':
                FdfFilename = os.path.realpath(FdfFilename)
            if not os.path.isabs(FdfFilename):
                FdfFilename = mws.join(GenFdsGlobalVariable.WorkSpaceDir, FdfFilename)
            if not os.path.exists(FdfFilename):
                EdkLogger.error("GenFds", FILE_NOT_FOUND, ExtraData=FdfFilename)

            GenFdsGlobalVariable.FdfFile = FdfFilename
            GenFdsGlobalVariable.FdfFileTimeStamp = os.path.getmtime(FdfFilename)
        else:
            EdkLogger.error("GenFds", OPTION_MISSING, "Missing FDF filename")

        if FdsCommandDict.get("build_target"):
            GenFdsGlobalVariable.TargetName = FdsCommandDict.get("build_target")

        if FdsCommandDict.get("toolchain_tag"):
            GenFdsGlobalVariable.ToolChainTag = FdsCommandDict.get("toolchain_tag")

        if FdsCommandDict.get("active_platform"):
            ActivePlatform = FdsCommandDict.get("active_platform")
            ActivePlatform = GenFdsGlobalVariable.ReplaceWorkspaceMacro(ActivePlatform)

            if ActivePlatform[0:2] == '..':
                ActivePlatform = os.path.realpath(ActivePlatform)

            if not os.path.isabs (ActivePlatform):
                ActivePlatform = mws.join(GenFdsGlobalVariable.WorkSpaceDir, ActivePlatform)

            if not os.path.exists(ActivePlatform):
                EdkLogger.error("GenFds", FILE_NOT_FOUND, "ActivePlatform doesn't exist!")
        else:
            EdkLogger.error("GenFds", OPTION_MISSING, "Missing active platform")

        GenFdsGlobalVariable.ActivePlatform = PathClass(NormPath(ActivePlatform))

        if FdsCommandDict.get("conf_directory"):
            # Get alternate Conf location, if it is absolute, then just use the absolute directory name
            ConfDirectoryPath = os.path.normpath(FdsCommandDict.get("conf_directory"))
            if ConfDirectoryPath.startswith('"'):
                ConfDirectoryPath = ConfDirectoryPath[1:]
            if ConfDirectoryPath.endswith('"'):
                ConfDirectoryPath = ConfDirectoryPath[:-1]
            if not os.path.isabs(ConfDirectoryPath):
                # Since alternate directory name is not absolute, the alternate directory is located within the WORKSPACE
                # This also handles someone specifying the Conf directory in the workspace. Using --conf=Conf
                ConfDirectoryPath = os.path.join(GenFdsGlobalVariable.WorkSpaceDir, ConfDirectoryPath)
        else:
            if "CONF_PATH" in os.environ:
                ConfDirectoryPath = os.path.normcase(os.environ["CONF_PATH"])
            else:
                # Get standard WORKSPACE/Conf, use the absolute path to the WORKSPACE/Conf
                ConfDirectoryPath = mws.join(GenFdsGlobalVariable.WorkSpaceDir, 'Conf')
        GenFdsGlobalVariable.ConfDir = ConfDirectoryPath
        if not GlobalData.gConfDirectory:
            GlobalData.gConfDirectory = GenFdsGlobalVariable.ConfDir
        BuildConfigurationFile = os.path.normpath(os.path.join(ConfDirectoryPath, "target.txt"))
        if os.path.isfile(BuildConfigurationFile) == True:
            # if no build target given in command line, get it from target.txt
            TargetObj = TargetTxtDict()
            TargetTxt = TargetObj.Target
            if not GenFdsGlobalVariable.TargetName:
                BuildTargetList = TargetTxt.TargetTxtDictionary[TAB_TAT_DEFINES_TARGET]
                if len(BuildTargetList) != 1:
                    EdkLogger.error("GenFds", OPTION_VALUE_INVALID, ExtraData="Only allows one instance for Target.")
                GenFdsGlobalVariable.TargetName = BuildTargetList[0]

            # if no tool chain given in command line, get it from target.txt
            if not GenFdsGlobalVariable.ToolChainTag:
                ToolChainList = TargetTxt.TargetTxtDictionary[TAB_TAT_DEFINES_TOOL_CHAIN_TAG]
                if ToolChainList is None or len(ToolChainList) == 0:
                    EdkLogger.error("GenFds", RESOURCE_NOT_AVAILABLE, ExtraData="No toolchain given. Don't know how to build.")
                if len(ToolChainList) != 1:
                    EdkLogger.error("GenFds", OPTION_VALUE_INVALID, ExtraData="Only allows one instance for ToolChain.")
                GenFdsGlobalVariable.ToolChainTag = ToolChainList[0]
        else:
            EdkLogger.error("GenFds", FILE_NOT_FOUND, ExtraData=BuildConfigurationFile)

        #Set global flag for build mode
        GlobalData.gIgnoreSource = FdsCommandDict.get("IgnoreSources")

        if FdsCommandDict.get("macro"):
            for Pair in FdsCommandDict.get("macro"):
                if Pair.startswith('"'):
                    Pair = Pair[1:]
                if Pair.endswith('"'):
                    Pair = Pair[:-1]
                List = Pair.split('=')
                if len(List) == 2:
                    if not List[1].strip():
                        EdkLogger.error("GenFds", OPTION_VALUE_INVALID, ExtraData="No Value given for Macro %s" %List[0])
                    if List[0].strip() in ["WORKSPACE", "TARGET", "TOOLCHAIN"]:
                        GlobalData.gGlobalDefines[List[0].strip()] = List[1].strip()
                    else:
                        GlobalData.gCommandLineDefines[List[0].strip()] = List[1].strip()
                else:
                    GlobalData.gCommandLineDefines[List[0].strip()] = "TRUE"
        os.environ["WORKSPACE"] = Workspace

        # Use the -t and -b option as gGlobalDefines's TOOLCHAIN and TARGET if they are not defined
        if "TARGET" not in GlobalData.gGlobalDefines:
            GlobalData.gGlobalDefines["TARGET"] = GenFdsGlobalVariable.TargetName
        if "TOOLCHAIN" not in GlobalData.gGlobalDefines:
            GlobalData.gGlobalDefines["TOOLCHAIN"] = GenFdsGlobalVariable.ToolChainTag
        if "TOOL_CHAIN_TAG" not in GlobalData.gGlobalDefines:
            GlobalData.gGlobalDefines['TOOL_CHAIN_TAG'] = GenFdsGlobalVariable.ToolChainTag

        """call Workspace build create database"""
        GlobalData.gDatabasePath = os.path.normpath(os.path.join(ConfDirectoryPath, GlobalData.gDatabasePath))

        if WorkSpaceDataBase:
            BuildWorkSpace = WorkSpaceDataBase
        else:
            BuildWorkSpace = WorkspaceDatabase()
        #
        # Get files real name in workspace dir
        #
        GlobalData.gAllFiles = DirCache(Workspace)
        GlobalData.gWorkspace = Workspace

        if FdsCommandDict.get("build_architecture_list"):
            ArchList = FdsCommandDict.get("build_architecture_list").split(',')
        else:
            ArchList = BuildWorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, TAB_COMMON, FdsCommandDict.get("build_target"), FdsCommandDict.get("toolchain_tag")].SupArchList

        TargetArchList = set(BuildWorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, TAB_COMMON, FdsCommandDict.get("build_target"), FdsCommandDict.get("toolchain_tag")].SupArchList) & set(ArchList)
        if len(TargetArchList) == 0:
            EdkLogger.error("GenFds", GENFDS_ERROR, "Target ARCH %s not in platform supported ARCH %s" % (str(ArchList), str(BuildWorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, TAB_COMMON].SupArchList)))

        for Arch in ArchList:
            GenFdsGlobalVariable.OutputDirFromDscDict[Arch] = NormPath(BuildWorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, Arch, FdsCommandDict.get("build_target"), FdsCommandDict.get("toolchain_tag")].OutputDirectory)

        # assign platform name based on last entry in ArchList
        GenFdsGlobalVariable.PlatformName = BuildWorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, ArchList[-1], FdsCommandDict.get("build_target"), FdsCommandDict.get("toolchain_tag")].PlatformName

        if FdsCommandDict.get("platform_build_directory"):
            OutputDirFromCommandLine = GenFdsGlobalVariable.ReplaceWorkspaceMacro(FdsCommandDict.get("platform_build_directory"))
            if not os.path.isabs (OutputDirFromCommandLine):
                OutputDirFromCommandLine = os.path.join(GenFdsGlobalVariable.WorkSpaceDir, OutputDirFromCommandLine)
            for Arch in ArchList:
                GenFdsGlobalVariable.OutputDirDict[Arch] = OutputDirFromCommandLine
        else:
            for Arch in ArchList:
                GenFdsGlobalVariable.OutputDirDict[Arch] = os.path.join(GenFdsGlobalVariable.OutputDirFromDscDict[Arch], GenFdsGlobalVariable.TargetName + '_' + GenFdsGlobalVariable.ToolChainTag)

        for Key in GenFdsGlobalVariable.OutputDirDict:
            OutputDir = GenFdsGlobalVariable.OutputDirDict[Key]
            if OutputDir[0:2] == '..':
                OutputDir = os.path.realpath(OutputDir)

            if OutputDir[1] != ':':
                OutputDir = os.path.join (GenFdsGlobalVariable.WorkSpaceDir, OutputDir)

            if not os.path.exists(OutputDir):
                EdkLogger.error("GenFds", FILE_NOT_FOUND, ExtraData=OutputDir)
            GenFdsGlobalVariable.OutputDirDict[Key] = OutputDir

        """ Parse Fdf file, has to place after build Workspace as FDF may contain macros from DSC file """
        if WorkSpaceDataBase:
            FdfParserObj = GlobalData.gFdfParser
        else:
            FdfParserObj = FdfParser(FdfFilename)
            FdfParserObj.ParseFile()

        if FdfParserObj.CycleReferenceCheck():
            EdkLogger.error("GenFds", FORMAT_NOT_SUPPORTED, "Cycle Reference Detected in FDF file")

        if FdsCommandDict.get("fd"):
            if FdsCommandDict.get("fd")[0].upper() in FdfParserObj.Profile.FdDict:
                GenFds.OnlyGenerateThisFd = FdsCommandDict.get("fd")[0]
            else:
                EdkLogger.error("GenFds", OPTION_VALUE_INVALID,
                                "No such an FD in FDF file: %s" % FdsCommandDict.get("fd")[0])

        if FdsCommandDict.get("fv"):
            if FdsCommandDict.get("fv")[0].upper() in FdfParserObj.Profile.FvDict:
                GenFds.OnlyGenerateThisFv = FdsCommandDict.get("fv")[0]
            else:
                EdkLogger.error("GenFds", OPTION_VALUE_INVALID,
                                "No such an FV in FDF file: %s" % FdsCommandDict.get("fv")[0])

        if FdsCommandDict.get("cap"):
            if FdsCommandDict.get("cap")[0].upper() in FdfParserObj.Profile.CapsuleDict:
                GenFds.OnlyGenerateThisCap = FdsCommandDict.get("cap")[0]
            else:
                EdkLogger.error("GenFds", OPTION_VALUE_INVALID,
                                "No such a Capsule in FDF file: %s" % FdsCommandDict.get("cap")[0])

        GenFdsGlobalVariable.WorkSpace = BuildWorkSpace
        if ArchList:
            GenFdsGlobalVariable.ArchList = ArchList

        # Dsc Build Data will handle Pcd Settings from CommandLine.

        """Modify images from build output if the feature of loading driver at fixed address is on."""
        if GenFdsGlobalVariable.FixedLoadAddress:
            GenFds.PreprocessImage(BuildWorkSpace, GenFdsGlobalVariable.ActivePlatform)

        # Record the FV Region info that may specific in the FD
        if FdfParserObj.Profile.FvDict and FdfParserObj.Profile.FdDict:
            for FvObj in FdfParserObj.Profile.FvDict.values():
                for FdObj in FdfParserObj.Profile.FdDict.values():
                    for RegionObj in FdObj.RegionList:
                        if RegionObj.RegionType != BINARY_FILE_TYPE_FV:
                            continue
                        for RegionData in RegionObj.RegionDataList:
                            if FvObj.UiFvName.upper() == RegionData.upper():
                                if not FvObj.BaseAddress:
                                    FvObj.BaseAddress = '0x%x' % (int(FdObj.BaseAddress, 0) + RegionObj.Offset)
                                if FvObj.FvRegionInFD:
                                    if FvObj.FvRegionInFD != RegionObj.Size:
                                        EdkLogger.error("GenFds", FORMAT_INVALID, "The FV %s's region is specified in multiple FD with different value." %FvObj.UiFvName)
                                else:
                                    FvObj.FvRegionInFD = RegionObj.Size
                                    RegionObj.BlockInfoOfRegion(FdObj.BlockSizeList, FvObj)

        """Call GenFds"""
        GenFds.GenFd('', FdfParserObj, BuildWorkSpace, ArchList)

        """Generate GUID cross reference file"""
        GenFds.GenerateGuidXRefFile(BuildWorkSpace, ArchList, FdfParserObj)

        """Display FV space info."""
        GenFds.DisplayFvSpaceInfo(FdfParserObj)

    except Warning as X:
        EdkLogger.error(X.ToolName, FORMAT_INVALID, File=X.FileName, Line=X.LineNumber, ExtraData=X.Message, RaiseError=False)
        ReturnCode = FORMAT_INVALID
    except FatalError as X:
        if FdsCommandDict.get("debug") is not None:
            import traceback
            EdkLogger.quiet(traceback.format_exc())
        ReturnCode = X.args[0]
    except:
        import traceback
        EdkLogger.error(
                    "\nPython",
                    CODE_ERROR,
                    "Tools code failure",
                    ExtraData="Please send email to %s for help, attaching following call stack trace!\n" % MSG_EDKII_MAIL_ADDR,
                    RaiseError=False
                    )
        EdkLogger.quiet(traceback.format_exc())
        ReturnCode = CODE_ERROR
    finally:
        ClearDuplicatedInf()
    return ReturnCode

def OptionsToCommandDict(Options):
    FdsCommandDict = {}
    FdsCommandDict["verbose"] = Options.verbose
    FdsCommandDict["FixedAddress"] = Options.FixedAddress
    FdsCommandDict["quiet"] = Options.quiet
    FdsCommandDict["debug"] = Options.debug
    FdsCommandDict["Workspace"] = Options.Workspace
    FdsCommandDict["GenfdsMultiThread"] = not Options.NoGenfdsMultiThread
    FdsCommandDict["fdf_file"] = [PathClass(Options.filename)] if Options.filename else []
    FdsCommandDict["build_target"] = Options.BuildTarget
    FdsCommandDict["toolchain_tag"] = Options.ToolChain
    FdsCommandDict["active_platform"] = Options.activePlatform
    FdsCommandDict["OptionPcd"] = Options.OptionPcd
    FdsCommandDict["conf_directory"] = Options.ConfDirectory
    FdsCommandDict["IgnoreSources"] = Options.IgnoreSources
    FdsCommandDict["macro"] = Options.Macros
    FdsCommandDict["build_architecture_list"] = Options.archList
    FdsCommandDict["platform_build_directory"] = Options.outputDir
    FdsCommandDict["fd"] = [Options.uiFdName] if Options.uiFdName else []
    FdsCommandDict["fv"] = [Options.uiFvName] if Options.uiFvName else []
    FdsCommandDict["cap"] = [Options.uiCapName] if Options.uiCapName else []
    return FdsCommandDict


gParamCheck = []
def SingleCheckCallback(option, opt_str, value, parser):
    if option not in gParamCheck:
        setattr(parser.values, option.dest, value)
        gParamCheck.append(option)
    else:
        parser.error("Option %s only allows one instance in command line!" % option)

## Parse command line options
#
# Using standard Python module optparse to parse command line option of this tool.
#
#   @retval Opt   A optparse.Values object containing the parsed options
#
def myOptionParser():
    usage = "%prog [options] -f input_file -a arch_list -b build_target -p active_platform -t tool_chain_tag -D \"MacroName [= MacroValue]\""
    Parser = OptionParser(usage=usage, description=__copyright__, version="%prog " + str(versionNumber))
    Parser.add_option("-f", "--file", dest="filename", type="string", help="Name of FDF file to convert", action="callback", callback=SingleCheckCallback)
    Parser.add_option("-a", "--arch", dest="archList", help="comma separated list containing one or more of: IA32, X64, IPF, ARM, AARCH64 or EBC which should be built, overrides target.txt?s TARGET_ARCH")
    Parser.add_option("-q", "--quiet", action="store_true", type=None, help="Disable all messages except FATAL ERRORS.")
    Parser.add_option("-v", "--verbose", action="store_true", type=None, help="Turn on verbose output with informational messages printed.")
    Parser.add_option("-d", "--debug", action="store", type="int", help="Enable debug messages at specified level.")
    Parser.add_option("-p", "--platform", type="string", dest="activePlatform", help="Set the ACTIVE_PLATFORM, overrides target.txt ACTIVE_PLATFORM setting.",
                      action="callback", callback=SingleCheckCallback)
    Parser.add_option("-w", "--workspace", type="string", dest="Workspace", default=os.environ.get('WORKSPACE'), help="Set the WORKSPACE",
                      action="callback", callback=SingleCheckCallback)
    Parser.add_option("-o", "--outputDir", type="string", dest="outputDir", help="Name of Build Output directory",
                      action="callback", callback=SingleCheckCallback)
    Parser.add_option("-r", "--rom_image", dest="uiFdName", help="Build the image using the [FD] section named by FdUiName.")
    Parser.add_option("-i", "--FvImage", dest="uiFvName", help="Build the FV image using the [FV] section named by UiFvName")
    Parser.add_option("-C", "--CapsuleImage", dest="uiCapName", help="Build the Capsule image using the [Capsule] section named by UiCapName")
    Parser.add_option("-b", "--buildtarget", type="string", dest="BuildTarget", help="Set the build TARGET, overrides target.txt TARGET setting.",
                      action="callback", callback=SingleCheckCallback)
    Parser.add_option("-t", "--tagname", type="string", dest="ToolChain", help="Using the tools: TOOL_CHAIN_TAG name to build the platform.",
                      action="callback", callback=SingleCheckCallback)
    Parser.add_option("-D", "--define", action="append", type="string", dest="Macros", help="Macro: \"Name [= Value]\".")
    Parser.add_option("-s", "--specifyaddress", dest="FixedAddress", action="store_true", type=None, help="Specify driver load address.")
    Parser.add_option("--conf", action="store", type="string", dest="ConfDirectory", help="Specify the customized Conf directory.")
    Parser.add_option("--ignore-sources", action="store_true", dest="IgnoreSources", default=False, help="Focus to a binary build and ignore all source files")
    Parser.add_option("--pcd", action="append", dest="OptionPcd", help="Set PCD value by command line. Format: \"PcdName=Value\" ")
    Parser.add_option("--genfds-multi-thread", action="store_true", dest="GenfdsMultiThread", default=True, help="Enable GenFds multi thread to generate ffs file.")
    Parser.add_option("--no-genfds-multi-thread", action="store_true", dest="NoGenfdsMultiThread", default=False, help="Disable GenFds multi thread to generate ffs file.")

    Options, _ = Parser.parse_args()
    return Options

## The class implementing the EDK2 flash image generation process
#
#   This process includes:
#       1. Collect workspace information, includes platform and module information
#       2. Call methods of Fd class to generate FD
#       3. Call methods of Fv class to generate FV that not belong to FD
#
class GenFds(object):
    FdfParsef = None
    OnlyGenerateThisFd = None
    OnlyGenerateThisFv = None
    OnlyGenerateThisCap = None

    ## GenFd()
    #
    #   @param  OutputDir           Output directory
    #   @param  FdfParserObject     FDF contents parser
    #   @param  Workspace           The directory of workspace
    #   @param  ArchList            The Arch list of platform
    #
    @staticmethod
    def GenFd (OutputDir, FdfParserObject, WorkSpace, ArchList):
        GenFdsGlobalVariable.SetDir ('', FdfParserObject, WorkSpace, ArchList)

        GenFdsGlobalVariable.VerboseLogger(" Generate all Fd images and their required FV and Capsule images!")
        if GenFds.OnlyGenerateThisCap is not None and GenFds.OnlyGenerateThisCap.upper() in GenFdsGlobalVariable.FdfParser.Profile.CapsuleDict:
            CapsuleObj = GenFdsGlobalVariable.FdfParser.Profile.CapsuleDict[GenFds.OnlyGenerateThisCap.upper()]
            if CapsuleObj is not None:
                CapsuleObj.GenCapsule()
                return

        if GenFds.OnlyGenerateThisFd is not None and GenFds.OnlyGenerateThisFd.upper() in GenFdsGlobalVariable.FdfParser.Profile.FdDict:
            FdObj = GenFdsGlobalVariable.FdfParser.Profile.FdDict[GenFds.OnlyGenerateThisFd.upper()]
            if FdObj is not None:
                FdObj.GenFd()
                return
        elif GenFds.OnlyGenerateThisFd is None and GenFds.OnlyGenerateThisFv is None:
            for FdObj in GenFdsGlobalVariable.FdfParser.Profile.FdDict.values():
                FdObj.GenFd()

        GenFdsGlobalVariable.VerboseLogger("\n Generate other FV images! ")
        if GenFds.OnlyGenerateThisFv is not None and GenFds.OnlyGenerateThisFv.upper() in GenFdsGlobalVariable.FdfParser.Profile.FvDict:
            FvObj = GenFdsGlobalVariable.FdfParser.Profile.FvDict[GenFds.OnlyGenerateThisFv.upper()]
            if FvObj is not None:
                Buffer = BytesIO()
                FvObj.AddToBuffer(Buffer)
                Buffer.close()
                return
        elif GenFds.OnlyGenerateThisFv is None:
            for FvObj in GenFdsGlobalVariable.FdfParser.Profile.FvDict.values():
                Buffer = BytesIO()
                FvObj.AddToBuffer(Buffer)
                Buffer.close()

        if GenFds.OnlyGenerateThisFv is None and GenFds.OnlyGenerateThisFd is None and GenFds.OnlyGenerateThisCap is None:
            if GenFdsGlobalVariable.FdfParser.Profile.CapsuleDict != {}:
                GenFdsGlobalVariable.VerboseLogger("\n Generate other Capsule images!")
                for CapsuleObj in GenFdsGlobalVariable.FdfParser.Profile.CapsuleDict.values():
                    CapsuleObj.GenCapsule()

            if GenFdsGlobalVariable.FdfParser.Profile.OptRomDict != {}:
                GenFdsGlobalVariable.VerboseLogger("\n Generate all Option ROM!")
                for OptRomObj in GenFdsGlobalVariable.FdfParser.Profile.OptRomDict.values():
                    OptRomObj.AddToBuffer(None)

    @staticmethod
    def GenFfsMakefile(OutputDir, FdfParserObject, WorkSpace, ArchList, GlobalData):
        GenFdsGlobalVariable.SetEnv(FdfParserObject, WorkSpace, ArchList, GlobalData)
        for FdObj in GenFdsGlobalVariable.FdfParser.Profile.FdDict.values():
            FdObj.GenFd(Flag=True)

        for FvObj in GenFdsGlobalVariable.FdfParser.Profile.FvDict.values():
            FvObj.AddToBuffer(Buffer=None, Flag=True)

        if GenFdsGlobalVariable.FdfParser.Profile.OptRomDict != {}:
            for OptRomObj in GenFdsGlobalVariable.FdfParser.Profile.OptRomDict.values():
                OptRomObj.AddToBuffer(Buffer=None, Flag=True)

        return GenFdsGlobalVariable.FfsCmdDict

    ## GetFvBlockSize()
    #
    #   @param  FvObj           Whose block size to get
    #   @retval int             Block size value
    #
    @staticmethod
    def GetFvBlockSize(FvObj):
        DefaultBlockSize = 0x1
        FdObj = None
        if GenFds.OnlyGenerateThisFd is not None and GenFds.OnlyGenerateThisFd.upper() in GenFdsGlobalVariable.FdfParser.Profile.FdDict:
            FdObj = GenFdsGlobalVariable.FdfParser.Profile.FdDict[GenFds.OnlyGenerateThisFd.upper()]
        if FdObj is None:
            for ElementFd in GenFdsGlobalVariable.FdfParser.Profile.FdDict.values():
                for ElementRegion in ElementFd.RegionList:
                    if ElementRegion.RegionType == BINARY_FILE_TYPE_FV:
                        for ElementRegionData in ElementRegion.RegionDataList:
                            if ElementRegionData is not None and ElementRegionData.upper() == FvObj.UiFvName:
                                if FvObj.BlockSizeList != []:
                                    return FvObj.BlockSizeList[0][0]
                                else:
                                    return ElementRegion.BlockSizeOfRegion(ElementFd.BlockSizeList)
            if FvObj.BlockSizeList != []:
                return FvObj.BlockSizeList[0][0]
            return DefaultBlockSize
        else:
            for ElementRegion in FdObj.RegionList:
                    if ElementRegion.RegionType == BINARY_FILE_TYPE_FV:
                        for ElementRegionData in ElementRegion.RegionDataList:
                            if ElementRegionData is not None and ElementRegionData.upper() == FvObj.UiFvName:
                                if FvObj.BlockSizeList != []:
                                    return FvObj.BlockSizeList[0][0]
                                else:
                                    return ElementRegion.BlockSizeOfRegion(ElementFd.BlockSizeList)
            return DefaultBlockSize

    ## DisplayFvSpaceInfo()
    #
    #   @param  FvObj           Whose block size to get
    #   @retval None
    #
    @staticmethod
    def DisplayFvSpaceInfo(FdfParserObject):

        FvSpaceInfoList = []
        MaxFvNameLength = 0
        for FvName in FdfParserObject.Profile.FvDict:
            if len(FvName) > MaxFvNameLength:
                MaxFvNameLength = len(FvName)
            FvSpaceInfoFileName = os.path.join(GenFdsGlobalVariable.FvDir, FvName.upper() + '.Fv.map')
            if os.path.exists(FvSpaceInfoFileName):
                FileLinesList = getlines(FvSpaceInfoFileName)
                TotalFound = False
                Total = ''
                UsedFound = False
                Used = ''
                FreeFound = False
                Free = ''
                for Line in FileLinesList:
                    NameValue = Line.split('=')
                    if len(NameValue) == 2:
                        if NameValue[0].strip() == 'EFI_FV_TOTAL_SIZE':
                            TotalFound = True
                            Total = NameValue[1].strip()
                        if NameValue[0].strip() == 'EFI_FV_TAKEN_SIZE':
                            UsedFound = True
                            Used = NameValue[1].strip()
                        if NameValue[0].strip() == 'EFI_FV_SPACE_SIZE':
                            FreeFound = True
                            Free = NameValue[1].strip()

                if TotalFound and UsedFound and FreeFound:
                    FvSpaceInfoList.append((FvName, Total, Used, Free))

        GenFdsGlobalVariable.InfLogger('\nFV Space Information')
        for FvSpaceInfo in FvSpaceInfoList:
            Name = FvSpaceInfo[0]
            TotalSizeValue = int(FvSpaceInfo[1], 0)
            UsedSizeValue = int(FvSpaceInfo[2], 0)
            FreeSizeValue = int(FvSpaceInfo[3], 0)
            if UsedSizeValue == TotalSizeValue:
                Percentage = '100'
            else:
                Percentage = str((UsedSizeValue + 0.0) / TotalSizeValue)[0:4].lstrip('0.')

            GenFdsGlobalVariable.InfLogger(Name + ' ' + '[' + Percentage + '%Full] '\
                                           + str(TotalSizeValue) + ' (' + hex(TotalSizeValue) + ')' + ' total, '\
                                           + str(UsedSizeValue) + ' (' + hex(UsedSizeValue) + ')' + ' used, '\
                                           + str(FreeSizeValue) + ' (' + hex(FreeSizeValue) + ')' + ' free')

    ## PreprocessImage()
    #
    #   @param  BuildDb         Database from build meta data files
    #   @param  DscFile         modules from dsc file will be preprocessed
    #   @retval None
    #
    @staticmethod
    def PreprocessImage(BuildDb, DscFile):
        PcdDict = BuildDb.BuildObject[DscFile, TAB_COMMON, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag].Pcds
        PcdValue = ''
        for Key in PcdDict:
            PcdObj = PcdDict[Key]
            if PcdObj.TokenCName == 'PcdBsBaseAddress':
                PcdValue = PcdObj.DefaultValue
                break

        if PcdValue == '':
            return

        Int64PcdValue = int(PcdValue, 0)
        if Int64PcdValue == 0 or Int64PcdValue < -1:
            return

        TopAddress = 0
        if Int64PcdValue > 0:
            TopAddress = Int64PcdValue

        ModuleDict = BuildDb.BuildObject[DscFile, TAB_COMMON, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag].Modules
        for Key in ModuleDict:
            ModuleObj = BuildDb.BuildObject[Key, TAB_COMMON, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
            print(ModuleObj.BaseName + ' ' + ModuleObj.ModuleType)

    @staticmethod
    def GenerateGuidXRefFile(BuildDb, ArchList, FdfParserObj):
        GuidXRefFileName = os.path.join(GenFdsGlobalVariable.FvDir, "Guid.xref")
        GuidXRefFile = []
        PkgGuidDict = {}
        GuidDict = {}
        ModuleList = []
        FileGuidList = []
        VariableGuidSet = set()
        for Arch in ArchList:
            PlatformDataBase = BuildDb.BuildObject[GenFdsGlobalVariable.ActivePlatform, Arch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
            PkgList = GenFdsGlobalVariable.WorkSpace.GetPackageList(GenFdsGlobalVariable.ActivePlatform, Arch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag)
            for P in PkgList:
                PkgGuidDict.update(P.Guids)
            for Name, Guid in PlatformDataBase.Pcds:
                Pcd = PlatformDataBase.Pcds[Name, Guid]
                if Pcd.Type in [TAB_PCDS_DYNAMIC_HII, TAB_PCDS_DYNAMIC_EX_HII]:
                    for SkuId in Pcd.SkuInfoList:
                        Sku = Pcd.SkuInfoList[SkuId]
                        if Sku.VariableGuid in VariableGuidSet:continue
                        VariableGuidSet.add(Sku.VariableGuid)
                        if Sku.VariableGuid and Sku.VariableGuid in PkgGuidDict.keys():
                            GuidDict[Sku.VariableGuid] = PkgGuidDict[Sku.VariableGuid]
            for ModuleFile in PlatformDataBase.Modules:
                Module = BuildDb.BuildObject[ModuleFile, Arch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
                if Module in ModuleList:
                    continue
                else:
                    ModuleList.append(Module)
                if GlobalData.gGuidPattern.match(ModuleFile.BaseName):
                    GuidXRefFile.append("%s %s\n" % (ModuleFile.BaseName, Module.BaseName))
                else:
                    GuidXRefFile.append("%s %s\n" % (Module.Guid, Module.BaseName))
                GuidDict.update(Module.Protocols)
                GuidDict.update(Module.Guids)
                GuidDict.update(Module.Ppis)
            for FvName in FdfParserObj.Profile.FvDict:
                for FfsObj in FdfParserObj.Profile.FvDict[FvName].FfsList:
                    if not isinstance(FfsObj, FileStatement):
                        InfPath = PathClass(NormPath(mws.join(GenFdsGlobalVariable.WorkSpaceDir, FfsObj.InfFileName)))
                        FdfModule = BuildDb.BuildObject[InfPath, Arch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
                        if FdfModule in ModuleList:
                            continue
                        else:
                            ModuleList.append(FdfModule)
                        GuidXRefFile.append("%s %s\n" % (FdfModule.Guid, FdfModule.BaseName))
                        GuidDict.update(FdfModule.Protocols)
                        GuidDict.update(FdfModule.Guids)
                        GuidDict.update(FdfModule.Ppis)
                    else:
                        FileStatementGuid = FfsObj.NameGuid
                        if FileStatementGuid in FileGuidList:
                            continue
                        else:
                            FileGuidList.append(FileStatementGuid)
                        Name = []
                        FfsPath = os.path.join(GenFdsGlobalVariable.FvDir, 'Ffs')
                        FfsPath = glob(os.path.join(FfsPath, FileStatementGuid) + TAB_STAR)
                        if not FfsPath:
                            continue
                        if not os.path.exists(FfsPath[0]):
                            continue
                        MatchDict = {}
                        ReFileEnds = compile('\S+(.ui)$|\S+(fv.sec.txt)$|\S+(.pe32.txt)$|\S+(.te.txt)$|\S+(.pic.txt)$|\S+(.raw.txt)$|\S+(.ffs.txt)$')
                        FileList = os.listdir(FfsPath[0])
                        for File in FileList:
                            Match = ReFileEnds.search(File)
                            if Match:
                                for Index in range(1, 8):
                                    if Match.group(Index) and Match.group(Index) in MatchDict:
                                        MatchDict[Match.group(Index)].append(File)
                                    elif Match.group(Index):
                                        MatchDict[Match.group(Index)] = [File]
                        if not MatchDict:
                            continue
                        if '.ui' in MatchDict:
                            for File in MatchDict['.ui']:
                                with open(os.path.join(FfsPath[0], File), 'rb') as F:
                                    F.read()
                                    length = F.tell()
                                    F.seek(4)
                                    TmpStr = unpack('%dh' % ((length - 4) // 2), F.read())
                                    Name = ''.join(chr(c) for c in TmpStr[:-1])
                        else:
                            FileList = []
                            if 'fv.sec.txt' in MatchDict:
                                FileList = MatchDict['fv.sec.txt']
                            elif '.pe32.txt' in MatchDict:
                                FileList = MatchDict['.pe32.txt']
                            elif '.te.txt' in MatchDict:
                                FileList = MatchDict['.te.txt']
                            elif '.pic.txt' in MatchDict:
                                FileList = MatchDict['.pic.txt']
                            elif '.raw.txt' in MatchDict:
                                FileList = MatchDict['.raw.txt']
                            elif '.ffs.txt' in MatchDict:
                                FileList = MatchDict['.ffs.txt']
                            else:
                                pass
                            for File in FileList:
                                with open(os.path.join(FfsPath[0], File), 'r') as F:
                                    Name.append((F.read().split()[-1]))
                        if not Name:
                            continue

                        Name = ' '.join(Name) if isinstance(Name, type([])) else Name
                        GuidXRefFile.append("%s %s\n" %(FileStatementGuid, Name))

       # Append GUIDs, Protocols, and PPIs to the Xref file
        GuidXRefFile.append("\n")
        for key, item in GuidDict.items():
            GuidXRefFile.append("%s %s\n" % (GuidStructureStringToGuidString(item).upper(), key))

        if GuidXRefFile:
            GuidXRefFile = ''.join(GuidXRefFile)
            SaveFileOnChange(GuidXRefFileName, GuidXRefFile, False)
            GenFdsGlobalVariable.InfLogger("\nGUID cross reference file can be found at %s" % GuidXRefFileName)
        elif os.path.exists(GuidXRefFileName):
            os.remove(GuidXRefFileName)


if __name__ == '__main__':
    r = main()
    ## 0-127 is a safe return range, and 1 is a standard default error
    if r < 0 or r > 127:
        r = 1
    exit(r)

