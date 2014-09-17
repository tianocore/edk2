## @file
# Global variables for GenFds
#
#  Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
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
import Common.LongFilePathOs as os
import sys
import subprocess
import struct
import array

from Common.BuildToolError import *
from Common import EdkLogger
from Common.Misc import SaveFileOnChange

from Common.TargetTxtClassObject import TargetTxtClassObject
from Common.ToolDefClassObject import ToolDefClassObject
from AutoGen.BuildEngine import BuildRule
import Common.DataType as DataType
from Common.Misc import PathClass
from Common.LongFilePathSupport import OpenLongFilePath as open

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
    EdkSourceDir = ''
    OutputDirFromDscDict = {}
    TargetName = ''
    ToolChainTag = ''
    RuleDict = {}
    ArchList = None
    VtfDict = {}
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
    
    BuildRuleFamily = "MSFT"
    ToolChainFamily = "MSFT"
    __BuildRuleDatabase = None
    
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

    SectionHeader = struct.Struct("3B 1B")
    
    ## LoadBuildRule
    #
    @staticmethod
    def __LoadBuildRule():
        if GenFdsGlobalVariable.__BuildRuleDatabase:
            return GenFdsGlobalVariable.__BuildRuleDatabase
        BuildConfigurationFile = os.path.normpath(os.path.join(GenFdsGlobalVariable.ConfDir, "target.txt"))
        TargetTxt = TargetTxtClassObject()
        if os.path.isfile(BuildConfigurationFile) == True:
            TargetTxt.LoadTargetTxtFile(BuildConfigurationFile)
            if DataType.TAB_TAT_DEFINES_BUILD_RULE_CONF in TargetTxt.TargetTxtDictionary:
                BuildRuleFile = TargetTxt.TargetTxtDictionary[DataType.TAB_TAT_DEFINES_BUILD_RULE_CONF]
            if BuildRuleFile in [None, '']:
                BuildRuleFile = 'Conf/build_rule.txt'
            GenFdsGlobalVariable.__BuildRuleDatabase = BuildRule(BuildRuleFile)
            ToolDefinitionFile = TargetTxt.TargetTxtDictionary[DataType.TAB_TAT_DEFINES_TOOL_CHAIN_CONF]
            if ToolDefinitionFile == '':
                ToolDefinitionFile = "Conf/tools_def.txt"
            if os.path.isfile(ToolDefinitionFile):
                ToolDef = ToolDefClassObject()
                ToolDef.LoadToolDefFile(ToolDefinitionFile)
                ToolDefinition = ToolDef.ToolsDefTxtDatabase
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
            Arch = 'COMMON'

        if not Arch in GenFdsGlobalVariable.OutputDirDict:
            return {}

        BuildRuleDatabase = GenFdsGlobalVariable.__LoadBuildRule()
        if not BuildRuleDatabase:
            return {}

        PathClassObj = PathClass(Inf.MetaFile.File,
                                 GenFdsGlobalVariable.WorkSpaceDir)
        Macro = {}
        Macro["WORKSPACE"             ] = GenFdsGlobalVariable.WorkSpaceDir
        Macro["MODULE_NAME"           ] = Inf.BaseName
        Macro["MODULE_GUID"           ] = Inf.Guid
        Macro["MODULE_VERSION"        ] = Inf.Version
        Macro["MODULE_TYPE"           ] = Inf.ModuleType
        Macro["MODULE_FILE"           ] = str(PathClassObj)
        Macro["MODULE_FILE_BASE_NAME" ] = PathClassObj.BaseName
        Macro["MODULE_RELATIVE_DIR"   ] = PathClassObj.SubDir
        Macro["MODULE_DIR"            ] = PathClassObj.SubDir

        Macro["BASE_NAME"             ] = Inf.BaseName

        Macro["ARCH"                  ] = Arch
        Macro["TOOLCHAIN"             ] = GenFdsGlobalVariable.ToolChainTag
        Macro["TOOLCHAIN_TAG"         ] = GenFdsGlobalVariable.ToolChainTag
        Macro["TOOL_CHAIN_TAG"        ] = GenFdsGlobalVariable.ToolChainTag
        Macro["TARGET"                ] = GenFdsGlobalVariable.TargetName

        Macro["BUILD_DIR"             ] = GenFdsGlobalVariable.OutputDirDict[Arch]
        Macro["BIN_DIR"               ] = os.path.join(GenFdsGlobalVariable.OutputDirDict[Arch], Arch)
        Macro["LIB_DIR"               ] = os.path.join(GenFdsGlobalVariable.OutputDirDict[Arch], Arch)
        BuildDir = os.path.join(
            GenFdsGlobalVariable.OutputDirDict[Arch],
            Arch,
            PathClassObj.SubDir,
            PathClassObj.BaseName
        )
        Macro["MODULE_BUILD_DIR"      ] = BuildDir
        Macro["OUTPUT_DIR"            ] = os.path.join(BuildDir, "OUTPUT")
        Macro["DEBUG_DIR"             ] = os.path.join(BuildDir, "DEBUG")

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
                if File.TagName in ("", "*", GenFdsGlobalVariable.ToolChainTag) and \
                    File.ToolChainFamily in ("", "*", GenFdsGlobalVariable.ToolChainFamily):
                    FileList.append((File, DataType.TAB_UNKNOWN_FILE))

        for File in Inf.Binaries:
            if File.Target in ['COMMON', '*', GenFdsGlobalVariable.TargetName]:
                FileList.append((File, File.Type))

        for File, FileType in FileList:
            LastTarget = None
            RuleChain = []
            SourceList = [File]
            Index = 0
            while Index < len(SourceList):
                Source = SourceList[Index]
                Index = Index + 1
    
                if File.IsBinary and File == Source and Inf.Binaries != None and File in Inf.Binaries:
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

        return list(TargetList)

    ## SetDir()
    #
    #   @param  OutputDir           Output directory
    #   @param  FdfParser           FDF contents parser
    #   @param  Workspace           The directory of workspace
    #   @param  ArchList            The Arch list of platform
    #
    def SetDir (OutputDir, FdfParser, WorkSpace, ArchList):
        GenFdsGlobalVariable.VerboseLogger( "GenFdsGlobalVariable.OutputDir :%s" %OutputDir)
#        GenFdsGlobalVariable.OutputDirDict = OutputDir
        GenFdsGlobalVariable.FdfParser = FdfParser
        GenFdsGlobalVariable.WorkSpace = WorkSpace
        GenFdsGlobalVariable.FvDir = os.path.join(GenFdsGlobalVariable.OutputDirDict[ArchList[0]], 'FV')
        if not os.path.exists(GenFdsGlobalVariable.FvDir) :
            os.makedirs(GenFdsGlobalVariable.FvDir)
        GenFdsGlobalVariable.FfsDir = os.path.join(GenFdsGlobalVariable.FvDir, 'Ffs')
        if not os.path.exists(GenFdsGlobalVariable.FfsDir) :
            os.makedirs(GenFdsGlobalVariable.FfsDir)
        if ArchList != None:
            GenFdsGlobalVariable.ArchList = ArchList

        T_CHAR_LF = '\n'
        #
        # Create FV Address inf file
        #
        GenFdsGlobalVariable.FvAddressFileName = os.path.join(GenFdsGlobalVariable.FfsDir, 'FvAddress.inf')
        FvAddressFile = open (GenFdsGlobalVariable.FvAddressFileName, 'w')
        #
        # Add [Options]
        #
        FvAddressFile.writelines("[options]" + T_CHAR_LF)
        BsAddress = '0'
        for Arch in ArchList:
            if GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, Arch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag].BsBaseAddress:
                BsAddress = GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, Arch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag].BsBaseAddress
                break

        FvAddressFile.writelines("EFI_BOOT_DRIVER_BASE_ADDRESS = " + \
                                       BsAddress          + \
                                       T_CHAR_LF)

        RtAddress = '0'
        for Arch in ArchList:
            if GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, Arch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag].RtBaseAddress:
                RtAddress = GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, Arch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag].RtBaseAddress

        FvAddressFile.writelines("EFI_RUNTIME_DRIVER_BASE_ADDRESS = " + \
                                       RtAddress          + \
                                       T_CHAR_LF)

        FvAddressFile.close()

    ## ReplaceWorkspaceMacro()
    #
    #   @param  String           String that may contain macro
    #
    def ReplaceWorkspaceMacro(String):
        Str = String.replace('$(WORKSPACE)', GenFdsGlobalVariable.WorkSpaceDir)
        if os.path.exists(Str):
            if not os.path.isabs(Str):
                Str = os.path.abspath(Str)
        else:
            Str = os.path.join(GenFdsGlobalVariable.WorkSpaceDir, String)
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
        if Input == None or len(Input) == 0:
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
                        GuidHdrLen=None, GuidAttr=[], Ui=None, Ver=None, InputAlign=None, BuildNumber=None):
        Cmd = ["GenSec"]
        if Type not in [None, '']:
            Cmd += ["-s", Type]
        if CompressionType not in [None, '']:
            Cmd += ["-c", CompressionType]
        if Guid != None:
            Cmd += ["-g", Guid]
        if GuidHdrLen not in [None, '']:
            Cmd += ["-l", GuidHdrLen]
        if len(GuidAttr) != 0:
            #Add each guided attribute
            for Attr in GuidAttr:
                Cmd += ["-r", Attr]
        if InputAlign != None:
            #Section Align is only for dummy section without section type
            for SecAlign in InputAlign:
                Cmd += ["--sectionalign", SecAlign]

        CommandFile = Output + '.txt'
        if Ui not in [None, '']:
            #Cmd += ["-n", '"' + Ui + '"']
            SectionData = array.array('B', [0,0,0,0])
            SectionData.fromstring(Ui.encode("utf_16_le"))
            SectionData.append(0)
            SectionData.append(0)
            Len = len(SectionData)
            GenFdsGlobalVariable.SectionHeader.pack_into(SectionData, 0, Len & 0xff, (Len >> 8) & 0xff, (Len >> 16) & 0xff, 0x15)
            SaveFileOnChange(Output,  SectionData.tostring())
        elif Ver not in [None, '']:
            Cmd += ["-n", Ver]
            if BuildNumber:
                Cmd += ["-j", BuildNumber]
            Cmd += ["-o", Output]

            SaveFileOnChange(CommandFile, ' '.join(Cmd), False)
            if not GenFdsGlobalVariable.NeedsUpdate(Output, list(Input) + [CommandFile]):
                return

            GenFdsGlobalVariable.CallExternalTool(Cmd, "Failed to generate section")
        else:
            Cmd += ["-o", Output]
            Cmd += Input

            SaveFileOnChange(CommandFile, ' '.join(Cmd), False)
            if GenFdsGlobalVariable.NeedsUpdate(Output, list(Input) + [CommandFile]):
                GenFdsGlobalVariable.DebugLogger(EdkLogger.DEBUG_5, "%s needs update because of newer %s" % (Output, Input))
                GenFdsGlobalVariable.CallExternalTool(Cmd, "Failed to generate section")

            if (os.path.getsize(Output) >= GenFdsGlobalVariable.LARGE_FILE_SIZE and
                GenFdsGlobalVariable.LargeFileInFvFlags):
                GenFdsGlobalVariable.LargeFileInFvFlags[-1] = True 

    @staticmethod
    def GetAlignment (AlignString):
        if AlignString == None:
            return 0
        if AlignString in ("1K", "2K", "4K", "8K", "16K", "32K", "64K"):
            return int (AlignString.rstrip('K')) * 1024
        else:
            return int (AlignString)

    @staticmethod
    def GenerateFfs(Output, Input, Type, Guid, Fixed=False, CheckSum=False, Align=None,
                    SectionAlign=None):
        Cmd = ["GenFfs", "-t", Type, "-g", Guid]
        if Fixed == True:
            Cmd += ["-x"]
        if CheckSum:
            Cmd += ["-s"]
        if Align not in [None, '']:
            Cmd += ["-a", Align]

        Cmd += ["-o", Output]
        for I in range(0, len(Input)):
            Cmd += ("-i", Input[I])
            if SectionAlign not in [None, '', []] and SectionAlign[I] not in [None, '']:
                Cmd += ("-n", SectionAlign[I])

        CommandFile = Output + '.txt'
        SaveFileOnChange(CommandFile, ' '.join(Cmd), False)
        if not GenFdsGlobalVariable.NeedsUpdate(Output, list(Input) + [CommandFile]):
            return
        GenFdsGlobalVariable.DebugLogger(EdkLogger.DEBUG_5, "%s needs update because of newer %s" % (Output, Input))

        GenFdsGlobalVariable.CallExternalTool(Cmd, "Failed to generate FFS")

    @staticmethod
    def GenerateFirmwareVolume(Output, Input, BaseAddress=None, ForceRebase=None, Capsule=False, Dump=False,
                               AddressFile=None, MapFile=None, FfsList=[], FileSystemGuid=None):
        if not GenFdsGlobalVariable.NeedsUpdate(Output, Input+FfsList):
            return
        GenFdsGlobalVariable.DebugLogger(EdkLogger.DEBUG_5, "%s needs update because of newer %s" % (Output, Input))

        Cmd = ["GenFv"]
        if BaseAddress not in [None, '']:
            Cmd += ["-r", BaseAddress]
        
        if ForceRebase == False:
            Cmd +=["-F", "FALSE"]
        elif ForceRebase == True:
            Cmd +=["-F", "TRUE"]
            
        if Capsule:
            Cmd += ["-c"]
        if Dump:
            Cmd += ["-p"]
        if AddressFile not in [None, '']:
            Cmd += ["-a", AddressFile]
        if MapFile not in [None, '']:
            Cmd += ["-m", MapFile]
        if FileSystemGuid:
            Cmd += ["-g", FileSystemGuid]
        Cmd += ["-o", Output]
        for I in Input:
            Cmd += ["-i", I]

        GenFdsGlobalVariable.CallExternalTool(Cmd, "Failed to generate FV")

    @staticmethod
    def GenerateVtf(Output, Input, BaseAddress=None, FvSize=None):
        if not GenFdsGlobalVariable.NeedsUpdate(Output, Input):
            return
        GenFdsGlobalVariable.DebugLogger(EdkLogger.DEBUG_5, "%s needs update because of newer %s" % (Output, Input))

        Cmd = ["GenVtf"]
        if BaseAddress not in [None, ''] and FvSize not in [None, ''] \
            and len(BaseAddress) == len(FvSize):
            for I in range(0, len(BaseAddress)):
                Cmd += ["-r", BaseAddress[I], "-s", FvSize[I]]
        Cmd += ["-o", Output]
        for F in Input:
            Cmd += ["-f", F]

        GenFdsGlobalVariable.CallExternalTool(Cmd, "Failed to generate VTF")

    @staticmethod
    def GenerateFirmwareImage(Output, Input, Type="efi", SubType=None, Zero=False,
                              Strip=False, Replace=False, TimeStamp=None, Join=False,
                              Align=None, Padding=None, Convert=False):
        if not GenFdsGlobalVariable.NeedsUpdate(Output, Input):
            return
        GenFdsGlobalVariable.DebugLogger(EdkLogger.DEBUG_5, "%s needs update because of newer %s" % (Output, Input))

        Cmd = ["GenFw"]
        if Type.lower() == "te":
            Cmd += ["-t"]
        if SubType not in [None, '']:
            Cmd += ["-e", SubType]
        if TimeStamp not in [None, '']:
            Cmd += ["-s", TimeStamp]
        if Align not in [None, '']:
            Cmd += ["-a", Align]
        if Padding not in [None, '']:
            Cmd += ["-p", Padding]
        if Zero:
            Cmd += ["-z"]
        if Strip:
            Cmd += ["-l"]
        if Replace:
            Cmd += ["-r"]
        if Join:
            Cmd += ["-j"]
        if Convert:
            Cmd += ["-m"]
        Cmd += ["-o", Output]
        Cmd += Input

        GenFdsGlobalVariable.CallExternalTool(Cmd, "Failed to generate firmware image")

    @staticmethod
    def GenerateOptionRom(Output, EfiInput, BinaryInput, Compress=False, ClassCode=None,
                        Revision=None, DeviceId=None, VendorId=None):
        InputList = []   
        Cmd = ["EfiRom"]
        if len(EfiInput) > 0:
            
            if Compress:
                Cmd += ["-ec"]
            else:
                Cmd += ["-e"]
                
            for EfiFile in EfiInput:
                Cmd += [EfiFile]
                InputList.append (EfiFile)
        
        if len(BinaryInput) > 0:
            Cmd += ["-b"]
            for BinFile in BinaryInput:
                Cmd += [BinFile]
                InputList.append (BinFile)

        # Check List
        if not GenFdsGlobalVariable.NeedsUpdate(Output, InputList):
            return
        GenFdsGlobalVariable.DebugLogger(EdkLogger.DEBUG_5, "%s needs update because of newer %s" % (Output, InputList))
                        
        if ClassCode != None:
            Cmd += ["-l", ClassCode]
        if Revision != None:
            Cmd += ["-r", Revision]
        if DeviceId != None:
            Cmd += ["-i", DeviceId]
        if VendorId != None:
            Cmd += ["-f", VendorId]

        Cmd += ["-o", Output]    
        GenFdsGlobalVariable.CallExternalTool(Cmd, "Failed to generate option rom")

    @staticmethod
    def GuidTool(Output, Input, ToolPath, Options='', returnValue=[]):
        if not GenFdsGlobalVariable.NeedsUpdate(Output, Input):
            return
        GenFdsGlobalVariable.DebugLogger(EdkLogger.DEBUG_5, "%s needs update because of newer %s" % (Output, Input))

        Cmd = [ToolPath, ]
        Cmd += Options.split(' ')
        Cmd += ["-o", Output]
        Cmd += Input

        GenFdsGlobalVariable.CallExternalTool(Cmd, "Failed to call " + ToolPath, returnValue)

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
            sys.stdout.write ('#')
            sys.stdout.flush()
            GenFdsGlobalVariable.SharpCounter = GenFdsGlobalVariable.SharpCounter + 1
            if GenFdsGlobalVariable.SharpCounter % GenFdsGlobalVariable.SharpNumberPerLine == 0:
                sys.stdout.write('\n')

        try:
            PopenObject = subprocess.Popen(' '.join(cmd), stdout=subprocess.PIPE, stderr= subprocess.PIPE, shell=True)
        except Exception, X:
            EdkLogger.error("GenFds", COMMAND_FAILURE, ExtraData="%s: %s" % (str(X), cmd[0]))
        (out, error) = PopenObject.communicate()

        while PopenObject.returncode == None :
            PopenObject.wait()
        if returnValue != [] and returnValue[0] != 0:
            #get command return value
            returnValue[0] = PopenObject.returncode
            return
        if PopenObject.returncode != 0 or GenFdsGlobalVariable.VerboseMode or GenFdsGlobalVariable.DebugLevel != -1:
            GenFdsGlobalVariable.InfLogger ("Return Value = %d" %PopenObject.returncode)
            GenFdsGlobalVariable.InfLogger (out)
            GenFdsGlobalVariable.InfLogger (error)
            if PopenObject.returncode != 0:
                print "###", cmd
                EdkLogger.error("GenFds", COMMAND_FAILURE, errorMess)

    def VerboseLogger (msg):
        EdkLogger.verbose(msg)

    def InfLogger (msg):
        EdkLogger.info(msg)

    def ErrorLogger (msg, File = None, Line = None, ExtraData = None):
        EdkLogger.error('GenFds', GENFDS_ERROR, msg, File, Line, ExtraData)

    def DebugLogger (Level, msg):
        EdkLogger.debug(Level, msg)

    ## ReplaceWorkspaceMacro()
    #
    #   @param  Str           String that may contain macro
    #   @param  MacroDict     Dictionary that contains macro value pair
    #
    def MacroExtend (Str, MacroDict = {}, Arch = 'COMMON'):
        if Str == None :
            return None

        Dict = {'$(WORKSPACE)'   : GenFdsGlobalVariable.WorkSpaceDir,
                '$(EDK_SOURCE)'  : GenFdsGlobalVariable.EdkSourceDir,
#                '$(OUTPUT_DIRECTORY)': GenFdsGlobalVariable.OutputDirFromDsc,
                '$(TARGET)' : GenFdsGlobalVariable.TargetName,
                '$(TOOL_CHAIN_TAG)' : GenFdsGlobalVariable.ToolChainTag,
                '$(SPACE)' : ' '
               }
        OutputDir = GenFdsGlobalVariable.OutputDirFromDscDict[GenFdsGlobalVariable.ArchList[0]]
        if Arch != 'COMMON' and Arch in GenFdsGlobalVariable.ArchList:
            OutputDir = GenFdsGlobalVariable.OutputDirFromDscDict[Arch]

        Dict['$(OUTPUT_DIRECTORY)'] = OutputDir

        if MacroDict != None  and len (MacroDict) != 0:
            Dict.update(MacroDict)

        for key in Dict.keys():
            if Str.find(key) >= 0 :
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
    def GetPcdValue (PcdPattern):
        if PcdPattern == None :
            return None
        PcdPair = PcdPattern.lstrip('PCD(').rstrip(')').strip().split('.')
        TokenSpace = PcdPair[0]
        TokenCName = PcdPair[1]

        PcdValue = ''
        for Arch in GenFdsGlobalVariable.ArchList:
            Platform = GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, Arch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
            PcdDict = Platform.Pcds
            for Key in PcdDict:
                PcdObj = PcdDict[Key]
                if (PcdObj.TokenCName == TokenCName) and (PcdObj.TokenSpaceGuidCName == TokenSpace):
                    if PcdObj.Type != 'FixedAtBuild':
                        EdkLogger.error("GenFds", GENFDS_ERROR, "%s is not FixedAtBuild type." % PcdPattern)
                    if PcdObj.DatumType != 'VOID*':
                        EdkLogger.error("GenFds", GENFDS_ERROR, "%s is not VOID* datum type." % PcdPattern)
                        
                    PcdValue = PcdObj.DefaultValue
                    return PcdValue
                
            for Package in GenFdsGlobalVariable.WorkSpace.GetPackageList(GenFdsGlobalVariable.ActivePlatform, 
                                                                         Arch, 
                                                                         GenFdsGlobalVariable.TargetName, 
                                                                         GenFdsGlobalVariable.ToolChainTag):
                PcdDict = Package.Pcds
                for Key in PcdDict:
                    PcdObj = PcdDict[Key]
                    if (PcdObj.TokenCName == TokenCName) and (PcdObj.TokenSpaceGuidCName == TokenSpace):
                        if PcdObj.Type != 'FixedAtBuild':
                            EdkLogger.error("GenFds", GENFDS_ERROR, "%s is not FixedAtBuild type." % PcdPattern)
                        if PcdObj.DatumType != 'VOID*':
                            EdkLogger.error("GenFds", GENFDS_ERROR, "%s is not VOID* datum type." % PcdPattern)
                            
                        PcdValue = PcdObj.DefaultValue
                        return PcdValue

        return PcdValue

    SetDir = staticmethod(SetDir)
    ReplaceWorkspaceMacro = staticmethod(ReplaceWorkspaceMacro)
    CallExternalTool = staticmethod(CallExternalTool)
    VerboseLogger = staticmethod(VerboseLogger)
    InfLogger = staticmethod(InfLogger)
    ErrorLogger = staticmethod(ErrorLogger)
    DebugLogger = staticmethod(DebugLogger)
    MacroExtend = staticmethod (MacroExtend)
    GetPcdValue = staticmethod(GetPcdValue)
