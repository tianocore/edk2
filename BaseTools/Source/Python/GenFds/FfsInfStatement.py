## @file
# process FFS generation from INF statement
#
#  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
#  Copyright (c) 2014-2016 Hewlett-Packard Development Company, L.P.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
from . import Rule
import Common.LongFilePathOs as os
from io import BytesIO
from struct import *
from .GenFdsGlobalVariable import GenFdsGlobalVariable
from .Ffs import SectionSuffix,FdfFvFileTypeToFileType
import subprocess
import sys
from . import Section
from . import RuleSimpleFile
from . import RuleComplexFile
from CommonDataClass.FdfClass import FfsInfStatementClassObject
from Common.MultipleWorkspace import MultipleWorkspace as mws
from Common.DataType import SUP_MODULE_USER_DEFINED
from Common.StringUtils import *
from Common.Misc import PathClass
from Common.Misc import GuidStructureByteArrayToGuidString
from Common.Misc import ProcessDuplicatedInf
from Common.Misc import GetVariableOffset
from Common import EdkLogger
from Common.BuildToolError import *
from .GuidSection import GuidSection
from .FvImageSection import FvImageSection
from Common.Misc import PeImageClass
from AutoGen.GenDepex import DependencyExpression
from PatchPcdValue.PatchPcdValue import PatchBinaryFile
from Common.LongFilePathSupport import CopyLongFilePath
from Common.LongFilePathSupport import OpenLongFilePath as open
import Common.GlobalData as GlobalData
from .DepexSection import DepexSection
from Common.Misc import SaveFileOnChange
from Common.Expression import *
from Common.DataType import *

## generate FFS from INF
#
#
class FfsInfStatement(FfsInfStatementClassObject):
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
        self.Depex = False

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
            if '.depex' not in self.FinalTargetSuffixMap and self.InfModule.ModuleType != SUP_MODULE_USER_DEFINED \
                and not self.InfModule.DxsFile and not self.InfModule.LibraryClass:
                ModuleType = self.InfModule.ModuleType
                PlatformDataBase = GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, self.CurrentArch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]

                if ModuleType != SUP_MODULE_USER_DEFINED:
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

        #
        # Cache lower case version of INF path before processing FILE_GUID override
        #
        InfLowerPath = str(PathClassObj).lower()
        if self.OverrideGuid:
            PathClassObj = ProcessDuplicatedInf(PathClassObj, self.OverrideGuid, GenFdsGlobalVariable.WorkSpaceDir)
        if self.CurrentArch is not None:

            Inf = GenFdsGlobalVariable.WorkSpace.BuildObject[PathClassObj, self.CurrentArch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
            #
            # Set Ffs BaseName, ModuleGuid, ModuleType, Version, OutputPath
            #
            self.BaseName = Inf.BaseName
            self.ModuleGuid = Inf.Guid
            self.ModuleType = Inf.ModuleType
            if Inf.Specification is not None and 'PI_SPECIFICATION_VERSION' in Inf.Specification:
                self.PiSpecVersion = Inf.Specification['PI_SPECIFICATION_VERSION']
            if Inf.AutoGenVersion < 0x00010005:
                self.ModuleType = Inf.ComponentType
            self.VersionString = Inf.Version
            self.BinFileList = Inf.Binaries
            self.SourceFileList = Inf.Sources
            if self.KeepReloc is None and Inf.Shadow:
                self.ShadowFromInfFile = Inf.Shadow

        else:
            Inf = GenFdsGlobalVariable.WorkSpace.BuildObject[PathClassObj, TAB_COMMON, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
            self.BaseName = Inf.BaseName
            self.ModuleGuid = Inf.Guid
            self.ModuleType = Inf.ModuleType
            if Inf.Specification is not None and 'PI_SPECIFICATION_VERSION' in Inf.Specification:
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

        if self.ModuleType == SUP_MODULE_SMM_CORE and int(self.PiSpecVersion, 16) < 0x0001000A:
            EdkLogger.error("GenFds", FORMAT_NOT_SUPPORTED, "SMM_CORE module type can't be used in the module with PI_SPECIFICATION_VERSION less than 0x0001000A", File=self.InfFileName)

        if self.ModuleType == SUP_MODULE_MM_CORE_STANDALONE and int(self.PiSpecVersion, 16) < 0x00010032:
            EdkLogger.error("GenFds", FORMAT_NOT_SUPPORTED, "MM_CORE_STANDALONE module type can't be used in the module with PI_SPECIFICATION_VERSION less than 0x00010032", File=self.InfFileName)

        if Inf._Defs is not None and len(Inf._Defs) > 0:
            self.OptRomDefs.update(Inf._Defs)

        self.PatchPcds = []
        InfPcds = Inf.Pcds
        Platform = GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, self.CurrentArch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
        FdfPcdDict = GenFdsGlobalVariable.FdfParser.Profile.PcdDict
        PlatformPcds = Platform.Pcds

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
            if Pcd.Type != TAB_PCDS_PATCHABLE_IN_MODULE:
                continue
            # Override Patchable PCD value by the value from DSC
            PatchPcd = None
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

            # Override Patchable PCD value by the value from Build Option
            BuildOptionOverride = False
            if GlobalData.BuildOptionPcd:
                for pcd in GlobalData.BuildOptionPcd:
                    if PcdKey == (pcd[1], pcd[0]):
                        if pcd[2]:
                            continue
                        DefaultValue = pcd[3]
                        BuildOptionOverride = True
                        break

            if not DscOverride and not FdfOverride and not BuildOptionOverride:
                continue

            # Support Flexible PCD format
            if DefaultValue:
                try:
                    DefaultValue = ValueExpressionEx(DefaultValue, Pcd.DatumType, Platform._GuidDict)(True)
                except BadExpression:
                    EdkLogger.error("GenFds", GENFDS_ERROR, 'PCD [%s.%s] Value "%s"' %(Pcd.TokenSpaceGuidCName, Pcd.TokenCName, DefaultValue), File=self.InfFileName)

            if Pcd.InfDefaultValue:
                try:
                    Pcd.InfDefaultValue = ValueExpressionEx(Pcd.InfDefaultValue, Pcd.DatumType, Platform._GuidDict)(True)
                except BadExpression:
                    EdkLogger.error("GenFds", GENFDS_ERROR, 'PCD [%s.%s] Value "%s"' %(Pcd.TokenSpaceGuidCName, Pcd.TokenCName, Pcd.DefaultValue), File=self.InfFileName)

            # Check value, if value are equal, no need to patch
            if Pcd.DatumType == TAB_VOID:
                if Pcd.InfDefaultValue == DefaultValue or not DefaultValue:
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
                if not Pcd.MaxDatumSize:
                    Pcd.MaxDatumSize = str(len(Pcd.InfDefaultValue.split(',')))
            else:
                Base1 = Base2 = 10
                if Pcd.InfDefaultValue.upper().startswith('0X'):
                    Base1 = 16
                if DefaultValue.upper().startswith('0X'):
                    Base2 = 16
                try:
                    PcdValueInImg = int(Pcd.InfDefaultValue, Base1)
                    PcdValueInDscOrFdf = int(DefaultValue, Base2)
                    if PcdValueInImg == PcdValueInDscOrFdf:
                        continue
                except:
                    continue
            # Check the Pcd size and data type
            if Pcd.DatumType == TAB_VOID:
                if int(MaxDatumSize) > int(Pcd.MaxDatumSize):
                    EdkLogger.error("GenFds", GENFDS_ERROR, "The size of VOID* type PCD '%s.%s' exceeds its maximum size %d bytes." \
                                    % (Pcd.TokenSpaceGuidCName, Pcd.TokenCName, int(MaxDatumSize) - int(Pcd.MaxDatumSize)))
            else:
                if PcdValueInDscOrFdf > MAX_VAL_TYPE[Pcd.DatumType] \
                    or PcdValueInImg > MAX_VAL_TYPE[Pcd.DatumType]:
                    EdkLogger.error("GenFds", GENFDS_ERROR, "The size of %s type PCD '%s.%s' doesn't match its data type." \
                                    % (Pcd.DatumType, Pcd.TokenSpaceGuidCName, Pcd.TokenCName))
            self.PatchPcds.append((Pcd, DefaultValue))

        self.InfModule = Inf
        self.PcdIsDriver = Inf.PcdIsDriver
        self.IsBinaryModule = Inf.IsBinaryModule
        if len(Inf.Depex.data) > 0 and len(Inf.DepexExpression.data) > 0:
            self.Depex = True

        GenFdsGlobalVariable.VerboseLogger("BaseName : %s" % self.BaseName)
        GenFdsGlobalVariable.VerboseLogger("ModuleGuid : %s" % self.ModuleGuid)
        GenFdsGlobalVariable.VerboseLogger("ModuleType : %s" % self.ModuleType)
        GenFdsGlobalVariable.VerboseLogger("VersionString : %s" % self.VersionString)
        GenFdsGlobalVariable.VerboseLogger("InfFileName :%s" % self.InfFileName)

        #
        # Set OutputPath = ${WorkSpace}\Build\Fv\Ffs\${ModuleGuid}+ ${ModuleName}\
        #

        self.OutputPath = os.path.join(GenFdsGlobalVariable.FfsDir, \
                                       self.ModuleGuid + self.BaseName)
        if not os.path.exists(self.OutputPath) :
            os.makedirs(self.OutputPath)

        self.EfiOutputPath, self.EfiDebugPath = self.__GetEFIOutPutPath__()
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
        #
        # If the module does not have any patches, then return path to input file
        #
        if not self.PatchPcds:
            return EfiFile

        #
        # Only patch file if FileType is PE32 or ModuleType is USER_DEFINED
        #
        if FileType != BINARY_FILE_TYPE_PE32 and self.ModuleType != SUP_MODULE_USER_DEFINED:
            return EfiFile

        #
        # Generate path to patched output file
        #
        Basename = os.path.basename(EfiFile)
        Output = os.path.normpath (os.path.join(self.OutputPath, Basename))

        #
        # If this file has already been patched, then return the path to the patched file
        #
        if self.PatchedBinFile == Output:
          return Output

        #
        # If a different file from the same module has already been patched, then generate an error
        #
        if self.PatchedBinFile:
            EdkLogger.error("GenFds", GENFDS_ERROR,
                            'Only one binary file can be patched:\n'
                            '  a binary file has been patched: %s\n'
                            '  current file: %s' % (self.PatchedBinFile, EfiFile),
                            File=self.InfFileName)

        #
        # Copy unpatched file contents to output file location to perform patching
        #
        CopyLongFilePath(EfiFile, Output)

        #
        # Apply patches to patched output file
        #
        for Pcd, Value in self.PatchPcds:
            RetVal, RetStr = PatchBinaryFile(Output, int(Pcd.Offset, 0), Pcd.DatumType, Value, Pcd.MaxDatumSize)
            if RetVal:
                EdkLogger.error("GenFds", GENFDS_ERROR, RetStr, File=self.InfFileName)

        #
        # Save the path of the patched output file
        #
        self.PatchedBinFile = Output

        #
        # Return path to patched output file
        #
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
    def GenFfs(self, Dict = {}, FvChildAddr = [], FvParentAddr=None, IsMakefile=False, FvName=None):
        #
        # Parse Inf file get Module related information
        #

        self.__InfParse__(Dict)
        Arch = self.GetCurrentArch()
        SrcFile = mws.join( GenFdsGlobalVariable.WorkSpaceDir, self.InfFileName);
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
            if self.Rule is None or self.Rule == "":
                self.Rule = "BINARY"

        if not IsMakefile and GenFdsGlobalVariable.EnableGenfdsMultiThread and self.Rule != 'BINARY':
            IsMakefile = True
        #
        # Get the rule of how to generate Ffs file
        #
        Rule = self.__GetRule__()
        GenFdsGlobalVariable.VerboseLogger( "Packing binaries from inf file : %s" %self.InfFileName)
        #
        # Convert Fv File Type for PI1.1 SMM driver.
        #
        if self.ModuleType == SUP_MODULE_DXE_SMM_DRIVER and int(self.PiSpecVersion, 16) >= 0x0001000A:
            if Rule.FvFileType == 'DRIVER':
                Rule.FvFileType = 'SMM'
        #
        # Framework SMM Driver has no SMM FV file type
        #
        if self.ModuleType == SUP_MODULE_DXE_SMM_DRIVER and int(self.PiSpecVersion, 16) < 0x0001000A:
            if Rule.FvFileType == 'SMM' or Rule.FvFileType == SUP_MODULE_SMM_CORE:
                EdkLogger.error("GenFds", FORMAT_NOT_SUPPORTED, "Framework SMM module doesn't support SMM or SMM_CORE FV file type", File=self.InfFileName)
        #
        # For the rule only has simpleFile
        #
        MakefilePath = None
        if self.IsBinaryModule:
            IsMakefile = False
        if IsMakefile:
            MakefilePath = self.InfFileName, Arch
        if isinstance (Rule, RuleSimpleFile.RuleSimpleFile):
            SectionOutputList = self.__GenSimpleFileSection__(Rule, IsMakefile=IsMakefile)
            FfsOutput = self.__GenSimpleFileFfs__(Rule, SectionOutputList, MakefilePath=MakefilePath)
            return FfsOutput
        #
        # For Rule has ComplexFile
        #
        elif isinstance(Rule, RuleComplexFile.RuleComplexFile):
            InputSectList, InputSectAlignments = self.__GenComplexFileSection__(Rule, FvChildAddr, FvParentAddr, IsMakefile=IsMakefile)
            FfsOutput = self.__GenComplexFileFfs__(Rule, InputSectList, InputSectAlignments, MakefilePath=MakefilePath)
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
        if self.CurrentArch is None:
            CurrentArchList = ['common']
        else:
            CurrentArchList.append(self.CurrentArch)

        for CurrentArch in CurrentArchList:
            RuleName = 'RULE'              + \
                       '.'                 + \
                       CurrentArch.upper() + \
                       '.'                 + \
                       self.ModuleType.upper()
            if self.Rule is not None:
                RuleName = RuleName + \
                           '.'      + \
                           self.Rule.upper()

            Rule = GenFdsGlobalVariable.FdfParser.Profile.RuleDict.get(RuleName)
            if Rule is not None:
                GenFdsGlobalVariable.VerboseLogger ("Want To Find Rule Name is : " + RuleName)
                return Rule

        RuleName = 'RULE'      + \
                   '.'         + \
                   TAB_COMMON    + \
                   '.'         + \
                   self.ModuleType.upper()

        if self.Rule is not None:
            RuleName = RuleName + \
                       '.'      + \
                       self.Rule.upper()

        GenFdsGlobalVariable.VerboseLogger ('Trying to apply common rule %s for INF %s' % (RuleName, self.InfFileName))

        Rule = GenFdsGlobalVariable.FdfParser.Profile.RuleDict.get(RuleName)
        if Rule is not None:
            GenFdsGlobalVariable.VerboseLogger ("Want To Find Rule Name is : " + RuleName)
            return Rule

        if Rule is None :
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

        InfFileKey = os.path.normpath(mws.join(GenFdsGlobalVariable.WorkSpaceDir, self.InfFileName))
        DscArchList = []
        for Arch in GenFdsGlobalVariable.ArchList :
            PlatformDataBase = GenFdsGlobalVariable.WorkSpace.BuildObject[GenFdsGlobalVariable.ActivePlatform, Arch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
            if  PlatformDataBase is not None:
                if InfFileKey in PlatformDataBase.Modules:
                    DscArchList.append (Arch)
                else:
                    #
                    # BaseTools support build same module more than once, the module path with FILE_GUID overridden has
                    # the file name FILE_GUIDmodule.inf, then PlatformDataBase.Modules use FILE_GUIDmodule.inf as key,
                    # but the path (self.MetaFile.Path) is the real path
                    #
                    for key in PlatformDataBase.Modules:
                        if InfFileKey == str((PlatformDataBase.Modules[key]).MetaFile.Path):
                            DscArchList.append (Arch)
                            break

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
        if self.UseArch is not None:
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
        DebugPath = ''
        (ModulePath, FileName) = os.path.split(self.InfFileName)
        Index = FileName.rfind('.')
        FileName = FileName[0:Index]
        if self.OverrideGuid:
            FileName = self.OverrideGuid
        Arch = "NoneArch"
        if self.CurrentArch is not None:
            Arch = self.CurrentArch

        OutputPath = os.path.join(GenFdsGlobalVariable.OutputDirDict[Arch],
                                  Arch,
                                  ModulePath,
                                  FileName,
                                  'OUTPUT'
                                  )
        DebugPath = os.path.join(GenFdsGlobalVariable.OutputDirDict[Arch],
                                  Arch,
                                  ModulePath,
                                  FileName,
                                  'DEBUG'
                                  )
        OutputPath = os.path.realpath(OutputPath)
        DebugPath = os.path.realpath(DebugPath)
        return OutputPath, DebugPath

    ## __GenSimpleFileSection__() method
    #
    #   Generate section by specified file name or a list of files with file extension
    #
    #   @param  self        The object pointer
    #   @param  Rule        The rule object used to generate section
    #   @retval string      File name of the generated section file
    #
    def __GenSimpleFileSection__(self, Rule, IsMakefile = False):
        #
        # Prepare the parameter of GenSection
        #
        FileList = []
        OutputFileList = []
        GenSecInputFile = None
        if Rule.FileName is not None:
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
        if self.ModuleType == SUP_MODULE_DXE_SMM_DRIVER and int(self.PiSpecVersion, 16) >= 0x0001000A:
            if SectionType == BINARY_FILE_TYPE_DXE_DEPEX:
                SectionType = BINARY_FILE_TYPE_SMM_DEPEX
        #
        # Framework SMM Driver has no SMM_DEPEX section type
        #
        if self.ModuleType == SUP_MODULE_DXE_SMM_DRIVER and int(self.PiSpecVersion, 16) < 0x0001000A:
            if SectionType == BINARY_FILE_TYPE_SMM_DEPEX:
                EdkLogger.error("GenFds", FORMAT_NOT_SUPPORTED, "Framework SMM module doesn't support SMM_DEPEX section type", File=self.InfFileName)
        NoStrip = True
        if self.ModuleType in (SUP_MODULE_SEC, SUP_MODULE_PEI_CORE, SUP_MODULE_PEIM):
            if self.KeepReloc is not None:
                NoStrip = self.KeepReloc
            elif Rule.KeepReloc is not None:
                NoStrip = Rule.KeepReloc
            elif self.ShadowFromInfFile is not None:
                NoStrip = self.ShadowFromInfFile

        if FileList != [] :
            for File in FileList:

                SecNum = '%d' %Index
                GenSecOutputFile= self.__ExtendMacro__(Rule.NameGuid) + \
                              SectionSuffix[SectionType] + SUP_MODULE_SEC + SecNum
                Index = Index + 1
                OutputFile = os.path.join(self.OutputPath, GenSecOutputFile)
                File = GenFdsGlobalVariable.MacroExtend(File, Dict, self.CurrentArch)

                #Get PE Section alignment when align is set to AUTO
                if self.Alignment == 'Auto' and (SectionType == BINARY_FILE_TYPE_PE32 or SectionType == BINARY_FILE_TYPE_TE):
                    ImageObj = PeImageClass (File)
                    if ImageObj.SectionAlignment < 0x400:
                        self.Alignment = str (ImageObj.SectionAlignment)
                    elif ImageObj.SectionAlignment < 0x100000:
                        self.Alignment = str (ImageObj.SectionAlignment // 0x400) + 'K'
                    else:
                        self.Alignment = str (ImageObj.SectionAlignment // 0x100000) + 'M'

                if not NoStrip:
                    FileBeforeStrip = os.path.join(self.OutputPath, ModuleName + '.reloc')
                    if not os.path.exists(FileBeforeStrip) or \
                           (os.path.getmtime(File) > os.path.getmtime(FileBeforeStrip)):
                        CopyLongFilePath(File, FileBeforeStrip)
                    StrippedFile = os.path.join(self.OutputPath, ModuleName + '.stipped')
                    GenFdsGlobalVariable.GenerateFirmwareImage(
                            StrippedFile,
                            [File],
                            Strip=True,
                            IsMakefile=IsMakefile
                        )
                    File = StrippedFile

                if SectionType == BINARY_FILE_TYPE_TE:
                    TeFile = os.path.join( self.OutputPath, self.ModuleGuid + 'Te.raw')
                    GenFdsGlobalVariable.GenerateFirmwareImage(
                            TeFile,
                            [File],
                            Type='te',
                            IsMakefile=IsMakefile
                        )
                    File = TeFile
                GenFdsGlobalVariable.GenerateSection(OutputFile, [File], Section.Section.SectionType[SectionType], IsMakefile=IsMakefile)
                OutputFileList.append(OutputFile)
        else:
            SecNum = '%d' %Index
            GenSecOutputFile= self.__ExtendMacro__(Rule.NameGuid) + \
                              SectionSuffix[SectionType] + SUP_MODULE_SEC + SecNum
            OutputFile = os.path.join(self.OutputPath, GenSecOutputFile)
            GenSecInputFile = GenFdsGlobalVariable.MacroExtend(GenSecInputFile, Dict, self.CurrentArch)

            #Get PE Section alignment when align is set to AUTO
            if self.Alignment == 'Auto' and (SectionType == BINARY_FILE_TYPE_PE32 or SectionType == BINARY_FILE_TYPE_TE):
                ImageObj = PeImageClass (GenSecInputFile)
                if ImageObj.SectionAlignment < 0x400:
                    self.Alignment = str (ImageObj.SectionAlignment)
                elif ImageObj.SectionAlignment < 0x100000:
                    self.Alignment = str (ImageObj.SectionAlignment // 0x400) + 'K'
                else:
                    self.Alignment = str (ImageObj.SectionAlignment // 0x100000) + 'M'

            if not NoStrip:
                FileBeforeStrip = os.path.join(self.OutputPath, ModuleName + '.reloc')
                if not os.path.exists(FileBeforeStrip) or \
                       (os.path.getmtime(GenSecInputFile) > os.path.getmtime(FileBeforeStrip)):
                    CopyLongFilePath(GenSecInputFile, FileBeforeStrip)

                StrippedFile = os.path.join(self.OutputPath, ModuleName + '.stipped')
                GenFdsGlobalVariable.GenerateFirmwareImage(
                        StrippedFile,
                        [GenSecInputFile],
                        Strip=True,
                        IsMakefile=IsMakefile
                    )
                GenSecInputFile = StrippedFile

            if SectionType == BINARY_FILE_TYPE_TE:
                TeFile = os.path.join( self.OutputPath, self.ModuleGuid + 'Te.raw')
                GenFdsGlobalVariable.GenerateFirmwareImage(
                        TeFile,
                        [GenSecInputFile],
                        Type='te',
                        IsMakefile=IsMakefile
                    )
                GenSecInputFile = TeFile
            GenFdsGlobalVariable.GenerateSection(OutputFile, [GenSecInputFile], Section.Section.SectionType[SectionType], IsMakefile=IsMakefile)
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
    def __GenSimpleFileFfs__(self, Rule, InputFileList, MakefilePath = None):
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

        if Rule.NameGuid is not None and Rule.NameGuid.startswith('PCD('):
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
                                             FdfFvFileTypeToFileType[Rule.FvFileType],
                                             self.ModuleGuid, Fixed=Rule.Fixed,
                                             CheckSum=Rule.CheckSum, Align=Rule.Alignment,
                                             SectionAlign=SectionAlignments,
                                             MakefilePath=MakefilePath
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
    def __GenComplexFileSection__(self, Rule, FvChildAddr, FvParentAddr, IsMakefile = False):
        if self.ModuleType in (SUP_MODULE_SEC, SUP_MODULE_PEI_CORE, SUP_MODULE_PEIM, SUP_MODULE_MM_CORE_STANDALONE):
            if Rule.KeepReloc is not None:
                self.KeepRelocFromRule = Rule.KeepReloc
        SectFiles = []
        SectAlignments = []
        Index = 1
        HasGeneratedFlag = False
        if self.PcdIsDriver == 'PEI_PCD_DRIVER':
            if self.IsBinaryModule:
                PcdExDbFileName = os.path.join(GenFdsGlobalVariable.FvDir, "PEIPcdDataBase.raw")
            else:
                PcdExDbFileName = os.path.join(self.EfiOutputPath, "PEIPcdDataBase.raw")
            PcdExDbSecName = os.path.join(self.OutputPath, "PEIPcdDataBaseSec.raw")
            GenFdsGlobalVariable.GenerateSection(PcdExDbSecName,
                                                 [PcdExDbFileName],
                                                 "EFI_SECTION_RAW",
                                                 IsMakefile = IsMakefile
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
                                                IsMakefile = IsMakefile
                                                )
            SectFiles.append(PcdExDbSecName)
            SectAlignments.append(None)
        for Sect in Rule.SectionList:
            SecIndex = '%d' %Index
            SectList  = []
            #
            # Convert Fv Section Type for PI1.1 SMM driver.
            #
            if self.ModuleType == SUP_MODULE_DXE_SMM_DRIVER and int(self.PiSpecVersion, 16) >= 0x0001000A:
                if Sect.SectionType == BINARY_FILE_TYPE_DXE_DEPEX:
                    Sect.SectionType = BINARY_FILE_TYPE_SMM_DEPEX
            #
            # Framework SMM Driver has no SMM_DEPEX section type
            #
            if self.ModuleType == SUP_MODULE_DXE_SMM_DRIVER and int(self.PiSpecVersion, 16) < 0x0001000A:
                if Sect.SectionType == BINARY_FILE_TYPE_SMM_DEPEX:
                    EdkLogger.error("GenFds", FORMAT_NOT_SUPPORTED, "Framework SMM module doesn't support SMM_DEPEX section type", File=self.InfFileName)
            #
            # process the inside FvImage from FvSection or GuidSection
            #
            if FvChildAddr != []:
                if isinstance(Sect, FvImageSection):
                    Sect.FvAddr = FvChildAddr.pop(0)
                elif isinstance(Sect, GuidSection):
                    Sect.FvAddr = FvChildAddr
            if FvParentAddr is not None and isinstance(Sect, GuidSection):
                Sect.FvParentAddr = FvParentAddr

            if Rule.KeyStringList != []:
                SectList, Align = Sect.GenSection(self.OutputPath, self.ModuleGuid, SecIndex, Rule.KeyStringList, self, IsMakefile = IsMakefile)
            else :
                SectList, Align = Sect.GenSection(self.OutputPath, self.ModuleGuid, SecIndex, self.KeyStringList, self, IsMakefile = IsMakefile)

            if not HasGeneratedFlag:
                UniVfrOffsetFileSection = ""
                ModuleFileName = mws.join(GenFdsGlobalVariable.WorkSpaceDir, self.InfFileName)
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
                    if IsMakefile:
                        if InfData.BuildType != 'UEFI_HII':
                            UniVfrOffsetFileName = os.path.join(self.OutputPath, self.BaseName + '.offset')
                            UniVfrOffsetFileSection = os.path.join(self.OutputPath, self.BaseName + 'Offset' + '.raw')
                            UniVfrOffsetFileNameList = []
                            UniVfrOffsetFileNameList.append(UniVfrOffsetFileName)
                            TrimCmd = "Trim --Vfr-Uni-Offset -o %s --ModuleName=%s --DebugDir=%s " % (UniVfrOffsetFileName, self.BaseName, self.EfiDebugPath)
                            GenFdsGlobalVariable.SecCmdList.append(TrimCmd)
                            GenFdsGlobalVariable.GenerateSection(UniVfrOffsetFileSection,
                                                                [UniVfrOffsetFileName],
                                                                "EFI_SECTION_RAW",
                                                                IsMakefile = True
                                                                )
                    else:
                        VfrUniOffsetList = self.__GetBuildOutputMapFileVfrUniInfo(VfrUniBaseName)
                        #
                        # Generate the Raw data of raw section
                        #
                        if VfrUniOffsetList:
                            UniVfrOffsetFileName = os.path.join(self.OutputPath, self.BaseName + '.offset')
                            UniVfrOffsetFileSection = os.path.join(self.OutputPath, self.BaseName + 'Offset' + '.raw')
                            FfsInfStatement.__GenUniVfrOffsetFile (VfrUniOffsetList, UniVfrOffsetFileName)
                            UniVfrOffsetFileNameList = []
                            UniVfrOffsetFileNameList.append(UniVfrOffsetFileName)
                            """Call GenSection"""

                            GenFdsGlobalVariable.GenerateSection(UniVfrOffsetFileSection,
                                                                 UniVfrOffsetFileNameList,
                                                                 "EFI_SECTION_RAW"
                                                                 )
                            #os.remove(UniVfrOffsetFileName)
                    if UniVfrOffsetFileSection:
                        SectList.append(UniVfrOffsetFileSection)
                        HasGeneratedFlag = True

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
    def __GenComplexFileFfs__(self, Rule, InputFile, Alignments, MakefilePath = None):

        if Rule.NameGuid is not None and Rule.NameGuid.startswith('PCD('):
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
                                             FdfFvFileTypeToFileType[Rule.FvFileType],
                                             self.ModuleGuid, Fixed=Rule.Fixed,
                                             CheckSum=Rule.CheckSum, Align=Rule.Alignment,
                                             SectionAlign=Alignments,
                                             MakefilePath=MakefilePath
                                             )
        return FfsOutput

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
        return GetVariableOffset(MapFileName, EfiFileName, list(VfrUniBaseName.values()))

    ## __GenUniVfrOffsetFile() method
    #
    #   Generate the offset file for the module which contain VFR or UNI file.
    #
    #   @param  VfrUniOffsetList        A list contain the VFR/UNI offsets in the EFI image file.
    #   @param  UniVfrOffsetFileName    The output offset file name.
    #
    @staticmethod
    def __GenUniVfrOffsetFile(VfrUniOffsetList, UniVfrOffsetFileName):

        # Use a instance of StringIO to cache data
        fStringIO = BytesIO()

        for Item in VfrUniOffsetList:
            if (Item[0].find("Strings") != -1):
                #
                # UNI offset in image.
                # GUID + Offset
                # { 0x8913c5e0, 0x33f6, 0x4d86, { 0x9b, 0xf1, 0x43, 0xef, 0x89, 0xfc, 0x6, 0x66 } }
                #
                UniGuid = b'\xe0\xc5\x13\x89\xf63\x86M\x9b\xf1C\xef\x89\xfc\x06f'
                fStringIO.write(UniGuid)
                UniValue = pack ('Q', int (Item[1], 16))
                fStringIO.write (UniValue)
            else:
                #
                # VFR binary offset in image.
                # GUID + Offset
                # { 0xd0bc7cb4, 0x6a47, 0x495f, { 0xaa, 0x11, 0x71, 0x7, 0x46, 0xda, 0x6, 0xa2 } };
                #
                VfrGuid = b'\xb4|\xbc\xd0Gj_I\xaa\x11q\x07F\xda\x06\xa2'
                fStringIO.write(VfrGuid)
                type (Item[1])
                VfrValue = pack ('Q', int (Item[1], 16))
                fStringIO.write (VfrValue)

        #
        # write data into file.
        #
        try :
            SaveFileOnChange(UniVfrOffsetFileName, fStringIO.getvalue())
        except:
            EdkLogger.error("GenFds", FILE_WRITE_FAILURE, "Write data to file %s failed, please check whether the file been locked or using by other applications." %UniVfrOffsetFileName, None)

        fStringIO.close ()


