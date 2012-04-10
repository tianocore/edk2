## @file
# generate flash image
#
#  Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
from optparse import OptionParser
import sys
import os
import linecache
import FdfParser
import Common.BuildToolError as BuildToolError
from GenFdsGlobalVariable import GenFdsGlobalVariable
from Workspace.WorkspaceDatabase import WorkspaceDatabase
from Workspace.BuildClassObject import PcdClassObject
from Workspace.BuildClassObject import ModuleBuildClassObject
import RuleComplexFile
from EfiSection import EfiSection
import StringIO
import Common.TargetTxtClassObject as TargetTxtClassObject
import Common.ToolDefClassObject as ToolDefClassObject
import Common.DataType
import Common.GlobalData as GlobalData
from Common import EdkLogger
from Common.String import *
from Common.Misc import DirCache,PathClass
from Common.Misc import SaveFileOnChange
from Common.BuildVersion import gBUILD_VERSION

## Version and Copyright
versionNumber = "1.0" + ' ' + gBUILD_VERSION
__version__ = "%prog Version " + versionNumber
__copyright__ = "Copyright (c) 2007 - 2010, Intel Corporation  All rights reserved."

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

    global Workspace
    Workspace = ""
    ArchList = None
    ReturnCode = 0

    EdkLogger.Initialize()
    try:
        if Options.verbose != None:
            EdkLogger.SetLevel(EdkLogger.VERBOSE)
            GenFdsGlobalVariable.VerboseMode = True
            
        if Options.FixedAddress != None:
            GenFdsGlobalVariable.FixedLoadAddress = True
            
        if Options.quiet != None:
            EdkLogger.SetLevel(EdkLogger.QUIET)
        if Options.debug != None:
            EdkLogger.SetLevel(Options.debug + 1)
            GenFdsGlobalVariable.DebugLevel = Options.debug
        else:
            EdkLogger.SetLevel(EdkLogger.INFO)

        if (Options.Workspace == None):
            EdkLogger.error("GenFds", OPTION_MISSING, "WORKSPACE not defined",
                            ExtraData="Please use '-w' switch to pass it or set the WORKSPACE environment variable.")
        elif not os.path.exists(Options.Workspace):
            EdkLogger.error("GenFds", PARAMETER_INVALID, "WORKSPACE is invalid",
                            ExtraData="Please use '-w' switch to pass it or set the WORKSPACE environment variable.")
        else:
            Workspace = os.path.normcase(Options.Workspace)
            GenFdsGlobalVariable.WorkSpaceDir = Workspace
            if 'EDK_SOURCE' in os.environ.keys():
                GenFdsGlobalVariable.EdkSourceDir = os.path.normcase(os.environ['EDK_SOURCE'])
            if (Options.debug):
                GenFdsGlobalVariable.VerboseLogger( "Using Workspace:" + Workspace)
        os.chdir(GenFdsGlobalVariable.WorkSpaceDir)

        if (Options.filename):
            FdfFilename = Options.filename
            FdfFilename = GenFdsGlobalVariable.ReplaceWorkspaceMacro(FdfFilename)

            if FdfFilename[0:2] == '..':
                FdfFilename = os.path.realpath(FdfFilename)
            if not os.path.isabs (FdfFilename):
                FdfFilename = os.path.join(GenFdsGlobalVariable.WorkSpaceDir, FdfFilename)
            if not os.path.exists(FdfFilename):
                EdkLogger.error("GenFds", FILE_NOT_FOUND, ExtraData=FdfFilename)
            if os.path.normcase (FdfFilename).find(Workspace) != 0:
                EdkLogger.error("GenFds", FILE_NOT_FOUND, "FdfFile doesn't exist in Workspace!")

            GenFdsGlobalVariable.FdfFile = FdfFilename
            GenFdsGlobalVariable.FdfFileTimeStamp = os.path.getmtime(FdfFilename)
        else:
            EdkLogger.error("GenFds", OPTION_MISSING, "Missing FDF filename")

        if (Options.BuildTarget):
            GenFdsGlobalVariable.TargetName = Options.BuildTarget
        else:
            EdkLogger.error("GenFds", OPTION_MISSING, "Missing build target")

        if (Options.ToolChain):
            GenFdsGlobalVariable.ToolChainTag = Options.ToolChain
        else:
            EdkLogger.error("GenFds", OPTION_MISSING, "Missing tool chain tag")

        if (Options.activePlatform):
            ActivePlatform = Options.activePlatform
            ActivePlatform = GenFdsGlobalVariable.ReplaceWorkspaceMacro(ActivePlatform)

            if ActivePlatform[0:2] == '..':
                ActivePlatform = os.path.realpath(ActivePlatform)

            if not os.path.isabs (ActivePlatform):
                ActivePlatform = os.path.join(GenFdsGlobalVariable.WorkSpaceDir, ActivePlatform)

            if not os.path.exists(ActivePlatform)  :
                EdkLogger.error("GenFds", FILE_NOT_FOUND, "ActivePlatform doesn't exist!")

            if os.path.normcase (ActivePlatform).find(Workspace) != 0:
                EdkLogger.error("GenFds", FILE_NOT_FOUND, "ActivePlatform doesn't exist in Workspace!")

            ActivePlatform = ActivePlatform[len(Workspace):]
            if len(ActivePlatform) > 0 :
                if ActivePlatform[0] == '\\' or ActivePlatform[0] == '/':
                    ActivePlatform = ActivePlatform[1:]
            else:
                EdkLogger.error("GenFds", FILE_NOT_FOUND, "ActivePlatform doesn't exist!")
        else:
            EdkLogger.error("GenFds", OPTION_MISSING, "Missing active platform")

        GenFdsGlobalVariable.ActivePlatform = PathClass(NormPath(ActivePlatform), Workspace)

        BuildConfigurationFile = os.path.normpath(os.path.join(GenFdsGlobalVariable.WorkSpaceDir, "Conf/target.txt"))
        if os.path.isfile(BuildConfigurationFile) == True:
            TargetTxtClassObject.TargetTxtClassObject(BuildConfigurationFile)
        else:
            EdkLogger.error("GenFds", FILE_NOT_FOUND, ExtraData=BuildConfigurationFile)

        if Options.Macros:
            for Pair in Options.Macros:
                Pair.strip('"')
                List = Pair.split('=')
                if len(List) == 2:
                    if List[0].strip() == "EFI_SOURCE":
                        GlobalData.gEfiSource = List[1].strip()
                        GlobalData.gGlobalDefines["EFI_SOURCE"] = GlobalData.gEfiSource
                        continue
                    elif List[0].strip() == "EDK_SOURCE":
                        GlobalData.gEdkSource = List[1].strip()
                        GlobalData.gGlobalDefines["EDK_SOURCE"] = GlobalData.gEdkSource
                        continue
                    elif List[0].strip() in ["WORKSPACE", "TARGET", "TOOLCHAIN"]:
                        GlobalData.gGlobalDefines[List[0].strip()] = List[1].strip()
                    else:
                        GlobalData.gCommandLineDefines[List[0].strip()] = List[1].strip()
                else:
                    GlobalData.gCommandLineDefines[List[0].strip()] = "TRUE"
        os.environ["WORKSPACE"] = Workspace

        """call Workspace build create database"""
        BuildWorkSpace = WorkspaceDatabase(None)
        BuildWorkSpace.InitDatabase()
        
        #
        # Get files real name in workspace dir
        #
        GlobalData.gAllFiles = DirCache(Workspace)
        GlobalData.gWorkspace = Workspace

        if (Options.archList) :
            ArchList = Options.archList.split(',')
        else:
#            EdkLogger.error("GenFds", OPTION_MISSING, "Missing build ARCH")
            ArchList = BuildWorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, 'COMMON', Options.BuildTarget, Options.ToolChain].SupArchList

        TargetArchList = set(BuildWorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, 'COMMON', Options.BuildTarget, Options.ToolChain].SupArchList) & set(ArchList)
        if len(TargetArchList) == 0:
            EdkLogger.error("GenFds", GENFDS_ERROR, "Target ARCH %s not in platform supported ARCH %s" % (str(ArchList), str(BuildWorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, 'COMMON'].SupArchList)))
        
        for Arch in ArchList:
            GenFdsGlobalVariable.OutputDirFromDscDict[Arch] = NormPath(BuildWorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, Arch, Options.BuildTarget, Options.ToolChain].OutputDirectory)
            GenFdsGlobalVariable.PlatformName = BuildWorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, Arch, Options.BuildTarget, Options.ToolChain].PlatformName

        if (Options.outputDir):
            OutputDirFromCommandLine = GenFdsGlobalVariable.ReplaceWorkspaceMacro(Options.outputDir)
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
        FdfParserObj = FdfParser.FdfParser(FdfFilename)
        FdfParserObj.ParseFile()

        if FdfParserObj.CycleReferenceCheck():
            EdkLogger.error("GenFds", FORMAT_NOT_SUPPORTED, "Cycle Reference Detected in FDF file")

        if (Options.uiFdName) :
            if Options.uiFdName.upper() in FdfParserObj.Profile.FdDict.keys():
                GenFds.OnlyGenerateThisFd = Options.uiFdName
            else:
                EdkLogger.error("GenFds", OPTION_VALUE_INVALID,
                                "No such an FD in FDF file: %s" % Options.uiFdName)

        if (Options.uiFvName) :
            if Options.uiFvName.upper() in FdfParserObj.Profile.FvDict.keys():
                GenFds.OnlyGenerateThisFv = Options.uiFvName
            else:
                EdkLogger.error("GenFds", OPTION_VALUE_INVALID,
                                "No such an FV in FDF file: %s" % Options.uiFvName)

        if (Options.uiCapName) :
            if Options.uiCapName.upper() in FdfParserObj.Profile.CapsuleDict.keys():
                GenFds.OnlyGenerateThisCap = Options.uiCapName
            else:
                EdkLogger.error("GenFds", OPTION_VALUE_INVALID,
                                "No such a Capsule in FDF file: %s" % Options.uiCapName)

        """Modify images from build output if the feature of loading driver at fixed address is on."""
        if GenFdsGlobalVariable.FixedLoadAddress:
            GenFds.PreprocessImage(BuildWorkSpace, GenFdsGlobalVariable.ActivePlatform)
        """Call GenFds"""
        GenFds.GenFd('', FdfParserObj, BuildWorkSpace, ArchList)

        """Generate GUID cross reference file"""
        GenFds.GenerateGuidXRefFile(BuildWorkSpace, ArchList)

        """Display FV space info."""
        GenFds.DisplayFvSpaceInfo(FdfParserObj)

    except FdfParser.Warning, X:
        EdkLogger.error(X.ToolName, FORMAT_INVALID, File=X.FileName, Line=X.LineNumber, ExtraData=X.Message, RaiseError = False)
        ReturnCode = FORMAT_INVALID
    except FatalError, X:
        if Options.debug != None:
            import traceback
            EdkLogger.quiet(traceback.format_exc())
        ReturnCode = X.args[0]
    except:
        import traceback
        EdkLogger.error(
                    "\nPython",
                    CODE_ERROR,
                    "Tools code failure",
                    ExtraData="Please send email to edk2-buildtools-devel@lists.sourceforge.net for help, attaching following call stack trace!\n",
                    RaiseError=False
                    )
        EdkLogger.quiet(traceback.format_exc())
        ReturnCode = CODE_ERROR
    return ReturnCode

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
#   @retval Args  Target of build command
#
def myOptionParser():
    usage = "%prog [options] -f input_file -a arch_list -b build_target -p active_platform -t tool_chain_tag -D \"MacroName [= MacroValue]\""
    Parser = OptionParser(usage=usage,description=__copyright__,version="%prog " + str(versionNumber))
    Parser.add_option("-f", "--file", dest="filename", type="string", help="Name of FDF file to convert", action="callback", callback=SingleCheckCallback)
    Parser.add_option("-a", "--arch", dest="archList", help="comma separated list containing one or more of: IA32, X64, IPF, ARM or EBC which should be built, overrides target.txt?s TARGET_ARCH")
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
    (Options, args) = Parser.parse_args()
    return Options

## The class implementing the EDK2 flash image generation process
#
#   This process includes:
#       1. Collect workspace information, includes platform and module information
#       2. Call methods of Fd class to generate FD
#       3. Call methods of Fv class to generate FV that not belong to FD
#
class GenFds :
    FdfParsef = None
    # FvName, FdName, CapName in FDF, Image file name
    ImageBinDict = {}
    OnlyGenerateThisFd = None
    OnlyGenerateThisFv = None
    OnlyGenerateThisCap = None

    ## GenFd()
    #
    #   @param  OutputDir           Output directory
    #   @param  FdfParser           FDF contents parser
    #   @param  Workspace           The directory of workspace
    #   @param  ArchList            The Arch list of platform
    #
    def GenFd (OutputDir, FdfParser, WorkSpace, ArchList):
        GenFdsGlobalVariable.SetDir ('', FdfParser, WorkSpace, ArchList)

        GenFdsGlobalVariable.VerboseLogger(" Generate all Fd images and their required FV and Capsule images!")
        if GenFds.OnlyGenerateThisCap != None and GenFds.OnlyGenerateThisCap.upper() in GenFdsGlobalVariable.FdfParser.Profile.CapsuleDict.keys():
            CapsuleObj = GenFdsGlobalVariable.FdfParser.Profile.CapsuleDict.get(GenFds.OnlyGenerateThisCap.upper())
            if CapsuleObj != None:
                CapsuleObj.GenCapsule()
                return

        if GenFds.OnlyGenerateThisFd != None and GenFds.OnlyGenerateThisFd.upper() in GenFdsGlobalVariable.FdfParser.Profile.FdDict.keys():
            FdObj = GenFdsGlobalVariable.FdfParser.Profile.FdDict.get(GenFds.OnlyGenerateThisFd.upper())
            if FdObj != None:
                FdObj.GenFd()
                return
        elif GenFds.OnlyGenerateThisFd == None and GenFds.OnlyGenerateThisFv == None:
            for FdName in GenFdsGlobalVariable.FdfParser.Profile.FdDict.keys():
                FdObj = GenFdsGlobalVariable.FdfParser.Profile.FdDict[FdName]
                FdObj.GenFd()

        GenFdsGlobalVariable.VerboseLogger("\n Generate other FV images! ")
        if GenFds.OnlyGenerateThisFv != None and GenFds.OnlyGenerateThisFv.upper() in GenFdsGlobalVariable.FdfParser.Profile.FvDict.keys():
            FvObj = GenFdsGlobalVariable.FdfParser.Profile.FvDict.get(GenFds.OnlyGenerateThisFv.upper())
            if FvObj != None:
                Buffer = StringIO.StringIO()
                FvObj.AddToBuffer(Buffer)
                Buffer.close()
                return
        elif GenFds.OnlyGenerateThisFv == None:
            for FvName in GenFdsGlobalVariable.FdfParser.Profile.FvDict.keys():
                Buffer = StringIO.StringIO('')
                FvObj = GenFdsGlobalVariable.FdfParser.Profile.FvDict[FvName]
                FvObj.AddToBuffer(Buffer)
                Buffer.close()
        
        if GenFds.OnlyGenerateThisFv == None and GenFds.OnlyGenerateThisFd == None and GenFds.OnlyGenerateThisCap == None:
            if GenFdsGlobalVariable.FdfParser.Profile.CapsuleDict != {}:
                GenFdsGlobalVariable.VerboseLogger("\n Generate other Capsule images!")
                for CapsuleName in GenFdsGlobalVariable.FdfParser.Profile.CapsuleDict.keys():
                    CapsuleObj = GenFdsGlobalVariable.FdfParser.Profile.CapsuleDict[CapsuleName]
                    CapsuleObj.GenCapsule()

            if GenFdsGlobalVariable.FdfParser.Profile.OptRomDict != {}:
                GenFdsGlobalVariable.VerboseLogger("\n Generate all Option ROM!")
                for DriverName in GenFdsGlobalVariable.FdfParser.Profile.OptRomDict.keys():
                    OptRomObj = GenFdsGlobalVariable.FdfParser.Profile.OptRomDict[DriverName]
                    OptRomObj.AddToBuffer(None)

    ## GetFvBlockSize()
    #
    #   @param  FvObj           Whose block size to get
    #   @retval int             Block size value
    #
    def GetFvBlockSize(FvObj):
        DefaultBlockSize = 0x1
        FdObj = None
        if GenFds.OnlyGenerateThisFd != None and GenFds.OnlyGenerateThisFd.upper() in GenFdsGlobalVariable.FdfParser.Profile.FdDict.keys():
            FdObj = GenFdsGlobalVariable.FdfParser.Profile.FdDict[GenFds.OnlyGenerateThisFd.upper()]
        if FdObj == None:
            for ElementFd in GenFdsGlobalVariable.FdfParser.Profile.FdDict.values():
                for ElementRegion in ElementFd.RegionList:
                    if ElementRegion.RegionType == 'FV':
                        for ElementRegionData in ElementRegion.RegionDataList:
                            if ElementRegionData != None and ElementRegionData.upper() == FvObj.UiFvName:
                                if FvObj.BlockSizeList != []:
                                    return FvObj.BlockSizeList[0][0]
                                else:
                                    return ElementRegion.BlockSizeOfRegion(ElementFd.BlockSizeList)
            if FvObj.BlockSizeList != []:
                return FvObj.BlockSizeList[0][0]
            return DefaultBlockSize
        else:
            for ElementRegion in FdObj.RegionList:
                    if ElementRegion.RegionType == 'FV':
                        for ElementRegionData in ElementRegion.RegionDataList:
                            if ElementRegionData != None and ElementRegionData.upper() == FvObj.UiFvName:
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
    def DisplayFvSpaceInfo(FdfParser):
        
        FvSpaceInfoList = []
        MaxFvNameLength = 0
        for FvName in FdfParser.Profile.FvDict:
            if len(FvName) > MaxFvNameLength:
                MaxFvNameLength = len(FvName)
            FvSpaceInfoFileName = os.path.join(GenFdsGlobalVariable.FvDir, FvName.upper() + '.Fv.map')
            if os.path.exists(FvSpaceInfoFileName):
                FileLinesList = linecache.getlines(FvSpaceInfoFileName)
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
            TotalSizeValue = long(FvSpaceInfo[1], 0)
            UsedSizeValue = long(FvSpaceInfo[2], 0)
            FreeSizeValue = long(FvSpaceInfo[3], 0)
            if UsedSizeValue == TotalSizeValue:
                Percentage = '100'
            else:
                Percentage = str((UsedSizeValue+0.0)/TotalSizeValue)[0:4].lstrip('0.') 
            
            GenFdsGlobalVariable.InfLogger(Name + ' ' + '[' + Percentage + '%Full] ' + str(TotalSizeValue) + ' total, ' + str(UsedSizeValue) + ' used, ' + str(FreeSizeValue) + ' free')

    ## PreprocessImage()
    #
    #   @param  BuildDb         Database from build meta data files
    #   @param  DscFile         modules from dsc file will be preprocessed
    #   @retval None
    #
    def PreprocessImage(BuildDb, DscFile):
        PcdDict = BuildDb.BuildObject[DscFile, 'COMMON', GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag].Pcds
        PcdValue = ''
        for Key in PcdDict:
            PcdObj = PcdDict[Key]
            if PcdObj.TokenCName == 'PcdBsBaseAddress':
                PcdValue = PcdObj.DefaultValue
                break
        
        if PcdValue == '':
            return
        
        Int64PcdValue = long(PcdValue, 0)
        if Int64PcdValue == 0 or Int64PcdValue < -1:    
            return
                
        TopAddress = 0
        if Int64PcdValue > 0:
            TopAddress = Int64PcdValue
            
        ModuleDict = BuildDb.BuildObject[DscFile, 'COMMON', GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag].Modules
        for Key in ModuleDict:
            ModuleObj = BuildDb.BuildObject[Key, 'COMMON', GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
            print ModuleObj.BaseName + ' ' + ModuleObj.ModuleType

    def GenerateGuidXRefFile(BuildDb, ArchList):
        GuidXRefFileName = os.path.join(GenFdsGlobalVariable.FvDir, "Guid.xref")
        GuidXRefFile = StringIO.StringIO('')
        for Arch in ArchList:
            PlatformDataBase = BuildDb.BuildObject[GenFdsGlobalVariable.ActivePlatform, Arch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
            for ModuleFile in PlatformDataBase.Modules:
                Module = BuildDb.BuildObject[ModuleFile, Arch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
                GuidXRefFile.write("%s %s\n" % (Module.Guid, Module.BaseName))
        if GuidXRefFile.getvalue():
            SaveFileOnChange(GuidXRefFileName, GuidXRefFile.getvalue(), False)
            GenFdsGlobalVariable.InfLogger("\nGUID cross reference file can be found at %s" % GuidXRefFileName)
        elif os.path.exists(GuidXRefFileName):
            os.remove(GuidXRefFileName)
        GuidXRefFile.close()

    ##Define GenFd as static function
    GenFd = staticmethod(GenFd)
    GetFvBlockSize = staticmethod(GetFvBlockSize)
    DisplayFvSpaceInfo = staticmethod(DisplayFvSpaceInfo)
    PreprocessImage = staticmethod(PreprocessImage)
    GenerateGuidXRefFile = staticmethod(GenerateGuidXRefFile)

if __name__ == '__main__':
    r = main()
    ## 0-127 is a safe return range, and 1 is a standard default error
    if r < 0 or r > 127: r = 1
    sys.exit(r)

