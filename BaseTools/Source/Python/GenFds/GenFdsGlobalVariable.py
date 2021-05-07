## @file
# Global variables for GenFds
#
#  Copyright (c) 2007 - 2021, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import print_function
from __future__ import absolute_import

import Common.LongFilePathOs as os
import sys
from sys import stdout
from subprocess import PIPE,Popen
from struct import Struct
from array import array

from Common.BuildToolError import COMMAND_FAILURE,GENFDS_ERROR
from Common import EdkLogger
from Common.Misc import SaveFileOnChange

from Common.TargetTxtClassObject import TargetTxtDict
from Common.ToolDefClassObject import ToolDefDict
from AutoGen.BuildEngine import ToolBuildRule
import Common.DataType as DataType
from Common.Misc import PathClass,CreateDirectory
from Common.LongFilePathSupport import OpenLongFilePath as open
from Common.MultipleWorkspace import MultipleWorkspace as mws
import Common.GlobalData as GlobalData
from Common.BuildToolError import *
from AutoGen.AutoGen import CalculatePriorityValue

## Global variables
#
#
class GenFdsGlobalVariable:
    FvDir = ''
    OutputDirDict = {}
    BinDir = ''
    # will be FvDir + os.sep + 'Ffs'
    FfsDir = ''
    FdfParser = None
    LibDir = ''
    WorkSpace = None
    WorkSpaceDir = ''
    ConfDir = ''
    OutputDirFromDscDict = {}
    TargetName = ''
    ToolChainTag = ''
    RuleDict = {}
    ArchList = None
    ActivePlatform = None
    FvAddressFileName = ''
    VerboseMode = False
    DebugLevel = -1
    SharpCounter = 0
    SharpNumberPerLine = 40
    FdfFile = ''
    FdfFileTimeStamp = 0
    FixedLoadAddress = False
    PlatformName = ''

    BuildRuleFamily = DataType.TAB_COMPILER_MSFT
    ToolChainFamily = DataType.TAB_COMPILER_MSFT
    __BuildRuleDatabase = None
    GuidToolDefinition = {}
    FfsCmdDict = {}
    SecCmdList = []
    CopyList   = []
    ModuleFile = ''
    EnableGenfdsMultiThread = True

    #
    # The list whose element are flags to indicate if large FFS or SECTION files exist in FV.
    # At the beginning of each generation of FV, false flag is appended to the list,
    # after the call to GenerateSection returns, check the size of the output file,
    # if it is greater than 0xFFFFFF, the tail flag in list is set to true,
    # and EFI_FIRMWARE_FILE_SYSTEM3_GUID is passed to C GenFv.
    # At the end of generation of FV, pop the flag.
    # List is used as a stack to handle nested FV generation.
    #
    LargeFileInFvFlags = []
    EFI_FIRMWARE_FILE_SYSTEM3_GUID = '5473C07A-3DCB-4dca-BD6F-1E9689E7349A'
    LARGE_FILE_SIZE = 0x1000000

    SectionHeader = Struct("3B 1B")

    # FvName, FdName, CapName in FDF, Image file name
    ImageBinDict = {}

    ## LoadBuildRule
    #
    @staticmethod
    def _LoadBuildRule():
        if GenFdsGlobalVariable.__BuildRuleDatabase:
            return GenFdsGlobalVariable.__BuildRuleDatabase
        BuildRule = ToolBuildRule()
        GenFdsGlobalVariable.__BuildRuleDatabase = BuildRule.ToolBuildRule
        TargetObj = TargetTxtDict()
        ToolDefinitionFile = TargetObj.Target.TargetTxtDictionary[DataType.TAB_TAT_DEFINES_TOOL_CHAIN_CONF]
        if ToolDefinitionFile == '':
            ToolDefinitionFile = "Conf/tools_def.txt"
        if os.path.isfile(ToolDefinitionFile):
            ToolDefObj = ToolDefDict((os.path.join(os.getenv("WORKSPACE"), "Conf")))
            ToolDefinition = ToolDefObj.ToolDef.ToolsDefTxtDatabase
            if DataType.TAB_TOD_DEFINES_BUILDRULEFAMILY in ToolDefinition \
               and GenFdsGlobalVariable.ToolChainTag in ToolDefinition[DataType.TAB_TOD_DEFINES_BUILDRULEFAMILY] \
               and ToolDefinition[DataType.TAB_TOD_DEFINES_BUILDRULEFAMILY][GenFdsGlobalVariable.ToolChainTag]:
                GenFdsGlobalVariable.BuildRuleFamily = ToolDefinition[DataType.TAB_TOD_DEFINES_BUILDRULEFAMILY][GenFdsGlobalVariable.ToolChainTag]

            if DataType.TAB_TOD_DEFINES_FAMILY in ToolDefinition \
               and GenFdsGlobalVariable.ToolChainTag in ToolDefinition[DataType.TAB_TOD_DEFINES_FAMILY] \
               and ToolDefinition[DataType.TAB_TOD_DEFINES_FAMILY][GenFdsGlobalVariable.ToolChainTag]:
                GenFdsGlobalVariable.ToolChainFamily = ToolDefinition[DataType.TAB_TOD_DEFINES_FAMILY][GenFdsGlobalVariable.ToolChainTag]
        return GenFdsGlobalVariable.__BuildRuleDatabase

    ## GetBuildRules
    #    @param Inf: object of InfBuildData
    #    @param Arch: current arch
    #
    @staticmethod
    def GetBuildRules(Inf, Arch):
        if not Arch:
            Arch = DataType.TAB_COMMON

        if not Arch in GenFdsGlobalVariable.OutputDirDict:
            return {}

        BuildRuleDatabase = GenFdsGlobalVariable._LoadBuildRule()
        if not BuildRuleDatabase:
            return {}

        PathClassObj = PathClass(Inf.MetaFile.File,
                                 GenFdsGlobalVariable.WorkSpaceDir)
        BuildDir = os.path.join(
            GenFdsGlobalVariable.OutputDirDict[Arch],
            Arch,
            PathClassObj.SubDir,
            PathClassObj.BaseName
        )
        BinDir = os.path.join(GenFdsGlobalVariable.OutputDirDict[Arch], Arch)
        Macro = {
        "WORKSPACE":GenFdsGlobalVariable.WorkSpaceDir,
        "MODULE_NAME":Inf.BaseName,
        "MODULE_GUID":Inf.Guid,
        "MODULE_VERSION":Inf.Version,
        "MODULE_TYPE":Inf.ModuleType,
        "MODULE_FILE":str(PathClassObj),
        "MODULE_FILE_BASE_NAME":PathClassObj.BaseName,
        "MODULE_RELATIVE_DIR":PathClassObj.SubDir,
        "MODULE_DIR":PathClassObj.SubDir,
        "BASE_NAME":Inf.BaseName,
        "ARCH":Arch,
        "TOOLCHAIN":GenFdsGlobalVariable.ToolChainTag,
        "TOOLCHAIN_TAG":GenFdsGlobalVariable.ToolChainTag,
        "TOOL_CHAIN_TAG":GenFdsGlobalVariable.ToolChainTag,
        "TARGET":GenFdsGlobalVariable.TargetName,
        "BUILD_DIR":GenFdsGlobalVariable.OutputDirDict[Arch],
        "BIN_DIR":BinDir,
        "LIB_DIR":BinDir,
        "MODULE_BUILD_DIR":BuildDir,
        "OUTPUT_DIR":os.path.join(BuildDir, "OUTPUT"),
        "DEBUG_DIR":os.path.join(BuildDir, "DEBUG")
        }

        BuildRules = {}
        for Type in BuildRuleDatabase.FileTypeList:
            #first try getting build rule by BuildRuleFamily
            RuleObject = BuildRuleDatabase[Type, Inf.BuildType, Arch, GenFdsGlobalVariable.BuildRuleFamily]
            if not RuleObject:
                # build type is always module type, but ...
                if Inf.ModuleType != Inf.BuildType:
                    RuleObject = BuildRuleDatabase[Type, Inf.ModuleType, Arch, GenFdsGlobalVariable.BuildRuleFamily]
            #second try getting build rule by ToolChainFamily
            if not RuleObject:
                RuleObject = BuildRuleDatabase[Type, Inf.BuildType, Arch, GenFdsGlobalVariable.ToolChainFamily]
                if not RuleObject:
                    # build type is always module type, but ...
                    if Inf.ModuleType != Inf.BuildType:
                        RuleObject = BuildRuleDatabase[Type, Inf.ModuleType, Arch, GenFdsGlobalVariable.ToolChainFamily]
            if not RuleObject:
                continue
            RuleObject = RuleObject.Instantiate(Macro)
            BuildRules[Type] = RuleObject
            for Ext in RuleObject.SourceFileExtList:
                BuildRules[Ext] = RuleObject
        return BuildRules

    ## GetModuleCodaTargetList
    #
    #    @param Inf: object of InfBuildData
    #    @param Arch: current arch
    #
    @staticmethod
    def GetModuleCodaTargetList(Inf, Arch):
        BuildRules = GenFdsGlobalVariable.GetBuildRules(Inf, Arch)
        if not BuildRules:
            return []

        TargetList = set()
        FileList = []

        if not Inf.IsBinaryModule:
            for File in Inf.Sources:
                if File.TagName in {"", DataType.TAB_STAR, GenFdsGlobalVariable.ToolChainTag} and \
                    File.ToolChainFamily in {"", DataType.TAB_STAR, GenFdsGlobalVariable.ToolChainFamily}:
                    FileList.append((File, DataType.TAB_UNKNOWN_FILE))

        for File in Inf.Binaries:
            if File.Target in {DataType.TAB_COMMON, DataType.TAB_STAR, GenFdsGlobalVariable.TargetName}:
                FileList.append((File, File.Type))

        for File, FileType in FileList:
            LastTarget = None
            RuleChain = []
            SourceList = [File]
            Index = 0
            while Index < len(SourceList):
                Source = SourceList[Index]
                Index = Index + 1

                if File.IsBinary and File == Source and Inf.Binaries and File in Inf.Binaries:
                    # Skip all files that are not binary libraries
                    if not Inf.LibraryClass:
                        continue
                    RuleObject = BuildRules[DataType.TAB_DEFAULT_BINARY_FILE]
                elif FileType in BuildRules:
                    RuleObject = BuildRules[FileType]
                elif Source.Ext in BuildRules:
                    RuleObject = BuildRules[Source.Ext]
                else:
                    # stop at no more rules
                    if LastTarget:
                        TargetList.add(str(LastTarget))
                    break

                FileType = RuleObject.SourceFileType

                # stop at STATIC_LIBRARY for library
                if Inf.LibraryClass and FileType == DataType.TAB_STATIC_LIBRARY:
                    if LastTarget:
                        TargetList.add(str(LastTarget))
                    break

                Target = RuleObject.Apply(Source)
                if not Target:
                    if LastTarget:
                        TargetList.add(str(LastTarget))
                    break
                elif not Target.Outputs:
                    # Only do build for target with outputs
                    TargetList.add(str(Target))

                # to avoid cyclic rule
                if FileType in RuleChain:
                    break

                RuleChain.append(FileType)
                SourceList.extend(Target.Outputs)
                LastTarget = Target
                FileType = DataType.TAB_UNKNOWN_FILE
                for Cmd in Target.Commands:
                    if "$(CP)" == Cmd.split()[0]:
                        CpTarget = Cmd.split()[2]
                        TargetList.add(CpTarget)

        return list(TargetList)

    ## SetDir()
    #
    #   @param  OutputDir           Output directory
    #   @param  FdfParser           FDF contents parser
    #   @param  Workspace           The directory of workspace
    #   @param  ArchList            The Arch list of platform
    #
    @staticmethod
    def SetDir (OutputDir, FdfParser, WorkSpace, ArchList):
        GenFdsGlobalVariable.VerboseLogger("GenFdsGlobalVariable.OutputDir:%s" % OutputDir)
        GenFdsGlobalVariable.FdfParser = FdfParser
        GenFdsGlobalVariable.WorkSpace = WorkSpace
        GenFdsGlobalVariable.FvDir = os.path.join(GenFdsGlobalVariable.OutputDirDict[ArchList[0]], DataType.TAB_FV_DIRECTORY)
        if not os.path.exists(GenFdsGlobalVariable.FvDir):
            os.makedirs(GenFdsGlobalVariable.FvDir)
        GenFdsGlobalVariable.FfsDir = os.path.join(GenFdsGlobalVariable.FvDir, 'Ffs')
        if not os.path.exists(GenFdsGlobalVariable.FfsDir):
            os.makedirs(GenFdsGlobalVariable.FfsDir)

        #
        # Create FV Address inf file
        #
        GenFdsGlobalVariable.FvAddressFileName = os.path.join(GenFdsGlobalVariable.FfsDir, 'FvAddress.inf')
        FvAddressFile = open(GenFdsGlobalVariable.FvAddressFileName, 'w')
        #
        # Add [Options]
        #
        FvAddressFile.writelines("[options]" + DataType.TAB_LINE_BREAK)
        BsAddress = '0'
        for Arch in ArchList:
            if GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, Arch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag].BsBaseAddress:
                BsAddress = GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, Arch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag].BsBaseAddress
                break

        FvAddressFile.writelines("EFI_BOOT_DRIVER_BASE_ADDRESS = " + \
                                       BsAddress + \
                                       DataType.TAB_LINE_BREAK)

        RtAddress = '0'
        for Arch in reversed(ArchList):
            temp = GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, Arch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag].RtBaseAddress
            if temp:
                RtAddress = temp
                break

        FvAddressFile.writelines("EFI_RUNTIME_DRIVER_BASE_ADDRESS = " + \
                                       RtAddress + \
                                       DataType.TAB_LINE_BREAK)

        FvAddressFile.close()

    @staticmethod
    def SetEnv(FdfParser, WorkSpace, ArchList, GlobalData):
        GenFdsGlobalVariable.ModuleFile = WorkSpace.ModuleFile
        GenFdsGlobalVariable.FdfParser = FdfParser
        GenFdsGlobalVariable.WorkSpace = WorkSpace.Db
        GenFdsGlobalVariable.ArchList = ArchList
        GenFdsGlobalVariable.ToolChainTag = GlobalData.gGlobalDefines["TOOL_CHAIN_TAG"]
        GenFdsGlobalVariable.TargetName = GlobalData.gGlobalDefines["TARGET"]
        GenFdsGlobalVariable.ActivePlatform = GlobalData.gActivePlatform
        GenFdsGlobalVariable.ConfDir  = GlobalData.gConfDirectory
        GenFdsGlobalVariable.EnableGenfdsMultiThread = GlobalData.gEnableGenfdsMultiThread
        for Arch in ArchList:
            GenFdsGlobalVariable.OutputDirDict[Arch] = os.path.normpath(
                os.path.join(GlobalData.gWorkspace,
                             WorkSpace.Db.BuildObject[GenFdsGlobalVariable.ActivePlatform, Arch, GlobalData.gGlobalDefines['TARGET'],
                             GlobalData.gGlobalDefines['TOOLCHAIN']].OutputDirectory,
                             GlobalData.gGlobalDefines['TARGET'] +'_' + GlobalData.gGlobalDefines['TOOLCHAIN']))
            GenFdsGlobalVariable.OutputDirFromDscDict[Arch] = os.path.normpath(
                             WorkSpace.Db.BuildObject[GenFdsGlobalVariable.ActivePlatform, Arch,
                             GlobalData.gGlobalDefines['TARGET'], GlobalData.gGlobalDefines['TOOLCHAIN']].OutputDirectory)
            GenFdsGlobalVariable.PlatformName = WorkSpace.Db.BuildObject[GenFdsGlobalVariable.ActivePlatform, Arch,
                                                                      GlobalData.gGlobalDefines['TARGET'],
                                                                      GlobalData.gGlobalDefines['TOOLCHAIN']].PlatformName
        GenFdsGlobalVariable.FvDir = os.path.join(GenFdsGlobalVariable.OutputDirDict[ArchList[0]], DataType.TAB_FV_DIRECTORY)
        if not os.path.exists(GenFdsGlobalVariable.FvDir):
            os.makedirs(GenFdsGlobalVariable.FvDir)
        GenFdsGlobalVariable.FfsDir = os.path.join(GenFdsGlobalVariable.FvDir, 'Ffs')
        if not os.path.exists(GenFdsGlobalVariable.FfsDir):
            os.makedirs(GenFdsGlobalVariable.FfsDir)

        #
        # Create FV Address inf file
        #
        GenFdsGlobalVariable.FvAddressFileName = os.path.join(GenFdsGlobalVariable.FfsDir, 'FvAddress.inf')
        FvAddressFile = open(GenFdsGlobalVariable.FvAddressFileName, 'w')
        #
        # Add [Options]
        #
        FvAddressFile.writelines("[options]" + DataType.TAB_LINE_BREAK)
        BsAddress = '0'
        for Arch in ArchList:
            BsAddress = GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, Arch,
                                                                   GlobalData.gGlobalDefines['TARGET'],
                                                                   GlobalData.gGlobalDefines["TOOL_CHAIN_TAG"]].BsBaseAddress
            if BsAddress:
                break

        FvAddressFile.writelines("EFI_BOOT_DRIVER_BASE_ADDRESS = " + \
                                 BsAddress + \
                                 DataType.TAB_LINE_BREAK)

        RtAddress = '0'
        for Arch in reversed(ArchList):
            temp = GenFdsGlobalVariable.WorkSpace.BuildObject[
                GenFdsGlobalVariable.ActivePlatform, Arch, GlobalData.gGlobalDefines['TARGET'],
                GlobalData.gGlobalDefines["TOOL_CHAIN_TAG"]].RtBaseAddress
            if temp:
                RtAddress = temp
                break

        FvAddressFile.writelines("EFI_RUNTIME_DRIVER_BASE_ADDRESS = " + \
                                 RtAddress + \
                                 DataType.TAB_LINE_BREAK)

        FvAddressFile.close()

    ## ReplaceWorkspaceMacro()
    #
    #   @param  String           String that may contain macro
    #
    @staticmethod
    def ReplaceWorkspaceMacro(String):
        String = mws.handleWsMacro(String)
        Str = String.replace('$(WORKSPACE)', GenFdsGlobalVariable.WorkSpaceDir)
        if os.path.exists(Str):
            if not os.path.isabs(Str):
                Str = os.path.abspath(Str)
        else:
            Str = mws.join(GenFdsGlobalVariable.WorkSpaceDir, String)
        return os.path.normpath(Str)

    ## Check if the input files are newer than output files
    #
    #   @param  Output          Path of output file
    #   @param  Input           Path list of input files
    #
    #   @retval True            if Output doesn't exist, or any Input is newer
    #   @retval False           if all Input is older than Output
    #
    @staticmethod
    def NeedsUpdate(Output, Input):
        if not os.path.exists(Output):
            return True
        # always update "Output" if no "Input" given
        if not Input:
            return True

        # if fdf file is changed after the 'Output" is generated, update the 'Output'
        OutputTime = os.path.getmtime(Output)
        if GenFdsGlobalVariable.FdfFileTimeStamp > OutputTime:
            return True

        for F in Input:
            # always update "Output" if any "Input" doesn't exist
            if not os.path.exists(F):
                return True
            # always update "Output" if any "Input" is newer than "Output"
            if os.path.getmtime(F) > OutputTime:
                return True
        return False

    @staticmethod
    def GenerateSection(Output, Input, Type=None, CompressionType=None, Guid=None,
                        GuidHdrLen=None, GuidAttr=[], Ui=None, Ver=None, InputAlign=[], BuildNumber=None, DummyFile=None, IsMakefile=False):
        Cmd = ["GenSec"]
        if Type:
            Cmd += ("-s", Type)
        if CompressionType:
            Cmd += ("-c", CompressionType)
        if Guid:
            Cmd += ("-g", Guid)
        if DummyFile:
            Cmd += ("--dummy", DummyFile)
        if GuidHdrLen:
            Cmd += ("-l", GuidHdrLen)
        #Add each guided attribute
        for Attr in GuidAttr:
            Cmd += ("-r", Attr)
        #Section Align is only for dummy section without section type
        for SecAlign in InputAlign:
            Cmd += ("--sectionalign", SecAlign)

        CommandFile = Output + '.txt'
        if Ui:
            if IsMakefile:
                if Ui == "$(MODULE_NAME)":
                    Cmd += ('-n', Ui)
                else:
                    Cmd += ("-n", '"' + Ui + '"')
                Cmd += ("-o", Output)
                if ' '.join(Cmd).strip() not in GenFdsGlobalVariable.SecCmdList:
                    GenFdsGlobalVariable.SecCmdList.append(' '.join(Cmd).strip())
            else:
                SectionData = array('B', [0, 0, 0, 0])
                SectionData.fromlist(array('B',Ui.encode('utf-16-le')).tolist())
                SectionData.append(0)
                SectionData.append(0)
                Len = len(SectionData)
                GenFdsGlobalVariable.SectionHeader.pack_into(SectionData, 0, Len & 0xff, (Len >> 8) & 0xff, (Len >> 16) & 0xff, 0x15)


                DirName = os.path.dirname(Output)
                if not CreateDirectory(DirName):
                    EdkLogger.error(None, FILE_CREATE_FAILURE, "Could not create directory %s" % DirName)
                else:
                    if DirName == '':
                        DirName = os.getcwd()
                    if not os.access(DirName, os.W_OK):
                        EdkLogger.error(None, PERMISSION_FAILURE, "Do not have write permission on directory %s" % DirName)

                try:
                    with open(Output, "wb") as Fd:
                        SectionData.tofile(Fd)
                        Fd.flush()
                except IOError as X:
                    EdkLogger.error(None, FILE_CREATE_FAILURE, ExtraData='IOError %s' % X)

        elif Ver:
            Cmd += ("-n", Ver)
            if BuildNumber:
                Cmd += ("-j", BuildNumber)
            Cmd += ("-o", Output)

            SaveFileOnChange(CommandFile, ' '.join(Cmd), False)
            if IsMakefile:
                if ' '.join(Cmd).strip() not in GenFdsGlobalVariable.SecCmdList:
                    GenFdsGlobalVariable.SecCmdList.append(' '.join(Cmd).strip())
            else:
                if not GenFdsGlobalVariable.NeedsUpdate(Output, list(Input) + [CommandFile]):
                    return
                GenFdsGlobalVariable.CallExternalTool(Cmd, "Failed to generate section")
        else:
            Cmd += ("-o", Output)
            Cmd += Input

            SaveFileOnChange(CommandFile, ' '.join(Cmd), False)
            if IsMakefile:
                if sys.platform == "win32":
                    Cmd = ['if', 'exist', Input[0]] + Cmd
                else:
                    Cmd = ['-test', '-e', Input[0], "&&"] + Cmd
                if ' '.join(Cmd).strip() not in GenFdsGlobalVariable.SecCmdList:
                    GenFdsGlobalVariable.SecCmdList.append(' '.join(Cmd).strip())
            elif GenFdsGlobalVariable.NeedsUpdate(Output, list(Input) + [CommandFile]):
                GenFdsGlobalVariable.DebugLogger(EdkLogger.DEBUG_5, "%s needs update because of newer %s" % (Output, Input))
                GenFdsGlobalVariable.CallExternalTool(Cmd, "Failed to generate section")
                if (os.path.getsize(Output) >= GenFdsGlobalVariable.LARGE_FILE_SIZE and
                    GenFdsGlobalVariable.LargeFileInFvFlags):
                    GenFdsGlobalVariable.LargeFileInFvFlags[-1] = True

    @staticmethod
    def GetAlignment (AlignString):
        if not AlignString:
            return 0
        if AlignString.endswith('K'):
            return int (AlignString.rstrip('K')) * 1024
        if AlignString.endswith('M'):
            return int (AlignString.rstrip('M')) * 1024 * 1024
        if AlignString.endswith('G'):
            return int (AlignString.rstrip('G')) * 1024 * 1024 * 1024
        return int (AlignString)

    @staticmethod
    def GenerateFfs(Output, Input, Type, Guid, Fixed=False, CheckSum=False, Align=None,
                    SectionAlign=None, MakefilePath=None):
        Cmd = ["GenFfs", "-t", Type, "-g", Guid]
        mFfsValidAlign = ["0", "8", "16", "128", "512", "1K", "4K", "32K", "64K", "128K", "256K", "512K", "1M", "2M", "4M", "8M", "16M"]
        if Fixed == True:
            Cmd.append("-x")
        if CheckSum:
            Cmd.append("-s")
        if Align:
            if Align not in mFfsValidAlign:
                Align = GenFdsGlobalVariable.GetAlignment (Align)
                for index in range(0, len(mFfsValidAlign) - 1):
                    if ((Align > GenFdsGlobalVariable.GetAlignment(mFfsValidAlign[index])) and (Align <= GenFdsGlobalVariable.GetAlignment(mFfsValidAlign[index + 1]))):
                        break
                Align = mFfsValidAlign[index + 1]
            Cmd += ("-a", Align)

        Cmd += ("-o", Output)
        for I in range(0, len(Input)):
            if MakefilePath:
                Cmd += ("-oi", Input[I])
            else:
                Cmd += ("-i", Input[I])
            if SectionAlign and SectionAlign[I]:
                Cmd += ("-n", SectionAlign[I])

        CommandFile = Output + '.txt'
        SaveFileOnChange(CommandFile, ' '.join(Cmd), False)

        GenFdsGlobalVariable.DebugLogger(EdkLogger.DEBUG_5, "%s needs update because of newer %s" % (Output, Input))
        if MakefilePath:
            if (tuple(Cmd), tuple(GenFdsGlobalVariable.SecCmdList), tuple(GenFdsGlobalVariable.CopyList)) not in GenFdsGlobalVariable.FfsCmdDict:
                GenFdsGlobalVariable.FfsCmdDict[tuple(Cmd), tuple(GenFdsGlobalVariable.SecCmdList), tuple(GenFdsGlobalVariable.CopyList)] = MakefilePath
            GenFdsGlobalVariable.SecCmdList = []
            GenFdsGlobalVariable.CopyList = []
        else:
            if not GenFdsGlobalVariable.NeedsUpdate(Output, list(Input) + [CommandFile]):
                return
            GenFdsGlobalVariable.CallExternalTool(Cmd, "Failed to generate FFS")

    @staticmethod
    def GenerateFirmwareVolume(Output, Input, BaseAddress=None, ForceRebase=None, Capsule=False, Dump=False,
                               AddressFile=None, MapFile=None, FfsList=[], FileSystemGuid=None):
        if not GenFdsGlobalVariable.NeedsUpdate(Output, Input+FfsList):
            return
        GenFdsGlobalVariable.DebugLogger(EdkLogger.DEBUG_5, "%s needs update because of newer %s" % (Output, Input))

        Cmd = ["GenFv"]
        if BaseAddress:
            Cmd += ("-r", BaseAddress)

        if ForceRebase == False:
            Cmd += ("-F", "FALSE")
        elif ForceRebase == True:
            Cmd += ("-F", "TRUE")

        if Capsule:
            Cmd.append("-c")
        if Dump:
            Cmd.append("-p")
        if AddressFile:
            Cmd += ("-a", AddressFile)
        if MapFile:
            Cmd += ("-m", MapFile)
        if FileSystemGuid:
            Cmd += ("-g", FileSystemGuid)
        Cmd += ("-o", Output)
        for I in Input:
            Cmd += ("-i", I)

        GenFdsGlobalVariable.CallExternalTool(Cmd, "Failed to generate FV")

    @staticmethod
    def GenerateFirmwareImage(Output, Input, Type="efi", SubType=None, Zero=False,
                              Strip=False, Replace=False, TimeStamp=None, Join=False,
                              Align=None, Padding=None, Convert=False, IsMakefile=False):
        if not GenFdsGlobalVariable.NeedsUpdate(Output, Input) and not IsMakefile:
            return
        GenFdsGlobalVariable.DebugLogger(EdkLogger.DEBUG_5, "%s needs update because of newer %s" % (Output, Input))

        Cmd = ["GenFw"]
        if Type.lower() == "te":
            Cmd.append("-t")
        if SubType:
            Cmd += ("-e", SubType)
        if TimeStamp:
            Cmd += ("-s", TimeStamp)
        if Align:
            Cmd += ("-a", Align)
        if Padding:
            Cmd += ("-p", Padding)
        if Zero:
            Cmd.append("-z")
        if Strip:
            Cmd.append("-l")
        if Replace:
            Cmd.append("-r")
        if Join:
            Cmd.append("-j")
        if Convert:
            Cmd.append("-m")
        Cmd += ("-o", Output)
        Cmd += Input
        if IsMakefile:
            if " ".join(Cmd).strip() not in GenFdsGlobalVariable.SecCmdList:
                GenFdsGlobalVariable.SecCmdList.append(" ".join(Cmd).strip())
        else:
            GenFdsGlobalVariable.CallExternalTool(Cmd, "Failed to generate firmware image")

    @staticmethod
    def GenerateOptionRom(Output, EfiInput, BinaryInput, Compress=False, ClassCode=None,
                        Revision=None, DeviceId=None, VendorId=None, IsMakefile=False):
        InputList = []
        Cmd = ["EfiRom"]
        if EfiInput:

            if Compress:
                Cmd.append("-ec")
            else:
                Cmd.append("-e")

            for EfiFile in EfiInput:
                Cmd.append(EfiFile)
                InputList.append (EfiFile)

        if BinaryInput:
            Cmd.append("-b")
            for BinFile in BinaryInput:
                Cmd.append(BinFile)
                InputList.append (BinFile)

        # Check List
        if not GenFdsGlobalVariable.NeedsUpdate(Output, InputList) and not IsMakefile:
            return
        GenFdsGlobalVariable.DebugLogger(EdkLogger.DEBUG_5, "%s needs update because of newer %s" % (Output, InputList))

        if ClassCode:
            Cmd += ("-l", ClassCode)
        if Revision:
            Cmd += ("-r", Revision)
        if DeviceId:
            Cmd += ("-i", DeviceId)
        if VendorId:
            Cmd += ("-f", VendorId)

        Cmd += ("-o", Output)
        if IsMakefile:
            if " ".join(Cmd).strip() not in GenFdsGlobalVariable.SecCmdList:
                GenFdsGlobalVariable.SecCmdList.append(" ".join(Cmd).strip())
        else:
            GenFdsGlobalVariable.CallExternalTool(Cmd, "Failed to generate option rom")

    @staticmethod
    def GuidTool(Output, Input, ToolPath, Options='', returnValue=[], IsMakefile=False):
        if not GenFdsGlobalVariable.NeedsUpdate(Output, Input) and not IsMakefile:
            return
        GenFdsGlobalVariable.DebugLogger(EdkLogger.DEBUG_5, "%s needs update because of newer %s" % (Output, Input))

        Cmd = [ToolPath, ]
        Cmd += Options.split(' ')
        Cmd += ("-o", Output)
        Cmd += Input
        if IsMakefile:
            if " ".join(Cmd).strip() not in GenFdsGlobalVariable.SecCmdList:
                GenFdsGlobalVariable.SecCmdList.append(" ".join(Cmd).strip())
        else:
            GenFdsGlobalVariable.CallExternalTool(Cmd, "Failed to call " + ToolPath, returnValue)

    @staticmethod
    def CallExternalTool (cmd, errorMess, returnValue=[]):

        if type(cmd) not in (tuple, list):
            GenFdsGlobalVariable.ErrorLogger("ToolError!  Invalid parameter type in call to CallExternalTool")

        if GenFdsGlobalVariable.DebugLevel != -1:
            cmd += ('--debug', str(GenFdsGlobalVariable.DebugLevel))
            GenFdsGlobalVariable.InfLogger (cmd)

        if GenFdsGlobalVariable.VerboseMode:
            cmd += ('-v',)
            GenFdsGlobalVariable.InfLogger (cmd)
        else:
            stdout.write ('#')
            stdout.flush()
            GenFdsGlobalVariable.SharpCounter = GenFdsGlobalVariable.SharpCounter + 1
            if GenFdsGlobalVariable.SharpCounter % GenFdsGlobalVariable.SharpNumberPerLine == 0:
                stdout.write('\n')

        try:
            PopenObject = Popen(' '.join(cmd), stdout=PIPE, stderr=PIPE, shell=True)
        except Exception as X:
            EdkLogger.error("GenFds", COMMAND_FAILURE, ExtraData="%s: %s" % (str(X), cmd[0]))
        (out, error) = PopenObject.communicate()

        while PopenObject.returncode is None:
            PopenObject.wait()
        if returnValue != [] and returnValue[0] != 0:
            #get command return value
            returnValue[0] = PopenObject.returncode
            return
        if PopenObject.returncode != 0 or GenFdsGlobalVariable.VerboseMode or GenFdsGlobalVariable.DebugLevel != -1:
            GenFdsGlobalVariable.InfLogger ("Return Value = %d" % PopenObject.returncode)
            GenFdsGlobalVariable.InfLogger(out.decode(encoding='utf-8', errors='ignore'))
            GenFdsGlobalVariable.InfLogger(error.decode(encoding='utf-8', errors='ignore'))
            if PopenObject.returncode != 0:
                print("###", cmd)
                EdkLogger.error("GenFds", COMMAND_FAILURE, errorMess)

    @staticmethod
    def VerboseLogger (msg):
        EdkLogger.verbose(msg)

    @staticmethod
    def InfLogger (msg):
        EdkLogger.info(msg)

    @staticmethod
    def ErrorLogger (msg, File=None, Line=None, ExtraData=None):
        EdkLogger.error('GenFds', GENFDS_ERROR, msg, File, Line, ExtraData)

    @staticmethod
    def DebugLogger (Level, msg):
        EdkLogger.debug(Level, msg)

    ## MacroExtend()
    #
    #   @param  Str           String that may contain macro
    #   @param  MacroDict     Dictionary that contains macro value pair
    #
    @staticmethod
    def MacroExtend (Str, MacroDict=None, Arch=DataType.TAB_COMMON):
        if Str is None:
            return None

        Dict = {'$(WORKSPACE)': GenFdsGlobalVariable.WorkSpaceDir,
#                '$(OUTPUT_DIRECTORY)': GenFdsGlobalVariable.OutputDirFromDsc,
                '$(TARGET)': GenFdsGlobalVariable.TargetName,
                '$(TOOL_CHAIN_TAG)': GenFdsGlobalVariable.ToolChainTag,
                '$(SPACE)': ' '
               }

        if Arch != DataType.TAB_COMMON and Arch in GenFdsGlobalVariable.ArchList:
            OutputDir = GenFdsGlobalVariable.OutputDirFromDscDict[Arch]
        else:
            OutputDir = GenFdsGlobalVariable.OutputDirFromDscDict[GenFdsGlobalVariable.ArchList[0]]

        Dict['$(OUTPUT_DIRECTORY)'] = OutputDir

        if MacroDict:
            Dict.update(MacroDict)

        for key in Dict:
            if Str.find(key) >= 0:
                Str = Str.replace (key, Dict[key])

        if Str.find('$(ARCH)') >= 0:
            if len(GenFdsGlobalVariable.ArchList) == 1:
                Str = Str.replace('$(ARCH)', GenFdsGlobalVariable.ArchList[0])
            else:
                EdkLogger.error("GenFds", GENFDS_ERROR, "No way to determine $(ARCH) for %s" % Str)

        return Str

    ## GetPcdValue()
    #
    #   @param  PcdPattern           pattern that labels a PCD.
    #
    @staticmethod
    def GetPcdValue (PcdPattern):
        if PcdPattern is None:
            return None
        if PcdPattern.startswith('PCD('):
            PcdPair = PcdPattern[4:].rstrip(')').strip().split('.')
        else:
            PcdPair = PcdPattern.strip().split('.')
        TokenSpace = PcdPair[0]
        TokenCName = PcdPair[1]

        for Arch in GenFdsGlobalVariable.ArchList:
            Platform = GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, Arch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
            PcdDict = Platform.Pcds
            for Key in PcdDict:
                PcdObj = PcdDict[Key]
                if (PcdObj.TokenCName == TokenCName) and (PcdObj.TokenSpaceGuidCName == TokenSpace):
                    if PcdObj.Type != DataType.TAB_PCDS_FIXED_AT_BUILD:
                        EdkLogger.error("GenFds", GENFDS_ERROR, "%s is not FixedAtBuild type." % PcdPattern)
                    if PcdObj.DatumType != DataType.TAB_VOID:
                        EdkLogger.error("GenFds", GENFDS_ERROR, "%s is not VOID* datum type." % PcdPattern)

                    return PcdObj.DefaultValue

            for Package in GenFdsGlobalVariable.WorkSpace.GetPackageList(GenFdsGlobalVariable.ActivePlatform,
                                                                         Arch,
                                                                         GenFdsGlobalVariable.TargetName,
                                                                         GenFdsGlobalVariable.ToolChainTag):
                PcdDict = Package.Pcds
                for Key in PcdDict:
                    PcdObj = PcdDict[Key]
                    if (PcdObj.TokenCName == TokenCName) and (PcdObj.TokenSpaceGuidCName == TokenSpace):
                        if PcdObj.Type != DataType.TAB_PCDS_FIXED_AT_BUILD:
                            EdkLogger.error("GenFds", GENFDS_ERROR, "%s is not FixedAtBuild type." % PcdPattern)
                        if PcdObj.DatumType != DataType.TAB_VOID:
                            EdkLogger.error("GenFds", GENFDS_ERROR, "%s is not VOID* datum type." % PcdPattern)

                        return PcdObj.DefaultValue

        return ''

## FindExtendTool()
#
#  Find location of tools to process data
#
#  @param  KeyStringList    Filter for inputs of section generation
#  @param  CurrentArchList  Arch list
#  @param  NameGuid         The Guid name
#
def FindExtendTool(KeyStringList, CurrentArchList, NameGuid):
    if GenFdsGlobalVariable.GuidToolDefinition:
        if NameGuid in GenFdsGlobalVariable.GuidToolDefinition:
            return GenFdsGlobalVariable.GuidToolDefinition[NameGuid]

    ToolDefObj = ToolDefDict((os.path.join(os.getenv("WORKSPACE"), "Conf")))
    ToolDef = ToolDefObj.ToolDef
    ToolDb = ToolDef.ToolsDefTxtDatabase
    # if user not specify filter, try to deduce it from global data.
    if KeyStringList is None or KeyStringList == []:
        Target = GenFdsGlobalVariable.TargetName
        ToolChain = GenFdsGlobalVariable.ToolChainTag
        if ToolChain not in ToolDb['TOOL_CHAIN_TAG']:
            EdkLogger.error("GenFds", GENFDS_ERROR, "Can not find external tool because tool tag %s is not defined in tools_def.txt!" % ToolChain)
        KeyStringList = [Target + '_' + ToolChain + '_' + CurrentArchList[0]]
        for Arch in CurrentArchList:
            if Target + '_' + ToolChain + '_' + Arch not in KeyStringList:
                KeyStringList.append(Target + '_' + ToolChain + '_' + Arch)

    ToolPathTmp = None
    ToolOption = None
    for Arch in CurrentArchList:
        MatchItem = None
        MatchPathItem = None
        MatchOptionsItem = None
        for KeyString in KeyStringList:
            KeyStringBuildTarget, KeyStringToolChain, KeyStringArch = KeyString.split('_')
            if KeyStringArch != Arch:
                continue
            for Item in ToolDef.ToolsDefTxtDictionary:
                if len(Item.split('_')) < 5:
                    continue
                ItemTarget, ItemToolChain, ItemArch, ItemTool, ItemAttr = Item.split('_')
                if ItemTarget == DataType.TAB_STAR:
                    ItemTarget = KeyStringBuildTarget
                if ItemToolChain == DataType.TAB_STAR:
                    ItemToolChain = KeyStringToolChain
                if ItemArch == DataType.TAB_STAR:
                    ItemArch = KeyStringArch
                if ItemTarget != KeyStringBuildTarget:
                    continue
                if ItemToolChain != KeyStringToolChain:
                    continue
                if ItemArch != KeyStringArch:
                    continue
                if ItemAttr != DataType.TAB_GUID:
                    # Not GUID attribute
                    continue
                if ToolDef.ToolsDefTxtDictionary[Item].lower() != NameGuid.lower():
                    # No GUID value match
                    continue
                if MatchItem:
                    if MatchItem.split('_')[3] == ItemTool:
                        # Tool name is the same
                        continue
                    if CalculatePriorityValue(MatchItem) > CalculatePriorityValue(Item):
                        # Current MatchItem is higher priority than new match item
                        continue
                MatchItem = Item
            if not MatchItem:
                continue
            ToolName = MatchItem.split('_')[3]
            for Item in ToolDef.ToolsDefTxtDictionary:
                if len(Item.split('_')) < 5:
                    continue
                ItemTarget, ItemToolChain, ItemArch, ItemTool, ItemAttr = Item.split('_')
                if ItemTarget == DataType.TAB_STAR:
                    ItemTarget = KeyStringBuildTarget
                if ItemToolChain == DataType.TAB_STAR:
                    ItemToolChain = KeyStringToolChain
                if ItemArch == DataType.TAB_STAR:
                    ItemArch = KeyStringArch
                if ItemTarget != KeyStringBuildTarget:
                    continue
                if ItemToolChain != KeyStringToolChain:
                    continue
                if ItemArch != KeyStringArch:
                    continue
                if ItemTool != ToolName:
                    continue
                if ItemAttr == 'PATH':
                    if MatchPathItem:
                        if CalculatePriorityValue(MatchPathItem) <= CalculatePriorityValue(Item):
                            MatchPathItem = Item
                    else:
                        MatchPathItem = Item
                if ItemAttr == 'FLAGS':
                    if MatchOptionsItem:
                        if CalculatePriorityValue(MatchOptionsItem) <= CalculatePriorityValue(Item):
                            MatchOptionsItem = Item
                    else:
                        MatchOptionsItem = Item
        if MatchPathItem:
            ToolPathTmp = ToolDef.ToolsDefTxtDictionary[MatchPathItem]
        if MatchOptionsItem:
            ToolOption = ToolDef.ToolsDefTxtDictionary[MatchOptionsItem]

    for Arch in CurrentArchList:
        MatchItem = None
        MatchPathItem = None
        MatchOptionsItem = None
        for KeyString in KeyStringList:
            KeyStringBuildTarget, KeyStringToolChain, KeyStringArch = KeyString.split('_')
            if KeyStringArch != Arch:
                continue
            Platform = GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, Arch, KeyStringBuildTarget, KeyStringToolChain]
            for Item in Platform.BuildOptions:
                if len(Item[1].split('_')) < 5:
                    continue
                ItemTarget, ItemToolChain, ItemArch, ItemTool, ItemAttr = Item[1].split('_')
                if ItemTarget == DataType.TAB_STAR:
                    ItemTarget = KeyStringBuildTarget
                if ItemToolChain == DataType.TAB_STAR:
                    ItemToolChain = KeyStringToolChain
                if ItemArch == DataType.TAB_STAR:
                    ItemArch = KeyStringArch
                if ItemTarget != KeyStringBuildTarget:
                    continue
                if ItemToolChain != KeyStringToolChain:
                    continue
                if ItemArch != KeyStringArch:
                    continue
                if ItemAttr != DataType.TAB_GUID:
                    # Not GUID attribute match
                    continue
                if Platform.BuildOptions[Item].lower() != NameGuid.lower():
                    # No GUID value match
                    continue
                if MatchItem:
                    if MatchItem[1].split('_')[3] == ItemTool:
                        # Tool name is the same
                        continue
                    if CalculatePriorityValue(MatchItem[1]) > CalculatePriorityValue(Item[1]):
                        # Current MatchItem is higher priority than new match item
                        continue
                MatchItem = Item
            if not MatchItem:
                continue
            ToolName = MatchItem[1].split('_')[3]
            for Item in Platform.BuildOptions:
                if len(Item[1].split('_')) < 5:
                    continue
                ItemTarget, ItemToolChain, ItemArch, ItemTool, ItemAttr = Item[1].split('_')
                if ItemTarget == DataType.TAB_STAR:
                    ItemTarget = KeyStringBuildTarget
                if ItemToolChain == DataType.TAB_STAR:
                    ItemToolChain = KeyStringToolChain
                if ItemArch == DataType.TAB_STAR:
                    ItemArch = KeyStringArch
                if ItemTarget != KeyStringBuildTarget:
                    continue
                if ItemToolChain != KeyStringToolChain:
                    continue
                if ItemArch != KeyStringArch:
                    continue
                if ItemTool != ToolName:
                    continue
                if ItemAttr == 'PATH':
                    if MatchPathItem:
                        if CalculatePriorityValue(MatchPathItem[1]) <= CalculatePriorityValue(Item[1]):
                            MatchPathItem = Item
                    else:
                        MatchPathItem = Item
                if ItemAttr == 'FLAGS':
                    if MatchOptionsItem:
                        if CalculatePriorityValue(MatchOptionsItem[1]) <= CalculatePriorityValue(Item[1]):
                            MatchOptionsItem = Item
                    else:
                        MatchOptionsItem = Item
    if MatchPathItem:
        ToolPathTmp = Platform.BuildOptions[MatchPathItem]
    if MatchOptionsItem:
        ToolOption = Platform.BuildOptions[MatchOptionsItem]
    GenFdsGlobalVariable.GuidToolDefinition[NameGuid] = (ToolPathTmp, ToolOption)
    return ToolPathTmp, ToolOption
