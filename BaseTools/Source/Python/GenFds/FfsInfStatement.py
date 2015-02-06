## @file
# process FFS generation from INF statement
#
#  Copyright (c) 2007 - 2015, Intel Corporation. All rights reserved.<BR>
#  Copyright (c) 2014 Hewlett-Packard Development Company, L.P.<BR>
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
import Rule
import Common.LongFilePathOs as os
import StringIO
from struct import *
from GenFdsGlobalVariable import GenFdsGlobalVariable
import Ffs
import subprocess
import sys
import Section
import RuleSimpleFile
import RuleComplexFile
from CommonDataClass.FdfClass import FfsInfStatementClassObject
from Common.String import *
from Common.Misc import PathClass
from Common.Misc import GuidStructureByteArrayToGuidString
from Common.Misc import ProcessDuplicatedInf
from Common.Misc import GetVariableOffset
from Common import EdkLogger
from Common.BuildToolError import *
from GuidSection import GuidSection
from FvImageSection import FvImageSection
from Common.Misc import PeImageClass
from AutoGen.GenDepex import DependencyExpression
from PatchPcdValue.PatchPcdValue import PatchBinaryFile
from Common.LongFilePathSupport import CopyLongFilePath
from Common.LongFilePathSupport import OpenLongFilePath as open

## generate FFS from INF
#
#
class FfsInfStatement(FfsInfStatementClassObject):
    ## The mapping dictionary from datum type to its maximum number.
    _MAX_SIZE_TYPE = {"BOOLEAN":0x01, "UINT8":0xFF, "UINT16":0xFFFF, "UINT32":0xFFFFFFFF, "UINT64":0xFFFFFFFFFFFFFFFF}
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        FfsInfStatementClassObject.__init__(self)
        self.TargetOverrideList = []
        self.ShadowFromInfFile = None
        self.KeepRelocFromRule = None
        self.InDsc = True
        self.OptRomDefs = {}
        self.PiSpecVersion = '0x00000000'
        self.InfModule = None
        self.FinalTargetSuffixMap = {}
        self.CurrentLineNum = None
        self.CurrentLineContent = None
        self.FileName = None
        self.InfFileName = None
        self.OverrideGuid = None
        self.PatchedBinFile = ''
        self.MacroDict = {}

    ## GetFinalTargetSuffixMap() method
    #
    #    Get final build target list
    def GetFinalTargetSuffixMap(self):
        if not self.InfModule or not self.CurrentArch:
            return []
        if not self.FinalTargetSuffixMap:
            FinalBuildTargetList = GenFdsGlobalVariable.GetModuleCodaTargetList(self.InfModule, self.CurrentArch)
            for File in FinalBuildTargetList:
                self.FinalTargetSuffixMap.setdefault(os.path.splitext(File)[1], []).append(File)

            # Check if current INF module has DEPEX
            if '.depex' not in self.FinalTargetSuffixMap and self.InfModule.ModuleType != "USER_DEFINED" \
                and not self.InfModule.DxsFile and not self.InfModule.LibraryClass:
                ModuleType = self.InfModule.ModuleType
                PlatformDataBase = GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, self.CurrentArch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]

                if ModuleType != DataType.SUP_MODULE_USER_DEFINED:
                    for LibraryClass in PlatformDataBase.LibraryClasses.GetKeys():
                        if LibraryClass.startswith("NULL") and PlatformDataBase.LibraryClasses[LibraryClass, ModuleType]:
                            self.InfModule.LibraryClasses[LibraryClass] = PlatformDataBase.LibraryClasses[LibraryClass, ModuleType]

                StrModule = str(self.InfModule)
                PlatformModule = None
                if StrModule in PlatformDataBase.Modules:
                    PlatformModule = PlatformDataBase.Modules[StrModule]
                    for LibraryClass in PlatformModule.LibraryClasses:
                        if LibraryClass.startswith("NULL"):
                            self.InfModule.LibraryClasses[LibraryClass] = PlatformModule.LibraryClasses[LibraryClass]

                DependencyList = [self.InfModule]
                LibraryInstance = {}
                DepexList = []
                while len(DependencyList) > 0:
                    Module = DependencyList.pop(0)
                    if not Module:
                        continue
                    for Dep in Module.Depex[self.CurrentArch, ModuleType]:
                        if DepexList != []:
                            DepexList.append('AND')
                        DepexList.append('(')
                        DepexList.extend(Dep)
                        if DepexList[-1] == 'END':  # no need of a END at this time
                            DepexList.pop()
                        DepexList.append(')')
                    if 'BEFORE' in DepexList or 'AFTER' in DepexList:
                        break
                    for LibName in Module.LibraryClasses:
                        if LibName in LibraryInstance:
                            continue
                        if PlatformModule and LibName in PlatformModule.LibraryClasses:
                            LibraryPath = PlatformModule.LibraryClasses[LibName]
                        else:
                            LibraryPath = PlatformDataBase.LibraryClasses[LibName, ModuleType]
                        if not LibraryPath:
                            LibraryPath = Module.LibraryClasses[LibName]
                        if not LibraryPath:
                            continue
                        LibraryModule = GenFdsGlobalVariable.WorkSpace.BuildObject[LibraryPath, self.CurrentArch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
                        LibraryInstance[LibName] = LibraryModule
                        DependencyList.append(LibraryModule)
                if DepexList:
                    Dpx = DependencyExpression(DepexList, ModuleType, True)
                    if len(Dpx.PostfixNotation) != 0:
                        # It means this module has DEPEX
                        self.FinalTargetSuffixMap['.depex'] = [os.path.join(self.EfiOutputPath, self.BaseName) + '.depex']
        return self.FinalTargetSuffixMap

    ## __InfParse() method
    #
    #   Parse inf file to get module information
    #
    #   @param  self        The object pointer
    #   @param  Dict        dictionary contains macro and value pair
    #
    def __InfParse__(self, Dict = {}):

        GenFdsGlobalVariable.VerboseLogger( " Begine parsing INf file : %s" %self.InfFileName)

        self.InfFileName = self.InfFileName.replace('$(WORKSPACE)', '')
        if len(self.InfFileName) > 1 and self.InfFileName[0] == '\\' and self.InfFileName[1] == '\\':
            pass
        elif self.InfFileName[0] == '\\' or self.InfFileName[0] == '/' :
            self.InfFileName = self.InfFileName[1:]

        if self.InfFileName.find('$') == -1:
            InfPath = NormPath(self.InfFileName)
            if not os.path.exists(InfPath):
                InfPath = GenFdsGlobalVariable.ReplaceWorkspaceMacro(InfPath)
                if not os.path.exists(InfPath):
                    EdkLogger.error("GenFds", GENFDS_ERROR, "Non-existant Module %s !" % (self.InfFileName))

        self.CurrentArch = self.GetCurrentArch()
        #
        # Get the InfClass object
        #

        PathClassObj = PathClass(self.InfFileName, GenFdsGlobalVariable.WorkSpaceDir)
        ErrorCode, ErrorInfo = PathClassObj.Validate(".inf")
        if ErrorCode != 0:
            EdkLogger.error("GenFds", ErrorCode, ExtraData=ErrorInfo)

        if self.OverrideGuid:
            PathClassObj = ProcessDuplicatedInf(PathClassObj, self.OverrideGuid, GenFdsGlobalVariable.WorkSpaceDir)
        if self.CurrentArch != None:

            Inf = GenFdsGlobalVariable.WorkSpace.BuildObject[PathClassObj, self.CurrentArch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
            #
            # Set Ffs BaseName, MdouleGuid, ModuleType, Version, OutputPath
            #
            self.BaseName = Inf.BaseName
            self.ModuleGuid = Inf.Guid
            self.ModuleType = Inf.ModuleType
            if Inf.Specification != None and 'PI_SPECIFICATION_VERSION' in Inf.Specification:
                self.PiSpecVersion = Inf.Specification['PI_SPECIFICATION_VERSION']
            if Inf.AutoGenVersion < 0x00010005:
                self.ModuleType = Inf.ComponentType
            self.VersionString = Inf.Version
            self.BinFileList = Inf.Binaries
            self.SourceFileList = Inf.Sources
            if self.KeepReloc == None and Inf.Shadow:
                self.ShadowFromInfFile = Inf.Shadow

        else:
            Inf = GenFdsGlobalVariable.WorkSpace.BuildObject[PathClassObj, 'COMMON', GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
            self.BaseName = Inf.BaseName
            self.ModuleGuid = Inf.Guid
            self.ModuleType = Inf.ModuleType
            if Inf.Specification != None and 'PI_SPECIFICATION_VERSION' in Inf.Specification:
                self.PiSpecVersion = Inf.Specification['PI_SPECIFICATION_VERSION']
            self.VersionString = Inf.Version
            self.BinFileList = Inf.Binaries
            self.SourceFileList = Inf.Sources
            if self.BinFileList == []:
                EdkLogger.error("GenFds", GENFDS_ERROR,
                                "INF %s specified in FDF could not be found in build ARCH %s!" \
                                % (self.InfFileName, GenFdsGlobalVariable.ArchList))

        if self.OverrideGuid:
            self.ModuleGuid = self.OverrideGuid

        if len(self.SourceFileList) != 0 and not self.InDsc:
            EdkLogger.warn("GenFds", GENFDS_ERROR, "Module %s NOT found in DSC file; Is it really a binary module?" % (self.InfFileName))

        if self.ModuleType == 'SMM_CORE' and int(self.PiSpecVersion, 16) < 0x0001000A:
            EdkLogger.error("GenFds", FORMAT_NOT_SUPPORTED, "SMM_CORE module type can't be used in the module with PI_SPECIFICATION_VERSION less than 0x0001000A", File=self.InfFileName)      

        if Inf._Defs != None and len(Inf._Defs) > 0:
            self.OptRomDefs.update(Inf._Defs)

        self.PatchPcds = []
        InfPcds = Inf.Pcds
        Platform = GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, self.CurrentArch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
        FdfPcdDict = GenFdsGlobalVariable.FdfParser.Profile.PcdDict

        # Workaround here: both build and GenFds tool convert the workspace path to lower case
        # But INF file path in FDF and DSC file may have real case characters.
        # Try to convert the path to lower case to see if PCDs value are override by DSC.
        DscModules = {}
        for DscModule in Platform.Modules:
            DscModules[str(DscModule).lower()] = Platform.Modules[DscModule]
        for PcdKey in InfPcds:
            Pcd = InfPcds[PcdKey]
            if not hasattr(Pcd, 'Offset'):
                continue
            if Pcd.Type != 'PatchableInModule':
                continue
            # Override Patchable PCD value by the value from DSC
            PatchPcd = None
            InfLowerPath = str(PathClassObj).lower()
            if InfLowerPath in DscModules and PcdKey in DscModules[InfLowerPath].Pcds:
                PatchPcd = DscModules[InfLowerPath].Pcds[PcdKey]
            elif PcdKey in Platform.Pcds:
                PatchPcd = Platform.Pcds[PcdKey]
            DscOverride = False
            if PatchPcd and Pcd.Type == PatchPcd.Type:
                DefaultValue = PatchPcd.DefaultValue
                DscOverride = True

            # Override Patchable PCD value by the value from FDF
            FdfOverride = False
            if PcdKey in FdfPcdDict:
                DefaultValue = FdfPcdDict[PcdKey]
                FdfOverride = True

            if not DscOverride and not FdfOverride:
                continue
            # Check value, if value are equal, no need to patch
            if Pcd.DatumType == "VOID*":
                if Pcd.DefaultValue == DefaultValue or DefaultValue in [None, '']:
                    continue
                # Get the string size from FDF or DSC
                if DefaultValue[0] == 'L':
                    # Remove L"", but the '\0' must be appended
                    MaxDatumSize = str((len(DefaultValue) - 2) * 2)
                elif DefaultValue[0] == '{':
                    MaxDatumSize = str(len(DefaultValue.split(',')))
                else:
                    MaxDatumSize = str(len(DefaultValue) - 1)
                if DscOverride:
                    Pcd.MaxDatumSize = PatchPcd.MaxDatumSize
                # If no defined the maximum size in DSC, try to get current size from INF
                if Pcd.MaxDatumSize in ['', None]:
                    Pcd.MaxDatumSize = str(len(Pcd.DefaultValue.split(',')))
            else:
                Base1 = Base2 = 10
                if Pcd.DefaultValue.upper().startswith('0X'):
                    Base1 = 16
                if DefaultValue.upper().startswith('0X'):
                    Base2 = 16
                try:
                    PcdValueInImg = int(Pcd.DefaultValue, Base1)
                    PcdValueInDscOrFdf = int(DefaultValue, Base2)
                    if PcdValueInImg == PcdValueInDscOrFdf:
                        continue
                except:
                    continue
            # Check the Pcd size and data type
            if Pcd.DatumType == "VOID*":
                if int(MaxDatumSize) > int(Pcd.MaxDatumSize):
                    EdkLogger.error("GenFds", GENFDS_ERROR, "The size of VOID* type PCD '%s.%s' exceeds its maximum size %d bytes." \
                                    % (Pcd.TokenSpaceGuidCName, Pcd.TokenCName, int(MaxDatumSize) - int(Pcd.MaxDatumSize)))
            else:
                if PcdValueInDscOrFdf > FfsInfStatement._MAX_SIZE_TYPE[Pcd.DatumType] \
                    or PcdValueInImg > FfsInfStatement._MAX_SIZE_TYPE[Pcd.DatumType]:
                    EdkLogger.error("GenFds", GENFDS_ERROR, "The size of %s type PCD '%s.%s' doesn't match its data type." \
                                    % (Pcd.DatumType, Pcd.TokenSpaceGuidCName, Pcd.TokenCName))
            self.PatchPcds.append((Pcd, DefaultValue))

        self.InfModule = Inf
        self.PcdIsDriver = Inf.PcdIsDriver
        self.IsBinaryModule = Inf.IsBinaryModule
        GenFdsGlobalVariable.VerboseLogger("BaseName : %s" % self.BaseName)
        GenFdsGlobalVariable.VerboseLogger("ModuleGuid : %s" % self.ModuleGuid)
        GenFdsGlobalVariable.VerboseLogger("ModuleType : %s" % self.ModuleType)
        GenFdsGlobalVariable.VerboseLogger("VersionString : %s" % self.VersionString)
        GenFdsGlobalVariable.VerboseLogger("InfFileName :%s" % self.InfFileName)

        #
        # Set OutputPath = ${WorkSpace}\Build\Fv\Ffs\${ModuleGuid}+ ${MdouleName}\
        #

        self.OutputPath = os.path.join(GenFdsGlobalVariable.FfsDir, \
                                       self.ModuleGuid + self.BaseName)
        if not os.path.exists(self.OutputPath) :
            os.makedirs(self.OutputPath)

        self.EfiOutputPath = self.__GetEFIOutPutPath__()
        GenFdsGlobalVariable.VerboseLogger( "ModuelEFIPath: " + self.EfiOutputPath)

    ## PatchEfiFile
    #
    #  Patch EFI file with patch PCD
    #
    #  @param EfiFile: EFI file needs to be patched.
    #  @retval: Full path of patched EFI file: self.OutputPath + EfiFile base name
    #           If passed in file does not end with efi, return as is
    #
    def PatchEfiFile(self, EfiFile, FileType):
        if not self.PatchPcds:
            return EfiFile
        if FileType != 'PE32' and self.ModuleType != "USER_DEFINED":
            return EfiFile
        if self.PatchedBinFile:
            EdkLogger.error("GenFds", GENFDS_ERROR,
                            'Only one binary file can be patched:\n'
                            '  a binary file has been patched: %s\n'
                            '  current file: %s' % (self.PatchedBinFile, EfiFile),
                            File=self.InfFileName)
        Basename = os.path.basename(EfiFile)
        Output = os.path.join(self.OutputPath, Basename)
        CopyLongFilePath(EfiFile, Output)
        for Pcd, Value in self.PatchPcds:
            RetVal, RetStr = PatchBinaryFile(Output, int(Pcd.Offset, 0), Pcd.DatumType, Value, Pcd.MaxDatumSize)
            if RetVal:
                EdkLogger.error("GenFds", GENFDS_ERROR, RetStr, File=self.InfFileName)
        self.PatchedBinFile = os.path.normpath(EfiFile)
        return Output
    ## GenFfs() method
    #
    #   Generate FFS
    #
    #   @param  self         The object pointer
    #   @param  Dict         dictionary contains macro and value pair
    #   @param  FvChildAddr  Array of the inside FvImage base address
    #   @param  FvParentAddr Parent Fv base address
    #   @retval string       Generated FFS file name
    #
    def GenFfs(self, Dict = {}, FvChildAddr = [], FvParentAddr=None):
        #
        # Parse Inf file get Module related information
        #

        self.__InfParse__(Dict)
        SrcFile = os.path.join( GenFdsGlobalVariable.WorkSpaceDir , self.InfFileName);
        DestFile = os.path.join( self.OutputPath, self.ModuleGuid + '.ffs')
        
        SrcFileDir = "."
        SrcPath = os.path.dirname(SrcFile)
        SrcFileName = os.path.basename(SrcFile)
        SrcFileBase, SrcFileExt = os.path.splitext(SrcFileName)   
        DestPath = os.path.dirname(DestFile)
        DestFileName = os.path.basename(DestFile)
        DestFileBase, DestFileExt = os.path.splitext(DestFileName)   
        self.MacroDict = {
            # source file
            "${src}"      :   SrcFile,
            "${s_path}"   :   SrcPath,
            "${s_dir}"    :   SrcFileDir,
            "${s_name}"   :   SrcFileName,
            "${s_base}"   :   SrcFileBase,
            "${s_ext}"    :   SrcFileExt,
            # destination file
            "${dst}"      :   DestFile,
            "${d_path}"   :   DestPath,
            "${d_name}"   :   DestFileName,
            "${d_base}"   :   DestFileBase,
            "${d_ext}"    :   DestFileExt
        }
        #
        # Allow binary type module not specify override rule in FDF file.
        # 
        if len(self.BinFileList) > 0:
            if self.Rule == None or self.Rule == "":
                self.Rule = "BINARY"
                
        #
        # Get the rule of how to generate Ffs file
        #
        Rule = self.__GetRule__()
        GenFdsGlobalVariable.VerboseLogger( "Packing binaries from inf file : %s" %self.InfFileName)
        #
        # Convert Fv File Type for PI1.1 SMM driver.
        #
        if self.ModuleType == 'DXE_SMM_DRIVER' and int(self.PiSpecVersion, 16) >= 0x0001000A:
            if Rule.FvFileType == 'DRIVER':
                Rule.FvFileType = 'SMM'
        #
        # Framework SMM Driver has no SMM FV file type
        #
        if self.ModuleType == 'DXE_SMM_DRIVER' and int(self.PiSpecVersion, 16) < 0x0001000A:
            if Rule.FvFileType == 'SMM' or Rule.FvFileType == 'SMM_CORE':
                EdkLogger.error("GenFds", FORMAT_NOT_SUPPORTED, "Framework SMM module doesn't support SMM or SMM_CORE FV file type", File=self.InfFileName)
        #
        # For the rule only has simpleFile
        #
        if isinstance (Rule, RuleSimpleFile.RuleSimpleFile) :
            SectionOutputList = self.__GenSimpleFileSection__(Rule)
            FfsOutput = self.__GenSimpleFileFfs__(Rule, SectionOutputList)
            return FfsOutput
        #
        # For Rule has ComplexFile
        #
        elif isinstance(Rule, RuleComplexFile.RuleComplexFile):
            InputSectList, InputSectAlignments = self.__GenComplexFileSection__(Rule, FvChildAddr, FvParentAddr)
            FfsOutput = self.__GenComplexFileFfs__(Rule, InputSectList, InputSectAlignments)

            return FfsOutput

    ## __ExtendMacro__() method
    #
    #   Replace macro with its value
    #
    #   @param  self        The object pointer
    #   @param  String      The string to be replaced
    #   @retval string      Macro replaced string
    #
    def __ExtendMacro__ (self, String):
        MacroDict = {
            '$(INF_OUTPUT)'  : self.EfiOutputPath,
            '$(MODULE_NAME)' : self.BaseName,
            '$(BUILD_NUMBER)': self.BuildNum,
            '$(INF_VERSION)' : self.VersionString,
            '$(NAMED_GUID)'  : self.ModuleGuid
        }
        String = GenFdsGlobalVariable.MacroExtend(String, MacroDict)
        String = GenFdsGlobalVariable.MacroExtend(String, self.MacroDict)        
        return String

    ## __GetRule__() method
    #
    #   Get correct rule for generating FFS for this INF
    #
    #   @param  self        The object pointer
    #   @retval Rule        Rule object
    #
    def __GetRule__ (self) :
        CurrentArchList = []
        if self.CurrentArch == None:
            CurrentArchList = ['common']
        else:
            CurrentArchList.append(self.CurrentArch)

        for CurrentArch in CurrentArchList:
            RuleName = 'RULE'              + \
                       '.'                 + \
                       CurrentArch.upper() + \
                       '.'                 + \
                       self.ModuleType.upper()
            if self.Rule != None:
                RuleName = RuleName + \
                           '.'      + \
                           self.Rule.upper()

            Rule = GenFdsGlobalVariable.FdfParser.Profile.RuleDict.get(RuleName)
            if Rule != None:
                GenFdsGlobalVariable.VerboseLogger ("Want To Find Rule Name is : " + RuleName)
                return Rule

        RuleName = 'RULE'      + \
                   '.'         + \
                   'COMMON'    + \
                   '.'         + \
                   self.ModuleType.upper()

        if self.Rule != None:
            RuleName = RuleName + \
                       '.'      + \
                       self.Rule.upper()

        GenFdsGlobalVariable.VerboseLogger ('Trying to apply common rule %s for INF %s' % (RuleName, self.InfFileName))

        Rule = GenFdsGlobalVariable.FdfParser.Profile.RuleDict.get(RuleName)
        if Rule != None:
            GenFdsGlobalVariable.VerboseLogger ("Want To Find Rule Name is : " + RuleName)
            return Rule

        if Rule == None :
            EdkLogger.error("GenFds", GENFDS_ERROR, 'Don\'t Find common rule %s for INF %s' \
                            % (RuleName, self.InfFileName))

    ## __GetPlatformArchList__() method
    #
    #   Get Arch list this INF built under
    #
    #   @param  self        The object pointer
    #   @retval list        Arch list
    #
    def __GetPlatformArchList__(self):

        InfFileKey = os.path.normpath(os.path.join(GenFdsGlobalVariable.WorkSpaceDir, self.InfFileName))
        DscArchList = []
        PlatformDataBase = GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, 'IA32', GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
        if  PlatformDataBase != None:
            if InfFileKey in PlatformDataBase.Modules:
                DscArchList.append ('IA32')

        PlatformDataBase = GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, 'X64', GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
        if  PlatformDataBase != None:
            if InfFileKey in PlatformDataBase.Modules:
                DscArchList.append ('X64')

        PlatformDataBase = GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, 'IPF', GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
        if PlatformDataBase != None:
            if InfFileKey in (PlatformDataBase.Modules):
                DscArchList.append ('IPF')

        PlatformDataBase = GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, 'ARM', GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
        if PlatformDataBase != None:
            if InfFileKey in (PlatformDataBase.Modules):
                DscArchList.append ('ARM')

        PlatformDataBase = GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, 'EBC', GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
        if PlatformDataBase != None:
            if InfFileKey in (PlatformDataBase.Modules):
                DscArchList.append ('EBC')

        PlatformDataBase = GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, 'AARCH64', GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
        if PlatformDataBase != None:
            if InfFileKey in (PlatformDataBase.Modules):
                DscArchList.append ('AARCH64')

        return DscArchList

    ## GetCurrentArch() method
    #
    #   Get Arch list of the module from this INF is to be placed into flash
    #
    #   @param  self        The object pointer
    #   @retval list        Arch list
    #
    def GetCurrentArch(self) :

        TargetArchList = GenFdsGlobalVariable.ArchList

        PlatformArchList = self.__GetPlatformArchList__()

        CurArchList = TargetArchList
        if PlatformArchList != []:
            CurArchList = list(set (TargetArchList) & set (PlatformArchList))
        GenFdsGlobalVariable.VerboseLogger ("Valid target architecture(s) is : " + " ".join(CurArchList))

        ArchList = []
        if self.KeyStringList != []:
            for Key in self.KeyStringList:
                Key = GenFdsGlobalVariable.MacroExtend(Key)
                Target, Tag, Arch = Key.split('_')
                if Arch in CurArchList:
                    ArchList.append(Arch)
                if Target not in self.TargetOverrideList:
                    self.TargetOverrideList.append(Target)
        else:
            ArchList = CurArchList

        UseArchList = TargetArchList
        if self.UseArch != None:
            UseArchList = []
            UseArchList.append(self.UseArch)
            ArchList = list(set (UseArchList) & set (ArchList))

        self.InfFileName = NormPath(self.InfFileName)
        if len(PlatformArchList) == 0:
            self.InDsc = False
            PathClassObj = PathClass(self.InfFileName, GenFdsGlobalVariable.WorkSpaceDir)
            ErrorCode, ErrorInfo = PathClassObj.Validate(".inf")
            if ErrorCode != 0:
                EdkLogger.error("GenFds", ErrorCode, ExtraData=ErrorInfo)
        if len(ArchList) == 1:
            Arch = ArchList[0]
            return Arch
        elif len(ArchList) > 1:
            if len(PlatformArchList) == 0:
                EdkLogger.error("GenFds", GENFDS_ERROR, "GenFds command line option has multiple ARCHs %s. Not able to determine which ARCH is valid for Module %s !" % (str(ArchList), self.InfFileName))
            else:
                EdkLogger.error("GenFds", GENFDS_ERROR, "Module built under multiple ARCHs %s. Not able to determine which output to put into flash for Module %s !" % (str(ArchList), self.InfFileName))
        else:
            EdkLogger.error("GenFds", GENFDS_ERROR, "Module %s appears under ARCH %s in platform %s, but current deduced ARCH is %s, so NO build output could be put into flash." \
                            % (self.InfFileName, str(PlatformArchList), GenFdsGlobalVariable.ActivePlatform, str(set (UseArchList) & set (TargetArchList))))

    ## __GetEFIOutPutPath__() method
    #
    #   Get the output path for generated files
    #
    #   @param  self        The object pointer
    #   @retval string      Path that output files from this INF go to
    #
    def __GetEFIOutPutPath__(self):
        Arch = ''
        OutputPath = ''
        (ModulePath, FileName) = os.path.split(self.InfFileName)
        Index = FileName.rfind('.')
        FileName = FileName[0:Index]
        if self.OverrideGuid:
            FileName = self.OverrideGuid
        Arch = "NoneArch"
        if self.CurrentArch != None:
            Arch = self.CurrentArch

        OutputPath = os.path.join(GenFdsGlobalVariable.OutputDirDict[Arch],
                                  Arch ,
                                  ModulePath,
                                  FileName,
                                  'OUTPUT'
                                  )
        OutputPath = os.path.realpath(OutputPath)
        return OutputPath

    ## __GenSimpleFileSection__() method
    #
    #   Generate section by specified file name or a list of files with file extension
    #
    #   @param  self        The object pointer
    #   @param  Rule        The rule object used to generate section
    #   @retval string      File name of the generated section file
    #
    def __GenSimpleFileSection__(self, Rule):
        #
        # Prepare the parameter of GenSection
        #
        FileList = []
        OutputFileList = []
        GenSecInputFile = None
        if Rule.FileName != None:
            GenSecInputFile = self.__ExtendMacro__(Rule.FileName)
            if os.path.isabs(GenSecInputFile):
                GenSecInputFile = os.path.normpath(GenSecInputFile)
            else:
                GenSecInputFile = os.path.normpath(os.path.join(self.EfiOutputPath, GenSecInputFile))
        else:
            FileList, IsSect = Section.Section.GetFileList(self, '', Rule.FileExtension)

        Index = 1
        SectionType = Rule.SectionType
        #
        # Convert Fv Section Type for PI1.1 SMM driver.
        #
        if self.ModuleType == 'DXE_SMM_DRIVER' and int(self.PiSpecVersion, 16) >= 0x0001000A:
            if SectionType == 'DXE_DEPEX':
                SectionType = 'SMM_DEPEX'
        #
        # Framework SMM Driver has no SMM_DEPEX section type
        #
        if self.ModuleType == 'DXE_SMM_DRIVER' and int(self.PiSpecVersion, 16) < 0x0001000A:
            if SectionType == 'SMM_DEPEX':
                EdkLogger.error("GenFds", FORMAT_NOT_SUPPORTED, "Framework SMM module doesn't support SMM_DEPEX section type", File=self.InfFileName)
        NoStrip = True
        if self.ModuleType in ('SEC', 'PEI_CORE', 'PEIM'):
            if self.KeepReloc != None:
                NoStrip = self.KeepReloc
            elif Rule.KeepReloc != None:
                NoStrip = Rule.KeepReloc
            elif self.ShadowFromInfFile != None:
                NoStrip = self.ShadowFromInfFile

        if FileList != [] :
            for File in FileList:

                SecNum = '%d' %Index
                GenSecOutputFile= self.__ExtendMacro__(Rule.NameGuid) + \
                              Ffs.Ffs.SectionSuffix[SectionType] + 'SEC' + SecNum
                Index = Index + 1
                OutputFile = os.path.join(self.OutputPath, GenSecOutputFile)
                File = GenFdsGlobalVariable.MacroExtend(File, Dict, self.CurrentArch)

                #Get PE Section alignment when align is set to AUTO
                if self.Alignment == 'Auto' and (SectionType == 'PE32' or SectionType == 'TE'):
                    ImageObj = PeImageClass (File)
                    if ImageObj.SectionAlignment < 0x400:
                        self.Alignment = str (ImageObj.SectionAlignment)
                    else:
                        self.Alignment = str (ImageObj.SectionAlignment / 0x400) + 'K'

                if not NoStrip:
                    FileBeforeStrip = os.path.join(self.OutputPath, ModuleName + '.reloc')
                    if not os.path.exists(FileBeforeStrip) or \
                           (os.path.getmtime(File) > os.path.getmtime(FileBeforeStrip)):
                        CopyLongFilePath(File, FileBeforeStrip)
                    StrippedFile = os.path.join(self.OutputPath, ModuleName + '.stipped')
                    GenFdsGlobalVariable.GenerateFirmwareImage(
                                            StrippedFile,
                                            [File],
                                            Strip=True
                                            )
                    File = StrippedFile

                if SectionType == 'TE':
                    TeFile = os.path.join( self.OutputPath, self.ModuleGuid + 'Te.raw')
                    GenFdsGlobalVariable.GenerateFirmwareImage(
                                            TeFile,
                                            [File],
                                            Type='te'
                                            )
                    File = TeFile

                GenFdsGlobalVariable.GenerateSection(OutputFile, [File], Section.Section.SectionType[SectionType])
                OutputFileList.append(OutputFile)
        else:
            SecNum = '%d' %Index
            GenSecOutputFile= self.__ExtendMacro__(Rule.NameGuid) + \
                              Ffs.Ffs.SectionSuffix[SectionType] + 'SEC' + SecNum
            OutputFile = os.path.join(self.OutputPath, GenSecOutputFile)
            GenSecInputFile = GenFdsGlobalVariable.MacroExtend(GenSecInputFile, Dict, self.CurrentArch)

            #Get PE Section alignment when align is set to AUTO
            if self.Alignment == 'Auto' and (SectionType == 'PE32' or SectionType == 'TE'):
                ImageObj = PeImageClass (GenSecInputFile)
                if ImageObj.SectionAlignment < 0x400:
                    self.Alignment = str (ImageObj.SectionAlignment)
                else:
                    self.Alignment = str (ImageObj.SectionAlignment / 0x400) + 'K'

            if not NoStrip:
                FileBeforeStrip = os.path.join(self.OutputPath, ModuleName + '.reloc')
                if not os.path.exists(FileBeforeStrip) or \
                       (os.path.getmtime(GenSecInputFile) > os.path.getmtime(FileBeforeStrip)):
                    CopyLongFilePath(GenSecInputFile, FileBeforeStrip)

                StrippedFile = os.path.join(self.OutputPath, ModuleName + '.stipped')
                GenFdsGlobalVariable.GenerateFirmwareImage(
                                        StrippedFile,
                                        [GenSecInputFile],
                                        Strip=True
                                        )
                GenSecInputFile = StrippedFile

            if SectionType == 'TE':
                TeFile = os.path.join( self.OutputPath, self.ModuleGuid + 'Te.raw')
                GenFdsGlobalVariable.GenerateFirmwareImage(
                                        TeFile,
                                        [GenSecInputFile],
                                        Type='te'
                                        )
                GenSecInputFile = TeFile

            GenFdsGlobalVariable.GenerateSection(OutputFile, [GenSecInputFile], Section.Section.SectionType[SectionType])
            OutputFileList.append(OutputFile)

        return OutputFileList

    ## __GenSimpleFileFfs__() method
    #
    #   Generate FFS
    #
    #   @param  self        The object pointer
    #   @param  Rule        The rule object used to generate section
    #   @param  InputFileList        The output file list from GenSection
    #   @retval string      Generated FFS file name
    #
    def __GenSimpleFileFfs__(self, Rule, InputFileList):
        FfsOutput = self.OutputPath                     + \
                    os.sep                              + \
                    self.__ExtendMacro__(Rule.NameGuid) + \
                    '.ffs'

        GenFdsGlobalVariable.VerboseLogger(self.__ExtendMacro__(Rule.NameGuid))
        InputSection = []
        SectionAlignments = []
        for InputFile in InputFileList:
            InputSection.append(InputFile)
            SectionAlignments.append(Rule.SectAlignment)

        if Rule.NameGuid != None and Rule.NameGuid.startswith('PCD('):
            PcdValue = GenFdsGlobalVariable.GetPcdValue(Rule.NameGuid)
            if len(PcdValue) == 0:
                EdkLogger.error("GenFds", GENFDS_ERROR, '%s NOT defined.' \
                            % (Rule.NameGuid))
            if PcdValue.startswith('{'):
                PcdValue = GuidStructureByteArrayToGuidString(PcdValue)
            RegistryGuidStr = PcdValue
            if len(RegistryGuidStr) == 0:
                EdkLogger.error("GenFds", GENFDS_ERROR, 'GUID value for %s in wrong format.' \
                            % (Rule.NameGuid))
            self.ModuleGuid = RegistryGuidStr

        GenFdsGlobalVariable.GenerateFfs(FfsOutput, InputSection,
                                         Ffs.Ffs.FdfFvFileTypeToFileType[Rule.FvFileType],
                                         self.ModuleGuid, Fixed=Rule.Fixed,
                                         CheckSum=Rule.CheckSum, Align=Rule.Alignment,
                                         SectionAlign=SectionAlignments
                                        )
        return FfsOutput

    ## __GenComplexFileSection__() method
    #
    #   Generate section by sections in Rule
    #
    #   @param  self         The object pointer
    #   @param  Rule         The rule object used to generate section
    #   @param  FvChildAddr  Array of the inside FvImage base address
    #   @param  FvParentAddr Parent Fv base address
    #   @retval string       File name of the generated section file
    #
    def __GenComplexFileSection__(self, Rule, FvChildAddr, FvParentAddr):
        if self.ModuleType in ('SEC', 'PEI_CORE', 'PEIM'):
            if Rule.KeepReloc != None:
                self.KeepRelocFromRule = Rule.KeepReloc
        SectFiles = []
        SectAlignments = []
        Index = 1
        HasGneratedFlag = False
        if self.PcdIsDriver == 'PEI_PCD_DRIVER':
            if self.IsBinaryModule:
                PcdExDbFileName = os.path.join(GenFdsGlobalVariable.FvDir, "PEIPcdDataBase.raw")
            else:
                PcdExDbFileName = os.path.join(self.EfiOutputPath, "PEIPcdDataBase.raw")
            PcdExDbSecName = os.path.join(self.OutputPath, "PEIPcdDataBaseSec.raw")
            GenFdsGlobalVariable.GenerateSection(PcdExDbSecName,
                                                 [PcdExDbFileName],
                                                 "EFI_SECTION_RAW",
                                                 )
            SectFiles.append(PcdExDbSecName)
            SectAlignments.append(None)
        elif self.PcdIsDriver == 'DXE_PCD_DRIVER':
            if self.IsBinaryModule:
                PcdExDbFileName = os.path.join(GenFdsGlobalVariable.FvDir, "DXEPcdDataBase.raw")
            else:
                PcdExDbFileName = os.path.join(self.EfiOutputPath, "DXEPcdDataBase.raw")
            PcdExDbSecName = os.path.join(self.OutputPath, "DXEPcdDataBaseSec.raw")
            GenFdsGlobalVariable.GenerateSection(PcdExDbSecName,
                                                 [PcdExDbFileName],
                                                 "EFI_SECTION_RAW",
                                                 )
            SectFiles.append(PcdExDbSecName)
            SectAlignments.append(None)
        for Sect in Rule.SectionList:
            SecIndex = '%d' %Index
            SectList  = []
            #
            # Convert Fv Section Type for PI1.1 SMM driver.
            #
            if self.ModuleType == 'DXE_SMM_DRIVER' and int(self.PiSpecVersion, 16) >= 0x0001000A:
                if Sect.SectionType == 'DXE_DEPEX':
                    Sect.SectionType = 'SMM_DEPEX'
            #
            # Framework SMM Driver has no SMM_DEPEX section type
            #
            if self.ModuleType == 'DXE_SMM_DRIVER' and int(self.PiSpecVersion, 16) < 0x0001000A:
                if Sect.SectionType == 'SMM_DEPEX':
                    EdkLogger.error("GenFds", FORMAT_NOT_SUPPORTED, "Framework SMM module doesn't support SMM_DEPEX section type", File=self.InfFileName)
            #
            # process the inside FvImage from FvSection or GuidSection
            #
            if FvChildAddr != []:
                if isinstance(Sect, FvImageSection):
                    Sect.FvAddr = FvChildAddr.pop(0)
                elif isinstance(Sect, GuidSection):
                    Sect.FvAddr = FvChildAddr
            if FvParentAddr != None and isinstance(Sect, GuidSection):
                Sect.FvParentAddr = FvParentAddr
            
            if Rule.KeyStringList != []:
                SectList, Align = Sect.GenSection(self.OutputPath , self.ModuleGuid, SecIndex, Rule.KeyStringList, self)
            else :
                SectList, Align = Sect.GenSection(self.OutputPath , self.ModuleGuid, SecIndex, self.KeyStringList, self)
            
            if not HasGneratedFlag:
                UniVfrOffsetFileSection = ""    
                ModuleFileName = os.path.join(GenFdsGlobalVariable.WorkSpaceDir, self.InfFileName)
                InfData = GenFdsGlobalVariable.WorkSpace.BuildObject[PathClass(ModuleFileName), self.CurrentArch]
                #
                # Search the source list in InfData to find if there are .vfr file exist.
                #
                VfrUniBaseName = {}
                VfrUniOffsetList = []
                for SourceFile in InfData.Sources:
                    if SourceFile.Type.upper() == ".VFR" :
                        #
                        # search the .map file to find the offset of vfr binary in the PE32+/TE file. 
                        #
                        VfrUniBaseName[SourceFile.BaseName] = (SourceFile.BaseName + "Bin")
                    if SourceFile.Type.upper() == ".UNI" :
                        #
                        # search the .map file to find the offset of Uni strings binary in the PE32+/TE file. 
                        #
                        VfrUniBaseName["UniOffsetName"] = (self.BaseName + "Strings")
                    
                
                if len(VfrUniBaseName) > 0:
                    VfrUniOffsetList = self.__GetBuildOutputMapFileVfrUniInfo(VfrUniBaseName)
                    #
                    # Generate the Raw data of raw section
                    #
                    os.path.join( self.OutputPath, self.BaseName + '.offset')
                    UniVfrOffsetFileName    =  os.path.join( self.OutputPath, self.BaseName + '.offset')
                    UniVfrOffsetFileSection =  os.path.join( self.OutputPath, self.BaseName + 'Offset' + '.raw')
                    
                    self.__GenUniVfrOffsetFile (VfrUniOffsetList, UniVfrOffsetFileName)
                    
                    UniVfrOffsetFileNameList = []
                    UniVfrOffsetFileNameList.append(UniVfrOffsetFileName)
                    """Call GenSection"""
                    GenFdsGlobalVariable.GenerateSection(UniVfrOffsetFileSection,
                                                         UniVfrOffsetFileNameList,
                                                         "EFI_SECTION_RAW"
                                                         )
                    os.remove(UniVfrOffsetFileName)         
                    SectList.append(UniVfrOffsetFileSection)
                    HasGneratedFlag = True
                
            for SecName in  SectList :
                SectFiles.append(SecName)
                SectAlignments.append(Align)
            Index = Index + 1
        return SectFiles, SectAlignments

    ## __GenComplexFileFfs__() method
    #
    #   Generate FFS
    #
    #   @param  self        The object pointer
    #   @param  Rule        The rule object used to generate section
    #   @param  InputFileList        The output file list from GenSection
    #   @retval string      Generated FFS file name
    #
    def __GenComplexFileFfs__(self, Rule, InputFile, Alignments):

        if Rule.NameGuid != None and Rule.NameGuid.startswith('PCD('):
            PcdValue = GenFdsGlobalVariable.GetPcdValue(Rule.NameGuid)
            if len(PcdValue) == 0:
                EdkLogger.error("GenFds", GENFDS_ERROR, '%s NOT defined.' \
                            % (Rule.NameGuid))
            if PcdValue.startswith('{'):
                PcdValue = GuidStructureByteArrayToGuidString(PcdValue)
            RegistryGuidStr = PcdValue
            if len(RegistryGuidStr) == 0:
                EdkLogger.error("GenFds", GENFDS_ERROR, 'GUID value for %s in wrong format.' \
                            % (Rule.NameGuid))
            self.ModuleGuid = RegistryGuidStr

        FfsOutput = os.path.join( self.OutputPath, self.ModuleGuid + '.ffs')
        GenFdsGlobalVariable.GenerateFfs(FfsOutput, InputFile,
                                         Ffs.Ffs.FdfFvFileTypeToFileType[Rule.FvFileType],
                                         self.ModuleGuid, Fixed=Rule.Fixed,
                                         CheckSum=Rule.CheckSum, Align=Rule.Alignment,
                                         SectionAlign=Alignments
                                        )
        return FfsOutput

    ## __GetGenFfsCmdParameter__() method
    #
    #   Create parameter string for GenFfs
    #
    #   @param  self        The object pointer
    #   @param  Rule        The rule object used to generate section
    #   @retval tuple       (FileType, Fixed, CheckSum, Alignment)
    #
    def __GetGenFfsCmdParameter__(self, Rule):
        result = tuple()
        result += ('-t', Ffs.Ffs.FdfFvFileTypeToFileType[Rule.FvFileType])
        if Rule.Fixed != False:
            result += ('-x',)
        if Rule.CheckSum != False:
            result += ('-s',)

        if Rule.Alignment != None and Rule.Alignment != '':
            result += ('-a', Rule.Alignment)

        return result
 
    ## __GetBuildOutputMapFileVfrUniInfo() method
    #
    #   Find the offset of UNI/INF object offset in the EFI image file.
    #
    #   @param  self                  The object pointer
    #   @param  VfrUniBaseName        A name list contain the UNI/INF object name.
    #   @retval RetValue              A list contain offset of UNI/INF object.
    #    
    def __GetBuildOutputMapFileVfrUniInfo(self, VfrUniBaseName):
        MapFileName = os.path.join(self.EfiOutputPath, self.BaseName + ".map")
        EfiFileName = os.path.join(self.EfiOutputPath, self.BaseName + ".efi")
        return GetVariableOffset(MapFileName, EfiFileName, VfrUniBaseName.values())
    
    ## __GenUniVfrOffsetFile() method
    #
    #   Generate the offset file for the module which contain VFR or UNI file.
    #
    #   @param  self                    The object pointer
    #   @param  VfrUniOffsetList        A list contain the VFR/UNI offsets in the EFI image file.
    #   @param  UniVfrOffsetFileName    The output offset file name.
    #
    def __GenUniVfrOffsetFile(self, VfrUniOffsetList, UniVfrOffsetFileName):
        
        try:
            fInputfile = open(UniVfrOffsetFileName, "wb+", 0)
        except:
            EdkLogger.error("GenFds", FILE_OPEN_FAILURE, "File open failed for %s" %UniVfrOffsetFileName,None)
            
        # Use a instance of StringIO to cache data
        fStringIO = StringIO.StringIO('')  
        
        for Item in VfrUniOffsetList:
            if (Item[0].find("Strings") != -1):
                #
                # UNI offset in image.
                # GUID + Offset
                # { 0x8913c5e0, 0x33f6, 0x4d86, { 0x9b, 0xf1, 0x43, 0xef, 0x89, 0xfc, 0x6, 0x66 } }
                #
                UniGuid = [0xe0, 0xc5, 0x13, 0x89, 0xf6, 0x33, 0x86, 0x4d, 0x9b, 0xf1, 0x43, 0xef, 0x89, 0xfc, 0x6, 0x66]
                UniGuid = [chr(ItemGuid) for ItemGuid in UniGuid]
                fStringIO.write(''.join(UniGuid))            
                UniValue = pack ('Q', int (Item[1], 16))
                fStringIO.write (UniValue)
            else:
                #
                # VFR binary offset in image.
                # GUID + Offset
                # { 0xd0bc7cb4, 0x6a47, 0x495f, { 0xaa, 0x11, 0x71, 0x7, 0x46, 0xda, 0x6, 0xa2 } };
                #
                VfrGuid = [0xb4, 0x7c, 0xbc, 0xd0, 0x47, 0x6a, 0x5f, 0x49, 0xaa, 0x11, 0x71, 0x7, 0x46, 0xda, 0x6, 0xa2]
                VfrGuid = [chr(ItemGuid) for ItemGuid in VfrGuid]
                fStringIO.write(''.join(VfrGuid))                   
                type (Item[1]) 
                VfrValue = pack ('Q', int (Item[1], 16))
                fStringIO.write (VfrValue)
            
        #
        # write data into file.
        #
        try :  
            fInputfile.write (fStringIO.getvalue())
        except:
            EdkLogger.error("GenFds", FILE_WRITE_FAILURE, "Write data to file %s failed, please check whether the file been locked or using by other applications." %UniVfrOffsetFileName,None)
        
        fStringIO.close ()
        fInputfile.close ()
        
                
                    
            
            
        
                                
        
